// FONT.CPP, block `drawer` (0x1074f800): cases for the text drawer. The state is the save-info snapshot's: the font array at TOC -0x3bd8 holds
// four loaded records (slot 0 small size 9, slot 1 medium 14 and slot 3 large 20 have a sfnt, a scaler and glyph caches; slot 2 is loaded
// with neither yet; slots 4-7 are not loaded). A window record, its 1-5-5-5 back buffer (random pixels) and the strings are built in the
// scratch area with the real layout (window: +0 buffer, +4 stride in pixels, +0xa and +0xc signed halfword width and height). The
// callback is the real descriptor 0x108bdaac (code 0x1076ca20, graph_carbon's), or 0, or the glue answered (a callback that asks to skip).
// The glyph cache 0x10750580 and the style selector 0x10750320 run for real on both sides (the engine renders the glyphs that are not
// cached). Labels start with "drawer:".
#include "font_cpp_checks.hpp"
#include "font_cpp_detail.hpp"
#include <cstdlib>
#include <random>

namespace cm::aot::font_cpp_checks {
namespace {
using namespace cm::aot::font_cpp;
constexpr std::uint32_t drawer = 0x1074f800, report = 0x107755c0, glue = 0x10781320, callback_descriptor = 0x108bdaac;
constexpr std::uint32_t window_at = scratch_base + 0x40000, surface_at = scratch_base + 0x40400, text_at = scratch_base + 0x45000, slot_at = scratch_base + 0x47000,
                        glyph_at = scratch_base + 0x48000, bitmap_at = scratch_base + 0x48100, long_text_at = scratch_base + 0x70000;
constexpr unsigned surface_pixels = 96 * 48;

std::uint16_t background(unsigned k) { return std::uint16_t(k * 0x9e37u + (k >> 3) * 0x7f4au + 0x1234u); }

struct Text { const char* label; const char* bytes; unsigned length; };
// The strings (NUL-terminated in the scratch area): ASCII, the bar, controls, characters above 0x7f, wide letters, spaces, a long one.
constexpr Text texts[] = {
    {"hello", "Hello", 5},      {"empty", "", 0},        {"bars", "A|B|C", 5},
    {"controls", "a\x01" "b\x1f" "c\x7f" "d\x80" "e\xff" "z", 11},     {"wide", "WWWWWWWWWWWW", 12},     {"spaces", "   ", 3},
    {"long", "The quick brown fox jumps over the lazy dog.", 44},      {"one", "g", 1},         {"high", "\xe9\xe7\xf1 \xc0\xdf", 6},
    {"phrase", "Hello, World 0123", 17},
};
constexpr unsigned text_stride = 0x80;
std::uint32_t text_address(unsigned k) { return text_at + k * text_stride; }
}

FONT_CPP_CASES("drawer") {
    std::mt19937 random(0x1074f800);
    const auto array = scenario.toc(-0x3bd8);
    if (std::getenv("CM_FONT_CPP_SHOW_STATE")) {
        std::fprintf(stderr, "drawer state: array %x\n", array);
        for (unsigned k = 0; k < 8; ++k) {
            const auto record = scenario.r32(array + 4 * k);
            std::fprintf(stderr, " slot %u: record %x loaded %u scaler %x size %u style %x\n", k, record, record ? scenario.u8(record) : 0,
                         record ? scenario.r32(record + 0x14) : 0, record ? scenario.u8(record + 0x1a) : 0, record ? scenario.u16(record + 0x18) : 0);
        }
    }
    const auto stack_callback = scenario.sp() + 0x38;

    // The window and the surface, and the strings: the memory each case writes.
    const auto frame = [&](std::uint32_t width, std::uint32_t height, std::uint32_t stride, std::uint32_t buffer = surface_at) {
        std::vector<Write> memory{w32(window_at, buffer), w32(window_at + 4, stride), w16(window_at + 0xa, width), w16(window_at + 0xc, height)};
        return memory;
    };
    static std::vector<Write> filled = [&] {
        std::vector<Write> memory;
        for (unsigned k = 0; k < surface_pixels; ++k) memory.push_back(w16(surface_at + 2 * k, background(k)));
        for (unsigned k = 0; k < std::size(texts); ++k) {
            for (unsigned n = 0; n <= texts[k].length; ++n) memory.push_back(w8(text_address(k) + n, std::uint8_t(texts[k].bytes[n])));
        }
        return memory;
    }();
    // Whether the surface changed (a drawing case must paint something), checked on the final state of both runs.
    const auto painted = [](State& s) {
        for (unsigned k = 0; k < surface_pixels; ++k)
            if (std::uint16_t(s.memory.read<2>(surface_at + 2 * k)) != background(k)) return true;
        return false;
    };

    // The fields are in the order the designated initializers below use.
    struct Draw {
        const char* what = "";
        std::uint32_t slot = 0, size = 9, flags = 0x200, x = 4, y = 3, colour = 0x7fff, text = 0;
        std::uint32_t width = 96, height = 48, stride = 96, callback = 0;
        bool expect_paint = true;
    };
    const auto make = [&](const Draw& d) {
        Case item{.entry = drawer};
        item.registers = {{3, array + 4 * d.slot}, {4, window_at}, {5, d.x}, {6, d.y}, {7, d.flags}, {8, d.size}, {9, d.colour}, {10, text_address(d.text)}};
        item.memory = filled;
        append(item.memory, frame(d.width, d.height, d.stride));
        item.memory.push_back(w32(stack_callback, d.callback));
        item.label = std::string("drawer: ") + d.what + " " + texts[d.text].label + " slot " + std::to_string(d.slot) + " size " + std::to_string(d.size) + " flags " +
                     hex(d.flags) + " at " + std::to_string(std::int16_t(d.x)) + "," + std::to_string(std::int16_t(d.y)) + " in " + std::to_string(d.width) + "x" +
                     std::to_string(d.height) + (d.callback ? " callback" : "");
        if (d.expect_paint) item.verify = painted;
        return item;
    };
    const auto first_case = cases.size();

    // ---- Rare sites first (the call trace pairs only the first cases of an entry): the recursion with the callback, the callback glue
    // answered to skip, the assertions answered, the text's end with and without the callback.
    cases.push_back(make({.what = "outline", .slot = 1, .size = 14, .flags = 0x280, .callback = callback_descriptor}));
    {
        auto item = make({.what = "outline, callback skips", .slot = 0, .flags = 0x280, .callback = callback_descriptor});
        // The answered glue writes no TOC save slot (0x14 of the frame), which the original reloads r2 from: poke the real TOC into the
        // slot of the frame and of the recursion's below it.
        item.answers = {{glue, 1}};
        item.pokes = {{glue, {w32(scenario.sp() - 0xa0 + 0x14, 0x108bba90), w32(scenario.sp() - 0x140 + 0x14, 0x108bba90)}}};
        item.verify = nullptr;
        cases.push_back(item);
        auto some = make({.what = "callback skips the second glyph", .slot = 1, .size = 14, .callback = callback_descriptor});
        some.nth_answers = {{glue, 2, 1}};
        cases.push_back(some);
        auto last = make({.what = "callback skips the last", .slot = 1, .size = 14, .callback = callback_descriptor});
        last.nth_answers = {{glue, 5, 1}};
        cases.push_back(last);
    }
    cases.push_back(make({.what = "empty text, callback", .slot = 1, .size = 14, .text = 1, .callback = callback_descriptor, .expect_paint = false}));
    cases.push_back(make({.what = "empty text", .slot = 1, .size = 14, .text = 1, .expect_paint = false}));
    cases.push_back(make({.what = "font not loaded", .slot = 4, .size = 14, .expect_paint = false}));
    cases.push_back(make({.what = "font not loaded, callback", .slot = 4, .size = 14, .flags = 0x280, .callback = callback_descriptor, .expect_paint = false}));
    {
        // The assertions: the reporter is answered (it does not return in the game), after which the original goes on with the null
        // pointer: page 0 is read, which both sides fault on unless the checker maps it (--null-page).
        auto window_null = make({.what = "window null", .expect_paint = false});
        window_null.registers[1] = {4, 0};
        window_null.answers = {{report, 0}};
        if (std::getenv("CM_FONT_CPP_NULL_PAGE")) cases.push_back(window_null);
        auto buffer_null = make({.what = "buffer null", .expect_paint = false});
        append(buffer_null.memory, {w32(window_at, 0)});
        buffer_null.answers = {{report, 0}};
        cases.push_back(buffer_null);
        auto text_null = make({.what = "text null", .expect_paint = false});
        text_null.registers[7] = {10, 0};
        text_null.answers = {{report, 0}};
        if (std::getenv("CM_FONT_CPP_NULL_PAGE")) cases.push_back(text_null);
        auto font_null = make({.what = "font record null", .expect_paint = false});
        font_null.memory.push_back(w32(slot_at, 0));
        font_null.registers[0] = {3, slot_at};
        font_null.answers = {{report, 0}};
        if (std::getenv("CM_FONT_CPP_NULL_PAGE")) cases.push_back(font_null);
        auto all_null = make({.what = "all null", .expect_paint = false});
        all_null.registers[0] = {3, slot_at};
        all_null.registers[1] = {4, 0};
        all_null.registers[7] = {10, 0};
        all_null.memory.push_back(w32(slot_at, 0));
        all_null.answers = {{report, 0}};
        if (std::getenv("CM_FONT_CPP_NULL_PAGE")) cases.push_back(all_null);
    }

    // ---- The three pixel loops, in each loaded font, with and without the callback, and the mirrored read, the bar flag and the
    // outline: every combination of the style bits the drawer tests, for each font.
    const std::uint32_t fonts[][2] = {{0, 9}, {1, 14}, {3, 20}, {2, 16}};
    for (const auto& font : fonts)
        for (const std::uint32_t flags : {0x200u, 0x280u, 0u, 0x20u, 0x40u, 0x60u, 0x100u, 0x120u, 0x140u, 0x160u, 0x220u, 0x240u, 0x2a0u, 0x2c0u, 0x300u, 0x380u, 0x10200u, 0x10280u})
            for (const bool with_callback : {false, true}) {
                if (with_callback && flags != 0x200 && flags != 0x280 && flags != 0x20 && flags != 0x300) continue;
                cases.push_back(make({.what = "style", .slot = font[0], .size = font[1], .flags = flags, .callback = with_callback ? callback_descriptor : 0,
                                     .expect_paint = font[0] != 0}));  // slot 0 (small, size 9) paints nothing for these texts: compared all the same
            }
    // The other texts, once each in the medium font, with each of the three loops.
    for (unsigned t = 0; t < std::size(texts); ++t)
        for (const std::uint32_t flags : {0x200u, 0x220u, 0x240u, 0x300u, 0x2a0u})
            cases.push_back(make({.what = "text", .slot = 1, .size = 14, .flags = flags, .text = t, .callback = (t % 2) ? callback_descriptor : 0, .expect_paint = false}));
    // Other sizes in a loaded font (the selector re-makes the scaler), other styles (the glyph cache renders).
    for (const std::uint32_t size : {8u, 10u, 12u, 16u, 24u, 0u, 255u})
        cases.push_back(make({.what = "size", .slot = 1, .size = size, .expect_paint = false}));
    for (const std::uint32_t flags : {0x201u, 0x202u, 0x203u, 0x204u, 0x208u, 0x20cu, 0x210u, 0x214u, 0x230u, 0x3ffu}) {
        cases.push_back(make({.what = "glyph style", .slot = 1, .size = 14, .flags = flags, .expect_paint = false}));
    }

    // ---- The clipping: positions around each edge of a small window, and windows of every small shape.
    for (const std::int32_t x : {-300, -40, -9, -4, -1, 0, 1, 10, 20, 30, 38, 39, 40, 41, 60})
        for (const std::int32_t y : {-40, -12, -6, -2, 0, 3, 10, 16, 18, 19, 20, 24, 40}) {
            Draw d{.what = "clip", .size = 9, .flags = ((x + y) & 1) ? 0x220u : 0x200u, .x = std::uint32_t(x) & 0xffff, .y = std::uint32_t(y) & 0xffff, .text = unsigned(x + y + 400) % 3, .width = 40, .height = 20, .stride = 96, .expect_paint = false};
            if (std::abs(x - 10) % 3 == 1) d.flags = 0x240;
            if (std::abs(y) % 4 == 3) d.flags |= 0x100;
            d.slot = std::abs(x) % 2 ? 0 : 1, d.size = d.slot ? 9 : 14;
            d.slot = d.slot ? 0 : 1;
            cases.push_back(make(d));
        }
    for (const std::uint32_t width : {0u, 1u, 5u, 12u, 0x7fffu, 0x8000u, 0xffffu})
        for (const std::uint32_t height : {0u, 1u, 6u, 0x7fffu, 0xffffu})
            cases.push_back(make({.what = "window shape", .width = width, .height = height, .stride = 96, .expect_paint = false}));
    for (const std::uint32_t stride : {0u, 1u, 7u, 30u, 200u, 0xffffffffu})
        cases.push_back(make({.what = "stride", .flags = 0x220, .width = 30, .height = 20, .stride = stride, .expect_paint = false}));
    // Positions given as the full words the callers pass (the halfword is what counts).
    cases.push_back(make({.what = "wide position", .slot = 1, .size = 14, .x = 0x12340005, .y = 0xabcd0004}));
    cases.push_back(make({.what = "position -1,-1", .flags = 0x220, .x = 0xffff, .y = 0xffff, .expect_paint = false}));

    // ---- Random combinations.
    for (unsigned n = 0; n < 120; ++n) {
        const auto& font = fonts[random() % 3];
        static const std::uint32_t styles[] = {0x200, 0x280, 0x20, 0x40, 0x60, 0x100, 0x120, 0x140, 0x300, 0x2a0, 0x2e0, 0, 0x201, 0x210};
        Draw d{.what = "random", .slot = font[0], .size = font[1], .flags = styles[random() % std::size(styles)], .x = std::uint32_t(std::int32_t(random() % 100) - 20) & 0xffff, .y = std::uint32_t(std::int32_t(random() % 60) - 12) & 0xffff, .colour = std::uint32_t(random() & 0xffff), .text = unsigned(random() % std::size(texts)), .width = 20 + random() % 60, .height = 10 + random() % 36, .stride = 0, .callback = (random() % 3) ? 0 : callback_descriptor, .expect_paint = false};
        d.stride = d.width + random() % 20;
        cases.push_back(make(d));
    }

    // ---- Synthetic glyphs: the glyph cache answered with a record built in the scratch area (the real layout: left and top in 1/64 pixel,
    // width, row bytes, height, the bitmap's address, the advance in 16.16), so that the clipping exits, glyphs of no size, every
    // coverage byte (the original's weight 126 - alpha is negative above 126), the mirrored read and the neighbour pixel of the embossed
    // loop are all reached, which the engine's glyphs (coverage up to 126, normal shapes) do not. Every character gets the same record.
    for (unsigned n = 0; n < 420; ++n) {
        static const std::uint32_t styles[] = {0x200, 0x20, 0x40, 0x100, 0x120, 0x140, 0, 0x220, 0x240, 0x280, 0x2a0, 0x300, 0x10020};
        const auto width = unsigned(random() % 24), height = unsigned(random() % 24);
        const auto row_bytes = width + random() % 4;
        const std::int32_t left = std::int32_t(random() % 1500) - 500, top = std::int32_t(random() % 1500) - 500;
        const std::uint32_t advance = (random() % 14) << 16 | (random() & 0xffff) * (random() % 2);
        Draw d{.what = "synthetic glyph", .slot = 1, .size = 14, .flags = styles[random() % std::size(styles)],
               .x = std::uint32_t(std::int32_t(random() % 120) - 30) & 0xffff, .y = std::uint32_t(std::int32_t(random() % 70) - 15) & 0xffff,
               .colour = std::uint32_t(random() & 0xffff), .text = (n % 3 == 0) ? 2u : unsigned(random() % 3 ? 0 : 4), .width = 10 + random() % 70,
               .height = 6 + random() % 40, .stride = 0, .callback = (random() % 4) ? 0 : callback_descriptor, .expect_paint = false};
        d.stride = d.width + random() % 20;
        auto item = make(d);
        item.label += " glyph " + std::to_string(width) + "x" + std::to_string(height) + " row " + std::to_string(row_bytes) + " at " + std::to_string(left) + "," + std::to_string(top) + " #" + std::to_string(n);
        append(item.memory, {w32(glyph_at, std::uint32_t(left)), w32(glyph_at + 4, std::uint32_t(top)), w32(glyph_at + 8, width), w32(glyph_at + 0xc, row_bytes),
                             w32(glyph_at + 0x10, height), w32(glyph_at + 0x14, bitmap_at), w32(glyph_at + 0x18, advance)});
        const auto high = random() % 3 == 0;  // a third of the glyphs have coverage above 126
        for (unsigned k = 0; k < height * row_bytes + 4; ++k) {
            const auto pick = random() % 10;
            item.memory.push_back(w8(bitmap_at + k, pick < 4 ? 0 : pick < 7 ? 1 + random() % 126 : high ? 127 + random() % 129 : 126));
        }
        item.answers = {{0x10750580, glyph_at}};
        cases.push_back(item);
    }
    // Saturated glyphs on a white surface with a black ink: the coverage 255 gives the mix a negative weight and a negative result (the sign
    // bit the loops leave in r5 or r7), for each loop, each reading direction and with the neighbour pixel of the embossed loop.
    for (const std::uint32_t flags : {0x200u, 0x20u, 0x40u, 0x100u, 0x120u, 0x140u})
        for (const std::uint32_t coverage : {255u, 200u, 127u, 126u}) {
            Draw d{.what = "saturated glyph", .slot = 1, .size = 14, .flags = flags, .x = 6, .y = 5, .colour = 0, .text = 7, .width = 40, .height = 30, .stride = 40, .expect_paint = false};
            auto item = make(d);
            item.label += " coverage " + std::to_string(coverage);
            for (unsigned k = 0; k < 40 * 30; ++k) item.memory.push_back(w16(surface_at + 2 * k, 0x7fff));
            append(item.memory, {w32(glyph_at, 0), w32(glyph_at + 4, 0), w32(glyph_at + 8, 4), w32(glyph_at + 0xc, 4), w32(glyph_at + 0x10, 3), w32(glyph_at + 0x14, bitmap_at),
                                 w32(glyph_at + 0x18, 5 << 16)});
            for (unsigned k = 0; k < 12 + 4; ++k) item.memory.push_back(w8(bitmap_at + k, coverage));
            item.answers = {{0x10750580, glyph_at}};
            cases.push_back(item);
        }
    // A text of 32,770 control characters: the character index is a halfword (`extsh`), so at 32768 it reads the byte 32768 before the text
    // (a zero here: the end), where an index that did not wrap would read on.
    {
        auto item = make({.what = "text longer than 32767", .slot = 1, .size = 14, .callback = callback_descriptor, .expect_paint = false});
        item.registers[7] = {10, long_text_at};
        item.label += " (32770 characters)";
        for (std::uint32_t k = 0; k < 32770; ++k) item.memory.push_back(w8(long_text_at + k, 1 + k % 31));
        item.memory.push_back(w8(long_text_at + 32770, 0));
        item.memory.push_back(w8(long_text_at - 32768, 0));
        cases.push_back(item);
    }
    (void)first_case;
}
}
