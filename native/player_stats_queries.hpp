#pragma once
// PLAYER_STATS.CPP: the ratings and values the match engine, recruitment and the screens ask of a player's season records.
#include "guest_abi.hpp"

namespace cm::aot::player_stats {
// 0x10303560: a player's value of `kind` (r6) for `category` (r5): one
// season record's value, or for the combined categories (100-102, or kind
// 0x13) a weighted rating over categories 2-5; `prior` stands in when there
// are few appearances.
double player_value(guest::Cpu& c, std::uint32_t stats, std::uint32_t person, std::uint32_t category, std::uint32_t kind,
                    double prior);
// 0x10303980: the value of `kind` (1-22) in one season record.
double season_value(guest::Cpu& c, std::uint32_t record, std::uint32_t kind, double prior);
// 0x103054e0: the address of a player's season record for `category` (1-6), or 0.
std::uint32_t season_record(guest::Cpu& c, std::uint32_t stats, std::uint32_t person, std::int8_t category);
// 0x10306a80: one of the two int16 figures of a player's entry (kind 1 or 2), as a float.
double entry_figure(guest::Cpu& c, std::uint32_t stats, std::uint32_t person, std::int8_t kind);
// 0x10303090: the average of a player's five packed ratings (each 4x its nibble, 27 when unset), over 4.
std::uint32_t packed_ratings_average(guest::Cpu& c, std::uint32_t stats, std::uint32_t person);
// 0x103031f0: a player's fifth packed rating as a float (6.5 when unset).
double fifth_rating(guest::Cpu& c, std::uint32_t stats, std::uint32_t person);
}
