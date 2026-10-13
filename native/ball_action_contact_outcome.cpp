#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

void emit_opposing_side(Call& a,std::uint32_t start) {
    a.at(start);a.load<1>(0,15,0x27);a.move(4,17);a.load(3,15,0x1a4);a.value(6,0xffffffff);a.sign8(0,0);
    a.load(5,15,0x1a4);a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);
    a.value(8,0xffffffff);a.sign8(7,0);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
}

void emit_recipient(Call& a,std::uint32_t start,unsigned message) {
    a.at(start);a.load<1>(0,15,0x27);a.value(4,message);a.load(3,15,0x1a4);a.value(8,0xffffffff);a.sign8(0,0);
    a.load<1>(6,14,0x2f);a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);
    a.load(5,15,0x1a4);a.sign8(7,0);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
}

void emit_actor(Call& a,std::uint32_t start,bool selected) {
    a.at(start);a.load(3,15,0x1a4);if(selected)a.move(4,16);else a.value(4,0x2109);
    a.load<1>(7,15,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);a.value(9,0xffffffff);
    a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);
}

void emit_side(Call& a,std::uint32_t start,unsigned message) {
    a.at(start);a.load(3,15,0x1a4);a.value(4,message);a.load<1>(7,15,0x27);a.value(6,0xffffffff);
    a.load(3,3,0x79f4);a.value(8,0xffffffff);a.load(5,15,0x1a4);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
}

void load_opposing_team(Call& a,std::uint32_t start) {
    a.at(start);a.load<1>(0,15,0x27);a.load(3,15,0x1a4);a.sign8(0,0);a.leading_zeroes(0,0);
    a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0,true);
    if(a.equal()){a.at(start+0x28);a.load(3,3,0x3118);a.load(3,3,0x1c);}
    else {a.load(3,3,0x3118);a.load(3,3,0x20);a.jump(start+0x30);}
}

void maybe_emit_opposition_reaction(Call& a) {
    load_opposing_team(a,0x1023f498);a.load(0,3,0x2c);a.compare(0,0,false);if(a.equal())return;
    // The original intentionally reloads the side and both ownership links.
    load_opposing_team(a,0x1023f4d4);a.load(3,3,0x2c);a.load<1>(0,3,0x119);a.compare(0,8);if(!a.equal())return;
    a.load(3,15,0x1a4);a.value(4,0x20f5);a.load(5,15,0x1a4);a.value(6,0);a.load(3,3,0x79f4);
    a.value(7,0);a.value(8,0xffffffff);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
}

void emit_contact_detail(Call& a) {
    a.at(0x1023f26c);a.sign16(0,16);a.compare(0,-1);if(a.nonpositive())return;
    detail::advance_action_random(a,0x1023f278);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
    a.mask(0,3,0xc0000000,false,30);a.mask(3,3,1,false,1);a.subtract(0,3,0);a.mask(0,0,0xffffffff,false,2);a.add_record(0,0,3);
    if(a.equal()) {
        a.load<1>(18,1,0x43);a.sign8(18,18,true);
        if(a.equal()){a.value(14,0);a.jump(0x1023f438);}else detail::sample_action_interval(a,0x1023f2dc,18,14);
        detail::advance_action_random(a,0x1023f438);a.load(4,30,0);a.value(3,0x66660000);a.load(0,27,0);a.offset(3,3,0x6667);
        a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,3);a.mask(3,0,1,false,1);
        a.add(0,0,3);a.multiply(0,0,20);a.subtract(0,0,4);a.compare_registers(14,0);
        if(!a.nonpositive())maybe_emit_opposition_reaction(a);
        emit_actor(a,0x1023f53c,false);
    }
    emit_actor(a,0x1023f564,true);a.load<1>(0,1,0x41);a.compare(0,6);bool response=a.equal();
    if(!response){a.load<1>(3,1,0x41);a.offset(0,3,-3);a.mask(0,0,0xff);a.compare(0,1,false);response=a.nonpositive();}
    if(!response){a.load<1>(0,1,0x41);a.compare(0,5);response=a.equal();}
    if(response){a.at(0x1023f5b8);a.load(3,15,0x1a4);a.move(4,15);a.value(5,0);a.call(0x101f1530);}
}

