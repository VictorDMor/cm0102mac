#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call=SemanticGuestCall;

void receiver_interval_limit(Call& a,std::uint32_t start,bool upper) {
    a.at(start);a.sign8(6,16);a.sign8(5,13);a.combine(4,6,18,false,true);a.value(3,upper?0x10000:0xffff0000);
    a.combine(0,5,17,false,true);a.shift_right(7,4,1);a.intersect(6,4,6);a.shift_right(4,0,1);a.intersect(0,0,5);
    a.subtract(0,0,4);a.offset(3,3,upper?-1:1);a.subtract(4,6,7);a.mask(0,0,1,false,1);a.mask(4,4,1,false,1);
    a.subtract_from(0,0,3);a.subtract(0,4,0);a.compare_registers(0,3);
}

void wide_receiver_sample(Call& a) {
    a.at(0x10243e54);a.load(4,30,0);a.value(3,0x80010000);a.value(0,0x43300000);a.sign8(12,16);a.load<2>(6,4,0);a.sign8(7,13);
    a.load(4,27,0);a.combine(9,12,18,false,true);a.offset(3,3,-0x7fff);a.combine(5,7,17,false,true);a.add(6,6,4);a.value(4,0x10000);
    a.multiply_high(11,3,6);a.store(1,0x78,0);a.sign8(10,16);a.floating_load(2,26,0x80);a.intersect(7,5,7);a.floating_load(0,26,0);
    a.add(0,11,6);a.value(11,0x43300000);a.shift_right(0,0,15);a.intersect(12,9,12);a.shift_right(19,9,1);a.offset(4,4,-1);a.shift_right(9,5,1);
    a.mask(5,0,1,false,1);a.subtract(7,7,9);a.subtract(12,12,19);a.mask(9,7,1,false,1);a.add(0,0,5);a.subtract_from(9,9,3);
    a.mask(12,12,1,false,1);a.subtract(12,12,9);a.sign8(8,13);a.exclusive_flags(12,12,0x80000000);a.combine(3,10,18,false,true);a.store(1,0x7c,12);
    a.product(5,0,4);a.combine(7,8,17,false,true);a.floating_load(1,1,0x78);a.shift_right(9,3,1);a.intersect(3,3,10);a.store(1,0x80,11);
    a.floating_subtract(2,1,2);a.subtract(19,5,6);a.exclusive_flags(5,19,0x80000000);a.subtract(9,3,9);a.value(3,0x80010000);a.store(1,0x84,5);
    a.floating_difference(2,2,0,true);a.shift_right(4,7,1);a.intersect(0,7,8);a.floating_load(1,26,0x80);a.subtract(0,0,4);a.floating_load(0,1,0x80);
    a.mask(0,0,1,false,1);a.floating_subtract(0,0,1);a.mask(4,9,1,false,1);a.offset(3,3,-0x7fff);a.subtract_from(0,0,3);a.subtract(0,4,0);
    a.floating_product(0,2,0);a.multiply(0,0,10);a.multiply_high(3,3,0);a.floating_integer(0,0);a.floating_store(0,1,0x88);a.add(0,3,0);a.load(19,1,0x8c);
    a.shift_right(0,0,15);a.mask(3,0,1,false,1);a.add(3,0,3);a.call(0x10226520);a.value(5,0x66660000);a.offset(3,3,5);a.offset(5,5,0x6667);
    a.sign8(4,16);a.multiply_high(6,5,3);a.sign8(0,13);a.combine(5,4,18,false,true);a.combine(3,0,17,false,true);a.intersect(4,5,4);a.intersect(0,3,0);
    a.shift_right(6,6,2);a.shift_right(5,5,1);a.shift_right(3,3,1);a.mask(7,6,1,false,1);a.subtract(0,0,3);a.subtract(3,4,5);a.add(4,6,7);
    a.mask(0,0,1,false,1);a.mask(3,3,1,false,1);a.add(19,19,4);a.subtract_from(0,0,3);a.subtract(3,3,0);a.offset(0,3,-1);a.compare_registers(19,0);
    if(a.nonpositive())return;
    a.sign8(5,16);a.sign8(4,13);a.combine(3,5,18,false,true);a.combine(0,4,17,false,true);a.shift_right(6,3,1);a.intersect(5,3,5);a.shift_right(3,0,1);
    a.intersect(0,0,4);a.subtract(0,0,3);a.subtract(3,5,6);a.mask(0,0,1,false,1);a.mask(3,3,1,false,1);a.subtract_from(0,0,3);a.subtract(3,3,0);
    a.offset(19,3,-1);a.jump(0x102440b4);
}

