#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

bool attempt_receiver_contact(Call& a) {
    a.load<2>(3,1,0x4a,true);a.sign16(0,17);a.compare_registers(3,0);if(a.at_least())return true;
    detail::advance_action_random(a,0x1023fae8);a.load(4,30,0);a.value(3,0x66660000);a.load(0,27,0);a.offset(3,3,0x6667);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);
    a.add(0,0,3);a.multiply(0,0,5);a.subtract(0,0,4,true);return a.equal();
}

void maybe_warn_receiver(Call& a) {
    a.load(3,15,0x1a4);a.load<1>(0,3,0x7931);a.sign8(0,0,true);if(a.equal())return;
    a.load<1>(0,15,0x27);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0);a.compare(0,1);
    const bool right=a.equal();detail::area_coordinates(a,right?0x1023fb98:0x1023fbe8,0x1023fc30,right,0,3,0,14);
    a.at(0x1023fc30);a.sign8(0,0,true);if(a.equal())return;
    a.load(3,15,0x1a4);a.load<1>(4,1,0x48);a.call(0x101ee530);a.sign8(0,3,true);bool warning=!a.equal();
    if(!warning){a.load<1>(0,1,0x42);a.sign8(0,0,true);warning=!a.equal();}
    if(!warning) {
        a.load<2>(19,14,0x32,true);a.compare(19,0);
        if(a.equal()){a.value(18,0);a.jump(0x1023fdc8);}else detail::sample_action_interval(a,0x1023fc6c,19,18);
        a.at(0x1023fdc8);a.compare(18,-10);if(a.at_least())return;
    }
    detail::advance_action_random(a,0x1023fdd0);a.load(4,30,0);a.value(3,0x55550000);a.load(0,27,0);a.offset(3,3,0x5556);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(3,3,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);
    a.subtract(0,0,4,true);if(a.equal())return;
    a.load(3,15,0x1a4);a.value(4,0x2168);a.load<1>(7,14,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);a.value(9,0xffffffff);
    a.load<1>(6,14,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);a.jump(0x1023fe5c);
}

bool emit_immediate_execution(Call& a) {
    a.at(0x1023fe5c);a.load<1>(0,1,0x40);a.sign8(0,0,true);if(a.equal())return false;
    detail::advance_action_random(a,0x1023fe68);a.load(4,30,0);a.value(3,0x88890000);a.load(0,27,0);a.offset(3,3,-0x7777);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.add(0,0,4);a.shift_right(0,0,4);a.mask(3,0,1,false,1);
    a.add(0,0,3);a.multiply(0,0,30);a.subtract(0,0,4,true);if(!a.equal())return false;
    a.load<1>(0,1,0x45);a.sign8(0,0,true);if(!a.equal())return false;
    a.load<1>(0,1,0x44);a.move(5,14);a.move(6,15);a.store(1,0x38,0);a.load<1>(0,1,0x43);a.store(1,0x3c,0);
    a.load<1>(4,1,0x48);a.load(3,15,0x1a4);a.load<1>(7,1,0x45);a.sign8(4,4);a.load<1>(8,1,0x40);
    a.load<1>(9,15,0x105);a.load<1>(10,15,0x106);a.call(0x101d68f0);a.jump(execution_result_phase);return true;
}
}

void prepare_execution(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,action_execution_phase,dependencies,observer);a.load(5,15,0x1a4);a.offset(3,1,0x45);a.value(0,0);a.move(4,15);
    a.offset(5,5,0x10000);a.move(6,13);a.store(5,-0x2000,15);a.offset(7,1,0x47);a.offset(8,1,0x46);a.offset(9,1,0x4a);
    a.store(1,0x38,3);a.offset(10,1,0x48);a.store(1,0x3c,0);a.load(3,15,0x1a4);a.load<1>(5,15,0x27);a.call(0x101eaf60);
    a.load<1>(7,1,0x48);a.move(14,3);a.load(3,15,0x1a4);a.move(4,15);a.sign8(5,13);a.move(6,14);a.sign8(7,7);a.value(8,0);a.call(0x101f5350);
    a.load(3,15,0x1a4);a.compare(14,0,false);a.value(0,0);a.offset(3,3,0x10000);a.store(3,-0x2000,0);
    if(a.equal()){state.pc=no_receiver_phase;return;}
    if(attempt_receiver_contact(a)) {
        a.at(0x1023fb44);a.load(3,15,0x1a4);a.move(4,14);a.load<1>(6,1,0x48);a.move(5,15);a.offset(8,1,0x44);
        a.offset(9,1,0x43);a.offset(10,1,0x42);a.value(7,0xffffffff);a.call(0x101f5050);a.store<1>(1,0x40,3);maybe_warn_receiver(a);
    } else {a.at(0x1023fe54);a.value(0,0);a.store<1>(1,0x40,0);}
    if(emit_immediate_execution(a)){state.pc=execution_result_phase;return;}
    a.at(0x1023ff10);a.load<1>(0,1,0x40);a.sign8(0,0,true);if(a.equal()){state.pc=execution_result_phase;return;}
    a.load<1>(0,1,0x42);a.move(4,14);a.move(5,15);a.offset(6,1,0x40);a.store(1,0x38,0);a.offset(10,1,0x4c);
    a.load(3,15,0x1a4);a.load<1>(7,1,0x44);a.load<1>(8,1,0x43);a.load<1>(9,1,0x45);a.call(0x101f5110);state.pc=execution_message_phase;
}
}
