#include "match_pl_movement_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerMovement::clamp_play_bias(std::uint32_t player) {
    c.set(0, std::uint32_t(c.i16(player + 0x1a0)));
    const auto bias = std::int32_t(c.r(0));
    if (compare(c, bias, 200) > 0) c.set(0, 200);
    else if (compare(c, bias, -200) < 0) c.set(0, std::uint32_t(-200));
    else return;
    c.put16(player + 0x1a0, c.r(0));
}

void PlayerMovement::play_bias_factor(unsigned spill, std::int32_t difference, std::uint32_t site) {
    c.put32(frame.at(spill), 0x43300000);
    c.put32(frame.at(spill + 4), std::uint32_t(difference) ^ 0x80000000);
    c.set_f(1, c.f64(c.r(26) + 0x80));
    c.set_f(3, c.f32(actor.address + 0xe8));
    c.set_f(2, c.f32(actor.address + 0x98));
    c.set_f(0, fp.fsubs(c.f64(frame.at(spill)), c.f(1)));
    if (spill == 0xbc) c.set_f(1, c.f32(c.r(31)));
    c.set_f(0, fp.multiply_add(site, c.f(3), c.f(2), c.f(0)));
}

void PlayerMovement::apply_play_bias() {
    c.set({{0, signed_side()}, {3, actor.match()}});
    c.set(4, c.r(3) + c.r(0));
    c.set(3, c.u8(c.r(4) + 0x77bc));
    c.set(0, c.r(3) + 5);
    c.put8(c.r(4) + 0x77bc, c.r(0));

    c.set(3, std::uint32_t(c.i16(frame.at(0x48))));
    const auto limit = std::int32_t(c.r(3)) + 50;
    c.set(0, std::uint32_t(limit));
    const bool capped = compare(c, limit, 250) <= 0;
    c.set(4, std::uint32_t(std::int16_t(capped ? 250 : limit)));
    const auto draw = std::int16_t(c.r(21));
    const auto difference = draw - std::int16_t(c.r(4));
    c.set({{3, std::uint32_t(difference)}, {0, std::uint32_t(difference) ^ 0x80000000}});
    play_bias_factor(0xbc, difference, 0x10231ed0);
    fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 8) c.set_f(0, c.f32(c.r(31)));
    else {
        c.set({{3, std::uint32_t(difference) ^ 0x80000000}, {0, 0x43300000}});
        play_bias_factor(0xc4, difference, 0x10231f14);
    }
    c.put64(frame.at(0xcc), fp.fctiwz(0, c.f(0)));
    c.set(4, c.u32(frame.at(0xd0)));
    c.set(0, std::uint32_t(c.i16(actor.address + 0x1a0)) + c.r(4));
    c.put16(actor.address + 0x1a0, c.r(0));
    clamp_play_bias(actor.address);

    const auto column = std::int32_t(c.i8(actor.address + 0x105));
    const auto row = std::int32_t(c.i8(actor.address + 0x106));
    c.set(5, actor.match());
    c.set(4, c.r(5) + column * 0x450);
    c.set(3, c.r(4) + row * 0x5c + (signed_side() == 0));
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0xa60)));
    if (compare(c, std::int32_t(c.r(0))) != 0) return;
    c.set(3, std::uint32_t(c.i16(actor.address + 0x1a0)));
    c.set(0, c.r(3) + 50);
    c.put16(actor.address + 0x1a0, c.r(0));
    clamp_play_bias(actor.address);
}

void PlayerMovement::record_play_totals(unsigned amount) {
    for (const auto field : {0x3108u, 0x310cu}) {
        c.set(4, actor.match());
        c.set(3, c.u32(c.r(4) + field));
        c.set(0, c.r(3) + amount);
        c.put32(c.r(4) + field, c.r(0));
    }
}

void PlayerMovement::credit_player_contact(unsigned condition) {
    c.set(3, c.u8(actor.address + 0x11));
    c.set(0, c.r(3) + 1);
    c.put8(actor.address + 0x11, c.r(0));
    c.set(3, std::uint32_t(c.i16(actor.address + 0x36)));
    c.set(0, c.r(3) + condition);
    c.put16(actor.address + 0x36, c.r(0));
}

