#pragma once
#include <cstdint>
#include <optional>

namespace cm::aot {
struct State;
struct CommandContext;
struct UIEvent;
// Host-only preference; it is never stored in an original career or resource.
bool native_debugging_available();
bool native_debugging_enabled();
void initialize_native_debugging(bool enabled = false);
void select_native_debugging(bool enabled);
enum class DebugFeature : unsigned { recover_players, play_for_free, extend_contract, cancel_sale,
    remove_sale_clause, underdogs, underdogs_max, add_goal, repeat_goal, red_cards, injuries, extend_all_contracts,
    match_result, force_purchase, count };
constexpr unsigned debug_feature_count = unsigned(DebugFeature::count);
constexpr std::uint32_t all_debug_features = (1u << debug_feature_count) - 1;
std::uint32_t native_debugging_features();
bool native_debugging_feature(DebugFeature);
void select_native_debugging_features(std::uint32_t);
void append_debugging_sidebar(State&);
bool dispatch_debugging_sidebar(State&);
bool build_debugging_options_screen(State&);

void append_debugging_option(State&);
void begin_debugging_options();
bool dispatch_debugging_option(State&);
void append_fixture_debugging_controls(State&);
// After 1028ae80 builds its rows: right-click result cells and their marks.
void append_fixture_result_controls(State&);
bool dispatch_fixture_debugging_control(State&);
void append_match_debugging_controls(State&);
bool dispatch_match_debugging_control(State&);
bool deliver_match_shortcut(State&, const CommandContext&, const UIEvent&);
void reset_match_debugging();
bool append_forced_club_control(State&);
void append_best_xi_control(State&);
bool dispatch_best_xi_control(State&);
void append_debug_contract_action(State&);
bool dispatch_debug_contract_action(State&);
bool extend_debug_contract(State&, std::uint32_t staff);
// Extend primary contracts owned by the club, including non-playing staff and loans out.
bool extend_debug_club_contracts(State&, std::uint32_t club);
bool cancel_scheduled_sale(State&, std::uint32_t staff);
bool remove_debug_sale_clause(State&, std::uint32_t staff);
// Select the match squad by decoded attributes, positions, condition and current
// tactical instructions, optimizing the eleven roles together. Fixture may be
// zero between fixtures; then the original UI's full nine-place bench is used.
// best_xi: everyone available. under_23: only players under 23 on the game date
// (original age operation 10717dd0), bench included. b_team: the best eleven of
// those left once the Best XI's starters are removed; the bench comes from the rest.
enum class LineupMode { best_xi, under_23, b_team };
bool select_debug_best_xi(State&, std::uint32_t club, unsigned managed,
                         std::uint32_t formation, std::uint32_t fixture,
                         LineupMode mode = LineupMode::best_xi);
// Recover every player in the club roster, including unselected reserves.
bool recover_debug_squad(State&, std::uint32_t club);
// Set every roster player's primary-contract wage to zero (one euro if the
// original store refuses zero) and clear clauses, bonuses and the release amount.
bool free_debug_squad(State&, std::uint32_t club);

// Shared validation for the live UI and focused original-engine checks.
bool eligible_debug_goal(const State&, std::uint32_t match, unsigned player);
void award_debug_goal(State&, std::uint32_t match, unsigned player);
enum class DebugMatchEvent { goal, red_card, injury };
unsigned debug_player_weight(const State&, std::uint32_t match, unsigned player, DebugMatchEvent);
std::optional<unsigned> random_debug_player(const State&, std::uint32_t match, unsigned side, DebugMatchEvent);
void award_debug_red_card(State&, std::uint32_t match, unsigned player);
unsigned random_debug_injury();
void award_debug_injury(State&, std::uint32_t match, unsigned player, unsigned injury);
}
