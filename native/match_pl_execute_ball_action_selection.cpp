#include "match_pl_execute_ball_action_detail.hpp"

namespace cm::aot::match_pl::ball_action_detail {
using namespace guest;

static bool can_clear_lane(Cpu& c, std::uint32_t actor) {
    const bool opposite_area = c.i8(actor + actor_side) == 0;
    c.set(0, opposite_area);
    c.compare(0, opposite_area, 1);
    check_area_coordinates(c, actor, opposite_area, 0, 3, 0);
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) return false;

    const auto reach = c.i8(actor + 0x10f);
    const auto action = std::int16_t(c.r(16));
    c.set(3, std::uint32_t(action));
    const auto threshold = subtract_from(c, std::uint32_t(reach), 20) * 7;
    c.set(0, threshold);
    c.compare(0, action, std::int32_t(threshold));
    if (!(c.cr(0) & 4)) return false;

    c.set(3, c.u32(actor + actor_match));
    c.set(4, c.u8(actor + actor_side));
    c.set(5, c.u8(actor + actor_column));
    c.set(6, c.u8(actor + actor_row));
    c.call(0x101e4ed0, 0x1023d9f0);
    c.set(0, signed_byte(c.r(3)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) == 0;
}

static void select_clearance(Cpu& c, std::uint32_t actor) {
    constexpr unsigned clearance = 12;
    c.set(13, clearance);
    c.set(3, c.u8(actor + 0x10f));
    c.set(0, clearance);
    c.set(5, c.u32(actor + actor_match));
    c.set(0, clearance * 2);
    c.set(4, signed_byte(c.r(3)));
    c.set(3, actor + c.r(0));
    c.set(23, 1);
    c.set(3, std::uint32_t(c.i16(c.r(3) + 0x11e)));
    c.set(0, 2);
    c.set(4, subtract_from(c, c.r(4), 20));
    c.put8(c.r(5) + ball_row, c.r(0));
    c.set(4, c.r(4) * 10);
    c.set(5, c.u32(actor + actor_match));
    c.set(0, c.r(3) - c.r(4));
    c.set(22, c.u8(c.r(5) + ball_row));
    c.set(17, signed_half(c.r(0)));
}

static void choose_kind(Cpu& c, std::uint32_t actor, unsigned kind) {
    c.set(13, kind);
    c.set(0, kind * 2);
    c.set(3, actor + c.r(0));
    c.set(17, std::uint32_t(c.i16(c.r(3) + 0x11e)));
}

static void sample_elevated_preference(Cpu& c, std::uint32_t actor) {
    const auto side = c.u8(actor + actor_side);
    c.set(0, side);
    c.compare(0, side, 1);
    check_area_coordinates(c, actor, side == 1, 0, 3, 13);
    c.set(3, actor);
    c.set(4, 0);
    call_owned(c, 0x102875f0, 0x1023daf0);
    const auto preference = std::int8_t(c.r(3));
    c.set(17, std::uint32_t(preference));
    c.compare(0, preference, 0);
    draw_narrow_interval(c, preference, 14, 0x1023db20);
}

static void sample_elevated_threshold(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u8(actor + 0x24));
    c.set(0, signed_byte(c.r(13)) * 1500);
    c.set(3, signed_byte(c.r(3)) * 80);
    const auto base = c.r(0) + c.r(3);
    const auto interval = base + 70;
    c.s.wr<1>(reg::xer_ca, interval < base);
    c.set(17, interval);
    c.compare(0, std::int32_t(interval), 0);
    // Area flag (0/1) and the signed byte limit this to a narrow
    // interval. The fixed +70 also prevents zero in this phase.
    draw_narrow_interval(c, std::int32_t(interval), 13, 0x1023dca4);
}

void select_elevated(Cpu& c) {
    const auto actor = c.r(actor_register);
    c.s.pc = ball_action::ball_state_selection_phase;
    const auto height = std::int8_t(c.r(22));
    c.set(0, std::uint32_t(height));
    c.compare(0, height, 4);
    if (height < 4) {
        c.s.pc = ball_action::low_ball_selection_phase;
        return;
    }

    if (can_clear_lane(c, actor)) select_clearance(c, actor);
    else {
        sample_elevated_preference(c, actor);
        sample_elevated_threshold(c, actor);
        c.compare(0, std::int32_t(c.r(14)), std::int32_t(c.r(13)));
        choose_kind(c, actor, (c.cr(0) & 4) ? 46 : 21);
    }
    c.s.pc = ball_action::selected_action_phase;
}

std::uint32_t high_product(std::uint32_t value, std::int32_t multiplier) {
    return std::uint32_t((std::int64_t(std::int32_t(value)) * multiplier) >> 32);
}

