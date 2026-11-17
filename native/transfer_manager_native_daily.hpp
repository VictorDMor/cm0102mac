#pragma once
// TRANSFER_MANAGER.CPP native-first block "daily": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include "transfer_manager_native_detail.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// Shared by the daily block's files: the contract lookup every routine here repeats, and the date order.
namespace daily {
inline constexpr std::uint32_t find_contract = 0x10082f20, club_flag = 0x1014c2e0;
inline constexpr std::uint32_t offer_list = 4, offer_club = 8, offer_person = 0xc, offer_stage = 0x34;
// The cells and tables the lookups go through.
struct Lookup {
    guest::Cpu& c;
    std::uint32_t contracts = c.toc(toc::contracts), found_cell = c.toc(toc::found_contract), clubs = c.toc(toc::clubs),
                  people = c.toc(toc::people);
    std::uint32_t person_of(std::uint32_t offer) const { return c.u32(people) + c.u32(offer + offer_person) * person_size; }
    std::uint32_t club_at(std::uint32_t index) const { return c.u32(clubs) + index * club_size; }
    std::uint32_t offer_club_of(std::uint32_t offer) const { return club_at(c.u32(offer + offer_club)); }
    // 0x10082f20 for a person; the result stays in the shared found-contract cell.
    std::uint32_t find(std::uint32_t person, std::uint32_t back) const {
        const auto found = invoke(c, find_contract, back, {c.u32(contracts), person});
        c.put32(found_cell, found);
        return found;
    }
    // Whether a contract is one the sides use (a contract of kind 5 is not).
    bool usable(std::uint32_t found) const { return found != 0 && (std::int32_t(c.i8(found + 0x38)) >> 2) != 5; }
    // The club of the contract in the found-contract cell (0 without one).
    std::uint32_t club_of_found(std::uint32_t found) const {
        return found ? club_at(c.u32(c.u32(found_cell) + 4)) : 0;
    }
};
// Whether the date (hi, lo) is before (hi2, lo2): the low halfword first (unsigned), then the high one (signed).
inline bool before(std::int16_t hi, std::uint16_t lo, std::int16_t hi2, std::uint16_t lo2) {
    return lo < lo2 || (lo == lo2 && hi < hi2);
}
}
// 0x103ea840: the transfer manager's pass for one game date (the low byte of `phase_register`, 0..2, is one of the day's three passes): runs the
// date rules and the offers due, steps the offer lists, and in phase 2 rolls the manager's clock on.
void run_daily_pass(guest::Cpu& c, std::uint32_t manager, std::uint32_t date, std::uint32_t phase_register);
// 0x103eb650: advances the offer whose pointer sits at `slot` in the manager's offer array, by its stage.
void process_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t slot);
// 0x103ec0f0: for each rule record of the manager (+0x9d8) whose month is this, the previous or the next
// month's, tests its date against the date `date` and calls the nation's rule object.
void dispatch_rule_dates(guest::Cpu& c, std::uint32_t manager, std::uint32_t date);
// 0x103e9ce0: for a nation, opens the offers of its clubs that are due and hands the rest on.
void process_nation_offers(guest::Cpu& c, std::uint32_t manager, std::uint32_t nation);
// 0x103e8280: decides an offer whose terms were answered (`final_answer` set after the club's reply).
void decide_answered_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t final_answer);
// 0x103f3100: for the manager's dated rows due by now, takes the offer each names through the terms test,
// then removes the rows done from the front of the table.
void run_due_offers(guest::Cpu& c, std::uint32_t manager);
}
