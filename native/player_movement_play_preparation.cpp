#include "player_movement.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_preparation_random(Call& a) {
    a.at(0x10231c54);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(0x10231c7c);
}
void sample_play_condition_wide(Call& a) {
    a.at(0x10231c9c);a.exclusive_flags(3,23,0x80000000);a.value(0,0x43300000);a.store(1,0xc0,3);a.value(6,0x80010000);
    a.load(7,30,0);a.value(4,0x43300000);a.store(1,0xbc,0);a.value(5,0x10000);a.floating_load(2,26,0x80);
    a.value(3,0x80010000);a.floating_load(1,1,0xbc);a.offset(6,6,-0x7fff);a.floating_load(0,26,0);
    a.offset(5,5,-1);a.floating_subtract(1,1,2);a.load<2>(8,7,0);a.load(7,27,0);a.multiply(0,23,10);
    a.offset(3,3,-0x7fff);a.store(1,0xc4,4);a.floating_difference(2,1,0,true);a.add(7,8,7);a.multiply_high(4,6,7);
    a.floating_load(1,26,0x80);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.add(4,4,6);
    a.product(4,4,5);a.subtract(13,4,7);a.exclusive_flags(4,13,0x80000000);a.store(1,0xc8,4);a.multiply_high(3,3,0);
    a.floating_load(0,1,0xc4);a.floating_subtract(0,0,1);a.add(0,3,0);a.shift_right(0,0,15);a.floating_product(0,2,0);
    a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,0xcc);a.load(13,1,0xd0);
    a.call(0x10226520);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,23,-1);
    a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(13,13,3);a.compare_registers(13,0);
    if(!a.nonpositive()){a.offset(13,23,-1);a.jump(0x10231db0);}
}
void sample_play_condition(Call& a) {
    a.load<2>(23,15,0x32,true);a.compare(23,0);
    if(a.equal()){a.value(13,0);a.jump(0x10231db0);return;}
    advance_preparation_random(a);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(23,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(23,3);wide=!a.at_least();}
    if(wide)sample_play_condition_wide(a);
    else {a.at(0x10231d80);a.load(3,30,0);a.compare(23,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
        a.divide(0,3,23);a.product(0,0,23);a.subtract(13,0,3);a.negate(0,13);if(!a.nonpositive())a.move(0,13);
        a.at(0x10231dac);a.move(13,0);}
}
void consider_play_condition_adjustment(Call& a) {
    a.at(0x10231c1c);a.load<1>(0,15,0x27);a.move(3,15);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);
    a.sign8(4,0);a.call(0x10239d10);a.sign8(0,3,true);if(!a.equal())return;
    sample_play_condition(a);a.at(0x10231db0);a.compare(13,-10);if(a.at_least())return;
    a.floating_load(1,31,0x18,true);a.move(3,15);a.call(0x102865a0);
    a.load<1>(0,15,0x27);a.value(4,0x211d);a.load(3,15,0x1a4);a.value(6,UINT32_MAX);a.sign8(0,0);a.load(5,15,0x1a4);
    a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);a.value(8,UINT32_MAX);
    a.sign8(7,0);a.value(9,UINT32_MAX);a.value(10,0);a.call(0x10201180);
}
void update_team_play_counters(Call& a) {
    a.at(0x10231dfc);a.load<1>(6,15,0x27);a.load(3,15,0x1a4);a.sign8(0,6);a.compare(0,-1);a.store<1>(3,0x7921,6);
    if(a.equal())return;
    a.sign8(4,6);a.sign8(0,6);a.add(5,3,4);a.load<1>(4,5,0x77bc);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);
    a.offset(4,4,1);a.store<1>(5,0x77bc,4);a.add(4,3,0);a.load<1>(0,4,0x77bc);a.sign8(0,0,true);if(a.nonpositive())return;
    a.sign8(0,6);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.add(4,3,0);a.load<1>(3,4,0x77bc);
    a.offset(0,3,-10);a.store<1>(4,0x77bc,0);
}
}
void continue_carrier_play_preparation(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_play_decision_phase,dependencies,observer);a.compare(22,0,false);
    if(!a.equal()){a.load<2>(0,1,0x48,true);a.sign16(3,21);a.compare_registers(3,0);
        if(a.nonpositive()){state.pc=carrier_repeated_contact_phase;return;}}
    consider_play_condition_adjustment(a);update_team_play_counters(a);state.pc=carrier_play_ready_phase;
}
}
