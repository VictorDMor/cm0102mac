#include "pol_cup_detail.hpp"
#include <array>

namespace cm::aot::pol_cup {
using namespace detail;
namespace {
struct Date { unsigned day, month, following_year, weekday; };
struct Round {
    unsigned number, flags, format, teams, ties, seeded, carried, legs, rules;
    Date draw, fixtures;
    unsigned fixture_weekday;
    unsigned exposed_count;
};
constexpr std::array<Round, 7> rounds{{
    {10, 0x403, 4, 8, 4, 8, 0, 1, 0, {6,7,0,4}, {9,7,0,2}, 2, 4},
    {20, 3, 4, 32,16,28,8,1,0, {11,7,0,4}, {13,8,0,2}, 2,16},
    {30, 0x403, 4,32,16,16,36,1,0, {15,8,0,4}, {23,8,0,5}, 1,16},
    {40, 0, 4,16,8,0,0,2,14, {29,8,0,4}, {8,10,0,2}, 2,8},
    {120,0,4,8,4,0,0,2,14, {3,2,1,4}, {14,2,1,2}, 2,8},
    {130,0,6,4,2,0,0,2,7, {1,3,1,4}, {11,3,1,2}, 2,4},
    {150,0,6,2,1,0,0,2,7, {19,3,1,3}, {16,4,1,1}, 2,0},
}};
void initialize_round(Cpu& c, std::uint32_t address, unsigned index, const Round& round) {
    c.put16(address + 8, round.number); c.put16(address + 10, index);
    c.put16(address + 12, index == 2 ? 8 : 0); c.put16(address + 14, round.flags);
    c.put16(address + 16, 0);
    // The final four rounds share header flag 3.
    if (index >= 3) c.put16(address + 16, 3);
    c.put8(address + 24, round.format);
    c.put16(address + 26, round.teams); c.put16(address + 28, round.ties);
    c.put16(address + 30, round.seeded); c.put16(address + 32, round.carried);
    c.put8(address + 34, 0); c.put8(address + 35, round.legs); c.put8(address + 36, round.rules);
    c.put32(address + 108, 0); c.put32(address + 112, index == 6 ? 500000 : 0);
    c.put32(address + 116, 0);
}
}
std::uint32_t calendar(Cpu& c, std::uint32_t address, std::uint32_t stage,
                       std::uint32_t round_count, std::uint32_t extra, std::uint32_t flag) {
    Frame f(c, 0x106cdbe0, 0x80, {.first_gpr = 28});
    Kept<30> object(c, address);
    keep(c, {{29, c.toc(-0x7ba8)}, {28, c.toc(slot::pool)}});
    Program p{c};
    if (compare(c, std::int8_t(stage), -1) != 0) return 0;
    if (compare_logical(c, flag) != 0) c.put8(flag, 0);
    c.put16(round_count, 7); c.put16(extra, 0);
    Kept<31> schedule(c, p.call(0x10775130, 0x106cdc3c, {0x348}));
    if (compare(c, schedule) == 0) {
        assertion(c, f, 0x106cdc58, 0x48, 0x40, 0x9d);
        c.set(4, c.toc(-0x7bac)); return 0;
    }
    constexpr std::array<std::uint32_t, 7> draw_back{
        0x106cdcbc,0x106cdd60,0x106cde10,0x106cdec4,0x106cdf70,0x106ce018,0x106ce0c4};
    constexpr std::array<std::uint32_t, 7> fixture_back{
        0x106cdce8,0x106cdd8c,0x106cde3c,0x106cdef0,0x106cdf9c,0x106ce044,0x106ce0f0};
    for (unsigned index = 0; index < rounds.size(); ++index) {
        const auto& round = rounds[index];
        p.call(0x104854f0, draw_back[index], {schedule, index, round.draw.day, round.draw.month,
               round.draw.following_year, round.draw.weekday, extended(Cup{c, object}.year())});
        c.put32(f.at(0x38), 0);
        p.call(0x10485570, fixture_back[index], {schedule, index, round.fixtures.day, round.fixtures.month,
               round.fixtures.following_year, round.fixtures.weekday, round.fixture_weekday,
               extended(Cup{c, object}.year())});
        initialize_round(c, std::uint32_t(schedule) + index * 0x78, index, round);
        // The original reuses its saved pool/message registers while filling
        // successive round records; later callees spill those exact values.
        if (index == 1) keep(c, {{28, 1}});
        if (index == 2) keep(c, {{28, 0}});
        if (index == 3) keep(c, {{28, 3}, {29, 0}});
        if (index == 4) keep(c, {{29, 4}});
        if (index == 5) keep(c, {{29, 0}});
        if (index != 6) {
            const auto& next = rounds[index + 1].draw;
            leave(c, {{3, schedule}, {4, index + 1}, {5, next.day}, {6, next.month},
                      {7, next.following_year}, {8, next.weekday}, {9, extended(Cup{c, object}.year())},
                      {10, round.exposed_count}});
        }
    }
    leave(c, {{3, schedule}, {4, 7}, {5, 1}, {6, 2}, {7, 3}, {8, 0}, {9, 6}});
    return schedule;
}
}
