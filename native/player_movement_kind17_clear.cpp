#include "player_movement.hpp"
#include "semantic_guest_call.hpp"
#include "player_movement_internal.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void describe_clear_action(Call& a,std::uint32_t site,std::uint32_t code) {
    a.at(site);a.load(3,15,0x1a4);a.value(4,code);a.load<1>(7,15,0x27);a.value(8,UINT32_MAX);a.load(3,3,0x79f4);
    a.value(9,UINT32_MAX);a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);a.jump(noncarrier_phase);
}
bool describe_space_pressure(Call& a) {
    a.sign8(0,19,true);if(a.equal())return false;
    a.load<1>(0,15,0x27);a.value(7,1);a.load(3,15,0x1a4);a.sign8(0,0);a.load<1>(5,15,0x105);a.leading_zeroes(0,0);
    a.load<1>(6,15,0x106);a.mask(0,0,0x07ffffff,false,27);a.sign8(4,0);a.call(0x101e3f10);a.floating_load(0,26,0x90);
    a.floating_compare(1,0);if(a.nonpositive())return false;
    a.load<1>(0,15,0x27);a.load(3,15,0x1a4);a.sign8(0,0);a.leading_zeroes(0,0);a.offset(3,3,0x10000);
    a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0);a.multiply(0,0,0x18e0);a.add(3,3,0);a.load(0,3,-0x7e0c);a.mask(0,0,0x400,true);
    if(a.equal())describe_clear_action(a,0x1023284c,0x1f45);else describe_clear_action(a,0x10232820,0x20de);return true;
}
bool describe_zone_crossing(Call& a,std::uint32_t delta=0) {
    a.at((0x10232878+delta));a.load<1>(0,15,0x27);a.move(3,15);a.move(5,16);a.move(6,17);a.sign8(0,0);
    a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(4,0);a.call(0x10239dc0);a.sign8(0,3,true);if(a.equal())return false;
    a.load<1>(0,15,0x27);a.move(3,15);a.load<1>(5,15,0x105);a.sign8(0,0);a.load<1>(6,15,0x106);
    a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(4,0);a.call(0x10239dc0);a.sign8(0,3,true);if(!a.equal())return false;
    describe_clear_action(a,(0x102328d0+delta),0x1f50);return true;
}
bool query_x_gate(Call& a,std::uint32_t site) {
    a.at(site);a.load(3,15,0x1a4);a.move(4,16);a.call(0x101ee9c0);a.sign8(0,3,true);return !a.equal();
}
bool reaches_goal_line(Call& a,std::uint32_t delta=0) {
    a.load<1>(0,15,0x27);a.sign8(0,0,true);
    if(a.equal()){a.sign8(0,17);a.compare(0,11);
        if(a.equal()){a.load<1>(0,15,0x106);a.sign8(0,0);a.compare(0,11);if(a.less())return true;}}
    a.at((0x10232938+delta));a.load<1>(0,15,0x27);a.compare(0,1);if(!a.equal())return false;
    a.sign8(0,17,true);if(!a.equal())return false;a.load<1>(0,15,0x106);a.sign8(0,0,true);return !a.nonpositive();
}
bool crosses_centre_line(Call& a,std::uint32_t delta=0) {
    a.at((0x10232984+delta));a.load<1>(0,15,0x27);a.sign8(0,0,true);
    if(a.equal()){a.sign8(0,17);a.compare(0,6);
        if(a.equal()){a.load<1>(0,15,0x106);a.sign8(0,0);a.compare(0,6);if(a.less())return true;}}
    a.at((0x102329ac+delta));a.load<1>(0,15,0x27);a.compare(0,1);if(!a.equal())return false;
    a.sign8(0,17);a.compare(0,5);if(!a.equal())return false;a.load<1>(0,15,0x106);a.sign8(0,0);a.compare(0,5);return !a.nonpositive();
}
bool moves_forward(Call& a,std::uint32_t delta=0) {
    a.load<1>(0,15,0x27);a.sign8(0,0,true);
    if(a.equal()){a.load<1>(0,15,0x106);a.sign8(3,17);a.sign8(0,0);a.compare_registers(3,0);if(a.less())return true;}
    a.at((0x10232a30+delta));a.load<1>(0,15,0x27);a.compare(0,1);if(!a.equal())return false;
    a.load<1>(0,15,0x106);a.sign8(3,17);a.sign8(0,0);a.compare_registers(3,0);return !a.nonpositive();
}
bool query_actor_trait(Call& a,std::uint32_t site,std::uint32_t target) {
    a.at(site);a.move(3,15);a.call(target);a.sign8(0,3,true);return !a.equal();
}
bool moves_to_outer_flank(Call& a,std::uint32_t site,bool left) {
    a.at(site);a.load<1>(0,15,0x27);a.sign8(0,0,true);
    if(a.equal()){a.load<1>(0,15,0x105);a.sign8(3,16);a.sign8(0,0);a.compare_registers(3,0);
        const bool direction=left?!a.at_least():!a.nonpositive();
        if(direction){a.sign8(0,16);a.compare(0,left?2:6);if(left?a.less():a.positive())return true;}}
    a.at(site+0x2c);a.load<1>(0,15,0x27);a.compare(0,1);if(!a.equal())return false;
    a.load<1>(0,15,0x105);a.sign8(3,16);a.sign8(0,0);a.compare_registers(3,0);
    if(left?a.nonpositive():a.at_least())return false;
    a.sign8(0,16);a.compare(0,left?6:2);return left?!a.nonpositive():!a.at_least();
}
bool describe_positional_change(Call& a,std::uint32_t delta=0) {
    if(query_x_gate(a,(0x102328fc+delta)) && reaches_goal_line(a,delta)){describe_clear_action(a,(0x10232958+delta),0x2156);return true;}
    if(crosses_centre_line(a,delta)){describe_clear_action(a,(0x102329d4+delta),0x1f51);return true;}
    if(query_actor_trait(a,(0x10232a00+delta),0x102721a0) && moves_forward(a,delta)){describe_clear_action(a,(0x10232a50+delta),0x1f54);return true;}
    if(query_actor_trait(a,(0x10232a7c+delta),0x10272100) && query_actor_trait(a,(0x10232a8c+delta),0x102720d0) && !query_x_gate(a,(0x10232a9c+delta))){
        describe_clear_action(a,(0x10232ab0+delta),0x1f4e);return true;}
    if(query_x_gate(a,(0x10232adc+delta)) && moves_to_outer_flank(a,(0x10232af0+delta),true)){describe_clear_action(a,(0x10232b48+delta),0x1f52);return true;}
    if(query_x_gate(a,(0x10232b74+delta)) && moves_to_outer_flank(a,(0x10232b88+delta),false)){describe_clear_action(a,(0x10232be0+delta),0x1f53);return true;}
    return false;
}
bool describe_stationary_attachment(Call& a,std::uint32_t delta=0) {
    a.at((0x10232c0c+delta));a.load(0,15,0x1b4);a.compare(0,0,false);if(a.equal())return false;
    a.load(3,15,0x1b4);a.load<1>(0,15,0x105);a.load<1>(4,3,0x105);a.sign8(3,0);a.sign8(0,4);a.compare_registers(3,0);if(!a.equal())return false;
    a.load(3,15,0x1b4);a.load<1>(0,15,0x106);a.load<1>(4,3,0x106);a.sign8(3,0);a.sign8(0,4);a.compare_registers(3,0);if(!a.equal())return false;
    a.load(4,15,0x1b4);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.load<1>(13,4,0x10d);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at((0x10232c80+delta));a.load(5,30,0);a.value(3,0x55550000);a.load(4,27,0);a.offset(3,3,0x5556);a.load<2>(5,5,0);
    a.sign8(0,13);a.add(5,5,4);a.multiply_high(4,3,5);a.mask(3,4,1,false,1);a.add(3,4,3);a.multiply(3,3,3);
    a.subtract(3,3,5);a.compare_registers(0,3);if(a.nonpositive())return false;
    a.load(3,15,0x1b4);a.compare(3,0,false);
    if(!a.equal()){a.load<1>(0,3,0x2c);a.jump((0x10232cd0+delta));}else {a.at((0x10232ccc+delta));a.value(0,0);}
    a.at((0x10232cd0+delta));a.sign8(0,0);a.compare(0,2);if(a.at_least())return false;
    a.load<1>(0,15,0x27);a.value(4,0x2172);a.load(3,15,0x1a4);a.value(10,0);a.load(8,15,0x1b4);a.sign8(0,0);
    a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);a.load(5,15,0x1a4);a.load<1>(6,15,0x2f);
    a.sign8(9,0);a.load<1>(7,15,0x27);a.load<1>(8,8,0x2f);a.call(0x10201180);a.jump(noncarrier_phase);return true;
}
void describe_clear_fallback(Call& a,std::uint32_t delta=0) {
    a.at((0x10232d1c+delta));a.load<2>(0,15,0x19c,true);a.compare(0,18);bool active=a.equal();
    if(!active){a.load<2>(0,15,0x19c,true);a.compare(0,17);active=a.equal();}
    if(active){describe_clear_action(a,(0x10232d34+delta),0x1f4c);return;}
    a.at((0x10232d60+delta));a.load<1>(0,15,0x27);a.move(3,15);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);
    a.sign8(4,0);a.call(0x10239d10);a.sign8(0,3,true);if(!a.equal()){describe_clear_action(a,(0x10232d84+delta),0x20d1);return;}
    a.at((0x10232db0+delta));a.load<1>(0,15,0x27);a.load(3,15,0x1a4);a.sign8(0,0);a.load<1>(5,15,0x106);
    a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(4,0);a.call(0x101ee9f0);a.sign8(0,3,true);
    if(a.equal())describe_clear_action(a,(0x10232e04+delta),0x20d2);else describe_clear_action(a,(0x10232dd8+delta),0x20d3);
}
}
void complete_carrier_clear_position(SemanticGuestCall& a,std::uint32_t delta){
    if(!describe_zone_crossing(a,delta) && !describe_positional_change(a,delta) && !describe_stationary_attachment(a,delta))describe_clear_fallback(a,delta);
}
void continue_carrier_kind17_clear(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_kind17_clear_phase,dependencies,observer);
    if(!describe_space_pressure(a))complete_carrier_clear_position(a,0);
    state.pc=noncarrier_phase;
}
}
