// DATABASE.CPP 0x100ce320 (database_records.hpp): the club a record's code
// belongs to. The original is 2,182 instructions: two stages of tests of the
// record's first word against 82 words each, then a mapping of the word to an
// index into the club table, all straight chains of TOC loads and compares.
// The chains are tables here (extracted from the instructions); the registers
// the original leaves after its last executed step are set at the end.
#include "database_records.hpp"
#include "database_records_ce320.hpp"
#include "database_records_detail.hpp"
#include "guest_values.hpp"
#include <array>

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Frame;
using namespace records;
using namespace records::club_code;

// The pointers the original keeps in registers across the routine (loaded at
// entry from consecutive TOC words; see toc_of_register).
using Pointers = std::array<std::uint32_t, 32>;

struct Walk {
    std::uint32_t r0, r3, r4, r5, r13;
};

// The record's code word is read once per stage (the original reloads it at
// every step, but nothing stores in between).
//
// Whether the word is the `code` word (first stage) or the `index` word
// (second stage) of one of the pairs. Each step loads the word and the
// member and compares them, signed: pairs below `loaded_pairs` through their
// TOC slot, the others through the register that holds them. Leaves r5 (the
// member) and r13 (the word) as the last step did.
bool is_member(Cpu& c, bool second_stage, std::uint32_t record, const Pointers& pointers, Walk& w) {
    const auto word = c.u32(record);
    for (unsigned i = 0; i < pair_count; ++i) {
        const auto slot = second_stage ? pairs[i].index : pairs[i].code;
        w.r13 = word;
        w.r5 = c.u32(second_stage && i >= loaded_pairs ? pointers[register_of_pair(i)] : c.toc(slot));
        if (word == w.r5) return true;
    }
    return false;
}

// The club table index the word maps to, in r0 (-1 when no pair matches):
// each step compares the word with a pair's member and, when equal, loads
// the other word of the pair. The registers follow the original's step
// shapes: first stage, TOC loads for the first 58 pairs, then pair 58 through
// r3 as it stands and the rest through their registers; second stage, TOC
// loads, then registers.
void map_word(Cpu& c, bool second_stage, std::uint32_t record, const Pointers& pointers, Walk& w) {
    const auto word = c.u32(record);
    w.r0 = 0xffffffff;
    for (unsigned i = 0; i < pair_count; ++i) {
        const auto& pair = pairs[i];
        const bool kept = i >= loaded_pairs;
        if (!second_stage) {
            if (i <= loaded_pairs) {
                w.r4 = c.u32(c.toc(pair.code));
                w.r5 = word;
                if (w.r5 != w.r4) continue;
                if (i < loaded_pairs) w.r3 = c.toc(pair.index);  // pair 58 answers through r3 as it stands
                w.r0 = c.u32(w.r3);
                return;
            }
            w.r3 = c.u32(c.toc(pair.code));
            w.r4 = word;
            if (w.r4 != w.r3) continue;
            w.r0 = c.u32(pointers[register_of_pair(i)]);
            return;
        }
        if (!kept) {
            w.r4 = c.u32(c.toc(pair.index));
            w.r5 = word;
            if (w.r5 != w.r4) continue;
            w.r3 = c.toc(pair.code);
            w.r0 = c.u32(w.r3);
            return;
        }
        w.r4 = word;
        w.r3 = c.u32(i == loaded_pairs ? w.r3 : pointers[register_of_pair(i)]);
        if (w.r4 != w.r3) continue;
        w.r3 = c.toc(pair.code);
        w.r0 = c.u32(w.r3);
        return;
    }
}
}

