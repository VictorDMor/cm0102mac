#pragma once
// DISPLAY.CPP block "table": the display table's lifecycle (reset, destroy, clear), its element
// destructors, the shared setup buffer, the unit's static initializer, the two "add a control" routines
// (a plain control and a grid control with their per-table lists), and a small anchor calculation.
//
// Layout (offsets from the table + 0x130000, as the generated code addresses them; names inferred from use):
//   -0x56d4 string_list, the list of owned blocks {pointer array, count}   -0x56dc, -0x56d8 owned buffers
//   -0x625c area count, -0x625a GUI object count (signed halfwords)
//   -0x5702 guio_limit / -0x5704 counts of the two lists of control indexes, at -0x6064 index_list (1200) / -0x6258 (250)
//   -0x56be / -0x56bf  "area table full" / "GUI object table full" flags
#include "display_detail.hpp"

namespace cm::aot::display {
// 0x10718b40: reset the table's fields (r4's low byte goes to -0x56c1).
void reset_table(Cpu&, std::uint32_t table, std::uint32_t flag);
// 0x10718ba0: destroy; frees the table itself when the halfword `flag` is positive. Returns the table.
std::uint32_t destroy_table(Cpu&, std::uint32_t table, std::uint32_t flag);
// 0x10718d80 / 0x10718dd0: element destructors (area / GUI object). Return the object.
std::uint32_t destroy_area(Cpu&, std::uint32_t object, std::uint32_t flag);
std::uint32_t destroy_gui_object(Cpu&, std::uint32_t object, std::uint32_t flag);
// 0x10718e20 / 0x10718ef0 / 0x10718f30: the shared setup buffer (0x6f544-byte records).
void allocate_setup_buffer(Cpu&, std::uint32_t count);
void free_setup_buffer(Cpu&);
void clear_setup_record(Cpu&, std::uint32_t index);
// 0x1071c070: the anchor point of a rectangle for a corner code.
void corner_anchor(Cpu&, std::uint32_t corner, std::uint32_t value);
// 0x107259a0: static initializer.
void initialize_unit(Cpu&);
// 0x10719560: clear a table in use (children destroyed when the flag byte is set and screen_set (-0x56bc) is clear).
void clear_table(Cpu&, std::uint32_t table, std::uint32_t destroy_children);
// 0x10719740: add a control; returns its index or -1.
std::uint32_t add_control(Cpu&, std::uint32_t table, std::uint32_t r4, std::uint32_t r5, std::uint32_t r6,
                          std::uint32_t r7, std::uint32_t r8, std::uint32_t r9, std::uint32_t r10);
// 0x107198c0: add a grid control (an area); sends its description to the network stream. Returns its index or -1.
std::uint32_t add_grid_control(Cpu&, std::uint32_t table, std::uint32_t r4, std::uint32_t r5, std::uint32_t r6,
                               std::uint32_t r7, std::uint32_t r8, std::uint32_t r9, std::uint32_t r10);
// 0x1071a100 / 0x1071a1c0: append an index to the table's control / area index list.
void add_control_index(Cpu&, std::uint32_t table, std::uint32_t index);
void add_area_index(Cpu&, std::uint32_t table, std::uint32_t index);
}
