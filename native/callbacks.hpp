#pragma once
#include <cstdint>
#include <span>
namespace cm::aot {
struct State;
std::uint32_t call_original(State& state, std::uint32_t transition_vector, std::span<const std::uint32_t> arguments);
}
