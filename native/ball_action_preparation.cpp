#include "ball_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

// Fitness is converted twice and narrowed before the wrapped product and
// reciprocal division. Preserve the live spills, flags and register schedule:
// aliases, faults and dependency interruptions can observe every operation.
void fitness_interval(Call& a, std::uint32_t start, bool upper) {
    a.at(start);a.floating_integer(1,31);a.value(5,0x14f90000);a.floating_integer(0,30);
    a.value(3,upper?0x10000:0xffff0000);a.offset(5,5,-0x4a77);a.floating_store(1,1,0xc0);
    a.offset(3,3,upper?-1:1);a.floating_store(0,1,0xc8);a.load(4,1,0xc4);a.load(0,1,0xcc);
    a.sign16(4,4);a.subtract_from(4,4,10000);a.sign16(0,0);a.subtract_from(0,0,10000);
    a.product(0,4,0);a.multiply_high(0,5,0);a.shift_right(0,0,12);a.mask(4,0,1,false,1);
    a.add(0,0,4);a.compare_registers(0,3);
}

void narrow_fitness_sample(Call& a) {
    a.at(0x1023d0d0);a.floating_integer(0,31);a.load(3,30,0);a.floating_integer(2,30);a.value(7,0x14f90000);
    a.floating_integer(1,31);a.value(5,0x14f90000);a.floating_store(0,1,0x78);a.floating_integer(0,30);
    a.load<2>(8,3,0);a.offset(7,7,-0x4a77);a.floating_store(2,1,0x80);a.offset(5,5,-0x4a77);
    a.load(3,1,0x7c);a.load(0,1,0x84);a.sign16(3,3);a.load(4,27,0);a.subtract_from(3,3,10000);
    a.sign16(0,0);a.subtract_from(0,0,10000);a.floating_store(1,1,0x88);a.product(6,3,0);a.add(0,8,4);
    a.floating_store(0,1,0x90);a.load(4,1,0x8c);a.load(3,1,0x94);a.sign16(4,4);a.multiply_high(6,7,6);
    a.sign16(3,3);a.shift_right(6,6,12);a.mask(7,6,1,false,1);a.subtract_from(4,4,10000);a.add(7,6,7);
    a.subtract_from(3,3,10000);a.divide(6,0,7);a.product(3,4,3);a.multiply_high(3,5,3);a.product(5,6,7);
    a.shift_right(3,3,12);a.mask(4,3,1,false,1);a.subtract(13,5,0);a.add_record(0,3,4);a.negate(0,13);
    if(!a.nonpositive())a.move(0,13);
    a.at(0x1023d180);a.move(13,0);
}

