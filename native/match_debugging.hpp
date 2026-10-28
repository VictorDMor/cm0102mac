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

void append_debugging_option(State&);
void begin_debugging_options();
bool dispatch_debugging_option(State&);
void append_match_debugging_controls(State&);
bool dispatch_match_debugging_control(State&);
bool deliver_match_shortcut(State&, const CommandContext&, const UIEvent&);
void reset_match_debugging();
void append_best_xi_control(State&);
bool dispatch_best_xi_control(State&);
void append_debug_contract_action(State&);
bool dispatch_debug_contract_action(State&);
bool extend_debug_contract(State&, std::uint32_t staff);
// Select the match squad using the original positional evaluator. Fixture may be
// zero between fixtures; then the original UI's full nine-place bench is used.
bool select_debug_best_xi(State&, std::uint32_t club, unsigned managed,
                         std::uint32_t formation, std::uint32_t fixture);
// Recover every player in the club roster, including unselected reserves.
bool recover_debug_squad(State&, std::uint32_t club);

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
