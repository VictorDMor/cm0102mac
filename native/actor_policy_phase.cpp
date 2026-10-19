#include "actor_policy.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_policy {
using Call = SemanticGuestCall;
void phase_marker(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10285790,dependencies,observer);
    a.at(0x10285790); a.load(0,3,0x1a4); a.compare(0,0,false);
    if (a.equal()) {
        a.at(0x102857ac); a.value(3,0x0); a.finish();
    } else {
        a.at(0x1028579c); a.load(3,3,0x1a4); a.load<1>(0,3,0x791f); a.sign8(3,0); a.finish();
    }
}
}
