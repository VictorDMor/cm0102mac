// MATCH_MAN.CPP squad_b block (match_man_squad_b.hpp): 0x1020c5d0.
#include "match_man_squad_b.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "match_man_adapter.hpp"
#include "match_man_squad_b_detail.hpp"
#include <bit>
#include <cmath>

namespace cm::aot::match_man {
namespace {
namespace ts = tactic_side;
namespace tm = tactic_match;
namespace tc = tactic_club;
namespace tl = tactic_local;
namespace tt = tactic_stats;
using guest::compare;
using guest::compare_logical;
using guest::Fpu;
using guest::Frame;
using guest::Kept;

constexpr std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }
constexpr std::int32_t s16(std::uint32_t value) { return std::int16_t(value); }
constexpr std::uint32_t u(std::int32_t value) { return std::uint32_t(value); }
constexpr std::uint32_t flip(std::int32_t value) { return u(value) ^ 0x80000000u; }  // xoris r,r,0x8000
constexpr std::uint32_t exponent_word = 0x43300000;                                // lis r,0x4330

// The pool of doubles at TOC -0x5278 is read by offset: 1.0 at +0, the int-to-double constant 2^52 + 2^31 at +0x90.
constexpr std::uint32_t one = 0x0, conversion = 0x90;
// The pool of singles at TOC -0x527c.
constexpr std::uint32_t single_base = 0xc, single_hit = 0x18, single_far = 0x1c;
// Callees outside MATCH_MAN.CPP (0x10459b50 is COMP_STATS, 0x10718130 DATE, 0x10780fc0 libm pow, 0x10775540 abs).
constexpr std::uint32_t pow_entry = 0x10780fc0, abs_entry = 0x10775540, situation = 0x101f21d0, rank_of = 0x101eee20,
                        stats_of = 0x10459b50, days_between = 0x10718130, judged = 0x10219e40;

// The compiler's x / 11 (mulhw by 0x2e8ba2e9, srawi 1, plus the sign bit) and x / 99 (mulhw by 0xa57eb503, add x,
// srawi 6, plus the sign bit) are the truncating divisions; both magic constants were checked against `/` for
// 300,000 values and the extremes.
constexpr std::int32_t over_11(std::int32_t x) { return x / 11; }
constexpr std::int32_t over_99(std::int32_t x) { return x / 99; }
}

