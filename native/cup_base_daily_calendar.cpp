// CUP.CPP, block `daily`: a round's calendar entries (cup::rounds rows of 0x78 bytes) written by the derived cups'
// calendars. The dates come from DATE.CPP's calendar routine 0x107156f0 into a frame buffer; the row keeps the day
// and the year relative to the season (the year shifted back by `shift`).
#include "cup_base_daily.hpp"
#include "cup_base_detail.hpp"
#include "date.hpp"

namespace cm::aot::cup_base::daily {

void set_draw_date(Cpu& c, std::uint32_t rounds, std::uint32_t round, std::uint32_t day, std::uint32_t month,
                   std::uint32_t year, std::uint32_t weekday, std::uint32_t shift) {
    const Frame frame(c, 0x104854f0, 0x50, {.first_gpr = 29});
    // The original keeps the rows in r29, the round in r30 and the shift in r31 across the DATE call.
    c.set({{29, rounds}, {30, round}, {31, shift}});
    const auto date = frame.at(0x38);
    const auto day_argument = sext16(day), month_argument = sext8(month), weekday_argument = sext16(weekday);
    const auto year_argument = ((year & 0xffff) + shift) & 0xffff;
    c.set({{4, day_argument}, {5, month_argument}, {7, weekday_argument}, {6, year_argument}, {3, date}});
    c.link(0x10485534);
    date::from_calendar(c, date, day_argument, month_argument, year_argument, weekday_argument, false);
    const auto row = sext16(round) * cup::round_row;
    const auto first = sext16(c.u16(date));
    c.set(4, first);
    c.put16(rounds + row, first);
    const auto relative_year = c.u16(date + 2) - shift;
    c.set(3, relative_year);
    c.put8(rounds + row + 2, relative_year);
}

void set_fixture_date(Cpu& c, std::uint32_t rounds, std::uint32_t round, std::uint32_t day, std::uint32_t month,
                      std::uint32_t year, std::uint32_t weekday, std::uint32_t kind, std::uint32_t shift,
                      std::uint32_t tail) {
    const Frame frame(c, 0x10485570, 0x60, {.first_gpr = 27});
    // r27 the round, r28 the kind, r29 the shift, r30 the stack word (read before the call), r31 the rows.
    c.set({{27, round}, {28, kind}, {29, shift}, {30, tail}, {31, rounds}});
    const auto date = frame.at(0x38);
    const auto year_argument = ((year & 0xffff) + shift) & 0xffff;
    // The day and month pass as they come (no sign extension here); the weekday is the r8 argument.
    c.set({{4, day}, {5, month}, {7, weekday}, {6, year_argument}, {3, date}});
    c.link(0x104855b4);
    date::from_calendar(c, date, day, month, year_argument, weekday, false);
    const auto row = sext16(round) * cup::round_row;
    const auto first = sext16(c.u16(date));
    c.set(3, first);
    c.put16(rounds + row + 4, first);
    const auto relative_year = c.u16(date + 2) - shift;
    c.set({{5, relative_year}, {4, row + 6}, {3, row + 7}});
    c.put8(rounds + row + 6, relative_year);
    c.put8(rounds + row + 7, kind);
    c.put32(rounds + row + 0x14, tail);
}

namespace {
// The four range checks share the assertion's shape; the original goes on after each (exit never comes back).
void range_failure(Cpu& c, const Frame& frame, const CheckSite& site) {
    report(c, frame, site);
    c.set(3, 0xffffffffu);
    invoke(c, callee::exit, site.at + 0x44, {0xffffffffu});
}
}

void set_round_slot(Cpu& c, std::uint32_t rounds, std::uint32_t round, std::uint32_t slot, std::uint32_t first,
                    std::uint32_t second, std::uint32_t third, std::uint32_t tail) {
    const Frame frame(c, 0x10485600, 0x100, {.first_gpr = 22});
    // The arguments live in r22-r28 (the slot's number in r24), the success flag, the message buffer and the pool in
    // r29-r31: the report's calls reach imports, where a replay compares them.
    c.set({{22, rounds}, {23, round}, {24, slot}, {25, first}, {26, second}, {27, third}, {28, tail},
           {29, c.toc(toc::success)}, {30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    if (std::int16_t(slot) < 0 || std::int16_t(slot) >= 8) range_failure(c, frame, {0x10485660, 0xb8, 0x50, 0x16dd});
    if (std::int16_t(first) < -1 || std::int16_t(first) > 6) range_failure(c, frame, {0x104856cc, 0x98, 0x48, 0x16e1});
    if (std::int8_t(second) < -1 || std::int8_t(second) >= 3) range_failure(c, frame, {0x10485738, 0x78, 0x40, 0x16e5});
    if (std::int8_t(third) < -1 || std::int8_t(third) > 3) range_failure(c, frame, {0x104857a4, 0x58, 0x38, 0x16e9});
    const auto offset = sext16(round) * cup::round_row + (sext16(slot) << 3);
    c.set({{5, offset}, {4, offset + 0x2d}, {3, offset + 0x2e}});
    c.put8(rounds + offset + 0x2c, first);
    c.put8(rounds + offset + 0x2d, second);
    c.put8(rounds + offset + 0x2e, third);
    c.put32(rounds + offset + 0x28, tail);
}

}
