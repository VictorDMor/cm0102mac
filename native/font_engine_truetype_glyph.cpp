// The font engine, block `truetype_glyph` (TRUETYPE.C, 0x10765810..0x10767460): the sfnt object (a TrueType font's tables over a stream),
// its creation and deletion (the API entries FONT.CPP calls), the lookup of a character in the cmap, the loading of a glyph by index
// and the metrics the scaler asks for.
//
// The sfnt (sfnt:: in font_engine_detail.hpp, 0x68 bytes from tsi_AllocMem, not zero-filled) keeps the table directory, the tables the
// readers of block `truetype_tables` build, the metrics' tables, the stream and the memory object. Every table is read through a view of
// the stream made at the table's offset (0x107630f0) and deleted again (0x107632a0).
//
// Registers and the stack. A routine builds the original's frame; every call to the allocator's release (0x10767bb0, a stack leaver)
// is made with r23-r31 as the original holds them, because its prologue saves r28-r31 into the dead stack where the game's printf reads
// them. The other callees (the readers of block `truetype_tables`, the stream's views, the allocator) are called through the dispatcher
// at the original's return addresses, with the arguments they read.
//
// Errors. The two API entries run setjmp on the memory object's jump buffer; an error site below (the allocator's) longjmps, and the
// transfer lands back here: the entry catches it at the setjmp's return (pc and sp as the setjmp left them), stores the code through
// the caller's pointer and frees what the memory object holds, as the original's code after the setjmp does.
#include "font_engine_detail.hpp"
#include <array>

