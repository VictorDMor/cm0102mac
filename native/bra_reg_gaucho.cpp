#include "bra_reg_gaucho_detail.hpp"
namespace cm::aot::bra_reg_gaucho {
using namespace detail;
std::uint32_t collect_clubs(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f1d60);
    Frame f(c, 0x106f1d60, 0x80, {.first_gpr = 26});
    Kept<30> object(c, address);
    Kept<26> total(c, c.toc(slot::club_total));
    Kept<27> division_id(c, c.toc(slot::regional));
    Kept<31> count(c, 0);
    Kept<28> index(c, c.toc(slot::message));
    Kept<29> club(c, c.toc(slot::pool));
    Program p{c}; const Competition league{c, address};
    league.club_count() = 17;
    p.call(0x10775130, 0x106f1d9c, {std::uint32_t(std::int32_t(league.club_count())) * standing_size});
    const auto allocated = c.r(3);
    compare_logical(c, allocated); league.standings() = allocated;
    if (allocated == 0) {
        assertion(c, f, 0x106f1dbc, 0x40, 0x38, 0x163);
        c.set(4, c.toc(slot::flag)); return 0;
    }
    index = 0; club = c.u32(c.toc(slot::clubs));
    while (compare(c, std::int16_t(std::uint32_t(index)), c.i32(total)) < 0) {
        const auto division = c.u32(std::uint32_t(club) + 0x38);
        if (compare_logical(c, division) != 0 && compare(c, c.i32(division), c.i32(division_id)) == 0) {
            c.set({{4, count}, {3, object}, {5, club}, {6, 0}, {7, 0}});
            count = std::uint32_t(count) + 1;
            c.call(0x104a60e0, 0x106f1e4c);
            if (compare(c, std::int8_t(std::uint32_t(count)), league.club_count()) == 0) break;
        }
        index = std::uint32_t(index) + 1; club = std::uint32_t(club) + club_size;
    }
    return compare(c, std::int8_t(std::uint32_t(count)), league.club_count()) == 0;
}

void fixture_times(Cpu& c, std::uint32_t address, std::uint32_t stage, std::uint32_t weekday,
                   std::uint32_t first_time, std::uint32_t second_time) {
    observe_entry(c, 0x106f2a30);
    Frame f(c, 0x106f2a30, 0x50, {.first_gpr = 28});
    keep(c, {{28, stage}, {29, weekday}, {30, first_time}, {31, second_time}});
    Program p{c};
    const auto fixture = p.call(0x10459b50, 0x106f2a64, {c.u32(address + 8)});
    if (compare(c, std::int8_t(stage), -1) == 0) {
        if (compare_logical(c, first_time & 255, 5) < 0) first_time = 1;
        if (compare_logical(c, second_time & 255, 5) < 0) second_time = 3;
    }
    c.put8(fixture + 0x76, weekday); c.put8(fixture + 0x77, first_time); c.put8(fixture + 0x78, second_time);
}
}
