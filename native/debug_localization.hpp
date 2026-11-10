#pragma once
#include <cstdint>
#include <string_view>

namespace cm::aot {
struct State;
enum class DebugText { best_xi, add_goal, goal, on, off, recover_squad, extend_contract, cancel_sale, remove_sale_clause, debugging, recover_players, play_for_free, underdogs, underdogs_max, options, repeat_goal, red_cards, injuries, options_sidebar, extend_all_contracts, match_results, draw, lineup, best_u23, b_team };
// Original language IDs and single-byte game text encoding; unknown IDs use English.
std::string_view debug_text(unsigned language, DebugText text);
std::string_view debug_text(const State&, DebugText text);
}
