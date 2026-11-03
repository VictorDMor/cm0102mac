#include "match_pl_movement_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;

bool PlayerMovement::contact_draw_is_zero(std::uint32_t back, std::uint32_t magic, unsigned shift, unsigned divisor) {
    refill_carrier_pool(back);
    c.set(4, c.u32(c.r(30)));
    c.set(3, magic);
    c.set(0, c.u32(c.r(27)));
    c.set(4, c.u16(c.r(4)) + c.r(0));
    auto high = high_product(c.r(4), magic);
    if (std::int32_t(magic) < 0) high += c.r(4);
    const auto shifted = shift ? shift_signed(high, shift) : high;
    c.set(3, shift ? shifted >> 31 : high);
    c.set(0, c.r(4) - toward_zero(shifted) * divisor);
    return compare(c, std::int32_t(c.r(0))) == 0;
}

void PlayerMovement::describe_contact() {
    c.set(3, c.r(22));
    c.call(0x10289560, 0x1022fff0);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0)), 1) == 0) return;
    c.set({{3, actor.match()}, {4, 0x1f4d}, {5, actor.match()}, {10, 0}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{6, c.u8(actor.address + 0x2f)}, {7, actor.side()}, {8, c.u8(c.r(22) + 0x2f)},
           {9, c.u8(c.r(22) + 0x27)}});
    c.call(0x10201180, 0x10230024);
}

bool PlayerMovement::evaluate_carrier_contact() {
    c.set(3, std::uint32_t(c.i16(frame.at(0x48))));
    c.set(0, std::uint32_t(std::int16_t(c.r(21))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) < 0 &&
        !contact_draw_is_zero(0x10230054, 0x66666667, 1, 5)) {
        c.set(0, 0);
        c.put8(frame.at(0x47), 0);
        return false;
    }
    c.set({{3, actor.match()}, {4, c.r(22)}, {6, c.u8(frame.at(0x46))}, {5, actor.address},
           {8, frame.at(0x42)}, {9, frame.at(0x41)}, {10, frame.at(0x40)}, {7, UINT32_MAX}});
    c.call(0x101f5050, 0x102300b4);
    c.put8(frame.at(0x47), c.r(3));
    c.set(3, actor.match());
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x7931)));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

bool PlayerMovement::marked_contact_goal_area(std::uint32_t player, std::uint32_t back) {
    c.set(0, signed_side());
    c.set(3, player);
    c.set(0, c.r(0) == 0);
    c.set(4, c.r(0));
    c.call(0x10239d10, back);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

bool PlayerMovement::contact_requires_warning() {
    if (!marked_contact_goal_area(c.r(22), 0x102300e4)) return false;
    c.set({{3, actor.match()}, {4, c.u8(frame.at(0x46))}});
    c.call(0x101ee530, 0x102300f8);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) != 0) return true;
    c.set(0, std::uint32_t(c.i8(frame.at(0x40))));
    if (compare(c, std::int32_t(c.r(0))) != 0) return true;

    c.set(24, std::uint32_t(c.i16(c.r(22) + 0x32)));
    const int bound = std::int32_t(c.r(24));
    if (compare(c, bound) == 0) c.set(23, 0);
    else {
        refill_carrier_pool(0x10230140);
        // The original loads the bound with lha before the two wide checks.
        c.set(3, 65535);
        compare(c, bound, 65535);
        c.set(3, std::uint32_t(-65535));
        compare(c, bound, -65535);
        c.set(3, c.u32(c.r(30)));
        const bool positive = compare(c, bound) > 0;
        c.set(0, c.u32(c.r(27)));
        c.set(3, c.u16(c.r(3)) + c.r(0));
        const auto remainder = c.r(3) - std::uint32_t(divide_signed<4>(c.r(3), c.r(24))) * c.r(24);
        c.set(0, positive ? remainder : 0u - remainder);
        c.set(23, c.r(0));
    }
    return compare(c, std::int32_t(c.r(23)), -10) < 0;
}

void PlayerMovement::describe_carrier_warning() {
    if (contact_draw_is_zero(0x102302a4, 0x55555556, 0, 3)) return;
    c.set({{3, actor.match()}, {4, 0x2168}, {7, c.u8(c.r(22) + 0x27)}, {8, UINT32_MAX}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{9, UINT32_MAX}, {6, c.u8(c.r(22) + 0x2f)}, {10, 0}, {5, actor.match()}});
    c.call(0x10201180, 0x10230304);
}

