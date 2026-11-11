#pragma once
// MANAGER_MANAGER.CPP assessment block, written native-first: 0x101c3220 (a club's assessment of a
// manager it is offered), with 0x101c1e20 (the verdict) and 0x101c3860 (the message that reports
// a stage) which only it calls.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101c3860: files a message of kind 0x178f about the manager record `person` and the club record
// `club` with the news desk: the person's index and three record ids, the club's index, four byte
// arguments, whether the club's budget (+4 of its record at +0x3c) is under a limit scaled by its
// reputation and tier, and whether its reputation and standing byte put it in the upper band. With
// a null person or club it reports the assertion instead and clears the success flag.
void post_assessment_message(guest::Cpu& c, std::uint32_t person, std::uint32_t club, std::int8_t first,
                             std::int8_t second, std::int8_t third, std::int8_t fourth);
// 0x101c1e20: the verdict on a manager offered to a club, a byte written at +9 of `record` (+8 holds the
// kind of assessment, 1 to 3 and the rest as the last case). `list` is the object whose rows (0x4c bytes) hold the
// clubs' states; the rows' fields are updated as the cases below do. Returns the verdict (the byte at +9 as the
// routine leaves it).
std::int8_t assess_verdict(guest::Cpu& c, std::uint32_t list, std::uint32_t record, std::uint32_t club,
                           std::uint32_t person);
// 0x101c3220: a club's assessment of the manager `person` for the club record `club`, `kind` a byte. Files the
// reports of its stages (0x101c3860), takes the verdict of 0x101c1e20 and acts on it: refusals lower the club's
// row rating, an acceptance posts a record to the list at +4 and flags the row.
void assess_manager(guest::Cpu& c, std::uint32_t list, std::uint32_t person, std::uint32_t club, std::uint32_t kind);
}
