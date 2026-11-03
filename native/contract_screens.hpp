#pragma once
#include "guest_abi.hpp"

namespace cm::aot::contract_screens {
void open_transfer_request_from_news(guest::Cpu&, std::uint32_t item, std::uint32_t revision);
void open_restricted_contract_from_news(guest::Cpu&, std::uint32_t item, std::uint32_t revision);
void open_offer_contract_from_news(guest::Cpu&, std::uint32_t item, std::uint32_t revision);
void open_contract_choice(guest::Cpu&, std::uint32_t person, std::uint32_t link,
                          std::uint32_t with_offer, std::uint32_t restrictions);
void draw_contract_choice(guest::Cpu&);
void draw_transfer_request(guest::Cpu&);
std::uint32_t transfer_request_command(guest::Cpu&, std::uint32_t control);
std::uint32_t sample_transfer_request(guest::Cpu&, std::uint32_t bound);
std::uint32_t negotiation_choice_command(guest::Cpu&, std::uint32_t control);
void open_new_negotiation(guest::Cpu&, std::uint32_t person, std::uint32_t state,
                          std::uint32_t kind, std::uint32_t mode);
void open_existing_negotiation(guest::Cpu&, std::uint32_t link, std::uint32_t person,
                               std::uint32_t state, std::uint32_t kind, std::uint32_t mode);
void release_offer_draft(guest::Cpu&);
void open_offer_details(guest::Cpu&, std::uint32_t offer_link, std::uint32_t first,
                        std::uint32_t second, std::uint32_t third, std::uint32_t fifth,
                        std::uint32_t fourth);
void open_staff_release(guest::Cpu&, std::uint32_t staff);
void draw_staff_release(guest::Cpu&);
std::uint32_t staff_release_command(guest::Cpu&, std::uint32_t control);
void load_database_with_language_setup(guest::Cpu&, std::uint32_t directory, std::uint32_t options);
void save_database(guest::Cpu&, std::uint32_t directory, std::uint32_t options, std::uint32_t version);
void load_database(guest::Cpu&, std::uint32_t directory, std::uint32_t options);
std::uint32_t compare_candidates(guest::Cpu&, std::uint32_t left_slot, std::uint32_t right_slot);
}
