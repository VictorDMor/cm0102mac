#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

void emit_restart_side(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,15,0x1a4);a.move(4,17);a.load<1>(0,15,0x27);a.value(6,0xffffffff);a.load(3,3,0x79f4);
    a.value(8,0xffffffff);a.load(5,15,0x1a4);a.sign8(7,0);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
}

void emit_restart_player(Call& a,std::uint32_t start,unsigned message) {
    a.at(start);a.load(3,15,0x1a4);a.value(4,message);a.load<1>(0,15,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);
    a.value(9,0xffffffff);a.load<1>(6,13,0x2f);a.sign8(7,0);a.load(5,15,0x1a4);a.value(10,0);a.call(0x10201180);
}

void emit_contacted_player(Call& a,std::uint32_t start,bool selected) {
    a.at(start);a.load(3,15,0x1a4);if(selected)a.move(4,16);else a.value(4,0x2109);
    a.load<1>(7,14,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);a.value(9,0xffffffff);
    a.load<1>(6,14,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);
}

void emit_opposing_side(Call& a,std::uint32_t start,unsigned message) {
    a.at(start);a.load<1>(0,15,0x27);a.value(4,message);a.load(3,15,0x1a4);a.value(6,0xffffffff);a.sign8(0,0);
    a.load(5,15,0x1a4);a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);
    a.value(8,0xffffffff);a.sign8(7,0);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
}

void emit_blocked_pair(Call& a,std::uint32_t start,unsigned message) {
    a.at(start);a.load(3,15,0x1a4);a.value(4,message);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
    a.load<1>(6,15,0x2f);a.load<1>(7,15,0x27);a.load<1>(8,14,0x2f);a.load<1>(9,14,0x27);a.call(0x10201180);
}

bool major_contact(Call& a,std::uint32_t start) {
    a.at(start);a.load<1>(3,1,0x40);a.offset(0,3,-3);a.mask(0,0,0xff);a.compare(0,3,false);if(a.nonpositive())return true;
    a.sign16(0,17);a.compare(0,0x1f7a);return a.equal();
}

void describe_blocked_action(Call& a) {
    a.load<1>(0,1,0x48);a.compare(0,44);bool relevant=a.equal();
    if(!relevant){a.load<1>(0,1,0x48);a.compare(0,47);relevant=a.equal();}
    if(!relevant)return;
    a.at(0x1024069c);a.load<1>(0,1,0x42);a.sign8(0,0,true);if(a.equal())return;
    a.load<1>(0,1,0x48);a.compare(0,47);
    if(a.equal()){emit_blocked_pair(a,0x102406b4,0x21da);a.jump(0x10240708);}
    else emit_blocked_pair(a,0x102406e0,0x21d9);
    if(major_contact(a,0x10240708)){a.at(0x10240728);a.value(0,1);a.store<1>(14,0x16,0);}
}

void load_receiver_opposition(Call& a,std::uint32_t start) {
    a.at(start);a.load<1>(0,14,0x27);a.load(3,15,0x1a4);a.sign8(0,0);a.leading_zeroes(0,0);
    a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0,true);
    if(a.equal()){a.at(start+0x28);a.load(3,3,0x3118);a.load(3,3,0x1c);}
    else {a.load(3,3,0x3118);a.load(3,3,0x20);a.jump(start+0x30);}
}

void maybe_emit_receiver_opposition(Call& a) {
    load_receiver_opposition(a,0x10240950);a.load(0,3,0x2c);a.compare(0,0,false);if(a.equal())return;
    load_receiver_opposition(a,0x1024098c);a.load(3,3,0x2c);a.load<1>(0,3,0x119);a.compare(0,8);if(!a.equal())return;
    a.load(3,15,0x1a4);a.value(4,0x20f5);a.load(5,15,0x1a4);a.value(6,0);a.load(3,3,0x79f4);
    a.value(7,0);a.value(8,0xffffffff);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);
}

void describe_receiver_contact(Call& a) {
    a.at(0x10240678);a.sign16(0,16);a.compare(0,-1);if(a.nonpositive())return;
    describe_blocked_action(a);
    detail::advance_action_random(a,0x10240730);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
    a.mask(0,3,0xc0000000,false,30);a.mask(3,3,1,false,1);a.subtract(0,3,0);a.mask(0,0,0xffffffff,false,2);a.add_record(0,0,3);
    if(a.equal()) {
        a.load<1>(18,1,0x43);a.sign8(18,18,true);
        if(a.equal()){a.value(13,0);a.jump(0x102408f0);}else detail::sample_action_interval(a,0x10240794,18,13);
        detail::advance_action_random(a,0x102408f0);a.load(4,30,0);a.value(3,0x66660000);a.load(0,27,0);a.offset(3,3,0x6667);
        a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,3);a.mask(3,0,1,false,1);
        a.add(0,0,3);a.multiply(0,0,20);a.subtract(0,0,4);a.compare_registers(13,0);
        if(!a.nonpositive())maybe_emit_receiver_opposition(a);
        emit_contacted_player(a,0x102409f4,false);
    }
    emit_contacted_player(a,0x10240a1c,true);a.load<1>(0,1,0x40);a.compare(0,6);bool response=a.equal();
    if(!response){a.load<1>(3,1,0x40);a.offset(0,3,-3);a.mask(0,0,0xff);a.compare(0,1,false);response=a.nonpositive();}
    if(!response){a.load<1>(0,1,0x40);a.compare(0,5);response=a.equal();}
    if(response){a.at(0x10240a70);a.load(3,15,0x1a4);a.move(4,14);a.value(5,0);a.call(0x101f1530);}
}

