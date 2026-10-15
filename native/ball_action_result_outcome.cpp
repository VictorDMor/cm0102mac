#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call=SemanticGuestCall;

void emit_restart_side(Call& a,std::uint32_t start) {
    a.at(start);a.load<1>(0,15,0x27);a.move(4,17);a.load(3,15,0x1a4);a.value(6,0xffffffff);a.sign8(0,0);
    a.load(5,15,0x1a4);a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);
    a.value(8,0xffffffff);a.sign8(7,0);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
}

void emit_restart_player(Call& a,std::uint32_t start,unsigned message) {
    a.at(start);a.load<1>(0,15,0x27);a.value(4,message);a.load(3,15,0x1a4);a.value(8,0xffffffff);a.sign8(0,0);
    a.load<1>(6,16,0x2f);a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);
    a.load(5,15,0x1a4);a.sign8(7,0);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
}

void emit_actor_contact(Call& a,std::uint32_t start,bool selected) {
    a.at(start);a.load(3,15,0x1a4);if(selected)a.move(4,16);else a.value(4,0x2109);
    a.load<1>(7,15,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);a.value(9,0xffffffff);
    a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);
}

void load_actor_opposition(Call& a,std::uint32_t start) {
    a.at(start);a.load<1>(0,15,0x27);a.load(3,15,0x1a4);a.sign8(0,0);a.leading_zeroes(0,0);
    a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0,true);
    if(a.equal()){a.at(start+0x28);a.load(3,3,0x3118);a.load(3,3,0x1c);}
    else {a.load(3,3,0x3118);a.load(3,3,0x20);a.jump(start+0x30);}
}

void maybe_emit_actor_opposition(Call& a) {
    load_actor_opposition(a,0x10241dd4);a.load(0,3,0x2c);a.compare(0,0,false);if(a.equal())return;
    load_actor_opposition(a,0x10241e10);a.load(3,3,0x2c);a.load<1>(0,3,0x119);a.compare(0,8);if(!a.equal())return;
    a.load(3,15,0x1a4);a.value(4,0x20f5);a.load(5,15,0x1a4);a.value(6,0);a.load(3,3,0x79f4);
    a.value(7,0);a.value(8,0xffffffff);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
}

void describe_actor_contact(Call& a) {
    a.at(0x10241ba8);a.sign16(0,16);a.compare(0,-1);if(a.nonpositive())return;
    detail::advance_action_random(a,0x10241bb4);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
    a.mask(0,3,0xc0000000,false,30);a.mask(3,3,1,false,1);a.subtract(0,3,0);a.mask(0,0,0xffffffff,false,2);a.add_record(0,0,3);
    if(a.equal()) {
        a.load<1>(19,1,0x43);a.sign8(19,19,true);
        if(a.equal()){a.value(18,0);a.jump(0x10241d74);}else detail::sample_action_interval(a,0x10241c18,19,18);
        detail::advance_action_random(a,0x10241d74);a.load(4,30,0);a.value(3,0x66660000);a.load(0,27,0);a.offset(3,3,0x6667);
        a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,3);a.mask(3,0,1,false,1);
        a.add(0,0,3);a.multiply(0,0,20);a.subtract(0,0,4);a.compare_registers(18,0);
        if(!a.nonpositive())maybe_emit_actor_opposition(a);
        emit_actor_contact(a,0x10241e78,false);
    }
    emit_actor_contact(a,0x10241ea0,true);a.load<1>(0,1,0x40);a.compare(0,6);bool response=a.equal();
    if(!response){a.load<1>(3,1,0x40);a.offset(0,3,-3);a.mask(0,0,0xff);a.compare(0,1,false);response=a.nonpositive();}
    if(!response){a.load<1>(0,1,0x40);a.compare(0,5);response=a.equal();}
    if(response){a.at(0x10241ef4);a.load(3,15,0x1a4);a.move(4,15);a.value(5,0);a.call(0x101f1530);}
}

void describe_restart_followup(Call& a) {
    a.at(0x10241ffc);a.sign16(0,17);a.compare(0,0x1f79);if(!a.equal())return;
    a.load<1>(0,1,0x42);a.sign8(0,0,true);if(!a.equal())return;
    a.load<1>(0,1,0x40);a.compare(0,7);
    if(a.equal()) {
        a.move(3,16);a.call(0x10272150);a.sign8(0,3,true);if(a.equal())return;
        a.load<1>(0,15,0x27);a.compare(0,1);const bool right=a.equal();
        detail::area_coordinates(a,right?0x1024203c:0x1024208c,0x102420d4,right,0,3,0,16);
        a.at(0x102420d4);a.sign8(0,0,true);if(!a.equal())return;
        emit_restart_player(a,0x102420dc,0x21e1);a.jump(0x102421c0);return;
    }
    a.at(0x10242118);a.load<1>(4,15,0x105);a.move(3,15);a.call(0x102720a0);a.sign8(0,3,true);
    if(!a.equal()) {
        a.move(3,16);a.call(0x10272150);a.sign8(0,3,true);
        if(!a.equal()){emit_restart_player(a,0x1024213c,0x21df);a.jump(0x102421c0);return;}
    }
    a.at(0x10242178);a.move(3,16);a.call(0x10272150);a.sign8(0,3,true);if(a.equal())return;
    emit_restart_player(a,0x10242188,0x21e0);
}

