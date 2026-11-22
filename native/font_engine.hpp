#pragma once
// The font engine (tools/font_engine_audit.py): the T2K scaler the game links for its text, rewritten as typed C++.
// font_engine_detail.hpp has the record views, constants and fixed-point helpers; this header lists the routines of the
// blocks written so far (the next blocks append theirs). A routine here takes the guest CPU only where it reads or writes
// guest memory or calls the game; the fixed-point helpers are plain functions.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::font_engine {
using guest::Cpu;

// ---- UTIL (0x10767ce0..0x10768074): the 16.16 and 2.30 fixed-point helpers, a sort of shorts and a distance -------------
// What a helper leaves in the volatile registers besides its result (its callers' indirect calls and varargs spills can read
// them, so the checker compares r4-r8 at return; r9 and r10 pass through untouched).
struct Leftovers {
    std::uint32_t r4 = 0, r5 = 0, r6 = 0, r7 = 0, r8 = 0;
};
// 0x10767d40: the 16.16 product (a * b) >> 16 of two signed values, from 16-bit halves of their magnitudes: the low-by-low
// product's high half is added to the cross products and the high-by-high product's low half shifted up. The halves wrap;
// the product is not rounded.
std::int32_t fix_multiply(std::int32_t a, std::int32_t b, Leftovers& left);
// 0x10767dc0: the same shape for a 2.30 fraction (a * b >> 30, the high-by-high product shifted up 26, the low-by-low product
// down 6, the cross products up 10).
std::int32_t frac_multiply(std::int32_t a, std::int32_t b, Leftovers& left);
// 0x10767e40: a 32-bit value times a signed halfword (the callers pass a sign-extended 16-bit scale) as an 18-bit product:
// (high * |scale| << 2) + (low * |scale| >> 14), signed by the operands' signs.
std::int32_t scale_multiply(std::int32_t value, std::int32_t scale, Leftovers& left);
// 0x10767eb0: the 16.16 quotient (a << 16) / b of two signed values: the integer part by unsigned division, the remainder
// shifted down until it fits 16 bits (the divisor with it) and divided again. A zero divisor faults as the generated code does.
std::int32_t fix_divide(std::int32_t a, std::int32_t b, Leftovers& left);
// 0x10767f20: the length of the vector (a, b) by the interpolation of the larger and the smaller component and three rounds
// of the Newton step (x + (a^2 + b^2) / x) / 2 in 16.16, with the helpers above.
std::int32_t distance(std::int32_t a, std::int32_t b, Leftovers& left);
// 0x10767ce0: bubble sort of `count` halfwords in guest memory, ascending (a pass repeats while one swapped).
void sort_shorts(Cpu& c, std::uint32_t array, std::int32_t count, Leftovers& left);

// ---- T2KSTRM (0x10762f40..0x107630e0): the input stream the font tables are read through ----------------------------------
// A stream is {+0 base, +4 position, +8 length}. A read past the length reports the failed assertion (0x107755c0) and goes on.
std::uint32_t read_unsigned_32(Cpu& c, std::uint32_t stream);
std::int32_t read_signed_16(Cpu& c, std::uint32_t stream);
void read_bytes(Cpu& c, std::uint32_t stream, std::uint32_t destination, std::int32_t count);

// ---- T2KTT / start of TRUETYPE (0x107630e0..0x1076460c): the stream constructors, the hint font and the cmap lookups ----
// The stream record's other fields (kind, origin, memory object) are font_engine_detail.hpp's `stream`. The three constructors and the
// delete setjmp on the memory object when given an error pointer: a longjmp's code goes there and the object's blocks are freed.
// 0x107630e0: the bytes from the stream's origin to its end (leaves the origin in r4).
std::int32_t stream_remaining(Cpu& c, std::uint32_t stream);
// 0x107630f0: a view of `length` bytes of the parent's data from `offset` (a block, or with `pooled` the allocator's scratch slot 2); 0 on error.
std::uint32_t new_substream(Cpu& c, std::uint32_t mem, std::uint32_t parent, std::uint32_t offset, std::uint32_t length, std::uint32_t pooled,
                            std::uint32_t error_out);
