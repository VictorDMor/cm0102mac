#include "player_movement.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void choose_contact_samples(Call& a) {
    a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.load<1>(13,22,0x10f);a.load<1>(14,22,0x110);
    a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(0x102342f4);a.load(5,30,0);a.value(4,0x66660000);a.load(3,30,0);a.offset(4,4,0x6667);a.load<2>(6,5,0);
    a.load(5,27,0);a.offset(3,3,2);a.load(0,29,0);a.add(5,6,5);a.store(30,0,3);a.multiply_high(4,4,5);
    a.compare_registers(3,0,false);a.shift_right(0,4,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,10);a.subtract(16,0,5);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
}
bool contact_bound_zero(Call& a) {
    a.at(0x1023434c);a.load(3,30,0);a.value(4,0x66660000);a.load(5,27,0);a.offset(4,4,0x6667);a.load<2>(7,3,0);
    a.sign8(0,13);a.combine(3,0,16,false,true);a.sign8(6,14);a.add(7,7,5);a.multiply_high(4,4,7);a.intersect(0,3,0);
    a.shift_right(4,4,2);a.mask(5,4,1,false,1);a.add(4,4,5);a.multiply(4,4,10);a.subtract(17,4,7);
    a.combine(4,6,17,false,true);a.shift_right(5,4,1);a.shift_right(3,3,1);a.intersect(4,4,6);a.subtract(0,0,3);a.subtract(3,4,5);
    a.mask(0,0,1,false,1);a.mask(3,3,1,false,1);a.subtract_from(0,0,3);a.subtract(0,3,0,true);return a.equal();
}
void advance_contact_random(Call& a) {
    a.at(0x102343c0);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(0x102343e8);
}
bool contact_bound_wide(Call& a) {
    a.sign8(6,14);a.sign8(5,13);a.combine(4,6,17,false,true);a.value(3,0x10000);a.combine(0,5,16,false,true);
    a.shift_right(7,4,1);a.intersect(6,4,6);a.shift_right(4,0,1);a.intersect(0,0,5);a.subtract(0,0,4);a.offset(3,3,-1);
    a.subtract(4,6,7);a.mask(0,0,1,false,1);a.mask(4,4,1,false,1);a.subtract_from(0,0,3);a.subtract(0,4,0);a.compare_registers(0,3);
    if(a.positive())return true;
    a.sign8(6,14);a.sign8(5,13);a.combine(4,6,17,false,true);a.value(3,0xffff0000);a.combine(0,5,16,false,true);
    a.shift_right(7,4,1);a.intersect(6,4,6);a.shift_right(4,0,1);a.intersect(0,0,5);a.subtract(0,0,4);a.offset(3,3,1);
    a.subtract(4,6,7);a.mask(0,0,1,false,1);a.mask(4,4,1,false,1);a.subtract_from(0,0,3);a.subtract(0,4,0);a.compare_registers(0,3);
    return !a.at_least();
}
void sample_contact_bound_wide(Call& a) {
    a.at(0x10234478);a.load(4,30,0);a.value(3,0x80010000);a.value(0,0x43300000);a.sign8(12,14);a.load<2>(6,4,0);
    a.sign8(7,13);a.load(4,27,0);a.combine(9,12,17,false,true);a.offset(3,3,-0x7fff);a.combine(5,7,16,false,true);
    a.add(6,6,4);a.value(4,0x10000);a.multiply_high(11,3,6);a.store(1,0xbc,0);a.sign8(10,14);a.floating_load(2,26,0x80);
    a.intersect(7,5,7);a.floating_load(0,26,0);a.add(0,11,6);a.value(11,0x43300000);a.shift_right(0,0,15);a.intersect(12,9,12);
    a.shift_right(18,9,1);a.offset(4,4,-1);a.shift_right(9,5,1);a.mask(5,0,1,false,1);a.subtract(7,7,9);a.subtract(12,12,18);
    a.mask(9,7,1,false,1);a.add(0,0,5);a.subtract_from(9,9,3);a.mask(12,12,1,false,1);a.subtract(12,12,9);a.sign8(8,13);
    a.exclusive_flags(12,12,0x80000000);a.combine(3,10,17,false,true);a.store(1,0xc0,12);a.product(5,0,4);a.combine(7,8,16,false,true);
    a.floating_load(1,1,0xbc);a.shift_right(9,3,1);a.intersect(3,3,10);a.store(1,0xc4,11);a.floating_subtract(2,1,2);
    a.subtract(18,5,6);a.exclusive_flags(5,18,0x80000000);a.subtract(9,3,9);a.value(3,0x80010000);a.store(1,0xc8,5);
    a.floating_difference(2,2,0,true);a.shift_right(4,7,1);a.intersect(0,7,8);a.floating_load(1,26,0x80);a.subtract(0,0,4);
    a.floating_load(0,1,0xc4);a.mask(0,0,1,false,1);a.floating_subtract(0,0,1);a.mask(4,9,1,false,1);a.offset(3,3,-0x7fff);
    a.subtract_from(0,0,3);a.subtract(0,4,0);a.floating_product(0,2,0);a.multiply(0,0,10);a.multiply_high(3,3,0);a.floating_integer(0,0);
    a.floating_store(0,1,0xcc);a.add(0,3,0);a.load(18,1,0xd0);a.shift_right(0,0,15);a.mask(3,0,1,false,1);a.add(3,0,3);
    a.call(0x10226520);a.value(5,0x66660000);a.offset(3,3,5);a.offset(5,5,0x6667);a.sign8(4,14);a.multiply_high(6,5,3);
    a.sign8(0,13);a.combine(5,4,17,false,true);a.combine(3,0,16,false,true);a.intersect(4,5,4);a.intersect(0,3,0);
    a.shift_right(6,6,2);a.shift_right(5,5,1);a.shift_right(3,3,1);a.mask(7,6,1,false,1);a.subtract(0,0,3);a.subtract(3,4,5);
    a.add(4,6,7);a.mask(0,0,1,false,1);a.mask(3,3,1,false,1);a.add(18,18,4);a.subtract_from(0,0,3);a.subtract(3,3,0);
    a.offset(0,3,-1);a.compare_registers(18,0);if(a.nonpositive())return;
    a.sign8(5,14);a.sign8(4,13);a.combine(3,5,17,false,true);a.combine(0,4,16,false,true);a.shift_right(6,3,1);a.intersect(5,3,5);
    a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.subtract(3,5,6);a.mask(0,0,1,false,1);a.mask(3,3,1,false,1);
    a.subtract_from(0,0,3);a.subtract(3,3,0);a.offset(18,3,-1);a.jump(0x102346d8);
}
void sample_contact_bound_narrow(Call& a) {
    a.at(0x1023463c);a.sign8(6,14);a.sign8(4,13);a.combine(3,6,17,false,true);a.load(8,30,0);a.combine(0,4,16,false,true);a.sign8(5,14);
    a.shift_right(7,3,1);a.intersect(6,3,6);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.subtract(3,6,7);
    a.mask(0,0,1,false,1);a.load<2>(7,8,0);a.load(4,27,0);a.mask(6,3,1,false,1);a.subtract_from(0,0,3);a.combine(3,5,17,false,true);
    a.add(9,7,4);a.sign8(4,13);a.subtract(8,6,0);a.shift_right(6,3,1);a.divide(7,9,8);a.combine(0,4,16,false,true);a.intersect(5,3,5);
    a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(4,5,6);a.product(5,7,8);a.subtract(0,0,3);a.mask(3,4,1,false,1);a.mask(0,0,1,false,1);
    a.subtract_from(0,0,3);a.subtract(18,5,9);a.subtract(0,3,0,true);a.negate(0,18);if(!a.nonpositive())a.move(0,18);
    a.at(0x102346d4);a.move(18,0);
}
}
void continue_carrier_distance(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    const bool existing=state.pc==0x102346e4;Call a(state,existing?0x102346e4:carrier_distance_phase,dependencies,observer);
    if(existing){a.sign8(0,13,true);if(!a.equal()){a.move(3,15);a.call(0x10288230);a.jump(carrier_ratings_phase);}}
    else {choose_contact_samples(a);if(contact_bound_zero(a)){a.value(18,0);a.jump(0x102346d8);}
        else {advance_contact_random(a);if(contact_bound_wide(a))sample_contact_bound_wide(a);else sample_contact_bound_narrow(a);}
        a.at(0x102346d8);a.store<1>(22,0x2c,18);a.value(16,1);a.jump(carrier_ratings_phase);}
    state.pc=carrier_ratings_phase;
}
}
