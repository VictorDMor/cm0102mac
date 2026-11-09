#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace cm::aot {
struct State;
enum class ForcedMatchResult { normal, home_win, draw, away_win };
struct MatchResultPlan {
    std::uint32_t home, away, date, competition, round;
    ForcedMatchResult result;
};
struct MatchResultDebugging {
    std::vector<MatchResultPlan> fixtures;
};
// Session-only choices, owned by this world; never part of a career save.
MatchResultDebugging& match_result_debugging(State&);
const MatchResultDebugging& match_result_debugging(const State&);
bool debug_match_result_eligible(const State&, std::uint32_t fixture);
bool set_debug_match_result(State&, std::uint32_t fixture, ForcedMatchResult);
ForcedMatchResult debug_match_result(const State&, std::uint32_t fixture);
std::optional<std::array<unsigned, 2>> debug_match_score(const State&, std::uint32_t match);
// Called at shared advancement; recognizes the original background callers.
void prepare_debug_match_result(State&, std::uint32_t match);
void finish_debug_match_result(State&, std::uint32_t fixture);
}
