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

// The style selector (block selector, font_cpp_selector.cpp) and the glyph cache and string width (block glyphs, font_cpp_glyphs.cpp). All
// three are stack-contract leavers: their frames and the registers they hold at each call are the original's. `slot` is the pointer to the
// slot that holds the font record's pointer (the callers' Font**); `style` and `size` are the callers' words as they hold them (the low
// halfword and the low byte are used).
// 0x10750320: makes the font record's sfnt and scaler match the style (bit 0 bold, bit 1 italic: the sfnt is recreated; bits 2 and 3 the
// skew's angle, with the size: the scaler's transformation) and stores the style and size in the record.
void select_style(Cpu&, std::uint32_t slot, std::uint32_t style, std::uint32_t size);
// 0x10750580: the glyph record of a character for a style and size, from the cache (moved to the front) or rendered into a new record
// (the oldest is reused when 256 are cached; style bit 0x10 smooths the bitmap with a 5x5 box). Returns the record's address.
std::uint32_t glyph_record(Cpu&, std::uint32_t slot, std::uint32_t style, std::uint32_t size, std::uint32_t character);
// 0x107500f0: the width of a string in whole pixels (a halfword): the advances of its characters, the last one's by its bitmap's left and
// width; 0 for a font record that is not loaded.
std::int32_t string_width(Cpu&, std::uint32_t slot, std::uint32_t style, std::uint32_t size, std::uint32_t text);
// Where a font's bytes come from (block fontfiles, font_cpp_fontfiles.cpp). Each returns what the original leaves in r3 (and r4-r10 as it leaves them).
// 0x1074d380: an object over a resource file ({+0 the current file, +2 the opened one}); `create` makes the file when it cannot be opened.
std::uint32_t open_resource_file(Cpu&, std::uint32_t object, std::uint32_t spec, std::uint32_t permission, std::uint32_t create);
// 0x1074d450: closes it (restoring the previous current file); a positive `flag` also frees the object.
std::uint32_t close_resource_file(Cpu&, std::uint32_t object, std::uint32_t flag);
// 0x1074f2c0: opens a .ttf or .t2k file for reading (index 0 only), the FILE stored at +0 of `font`; 1 when opened.
std::uint32_t open_font_file(Cpu&, std::uint32_t path, std::uint32_t font, std::uint32_t index);
// 0x1074f170: font number `index` of a file: an 'sfnt' resource (the detached Handle at +4 of `font`) or the file itself; 1 when found.
std::uint32_t find_font_data(Cpu&, std::uint32_t path, std::uint32_t font, std::uint32_t index);
// 0x10750f00: the full name (name id 4) of the font data in `font` read from its 'name' table into `buffer` (at most size - 1 bytes); 1 when found.
std::uint32_t read_font_name(Cpu&, std::uint32_t font, std::uint32_t buffer, std::uint32_t size);
// 0x10750d30: builds the font list (the FONTS folder's files, their fonts and names); 1 when there are fonts and the table was allocated.
std::uint32_t scan_font_folder(Cpu&);
// The life of a font record (block record, font_cpp_record.cpp). `slot` is the pointer to the slot that holds the record's pointer.
// 0x1074f390: the array's constructor of an element: builds the font list (0x10750a00), allocates the 0x24-byte record and zeroes it.
std::uint32_t new_font_record(Cpu&, std::uint32_t slot);
// 0x1074f410: its destructor: releases and frees the record; a positive `flag` frees the slot too.
std::uint32_t delete_font_record(Cpu&, std::uint32_t slot, std::uint32_t flag);
// 0x10750200: frees everything the record holds (glyph cache, scaler, sfnt, stream, memory object, file copy) and zeroes it.
std::uint32_t release_font(Cpu&, std::uint32_t slot);
// 0x1074f470: loads the record from the selected font's file or from small/medium/large.t2k or symbol.ttf of the data folder, by `kind`
// (0 small, 1 medium, 2 large, 3 symbol); 1 when loaded.
std::uint32_t load_font(Cpu&, std::uint32_t slot, std::uint32_t kind);

// The text drawer (block drawer, font_cpp_drawer.cpp). 0x1074f800 paints a string into a window's 1-5-5-5 back buffer, glyph by glyph (the
// selector and the glyph cache are its callees), calling the descriptor at caller sp + 0x38 with each glyph's rectangle when it is not 0.
// A stack-contract leaver (stmw r13) that calls itself for the outline style (flag 0x80). `font_slot` is the callers' Font**, `x` and
// `y` are the pen's whole pixels (low halfwords), `flags` the style (the low halfword is used by the drawing, the selector reads it
// whole), `size` a byte, `colour` a 1-5-5-5 pixel. Returns 1 (0 for a font record that is not loaded).
struct TextRequest {
    std::uint32_t font_slot, window, x, y, flags, size, colour, text;
};
std::uint32_t draw_text(Cpu&, const TextRequest&);
}