void narrow_receiver_sample(Call& a) {
    a.at(0x10244018);a.sign8(5,16);a.sign8(4,13);a.combine(3,5,18,false,true);a.load(8,30,0);a.combine(0,4,17,false,true);a.sign8(6,16);
    a.shift_right(7,3,1);a.intersect(5,3,5);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.subtract(3,5,7);a.mask(0,0,1,false,1);
    a.load<2>(8,8,0);a.load(5,27,0);a.mask(7,3,1,false,1);a.subtract_from(3,0,3);a.combine(4,6,18,false,true);a.add(0,8,5);a.sign8(5,13);
    a.subtract(9,7,3);a.shift_right(7,4,1);a.divide(8,0,9);a.combine(3,5,17,false,true);a.intersect(6,4,6);a.shift_right(4,3,1);a.intersect(3,3,5);
    a.subtract(5,6,7);a.product(6,8,9);a.subtract(3,3,4);a.mask(4,5,1,false,1);a.mask(3,3,1,false,1);a.subtract_from(3,3,3);a.subtract(19,6,0);
    a.subtract(0,4,3,true);a.negate(0,19);if(!a.nonpositive())a.move(0,19);a.at(0x102440b0);a.move(19,0);
}

void receiver_performance_ratio(Call& a,std::uint32_t start,unsigned denominator,unsigned numerator) {
    a.at(start);a.load<1>(0,14,denominator);a.compare(0,0,false);
    if(a.equal()){a.at(start+0x4c);a.floating_load(29,31,0,true);return;}
    a.load<1>(5,14,numerator);a.value(4,0x43300000);a.load<1>(3,14,denominator);a.value(0,0x43300000);a.store(1,0x7c,5);
    a.floating_load(2,26,0xf0);a.store(1,0x78,4);a.floating_load(1,26,0xf0);a.floating_load(0,1,0x78);a.store(1,0x84,3);
    a.floating_subtract(2,0,2,true);a.store(1,0x80,0);a.floating_load(0,1,0x80);a.floating_subtract(0,0,1,true);
    a.floating_difference(29,2,0,true,true);a.jump(start+0x50);
}

bool receiver_club_allows_report(Call& a,std::uint32_t start) {
    a.at(start);a.load<1>(0,14,0x27);a.sign8(0,0,true);
    if(a.equal()){a.at(start+0x20);a.load(3,14,0x1a4);a.offset(3,3,0x10000);a.load<1>(0,3,-0x1f8d);a.sign8(0,0);}
    else {a.load(3,14,0x1a4);a.offset(3,3,0x10000);a.load<1>(0,3,-0x1fc3);a.sign8(0,0);a.jump(start+0x30);}
    a.at(start+0x30);a.sign8(0,0,true);if(!a.equal())return false;
    a.floating_load(0,26,0x288);a.floating_compare(29,0);return !a.nonpositive();
}

