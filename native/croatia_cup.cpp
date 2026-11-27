#include "croatia_cup.hpp"
#include "croatia_cup_profile.hpp"
#include "croatia_cup_lifecycle.hpp"
#include "croatia_cup_entrants.hpp"
#include "croatia_cup_rounds.hpp"
#include "cup_base_detail.hpp"
#include "season_competition_rules.hpp"

namespace cm::aot::croatia_cup {
namespace {
using guest::Cpu;
using u32 = std::uint32_t;
namespace object = comp::competition;

// The constructor tests for zero, while the season transition tests for a negative result.
u32 entrant_failure(u32 caller) {
    return caller == 0x106bf1cc ? 0u : UINT32_MAX;
}

}

const Profile& season_profile(Cpu& c, u32 cup) {
    return *competition_rules(c.s, RuleCompetition::croatian_cup, c.i16(cup + object::year)).croatian_cup;
}

u32 configure(Cpu& c, u32 cup, const Profile& profile) {
    const guest::Frame frame(c, 0x106bfe30, 0x40, {.first_gpr = 31});
    c.set(31, cup);
    c.put32(cup + object::word_28, profile.prize_pool);
    const auto count = cup + object::rounds, code = cup + object::code;
    c.set({{3, cup}, {4, UINT32_MAX}, {5, count}, {6, code}, {7, 0}});
    const auto rows = cup_method(c, cup, 0x44, 0x106bf4c0, 0x106bfe6c, [&] {
        c.set(3, calendar(c, cup, UINT32_MAX, count, code, 0, profile));
    });
    c.put32(cup + cup_base::cup::rounds, rows);
    // As on both releases, calendar reports allocation failure via the engine success flag.
    return 1;
}

bool dispatch(State& s, u32 address) {
    Cpu c(s);
    const u32 cup = c.r(3), back = c.lr();
    switch (address) {
    case 0x106bf0f0:
        c.set(3, construct(c, cup, c.r(4), c.r(5)));
        break;
    case 0x106bf3d0:
        c.set(3, destroy(c, cup, c.r(4)));
        break;
    case 0x106bfbe0:
        next_season(c, cup);
        break;
    case 0x106bf4c0:
        c.set(3, calendar(c, cup, c.r(4), c.r(5), c.r(6), c.r(7), season_profile(c, cup)));
        break;
    case 0x106bf9b0: {
        const auto result = select_entrants(c, cup, season_profile(c, cup));
        c.set(3, result < 0 ? entrant_failure(back) : u32(result));
        break;
    }
    case 0x106bfe30:
        c.set(3, configure(c, cup, season_profile(c, cup)));
        break;
    default:
        return false;
    }
    c.set_lr(back);
    s.pc = back;
    return true;
}
}
