#include "main_program_detail.hpp"

namespace cm::aot::main_program {
using namespace detail;
namespace {
void set_carry(Cpu& c, bool carry) { c.s.wr<1>(guest::reg::xer_ca, carry ? 1 : 0); }
// `srawi rD,rS,16`: the carry is set when a negative value loses one bits.
std::uint32_t shift_right_16(Cpu& c, std::uint32_t value) {
    set_carry(c, std::int32_t(value) < 0 && (value & 0xffff) != 0);
    return std::uint32_t(std::int32_t(value) >> 16);
}
// `cntlzw; srwi 5`: 1 when the value is zero.
std::uint32_t is_zero(std::uint32_t value) { return value == 0 ? 1 : 0; }
// The game's scroll and key actions (0x10743590 with the game object).
void game_action(Cpu& c, std::uint32_t action, std::uint32_t amount, std::uint32_t back) {
    c.set({{3, c.r(30)}, {4, action}, {5, amount}});
    c.call(0x10743590, back);
}
void request_screen(Cpu& c, unsigned base_register, std::uint16_t request) {
    c.set({{base_register, c.toc(slot::game) + 0x260000}, {0, request}});
    c.put16(c.r(base_register) - 0x7e62, request);
}
// pump_events's mouse-down: FindWindow's part code.
void mouse_down(Cpu& c, const Frame& frame) {
    const auto event = frame.at(0x48), window = frame.at(0x3c), where = event + event_record::where;
    c.set({{3, c.u32(where)}, {4, window}});
    toolbox(c, frame, 0x10780720, 0x1076fff0);  // FindWindow
    const auto part = std::int16_t(c.r(3));
    c.set(0, std::uint32_t(std::int32_t(part)));
    const auto same_window = [&](unsigned window_register) {
        c.set({{3, c.u32(window)}, {0, c.u32(c.r(window_register))}});
        return compare_logical(c, c.r(3), c.r(0)) == 0;
    };
    if (const auto against_drag = compare(c, part, 4); against_drag == 0) {  // inDrag
        if (same_window(30)) return;
        toolbox(c, frame, 0x10780750, 0x10770070);  // GetGrayRgn
        c.set(4, frame.at(0x40));
        toolbox(c, frame, 0x10780768, 0x1077007c);  // GetRegionBounds
        c.set(3, c.u32(window));
        toolbox(c, frame, 0x10780738, 0x10770088);  // SelectWindow
        c.set({{3, c.u32(window)}, {5, frame.at(0x40)}, {4, c.u32(where)}});
        toolbox(c, frame, 0x10780780, 0x1077009c);  // DragWindow
    } else if (against_drag > 0) {
        if (compare(c, part, 6) != 0) return;  // inGoAway
        c.set({{3, c.u32(window)}, {4, c.u32(where)}});
        toolbox(c, frame, 0x10780798, 0x107700b0);  // TrackGoAway
        if (!test_byte(c, c.r(3))) return;
        if (same_window(27)) request_screen(c, 3, request_quit);
    } else if (const auto against_system = compare(c, part, 2); against_system > 0) {  // inContent
        if (same_window(27)) {
            c.set(3, event);
            c.link(0x10770040);
            content_click(c, event);
        }
        if (same_window(30)) return;
        toolbox(c, frame, 0x10780738, 0x10770054);  // SelectWindow
    } else if (against_system < 0 && compare(c, part, 1) >= 0) {  // inMenuBar
        c.set(3, event);
        c.link(0x107700e4);
        adjust_menus(c, event);
        c.set(3, c.u32(where));
        toolbox(c, frame, 0x10780810, 0x107700ec);  // MenuSelect
        c.set(0, c.r(3));
        c.set({{3, event}, {4, c.r(0)}});
        c.link(0x10770100);
        menu_command(c, event, c.r(4));
    }
}
}

void pump_events(Cpu& c) {
    Frame frame(c, 0x1076fe80, 0x80, {.first_gpr = 24});
    c.set({{29, c.toc(slot::button_down)}, {30, c.toc(slot::other_window)}, {24, c.toc(slot::control_down)},
           {25, c.toc(slot::pending_scroll)}, {26, c.toc(slot::idle_wait)}, {27, c.toc(slot::game_window)},
           {28, c.toc(slot::game)}, {31, c.toc(slot::left_held)}});
    c.set(3, frame.at(0x38));
    toolbox(c, frame, 0x107803f0, 0x1076feb8);  // GetPort: kept at sp+0x38
    c.set(3, c.u32(c.r(27)));
    toolbox(c, frame, 0x107805a0, 0x1076fec4);  // GetWindowPort
    toolbox(c, frame, 0x107804f8, 0x1076fecc);  // SetPort
    c.call(0x10769b50, 0x1076fed4);
    c.call(0x10746a30, 0x1076fed8);  // the game's tick count
    c.set({{4, c.u32(c.r(26))}, {5, c.toc(slot::timer)}});
    const bool waiting = compare(c, std::int32_t(c.r(4))) != 0;
    c.put32(c.r(5), c.r(3));
    if (waiting) {
        // Counting down before events are read again; the port stays the
        // game window's (the original returns without restoring it).
        c.set(0, c.r(4) - 1);
        set_carry(c, c.r(4) != 0);
        const bool more = compare(c, std::int32_t(c.r(0))) != 0;
        c.put32(c.r(26), c.r(0));
        if (!more) {
            c.set(0, 0);
            c.put8(c.r(31), 0);
        }
        return;
    }
    c.set(5, c.u32(c.r(25)));
    if (const auto scroll = compare(c, std::int32_t(c.r(5))); scroll != 0) {
        // A wheel turn the Carbon handler noted: up, or down, a page with control.
        c.set({{0, c.u8(c.r(24))}, {3, c.r(28)}});
        c.set(4, scroll > 0 ? (test8(c, c.r(0)) ? 3 : 1) : (test8(c, c.r(0)) ? 2 : 0));
        if (scroll > 0) c.call(0x10743590, 0x1076ff30);
        else {
            c.set(5, 0u - c.r(5));
            c.call(0x10743590, 0x1076ff54);
        }
        c.set(0, 0);
        c.put32(c.r(25), 0);
        return;
    }
    c.link(0x1076ff64);
    track_mouse(c);
    const auto event = frame.at(0x48);
    c.set({{3, 0xffff}, {5, word(c, global::sleep_ticks)}, {6, word(c, global::mouse_region)}, {4, event}});
    toolbox(c, frame, 0x10780510, 0x1076ff7c);  // WaitNextEvent(everyEvent)
    if (test_byte(c, c.r(3))) {
        const std::uint32_t what = c.u16(event + event_record::what);
        c.set(0, what);
        if (compare_logical(c, what, 0x17) <= 0) {
            // The original's jump table by event kind.
            c.set({{3, c.toc(slot::event_table)}, {0, what << 2}});
            c.set(3, c.u32(c.r(3) + c.r(0)));
            c.s.wr<4>(guest::reg::ctr, c.r(3));
            const auto handle = [&](void (*handler)(Cpu&, std::uint32_t), std::uint32_t back) {
                c.set(3, event);
                c.link(back);
                handler(c, event);
            };
            switch (c.r(3)) {
            case 0x1076ffa8: c.set(3, event); c.link(0x1076ffac); activate_event(c); break;
            case 0x1076ffb4: handle(os_event, 0x1076ffbc); break;
            case 0x1076ffc0: handle(high_level_event, 0x1076ffc8); break;
            case 0x1076ffcc: handle(key_down, 0x1076ffd4); break;
            case 0x1076ffd8: c.set(3, event); c.link(0x1076ffe0); mouse_up(c); break;
            case 0x1076ffe4: mouse_down(c, frame); break;
            case 0x10770148: break;
            default: throw std::runtime_error("MAIN.CPP event table entry " + hex_address(c.r(3)));
            }
        }
    } else {
        // No event: a held button ends when Button() says it is up.
        c.set(0, c.u8(c.r(31)));
        bool held = test8(c, c.r(0));
        if (!held) {
            c.set(0, c.u8(c.r(29)));
            held = test8(c, c.r(0));
        }
        if (held) {
            c.set(3, c.toc(slot::button_state));
            c.set(0, c.u16(c.r(3)));
            if (compare(c, std::int32_t(c.r(0)), 1) == 0) {
                toolbox(c, frame, 0x10780828, 0x10770130);  // Button
                if (!test_byte(c, c.r(3))) {
                    c.set(0, 0);
                    c.put8(c.r(31), 0);
                    c.put8(c.r(29), 0);
                }
            }
        }
    }
    c.set(3, c.u32(frame.at(0x38)));
    toolbox(c, frame, 0x107804f8, 0x10770150);  // SetPort: back to the saved port
}

void poll_high_level_events(Cpu& c) {
    Frame frame(c, 0x10770170, 0x50, {});
    const auto event = frame.at(0x38);
    c.set({{3, 0x400}, {5, word(c, global::sleep_ticks)}, {4, event}, {6, word(c, global::mouse_region)}});
    toolbox(c, frame, 0x10780510, 0x10770190);  // WaitNextEvent(highLevelEventMask)
    if (!test_byte(c, c.r(3))) return;
    c.set(0, c.u16(event + event_record::what));
    if (compare(c, std::int32_t(c.r(0)), 0x17) != 0) return;
    c.set(3, event);
    c.link(0x107701b4);
    high_level_event(c, event);
}

void track_mouse(Cpu& c) {
    Frame frame(c, 0x107701d0, 0x50, {});
    c.set(5, c.toc(slot::screen_rect));
    c.set({{4, c.u32(c.r(5))}, {3, frame.at(0x38)}, {0, c.u32(c.r(5) + 4)}});
    c.put32(frame.at(0x3c), c.r(4));
    c.put32(frame.at(0x40), c.r(0));
    toolbox(c, frame, 0x10780618, 0x107701f8);  // GetMouse
    c.set({{4, c.u32(frame.at(0x38))}, {3, frame.at(0x3c)}});
    toolbox(c, frame, 0x10780630, 0x10770208);  // PinRect: v in the high half, h in the low
    const auto point = c.r(3);
    c.set({{5, c.toc(slot::mouse_x)}, {6, point & 0xffff}, {4, c.toc(slot::mouse_y)}, {0, shift_right_16(c, point)}});
    c.put32(c.r(5), c.r(6));
    c.put32(c.r(4), c.r(0));
}

void adjust_menus(Cpu& c, std::uint32_t) {
    Frame frame(c, 0x10770240, 0x50, {.first_gpr = 29});
    c.set({{31, c.toc(slot::pool)}, {3, 2}});
    toolbox(c, frame, 0x107807b0, 0x10770264);  // GetMenu(File)
    c.set(29, c.r(3));
    if (compare(c, std::int32_t(c.r(3))) != 0) {
        // Item 5 (Quit) and item 1 (Save) say when their request is pending,
        // and are disabled then.
        const auto item = [&](std::uint32_t number, bool pending, std::uint32_t enable_back, std::int32_t text,
                              std::uint32_t text_back) {
            c.set({{3, c.r(29)}, {4, number}});
            toolbox(c, frame, pending ? 0x107807c8 : 0x107807f8, enable_back);  // DisableMenuItem / EnableMenuItem
            c.set({{3, c.r(29)}, {5, c.r(31) + std::uint32_t(text)}, {4, number}});
            toolbox(c, frame, 0x107807e0, text_back);  // SetMenuItemText
        };
        c.set(4, c.toc(slot::game));
        c.set(30, c.r(4) + screen_request);
        c.set(0, std::uint32_t(c.i16(c.r(30))));
        if (compare(c, std::int32_t(c.r(0)), request_quit) == 0) item(5, true, 0x1077028c, 0x1e, 0x107702a0);
        else item(5, false, 0x107702b0, 0x30, 0x107702c4);
        c.set(0, std::uint32_t(c.i16(c.r(30))));
        if (compare(c, std::int32_t(c.r(0)), request_save) == 0) item(1, true, 0x107702e0, 0x35, 0x107702f4);
        else item(1, false, 0x10770308, 0x47, 0x1077031c);
    }
    c.set(3, 4);
    toolbox(c, frame, 0x107807b0, 0x10770328);  // GetMenu(Debug)
    c.set(30, c.r(3));
    if (compare(c, std::int32_t(c.r(3))) == 0) return;
    constexpr std::pair<std::uint32_t, std::uint32_t> disabled[]{{1, 0x1077033c}, {3, 0x1077034c}, {2, 0x1077035c}};
    for (const auto& [number, back] : disabled) {
        c.set({{3, c.r(30)}, {4, number}});
        toolbox(c, frame, 0x107807c8, back);  // DisableMenuItem
    }
}

void key_down(Cpu& c, std::uint32_t event) {
    Frame frame(c, 0x10770380, 0xb0, {.first_gpr = 28});
    const auto message = c.u32(event + event_record::message);
    // r28 the event, r29 the character, r30 the game, r31 the key code.
    c.set({{4, c.toc(slot::pool) + 0x67}, {30, c.toc(slot::game)}, {28, event}, {0, message}, {3, frame.at(0x38)},
           {5, message & 0xff}, {31, (message >> 8) & 0x7f}});
    c.set({{29, c.r(5)}, {6, c.r(5)}, {7, c.r(31)}});
    c.call(0x1077b720, 0x107703cc);  // sprintf: a debugging line nothing shows
    c.set(0, c.u16(event + event_record::modifiers) & 0x900);
    if (compare(c, std::int32_t(c.r(0))) != 0) {  // command or option
        c.set(3, c.r(29));
        c.link(0x107703e0);
        c.set(3, speech_key(c, c.r(29)));
        if (test8(c, c.r(3))) return;
    }
    c.set(0, c.u16(event + event_record::modifiers) & 0x100);
    if (compare(c, std::int32_t(c.r(0))) != 0) {  // command: a menu key
        c.set(3, c.r(29) & 0xff);
        toolbox(c, frame, 0x10780660, 0x107703fc);  // MenuKey
        c.set(4, c.r(3));
        if (compare(c, std::int32_t(c.r(4))) != 0) {
            c.set(3, c.r(28));
            c.link(0x10770410);
            menu_command(c, c.r(28), c.r(4));
            return;
        }
    }
    c.set(0, c.r(31) - 0x73);
    if (compare_logical(c, c.r(0), 0xb) <= 0) {
        // The original's jump table for the navigation keys (0x73-0x7e).
        c.set({{3, c.toc(slot::key_table)}, {0, c.r(0) << 2}});
        c.set(3, c.u32(c.r(3) + c.r(0)));
        c.s.wr<4>(guest::reg::ctr, c.r(3));
        switch (c.r(3)) {
        case 0x10770434: game_action(c, 1, 1, 0x10770444); return;  // up
        case 0x10770448: game_action(c, 0, 1, 0x10770458); return;  // down
        case 0x1077045c: game_action(c, 3, 1, 0x1077046c); return;  // page up
        case 0x10770470: game_action(c, 2, 1, 0x10770480); return;  // page down
        case 0x10770484: game_action(c, 5, 1, 0x10770494); return;  // home
        case 0x10770498: game_action(c, 4, 1, 0x107704a8); return;  // end
        case 0x107704ac: break;
        default: throw std::runtime_error("MAIN.CPP key table entry " + hex_address(c.r(3)));
        }
    }
    // Any other key is left for the game's screens.
    c.set({{4, c.toc(slot::key_code)}, {3, c.toc(slot::key_character)}});
    c.put32(c.r(4), c.r(31));
    c.put8(c.r(3), c.r(29));
}

std::uint32_t speech_key(Cpu& c, std::uint32_t character) {
    Frame frame(c, 0x107704e0, 0x40, {});
    c.set(0, character & 0xff);
    const bool s_key = compare(c, std::int32_t(c.r(0)), 0x73) == 0;
    c.set(3, c.toc(slot::speech_wanted));
    if (!s_key) return 0;
    const auto wanted = c.r(3);
    c.set(0, is_zero(std::uint32_t(std::int32_t(c.i8(wanted)))));
    c.put8(wanted, c.r(0));
    c.set(0, c.u8(wanted));
    if (!test8(c, c.r(0))) release_speech(c, frame, 0x10770530, 0x10770544);
    return 1;
}

void content_click(Cpu& c, std::uint32_t event) {
    Frame frame(c, 0x10770570, 0x50, {.first_gpr = 27});
    c.set({{30, c.toc(slot::left_down)}, {27, event}, {31, c.toc(slot::right_down)}, {28, c.toc(slot::button_state)},
           {29, c.toc(slot::control_down)}});
    c.link(0x10770598);
    c.set(3, aqua_menus(c));
    if (test8(c, c.r(3))) {
        // The Carbon click handler already set the button state.
        const std::uint32_t state = c.u16(c.r(28));
        c.set({{4, is_zero(2 - state)}, {3, is_zero(1 - state)}, {0, is_zero(3 - state)}});
        set_carry(c, state <= 3);
        c.put8(c.r(31), c.r(4));
        c.put8(c.r(30), c.r(3));
        c.put8(c.r(29), c.r(0));
    } else {
        // A click with control or option is the right button.
        c.set(4, c.toc(slot::right_button_modifiers));
        c.set({{3, 0}, {5, c.u16(event + event_record::modifiers)}, {0, 1}, {4, c.u32(c.r(4))}});
        c.put8(c.r(29), 0);
        c.set(4, c.r(5) & c.r(4));
        c.set(3, 0u - c.r(4));
        c.put16(c.r(28), 1);
        c.set(0, (c.r(3) | c.r(4)) >> 31);
        c.put8(c.r(31), c.r(0));
        c.set(0, is_zero(std::uint32_t(std::int32_t(c.i8(c.r(31))))));
        c.put8(c.r(30), c.r(0));
    }
    c.set({{5, c.u8(c.r(30))}, {4, c.toc(slot::left_held)}, {0, c.u8(c.r(31))}, {3, c.toc(slot::button_down)}});
    c.put8(c.r(4), c.r(5));
    c.put8(c.r(3), c.r(0));
}

void mouse_up(Cpu& c) {
    c.set({{4, c.toc(slot::left_held)}, {0, 0}, {3, c.toc(slot::button_down)}});
    c.put8(c.r(4), 0);
    c.put8(c.r(3), 0);
}

std::uint32_t window_event(Cpu& c, std::uint32_t handler, std::uint32_t event, std::uint32_t data) {
    Frame frame(c, 0x10770670, 0x50, {.first_gpr = 29});
    c.set({{31, data}, {30, event}, {29, handler}, {3, event}});
    toolbox(c, frame, 0x10780eb8, 0x1077069c);  // GetEventClass
    const auto event_class = std::int32_t(c.r(3));
    const auto kind = [&](std::uint32_t back) {
        c.set(3, c.r(30));
        toolbox(c, frame, 0x10780ed0, back);  // GetEventKind
        return std::int32_t(c.r(3));
    };
    const auto pass_on = [&](std::uint32_t (*callee)(Cpu&, std::uint32_t, std::uint32_t, std::uint32_t),
                             std::uint32_t back) {
        c.set({{3, c.r(29)}, {4, c.r(30)}, {5, c.r(31)}});
        c.link(back);
        return callee(c, c.r(3), c.r(4), c.r(5));
    };
    c.set({{4, 0x77690000}, {0, 0x77696e64}});
    const auto against_window = compare(c, event_class, 0x77696e64);
    if (against_window > 0) return std::uint32_t(event_not_handled);
    if (against_window < 0) {
        c.set({{4, 0x6d6f0000}, {0, 0x6d6f7573}});
        if (compare(c, event_class, 0x6d6f7573) != 0) return std::uint32_t(event_not_handled);
        if (compare(c, kind(0x107706d0), 10) == 0) return pass_on(wheel_event, 0x107706f0);
        // Other mouse events fall into the window events' test (the original
        // asks for the kind again).
    }
    const auto window_kind = kind(0x107706fc);
    if (const auto against_click = compare(c, window_kind, 0x55); against_click == 0) return pass_on(click_event, 0x1077073c);
    else if (against_click < 0 && compare(c, window_kind, 2) == 0) return pass_on(bounds_event, 0x10770728);
    return std::uint32_t(event_not_handled);
}

std::uint32_t wheel_event(Cpu& c, std::uint32_t, std::uint32_t event, std::uint32_t) {
    Frame frame(c, 0x10770760, 0x40, {});
    c.set({{5, 0x6c6f6e67}, {3, event}, {4, 0x6d77646c}, {6, 0}, {7, 4}, {9, frame.at(0x38)}, {8, 0}});
    toolbox(c, frame, 0x10780ee8, 0x10770794);  // GetEventParameter('mwdl', 'long')
    const auto scroll = c.toc(slot::pending_scroll);
    c.set({{5, scroll}, {3, std::uint32_t(event_not_handled)}, {0, c.u32(frame.at(0x38))}, {4, c.u32(scroll)}});
    c.set(0, c.r(4) + c.r(0));
    c.put32(scroll, c.r(0));
    return std::uint32_t(event_not_handled);
}

std::uint32_t click_event(Cpu& c, std::uint32_t, std::uint32_t event, std::uint32_t) {
    Frame frame(c, 0x107707c0, 0x50, {.first_gpr = 30});
    const auto state = c.toc(slot::button_state);
    c.set({{31, state}, {30, event}, {4, 0x6d62746e}, {6, 0}, {9, state}, {3, event}, {7, 2}, {5, 0x6d62746e}, {8, 0}});
    toolbox(c, frame, 0x10780ee8, 0x10770800);  // GetEventParameter('mbtn', 'mbtn')
    c.set(0, c.u16(state));
    if (compare(c, std::int32_t(c.r(0)), 1) == 0) {
        c.set({{3, event}, {9, frame.at(0x38)}, {4, 0x6b6d6f64}, {5, 0x6d61676e}, {6, 0}, {7, 4}, {8, 0}});
        toolbox(c, frame, 0x10780ee8, 0x10770838);  // GetEventParameter('kmod', 'magn')
        c.set({{3, c.toc(slot::right_button_modifiers)}, {4, c.u32(frame.at(0x38))}});
        c.set(0, c.r(4) & c.u32(c.r(3)));
        if (compare(c, std::int32_t(c.r(0))) != 0) {
            c.set(0, 2);
            c.put16(state, 2);
        }
    }
    return std::uint32_t(event_not_handled);
}

std::uint32_t bounds_event(Cpu& c, std::uint32_t, std::uint32_t, std::uint32_t) {
    Frame frame(c, 0x10770880, 0x40, {});
    c.set({{3, 0}, {4, 0}, {5, 0x31f}, {6, 0x257}});
    c.call(0x107692f0, 0x107708a0);  // redraws the 800 x 600 screen
    return 0;
}

void menu_command(Cpu& c, std::uint32_t, std::uint32_t choice) {
    Frame frame(c, 0x107708c0, 0x40, {});
    if (compare(c, std::int32_t(choice)) == 0) return;
    c.set({{0, shift_right_16(c, choice)}, {4, choice & 0xffff}});
    c.set(0, std::uint32_t(std::int16_t(c.r(0))));
    const auto menu = std::int32_t(c.r(0));
    const auto against_edit = compare(c, menu, 3);
    c.set(4, std::uint32_t(std::int16_t(c.r(4))));
    const auto item = c.r(4);
    if (against_edit == 0) {
        c.link(0x10770918);
        edit_menu(c, item);
    } else if (against_edit > 0) {
        if (compare(c, menu, 5) < 0) {
            c.link(0x10770928);
            debug_menu(c);
        }
    } else if (const auto against_apple = compare(c, menu, 1); against_apple == 0) {
        c.link(0x10770910);
        apple_menu(c, item);
    } else if (against_apple > 0) {
        c.link(0x10770920);
        file_menu(c, item);
    }
    c.set(3, 0);
    toolbox(c, frame, 0x10780708, 0x10770930);  // HiliteMenu(0)
}

void apple_menu(Cpu& c, std::uint32_t item) {
    c.set(0, std::uint32_t(std::int16_t(item)));
    if (compare(c, std::int32_t(c.r(0)), 1) == 0) request_screen(c, 3, request_about);
}

void file_menu(Cpu& c, std::uint32_t item) {
    Frame frame(c, 0x10770970, 0x40, {});
    c.set({{3, c.toc(slot::game)}, {0, std::uint32_t(std::int16_t(item))}});
    const auto number = std::int32_t(c.r(0));
    if (const auto against_print = compare(c, number, 3); against_print == 0) {
        c.set(3, c.toc(slot::settings));
        c.call(0x1074cc30, 0x107709d0);
    } else if (against_print > 0) {
        if (compare(c, number, 5) == 0) request_screen(c, 3, request_quit);
    } else if (compare(c, number, 1) == 0) request_screen(c, 3, request_save);
}

void edit_menu(Cpu& c, std::uint32_t item) {
    Frame frame(c, 0x107709e0, 0x40, {});
    c.set(0, std::uint32_t(std::int16_t(item)));
    if (compare(c, std::int32_t(c.r(0)), 8) != 0) return;
    c.link(0x10770a00);
    about_box(c);
}

void debug_menu(Cpu&) {}
void activate_event(Cpu&) {}

void os_event(Cpu& c, std::uint32_t event) {
    c.set({{3, c.u32(event + event_record::message)}, {4, c.toc(slot::foreground)}});
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(c.r(3) >> 24))));
    if (compare(c, std::int32_t(c.r(0)), 1) != 0) return;  // suspendResumeMessage
    c.set(0, c.r(3) & 1);
    const bool resume = compare(c, std::int32_t(c.r(0))) != 0;
    c.set({{3, resume ? 0u : 0x1eu}, {0, resume ? 1u : 0u}});
    c.put32(at(c, global::sleep_ticks), c.r(3));
    c.put8(c.r(4), c.r(0));
}

