#include "match_pl_movement_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerMovement::penalize_transfer_contact(unsigned amount) {
    c.set(3, std::uint32_t(c.i16(actor.address + 0x36)));
    c.set(0, c.r(3) - amount);
    c.put16(actor.address + 0x36, c.r(0));
}

void PlayerMovement::react_to_transfer_contact() {
    penalize_transfer_contact(250);
    c.set(0, std::uint32_t(c.i8(c.u32(actor.address + 0x6c) + 0x66)));
    c.set(3, c.r(0) * 30);
    c.set(16, c.r(3) + 20);
    c.s.wr<1>(reg::xer_ca, c.r(3) > UINT32_MAX - 20);
    const auto bound = std::int32_t(c.r(16));
    if (compare(c, bound) == 0) c.set(14, 0);
    else {
        refill_carrier_pool(0x10233fe4);
        // The signed staff byte times thirty plus twenty cannot be wide.
        c.set(3, 65535);
        compare(c, bound, 65535);
        c.set(3, std::uint32_t(-65535));
        compare(c, bound, -65535);
        narrow_possession_draw(bound, 14);
    }
    if (compare(c, std::int32_t(c.r(14))) != 0) return;
    refill_carrier_pool(0x10234148);
    c.set(5, c.u16(c.u32(c.r(30))) + c.u32(c.r(27)));
    c.set(3, actor.address);
    c.set(4, high_product(c.r(5), 0x55555556));
    c.set(0, toward_zero(c.r(4)) * 3 - c.r(5));
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    c.call(0x10288f30, 0x1023418c);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) == 0) return;
    c.set({{3, actor.address}, {4, 0}});
    c.call(0x1022ba10, 0x102341a0);
}

bool PlayerMovement::transfer_statistics_required() {
    c.set(3, std::uint32_t(c.i16(frame.at(0x48))) - c.r(21));
    c.set(0, c.r(3) - 200);
    c.put16(c.r(22) + 0x1a0, c.r(0));
    clamp_play_bias(c.r(22));

    c.set({{0, std::uint32_t(c.i8(frame.at(0x46)))}, {16, 0}, {4, 100}});
    const auto direction = std::int32_t(c.r(0));
    const auto versus33 = compare(c, direction, 33);
    if (versus33 == 0 || (versus33 > 0 && compare(c, direction, 45) == 0)) c.set(4, 250);
    else if (versus33 < 0 && compare(c, direction, 19) == 0) c.set(4, 400);
    c.set({{3, std::uint32_t(c.i16(c.r(22) + 0x1a0))}, {0, c.r(4)}});
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) >= 0) return true;
    c.set({{3, actor.match()}, {0, c.u8(actor.match() + 0x791f)}});
    if (compare(c, std::int32_t(c.r(0)), 4) == 0) return true;
    c.set({{3, actor.match()}, {0, std::uint32_t(c.i8(actor.match() + 0x791f))}});
    return compare(c, std::int32_t(c.r(0)), -1) == 0;
}

void PlayerMovement::choose_distance_samples() {
    c.set(13, c.u8(c.r(22) + 0x10f));
    c.set(14, c.u8(c.r(22) + 0x110));
    refill_carrier_pool(0x102342ec);

    const auto cursor = c.u32(c.r(30));
    c.set(6, c.u16(cursor));
    c.set(5, c.r(6) + c.u32(c.r(27)));
    c.set(3, cursor + 2);
    c.set(0, c.u32(c.r(29)));
    c.put32(c.r(30), c.r(3));
    c.set(4, high_product(c.r(5), 0x66666667));
    const bool wrapped = compare_logical(c, c.r(3), c.r(0)) > 0;
    c.set(0, shift_signed(c.r(4), 2));
    c.set(3, c.r(0) >> 31);
    c.set(0, toward_zero(c.r(0)) * 10);
    c.set(16, c.r(5) - c.r(0));
    if (wrapped) {
        c.put32(c.r(30), c.r(28));
        c.call(0x1077b770, 0x10234344);
        c.set(0, c.r(3) & 0xffff);
        c.put32(c.r(27), c.r(0));
    }
}

void PlayerMovement::distance_sample_bound() {
    c.set(5, c.u32(c.r(27)));
    c.set(7, c.u16(c.u32(c.r(30))) + c.r(5));
    const auto x = std::uint32_t(std::int8_t(c.r(13)));
    const auto y = std::uint32_t(std::int8_t(c.r(14)));
    const auto mixed_x = x ^ c.r(16);
    c.set(6, y);
    c.set(4, shift_signed(high_product(c.r(7), 0x66666667), 2));
    c.set(5, c.r(4) >> 31);
    c.set(17, c.r(7) - toward_zero(c.r(4)) * 10);
    const auto mixed_y = y ^ c.r(17);
    c.set(5, shift_signed(mixed_y, 1));
    const auto half_x = shift_signed(mixed_x, 1);
    c.set(4, y & mixed_y);
    const auto sign_x = (half_x - (x & mixed_x)) >> 31;
    c.set(3, (c.r(5) - c.r(4)) >> 31);
    c.set(0, 3 - sign_x - c.r(3));
    c.s.wr<1>(reg::xer_ca, 1);
    compare(c, std::int32_t(c.r(0)));
}

