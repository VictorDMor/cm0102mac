#include "match_pl_execute_ball_action_detail.hpp"

namespace cm::aot::match_pl::ball_action_detail {
using namespace guest;

static std::uint32_t signed_mix_bit(std::uint32_t trait, std::uint32_t draw) {
    const auto mixed = trait ^ draw;
    return (std::uint32_t(std::int32_t(mixed) >> 1) - (mixed & trait)) >> 31;
}

static std::uint32_t receiver_interval(Cpu& c) {
    const auto first = signed_mix_bit(signed_byte(c.r(13)), c.r(17));
    const auto second = signed_mix_bit(signed_byte(c.r(16)), c.r(18));
    c.s.wr<1>(reg::xer_ca, 1);
    return 3 - first - second;
}

static void finish_receiver_random_refill(Cpu& c, std::uint32_t back) {
    if (!(c.cr(0) & 4)) return;
    c.put32(c.r(30), c.r(28));
    c.call(0x1077b770, back);
    c.set(0, c.r(3) & 0xffff);
    c.put32(c.r(27), c.r(0));
}

static void draw_receiver_traits(Cpu& c, std::uint32_t recipient) {
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(29)));
    c.set(3, c.r(3) + 2);
    c.set(13, c.u8(recipient + 0x10f));
    c.set(16, c.u8(recipient + 0x110));
    c.compare_logical(0, c.r(3), c.r(0));
    c.put32(c.r(30), c.r(3));
    finish_receiver_random_refill(c, 0x10243cc8);

    c.set(5, c.u32(c.r(30)));
    c.set(4, 0x66660000);
    c.set(3, c.u32(c.r(30)));
    c.set(4, 0x66666667);
    c.set(6, c.u16(c.r(5)));
    c.set(5, c.u32(c.r(27)));
    c.set(3, c.r(3) + 2);
    c.set(0, c.u32(c.r(29)));
    const auto first = c.r(6) + c.r(5);
    c.set(5, first);
    c.put32(c.r(30), c.r(3));
    c.set(4, high_product(first, 0x66666667));
    c.compare_logical(0, c.r(3), c.r(0));
    const auto quotient = shift_signed_with_carry(c, c.r(4), 2);
    c.set(3, quotient >> 31);
    c.set(0, truncated_interval(quotient) * 10);
    c.set(17, first - c.r(0));
    finish_receiver_random_refill(c, 0x10243d20);

    c.set(3, c.u32(c.r(30)));
    c.set(4, 0x66660000);
    c.set(5, c.u32(c.r(27)));
    c.set(4, 0x66666667);
    c.set(7, c.u16(c.r(3)));
    c.set(0, signed_byte(c.r(13)));
    c.set(3, c.r(0) ^ c.r(17));
    c.set(6, signed_byte(c.r(16)));
    const auto second = c.r(7) + c.r(5);
    c.set(7, second);
    c.set(4, high_product(second, 0x66666667));
    c.set(0, c.r(3) & c.r(0));
    c.set(4, shift_signed_with_carry(c, c.r(4), 2));
    c.set(5, c.r(4) >> 31);
    c.set(4, truncated_interval(c.r(4)) * 10);
    c.set(18, second - c.r(4));
    c.set(4, c.r(6) ^ c.r(18));
    c.set(5, shift_signed_with_carry(c, c.r(4), 1));
    c.set(3, shift_signed_with_carry(c, c.r(3), 1));
    c.set(4, c.r(4) & c.r(6));
    c.set(3, (c.r(5) - c.r(4)) >> 31);
    c.set(0, receiver_interval(c));
    c.compare(0, std::int32_t(c.r(0)), 0);
}

