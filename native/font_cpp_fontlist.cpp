// FONT.CPP, block `fontlist` (0x10750a00..0x10750d28, five routines, 196 instructions): the list of fonts the preferences screen
// offers. The list is built on first use by the generated 0x10750d30 (a later block): each entry point first runs the same
// prelude (find the FONTS folder, build the list, mark it built), then reads or writes the list's variables.
//
// Registers: every routine leaves r3 and r4-r10 as the original does (the checker compares them in full). The original changes only
// r3, r4 and r5 itself, before each of its calls, so that is all this code sets; the registers a callee leaves stay in place. Each
// entry builds the original's frame (the size and the saved registers of its prologue) so that the callees' own frames lie at the
// original's depth: a callee leaves addresses of its locals in r4-r10 (the generated builder does), which a comparison in full sees.
// The stack bytes themselves are free (tools/stack_contract.py module FONT.CPP lists no reader of these routines' frames).
#include "font_cpp_detail.hpp"
#include "utils.hpp"

namespace cm::aot::font_cpp {
namespace {
// The original's two `bl`s of the prelude: the folder finder and the list builder, by the address after each.
struct PreludeReturns { std::uint32_t folder, builder; };

// `extsb. r0,rN; beq`: a byte result tested as a signed byte.
bool byte_nonzero(Cpu& c, std::uint32_t value) { return nonzero(c, std::int32_t(std::int8_t(value))); }

// The prelude of 0x10750a00, 0x10750a60, 0x10750b00 and 0x10750b60: when the built flag is zero, find the folder and, when found,
// build the list and set the flag. The flag byte, the folder result and the builder result are all tested as signed bytes.
void ensure_font_list(Cpu& c, PreludeReturns back) {
    const auto built = c.toc(toc::list_built);
    if (byte_nonzero(c, c.u8(built))) return;
    c.link(back.folder);
    locate_font_folder(c);
    if (!byte_nonzero(c, c.r(3))) return;
    c.call(build_font_list, back.builder);
    if (!byte_nonzero(c, c.r(3))) return;
    c.put8(built, 1);
}

// `bl 0x1077dc10`: the limited copy of text into the buffer.
void copy_limited_text(Cpu& c, std::uint32_t buffer, std::uint32_t text, std::uint32_t size, std::uint32_t back) {
    c.set({{3, buffer}, {4, text}, {5, size}});
    c.call(copy_limited, back);
}
}

std::uint32_t locate_font_folder(Cpu& c) {
    Frame frame(c, 0x10750ca0, 0x40, {.first_gpr = 30});
    const auto pool = c.toc(toc::pool), folder = c.toc(toc::folder_path);
    // The data path of FONTS under SI_DATA; failing that under CM3_DATA. (utils::data_path always returns its static path buffer, a
    // TOC word that is never zero, so the second lookup and the empty-folder branch are unreachable in the game and no case of the
    // checker can reach them except with the page at address 0 mapped and that word poked to zero: font_cpp_checks_fontlist.cpp.)
    c.set({{3, pool + pool_text::data_variable}, {4, pool + pool_text::fonts_file}});
    direct_call(c, data_path, 0x10750cc8);
    auto path = utils::data_path(c, c.r(3), c.r(4));
    c.set(4, path);
    if (path == 0) {
        c.set({{3, pool + pool_text::second_variable}, {4, pool + pool_text::fonts_file}});
        direct_call(c, data_path, 0x10750cdc);
        path = utils::data_path(c, c.r(3), c.r(4));
        c.set(4, path);
    }
    std::uint32_t found;
    if (path != 0) {
        copy_limited_text(c, folder, path, folder_size, 0x10750cf4);
        c.put8(folder + folder_size - 1, 0);
        found = 1;
    } else {
        c.put8(folder, 0);
        found = 0;
    }
    c.set(3, found);
    return found;
}

void load_font_list(Cpu& c) {
    Frame frame(c, 0x10750a00, 0x40, {.first_gpr = 31});
    ensure_font_list(c, {0x10750a24, 0x10750a30});
}

void select_font(Cpu& c, std::int32_t index) {
    Frame frame(c, 0x10750a60, 0x50, {.first_gpr = 29});
    ensure_font_list(c, {0x10750a94, 0x10750aa0});
    // The two built-in styles are always selectable; any other index must be below the count. An index below -2 is not rejected:
    // the original compares it, signed, with the (unsigned short) count, which it is always below.
    bool selectable = index < 0 && index >= futuristic_style;
    if (!selectable) {
        c.set(3, c.toc(toc::list_count));
        selectable = index < std::int32_t(c.u16(c.r(3)));
    }
    if (selectable) c.put32(c.toc(toc::selection), std::uint32_t(index));
}

std::uint32_t font_count(Cpu& c) {
    Frame frame(c, 0x10750b00, 0x40, {.first_gpr = 31});
    ensure_font_list(c, {0x10750b24, 0x10750b30});
    c.set(3, c.toc(toc::list_count));
    const auto count = std::uint32_t(c.u16(c.r(3)));
    c.set(3, count);
    return count;
}

void font_name(Cpu& c, std::int32_t index, std::uint32_t buffer, std::int32_t size) {
    Frame frame(c, 0x10750b60, 0x50, {.first_gpr = 26});
    ensure_font_list(c, {0x10750b98, 0x10750ba4});
    const auto pool = c.toc(toc::pool);
    if (index == traditional_style || index == futuristic_style) {
        // The built-in styles: the localized "<name> - COMMENT - fonts" text, copied.
        const auto text = c.toc(toc::name_text);
        const bool traditional = index == traditional_style;
        c.set({{3, text}, {4, pool + (traditional ? pool_text::traditional : pool_text::futuristic)}, {5, pool + pool_text::none}});
        c.call(localized_text, traditional ? 0x10750bdc : 0x10750c00);
        copy_limited_text(c, buffer, text, std::uint32_t(size), traditional ? 0x10750bec : 0x10750c10);
    } else {
        // A listed font: the row's alias, else its name. An empty table, a negative index or one past the count give an empty
        // string, and then the last byte is not written.
        const auto table = c.toc(toc::list_table);
        c.set(3, table);
        const auto rows = c.u32(table);
        c.set(5, rows);
        bool listed = rows != 0 && index >= 0;
        if (listed) {
            c.set(3, c.toc(toc::list_count));
            listed = index < std::int32_t(c.u16(c.r(3)));
        }
        if (!listed) {
            c.put8(buffer, 0);
            return;
        }
        const auto offset = std::uint32_t(index) * font_row::size;
        c.set(3, rows + offset);
        auto name = c.u32(rows + offset + font_row::alias);
        c.set(4, name);
        if (name != 0) {
            copy_limited_text(c, buffer, name, std::uint32_t(size), 0x10750c68);
        } else {
            name = c.u32(rows + offset + font_row::name);
            copy_limited_text(c, buffer, name, std::uint32_t(size), 0x10750c7c);
        }
    }
    // Both ways end by terminating the buffer's last byte (r3 holds its end).
    const auto end = buffer + std::uint32_t(size);
    c.set(3, end);
    c.put8(end - 1, 0);
}
}
