#include "match_pl_execute_ball_action_detail.hpp"

namespace cm::aot::match_pl::ball_action_detail {
using namespace guest;

constexpr std::uint32_t contact_code_slot = 0x41;

static std::int32_t compare_contact_bound(Cpu& c, Fpu& fp, std::uint32_t site, std::int32_t bound) {
    c.set_f(1, c.f32(c.r(31) + 0x3c));
    c.set(3, bound > 0 ? 0x10000u : 0xffff0000u);
    c.set_f(0, c.f32(c.r(31) + 0x2c));
    c.set(3, std::uint32_t(bound));
    c.set_f(0, fp.multiply_add(site, c.f(1), c.f(29), c.f(0)));
    c.put64(c.sp() + 0x78, fp.fctiwz(0, c.f(0)));
    c.set(0, c.u32(c.sp() + 0x7c));
    c.compare(0, std::int32_t(c.r(0)), bound);
    return std::int32_t(c.r(0));
}

static void sample_narrow_contact(Cpu& c, Fpu& fp) {
    const auto stack = c.sp();
    c.set_f(1, c.f32(c.r(31) + 0x3c));
    c.set_f(0, c.f32(c.r(31) + 0x2c));
    c.set(3, c.u32(c.r(30)));
    c.set_f(0, fp.multiply_add(0x1023ec54, c.f(1), c.f(29), c.f(0)));
    c.set(0, c.u32(c.r(27)));
    c.set(3, c.u16(c.r(3)));
    c.set_f(1, c.f32(c.r(31) + 0x3c));
    const auto interval_bits = fp.fctiwz(2, c.f(0));
    c.set_f(0, c.f32(c.r(31) + 0x2c));
    const auto sample = c.r(3) + c.r(0);
    c.set(5, sample);
    c.set_f(0, fp.multiply_add(0x1023ec70, c.f(1), c.f(29), c.f(0)));
    c.put64(stack + 0x78, interval_bits);
    const auto interval = c.u32(stack + 0x7c);
    c.set(4, interval);
    const auto sign_bits = fp.fctiwz(0, c.f(0));
    // The pinned original division at 0x1023ec80 yields zero on a
    // divisor that becomes zero after converting a fractional interval.
    const auto quotient = interval ? std::uint32_t(divide_signed<4>(sample, interval)) : 0u;
    c.set(3, quotient);
    c.put64(stack + 0x80, sign_bits);
    c.set(0, c.u32(stack + 0x84));
    c.compare(0, std::int32_t(c.r(0)), 0);
    const auto remainder = sample - quotient * interval;
    c.set(0, (c.cr(0) & 4) ? remainder : 0u - remainder);
    c.set(14, c.r(0));
}

static void sample_wide_contact(Cpu& c, Fpu& fp) {
    constexpr std::uint32_t double_prefix = 0x43300000;
    constexpr std::int32_t reciprocal = std::int32_t(0x80008001u);
    const auto stack = c.sp();

    c.set_f(1, c.f32(c.r(31) + 0x3c));
    c.set(4, double_prefix);
    c.set_f(0, c.f32(c.r(31) + 0x2c));
    c.set(6, 0x80010000);
    c.set(7, c.u32(c.r(30)));
    c.set(0, double_prefix);
    c.set_f(0, fp.multiply_add(0x1023eb1c, c.f(1), c.f(29), c.f(0)));
    c.put32(stack + 0x80, double_prefix);
    c.set(5, 0x10000);
    c.set(3, 0x80010000);
    c.set(7, c.u16(c.r(7)));
    c.set(6, std::uint32_t(reciprocal));
    const auto divisor = fp.fctiwz(0, c.f(0));
    c.set(4, c.u32(c.r(27)));
    c.set_f(4, c.f64(c.r(26) + 0x80));
    c.set(5, 65535);
    const auto sample = c.r(7) + c.r(4);
    c.set(7, sample);
    c.set_f(2, c.f64(c.r(26)));
    c.put64(stack + 0x78, divisor);
    c.set(6, high_product(sample, reciprocal));
    c.set_f(1, c.f32(c.r(31) + 0x3c));
    c.set(3, std::uint32_t(reciprocal));
    c.set(4, c.u32(stack + 0x7c));
    c.put32(stack + 0x88, double_prefix);
    c.set(4, c.r(4) ^ 0x80000000u);
    c.put32(stack + 0x84, c.r(4));
    const auto quotient = shift_signed_with_carry(c, c.r(6) + sample, 15);
    c.set(0, quotient);
    c.set_f(0, c.f32(c.r(31) + 0x2c));
    c.set_f(3, c.f64(stack + 0x80));
    c.set(4, quotient >> 31);
    c.set_f(0, fp.multiply_add(0x1023eb80, c.f(1), c.f(29), c.f(0)));
    c.set(0, truncated_interval(quotient));
    c.set_f(3, fp.fsub(c.f(3), c.f(4)));
    c.set(0, c.r(0) * 65535u);
    c.set_f(1, c.f64(c.r(26) + 0x80));
    const auto repeated_divisor = fp.fctiwz(0, c.f(0));
    c.set_f(2, fp.fdiv(c.f(3), c.f(2)));
    c.set(14, sample - c.r(0));
    c.put64(stack + 0x98, repeated_divisor);
    c.set(4, c.r(14) ^ 0x80000000u);
    c.put32(stack + 0x8c, c.r(4));
    c.set(0, c.u32(stack + 0x9c));
    c.set_f(0, c.f64(stack + 0x88));
    c.set(0, c.r(0) * 10);
    c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.set(3, high_product(c.r(0), reciprocal));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    c.set(0, c.r(3) + c.r(0));
    const auto converted_sample = fp.fctiwz(0, c.f(0));
    c.set(0, shift_signed_with_carry(c, c.r(0), 15));
    c.set(3, truncated_interval(c.r(0)));
    c.put64(stack + 0x90, converted_sample);
    c.set(14, c.u32(stack + 0x94));
    call_owned(c, 0x10226520, 0x1023ebe4);

    c.set_f(1, c.f32(c.r(31) + 0x3c));
    c.set(4, 0x66660000);
    c.set_f(0, c.f32(c.r(31) + 0x2c));
    c.set(0, c.r(3) + 5);
    c.set(4, 0x66666667);
    c.set_f(0, fp.multiply_add(0x1023ebf8, c.f(1), c.f(29), c.f(0)));
    c.set(0, high_product(c.r(0), 0x66666667));
    const auto correction_bound = fp.fctiwz(0, c.f(0));
    c.set(0, shift_signed_with_carry(c, c.r(0), 2));
    c.put64(stack + 0xa0, correction_bound);
    c.set(4, c.r(0) >> 31);
    c.set(0, truncated_interval(c.r(0)));
    c.set(3, c.u32(stack + 0xa4));
    c.set(14, c.r(14) + c.r(0));
    c.set(0, c.r(3) - 1);
    c.compare(0, std::int32_t(c.r(14)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 4)) return;

    c.set_f(1, c.f32(c.r(31) + 0x3c));
    c.set_f(0, c.f32(c.r(31) + 0x2c));
    c.set_f(0, fp.multiply_add(0x1023ec30, c.f(1), c.f(29), c.f(0)));
    c.put64(stack + 0xa8, fp.fctiwz(0, c.f(0)));
    c.set(3, c.u32(stack + 0xac));
    c.set(14, c.r(3) - 1);
}

static void sample_contact(Cpu& c) {
    Fpu fp(c);
    c.set_f(2, c.f32(c.r(31) + 0x3c));
    c.set_f(1, c.f32(c.r(31) + 0x2c));
    c.set_f(0, c.f32(c.r(31)));
    c.set_f(1, fp.multiply_add(0x1023ea78, c.f(2), c.f(29), c.f(1)));
    fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 2) {
        c.set(14, 0);
        return;
    }