std::uint32_t shift_signed_with_carry(Cpu& c, std::uint32_t value, unsigned bits) {
    c.s.wr<1>(reg::xer_ca, std::int32_t(value) < 0 && (value & ((1u << bits) - 1)) != 0);
    return std::uint32_t(std::int32_t(value) >> bits);
}

static void sample_wide_interval(Cpu& c, std::uint32_t bound, unsigned result, std::uint32_t service_back) {
    constexpr std::uint32_t double_prefix = 0x43300000;
    constexpr std::int32_t reciprocal = std::int32_t(0x80008001u);
    const auto stack = c.sp();
    Fpu fp(c);

    c.set(3, bound ^ 0x80000000u);
    c.set(0, double_prefix);
    c.put32(stack + 0x7c, c.r(3));
    c.set(6, 0x80010000);
    c.set(7, c.u32(c.r(30)));
    c.set(4, double_prefix);
    c.put32(stack + 0x78, double_prefix);
    c.set(5, 0x10000);
    c.set_f(2, c.f64(c.r(26) + 0x80));
    c.set(3, 0x80010000);
    c.set_f(1, c.f64(stack + 0x78));
    c.set(6, std::uint32_t(reciprocal));
    c.set_f(0, c.f64(c.r(26)));
    c.set(5, 65535);
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set(8, c.u16(c.r(7)));
    c.set(7, c.u32(c.r(27)));
    c.set(0, bound * 10);
    c.set(3, std::uint32_t(reciprocal));
    c.put32(stack + 0x80, double_prefix);
    c.set_f(2, fp.fdiv(c.f(1), c.f(0)));

    const auto sample = c.r(8) + c.r(7);
    c.set(7, sample);
    c.set(4, high_product(sample, reciprocal));
    c.set_f(1, c.f64(c.r(26) + 0x80));
    const auto quotient = shift_signed_with_carry(c, c.r(4) + sample, 15);
    c.set(6, quotient >> 31);
    c.set(4, truncated_interval(quotient) * 65535u);
    c.set(result, sample - c.r(4));
    c.set(4, c.r(result) ^ 0x80000000u);
    c.put32(stack + 0x84, c.r(4));
    c.set(3, high_product(c.r(0), reciprocal));
    c.set_f(0, c.f64(stack + 0x80));
    c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.set(0, shift_signed_with_carry(c, c.r(3) + c.r(0), 15));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    c.set(3, truncated_interval(c.r(0)));
    c.put64(stack + 0x88, fp.fctiwz(0, c.f(0)));
    c.set(result, c.u32(stack + 0x8c));
    call_owned(c, 0x10226520, service_back);

    c.set(4, 0x66666667);
    c.set(3, c.r(3) + 5);
    c.set(0, bound - 1);
    const auto correction = shift_signed_with_carry(c, high_product(c.r(3), 0x66666667), 2);
    c.set(4, correction >> 31);
    c.set(3, truncated_interval(correction));
    const auto value = c.r(result) + c.r(3);
    c.set(result, value);
    c.compare(0, std::int32_t(value), std::int32_t(c.r(0)));
    if (c.cr(0) & 4) c.set(result, c.r(0));
}

static void draw_converted_interval(Cpu& c, std::int32_t interval, unsigned result,
                             std::uint32_t refill_back, std::uint32_t service_back) {
    if (interval == 0) {
        c.set(result, 0);
        return;
    }
    advance_stream(c, refill_back);
    if (interval < -65535 || interval > 65535) {
        sample_wide_interval(c, std::uint32_t(interval), result, service_back);
        return;
    }
    c.set(3, std::uint32_t(-65535));
    c.compare(0, interval, -65535);
    sample_narrow_interval(c, interval, result);
}