bool PlayerMovement::contact_frame_byte(unsigned slot) {
    c.set(0, std::uint32_t(c.i8(frame.at(slot))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

bool PlayerMovement::resolve_contact_directly() {
    if (!contact_frame_byte(0x47) || !contact_draw_is_zero(0x1023033c, 0x88888889, 4, 30) ||
        contact_frame_byte(0x43)) return false;
    c.set({{0, c.u8(frame.at(0x42))}, {5, c.r(22)}, {6, actor.address}});
    c.put32(frame.at(0x38), c.r(0));
    c.set(0, c.u8(frame.at(0x41)));
    c.put32(frame.at(0x3c), c.r(0));
    c.set({{4, c.u8(frame.at(0x46))}, {3, actor.match()}, {7, c.u8(frame.at(0x43))}});
    c.set({{4, std::uint32_t(std::int8_t(c.r(4)))}, {8, c.u8(frame.at(0x47))},
           {9, c.u8(actor.address + 0x105)}, {10, c.u8(actor.address + 0x106)}});
    c.call(0x101d68f0, 0x102303c0);
    return true;
}

void PlayerMovement::publish_contact_outcome(bool failed) {
    if (failed && contact_frame_byte(0x40) &&
        (marked_contact_goal_area(actor.address, 0x102316f0) || contact_frame_byte(0x43))) c.set(13, 1);

    c.set({{0, std::uint32_t(c.i8(frame.at(0x46)))}, {4, c.r(22)}});
    c.put16(c.r(22) + 0x19c, c.r(0));
    c.set(3, c.u8(c.r(22) + 4));
    c.set(0, c.r(3) + 1);
    c.put8(c.r(22) + 4, c.r(0));
    c.set(3, actor.match());
    c.call(0x101e3d50, 0x1023172c);
    c.set(0, c.r(3) & 0x30);
    bool lighter = compare(c, std::int32_t(c.r(0))) != 0;
    if (lighter) {
        c.set({{3, actor.match()}, {4, c.r(22)}});
        c.call(0x101e3d50, 0x10231740);
        c.set(0, std::uint32_t(std::int16_t(c.r(3))) & 0x880);
        lighter = compare(c, std::int32_t(c.r(0))) != 0;
    }
    c.set(3, std::uint32_t(c.i16(c.r(22) + 0x36)));
    c.set(0, c.r(3) - (lighter ? 59 : 70));
    c.put16(c.r(22) + 0x36, c.r(0));
}

void PlayerMovement::queue_contact_decision() {
    c.set({{0, c.u8(frame.at(0x40))}, {4, c.r(22)}, {5, actor.address}, {6, frame.at(0x47)}});
    c.put32(frame.at(0x38), c.r(0));
    c.set({{10, frame.at(0x4c)}, {3, actor.match()}, {7, c.u8(frame.at(0x42))},
           {8, c.u8(frame.at(0x41))}, {9, c.u8(frame.at(0x43))}});
    c.call(0x101f5110, 0x102303fc);
}

void PlayerMovement::narrow_possession_draw(std::int32_t bound, unsigned destination) {
    c.set(3, c.u32(c.r(30)));
    const bool positive = compare(c, bound) > 0;
    c.set(0, c.u32(c.r(27)));
    c.set(3, c.u16(c.r(3)) + c.r(0));
    const auto divisor = std::uint32_t(bound);
    const auto remainder = c.r(3) - std::uint32_t(divide_signed<4>(c.r(3), divisor)) * divisor;
    c.set(0, positive ? remainder : 0u - remainder);
    c.set(destination, c.r(0));
}

void PlayerMovement::sample_possession_failure() {
    c.set_f(2, c.f32(c.r(31) + 0x3c));
    c.set_f(1, c.f32(actor.address + 0xe8));
    c.set_f(0, c.f32(c.r(31) + 0x90));
    c.set_f(0, fp.multiply_add(0x10231798, c.f(2), c.f(1), c.f(0)));
    c.put64(frame.at(0xbc), fp.fctiwz(0, c.f(0)));
    c.set(24, c.u32(frame.at(0xc0)));
    const auto bound = std::int32_t(c.r(24));
    if (compare(c, bound) == 0) {
        c.set(23, 0);
        return;
    }
    refill_carrier_pool(0x102317d8);
    c.set(3, 65535);
    bool wide = compare(c, bound, 65535) > 0;
    if (!wide) {
        c.set(3, std::uint32_t(-65535));
        wide = compare(c, bound, -65535) < 0;
    }
    if (wide) sample_wide_action(bound, 0x102318b0, 23);
    else narrow_possession_draw(bound, 23);
}

bool PlayerMovement::should_keep_possession() {
    c.set(0, std::uint32_t(c.i8(actor.address + 0x1d)));
    if (compare(c, std::int32_t(c.r(0))) != 0) return false;
    c.set(0, std::uint32_t(std::int8_t(c.r(20))));
    if (compare(c, std::int32_t(c.r(0)), 4) == 0) return true;
    c.set(0, std::uint32_t(std::int16_t(c.r(21))));
    if (compare(c, std::int32_t(c.r(0)), 1) >= 0) return true;
    sample_possession_failure();
    if (compare(c, std::int32_t(c.r(23))) != 0) return true;
    if (compare_logical(c, c.r(22)) == 0) return false;
    c.set(3, std::uint32_t(c.i16(frame.at(0x48))));
    c.set(0, std::uint32_t(std::int16_t(c.r(21))));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) >= 0;
}

void PlayerMovement::release_ball() {
    c.set({{3, actor.match()}, {0, 0}, {5, UINT32_MAX}, {7, 0}});
    c.put32(c.r(3) + 0x310c, 0);
    c.set({{0, 1}, {6, 0}, {4, 0}, {8, actor.match()}, {3, 0}});
    c.put8(frame.at(0x114), c.r(0));
    c.put8(c.r(8) + 0x7924, c.r(5));
    c.set(5, actor.match() + 0x10000);
    c.put32(c.r(5) - 0x2000, c.r(7));
    c.set(5, actor.match());
    c.put8(c.r(5) + 0x1c0, c.r(6));
    c.put16(actor.address + 0x2a, c.r(4));
    c.put16(actor.address + 0x1a0, c.r(3));

    refill_carrier_pool(0x102319a0);
    c.set(3, c.u32(c.r(30)));
    c.set(4, 0x1f42);
    c.set(0, c.u32(c.r(27)));
    c.set(8, UINT32_MAX);
    c.set(3, c.u16(c.r(3)));
    c.set({{9, UINT32_MAX}, {10, 0}});
    c.set(0, c.r(3) + c.r(0));
    c.set(3, std::uint32_t(std::int32_t(c.r(0)) % 2));
    c.set(0, c.r(3) + 1);
    c.put8(actor.address + 0x2c, c.r(0));
    c.set(3, std::uint32_t(c.i16(actor.address + 0x36)));
    c.set(0, c.r(3) - 150);
    c.put16(actor.address + 0x36, c.r(0));
    c.set({{3, actor.match()}, {7, actor.side()}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{6, c.u8(actor.address + 0x2f)}, {5, actor.match()}});
    c.call(0x10201180, 0x10231a04);
}

void PlayerMovement::react_to_possession_release() {
    c.set(3, c.u32(actor.address + 0x6c));
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x66)));
    c.set(3, c.r(0) * 20);
    c.set(14, c.r(3) + 20);
    c.s.wr<1>(reg::xer_ca, c.r(3) > UINT32_MAX - 20);
    const auto bound = std::int32_t(c.r(14));
    if (compare(c, bound) == 0) c.set(13, 0);
    else {
        refill_carrier_pool(0x10231a44);
        // The signed staff byte times twenty plus twenty cannot be wide.
        c.set(3, 65535);
        compare(c, bound, 65535);
        c.set(3, std::uint32_t(-65535));
        compare(c, bound, -65535);
        narrow_possession_draw(bound, 13);
    }
    if (compare(c, std::int32_t(c.r(13))) != 0) return;
    refill_carrier_pool(0x10231ba8);
    c.set(3, c.u32(c.r(30)));
    c.set(4, 0x55555556);
    c.set(0, c.u32(c.r(27)));
    c.set(5, c.u16(c.r(3)));
    c.set(3, actor.address);
    c.set(5, c.r(5) + c.r(0));
    c.set(4, high_product(c.r(5), c.r(4)));
    c.set(0, toward_zero(c.r(4)) * 3 - c.r(5));
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    c.call(0x10288f30, 0x10231bec);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) == 0) return;
    c.set({{3, actor.address}, {4, 0}});
    c.call(0x1022ba10, 0x10231c00);
}

