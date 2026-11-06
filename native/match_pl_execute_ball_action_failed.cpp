#include "match_pl_execute_ball_action_detail.hpp"

namespace cm::aot::match_pl::ball_action_detail {
using namespace guest;

static bool needs_failed_area_report(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    const auto stack = c.sp();
    c.set(3, c.u32(actor + actor_match));
    c.set(4, c.u8(stack + execution_kind_slot));
    c.call(0x101ee530, 0x102433e8);
    if (compare_signed_result(c)) return true;
    c.set(0, std::uint32_t(c.i8(stack + execution_marker_slot)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) return true;
    const auto tendency = c.i16(recipient + 0x32);
    c.set(16, std::uint32_t(tendency));
    c.compare(0, tendency, 0);
    draw_narrow_interval(c, tendency, 13, 0x10243430);
    c.compare(0, std::int32_t(c.r(13)), -10);
    return c.cr(0) & 8;
}

static std::uint32_t select_failed_comparison_message(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    const auto stack = c.sp();
    Fpu fp(c);
    advance_stream(c, 0x10243ae4);
    c.set(5, c.u32(c.r(30)));
    c.set(3, 0x2aab0000);
    c.set(0, 0x43300000);
    c.set(4, c.u32(c.r(27)));
    c.set(5, c.u16(c.r(5)));
    c.set(3, 0x2aaaaaab);
    c.put32(stack + 0x78, c.r(0));
    const auto sample = c.r(5) + c.r(4);
    c.set(4, sample);
    c.set_f(1, c.f32(c.r(31) + 0xe4));
    c.set(3, high_product(sample, 0x2aaaaaab));
    c.set_f(0, c.f32(recipient + 0xb0));
    c.set_f(2, c.f64(c.r(26) + 0x80));
    c.set_f(3, fp.fadds(c.f(1), c.f(0)));
    c.set_f(0, c.f32(actor + 0xb0));
    const auto remainder = sample - truncated_interval(c.r(3)) * 6;
    c.set(0, remainder ^ 0x80000000u);
    c.put32(stack + 0x7c, c.r(0));
    c.set_f(1, c.f64(stack + 0x78));
    c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
    c.set_f(1, fp.fadds(c.f(3), c.f(1)));
    fp.fcmp(0, c.f(1), c.f(0));
    return (c.cr(0) & 8) ? 0x10243b50 : 0x10243b7c;
}

static void prepare_receiver_balance(Cpu& c, std::uint32_t recipient) {
    c.set(3, recipient);
    call_owned(c, 0x1022e3a0, 0x10243bac);
    c.set(0, std::uint32_t(c.i16(c.sp() + execution_rating_slot)));
    c.set(3, c.r(0) - c.r(17));
    c.set(0, c.r(3) - 200);
    c.put16(recipient + actor_balance, c.r(0));
    c.set(0, std::uint32_t(c.i16(recipient + actor_balance)));
    c.compare(0, std::int32_t(c.r(0)), 200);
    if (c.cr(0) & 4) {
        c.set(0, 200);
        c.put16(recipient + actor_balance, 200);
    } else {
        c.set(0, std::uint32_t(c.i16(recipient + actor_balance)));
        c.compare(0, std::int32_t(c.r(0)), -200);
        if (c.cr(0) & 8) {
            c.set(0, std::uint32_t(-200));
            c.put16(recipient + actor_balance, c.r(0));
        }
    }
}

static bool needs_receiver_position(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    const auto kind = c.i8(c.sp() + execution_kind_slot);
    c.set(17, 0);
    c.set(4, 100);
    c.set(0, std::uint32_t(kind));
    c.compare(0, kind, 33);
    if (kind == 33) c.set(4, 250);
    else {
        c.compare(0, kind, kind < 33 ? 19 : 45);
        if (c.cr(0) & 2) c.set(4, kind == 19 ? 400 : 250);
    }
    c.set(3, std::uint32_t(c.i16(recipient + actor_balance)));
    c.set(0, signed_half(c.r(4)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 8)) return true;

    c.set(3, c.u32(actor + actor_match));
    c.set(0, c.u8(c.r(3) + ball_row));
    c.compare(0, std::int32_t(c.r(0)), 4);
    if (c.cr(0) & 2) return true;
    c.set(3, c.u32(actor + actor_match));
    c.set(0, std::uint32_t(c.i8(c.r(3) + ball_row)));
    c.compare(0, std::int32_t(c.r(0)), -1);
    return c.cr(0) & 2;
}

void prepare_failed(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    c.s.pc = ball_action::unsuccessful_action_phase;
    c.compare_logical(0, recipient, 0);
    if (c.cr(0) & 2) {
        c.s.pc = ball_action::missing_receiver_phase;
        return;
    }
    if (needs_failed_area_report(c, actor, recipient)) {
        const auto side = c.u8(recipient + actor_side);
        c.set(0, side);
        c.compare(0, side, 1);
        check_area_coordinates(c, recipient, side == 1, 0, 3, 0);
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (!(c.cr(0) & 2) && draw_major_contact_parity(c, 0x10243640)) {
            c.s.pc = ball_action::failed_message_phase;
            return;
        }
    }
    c.s.pc = ball_action::failed_fallback_phase;
}

void describe_failed_comment(Cpu& c) {
    auto destination = c.s.pc;
    if (destination == ball_action::failed_comment_phase) {
        c.set(0, signed_byte(c.r(22)));
        c.compare(0, std::int32_t(c.r(0)), 4);
        if (!(c.cr(0) & 8)) {
            c.s.pc = ball_action::failed_outcome_phase;
            return;
        }
        const auto sample = draw_opposition_reaction(c, 50, 0x51eb851f, 4, 0x102438c8);
        destination = live_commentary_target(c, sample, 9, -0x5210, 0x10243994);
    }

    unsigned message;
    switch (destination) {
    case 0x1024391c: message = 0x20eb; break;
    case 0x10243958: message = 0x20ea; break;
    case 0x10243994: message = 0x20e8; break;
    default:
        c.s.pc = destination;
        return;
    }
    comment_on_opposing_side_of(c, c.r(actor_register), c.r(recipient_register), message, destination + 0x38);
    c.s.pc = ball_action::failed_outcome_phase;
}

void describe_failed_fallback(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    auto destination = c.s.pc;
    if (destination == ball_action::failed_fallback_phase) {
        const auto kind = std::uint32_t(c.i8(c.sp() + execution_kind_slot));
        destination = live_commentary_target(c, kind, 46, -0x5214, ball_action::failed_outcome_phase);
    }
    if (destination == 0x102439f4) {
        c.set(0, std::uint32_t(c.i16(c.sp() + execution_rating_slot)));
        c.set(3, recipient);
        c.set(0, c.r(0) - c.r(17));
        c.set(4, signed_half(c.r(0)));
        call_owned(c, 0x10270320, 0x10243a08);
        c.s.pc = compare_signed_result(c) ? ball_action::failed_outcome_phase : ball_action::return_phase;
        return;
    }
    if (destination == 0x10243ac4) destination = select_failed_comparison_message(c, actor, recipient);

    unsigned message;
    switch (destination) {
    case 0x10243a14: message = 0x1f61; break;
    case 0x10243a40: message = 0x1f62; break;
    case 0x10243a6c: message = 0x2150; break;
    case 0x10243a98: message = 0x1f64; break;
    case 0x10243b50: message = 0x2164; break;
    case 0x10243b7c: message = 0x1f5d; break;
    default:
        c.s.pc = destination;
        return;
    }
    comment_on_ordered_pair(c, actor, recipient, actor, message, destination + 0x28);
    c.s.pc = ball_action::failed_outcome_phase;
}

void prepare_failed_outcome(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    c.s.pc = ball_action::failed_outcome_phase;
    prepare_receiver_balance(c, recipient);
    if (needs_receiver_position(c, actor, recipient)) {
        c.s.pc = ball_action::receiver_position_phase;
        return;
    }

    c.set(0, c.u8(actor + actor_side));
    c.set(6, 1);
    c.set(3, c.u32(actor + actor_match));
    c.set(8, 0);
    c.set(0, signed_byte(c.r(0)));
    c.set(4, c.u8(actor + actor_column));
    c.set(0, std::countl_zero(c.r(0)));
    c.set(5, c.u8(actor + actor_row));
    c.set(0, c.r(0) >> 5);
    c.set(9, 1);
    c.set(7, c.r(0));
    c.set(10, 0);
    c.call(0x101f1100, 0x10243c90);
    c.set(3, c.u32(actor + actor_match));
    c.set(0, std::uint32_t(c.i8(c.r(3) + match_status)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    c.s.pc = (c.cr(0) & 2) ? ball_action::receiver_sampling_phase : ball_action::receiver_marker_phase;
}
}