void describe_receiver_performance(Call& a,bool special) {
    const auto ratio=special?0x102441dc:0x10244340,draw=special?0x1024422c:0x10244390;
    receiver_performance_ratio(a,ratio,special?7:4,special?8:5);detail::advance_action_random(a,draw);
    a.load(3,30,0);a.value(4,0x66660000);a.load(0,27,0);a.offset(4,4,0x6667);a.load<2>(5,3,0);a.sign8(3,13);a.add(5,5,0);
    a.multiply_high(0,4,5);a.shift_right(0,0,3);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,20);a.subtract(4,0,5);
    a.offset(0,4,special?55:65);a.sign8(0,0);a.compare_registers(3,0);if(a.nonpositive())return;
    if(!receiver_club_allows_report(a,special?0x10244298:0x102443fc))return;
    if(special){a.load<1>(0,14,8);a.compare(0,3,false);if(a.nonpositive())return;a.load<1>(0,14,9);a.compare(0,0,false);if(a.equal())return;}
    else {a.load<1>(0,14,6);a.compare(0,1,false);if(a.nonpositive())return;}
    a.load<2>(0,14,0x36,true);a.compare(0,0x1a5e);if(a.nonpositive())return;
    a.load<1>(0,14,0x100);a.sign8(0,0,true);if(!a.equal())return;
    a.load(3,15,0x1a4);a.value(4,special?0x21ce:0x21cd);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
    a.load<1>(6,14,0x2f);a.load<1>(7,14,0x27);a.load<1>(8,15,0x2f);a.load<1>(9,15,0x27);a.call(0x10201180);
    a.value(0,1);a.store<1>(14,0x100,0);if(special)a.jump(receiver_effects_phase);
}
}

void prepare_receiver_outcome(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,failed_outcome_phase,dependencies,observer);a.move(3,14);a.call(0x1022e3a0);a.load<2>(0,1,0x4a,true);a.subtract(3,17,0);a.offset(0,3,-200);
    a.store<2>(14,0x1a0,0);a.load<2>(0,14,0x1a0,true);a.compare(0,200);
    if(!a.nonpositive()){a.value(0,200);a.store<2>(14,0x1a0,0);a.jump(0x10243be8);}
    else {a.at(0x10243bd4);a.load<2>(0,14,0x1a0,true);a.compare(0,-200);if(!a.at_least()){a.value(0,std::uint32_t(-200));a.store<2>(14,0x1a0,0);}}
    a.at(0x10243be8);a.load<1>(0,1,0x48);a.value(17,0);a.value(4,100);a.sign8(0,0);a.compare(0,33);bool larger=a.equal();
    if(!larger){if(!a.at_least()){a.compare(0,19);if(a.equal()){a.at(0x10243c24);a.value(4,400);}else a.jump(0x10243c28);}
        else {a.at(0x10243c10);a.compare(0,45);if(a.equal())larger=true;else a.jump(0x10243c28);}}
    if(larger){a.at(0x10243c1c);a.value(4,250);a.jump(0x10243c28);}
    a.at(0x10243c28);a.load<2>(3,14,0x1a0,true);a.sign16(0,4);a.compare_registers(3,0);if(a.at_least()){state.pc=receiver_position_phase;return;}
    a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.compare(0,4);if(a.equal()){state.pc=receiver_position_phase;return;}
    a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,-1);if(a.equal()){state.pc=receiver_position_phase;return;}
    a.load<1>(0,15,0x27);a.value(6,1);a.load(3,15,0x1a4);a.value(8,0);a.sign8(0,0);a.load<1>(4,15,0x105);a.leading_zeroes(0,0);
    a.load<1>(5,15,0x106);a.mask(0,0,0x07ffffff,false,27);a.value(9,1);a.sign8(7,0);a.value(10,0);a.call(0x101f1100);
    a.load(3,15,0x1a4);a.load<1>(0,3,0x7928);a.sign8(0,0,true);state.pc=a.equal()?receiver_sampling_phase:receiver_marker_phase;
}

