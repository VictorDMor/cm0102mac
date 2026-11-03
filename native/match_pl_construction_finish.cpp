#include "match_pl_construction.hpp"

namespace cm::aot::match_pl {
using namespace guest;
bool PlayerConstruction::apply_coach_influence() {
    const auto side = actor.side(); c.set(3, actor.match()); const bool away = compare(c, std::int8_t(side)) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118)); c.set(3, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    c.set({{4, 0}, {5, 1}, {6, 0}, {7, 0}}); c.call(0x1014c2e0, 0x10225748);
    if (compare(c, std::int8_t(c.r(3))) != 0) return false;
    const auto controller = [&] {
        const auto live_side = std::int8_t(actor.side()); c.set(4, actor.match());
        c.set(3, std::uint32_t(std::int32_t(live_side)) * 0xd8 + 0x7aac); c.set(3, c.r(4) + c.r(3));
    };
    controller(); if (compare_logical(c, c.u32(c.r(3) + 0x20)) == 0) return false;
    controller(); c.set(3, c.u32(c.r(3) + 0x20));
    if (compare_logical(c, c.u32(c.r(3) + 0x1c)) == 0) return false;
    const auto first_side = std::int8_t(actor.side()); c.set(4, actor.side()); c.set(6, actor.match());
    c.set(3, std::uint32_t(std::int32_t(first_side)) * 0xd8); c.set(5, actor.match());
    const auto second_side = std::int8_t(c.r(4)); c.set(7, c.r(3) + 0x7aac); c.set(4, std::uint32_t(std::int32_t(second_side)) * 0xd8);
    c.set(7, c.r(6) + c.r(7)); c.set(3, c.r(4) + 0x7aac); c.set(4, c.u32(c.r(7) + 0x20)); c.set(3, c.r(5) + c.r(3));
    c.set(3, c.u32(c.r(3) + 0x20)); c.set(4, c.u32(c.r(4) + 0x1c)); c.set(3, c.u32(c.r(3) + 0x1c));
    c.set(4, c.u8(c.r(4) + 0x3b)); c.call(0x100d2f50, 0x102257e4); c.set(3, std::int32_t(std::int8_t(c.r(3))));
    const auto influence = c.r(3) - 10; c.s.wr<1>(reg::xer_ca, c.r(3) >= 10);
    Kept<14> bound(c, influence);
    // Signed-byte service result minus ten stays in [-138,117].
    Kept<13> selected(c, narrow_preference(std::int32_t(influence), 0x10225818));
    c.set(3, c.u8(actor.address + 0x103)); c.put8(actor.address + 0x103, c.r(3) + std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(selected)))));
    return true;
}

void PlayerConstruction::apply_fallback_influence() {
    if (compare(c, std::int8_t(c.r(19))) != 0) {
        advance(0x10225990); c.set(3, c.u32(cursor)); const auto bias = c.u32(seed); c.set(4, c.u16(c.r(3)));
        c.set(3, c.u8(actor.address + 0x103)); const auto draw = c.r(4) + bias; c.set(4, draw);
        const auto shifted = std::rotr(draw, 3) & 0xe0000000; const auto sign = draw >> 31; c.set(4, sign);
        const auto addition = std::rotl(shifted - sign, 3) + sign;
        c.put8(actor.address + 0x103, c.r(3) + std::uint32_t(std::int32_t(std::int8_t(addition))));
    } else {
        advance(0x102259f0); c.set(3, c.u32(cursor)); c.set(4, 0x2aab0000);
        const auto bias = c.u32(seed); c.set(4, 0x2aaaaaab); c.set(5, c.u16(c.r(3))); c.set(3, c.u8(actor.address + 0x103));
        const auto draw = c.r(5) + bias; c.set(5, draw);
        const auto product = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(draw)) * 0x2aaaaaab) >> 32); c.set(4, product);
        const auto remainder = draw - (std::uint32_t(product) + unsigned(product < 0)) * 6;
        c.put8(actor.address + 0x103, c.r(3) + std::uint32_t(std::int32_t(std::int8_t(remainder))));
    }
}

