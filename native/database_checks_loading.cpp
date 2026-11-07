// cm-database-checks fixtures for DATABASE.CPP's loading block (database_checks.hpp): the table release and
// growth wrappers, the language object (release, file load, seven accessors), the class lookup, the language
// file's name lookup and the release of every table. The allocator, the stream and file calls, the assert
// macro's calls and the language reset are scripted: every call is recorded with its registers, answers come
// from the case (success, failure, allocation failure), and each call leaves clobbered volatile registers
// and cr0 that the caller must carry through. The language object's own routines run for real.
#include "database_checks.hpp"
#include "database_adapter.hpp"
#include "guest_abi.hpp"
#include <algorithm>
#include <array>
#include <initializer_list>
#include <map>
#include <utility>

namespace cm::aot { bool database_reference(State&); }
namespace cm::aot::database_checks {
namespace {
using namespace cm::aot::guest;

constexpr std::uint32_t release_free = 0x10775170, allocate = 0x10775130, resize = 0x107751b0, operator_new = 0x10773520,
                        operator_delete = 0x107735a0, folder_path = 0x10746280, open_stream = 0x10739de0,
                        close_stream = 0x107766f0, read_marker = 0x107464d0, read_block = 0x10775fe0,
                        file_exists = 0x10739d60, delete_temporary_files = 0x10747260, split_name = 0x10739890,
                        seek_stream = 0x10777f10, show_progress = 0x10743120, service_release = 0x106a8f60,
                        service_construct = 0x106a8a50, copy_path = 0x10727890,
                        format_text = 0x1077b720, show_error = 0x10769b60;
constexpr std::array stubbed{release_free, allocate, resize, operator_new, operator_delete, folder_path, open_stream,
                             close_stream, read_marker, read_block, file_exists, delete_temporary_files, split_name,
                             format_text, show_error, seek_stream, show_progress, service_release, service_construct, copy_path};
// How many argument registers a scripted call records in the light mode (the registers its callee could save); close_stream records r4 too, which the header checks leave.
constexpr std::array<unsigned, stubbed.size()> argument_counts{1, 1, 2, 1, 1, 2, 2, 2, 1, 4, 1, 1, 5, 8, 3, 3, 5, 2, 2, 2};  // 0x10768810 is a leaf: it runs for real
constexpr std::uint32_t signature = 0xf1fffff1;

// Where things live in the fixture's memory (the trace is at data + 0x18000).
constexpr std::uint32_t counters = data + 0x1f100, header_at = data + 0x1e000, mode_cell = data + 0x80,
                        index_cell = data + 0x90, map_cell = data + 0xa0, record = data + 0x200,
                        object = data + 0x2000, blocks = data + 0x4000, map = data + 0x6000, file = data + 0x7100,
                        path = data + 0x7000, cells = data + 0x100;

struct Mix {
    std::uint32_t state;
    std::uint32_t next() { return state = state * 1664525u + 1013904223u; }
    std::uint32_t below(std::uint32_t n) { return (next() >> 8) % n; }
};

// A case: what the scripted callees answer, per stubbed entry (the last answer repeats), plus a setup.
struct Script {
    bool light = false;  // record the arguments and r13-r31 only (no scratch registers or CR)
    std::uint32_t seed = 0;
    unsigned pattern = 0;  // 1, 2: records of text with a -1 at +0x4e, and (2) an empty first byte
    std::array<std::vector<std::uint32_t>, stubbed.size()> answers;
    Script& answer(std::uint32_t entry, std::initializer_list<std::uint32_t> values) {
        answers[std::ranges::find(stubbed, entry) - stubbed.begin()] = values;
        return *this;
    }
};

// Appends the call's target, LR, the first `arguments` argument registers and r13-r31 to the trace.
void record_arguments(Cpu& c, unsigned arguments) {
    const auto size = c.u32(trace_at);
    auto at = trace_at + 4 + size * 4;
    const auto add = [&](std::uint32_t value) {
        c.put32(at, value);
        at += 4;
    };
    add(c.s.pc);
    add(c.lr());
    for (unsigned r = 3; r < 3 + arguments; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    c.put32(trace_at, (at - trace_at - 4) / 4);
}

// The owners of the scripted callees decline, so that every call (also those the generated pages make
// after a scripted call returned into them) goes through the scripted answers. Set at every probe: the
// in-process runs start from clones that may not carry the flags.
void decline_owners(State& s) {
    s.allocator_original = s.stream_buffer_original = s.stream_io_original = s.file_path_original = true;
    s.cpp_object_storage_original = s.file_controller_original = s.stdlib_core_original = true;
    s.formatted_output_original = s.string_original = s.native_service_original = true;
}

bool scripted(State& s, const Script& script) {
    decline_owners(s);
    // A scripted call returns into the middle of a routine; its generated production page would call the
    // owners of the other modules directly. The reference pages hand every call to the dispatcher instead.
    if (s.pc >= module_start && s.pc < module_end && !database_lists(s.pc)) return database_reference(s);
    const auto found = std::ranges::find(stubbed, s.pc);
    if (found == stubbed.end()) return false;
    const auto k = unsigned(found - stubbed.begin());
    Cpu c(s);
    const auto n = c.u32(counters + 4 * k);
    c.put32(counters + 4 * k, n + 1);
    const auto calls = c.u32(trace_at);
    if (script.light) record_arguments(c, argument_counts[k]);
    else record_call(c);
    const auto entry = s.pc;
    const auto& values = script.answers[k];
    std::uint32_t answer = values.empty() ? 0 : values[std::min<std::size_t>(n, values.size() - 1)];
    if (values.empty()) {
        switch (entry) {
        case allocate: case resize: answer = blocks + 0x400 * n; break;
        case operator_new: answer = object; break;
        case folder_path: answer = path; break;
        case open_stream: answer = file; break;
        case read_marker: answer = signature; break;
        case read_block: answer = c.r(5); break;
        case file_exists: answer = 1; break;
        default: break;
        }
    }
    if (entry == read_block && c.r(4) == 0x20 && c.r(5) == 1)
        for (std::uint32_t i = 0; i < 8; ++i) c.put32(c.r(3) + 4 * i, c.u32(header_at + 4 * i));
    if (entry == read_block && script.light && !(c.r(4) == 0x20 && c.r(5) == 1)) {
        // Record data: zeros, -1 and small values, so that every test of the loader goes both ways.
        std::uint32_t state = script.seed * 0x9e3779b1u + n * 0x85ebca6bu + 1;
        for (std::uint32_t i = 0; i < c.r(4) * c.r(5); ++i) {
            state = state * 1664525u + 1013904223u;
            const auto pick = (state >> 24) * 100 / 256;
            const auto at = i % c.r(4);
            if (script.pattern) {
                c.put8(c.r(3) + i, at == 0 ? (script.pattern == 2 ? 0 : 'a') : at == 0x4e ? 0xff : at == 0x34 ? 0 : at < 0x33 ? 'b' : 0);
                continue;
            }
            c.put8(c.r(3) + i, pick < 30 ? 0 : pick < 45 ? 0xff : pick < 75 ? 1 + ((state >> 8) & 0x3f) : (state >> 8) & 0xff);
        }
    }
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0x41000000 + calls * 0x100 + r);
    c.compare(0, std::int32_t(17 + calls), 29);
    c.set_cr(6, 0x5);
    c.set(3, answer);
    s.pc = c.lr();
    return true;
}

void put(Cpu& c, std::int32_t slot, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(slot), value); }

// The state every case starts from: the language byte and object slot, the header's expected counts (the TOC
// words that point at them) and a header in the file that matches them.
struct World {
    std::uint8_t mode = 2;
    std::uint32_t object_slot = 0;
    bool filled_object = false;
    std::uint32_t map_table = 0;  // content of the word TOC -0x74f8 points at
    std::uint32_t index = 0;
    std::uint32_t flag = 0;
    std::array<std::uint32_t, 8> header{3, 4, 5, 6, 7, 8, 9, 10};
    bool swapped_file = false;
};
void build(Cpu& c, const World& w) {
    put(c, -0x7ab0, mode_cell);
    c.put8(mode_cell, w.mode);
    put(c, -0x39e4, w.object_slot);
    c.put8(c.r(2) - 0x39d0, w.flag);
    put(c, -0x74f8, map_cell);
    c.put32(map_cell, w.map_table);
    for (std::uint32_t i = 0; i < 64; ++i) c.put32(map + 4 * i, (i * 7 + 3) % 11);
    c.put32(index_cell, w.index);
    c.put32(record + 0x68, w.index);
    // The expected counts (header words 1, 3, 4, 6, 7; word 5 is made to satisfy the sum with word 2).
    const std::array<std::pair<std::int32_t, unsigned>, 5> expected{{{-0x7524, 1}, {-0x7b04, 3}, {-0x7558, 4}, {-0x7568, 6}, {-0x755c, 7}}};
    for (unsigned i = 0; i < expected.size(); ++i) {
        put(c, expected[i].first, cells + 0x40 + 4 * i);
        c.put32(cells + 0x40 + 4 * i, w.header[expected[i].second]);
    }
    put(c, -0x7b64, cells + 0x60);
    c.put32(cells + 0x60, w.header[2] + w.header[5] + 0x7f);
    for (unsigned i = 0; i < 8; ++i)
        c.put32(header_at + 4 * i, w.swapped_file ? std::byteswap(w.header[i]) : w.header[i]);
    put(c, -0x5618, data + 0x7800);
    put(c, -0x7bac, data + 0x7380);
    put(c, -0x7ba8, data + 0x7400);
    if (w.filled_object) {
        for (std::uint32_t i = 0; i < 8; ++i) {
            c.put32(object + 4 * i, 2 + i);
            c.put32(object + 0x20 + 4 * i, blocks + 0x1000 + 0x100 * i);
        }
    }
}

Fixture fixture(std::uint32_t entry, std::string description, std::function<void(Cpu&)> setup, const Script& script,
                bool overflow = false) {
    Fixture f;
    f.entry = entry;
    f.description = std::move(description);
    // The owners of the scripted callees decline, so that every call (also those the generated pages make
    // after a scripted call returned into them) goes through the scripted answers.
    f.setup = std::move(setup);
    f.stub = [script](State& s) { return scripted(s, script); };
    f.summary_overflow = overflow;
    return f;
}

// Words for a table's base cell: null, a block, one above and one below 0x80000000.
constexpr std::array<std::uint32_t, 4> blocks_seen{0, data + 0x5000, 0x80000010, 0xfffffff0};

void add_release_table(std::vector<Fixture>& out) {
    for (const auto [entry, base, count] : {std::array<std::uint32_t, 3>{0x100a4c00, 0x74c4, 0x74c8},
                                            std::array<std::uint32_t, 3>{0x100a4c70, 0x74cc, 0x74d0}})
        for (const auto block : blocks_seen)
            for (const std::uint32_t flag : {0u, 1u, 0x7fu, 0x80u, 0xffu})
                for (const bool overflow : {false, true}) {
                    Script script;
                    out.push_back(fixture(
                        entry, "block " + hex_address(block) + " flag " + std::to_string(flag),
                        [=](Cpu& c) {
                            put(c, -std::int32_t(base), cells);
                            put(c, -std::int32_t(count), cells + 4);
                            c.put32(cells, block);
                            c.put32(cells + 4, 0x77);
                            c.put8(c.r(2) - 0x39d0, flag);
                        },
                        script, overflow));
                }
}

void add_grow(std::vector<Fixture>& out) {
    constexpr std::uint32_t entry = 0x100a4ce0;
    const std::array<std::int32_t, 10> counts{-3, -1, 0, 1, 2, 3, 7, 40, std::int32_t(0x80000000), -100};
    for (const auto count : counts)
        for (const std::uint32_t old_count : {0u, 1u, 5u, 0xfffffffeu})
            for (const std::uint32_t first : {0u, 3u, 0x80000000u})
                for (const std::uint32_t grown : {0u, data + 0x4000, high + 0x1000}) {
                    Script script;
                    script.answer(resize, {grown});
                    out.push_back(fixture(
                        entry,
                        "count " + std::to_string(count) + " old " + hex_address(old_count) + " first " + hex_address(first) +
                            " resize " + hex_address(grown),
                        [=](Cpu& c) {
                            put(c, -0x7a34, cells);
                            put(c, -0x7a38, cells + 4);
                            put(c, -0x751c, cells + 8);
                            put(c, -0x7518, cells + 12);
                            c.put32(cells, data + 0x3000);
                            c.put32(cells + 4, old_count);
                            c.put32(cells + 8, first);
                            c.set(3, std::uint32_t(count));
                        },
                        script, count == 2));
                }
}

void add_release_object(std::vector<Fixture>& out) {
    constexpr std::uint32_t entry = 0x100d4880;
    const std::array<std::uint32_t, 8> deletes{0, 1, 0x7fff, 0x8000, 0xffff, 0x10001, 0x18000, 0x80000000};
    for (const std::uint32_t pointer : {0u, object, high + 0x100})
        for (unsigned mask = 0; mask < 256; mask += pointer ? 1 : 85)
            for (unsigned d = 0; d < deletes.size(); d += (mask % 3 == 0 || mask == 255) ? 1 : 3) {
                Script script;
                out.push_back(fixture(
                    entry, "object " + hex_address(pointer) + " arrays " + std::to_string(mask) + " delete " + hex_address(deletes[d]),
                    [=](Cpu& c) {
                        for (std::uint32_t i = 0; i < 8 && pointer; ++i) {
                            c.put32(pointer + 0x20 + 4 * i, mask >> i & 1 ? blocks + 0x100 * (i + 1) : 0);
                        }
                        c.set(3, pointer);
                        c.set(4, deletes[d]);
                    },
                    script, mask % 5 == 0));
            }
}

void add_load(std::vector<Fixture>& out) {
    constexpr std::uint32_t entry = 0x100d4970;
    // Header variants: matching, then each compared word wrong by one, the sum wrong, byte-swapped files.
    struct Variant {
        std::string name;
        std::array<std::uint32_t, 8> header;
        bool swapped;
    };
    const std::array<std::uint32_t, 8> good{3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<Variant> variants{{"matching", good, false}, {"matching swapped", good, true}};
    for (const unsigned word : {1u, 3u, 4u, 6u, 7u}) {
        auto wrong = good;
        wrong[word] += 1;
        variants.push_back({"word " + std::to_string(word) + " wrong", wrong, false});
        variants.push_back({"word " + std::to_string(word) + " wrong swapped", wrong, true});
    }
    for (const unsigned word : {2u, 5u}) {
        auto wrong = good;
        wrong[word] += 1;
        variants.push_back({"sum wrong by word " + std::to_string(word), wrong, false});
    }
    // Wide counts: sizes wrap in 32 bits; counts of 0 give an empty allocation request.
    variants.push_back({"counts 0", {0, 4, 0, 6, 7, 0, 9, 10}, false});
    variants.push_back({"counts huge", {0x80000000, 4, 0x40000000, 6, 7, 0xffffffff, 9, 10}, true});
    for (const auto& variant : variants)
        for (const std::uint32_t marker : {signature, 0u, signature + 1, signature - 1, signature + 0x80000000u, 0xffffffffu})
            for (const std::uint32_t opened : {file, 0u, 0x80000100u}) {
                Script script;
                script.answer(open_stream, {opened}).answer(read_marker, {marker});
                World w;
                w.header = variant.header;
                w.swapped_file = variant.swapped;
                out.push_back(fixture(
                    entry, variant.name + " marker " + hex_address(marker) + " open " + hex_address(opened),
                    [=](Cpu& c) {
                        build(c, w);
                        c.set(3, object);
                    },
                    script, marker == 0));
            }
    // Allocation answers: each array failing in turn, and an answer of 0 for all.
    for (unsigned failing = 0; failing <= 8; ++failing) {
        Script script;
        std::vector<std::uint32_t> answers;
        for (unsigned i = 0; i < 8; ++i) answers.push_back(i == failing ? 0 : blocks + 0x400 * (i + 1));
        script.answers[std::ranges::find(stubbed, allocate) - stubbed.begin()] = answers;
        World w;
        out.push_back(fixture(
            entry, "allocation " + std::to_string(failing) + " fails",
            [=](Cpu& c) {
                build(c, w);
                c.set(3, object);
            },
            script, failing == 3));
    }
}

// The accessors: language byte, object slot (existing, to create, creation failing), index and the word
// at TOC -0x74f8 points at.
void add_accessors(std::vector<Fixture>& out) {
    struct Accessor {
        std::uint32_t entry;
        bool by_record;  // the argument's word at +0x68 holds the index
    };
    const std::array<Accessor, 7> accessors{{{0x100d4df0, true}, {0x100d4ee0, true}, {0x100d4fd0, false}, {0x100d50e0, false},
                                            {0x100d51d0, false}, {0x100d5310, false}, {0x100d5450, false}}};
    const std::array<std::uint32_t, 8> indices{0, 1, 4, 5, 6, 0xffffffffu, 0x80000000u, 40};
    for (const auto& accessor : accessors)
        for (const std::uint8_t mode : {0, 1, 2, 5, 0x80, 0xff})
            for (const unsigned state : {0u, 1u, 2u, 3u, 4u}) {
                // 0: object exists; 1: created and loaded; 2: created, file does not open; 3: creation fails;
                // 4: object exists but its arrays are empty.
                for (const auto index : indices)
                    for (const std::uint32_t map_table : {0u, map, high + 0x100}) {
                        // Only the two accessors that read the map word see the map.
                        const bool maps = accessor.entry == 0x100d51d0 || accessor.entry == 0x100d5310;
                        if (!maps && map_table != 0) continue;
                        Script script;
                        World w;
                        w.mode = mode;
                        w.index = index;
                        w.map_table = map_table;
                        w.object_slot = state == 0 || state == 4 ? object : 0;
                        w.filled_object = state == 0;
                        if (state == 2) script.answer(open_stream, {0});
                        if (state == 3) script.answer(operator_new, {0});
                        const auto argument = accessor.by_record ? record : index_cell;
                        out.push_back(fixture(
                            accessor.entry,
                            "mode " + std::to_string(mode) + " state " + std::to_string(state) + " index " + hex_address(index) +
                                " map " + hex_address(map_table),
                            [=](Cpu& c) {
                                build(c, w);
                                if (state == 4) for (std::uint32_t i = 0; i < 8; ++i) { c.put32(object + 4 * i, 0); c.put32(object + 0x20 + 4 * i, 0); }
                                c.set(3, argument);
                            },
                            script, index == 5));
                    }
            }
}

// The class lookup's switch tables, as the original binary has them: handler blocks `li r0,n; b` 8 bytes
// apart, a table that repeats its last handler up to the 36th entry.
void write_tables(Cpu& c) {
    struct Table {
        std::int32_t slot;
        std::uint32_t first, distinct;
    };
    constexpr std::array<Table, 7> tables{{{-0x5694, 0x100d5588, 34}, {-0x5698, 0x100d56c4, 22}, {-0x569c, 0x100d57a0, 22},
                                           {-0x56a0, 0x100d587c, 22}, {-0x56a4, 0x100d5958, 32}, {-0x56a8, 0x100d5a84, 34},
                                           {-0x56ac, 0x100d5bc0, 35}}};
    for (unsigned t = 0; t < tables.size(); ++t) {
        const auto at = data + 0x3000 + 0x100 * t;
        put(c, tables[t].slot, at);
        c.put32(at, 0x100d5cd4);
        for (std::uint32_t i = 1; i < 36; ++i) c.put32(at + 4 * i, tables[t].first + 8 * (std::min(i, tables[t].distinct) - 1));
    }
}

void add_class(std::vector<Fixture>& out) {
    constexpr std::uint32_t entry = 0x100d5540;
    std::vector<std::int32_t> kinds{-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0x7f, 0x80, 0x103, 0x109, 0x10a, -250};
    std::vector<std::int32_t> indexes;
    for (std::int32_t i = -2; i <= 0x26; ++i) indexes.push_back(i);
    for (const std::int32_t i : {0x7f, 0x80, 0xff, 0x100, 0x123, 0x10c, -128, -129})
        indexes.push_back(i);
    for (const auto kind : kinds)
        for (const auto index : indexes) {
            Script script;
            out.push_back(fixture(
                entry, "index " + std::to_string(index) + " kind " + std::to_string(kind),
                [=](Cpu& c) {
                    write_tables(c);
                    c.set(3, std::uint32_t(index));
                    c.set(4, std::uint32_t(kind));
                },
                script, (index & 3) == 1));
        }
}

void add_file_exists(std::vector<Fixture>& out) {
    constexpr std::uint32_t entry = 0x100b8b80;
    for (std::int32_t language = -3; language <= 13; ++language)
        for (const std::uint32_t pad : {0u, 0x100u, 0xffffff00u})
            for (const std::uint32_t exists : {0u, 1u}) {
                Script script;
                script.answer(file_exists, {exists});
                out.push_back(fixture(
                    entry, "language " + std::to_string(language) + " pad " + hex_address(pad),
                    [=](Cpu& c) {
                        put(c, -0x5618, data + 0x7800);
                        c.set(3, std::uint32_t(language) + pad);
                    },
                    script, language == 4));
            }
}

// Every table's base word empty or holding a block, the flag byte, the language object present or not.
void add_release_all(std::vector<Fixture>& out) {
    constexpr std::uint32_t entry = 0x100a4e50;
    // The words whose content the routine tests (TOC slots holding the base words' addresses; -0x39e0 is
    // the word in the TOC itself), and the slots whose words it clears.
    constexpr std::array<std::int32_t, 23> bases{-0x7520, -0x7548, -0x7544, -0x7540, -0x7bbc, -0x753c, -0x7538, -0x7534,
                                                  -0x7530, -0x752c, -0x7ab4, -0x7518, -0x74f8, -0x7a34, -0x7b98, -0x74cc,
                                                  -0x74c4, -0x79f8, -0x7b38, -0x74bc, -0x7be0, -0x7be8, -0x39e0};
    constexpr std::array<std::int32_t, 23> mirrors{-0x7524, -0x7568, -0x7564, -0x7560, -0x7b64, -0x755c, -0x7558, -0x7554,
                                                    -0x7550, -0x754c, -0x7528, -0x751c, -0x7500, -0x7a38, -0x7b04, -0x74d0,
                                                    -0x74c8, -0x79f4, -0x74d4, -0x74c0, -0x7be4, -0x7b50, -0x39dc};
    const auto build_all = [=](Cpu& c, std::uint32_t mask, std::uint32_t flag, std::uint32_t language_object,
                               unsigned object_mask) {
        for (unsigned i = 0; i < bases.size(); ++i) {
            const bool occupied = mask >> i & 1;
            const auto content = occupied ? blocks + 0x40 * i : 0;
            if (bases[i] == -0x39e0) {
                c.put32(c.r(2) - 0x39e0, content);
                c.put32(c.r(2) - 0x39dc, 0x55);
                continue;
            }
            const auto base_cell = cells + 8 * i, mirror_cell = cells + 0x400 + 8 * i;
            put(c, bases[i], base_cell);
            put(c, mirrors[i], mirror_cell);
            c.put32(base_cell, content);
            c.put32(mirror_cell, 0x66 + i);
        }
        put(c, -0x74fc, cells + 0x800);
        put(c, -0x7ab0, mode_cell);
        c.put8(mode_cell, 3);
        put(c, -0x5618, data + 0x7800);
        c.put8(c.r(2) - 0x39d0, flag);
        c.put32(c.r(2) - 0x39e4, language_object);
        for (std::uint32_t i = 0; i < 8; ++i) c.put32(object + 0x20 + 4 * i, object_mask >> i & 1 ? blocks + 0x100 * (i + 1) : 0);
    };
    Mix mix{0x4e50};
    constexpr std::uint32_t all = (1u << 23) - 1;
    std::vector<std::uint32_t> masks{0, all};
    for (unsigned i = 0; i < 23; ++i) masks.push_back(1u << i);
    for (unsigned i = 0; i < 23; ++i) masks.push_back(all & ~(1u << i));
    for (unsigned n = 0; n < 150; ++n) masks.push_back(mix.next() >> 9);
    for (const auto mask : masks)
        for (const std::uint32_t flag : {0u, 1u, 0x80u}) {
            const auto language_object = mix.below(3) == 0 ? 0u : object;
            const auto object_mask = mix.below(256);
            Script script;
            out.push_back(fixture(
                entry, "tables " + hex_address(mask) + " flag " + std::to_string(flag) + " object " + hex_address(language_object),
                [=](Cpu& c) { build_all(c, mask, flag, language_object, object_mask); }, script, mix.below(4) == 0));
        }
    for (const std::uint32_t flag : {0u, 1u, 0xffu, 0x40u, 0x100u}) {
        Script script;
        out.push_back(fixture(
            entry, "everything flag " + std::to_string(flag),
            [=](Cpu& c) { build_all(c, all, flag, object, 255); }, script, flag == 1));
    }
}
// The language file loader: the world is the club table and the seven others (random rows, club names that
// sometimes agree), the TOC words that point at them and at their counts, the file's header (matching the
// counts, or wrong in one compare, or byte-swapped) and the answers of the scripted calls (the open, the
// signature marker, each allocation).
struct LoadCase {
    std::int32_t language = 0;
    std::uint32_t second = 0;
    std::uint8_t current = 0xff;      // the language byte
    bool tables_loaded = true;        // the club table's base word is not null
    std::array<std::uint32_t, 8> header{2, 2, 2, 2, 2, 2, 2, 2};
    unsigned wrong = 0;               // 1..6: the compare of that position fails
    bool swapped = false;
    std::uint32_t marker = signature, opened = file;
    bool mapped = false;
    std::uint32_t clubs = 2, seed = 1, alloc_failures = 0;
    bool object_exists = false, service_exists = false;
    unsigned pattern = 0;
};

void language_world(Cpu& c, const LoadCase& k) {
    const auto cell = [](unsigned i) { return data + 0x100 + 8 * i; };
    const auto bind = [&](std::int32_t slot, unsigned i, std::uint32_t content) {
        put(c, slot, cell(i));
        c.put32(cell(i), content);
    };
    const auto& h = k.header;
    constexpr std::uint32_t clubs_at = data + 0x10000, table_28 = data + 0x11000, table_25 = data + 0x12000,
                            table_22 = data + 0x13000, table_21 = data + 0x14000, table_24 = data + 0x15000,
                            table_23 = data + 0x16000, map_at = data + 0x17000;
    bind(-0x7b98, 0, k.tables_loaded ? table_28 : 0);
    bind(-0x7b64, 1, h[5] + h[2] + 0x7f + (k.wrong == 4));
    bind(-0x7548, 2, table_21);
    bind(-0x7bbc, 3, table_22);
    bind(-0x753c, 4, table_23);
    bind(-0x7538, 5, table_24);
    bind(-0x7520, 6, table_25);
    bind(-0x74f8, 7, k.mapped ? map_at : 0);
    bind(-0x7b04, 8, h[3] + (k.wrong == 2));
    bind(-0x7b50, 9, k.clubs);
    bind(-0x7be8, 10, clubs_at);
    bind(-0x7ab0, 11, 0);
    c.put8(cell(11), k.current);
    bind(-0x7bb0, 12, 0x1234);
    bind(-0x7570, 13, k.service_exists ? data + 0x3000 : 0);
    bind(-0x7524, 14, h[1] + (k.wrong == 1));
    bind(-0x7558, 15, h[4] + (k.wrong == 3));
    bind(-0x7568, 16, h[6] + (k.wrong == 5));
    bind(-0x755c, 17, h[7] + (k.wrong == 6));
    c.put32(c.r(2) - 0x39e4, k.object_exists ? object : 0);
    for (std::uint32_t i = 0; i < 8; ++i) {
        c.put32(object + 0x20 + 4 * i, k.object_exists && (i + k.seed) % 3 ? blocks + 0x100 * i : 0);
        c.put32(header_at + 4 * i, k.swapped ? std::byteswap(h[i]) : h[i]);
    }
    c.put8(path, 'e');
    c.put8(path + 1, 0);
    for (std::uint32_t i = 0; i < 4; ++i) c.put32(map_at + 4 * i, (i * 5 + k.seed) % 4);
    // Rows of random bytes: zeros, -1 and small values; the clubs' two names agree in half of the rows.
    std::uint32_t state = k.seed * 2654435761u + 7;
    const auto fill = [&](std::uint32_t at, std::uint32_t size) {
        for (std::uint32_t i = 0; i < size; ++i) {
            state = state * 1664525u + 1013904223u;
            const auto pick = (state >> 24) * 100 / 256;
            c.put8(at + i, pick < 25 ? 0 : pick < 40 ? 0xff : pick < 70 ? 1 + ((state >> 8) & 0x3f) : (state >> 8) & 0xff);
        }
    };
    fill(clubs_at, 4 * 0x248);
    for (std::uint32_t row = 0; row < 4; ++row) {
        state = state * 1664525u + 1013904223u;
        if (state >> 31)
            for (std::uint32_t i = 0; i < 0x33; ++i) c.put8(clubs_at + row * 0x248 + 0x221 + i, c.u8(clubs_at + row * 0x248 + 0x1ee + i));
    }
    fill(table_28, 4 * 0x128);
    fill(table_25, 4 * 0xc8);
    fill(table_22, 8 * 0x6c);
    fill(table_21, 4 * 0x50);
    fill(table_24, 4 * 0x38);
    fill(table_23, 4 * 0x68);
    c.set(3, std::uint32_t(k.language));
    c.set(4, k.second);
}

void add_language_file(std::vector<Fixture>& out) {
    constexpr std::uint32_t entry = 0x100b6050;
    Mix mix{0xb6050};
    const auto add = [&](const LoadCase& k, const std::string& what) {
        Script script;
        script.light = true;
        script.seed = k.seed;
        script.pattern = k.pattern;
        script.answer(open_stream, {k.opened}).answer(read_marker, {k.marker});
        std::vector<std::uint32_t> blocks_answered;
        for (std::uint32_t n = 0; n < 24; ++n) blocks_answered.push_back(k.alloc_failures >> n & 1 ? 0 : blocks + 0x300 * n);
        script.answers[std::ranges::find(stubbed, allocate) - stubbed.begin()] = blocks_answered;
        script.answer(operator_new, {object + 0x100});
        out.push_back(fixture(
            entry,
            what + " language " + std::to_string(k.language) + " second " + std::to_string(k.second) + " current " +
                std::to_string(k.current) + " seed " + std::to_string(k.seed) + " failures " + hex_address(k.alloc_failures),
            [=](Cpu& c) { language_world(c, k); }, script, k.seed % 5 == 0));
    };
    const auto random_header = [&] {
        std::array<std::uint32_t, 8> h{};
        for (auto& word : h) word = mix.below(4);
        return h;
    };
    // Every language with its second argument, loaded for real, with random counts and rows.
    for (const std::int32_t language : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0x80, 0xff, 0x105})
        for (const std::uint32_t second : {0u, 1u, 5u, 0x100u})
            for (unsigned round = 0; round < 3; ++round) {
                LoadCase k;
                k.language = language;
                k.second = second;
                k.header = random_header();
                k.seed = mix.next() >> 8;
                k.mapped = round == 1;
                k.clubs = mix.below(4);
                k.object_exists = round == 2;
                k.service_exists = round != 0;
                k.current = round == 0 ? 0xff : 7;
                add(k, "language");
            }
    // Allocation failures: every single failure, pairs and all of them, with the nesting English load.
    for (const std::int32_t language : {0, 5, 3})
        for (const bool mapped : {false, true}) {
            std::vector<std::uint32_t> masks{0xffffff};
            for (unsigned n = 0; n < (language == 0 ? 8u : 16u); ++n) masks.push_back(1u << n);
            for (unsigned n = 0; n < 6; ++n) masks.push_back(mix.next() & 0xffff);
            for (const auto mask : masks) {
                LoadCase k;
                k.language = language;
                k.header = random_header();
                k.seed = mix.next() >> 8;
                k.mapped = mapped;
                k.clubs = 1 + mix.below(3);
                k.alloc_failures = mask;
                add(k, "allocation");
            }
        }
    // Structured records (-1 in the flag byte, no second name): the club rows' -1 branches, with a header word 3
    // of 0 so that every club index is below the limit.
    for (const std::int32_t language : {0, 5, 2})
        for (const bool mapped : {false, true})
            for (const unsigned pattern : {1u, 2u})
                for (const std::uint32_t failures : {0u, 0xffffffu, 1u}) {
                    LoadCase k;
                    k.language = language;
                    k.header = random_header();
                    k.header[3] = 0;
                    k.mapped = mapped;
                    k.pattern = pattern;
                    k.clubs = 1 + mix.below(3);
                    k.seed = mix.next() >> 8;
                    k.alloc_failures = failures;
                    add(k, "pattern");
                }
    // The header: each compare wrong, swapped files with and without the signature, a missing file, no club
    // table, the language already current.
    for (const std::int32_t language : {0, 4})
        for (unsigned wrong = 0; wrong <= 6; ++wrong)
            for (const bool swapped : {false, true})
                for (const std::uint32_t marker : {signature, 0u, signature + 0x80000000u}) {
                    LoadCase k;
                    k.language = language;
                    k.header = random_header();
                    k.wrong = wrong;
                    k.swapped = swapped;
                    k.marker = marker;
                    k.seed = mix.next() >> 8;
                    add(k, "header");
                }
    for (const std::int32_t language : {0, 4, 99}) {
        LoadCase k;
        k.language = language;
        k.seed = 3;
        k.opened = 0;
        add(k, "file missing");
        k.opened = file;
        k.tables_loaded = false;
        add(k, "no club table");
        k.tables_loaded = true;
        k.current = std::uint8_t(language);
        add(k, "current language");
    }
}
}

std::vector<Fixture> loading_fixtures() {
    std::vector<Fixture> out;
    add_release_table(out);
    add_grow(out);
    add_release_object(out);
    add_load(out);
    add_accessors(out);
    add_class(out);
    add_file_exists(out);
    add_release_all(out);
    add_language_file(out);
    return out;
}
}
