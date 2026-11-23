#pragma once
// FONT.CPP's record views and constants (tools/font_cpp_audit.py, .local/rewrites/font_cpp/brief.md). Offsets come from the listing
// and the decompilation; "?" marks one inferred from a single reader.
#include "font_cpp.hpp"
#include "font_cpp_adapter.hpp"
#include "guest_values.hpp"

namespace cm::aot::font_cpp {
using namespace guest;

// Table of contents: the module's string pool (".ttf" first, the file name "FONT.CPP" at +0x12), the double constants of the skew,
// the style functions the selector hands to the sfnt, and the font list's variables.
namespace toc {
inline constexpr std::int32_t pool = -0x3e0c, skew_constants = -0x3e10, bold_style = -0x5b78, italic_style = -0x5b74,
                              list_count = -0x5b80,   // unsigned short: the number of listed fonts
                              list_built = -0x5b7c,   // byte: the list has been built
                              list_table = -0x5b6c,   // pointer variable: the table of 12-byte rows
                              folder_path = -0x5b70,  // 0x100 bytes: the FONTS folder's path
                              selection = -0x740c,    // int: the selected font index (-2, -1 or a row), read by graph_carbon.cpp
                              name_text = -0x7bb8;    // the localized names' text buffer
}
// Offsets into the string pool.
namespace pool_text {
inline constexpr std::uint32_t data_variable = 0x45,    // "SI_DATA"
                               traditional = 0xa3,      // "Traditional<%s - COMMENT - fonts>"
                               none = 0xc5,             // ""
                               futuristic = 0xc6,       // "Futuristic<%s - COMMENT - fonts>"
                               fonts_file = 0xe7,       // "FONTS"
                               second_variable = 0xed;  // "CM3_DATA"
}
// A row of the font table (0xc bytes): the name the list shows is the alias when it has one, else the first name.
namespace font_row {
inline constexpr std::uint32_t size = 0xc, name = 0, alias = 4;
}
inline constexpr std::uint32_t folder_size = 0x100;
// The index of the built-in styles.
inline constexpr std::int32_t traditional_style = -1, futuristic_style = -2;

// The font record (0x24 bytes, ctor 0x1074f390), as the style selector, the glyph cache and the string width read it. The callers pass a
// pointer to a slot that holds the record's pointer.
namespace font_record {
inline constexpr std::uint32_t loaded = 0x0, sfnt_memory = 0x8, stream = 0xc, sfnt = 0x10, scaler = 0x14, style = 0x18 /* ushort */,
                               size = 0x1a /* byte */, cache = 0x1c /* array of glyph record pointers */, cache_count = 0x20;
inline constexpr std::uint32_t cache_limit = 0x100;
}
// A glyph record of the cache (0x20 bytes, malloc'd by 0x10750580): the bitmap's placement and size, the pixels, the advance and the key
// the cache is searched by (the style's low five bits, the size and the character).
namespace glyph_cache {
inline constexpr std::uint32_t left = 0x0, top = 0x4, width = 0x8, row_bytes = 0xc, height = 0x10, bitmap = 0x14, advance = 0x18,
                               flags = 0x1c /* ushort */, size = 0x1e /* byte */, character = 0x1f /* byte */, record_size = 0x20;
}
// The font engine's scaler object (T2K.C) and its render result, the fields this module reads (font_engine_detail.hpp names them).
namespace scaler {
inline constexpr std::uint32_t advance = 0x94, top = 0xa4, left = 0xa8, bitmap_width = 0xd0, bitmap_height = 0xd4, bitmap_stride = 0xd8,
                               bitmap = 0xdc, sfnt = 0x144;
}
// The sfnt's platform and encoding ids (TRUETYPE.C), set by the selector to 3 and 1 (Microsoft, Unicode).
namespace sfnt_ids {
inline constexpr std::uint32_t platform = 0x30, encoding = 0x32;
}

// Routines of other modules (still generated unless said): the list builder of this module (0x10750d30, generated until its block),
// the string copy with a limit (0x1077dc10, MAIN.CPP's strncpy owner) and the localized text formatter (LANGLIB.CPP, generated).
inline constexpr std::uint32_t build_font_list = 0x10750d30, copy_limited = 0x1077dc10, localized_text = 0x10727890,
                               data_path = 0x10746280;

// `bl target` into a routine of another module that is rewritten in C++ and called directly: the link register as the original's call
// leaves it, and the checker's call trace told (the original's generated call is a dispatch it records).
inline void direct_call(Cpu& c, std::uint32_t target, std::uint32_t back) {
    c.link(back);
    if (font_cpp_direct_call_probe) font_cpp_direct_call_probe(c.s, target);
}
}