static void draw_receiver_counter(Cpu& c) {
    // Each mixed term contributes only its sign bit. The original
    // interval is always 1..3, so neither a zero nor a wide draw occurs.
    advance_stream(c, 0x10243dbc);
    const auto interval = receiver_interval(c);
    c.set(3, std::uint32_t(-65535));
    c.set(0, interval);
    c.compare(0, std::int32_t(interval), -65535);

    c.set(5, signed_byte(c.r(16)));
    c.set(4, signed_byte(c.r(13)));
    c.set(3, c.r(5) ^ c.r(18));
    c.set(8, c.u32(c.r(30)));
    c.set(0, c.r(4) ^ c.r(17));
    c.set(6, signed_byte(c.r(16)));
    const auto first_bit = signed_mix_bit(c.r(4), c.r(17));
    const auto second_bit = signed_mix_bit(c.r(6), c.r(18));
    c.set(0, first_bit);
    c.set(8, c.u16(c.r(8)));
    c.set(5, c.u32(c.r(27)));
    c.set(7, second_bit);
    c.set(3, 3 - first_bit);
    c.set(4, c.r(6) ^ c.r(18));
    const auto sample = c.r(8) + c.r(5);
    c.set(0, sample);
    c.set(5, signed_byte(c.r(13)));
    c.set(9, interval);
    c.set(7, shift_signed_with_carry(c, c.r(4), 1));
    const auto quotient = std::uint32_t(divide_signed<4>(sample, interval));
    c.set(8, quotient);
    c.set(3, c.r(5) ^ c.r(17));
    c.set(6, c.r(4) & c.r(6));
    c.set(4, shift_signed_with_carry(c, c.r(3), 1));
    c.set(3, c.r(3) & c.r(5));
    c.set(5, c.r(7) - c.r(6));
    c.set(6, quotient * interval);
    c.set(3, c.r(4) - c.r(3));
    c.set(4, second_bit);
    c.set(3, 3 - first_bit);
    c.s.wr<1>(reg::xer_ca, 1);
    c.compare(0, std::int32_t(interval), 0);
    c.set(0, sample - c.r(6));
    c.set(19, c.r(0));
}

static void normalize_receiver_position(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + actor_match));
    c.set(0, std::uint32_t(c.i8(c.r(3) + ball_row)));
    c.compare(0, std::int32_t(c.r(0)), 4);
    if (!(c.cr(0) & 8)) return;
    c.set(3, c.u32(actor + actor_match));
    c.set(0, std::uint32_t(c.i8(c.r(3) + ball_row)));
    c.compare(0, std::int32_t(c.r(0)), -1);
    if (c.cr(0) & 4) {
        c.set(3, c.u32(actor + actor_match));
        c.set(0, 0);
        c.put8(c.r(3) + ball_row, 0);
    }
}

void sample_receiver(Cpu& c) {
    c.s.pc = ball_action::receiver_sampling_phase;
    const auto recipient = c.r(recipient_register);
    draw_receiver_traits(c, recipient);
    draw_receiver_counter(c);
    c.put8(recipient + 0x2c, c.r(19));
    c.set(17, 1);
    c.s.pc = ball_action::receiver_effects_phase;
}