// 0x107631c0 (the game's New_InputStream): a stream over the caller's data; 0 on error.
std::uint32_t new_input_stream(Cpu& c, std::uint32_t mem, std::uint32_t data, std::uint32_t length, std::uint32_t error_out);
// 0x10763280, 0x10763290: the position to the origin, to the origin plus an offset.
void stream_rewind(Cpu& c, std::uint32_t stream);
void stream_seek(Cpu& c, std::uint32_t stream, std::uint32_t offset);
// 0x107632a0 (the game's delete): the data and the stream freed by kind (the stack leaver 0x10767bb0 does the frees).
void delete_input_stream(Cpu& c, std::uint32_t stream, std::uint32_t error_out);
// 0x10763350: a number of 7-bit groups, least significant first, the high bit of a byte saying more follow.
std::uint32_t read_packed(Cpu& c, std::uint32_t stream);
// 0x10763820, 0x10763840, 0x10763890: the scale callbacks the interpreter calls by pointer (the graphic state and a value).
std::int32_t scale_shifted(Cpu& c, std::uint32_t state, std::int32_t value);
std::int32_t scale_divided(Cpu& c, std::uint32_t state, std::int32_t value);
std::int32_t scale_fixed(Cpu& c, std::uint32_t state, std::int32_t value);
// 0x107643d0: the ratio of two sizes in the record at `out` {+0 multiplier, +2 shift, +4 divisor, +8 half, +0xc 16.16 quotient, +0x10 kind
// (0 power of two, 1 other, 2 out of range)}; the first four are left alone (the caller reads what the stack held) when out of range.
void scale_ratio(Cpu& c, std::uint32_t size_a, std::uint32_t size_b, std::uint32_t out);
// 0x10763390: the hint font of an sfnt (the TrueType interpreter's state: stack, storage, cvt, definitions, zones, the prep and fpgm programs;
// the fpgm is run at once); 0x107638c0 sets its size (scales the cvt, runs the prep); 0x10763eb0 loads a glyph's points and runs its
// instructions (not run by the game); 0x107642e0 releases it.
std::uint32_t new_hint_font(Cpu& c, std::uint32_t mem, std::uint32_t input, std::uint32_t font);
void set_hint_size(Cpu& c, std::uint32_t hint, std::int32_t x, std::int32_t y);
void load_glyph_program(Cpu& c, std::uint32_t hint, std::uint32_t glyph);
void delete_hint_font(Cpu& c, std::uint32_t hint);
// 0x107644c0, 0x107645a0: a character's glyph through the chosen cmap record's format 4 and format 6 table (0 when unmapped).
std::uint32_t cmap4_lookup(Cpu& c, std::uint32_t cmap, std::uint32_t code);
std::uint32_t cmap6_lookup(Cpu& c, std::uint32_t cmap, std::uint32_t code);

// ---- TRUETYPE tables (0x10764610..0x10765810): the readers of an sfnt's tables over a stream at the table, and the directory lookups ----
// Each reads big-endian values through the stream readers into a record of the memory object (font_engine_detail.hpp names the records).
// 0x10764610: the cmap over the table's stream; the encoding record chosen by (platform, encoding) with format 0, 4 or 6 (0xffff matches
// any), else by format 0, 6, 4; the glyphs of '0'..'9'. A format of another kind raises the error 0x2725.
std::uint32_t new_cmap(Cpu& c, std::uint32_t mem, std::uint32_t platform, std::uint32_t encoding, std::uint32_t input);
// 0x107649a0, 0x10764b10: a kern subtable (`apple` for the 32-bit header) and the kern table.
std::uint32_t new_kern_subtable(Cpu& c, std::uint32_t mem, std::uint32_t apple, std::uint32_t input);
std::uint32_t new_kern(Cpu& c, std::uint32_t mem, std::uint32_t input);
// 0x10764bf0 (hhea and vhea), 0x10764d10, 0x10764e50, 0x10764f60: the metric header, head, maxp and loca (the table's format and length).
std::uint32_t new_hhea(Cpu& c, std::uint32_t mem, std::uint32_t input);
std::uint32_t new_head(Cpu& c, std::uint32_t mem, std::uint32_t input);
std::uint32_t new_maxp(Cpu& c, std::uint32_t mem, std::uint32_t input);
std::uint32_t new_loca(Cpu& c, std::uint32_t mem, std::uint32_t input, std::uint32_t format, std::uint32_t length);
// 0x10765060: a glyph from glyf (the allocator's scratch slot 0 holds the record): contours, flags with repeats, deltas, a composite's
// component words, the instructions (kept when `keep_instructions`), the bounding box and the four phantom points from the metrics.
std::uint32_t new_glyph(Cpu& c, std::uint32_t mem, std::uint32_t input, std::uint32_t keep_instructions, std::uint32_t left_bearing, std::uint32_t advance,
                        std::uint32_t top, std::uint32_t bottom);
