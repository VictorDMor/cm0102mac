#pragma once
#include <cstdint>
namespace cm::aot {
struct State;
// Used only by statically identified PowerPC fctiwz instructions.
void original_fctiwz(State& state, unsigned destination, unsigned source);
}
