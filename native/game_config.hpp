#pragma once
// The original's GAME_CONFIG.CPP: the preferences record (options, language, currency, save folder,
// font), its file "CM3 Config" in the preferences folder, and what applying it switches on and off:
// the display, the sound player and the background pictures. 12 routines, 0x1013f030..0x10140724
// (tools/game_config_audit.py). Contract and quirks: docs/cpp-rewrites.md.
#include "guest_abi.hpp"

namespace cm::aot::game_config {
// 0x1013f030: builds the game's own record: the defaults, the languages, the stored file when it
// is whole, the command line's switches; then opens the display, the sound and the backgrounds.
std::uint32_t construct(guest::Cpu& c, std::uint32_t self);
// 0x1013f410: a working copy of `source` for the options screens; applying it writes `source`.
std::uint32_t copy(guest::Cpu& c, std::uint32_t self, std::uint32_t source);
// 0x1013f4d0: the game's own record closes the backgrounds, the sound, the display and the
// languages; `deleting` (signed low half) above zero frees the record. Null is accepted.
std::uint32_t destroy(guest::Cpu& c, std::uint32_t self, std::uint32_t deleting);
// 0x1013f570: the options a new record starts with, by processor, memory and printer.
bool set_defaults(guest::Cpu& c, std::uint32_t self);
// 0x1013f680: selects one option, clearing the others of its group; the sound options start or
// stop the sound player at once. False (after the assertion report) for anything else.
bool select_option(guest::Cpu& c, std::uint32_t self, std::uint32_t option);
// 0x1013fb60: an available language becomes the record's and the game's, with its currency.
bool set_language(guest::Cpu& c, std::uint32_t self, std::uint32_t language);
// 0x1013fc50: stores a currency index 0..24; the monetary tables change when the record is applied.
bool set_currency(guest::Cpu& c, std::uint32_t self, std::uint32_t currency);
// 0x1013fd00: the currency a language starts with. Always true.
bool currency_for_language(guest::Cpu& c, std::uint32_t self);
// 0x1013fe00: stores a number of days, 7..365.
bool set_interval(guest::Cpu& c, std::uint32_t self, std::uint32_t days);
// 0x1013feb0: a working copy writes its source and applies that; the game's own record switches
// language, currency, display, sound and backgrounds to its values and writes the file.
bool apply(guest::Cpu& c, std::uint32_t self);
// 0x101406e0 and 0x101406f0: the font's identifier; setting it also stores the font's name.
std::uint32_t font(guest::Cpu& c, std::uint32_t self);
void set_font(guest::Cpu& c, std::uint32_t self, std::uint32_t identifier);
}
