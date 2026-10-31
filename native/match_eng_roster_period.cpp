// MATCH_ENG.CPP, roster: the statistics kept per period and the possession shares.
#include "match_eng_roster.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
std::uint32_t wrap(std::int32_t value) { return std::uint32_t(value); }
constexpr std::uint32_t vault = 0x10000;  // the original addresses these fields as 0x10000 - n

// The pool's magic double (2^52 + 2^31) and the int-to-double idiom around it:
// the integer's bits with the sign flipped under the exponent 0x43300000.
std::uint32_t magic_at(Cpu& c) { return c.toc(-0x52c4) + 0x60; }
double magic(Cpu& c) { const double value = c.f64(magic_at(c)); c.set_f(1, value); return value; }
double convert(Cpu& c, Fpu& fp, std::int32_t value, std::uint32_t lo_at, std::uint32_t hi_at, double minus) {
    c.put32(lo_at, wrap(value) ^ 0x80000000);
    c.put32(hi_at, 0x43300000);
    return fp.fsubs(c.f64(hi_at), minus);
}

// One side's players' contribution to the side's period totals, for the end of a period
// (the same bookkeeping three times over in the original, once per entry mode).
enum class Tail { Final, First, Second };
struct StatisticFields { std::uint32_t at; unsigned size; std::uint32_t player_byte; };
constexpr StatisticFields cleared[] = {
    {0xe042, 1, 0xf}, {0xe040, 1, 0xd}, {0xe041, 1, 0xe}, {0xe03e, 1, 0xa}, {0xe03f, 1, 0xb},
    {0xe062, 2, 1}, {0xe064, 2, 2}, {0xe066, 2, 4}, {0xe068, 2, 5}, {0xe06a, 2, 7}, {0xe06c, 2, 8}, {0xe06e, 2, 9}, {0xe070, 2, 0x12},
};

// The loops over both sides: zero the side's totals, then add up each of its 20 players, and
// leave the registers as the last player of the second side left them (they differ per variant).
void add_up_players(Cpu& c, std::uint32_t match, Tail variant) {
    std::uint32_t before_c = 0, raw_24 = 0;
    for (unsigned side = 0; side < 2; ++side) {
        const auto totals = match + vault + side * 0x36;
        for (const auto& field : cleared) {
            if (field.size == 1) c.put8(totals + field.at - vault, 0); else c.put16(totals + field.at - vault, 0);
        }
        c.put8(totals + 0xe04b - vault, 0);
        c.put8(totals + 0xe04c - vault, 0);
        for (unsigned index = 0; index < 20; ++index) {
            const auto player = match + side * side_size + index * player_size + players_at;
            // The rating, in tenths of a thousand, kept between 1 and 10.
            const auto rating = std::int32_t(c.i16(player + 0x36)) + 500;
            auto tenth = std::int32_t((std::int64_t(0x10624dd3) * rating) >> 32) >> 6;
            tenth += tenth < 0;
            c.put8(player + 0x1b, wrap(tenth));
            if (cmp_gt(c, s8(c.u8(player + 0x1b)), 10)) c.put8(player + 0x1b, 10);
            if (cmp_lt(c, s8(c.u8(player + 0x1b)), 1)) c.put8(player + 0x1b, 1);
            for (const auto& field : cleared) {
                const auto at = totals + field.at - vault;
                if (field.size == 1) c.put8(at, c.u8(at) + c.u8(player + field.player_byte));
                else c.put16(at, wrap(c.i16(at) + c.u8(player + field.player_byte)));
            }
            const auto at_b = totals + 0xe04b - vault, at_c = totals + 0xe04c - vault;
            c.put8(at_b, wrap(c.u8(at_b) + s8(c.u8(player + 0x23))));
            before_c = c.u8(at_c);
            raw_24 = c.u8(player + 0x24);
            c.put8(at_c, wrap(std::int32_t(before_c) + s8(raw_24)));
        }
    }
    keep(c, {{30, 2}, {29, 0x14}});
    const std::uint32_t sum = wrap(std::int32_t(before_c) + s8(raw_24)), offset = 0x36 + 0xe04c;
    switch (variant) {
    case Tail::Final:
        c.set({{3, sum}, {4, wrap(s8(raw_24))}, {5, raw_24}, {6, side_size}, {7, before_c}, {8, offset}});
        break;
    case Tail::First:  // mode 0
        c.set({{3, side_size}, {4, sum & 0xff}, {5, side_size + 19 * player_size}, {6, sum}, {7, wrap(s8(raw_24))}, {8, before_c}, {9, offset}});
        break;
    case Tail::Second:  // mode 1
        c.set({{3, raw_24}, {4, sum & 0xff}, {5, sum}, {6, wrap(s8(raw_24))}, {7, before_c}, {8, offset}, {9, side_size}});
        break;
    }
}
}  // namespace

