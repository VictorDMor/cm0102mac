#include "player_movement.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_possession_random(Call& a,std::uint32_t site) {
    a.at(site);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(site+0x28);
}
void sample_control_failure_wide(Call& a) {
    a.at(0x10231800);a.exclusive_flags(3,24,0x80000000);a.value(0,0x43300000);a.store(1,0xc0,3);a.value(6,0x80010000);
    a.load(7,30,0);a.value(4,0x43300000);a.store(1,0xbc,0);a.value(5,0x10000);a.floating_load(2,26,0x80);
    a.value(3,0x80010000);a.floating_load(1,1,0xbc);a.offset(6,6,-0x7fff);a.floating_load(0,26,0);
    a.offset(5,5,-1);a.floating_subtract(1,1,2);a.load<2>(8,7,0);a.load(7,27,0);a.multiply(0,24,10);
    a.offset(3,3,-0x7fff);a.store(1,0xc4,4);a.floating_difference(2,1,0,true);a.add(7,8,7);a.multiply_high(4,6,7);
    a.floating_load(1,26,0x80);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.add(4,4,6);
    a.product(4,4,5);a.subtract(23,4,7);a.exclusive_flags(4,23,0x80000000);a.store(1,0xc8,4);a.multiply_high(3,3,0);
    a.floating_load(0,1,0xc4);a.floating_subtract(0,0,1);a.add(0,3,0);a.shift_right(0,0,15);a.floating_product(0,2,0);
    a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,0xcc);a.load(23,1,0xd0);
    a.call(0x10226520);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,24,-1);
    a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(23,23,3);a.compare_registers(23,0);
    if(!a.nonpositive()){a.offset(23,24,-1);a.jump(0x10231914);}
}
void sample_control_failure(Call& a) {
    a.floating_load(2,31,0x3c,true);a.floating_load(1,15,0xe8,true);a.floating_load(0,31,0x90,true);
    a.floating_product_sum(0,2,1,0);a.floating_integer(0,0);a.floating_store(0,1,0xbc);a.load(24,1,0xc0);a.compare(24,0);
    if(a.equal()){a.value(23,0);a.jump(0x10231914);return;}
    advance_possession_random(a,0x102317b8);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(24,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(24,3);wide=!a.at_least();}
    if(wide)sample_control_failure_wide(a);
    else {a.at(0x102318e4);a.load(3,30,0);a.compare(24,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
        a.divide(0,3,24);a.product(0,0,24);a.subtract(23,0,3);a.negate(0,23);if(!a.nonpositive())a.move(0,23);
        a.at(0x10231910);a.move(23,0);}
}
bool possession_should_continue(Call& a) {
    a.load<1>(0,15,0x1d);a.sign8(0,0,true);if(!a.equal())return false;
    a.sign8(0,20);a.compare(0,4);if(a.equal())return true;
    a.sign16(0,21);a.compare(0,1);if(a.at_least())return true;
    sample_control_failure(a);a.at(0x10231914);a.compare(23,0);if(!a.equal())return true;
    a.compare(22,0,false);if(a.equal())return false;
    a.load<2>(3,1,0x48,true);a.sign16(0,21);a.compare_registers(3,0);return a.at_least();
}
void release_possession(Call& a) {
    a.at(0x10231934);a.load(3,15,0x1a4);a.value(0,0);a.value(5,UINT32_MAX);a.value(7,0);a.store(3,0x310c,0);
    a.value(0,1);a.value(6,0);a.value(4,0);a.load(8,15,0x1a4);a.value(3,0);a.store<1>(1,0x114,0);
    a.store<1>(8,0x7924,5);a.load(5,15,0x1a4);a.offset(5,5,0x10000);a.store(5,-0x2000,7);a.load(5,15,0x1a4);
    a.store<1>(5,0x1c0,6);a.store<2>(15,0x2a,4);a.store<2>(15,0x1a0,3);
    advance_possession_random(a,0x10231980);a.load(3,30,0);a.value(4,0x1f42);a.load(0,27,0);a.value(8,UINT32_MAX);
    a.load<2>(3,3,0);a.value(9,UINT32_MAX);a.value(10,0);a.add(0,3,0);a.mask(3,0,1,false,1);a.mask(0,0,1);
    a.combine(0,0,3,false,true);a.subtract(3,3,0);a.offset(0,3,1);a.store<1>(15,0x2c,0);
    a.load<2>(3,15,0x36,true);a.offset(0,3,-150);a.store<2>(15,0x36,0);a.load(3,15,0x1a4);a.load<1>(7,15,0x27);
    a.load(3,3,0x79f4);a.load<1>(6,15,0x2f);a.load(5,15,0x1a4);a.call(0x10201180);
}
void sample_release_reaction_wide(Call& a) {
    a.at(0x10231a6c);a.exclusive_flags(3,14,0x80000000);a.value(0,0x43300000);a.store(1,0xc0,3);a.value(6,0x80010000);
    a.load(7,30,0);a.value(4,0x43300000);a.store(1,0xbc,0);a.value(5,0x10000);a.floating_load(2,26,0x80);
    a.value(3,0x80010000);a.floating_load(1,1,0xbc);a.offset(6,6,-0x7fff);a.floating_load(0,26,0);
    a.offset(5,5,-1);a.floating_subtract(1,1,2);a.load<2>(8,7,0);a.load(7,27,0);a.multiply(0,14,10);
    a.offset(3,3,-0x7fff);a.store(1,0xc4,4);a.floating_difference(2,1,0,true);a.add(7,8,7);a.multiply_high(4,6,7);
    a.floating_load(1,26,0x80);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.add(4,4,6);
    a.product(4,4,5);a.subtract(13,4,7);a.exclusive_flags(4,13,0x80000000);a.store(1,0xc8,4);a.multiply_high(3,3,0);
    a.floating_load(0,1,0xc4);a.floating_subtract(0,0,1);a.add(0,3,0);a.shift_right(0,0,15);a.floating_product(0,2,0);
    a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,0xcc);a.load(13,1,0xd0);
    a.call(0x10226520);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,14,-1);
    a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(13,13,3);a.compare_registers(13,0);
    if(!a.nonpositive()){a.offset(13,14,-1);a.jump(0x10231b80);}
}
void sample_release_reaction(Call& a) {
    a.load(3,15,0x6c);a.load<1>(0,3,0x66);a.sign8(0,0);a.multiply(3,0,20);a.offset_record(14,3,20);
    if(a.equal()){a.value(13,0);a.jump(0x10231b80);return;}
    advance_possession_random(a,0x10231a24);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(14,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(14,3);wide=!a.at_least();}
    if(wide)sample_release_reaction_wide(a);
    else {a.at(0x10231b50);a.load(3,30,0);a.compare(14,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
        a.divide(0,3,14);a.product(0,0,14);a.subtract(13,0,3);a.negate(0,13);if(!a.nonpositive())a.move(0,13);
        a.at(0x10231b7c);a.move(13,0);}
}
void apply_release_reaction(Call& a) {
    sample_release_reaction(a);a.at(0x10231b80);a.compare(13,0);if(!a.equal())return;
    advance_possession_random(a,0x10231b88);a.load(3,30,0);a.value(4,0x55550000);a.load(0,27,0);a.offset(4,4,0x5556);
    a.load<2>(5,3,0);a.move(3,15);a.add(5,5,0);a.multiply_high(4,4,5);a.mask(0,4,1,false,1);a.add(0,4,0);
    a.multiply(0,0,3);a.subtract(0,0,5);a.negate(0,0);a.sign8(4,0);a.call(0x10288f30);a.sign8(0,3,true);if(a.equal())return;
    a.move(3,15);a.value(4,0);a.call(0x1022ba10);a.jump(noncarrier_phase);
}
}
void continue_carrier_possession(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_no_contact_phase,dependencies,observer);
    if(possession_should_continue(a)){state.pc=carrier_play_decision_phase;return;}
    release_possession(a);apply_release_reaction(a);state.pc=noncarrier_phase;
}
}
