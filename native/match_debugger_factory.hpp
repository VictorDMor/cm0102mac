#pragma once
#include "match_debugger_session.hpp"
#include <filesystem>
#include <functional>
#include <string_view>

namespace cm::aot::match_debugger {
struct MatchSetup {
    std::uint32_t home_club = 0, away_club = 1, random_seed = 12345;
    std::uint16_t year = 2001, ordinal_day = 194;
};
struct MatchInputs {
    std::filesystem::path original_program, original_game, new_private_directory;
};
using CreationProgress = std::function<void(std::string_view)>;

// Creates an actual detailed match from original database records. No career,
// recorded match or synthetic football model is a creation input. Remaining
// original setup dependencies execute through the production dispatcher.
EngineInstance create_match(const MatchInputs&, const MatchSetup&,
    const CreationProgress& progress = {});

// create_match in two stages, for tools that start many matches from one
// world (restore a checkpoint of the prepared state for each): the world,
// services, match record and fixture up to the original initializer; then
// the initializer, substitutes and friendly binding.
struct MatchPreparation {
    std::unique_ptr<State> state;
    std::uint32_t match = 0, fixture = 0;
    std::array<std::vector<RosterMember>, 2> rosters{};
};
MatchPreparation prepare_match(const MatchInputs&, const MatchSetup&, const CreationProgress& progress = {});
EngineInstance start_match(std::unique_ptr<State>, std::uint32_t match, std::uint32_t fixture,
    std::array<std::vector<RosterMember>, 2> rosters, const CreationProgress& progress = {});
inline constexpr std::uint64_t fresh_match_step_limit = 50000000;
}
