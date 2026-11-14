#pragma once
// INJURY.CPP: how injuries run their course (recovery, relapse and their news) and the club's queue of
// injury events (injury_unit_course.hpp).
#include "guest_abi.hpp"

namespace cm::aot::injury_unit {
// 0x1018b2b0: the pass over injuries after the daily update. Walks the clubs' people for injuries that end
// today, then the due queue (sorted through the game's search), and drops what has passed.
void daily_course_pass(guest::Cpu& c, std::uint32_t manager_object);
// 0x1018dcb0: one injury's recovery or relapse for `person`, with its notice. `stage`, `injury_type`,
// `queued` and `days` are the caller's registers as they come (the routine reads them as a signed byte,
// a signed byte, a byte test and a signed halfword).
void injury_course(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t stage,
                   std::uint32_t injury_type, std::uint32_t queued, std::uint32_t days);
// 0x1018bb50: adds an event to the club's queue (for some kinds, only after a draw), sorted by the game's sort.
void queue_club_event(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t date, std::uint32_t kind,
                      std::uint32_t club, std::uint32_t person);
// 0x1018bdd0: works through the queued club events that are due, then drops the passed ones.
void run_club_events(guest::Cpu& c, std::uint32_t manager_object);
// The three sort comparators (the game's sort reaches them through transition vectors): -1 when the row
// is later than the key, 1 when earlier, 0 when equal.
// 0x1018ff10: dates at +8. 0x1018ffe0 and 0x101900b0: dates at +0.
std::int32_t compare_due_dates_b(guest::Cpu& c, std::uint32_t key, std::uint32_t row);
std::int32_t compare_due_dates_a(guest::Cpu& c, std::uint32_t key, std::uint32_t row);
std::int32_t compare_event_dates(guest::Cpu& c, std::uint32_t key, std::uint32_t row);
}