// ---------------------------------------------------------------------------
// 0x101f2400
void period_statistics(Cpu& c, std::uint32_t match, std::uint32_t mode) {
    Frame frame(c, 0x101f2400, 0x60, {.first_gpr = 26});
    keep(c, {{31, match}});
    c.put16(frame.at(0x7e), mode);
    const auto clock = std::int32_t(c.i16(match + 0x794c));
    c.set(3, wrap(clock));
    std::uint32_t half;
    if (cmp_eq(c, clock, 0x1ef)) half = 0;
    else {
        c.set(5, wrap(std::int32_t(c.i16(match + 0x794c))));
        half = cmp_eq(c, clock, 0x3de) ? 1 : 2;
    }
    keep(c, {{28, half}});
    const auto entry = std::int32_t(c.i16(frame.at(0x7e)));
    c.set(4, wrap(entry));
    Fpu fp(c);
    const auto totals = match + vault;
    const auto share_stores = [&](std::uint32_t at, double value) { c.put_f32(totals + at - vault, float(value)); };
    // The two draws of 7 (+3), as bytes, then 5 + the side totals (at least 10).
    const auto draw = [&](std::uint32_t amount, std::uint32_t back) { c.set(3, amount); c.call(0x101d3c90, back); return c.r(3); };
    if (cmp_eq(c, entry, 1)) {
        // Mode 1.
        auto value = draw(7, 0x101f2ec8);
        c.set({{5, wrap(s8(value + 3))}, {6, totals}, {7, 0x43300000}});
        c.set(3, 0);
        auto minus = magic(c);
        share_stores(0xe0a8, convert(c, fp, s8(value + 3), frame.at(0x3c), frame.at(0x38), minus));
        value = draw(7, 0x101f2f00);
        c.set({{4, totals}, {5, 0x43300000}});
        minus = magic(c);
        share_stores(0xe0ac, convert(c, fp, s8(value + 3), frame.at(0x44), frame.at(0x40), minus));
        value = draw(5, 0x101f2f38);
        const auto goals_a = std::uint32_t(c.u8(totals + 0xe03d - vault)), goals_b = std::uint32_t(c.u8(totals + 0xe073 - vault));
        c.set({{8, totals}, {7, goals_a}, {5, totals}, {4, value + goals_b}});
        if (cmp_lt(c, std::int32_t(goals_a + value + goals_b), 10)) keep(c, {{26, 10}});
        else {
            value = draw(5, 0x101f2f68);
            keep(c, {{26, goals_a + (value + std::uint32_t(c.u8(totals + 0xe073 - vault)))}});
        }
        minus = magic(c);
        share_stores(0xe0b0, convert(c, fp, s8(c.r(26)), frame.at(0x3c), frame.at(0x38), minus));
        add_up_players(c, match, Tail::Second);
    } else if (!cmp_ge(c, entry, 1) && cmp_ge(c, entry, 0)) {
        // Mode 0.
        auto value = draw(7, 0x101f2928);
        c.set({{5, value + 3}, {4, wrap(s8(value + 3))}});
        auto minus = magic(c);
        share_stores(0xe0a8, convert(c, fp, s8(value + 3), frame.at(0x44), frame.at(0x40), minus));
        value = draw(7, 0x101f2960);
        c.set({{8, wrap(s8(value + 3))}, {4, magic_at(c) - 0x60}, {5, totals}});
        minus = magic(c);
        share_stores(0xe0ac, convert(c, fp, s8(value + 3), frame.at(0x3c), frame.at(0x38), minus));
        value = draw(5, 0x101f2998);
        const auto goals_a = std::uint32_t(c.u8(totals + 0xe03d - vault)), goals_b = std::uint32_t(c.u8(totals + 0xe073 - vault));
        c.set({{5, goals_a}, {4, goals_b}, {7, totals}, {9, value + goals_b}, {8, goals_a + (value + goals_b)}});
        if (cmp_lt(c, std::int32_t(goals_a + (value + goals_b)), 10)) keep(c, {{27, 10}});
        else {
            value = draw(5, 0x101f29c8);
            c.set({{6, totals}, {5, std::uint32_t(c.u8(totals + 0xe03d - vault))}, {4, std::uint32_t(c.u8(totals + 0xe073 - vault))}});
            keep(c, {{27, c.r(5) + (value + c.r(4))}});
        }
        minus = magic(c);
        share_stores(0xe0b0, convert(c, fp, s8(c.r(27)), frame.at(0x3c), frame.at(0x38), minus));
        add_up_players(c, match, Tail::First);
    } else if (cmp_ge(c, entry, 1) && cmp_eq(c, entry, 0x3de)) {
        add_up_players(c, match, Tail::Final);
    }
    leave(c, {{3, match}, {4, half}});
    enter(c, 0x101f3468, {match, half});
    share_percentages(c, match, half);
}