void PlayerMovement::react_to_play_success() {
    c.set(0, std::uint32_t(c.i8(actor.address + 0x17)));
    c.set(3, c.r(0) * 20);
    c.set(21, c.r(3) + 20);
    c.s.wr<1>(reg::xer_ca, c.r(3) > UINT32_MAX - 20);
    const auto bound = std::int32_t(c.r(21));
    if (compare(c, bound) == 0) c.set(13, 0);
    else {
        refill_carrier_pool(0x102320f8);
        // The signed rating byte times twenty plus twenty cannot be wide.
        c.set(3, 65535);
        compare(c, bound, 65535);
        c.set(3, std::uint32_t(-65535));
        compare(c, bound, -65535);
        narrow_possession_draw(bound, 13);
    }
    if (compare(c, std::int32_t(c.r(13))) != 0) return;
    refill_carrier_pool(0x1023225c);
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(27)));
    c.set(5, c.u16(c.r(3)) + c.r(0));
    c.set(3, actor.address);
    c.set(0, shift_signed(high_product(c.r(5), 0x66666667), 1));
    c.set(0, c.r(5) - toward_zero(c.r(0)) * 5);
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    c.call(0x10288f30, 0x102322a0);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) == 0) return;
    c.set({{3, actor.address}, {4, 0}});
    c.call(0x1022ba10, 0x102322b4);
}

void PlayerMovement::credit_carrier_play() {
    if (compare_logical(c, c.r(22)) == 0) {
        c.set(3, std::uint32_t(c.i16(actor.address + 0x36)));
        c.set(0, c.r(3) + 9);
        c.put16(actor.address + 0x36, c.r(0));
        record_play_totals(36);
        return;
    }

    c.set(0, std::uint32_t(std::int16_t(c.r(18))));
    const bool kind15 = compare(c, std::int32_t(c.r(0)), 15) == 0;
    c.set(3, c.u8(c.r(22) + 0x2c));
    const auto delay = c.u8(frame.at(0x44));
    unsigned contact_delay = delay;
    if (!kind15) {
        c.set(0, std::uint32_t(std::int8_t(delay)));
        contact_delay = compare(c, std::int32_t(c.r(0)), 2) > 0 ? delay : 2;
    }
    c.set(0, c.r(3) + contact_delay);
    c.put8(c.r(22) + 0x2c, c.r(0));
    c.set(0, std::uint32_t(std::int16_t(c.r(18))));
    compare(c, std::int32_t(c.r(0)), 15);
    if (kind15) {
        credit_player_contact(75);
        record_play_totals(300);
        return;
    }

    c.set(3, c.r(22));
    c.call(0x10289560, 0x10232048);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    const bool special = compare(c, std::int32_t(c.r(0)), 1) == 0;
    if (special) c.put8(actor.match() + 0x1bd, 1);
    record_play_totals(special ? 200 : 400);
    credit_player_contact(100);
    react_to_play_success();
}

bool PlayerMovement::play_rule(std::uint32_t target, std::uint32_t back) {
    c.set(3, actor.address);
    c.call(target, back);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

void PlayerMovement::narrate_opposed_play(std::uint32_t message, std::uint32_t back) {
    c.set({{3, c.u32(actor.match() + 0x79f4)}, {4, message}, {5, actor.match()}, {10, 0}});
    c.set({{6, c.u8(actor.address + 0x2f)}, {7, actor.side()},
           {8, c.u8(c.r(22) + 0x2f)}, {9, c.u8(c.r(22) + 0x27)}});
    c.call(0x10201180, back);
}

bool PlayerMovement::opposed_skill_is_distinct() {
    c.set_f(1, c.f32(actor.address + 0x74));
    c.set_f(0, c.f32(c.r(31) + 0x94));
    fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 4) return true;
    c.set_f(2, c.f32(actor.address + 0x74));
    c.set_f(1, c.f32(c.r(22) + 0x74));
    c.set_f(0, c.f32(c.r(31) + 0x5c));
    c.set_f(1, fp.fsubs(c.f(2), c.f(1)));
    fp.fcmp(0, c.f(1), c.f(0));
    return (c.cr(0) & 4) != 0;
}

