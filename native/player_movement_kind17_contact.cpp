#include "player_movement.hpp"
#include "semantic_guest_call.hpp"
#include "player_movement_internal.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_contact_random(Call& a,std::uint32_t site) {
    a.at(site);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(site+0x28);
}
void describe_actor_action(Call& a,std::uint32_t site,std::uint32_t code) {
    a.at(site);a.load(3,15,0x1a4);a.value(4,code);a.load<1>(7,15,0x27);a.value(8,UINT32_MAX);a.load(3,3,0x79f4);
    a.value(9,UINT32_MAX);a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);a.jump(noncarrier_phase);
}
void describe_opposed_action(Call& a,std::uint32_t site,std::uint32_t code,bool branch=true,std::uint32_t delta=0) {
    a.at(site);a.load(3,15,0x1a4);a.value(4,code);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
    a.load<1>(6,15,0x2f);a.load<1>(7,15,0x27);a.load<1>(8,22,0x2f);a.load<1>(9,22,0x27);a.call(0x10201180);
    if(branch)a.jump(0x102326f0+delta);
}
void query_opposite_zone(Call& a,std::uint32_t site) {
    a.at(site);a.load<1>(0,15,0x27);a.move(3,15);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);
    a.sign8(4,0);a.call(0x10239d10);a.sign8(0,3,true);
}
bool prepare_kind17_action(Call& a) {
    query_opposite_zone(a,carrier_kind17_phase);bool restricted=!a.equal();
    if(!restricted){a.move(3,15);a.call(0x102721a0);a.sign8(0,3,true);restricted=!a.equal();}
    if(restricted){a.at(0x10232388);a.value(19,0);}
    a.at(0x1023238c);a.sign8(0,20);a.compare(0,4);if(!a.equal())return true;
    a.load<2>(0,15,0x19c,true);a.compare(0,15);
    if(a.equal())describe_actor_action(a,0x102323a4,0x20d2);else describe_actor_action(a,0x102323d0,0x2009);return false;
}
void sample_opposed_skill_wide(Call& a,std::uint32_t delta=0) {
    a.at((0x102324a8+delta));a.exclusive_flags(3,14,0x80000000);a.value(0,0x43300000);a.store(1,0xc0,3);a.value(6,0x80010000);
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
    if(!a.nonpositive()){a.offset(13,14,-1);a.jump((0x102325bc+delta));}
}
void sample_opposed_skill(Call& a,std::uint32_t delta=0) {
    a.at((0x10232440+delta));a.floating_load(0,15,0x98,true);a.floating_integer(0,0);a.floating_store(0,1,0xbc);a.load(14,1,0xc0);a.compare(14,0);
    if(a.equal()){a.value(13,0);a.jump((0x102325bc+delta));return;}
    advance_contact_random(a,(0x10232460+delta));a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(14,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(14,3);wide=!a.at_least();}
    if(wide)sample_opposed_skill_wide(a,delta);
    else {a.at((0x1023258c+delta));a.load(3,30,0);a.compare(14,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
        a.divide(0,3,14);a.product(0,0,14);a.subtract(13,0,3);a.negate(0,13);if(!a.nonpositive())a.move(0,13);
        a.at((0x102325b8+delta));a.move(13,0);}
}
bool opposed_skill_is_distinct(Call& a) {
    a.floating_load(1,15,0x74,true);a.floating_load(0,31,0x94,true);a.floating_compare(1,0);if(a.positive())return true;
    a.floating_load(2,15,0x74,true);a.floating_load(1,22,0x74,true);a.floating_load(0,31,0x5c,true);
    a.floating_subtract(1,2,1,true);a.floating_compare(1,0);return !a.nonpositive();
}
void describe_opposition(Call& a,std::uint32_t delta=0) {
    a.move(3,22);a.call(0x10289560);a.sign16(0,3);a.compare(0,1);
    if(a.equal()){query_opposite_zone(a,(0x10232678+delta));
        if(a.equal())describe_opposed_action(a,(0x1023269c+delta),0x2158,true,delta);else describe_opposed_action(a,(0x102326c8+delta),0x1f55,false,delta);return;}
    if(!opposed_skill_is_distinct(a)){describe_opposed_action(a,(0x1023264c+delta),0x1f4a,true,delta);return;}
    sample_opposed_skill(a,delta);advance_contact_random(a,(0x102325bc+delta));a.load(4,30,0);a.value(3,0x88890000);a.load(0,27,0);a.offset(3,3,-0x7777);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.add(0,0,4);a.shift_right(0,0,4);a.mask(3,0,1,false,1);
    a.add(0,0,3);a.multiply(0,0,30);a.subtract(0,0,4);a.compare_registers(13,0);
    if(a.at_least())describe_opposed_action(a,(0x1023264c+delta),0x1f4a,true,delta);else describe_opposed_action(a,(0x10232620+delta),0x2173,true,delta);
}
void consider_opposed_space(Call& a) {
    a.at(0x102326f0);a.sign8(0,19,true);if(a.equal())return;
    a.load<1>(0,15,0x27);a.value(7,1);a.load(3,15,0x1a4);a.sign8(0,0);a.load<1>(5,15,0x105);a.leading_zeroes(0,0);
    a.load<1>(6,15,0x106);a.mask(0,0,0x07ffffff,false,27);a.sign8(4,0);a.call(0x101e3f10);a.floating_load(0,26,0x90);
    a.floating_compare(1,0);if(a.nonpositive())return;
    a.load<1>(0,15,0x27);a.load(3,15,0x1a4);a.sign8(0,0);a.leading_zeroes(0,0);a.offset(3,3,0x10000);
    a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0);a.multiply(0,0,0x18e0);a.add(3,3,0);a.load(0,3,-0x7e0c);a.mask(0,0,0x400,true);
    if(a.equal())describe_actor_action(a,0x10232788,0x1f45);else describe_actor_action(a,0x1023275c,0x20de);
}
}
void describe_carrier_opposition(SemanticGuestCall& a,std::uint32_t delta){describe_opposition(a,delta);}
void continue_carrier_kind17_contact(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_kind17_phase,dependencies,observer);
    if(!prepare_kind17_action(a)){state.pc=noncarrier_phase;return;}
    a.at(0x102323fc);a.compare(22,0,false);if(a.equal()){state.pc=carrier_kind17_clear_phase;return;}
    describe_opposition(a);consider_opposed_space(a);state.pc=noncarrier_phase;
}
}
