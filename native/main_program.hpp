#pragma once
// MAIN.CPP of the original (0x1076f2b0-0x10771110): main(), the start-up
// (process information, Apple-event handlers, menus, the preferences and
// application folders), the main loop around the game's step, the event
// pump with its mouse, key, menu and window handling, the Carbon event and
// Apple-event callbacks, the debugging script and speech helpers, and the
// shut-down. Each routine leaves what the original leaves (guest_abi.hpp);
// routine bounds are in tools/main_program_audit.py.
#include "guest_abi.hpp"

namespace cm::aot::main_program {
// MAIN.CPP's globals, at fixed offsets from the TOC (r2).
namespace global {
constexpr std::int32_t menu_checked = -0x354e;     // int8: menu_features has been asked
constexpr std::int32_t menu_features = -0x354c;    // int32: Gestalt('menu'), -1 until asked
constexpr std::int32_t setup_text = -0x3548;       // char*: AUTO_SETUP lines from dropped .rgn files
constexpr std::int32_t command_line = -0x3544;     // char[0x400]*: the game's start-up arguments
constexpr std::int32_t spoken_text = -0x3540;      // char*: the text being spoken
constexpr std::int32_t speech_channel = -0x353c;   // SpeechChannel, 0 when none
constexpr std::int32_t speech_available = -0x3538; // int8: Gestalt('ttsc') bit 0
constexpr std::int32_t mouse_region = -0x3534;     // RgnHandle passed to WaitNextEvent
constexpr std::int32_t sleep_ticks = -0x3530;      // WaitNextEvent sleep: 0 in front, 30 behind
constexpr std::int32_t opened = -0x352c;           // int8: the open-application/documents event came
constexpr std::int32_t running = -0x352b;          // int8: the main loop goes on
}

// 0x1076f2b0: main(): runs the game and returns 0.
void main(guest::Cpu& c);
// 0x1076f2e0: the program: start-up, the main loop, shut-down; the
// setjmp value (non-zero after a fatal exit).
std::uint32_t run(guest::Cpu& c);
// 0x1076f440: reads this process's information and the speech gestalt; 0 on failure.
std::uint32_t process_information(guest::Cpu& c);
// 0x1076f4f0: cursor, the four required Apple-event handlers, master pointers.
void install_handlers(guest::Cpu& c);
// 0x1076f600: the globals, the game's settings, the start-up volume's name.
void initialize(guest::Cpu& c);
// 0x1076f730: shut-down: the game, speech, the settings object, the mouse region.
void shut_down(guest::Cpu& c);
// 0x1076f780: stops speaking and frees the spoken text.
void stop_speech(guest::Cpu& c);
// 0x1076f7d0: ends the program with `code`: back to run() (longjmp) while the
// game runs, else through exit().
void fatal_exit(guest::Cpu& c, std::uint32_t code);
// 0x1076f880: does nothing.
void nothing(guest::Cpu& c);
// 0x1076f890: the menu bar (Quit removed under Aqua, desk accessories).
void set_up_menus(guest::Cpu& c);
// 0x1076f960: keeps a copy of each menu item's text (up to 50).
void remember_menu_texts(guest::Cpu& c);
// 0x1076fa20: puts the localized text of each remembered item back.
void localize_menus(guest::Cpu& c);
// 0x1076faf0: the preferences file and the game's data folder.
void find_folders(guest::Cpu& c);
// 0x1076fd90: the Carbon event handlers of the game window.
void install_window_handlers(guest::Cpu& c);
// 0x1076fe80: one turn of the event pump.
void pump_events(guest::Cpu& c);
// 0x10770170: delivers a waiting high-level event, if any.
void poll_high_level_events(guest::Cpu& c);
// 0x107701d0: the mouse position pinned to the 800 x 600 screen.
void track_mouse(guest::Cpu& c);
// 0x10770240: the File and Debug menus' items before MenuSelect.
void adjust_menus(guest::Cpu& c, std::uint32_t event);
// 0x10770380: a key press.
void key_down(guest::Cpu& c, std::uint32_t event);
// 0x107704e0: S with command or option toggles speech; 1 when the key was taken.
std::uint32_t speech_key(guest::Cpu& c, std::uint32_t character);
// 0x10770570: a click in the game window: which mouse button it means.
void content_click(guest::Cpu& c, std::uint32_t event);
// 0x10770650: the mouse button went up.
void mouse_up(guest::Cpu& c);
// 0x10770670: the Carbon event handler (window bounds, wheel, clicks).
std::uint32_t window_event(guest::Cpu& c, std::uint32_t handler, std::uint32_t event, std::uint32_t data);
// 0x10770760: the mouse wheel: adds its delta to the pending scroll.
std::uint32_t wheel_event(guest::Cpu& c, std::uint32_t handler, std::uint32_t event, std::uint32_t data);
// 0x107707c0: the button of a click, with control making it the right button.
std::uint32_t click_event(guest::Cpu& c, std::uint32_t handler, std::uint32_t event, std::uint32_t data);
// 0x10770880: the window's bounds changed: redraws the screen.
std::uint32_t bounds_event(guest::Cpu& c, std::uint32_t handler, std::uint32_t event, std::uint32_t data);
// 0x107708c0: a menu choice (MenuSelect's or MenuKey's result).
void menu_command(guest::Cpu& c, std::uint32_t event, std::uint32_t choice);
// 0x10770950, 0x10770970, 0x107709e0, 0x10770a10: the Apple, File, Edit and
// Debug menus' items.
void apple_menu(guest::Cpu& c, std::uint32_t item);
void file_menu(guest::Cpu& c, std::uint32_t item);
void edit_menu(guest::Cpu& c, std::uint32_t item);
void debug_menu(guest::Cpu& c);
// 0x10770a20: an activate event; nothing to do.
void activate_event(guest::Cpu& c);
// 0x10770a30: suspend and resume: the sleep time and the foreground flag.
void os_event(guest::Cpu& c, std::uint32_t event);
// 0x10770a80: AEProcessAppleEvent.
void high_level_event(guest::Cpu& c, std::uint32_t event);
// 0x10770ab0, 0x10770ac0, 0x10770c90, 0x10770ca0: the open-application,
// open-documents, print-documents and quit Apple-event handlers.
std::uint32_t open_application(guest::Cpu& c);
std::uint32_t open_documents(guest::Cpu& c, std::uint32_t event);
std::uint32_t print_documents(guest::Cpu& c);
std::uint32_t quit_application(guest::Cpu& c);
// 0x10770cc0: runs each line of a debugging script.
void run_script(guest::Cpu& c, std::uint32_t text);
// 0x10770d20: speaks `text` (with a full stop), waiting for earlier speech.
void speak(guest::Cpu& c, std::uint32_t text);
// 0x10770e40: the About box.
void about_box(guest::Cpu& c);
// 0x10770ee0: whether the menus follow the Aqua layout (Gestalt('menu') bit 1).
std::uint32_t aqua_menus(guest::Cpu& c);
// 0x10770f60, 0x10771010: per document of an open-documents event: writes
// AUTO_SETUP=<path> into quick_start.cfg, or adds <path> to the setup text.
std::uint32_t write_quick_start(guest::Cpu& c, std::uint32_t spec, std::uint32_t file);
std::uint32_t add_setup_line(guest::Cpu& c, std::uint32_t spec);
}
