#pragma once
#include "player_decision.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {
bool player_decision_reference(State&);
void player_decision_reference_step_seen(State&, std::uint32_t);
void player_decision_reference_call_seen(State&, std::uint32_t, std::uint32_t);
}
namespace cm::player_decision_checks {
using namespace cm::semantic_io_checks;
inline constexpr auto entry = cm::aot::player_decision::entry;
inline constexpr std::uint32_t end = 0x10285748;
struct Call {
    std::uint32_t pc, target;
    std::array<std::uint32_t, 8> arguments;
    decltype(State::registers) before;
    bool operator==(const Call&) const = default;
};
struct Trace {
    std::set<std::uint32_t> instructions;
    std::vector<std::uint32_t> ordered;
    std::vector<Call> calls;
};
inline thread_local Trace* active_trace = nullptr;
struct TraceScope {
    Trace* previous;
    explicit TraceScope(Trace& trace) : previous(active_trace) { active_trace = &trace; }
    ~TraceScope() { active_trace = previous; }
};
struct Probe final : SemanticGuestObserver {
    void before_step(State& s, std::uint32_t pc) override { player_decision_reference_step_seen(s, pc); }
    void before_dependency(State& s, std::uint32_t pc, std::uint32_t target) override { player_decision_reference_call_seen(s, pc, target); }
};
inline Probe probe;
}
