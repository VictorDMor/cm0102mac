#include "player_movement.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::player_movement {
namespace {
using Call = SemanticGuestCall;

void advance_environment_random(Call& a,std::uint32_t start,std::uint32_t ready) {
    a.at(start);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(ready);
}

void apply_surface_adjustment(Call& a) {
    a.load(3,15,0x1a4);a.load<1>(0,3,1);a.sign8(0,0);a.compare(0,8);
    std::uint32_t selected=0;
    if(a.equal())selected=0x1022f5b0;
    else if(!a.at_least()) {
        a.compare(0,3);if(!a.at_least()){a.compare(0,1);if(a.at_least())selected=0x1022f54c;else a.jump(0x1022f670);}
    } else {a.at(0x1022f540);a.compare(0,32);if(a.equal())selected=0x1022f618;else a.jump(0x1022f670);}
    if(!selected)return;
    advance_environment_random(a,selected,selected+0x28);
    if(selected==0x1022f54c) {
        a.load(4,30,0);a.value(3,0x55550000);a.load(0,27,0);a.offset(3,3,0x5556);a.load<2>(4,4,0);
        a.add(4,4,0);a.multiply_high(3,3,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);
        a.subtract(3,0,4);a.offset(0,3,1);a.sign16(0,0);a.subtract(21,0,21);a.jump(0x1022f670);
    } else if(selected==0x1022f5b0) {
        a.load(4,30,0);a.value(3,0x66660000);a.load(0,27,0);a.offset(3,3,0x6667);a.load<2>(4,4,0);
        a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);
        a.multiply(0,0,5);a.subtract(3,0,4);a.offset(0,3,2);a.sign16(0,0);a.subtract(21,0,21);a.jump(0x1022f670);
    } else {
        a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);a.mask(0,3,0xe0000000,false,29);
        a.mask(3,3,1,false,1);a.subtract(0,3,0);a.mask(0,0,0xffffffff,false,3);a.add(3,0,3);
        a.offset(0,3,3);a.sign16(0,0);a.subtract(21,0,21);
    }
}

void narrow_ball_state_sample(Call& a) {
    a.at(0x1022f868);a.sign8(3,24);a.sign8(0,23);a.product(4,3,0);a.sign8(5,25);a.load(7,30,0);
    a.sign8(3,24);a.sign8(0,23);a.load(6,27,0);a.product(5,5,4);a.load<2>(7,7,0);a.sign8(4,25);
    a.add(7,7,6);a.multiply(6,5,50);a.divide(5,7,6);a.product(0,3,0);a.product(0,4,0);a.product(3,5,6);
    a.multiply(0,0,50);a.subtract(22,3,7);a.compare(0,0);a.negate(0,22);if(!a.nonpositive())a.move(0,22);
    a.at(0x1022f8c4);a.move(22,0);
}

void wide_ball_state_sample(Call& a) {
    a.at(0x1022f728);a.sign8(3,24);a.sign8(0,23);a.product(3,3,0);a.sign8(4,25);a.load(6,30,0);
    a.value(0,0x43300000);a.store(1,0xbc,0);a.value(5,0x80010000);a.product(7,4,3);a.value(0,0x43300000);
    a.value(4,0x10000);a.load<2>(9,6,0);a.value(3,0x80010000);a.floating_load(3,26,0x80);a.multiply(7,7,50);
    a.floating_load(0,26,0);a.load(8,27,0);a.offset(5,5,-0x7fff);a.store(1,0xc4,0);a.offset(4,4,-1);
    a.exclusive_flags(6,7,0x80000000);a.add(0,9,8);a.store(1,0xc0,6);a.multiply_high(8,5,0);a.sign8(6,24);
    a.floating_load(1,26,0x80);a.floating_load(2,1,0xbc);a.sign8(5,23);a.sign8(7,25);a.floating_subtract(2,2,3);
    a.add(8,8,0);a.shift_right(8,8,15);a.offset(3,3,-0x7fff);a.mask(9,8,1,false,1);a.floating_difference(2,2,0,true);
    a.add(8,8,9);a.product(8,8,4);a.subtract(22,8,0);a.exclusive_flags(0,22,0x80000000);a.store(1,0xc8,0);
    a.product(4,6,5);a.floating_load(0,1,0xc4);a.floating_subtract(0,0,1);a.product(0,7,4);a.floating_product(0,2,0);
    a.floating_integer(0,0);a.multiply(0,0,500);a.floating_store(0,1,0xcc);a.multiply_high(3,3,0);a.load(22,1,0xd0);
    a.add(0,3,0);a.shift_right(0,0,15);a.mask(3,0,1,false,1);a.add(3,0,3);a.call(0x10226520);
    a.sign8(4,24);a.sign8(0,23);a.product(0,4,0);a.value(5,0x66660000);a.sign8(4,25);a.offset(3,3,5);
    a.offset(5,5,0x6667);a.multiply_high(3,5,3);a.product(0,4,0);a.shift_right(4,3,2);a.mask(5,4,1,false,1);
    a.multiply(3,0,50);a.add(0,4,5);a.add(22,22,0);a.offset(0,3,-1);a.compare_registers(22,0);
    if(!a.nonpositive()){a.sign8(3,24);a.sign8(0,23);a.product(0,3,0);a.sign8(3,25);a.product(0,3,0);
        a.multiply(3,0,50);a.offset(22,3,-1);a.jump(0x1022f8c8);}
}

