#pragma once
#include <cstdint>
#include "presentation.hpp"

namespace cm::aot {
struct State;
// Original coordinates remain the compatibility layout. Only the active
// native screen may select a larger canvas; no size enters a career record.
GuiCanvas gui_canvas(const State&);
bool adaptive_screen_enabled(std::uint32_t builder);
bool adaptive_screen_available(const State&, std::uint32_t builder);
std::uint32_t adaptive_club_section(const State&);
bool adaptive_gui_instruction(State&, std::uint32_t pc);
// Check tools' views (the checkers have no platform): the canvas the hook sees in place of the platform's, and each instruction the hook is
// asked about with whether it replaced it. Both null in the app.
extern GuiCanvas (*adaptive_gui_canvas_probe)(const State&);
extern void (*adaptive_gui_instruction_probe)(State&, std::uint32_t pc, bool replaced);
void draw_adaptive_background(State&);
}
