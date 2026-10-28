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
std::uint32_t priority_for_manager(guest::Cpu&, std::uint32_t item, std::uint32_t manager);
std::uint32_t load_queue(guest::Cpu&, std::uint32_t queue, std::uint32_t context);
std::uint32_t save_queue(guest::Cpu&, std::uint32_t queue, std::uint32_t context);
std::uint32_t sample_range(guest::Cpu&, std::uint32_t bound);
std::uint32_t construct_queue(guest::Cpu&, std::uint32_t queue, std::uint32_t context);
std::uint32_t publish_to_manager(guest::Cpu&, std::uint32_t queue, std::uint32_t item, std::uint32_t manager);
std::uint32_t distribute(guest::Cpu&, std::uint32_t queue, std::uint32_t item,
                         std::uint32_t club, std::uint32_t entity, std::uint32_t nation,
                         std::uint32_t continent, std::uint32_t kind);
std::uint32_t distribute_to_club(guest::Cpu&, std::uint32_t queue, std::uint32_t item, std::uint32_t club);
std::uint32_t distribute_to_competition(guest::Cpu&, std::uint32_t queue, std::uint32_t item, std::uint32_t competition);
std::uint32_t distribute_to_nation(guest::Cpu&, std::uint32_t queue, std::uint32_t item, std::uint32_t nation);
std::uint32_t distribute_to_club_nation(guest::Cpu&, std::uint32_t queue, std::uint32_t item, std::uint32_t nation);
std::uint32_t distribute_to_continent(guest::Cpu&, std::uint32_t queue, std::uint32_t item, std::uint32_t continent);
std::uint32_t distribute_to_all(guest::Cpu&, std::uint32_t queue, std::uint32_t item);
std::uint32_t distribute_to_entity(guest::Cpu&, std::uint32_t queue, std::uint32_t item, std::uint32_t entity);
std::uint32_t recent_items(guest::Cpu&, std::uint32_t queue, std::uint32_t manager, std::uint32_t count, std::uint32_t category);
std::uint32_t acknowledge(guest::Cpu&, std::uint32_t queue, std::uint32_t manager, std::uint32_t id, std::uint32_t propagate);
std::uint32_t requires_attention(guest::Cpu&, std::uint32_t queue, std::uint32_t manager, std::uint32_t inspect_actions);
std::uint32_t perform_matching_actions(guest::Cpu&, std::uint32_t queue, std::uint32_t manager, std::uint32_t club);
void clear_inbox(guest::Cpu&, std::uint32_t queue, std::uint32_t manager);
std::uint32_t action_label(guest::Cpu&, std::uint32_t item, std::uint32_t manager,
                           std::uint32_t output, std::uint32_t capacity, std::uint32_t callback);
void perform_action(guest::Cpu&, std::uint32_t item, std::uint32_t manager, std::uint32_t club);
std::uint32_t render_message(guest::Cpu&,std::uint32_t item,std::uint32_t manager,
                             std::uint32_t output,std::uint32_t detail,std::uint32_t suppress_reply);
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
