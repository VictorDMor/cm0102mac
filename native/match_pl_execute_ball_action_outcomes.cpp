#include "match_pl_execute_ball_action_detail.hpp"

namespace cm::aot::match_pl::ball_action_detail {
using namespace guest;

enum class OutcomeKind { receiver, result };

static bool major_execution_contact(Cpu& c) {
    c.set(3, c.u8(c.sp() + execution_contact_slot));
    c.set(0, std::uint8_t(c.r(3) - 3));
    c.compare_logical(0, c.r(0), 3);
    if (!(c.cr(0) & 4)) return true;
    c.set(0, signed_half(c.r(17)));
    c.compare(0, std::int32_t(c.r(0)), 0x1f7a);
    return c.cr(0) & 2;
}

void comment_on_ordered_pair(Cpu& c, std::uint32_t actor, std::uint32_t first, std::uint32_t second,
                             unsigned message, std::uint32_t back) {
    c.set(3, c.u32(actor + actor_match));
    c.set(4, message);
    c.set(5, c.u32(actor + actor_match));
    c.set(10, 0);
    c.set(3, c.u32(c.r(3) + match_commentary));
    c.set(6, c.u8(first + 0x2f));
    c.set(7, c.u8(first + actor_side));
    c.set(8, c.u8(second + 0x2f));
    c.set(9, c.u8(second + actor_side));
    c.call(0x10201180, back);
}

void comment_on_pair(Cpu& c, std::uint32_t actor, std::uint32_t recipient, unsigned message, std::uint32_t back) {
    comment_on_ordered_pair(c, actor, actor, recipient, message, back);
}

static void describe_blocked_action(Cpu& c, std::uint32_t actor, std::uint32_t recipient) {
    const auto stack = c.sp();
    c.set(0, c.u8(stack + execution_kind_slot));
    c.compare(0, std::int32_t(c.r(0)), 44);
    if (!(c.cr(0) & 2)) {
        c.set(0, c.u8(stack + execution_kind_slot));
        c.compare(0, std::int32_t(c.r(0)), 47);
        if (!(c.cr(0) & 2)) return;
    }
    c.set(0, std::uint32_t(c.i8(stack + execution_marker_slot)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) return;
    c.set(0, c.u8(stack + execution_kind_slot));
    c.compare(0, std::int32_t(c.r(0)), 47);
    if (c.cr(0) & 2) comment_on_pair(c, actor, recipient, 0x21da, 0x102406dc);
    else comment_on_pair(c, actor, recipient, 0x21d9, 0x10240708);
    if (major_execution_contact(c)) {
        c.set(0, 1);
        c.put8(recipient + 0x16, 1);
    }
}

static std::uint32_t draw_signed_four(Cpu& c, std::uint32_t refill_back) {
    advance_stream(c, refill_back);
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(27)));
    const auto sample = c.u16(c.r(3)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(3, sign);
    const auto result = std::rotl(((sample & 3) << 30) - sign, 2) + sign;
    c.set(0, result);
    c.compare(0, std::int32_t(result), 0);
    return result;
}

static void describe_execution_contact(Cpu& c, std::uint32_t actor, std::uint32_t recipient,
                                OutcomeKind kind) {
    const bool receiver = kind == OutcomeKind::receiver;
    const auto subject = receiver ? recipient : actor;
    c.set(0, signed_half(c.r(16)));
    c.compare(0, std::int32_t(c.r(0)), -1);
    if (!(c.cr(0) & 4)) return;
    if (receiver) describe_blocked_action(c, actor, recipient);

    if (draw_signed_four(c, receiver ? 0x10240750 : 0x10241bd4) == 0) {
        const auto tendency = c.i8(c.sp() + contact_row_slot);
        const unsigned tendency_register = receiver ? 18 : 19;
        const unsigned draw_register = receiver ? 13 : 18;
        c.set(tendency_register, std::uint32_t(tendency));
        c.compare(0, tendency, 0);
        draw_narrow_interval(c, tendency, draw_register, receiver ? 0x102407b4 : 0x10241c38);
        const auto threshold = draw_opposition_reaction(c, 20, 0x66666667, 3, receiver ? 0x10240910 : 0x10241d94);
        c.compare(0, std::int32_t(c.r(draw_register)), std::int32_t(threshold));
        if (c.cr(0) & 4)
            maybe_comment_on_opposition_reaction(c, actor, subject, receiver ? 0x102409f4 : 0x10241e78);
        comment_on_player(c, actor, subject, 0x2109, receiver ? 0x10240a1c : 0x10241ea0);
    }
    comment_on_player(c, actor, subject, c.r(16), receiver ? 0x10240a44 : 0x10241ec8);

    auto code = c.u8(c.sp() + execution_contact_slot);
    c.set(0, code);
    c.compare(0, code, 6);
    bool response = c.cr(0) & 2;
    if (!response) {
        code = c.u8(c.sp() + execution_contact_slot);
        c.set(3, code);
        c.set(0, std::uint8_t(code - 3));
        c.compare_logical(0, c.r(0), 1);
        response = !(c.cr(0) & 4);
    }
    if (!response) {
        code = c.u8(c.sp() + execution_contact_slot);
        c.set(0, code);
        c.compare(0, code, 5);
        response = c.cr(0) & 2;
    }
    if (response) {
        c.set(3, c.u32(actor + actor_match));
        c.set(4, subject);
        c.set(5, 0);
        c.call(0x101f1530, receiver ? 0x10240a80 : 0x10241f04);
    }
}

static void comment_on_restart_side(Cpu& c, std::uint32_t actor, std::uint32_t back, OutcomeKind kind) {
    if (kind == OutcomeKind::result) {
        comment_on_opposing_side(c, actor, c.r(17), back);
        return;
    }
    c.set(3, c.u32(actor + actor_match));
    c.set(4, c.r(17));
    c.set(0, c.u8(actor + actor_side));
    c.set(6, no_comment);
    c.set(3, c.u32(c.r(3) + match_commentary));
    c.set(8, no_comment);
    c.set(5, c.u32(actor + actor_match));
    c.set(7, signed_byte(c.r(0)));
    c.set(9, no_comment);
    c.set(10, 0);
    c.call(0x10201180, back);
}

static void comment_on_restart_player(Cpu& c, std::uint32_t actor, unsigned message, std::uint32_t back,
                               OutcomeKind kind) {
    if (kind == OutcomeKind::result) {
        comment_on_recipient(c, actor, c.r(16), message, back);
        return;
    }
    c.set(3, c.u32(actor + actor_match));
    c.set(4, message);
    c.set(0, c.u8(actor + actor_side));
    c.set(8, no_comment);
    c.set(3, c.u32(c.r(3) + match_commentary));
    c.set(9, no_comment);
    c.set(6, c.u8(c.r(13) + 0x2f));
    c.set(7, signed_byte(c.r(0)));
    c.set(5, c.u32(actor + actor_match));
    c.set(10, 0);
    c.call(0x10201180, back);
}

static bool in_attacking_band(Cpu& c, std::uint32_t player, std::uint32_t back) {
    c.set(3, player);
    call_owned(c, 0x10272150, back);
    return compare_signed_result(c);
}

static void describe_restart_followup(Cpu& c, std::uint32_t actor, OutcomeKind kind) {
    const bool receiver = kind == OutcomeKind::receiver;
    const auto tested_player = receiver ? actor : c.r(16);
    const auto stack = c.sp();
    c.set(0, signed_half(c.r(17)));
    c.compare(0, std::int32_t(c.r(0)), 0x1f79);
    if (!(c.cr(0) & 2)) return;
    c.set(0, std::uint32_t(c.i8(stack + execution_marker_slot)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) return;
    c.set(0, c.u8(stack + execution_contact_slot));
    c.compare(0, std::int32_t(c.r(0)), 7);
    if (c.cr(0) & 2) {
        if (!in_attacking_band(c, tested_player, receiver ? 0x10240b70 : 0x10242028)) return;
        const auto side = c.u8(actor + actor_side);
        const auto area_side = receiver ? unsigned(std::int8_t(side) == 0) : side;
        c.set(0, area_side);
        c.compare(0, area_side, 1);
        check_area_coordinates(c, tested_player, area_side == 1, 0, 3, 0);
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (c.cr(0) & 2) comment_on_restart_player(c, actor, 0x21e1, receiver ? 0x10240c60 : 0x10242114, kind);
        return;
    }

    c.set(4, c.u8(actor + actor_column));
    c.set(3, actor);
    call_owned(c, 0x102720a0, receiver ? 0x10240c70 : 0x10242124);
    if (compare_signed_result(c) && in_attacking_band(c, tested_player, receiver ? 0x10240c80 : 0x10242134))
        comment_on_restart_player(c, actor, 0x21df, receiver ? 0x10240cb4 : 0x10242174, kind);
    else if (in_attacking_band(c, tested_player, receiver ? 0x10240cc0 : 0x10242180))
        comment_on_restart_player(c, actor, 0x21e0, receiver ? 0x10240cf4 : 0x102421c0, kind);
}

static void choose_execution_restart(Cpu& c, std::uint32_t actor, OutcomeKind kind) {
    const bool receiver = kind == OutcomeKind::receiver;
    c.set(0, signed_half(c.r(17)));
    c.compare(0, std::int32_t(c.r(0)), 0x1f7a);
    if (c.cr(0) & 2) return;

    if (receiver) {
        c.set(4, c.u32(actor + actor_match));
        c.set(6, 0);
        c.set(3, c.u32(actor + actor_match));
        c.set(5, c.u8(c.r(4) + match_status));
        c.set(4, c.u8(actor + actor_side));
    } else {
        c.set(0, c.u8(actor + actor_side));
        c.set(6, 0);
        c.set(3, c.u32(actor + actor_match));
        c.set(0, std::countl_zero(signed_byte(c.r(0))));
        c.set(5, c.u8(c.r(3) + match_status));
        c.set(0, c.r(0) >> 5);
        c.set(3, c.u32(actor + actor_match));
        c.set(4, c.r(0));
    }
    c.call(0x101f3a30, receiver ? 0x10240aa4 : 0x10241f38);
    c.set(receiver ? 13 : 16, c.r(3));
    c.compare(0, std::int32_t(c.r(3)), 0);
    if (c.cr(0) & 2) {
        comment_on_restart_side(c, actor, receiver ? 0x10240ad8 : 0x10241f78, kind);
        return;
    }
    c.set(0, c.u8(c.sp() + execution_contact_slot));
    c.compare(0, std::int32_t(c.r(0)), 7);
    if (c.cr(0) & 2)
        comment_on_restart_player(c, actor, 0x21e2, receiver ? 0x10240b14 : 0x10241fc0, kind);
    else comment_on_restart_side(c, actor, receiver ? 0x10240b44 : 0x10241ffc, kind);
    describe_restart_followup(c, actor, kind);
}

bool draw_major_contact_parity(Cpu& c, std::uint32_t back) {
    advance_stream(c, back);
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(27)));
    c.set(3, c.u16(c.r(3)));
    const auto sample = c.r(3) + c.r(0);
    const auto sign = sample >> 31;
    c.set(3, sign);
    c.set(0, (sample & 1u) ^ sign);
    c.s.wr<1>(reg::xer_ca, c.r(0) >= sign);
    c.set(0, c.r(0) - sign);
    c.compare(0, std::int32_t(c.r(0)), 0);
    return c.cr(0) & 2;
}