void PlayerMovement::sample_opposed_skill(std::uint32_t delta) {
    c.set_f(0, c.f32(actor.address + 0x98));
    c.put64(frame.at(0xbc), fp.fctiwz(0, c.f(0)));
    c.set(14, c.u32(frame.at(0xc0)));
    const auto bound = std::int32_t(c.r(14));
    if (compare(c, bound) == 0) {
        c.set(13, 0);
        return;
    }
    refill_carrier_pool(0x10232480 + delta);
    c.set(3, 65535);
    bool wide = compare(c, bound, 65535) > 0;
    if (!wide) {
        c.set(3, std::uint32_t(-65535));
        wide = compare(c, bound, -65535) < 0;
    }
    if (wide) sample_wide_action(bound, 0x10232558 + delta, 13);
    else narrow_possession_draw(bound, 13);
}

void PlayerMovement::describe_carrier_opposition(std::uint32_t delta) {
    c.set(3, c.r(22));
    c.call(0x10289560, 0x1023240c + delta);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0)), 1) == 0) {
        const bool marked = marked_contact_goal_area(actor.address, 0x10232694 + delta);
        narrate_opposed_play(marked ? 0x1f55 : 0x2158, (marked ? 0x102326f0 : 0x102326c4) + delta);
        return;
    }
    if (opposed_skill_is_distinct()) {
        sample_opposed_skill(delta);
        refill_carrier_pool(0x102325dc + delta);
        c.set(4, c.u16(c.u32(c.r(30))) + c.u32(c.r(27)));
        const auto quotient = shift_signed(high_product(c.r(4), 0x88888889) + c.r(4), 4);
        c.set(3, quotient >> 31);
        c.set(0, c.r(4) - toward_zero(quotient) * 30);
        if (compare(c, std::int32_t(c.r(13)), std::int32_t(c.r(0))) < 0) {
            narrate_opposed_play(0x2173, 0x10232648 + delta);
            return;
        }
    }
    narrate_opposed_play(0x1f4a, 0x10232674 + delta);
}

bool PlayerMovement::opposed_space_is_open(std::uint32_t back) {
    c.set({{0, std::uint32_t(signed_side() == 0)}, {7, 1}, {3, actor.match()},
           {5, c.u8(actor.address + 0x105)}, {6, c.u8(actor.address + 0x106)}});
    c.set(4, c.r(0));
    c.call(0x101e3f10, back);
    c.set_f(0, c.f64(c.r(26) + 0x90));
    fp.fcmp(0, c.f(1), c.f(0));
    return (c.cr(0) & 4) != 0;
}

bool PlayerMovement::opponent_space_flag() {
    c.set(0, std::uint32_t(signed_side() == 0) * 0x18e0);
    c.set(3, actor.match() + 0x10000 + c.r(0));
    c.set(0, c.u32(c.r(3) - 0x7e0c) & 0x400);
    return compare(c, std::int32_t(c.r(0))) != 0;
}

