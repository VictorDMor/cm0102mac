#pragma once
// The font engine's records, constants and arithmetic helpers (font_engine.hpp has the routines).
//
// Fixed-point arithmetic. The original's helpers (UTIL, 0x10767ce0..0x10768074) are the only place the engine multiplies
// and divides 16.16 values from halves; the autogridder, the interpreter and the scan converter call them. Every other
// multiply the engine writes inline is a plain mullw or mulhw, wrapping at 32 bits: write those with the helpers below
// (`wrap`, `rlwinm`, `mulhw`) so the width and the sign are explicit, and check each against the original over the edge values
// (font_engine_checks_util.cpp does for the helpers).
#include "font_engine.hpp"
#include <bit>
#include <cstdint>

namespace cm::aot::font_engine {
// TOC slots (`lwz rX,slot(r2)`: the word is a pointer to the unit's literal pool).
namespace toc {
inline constexpr std::int32_t t2kstrm_pool = -0x3c30;  // 0x108a0824: "pos <= t->maxPos", the unit's first assertion
inline constexpr std::int32_t t2ktt_pool = -0x3c20, truetype_pool = -0x3c1c;  // T2KTT.C's pool (file name at +0xd), TRUETYPE.C's (at +0x16)
inline constexpr std::int32_t scale_shift_callback = -0x3c24, scale_divide_callback = -0x3c28, scale_fixed_callback = -0x3c2c, default_state_word = -0x5b3c;
inline constexpr std::int32_t glyph_pool = -0x3c44;    // GLYPH.C's pool: its assertion texts, the file name at +0x16
inline constexpr std::int32_t tsimem_pool = -0x3c18;   // TSIMEM.C's pool: its assertion texts, the file name at +0x10
inline constexpr std::int32_t t2kextra_pool = -0x3c38; // T2KEXTRA.C's pool: the assertion text at +0x80, the file name at +8
}
// The assertion reporter 0x107755c0 (MAIN.CPP) takes the assertion's text, the unit's file name and the line.
inline constexpr std::uint32_t assertion_report = 0x107755c0;
// The byte move (NETWORK.CPP) read_bytes copies with: (destination, source, count).
inline constexpr std::uint32_t byte_move = 0x10739ef0;
// The unit's file name sits 0x11 bytes after the pool's first string in T2KSTRM.C's pool.
inline constexpr std::uint32_t t2kstrm_name_offset = 0x11;

// rlwinm: rotate left by `shift`, mask from bit `mb` to `me` (PowerPC numbering: bit 0 is the most significant).
constexpr std::uint32_t rlwinm(std::uint32_t value, unsigned shift, unsigned mb, unsigned me) {
    const auto rotated = std::rotl(value, int(shift));
    const std::uint32_t from = 0xffffffffu >> mb, to = 0xffffffffu << (31 - me);
    return rotated & (mb <= me ? (from & to) : (from | to));
}
// A value's low halfword, zero-extended (rlwinm rX,rY,0,16,31) and the high halfword of a signed value, as the original
// extracts them (srawi 16 then the mask).
constexpr std::uint32_t low_half(std::uint32_t v) { return v & 0xffffu; }
constexpr std::uint32_t high_half(std::uint32_t v) { return v >> 16; }
// 32-bit wrapping arithmetic: C++ signed overflow is undefined, the guest wraps.
constexpr std::int32_t wrap(std::uint32_t v) { return std::int32_t(v); }
constexpr std::uint32_t negate(std::uint32_t v) { return 0u - v; }

// The stream record (T2KSTRM.C): the data, the read position and the length.
namespace stream {
inline constexpr std::uint32_t base = 0x0, position = 0x4, length = 0x8;
// The rest of the record (T2KTT.C, 0x20 bytes): the offset the stream was made at (the length less it is what remains), a kind byte,
// the memory object and two words the constructors zero.
inline constexpr std::uint32_t origin = 0xc, kind = 0x10, mem = 0x14, reserved_18 = 0x18, reserved_1c = 0x1c, size = 0x20;
// kind: 1 owns its data block (the delete frees data and stream), 2 a view of another stream's data (offset and length), 3 over a
// caller's data (what the game's New_InputStream makes); neither a view nor a caller's data is freed with its stream.
inline constexpr std::uint8_t owns_data = 1, view = 2, over_caller = 3;
}

// ---- Records of TRUETYPE.C and T2KTT.C, named from the readers' stores ("?" = inferred from one reader) ----
// The sfnt (the sfntClass 0x10766ab0 builds), as words: the table directory, the hint font, the tables the readers below build, the
// memory object and the stream the tables are read from.
namespace sfnt {
inline constexpr std::uint32_t directory = 0x00, hint_font = 0x04, ttc_header = 0x08, head = 0x0c, maxp = 0x10, loca = 0x14, hhea = 0x18,
    vhea = 0x1c, hmtx = 0x20, vmtx = 0x24, kern = 0x2c, stream = 0x54, mem = 0x5c;
}
// The fields of the sfnt the glyph block names (TRUETYPE.C, block truetype_glyph): the cmap, the platform and encoding the cmap is chosen by (set
// by the scaler before the first character lookup), post's underline position and thickness, the six words copied from the caller's callback
// table (the first runs on each loaded glyph, the second on the horizontal metrics, both with the data from +0x44), the glyph limit, a word the
// constructor zeroes and 0x1075bc90 passes along, and the size of the record.
namespace sfnt {
inline constexpr std::uint32_t cmap = 0x28, platform_id = 0x30, encoding_id = 0x32, underline_position = 0x34 /* hw */, underline_thickness = 0x36 /* hw */,
    glyph_callback = 0x3c, metrics_callback = 0x40, callback_data = 0x44 /* four words to +0x50 */, extra_60 = 0x60, glyph_limit = 0x64, size = 0x68;
}
// The table directory (sfnt_OffsetTable) and its entries {+0 mem, +4 tag, +8 checksum, +0xc offset, +0x10 length}, 0x14 bytes each.
namespace table_directory {
inline constexpr std::uint32_t mem = 0x00, version = 0x04, count = 0x08 /* s16 */, search_range = 0x0a, entry_selector = 0x0c, range_shift = 0x0e,
    entries = 0x10 /* pointer to an array of entry pointers */;
}
namespace table_entry {
inline constexpr std::uint32_t mem = 0x00, tag = 0x04, checksum = 0x08, offset = 0x0c, length = 0x10, size = 0x14;
}
namespace tag {  // four characters, big-endian
inline constexpr std::uint32_t cvt = 0x63767420, prep = 0x70726570, fpgm = 0x6670676d;
}
// maxp (0x24 bytes): the version word, then fourteen halfwords as the table has them.
namespace maxp {
inline constexpr std::uint32_t mem = 0x00, version = 0x04, num_glyphs = 0x08, max_points = 0x0a, max_contours = 0x0c, max_composite_points = 0x0e,
    max_composite_contours = 0x10, max_zones = 0x12, max_twilight_points = 0x14, max_storage = 0x16, max_function_defs = 0x18,
    max_instruction_defs = 0x1a, max_stack_elements = 0x1c, max_size_of_instructions = 0x1e, max_component_elements = 0x20,
    max_component_depth = 0x22, size = 0x24;
}
// head (0x3c bytes): four words, flags and unitsPerEm, the dates as word pairs, the bounding box and five halfwords.
namespace head {
inline constexpr std::uint32_t mem = 0x00, version = 0x04, font_revision = 0x08, checksum_adjustment = 0x0c, magic = 0x10, flags = 0x14,
    units_per_em = 0x16, created_high = 0x18, created_low = 0x1c, modified_high = 0x20, modified_low = 0x24, x_min = 0x28, y_min = 0x2a,
    x_max = 0x2c, y_max = 0x2e, mac_style = 0x30, lowest_rec_ppem = 0x32, direction_hint = 0x34, index_to_loc_format = 0x36,
    glyph_data_format = 0x38, size = 0x3c;
}
// hhea and vhea (0x28 bytes, one reader): the version word, then sixteen halfwords.
namespace hhea {
inline constexpr std::uint32_t mem = 0x00, version = 0x04, ascender = 0x08, descender = 0x0a, line_gap = 0x0c, advance_max = 0x0e, min_lsb = 0x10,
    min_rsb = 0x12, x_max_extent = 0x14, caret_slope_rise = 0x16, caret_slope_run = 0x18, caret_offset = 0x1a, reserved = 0x1c /* four */,
    metric_data_format = 0x24, number_of_metrics = 0x26, size = 0x28;
}
// loca (0x10 bytes): the offsets as words (a short table's doubled), their count, the table's format (0 short, 1 long).
namespace loca {
inline constexpr std::uint32_t mem = 0x00, offsets = 0x04, count = 0x08, format = 0x0c, size = 0x10;
}
// kern: {mem, version, table count, array of subtable pointers} (0x10 bytes); a subtable {mem, format, length, coverage, pair table}
// (0x14); a pair table {mem, pair count, search range, entry selector, range shift, entries} (0x10), each entry a word (left and right
// glyph) and a halfword value, 8 bytes.
namespace kern {
inline constexpr std::uint32_t mem = 0x00, version = 0x04, table_count = 0x08, tables = 0x0c, size = 0x10;
}
namespace kern_subtable {
inline constexpr std::uint32_t mem = 0x00, format = 0x04, length = 0x08, coverage = 0x0c, pairs = 0x10, size = 0x14;
}
namespace kern_pairs {
inline constexpr std::uint32_t mem = 0x00, count = 0x04, search_range = 0x06, entry_selector = 0x08, range_shift = 0x0a, entries = 0x0c,
    entry_size = 8, size = 0x10;
}
// cmap (0x2c bytes): an array of encoding records {+0 platform, +2 encoding, +4 offset} (8 bytes) over a copy of the table, the index
// of the record chosen and its format (0, 4 or 6), and the glyphs of the ten digits '0'..'9'.
namespace cmap {
inline constexpr std::uint32_t mem = 0x00, version = 0x04, encoding_count = 0x06, encodings = 0x08, data = 0x0c, data_length = 0x10,
    chosen = 0x14, format = 0x16, digit_glyphs = 0x18 /* ten halfwords */, size = 0x2c;
}
namespace cmap_encoding {
inline constexpr std::uint32_t platform = 0x00, encoding = 0x02, offset = 0x04, size = 0x08;
}
// A glyph outline as TRUETYPE.C loads it (0x78 bytes): the contour and point arrays (halfword coordinates, on-curve bytes), a composite's
// component words, the instructions, the bounding box; four phantom points follow the outline's points in the coordinate arrays.
namespace glyph_record {
inline constexpr std::uint32_t mem = 0x00, contour_count = 0x04 /* hw */, point_count = 0x08, inline_ends = 0x14 /* 0x10 bytes, <= 8 contours */,
    shape = 0x34 /* hw, 2 */, contours = 0x36 /* hw as read: negative for a composite */, outline_points = 0x38 /* hw */, contour_starts = 0x3c, contour_ends = 0x40,
    xs = 0x44, ys = 0x48, on_curve = 0x4c, original_xs = 0x50, original_ys = 0x54, components = 0x58, component_words = 0x5c,
    component_capacity = 0x60, instructions = 0x64, instruction_length = 0x68, x_min = 0x6c, y_min = 0x6e, x_max = 0x70, y_max = 0x72,
    size = 0x78;
}
// The hint font (T2KTT.C, 0x14c bytes; sfnt.hint_font): the TrueType interpreter's state for one font and size. The graphic state is
// embedded at +0x18 (`state`; the offsets of graphic_state are from there); the twilight and glyph zones follow at +0xf4.
namespace hint_font {
inline constexpr std::uint32_t mem = 0x00, ppem_x = 0x04, ppem_y = 0x08, cvt_words = 0x0c /* the cvt table's halfwords */, cvt_count = 0x10,
    zone_block = 0x14, state = 0x18, x_scale = 0xd0, y_scale = 0xd4, flag_e0 = 0xe0, flag_e1 = 0xe1, units_per_em = 0xe4 /* hw */,
    twilight_count = 0xe6 /* hw */, prep_length = 0xe8, fpgm_length = 0xec, zones = 0xf4, size = 0x14c;
// The glyph being interpreted (set by 0x10763eb0): the glyph record's original coordinates, the zone's current coordinates, the glyph's
// own arrays, its contours and a byte per point; the graphic state's saved copy of its defaults is at +0x70.
inline constexpr std::uint32_t glyph_original_xs = 0x120, glyph_original_ys = 0x124, zone_xs = 0x128, zone_ys = 0x12c, glyph_xs = 0x130,
    glyph_ys = 0x134, glyph_on_curve = 0x138, glyph_contours = 0x13c /* hw */, glyph_contour_flag = 0x13e /* hw */, glyph_contour_starts = 0x140,
    glyph_contour_ends = 0x144, zone_touched = 0x148, saved_defaults = 0x70, saved_flag_a0 = 0xa0 /* two halfwords */;
}
namespace graphic_state {  // offsets from hint_font::state
inline constexpr std::uint32_t stack = 0x00, storage = 0x04, cvt = 0x08, defaults = 0x24 /* 0x34 bytes, copied to saved */, saved = 0x58,
    scratch = 0x8c, function_defs = 0x90, instruction_defs = 0x94, scale_callback = 0x98, prep = 0x9c, fpgm = 0xa0, round_unit = 0xa4,
    scale_mult = 0xa8, scale_div = 0xac, scale_add = 0xb0, scale_shift = 0xb4 /* hw */, mode = 0xb6 /* byte */, ratio_down = 0xb8,
    ratio_up = 0xbc, size_flag_c8 = 0xc8, size_flag_c9 = 0xc9;
}

// Routines of other blocks that still run generated, called as the original's `bl` does: through the dispatcher, with the arguments
// they read and at the original's return address. (The allocator's C++ lands with block `tsimem`; these stay the calls until a block
// replaces one with a direct call.) Each takes the address after the `bl`.
namespace dispatched {
inline constexpr std::uint32_t setjmp_entry = 0x10773eb0, allocate_entry = 0x107678a0, scratch_entry = 0x107675c0, free_blocks_entry = 0x10767820,
    release_entry = 0x10767bb0, reallocate_entry = 0x10767a00, raise_entry = 0x10767430;
inline std::uint32_t allocate(Cpu& c, std::uint32_t mem, std::uint32_t size, std::uint32_t back) {
    c.set({{3, mem}, {4, size}});
    c.call(allocate_entry, back);
    return c.r(3);
}
// 0x107675c0: the scratch buffer of a slot, grown to `size`.
inline std::uint32_t scratch(Cpu& c, std::uint32_t mem, std::uint32_t size, std::uint32_t slot, std::uint32_t back) {
    c.set({{3, mem}, {4, size}, {5, slot}});
    c.call(scratch_entry, back);
    return c.r(3);
}
inline std::uint32_t reallocate(Cpu& c, std::uint32_t mem, std::uint32_t pointer, std::uint32_t size, std::uint32_t back) {
    c.set({{3, mem}, {4, pointer}, {5, size}});
    c.call(reallocate_entry, back);
    return c.r(3);
}
// 0x10767bb0 (a stack leaver: the caller sets r28-r31 as the original holds them first).
inline void release(Cpu& c, std::uint32_t mem, std::uint32_t pointer, std::uint32_t back) {
    c.set({{3, mem}, {4, pointer}});
    c.call(release_entry, back);
}
inline void free_blocks(Cpu& c, std::uint32_t mem, std::uint32_t back) {
    c.set(3, mem);
    c.call(free_blocks_entry, back);
}
// 0x10767430 (mem, code): the error site; it does not return in the game.
inline void raise_error(Cpu& c, std::uint32_t mem, std::uint32_t code, std::uint32_t back) {
    c.set({{3, mem}, {4, code}});
    c.call(raise_entry, back);
}
// The assertion `bl 0x107755c0` of a unit's pool: the text and the file name at their offsets, and the line.
inline void assertion(Cpu& c, std::uint32_t pool, std::uint32_t text, std::uint32_t file, std::uint32_t line, std::uint32_t back) {
    c.set({{3, pool + text}, {4, pool + file}, {5, line}});
    c.call(assertion_report, back);
}
}

// TSIMEM.C: the allocator over the game's malloc. A tsiMemObject (0x17c bytes) and the blocks it hands out.
namespace tsimem {
// tsiMemObject: stamp, the number of blocks held, the table's capacity and the table of block pointers, the jump buffer the API
// entries fill with setjmp (+0x10..), six scratch buffers (pointer, size, free flag: three arrays of six words), the table cursor,
// the error stamp an error site stores before its longjmp, and the end stamp.
inline constexpr std::uint32_t stamp = 0x0, count = 0x4, capacity = 0x8, table = 0xc, jump_buffer = 0x10, scratch_pointer = 0x128,
                               scratch_size = 0x140, scratch_free = 0x158, cursor = 0x170, error_stamp = 0x174, end_stamp = 0x178, object_size = 0x17c;
inline constexpr std::uint32_t scratch_slots = 6, initial_capacity = 0x1f5;
inline constexpr std::uint32_t object_stamp = 0xab1500ff, end_stamp_value = 0xa5a55a5a, error_stamp_initial = 0xaa005501, error_raised = 0x5500aaff;
// A block is malloc(size + 0x12): header {+0 stamp, +4 size, +8 table index}, the user bytes at +0x10, then the trailer bytes.
inline constexpr std::uint32_t header_size = 0x10, block_stamp_at = 0x0, block_size_at = 0x4, block_index_at = 0x8;
inline constexpr std::uint32_t block_stamp = 0xaa53c5aa, trailer_first = 0x5a, trailer_second = 0xf0;
// The codes an error site gives longjmp (the API entry returns them), and the engine's calls out.
inline constexpr std::uint32_t error_out_of_memory = 0x2718, error_bad_block = 0x2719, error_null_object = 0x271b, error_too_many_pointers = 0x271c,
                               error_bad_count = 0x271d, error_realloc_failed = 0x271e, error_slot_mismatch = 0x271f, error_no_free_slot = 0x2721;
inline constexpr std::uint32_t game_malloc = 0x10775130, game_free = 0x10775170, game_realloc = 0x107751b0, longjmp_entry = 0x10773f30;
inline constexpr std::uint32_t name_offset = 0x10;  // the file name in the unit's pool
}

// STYLE: the bold functions the sfnt layer calls through pointers. The outline record (T2K's glyph: +0x36 contour count, +0x3c start
// points, +0x40 end points, +0x44 x and +0x48 y coordinates, halfword arrays) and the list of advance widths (+4 count, +0x10 halfwords).
namespace style {
inline constexpr std::uint32_t contour_count = 0x36, start_points = 0x3c, end_points = 0x40, x_coordinates = 0x44, y_coordinates = 0x48;
inline constexpr std::uint32_t advance_count = 0x4, advance_values = 0x10;
}

// T2KEXTRA: the scaler record's (0x5f8, the T2K object) render result fields the colour conversion rewrites.
namespace extra {
inline constexpr std::uint32_t mem_at = 0x4, left_at = 0xa8, margin_at = 0xc4, width_at = 0xd0, height_at = 0xd4, stride_at = 0xd8, bitmap_at = 0xdc;
}
}
