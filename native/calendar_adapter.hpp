#pragma once
#include "aot_runtime.hpp"
namespace cm::aot {
// Invoked at generated entry labels, covering both same-page direct branches
// and dispatcher/callback/indirect entry. False keeps the private reference.
bool try_modern_calendar(State& state, std::uint32_t entry);
void open_calendar_audit(const std::filesystem::path& path, bool record, bool perturb = false,
                         bool perturb_difference = false);
void close_calendar_audit(bool require_end = false);
std::string calendar_call_summary(const State& state);
}
