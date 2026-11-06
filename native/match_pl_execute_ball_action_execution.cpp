#include "match_pl_execute_ball_action_detail.hpp"

namespace cm::aot::match_pl::ball_action_detail {
using namespace guest;

constexpr std::uint32_t match_execution_actor = 0xe000;

static bool attempt_execution_contact(Cpu& c) {
    c.set(3, std::uint32_t(c.i16(c.sp() + execution_rating_slot)));
    c.set(0, signed_half(c.r(17)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 8)) return true;

    const auto sample = draw_opposition_reaction(c, 5, 0x66666667, 1, 0x1023fb08);
    c.compare(0, std::int32_t(sample), 0);
    return c.cr(0) & 2;
}

static void comment_on_warning(Cpu& c, std::uint32_t actor, std::uint32_t recipient, std::uint32_t back) {
    c.set(3, c.u32(actor + actor_match));
    c.set(4, 0x2168);
    c.set(7, c.u8(recipient + actor_side));
    c.set(8, no_comment);
    c.set(3, c.u32(c.r(3) + match_commentary));
    c.set(9, no_comment);
    c.set(6, c.u8(recipient + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + actor_match));
    c.call(0x10201180, back);
}

std::uint32_t draw_modulo_three(Cpu& c, std::uint32_t refill_back) {
    advance_stream(c, refill_back);
    c.set(4, c.u32(c.r(30)));
    c.set(3, 0x55550000);
    c.set(0, c.u32(c.r(27)));
    c.set(3, 0x55555556);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto quotient = high_product(sample, 0x55555556);
    c.set(3, quotient);
    const auto remainder = sample - truncated_interval(quotient) * 3;
    c.set(0, remainder);
    c.compare(0, std::int32_t(remainder), 0);
    return remainder;
}

static void maybe_warn_on_execution(Cpu& c, std::uint32_t actor, bool result_phase) {
    const auto stack = c.sp();
    c.set(3, c.u32(actor + actor_match));
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x7931)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) return;

    const auto side = c.u8(actor + actor_side);
    const auto area_side = result_phase ? side : std::uint32_t(std::int8_t(side) == 0);
    c.set(0, area_side);
    c.compare(0, std::int32_t(area_side), 1);
    check_area_coordinates(c, result_phase ? actor : c.r(recipient_register), area_side == 1, 0, 3, 0);
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) return;

    c.set(3, c.u32(actor + actor_match));
    c.set(4, result_phase ? c.r(13) : c.u8(stack + execution_kind_slot));
    c.call(0x101ee530, result_phase ? 0x10241160 : 0x1023fc44);
    if (!compare_signed_result(c)) {
        c.set(0, std::uint32_t(c.i8(stack + execution_marker_slot)));
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (c.cr(0) & 2) {
            const auto tendency = c.i16(c.r(recipient_register) + 0x32);
            c.set(19, std::uint32_t(tendency));
            c.compare(0, tendency, 0);
            draw_narrow_interval(c, tendency, 18, result_phase ? 0x102411a8 : 0x1023fc8c);
            c.compare(0, std::int32_t(c.r(18)), -10);
            if (!(c.cr(0) & 8)) return;
        }
    }
    if (draw_modulo_three(c, result_phase ? 0x1024130c : 0x1023fdf0) != 0)
        comment_on_warning(c, actor, result_phase ? actor : c.r(recipient_register),
            result_phase ? 0x1024136c : 0x1023fe50);
}