void high_level_event(Cpu& c, std::uint32_t) {
    Frame frame(c, 0x10770a80, 0x40, {});
    toolbox(c, frame, 0x10780528, 0x10770a90);  // AEProcessAppleEvent
}

std::uint32_t open_application(Cpu& c) {
    c.set(0, 1);
    c.put8(at(c, global::opened), 1);
    return 0;
}

std::uint32_t open_documents(Cpu& c, std::uint32_t) {
    Frame frame(c, 0x10770ac0, 0x1a0, {.first_gpr = 31});
    const auto list = frame.at(0x44), spec = frame.at(0x4c), suffix = frame.at(0x3c), path = frame.at(0x94);
    c.set({{4, 0x2d2d2d2d}, {5, 0x6c697374}, {31, c.toc(slot::pool)}, {6, list}});
    toolbox(c, frame, 0x1077fb80, 0x10770aec);  // AEGetParamDesc(keyDirectObject, typeAEList)
    if (!test16(c, c.r(3))) {
        c.set({{3, list}, {4, spec}});
        c.call(0x107681d0, 0x10770b04);  // the list's first FSSpec
        if (!test16(c, c.r(3))) {
            // The file name's last three characters, as a Pascal string.
            c.set(3, c.u8(spec + fsspec::name));
            if (compare_logical(c, c.r(3), 3) > 0) {
                c.set({{4, 3}, {0, spec}, {3, c.r(3) + 4}});
                c.put8(suffix, 3);
                c.set({{3, c.r(0) + c.r(3)}, {4, suffix + 1}, {5, 3}});
                toolbox(c, frame, 0x1077fc28, 0x10770b38);  // BlockMoveData
            } else {
                c.set(0, 0);
                c.put8(suffix, 0);
            }
            const auto is = [&](std::uint32_t text, std::uint32_t back) {
                c.set({{3, suffix}, {4, c.r(31) + text}, {5, 0}, {6, 0}});
                toolbox(c, frame, 0x1077fc40, back);  // EqualString
                return test_byte(c, c.r(3));
            };
            if (is(0x83, 0x10770b5c)) {
                // .cfg: each document becomes an AUTO_SETUP line of quick_start.cfg.
                c.set({{3, c.r(31) + 0x87}, {4, c.r(31) + 0x97}});
                c.call(0x107768f0, 0x10770b74);  // fopen("quick_start.cfg", "w")
                c.set(31, c.r(3));
                if (compare(c, std::int32_t(c.r(3))) != 0) {
                    c.set({{4, c.toc(slot::quick_start_callback)}, {5, c.r(31)}, {3, list}});
                    c.call(0x10768080, 0x10770b8c);  // for each document
                    c.set(3, c.r(31));
                    c.call(0x107766f0, 0x10770b94);  // fclose
                    c.set({{3, c.toc(slot::quick_start)}, {0, 1}});
                    c.put8(c.r(3), 1);
                }
            } else if (is(0x99, 0x10770bb8)) {
                // .rgn: each document's path goes into the setup text.
                c.set({{4, c.toc(slot::setup_callback)}, {3, list}, {5, 0}});
                c.call(0x10768080, 0x10770bd4);  // for each document
            } else if (is(0x9d, 0x10770bec)) {
                // .sav: the game starts with -load "<path>".
                c.set({{3, spec}, {4, frame.at(0x38)}, {5, frame.at(0x40)}});
                c.call(0x1076eaf0, 0x10770c08);  // its full path, in a handle
                if (!test16(c, c.r(3))) {
                    c.set({{4, path}, {5, std::uint32_t(c.i16(frame.at(0x38)))}, {3, c.u32(c.u32(frame.at(0x40)))}});
                    toolbox(c, frame, 0x1077fc28, 0x10770c24);  // BlockMoveData
                    c.set({{0, std::uint32_t(c.i16(frame.at(0x38)))}, {4, path}, {5, 0}, {3, c.u32(frame.at(0x40))}});
                    c.put8(c.r(4) + c.r(0), 0);
                    toolbox(c, frame, 0x107800c0, 0x10770c40);  // DisposeHandle
                    c.set(3, word(c, global::command_line));
                    if (compare_logical(c, c.r(3)) != 0) {
                        c.set(4, c.r(31) + 0xa1);
                        c.call(0x1077dc60, 0x10770c58);  // strcat
                        c.set({{3, word(c, global::command_line)}, {4, path}});
                        c.call(0x1077dc60, 0x10770c64);
                        c.set({{3, word(c, global::command_line)}, {4, c.r(31) + 0xaa}});
                        c.call(0x1077dc60, 0x10770c70);
                    }
                }
            }
        }
    }
    c.set(0, 1);
    c.put8(at(c, global::opened), 1);
    return std::uint32_t(ae_not_handled);
}

