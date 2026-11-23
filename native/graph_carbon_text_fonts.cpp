// graph_carbon.cpp, block `text` (the bitmap fonts' life): the routine that loads the game's eight fonts 0x1076bbf0, the free of one bitmap
// font's glyph pixels 0x1076c980 and of all seven 0x1076c050, the two exception-cleanup destructors 0x1076c010 and 0x1076d1a0 (reached through
// descriptors only) and the destructor of the font array 0x1076ea70.
//
// The eight fonts are FONT.CPP's four outline records (the slots of the array at TOC -0x3bd8: the loader 0x1074f470 by kind 3, 0, 1, 2) and
// seven bitmap fonts of the module's own .fnt files (0x1076c0b0, in graph_carbon_text_files.cpp) named by the pool. None is a stack-contract
// leaver, so the routines here build their frames (below the leavers they reach, whose registers and dead stack are compared) and hold the
// callee-saved registers as the original does at each call. Calls to generated routines or other modules' owners go through the dispatcher at the
// original return address (`c.call`).
#include "graph_carbon_text_detail.hpp"

namespace cm::aot::graph_carbon {
namespace {
guest::Assertion assertion_at(const Cpu& c, std::uint32_t at, std::uint32_t name, std::uint32_t extension, std::uint32_t line) {
    const auto pool = c.toc(toc::pool);
    return {at, pool, pool + text_pool::format, pool + text_pool::title, name, extension, line};
}
// One of the seven bitmap fonts the loader reads: the pool's name of its file and, for the assertion that fails it, the line and the two stack
// buffers' offsets. The sites are 0x78 bytes apart from 0x1076bca4.
struct FontFile { std::uint32_t name, line, name_buffer, extension_buffer; };
constexpr FontFile font_files[7] = {{0x9d, 0x95e, 0x130, 0x68}, {0xb1, 0x964, 0x110, 0x60}, {0xc5, 0x96a, 0xf0, 0x58}, {0xd2, 0x970, 0xd0, 0x50},
                                    {0xdf, 0x976, 0xb0, 0x48}, {0xec, 0x97c, 0x90, 0x40}, {0x103, 0x982, 0x70, 0x38}};
// FONT.CPP's four records by the slot's offset in the array and the loader's kind (0 small, 1 medium, 2 large, 3 symbol); the loader's `bl` ends at
// the given return address.
struct OutlineLoad { std::uint32_t slot_offset, kind, back; };
constexpr OutlineLoad outline_loads[4] = {{0, 3, 0x1076bc40}, {4, 0, 0x1076bc5c}, {8, 1, 0x1076bc78}, {0xc, 2, 0x1076bc94}};
constexpr std::uint32_t select_font_entry = 0x10750a60, load_font_entry = 0x1074f470, data_path_entry = 0x10746280, load_bitmap_font_entry = 0x1076c0b0,
                        free_entry = 0x10775170, delete_entry = 0x107735a0, destruct_array_entry = 0x10773420;
}

// 0x1076bbf0: loads the eight fonts: FONT.CPP's selection (called with the caller's r3, as the original), the four outline records, then the seven
// .fnt files of the data folder into the bitmap font table. 1 when all loaded; otherwise 0 (a loader's refusal, or the assertion of the line of the
// font that failed); 0 at once when the graphics are already initialised.
std::uint32_t load_fonts(Cpu& c) {
    Frame frame(c, 0x1076bbf0, 0x160, {.first_gpr = 28});
    const auto pool = c.toc(toc::pool), slots = c.toc(text_toc::font_slots);
    c.set({{31, pool}, {30, c.toc(text_toc::message_buffer)}, {29, c.toc(text_toc::message_flag)}, {28, slots}});
    if (std::int8_t(c.u8(small(c, text_small::initialised))) != 0) { c.set(3, 0); return 0; }
    c.call(select_font_entry, 0x1076bc34);
    for (const auto& load : outline_loads) {
        c.set({{3, slots + load.slot_offset}, {4, load.kind}});
        c.call(load_font_entry, load.back);
        if (std::int8_t(c.r(3)) == 0) { c.set(3, 0); return 0; }
    }
    for (std::uint32_t n = 0; n < 7; ++n) {
        const auto site = 0x1076bca4 + 0x78 * n;
        c.set({{3, pool + 0x95}, {4, pool + font_files[n].name}});
        c.call(data_path_entry, site + 0xc);
        c.set({{4, c.r(3)}, {3, n + 1}});
        c.call(load_bitmap_font_entry, site + 0x18);
        if (std::int8_t(c.r(3)) == 0) {
            guest::report_assertion(c, assertion_at(c, site + 0x34, frame.at(font_files[n].name_buffer), frame.at(font_files[n].extension_buffer), font_files[n].line),
                                    [&](std::uint32_t entry, std::uint32_t back) { c.call(entry, back); });
            c.set(3, 0);
            return 0;
        }
    }
    c.set(3, 1);
    return 1;
}

// 0x1076c980: frees the pixels of the glyphs 0x20..0xff of a bitmap font (the pointers stay as they are: the table is rebuilt or zeroed by the
// callers); nothing for the font number -1. r3 is left as the last glyph's address or the free's result.
void free_font_pixels(Cpu& c, std::uint32_t font) {
    Frame frame(c, 0x1076c980, 0x50, {.first_gpr = 29});
    const auto table = c.toc(text_toc::font_table);
    c.set({{31, table}, {29, font}});
    if (extsh(font) == 0xffffffffu) return;
    const auto font_base = table + extsh(font) * bitmap_font::size;
    for (std::uint32_t character = 0x20; std::int32_t(character) <= 0xff; ++character) {
        c.set({{30, character}});
        // The pixel pointer sits 0x14 bytes into the record that starts at (font - 1) * 0x1404 + character * 0x14, read as -0x13f0 from the next font's start.
        const auto slot = font_base + character * bitmap_font::glyph_size - 0x13f0;
        c.set(3, slot + 0x13f0);
        if (c.u32(slot) != 0) {
            c.set(3, c.u32(slot));
            c.call(free_entry, 0x1076c9f0);
        }
    }
}

// 0x1076c050: frees the pixels of all seven bitmap fonts.
void free_all_fonts(Cpu& c) {
    Frame frame(c, 0x1076c050, 0x40, {});
    static constexpr std::uint32_t backs[7] = {0x1076c064, 0x1076c06c, 0x1076c074, 0x1076c07c, 0x1076c084, 0x1076c08c, 0x1076c094};
    for (std::uint32_t font = 1; font <= 7; ++font) {
        c.set(3, font);
        c.call(0x1076c980, backs[font - 1]);
    }
}

// 0x1076c010 and 0x1076d1a0: the cleanup destructors of two compiler-generated objects: delete the object when it is not null and `flags` (a
// halfword) is positive; return the object. `entry` names which (they differ in their return address after the delete's `bl`).
std::uint32_t delete_cleanup_object(Cpu& c, std::uint32_t entry, std::uint32_t object, std::uint32_t flags) {
    Frame frame(c, entry, 0x40, {.first_gpr = 31});
    c.set(31, object);
    if (object != 0 && std::int32_t(extsh(flags)) > 0) {
        c.set(3, object);
        c.call(delete_entry, entry == 0x1076c010 ? 0x1076c034 : 0x1076d1c8);
    }
    c.set(3, object);
    return object;
}

// 0x1076ea70: the destructor of the array of eight font slots: FONT.CPP's element destructors through the object storage's array destructor.
void destroy_font_array(Cpu& c) {
    Frame frame(c, 0x1076ea70, 0x40, {});
    c.set({{3, c.toc(text_toc::font_slots)}, {5, 4}, {4, c.toc(-0x5b1c)}, {6, 8}});
    c.call(destruct_array_entry, 0x1076ea90);
}
}
