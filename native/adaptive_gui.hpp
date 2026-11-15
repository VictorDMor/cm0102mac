#pragma once
#include <cstdint>
#include "presentation.hpp"

namespace cm::aot {
struct State;
// Original coordinates remain the compatibility layout. Only the active
// native screen may select a larger canvas; no size enters a career record.
GuiCanvas gui_canvas(const State&);
bool adaptive_screen_enabled(std::uint32_t builder);
bool adaptive_gui_instruction(State&, std::uint32_t pc);
void draw_adaptive_background(State&);
}
