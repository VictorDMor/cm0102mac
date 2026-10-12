#include "player_movement.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::player_movement {
namespace {
using Call = SemanticGuestCall;

// The original interval keeps signed halfword conversion, word multiplication
// and reciprocal division. Its stack slots and intermediate registers remain
// visible to interruptions and to subsequent sampling dependencies.
void control_interval(Call& a,std::uint32_t start,unsigned first_slot,unsigned second_slot,bool upper_limit=false) {
    a.at(start);a.floating_integer(1,31);a.value(5,0x14f90000);a.floating_integer(0,30);
    if(upper_limit)a.value(3,0x10000);
    a.offset(5,5,-0x4a77);a.floating_store(1,1,first_slot);
    if(upper_limit)a.offset(3,3,-1);
    a.floating_store(0,1,second_slot);a.load(4,1,first_slot+4);a.load(0,1,second_slot+4);
    a.sign16(4,4);a.subtract_from(4,4,10000);a.sign16(0,0);a.subtract_from(0,0,10000);a.product(0,4,0);
    a.multiply_high(0,5,0);a.shift_right(0,0,12);a.mask(4,0,1,false,1);a.add(0,0,4);
}

void narrow_control_sample(Call& a) {
    a.at(0x1022ef64);a.floating_integer(0,31);a.load(3,30,0);a.floating_integer(2,30);a.value(7,0x14f90000);
    a.floating_integer(1,31);a.value(5,0x14f90000);a.floating_store(0,1,0xbc);a.floating_integer(0,30);a.load<2>(8,3,0);
    a.offset(7,7,-0x4a77);a.floating_store(2,1,0xc4);a.offset(5,5,-0x4a77);a.load(3,1,0xc0);a.load(0,1,0xc8);
    a.sign16(3,3);a.load(4,27,0);a.subtract_from(3,3,10000);a.sign16(0,0);a.subtract_from(0,0,10000);
    a.floating_store(1,1,0xcc);a.product(6,3,0);a.add(0,8,4);a.floating_store(0,1,0xd4);a.load(4,1,0xd0);a.load(3,1,0xd8);
    a.sign16(4,4);a.multiply_high(6,7,6);a.sign16(3,3);a.shift_right(6,6,12);a.mask(7,6,1,false,1);
    a.subtract_from(4,4,10000);a.add(7,6,7);a.subtract_from(3,3,10000);a.divide(6,0,7);a.product(3,4,3);
    a.multiply_high(3,5,3);a.product(5,6,7);a.shift_right(3,3,12);a.mask(4,3,1,false,1);a.subtract(21,5,0);
    a.add_record(0,3,4);a.negate(0,21);if(a.nonpositive())a.at(0x1022f014);else a.move(0,21);
    a.at(0x1022f014);a.move(21,0);
}

void wide_control_sample(Call& a) {
    a.at(0x1022ed70);a.load(5,30,0);a.value(3,0x80010000);a.floating_integer(1,31);a.value(4,0x14f90000);
    a.load<2>(5,5,0);a.floating_integer(0,30);a.load(0,27,0);a.value(11,0x43300000);a.add(7,5,0);
    a.offset(3,3,-0x7fff);a.multiply_high(0,3,7);a.floating_store(1,1,0x104);a.floating_integer(1,31);a.value(5,0x10000);
    a.floating_store(0,1,0x10c);a.floating_integer(0,30);a.load(6,1,0x108);a.add(0,0,7);a.load(3,1,0x110);
    a.shift_right(0,0,15);a.sign16(6,6);a.floating_store(0,1,0xdc);a.subtract_from(6,6,10000);a.sign16(3,3);
    a.subtract_from(3,3,10000);a.floating_store(1,1,0xe4);a.product(10,6,3);a.offset(4,4,-0x4a77);a.load(9,1,0xe8);
    a.mask(6,0,1,false,1);a.load(8,1,0xe0);a.value(3,0x14f90000);a.multiply_high(12,4,10);a.store(1,0xfc,11);
    a.value(10,0x43300000);a.floating_load(2,26,0x80);a.value(4,0x80010000);a.floating_load(0,26,0);
    a.shift_right(11,12,12);a.sign16(9,9);a.mask(12,11,1,false,1);a.sign16(8,8);a.add(11,11,12);
    a.subtract_from(9,9,10000);a.exclusive_flags(11,11,0x80000000);a.add(6,0,6);a.store(1,0x100,11);
    a.subtract_from(0,8,10000);a.offset(5,5,-1);a.offset(3,3,-0x4a77);a.floating_load(1,1,0xfc);a.product(5,6,5);
    a.store(1,0xf4,10);a.offset(4,4,-0x7fff);a.floating_subtract(2,1,2);a.floating_load(1,26,0x80);
    a.floating_difference(2,2,0,true);a.product(0,9,0);a.subtract(21,5,7);a.exclusive_flags(5,21,0x80000000);
    a.store(1,0xf8,5);a.multiply_high(0,3,0);a.floating_load(0,1,0xf4);a.shift_right(0,0,12);a.mask(3,0,1,false,1);
    a.floating_subtract(0,0,1);a.add(0,0,3);a.multiply(0,0,10);a.floating_product(0,2,0);a.multiply_high(3,4,0);
    a.floating_integer(0,0);a.add(0,3,0);a.floating_store(0,1,0xec);a.shift_right(0,0,15);a.mask(3,0,1,false,1);
    a.load(21,1,0xf0);a.add(3,0,3);a.call(0x10226520);
    a.floating_integer(1,31);a.value(5,0x66660000);a.floating_integer(0,30);a.offset(0,3,5);a.offset(5,5,0x6667);
    a.value(4,0x14f90000);a.multiply_high(5,5,0);a.floating_store(1,1,0xd4);a.offset(4,4,-0x4a77);
    a.floating_store(0,1,0xcc);a.load(3,1,0xd8);a.load(0,1,0xd0);a.shift_right(5,5,2);a.sign16(3,3);
    a.subtract_from(3,3,10000);a.sign16(0,0);a.subtract_from(0,0,10000);a.mask(6,5,1,false,1);a.product(0,3,0);
    a.add(3,5,6);a.add(21,21,3);a.multiply_high(0,4,0);a.shift_right(0,0,12);a.mask(3,0,1,false,1);
    a.add(3,0,3);a.offset(0,3,-1);a.compare_registers(21,0);
    if(a.nonpositive())return;
    a.floating_integer(1,31);a.value(4,0x14f90000);a.floating_integer(0,30);a.offset(4,4,-0x4a77);
    a.floating_store(1,1,0xc4);a.floating_store(0,1,0xbc);a.load(3,1,0xc8);a.load(0,1,0xc0);
    a.sign16(3,3);a.subtract_from(3,3,10000);a.sign16(0,0);a.subtract_from(0,0,10000);a.product(0,3,0);
    a.multiply_high(0,4,0);a.shift_right(0,0,12);a.mask(3,0,1,false,1);a.add(3,0,3);a.offset(21,3,-1);a.jump(0x1022f018);
}

void sample_general_control(Call& a) {
    a.floating_load(31,15,0x50,true);a.value(4,0x14f90000);a.floating_load(30,15,0x50,true);a.offset(4,4,-0x4a77);
    a.floating_integer(1,31);a.floating_integer(0,30);a.floating_store(1,1,0x10c);a.floating_store(0,1,0x104);
    a.load(3,1,0x110);a.load(0,1,0x108);a.sign16(3,3);a.subtract_from(3,3,10000);a.sign16(0,0);
    a.subtract_from(0,0,10000);a.product(0,3,0);a.multiply_high(0,4,0);a.shift_right(0,0,12);a.mask(3,0,1,false,1);
    a.add_record(0,0,3);if(a.equal()){a.value(21,0);a.jump(0x1022f018);return;}
    a.at(0x1022eca0);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    control_interval(a,0x1022ecc8,0x104,0x10c,true);a.compare_registers(0,3);
    bool wide=a.positive();
    if(!wide){a.at(0x1022ed1c);a.floating_integer(1,31);a.value(5,0x14f90000);a.floating_integer(0,30);a.value(3,0xffff0000);
        a.offset(5,5,-0x4a77);a.floating_store(1,1,0x104);a.offset(3,3,1);a.floating_store(0,1,0x10c);
        a.load(4,1,0x108);a.load(0,1,0x110);a.sign16(4,4);a.subtract_from(4,4,10000);a.sign16(0,0);a.subtract_from(0,0,10000);
        a.product(0,4,0);a.multiply_high(0,5,0);a.shift_right(0,0,12);a.mask(4,0,1,false,1);a.add(0,0,4);
        a.compare_registers(0,3);wide=!a.at_least();}
    if(wide)wide_control_sample(a);else narrow_control_sample(a);
}

void accumulate_action_bias(Call& a) {
    a.at(0x1022f018);a.sign8(0,19,true);a.sign16(4,21);if(a.equal())return;
    a.floating_load(1,26,0x40);a.floating_load(0,15,0x9c,true);a.load<2>(0,15,0x1a0,true);a.floating_sum(0,1,0);
    a.floating_integer(0,0);a.floating_store(0,1,0xbc);a.load(3,1,0xc0);a.add(0,0,3);a.store<2>(15,0x1a0,0);
    a.load<2>(0,15,0x1a0,true);a.compare(0,200);
    if(!a.nonpositive()){a.value(0,200);a.store<2>(15,0x1a0,0);a.jump(0x1022f074);return;}
    a.at(0x1022f060);a.load<2>(0,15,0x1a0,true);a.compare(0,-200);
    if(!a.at_least()){a.value(0,0xffffff38);a.store<2>(15,0x1a0,0);}
}

// Three movement kinds use the same signed sampling operation. Only their
// live actor attribute, entry location and final branch schedule differ.
void sample_action_adjustment(Call& a,std::uint32_t start,unsigned attribute,bool final_kind) {
    const auto done=start+0x17c;
    a.at(start);a.load<2>(0,15,attribute,true);a.sign16(4,4);a.load<2>(3,15,0x1a0,true);a.add(0,3,0);
    a.subtract(22,4,0,true);
    if(a.equal()){a.value(21,0);a.jump(done);}
    else {
        a.at(start+0x20);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
        if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
        a.at(start+0x48);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(22,3);bool wide=a.positive();
        if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(22,3);wide=!a.at_least();}
        if(wide) {
            a.at(start+0x68);a.exclusive_flags(3,22,0x80000000);a.value(0,0x43300000);a.store(1,0xc0,3);a.value(6,0x80010000);
            a.load(7,30,0);a.value(4,0x43300000);a.store(1,0xbc,0);a.value(5,0x10000);a.floating_load(2,26,0x80);
            a.value(3,0x80010000);a.floating_load(1,1,0xbc);a.offset(6,6,-0x7fff);a.floating_load(0,26,0);
            a.offset(5,5,-1);a.floating_subtract(1,1,2);a.load<2>(8,7,0);a.load(7,27,0);a.multiply(0,22,10);
            a.offset(3,3,-0x7fff);a.store(1,0xc4,4);a.floating_difference(2,1,0,true);a.add(7,8,7);a.multiply_high(4,6,7);
            a.floating_load(1,26,0x80);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.add(4,4,6);
            a.product(4,4,5);a.subtract(21,4,7);a.exclusive_flags(4,21,0x80000000);a.store(1,0xc8,4);
            a.multiply_high(3,3,0);a.floating_load(0,1,0xc4);a.floating_subtract(0,0,1);a.add(0,3,0);a.shift_right(0,0,15);
            a.floating_product(0,2,0);a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,0xcc);
            a.load(21,1,0xd0);a.call(0x10226520);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,22,-1);
            a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(21,21,3);a.compare_registers(21,0);
            if(!a.nonpositive()){a.offset(21,22,-1);a.jump(done);}
        } else {
            a.at(start+0x14c);a.load(3,30,0);a.compare(22,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
            a.divide(0,3,22);a.product(0,0,22);a.subtract(21,0,3);a.negate(0,21);
            if(!a.nonpositive())a.move(0,21);else a.at(start+0x178);
            a.at(start+0x178);a.move(21,0);
        }
    }
    a.at(done);a.sign16(21,21);if(!final_kind)a.jump(carrier_decision_phase);
}

void sample_carrier(Call& a) {
    sample_general_control(a);accumulate_action_bias(a);a.at(0x1022f074);a.sign16(0,18);a.compare(0,15);
    if(a.equal())sample_action_adjustment(a,0x1022f080,0x13c,false);
    else {a.at(0x1022f204);a.sign16(0,18);a.compare(0,18);
        if(a.equal())sample_action_adjustment(a,0x1022f210,0x142,false);
        else sample_action_adjustment(a,0x1022f394,0x140,true);}
}
}

void continue_carrier_sampling(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call call(state,carrier_phase,dependencies,observer);sample_carrier(call);state.pc=carrier_decision_phase;
}
}
