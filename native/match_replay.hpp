#pragma once
#include "aot_runtime.hpp"
#include <filesystem>

namespace cm::aot {
// Diagnostic fixture captured from a private, real-game match session.
bool match_snapshot_requested();
void capture_match_snapshot(const State& state);
void load_match_snapshot(State& state, const std::filesystem::path& path);
void save_runtime_snapshot(const State& state, const std::filesystem::path& path);
std::uint64_t match_snapshot_memory_hash(const State& state);
}