void sample_ball_state(Call& a) {
    a.at(0x1022f670);a.load(3,15,0x1a4);a.load(4,15,0x1a4);a.load<1>(24,3,0x791f);a.load<1>(23,4,0x791f);
    a.sign8(3,24);a.load(4,15,0x1a4);a.sign8(0,23);a.product(0,3,0);a.load<1>(25,4,0x791f);
    a.sign8(3,25);a.product(0,3,0);a.multiply(0,0,50);a.compare(0,0);
    if(a.equal()){a.value(22,0);a.jump(0x1022f8c8);return;}
    advance_environment_random(a,0x1022f6b0,0x1022f6d8);
    a.sign8(3,24);a.sign8(0,23);a.product(0,3,0);a.sign8(4,25);a.value(3,0x10000);a.offset(3,3,-1);
    a.product(0,4,0);a.multiply(0,0,50);a.compare_registers(0,3);bool wide=a.positive();
    if(!wide){a.sign8(3,24);a.sign8(0,23);a.product(0,3,0);a.sign8(4,25);a.value(3,0xffff0000);
        a.offset(3,3,1);a.product(0,4,0);a.multiply(0,0,50);a.compare_registers(0,3);wide=!a.at_least();}
    if(wide)wide_ball_state_sample(a);else narrow_ball_state_sample(a);
}

void wide_condition_sample(Call& a) {
    a.at(0x1022f944);a.exclusive_flags(3,23,0x80000000);a.value(0,0x43300000);a.store(1,0xc0,3);a.value(6,0x80010000);
    a.load(7,30,0);a.value(4,0x43300000);a.store(1,0xbc,0);a.value(5,0x10000);a.floating_load(2,26,0x80);
    a.value(3,0x80010000);a.floating_load(1,1,0xbc);a.offset(6,6,-0x7fff);a.floating_load(0,26,0);
    a.offset(5,5,-1);a.floating_subtract(1,1,2);a.load<2>(8,7,0);a.load(7,27,0);a.multiply(0,23,10);
    a.offset(3,3,-0x7fff);a.store(1,0xc4,4);a.floating_difference(2,1,0,true);a.add(7,8,7);a.multiply_high(4,6,7);
    a.floating_load(1,26,0x80);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.add(4,4,6);
    a.product(4,4,5);a.subtract(22,4,7);a.exclusive_flags(4,22,0x80000000);a.store(1,0xc8,4);a.multiply_high(3,3,0);
    a.floating_load(0,1,0xc4);a.floating_subtract(0,0,1);a.add(0,3,0);a.shift_right(0,0,15);a.floating_product(0,2,0);
    a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,0xcc);a.load(22,1,0xd0);
    a.call(0x10226520);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,23,-1);
    a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(22,22,3);a.compare_registers(22,0);
    if(!a.nonpositive()){a.offset(22,23,-1);a.jump(carrier_interaction_phase);}
}

void sample_condition_adjustment(Call& a) {
    a.at(0x1022f8c8);a.value(3,0x66660000);a.load<2>(0,15,0x2a,true);a.offset(3,3,0x6667);a.sign16(4,22);
    a.multiply_high(0,3,0);a.subtract(21,4,21);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(3,0,3);
    a.offset_record(23,3,-30);if(a.equal()){a.value(22,0);a.jump(carrier_interaction_phase);return;}
    advance_environment_random(a,0x1022f8fc,0x1022f924);
    a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(23,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(23,3);wide=!a.at_least();}
    if(wide)wide_condition_sample(a);
    else {a.at(0x1022fa28);a.load(3,30,0);a.compare(23,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
        a.divide(0,3,23);a.product(0,0,23);a.subtract(22,0,3);a.negate(0,22);if(!a.nonpositive())a.move(0,22);
        a.at(0x1022fa54);a.move(22,0);}
}
}

void continue_carrier_environment(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call call(state,carrier_decision_phase,dependencies,observer);
    apply_surface_adjustment(call);sample_ball_state(call);sample_condition_adjustment(call);state.pc=carrier_interaction_phase;
}
}
