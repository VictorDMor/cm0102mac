// DISPLAY.CPP checker cases for the "drawing" block (display_drawing.cpp, display_drawing_drag.cpp).
// The real career's two display tables are empty (no areas, no controls), so each case writes the rows it
// needs into the real tables (memory applied to both sides and undone afterwards).
#include "display_checks.hpp"
#include <string>

namespace cm::aot::display_checks {
namespace {
constexpr std::uint32_t paint_first = 0x10718f60, draw_screen = 0x10718fc0, screen_step = 0x10719350,
    scroll_bar_drag = 0x1071add0, area_drag = 0x1071afc0, update_blinking = 0x10725770;
// Other modules' routines a case answers (drawing to a missing window, image files, the clock).
constexpr std::uint32_t title_of = 0x1073edb0, clock_ms = 0x10746a30, load_image = 0x1076b2f0, make_image = 0x1076b020;

struct Rows {
    std::vector<Write> memory;
    void b(std::uint32_t at, std::uint32_t v) { memory.push_back({1, at, v}); }
    void h(std::uint32_t at, std::uint32_t v) { memory.push_back({2, at, v}); }
    void w(std::uint32_t at, std::uint32_t v) { memory.push_back({4, at, v}); }
    void text(std::uint32_t at, const std::string& s) {
        for (std::size_t n = 0; n <= s.size(); ++n) b(at + std::uint32_t(n), n < s.size() ? std::uint8_t(s[n]) : 0);
    }
};
// Field offsets of a display table, as the block names them.
namespace field {
constexpr std::uint32_t area_order = 0x129da8, guio_order = 0x129f9c, guio_count = 0x129da6, area_count = 0x12a8fc,
    guio_order_count = 0x12a8fe, painted_once = 0x12a900, key_expected = 0x12a901, key_result = 0x12a902,
    cancel_result = 0x12a904, enter_result = 0x12a906, last_x = 0x12a90c, last_y = 0x12a910, left = 0x12a914,
    top = 0x12a918, right = 0x12a91c, bottom = 0x12a920, saved_bits = 0x12a924, background = 0x12a928,
    drag_area = 0x12a932, drag_guio = 0x12a934, clear_flag = 0x12a938, skip_draw = 0x12a93e, drawn = 0x12a940,
    background_name = 0x12a946;
}
std::uint32_t area_row(std::uint32_t table, unsigned index) { return table + index * 0xbf0; }
std::uint32_t guio_at(std::uint32_t table, unsigned index) { return table + index * 0x17c + 0xba864; }
constexpr std::uint32_t title_string = scratch_base + 0x1000, empty_string = scratch_base + 0x1100;
}

DISPLAY_CASES("drawing") {
    if (scenario.match) return;
    const auto game = scenario.toc(-0x7bb0);
    const std::uint32_t tables[] = {game + 0x2c68, game + 0x12d6b0};
    const auto read32 = [&](std::uint32_t at) { return scenario.r32(at); };
    const auto add = [&](Case c) { cases.push_back(std::move(c)); };

    // The unit's pointer and flag bytes behind TOC words (the real memory each points at).
    const std::uint32_t key_flag = scenario.toc(-0x74b0), cancel_flag = scenario.toc(-0x744c), enter_flag = scenario.toc(-0x5c00),
        pointer_x = scenario.toc(-0x5c10), pointer_y = scenario.toc(-0x5c14), button_gui = scenario.toc(-0x5c0c),
        shared_flag = scenario.toc(-0x7bac);
    // The graphics routines that cannot draw in a snapshot (no window), answered alike on both sides.
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> graphics{
        {0x107692f0, 0}, {0x1076b140, 0}, {0x1076b7e0, 0}, {0x1076a580, 0}, {0x1076ad50, 0}, {0x1076b020, 0x70000030},
        {0x10769460, 0}, {0x10769980, 0}, {0x1076e920, 0}, {0x10715110, 0}, {0x10715340, 0},
        {0x10714110, 0}, {0x10714630, 0}, {0x10725ec0, 0}, {0x107268c0, 0}};
    auto with = [&](std::vector<std::pair<std::uint32_t, std::uint32_t>> answers) {
        auto all = graphics; all.insert(all.end(), answers.begin(), answers.end()); return all;
    };

    // 0x10718f60: the first paint happens once per table.
    for (const auto table : tables)
        for (const std::uint32_t painted : {0u, 1u}) {
            Rows r; r.b(table + field::painted_once, painted);
            add({.entry = paint_first, .registers = {{3, table}}, .memory = r.memory,
                 .label = "paint_first, painted " + std::to_string(painted), .answers = graphics,
                 .nth_answers = {{0x10769980, 1, 0}, {0x107692f0, 1, 0}}});
        }

    // Lists for the table: two areas (one with a parent-less hole, one without), one of them hidden, and two controls.
    const auto lists = [&](Rows& r, std::uint32_t table, bool populated) {
        r.h(table + field::area_count, populated ? 3 : 0);
        r.h(table + field::guio_order_count, populated ? 2 : 0);
        if (!populated) return;
        r.h(table + field::area_order, 3); r.h(table + field::area_order + 2, 4); r.h(table + field::area_order + 4, 6);
        r.b(area_row(table, 3) + 0xbf2, 1); r.h(area_row(table, 3) + 0x212, 0xffff);   // drawn before the controls
        r.b(area_row(table, 4) + 0xbf2, 1); r.h(area_row(table, 4) + 0x212, 5);        // drawn after them
        r.b(area_row(table, 6) + 0xbf2, 0); r.h(area_row(table, 6) + 0x212, 0xffff);   // hidden
        r.h(table + field::guio_order, 7); r.h(table + field::guio_order + 2, 9);
    };

    // 0x10718fc0: titles and the background image.
    struct Title { const char* label; const char* name; const char* previous; bool named_game; std::uint32_t first, second; };
    const Title titles[] = {
        {"empty title", "", "Old", false, 0, 0},
        {"empty title, same name", "", "", true, 0, 0},
        {"new title, image found", "Bg", "Old", true, 0x70000010, 0},
        {"new title, fallback image", "Bg", "Old", false, 0, 0x70000020},
        {"new title, no image", "Bg", "Old", true, 0, 0},
        {"same title", "Bg", "Bg", true, 0x70000010, 0},
    };
    for (const auto table : tables)
        for (const bool populated : {false, true})
            for (const auto& title : titles) {
                Rows r; lists(r, table, populated);
                r.text(title_string, title.name); r.text(empty_string, "");
                r.text(table + field::background_name, title.previous);
                r.b(game + 0x25819c, title.named_game);
                // The image the table holds after: the old image is the table's (the first table's is real).
                const auto old = read32(table + field::background);
                add({.entry = draw_screen, .registers = {{3, table}, {4, 10}, {5, 20}, {6, 100}, {7, 200}}, .memory = r.memory,
                     .label = std::string("draw_screen, ") + title.label + (populated ? ", rows" : ", no rows") + (old ? ", old image" : ""),
                     .answers = with({{title_of, title_string}, {make_image, 0x70000030}}),
                     .nth_answers = {{load_image, 1, title.first}, {load_image, 2, title.second}}});
            }
    // The same image answered as the table's own (no change), and as another one.
    for (const auto table : tables) {
        Rows r; r.text(title_string, "Bg"); r.text(table + field::background_name, "Old");
        const auto old = read32(table + field::background);
        add({.entry = draw_screen, .registers = {{3, table}, {4, 10}, {5, 20}, {6, 100}, {7, 200}}, .memory = r.memory,
             .label = "draw_screen, image already held", .answers = with({{title_of, title_string}}),
             .nth_answers = {{load_image, 1, old}, {load_image, 2, 0}}});
    }
    // Flags: skip, drawn, painted and the game's gate, the clear flag, and the rectangle.
    for (const auto table : tables)
        for (const bool populated : {false, true})
            for (const std::uint32_t drawn : {0u, 1u})
                for (const std::uint32_t gate : {0u, 1u, 2u})  // painted/2c61: 0/0, 1/1, 1/0
                    for (const std::uint32_t clear : {0u, 1u}) {
                        Rows r; lists(r, table, populated);
                        r.text(title_string, ""); r.text(table + field::background_name, "");
                        r.b(table + field::drawn, drawn);
                        r.b(table + field::painted_once, gate != 0); r.b(game + 0x2c61, gate == 1);
                        r.b(table + field::clear_flag, clear);
                        add({.entry = draw_screen, .registers = {{3, table}, {4, 10}, {5, 20}, {6, 100}, {7, 200}}, .memory = r.memory,
                             .label = "draw_screen, drawn " + std::to_string(drawn) + ", gate " + std::to_string(gate) + ", clear " +
                                      std::to_string(clear) + (populated ? ", rows" : ", no rows"),
                             .answers = with({{title_of, title_string}})});
                    }
    for (const auto table : tables) {
        Rows r; r.b(table + field::skip_draw, 1);
        add({.entry = draw_screen, .registers = {{3, table}, {4, 10}, {5, 20}, {6, 100}, {7, 200}}, .memory = r.memory,
             .label = "draw_screen, skipped", .answers = with({{title_of, title_string}})});
        for (unsigned position = 0; position < 4; ++position) {
            std::vector<std::pair<unsigned, std::uint32_t>> regs{{3, table}, {4, 10}, {5, 20}, {6, 100}, {7, 200}};
            regs[1 + position].second = 0xffff;  // -1 as a halfword the caller's register holds
            Rows q; q.text(title_string, "");
            add({.entry = draw_screen, .registers = regs, .memory = q.memory,
                 .label = "draw_screen, whole screen by argument " + std::to_string(position), .answers = with({{title_of, title_string}})});
        }
    }

    // 0x10725770: the two timers. The clock is answered; the globals sit at fixed offsets from the TOC.
    const std::uint32_t r2 = 0x108bba90;
    for (const auto table : tables)
        for (const bool populated : {false, true})
            for (const std::uint32_t now : {500u, 1500u, 5000u, 90u})
                for (const std::uint32_t kind : {0u, 1u, 2u}) {
                    // Start times 1000 (slow) and 1400 (fast); the three kinds pick the controls.
                    Rows r;
                    r.b(r2 - 0x35b4, 1); r.w(r2 - 0x35b0, 1000); r.b(r2 - 0x35b9, 1); r.w(r2 - 0x35b8, 1400);
                    r.h(table + field::guio_count, populated ? 3 : 0);
                    for (unsigned n = 0; n < 3; ++n) {
                        const auto control = guio_at(table, n);
                        const std::uint32_t flags = kind == 0 ? 0x800 : kind == 1 ? 0x1000 : 0x1800;
                        r.w(control + 0xc, n == 2 ? 0 : flags);
                        r.text(control + 0x78, n == 0 ? "ab" : "cd");
                        r.h(control + 0x6a, 0x11 + n); r.h(control + 0x70, 0x21 + n);
                    }
                    add({.entry = update_blinking, .registers = {{3, table}}, .memory = r.memory,
                         .label = "update_blinking, now " + std::to_string(now) + ", kind " + std::to_string(kind) +
                                  (populated ? ", controls" : ", none"),
                         .answers = with({{clock_ms, now}})});
                }

    // 0x10719350: pending key results, then the mode's handler.
    for (const auto table : tables) {
        const auto base = [&](Rows& r) {
            r.b(key_flag, 0); r.b(cancel_flag, 0); r.b(enter_flag, 0);
            r.b(table + field::key_expected, 0); r.h(table + field::key_result, 0xffff);
            r.h(table + field::cancel_result, 0xffff); r.h(table + field::enter_result, 0xffff);
            r.b(r2 - 0x35b4, 1); r.w(r2 - 0x35b0, 100); r.b(r2 - 0x35b9, 1); r.w(r2 - 0x35b8, 100);
            r.h(table + field::guio_count, 0);
        };
        const auto step = [&](Rows& r, const std::string& label, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            add({.entry = screen_step, .registers = {{3, table}}, .memory = r.memory, .label = "screen_step, " + label,
                 .answers = with(std::move(answers))});
        };
        // A pending key result: matched, not matched, absent.
        for (const std::uint32_t result : {0x0123u, 0xffffu, 0x8001u})
            for (const std::uint32_t expected : {0u, 5u})
                for (const std::uint32_t pressed : {5u, 6u}) {
                    Rows r; base(r);
                    r.b(table + field::key_expected, expected); r.h(table + field::key_result, result); r.b(key_flag, pressed);
                    step(r, "key " + std::to_string(pressed) + " for " + std::to_string(expected) + ", result " + std::to_string(result),
                         {{clock_ms, 100}});
                }
        // Cancel and enter flags, with and without a result.
        for (const std::uint32_t result : {0xffffu, 0x0042u, 0x8002u})
            for (const bool cancel : {true, false}) {
                Rows r; base(r);
                r.b(cancel ? cancel_flag : enter_flag, 1); r.h(table + (cancel ? field::cancel_result : field::enter_result), result);
                step(r, std::string(cancel ? "cancel" : "enter") + " flag, result " + std::to_string(result), {{clock_ms, 100}});
            }
        {
            Rows r; base(r); r.b(cancel_flag, 1); r.b(enter_flag, 1);
            r.h(table + field::cancel_result, 0xffff); r.h(table + field::enter_result, 0x77);
            step(r, "cancel without result, then enter", {{clock_ms, 100}});
        }
        // The mode: the input block's two handlers, this block's two drags, and an invalid mode.
        for (const std::uint32_t mode : {0u, 1u, 2u, 3u, 4u, 0xffu}) {
            Rows r; base(r);
            r.b(table, mode);
            r.b(r2 - 0x35ba, 7);
            // A drag of the scroll bar (mode 2) and of a rectangle (mode 3): the shared flag is the button.
            r.h(table + field::drag_area, 2); r.h(table + field::drag_guio, 1);
            r.b(shared_flag, 1); r.w(pointer_y, 150); r.w(pointer_x, 160); r.w(table + field::last_y, 140); r.w(table + field::last_x, 150);
            const auto bar = area_row(table, 2) + 0xb80;
            r.w(bar + 0x10, 0); r.w(bar + 0x14, 100); r.w(bar + 0x18, 50); r.w(bar + 0x1c, 10); r.w(bar + 0x20, 5);
            r.w(bar + 0x24, 20); r.w(bar + 0x28, 30); r.w(bar + 0x2c, 0);
            r.w(table + field::left, 100); r.w(table + field::top, 100); r.w(table + field::right, 200); r.w(table + field::bottom, 200);
            r.w(guio_at(table, 1) + 0xc, 0);
            step(r, "mode " + std::to_string(mode), {{clock_ms, 100}});
        }
    }

    // 0x1071add0: dragging the scroll bar of area 2 (its record sits at row + 0xb80).
    for (const auto table : tables) {
        const auto bar = area_row(table, 2) + 0xb80;
        const auto drag = [&](const std::string& label, std::uint32_t pressed, std::uint32_t y, std::uint32_t last_y, std::uint32_t saved,
                              std::uint32_t position = 20, std::uint32_t end = 30, std::uint32_t thumb = 10) {
            Rows r;
            r.b(shared_flag, pressed); r.w(pointer_y, y); r.w(table + field::last_y, last_y); r.h(table + field::drag_area, 2);
            r.w(bar + 0x0, 10); r.w(bar + 0x4, 20); r.w(bar + 0x8, 30); r.w(bar + 0xc, 40);
            r.w(bar + 0x10, 0); r.w(bar + 0x14, 100); r.w(bar + 0x18, 50); r.w(bar + 0x1c, thumb); r.w(bar + 0x20, 5);
            r.w(bar + 0x24, position); r.w(bar + 0x28, end); r.w(bar + 0x2c, saved);
            add({.entry = scroll_bar_drag, .registers = {{3, table}}, .memory = r.memory, .label = "scroll_bar_drag, " + label,
                 .answers = graphics});
        };
        drag("released", 0, 150, 140, 0);
        drag("pressed, no movement", 1, 140, 140, 0);
        drag("released, no movement", 0, 140, 140, 0);
        drag("down by 10", 1, 150, 140, 0);
        drag("up by 10", 1, 130, 140, 0);
        drag("up past the minimum", 1, 90, 140, 0);
        drag("down past the maximum", 1, 340, 140, 0);
        drag("saved image held", 1, 150, 140, 0x70000040);
        drag("blocked at the minimum", 1, 130, 140, 0, 0, 10);
        drag("blocked at the maximum", 1, 150, 140, 0, 90, 100);
        drag("thumb unchanged", 1, 141, 140, 0, 20, 30, 10);
        drag("thumb at its scaled place", 1, 150, 140, 0, 20, 30, 50 * 30 / 96);
    }

    // 0x1071afc0: dragging a control's rectangle, and the release choosing controls under the mouse.
    for (const auto table : tables) {
        struct Drag { const char* label; std::uint32_t x, y, last_x, last_y, saved, button, flags, flags2; };
        const std::uint32_t pressed_gui = 1, shared = 2;  // which button byte reads as down
        const Drag drags[] = {
            {"move", 160, 150, 150, 140, 0x70000040, shared, 0, 0},
            {"move with the control's own button", 160, 150, 150, 140, 0x70000040, pressed_gui, 0x100, 0},
            {"move, framed", 160, 150, 150, 140, 0x70000040, shared, 0, 0x10000},
            {"move, no saved image", 160, 150, 150, 140, 0, shared, 0, 0},
            {"idle with saved image", 150, 140, 150, 140, 0x70000040, shared, 0, 0},
            {"idle without saved image", 150, 140, 150, 140, 0, shared, 0, 0},
            {"past the left edge", 50, 140, 150, 140, 0x70000040, shared, 0, 0},
            {"past the right edge", 900, 140, 150, 140, 0x70000040, shared, 0, 0},
            {"past the top edge", 150, 20, 150, 140, 0x70000040, shared, 0, 0},
            {"past the bottom edge", 150, 700, 150, 140, 0x70000040, shared, 0, 0},
        };
        for (const auto& d : drags) {
            Rows r;
            r.h(table + field::drag_guio, 1);
            r.w(guio_at(table, 1) + 0xc, d.flags); r.w(guio_at(table, 1) + 0x38, d.flags2);
            r.b(d.button == shared ? shared_flag : button_gui, 1); r.b(d.button == shared ? button_gui : shared_flag, 0);
            r.w(pointer_x, d.x); r.w(pointer_y, d.y); r.w(table + field::last_x, d.last_x); r.w(table + field::last_y, d.last_y);
            r.w(table + field::left, 100); r.w(table + field::top, 100); r.w(table + field::right, 200); r.w(table + field::bottom, 200);
            r.w(table + field::saved_bits, d.saved);
            add({.entry = area_drag, .registers = {{3, table}}, .memory = r.memory,
                 .label = std::string("area_drag, ") + d.label, .answers = graphics});
        }
        // The release: controls 3..8 (bounds around (160, 150) except where a case breaks one), areas with entries.
        struct Control { std::uint32_t flags, left, top, right, bottom; };
        const Control hit{2, 100, 100, 200, 200};
        const Control controls[] = {hit, {0x202, 100, 100, 200, 200}, {0, 100, 100, 200, 200}, {2, 161, 100, 200, 200},
                                    {2, 100, 100, 159, 200}, {2, 100, 151, 200, 200}, {2, 100, 100, 200, 149}};
        for (unsigned variant = 0; variant < 3 + sizeof(controls) / sizeof(controls[0]); ++variant) {
            Rows r;
            r.h(table + field::drag_guio, 1); r.h(guio_at(table, 1) + 0x76, 0x1234);
            r.w(guio_at(table, 1) + 0xc, variant & 1 ? 0x100 : 0);
            r.b(variant & 1 ? button_gui : shared_flag, 0); r.b(variant & 1 ? shared_flag : button_gui, 1);
            r.w(pointer_x, 160); r.w(pointer_y, 150); r.w(table + field::last_x, 150); r.w(table + field::last_y, 140);
            r.w(table + field::left, 100); r.w(table + field::top, 100); r.w(table + field::right, 200); r.w(table + field::bottom, 200);
            r.w(table + field::saved_bits, variant == 2 ? 0x70000040 : 0);
            // Controls in the order list: 3..9 take the case's control, so each bound is broken once.
            const unsigned count = 7;
            r.h(table + field::guio_order_count, count);
            for (unsigned n = 0; n < count; ++n) {
                const auto index = 3 + n;
                r.h(table + field::guio_order + 2 * n, index);
                const auto& control = controls[variant < 3 ? (n == 0 || n == 1 ? 0 : n) : (n + variant) % 7];
                const auto record = guio_at(table, index);
                r.w(record + 0xc, control.flags); r.w(record + 0x10, control.left); r.w(record + 0x14, control.top);
                r.w(record + 0x18, control.right); r.w(record + 0x1c, control.bottom);
            }
            // Areas: area 2 visible and without a parent, with three entries (controls 10..12, increasing keys);
            // area 3 hidden; area 4 with a parent.
            r.h(table + field::area_count, 3);
            r.h(table + field::area_order, 2); r.h(table + field::area_order + 2, 3); r.h(table + field::area_order + 4, 4);
            for (unsigned a = 2; a <= 4; ++a) {
                const auto row = area_row(table, a);
                r.b(row + 0xbf2, a == 3 ? 0 : 1); r.h(row + 0x212, a == 4 ? 0 : 0xffff);
                r.h(row + 0xb7a, 0); r.h(row + 0xb78, 3); r.h(row + 0xb7c, 0x55); r.b(row + 0xbb5, variant % 2 ? 1 : 5);
                for (unsigned k = 0; k < 3; ++k) r.h(row + 0x216 + 2 * k, a == 2 ? 10 + k : 13 + k);
            }
            for (unsigned item = 10; item <= 15; ++item) {
                const auto record = guio_at(table, item);
                r.w(record + 0xc, item == 11 ? 0x202 : 2); r.w(record + 0x10, 100); r.w(record + 0x14, 100);
                r.w(record + 0x18, 200); r.w(record + 0x1c, item == 12 ? 120 : 200);
                r.w(record + 0x24, 100 + (item - 10) * 3);
            }
            add({.entry = area_drag, .registers = {{3, table}}, .memory = r.memory,
                 .label = "area_drag, release, variant " + std::to_string(variant), .answers = graphics});
        }
    }
}
}
