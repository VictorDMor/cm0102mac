// MANAGER_MANAGER.CPP manager review block (manager_manager_review.hpp): 0x101a8ad0, the board's
// review of a club's manager; 0x101c1690, the pending-departure check it makes first; 0x10195e10,
// the module's draw from the halfword table.
//
// Every routine keeps the original's stack image: the prologue stores, the int-to-double words
// (0x43300000 and value ^ 0x80000000), the eight bytes of each fctiwz + stfd, the record
// built for a list append, and the registers the original leaves in r3-r10 and cr0 (the callers'
// next varargs call stores r3-r10 into its frame).
#include "manager_manager_review.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "manager_manager_assessment.hpp"
#include "manager_manager_detail.hpp"
#include "manager_manager_review_detail.hpp"
#include <bit>

namespace cm::aot::manager_manager {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;
using guest::Kept;
using guest::compare;
using guest::compare_logical;
using namespace review_detail;
namespace {
namespace callee {
constexpr std::uint32_t calendar = 0x10718130,      // days between two dates (r3, r4, r5)
    redraw = 0x1077b770,                            // the base for the draw table
    list_next = 0x10738e00, list_remove = 0x10738e60, list_append = 0x10738d10,
    league_results = 0x10459b50,                    // the club's results record
    manager_flag = 0x102de3c0, club_flag = 0x100c7d00, club_state = 0x1014c2e0,
    pressure = 0x101c1830, post_kind = 0x101a9e00, offer_value = 0x101abda0, outcome = 0x101b2a30,
    virtual_glue = 0x10781320;
}
namespace toc {
constexpr std::int32_t pool = -0x536c, date = -0x7b80, leagues = -0x7b5c, scale_owner = -0x7b28, stamp = -0x7a9c;
}

std::uint32_t extended_byte(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
// The compiler's mulhw: the high word of the signed product.
std::int32_t mulhw(std::uint32_t a, std::int32_t b) { return std::int32_t((std::int64_t(std::int32_t(a)) * b) >> 32); }
// `divw` as the generated code does it: the quotient of the sign-extended words, truncated.
std::int32_t divide(std::int32_t a, std::int32_t b) { return std::int32_t(std::int64_t(a) / b); }
// The remainder after a divw, mullw and subf (the divisor is not zero where it is used).
std::int32_t remainder(std::int32_t a, std::int32_t b) {
    return std::int32_t(std::uint32_t(a) - std::uint32_t(divide(a, b)) * std::uint32_t(b));
}
// A draw's remainder step: the result takes the divisor's sign, as `neg` and the conditional move do.
std::int32_t signed_remainder(std::int32_t x, std::int32_t bound) {
    const auto rest = remainder(x, bound);
    return bound > 0 ? rest : std::int32_t(0u - std::uint32_t(rest));
}
// The compiler's divisions by constants (mulhw, shift, add of the sign bit) are the truncating
// division for every 32-bit input (checked over all 2^32 values), and the sign bit it keeps is
// the dividend's.
std::uint32_t sign_of(std::int32_t value) { return value < 0 ? 1 : 0; }

// The module's draw table: the cell with its base (a value redrawn at each wrap), its first entry,
// the cell with its last entry and the cell with the cursor (a pointer to the next halfword).
struct Table {
    std::uint32_t base_cell, first, last_cell, cursor_cell;
};
Table table_of(const Cpu& c) {
    return {c.toc(table_slots::base), c.toc(table_slots::first), c.toc(table_slots::last), c.toc(table_slots::cursor)};
}
// The cursor advances by two halfwords; past the last entry it returns to the first and the base
// is redrawn (0x1077b770 answers it). `back` is the address after the original's `bl`. The caller
// passes the cursor it holds (r3 in the original).
void step_from(Cpu& c, const Table& t, std::uint32_t cursor, std::uint32_t back) {
    const auto last = c.u32(t.last_cell);
    cursor += 2;
    c.set(3, cursor);
    c.put32(t.cursor_cell, cursor);
    compare_logical(c, cursor, last);
    if (cursor > last) {
        c.put32(t.cursor_cell, t.first);
        c.call(callee::redraw, back);
        c.put32(t.base_cell, c.r(3) & 0xffff);
    }
}
void step(Cpu& c, const Table& t, std::uint32_t back) { step_from(c, t, c.u32(t.cursor_cell), back); }
// The table's current number: the halfword at the cursor plus the base (left in r3).
std::uint32_t table_value(Cpu& c, const Table& t) {
    const auto cursor = c.u32(t.cursor_cell);
    c.set(3, cursor);
    const auto base = c.u32(t.base_cell);
    const auto x = std::uint32_t(c.u16(cursor)) + base;
    c.set(3, x);
    return x;
}
// The draw's small-bound path (|bound| <= 0xffff, bound not 0): the table's number modulo the bound.
std::int32_t small_bound(Cpu& c, const Table& t, std::int32_t bound) {
    const auto cursor = c.u32(t.cursor_cell);
    c.set(3, cursor);
    compare(c, bound);  // cmpwi bound,0
    const auto base = c.u32(t.base_cell);
    const auto x = std::uint32_t(c.u16(cursor)) + base;
    c.set(3, x);
    return signed_remainder(std::int32_t(x), bound);
}

// The frame slots of the draw's floating-point stage: the converted bound, the remainder's conversion
// and the fctiwz result (offsets of the high words from the frame base).
constexpr std::uint32_t stage_in = 0x48, stage_rest = 0x40, stage_out = 0x38;
struct Stage {
    std::int32_t next;     // bound * 10 / 0xffff (the product wraps to 32 bits)
    std::uint32_t part;    // the stage's own share: bound / unit * (number % 0xffff), truncated
};
// The draw's floating-point stage for a bound beyond +-0xffff: the bound as a double over the unit
// (pool +0x40) times the table's number modulo 0xffff, truncated by fctiwz. The original keeps the
// cursor and the sign bits of its divisions in r3-r9.
Stage float_stage(Cpu& c, Fpu& fpu, const Frame& frame, const Table& t, std::int32_t bound) {
    const auto pool = c.r(29);
    c.set(9, conversion_high);
    c.put32(frame.at(stage_in + 4), std::uint32_t(bound) ^ 0x80000000u);
    const auto cursor = c.u32(t.cursor_cell);
    c.set(3, cursor);
    c.put32(frame.at(stage_in), conversion_high);
    const auto bias = c.f64(pool + pool::bias);
    const auto tenfold = std::int32_t(std::uint32_t(bound) * 10u);
    auto ratio = c.f64(frame.at(stage_in));
    const auto unit = c.f64(pool + pool::unit);
    ratio = fpu.fsub(ratio, bias);
    const auto halfword = c.u16(cursor);
    const auto base = c.u32(t.base_cell);
    ratio = fpu.fdiv(ratio, unit);
    c.set_f(1, ratio);
    c.put32(frame.at(stage_rest), conversion_high);
    const auto x = std::int32_t(std::uint32_t(halfword) + base);
    const auto next = tenfold / 0xffff;
    const auto whole = std::uint32_t(x / 0xffff) * 0xffffu;
    const auto rest = std::uint32_t(x) - whole;
    c.set({{4, sign_of(tenfold)}, {5, whole}, {6, sign_of(x)}, {7, std::uint32_t(x)}, {8, halfword}});
    compare(c, next);  // add. r22,r0,r4
    c.put32(frame.at(stage_rest + 4), rest ^ 0x80000000u);
    auto share = c.f64(frame.at(stage_rest));
    share = fpu.fsub(share, bias);
    share = fpu.fmul(ratio, share);
    c.put64(frame.at(stage_out), fpu.fctiwz(0, share));
    return {next, c.u32(frame.at(stage_out + 4))};
}

}  // namespace

// 0x101c1690. Frame 0x50 (r28-r31 saved one by one, LR at +8). The pending list is walked with
// the list's own cursor; the entry for the club is removed whether or not it posts.
bool departure(Cpu& c, std::uint32_t object, std::uint32_t club) {
    c.set(0, c.lr());
    Frame frame(c, 0x101c1690, 0x50, {.first_gpr = 28});
    guest::keep(c, {{31, club}, {30, object}, {29, c.toc(toc::date)}});
    const auto date = c.r(29);
    auto list = c.u32(object + object::pending);
    c.set(3, list);
    const auto count = c.i16(list);
    compare(c, count);  // extsh. r0,r0
    if (count == 0) return guest::result(c, 0);
    c.put32(list + 0xc, c.u32(list + 0x4));  // the cursor to the first node
    Kept<28> index(c, 0);
    for (;; index = std::uint32_t(std::int32_t(index) + 1)) {
        list = c.u32(object + object::pending);
        c.set(3, list);
        const auto limit = c.i16(list);
        compare(c, std::int32_t(index), limit);
        if (std::int32_t(index) >= limit) return guest::result(c, 0);
        c.call(callee::list_next, 0x101c16e4);
        const auto entry = c.r(3);
        c.set(4, entry);
        compare(c, std::int32_t(entry));  // or. r4,r3,r3
        if (!entry) continue;
        c.set(3, c.u32(club));
        const auto wanted = std::int32_t(c.u32(club));
        const auto listed = std::int32_t(c.u32(entry));
        compare(c, wanted, listed);
        if (wanted != listed) continue;
        c.set(3, c.u32(object + object::pending));
        c.call(callee::list_remove, 0x101c1704);
        const auto manager = c.u32(club + club::manager);
        c.set(5, manager);
        compare_logical(c, manager);
        if (!manager) continue;
        c.set(4, c.u32(manager + manager::since_a));
        c.set(3, date);
        c.set(5, c.u32(manager + manager::since_b));
        c.call(callee::calendar, 0x101c1720);
        const auto days = std::int32_t(c.r(3));
        compare(c, days, 100);
        if (days <= 100) continue;
        // Past 100 days: the record {1, kind, club id} at frame+0x38 (kind 2, or 7 when the
        // results record's signal is not positive).
        const auto id = c.u32(club);
        c.set(3, id);
        c.put32(frame.at(0x3c), id);
        c.put8(frame.at(0x38), 1);
        compare_logical(c, club);
        std::uint32_t kind = 2;
        if (club) {
            const auto league_record = c.u32(club + club::league);
            c.set(4, league_record);
            compare_logical(c, league_record);
            if (league_record) {
                const auto leagues = c.toc(toc::leagues);
                c.set(3, leagues);
                const auto slot = c.u32(league_record) << 2;
                const auto league_table = c.u32(leagues);
                c.set(3, league_table);
                const auto league_entry = c.u32(league_table + slot);
                c.set(3, league_entry);
                compare_logical(c, league_entry);
                if (league_entry) {
                    const auto owner = c.u32(league_entry + league::owner);
                    c.set(3, owner);
                    compare_logical(c, owner);
                    if (owner) {
                        c.set(4, club);
                        c.call(callee::league_results, 0x101c177c);
                        const auto results_record = c.r(3);
                        compare_logical(c, results_record);
                        kind = 7;
                        if (results_record) {
                            const auto signal = c.i32(results_record + results::signal_3);
                            compare(c, signal);
                            if (signal > 0) kind = 2;
                        }
                    }
                }
            }
        }
        c.put8(frame.at(0x39), kind);
        c.set(3, c.u32(object + object::departures));
        c.set(4, frame.at(0x38));
        c.set(5, 8);
        c.call(callee::list_append, 0x101c17c0);
        const auto at = c.u32(club) * row::size + row::flags;
        const auto table = c.u32(object + object::table);
        c.set({{6, 1}, {5, table}, {3, 1}, {4, at}});
        c.put8(table + at, c.u8(table + at) | row::posted);
        return true;
    }
}

// 0x10195e10. Frame 0x80 (stmw r20-r31; the original keeps the bound in r30, the stage's results in
// r22 (next bound) and r31 (share), and a second stage's in r21/r24). A small bound (|b| <= 0xffff)
// takes the table's number modulo the bound. A larger one splits into a share through floating
// point (above) and the next bound, bound * 10 / 0xffff, which draws again: the original unrolls
// four such stages and a recursive call, but the product wraps to 32 bits, so the next bound is
// within +-32768 and the second stage always takes the small path: its own next stages never run.
std::int32_t draw(Cpu& c, std::int32_t bound) {
    c.set(0, c.lr());
    Frame frame(c, 0x10195e10, 0x80, {.first_gpr = 20});
    const auto table = table_of(c);
    guest::keep(c, {{25, table.base_cell}, {26, table.first}, {27, table.last_cell}, {28, table.cursor_cell},
                    {29, c.toc(toc::pool)}, {30, std::uint32_t(bound)}});
    compare(c, bound);  // or. r30,r3,r3
    std::int32_t result = 0;
    if (bound == 0) {
        c.set(3, 0);
    } else {
        step(c, table, 0x10195e64);
        if (bound <= 0xffff && bound >= -0xffff) {
            result = small_bound(c, table, bound);
            c.set(3, std::uint32_t(result));
        } else {
            Fpu fpu(c);
            const auto stage = float_stage(c, fpu, frame, table, bound);
            c.set(22, std::uint32_t(stage.next));
            c.set(31, stage.part);
            std::int32_t inner = 0;
            if (stage.next == 0) {
                c.set(24, 0);
            } else {
                step_from(c, table, c.r(3), 0x10195f50);
                inner = small_bound(c, table, stage.next);
                c.set(24, std::uint32_t(inner));
            }
            // The share plus a tenth (rounded) of the second draw, never above bound - 1.
            const auto shifted = std::int32_t(std::uint32_t(inner) + 5u);
            const auto limit = std::int32_t(std::uint32_t(bound) - 1u);
            auto total = std::int32_t(stage.part + std::uint32_t(shifted / 10));
            c.set(4, sign_of(shifted));
            c.set(31, std::uint32_t(total));
            compare(c, total, limit);
            if (total > limit) {
                total = limit;
                c.set(31, std::uint32_t(total));
            }
            result = total;
            c.set(3, std::uint32_t(total));
        }
    }
    lmw_leaves(c, frame.caller());
    return result;
}

namespace {
// 0x101a8ad0. Frame 0xa0 (stmw r19-r31). The original keeps the object in r30, the club in r31,
// the date in r24, the draw table's cells in r25-r28, the pool in r29 and the scale owner's cell
// in r23; r21 starts as the leagues table's cell and is reused for bytes, r19 holds the review's
// threshold, r20 a draw, r22 the club's results record or a byte. The draws the original inlines
// always have |bound| <= 6400 (a sign-extended byte, times 50 or plus 1 at most), so they take
// draw's remainder path and never its floating-point stages.
class Review {
public:
    Review(Cpu& c, Frame& frame, std::uint32_t object, std::uint32_t club)
        : c(c), frame(frame), fpu(c), table(table_of(c)), object(object), club(club), date(c.toc(toc::date)),
          pool(c.toc(toc::pool)), threshold(c, c.r(19)), roll(c, c.r(20)), aux(c, c.toc(toc::leagues)),
          subject(c, c.r(22)) {
        c.set(6, c.toc(toc::stamp));
        guest::keep(c, {{30, object}, {31, club}, {24, date}, {23, c.toc(toc::scale_owner)}, {27, table.base_cell},
                        {26, table.first}, {25, table.last_cell}, {28, table.cursor_cell}, {29, pool}});
    }

