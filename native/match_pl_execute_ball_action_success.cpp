#include "match_pl_execute_ball_action_detail.hpp"

namespace cm::aot::match_pl::ball_action_detail {
using namespace guest;

static void mark_unopposed_execution(Cpu& c, std::uint32_t actor) {
    const auto stack = c.sp();
    c.set(0, std::uint32_t(c.i8(stack + execution_marker_slot)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) return;
    const bool opposing_area = c.i8(actor + actor_side) == 0;
    c.set(0, opposing_area);
    c.compare(0, opposing_area, 1);
    check_area_coordinates(c, actor, opposing_area, 0, 3, 0);
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) {
        c.set(0, std::uint32_t(c.i8(stack + execution_blocked_slot)));
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (c.cr(0) & 2) return;
    }
    c.set(0, 1);
    c.put8(stack + 0xd0, 1);
}

void complete_unopposed(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    c.s.pc = ball_action::unopposed_result_phase;
    mark_unopposed_execution(c, actor);
    c.set(3, c.u8(c.sp() + execution_kind_slot));
    c.set(0, signed_byte(c.r(22)));
    c.compare(0, std::int32_t(c.r(0)), 4);
    c.set(0, signed_byte(c.r(3)));
    c.put16(recipient + 0x19c, c.r(0));
    if (!(c.cr(0) & 2)) {
        c.set(3, c.u8(recipient + 4));
        c.set(4, recipient);
        c.set(0, c.r(3) + 1);
        c.put8(recipient + 4, c.r(0));
        c.set(3, c.u32(actor + actor_match));
        c.call(0x101e3d50, 0x102425b8);
        c.set(0, c.r(3) & 0x30);
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (!(c.cr(0) & 2)) {
            c.set(3, c.u32(actor + actor_match));
            c.set(4, recipient);
            c.call(0x101e3d50, 0x102425cc);
            c.set(0, signed_half(c.r(3)) & 0x880);
            c.compare(0, std::int32_t(c.r(0)), 0);
        }
        const auto loss = (c.cr(0) & 2) ? 70 : 59;
        c.set(3, std::uint32_t(c.i16(recipient + 0x36)));
        c.set(0, c.r(3) - loss);
        c.put16(recipient + 0x36, c.r(0));
    }
    c.s.pc = ball_action::no_receiver_phase;
}

void prepare_final(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    const auto stack = c.sp();
    c.s.pc = ball_action::no_receiver_phase;
    c.compare_logical(0, recipient, 0);
    if (recipient != 0) {
        c.set(0, std::uint32_t(c.i16(stack + execution_rating_slot)));
        c.set(3, signed_half(c.r(17)));
        c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
        if (!(c.cr(0) & 4)) {
            c.s.pc = ball_action::unsuccessful_action_phase;
            return;
        }
    }

    c.set(3, actor);
    call_owned(c, 0x1022e3a0, 0x10242614);
    constexpr std::uint32_t prefix = 0x43300000;
    constexpr std::uint32_t sign_flip = 0x80000000u;
    Fpu fp(c);
    c.set(0, signed_half(c.r(17)));
    c.set(4, prefix);
    c.set(3, c.r(0) ^ sign_flip);
    c.put32(stack + 0x7c, c.r(3));
    c.set(0, prefix);
    c.set(5, signed_byte(c.r(13)));
    c.put32(stack + 0x78, prefix);
    c.compare_logical(0, recipient, 0);
    c.set_f(2, c.f64(c.r(26) + 0x80));
    c.set_f(1, c.f64(stack + 0x78));
    c.set_f(0, c.f64(c.r(26) + 0x330));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.put16(actor + 0x19c, c.r(5));
    c.set_f(2, c.f64(c.r(26) + 0x80));
    c.put32(stack + 0x8c, c.r(3));
    c.set_f(3, fp.fdiv(c.f(1), c.f(0)));
    c.set(3, std::uint32_t(c.i16(actor + 0x36)));
    c.put32(stack + 0x88, prefix);
    c.set(0, c.r(3) + 4);
    c.set_f(0, c.f64(c.r(26) + 0x330));
    c.set_f(1, c.f64(stack + 0x88));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.put16(actor + 0x36, c.r(0));
    const auto first = fp.fctiwz(2, c.f(3));
    c.set(4, c.u32(actor + actor_match));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0)));
    c.put64(stack + 0x80, first);
    c.set(3, c.u32(c.r(4) + 0x3108));
    c.set(0, c.u32(stack + 0x84));
    c.set(0, c.r(3) + c.r(0));
    c.put32(c.r(4) + 0x3108, c.r(0));
    const auto second = fp.fctiwz(0, c.f(0));
    c.set(4, c.u32(actor + actor_match));
    c.set(3, c.u32(c.r(4) + 0x310c));
    c.put64(stack + 0x90, second);
    c.set(0, c.u32(stack + 0x94));
    c.set(0, c.r(3) + c.r(0));
    c.put32(c.r(4) + 0x310c, c.r(0));
    if (!(c.cr(0) & 2)) {
        c.set(3, c.u8(recipient + 0x2c));
        c.set(0, c.u8(stack + 0x46));
        c.set(0, c.r(3) + c.r(0));
        c.put8(recipient + 0x2c, c.r(0));
    }
    c.s.pc = ball_action::final_effects_phase;
}

