// INJURY.CPP checker cases for the status questions (injury_unit_status.hpp).
//
// The injury manager is the game's own (global -0x7ba0): its status rows, one per person, and its
// injury records. People are the career's: some with an injury record (status row +0x10 is its
// index), most without.
#include "injury_unit_checks.hpp"
#include <string>

namespace cm::aot::injury_unit_checks {
namespace {
constexpr std::uint32_t condition_of = 0x10187650, set_status_flag = 0x10187a20, type_class = 0x1018c180,
                        current_injury = 0x1018c340;
}

INJURY_UNIT_CASES("status") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ba0);
    const auto rows = scenario.r32(manager);
    const auto people = scenario.people();
    // People with an injury record and without one, spread over the table.
    std::vector<std::uint32_t> injured, healthy;
    for (std::uint32_t index = 0; index < people && (injured.size() < 12 || healthy.size() < 12); index += 7) {
        auto& list = scenario.r32(rows + index * 0x24 + 0x10) == 0xffffffff ? healthy : injured;
        if (list.size() < 12) list.push_back(index);
    }
    for (std::uint32_t index = 0; index < people && injured.size() < 12; ++index)
        if (scenario.r32(rows + index * 0x24 + 0x10) != 0xffffffff) injured.push_back(index);

    for (const std::uint32_t index : {0u, 1u, people / 2, people - 1, people, people + 5})
        cases.push_back({.entry = condition_of, .registers = {{3, manager}, {4, index}},
                         .label = "condition of index " + std::to_string(index)});
    for (const auto index : injured)
        cases.push_back({.entry = condition_of, .registers = {{3, manager}, {4, index}},
                         .label = "condition of injured " + std::to_string(index)});

    for (const std::uint32_t value : {0u, 1u, 0x1ffu})
        for (const auto index : {healthy[0], injured.empty() ? healthy[1] : injured[0], people - 1})
            cases.push_back({.entry = set_status_flag, .registers = {{3, manager}, {4, scenario.person(index)}, {5, value}},
                             .label = "flag " + std::to_string(value) + " person " + std::to_string(index)});

    for (const std::uint32_t type : {0u, 1u, 17u, 40u, 81u, 0x100u + 5u})
        cases.push_back({.entry = type_class, .registers = {{3, manager}, {4, type}}, .label = "class of type " + std::to_string(type)});

    cases.push_back({.entry = current_injury, .registers = {{3, manager}, {4, 0}}, .label = "current injury, no person"});
    for (const auto index : injured)
        cases.push_back({.entry = current_injury, .registers = {{3, manager}, {4, scenario.person(index)}},
                         .label = "current injury of injured " + std::to_string(index)});
    for (const auto index : healthy)
        cases.push_back({.entry = current_injury, .registers = {{3, manager}, {4, scenario.person(index)}},
                         .label = "current injury of healthy " + std::to_string(index)});
    // A person record whose index is the people count: past the table, null without an update.
    {
        const auto record = scratch_base + 0x100;
        cases.push_back({.entry = current_injury, .registers = {{3, manager}, {4, record}},
                         .memory = {{4, record, people}}, .label = "current injury past the table"});
        // Closing an injury writes the history file (0x10748ce0), which the bridge does not implement: answered on both sides.
        for (auto& each : cases) if (each.entry == current_injury) each.answers.push_back({0x10748ce0, 0});
    }
}
}