namespace cm::aot::font_engine {
namespace {
using guest::Frame;
constexpr std::uint32_t sext16(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }
// Callees outside the block (still generated, or C++ reached through the dispatcher).
constexpr std::uint32_t open_view = 0x107630f0, close_view = 0x107632a0, rewind_stream = 0x10763280, seek_stream = 0x10763290,
                        make_hint_font = 0x10763390, hint_font_delete_entry = 0x107642e0, make_cmap = 0x10764610, make_kern = 0x10764b10,
                        make_hhea = 0x10764bf0, make_head = 0x10764d10, make_maxp = 0x10764e50, make_loca = 0x10764f60,
                        glyf_glyph = 0x10765060, cmap6 = 0x107645a0, cmap4 = 0x107644c0, indirect_glue = 0x10781320,
                        setjmp_entry = 0x10773eb0;
// Tags (four characters, big-endian).
constexpr std::uint32_t tag_ttcf = 0x74746366, tag_glyf = 0x676c7966, tag_head = 0x68656164, tag_hhea = 0x68686561, tag_vhea = 0x76686561,
                        tag_maxp = 0x6d617870, tag_loca = 0x6c6f6361, tag_hmtx = 0x686d7478, tag_vmtx = 0x766d7478, tag_kern = 0x6b65726e,
                        tag_fvar = 0x66766172, tag_post = 0x706f7374, tag_cmap = 0x636d6170;
// The pool's strings are at these offsets of TRUETYPE.C's pool; the file name at 0x16.
constexpr std::uint32_t file_name = 0x16;

// What the generated stream readers (0x10762f40, 0x10762fd0) store in their caller's frame before they run: r29-r31 below the stack pointer, the
// link register and the back chain. The C++ readers build a frame only to report an assertion, so a routine that calls one directly writes
// these words itself (the dead stack is then byte for byte the original's); the stack pointer stays the caller's.
void reader_prologue(Cpu& c) {
    const auto sp = c.sp();
    c.put32(sp - 4, c.r(31));
    c.put32(sp - 8, c.r(30));
    c.put32(sp - 0xc, c.r(29));
    c.put32(sp + 8, c.lr());
    c.put32(sp - 0x50, sp);
}
std::uint32_t pool_of(Cpu& c) { return c.toc(toc::truetype_pool); }
// The first entry of the sfnt's table directory with `tag`, or 0 (the loop of every table search: the count is read as a signed
// halfword each time).
std::uint32_t find_entry(Cpu& c, std::uint32_t font, std::uint32_t tag) {
    for (std::int32_t n = 0;; ++n) {
        const auto directory = c.u32(font + sfnt::directory);
        if (!(n < c.i16(directory + table_directory::count))) return 0;
        const auto entry = c.u32(c.u32(directory + table_directory::entries) + 4 * std::uint32_t(n));
        if (c.u32(entry + table_entry::tag) == tag) return entry;
    }
}
void report(Cpu& c, std::uint32_t pool, std::uint32_t text, std::uint32_t line, std::uint32_t back) {
    dispatched::assertion(c, pool, text, file_name, line, back);
}
}

// 0x10766430: the number of the last glyph the font has: maxp's numGlyphs, or loca's entry count less one when it is not above it.
std::uint32_t glyph_limit(Cpu& c, std::uint32_t font) {
    Frame frame(c, 0x10766430, 0x40, {.first_gpr = 31});
    const auto pool = pool_of(c);
    c.set({{31, font}, {4, pool}});
    if (c.u32(font + sfnt::maxp) == 0) report(c, pool, 0xf1, 0x892, 0x10766464);
    std::uint32_t count = c.u16(c.u32(font + sfnt::maxp) + maxp::num_glyphs);
    const auto loca = c.u32(font + sfnt::loca);
    c.set(4, loca);
    if (loca != 0) {
        const auto entries = c.u32(loca + loca::count);
        c.set(4, entries);
        if (!(std::int32_t(entries) > std::int32_t(count))) count = entries - 1;
    }
    c.set(3, count);
    return count;
}

// 0x10766cc0: units per em, 0x800 without a head table.
std::uint32_t units_per_em(Cpu& c, std::uint32_t font) {
    Frame frame(c, 0x10766cc0, 0x40, {.first_gpr = 30});
    const auto pool = pool_of(c);
    c.set({{31, 0x800}, {30, font}, {4, pool}});
    if (font == 0) report(c, pool, 0x21, 0xc39, 0x10766cf4);
    const auto head = c.u32(font + sfnt::head);
    c.set(3, head);
    std::uint32_t result = 0x800;
    if (head != 0) result = c.u16(head + head::units_per_em);
    c.set(3, result);
    return result;
}

// 0x10766d30: the larger of maxp's maxPoints and maxCompositePoints (a halfword).
std::uint32_t max_outline_points(Cpu& c, std::uint32_t font) {
    Frame frame(c, 0x10766d30, 0x40, {.first_gpr = 30});
    const auto pool = pool_of(c);
    c.set({{31, pool}, {30, font}});
    if (font == 0) report(c, pool, 0x21, 0xc53, 0x10766d60);
    if (c.u32(font + sfnt::maxp) == 0) report(c, pool, 0xf1, 0xc63, 0x10766d7c);
    const auto maxp_record = c.u32(font + sfnt::maxp);
    const auto first = c.i16(maxp_record + maxp::max_points), second = c.i16(maxp_record + maxp::max_composite_points);
    c.set(3, second);
    const auto result = sext16(std::uint32_t(first > second ? first : second));
    c.set(3, result);
    return result;
}

// 0x10767430: the allocator's error site, as a routine: the stamp 0x5500aaff at mem + 0x174, then longjmp(mem + 0x10, code). In the game
// it does not return.
void raise_error(Cpu& c, std::uint32_t mem, std::uint32_t code) {
    Frame frame(c, 0x10767430, 0x40, {});
    c.set(0, tsimem::error_raised);
    c.put32(mem + tsimem::error_stamp, tsimem::error_raised);
    c.set({{3, mem + tsimem::jump_buffer}, {4, code}, {5, 0x55010000}});
    c.call(0x10773f30, 0x10767450);
}

namespace {
// The cmap of the font, read on first use from its table: the table's view, the reader of block `truetype_tables` called with the
// platform and encoding the sfnt was given, the view deleted. The three return addresses differ at each of the two callers, and so does the
// register the original keeps the view in (r31 for the character lookup, r30 for the digits, whose r31 is the pool).
void load_cmap(Cpu& c, std::uint32_t font, unsigned view_register, std::uint32_t opened, std::uint32_t made, std::uint32_t closed) {
    const auto entry = find_entry(c, font, tag_cmap);
    const auto mem = c.u32(font + sfnt::mem);
    c.set({{3, mem}, {4, c.u32(font + sfnt::stream)}, {5, c.u32(entry + table_entry::offset)}, {6, c.u32(entry + table_entry::length)}, {7, 0}, {8, 0}});
    c.call(open_view, opened);
    const auto view = c.r(3);
    c.set(view_register, view);
    c.set({{3, c.u32(font + sfnt::mem)}, {4, c.u16(font + sfnt::platform_id)}, {5, c.u16(font + sfnt::encoding_id)}, {6, view}});
    c.call(make_cmap, made);
    c.put32(font + sfnt::cmap, c.r(3));
    c.set({{3, view}, {4, 0}});
    c.call(close_view, closed);
}
}

// 0x10766770: the glyph of a character code: the cmap is read when it is not yet, then looked up by its format (0: one byte codes
// in a 256 entry table after the record's six bytes; 6 and 4 by the readers of block `t2ktt_head`). 0 for a code the cmap lacks or
// a format it does not know.
std::uint32_t character_glyph(Cpu& c, std::uint32_t font, std::uint32_t code) {
    Frame frame(c, 0x10766770, 0x50, {.first_gpr = 29});
    c.set({{29, font}, {30, code}});
    if (c.u32(font + sfnt::cmap) == 0) load_cmap(c, font, 31, 0x10766808, 0x10766820, 0x10766830);
    const auto cmap = c.u32(font + sfnt::cmap);
    std::uint32_t result = 0;
    const auto format = c.u16(cmap + cmap::format);
    c.set({{3, cmap}, {4, format}});
    if (format == 0) {
        const auto index = code & 0xffff;
        c.set(6, index);
        if (index < 0x100) {
            const auto record = c.u32(c.u32(cmap + cmap::encodings) + (sext16(c.u16(cmap + cmap::chosen)) << 2));
            result = c.u8(c.u32(cmap + cmap::data) + c.u32(record + cmap_encoding::offset) + 6 + index);
        }
    } else if (format == 6) {
        c.set({{3, cmap}, {4, code & 0xffff}});
        c.call(cmap6, 0x10766888);
        result = c.r(3);
    } else if (format == 4) {
        c.set({{3, cmap}, {4, code & 0xffff}});
        c.call(cmap4, 0x107668a0);
        result = c.r(3);
    }
    c.set(3, result);
    return result;
}

// 0x107668d0: whether a character code is the glyph of one of the cmap's ten digits (1, else 0).
std::uint32_t is_digit_glyph(Cpu& c, std::uint32_t font, std::uint32_t code) {
    Frame frame(c, 0x107668d0, 0x50, {.first_gpr = 28});
    const auto pool = pool_of(c);
    c.set({{31, pool}, {29, code}, {28, font}});
    if (c.u32(font + sfnt::cmap) == 0) load_cmap(c, font, 30, 0x10766970, 0x10766988, 0x10766998);
    const auto cmap = c.u32(font + sfnt::cmap);
    c.set(30, cmap);
    if (cmap == 0) report(c, pool, 0x21, 0x59, 0x107669b4);
    std::uint32_t result = 0;
    for (unsigned n = 0; n < 10; ++n)
        if (c.u16(cmap + cmap::digit_glyphs + 2 * n) == (code & 0xffff)) { result = 1; break; }
    c.set(3, result);
    return result;
}

// 0x10766dc0: the slopes of the caret (hhea, into `horizontal`) and of its vertical twin (vhea, into `vertical`) for the scaler: each
// record is {+0 present, +4 ascender, +6 descender, +8 line gap, +0xa advance max, +0xc caret run, +0x10 caret rise, +0x14 and +0x16
// the font's underline position and thickness (the horizontal one only)}; run and rise are doubled together while both stay inside
// (-0x10000, 0x10000), at most sixteen times. (A leaf; its callers' callers read r9, written below.)
void caret_slopes(Cpu& c, std::uint32_t font, std::uint32_t horizontal, std::uint32_t vertical) {
    c.put32(vertical, 0);
    c.put32(horizontal, 0);
    c.put16(horizontal + 0x14, 0);
    c.put16(horizontal + 0x16, 0);
    c.put16(vertical + 0x14, 0);
    c.put16(vertical + 0x16, 0);
    c.put16(horizontal + 0x14, c.u16(font + sfnt::underline_position));
    c.put16(horizontal + 0x16, c.u16(font + sfnt::underline_thickness));
    // r9 (read by the callers' callers, mask 0x60): the horizontal record's loop sets it to 0 and adds 3 for each completed pass of four doublings,
    // when the font has an hhea; r10 is not written.
    const auto fill = [&](std::uint32_t record, std::uint32_t table) {
        const bool is_horizontal = record == horizontal;
        std::uint32_t steps = 0;
        if (is_horizontal) c.set(9, 0);
        const auto settle = [&] { if (is_horizontal) c.set(9, 3 * (steps / 4)); };
        c.put32(record, 1);
        c.put16(record + 4, c.u16(table + hhea::ascender));
        c.put16(record + 6, c.u16(table + hhea::descender));
        c.put16(record + 8, c.u16(table + hhea::line_gap));
        c.put16(record + 0xa, c.u16(table + hhea::advance_max));
        c.put32(record + 0xc, std::uint32_t(c.i16(table + hhea::caret_slope_run)));
        c.put32(record + 0x10, std::uint32_t(c.i16(table + hhea::caret_slope_rise)));
        for (unsigned n = 0; n < 16; ++n) {
            const auto run = c.i32(record + 0xc);
            if (run >= 0x10000 || run <= -0x10000) return settle();
            const auto rise = c.i32(record + 0x10);
            if (rise >= 0x10000 || rise <= -0x10000) return settle();
            c.put32(record + 0xc, std::uint32_t(run) << 1);
            c.put32(record + 0x10, std::uint32_t(rise) << 1);
            ++steps;
        }
        settle();
    };
    if (c.u32(font + sfnt::hhea) != 0) fill(horizontal, c.u32(font + sfnt::hhea));
    if (c.u32(font + sfnt::vhea) != 0) fill(vertical, c.u32(font + sfnt::vhea));
}

// 0x107670e0: the cmap and its blocks released (the encoding records, their array, the table's copy, the cmap), the sfnt's slot cleared.
void delete_cmap(Cpu& c, std::uint32_t font) {
    Frame frame(c, 0x107670e0, 0x50, {.first_gpr = 28});
    const auto cmap = c.u32(font + sfnt::cmap);
    std::uint32_t r28 = font, r29 = c.r(29), r30 = cmap, r31 = c.r(31);
    const auto release = [&](std::uint32_t mem, std::uint32_t block, std::uint32_t back) {
        c.set({{28, r28}, {29, r29}, {30, r30}, {31, r31}});
        dispatched::release(c, mem, block, back);
    };
    if (cmap != 0) {
        r31 = 0;
        r29 = 0;
        while (std::int32_t(r31) < c.i16(cmap + cmap::encoding_count)) {
            release(c.u32(cmap + cmap::mem), c.u32(c.u32(cmap + cmap::encodings) + r29), 0x10767128);
            r29 += 4;
            ++r31;
        }
        release(c.u32(cmap + cmap::mem), c.u32(cmap + cmap::encodings), 0x10767148);
        release(c.u32(cmap + cmap::mem), c.u32(cmap + cmap::data), 0x10767154);
        release(c.u32(cmap + cmap::mem), cmap, 0x10767160);
    }
    c.put32(font + sfnt::cmap, 0);
}

// 0x107664a0: the glyph of an index. The glyf table, loca and a metrics table must be there, and the index within the glyph limit, else
// the glyph is an empty one without metrics. The advance and bearing come from hmtx (0x800 and 0 without), the vertical pair from vmtx or
// from hhea (ascender - descender + line gap and a tenth of it). A glyph with data is read through a view of the stream over its
// bytes: by 0x1075bc90 when head's glyph data format is 2000 (T2K's compact glyphs, the format the game's .t2k files carry), by block
// `truetype_tables` for the TrueType format (0; any other value reports the assertion); the view is deleted, and the font's glyph callback,
// when it has one, runs on the glyph (glyph, memory object, units per em, the callback data at sfnt + 0x44). The advance width and
// the vertical advance are written through the two pointers, the index into the record's +0x76.
std::uint32_t load_glyph(Cpu& c, std::uint32_t font, std::uint32_t index, std::uint32_t keep_instructions, std::uint32_t advance_out,
                         std::uint32_t vertical_out) {
    Frame frame(c, 0x107664a0, 0x70, {.first_gpr = 21});
    const auto pool = pool_of(c);
    std::uint32_t r21 = c.r(21), r22 = font, r23 = keep_instructions, r24 = advance_out, r25 = vertical_out, r26 = c.r(26), r27 = c.r(27),
                  r28 = c.r(28), r29 = c.r(29), r30 = pool, r31 = index;
    const auto hold = [&] {
        c.set({{21, r21}, {22, r22}, {23, r23}, {24, r24}, {25, r25}, {26, r26}, {27, r27}, {28, r28}, {29, r29}, {30, r30}, {31, r31}});
    };
    c.put16(advance_out, 0);
    c.put16(vertical_out, 0);
    const auto empty = [&] {
        c.set({{3, c.u32(font + sfnt::mem)}, {4, 0}, {5, 0}, {6, 0}, {7, 0}});
        hold();
        c.link(0x10766748);
        r23 = new_empty_glyph(c, c.r(3), c.r(5), c.r(6), c.r(7));
    };
    const auto glyf = find_entry(c, font, tag_glyf);
    const auto loca = c.u32(font + sfnt::loca);
    const auto hmtx = c.u32(font + sfnt::hmtx);
    if (glyf == 0 || loca == 0 || (hmtx == 0 && c.u32(font + sfnt::vmtx) == 0) || std::int32_t(index) < 0 ||
        std::int32_t(index) >= c.i32(font + sfnt::glyph_limit)) {
        empty();
    } else {
        const auto offsets = c.u32(loca + loca::offsets);
        const auto start = c.u32(offsets + (index << 2)), next = c.u32(offsets + (index << 2) + 4);
        const auto length = next - start;
        // r9 is left as the original's metric lookups leave it (index * 2 after hmtx's, the line gap after hhea's): the callers read it.
        if (hmtx != 0) {
            c.set(9, index << 1);
            r29 = c.u16(c.u32(hmtx + 0x10) + (index << 1));
            r27 = sext16(c.u16(c.u32(hmtx + 0xc) + (index << 1)));
        } else {
            r29 = 0x800;
            r27 = 0;
        }
        const auto vmtx = c.u32(font + sfnt::vmtx);
        r28 = 0;
        r26 = 0;
        if (vmtx != 0) {
            r28 = c.u16(c.u32(vmtx + 0x10) + (index << 1));
            r26 = sext16(c.u16(c.u32(vmtx + 0xc) + (index << 1)));
        } else if (const auto hhea = c.u32(font + sfnt::hhea); c.set(9, hhea), hhea != 0) {
            c.set(9, sext16(c.u16(hhea + hhea::line_gap)));
            r28 = (sext16(c.u16(hhea + hhea::ascender)) - sext16(c.u16(hhea + hhea::descender)) + sext16(c.u16(hhea + hhea::line_gap))) & 0xffff;
            const auto product = std::int32_t(std::int64_t(std::int32_t(0x66666667)) * std::int32_t(r28) >> 32) >> 2;
            r26 = sext16(std::uint32_t(product + std::int32_t(std::uint32_t(product) >> 31)));
        }
        if (next <= start) {
            // A glyph without data: an empty one with the metrics.
            c.set({{3, c.u32(font + sfnt::mem)}, {4, r27}, {5, r29}, {6, r26}, {7, r28}});
            hold();
            c.link(0x10766720);
            r23 = new_empty_glyph(c, c.r(3), c.r(5), c.r(6), c.r(7));
            c.put16(advance_out, r29);
            c.put16(vertical_out, r28);
        } else {
            c.set({{3, c.u32(font + sfnt::mem)}, {4, c.u32(font + sfnt::stream)}, {5, c.u32(glyf + table_entry::offset) + start}, {6, length}, {7, 2}, {8, 0}});
            hold();
            c.call(open_view, 0x10766634);
            r21 = c.r(3);
            const auto format = c.i16(c.u32(font + sfnt::head) + head::glyph_data_format);
            if (format == 0x7d0) {
                c.set({{3, c.u32(font + sfnt::mem)}, {4, r21}, {5, r23}, {6, r27}, {7, r29}, {8, r26}, {9, r28}, {10, c.u32(font + sfnt::extra_60)}});
                hold();
                c.link(0x1076666c);
                r23 = new_compact_glyph(c, c.r(3), c.r(4), c.r(6), c.r(7), c.r(8), c.r(9));
            } else {
                if (format != 0) { hold(); report(c, pool, 0x101, 0x8f5, 0x1076668c); }
                c.set({{3, c.u32(font + sfnt::mem)}, {4, r21}, {5, r23}, {6, r27}, {7, r29}, {8, r26}, {9, r28}});
                hold();
                c.call(glyf_glyph, 0x107666ac);
                r23 = c.r(3);
            }
            c.set({{3, r21}, {4, 0}});
            hold();
            c.call(close_view, 0x107666bc);
            if (r23 == 0) { hold(); report(c, pool, 0x11f, 0x8fa, 0x107666d4); }
            if (c.u32(font + sfnt::glyph_callback) != 0) {
                c.set(3, font);
                hold();
                c.link(0x107666e8);
                const auto units = units_per_em(c, font);
                c.set({{4, c.u32(font + sfnt::mem)}, {5, sext16(units)}, {12, c.u32(font + sfnt::glyph_callback)}, {3, r23}, {6, font + sfnt::callback_data}});
                hold();
                c.call(indirect_glue, 0x10766700);
                c.set(2, c.u32(c.sp() + 0x14));
            }
            c.put16(advance_out, r29);
            c.put16(vertical_out, r28);
        }
    }
    c.put16(r23 + 0x76, r31);
    c.set(3, r23);
    return r23;
}

// 0x10765810: the sfnt filled from the stream. What the sfnt holds is released first (a second call re-reads the font), then: a
// 'ttcf' collection header (version, count, the offsets of the fonts) when the stream starts with one, the offset table the font index
// selects, the table directory (version, four halfwords, then per table four words), and the tables each through a view of the
// stream: head, hhea, vhea, maxp, loca (by head's index format), hmtx and vmtx (the advance and bearing of each glyph, the last
// advance repeated up to the glyph limit; the metrics callback runs on hmtx), kern (unless the font has an fvar table), the hint
// font, and post's underline position and thickness. A table the font lacks leaves its slot empty (maxp must be there).
void read_sfnt(Cpu& c, std::uint32_t font, std::uint32_t stream, std::uint32_t index) {
    Frame frame(c, 0x10765810, 0x60, {.first_gpr = 23});
    const auto pool = pool_of(c);
    std::uint32_t r23 = c.r(23), r24 = c.r(24), r25 = c.r(25), r26 = c.r(26), r27 = c.r(27), r28 = index, r29 = pool, r30 = stream, r31 = font;
    const auto hold = [&] { c.set({{23, r23}, {24, r24}, {25, r25}, {26, r26}, {27, r27}, {28, r28}, {29, r29}, {30, r30}, {31, r31}}); };
    const auto release = [&](std::uint32_t mem, std::uint32_t block, std::uint32_t back) {
        hold();
        dispatched::release(c, mem, block, back);
    };
    // A word or a halfword of a stream (the pilot readers, called directly), and the allocator's plain allocation.
    const auto read_word = [&](std::uint32_t source, std::uint32_t back) {
        c.set(3, source);
        hold();
        c.link(back);
        reader_prologue(c);
        const auto value = read_unsigned_32(c, source);
        c.set(3, value);
        return value;
    };
    const auto read_half = [&](std::uint32_t source, std::uint32_t back) {
        c.set(3, source);
        hold();
        c.link(back);
        reader_prologue(c);
        const auto value = std::uint32_t(read_signed_16(c, source));
        c.set(3, value);
        return value;
    };
    const auto allocate = [&](std::uint32_t mem, std::uint32_t size, std::uint32_t back) {
        hold();
        return dispatched::allocate(c, mem, size, back);
    };
    // A view of the stream over a table (the call every table reader begins with), the reader that builds the record, the view's deletion.
    const auto open = [&](std::uint32_t entry, std::uint32_t back) {
        c.set({{3, c.u32(font + sfnt::mem)}, {4, stream}, {5, c.u32(entry + table_entry::offset)}, {6, c.u32(entry + table_entry::length)}, {7, 0}, {8, 0}});
        hold();
        c.call(open_view, back);
        return c.r(3);
    };
    const auto close = [&](std::uint32_t view, std::uint32_t back) {
        c.set({{3, view}, {4, 0}});
        hold();
        c.call(close_view, back);
    };
    const auto make = [&](std::uint32_t reader, std::uint32_t view, std::uint32_t back) {
        c.set({{3, c.u32(font + sfnt::mem)}, {4, view}});
        hold();
        c.call(reader, back);
        return c.r(3);
    };
    // ---- what the sfnt holds is released ----
    r26 = c.u32(font + sfnt::ttc_header);
    if (r26 != 0) {
        release(c.u32(r26), c.u32(r26 + 0xc), 0x10765848);
        release(c.u32(r26), r26, 0x10765854);
    }
    r27 = c.u32(font + sfnt::directory);
    if (r27 != 0) {
        r26 = 0;
        r23 = 0;
        while (std::int32_t(r26) < c.i16(r27 + table_directory::count)) {
            const auto entry = c.u32(c.u32(r27 + table_directory::entries) + r23);
            release(c.u32(entry), entry, 0x10765880);
            r23 += 4;
            ++r26;
        }
        release(c.u32(r27), c.u32(r27 + table_directory::entries), 0x107658a0);
        release(c.u32(r27), r27, 0x107658ac);
    }
    const auto drop = [&](std::uint32_t field, std::uint32_t back) {
        const auto record = c.u32(font + field);
        if (record != 0) release(c.u32(record), record, back);
    };
    drop(sfnt::head, 0x107658c0);
    c.put32(font + sfnt::head, 0);
    drop(sfnt::hhea, 0x107658dc);
    c.put32(font + sfnt::hhea, 0);
    drop(sfnt::vhea, 0x107658f8);
    c.put32(font + sfnt::vhea, 0);
    const auto drop_metrics = [&](std::uint32_t field, std::uint32_t first, std::uint32_t second, std::uint32_t third) {
        r26 = c.u32(font + field);
        if (r26 != 0) {
            release(c.u32(r26), c.u32(r26 + 0xc), first);
            release(c.u32(r26), c.u32(r26 + 0x10), second);
            release(c.u32(r26), r26, third);
        }
        c.put32(font + field, 0);
    };
    drop_metrics(sfnt::hmtx, 0x10765918, 0x10765924, 0x10765930);
    drop_metrics(sfnt::vmtx, 0x10765950, 0x1076595c, 0x10765968);
    drop(sfnt::maxp, 0x10765984);
    r26 = c.u32(font + sfnt::loca);
    if (r26 != 0) {
        release(c.u32(r26), c.u32(r26 + loca::offsets), 0x1076599c);
        release(c.u32(r26), r26, 0x107659a8);
    }
    r26 = 0;
    c.put32(font + sfnt::loca, r26);
    r27 = c.u32(font + sfnt::kern);
    if (r27 != 0) {
        r25 = r26;
        while (std::int32_t(r26) < c.i32(r27 + kern::table_count)) {
            r23 = c.u32(c.u32(r27 + kern::tables) + r25);
            if (c.u16(r23 + kern_subtable::format) == 0) {
                r24 = c.u32(r23 + kern_subtable::pairs);
                if (r24 != 0) {
                    release(c.u32(r24), c.u32(r24 + kern_pairs::entries), 0x107659f0);
                    release(c.u32(r24), r24, 0x107659fc);
                }
            }
            release(c.u32(r23), r23, 0x10765a08);
            r25 += 4;
            ++r26;
        }
        release(c.u32(r27), c.u32(r27 + kern::tables), 0x10765a28);
        release(c.u32(r27), r27, 0x10765a34);
    }
    // ---- the collection header and the table directory ----
    r26 = c.u32(font + sfnt::mem);
    r27 = 0;
    if (read_word(stream, 0x10765a44) == tag_ttcf) {
        r27 = allocate(r26, 0x10, 0x10765a5c);
        c.put32(r27, r26);
        c.put32(r27 + 4, read_word(stream, 0x10765a6c));
        c.put32(r27 + 8, read_word(stream, 0x10765a78));
        c.put32(r27 + 0xc, allocate(r26, c.u32(r27 + 8) << 2, 0x10765a8c));
        r26 = 0;
        r23 = r26;
        while (r26 < c.u32(r27 + 8)) {
            const auto offset = read_word(stream, 0x10765aa4);
            c.put32(c.u32(r27 + 0xc) + r23, offset);
            ++r26;
            r23 += 4;
        }
    }
    c.set(3, stream);
    hold();
    c.call(rewind_stream, 0x10765ac8);
    c.put32(font + sfnt::ttc_header, r27);
    if (const auto ttc = c.u32(font + sfnt::ttc_header); ttc != 0) {
        bool in_range = false;
        if (std::int32_t(r28) >= 0 && std::int32_t(r28) < c.i32(ttc + 8)) in_range = true;
        if (!in_range) { hold(); report(c, pool, 0xa5, 0x787, 0x10765b0c); }
        c.set({{3, stream}, {4, c.u32(c.u32(c.u32(font + sfnt::ttc_header) + 0xc) + (r28 << 2))}});
        hold();
        c.call(seek_stream, 0x10765b24);
    }
    r25 = c.u32(font + sfnt::mem);
    r27 = allocate(r25, 0x14, 0x10765b34);
    c.put32(r27, r25);
    c.put32(r27 + table_directory::version, read_word(stream, 0x10765b44));
    c.put16(r27 + table_directory::count, read_half(stream, 0x10765b50));
    c.put16(r27 + table_directory::search_range, read_half(stream, 0x10765b5c));
    c.put16(r27 + table_directory::entry_selector, read_half(stream, 0x10765b68));
    c.put16(r27 + table_directory::range_shift, read_half(stream, 0x10765b74));
    c.put32(r27 + table_directory::entries, allocate(r25, std::uint32_t(c.i16(r27 + table_directory::count)) << 2, 0x10765b88));
    r28 = 0;
    r23 = r28;
    while (std::int32_t(r28) < c.i16(r27 + table_directory::count)) {
        r26 = allocate(r25, 0x14, 0x10765ba4);
        c.put32(r26, r25);
        c.put32(r26 + table_entry::tag, read_word(stream, 0x10765bb4));
        c.put32(r26 + table_entry::checksum, read_word(stream, 0x10765bc0));
        c.put32(r26 + table_entry::offset, read_word(stream, 0x10765bcc));
        c.put32(r26 + table_entry::length, read_word(stream, 0x10765bd8));
        ++r28;
        c.put32(c.u32(r27 + table_directory::entries) + r23, r26);
        r23 += 4;
    }
    c.put32(font + sfnt::directory, r27);
    // ---- the tables ----
    const auto find = [&](std::uint32_t tag) { return find_entry(c, font, tag); };
    if (const auto entry = find(tag_head)) {
        r23 = open(entry, 0x10765c74);
        c.put32(font + sfnt::head, make(make_head, r23, 0x10765c88));
        close(r23, 0x10765c98);
    }
    if (const auto entry = find(tag_hhea)) {
        r23 = open(entry, 0x10765d0c);
        c.put32(font + sfnt::hhea, make(make_hhea, r23, 0x10765d1c));
        close(r23, 0x10765d2c);
    }
    if (const auto entry = find(tag_vhea)) {
        r23 = open(entry, 0x10765da4);
        c.put32(font + sfnt::vhea, make(make_hhea, r23, 0x10765db4));
        close(r23, 0x10765dc4);
    }
    {
        const auto entry = find(tag_maxp);
        r23 = open(entry, 0x10765e34);
        c.put32(font + sfnt::maxp, make(make_maxp, r23, 0x10765e44));
        close(r23, 0x10765e54);
    }
    r25 = find(tag_loca);
    if (r25 != 0) {
        r23 = open(r25, 0x10765ecc);
        const auto format = sext16(c.u16(c.u32(font + sfnt::head) + head::index_to_loc_format));
        c.set({{3, c.u32(font + sfnt::mem)}, {4, r23}, {5, format}, {6, c.u32(r25 + table_entry::length)}});
        hold();
        c.call(make_loca, 0x10765ee8);
        c.put32(font + sfnt::loca, c.r(3));
        close(r23, 0x10765ef8);
    }
    // hmtx and vmtx: a record {+0 mem, +4 glyph count, +8 metric count, +0xc bearings, +0x10 advances}; the last advance is repeated up
    // to the glyph limit, and the bearings of the glyphs after the metrics follow in the table.
    const auto hmtx_entry = find(tag_hmtx);  // r6 in the original: r25 keeps the loca entry until the number of metrics is read
    if (hmtx_entry != 0 && c.u32(font + sfnt::hhea) != 0) {
        r28 = open(hmtx_entry, 0x10765f78);
        r25 = c.u16(c.u32(font + sfnt::hhea) + hhea::number_of_metrics);
        c.set(3, font);
        hold();
        c.link(0x10765f8c);
        r27 = glyph_limit(c, font);
        r24 = c.u32(font + sfnt::mem);
        r26 = allocate(r24, 0x14, 0x10765fa0);
        r23 = r27 << 1;
        c.put32(r26, r24);
        c.put32(r26 + 4, r27);
        c.put32(r26 + 8, r25);
        c.put32(r26 + 0xc, allocate(r24, r23, 0x10765fc0));
        if (c.u32(r26 + 0xc) == 0) { hold(); report(c, pool, 0x3b, 0x3e4, 0x10765fe0); }
        c.put32(r26 + 0x10, allocate(r24, r23, 0x10765fec));
        if (c.u32(r26 + 0x10) == 0) { hold(); report(c, pool, 0x4a, 0x3e5, 0x1076600c); }
        r24 = 0;
        r23 = r24;
        while (std::int32_t(r24) < std::int32_t(r25)) {
            c.put16(c.u32(r26 + 0x10) + r23, read_half(r28, 0x10766020));
            const auto bearing = read_half(r28, 0x10766030);
            ++r24;
            c.put16(c.u32(r26 + 0xc) + r23, bearing);
            r23 += 2;
        }
        r23 = r24 << 1;
        r25 = c.u16(c.u32(r26 + 0x10) + r23 - 2);
        while (std::int32_t(r24) < std::int32_t(r27)) {
            c.put16(c.u32(r26 + 0x10) + r23, r25);
            const auto bearing = read_half(r28, 0x10766070);
            ++r24;
            c.put16(c.u32(r26 + 0xc) + r23, bearing);
            r23 += 2;
        }
        c.put32(font + sfnt::hmtx, r26);
        close(r28, 0x10766098);
        if (c.u32(font + sfnt::metrics_callback) != 0) {
            c.set(3, font);
            hold();
            c.link(0x107660ac);
            const auto units = units_per_em(c, font);
            c.set({{5, sext16(units)}, {3, c.u32(font + sfnt::hmtx)}, {4, c.u32(font + sfnt::mem)}, {6, font + sfnt::callback_data}, {12, c.u32(font + sfnt::metrics_callback)}});
            hold();
            c.call(indirect_glue, 0x107660c4);
            c.set(2, c.u32(c.sp() + 0x14));
        }
    }
    const auto vmtx_entry = find(tag_vmtx);
    if (vmtx_entry != 0 && c.u32(font + sfnt::vhea) != 0) {
        r28 = open(vmtx_entry, 0x10766148);
        r23 = c.u16(c.u32(font + sfnt::vhea) + hhea::number_of_metrics);
        c.set(3, font);
        hold();
        c.link(0x1076615c);
        r26 = glyph_limit(c, font);
        r24 = c.u32(font + sfnt::mem);
        r27 = allocate(r24, 0x14, 0x10766170);
        r25 = r26 << 1;
        c.put32(r27, r24);
        c.put32(r27 + 4, r26);
        c.put32(r27 + 8, r23);
        c.put32(r27 + 0xc, allocate(r24, r25, 0x10766190));
        if (c.u32(r27 + 0xc) == 0) { hold(); report(c, pool, 0x3b, 0x3e4, 0x107661b0); }
        c.put32(r27 + 0x10, allocate(r24, r25, 0x107661bc));
        if (c.u32(r27 + 0x10) == 0) { hold(); report(c, pool, 0x4a, 0x3e5, 0x107661dc); }
        r25 = 0;
        r24 = r25;
        while (std::int32_t(r25) < std::int32_t(r23)) {
            c.put16(c.u32(r27 + 0x10) + r24, read_half(r28, 0x107661f0));
            const auto bearing = read_half(r28, 0x10766200);
            ++r25;
            c.put16(c.u32(r27 + 0xc) + r24, bearing);
            r24 += 2;
        }
        r23 = r25 << 1;
        r24 = c.u16(c.u32(r27 + 0x10) + r23 - 2);
        while (std::int32_t(r25) < std::int32_t(r26)) {
            c.put16(c.u32(r27 + 0x10) + r23, r24);
            const auto bearing = read_half(r28, 0x10766240);
            ++r25;
            c.put16(c.u32(r27 + 0xc) + r23, bearing);
            r23 += 2;
        }
        c.put32(font + sfnt::vmtx, r27);
        close(r28, 0x10766268);
    }
    // kern, unless the font has an fvar table.
    std::uint32_t kern_entry = find(tag_kern);
    if (kern_entry != 0 && find(tag_fvar) != 0) kern_entry = 0;
    c.put32(font + sfnt::kern, 0);
    c.set(9, kern_entry);
    if (kern_entry != 0) {
        r23 = open(kern_entry, 0x10766350);
        c.put32(font + sfnt::kern, make(make_kern, r23, 0x10766360));
        close(r23, 0x10766370);
    }
    c.set({{3, c.u32(font + sfnt::mem)}, {4, stream}, {5, font}});
    hold();
    c.call(make_hint_font, 0x10766380);
    c.put32(font + sfnt::hint_font, c.r(3));
    if (const auto post = find(tag_post)) {
        c.set({{3, stream}, {4, c.u32(post + table_entry::offset) + 8}});
        hold();
        c.call(seek_stream, 0x107663f0);
        c.put16(font + sfnt::underline_position, read_half(stream, 0x107663f8));
        c.put16(font + sfnt::underline_thickness, read_half(stream, 0x10766404));
    } else {
        c.set(3, 0);
        c.put16(font + sfnt::underline_position, 0);
        c.put16(font + sfnt::underline_thickness, 0);
    }
}

// 0x10766ab0 New_sfntClassLogical(mem, format, index, stream, callbacks, error): the sfnt of a TrueType font over `stream`. With an
// error pointer it runs setjmp on the memory object's jump buffer and stores the code (0, or what an error site longjmps with: the
// blocks the memory object holds are then freed and the result is null). The callbacks, when given, are six words copied to
// sfnt + 0x3c (the first must be set). Format 2 reads the tables (0x10765810); any other reports the assertion. The result is the sfnt,
// its glyph limit (loca's count less one when it is not above maxp's numGlyphs) in +0x64.
std::uint32_t new_sfnt(Cpu& c, std::uint32_t mem, std::uint32_t format, std::uint32_t index, std::uint32_t stream, std::uint32_t callbacks,
                       std::uint32_t error_out) {
    Frame frame(c, 0x10766ab0, 0x60, {.first_gpr = 25});
    const auto pool = pool_of(c);
    std::uint32_t r25 = mem, r26 = stream, r27 = format, r28 = index, r29 = callbacks, r30 = error_out, r31 = pool;
    const auto hold = [&] { c.set({{25, r25}, {26, r26}, {27, r27}, {28, r28}, {29, r29}, {30, r30}, {31, r31}}); };
    if (mem == 0) { hold(); report(c, pool, 0x148, 0x982, 0x10766af0); }
    if (stream == 0) { hold(); report(c, pool, 0x154, 0x983, 0x10766b08); }
    // The code an error site gave, once setjmp has returned a second time (or a case's answer for it the first time).
    std::uint32_t code = 0;
    const auto fail = [&] {
        c.set(3, r25);
        hold();
        dispatched::free_blocks(c, r25, 0x10766ca0);
        r30 = 0;
        c.set(3, 0);
        return std::uint32_t(0);
    };
    try {
        if (error_out != 0) {
            c.set(3, mem + tsimem::jump_buffer);
            hold();
            c.compare_logical(0, error_out, 0);  // `cmplwi r30,0`: setjmp stores the condition register into the jump buffer
            c.call(setjmp_entry, 0x10766b18);
            code = c.r(3);
            c.put32(error_out, code);
            if (code != 0) return fail();
        }
        c.set({{3, mem}, {4, 0x68}});
        hold();
        c.call(0x107678a0, 0x10766b30);
        const auto font = r30 = c.r(3);
        c.put32(font + sfnt::mem, r25);
        c.set({{4, 0}, {3, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0}});
        c.put32(font + sfnt::stream, r26);
        c.put32(font + 0x58, 0);
        c.put32(font + sfnt::directory, 0);
        c.put32(font + sfnt::head, 0);
        for (const auto field : {sfnt::hhea, sfnt::vhea, sfnt::hmtx, sfnt::vmtx, sfnt::maxp, sfnt::loca, sfnt::cmap, sfnt::kern, sfnt::extra_60}) c.put32(font + field, 0);
        if (callbacks != 0) {
            if (c.u32(callbacks) == 0) { hold(); report(c, pool, 0x12d, 0x961, 0x10766bc0); }
            for (unsigned n = 0; n < 6; ++n) c.put32(font + sfnt::glyph_callback + 4 * n, c.u32(callbacks + 4 * n));
        } else {
            c.put32(font + sfnt::glyph_callback, 0);
            c.put32(font + sfnt::metrics_callback, 0);
        }
        c.put32(font + 0x38, 0);
        c.put32(font + sfnt::hint_font, 0);
        c.put32(font + sfnt::ttc_header, 0);
        if (sext16(format) == 2) {
            c.set({{3, font}, {4, c.u32(font + sfnt::stream)}, {5, r28}});
            hold();
            c.link(0x10766c38);
            read_sfnt(c, font, c.r(4), r28);
        } else {
            hold();
            report(c, pool, 0x58, 0x9da, 0x10766c4c);
        }
        if (c.u32(font + sfnt::maxp) == 0) { hold(); report(c, pool, 0xf1, 0x892, 0x10766c68); }
        std::uint32_t count = c.u16(c.u32(font + sfnt::maxp) + maxp::num_glyphs);
        const auto loca = c.u32(font + sfnt::loca);
        c.set(4, loca);
        if (loca != 0) {
            const auto entries = c.u32(loca + loca::count);
            c.set(4, loca + loca::count);
            if (!(std::int32_t(entries) > std::int32_t(count))) count = c.u32(loca + loca::count) - 1;
        }
        c.put32(font + sfnt::glyph_limit, count);
        c.set(3, font);
        return font;
    } catch (const GuestContextTransfer& transfer) {
        if (transfer.state != &c.s || c.s.pc != 0x10766b18 || c.sp() != frame.base()) throw;
        c.put32(error_out, c.r(3));
        r30 = error_out;  // the longjmp restored the registers the setjmp saved: r30 is the error pointer again
        return fail();
    }
}

// 0x10767190 Delete_sfntClass(font, error): the sfnt and everything it holds released (the ttc header, the directory and its entries,
// head, hhea, vhea, the metrics tables, maxp, loca, the cmap, kern, the hint font, the sfnt itself); with an error pointer it runs
// setjmp as the constructor does and an error site's code is stored through it (the blocks the memory object holds are then freed).
void delete_sfnt(Cpu& c, std::uint32_t font, std::uint32_t error_out) {
    Frame frame(c, 0x10767190, 0x50, {.first_gpr = 26});
    std::uint32_t r26 = error_out, r27 = c.r(27), r28 = c.r(28), r29 = c.r(29), r30 = c.r(30), r31 = font;
    const std::uint32_t saved27 = r27, saved28 = r28, saved29 = r29, saved30 = r30;
    const auto hold = [&] { c.set({{26, r26}, {27, r27}, {28, r28}, {29, r29}, {30, r30}, {31, r31}}); };
    const auto release = [&](std::uint32_t mem, std::uint32_t block, std::uint32_t back) {
        hold();
        dispatched::release(c, mem, block, back);
    };
    const auto fail = [&] {
        c.set(3, c.u32(font + sfnt::mem));
        hold();
        dispatched::free_blocks(c, c.u32(font + sfnt::mem), 0x10767418);
    };
    try {
        if (error_out != 0) {
            c.set(3, c.u32(font + sfnt::mem) + tsimem::jump_buffer);
            hold();
            c.compare(0, std::int32_t(error_out), 0);  // `or. r26,r4,r4`: setjmp stores the condition register into the jump buffer
            c.call(setjmp_entry, 0x107671b8);
            c.put32(error_out, c.r(3));
            if (c.r(3) != 0) return fail();
        }
        r30 = c.u32(font + sfnt::ttc_header);
        if (r30 != 0) {
            release(c.u32(r30), c.u32(r30 + 0xc), 0x107671dc);
            release(c.u32(r30), r30, 0x107671e8);
        }
        r29 = c.u32(font + sfnt::directory);
        if (r29 != 0) {
            r30 = 0;
            r27 = 0;
            while (std::int32_t(r30) < c.i16(r29 + table_directory::count)) {
                const auto entry = c.u32(c.u32(r29 + table_directory::entries) + r27);
                release(c.u32(entry), entry, 0x10767210);
                r27 += 4;
                ++r30;
            }
            release(c.u32(r29), c.u32(r29 + table_directory::entries), 0x10767230);
            release(c.u32(r29), r29, 0x1076723c);
        }
        for (const auto [field, back] : std::array<std::array<std::uint32_t, 2>, 3>{{{sfnt::head, 0x10767250u}, {sfnt::hhea, 0x10767264u}, {sfnt::vhea, 0x10767278u}}}) {
            const auto record = c.u32(font + field);
            if (record != 0) release(c.u32(record), record, back);
        }
        for (const auto [field, first, second, third] :
             std::array<std::array<std::uint32_t, 4>, 2>{{{sfnt::hmtx, 0x10767290u, 0x1076729cu, 0x107672a8u}, {sfnt::vmtx, 0x107672c0u, 0x107672ccu, 0x107672d8u}}}) {
            r29 = c.u32(font + field);
            if (r29 != 0) {
                release(c.u32(r29), c.u32(r29 + 0xc), first);
                release(c.u32(r29), c.u32(r29 + 0x10), second);
                release(c.u32(r29), r29, third);
            }
        }
        if (const auto record = c.u32(font + sfnt::maxp); record != 0) release(c.u32(record), record, 0x107672ec);
        r29 = c.u32(font + sfnt::loca);
        if (r29 != 0) {
            release(c.u32(r29), c.u32(r29 + loca::offsets), 0x10767304);
            release(c.u32(r29), r29, 0x10767310);
        }
        r30 = c.u32(font + sfnt::cmap);
        if (r30 != 0) {
            r29 = 0;
            r27 = 0;
            while (std::int32_t(r29) < c.i16(r30 + cmap::encoding_count)) {
                release(c.u32(r30), c.u32(c.u32(r30 + cmap::encodings) + r27), 0x10767338);
                r27 += 4;
                ++r29;
            }
            release(c.u32(r30), c.u32(r30 + cmap::encodings), 0x10767358);
            release(c.u32(r30), c.u32(r30 + cmap::data), 0x10767364);
            release(c.u32(r30), r30, 0x10767370);
        }
        r30 = c.u32(font + sfnt::kern);
        if (r30 != 0) {
            r29 = 0;
            r26 = 0;
            while (std::int32_t(r29) < c.i32(r30 + kern::table_count)) {
                r28 = c.u32(c.u32(r30 + kern::tables) + r26);
                if (c.u16(r28 + kern_subtable::format) == 0) {
                    r27 = c.u32(r28 + kern_subtable::pairs);
                    if (r27 != 0) {
                        release(c.u32(r27), c.u32(r27 + kern_pairs::entries), 0x107673b4);
                        release(c.u32(r27), r27, 0x107673c0);
                    }
                }
                release(c.u32(r28), r28, 0x107673cc);
                r26 += 4;
                ++r29;
            }
            release(c.u32(r30), c.u32(r30 + kern::tables), 0x107673ec);
            release(c.u32(r30), r30, 0x107673f8);
        }
        c.set(3, c.u32(font + sfnt::hint_font));
        hold();
        c.call(hint_font_delete_entry, 0x10767400);
        release(c.u32(font + sfnt::mem), font, 0x1076740c);
    } catch (const GuestContextTransfer& transfer) {
        if (transfer.state != &c.s || c.s.pc != 0x107671b8 || c.sp() != frame.base()) throw;
        c.put32(error_out, c.r(3));
        // The longjmp restored the registers the setjmp saved: r26 the error pointer and r27-r30 as the routine was entered.
        r26 = error_out;
        r27 = saved27;
        r28 = saved28;
        r29 = saved29;
        r30 = saved30;
        fail();
    }
}
}
