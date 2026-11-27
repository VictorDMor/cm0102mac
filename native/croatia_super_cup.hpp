#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
namespace croatia_super_cup {
void new_career(State&);
bool restore(State&, std::uint32_t country);
void before_rollover(State&, std::uint32_t country);
void after_rollover(State&, std::uint32_t country);
bool owns(State&, std::uint32_t object);
bool calendar(State&);
}
}
