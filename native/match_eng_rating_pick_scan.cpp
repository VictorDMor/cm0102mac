// MATCH_ENG.CPP, rating block, part two: tests and scans over a side's 20 players, and the record tables' release.
#include "match_eng_rating_pick.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
// The side's players the original walks, by the pointer to the first of them.
std::uint32_t side_players(std::uint32_t match, std::uint32_t side_word, bool opposite) {
    const auto side = s8(side_word);
    return match + u((opposite ? (side == 0 ? 1 : 0) : side) * std::int32_t(side_size)) + players_at;
}
}  // namespace

// ---------------------------------------------------------------------------
// 0x101fea10
std::uint32_t ball_cell_side(Cpu& c, std::uint32_t match_in) {
    Frame frame(c, 0x101fea10, 0x40, {.first_gpr = 31});
    Kept<31> match(c, match_in);
    c.set({{3, match}, {4, 0}, {5, c.u8(match + 0x791d)}, {6, c.u8(match + 0x791e)}});
    call(c, 0x101eea30, 0x101fea38);
    if (compare(c, s8(c.r(3))) != 0) return result(c, 1);
    c.set({{3, match}, {4, 1}, {5, c.u8(match + 0x791d)}, {6, c.u8(match + 0x791e)}});
    call(c, 0x101eea30, 0x101fea60);
    if (compare(c, s8(c.r(3))) != 0) return result(c, 0xffffffff);
    return result(c, 0);
}

// ---------------------------------------------------------------------------
// 0x101feae0
std::uint32_t totals_not_ahead(Cpu& c, std::uint32_t match, std::uint32_t argument) {
    Frame frame(c, 0x101feae0, 0x40, {});
    c.put32(frame.at(0x58), match);
    c.put8(frame.at(0x5f), argument);
    c.set({{3, c.u32(frame.at(0x58))}, {4, c.u8(frame.at(0x5f))}, {5, frame.at(0x3c)}, {6, frame.at(0x3b)},
           {7, frame.at(0x3a)}, {8, frame.at(0x39)}, {9, frame.at(0x38)}, {10, 0}});
    call(c, 0x101f21d0, 0x101feb18);
    const auto first = s8(c.u8(frame.at(0x3c)));
    c.set(3, u(first));
    if (compare(c, first, s8(c.u8(frame.at(0x3b)))) < 0) return result(c, 1);
    if (compare(c, s8(c.u8(frame.at(0x39)))) == 0) return result(c, 0);
    if (compare(c, s8(c.u8(frame.at(0x38)))) != 0) return result(c, 0);
    if (compare(c, s8(c.u8(frame.at(0x3a)))) != 0) return result(c, 0);
    const auto again = s8(c.u8(frame.at(0x3c)));
    c.set(3, u(again));
    if (compare(c, again, s8(c.u8(frame.at(0x3b)))) > 0) return result(c, 0);
    return result(c, 1);
}

// ---------------------------------------------------------------------------
// 0x101feba0
std::uint32_t count_matching_players(Cpu& c, std::uint32_t match, std::uint32_t key_in) {
    Frame frame(c, 0x101feba0, 0x50, {.first_gpr = 26});
    c.put32(frame.at(0x68), match);
    Kept<26> key(c, key_in);
    c.set(3, key);
    c.call(0x10283690, 0x101febc0);
    Kept<28> side(c, c.r(3));
    c.set(4, c.u32(frame.at(0x68)));
    Kept<31> player(c, side_players(match, side, true));
    Kept<30> count(c, 0);
    Kept<29> n(c, 0);
    Kept<27> previous(c, c.r(27));
    for (; compare(c, s8(n), 0x14) < 0; n = n + 1, previous = player, player = player + player_size) {
        if (compare(c, s8(position(c, player))) < 0) continue;
        c.set(3, player);
        c.call(0x10285ef0, 0x101fec08);
        if (compare_logical(c, c.r(3), key) == 0) count = count + 1;
    }
    return result(c, count);
}

// ---------------------------------------------------------------------------
// 0x101fec50
std::uint32_t first_matching_player(Cpu& c, std::uint32_t match, std::uint32_t key_in) {
    Frame frame(c, 0x101fec50, 0x50, {.first_gpr = 27});
    c.put32(frame.at(0x68), match);
    Kept<27> key(c, key_in);
    c.set(3, key);
    c.call(0x10283690, 0x101fec70);
    Kept<29> side(c, c.r(3));
    c.set(4, c.u32(frame.at(0x68)));
    Kept<31> player(c, side_players(match, side, true));
    Kept<30> n(c, 0);
    Kept<28> previous(c, c.r(28));
    for (; compare(c, s8(n), 0x14) < 0; n = n + 1, previous = player, player = player + player_size) {
        if (compare(c, s8(position(c, player))) < 0) continue;
        c.set(3, player);
        c.call(0x10285ef0, 0x101fecb4);
        if (compare_logical(c, c.r(3), key) == 0) return result(c, player);
    }
    return result(c, 0);
}

