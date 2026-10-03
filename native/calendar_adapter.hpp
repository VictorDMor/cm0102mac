#pragma once
#include "aot_runtime.hpp"
namespace cm::aot {
// Invoked at generated entry labels, covering both same-page direct branches
// and dispatcher/callback/indirect entry. False keeps the private reference.
bool try_modern_calendar(State& state, std::uint32_t entry);
void open_calendar_audit(const std::filesystem::path& path, bool record, bool perturb = false,
                         bool perturb_difference = false, bool perturb_ordering = false,
                         bool include_before = false, bool perturb_before = false);
void close_calendar_audit(bool require_end = false);
// Separate month stream; historical calendar formats remain byte-identical.
void open_month_audit(const std::filesystem::path& path, bool record, bool perturb = false);
void close_month_audit(bool require_end = false);
// Separate CMDOM001 stream; neither historical format is changed.
void open_day_of_month_audit(const std::filesystem::path& path, bool record, bool perturb = false);
void close_day_of_month_audit(bool require_end = false);
std::string calendar_call_summary(const State& state);
}