std::uint32_t print_documents(Cpu&) { return std::uint32_t(ae_not_handled); }

std::uint32_t quit_application(Cpu& c) {
    request_screen(c, 4, request_quit);
    return std::uint32_t(ae_not_handled);
}

void run_script(Cpu& c, std::uint32_t text) {
    Frame frame(c, 0x10770cc0, 0x140, {});
    // The cursor lives in the caller's parameter area (sp+0x158 is its sp+0x18).
    c.put32(frame.at(0x158), text);
    for (;;) {
        c.set({{3, frame.at(0x158)}, {4, frame.at(0x38)}, {5, 0x100}});
        c.call(0x1074c6e0, 0x10770d00);  // the next line, advancing the cursor
        if (!test_byte(c, c.r(3))) return;
        c.set({{3, frame.at(0x38)}, {4, 0}});
        c.call(0x1076b2f0, 0x10770ce4);  // parse the command
        if (compare_logical(c, c.r(3)) != 0) c.call(0x1076b7e0, 0x10770cf0);  // and run it
    }
}

void speak(Cpu& c, std::uint32_t text) {
    Frame frame(c, 0x10770d20, 0x50, {.first_gpr = 28});
    c.set({{31, c.toc(slot::timer)}, {28, text}});
    c.set(0, c.u8(at(c, global::speech_available)));
    if (!test8(c, c.r(0))) return;
    c.set(3, c.toc(slot::speech_wanted));
    c.set(0, c.u8(c.r(3)));
    if (!test8(c, c.r(0))) return;
    c.set(0, word(c, global::speech_channel));
    if (compare_logical(c, c.r(0)) == 0) {
        c.set({{4, at(c, global::speech_channel)}, {3, 0}});
        toolbox(c, frame, 0x10780ff0, 0x10770d78);  // NewSpeechChannel
        if (test16(c, c.r(3))) {
            c.set(0, 0);
            c.put32(at(c, global::speech_channel), 0);
        }
    }
    c.set(0, word(c, global::speech_channel));
    if (compare_logical(c, c.r(0)) == 0) return;
    c.set(3, c.r(28));
    c.call(0x1077dbd0, 0x10770da0);  // strlen
    c.set(30, c.r(3));
    c.set(3, c.r(30) + 1);
    c.call(0x10775130, 0x10770dac);  // malloc
    c.set({{0, c.r(3)}, {3, c.r(28)}, {29, c.r(3)}});
    c.set({{5, c.r(30)}, {4, c.r(29)}});
    toolbox(c, frame, 0x1077fc28, 0x10770dc4);  // BlockMoveData
    c.set(0, 0x2e);
    c.put8(c.r(29) + c.r(30), 0x2e);
    // Earlier speech finishes first, with the events going on; the game's
    // clock is put back afterwards.
    c.set(28, c.u32(c.r(31)));
    for (;;) {
        toolbox(c, frame, 0x10781008, 0x10770de0);  // SpeechBusy
        if (compare(c, std::int16_t(c.r(3))) <= 0) break;
        c.link(0x10770ddc);
        pump_events(c);
    }
    c.set(3, word(c, global::spoken_text));
    c.put32(c.r(31), c.r(28));
    if (compare_logical(c, c.r(3)) != 0) c.call(0x10775170, 0x10770e00);  // free
    c.put32(at(c, global::spoken_text), c.r(29));
    c.set({{4, c.r(29)}, {3, word(c, global::speech_channel)}, {5, c.r(30)}});
    toolbox(c, frame, 0x10781020, 0x10770e14);  // SpeakText
}