void choose_restart_player(Call& a) {
    a.at(0x10241f04);a.sign16(0,17);a.compare(0,0x1f7a);if(a.equal())return;
    a.load<1>(0,15,0x27);a.value(6,0);a.load(3,15,0x1a4);a.sign8(0,0);a.leading_zeroes(0,0);a.load<1>(5,3,0x7928);
    a.mask(0,0,0x07ffffff,false,27);a.load(3,15,0x1a4);a.sign8(4,0);a.call(0x101f3a30);a.move(16,3,true);
    if(a.equal()){emit_restart_side(a,0x10241f40);a.jump(0x102421c0);return;}
    a.at(0x10241f7c);a.load<1>(0,1,0x40);a.compare(0,7);
    if(a.equal()){emit_restart_player(a,0x10241f88,0x21e2);a.jump(0x10241ffc);}else emit_restart_side(a,0x10241fc4);
    describe_restart_followup(a);
}

bool major_contact(Call& a,std::uint32_t start) {
    a.at(start);a.load<1>(3,1,0x40);a.offset(0,3,-3);a.mask(0,0,0xff);a.compare(0,3,false);if(a.nonpositive())return true;
    a.sign16(0,17);a.compare(0,0x1f7a);return a.equal();
}

void emit_actor_side(Call& a,std::uint32_t start,unsigned message) {
    a.at(start);a.load(3,15,0x1a4);a.value(4,message);a.load<1>(7,15,0x27);a.value(6,0xffffffff);a.load(3,3,0x79f4);
    a.value(8,0xffffffff);a.load(5,15,0x1a4);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
}

void describe_execution_end(Call& a) {
    a.load<1>(0,1,0x42);a.sign8(0,0,true);
    if(!a.equal()) {
        a.load<1>(0,15,0x27);a.value(4,0x20dd);a.load(3,15,0x1a4);a.value(8,0xffffffff);a.sign8(0,0);a.load<1>(6,14,0x2f);
        a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);a.load(5,15,0x1a4);a.sign8(7,0);
        a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
        a.load<1>(0,1,0x40);a.compare(0,6);bool mark=a.equal();
        if(!mark){a.load<1>(0,1,0x40);a.compare(0,4);mark=a.equal();}
        if(!mark){a.load<1>(0,1,0x40);a.compare(0,5);mark=a.equal();}
        if(mark){a.at(0x1024223c);a.value(0,1);a.store<1>(14,0x16,0);}
        if(!major_contact(a,0x10242244))return;
        detail::advance_action_random(a,0x10242264);a.load(4,30,0);a.value(3,0x55550000);a.load(0,27,0);a.offset(3,3,0x5556);
        a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(3,3,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);a.subtract(0,0,4,true);
        if(!a.equal()){emit_actor_side(a,0x102422bc,0x20d9);a.jump(0x10242480);}return;
    }
    detail::advance_action_random(a,0x102422e8);a.load(4,30,0);a.value(3,0x38e40000);a.load(0,27,0);a.offset(3,3,-0x71c7);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);
    a.add(0,0,3);a.multiply(0,0,9);a.subtract(0,0,4,true);if(!a.equal())return;
    emit_actor_side(a,0x10242344,0x20df);
    detail::advance_action_random(a,0x1024236c);a.load(4,30,0);a.value(3,0x55550000);a.load(0,27,0);a.offset(3,3,0x5556);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(3,3,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);a.subtract(0,0,4,true);
    if(a.equal())emit_actor_side(a,0x102423c4,0x20e0);
    if(!major_contact(a,0x102423ec))return;
    detail::advance_action_random(a,0x1024240c);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(0,3,0);
    a.mask(3,0,1,false,1);a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0,true);
    if(!a.equal())emit_actor_side(a,0x10242458,0x20d9);
}
}

void complete_execution_result(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,result_outcome_phase,dependencies,observer);a.load<1>(4,15,0x105);a.sign16(0,17);a.load(3,15,0x1a4);a.compare(0,0x1f7a);
    a.value(0,0);a.store<1>(3,0x791d,4);a.load<1>(4,15,0x106);a.load(3,15,0x1a4);a.store<1>(3,0x791e,4);
    a.load<1>(3,15,0x27);a.load(4,15,0x1a4);a.sign8(3,3);a.leading_zeroes(3,3);a.mask(3,3,0x07ffffff,false,27);a.store<1>(4,0x7924,3);
    a.load(3,15,0x1a4);a.offset(3,3,0x10000);a.store(3,-0x2000,0);if(a.equal())emit_restart_side(a,0x10241b70);
    describe_actor_contact(a);choose_restart_player(a);
    a.at(0x102421c0);a.load(3,15,0x1a4);a.move(4,13);a.call(0x101ee530);a.sign8(0,3,true);
    if(a.equal()) {
        describe_execution_end(a);a.at(0x10242480);a.load(3,15,0x1a4);a.move(4,15);a.floating_load(1,1,0x4c,true);a.call(0x101d6780);
        a.move(3,15);a.value(4,0);a.call(0x102884d0);
    }
    a.at(0x1024249c);a.load<2>(3,15,0x36,true);a.offset(0,3,-25);a.store<2>(15,0x36,0);a.jump(return_phase);state.pc=return_phase;
}
}