void PlayerMovement::adjust_play_condition() {
    c.set({{0, std::uint32_t(signed_side() == 0)}, {3, actor.address}});
    c.set(4, c.r(0));
    c.call(0x10239d10, 0x10231c38);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) != 0) return;
    const auto condition = c.i16(actor.address + 0x32);
    c.set(23, std::uint32_t(condition));
    if (compare(c, condition) == 0) c.set(13, 0);
    else {
        refill_carrier_pool(0x10231c74);
        // The signed halfword fits either narrow interval.
        c.set(3, 65535);
        compare(c, condition, 65535);
        c.set(3, std::uint32_t(-65535));
        compare(c, condition, -65535);
        narrow_possession_draw(condition, 13);
    }
    if (compare(c, std::int32_t(c.r(13)), -10) >= 0) return;
    c.set_f(1, c.f32(c.r(31) + 0x18));
    c.set(3, actor.address);
    c.call(0x102865a0, 0x10231dc4);
    c.set({{0, std::uint32_t(signed_side() == 0)}, {4, 0x211d}, {3, actor.match()},
           {6, UINT32_MAX}, {5, actor.match()}, {8, UINT32_MAX}});
    c.set({{3, c.u32(c.r(3) + 0x79f4)}, {7, c.r(0)}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, 0x10231dfc);
}

void PlayerMovement::count_team_play() {
    c.set({{6, actor.side()}, {3, actor.match()}, {0, signed_side()}});
    const bool unassigned = compare(c, std::int32_t(c.r(0)), -1) == 0;
    c.put8(c.r(3) + 0x7921, c.r(6));
    if (unassigned) return;
    const auto side = std::uint32_t(std::int8_t(c.r(6)));
    const auto opponent = std::uint32_t(side == 0);
    c.set({{5, c.r(3) + side}, {0, opponent}});
    c.set(4, c.u8(c.r(5) + 0x77bc) + 1);
    c.put8(c.r(5) + 0x77bc, c.r(4));
    c.set(4, c.r(3) + opponent);
    c.set(0, std::uint32_t(c.i8(c.r(4) + 0x77bc)));
    if (compare(c, std::int32_t(c.r(0))) <= 0) return;
    c.set(0, opponent);
    c.set(3, c.u8(c.r(4) + 0x77bc));
    c.set(0, c.r(3) - 10);
    c.put8(c.r(4) + 0x77bc, c.r(0));
}