bool PlayerMovement::crosses_zone_boundary(bool target, std::uint32_t back) {
    c.set({{0, std::uint32_t(signed_side() == 0)}, {3, actor.address},
           {5, target ? c.r(16) : c.u8(actor.address + 0x105)},
           {6, target ? c.r(17) : c.u8(actor.address + 0x106)}});
    c.set(4, c.r(0));
    c.call(0x10239dc0, back);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

bool PlayerMovement::play_column_allowed(std::uint32_t back) {
    c.set({{3, actor.match()}, {4, c.r(16)}});
    c.call(0x101ee9c0, back);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

bool PlayerMovement::crosses_play_row(bool goal) {
    c.set(0, signed_side());
    if (compare(c, std::int32_t(c.r(0))) == 0) {
        c.set(0, std::uint32_t(std::int8_t(c.r(17))));
        const int row = goal ? 11 : 6;
        if (compare(c, std::int32_t(c.r(0)), row) == 0) {
            c.set(0, std::uint32_t(actor.row()));
            if (compare(c, std::int32_t(c.r(0)), row) < 0) return true;
        }
    }
    c.set(0, actor.side());
    if (compare(c, std::int32_t(c.r(0)), 1) != 0) return false;
    c.set(0, std::uint32_t(std::int8_t(c.r(17))));
    const int row = goal ? 0 : 5;
    if (compare(c, std::int32_t(c.r(0)), row) != 0) return false;
    c.set(0, std::uint32_t(actor.row()));
    return compare(c, std::int32_t(c.r(0)), row) > 0;
}

bool PlayerMovement::moved_play_axis(bool column, bool left) {
    const auto requested = std::int8_t(c.r(column ? 16 : 17));
    const auto compare_position = [&] {
        c.set({{3, std::uint32_t(requested)}, {0, std::uint32_t(column ? actor.column() : actor.row())}});
        return compare(c, requested, std::int32_t(c.r(0)));
    };
    c.set(0, signed_side());
    if (compare(c, std::int32_t(c.r(0))) == 0) {
        const auto order = compare_position();
        if (left ? order < 0 : order > 0) {
            if (!column) return true;
            c.set(0, std::uint32_t(requested));
            const auto flank = compare(c, requested, left ? 2 : 6);
            if (left ? flank < 0 : flank > 0) return true;
        }
    }
    c.set(0, actor.side());
    if (compare(c, std::int32_t(c.r(0)), 1) != 0) return false;
    const auto order = compare_position();
    if (left ? order <= 0 : order >= 0) return false;
    if (!column) return true;
    c.set(0, std::uint32_t(requested));
    const auto flank = compare(c, requested, left ? 6 : 2);
    return left ? flank > 0 : flank < 0;
}

bool PlayerMovement::attached_player_is_stationary() {
    c.set(0, c.u32(actor.address + 0x1b4));
    if (compare_logical(c, c.r(0)) == 0) return false;
    for (const auto field : {0x105u, 0x106u}) {
        const auto attached = c.u32(actor.address + 0x1b4);
        c.set({{4, c.u8(attached + field)}, {3, std::uint32_t(c.i8(actor.address + field))}});
        c.set(0, std::uint32_t(std::int8_t(c.r(4))));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) != 0) return false;
    }
    return true;
}

bool PlayerMovement::narrate_stationary_attachment(std::uint32_t delta) {
    if (!attached_player_is_stationary()) return false;
    c.set(4, c.u32(actor.address + 0x1b4));
    c.set(13, c.u8(c.r(4) + 0x10d));
    refill_carrier_pool(0x10232c78 + delta);
    c.set(4, c.u32(c.r(27)));
    c.set(5, c.u16(c.u32(c.r(30))) + c.r(4));
    c.set(0, std::uint32_t(std::int8_t(c.r(13))));
    c.set(4, high_product(c.r(5), 0x55555556));
    c.set(3, c.r(5) - toward_zero(c.r(4)) * 3);
    if (compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(3))) <= 0) return false;
    c.set(3, c.u32(actor.address + 0x1b4));
    const bool attached = compare_logical(c, c.r(3)) != 0;
    c.set(0, attached ? std::uint32_t(c.i8(c.r(3) + 0x2c)) : 0);
    if (compare(c, std::int32_t(c.r(0)), 2) >= 0) return false;
    c.set({{0, std::uint32_t(signed_side() == 0)}, {4, 0x2172}, {3, c.u32(actor.match() + 0x79f4)},
           {10, 0}, {8, c.u32(actor.address + 0x1b4)}, {5, actor.match()},
           {6, c.u8(actor.address + 0x2f)}, {7, actor.side()}});
    c.set({{9, c.r(0)}, {8, c.u8(c.r(8) + 0x2f)}});
    c.call(0x10201180, 0x10232d18 + delta);
    return true;
}