void PlayerMovement::sample_contact_distance() {
    choose_distance_samples();
    distance_sample_bound();
    // Each original half-expression contributes only its sign bit.
    // The divisor is 3 minus those two bits, hence always in [1, 3].
    const auto bound = c.r(0);
    refill_carrier_pool(0x102343e0);
    c.set(3, 65535);
    compare(c, std::int32_t(bound), 65535);
    c.set(3, std::uint32_t(-65535));
    compare(c, std::int32_t(bound), -65535);

    const auto y = std::uint32_t(std::int8_t(c.r(14)));
    const auto mixed_y = y ^ c.r(17);
    c.set(6, std::uint32_t(std::int32_t(mixed_y) >> 1));
    c.set(4, c.r(6) - (y & mixed_y));
    c.set(3, c.r(4) >> 31);
    c.set(8, bound);
    c.set(9, c.u16(c.u32(c.r(30))) + c.u32(c.r(27)));
    c.set(7, std::uint32_t(divide_signed<4>(c.r(9), bound)));
    c.set(5, c.r(7) * bound);
    c.s.wr<1>(reg::xer_ca, 1);
    compare(c, std::int32_t(bound));
    c.set(0, c.r(9) - c.r(5));
    c.set(18, c.r(0));
    c.put8(c.r(22) + 0x2c, c.r(18));
    c.set(16, 1);
}

void PlayerMovement::position_after_transfer() {
    c.set({{3, actor.match()}, {0, std::uint32_t(c.i8(actor.match() + 0x791f))}});
    const auto phase = std::int32_t(c.r(0));
    if (compare(c, phase, 4) < 0 && compare(c, phase, -1) > 0) c.put8(actor.match() + 0x791f, 0);
    c.set(0, std::uint32_t(c.i8(c.r(22) + 0x27)));
    bool toward_end = compare(c, std::int32_t(c.r(0))) == 0;
    if (toward_end) {
        c.set({{3, actor.match()}, {0, std::uint32_t(c.i8(actor.match() + 0x791f))}});
        toward_end = compare(c, std::int32_t(c.r(0)), -1) != 0;
    }
    const auto row = actor.row();
    c.set({{0, std::uint32_t(row)}, {3, c.r(22)}, {4, c.u8(actor.address + 0x105)},
           {5, c.u8(actor.address + 0x106)}, {6, 4}});
    const auto order = compare(c, row, toward_end ? 9 : 3);
    c.set(0, std::uint32_t(toward_end ? (order < 0 ? row + 3 : 11) : (order > 0 ? row - 3 : 0)));
    c.set(7, std::uint32_t(std::int8_t(c.r(0))));
    c.call(0x10237830, toward_end ? 0x10234788 : 0x102347c8);
}

void PlayerMovement::compute_transfer_ratio(bool shot) {
    const unsigned denominator = shot ? 7 : 4;
    c.set(0, c.u8(c.r(22) + denominator));
    if (compare_logical(c, c.r(0)) == 0) {
        c.set_f(30, c.f32(c.r(31)));
        return;
    }
    c.set({{5, c.u8(c.r(22) + denominator + 1)}, {4, 0x43300000},
           {3, c.u8(c.r(22) + denominator)}, {0, 0x43300000}});
    c.put32(frame.at(0xc0), c.r(5));
    c.set_f(2, c.f64(c.r(26) + 0xf0));
    c.put32(frame.at(0xbc), c.r(4));
    c.set_f(1, c.f64(c.r(26) + 0xf0));
    c.set_f(0, c.f64(frame.at(0xbc)));
    c.put32(frame.at(0xc8), c.r(3));
    c.set_f(2, fp.fsubs(c.f(0), c.f(2)));
    c.put32(frame.at(0xc4), c.r(0));
    c.set_f(0, fp.fsubs(c.f64(frame.at(0xc4)), c.f(1)));
    c.set_f(30, fp.fdivs(c.f(2), c.f(0)));
}

