// cm-database-checks fixtures for DATABASE.CPP's load block (database_checks.hpp): 0x100a55a0, the database loader.
//
// Every call outside DATABASE.CPP is scripted: the allocator hands out blocks of one big region, the files are a
// byte image the read stubs copy from (positions in guest memory, so every run sees the same), the 18 table
// readers answer success or failure (the people reader fills rows of mixed content, so that the fix-up meets
// each of its tests), and every call is recorded with its argument registers and r13-r31 (a hash after the first
// calls, the trace being small). DATABASE.CPP's own callees run for real. A stub leaves clobbered volatile
// registers and cr0 that the caller must carry through.
#include "database_checks.hpp"
#include "database_adapter.hpp"
#include "guest_abi.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace cm::aot { bool database_reference(State&); }
namespace cm::aot::database_checks {
namespace {
using namespace cm::aot::guest;

struct Callee {
    std::uint32_t entry;
    unsigned arguments;
};
constexpr std::uint32_t folder_path = 0x10746280, copy_text = 0x1077dbf0, copy_path = 0x10727890, progress = 0x10743120,
                        open_path = 0x10748460, reset_language = 0x1013fb60, find_first = 0x10739b40,
                        find_next = 0x10739690, find_name = 0x107470b0, find_step = 0x107397d0,
                        packed_name = 0x10746470, packed_open = 0x107475b0, plain_open = 0x105d71b0,
                        read_marker = 0x107464d0, read_packed = 0x10747a80, read_plain = 0x10775fe0,
                        reread_packed = 0x10747cc0, reread_plain = 0x10776360, close_packed = 0x10747900,
                        close_plain = 0x107766f0, allocate = 0x10775130, release = 0x10775170, split_name = 0x10739890,
                        format_text = 0x1077b720, show_error = 0x10769b60, abort_run = 0x1076f7d0,
                        seek_packed = 0x10747fe0, seek_plain = 0x10777f10, reopen = 0x105d7200, date_make = 0x107156f0,
                        flush = 0x10776690, finish = 0x1014d560, random = 0x1077b770, file_exists = 0x10739d60;
constexpr std::array<std::uint32_t, 18> readers{0x105d8f60, 0x105dfa80, 0x105d7250, 0x105d7750, 0x105d8b40, 0x105df630,
                                                0x105db230, 0x105dc1d0, 0x105dede0, 0x105de150, 0x105dfe60, 0x105dea20,
                                                0x105d85d0, 0x105dd980, 0x105dac80, 0x105da6e0, 0x105ddd90, 0x105df210};
constexpr std::uint32_t people_reader = 0x105da6e0;
constexpr std::array<Callee, 36> plain_callees{{
    {0x10747260, 1},
    {folder_path, 2}, {copy_text, 2}, {copy_path, 2}, {progress, 5}, {open_path, 3}, {reset_language, 2},
    {find_first, 5}, {find_next, 3}, {find_name, 2}, {find_step, 1}, {packed_name, 3},
    {packed_open, 5}, {plain_open, 2}, {read_marker, 1}, {read_packed, 4}, {read_plain, 4}, {reread_packed, 4},
    {reread_plain, 4}, {close_packed, 1}, {close_plain, 1}, {allocate, 1}, {release, 1}, {split_name, 5},
    {format_text, 8}, {show_error, 3}, {abort_run, 1}, {seek_packed, 3}, {seek_plain, 3}, {reopen, 2},
    {date_make, 5}, {flush, 1}, {finish, 0}, {random, 0}, {file_exists, 1}, {0x1013fb60 + 1, 0}}};

std::uint32_t arguments_of(std::uint32_t entry) {
    for (const auto& callee : plain_callees)
        if (callee.entry == entry) return callee.arguments;
    return 4;  // the readers
}
bool stubbed(std::uint32_t entry) {
    for (const auto& callee : plain_callees)
        if (callee.entry == entry) return true;
    return std::ranges::find(readers, entry) != readers.end();
}

// Where things live (the trace is at data + 0x18000).
constexpr std::uint32_t cells_at = data + 0x1000, entries_at = data + 0x4000,
                        pool_at = data + 0x6000, path_at = data + 0x7000, file_handle = data + 0x7100,
                        language_object = data + 0x7200, language_byte = data + 0x7240, flag_cells = data + 0x7260,
                        sample_at = data + 0x7400, text_buffer = data + 0x7a00,
                        message_at = data + 0x7c00, clubs_at = data + 0x9000,
                        name_text = data + 0x9800, small_state = data + 0x1f000, big = 0x74000000, big_size = 0x800000;
constexpr std::uint32_t counters = small_state, bump_cell = small_state + 0x400, position_cell = small_state + 0x404,
                        hash_cell = small_state + 0x408, call_cell = small_state + 0x40c, close_cell = small_state + 0x410;
constexpr std::uint32_t signature = 0xf1fffff1;
constexpr unsigned recorded_calls = 120;
constexpr std::uint32_t natural = 0xfffffffd;

struct Entry {
    std::uint32_t kind, count, offset, flag;
};

// A case: the arguments, the file and what the scripted callees answer.
struct Case {
    std::string name;
    std::uint32_t single = 0, mode = 0;
    bool second_name = false;               // the first name does not open, the second does
    bool swapped = false;                   // plain files: the marker differs and the words are in the other order
    std::uint32_t word_a = 0x1234, word_b = 0x10;
    bool extended = false;                  // the language block: flag byte, 0x1cc bytes, map words, the language byte
    std::uint32_t map_count = 0xffffffff;
    std::array<Entry, 22> entries;
    std::map<std::uint32_t, std::vector<std::uint32_t>> answers;
    unsigned zero_at = ~0u;                 // the close that releases kinds 17 and 18's tables behind the loader's back
    std::uint32_t image_limit = 0;          // truncate the image to this many bytes (0: whole)
    std::uint32_t language = 3;
    bool filled_tables = true;              // the people reader fills rows; otherwise they stay zero
    Case& answer(std::uint32_t entry, std::initializer_list<std::uint32_t> values) {
        answers[entry] = values;
        return *this;
    }
};

std::vector<std::uint8_t> make_image(const Case& k, bool swapped_layout) {
    std::vector<std::uint8_t> out;
    if ((k.mode & 0xff) == 0 && (k.swapped || k.second_name))  // the second name's plain file starts with the marker word
        for (const auto byte : {0xf1, 0xff, 0xff, 0xf1}) out.push_back(std::uint8_t(byte));
    const auto put32 = [&](std::uint32_t v) {
        if (swapped_layout) v = std::byteswap(v);
        for (int shift = 24; shift >= 0; shift -= 8) out.push_back(std::uint8_t(v >> shift));
    };
    put32(k.word_a);
    put32(k.word_b);
    for (const auto& e : k.entries) {
        for (unsigned i = 0; i < 0x33; ++i) out.push_back(std::uint8_t('a' + i % 26));
        if (!swapped_layout) out.push_back(0);
        put32(e.kind);
        put32(e.count);
        put32(e.offset);
        put32(e.flag);
    }
    out.push_back(k.extended ? 1 : 0);
    if (k.extended) {
        for (unsigned i = 0; i < 0x1cc; ++i) out.push_back(std::uint8_t(i));
        for (const auto byte : {std::uint8_t(k.map_count >> 24), std::uint8_t(k.map_count >> 16), std::uint8_t(k.map_count >> 8), std::uint8_t(k.map_count)}) out.push_back(byte);
        if (k.map_count != 0xffffffff)
            for (unsigned i = 0; i < k.map_count * 4 + 16; ++i) out.push_back(std::uint8_t(i * 3));
        out.push_back(2);
    }
    while (out.size() < 0x800) out.push_back(0x5a);
    if (k.image_limit) out.resize(k.image_limit);
    return out;
}

void put(Cpu& c, std::int32_t slot, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(slot), value); }

void record_arguments(Cpu& c, unsigned arguments) {
    const auto calls = c.u32(call_cell);
    c.put32(call_cell, calls + 1);
    if (calls >= recorded_calls) {
        auto hash = c.u32(hash_cell);
        const auto mix = [&](std::uint32_t v) { hash = (hash ^ v) * 0x01000193u; };
        mix(c.s.pc);
        mix(c.lr());
        for (unsigned r = 3; r < 3 + arguments; ++r) mix(c.r(r));
        for (unsigned r = 13; r < 32; ++r) mix(c.r(r));
        c.put32(hash_cell, hash);
        return;
    }
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

void decline_owners(State& s) {
    s.allocator_original = s.stream_buffer_original = s.stream_io_original = s.file_path_original = true;
    s.cpp_object_storage_original = s.file_controller_original = s.stdlib_core_original = true;
    s.formatted_output_original = s.string_original = s.native_service_original = true;
}

// A person row of mixed content: links to club-like records, role bytes and flags picked from the index.
void fill_person(Cpu& c, std::uint32_t row, std::uint32_t i, bool indexes) {
    const auto h = i * 2654435761u >> 7;
    // Pointers to club-like records; in the single-block form the fields hold indexes that 0x100af8a0 resolves.
    const auto club = [&](std::uint32_t n) { return indexes ? (n % 4 == 3 ? 0xffffffffu : n % 3) : clubs_at + 0x248 * (n % 4); };
    // Role values: every switch case, 16, and bytes above 0x10 (one with the sign bit).
    constexpr std::array<std::uint8_t, 21> roles{0, 1, 5, 7, 11, 12, 13, 15, 3, 14, 2, 4, 8, 9, 10, 16, 0x11, 0x7f, 0x80, 0xff, 6};
    c.put32(row, i);
    c.put32(row + 0xc, h & 1 ? club(h >> 20) : 0);
    c.put32(row + 0x14, h >> 1 & 1);
    c.put32(row + 0x1c, h >> 2 & 1);
    c.put32(row + 0x20, h >> 3 & 1 ? club(h >> 4) : 0);
    c.put32(row + 0x24, (h >> 5 & 3) == 0 ? 0 : club(h >> 7));
    // A club-like record's slot that names this person (role 11 clears it).
    if (!indexes && (h >> 5 & 3) != 0) c.put32(club(h >> 7) + 0x98 + 4 * (i % 50), row);
    c.put32(row + 0x28, h >> 9 & 1 ? (indexes ? 0xffffffffu : name_text) : 0);
    c.put32(row + 0x2c, h >> 10 & 1 ? (indexes ? 0xffffffffu : name_text) : 0);
    c.put32(row + 0x30, h >> 11 & 1 ? (indexes ? 0xffffffffu : name_text) : 0);
    c.put16(row + 0x4c, 0x7ce + (h >> 3 & 3) - 1);
    c.put16(row + 0x4e, (h >> 6 & 3));
    c.put8(row + 0x5e, h >> 12 & 1);
    c.put8(row + 0x5f, h >> 13 & 1);
    c.put8(row + 0x60, roles[(h >> 14) % roles.size()]);
    c.put8(row + 0x62, roles[(h >> 17) % roles.size()]);
    c.put8(row + 0x6a, h >> 19 & 3);
    c.put8(row + 0x6b, (h >> 5) % 7);
}

bool scripted(State& s, const Case& k, const std::vector<std::uint8_t>& image) {
    decline_owners(s);
    if (s.pc >= module_start && s.pc < module_end && !database_lists(s.pc)) return database_reference(s);
    if (!stubbed(s.pc)) return false;
    Cpu c(s);
    const auto entry = s.pc;
    unsigned slot = 0;
    while (slot < plain_callees.size() && plain_callees[slot].entry != entry) ++slot;
    if (slot == plain_callees.size()) slot += unsigned(std::ranges::find(readers, entry) - readers.begin());
    const auto n = c.u32(counters + 4 * slot);
    c.put32(counters + 4 * slot, n + 1);
    record_arguments(c, arguments_of(entry));
    std::uint32_t answer = 0;
    bool has = false;
    if (const auto found = k.answers.find(entry); found != k.answers.end() && !found->second.empty()) {
        answer = found->second[std::min<std::size_t>(n, found->second.size() - 1)];
        has = answer != natural;  // `natural` asks for the stub's own answer at that call
        if (!has) answer = 0;
    }
    switch (entry) {
    case allocate: {
        const auto at = c.u32(bump_cell);
        c.put32(bump_cell, at + ((c.r(3) + 15) & ~15u));
        if (!has) answer = at;
        break;
    }
    case close_packed: case close_plain: {
        const auto closes = c.u32(close_cell);
        c.put32(close_cell, closes + 1);
        if (closes == k.zero_at)
            for (const auto slot : {-0x74c4, -0x74cc}) c.put32(c.toc(slot), 0);
        break;
    }
    case folder_path: if (!has) answer = path_at; break;
    case open_path: if (!has) answer = 1; break;
    case find_next: case find_step: if (!has) answer = 1; break;
    case packed_open: case plain_open: case reopen:
        c.put32(position_cell, 0);
        if (!has) answer = file_handle;
        break;
    case read_marker:
        c.put32(position_cell, c.u32(position_cell) + 4);
        if (!has) answer = k.swapped ? signature ^ 0x100 : signature;
        break;
    case file_exists: if (!has) answer = 1; break;
    case seek_packed: case seek_plain: c.put32(position_cell, c.r(4)); break;
    case read_packed: case read_plain: case reread_packed: case reread_plain: {
        const auto position = c.u32(position_cell);
        const auto size = c.r(4), count = c.r(5);
        std::uint32_t got = 0;
        for (; got < count; ++got) {
            if (position + (got + 1) * size > image.size()) break;
            for (std::uint32_t b = 0; b < size; ++b) c.put8(c.r(3) + got * size + b, image[position + got * size + b]);
        }
        c.put32(position_cell, position + got * size);
        answer = has ? answer : got;
        break;
    }
    case date_make: {
        c.put32(c.r(3), 0x07ce0000 | (n & 3));
        c.put32(c.r(3) + 4, 0x00010000 + n);
        if (!has) answer = c.r(3);
        break;
    }
    default:
        if (std::ranges::find(readers, entry) != readers.end() && !has) answer = 1;
        break;
    }
    if (entry == people_reader && k.filled_tables && answer != 0 && c.r(5) != 0) {
        const auto rows = c.u32(c.toc(-0x7be4));
        for (std::uint32_t i = 0; i < rows && i < 0x4000; ++i) fill_person(c, c.r(5) + i * 0x70, i, k.single != 0);
    }
    if (entry == 0x105d8f60 && answer != 0 && c.r(5) != 0) {
        // The clubs: each links to another club row whose +0x68 picks the club the final pass halves.
        const auto first = c.u32(c.toc(-0x7be8));
        const auto rows = c.r(5) == first ? c.u32(c.toc(-0x7b50)) : c.u32(c.toc(-0x7500));
            for (std::uint32_t i = 0; i < rows && i < 64; ++i) {
            const auto row = c.r(5) + i * 0x248;
            c.put32(row + 0x2c, k.single ? 1u : c.r(5) + ((i + 1) % rows) * 0x248);  // an index the single-block form resolves
            c.put32(row + 0x68, i & 1);
            c.put16(row + 0x1ec, (i & 1) ? 0xff9c : i * 1000 + 7);
        }
    }
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0x41000000 + c.u32(call_cell) * 0x100 + r);
    c.compare(0, std::int32_t(17 + c.u32(call_cell)), 29);
    c.set_cr(6, 0x5);
    c.set(3, answer);
    s.pc = c.lr();
    return true;
}
}