void PlayerMovement::narrate_clear_fallback(std::uint32_t delta) {
    c.set(0, std::uint32_t(c.i16(actor.address + 0x19c)));
    const auto previous = std::int32_t(c.r(0));
    if (compare(c, previous, 18) == 0 || compare(c, previous, 17) == 0) {
        narrate_followup_contact(0x1f4c, 0x10232d5c + delta, true, true);
        return;
    }
    if (marked_contact_goal_area(actor.address, 0x10232d7c + delta)) {
        narrate_followup_contact(0x20d1, 0x10232dac + delta, true, true);
        return;
    }
    c.set({{0, std::uint32_t(signed_side() == 0)}, {3, actor.match()}, {5, c.u8(actor.address + 0x106)}});
    c.set(4, c.r(0));
    c.call(0x101ee9f0, 0x10232dd0 + delta);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    const bool special = compare(c, std::int32_t(c.r(0))) != 0;
    narrate_followup_contact(special ? 0x20d3 : 0x20d2, (special ? 0x10232e00 : 0x10232e2c) + delta, true, true);
}

void PlayerMovement::complete_clear_play(std::uint32_t delta) {
    const auto narrate = [&](std::uint32_t message, std::uint32_t back) {
        narrate_followup_contact(message, back + delta, true, true);
    };
    if (crosses_zone_boundary(true, 0x1023289c + delta) && !crosses_zone_boundary(false, 0x102328c8 + delta))
        narrate(0x1f50, 0x102328f8);
    else if (play_column_allowed(0x10232908 + delta) && crosses_play_row(true))
        narrate(0x2156, 0x10232980);
    else if (crosses_play_row(false))
        narrate(0x1f51, 0x102329fc);
    else if (play_rule(0x102721a0, 0x10232a08 + delta) && moved_play_axis(false, true))
        narrate(0x1f54, 0x10232a78);
    else if (play_rule(0x10272100, 0x10232a84 + delta) &&
             play_rule(0x102720d0, 0x10232a94 + delta) && !play_column_allowed(0x10232aa8 + delta))
        narrate(0x1f4e, 0x10232ad8);
    else if (play_column_allowed(0x10232ae8 + delta) && moved_play_axis(true, true))
        narrate(0x1f52, 0x10232b70);
    else if (play_column_allowed(0x10232b80 + delta) && moved_play_axis(true, false))
        narrate(0x1f53, 0x10232c08);
    else if (!narrate_stationary_attachment(delta))
        narrate_clear_fallback(delta);
}

bool PlayerMovement::repeated_contact_warns() {
    c.set({{3, actor.match()}, {4, c.u8(frame.at(0x46))}});
    c.call(0x101ee530, 0x1023390c);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) == 0 && !contact_frame_byte(0x40)) {
        const auto condition = c.i16(c.r(22) + 0x32);
        c.set(16, std::uint32_t(condition));
        if (compare(c, condition) == 0) c.set(14, 0);
        else {
            refill_carrier_pool(0x10233954);
            // The lha condition is bounded by the signed halfword range.
            c.set(3, 65535);
            compare(c, condition, 65535);
            c.set(3, std::uint32_t(-65535));
            compare(c, condition, -65535);
            narrow_possession_draw(condition, 14);
        }
        if (compare(c, std::int32_t(c.r(14)), -10) >= 0) return false;
    }
    c.set({{4, c.u8(c.r(22) + 0x27)}, {3, c.r(22)}});
    c.call(0x10239d10, 0x10233aa4);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0))) != 0 && even_restart_sample(0x10233acc);
}

void PlayerMovement::narrate_repeated_pair(std::uint32_t message, std::uint32_t back) {
    c.set({{3, c.u32(actor.match() + 0x79f4)}, {4, message}, {5, actor.match()}, {10, 0}});
    c.set({{6, c.u8(c.r(22) + 0x2f)}, {7, c.u8(c.r(22) + 0x27)},
           {8, c.u8(actor.address + 0x2f)}, {9, actor.side()}});
    c.call(0x10201180, back);
}