void sample_receiver_counter(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,receiver_sampling_phase,dependencies,observer);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.load<1>(13,14,0x10f);a.load<1>(16,14,0x110);
    a.compare_registers(3,0,false);a.store(30,0,3);if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(0x10243cd0);a.load(5,30,0);a.value(4,0x66660000);a.load(3,30,0);a.offset(4,4,0x6667);a.load<2>(6,5,0);a.load(5,27,0);a.offset(3,3,2);
    a.load(0,29,0);a.add(5,6,5);a.store(30,0,3);a.multiply_high(4,4,5);a.compare_registers(3,0,false);a.shift_right(0,4,2);a.mask(3,0,1,false,1);
    a.add(0,0,3);a.multiply(0,0,10);a.subtract(17,0,5);if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(0x10243d28);a.load(3,30,0);a.value(4,0x66660000);a.load(5,27,0);a.offset(4,4,0x6667);a.load<2>(7,3,0);a.sign8(0,13);a.combine(3,0,17,false,true);
    a.sign8(6,16);a.add(7,7,5);a.multiply_high(4,4,7);a.intersect(0,3,0);a.shift_right(4,4,2);a.mask(5,4,1,false,1);a.add(4,4,5);
    a.multiply(4,4,10);a.subtract(18,4,7);a.combine(4,6,18,false,true);a.shift_right(5,4,1);a.shift_right(3,3,1);a.intersect(4,4,6);
    a.subtract(0,0,3);a.subtract(3,4,5);a.mask(0,0,1,false,1);a.mask(3,3,1,false,1);a.subtract_from(0,0,3);a.subtract(0,3,0,true);
    if(a.equal()){a.value(19,0);a.jump(0x102440b4);}
    else {
        detail::advance_action_random(a,0x10243d9c);receiver_interval_limit(a,0x10243dc4,true);bool wide=a.positive();
        if(!wide){receiver_interval_limit(a,0x10243e0c,false);wide=!a.at_least();}
        if(wide)wide_receiver_sample(a);else narrow_receiver_sample(a);
    }
    a.at(0x102440b4);a.store<1>(14,0x2c,19);a.value(17,1);a.jump(receiver_effects_phase);state.pc=receiver_effects_phase;
}

void handle_receiver_marker(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,receiver_marker_phase,dependencies,observer);a.load<1>(0,1,0xd0);a.sign8(0,0,true);
    if(!a.equal()){a.move(3,15);a.call(0x10288230);a.jump(receiver_effects_phase);}state.pc=receiver_effects_phase;
}

void position_receiver(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,receiver_position_phase,dependencies,observer);a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,4);bool normalize=!a.at_least();
    if(normalize){a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,-1);normalize=!a.nonpositive();}
    if(normalize){a.load(3,15,0x1a4);a.value(0,0);a.store<1>(3,0x791f,0);}
    a.at(0x1024410c);a.load<1>(0,14,0x27);a.sign8(0,0,true);bool right=a.equal();
    if(right){a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,-1);right=!a.equal();}
    a.at(right?0x1024412c:0x1024416c);a.load<1>(0,15,0x106);a.move(3,14);a.load<1>(4,15,0x105);a.value(6,4);a.sign8(0,0);a.load<1>(5,15,0x106);a.compare(0,right?9:3);
    const bool clamp=right?a.at_least():a.nonpositive();
    if(clamp){a.at(right?0x1024415c:0x1024419c);a.value(0,right?11:0);}
    else {a.load<1>(0,15,0x106);a.sign8(7,0);a.offset(0,7,right?3:-3);a.jump(right?0x10244160:0x102441a0);}
    a.at(right?0x10244160:0x102441a0);a.sign8(7,0);a.call(0x10237830);if(right)a.jump(receiver_assessment_phase);state.pc=receiver_assessment_phase;
}

void assess_receiver(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,receiver_assessment_phase,dependencies,observer);a.load(3,15,0x1a4);a.value(4,0x2e8c0000);a.load<1>(0,1,0x48);a.offset(4,4,-0x5d17);
    a.load<2>(3,3,0x7948,true);a.sign8(0,0);a.compare(0,21);a.multiply_high(0,4,3);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.sign8(13,0);
    const bool special=a.equal();describe_receiver_performance(a,special);state.pc=receiver_effects_phase;
}
}