static void adjust_action_balance(Cpu& c, std::uint32_t actor) {
    c.set(3, std::uint32_t(c.i16(c.sp() + execution_rating_slot)));
    c.set(0, std::uint32_t(c.i16(actor + actor_balance)));
    const auto adjustment = signed_half(c.r(17) - c.r(3) - 200);
    c.set(3, adjustment);
    c.set(0, c.r(0) + adjustment);
    c.put16(actor + actor_balance, c.r(0));
    c.set(0, std::uint32_t(c.i16(actor + actor_balance)));
    c.compare(0, std::int32_t(c.r(0)), 200);
    if (c.cr(0) & 4) {
        c.set(0, 200);
        c.put16(actor + actor_balance, 200);
    } else {
        c.set(0, std::uint32_t(c.i16(actor + actor_balance)));
        c.compare(0, std::int32_t(c.r(0)), -200);
        if (c.cr(0) & 8) {
            c.set(0, std::uint32_t(-200));
            c.put16(actor + actor_balance, c.r(0));
        }
    }
}

static bool is_regular_comment_eligible(Cpu& c, std::uint32_t actor) {
    c.compare_logical(0, c.r(24), actor);
    if (!(c.cr(0) & 2)) return false;
    c.set(0, signed_byte(c.r(22)));
    c.compare(0, std::int32_t(c.r(0)), 2);
    if (!(c.cr(0) & 8)) return false;

    c.set(0, std::uint32_t(c.i8(actor + actor_row)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) {
        c.set(0, c.u8(actor + actor_row));
        c.compare(0, std::int32_t(c.r(0)), 11);
        if (!(c.cr(0) & 2)) return true;
    }
    c.set(0, std::uint32_t(c.i8(actor + actor_column)));
    c.set(3, subtract_from(c, c.r(0), 4));
    c.call(0x10775540, 0x10242d30);
    c.compare(0, std::int32_t(c.r(3)), 1);
    return !(c.cr(0) & 4);
}

static void describe_regular_action(Cpu& c, std::uint32_t actor) {
    c.set(0, signed_half(c.r(16)));
    c.compare(0, std::int32_t(c.r(0)), -500);
    if (c.cr(0) & 8) {
        comment_on_actor(c, actor, 0x21cc, 0x10242d6c);
        return;
    }

    c.set(3, actor);
    call_owned(c, 0x10272050, 0x10242d78);
    if (compare_signed_result(c) && !draw_major_contact_parity(c, 0x10242da0)) {
        comment_on_actor(c, actor, 0x21db, 0x10242df4);
        return;
    }

    c.set(3, actor);
    call_owned(c, 0x102721a0, 0x10242e00);
    if (compare_signed_result(c) && !draw_major_contact_parity(c, 0x10242e28))
        comment_on_actor(c, actor, 0x21dc, 0x10242e7c);
    else comment_on_actor(c, actor, 0x1f47, 0x10242ea8);
}

static bool should_dispatch_completed_action(Cpu& c, std::uint32_t actor) {
    c.set(0, signed_byte(c.r(22)));
    c.compare(0, std::int32_t(c.r(0)), 4);
    if (c.cr(0) & 2) return true;

    const auto balance = c.i16(actor + actor_balance);
    c.set(14, std::uint32_t(balance));
    c.compare(0, balance, 0);
    draw_narrow_interval(c, balance, 13, 0x10242f7c);
    const auto threshold = draw_opposition_reaction(c, 200, 0x51eb851f, 6, 0x102430d8);
    c.compare(0, std::int32_t(c.r(13)), std::int32_t(threshold));
    return !(c.cr(0) & 8);
}

static void dispatch_completed_action(Cpu& c, std::uint32_t actor) {
    c.set(0, signed_byte(c.r(22)));
    c.compare(0, std::int32_t(c.r(0)), 4);
    if (c.cr(0) & 8) {
        c.set(3, c.u32(actor + actor_match));
        c.set(0, 1);
        c.put8(c.r(3) + 0x1c0, 1);
        for (const auto counter : {0x3108, 0x310c}) {
            c.set(4, c.u32(actor + actor_match));
            c.set(3, c.u32(c.r(4) + counter));
            c.set(0, c.r(3) + 100);
            c.put32(c.r(4) + counter, c.r(0));
        }
    }
    c.set(3, actor);
    call_owned(c, 0x102775e0, 0x10243158);
}

void complete_regular(Cpu& c) {
    const auto actor = c.r(actor_register);
    c.s.pc = ball_action::regular_effects_phase;
    c.set(0, signed_byte(c.r(23)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) comment_on_actor(c, actor, 0x216e, 0x10242cf0);
    else {
        if (is_regular_comment_eligible(c, actor)) describe_regular_action(c, actor);
        adjust_action_balance(c, actor);
    }
    c.s.pc = ball_action::success_reaction_phase;
}

void react_to_success(Cpu& c) {
    const auto actor = c.r(actor_register);
    c.s.pc = ball_action::success_reaction_phase;
    if (should_dispatch_completed_action(c, actor)) dispatch_completed_action(c, actor);

    const bool opposing_area = c.i8(actor + actor_side) == 0;
    c.set(0, opposing_area);
    c.compare(0, opposing_area, 1);
    check_area_coordinates(c, actor, opposing_area, 0, 3, 0);
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) {
        const auto tendency = c.i16(actor + 0x32);
        c.set(13, std::uint32_t(tendency));
        c.compare(0, tendency, 0);
        draw_narrow_interval(c, tendency, 14, 0x10243248);
        c.compare(0, std::int32_t(c.r(14)), -10);
        if (c.cr(0) & 8) {
            c.set_f(1, c.f32(c.r(31) + 0x18));
            c.set(3, actor);
            call_owned(c, 0x102865a0, 0x10243398);
            comment_on_opposing_side(c, actor, 0x211d, 0x102433d0);
        }
    }
    c.s.pc = ball_action::return_phase;
}

static bool has_player_profiles(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    c.set(0, c.u32(actor + 0x70));
    c.compare_logical(0, c.r(0), 0);
    if (c.cr(0) & 2) return false;
    c.set(0, c.u32(recipient + 0x70));
    c.compare_logical(0, c.r(0), 0);
    return (c.cr(0) & 2) == 0;
}

static bool is_goalkeeper(Cpu& c, std::uint32_t player, std::uint32_t back) {
    c.set(3, player);
    call_owned(c, 0x10289560, back);
    c.set(0, signed_half(c.r(3)));
    c.compare(0, std::int32_t(c.r(0)), 1);
    return c.cr(0) & 2;
}

static bool describe_profile_advantage(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    if (!has_player_profiles(c, actor, recipient)) return false;
    advance_stream(c, 0x10242714);
    c.set(5, c.u32(c.r(30)));
    c.set(4, 0x55550000u);
    c.set(3, c.u32(actor + 0x70));
    c.set(4, 0x55555556u);
    c.set(6, c.u16(c.r(5)));
    c.set(5, c.u32(c.r(27)));
    c.set(0, c.u8(c.r(3) + 0x2e));
    const auto sample = c.r(6) + c.r(5);
    c.set(6, sample);
    c.set(3, c.u32(recipient + 0x70));
    c.set(5, high_product(sample, 0x55555556));
    c.set(4, signed_byte(c.r(0)));
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x2e)));
    c.set(3, c.r(4) + sample - truncated_interval(c.r(5)) * 3 + 3);
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 8)) return false;
    comment_on_pair(c, actor, recipient, 0x2164, 0x10242798);
    return true;
}

