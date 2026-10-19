#include "actor_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_action {
using Call = SemanticGuestCall;
namespace {
void turn_phase_1(Call&,State&);
void turn_phase_2(Call&,State&);
void turn_phase_3(Call&,State&);
void turn_phase_3(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281a68); a.sign16(3,5); a.sign16(0,6); a.compare_registers(3,0);
    if (a.nonpositive()) {
        a.at(0x10281a98); a.sign16(3,5); a.sign16(0,4); a.compare_registers(3,0);
        if (a.at_least()) {
            a.at(0x10281ac8); a.load<1>(3,29,0x2d);
        } else {
            a.at(0x10281aa8); a.load(4,29,0x70); a.value(3,0x1); a.load<1>(0,4,0x3b); a.sign8(0,0);
            a.subtract_from(0,0,0x14); a.multiply(0,0,0xc); a.store<2>(30,0x0,0); a.jump(0x10281acc);
        }
    } else {
        a.at(0x10281a78); a.load(4,29,0x70); a.value(3,0x0); a.load<1>(0,4,0x30); a.sign8(0,0);
        a.subtract_from(0,0,0x14); a.multiply(0,0,0xc); a.store<2>(30,0x0,0); a.jump(0x10281acc);
    }
}
void turn_phase_2(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281a00); a.floating_load(0,31,360,true); a.sign8(0,3); a.product(0,5,0);
    a.floating_load(1,31,60,true); a.floating_product_single(2,0,31); a.floating_load(0,29,232,true);
    a.floating_load(3,31,360,true); a.floating_difference(1,2,1,true,true); a.sign16(5,0); a.move(3,5);
    a.floating_subtract(0,1,0,true); a.floating_subtract(0,3,0,true); a.floating_integer(0,0);
    a.floating_store(0,1,56); a.load(6,1,0x3c); a.sign16(0,6); a.negate(4,6); a.compare_registers(3,0);
    a.sign16(4,4);
    if (a.positive()) {
        a.at(0x10281a68);
        turn_phase_3(a,state);
    } else {
        a.at(0x10281a50); a.sign16(3,5); a.sign16(0,4); a.compare_registers(3,0);
        if (a.less()) {
            a.at(0x10281a68);
            turn_phase_3(a,state);
        } else {
            a.at(0x10281a60); a.load<1>(3,29,0x2d); a.jump(0x10281acc);
        }
    }
}
void turn_phase_1(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102819e0); a.move(3,0); a.sign8(0,3); a.product(0,5,0); a.sign16(5,0); a.compare(5,180);
    if (a.positive()) {
        a.at(0x102819f8); a.subtract_from(0,5,0x168); a.sign16(5,0);
    }
    a.at(0x10281a00);
    turn_phase_2(a,state);
}
}
void choose_turn_side(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x102818f0,dependencies,observer);
    a.at(0x102818f0); a.link(); a.floating_store(31,1,-8); a.floating_move(31,1); a.value(9,0x0);
    a.store(1,-0x14,31); a.load(31,2,-0x51e0); a.store(1,-0x18,30); a.move(30,10); a.store(1,-0x1c,29);
    a.move(29,3); a.value(3,0x2); a.save_link(); a.sign16(0,4); a.compare(0,91); a.frame(96); a.store<2>(10,0x0,9);
    if (!a.equal()) {
        a.at(0x10281934); a.sign16(0,4); a.compare(0,92);
        if (a.equal()) {
        } else {
            a.at(0x10281940); a.sign16(0,4); a.compare(0,61);
            if (a.equal()) {
            } else {
                a.at(0x1028194c); a.sign16(0,4); a.compare(0,54);
                if (a.equal()) {
                } else {
                    a.at(0x10281958); a.sign16(0,4); a.compare(0,52);
                    if (a.equal()) {
                    } else {
                        a.at(0x10281964); a.sign16(0,4); a.compare(0,55);
                        if (a.equal()) {
                        } else {
                            a.at(0x10281970); a.sign16(0,4); a.compare(0,90);
                            if (a.equal()) {
                            } else {
                                a.at(0x1028197c); a.sign16(0,4); a.compare(0,51);
                                if (a.equal()) {
                                } else {
                                    a.at(0x10281988); a.sign16(0,4); a.compare(0,53);
                                    if (a.equal()) {
                                    } else {
                                        a.at(0x10281994); a.sign16(0,4); a.compare(0,57);
                                        if (a.equal()) {
                                        } else {
                                            a.at(0x102819a0); a.sign16(0,4); a.compare(0,3);
                                            if (a.equal()) {
                                            } else {
                                                a.at(0x102819ac); a.load(3,29,0x1a4); a.move(4,5); a.move(5,6);
                                                a.move(6,7); a.move(7,8); a.call(0x101ee610);
                                                a.load<2>(4,29,0x19e,true); a.sign16(3,3); a.value(0,0x1);
                                                a.subtract(3,4,3); a.sign16(5,3,true);
                                                if (a.less()) {
                                                    a.at(0x102819dc); a.value(0,0xffffffff);
                                                }
                                                a.at(0x102819e0);
                                                turn_phase_1(a,state);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    a.at(0x10281acc); a.load(0,1,0x68); a.offset(1,1,0x60); a.floating_load(31,1,-8); a.restore_link();
    a.load(31,1,-0x14); a.load(30,1,-0x18); a.load(29,1,-0x1c); a.finish();
}
}
