#pragma once
// MANAGER_MANAGER.CPP board reaction block.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101a05c0: the two sets of flags the board draws from a job record (`record`: club index at +0x8, person
// index at +0xc, a byte at +0x24 that ends the judging when it is 1). The person's own figures fill the
// halfword at `flags_a`: the club's market value against the person's valuation (and the club's reputation),
// then what the pair queries 0x100d9480/0x100d94d0/0x100d9520/0x100d9580 say about the club. The person's
// employer (person +0x24, when it has one) fills `flags_b` the same way. Both halfwords are cleared first,
// so a record that ends the judging at once (+0x24 is 1) leaves them unwritten. The valuations also move
// the club's row (a double at +0x38 of the board's row, added for the club and taken for the employer).
void judge(guest::Cpu& c, std::uint32_t object, std::uint32_t record, std::uint32_t flags_a, std::uint32_t flags_b);
// 0x101a2eb0: the board's reaction to a job record. Judges it (0x101a05c0, into two halfwords of its frame:
// reading them back reads whatever the stack held when the judging ended at once), then moves the club's row
// (value, confidence and trust halfwords) by the fixed steps of the first set's lowest flag, and the
// employer's row by those of the second set, each clamped (value 1..10000, trust at least -10000), unless
// the person has no employer, a flagged employer (0x100c7d00) or one whose record (0x10103510) has the byte
// at +0x165 set to 1. Ends with 0x100d9e10 on the employer's second person.
void react(guest::Cpu& c, std::uint32_t object, std::uint32_t record);
}
