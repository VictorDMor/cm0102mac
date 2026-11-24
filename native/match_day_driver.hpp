#pragma once
#include <cstdint>

namespace cm::aot {
struct State;

// How the background match day (0x101cb9d0) orders its work.
//  original: the generated routine runs (the reference; every equivalence check uses it).
//  in_order: the C++ driver below, match by match as the original: set-up, play, post-match. A check of the
//            driver itself: a replay must end as with `original`.
//  phases:   the C++ driver sets up every match of a stretch of the day first, plays them, then runs their
//            post-match work in fixture order. The order of RNG draws differs from the original
//            (CLAUDE.md, the approved exception; TASKS.md, "Parallel play phase").
//  parallel: as phases, with the plays on guest workers, several at once: each match has its own RNG stream
//            and the shared writes of its play are held back for the post-match phase (match_play.hpp).
//            CM_MATCH_WORKERS sets the number of workers (default: the performance cores, at most 8).
enum class MatchDayOrder { original, in_order, phases, parallel };
// Initially from CM_MATCH_DAY_ORDER (original, in_order, phases, parallel); original when unset.
MatchDayOrder match_day_order();
void set_match_day_order(MatchDayOrder);
bool try_modern_match_day_driver(State&, std::uint32_t entry);
}