std::uint32_t PlayerMovement::contact_direction_target(std::int16_t table_slot, std::uint32_t default_target) {
    c.set(0, std::uint32_t(c.i8(frame.at(0x46))));
    if (compare_logical(c, c.r(0), 46) > 0) return default_target;
    c.set(3, c.toc(table_slot));
    c.set(0, c.r(0) * 4);
    c.set(3, c.u32(c.r(3) + c.r(0)));
    c.s.wr<4>(reg::ctr, c.r(3));
    return c.r(3);
}

bool PlayerMovement::narrate_contact_direction(std::uint32_t target, std::uint32_t first_target) {
    constexpr std::array<std::uint32_t, 12> messages{
        0x1fea, 0x1feb, 0x1fec, 0x1fed, 0x1fee, 0x1ff2,
        0x1fef, 0x1ff1, 0x1ff0, 0x1ff3, 0x1ff4, 0x1fe9
    };
    const auto offset = target - first_target;
    if (offset > 0x2c * (messages.size() - 1) || offset % 0x2c != 0) return false;
    c.set({{3, actor.match()}, {4, messages[offset / 0x2c]}, {5, actor.match()}, {10, 0}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{6, c.u8(c.r(22) + 0x2f)}, {7, c.u8(c.r(22) + 0x27)},
           {8, c.u8(actor.address + 0x2f)}, {9, actor.side()}});
    c.call(0x10201180, target + 0x28);
    return true;
}

void PlayerMovement::select_contact_result() {
    c.set({{0, std::uint32_t(c.i8(frame.at(0x47)))}, {16, UINT32_MAX}});
    const auto outcome = std::int32_t(c.r(0));
    const auto versus4 = compare(c, outcome, 4);
    if (versus4 == 0) { c.set(16, 0x1ff8); return; }
    if (versus4 > 0) {
        if (compare(c, outcome, 6) < 0) { c.set(16, 0x1ff6); return; }
    } else {
        const auto versus2 = compare(c, outcome, 2);
        if (versus2 == 0) { c.set(16, 0x1ff9); return; }
        if (versus2 > 0) { c.set(16, 0x1ff7); return; }
    }

    refill_carrier_pool(0x1023069c);
    c.set(6, c.u16(c.u32(c.r(30))));
    c.set(5, c.r(6) + c.u32(c.r(27)));
    c.set(3, std::uint32_t(c.i8(frame.at(0x41))));
    const auto quotient = shift_signed(high_product(c.r(5), 0x92492493) + c.r(5), 2);
    c.set(4, c.r(5) - toward_zero(quotient) * 7);
    c.set(0, c.r(4) + 2);
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0) c.set(16, 0x210a);
}

void PlayerMovement::narrate_random_contact() {
    refill_carrier_pool(0x10230760);
    const auto sample = c.u16(c.u32(c.r(30))) + c.u32(c.r(27));
    c.set({{3, sample >> 31}, {0, std::uint32_t(std::int32_t(sample) % 4)}});
    const auto versus1 = compare(c, std::int32_t(c.r(0)), 1);
    if (versus1 > 0 || (versus1 < 0 && compare(c, std::int32_t(c.r(0))) < 0)) return;
    c.set({{3, actor.match()}, {4, versus1 == 0 ? 0x20ecu : 0x20e9u}, {7, actor.side()}, {6, UINT32_MAX}});
    c.set({{3, c.u32(c.r(3) + 0x79f4)}, {8, UINT32_MAX}, {5, actor.match()}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, versus1 == 0 ? 0x102307f8 : 0x102307cc);
}

void PlayerMovement::set_match_contact_state(unsigned status, unsigned message) {
    c.set({{3, actor.match()}, {0, status}, {17, message}});
    c.put8(c.r(3) + 0x7928, c.r(0));
}

void PlayerMovement::apply_major_contact() {
    set_match_contact_state(5, 0x1f7a);
    c.set(3, actor.match());
    c.put32(c.r(3) + 0x3110, actor.address);
    c.set(3, std::uint32_t(c.i16(c.r(22) + 0x36)));
    c.set(0, c.r(3) - 500);
    c.put16(c.r(22) + 0x36, c.r(0));

    c.set(0, c.u8(c.u32(c.r(22) + 0x6c) + 0x66));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(14, c.r(3) + 6);
    c.s.wr<1>(reg::xer_ca, c.r(3) > UINT32_MAX - 6);
    const auto bound = std::int32_t(c.r(14));
    if (compare(c, bound) == 0) c.set(13, 0);
    else {
        refill_carrier_pool(0x10230858);
        // The signed staff byte plus six cannot be wide.
        c.set(3, 65535);
        compare(c, bound, 65535);
        c.set(3, std::uint32_t(-65535));
        compare(c, bound, -65535);
        narrow_possession_draw(bound, 13);
    }
    if (compare(c, std::int32_t(c.r(13))) != 0) return;
    refill_carrier_pool(0x102309bc);
    const auto sample = c.u16(c.u32(c.r(30))) + c.u32(c.r(27));
    c.set({{3, c.r(22)}, {0, std::uint32_t(-(std::int32_t(sample) % 8))}});
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    c.call(0x10288f30, 0x102309f8);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) == 0) return;
    c.set({{3, c.r(22)}, {4, 0}});
    c.call(0x1022ba10, 0x10230a0c);
}

