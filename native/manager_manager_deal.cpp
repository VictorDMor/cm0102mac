// MANAGER_MANAGER.CPP transfer-deal verdict block (manager_manager_deal.hpp): 0x101a0f50, the board's
// verdict on a deal for a player.
//
// The routine keeps the original's stack image: the prologue stores, the flag byte it stores above
// its frame, the int-to-double words (0x43300000 and value ^ 0x80000000), the eight bytes of each
// fctiwz + stfd, and the registers the original holds at each call (the callee's prologue saves
// r13-r31 and f14-f31) and leaves at its end (r3-r10, cr0, f1).
#include "manager_manager_deal.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "manager_manager_detail.hpp"
#include "manager_manager_review.hpp"
#include "manager_manager_deal_detail.hpp"
#include <cmath>

namespace cm::aot::manager_manager {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;
using guest::Kept;
using guest::compare;
using guest::compare_logical;
using namespace deal_detail;
namespace {
namespace callee {
constexpr std::uint32_t staff_lookup = 0x1018c340,  // the staff record the screening reads
    staff_check = 0x1018c270, seed = 0x1077b790, time = 0x1077f900, staff_flag = 0x102de3f0,
    rating = 0x100d2f50, deal_record = 0x10082f20, valuation = 0x10084be0, buyer_object = 0x10103510,
    club_state = 0x1014c2e0, club_check = 0x100c9070, base_value = 0x10085860, player_value = 0x100f5770,
    divide64 = 0x10773960, level_of = 0x100f55b0, glue = 0x10780fc0;
}
namespace toc {
constexpr std::int32_t pool = -0x536c, staff_owner = -0x7ba0, scale_owner = -0x7b28, deals = -0x7b7c,
                       deal_cell = -0x7b78, clubs = -0x7be8, buyers = -0x7ab8, home_league = -0x7a0c;
}
// Frame slots (offsets from the frame base).
namespace slot {
constexpr std::uint32_t flag = 0xfb, toc_save = 0x14, time_out = 0x38;
// The floating-point stage of the draw the original inlines for the staff's figure.
constexpr std::uint32_t stage_bits = 0x54, stage_bound = 0x58, stage_convert = 0x5c, stage_bound_low = 0x60,
                        stage_rest_high = 0x64, stage_rest_low = 0x68, stage_copy = 0x44, stage_copy_low = 0x48,
                        stage_out = 0x4c, stage_out_low = 0x50, stage_again = 0x3c, stage_again_low = 0x40;
}

std::int32_t sbyte(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
std::int32_t shalf(std::uint32_t value) { return std::int32_t(std::int16_t(value)); }
std::uint32_t word(std::int32_t value) { return std::uint32_t(value); }
// The compiler's divisions by constants (mulhw, shifts, the quotient's sign bit added back) are the
// truncating C++ ones for every 32-bit input (all 2^32 values checked for /200, /10 and /0xffff).
std::uint32_t sign_of(std::int32_t value) { return value < 0 ? 1 : 0; }
std::int32_t quotient(std::int32_t value, std::int32_t divisor) { return value / divisor; }
std::uint32_t product(std::int32_t value, std::int32_t factor) { return word(value) * word(factor); }
// The remainder of the table's number by a bound, as the draw's modulo takes it: the result has
// the bound's sign (the original negates it for a bound that is not positive). A bound of 0
// (only the pool-driven draw can reach it) divides by zero, whose quotient the original multiplies
// back by the divisor: the remainder is the number itself.
std::int32_t signed_remainder(std::int32_t number, std::int32_t bound) {
    const auto rest = bound == 0 ? number : remainder(number, bound);
    return bound > 0 ? rest : std::int32_t(0u - word(rest));
}

class Deal {
public:
    Deal(Cpu& c, Frame& frame, std::uint32_t object, std::uint32_t player, std::uint32_t club, std::uint32_t fee,
         std::uint32_t flag)
        : c(c), frame(frame), fpu(c), table(draw_table::cells(c)), pool(c.toc(toc::pool)),
          player(player), club(club), fee(fee), flag(flag), r13(c, 0), r14(c, c.toc(toc::scale_owner)),
          r19(c, c.r(19)), r20(c, c.r(20)), r21(c, c.r(21)), r22(c, c.r(22)), r23(c, c.r(23)), r24(c, c.r(24)) {
        guest::keep(c, {{15, object}, {16, player}, {17, club}, {18, fee}, {25, c.toc(toc::staff_owner)},
                        {26, table.base_cell}, {27, table.first}, {28, table.last_cell}, {29, table.cursor_cell},
                        {30, pool}});
    }

    // The verdict bits (r3).
    std::uint32_t run() {
        c.put8(frame.at(slot::flag), flag);
        const auto byte = std::int8_t(flag);
        compare(c, byte);  // extsb. r0,r0
        if (byte != 0 && screen()) return finish(0x2000);
        return finish(price());
    }

private:
    std::uint32_t finish(std::uint32_t verdict) {
        c.set(3, verdict);
        lmw_leaves(c, frame.caller());
        return verdict;
    }
    double constant(std::uint32_t offset) const { return c.f64(pool + offset); }
    // The int-to-double idiom: the high word and the value with its sign bit flipped, as a double.
    void put_conversion(std::uint32_t low_slot, std::uint32_t value) {
        c.put32(frame.at(low_slot - 4), conversion_high);
        c.put32(frame.at(low_slot), value ^ 0x80000000u);
    }

    // ---- The staff's screening (0x101a0fa4..0x101a1c74), for a flagged call. True: refused. ----

    bool screen() {
        c.set(3, c.u32(c.r(25)));
        c.call(callee::staff_lookup, 0x101a0fac);
        const auto record = c.r(3);
        c.set(31, record);
        compare(c, std::int32_t(record));  // or. r31,r3,r3
        if (record == 0) return false;
        const auto figure = c.i16(record + screen::figure);
        compare(c, figure, screen::figure_floor);
        if (figure <= screen::figure_floor) return false;
        for (r19 = 0, r23 = 0;;) {
            if (staff_turn()) return true;
            r19 = word(std::int32_t(std::uint32_t(r19)) + 1);
            r23 = std::uint32_t(r23) + 4;
            compare(c, std::int32_t(std::uint32_t(r19)), 3);
            if (!(std::int32_t(std::uint32_t(r19)) < 3)) return false;
        }
    }

    // The draw the original inlines for a bound (never beyond +-0xffff but for the pool-driven one,
    // so the original's floating-point stage 0x...1080..0x...111c is dead): the table's step, then the
    // table's number modulo the bound. `back` is the address after the redraw's `bl`.
    std::int32_t roll(std::int32_t bound, std::uint32_t back) {
        draw_table::step(c, table, back);
        return after_step(bound);
    }
    std::int32_t after_step(std::int32_t bound) {
        const auto cursor = c.u32(table.cursor_cell);
        c.set(3, cursor);
        compare(c, bound);  // cmpwi bound,0
        const auto base = c.u32(table.base_cell);
        const auto x = std::uint32_t(c.u16(cursor)) + base;
        c.set(3, x);
        return signed_remainder(std::int32_t(x), bound);
    }
    // The staff member's rating: 0x100d2f50 on his staff record (r5 holds the record in one variant).
    std::int32_t rating_of(std::uint32_t back, bool keeps_record = false) {
        const auto record = c.u32(std::uint32_t(r20) + staff_person::record);
        if (keeps_record) c.set(5, record);
        c.set(4, c.u8(record + staff_record::rating));
        c.set(3, record);
        c.call(callee::rating, back);
        return sbyte(c.r(3));
    }

    // One of the club's three staff people (0x101a0fcc..0x101a1c64). True: the verdict is 0x2000.
    bool staff_turn() {
        r20 = c.u32(club + club::staff + std::uint32_t(r23));
        compare_logical(c, r20);
        if (std::uint32_t(r20) == 0) return false;
        const auto member = c.u32(std::uint32_t(r20) + staff_person::record);
        compare_logical(c, member);
        if (member == 0) return false;
        // The generator is seeded from the player's and the staff member's ids.
        const auto player_id = c.u32(player), staff_id = c.u32(std::uint32_t(r20));
        c.set(4, player_id + 1);
        c.set(3, (player_id + 1) * (staff_id + 1));
        c.call(callee::seed, 0x101a1000);
        c.set(3, c.u32(c.r(14)));
        c.set(4, player);
        r13 = 1;
        c.set(5, 1);
        c.call(callee::staff_flag, 0x101a1014);
        compare_logical(c, c.r(3));
        if (c.r(3) != 0) {
            // The rating draws against itself: a non-zero draw refuses.
            r21 = word(rating_of(0x101a1028));
            compare(c, std::int32_t(std::uint32_t(r21)));
            r22 = std::uint32_t(r21) == 0 ? 0 : word(roll(std::int32_t(std::uint32_t(r21)), 0x101a1058));
            compare(c, std::int32_t(std::uint32_t(r22)));
            if (std::uint32_t(r22) != 0) return true;
            c.set(3, c.u32(c.r(14)));
            c.set(4, player);
            c.set(5, 0);
            c.call(callee::staff_flag, 0x101a1198);
            compare_logical(c, c.r(3));
            if (c.r(3) != 0) return true;
        }
        if (figure_beats_table()) return true;
        if (staff_check_draw()) return true;
        return pool_driven_draw();
    }

    // 0x101a11a8..0x101a14ec: the staff record's figure against the table (a number modulo 200 plus
    // 25), then two draws against each other.
    bool figure_beats_table() {
        draw_table::step(c, table, 0x101a11c8);
        const auto cursor = c.u32(table.cursor_cell);
        c.set(5, cursor);
        const auto base = c.u32(table.base_cell);
        c.set(4, base);
        const auto halfword = c.u16(cursor);
        c.set(3, halfword);
        const auto figure = c.i16(c.r(31) + screen::figure);
        c.set(5, guest::extended(figure));
        const auto x = std::int32_t(std::uint32_t(halfword) + base);
        c.set(4, word(x));
        const auto rest = x - quotient(x, 200) * 200;
        c.set(3, word(rest));
        compare(c, figure, rest + 0x19);
        if (figure <= rest + 0x19) return false;
        const auto level = rating_of(0x101a121c);
        const auto stage = (0x15 - level) * 5;
        c.set(3, word(stage));
        r21 = word(stage + 0x64);
        compare(c, std::int32_t(std::uint32_t(r21)));
        r22 = std::uint32_t(r21) == 0 ? 0 : word(roll(std::int32_t(std::uint32_t(r21)), 0x101a1258));
        r24 = guest::extended(c.i16(c.r(31) + screen::figure));
        compare(c, std::int32_t(std::uint32_t(r24)));
        r21 = std::uint32_t(r24) == 0 ? 0 : word(roll(std::int32_t(std::uint32_t(r24)), 0x101a13b4));
        compare(c, std::int32_t(std::uint32_t(r22)), std::int32_t(std::uint32_t(r21)));
        return std::int32_t(std::uint32_t(r22)) < std::int32_t(std::uint32_t(r21));
    }

    // 0x101a14f0..0x101a16e8: when 0x1018c270 answers non-zero for the player, the sum of its answer
    // and the member's rating draws against a number modulo 10.
    bool staff_check_draw() {
        c.set(3, c.u32(c.r(25)));
        c.set(4, player);
        c.call(callee::staff_check, 0x101a14fc);
        compare(c, sbyte(c.r(3)));  // extsb. r0,r3
        if (sbyte(c.r(3)) == 0) return false;
        c.set(3, c.u32(c.r(25)));
        c.set(4, player);
        c.call(callee::staff_check, 0x101a1510);
        r21 = word(sbyte(c.r(3)));
        const auto level = rating_of(0x101a1524, true);
        r21 = word(level + sbyte(std::uint32_t(r21)));
        compare(c, std::int32_t(std::uint32_t(r21)));  // add. r21,r0,r21
        r22 = std::uint32_t(r21) == 0 ? 0 : word(roll(std::int32_t(std::uint32_t(r21)), 0x101a1558));
        draw_table::step(c, table, 0x101a16a0);
        const auto cursor = c.u32(table.cursor_cell);
        c.set(5, cursor);
        const auto base = c.u32(table.base_cell);
        c.set(4, base);
        const auto halfword = c.u16(cursor);
        c.set(3, halfword);
        const auto x = std::int32_t(std::uint32_t(halfword) + base);
        const auto rest = x - quotient(x, 10) * 10;
        c.set(4, word(x));
        c.set(3, sign_of(x));
        compare(c, std::int32_t(std::uint32_t(r22)), rest);
        return std::int32_t(std::uint32_t(r22)) > rest;
    }

    // 0x101a16ec..0x101a1c64: the draw the member's rating drives through the import glue (f31 =
    // offset + glue(rating - bias, exponent)), then two more draws against numbers modulo 200.
    bool pool_driven_draw() {
        const auto level = rating_of(0x101a16f8);
        c.set(3, word(level) ^ 0x80000000u);
        put_conversion(slot::stage_bound, word(level));  // 0x54 high, 0x58 low
        const auto bias = constant(pool::bias);
        c.set_f(1, bias);
        c.set_f(2, constant(pool::glue_exponent));
        c.set_f(1, fpu.fsub(c.f64(frame.at(slot::stage_bits)), bias));
        c.call(callee::glue, 0x101a1720);
        c.set(2, c.u32(frame.at(slot::toc_save)));
        const auto offset = constant(pool::glue_offset);
        const auto f31 = fpu.fadd(offset, c.f(1));
        c.set_f(31, f31);
        fpu.fcmp(0, f31, constant(pool::zero));
        if (c.cr(0) & 2) r21 = 0;  // fcmpu equal: no draw
        else r21 = word(f31_roll(f31));
        compare(c, std::int32_t(std::uint32_t(r21)));
        if (std::uint32_t(r21) == 0) return true;
        // The member's rating again, as the bound of the next draw.
        r20 = word(rating_of(0x101a18e8));
        compare(c, std::int32_t(std::uint32_t(r20)));  // extsb. r20,r3
        r21 = std::uint32_t(r20) == 0 ? 0 : word(roll(std::int32_t(std::uint32_t(r20)), 0x101a1918));
        draw_table::step(c, table, 0x101a1a60);
        {
            const auto cursor = c.u32(table.cursor_cell);
            c.set(3, cursor);
            const auto base = c.u32(table.base_cell);
            c.set(5, base);
            const auto halfword = c.u16(cursor);
            c.set(4, halfword);
            const auto x = std::int32_t(std::uint32_t(halfword) + base);
            c.set(5, word(x));
            const auto rest = x - quotient(x, 200) * 200;
            c.set(4, sign_of(x));
            compare(c, std::int32_t(std::uint32_t(r21)), rest);
            if (std::int32_t(std::uint32_t(r21)) <= rest) return false;
        }
        const auto career_record = c.u32(player + person::career);
        c.set(4, career_record);
        r20 = word(sbyte(c.u8(career_record + career::byte)));
        compare(c, std::int32_t(std::uint32_t(r20)));
        if (std::uint32_t(r20) == 0) {
            r21 = 0;
        } else {
            draw_table::step_from(c, table, c.r(3), 0x101a1ad4);
            r21 = word(after_step(std::int32_t(std::uint32_t(r20))));
        }
        draw_table::step(c, table, 0x101a1c1c);
        const auto cursor = c.u32(table.cursor_cell);
        c.set(5, cursor);
        const auto base = c.u32(table.base_cell);
        c.set(4, base);
        const auto halfword = c.u16(cursor);
        c.set(3, halfword);
        const auto x = std::int32_t(std::uint32_t(halfword) + base);
        c.set(4, word(x));
        const auto rest = x - quotient(x, 200) * 200;
        c.set(3, sign_of(x));
        compare(c, std::int32_t(std::uint32_t(r21)), rest);
        return std::int32_t(std::uint32_t(r21)) > rest;
    }

    // 0x101a1740..0x101a18c8: the draw of the glue's number (f31 truncated by fctiwz). A bound within
    // +-0xffff takes the table's number modulo the bound; a wider one the floating-point stage
    // (a share through the unit constant, then the module's draw 0x10195e10 for the rest). Answers r21.
    std::int32_t f31_roll(double f31) {
        draw_table::step(c, table, 0x101a1760);
        auto bits = fpu.fctiwz(0, f31);
        c.put64(frame.at(slot::stage_bits), bits);
        auto bound = c.i32(frame.at(slot::stage_bound));
        c.set(3, word(bound));
        compare(c, bound, 0xffff);
        bool wide = bound > 0xffff;
        if (!wide) {
            c.put64(frame.at(slot::stage_bits), bits);
            bound = c.i32(frame.at(slot::stage_bound));
            c.set(3, word(bound));
            compare(c, bound, -0xffff);
            wide = !(bound >= -0xffff);
        }
        if (!wide) {
            const auto cursor = c.u32(table.cursor_cell);
            c.set(3, cursor);
            c.put64(frame.at(slot::stage_again), bits);
            const auto halfword = c.u16(cursor);
            c.set(3, halfword);
            const auto base = c.u32(table.base_cell);
            const auto taken = c.i32(frame.at(slot::stage_again_low));
            c.set(4, word(taken));
            const auto x = std::int32_t(std::uint32_t(halfword) + base);
            c.set(5, word(x));
            c.put64(frame.at(slot::stage_copy), bits);
            c.set(3, word(taken == 0 ? 0 : divide(x, taken)));
            const auto again = c.i32(frame.at(slot::stage_copy_low));
            compare(c, again);  // cmpwi r0,0
            return signed_remainder(x, taken);
        }
        bits = fpu.fctiwz(0, f31);
        c.set(4, conversion_high);
        c.put32(frame.at(slot::stage_convert), conversion_high);
        const auto cursor = c.u32(table.cursor_cell);
        c.set(6, cursor);
        c.put64(frame.at(slot::stage_bits), bits);
        c.put64(frame.at(slot::stage_copy), bits);
        const auto bias = constant(pool::bias);
        const auto wide_bound = c.i32(frame.at(slot::stage_bound));
        c.put32(frame.at(slot::stage_bound_low), word(wide_bound) ^ 0x80000000u);
        const auto unit = constant(pool::unit);
        const auto tenfold = std::int32_t(product(c.i32(frame.at(slot::stage_copy_low)), 10));
        auto ratio = c.f64(frame.at(slot::stage_convert));
        const auto halfword = c.u16(cursor);
        ratio = fpu.fsub(ratio, bias);
        const auto base = c.u32(table.base_cell);
        c.put32(frame.at(slot::stage_rest_high), conversion_high);
        const auto x = std::int32_t(std::uint32_t(halfword) + base);
        ratio = fpu.fdiv(ratio, unit);
        const auto whole = quotient(x, 0xffff);
        const auto rest = word(x) - word(whole) * 0xffffu;
        c.put32(frame.at(slot::stage_rest_low), rest ^ 0x80000000u);
        c.set({{5, 0xffff}, {6, sign_of(x)}, {7, word(x)}, {4, rest ^ 0x80000000u}});
        auto share = c.f64(frame.at(slot::stage_rest_high));
        share = fpu.fsub(share, bias);
        share = fpu.fmul(ratio, share);
        const auto next = quotient(tenfold, 0xffff);
        c.set(3, word(next));
        c.put64(frame.at(slot::stage_out), fpu.fctiwz(0, share));
        r21 = c.u32(frame.at(slot::stage_out_low));
        c.set_f(1, ratio);
        guest::enter(c, 0x101a1850, {word(next)});
        const auto drawn = draw(c, next);
        bits = fpu.fctiwz(0, f31);
        c.put64(frame.at(slot::stage_again), bits);
        const auto limit = c.i32(frame.at(slot::stage_again_low));
        const auto shifted = std::int32_t(word(drawn) + 5u);
        const auto tenth = quotient(shifted, 10);
        c.set(4, word(limit) - 1u);
        c.set(3, sign_of(shifted));
        r21 = std::uint32_t(r21) + word(tenth);
        const auto cap = std::int32_t(word(limit) - 1u);
        compare(c, std::int32_t(std::uint32_t(r21)), cap);
        if (std::int32_t(std::uint32_t(r21)) > cap) r21 = word(cap);
        return std::int32_t(std::uint32_t(r21));
    }

    // ---- The price (0x101a1c78..0x101a2460). ----

    std::uint32_t price() {
        compare(c, std::int8_t(std::uint32_t(r13)));  // extsb. r0,r13
        if (std::int8_t(std::uint32_t(r13)) != 0) {
            // Staff were asked: the generator is reseeded from the clock.
            c.set(3, frame.at(slot::time_out));
            c.call(callee::time, 0x101a1c88);
            c.call(callee::seed, 0x101a1c8c);
        }
        c.set(3, c.toc(toc::deals));
        c.set(4, player);
        c.set(3, c.u32(c.r(3)));
        c.call(callee::deal_record, 0x101a1c9c);
        r13 = c.r(3);
        compare(c, std::int32_t(std::uint32_t(r13)));  // or. r13,r3,r3
        if (std::uint32_t(r13) == 0) return 0;
        // A six-bit field of the record's byte at +0x38: 1 and 3 end the verdict.
        const auto kind = std::int32_t((std::uint32_t(c.u8(std::uint32_t(r13) + deal::kind_byte)) & 0xfe) << 24) >> 26;
        compare(c, kind, 1);
        if (kind == 1) return 0;
        compare(c, kind, 3);
        if (kind == 3) return 0;
        const auto link = c.u32(player + person::link);
        compare_logical(c, link);
        if (link == 0) return 0;
        c.set({{3, club}, {4, 1}, {5, 1}, {6, 0}, {7, 0}});
        c.call(callee::club_state, 0x101a1cf8);
        compare(c, sbyte(c.r(3)));  // extsb. r0,r3
        if (sbyte(c.r(3)) == 0) return 0;
        // The reputation, scaled and squared, bounds the price from 150000 to 2000000.
        const auto reputation = c.i16(club + club::reputation);
        c.set(4, guest::extended(reputation));
        c.set(3, 0x20000);
        put_conversion(0x40, guest::extended(reputation));
        const auto bias = constant(pool::bias);
        c.set_f(1, bias);
        auto scaled = fpu.fsub(c.f64(frame.at(0x3c)), bias);
        scaled = fpu.fmul(constant(pool::reputation_scale), scaled);
        scaled = fpu.fmul(scaled, scaled);
        c.put64(frame.at(slot::stage_copy), fpu.fctiwz(0, scaled));
        r14 = c.u32(frame.at(slot::stage_copy_low));
        const auto drawn = std::int32_t(std::uint32_t(r14));
        compare(c, drawn, 0x249f0);
        if (drawn < 0x249f0) {
            r14 = 0x249f0;
        } else {
            c.set(3, 0x1f0000);
            compare(c, drawn, 0x1e8480);
            if (drawn > 0x1e8480) r14 = 0x1e8480;
        }
        const auto stored = std::int8_t(c.u8(frame.at(slot::flag)));  // lbz, extsb.: the original branches on the reloaded byte
        compare(c, stored);
        return stored != 0 ? bid() : ask();
    }

    // The board's row of the club lowered by three steps (the value is then at least 1).
    std::uint32_t lower_row(std::int32_t first, std::int32_t second, std::int32_t third, std::uint32_t verdict) {
        const auto row_of = [&] { return c.u32(c.r(15)) + c.u32(c.r(17)) * row::size; };
        auto row_at = row_of();
        c.set(4, row_at);
        c.put16(row_at + row::value, word(shalf(c.u16(row_at + row::value)) - first));
        row_at = row_of();
        c.set(4, row_at);
        c.put16(row_at + row::standing, word(shalf(c.u16(row_at + row::standing)) - second));
        row_at = row_of();
        c.set(4, row_at);
        c.put16(row_at + row::trust, word(shalf(c.u16(row_at + row::trust)) - third));
        row_at = row_of();
        const auto value = c.i16(row_at + row::value);
        compare(c, value, 1);
        if (value < 1) c.put16(row_at + row::value, 1);
        return verdict;
    }

    // The deal record's club (0x10082f20), kept in the game's cell, and its club record: r5 for the
    // valuation that follows (0 without a record).
    void deal_club(std::uint32_t back) {
        c.set(3, c.toc(toc::deals));
        c.set(4, player);
        c.set(3, c.u32(c.r(3)));
        c.call(callee::deal_record, back);
        const auto record = c.r(3);
        c.set(4, c.toc(toc::deal_cell));
        compare_logical(c, record);
        c.put32(c.r(4), record);
        if (record != 0) {
            const auto index = c.u32(record + deal::club_index);
            c.set(3, c.toc(toc::clubs));
            c.set(3, c.u32(c.r(3)));
            c.set(5, c.r(3) + index * club_size);
        } else {
            c.set(5, 0);
        }
    }
    // 0x10084be0 for the player: the deal record's club (`own` 1) or the given club (0), at `link`.
    std::int32_t valuation(std::uint32_t club_record, std::uint32_t last, std::uint32_t back) {
        c.set(3, c.toc(toc::deals));
        c.set(4, player);
        c.set(5, club_record);
        c.set({{6, 0}, {7, 0xffffffffu}, {8, last}});
        c.set(3, c.u32(c.r(3)));
        c.call(callee::valuation, back);
        return std::int32_t(c.r(3));
    }

    // Is amount * 1 above the factor times the fee (fcmpo)? The converted words sit at 0x40/0x48.
    // Leaves f1 the subject's double.
    bool above(std::uint32_t factor, std::uint32_t subject) {
        c.set(3, conversion_high);
        c.put32(frame.at(0x48), fee ^ 0x80000000u);
        c.put32(frame.at(0x44), conversion_high);
        const auto bias = constant(pool::bias);
        auto limit = fpu.fsub(c.f64(frame.at(0x44)), bias);
        c.put32(frame.at(0x40), subject ^ 0x80000000u);
        c.put32(frame.at(0x3c), conversion_high);
        limit = fpu.fmul(constant(factor), limit);
        const auto amount = fpu.fsub(c.f64(frame.at(0x3c)), bias);
        c.set_f(1, amount);
        fpu.fcmp(0, amount, limit);
        return (c.cr(0) & 4) != 0;  // `ble` branches when GT is clear (unordered too), so only GT falls through
    }

    // A player not flagged: his price against the value the game puts on him.
    std::uint32_t ask() {
        const auto region_record = c.u32(club + club::region);
        c.set(3, region_record);
        compare_logical(c, region_record);
        bool cheap = false;
        if (region_record != 0) {
            const auto marked = c.i8(region_record + region::flag);
            compare(c, marked);  // extsb. r0,r0
            if (marked != 0) {
                const auto league = c.u32(club + club::league);
                c.set(4, league);
                compare_logical(c, league);
                if (league != 0) {
                    const auto home = c.toc(toc::home_league);
                    c.set(3, home);
                    const auto league_id = c.u32(league);
                    c.set(4, league_id);
                    const auto home_id = c.u32(home);
                    compare(c, std::int32_t(league_id), std::int32_t(home_id));
                    if (league_id != home_id) {
                        c.set({{3, club}, {4, 0}, {5, 0}});
                        c.call(callee::club_check, 0x101a1dc0);
                        const auto checked = sbyte(c.r(3));
                        compare(c, checked, 0xf);
                        cheap = checked < 0xf;
                    }
                }
            }
        }
        if (cheap) return lower_row(0x4e2, 0x4e2, 0x1f4, 0x200);
        c.set(3, c.toc(toc::deals));
        c.set(5, club);
        c.set(4, c.u32(player + person::link));
        c.set(6, player);
        c.set(3, c.u32(c.r(3)));
        c.set({{7, 0}, {8, 0}, {9, 1}});
        c.call(callee::base_value, 0x101a1e70);
        r19 = c.r(3);
        deal_club(0x101a1e88);
        const auto own = valuation(c.r(5), 1, 0x101a1ed0);
        compare(c, std::int32_t(std::uint32_t(r19)), own);
        if (std::int32_t(std::uint32_t(r19)) > own) r19 = word(own);
        c.set(3, c.toc(toc::deals));
        c.set(4, player);
        c.set(5, c.u32(player + person::link));
        c.set({{6, 0}, {7, 0xffffffffu}, {8, 0}});
        c.set(3, c.u32(c.r(3)));
        c.call(callee::valuation, 0x101a1efc);
        const auto asking = std::int32_t(c.r(3));
        const auto amount = std::int32_t(std::uint32_t(r19));
        compare(c, amount, std::int32_t(std::uint32_t(r14)));
        if (amount <= std::int32_t(std::uint32_t(r14))) return 0;
        compare(c, asking, std::int32_t(fee));
        if (asking < std::int32_t(fee)) return 0;
        // The amount far above the fee at a rich deal level, or above 5000000, or above the floor ratio.
        if (above(pool::ask_ratio, word(amount))) {
            const auto level = std::int32_t(std::int8_t(c.u8(std::uint32_t(r13) + deal::level)));
            compare(c, level, 0x19);
            if (level > 0x19) return lower_row(0x4e2, 0x4e2, 0x1f4, 0x800);
        }
        c.set(3, 0x4c0000);
        compare(c, amount, 0x4c4b40);
        if (amount > 0x4c4b40) {
            const auto again = std::int32_t(std::int8_t(c.u8(std::uint32_t(r13) + deal::level)));
            compare(c, again);  // extsb. r0,r0
            if (again > 0 && above(pool::big_ask_ratio, word(amount)))
                return lower_row(0x4e2, 0x4e2, 0x1f4, 0x800);
        }
        if (above(pool::floor_ratio, word(amount))) return lower_row(0x1f4, 0x1f4, 0xfa, 0x800);
        return 0;
    }

    // A flagged player: the price against the bought player's value times 1.2, then against the
    // club's means by the level of its buyer.
    std::uint32_t bid() {
        c.set(3, c.u32(c.toc(toc::buyers)));
        c.set(4, club);
        c.call(callee::buyer_object, 0x101a2188);
        c.set({{4, player}, {5, 0}, {6, 0}, {7, 1}, {8, 0}});
        c.call(callee::player_value, 0x101a21a0);
        const auto high = c.r(3), low = c.r(4);
        const auto carry = std::uint32_t((std::uint64_t(low) * 12) >> 32);
        c.set({{7, 12}, {5, 0}, {6, 10}, {3, carry + high * 12}, {4, low * 12}});
        c.call(callee::divide64, 0x101a21c0);
        // A signed 64-bit compare of the quotient with the price (subfc, subfe, subfe, neg.).
        const auto quotient_high = c.r(3), quotient_low = c.r(4);
        const auto value = std::int64_t(std::uint64_t(quotient_high) << 32 | quotient_low);
        const bool below = value < std::int64_t(std::int32_t(fee));
        c.set(5, quotient_high ^ 0x80000000u);
        c.set(3, below ? 1 : 0);
        compare(c, below ? 1 : 0);
        if (below) return 0x800;
        compare(c, std::int32_t(fee), std::int32_t(std::uint32_t(r14)));
        if (std::int32_t(fee) <= std::int32_t(std::uint32_t(r14))) return 0;
        r13 = word(valuation(club, 0, 0x101a2210));
        deal_club(0x101a2224);
        const auto other = valuation(c.r(5), 1, 0x101a226c);
        compare(c, std::int32_t(std::uint32_t(r13)), other);
        if (std::int32_t(std::uint32_t(r13)) > other) r13 = word(other);
        c.set(3, c.u32(c.toc(toc::buyers)));
        c.set(4, club);
        c.call(callee::buyer_object, 0x101a2288);
        compare_logical(c, c.r(3));
        if (c.r(3) == 0) return 0;
        c.call(callee::level_of, 0x101a229c);
        const auto level = sbyte(c.r(3));
        std::uint32_t site, gain;
        compare(c, level, 2);
        if (level == 2) {
            site = 0x101a22dc;
            gain = pool::level_two;
        } else {
            compare(c, level, 1);
            if (level == 1) {
                site = 0x101a2334;
                gain = pool::level_one;
            } else {
                site = 0x101a2384;
                gain = pool::level_other;
            }
        }
        c.set(3, conversion_high);
        const auto bias = constant(pool::bias);
        c.put32(frame.at(0x40), std::uint32_t(r14) ^ 0x80000000u);
        c.put32(frame.at(0x48), std::uint32_t(r13) ^ 0x80000000u);
        c.put32(frame.at(0x3c), conversion_high);
        c.put32(frame.at(0x44), conversion_high);
        const auto gain_scale = constant(pool::value_gain);
        auto base_value = fpu.fsub(c.f64(frame.at(0x3c)), bias);
        auto sum = fpu.multiply_add(site, gain_scale, base_value, constant(gain));
        c.set_f(1, sum);
        auto extra = fpu.fsub(c.f64(frame.at(0x44)), bias);
        sum = fpu.fadd(sum, extra);
        c.put64(frame.at(slot::stage_out), fpu.fctiwz(0, sum));
        const auto target = c.u32(frame.at(slot::stage_out_low));
        c.set(4, target);
        if (!limit_test(target)) return 0;
        return lower_row(0xfa, 0x1f4, 0x1f4, 0x800);
    }
    // 0x101a239c: the target against the limit ratio times the price (fcmpo, bge: not below).
    bool limit_test(std::uint32_t target) {
        c.set(3, conversion_high);
        c.put32(frame.at(0x48), fee ^ 0x80000000u);
        c.put32(frame.at(0x44), conversion_high);
        const auto bias = constant(pool::bias);
        auto limit = fpu.fsub(c.f64(frame.at(0x44)), bias);
        c.put32(frame.at(0x40), target ^ 0x80000000u);
        c.put32(frame.at(0x3c), conversion_high);
        limit = fpu.fmul(constant(pool::limit_ratio), limit);
        const auto shown = fpu.fsub(c.f64(frame.at(0x3c)), bias);
        c.set_f(1, shown);
        fpu.fcmp(0, shown, limit);
        return (c.cr(0) & 8) != 0;  // `bge` branches when LT is clear (unordered too): only LT lowers the row
    }

    Cpu& c;
    Frame& frame;
    Fpu fpu;
    draw_table::Table table;
    std::uint32_t pool, player, club, fee, flag;
    Kept<13> r13;
    Kept<14> r14;
    Kept<19> r19;
    Kept<20> r20;
    Kept<21> r21;
    Kept<22> r22;
    Kept<23> r23;
    Kept<24> r24;
};
}  // namespace

std::uint32_t deal_verdict(Cpu& c, std::uint32_t object, std::uint32_t player, std::uint32_t club, std::uint32_t fee,
                           std::uint32_t flag) {
    c.set(0, c.lr());
    Frame frame(c, 0x101a0f50, 0xd0, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 31});
    Deal verdict(c, frame, object, player, club, fee, flag);
    return verdict.run();
}
}  // namespace cm::aot::manager_manager