void handle_marker(Cpu& c) {
    c.s.pc = ball_action::receiver_marker_phase;
    c.set(0, std::uint32_t(c.i8(c.sp() + 0xd0)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) {
        c.set(3, c.r(actor_register));
        call_owned(c, 0x10288230, 0x102440d4);
    }
    c.s.pc = ball_action::receiver_effects_phase;
}

void position_receiver(Cpu& c) {
    c.s.pc = ball_action::receiver_position_phase;
    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    normalize_receiver_position(c, actor);
    c.set(0, std::uint32_t(c.i8(recipient + actor_side)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    bool home = c.cr(0) & 2;
    if (home) {
        c.set(3, c.u32(actor + actor_match));
        c.set(0, std::uint32_t(c.i8(c.r(3) + ball_row)));
        c.compare(0, std::int32_t(c.r(0)), -1);
        home = (c.cr(0) & 2) == 0;
    }
    const auto row = c.i8(actor + actor_row);
    c.set(0, std::uint32_t(row));
    c.set(3, recipient);
    c.set(4, c.u8(actor + actor_column));
    c.set(6, 4);
    c.set(5, c.u8(actor + actor_row));
    c.compare(0, row, home ? 9 : 3);
    if (home ? row >= 9 : row <= 3) c.set(0, home ? 11 : 0);
    else {
        c.set(7, std::uint32_t(c.i8(actor + actor_row)));
        c.set(0, c.r(7) + (home ? 3u : -3u));
    }
    c.set(7, signed_byte(c.r(0)));
    call_owned(c, 0x10237830, home ? 0x10244168 : 0x102441a8);
    c.s.pc = ball_action::receiver_assessment_phase;
}

static void receiver_performance_ratio(Cpu& c, std::uint32_t recipient, unsigned denominator, unsigned numerator) {
    const auto stack = c.sp();
    Fpu fp(c);
    c.set(0, c.u8(recipient + denominator));
    c.compare_logical(0, c.r(0), 0);
    if (c.cr(0) & 2) {
        c.set_f(29, c.f32(c.r(31)));
        return;
    }
    constexpr std::uint32_t prefix = 0x43300000;
    c.set(5, c.u8(recipient + numerator));
    c.set(4, prefix);
    c.set(3, c.u8(recipient + denominator));
    c.set(0, prefix);
    c.put32(stack + 0x7c, c.r(5));
    c.set_f(2, c.f64(c.r(26) + 0xf0));
    c.put32(stack + 0x78, prefix);
    c.set_f(1, c.f64(c.r(26) + 0xf0));
    c.set_f(0, c.f64(stack + 0x78));
    c.put32(stack + 0x84, c.r(3));
    c.set_f(2, fp.fsubs(c.f(0), c.f(2)));
    c.put32(stack + 0x80, prefix);
    c.set_f(0, c.f64(stack + 0x80));
    c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
    c.set_f(29, fp.fdivs(c.f(2), c.f(0)));
}

static bool club_allows_receiver_report(Cpu& c, std::uint32_t recipient) {
    c.set(0, std::uint32_t(c.i8(recipient + actor_side)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    const auto club_flag = (c.cr(0) & 2) ? 0xe073 : 0xe03d;
    c.set(3, c.u32(recipient + actor_match) + 0x10000);
    c.set(0, std::uint32_t(c.i8(c.r(3) - 0x10000 + club_flag)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) return false;
    Fpu fp(c);
    c.set_f(0, c.f64(c.r(26) + 0x288));
    fp.fcmp(0, c.f(29), c.f(0));
    return c.cr(0) & 4;
}

static void describe_receiver_performance(Cpu& c, std::uint32_t actor, std::uint32_t recipient, bool special) {
    receiver_performance_ratio(c, recipient, special ? 7 : 4, special ? 8 : 5);
    advance_stream(c, special ? 0x1024424c : 0x102443b0);
    c.set(3, c.u32(c.r(30)));
    c.set(4, 0x66660000);
    c.set(0, c.u32(c.r(27)));
    c.set(4, 0x66666667);
    c.set(5, c.u16(c.r(3)));
    c.set(3, signed_byte(c.r(13)));
    const auto sample = c.r(5) + c.r(0);
    c.set(5, sample);
    const auto quotient = shift_signed_with_carry(c, high_product(sample, 0x66666667), 3);
    c.set(4, sample - truncated_interval(quotient) * 20);
    c.set(0, signed_byte(c.r(4) + (special ? 55 : 65)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 4) || !club_allows_receiver_report(c, recipient)) return;

    c.set(0, c.u8(recipient + (special ? 8 : 6)));
    c.compare_logical(0, c.r(0), special ? 3 : 1);
    if (!(c.cr(0) & 4)) return;
    if (special) {
        c.set(0, c.u8(recipient + 9));
        c.compare_logical(0, c.r(0), 0);
        if (c.cr(0) & 2) return;
    }
    c.set(0, std::uint32_t(c.i16(recipient + 0x36)));
    c.compare(0, std::int32_t(c.r(0)), 0x1a5e);
    if (!(c.cr(0) & 4)) return;
    c.set(0, std::uint32_t(c.i8(recipient + 0x100)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) return;
    comment_on_ordered_pair(c, actor, recipient, actor, special ? 0x21ce : 0x21cd,
        special ? 0x10244334 : 0x1024448c);
    c.set(0, 1);
    c.put8(recipient + 0x100, 1);
}

void assess_receiver(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    c.s.pc = ball_action::receiver_assessment_phase;
    c.set(3, c.u32(actor + actor_match));
    c.set(4, 0x2e8c0000);
    c.set(0, c.u8(c.sp() + execution_kind_slot));
    c.set(4, 0x2e8ba2e9);
    c.set(3, std::uint32_t(c.i16(c.r(3) + 0x7948)));
    c.set(0, signed_byte(c.r(0)));
    c.compare(0, std::int32_t(c.r(0)), 21);
    const auto adjusted = shift_signed_with_carry(c, high_product(c.r(3), 0x2e8ba2e9), 1);
    c.set(3, adjusted >> 31);
    c.set(0, truncated_interval(adjusted));
    c.set(13, signed_byte(c.r(0)));
    describe_receiver_performance(c, actor, recipient, c.cr(0) & 2);
    c.s.pc = ball_action::receiver_effects_phase;
}

static void apply_receiver_reaction(Cpu& c, std::uint32_t recipient, bool special) {
    const unsigned counter = special ? 9 : 6;
    c.set(3, c.u8(recipient + counter));
    c.set(0, c.r(3) + 1);
    c.put8(recipient + counter, c.r(0));
    c.set(3, std::uint32_t(c.i16(recipient + 0x36)));
    c.set(0, c.r(3) + 300);
    c.put16(recipient + 0x36, c.r(0));
    c.set(0, std::uint32_t(c.i8(recipient + 0x17)));
    const auto base = c.r(0) * 30;
    const auto interval = base + 70;
    c.set(3, base);
    c.set(16, interval);
    c.s.wr<1>(reg::xer_ca, interval < base);
    c.compare(0, std::int32_t(interval), 0);
    draw_narrow_interval(c, std::int32_t(interval), 13, special ? 0x10244998 : 0x10244668);
    c.compare(0, std::int32_t(c.r(13)), 0);
    if (!(c.cr(0) & 2)) return;

    advance_stream(c, special ? 0x10244afc : 0x102447cc);
    if (special) {
        c.set(4, c.u32(c.r(30)));
        c.set(3, recipient);
        c.set(0, c.u32(c.r(27)));
        c.set(4, c.u16(c.r(4)));
        const auto sample = c.r(4) + c.r(0);
        const auto sign = sample >> 31;
        c.set(0, ((sample & 1u) ^ sign) - sign);
    } else {
        c.set(3, c.u32(c.r(30)));
        c.set(4, 0x55550000u);
        c.set(0, c.u32(c.r(27)));
        c.set(4, 0x55555556);
        c.set(5, c.u16(c.r(3)));
        c.set(3, recipient);
        const auto sample = c.r(5) + c.r(0);
        c.set(5, sample);
        c.set(4, high_product(sample, 0x55555556));
        c.set(0, sample - truncated_interval(c.r(4)) * 3);
    }
    c.set(4, signed_byte(c.r(0)));
    call_owned(c, 0x10288f30, special ? 0x10244b30 : 0x1024480c);
    if (compare_signed_result(c)) {
        c.set(3, recipient);
        c.set(4, 0);
        call_owned(c, 0x1022ba10, special ? 0x10244b44 : 0x10244820);
    }
}

static void add_receiver_rating(Cpu& c, std::uint32_t player, int change) {
    c.set(4, std::uint32_t(c.i16(player + 0x36)));
    c.set(0, c.r(4) + change);
    c.put16(player + 0x36, c.r(0));
}

static void reward_regular_receiver(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    c.set(4, c.u8(recipient + 5));
    c.set(3, recipient);
    c.set(0, c.r(4) + 1);
    c.put8(recipient + 5, c.r(0));
    call_owned(c, 0x10289560, 0x102444e4);
    c.set(0, c.r(3) & 4);
    c.compare(0, std::int32_t(c.r(0)), 0);
    bool single = (c.cr(0) & 2) == 0;
    if (!single) {
        c.set(3, recipient);
        call_owned(c, 0x10289560, 0x102444f4);
        c.set(0, c.r(3) & 2);
        c.compare(0, std::int32_t(c.r(0)), 0);
        single = (c.cr(0) & 2) == 0;
    }
    c.set(0, signed_byte(c.r(17)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    c.set(3, std::uint32_t(c.i16(recipient + 0x36)));
    const auto bonus = (c.cr(0) & 2) ? 200u : 120u;
    c.set(0, c.r(3) + (single ? bonus : 2 * bonus));
    c.put16(recipient + 0x36, c.r(0));

    c.set(3, recipient);
    call_owned(c, 0x10270ba0, 0x1024454c);
    if (compare_signed_result(c) && aligns_with_goal(c, actor, 0x102445b4, 0x1024460c))
        apply_receiver_reaction(c, recipient, false);
}

static void reward_special_receiver(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    c.set(0, 123);
    c.set(3, recipient);
    c.put16(recipient + 0x19c, 123);
    add_player_counter(c, recipient, 7, 1);
    add_player_counter(c, recipient, 8, 1);
    add_receiver_rating(c, recipient, -10);
    add_receiver_rating(c, recipient, 50);
    add_player_counter(c, actor, 7, 1);
    add_receiver_rating(c, actor, -10);
    call_owned(c, 0x10270ba0, 0x1024487c);
    if (compare_signed_result(c) && aligns_with_goal(c, actor, 0x102448e4, 0x1024493c))
        apply_receiver_reaction(c, recipient, true);
}

void complete_receiver_effects(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    c.s.pc = ball_action::receiver_effects_phase;
    c.set(4, c.u8(actor + 0x2c));
    c.set(0, signed_byte(c.r(17)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    c.set(3, c.u8(c.sp() + 0x47));
    c.set(0, 200);
    c.set(3, c.r(4) + c.r(3));
    c.put8(actor + 0x2c, c.r(3));
    c.set(4, std::uint32_t(c.i16(recipient + 0x36)));
    const auto bonus = (c.cr(0) & 2) ? 200u : 120u;
    c.set(3, bonus);
    c.set(0, signed_byte(c.r(22)));
    c.set(3, c.r(4) + bonus);
    c.compare(0, std::int32_t(c.r(0)), 4);
    c.put16(recipient + 0x36, c.r(3));
    if (c.cr(0) & 2) reward_special_receiver(c, actor, recipient);
    else reward_regular_receiver(c, actor, recipient);
    c.s.pc = ball_action::receiver_final_reaction_phase;
}

void react_to_receiver(Cpu& c) {
    c.s.pc = ball_action::receiver_final_reaction_phase;
    c.set(0, signed_byte(c.r(17)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) {
        advance_stream(c, 0x10244b6c);
        c.set(4, c.u32(c.r(30)));
        c.set(3, 0x92490000u);
        c.set(0, c.u32(c.r(27)));
        c.set(3, 0x92492493u);
        const auto sample = c.u16(c.r(4)) + c.r(0);
        c.set(4, sample);
        const auto quotient = shift_signed_with_carry(c,
            high_product(sample, std::int32_t(0x92492493u)) + sample, 2);
        c.set(3, quotient >> 31);
        c.set(0, sample - truncated_interval(quotient) * 7);
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (c.cr(0) & 2)
            comment_on_player(c, c.r(actor_register), c.r(recipient_register), 0x1f41, 0x10244bd4);
    }
    c.s.pc = ball_action::return_phase;
}

void report_missing_receiver(Cpu& c) {
    const auto stack = c.sp();
    c.s.pc = ball_action::missing_receiver_phase;
    c.set(3, c.toc(-0x51e4));
    c.set({{6, stack + 0x58}, {7, stack + 0x50}, {4, 0}});
    c.set(3, c.r(3) + 0x8f);
    c.set(5, 0);
    c.call(0x10739890, 0x10244bf4);
    c.call(0x10768810, 0x10244bf8);
    c.set(4, c.toc(-0x51e4));
    c.set(5, c.r(3));
    c.set(3, c.toc(-0x7ba8));
    c.set(6, stack + 0x58);
    c.set(4, c.r(4) + 0x9c);
    c.set(7, stack + 0x50);
    c.set(8, 0x1ee6);
    c.call(0x1077b720, 0x10244c18);
    c.set(3, c.toc(-0x51e4));
    c.set(5, 0);
    c.set(4, c.toc(-0x7ba8));
    c.set(3, c.r(3) + 0xa9);
    c.call(0x10769b60, 0x10244c2c);
    c.set(3, c.toc(-0x7bac));
    c.set(0, 0);
    c.put8(c.r(3), 0);
    c.s.pc = ball_action::return_phase;
}
}