bool PlayerConstruction::own_competition_has_history() {
    if (compare_logical(c, c.u32(actor.address + 0xfc)) == 0) return false;
    c.set(3, c.u32(actor.address + 0xfc)); if (compare_logical(c, c.u32(c.r(3) + 0x30)) == 0) return false;
    if (compare_logical(c, c.r(20)) == 0) return false;
    c.set(4, actor.match()); c.set(3, c.u32(actor.address + 0xfc)); c.set(4, c.u32(c.r(4) + 0x3118));
    const auto own_competition = c.u32(c.r(3) + 0x30); c.set(3, c.u32(c.r(4) + 0x14));
    if (compare_logical(c, c.r(3), own_competition) != 0) return false;
    if (compare(c, c.i8(c.r(20) + 0x44), 25) <= 0) return false;
    c.set(3, c.u32(actor.address + 0xfc)); c.set(4, c.u32(c.r(24))); c.set(3, c.u32(c.r(3) + 0x30));
    if (compare_logical(c, c.u32(c.r(4) + c.u32(c.r(3)) * 4)) == 0) return false;
    c.set(3, c.u32(actor.address + 0xfc)); c.set(4, c.u32(c.r(24))); c.set(3, c.u32(c.r(3) + 0x30));
    c.set(3, c.u32(c.r(4) + c.u32(c.r(3)) * 4)); return compare_logical(c, c.u32(c.r(3) + 8)) != 0;
}

void PlayerConstruction::apply_standings_pressure() {
    c.set(3, c.u32(actor.address + 0xfc)); c.set(5, c.u32(c.r(24))); c.set(3, c.u32(c.r(3) + 0x30)); c.set(4, c.u32(actor.address + 0xfc));
    c.set(3, c.u32(c.r(5) + c.u32(c.r(3)) * 4)); c.set(3, c.u32(c.r(3) + 8)); c.call(0x10459b50, 0x10225aec);
    if (compare_logical(c, c.r(3)) == 0) return;
    const auto exceeds = [&](int standing, int points) {
        return compare(c, c.i32(c.r(3) + 0x1c), standing) > 0 || compare(c, c.i32(c.r(3) + 0x44), points) > 0;
    };
    const auto temperament_below = [&](unsigned adjustment) {
        c.set(4, c.u32(c.r(3) + 0x1c)); c.set(5, c.u8(staff + 0x67));
        const auto limit = std::int8_t(c.r(4) + adjustment); c.set(4, std::int32_t(std::int8_t(c.r(5))));
        return compare(c, std::int32_t(c.r(4)), limit) < 0;
    };
    bool reduced = exceeds(5, 9) && temperament_below(10);
    if (reduced) { c.set(4, c.u8(actor.address + 0x103)); c.put8(actor.address + 0x103, c.r(4) - 1); }
    else if (exceeds(3, 7) && temperament_below(6)) {
        c.set(4, c.u8(actor.address + 0x103)); c.put8(actor.address + 0x103, c.r(4) - 1);
    }
    if (!exceeds(3, 5)) return;
    Kept<14> temperament(c, std::uint32_t(std::int32_t(c.i8(staff + 0x67))));
    Kept<13> first(c, narrow_preference(std::int32_t(std::uint32_t(temperament)), 0x10225bc4));
    bool reduce = compare(c, std::int32_t(std::uint32_t(first))) == 0;
    if (!reduce) {
        Kept<14> composure(c, std::uint32_t(std::int32_t(c.i8(staff + 0x66))));
        Kept<13> second(c, narrow_preference(std::int32_t(std::uint32_t(composure)), 0x10225d3c));
        reduce = compare(c, std::int32_t(std::uint32_t(second))) == 0;
    }
    if (reduce) { c.set(3, c.u8(actor.address + 0x103)); c.put8(actor.address + 0x103, c.r(3) - 1); }
}

void PlayerConstruction::apply_club_history_pressure() {
    c.set(3, c.u32(actor.address + 0xfc)); if (compare_logical(c, c.u32(c.r(3) + 0x90)) == 0) return;
    c.set(3, c.toc(-0x7b28)); c.set(4, c.u32(actor.address + 0xfc)); c.set(3, c.u32(c.r(3))); c.set(4, c.u32(c.r(4) + 0x90));
    c.call(0x102de3c0, 0x10225eb0); if (compare(c, std::int8_t(c.r(3))) == 0) return;
    Kept<14> temperament(c, std::uint32_t(std::int32_t(c.i8(staff + 0x67))));
    Kept<13> first(c, narrow_preference(std::int32_t(std::uint32_t(temperament)), 0x10225eec));
    bool reduce = compare(c, std::int32_t(std::uint32_t(first))) == 0;
    if (!reduce) {
        Kept<14> loyalty(c, std::uint32_t(std::int32_t(c.i8(staff + 0x65))));
        Kept<13> second(c, narrow_preference(std::int32_t(std::uint32_t(loyalty)), 0x10226064));
        reduce = compare(c, std::int32_t(std::uint32_t(second))) == 0;
    }
    if (reduce) { c.set(3, c.u8(actor.address + 0x103)); c.put8(actor.address + 0x103, c.r(3) - 1); }
}

