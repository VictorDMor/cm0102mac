#include "player_decision_test_trace.hpp"
namespace cm::aot {
void player_decision_reference_step_seen(State&, std::uint32_t pc) {
    auto* trace = cm::player_decision_checks::active_trace;
    if (trace) { trace->instructions.insert(pc); trace->ordered.push_back(pc); }
}
void player_decision_reference_call_seen(State& state, std::uint32_t pc, std::uint32_t target) {
    auto* trace = cm::player_decision_checks::active_trace;
    if (!trace) return;
    cm::player_decision_checks::Call call{pc, target, {}, state.registers};
    for (unsigned n = 0; n < call.arguments.size(); ++n) call.arguments[n] = std::uint32_t(state.r<4>((n + 3) * 4));
    trace->calls.push_back(call);
}
}
