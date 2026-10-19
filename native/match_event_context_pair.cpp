#include "match_event.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_event {
using Call = SemanticGuestCall;
namespace {
void context_pair_rule_1(Call&);
void context_pair_rule_2(Call&);
void context_pair_rule_3(Call&);
void context_pair_rule_3(Call& a) {
    a.at(0x10201178); a.value(3,0x0); a.finish();
}
void context_pair_rule_2(Call& a) {
    a.at(0x10201170); a.value(3,0x1); a.finish();
}
void context_pair_rule_1(Call& a) {
    a.at(0x10201128); a.sign8(3,7); a.sign8(0,6); a.compare_registers(3,0);
    if (!a.equal()) {
        a.at(0x10201178);
        context_pair_rule_3(a);
    } else {
        a.at(0x10201138); a.load<1>(3,4,0x6); a.load<1>(0,5,0x6); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x10201178);
            context_pair_rule_3(a);
        } else {
            a.at(0x10201150); a.load<1>(0,4,0x7); a.sign8(0,0); a.compare(0,-1);
            if (a.equal()) {
                a.at(0x10201170);
                context_pair_rule_2(a);
            } else {
                a.at(0x10201160); a.load<1>(0,5,0x7); a.sign8(0,0); a.compare(0,-1);
                if (!a.equal()) {
                    a.at(0x10201178);
                    context_pair_rule_3(a);
                } else {
                    a.at(0x10201170);
                    context_pair_rule_2(a);
                }
            }
        }
    }
}
}
void compatible_actor_context(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x102010c0,dependencies,observer);
    a.at(0x102010c0); a.load<1>(7,4,0x5); a.load<1>(6,5,0x5); a.sign8(3,7); a.sign8(0,6); a.compare_registers(3,0);
    if (!a.equal()) {
        a.at(0x10201128);
        context_pair_rule_1(a);
    } else {
        a.at(0x102010d8); a.load<1>(3,4,0x6); a.load<1>(0,5,0x6); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x10201128);
            context_pair_rule_1(a);
        } else {
            a.at(0x102010f0); a.load<1>(3,4,0x7); a.load<1>(0,5,0x7); a.sign8(3,3); a.sign8(0,0);
            a.compare_registers(3,0);
            if (!a.equal()) {
                a.at(0x10201128);
                context_pair_rule_1(a);
            } else {
                a.at(0x10201108); a.load<1>(3,4,0x8); a.load<1>(0,5,0x8); a.sign8(3,3); a.sign8(0,0);
                a.compare_registers(3,0);
                if (!a.equal()) {
                    a.at(0x10201128);
                    context_pair_rule_1(a);
                } else {
                    a.at(0x10201120); a.value(3,0x1); a.finish();
                }
            }
        }
    }
}
}
