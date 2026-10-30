#pragma once
#include "guest_abi.hpp"

namespace cm::aot::award_shortlist {
using guest::Cpu;
std::uint32_t construct(Cpu&, std::uint32_t object, std::uint32_t specification, std::uint32_t file);
std::uint32_t destroy(Cpu&, std::uint32_t object, std::uint32_t deleting);
std::uint32_t load(Cpu&, std::uint32_t object, std::uint32_t file);
void save(Cpu&, std::uint32_t object, std::uint32_t file);
std::uint32_t add(Cpu&, std::uint32_t object, std::uint32_t person, std::uint32_t club, double score);
void publish(Cpu&, std::uint32_t object, std::uint32_t date, std::uint32_t first, std::uint32_t second);
std::uint32_t vote(Cpu&, std::uint32_t object, std::uint32_t output, std::uint32_t limit);
std::uint32_t sample(Cpu&, std::uint32_t bound);
void record_vote(Cpu&, std::uint32_t object, std::uint32_t manager, std::uint32_t person);
std::uint32_t eligibility(Cpu&, std::uint32_t object, std::uint32_t manager);
void clear(Cpu&, std::uint32_t object);
std::uint32_t compare_names(Cpu&, std::uint32_t left, std::uint32_t right);
std::uint32_t compare_scores(Cpu&, std::uint32_t left, std::uint32_t right);
std::uint32_t compare_votes(Cpu&, std::uint32_t left, std::uint32_t right);
}
