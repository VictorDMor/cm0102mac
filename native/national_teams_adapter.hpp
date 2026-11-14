#pragma once

#include <cstdint>

namespace cm::aot {
struct State;
bool try_modern_national_teams(State&, std::uint32_t);
void national_teams_run_original(State&, std::uint32_t);
void national_teams_run_rewrite(State&, std::uint32_t);
}
