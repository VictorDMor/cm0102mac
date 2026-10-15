#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call=SemanticGuestCall;

bool initiating_actor_area(Call& a,bool special) {
    const auto start=special?0x10244884:0x10244554,join=start+0xb8;
    a.at(start);a.load<1>(0,15,0x27);a.sign8(0,0,true);const bool right=a.equal();
    a.at(start+(right?0xc:0x64));a.load<1>(0,15,0x106);if(right)a.compare(0,11);else a.sign8(0,0,true);
    bool direct=a.equal();
    if(direct){a.load<1>(0,15,0x105);a.sign8(0,0);a.compare(0,3);direct=!a.less();}
    if(direct){a.load<1>(0,15,0x105);a.sign8(0,0);a.compare(0,5);direct=!a.positive();}
    if(direct){a.value(3,1);a.jump(join);}
    else {a.at(start+(right?0x40:0x98));a.floating_load(1,31,0,true);a.move(3,15);a.floating_load(2,31,0,true);a.value(4,4);a.value(5,right?11:0);
        a.value(6,0);a.value(9,0);a.call(0x10244c60);if(right)a.jump(join);}
    a.at(join);a.sign8(0,3,true);return !a.equal();
}

void react_receiver(Call& a,bool special) {
    a.at(special?0x10244944:0x10244614);a.load<1>(3,14,special?9:6);a.offset(0,3,1);a.store<1>(14,special?9:6,0);
    a.load<2>(3,14,0x36,true);a.offset(0,3,300);a.store<2>(14,0x36,0);a.load<1>(0,14,0x17);a.sign8(0,0);a.multiply(3,0,30);a.offset_record(16,3,70);
    const auto sampled=special?0x10244ad4:0x102447a4;
    if(a.equal()){a.value(13,0);a.jump(sampled);}else detail::sample_action_interval(a,special?0x10244978:0x10244648,16,13);
    a.at(sampled);a.compare(13,0);if(!a.equal())return;
    detail::advance_action_random(a,special?0x10244adc:0x102447ac);
    if(special) {
        a.load(4,30,0);a.move(3,14);a.load(0,27,0);a.load<2>(4,4,0);a.add(0,4,0);a.mask(4,0,1,false,1);a.mask(0,0,1);
        a.combine(0,0,4,false,true);a.subtract(0,4,0);a.sign8(4,0);
    } else {
        a.load(3,30,0);a.value(4,0x55550000);a.load(0,27,0);a.offset(4,4,0x5556);a.load<2>(5,3,0);a.move(3,14);a.add(5,5,0);
        a.multiply_high(4,4,5);a.mask(0,4,1,false,1);a.add(0,4,0);a.multiply(0,0,3);a.subtract(0,0,5);a.sign8(4,0);
    }
    a.call(0x10288f30);a.sign8(0,3,true);if(a.equal())return;
    a.move(3,14);a.value(4,0);a.call(0x1022ba10);if(!special)a.jump(receiver_final_reaction_phase);
}

void reward_regular_receiver(Call& a) {
    a.at(0x102444d0);a.load<1>(4,14,5);a.move(3,14);a.offset(0,4,1);a.store<1>(14,5,0);a.call(0x10289560);a.mask(0,3,4,true);bool single=!a.equal();
    if(!single){a.move(3,14);a.call(0x10289560);a.mask(0,3,2,true);single=!a.equal();}
    a.at(single?0x10244524:0x102444fc);a.sign8(0,17,true);a.load<2>(3,14,0x36,true);a.value(0,200);
    if(!a.equal())a.value(0,120);
    a.at(single?0x10244538:0x10244510);if(!single)a.mask(0,0,0xfffffffe,false,1);a.sign16(0,0);a.add(0,3,0);a.store<2>(14,0x36,0);
    if(!single)a.jump(0x10244544);
    a.at(0x10244544);a.move(3,14);a.call(0x10270ba0);a.sign8(0,3,true);if(a.equal())return;
    if(initiating_actor_area(a,false))react_receiver(a,false);
}

void reward_special_receiver(Call& a) {
    a.at(0x10244824);a.value(0,123);a.move(3,14);a.store<2>(14,0x19c,0);a.load<1>(4,14,7);a.offset(0,4,1);a.store<1>(14,7,0);
    a.load<1>(4,14,8);a.offset(0,4,1);a.store<1>(14,8,0);a.load<2>(4,14,0x36,true);a.offset(0,4,-10);a.store<2>(14,0x36,0);
    a.load<2>(4,14,0x36,true);a.offset(0,4,50);a.store<2>(14,0x36,0);a.load<1>(4,15,7);a.offset(0,4,1);a.store<1>(15,7,0);
    a.load<2>(4,15,0x36,true);a.offset(0,4,-10);a.store<2>(15,0x36,0);a.call(0x10270ba0);a.sign8(0,3,true);if(a.equal())return;
    if(initiating_actor_area(a,true))react_receiver(a,true);
}
}

void complete_receiver_outcome(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,receiver_effects_phase,dependencies,observer);a.load<1>(4,15,0x2c);a.sign8(0,17,true);a.load<1>(3,1,0x47);a.value(0,200);
    a.add(3,4,3);a.store<1>(15,0x2c,3);a.load<2>(4,14,0x36,true);if(!a.equal())a.value(0,120);
    a.at(0x102444b8);a.sign16(3,0);a.sign8(0,22);a.add(3,4,3);a.compare(0,4);a.store<2>(14,0x36,3);
    if(a.equal())reward_special_receiver(a);else reward_regular_receiver(a);state.pc=receiver_final_reaction_phase;
}

void react_to_receiver_outcome(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,receiver_final_reaction_phase,dependencies,observer);a.sign8(0,17,true);if(a.equal()){state.pc=return_phase;return;}
    detail::advance_action_random(a,0x10244b4c);a.load(4,30,0);a.value(3,0x92490000);a.load(0,27,0);a.offset(3,3,0x2493);a.load<2>(4,4,0);a.add(4,4,0);
    a.multiply_high(0,3,4);a.add(0,0,4);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,7);a.subtract(0,0,4,true);
    if(!a.equal()){state.pc=return_phase;return;}
    a.load(3,15,0x1a4);a.value(4,0x1f41);a.load<1>(7,14,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);a.value(9,0xffffffff);a.load<1>(6,14,0x2f);
    a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);a.jump(return_phase);state.pc=return_phase;
}

void report_missing_receiver(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,missing_receiver_phase,dependencies,observer);a.load(3,2,-0x51e4);a.offset(6,1,0x58);a.offset(7,1,0x50);a.value(4,0);a.offset(3,3,0x8f);
    a.value(5,0);a.call(0x10739890);a.call(0x10768810);a.load(4,2,-0x51e4);a.move(5,3);a.load(3,2,-0x7ba8);a.offset(6,1,0x58);
    a.offset(4,4,0x9c);a.offset(7,1,0x50);a.value(8,0x1ee6);a.call(0x1077b720);a.load(3,2,-0x51e4);a.value(5,0);a.load(4,2,-0x7ba8);
    a.offset(3,3,0xa9);a.call(0x10769b60);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);state.pc=return_phase;
}

void return_action_frame(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,return_phase,dependencies,observer);a.load(0,1,0x158);a.offset(1,1,0x150);a.floating_load(31,1,-8);a.floating_load(30,1,-16);a.floating_load(29,1,-24);
    a.restore_link();a.multiple(13,true,-32);a.finish();
}
}