void emit_recipient_followup(Call& a) {
    a.at(0x1023f6c0);a.sign16(0,17);a.compare(0,0x1f79);if(!a.equal())return;
    a.load<1>(0,1,0x42);a.sign8(0,0,true);if(!a.equal())return;
    a.load<1>(0,1,0x41);a.compare(0,7);
    if(a.equal()) {
        a.move(3,14);a.call(0x10272150);a.sign8(0,3,true);if(a.equal())return;
        a.load<1>(0,15,0x27);a.compare(0,1);const auto right=a.equal();
        detail::area_coordinates(a,right?0x1023f700:0x1023f750,0x1023f798,right,0,3,0,14);
        a.at(0x1023f798);a.sign8(0,0,true);if(!a.equal())return;
        emit_recipient(a,0x1023f7a0,0x21e1);a.jump(0x1023f884);return;
    }
    a.at(0x1023f7dc);a.load<1>(4,15,0x105);a.move(3,15);a.call(0x102720a0);a.sign8(0,3,true);
    if(!a.equal()) {
        a.move(3,14);a.call(0x10272150);a.sign8(0,3,true);
        if(!a.equal()){emit_recipient(a,0x1023f800,0x21df);a.jump(0x1023f884);return;}
    }
    a.at(0x1023f83c);a.move(3,14);a.call(0x10272150);a.sign8(0,3,true);if(a.equal())return;
    emit_recipient(a,0x1023f84c,0x21e0);
}

void select_contact_recipient(Call& a) {
    a.at(0x1023f5c8);a.sign16(0,17);a.compare(0,0x1f7a);if(a.equal())return;
    a.load<1>(0,15,0x27);a.value(6,0);a.load(3,15,0x1a4);a.sign8(0,0);a.leading_zeroes(0,0);
    a.load<1>(5,3,0x7928);a.mask(0,0,0x07ffffff,false,27);a.load(3,15,0x1a4);a.sign8(4,0);a.call(0x101f3a30);a.move(14,3,true);
    if(a.equal()){emit_opposing_side(a,0x1023f604);a.jump(0x1023f884);return;}
    a.at(0x1023f640);a.load<1>(0,1,0x41);a.compare(0,7);
    if(a.equal()){emit_recipient(a,0x1023f64c,0x21e2);a.jump(0x1023f6c0);}
    else emit_opposing_side(a,0x1023f688);
    emit_recipient_followup(a);
}

void maybe_emit_contact_end(Call& a) {
    detail::advance_action_random(a,0x1023f898);a.load(4,30,0);a.value(3,0x38e40000);a.load(0,27,0);a.offset(3,3,-0x71c7);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);
    a.add(0,0,3);a.multiply(0,0,9);a.subtract(0,0,4,true);if(!a.equal())return;
    emit_side(a,0x1023f8f4,0x20df);
    detail::advance_action_random(a,0x1023f91c);a.load(4,30,0);a.value(3,0x55550000);a.load(0,27,0);a.offset(3,3,0x5556);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(3,3,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);a.subtract(0,0,4,true);
    if(a.equal())emit_side(a,0x1023f974,0x20e0);
    a.at(0x1023f99c);a.load<1>(3,1,0x41);a.offset(0,3,-3);a.mask(0,0,0xff);a.compare(0,3,false);bool eligible=a.nonpositive();
    if(!eligible){a.sign16(0,17);a.compare(0,0x1f7a);eligible=a.equal();}
    if(!eligible)return;
    detail::advance_action_random(a,0x1023f9bc);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(0,3,0);
    a.mask(3,0,1,false,1);a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0,true);
    if(a.equal())emit_side(a,0x1023fa08,0x20d9);
}
}

void complete_contact(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,contact_outcome_phase,dependencies,observer);a.load<1>(4,15,0x105);a.sign16(0,17);a.load(3,15,0x1a4);a.compare(0,0x1f7a);
    a.value(0,0);a.store<1>(3,0x791d,4);a.load<1>(4,15,0x106);a.load(3,15,0x1a4);a.store<1>(3,0x791e,4);
    a.load<1>(3,15,0x27);a.load(4,15,0x1a4);a.sign8(3,3);a.leading_zeroes(3,3);a.mask(3,3,0x07ffffff,false,27);a.store<1>(4,0x7924,3);
    a.load(3,15,0x1a4);a.offset(3,3,0x10000);a.store(3,-0x2000,0);
    if(a.equal())emit_opposing_side(a,0x1023f234);
    emit_contact_detail(a);select_contact_recipient(a);
    a.at(0x1023f884);a.load(3,15,0x1a4);a.move(4,13);a.call(0x101ee530);a.sign8(0,3,true);
    if(a.equal()) {
        maybe_emit_contact_end(a);a.at(0x1023fa30);a.load(3,15,0x1a4);a.move(4,15);a.floating_load(1,1,0x4c,true);a.call(0x101d6780);
        a.move(3,15);a.value(4,0);a.call(0x102884d0);
    }
    a.at(0x1023fa4c);a.load<2>(3,15,0x36,true);a.offset(0,3,-25);a.store<2>(15,0x36,0);a.jump(return_phase);state.pc=return_phase;
}
}
