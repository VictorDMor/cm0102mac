#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
// Build-time specialization of bounded record conversions. Finite execution,
// active diagnostic observers and unknown entries retain the original body.
bool try_native_record_fields(State&, std::uint32_t entry);
}
