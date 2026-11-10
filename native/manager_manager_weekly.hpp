#pragma once
// MANAGER_MANAGER.CPP weekly pass 0x101a3350 (manager_manager_weekly.cpp and its part files).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101a3350: the module's weekly pass over every club (called with the module object, *TOC[-0x79f0],
// by the daily update every seventh day): per club, the board row's value moves toward its target, and
// the club's events, hires and standing with its manager are decided by draws from the game's generator.
void weekly(guest::Cpu& c, std::uint32_t object);
}
