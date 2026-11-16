#pragma once
// Pieces shared by the "news" block's routines: the contract lookup every news builder repeats and the
// four person-identity fields most news items start with.
#include "transfer_manager_native_detail.hpp"
#include <array>

namespace cm::aot::transfer_manager_native::news_common {
inline constexpr std::uint32_t find_contract = 0x10082f20;      // (contract manager, person) -> contract or 0
inline constexpr std::uint32_t contract_club_index = 4, contract_state = 0x38;
inline constexpr std::uint32_t person_name = 0x28, person_club_name = 0x2c, person_nation = 0x30;
inline constexpr std::uint32_t name_id = 0x20;
inline constexpr std::uint32_t none = 0xffffffff;                // an empty news field

// The person record at `index` of the person table.
inline std::uint32_t person_record(guest::Cpu& c, std::uint32_t index) { return c.u32(c.toc(toc::people)) + index * person_size; }
// The club record at `index` of the club table.
inline std::uint32_t club_record(guest::Cpu& c, std::uint32_t index) { return c.u32(c.toc(toc::clubs)) + index * club_size; }
// Looks up the person's contract (the callers' scratch cell keeps the result), returns it or 0.
inline std::uint32_t lookup_contract(guest::Cpu& c, std::uint32_t person, std::uint32_t back) {
    const auto contract = invoke(c, find_contract, back, {c.u32(c.toc(toc::contracts)), person});
    c.put32(c.toc(toc::found_contract), contract);
    return contract;
}
// The club of a contract found by lookup_contract, 0 without a contract.
inline std::uint32_t contract_club(guest::Cpu& c, std::uint32_t contract) {
    return contract == 0 ? 0 : club_record(c, c.u32(contract + contract_club_index));
}
// The contract's signed state (byte +0x38 shifted right by two) is the one of a released player.
inline bool contract_state_is(guest::Cpu& c, std::uint32_t contract, int state) {
    return std::int32_t(c.i8(contract + contract_state)) >> 2 == state;
}
// A live contract exists and is not in state 5.
inline bool live_contract(guest::Cpu& c, std::uint32_t contract) { return contract != 0 && !contract_state_is(c, contract, 5); }
// The club of a live contract, 0 otherwise.
inline std::uint32_t live_contract_club(guest::Cpu& c, std::uint32_t contract) {
    return live_contract(c, contract) ? contract_club(c, contract) : 0;
}
// A word read through a record that may be null: the original reads guest address 0 then.
inline std::uint32_t word_of(guest::Cpu& c, std::uint32_t record, std::uint32_t offset = 0) { return c.u32(record + offset); }
inline std::uint32_t sign8(guest::Cpu& c, std::uint32_t address) { return std::uint32_t(std::int32_t(c.i8(address))); }
// Four fields from `first`: the record's id and the identifiers of the three names behind it (a
// person's, or what a club's manager record holds the same way).
inline void identity_fields(guest::Cpu& c, const NewsItem& item, std::uint32_t record, std::uint32_t first,
                            const std::array<std::uint32_t, 4>& back) {
    item.field(first, c.u32(record), back[0]);
    item.field(first + 1, c.u32(c.u32(record + person_name) + name_id), back[1]);
    item.field(first + 2, c.u32(c.u32(record + person_club_name) + name_id), back[2]);
    item.field(first + 3, c.u32(c.u32(record + person_nation) + name_id), back[3]);
}
// Fields 0-3 for a person.
inline void person_fields(guest::Cpu& c, const NewsItem& item, std::uint32_t person, const std::array<std::uint32_t, 4>& back) {
    identity_fields(c, item, person, 0, back);
}
}