bool PlayerMovement::report_staff_eligible(std::uint32_t player) {
    c.set(3, c.toc(-0x7be4));
    c.set(4, c.u32(player + 0x6c));
    c.set(0, c.u32(c.r(3)));
    c.set(3, c.u32(c.r(4)));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) < 0;
}

bool PlayerMovement::report_contact_eligible() {
    c.set(3, actor.match());
    c.set(0, c.u32(c.r(3) + 0x3114));
    if (compare_logical(c, c.r(0)) == 0) return false;
    c.set(0, c.u8(frame.at(0x47)));
    if (compare(c, std::int32_t(c.r(0)), 5) != 0 && compare(c, std::int32_t(c.r(0)), 4) != 0) return false;
    c.set(0, c.u8(frame.at(0x42)));
    if (compare(c, std::int32_t(c.r(0)), 10) != 0 && compare(c, std::int32_t(c.r(0)), 5) != 0 &&
        compare(c, std::int32_t(c.r(0)), 11) != 0) return false;
    for (int check = 0; check < 3; ++check) {
        c.set(0, c.u32(c.r(22) + 0x6c));
        if (compare_logical(c, c.r(0)) == 0) return false;
    }
    if (!report_staff_eligible(c.r(22))) return false;
    c.set(0, c.u32(actor.address + 0x6c));
    if (compare_logical(c, c.r(0)) != 0 && !report_staff_eligible(actor.address)) return false;
    c.set(0, c.u32(c.r(22) + 0xfc));
    if (compare_logical(c, c.r(0)) == 0) return false;
    c.set({{3, c.r(0)}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, 0x10230b64);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) != 0) return false;
    c.set(4, c.u32(c.r(22) + 0x6c));
    c.set(0, std::uint32_t(std::int8_t(c.r(13)) + 10));
    c.set(3, std::uint32_t(c.i8(c.r(4) + 0x67)));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) < 0;
}

bool PlayerMovement::report_contact_attribute(unsigned field, std::uint32_t back) {
    refill_carrier_pool(back);
    c.set(6, c.u16(c.u32(c.r(30))) + c.u32(c.r(27)));
    c.set(3, std::uint32_t(c.i8(c.u32(c.r(22) + 0x70) + field)));
    const auto quotient = shift_signed(high_product(c.r(6), 0x66666667), 1);
    c.set(5, quotient >> 31);
    c.set(4, c.r(6) - toward_zero(quotient) * 5);
    c.s.wr<1>(reg::xer_ca, c.r(13) <= 15);
    c.set(0, std::uint32_t(std::int8_t(15 - c.r(13) + c.r(4))));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0;
}

bool PlayerMovement::report_contact_severity() {
    refill_carrier_pool(0x10230c98);
    c.set(6, c.u16(c.u32(c.r(30))));
    c.set(5, c.r(6) + c.u32(c.r(27)));
    c.set(3, std::uint32_t(c.i16(c.u32(c.r(22) + 0x70) + 10)));
    const auto quotient = shift_signed(high_product(c.r(5), 0x2572fb07), 9);
    c.set(4, c.r(5) - toward_zero(quotient) * 3500);
    c.set(0, std::uint32_t(std::int16_t(c.r(4) + 3000)));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0;
}

bool PlayerMovement::report_contact_rating() {
    refill_carrier_pool(0x10230d08);
    c.set(5, c.u16(c.u32(c.r(30))) + c.u32(c.r(27)));
    c.set(3, std::uint32_t(std::int8_t(c.r(13))));
    c.set(4, c.r(5) - toward_zero(high_product(c.r(5), 0x55555556)) * 3);
    c.set(0, c.r(4) + 1);
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0;
}

void PlayerMovement::append_contact_report() {
    c.set({{0, 0}, {3, actor.address}});
    c.set(4, c.u32(c.u32(c.r(22) + 0x6c)));
    c.put32(frame.at(0xa0), c.r(4));
    c.set(4, c.u32(c.u32(c.r(22) + 0xfc)));
    c.put32(frame.at(0xa4), c.r(4));
    c.set(4, c.u32(c.u32(actor.address + 0x6c)));
    c.put32(frame.at(0xb8), c.r(4));
    c.put8(frame.at(0xac), 0);
    c.call(0x10283690, 0x10230d80);
    c.set({{8, 0}, {7, UINT32_MAX}, {6, 0x82}, {0, 0}, {4, frame.at(0xa0)}, {5, 0x1c}});
    c.put8(frame.at(0xad), c.r(3));
    c.put8(frame.at(0xae), 0);
    c.put32(frame.at(0xb0), UINT32_MAX);
    c.put16(frame.at(0xb4), 0x82);
    c.put32(frame.at(0xa8), 0);
    c.set(3, c.u32(actor.match() + 0x3114));
    c.call(0x10738d10, 0x10230db8);
}

