// graph_carbon.cpp, block `box` (0x10768810 and 0x10769b60): cases for the assertion box.
//
// 0x10768810 is a getter (r3 only). 0x10769b60 runs for real on both sides (Case::real_dialog) with its callees ANSWERED (the checker's dispatcher
// answers them at their dispatch, the same on both sides, so each case compares what the box does itself: the arguments of every call and their
// order (the call trace), the registers it leaves, its frame and its stores on the dead stack (the entry is compared on the exact stack), and the
// re-entrancy bytes): the text width (the layout), the surface capture, the cursor getter, the localized captions, the string copy, the framed
// rectangle, the text box, the flush, the cursor setter, the blit and the free. The event pump 0x1076fe80 is answered too, and it is the script:
// each dispatch of it (a poke with an nth) writes the next state of MAIN's mouse position, button byte and key code, which the loop reads. MAIN's
// exit 0x1076f7d0 is the checker's own (it ends the run: what the box wrote up to the call is compared, with the registers it holds).
//
// The cases cover: the re-entrancy bytes and the "show dialogs" byte in every combination, the printf path, the panel's width (the text width
// above, below and at the limits, wrapping values), the Quit button offered or not (every low byte of the flag), and the hot-button state machine
// of OK and Quit (press, hold, release inside, release outside, move while pressed, the edges of both rectangles, the return key, other keys).
// A last group runs the real width, capture, text, frame, blit and free callees on a surface the case builds in scratch memory (the pump, the
// flush and the cursor setter stay answered: they reach the Toolbox).
// Labels start with "box:".
#include "graph_carbon_checks.hpp"
#include "graph_carbon_text_detail.hpp"
#include <string>

namespace cm::aot::graph_carbon_checks {
namespace {
using namespace cm::aot::graph_carbon;
constexpr std::uint32_t game_toc = 0x108bba90;
constexpr std::uint32_t box_entry = 0x10769b60, version_entry = 0x10768810;
// The callees of the box that the scripted cases answer (printf runs for real: its C++ owner runs in place on the original's page).
constexpr std::uint32_t text_width = 0x1076d1e0, capture = 0x1076b020, current_cursor = 0x1076e910, localized = 0x10727890,
    copy_string = 0x1077dbf0, framed = 0x1076d6e0, text_box = 0x1076e160, flush = 0x107692f0, pump = 0x1076fe80, set_cursor = 0x1076e920,
    blit = 0x1076b140, free_bitmap = 0x1076b7e0;
constexpr std::uint32_t message_at = scratch_base + 0x20000, detail_at = scratch_base + 0x20100;
constexpr std::uint32_t slot(std::int32_t offset) { return game_toc + std::uint32_t(offset); }
constexpr std::uint32_t key_return = 0x24;

std::vector<std::pair<unsigned, std::uint32_t>> registers(std::initializer_list<std::pair<unsigned, std::uint32_t>> used) {
    std::vector<std::pair<unsigned, std::uint32_t>> all;
    for (unsigned n = 3; n <= 10; ++n) all.emplace_back(n, 0x7a000000u + n * 0x01010101u);
    for (const auto& [n, value] : used) all[n - 3].second = value;
    return all;
}
std::vector<Write> text(std::uint32_t at, const std::string& value) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < value.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), std::uint8_t(value[n])));
    memory.push_back(w8(at + std::uint32_t(value.size()), 0));
    return memory;
}
// The panel's geometry as the listing says it (an independent statement of the layout, not the C++'s): the width is the first text's width + 0x18, at
// least 200 and at most 600, centred on 800; the OK button sits at the centre (flag clear) or left of it (Quit offered), Quit 0x55 to its right.
struct Layout {
    std::int32_t width, left, right, ok_left, ok_right, quit_left, quit_right;
    static constexpr std::int32_t top = 0x13e, bottom = 0x156;
};
// The canvas the panel is centred on: the original's 800, or the checker's (CM_GUI_CANVAS=WxH, which the hook of 0x10769c40 reads: graph_carbon_checks.cpp).
std::int32_t canvas_width() {
    static const std::int32_t width = [] {
        unsigned w = 800, h = 600;
        if (const char* text = std::getenv("CM_GUI_CANVAS")) std::sscanf(text, "%ux%u", &w, &h);
        return std::int32_t(w < 800 ? 800 : w > 3200 ? 3200 : w);
    }();
    return width;
}
Layout layout(std::uint32_t answer, bool quit) {
    const auto measured = std::int32_t(answer + 0x18);
    const std::int32_t width = measured <= 200 ? 200 : measured >= 600 ? 600 : measured;
    const std::int32_t left = (canvas_width() - width) / 2;
    const std::int32_t ok_left = left + (width - (quit ? 0x8c : 0x4b)) / 2;
    return {width, left, left + width - 1, ok_left, ok_left + 0x4a, ok_left + 0x55, ok_left + 0x9f};
}
// One state of the mouse and keyboard as the pump leaves it.
struct Step { std::int32_t x, y; std::uint32_t button, key; };
}