std::uint32_t PlayerMovement::narrate_repeat_warning(std::uint32_t target) {
    if (target < 0x10233d28) {
        if (!narrate_contact_direction(target, 0x10233b1c)) return target;
        refill_carrier_pool(0x10233d48);
        c.set(4, c.u16(c.u32(c.r(30))) + c.u32(c.r(27)));
        const auto quotient = shift_signed(high_product(c.r(4), 0x51eb851f), 4);
        c.set(3, quotient >> 31);
        c.set(0, c.r(4) - toward_zero(quotient) * 50);
        target = 0x10233e14;
        if (compare_logical(c, c.r(0), 10) <= 0) {
            c.set(3, c.toc(-0x51fc));
            c.set(0, c.r(0) * 4);
            c.set(3, c.u32(c.r(3) + c.r(0)));
            c.s.wr<4>(reg::ctr, c.r(3));
            target = c.r(3);
        }
    }
    std::uint32_t message;
    switch (target) {
    case 0x10233d9c: message = 0x20eb; break;
    case 0x10233dd8: message = 0x20ea; break;
    case 0x10233e14: message = 0x20e8; break;
    default: return target;
    }
    c.set(0, std::uint32_t(c.i8(c.r(22) + 0x27) == 0));
    c.set({{3, c.u32(actor.match() + 0x79f4)}, {4, message}, {5, actor.match()},
           {6, UINT32_MAX}, {7, c.r(0)}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, target + 0x38);
    return 0x10233f7c;
}

std::uint32_t PlayerMovement::repeated_contact_fallback() {
    c.set(0, std::uint32_t(c.i8(frame.at(0x46))));
    const auto direction = std::int32_t(c.r(0));
    const auto against34 = compare(c, direction, 34);
    if (against34 == 0) narrate_repeated_pair(0x2150, 0x10233f50);
    else if (against34 < 0) {
        const auto against19 = compare(c, direction, 19);
        if (against19 == 0 || (against19 > 0 && compare(c, direction, 33) >= 0))
            narrate_repeated_pair(0x1f62, 0x10233f24);
        else if (against19 < 0 && compare(c, direction, 1) == 0) {
            const auto gap = std::uint32_t(c.i16(frame.at(0x48))) - c.r(21);
            c.set({{0, gap}, {3, c.r(22)}, {4, std::uint32_t(std::int16_t(gap))}});
            c.call(0x10270320, 0x10233ec4);
            c.set(0, std::uint32_t(std::int8_t(c.r(3))));
            if (compare(c, std::int32_t(c.r(0))) == 0) return 0x10235d88;
        }
    } else {
        const auto against44 = compare(c, direction, 44);
        if (against44 == 0) narrate_repeated_pair(0x2150, 0x10233f50);
        else if (against44 < 0) {
            const auto against41 = compare(c, direction, 41);
            if (against41 == 0) narrate_repeated_pair(0x1f61, 0x10233ef8);
            else if (against41 > 0) narrate_repeated_pair(0x1f64, 0x10233f7c);
        } else if (compare(c, direction, 46) < 0) narrate_repeated_pair(0x1f62, 0x10233f24);
    }
    return 0x10233f7c;
}

std::uint32_t PlayerMovement::choose_carrier_play() {
    apply_play_bias();
    credit_carrier_play();
    const auto kind = std::int16_t(c.r(18));
    c.set({{0, std::uint32_t(kind)}, {4, actor.match()}, {3, 0}});
    const auto versus17 = compare(c, kind, 17);
    c.put8(c.r(4) + 0x791f, 0);
    if (versus17 == 0) return 0x10232354;
    if (versus17 > 0) return compare(c, kind, 19) < 0 ? 0x10232e30 : 0x10233890;
    return compare(c, kind, 15) == 0 ? 0x10233830 : 0x10233890;
}

void PlayerMovement::continue_simple_play(std::uint32_t phase) {
    if (phase == 0x10233830) {
        if (compare_logical(c, c.r(22)) == 0) {
            narrate_followup_contact(0x1f48, 0x1023388c, true, true);
            return;
        }
        c.set({{3, c.u32(actor.match() + 0x79f4)}, {4, 0x1f49}, {5, actor.match()}, {10, 0}});
        c.set({{6, c.u8(actor.address + 0x2f)}, {7, actor.side()},
               {8, c.u8(c.r(22) + 0x2f)}, {9, c.u8(c.r(22) + 0x27)}});
        c.call(0x10201180, 0x10233860);
        return;
    }
    c.set({{3, c.toc(-0x7bb8)}, {4, c.r(14) + 0x63}, {5, std::uint32_t(std::int16_t(c.r(18)))}});
    c.call(0x1077b720, 0x102338a0);
    report_movement_diagnostic(0x80, 0x58, 0xfce, 0x102338b8);
}

bool PlayerMovement::continue_kind17_contact() {
    if (marked_contact_goal_area(actor.address, 0x10232370) || play_rule(0x102721a0, 0x10232380))
        c.set(19, 0);
    c.set(0, std::uint32_t(std::int8_t(c.r(20))));
    if (compare(c, std::int32_t(c.r(0)), 4) == 0) {
        c.set(0, std::uint32_t(c.i16(actor.address + 0x19c)));
        const bool kind15 = compare(c, std::int32_t(c.r(0)), 15) == 0;
        narrate_followup_contact(kind15 ? 0x20d2 : 0x2009, kind15 ? 0x102323cc : 0x102323f8, true, true);
        return false;
    }
    if (compare_logical(c, c.r(22)) == 0) return true;
    describe_carrier_opposition(0);
    c.set(0, std::uint32_t(std::int8_t(c.r(19))));
    if (compare(c, std::int32_t(c.r(0))) != 0 && opposed_space_is_open(0x10232720)) {
        const bool flagged = opponent_space_flag();
        narrate_followup_contact(flagged ? 0x20de : 0x1f45, flagged ? 0x10232784 : 0x102327b0, true, true);
    }
    return false;
}

void PlayerMovement::continue_kind17_clear() {
    c.set(0, std::uint32_t(std::int8_t(c.r(19))));
    if (compare(c, std::int32_t(c.r(0))) != 0 && opposed_space_is_open(0x102327e4)) {
        const bool flagged = opponent_space_flag();
        narrate_followup_contact(flagged ? 0x20de : 0x1f45, flagged ? 0x10232848 : 0x10232874, true, true);
    } else complete_clear_play(0);
}

void PlayerMovement::continue_kind18() {
    bool restricted = marked_contact_goal_area(actor.address, 0x10232e4c);
    if (!restricted) {
        c.set(0, std::uint32_t(std::int8_t(c.r(20))));
        restricted = compare(c, std::int32_t(c.r(0)), 4) == 0;
    }
    if (restricted) c.set(19, 0);
    if (compare_logical(c, c.r(22)) == 0) {
        complete_clear_play(0xa00);
        return;
    }
    describe_carrier_opposition(0xa68);
    c.set(0, std::uint32_t(std::int8_t(c.r(19))));
    if (compare(c, std::int32_t(c.r(0))) == 0 || !play_rule(0x10272100, 0x10233168) ||
        !opposed_space_is_open(0x10233198)) return;
    const bool flagged = opponent_space_flag() && even_restart_sample(0x102331f4);
    narrate_followup_contact(flagged ? 0x20de : 0x1f45, flagged ? 0x10233248 : 0x10233274, true, true);
}

std::uint32_t PlayerMovement::continue_repeated_contact(std::uint32_t phase) {
    if (phase != 0x102338f8) return narrate_repeat_warning(phase);
    if (compare_logical(c, c.r(22)) == 0) return 0x10234dec;
    if (!repeated_contact_warns()) return repeated_contact_fallback();
    const auto target = contact_direction_target(-0x51f8, 0x10233d00);
    return narrate_repeat_warning(target);
}
}
