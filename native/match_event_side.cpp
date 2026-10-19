#include "match_event.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_event {
using Call = SemanticGuestCall;
void ball_cell_side(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101fea10,dependencies,observer);
    a.at(0x101fea10); a.link(); a.store(1,-0x4,31); a.save_link(); a.frame(64); a.move(31,3); a.move(3,31);
    a.value(4,0x0); a.load<1>(5,31,0x791d); a.load<1>(6,31,0x791e); a.call(0x101eea30); a.sign8(0,3);
    a.compare(0,0);
    if (a.equal()) {
        a.at(0x101fea4c); a.move(3,31); a.value(4,0x1); a.load<1>(5,31,0x791d); a.load<1>(6,31,0x791e);
        a.call(0x101eea30); a.sign8(0,3); a.compare(0,0);
        if (a.equal()) {
            a.at(0x101fea74); a.value(3,0x0);
        } else {
            a.at(0x101fea6c); a.value(3,0xffffffff); a.jump(0x101fea78);
        }
    } else {
        a.at(0x101fea44); a.value(3,0x1); a.jump(0x101fea78);
    }
    a.at(0x101fea78); a.pop(64); a.load(31,1,-0x4); a.finish();
}
}
