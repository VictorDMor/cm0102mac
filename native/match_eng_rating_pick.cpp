// MATCH_ENG.CPP, rating block, part two: fixture tests, the opposing player to meet, and scans over a side's players.
#include "match_eng_rating_pick.hpp"
#include <array>
#include <optional>

namespace cm::aot::match_eng {
using namespace guest;

namespace {
// The player's flag word (0x10289560, a halfword), `bl` returning to `back`.
std::int32_t player_flags(Cpu& c, std::uint32_t player, std::uint32_t back) {
    c.set(3, player);
    c.call(0x10289560, back);
    return std::int16_t(c.r(3));
}
bool flag_set(Cpu& c, std::uint32_t player, std::uint32_t back, std::uint32_t mask) {
    return nonzero(c, player_flags(c, player, back) & mask);
}
// The fixture totals of 0x101f21d0 into the five bytes at sp+0x38..0x3c; its result.
std::int32_t totals(Cpu& c, std::uint32_t frame_base, std::uint32_t match, std::uint32_t argument, std::uint32_t back) {
    c.set({{3, match}, {4, argument}, {5, frame_base + 0x3c}, {6, frame_base + 0x3b}, {7, frame_base + 0x3a},
           {8, frame_base + 0x39}, {9, frame_base + 0x38}, {10, 0}});
    call(c, 0x101f21d0, back);
    return s8(c.r(3));
}
// One of the two switches over the halfwords 0x10116780 leaves: the value of the compare chain's label, or nothing.
// `low` (at sp+0x3a, bits 2..0x40) and `high` (at sp+0x38, bits 0x80..0x800) map to the tables given.
std::optional<std::uint32_t> low_code(Cpu& c, std::int32_t v, const std::array<std::uint32_t, 6>& map) {
    auto order = compare(c, v, 0x10);
    if (order == 0) return map[3];
    if (order > 0) {
        order = compare(c, v, 0x40);
        if (order == 0) return map[5];
        if (order > 0) return std::nullopt;
        return compare(c, v, 0x20) == 0 ? std::optional(map[4]) : std::nullopt;
    }
    order = compare(c, v, 4);
    if (order == 0) return map[1];
    if (order > 0) return compare(c, v, 8) == 0 ? std::optional(map[2]) : std::nullopt;
    return compare(c, v, 2) == 0 ? std::optional(map[0]) : std::nullopt;
}
std::optional<std::uint32_t> high_code(Cpu& c, std::int32_t v, const std::array<std::uint32_t, 5>& map) {
    auto order = compare(c, v, 0x200);
    if (order == 0) return map[2];
    if (order > 0) {
        order = compare(c, v, 0x800);
        if (order == 0) return map[4];
        if (order > 0) return std::nullopt;
        return compare(c, v, 0x400) == 0 ? std::optional(map[3]) : std::nullopt;
    }
    order = compare(c, v, 0x100);
    if (order == 0) return map[1];
    if (order > 0) return std::nullopt;
    return compare(c, v, 0x80) == 0 ? std::optional(map[0]) : std::nullopt;
}
}  // namespace

// ---------------------------------------------------------------------------
// 0x101fce60
std::uint32_t kind_82_free(Cpu& c, std::uint32_t match, std::uint32_t argument) {
    Frame frame(c, 0x101fce60, 0x50, {.first_gpr = 31});
    Kept<31> self(c, match);
    c.put8(frame.at(0x6f), argument);
    if (compare(c, c.i16(c.u32(match + fixture_at) + 0x32)) != 0) return result(c, 0);
    c.set(3, c.u32(c.u32(match + fixture_at)));
    c.call(0x104646d0, 0x101fce9c);
    if (compare(c, s8(c.r(3))) != 0) return result(c, 0);
    if (compare(c, c.i16(c.u32(match + fixture_at) + 0x30), 0x82) != 0) return result(c, 0);
    if (compare(c, totals(c, frame.base(), match, c.u8(frame.at(0x6f)), 0x101fcee4)) <= 0) return result(c, 0);
    if (compare(c, s8(c.u8(frame.at(0x38)))) != 0) return result(c, 0);
    return result(c, 1);
}

// ---------------------------------------------------------------------------
// 0x101fcf20
std::uint32_t kind_96_free(Cpu& c, std::uint32_t match, std::uint32_t argument) {
    Frame frame(c, 0x101fcf20, 0x50, {.first_gpr = 30});
    Kept<31> self(c, match);
    Kept<30> key(c, argument);
    c.set(3, c.u32(c.u32(match + fixture_at)));
    c.call(0x104646d0, 0x101fcf48);
    if (compare(c, s8(c.r(3))) != 0) {
        c.set({{3, match}, {4, key}});
        call(c, 0x101d3c50, 0x101fcf60);
        c.set(4, c.r(3));
        const auto table = c.u32(c.toc(-0x7b5c));
        const auto fixture = c.u32(match + fixture_at);
        c.set(3, c.u32(table + (c.u32(fixture) << 2)));
        c.set(5, fixture);
        c.call(0x104a26b0, 0x101fcf84);
        return c.r(3);
    }
    if (compare(c, c.i16(c.u32(match + fixture_at) + 0x30), 0x96) != 0) return result(c, 0);
    if (compare(c, totals(c, frame.base(), match, key, 0x101fcfbc)) <= 0) return result(c, 0);
    if (compare(c, s8(c.u8(frame.at(0x38)))) != 0) return result(c, 0);
    return result(c, 1);
}

// ---------------------------------------------------------------------------
// 0x101fd000
std::uint32_t side_flagged(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101fd000, 0x40, {.first_gpr = 30});
    Kept<31> self(c, match);
    Kept<30> key(c, side);
    if (compare(c, s8(c.u8(c.u32(match + fixture_at) + 0x3a)), 1) == 0) return result(c, 0);
    if (compare(c, s8(c.u8(c.u32(match + fixture_at) + 0x3a)), 9) == 0) return result(c, 0);
    c.set({{3, match}, {4, key}});
    call(c, 0x101d3c50, 0x101fd058);
    c.set({{4, 0}, {5, c.u8(c.u32(match + fixture_at) + 0x40)}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, 0x101fd070);
    if (compare(c, s8(c.r(3))) != 0) {
        c.set(3, match + u(s8(key) * team_tactics_size) + team_tactics);
        call(c, 0x101d56f0, 0x101fd090);
        return result(c, nonzero(c, std::int32_t(c.r(3) & 0x4000)) ? 1 : 0);
    }
    c.set(3, u(s8(key)));
    if (compare(c, s8(c.u8(match + u(s8(key)) + 0x1c4)), 3) == 0) return result(c, 1);
    c.set(3, u(s8(key)));
    if (compare(c, s8(c.u8(match + u(s8(key)) + 0x1c4)), 4) == 0) return result(c, 1);
    return result(c, 0);
}

// ---------------------------------------------------------------------------
// 0x101fd100
std::uint32_t pick_opponent(Cpu& c, std::uint32_t match_in, std::uint32_t side_in, std::uint32_t index) {
    Frame frame(c, 0x101fd100, 0x80, {.first_gpr = 20, .first_fpr = 30});
    c.put8(frame.at(0xa3), index);
    Kept<23> match(c, match_in);
    Kept<22> side(c, side_in);
    Kept<30> own(c, match_in + u(s8(side_in) * std::int32_t(side_size)) + u(s8(index) * std::int32_t(player_size)) + players_at);
    Fpu fp(c);
    const auto fail = [&] { return result(c, 0xffffffff); };
    if (compare(c, s8(position(c, own))) < 0 || flag_set(c, own, 0x101fd160, 1)) return fail();
    // The other side's players (a side other than 0 is met by side 0, and side 0 by side 1).
    const std::uint32_t others = match_in + (s8(side_in) == 0 ? side_size : 0) + players_at;
    Kept<31> other(c, others);
    Kept<21> previous_a(c, c.r(21));
    Kept<29> n(c, 0);
    const auto own_flag = [&](std::uint32_t back, std::uint32_t mask) { return flag_set(c, own, back, mask); };
    const auto other_flag = [&](std::uint32_t back, std::uint32_t mask) { return flag_set(c, other, back, mask); };
    // Whether the own player's flags and the other's pair up, bit against bit; each test re-reads both (calls in order).
    const auto pairs = [&] {
        if (compare(c, s8(position(c, own))) < 0) return false;
        const bool first = own_flag(0x101fd1b4, 2) || own_flag(0x101fd1cc, 4);
        if (first && !other_flag(0x101fd1e4, 0x40)) return false;
        if (own_flag(0x101fd1fc, 8) && !other_flag(0x101fd214, 0x20)) return false;
        if (own_flag(0x101fd22c, 0x10) && !other_flag(0x101fd244, 0x10)) return false;
        if (own_flag(0x101fd25c, 0x20) && !other_flag(0x101fd274, 8)) return false;
        if (own_flag(0x101fd28c, 0x40) && !other_flag(0x101fd2a4, 4) && !other_flag(0x101fd2bc, 2)) return false;
        if (own_flag(0x101fd2d4, 0x80) && other_flag(0x101fd2ec, 0x800)) return true;
        if (own_flag(0x101fd30c, 0x100) && other_flag(0x101fd324, 0x400)) return true;
        if (own_flag(0x101fd344, 0x200) && other_flag(0x101fd35c, 0x200)) return true;
        if (own_flag(0x101fd37c, 0x400) && other_flag(0x101fd394, 0x100)) return true;
        if (own_flag(0x101fd3b4, 0x800) && other_flag(0x101fd3cc, 0x80)) return true;
        return false;
    };
    for (; compare(c, s8(n), 0x14) < 0; n = n + 1, previous_a = other, other = other + player_size) {
        if (pairs()) return result(c, n);
    }
    player_flags(c, own, 0x101fd404);
    c.set({{4, frame.at(0x3a)}, {5, frame.at(0x38)}});
    c.call(0x10116780, 0x101fd410);
    Kept<28> column_a(c, c.r(28));
    Kept<26> column_b(c, c.r(26));
    if (const auto code = low_code(c, c.i16(frame.at(0x3a)), {0, 1, 2, 3, 4, 5})) column_a = *code;
    if (const auto code = high_code(c, c.i16(frame.at(0x38)), {1, 3, 4, 5, 7})) column_b = *code;
    Kept<24> best(c, 0xffffffff);
    c.set_f(30, double(c.f32(c.toc(-0x52c8) + 0xdc)));
    other = others;
    n = 0;
    Kept<20> previous_b(c, c.r(20));
    Kept<27> row_a(c, c.r(27));
    Kept<25> row_b(c, c.r(25));
    for (; compare(c, s8(n), 0x14) < 0; n = n + 1, previous_b = other, other = other + player_size) {
        if (compare(c, s8(position(c, other))) < 0) continue;
        if (flag_set(c, other, 0x101fd534, 1)) continue;
        player_flags(c, other, 0x101fd54c);
        c.set({{4, frame.at(0x3a)}, {5, frame.at(0x38)}});
        c.call(0x10116780, 0x101fd558);
        // The mirrored tables of the second pass.
        if (const auto code = low_code(c, c.i16(frame.at(0x3a)), {6, 5, 4, 3, 2, 1})) row_a = *code;
        if (const auto code = high_code(c, c.i16(frame.at(0x38)), {7, 5, 4, 3, 1})) row_b = *code;
        c.set({{3, match}, {4, column_b}, {5, column_a}, {6, row_b}, {7, row_a}});
        call(c, 0x101ef8d0, 0x101fd64c);
        const double score = c.f(1);
        c.set_f(31, score);
        bool better = compare(c, s8(best), -1) == 0;
        if (!better) {
            fp.fcmp(0, score, c.f(30));
            better = (c.cr(0) & 8) != 0;
        }
        if (better) {
            best = n;
            c.set_f(30, score);
        }
    }
    return result(c, best);
}
}
