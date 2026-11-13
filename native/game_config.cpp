// GAME_CONFIG.CPP: the preferences record, its options and its setters (game_config.hpp). Each
// routine keeps the original's frame and holds the original's non-volatile registers at its calls:
// the routines below this unit reach file and display imports with them, and no capture that
// replays reaches this unit to say which of them a later reader depends on.
#include "game_config_detail.hpp"
#include <algorithm>
#include <bit>

namespace cm::aot::game_config {
using guest::Cpu;
using guest::Frame;

bool start_sound(Cpu& c, std::uint32_t self, const SoundStart& at) {
    const auto pool = c.toc(toc::pool), cell = c.toc(toc::sound);
    const auto player = invoke(c, callee::allocate_object, at.allocated, {sound_player_size});
    c.set(at.player_register, player);
    if (player != 0) invoke(c, callee::sound_construct, at.constructed, {player});
    c.put32(cell, player);
    if (player != 0 && c.i8(player + sound_player_started) != 0) return true;
    invoke(c, callee::message_box, at.told, {pool + text::error, pool + text::sound_failed, 0});
    c.link(at.switched_off);
    select_option(c, self, option::sound_off);
    // Selecting "sound off" has just emptied the cell, so the original's test here never finds a player;
    // kept as the original has it.
    if (const auto left = c.u32(cell); left != 0) {
        invoke(c, callee::sound_destroy, at.destroyed, {left, 1});
        c.put32(cell, 0);
    }
    return false;
}

void stop_sound(Cpu& c, const SoundStop& at) {
    const auto cell = c.toc(toc::sound);
    if (const auto player = c.u32(cell); player != 0) {
        invoke(c, callee::sound_stop, at.stopped, {player});
        invoke(c, callee::sound_destroy, at.destroyed, {c.u32(cell), 1});
        c.put32(cell, 0);
    }
}

std::uint32_t copy(Cpu& c, std::uint32_t self, std::uint32_t source) {
    Frame frame(c, 0x1013f410, 0x40, {.first_gpr = 30});
    c.set({{30, self}, {31, source}});
    c.put32(self + field::source, source);
    c.put32(self + field::options, c.u32(source + field::options));
    c.put8(self + field::language, c.u8(source + field::language));
    c.put8(self + field::currency, c.u8(source + field::currency));
    c.put16(self + field::interval, c.u16(source + field::interval));
    // Each text is copied one byte short of its size and closed by hand.
    invoke(c, callee::copy_bounded, 0x1013f460,
           {self + field::save_folder, source + field::save_folder, field::save_folder_size - 1});
    c.put8(self + field::save_folder + field::save_folder_size - 1, 0);
    invoke(c, callee::copy_bounded, 0x1013f478,
           {self + field::unsaved_text, source + field::unsaved_text, field::short_text_size - 1});
    c.put8(self + field::unsaved_text + field::short_text_size - 1, 0);
    c.put8(self + field::stored, c.u8(source + field::stored));
    c.put32(self + field::font, c.u32(source + field::font));
    invoke(c, callee::copy_bounded, 0x1013f4a0,
           {self + field::font_name, source + field::font_name, field::short_text_size - 1});
    c.put8(self + field::font_name + field::short_text_size - 1, 0);
    return self;
}

std::uint32_t destroy(Cpu& c, std::uint32_t self, std::uint32_t deleting) {
    Frame frame(c, 0x1013f4d0, 0x50, {.first_gpr = 29});
    c.set({{29, self}, {30, deleting}, {31, c.toc(toc::sound)}});
    if (self == 0) return self;
    if (c.u32(self + field::source) == 0) {
        invoke(c, callee::backgrounds_stop, 0x1013f508);
        stop_sound(c, {0x1013f518, 0x1013f524});
        invoke(c, callee::close_display, 0x1013f530);
        invoke(c, callee::close_languages, 0x1013f534);
    }
    if (std::int16_t(deleting) > 0) invoke(c, callee::free_object, 0x1013f544, {self});
    return self;
}

bool set_defaults(Cpu& c, std::uint32_t self) {
    Frame frame(c, 0x1013f570, 0x40, {.first_gpr = 30});
    c.set(30, self);
    const auto answer = invoke(c, callee::processor_class, 0x1013f58c);
    c.set(31, answer);
    const auto processor = std::int8_t(answer);
    const auto memory = std::int32_t(invoke(c, callee::memory_megabytes, 0x1013f594));
    c.put32(self + field::options, default_options);
    const auto select = [&](bool first, std::uint32_t option, std::uint32_t back, std::uint32_t other,
                            std::uint32_t other_back) {
        c.link(first ? back : other_back);
        select_option(c, self, first ? option : other);
    };
    select(processor == 2 || memory < 63, option::slow_machine, 0x1013f5c0, option::fast_machine, 0x1013f5d0);
    select(processor == 0, option::fastest_processor, 0x1013f5e8, option::other_processor, 0x1013f5fc);
    select(processor == 2, option::slowest_processor, 0x1013f618, option::faster_processor, 0x1013f62c);
    const bool printer = flag(invoke(c, callee::printing_available, 0x1013f634, {c.toc(toc::printer)}));
    select(printer, option::with_printer, 0x1013f64c, option::without_printer, 0x1013f660);
    return true;
}

bool select_option(Cpu& c, std::uint32_t self, std::uint32_t option) {
    Frame frame(c, 0x1013f680, 0x70, {.first_gpr = 28});
    const auto pool = c.toc(toc::pool), cell = c.toc(toc::sound);
    c.set({{28, self}, {29, cell}, {30, c.toc(toc::message)}, {31, pool}});
    // The original is a tree of equality tests: exactly one bit, in one of the twelve groups.
    const auto group = std::ranges::find_if(option::groups, [&](std::uint32_t mask) { return (mask & option) != 0; });
    if (!std::has_single_bit(option) || group == option::groups.end()) {
        report_check(c, frame, {0x1013faec, 0x40, 0x38, 0x1f9});
        return false;
    }
    c.put32(self + field::options, (c.u32(self + field::options) & ~*group) | option);
    if (*group != option::sound) return true;
    const auto options = c.u32(self + field::options);
    if ((options & option::sound_on) && c.u32(cell) == 0) {
        if (start_sound(c, self, {0x1013f8ec, 0x1013f8f8, 0x1013f920, 0x1013f92c, 0x1013f940, 30})) {
            const auto intro = invoke(c, callee::make_path, 0x1013f958, {pool + text::sounds_folder, pool + text::intro});
            invoke(c, callee::sound_play, 0x1013f96c, {c.u32(cell), 2, intro, 0});
        }
    } else if (options & option::sound_off) {
        stop_sound(c, {0x1013f988, 0x1013f994});
    }
    return true;
}

bool set_language(Cpu& c, std::uint32_t self, std::uint32_t language) {
    Frame frame(c, 0x1013fb60, 0x70, {.first_gpr = 28});
    c.set({{28, self}, {29, language}, {30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    if (!flag(invoke(c, callee::language_available, 0x1013fb94, {language}))) {
        report_check(c, frame, {0x1013fbb0, 0x40, 0x38, 0x20d});
        return false;
    }
    c.put8(self + field::language, language);
    c.link(0x1013fc04);
    currency_for_language(c, self);
    invoke(c, callee::use_language, 0x1013fc10, {c.u8(self + field::language), 1});
    invoke(c, callee::refresh_screens, 0x1013fc20, {c.toc(toc::screens), 1, 1});
    return true;
}

bool set_currency(Cpu& c, std::uint32_t self, std::uint32_t currency) {
    Frame frame(c, 0x1013fc50, 0x70, {.first_gpr = 30});
    c.set({{30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    if (const auto index = std::int8_t(currency); index < 0 || index >= currency_count) {
        report_check(c, frame, {0x1013fc94, 0x40, 0x38, 0x22d});
        return false;
    }
    c.put8(self + field::currency, currency);
    return true;
}

bool currency_for_language(Cpu& c, std::uint32_t self) {
    // Languages 0..10; any other gets currency 0.
    static constexpr std::array<std::uint8_t, 11> currencies{0, 24, 9, 10, 14, 20, 6, 11, 16, 18, 21};
    const auto language = c.i8(self + field::language);
    c.put8(self + field::currency, language >= 0 && std::size_t(language) < currencies.size() ? currencies[std::size_t(language)] : 0);
    return true;
}

bool set_interval(Cpu& c, std::uint32_t self, std::uint32_t days) {
    Frame frame(c, 0x1013fe00, 0x70, {.first_gpr = 30});
    c.set({{30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    if (const auto count = std::int16_t(days); count < shortest_interval || count > longest_interval) {
        report_check(c, frame, {0x1013fe44, 0x40, 0x38, 0x26b});
        return false;
    }
    c.put16(self + field::interval, days);
    return true;
}

std::uint32_t font(Cpu& c, std::uint32_t self) { return c.u32(self + field::font); }

void set_font(Cpu& c, std::uint32_t self, std::uint32_t identifier) {
    Frame frame(c, 0x101406f0, 0x40, {});
    c.put32(self + field::font, identifier);
    invoke(c, callee::font_name, 0x10140714, {identifier, self + field::font_name, field::short_text_size});
}
}
