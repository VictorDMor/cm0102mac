#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool followthrough_equals(Cpu& c, std::int32_t value, std::int32_t against = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, against);
    return (c.cr(0) & 2) != 0;
}

static std::uint32_t match_byte(Cpu& c, std::uint32_t actor, unsigned offset) {
    c.set(3, c.u32(actor + 0x1a4));
    return c.u8(c.r(3) + offset);
}

static bool flight_leaves_pitch(Cpu& c, std::uint32_t actor) {
    followthrough_equals(c, std::int8_t(match_byte(c, actor, 0x791d)));
    if (c.cr(0) & 8) return true;
    followthrough_equals(c, std::int8_t(match_byte(c, actor, 0x791d)), 9);
    if (!(c.cr(0) & 8)) {
        followthrough_equals(c, std::int8_t(match_byte(c, actor, 0x791e)));
        if (c.cr(0) & 8) return true;
    }
    followthrough_equals(c, std::int8_t(match_byte(c, actor, 0x791e)), 12);
    return (c.cr(0) & 8) == 0;
}

static void end_released_shot(Cpu& c, std::uint32_t actor) {
    if (flight_leaves_pitch(c, actor)) {
        c.set(0, c.u8(actor + 0x27));
        c.set(3, c.u32(actor + 0x1a4));
        c.set(0, std::int8_t(c.r(0)) == 0);
        c.set(4, c.r(0));
        c.call(0x101f0c00, 0x1025b914);
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, 1);
    c.put8(c.r(3) + 0x1e6, 1);
    c.s.pc = 0x1026a190;
}

static void place_shot_on_goal_line(Cpu& c, std::uint32_t actor) {
    unsigned row = 0;
    if (!followthrough_equals(c, c.u8(actor + 0x27), 1)) {
        if (!followthrough_equals(c, c.i8(actor + 0x27))) return;
        row = 11;
    }
    c.set(4, c.u32(actor + 0x1a4));
    c.set({{3, 4}, {0, row}});
    c.put8(c.r(4) + 0x791d, c.r(3));
    c.set(3, c.u32(actor + 0x1a4));
    c.put8(c.r(3) + 0x791e, c.r(0));
}

static std::uint32_t arithmetic_sample_shift(Cpu& c, std::int32_t high, unsigned bits) {
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
    return std::uint32_t(high >> bits);
}

static std::uint32_t high_sample_product(std::uint32_t sample, std::int32_t multiplier) {
    return std::uint32_t((std::int64_t(std::int32_t(sample)) * multiplier) >> 32);
}

static bool should_reset_pitch_players(Cpu& c, std::uint32_t stack) {
    advance_shot_stream(c, 0x1025b990);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x88890000);
    c.set(5, c.u32(c.r(28)));
    c.set(4, 0x88888889);
    c.set(6, c.u16(c.r(3)));
    c.set(0, c.u8(stack + 0x127));
    const auto sample = c.r(6) + c.r(5);
    c.set(5, sample);
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    const auto high = high_sample_product(sample, std::int32_t(0x88888889)) + sample;
    const auto shifted = arithmetic_sample_shift(c, std::int32_t(high), 3);
    c.set(4, sample - (shifted + (shifted >> 31)) * 15);
    c.set(0, c.r(4) + 25);
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    return (c.cr(0) & 4) != 0;
}

static bool should_reset_player(Cpu& c, std::uint32_t player, std::uint32_t actor) {
    followthrough_equals(c, c.i8(player + 0x19));
    if (c.cr(0) & 8) return false;
    followthrough_equals(c, c.i8(player + 0x19), 10);
    if (c.cr(0) & 4) return false;
    if (!followthrough_equals(c, c.i8(player + 0x2c))) return false;
    c.compare_logical(0, player, actor);
    return (c.cr(0) & 2) == 0;
}

static void reset_pitch_players(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, 5);
    c.set(4, c.u32(actor + 0x1a4));
    c.set(21, 0);
    c.set(25, c.u8(c.r(3) + 0x791f));
    c.put8(c.r(4) + 0x791f, c.r(0));
    for (;;) {
        followthrough_equals(c, std::int8_t(c.r(21)), 2);
        if (!(c.cr(0) & 8)) break;
        c.set(0, std::uint32_t(std::int8_t(c.r(21))));
        c.set(3, c.u32(actor + 0x1a4));
        c.set(0, c.r(0) * 0x2350);
        c.set(23, 0);
        c.set(3, c.r(3) + 0x311c);
        c.set(14, c.r(3) + c.r(0));
        for (;;) {
            followthrough_equals(c, std::int8_t(c.r(23)), 20);
            if (!(c.cr(0) & 8)) break;
            if (should_reset_player(c, c.r(14), actor)) {
                c.set(3, c.r(14));
                call_shot_owner(c, 0x102775e0, 0x1025ba50);
            }
            c.set(23, c.r(23) + 1);
            c.set(14, c.r(14) + 0x1c4);
        }
        c.set(21, c.r(21) + 1);
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.put8(c.r(3) + 0x791f, c.r(25));
}

