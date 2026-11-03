#pragma once
#include "guest_abi.hpp"

namespace cm::aot::contract_screens {
void open_staff_contract_from_news(guest::Cpu&, std::uint32_t item, std::uint32_t revision);
void release_offer_draft(guest::Cpu&);
void open_offer_details(guest::Cpu&, std::uint32_t offer_link, std::uint32_t first,
                        std::uint32_t second, std::uint32_t third, std::uint32_t fifth,
                        std::uint32_t fourth);
void open_staff_contract(guest::Cpu&, std::uint32_t staff);
std::uint32_t compare_candidates(guest::Cpu&, std::uint32_t left_slot, std::uint32_t right_slot);
}