void about_box(Cpu& c) {
    Frame frame(c, 0x10770e40, 0x50, {.first_gpr = 30});
    c.set({{3, 0x80}, {4, 0}, {5, std::uint32_t(-1)}});
    toolbox(c, frame, 0x10780678, 0x10770e64);  // GetNewDialog(128)
    c.set(31, c.r(3));
    if (compare(c, std::int32_t(c.r(3))) == 0) return;
    toolbox(c, frame, 0x10780690, 0x10770e74);  // GetDialogWindow
    toolbox(c, frame, 0x107806a8, 0x10770e7c);  // ShowWindow
    for (c.set(30, 0); !test8(c, c.r(30));) {
        c.set({{4, frame.at(0x38)}, {3, 0}});
        toolbox(c, frame, 0x107806c0, 0x10770e94);  // ModalDialog
        c.set(0, std::uint32_t(c.i16(frame.at(0x38))));
        const auto item = std::int32_t(c.r(0));
        if (compare(c, item, 3) < 0 && compare(c, item, 1) >= 0) c.set(30, 1);  // items 1 and 2 close it
    }
    c.set(3, c.r(31));
    toolbox(c, frame, 0x107806d8, 0x10770ec4);  // DisposeDialog
}

std::uint32_t write_quick_start(Cpu& c, std::uint32_t spec, std::uint32_t file) {
    Frame frame(c, 0x10770f60, 0x150, {.first_gpr = 31});
    c.set(31, file);
    if (compare(c, std::int32_t(file)) == 0 || compare_logical(c, spec) == 0) return 0;
    c.set({{4, frame.at(0x38)}, {5, frame.at(0x3c)}});
    c.call(0x1076eaf0, 0x10770f8c);  // the document's full path, in a handle
    if (test16(c, c.r(3))) return 0;
    c.set({{3, std::uint32_t(c.i16(frame.at(0x38)))}, {0, 0x100}});
    if (compare(c, std::int32_t(c.r(3)), 0x100) < 0) c.set(0, c.r(3));
    c.set({{4, frame.at(0x40)}, {5, std::uint32_t(std::int16_t(c.r(0)))}, {3, c.u32(c.u32(frame.at(0x3c)))}});
    toolbox(c, frame, 0x1077fc28, 0x10770fbc);  // BlockMoveData, at most 256 bytes
    // The terminator goes at the full length, even past 256 (as the original).
    c.set({{0, std::uint32_t(c.i16(frame.at(0x38)))}, {4, frame.at(0x40)}, {5, 0}, {3, c.u32(frame.at(0x3c))}});
    c.put8(c.r(4) + c.r(0), 0);
    toolbox(c, frame, 0x107800c0, 0x10770fd8);  // DisposeHandle
    c.set({{3, c.r(31)}, {5, frame.at(0x40)}, {4, c.toc(slot::pool) + 0xf7}});
    c.call(0x1077b610, 0x10770ff0);  // fprintf("AUTO_SETUP=%s\n\r")
    return 0;
}