    void run(std::uint32_t flag) {
        if (!date_gate() || !flag_gate()) return;
        compare(c, std::int8_t(flag));  // extsb. r0,r5
        if (std::int8_t(flag) != 0) {
            c.set({{3, object}, {4, club}});
            c.link(0x101a8ba0);
            departure(c, object, club);
            const auto posted = std::int8_t(c.r(3));
            compare(c, posted);
            if (posted != 0) return;
        }
        const auto value = c.i16(row_of() + row::value);
        compare(c, value, 0x10b3);
        if (value > 0x10b3) return;
        const auto days = tenure(0x101a8bd8);
        compare(c, days, 0x96);
        if (days < 0x96) return;
        if (settled_manager()) return;
        pick_threshold();
        if (form_gates()) return;
        scale_for_manager_flag();
        if (second_person_draw() || third_draw() || board_pressure()) return;
        verdict();
    }

private:
    // The row of the object's table for the club (id * 0x4c from its base), left in r3.
    std::uint32_t row_of() {
        const auto id = c.u32(club + club::id);
        const auto row = c.u32(object + object::table) + id * row::size;
        c.set(3, row);
        return row;
    }
    // The league object of a club's league record: the leagues table (cell r21) indexed by its first word.
    std::uint32_t league_entry(std::uint32_t league_record) { return c.u32(c.u32(aux) + (c.u32(league_record) << 2)); }
    std::uint32_t manager_of() { return c.u32(club + club::manager); }
    // The second person's staff record (0 when the person or the record is missing), as r3.
    std::uint32_t staff_of_second() {
        const auto second = c.u32(club + club::second_person);
        c.set(3, second);
        compare_logical(c, second);
        if (!second) return 0;
        const auto record = c.u32(second + manager::staff);
        c.set(3, record);
        compare_logical(c, record);
        return record;
    }

