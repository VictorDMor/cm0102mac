#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

bool clear_lane_action(Call& a) {
    a.load<1>(0,15,0x27);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);
    a.sign8(0,0);a.compare(0,1);const auto right=a.equal();
    detail::area_coordinates(a,right?0x1023d920:0x1023d970,0x1023d9b8,right,0,3,0);
    a.at(0x1023d9b8);a.sign8(0,0,true);
    if(!a.equal())return false;
    a.load<1>(0,15,0x10f);a.sign16(3,16);a.sign8(0,0);a.subtract_from(0,0,20);a.multiply(0,0,7);a.compare_registers(3,0);
    if(a.nonpositive())return false;
    a.load(3,15,0x1a4);a.load<1>(4,15,0x27);a.load<1>(5,15,0x105);a.load<1>(6,15,0x106);a.call(0x101e4ed0);
    a.sign8(0,3,true);if(a.equal())return false;
    a.value(13,12);a.load<1>(3,15,0x10f);a.sign8(0,13);a.load(5,15,0x1a4);a.mask(0,0,0xfffffffe,false,1);
    a.sign8(4,3);a.add(3,15,0);a.value(23,1);a.load<2>(3,3,0x11e,true);a.value(0,2);a.subtract_from(4,4,20);
    a.store<1>(5,0x791f,0);a.multiply(4,4,10);a.load(5,15,0x1a4);a.subtract(0,4,3);a.load<1>(22,5,0x791f);
    a.sign16(17,0);a.jump(selected_action_phase);return true;
}

void sample_elevated_preference(Call& a) {
    a.at(0x1023da40);a.load<1>(0,15,0x27);a.compare(0,1);const auto right=a.equal();
    detail::area_coordinates(a,right?0x1023da4c:0x1023da9c,0x1023dae4,right,0,3,13);
    a.at(0x1023dae4);a.move(3,15);a.value(4,0);a.call(0x102875f0);a.sign8(17,3,true);
    if(a.equal()){a.value(14,0);a.jump(0x1023dc5c);}
    else detail::sample_action_interval(a,0x1023db00,17,14);
}

void sample_elevated_threshold(Call& a) {
    a.at(0x1023dc5c);a.load<1>(3,15,0x24);a.sign8(0,13);a.multiply(0,0,1500);a.sign8(3,3);
    a.multiply(3,3,80);a.add(17,0,3);a.offset_record(17,17,70);
    if(a.equal()){a.value(13,0);a.jump(0x1023dde0);}
    else detail::sample_action_interval(a,0x1023dc84,17,13);
}
}

void select_elevated_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,ball_state_selection_phase,dependencies,observer);a.sign8(0,22);a.compare(0,4);
    if(a.less()){state.pc=low_ball_selection_phase;return;}
    if(clear_lane_action(a)){state.pc=selected_action_phase;return;}
    sample_elevated_preference(a);sample_elevated_threshold(a);a.at(0x1023dde0);a.compare_registers(14,13);
    if(a.nonpositive()){a.at(0x1023de00);a.value(13,21);}else a.value(13,46);
    a.sign8(0,13);a.mask(0,0,0xfffffffe,false,1);a.add(3,15,0);a.load<2>(17,3,0x11e,true);a.jump(selected_action_phase);
    state.pc=selected_action_phase;
}
}