static bool describe_outfield_advantage(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    if (!has_player_profiles(c, actor, recipient) || is_goalkeeper(c, recipient, 0x102427bc)) return false;
    advance_stream(c, 0x102427e8);
    c.set(4, c.u32(c.r(30)));
    c.set(5, 0x55550000u);
    c.set(0, c.u32(c.r(27)));
    c.set(5, 0x55555556u);
    c.set(6, c.u16(c.r(4)));
    c.set(3, c.u32(recipient + 0x70));
    const auto sample = c.r(6) + c.r(0);
    c.set(6, sample);
    c.set(4, c.u32(actor + 0x70));
    c.set(5, high_product(sample, 0x55555556));
    c.set(0, c.u8(c.r(3) + 0x2e));
    c.set(3, c.u8(c.r(4) + 0x2e));
    c.set(0, signed_byte(c.r(0)));
    c.set(4, signed_byte(c.r(3)));
    c.set(3, c.r(0) + sample - truncated_interval(c.r(5)) * 3);
    c.set(0, c.r(3) + 5);
    c.compare(0, std::int32_t(c.r(4)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 4)) return false;
    comment_on_pair(c, actor, recipient, 0x21cb, 0x1024286c);
    return true;
}

static void describe_special_receiver(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    if (describe_profile_advantage(c, actor, recipient) || describe_outfield_advantage(c, actor, recipient)) return;
    if (is_goalkeeper(c, recipient, 0x10242878)) {
        const auto fifth = draw_opposition_reaction(c, 5, 0x66666667, 1, 0x102428a4);
        c.compare(0, std::int32_t(fifth), 0);
        if (!(c.cr(0) & 2)) {
            comment_on_pair(c, actor, recipient, 0x214f, 0x10242908);
            return;
        }
    }
    comment_on_pair(c, actor, recipient, 0x1f5d, 0x10242934);
}

