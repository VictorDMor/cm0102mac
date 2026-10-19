#include "actor_policy.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_policy {
using Call = SemanticGuestCall;
void column_excess(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10285750,dependencies,observer);
    a.at(0x10285750); a.link(); a.move(4,3); a.save_link(); a.frame(64); a.load(0,3,0x1a4); a.compare(0,0,false);
    if (a.equal()) {
        a.at(0x1028577c); a.value(3,0xffffffff);
    } else {
        a.at(0x1028576c); a.load(3,4,0x1a4); a.call(0x101e4e10); a.sign8(3,3); a.jump(0x10285780);
    }
    a.at(0x10285780); a.load(0,1,0x48); a.offset(1,1,0x40); a.restore_link(); a.finish();
}
}
