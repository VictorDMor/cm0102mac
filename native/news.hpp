#pragma once
#include "guest_abi.hpp"

namespace cm::aot::news {
std::uint32_t construct_item(guest::Cpu&, std::uint32_t item, std::uint32_t type, std::uint32_t priority);
std::uint32_t category(guest::Cpu&, std::uint32_t item);
std::uint32_t set_word(guest::Cpu&, std::uint32_t item, std::uint32_t slot, std::uint32_t value);
std::uint32_t word(guest::Cpu&, std::uint32_t item, std::uint32_t slot);
std::uint32_t set_text(guest::Cpu&, std::uint32_t item, std::uint32_t slot,
                       std::uint32_t text, std::uint32_t next_slot);
std::uint32_t text(guest::Cpu&, std::uint32_t item, std::uint32_t slot,
                   std::uint32_t output, std::uint32_t capacity);
std::uint32_t destroy_queue(guest::Cpu&, std::uint32_t queue, std::uint32_t deleting);
std::uint32_t find_item(guest::Cpu&, std::uint32_t queue, std::uint32_t manager,
                        std::uint32_t id, std::uint32_t output);
std::uint32_t update_word(guest::Cpu&, std::uint32_t queue, std::uint32_t manager,
                          std::uint32_t id, std::uint32_t slot, std::uint32_t value,
                          std::uint32_t current_item);
std::uint32_t unread_count(guest::Cpu&, std::uint32_t queue, std::uint32_t manager,
                           std::uint32_t category);
std::uint32_t club_enabled(guest::Cpu&, std::uint32_t queue, std::uint32_t club);
}