// The real-callee group: rows 242-357 of a canvas-wide (800 pixels) 15-bit surface in scratch memory (the panel's rows), the surface variables pointing at it
// (the base is chosen so that row 242 starts at `rows_at`; the rest of the surface is never touched), filled with a pattern a restored panel must show again.
// The rows are as wide as the checker's canvas (CM_GUI_CANVAS, at most 1366 pixels: 116 rows of it fit below the imports log).
constexpr std::uint32_t rows_at = scratch_base + 0x30000, first_row = 242, row_count = 116;
std::uint32_t row_width() { return std::uint32_t(canvas_width()); }
constexpr std::uint32_t pattern(std::uint32_t x, std::uint32_t y) { return (x * 7 + y * 13 + (x >> 3) * 5) & 0x7fff; }
void build_rows(State& s) {
    s.memory.write<4>(slot(text_small::surface), rows_at - first_row * row_width() * 2);
    s.memory.write<2>(slot(text_small::stride), row_width());
    s.memory.write<2>(slot(text_small::row_bytes), row_width() * 2);
    for (std::uint32_t y = 0; y < row_count; ++y)
        for (std::uint32_t x = 0; x < row_width(); ++x) s.memory.write<2>(rows_at + (y * row_width() + x) * 2, pattern(x, first_row + y));
}
bool rows_restored(State& s) {
    for (std::uint32_t y = 0; y < row_count; ++y)
        for (std::uint32_t x = 0; x < row_width(); ++x)
            if (s.memory.read<2>(rows_at + (y * row_width() + x) * 2) != pattern(x, first_row + y)) return false;
    return true;
}

