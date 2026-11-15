#pragma once
// The original's DISPLAY.CPP: the display table (areas, controls and their grids), mouse and keyboard
// input on it, drawing it, and its network form (sending and interpreting screen commands).
// 32 routines, 0x10718b40..0x10725a00 (tools/display_audit.py). Contract and quirks: docs/cpp-rewrites.md.
#include "display_table.hpp"
#include "display_input.hpp"
#include "display_drawing.hpp"
#include "display_network.hpp"
#include "display_interpreter.hpp"