    advance_stream(c, 0x1023eaac);
    if (compare_contact_bound(c, fp, 0x1023eac4, 65535) > 65535 ||
        compare_contact_bound(c, fp, 0x1023eaec, -65535) < -65535)
        sample_wide_contact(c, fp);
    else sample_narrow_contact(c, fp);
}

static void emit_early_contact(Cpu& c, std::uint32_t actor) {
    const auto stack = c.sp();
    c.set(0, c.u8(stack + contact_column_slot));
    c.set(5, actor);
    c.set(4, signed_byte(c.r(13)));
    c.set(6, 0);
    c.put32(stack + 0x38, c.r(0));
    c.set(7, 0);
    c.set(0, c.u8(stack + contact_row_slot));
    c.put32(stack + 0x3c, c.r(0));
    c.set(3, c.u32(actor + actor_match));
    c.set(8, c.u8(stack + contact_code_slot));
    c.set(9, c.u8(actor + actor_column));
    c.set(10, c.u8(actor + actor_row));
    c.call(0x101d68f0, 0x1023ece4);
}

void prepare_contact(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto stack = c.sp();
    c.s.pc = ball_action::contact_selection_phase;
    c.set(3, c.u32(actor + actor_match));
    c.set(4, actor);
    c.set(5, signed_byte(c.r(13)));
    c.set(6, stack + contact_column_slot);
    c.set(7, stack + contact_row_slot);
    c.call(0x101f50c0, 0x1023ea5c);
    c.put8(stack + contact_code_slot, c.r(3));
    c.set(0, std::uint32_t(c.i8(stack + contact_code_slot)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) {
        sample_contact(c);
        c.compare(0, std::int32_t(c.r(14)), 0);
        if (c.cr(0) & 2) {
            emit_early_contact(c, actor);
            c.s.pc = ball_action::action_execution_phase;
            return;
        }
    }
    c.set(0, std::uint32_t(c.i8(stack + contact_code_slot)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    c.s.pc = (c.cr(0) & 2) ? ball_action::action_execution_phase : ball_action::contact_resolution_phase;
}

void comment_on_player(Cpu& c, std::uint32_t actor, std::uint32_t subject, unsigned message, std::uint32_t back) {
    c.set(3, c.u32(actor + actor_match));
    c.set(4, message);
    c.set(7, c.u8(subject + actor_side));
    c.set(8, no_comment);
    c.set(3, c.u32(c.r(3) + match_commentary));
    c.set(9, no_comment);
    c.set(6, c.u8(subject + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + actor_match));
    c.call(0x10201180, back);
}

void comment_on_actor(Cpu& c, std::uint32_t actor, unsigned message, std::uint32_t back) {
    comment_on_player(c, actor, actor, message, back);
}

void comment_on_opposing_side_of(Cpu& c, std::uint32_t actor, std::uint32_t subject, unsigned message, std::uint32_t back) {
    c.set(0, c.u8(subject + actor_side));
    c.set(4, message);
    c.set(3, c.u32(actor + actor_match));
    c.set(6, no_comment);
    c.set(0, signed_byte(c.r(0)));
    c.set(5, c.u32(actor + actor_match));
    c.set(0, std::countl_zero(c.r(0)));
    c.set(3, c.u32(c.r(3) + match_commentary));
    c.set(0, c.r(0) >> 5);
    c.set(8, no_comment);
    c.set(7, c.r(0));
    c.set(9, no_comment);
    c.set(10, 0);
    c.call(0x10201180, back);
}
void comment_on_opposing_side(Cpu& c, std::uint32_t actor, unsigned message, std::uint32_t back) {
    comment_on_opposing_side_of(c, actor, actor, message, back);
}

enum class ContactMessageKind { actor, execution };

static void classify_contact_message(Cpu& c, ContactMessageKind kind, std::uint32_t refill_back) {
    const auto code = c.i8(c.sp() + (kind == ContactMessageKind::actor ? contact_code_slot : 0x40));
    c.set(16, no_comment);
    c.set(0, std::uint32_t(code));
    c.compare(0, code, 4);
    if (code == 4) {
        c.set(16, 0x1ff8);
        return;
    }
    if (code > 4) {
        c.compare(0, code, 6);
        const bool special = kind == ContactMessageKind::actor ? code == 6 : code < 6;
        if (special) {
            c.set(16, kind == ContactMessageKind::actor ? 0x1ff5 : 0x1ff6);
            return;
        }
    } else {
        c.compare(0, code, 2);
        if (code >= 2) {
            c.set(16, code == 2 ? 0x1ff9 : 0x1ff7);
            return;
        }
    }

    advance_stream(c, refill_back);
    c.set(3, c.u32(c.r(30)));
    c.set(4, 0x92490000u);
    c.set(5, c.u32(c.r(27)));
    c.set(4, 0x92492493u);
    c.set(6, c.u16(c.r(3)));
    c.set(0, c.u8(c.sp() + contact_row_slot));
    const auto sample = c.r(6) + c.r(5);
    c.set(5, sample);
    c.set(3, signed_byte(c.r(0)));
    const auto quotient = shift_signed_with_carry(c,
        high_product(sample, std::int32_t(0x92492493u)) + sample, 2);
    c.set(4, sample - truncated_interval(quotient) * 7);
    c.set(0, c.r(4) + 2);
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (c.cr(0) & 4) c.set(16, 0x210a);
}

using AreaComment = void (*)(Cpu&, std::uint32_t, unsigned, std::uint32_t);
static void maybe_comment_on_area(Cpu& c, std::uint32_t actor, AreaComment comment,
                           std::uint32_t refill_back, std::uint32_t zero_back,
                           std::uint32_t one_back) {
    c.set(0, signed_half(c.r(16)));
    c.compare(0, std::int32_t(c.r(0)), -1);
    if (!(c.cr(0) & 2)) return;

    advance_stream(c, refill_back);
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(27)));
    const auto sample = c.u16(c.r(3)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(3, sign);
    const auto outcome = std::rotl(((sample & 3) << 30) - sign, 2) + sign;
    c.set(0, outcome);
    c.compare(0, std::int32_t(outcome), 1);
    if (c.cr(0) & 2) comment(c, actor, 0x20ec, one_back);
    else if (c.cr(0) & 8) {
        c.compare(0, std::int32_t(outcome), 0);
        if (!(c.cr(0) & 8)) comment(c, actor, 0x20e9, zero_back);
    }
}

enum class AreaReactionKind { initial_contact, receiver, result };
struct AreaReactionSites {
    AreaReactionKind kind;
    unsigned tendency_register, draw_register;
    std::uint32_t draw_back, morale_draw_back, morale_back, response_back;
};

static void apply_area_reaction(Cpu& c, std::uint32_t actor, std::uint32_t recipient, AreaReactionSites sites) {
    const bool initial = sites.kind == AreaReactionKind::initial_contact;
    const bool receiver = sites.kind == AreaReactionKind::receiver;
    const auto affected_player = receiver ? recipient : actor;
    const auto responsible_player = initial ? 0 : receiver ? actor : recipient;
    if (initial) {
        c.set(4, c.u32(actor + actor_match));
        c.set(3, 5);
        c.set(0, 0);
        c.set(17, 0x1f7a);
        c.put8(c.r(4) + match_status, 5);
    } else {
        c.set(3, c.u32(actor + actor_match));
        c.set(0, 5);
        c.set(17, 0x1f7a);
        c.put8(c.r(3) + match_status, 5);
    }
    c.set(3, c.u32(actor + actor_match));
    c.put32(c.r(3) + 0x3110, responsible_player);
    c.set(3, std::uint32_t(c.i16(affected_player + 0x36)));
    c.set(0, c.r(3) - 500);
    c.put16(affected_player + 0x36, c.r(0));
    c.set(3, c.u32(affected_player + 0x6c));
    const auto tendency = c.i8(c.r(3) + 0x66);
    c.set(sites.tendency_register, std::uint32_t(tendency));
    c.compare(0, tendency, 0);
    draw_narrow_interval(c, tendency, sites.draw_register, sites.draw_back);
    c.compare(0, std::int32_t(c.r(sites.draw_register)), 0);
    if (!(c.cr(0) & 2)) return;

    advance_stream(c, sites.morale_draw_back);
    c.set(3, c.u32(c.r(30)));
    c.set(4, 0x66660000u);
    c.set(0, c.u32(c.r(27)));
    c.set(4, 0x66666667u);
    c.set(5, c.u16(c.r(3)));
    c.set(3, affected_player);
    const auto sample = c.r(5) + c.r(0);
    c.set(5, sample);
    const auto quotient = shift_signed_with_carry(c, high_product(sample, 0x66666667), 2);
    c.set(4, quotient >> 31);
    c.set(0, 0u - (sample - truncated_interval(quotient) * 10));
    c.set(4, signed_byte(c.r(0)));
    call_owned(c, 0x10288f30, sites.morale_back);
    c.set(0, signed_byte(c.r(3)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) {
        c.set(3, affected_player);
        c.set(4, 0);
        call_owned(c, 0x1022ba10, sites.response_back);
    }
}

static void publish_contact_status(Cpu& c, std::uint32_t actor, unsigned status) {
    c.set(3, c.u32(actor + actor_match));
    c.set(0, status);
    c.set(17, 0x1f79);
    c.put8(c.r(3) + match_status, status);
}

void resolve_contact(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto stack = c.sp();
    c.s.pc = ball_action::contact_resolution_phase;
    c.set(3, c.u32(actor + actor_match));
    c.set(4, actor);
    c.set(6, c.u8(stack + contact_column_slot));
    c.set(5, stack + contact_code_slot);
    c.set(7, c.u8(stack + contact_row_slot));
    c.set(9, stack + 0x4c);
    c.set(8, 0);
    c.call(0x101f5180, 0x1023ed14);
    comment_on_actor(c, actor, 0x1fe8, 0x1023ed3c);
    classify_contact_message(c, ContactMessageKind::actor, 0x1023edb0);

    const auto code = c.u8(stack + contact_code_slot);
    c.set(0, code);
    c.compare(0, code, 7);
    if (code == 7) publish_contact_status(c, actor, 7);
    else {
        const auto side = c.u8(actor + actor_side);
        c.set(0, side);
        c.compare(0, side, 1);
        check_area_coordinates(c, actor, side == 1, 0, 3, 0);
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (c.cr(0) & 2) publish_contact_status(c, actor, 6);
        else {
            maybe_comment_on_area(c, actor, comment_on_opposing_side, 0x1023eefc, 0x1023ef78, 0x1023efb4);
            apply_area_reaction(c, actor, 0, {AreaReactionKind::initial_contact, 18, 14,
                0x1023f014, 0x1023f178, 0x1023f1c0, 0x1023f1d4});
        }
    }
    c.s.pc = ball_action::contact_outcome_phase;
}

void comment_on_recipient(Cpu& c, std::uint32_t actor, std::uint32_t recipient, unsigned message, std::uint32_t back) {
    c.set(0, c.u8(actor + actor_side));
    c.set(4, message);
    c.set(3, c.u32(actor + actor_match));
    c.set(8, no_comment);
    c.set(0, signed_byte(c.r(0)));
    c.set(6, c.u8(recipient + 0x2f));
    c.set(0, std::countl_zero(c.r(0)));
    c.set(3, c.u32(c.r(3) + match_commentary));
    c.set(0, c.r(0) >> 5);
    c.set(5, c.u32(actor + actor_match));
    c.set(7, c.r(0));
    c.set(9, no_comment);
    c.set(10, 0);
    c.call(0x10201180, back);
}

void comment_on_side(Cpu& c, std::uint32_t actor, unsigned message, std::uint32_t back) {
    c.set(3, c.u32(actor + actor_match));
    c.set(4, message);
    c.set(7, c.u8(actor + actor_side));
    c.set(6, no_comment);
    c.set(3, c.u32(c.r(3) + match_commentary));
    c.set(8, no_comment);
    c.set(5, c.u32(actor + actor_match));
    c.set(9, no_comment);
    c.set(10, 0);
    c.call(0x10201180, back);
}

bool compare_signed_result(Cpu& c) {
    c.set(0, signed_byte(c.r(3)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) == 0;
}

std::uint32_t draw_opposition_reaction(Cpu& c, unsigned modulus, std::uint32_t multiplier,
                                      unsigned shift, std::uint32_t refill_back) {
    advance_stream(c, refill_back);
    c.set(4, c.u32(c.r(30)));
    c.set(3, (multiplier + 0x8000u) & 0xffff0000u);
    c.set(0, c.u32(c.r(27)));
    c.set(3, multiplier);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto quotient = shift_signed_with_carry(c, high_product(sample, std::int32_t(multiplier)), shift);
    c.set(3, quotient >> 31);
    const auto remainder = sample - truncated_interval(quotient) * modulus;
    c.set(0, remainder);
    return remainder;
}

static void load_opposing_team(Cpu& c, std::uint32_t actor, std::uint32_t subject) {
    c.set(0, c.u8(subject + actor_side));
    c.set(3, c.u32(actor + actor_match));
    const bool opponent_is_away = signed_byte(c.r(0)) == 0;
    c.set(0, opponent_is_away);
    c.compare(0, opponent_is_away, 0);
    c.set(3, c.u32(c.r(3) + 0x3118));
    c.set(3, c.u32(c.r(3) + (opponent_is_away ? 0x20 : 0x1c)));
}

void maybe_comment_on_opposition_reaction(Cpu& c, std::uint32_t actor, std::uint32_t subject,
                                          std::uint32_t back) {
    load_opposing_team(c, actor, subject);
    c.set(0, c.u32(c.r(3) + 0x2c));
    c.compare_logical(0, c.r(0), 0);
    if (c.cr(0) & 2) return;
    load_opposing_team(c, actor, subject);
    c.set(3, c.u32(c.r(3) + 0x2c));
    c.set(0, c.u8(c.r(3) + 0x119));
    c.compare(0, std::int32_t(c.r(0)), 8);
    if (!(c.cr(0) & 2)) return;

    c.set(3, c.u32(actor + actor_match));
    c.set(4, 0x20f5);
    c.set(5, c.u32(actor + actor_match));
    c.set(6, 0);
    c.set(3, c.u32(c.r(3) + match_commentary));
    c.set({{7, 0}, {8, no_comment}, {9, no_comment}, {10, 0}});
    c.call(0x10201180, back);
}

static void comment_on_contact_detail(Cpu& c, std::uint32_t actor) {
    c.set(0, signed_half(c.r(16)));
    c.compare(0, std::int32_t(c.r(0)), -1);
    if (!(c.cr(0) & 4)) return;

    advance_stream(c, 0x1023f298);
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(27)));
    const auto sample = c.u16(c.r(3)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(3, sign);
    const auto outcome = std::rotl(((sample & 3) << 30) - sign, 2) + sign;
    c.set(0, outcome);
    c.compare(0, std::int32_t(outcome), 0);
    if (c.cr(0) & 2) {
        const auto tendency = c.i8(c.sp() + contact_row_slot);
        c.set(18, std::uint32_t(tendency));
        c.compare(0, tendency, 0);
        draw_narrow_interval(c, tendency, 14, 0x1023f2fc);
        const auto threshold = draw_opposition_reaction(c, 20, 0x66666667, 3, 0x1023f458);
        c.compare(0, std::int32_t(c.r(14)), std::int32_t(threshold));
        if (c.cr(0) & 4) maybe_comment_on_opposition_reaction(c, actor, actor, 0x1023f53c);
        comment_on_actor(c, actor, 0x2109, 0x1023f564);
    }
    comment_on_actor(c, actor, c.r(16), 0x1023f58c);
    auto code = c.u8(c.sp() + contact_code_slot);
    c.set(0, code);
    c.compare(0, code, 6);
    bool response = c.cr(0) & 2;
    if (!response) {
        code = c.u8(c.sp() + contact_code_slot);
        c.set(3, code);
        c.set(0, std::uint8_t(code - 3));
        c.compare_logical(0, c.r(0), 1);
        response = !(c.cr(0) & 4);
    }
    if (!response) {
        code = c.u8(c.sp() + contact_code_slot);
        c.set(0, code);
        c.compare(0, code, 5);
        response = c.cr(0) & 2;
    }
    if (response) {
        c.set(3, c.u32(actor + actor_match));
        c.set(4, actor);
        c.set(5, 0);
        c.call(0x101f1530, 0x1023f5c8);
    }
}

static bool recipient_is_eligible(Cpu& c, std::uint32_t back) {
    c.set(3, c.r(14));
    call_owned(c, 0x10272150, back);
    return compare_signed_result(c);
}

static void follow_up_with_recipient(Cpu& c, std::uint32_t actor) {
    c.set(0, signed_half(c.r(17)));
    c.compare(0, std::int32_t(c.r(0)), 0x1f79);
    if (!(c.cr(0) & 2)) return;
    c.set(0, std::uint32_t(c.i8(c.sp() + 0x42)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (!(c.cr(0) & 2)) return;

    const auto code = c.u8(c.sp() + contact_code_slot);
    c.set(0, code);
    c.compare(0, code, 7);
    if (c.cr(0) & 2) {
        if (!recipient_is_eligible(c, 0x1023f6ec)) return;
        const auto side = c.u8(actor + actor_side);
        c.set(0, side);
        c.compare(0, side, 1);
        check_area_coordinates(c, c.r(14), side == 1, 0, 3, 0);
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (c.cr(0) & 2) comment_on_recipient(c, actor, c.r(14), 0x21e1, 0x1023f7d8);
        return;
    }

    c.set(4, c.u8(actor + actor_column));
    c.set(3, actor);
    call_owned(c, 0x102720a0, 0x1023f7e8);
    if (compare_signed_result(c) && recipient_is_eligible(c, 0x1023f7f8))
        comment_on_recipient(c, actor, c.r(14), 0x21df, 0x1023f838);
    else if (recipient_is_eligible(c, 0x1023f844))
        comment_on_recipient(c, actor, c.r(14), 0x21e0, 0x1023f884);
}

static void select_contact_recipient(Cpu& c, std::uint32_t actor) {
    c.set(0, signed_half(c.r(17)));
    c.compare(0, std::int32_t(c.r(0)), 0x1f7a);
    if (c.cr(0) & 2) return;

    c.set(0, c.u8(actor + actor_side));
    c.set(6, 0);
    c.set(3, c.u32(actor + actor_match));
    c.set(0, std::countl_zero(signed_byte(c.r(0))));
    c.set(5, c.u8(c.r(3) + match_status));
    c.set(0, c.r(0) >> 5);
    c.set(3, c.u32(actor + actor_match));
    c.set(4, c.r(0));
    c.call(0x101f3a30, 0x1023f5fc);
    c.set(14, c.r(3));
    c.compare(0, std::int32_t(c.r(14)), 0);
    if (c.cr(0) & 2) {
        comment_on_opposing_side(c, actor, c.r(17), 0x1023f63c);
        return;
    }

    const auto code = c.u8(c.sp() + contact_code_slot);
    c.set(0, code);
    c.compare(0, code, 7);
    if (c.cr(0) & 2) comment_on_recipient(c, actor, c.r(14), 0x21e2, 0x1023f684);
    else comment_on_opposing_side(c, actor, c.r(17), 0x1023f6c0);
    follow_up_with_recipient(c, actor);
}

static void maybe_comment_on_contact_end(Cpu& c, std::uint32_t actor) {
    const auto ninth = draw_opposition_reaction(c, 9, 0x38e38e39, 1, 0x1023f8b8);
    c.compare(0, std::int32_t(ninth), 0);
    if (!(c.cr(0) & 2)) return;

    comment_on_side(c, actor, 0x20df, 0x1023f91c);
    advance_stream(c, 0x1023f93c);
    c.set(4, c.u32(c.r(30)));
    c.set(3, 0x55550000u);
    c.set(0, c.u32(c.r(27)));
    c.set(3, 0x55555556);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto quotient = high_product(sample, 0x55555556);
    c.set(3, quotient);
    const auto third = sample - truncated_interval(quotient) * 3;
    c.set(0, third);
    c.compare(0, std::int32_t(third), 0);
    if (c.cr(0) & 2) comment_on_side(c, actor, 0x20e0, 0x1023f99c);

    const auto code = c.u8(c.sp() + contact_code_slot);
    c.set(3, code);
    c.set(0, std::uint8_t(code - 3));
    c.compare_logical(0, c.r(0), 3);
    if (c.cr(0) & 4) {
        c.set(0, signed_half(c.r(17)));
        c.compare(0, std::int32_t(c.r(0)), 0x1f7a);
        if (!(c.cr(0) & 2)) return;
    }

    advance_stream(c, 0x1023f9dc);
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(27)));
    c.set(3, c.u16(c.r(3)));
    const auto last = c.r(3) + c.r(0);
    const auto sign = last >> 31;
    c.set(3, sign);
    c.set(0, (last & 1u) ^ sign);
    c.s.wr<1>(reg::xer_ca, c.r(0) >= sign);
    c.set(0, c.r(0) - sign);
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) comment_on_side(c, actor, 0x20d9, 0x1023fa30);
}

void complete_contact(Cpu& c) {
    const auto actor = c.r(actor_register);
    const auto stack = c.sp();
    c.s.pc = ball_action::contact_outcome_phase;
    c.set(4, c.u8(actor + actor_column));
    c.set(0, signed_half(c.r(17)));
    c.set(3, c.u32(actor + actor_match));
    c.compare(0, std::int32_t(c.r(0)), 0x1f7a);
    c.set(0, 0);
    c.put8(c.r(3) + 0x791d, c.r(4));
    c.set(4, c.u8(actor + actor_row));
    c.set(3, c.u32(actor + actor_match));
    c.put8(c.r(3) + 0x791e, c.r(4));
    c.set(3, c.u8(actor + actor_side));
    c.set(4, c.u32(actor + actor_match));
    c.set(3, std::uint32_t(signed_byte(c.r(3)) == 0));
    c.put8(c.r(4) + 0x7924, c.r(3));
    c.set(3, c.u32(actor + actor_match) + 0x10000);
    c.put32(c.r(3) - 0x2000, 0);
    if (c.cr(0) & 2) comment_on_opposing_side(c, actor, c.r(17), 0x1023f26c);
    comment_on_contact_detail(c, actor);
    select_contact_recipient(c, actor);

    c.set(3, c.u32(actor + actor_match));
    c.set(4, c.r(13));
    c.call(0x101ee530, 0x1023f890);
    if (!compare_signed_result(c)) {
        maybe_comment_on_contact_end(c, actor);
        c.set(3, c.u32(actor + actor_match));
        c.set(4, actor);
        c.set_f(1, c.f32(stack + 0x4c));
        c.call(0x101d6780, 0x1023fa40);
        c.set(3, actor);
        c.set(4, 0);
        call_owned(c, 0x102884d0, 0x1023fa4c);
    }
    c.set(3, std::uint32_t(c.i16(actor + 0x36)));
    c.set(0, c.r(3) - 25);
    c.put16(actor + 0x36, c.r(0));
    c.s.pc = ball_action::return_phase;
}

struct ExecutionContact {
    std::uint32_t classification_back, comment_refill_back, comment_zero_back, comment_one_back;
    AreaReactionSites reaction;
    std::uint32_t next;
};

static void resolve_execution_contact(Cpu& c, const ExecutionContact& sites) {
    const auto actor = c.r(actor_register);
    const auto recipient = c.r(recipient_register);
    const bool receiver = sites.reaction.kind == AreaReactionKind::receiver;
    classify_contact_message(c, ContactMessageKind::execution, sites.classification_back);

    const auto code = c.u8(c.sp() + execution_contact_slot);
    c.set(0, code);
    c.compare(0, code, 7);
    if (code == 7) publish_contact_status(c, actor, 7);
    else {
        const auto side = c.u8(actor + actor_side);
        const auto side_for_bounds = receiver ? std::uint32_t(std::int8_t(side) == 0) : side;
        c.set(0, side_for_bounds);
        c.compare(0, std::int32_t(side_for_bounds), 1);
        // Both paths test the actor's cell. The receiver path switches
        // which end of the pitch counts as its area.
        check_area_coordinates(c, actor, side_for_bounds == 1, 0, 3, 0);
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (c.cr(0) & 2) publish_contact_status(c, actor, 6);
        else {
            maybe_comment_on_area(c, actor, receiver ? comment_on_side : comment_on_opposing_side,
                sites.comment_refill_back, sites.comment_zero_back, sites.comment_one_back);
            apply_area_reaction(c, actor, recipient, sites.reaction);
        }
    }
    c.s.pc = sites.next;
}

void resolve_receiver(Cpu& c) {
    c.s.pc = ball_action::receiver_resolution_phase;
    resolve_execution_contact(c, {0x102401e8, 0x10240344, 0x102403b0, 0x102403dc,
        {AreaReactionKind::receiver, 18, 13, 0x10240438, 0x1024059c, 0x102405e4, 0x102405f8},
        ball_action::receiver_outcome_phase});
}

void resolve_result(Cpu& c) {
    c.s.pc = ball_action::result_resolution_phase;
    resolve_execution_contact(c, {0x102416f0, 0x1024183c, 0x102418b8, 0x102418f4,
        {AreaReactionKind::result, 19, 18, 0x10241950, 0x10241ab4, 0x10241afc, 0x10241b10},
        ball_action::result_outcome_phase});
}
}
