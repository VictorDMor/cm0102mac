#pragma once
#include "media.hpp"

namespace cm::aot::media {
// 0x10647c00: a draw in [0, n) from the shared random table; used by 237 callers.
std::int32_t random_below(guest::Cpu&, std::int32_t n);
void post_item_bdb(guest::Cpu&, std::uint32_t player, std::uint32_t club, std::uint32_t kind);
void post_item_bcf(guest::Cpu&, std::uint32_t player, std::uint32_t club, std::uint32_t kind, std::uint32_t extra);
void post_item_bdc(guest::Cpu&, std::uint32_t player, std::uint32_t club, std::uint32_t figure, std::uint32_t grade);
std::uint32_t pick_staff(guest::Cpu&, std::uint32_t club, std::uint32_t other);
void post_item_bda(guest::Cpu&, std::uint32_t query, std::uint32_t person, std::uint32_t club, std::uint32_t kind, std::uint32_t extra);
void story_value(guest::Cpu&, double scale, std::uint32_t person, std::uint32_t club, std::uint32_t standing, std::uint32_t kind, std::uint32_t grade);
void change_rating_a(guest::Cpu&, std::uint32_t club, std::uint32_t other, std::uint32_t kind, std::uint32_t change, std::uint32_t grade);
void change_rating_b(guest::Cpu&, std::uint32_t club, std::uint32_t other, std::uint32_t kind, std::uint32_t change, std::uint32_t grade, std::uint32_t flag);
std::uint32_t find_manager(guest::Cpu&, std::uint32_t club);
}