constexpr std::array<std::int32_t, 44> cell_slots{
    -0x7b50, -0x7500, -0x7564, -0x7524, -0x7b04, -0x7568, -0x7be4, -0x7528, -0x79f4, -0x74d4, -0x755c, -0x7b64, -0x754c, -0x7550,
    -0x7554, -0x7560, -0x74c8, -0x74d0, -0x7a38, -0x751c, -0x7558, -0x74c0, -0x7be8, -0x74fc, -0x7544, -0x7520, -0x7b98, -0x7548,
    -0x7be0, -0x7ab4, -0x79f8, -0x7b38, -0x753c, -0x7bbc, -0x752c, -0x7530, -0x7534, -0x7540, -0x74c4, -0x74cc, -0x7a34, -0x7518,
    -0x7538, -0x74bc};

// The world: the TOC words the loader and its real callees read, the sample pool, the club-like records the
// people link to, and the big region the allocator hands out.
void build(Cpu& c, const Case& k) {
    c.s.memory.map(big, big_size);
    for (std::uint32_t i = 0; i < 0x100; ++i) c.put32(counters + 4 * i, 0);
    c.put32(close_cell, 0);
    c.put32(bump_cell, big);
    c.put32(position_cell, 0);
    c.put32(hash_cell, 0x811c9dc5);
    c.put32(call_cell, 0);
    for (unsigned i = 0; i < cell_slots.size(); ++i) {
        put(c, cell_slots[i], cells_at + 8 * i);
        c.put32(cells_at + 8 * i, 0);
    }
    put(c, -0x7ac8, flag_cells);
    c.put32(flag_cells, language_object);
    c.put8(language_object + 8, k.language);
    put(c, -0x7ab0, language_byte);
    c.put8(language_byte, 0x77);
    put(c, -0x74f8, flag_cells + 8);
    c.put32(flag_cells + 8, 0);
    put(c, -0x756c, entries_at);
    put(c, -0x5618, pool_at);
    put(c, -0x7bac, data + 0x7380);
    put(c, -0x7ba8, message_at);
    put(c, -0x7bb0, 0x1234);
    put(c, -0x7bb8, text_buffer);
    for (std::uint32_t i = 0; i < 0x100; ++i) c.put16(sample_at + 2 * i, std::uint16_t((i * 7919u + 13u) >> 1));
    put(c, -0x7b84, flag_cells + 0x10);
    c.put32(flag_cells + 0x10, sample_at + 0x40);
    put(c, -0x7b88, flag_cells + 0x18);
    c.put32(flag_cells + 0x18, sample_at + 0x44);
    put(c, -0x7b8c, sample_at);
    put(c, -0x7b90, flag_cells + 0x28);
    c.put32(flag_cells + 0x28, 0x1234);
    put(c, -0x7a54, flag_cells + 0x30);
    c.put32(flag_cells + 0x30, 0);
    for (const auto slot : {-0x39d4, -0x39d8, -0x39dc, -0x39e0, -0x39e4}) put(c, slot, 0);
    c.put8(c.r(2) - 0x39d0, 0);
    for (std::uint32_t i = 0; i < 4; ++i) {
        const auto club = clubs_at + 0x248 * i;
        c.put32(club + 0x2c, i % 3 == 0 ? 0 : clubs_at + 0x248 * ((i + 1) % 4));
        for (std::uint32_t slot = 0; slot < 50; ++slot) c.put32(club + 0x98 + 4 * slot, 0);
    }
    for (std::uint32_t i = 0; i < 5; ++i) c.put8(name_text + i, "Anna"[i]);
    // The file's name; the index words come from the image.
    c.set({{3, path_at}, {4, k.single}, {5, k.mode}});
}