// 0x100ce320: the club a record's code names, or 0 (and, for a missing club,
// the record's byte at +0x244 cleared). `found` (when not null) receives 1 for
// a code of the first set, 0 for the second. `require_flag` (a signed byte)
// asks for the byte at +0x244 to be set. The record's owner (+0x2c) must have
// one of three words (TOC -0x7a18, -0x7af4, -0x79fc) at +0x68 (inferred: the
// game's own club types). Quirk: the first stage fails only on index -1, the
// second on any negative index (signed), as the original compares.
std::uint32_t club_of_code(Cpu& c, std::uint32_t record, std::uint32_t found, std::int8_t require_flag) {
    c.set(6, record);
    c.set(0, c.lr());
    Frame frame(c, 0x100ce320, 0xb0, {.first_gpr = 13});
    Pointers pointers{};
    for (unsigned r = 15; r <= 31; ++r) pointers[r] = c.toc(toc_of_register(r));
    for (const unsigned r : {12u, 11u, 10u, 9u, 8u, 7u, 3u}) pointers[r] = c.toc(toc_of_register(r));
    for (unsigned r = 15; r <= 31; ++r) c.set(r, pointers[r]);
    for (const unsigned r : {12u, 11u, 10u, 9u, 8u, 7u, 3u}) c.set(r, pointers[r]);
    c.set(0, 0xffffffff);
    c.set({{13, c.toc(slot::message)}, {14, c.toc(slot::text_pool)}});
    // `or. r6,r3,r3` ran first, on the argument.
    c.compare(0, std::int32_t(record), 0);
    if (record == 0) {
        fail_assertion(c, frame, 0x100ce3b8, 0x430a, 0x40, 0x38, 0, true);
        return 0;
    }
    const auto no = [&] {
        c.set(3, 0);
        return std::uint32_t(0);
    };
    const auto owner = c.u32(record + 0x2c);
    c.set(13, owner);
    if (compare_logical(c, owner) == 0) return no();
    const auto flag = guest::extended(require_flag);
    c.set(5, flag);
    if (compare(c, std::int32_t(flag)) != 0) {
        const auto set = guest::extended(c.i8(record + 0x244));
        c.set(5, set);
        if (compare(c, std::int32_t(set)) == 0) return no();
    }
    // The owner's +0x68 against three words.
    bool accepted = false;
    for (const auto at : {-0x7a18, -0x7af4, -0x79fc}) {
        const auto owner_again = c.u32(record + 0x2c);
        c.set(5, owner_again);
        const auto word_address = c.toc(at);
        c.set(13, word_address);
        const auto kind = c.u32(owner_again + 0x68);
        c.set(5, kind);
        const auto expected = c.u32(word_address);
        c.set(13, expected);
        if (compare(c, std::int32_t(kind), std::int32_t(expected)) == 0) {
            accepted = true;
            break;
        }
    }
    if (!accepted) return no();
    Walk w{0xffffffff, c.r(3), found, c.r(5), c.r(13)};
    // The end: cr0 from the index's compare, then the club or the cleared byte.
    const auto finish_club = [&](bool fail_on_negative) {
        compare(c, std::int32_t(w.r0), -1);
        c.set({{4, w.r4}, {5, w.r5}, {3, w.r3}, {0, w.r0}});
        const bool failed = fail_on_negative ? std::int32_t(w.r0) <= -1 : w.r0 == 0xffffffff;
        if (failed) {
            c.set({{0, 0}, {3, 0}});
            c.put8(record + 0x244, 0);
            return std::uint32_t(0);
        }
        const auto table = c.toc(-0x7be8);
        c.set(3, table);
        const auto offset = w.r0 * club_stride;
        c.set(0, offset);
        const auto club = c.u32(table) + offset;
        c.set(3, club);
        return club;
    };
    const auto store_found = [&](std::uint32_t value) {
        if (compare_logical(c, w.r4) != 0) {
            w.r5 = value;
            c.put8(w.r4, value);
        }
    };
    if (is_member(c, false, record, pointers, w)) {
        store_found(1);
        map_word(c, false, record, pointers, w);
        return finish_club(false);
    }
    if (is_member(c, true, record, pointers, w)) {
        store_found(0);
        map_word(c, true, record, pointers, w);
        return finish_club(true);
    }
    compare(c, std::int32_t(w.r13), std::int32_t(w.r5));
    c.set(5, w.r5);
    return no();
}
}
