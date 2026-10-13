#include "ball_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

void orientation_bias(Call& a) {
    a.load(3,15,0x1a4);a.sign16(21,13);a.load<2>(0,15,0x19e,true);a.load<2>(3,3,0x7950,true);
    a.subtract(3,3,0);a.call(0x10775540);a.sign16(13,3);a.compare(13,180);
    if(!a.nonpositive()){a.subtract_from(0,13,360);a.sign16(13,0);}
    a.at(0x1023d1b0);a.sign16(3,13);a.sign8(0,22);a.mask(4,3,0xfffffffe,false,1);a.value(3,1);
    a.subtract_from(4,4,360);a.subtract_from(0,0,5);a.compare(0,1);a.sign16(20,4);
    if(!a.less()){a.sign8(0,22);a.subtract_from(3,0,5);}
    a.at(0x1023d1dc);a.sign16(3,3);a.sign16(0,13);a.divide(3,20,3);a.compare(0,90);a.sign16(20,3);
    if(a.at_least()){a.at(0x1023d224);a.mask(0,20,0xfffffffe,false,1);a.sign16(20,0);}
    else {
        a.value(3,0x66660000);a.sign16(0,13);a.offset(3,3,0x6667);a.multiply_high(0,3,0);a.shift_right(0,0,2);
        a.mask(3,0,1,false,1);a.add(0,0,3);a.subtract_from(0,0,11);a.sign16(0,0);a.product(0,20,0);
        a.sign16(20,0);a.jump(0x1023d22c);
    }
}

// The surface-specific adjustments advance the shared stream before reading its
// sample. Keep each branch's signed remainder and halfword narrowing unchanged.
void surface_adjustment(Call& a,bool extended) {
    a.at(extended?0x1023d2e4:0x1023d278);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);
    a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    if(extended) {
        a.at(0x1023d30c);a.load(4,30,0);a.sign8(3,14);a.load(0,27,0);a.load<2>(4,4,0);a.add(4,4,0);
        a.mask(0,4,0xe0000000,false,29);a.mask(4,4,1,false,1);a.subtract(0,4,0);a.mask(0,0,0xffffffff,false,3);
        a.add(4,0,4);a.offset(0,4,3);a.product(0,3,0);a.sign16(0,0);a.subtract(16,0,16);
    } else {
        a.at(0x1023d2a0);a.load(3,30,0);a.value(4,0x55550000);a.load(0,27,0);a.offset(4,4,0x5556);
        a.load<2>(5,3,0);a.sign8(3,14);a.add(5,5,0);a.multiply_high(4,4,5);a.mask(0,4,1,false,1);
        a.add(0,4,0);a.multiply(0,0,3);a.subtract(4,0,5);a.offset(0,4,1);a.product(0,3,0);a.sign16(0,0);
        a.subtract(16,0,16);a.jump(action_bias_phase);
    }
}

void height_and_surface_bias(Call& a) {
    a.at(0x1023d22c);a.load(3,15,0x1a4);a.load<1>(3,3,0x7920);a.offset(0,3,-1);a.sign8(14,0,true);
    a.product(0,14,14);a.product(0,14,0);a.multiply(0,0,20);a.sign16(0,0);a.subtract(16,0,16);
    if(a.equal())return;
    a.load(3,15,0x1a4);a.load<1>(0,3,1);a.sign8(0,0);a.compare(0,32);
    if(a.equal()){surface_adjustment(a,true);return;}
    if(a.at_least())return;
    a.compare(0,2);
    if(a.equal())surface_adjustment(a,false);else a.jump(action_bias_phase);
}
}

void orient(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,orientation_phase,dependencies,observer);orientation_bias(a);height_and_surface_bias(a);
    state.pc=action_bias_phase;
}
}
