#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

void maybe_warn_execution_contact(Call& a) {
    a.at(0x10241098);a.load(3,15,0x1a4);a.load<1>(0,3,0x7931);a.sign8(0,0,true);if(a.equal())return;
    a.load<1>(0,15,0x27);a.compare(0,1);const bool right=a.equal();
    detail::area_coordinates(a,right?0x102410b4:0x10241104,0x1024114c,right,0,3,0);
    a.at(0x1024114c);a.sign8(0,0,true);if(a.equal())return;
    a.load(3,15,0x1a4);a.move(4,13);a.call(0x101ee530);a.sign8(0,3,true);bool warn=!a.equal();
    if(!warn){a.load<1>(0,1,0x42);a.sign8(0,0,true);warn=!a.equal();}
    if(!warn) {
        a.load<2>(19,14,0x32,true);a.compare(19,0);
        if(a.equal()){a.value(18,0);a.jump(0x102412e4);}else detail::sample_action_interval(a,0x10241188,19,18);
        a.at(0x102412e4);a.compare(18,-10);if(a.at_least())return;
    }
    detail::advance_action_random(a,0x102412ec);a.load(4,30,0);a.value(3,0x55550000);a.load(0,27,0);a.offset(3,3,0x5556);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(3,3,4);a.mask(0,3,1,false,1);a.add(0,3,0);
    a.multiply(0,0,3);a.subtract(0,0,4,true);if(a.equal())return;
    a.load(3,15,0x1a4);a.value(4,0x2168);a.load<1>(7,15,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);
    a.value(9,0xffffffff);a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);
}

void attempt_execution_contact(Call& a) {
    a.load<2>(0,1,0x4a,true);a.sign16(3,17);a.compare_registers(3,0);bool attempt=a.positive();
    if(!attempt) {
        detail::advance_action_random(a,0x10241014);a.load(4,30,0);a.value(3,0x66660000);a.load(0,27,0);a.offset(3,3,0x6667);
        a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);
        a.add(0,0,3);a.multiply(0,0,5);a.subtract(0,0,4,true);attempt=a.equal();
    }
    if(!attempt)return;
    a.at(0x10241070);a.load(3,15,0x1a4);a.move(4,15);a.move(5,14);a.move(6,13);a.offset(8,1,0x44);
    a.offset(9,1,0x43);a.offset(10,1,0x42);a.value(7,0xffffffff);a.call(0x101f5050);a.store<1>(1,0x40,3);
    maybe_warn_execution_contact(a);
}

void maybe_cancel_execution_contact(Call& a) {
    a.at(0x1024136c);a.load<1>(0,1,0x40);a.sign8(0,0,true);if(a.equal())return;
    detail::advance_action_random(a,0x10241378);a.load(4,30,0);a.value(3,0x66660000);a.load(0,27,0);a.offset(3,3,0x6667);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,2);a.mask(3,0,1,false,1);
    a.add(0,0,3);a.multiply(0,0,10);a.subtract(0,0,4,true);if(!a.equal())return;
    a.load<1>(0,1,0x44);a.move(5,15);a.move(6,14);a.sign8(4,13);a.store(1,0x38,0);a.value(7,0);
    a.load<1>(0,1,0x43);a.store(1,0x3c,0);a.load(3,15,0x1a4);a.load<1>(8,1,0x40);
    a.load<1>(9,15,0x105);a.load<1>(10,15,0x106);a.call(0x101d68f0);a.value(0,0);a.store<1>(1,0x40,0);a.jump(0x1024141c);
}
}

void prepare_execution_result(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,execution_result_phase,dependencies,observer);a.load<1>(0,1,0x40);a.sign8(0,0,true);
    if(!a.equal()){a.at(0x10241414);a.value(0,0);a.store<1>(1,0x40,0);}
    else {attempt_execution_contact(a);maybe_cancel_execution_contact(a);}
    a.at(0x1024141c);a.load<1>(0,1,0x40);a.sign8(0,0,true);
    if(a.equal()){state.pc=unopposed_result_phase;return;}
    a.load<1>(0,1,0x42);a.move(4,15);a.move(5,14);a.offset(6,1,0x40);a.store(1,0x38,0);a.offset(10,1,0x4c);a.value(9,0);
    a.load(3,15,0x1a4);a.load<1>(7,1,0x44);a.load<1>(8,1,0x43);a.call(0x101f5110);state.pc=result_message_phase;
}
}
