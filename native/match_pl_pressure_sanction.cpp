#include "match_pl_pressure.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void CellPressure::announce_actor(unsigned message, std::uint32_t back) {
    c.set({{3, actor.match()}, {4, message}, {7, actor.side()}, {8, UINT32_MAX}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{9, UINT32_MAX}, {6, c.u8(actor.address + 0x2f)}, {10, 0}, {5, actor.match()}});
    c.call(0x10201180, back);
}

bool CellPressure::has_sanction() {
    c.set(0, std::uint32_t(c.i8(frame.at(0x40))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

bool CellPressure::defer_sanction() {
    refill(0x1023b0a0);
    c.set(4, c.u32(c.r(30)));
    c.set(3, 0x66666667);
    c.set(0, c.u32(c.r(28)));
    c.set(4, c.u16(c.r(4)) + c.r(0));
    const auto scaled = shifted_product(c.r(4), std::int32_t(c.r(3)), 2);
    c.set(3, unsigned(scaled < 0));
    c.set(0, c.r(4) - 10u * std::uint32_t(scaled + (scaled < 0)));
    if (compare(c, std::int32_t(c.r(0))) != 0) return false;
    c.set(0, c.u8(frame.at(0x42)));
    c.set({{4, c.r(23)}, {5, actor.address}, {6, 0}});
    c.put32(frame.at(0x38), c.r(0));
    c.set(7, 0);
    c.set(0, c.u8(frame.at(0x41)));
    c.put32(frame.at(0x3c), c.r(0));
    c.set({{3, actor.match()}, {8, c.u8(frame.at(0x40))}, {9, c.u8(actor.address + 0x105)},
           {10, c.u8(actor.address + 0x106)}});
    c.call(0x101d68f0, 0x1023b110);
    return true;
}

void CellPressure::prepare_sanction_comment() {
    c.set({{3, actor.match()}, {4, actor.address}, {6, c.u8(frame.at(0x42))}, {5, frame.at(0x40)},
           {7, c.u8(frame.at(0x41))}, {9, frame.at(0x44)}, {8, 0}});
    c.call(0x101f5180, 0x1023b140);
    announce_actor(0x1fe8, 0x1023b168);
    c.set(0, std::uint32_t(c.i8(frame.at(0x40))));
    c.set(24, UINT32_MAX);
    switch (std::int32_t(c.r(0))) {
    case 2: c.set(24, 0x1ff9); return;
    case 3: c.set(24, 0x1ff7); return;
    case 4: c.set(24, 0x1ff8); return;
    case 6: c.set(24, 0x1ff5); return;
    }
    refill(0x1023b1dc);
    c.set(3, c.u32(c.r(30)));
    c.set(4, 0x92492493);
    c.set(5, c.u32(c.r(28)));
    c.set(6, c.u16(c.r(3)));
    c.set(0, c.u8(frame.at(0x41)));
    c.set(5, c.r(6) + c.r(5));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    const auto scaled = shifted_product(c.r(5), std::int32_t(c.r(4)), 2);
    c.set(4, c.r(5) - 7u * std::uint32_t(scaled + (scaled < 0)));
    c.set(0, c.r(4) + 2);
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0) c.set(24, 0x210a);
}

void CellPressure::announce_goal_region_contact() {
    c.set(0, std::uint32_t(std::int16_t(c.r(24))));
    if (compare(c, std::int32_t(c.r(0)), -1) != 0) return;
    refill(0x1023b328);
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(28)));
    const auto draw = c.u16(c.r(3)) + c.r(0);
    c.set(3, draw >> 31);
    const auto choice = std::int32_t(draw) % 4;
    c.set(0, std::uint32_t(choice));
    const bool second = compare(c, choice, 1) == 0;
    if (!second && (choice >= 1 || compare(c, choice) < 0)) return;
    announce_other_side(second ? 0x20ec : 0x20e9, second ? 0x1023b3e0 : 0x1023b3a4);
}

