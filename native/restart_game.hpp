#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
// Game > Restart, confirmed. The Mac original calls an empty routine here (0x1076f880) and carries on, so its
// Restart does nothing. The host gives it the meaning the control has: the game closes as on Quit (the
// original's own exit routine, 0x1076f7d0) and the application opens again at its start screen.
// Called by the generated menu bar handler at the two calls of that routine; does not return.
bool restart_game(State&, std::uint32_t pc);
// After the game has closed: opens the application again when a restart asked for it.
void reopen_after_restart();
}
