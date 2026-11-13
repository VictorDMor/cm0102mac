// GAME_CONFIG.CPP: building the game's record from the file "CM3 Config" and the command line, and
// applying a record: the live language, currency, display, sound and backgrounds, then the file.
// The file is a magic word, the options, the language and currency bytes, the interval, the save
// folder (0x100 bytes), the font identifier and the font's name (0x1f bytes), as the record holds them.
#include "game_config_detail.hpp"

namespace cm::aot::game_config {
using guest::Cpu;
using guest::Frame;

namespace {
constexpr std::uint32_t display_width = 800, display_height = 600;

// Reads the stored file into the record. False as soon as an item is short or the magic word is
// another: whatever was read by then stays in the record.
bool read_stored(Cpu& c, const Frame& frame, std::uint32_t self, std::uint32_t file) {
    const auto read = [&](std::uint32_t at, std::uint32_t size, std::uint32_t back) {
        return invoke(c, callee::file_read, back, {at, size, 1, file}) == 1;
    };
    const auto magic = frame.at(0x3c), language = frame.at(0x39), currency = frame.at(0x38),
               folder = frame.at(0x88), name = frame.at(0x48);
    if (!read(magic, 4, 0x1013f144) || c.u32(magic) != file_magic) return false;
    if (!read(self + field::options, 4, 0x1013f170)) return false;
    if (!read(language, 1, 0x1013f18c)) return false;
    // A language that is not installed keeps the system's.
    if (flag(invoke(c, callee::language_available, 0x1013f19c, {c.u8(language)}))) {
        c.put8(self + field::language, c.u8(language));
        invoke(c, callee::use_language, 0x1013f1b4, {c.u8(language), 0});
        invoke(c, callee::database_use_language, 0x1013f1c0, {c.u8(self + field::language), 0});
        c.link(0x1013f1c8);
        currency_for_language(c, self);
    }
    if (!read(currency, 1, 0x1013f1dc)) return false;
    if (const auto stored = c.u8(currency); std::int8_t(stored) >= 0 && std::int8_t(stored) < currency_count) {
        c.put8(self + field::currency, stored);
        invoke(c, callee::use_currency, 0x1013f204, {stored});
    }
    if (!read(self + field::interval, 2, 0x1013f218)) return false;
    // The stored save folder is read and dropped: the record keeps the "games" folder of this run.
    if (!read(folder, field::save_folder_size, 0x1013f234)) return false;
    if (!read(self + field::font, 4, 0x1013f250)) return false;
    if (!read(self + field::font_name, field::short_text_size, 0x1013f26c)) return false;
    // A font identifier that now names another font goes back to the default.
    invoke(c, callee::font_name, 0x1013f284, {c.u32(self + field::font), name, field::short_text_size});
    if (invoke(c, callee::compare_text, 0x1013f290, {self + field::font_name, name}) != 0)
        c.put32(self + field::font, default_font);
    return true;
}
}

std::uint32_t construct(Cpu& c, std::uint32_t self) {
    Frame frame(c, 0x1013f030, 0x1a0, {.first_gpr = 26});
    const auto pool = c.toc(toc::pool), command_line = c.toc(toc::command_line);
    c.set({{27, c.toc(toc::message)}, {28, self}, {29, c.toc(toc::sound)}, {30, command_line}, {31, pool}});
    c.put32(self + field::source, 0);
    c.link(0x1013f060);
    set_defaults(c, self);
    c.put32(self + field::font, default_font);
    const auto languages = invoke(c, callee::make_path, 0x1013f074, {pool + text::data_folder, pool + text::language_file});
    if (!flag(invoke(c, callee::load_languages, 0x1013f07c, {languages, 1}))) {
        report_check(c, frame, {0x1013f098, 0x68, 0x40, 0x54});
        invoke(c, callee::leave_program, 0x1013f0e0, {0xffffffff});
    }
    c.put8(self + field::language, invoke(c, callee::current_language, 0x1013f0e4));
    c.link(0x1013f0f0);
    currency_for_language(c, self);
    c.put16(self + field::interval, default_interval);
    invoke(c, callee::folder_path, 0x1013f104, {pool + text::games, self + field::save_folder});
    c.put8(self + field::unsaved_text, 0);
    c.put8(self + field::stored, 0);

    const auto config = invoke(c, callee::make_path, 0x1013f11c, {pool + text::config_folder, pool + text::config_file});
    const auto file = invoke(c, callee::file_open, 0x1013f124, {config, pool + text::reading});
    c.set(27, file);
    if (file != 0) {
        c.set(26, 0);
        const bool whole = read_stored(c, frame, self, file);
        if (whole) c.set(26, 1);
        invoke(c, callee::file_close, 0x1013f2ac, {file});
        if (whole) {
            c.put8(self + field::stored, 1);
        } else {
            // A file that is short or not ours is deleted.
            const auto broken = invoke(c, callee::make_path, 0x1013f2c0, {pool + text::config_folder, pool + text::config_file});
            invoke(c, callee::file_remove, 0x1013f2c4, {broken});
        }
    }

    if (invoke(c, callee::find_text, 0x1013f2dc, {command_line, pool + text::windowed_switch}) != 0) {
        c.link(0x1013f2f0);
        select_option(c, self, option::windowed);
    }
    if (invoke(c, callee::find_text, 0x1013f2fc, {command_line, pool + text::no_sound_switch}) != 0) {
        c.link(0x1013f310);
        select_option(c, self, option::sound_off);
    }
    if (invoke(c, callee::find_text, 0x1013f31c, {command_line, pool + text::default_font_switch}) != 0)
        c.put32(self + field::font, default_font);

    const std::uint32_t full_screen = (c.u32(self + field::options) & option::full_screen) ? 1 : 0;
    if (!flag(invoke(c, callee::open_display, 0x1013f358,
                     {display_width, display_height, 2, font(c, self), full_screen, pool + text::title})))
        invoke(c, callee::leave_program, 0x1013f368, {0});
    // Unlike the other two sites, this one starts a player whatever the cell holds.
    if (c.u32(self + field::options) & option::sound_on)
        start_sound(c, self, {0x1013f37c, 0x1013f388, 0x1013f3b0, 0x1013f3bc, 0x1013f3d0, 26});
    invoke(c, callee::backgrounds_start, 0x1013f3dc);
    invoke(c, callee::backgrounds_enable, 0x1013f3ec, {(c.u32(self + field::options) & option::backgrounds_on) ? 1u : 0u});
    return self;
}

namespace {
// A working copy's values go to its source. The original reads the link again for every field.
void write_source(Cpu& c, std::uint32_t self) {
    const auto source = [&] { return c.u32(self + field::source); };
    c.put32(source() + field::options, c.u32(self + field::options));
    c.put8(source() + field::language, c.u8(self + field::language));
    c.put8(source() + field::currency, c.u8(self + field::currency));
    c.put16(source() + field::interval, c.u16(self + field::interval));
    invoke(c, callee::copy_text, 0x1013ff2c, {source() + field::save_folder, self + field::save_folder});
    invoke(c, callee::copy_text, 0x1013ff3c, {source() + field::unsaved_text, self + field::unsaved_text});
    c.put8(source() + field::stored, c.u8(self + field::stored));
    c.put32(source() + field::font, c.u32(self + field::font));
    invoke(c, callee::copy_text, 0x1013ff64, {source() + field::font_name, self + field::font_name});
}

// The display is opened again when the font changed or its mode is not the selected one.
void apply_display(Cpu& c, const Frame& frame, std::uint32_t self) {
    const auto title = c.toc(toc::pool) + text::title;
    const bool other_font = c.u32(self + field::font) != c.u32(c.toc(toc::current_font));
    c.set({{24, other_font}, {25, 0}, {27, 1}});
    bool other_mode = false;
    if (c.u32(self + field::options) & option::full_screen)
        other_mode = !flag(invoke(c, callee::display_is_full_screen, 0x10140040));
    if (!other_mode && (c.u32(self + field::options) & option::windowed))
        other_mode = flag(invoke(c, callee::display_is_full_screen, 0x10140068));
    c.set({{24, other_font || other_mode}, {25, other_mode}, {27, other_mode}});
    if (!other_font && !other_mode) return;
    // The first attempt passes the option's bit itself (8 or 0) as the full-screen flag.
    const auto full_screen = c.u32(self + field::options) & option::full_screen;
    c.set(24, full_screen);
    if (flag(invoke(c, callee::open_display, 0x101400bc,
                    {display_width, display_height, 2, font(c, self), full_screen, title})))
        return;
    // It failed: select the other mode and try that one.
    c.link(0x101400dc);
    select_option(c, self, full_screen ? option::windowed : option::full_screen);
    if (flag(invoke(c, callee::open_display, 0x10140108,
                    {display_width, display_height, 2, font(c, self), full_screen == 0 ? 1u : 0u, title})))
        return;
    report_check(c, frame, {0x10140124, 0x1ac, 0x84, 0x2cd});
    invoke(c, callee::leave_program, 0x10140168, {0xffffffff});
}

// Writes the record's file. False after the assertion report of the item that failed.
bool write_stored(Cpu& c, const Frame& frame, std::uint32_t self) {
    const auto pool = c.toc(toc::pool);
    const auto config = invoke(c, callee::make_path, 0x10140264, {pool + text::config_folder, pool + text::config_file});
    const auto file = invoke(c, callee::file_open, 0x1014026c, {config, pool + text::writing});
    c.set(27, file);
    if (file == 0) {
        report_check(c, frame, {0x10140288, 0x18c, 0x7c, 0x309});
        return false;
    }
    struct Item { std::uint32_t at, size; };
    const Item items[] = {{frame.at(0x38), 4},  // the magic word
                          {self + field::options, 4},
                          {self + field::language, 1},
                          {self + field::currency, 1},
                          {self + field::interval, 2},
                          {self + field::save_folder, field::save_folder_size},
                          {self + field::font, 4},
                          {self + field::font_name, field::short_text_size}};
    // The eight writes are 0x7c bytes of code apart, each with its own buffers and source line.
    for (std::uint32_t n = 0; n < std::size(items); ++n) {
        const auto step = n * 0x7c;
        if (invoke(c, callee::file_write, 0x101402e0 + step, {items[n].at, items[n].size, 1, file}) == 1) continue;
        report_check(c, frame, {0x101402fc + step, 0x16c - std::int32_t(n) * 0x20, 0x74 - std::int32_t(n) * 8, 0x311 + n * 8});
        invoke(c, callee::file_close, 0x10140340 + step, {file});
        return false;
    }
    invoke(c, callee::file_close, 0x101406b4, {file});
    c.put8(self + field::stored, 1);
    return true;
}
}

bool apply(Cpu& c, std::uint32_t self) {
    Frame frame(c, 0x1013feb0, 0x1f0, {.first_gpr = 24});
    const auto screens = c.toc(toc::screens), cell = c.toc(toc::sound);
    c.set({{25, screens}, {26, self}, {28, cell}, {29, c.toc(toc::success)}, {30, c.toc(toc::message)},
           {31, c.toc(toc::pool)}});
    c.put32(frame.at(0x38), file_magic);
    if (c.u32(self + field::source) != 0) {
        write_source(c, self);
        c.link(0x1013ff6c);
        return apply(c, c.u32(self + field::source));
    }

    if (const auto live = std::int8_t(invoke(c, callee::current_language, 0x1013ff74)); c.i8(self + field::language) != live) {
        invoke(c, callee::use_language, 0x1013ff94, {c.u8(self + field::language), 1});
        invoke(c, callee::refresh_screens, 0x1013ffa4, {screens, 1, 1});
    }
    // The database's texts follow only once a database is loaded.
    if (const auto live = std::int8_t(invoke(c, callee::database_language, 0x1013ffa8));
        c.i8(self + field::language) != live && c.u32(c.toc(toc::database)) != 0) {
        invoke(c, callee::database_use_language, 0x1013ffd8, {c.u8(self + field::language), 0});
        invoke(c, callee::refresh_screens, 0x1013ffe8, {screens, 1, 1});
    }
    if (const auto live = std::int8_t(invoke(c, callee::current_currency, 0x1013ffec)); c.i8(self + field::currency) != live)
        invoke(c, callee::use_currency, 0x10140008, {c.u8(self + field::currency)});

    apply_display(c, frame, self);

    if (const auto options = c.u32(self + field::options); (options & option::sound_on) && c.u32(cell) == 0)
        start_sound(c, self, {0x10140188, 0x10140194, 0x101401bc, 0x101401c8, 0x101401dc, 24});
    else if (options & option::sound_off)
        stop_sound(c, {0x10140200, 0x1014020c});

    if ((c.u32(self + field::options) & option::backgrounds_on) && !flag(invoke(c, callee::backgrounds_enabled, 0x10140224)))
        invoke(c, callee::backgrounds_enable, 0x10140234, {1});
    else if ((c.u32(self + field::options) & option::backgrounds_off) && flag(invoke(c, callee::backgrounds_enabled, 0x10140248)))
        invoke(c, callee::backgrounds_enable, 0x10140258, {0});

    return write_stored(c, frame, self);
}
}
