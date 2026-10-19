#include "actor_flags.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_flags {
using Call = SemanticGuestCall;
void test_bit21(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x102862b0,dependencies,observer);
    a.at(0x102862b0); a.link(); a.save_link(); a.frame(64); a.load<1>(0,3,0x27); a.load(5,3,0x1a4); a.sign8(0,0);
    a.load<1>(4,3,0x19); a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x10113e10);
    a.value(6,0x200000); a.value(7,0x0); a.offset(6,6,0x0); a.value(5,0x0); a.intersect(6,4,6); a.intersect(4,3,7);
    a.value(0,0x0); a.combine(3,6,5,false,true); a.combine(0,4,0,false,true); a.combine(0,3,0);
    a.offset(3,0,-1); state.wr<1>(1027,a.r(0) != 0); a.subtract_extended(0,3,0); a.sign8(3,0); a.pop(64);
    a.finish();
}
void test_bit25(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10286320,dependencies,observer);
    a.at(0x10286320); a.link(); a.save_link(); a.frame(64); a.load<1>(0,3,0x27); a.load(5,3,0x1a4); a.sign8(0,0);
    a.load<1>(4,3,0x19); a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x10113e10);
    a.value(6,0x2000000); a.value(7,0x0); a.offset(6,6,0x0); a.value(5,0x0); a.intersect(6,4,6);
    a.intersect(4,3,7); a.value(0,0x0); a.combine(3,6,5,false,true); a.combine(0,4,0,false,true); a.combine(0,3,0);
    a.offset(3,0,-1); state.wr<1>(1027,a.r(0) != 0); a.subtract_extended(0,3,0); a.sign8(3,0); a.pop(64);
    a.finish();
}
void test_bit29(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10286390,dependencies,observer);
    a.at(0x10286390); a.link(); a.save_link(); a.frame(64); a.load<1>(0,3,0x27); a.load(5,3,0x1a4); a.sign8(0,0);
    a.load<1>(4,3,0x19); a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x10113e10);
    a.value(6,0x20000000); a.value(7,0x0); a.offset(6,6,0x0); a.value(5,0x0); a.intersect(6,4,6);
    a.intersect(4,3,7); a.value(0,0x0); a.combine(3,6,5,false,true); a.combine(0,4,0,false,true); a.combine(0,3,0);
    a.offset(3,0,-1); state.wr<1>(1027,a.r(0) != 0); a.subtract_extended(0,3,0); a.sign8(3,0); a.pop(64);
    a.finish();
}
void test_bit27(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10286400,dependencies,observer);
    a.at(0x10286400); a.link(); a.save_link(); a.frame(64); a.load<1>(0,3,0x27); a.load(5,3,0x1a4); a.sign8(0,0);
    a.load<1>(4,3,0x19); a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x10113e10);
    a.value(6,0x8000000); a.value(7,0x0); a.offset(6,6,0x0); a.value(5,0x0); a.intersect(6,4,6);
    a.intersect(4,3,7); a.value(0,0x0); a.combine(3,6,5,false,true); a.combine(0,4,0,false,true); a.combine(0,3,0);
    a.offset(3,0,-1); state.wr<1>(1027,a.r(0) != 0); a.subtract_extended(0,3,0); a.sign8(3,0); a.pop(64);
    a.finish();
}
void read_flags(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10286470,dependencies,observer);
    a.at(0x10286470); a.link(); a.save_link(); a.frame(64); a.load<1>(0,3,0x27); a.load(5,3,0x1a4); a.sign8(0,0);
    a.load<1>(4,3,0x19); a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x10113e10); a.pop(64);
    a.finish();
}
void read_flags_alternate(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x102864b0,dependencies,observer);
    a.at(0x102864b0); a.link(); a.save_link(); a.frame(64); a.load<1>(0,3,0x27); a.load(5,3,0x1a4); a.sign8(0,0);
    a.load<1>(4,3,0x19); a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x10113e10); a.pop(64);
    a.finish();
}
void team_or_slot_status(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x102864f0,dependencies,observer);
    a.at(0x102864f0); a.link(); a.save_link(); a.frame(64); a.load<1>(0,3,0x27); a.load(4,3,0x1a4); a.sign8(0,0);
    a.multiply(0,0,0x18e0); a.offset(4,4,0x10000); a.add(4,4,0); a.load(0,4,-0x7e0c); a.mask(0,0,0x1000,true,0);
    if (a.equal()) {
        a.at(0x10286538); a.load<1>(0,3,0x27); a.load(5,3,0x1a4); a.sign8(0,0); a.load<1>(4,3,0x19);
        a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x10113e10); a.value(6,0x800000);
        a.value(7,0x0); a.offset(6,6,0x0); a.value(5,0x0); a.intersect(6,4,6); a.intersect(4,3,7); a.value(0,0x0);
        a.combine(3,6,5,false,true); a.combine(0,4,0,false,true); a.combine(0,3,0);
        a.offset(3,0,-1); state.wr<1>(1027,a.r(0) != 0); a.subtract_extended(0,3,0); a.sign8(3,0);
    } else {
        a.at(0x10286520); a.load<1>(0,3,0x27); a.load(3,3,0x1a4); a.sign8(0,0); a.add(3,3,0); a.load<1>(3,3,0x6);
        a.jump(0x1028658c);
    }
    a.at(0x1028658c); a.pop(64); a.finish();
}
}
