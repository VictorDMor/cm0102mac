// MATCH_MAN.CPP checker cases for the leaves block.
#include "match_man_checks.hpp"

namespace cm::aot::match_man_checks {
MATCH_MAN_CASES("leaves") {
    if (!scenario.match) {
        // The callback reads nothing: any state shows what it leaves.
        cases.push_back({.entry = 0x102065e0, .registers = {{3, 0}}, .label = "empty callback"});
        return;
    }
    cases.push_back({.entry = 0x102065e0, .registers = {{3, scenario.match}}, .label = "empty callback"});
    for (unsigned side = 0; side < 2; ++side) {
        const auto record = scenario.side(side);
        const auto players = scenario.r32(record + 0x28);
        for (unsigned index : {0u, 1u, 10u, 19u})
            cases.push_back({.entry = 0x102107c0, .registers = {{3, record}, {4, scenario.r32(players + index * 0x1c4 + 0x6c)}},
                             .label = "person of player " + std::to_string(index)});
        cases.push_back({.entry = 0x102107c0, .registers = {{3, record}, {4, 0x7fffffff}}, .label = "absent person"});
        cases.push_back({.entry = 0x102107c0, .registers = {{3, record}, {4, 0}}, .label = "null person"});
    }
}
}
