// MATCH_ENG.CPP, roster: choosing a side's player for an action, and rating adjustments.
#include "match_eng_roster.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
std::uint32_t wrap(std::int32_t value) { return std::uint32_t(value); }
constexpr std::uint32_t goalkeeper_first = 0xe01c, goalkeeper_second = 0xe020;
}  // namespace

// ---------------------------------------------------------------------------
// 0x101f3a30
std::uint32_t pick_player(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t kind, std::uint32_t excluded) {
    Frame frame(c, 0x101f3a30, 0x160, {.first_gpr = 15, .gpr_offset = -0x64, .first_fpr = 28});
    keep(c, {{29, match}, {21, side}, {20, kind}, {16, excluded}});
    keep(c, {{26, c.toc(-0x7a54)}, {27, c.toc(-0x7a6c)}, {22, c.toc(-0x7bac)}, {23, c.toc(-0x7ba8)}, {28, c.toc(-0x52cc)},
             {24, c.toc(-0x52c8)}, {31, c.toc(-0x52c4)}, {18, 0}});
    c.put16(frame.at(0x3a), 0xffff);  // the best score so far
    keep(c, {{19, 0}});
    Kept<25> score(c, c.r(25));
    Kept<15> flag_40(c, c.r(15));
    Kept<17> rank(c, c.r(17));
    Fpu fp(c);
    const auto pool = c.r(31), singles = c.r(24), table = c.r(27);
    const auto side8 = s8(side), kind8 = s8(kind);
    const auto fail = [&](std::uint32_t at, std::uint32_t line, std::int32_t name, std::int32_t extension, std::uint32_t back) {
        const auto messages = c.r(28);
        report_assertion(c, {.at = at, .file = messages + 0x45, .format = messages + 0x53, .title = messages + 0x60,
                             .name = frame.at(name), .extension = frame.at(extension), .line = line});
        c.set(3, wrap(-1));
        c.call(0x1076f7d0, back);
    };
    const auto magic_double = [&] { const double value = c.f64(pool + 0x60); c.set_f(1, value); return value; };
    // The integer at `value` as the original's double idiom, then x - magic.
    const auto convert = [&](std::int32_t value, std::int32_t lo, std::int32_t hi, double minus, bool single) {
        c.put32(frame.at(lo), wrap(value) ^ 0x80000000);
        c.put32(frame.at(hi), 0x43300000);
        const double f0 = c.f64(frame.at(hi));
        return single ? fp.fsubs(f0, minus) : fp.fsub(f0, minus);
    };
    const auto truncate = [&](double value, std::int32_t store, std::int32_t load) {
        c.put64(frame.at(store), fp.fctiwz(0, value));
        score = c.u32(frame.at(load));
    };
    // score = trunc(factor * (score as a halfword)); the stack slots differ from use to use.
    const auto scale = [&](std::uint32_t factor_at, std::int32_t lo, std::int32_t hi, std::int32_t store, std::int32_t load, bool single) {
        const double factor = c.f64(pool + factor_at);
        const double minus = magic_double();
        const auto f0 = convert(std::int16_t(score), lo, hi, minus, single);
        truncate(fp.fmul(factor, f0), store, load);
    };
    for (; cmp_lt(c, std::int32_t(c.r(19)), 0x14); keep(c, {{19, c.r(19) + 1}})) {
        const auto index = c.r(19);
        const auto player = match + wrap(side8 * std::int32_t(side_size)) + index * player_size + players_at;
        keep(c, {{30, player}});
        const auto position_now = position(c, player);
        c.put8(frame.at(0x38), position_now);
        if (cmp_eq(c, kind8, 5) && cmpl_ne(c, excluded)) {
            const auto at = wrap(side8 * 0x14) + index;
            c.set({{5, at}, {3, c.u8(excluded + at)}});
            if (cmp_ne(c, s8(c.r(3)))) continue;
        }
        if (cmp_lt(c, s8(c.u8(frame.at(0x38))))) continue;
        c.set(5, wrap(side8));
        const bool keeper = (cmp_eq(c, side8) && cmpl_eq(c, player, c.u32(match + goalkeeper_first))) ||
                            (cmp_eq(c, side8, 1) && cmpl_eq(c, player, c.u32(match + goalkeeper_second)));
        if (keeper) {
            if (cmp_eq(c, kind8, 2) || cmp_eq(c, kind8, 1) || cmp_eq(c, kind8, 3) || cmp_eq(c, kind8, 8)) continue;
            if (cmp_eq(c, kind8, 6) || cmp_eq(c, kind8, 7)) {
                c.set({{3, match}, {4, side}, {5, c.u8(match + ball_row_at)}, {6, c.u8(match + ball_column_at)}});
                call(c, 0x101eea30, 0x101f3b80);
                if (cmp_eq(c, s8(c.r(3)))) continue;
            }
        }
        if (cmp_gt(c, s8(c.u8(frame.at(0x38))), 10)) fail(0x101f3bb0, 0x3d23, 0xb9, 0x57, 0x101f3bf4);
        if (cmpl_eq(c, c.u32(player + 0x70))) fail(0x101f3c14, 0x3d26, 0x9a, 0x52, 0x101f3c58);
        const auto row = [&] { return std::uint32_t(c.u8(match + ball_row_at)); };
        const auto column = [&] { return std::uint32_t(c.u8(match + ball_column_at)); };
        // The player's team and fixture side asked of the other routines: is it this player's team's turn.
        const auto turn_matches = [&](std::uint32_t at, std::uint32_t round, std::uint32_t second) {
            c.set({{3, match}, {4, c.u8(player + 0x27)}});
            call(c, 0x101d3bf0, at + 4);
            keep(c, {{15, c.r(3)}});
            c.set({{3, match}, {4, c.u8(player + 0x27)}});
            call(c, 0x101d3c50, at + 0x14);
            c.set({{4, c.r(3)}, {3, c.u32(table)}, {5, round}, {6, second}, {7, flag_40}});
            c.call(0x103b0e70, at + 0x2c);
            return cmpl_eq(c, c.u32(player + 0x6c), c.r(3));
        };
        // Own half of the pitch for the side: the first pair of lookups in the original's layout.
        const auto in_own_half = [&] {
            if (cmp_eq(c, side8) && cmp_lt(c, s8(c.u8(match + ball_row_at)), 4)) return true;
            return cmp_eq(c, side8, 1) && cmp_gt(c, s8(c.u8(match + ball_row_at)), 4);
        };
        const auto distance = [&](std::uint32_t back) {
            c.set({{3, player}, {4, row()}, {5, column()}});
            c.call(0x1022e760, back);
            return c.f(1);
        };
        compare_logical(c, wrap(kind8), 8);
        switch (kind8) {
        case 1:
        case 2: {
            const double d = distance(kind8 == 1 ? 0x101f3c88 : 0x101f3cb4);
            const double f2 = c.f32(singles + 0x18), f0 = c.f32(player + (kind8 == 1 ? 0xe0 : 0xec));
            truncate(fp.multiply_add(kind8 == 1 ? 0x101f3c90 : 0x101f3cbc, f2, d, f0), 0xe8, 0xec);
            break;
        }
        case 3: {
            const auto finish_tail = [&](std::uint32_t call_back, std::int32_t pos_lo, std::int32_t pos_hi, std::int32_t pos_store, std::int32_t pos_load, std::uint32_t site) {
                const double f2 = c.f32(player + 0xb0), f1 = c.f32(singles + 0x54), f0 = c.f32(player + 0x8c);
                c.set_f(1, f1);
                truncate(fp.multiply_add(site, f2, f1, f0), pos_store == 0xe0 ? 0xe8 : 0xe0, pos_store == 0xe0 ? 0xec : 0xe4);
                c.set({{3, match}, {4, player}});
                call(c, 0x101e3d50, call_back);
                if (cmp_eq(c, std::int16_t(c.r(3)) & 0x40)) return;
                if (cmp_gt(c, std::int16_t(score))) scale(0x118, pos_lo, pos_hi, pos_store, pos_load, false);
                else scale(0x78, 0xe4, 0xe0, 0xe8, 0xec, false);
            };
            if (in_own_half()) {
                if (turn_matches(0x101f3d10, 2, 0)) { score = 0x2710; break; }
                if (turn_matches(0x101f3d58, 2, 1)) { score = 0x2328; break; }
                finish_tail(0x101f3dc0, 0xec, 0xe8, 0xe0, 0xe4, 0x101f3da4);
            } else {
                if (turn_matches(0x101f3e54, 3, 0)) {
                    draw(c, 1000, 0x101f3e94);
                    score = wrap(std::int16_t(10000 - c.r(3)));
                    break;
                }
                if (turn_matches(0x101f3ea8, 3, 1)) {
                    draw(c, 500, 0x101f3ee8);
                    score = wrap(std::int16_t(c.r(3) + 0x2328));
                    break;
                }
                // 0x101f3f00: the same lookup with the other slots.
                const double f2 = c.f32(player + 0xb0), f1 = c.f32(singles + 0x54), f0 = c.f32(player + 0x8c);
                c.set_f(1, f1);
                truncate(fp.multiply_add(0x101f3f00, f2, f1, f0), 0xe0, 0xe4);
                c.set({{3, match}, {4, player}});
                call(c, 0x101e3d50, 0x101f3f1c);
                if (cmp_eq(c, std::int16_t(c.r(3)) & 0x40)) break;
                if (cmp_gt(c, std::int16_t(score))) scale(0x118, 0xe4, 0xe0, 0xe8, 0xec, false);
                else scale(0x78, 0xe4, 0xe0, 0xe8, 0xec, false);
            }
            break;
        }
        case 4: {
            c.set({{3, match}, {4, player}});
            call(c, 0x101e3d50, 0x101f3fb4);
            c.put32(frame.at(0x44), cmp_eq(c, std::int16_t(c.r(3)), 1) ? 100 : 0);
            score = wrap(std::int16_t(c.u32(frame.at(0x44))));
            break;
        }
        case 5: {
            c.set({{3, match}, {4, side}});
            call(c, 0x101da5a0, 0x101f3fec);
            if (cmp_ne(c, s8(c.r(3))) && cmpl_eq(c, excluded)) {
                const double f2 = c.f32(player + 0xcc);
                const auto ability = s8(c.u8(c.u32(player + 0x70) + 0xf));
                const double minus = magic_double();
                const auto f0 = convert((ability - 1) * 0x14, 0xe4, 0xe0, minus, true);
                truncate(fp.fsubs(f2, f0), 0xe8, 0xec);
                keep(c, {{17, 0}});
                for (; cmp_lt(c, s8(rank), 0x32); keep(c, {{17, rank + 1}})) {
                    c.set(3, match + wrap(side8 * 0xd8) + 0x7aac);
                    call(c, 0x101da750, 0x101f4060);
                    c.set(5, c.r(3));
                    c.set({{3, c.u32(c.r(26))}, {4, rank}});
                    c.call(0x1014c890, 0x101f4070);
                    if (cmpl_eq(c, c.r(3), c.u32(player + 0x6c))) { score = wrap(std::int16_t(10000 - s8(rank))); break; }
                }
                break;
            }
            if (cmpl_eq(c, player, c.u32(match + goalkeeper_first)) || cmpl_eq(c, player, c.u32(match + goalkeeper_second))) {
                score = wrap(-10000);
                break;
            }
            truncate(c.f32(player + 0xcc), 0xe0, 0xe4);
            if (cmp_le(c, std::int16_t(score))) break;
            const auto team = c.u32(player + 0x6c);
            if (cmp_lt(c, s8(c.u16(team + 0x5c)), 0x14)) scale(0x1e0, 0xe4, 0xe0, 0xe8, 0xec, true);
            else if (cmp_lt(c, s8(c.u16(c.u32(player + 0x6c) + 0x5c)), 0x18)) scale(0x118, 0xe4, 0xe0, 0xe8, 0xec, true);
            const auto trait = c.u32(player + 0x6c);
            if (cmp_ne(c, s8(c.u8(trait + 0x66)))) {
                draw(c, wrap(s8(c.u8(c.u32(player + 0x6c) + 0x66))), 0x101f419c);
                if (cmp_eq(c, std::int32_t(c.r(3)))) scale(0x1e0, 0xe4, 0xe0, 0xe8, 0xec, true);
            }
            const double f2 = c.f64(pool + 0x2a0);
            const auto bonus = std::int16_t(c.u16(c.u32(player + 0x70) + 0xa));
            double minus = magic_double();
            const auto f1 = convert(bonus, 0xe4, 0xe0, minus, true);
            c.set_f(1, f1);
            const double f31 = fp.frsp(fp.multiply_add(0x101f4204, f2, f1, c.f64(pool + 0x118)));
            c.set_f(31, f31);
            minus = magic_double();
            const auto f0 = convert(std::int16_t(score), 0xec, 0xe8, minus, true);
            truncate(fp.fmuls(f31, f0), 0xd8, 0xdc);
            break;
        }
        case 6:
        case 7: {
            bool near_goal = false;
            c.set({{3, match}, {4, row()}});
            call(c, 0x101ee9c0, 0x101f424c);
            if (cmp_eq(c, s8(c.r(3)))) {
                // Not in the outer rows: how close is the ball (4.1 limit)?
                c.put32(frame.at(0x40), cmp_ne(c, side8) ? 0 : 0xb);
                c.set({{3, match}, {4, row()}, {5, column()}, {6, 4}, {7, wrap(s8(c.u32(frame.at(0x40))))}});
                call(c, 0x101ef8d0, 0x101f4294);
                fp.fcmp(0, c.f(1), c.f64(pool + 0x2a8));
                near_goal = (c.cr(0) & 8) != 0;
                if (near_goal) {
                    // The `ball in the side's own half` pair of lookups again, with the long value at the end.
                    const bool own = in_own_half();
                    const auto first_at = own ? 0x101f42e0u : 0x101f449cu;
                    const auto second_at = own ? 0x101f4328u : 0x101f44e4u;
                    const auto round = own ? 0u : 1u;
                    if (turn_matches(first_at, round, 0)) { score = 0x2710; break; }
                    if (turn_matches(second_at, round, 1)) { score = 0x2328; break; }
                    trait_10c(c, player);
                    const auto doubled = wrap(s8(c.r(3)) * 2);
                    c.set(4, doubled);
                    c.set(3, c.u32(player + 0x70));
                    score = wrap(std::int16_t(s8(c.u8(c.r(3) + 0x29)) * 5 + doubled));
                    if (cmp_le(c, std::int32_t(score))) break;
                    const auto team = c.u32(player + 0x6c);
                    const auto grade = s8(c.u16(team + 0x5c));
                    if (cmp_lt(c, grade, 0x14)) scale(0x1a0, 0xdc, 0xd8, 0xe0, 0xe4, true);
                    else if (cmp_lt(c, s8(c.u16(c.u32(player + 0x6c) + 0x5c)), 0x18)) scale(0x1d8, 0xdc, 0xd8, 0xe0, 0xe4, true);
                    const double f2 = c.f64(pool + 0x2a0);
                    const auto bonus = std::int16_t(c.u16(c.u32(player + 0x70) + 0xa));
                    double minus = magic_double();
                    const auto f1 = convert(bonus, 0xdc, 0xd8, minus, true);
                    c.set_f(1, f1);
                    const double blended = fp.frsp(fp.multiply_add(own ? 0x101f4458 : 0x101f4614, f2, f1, c.f64(pool + 0x118)));
                    c.set_f(own ? 30 : 29, blended);
                    minus = magic_double();
                    const auto f0 = convert(std::int16_t(score), 0xe4, 0xe0, minus, true);
                    truncate(fp.fmuls(blended, f0), 0xe8, 0xec);
                    break;
                }
            }
            // 0x101f4650
            c.set({{3, match}, {4, row()}});
            call(c, 0x101ee9c0, 0x101f465c);
            c.set(3, wrap(s8(c.r(3))));
            if (cmp_ne(c, s8(c.r(3)))) {
                c.put32(frame.at(0x3c), cmp_eq(c, side8) ? 0xb : 0);
                c.set({{3, match}, {4, row()}, {5, column()}, {6, 4}, {7, wrap(s8(c.u32(frame.at(0x3c))))}});
                call(c, 0x101ef8d0, 0x101f46a4);
                fp.fcmp(0, c.f(1), c.f64(pool + 0x128));
                if ((c.cr(0) & 8) != 0) {
                    const double f3 = c.f32(singles + 0x18), f2 = c.f32(player + 0xa4), f1 = c.f32(player + 0xb0);
                    c.set_f(1, f1);
                    const double f0 = fp.fmuls(f1, c.f32(singles + 0x54));
                    truncate(fp.multiply_add(0x101f46c4, f3, f2, f0), 0xd8, 0xdc);
                    break;
                }
            }
            const double first = distance(0x101f46e8);
            c.set_f(28, first);
            const double second = distance(0x101f46fc);
            truncate(fp.multiply_add(0x101f4700, second, first, c.f32(player + 0xa4)), 0xd8, 0xdc);
            break;
        }
        case 8: {
            const double d = distance(0x101f4724);
            truncate(fp.fsubs(c.f32(singles + 0x20), d), 0xd8, 0xdc);
            break;
        }
        default:
            fail(0x101f4750, 0x3e0d, 0x7b, 0x4d, 0x101f4794);
            break;
        }
        // The best so far.
        bool better = true;
        if (cmpl_ne(c, c.r(18))) {
            const auto best = c.i16(frame.at(0x3a));
            c.set(3, wrap(best));
            better = cmp_gt(c, std::int16_t(score), best);
        }
        if (better) {
            c.put16(frame.at(0x3a), score);
            keep(c, {{18, player}});
        }
    }
    if (cmpl_ne(c, c.r(18))) return result(c, c.r(18));
    if (cmpl_ne(c, c.r(16))) return result(c, 0);
    keep(c, {{18, 0}});
    fail(0x101f47f0, 0x3e22, 0x5c, 0x48, 0x101f4834);
    return result(c, c.r(18));
}

// ---------------------------------------------------------------------------
// 0x101f4860
void adjust_side_ratings(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t amount) {
    Frame frame(c, 0x101f4860, 0x50, {.first_gpr = 29});
    c.put32(frame.at(0x68), match);
    c.put8(frame.at(0x6f), side);
    c.put16(frame.at(0x72), amount);
    c.set(4, c.u32(frame.at(0x68)));
    keep(c, {{31, c.r(4) + wrap(s8(c.u8(frame.at(0x6f))) * std::int32_t(side_size)) + players_at}, {30, 0}});
    for (; cmp_lt(c, s8(c.r(30)), 0x14); keep(c, {{30, c.r(30) + 1}, {29, c.r(31)}, {31, c.r(31) + player_size}})) {
        const auto player = c.r(31);
        if (cmp_ge(c, s8(position(c, player)))) {
            c.set(3, wrap(c.i16(player + 0x36)));
            c.put16(player + 0x36, wrap(c.i16(player + 0x36) + c.i16(frame.at(0x72))));
        }
    }
}
}