std::uint32_t add_setup_line(Cpu& c, std::uint32_t spec) {
    Frame frame(c, 0x10771010, 0x50, {.first_gpr = 29});
    if (compare_logical(c, spec) == 0) return 0;
    c.set({{4, frame.at(0x38)}, {5, frame.at(0x3c)}});
    c.call(0x1076eaf0, 0x1077103c);  // the document's full path, in a handle
    if (test16(c, c.r(3))) return 0;
    // r29 the new text, r30 its size, r31 the old text's length.
    c.set(3, word(c, global::setup_text));
    if (compare_logical(c, c.r(3)) != 0) {
        c.call(0x1077dbd0, 0x10771054);  // strlen
        c.set(31, c.r(3));
    } else c.set(31, 0);
    c.set(3, std::uint32_t(c.i16(frame.at(0x38))));
    c.set(30, c.r(31) + c.r(3) + 2);
    c.set(3, c.r(30));
    toolbox(c, frame, 0x1077fc70, 0x10771074);  // NewPtr
    c.set(29, c.r(3));
    if (compare(c, std::int32_t(c.r(3))) == 0) return 0;
    if (compare(c, std::int32_t(c.r(31))) != 0) {
        c.set({{3, word(c, global::setup_text)}, {4, c.r(29)}, {5, c.r(31)}});
        toolbox(c, frame, 0x107800f0, 0x10771098);  // BlockMove
    }
    c.set({{4, c.r(29) + c.r(31)}, {5, std::uint32_t(c.i16(frame.at(0x38)))}, {3, c.u32(c.u32(frame.at(0x3c)))}});
    toolbox(c, frame, 0x1077fc28, 0x107710b0);  // BlockMoveData
    c.set(3, c.u32(frame.at(0x3c)));
    toolbox(c, frame, 0x107800c0, 0x107710bc);  // DisposeHandle
    c.set({{3, c.r(29) + c.r(30)}, {0, 0xd}});
    c.put8(c.r(3) - 2, 0xd);  // "\r"
    c.set(0, 0);
    c.put8(c.r(3) - 1, 0);
    c.set(3, word(c, global::setup_text));
    if (compare_logical(c, c.r(3)) != 0) toolbox(c, frame, 0x1077fc58, 0x107710e4);  // DisposePtr
    c.put32(at(c, global::setup_text), c.r(29));
    return 0;
}
}