// ---------------------------------------------------------------------------
// 0x101fed00
std::uint32_t first_eligible_player(Cpu& c, std::uint32_t match_in, std::uint32_t key) {
    Frame frame(c, 0x101fed00, 0x60, {.first_gpr = 27});
    Kept<27> match(c, match_in);
    c.put32(frame.at(0x7c), key);
    c.set(3, c.u32(frame.at(0x7c)));
    c.call(0x10283690, 0x101fed20);
    Kept<29> side(c, c.r(3));
    Kept<31> player(c, side_players(match_in, side, false));
    Kept<30> n(c, 0);
    Kept<28> previous(c, c.r(28));
    for (; compare(c, s8(n), 0x14) < 0; n = n + 1, previous = player, player = player + player_size) {
        if (compare(c, s8(position(c, player))) < 0) continue;
        c.set(3, player);
        c.call(0x10272100, 0x101fed58);
        if (compare(c, s8(c.r(3))) == 0) continue;
        c.set(3, player);
        c.call(0x102720d0, 0x101fed6c);
        if (compare(c, s8(c.r(3))) != 0) continue;
        c.set({{3, match}, {4, player}, {5, frame.at(0x38)}, {6, 0}});
        call(c, 0x101d7dc0, 0x101fed8c);
        if (compare(c, s8(c.r(3))) != 0) continue;
        return result(c, player);
    }
    return result(c, 0);
}

// ---------------------------------------------------------------------------
// 0x101fedd0 and 0x101fee30
namespace {
void apply_record(Cpu& c, std::uint32_t entry, std::uint32_t target, std::uint32_t match_in, std::uint32_t second) {
    Frame frame(c, entry, 0x40, {.first_gpr = 31});
    Kept<31> match(c, match_in);
    c.put8(frame.at(0x5f), second);
    if (compare(c, s8(c.u8(frame.at(0x5f)))) != 0) {
        c.set(3, match + 0x10000 - 0x6ab4);
        c.call(target, entry + 0x34);
    } else {
        c.set(3, match + 0x7c6c);
        c.call(target, entry + 0x40);
    }
}
}  // namespace
void apply_record_a(Cpu& c, std::uint32_t match, std::uint32_t second) { apply_record(c, 0x101fedd0, 0x101182a0, match, second); }
void apply_record_b(Cpu& c, std::uint32_t match, std::uint32_t second) { apply_record(c, 0x101fee30, 0x10118a40, match, second); }

// ---------------------------------------------------------------------------
// 0x101fee90
void free_record_tables(Cpu& c) {
    Frame frame(c, 0x101fee90, 0x60, {.first_gpr = 24});
    Kept<30> sentinel(c, c.toc(-0x529c));
    Kept<25> n(c, 0);
    Kept<31> records(c, c.toc(-0x5298));
    Kept<28> offset(c, 0);
    Kept<26> record(c, c.r(26));
    Kept<27> slot(c, c.r(27));
    Kept<24> k(c, c.r(24));
    Kept<29> table_offset(c, c.r(29));
    do {
        record = records + offset;
        slot = record + 8;
        const auto table = c.u32(slot);
        if (compare_logical(c, table, sentinel) != 0 && compare_logical(c, table, 0) != 0) {
            k = 0;
            table_offset = 0;
            for (; compare(c, k, c.i16(record + 0xc)) < 0; k = k + 1, table_offset = table_offset + 0x10c) {
                c.set(3, c.u32(c.u32(slot) + table_offset + 0x104));
                if (compare_logical(c, c.r(3)) != 0) c.call(0x10775170, 0x101feef0);
                c.set(3, c.u32(c.u32(slot) + table_offset + 0x108));
                if (compare_logical(c, c.r(3)) != 0) c.call(0x10775170, 0x101fef08);
            }
            c.set(3, c.u32(slot));
            c.call(0x10775170, 0x101fef24);
            c.put32(slot, 0);
        }
        n = n + 1;
        offset = offset + 0x14;
    } while (compare(c, std::int32_t(n), 0x2a5) < 0);
    c.set(3, 0);
    c.put8(c.r(2) - 0x3903, 0);  // two bytes of the data area: cleared and -1
    c.put8(c.r(2) - 0x390c, 0xff);
}
}
