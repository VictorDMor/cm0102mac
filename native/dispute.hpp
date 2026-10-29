#pragma once
#include "guest_abi.hpp"

namespace cm::aot::dispute {
using guest::Cpu;
std::uint32_t construct(Cpu&, std::uint32_t object, std::uint32_t file);
std::uint32_t destroy(Cpu&, std::uint32_t object, std::uint32_t deleting);
std::uint32_t load(Cpu&, std::uint32_t object, std::uint32_t file);
std::uint32_t save(Cpu&, std::uint32_t object, std::uint32_t file);
std::uint32_t active(Cpu&, std::uint32_t object, std::uint32_t person, std::uint32_t unchecked);
void update(Cpu&, std::uint32_t object);
std::uint32_t club_id(Cpu&, std::uint32_t object, std::uint32_t person);
void remove_person(Cpu&, std::uint32_t object, std::uint32_t person);
void remove_record(Cpu&, std::uint32_t object, std::uint32_t record);
void start(Cpu&, std::uint32_t object, std::uint32_t person, std::uint32_t club,
           std::uint32_t reason, std::uint32_t temporary);
std::uint32_t sample(Cpu&, std::uint32_t bound);
void publish(Cpu&, std::uint32_t object, std::uint32_t person, std::uint32_t club,
             std::uint32_t phase, std::uint32_t reason, std::uint32_t temporary);
void render(Cpu&, std::uint32_t object, std::uint32_t item, std::uint32_t detailed, std::uint32_t output);
}
