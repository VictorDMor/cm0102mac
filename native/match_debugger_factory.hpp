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
inline constexpr std::uint64_t fresh_match_step_limit = 50000000;
}
