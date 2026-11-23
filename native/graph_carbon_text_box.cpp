// graph_carbon.cpp, block `text` (the text box): 0x1076e160 draws a text word-wrapped into a rectangle.
//
//   (x1, y1)-(x2, y2) the box (inclusive); `style`: 1 left-aligned (else centred, or right-aligned with 0x40), 2 top-aligned (else centred
//   vertically), 0x20 a shadow (the line twice: lightened to 166 percent one pixel right and down, then darkened to 66 percent in place, both
//   from the colour of the surface pixel under the line's first character), 0x80 replaces the characters with '*', 0x100 ends a line at the
//   first cut by width instead of carrying on; `font` a font number, `colour`, `text`, and the ninth argument on the stack (the caller's
//   sp + 0x38): `limit` (-1 none) handed to the drawer 0x1076ca50.
//
// The text is split into lines kept in a list object (allocated with the object storage's new, built by 0x10738c00, filled by 0x10738d10,
// walked by 0x10738e00 and destroyed by 0x10738c20). Characters below 0x20 are skipped; one that would make the line wider than the box (the
// string so far measured by 0x1076d1e0) is dropped and cuts the line; a newline or the end of the text ends one. With a limit of 0 or more a
// list without a line gets an empty one. The lines are drawn top to bottom, a font height apart; nothing is drawn while the graphics are being
// created, for the font number -1 or for no text.
//
// STACK CONTRACT: a leaver (stmw r18): the frame is the original's (0x170: the three-byte buffer for one character at 0x38 and the line
// buffer at 0x3c, which the C library routines fill) and every call is made at its own stack pointer with r18-r31 as the original holds them
// there (the `Kept` registers below, named for their dominant role; r18, r23, r24, r29 and r30 change role as the comments say). r3-r10 are
// those the last callee left, with the arguments and the temporaries the original sets: the callees see exactly the registers they saw.
#include "graph_carbon_text_detail.hpp"

