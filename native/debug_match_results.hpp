#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace cm::aot {
struct State;
enum class ForcedMatchResult { normal, home_win, draw, away_win };
enum class ForcedClubResult { win, draw, loss };
struct ClubResultRule {
    std::uint32_t club;
    ForcedClubResult result = ForcedClubResult::win;
};
struct MatchResultPlan {
    std::uint32_t home, away, date, competition, round;
    ForcedMatchResult result;
};
struct MatchResultDebugging {
    std::vector<MatchResultPlan> fixtures;
    std::vector<ClubResultRule> clubs;
    bool choosing_club = false;
    bool removing_club = false;
    std::optional<std::uint32_t> confirm_removal;
    unsigned club_page = 0;
};
// Manual choices last for this session; club rules persist beside each career save.
MatchResultDebugging& match_result_debugging(State&);
const MatchResultDebugging& match_result_debugging(const State&);
// Forgets every choice and rule. Unlike assigning through the accessor above, it does not create
// the runtime diagnostics of a state that has none (replays and check tools take other paths then).
void clear_match_result_debugging(State&);
bool debug_match_result_eligible(const State&, std::uint32_t fixture);
bool set_debug_match_result(State&, std::uint32_t fixture, ForcedMatchResult);
ForcedMatchResult debug_match_result(const State&, std::uint32_t fixture);
// Manual fixture choices take precedence; two listed clubs cancel both rules.
ForcedMatchResult effective_debug_match_result(const State&, std::uint32_t fixture);
bool valid_debug_result_club(const State&, std::uint32_t club);
bool add_debug_result_club(State&, std::uint32_t club);
bool set_debug_club_result(State&, std::uint32_t club, ForcedClubResult);
bool remove_debug_result_club(State&, std::uint32_t club);
// A separate companion file; original career bytes are never extended.
void load_debug_club_results(State&, const std::filesystem::path& save);
void save_debug_club_results(const State&, const std::filesystem::path& save);
std::optional<std::array<unsigned, 2>> debug_match_score(const State&, std::uint32_t match);
// Called at shared advancement; recognizes the original background callers.
void prepare_debug_match_result(State&, std::uint32_t match);
void finish_debug_match_result(State&, std::uint32_t fixture);
}
