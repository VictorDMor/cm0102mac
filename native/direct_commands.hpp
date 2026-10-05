#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace cm::aot {
struct State;
enum class DirectCommand : std::uint32_t { continue_game, save_game, load_game, open_game_options, back_or_cancel, count };
constexpr auto direct_command_count = std::size_t(DirectCommand::count);
constexpr std::array<std::string_view, direct_command_count> direct_command_names{
    "continue_game", "save_game", "load_game", "open_game_options", "back_or_cancel"};
struct CommandContext {
    std::uint32_t root = 0, screen = 0, handler = 0;
    std::uint64_t identity = 0;
    std::array<std::int16_t, direct_command_count> actions{};
    std::array<std::int16_t, direct_command_count> controls{-1,-1,-1,-1,-1};
    bool ready = false;
    bool operator==(const CommandContext&) const = default;
};
// Read only the original active UI descriptor and control table, at its normal
// event boundary. No titles, coordinates, career data, or simulation fields.
CommandContext original_command_context(const State& state, std::uint16_t event_mask);
// A newly accepted host command replaces the previously delivered key input.
// Call only after context/menu validation, before dispatching its original action.
void clear_previous_command_key(State& state);
// Activate the original preselected control input (107193cc), consumed and
// cleared by the original picker before its screen handler receives the index.
// Only currently enabled named controls are accepted; no pointer position.
bool deliver_original_command(State& state, const CommandContext& expected, DirectCommand command);
}