static bool can_emit_immediate_execution(Cpu& c) {
    const auto stack = c.sp();
    c.set(0, std::uint32_t(c.i8(stack + execution_contact_slot)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) return false;

    advance_stream(c, 0x1023fe88);
    c.set(4, c.u32(c.r(30)));
    c.set(3, 0x88890000u);
    c.set(0, c.u32(c.r(27)));
    c.set(3, 0x88888889u);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto quotient = shift_signed_with_carry(c, high_product(sample, std::int32_t(c.r(3))) + sample, 4);
    c.set(3, quotient >> 31);
    c.set(0, sample - truncated_interval(quotient) * 30);
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) return false;

    c.set(0, std::uint32_t(c.i8(stack + execution_blocked_slot)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    return c.cr(0) & 2;
}

static void emit_immediate_execution(Cpu& c, std::uint32_t actor) {
    const auto stack = c.sp();
    c.set(0, c.u8(stack + contact_column_slot));
    c.set(5, c.r(recipient_register));
    c.set(6, actor);
    c.put32(stack + 0x38, c.r(0));
    c.set(0, c.u8(stack + contact_row_slot));
    c.put32(stack + 0x3c, c.r(0));
    c.set(4, c.u8(stack + execution_kind_slot));
    c.set(3, c.u32(actor + actor_match));
    c.set(7, c.u8(stack + execution_blocked_slot));
    c.set(4, signed_byte(c.r(4)));
    c.set(8, c.u8(stack + execution_contact_slot));
    c.set(9, c.u8(actor + actor_column));
    c.set(10, c.u8(actor + actor_row));
    c.call(0x101d68f0, 0x1023ff0c);
}

void prepare_execution(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto stack = c.sp();
    c.s.pc = ball_action::action_execution_phase;
    c.set(5, c.u32(actor + actor_match));
    c.set(3, stack + execution_blocked_slot);
    c.set(0, 0);
    c.set(4, actor);
    c.set(5, c.r(5) + 0x10000);
    c.set(6, c.r(13));
    c.put32(c.r(5) - 0x10000 + match_execution_actor, actor);
    c.set({{7, stack + 0x47}, {8, stack + 0x46}, {9, stack + execution_rating_slot}});
    c.put32(stack + 0x38, c.r(3));
    c.set(10, stack + execution_kind_slot);
    c.put32(stack + 0x3c, 0);
    c.set(3, c.u32(actor + actor_match));
    c.set(5, c.u8(actor + actor_side));
    c.call(0x101eaf60, 0x1023fa9c);

    c.set(7, c.u8(stack + execution_kind_slot));
    c.set(recipient_register, c.r(3));
    c.set(3, c.u32(actor + actor_match));
    c.set(4, actor);
    c.set(5, signed_byte(c.r(13)));
    c.set(6, c.r(recipient_register));
    c.set(7, signed_byte(c.r(7)));
    c.set(8, 0);
    c.call(0x101f5350, 0x1023fac0);
    c.set(3, c.u32(actor + actor_match));
    c.compare_logical(0, c.r(recipient_register), 0);
    c.set(0, 0);
    c.set(3, c.r(3) + 0x10000);
    c.put32(c.r(3) - 0x10000 + match_execution_actor, 0);
    if (c.cr(0) & 2) {
        c.s.pc = ball_action::no_receiver_phase;
        return;
    }

    if (attempt_execution_contact(c)) {
        c.set(3, c.u32(actor + actor_match));
        c.set(4, c.r(recipient_register));
        c.set(6, c.u8(stack + execution_kind_slot));
        c.set(5, actor);
        c.set({{8, stack + contact_column_slot}, {9, stack + contact_row_slot},
               {10, stack + execution_marker_slot}, {7, no_comment}});
        c.call(0x101f5050, 0x1023fb68);
        c.put8(stack + execution_contact_slot, c.r(3));
        maybe_warn_on_execution(c, actor, false);
    } else {
        c.set(0, 0);
        c.put8(stack + execution_contact_slot, 0);
    }
    if (can_emit_immediate_execution(c)) {
        emit_immediate_execution(c, actor);
        c.s.pc = ball_action::execution_result_phase;
        return;
    }

    c.set(0, std::uint32_t(c.i8(stack + execution_contact_slot)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) {
        c.s.pc = ball_action::execution_result_phase;
        return;
    }
    c.set(0, c.u8(stack + execution_marker_slot));
    c.set(4, c.r(recipient_register));
    c.set(5, actor);
    c.set(6, stack + execution_contact_slot);
    c.put32(stack + 0x38, c.r(0));
    c.set(10, stack + execution_feedback_slot);
    c.set(3, c.u32(actor + actor_match));
    c.set(7, c.u8(stack + contact_column_slot));
    c.set(8, c.u8(stack + contact_row_slot));
    c.set(9, c.u8(stack + execution_blocked_slot));
    c.call(0x101f5110, ball_action::execution_message_phase);
    c.s.pc = ball_action::execution_message_phase;
}

enum class ActionCommentary { execution, result, failed };

std::uint32_t live_commentary_target(Cpu& c, std::uint32_t index, unsigned last,
                                      std::int32_t toc_slot, std::uint32_t fallback) {
    c.set(0, index);
    c.compare_logical(0, index, last);
    if (c.cr(0) & 4) return fallback;
    c.set(3, c.toc(toc_slot));
    c.set(0, index * 4);
    c.set(3, c.u32(c.r(3) + c.r(0)));
    c.s.wr<4>(reg::ctr, c.r(3));
    return c.r(3);
}

static void describe_execution_message(Cpu& c, ActionCommentary kind) {
    auto destination = c.s.pc;
    const bool result_phase = kind == ActionCommentary::result;
    const bool failed = kind == ActionCommentary::failed;
    const auto first = result_phase ? 0x10241474u : failed ? 0x10243690u : 0x1023ff6cu;
    const auto fallback = first + 11 * 0x2c;
    const auto table_entry = result_phase ? ball_action::result_message_phase :
        failed ? ball_action::failed_message_phase : ball_action::execution_message_phase;
    if (destination == table_entry) {
        const auto index = result_phase ? signed_byte(c.r(13)) : std::uint32_t(c.i8(c.sp() + execution_kind_slot));
        destination = live_commentary_target(c, index, result_phase ? 56 : 46,
            result_phase ? -0x5208 : failed ? -0x520c : -0x5204, fallback);
    }

    constexpr std::array messages{
        0x1feau, 0x1febu, 0x1fecu, 0x1fedu, 0x1feeu, 0x1ff2u,
        0x1fefu, 0x1ff1u, 0x1ff0u, 0x1ff3u, 0x1ff4u, 0x1fe9u,
    };
    // Preserve live targets outside this commentary family. The root
    // owns their continuation and retains the incoming LR and the CTR.
    if (destination < first || destination > fallback || (destination - first) % 0x2c != 0) {
        c.s.pc = destination;
        return;
    }

    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    const auto first_player = result_phase ? actor : recipient;
    const auto second_player = result_phase ? recipient : actor;
    const auto index = (destination - first) / 0x2c;
    c.set(3, c.u32(actor + actor_match));
    c.set(4, messages[index]);
    c.set(5, c.u32(actor + actor_match));
    c.set(10, 0);
    c.set(3, c.u32(c.r(3) + match_commentary));
    c.set(6, c.u8(first_player + 0x2f));
    c.set(7, c.u8(first_player + actor_side));
    c.set(8, c.u8(second_player + 0x2f));
    c.set(9, c.u8(second_player + actor_side));
    c.call(0x10201180, destination + 0x28);
    c.s.pc = result_phase ? ball_action::result_resolution_phase :
        failed ? ball_action::failed_comment_phase : ball_action::receiver_resolution_phase;
}

void describe_execution(Cpu& c) { describe_execution_message(c, ActionCommentary::execution); }
void describe_result(Cpu& c) { describe_execution_message(c, ActionCommentary::result); }
void describe_failed(Cpu& c) { describe_execution_message(c, ActionCommentary::failed); }

static void attempt_result_contact(Cpu& c, std::uint32_t actor) {
    const auto stack = c.sp();
    c.set(0, std::uint32_t(c.i16(stack + execution_rating_slot)));
    c.set(3, signed_half(c.r(17)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 4)) {
        const auto sample = draw_opposition_reaction(c, 5, 0x66666667, 1, 0x10241034);
        c.compare(0, std::int32_t(sample), 0);
        if (!(c.cr(0) & 2)) return;
    }
    c.set(3, c.u32(actor + actor_match));
    c.set(4, actor);
    c.set(5, c.r(recipient_register));
    c.set(6, c.r(13));
    c.set({{8, stack + contact_column_slot}, {9, stack + contact_row_slot},
           {10, stack + execution_marker_slot}, {7, no_comment}});
    c.call(0x101f5050, 0x10241094);
    c.put8(stack + execution_contact_slot, c.r(3));
    maybe_warn_on_execution(c, actor, true);
}

static bool has_execution_contact(Cpu& c) {
    c.set(0, std::uint32_t(c.i8(c.sp() + execution_contact_slot)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) == 0;
}

static void maybe_cancel_result_contact(Cpu& c, std::uint32_t actor) {
    if (!has_execution_contact(c)) return;
    const auto tenth = draw_opposition_reaction(c, 10, 0x66666667, 2, 0x10241398);
    c.compare(0, std::int32_t(tenth), 0);
    if (!(c.cr(0) & 2)) return;

    const auto stack = c.sp();
    c.set(0, c.u8(stack + contact_column_slot));
    c.set(5, actor);
    c.set(6, c.r(recipient_register));
    c.set(4, signed_byte(c.r(13)));
    c.put32(stack + 0x38, c.r(0));
    c.set(7, 0);
    c.set(0, c.u8(stack + contact_row_slot));
    c.put32(stack + 0x3c, c.r(0));
    c.set(3, c.u32(actor + actor_match));
    c.set(8, c.u8(stack + execution_contact_slot));
    c.set(9, c.u8(actor + actor_column));
    c.set(10, c.u8(actor + actor_row));
    c.call(0x101d68f0, 0x10241408);
    c.set(0, 0);
    c.put8(stack + execution_contact_slot, 0);
}

void prepare_result(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto stack = c.sp();
    c.s.pc = ball_action::execution_result_phase;
    if (has_execution_contact(c)) {
        c.set(0, 0);
        c.put8(stack + execution_contact_slot, 0);
    } else {
        attempt_result_contact(c, actor);
        maybe_cancel_result_contact(c, actor);
    }
    if (!has_execution_contact(c)) {
        c.s.pc = ball_action::unopposed_result_phase;
        return;
    }

    c.set(0, c.u8(stack + execution_marker_slot));
    c.set(4, actor);
    c.set(5, c.r(recipient_register));
    c.set(6, stack + execution_contact_slot);
    c.put32(stack + 0x38, c.r(0));
    c.set(10, stack + execution_feedback_slot);
    c.set(9, 0);
    c.set(3, c.u32(actor + actor_match));
    c.set(7, c.u8(stack + contact_column_slot));
    c.set(8, c.u8(stack + contact_row_slot));
    c.call(0x101f5110, ball_action::result_message_phase);
    c.s.pc = ball_action::result_message_phase;
}
}
