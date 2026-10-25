#include "main_program_detail.hpp"
#include <array>

namespace cm::aot::main_program {
using namespace detail;
namespace {

// A mouse-down in the event pump, by FindWindow's part.
void press(Cpu& c, const Frame& frame, EventRecord event) {
    Toolbox os{c, frame};
    Shared shared{c};
    const Field<std::uint32_t> window{c, frame.at(0x3c)};
    const auto where = event.where();
    const auto pressed = os.FindWindow(where, window.address(), 0x1076fff0);
    const auto is = [&](Field<std::uint32_t> other) { return compare_logical(c, window, other) == 0; };
    if (const auto against_drag = compare(c, pressed, part::drag); against_drag == 0) {
        // The title bar of a window other than the dialog-like one: drag it.
        if (is(shared.other_window())) return;
        const auto bounds = frame.at(0x40);
        os.GetRegionBounds(os.GetGrayRgn(0x10770070), bounds, 0x1077007c);
        os.SelectWindow(window, 0x10770088);
        os.DragWindow(window, where, bounds, 0x1077009c);
    } else if (against_drag > 0) {
        // The game window's close box asks the game to quit.
        if (compare(c, pressed, part::go_away) == 0 && nonzero(c, os.TrackGoAway(window, where, 0x107700b0)) &&
            is(shared.game_window()))
            shared.screen_request() = request_quit;
    } else if (const auto against_system = compare(c, pressed, part::system_window); against_system > 0) {
        if (is(shared.game_window())) {
            enter(c, 0x10770040, {event.address});
            content_click(c, event.address);
        }
        if (!is(shared.other_window())) os.SelectWindow(window, 0x10770054);
    } else if (against_system < 0 && compare(c, pressed, part::menu_bar) >= 0) {
        enter(c, 0x107700e4, {event.address});
        adjust_menus(c, event.address);
        const auto choice = os.MenuSelect(where, 0x107700ec);
        enter(c, 0x10770100, {event.address, choice});
        menu_command(c, event.address, choice);
    }
}
}

void pump_events(Cpu& c) {
    Frame frame(c, 0x1076fe80, 0x80, {.first_gpr = 24});
    Toolbox os{c, frame};
    Program program{c};
    Globals g{c};
    Shared shared{c};
    // Resolved once: the loop runs this every turn.
    const auto control_down = shared.control_down(), left_held = shared.left_held(), right_held = shared.right_held();
    const auto pending_scroll = shared.pending_scroll(), idle_wait = shared.idle_wait();
    const auto game_window = shared.game_window(), other_window = shared.other_window();
    const auto game = c.toc(slot::game);
    keep(c, {{24, control_down.address()}, {25, pending_scroll.address()}, {26, idle_wait.address()},
             {27, game_window.address()}, {28, game}, {29, right_held.address()}, {30, other_window.address()},
             {31, left_held.address()}});
    const Field<std::uint32_t> saved_port{c, frame.at(0x38)};
    os.GetPort(saved_port.address(), 0x1076feb8);
    os.SetPort(os.GetWindowPort(game_window, 0x1076fec4), 0x1076fecc);
    program.backdrop_shown(0x1076fed4);  // asked, and the answer dropped
    const auto now = program.milliseconds(0x1076fed8);
    const auto timer = shared.timer();
    const std::int32_t wait = idle_wait;
    c.set({{4, std::uint32_t(wait)}, {5, timer.address()}});  // the original's loads
    const bool waiting = nonzero(c, wait);
    timer = now;
    if (waiting) {
        // Turns to wait before events are read again; the port stays the
        // game window's (the original returns without restoring it).
        const auto left = std::int32_t(std::uint32_t(wait) - 1);
        idle_wait = left;
        if (!nonzero(c, left)) left_held = 0;
        return;
    }
    const std::int32_t scroll = pending_scroll;
    c.set(5, std::uint32_t(scroll));  // loaded into r5, the scroll's third argument
    if (const auto direction = compare(c, scroll); direction != 0) {
        // A wheel turn the Carbon handler noted: lines, or pages with control.
        const bool pages = nonzero(c, control_down);
        if (direction > 0) program.scroll(game, pages ? 3 : 1, std::uint32_t(scroll), 0x1076ff30);  // up
        else program.scroll(game, pages ? 2 : 0, 0u - std::uint32_t(scroll), 0x1076ff54);         // down
        pending_scroll = 0;
        return;
    }
    enter(c, 0x1076ff64);
    track_mouse(c);
    const EventRecord event{c, frame.at(0x48)};
    if (nonzero(c, os.WaitNextEvent(0xffff, event, g.sleep_ticks(), g.mouse_region(), 0x1076ff7c))) {
        const std::uint16_t what = event.what();
        const auto handle = [&](void (*handler)(Cpu&, std::uint32_t), std::uint32_t back) {
            enter(c, back, {event.address});
            handler(c, event.address);
        };
        // The original's jump table covers kinds 0-23.
        if (compare_logical(c, what, 23) <= 0) switch (EventKind(what)) {
        case EventKind::mouse_down: press(c, frame, event); break;
        case EventKind::mouse_up: enter(c, 0x1076ffe0, {event.address}); mouse_up(c); break;
        case EventKind::key_down: handle(key_down, 0x1076ffd4); break;
        case EventKind::activate: enter(c, 0x1076ffac, {event.address}); activate_event(c); break;
        case EventKind::os: handle(os_event, 0x1076ffbc); break;
        case EventKind::high_level: handle(high_level_event, 0x1076ffc8); break;
        default: break;  // null, key up, auto-key, update, disk and the rest
        }
    } else {
        // No event: a button held in the game is let go when Button() says so.
        const bool held = nonzero(c, left_held) || nonzero(c, right_held);
        if (held && compare(c, shared.button_state(), 1) == 0 && !nonzero(c, os.Button(0x10770130))) {
            left_held = 0;
            right_held = 0;
        }
    }
    os.SetPort(saved_port, 0x10770150);
}

void poll_high_level_events(Cpu& c) {
    Frame frame(c, 0x10770170, 0x50, {});
    Toolbox os{c, frame};
    Globals g{c};
    const EventRecord event{c, frame.at(0x38)};
    if (!nonzero(c, os.WaitNextEvent(0x400, event, g.sleep_ticks(), g.mouse_region(), 0x10770190))) return;
    if (compare(c, event.what(), std::int32_t(EventKind::high_level)) != 0) return;
    enter(c, 0x107701b4, {event.address});
    high_level_event(c, event.address);
}

void track_mouse(Cpu& c) {
    Frame frame(c, 0x107701d0, 0x50, {});
    Toolbox os{c, frame};
    Shared shared{c};
    // GetMouse, pinned to the 800 x 600 screen.
    const auto screen = c.toc(slot::screen_rect), bounds = frame.at(0x3c);
    const Field<std::uint32_t> mouse{c, frame.at(0x38)};
    c.put32(bounds, c.u32(screen));
    c.put32(bounds + 4, c.u32(screen + 4));
    os.GetMouse(mouse.address(), 0x107701f8);
    const auto point = os.PinRect(bounds, mouse, 0x10770208);  // v << 16 | h
    shared.mouse_x() = std::int32_t(point & 0xffff);
    shared.mouse_y() = std::int32_t(point) >> 16;
    leave(c, {{4, shared.mouse_y().address()}, {5, shared.mouse_x().address()}, {6, point & 0xffff}});
}

void adjust_menus(Cpu& c, std::uint32_t) {
    Frame frame(c, 0x10770240, 0x50, {.first_gpr = 29});
    Toolbox os{c, frame};
    Shared shared{c};
    const Kept<31> pool{c, c.toc(slot::pool)};
    const Kept<29> file{c, os.GetMenu(2, 0x10770264)};
    if (nonzero(c, std::int32_t(file))) {
        // Quit (item 5) and Save (item 1) say when the game has their
        // request pending, and are disabled meanwhile.
        const auto item = [&](std::uint16_t number, bool pending, std::uint32_t text, std::uint32_t enable_back,
                              std::uint32_t text_back) {
            if (pending) os.DisableMenuItem(file, number, enable_back);
            else os.EnableMenuItem(file, number, enable_back);
            os.SetMenuItemText(file, std::int16_t(number), pool + text, text_back);
        };
        keep(c, {{30, shared.screen_request().address()}});
        if (compare(c, shared.screen_request(), request_quit) == 0) item(5, true, text::quit_requested, 0x1077028c, 0x107702a0);
        else item(5, false, text::quit, 0x107702b0, 0x107702c4);
        if (compare(c, shared.screen_request(), request_save) == 0) item(1, true, text::save_requested, 0x107702e0, 0x107702f4);
        else item(1, false, text::save, 0x10770308, 0x1077031c);
    }
    const Kept<30> debug{c, os.GetMenu(4, 0x10770328)};
    if (nonzero(c, std::int32_t(debug)))
        for (const auto& [number, back] : std::array<std::pair<std::uint16_t, std::uint32_t>, 3>{
                 {{1, 0x1077033c}, {3, 0x1077034c}, {2, 0x1077035c}}})
            os.DisableMenuItem(debug, number, back);
}

void key_down(Cpu& c, std::uint32_t address) {
    Frame frame(c, 0x10770380, 0xb0, {.first_gpr = 28});
    Toolbox os{c, frame};
    Shared shared{c};
    const EventRecord event{c, address};
    const std::uint32_t message = event.message();
    const std::uint32_t character = message & 0xff, code = (message >> 8) & 0x7f;
    keep(c, {{28, address}, {29, character}, {30, c.toc(slot::game)}, {31, code}});
    // "key ascii: %c (%d), raw: %x": a debugging line nothing shows.
    Program program{c};
    program.sprintf(frame.at(0x38), c.toc(slot::pool) + text::key_format, character, character, code, 0x107703cc);
    if (nonzero(c, event.modifiers() & (modifier::command | modifier::option))) {
        enter(c, 0x107703e0, {character});
        if (nonzero(c, std::int8_t(result(c, speech_key(c, character))))) return;
    }
    if (nonzero(c, event.modifiers() & modifier::command)) {
        const auto choice = os.MenuKey(std::uint8_t(character), 0x107703fc);
        c.set(4, choice);  // the original's move
        if (nonzero(c, std::int32_t(choice))) {
            enter(c, 0x10770410, {address, choice});
            menu_command(c, address, choice);
            return;
        }
    }
    // The navigation keys move through the game's lists (the original's jump
    // table for key codes 0x73-0x7e).
    struct Navigation {
        std::uint32_t code, action, back;
    };
    constexpr Navigation keys[]{{0x7e, 1, 0x10770444}, {0x7d, 0, 0x10770458}, {0x74, 3, 0x1077046c},  // up, down, page up
                                {0x79, 2, 0x10770480}, {0x73, 5, 0x10770494}, {0x77, 4, 0x107704a8}}; // page down, home, end
    if (compare_logical(c, code - 0x73, 0xb) <= 0)
        for (const auto& key : keys)
            if (key.code == code) {
                program.scroll(c.toc(slot::game), key.action, 1, key.back);
                return;
            }
    // Any other key is left for the game's screens.
    shared.key_code() = code;
    shared.key_character() = std::int8_t(character);
    leave(c, {{4, shared.key_code().address()}, {3, shared.key_character().address()}});
}

std::uint32_t speech_key(Cpu& c, std::uint32_t character) {
    Frame frame(c, 0x107704e0, 0x40, {});
    if (compare(c, std::int32_t(character & 0xff), 's') != 0) return 0;
    auto wanted = Shared{c}.speech_wanted();
    wanted = std::int8_t(wanted.get() == 0);
    if (!nonzero(c, wanted)) release_speech(c, frame, 0x10770530, 0x10770544);
    return 1;
}

void content_click(Cpu& c, std::uint32_t address) {
    Frame frame(c, 0x10770570, 0x50, {.first_gpr = 27});
    Shared shared{c};
    keep(c, {{27, address}, {28, shared.button_state().address()}, {29, shared.control_down().address()},
             {30, shared.left_down().address()}, {31, shared.right_down().address()}});
    enter(c, 0x10770598);
    if (nonzero(c, std::int8_t(result(c, aqua_menus(c))))) {
        // The Carbon click handler has set the button state.
        const std::uint16_t state = shared.button_state();
        shared.right_down() = std::int8_t(state == 2);
        shared.left_down() = std::int8_t(state == 1);
        shared.control_down() = std::int8_t(state == 3);
    } else {
        // A click with a right-button modifier is the right button.
        const std::uint32_t modifiers = EventRecord{c, address}.modifiers(), bits = modifiers & shared.right_button_modifiers();
        shared.control_down() = 0;
        shared.button_state() = 1;
        shared.right_down() = std::int8_t(bits != 0);
        shared.left_down() = std::int8_t(shared.right_down().get() == 0);
    }
    shared.left_held() = shared.left_down().get();
    shared.right_held() = shared.right_down().get();
    leave(c, {{5, std::uint8_t(shared.left_down())}, {4, shared.left_held().address()}, {3, shared.right_held().address()}});
}

void mouse_up(Cpu& c) {
    Shared shared{c};
    shared.left_held() = 0;
    shared.right_held() = 0;
    leave(c, {{4, shared.left_held().address()}, {3, shared.right_held().address()}});
}

std::uint32_t window_event(Cpu& c, std::uint32_t handler, std::uint32_t event, std::uint32_t data) {
    Frame frame(c, 0x10770670, 0x50, {.first_gpr = 29});
    Toolbox os{c, frame};
    keep(c, {{29, handler}, {30, event}, {31, data}});
    const auto event_class = std::int32_t(os.GetEventClass(event, 0x1077069c));
    const auto kind = [&](std::uint32_t back) { return std::int32_t(os.GetEventKind(event, back)); };
    const auto pass_on = [&](std::uint32_t (*callee)(Cpu&, std::uint32_t, std::uint32_t, std::uint32_t), std::uint32_t back) {
        enter(c, back, {handler, event, data});
        return result(c, callee(c, handler, event, data));
    };
    c.set(4, 0x77690000);  // the original builds the class constants in r4
    const auto against_window = compare(c, event_class, std::int32_t(four_char('w', 'i', 'n', 'd')));
    if (against_window > 0) return std::uint32_t(event_not_handled);
    if (against_window < 0) {
        c.set(4, 0x6d6f0000);
        if (compare(c, event_class, std::int32_t(four_char('m', 'o', 'u', 's'))) != 0) return std::uint32_t(event_not_handled);
        if (compare(c, kind(0x107706d0), 10) == 0) return pass_on(wheel_event, 0x107706f0);  // kEventMouseWheelMoved
        // Other mouse events fall into the window events' test (the original
        // asks for the kind again).
    }
    const auto window_kind = kind(0x107706fc);
    const auto against_click = compare(c, window_kind, 0x55);
    if (against_click == 0) return pass_on(click_event, 0x1077073c);
    if (against_click < 0 && compare(c, window_kind, 2) == 0) return pass_on(bounds_event, 0x10770728);
    return std::uint32_t(event_not_handled);
}

std::uint32_t wheel_event(Cpu& c, std::uint32_t, std::uint32_t event, std::uint32_t) {
    Frame frame(c, 0x10770760, 0x40, {});
    Toolbox os{c, frame};
    const Field<std::int32_t> delta{c, frame.at(0x38)};
    os.GetEventParameter(event, four_char('m', 'w', 'd', 'l'), four_char('l', 'o', 'n', 'g'), 0, 4, 0, delta.address(), 0x10770794);
    auto scroll = Shared{c}.pending_scroll();
    c.set({{4, std::uint32_t(std::int32_t(scroll))}, {5, scroll.address()}});  // the original's loads
    scroll = std::int32_t(std::uint32_t(scroll.get()) + std::uint32_t(delta.get()));  // wraps, as the original's add
    return std::uint32_t(event_not_handled);
}

std::uint32_t click_event(Cpu& c, std::uint32_t, std::uint32_t event, std::uint32_t) {
    Frame frame(c, 0x107707c0, 0x50, {.first_gpr = 30});
    Toolbox os{c, frame};
    Shared shared{c};
    auto state = shared.button_state();
    keep(c, {{31, state.address()}, {30, event}});
    os.GetEventParameter(event, four_char('m', 'b', 't', 'n'), four_char('m', 'b', 't', 'n'), 0, 2, 0, state.address(), 0x10770800);
    if (compare(c, state, 1) == 0) {
        // A left click with a right-button modifier is the right button.
        const Field<std::uint32_t> modifiers{c, frame.at(0x38)};
        os.GetEventParameter(event, four_char('k', 'm', 'o', 'd'), four_char('m', 'a', 'g', 'n'), 0, 4, 0, modifiers.address(), 0x10770838);
        c.set({{3, shared.right_button_modifiers().address()}, {4, modifiers}});  // the original's loads
        if (nonzero(c, std::int32_t(modifiers & shared.right_button_modifiers()))) state = 2;
    }
    return std::uint32_t(event_not_handled);
}

std::uint32_t bounds_event(Cpu& c, std::uint32_t, std::uint32_t, std::uint32_t) {
    Frame frame(c, 0x10770880, 0x40, {});
    Program{c}.redraw(0, 0, 799, 599, 0x107708a0);  // the whole 800 x 600 screen
    return 0;
}

void menu_command(Cpu& c, std::uint32_t, std::uint32_t choice) {
    Frame frame(c, 0x107708c0, 0x40, {});
    Toolbox os{c, frame};
    if (!nonzero(c, std::int32_t(choice))) return;
    const auto menu = std::int16_t(std::int32_t(choice) >> 16), item = std::int16_t(choice);
    const auto against_edit = compare(c, menu, 3);
    c.set(4, extended(item));  // each menu's routine takes the item in r4
    if (against_edit == 0) {
        enter(c, 0x10770918);
        edit_menu(c, extended(item));
    } else if (against_edit > 0) {
        if (compare(c, menu, 5) < 0) {
            enter(c, 0x10770928);
            debug_menu(c);
        }
    } else if (const auto against_apple = compare(c, menu, 1); against_apple == 0) {
        enter(c, 0x10770910);
        apple_menu(c, extended(item));
    } else if (against_apple > 0) {
        enter(c, 0x10770920);
        file_menu(c, extended(item));
    }
    os.HiliteMenu(0, 0x10770930);
}

void apple_menu(Cpu& c, std::uint32_t item) {
    if (compare(c, std::int16_t(item), 1) == 0) Shared{c}.screen_request() = request_about;  // About
}

void file_menu(Cpu& c, std::uint32_t item) {
    Frame frame(c, 0x10770970, 0x40, {});
    Shared shared{c};
    const auto number = std::int16_t(item);
    if (const auto against_three = compare(c, number, 3); against_three == 0) {
        Program{c}.settings_command(c.toc(slot::settings), 0x107709d0);
    } else if (against_three > 0) {
        if (compare(c, number, 5) == 0) shared.screen_request() = request_quit;  // Quit
    } else if (compare(c, number, 1) == 0) shared.screen_request() = request_save;  // Save
}

void edit_menu(Cpu& c, std::uint32_t item) {
    Frame frame(c, 0x107709e0, 0x40, {});
    if (compare(c, std::int16_t(item), 8) != 0) return;
    enter(c, 0x10770a00);
    preferences_dialog(c);
}

void debug_menu(Cpu&) {}
void activate_event(Cpu&) {}

void os_event(Cpu& c, std::uint32_t address) {
    Shared shared{c};
    const std::uint32_t message = EventRecord{c, address}.message();
    leave(c, {{3, message}, {4, shared.foreground().address()}});
    if (compare(c, std::int8_t(message >> 24), 1) != 0) return;  // suspendResumeMessage only
    const bool resume = nonzero(c, message & 1);
    Globals{c}.sleep_ticks() = resume ? 0 : 30;  // ticks WaitNextEvent may sleep while in the background
    shared.foreground() = std::int8_t(resume);
    leave(c, {{3, resume ? 0u : 30u}});
}

void high_level_event(Cpu& c, std::uint32_t event) {
    Frame frame(c, 0x10770a80, 0x40, {});
    Toolbox{c, frame}.AEProcessAppleEvent(event, 0x10770a90);
}

std::uint32_t open_application(Cpu& c) {
    Globals{c}.opened() = 1;
    return 0;
}

std::uint32_t open_documents(Cpu& c, std::uint32_t event) {
    Frame frame(c, 0x10770ac0, 0x1a0, {.first_gpr = 31});
    Toolbox os{c, frame};
    Program program{c};
    Globals g{c};
    const auto pool = c.toc(slot::pool), list = frame.at(0x44), suffix = frame.at(0x3c), path = frame.at(0x94);
    const FSSpec spec{c, frame.at(0x4c)};
    Kept<31> held{c, pool};  // the pool, then the quick-start file
    if (!nonzero(c, os.AEGetParamDesc(event, four_char('-', '-', '-', '-'), four_char('l', 'i', 's', 't'), list, 0x10770aec))) {
        if (!nonzero(c, program.first_document(list, spec, 0x10770b04))) {
            // The document name's last three characters, as a Pascal string.
            const std::uint8_t length = spec.name_length();
            c.set(3, length);  // the original's load
            if (compare_logical(c, length, 3) > 0) {
                c.put8(suffix, 3);
                os.BlockMoveData(spec.name() + length - 2, suffix + 1, 3, 0x10770b38);
            } else c.put8(suffix, 0);
            const auto is = [&](std::uint32_t name, std::uint32_t back) { return nonzero(c, os.EqualString(suffix, pool + name, false, false, back)); };
            if (is(text::cfg, 0x10770b5c)) {
                // A .cfg: each document becomes an AUTO_SETUP line of quick_start.cfg.
                const auto file = program.fopen(pool + text::quick_start_file, pool + text::write_mode, 0x10770b74);
                held = file;
                if (nonzero(c, std::int32_t(file))) {
                    program.each_document(list, c.toc(slot::quick_start_callback), file, 0x10770b8c);  // write_quick_start
                    program.fclose(file, 0x10770b94);
                    Shared{c}.quick_start() = 1;
                    c.set(3, Shared{c}.quick_start().address());  // the original's store goes through r3
                }
            } else if (is(text::rgn, 0x10770bb8)) {
                // A .rgn: each document's path goes into the setup text.
                program.each_document(list, c.toc(slot::setup_callback), 0, 0x10770bd4);  // add_setup_line
            } else if (is(text::sav, 0x10770bec)) {
                // A .sav: the game starts with -load "<path>".
                const Field<std::int16_t> size{c, frame.at(0x38)};
                const Field<std::uint32_t> handle{c, frame.at(0x40)};
                if (!nonzero(c, program.full_path(spec, size.address(), handle.address(), 0x10770c08))) {
                    os.BlockMoveData(c.u32(handle), path, extended(size), 0x10770c24);
                    c.put8(path + extended(size), 0);
                    c.set({{4, path}, {5, 0}});  // the terminator's store
                    os.DisposeHandle(handle, 0x10770c40);
                    if (nonzero_logical(c, result(c, g.command_line())))
                        for (const auto& [part, back] : std::array<std::pair<std::uint32_t, std::uint32_t>, 3>{
                                 {{pool + text::load_option, 0x10770c58}, {path, 0x10770c64}, {pool + text::quote, 0x10770c70}}})
                            program.strcat(g.command_line(), part, back);
                }
            }
        }
    }
    g.opened() = 1;
    return std::uint32_t(ae_not_handled);
}

std::uint32_t print_documents(Cpu&) { return std::uint32_t(ae_not_handled); }

std::uint32_t quit_application(Cpu& c) {
    Shared{c}.screen_request() = request_quit;
    leave(c, {{4, c.toc(slot::game) + 0x260000}});  // from the original's address arithmetic
    return std::uint32_t(ae_not_handled);
}

void run_script(Cpu& c, std::uint32_t text) {
    Frame frame(c, 0x10770cc0, 0x140, {});
    // The cursor lives in the caller's parameter area (sp+0x158 is its sp+0x18).
    Field<std::uint32_t>{c, frame.at(0x158)} = text;
    const auto line = frame.at(0x38);
    Program program{c};
    while (nonzero(c, program.next_line(frame.at(0x158), line, 0x100, 0x10770d00)))
        if (const auto command = program.parse_command(line, 0, 0x10770ce4); nonzero_logical(c, command))
            program.run_command(command, 0x10770cf0);
}

void speak(Cpu& c, std::uint32_t text) {
    Frame frame(c, 0x10770d20, 0x50, {.first_gpr = 28});
    Toolbox os{c, frame};
    Program program{c};
    Globals g{c};
    Shared shared{c};
    keep(c, {{31, shared.timer().address()}});
    Kept<28> held{c, text};  // the text, then the saved clock
    if (!nonzero(c, g.speech_available())) return;
    c.set(3, shared.speech_wanted().address());  // the original's load
    if (!nonzero(c, shared.speech_wanted())) return;
    if (!nonzero_logical(c, g.speech_channel()) && nonzero(c, os.NewSpeechChannel(0, g.speech_channel().address(), 0x10770d78)))
        g.speech_channel() = 0;
    if (!nonzero_logical(c, g.speech_channel())) return;
    const Kept<30> length{c, program.strlen(text, 0x10770da0)};
    const Kept<29> copy{c, program.malloc(length + 1, 0x10770dac)};
    os.BlockMoveData(text, copy, length, 0x10770dc4);
    c.put8(copy + length, '.');
    // Earlier speech finishes first, the events going on meanwhile; the
    // game's clock is put back afterwards.
    const std::uint32_t clock = shared.timer();
    held = clock;
    while (compare(c, os.SpeechBusy(0x10770de0)) > 0) {
        enter(c, 0x10770ddc);
        pump_events(c);
    }
    shared.timer() = clock;
    if (const std::uint32_t old = g.spoken_text(); nonzero_logical(c, result(c, old))) program.free(old, 0x10770e00);
    g.spoken_text() = copy;
    os.SpeakText(g.speech_channel(), copy, length, 0x10770e14);
}

void preferences_dialog(Cpu& c) {
    Frame frame(c, 0x10770e40, 0x50, {.first_gpr = 30});
    Toolbox os{c, frame};
    const Kept<31> dialog{c, os.GetNewDialog(128, 0, std::uint32_t(-1), 0x10770e64)};  // "Preferences", in front
    if (!nonzero(c, std::int32_t(dialog))) return;
    os.ShowWindow(os.GetDialogWindow(dialog, 0x10770e74), 0x10770e7c);
    const Field<std::int16_t> item{c, frame.at(0x38)};
    Kept<30> closed{c, 0};
    while (!nonzero(c, closed)) {
        os.ModalDialog(0, item.address(), 0x10770e94);
        if (compare(c, item, 3) < 0 && compare(c, item, 1) >= 0) closed = 1;  // OK (1) and Cancel (2) close it
    }
    os.DisposeDialog(dialog, 0x10770ec4);
}

std::uint32_t write_quick_start(Cpu& c, std::uint32_t spec, std::uint32_t file) {
    Frame frame(c, 0x10770f60, 0x150, {.first_gpr = 31});
    Toolbox os{c, frame};
    Program program{c};
    const Kept<31> held{c, file};
    if (!nonzero(c, std::int32_t(file)) || !nonzero_logical(c, spec)) return 0;
    const Field<std::int16_t> length{c, frame.at(0x38)};
    const Field<std::uint32_t> handle{c, frame.at(0x3c)};
    const auto path = frame.at(0x40);
    if (nonzero(c, program.full_path(FSSpec{c, spec}, length.address(), handle.address(), 0x10770f8c))) return 0;
    // At most 256 bytes are copied; the terminator goes at the full length,
    // past them for a longer path (as the original).
    const std::int16_t copied = compare(c, length, 0x100) < 0 ? std::int16_t(length) : std::int16_t(0x100);
    os.BlockMoveData(c.u32(handle), path, extended(copied), 0x10770fbc);
    c.put8(path + extended(length), 0);
    c.set({{4, path}, {5, 0}});  // the terminator's store
    os.DisposeHandle(handle, 0x10770fd8);
    program.fprintf(file, c.toc(slot::pool) + text::auto_setup, path, 0x10770ff0);  // "AUTO_SETUP=%s\n\r"
    return 0;
}

std::uint32_t add_setup_line(Cpu& c, std::uint32_t spec) {
    Frame frame(c, 0x10771010, 0x50, {.first_gpr = 29});
    Toolbox os{c, frame};
    Program program{c};
    Globals g{c};
    if (!nonzero_logical(c, spec)) return 0;
    const Field<std::int16_t> length{c, frame.at(0x38)};
    const Field<std::uint32_t> handle{c, frame.at(0x3c)};
    if (nonzero(c, program.full_path(FSSpec{c, spec}, length.address(), handle.address(), 0x1077103c))) return 0;
    // The setup text grows by the path and a carriage return.
    std::uint32_t old_length = 0;
    if (const std::uint32_t old = g.setup_text(); nonzero_logical(c, result(c, old))) old_length = program.strlen(old, 0x10771054);
    const Kept<31> held_length{c, old_length};
    const Kept<30> size{c, old_length + extended(length) + 2};
    const Kept<29> text{c, os.NewPtr(size, 0x10771074)};
    if (!nonzero(c, std::int32_t(text))) return 0;
    if (nonzero(c, std::int32_t(old_length))) os.BlockMove(g.setup_text(), text, old_length, 0x10771098);
    os.BlockMoveData(c.u32(handle), text + old_length, extended(length), 0x107710b0);
    os.DisposeHandle(handle, 0x107710bc);
    c.put8(text + size - 2, '\r');
    c.put8(text + size - 1, 0);
    if (const std::uint32_t old = g.setup_text(); nonzero_logical(c, old)) os.DisposePtr(old, 0x107710e4);
    g.setup_text() = text;
    return 0;
}
}
