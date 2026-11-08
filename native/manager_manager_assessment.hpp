#pragma once
// MANAGER_MANAGER.CPP assessment block.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101ad080: a staff member's score for a job at a club (-10000 on the early
// rejections). `person` is the 0x70-byte person record (staff record at +0x1c),
// `second` another person (club+0x80 when null or without a staff record), `job`
// the job byte (sign-extended).
std::uint32_t assess(guest::Cpu& c, std::uint32_t person, std::uint32_t club, std::uint32_t second, std::uint32_t job);
}
