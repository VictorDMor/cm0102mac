#include "match_event.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_event {
using Call = SemanticGuestCall;
namespace {
void actor_pair_rule_1(Call&);
void actor_pair_rule_1(Call& a) {
    a.at(0x102010b8); a.value(3,0x0); a.finish();
}
}
void same_actor_pair(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10201080,dependencies,observer);
    a.at(0x10201080); a.load<1>(3,4,0x5); a.load<1>(0,5,0x5); a.sign8(3,3); a.sign8(0,0); a.compare_registers(3,0);
    if (!a.equal()) {
        a.at(0x102010b8);
        actor_pair_rule_1(a);
    } else {
        a.at(0x10201098); a.load<1>(3,4,0x6); a.load<1>(0,5,0x6); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x102010b8);
            actor_pair_rule_1(a);
        } else {
            a.at(0x102010b0); a.value(3,0x1); a.finish();
        }
    }
}
}
