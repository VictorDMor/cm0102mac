#pragma once
// INJURY.CPP, block "onset": how a person gets a new injury, and the daily pass that hands injuries out.
#include "guest_abi.hpp"

namespace cm::aot::injury_unit {
// The arguments of 0x10188cc0 as the original receives them: r3..r10 and the word the ninth argument
// leaves at the caller's stack pointer + 0x38.
struct Registration {
    std::uint32_t manager;        // r3
    std::uint32_t type;           // r4: the injury type (the low byte, signed, is the type)
    std::uint32_t starts_later;   // r5: nonzero (signed byte) dates the injury a few days ahead
    std::uint32_t person;         // r6
    std::uint32_t match;          // r7: the match the injury happened in, or 0
    std::uint32_t report;         // r8: nonzero asks for the news of the club's injured list (types 0-2)
    std::uint32_t news_argument;  // r9: the extra the news item carries
    std::uint32_t chance;         // r10: signed byte, the injury waiting to happen (negative: one in -n/10)
    std::uint32_t pending_only;   // the byte at stack +0x3b (the ninth argument's word is at +0x38): only note the pending injury
};
// 0x10188cc0: registers a new injury for a person: the type, the length from the player's skill, the
// status row, the news. Gives the index of the injury record, or -1.
std::int32_t register_injury(guest::Cpu& c, const Registration& arguments);
// 0x1018aed0: puts the injury's news item together (the record's type, dates and lengths, the club and
// the people) and hands it to the news object.
void post_injury_news(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t record,
                      std::uint32_t news_argument);
// 0x1018b6e0: a person's status row without a pending injury picks one up from a pending type of
// another person's: the one best placed (a sample of the rows) gets a registration of a type from the
// class of the person, or loses the pending mark.
void settle_pending_injury(guest::Cpu& c, std::uint32_t manager, std::uint32_t status_row, std::uint32_t record);
// 0x10185410: the daily pass: counts the injuries down, refreshes everybody's status at the season
// boundary and, outside the summer weeks, samples people for injuries and illnesses.
void daily_injury_pass(guest::Cpu& c, std::uint32_t manager);
}
