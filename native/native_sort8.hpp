#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
// Called only at the audited original entry label, including intra-page calls.
bool try_native_sort8(State&);
// Process-local eligible attempts, including faults and generated side exits.
// This diagnostic is never serialized in a career or runtime snapshot.
std::uint64_t native_sort8_calls();
}
