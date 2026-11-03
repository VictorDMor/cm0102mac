#include "match_pl_pressure.hpp"

namespace cm::aot::match_pl {
using namespace guest;

bool CellPressure::wins_unsettled_contact() {
    c.set(3, actor.match());
    c.set(0, 0x43300000);
    c.set_f(0, c.f32(c.r(21) + 0x1c));
    c.set(3, c.u8(c.r(3) + 0x7920));
    c.put32(frame.at(0x70), c.r(0));
    c.set_f(2, fp.fmuls(c.f(0), c.f(31)));
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    c.set_f(0, c.f32(c.r(21) + 0xdc));
    c.set(0, c.r(0) * 100);
    c.set_f(1, c.f64(c.r(27) + 0x80));
    c.set_f(2, fp.multiply_add(0x1023be9c, c.f(2), c.f(31), c.f(0)));
    c.set(0, c.r(0) ^ 0x80000000);
    c.put32(frame.at(0x74), c.r(0));
    c.set_f(0, c.f64(frame.at(0x70)));
    c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
    c.set_f(0, fp.fadds(c.f(2), c.f(0)));
    c.put64(frame.at(0x78), fp.fctiwz(0, c.f(0)));
    c.set(4, c.u32(frame.at(0x7c)));
    c.set(0, std::uint32_t(std::int16_t(c.r(4))));
    if (compare(c, std::int32_t(c.r(0)), 160) < 0) c.set(4, 160);
    c.set(3, std::uint32_t(std::int16_t(c.r(18))));
    c.set(0, std::uint32_t(std::int16_t(c.r(4))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) < 0) return false;

    c.set_f(0, c.f64(c.r(27) + 0x10));
    fp.fcmp(0, c.f(31), c.f(0));
    if (c.cr(0) & 8) return false;
    c.set(3, actor.match());
    c.set(0, 0x43300000);
    c.put32(frame.at(0x70), c.r(0));
    c.set(0, c.u8(c.r(3) + 0x7920));
    c.set_f(1, c.f64(c.r(27) + 0x80));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, (c.r(3) + 1) ^ 0x80000000);
    c.put32(frame.at(0x74), c.r(0));
    c.set_f(0, c.f64(frame.at(0x70)));
    c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
    fp.fcmp(0, c.f(31), c.f(0));
    // The original ORs CR0.LT into CR0.EQ before testing the result.
    if (c.cr(0) & 8) c.set_cr(0, c.cr(0) | 2);
    return !(c.cr(0) & 2);
}

void CellPressure::credit_contact(unsigned points) {
    c.set(0, std::uint32_t(std::int8_t(c.r(22))));
    if (compare(c, std::int32_t(c.r(0))) == 0) return;
    c.set(3, c.u8(actor.address + 0x12));
    c.set(0, c.r(3) + 1);
    c.put8(actor.address + 0x12, c.r(0));
    c.set(3, std::uint32_t(c.i16(actor.address + 0x36)));
    c.set(0, c.r(3) + points);
    c.put16(actor.address + 0x36, c.r(0));
}

