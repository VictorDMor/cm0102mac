// cm-database-checks fixtures for DATABASE.CPP's save block: 0x100abfb0 (save_database).
// Every callee is scripted (their arguments, the registers they would save, and CR are recorded in guest memory):
// the opens, the per-table writers/readers, the stream calls, the zip and progress calls, the copy of a directory
// entry (which also leaves the TOC pointer where the call glue leaves it), the name compare and the two index
// resolvers. A case sets the direction, the loose flag, which directory ids exist, which tables exist, and where
// the n-th open, writer or transfer fails (with a result whose low byte is 0 but is not 0), so every message and
// every failure path (and its close order) is compared against the generated original. Run it with
// CM_UTILS_ORIGINAL=1 CM_DB_FILES_ORIGINAL=1 CM_SCREEN_MANAGER_ORIGINAL=1: the production owners of the callees
// (the table writers, the opener, the progress line) would otherwise run in place in the original and skip the script.
#include "database_checks.hpp"
#include "database_save_detail.hpp"
#include <algorithm>
#include <array>
#include <map>

namespace cm::aot::database_checks {
namespace {
using namespace database::save;
constexpr std::uint32_t entry = 0x100abfb0;
constexpr std::uint32_t cells = data + 0x1000, directory = data + 0x4000, pool = data + 0x6000, buffer = data + 0x7000,
                        flag_cell = data + 0x7800, trailer_cell = data + 0x7a00, array_data = data + 0x8000,
                        progress_object = data + 0x9000, progress_text = data + 0xbd00, tables = data + 0xd000,
                        counters = data + 0x1a000;
constexpr std::uint32_t table_room = 0x700;
constexpr std::uint32_t table_at(unsigned n) { return tables + table_room * n; }
// Counters in guest memory (the stub is the same in every run): opens, writers, transfers.
// BlockMoveData: the call glue (0x1077fc28) jumps through the descriptor the TOC word -0x7e64 points at; the
// scripted copy sits at an address no code uses.
constexpr std::uint32_t block_move = 0x107ffff0, descriptor_cell = data + 0x7c00;
constexpr std::uint32_t opens_at = counters, callees_at = counters + 4, transfers_at = counters + 8;

constexpr std::array<std::uint32_t, 17> callees{0x105d76e0, 0x105dfa10, 0x105e01d0, 0x105d8ad0, 0x105da670, 0x105df1a0,
                                                 0x105de9b0, 0x105df5c0, 0x105ded70, 0x105dfdf0, 0x105d8560, 0x105d8ef0,
                                                 0x105dac10, 0x105ddd20, 0x105dc160, 0x105dd910, 0x105de0e0};

struct Case {
    std::uint32_t writing = 1, flag = 0, split = 0, array = 1, version = 0x11111111;
    unsigned missing = 0;        // bit n: section n has no directory entry
    unsigned absent = 0;         // bit n: section n's table does not exist
    int open_fails = -1;         // the n-th open (the sections, then the index file) returns 0
    int callee_fails = -1;       // the n-th writer/reader returns a value with a zero low byte
    std::uint32_t fail_value = 0;
    int transfer_fails = -1;     // the n-th stream write/read comes back one short
    bool names_equal = false, reverse = false, duplicate = false, second_id = false;
    bool summary_overflow = false;
    std::uint32_t count_seed = 1;
    std::string text;
};

void put_toc(Cpu& c, std::int32_t offset, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(offset), value); }

// Everything the routine reaches through the TOC, each global in a cell of its own.
class World {
public:
    explicit World(Cpu& cpu, const Case& t) : c(cpu), t(t) {}
    void build() {
        put_toc(c, -0x756c, directory);
        put_toc(c, -0x7bb0, progress_object);
        put_toc(c, -0x7bb8, buffer);
        put_toc(c, -0x7bac, flag_cell);
        put_toc(c, -0x7ba8, buffer + 0x400);
        put_toc(c, -0x5618, pool);
        put_toc(c, -0x7e64, descriptor_cell);
        c.put32(descriptor_cell, block_move);
        c.put32(descriptor_cell + 4, c.r(2));
        c.put8(flag_cell, 1);
        c.put8(c.r(2) - 0x39d0, t.flag);
        c.put32(c.r(2) - 0x39d4, t.split);
        c.put32(c.r(2) - 0x39d8, t.version);
        // Tables and counts. A slot holds the address of a cell; the cell holds the table pointer or the count.
        for (unsigned n = 0; n < sections.size(); ++n) {
            const auto& s = sections[n];
            // Present tables are mapped and zeroed with a count of 1-3 records (the resolvers run for real on them).
            cell(s.table, (t.absent >> n) & 1 ? 0 : table_at(n));
            cell(s.count, (t.absent >> n) & 1 ? 0 : 1 + (n + t.count_seed) % 3);
            if (s.minus) cell(s.minus, (n + t.count_seed) % 2);
        }
        cell(-0x7b64, 3);
        cell(-0x74f8, t.array ? array_data : 0);
        for (std::uint32_t at = 0; at < 22 * table_room; at += 4) c.put32(tables + at, 0);
        // The progress object the real progress routine (0x10743120) writes its line into: a buffer and its limit.
        c.put32(progress_object + 0x2c2c, progress_text);
        c.put32(progress_object + 0x2c34, 0x1000);
        put_toc(c, -0x7ab0, trailer_cell);
        c.put8(trailer_cell, 0x5a);
        // The directory: 22 entries, one per section id (some missing), in a fixed or reversed order.
        std::array<std::uint32_t, 22> ids{};
        for (unsigned i = 0; i < ids.size(); ++i) ids[i] = i;
        if (t.reverse) std::reverse(ids.begin(), ids.end());
        for (unsigned i = 0; i < ids.size(); ++i) {
            const auto at = directory + i * 0x44;
            for (std::uint32_t b = 0; b < 0x44; b += 4) c.put32(at + b, 0x01010101 * (i + 1));
            std::uint32_t id = ids[i];
            c.put32(at + 0x34, id);
        }
        // The entry of section 8 may carry its second id (0x16); hiding the section then hides that entry.
        if (t.second_id)  // the entry of section 8 carries its second id
            for (unsigned i = 0; i < 22; ++i)
                if (c.u32(directory + i * 0x44 + 0x34) == 8) c.put32(directory + i * 0x44 + 0x34, 0x16);
        // Section ids are the table's own numbering (0-0x15), 0x16 is the second id of the section 8.
        for (unsigned n = 0; n < sections.size(); ++n)
            if ((t.missing >> n) & 1) hide(sections[n].id);
        if (t.duplicate)  // a second entry with the first section's id after it
            c.put32(directory + 21 * 0x44 + 0x34, sections[0].id);
        c.put32(opens_at, 0);
        c.put32(callees_at, 0);
        c.put32(transfers_at, 0);
    }

private:
    // The entries with an id are renumbered to one no section uses.
    void hide(std::uint32_t id) {
        for (unsigned i = 0; i < 22; ++i) {
            const auto at = directory + i * 0x44 + 0x34;
            if (c.u32(at) == id) c.put32(at, 0x7f);
            if (id == 8 && c.u32(at) == 0x16) c.put32(at, 0x7f);
        }
    }
    std::uint32_t cell(std::int32_t slot, std::uint32_t value) {
        auto found = cell_of.find(slot);
        if (found == cell_of.end()) {
            found = cell_of.emplace(slot, cells + 8 * std::uint32_t(cell_of.size())).first;
            put_toc(c, slot, found->second);
        }
        c.put32(found->second, value);
        return found->second;
    }
    Cpu& c;
    const Case& t;
    std::map<std::int32_t, std::uint32_t> cell_of;
};

// How many argument registers each scripted callee reads (the rest are scratch the original leaves anywhere).
unsigned arity(std::uint32_t pc) {
    switch (pc) {
    case 0x10727890: return 2;  // r3 the buffer, r4 pool + 0x1e6 (set earlier in the prologue)
    case 0x10747900: case 0x107766f0: case 0x10748150: case 0x10777cc0: return 1;
    case 0x105d7200: case 0x1077dc90: case 0x1077dbf0: case 0x10746280: return 2;
    case block_move: case 0x10769b60: return 3;
    case 0x10747cc0: case 0x10776360: return 4;
    case 0x107475b0: case 0x107485f0: case 0x10739890: return 5;
    case 0x1077b720: return 8;
    case 0x10768810: return 0;
    default: return 4;  // the table writers/readers
    }
}

// (CR at a call is not recorded: none of the callees saves it before writing cr0, as for the other modules.)
void record(Cpu& c) {
    const auto size = c.u32(trace_at);
    auto at = trace_at + 4 + size * 4;
    const auto add = [&](std::uint32_t value) {
        c.put32(at, value);
        at += 4;
    };
    add(c.s.pc);
    add(c.lr());
    const auto args = arity(c.s.pc);
    for (unsigned r = 3; r < 3 + args; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    c.put32(trace_at, (at - trace_at - 4) / 4);
}

bool is_callee(std::uint32_t pc) { return std::find(callees.begin(), callees.end(), pc) != callees.end(); }

bool stub(State& s, const Case& t) {
    const auto pc = s.pc;
    const bool opening = pc == 0x107475b0 || pc == 0x105d7200;
    const bool transferring = pc == 0x10747cc0 || pc == 0x10776360;
    const bool plain = pc == 0x10727890 || pc == block_move || pc == 0x10747900 || pc == 0x107766f0 ||
                       pc == 0x10748150 || pc == 0x10777cc0 || pc == 0x1077dc90 || pc == 0x1077dbf0 || pc == 0x1077b720 ||
                       pc == 0x10739890 || pc == 0x10768810 || pc == 0x10769b60 || pc == 0x10746280 || pc == 0x107485f0;
    if (!plain && !opening && !transferring && !is_callee(pc)) return false;
    Cpu c(s);
    const auto calls = c.u32(trace_at);
    record(c);
    std::uint32_t result = 0x31000000 + calls;
    if (pc == block_move) {  // BlockMoveData(src, dst, n)
        c.s.memory.copy_in(c.r(4), c.s.memory.copy_out(c.r(3), c.r(5)));
    } else if (pc == 0x10727890) {  // the progress line's buffer starts empty
        c.put8(c.r(3), 0);
    } else if (opening) {
        const auto n = c.u32(opens_at);
        c.put32(opens_at, n + 1);
        result = int(n) == t.open_fails ? 0 : 0x64000000 + n;
    } else if (is_callee(pc)) {
        const auto n = c.u32(callees_at);
        c.put32(callees_at, n + 1);
        result = int(n) == t.callee_fails ? t.fail_value : 0x7f000001 + 0x100 * n;
    } else if (transferring) {
        const auto n = c.u32(transfers_at);
        c.put32(transfers_at, n + 1);
        result = int(n) == t.transfer_fails ? c.r(5) - 1 : c.r(5);
    } else if (pc == 0x1077dc90) {
        result = t.names_equal ? 0 : 0xfffffffb;
    } else if (pc == 0x10748150 || pc == 0x10777cc0) {
        result = 0x1234;
    }
    c.set(3, result);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0x41000000 + calls * 0x100 + r);
    c.compare(0, 9, std::int32_t(calls));
    s.pc = c.lr();
    return true;
}

Fixture make(const Case& t) {
    Fixture f;
    f.entry = entry;
    f.description = t.text + " writing=" + hex_address(t.writing) + " flag=" + hex_address(t.flag);
    f.setup = [t](Cpu& c) {
        World(c, t).build();
        c.set({{3, 0x65000000}, {4, t.writing}, {5, 0x66000000}, {6, 0x67000000}});
    };
    f.stub = [t](State& s) { return stub(s, t); };
    f.summary_overflow = t.summary_overflow;
    f.results = 1;
    return f;
}
}

