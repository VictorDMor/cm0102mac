#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

void classify_receiver_contact(Call& a) {
    a.load<1>(0,1,0x40);a.value(16,0xffffffff);a.sign8(0,0);a.compare(0,4);
    if(a.equal()){a.at(0x102401c0);a.value(16,0x1ff8);a.jump(0x1024023c);return;}
    if(a.at_least()) {
        a.at(0x102401a0);a.compare(0,6);
        if(!a.at_least()){a.value(16,0x1ff6);a.jump(0x1024023c);return;}
    } else {
        a.compare(0,2);
        if(a.equal()){a.at(0x102401b8);a.value(16,0x1ff9);a.jump(0x1024023c);return;}
        if(a.at_least()){a.at(0x102401b0);a.value(16,0x1ff7);a.jump(0x1024023c);return;}
        a.jump(0x102401c8);
    }
    detail::advance_action_random(a,0x102401c8);a.load(3,30,0);a.value(4,0x92490000);a.load(5,27,0);a.offset(4,4,0x2493);
    a.load<2>(6,3,0);a.load<1>(0,1,0x43);a.add(5,6,5);a.sign8(3,0);a.multiply_high(0,4,5);a.add(0,0,5);
    a.shift_right(0,0,2);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,7);a.subtract(4,0,5);a.offset(0,4,2);
    a.compare_registers(3,0);if(!a.nonpositive())a.value(16,0x210a);
}

void emit_receiver_area_comment(Call& a,std::uint32_t start,unsigned message,bool jump) {
    a.at(start);a.load(3,15,0x1a4);a.value(4,message);a.load<1>(7,15,0x27);a.value(6,0xffffffff);a.load(3,3,0x79f4);
    a.value(8,0xffffffff);a.load(5,15,0x1a4);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
    if(jump)a.jump(0x102403dc);
}

void maybe_emit_receiver_area_comment(Call& a) {
    a.sign16(0,16);a.compare(0,-1);if(!a.equal())return;
    detail::advance_action_random(a,0x10240324);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
    a.mask(0,3,0xc0000000,false,30);a.mask(3,3,1,false,1);a.subtract(0,3,0);a.mask(0,0,0xffffffff,false,2);
    a.add(0,0,3);a.compare(0,1);
    if(a.equal()){emit_receiver_area_comment(a,0x102403b4,0x20ec,false);return;}
    if(a.at_least())return;
    a.compare(0,0);
    if(a.at_least())emit_receiver_area_comment(a,0x10240388,0x20e9,true);else a.jump(0x102403dc);
}

void apply_receiver_area_reaction(Call& a) {
    a.at(0x102403dc);a.load(3,15,0x1a4);a.value(0,5);a.value(17,0x1f7a);a.store<1>(3,0x7928,0);
    a.load(3,15,0x1a4);a.store(3,0x3110,15);a.load<2>(3,14,0x36,true);a.offset(0,3,-500);a.store<2>(14,0x36,0);
    a.load(3,14,0x6c);a.load<1>(18,3,0x66);a.sign8(18,18,true);
    if(a.equal()){a.value(13,0);a.jump(0x10240574);}else detail::sample_action_interval(a,0x10240418,18,13);
    a.at(0x10240574);a.compare(13,0);if(!a.equal())return;
    detail::advance_action_random(a,0x1024057c);a.load(3,30,0);a.value(4,0x66660000);a.load(0,27,0);a.offset(4,4,0x6667);
    a.load<2>(5,3,0);a.move(3,14);a.add(5,5,0);a.multiply_high(0,4,5);a.shift_right(0,0,2);a.mask(4,0,1,false,1);
    a.add(0,0,4);a.multiply(0,0,10);a.subtract(0,0,5);a.negate(0,0);a.sign8(4,0);a.call(0x10288f30);a.sign8(0,3,true);
    if(a.equal())return;
    a.move(3,14);a.value(4,0);a.call(0x1022ba10);a.jump(receiver_outcome_phase);
}
}

void resolve_receiver(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,receiver_resolution_phase,dependencies,observer);classify_receiver_contact(a);a.at(0x1024023c);a.load<1>(0,1,0x40);a.compare(0,7);
    if(a.equal()) {
        a.load(3,15,0x1a4);a.value(0,7);a.value(17,0x1f79);a.store<1>(3,0x7928,0);a.jump(receiver_outcome_phase);
    } else {
        a.at(0x1024025c);a.load<1>(0,15,0x27);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0);a.compare(0,1);
        const bool right=a.equal();detail::area_coordinates(a,right?0x10240278:0x102402c8,0x10240310,right,0,3,0);
        a.at(0x10240310);a.sign8(0,0,true);
        if(a.equal()){a.at(0x102405fc);a.load(3,15,0x1a4);a.value(0,6);a.value(17,0x1f79);a.store<1>(3,0x7928,0);}
        else {maybe_emit_receiver_area_comment(a);apply_receiver_area_reaction(a);}
    }
    state.pc=receiver_outcome_phase;
}
}
