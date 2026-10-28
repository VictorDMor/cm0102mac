#pragma once
#include "guest_abi.hpp"
namespace cm::aot::oceania_nations {
std::uint32_t construct(guest::Cpu&, std::uint32_t object, std::uint32_t year, std::uint32_t definition);
std::uint32_t destroy(guest::Cpu&, std::uint32_t object, std::uint32_t deleting);
std::uint32_t calendar(guest::Cpu&, std::uint32_t object, std::uint32_t stage, std::uint32_t count, std::uint32_t kind, std::uint32_t flag);
void describe_event(guest::Cpu&, std::uint32_t object, std::uint32_t club, std::uint32_t event, std::uint32_t stage);
std::uint32_t configure(guest::Cpu&, std::uint32_t object);
void finalize(guest::Cpu&, std::uint32_t object);
std::uint32_t notify(guest::Cpu&, std::uint32_t object, std::uint32_t club, std::uint32_t event, std::uint32_t stage, std::uint32_t record);
void next_season(guest::Cpu&, std::uint32_t object);
void advance_stage(guest::Cpu&, std::uint32_t object);
std::uint32_t create_group(guest::Cpu&, std::uint32_t object, std::uint32_t group);
void create_finals(guest::Cpu&, std::uint32_t object);
std::uint32_t select_entrants(guest::Cpu&, std::uint32_t object);
void update(guest::Cpu&, std::uint32_t object, std::uint32_t date, std::uint32_t active);
void record_positions(guest::Cpu&, std::uint32_t object);
void update_record(guest::Cpu&, std::uint32_t object, std::uint32_t event, std::uint32_t stage, std::uint32_t first, std::uint32_t second, std::uint32_t third);
}