    // The date the stamp holds (a copy of two words at frame+0x38) is compared with the game date:
    // the halfword at +2 as unsigned, equal ones then the signed halfword at +0. The review runs
    // while the copy is not after the date.
    bool date_gate() {
        const auto stamp = c.toc(toc::stamp);
        const auto first = c.u32(stamp), second = c.u32(stamp + 4);
        c.put32(frame.at(0x38), first);
        c.set(3, c.u16(date + 2));
        c.set(4, c.u16(frame.at(0x3a)));
        c.put32(frame.at(0x3c), second);
        compare_logical(c, c.r(4), c.r(3));
        std::uint32_t later;
        if (c.r(4) > c.r(3)) {
            later = 1;
        } else if (c.r(4) < c.r(3)) {
            later = 0;
        } else {
            const auto mine = guest::extended(c.i16(frame.at(0x38)));
            c.set(4, mine);
            const auto theirs = guest::extended(c.i16(date));
            const auto differing = mine ^ theirs;
            c.set(3, std::uint32_t(std::int32_t(differing) >> 1));
            // (a ^ b) >> 1 less (a ^ b) & a, its sign bit: the compiler's signed halfword compare.
            later = (c.r(3) - (differing & mine)) >> 31;
        }
        compare(c, std::int8_t(later));
        return later == 0;
    }
    // A club whose row carries the posted bit (0x40) is skipped.
    bool flag_gate() {
        const auto id = c.u32(club + club::id);
        const auto table_base = c.u32(object + object::table);
        c.set(4, table_base);
        c.set(3, id * row::size);
        const std::int8_t flagged = (c.u8(table_base + id * row::size + row::flags) & row::posted) ? -1 : 0;
        compare(c, flagged);
        return flagged == 0;
    }
    // Days since the manager's appointment (CALENDAR).
    std::int32_t tenure(std::uint32_t back) {
        const auto manager = manager_of();
        c.set(5, manager);
        c.set(3, date);
        c.set(4, c.u32(manager + manager::since_a));
        c.set(5, c.u32(manager + manager::since_b));
        c.call(callee::calendar, back);
        return std::int32_t(c.r(3));
    }
    // Days since the two dates in the club's row.
    std::int32_t days_since_row_date(std::uint32_t back) {
        const auto id = c.u32(club + club::id);
        c.set(3, date);
        const auto row = c.u32(object + object::table) + id * row::size;
        c.set(5, row);
        c.set(4, c.u32(row + row::date_a));
        c.set(5, c.u32(row + row::date_b));
        c.call(callee::calendar, back);
        return std::int32_t(c.r(3));
    }
    void cap_row_value() {
        const auto id = c.u32(club + club::id);
        c.set(4, 0x10b3);
        const auto row = c.u32(object + object::table) + id * row::size;
        c.set(3, row);
        c.put16(row + row::value, 0x10b3);
    }
    // x = the table's number; its remainder by `divisor` as the compiler's mulhw sequence takes it
    // (r5 the cursor, r4 the number, r3 the quotient's sign bit, or the high word undivided when
    // `shift` is 0). The record form (subf.) compares the remainder with zero.
    std::int32_t table_remainder(std::int32_t divisor, std::uint32_t magic, unsigned shift) {
        const auto cursor = c.u32(table.cursor_cell);
        c.set(5, cursor);
        const auto x = std::int32_t(table_value(c, table));
        c.set(4, std::uint32_t(x));
        c.set(3, shift == 0 ? std::uint32_t(mulhw(magic, x)) : sign_of(x));
        const auto rest = remainder(x, divisor);
        compare(c, rest);
        return rest;
    }
    // A draw the original inlines: the table's step, then the remainder of the table's number by the
    // bound (never 0).
    std::int32_t inline_draw(std::int32_t bound, std::uint32_t back) {
        step(c, table, back);
        return small_bound(c, table, bound);
    }

