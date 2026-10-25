#pragma once
// Helpers shared by main_program_*.cpp.
#include "main_program.hpp"

namespace cm::aot::main_program::detail {
using guest::Cpu;
using guest::Frame;

// TOC slots MAIN.CPP reads: its string pool and other modules' objects.
namespace slot {
constexpr std::int32_t pool = -0x3bac;             // MAIN.CPP's strings ("MAIN.CPP" first)
constexpr std::int32_t jump_buffer = -0x3ba8;      // run()'s setjmp buffer
constexpr std::int32_t menu_texts = -0x3bb0;       // char*[50]: remembered menu item texts
constexpr std::int32_t click_specs = -0x3bb8;      // EventTypeSpec[2]: {mous 10}, {wind 0x55}
constexpr std::int32_t bounds_spec = -0x3bb4;      // EventTypeSpec: {wind 2}
constexpr std::int32_t event_table = -0x3bbc;      // pump_events's jump table, by event kind
constexpr std::int32_t screen_rect = -0x3bc0;      // Rect {0, 0, 600, 800}
constexpr std::int32_t key_table = -0x3bc4;        // key_down's jump table, key codes 0x73-0x7e
constexpr std::int32_t quick_start_callback = -0x3bc8;  // TVector of write_quick_start
constexpr std::int32_t setup_callback = -0x3bcc;   // TVector of add_setup_line
constexpr std::int32_t message = -0x7ba8;          // the shared message buffer
constexpr std::int32_t left_held = -0x7bac;        // int8: the left button is held (the game's asserts clear it)
constexpr std::int32_t game = -0x7bb0;             // the game object
constexpr std::int32_t settings = -0x7408;         // the settings object
constexpr std::int32_t game_started = -0x7418;     // int8: the game is initialized
constexpr std::int32_t process = -0x5bbc;          // ProcessInfoRec
constexpr std::int32_t application = -0x5bac;      // FSSpec of the application
constexpr std::int32_t data_directory = -0x5bb4;   // int32: the data folder's directory id
constexpr std::int32_t data_volume = -0x5bb0;      // int16: the data folder's volume
constexpr std::int32_t start_volume = -0x5ba8;     // int16: the application's volume
constexpr std::int32_t volume_name = -0x5bb8;      // char[]: the application volume's name
constexpr std::int32_t speech_wanted = -0x5b98;    // int8: the speech preference
constexpr std::int32_t foreground = -0x5b14;       // int8: the application is in front
constexpr std::int32_t settings_name = -0x5b20;    // char[]
constexpr std::int32_t right_button_modifiers = -0x5ad0;  // uint32: modifiers that make a right click
constexpr std::int32_t open_application_handler = -0x5ac0, open_documents_handler = -0x5ac4,
                       print_documents_handler = -0x5ac8, quit_handler = -0x5acc;  // TVectors
constexpr std::int32_t window_handler = -0x5ad4;   // TVector of window_event
constexpr std::int32_t game_window = -0x5af8;      // WindowRef
constexpr std::int32_t other_window = -0x5aec;     // WindowRef
constexpr std::int32_t button_state = -0x5ae0;     // uint16: 1 left, 2 right, 3 both
constexpr std::int32_t pending_scroll = -0x5ad8;   // int32: wheel delta not yet delivered
constexpr std::int32_t control_down = -0x5adc;     // int8
constexpr std::int32_t left_down = -0x744c, right_down = -0x5c00;  // int8 each
constexpr std::int32_t button_down = -0x5c0c;      // int8: a button is held
constexpr std::int32_t idle_wait = -0x5c20;        // int32: turns to wait before the next event
constexpr std::int32_t timer = -0x5c18;            // int32: the game's tick count
constexpr std::int32_t mouse_x = -0x5c10, mouse_y = -0x5c14;
constexpr std::int32_t key_code = -0x5c1c;         // int32: the last plain key's code
constexpr std::int32_t key_character = -0x74b0;    // int8: and its character
constexpr std::int32_t quick_start = -0x62c8;      // int8: quick_start.cfg was written
}
// Toolbox records MAIN.CPP reads.
namespace event_record {  // EventRecord
constexpr std::uint32_t what = 0, message = 2, when = 6, where = 10, modifiers = 14;
}
namespace fsspec {  // FSSpec: vRefNum, parID, then the name (a Pascal string)
constexpr std::uint32_t name = 6;
}
// The game object's screen request (halfword at +0x25819e).
constexpr std::uint32_t screen_request = 0x260000 - 0x7e62;
constexpr std::uint16_t request_quit = 0x402, request_save = 0x3fe, request_about = 0x42b;
constexpr std::int32_t event_not_handled = -0x2692;   // eventNotHandledErr
constexpr std::int32_t ae_not_handled = -0x6ac;       // errAEEventNotHandled

// A MAIN.CPP global's address and value.
inline std::uint32_t at(const Cpu& c, std::int32_t offset) { return c.r(2) + std::uint32_t(offset); }
inline std::uint32_t word(const Cpu& c, std::int32_t offset) { return c.u32(at(c, offset)); }

// The original's compares into cr0 (cmpwi/cmpw, cmplwi/cmplw, and the record
// forms, which compare their result with 0). Callees that save the condition
// register store it whole, so each compare is made here too.
inline std::int32_t compare(Cpu& c, std::int32_t a, std::int32_t b = 0) {
    c.compare(0, a, b);
    return a < b ? -1 : a > b ? 1 : 0;
}
inline std::int32_t compare_logical(Cpu& c, std::uint32_t a, std::uint32_t b = 0) {
    c.compare_logical(0, a, b);
    return a < b ? -1 : a > b ? 1 : 0;
}
// `extsh.`, `extsb.`, `rlwinm. rX,rY,0,24,31`: whether the value is non-zero.
inline bool test16(Cpu& c, std::uint32_t value) { return compare(c, std::int16_t(value)) != 0; }
inline bool test8(Cpu& c, std::uint32_t value) { return compare(c, std::int8_t(value)) != 0; }
inline bool test_byte(Cpu& c, std::uint32_t value) { return compare(c, std::int32_t(value & 0xff)) != 0; }

// A Toolbox call through its glue, which saves r2 at sp+0x14; the TOC comes
// back from there.
inline void toolbox(Cpu& c, const Frame& frame, std::uint32_t glue, std::uint32_t back) {
    c.call(glue, back);
    c.set(2, c.u32(frame.at(0x14)));
}

// Stops the speech channel and frees the text it was given (stop_speech();
// fatal_exit() and speech_key() repeat it in line). `dispose_back` and
// `free_back` are the return addresses of its two calls.
inline void release_speech(Cpu& c, const Frame& frame, std::uint32_t dispose_back, std::uint32_t free_back) {
    c.set(3, word(c, global::speech_channel));
    if (compare_logical(c, c.r(3)) == 0) return;
    toolbox(c, frame, 0x10780648, dispose_back);  // DisposeSpeechChannel
    c.set(3, word(c, global::spoken_text));
    if (compare_logical(c, c.r(3)) != 0) c.call(0x10775170, free_back);  // free
    c.set(0, 0);
    c.put32(at(c, global::speech_channel), 0);
    c.put32(at(c, global::spoken_text), 0);
}
}
