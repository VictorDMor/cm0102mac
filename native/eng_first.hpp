#pragma once
#include "guest_abi.hpp"

namespace cm::aot::eng_first {
std::uint32_t construct(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t destroy(guest::Cpu&, std::uint32_t, std::uint32_t);
std::uint32_t calendar(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t collect_clubs(guest::Cpu&, std::uint32_t);
std::uint32_t configure(guest::Cpu&, std::uint32_t);
void next_season(guest::Cpu&, std::uint32_t);
void advance_stage(guest::Cpu&, std::uint32_t);
void create_playoffs(guest::Cpu&, std::uint32_t);
std::uint32_t club_event(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
void describe_fixture(guest::Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
// Direct in production; checker parents and excluded entries use the dispatcher.
void call(guest::Cpu&, std::uint32_t entry, std::uint32_t back);
void set_fixture_places(guest::Cpu&, std::uint32_t competition, std::uint32_t fixture,
                        std::uint32_t stage, std::uint32_t first_place,
                        std::uint32_t second_place, std::uint32_t third_place);
}