    // 0x101a8be0..0x101a8e3c. A manager the club's reputation has outgrown (morale above 5000
    // and above the reputation less 750, under 400 days in post) puts the row's value at its
    // cap when the second person's draw comes up non-zero, or, without that person, one time in
    // ten. True: the review ends.
    bool settled_manager() {
        const auto manager = manager_of();
        c.set(5, manager);
        compare_logical(c, manager);
        if (!manager) return false;
        const auto staff_record = c.u32(manager + manager::staff);
        c.set(3, staff_record);
        compare_logical(c, staff_record);
        if (!staff_record) return false;
        const auto morale = c.i16(staff_record + staff::morale);
        c.set(4, guest::extended(morale));
        compare(c, morale, 0x1388);
        if (morale <= 0x1388) return false;
        const auto reputation = c.i16(club + club::reputation);
        c.set(3, guest::extended(reputation));
        const auto limit = std::int16_t(reputation - 0x2ee);
        compare(c, limit, morale);
        if (limit >= morale) return false;
        c.set(4, c.u32(manager + manager::since_a));
        c.set(3, date);
        c.set(5, c.u32(manager + manager::since_b));
        c.call(callee::calendar, 0x101a8c28);
        const auto days = std::int32_t(c.r(3));
        compare(c, days, 0x190);
        if (days >= 0x190) return false;
        const auto second_staff = staff_of_second();
        if (second_staff) {
            subject = c.u8(second_staff + staff::trait);
            const auto bound = std::int32_t(std::int8_t(std::uint32_t(subject)));
            subject = std::uint32_t(bound);
            compare(c, bound);
            roll = bound == 0 ? 0 : std::uint32_t(inline_draw(bound, 0x101a8c7c));
            compare(c, std::int32_t(std::uint32_t(roll)));
            if (std::uint32_t(roll) == 0) return false;
        } else {
            step(c, table, 0x101a8de8);
            if (table_remainder(10, 0x66666667u, 2) == 0) return false;
        }
        cap_row_value();
        return true;
    }

