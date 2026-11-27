#include "croatia_second.hpp"
#include "croatia_second_calendar.hpp"
#include "croatia_second_rules.hpp"
#include "comp_detail.hpp"

namespace cm::aot::croatia_second {
bool dispatch(State& s, std::uint32_t address) {
    if (!s.platform) return false;
    guest::Cpu c(s);
    // The original entrant routine keeps the competition in r30 across its
    // database scan and prize calculation. At method entries it is in r3.
    const bool entrants = address == 0x106ddfe0 || address == 0x106ddff4 || address == 0x106de11c;
    const auto league = c.r(entrants ? 30 : 3);
    const auto record = c.u32(league + comp::competition::record);
    if (!record || c.i32(record) != c.i32(c.toc(national_division_slot)) ||
        !uses_single_table(s, c.i16(league + comp::competition::year))) return false;

    const auto back = c.lr();
    switch (address) {
    case 0x106ddfe0:
        c.set(27, c.toc(national_division_slot));
        s.pc = address + 4;
        return true;
    case 0x106ddff4:
        c.set(0, 18);
        s.pc = address + 4;
        return true;
    case 0x106de11c:
        // 3.89 stores the prize budget and mode, then returns directly.
        s.pc = 0x106de124;
        return true;
    case 0x106dd4b0:
        c.set(3, calendar(c, league, c.r(4), c.r(5), c.r(6), c.r(7)));
        break;
    case 0x106de140:
        // The regional competition suppresses the generic end-of-stage news.
        // CRO_SECOND starts the season with this notification still pending.
        c.put8(league + comp::competition::byte_49, 0);
        c.set(3, configure(c, league));
        break;
    case 0x106deca0:
        s.pc = 0x104a1540; // regular league day
        return true;
    case 0x106de840:
        s.pc = 0x104a47f0; // regular league placing / history
        return true;
    case 0x106de4c0:
        // The national table has no separate regional tiebreak stage.
        break;
    default:
        return false;
    }
    c.set_lr(back);
    s.pc = back;
    return true;
}
}