bool aligns_with_goal(Cpu& c, std::uint32_t actor, std::uint32_t home_back, std::uint32_t away_back) {
    const auto side = c.i8(actor + actor_side);
    c.set(0, std::uint32_t(side));
    c.compare(0, side, 0);
    const auto row = c.u8(actor + actor_row);
    const auto goal_row = side == 0 ? 11u : 0u;
    const auto compared_row = side == 0 ? int(row) : int(std::int8_t(row));
    c.set(0, std::uint32_t(compared_row));
    c.compare(0, compared_row, goal_row);
    bool goal_cell = c.cr(0) & 2;
    if (goal_cell) {
        const auto col = c.i8(actor + actor_column);
        c.set(0, std::uint32_t(col));
        c.compare(0, col, 3);
        if (col >= 3) {
            const auto upper_col = c.i8(actor + actor_column);
            c.set(0, std::uint32_t(upper_col));
            c.compare(0, upper_col, 5);
            goal_cell = upper_col <= 5;
        } else goal_cell = false;
    }
    if (goal_cell) c.set(3, 1);
    else {
        c.set_f(1, c.f32(c.r(31)));
        c.set(3, actor);
        c.set_f(2, c.f32(c.r(31)));
        c.set({{4, 4}, {5, goal_row}, {6, 0}, {9, 0}});
        call_owned(c, 0x10244c60, side == 0 ? home_back : away_back);
    }
    c.set(0, signed_byte(c.r(3)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) == 0;
}

static bool wins_low_ball_competition(Cpu& c, std::uint32_t actor) {
    Fpu fp(c);
    c.set_f(0, c.f64(c.r(26) + 0x328));
    fp.fcmp(0, c.f(29), c.f(0));
    if (!(c.cr(0) & 8)) return false;

    const auto first = c.i16(actor + 0x194);
    c.set(14, std::uint32_t(first));
    c.compare(0, first, 0);
    draw_narrow_interval(c, first, 13, 0x1023df30);
    const auto second = c.i16(actor + 0x136);
    c.set(17, std::uint32_t(second));
    c.compare(0, second, 0);
    draw_narrow_interval(c, second, 14, 0x1023e0a0);
    c.compare(0, std::int32_t(c.r(13)), std::int32_t(c.r(14)));
    return c.cr(0) & 4;
}

static bool attempts_low_ball(Cpu& c, std::uint32_t actor) {
    Fpu fp(c);
    const auto stack = c.sp();
    c.put64(stack + 0x78, fp.fctiwz(0, c.f32(actor + 0x9c)));
    const auto interval = c.i32(stack + 0x7c);
    c.set(14, std::uint32_t(interval));
    c.compare(0, interval, 0);
    draw_converted_interval(c, interval, 13, 0x1023e224, 0x1023e2fc);

    advance_stream(c, 0x1023e380);
    c.set(4, c.u32(c.r(30)));
    c.set(3, 0x66660000);
    c.set(0, c.u32(c.r(27)));
    c.set(3, 0x66666667);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto quotient = shift_signed_with_carry(c, high_product(sample, 0x66666667), 1);
    c.set(3, quotient >> 31);
    c.set(0, sample - truncated_interval(quotient) * 5);
    c.compare(0, std::int32_t(c.r(13)), std::int32_t(c.r(0)));
    return (c.cr(0) & 8) == 0;
}

void select_low(Cpu& c) {
    const auto actor = c.r(actor_register);
    c.s.pc = ball_action::low_ball_selection_phase;
    const auto height = std::int8_t(c.r(22));
    c.set(0, std::uint32_t(height));
    c.compare(0, height, 3);
    if (height != 3) c.compare(0, height, 2);
    if (height != 2 && height != 3) choose_kind(c, actor, 13);
    else {
        const bool attempt = aligns_with_goal(c, actor, 0x1023de90, 0x1023dee8) && wins_low_ball_competition(c, actor) &&
            attempts_low_ball(c, actor);
        choose_kind(c, actor, attempt ? 59 : 12);
    }
    c.s.pc = ball_action::selected_action_phase;
}

static void sample_adjustment(Cpu& c, unsigned source, unsigned result, std::uint32_t refill_back) {
    const auto interval = std::int16_t(c.r(source));
    c.set(0, std::uint32_t(interval));
    c.compare(0, interval, 0);
    if (interval == 0) {
        c.set(result, 0);
        return;
    }

    advance_stream(c, refill_back);
    // The original bounds are wider than the signed-halfword input.
    c.set(3, std::uint32_t(-65535));
    c.set(0, std::uint32_t(interval));
    c.compare(0, interval, -65535);
    c.set(5, c.u32(c.r(30)));
    c.set(3, std::uint32_t(interval));
    c.set(4, c.u32(c.r(27)));
    c.compare(0, interval, 0);
    c.set(0, c.u16(c.r(5)));
    const auto sample = c.r(0) + c.r(4);
    c.set(4, sample);
    const auto bound = std::uint32_t(interval);
    const auto remainder = sample - std::uint32_t(divide_signed<4>(sample, bound)) * bound;
    c.set(0, interval > 0 ? remainder : 0u - remainder);
    c.set(result, c.r(0));
}

void sample_selected(Cpu& c) {
    c.s.pc = ball_action::selected_action_phase;
    sample_adjustment(c, 17, 14, 0x1023e434);
    sample_adjustment(c, 16, 17, 0x1023e5bc);
    sample_adjustment(c, 21, 18, 0x1023e744);
    sample_adjustment(c, 20, 19, 0x1023e8cc);

    c.set(0, c.r(14) + c.r(17));
    c.set_f(1, c.f32(c.r(31) + 0xe0));
    c.set(0, c.r(0) - c.r(18));
    c.set(3, c.r(actor_register));
    c.set(0, c.r(0) - c.r(19));
    c.set(5, signed_byte(c.r(13)));
    c.set(17, signed_half(c.r(0)));
    call_owned(c, 0x10239590, ball_action::contact_selection_phase);
    c.s.pc = ball_action::contact_selection_phase;
}
}
