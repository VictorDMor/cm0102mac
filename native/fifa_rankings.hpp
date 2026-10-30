#pragma once
#include "guest_abi.hpp"

namespace cm::aot::fifa_rankings {
using guest::Cpu;
std::uint32_t construct(Cpu&, std::uint32_t manager, std::uint32_t file);
std::uint32_t destroy(Cpu&, std::uint32_t manager, std::uint32_t deleting);
void record_match(Cpu&, std::uint32_t manager, std::uint32_t match);
void update(Cpu&, std::uint32_t manager, std::uint32_t date);
std::uint32_t ranked(Cpu&, std::uint32_t manager, std::uint32_t count);
std::uint32_t count_matches(Cpu&, std::uint32_t manager, std::uint32_t nation,
                          std::uint32_t first_month, std::uint32_t first_year,
                          std::uint32_t last_month, std::uint32_t last_year);
std::uint32_t load(Cpu&, std::uint32_t manager, std::uint32_t file);
void save(Cpu&, std::uint32_t manager, std::uint32_t file);
void initialize(Cpu&, std::uint32_t manager);
void append(Cpu&, std::uint32_t manager, std::uint32_t nation, std::uint32_t points, std::uint32_t match);
void assign_positions(Cpu&, std::uint32_t manager);
void calculate(Cpu&, std::uint32_t manager);
std::uint32_t category(Cpu&, std::uint32_t manager, std::uint32_t nation);
std::uint32_t compare_matches(Cpu&, std::uint32_t left, std::uint32_t right);
std::uint32_t compare_rankings(Cpu&, std::uint32_t left, std::uint32_t right);
}
