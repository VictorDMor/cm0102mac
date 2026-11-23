// graph_carbon.cpp, block `text` (the fonts' life): cases for the .fnt loader 0x1076c0b0, the loader of the eight fonts 0x1076bbf0, the free of the glyph
// pixels 0x1076c980 / 0x1076c050, the cleanup destructors 0x1076c010 / 0x1076d1a0 and the font array's destructor 0x1076ea70.
//
// The .fnt loader reads its file with the stream layer's fread (0x10775fe0) straight into the font record. A case answers the file layer: fopen
// 0x10739de0 returns a FILE pointer of the scratch area (or 0), fread is answered 1 (or failed at its n-th call by `nth_answers`), fclose 0; the
// bytes the file would hold are written into the record by a poke at each dispatch of fread (the loader memsets the record first): the height and
// the four words of every glyph, as the record holds them after the loader's in-place swap would leave them, i.e. as small numbers (the poke
// re-writes them at every read, so the loader's later reads of a swapped word see the small number again: the swap itself shows in the registers and
// arguments of the calls that follow it, which the call trace compares). The pixel buffers are the allocator's real blocks. Labels start with "files:".
#include "graph_carbon_checks.hpp"
#include "graph_carbon_text_detail.hpp"
#include <string>

namespace cm::aot::graph_carbon_checks {
namespace {
using namespace cm::aot::graph_carbon;
constexpr std::uint32_t load_font_entry = 0x1076c0b0, load_fonts_entry = 0x1076bbf0, free_pixels_entry = 0x1076c980, free_all_entry = 0x1076c050, cleanup_a = 0x1076c010,
                        cleanup_b = 0x1076d1a0, destroy_array_entry = 0x1076ea70;
constexpr std::uint32_t fopen_entry = 0x10739de0, fread_entry = 0x10775fe0, fclose_entry = 0x107766f0, allocate_entry = 0x10775130, delete_entry = 0x107735a0,
                        destruct_array_entry = 0x10773420, select_font_entry = 0x10750a60, outline_loader_entry = 0x1074f470, data_path_entry = 0x10746280;
constexpr std::uint32_t file_at = scratch_base + 0x1000, path_at = scratch_base + 0x2000;
constexpr std::uint32_t minus_one = 0xffffffffu;

std::vector<std::pair<unsigned, std::uint32_t>> registers(std::initializer_list<std::pair<unsigned, std::uint32_t>> used) {
    std::vector<std::pair<unsigned, std::uint32_t>> all;
    for (unsigned n = 3; n <= 10; ++n) all.emplace_back(n, 0x7a000000u + n * 0x01010101u);
    for (const auto& [n, value] : used) all[n - 3].second = value;
    return all;
}
// The file's words as the record holds them for font `font`: the height and per character a width, the bytes a row (0 for a glyph without pixels), the
// space before and after. `rows` makes character 0x20 carry pixels too (to reach the pixel read first).
std::vector<Write> font_words(std::uint32_t base, std::uint32_t font, bool rows) {
    std::vector<Write> memory = {w32(base + bitmap_font::height, 5 + font)};
    for (std::uint32_t character = 0x20; character <= 0xff; ++character) {
        const auto record = bitmap_font::glyph(base, character);
        const std::uint32_t width = (character * 7 + font) % 7;
        const std::uint32_t row_bytes = character == 0x20 ? (rows ? 2 : 0) : (character % 5 == 1 ? 0 : (width + 1) / 2);
        memory.push_back(w32(record + bitmap_font::glyph_field::width, width));
        memory.push_back(w32(record + bitmap_font::glyph_field::row_bytes, row_bytes));
        memory.push_back(w32(record + bitmap_font::glyph_field::lead, (character * 3 + font) % 5));
        memory.push_back(w32(record + bitmap_font::glyph_field::trail, (character * 5 + font) % 4));
    }
    return memory;
}
}

GRAPH_CARBON_CASES("text") {
    const auto table = scenario.toc(text_toc::font_table);
    const auto flag = scenario.toc(text_toc::message_flag);
    (void)flag;
    const auto base_of = [&](std::uint32_t font) { return table + (font - 1) * bitmap_font::size; };

    // ---- 0x1076c0b0: loads a .fnt file. Rare paths first (the call trace pairs the first cases): no path, fopen fails, each read fails, the allocation fails.
    const auto load = [&](std::uint32_t font, std::uint32_t path, const std::string& label) {
        Case item{.entry = load_font_entry, .registers = registers({{3, font}, {4, path}}), .label = "files: load " + label};
        item.answers = {{fopen_entry, file_at}, {fread_entry, 1}, {fclose_entry, 0}};
        item.original_owners = {fread_entry};
        item.pokes = {{fread_entry, font_words(base_of(font == 0 ? 1 : font), font, false)}};
        return item;
    };
    cases.push_back(load(3, 0, "no path (assertion)"));
    {
        auto item = load(3, path_at, "fopen fails");
        item.answers = {{fopen_entry, 0}};
        item.pokes = {};
        cases.push_back(item);
    }
    for (const std::uint32_t font : {3u, 6u})
        for (const unsigned failing : {1u, 2u, 3u, 4u, 5u, 6u, 9u, 40u, 400u}) {
            auto item = load(font, path_at, "font " + std::to_string(font) + " read " + std::to_string(failing) + " fails");
            item.pokes = {{fread_entry, font_words(base_of(font), font, true)}};
            item.nth_answers = {{fread_entry, failing, 0}};
            cases.push_back(item);
        }
    for (const std::uint32_t font : {3u, 7u})
        for (const unsigned failing : {1u, 2u, 7u, 100u}) {
            auto item = load(font, path_at, "font " + std::to_string(font) + " allocation " + std::to_string(failing) + " fails");
            item.nth_answers = {{allocate_entry, failing, 0}};
            item.original_owners = {fread_entry, allocate_entry};
            cases.push_back(item);
        }
    for (std::uint32_t font = 1; font <= 7; ++font) {
        cases.push_back(load(font, path_at, "font " + std::to_string(font)));
        auto item = load(font, path_at, "font " + std::to_string(font) + " with pixels on the first glyph");
        item.pokes = {{fread_entry, font_words(base_of(font), font, true)}};
        cases.push_back(item);
    }

    // ---- 0x1076bbf0: loads the eight fonts. The callees answered: FONT.CPP's selection and loader (the n-th loader call fails: four sites), the data path
    // and the bitmap font loader (the n-th of seven fails: the seven assertions).
    for (unsigned failing = 1; failing <= 7; ++failing) {
        Case item{.entry = load_fonts_entry, .registers = registers({{3, 0x5151}}), .label = "files: load fonts, bitmap font " + std::to_string(failing) + " fails"};
        item.answers = {{select_font_entry, 0}, {outline_loader_entry, 1}, {data_path_entry, path_at}, {load_font_entry, 1}};
        item.nth_answers = {{load_font_entry, failing, 0}};
        cases.push_back(item);
    }
    for (unsigned failing = 1; failing <= 4; ++failing) {
        Case item{.entry = load_fonts_entry, .registers = registers({{3, 0x5151}}), .label = "files: load fonts, outline loader " + std::to_string(failing) + " fails"};
        item.answers = {{select_font_entry, 0}, {outline_loader_entry, 1}, {data_path_entry, path_at}, {load_font_entry, 1}};
        item.nth_answers = {{outline_loader_entry, failing, 0x100}};  // a nonzero word with a zero low byte: the byte test fails
        cases.push_back(item);
    }
    for (const std::uint32_t result : {1u, 0x101u, 0xff01u, 0x80u}) {
        Case item{.entry = load_fonts_entry, .registers = registers({{3, 0x5151}}), .label = "files: load fonts, all loaded with " + hex(result)};
        item.answers = {{select_font_entry, 0}, {outline_loader_entry, result}, {data_path_entry, path_at}, {load_font_entry, result}};
        cases.push_back(item);
    }
    {
        Case item{.entry = load_fonts_entry, .registers = registers({{3, 0x5151}}), .label = "files: load fonts, initialised"};
        item.memory = {w8(0x108bba90 - 0x3550, 1)};
        cases.push_back(item);
    }

    // ---- 0x1076c980: frees the glyph pixels of a bitmap font (the snapshot's real fonts: real blocks; pointers cleared in some glyphs).
    for (const std::uint32_t font : {1u, 2u, 7u, 0xffffu, minus_one, 0x10003u}) {
        std::vector<Write> memory;
        if (font == 2u)
            for (std::uint32_t character = 0x20; character <= 0xff; character += 3) memory.push_back(w32(bitmap_font::glyph(base_of(2), character) + bitmap_font::glyph_field::pixels, 0));
        cases.push_back({.entry = free_pixels_entry, .registers = registers({{3, font}}), .memory = memory, .label = "files: free pixels font " + hex(font)});
    }
    cases.push_back({.entry = free_all_entry, .registers = registers({}), .label = "files: free all fonts"});

    // ---- 0x1076c010 and 0x1076d1a0: the cleanup destructors.
    for (const std::uint32_t entry : {cleanup_a, cleanup_b})
        for (const std::uint32_t object : {0u, scratch_base + 0x3000})
            for (const std::uint32_t flags : {0u, 1u, 2u, 0x7fffu, 0x8000u, 0xffffu, 0x10001u, 0x18000u, minus_one}) {
                Case item{.entry = entry, .registers = registers({{3, object}, {4, flags}}), .label = "files: cleanup " + hex(entry) + " object " + hex(object) + " flags " + hex(flags)};
                item.answers = {{delete_entry, 0x1234}};
                item.original_owners = {delete_entry};
                cases.push_back(item);
            }

    // The delete runs for real on a pointer that is no block when the flags ask for none: a delete the rewrite wrongly makes would fault or corrupt.
    for (const std::uint32_t entry : {cleanup_a, cleanup_b})
        for (const std::uint32_t flags : {0u, 0x8000u, 0xffffu, 0x18000u})
            cases.push_back({.entry = entry, .registers = registers({{3, scratch_base + 0x3010}, {4, flags}}), .label = "files: cleanup " + hex(entry) + " real delete not asked, flags " + hex(flags)});

    // ---- 0x1076ea70: the font array's destructor.
    {
        Case item{.entry = destroy_array_entry, .registers = registers({}), .label = "files: destroy font array"};
        item.answers = {{destruct_array_entry, 0x4321}};
        cases.push_back(item);
    }
}
}
