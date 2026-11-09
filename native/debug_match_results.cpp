#include "debug_match_results.hpp"
#include "aot_runtime.hpp"
#include "match_debugging.hpp"
#include <algorithm>

namespace cm::aot {
namespace {
MatchResultPlan identity(const State& s, std::uint32_t fixture, ForcedMatchResult result) {
    const auto word = [&](unsigned offset) { return std::uint32_t(s.memory.read<4>(fixture + offset)); };
    return {word(0x1c), word(0x20), word(0x28), word(0), word(0x30), result};
}
bool matches(const MatchResultPlan& a, const MatchResultPlan& b) {
    return a.home == b.home && a.away == b.away && a.date == b.date &&
        a.competition == b.competition && a.round == b.round;
}
bool human_fixture(const State& s, std::uint32_t fixture) {
    const auto wanted = identity(s, fixture, ForcedMatchResult::normal);
    // Human-manager records include spectators without a managed team. The
    // record points at a queue entry: original core pointer, then copied core.
    for (unsigned i = 0; i < 16; ++i) {
        const auto queued = std::uint32_t(s.memory.read<4>(0x109806f0 + i * 0xfc + 0xec));
        if (!queued) continue;
        if (s.memory.read<4>(queued) == fixture || matches(wanted, identity(s, queued + 4, ForcedMatchResult::normal)))
            return true;
    }
    return false;
}
}

bool debug_match_result_eligible(const State& s, std::uint32_t fixture) {
    if (!fixture || std::int8_t(s.memory.read<1>(fixture + 0x43)) != -1) return false;
    const auto plan = identity(s, fixture, ForcedMatchResult::normal);
    return plan.home && plan.away && plan.home != plan.away && !human_fixture(s, fixture);
}

ForcedMatchResult debug_match_result(const State& s, std::uint32_t fixture) {
    const auto& plans = match_result_debugging(s).fixtures;
    if (!fixture || plans.empty()) return ForcedMatchResult::normal;
    const auto wanted = identity(s, fixture, ForcedMatchResult::normal);
    const auto found = std::ranges::find_if(plans, [&](const auto& plan) { return matches(plan, wanted); });
    return found == plans.end() ? ForcedMatchResult::normal : found->result;
}

bool set_debug_match_result(State& s, std::uint32_t fixture, ForcedMatchResult result) {
    if (!native_debugging_feature(DebugFeature::match_result) || !debug_match_result_eligible(s, fixture)) return false;
    if (result != ForcedMatchResult::normal && result != ForcedMatchResult::home_win &&
        result != ForcedMatchResult::draw && result != ForcedMatchResult::away_win) return false;
    const auto selected = identity(s, fixture, result);
    auto& plans = match_result_debugging(s).fixtures;
    std::erase_if(plans, [&](const auto& plan) { return matches(plan, selected); });
    if (result != ForcedMatchResult::normal) plans.push_back(selected);
    return true;
}

std::optional<std::array<unsigned, 2>> debug_match_score(const State& s, std::uint32_t match) {
    if (match_result_debugging(s).fixtures.empty() || !native_debugging_feature(DebugFeature::match_result)) return {};
    // Byte zero selects the original quick simulation. Never affect a match
    // being watched, even if it was marked before the user chose to attend.
    if (!s.memory.read<1>(match)) return {};
    const auto fixture = std::uint32_t(s.memory.read<4>(match + 0x3118));
    if (!debug_match_result_eligible(s, fixture)) return {};
    const auto result = debug_match_result(s, fixture);
    if (result == ForcedMatchResult::normal) return {};
    const auto plan = identity(s, fixture, result);
    // Stable variation across fixtures, without consuming the game's RNG just
    // to configure this optional feature. Football events still use its RNG.
    const auto home_id = std::uint32_t(s.memory.read<4>(plan.home));
    const auto away_id = std::uint32_t(s.memory.read<4>(plan.away));
    const auto variation = (home_id * 2654435761u) ^ (away_id * 2246822519u) ^ plan.date ^ plan.round;
    if (result == ForcedMatchResult::draw) {
        constexpr unsigned goals[]{0, 1, 1, 1, 2};
        const auto value = goals[variation % std::size(goals)];
        return std::array{value, value};
    }
    constexpr std::array<unsigned, 2> scores[]{{1, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 1}, {3, 1}, {3, 2}};
    auto score = scores[variation % std::size(scores)];
    if (result == ForcedMatchResult::away_win) std::swap(score[0], score[1]);
    return score;
}

void finish_debug_match_result(State& s, std::uint32_t fixture) {
    if (!fixture || match_result_debugging(static_cast<const State&>(s)).fixtures.empty()) return;
    const auto finished = identity(s, fixture, ForcedMatchResult::normal);
    std::erase_if(match_result_debugging(s).fixtures, [&](const auto& plan) { return matches(plan, finished); });
}
}