Fixture make(const Case& k) {
    Fixture f;
    f.entry = 0x100a55a0;
    f.description = k.name;
    const bool swapped_layout = k.swapped && (k.mode & 0xff) == 0;
    Case k2 = k;
    if (k.swapped || k.second_name) k2.answers[(k.mode & 0xff) ? packed_open : plain_open] = {0, natural};
    const auto image = std::make_shared<std::vector<std::uint8_t>>(make_image(k, swapped_layout));
    f.setup = [k](Cpu& c) { build(c, k); };
    f.stub = [k2, image](State& s) { return scripted(s, k2, *image); };
    return f;
}

Case standard(std::string name) {
    Case k;
    k.name = std::move(name);
    constexpr std::array<std::uint32_t, 22> kinds{0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};
    constexpr std::array<std::uint32_t, 23> counts{4, 3, 2, 2, 1, 2, 0x110, 2, 0, 0x30, 2, 3, 2, 0x20, 0x20, 0x20, 2, 2, 2, 2, 2, 2, 0x20};
    for (unsigned i = 0; i < 22; ++i) k.entries[i] = {kinds[i], counts[kinds[i]], 0x100 * (i + 1), 0};
    return k;
}

// `n` natural answers, then `value`.
std::vector<std::uint32_t> after(unsigned n, std::uint32_t value) {
    std::vector<std::uint32_t> v(n, natural);
    v.push_back(value);
    v.push_back(natural);
    return v;
}