static void react_as_beaten_keeper(Cpu& c, std::uint32_t keeper) {
    c.set(3, std::uint32_t(c.i16(keeper + 0x36)));
    c.set(0, c.r(3) - 800);
    c.put16(keeper + 0x36, c.r(0));
    c.set(3, c.u32(keeper + 0x6c));
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x66)));
    c.set(3, c.r(0) * 20);
    const auto bound = c.r(3) + 10;
    c.s.wr<1>(reg::xer_ca, bound < c.r(3));
    c.set(21, bound);
    // The signed tendency bounds this sample to [-2550, 2550].
    draw_narrow_shot_sample(c, std::int32_t(bound), 14, 0x1025bafc);
    c.compare(0, std::int32_t(c.r(14)), 0);
    if (c.cr(0) & 2) {
        advance_shot_stream(c, 0x1025bc60);
        c.set(3, c.u32(c.r(31)));
        c.set(4, 0x55550000);
        c.set(0, c.u32(c.r(28)));
        c.set(4, 0x55555556);
        c.set(5, c.u16(c.r(3)));
        c.set(3, keeper);
        const auto sample = c.r(5) + c.r(0);
        c.set(5, sample);
        const auto high = high_sample_product(sample, 0x55555556);
        c.set(0, (high + (high >> 31)) * 3 - sample);
        c.set(4, std::uint32_t(std::int8_t(c.r(0))));
        call_shot_owner(c, 0x10288f30, 0x1025bca4);
    }
    c.set({{3, keeper}, {4, 0}});
    call_shot_owner(c, 0x1022ba10, 0x1025bcb0);
}

static bool in_front_of_keeper(Cpu& c, std::uint32_t actor, std::uint32_t keeper, bool home) {
    c.set(3, c.u8(actor + 0x105));
    c.set(0, c.u8(keeper + 0x105));
    c.set(3, std::uint32_t(std::int8_t(c.r(3))));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    return (c.cr(0) & (home ? 4 : 8)) != 0;
}

static void adjust_shot_power_for_keeper(Cpu& c, std::uint32_t actor, std::uint32_t keeper) {
    const bool home = followthrough_equals(c, c.i8(actor + 0x27));
    const bool draw = home && in_front_of_keeper(c, actor, keeper, true);
    if (!draw && (!followthrough_equals(c, c.u8(actor + 0x27), 1) ||
                  !in_front_of_keeper(c, actor, keeper, false))) {
        c.set(16, c.r(16) + 190);
        return;
    }
    advance_shot_stream(c, draw ? 0x1025bcf4 : 0x1025bd80);
    c.set(4, c.u32(c.r(31)));
    c.set(3, 0x51ec0000);
    c.set(0, c.u32(c.r(28)));
    c.set(3, 0x51eb851f);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto shifted = arithmetic_sample_shift(c, std::int32_t(high_sample_product(sample, 0x51eb851f)), 7);
    c.set(3, sample - (shifted + (shifted >> 31)) * 400);
    c.set(0, std::uint32_t(std::int16_t(c.r(3) + 190)));
    c.set(16, c.r(16) + c.r(0));
}

void prepare_goalkeeper_followthrough(Cpu& c) {
    const auto actor = c.r(15), keeper = c.r(24), stack = c.sp();
    if (followthrough_equals(c, c.u8(stack + 0x128), 1)) {
        end_released_shot(c, actor);
        return;
    }
    place_shot_on_goal_line(c, actor);
    if (should_reset_pitch_players(c, stack)) reset_pitch_players(c, actor);
    if (!followthrough_equals(c, c.i8(stack + 0x46))) {
        if (followthrough_equals(c, std::int8_t(c.r(17)), 1)) {
            c.set(3, keeper);
            call_shot_owner(c, 0x10289560, 0x1025baa4);
            if (followthrough_equals(c, std::int16_t(c.r(3)), 1)) react_as_beaten_keeper(c, keeper);
        }
        adjust_shot_power_for_keeper(c, actor, keeper);
    }
    c.compare_logical(0, keeper, 0);
    if (!(c.cr(0) & 2)) {
        c.set(3, c.u32(actor + 0x1a4) + 0x10000);
        c.set(0, c.u32(c.r(3) - 0x1fd4));
        c.compare_logical(0, c.r(0), keeper);
        if (c.cr(0) & 2) {
            c.set(0, 0);
            c.put16(stack + 0x132, 0);
        }
    }
    c.s.pc = 0x1025bdf0;
}
}
