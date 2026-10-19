#include "target_acceptance.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::target_acceptance {
using Call = SemanticGuestCall;
namespace {
void eligibility_phase_1(Call&,State&);
void eligibility_phase_2(Call&,State&);
void eligibility_phase_3(Call&,State&);
void eligibility_phase_4(Call&,State&);
void eligibility_phase_4(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101eec8c); a.value(3,0x0);
}
void eligibility_phase_3(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101eec10); a.sign8(0,30); a.compare(0,2);
    if (a.nonpositive()) {
        a.at(0x101eec8c);
        eligibility_phase_4(a,state);
    } else {
        a.at(0x101eec1c); a.sign8(0,30); a.compare(0,6);
        if (a.at_least()) {
            a.at(0x101eec8c);
            eligibility_phase_4(a,state);
        } else {
            a.at(0x101eec28); a.move(3,31); a.call(0x101d5690); a.sign8(3,3); a.sign8(0,30); a.subtract(3,0,3);
            a.call(0x10775540); a.compare(3,1);
            if (a.nonpositive()) {
                a.at(0x101eec8c);
                eligibility_phase_4(a,state);
            } else {
                a.at(0x101eec48); a.move(3,31); a.call(0x101d5680); a.move(26,3); a.move(3,31); a.call(0x101d5690);
                a.move(6,3); a.move(3,27); a.move(4,30); a.move(5,29); a.move(7,26); a.call(0x101ef8d0);
                a.load(3,2,-0x52c4); a.floating_load(0,3,440); a.floating_compare(1,0);
                if (a.nonpositive()) {
                    a.at(0x101eec8c);
                    eligibility_phase_4(a,state);
                } else {
                    a.at(0x101eec84); a.value(3,0x1); a.jump(0x101eec90);
                }
            }
        }
    }
}
void eligibility_phase_2(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101eebf4); a.move(3,31); a.call(0x101d5680); a.sign8(0,3); a.compare(0,2);
    if (a.nonpositive()) {
        a.at(0x101eec10);
        eligibility_phase_3(a,state);
    } else {
        a.at(0x101eec08); a.value(3,0x0); a.jump(0x101eec90);
    }
}
void eligibility_phase_1(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101eeb90); a.move(3,31); a.call(0x101d5680); a.sign8(0,3); a.compare(0,9);
    if (a.at_least()) {
        a.at(0x101eec10);
        eligibility_phase_3(a,state);
    } else {
        a.at(0x101eeba4); a.value(3,0x0); a.jump(0x101eec90);
    }
}
}
void target_position_allowed(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101eeac0,dependencies,observer);
    a.at(0x101eeac0); a.link(); a.multiple(26,false,0); a.save_link(); a.frame(80); a.move(27,3); a.move(31,4);
    a.move(30,5); a.move(29,6); a.move(3,31); a.call(0x10283690); a.move(28,3); a.move(3,27); a.sign8(0,28);
    a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27); a.sign8(4,0); a.move(5,30); a.move(6,29);
    a.call(0x101eea30); a.sign8(0,3); a.compare(0,0);
    if (!a.equal()) {
        a.at(0x101eeb20); a.move(3,31); a.call(0x101d5690); a.sign8(0,3); a.compare(0,4);
        if (!a.equal()) {
            a.at(0x101eeb3c); a.sign8(0,28); a.compare(0,0);
            if (!a.equal()) {
                a.at(0x101eebac); a.move(3,31); a.call(0x101d5690); a.sign8(0,3); a.compare(0,6);
                if (a.at_least()) {
                    a.at(0x101eebf4);
                    eligibility_phase_2(a,state);
                } else {
                    a.at(0x101eebc0); a.move(3,31); a.call(0x101d5690); a.sign8(0,3); a.compare(0,2);
                    if (a.nonpositive()) {
                        a.at(0x101eebf4);
                        eligibility_phase_2(a,state);
                    } else {
                        a.at(0x101eebd4); a.move(3,31); a.call(0x101d5680); a.sign8(3,3); a.sign8(0,29);
                        a.compare_registers(0,3);
                        if (a.nonpositive()) {
                            a.at(0x101eebf4);
                            eligibility_phase_2(a,state);
                        } else {
                            a.at(0x101eebec); a.value(3,0x0); a.jump(0x101eec90);
                        }
                    }
                }
            } else {
                a.at(0x101eeb48); a.move(3,31); a.call(0x101d5690); a.sign8(0,3); a.compare(0,6);
                if (a.at_least()) {
                    a.at(0x101eeb90);
                    eligibility_phase_1(a,state);
                } else {
                    a.at(0x101eeb5c); a.move(3,31); a.call(0x101d5690); a.sign8(0,3); a.compare(0,2);
                    if (a.nonpositive()) {
                        a.at(0x101eeb90);
                        eligibility_phase_1(a,state);
                    } else {
                        a.at(0x101eeb70); a.move(3,31); a.call(0x101d5680); a.sign8(3,3); a.sign8(0,29);
                        a.compare_registers(0,3);
                        if (a.at_least()) {
                            a.at(0x101eeb90);
                            eligibility_phase_1(a,state);
                        } else {
                            a.at(0x101eeb88); a.value(3,0x0); a.jump(0x101eec90);
                        }
                    }
                }
            }
        } else {
            a.at(0x101eeb34); a.value(3,0x0); a.jump(0x101eec90);
        }
    } else {
        a.at(0x101eeb18); a.value(3,0x0); a.jump(0x101eec90);
    }
    a.at(0x101eec90); a.load(0,1,0x58); a.offset(1,1,0x50); a.restore_link(); a.multiple(26,true,0); a.finish();
}
}
