#pragma once
#include "media.hpp"

namespace cm::aot::media {
std::uint32_t construct_press(guest::Cpu&, std::uint32_t self, std::uint32_t path);
std::uint32_t destroy_press(guest::Cpu&, std::uint32_t self, std::uint32_t flags);
std::uint32_t load_press(guest::Cpu&, std::uint32_t self, std::uint32_t path);
std::uint32_t save_press(guest::Cpu&, std::uint32_t self, std::uint32_t path);
void add_history_row(guest::Cpu&, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t story,
                     std::uint32_t grade, std::uint32_t mark);
std::uint32_t mark_history_row(guest::Cpu&, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t story,
                               std::uint32_t grade, std::uint32_t mark);
void prune_history(guest::Cpu&, std::uint32_t self);
std::uint32_t has_history_row(guest::Cpu&, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t story,
                              std::uint32_t grade, std::uint32_t date);
void add_press_event(guest::Cpu&, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::int32_t amount, double weight);
void remove_press_events(guest::Cpu&, std::uint32_t self, std::uint32_t person);
std::uint32_t story_roll_for_club(guest::Cpu&, std::uint32_t club);
std::uint32_t story_roll_for_player(guest::Cpu&, std::uint32_t player);
void log_kind_1790(guest::Cpu&, std::uint32_t club);
void log_kind_178e(guest::Cpu&, std::uint32_t club, std::uint32_t other, std::uint32_t first, std::uint32_t second,
                   std::uint32_t third, std::uint32_t fourth);
void log_kind_bcb(guest::Cpu&, std::uint32_t player, std::uint32_t club, std::uint32_t match);
void log_kind_2e(guest::Cpu&, std::uint32_t club, std::uint32_t other, std::uint32_t flag);
}