void CellPressure::announce_other_side(unsigned message, std::uint32_t back) {
    c.set(0, std::uint32_t(std::int8_t(actor.side())));
    c.set(4, message);
    c.set({{3, actor.match()}, {6, UINT32_MAX}, {5, actor.match()}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(0, c.r(0) == 0);
    c.set({{8, UINT32_MAX}, {7, c.r(0)}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

void CellPressure::draw_signed_byte_into_r19(std::uint32_t back) {
    // The two callers place a sign-extended byte in r20.
    const auto bound = std::int32_t(c.r(20));
    if (compare(c, bound) == 0) {
        c.set(19, 0);
        return;
    }
    refill(back);
    c.set(3, 65535);
    compare(c, bound, 65535);
    c.set(3, std::uint32_t(-65535));
    compare(c, bound, -65535);
    c.set(3, c.u32(c.r(30)));
    compare(c, bound);
    c.set(0, c.u32(c.r(28)));
    c.set(3, c.u16(c.r(3)) + c.r(0));
    const auto remainder = std::uint32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
    c.set(0, bound > 0 ? remainder : 0u - remainder);
    c.set(19, c.r(0));
}

void CellPressure::apply_goal_region_sanction() {
    c.set({{4, actor.match()}, {3, 5}, {0, 0}, {22, 0x1f7a}});
    c.put8(c.r(4) + 0x7928, c.r(3));
    c.set(3, actor.match());
    c.put32(c.r(3) + 0x3110, c.r(0));
    c.set(3, std::uint32_t(c.i16(actor.address + 0x36)));
    c.set(0, c.r(3) - 500);
    c.put16(actor.address + 0x36, c.r(0));
    c.set(3, c.u32(actor.address + 0x6c));
    c.set(20, std::uint32_t(c.i8(c.r(3) + 0x66)));
    draw_signed_byte_into_r19(0x1023b440);
    if (compare(c, std::int32_t(c.r(19))) != 0) return;

    refill(0x1023b5a4);
    c.set(3, c.u32(c.r(30)));
    c.set(4, 0x66666667);
    c.set(0, c.u32(c.r(28)));
    c.set(5, c.u16(c.r(3)));
    c.set(3, actor.address);
    c.set(5, c.r(5) + c.r(0));
    const auto scaled = shifted_product(c.r(5), std::int32_t(c.r(4)), 2);
    c.set(4, unsigned(scaled < 0));
    c.set(0, 10u * std::uint32_t(scaled + (scaled < 0)) - c.r(5));
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    c.call(0x10288f30, 0x1023b5ec);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) == 0) return;
    c.set({{3, actor.address}, {4, 0}});
    c.call(0x1022ba10, 0x1023b600);
}

bool CellPressure::sanction_comment_lottery() {
    refill(0x1023b6c4);
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(28)));
    const auto draw = c.u16(c.r(3)) + c.r(0);
    c.set(3, draw >> 31);
    c.set(0, std::uint32_t(std::int32_t(draw) % 4));
    return compare(c, std::int32_t(c.r(0))) == 0;
}

std::uint32_t CellPressure::opposing_club() {
    c.set(0, actor.side());
    c.set(3, actor.match());
    c.set(0, std::int8_t(c.r(0)) == 0);
    const bool away = compare(c, std::int32_t(c.r(0))) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118));
    c.set(3, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    return c.r(3);
}

bool CellPressure::sanction_club_comment() {
    c.set(20, std::uint32_t(c.i8(frame.at(0x41))));
    draw_signed_byte_into_r19(0x1023b728);
    refill(0x1023b884);
    c.set(4, c.u32(c.r(30)));
    c.set(3, 0x66666667);
    c.set(0, c.u32(c.r(28)));
    c.set(4, c.u16(c.r(4)) + c.r(0));
    const auto scaled = shifted_product(c.r(4), std::int32_t(c.r(3)), 3);
    c.set(3, unsigned(scaled < 0));
    c.set(0, c.r(4) - 20u * std::uint32_t(scaled + (scaled < 0)));
    if (compare(c, std::int32_t(c.r(19)), std::int32_t(c.r(0))) <= 0) return false;
    c.set(0, c.u32(opposing_club() + 0x2c));
    if (compare_logical(c, c.r(0)) == 0) return false;
    c.set(3, c.u32(opposing_club() + 0x2c));
    c.set(0, c.u8(c.r(3) + 0x119));
    return compare(c, std::int32_t(c.r(0)), 8) == 0;
}

