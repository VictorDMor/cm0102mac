#include "actor_policy.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_policy {
using Call = SemanticGuestCall;
namespace {
void context_phase_1(Call&,State&);
void context_phase_1(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101f52e8); a.move(3,30); a.call(0x10283690); a.sign8(0,3); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x101f5318); a.offset(3,31,0x10000); a.load(0,3,-0x1ffc); a.subtract(0,30,0); a.leading_zeroes(0,0);
        a.mask(0,0,0x7ffffff,false,27); a.sign8(3,0);
    } else {
        a.at(0x101f52fc); a.offset(3,31,0x10000); a.load(0,3,-0x1ff8); a.subtract(0,30,0); a.leading_zeroes(0,0);
        a.mask(0,0,0x7ffffff,false,27); a.sign8(3,0); a.jump(0x101f5330);
    }
}
}
void context_control(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101f5280,dependencies,observer);
    a.at(0x101f5280); a.link(); a.store(1,-0x4,31); a.store(1,-0x8,30); a.save_link(); a.frame(64); a.move(31,3);
    a.move(30,4); a.move(3,30); a.call(0x10289560); a.sign16(0,3); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x101f52e8);
        context_phase_1(a,state);
    } else {
        a.at(0x101f52b0); a.move(3,30); a.call(0x10283690); a.sign8(0,3); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x101f52cc); a.move(3,30); a.call(0x10283690); a.sign8(0,3); a.compare(0,0);
            if (!a.equal()) {
                a.at(0x101f52e8);
                context_phase_1(a,state);
            } else {
                a.at(0x101f52e0); a.load<1>(3,31,0x7922); a.jump(0x101f5330);
            }
        } else {
            a.at(0x101f52c4); a.load<1>(3,31,0x7923); a.jump(0x101f5330);
        }
    }
    a.at(0x101f5330); a.load(0,1,0x48); a.offset(1,1,0x40); a.restore_link(); a.load(31,1,-0x4); a.load(30,1,-0x8);
    a.finish();
}
}