static void mark_major_contact(Cpu& c, std::uint32_t recipient, OutcomeKind kind) {
    const bool receiver = kind == OutcomeKind::receiver;
    if (!receiver) {
        c.set(0, c.u8(c.sp() + execution_contact_slot));
        c.compare(0, std::int32_t(c.r(0)), 6);
    }
    if (receiver || !(c.cr(0) & 2)) {
        c.set(0, c.u8(c.sp() + execution_contact_slot));
        c.compare(0, std::int32_t(c.r(0)), 4);
        if (!(c.cr(0) & 2)) {
            c.set(0, c.u8(c.sp() + execution_contact_slot));
            c.compare(0, std::int32_t(c.r(0)), 5);
        }
    }
    if (c.cr(0) & 2) {
        c.set(0, 1);
        c.put8(recipient + 0x16, 1);
    }
}

static void describe_execution_end(Cpu& c, std::uint32_t actor, std::uint32_t recipient, OutcomeKind kind) {
    const bool receiver = kind == OutcomeKind::receiver;
    c.set(0, std::uint32_t(c.i8(c.sp() + execution_marker_slot)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) {
        if (receiver) comment_on_actor(c, actor, 0x20dd, 0x10240d3c);
        else comment_on_recipient(c, actor, recipient, 0x20dd, 0x10242218);
        mark_major_contact(c, recipient, kind);
        if (!major_execution_contact(c)) return;
        if (receiver) {
            if (draw_major_contact_parity(c, 0x10240d9c))
                comment_on_opposing_side(c, actor, 0x20d9, 0x10240e00);
        } else if (draw_modulo_three(c, 0x10242284) != 0)
            comment_on_side(c, actor, 0x20d9, 0x102422e4);
        return;
    }

    const auto ninth = draw_opposition_reaction(c, 9, 0x38e38e39, 1, receiver ? 0x10240e24 : 0x10242308);
    c.compare(0, std::int32_t(ninth), 0);
    if (!(c.cr(0) & 2)) return;

    const auto emit = receiver ? comment_on_opposing_side : comment_on_side;
    emit(c, actor, 0x20df, receiver ? 0x10240e98 : 0x1024236c);
    if (draw_modulo_three(c, receiver ? 0x10240eb8 : 0x1024238c) == 0)
        emit(c, actor, 0x20e0, receiver ? 0x10240f28 : 0x102423ec);
    if (major_execution_contact(c) &&
        draw_major_contact_parity(c, receiver ? 0x10240f68 : 0x1024242c) == receiver)
        emit(c, actor, 0x20d9, receiver ? 0x10240fcc : 0x10242480);
}

static void complete_execution_outcome(Cpu& c, OutcomeKind kind) {
    const bool receiver = kind == OutcomeKind::receiver;
    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    const auto subject = receiver ? recipient : actor;
    c.set(4, c.u8(actor + actor_column));
    c.set(0, signed_half(c.r(17)));
    c.set(3, c.u32(actor + actor_match));
    c.compare(0, std::int32_t(c.r(0)), 0x1f7a);
    c.set(0, 0);
    c.put8(c.r(3) + 0x791d, c.r(4));
    c.set(4, c.u8(actor + actor_row));
    c.set(3, c.u32(actor + actor_match));
    c.put8(c.r(3) + 0x791e, c.r(4));
    if (receiver) {
        c.set(4, c.u8(actor + actor_side));
        c.set(3, c.u32(actor + actor_match));
        c.put8(c.r(3) + 0x7924, c.r(4));
    } else {
        c.set(3, c.u8(actor + actor_side));
        c.set(4, c.u32(actor + actor_match));
        c.set(3, unsigned(signed_byte(c.r(3)) == 0));
        c.put8(c.r(4) + 0x7924, c.r(3));
    }
    c.set(3, c.u32(actor + actor_match) + 0x10000);
    c.put32(c.r(3) - 0x2000, 0);
    if (c.cr(0) & 2) comment_on_restart_side(c, actor, receiver ? 0x10240678 : 0x10241ba8, kind);
    describe_execution_contact(c, actor, recipient, kind);
    choose_execution_restart(c, actor, kind);

    c.set(3, c.u32(actor + actor_match));
    c.set(4, receiver ? c.u8(c.sp() + execution_kind_slot) : c.r(13));
    c.call(0x101ee530, receiver ? 0x10240d00 : 0x102421cc);
    if (!compare_signed_result(c)) {
        describe_execution_end(c, actor, recipient, kind);
        c.set(3, c.u32(actor + actor_match));
        c.set(4, subject);
        c.set_f(1, c.f32(c.sp() + execution_feedback_slot));
        c.call(0x101d6780, receiver ? 0x10240fdc : 0x10242490);
        c.set(3, subject);
        c.set(4, 0);
        call_owned(c, 0x102884d0, receiver ? 0x10240fe8 : 0x1024249c);
    }
    c.set(3, std::uint32_t(c.i16(subject + 0x36)));
    c.set(0, c.r(3) - 25);
    c.put16(subject + 0x36, c.r(0));
    c.s.pc = ball_action::return_phase;
}

void complete_receiver(Cpu& c) {
    c.s.pc = ball_action::receiver_outcome_phase;
    complete_execution_outcome(c, OutcomeKind::receiver);
}

void complete_result(Cpu& c) {
    c.s.pc = ball_action::result_outcome_phase;
    complete_execution_outcome(c, OutcomeKind::result);
}
}
