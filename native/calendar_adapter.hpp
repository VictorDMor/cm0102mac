#pragma once
#include "aot_runtime.hpp"
namespace cm::aot {
// Invoked at generated entry labels, covering both same-page direct branches
// and dispatcher/callback/indirect entry. False keeps the private reference.
bool try_modern_calendar(State& state, std::uint32_t entry);
// One candidate scan's 200-day cutoff. The date itself is read again at each
// boundary. Only the arithmetic for matching day/year inputs is reused.
struct CandidateCutoff {
    int input_day = -32768;
    std::uint16_t input_year = 0;
    std::uint16_t day = 0, year = 0;
    std::uint16_t leap_clobber_year = 0;
    bool leap = false, clobbers_r4 = false;
};
// Call with the guest subtraction arguments and link set (r5 = 200).
bool try_candidate_cutoff(State& state, CandidateCutoff& cutoff);
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
// Independent CMWKD001 stream; all three historical encodings remain intact.
void open_weekday_audit(const std::filesystem::path& path, bool record, bool perturb = false);
void close_weekday_audit(bool require_end = false);
std::string calendar_call_summary(const State& state);
}
