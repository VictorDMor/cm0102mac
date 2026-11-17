#pragma once
// TRANSFER_MANAGER.CPP native-first block "lists": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x103f9200: from the start date a year ahead, the first day the record's rule row accepts (any day without a
// row), written to `out`; none within the year reports an error and clears the error flag.
void first_accepted_day(guest::Cpu& c, std::uint32_t manager, std::uint32_t record, std::uint32_t out);
// 0x103f3670: the history's dated entries (0x28 bytes, date at +0x20) from a day and year up to a day and year, into the
// list whose pointer is at `list_cell` (grown 100 at a time; its size cell is `capacity_cell`, set to the count at the end):
// a list that already covers the range stays; else the manager's table, then the yearly files, are read.
void list_dated_entries(guest::Cpu& c, std::uint32_t manager, std::uint32_t from_day, std::uint32_t from_year, std::uint32_t to_day,
                        std::uint32_t to_year, std::uint32_t list_cell, std::uint32_t capacity_cell);
// 0x103f6240: moves a person's contract (found for the club) to the stage `stage_word` (the low nibble of its byte
// +0x60; -1 leaves it): the mood changes per stage, and, with a fixture a week away, the draws deciding whether the
// person's contract talks go on (flags set or cleared on the contract, the person's club left); `flag_out` gets a byte
// 1 for some stages; without a contract the error flag is cleared.
void advance_contract_stage(guest::Cpu& c, std::uint32_t manager, std::uint32_t person_record, std::uint32_t club,
                            std::uint32_t amount, std::uint32_t flag_out, std::uint32_t stage_word, std::uint32_t reasons,
                            std::uint32_t clear_note);
}