std::uint32_t classify_tactic(Cpu& c, std::uint32_t side) {
    constexpr std::uint32_t entry = 0x1020c5d0;
    c.set(0, c.lr());
    // f29-f31 at -0x18, r18-r31 at -0x58 (stmw; a word of padding sits under f29).
    Frame frame(c, entry, 0xb0, {.first_gpr = 18, .gpr_offset = -0x58, .first_fpr = 29, .fpr_offset = -0x18});
    Fpu fpu(c);
    // What the original keeps in r18-r31 across its calls (each callee's prologue saves them). r18, r21, r22,
    // r26, r29 and r30 keep the caller's values until set.
    Kept<18> r18(c, c.r(18));
    Kept<19> r19(c, 0);
    Kept<20> r20(c, 0);
    Kept<21> r21(c, c.r(21));
    Kept<22> r22(c, c.r(22));
    Kept<23> r23(c, c.toc(-0x527c));
    Kept<24> r24(c, c.toc(-0x7b5c));
    Kept<25> r25(c, c.toc(-0x5278));
    Kept<26> r26(c, c.r(26));
    Kept<27> r27(c, c.toc(-0x7b80));
    Kept<28> r28(c, side);
    Kept<29> r29(c, c.r(29));
    Kept<30> r30(c, c.r(30));
    Kept<31> r31(c, 0);
    // f29-f31 likewise: written to the registers before each call.
    double f29 = c.f(29), f30 = c.f(30), f31 = c.f(31);
    const auto sync = [&] { c.set_f(29, f29); c.set_f(30, f30); c.set_f(31, f31); };
    const auto call_out = [&](std::uint32_t target, std::uint32_t back) { sync(); c.call(target, back); };
    const auto K = [&](std::uint32_t offset) { return c.f64(std::uint32_t(r25) + offset); };
    const auto S = [&](std::uint32_t offset) { return double(c.f32(std::uint32_t(r23) + offset)); };
    const auto at = [&](std::uint32_t offset) { return frame.at(std::int32_t(offset)); };
    const auto byte_at = [&](std::uint32_t offset) { return c.u8(at(offset)); };
    const auto match = [&] { return c.u32(side + ts::match); };
    const auto number = [&] { return s8(c.u8(side + ts::number)); };
    const auto restore_toc = [&] { c.set(2, c.u32(frame.at(0x14))); };
    // The int-to-double idiom: 0x43300000 and the flipped value at sp+`offset`, loaded as a double and reduced by the
    // pool's 2^52 + 2^31 (fsub, or fsubs where the original rounds to single).
    const auto convert = [&](std::uint32_t offset, std::int32_t value) {
        c.put32(at(offset), exponent_word);
        c.put32(at(offset + 4), flip(value));
        return fpu.fsub(c.f64(at(offset)), K(conversion));
    };
    const auto convert_single = [&](std::uint32_t offset, std::int32_t value) {
        c.put32(at(offset), exponent_word);
        c.put32(at(offset + 4), flip(value));
        return fpu.fsubs(c.f64(at(offset)), K(conversion));
    };
    // `fcmpo cr0,a,b; cror eq,lt,eq`: true when a <= b (ordered).
    const auto fcmp_le = [&](double a, double b) {
        fpu.fcmp(0, a, b);
        const auto cr = c.cr(0);
        c.set_cr(0, std::uint8_t((cr & ~2) | ((cr & 0xa) ? 2 : 0)));
        return a < b || a == b;
    };
    // The compiler's min/max pair: `lower` when the value is below it, else `upper` when it is above, else the
    // value (a NaN passes through).
    const auto clamp = [&](double value, double lower, double upper) {
        fpu.fcmp(0, lower, value);
        if (lower > value) return lower;
        fpu.fcmp(0, upper, value);
        return upper < value ? upper : value;
    };

    // ---- The abilities of both sides ------------------------------------------------------------------------
    // The two clubs' ratings; in a live match the judged abilities of the 20 players of each side replace them.
    r30 = u(c.i16(c.u32(side + ts::club) + tc::rating));
    r29 = u(c.i16(c.u32(side + ts::rival_club) + tc::rating));
    if (compare(c, s8(c.u8(match() + tm::live)), 0) == 0) {
        r18 = r30;
        r19 = r29;
    } else {
        r22 = 0;
        r26 = 0;
        while (compare(c, s8(r22), 2) < 0) {
            r21 = 0;
            r18 = match() + std::uint32_t(r26) + tm::players;
            while (compare(c, s8(r21), 20) < 0) {
                if (compare(c, s8(c.u8(std::uint32_t(r18) + tactic_player::slot)), 0) >= 0) {
                    // 0x10219e40(judge, the player's person, the side's club, kind 2, no estimate): the
                    // current ability as the judge sees it.
                    const bool first_side = s8(r22) == 0;
                    c.set({{3, c.u32(side + ts::judge)}, {4, c.u32(std::uint32_t(r18) + tactic_player::person)},
                           {5, c.u32(side + ts::club)}, {6, 2}, {7, 0xffffffff}});
                    call(c, judged, first_side ? 0x1020c684 : 0x1020c6a8);
                    if (first_side) r20 = std::uint32_t(r20) + u(s16(c.r(3)));
                    else r19 = std::uint32_t(r19) + u(s16(c.r(3)));
                }
                r21 = std::uint32_t(r21) + 1;
                r18 = std::uint32_t(r18) + tm::player_size;
            }
            r26 = std::uint32_t(r26) + tm::player_stride;
            r22 = std::uint32_t(r22) + 1;
        }
        // Each side's sum becomes sum * 50 / 11 (the product wraps), as 16 bits; the side's own first.
        const auto scaled = [&](std::uint32_t sum) { return u(s16(u(over_11(std::int32_t(sum * 50u))))); };
        const std::uint32_t sum_first = r20, sum_second = r19;
        const bool first = compare(c, number(), 0) == 0;
        r18 = scaled(first ? sum_first : sum_second);
        r19 = scaled(first ? sum_second : sum_first);
    }

    // ---- The situation bytes (sp+0x38..0x3c) -----------------------------------------------------------------
    c.set({{3, match()}, {4, c.u8(side + ts::number)}, {5, at(tl::own)}, {6, at(tl::rival)}, {7, at(tl::byte_3a)},
           {8, at(tl::byte_39)}, {9, at(tl::byte_38)}, {10, 0}});
    call_out(situation, 0x1020c77c);

    // ---- The base value from the ability gap and the match's results --------------------------------------------
    const std::int32_t own_ability = s16(r18), rival_ability = s16(r19);
    const std::int32_t gap = own_ability - rival_ability;
    double base = fpu.fdiv(convert(0x48, own_ability), K(0x140));
    base = fpu.fsub(K(0x20), base);
    const double gap_value = convert(0x50, gap);
    base = fpu.fmul(K(0x138), base);
    base = fpu.fdiv(gap_value, base);
    base = fpu.fadd(K(one), base);  // f0, kept until 0x1020c9d0
    const std::uint32_t match_record = match();
    const std::int32_t wins = c.i16(match_record + tm::wins), draws = c.i16(match_record + tm::draws),
                       losses = c.i16(match_record + tm::losses);
    const std::int32_t played = losses + (draws + wins);
    compare(c, played, 0);
    // divw: the shares of wins and of losses as whole percent, as signed bytes (zero when nothing was played).
    const std::int32_t win_share = s8(u(played != 0 ? wins * 100 / played : 0));
    compare(c, played, 0);
    const std::int32_t loss_share = s8(u(played != 0 ? losses * 100 / played : 0));
    c.set({{7, u(win_share)}, {8, u(loss_share)}});
    const std::int32_t first_side = number();
    // One of the two shares divided by the other; below the pool's break value it is steeper.
    const auto bend = [&](std::int32_t numerator, std::int32_t denominator, std::uint32_t site_high, std::uint32_t site_low) {
        const double top = convert(0x48, numerator);
        const double bottom = convert(0x50, denominator);
        const double ratio = fpu.fdiv(top, bottom);
        fpu.fcmp(0, ratio, K(one));
        if (ratio > K(one)) return fpu.multiply_add(site_high, K(0x78), ratio, K(one));
        return fpu.multiply_add(site_low, K(0x148), ratio, K(0x150));
    };
    double shaped = base;
    bool bent = false;
    if (compare(c, first_side, 0) == 0 && compare(c, loss_share, 0) != 0) {
        shaped = bend(win_share, loss_share, 0x1020c880, 0x1020c890);
        bent = true;
    }
    if (!bent && compare(c, first_side, 1) == 0 && compare(c, win_share, 0) != 0) {
        shaped = bend(loss_share, win_share, 0x1020c8f0, 0x1020c900);
        bent = true;
    }
    const double window = clamp(shaped, K(0x158), K(0x160));

    // ---- Minute-weighted strength -------------------------------------------------------------------------
    // 4 * minutes / 99 as a signed byte, at most 20 (the magic division by 99).
    std::int32_t progress = s8(u(over_99(std::int32_t(u(c.i16(match_record + tm::clock)) * 4u))));
    compare(c, progress, 20);
    if (progress > 20) progress = 20;
    c.set({{5, exponent_word}, {6, flip(progress)}});
    const std::int32_t own_flag = s8(c.u8(match_record + u(number()) + tm::flag_b));
    const double progress_value = convert(0x50, progress);
    double mix = fpu.fmul(progress_value, window);
    const double progress_again = convert(0x48, progress);
    const double remaining = fpu.fsub(K(0x28), progress_again);
    const double flag_value = convert(0x40, own_flag);
    mix = fpu.multiply_add(0x1020c9d0, remaining, base, mix);
    const double flag_factor = fpu.multiply_add(0x1020c9d4, K(0x168), flag_value, K(one));
    f31 = fpu.fdiv(mix, K(0x28));
    f31 = fpu.fmul(f31, flag_factor);
    if (fcmp_le(f31, K(0x178))) f31 = K(0x170);

    // The situation bytes 0x3c/0x3b are shifted by one or two depending on the side.
    if (compare(c, s8(byte_at(tl::byte_38)), 0) != 0) {
        bool shifted = false;
        if (compare(c, number(), 1) == 0) {
            const auto value = byte_at(tl::own);
            if (compare(c, s8(value), 0) != 0) {
                c.put8(at(tl::own), value + 2);
                shifted = true;
            }
        }
        if (!shifted && compare(c, number(), 0) == 0) {
            const auto value = byte_at(tl::rival);
            if (compare(c, s8(value), 0) != 0) c.put8(at(tl::rival), value + 2);
            else c.put8(at(tl::own), byte_at(tl::own) + 1);
        }
    }

    // In a live match the rank of the two sides (0x101eee20) corrects the own byte by twice their difference.
    r19 = match();
    if (compare(c, s8(c.u8(std::uint32_t(r19) + tm::live)), 0) != 0) {
        c.set({{3, r19}, {4, c.u8(side + ts::number)}});
        call_out(rank_of, 0x1020ca68);
        r18 = c.r(3);
        const std::int32_t other = number() == 0 ? 1 : 0;  // cntlzw, rlwinm: the other side's number
        c.set({{3, r19}, {4, u(s8(u(other)))}});
        call_out(rank_of, 0x1020ca88);
        r31 = u(s8(std::uint32_t(r18) - c.r(3)));
        if (compare(c, std::int32_t(r31), 0) != 0)
            c.put8(at(tl::own), byte_at(tl::own) - u(s8(u(std::int32_t(r31) * 2))));
    }

    // ---- Corrections from the minute, the scoreline and the competition ---------------------------------------------
    const std::uint32_t match_now = match();
    f29 = convert(0x40, over_11(c.i16(match_now + tm::clock)));  // the minute in elevenths
    if (compare(c, s8(byte_at(tl::byte_38)), 0) == 0 && compare(c, s8(byte_at(tl::byte_39)), 0) == 0 &&
        compare(c, s8(byte_at(tl::own)) + s8(byte_at(tl::rival)), 0) == 0) {
        // Level and quiet: the side with the higher flag byte (match +0x1c6) is favoured or not by the minute.
        const std::uint32_t flags = match_now + tm::flag_b;
        const std::int32_t own_byte = s8(c.u8(flags + u(number())));
        const std::int32_t other_byte = s8(c.u8(flags + u(number() == 0 ? 1 : 0)));
        const auto order = compare(c, other_byte, own_byte);
        if (order > 0) {
            double factor = fpu.fdiv(f29, K(0x188));
            factor = fpu.fadd(K(0x180), factor);
            f31 = fpu.fmul(f31, factor);
        } else if (order < 0) {
            f31 = fpu.fmul(f31, K(0x190));
        }
    }
    const std::uint32_t club_record = c.u32(side + ts::club), rival_record = c.u32(side + ts::rival_club);
    {
        double factor = fpu.fdiv(convert(0x40, s8(byte_at(tl::own)) - s8(byte_at(tl::rival))), K(0x88));
        factor = fpu.fsub(K(one), factor);
        f31 = fpu.fmul(f31, factor);
    }

    // The club's standing in the fixture's competition (0x10459b50 for each club), from the cc18.. part of the
    // listing: a bonus f30 from the club's recent and coming results and the rank gap to the rival's record.
    const auto standing = [&] {
        const bool have_club_stats = compare(c, std::int32_t(r26), 0) != 0;
        f30 = S(single_base);
        if (have_club_stats) {
            bool measured = false;
            if (compare(c, c.i32(std::uint32_t(r26) + tt::a58), 1) > 0) {
                c.set({{3, r27}, {4, c.u32(std::uint32_t(r26) + tt::a64)}, {5, c.u32(std::uint32_t(r26) + tt::a68)}});
                call_out(days_between, 0x1020cc40);
                if (compare(c, std::int32_t(c.r(3)), 0x1e) < 0) {
                    std::int32_t lowest = std::int32_t((0u - c.u32(std::uint32_t(r26) + tt::a30)) * 2u);
                    const std::int32_t negated = std::int32_t(0u - c.u32(std::uint32_t(r26) + tt::a58));
                    if (compare(c, negated, lowest) < 0) lowest = negated;
                    f30 = convert_single(0x40, lowest);
                    measured = true;
                }
            }
            if (!measured && compare(c, c.i32(std::uint32_t(r26) + tt::a44), 1) > 0) {
                c.set({{3, r27}, {4, c.u32(std::uint32_t(r26) + tt::a50)}, {5, c.u32(std::uint32_t(r26) + tt::a54)}});
                call_out(days_between, 0x1020cca4);
                if (compare(c, std::int32_t(c.r(3)), 10) < 0) {
                    std::int32_t highest = std::int32_t(c.u32(std::uint32_t(r26) + tt::a1c) * 2u);
                    const std::int32_t value = c.i32(std::uint32_t(r26) + tt::a44);
                    if (!(compare(c, value, highest) <= 0)) highest = value;
                    f30 = convert_single(0x40, highest);
                }
            }
            // The bonus times its own size (the sign kept), floored at the pool's single when the side's flag is clear.
            const double magnitude = fpu.frsp(std::fabs(f30));
            f30 = fpu.fmuls(f30, magnitude);
            if (compare(c, s8(c.u8(match() + u(number()) + tm::flag_b)), 0) == 0) {
                const double floor_value = S(single_base);
                fpu.fcmp(0, floor_value, f30);
                if (floor_value > f30) f30 = floor_value;
            }
            if (compare_logical(c, c.u8(std::uint32_t(r26) + tt::rank), 5) < 0 &&
                compare(c, s8(byte_at(tl::own)) - 3, s8(byte_at(tl::rival))) < 0) {
                const std::uint32_t table = c.u32(std::uint32_t(r24));
                const std::uint32_t rival_entry = c.u32(table + c.u32(c.u32(side + ts::fixture)) * 4);
                c.set({{3, c.u32(rival_entry + tactic_table_entry::source)}, {4, c.u32(side + ts::rival_club)}, {5, table}});
                call_out(stats_of, 0x1020cd60);
                r27 = c.r(3);
                if (compare(c, std::int32_t(r27), 0) != 0 &&
                    compare_logical(c, c.u8(std::uint32_t(r27) + tt::rank), c.u8(std::uint32_t(r26) + tt::rank)) > 0) {
                    const std::uint32_t fixture = c.u32(side + ts::fixture);
                    const std::int32_t weight = c.i16(c.u32(c.u32(std::uint32_t(r24)) + c.u32(fixture) * 4) +
                                                      tactic_table_entry::weight);
                    double scale = compare(c, weight, 0) > 0 ? fpu.fdiv(K(0x1a0), convert(0x40, weight)) : K(one);
                    const std::uint32_t record = c.u32(fixture + tactic_fixture::record);
                    const std::int32_t record_value = c.i16(record + tactic_fixture::record_value);
                    c.set({{3, record}, {4, fixture}});
                    if (compare(c, record_value, 10) >= 0) {
                        const std::uint32_t own_club = c.u32(side + ts::club);
                        const std::int32_t limit = record_value * 0x1f4;
                        const std::int32_t rating = c.i16(own_club + tc::rating);
                        c.set({{3, own_club}, {5, u(limit)}, {6, u(rating)}});
                        if (compare(c, rating, limit) > 0) {
                            c.set({{3, exponent_word}, {4, flip(rating - 0x1388)}});
                            double over = convert(0x40, rating - 0x1388);
                            over = fpu.fdiv(over, K(0x1a8));
                            double above = convert(0x48, rating - limit);
                            over = fpu.fadd(K(one), over);
                            above = fpu.fdiv(above, K(0x1a8));
                            scale = fpu.fmul(scale, fpu.fadd(over, above));
                        }
                    }
                    c.set_f(1, scale);
                    c.set_f(2, K(0x20));
                    call_out(pow_entry, 0x1020ce44);
                    restore_toc();
                    const double powered = c.f(1);
                    const std::int32_t rank_gap = std::int32_t(c.u8(std::uint32_t(r27) + tt::rank)) -
                                                  std::int32_t(c.u8(std::uint32_t(r26) + tt::rank));
                    const std::int32_t shape = s8(c.u8(match() + u(number()) + tm::flag_b));
                    const double gap_scaled = convert(0x40, rank_gap);
                    const double cube_base = fpu.fmul(powered, gap_scaled);
                    double cube = fpu.fmul(cube_base, cube_base);
                    const double shape_square = convert(0x48, shape * shape);
                    cube = fpu.fmul(cube_base, cube);
                    double divisor = fpu.multiply_add(0x1020ceb8, K(0x1b8), shape_square, K(0x1b0));
                    divisor = fpu.fdiv(cube, divisor);
                    divisor = fpu.fadd(K(one), divisor);
                    f31 = fpu.fmul(f31, fpu.frsp(clamp(divisor, K(one), K(0x88))));
                }
            }
        }
        // The bonus, spread over the pool's range, scales the strength.
        double bonus = fpu.fdiv(f30, K(0x1c8));
        bonus = fpu.fadd(K(one), bonus);
        f31 = fpu.fmul(f31, fpu.frsp(clamp(bonus, K(0x1c0), K(0x100))));
    };
    if (compare_logical(c, c.u32(club_record + tc::region), c.u32(rival_record + tc::region)) != 0) {
        f31 = fpu.fmul(f31, K(0x198));
    } else {
        const std::uint32_t fixture = c.u32(side + ts::fixture);
        const std::uint32_t competition = c.u32(fixture + tactic_fixture::record);
        if (compare_logical(c, competition, 0) != 0) {
            f31 = fpu.fmul(f31, K(0x78));
            if (compare_logical(c, competition, c.u32(club_record + tc::link)) == 0) {
                const std::uint32_t entry_record = c.u32(c.u32(std::uint32_t(r24)) + c.u32(fixture) * 4);
                if (compare_logical(c, entry_record, 0) != 0) {
                    const std::uint32_t source = c.u32(entry_record + tactic_table_entry::source);
                    if (compare_logical(c, source, 0) != 0) {
                        c.set({{3, source}, {4, club_record}, {5, fixture}, {6, u(s8(byte_at(tl::rival)))}});
                        call_out(stats_of, 0x1020cc18);
                        r26 = c.r(3);
                        standing();
                    }
                }
            }
        }
    }

    // ---- The situation's factors --------------------------------------------------------------------------------
    // x and y are the signed bytes at sp+0x3b and sp+0x3c; the original keeps them in r3, r4 (and r5, r6 raw).
    const std::int32_t x = s8(byte_at(tl::rival)), y = s8(byte_at(tl::own));
    const std::int32_t sum = x + y;
    c.set({{6, byte_at(tl::rival)}, {5, byte_at(tl::own)}, {3, u(x)}, {4, u(y)}});
    if (compare(c, sum, 6) > 0) f31 = fpu.fmul(f31, K(0x1d0));
    else if (compare(c, sum, 4) > 0) f31 = fpu.fmul(f31, K(0x1d8));

    const std::uint32_t side_raw = c.u8(side + ts::number);
    const std::uint32_t match_cf = match();
    const std::uint32_t style_raw = c.u8(match_cf + u(s8(side_raw)) + tm::flag_b);
    c.set({{8, side_raw}, {10, match_cf}, {7, style_raw}, {9, u(s8(style_raw))}});
    const std::int32_t style = s8(style_raw);
    const std::int32_t level_31 = std::int32_t(std::uint32_t(r31));
    const std::int32_t rating_29 = std::int32_t(std::uint32_t(r29)), rating_30 = std::int32_t(std::uint32_t(r30));
    bool candidate = compare(c, style, 0) != 0;
    if (!candidate) {
        c.set(9, u(level_31));
        candidate = compare(c, level_31, 0) < 0;
    }
    if (candidate) {
        // A big favourite or a near-level game with the favourite behind: reaches 0x1020cfc0.
        bool gate = false;
        if (compare(c, rating_29, 0x1d4c) > 0) {
            c.set(9, u(rating_29 - rating_30));
            gate = compare(c, rating_29 - rating_30, 0x7d0) > 0;
        }
        if (!gate && compare(c, rating_29, 0x2134) > 0) {
            c.set(9, u(rating_29 - rating_30));
            gate = compare(c, rating_29 - rating_30, 0x2ee) > 0;
        }
        if (!gate && compare(c, rating_29, 0x1f40) > 0) {
            c.set(9, u(y - 1));
            gate = compare(c, y - 1, x) > 0;
        }
        if (gate) {
            bool apply = false, check_order = true;
            c.set(9, u(x - y));
            if (compare(c, x - y, 2) < 0) {
                const std::int32_t kind = c.i16(match_cf + tm::period_end);
                c.set(9, u(kind));
                if (compare(c, kind, 0x1ef) == 0) {
                    apply = true;
                    check_order = false;
                }
            }
            if (check_order) {
                c.set({{10, u(x)}, {9, u(y)}});
                apply = !(compare(c, x, y) > 0);
            }
            if (apply) f31 = fpu.fmul(f31, K(0x1e0));
        }
    }

    c.set(9, u(style));  // extsb. r9,r7
    if (compare(c, style, 0) != 0) {
        const std::int32_t packed = style + (y - x) * 2;
        c.set({{10, u(y - x)}, {9, flip(packed)}});
        const double value = convert(0x40, packed);
        f31 = fpu.fmul(f31, fpu.multiply_add(0x1020d02c, K(0x1f0), value, K(0x1e8)));
    } else {
        c.set({{10, flip(y - x)}, {9, exponent_word}});
        double factor = fpu.fdiv(convert(0x48, y - x), K(0x88));
        factor = fpu.fsub(K(one), factor);
        f31 = fpu.fmul(f31, factor);
    }

    // The score's ordering: ahead (or level with the decider byte set, or level on the first side) or the rest.
    c.set({{10, u(y)}, {9, u(x)}});
    const std::int32_t ordering = compare(c, y, x);
    bool ahead = false;
    if (ordering > 0) {
        ahead = true;
    } else if (ordering == 0) {
        const std::int32_t third = s8(byte_at(tl::byte_3a));
        c.set(9, u(third));
        if (compare(c, third, 0) != 0) {
            ahead = true;
        } else {
            c.set(8, u(s8(side_raw)));
            ahead = compare(c, s8(side_raw), 1) == 0;
        }
    }
    if (ahead) {
        const double top = K(one), cap = K(0xa8);
        fpu.fcmp(0, top, f29);
        double floor_minute = top > f29 ? top : f29;
        double ratio = fpu.fdiv(cap, floor_minute);
        fpu.fcmp(0, top, ratio);
        double factor;
        if (!(top < ratio)) {
            factor = K(one);
            fpu.fcmp(0, factor, f29);
            if (!(factor > f29)) factor = f29;
            factor = fpu.fdiv(cap, factor);
        } else {
            factor = K(one);
        }
        f31 = fpu.fmul(f31, factor);
    } else {
        // Behind, or level with the decider byte clear and not the first side (0x1020d0ec): y > x cannot reach here.
        c.set({{9, u(y)}, {8, u(x)}});
        bool applies = compare(c, y, x) < 0;
        if (!applies) {
            c.set(7, u(style));
            applies = compare(c, style, 0) == 0;
        }
        if (applies) {
            double factor = fpu.fdiv(f29, K(0xa8));
            fpu.fcmp(0, K(one), factor);
            double limit = K(one);
            if (!(limit > factor)) limit = factor;
            f31 = fpu.fmul(f31, limit);
        }
    }

    // x - y between 3 and 5: a penalty linear in the gap.
    c.set(7, u(x - y));
    if (compare(c, x - y, 2) > 0 && compare(c, x - y, 6) < 0) {
        c.set({{8, flip(x - y)}, {7, exponent_word}});
        const double value = convert(0x40, x - y);
        f31 = fpu.fmul(f31, fpu.multiply_add(0x1020d160, K(0x200), value, K(0x1f8)));
    }

    // Floors and ceilings from the clubs' ratings.
    bool raise = false;
    if (compare(c, sum, 5) < 0) {
        c.set(7, u(level_31));
        if (compare(c, level_31, -1) > 0) {
            c.set({{7, u(y)}, {5, u(x)}});
            if (compare(c, y, x) == 0) {
                const std::int32_t quiet = s8(byte_at(tl::byte_38));
                c.set(5, u(quiet));
                if (compare(c, quiet, 0) == 0) {
                    const std::int32_t own_rating = c.i16(c.u32(side + ts::club) + tc::rating);
                    c.set(6, u(own_rating));
                    const std::int32_t rival_rating = c.i16(c.u32(side + ts::rival_club) + tc::rating);
                    c.set(5, u(rival_rating));
                    raise = compare(c, own_rating, rival_rating) > 0;
                }
            }
        }
    }
    if (!raise) {
        c.set(5, u(level_31));
        raise = compare(c, level_31, 0) > 0;
    }
    if (raise) {
        const double lowest = K(one);
        fpu.fcmp(0, f31, lowest);
        if (!(f31 > lowest)) f31 = lowest;
    }
    if (compare(c, sum, 4) < 0 && compare(c, level_31, -1) > 0 && compare(c, y - 2, x) >= 0) {
        const std::int32_t own_rating = c.i16(c.u32(side + ts::club) + tc::rating);
        c.set(4, u(own_rating));
        const std::int32_t rival_rating = c.i16(c.u32(side + ts::rival_club) + tc::rating);
        c.set(3, c.u32(side + ts::rival_club));
        if (compare(c, own_rating, rival_rating) > 0) {
            const double ceiling = K(0x208);
            fpu.fcmp(0, f31, ceiling);
            if (!(f31 > ceiling)) f31 = ceiling;
        }
    }

    // The manager's trait softens the strength: f30 = f31 scaled, tempered by pow(trait, 2), floored just below f31.
    const std::uint32_t manager = c.u32(side + ts::manager);
    c.set(3, manager);
    if (compare_logical(c, manager, 0) != 0 && compare(c, level_31, -1) > 0) {
        const std::int32_t trait = s8(c.u8(manager + 0x2e));
        c.set({{3, exponent_word}, {5, u(trait)}, {4, flip(trait - 10)}});
        double centred = convert(0x40, trait - 10);
        centred = fpu.fdiv(centred, K(0x8));
        const double scale = fpu.fadd(K(one), centred);
        const double trait_value = convert(0x48, trait);
        f30 = fpu.fmul(f31, scale);
        c.set_f(1, trait_value);
        c.set_f(2, K(0x78));
        call_out(pow_entry, 0x1020d288);
        restore_toc();
        const double threshold = fpu.fdiv(c.f(1), K(0x220));
        fpu.fcmp(0, f30, threshold);
        if (f30 < threshold) {
            const std::uint32_t again = c.u32(side + ts::manager);
            c.set(3, again);
            const double value = convert(0x50, s8(c.u8(again + 0x2e)));
            c.set_f(1, value);
            c.set_f(2, K(0x78));
            call_out(pow_entry, 0x1020d2cc);
            restore_toc();
            const double divisor = K(0x218), factor = K(0x210);
            const double scaled = fpu.fdiv(c.f(1), divisor);
            f30 = fpu.multiply_add(0x1020d2dc, f30, factor, scaled);
        }
        const double lower = fpu.fsub(f31, K(one));
        fpu.fcmp(0, f30, lower);
        if (f30 < lower) f30 = lower;
        f31 = f30;
    }

    // A small gap in a live side with a flag: the minute raises the strength to a floor.
    if (compare(c, rating_29, 0x1d4c) > 0 || compare(c, level_31, 0) < 0) {
        const std::uint32_t match_d3 = match();
        const std::int32_t sn = number();
        c.set(3, match_d3 + u(sn));
        if (compare(c, s8(c.u8(match_d3 + u(sn) + tm::flag_b)), 0) != 0) {
            c.set(3, u(y - x));
            call_out(abs_entry, 0x1020d33c);
            if (compare(c, std::int32_t(c.r(3)), 2) < 0) {
                c.set(3, u(y));
                bool lift = compare(c, y, x) >= 0;
                if (!lift) {
                    fpu.fcmp(0, f29, K(0x1c8));
                    lift = f29 < K(0x1c8) || compare(c, y + x, 3) > 0;
                }
                if (lift) {
                    double floor_value = fpu.fdiv(f29, K(0x228));
                    floor_value = fpu.fadd(K(0x200), floor_value);
                    fpu.fcmp(0, f31, floor_value);
                    if (!(f31 < floor_value)) f31 = floor_value;
                }
            }
        }
    }

    // Quiet game, the scores one apart: the minute lowers the strength to a cap.
    const std::int32_t quiet_38 = s8(byte_at(tl::byte_38)), quiet_39 = s8(byte_at(tl::byte_39)),
                       quiet_3a = s8(byte_at(tl::byte_3a));
    c.set(3, u(quiet_39));
    if (compare(c, quiet_39, 0) != 0) {
        c.set(3, u(quiet_38));
        if (compare(c, quiet_38, 0) == 0) {
            c.set(3, u(quiet_3a));
            if (compare(c, quiet_3a, 0) == 0) {
                c.set({{3, u(y)}, {4, u(x)}});
                bool near = compare(c, y, x) == 0;
                if (!near) {
                    c.set(3, u(y + 1));
                    near = compare(c, y + 1, x) == 0;
                }
                if (near) {
                    const std::int32_t third = s8(c.u8(match() + u(number()) + tm::flag_b));
                    c.set({{3, u(third)}, {4, match()}});
                    if (compare(c, third, 0) == 0) {
                        const auto cap_minute = [&] {
                            double minute = K(0x230);
                            fpu.fcmp(0, f29, minute);
                            if (f29 < minute) minute = f29;
                            double cap = fpu.fdiv(minute, K(0x228));
                            return fpu.fadd(K(0x200), cap);
                        };
                        const double cap = cap_minute();
                        fpu.fcmp(0, f31, cap);
                        if (!(f31 > cap)) f31 = cap_minute();
                    }
                }
            }
        }
    }

    // Both sides have scored: the goal rate against the pool's limit dampens the strength.
    const std::uint32_t raw_rival = byte_at(tl::rival), raw_own = byte_at(tl::own);
    const std::uint32_t raw_38 = byte_at(tl::byte_38);
    c.set({{3, raw_rival}, {6, u(x)}});
    if (compare(c, x, 1) > 0) {
        c.set({{7, raw_own}, {5, u(y)}});
        if (compare(c, y, 1) > 0) {
            c.set({{8, raw_38}, {4, u(s8(raw_38))}});
            bool damp = false;
            if (compare(c, s8(raw_38), 0) == 0) {
                c.set(4, u(quiet_39));
                if (compare(c, quiet_39, 0) != 0) {
                    c.set({{5, flip(x + y)}, {4, exponent_word}});
                    const double numerator = fpu.fadd(K(0x238), f29);
                    const double total = convert(0x40, x + y);
                    const double rate = fpu.fdiv(numerator, total);
                    fpu.fcmp(0, rate, K(0x240));
                    damp = rate < K(0x240);
                }
            }
            if (!damp) {
                c.set(4, u(quiet_39));
                bool measure = compare(c, quiet_39, 0) == 0;
                if (!measure) {
                    c.set(4, u(s8(raw_38)));
                    measure = compare(c, s8(raw_38), 0) != 0;
                }
                if (measure) {
                    c.set({{5, flip(x + y)}, {4, exponent_word}});
                    const double total = convert(0x40, x + y);
                    const double rate = fpu.fdiv(f29, total);
                    fpu.fcmp(0, rate, K(0x240));
                    damp = rate < K(0x240);
                }
            }
            if (damp) f31 = fpu.fmul(f31, K(0x1e0));
        }
    }

    // ---- The result -----------------------------------------------------------------------------------------------
    // Three early exits store a single from the pool at match + 0x30f0 + side * 4 and return a class: 1 (a late,
    // level or one-behind game of kind 0x3de, with the minute above a margin of the strength), and 5. Otherwise
    // the strength is stored and the class follows from it.
    const double doubled = fpu.fmul(K(0x88), f31);  // f2 stays in the original: 0x1020d5a0 reads it again
    const double headroom = fpu.fsub(K(0x248), doubled);
    c.set_f(1, K(0x248));
    const std::uint32_t side_bytes = u(number()) * 4;
    const auto store_single = [&](std::uint32_t record, std::uint32_t pool_offset) {
        c.put32(record + tm::strength + side_bytes, c.u32(std::uint32_t(r23) + pool_offset));
    };
    fpu.fcmp(0, f29, headroom);
    if (f29 > headroom) {
        const std::uint32_t record = match();
        const std::int32_t kind = c.i16(record + tm::period_end);
        c.set({{7, record}, {4, u(kind)}});
        if (compare(c, kind, 0x3de) == 0) {
            c.set({{5, byte_at(tl::own)}});
            c.set({{4, u(x)}, {6, u(y)}, {5, u(y + 1)}});
            bool hit = compare(c, y + 1, x) == 0;
            if (!hit && compare(c, s8(byte_at(tl::byte_39)), 0) != 0 && compare(c, s8(byte_at(tl::byte_38)), 0) == 0 &&
                compare(c, s8(byte_at(tl::byte_3a)), 0) == 0)
                hit = compare(c, y, x) == 0;
            if (hit) {
                store_single(record, single_hit);
                c.set(4, record + side_bytes);
                return guest::result(c, 1);
            }
        }
    }
    c.set({{5, byte_at(tl::own)}, {7, u(x)}, {8, u(y)}});
    if (compare(c, y + x, 5) < 0) {
        const double below = fpu.fsub(K(0x250), doubled);
        fpu.fcmp(0, f29, below);
        if (f29 > below) {
            const std::uint32_t record = match();
            c.set(6, record);
            const std::int32_t kind = c.i16(record + tm::period_end);
            if (compare(c, kind, 0x3de) == 0 && compare(c, y, x) == 0 && compare(c, s8(byte_at(tl::byte_3a)), 0) == 0) {
                fpu.fcmp(0, f31, K(one));
                bool hit = f31 > K(one);
                if (!hit) {
                    c.set(4, record + u(number()));
                    hit = compare(c, std::int32_t(c.u8(record + u(number()) + tm::flag_a)), 1) == 0;
                }
                if (hit) {
                    store_single(record, single_hit);
                    c.set(4, record + side_bytes);
                    return guest::result(c, 1);
                }
            }
        }
    }
    // A low strength with the side's flag set and a lead of two or more is raised to the pool's step.
    fpu.fcmp(0, f31, K(0x10));
    if (f31 < K(0x10)) {
        const std::uint32_t at_side = match() + u(number());
        c.set(4, at_side);
        if (compare(c, s8(c.u8(at_side + tm::flag_b)), 0) != 0 && compare(c, y - x, 1) > 0) f31 = K(0x200);
    }
    const std::int32_t lead = y - x;
    c.set(6, u(lead));
    bool far = false;
    if (compare(c, lead, 3) > 0) {
        fpu.fcmp(0, f31, K(one));
        far = f31 > K(one);
    }
    if (!far) {
        fpu.fcmp(0, f29, K(0xa8));
        if (f29 > K(0xa8)) {
            c.set(4, u(y));
            if (compare(c, y, x) >= 0) {
                const std::int32_t square = (lead + 1) * (lead + 1);  // mullw
                const double limit = fpu.fdiv(K(0x88), f31);
                c.set(3, u(square));
                const double lead_value = convert(0x40, square);
                c.set_f(1, lead_value);
                fpu.fcmp(0, lead_value, limit);
                if (lead_value > limit) {
                    const std::uint32_t rival_club_record = c.u32(side + ts::rival_club);
                    const std::uint32_t own_club_record = c.u32(side + ts::club);
                    c.set({{3, rival_club_record}, {4, own_club_record}});
                    far = true;
                    for (const auto person : tc::persons) {
                        if (compare_logical(c, own_club_record, c.u32(rival_club_record + person)) == 0) {
                            far = false;
                            break;
                        }
                    }
                }
            }
        }
    }
    if (far) {
        const std::uint32_t record = match();
        c.set(4, record + side_bytes);
        store_single(record, single_far);
        return guest::result(c, 5);
    }
    // The strength is stored, and the class follows from it; a live match whose stored strength is within the
    // pool's tolerance keeps the class it has.
    const std::uint32_t record = match();
    c.set(5, record);
    if (compare(c, s8(c.u8(record + tm::live)), 0) != 0) {
        c.set(4, u(number()));
        const double previous = double(c.f32(record + tm::strength + side_bytes));
        const double difference = std::fabs(fpu.fsub(f31, previous));
        c.set_f(1, difference);
        fpu.fcmp(0, difference, K(0x210));
        if (difference < K(0x210)) return guest::result(c, c.u8(record + u(number()) + tm::flag_a));
    }
    const double stored = fpu.frsp(f31);
    c.set_f(1, stored);
    const std::uint32_t record_again = match();
    c.put_f32(record_again + tm::strength + side_bytes, float(stored));
    std::uint32_t grade;
    if (fcmp_le(f31, K(0x1e0))) {
        grade = 4;
    } else {
        fpu.fcmp(0, f31, K(0x258));
        if (f31 < K(0x258)) {
            grade = 3;
        } else {
            fpu.fcmp(0, f31, K(0x20));
            grade = f31 > K(0x20) ? 1 : 2;
        }
    }
    return guest::result(c, grade);
}
}