GRAPH_CARBON_CASES("box") {
    const auto mouse_x = scenario.toc(-0x5c10), mouse_y = scenario.toc(-0x5c14), button = scenario.toc(-0x7bac), key = scenario.toc(-0x5c1c);
    const auto show_dialogs = scenario.toc(-0x7418);
    const std::uint32_t has_run = slot(-0x3570), showing = slot(-0x356f), initialising = slot(-0x3550);

    // 0x10768810: the version text's address, whatever the other registers hold.
    cases.push_back({.entry = version_entry, .registers = registers({}), .label = "box: version text"});
    cases.push_back({.entry = version_entry, .registers = registers({{3, 0}, {4, 0xffffffffu}}), .label = "box: version text, other registers"});

    const std::vector<std::pair<std::uint32_t, std::uint32_t>> all_answers = {
        {text_width, 0x100}, {capture, 0x72}, {current_cursor, 3}, {localized, 0x74}, {copy_string, 0x75}, {framed, 0x76}, {text_box, 0x77},
        {flush, 0x78}, {pump, 0x79}, {set_cursor, 0x7a}, {blit, 0x7b}, {free_bitmap, 0x7c}};
    const auto answers_with = [&](std::uint32_t width_answer, std::uint32_t cursor = 3) {
        auto answers = all_answers;
        for (auto& [entry, value] : answers) {
            if (entry == text_width) value = width_answer;
            if (entry == current_cursor) value = cursor;
        }
        return answers;
    };
    // A box case: the texts in scratch memory, the bytes it reads set, the pump's script as pokes (step n at the nth dispatch).
    const auto make = [&](std::string label, std::uint32_t flag, std::uint32_t width_answer, std::vector<Step> script, std::vector<Write> extra = {}) {
        Case item{.entry = box_entry, .registers = registers({{3, message_at}, {4, detail_at}, {5, flag}}), .label = "box: " + std::move(label)};
        item.real_dialog = true;
        item.answers = answers_with(width_answer);
        append(item.memory, text(message_at, "First text"));
        append(item.memory, text(detail_at, "Second text"));
        append(item.memory, {w8(show_dialogs, 1), w8(initialising, 0), w8(has_run, 1), w8(showing, 0), w32(key, 0), w8(button, 0), w32(mouse_x, 0), w32(mouse_y, 0)});
        append(item.memory, extra);
        for (std::size_t n = 0; n < script.size(); ++n)
            item.pokes.push_back({pump, {w32(mouse_x, std::uint32_t(script[n].x)), w32(mouse_y, std::uint32_t(script[n].y)), w8(button, script[n].button),
                                         w32(key, script[n].key)}, unsigned(n + 1)});
        return item;
    };
    const Step end_key{0, 0, 0, key_return};

    // ---- The guards: re-entrancy bytes (has run / showing), "show dialogs", "initialising". Rare paths first: the first run clears "showing" and sets
    // "has run"; a showing box returns at once; printf when dialogs are off or the graphics are being created.
    for (const std::uint32_t run : {0u, 1u, 0x80u, 0xffu, 0x100u})
        for (const std::uint32_t show : {0u, 1u, 0xffu, 0x80u})
            for (const std::uint32_t init : {0u, 1u, 0x80u})
                for (const std::uint32_t shown : {0u, 1u, 0x80u}) {
                    if ((run != 0 && run != 1 && run != 0x80) && (shown == 0x80 || init == 0x80)) continue;
                    auto item = make("guards has_run " + hex(run) + " show " + hex(show) + " init " + hex(init) + " showing " + hex(shown), 1, 0x100, {end_key},
                                     {w8(has_run, run), w8(show_dialogs, show), w8(initialising, init), w8(showing, shown)});
                    cases.push_back(item);
                }
    // The printf path: the texts, with the flag set and clear and unusual pointers (the text pointers are only formatted).
    for (const std::uint32_t flag : {0u, 1u})
        for (const std::uint32_t texts : {0u, 1u, 2u}) {
            auto item = make("printf path flag " + hex(flag) + " texts " + std::to_string(texts), flag, 0x100, {}, {w8(initialising, 1)});
            item.registers = registers({{3, texts == 0 ? 0u : message_at}, {4, texts == 2 ? message_at : detail_at}, {5, flag}});
            cases.push_back(item);
        }
    cases.push_back(make("printf path, dialogs off", 0, 0x100, {}, {w8(show_dialogs, 0)}));

    // ---- The width: measured + 0x18 below, at and above 200 and 600; wrap-around values. The flag's low byte decides about Quit: only the low
    // byte counts (0x100 is clear, 0x80 and 0xff are set), the rest of the register is ignored.
    for (const std::uint32_t answer : {0u, 0x28u, 0x7fu, 0xafu, 0xb0u, 0xb1u, 0x100u, 0x1e8u, 0x23fu, 0x240u, 0x241u, 0x1000u, 0x7fffffe8u, 0xffffffffu, 0x80000000u,
                                       0xffffffe8u, 0x7fffffffu})
        for (const std::uint32_t flag : {0u, 1u, 0x100u, 0x80u, 0xffffff00u, 0x12345601u, 0xffffffffu})
            cases.push_back(make("width answer " + hex(answer) + " flag " + hex(flag), flag, answer, {end_key}));

    // ---- The return key alone (without any mouse): other keys first, then the return key; a nonzero key that is not 0x24 keeps the loop going.
    cases.push_back(make("return key at once", 1, 0x100, {end_key}));
    cases.push_back(make("other keys then return", 1, 0x100, {{0, 0, 0, 0x23}, {0, 0, 0, 0x25}, {0, 0, 0, 0x2400}, {0, 0, 0, 0xffffffffu}, {0, 0, 0, 0}, end_key}));
    cases.push_back(make("return key, flag clear", 0, 0x100, {{0, 0, 0, 1}, end_key}));

    // ---- The mouse. For each flag (Quit offered or not) and a few widths: press and release over OK, press OK and release outside, hold, move out and
    // release, the same for Quit (a release over Quit leaves the game through the exit: the run ends there), cursor positions, edges.
    for (const std::uint32_t answer : {0x100u, 0u, 0x1e8u})
        for (const std::uint32_t flag : {1u, 0u}) {
            const auto g = layout(answer, flag != 0);
            const std::int32_t ok_x = g.ok_left + 5, quit_x = g.quit_left + 5, y = 0x140;
            const std::string where = " answer " + hex(answer) + " flag " + hex(flag);
            // OK: press and release inside ends the box (the key is not needed).
            cases.push_back(make("ok press release" + where, flag, answer, {{ok_x, y, 0, 0}, {ok_x, y, 1, 0}, {ok_x, y, 0, 0}}));
            // Hold: the button stays down for several pumps, then released.
            cases.push_back(make("ok hold" + where, flag, answer, {{ok_x, y, 1, 0}, {ok_x, y, 1, 0}, {ok_x + 1, y, 1, 0}, {ok_x, y, 0, 0}}));
            // Pressed, then moved out and released outside: cold again, then the return key.
            cases.push_back(make("ok press move out release" + where, flag, answer, {{ok_x, y, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {5, 5, 0, 0}, end_key}));
            // Pressed outside, moved over OK and released: nothing happens (OK was not hot).
            cases.push_back(make("press outside, release over ok" + where, flag, answer, {{0, 0, 1, 0}, {ok_x, y, 1, 0}, {ok_x, y, 0, 0}, end_key}));
            // The return key while the button is down over OK (the key is only looked at when nothing else matches).
            cases.push_back(make("return key during ok press" + where, flag, answer, {{ok_x, y, 1, key_return}, {ok_x, y, 1, key_return}, {ok_x, y, 0, key_return}}));
            cases.push_back(make("return key while ok hot, then outside" + where, flag, answer, {{ok_x, y, 1, 0}, {ok_x, y, 1, key_return}, {0, 0, 1, key_return}, {0, 0, 0, key_return}}));
            // Quit (offered only with the flag; clear: the (-1, -1) box is never consulted, and a press at the place Quit would be does nothing).
            cases.push_back(make("quit press release" + where, flag, answer, {{quit_x, y, 0, 0}, {quit_x, y, 1, 0}, {quit_x, y, 0, 0}, end_key}));
            cases.push_back(make("quit hold" + where, flag, answer, {{quit_x, y, 1, 0}, {quit_x, y, 1, 0}, {quit_x, y, 1, key_return}, {quit_x, y, 0, 0}, end_key}));
            cases.push_back(make("quit press move out release" + where, flag, answer, {{quit_x, y, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, end_key}));
            cases.push_back(make("press outside, release over quit" + where, flag, answer, {{0, 0, 1, 0}, {quit_x, y, 1, 0}, {quit_x, y, 0, 0}, end_key}));
            // OK hot and Quit pressed at once is impossible (the rectangles are apart); an OK press followed by a Quit press after release outside.
            cases.push_back(make("ok then quit" + where, flag, answer, {{ok_x, y, 1, 0}, {0, 0, 0, 0}, {quit_x, y, 1, 0}, {quit_x, y, 0, 0}, end_key}));
            // The mouse at (-1, -1) pressed, with the flag clear: the absent Quit box is (-1, -1, -1, -1).
            cases.push_back(make("mouse at minus one" + where, flag, answer, {{-1, -1, 1, 0}, {-1, -1, 1, 0}, {-1, -1, 0, 0}, end_key}));
            // The edges of both rectangles (inclusive): every corner and the pixel just outside it, pressed and released (flag set, the default width).
            if (answer == 0x100 && flag == 1)
                for (unsigned which = 0; which < 2; ++which)
                    for (const std::int32_t px : {(which ? g.quit_left : g.ok_left) - 1, which ? g.quit_left : g.ok_left, which ? g.quit_right : g.ok_right,
                                                  (which ? g.quit_right : g.ok_right) + 1})
                        for (const std::int32_t py : {Layout::top - 1, Layout::top, Layout::bottom, Layout::bottom + 1})
                            cases.push_back(make(std::string(which ? "quit" : "ok") + " edge " + std::to_string(px) + "," + std::to_string(py), flag, answer,
                                                 {{px, py, 0, 0}, {px, py, 1, 0}, {px, py, 0, 0}, end_key}));
        }
    // Negative and huge mouse coordinates (signed compares).
    for (const std::int32_t x : {-5, 0x7fffffff, std::int32_t(0x80000000), 300, 799, 800})
        cases.push_back(make("mouse x " + std::to_string(x), 1, 0x100, {{x, 0x140, 1, 0}, {x, 0x140, 0, 0}, end_key}));

    // ---- The saved cursor and bitmap flow through the frame: other answers of the cursor getter, the capture and the localizer reach the later calls.
    for (const std::uint32_t cursor : {0u, 3u, 0xffu, 0x100u, 0x80u, 0xfffffffeu}) {
        auto item = make("cursor answer " + hex(cursor), 1, 0x100, {end_key});
        item.answers = answers_with(0x100, cursor);
        cases.push_back(item);
    }
    for (const std::uint32_t bitmap : {0u, 0x20000040u, 0xffffffffu}) {
        auto item = make("capture answer " + hex(bitmap), 0, 0x100, {end_key});
        for (auto& [entry, value] : item.answers) if (entry == capture) value = bitmap;
        cases.push_back(item);
    }

    // ---- Real callees: the text width, the capture, the cursor getter, the localizer, the string copy, the framed rectangles, the text boxes, the blit and the
    // free run for real on both sides (the pump, the flush and the cursor setter reach the Toolbox and stay answered), on the rows built above. The box saves
    // the surface under it, draws, waits for the return key, puts the saved surface back: afterwards every pixel is the pattern again (an expectation of its
    // own: the two sides could agree and both leave the panel on the screen).
    for (const std::uint32_t flag : {1u, 0u})
        for (const std::uint32_t pumps : {1u, 3u}) {
            std::vector<Step> script(pumps, Step{0, 0, 0, 0x23});
            script.back() = end_key;
            auto item = make("real callees flag " + hex(flag) + " pumps " + std::to_string(pumps), flag, 0x100, script);
            item.answers = {{flush, 0x78}, {pump, 0x79}, {set_cursor, 0x7a}};
            item.prepare = build_rows;
            item.verify = rows_restored;
            cases.push_back(item);
        }

    // Real drawing callees with the layout known: only the text width, the flush, the pump and the cursor setter are answered, so the framed rectangles and
    // the text boxes of a hot (pressed) button run for real below the box's frame (their saved registers include the box's held r16 / r17) on the rows.
    {
        const auto g = layout(0x100, true);
        const std::int32_t ok_x = g.ok_left + 5, quit_x = g.quit_left + 5, y = 0x140;
        const std::vector<std::pair<std::string, std::vector<Step>>> scripts = {
            {"ok hot then released inside", {{ok_x, y, 1, 0}, {ok_x, y, 1, 0}, {ok_x, y, 0, 0}}},
            {"ok hot then released outside", {{ok_x, y, 1, 0}, {ok_x, y, 1, 0}, {0, 0, 0, 0}, end_key}},
            {"quit hot then released outside", {{quit_x, y, 1, 0}, {quit_x, y, 1, 0}, {0, 0, 0, 0}, end_key}},
            {"quit hot then released inside (exit)", {{quit_x, y, 1, 0}, {quit_x, y, 1, 0}, {quit_x, y, 0, 0}}},
        };
        for (const auto& [name, script] : scripts) {
            auto item = make("real drawing, " + name, 1, 0x100, script);
            item.answers = {{text_width, 0x100}, {flush, 0x78}, {pump, 0x79}, {set_cursor, 0x7a}};
            item.prepare = build_rows;
            if (name.find("exit") == std::string::npos) item.verify = rows_restored;
            cases.push_back(item);
        }
    }
}
}
