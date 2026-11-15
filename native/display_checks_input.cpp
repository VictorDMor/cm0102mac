// DISPLAY.CPP checker cases for the "input" block (display_input.cpp). The season-day snapshot's display tables
// are empty, so each case writes the few records its branch reads (controls, areas, order lists, the pointer and
// key globals) into the real table inside the main object.
#include "display_checks.hpp"
#include <string>
#include <tuple>

namespace cm::aot::display_checks {
namespace {
using u32 = std::uint32_t;
[[maybe_unused]] constexpr u32 pointer_inside = 0x1071c190, pointer_press = 0x1071a280, pointer_nested = 0x1071aa10,
              text_field_key = 0x1071b520, scroll_bar = 0x1071bd00;
// Table layout (see display_input.cpp).
namespace tbl {
[[maybe_unused]] constexpr u32 area_order = 0x129da8, area_order_count = 0x12a8fc, guio_order = 0x129f9c, guio_order_count = 0x12a8fe,
              pressed_area = 0x12a930, text_focus = 0x12a936;
}
namespace ctl {
[[maybe_unused]] constexpr u32 base = 0xba864, size = 0x17c, tracks = 0x8, flags = 0xc, left = 0x10, top = 0x14, right = 0x18, bottom = 0x1c,
              line = 0x24, max_length = 0x40, caret = 0x44, overlay = 0x4c, stamp = 0x50, font = 0x6e, owner = 0x72,
              area = 0x74, text = 0x78, changed = 0x178, hot = 0x17a;
[[maybe_unused]] constexpr u32 hover = 2, edit = 8, drag = 0x40, keep_focus = 0x20, pressed_flag = 0x100, path = 0x8000, wildcard = 0x10000;
}
namespace ar {
[[maybe_unused]] constexpr u32 parent = 0x212, child = 0x214, entries = 0x216, count = 0xb78, start = 0xb7a, cursor = 0xb7c, scroll = 0xb80,
              step = 0xbb5, enabled = 0xbf2, has_scroll = 0xbf3;
}
namespace bar {
[[maybe_unused]] constexpr u32 left = 0, top = 4, right = 8, bottom = 0xc, track_top = 0x10, track_bottom = 0x14, steps = 0x18, position = 0x1c,
              thumb = 0x20, thumb_top = 0x24, thumb_bottom = 0x28, image = 0x2c, hot = 0x30;
}

struct Builder {
    explicit Builder(u32 t) : table(t) {}
    u32 table;
    std::vector<Write> writes;
    void put(unsigned size, u32 at, u32 value) { writes.push_back({size, at, value}); }
    void field(unsigned size, u32 offset, u32 value) { put(size, table + offset, value); }
    u32 control(u32 index) const { return table + ctl::base + index * ctl::size; }
    u32 area(u32 index) const { return table + index * 0xbf0; }
    void c32(u32 index, u32 offset, u32 value) { put(4, control(index) + offset, value); }
    void c16(u32 index, u32 offset, u32 value) { put(2, control(index) + offset, value); }
    void c8(u32 index, u32 offset, u32 value) { put(1, control(index) + offset, value); }
    void rect(u32 index, u32 l, u32 t, u32 r, u32 b) { c32(index, ctl::left, l); c32(index, ctl::top, t); c32(index, ctl::right, r); c32(index, ctl::bottom, b); }
    void text(u32 index, const std::string& s) {
        for (std::size_t i = 0; i <= s.size(); ++i) c8(index, ctl::text + u32(i), i < s.size() ? std::uint8_t(s[i]) : 0);
    }
    // A control: flags, rectangle, line, no tracks, no area, not hot.
    void make(u32 index, u32 flags, u32 l, u32 t, u32 r, u32 b, u32 line = 0) {
        c32(index, ctl::flags, flags); rect(index, l, t, r, b); c32(index, ctl::line, line);
        c32(index, ctl::tracks, 0); c16(index, ctl::area, 0xffff); c8(index, ctl::hot, 0); c32(index, ctl::stamp, 0);
    }
    // An area: enabled, parent, child, its entries (control indexes), step; cursor at the start.
    void make_area(u32 index, bool enabled, std::uint16_t parent, std::uint16_t child, std::initializer_list<std::uint16_t> entries,
                   u32 step = 3, bool scroll = false) {
        const auto a = area(index);
        put(1, a + ar::enabled, enabled); put(1, a + ar::has_scroll, scroll); put(2, a + ar::parent, parent); put(2, a + ar::child, child);
        put(2, a + ar::start, 0); put(2, a + ar::cursor, 0); put(2, a + ar::count, u32(entries.size())); put(1, a + ar::step, step);
        u32 n = 0;
        for (const auto e : entries) put(2, a + ar::entries + 2 * n++, e);
    }
    void area_order(std::initializer_list<std::uint16_t> areas) {
        u32 n = 0;
        for (const auto a : areas) field(2, tbl::area_order + 2 * n++, a);
        field(2, tbl::area_order_count, u32(areas.size()));
    }
    void guio_order(std::initializer_list<std::uint16_t> guios) {
        u32 n = 0;
        for (const auto g : guios) field(2, tbl::guio_order + 2 * n++, g);
        field(2, tbl::guio_order_count, u32(guios.size()));
    }
};
}

DISPLAY_CASES("input") {
    if (scenario.match) return;
    const auto game = scenario.toc(-0x7bb0);
    const u32 mouse_x = scenario.toc(-0x5c10), mouse_y = scenario.toc(-0x5c14), pressed_flag = scenario.toc(-0x5c0c),
              success = scenario.toc(-0x7bac), timer = scenario.toc(-0x5c18), key = scenario.toc(-0x5c1c), typed = scenario.toc(-0x74b0);
    // Drawing and window routines of other modules cannot run in a snapshot: answered on both sides.
    const std::vector<std::pair<u32, u32>> drawing{{0x10715110, 0}, {0x10715340, 0}, {0x107692f0, 0}, {0x1076b140, 0}, {0x1076b7e0, 0},
                                                   {0x10726710, 0}, {0x107267e0, 0}, {0x10713f70, 0}, {0x10714000, 0}, {0x10725ec0, 0},
                                                   {0x107268c0, 0}};
    const std::vector<u32> tables{game + 0x2c68, game + 0x12d6b0};
    // The pointer and keyboard globals the routines read.
    struct Input { std::int32_t x = 0, y = 0; u32 success = 1, pressed = 1, now = 1000, key = 0, typed = 0; };
    const auto globals = [&](const Input& in) {
        return std::vector<Write>{{4, mouse_x, u32(in.x)}, {4, mouse_y, u32(in.y)}, {1, success, in.success}, {1, pressed_flag, in.pressed},
                                  {4, timer, in.now}, {4, key, in.key}, {1, typed, in.typed}};
    };
    const auto join = [](std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; };

    // 0x1071c190: the pointer in a rectangle, on and off every edge.
    for (const auto& [x, y, label] : std::initializer_list<std::tuple<std::int32_t, std::int32_t, const char*>>{
             {50, 60, "inside"}, {10, 60, "left edge"}, {9, 60, "left of"}, {100, 60, "right edge"}, {101, 60, "right of"},
             {50, 20, "top edge"}, {50, 19, "above"}, {50, 80, "bottom edge"}, {50, 81, "below"}, {-5, 60, "negative x"}})
        cases.push_back({.entry = pointer_inside, .registers = {{3, tables[0]}, {4, 10}, {5, 20}, {6, 100}, {7, 80}},
                         .memory = {{4, mouse_x, u32(x)}, {4, mouse_y, u32(y)}}, .label = std::string("pointer ") + label});
    cases.push_back({.entry = pointer_inside, .registers = {{3, tables[0]}, {4, 0xfffffff0u}, {5, 0xfffffff0u}, {6, 5}, {7, 5}},
                     .memory = {{4, mouse_x, 0xfffffff8u}, {4, mouse_y, 0}}, .label = "pointer negative rectangle"});

    for (const auto table : tables) {
        const std::string which = table == tables[0] ? "first table, " : "second table, ";
        // 0x1071b520: a text field (control 2, "Hello", rectangle 10,20 .. 200,40, caret at `caret`).
        struct Field { u32 flags = ctl::edit; std::int32_t caret = 2, max_length = 20; u32 overlay = 0, changed = 0, right = 200;
                       std::int32_t focus = -1; std::string text = "Hello"; };
        const auto field = [&](const Field& f, const Input& in) {
            Builder b{table};
            b.make(2, f.flags, 10, 20, f.right, 40);
            b.text(2, f.text);
            b.c32(2, ctl::caret, u32(f.caret)); b.c32(2, ctl::max_length, u32(f.max_length)); b.c32(2, ctl::overlay, f.overlay);
            b.c8(2, ctl::changed, f.changed); b.c16(2, ctl::font, 0);
            b.field(2, tbl::text_focus, u32(f.focus));
            return join(b.writes, globals(in));
        };
        const auto edit = [&](const std::string& label, const Field& f, const Input& in) {
            cases.push_back({.entry = text_field_key, .registers = {{3, table}, {4, 2}}, .memory = field(f, in), .answers = drawing, .label = which + "text " + label});
        };
        edit("assertion: not an edit control", {.flags = 0}, {});
        for (const u32 overlay : {0u, 0x1234u}) {
            const auto o = std::string(overlay ? ", overlay" : "");
            edit("pointer enters idle field" + o, {.caret = -1, .overlay = overlay}, {.x = 50, .y = 30});
            edit("pointer on active field" + o, {.caret = 2, .overlay = overlay}, {.x = 50, .y = 30});
            edit("pointer leaves, unchanged" + o, {.caret = 3, .overlay = overlay}, {.x = 500, .y = 30});
            edit("pointer leaves, changed" + o, {.caret = 3, .overlay = overlay, .changed = 1}, {.x = 500, .y = 30});
            edit("pointer leaves idle field" + o, {.caret = -1, .overlay = overlay}, {.x = 500, .y = 30});
            for (const u32 k : {0x30u, 0x24u})
                for (const u32 flags : {ctl::edit, ctl::edit | ctl::keep_focus})
                    edit("escape key " + std::to_string(k) + (flags & ctl::keep_focus ? " keeps focus" : "") + o,
                         {.flags = flags, .caret = 1, .overlay = overlay, .changed = k & 1}, {.success = 0, .key = k});
            edit("escape without caret", {.caret = -1}, {.success = 0, .key = 0x30});
            edit("clear on first key" + o, {.caret = 0, .max_length = -3, .overlay = overlay}, {.success = 0, .key = 0x41});
            edit("left arrow" + o, {.caret = 3, .overlay = overlay}, {.success = 0, .key = 0x7b});
            edit("right arrow" + o, {.caret = 2, .overlay = overlay}, {.success = 0, .key = 0x7c});
            edit("backspace" + o, {.caret = 3, .overlay = overlay}, {.success = 0, .key = 0x33});
            edit("forward delete" + o, {.caret = 1, .overlay = overlay}, {.success = 0, .key = 0x75});
            edit("type at the end" + o, {.caret = 5, .overlay = overlay}, {.success = 0, .typed = 'x'});
            edit("type in the middle" + o, {.caret = 2, .overlay = overlay}, {.success = 0, .typed = 'x'});
        }
        edit("focus elsewhere, idle field", {.caret = -1, .focus = 5}, {.success = 0});
        edit("focus elsewhere, overlay", {.caret = -1, .overlay = 7, .focus = 5}, {.success = 0});
        edit("focus elsewhere, active field", {.caret = 1, .focus = 5}, {.success = 0});
        edit("focus is this field", {.caret = -1, .focus = 2}, {.success = 0});
        edit("idle, no key", {.caret = -1}, {.success = 0});
        edit("idle with a key", {.caret = -1, .max_length = -1}, {.success = 0, .key = 0x41});
        edit("first key but a limit", {.caret = 0, .max_length = 20}, {.success = 0, .key = 0x41});
        edit("left arrow at the start", {.caret = 0}, {.success = 0, .key = 0x7b});
        edit("right arrow at the end", {.caret = 5}, {.success = 0, .key = 0x7c});
        edit("right arrow, other key", {.caret = 2}, {.success = 0, .key = 0x7d});
        edit("backspace at the start", {.caret = 0}, {.success = 0, .key = 0x33});
        edit("backspace at the end", {.caret = 5}, {.success = 0, .key = 0x33});
        edit("forward delete at the end", {.caret = 5}, {.success = 0, .key = 0x75});
        edit("forward delete at the last", {.caret = 4}, {.success = 0, .key = 0x75});
        edit("forward delete at the start", {.caret = 0}, {.success = 0, .key = 0x75});
        edit("empty text, type", {.caret = 0, .text = ""}, {.success = 0, .typed = 'a'});
        edit("empty text, backspace", {.caret = 0, .text = ""}, {.success = 0, .key = 0x33});
        edit("full field", {.caret = 5, .max_length = 5}, {.success = 0, .typed = 'x'});
        edit("past the limit", {.caret = 5, .max_length = 3}, {.success = 0, .typed = 'x'});
        edit("too wide", {.caret = 5, .right = 25}, {.success = 0, .typed = 'W'});
        edit("too wide, middle", {.caret = 1, .right = 25}, {.success = 0, .typed = 'W'});
        for (const u32 ch : {0x00u, 0x1fu, 0x20u, 0x21u, 0x22u, 0x2au, 0x2fu, 0x3au, 0x3cu, 0x3eu, 0x3fu, 0x5cu, 0x7bu, 0x7cu, 0x7du, 0x5bu,
                             0x5du, 0x7eu, 0x7fu, 0x80u, 0xffu})
            for (const u32 flags : {ctl::edit, ctl::edit | ctl::path | ctl::wildcard}) {
                if (flags != ctl::edit && ch != 0x3au && ch != 0x3fu && ch != 0x5cu) continue;
                edit("character " + std::to_string(ch) + (flags != ctl::edit ? " with path flags" : ""), {.flags = flags, .caret = 2},
                     {.success = 0, .typed = ch});
            }
        edit("character 0x3f with only the path flag", {.flags = ctl::edit | ctl::path, .caret = 2}, {.success = 0, .typed = 0x3f});
        edit("character 0x3a with only the wildcard flag", {.flags = ctl::edit | ctl::wildcard, .caret = 2}, {.success = 0, .typed = 0x3a});
        edit("character 0x5c with only the wildcard flag", {.flags = ctl::edit | ctl::wildcard, .caret = 2}, {.success = 0, .typed = 0x5c});
        edit("negative caret, typed", {.caret = -5}, {.success = 0, .typed = 'x'});

        // 0x1071bd00: the scroll bar of area 3 (rectangle 700,10 .. 716,300, track 26..284, thumb 100..139).
        struct Bar { std::int32_t position = 4, steps = 10, thumb_top = 100, thumb_bottom = 139, thumb = 40; u32 image = 0, hot = 0, step = 3; };
        const auto scroll = [&](const std::string& label, const Bar& bar, const Input& in, u32 area = 3) {
            Builder b{table};
            const auto a = b.area(area);
            b.put(1, a + ar::step, bar.step);
            const auto s = a + ar::scroll;
            for (const auto& [offset, value] : std::initializer_list<std::pair<u32, u32>>{
                     {bar::left, 700}, {bar::top, 10}, {bar::right, 716}, {bar::bottom, 300}, {bar::track_top, 26}, {bar::track_bottom, 284},
                     {bar::steps, u32(bar.steps)}, {bar::position, u32(bar.position)}, {bar::thumb, u32(bar.thumb)},
                     {bar::thumb_top, u32(bar.thumb_top)}, {bar::thumb_bottom, u32(bar.thumb_bottom)}, {bar::image, bar.image}})
                b.put(4, s + offset, value);
            b.put(1, s + bar::hot, bar.hot);
            cases.push_back({.entry = scroll_bar, .registers = {{3, table}, {4, area}}, .memory = join(b.writes, globals(in)),
                             .answers = drawing, .label = which + "scroll bar " + label});
        };
        scroll("arrow up", {}, {.x = 705, .y = 15});
        scroll("arrow up at the top", {.position = 0}, {.x = 705, .y = 15});
        scroll("arrow up at the top, hot before", {.position = 0, .hot = 1}, {.x = 705, .y = 15});
        scroll("arrow up with an image", {.image = 0x40}, {.x = 705, .y = 15});
        scroll("arrow down", {}, {.x = 705, .y = 290});
        scroll("arrow down at the bottom", {.position = 10}, {.x = 705, .y = 290});
        scroll("arrow down, hot before", {.position = 10, .hot = 2}, {.x = 705, .y = 290});
        scroll("arrow down, hot before, image", {.position = 10, .image = 0x40, .hot = 1}, {.x = 705, .y = 290});
        scroll("page up", {}, {.x = 705, .y = 50});
        scroll("page up near the top", {.position = 2}, {.x = 705, .y = 50});
        scroll("page down", {}, {.x = 705, .y = 200});
        scroll("page down near the bottom", {.position = 9}, {.x = 705, .y = 200});
        scroll("page down, step 0", {.step = 0}, {.x = 705, .y = 200});
        scroll("thumb press", {}, {.x = 705, .y = 120});
        scroll("thumb press, hot before", {.hot = 2}, {.x = 705, .y = 120});
        scroll("no press", {}, {.x = 705, .y = 120, .success = 0});
        scroll("no press, hot before", {.hot = 1}, {.x = 705, .y = 120, .success = 0});
        scroll("pointer outside", {}, {.x = 0, .y = 0});
        scroll("pointer outside the bar, hot before", {.hot = 2}, {.x = 500, .y = 120});
        scroll("pointer between", {}, {.x = 705, .y = 20});
        scroll("pointer on the gap", {}, {.x = 705, .y = 295});
        scroll("steps 0", {.steps = 0, .position = 0}, {.x = 705, .y = 290});
        scroll("arrow down, steps 1", {.steps = 1, .position = 0}, {.x = 705, .y = 290});
        scroll("page down, negative position", {.position = -7}, {.x = 705, .y = 200});
        scroll("arrow up, large position", {.position = 0x7fffffff, .steps = 0x7fffffff}, {.x = 705, .y = 15});
        scroll("arrow up, minimum position", {.position = std::int32_t(0x80000000u)}, {.x = 705, .y = 15});
        scroll("page up from the minimum", {.position = std::int32_t(0x80000000u)}, {.x = 705, .y = 50});
        scroll("thumb recompute, overflow", {.position = 0x7fffffff, .steps = 3}, {.x = 705, .y = 290});
        scroll("area 0", {}, {.x = 705, .y = 15}, 0);
        scroll("area 200", {}, {.x = 705, .y = 15}, 200);

        // 0x1071a280 and 0x1071aa10: scenes of controls and areas.
        const auto both = [&](const std::string& label, const Builder& b, const Input& in) {
            for (const auto entry : {pointer_press, pointer_nested})
                cases.push_back({.entry = entry, .registers = {{3, table}}, .memory = join(b.writes, globals(in)), .answers = drawing,
                                 .label = which + (entry == pointer_press ? "press " : "nested ") + label});
        };
        // Controls only: 0 hover, 1 hover with the pressed flag, 2 hover and drag with tracks, in order [0, 1, 2].
        for (const auto& [x, name] : std::initializer_list<std::pair<std::int32_t, const char*>>{{50, "on 0"}, {250, "on 1"}, {450, "on 2"}, {700, "nowhere"}})
            for (const u32 ok : {0u, 1u})
                for (const u32 pressed : {0u, 1u})
                    for (const u32 hot : {0u, 1u})
                        for (const u32 area : {0xffffu, 4u}) {
                            Builder b{table};
                            b.make(0, ctl::hover, 0, 0, 100, 100); b.make(1, ctl::hover | ctl::pressed_flag, 200, 0, 300, 100);
                            b.make(2, ctl::hover | ctl::drag, 400, 0, 500, 100);
                            b.c32(2, ctl::tracks, 1);
                            for (u32 n = 0; n < 3; ++n) { b.c8(n, ctl::hot, hot); b.c16(n, ctl::area, area); }
                            b.c32(0, ctl::tracks, area == 4 ? 0 : 1);
                            b.guio_order({0, 1, 2});
                            b.area_order({});
                            b.make_area(4, true, 0xffff, 0xffff, {});
                            both(std::string("controls ") + name + ", success " + std::to_string(ok) + ", flag " + std::to_string(pressed) +
                                     ", hot " + std::to_string(hot) + (area == 4 ? ", area" : ""),
                                 b, {.x = x, .y = 50, .success = ok, .pressed = pressed});
                        }
        // Text fields in the control order: control 1 consumes the event (caret set, pointer outside, changed).
        for (const u32 caret1 : {0xffffffffu, 3u})
            for (const u32 changed : {0u, 1u})
                for (const u32 pressed_ok : {0u, 1u}) {
                    Builder b{table};
                    for (u32 n = 0; n < 3; ++n) {
                        b.make(n, ctl::hover | ctl::edit, n * 200, 0, n * 200 + 100, 100);
                        b.text(n, "abc"); b.c32(n, ctl::caret, n == 1 ? caret1 : 0xffffffffu); b.c32(n, ctl::max_length, 20);
                        b.c8(n, ctl::changed, n == 1 ? changed : 0); b.c32(n, ctl::overlay, 0);
                    }
                    b.field(2, tbl::text_focus, 0xffff);
                    b.guio_order({0, 1, 2}); b.area_order({});
                    both("fields, caret " + std::to_string(caret1) + ", changed " + std::to_string(changed) + ", success " + std::to_string(pressed_ok),
                         b, {.x = 700, .y = 50, .success = pressed_ok});
                    both("fields on 1, caret " + std::to_string(caret1) + ", changed " + std::to_string(changed) + ", success " + std::to_string(pressed_ok),
                         b, {.x = 250, .y = 50, .success = pressed_ok});
                }
        // Areas: area 0 holds controls 3, 4, 5 (lines 0, 2, 3, step 3), area 1 control 6, order [0, 1, 2 (parent), 3 (disabled)].
        for (const auto& [x, y, name] : std::initializer_list<std::tuple<std::int32_t, std::int32_t, const char*>>{
                 {50, 50, "on 3"}, {250, 50, "on 4"}, {450, 50, "on 5 (beyond the rows)"}, {650, 50, "on 6"}, {700, 400, "nowhere"}})
            for (const u32 ok : {0u, 1u})
                for (const u32 edit_mask : {0u, 2u, 8u, 4u})  // which of 3, 4, 5, 6 are text fields (bit n-3... by listing)
                    for (const bool scrolled : {false, true})
                      for (const u32 hot_before : {0u, 1u}) {
                        Builder b{table};
                        b.make(3, ctl::hover | (edit_mask & 1 ? ctl::edit : 0), 0, 0, 100, 100, 0);
                        b.make(4, ctl::hover | (edit_mask & 2 ? ctl::edit : 0), 200, 0, 300, 100, 2);
                        b.make(5, ctl::hover | (edit_mask & 4 ? ctl::edit : 0), 400, 0, 500, 100, 3);
                        b.make(6, ctl::hover | (edit_mask & 8 ? ctl::edit : 0), 600, 0, 700, 100, 0);
                        for (u32 n = 3; n <= 6; ++n) {
                            b.c32(n, ctl::caret, 2); b.text(n, "abc"); b.c32(n, ctl::max_length, 20); b.c8(n, ctl::changed, 1);
                            b.c32(n, ctl::overlay, 0);
                        }
                        for (u32 n = 3; n <= 6; ++n) b.c8(n, ctl::hot, hot_before);
                        b.field(2, tbl::text_focus, 0xffff);
                        b.make_area(0, true, 0xffff, 0xffff, {3, 4, 5}, 3, scrolled);
                        b.make_area(1, true, 0xffff, 0xffff, {6}, 3, scrolled);
                        b.make_area(2, true, 9, 0xffff, {}, 3, true);
                        b.make_area(3, false, 0xffff, 0xffff, {3}, 3, true);
                        b.area_order({0, 1, 2, 3});
                        b.guio_order({});
                        for (const u32 a : {0u, 1u}) {
                            const auto s = b.area(a) + ar::scroll;
                            for (const auto& [offset, value] : std::initializer_list<std::pair<u32, u32>>{
                                     {bar::left, 800}, {bar::top, 0}, {bar::right, 816}, {bar::bottom, 300}, {bar::track_top, 26},
                                     {bar::track_bottom, 284}, {bar::steps, 10}, {bar::position, 4}, {bar::thumb, 40},
                                     {bar::thumb_top, 100}, {bar::thumb_bottom, 139}, {bar::image, 0}})
                                b.put(4, s + offset, value);
                            b.put(1, s + bar::hot, 0);
                        }
                        both(std::string("areas ") + name + ", success " + std::to_string(ok) + ", fields " + std::to_string(edit_mask) +
                                 (scrolled ? ", scroll bars" : "") + (hot_before ? ", hot" : ""),
                             b, {.x = x, .y = y, .success = ok});
                    }
        // A scroll bar press through the areas pass.
        for (const std::int32_t y : {15, 290, 50, 200, 120}) {
            Builder b{table};
            b.make_area(0, true, 0xffff, 0xffff, {}, 3, true);
            b.area_order({0}); b.guio_order({});
            const auto s = b.area(0) + ar::scroll;
            for (const auto& [offset, value] : std::initializer_list<std::pair<u32, u32>>{
                     {bar::left, 800}, {bar::top, 10}, {bar::right, 816}, {bar::bottom, 300}, {bar::track_top, 26},
                     {bar::track_bottom, 284}, {bar::steps, 10}, {bar::position, 4}, {bar::thumb, 40},
                     {bar::thumb_top, 100}, {bar::thumb_bottom, 139}, {bar::image, 0}})
                b.put(4, s + offset, value);
            b.put(1, s + bar::hot, 0);
            both("scroll bar at y " + std::to_string(y), b, {.x = 805, .y = y});
        }
        // Nested areas: area 0 (parent control 7, child control 3, entries 8 and 9), control 7 owned by area 1
        // whose child is control 6; the timer against each control's stamp; a control's area (4) open or closed.
        for (const auto& [x, y, name] : std::initializer_list<std::tuple<std::int32_t, std::int32_t, const char*>>{
                 {50, 50, "on 8"}, {50, 250, "on 9"}, {50, 450, "on neither"}})
            for (const u32 ok : {0u, 1u})
                for (const u32 hot8 : {0u, 1u})
                    for (const u32 hot7 : {0u, 1u})
                        for (const u32 hot_children : {0u, 1u})
                            for (const u32 age : {0u, 1000u})
                                for (const u32 target : {0xffffu, 4u})
                                    for (const bool target_open : {false, true}) {
                                        if (target == 0xffffu && target_open) continue;
                                        Builder b{table};
                                        b.make(8, ctl::hover, 0, 0, 100, 100, 0);
                                        b.make(9, ctl::hover, 0, 200, 100, 300, 1);
                                        b.make(7, 0, 0, 0, 0, 0); b.make(3, 0, 0, 0, 0, 0); b.make(6, 0, 0, 0, 0, 0);
                                        b.c8(8, ctl::hot, hot8); b.c8(9, ctl::hot, hot8 ^ 1);
                                        b.c8(7, ctl::hot, hot7); b.c8(3, ctl::hot, hot_children); b.c8(6, ctl::hot, hot_children);
                                        b.c16(7, ctl::owner, 1);
                                        b.c16(8, ctl::area, target); b.c16(9, ctl::area, target);
                                        b.c32(8, ctl::stamp, 1000 - age); b.c32(9, ctl::stamp, 1000 - age);
                                        b.make_area(0, true, 7, 3, {8, 9}, 3);
                                        b.make_area(1, true, 0xffff, 6, {});
                                        b.make_area(4, target_open, 0xffff, 0xffff, {});
                                        b.field(2, tbl::pressed_area, 4);
                                        b.area_order({0}); b.guio_order({});
                                        for (const auto entry : {pointer_nested})
                                            cases.push_back({.entry = entry, .registers = {{3, table}},
                                                             .memory = join(b.writes, globals({.x = x, .y = y, .success = ok})),
                                                             .answers = drawing, .label = which + "nested " + name + ", success " + std::to_string(ok) + ", hot8 " +
                                                                      std::to_string(hot8) + ", hot7 " + std::to_string(hot7) + ", children " +
                                                                      std::to_string(hot_children) + (age ? ", old" : ", new") +
                                                                      (target == 4 ? (target_open ? ", target open" : ", target closed") : "")});
                                    }
    }
    // The assertion of 0x1071bd00: the scroll bar's address wraps to 0 (the original compares the sum only; no
    // memory is touched before the report).
    cases.push_back({.entry = scroll_bar, .registers = {{3, 0u - 0xb80u}, {4, 0}}, .label = "scroll bar assertion, address wraps to 0"});
}
}