// 0x10765730: the directory entry with a tag (0 when none); 0x10765780: a stream over that table of the sfnt's own stream.
std::uint32_t find_table_entry(Cpu& c, std::uint32_t font, std::uint32_t tag_word);
std::uint32_t new_table_stream(Cpu& c, std::uint32_t font, std::uint32_t tag_word);

// ---- TSIMEM (0x10767460..0x10767ce0): the allocator over the game's malloc; every routine leaves r3 (and r4-r7) as the original ----
// An error site stores the stamp 0x5500aaff at mem + 0x174 and calls longjmp(mem + 0x10, code) (0x10773f30); the game never returns
// from it (the API entry's setjmp resumes), so the code after a site runs only where a checker answers the longjmp.
// 0x10767460 tsi_NewMemhandler(error): the object and its table; *error = 0x2718 and null when a malloc fails.
std::uint32_t new_memory_handler(Cpu& c, std::uint32_t error_out);
// 0x107675c0: the scratch buffer of slot 0..5 (the assertion fires for others), grown to `size`.
std::uint32_t scratch_allocate(Cpu& c, std::uint32_t mem, std::uint32_t size, std::int32_t slot);
// 0x10767690: the six scratch buffers released and their slots reset.
void scratch_release_all(Cpu& c, std::uint32_t mem);
// 0x10767720 tsi_DeleteMemhandler(mem): stamps checked, scratch released, the table and the object freed.
void delete_memory_handler(Cpu& c, std::uint32_t mem);
// 0x10767820: every block the table holds, the table and the object freed (an error's unwinding).
void free_blocks(Cpu& c, std::uint32_t mem);
// 0x107678a0 tsi_AllocMem(mem, size): a block with header and trailer in the first free table slot; the user pointer (+0x10).
std::uint32_t allocate_block(Cpu& c, std::uint32_t mem, std::uint32_t size);
// 0x10767a00 tsi_ReAllocMem(mem, pointer, size) (not run by the game).
std::uint32_t reallocate_block(Cpu& c, std::uint32_t mem, std::uint32_t pointer, std::uint32_t size);
// 0x10767bb0 tsi_DeAllocMem(mem, pointer): the block verified, its slot cleared and the block freed. The stack contract: its frame
// (saved r28-r31, link register, back chain) is what the game's printf reads as leftovers.
void release_block(Cpu& c, std::uint32_t mem, std::uint32_t pointer);

// ---- STYLE (0x1075e0c0..0x1075e960): the bold style the sfnt layer calls through two function pointers ----------------------------
// (record, memory object, size, pointer to the 16.16 strength); each leaves r9 and r10 as the original (the callers read them).
// 0x1075e1a0: every advance width of the list grows by twice the strength.
void embolden_advances(Cpu& c, std::uint32_t advances, std::uint32_t size, std::uint32_t factor_pointer);
// 0x1075e320: the glyph outline offset outwards: a copy of the points through tsi_AllocMem, three passes, the copy released.
void embolden_outline(Cpu& c, std::uint32_t glyph, std::uint32_t mem, std::uint32_t size, std::uint32_t factor_pointer);
// 0x1075e0c0: the corner of two offset edges, written to two halfword addresses (the callee's parameter words at sp + 0x38/0x3c).
void offset_corner(Cpu& c, std::uint32_t x1, std::uint32_t y1, std::uint32_t x2, std::uint32_t y2, std::uint32_t x3, std::uint32_t y3,
                   std::uint32_t x4, std::uint32_t y4, std::uint32_t out_x, std::uint32_t out_y);

