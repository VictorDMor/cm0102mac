#pragma once
#include <cstdint>
#include <string_view>

namespace cm::aot {
struct State;
enum class DebugText { best_xi, add_goal, goal, on, off, recover_squad };
// Original language IDs and single-byte game text encoding; unknown IDs use English.
std::string_view debug_text(unsigned language, DebugText text);
std::string_view debug_text(const State&, DebugText text);
}
