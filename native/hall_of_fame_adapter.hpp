#pragma once

#include <cstdint>

namespace cm::aot {
struct State;
namespace engine_initialization { struct DependencyDispatch; }
inline constexpr auto hall_of_fame_static_table_entry = 0x10143240u;
bool hall_of_fame_wants_modern(std::uint32_t);
void hall_of_fame_initialize_static_table(State&, engine_initialization::DependencyDispatch* = nullptr);
bool try_modern_hall_of_fame(State&, std::uint32_t);
void hall_of_fame_run_original(State&, std::uint32_t);
void hall_of_fame_run_rewrite(State&, std::uint32_t);
}
