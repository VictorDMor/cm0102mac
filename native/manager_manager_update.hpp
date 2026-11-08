#pragma once
// MANAGER_MANAGER.CPP post-match manager update block.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101c15f0: drops the club's entry from the object's pending list (+0x10). Like departure
// (0x101c1690) without the posting; no result.
void drop_pending(guest::Cpu& c, std::uint32_t object, std::uint32_t club);
// 0x1019e990: the club's manager after a match. `match` is the match record (0 when the match
// has no squad data), `home` a byte: set when `club` is the fixture's first club (+0x1c). The
// result and the sides' standing move the board's row for the club (value, trust and the three
// other figures), start the events the result calls for, and end with the board's review
// (0x101a8ad0) or the pending list's clean-up (0x101c15f0), except for a fixture of round 0xa0,
// which returns before either.
void update(guest::Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t fixture, std::uint32_t match,
            std::uint32_t home);
// 0x1019e8f0: the module's hook of the post-match broadcast: updates the fixture's two clubs that
// have a manager (the first club as home), unless the fixture is voided (+0 is -2) or unplayed (+0x40).
void after_match(guest::Cpu& c, std::uint32_t object, std::uint32_t fixture, std::uint32_t match);
}
