#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

void classify_contact_message(Call& a) {
    a.load<1>(0,1,0x41);a.value(16,0xffffffff);a.sign8(0,0);a.compare(0,4);
    if(a.equal()){a.at(0x1023ed88);a.value(16,0x1ff8);a.jump(0x1023ee04);return;}
    if(a.at_least()) {
        a.at(0x1023ed64);a.compare(0,6);
        if(a.equal()){a.at(0x1023ed70);a.value(16,0x1ff5);a.jump(0x1023ee04);return;}
        a.jump(0x1023ed90);
    } else {
        a.compare(0,2);
        if(a.equal()){a.at(0x1023ed80);a.value(16,0x1ff9);a.jump(0x1023ee04);return;}
        if(a.at_least()){a.at(0x1023ed78);a.value(16,0x1ff7);a.jump(0x1023ee04);return;}
        a.jump(0x1023ed90);
    }
    detail::advance_action_random(a,0x1023ed90);a.load(3,30,0);a.value(4,0x92490000);a.load(5,27,0);a.offset(4,4,0x2493);
    a.load<2>(6,3,0);a.load<1>(0,1,0x43);a.add(5,6,5);a.sign8(3,0);a.multiply_high(0,4,5);a.add(0,0,5);
    a.shift_right(0,0,2);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,7);a.subtract(4,0,5);a.offset(0,4,2);
    a.compare_registers(3,0);if(!a.nonpositive())a.value(16,0x210a);
}

void emit_area_comment(Call& a,std::uint32_t start,unsigned message,bool jump) {
    a.at(start);a.load<1>(0,15,0x27);a.value(4,message);a.load(3,15,0x1a4);a.value(6,0xffffffff);a.sign8(0,0);
    a.load(5,15,0x1a4);a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);a.value(8,0xffffffff);
    a.sign8(7,0);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
    if(jump)a.jump(0x1023efb4);
}

void maybe_emit_area_comment(Call& a) {
    a.sign16(0,16);a.compare(0,-1);if(!a.equal())return;
    detail::advance_action_random(a,0x1023eedc);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
    a.mask(0,3,0xc0000000,false,30);a.mask(3,3,1,false,1);a.subtract(0,3,0);a.mask(0,0,0xffffffff,false,2);
    a.add(0,0,3);a.compare(0,1);
    if(a.equal()){emit_area_comment(a,0x1023ef7c,0x20ec,false);return;}
    if(a.at_least())return;
    a.compare(0,0);
    if(a.at_least())emit_area_comment(a,0x1023ef40,0x20e9,true);else a.jump(0x1023efb4);
}

void apply_area_reaction(Call& a) {
    a.at(0x1023efb4);a.load(4,15,0x1a4);a.value(3,5);a.value(0,0);a.value(17,0x1f7a);a.store<1>(4,0x7928,3);
    a.load(3,15,0x1a4);a.store(3,0x3110,0);a.load<2>(3,15,0x36,true);a.offset(0,3,-500);a.store<2>(15,0x36,0);
    a.load(3,15,0x6c);a.load<1>(18,3,0x66);a.sign8(18,18,true);
    if(a.equal()){a.value(14,0);a.jump(0x1023f150);}else detail::sample_action_interval(a,0x1023eff4,18,14);
    a.at(0x1023f150);a.compare(14,0);if(!a.equal())return;
    detail::advance_action_random(a,0x1023f158);a.load(3,30,0);a.value(4,0x66660000);a.load(0,27,0);a.offset(4,4,0x6667);
    a.load<2>(5,3,0);a.move(3,15);a.add(5,5,0);a.multiply_high(0,4,5);a.shift_right(0,0,2);a.mask(4,0,1,false,1);
    a.add(0,0,4);a.multiply(0,0,10);a.subtract(0,0,5);a.negate(0,0);a.sign8(4,0);a.call(0x10288f30);a.sign8(0,3,true);
    if(a.equal())return;
    a.move(3,15);a.value(4,0);a.call(0x1022ba10);a.jump(contact_outcome_phase);
}
}

void resolve_contact(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,contact_resolution_phase,dependencies,observer);a.load(3,15,0x1a4);a.move(4,15);a.load<1>(6,1,0x44);
    a.offset(5,1,0x41);a.load<1>(7,1,0x43);a.offset(9,1,0x4c);a.value(8,0);a.call(0x101f5180);
    a.load(3,15,0x1a4);a.value(4,0x1fe8);a.load<1>(7,15,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);
    a.value(9,0xffffffff);a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);
    classify_contact_message(a);a.at(0x1023ee04);a.load<1>(0,1,0x41);a.compare(0,7);
    if(a.equal()) {
        a.load(3,15,0x1a4);a.value(0,7);a.value(17,0x1f79);a.store<1>(3,0x7928,0);a.jump(contact_outcome_phase);
    } else {
        a.at(0x1023ee24);a.load<1>(0,15,0x27);a.compare(0,1);const auto right=a.equal();
        detail::area_coordinates(a,right?0x1023ee30:0x1023ee80,0x1023eec8,right,0,3,0);
        a.at(0x1023eec8);a.sign8(0,0,true);
        if(a.equal()){a.at(0x1023f1d8);a.load(3,15,0x1a4);a.value(0,6);a.value(17,0x1f79);a.store<1>(3,0x7928,0);}
        else {maybe_emit_area_comment(a);apply_area_reaction(a);}
    }
    state.pc=contact_outcome_phase;
}
}
