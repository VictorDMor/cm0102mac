#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call=SemanticGuestCall;

void emit_receiver_pair(Call& a,std::uint32_t start,unsigned message,std::uint32_t join,bool jump=true) {
    a.at(start);a.load(3,15,0x1a4);a.value(4,message);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
    a.load<1>(6,14,0x2f);a.load<1>(7,14,0x27);a.load<1>(8,15,0x2f);a.load<1>(9,15,0x27);a.call(0x10201180);
    if(jump)a.jump(join);
}

bool needs_failed_area_report(Call& a) {
    a.load(3,15,0x1a4);a.load<1>(4,1,0x48);a.call(0x101ee530);a.sign8(0,3,true);if(!a.equal())return true;
    a.load<1>(0,1,0x42);a.sign8(0,0,true);if(!a.equal())return true;
    a.load<2>(16,14,0x32,true);a.compare(16,0);
    if(a.equal()){a.value(13,0);a.jump(0x1024356c);}else detail::sample_action_interval(a,0x10243410,16,13);
    a.at(0x1024356c);a.compare(13,-10);return !a.at_least();
}
}

void prepare_failed_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,unsuccessful_action_phase,dependencies,observer);a.compare(14,0,false);
    if(a.equal()){state.pc=missing_receiver_phase;return;}
    if(!needs_failed_area_report(a)){state.pc=failed_fallback_phase;return;}
    a.at(0x10243574);a.load<1>(0,14,0x27);a.compare(0,1);const bool right=a.equal();
    detail::area_coordinates(a,right?0x10243580:0x102435d0,0x10243618,right,0,3,0,14);
    a.at(0x10243618);a.sign8(0,0,true);if(a.equal()){state.pc=failed_fallback_phase;return;}
    detail::advance_action_random(a,0x10243620);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(0,3,0);
    a.mask(3,0,1,false,1);a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0,true);
    state.pc=a.equal()?failed_message_phase:failed_fallback_phase;
}

void describe_failed_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,failed_message_phase,dependencies,observer);auto destination=state.pc;
    if(destination==failed_message_phase) {
        a.load<1>(0,1,0x48);a.sign8(0,0);a.compare(0,46,false);destination=0x10243874;
        if(!a.positive()){a.load(3,2,-0x520c);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);destination=a.count_branch();}
    }
    unsigned message=0;
    switch(destination) {
    case 0x10243690:message=0x1fea;break;case 0x102436bc:message=0x1feb;break;
    case 0x102436e8:message=0x1fec;break;case 0x10243714:message=0x1fed;break;
    case 0x10243740:message=0x1fee;break;case 0x1024376c:message=0x1ff2;break;
    case 0x10243798:message=0x1fef;break;case 0x102437c4:message=0x1ff1;break;
    case 0x102437f0:message=0x1ff0;break;case 0x1024381c:message=0x1ff3;break;
    case 0x10243848:message=0x1ff4;break;case 0x10243874:message=0x1fe9;break;
    default:return;
    }
    emit_receiver_pair(a,destination,message,failed_comment_phase,destination!=0x10243874);state.pc=failed_comment_phase;
}

void describe_failed_comment(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,failed_comment_phase,dependencies,observer);auto destination=state.pc;
    if(destination==failed_comment_phase) {
        a.sign8(0,22);a.compare(0,4);if(a.at_least()){state.pc=failed_outcome_phase;return;}
        detail::advance_action_random(a,0x102438a8);a.load(4,30,0);a.value(3,0x51ec0000);a.load(0,27,0);a.offset(3,3,-0x7ae1);
        a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,4);a.mask(3,0,1,false,1);
        a.add(0,0,3);a.multiply(0,0,50);a.subtract(0,0,4);a.compare(0,9,false);destination=0x10243994;
        if(!a.positive()){a.load(3,2,-0x5210);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);destination=a.count_branch();}
    }
    unsigned message=0;
    switch(destination){case 0x1024391c:message=0x20eb;break;case 0x10243958:message=0x20ea;break;case 0x10243994:message=0x20e8;break;default:return;}
    a.at(destination);a.load<1>(0,14,0x27);a.value(4,message);a.load(3,15,0x1a4);a.value(6,0xffffffff);a.sign8(0,0);
    a.load(5,15,0x1a4);a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);
    a.value(8,0xffffffff);a.sign8(7,0);a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);a.jump(failed_outcome_phase);state.pc=failed_outcome_phase;
}

void describe_failed_fallback(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,failed_fallback_phase,dependencies,observer);auto destination=state.pc;
    if(destination==failed_fallback_phase) {
        a.load<1>(0,1,0x48);a.sign8(0,0);a.compare(0,46,false);if(a.positive()){state.pc=failed_outcome_phase;return;}
        a.load(3,2,-0x5214);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);destination=a.count_branch();
    }
    if(destination==0x102439f4) {
        a.at(destination);a.load<2>(0,1,0x4a,true);a.move(3,14);a.subtract(0,17,0);a.sign16(4,0);a.call(0x10270320);a.sign8(0,3,true);
        if(!a.equal()){state.pc=failed_outcome_phase;return;}
        a.jump(return_phase);state.pc=return_phase;return;
    }
    unsigned message=0;bool jump=true;
    switch(destination) {
    case 0x10243a14:message=0x1f61;break;case 0x10243a40:message=0x1f62;break;
    case 0x10243a6c:message=0x2150;break;case 0x10243a98:message=0x1f64;break;
    case 0x10243b50:message=0x2164;break;case 0x10243b7c:message=0x1f5d;jump=false;break;
    case 0x10243ac4:
        detail::advance_action_random(a,destination);a.load(5,30,0);a.value(3,0x2aab0000);a.value(0,0x43300000);a.load(4,27,0);a.load<2>(5,5,0);
        a.offset(3,3,-0x5555);a.store(1,0x78,0);a.add(4,5,4);a.floating_load(1,31,0xe4,true);a.multiply_high(3,3,4);a.floating_load(0,14,0xb0,true);
        a.floating_load(2,26,0x80);a.floating_sum_single(3,1,0);a.floating_load(0,15,0xb0,true);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,6);
        a.subtract(0,0,4);a.exclusive_flags(0,0,0x80000000);a.store(1,0x7c,0);a.floating_load(1,1,0x78);a.floating_subtract(1,1,2,true);
        a.floating_sum_single(1,3,1);a.floating_compare(1,0);
        if(a.at_least()){destination=0x10243b7c;message=0x1f5d;jump=false;}else{destination=0x10243b50;message=0x2164;}
        break;
    default:return;
    }
    emit_receiver_pair(a,destination,message,failed_outcome_phase,jump);state.pc=failed_outcome_phase;
}
}