void CellPressure::announce_sanction_club() {
    c.set({{3, actor.match()}, {4, 0x20f5}, {5, actor.match()}, {6, 0}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{7, 0}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, 0x1023b968);
}

bool CellPressure::sanction_updates_lineup() {
    c.set(0, c.u8(frame.at(0x40)));
    if (compare(c, std::int32_t(c.r(0)), 6) == 0) return true;
    c.set(3, c.u8(frame.at(0x40)));
    c.set(0, std::uint8_t(c.r(3) - 3));
    if (compare_logical(c, c.r(0), 1) <= 0) return true;
    c.set(0, c.u8(frame.at(0x40)));
    return compare(c, std::int32_t(c.r(0)), 5) == 0;
}

bool CellPressure::find_restart_recipient() {
    c.set(0, std::uint32_t(std::int8_t(actor.side())));
    c.set({{6, 0}, {3, actor.match()}});
    c.set(5, c.u8(c.r(3) + 0x7928));
    c.set(0, c.r(0) == 0);
    c.set({{3, actor.match()}, {4, c.r(0)}});
    c.call(0x101f3a30, 0x1023ba28);
    c.set(29, c.r(3));
    return compare(c, std::int32_t(c.r(29))) != 0;
}

void CellPressure::announce_recipient(unsigned message, std::uint32_t back) {
    c.set(0, std::uint32_t(std::int8_t(actor.side())));
    c.set({{4, message}, {3, actor.match()}, {8, UINT32_MAX}});
    c.set(6, c.u8(c.r(29) + 0x2f));
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(0, c.r(0) == 0);
    c.set({{5, actor.match()}, {7, c.r(0)}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

bool CellPressure::recipient_in_attacking_band(std::uint32_t back) {
    c.set(3, c.r(29));
    c.call(0x10272150, back);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

void CellPressure::announce_restart_recipient() {
    c.set(0, std::uint32_t(std::int16_t(c.r(22))));
    if (compare(c, std::int32_t(c.r(0)), 0x1f7a) == 0) return;
    if (!find_restart_recipient()) {
        announce_other_side(c.r(22), 0x1023ba68);
        return;
    }
    c.set(0, c.u8(frame.at(0x40)));
    const bool penalty = compare(c, std::int32_t(c.r(0)), 7) == 0;
    if (penalty) announce_recipient(0x21e2, 0x1023bab0);
    else announce_other_side(c.r(22), 0x1023baec);

    c.set(0, std::uint32_t(std::int16_t(c.r(22))));
    if (compare(c, std::int32_t(c.r(0)), 0x1f79) != 0) return;
    c.set(0, c.u8(frame.at(0x40)));
    if (compare(c, std::int32_t(c.r(0)), 7) == 0) {
        if (!recipient_in_attacking_band(0x1023bb0c)) return;
        c.set(0, actor.side());
        const bool away = compare(c, std::int32_t(c.r(0)), 1) == 0;
        const Player recipient{c, c.r(29)};
        c.set(0, unsigned(inside_goal_region(recipient, away, 0, 3)));
        if (compare(c, std::int32_t(c.r(0))) == 0) announce_recipient(0x21e1, 0x1023bbf8);
        return;
    }
    c.set({{4, c.u8(actor.address + 0x105)}, {3, actor.address}});
    c.call(0x102720a0, 0x1023bc08);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) != 0 && recipient_in_attacking_band(0x1023bc18))
        announce_recipient(0x21df, 0x1023bc58);
    else if (recipient_in_attacking_band(0x1023bc64))
        announce_recipient(0x21e0, 0x1023bca4);
}

bool CellPressure::sanction_draw_zero(std::uint32_t back, unsigned divisor, std::int32_t magic, unsigned shift) {
    refill(back);
    c.set(4, c.u32(c.r(30)));
    c.set(3, std::uint32_t(magic));
    c.set(0, c.u32(c.r(28)));
    c.set(4, c.u16(c.r(4)) + c.r(0));
    const auto scaled = shift ? shifted_product(c.r(4), magic, shift)
        : std::int32_t((std::int64_t(std::int32_t(c.r(4))) * magic) >> 32);
    c.set(3, shift ? unsigned(scaled < 0) : std::uint32_t(scaled));
    c.set(0, c.r(4) - divisor * std::uint32_t(scaled + (scaled < 0)));
    return compare(c, std::int32_t(c.r(0))) == 0;
}

void CellPressure::announce_unidentified_actor(unsigned message, std::uint32_t back) {
    c.set({{3, actor.match()}, {4, message}, {7, actor.side()}, {6, UINT32_MAX}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{8, UINT32_MAX}, {5, actor.match()}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

void CellPressure::announce_sanction_effects() {
    if (!sanction_draw_zero(0x1023bcd8, 9, 0x38e38e39, 1)) return;
    announce_unidentified_actor(0x20df, 0x1023bd3c);
    if (sanction_draw_zero(0x1023bd5c, 3, 0x55555556, 0))
        announce_unidentified_actor(0x20e0, 0x1023bdbc);
    c.set(3, c.u8(frame.at(0x40)));
    c.set(0, std::uint8_t(c.r(3) - 3));
    if (compare_logical(c, c.r(0), 3) > 0) {
        c.set(0, std::uint32_t(std::int16_t(c.r(22))));
        if (compare(c, std::int32_t(c.r(0)), 0x1f7a) != 0) return;
    }
    refill(0x1023bdfc);
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(28)));
    const auto draw = c.u16(c.r(3)) + c.r(0);
    c.set(3, draw >> 31);
    c.set(0, std::uint32_t(std::int32_t(draw) % 2));
    if (compare(c, std::int32_t(c.r(0))) == 0) announce_unidentified_actor(0x20d9, 0x1023be50);
}

CellPressure::PressureContinuation CellPressure::route_sanction() {
    c.put8(frame.at(0x40), c.r(3));
    if (has_sanction() && defer_sanction()) return PressureContinuation::unsettled_contact;
    if (!has_sanction()) return PressureContinuation::unsettled_contact;
    prepare_sanction_comment();

    c.set(0, c.u8(frame.at(0x40)));
    if (compare(c, std::int32_t(c.r(0)), 7) == 0) {
        c.set({{3, actor.match()}, {0, 7}, {22, 0x1f79}});
        c.put8(c.r(3) + 0x7928, c.r(0));
        return PressureContinuation::penalty_restart;
    }
    c.set(0, actor.side());
    const bool away = compare(c, std::int32_t(c.r(0)), 1) == 0;
    c.set(0, unsigned(inside_goal_region(away, 0, 3)));
    if (compare(c, std::int32_t(c.r(0))) == 0) return PressureContinuation::outside_goal_region;
    announce_goal_region_contact();
    return PressureContinuation::goal_region;
}

bool CellPressure::publish_sanction(PressureContinuation route) {
    switch (route) {
    case PressureContinuation::unsettled_contact: return false;
    case PressureContinuation::goal_region: apply_goal_region_sanction(); break;
    case PressureContinuation::outside_goal_region:
        c.set({{3, actor.match()}, {0, 6}, {22, 0x1f79}});
        c.put8(c.r(3) + 0x7928, c.r(0));
        break;
    case PressureContinuation::penalty_restart: break;
    }

    c.set(4, c.u8(actor.address + 0x105));
    c.set(0, std::uint32_t(std::int16_t(c.r(22))));
    c.set(3, actor.match());
    const bool announce_restart = compare(c, std::int32_t(c.r(0)), 0x1f7a) == 0;
    c.set(0, 0);
    c.put8(c.r(3) + 0x791d, c.r(4));
    c.set(4, c.u8(actor.address + 0x106));
    c.set(3, actor.match());
    c.put8(c.r(3) + 0x791e, c.r(4));
    c.set(3, actor.side());
    c.set(4, actor.match());
    c.set(3, std::int8_t(c.r(3)) == 0);
    c.put8(c.r(4) + 0x7924, c.r(3));
    c.set(3, actor.match() + 0x10000);
    c.put32(c.r(3) - 0x2000, c.r(0));
    if (announce_restart) announce_other_side(c.r(22), 0x1023b698);
    return true;
}

void CellPressure::announce_published_sanction() {
    c.set(0, std::uint32_t(std::int16_t(c.r(24))));
    if (compare(c, std::int32_t(c.r(0)), -1) <= 0) return;
    if (sanction_comment_lottery()) {
        if (sanction_club_comment()) announce_sanction_club();
        announce_actor(0x2109, 0x1023b990);
    }
    announce_actor(c.r(24), 0x1023b9b8);
    if (sanction_updates_lineup()) {
        c.set({{3, actor.match()}, {4, actor.address}, {5, 0}});
        c.call(0x101f1530, 0x1023b9f4);
    }
}

void CellPressure::finish_published_sanction() {
    announce_restart_recipient();
    c.set({{3, actor.match()}, {4, std::uint32_t(std::int8_t(c.r(23)))}});
    c.call(0x101ee530, 0x1023bcb0);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) == 0) {
        announce_sanction_effects();
        c.set({{3, actor.match()}, {4, actor.address}});
        c.set_f(1, c.f32(frame.at(0x44)));
        c.call(0x101d6780, 0x1023be60);
        c.set({{3, actor.address}, {4, 0}});
        c.call(0x102884d0, 0x1023be6c);
    }
    c.set(3, 1);
}
}
