#include "ball_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

void wide_facing_sample(Call& a) {
    a.at(0x1023d3cc);a.sign16(3,19);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store(1,0x78,0);
    a.load(8,30,0);a.value(7,0x80010000);a.store(1,0x7c,3);a.sign16(0,19);a.floating_load(2,26,0x80);
    a.value(5,0x43300000);a.floating_load(1,1,0x78);a.value(6,0x10000);a.load<2>(9,8,0);a.value(4,0x80010000);
    a.floating_subtract(1,1,2);a.floating_load(0,26,0);a.load(8,27,0);a.multiply(3,0,10);a.offset(7,7,-0x7fff);
    a.store(1,0x80,5);a.floating_difference(2,1,0,true);a.add(0,9,8);a.multiply_high(5,7,0);a.offset(6,6,-1);
    a.floating_load(1,26,0x80);a.offset(4,4,-0x7fff);a.add(5,5,0);a.shift_right(5,5,15);a.mask(7,5,1,false,1);
    a.add(5,5,7);a.product(5,5,6);a.subtract(25,5,0);a.exclusive_flags(0,25,0x80000000);a.store(1,0x84,0);
    a.multiply_high(0,4,3);a.floating_load(0,1,0x80);a.floating_subtract(0,0,1);a.add(0,0,3);a.shift_right(0,0,15);
    a.floating_product(0,2,0);a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,0x88);
    a.load(25,1,0x8c);a.call(0x10226520);a.value(4,0x66660000);a.offset(0,3,5);a.offset(4,4,0x6667);
    a.sign16(3,19);a.multiply_high(4,4,0);a.offset(0,3,-1);a.shift_right(3,4,2);a.mask(4,3,1,false,1);
    a.add(3,3,4);a.add(25,25,3);a.compare_registers(25,0);
    if(!a.nonpositive()){a.sign16(3,19);a.offset(25,3,-1);a.jump(0x1023d4f4);}
}

void sample_facing(Call& a) {
    a.at(0x1023d364);a.sign16(0,19,true);a.load<1>(13,15,0x10f);a.load<1>(14,15,0x108);
    if(a.equal()){a.value(25,0);a.jump(0x1023d4f4);return;}
    a.at(0x1023d37c);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(0x1023d3a4);a.value(3,0x10000);a.sign16(0,19);a.offset(3,3,-1);a.compare_registers(0,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.sign16(0,19);a.offset(3,3,1);a.compare_registers(0,3);wide=!a.at_least();}
    if(wide)wide_facing_sample(a);
    else {
        a.at(0x1023d4c0);a.load(5,30,0);a.sign16(3,19);a.load(4,27,0);a.sign16(0,19,true);a.load<2>(0,5,0);
        a.add(4,0,4);a.divide(0,4,3);a.product(0,0,3);a.subtract(25,0,4);a.negate(0,25);
        if(!a.nonpositive())a.move(0,25);
        a.at(0x1023d4f0);a.move(25,0);
    }
}

void face_towards_attack(Call& a) {
    a.at(0x1023d52c);a.load<1>(0,15,0x27);a.sign8(0,0,true);
    if(a.equal()) {
        a.load<1>(0,15,0x106);a.move(3,15);a.load<1>(4,15,0x105);a.value(6,4);a.sign8(0,0);
        a.load<1>(5,15,0x106);a.compare(0,9);
        if(a.at_least()){a.at(0x1023d568);a.value(0,11);}
        else {a.load<1>(0,15,0x106);a.sign8(7,0);a.offset(0,7,3);a.jump(0x1023d56c);}
        a.at(0x1023d56c);a.sign8(7,0);a.call(0x10237830);a.jump(area_selection_phase);
    } else {
        a.at(0x1023d578);a.load<1>(0,15,0x106);a.move(3,15);a.load<1>(4,15,0x105);a.value(6,4);a.sign8(0,0);
        a.load<1>(5,15,0x106);a.compare(0,3);
        if(a.nonpositive()){a.at(0x1023d5a8);a.value(0,0);}
        else {a.load<1>(0,15,0x106);a.sign8(7,0);a.offset(0,7,-3);a.jump(0x1023d5ac);}
        a.at(0x1023d5ac);a.sign8(7,0);a.call(0x10237830);a.jump(area_selection_phase);
    }
}

void select_facing(Call& a) {
    a.at(0x1023d4f4);a.sign8(3,13);a.sign8(0,14);a.product(0,3,0);a.compare_registers(0,25);
    bool attack=a.positive();
    if(!attack){a.floating_load(0,26,0xc8);a.floating_compare(29,0);attack=a.less();}
    if(!attack){a.load(3,15,0x1a4);a.move(4,15);a.value(5,0);a.call(0x101d8d70);a.sign8(0,3,true);attack=!a.equal();}
    if(attack){face_towards_attack(a);return;}
    a.at(0x1023d5b8);a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,4);
    if(a.at_least())return;
    a.load<1>(4,15,0x105);a.move(3,15);a.load<1>(5,15,0x106);a.move(6,17);a.move(7,18);a.call(0x10237830);
}
}

void face_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,action_bias_phase,dependencies,observer);a.load(3,15,0x1a4);a.sign16(0,13);a.compare(0,90);
    a.value(19,200);a.offset(3,3,0x10000);a.load(24,3,-0x2008);
    if(a.nonpositive()){sample_facing(a);select_facing(a);}else a.jump(area_selection_phase);
    state.pc=area_selection_phase;
}
}