    // 0x101a8e40: the threshold by the row's value (above 3500: 10; above 3000: 200; else 500).
    void pick_threshold() {
        const auto value = c.i16(row_of() + row::value);
        compare(c, value, 0xdac);
        if (value > 0xdac) {
            threshold = 10;
        } else {
            compare(c, value, 0xbb8);
            threshold = value > 0xbb8 ? 200 : 500;
        }
    }

    // 0x101a8e78..0x101a9198: the results record of the club's league (0x10459b50) bounds the
    // games the form may count; the last five results are tallied (3 scores 3, 2 and 1 score 1 each,
    // -3 only counts) and their average must not pass the pool's limit; the record's signals then
    // adjust the threshold. Without a record none of it runs. True: the review ends.
    bool form_gates() {
        const auto league_record = c.u32(club + club::league);
        c.set(3, league_record);
        compare_logical(c, league_record);
        if (!league_record) return false;
        auto entry = league_entry(league_record);
        c.set(3, entry);
        compare_logical(c, entry);
        if (!entry) return false;
        const auto owner = c.u32(entry + league::owner);
        c.set(3, owner);
        compare_logical(c, owner);
        if (!owner) return false;
        c.set(4, club);
        c.call(callee::league_results, 0x101a8eb0);
        subject = c.r(3);
        compare(c, std::int32_t(c.r(3)));
        if (c.r(3) == 0) return false;
        const auto results_record = std::uint32_t(subject);
        const auto second_staff = staff_of_second();
        if (second_staff) {
            const auto trait = std::int8_t(c.u8(second_staff + staff::trait));
            roll = extended_byte(std::uint32_t(std::int32_t(trait) >> 1));
        } else {
            roll = 5;
        }
        const auto allowance = [&] { return std::int32_t(0x1e - std::int8_t(std::uint32_t(roll))); };
        const auto league_now = c.u32(club + club::league);
        c.set(3, league_now);
        const auto leagues_base = c.u32(aux);
        c.set(4, leagues_base);
        entry = league_entry(league_now);
        c.set(3, entry);
        const auto high_bound = c.u8(entry + league::form_high);
        c.set(5, high_bound);
        compare(c, std::int8_t(high_bound));  // extsb. r0,r5
        if (std::int8_t(high_bound) != 0) {
            const auto bound = c.u8(results_record + results::bound);
            c.set(4, bound);
            compare(c, std::int32_t(bound), std::int32_t(std::int8_t(high_bound)));
            if (std::int32_t(bound) <= std::int32_t(std::int8_t(high_bound))) {
                const auto games = c.u8(results_record + results::count);
                c.set(4, games);
                compare(c, std::int32_t(games), allowance());
                if (std::int32_t(games) > allowance()) return true;
            }
        }
        // The league object's virtual method at +0xa0 (through the pointer glue).
        c.set(12, c.u32(c.u32(entry) + league::vtable_method));
        c.call(callee::virtual_glue, 0x101a8f38);
        c.set(2, c.u32(frame.at(0x14)));
        compare(c, std::int8_t(c.r(3)));
        if (std::int8_t(c.r(3)) != 0) {
            const auto league_again = c.u32(club + club::league);
            c.set(3, league_again);
            const auto leagues_again = c.u32(aux);
            c.set(4, leagues_again);
            const auto bound = c.u8(results_record + results::bound);
            c.set(5, bound);
            entry = league_entry(league_again);
            c.set(3, entry);
            const auto low_bound = std::int32_t(std::int8_t(c.u8(entry + league::form_low)));
            compare(c, std::int32_t(bound), low_bound);
            if (std::int32_t(bound) <= low_bound) {
                const auto games = c.u8(results_record + results::count);
                c.set(3, games);
                compare(c, std::int32_t(games), allowance());
                if (std::int32_t(games) > allowance()) return true;
            }
        }
        std::int32_t points = 0, played = 0, index = 4;
        c.set({{3, 0}, {4, 0}, {5, 4}});
        for (int round = 0; round < 5; ++round, --index) {
            const auto result = std::int8_t(c.u8(results_record + results::last_games + std::uint32_t(index)));
            if (result == 3) {
                points = std::int16_t(points + 3);
                ++played;
            } else if (result == 2 || result == 1) {
                points = std::int16_t(points + 1);
                ++played;
            } else if (result == -3) {
                ++played;
            }
            c.set({{3, std::uint32_t(points)}, {4, std::uint32_t(played)}, {5, std::uint32_t(index - 1)}});
        }
        compare(c, std::int16_t(played));  // extsh. r0,r4
        if (std::int16_t(played) > 0) {
            const auto average = divide(std::int16_t(points), std::int16_t(played));
            c.set(3, std::uint32_t(average));
            c.put32(frame.at(0x48), conversion_high);
            const auto bias = c.f64(pool + pool::bias);
            const auto limit = c.f64(pool + pool::average_limit);
            c.put32(frame.at(0x4c), std::uint32_t(average) ^ 0x80000000u);
            auto shown = c.f64(frame.at(0x48));
            shown = fpu.fsubs(shown, bias);  // single precision, as the original
            c.set_f(1, shown);
            fpu.fcmp(0, shown, limit);
            if (c.cr(0) & 4) return true;  // bgt
        }
        return signals_end(results_record);
    }
    // The converted word (value ^ 0x80000000, the caller sets r3 where the original does) over the
    // stack double at 0x48, times the pool factor, truncated by fctiwz into the slot at 0x50;
    // the low word. Leaves f1 the bias.
    std::uint32_t scale_through_frame(std::uint32_t value, std::uint32_t factor) {
        c.put32(frame.at(0x4c), value ^ 0x80000000u);
        c.put32(frame.at(0x48), conversion_high);
        const auto bias = c.f64(pool + pool::bias);
        c.set_f(1, bias);
        const auto scale = c.f64(pool + factor);
        auto scaled = c.f64(frame.at(0x48));
        scaled = fpu.fsub(scaled, bias);
        scaled = fpu.fmul(scale, scaled);
        c.put64(frame.at(0x50), fpu.fctiwz(0, scaled));
        return c.u32(frame.at(0x54));
    }
    // The threshold scaled by a pool factor.
    void scale_threshold(std::uint32_t factor) {
        const auto current = guest::extended(current_threshold());
        c.set(3, current ^ 0x80000000u);
        threshold = scale_through_frame(current, factor);
    }
    std::int16_t current_threshold() { return std::int16_t(std::uint32_t(threshold)); }
    // 0x101a903c..0x101a9198: the first of the record's four signals above 1 decides how the
    // threshold changes; at a threshold of 10 the first two end the review. True: it ends.
    bool signals_end(std::uint32_t record) {
        const auto first = c.i32(record + results::signal_1);
        c.set(3, std::uint32_t(first));
        compare(c, first, 1);
        if (first > 1) {
            const auto current = current_threshold();
            compare(c, current, 0xa);
            if (current == 0xa) return true;
            compare(c, first, 2);
            if (first > 2) return true;
            compare(c, current, 0xc8);
            if (current == 0xc8) scale_threshold(pool::tier_a);
            return false;
        }
        const auto second = c.i32(record + results::signal_2);
        c.set(3, std::uint32_t(second));
        compare(c, second, 1);
        if (second > 1) {
            const auto current = current_threshold();
            compare(c, current, 0xa);
            if (current == 0xa) return true;
            compare(c, second, 3);
            if (second > 3) return true;
            compare(c, current, 0xc8);
            if (current == 0xc8) scale_threshold(pool::tier_b);
            return false;
        }
        const auto third = c.i32(record + results::signal_3);
        compare(c, third, 1);
        if (third > 1) {
            scale_threshold(pool::tier_c);
            return false;
        }
        const auto fourth = c.i32(record + results::signal_4);
        compare(c, fourth, 1);
        if (fourth > 1) {
            const auto current = current_threshold();
            compare(c, current, 0xa);
            if (current == 0xa) {
                threshold = 0xc8;
            } else {
                compare(c, current, 0xc8);
                if (current == 0xc8) scale_threshold(pool::tier_c);
            }
        }
        return false;
    }