// ---------------------------------------------------------------------------
// 0x101f3480
void share_percentages(Cpu& c, std::uint32_t match, std::uint32_t first_kind) {
    const auto sp = c.sp();
    c.put32(sp - 4, c.r(31));
    c.put32(sp - 8, c.r(30));
    const auto singles = c.toc(-0x52c8);
    keep(c, {{31, singles}});
    c.put8(sp - 0x28, first_kind);
    c.put8(sp - 0x27, 3);
    c.put8(sp - 0x26, 4);
    c.set(5, 4);
    keep(c, {{30, 0}});
    Fpu fp(c);
    const auto hundred = [&] { return c.f32(singles + 0x20); };
    struct Slots { std::int32_t first_lo, first_hi, second_lo, second_hi, store, load; };
    // A share: 100 * numerator / denominator as a byte, and the byte it leaves to 100.
    const auto share = [&](std::uint32_t numerator_base, std::uint32_t denominator_base_a, std::uint32_t denominator_base_b,
                           std::uint32_t kind, const Slots& slots, std::uint32_t first_out, std::uint32_t second_out) {
        const auto at = [&](std::uint32_t base) { return c.i32(match + base + wrap(s8(kind)) * 4); };
        const auto numerator = at(numerator_base);
        const auto denominator = std::int32_t(wrap(at(denominator_base_a)) + wrap(at(denominator_base_b)));
        const auto out = [&](std::uint32_t where) { return match + vault + wrap(s8(kind)) - where; };
        if (cmp_gt(c, denominator, 0)) {
            const auto f3 = hundred();
            c.set(5, c.toc(-0x52c4));
            auto minus = magic(c);
            const auto f2 = convert(c, fp, numerator, sp + slots.first_lo, sp + slots.first_hi, minus);
            c.set(5, c.toc(-0x52c4));
            minus = magic(c);
            auto f0 = convert(c, fp, denominator, sp + slots.second_lo, sp + slots.second_hi, minus);
            f0 = fp.fdivs(f2, f0);
            f0 = fp.fmuls(f3, f0);
            c.put64(sp + slots.store, fp.fctiwz(0, f0));
            c.put8(out(first_out), c.u32(sp + slots.load) & 0xff);
            const auto first = c.u8(out(first_out));
            c.put8(out(second_out), wrap(std::int8_t(100 - first)));
        } else {
            c.put8(out(first_out), 0x32);
            c.put8(out(second_out), 0x32);
        }
        return denominator;
    };
    std::int32_t last_total = 0;
    std::uint32_t kind = 0;
    for (unsigned index = 0; cmp_lt(c, std::int32_t(index), 3); ++index) {
        kind = c.u8(sp - 0x28 + index);
        c.set(4, kind);
        c.set(30, index);
        // Shares of the first three pairs of counters.
        share(0x79cc, 0x79cc, 0x7990, kind, {-0x10, -0x14, -0x18, -0x1c, -0x24, -0x20}, 0x1fa3, 0x1f77);
        share(0x79a4, 0x79a4, 0x79b8, kind, {-0x20, -0x24, -0x18, -0x1c, -0x14, -0x10}, 0x1fa8, 0x1f72);
        share(0x79e0, 0x79e0, 0x797c, kind, {-0x20, -0x24, -0x18, -0x1c, -0x14, -0x10}, 0x1f6d, 0x1fad);
        // The fourth: three of the six counters over all six.
        const auto at = [&](std::uint32_t base) { return c.i32(match + base + wrap(s8(kind)) * 4); };
        last_total = std::int32_t(wrap(at(0x797c)) + wrap(at(0x79a4)) + wrap(at(0x79e0)) + wrap(at(0x7990)) + wrap(at(0x79b8)) + wrap(at(0x79cc)));
        const auto out = [&](std::uint32_t where) { return match + vault + wrap(s8(kind)) - where; };
        if (cmp_gt(c, last_total, 0)) {
            const auto f3 = hundred();
            c.set(5, c.toc(-0x52c4));
            auto minus = magic(c);
            const auto numerator = std::int32_t(wrap(at(0x797c)) + wrap(at(0x79cc)) + wrap(at(0x79a4)));
            const auto f2 = convert(c, fp, numerator, sp - 0x20, sp - 0x24, minus);
            c.set(5, c.toc(-0x52c4));
            minus = magic(c);
            auto f0 = convert(c, fp, last_total, sp - 0x18, sp - 0x1c, minus);
            f0 = fp.fdivs(f2, f0);
            f0 = fp.fmuls(f3, f0);
            c.put64(sp - 0x14, fp.fctiwz(0, f0));
            c.put8(out(0x1fb2), c.u32(sp - 0x10) & 0xff);
            c.put8(out(0x1f7c), wrap(std::int8_t(100 - c.u8(out(0x1fb2)))));
        } else {
            c.put8(out(0x1fb2), 0x32);
            c.put8(out(0x1f7c), 0x32);
        }
    }
    // The registers as the last pass leaves them.
    const auto at = [&](std::uint32_t base) { return c.u32(match + base + wrap(s8(kind)) * 4); };
    const auto positive = last_total > 0;
    c.set({{5, wrap(s8(kind)) + 0xe084}, {6, positive ? at(0x79cc) : 0x32}, {7, positive ? at(0x797c) : at(0x79a4)},
           {8, at(0x79e0)}, {9, positive ? at(0x79b8) : at(0x7990)}, {4, kind}});
    c.set(30, c.u32(sp - 8));
    c.set(31, c.u32(sp - 4));
}

