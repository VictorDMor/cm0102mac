#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

bool area_status_exempts_action(Call& a) {
    a.load<1>(0,15,0x27);a.compare(0,1);
    const auto right=a.equal();
    detail::area_coordinates(a,right?0x1023d5f0:0x1023d640,0x1023d688,right,0,3,0);
    a.at(0x1023d688);a.sign8(0,0,true);
    if(a.equal())return false;
    a.load(3,15,0x1a4);a.move(4,15);a.call(0x101e3d50);a.sign16(0,3);a.compare(0,1);
    return a.equal();
}

void sample_area_reaction(Call& a) {
    a.at(0x1023d6a8);a.load(3,15,0x6c);a.load<1>(0,15,0x27);a.load<1>(3,3,0x68);a.sign8(0,0);
    a.sign8(3,3);a.compare(0,1);a.multiply(0,3,200);
    const auto right=a.equal();
    detail::area_coordinates(a,right?0x1023d6c8:0x1023d718,0x1023d760,right,3,4,3);
    a.at(0x1023d760);a.sign8(3,3);a.multiply(3,3,500);a.add(14,0,3);a.offset_record(14,14,100);
    if(a.equal()){a.value(13,0);a.jump(0x1023d8d8);}
    else detail::sample_action_interval(a,0x1023d77c,14,13);
}
}

void select_area_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,area_selection_phase,dependencies,observer);
    if(area_status_exempts_action(a)){state.pc=ball_state_selection_phase;return;}
    sample_area_reaction(a);a.at(0x1023d8d8);a.compare(13,0);
    if(!a.equal()){state.pc=ball_state_selection_phase;return;}
    a.value(13,37);a.sign8(0,13);a.mask(0,0,0xfffffffe,false,1);a.add(3,15,0);a.load<2>(17,3,0x11e,true);
    a.jump(selected_action_phase);state.pc=selected_action_phase;
}
}
