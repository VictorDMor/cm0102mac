#pragma once
#include "guest_abi.hpp"

namespace cm::aot::awol {
using guest::Cpu;
// AWOL.CPP: the staff absence records. One flag byte per staff member (bit 0
// primary absence, bit 1 training absence) and three lists of dated records.
std::uint32_t construct(Cpu&, std::uint32_t records, std::uint32_t file);
std::uint32_t destroy(Cpu&, std::uint32_t records, std::uint32_t deleting);
std::uint32_t load(Cpu&, std::uint32_t records, std::uint32_t file);
std::uint32_t save(Cpu&, std::uint32_t records, std::uint32_t file);
std::uint32_t primary_status(Cpu&, std::uint32_t records, std::uint32_t staff, std::uint32_t raw);
std::uint32_t secondary_status(Cpu&, std::uint32_t records, std::uint32_t staff);
std::uint32_t pending_status(Cpu&, std::uint32_t records, std::uint32_t staff);
void remove_primary_by_staff(Cpu&, std::uint32_t records, std::uint32_t staff);
void remove_primary_item(Cpu&, std::uint32_t records, std::uint32_t item);
void remove_secondary_item(Cpu&, std::uint32_t records, std::uint32_t item);
void remove_secondary_by_staff(Cpu&, std::uint32_t records, std::uint32_t staff);
void notify(Cpu&, std::uint32_t staff, std::uint32_t club, std::uint32_t kind, std::uint32_t value);
std::uint32_t sample(Cpu&, std::uint32_t bound);
void register_absence(Cpu&, std::uint32_t records, std::uint32_t staff, std::uint32_t club, std::uint32_t kind);
void notify_training(Cpu&, std::uint32_t staff, std::uint32_t club);
void describe_primary(Cpu&, std::uint32_t message, std::uint32_t brief, std::uint32_t output);
void describe_training(Cpu&, std::uint32_t message, std::uint32_t brief, std::uint32_t output);
void describe_fitness(Cpu&, std::uint32_t message, std::uint32_t brief, std::uint32_t output);
void review_daily(Cpu&, std::uint32_t records);
void training_event(Cpu&, std::uint32_t records, std::uint32_t staff);
void fitness_event(Cpu&, std::uint32_t records, std::uint32_t staff, std::uint32_t club);
}
