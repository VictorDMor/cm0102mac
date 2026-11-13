#pragma once
// GAME_CONFIG.CPP: the record's layout, the globals and the routines outside the unit it uses.
#include "game_config.hpp"
#include <array>
#include <initializer_list>

namespace cm::aot::game_config {
// The preferences record (0x150 bytes). `unsaved_text` is copied between records and never
// filled or written by this unit; what it holds is not established.
namespace field {
inline constexpr std::uint32_t source = 0x00,        // a working copy's original; null in the game's own record
                               options = 0x04,       // one bit per selected option
                               language = 0x08,      // signed byte
                               currency = 0x09,      // signed byte, 0..24
                               interval = 0x0a,      // signed halfword, days
                               save_folder = 0x0c,   // 0x100 bytes
                               unsaved_text = 0x10c, // 0x1f bytes
                               stored = 0x12b,       // the file was read whole or written
                               font = 0x12c,         // font identifier
                               font_name = 0x130;    // 0x1f bytes
inline constexpr std::uint32_t save_folder_size = 0x100, short_text_size = 0x1f;
}
inline constexpr std::uint32_t default_font = 0xfffffffe, file_magic = 0x1553117c, default_options = 0x212552a9;
inline constexpr std::int32_t currency_count = 25, shortest_interval = 7, longest_interval = 365, default_interval = 30;

// Options are single bits; selecting one clears the rest of its group. Only the groups this
// unit acts on have names.
namespace option {
inline constexpr std::uint32_t full_screen = 0x8, windowed = 0x10, backgrounds_on = 0x20, backgrounds_off = 0x40,
                               sound_on = 0x80, sound_off = 0x100;
inline constexpr std::uint32_t sound = sound_on | sound_off;
inline constexpr std::array<std::uint32_t, 12> groups{0x7,      0x18,     0x60,      0x180,    0xe00,     0x3000,
                                                      0xc000,   0x30000,  0x1c0000,  0xe00000, 0x7000000, 0x60000000};
// What set_defaults selects.
inline constexpr std::uint32_t slow_machine = 0x2, fast_machine = 0x1, fastest_processor = 0x20000,
                               other_processor = 0x10000, slowest_processor = 0x80000, faster_processor = 0x40000,
                               with_printer = 0x1000000, without_printer = 0x2000000;
}

// Words of the table of contents (Cpu::toc).
namespace toc {
inline constexpr std::int32_t pool = -0x54e8,          // the unit's strings
                              message = -0x7ba8,       // the shared message buffer
                              success = -0x7bac,       // the shared success flag
                              sound = -0x7468,         // the cell holding the sound player
                              command_line = -0x7410,  // the command line's text
                              screens = -0x7bb0,       // the screen manager
                              database = -0x7b98,      // the cell holding the loaded database
                              current_font = -0x740c,  // the cell holding the display's font
                              printer = -0x7408;       // the printer object
}
// Offsets in the unit's string pool.
namespace text {
inline constexpr std::uint32_t data_folder = 0x00,    // "CM3_DATA"
                               language_file = 0x09,  // "language.ldb"
                               source_file = 0x16,    // "GAME_CONFIG.CPP"
                               report_format = 0x26,  // "v%s %s.%s %d"
                               error = 0x33,          // "Error"
                               games = 0x39,          // "games"
                               config_folder = 0x3f,  // "CM3_PREFS"
                               config_file = 0x49,    // "CM3 Config"
                               reading = 0x54,        // "rb"
                               windowed_switch = 0x57,       // "-windowed"
                               no_sound_switch = 0x61,       // "-nosound"
                               default_font_switch = 0x6a,   // "-defaultfont"
                               title = 0x77,          // "Championship Manager 2001/02"
                               sound_failed = 0x94,   // "Unable to initialise the sound - ..."
                               sounds_folder = 0xed,  // "CM3_SOUNDS"
                               intro = 0xf8,          // "intro2.wav"
                               writing = 0x103;       // "wb"
}
// The sound player: 0x2c bytes, with a byte that says whether it started.
inline constexpr std::uint32_t sound_player_size = 0x2c, sound_player_started = 0x28;

namespace callee {
inline constexpr std::uint32_t
    make_path = 0x10746280,           // (folder, name) -> the path's text
    folder_path = 0x10746300,         // (folder, out)
    load_languages = 0x10726be0,      // (path, 1) -> whether it loaded
    close_languages = 0x10726cc0,
    language_available = 0x10726da0,  // (language) -> flag
    use_language = 0x10726e90,        // (language, refresh)
    current_language = 0x10727880,
    database_use_language = 0x100b6050,  // (language, 0)
    database_language = 0x100b8b70,
    use_currency = 0x1000a440,        // (currency)
    current_currency = 0x1000b150,
    refresh_screens = 0x10742ec0,     // (screen manager, 1, 1)
    sound_construct = 0x10744670, sound_destroy = 0x107446b0,  // (player), (player, 1)
    sound_stop = 0x10744700,          // (player)
    sound_play = 0x10744860,          // (player, 2, path, 0)
    font_name = 0x10750b60,           // (identifier, out, size)
    open_display = 0x10768820,        // (width, height, 2, font, full screen, title) -> flag
    close_display = 0x10768990,
    display_is_full_screen = 0x10769b50,
    message_box = 0x10769b60,         // (title, text, 0)
    backgrounds_start = 0x10711b40, backgrounds_stop = 0x10712c50,
    backgrounds_enable = 0x10712cc0,  // (flag)
    backgrounds_enabled = 0x10712ce0,
    processor_class = 0x1073af50,     // 0, 1, or 2 for the slowest (and when it cannot tell)
    memory_megabytes = 0x1073b060,
    printing_available = 0x1074cc20,  // (printer) -> flag
    leave_program = 0x1076f7d0,       // (status)
    allocate_object = 0x10773520, free_object = 0x107735a0,
    file_open = 0x10739de0,           // (path, mode) -> file or null
    file_read = 0x10775fe0, file_write = 0x10776360,  // (at, size, count, file) -> items moved
    file_close = 0x107766f0, file_remove = 0x10776690,
    copy_text = 0x1077dbf0,           // strcpy
    copy_bounded = 0x1077dc10,        // strncpy
    compare_text = 0x1077dc90,        // strcmp
    find_text = 0x1077dda0;           // strstr
}

// `bl entry` with the arguments in r3 onwards; the callee's r3.
inline std::uint32_t invoke(guest::Cpu& c, std::uint32_t entry, std::uint32_t back,
                            std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    c.call(entry, back);
    return c.r(3);
}
// A callee's answer as the original tests it: the signed low byte against zero.
inline bool flag(std::uint32_t answer) { return std::int8_t(answer) != 0; }

// The assertion report of a failed check: `at` is the address of the report's first call, `name`
// and `extension` the frame offsets of its two buffers, `line` the source line.
struct CheckSite { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
inline void report_check(guest::Cpu& c, const guest::Frame& frame, const CheckSite& site) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {site.at, pool + text::source_file, pool + text::report_format, pool + text::error,
                                frame.at(site.name), frame.at(site.extension), site.line});
}

// The return addresses of one site that starts the sound player, and the register the original
// keeps the new player in.
struct SoundStart { std::uint32_t allocated, constructed, told, switched_off, destroyed; unsigned player_register; };
struct SoundStop { std::uint32_t stopped, destroyed; };
// Creates the player and stores it in its cell. When it cannot start, tells the user, selects
// "sound off" (which stops and frees the player) and frees whatever the cell still holds.
bool start_sound(guest::Cpu& c, std::uint32_t self, const SoundStart& at);
// Stops and frees the player the cell holds, if any.
void stop_sound(guest::Cpu& c, const SoundStop& at);
}