bool PlayerMovement::transfer_commentary_due(bool shot) {
    refill_carrier_pool(shot ? 0x1023486c : 0x102349a8);
    c.set(5, c.u16(c.u32(c.r(30))) + c.u32(c.r(27)));
    c.set(3, std::uint32_t(std::int8_t(c.r(13))));
    const auto quotient = shift_signed(high_product(c.r(5), 0x66666667), 3);
    c.set(4, c.r(5) - toward_zero(quotient) * 20);
    c.set(0, std::uint32_t(std::int8_t(c.r(4) + (shot ? 55 : 65))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) <= 0) return false;
    c.set(3, c.r(22));
    c.call(0x10237b10, shot ? 0x102348c0 : 0x102349fc);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) != 0) return false;
    c.set_f(0, c.f64(c.r(26) + 0x288));
    fp.fcmp(0, c.f(30), c.f(0));
    return (c.cr(0) & 4) != 0;
}

void PlayerMovement::narrate_transfer_statistics(bool shot) {
    compute_transfer_ratio(shot);
    if (!transfer_commentary_due(shot)) return;
    c.set(0, c.u8(c.r(22) + (shot ? 8 : 6)));
    if (compare_logical(c, c.r(0), shot ? 3 : 1) <= 0) return;
    if (shot) {
        c.set(0, c.u8(c.r(22) + 9));
        if (compare_logical(c, c.r(0)) == 0) return;
    }
    c.set(0, std::uint32_t(c.i16(c.r(22) + 0x36)));
    if (compare(c, std::int32_t(c.r(0)), 6750) <= 0) return;
    c.set(0, std::uint32_t(c.i8(c.r(22) + 0x100)));
    if (compare(c, std::int32_t(c.r(0))) != 0) return;
    narrate_repeated_pair(shot ? 0x21ce : 0x21cd, shot ? 0x1023492c : 0x10234a5c);
    c.set(0, 1);
    c.put8(c.r(22) + 0x100, 1);
}

bool PlayerMovement::transfer_role(std::uint32_t bit, std::uint32_t back) {
    c.set(3, c.r(22));
    c.call(0x10289560, back);
    c.set(0, c.r(3) & bit);
    return compare(c, std::int32_t(c.r(0))) != 0;
}

void PlayerMovement::choose_transfer_rating() {
    c.set(4, c.u8(c.r(22) + 5));
    c.set(0, c.r(4) + 1);
    c.put8(c.r(22) + 5, c.r(0));
    const bool role = transfer_role(4, 0x10234a78) || transfer_role(2, 0x10234a88);
    c.set(0, std::uint32_t(std::int8_t(c.r(16))));
    const bool distance = compare(c, std::int32_t(c.r(0))) != 0;
    c.set(0, (distance ? 120 : 200) * (role ? 1 : 2));
    c.set(13, c.r(0));
    if (play_rule(0x10272050, 0x10234ac8) || play_rule(0x10272100, 0x10234ad8)) return;
    c.set({{3, c.r(13) ^ 0x80000000}, {0, 0x43300000}});
    c.put32(frame.at(0xbc), c.r(0));
    c.set_f(1, c.f64(c.r(26) + 0x80));
    c.put32(frame.at(0xc0), c.r(3));
    c.set_f(2, c.f64(c.r(26) + 0x1a0));
    c.set_f(0, fp.fsub(c.f64(frame.at(0xbc)), c.f(1)));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    c.put64(frame.at(0xc4), fp.fctiwz(0, c.f(0)));
    c.set(13, c.u32(frame.at(0xc8)));
}

bool PlayerMovement::credit_transfer_rating() {
    c.set(3, std::uint32_t(c.i16(c.r(22) + 0x36)));
    c.set(0, std::uint32_t(std::int8_t(c.r(19))));
    const bool credited = compare(c, std::int32_t(c.r(0))) != 0;
    c.set(0, c.r(3) + c.r(13));
    c.put16(c.r(22) + 0x36, c.r(0));
    if (credited) return true;
    c.set(3, c.r(22));
    c.call(0x10270ba0, 0x10234b30);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

void PlayerMovement::reward_transfer_rating() {
    c.set(3, c.u8(c.r(22) + 6));
    c.set(0, c.r(3) + 1);
    c.put8(c.r(22) + 6, c.r(0));
    c.set(3, std::uint32_t(c.i16(c.r(22) + 0x36)));
    c.set(0, c.r(3) + 300);
    c.put16(c.r(22) + 0x36, c.r(0));
    c.set(0, std::uint32_t(c.i8(c.r(22) + 0x17)));
    c.set(3, c.r(0) * 30);
    c.set(14, c.r(3) + 50);
    c.s.wr<1>(reg::xer_ca, c.r(3) > UINT32_MAX - 50);
    const auto bound = std::int32_t(c.r(14));
    // No signed byte makes this bound zero or takes the wide path.
    compare(c, bound);
    refill_carrier_pool(0x10234b8c);
    c.set(3, 65535);
    compare(c, bound, 65535);
    c.set(3, std::uint32_t(-65535));
    compare(c, bound, -65535);
    narrow_possession_draw(bound, 13);
    if (compare(c, std::int32_t(c.r(13))) != 0) return;

    refill_carrier_pool(0x10234cf0);
    c.set(5, c.u16(c.u32(c.r(30))) + c.u32(c.r(27)));
    c.set(3, c.r(22));
    const auto quotient = shift_signed(high_product(c.r(5), 0x66666667), 1);
    c.set(0, c.r(5) - toward_zero(quotient) * 5);
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    c.call(0x10288f30, 0x10234d34);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) == 0) return;
    c.set({{3, c.r(22)}, {4, 0}});
    c.call(0x1022ba10, 0x10234d48);
}

