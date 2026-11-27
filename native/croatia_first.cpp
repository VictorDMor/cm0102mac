#include "croatia_first.hpp"
#include "croatia_first_calendar.hpp"
#include "croatia_first_rollover.hpp"
#include "croatia_first_split.hpp"
#include "croatia_second_rules.hpp"
#include "league_base_detail.hpp"
#include "league_base_rollover.hpp"
#include "season_competition_rules.hpp"

namespace cm::aot::croatia_first {
bool format_group_news(guest::Cpu& c, std::uint32_t league, std::uint32_t kind) {
    const auto stage = std::int8_t(c.r(6));
    if (stage == -1 && std::int8_t(kind) != 1) return false;
    if (!c.s.platform || !league || competition_rules(c.s).croatian_first != CroatianFirstFormat::split_table_12 ||
        c.u32(league) != c.toc(-0x5ce0))
        return false;
    if (stage == -1) c.call(0x1057e810, c.lr());
    return true;
}

bool dispatch(State& s, std::uint32_t address) {
    guest::Cpu c(s);
    if (address == 0x1057ea40) {
        // Scotland owns this descriptor too. The sorting engine publishes the
        // league before invoking its comparator.
        const auto league = c.toc(league_base::toc::sort_league);
        if (!league || c.u32(league) != c.toc(-0x5ce0)) return false;
    }
    if (competition_rules(s).croatian_first != CroatianFirstFormat::split_table_12) return false;

    const auto back = c.lr();
    const auto league = c.r(3);
    switch (address) {
    case 0x106dae28:
        c.set(3, c.u32(c.sp() + 0x138));
        // Early A1 pilot saves named the northern A2 as the relegation destination.
        if (croatia_second::uses_single_table(s, c.i16(c.r(3) + comp::competition::year)) &&
            c.i32(c.r(3) + comp::competition::next) == c.i32(c.toc(-0x6a44)))
            c.put32(c.r(3) + comp::competition::next, c.u32(c.toc(croatia_second::national_division_slot)));
        restore_split_order(c, c.r(3));
        s.pc = address + 4;
        return true;
    case 0x106dbcd4:
        c.set(0, 12);
        s.pc = address + 4;
        return true;
    case 0x106dbde0:
        c.set(4, 20000);
        s.pc = address + 4;
        return true;
    case 0x106db190:
        c.set(3, calendar(c, league, c.r(4), c.r(5), c.r(6), c.r(7)));
        break;
    case 0x106dbe20:
        c.set(3, configure(c, league));
        break;
    case 0x1057ea40:
        c.set(3, compare_split(c, league, c.r(4)));
        break;
    case 0x106dcf20: {
        const auto date = c.r(4), context = c.r(5);
        maybe_split(c, league, context);
        c.set({{3, league}, {4, date}, {5, context}});
        c.set_lr(back);
        c.set(3, league_base::daily_update(c, league, date, context));
        break;
    }
    case 0x106dbf10:
        rollover(c, league);
        break;
    case 0x106dccc0:
        exchange_divisions(c, league);
        break;
    case 0x106dc5c0:
        // The Scottish title / upper group / relegation event classifier
        // has the same three outcomes and calling convention as CRO_A1 3.89.
        s.pc = 0x1057e780;
        return true;
    case 0x106dc240:
        break;
    default:
        return false;
    }
    c.set_lr(back);
    s.pc = back;
    return true;
}
}
