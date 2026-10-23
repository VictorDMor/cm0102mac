#pragma once
#include "aot_runtime.hpp"
#include <optional>
#include <span>
#include <string>

namespace cm::aot {
// Private, versioned diagnostic fixtures; never part of the game's save format.
struct SeasonReplayComplete {};
void season_day_boundary(State& state, std::uint32_t pc);
void season_record_import_begin(State& state);
void season_record_import_end(State& state, const char* name = nullptr);
void season_record_callback_begin(State& state, std::uint32_t vector,
                                  std::span<const std::uint32_t> arguments);
void season_record_callback_end(State& state, std::uint32_t result);
bool season_replay_import(State& state, const char* name);
void open_season_replay(const std::filesystem::path& directory, bool semantic_calendar = false);
// A bounded operation (a captured world pass): pure imports such as pow and
// BlockMoveData run live through the bridges and their records are skipped;
// OS imports replay from the transcript. Instruction and native-call counts
// are diagnostics here. The end state is checked by the caller.
void open_operation_replay(const std::filesystem::path& directory);
// Version 2 operation replays check the heap against a model built from the
// recorded heap changes; seed it with the replay's initial state, before
// anything runs.
void seed_season_replay(const State& initial);
// For timing: skip the recorded RNG state writes instead of checking each
// one through a memory observer (an observer also switches C++ owners to
// their observed paths), and skip the heap check before each OS import.
// The caller's end-state check still applies.
void open_timing_replay(const std::filesystem::path& directory);
void close_season_replay();
// Session captures for the automation channel: a directory starts recording
// at the end of the next import; nullopt stops at the next OS import. Returns
// a status line; throws when the request cannot be taken.
std::string season_session_request(const std::optional<std::filesystem::path>& directory);
// Replays a session capture: ends with SeasonReplayComplete at the OS import
// the capture stopped before.
void open_session_replay(const std::filesystem::path& directory, bool timing);
// After seed_season_replay, on the State that runs the session.
void begin_session_replay(State& state);
// In a forked child (native/routine_check.cpp): read the transcript through a
// file offset of its own.
void season_replay_after_fork();
// Direct dependency fixtures can end after the last import, without a day
// boundary. Require physical EOF/checksum before closing those short fixtures.
void require_season_replay_eof();
// Empty means exact agreement. Reports named game/RNG fields first, then the
// first architectural or allocated-memory byte. No volatile fields are masked.
std::string compare_season_states(const State& expected, const State& actual, bool semantic_calendar = false);
// For an operation replay: as above, but instruction and native-call counts
// are diagnostics, and only registers the caller can read are compared.
std::string compare_operation_states(const State& expected, const State& actual);
std::string season_state_summary(const State& state);
}