namespace cm::aot::graph_carbon {
namespace {
// The routines the text box calls: the C library's strlen / strcpy / strcat / sprintf, the object storage's new and the list object's four
// methods, and this module's width, height and drawer.
namespace callee {
inline constexpr std::uint32_t new_object = 0x10773520, construct_list = 0x10738c00, append_line = 0x10738d10, next_line = 0x10738e00,
    destroy_list = 0x10738c20, copy_string = 0x1077dbf0, string_length = 0x1077dbd0, append_string = 0x1077dc60, format = 0x1077b720,
    text_width = 0x1076d1e0, font_height = 0x1076d470, draw_text = 0x1076ca50;
}
// The pool's strings: the empty string and the "%c" format.
namespace box_pool {
inline constexpr std::uint32_t empty = 0x74, character_format = 0x14b;
}
// Style bits.
namespace style_bit {
inline constexpr std::uint32_t left = 1, top = 2, shadow = 0x20, right = 0x40, mask = 0x80, cut_ends_line = 0x100;
}
// The frame's locals.
constexpr std::int32_t one_character = 0x38, line_buffer = 0x3c;
// A list object: the count (halfword) at +0, the first line at +4, the cursor at +0xc.
namespace list_field {
inline constexpr std::uint32_t count = 0, first = 4, cursor = 0xc;
}

// One channel of the pixel under the shadow, scaled by `percent` and clamped, as the original does it: the product divided by 100 through the
// compiler's multiply by 0x51eb851f (a non-negative product below 2^17: plain division), at most 255.
constexpr std::uint32_t scaled(std::uint32_t channel, std::uint32_t percent) {
    const auto value = channel * percent / 100;
    return value < 0xff ? value : 0xffu;
}
// The 15-bit pixel of a surface pixel scaled by `percent`: red 10-14, green 5-9 and blue 0-4 from the top five bits of each 8-bit channel.
constexpr std::uint32_t shadow_colour(std::uint32_t under, std::uint32_t percent) {
    const auto red = scaled((under & pixel::red_mask) >> 7, percent), green = scaled((under & pixel::green_mask) >> 2, percent),
               blue = scaled((under << 3) & 0xf8, percent);
    return (((red & 0xf8) << 7) | ((blue & 0xff) >> 3) | ((green & 0xf8) << 2)) & 0xffff;
}
}

void text_box(Cpu& c, std::uint32_t x1, std::uint32_t y1, std::uint32_t x2, std::uint32_t y2, std::uint32_t style, std::uint32_t font,
              std::uint32_t colour, std::uint32_t text) {
    Frame frame(c, 0x1076e160, 0x170, {.first_gpr = 18});
    const auto pool = c.toc(toc::pool);
    // Held registers. r18, r27 and r30 keep the caller's values until the original writes them.
    Kept<18> flag(c, c.r(18));        // the object, then "the line was cut", then the line count, then the mask loop's index
    Kept<19> height(c);               // the box's height
    Kept<20> ink(c, colour);          // the colour
    Kept<21> right(c, x2);
    Kept<22> width(c);                // the box's width
    Kept<23> kept_pool(c, pool);      // the pool, then the shadow's surface pixel
    Kept<24> kept_text(c, text);      // the text, then the line's x
    Kept<25> left(c, x1);
    Kept<26> limit(c, c.u32(frame.caller() + 0x38));
    Kept<27> lines(c, c.r(27));       // the list object
    Kept<28> kept_style(c, style);
    Kept<29> top(c, y1);              // the top, then the line being drawn
    Kept<30> position(c, c.r(30));    // the index in the text, then the line's y
    Kept<31> kept_font(c, font);
    if (std::int8_t(c.u8(small(c, text_small::initialised))) != 0 || extsh(font) == 0xffffffffu || text == 0) return;

    const auto call = [&](std::uint32_t target, std::uint32_t back, std::initializer_list<std::pair<unsigned, std::uint32_t>> arguments = {}) {
        c.set(arguments);
        c.call(target, back);
        return c.r(3);
    };
    const auto length_of = [&](std::uint32_t string, std::uint32_t back) { return std::int32_t(call(callee::string_length, back, {{3, string}})); };
    width = x2 - x1 + 1;
    height = y2 - y1 + 1;
    const auto at_line = frame.at(line_buffer), one = frame.at(one_character);
    // The list object (new gives 0 when the storage is exhausted: the constructor is then skipped, and the first use faults in the game too).
    c.set({{3, 0x10}, {4, y2 - y1}});
    c.call(callee::new_object, 0x1076e1cc);
    flag = c.r(3);
    if (c.r(3) != 0) call(callee::construct_list, 0x1076e1dc, {{3, flag}});
    lines = flag;
    call(callee::copy_string, 0x1076e1ec, {{3, at_line}, {4, pool + box_pool::empty}});
    flag = 0;
    position = 1;
    // Split the text into lines: add each character that fits to the line buffer, cut where the width exceeds the box, end the line at a newline,
    // at the end of the text or (style 0x100) at a cut.
    for (;; position = position + 1) {
        if (std::int32_t(position) > length_of(kept_text, 0x1076e2cc)) break;
        const auto character = c.u8(kept_text + position - 1);
        if (character >= 0x20 && std::int8_t(flag & 0xff) == 0) {
            call(callee::format, 0x1076e228, {{3, one}, {4, pool + box_pool::character_format}, {5, std::uint32_t(std::int32_t(std::int8_t(character)))}});
            call(callee::append_string, 0x1076e234, {{3, at_line}, {4, one}});
            call(callee::text_width, 0x1076e240, {{3, kept_font}, {4, at_line}});
            if (std::int32_t(c.r(3)) > std::int32_t(width)) {
                flag = 0;
                const auto used = length_of(at_line, 0x1076e254);
                c.set(4, frame.at(line_buffer - 1));
                c.put8(frame.at(line_buffer - 1) + std::uint32_t(used), 0);
                flag = 1;
            }
        }
        bool ends = c.u8(kept_text + position - 1) == '\n';
        if (!ends) ends = std::int32_t(position) == length_of(kept_text, 0x1076e278);
        if (!ends) ends = (kept_style & style_bit::cut_ends_line) != 0 && std::int8_t(flag & 0xff) != 0;
        if (ends) {
            const auto used = length_of(at_line, 0x1076e298);
            call(callee::append_line, 0x1076e2b0, {{3, lines}, {4, at_line}, {5, extsh(std::uint32_t(used) + 1)}});
            call(callee::copy_string, 0x1076e2bc, {{3, at_line}, {4, pool + box_pool::empty}});
            flag = 0;
        }
    }
    // With a limit to draw towards, an empty text still has one (empty) line.
    if (std::int32_t(limit) > -1 && c.i16(lines + list_field::count) == 0)
        call(callee::append_line, 0x1076e2f8, {{3, lines}, {4, pool + box_pool::empty}, {5, 1}});
    // The first line's y: the top for style 2, else the box centred on the lines.
    if ((kept_style & style_bit::top) != 0) {
        position = top;
    } else {
        flag = extsh(c.u16(lines + list_field::count));
        const auto line_height = call(callee::font_height, 0x1076e314, {{3, kept_font}});
        const auto room = std::uint32_t(height) - extsh(flag) * line_height;
        position = top + std::uint32_t(std::int32_t(room) / 2);
    }
    c.put32(lines + list_field::cursor, c.u32(lines + list_field::first));

    for (;;) {
        top = call(callee::next_line, 0x1076e5bc, {{3, lines}});
        if (top == 0) break;
        const auto line = std::uint32_t(top);
        if ((kept_style & style_bit::mask) != 0) {
            // The line becomes asterisks (the length is asked again at each step).
            flag = 0;
            while (std::int32_t(flag) < length_of(line, 0x1076e360)) {
                c.put8(line + flag, '*');
                flag = flag + 1;
            }
        }
        if ((kept_style & style_bit::left) != 0) {
            kept_text = left;
        } else if ((kept_style & style_bit::right) != 0) {
            const auto measured = call(callee::text_width, 0x1076e38c, {{3, kept_font}, {4, line}});
            kept_text = right - measured + 1;
        } else {
            const auto measured = call(callee::text_width, 0x1076e3a4, {{3, kept_font}, {4, line}});
            kept_text = left + std::uint32_t(std::int32_t(std::uint32_t(width) - measured) / 2);
        }
        if ((kept_style & style_bit::shadow) == 0) {
            c.set({{3, kept_text}, {4, position}, {5, kept_font}, {6, ink}, {7, line}, {8, limit}});
            c.call(callee::draw_text, 0x1076e5a8);
        } else {
            // The shadow: the colour of the surface pixel under the line's start, lighter one pixel right and down, then darker in place.
            const auto stride = c.i16(small(c, text_small::stride));
            const auto offset = (kept_text << 1) + ((position * std::uint32_t(std::int32_t(stride))) << 1);
            const std::uint32_t under = c.u16(c.u32(small(c, text_small::surface)) + offset);
            kept_pool = under;
            c.set({{3, kept_text + 1}, {4, position + 1}, {5, kept_font}, {6, shadow_colour(under, 0xa6)}, {7, line}, {8, limit}});
            c.call(callee::draw_text, 0x1076e4b4);
            c.set({{3, kept_text}, {4, position}, {5, kept_font}, {6, shadow_colour(under, 0x42)}, {7, line}, {8, limit}});
            c.call(callee::draw_text, 0x1076e588);
        }
        position = position + call(callee::font_height, 0x1076e5b0, {{3, kept_font}});
    }
    call(callee::destroy_list, 0x1076e5d0, {{3, lines}, {4, 1}});
}
}
