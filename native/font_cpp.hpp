#pragma once
// FONT.CPP (tools/font_cpp_audit.py): the game's text drawing layer over the font engine, rewritten as typed C++. Each function is
// one routine of the original; the adapter (font_cpp_adapter.cpp, font_cpp_routines*.inc) reads the guest ABI's arguments, calls it
// and leaves the results the original leaves. The routines are native inside and keep the guest ABI at their edge: guest memory is
// the only truth, the registers they leave (r3 and r4-r10) are those of the original's last callee or its own last instruction.
#include "guest_abi.hpp"

namespace cm::aot::font_cpp {
using guest::Cpu;

// The font list (block fontlist, font_cpp_fontlist.cpp): the fonts the preferences screen offers. Two built-in styles (index -1
// "Traditional", -2 "Futuristic") and the fonts found under the FONTS folder (index 0 up to the count), built on first use.
// 0x10750ca0: finds the data folder's FONTS path (SI_DATA, else CM3_DATA) into the folder buffer; 1 when found, else 0.
std::uint32_t locate_font_folder(Cpu&);
// 0x10750a00: builds the list when it is not built yet.
void load_font_list(Cpu&);
// 0x10750a60: selects a font by index (-2 and -1 the built-in styles, 0..count - 1 a listed font; an index below -2 is stored
// too: the original's compare lets it through).
void select_font(Cpu&, std::int32_t index);
// 0x10750b00: the number of listed fonts.
std::uint32_t font_count(Cpu&);
// 0x10750b60: the name of a font, at most `size` bytes with a terminator in the buffer's last byte (an empty string for an index
// outside the list).
void font_name(Cpu&, std::int32_t index, std::uint32_t buffer, std::int32_t size);
}