bool CellPressure::keeper_defends_goal(std::uint32_t back) {
    if (!role_is_keeper(back)) return false;
    c.set(0, actor.side());
    const bool away = compare(c, std::int32_t(c.r(0)), 1) == 0;
    c.set(0, unsigned(inside_goal_region(away, 0, 3)));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

void CellPressure::scatter_after_contact(std::uint32_t draw_back, std::uint32_t scatter_back, bool confined, bool send_keeper) {
    c.set(3, actor.match());
    c.set(19, c.u8(c.r(3) + 0x7920));
    const auto clock = std::int32_t(std::int8_t(c.r(19)));
    c.set(0, std::uint32_t(clock));
    if (compare(c, clock) == 0) c.set(20, 0);
    else {
        refill(draw_back);
        c.set({{3, 65535}, {0, std::uint32_t(clock)}});
        compare(c, clock, 65535);
        c.set(3, std::uint32_t(-65535));
        compare(c, clock, -65535);
        // This sampler's bound is always a sign-extended match clock byte.
        c.set(5, c.u32(c.r(30)));
        c.set(3, std::uint32_t(clock));
        c.set(4, c.u32(c.r(28)));
        compare(c, clock);
        c.set(0, c.u16(c.r(5)));
        c.set(4, c.r(0) + c.r(4));
        const auto remainder = std::uint32_t(std::int64_t(std::int32_t(c.r(4))) % clock);
        c.set(0, clock > 0 ? remainder : 0u - remainder);
        c.set(20, c.r(0));
    }
    c.set({{0, c.r(20) + 1}, {3, actor.match()}, {4, c.u8(actor.address + 0x105)}});
    if (send_keeper) c.set(10, actor.address);
    c.set({{5, c.u8(actor.address + 0x106)}, {6, std::uint32_t(std::int8_t(c.r(0)))},
           {7, actor.side()}, {8, unsigned(confined)}, {9, 1}});
    if (!send_keeper) c.set(10, 0);
    c.call(0x101f1100, scatter_back);
}

void CellPressure::deflect_unsettled_contact() {
    credit_contact(10);
    if (keeper_defends_goal(0x1023bf50)) {
        announce_actor(0x20d4, 0x1023c25c);
        c.set(4, actor.match());
        c.set(3, c.u32(c.r(4) + 0x3108));
        c.set(0, c.r(3) - 300);
        c.put32(c.r(4) + 0x3108, c.r(0));
        if (!sanction_draw_zero(0x1023c28c, 10, 0x66666667, 2))
            scatter_after_contact(0x1023c300, 0x1023c480, true, false);
        else if (!sanction_draw_zero(0x1023c4a4, 200, 0x51eb851f, 6))
            scatter_after_contact(0x1023c518, 0x1023c698, false, false);
        else scatter_after_contact(0x1023c6d4, 0x1023c854, false, true);
    } else {
        c.set(0, std::uint32_t(std::int8_t(c.r(24))));
        const bool forceful = compare(c, std::int32_t(c.r(0)), 2) > 0;
        announce_actor(forceful ? 0x1f71 : 0x213e, forceful ? 0x1023c03c : 0x1023c068);
        c.set(4, actor.match());
        c.set(3, c.u32(c.r(4) + 0x3108));
        c.set(0, c.r(3) - 300);
        c.put32(c.r(4) + 0x3108, c.r(0));
        scatter_after_contact(0x1023c0b0, 0x1023c230, false, true);
    }
    adjust_contact_count(0x1023c874, 1);
    c.put16(actor.address + 0x19c, c.r(23));
}

void CellPressure::measure_recovery_rate(unsigned attempts, unsigned successes) {
    c.set(0, c.u8(actor.address + attempts));
    if (compare_logical(c, c.r(0)) == 0) {
        c.set_f(31, c.f32(c.r(21)));
        return;
    }
    c.set({{5, c.u8(actor.address + successes)}, {4, 0x43300000}, {3, c.u8(actor.address + attempts)},
           {0, 0x43300000}});
    c.put32(frame.at(0x74), c.r(5));
    c.set_f(2, c.f64(c.r(27) + 0xf0));
    c.put32(frame.at(0x70), c.r(4));
    c.set_f(1, c.f64(c.r(27) + 0xf0));
    c.set_f(0, c.f64(frame.at(0x70)));
    c.put32(frame.at(0x7c), c.r(3));
    c.set_f(2, fp.fsubs(c.f(0), c.f(2)));
    c.put32(frame.at(0x78), c.r(0));
    c.set_f(0, c.f64(frame.at(0x78)));
    c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
    c.set_f(31, fp.fdivs(c.f(2), c.f(0)));
}

bool CellPressure::recovery_announced(bool goalward) {
    refill(goalward ? 0x1023cabc : 0x1023cc20);
    c.set(3, c.u32(c.r(30)));
    c.set(4, 0x66666667);
    c.set(0, c.u32(c.r(28)));
    c.set(5, c.u16(c.r(3)));
    c.set(3, std::uint32_t(std::int8_t(c.r(18))));
    c.set(5, c.r(5) + c.r(0));
    const auto scaled = shifted_product(c.r(5), std::int32_t(c.r(4)), 3);
    c.set(4, c.r(5) - 20u * std::uint32_t(scaled + (scaled < 0)));
    c.set(0, std::uint32_t(std::int8_t(c.r(4) + (goalward ? 55 : 65))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) <= 0) return false;
    c.set(0, std::uint32_t(std::int8_t(actor.side())));
    const bool first_side = compare(c, std::int32_t(c.r(0))) == 0;
    c.set(3, actor.match() + 0x10000);
    c.set(0, std::uint32_t(c.i8(c.r(3) - (first_side ? 0x1f8d : 0x1fc3))));
    if (compare(c, std::int32_t(c.r(0))) != 0) return false;
    c.set_f(0, c.f64(c.r(27) + 0x288));
    fp.fcmp(0, c.f(31), c.f(0));
    if (!(c.cr(0) & 4)) return false;
    c.set(0, c.u8(actor.address + (goalward ? 8 : 6)));
    if (compare_logical(c, c.r(0), goalward ? 3 : 1) <= 0) return false;
    if (goalward) {
        c.set(0, c.u8(actor.address + 9));
        if (compare_logical(c, c.r(0)) == 0) return false;
    }
    c.set(0, std::uint32_t(c.i16(actor.address + 0x36)));
    if (compare(c, std::int32_t(c.r(0)), 0x1a5e) <= 0) return false;
    c.set(0, std::uint32_t(c.i8(actor.address + 0x100)));
    return compare(c, std::int32_t(c.r(0))) == 0;
}

void CellPressure::announce_recovery(bool goalward) {
    c.set({{3, actor.match()}, {4, goalward ? 0x21ceu : 0x21cdu}, {5, actor.match()}, {10, 0}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{6, c.u8(actor.address + 0x2f)}, {7, actor.side()},
           {8, c.u8(actor.address + 0x2f)}, {9, actor.side()}});
    c.call(0x10201180, goalward ? 0x1023cba4 : 0x1023ccfc);
    c.set(0, 1);
    c.put8(actor.address + 0x100, c.r(0));
}

void CellPressure::win_unsettled_contact() {
    credit_contact(28);
    c.set(3, c.u8(actor.address + 0x2c));
    c.set(4, actor.address);
    c.set(0, c.r(3) + 1);
    c.put8(actor.address + 0x2c, c.r(0));
    c.set(3, actor.match());
    c.call(0x101e3d50, 0x1023c8f0);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    bool keeper_here = false;
    if (compare(c, std::int32_t(c.r(0)), 1) == 0) {
        c.set(0, actor.side());
        const bool away = compare(c, std::int32_t(c.r(0)), 1) == 0;
        c.set(0, unsigned(inside_goal_region(away, 0, 3)));
        keeper_here = compare(c, std::int32_t(c.r(0))) != 0;
    }
    if (keeper_here) announce_actor(0x1fe2, 0x1023ca10);
    else {
        c.set(3, actor.match());
        c.set(0, std::uint32_t(c.i8(c.r(3) + 0x791f)));
        if (compare(c, std::int32_t(c.r(0)), 4) < 0) announce_actor(0x1f5f, 0x1023c9e4);
    }
    c.set(3, actor.address);
    c.call(0x1022e3a0, 0x1023ca18);
    c.put16(actor.address + 0x19c, c.r(23));

    c.set(0, std::uint32_t(std::int16_t(c.r(23))));
    c.set(3, 0x2e8ba2e9);
    c.set(4, actor.match());
    const bool goalward = compare(c, std::int32_t(c.r(0)), 21) == 0;
    const auto scaled = shifted_product(std::uint32_t(c.i16(c.r(4) + 0x7948)), std::int32_t(c.r(3)), 1);
    c.set(3, unsigned(scaled < 0));
    c.set(0, std::uint32_t(scaled + (scaled < 0)));
    c.set(18, std::uint32_t(std::int8_t(c.r(0))));
    measure_recovery_rate(goalward ? 7 : 4, goalward ? 8 : 5);
    if (recovery_announced(goalward)) announce_recovery(goalward);
    c.set(3, 1);
}

void CellPressure::finish_unsettled_contact() {
    if (wins_unsettled_contact()) win_unsettled_contact();
    else deflect_unsettled_contact();
}
}
