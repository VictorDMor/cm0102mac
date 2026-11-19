// LEAGUE.CPP, block season: the small routines of the schedule. 0x104a9810 and 0x104a9890 are called thousands of
// times by the derived leagues' calendars (the Brazilian and English checkers script them); 0x104a9600 draws the
// next round of a league whose rounds are not fixed; 0x104a3350 is the game's general random draw (the same
// routine CUP_STAGE.CPP has as 0x104be7e0); 0x104a38d0 is the order the schedule is searched and sorted by.
#include "date.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "league_base_detail.hpp"
#include "league_base_season.hpp"
#include <bit>
#include <stdexcept>

namespace cm::aot::league_base::season {
using namespace detail;
using guest::Fpu;

std::int32_t compare_rounds(Cpu& c, std::uint32_t left, std::uint32_t right) {
    // Years as unsigned halfwords, then days as signed ones; the equal days give 0 or 1 by a branch-free test of
    // (a ^ b) >> 1 against (a ^ b) & b (the original's compare of two signed halfwords: a > b gives 1).
    const auto year_left = c.u16(left + Round::year), year_right = c.u16(right + Round::year);
    // The original keeps the first halfword in r5 (0x104a38d0, 0x104a38f4): the qsort around it hands it on to its
    // caller, whose registers a paired check compares (0x104a1540 leaves it in r5).
    c.set(5, year_left);
    if (year_left < year_right) return -1;
    if (year_left > year_right) return 1;
    const auto first = std::int32_t(c.i16(left + Round::day)), second = std::int32_t(c.i16(right + Round::day));
    c.set(5, std::uint32_t(first));
    if (first < second) return -1;
    const auto difference = std::uint32_t(first) ^ std::uint32_t(second);
    const auto shifted = std::int32_t(difference) >> 1;
    return std::int32_t((std::uint32_t(shifted) - (difference & std::uint32_t(first))) >> 31);
}

std::int32_t next_round(Cpu& c, std::uint32_t previous, std::uint32_t phase, std::uint32_t competition,
                        std::uint32_t clubs, std::uint32_t legs, std::uint32_t rounds, std::uint32_t flags) {
    const Frame frame(c, 0x104a9600, 0x50, {.first_gpr = 27});
    const auto before = std::int32_t(previous);
    const auto last = std::int32_t(std::int16_t(rounds));
    // The callers read r4-r10 after it (0x104a00a0 returns them): the original loads r10 with the special competition's
    // cell at once and leaves r5-r9 as given; r4 is set on the paths below that load it.
    c.set(10, c.toc(slot::special_competition));
    // The competition the game gives its own order (the cell at TOC -0x7a30).
    if (std::int32_t(competition) == c.i32(c.toc(slot::special_competition))) {
        if (before == 2) return 4;
        return before == 5 ? 3 : before + 1;
    }
    if (std::int16_t(legs) < 2 || std::int16_t(clubs) < 8) return before + 1;
    const auto mode = std::int32_t(std::int16_t(flags));
    if (mode & 0x40) return before + 1;
    if (mode & 0x80) {
        // The two halves of the season: the round after the first half's last wraps to the second half's start.
        const auto next = before + 1;
        const auto half = (last + std::int32_t(std::uint32_t(last) >> 31)) >> 1;
        c.set(4, std::uint32_t(last));
        if (next == half) return last - 1;
        if (next == last) return half;
        return next;
    }
    const auto wrap = [&](std::int32_t round) { return round < last ? round : std::int32_t(std::uint32_t(round) - std::uint32_t(last)); };
    const auto step_back = [&]() { const auto round = before - 1; return round >= 0 ? round : last - 1; };
    switch (std::int8_t(c.u8(phase))) {
    case 2: c.put8(phase, 3); return step_back();
    case 1: c.put8(phase, 2); return step_back();
    case 3: c.set(4, std::uint32_t(last)); c.put8(phase, 0); return wrap(before + 3);
    default: break;  // 0, a negative byte or 4 and more: a draw
    }
    if (before >= last - 5) return wrap(before + 1);
    const auto pool = Pool::at(c);
    c.set(4, c.u32(pool.cursor));
    pool.advance(0x104a9780);
    // The sample's low two bits as the original's rotate-and-subtract reads them (the sign bit of the sum adjusts).
    const auto sum = pool.sample();
    const auto sign = std::rotl(sum, 1) & 1u;
    const auto low = std::int32_t(std::rotl(std::uint32_t(std::rotl(sum, 30) & 0xc0000000u) - sign, 2) + sign);
    if (low < 2) return wrap(before + 1);
    c.put8(phase, 1);
    return wrap(before + 3);
}

void set_round_date(Cpu& c, std::uint32_t rounds, std::uint32_t index, std::uint32_t day, std::uint32_t month,
                    std::uint32_t year, std::uint32_t weekday, std::uint32_t kind, std::uint32_t shift, std::uint32_t tail) {
    const Frame frame(c, 0x104a9810, 0x60, {.first_gpr = 27});
    const auto moved = sext16(shift);
    const auto date = frame.at(0x38);
    c.set({{27, rounds}, {28, index}, {29, kind}, {30, tail}, {31, moved}});
    c.set({{3, date}, {4, day}, {5, month}, {6, (year + moved) & 0xffff}, {7, weekday}});
    c.link(0x104a9850);
    date::from_calendar(c, date, day, month, (year + moved) & 0xffff, weekday, false);
    const auto round = rounds + sext16(index) * league::round_row;
    c.put16(round + Round::day, c.u16(date));
    c.set(3, round);
    c.put16(round + Round::year, c.u16(date + 2) - moved);
    c.put8(round + Round::kind, kind);
    c.put32(round + Round::tail, tail);
}

void set_match_slot(Cpu& c, std::uint32_t rounds, std::uint32_t index, std::uint32_t slot_index, std::uint32_t first,
                    std::uint32_t second, std::uint32_t third, std::uint32_t tail) {
    const Frame frame(c, 0x104a9890, 0x100, {.first_gpr = 22});
    // The original keeps its arguments in r22-r28 and the flag, message buffer and pool in r29-r31; the report's
    // calls reach imports, where a replay compares them.
    c.set({{22, rounds}, {23, index}, {24, slot_index}, {25, first}, {26, second}, {27, third}, {28, tail},
           {29, c.toc(toc::success)}, {30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    const auto slot_number = std::int16_t(slot_index);
    // Each range failure reports and goes on: the slot is written whatever the arguments were.
    if (slot_number < 0 || slot_number >= 8) report(c, frame, {0x104a98f0, 0xb8, 0x50, 0x168c});
    if (std::int16_t(first) < -1 || std::int16_t(first) > 6) report(c, frame, {0x104a9954, 0x98, 0x48, 0x1690});
    if (std::int8_t(second) < -1 || std::int8_t(second) >= 3) report(c, frame, {0x104a99b8, 0x78, 0x40, 0x1694});
    if (std::int8_t(third) < -1 || std::int8_t(third) > 4) report(c, frame, {0x104a9a1c, 0x58, 0x38, 0x1698});
    const auto slot_at = rounds + sext16(index) * league::round_row + (sext16(slot_index) << 3);
    c.put8(slot_at + 0xc, first);
    c.put8(slot_at + 0xd, second);
    c.put8(slot_at + 0xe, third);
    c.put32(slot_at + 0x8, tail);
}

void preset_fixture(Cpu& c, std::uint32_t fixture, std::uint32_t league_object, std::uint32_t day, std::uint32_t month,
                    std::uint32_t year, std::uint32_t home_name, std::uint32_t away_name) {
    const Frame frame(c, 0x104aab30, 0x50, {.first_gpr = 26});
    c.set({{28, fixture}, {29, league_object}, {30, home_name}, {31, away_name}});
    c.set({{3, fixture}, {4, day}, {5, month}, {6, year}, {7, 0xffffffffu}});
    c.link(0x104aab64);
    date::from_calendar(c, fixture, day, month, year, 0xffffffffu, true);
    c.put32(fixture + PresetFixture::away, 0xffffffffu);
    c.put32(fixture + PresetFixture::home, 0xffffffffu);
    // The first row (by the standings' order) whose club's name equals each name; the walk goes on while either is
    // still missing, so a later row with the same name replaces an earlier one until both are found.
    for (std::int32_t row = 0; row < c.i16(league_object + field::clubs); ++row) {
        const auto at = std::uint32_t(row) * league::standing_row;
        c.set({{26, std::uint32_t(row)}, {27, at}});
        const auto club_of = [&] { return c.u32(c.u32(league_object + field::standings) + at); };
        const auto name = invoke(c, entry::club_name, 0x104aab88, {club_of()});
        if (invoke(c, comp::callee::strcmp, 0x104aab90, {name, home_name}) == 0)
            c.put32(fixture + PresetFixture::home, c.u32(club_of()));
        const auto other = invoke(c, entry::club_name, 0x104aabb4, {club_of()});
        if (invoke(c, comp::callee::strcmp, 0x104aabbc, {other, away_name}) == 0)
            c.put32(fixture + PresetFixture::away, c.u32(club_of()));
        if (c.i32(fixture + PresetFixture::home) != -1 && c.i32(fixture + PresetFixture::away) != -1) break;
    }
}

namespace {
// Signed division by 65535 and by 10 as the original's multiply-high sequences do it (the quotient rounds toward
// zero; `floor` is the value before the sign correction).
std::int32_t multiply_high(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * b) >> 32); }
std::int32_t divide_65535(std::int32_t value) {
    const auto floor = std::int32_t(std::uint32_t(multiply_high(std::int32_t(0x80008001u), value)) + std::uint32_t(value)) >> 15;
    return std::int32_t(std::uint32_t(floor) + (std::uint32_t(floor) >> 31));
}
std::int32_t divide_ten(std::int32_t value) {
    const auto floor = multiply_high(0x66666667, value) >> 2;
    return std::int32_t(std::uint32_t(floor) + (std::uint32_t(floor) >> 31));
}
}

std::int32_t random_below(Cpu& c, std::int32_t bound) {
    const Frame frame(c, 0x104a3350, 0x80, {.first_gpr = 20});
    if (bound == 0) return 0;
    const auto pool = Pool::at(c);
    pool.advance(0x104a33a4);
    if (bound < 0x10000 && bound > -0x10000) return reduce(pool.sample(), bound);
    // A bound beyond +-65535: the draw splits into a scaled part and a finer one drawn (by the same routine) from a
    // bound a tenth of the size... The original unrolls four levels of that recursion and calls itself for the
    // fifth. The next bound is `bound * 10 / 65535` with the product wrapped to 32 bits, so it is at most 32768 in
    // size: the second level always takes the small arm and the deeper levels (0x104a3688..0x104a38b0 and the
    // recursive call at 0x104a372c) cannot run.
    Fpu fpu(c);
    const auto constants = c.toc(slot::doubles);
    const auto as_double = [](std::int32_t value) {
        return std::bit_cast<double>(std::uint64_t(0x43300000u) << 32 | (std::uint32_t(value) ^ 0x80000000u));
    };
    const auto scale = fpu.fdiv(fpu.fsub(as_double(bound), c.f64(constants + 8)), c.f64(constants));
    const auto next_bound = divide_65535(std::int32_t(std::uint32_t(bound) * 10u));
    const auto sum = std::int32_t(pool.sample());
    const auto remainder = std::int32_t(std::uint32_t(sum) - std::uint32_t(divide_65535(sum)) * 0xffffu);
    const auto part = fpu.fmul(scale, fpu.fsub(as_double(remainder), c.f64(constants + 8)));
    auto result = std::int32_t(std::uint32_t(fpu.fctiwz(0, part)));
    std::int32_t finer = 0;
    if (next_bound != 0) {
        pool.advance(0x104a3490);
        if (next_bound >= 0x10000 || next_bound <= -0x10000)
            throw std::logic_error("0x104a3350: the second level's bound is beyond 65535 (unreachable: |bound * 10 / 65535| <= 32768)");
        finer = reduce(pool.sample(), next_bound);
    }
    result = std::int32_t(std::uint32_t(result) + std::uint32_t(divide_ten(std::int32_t(std::uint32_t(finer) + 5u))));
    const auto limit = std::int32_t(std::uint32_t(bound) - 1u);
    if (result > limit) result = limit;
    return result;
}

}