void add_player_counter(Cpu& c, std::uint32_t player, std::uint32_t offset, std::uint32_t change) {
    c.set(4, c.u8(player + offset));
    c.set(0, c.r(4) + change);
    c.put8(player + offset, c.r(0));
}

static void react_special_receiver(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    c.set(3, c.u8(actor + 9));
    c.set(0, c.r(3) + 1);
    c.put8(actor + 9, c.r(0));
    c.set(3, std::uint32_t(c.i16(actor + 0x36)));
    c.set(0, c.r(3) + 300);
    c.put16(actor + 0x36, c.r(0));
    c.set(0, std::uint32_t(c.i8(recipient + 0x17)));
    const auto base = c.r(0) * 30;
    c.set(3, base);
    c.set(16, base + 70);
    c.s.wr<1>(reg::xer_ca, c.r(16) < base);
    c.compare(0, std::int32_t(c.r(16)), 0);
    draw_narrow_interval(c, std::int32_t(c.r(16)), 13, 0x10242ac8);
    c.compare(0, std::int32_t(c.r(13)), 0);
    if (!(c.cr(0) & 2)) return;

    advance_stream(c, 0x10242c2c);
    c.set(4, c.u32(c.r(30)));
    c.set(3, recipient);
    c.set(0, c.u32(c.r(27)));
    c.set(4, c.u16(c.r(4)));
    const auto sample = c.r(4) + c.r(0);
    const auto sign = sample >> 31;
    c.set(0, ((sample & 1u) ^ sign) - sign);
    c.set(4, signed_byte(c.r(0)));
    call_owned(c, 0x10288f30, 0x10242c60);
    if (compare_signed_result(c)) {
        c.set(3, recipient);
        c.set(4, 0);
        call_owned(c, 0x1022ba10, 0x10242c74);
    }
}

static void apply_special_receiver(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    describe_special_receiver(c, actor, recipient);
    c.set(0, 123);
    c.set(3, recipient);
    c.put16(actor + 0x19c, 123);
    add_player_counter(c, recipient, 7, 1);
    call_owned(c, 0x10289560, 0x10242950);
    c.set(0, signed_half(c.r(3)));
    c.set(5, std::uint32_t(-600));
    c.compare(0, std::int32_t(c.r(0)), 1);
    if (!(c.cr(0) & 2)) c.set(5, std::uint32_t(-10));

    c.set(4, std::uint32_t(c.i16(recipient + 0x36)));
    c.set(0, signed_half(c.r(5)));
    c.set(3, recipient);
    c.set(0, c.r(4) + c.r(0));
    c.put16(recipient + 0x36, c.r(0));
    add_player_counter(c, actor, 7, 1);
    add_player_counter(c, actor, 8, 1);
    for (const auto change : {-10, 50}) {
        c.set(4, std::uint32_t(c.i16(actor + 0x36)));
        c.set(0, c.r(4) + change);
        c.put16(actor + 0x36, c.r(0));
    }
    call_owned(c, 0x10270ba0, 0x102429ac);
    if (compare_signed_result(c) && aligns_with_goal(c, actor, 0x10242a14, 0x10242a6c))
        react_special_receiver(c, actor, recipient);
}

void complete_special(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    c.s.pc = ball_action::final_effects_phase;
    c.set(0, signed_byte(c.r(22)));
    c.compare(0, std::int32_t(c.r(0)), 4);
    if (!(c.cr(0) & 2)) {
        c.s.pc = ball_action::regular_effects_phase;
        return;
    }
    c.compare_logical(0, recipient, 0);
    if (!(c.cr(0) & 2)) apply_special_receiver(c, actor, recipient);
    adjust_action_balance(c, actor);
    c.s.pc = ball_action::success_reaction_phase;
}
}
