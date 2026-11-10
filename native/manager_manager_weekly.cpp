// MANAGER_MANAGER.CPP weekly pass 0x101a3350 (manager_manager_weekly.hpp): the loop over every club and
// the first sections of a club's body (to 0x101a4e08); the later sections are in the part_c, part_d and
// part_e files, included inside `class Club`. A section returns the address of the original's label where
// the body goes on (`label::`), which `body()` chains.
//
// A register the original reads before a club writes it: r13 (the level byte) is read at 0x101a3f1c,
// 0x101a4508 and 0x101a4658 on paths where this club has not set it, so it holds what an earlier club
// left there, or the caller's r13 (the daily update's, 0x10196390) for the first club to get there.
// `Kept<13>` keeps that; the level must never be initialised per club.
//
// The original keeps in registers (set once by the prologue): the board's rows cell r26 -> the
// object, the clubs' cursor r31, the row offset r16, the game object's cell r17 (0x106..), the
// leagues' cell r18, the draw table's cells r19-r22, the news record r23 and the constants pool r24.
#include "manager_manager_weekly.hpp"
#include <bit>
#include <stdexcept>
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "manager_manager_detail.hpp"
#include "manager_manager_weekly_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;
using guest::Kept;
using guest::compare;
using guest::compare_logical;
using guest::nonzero;
using guest::nonzero_logical;
using namespace weekly_detail;
namespace {
namespace callee {
constexpr std::uint32_t club_flag = 0x100c7d00, bad_run = 0x100d3c40, own_club = 0x100c76c0, region_person = 0x102b7450,
    person_ok = 0x102aed80, date_plus = 0x10717890, date_of = 0x10453190, staff_of = 0x10459b50, hire_by_club = 0x106487d0,
    hire_any = 0x10647730, post_event = 0x106891b0, stats_of = 0x10462870, days_between = 0x10718130;
}

std::int32_t sbyte(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
std::uint32_t word(std::int32_t value) { return std::uint32_t(value); }
std::int32_t mulhw(std::uint32_t a, std::int32_t b) { return std::int32_t((std::int64_t(std::int32_t(a)) * b) >> 32); }
// The sign bit of the halved xor trick the original compares two days with: the sign of
// ((a ^ b) >> 1) - ((a ^ b) & mask), a and b the halfwords' sign-extended values.
std::uint32_t halved_xor_sign(std::int32_t a, std::int32_t b, std::int32_t mask) {
    const auto x = a ^ b;
    return (std::uint32_t(x >> 1) - std::uint32_t(x & mask)) >> 31;
}
// One club's body. The members that live in the original's non-volatile registers are Kept<N>:
// r13 the manager's record's byte at +0x36 of the league's person (sign-extended), r14 the
// manager record's word at +0x1c, r15 a staff record or a person, r25 and r28 working values,
// r27 the person the region names.
class Club {
public:
    Club(Cpu& c, Frame& frame, std::uint32_t object)
        : c(c), frame(frame), fpu(c), t(draw_table::cells(c)), object(object), club(c.r(31)), row_offset(c.r(16)),
          pool(c.r(24)), news(c.r(23)), r13(c, c.r(13)), r14(c, c.r(14)), r15(c, c.r(15)), r25(c, c.r(25)),
          r27(c, c.r(27)), r28(c, c.r(28)) {}

    // 0x101a33b4..: the sections in order, each returning the label the body goes on at; every path
    // ends at the advance to the next club (0x101a8a78).
    void body() {
        if (sections() != label::club_end) throw std::logic_error("MANAGER_MANAGER weekly pass: a club's body did not reach its end");
    }

private:
    std::uint32_t sections() {
        if (const auto next = opening()) return next;
        auto next = row_update();
        if (next == 0) next = own_events();
        if (next == label::hub) next = hub();
        if (next == 0x101a3cb8 || next == 0x101a3d5c) next = after_factor(next);
        // Part D: the sections from 0x101a6958 on; each returns the label it ends at.
        if (next == label::wide) next = ladder();
        if (next == label::quiet) next = quiet_stage();
        if (next == label::rows_end) next = rows_stage();
        if (next == label::own_end) next = closing();
        if (next == 0x101a7f7c) next = finish();
        return next;
    }

    // ---- 0x101a33b4..0x101a3404: the club's tests; a club failing one is skipped. ----
    std::uint32_t opening() {
        c.set(3, club);
        c.call(callee::club_flag, 0x101a33bc);
        const auto flagged = sbyte(c.r(3));
        c.set(0, word(flagged));
        if (nonzero(c, flagged)) {
            const auto region = c.u32(club + club::region);
            c.set(3, region);
            if (!nonzero_logical(c, region)) return label::club_end;
            const auto monthly = c.u32(region + region::monthly);
            c.set(0, monthly);
            if (!nonzero_logical(c, monthly)) return label::club_end;
        }
        const auto manager = c.u32(club + club::manager);
        c.set(3, manager);
        if (!nonzero_logical(c, manager)) return label::club_end;
        const auto manager_word = c.u32(manager + 0x1c);
        c.set(0, manager_word);
        if (!nonzero_logical(c, manager_word)) return label::club_end;
        c.set(3, club);
        c.call(callee::bad_run, 0x101a33fc);
        const auto ok = sbyte(c.r(3));
        c.set(0, word(ok));
        if (!nonzero(c, ok)) return label::club_end;
        return 0;
    }

    // ---- 0x101a3404..0x101a34ec: the row's two halfwords move toward each other, the third follows. ----
    // The first is clamped into [half, twice] of the second one's earlier value; then the value moves
    // by a fraction of the gap; the spread (+0x16) absorbs the change. Ends with the test whether this
    // club is its region's own (0x100c76c0): not, the hub at 0x101a3ad4.
    std::uint32_t row_update() {
        const auto rows = c.u32(object);
        const auto row = rows + row_offset;
        c.set(4, row);
        const auto value = std::int32_t(c.i16(row + row::value)), target = std::int32_t(c.i16(row + row::target));
        c.set({{5, word(value)}, {6, word(target)}});
        const auto manager = c.u32(club + club::manager);
        c.set(3, manager);
        const auto half = (value + (value < 0 ? 1 : 0)) >> 1;
        std::int32_t clamped;
        if (half > target) {
            clamped = half;
        } else {
            const auto twice = std::int32_t(word(value) << 1);
            if (twice >= target) {
                clamped = target;
            } else {
                c.set(6, word(twice));
                clamped = twice;
            }
        }
        c.set(0, word(clamped));
        c.put32(frame.at(slot::record), manager);
        r14 = c.u32(manager + 0x1c);
        c.put16(row + row::target, word(clamped));
        // The conversions of both halfwords through the doubles' bias, then fctiwz of a fraction of
        // the gap (the frame keeps every word).
        c.put32(frame.at(slot::conv_b), high_word);
        auto f2 = c.f64(pool + 0x38);
        const auto row_again = c.u32(object) + row_offset;
        auto f3 = c.f64(pool + 0x48);
        const auto old_value = c.i16(row_again + row::value);
        const auto old_target = c.i16(row_again + row::target);
        c.put16(frame.at(slot::stage_half), word(old_value));
        c.put32(frame.at(slot::conv_a), high_word);
        c.put32(frame.at(slot::conv_a + 4), word(old_value) ^ 0x80000000u);
        c.put32(frame.at(slot::conv_b + 4), word(old_target) ^ 0x80000000u);
        const auto old = std::int32_t(c.i16(frame.at(slot::stage_half)));
        auto f0 = c.f64(frame.at(slot::conv_a));
        auto f1 = c.f64(frame.at(slot::conv_b));
        f0 = fpu.fsubs(f0, f2);
        f1 = fpu.fsubs(f1, f2);
        f0 = fpu.fsubs(f1, f0);
        f0 = fpu.fmul(f3, f0);
        c.put64(frame.at(slot::conv_c), fpu.fctiwz(0, f0));
        const auto moved = c.u32(frame.at(slot::conv_c + 4));
        c.set_f(1, f1);
        c.set_f(2, f2);
        c.set_f(3, f3);
        c.set(3, moved);
        c.put16(row_again + row::value, word(old) + moved);
        const auto row_third = c.u32(object) + row_offset;
        c.set(5, row_third);
        const auto previous = std::int32_t(c.i16(frame.at(slot::stage_half)));
        const auto now = std::int32_t(c.i16(row_third + row::value));
        const auto spread = std::int32_t(c.i16(row_third + row::spread));
        c.set({{4, word(now)}, {3, word(spread)}});
        const auto change = std::int32_t(std::int16_t(word(previous) - word(now)));
        c.set(0, word(std::int32_t(word(spread) - word(change))));
        c.put16(row_third + row::spread, word(spread) - word(change));
        c.set(3, c.u32(club + club::region));
        c.call(callee::own_club, 0x101a34e8);
        compare_logical(c, club, c.r(3));
        if (club != c.r(3)) return label::hub;
        return 0;
    }

    // ---- 0x101a3ad4..0x101a3cb4: the hub. The league's person must exist with a staff record for the
    // club (r25); of the staff record's four dated posts the first one that is recent (under a week
    // from today) with its count above 1 (above 2 for the last two) gives its count (r3) and a
    // bonus (r4: 25, -30, -5, 5). The count is squared (to a cap of 5 squared: past 5 it feeds
    // the multiply-add), and f31 is that times the bonus. ----
    struct Post { std::uint32_t count, since, until; std::int32_t threshold, bonus; std::uint32_t back; };
    static constexpr Post posts[] = {{0x1c, 0x28, 0x2c, 1, 0x19, 0x101a3b5c}, {0x30, 0x3c, 0x40, 1, -0x1e, 0x101a3b90},
                                    {0x58, 0x64, 0x68, 2, -5, 0x101a3bc4}, {0x44, 0x50, 0x54, 2, 5, 0x101a3bf8}};
    std::uint32_t hub() {
        const auto league = c.u32(club + club::league);
        c.set(3, league);
        if (!nonzero_logical(c, league)) return label::own_end;
        const auto leagues = c.u32(c.r(18));
        const auto record = c.u32(leagues + (c.u32(league) << 2));
        c.set({{0, c.u32(league)}, {3, record}});
        if (!nonzero_logical(c, record)) return label::own_end;
        const auto owner = c.u32(record + 8);
        c.set(3, owner);
        if (!nonzero_logical(c, owner)) return label::own_end;
        c.set({{3, owner}, {4, club}});
        c.call(callee::staff_of, 0x101a3b0c);
        r25 = c.r(3);
        c.set(0, r25);
        if (!nonzero_logical(c, r25)) return label::own_end;
        {
            const auto again = c.u32(club + club::league);
            const auto table = c.u32(c.r(18));
            const auto owner_again = c.u32(c.u32(table + (c.u32(again) << 2)) + 8);
            c.set({{3, owner_again}, {4, club}, {8, table}, {5, frame.at(0x3a)}, {6, frame.at(0x39)}, {7, frame.at(0x38)}});
            c.call(callee::stats_of, 0x101a3b40);
        }
        std::int32_t count = 0, bonus = 0;
        for (const auto& post : posts) {
            const auto n = c.i32(r25 + post.count);
            c.set(0, word(n));
            if (compare(c, n, post.threshold) > 0) {
                c.set({{4, c.u32(r25 + post.since)}, {3, c.toc(toc::date)}, {5, c.u32(r25 + post.until)}});
                c.call(callee::days_between, post.back);
                const auto days = std::int32_t(c.r(3));
                if (compare(c, days, 7) < 0) {
                    const auto raw = c.i32(r25 + post.count);
                    c.set({{0, word(raw)}, {4, word(post.bonus)}, {3, word(std::int32_t(std::int16_t(word(raw))))}});
                    count = std::int32_t(std::int16_t(word(raw)));
                    bonus = post.bonus;
                    return factor(count, bonus);
                }
            }
        }
        c.set({{4, 0}, {3, 0}});
        return factor(0, 0);
    }
    // 0x101a3c18..0x101a3cb4: f31 = (count squared, past 5 a linear extension) times the bonus, as
    // single-precision floating point; then the test against the news record's constant.
    std::uint32_t factor(std::int32_t count, std::int32_t bonus) {
        const auto n = std::int32_t(std::int16_t(word(count)));
        c.set(0, word(n));
        double f0, f1, f2, f3;
        if (compare(c, n, 4) < 0) {
            const auto square = std::int32_t(word(n) * word(n));
            c.put32(frame.at(slot::conv_c), high_word);
            f1 = c.f64(pool + 0x38);
            c.put32(frame.at(slot::conv_c + 4), word(square) ^ 0x80000000u);
            f0 = c.f64(frame.at(slot::conv_c));
            f2 = fpu.fsubs(f0, f1);
            c.set(3, word(square));
        } else {
            std::int32_t m = n;
            if (compare(c, n, 5) > 0) m = 5;
            m = std::int32_t(std::int16_t(word(m)));
            const auto square = std::int32_t(word(m) * word(m));
            c.put32(frame.at(slot::conv_c), high_word);
            f2 = c.f64(pool + 0x38);
            f3 = c.f64(pool + 0x60);
            f0 = c.f64(pool + 0x168);
            c.put32(frame.at(slot::conv_c + 4), word(square) ^ 0x80000000u);
            f1 = c.f64(frame.at(slot::conv_c));
            f1 = fpu.fsub(f1, f2);
            f2 = fpu.multiply_add(0x101a3c80, f3, f1, f0);
            f2 = fpu.frsp(f2);
            c.set_f(3, f3);
            c.set(3, word(square));
        }
        const auto b = std::int32_t(std::int16_t(word(bonus)));
        c.put32(frame.at(slot::conv_c), high_word);
        f3 = c.f64(pool + 0x38);
        c.put32(frame.at(slot::conv_c + 4), word(b) ^ 0x80000000u);
        f0 = c.f32(news + 4);
        f1 = c.f64(frame.at(slot::conv_c));
        f1 = fpu.fsubs(f1, f3);
        const auto f31 = fpu.fmuls(f1, f2);
        c.set(0, high_word);
        c.set(3, word(b) ^ 0x80000000u);
        c.set_f(0, f0);
        c.set_f(1, f1);
        c.set_f(2, f2);
        c.set_f(3, f3);
        c.set_f(31, f31);
        fpu.fcmp(0, f31, f0);
        return (c.cr(0) & 8) ? 0x101a3cb8 : 0x101a3d5c;  // fcmpo; bge: unordered takes it too
    }

    // ======== Part B: 0x101a3cb8..0x101a4e08 ========
    double pool_f(std::uint32_t offset) const { return c.f64(pool + offset); }
    double news_f(std::uint32_t offset) const { return c.f32(news + offset); }
    // fcmpo into cr0; the caller tests the bits (8 less, 4 greater, 2 equal, 1 unordered) as the
    // original's branch senses do (ble/bge take the branch on "not greater"/"not less").
    std::uint8_t fcmp(double a, double b) {
        fpu.fcmp(0, a, b);
        return c.cr(0);
    }
    // The module's draw 0x10195e10 below `bound`.
    std::int32_t module_draw(std::uint32_t bound, std::uint32_t back) {
        c.set(3, bound);
        c.call(0x10195e10, back);
        return std::int32_t(c.r(3));
    }
    static std::int32_t trunc_div(std::int32_t a, std::int32_t b) { return std::int32_t(std::int64_t(a) / b); }

    // ---- 0x101a3cb8..0x101a3d5c: the league's size (halfword at +0x18) scaled by a pool constant
    // picks a factor from the news record; f31 is multiplied by it; then (club not flagged) a ratio
    // of two doubles in the club's row may scale it again and lowers the row's +0x14 by 150. ----
    void league_boost() {
        const auto league = c.u32(club + club::league);
        c.set(3, league);
        c.put32(frame.at(slot::conv_c), c.r(0));
        const auto size = std::int32_t(c.i16(league + 0x18));
        c.set(0, word(size) ^ 0x80000000u);
        double f2 = pool_f(0x48), f0 = pool_f(0x98);
        c.put32(frame.at(slot::conv_c + 4), c.r(0));
        double f1 = c.f64(frame.at(slot::conv_c));
        f1 = fpu.fsub(f1, c.f(3));
        f1 = fpu.fmul(f2, f1);
        f1 = fpu.frsp(f1);
        if (fcmp(f1, f0) & 4) {
            f1 = news_f(0x20);
        } else {
            f0 = pool_f(0);
            if (fcmp(f1, f0) & 8) f1 = news_f(0x24);
        }
        c.set_f(0, f0);
        c.set_f(1, f1);
        c.set_f(2, f2);
        c.set_f(31, fpu.fmuls(c.f(31), f1));
        c.set(3, club);
        c.call(callee::club_flag, 0x101a3d10);
        const auto flagged = sbyte(c.r(3));
        c.set(0, word(flagged));
        if (nonzero(c, flagged)) return;
        const auto row = c.u32(object) + row_offset;
        c.set({{0, c.u32(object)}, {4, row}});
        f0 = c.f64(row + 0x38);
        f1 = c.f64(row + 0x30);
        c.set_f(0, f0);
        c.set_f(1, f1);
        if (!(fcmp(f0, f1) & 4)) return;
        f2 = fpu.frsp(f0);
        f0 = pool_f(0x130);
        f1 = fpu.frsp(f1);
        f1 = fpu.fdivs(f2, f1);
        if (fcmp(f1, f0) & 4) f1 = news_f(0x28);
        c.set_f(0, f0);
        c.set_f(1, f1);
        c.set_f(2, f2);
        const auto old = std::int32_t(c.i16(row + 0x14));
        c.set(3, word(old));
        c.set_f(31, fpu.fmuls(c.f(31), f1));
        c.set(0, word(old) - 0x96);
        c.put16(row + 0x14, word(old) - 0x96);
    }

    // 0x101a3d5c..0x101a3e24: the league's size against the club's reputation: the ratio
    // (size*500+500)/reputation, clamped to news constants, divides or multiplies f31 by the
    // sign of f31. Returns true when the pass jumps to the row stage (0x101a4860).
    bool reputation_scale() {
        const auto league = c.u32(club + club::league);
        c.set(3, league);
        const auto size = std::int32_t(c.i16(league + 0x18));
        c.set({{4, word(size)}, {0, word(size)}});
        if (!nonzero(c, size)) return true;
        const auto reputation = std::int32_t(c.i16(club + club::reputation));
        c.set({{6, word(reputation)}, {0, word(reputation)}});
        if (!nonzero(c, reputation)) return true;
        compare(c, size, 7);
        if (size <= 7) return false;
        const auto top = std::int32_t(word(size) * 0x1f4u);
        c.set(5, word(top));
        c.set(4, high_word);
        c.set(0, word(reputation) ^ 0x80000000u);
        c.put32(frame.at(slot::conv_c), high_word);
        double f3 = pool_f(0x38), f0 = pool_f(0x90);
        const auto bound = top + 0x1f4;
        c.set(5, word(bound) ^ 0x80000000u);
        c.put32(frame.at(slot::conv_c + 0xc), word(reputation) ^ 0x80000000u);
        c.put32(frame.at(slot::conv_c + 4), word(bound) ^ 0x80000000u);
        c.put32(frame.at(slot::conv_c + 8), high_word);
        double f2 = c.f64(frame.at(slot::conv_c)), f1 = c.f64(frame.at(slot::conv_c + 8));
        f2 = fpu.fsubs(f2, f3);
        f1 = fpu.fsubs(f1, f3);
        f1 = fpu.fdivs(f2, f1);
        if (fcmp(f1, f0) & 4) {
            f1 = news_f(0x2c);
        } else {
            f0 = pool_f(0);
            if (fcmp(f1, f0) & 8) f1 = news_f(0x24);
        }
        double f31 = c.f(31);
        f0 = news_f(4);
        bool divided = false;
        if (fcmp(f31, f0) & 8) {
            f0 = pool_f(0x50);
            if (fcmp(f1, f0) & 8) {
                f31 = fpu.fdivs(f31, f1);
                divided = true;
            }
        }
        if (!divided) {
            f0 = news_f(4);
            if ((fcmp(f31, f0) & 8)) {
                f0 = pool_f(0x50);
                if (fcmp(f1, f0) & 4) {
                    f31 = fpu.fdivs(f31, f1);
                    divided = true;
                }
            }
            if (!divided) f31 = fpu.fmuls(f31, f1);
        }
        c.set_f(0, f0);
        c.set_f(1, f1);
        c.set_f(2, f2);
        c.set_f(3, f3);
        c.set_f(31, f31);
        return false;
    }

    // 0x101a3e24..0x101a3e48: a club not flagged at +0x23d that is below the news constant: f31 times pool+0x10.
    void unflagged_penalty() {
        auto f31 = c.f(31);
        double f0 = news_f(4);
        c.set_f(0, f0);
        if (fcmp(f31, f0) & 8) {
            const auto byte = c.u8(club + 0x23d);
            c.set(0, byte);
            compare(c, std::int32_t(byte), 1);
            if (byte != 1) {
                f0 = pool_f(0x10);
                f31 = fpu.frsp(fpu.fmul(f31, f0));
                c.set_f(0, f0);
                c.set_f(31, f31);
            }
        }
    }

    // The chain of tests the staff-hire stanzas open with, the league in r4 and the region in r3: the league,
    // its region (with the byte at +0x120 set), the manager record, and the league's record's word at
    // +0x1c (compared with -1, left in `word_at`). False when one of the first four tests fails.
    bool chain(std::int32_t& word_at) {
        const auto league = c.u32(club + club::league);
        c.set(4, league);
        if (!nonzero_logical(c, league)) return false;
        const auto region = c.u32(club + club::region);
        c.set(3, region);
        if (!nonzero_logical(c, region)) return false;
        const auto flag = sbyte(c.u8(region + 0x120));
        c.set(0, word(flag));
        if (!nonzero(c, flag)) return false;
        const auto manager = c.u32(club + club::manager);
        c.set(0, manager);
        if (!nonzero_logical(c, manager)) return false;
        const auto index = c.u32(league);
        c.set(0, index);
        const auto table = c.u32(c.r(18));
        c.set(3, table);
        const auto record = c.u32(table + (index << 2));
        c.set({{0, index << 2}, {3, record}});
        word_at = c.i32(record + 0x1c);
        c.set(0, word(word_at));
        compare(c, word_at, -1);
        return true;
    }

    // The hire and the event: the person by the club (0x106487d0, by index) or any (0x10647730), then the
    // event `code` for them (0x106891b0). bl_* are the addresses of the bl instructions.
    template <class K>
    void hire_post(K& who, bool by_index, std::uint32_t bl_index, std::uint32_t bl_any, std::uint32_t bl_post,
                   std::uint32_t code) {
        c.set({{3, c.u32(c.r(17))}, {4, club}});
        if (by_index) {
            who = 0;
            c.call(callee::hire_by_club, bl_index + 4);
            const auto found = std::int32_t(c.r(3));
            compare(c, found, -1);
            if (found > -1) {
                const auto people = c.u32(c.toc(toc::people));
                c.set({{0, word(found) * 0x70}, {3, people}});
                who = people + word(found) * 0x70;
            }
        } else {
            c.set(5, 0);
            c.call(callee::hire_any, bl_any + 4);
            who = c.r(3);
        }
        post_with(who, code, 0xffffffffu, bl_post);
    }
    void post_with(std::uint32_t who, std::uint32_t code, std::uint32_t last, std::uint32_t bl_post) {
        c.set({{3, c.u32(c.r(17))}, {4, club}, {6, who}, {5, code}, {7, 0}, {8, 0}, {9, last}});
        c.call(callee::post_event, bl_post + 4);
    }
    // x % n as the original's divisions leave it (C++ truncation, the sign following the dividend).
    static std::int32_t rem(std::int32_t x, std::int32_t n) { return x % n; }

    // 0x101a3e48..0x101a4738: the staff record's bytes decide: unequal +0x77/+0x78 scale f31 by the
    // idle/rank bytes; equal ones (and a record not busy) may lead to a hire with an event.
    void staff_rules() {
        const auto a77 = c.u8(r25 + 0x77), a78 = c.u8(r25 + 0x78);
        c.set({{4, a77}, {0, a78}});
        compare_logical(c, a77, a78);
        if (a77 != a78) {
            const auto a = c.u8(r25 + 0x76), b = c.u8(r25 + 0x75);
            c.set({{3, a}, {0, b}});
            compare_logical(c, a, b);
            auto f31 = c.f(31);
            double f0;
            bool skip_to_end = false;
            if (a < b) {
                f0 = news_f(4);
                c.set_f(0, f0);
                if (fcmp(f31, f0) & 8) {
                    f0 = pool_f(0x70);
                    f31 = fpu.frsp(fpu.fmul(f0, f31));
                    c.set_f(0, f0);
                    skip_to_end = true;
                }
            }
            if (!skip_to_end) {
                compare_logical(c, a, b);
                if (a == b) {
                    f0 = news_f(4);
                    c.set_f(0, f0);
                    if (fcmp(f31, f0) & 8) {
                        f0 = pool_f(0x48);
                        f31 = fpu.frsp(fpu.fmul(f0, f31));
                        c.set_f(0, f0);
                    }
                }
            }
            compare_logical(c, a, b);
            if (a > b) {
                f0 = news_f(4);
                c.set_f(0, f0);
                if (fcmp(f31, f0) & 4) {
                    f0 = pool_f(0);
                    f31 = fpu.frsp(fpu.fmul(f0, f31));
                    c.set_f(0, f0);
                }
            }
            c.set_f(31, f31);
            return;
        }
        const auto busy = c.u8(r25 + 4);
        c.set(0, busy);
        if (nonzero_logical(c, busy)) return;
        // r3 is still the league (0x101a3d5c); a stanza from here needs the league, the region ...
        if (!chain_3ed4()) return hire_block_a();
        const auto level = sbyte(r13);
        c.set(0, word(level));
        const auto idle = std::int32_t(c.u8(r25 + 0x75));
        const auto quarter = trunc_div(level, 4);
        c.set({{3, word(idle)}, {0, word(quarter)}});
        compare(c, idle, quarter);
        if (idle >= quarter) {
            const auto x = draw(0x101a3f54);
            const auto reputation = std::int32_t(c.i16(club + club::reputation));
            c.set(5, word(reputation));
            const auto r = rem(x, 2000);
            c.set({{4, word(x)}, {3, word(x - r)}, {0, word(r + 0x1964)}});
            compare(c, reputation, r + 0x1964);
            if (reputation <= r + 0x1964) return hire_block_a();
        }
        window_hire();
    }
    // 0x101a3ed4..0x101a3f18: the chain with the league in r3, the region in r4 (the word at the record +0x1c must be -1).
    bool chain_3ed4() {
        const auto league = c.r(3);
        if (!nonzero_logical(c, league)) return false;
        const auto region = c.u32(club + club::region);
        c.set(4, region);
        if (!nonzero_logical(c, region)) return false;
        const auto flag = sbyte(c.u8(region + 0x120));
        c.set(0, word(flag));
        if (!nonzero(c, flag)) return false;
        const auto manager = c.u32(club + club::manager);
        c.set(0, manager);
        if (!nonzero_logical(c, manager)) return false;
        const auto index = c.u32(league);
        c.set(0, index);
        const auto table = c.u32(c.r(18));
        c.set(3, table);
        const auto record = c.u32(table + (index << 2));
        c.set({{0, index << 2}, {3, record}});
        const auto word_at = c.i32(record + 0x1c);
        c.set(0, word(word_at));
        compare(c, word_at, -1);
        return word_at == -1;
    }

    // ---- 0x101a3f9c..0x101a4738: the window around today + 100 days (the later of it and the manager
    // record's date), the dated list of 0x103f3670 over it, and what the club's entries add up to. ----
    // The amount: the table's number cut to 0xffff, scaled by a pool constant and truncated, plus a tenth
    // of the module's draw below `bound`; capped. Added into r15.
    void amount(std::uint32_t scale, std::uint32_t bound, std::uint32_t back_step, std::uint32_t back_draw,
                std::int32_t cap) {
        draw_table::step(c, t, back_step);
        const auto x = std::int32_t(c.u16(c.u32(t.cursor_cell)) + c.u32(t.base_cell));
        const auto q = trunc_div(x, 0xffff);
        const auto part = std::int32_t(word(x) - word(q) * 0xffffu);
        c.set(6, word(part));
        c.set(0, word(part) ^ 0x80000000u);
        c.put32(frame.at(slot::conv_c), high_word);
        double f1 = pool_f(0x38), f2 = pool_f(scale);
        c.put32(frame.at(slot::conv_c + 4), word(part) ^ 0x80000000u);
        double f0 = c.f64(frame.at(slot::conv_c));
        f0 = fpu.fsub(f0, f1);
        f0 = fpu.fmul(f2, f0);
        c.put64(frame.at(slot::conv_c + 8), fpu.fctiwz(0, f0));
        c.set_f(1, f1);
        c.set_f(2, f2);
        r15 = c.u32(frame.at(slot::conv_c + 12));
        const auto drawn = module_draw(bound, back_draw);
        const auto tenth = trunc_div(drawn + 5, 10);
        c.set(3, word(tenth));
        r15 = std::uint32_t(r15) + word(tenth);
        compare(c, std::int32_t(r15), cap);
        if (std::int32_t(r15) > cap) r15 = word(cap);
    }

    void window_hire() {
        const auto date_cell = c.toc(toc::date);
        c.set({{4, date_cell}, {3, frame.at(0x5c)}, {5, 100}});
        c.call(0x10717b20, 0x101a3fac);
        const auto year = std::int32_t(c.i16(frame.at(0x5c)));
        const auto day = c.u16(frame.at(0x5e));
        const auto extra = c.u8(frame.at(0x60));
        c.set({{4, word(year)}, {3, day}, {0, extra}});
        c.put16(frame.at(0x8e), day);
        c.put16(frame.at(0x8c), word(year));
        const auto low = c.u16(frame.at(0x8e));
        c.set(4, low);
        c.put8(frame.at(0x90), extra);
        const auto manager = c.u32(club + club::manager);
        c.set(5, manager);
        const auto m1 = c.u32(manager + 0x4c), m2 = c.u32(manager + 0x50);
        c.set({{3, m1}, {0, m2}});
        c.put32(frame.at(0x44), m1);
        const auto m_low = c.u16(frame.at(0x46));
        c.set(3, m_low);
        c.put32(frame.at(0x48), m2);
        compare_logical(c, m_low, low);
        std::uint32_t later;
        if (m_low > low) {
            later = 1;
        } else if (m_low < low) {
            later = 0;
        } else {
            const auto a = std::int32_t(c.i16(frame.at(0x44))), b = std::int32_t(c.i16(frame.at(0x8c)));
            c.set(4, word(a));
            later = halved_xor_sign(a, b, a);
        }
        c.set(0, later);
        if (nonzero(c, sbyte(later))) {
            const auto h = c.u16(manager + 0x4e);
            c.put16(frame.at(0x8e), h);
            const auto d = c.i16(manager + 0x4c);
            c.put16(frame.at(0x8c), word(d));
            const auto e = c.u8(manager + 0x50);
            c.put8(frame.at(0x90), e);
            c.set(0, e);
        }
        const auto date = c.toc(toc::date);
        c.set({{3, date}, {0, 0}});
        c.put32(frame.at(0x3c), 0);
        r15 = c.u16(date + 2);
        r27 = c.u16(frame.at(0x8e));
        c.call(0x10718360, 0x101a4054);
        r13 = c.r(3);
        c.set(3, frame.at(0x8c));
        c.call(0x10718360, 0x101a4060);
        c.set(4, c.r(3));
        const auto list_cell = c.toc(-0x7b40);
        c.set({{3, list_cell}, {5, r27}, {6, r13}});
        c.set({{3, c.u32(list_cell)}, {7, r15}, {8, frame.at(0x3c)}, {9, frame.at(0x40)}});
        c.call(0x103f3670, 0x101a4084);
        Kept<29> entry(c, c.u32(frame.at(0x3c)));
        r27 = 0;
        r28 = 0;
        r13 = 0;
        Kept<30> index(c, 0);
        // 0x101a42b4: the loop over the list's 0x28-byte entries.
        for (;;) {
            const auto count = c.i32(frame.at(0x40));
            c.set(0, word(count));
            compare(c, std::int32_t(index), count);
            if (!(std::int32_t(index) < count)) break;
            const auto id = c.u32(entry + 0x10), mine = c.u32(club);
            c.set({{0, id}, {3, mine}});
            compare(c, std::int32_t(id), std::int32_t(mine));
            if (id == mine) {
                const auto amount_here = c.u32(entry + 0x18);
                c.set(4, amount_here);
                r27 = std::uint32_t(r27) + amount_here;
                amount(0x170, 0x2fa, 0x101a40d4, 0x101a4150, 0x4c4b3f);
                {
                    const auto limit = std::int32_t(word(std::int32_t(r15)) + 0x2625a0u);
                    const auto share = c.i32(entry + 0x18);
                    c.set({{0, word(limit)}, {4, word(share)}});
                    compare(c, share, limit);
                    if (share > limit) r28 = std::uint32_t(r28) + 1;
                }
                amount(0x170, 0x2fa, 0x101a41bc, 0x101a4238, 0x4c4b3f);
                {
                    const auto limit = std::int32_t(word(std::int32_t(r15)) + 0x7270e0u);
                    const auto share = c.i32(entry + 0x18);
                    c.set({{0, word(limit)}, {4, word(share)}});
                    compare(c, share, limit);
                    if (share > limit) {
                        const auto who = c.u32(entry);
                        const auto people = c.u32(c.toc(toc::people));
                        c.set({{0, who * 0x70}, {3, people}});
                        r13 = people + who * 0x70;
                    }
                }
            } else {
                const auto other = c.u32(entry + 0x14);
                c.set(0, other);
                compare(c, std::int32_t(other), std::int32_t(mine));
                if (other == mine) {
                    const auto amount_here = c.u32(entry + 0x18);
                    c.set(0, amount_here);
                    r27 = std::uint32_t(r27) - amount_here;
                }
            }
            index = std::uint32_t(index) + 1;
            entry = entry + 0x28;
        }
        // 0x101a42c0: the list is released.
        const auto list = c.u32(frame.at(0x3c));
        c.set(3, list);
        if (nonzero_logical(c, list)) c.call(0x10775170, 0x101a42d0);
        auto x = draw(0x101a42f0);
        {
            c.set(3, x < 0 ? 1u : 0u);
            const auto r = rem(x, 2);
            c.set(0, word(r + 1));
            compare(c, std::int32_t(r28), r + 1);
            if (std::int32_t(r28) <= r + 1) return;
        }
        amount(0x178, 0x5f5, 0x101a4340, 0x101a43bc, 0x98967f);
        {
            const auto limit = std::int32_t(word(std::int32_t(r15)) + 0xe4e1c0u);
            c.set(0, word(limit));
            compare(c, std::int32_t(r27), limit);
            if (std::int32_t(r27) <= limit) return;
        }
        x = draw(0x101a4420);
        {
            const auto r = rem(x, 2);
            c.set(3, word(r < 0 ? 1 : 0));
            c.set(0, word(r + 2));
            compare(c, std::int32_t(r28), r + 2);
            if (std::int32_t(r28) < r + 2) r13 = 0;
        }
        c.set({{3, c.u32(c.r(17))}, {4, club}, {6, r13}, {9, r27}, {5, 0x5b}, {7, 0}, {8, 0}});
        c.call(callee::post_event, 0x101a4478);
    }

    // 0x101a447c..0x101a45f8: the staff record is idle for less than a fifth of the level plus one,
    // less a draw modulo 3: a hire and the event 0x5d. A failed test goes on at 0x101a45fc.
    void hire_block_a() {
        std::int32_t w;
        if (!chain(w) || w != -1) return hire_block_b();
        auto x = draw(0x101a44e8);
        const auto level = sbyte(r13);
        const auto limit = trunc_div(level, 5) + 1 - rem(x, 3);
        const auto idle = std::int32_t(c.u8(r25 + 0x75));
        c.set({{7, word(x)}, {0, word(idle)}, {4, word(limit)}});
        compare(c, idle, limit);
        if (idle >= limit) return hire_block_b();
        x = draw(0x101a4568);
        c.set(3, word(x) >> 31);
        c.set(0, word(rem(x, 2)));
        const bool by_index = compare(c, rem(x, 2)) == 0;
        hire_post(r13, by_index, 0x101a45a4, 0x101a45d0, 0x101a45f4, 0x5d);
    }

    // 0x101a45fc..0x101a4738: the idle byte above 1 and below a fifth plus a tenth of the level: event 0x42.
    void hire_block_b() {
        std::int32_t w;
        if (!chain(w) || w != -1) return;
        const auto idle = std::int32_t(c.u8(r25 + 0x75));
        c.set(6, word(idle));
        compare_logical(c, word(idle), 1);
        if (idle <= 1) return;
        const auto level = sbyte(r13);
        const auto limit = trunc_div(level, 5) + trunc_div(level, 10);
        c.set({{4, word(level)}, {0, word(limit)}});
        compare(c, idle, limit);
        if (idle >= limit) return;
        const auto x = draw(0x101a46a8);
        const auto r = rem(x, 2);
        c.set({{3, word(x) >> 31}, {0, word(r)}});
        const bool by_index = compare(c, r) == 0;
        hire_post(r13, by_index, 0x101a46e4, 0x101a4710, 0x101a4734, 0x42);
    }

    // ---- 0x101a4738..0x101a4860: the club's +0x80 record's +0x1c record (a manager's) and f31. ----
    void tail_rules() {
        double f31 = c.f(31), f0, f1;
        const auto holder = c.u32(club + 0x80);
        c.set(3, holder);
        if (nonzero_logical(c, holder)) {
            const auto inner = c.u32(holder + 0x1c);
            c.set(3, inner);
            if (nonzero_logical(c, inner)) {
                f0 = news_f(4);
                c.set_f(0, f0);
                if (fcmp(f31, f0) & 8) {
                    auto level = sbyte(c.u8(inner + 0x3d));
                    c.set(0, word(level));
                    compare(c, level, 1);
                    if (level < 1) {
                        c.set(0, 10);
                        c.put8(inner + 0x3d, 10);
                    }
                    const auto again = c.u32(c.u32(club + 0x80) + 0x1c);
                    c.set(3, again);
                    level = sbyte(c.u8(again + 0x3d));
                    c.set(0, word(level));
                    compare(c, level, 1);
                    if (level == 1) {
                        f1 = news_f(0x30);
                    } else {
                        compare(c, level, 0x14);
                        if (level == 0x14) {
                            f1 = news_f(0x24);
                        } else {
                            const auto left = 0x15 - level;
                            c.set(3, word(left) ^ 0x80000000u);
                            c.set(0, high_word);
                            c.put32(frame.at(slot::conv_c), high_word);
                            double f3 = pool_f(0x38);
                            c.put32(frame.at(slot::conv_c + 4), word(left) ^ 0x80000000u);
                            f1 = pool_f(0x180);
                            double f2 = c.f64(frame.at(slot::conv_c));
                            f0 = pool_f(0x98);
                            f2 = fpu.fsubs(f2, f3);
                            f1 = fpu.fdiv(f2, f1);
                            f1 = fpu.frsp(f1);
                            if (fcmp(f1, f0) & 4) {
                                f1 = news_f(0x20);
                            } else {
                                f0 = pool_f(0x20);
                                if (fcmp(f1, f0) & 8) f1 = news_f(0x1c);
                            }
                            c.set_f(2, f2);
                            c.set_f(3, f3);
                        }
                    }
                    c.set_f(0, f0);
                    c.set_f(1, f1);
                    f31 = fpu.fmuls(f31, f1);
                    c.set_f(31, f31);
                }
            }
        }
        f0 = news_f(4);
        c.set_f(0, f0);
        if (!(fcmp(f31, f0) & 8)) return;
        const auto manager = c.u32(club + club::manager);
        c.set(5, manager);
        if (!nonzero_logical(c, manager)) return;
        const auto row_base = c.u32(object);
        c.set({{3, row_base}, {0, row_offset + 2}});
        const auto byte = c.u8(row_base + row_offset + 2);
        c.set(0, byte);
        const auto flag = (byte & 0x80) ? -1 : 0;
        c.set(0, word(flag));
        if (!nonzero(c, flag)) return;
        const auto d1 = c.u32(manager + 0x4c), d2 = c.u32(manager + 0x50);
        c.set({{4, d1}, {5, d2}, {3, c.toc(toc::date)}});
        c.call(callee::days_between, 0x101a483c);
        const auto days = std::int32_t(c.r(3));
        compare(c, days, 0xb4);
        if (days < 0xb4) {
            f0 = pool_f(0x60);
        } else {
            f0 = pool_f(0x10);
        }
        c.set_f(0, f0);
        c.set_f(31, fpu.frsp(fpu.fmul(f0, f31)));
    }

    // ---- 0x101a4860..0x101a48e0: f31, truncated, moves the row's +6, +8 and +0x12 halfwords, and a
    // tenth-ish share of it (pool+0x90) the +0x16 one. ----
    void row_apply() {
        const double f31 = c.f(31);
        const auto bits = fpu.fctiwz(1, f31);
        double f0 = pool_f(0x90);
        auto base = c.u32(object);
        c.set(8, base);
        f0 = fpu.fmul(f0, f31);
        c.set_f(0, f0);
        const auto at_value = row_offset + 6, at_target = row_offset + 8, at_spread = row_offset + 0x16,
                   at_extra = row_offset + 0x12;
        c.set({{7, at_value}, {5, at_target}, {6, at_spread}, {4, at_extra}});
        c.put64(frame.at(slot::conv_c), bits);
        const auto value = std::int32_t(c.i16(base + at_value));
        c.set(3, word(value));
        const auto low1 = c.u32(frame.at(slot::conv_c + 4));
        c.set(0, low1);
        const auto bits0 = fpu.fctiwz(0, f0);
        c.put64(frame.at(slot::conv_c + 8), bits);
        c.set(0, word(value) + low1);
        c.put16(base + at_value, word(value) + low1);
        const auto low2 = c.u32(frame.at(slot::conv_c + 12));
        c.set(0, low2);
        base = c.u32(object);
        c.set(7, base);
        c.put64(frame.at(0xd4), bits0);
        const auto target = std::int32_t(c.i16(base + at_target));
        c.set(3, word(target));
        c.put64(frame.at(0xbc), bits);
        c.set(0, word(target) + low2);
        const auto low3 = c.u32(frame.at(0xd8));
        c.set(3, low3);
        c.put16(base + at_target, word(target) + low2);
        const auto low4 = c.u32(frame.at(0xc0));
        c.set(0, low4);
        base = c.u32(object);
        c.set(7, base);
        const auto spread = std::int32_t(c.i16(base + at_spread));
        c.set(5, word(spread));
        c.set(3, word(spread) + low3);
        c.put16(base + at_spread, word(spread) + low3);
        base = c.u32(object);
        c.set(5, base);
        const auto extra = std::int32_t(c.i16(base + at_extra));
        c.set(3, word(extra));
        c.set(0, word(extra) + low4);
        c.put16(base + at_extra, word(extra) + low4);
    }

    // ---- 0x101a48e0..0x101a49a4: the league record's halfword at +0x3e (a signed byte, r13), and the
    // ratio of the club's reputation to 500 times the league's size, give f1; pow(f1, pool+0x28) is
    // the new f31. ----
    void power_stage() {
        const auto league = c.u32(club + club::league);
        c.set(4, league);
        const auto table = c.u32(c.r(18));
        const auto index = c.u32(league);
        c.set({{3, table}, {0, index}});
        const auto record = c.u32(table + (index << 2));
        c.set(3, record);
        const auto raw = std::int32_t(c.i16(record + 0x3e));
        c.set(0, word(raw));
        r13 = word(sbyte(word(raw)));
        compare(c, sbyte(word(raw)));
        double f1;
        const auto level = sbyte(word(raw));
        if (level > 0) {
            c.set(3, word(level) ^ 0x80000000u);
            c.set(0, high_word);
            c.put32(frame.at(0xb8), word(level) ^ 0x80000000u);
            f1 = pool_f(0x38);
            c.put32(frame.at(0xb4), high_word);
            double f2 = pool_f(0x188);
            double f0 = c.f64(frame.at(0xb4));
            f0 = fpu.fsub(f0, f1);
            f1 = fpu.fdiv(f2, f0);
            c.set_f(2, f2);
            c.set_f(0, f0);
        } else {
            f1 = pool_f(0x50);
        }
        const auto size = std::int32_t(c.i16(league + 0x18));
        c.set(0, word(size));
        compare(c, size, 10);
        if (size >= 10) {
            const auto top = std::int32_t(word(size) * 0x1f4u);
            c.set(5, word(top));
            const auto reputation = std::int32_t(c.i16(club + club::reputation));
            c.set(6, word(reputation));
            compare(c, reputation, top);
            if (reputation > top) {
                const auto a = reputation - 0x1388;
                c.set({{0, word(a)}, {3, high_word}, {4, word(a) ^ 0x80000000u}});
                const auto b = reputation - top;
                c.set(0, word(b));
                c.put32(frame.at(0xb8), word(a) ^ 0x80000000u);
                c.set(0, word(b) ^ 0x80000000u);
                double f4 = pool_f(0x38);
                c.put32(frame.at(0xb4), high_word);
                double f3 = pool_f(0x190);
                double f0 = c.f64(frame.at(0xb4));
                c.put32(frame.at(0xc0), word(b) ^ 0x80000000u);
                double f2 = fpu.fsub(f0, f4);
                double f5 = pool_f(0x50);
                c.put32(frame.at(0xbc), high_word);
                f0 = c.f64(frame.at(0xbc));
                f2 = fpu.fdiv(f2, f3);
                f0 = fpu.fsub(f0, f4);
                f2 = fpu.fadd(f5, f2);
                f0 = fpu.fdiv(f0, f3);
                f0 = fpu.fadd(f2, f0);
                f1 = fpu.fmul(f1, f0);
                c.set_f(0, f0);
                c.set_f(2, f2);
                c.set_f(3, f3);
                c.set_f(4, f4);
                c.set_f(5, f5);
            }
        }
        const double f2 = pool_f(0x28);
        c.set_f(1, f1);
        c.set_f(2, f2);
        c.call(0x10780fc0, 0x101a49a4);
        c.set(2, c.u32(frame.at(0x14)));
        c.set_f(31, c.f(1));
    }

    // ---- 0x101a49a4..0x101a4a74: equal +0x77/+0x78 bytes end the club's pass; the row's byte at +2 (bit 7)
    // too; else the row's flag bits (5, 4) and its +6 halfword move by 750 when the bytes at +0x75
    // and +0x78 / +0x77 differ. r15 leaves holding the amount moved. ----
    std::uint32_t after_power() {
        const auto s77 = c.u8(r25 + 0x77);
        c.set(7, s77);
        const auto s78 = c.u8(r25 + 0x78);
        c.set(5, s78);
        compare_logical(c, s77, s78);
        if (s77 == s78) return label::own_end;
        const auto row = c.u32(object) + row_offset;
        c.set({{0, c.u32(object)}, {3, row}});
        const auto flags = c.u8(row + 2);
        c.set(4, flags);
        const auto top = (flags & 0x80) ? -1 : 0;
        c.set(0, word(top));
        if (nonzero(c, top)) return label::own_end;
        const auto s75 = c.u8(r25 + 0x75);
        c.set(6, s75);
        r15 = 0;
        compare_logical(c, s78, s75);
        if (s78 < s75) {
            const auto bit = (flags & 0x20) ? -1 : 0;
            c.set(0, word(bit));
            if (!nonzero(c, bit)) {
                c.set(0, 1);
                r15 = 0x2ee;
                c.set(4, flags | 0x20);
                c.put8(row + 2, flags | 0x20);
                return label::rows_end;
            }
            const auto value = std::int32_t(c.i16(row + 6));
            c.set(4, word(value));
            compare(c, value, 0xfa0);
            if (value >= 0xfa0) return label::rows_end;
            c.set(0, word(value) + 0x2ee);
            c.put16(row + 6, word(value) + 0x2ee);
            return label::rows_end;
        }
        compare_logical(c, s77, s75);
        if (s77 > s75) {
            const auto bit = (flags & 0x10) ? -1 : 0;
            c.set(0, word(bit));
            if (!nonzero(c, bit)) {
                c.set(0, 1);
                r15 = word(-0x2ee);
                c.set(4, flags | 0x10);
                c.put8(row + 2, flags | 0x10);
                return label::rows_end;
            }
            const auto value = std::int32_t(c.i16(row + 6));
            c.set(4, word(value));
            compare(c, value, 0x1964);
            if (value <= 0x1964) return label::rows_end;
            c.set(0, word(value) - 0x2ee);
            c.put16(row + 6, word(value) - 0x2ee);
            return label::rows_end;
        }
        const auto busy = c.u8(r25 + 4);
        c.set(0, busy);
        compare_logical(c, busy, 4);
        if (busy <= 4) return label::quiet;
        return event_5f();
    }

    // ---- 0x101a4a74..0x101a4e08: a long chain of draws against the staff record's numbers; the one
    // that passes them all hires (event 0x5f) and ends at 0x101a6958; any failure goes on with the row of stanzas of Part C
    // (0x101a4e08, manager_manager_weekly_part_c.inc), which also ends at 0x101a6958. ----
    std::uint32_t event_5f() {
        std::int32_t w;
        if (!chain(w) || w == -1) return stanzas();
        auto x = draw(0x101a4ae0);
        auto idle = std::int32_t(c.u8(r25 + 4));
        auto r = rem(x, 10);
        c.set({{5, word(x)}, {6, word(idle)}, {4, word(r)}, {0, word(r + 10)}});
        compare(c, idle, r + 10);
        if (idle < r + 10) return stanzas();
        x = draw(0x101a4b44);
        idle = std::int32_t(c.u8(r25 + 4));
        r = rem(x, 5);
        c.set({{5, word(x)}, {6, word(idle)}, {4, word(r)}, {0, word(r + 0x19)}});
        compare(c, idle, r + 0x19);
        if (idle >= r + 0x19) return stanzas();
        const auto rank = c.u8(r25 + 0x76);
        c.set(0, rank);
        compare_logical(c, rank, 4);
        if (rank >= 4) return stanzas();
        x = draw(0x101a4bb4);
        idle = std::int32_t(c.u8(r25 + 0x75));
        r = rem(x, 3);
        c.set({{5, word(x)}, {6, word(idle)}, {0, word(6 - r)}});
        compare(c, idle, 6 - r);
        if (idle >= 6 - r) return stanzas();
        const auto league = c.u32(club + club::league);
        const auto table = c.u32(c.r(18));
        const auto index = c.u32(league);
        const auto stat = std::int32_t(c.u8(frame.at(0x3a)));
        const auto record = c.u32(table + (index << 2));
        c.set({{4, league}, {5, record}, {0, index << 2}, {6, word(stat)}});
        const auto sum = sbyte(c.u8(record + 0xc0)) + sbyte(c.u8(record + 0xc1));
        c.set({{4, word(sbyte(c.u8(record + 0xc0)))}, {0, word(sum)}});
        compare(c, stat, sum);
        if (stat > sum) return stanzas();
        const auto flag = c.u8(frame.at(0x38));
        c.set(0, flag);
        compare_logical(c, flag, 1);
        if (flag == 1) return stanzas();
        x = draw(0x101a4c54);
        auto word_at = c.i32(r25 + 0x1c);
        r = rem(x, 5);
        c.set({{5, word(x)}, {6, word(word_at)}, {4, word(r)}, {0, word(r + 2)}});
        compare(c, word_at, r + 2);
        if (!(word_at > r + 2)) {
            x = draw(0x101a4cb8);
            word_at = c.i32(r25 + 0x44);
            r = rem(x, 5);
            c.set({{4, word(x)}, {5, word(word_at)}, {3, word(r)}, {0, word(r + 4)}});
            compare(c, word_at, r + 4);
            if (word_at <= r + 4) return stanzas();
        }
        x = draw(0x101a4d20);
        r = rem(x, 3);
        c.set({{4, word(x)}, {0, word(r)}});
        compare(c, r);
        if (r != 0) return stanzas();
        x = draw(0x101a4d74);
        r = rem(x, 2);
        c.set({{3, word(x < 0 ? 1 : 0)}, {0, word(r)}});
        const bool by_index = compare(c, r) == 0;
        hire_post(r15, by_index, 0x101a4db0, 0x101a4ddc, 0x101a4e00, 0x5f);
        return label::wide;
    }

    // 0x101a3cb8 / 0x101a3d5c: f31 is final for the factor stages; the rest of the club's body through the row stage.
    std::uint32_t after_factor(std::uint32_t entry) {
        if (entry == 0x101a3cb8) league_boost();
        if (!reputation_scale()) {
            unflagged_penalty();
            staff_rules();
            tail_rules();
        }
        row_apply();
        power_stage();
        return after_power();
    }

#include "manager_manager_weekly_part_c.inc"
#include "manager_manager_weekly_part_d.inc"
#include "manager_manager_weekly_part_e.inc"

    // ---- The table's step, then the number it gives (the halfword at the cursor plus the base). ----
    std::int32_t draw(std::uint32_t back) {
        draw_table::step(c, t, back);
        const auto cursor = c.u32(t.cursor_cell);
        c.set(3, cursor);
        const auto x = c.u16(cursor) + c.u32(t.base_cell);
        return std::int32_t(x);
    }

    // ---- 0x101a34f0..0x101a3ad0: the club is its region's own: one time in ten the region's person
    // may be given an event (a hire) for the club, when his window is open. ----
    std::uint32_t own_events() {
        const auto x = draw(0x101a3510);
        const auto rest = x % 10;
        c.set(0, word(rest));
        if (compare(c, rest) != 0) return label::hub;
        const auto cell = c.toc(toc::object_a);
        c.set(3, c.u32(cell));
        c.set(4, c.u32(club + club::region));
        c.call(callee::region_person, 0x101a355c);
        const auto person = c.r(3);
        c.set(0, person);
        r27 = person;
        c.set(3, c.u32(c.toc(toc::object_a)));
        c.set(4, person);
        c.call(callee::person_ok, 0x101a3574);
        const auto accepted = sbyte(c.r(3));
        c.set(0, word(accepted));
        if (!nonzero(c, accepted)) return label::hub;
        const auto leagues = c.u32(c.toc(toc::leagues));
        const auto index = c.u32(person);
        c.set(3, leagues);
        const auto league_record = c.u32(leagues + (index << 2));
        c.set(0, league_record);
        if (!nonzero_logical(c, league_record)) return label::hub;
        // The window: today + 10 days against the league record's date, then today + 100 days.
        const auto date_cell = c.toc(toc::date);
        c.set({{4, date_cell}, {3, frame.at(slot::date_in)}, {5, 10}});
        c.call(callee::date_plus, 0x101a35a4);
        auto leagues_now = c.u32(c.toc(toc::leagues));
        c.set(4, leagues_now);
        const auto first = c.u32(frame.at(slot::date_in)), second = c.u32(frame.at(slot::date_in + 4));
        c.set({{6, first}, {5, second}, {3, frame.at(slot::date_out)}});
        c.put32(frame.at(slot::date_copy), first);
        c.set(4, c.u32(leagues_now + (c.u32(person) << 2)));
        c.put32(frame.at(slot::date_copy + 4), second);
        c.call(callee::date_of, 0x101a35cc);
        {
            const auto year_a = c.u16(frame.at(slot::date_copy + 2)), year_b = c.u16(frame.at(slot::date_out + 2));
            c.set({{3, year_a}, {0, year_b}});
            compare_logical(c, year_a, year_b);
            std::uint32_t result;
            if (year_a < year_b) {
                result = 1;
            } else if (year_a > year_b) {
                result = 0;
            } else {
                const auto day_a = std::int32_t(c.i16(frame.at(slot::date_copy))), day_b = std::int32_t(c.i16(frame.at(slot::date_out)));
                c.set(4, word(day_b));
                result = halved_xor_sign(day_a, day_b, day_b);
            }
            c.set(0, result);
            if (!nonzero(c, sbyte(result))) return label::hub;
        }
        c.set({{4, date_cell}, {3, frame.at(slot::date_in_b)}, {5, 100}});
        c.call(callee::date_plus, 0x101a3628);
        leagues_now = c.u32(c.toc(toc::leagues));
        const auto first_b = c.u32(frame.at(slot::date_in_b)), second_b = c.u32(frame.at(slot::date_in_b + 4));
        c.set({{6, first_b}, {5, second_b}, {3, frame.at(slot::date_out_b)}});
        c.put32(frame.at(slot::date_copy_b), first_b);
        c.set(4, c.u32(leagues_now + (c.u32(person) << 2)));
        c.put32(frame.at(slot::date_copy_b + 4), second_b);
        c.call(callee::date_of, 0x101a3650);
        {
            const auto year_a = c.u16(frame.at(slot::date_copy_b + 2)), year_b = c.u16(frame.at(slot::date_out_b + 2));
            c.set({{3, year_a}, {0, year_b}});
            compare_logical(c, year_a, year_b);
            std::uint32_t result;
            if (year_a > year_b) {
                result = 1;
            } else if (year_a < year_b) {
                result = 0;
            } else {
                const auto day_a = std::int32_t(c.i16(frame.at(slot::date_copy_b))), day_b = std::int32_t(c.i16(frame.at(slot::date_out_b)));
                c.set(4, word(day_a));
                result = halved_xor_sign(day_a, day_b, day_a);
            }
            c.set(0, result);
            if (!nonzero(c, sbyte(result))) return label::hub;
        }
        return post_window(person);
    }

    // ---- 0x101a369c..0x101a3ad0: the window is open. The staff record of the league's person for
    // this club must exist and be idle; then (hired flag clear and the staff record at +0x77 equal
    // to 1) a draw may hire for the club (event 0x5a, hired flag), else (other flag clear) the
    // second rule with event 0x44. ----
    std::uint32_t post_window(std::uint32_t person) {
        const auto leagues = c.u32(c.toc(toc::leagues));
        const auto index = c.u32(person);
        c.set(3, leagues);
        const auto league_person = c.u32(leagues + (index << 2));
        const auto staff_owner = c.u32(league_person + 8);
        c.set(3, staff_owner);
        if (!nonzero_logical(c, staff_owner)) return label::own_end;
        c.set({{3, staff_owner}, {4, club}});
        c.call(callee::staff_of, 0x101a36c0);
        r15 = c.r(3);
        if (!nonzero_logical(c, r15)) return label::own_end;
        const auto busy = c.u8(r15 + 4);
        c.set(0, busy);
        if (nonzero_logical(c, busy)) return label::own_end;
        const auto hired = sbyte(c.u8(frame.at(slot::hired_flag)));
        c.set(0, word(hired));
        c.set(3, c.u32(person));
        const auto set = c.u32(c.toc(toc::leagues));
        c.set(4, set);
        const auto holder = c.u32(set + (c.u32(person) << 2));
        c.set(3, holder);
        const auto raw = c.i16(holder + 0x36);
        c.set(0, word(raw));
        r13 = word(sbyte(word(raw)));
        const auto level = sbyte(word(raw));
        const bool hired_set = nonzero(c, hired);
        if (!hired_set) {
            const auto kind = c.u8(r15 + 0x77);
            c.set(0, kind);
            compare_logical(c, kind, 1);
            if (kind == 1) {
                if (const auto next = first_hire(level)) return next;
            }
        }
        return second_rule(level);
    }

    // 0x101a3704..0x101a383c: one more draw decides whether the person is hired for the club.
    // Returns the label the body goes on at, or 0 when the rule does not fire (the second rule follows).
    std::uint32_t first_hire(std::int32_t level) {
        auto x = draw(0x101a3724);
        const auto idle = std::int32_t(c.u8(r15 + 0x75));
        c.set(0, word(idle));
        // The limit: a fifth of the level, plus 2, less the draw modulo 3 (the original's mulhw
        // divisions leave their intermediates in r4-r8).
        const auto fifth = mulhw(0x66666667, level) >> 1;
        const auto q = fifth + (fifth < 0 ? 1 : 0);
        const auto third = mulhw(0x55555556, x);
        const auto thirds = third + (third < 0 ? 1 : 0);
        const auto rest3 = x - thirds * 3;
        const auto limit = q + 2 - rest3;
        c.set({{5, word(rest3)}, {6, third < 0 ? 1u : 0u}, {7, word(third)}, {8, word(x)}, {4, word(limit)}});
        compare(c, idle, limit);
        if (idle >= limit) return 0;
        x = draw(0x101a37a0);
        c.set(3, x < 0 ? 1u : 0u);
        const auto rest = x % 4;
        c.set(0, word(rest));
        if (compare(c, rest) == 0)
            hire(0x5a, slot::hired_flag, 0x101a37e4, 0x101a3810, 0x101a3834);
        else
            hire_any(0x5a, slot::hired_flag, 0x101a3810, 0x101a3834);
        return label::own_end;
    }

    // The hire: the game object (r17) names the club's (or any) person; then the event is posted.
    void hire(std::uint32_t code, std::int32_t flag, std::uint32_t back_a, std::uint32_t back_b, std::uint32_t back_c) {
        const auto game = c.u32(c.r(17));
        c.set({{3, game}, {4, club}});
        r15 = 0;
        c.call(callee::hire_by_club, back_a);
        const auto found = std::int32_t(c.r(3));
        compare(c, found, -1);
        if (found > -1) {
            const auto people = c.u32(c.toc(toc::people));
            r15 = people + word(found) * 0x70;
            c.set(0, word(found) * 0x70);
        }
        post(code, flag, back_c);
        (void)back_b;
    }
    void hire_any(std::uint32_t code, std::int32_t flag, std::uint32_t back_b, std::uint32_t back_c) {
        const auto game = c.u32(c.r(17));
        c.set({{3, game}, {4, club}, {5, 0}});
        c.call(callee::hire_any, back_b);
        r15 = c.r(3);
        post(code, flag, back_c);
    }
    void post(std::uint32_t code, std::int32_t flag, std::uint32_t back) {
        const auto game = c.u32(c.r(17));
        c.set({{3, game}, {4, club}, {6, r15}, {5, code}, {7, 0}, {8, 0}, {9, 0xffffffffu}});
        c.call(callee::post_event, back);
        c.set(0, 1);
        c.put8(frame.at(flag), 1);
    }

    // ---- 0x101a3840..0x101a3ad0: the second rule (event 0x44, the other flag): an unlucky club the
    // region person's level makes likely. ----
    std::uint32_t second_rule(std::int32_t level) {
        const auto stopped = sbyte(c.u8(frame.at(slot::other_flag)));
        c.set(0, word(stopped));
        if (nonzero(c, stopped)) return label::own_end;
        // r25 is the level squared over 100 (at most 163: the level is a sign-extended byte); r28
        // the bound the draw is cut by, r25 + 1.
        const auto square = std::int32_t(word(level) * word(level));
        const auto scaled = std::int32_t((std::int64_t(0x51eb851f) * square) >> 32) >> 6;
        r25 = word(scaled) + (word(scaled) >> 31);
        auto bound = std::int32_t(r25) + 1;
        r28 = word(bound);
        if (compare(c, bound) == 0) {
            r28 = 0;
        } else {
            const auto x = draw(0x101a3898);
            // The bound is within 1..164 (0x101a384c..0x101a3878), so the table's number is taken modulo the
            // bound (0x101a3990): the floating-point stage at 0x101a38c0, which needs |bound| > 0xffff, is
            // not written.
            if (bound > 0xffff || bound < -0xffff) throw std::logic_error("MANAGER_MANAGER weekly pass: the second rule's wide bound is not written (0x101a38c0)");
            compare(c, bound, 0xffff);
            compare(c, bound, -0xffff);
            const auto cursor = c.u32(t.cursor_cell);
            c.set(3, cursor);
            compare(c, bound);
            const auto sum = std::int32_t(c.u16(cursor) + c.u32(t.base_cell));
            (void)x;
            const auto rest = std::int32_t(word(sum) - word(sum / bound) * word(bound));
            c.set(0, word(rest));
            r28 = word(bound > 0 ? rest : std::int32_t(0u - word(rest)));
        }
        // 0x101a39bc: the staff record's idle byte must exceed r28 + 1 and be below a share of the level.
        const auto idle = std::int32_t(c.u8(r15 + 0x75));
        c.set(6, word(idle));
        const auto above = std::int32_t(r28) + 1;
        c.set(0, word(above));
        if (compare(c, idle, above) <= 0) return label::own_end;
        const auto share = level / 5 + level / 10 + 2;
        const auto fifth = mulhw(0x66666667, level) >> 1;
        c.set({{4, word(level / 5)}, {5, fifth < 0 ? 1u : 0u}, {3, word(share - 2)}});
        c.set(0, word(share));
        if (compare(c, idle, share) >= 0) return label::own_end;
        // 0x101a3a00: the region person's halfword at +0x18 against the club's reputation.
        const auto strength = std::int32_t(c.i16(r27 + 0x18));
        const auto reputation = std::int32_t(c.i16(club + club::reputation));
        c.set({{0, word(strength) * 0x15eu}, {3, word(reputation)}});
        if (compare(c, reputation, std::int32_t(word(strength) * 0x15eu)) <= 0) return label::own_end;
        const auto y = draw(0x101a3a34);
        c.set(3, y < 0 ? 1u : 0u);
        const auto rest = y % 4;
        c.set(0, word(rest));
        if (compare(c, rest) == 0)
            hire(0x44, slot::other_flag, 0x101a3a78, 0x101a3aa4, 0x101a3ac8);
        else
            hire_any(0x44, slot::other_flag, 0x101a3aa4, 0x101a3ac8);
        return label::own_end;
    }

    Cpu& c;
    Frame& frame;
    Fpu fpu;
    draw_table::Table t;
    std::uint32_t object, club, row_offset, pool, news;
    Kept<13> r13;
    Kept<14> r14;
    Kept<15> r15;
    Kept<25> r25;
    Kept<27> r27;
    Kept<28> r28;
};
}  // namespace

void weekly(Cpu& c, std::uint32_t object) {
    Frame frame(c, 0x101a3350, 0x150, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 31});
    // 0x101a335c..0x101a33b0: the prologue's register loads and the loop's frame slots.
    c.set(26, object);
    c.set(16, 0);
    c.set({{17, c.toc(toc::game)}, {18, c.toc(toc::leagues)}, {19, c.toc(-0x7b90)}, {20, c.toc(-0x7b8c)},
           {21, c.toc(-0x7b88)}, {22, c.toc(-0x7b84)}, {23, c.toc(toc::news)}, {24, c.toc(toc::pool)}});
    c.set(31, c.u32(c.toc(toc::clubs)));
    c.put8(frame.at(slot::hired_flag), 0);
    c.put8(frame.at(slot::other_flag), 0);
    c.put32(frame.at(slot::club_pointer), 0);
    c.put32(frame.at(slot::club_index), 0);
    // 0x101a8a98..0x101a8aa8: the loop test, the counter at sp+0xe0 against the club count (read again each time).
    const auto test = [&] {
        const auto cell = c.toc(toc::club_count);
        c.set(3, cell);
        const auto counter = std::int32_t(c.u32(frame.at(slot::club_index)));
        c.set(0, word(counter));
        const auto total = std::int32_t(c.u32(cell));
        c.set(3, word(total));
        compare(c, counter, total);
        return counter < total;
    };
    while (test()) {
        {
            Club club(c, frame, object);
            club.body();
        }
        // 0x101a8a78: on to the next club (r16 the row offset, r31 the record, sp+0xec the record pointer).
        c.set(3, c.u32(frame.at(slot::club_pointer)));
        c.set(16, c.r(16) + 0x4c);
        c.set(31, c.r(31) + 0x248);
        c.set(3, c.r(3) + 0x248);
        c.put32(frame.at(slot::club_pointer), c.r(3));
        c.set(3, c.u32(frame.at(slot::club_index)));
        c.set(3, c.r(3) + 1);
        c.put32(frame.at(slot::club_index), c.r(3));
    }
}
}