void PlayerConstruction::apply_goalkeeper_context() {
    c.set(3, c.u32(actor.address + 0xfc)); c.set({{4, 0}, {5, 1}, {6, 0}, {7, 0}}); c.call(0x1014c2e0, 0x102261cc);
    if (compare(c, std::int8_t(c.r(3))) == 0) return;
    enter(c, 0x102261dc, {actor.address}); current_role(c, actor.address);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) { c.set(3, c.u8(actor.address + 0x103)); c.put8(actor.address + 0x103, c.r(3) + 1); }
}

void PlayerConstruction::apply_staff_context() {
    if (compare_logical(c, c.u32(actor.address + 0xfc)) == 0 || compare_logical(c, c.r(20)) == 0 || compare_logical(c, c.u32(c.r(24))) == 0) return;
    c.set(3, c.u32(actor.address + 0xfc)); if (compare_logical(c, c.u32(c.r(3) + 0x30)) == 0) return;
    c.set(3, c.u32(actor.address + 0xfc)); c.set(4, c.u32(c.r(24))); c.set(3, c.u32(c.r(3) + 0x30));
    if (compare_logical(c, c.u32(c.r(4) + c.u32(c.r(3)) * 4)) == 0) return;
    c.set(3, c.u32(actor.address + 0xfc)); c.set({{4, 0}, {5, 1}, {6, 0}, {7, 0}}); c.call(0x1014c2e0, 0x1022625c);
    if (compare(c, std::int8_t(c.r(3))) == 0 && compare(c, c.i8(actor.address + 0x103), 5) >= 0) return;
    if (compare(c, c.i32(c.r(20) + 0x50)) != 0 || compare(c, c.i8(c.r(20) + 0x4c), 25) <= 0) return;
    c.set(3, staff); c.set(4, 0); c.call(0x100d3150, 0x1022629c);
    if (compare(c, std::int8_t(c.r(3))) != 0) return;
    c.set(3, c.u32(actor.address + 0xfc)); c.set(4, c.toc(-0x7984)); const auto club_id = c.u32(c.r(3));
    c.set(3, c.u32(c.r(4))); c.set(3, c.r(3) + club_id * 8);
    if (compare(c, c.i8(c.r(3) + 5)) != 0 || compare(c, c.i8(staff + 0x67), 14) <= 0) return;
    Kept<13> loyalty(c, std::uint32_t(std::int32_t(c.i8(staff + 0x65))));
    // The original reverses the retained bound/result registers for this draw.
    Kept<14> selected(c, narrow_preference(std::int32_t(std::uint32_t(loyalty)), 0x1022630c));
    if (compare(c, std::int32_t(std::uint32_t(selected))) != 0) { c.set(3, c.u8(actor.address + 0x103)); c.put8(actor.address + 0x103, c.r(3) + 1); }
}

void PlayerConstruction::finish_attributes() {
    const auto personality = std::int32_t(c.i8(actor.address + 0x103)); c.set(4, c.u32(actor.address + 0x70)); c.set(3, personality);
    c.set(4, c.u8(c.r(4) + 0x25)); const auto rounded = std::uint32_t(personality) + unsigned(personality < 0);
    c.set(3, std::int32_t(std::int8_t(c.r(4))));
    c.s.wr<1>(reg::xer_ca, std::int32_t(rounded) < 0 && (rounded & 1));
    const auto halved = std::int32_t(rounded) >> 1; c.set(3, c.r(3) + std::uint32_t(halved));
    const auto adjusted = c.r(3) + 10; const auto quotient = std::int32_t(adjusted) >> 2;
    const auto carry = std::int32_t(adjusted) < 0 && (adjusted & 3); c.s.wr<1>(reg::xer_ca, carry);
    // addze consumes the arithmetic-shift carry and leaves the addition carry.
    const auto result = std::uint32_t(quotient) + unsigned(carry); c.s.wr<1>(reg::xer_ca, carry && result == 0);
    c.put8(actor.address + 0x102, result);
    const auto side = actor.side(); c.set(3, actor.match()); const bool away = compare(c, std::int8_t(side)) != 0;
    const auto kit = c.u8(c.r(3) + (away ? 0x77c6 : 0x77c5)); c.set(3, c.u32(actor.address + 0xfc));
    c.set(4, kit); c.set(5, at(0x3a)); c.set(6, at(0x38)); c.call(0x100c7a50, 0x102264c0);
    c.set(3, c.u16(at(0x38))); const bool original_away = compare(c, std::int8_t(side_argument)) != 0;
    c.put16(actor.address + 0x1c0, c.r(3)); c.put16(actor.address + 0x19e, original_away ? 180 : 0);
    c.set(4, 0); c.put8(actor.address + 0x2c, 0); c.set(3, 0);
    c.put8(actor.address + 0x105, 0); c.put8(actor.address + 0x106, 0); c.put16(actor.address + 0x19c, 101);
}
}