void PlayerMovement::finish_transfer_rating() {
    c.set(3, c.u8(actor.address + 0x2c));
    c.set(0, std::uint32_t(std::int8_t(c.r(16))));
    const bool distance = compare(c, std::int32_t(c.r(0))) != 0;
    c.set(0, c.r(3) + c.u8(frame.at(0x45)));
    c.put8(actor.address + 0x2c, c.r(0));
    if (!distance) return;
    refill_carrier_pool(0x10234d80);
    c.set(4, c.u16(c.u32(c.r(30))) + c.u32(c.r(27)));
    const auto quotient = shift_signed(high_product(c.r(4), 0x92492493) + c.r(4), 2);
    c.set(3, quotient >> 31);
    c.set(0, c.r(4) - toward_zero(quotient) * 7);
    if (compare(c, std::int32_t(c.r(0))) == 0) narrate_followup_contact(0x1f41, 0x10234de8, false);
}

void PlayerMovement::report_movement_diagnostic(unsigned first, unsigned second, unsigned code, std::uint32_t back) {
    c.set({{3, c.r(14) + 0x8f}, {6, frame.at(first)}, {7, frame.at(second)}, {4, 0}, {5, 0}});
    c.call(0x10739890, back);
    c.call(0x10768810, back + 4);
    c.set(5, c.r(3));
    c.set({{3, c.toc(-0x7ba8)}, {4, c.r(14) + 0x9c}, {6, frame.at(first)}, {7, frame.at(second)}, {8, code}});
    c.call(0x1077b720, back + 0x20);
    c.set({{4, c.toc(-0x7ba8)}, {3, c.r(14) + 0xa9}, {5, 0}});
    c.call(0x10769b60, back + 0x30);
    c.set({{3, c.toc(-0x7bac)}, {0, 0}});
    c.put8(c.r(3), 0);
}

std::uint32_t PlayerMovement::continue_carrier_transfer() {
    c.set(3, c.r(22));
    c.call(0x1022e3a0, 0x10233f84);
    c.set({{4, actor.side()}, {3, actor.address}});
    c.call(0x10239d10, 0x10233f90);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) != 0) react_to_transfer_contact();
    else penalize_transfer_contact(play_rule(0x10272050, 0x102341ac) ? 100 : 20);
    if (transfer_statistics_required()) return 0x102346f8;
    c.set({{0, std::uint32_t(signed_side() == 0)}, {6, 1}, {3, actor.match()}, {8, 0},
           {4, c.u8(actor.address + 0x105)}, {5, c.u8(actor.address + 0x106)}, {9, 1}, {10, 0}});
    c.set(7, c.r(0));
    c.call(0x101f1100, 0x102342b4);
    c.set({{3, actor.match()}, {0, std::uint32_t(c.i8(actor.match() + 0x7928))}});
    return compare(c, std::int32_t(c.r(0))) == 0 ? 0x102342c4 : 0x102346e4;
}

void PlayerMovement::continue_carrier_distance(std::uint32_t phase) {
    if (phase == 0x102342c4) sample_contact_distance();
    else {
        c.set(0, std::uint32_t(std::int8_t(c.r(13))));
        if (compare(c, std::int32_t(c.r(0))) != 0) {
            c.set(3, actor.address);
            c.call(0x10288230, 0x102346f4);
        }
    }
}

void PlayerMovement::continue_transfer_statistics() {
    position_after_transfer();
    c.set(3, std::uint32_t(c.i16(actor.match() + 0x7948)));
    c.set(4, 0x2e8ba2e9);
    c.set(0, std::uint32_t(c.i8(frame.at(0x46))));
    const bool shot = compare(c, std::int32_t(c.r(0)), 21) == 0;
    c.set(0, shift_signed(high_product(c.r(3), c.r(4)), 1));
    c.set(3, c.r(0) >> 31);
    c.set(0, toward_zero(c.r(0)));
    c.set(13, std::uint32_t(std::int8_t(c.r(0))));
    narrate_transfer_statistics(shot);
}

void PlayerMovement::continue_transfer_rating() {
    choose_transfer_rating();
    if (credit_transfer_rating()) reward_transfer_rating();
    finish_transfer_rating();
}

void PlayerMovement::continue_null_contact() {
    report_movement_diagnostic(0x60, 0x50, 0x10e9, 0x10234e04);
}
}
