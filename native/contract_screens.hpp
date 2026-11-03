#pragma once
#include "guest_abi.hpp"

namespace cm::aot::contract_screens {
void open_transfer_request_from_news(guest::Cpu&, std::uint32_t item, std::uint32_t revision);
void draw_transfer_request(guest::Cpu&);
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
