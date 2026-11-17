#include "match_phase_timing.hpp"
#include <chrono>
#include <ostream>

namespace cm::aot::match_phase_timing {
namespace {
std::uint64_t monotonic_now() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
constexpr std::array names{"advance", "clock_and_restarts", "tactics_and_ball", "marking", "actor_turns", "reports",
                           "event_simulation", "actor_dispatch", "context_action", "secondary_action", "tendencies"};
static_assert(names.size() == static_cast<unsigned>(Phase::count));
}
Recorder::Recorder(std::uint64_t (*clock)()) : previous_(active), now_(clock ? clock : monotonic_now) { active = this; }
Recorder::~Recorder() { active = previous_; }
void Scope::finish(std::uint64_t now) {
    auto& total = recorder_->totals_[static_cast<unsigned>(phase_)];
    ++total.calls;
    total.nanoseconds += now - started_;
}
void Recorder::report(std::ostream& out) const {
    // advance includes its phases; named routines can nest in them and recurse.
    // Only the four tick phases are sequential. Do not sum all rows.
    for (unsigned i = 0; i < totals_.size(); ++i)
        out << "match_phase " << names[i] << " calls=" << totals_[i].calls
            << " ns=" << totals_[i].nanoseconds << '\n';
}
}