void PlayerMovement::consider_contact_report() {
    c.set({{3, c.u32(c.toc(-0x7b3c))}, {5, 11}, {4, c.u32(c.r(22) + 0x70)}, {6, 2}});
    c.set_f(1, c.f32(c.r(31) + 4));
    c.call(0x10303560, 0x10230a7c);
    const auto rating = fp.fctiwz(0, c.f(1));
    c.set(0, actor.match());
    const bool has_match = compare_logical(c, c.r(0)) != 0;
    c.put64(frame.at(0xbc), rating);
    c.set(13, c.u32(frame.at(0xc0)));
    if (has_match && report_contact_eligible() && report_contact_attribute(0x1c, 0x10230ba8) &&
        report_contact_attribute(0x25, 0x10230c20) && report_contact_severity() && report_contact_rating())
        append_contact_report();
}

void PlayerMovement::mark_followup_direction() {
    c.set(0, c.u8(frame.at(0x46)));
    if (compare(c, std::int32_t(c.r(0)), 44) != 0 && compare(c, std::int32_t(c.r(0)), 47) != 0) return;
    if (!contact_frame_byte(0x40)) return;
    c.set(0, c.u8(frame.at(0x46)));
    const bool direction47 = compare(c, std::int32_t(c.r(0)), 47) == 0;
    c.set({{3, actor.match()}, {4, direction47 ? 0x21dau : 0x21d9u}, {5, actor.match()}, {10, 0}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{6, c.u8(actor.address + 0x2f)}, {7, actor.side()},
           {8, c.u8(c.r(22) + 0x2f)}, {9, c.u8(c.r(22) + 0x27)}});
    c.call(0x10201180, direction47 ? 0x10230e10 : 0x10230e3c);
    c.set(3, c.u8(frame.at(0x47)));
    c.set(0, (c.r(3) - 3) & 255);
    if (compare_logical(c, c.r(0), 3) > 0) {
        c.set(0, std::uint32_t(std::int16_t(c.r(17))));
        if (compare(c, std::int32_t(c.r(0)), 0x1f7a) != 0) return;
    }
    c.set(0, 1);
    c.put8(c.r(22) + 0x16, 1);
}

void PlayerMovement::narrate_followup_contact(std::uint32_t message, std::uint32_t back, bool for_actor, bool shirt) {
    const auto participant = for_actor ? actor.address : c.r(22);
    c.set({{3, actor.match()}, {4, message}, {7, c.u8(participant + 0x27)}, {8, UINT32_MAX}});
    c.set({{3, c.u32(c.r(3) + 0x79f4)}, {9, UINT32_MAX},
           {6, for_actor && !shirt ? UINT32_MAX : c.u8(participant + 0x2f)}, {10, 0}, {5, actor.match()}});
    c.call(0x10201180, back);
}

std::uint32_t PlayerMovement::opponent_contact_club() {
    c.set({{0, std::uint32_t(c.i8(c.r(22) + 0x27) == 0)}, {3, actor.match()}});
    const bool opponent = compare(c, std::int32_t(c.r(0))) != 0;
    c.set(3, c.u32(c.u32(c.r(3) + 0x3118) + (opponent ? 0x20 : 0x1c)));
    return c.r(3);
}

