#pragma once
// Shared pieces of the "news_outcomes" block: the contract lookup every transfer news item starts from,
// and small readers of person and club records. Private to transfer_manager_native_news_outcomes*.cpp.
#include "transfer_manager_native_detail.hpp"

namespace cm::aot::transfer_manager_native::news_outcomes {
inline constexpr std::uint32_t find_contract = 0x10082f20,       // (contract manager, person) -> contract or 0
                               human_club = 0x102cdbf0,          // (news manager, club) -> non-zero when a human manages it
                               send_to_all = 0x102cc8e0,         // (news manager, item)
                               list_next = 0x10738e00,           // (list) -> next entry or 0
                               offer_people = 0x103fb840,        // (manager, offer, kind, out words, 30) -> count (char)
                               offer_value = 0x10416710,
                               person_status = 0x103ecef0;       // (manager, person, club, 0) -> char
inline constexpr std::uint32_t contract_status = 0x38, contract_club_index = 4;

struct ContractClub {
    bool found;            // the original's flag register: a contract that is not status 5
    std::uint32_t club;    // its club record, 0 without one
};
// The lookup of the person's contract (0x10082f20). The result goes to the shared cell; a contract
// with status byte >> 2 equal to 5 counts as none.
inline ContractClub contract_club(guest::Cpu& c, std::uint32_t person, std::uint32_t back) {
    const auto contract = invoke(c, find_contract, back, {c.u32(c.toc(toc::contracts)), person});
    const auto cell = c.toc(toc::found_contract);
    c.put32(cell, contract);
    if (contract == 0 || (c.i8(contract + contract_status) >> 2) == 5) return {false, 0};
    return {true, c.u32(c.toc(toc::clubs)) + c.u32(c.u32(cell) + contract_club_index) * club_size};
}
// The same lookup without the status test (the original of 0x10410500 has none).
inline std::uint32_t contract_club_any(guest::Cpu& c, std::uint32_t person, std::uint32_t back) {
    const auto contract = invoke(c, find_contract, back, {c.u32(c.toc(toc::contracts)), person});
    c.put32(c.toc(toc::found_contract), contract);
    if (contract == 0) return 0;
    return c.u32(c.toc(toc::clubs)) + c.u32(contract + contract_club_index) * club_size;
}

inline std::uint32_t sign8(guest::Cpu& c, std::uint32_t at) { return std::uint32_t(std::int32_t(c.i8(at))); }
inline std::uint32_t sign16(std::uint32_t raw) { return std::uint32_t(std::int32_t(std::int16_t(raw))); }
// The person (0x70 each) an offer concerns: the index at +0xc.
inline std::uint32_t offer_person(guest::Cpu& c, std::uint32_t offer) {
    return c.u32(c.toc(toc::people)) + c.u32(offer + 0xc) * person_size;
}
// A club by index.
inline std::uint32_t club_at(guest::Cpu& c, std::uint32_t index) { return c.u32(c.toc(toc::clubs)) + index * club_size; }
// The value of the name record a person points at (+0x28, +0x2c, +0x30 hold three of them).
inline std::uint32_t name_value(guest::Cpu& c, std::uint32_t person, std::uint32_t slot) { return c.u32(c.u32(person + slot) + 0x20); }
}
