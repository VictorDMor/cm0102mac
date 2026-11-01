#include "european_cup_detail.hpp"
#include <array>

namespace cm::aot::european_cup {
using namespace detail;
namespace {
// One leg of a group round: day, month, whether in the next year, weekday rule.
struct Leg {
    std::uint32_t day, month, next_year, rule;
};
// Group rounds, as the original lists them: groups 0-3 and 4-7 of the first
// phase, and groups 8-9 and 10-11 of the second, play on different dates.
constexpr std::array<Leg, 6> first_phase_early{{{0xc, 8, 0, 1}, {0x14, 8, 0, 2}, {0x1b, 8, 0, 2}, {0x11, 9, 0, 1}, {0x19, 9, 0, 2}, {7, 0xa, 0, 1}}};
constexpr std::array<Leg, 6> first_phase_late{{{0xd, 8, 0, 2}, {0x13, 8, 0, 1}, {0x1a, 8, 0, 1}, {0x12, 9, 0, 2}, {0x18, 9, 0, 1}, {8, 0xa, 0, 2}}};
constexpr std::array<Leg, 6> second_phase_early{{{0x15, 0xa, 0, 1}, {6, 0xb, 0, 2}, {0xe, 1, 1, 2}, {0x14, 1, 1, 1}, {7, 2, 1, 2}, {0xd, 2, 1, 1}}};
constexpr std::array<Leg, 6> second_phase_late{{{0x16, 0xa, 0, 2}, {5, 0xb, 0, 1}, {0xd, 1, 1, 1}, {0x15, 1, 1, 2}, {6, 2, 1, 1}, {0xe, 2, 1, 2}}};

// A knockout round's fixed format values in the 0x78-byte round record. No
// reader was traced for all of them, so some keep their offsets as names.
struct RoundFormat {
    unsigned label, index, field_c, flags, field_10, entrants, ties, field_1e, field_20;
    unsigned leg_rule, field_24;
    std::uint32_t prize_6c, prize_70, prize_74;
};
void format_round(Cpu& c, std::uint32_t round, const RoundFormat& f) {
    c.put16(round + 8, f.label); c.put16(round + 0xa, f.index); c.put16(round + 0xc, f.field_c);
    c.put16(round + 0xe, f.flags); c.put16(round + 0x10, f.field_10);
    c.put8(round + 0x18, 8); c.put16(round + 0x1a, f.entrants); c.put16(round + 0x1c, f.ties);
    c.put16(round + 0x1e, f.field_1e); c.put16(round + 0x20, f.field_20); c.put8(round + 0x22, 0);
    c.put8(round + 0x23, f.leg_rule); c.put8(round + 0x24, f.field_24);
    c.put32(round + 0x6c, f.prize_6c); c.put32(round + 0x70, f.prize_70); c.put32(round + 0x74, f.prize_74);
}
void group_legs(Cpu& c, const Frame& f, std::uint32_t schedule, const Competition& cup, const std::array<Leg, 6>& legs, std::uint32_t first_back) {
    std::uint32_t back = first_back;
    for (std::uint32_t i = 0; i < legs.size(); ++i, back += 0x2c) {
        const auto& leg = legs[i];
        c.put32(f.at(0x38), 0);
        c.set({{3, schedule}, {4, i}, {5, leg.day}, {6, leg.month}, {7, leg.next_year}, {8, leg.rule}, {9, 2}, {10, extended(cup.year())}});
        c.call(0x104a9810, back);
    }
}
void clear_times(Cpu& c, std::uint32_t schedule, std::uint32_t back) {
    for (c.set(26, 0); compare(c, std::int16_t(c.r(26)), 6) < 0; c.set(26, c.r(26) + 1)) {
        c.set({{3, schedule}, {4, c.r(26)}, {5, 0}, {6, 0xffffffff}, {7, 0xffffffff}, {8, 0xffffffff}, {9, 0}});
        c.call(0x104a9890, back);
    }
}
// `lha` of the year into r9, for 0x104854f0's dated legs.
void add_round(Cpu& c, std::uint32_t schedule, const Competition& cup, std::uint32_t index, std::uint32_t day, std::uint32_t month,
               std::uint32_t next_year, std::uint32_t rule, std::uint32_t back) {
    c.set({{3, schedule}, {4, index}, {5, day}, {6, month}, {7, next_year}, {8, rule}, {9, extended(cup.year())}});
    c.call(0x104854f0, back);
}
void add_leg(Cpu& c, const Frame& f, std::uint32_t schedule, const Competition& cup, std::uint32_t index, std::uint32_t day, std::uint32_t month,
             std::uint32_t next_year, std::uint32_t rule, std::uint32_t option, std::uint32_t back) {
    c.put32(f.at(0x38), option);
    c.set({{3, schedule}, {4, index}, {5, day}, {6, month}, {7, next_year}, {8, rule}, {9, 2}, {10, extended(cup.year())}});
    c.call(0x10485570, back);
}
void add_times(Cpu& c, std::uint32_t schedule, std::uint32_t index, std::uint32_t back) {
    c.set({{3, schedule}, {4, index}, {5, 0}, {6, 0xffffffff}, {7, 0xffffffff}, {8, 0xffffffff}, {9, 0}});
    c.call(0x10485600, back);
}
}

std::uint32_t calendar(Cpu& c, std::uint32_t address, std::uint32_t stage, std::uint32_t count_out, std::uint32_t extra_out, std::uint32_t enabled_out) {
    Frame f(c, 0x104ea620, 0x110, {.first_gpr = 26});
    keep(c, {{27, stage}, {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {26, address}, {31, c.toc(slot::pool)}});
    const Competition cup{c, address};
    const auto id = std::int32_t(std::int8_t(stage));
    const auto outputs = [&](std::uint32_t enabled, std::uint32_t count, std::uint32_t extra) {
        if (compare_logical(c, enabled_out) != 0) c.put8(enabled_out, enabled);
        c.put16(count_out, count); c.put16(extra_out, extra);
    };
    // Each allocation failure reports and returns null.
    const auto allocate = [&](std::uint32_t size, std::uint32_t back, std::uint32_t at, unsigned name, unsigned extension, unsigned line) {
        c.set(3, size);
        c.call(0x10775130, back);
        keep(c, {{28, c.r(3)}});
        if (compare(c, std::int32_t(c.r(3))) != 0) return true;
        assertion(c, f, at, name, extension, line);
        return false;
    };
    if (compare(c, id, -1) == 0) {
        outputs(0, 3, 0);
        if (!allocate(0x168, 0x104ea678, 0x104ea694, 0xd0, 0x68, 0x12c)) return 0;
        const auto schedule = c.r(28);
        add_round(c, schedule, cup, 0, 0x1c, 5, 0, 2, 0x104ea6f4);
        add_leg(c, f, schedule, cup, 0, 0xc, 6, 0, 2, 0, 0x104ea720);
        keep(c, {{27, 0}});
        format_round(c, schedule, {0xfa, 0, 4, 0, 3, 0x12, 9, 0x12, 0, 2, 7, 0, 0, 0});
        c.set(10, 3);
        add_round(c, schedule, cup, 1, 0x1c, 5, 0, 2, 0x104ea7a4);
        add_leg(c, f, schedule, cup, 1, 0x1a, 6, 0, 2, 0, 0x104ea7d0);
        keep(c, {{29, 0xe}, {27, 0x13}});
        format_round(c, schedule + round_size, {0x104, 1, 4, 0x400, 3, 0x1c, 0xe, 0x13, 0x12, 2, 7, 0, 0, 0});
        c.set(10, 0);
        add_round(c, schedule, cup, 2, 0x15, 6, 0, 4, 0x104ea864);
        add_leg(c, f, schedule, cup, 2, 9, 7, 0, 2, 0, 0x104ea890);
        format_round(c, schedule + 2 * round_size, {0x10e, 2, 4, 0x400, 3, 0x20, 0x10, 0x12, 0x25, 2, 0xe, 0, 0, 0});
        c.set({{3, schedule}, {4, 0}, {5, 0x25}, {6, 3}, {7, 0x12}, {8, 0x10}, {9, 0x20}, {10, 8}});
        return schedule;
    }
    if (compare(c, id, 8) < 0) {
        outputs(1, 6, std::uint32_t(id + 0x43d));
        if (!allocate(0x1c8, 0x104ea93c, 0x104ea958, 0xb0, 0x60, 0x17a)) return 0;
        const auto schedule = c.r(28);
        if (compare(c, id, 4) < 0) group_legs(c, f, schedule, cup, first_phase_early, 0x104ea9d0);
        else group_legs(c, f, schedule, cup, first_phase_late, 0x104eaad8);
        clear_times(c, schedule, 0x104eabdc);
        c.set(3, schedule);
        return schedule;
    }
    if (compare(c, id, 12) < 0) {
        outputs(1, 6, std::uint32_t(id + 0x43d));
        if (!allocate(0x1c8, 0x104eac28, 0x104eac44, 0x90, 0x58, 0x1aa)) return 0;
        const auto schedule = c.r(28);
        // The next year's date, normalized; the stack holds both forms.
        c.set({{3, f.at(0x50)}, {4, 1}, {5, 2}, {7, 2}, {6, (extended(cup.year()) + 1) & 0xffff}});
        c.call(0x107156f0, 0x104eaca8);
        c.set({{3, f.at(0x40)}, {4, f.at(0x50)}, {5, 1}});
        c.call(0x10717b20, 0x104eacb8);
        c.set({{3, c.u8(f.at(0x44))}, {4, c.u16(f.at(0x42))}});
        c.put16(f.at(0x52), c.r(4)); c.put16(f.at(0x50), c.u16(f.at(0x40))); c.put8(f.at(0x54), c.r(3));
        if (compare(c, id, 10) < 0) group_legs(c, f, schedule, cup, second_phase_early, 0x104ead08);
        else group_legs(c, f, schedule, cup, second_phase_late, 0x104eae14);
        clear_times(c, schedule, 0x104eaf18);
        c.set(3, schedule);
        return schedule;
    }
    if (id != 12) return 0;
    outputs(0, 3, 0);
    if (!allocate(0x168, 0x104eaf5c, 0x104eaf78, 0x70, 0x48, 0x1db)) return 0;
    const auto schedule = c.r(28);
    add_round(c, schedule, cup, 0, 0xf, 2, 1, 3, 0x104eafd8);
    add_leg(c, f, schedule, cup, 0, 4, 3, 1, 2, 0, 0x104eb004);
    add_times(c, schedule, 0, 0x104eb024);
    keep(c, {{29, 0}, {27, 8}});
    format_round(c, schedule, {0x78, 0, 0, 0x200, 3, 8, 4, 8, 0, 2, 0xe, 0x186a00, 0, 0});
    c.set(10, 2);
    add_round(c, schedule, cup, 1, 0x14, 3, 1, 4, 0x104eb0ac);
    add_leg(c, f, schedule, cup, 1, 2, 4, 1, 2, 0, 0x104eb0d8);
    add_times(c, schedule, 1, 0x104eb0f8);
    keep(c, {{27, 0}});
    format_round(c, schedule + round_size, {0x82, 1, 0, 0, 3, 4, 2, 0, 0, 2, 7, 0x1e8480, 0, 0});
    c.set(10, 3);
    add_round(c, schedule, cup, 2, 3, 4, 1, 3, 0x104eb180);
    const auto year = extended(cup.year());
    c.set(10, year);
    // The final's kick-off option differs for 2000.
    const bool millennium = compare(c, std::int32_t(year), 0x7d0) == 0;
    c.put32(f.at(0x38), millennium ? 0x80 : 4);
    c.set({{3, schedule}, {4, 2}, {5, 0x17}, {6, 4}, {7, 1}, {8, 2}, {9, 2}});
    c.call(0x10485570, millennium ? 0x104eb1b4 : 0x104eb1e0);
    add_times(c, schedule, 2, 0x104eb200);
    // The final's record: no label for the leg rule or the amount order.
    const auto last = schedule + 2 * round_size;
    c.put16(last + 8, 0x96); c.put16(last + 0xa, 2); c.put16(last + 0xc, 0); c.put16(last + 0xe, 0x83);
    c.put8(last + 0x18, 8); c.put16(last + 0x1a, 2); c.put16(last + 0x1c, 1); c.put16(last + 0x1e, 0);
    c.put16(last + 0x10, 0); c.put16(last + 0x20, 0); c.put8(last + 0x22, 0); c.put8(last + 0x23, 1); c.put8(last + 0x24, 0);
    c.put32(last + 0x6c, 0); c.put32(last + 0x70, 0x3d0900); c.put32(last + 0x74, 0x30d400);
    c.set({{3, schedule}, {4, 0x3d0900}, {5, 1}, {6, 8}, {7, 0x83}, {8, 0}, {9, 2}});
    return schedule;
}

void fixture_times(Cpu& c, std::uint32_t address, std::uint32_t stage, std::uint32_t weekday, std::uint32_t first_time, std::uint32_t second_time) {
    Frame f(c, 0x104eee20, 0x50, {.first_gpr = 28});
    keep(c, {{31, second_time}, {30, first_time}, {29, weekday}, {28, stage}});
    Program p{c};
    const auto fixture = p.call(0x10459b50, 0x104eee54, {c.u32(address + 8)});
    const auto id = std::int32_t(std::int8_t(stage));
    // The compiler's compare tree: cr0 ends as its last compare left it.
    const auto qualifying = [&](std::uint32_t value, std::uint32_t for_one) {
        const auto byte = std::int32_t(value & 0xff);
        if (compare(c, byte, 0x21) == 0) return 0x31u;
        if (byte > 0x21) return compare(c, byte, 0x2f) == 0 ? 0x3fu : value;
        if (compare(c, byte, 0x11) == 0) return 0x21u;
        if (byte > 0x11) return value;
        return compare(c, byte, 1) == 0 ? for_one : value;
    };
    const auto tiers = [&](std::uint32_t value, std::uint32_t low, std::uint32_t three, std::uint32_t four) {
        const auto byte = std::int32_t(value & 0xff);
        if (compare(c, byte, 3) == 0) return three;
        if (byte > 3) return compare(c, byte, 5) >= 0 ? value : four;
        return compare(c, byte, 1) >= 0 ? low : value;
    };
    if (compare(c, id, -1) == 0) {
        first_time = qualifying(first_time, 1);
        second_time = qualifying(second_time, 0x19);
        weekday = qualifying(weekday, 0x19);
    } else if (compare(c, id, 8) < 0) {
        first_time = tiers(first_time, 1, 0x11, 0x19);
        second_time = tiers(second_time, 0xd, 0x11, 0x19);
        weekday = tiers(weekday, 1, 0x11, 0x19);
    } else if (compare(c, id, 12) < 0) {
        first_time = tiers(first_time, 1, 9, 0xd);
        second_time = tiers(second_time, 7, 9, 0xd);
        weekday = tiers(weekday, 1, 9, 0xd);
    }
    c.put8(fixture + 0x76, weekday); c.put8(fixture + 0x77, first_time); c.put8(fixture + 0x78, second_time);
}
}
