#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

void target_goal_alignment(Call& a) {
    a.at(0x1023de30);a.load<1>(0,15,0x27);a.sign8(0,0,true);const bool right=a.equal();
    bool goal_cell;
    if(right){a.load<1>(0,15,0x106);a.compare(0,11);goal_cell=a.equal();}
    else {a.at(0x1023de94);a.load<1>(0,15,0x106);a.sign8(0,0,true);goal_cell=a.equal();}
    if(goal_cell){a.load<1>(0,15,0x105);a.sign8(0,0);a.compare(0,3);goal_cell=!a.less();}
    if(goal_cell){a.load<1>(0,15,0x105);a.sign8(0,0);a.compare(0,5);goal_cell=!a.positive();}
    if(goal_cell){a.value(3,1);a.jump(0x1023dee8);}
    else {
        a.at(right?0x1023de70:0x1023dec8);a.floating_load(1,31,0,true);a.move(3,15);a.floating_load(2,31,0,true);
        a.value(4,4);a.value(5,right?11:0);a.value(6,0);a.value(9,0);a.call(0x10244c60);
        if(right)a.jump(0x1023dee8);
    }
}

bool low_ball_competition(Call& a) {
    a.at(0x1023defc);a.load<2>(14,15,0x194,true);a.compare(14,0);
    if(a.equal()){a.value(13,0);a.jump(0x1023e06c);}
    else detail::sample_action_interval(a,0x1023df10,14,13);
    a.at(0x1023e06c);a.load<2>(17,15,0x136,true);a.compare(17,0);
    if(a.equal()){a.value(14,0);a.jump(0x1023e1dc);}
    else detail::sample_action_interval(a,0x1023e080,17,14);
    a.at(0x1023e1dc);a.compare_registers(13,14);return !a.nonpositive();
}

bool low_ball_attempt(Call& a) {
    a.floating_load(0,15,0x9c,true);a.floating_integer(0,0);a.floating_store(0,1,0x78);a.load(14,1,0x7c);a.compare(14,0);
    if(a.equal()){a.value(13,0);a.jump(0x1023e360);}
    else detail::sample_action_interval(a,0x1023e204,14,13);
    a.at(0x1023e360);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(0x1023e388);a.load(4,30,0);a.value(3,0x66660000);a.load(0,27,0);a.offset(3,3,0x6667);a.load<2>(4,4,0);
    a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);
    a.multiply(0,0,5);a.subtract(0,0,4);a.compare_registers(13,0);return !a.less();
}

void select_kind(Call& a,std::uint32_t site,unsigned kind,bool jump) {
    a.at(site);a.value(13,kind);a.sign8(0,13);a.mask(0,0,0xfffffffe,false,1);a.add(3,15,0);a.load<2>(17,3,0x11e,true);
    if(jump)a.jump(selected_action_phase);
}
}

void select_low_ball_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,low_ball_selection_phase,dependencies,observer);a.sign8(0,22);a.compare(0,3);bool eligible=a.equal();
    if(!eligible){a.sign8(0,22);a.compare(0,2);eligible=a.equal();}
    if(!eligible)select_kind(a,0x1023e3f0,13,false);
    else {
        target_goal_alignment(a);a.at(0x1023dee8);a.sign8(0,3,true);bool attempt=!a.equal();
        if(attempt){a.floating_load(0,26,0x328);a.floating_compare(29,0);attempt=!a.at_least();}
        if(attempt)attempt=low_ball_competition(a);
        if(attempt)attempt=low_ball_attempt(a);
        select_kind(a,attempt?0x1023e3c0:0x1023e3d8,attempt?59:12,true);
    }
    state.pc=selected_action_phase;
}
}