    // 0x101a9198: the manager flag (0x102de3c0) scales the threshold once more.
    void scale_for_manager_flag() {
        c.set(3, c.u32(c.r(23)));
        c.set(4, manager_of());
        c.call(callee::manager_flag, 0x101a91a4);
        compare(c, std::int8_t(c.r(3)));
        if (std::int8_t(c.r(3)) != 0) scale_threshold(pool::tier_a);
    }

    // 0x101a91e0..0x101a93cc: the second person's trait (times 50) as a draw bound; the draw must not
    // pass the threshold. Without that person one draw of the table modulo 500 does the same.
    // True: the review ends.
    bool second_person_draw() {
        if (const auto second_staff = staff_of_second()) {
            const auto trait = std::int32_t(std::int8_t(c.u8(second_staff + staff::trait)));
            const auto bound = std::int32_t(std::int16_t(trait * 0x32));
            subject = guest::extended(std::int16_t(bound));
            compare(c, bound);
            roll = bound == 0 ? 0 : std::uint32_t(inline_draw(bound, 0x101a9234));
            const auto limit = std::int32_t(guest::extended(current_threshold()));
            compare(c, std::int32_t(std::uint32_t(roll)), limit);
            return std::int32_t(std::uint32_t(roll)) > limit;
        }
        step(c, table, 0x101a938c);
        const auto cursor = c.u32(table.cursor_cell);
        c.set(5, cursor);
        const auto base = c.u32(table.base_cell);
        c.set(4, base);
        c.set(3, 0x10624dd3u);
        const auto x = std::int32_t(std::uint32_t(c.u16(cursor)) + base);
        c.set(5, std::uint32_t(x));
        const auto limit = std::int32_t(guest::extended(current_threshold()));
        const auto rest = remainder(x, 500);
        c.set(4, sign_of(x));
        c.set(3, std::uint32_t(rest));
        compare(c, rest, limit);
        return rest > limit;
    }