std::vector<Fixture> save_fixtures() {
    std::vector<Fixture> found;
    const auto add = [&](Case t, const std::string& what) {
        t.text = what;
        found.push_back(make(t));
    };
    // The direction is the low byte of the argument: 0, 1, a byte with the sign bit, and values whose low byte is 0.
    const std::array<std::uint32_t, 6> directions{0, 1, 0x80, 0xff, 0x100, 0x1234500};
    const std::array<std::uint32_t, 4> flags{0, 1, 0x80, 0xff};
    for (const auto writing : directions)
        for (const auto flag : flags)
            for (const bool overflow : {false, true}) {
                Case t;
                t.writing = writing;
                t.flag = flag;
                t.summary_overflow = overflow;
                add(t, "everything present");
            }
    for (const std::uint32_t writing : {0u, 1u}) {
        Case base;
        base.writing = writing;
        base.flag = 1;
        // The directory in reverse, a duplicate id, no flag, and a null index array.
        Case t = base;
        t.reverse = true;
        add(t, "reverse directory");
        t = base;
        t.duplicate = true;
        add(t, "duplicate id");
        t = base;
        t.second_id = true;
        add(t, "second id");
        t.missing = 1u << 18;  // section 18 is id 8: hide() also hides its 0x16 entry
        add(t, "second id, entry hidden");
        t = base;
        t.array = 0;
        add(t, "no array");
        t.flag = 0;
        add(t, "no array, no flag");
        for (const std::uint32_t split : {1u, 5u, 0xffffffffu, 0x50u, 0x7fffffffu}) {
            t = base;
            t.split = split;
            add(t, "split " + std::to_string(split));
        }
        for (const bool equal : {false, true}) {
            t = base;
            t.names_equal = equal;
            add(t, equal ? "names equal" : "names differ");
        }
        for (unsigned n = 0; n < sections.size(); ++n) {
            // The section's table exists but its entry does not (the report), and the reverse.
            t = base;
            t.missing = 1u << n;
            add(t, "no entry for section " + std::to_string(n));
            t.flag = 0;
            add(t, "no entry for section " + std::to_string(n) + ", no flag");
            // (An absent table leaves the resolvers nothing valid to follow, so these run with no flag.)
            t = base;
            t.flag = 0;
            t.absent = 1u << n;
            add(t, "no table for section " + std::to_string(n));
            t.missing = 1u << n;
            add(t, "neither for section " + std::to_string(n));
        }
        t = base;
        t.missing = 0x3fffff;
        add(t, "no entries");
        t.flag = 0;
        t.absent = 0x3fffff;
        add(t, "nothing at all");
        t = base;
        t.flag = 0;
        t.absent = 0x3fffff;
        add(t, "no tables");
        // Failures: every open, every writer/reader (zero low byte, and not zero), every stream transfer.
        for (int n = 0; n < int(sections.size()) + 1; ++n)
            for (const std::uint32_t flag : {0u, 1u}) {
                t = base;
                t.flag = flag;
                t.open_fails = n;
                add(t, "open " + std::to_string(n) + " fails");
            }
        for (int n = 0; n < int(sections.size()) + 2; ++n)
            for (const std::uint32_t value : {0u, 0x100u, 0x7fffff00u})
                for (const std::uint32_t flag : {0u, 1u}) {
                    t = base;
                    t.flag = flag;
                    t.callee_fails = n;
                    t.fail_value = value;
                    t.split = n == 11 ? 1 : 0;
                    add(t, "writer " + std::to_string(n) + " fails");
                }
        // The second part of section 10 (id 0x10) is the writer call number 11, reached only for a positive split.
        for (const std::uint32_t split : {0u, 1u, 3u, 0x30u, 0x31u, 0x50u, 0x7fffffffu, 0x80000000u})
            for (const std::uint32_t flag : {0u, 1u}) {
                t = base;
                t.flag = flag;
                t.split = split;
                t.callee_fails = 11;
                add(t, "second part fails, split " + std::to_string(split));
            }
        for (int n = 0; n < 12; ++n)
            for (const std::uint32_t flag : {0u, 1u})
                for (const std::uint32_t array : {0u, 1u}) {
                    t = base;
                    t.flag = flag;
                    t.array = array;
                    t.transfer_fails = n;
                    add(t, "transfer " + std::to_string(n) + " short");
                }
        // Differing counts.
        for (const std::uint32_t seed : {0u, 2u, 0x1000u}) {
            t = base;
            t.count_seed = seed;
            add(t, "counts seed " + std::to_string(seed));
        }
    }
    return found;
}
}