// ---- GLYPH (0x1075b970..0x1075c97c): the glyph record (glyph_record in font_engine_detail.hpp) the outlines are built in ----
// 0x1075b970: one point of the compact glyph format from the stream, written as halfwords through the two pointers; returns the on-curve flag.
std::uint32_t read_compact_point(Cpu& c, std::uint32_t stream, std::uint32_t x_out, std::uint32_t y_out);
// 0x1075c830 (a stack-contract leaver): the point arrays for `count` points and the four phantom points.
void allocate_glyph_points(Cpu& c, std::uint32_t g, std::int32_t count);
// 0x1075bba0: a glyph with no outline points, only the four phantom points.
std::uint32_t new_empty_glyph(Cpu& c, std::uint32_t mem, std::uint32_t advance_width, std::uint32_t top_bearing, std::uint32_t advance_height);
// 0x1075bc90: a glyph read from the stream (simple or composite), its phantom points placed from the four metrics.
std::uint32_t new_compact_glyph(Cpu& c, std::uint32_t mem, std::uint32_t stream, std::uint32_t left_bearing, std::uint32_t advance_width,
                        std::uint32_t top_bearing, std::uint32_t advance_height);
// 0x1075c0f0: a component glyph added to the glyph `*slot` (which becomes it when empty), at the offset its flags give.
void append_glyph(Cpu& c, std::uint32_t slot, std::uint32_t source, std::uint32_t flags, std::uint32_t dx, std::uint32_t dy);
// 0x1075c8c0: a glyph's blocks and the record released.
void delete_glyph(Cpu& c, std::uint32_t g);

// ---- TRUETYPE.C (0x10765810..0x10767460): the sfnt, its creation and deletion, the glyph by index (block truetype_glyph) ----
// 0x10766430: the number of the last glyph (maxp's numGlyphs, or loca's count less one when it is not above it).
std::uint32_t glyph_limit(Cpu& c, std::uint32_t font);
// 0x10766cc0: units per em from head (0x800 without a head).
std::uint32_t units_per_em(Cpu& c, std::uint32_t font);
// 0x10766d30: the larger of maxp's maxPoints and maxCompositePoints.
std::uint32_t max_outline_points(Cpu& c, std::uint32_t font);
// 0x10767430: the allocator's error site as a routine: the stamp at mem + 0x174, then longjmp(mem + 0x10, code).
void raise_error(Cpu& c, std::uint32_t mem, std::uint32_t code);
// 0x10766770: the glyph of a character code through the cmap (read when it is not yet); 0x107668d0: whether a code is one of its ten digits.
std::uint32_t character_glyph(Cpu& c, std::uint32_t font, std::uint32_t code);
std::uint32_t is_digit_glyph(Cpu& c, std::uint32_t font, std::uint32_t code);
// 0x10766dc0: the caret slopes of hhea and vhea for the scaler (a leaf); 0x107670e0: the cmap released.
void caret_slopes(Cpu& c, std::uint32_t font, std::uint32_t horizontal, std::uint32_t vertical);
void delete_cmap(Cpu& c, std::uint32_t font);
// 0x107664a0: the glyph of an index (T2K compact or TrueType data, or an empty one), its advance and vertical advance through the two pointers.
std::uint32_t load_glyph(Cpu& c, std::uint32_t font, std::uint32_t index, std::uint32_t keep_instructions, std::uint32_t advance_out,
                         std::uint32_t vertical_out);
// 0x10765810: the sfnt read from the stream (the old tables released first); 0x10766ab0 / 0x10767190: the API entries that create and delete
// an sfnt (setjmp on the memory object, the error code through the pointer).
void read_sfnt(Cpu& c, std::uint32_t font, std::uint32_t stream, std::uint32_t index);
std::uint32_t new_sfnt(Cpu& c, std::uint32_t mem, std::uint32_t format, std::uint32_t index, std::uint32_t stream, std::uint32_t callbacks,
                       std::uint32_t error_out);
void delete_sfnt(Cpu& c, std::uint32_t font, std::uint32_t error_out);

// ---- T2KEXTRA (0x10761070): the colour table rendering for command bit 0x80, which the game never sets ----------------------------
// The 8-bit bitmap of a rendered glyph (the T2K object) widened by four with a five-tap filter; the old bitmap released (leaves nothing in r3-r10).
void render_color_table(Cpu& c, std::uint32_t t2k);
}