void describe_restart_followup(Call& a) {
    a.at(0x10240b44);a.sign16(0,17);a.compare(0,0x1f79);if(!a.equal())return;
    a.load<1>(0,1,0x42);a.sign8(0,0,true);if(!a.equal())return;
    a.load<1>(0,1,0x40);a.compare(0,7);
    if(a.equal()) {
        a.move(3,15);a.call(0x10272150);a.sign8(0,3,true);if(a.equal())return;
        a.load<1>(0,15,0x27);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0);a.compare(0,1);
        const bool right=a.equal();detail::area_coordinates(a,right?0x10240b94:0x10240be4,0x10240c2c,right,0,3,0);
        a.at(0x10240c2c);a.sign8(0,0,true);if(!a.equal())return;
        emit_restart_player(a,0x10240c34,0x21e1);a.jump(0x10240cf4);return;
    }
    a.at(0x10240c64);a.load<1>(4,15,0x105);a.move(3,15);a.call(0x102720a0);a.sign8(0,3,true);
    if(!a.equal()) {
        a.move(3,15);a.call(0x10272150);a.sign8(0,3,true);
        if(!a.equal()){emit_restart_player(a,0x10240c88,0x21df);a.jump(0x10240cf4);return;}
    }
    a.at(0x10240cb8);a.move(3,15);a.call(0x10272150);a.sign8(0,3,true);if(a.equal())return;
    emit_restart_player(a,0x10240cc8,0x21e0);
}

void choose_restart_player(Call& a) {
    a.at(0x10240a80);a.sign16(0,17);a.compare(0,0x1f7a);if(a.equal())return;
    a.load(4,15,0x1a4);a.value(6,0);a.load(3,15,0x1a4);a.load<1>(5,4,0x7928);a.load<1>(4,15,0x27);a.call(0x101f3a30);a.move(13,3,true);
    if(a.equal()){emit_restart_side(a,0x10240aac);a.jump(0x10240cf4);return;}
    a.at(0x10240adc);a.load<1>(0,1,0x40);a.compare(0,7);
    if(a.equal()){emit_restart_player(a,0x10240ae8,0x21e2);a.jump(0x10240b44);}else emit_restart_side(a,0x10240b18);
    describe_restart_followup(a);
}

void maybe_emit_major_contact(Call& a,std::uint32_t start,std::uint32_t draw,std::uint32_t message,bool jump) {
    if(!major_contact(a,start))return;
    detail::advance_action_random(a,draw);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(0,3,0);
    a.mask(3,0,1,false,1);a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0,true);if(!a.equal())return;
    emit_opposing_side(a,message,0x20d9);if(jump)a.jump(0x10240fcc);
}

void describe_receiver_end(Call& a) {
    a.load<1>(0,1,0x42);a.sign8(0,0,true);
    if(!a.equal()) {
        a.load(3,15,0x1a4);a.value(4,0x20dd);a.load<1>(7,15,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);
        a.value(9,0xffffffff);a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);
        a.load<1>(0,1,0x40);a.compare(0,4);bool mark=a.equal();
        if(!mark){a.load<1>(0,1,0x40);a.compare(0,5);mark=a.equal();}
        if(mark){a.at(0x10240d54);a.value(0,1);a.store<1>(14,0x16,0);}
        maybe_emit_major_contact(a,0x10240d5c,0x10240d7c,0x10240dc8,true);return;
    }
    detail::advance_action_random(a,0x10240e04);a.load(4,30,0);a.value(3,0x38e40000);a.load(0,27,0);a.offset(3,3,-0x71c7);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);
    a.add(0,0,3);a.multiply(0,0,9);a.subtract(0,0,4,true);if(!a.equal())return;
    emit_opposing_side(a,0x10240e60,0x20df);
    detail::advance_action_random(a,0x10240e98);a.load(4,30,0);a.value(3,0x55550000);a.load(0,27,0);a.offset(3,3,0x5556);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(3,3,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);a.subtract(0,0,4,true);
    if(a.equal())emit_opposing_side(a,0x10240ef0,0x20e0);
    maybe_emit_major_contact(a,0x10240f28,0x10240f48,0x10240f94,false);
}
}

void complete_receiver_contact(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,receiver_outcome_phase,dependencies,observer);a.load<1>(4,15,0x105);a.sign16(0,17);a.load(3,15,0x1a4);a.compare(0,0x1f7a);
    a.value(0,0);a.store<1>(3,0x791d,4);a.load<1>(4,15,0x106);a.load(3,15,0x1a4);a.store<1>(3,0x791e,4);
    a.load<1>(4,15,0x27);a.load(3,15,0x1a4);a.store<1>(3,0x7924,4);a.load(3,15,0x1a4);a.offset(3,3,0x10000);a.store(3,-0x2000,0);
    if(a.equal())emit_restart_side(a,0x1024064c);
    describe_receiver_contact(a);choose_restart_player(a);
    a.at(0x10240cf4);a.load(3,15,0x1a4);a.load<1>(4,1,0x48);a.call(0x101ee530);a.sign8(0,3,true);
    if(a.equal()) {
        describe_receiver_end(a);a.at(0x10240fcc);a.load(3,15,0x1a4);a.move(4,14);a.floating_load(1,1,0x4c,true);a.call(0x101d6780);
        a.move(3,14);a.value(4,0);a.call(0x102884d0);
    }
    a.at(0x10240fe8);a.load<2>(3,14,0x36,true);a.offset(0,3,-25);a.store<2>(14,0x36,0);a.jump(return_phase);state.pc=return_phase;
}
}