void PlayerMovement::narrate_contact_interest() {
    refill_carrier_pool(0x10230e84);
    const auto sample = c.u16(c.u32(c.r(30))) + c.u32(c.r(27));
    c.set({{3, sample >> 31}, {0, std::uint32_t(std::int32_t(sample) % 4)}});
    if (compare(c, std::int32_t(c.r(0))) != 0) return;
    c.set(14, std::uint32_t(c.i8(frame.at(0x41))));
    const auto bound = std::int32_t(c.r(14));
    if (compare(c, bound) == 0) c.set(13, 0);
    else {
        refill_carrier_pool(0x10230ee8);
        // The signed frame byte cannot take the wide RNG path.
        c.set(3, 65535);
        compare(c, bound, 65535);
        c.set(3, std::uint32_t(-65535));
        compare(c, bound, -65535);
        narrow_possession_draw(bound, 13);
    }
    refill_carrier_pool(0x10231044);
    c.set(4, c.u16(c.u32(c.r(30))) + c.u32(c.r(27)));
    const auto quotient = shift_signed(high_product(c.r(4), 0x66666667), 3);
    c.set(3, quotient >> 31);
    c.set(0, c.r(4) - toward_zero(quotient) * 20);
    if (compare(c, std::int32_t(c.r(13)), std::int32_t(c.r(0))) > 0) {
        c.set(0, c.u32(opponent_contact_club() + 0x2c));
        if (compare_logical(c, c.r(0)) != 0) {
            c.set(3, c.u32(opponent_contact_club() + 0x2c));
            c.set(0, c.u8(c.r(3) + 0x119));
            if (compare(c, std::int32_t(c.r(0)), 8) == 0) {
                c.set({{3, c.u32(actor.match() + 0x79f4)}, {4, 0x20f5}, {5, actor.match()},
                       {6, 0}, {7, 0}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
                c.call(0x10201180, 0x10231128);
            }
        }
    }
    narrate_followup_contact(0x2109, 0x10231150, false);
}

void PlayerMovement::narrate_restart(std::uint32_t message, std::uint32_t back, unsigned player_register, bool opponent) {
    c.set(0, opponent ? std::uint32_t(signed_side() == 0) : actor.side());
    c.set({{3, c.u32(actor.match() + 0x79f4)}, {4, message}, {5, actor.match()},
           {6, player_register ? c.u8(c.r(player_register) + 0x2f) : UINT32_MAX},
           {7, std::uint32_t(std::int8_t(c.r(0)))}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

bool PlayerMovement::restart_qualified(std::uint32_t back) {
    c.set(3, actor.address);
    c.call(0x10272150, back);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

void PlayerMovement::choose_restart_recipient() {
    c.set(0, std::uint32_t(std::int16_t(c.r(17))));
    if (compare(c, std::int32_t(c.r(0)), 0x1f7a) == 0) return;
    c.set({{3, actor.match()}, {4, actor.side()}, {5, c.u8(actor.match() + 0x7928)}, {6, 0}});
    c.call(0x101f3a30, 0x102311d8);
    c.set(13, c.r(3));
    if (compare(c, std::int32_t(c.r(13))) == 0) {
        narrate_restart(c.r(17), 0x1023120c, 0, false);
        return;
    }

    c.set(0, c.u8(frame.at(0x47)));
    const bool seventh = compare(c, std::int32_t(c.r(0)), 7) == 0;
    if (seventh) narrate_restart(0x21e2, 0x10231248, 13, false);
    else narrate_restart(c.r(17), 0x10231278, 0, false);

    c.set(0, std::uint32_t(std::int16_t(c.r(17))));
    if (compare(c, std::int32_t(c.r(0)), 0x1f79) != 0 || contact_frame_byte(0x40)) return;
    c.set(0, c.u8(frame.at(0x47)));
    if (compare(c, std::int32_t(c.r(0)), 7) == 0) {
        if (restart_qualified(0x102312a4) && !marked_contact_goal_area(actor.address, 0x102312c8))
            narrate_restart(0x21e1, 0x102312fc, 13, false);
        return;
    }
    c.set({{4, c.u8(actor.address + 0x105)}, {3, actor.address}});
    c.call(0x102720a0, 0x1023130c);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) != 0 && restart_qualified(0x1023131c)) {
        narrate_restart(0x21df, 0x10231350, 13, false);
        return;
    }
    if (restart_qualified(0x1023135c)) narrate_restart(0x21e0, 0x10231390, 13, false);
}

bool PlayerMovement::restart_comment_expected() {
    c.set(3, c.u8(frame.at(0x47)));
    c.set(0, (c.r(3) - 3) & 255);
    if (compare_logical(c, c.r(0), 3) <= 0) return true;
    c.set(0, std::uint32_t(std::int16_t(c.r(17))));
    return compare(c, std::int32_t(c.r(0)), 0x1f7a) == 0;
}

bool PlayerMovement::even_restart_sample(std::uint32_t back) {
    refill_carrier_pool(back);
    const auto sample = c.u16(c.u32(c.r(30))) + c.u32(c.r(27));
    c.set(3, sample >> 31);
    c.set(0, std::uint32_t(std::int32_t(sample) % 2));
    return compare(c, std::int32_t(c.r(0))) == 0;
}

bool PlayerMovement::zero_restart_sample(std::uint32_t back, unsigned divisor, std::uint32_t multiplier) {
    refill_carrier_pool(back);
    c.set(4, c.u16(c.u32(c.r(30))) + c.u32(c.r(27)));
    const auto high = high_product(c.r(4), multiplier);
    const auto quotient = divisor == 9 ? shift_signed(high, 1) : high;
    c.set(3, divisor == 9 ? quotient >> 31 : high);
    c.set(0, c.r(4) - toward_zero(quotient) * divisor);
    return compare(c, std::int32_t(c.r(0))) == 0;
}

void PlayerMovement::narrate_flagged_restart() {
    // This call keeps r0 from the flag comparison live.
    narrate_followup_contact(0x20dd, 0x102313d8, true, true);
    c.set(0, c.u8(frame.at(0x47)));
    if (compare(c, std::int32_t(c.r(0)), 4) == 0 || compare(c, std::int32_t(c.r(0)), 5) == 0) {
        c.set(0, 1);
        c.put8(c.r(22) + 0x16, 1);
    }
    if (restart_comment_expected() && even_restart_sample(0x10231438))
        narrate_restart(0x20d9, 0x1023149c, 0, true);
}

void PlayerMovement::narrate_unflagged_restart() {
    if (!zero_restart_sample(0x102314c0, 9, 0x38e38e39)) return;
    narrate_restart(0x20df, 0x10231534, 0, true);
    if (zero_restart_sample(0x10231554, 3, 0x55555556))
        narrate_restart(0x20e0, 0x102315c4, 0, true);
    if (restart_comment_expected() && even_restart_sample(0x10231604))
        narrate_restart(0x20d9, 0x10231668, 0, true);
}

bool PlayerMovement::decide_carrier_contact() {
    if (compare_logical(c, c.r(22)) == 0) return false;
    describe_contact();
    if (evaluate_carrier_contact() && contact_requires_warning()) describe_carrier_warning();
    if (resolve_contact_directly()) {
        publish_contact_outcome(false);
        return false;
    }
    if (!contact_frame_byte(0x47)) {
        publish_contact_outcome(true);
        return false;
    }
    queue_contact_decision();
    return true;
}

void PlayerMovement::resume_carrier_contact_outcome(bool failed) {
    publish_contact_outcome(failed);
}

bool PlayerMovement::finish_carrier_possession() {
    if (should_keep_possession()) return true;
    release_ball();
    react_to_possession_release();
    return false;
}

bool PlayerMovement::prepare_carrier_play() {
    if (compare_logical(c, c.r(22)) != 0) {
        c.set({{0, std::uint32_t(c.i16(frame.at(0x48)))},
               {3, std::uint32_t(std::int16_t(c.r(21)))}});
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) <= 0) return false;
    }
    adjust_play_condition();
    count_team_play();
    return true;
}

std::uint32_t PlayerMovement::choose_contact_action(std::uint32_t target) {
    if (target == 0x102303fc) target = contact_direction_target(-0x51f4, 0x10230604);
    if (!narrate_contact_direction(target, 0x10230420)) return target;
    select_contact_result();
    c.set(0, c.u8(frame.at(0x47)));
    if (compare(c, std::int32_t(c.r(0)), 7) == 0) set_match_contact_state(7, 0x1f79);
    else {
        c.set({{0, std::uint32_t(signed_side() == 0)}, {3, actor.address}});
        c.set(4, c.r(0));
        c.call(0x10239d10, 0x1023072c);
        c.set(0, std::uint32_t(std::int8_t(c.r(3))));
        if (compare(c, std::int32_t(c.r(0))) == 0) set_match_contact_state(6, 0x1f79);
        else {
            c.set(0, std::uint32_t(std::int16_t(c.r(16))));
            if (compare(c, std::int32_t(c.r(0)), -1) == 0) narrate_random_contact();
            apply_major_contact();
        }
    }
    return 0x10230a20;
}

void PlayerMovement::follow_up_carrier_contact() {
    c.set(0, std::uint32_t(std::int16_t(c.r(17))));
    if (compare(c, std::int32_t(c.r(0)), 0x1f7a) == 0)
        narrate_followup_contact(c.r(17), 0x10230a54, true);
    c.set(0, std::uint32_t(std::int16_t(c.r(16))));
    if (compare(c, std::int32_t(c.r(0)), -1) <= 0) return;
    consider_contact_report();
    mark_followup_direction();
    narrate_contact_interest();
    narrate_followup_contact(c.r(16), 0x10231178, false);
    c.set(0, c.u8(frame.at(0x47)));
    if (compare(c, std::int32_t(c.r(0)), 6) != 0) {
        c.set(3, c.u8(frame.at(0x47)));
        c.set(0, (c.r(3) - 3) & 255);
        if (compare_logical(c, c.r(0), 1) > 0) {
            c.set(0, c.u8(frame.at(0x47)));
            if (compare(c, std::int32_t(c.r(0)), 5) != 0) return;
        }
    }
    c.set({{3, actor.match()}, {4, c.r(22)}, {5, 0}});
    c.call(0x101f1530, 0x102311b4);
}

void PlayerMovement::apply_carrier_restart() {
    choose_restart_recipient();
    c.set({{3, actor.match()}, {4, c.u8(frame.at(0x46))}});
    c.call(0x101ee530, 0x1023139c);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) == 0) {
        if (contact_frame_byte(0x40)) narrate_flagged_restart();
        else narrate_unflagged_restart();
        c.set({{3, actor.match()}, {4, c.r(22)}});
        c.set_f(1, c.f32(frame.at(0x4c)));
        c.call(0x101d6780, 0x10231678);
        c.set({{3, c.r(22)}, {4, 0}});
        c.call(0x102884d0, 0x10231684);
    }
    c.set(0, 0);
    for (const auto [player_field, match_field] : {std::pair{0x105u, 0x791du}, {0x106u, 0x791eu}, {0x27u, 0x7924u}}) {
        c.set({{4, c.u8(actor.address + player_field)}, {3, actor.match()}});
        c.put8(c.r(3) + match_field, c.r(4));
    }
    c.set(3, actor.match() + 0x10000);
    c.put32(c.r(3) - 0x2000, c.r(0));
    c.set(3, std::uint32_t(c.i16(c.r(22) + 0x36)));
    c.set(0, c.r(3) - 25);
    c.put16(c.r(22) + 0x36, c.r(0));
}
}
