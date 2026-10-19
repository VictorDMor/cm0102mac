#include "match_event.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_event {
using Call = SemanticGuestCall;
void actor_address(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101e3180,dependencies,observer);
    a.at(0x101e3180); a.sign8(0,4); a.multiply(6,0,0x2350); a.sign8(0,5); a.multiply(0,0,0x1c4); a.add(6,6,0);
    a.offset(0,6,0x311c); a.add(3,3,0); a.finish();
}
}
