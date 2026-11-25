#pragma once
#include "guest_worker.hpp"
#include <memory>
#include <vector>

namespace cm::aot {
// The guest workers of the game's State (own stack and scratch heap each), made at first use and kept: the
// matches' play phases and the shortlist manager's candidate judging share them, one user at a time.
// CM_MATCH_WORKERS sets their number (default: the performance cores, at most 8).
std::vector<std::unique_ptr<GuestWorker>>& guest_workers(State& main);
}