std::vector<Fixture> load_fixtures() {
    std::vector<Fixture> out;
    const auto add = [&](Case k, bool overflow = false) {
        auto f = make(k);
        f.summary_overflow = overflow;
        out.push_back(std::move(f));
    };
    const auto name_of = [](const Case& k, const std::string& what) {
        return std::string(k.single ? "single block" : "separate") + " mode " + std::to_string(k.mode) +
               (k.swapped ? " swapped" : "") + (k.extended ? " extended" : "") + ": " + what;
    };
    // The matrix: the two forms, the three file calls (plain, packed, a packed byte with the sign bit), the byte
    // order, the language block, the second header word.
    for (const std::uint32_t single : {0u, 1u})
        for (const std::uint32_t mode : {0u, 1u, 0xffu})
            for (const bool swapped : {false, true})
                for (const bool extended : {false, true})
                    for (const std::uint32_t word_b : {0x10u, 2u, 0x11u}) {
                        if (swapped && mode) continue;
                        if (single && !extended && (mode == 0xff || swapped || word_b != 0x10)) continue;  // 0x5208 default rows: keep few
                        if (word_b != 0x10 && (swapped || mode == 0xff)) continue;
                        auto k = standard("matrix");
                        k.single = single;
                        k.mode = mode;
                        k.swapped = swapped;
                        k.extended = extended;
                        k.map_count = extended ? (mode == 1 ? 3 : 0xffffffffu) : 0xffffffffu;
                        k.word_b = word_b;
                        k.name = name_of(k, "word b " + std::to_string(word_b));
                        add(k, word_b == 2);
                    }
    // The second name and the language file's presence, the folder walk, the packed path lookup.
    for (const std::uint32_t mode : {0u, 1u}) {
        auto k = standard("");
        k.mode = mode;
        k.second_name = true;
        k.name = name_of(k, "second name");
        add(k);
        k = standard("");
        k.mode = mode;
        k.answer(file_exists, {0});
        k.name = name_of(k, "language file missing");
        add(k);
        k = standard("");
        k.mode = mode;
        k.answer(open_path, {0});
        k.answer(find_next, {0, 0, 1});
        k.answer(find_step, {0, 1});
        k.name = name_of(k, "path lookup fails, folder walk");
        add(k);
        k = standard("");
        k.mode = mode;
        k.answer(plain_open, {0, 0});
        k.answer(packed_open, {0, 0});
        k.name = name_of(k, "neither name opens");
        add(k);
    }
    {
        auto k = standard("");
        k.second_name = true;
        k.answer(read_marker, {0});
        k.name = name_of(k, "marker 0");
        add(k);
        k.answer(read_marker, {signature ^ 0x80000000u});
        add(k);
    }
    // Entry shapes: kind 6 with the extended flag, unknown kinds, kind 8, repeated kinds, zero and big counts.
    for (const std::uint32_t single : {0u, 1u})
        for (const bool extended : {false, true}) {
            if (single && !extended) continue;
            auto k = standard("");
            k.single = single;
            k.extended = extended;
            k.entries[6].flag = 1;
            k.name = name_of(k, "kind 6 flag 1");
            add(k);
            k = standard("");
            k.single = single;
            k.extended = extended;
            k.entries[1].kind = 8;
            k.entries[2].kind = 0x17;
            k.entries[3].kind = 0x80000000u;
            k.entries[4].kind = 0xffffffffu;
            k.name = name_of(k, "kinds 8, 0x17, huge");
            add(k);
            k = standard("");
            k.single = single;
            k.extended = extended;
            for (auto& e : k.entries) e.count = e.kind == 6 || e.kind == 9 || e.kind == 22 || e.kind == 13 || e.kind == 14 || e.kind == 15 ? 0x10 : 0;
            k.name = name_of(k, "zero counts");
            add(k);
        }
    // Short or wrong read counts and allocation or open failures, one at a time.
    for (const std::uint32_t mode : {0u, 1u})
        for (const bool swapped : {false, true}) {
            if (swapped && mode) continue;
            for (const unsigned n : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 25u, 60u, 113u, 114u, 115u, 116u})
                for (const std::uint32_t wrong : {0u, 2u}) {
                    auto k = standard("");
                    k.mode = mode;
                    k.swapped = swapped;
                    k.extended = true;
                    k.map_count = 2;
                    k.answers[read_packed] = after(n, wrong);
                    k.answers[read_plain] = after(n, wrong);
                    k.name = name_of(k, "read " + std::to_string(n) + " answers " + std::to_string(wrong));
                    add(k);
                }
            // A table that fails to allocate is then dereferenced by the original too (the people, the club tables,
            // the sample draw, the index loops): those cases would crash both runs, so they are left out.
            constexpr std::array<unsigned, 6> separate_crashes{0, 4, 6, 14, 15, 19};
            constexpr std::array<unsigned, 8> single_crashes{2, 3, 4, 5, 7, 9, 10, 11};
            for (unsigned n = 0; n <= 24; ++n) {
                if (std::ranges::find(separate_crashes, n) == separate_crashes.end()) {
                    auto k = standard("");
                    k.mode = mode;
                    k.swapped = swapped;
                    k.answers[allocate] = after(n, 0);
                    k.name = name_of(k, "allocation " + std::to_string(n) + " fails");
                    add(k);
                }
                if (!swapped && std::ranges::find(single_crashes, n) == single_crashes.end()) {
                    auto k = standard("");
                    k.mode = mode;
                    k.single = 1;
                    k.extended = true;
                    k.entries[6].flag = 1;
                    k.answers[allocate] = after(n, 0);
                    k.name = name_of(k, "allocation " + std::to_string(n) + " fails");
                    add(k);
                }
                auto k = standard("");
                k.mode = mode;
                k.swapped = swapped;
                k.second_name = n == 0;
                k.answers[packed_open] = after(n, 0);
                k.answers[plain_open] = after(n, 0);
                k.name = name_of(k, "open " + std::to_string(n) + " fails");
                add(k);
            }
            for (const auto reader : readers) {
                auto k = standard("");
                k.mode = mode;
                k.swapped = swapped;
                k.answers[reader] = {0};
                k.name = name_of(k, "reader " + hex_address(reader) + " fails");
                add(k);
            }
        }
    // The inline sample over the 0x128-byte table: counts on both sides of the narrow limit (the last entry, so
    // that its allocation is the last).
    for (const std::uint32_t mode : {0u, 1u})
        for (const std::uint32_t count : {1u, 7u, 0xffffu, 0x10000u, 0x10001u, 0xfffeffffu, 0xffff0001u, 0xffff0000u,
                                          0x7fffffffu, 0x80000000u, 0xffffffffu, 0xfffffff0u}) {
            auto k = standard("");
            k.mode = mode;
            k.extended = true;
            k.entries[4].count = count;
            std::swap(k.entries[4], k.entries[21]);
            k.name = name_of(k, "table 128 count " + hex_address(count));
            add(k, count == 0x10000);
        }
    // Repeated kinds: a second entry of the same kind with another count, so that adding differs from setting
    // (both passes). The duplicate takes the last entry's place.
    for (const std::uint32_t kind : {2u, 3u, 4u, 5u, 7u, 9u, 10u, 11u, 13u, 14u, 15u, 17u, 18u, 21u, 22u})
        for (const std::uint32_t single : {0u, 1u}) {
            auto k = standard("");
            k.single = single;
            k.extended = single != 0;
            std::uint32_t first = 0;
            for (const auto& e : k.entries)
                if (e.kind == kind) first = e.count;
            k.entries[kind == 22 ? 2 : 21] = {kind, kind == 4 ? first : first + 3, 0x900, 0};
            k.name = name_of(k, "kind " + std::to_string(kind) + " twice");
            add(k);
        }
    // Kinds 17 and 18 (optional tables) that fail to open: released through the single block's call site or the
    // separate one; and released by the close itself, so that the table is null when the loader looks again.
    for (const std::uint32_t mode : {0u, 1u})
        for (const unsigned n : {17u, 18u}) {
            auto k = standard("");
            k.mode = mode;
            k.single = 1;
            k.extended = true;
            k.answers[packed_open] = after(n, 0);
            k.answers[plain_open] = after(n, 0);
            k.name = name_of(k, "optional kind open " + std::to_string(n) + " fails");
            add(k);
            k.single = 0;
            k.extended = false;
            k.zero_at = n;
            k.name = name_of(k, "optional kind open " + std::to_string(n) + " fails, closed table cleared");
            add(k);
        }
    // The closing re-read of a swapped file.
    {
        auto k = standard("");
        k.swapped = true;
        k.answers[reopen] = {0};
        k.name = name_of(k, "reopen fails");
        add(k);
        for (const unsigned n : {0u, 1u, 2u}) {
            k = standard("");
            k.swapped = true;
            k.answers[reread_plain] = after(n, 0);
            k.name = name_of(k, "reread " + std::to_string(n) + " fails");
            add(k);
        }
    }
    return out;
}
}
