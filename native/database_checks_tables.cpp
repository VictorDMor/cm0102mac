// cm-database-checks fixtures for DATABASE.CPP's tables block (database_checks.hpp). The allocator, the assert
// macro's calls, the exit call and the club-slot recorder are scripted: each call is recorded with its registers
// (record_call) and answered from the case; the routines' own code and the tables it walks are real.
#include "database_checks.hpp"
#include "guest_abi.hpp"
#include <algorithm>
#include <array>

namespace cm::aot::database_checks {
namespace {
using namespace cm::aot::guest;

struct Mix {
    std::uint32_t state;
    std::uint32_t next() { state = state * 1664525u + 1013904223u; return state >> 8; }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
};

constexpr std::uint32_t assert_calls[] = {0x10739890, 0x10768810, 0x1077b720, 0x10769b60};
constexpr std::uint32_t allocate_cleared = 0x10775200, release = 0x10775170, quit = 0x1076f7d0,
                        record_new_index = 0x10159050, new_array = 0x107735f0, delete_array = 0x10773610,
                        allocate = 0x10775130, reallocate = 0x107751b0, memcpy_code = 0x10fe0000;

// The world of the club-selection routine 0x100a34e0.
namespace keep {
constexpr std::uint32_t clubs = data + 0x1000, nations = data + 0x7000, staff = data + 0x7800,
                        links = data + 0x8000, link_ids = data + 0x8200, pairs = data + 0x8400,
                        pair_records = data + 0x8600, marks_arena = data + 0x9000, groups_arena = data + 0x9400,
                        out_map = data + 0xa000, cells = data + 0xb000, table3 = data + 0xb400, map_arena = data + 0xb800,
                        index_arena = data + 0xbc00, other_table = data + 0xc000;
constexpr std::uint32_t club_stride = 0x248, nation_stride = 0x130;

struct Case {
    std::uint32_t seed = 0;
    unsigned clubs = 8, nation_count = 3;
    std::uint32_t mode = 0;
    bool first_cell_zero = false, null_out = false;
    std::uint32_t marks_answer = marks_arena, groups_answer = groups_arena;
    unsigned staff_rows = 3, link_rows = 3, pair_rows = 3;
    std::uint32_t slot_answers = 0;  // bit n: the n-th call of 0x10159050 answers non-zero
    // The compaction routine 0x100a2da0.
    std::uint32_t entry = 0x100a34e0;
    std::uint32_t array_answer = map_arena, index_answer = index_arena, realloc_answer = clubs;
    unsigned rows3 = 3, removed = 2;
    // The release call takes r3 only: record that alone and clobber every volatile register, cr0/cr1/cr5-7 and
    // XER[CA] after it, as a real callee leaves them, so that the leftovers a caller never reads do not show.
    bool trim_release = false;
    bool ignore_r0 = false;  // r0 is scratch at a call: record it as 0 (0x100a3db0 leaves another one at its last call)
};

void put_cell(Cpu& c, std::int32_t slot, std::uint32_t value_address, std::uint32_t value) {
    c.put32(c.r(2) + std::uint32_t(slot), value_address);
    c.put32(value_address, value);
}

void setup(Cpu& c, const Case& t) {
    Mix mix{t.seed * 2654435761u + 977u};
    unsigned cell = 0;
    const auto next_cell = [&] { return cells + 4 * cell++; };
    put_cell(c, -0x7b98, next_cell(), t.first_cell_zero ? 0 : 1);
    // TOC cells that hold nation numbers.
    for (const std::int32_t slot : {-0x7514, -0x7a1c, -0x79fc, -0x7a24, -0x7a28, -0x7510, -0x750c, -0x7a10, -0x7a14,
                                    -0x7af4, -0x7a18, -0x7a00, -0x7504, -0x7b68, -0x7508})
        put_cell(c, slot, next_cell(), mix.below(5) < 3 ? 100 + mix.below(2) : mix.below(t.nation_count + 1));
    put_cell(c, -0x7b04, next_cell(), t.nation_count);
    put_cell(c, -0x7b50, next_cell(), t.clubs);
    put_cell(c, -0x7be8, next_cell(), clubs);
    put_cell(c, -0x7bac, next_cell(), 1);
    put_cell(c, -0x7ba8, next_cell(), 0);
    put_cell(c, -0x7be4, next_cell(), t.staff_rows);
    put_cell(c, -0x7be0, next_cell(), staff);
    put_cell(c, -0x7a38, next_cell(), t.link_rows);
    put_cell(c, -0x7a34, next_cell(), links);
    put_cell(c, -0x74c8, next_cell(), t.pair_rows);
    put_cell(c, -0x74c4, next_cell(), pairs);
    // Nations: number at +0x68 (0..nation_count-1, sometimes shared with the cells), flag byte at +0x120.
    for (unsigned n = 0; n < 5; ++n) {
        const auto record = nations + n * nation_stride;
        c.put32(record + 0x68, n % std::max(1u, t.nation_count));
        c.put8(record + 0x120, mix.below(3) == 0 ? (mix.below(2) ? 1 : 0x80) : 0);
    }
    // With no group (nation count 0) the allocation clears nothing, yet clubs still use group 0: keep it clean.
    c.s.memory.clear(groups_arena, 0x400);
    if (t.clubs == 0) c.s.memory.clear(clubs, club_stride);  // rows that point at a club in a table of none read this record
    for (unsigned i = 0; i < t.clubs; ++i) {
        const auto record = clubs + i * club_stride;
        c.put32(record, i);
        const auto kind = mix.below(10);
        c.put32(record + 0x2c, kind == 0 ? 0 : nations + mix.below(std::min(5u, std::max(1u, t.nation_count))) * nation_stride);
        for (const std::uint32_t field : {0x74u, 0x78u, 0x7cu})  // pointers to other clubs, or none
            c.put32(record + field, mix.below(3) ? clubs + mix.below(t.clubs) * club_stride : 0);
        c.put16(record + 0x1ec, mix.below(2) == 0 ? mix.below(4) * 250 : (mix.next() & 0xffff));
    }
    // Rows pointing at clubs (and some at nothing, or at ids out of range).
    const auto club_at = [&] { return clubs + mix.below(std::max(1u, t.clubs)) * club_stride; };
    for (unsigned i = 0; i < t.staff_rows; ++i) {
        c.put32(staff + i * 0x70 + 0x20, mix.below(3) ? club_at() : 0);
        c.put32(staff + i * 0x70 + 0x24, mix.below(3) ? club_at() : 0);
    }
    for (unsigned i = 0; i < t.link_rows; ++i)
        for (unsigned k = 0; k < 3; ++k) {
            const auto id_word = link_ids + (i * 3 + k) * 4;
            const auto kind = mix.below(5);
            c.put32(id_word, kind == 0 ? 0xffffffffu : kind == 1 ? t.clubs + mix.below(3) : mix.below(std::max(1u, t.clubs)));
            c.put32(links + i * 0x1c + 8 + 4 * k, mix.below(4) ? id_word : 0);
        }
    for (unsigned i = 0; i < t.link_rows; ++i) c.put32(links + i * 0x1c + 0x14, mix.below(2) ? link_ids + i * 12 : 0);
    for (unsigned i = 0; i < t.pair_rows; ++i) {
        const auto holder = pair_records + i * 0x20;
        c.put32(holder + 0xc, mix.below(3) ? club_at() : 0);
        c.put32(pairs + i * 0x14 + 4, mix.below(5) ? holder : 0);
        c.put32(pairs + i * 0x14 + 8, mix.below(5) ? club_at() : 0);
    }
    // Flag bytes of the clubs' own records are the nations' (+0x120 of the club's linked record): give some
    // of the holders' clubs a flagged record.
    // The compaction routine's cells and the sixth-column table of 0x34-byte records.
    put_cell(c, -0x74f8, next_cell(), 0xa5a5a5a5);
    put_cell(c, -0x74c0, next_cell(), t.rows3);
    put_cell(c, -0x74bc, next_cell(), table3);
    put_cell(c, -0x7500, next_cell(), t.removed);
    put_cell(c, -0x74fc, next_cell(), 0);
    put_cell(c, -0x7bb8, next_cell(), 0);
    c.put32(c.r(2) - 0x7e64, data + 0xc00);
    c.put32(data + 0xc00, memcpy_code);
    c.put32(data + 0xc04, c.r(2) + 0x1000);
    for (unsigned i = 0; i < t.rows3; ++i)
        for (unsigned f = 4; f <= 0x18; f += 4) c.put32(table3 + i * 0x34 + f, mix.below(3) ? club_at() : 0);
    if (t.entry == 0x100a34e0) {
        c.set(3, t.null_out ? 0 : out_map);
        c.set(4, t.mode);
    } else {
        c.set(3, t.mode);
    }
}

// The owners of the scripted callees decline, so that every call (also those the generated pages make) goes
// through the scripted answers. Set at every probe: in-process runs start from clones that may not carry the flags.
void decline_owners(State& s) {
    s.allocator_original = s.stream_buffer_original = s.stream_io_original = s.file_path_original = true;
    s.cpp_object_storage_original = s.file_controller_original = s.stdlib_core_original = true;
    s.formatted_output_original = s.string_original = s.native_service_original = true;
}

bool stub(State& s, const Case& t) {
    decline_owners(s);
    const bool assertion = std::ranges::find(assert_calls, s.pc) != std::end(assert_calls);
    const bool library = s.pc == new_array || s.pc == delete_array || s.pc == allocate || s.pc == reallocate ||
                         s.pc == memcpy_code;
    if (!assertion && !library && s.pc != allocate_cleared && s.pc != release && s.pc != quit &&
        s.pc != record_new_index)
        return false;
    Cpu c(s);
    const auto calls = c.u32(trace_at);
    const auto scratch = c.r(0);
    if (s.pc == release && t.trim_release) {
        // target, LR, r3, r13-r31 and CR
        auto at = trace_at + 4 + calls * 4;
        const auto add = [&](std::uint32_t value) { c.put32(at, value), at += 4; };
        add(s.pc);
        add(c.lr());
        add(c.r(3));
        for (unsigned r = 13; r < 32; ++r) add(c.r(r));
        for (unsigned f = 0; f < 8; ++f) add(c.cr(f));
        c.put32(trace_at, (at - trace_at - 4) / 4);
        c.set(3, 0);
        for (unsigned r : {0u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u}) c.set(r, 0xdead0000u + r);
        for (unsigned f : {0u, 1u, 5u, 6u, 7u}) c.set_cr(f, 0x4 | (f & 1) << 3);
        c.s.wr<1>(guest::reg::xer_ca, 1);
        s.pc = c.lr();
        return true;
    }
    if (t.ignore_r0) c.set(0, 0);
    record_call(c);
    c.set(0, scratch);
    std::uint32_t answer = 0;
    if (assertion) {
        answer = s.pc == 0x10768810 ? data + 0x7c00 : data + 0xa000;
    } else if (s.pc == allocate_cleared) {
        const bool marks = c.r(4) == 1;
        answer = marks ? t.marks_answer : t.groups_answer;
        if (answer) s.memory.clear(answer, std::uint64_t(c.r(3)) * c.r(4));  // the cleared block
    } else if (s.pc == new_array) {
        answer = t.array_answer;
    } else if (s.pc == allocate) {
        answer = t.index_answer;
    } else if (s.pc == reallocate) {
        answer = t.realloc_answer;
    } else if (s.pc == memcpy_code) {
        answer = c.r(3);  // copies (source r3, destination r4, size r5), like the library routine
        for (std::uint32_t i = 0; i < c.r(5); ++i) c.put8(c.r(4) + i, c.u8(c.r(3) + i));
    } else if (s.pc == record_new_index) {
        answer = (t.slot_answers >> (calls % 32)) & 1 ? (calls & 2 ? 1 : 0x101) : (calls & 2 ? 0 : 0x100);
        c.set(4, 0xc0de0000 + calls);  // the callee's leftover, which the original keeps in r4
    }
    c.set(3, answer);
    s.pc = c.lr();
    return true;
}

std::vector<Fixture> fixtures() {
    std::vector<Fixture> found;
    const auto add = [&](const Case& t, const std::string& text) {
        Fixture f;
        f.entry = t.entry;
        f.description = text;
        f.setup = [t](Cpu& c) { setup(c, t); };
        f.stub = [t](State& s) { return stub(s, t); };
        f.summary_overflow = t.seed % 5 == 0;
        found.push_back(std::move(f));
    };
    Case t;
    t.first_cell_zero = true;
    add(t, "first cell zero");
    t = {};
    t.null_out = true;
    add(t, "null out map");
    t = {};
    t.marks_answer = 0;
    t.clubs = t.nation_count = t.staff_rows = t.link_rows = t.pair_rows = 0;  // nothing may touch the null block
    add(t, "mark allocation fails");
    t = {};
    t.groups_answer = 0;
    t.clubs = 0;
    t.nation_count = t.staff_rows = t.link_rows = t.pair_rows = 0;
    add(t, "group allocation fails");
    Mix mix{12345};
    for (unsigned n = 1; n <= 700; ++n) {
        t = {};
        t.seed = n;
        t.clubs = std::array{0u, 1u, 3u, 8u, 16u, 24u, 40u, 40u}[mix.below(8)];
        t.nation_count = mix.below(5);  // 0: no group, so the grouping pass's last registers reach the release
        t.mode = std::array{0u, 1u, 2u, 3u, 0x101u, 0x182u, 0xffu, 0x80u, 1u, 2u}[mix.below(10)];
        t.staff_rows = mix.below(4);
        t.link_rows = mix.below(4);
        t.pair_rows = mix.below(4);
        t.slot_answers = mix.next() & 0xffff;
        add(t, "seed " + std::to_string(n) + " clubs " + std::to_string(t.clubs) + " nations " +
                   std::to_string(t.nation_count) + " mode " + hex_address(t.mode));
    }
    // The compaction routine: the same worlds, then its own failure paths.
    t = {};
    t.entry = 0x100a2da0;
    t.array_answer = 0;
    add(t, "compact: no map");
    t = {};
    t.entry = 0x100a2da0;
    t.first_cell_zero = true;
    add(t, "compact: nothing kept");
    t = {};
    t.entry = 0x100a2da0;
    t.index_answer = 0;
    add(t, "compact: no index array");
    t = {};
    t.entry = 0x100a2da0;
    t.realloc_answer = other_table;
    add(t, "compact: table moves");
    Mix again{777};
    for (unsigned n = 1; n <= 500; ++n) {
        t = {};
        t.entry = 0x100a2da0;
        t.seed = 1000 + n;
        t.clubs = std::array{0u, 1u, 3u, 8u, 16u, 24u, 40u, 40u}[again.below(8)];
        t.nation_count = again.below(5);
        t.mode = std::array{0u, 1u, 2u, 3u, 0x101u, 0x182u, 0xffu, 0x80u, 1u, 2u}[again.below(10)];
        t.staff_rows = again.below(4);
        t.link_rows = again.below(4);
        t.pair_rows = again.below(4);
        t.rows3 = again.below(4);
        t.removed = again.below(4);
        t.slot_answers = again.next() & 0xffff;
        if (again.below(12) == 0) t.realloc_answer = other_table;
        add(t, "compact seed " + std::to_string(n) + " clubs " + std::to_string(t.clubs) + " nations " +
                   std::to_string(t.nation_count) + " mode " + hex_address(t.mode));
    }
    return found;
}
}  // namespace keep

// 0x100ab5d0: the loaded records and the two tables it fills.
namespace staff {
constexpr std::uint32_t records = data + 0xc000, staff_arena = data + 0xc800, second_arena = data + 0xd000,
                        cells = data + 0xe000;
struct Case {
    std::uint32_t seed = 0;
    unsigned count = 2, extra = 1;
    bool no_records = false, no_staff = false, second_exists = false, alloc_fails = false;
};
void setup(Cpu& c, const Case& t) {
    Mix mix{t.seed * 2654435761u + 31u};
    unsigned cell = 0;
    const auto put = [&](std::int32_t slot, std::uint32_t value) {
        c.put32(c.r(2) + std::uint32_t(slot), cells + 4 * cell);
        c.put32(cells + 4 * cell, value);
        ++cell;
    };
    put(-0x7bac, 1);
    put(-0x7ba8, 0);
    put(-0x7be0, t.no_staff ? 0 : staff_arena);
    put(-0x74c0, 0xa5a5a5a5);
    put(-0x74bc, t.second_exists ? second_arena + 0x400 : 0);
    c.put32(c.r(2) - 0x39e0, t.no_records ? 0 : records);
    c.put32(c.r(2) - 0x39dc, t.count);
    c.put32(c.r(2) - 0x39d8, t.extra);
    for (std::uint32_t i = 0; i < 0x9d * 4; ++i) c.put8(records + i, mix.next() >> 3);
    for (std::uint32_t i = 0; i < 0x70 * 4; ++i) c.put8(staff_arena + i, 0x5a);
}
std::vector<Fixture> fixtures() {
    std::vector<Fixture> found;
    const auto add = [&](const Case& t, const std::string& text) {
        keep::Case stub_case;
        stub_case.index_answer = t.alloc_fails ? 0 : second_arena;
        Fixture f;
        f.entry = 0x100ab5d0;
        f.description = text;
        f.setup = [t](Cpu& c) { setup(c, t); };
        f.stub = [stub_case](State& s) { return keep::stub(s, stub_case); };
        f.summary_overflow = t.seed % 3 == 0;
        f.results = 0;
        found.push_back(std::move(f));
    };
    Case t;
    t.no_records = true;
    t.count = t.extra = 0;
    add(t, "no loaded records");
    t = {};
    t.no_staff = true;
    t.count = t.extra = 0;
    add(t, "no staff table");
    t = {};
    t.second_exists = true;
    add(t, "second table exists");
    t = {};
    t.alloc_fails = true;
    t.count = t.extra = 0;
    add(t, "allocation fails");
    for (unsigned count = 0; count <= 3; ++count)
        for (unsigned extra = 0; extra <= 3; ++extra)
            for (unsigned seed = 1; seed <= 6; ++seed) {
                t = {};
                t.seed = seed;
                t.count = count;
                t.extra = extra;
                add(t, "records " + std::to_string(count) + " extra " + std::to_string(extra) + " seed " + std::to_string(seed));
            }
    return found;
}
}  // namespace staff

// 0x100a1950: the staff table, the records its fields point at and the sample pool of the random tests.
namespace people {
constexpr std::uint32_t staff_table = data + 0x1000, linked = data + 0x2000, ratings_b = data + 0x3000,
                        ratings_a = data + 0x3400, clubs = data + 0x4000, club_linked = data + 0x5000,
                        link_b = data + 0x6000, words = data + 0x7000, strings = data + 0x7100, cells = data + 0x8000,
                        samples = data + 0x9000, out_map = data + 0xa000, marks_arena = data + 0xb000,
                        groups_arena = data + 0xb400;
constexpr std::uint32_t pool_texts[4] = {0x64, 0x6b, 0x72, 0x78};
struct Case {
    std::uint32_t seed = 0;
    unsigned staff = 6, groups = 2;
    std::uint32_t mode = 0;
    bool first_cell_zero = false, null_out = false;
    std::uint32_t marks_answer = marks_arena, groups_answer = groups_arena;
};
void setup(Cpu& c, const Case& t) {
    Mix mix{t.seed * 2654435761u + 4242u};
    unsigned cell = 0;
    const auto put = [&](std::int32_t slot, std::uint32_t value) {
        c.put32(c.r(2) + std::uint32_t(slot), cells + 4 * cell);
        c.put32(cells + 4 * cell, value);
        ++cell;
    };
    put(-0x7b98, t.first_cell_zero ? 0 : 1);
    put(-0x7bac, 1);
    put(-0x7ba8, 0);
    put(-0x7be0, staff_table);
    put(-0x7be4, t.staff);
    put(-0x7b04, t.groups);
    put(-0x7b50, 0);           // no clubs: 0x100a3db0 (run for real) has nothing to generate
    put(-0x7be8, clubs);
    put(-0x7a0c, 7);           // the shared id word
    for (const std::int32_t slot : {-0x74f4, -0x74f0, -0x74ec, -0x74d8, -0x74dc, -0x74e0, -0x74e4, -0x74e8})
        put(slot, mix.below(2) ? 100 + mix.below(2) : mix.below(std::max(1u, t.groups)));
    // The sample pool: a cursor into 16-bit samples that wraps after a few draws (a new salt from rng_core).
    for (unsigned i = 0; i < 0x100; ++i) c.put16(samples + 2 * i, mix.next());
    put(-0x7b90, mix.next() & 0xffff);                  // salt
    c.put32(c.r(2) - 0x7b8c, samples);                  // start (a word of the TOC, not a cell)
    put(-0x7b88, samples + 0x30 + 2 * mix.below(8));    // end
    put(-0x7b84, samples + 2 * mix.below(20));          // cursor
    c.put32(c.r(2) - 0x34c4, t.seed * 7919u + 13u);     // rng_core's state
    const std::array<std::int32_t, 20> halfwords{0, 0x95, 0x96, 0x97, 0x9a, 0x9b, 0x9c, 0x9f, 0xa0, 0xa1, 0x28,
                                                 0x27, 0x4f, 0x50, -1, -2, 0x1d4b, 0x1d4c, 0x7fff, -0x8000};
    const auto halfword = [&] { return std::uint32_t(mix.below(3) == 0 ? mix.below(300) : halfwords[mix.below(20)]); };
    const std::array<std::uint32_t, 6> classes{0xfffd, 0, 4, 5, 6, 10};
    const auto pick = [&](std::uint32_t base, std::uint32_t stride, unsigned n) {
        return mix.below(5) == 0 ? 0u : base + mix.below(n) * stride;
    };
    // Mostly clear flags, so that the staff get past the flag tests to the pool draws (the small modes' draws follow them).
    const std::array<std::uint8_t, 8> flags{0, 0, 0, 0, 0, 1, 0x80, 0x7f};
    const std::array<std::uint8_t, 8> bytes{0, 5, 6, 9, 10, 11, 0x80, 0xff};
    for (unsigned i = 0; i < 6; ++i) {
        const auto record = linked + i * 0x130;
        c.put32(record + 0x68, i % std::max(1u, t.groups));
        c.put8(record + 0x120, flags[mix.below(8)]);
        c.put8(record + 0x11f, bytes[mix.below(8)]);
        c.put16(record + 0xac, mix.below(3) == 0 ? classes[mix.below(6)] : mix.below(30));
    }
    for (unsigned i = 0; i < 4; ++i) {
        c.put8(link_b + i * 0x130 + 0x120, flags[mix.below(8)]);
        c.put8(club_linked + i * 0x130 + 0x120, flags[mix.below(8)]);
        c.put8(club_linked + i * 0x130 + 0x11f, bytes[mix.below(8)]);
    }
    for (unsigned i = 0; i < 8; ++i) {
        for (std::uint32_t at = 4; at <= 0xc; at += 2) c.put16(ratings_b + i * 0x20 + at, halfword());
        for (std::uint32_t at = 0x24; at <= 0x2c; at += 2) c.put16(ratings_a + i * 0x30 + at, halfword());
        c.put32(words + i * 4, mix.below(3) == 0 ? 7 : mix.below(12));
    }
    for (unsigned i = 0; i < 6; ++i) {
        const auto record = clubs + i * 0x200;
        c.put16(record + 0x1ec, halfword());
        c.put32(record + 0x2c, pick(club_linked, 0x130, 4));
        c.put32(record + 0x30, pick(words, 4, 8));
    }
    // The four pool texts the names are matched against, copied beside decoys.
    const auto pool = c.toc(-0x5618);
    for (unsigned i = 0; i < 4; ++i) {
        std::uint32_t at = 0;
        for (;; ++at) {
            const auto byte = c.u8(pool + pool_texts[i] + at);
            c.put8(strings + i * 0x20 + at, byte);
            if (!byte) break;
        }
        c.put8(strings + 0x80 + i * 0x20, 'z');
        c.put8(strings + 0x81 + i * 0x20, 0);
    }
    const auto name = [&] {
        const auto kind = mix.below(10);
        return kind == 0 ? 0u : kind < 6 ? strings + mix.below(4) * 0x20 : strings + 0x80 + mix.below(4) * 0x20;
    };
    for (unsigned i = 0; i < t.staff; ++i) {
        const auto record = staff_table + i * 0x70;
        c.put32(record + 0x0, i);
        c.put32(record + 0xc, pick(linked, 0x130, 6));
        c.put32(record + 0x10, pick(link_b, 0x130, 4));
        c.put32(record + 0x14, pick(ratings_b, 0x20, 8));
        c.put32(record + 0x1c, pick(ratings_a, 0x30, 8));
        c.put32(record + 0x20, mix.below(2) ? clubs : 0);
        c.put32(record + 0x24, pick(clubs, 0x200, 6));
        // The names: the case's pairs sit together (a pair matches only when both texts do).
        const auto pair = mix.below(4);
        const bool same = mix.below(8) == 0;
        c.put32(record + 0x28, same ? strings + (pair & 2) * 0x20 : name());
        c.put32(record + 0x2c, same ? strings + ((pair & 2) + 1) * 0x20 : name());
        c.put8(record + 0x5e, mix.below(4) == 0 ? mix.below(3) : 0);
        c.put8(record + 0x60, mix.below(3) == 0 ? (mix.below(2) ? 5 : 0xc) : mix.below(20));
        c.put8(record + 0x6b, mix.below(8));
    }
    c.set(3, t.null_out ? 0 : out_map);
    c.set(4, t.mode);
}
std::vector<Fixture> fixtures() {
    std::vector<Fixture> found;
    const auto add = [&](const Case& t, const std::string& text) {
        keep::Case stub_case;
        stub_case.marks_answer = t.marks_answer;
        stub_case.groups_answer = t.groups_answer;
        stub_case.ignore_r0 = true;
        stub_case.trim_release = true;
        Fixture f;
        f.entry = 0x100a1950;
        f.description = text;
        f.setup = [t](Cpu& c) { setup(c, t); };
        f.stub = [stub_case](State& s) { return keep::stub(s, stub_case); };
        f.summary_overflow = t.seed % 7 == 0;
        found.push_back(std::move(f));
    };
    Case t;
    t.first_cell_zero = true;
    add(t, "first cell zero");
    t = {};
    t.null_out = true;
    add(t, "null out map");
    t = {};
    t.marks_answer = 0;
    t.staff = 0;
    t.groups = 0;
    add(t, "mark allocation fails");
    t = {};
    t.groups_answer = 0;
    t.staff = 0;
    t.groups = 0;
    t.mode = 0;
    add(t, "group allocation fails");
    Mix mix{99};
    for (unsigned n = 1; n <= 900; ++n) {
        t = {};
        t.seed = n;
        // Ids below count - 16 reach the keeping tests (and their pool draws): most cases have more than 16 staff.
        t.staff = std::array{0u, 1u, 3u, 17u, 20u, 24u, 30u, 36u}[mix.below(8)];
        t.groups = 1 + mix.below(3);
        t.mode = std::array{0u, 1u, 2u, 1u, 2u, 4u, 0x101u, 0x182u, 3u, 0xffu, 2u, 1u}[mix.below(12)];
        add(t, "seed " + std::to_string(n) + " staff " + std::to_string(t.staff) + " groups " + std::to_string(t.groups) +
                   " mode " + hex_address(t.mode));
    }
    return found;
}
}  // namespace people

// 0x1009efc0: the staff table compaction. The world of 0x100a1950 (people::setup) with the tables this routine walks
// added; the staff count and the club count are changed by the stub at the calls (see Case) because the original's
// share of virtual rows needs a table of more than 1325 rows to stay small, which the fixture memory cannot hold.
namespace expand {
constexpr std::uint32_t nations = data + 0xc000, clubs = data + 0xc800, row48 = data + 0xe000, row44 = data + 0xf000,
                        row34 = data + 0xfc00, row14 = data + 0x10800, row3c = data + 0x10a00, map = data + 0x11000,
                        map_size = 0x1800, samples = data + 0x13000, cells = data + 0x14000, constants = data + 0x14800,
                        descriptor = data + 0x1e000;
constexpr std::uint32_t staff_table = people::staff_table, linked = people::linked;
constexpr unsigned pool_samples = 0x400;

struct Case {
    std::uint32_t seed = 0;
    unsigned staff = 12, groups = 2, clubs = 4;
    std::uint32_t mode = 0;
    bool first_cell_zero = false, array_fails = false, zero_nation_rows = false;
    // What the stub does to the staff count word after the last kept staff was copied (or at the end of 0x100a1950
    // when nothing is kept): 0 nothing, 1 a count of 1400 (no clamp), 2 a count that clamps the virtual count to
    // 3, 3 one that clamps it below zero (to 0), 4 one that clamps it to 1.
    int count_kind = 1;
    // Forced bounds of the three draws' wide arms: the 0x44-byte row count (r15), the staff draw's bound (through
    // the count word, for one virtual staff: count_kind 4) and the club count. Positive: 70000; negative: a bound
    // whose row address wraps to rows before the table. wide_first/wide_staff: sign only.
    std::int32_t wide_first = 0, wide_staff = 0;
    unsigned fail_realloc = 0;  // 1..4: that table's reallocation answers another address
    unsigned rows48 = 6, rows44 = 6, rows34 = 6, rows14 = 3, rows3c = 3;
    bool fixed = false;                      // a pool whose every sample plus salt is `sum` (no wrap)
    std::uint32_t sum = 0, salt = 0;
    std::int32_t wide_clubs = 0;            // the club count the stub sets after the 0x44-byte rows were copied (a bound)
    double share_small = 0.25, share_big = 0.2;
};

void setup(Cpu& c, const Case& t) {
    people::Case base;
    base.seed = t.seed;
    base.staff = t.staff;
    base.groups = t.groups;
    base.first_cell_zero = t.first_cell_zero;
    people::setup(c, base);
    Mix mix{t.seed * 2246822519u + 5u};
    unsigned cell = 0;
    const auto put = [&](std::int32_t slot, std::uint32_t value) {
        c.put32(c.r(2) + std::uint32_t(slot), cells + 4 * cell);
        c.put32(cells + 4 * cell, value);
        ++cell;
    };
    // The first cell of the 0x128-byte nation records becomes their base (still non-zero unless the case says so).
    c.put32(c.toc(-0x7b98), t.first_cell_zero ? 0 : nations);
    c.put32(c.toc(-0x7be8), clubs);
    for (unsigned n = 0; n < 4; ++n) {
        c.put32(nations + n * 0x128 + 0x6c, mix.next());
        c.put32(nations + n * 0x128 + 0x68, n);
    }
    // The constants: the real pool's words, with this case's two shares.
    const auto real = c.toc(-0x5614);
    for (std::uint32_t at = 0; at < 0x20; at += 8) c.put64(constants + at, c.u64(real + at));
    c.put_f64(constants, t.share_small);
    c.put_f64(constants + 8, t.share_big);
    c.put32(c.r(2) - 0x5614, constants);
    // The sample pool: random with wraps, or constant.
    const auto salt = t.fixed ? t.salt : (mix.next() & 0xffff);
    for (unsigned i = 0; i < pool_samples; ++i)
        c.put16(samples + 2 * i, t.fixed ? t.sum - salt : mix.next());
    c.put32(c.toc(-0x7b90), salt);
    c.put32(c.r(2) - 0x7b8c, samples);
    c.put32(c.toc(-0x7b88), t.fixed ? samples + 2 * (pool_samples - 1) : samples + 0x40 + 2 * mix.below(0x80));
    c.put32(c.toc(-0x7b84), samples + 2 * mix.below(8));
    // Staff rows: nation record, and a link to each of the three tables.
    const auto staff_pointer = [&](unsigned limit) { return staff_table + mix.below(std::max(1u, limit)) * 0x70; };
    const auto halfword = [&] {
        constexpr std::array<std::uint32_t, 8> values{0, 0x95, 0x96, 0x9f, 0x1d4b, 0x1d4c, 0xffff, 0x8000};
        return mix.below(3) == 0 ? mix.below(300) : values[mix.below(8)];
    };
    for (unsigned i = 0; i < t.staff; ++i) {
        const auto record = staff_table + i * 0x70;
        const bool no_nation = t.zero_nation_rows && i >= 8 && mix.below(4) == 0;
        c.put32(record + 0xc, no_nation ? 0 : linked + mix.below(6) * 0x130);
        c.put32(record + 0x14, t.rows48 && mix.below(5) ? row48 + mix.below(t.rows48) * 0x48 : 0);
        c.put32(record + 0x1c, t.rows44 && mix.below(5) ? row44 + mix.below(t.rows44) * 0x44 : 0);
        c.put32(record + 0x18, t.rows34 && mix.below(5) ? row34 + mix.below(t.rows34) * 0x34 : 0);
    }
    // The rows before the table are read when fewer than 16 staff are kept (the last 16 move up): give them a nation.
    for (unsigned i = 1; i <= 18; ++i) {
        c.s.memory.clear(staff_table - i * 0x70, 0x70);
        c.put32(staff_table - i * 0x70 + 0xc, linked + mix.below(6) * 0x130);
    }
    for (unsigned i = 0; i < t.rows48; ++i) {
        for (std::uint32_t at = 0; at < 0x48; at += 4) c.put32(row48 + i * 0x48 + at, mix.next());
        for (std::uint32_t at = 4; at <= 0xc; at += 2) c.put16(row48 + i * 0x48 + at, halfword());
    }
    for (unsigned i = 0; i < t.rows44; ++i) {
        for (std::uint32_t at = 0; at < 0x44; at += 4) c.put32(row44 + i * 0x44 + at, mix.next());
        for (std::uint32_t at = 0x24; at <= 0x2c; at += 2) c.put16(row44 + i * 0x44 + at, halfword());
    }
    for (unsigned i = 0; i < t.rows34; ++i) c.put32(row34 + i * 0x34, mix.next());
    // The staff pointers of the club table and of the 0x44-, 0x34- and 0x14-byte rows (a few null, a few to staff
    // past the table's real count would read garbage ids, so none).
    const auto maybe_staff = [&] { return mix.below(3) ? staff_pointer(t.staff) : 0u; };
    for (unsigned i = 0; i < t.clubs; ++i) {
        const auto record = clubs + i * 0x248;
        c.put32(record + 0x2c, linked + mix.below(6) * 0x130);
        c.put32(record + 8, mix.below(500));
        c.put16(record + 0x1ec, mix.below(3) == 0 ? mix.below(300) : mix.below(2000));
        for (std::uint32_t at = 0x5c; at < 0x248; at += 4) c.put32(record + at, t.staff ? maybe_staff() : 0);
        c.put32(record + 0x2c, linked + mix.below(6) * 0x130);
        c.put32(record + 0, i);
    }
    // 4 clubs only exist; a club table of several more is read as garbage by the draws, so the pad is clean.
    for (unsigned i = t.clubs; i < 8; ++i) c.s.memory.clear(clubs + i * 0x248, 0x248);
    for (unsigned i = 0; i < t.rows44; ++i)
        for (std::uint32_t at = 4; at <= 0x20; at += 4) c.put32(row44 + i * 0x44 + at, t.staff ? maybe_staff() : 0);
    for (unsigned i = 0; i < t.rows34; ++i)
        for (std::uint32_t at = 0x1c; at <= 0x30; at += 4) c.put32(row34 + i * 0x34 + at, t.staff ? maybe_staff() : 0);
    for (unsigned i = 0; i < t.rows14; ++i) c.put32(row14 + i * 0x14 + 4, t.staff ? maybe_staff() : 0);
    for (unsigned i = 0; i < t.rows3c; ++i)
        for (std::uint32_t at = 0x14; at <= 0x1c; at += 4) {
            const auto kind = mix.below(4);
            c.put32(row3c + i * 0x3c + at, kind == 0 ? 0xffffffffu : kind == 1 ? 0xfffffffbu
                                           : t.staff ? mix.below(t.staff) : 0xffffffffu);
        }
    // Cells of the tables.
    put(-0x74bc, row34);
    put(-0x74c0, t.rows34);
    put(-0x79f8, row44);
    put(-0x79f4, t.rows44);
    put(-0x7b38, row48);
    put(-0x74d4, t.rows48);
    put(-0x74c4, row14);
    put(-0x74c8, t.rows14);
    put(-0x74cc, row3c);
    put(-0x74d0, t.rows3c);
    put(-0x7bb8, 0);
    c.put32(c.r(2) - 0x7e64, descriptor);
    c.put32(descriptor, 0x10fe0000);
    c.put32(descriptor + 4, c.r(2) + 0x1000);
    c.s.memory.clear(map, map_size);
    c.set(3, t.mode);
}

// Answers the scripted callees. The map allocation answers the zeroed arena; the reallocations answer the same
// table (or, for the failing one, another address); the staff and club counts and some registers are changed
// where the case says. Calls made by this routine record only the registers their callee reads (plus the
// non-volatile ones and cr2-cr4) and clobber every volatile register, cr0/cr1/cr5-7 and XER[CA] as a real callee
// does, so that leftovers the original never reads do not show.
// The staff count word that gives `kept` staff (the loop ends, the virtual count clamps): v = count + 0x52d + kept.
std::uint32_t count_for_kind(int kind, std::uint32_t kept) {
    constexpr std::uint32_t capacity = 0x52d;
    switch (kind) {
    case 1: return 1400;                               // no clamp: v = kept * share
    case 2: return 3 - capacity - kept;                // clamped to v = 3
    case 3: return std::uint32_t(-5) - capacity - kept;  // v would be negative: 0
    default: return 1 - capacity - kept;               // clamped to v = 1
    }
}
constexpr std::uint32_t generation_random = 0x1077b770;
constexpr std::uint32_t routine_start = 0x1009efc0, routine_end = 0x100a0d78;
constexpr std::int32_t wide_positive = 70000;
// The negative bounds below make a draw's wide arm clamp to bound - 1: the row addresses wrap into the rows before
// each table (a 2^k offset cancels the row stride), where the fixture keeps valid records.
constexpr std::int32_t wide_negative_first = 1 - (1 << 30) - 3;   // r15: (bound - 1) * 0x44 wraps to -3 rows
constexpr std::int32_t wide_negative_staff = 1 - (1 << 28) - 2;   // r16: (bound - 1) * 0x70 wraps to -2 rows
constexpr std::int32_t wide_negative_club = 1 - (1 << 29) - 1;    // r16: (bound - 1) * 0x248 wraps to -1 row

void record_arguments(Cpu& c, unsigned arguments) {
    const auto size = c.u32(trace_at);
    auto at = trace_at + 4 + size * 4;
    const auto add = [&](std::uint32_t value) { c.put32(at, value), at += 4; };
    add(c.s.pc);
    add(c.lr());
    for (unsigned r = 3; r < 3 + arguments; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned f = 2; f <= 4; ++f) add(c.cr(f));
    c.put32(trace_at, (at - trace_at - 4) / 4);
}
void clobber_volatile(Cpu& c) {
    for (unsigned r : {0u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u}) c.set(r, 0xdead0000u + r);
    for (unsigned f : {0u, 1u, 5u, 6u, 7u}) c.set_cr(f, 0x4 | (f & 1) << 3);
    c.s.wr<1>(guest::reg::xer_ca, 1);
}

bool stub(State& s, const Case& t) {
    keep::decline_owners(s);
    Cpu c(s);
    // Inside the copy routine r2 is the callee's TOC; the glue saved the caller's at 0x14(r1).
    const auto caller_toc = s.pc == memcpy_code ? c.u32(c.sp() + 0x14) : c.r(2);
    const auto count_cell = c.u32(caller_toc - 0x7be4);
    const bool mine = c.lr() >= routine_start && c.lr() < routine_end;
    if (s.pc == allocate_cleared && t.clubs) c.put32(c.toc(-0x7b50), t.clubs);
    if (s.pc == release && c.r(3) == people::marks_arena && t.count_kind) {
        // The end of 0x100a1950 with nothing kept: the count word changes here, as no copy will follow.
        bool any = false;
        for (std::int32_t i = 0, n = c.i32(count_cell); i < n && !any; ++i) any = c.u32(map + 4 * std::uint32_t(i)) != 0;
        if (!any) c.put32(count_cell, count_for_kind(t.count_kind, 0));
    }
    if (s.pc == memcpy_code && c.lr() >= 0x1009fc30 && c.lr() < 0x1009fd20 && t.count_kind) {
        // Copying the last kept staff's rows: the count the loop and the virtual count read changes. r13 counts the
        // kept staff, one more once the staff row itself (the first copy) was made.
        bool later = false;
        for (std::int32_t i = std::int32_t(c.r(20)) + 1, n = c.i32(count_cell); i < n && !later; ++i) later = c.u32(map + 4 * std::uint32_t(i)) != 0;
        if (!later) c.put32(count_cell, count_for_kind(t.count_kind, c.r(13) + (c.lr() == 0x1009fc30 ? 1 : 0)));
    }
    if (s.pc == delete_array && c.lr() == 0x100a02f0 && t.wide_clubs) c.put32(c.toc(-0x7b50), std::uint32_t(t.wide_clubs));
    if (s.pc == generation_random && c.lr() == 0x100a0410 && t.wide_staff)  // the one virtual staff's draws follow
        c.put32(count_cell, std::uint32_t(t.wide_staff > 0 ? wide_positive : wide_negative_staff) + c.r(14) + 0x10);
    if (!mine) {
        keep::Case plain;
        plain.marks_answer = people::marks_arena;
        plain.groups_answer = people::groups_arena;
        plain.array_answer = t.array_fails ? 0 : map;
        plain.trim_release = true;
        plain.ignore_r0 = (c.lr() >= 0x100a1950 && c.lr() < 0x100a2d98) || (c.lr() >= 0x100a3db0 && c.lr() < 0x100a4bf8);
        return keep::stub(s, plain);
    }
    std::uint32_t answer = 0;
    unsigned arguments = 0;
    if (s.pc == reallocate) {
        const auto asked = c.r(3);
        arguments = 2;
        const auto bases = std::array<std::int32_t, 4>{-0x7be0, -0x7b38, -0x79f8, -0x74bc};
        unsigned index = 0;
        for (unsigned i = 0; i < 4; ++i)
            if (asked == c.u32(c.toc(bases[i])) && index == 0) index = i + 1;
        if (index == 4 && t.wide_first)  // the 0x44-byte rows kept: the first draw's bound
            c.set(15, std::uint32_t(t.wide_first > 0 ? wide_positive : wide_negative_first));
        answer = index && index == t.fail_realloc ? asked + 0x10 : asked;
    } else if (s.pc == new_array) {
        arguments = 1;
        answer = t.array_fails ? 0 : map;
    } else if (s.pc == delete_array || s.pc == quit) {
        arguments = 1;
    } else if (s.pc == memcpy_code) {
        arguments = 3;
        answer = c.r(3);  // copies (source r3, destination r4, size r5), like the library routine
        for (std::uint32_t i = 0; i < c.r(5); ++i) c.put8(c.r(4) + i, c.u8(c.r(3) + i));
    } else if (s.pc == 0x1077b720) {  // the formatter: the summaries take 4 and 3 registers, the assert macro 6
        arguments = c.lr() == 0x100a0d48 ? 4 : c.lr() == 0x100a0d60 ? 3 : 6;
        answer = data + 0xa000;
    } else if (s.pc == 0x10739890) {
        arguments = 5;
        answer = data + 0xa000;
    } else if (s.pc == 0x10768810) {
        answer = data + 0x7c00;
    } else if (s.pc == 0x10769b60) {
        arguments = 3;
        answer = data + 0xa000;
    } else {
        return false;  // the generator, the date routines: they run
    }
    record_arguments(c, arguments);
    clobber_volatile(c);
    c.set(3, answer);
    s.pc = c.lr();
    return true;
}

std::vector<Fixture> fixtures() {
    std::vector<Fixture> found;
    const auto add = [&](const Case& t, const std::string& text) {
        Fixture f;
        f.entry = 0x1009efc0;
        f.description = text;
        f.setup = [t](Cpu& c) { setup(c, t); };
        f.stub = [t](State& s) { return stub(s, t); };
        f.summary_overflow = t.seed % 7 == 0;
        f.results = 0;
        found.push_back(std::move(f));
    };
    Case t;
    t.array_fails = true;
    add(t, "no map");
    t = {};
    t.first_cell_zero = true;
    add(t, "nothing kept (0x100a1950 answers 0)");
    for (unsigned k = 1; k <= 4; ++k) {
        t = {};
        t.seed = 40 + k;
        t.staff = 14;
        t.fail_realloc = k;
        add(t, "realloc " + std::to_string(k) + " moves");
    }
    // The wide arms of the three draws, with and without the clamp to bound - 1 (a negative bound always clamps):
    // pool sums with a small remainder modulo 65535 keep the unclamped numbers inside the fixture's rows.
    constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 2> small_remainder{{{65540, 10}, {131070, 65535}}};
    for (unsigned n = 0; n < 24; ++n) {
        t = {};
        t.seed = 700 + n;
        t.staff = 14 + n % 11;
        t.mode = std::array{0u, 1u, 2u, 4u}[n % 4];
        t.fixed = true;
        t.sum = small_remainder[n % 2].first;
        t.salt = small_remainder[n % 2].second;
        const std::int32_t sign = (n / 2) % 2 ? -1 : 1;
        switch (n / 4 % 3) {
        case 0:
            t.wide_first = sign;
            break;
        case 1:
            t.wide_staff = sign;
            t.count_kind = 4;
            t.staff = std::max(t.staff, 16u);
            break;
        default:
            t.wide_clubs = sign > 0 ? wide_positive : wide_negative_club;
            t.clubs = 4;
            t.count_kind = 2;  // three virtual staff, whatever is kept
            break;
        }
        add(t, "wide draw seed " + std::to_string(t.seed) + " first " + std::to_string(t.wide_first) + " staff " +
                   std::to_string(t.wide_staff) + " clubs " + std::to_string(t.wide_clubs));
    }
    Mix mix{2026};
    for (unsigned n = 1; n <= 400; ++n) {
        t = {};
        t.seed = n;
        t.staff = std::array{0u, 1u, 3u, 8u, 14u, 20u, 24u, 24u}[mix.below(8)];
        t.groups = 1 + mix.below(3);
        t.clubs = std::array{0u, 1u, 3u, 4u, 4u}[mix.below(5)];
        t.mode = std::array{0u, 1u, 2u, 3u, 4u, 0x101u, 0x80u, 0xffu, 0u, 4u}[mix.below(10)];
        t.count_kind = 1 + mix.below(4);
        t.zero_nation_rows = t.staff >= 20 && mix.below(2);
        t.rows48 = mix.below(7);
        t.rows44 = mix.below(7);
        t.rows34 = mix.below(7);
        t.rows14 = mix.below(4);
        t.rows3c = mix.below(4);
        t.share_small = std::array{0.0, 0.1, 0.25, 0.3}[mix.below(4)];
        t.share_big = std::array{0.0, 0.1, 0.2, 0.3}[mix.below(4)];
        if (mix.below(3) == 0) {
            const auto sums = std::array<std::pair<std::uint32_t, std::uint32_t>, 7>{
                {{0, 0}, {50, 0}, {100, 0}, {7, 3}, {65540, 10}, {131070, 65535}, {1000, 0}}};
            const auto [sum, salt] = sums[mix.below(7)];
            t.fixed = true;
            t.sum = sum;
            t.salt = salt;
            if (sum == 65540 || sum == 131070) t.wide_clubs = wide_positive;
        }
        if (mix.below(15) == 0) t.fail_realloc = 1 + mix.below(4);
        add(t, "seed " + std::to_string(n) + " staff " + std::to_string(t.staff) + " mode " + hex_address(t.mode) +
                   " kind " + std::to_string(t.count_kind));
    }
    return found;
}
}  // namespace expand
}  // namespace

std::vector<Fixture> tables_fixtures() {
    auto found = keep::fixtures();
    for (auto& f : staff::fixtures()) found.push_back(std::move(f));
    for (auto& f : people::fixtures()) found.push_back(std::move(f));
    for (auto& f : expand::fixtures()) found.push_back(std::move(f));
    return found;
}
}