void wide_fitness_sample(Call& a) {
    a.at(0x1023cedc);a.load(5,30,0);a.value(4,0x80010000);a.floating_integer(1,31);a.value(3,0x14f90000);
    a.load<2>(5,5,0);a.floating_integer(0,30);a.load(0,27,0);a.value(11,0x43300000);a.add(6,5,0);
    a.offset(4,4,-0x7fff);a.multiply_high(0,4,6);a.floating_store(1,1,0xc0);a.floating_integer(1,31);a.value(4,0x10000);
    a.floating_store(0,1,0xc8);a.floating_integer(0,30);a.load(7,1,0xc4);a.add(0,0,6);a.load(5,1,0xcc);
    a.shift_right(0,0,15);a.sign16(7,7);a.floating_store(1,1,0xa0);a.subtract_from(7,7,10000);a.sign16(5,5);
    a.subtract_from(5,5,10000);a.floating_store(0,1,0x98);a.product(10,7,5);a.offset(3,3,-0x4a77);a.load(8,1,0xa4);
    a.mask(5,0,1,false,1);a.load(7,1,0x9c);a.value(9,0x14f90000);a.multiply_high(12,3,10);a.store(1,0xb8,11);
    a.value(10,0x43300000);a.floating_load(2,26,0x80);a.value(3,0x80010000);a.floating_load(0,26,0);
    a.shift_right(11,12,12);a.sign16(8,8);a.mask(12,11,1,false,1);a.sign16(7,7);a.add(11,11,12);
    a.subtract_from(8,8,10000);a.exclusive_flags(11,11,0x80000000);a.add(5,0,5);a.store(1,0xbc,11);
    a.subtract_from(0,7,10000);a.offset(4,4,-1);a.offset(9,9,-0x4a77);a.floating_load(1,1,0xb8);a.product(4,5,4);
    a.store(1,0xb0,10);a.offset(3,3,-0x7fff);a.floating_subtract(2,1,2);a.floating_load(1,26,0x80);
    a.floating_difference(2,2,0,true);a.product(0,8,0);a.subtract(13,4,6);a.exclusive_flags(4,13,0x80000000);
    a.store(1,0xb4,4);a.multiply_high(0,9,0);a.floating_load(0,1,0xb0);a.shift_right(0,0,12);a.mask(4,0,1,false,1);
    a.floating_subtract(0,0,1);a.add(0,0,4);a.multiply(0,0,10);a.floating_product(0,2,0);a.multiply_high(3,3,0);
    a.floating_integer(0,0);a.add(0,3,0);a.floating_store(0,1,0xa8);a.shift_right(0,0,15);a.mask(3,0,1,false,1);
    a.load(13,1,0xac);a.add(3,0,3);a.call(0x10226520);

    a.floating_integer(1,31);a.value(5,0x66660000);a.floating_integer(0,30);a.offset(0,3,5);a.offset(5,5,0x6667);
    a.value(4,0x14f90000);a.multiply_high(5,5,0);a.floating_store(1,1,0x90);a.offset(4,4,-0x4a77);
    a.floating_store(0,1,0x88);a.load(3,1,0x94);a.load(0,1,0x8c);a.shift_right(5,5,2);a.sign16(3,3);
    a.subtract_from(3,3,10000);a.sign16(0,0);a.subtract_from(0,0,10000);a.mask(6,5,1,false,1);a.product(0,3,0);
    a.add(3,5,6);a.add(13,13,3);a.multiply_high(0,4,0);a.shift_right(0,0,12);a.mask(3,0,1,false,1);
    a.add(3,0,3);a.offset(0,3,-1);a.compare_registers(13,0);
    if(a.nonpositive())return;
    a.floating_integer(1,31);a.value(4,0x14f90000);a.floating_integer(0,30);a.offset(4,4,-0x4a77);
    a.floating_store(1,1,0x80);a.floating_store(0,1,0x78);a.load(3,1,0x84);a.load(0,1,0x7c);
    a.sign16(3,3);a.subtract_from(3,3,10000);a.sign16(0,0);a.subtract_from(0,0,10000);a.product(0,3,0);
    a.multiply_high(0,4,0);a.shift_right(0,0,12);a.mask(3,0,1,false,1);a.add(3,0,3);a.offset(13,3,-1);
    a.jump(orientation_phase);
}

void sample_fitness(Call& a) {
    a.at(0x1023cdb4);a.floating_load(31,15,0x50,true);a.value(4,0x14f90000);a.floating_load(30,15,0x50,true);
    a.offset(4,4,-0x4a77);a.floating_integer(1,31);a.floating_integer(0,30);a.floating_store(1,1,0xc8);
    a.floating_store(0,1,0xc0);a.load(3,1,0xcc);a.load(0,1,0xc4);a.sign16(3,3);a.subtract_from(3,3,10000);
    a.sign16(0,0);a.subtract_from(0,0,10000);a.product(0,3,0);a.multiply_high(0,4,0);a.shift_right(0,0,12);
    a.mask(3,0,1,false,1);a.add_record(0,0,3);
    if(a.equal()){a.value(13,0);a.jump(orientation_phase);return;}
    a.at(0x1023ce0c);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    fitness_interval(a,0x1023ce34,true);bool wide=a.positive();
    if(!wide){fitness_interval(a,0x1023ce88,false);wide=!a.at_least();}
    if(wide)wide_fitness_sample(a);else narrow_fitness_sample(a);
}
}

void prepare(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,entry,dependencies,observer);
    a.link();a.floating_store(31,1,-8);a.floating_store(30,1,-16);a.floating_store(29,1,-24);a.multiple(13,false,-32);
    a.move(15,3);a.load(26,2,-0x51e8);a.move(16,4);a.load(27,2,-0x7b90);a.move(17,5);a.load(28,2,-0x7b8c);
    a.move(18,6);a.load(29,2,-0x7b88);a.value(23,0);a.load(30,2,-0x7b84);a.load(31,2,-0x51e0);
    a.save_link();a.value(0,0);a.frame(0x150);a.store<1>(1,0xd0,0);a.call(0x10245160);
    a.value(0,0);a.value(4,0);a.store<2>(1,0x4a,0);a.sign8(0,17);a.value(3,0);a.floating_move(29,1);
    a.store<1>(1,0x48,4);a.compare(0,-1);a.load(4,15,0x1a4);a.load<1>(22,4,0x791f);a.store<1>(1,0x42,3);
    if(a.equal()){a.load(3,15,0x1a4);a.load(4,15,0x1a4);a.load<1>(17,3,0x791d);a.load<1>(18,4,0x791e);}
    sample_fitness(a);state.pc=orientation_phase;
}
}