    // 0x101a93d0..0x101a95d4: with the second person, the trait plus 1 as a draw bound (without
    // them one in 11 of the table); a zero draw asks 0x101c1830, which can end the review.
    bool third_draw() {
        bool asks;
        if (const auto second_staff = staff_of_second()) {
            const auto value = extended_byte(c.u8(second_staff + staff::trait));
            c.set(3, value);
            const auto bound = std::int32_t(value + 1);
            aux = std::uint32_t(bound);
            compare(c, bound);
            roll = bound == 0 ? 0 : std::uint32_t(inline_draw(bound, 0x101a9420));
            compare(c, std::int32_t(std::uint32_t(roll)));
            asks = std::uint32_t(roll) == 0;
            if (asks) {
                c.set({{3, object}, {4, club}});
                c.call(callee::pressure, 0x101a955c);
            }
        } else {
            step(c, table, 0x101a9588);
            asks = table_remainder(11, 0x2e8ba2e9u, 1) == 0;
            if (asks) {
                c.set({{3, object}, {4, club}});
                c.call(callee::pressure, 0x101a95d0);
            }
        }
        if (!asks) return false;
        compare(c, std::int8_t(c.r(3)));
        return std::int8_t(c.r(3)) != 0;
    }

    // 0x101a95d8..0x101a981c: for a club with a manager that the club state (0x1014c2e0) and the
    // manager flag let through, a zero draw of the manager's trait and a table number divisible by 4
    // post a departure of kind 2. True: the review ends.
    bool board_pressure() {
        const auto manager = c.u32(club + club::manager);
        compare_logical(c, manager);
        if (!manager) return false;
        c.set({{3, club}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
        c.call(callee::club_state, 0x101a95fc);
        compare(c, std::int8_t(c.r(3)));
        if (std::int8_t(c.r(3)) != 0) return false;
        c.set(3, c.u32(c.r(23)));
        c.set(4, manager_of());
        c.call(callee::manager_flag, 0x101a9610);
        compare(c, std::int8_t(c.r(3)));
        if (std::int8_t(c.r(3)) != 0) return false;
        const auto person = manager_of();
        c.set(3, person);
        aux = c.u8(person + manager::trait);
        const auto bound = std::int32_t(std::int8_t(std::uint32_t(aux)));
        aux = std::uint32_t(bound);
        compare(c, bound);
        roll = bound == 0 ? 0 : std::uint32_t(inline_draw(bound, 0x101a9650));
        compare(c, std::int32_t(std::uint32_t(roll)));
        if (std::uint32_t(roll) != 0) return false;
        step(c, table, 0x101a97a0);
        const auto x = table_value(c, table);
        // x % 4 as the compiler's rotate sequence takes it (the sign bit enters twice).
        const auto sign = x >> 31;
        c.set(3, sign);
        const auto residue = std::rotl(((x & 3u) << 30) - sign, 2) + sign;
        compare(c, std::int32_t(residue));
        if (residue != 0) return false;
        post_record(2, 0x101a97e8, 0x101a97fc);
        return true;
    }

    // The record {kind, 0x101a9e00's byte, club id} (8 bytes at frame+0x40) appended to the object's
    // departures, and the row marked as posted.
    void post_record(std::uint8_t kind, std::uint32_t back_kind, std::uint32_t back_append) {
        const auto id = c.u32(club + club::id);
        c.set(4, id);
        c.set(3, object);
        c.put32(frame.at(0x44), id);
        c.put8(frame.at(0x40), kind);
        c.call(callee::post_kind, back_kind);
        c.put8(frame.at(0x41), c.r(3));
        c.set(4, frame.at(0x40));
        c.set(5, 8);
        c.set(3, c.u32(object + object::departures));
        c.call(callee::list_append, back_append);
        const auto at = c.u32(club + club::id) * row::size + row::flags;
        c.set(5, 1);
        const auto table_base = c.u32(object + object::table);
        c.set(4, table_base);
        c.set(3, at);
        c.put8(table_base + at, c.u8(table_base + at) | row::posted);
    }
    // The row's value scaled by the pool factor 0x1d0 (fctiwz through frame+0x50) and floored at 1.
    void scale_row_value() {
        const auto id = c.u32(club + club::id);
        const auto table_base = c.u32(object + object::table);
        c.set(4, table_base);
        const auto row = table_base + id * row::size;
        c.set(3, row);
        c.put16(row + row::value, scale_through_frame(guest::extended(c.i16(row + row::value)), pool::row_value));
        const auto row_again = row_of();
        const auto value = c.i16(row_again + row::value);
        compare(c, value, 1);
        if (value < 1) c.put16(row_again + row::value, 1);
    }
    // The manager's morale less 250, floored.
    void lower_morale(std::int16_t floor) {
        const auto manager = manager_of();
        c.set(3, manager);
        compare_logical(c, manager);
        if (!manager) return;
        const auto record = c.u32(manager + manager::staff);
        c.set(4, record);
        compare_logical(c, record);
        if (!record) return;
        const auto morale = c.i16(record + staff::morale);
        c.set(3, guest::extended(morale));
        c.put16(record + staff::morale, std::uint32_t(morale - 0xfa));
        const auto person = manager_of();
        c.set(3, person);
        const auto again = c.u32(person + manager::staff);
        c.set(3, again);
        const auto now = c.i16(again + staff::morale);
        compare(c, now, floor);
        if (now < floor) c.put16(again + staff::morale, std::uint32_t(floor));
    }
    // 0x101a9a1c..0x101a9b14: the manager's job assessment (0x101ad080, job 5, 0 without a staff
    // record) against the offer value (0x101abda0) plus 450; above it the row's value is scaled
    // and, 30 days past the row's dates, outcome 1 runs. True when above (the review ends).
    bool sentence(std::uint32_t back_assess, std::uint32_t back_offer, std::uint32_t back_days, std::uint32_t back_outcome) {
        const auto manager = manager_of();
        c.set(4, manager);
        compare_logical(c, manager);
        bool assessed = false;
        if (manager) {
            const auto record = c.u32(manager + manager::staff);
            compare_logical(c, record);
            assessed = record != 0;
        }
        if (assessed) {
            c.set({{3, object}, {5, club}, {6, 0}, {7, 5}});
            c.link(back_assess);
            assess(c, manager, club, 0, 5);
            threshold = c.r(3);
        } else {
            threshold = 0;
        }
        c.set(5, manager_of());
        c.set({{3, object}, {4, club}});
        c.call(callee::offer_value, back_offer);
        const auto limit = std::int32_t(c.r(3) + 0x1c2u);
        compare(c, std::int32_t(std::uint32_t(threshold)), limit);
        if (std::int32_t(std::uint32_t(threshold)) <= limit) return false;
        scale_row_value();
        const auto days = days_since_row_date(back_days);
        compare(c, days, 0x1e);
        if (days > 0x1e) {
            c.set({{3, object}, {4, club}, {5, 1}});
            c.call(callee::outcome, back_outcome);
        }
        return true;
    }
    // An outcome 2 for a row 14 days past its dates.
    void quiet_notice() {
        const auto days = days_since_row_date(0x101a9dc8);
        compare(c, days, 0xe);
        if (days > 0xe) {
            c.set({{3, object}, {4, club}, {5, 2}});
            c.call(callee::outcome, 0x101a9de0);
        }
    }

    // 0x101a9820..0x101a9de0: the last rules, with or without a second person.
    void verdict() {
        const auto second = c.u32(club + club::second_person);
        c.set(3, second);
        compare_logical(c, second);
        bool staffed = false;
        if (second) {
            const auto record = c.u32(second + manager::staff);
            compare_logical(c, record);
            staffed = record != 0;
        }
        if (staffed) with_second_person();
        else without_second_person();
    }
    void with_second_person() {
        c.set(3, club);
        c.call(callee::club_flag, 0x101a9840);
        compare(c, std::int8_t(c.r(3)));
        if (std::int8_t(c.r(3)) == 0) {
            const auto league_record = c.u32(club + club::league);
            compare_logical(c, league_record);
            if (!league_record) return quiet_notice();
        }
        const auto second = c.u32(club + club::second_person);
        c.set(3, second);
        const auto record = c.u32(second + manager::staff);
        c.set(3, record);
        aux = c.u8(record + staff::trait);
        const auto bound = std::int32_t(std::int8_t(std::uint32_t(aux)));
        aux = std::uint32_t(bound);
        compare(c, bound);
        roll = bound == 0 ? 0 : std::uint32_t(inline_draw(bound, 0x101a9890));
        compare(c, std::int32_t(std::uint32_t(roll)), 5);
        if (std::int32_t(std::uint32_t(roll)) >= 5) return quiet_notice();
        step(c, table, 0x101a99e0);
        if (table_remainder(20, 0x66666667u, 3) == 0) return quiet_notice();
        if (sentence(0x101a9a48, 0x101a9a64, 0x101a9afc, 0x101a9b14)) return;
        lower_morale(0x5dc);
        post_record(1, 0x101a9b70, 0x101a9b84);
    }
    void without_second_person() {
        c.set(3, club);
        c.call(callee::club_flag, 0x101a9bb0);
        compare(c, std::int8_t(c.r(3)));
        if (std::int8_t(c.r(3)) == 0) {
            const auto league_record = c.u32(club + club::league);
            compare_logical(c, league_record);
            if (!league_record) return dismissal();
        }
        step(c, table, 0x101a9be4);
        if (table_remainder(3, 0x55555556u, 0) != 0) return dismissal();
        if (sentence(0x101a9c48, 0x101a9c64, 0x101a9cfc, 0x101a9d14)) return;
        dismissal();
    }
    void dismissal() {
        lower_morale(1);
        post_record(1, 0x101a9d70, 0x101a9d84);
    }

    Cpu& c;
    Frame& frame;
    Fpu fpu;
    Table table;
    std::uint32_t object, club, date, pool;
    Kept<19> threshold;
    Kept<20> roll;
    Kept<21> aux;
    Kept<22> subject;
};
}  // namespace

void review(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t flag) {
    c.set(0, c.lr());
    Frame frame(c, 0x101a8ad0, 0xa0, {.first_gpr = 19});
    Review(c, frame, object, club).run(flag);
    lmw_leaves(c, frame.caller());
}
}  // namespace cm::aot::manager_manager
