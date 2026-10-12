#include "player_movement.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_contact_random(Call& a,std::uint32_t site) {
    a.at(site);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(site+0x28);
}
void describe_contact(Call& a) {
    a.move(3,22);a.call(0x10289560);a.sign16(0,3);a.compare(0,1);
    if(!a.equal()){a.load(3,15,0x1a4);a.value(4,0x1f4d);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
        a.load<1>(6,15,0x2f);a.load<1>(7,15,0x27);a.load<1>(8,22,0x2f);a.load<1>(9,22,0x27);a.call(0x10201180);}
}
bool evaluate_contact(Call& a) {
    a.at(0x10230024);a.load<2>(3,1,0x48,true);a.sign16(0,21);a.compare_registers(3,0);
    if(!a.at_least()){
        advance_contact_random(a,0x10230034);a.load(4,30,0);a.value(3,0x66660000);a.load(0,27,0);
        a.offset(3,3,0x6667);a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,1);
        a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,5);a.subtract(0,0,4,true);
        if(!a.equal()){a.at(0x10230308);a.value(0,0);a.store<1>(1,0x47,0);return false;}
    }
    a.at(0x10230090);a.load(3,15,0x1a4);a.move(4,22);a.load<1>(6,1,0x46);a.move(5,15);
    a.offset(8,1,0x42);a.offset(9,1,0x41);a.offset(10,1,0x40);a.value(7,UINT32_MAX);a.call(0x101f5050);
    a.store<1>(1,0x47,3);a.load(3,15,0x1a4);a.load<1>(0,3,0x7931);a.sign8(0,0,true);return !a.equal();
}
void sample_contact_condition_wide(Call& a) {
    a.at(0x10230168);a.exclusive_flags(3,24,0x80000000);a.value(0,0x43300000);a.store(1,0xc0,3);
    a.value(6,0x80010000);a.load(7,30,0);a.value(4,0x43300000);a.store(1,0xbc,0);a.value(5,0x10000);
    a.floating_load(2,26,0x80);a.value(3,0x80010000);a.floating_load(1,1,0xbc);a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0);a.offset(5,5,-1);a.floating_subtract(1,1,2);a.load<2>(8,7,0);a.load(7,27,0);
    a.multiply(0,24,10);a.offset(3,3,-0x7fff);a.store(1,0xc4,4);a.floating_difference(2,1,0,true);a.add(7,8,7);
    a.multiply_high(4,6,7);a.floating_load(1,26,0x80);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);
    a.add(4,4,6);a.product(4,4,5);a.subtract(23,4,7);a.exclusive_flags(4,23,0x80000000);a.store(1,0xc8,4);
    a.multiply_high(3,3,0);a.floating_load(0,1,0xc4);a.floating_subtract(0,0,1);a.add(0,3,0);a.shift_right(0,0,15);
    a.floating_product(0,2,0);a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,0xcc);
    a.load(23,1,0xd0);a.call(0x10226520);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);
    a.offset(0,24,-1);a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(23,23,3);
    a.compare_registers(23,0);if(!a.nonpositive()){a.offset(23,24,-1);a.jump(0x1023027c);}
}
void sample_contact_condition(Call& a) {
    a.at(0x1023010c);a.load<2>(24,22,0x32,true);a.compare(24,0);
    if(a.equal()){a.value(23,0);a.jump(0x1023027c);return;}
    advance_contact_random(a,0x10230120);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(24,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(24,3);wide=!a.at_least();}
    if(wide)sample_contact_condition_wide(a);
    else {a.at(0x1023024c);a.load(3,30,0);a.compare(24,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
        a.divide(0,3,24);a.product(0,0,24);a.subtract(23,0,3);a.negate(0,23);if(!a.nonpositive())a.move(0,23);
        a.at(0x10230278);a.move(23,0);}
}
void describe_contact_warning(Call& a) {
    advance_contact_random(a,0x10230284);a.load(4,30,0);a.value(3,0x55550000);a.load(0,27,0);a.offset(3,3,0x5556);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(3,3,4);a.mask(0,3,1,false,1);a.add(0,3,0);
    a.multiply(0,0,3);a.subtract(0,0,4,true);if(a.equal())return;
    a.load(3,15,0x1a4);a.value(4,0x2168);a.load<1>(7,22,0x27);a.value(8,UINT32_MAX);a.load(3,3,0x79f4);
    a.value(9,UINT32_MAX);a.load<1>(6,22,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);a.jump(0x10230310);
}
void check_contact_warning(Call& a) {
    a.load<1>(0,15,0x27);a.move(3,22);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);
    a.sign8(4,0);a.call(0x10239d10);a.sign8(0,3,true);if(a.equal())return;
    a.load(3,15,0x1a4);a.load<1>(4,1,0x46);a.call(0x101ee530);a.sign8(0,3,true);bool warn=!a.equal();
    if(!warn){a.load<1>(0,1,0x40);a.sign8(0,0,true);warn=!a.equal();}
    if(!warn){sample_contact_condition(a);a.at(0x1023027c);a.compare(23,-10);if(a.at_least())return;}
    describe_contact_warning(a);
}
bool resolve_contact_directly(Call& a) {
    a.at(0x10230310);a.load<1>(0,1,0x47);a.sign8(0,0,true);if(a.equal())return false;
    advance_contact_random(a,0x1023031c);a.load(4,30,0);a.value(3,0x88890000);a.load(0,27,0);a.offset(3,3,-0x7777);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.add(0,0,4);a.shift_right(0,0,4);a.mask(3,0,1,false,1);
    a.add(0,0,3);a.multiply(0,0,30);a.subtract(0,0,4,true);if(!a.equal())return false;
    a.load<1>(0,1,0x43);a.sign8(0,0,true);if(!a.equal())return false;
    a.load<1>(0,1,0x42);a.move(5,22);a.move(6,15);a.store(1,0x38,0);a.load<1>(0,1,0x41);a.store(1,0x3c,0);
    a.load<1>(4,1,0x46);a.load(3,15,0x1a4);a.load<1>(7,1,0x43);a.sign8(4,4);a.load<1>(8,1,0x47);
    a.load<1>(9,15,0x105);a.load<1>(10,15,0x106);a.call(0x101d68f0);a.jump(carrier_resolved_contact_phase);return true;
}
}
void continue_carrier_contact(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_contact_phase,dependencies,observer);a.compare(22,0,false);
    if(a.equal()){state.pc=carrier_no_contact_phase;return;}describe_contact(a);
    if(evaluate_contact(a))check_contact_warning(a);
    if(resolve_contact_directly(a)){state.pc=carrier_resolved_contact_phase;return;}
    a.at(0x102303c4);a.load<1>(0,1,0x47);a.sign8(0,0,true);
    if(a.equal()){state.pc=carrier_failed_contact_phase;return;}
    a.load<1>(0,1,0x40);a.move(4,22);a.move(5,15);a.offset(6,1,0x47);a.store(1,0x38,0);a.offset(10,1,0x4c);
    a.load(3,15,0x1a4);a.load<1>(7,1,0x42);a.load<1>(8,1,0x41);a.load<1>(9,1,0x43);a.call(0x101f5110);
    state.pc=carrier_contact_decision_phase;
}
void continue_carrier_contact_outcome(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    const bool failed=state.pc==carrier_failed_contact_phase;
    Call a(state,failed?carrier_failed_contact_phase:carrier_resolved_contact_phase,dependencies,observer);
    if(failed){a.load<1>(0,1,0x40);a.sign8(0,0,true);
        if(!a.equal()){a.load<1>(0,15,0x27);a.move(3,15);a.sign8(0,0);a.leading_zeroes(0,0);
            a.mask(0,0,0x07ffffff,false,27);a.sign8(4,0);a.call(0x10239d10);a.sign8(0,3,true);bool flag=!a.equal();
            if(!flag){a.load<1>(0,1,0x43);a.sign8(0,0,true);flag=!a.equal();}
            if(flag){a.at(0x10231704);a.value(13,1);}}
    }
    a.at(carrier_resolved_contact_phase);a.load<1>(0,1,0x46);a.move(4,22);a.sign8(0,0);a.store<2>(22,0x19c,0);
    a.load<1>(3,22,4);a.offset(0,3,1);a.store<1>(22,4,0);a.load(3,15,0x1a4);a.call(0x101e3d50);a.mask(0,3,0x30,true);
    bool reduced=false;
    if(!a.equal()){a.load(3,15,0x1a4);a.move(4,22);a.call(0x101e3d50);a.sign16(0,3);a.mask(0,0,0x880,true);
        if(!a.equal()){a.load<2>(3,22,0x36,true);a.offset(0,3,-59);a.store<2>(22,0x36,0);a.jump(carrier_no_contact_phase);reduced=true;}}
    if(!reduced){a.at(0x1023175c);a.load<2>(3,22,0x36,true);a.offset(0,3,-70);a.store<2>(22,0x36,0);}
    state.pc=carrier_no_contact_phase;
}
}
