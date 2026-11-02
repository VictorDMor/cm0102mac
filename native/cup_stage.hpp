#pragma once
#include "guest_abi.hpp"

namespace cm::aot::cup_stage {
using guest::Cpu;
std::uint32_t construct(Cpu&, std::uint32_t stage, std::uint32_t parent, std::uint32_t entrant_count,
                        std::uint32_t clubs, std::uint32_t round_count, std::uint32_t definition,
                        std::uint32_t schedule, std::uint32_t year);
std::uint32_t allocate_for_load(Cpu&, std::uint32_t stage, std::int16_t entrants, std::int16_t rounds, std::uint32_t match_counts);
std::uint32_t load(Cpu&, std::uint32_t stage, std::uint32_t file);
void save(Cpu&, std::uint32_t stage, std::uint32_t file);
std::uint32_t build_draw(Cpu&, std::uint32_t stage);
std::uint32_t random_below(Cpu&, std::uint32_t bound);
std::uint32_t destroy(Cpu&, std::uint32_t stage, std::int16_t deleting);
std::uint32_t initialize_entrants(Cpu&, std::uint32_t stage, std::int32_t count,
                                 std::uint32_t clubs, std::int8_t numbered,
                                 std::uint32_t seeds);
}
