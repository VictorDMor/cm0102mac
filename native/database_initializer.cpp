// DATABASE.CPP's static initializer (database.hpp).
#include "database.hpp"
#include "guest_values.hpp"

namespace cm::aot::database {
namespace {
constexpr std::int32_t season_date_slot = -0x79e8;  // the module's date word (TOC)
constexpr std::uint32_t date_make = 0x107156f0;      // date(out, day, month, year, extra)
}

// 0x100de3b0: builds the date 31/07/2001 in a stack local through the date
// constructor and copies it into the module's date word: the 16-bit day
// (signed), the 16-bit second field and the byte at +4, in that order.
void initialize_season_date(Cpu& c) {
    c.set(0, c.lr());
    guest::Frame frame(c, 0x100de3b0, 0x50, {.first_gpr = 31});
    const auto date = c.toc(season_date_slot);
    guest::keep(c, {{31, date}});
    const auto local = frame.at(0x38);
    c.set({{3, local}, {4, 0x1f}, {5, 7}, {6, 0x7d1}, {7, 0xffffffffu}});
    c.call_in_place(date_make, 0x100de3dc);
    const auto first = guest::extended(c.i16(local));
    const auto second = std::uint32_t(c.u16(local + 2));
    c.put16(date, first);
    const auto last = std::uint32_t(c.u8(local + 4));
    c.put16(date + 2, second);
    c.put8(date + 4, last);
    c.set(3, second);
}
}