// ---------------------------------------------------------------------------
// 0x101f4900
void count_event(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t flag) {
    c.put32(c.sp() - 4, c.r(31));  // red zone: the caller's r31
    const auto clock = std::int32_t(c.i16(match + 0x794c));
    std::uint32_t period;
    if (cmp_eq(c, clock, 0x1ef)) period = 0;
    else {
        c.set(6, wrap(clock));
        period = cmp_eq(c, clock, 0x3de) ? 1 : 2;
    }
    keep(c, {{31, period}});
    // Counters; each answers the value it held (the original leaves it in r6 or r7).
    const auto bump16 = [&](std::uint32_t at) {
        const auto old = c.i16(match + at);
        c.put16(match + at, wrap(old + 1));
        return wrap(old);
    };
    const auto bump32 = [&](std::uint32_t at) {
        const auto old = c.u32(match + at);
        c.put32(match + at, old + 1);
        return old;
    };
    const auto offset16 = 0x22e + period * 2;
    bump16(offset16);
    c.set({{6, bump16(0x234)}, {7, wrap(s8(flag))}});
    const auto column = s8(c.u8(match + 0x791e));
    // The ball's zone along the pitch: up to column 3, up to column 7, the rest.
    // For each: the side's word counter (by period) and its two totals, the half-word
    // counters, and what each of the six ring entries gets.
    struct Zone {
        std::uint32_t word[2], totals[2][2], half, half_totals[2];
        std::uint32_t ring_side[2], ring_here[3];
    };
    static constexpr Zone zones[3] = {
        {{0x79cc, 0x7990}, {{0x79d8, 0x79dc}, {0x799c, 0x79a0}}, 0x210, {0x216, 0x218}, {0x78a3, 0x77fe}, {1, 0, 0}},
        {{0x79a4, 0x79b8}, {{0x79b0, 0x79b4}, {0x79c4, 0x79c8}}, 0x21a, {0x220, 0x222}, {0x7835, 0x786c}, {0, 1, 0}},
        {{0x797c, 0x79e0}, {{0x7988, 0x798c}, {0x79ec, 0x79f0}}, 0x224, {0x22a, 0x22c}, {0x77c7, 0x78da}, {0, 0, 1}},
    };
    if (cmp_ne(c, s8(flag))) {
        const auto ring_at = [&](std::uint32_t base) { return match + base + wrap(s8(c.u8(match + 0x7911))); };
        // The ring entry that falls out of the totals.
        const auto drop16 = [&](std::uint32_t base, std::uint32_t total) {
            c.put16(match + total, wrap(c.i16(match + total) - s8(c.u8(ring_at(base)))));
        };
        const auto drop32 = [&](std::uint32_t base, std::uint32_t total) {
            c.put32(match + total, c.u32(match + total) - wrap(s8(c.u8(ring_at(base)))));
        };
        drop16(0xa, 0x218);
        drop16(0x41, 0x222);
        drop16(0x78, 0x22c);
        drop32(0x77c7, 0x798c);
        drop32(0x77fe, 0x79a0);
        drop32(0x7835, 0x79b4);
        drop32(0x786c, 0x79c8);
        drop32(0x78a3, 0x79dc);
        drop32(0x78da, 0x79f0);
        for (const auto base : {0x77c7u, 0x77feu, 0x7835u, 0x786cu, 0x78a3u, 0x78dau}) c.put8(ring_at(base), 0);
        const unsigned zone = cmp_le(c, column, 3) ? 0 : cmp_lt(c, column, 8) ? 1 : 2;
        const auto& z = zones[zone];
        const unsigned side_index = cmp_eq(c, s8(side)) ? 0 : cmp_eq(c, s8(side), 1) ? 1 : 2;
        if (side_index < 2) {
            bump32(z.word[side_index] + period * 4);
            bump32(z.totals[side_index][0]);
            bump32(z.totals[side_index][1]);
            c.put8(ring_at(z.ring_side[side_index]), 1);
        }
        bump16(z.half + period * 2);
        bump16(z.half_totals[0]);
        bump16(z.half_totals[1]);
        c.put8(ring_at(0xa), z.ring_here[0]);
        c.put8(ring_at(0x41), z.ring_here[1]);
        c.put8(ring_at(0x78), z.ring_here[2]);
        c.set(7, z.ring_here[2]);  // the last `li r7` before the cursor moves on
        const auto slot = c.u8(match + 0x7911);
        c.set(6, slot);
        c.put8(match + 0x7911, slot + 1u);
        if (!cmp_lt(c, s8(c.u8(match + 0x7911)), 0x37)) { c.set(6, 0); c.put8(match + 0x7911, 0); }
    } else if (!cmp_eq(c, s8(side), -1)) {
        const unsigned zone = cmp_le(c, column, 3) ? 0 : cmp_lt(c, column, 8) ? 1 : 2;
        const auto& z = zones[zone];
        const unsigned side_index = cmp_eq(c, s8(side)) ? 0 : cmp_eq(c, s8(side), 1) ? 1 : 2;
        if (side_index < 2) {
            bump32(z.word[side_index] + period * 4);
            bump32(z.totals[side_index][0]);
        }
        const auto half = z.half + period * 2;
        // The registers left: r7 the counter's value (middle zone) or its offset, r6 the total's value.
        if (zone == 1) c.set(7, bump16(half));
        else { bump16(half); c.set(7, half); }
        c.set(6, bump16(z.half_totals[0]));
    }
    c.set(31, c.u32(c.sp() - 4));
}
}
