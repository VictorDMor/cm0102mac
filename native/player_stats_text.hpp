#pragma once
// PLAYER_STATS.CPP: the stats screens' texts: competition names, column titles and formatted values.
#include "guest_abi.hpp"

namespace cm::aot::player_stats {
// 0x10302cb0: the localized name of competition type `type` (1-6) into
// `buffer` (at least 51 bytes); 1, or 0 after an assertion report.
std::uint32_t competition_name(guest::Cpu& c, std::int8_t type, std::uint32_t buffer, std::int32_t size);
// 0x10302e70: a player's last five match ratings as "7-6-8" into `buffer`
// ("-" when he has none or is not a player); `player` points at his id.
void recent_ratings_text(guest::Cpu& c, std::uint32_t stats, std::uint32_t player, std::uint32_t buffer,
                         std::int32_t size);
// 0x103032e0: the localized short title of stats column `column` (1-17)
// into `buffer`; 1, or 0 after an assertion report.
std::uint32_t column_title(guest::Cpu& c, std::int8_t column, std::uint32_t buffer, std::int32_t size);
// 0x10303ff0: a player's value of stats column `column` for competition type
// `type` as text: his season record's (season_value_text), "-" without one.
// `column`, `type` and the flags are the registers as passed (the original
// keeps them whole); `zeros` is the stack argument byte.
std::uint32_t player_value_text(guest::Cpu& c, std::uint32_t stats, std::uint32_t player, std::uint32_t type,
                                std::uint32_t column, std::uint32_t buffer, std::int32_t size, std::uint32_t detail,
                                std::uint32_t per_appearance, std::uint32_t zeros);
// 0x10304190: one season record's value of stats column `column` (1-17) as
// text into `buffer` (sprintf formats; "-", or "----" for the average
// rating, when there is nothing to show). The flags (char arguments, kept
// whole as the original does): `detail` adds the bracketed figure
// (substitute appearances, penalties), `per_appearance` " / appearances",
// `zeros` shows tallies of a player without appearances.
std::uint32_t season_value_text(guest::Cpu& c, std::uint32_t record, std::int8_t column, std::uint32_t buffer,
                                std::int32_t size, std::uint32_t detail, std::uint32_t per_appearance,
                                std::uint32_t zeros);
// 0x10304b10: the same for a player's totals (SeasonTotals), with per-90
// minutes figures and minimum samples for the percentages.
std::uint32_t totals_value_text(guest::Cpu& c, std::uint32_t totals, std::int8_t column, std::uint32_t buffer,
                                std::int32_t size, std::uint32_t detail, std::uint32_t per_appearance,
                                std::uint32_t zeros);
}
