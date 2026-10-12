#include "player_movement.hpp"
#include "player_movement_internal.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_repeat_random(Call& a,std::uint32_t site) {
    a.at(site);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(site+0x28);
}
void describe_repeat_pair(Call& a,std::uint32_t site,std::uint32_t code,bool branch=true) {
    a.at(site);a.load(3,15,0x1a4);a.value(4,code);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
    a.load<1>(6,22,0x2f);a.load<1>(7,22,0x27);a.load<1>(8,15,0x2f);a.load<1>(9,15,0x27);a.call(0x10201180);
    if(branch)a.jump(carrier_transfer_phase);
}
void describe_repeat_side(Call& a,std::uint32_t site,std::uint32_t code) {
    a.at(site);a.load<1>(0,22,0x27);a.value(4,code);a.load(3,15,0x1a4);a.value(6,UINT32_MAX);a.sign8(0,0);
    a.load(5,15,0x1a4);a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);
    a.value(8,UINT32_MAX);a.sign8(7,0);a.value(9,UINT32_MAX);a.value(10,0);a.call(0x10201180);a.jump(carrier_transfer_phase);
}
void sample_repeat_condition_wide(Call& a,std::uint32_t delta) {
    a.at(0x1023397c+delta);a.exclusive_flags(3,16,0x80000000);a.value(0,0x43300000);a.store(1,0xc0,3);a.value(6,0x80010000);
    a.load(7,30,0);a.value(4,0x43300000);a.store(1,0xbc,0);a.value(5,0x10000);a.floating_load(2,26,0x80);
    a.value(3,0x80010000);a.floating_load(1,1,0xbc);a.offset(6,6,-0x7fff);a.floating_load(0,26,0);
    a.offset(5,5,-1);a.floating_subtract(1,1,2);a.load<2>(8,7,0);a.load(7,27,0);a.multiply(0,16,10);
    a.offset(3,3,-0x7fff);a.store(1,0xc4,4);a.floating_difference(2,1,0,true);a.add(7,8,7);a.multiply_high(4,6,7);
    a.floating_load(1,26,0x80);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.add(4,4,6);
    a.product(4,4,5);a.subtract(14,4,7);a.exclusive_flags(4,14,0x80000000);a.store(1,0xc8,4);a.multiply_high(3,3,0);
    a.floating_load(0,1,0xc4);a.floating_subtract(0,0,1);a.add(0,3,0);a.shift_right(0,0,15);a.floating_product(0,2,0);
    a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,0xcc);a.load(14,1,0xd0);
    a.call(0x10226520);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,16,-1);
    a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(14,14,3);a.compare_registers(14,0);
    if(!a.nonpositive()){a.offset(14,16,-1);a.jump(0x10233a90+delta);}
}
void sample_repeat_bound(Call& a,std::uint32_t delta) {
    advance_repeat_random(a,0x10233934+delta);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(16,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(16,3);wide=!a.at_least();}
    if(wide)sample_repeat_condition_wide(a,delta);
    else {a.at(0x10233a60+delta);a.load(3,30,0);a.compare(16,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
        a.divide(0,3,16);a.product(0,0,16);a.subtract(14,0,3);a.negate(0,14);if(!a.nonpositive())a.move(0,14);
        a.at(0x10233a8c+delta);a.move(14,0);}
}
void sample_repeat_condition(Call& a) {
    a.load<2>(16,22,0x32,true);a.compare(16,0);
    if(a.equal()){a.value(14,0);a.jump(0x10233a90);return;}
    sample_repeat_bound(a,0);
}
bool repeat_warning_needed(Call& a) {
    a.load(3,15,0x1a4);a.load<1>(4,1,0x46);a.call(0x101ee530);a.sign8(0,3,true);bool warning=!a.equal();
    if(!warning){a.load<1>(0,1,0x40);a.sign8(0,0,true);warning=!a.equal();}
    if(!warning){sample_repeat_condition(a);a.at(0x10233a90);a.compare(14,-10);if(a.at_least())return false;}
    a.at(0x10233a98);a.load<1>(4,22,0x27);a.move(3,22);a.call(0x10239d10);a.sign8(0,3,true);if(a.equal())return false;
    advance_repeat_random(a,0x10233aac);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(0,3,0);
    a.mask(3,0,1,false,1);a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0,true);return a.equal();
}
bool describe_repeat_warning(Call& a,std::uint32_t direct=0) {
    a.at(0x10233af8);if(!describe_carrier_direction(a,-0x51f8,0x36fc,direct))return false;
    advance_repeat_random(a,0x10233d28);a.load(4,30,0);a.value(3,0x51ec0000);a.load(0,27,0);a.offset(3,3,-0x7ae1);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,4);a.mask(3,0,1,false,1);
    a.add(0,0,3);a.multiply(0,0,50);a.subtract(0,0,4);a.compare(0,10,false);std::uint32_t destination=0x10233e14;
    if(!a.positive()){a.load(3,2,-0x51fc);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);destination=a.count_branch();}
    switch(destination){
    case 0x10233d9c:describe_repeat_side(a,destination,0x20eb);break;
    case 0x10233dd8:describe_repeat_side(a,destination,0x20ea);break;
    case 0x10233e14:describe_repeat_side(a,destination,0x20e8);break;
    default:return false;
    }
    return true;
}
std::uint32_t describe_repeat_fallback(Call& a) {
    a.at(0x10233e50);a.load<1>(0,1,0x46);a.sign8(0,0);a.compare(0,34);std::uint32_t site=0,code=0;
    if(a.equal()){site=0x10233f28;code=0x2150;}
    else if(!a.at_least()){
        a.compare(0,19);if(a.equal()){site=0x10233efc;code=0x1f62;}
        else if(!a.at_least()){a.compare(0,1);if(a.equal()){
            a.at(0x10233eb0);a.load<2>(0,1,0x48,true);a.move(3,22);a.subtract(0,21,0);a.sign16(4,0);a.call(0x10270320);a.sign8(0,3,true);
            if(!a.equal())return carrier_transfer_phase;a.jump(common_return_phase);return common_return_phase;
        }a.jump(carrier_transfer_phase);return carrier_transfer_phase;}
        else {a.at(0x10233e7c);a.compare(0,33);if(a.at_least()){site=0x10233efc;code=0x1f62;}else {a.jump(carrier_transfer_phase);return carrier_transfer_phase;}}
    } else {
        a.at(0x10233e88);a.compare(0,44);if(a.equal()){site=0x10233f28;code=0x2150;}
        else if(!a.at_least()){a.compare(0,41);if(a.equal()){site=0x10233ed0;code=0x1f61;}
            else if(a.at_least()){site=0x10233f54;code=0x1f64;}else {a.jump(carrier_transfer_phase);return carrier_transfer_phase;}}
        else {a.at(0x10233ea4);a.compare(0,46);if(a.at_least())return carrier_transfer_phase;a.jump(0x10233efc);site=0x10233efc;code=0x1f62;}
    }
    if(site)describe_repeat_pair(a,site,code,site!=0x10233f54);return carrier_transfer_phase;
}
}
void sample_carrier_condition(SemanticGuestCall& a,std::uint32_t delta){sample_repeat_bound(a,delta);}
void continue_carrier_repeated_table(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    const auto destination=state.pc;Call a(state,destination,dependencies,observer);
    if(destination<0x10233d28){if(describe_repeat_warning(a,destination))state.pc=carrier_transfer_phase;return;}
    switch(destination){
    case 0x10233d9c:describe_repeat_side(a,destination,0x20eb);break;
    case 0x10233dd8:describe_repeat_side(a,destination,0x20ea);break;
    case 0x10233e14:describe_repeat_side(a,destination,0x20e8);break;
    default:throw std::runtime_error("Unknown repeated-contact action destination");
    }
    state.pc=carrier_transfer_phase;
}
void continue_carrier_simple_play(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    const bool simple=state.pc==carrier_kind15_phase;Call a(state,simple?carrier_kind15_phase:carrier_other_kind_phase,dependencies,observer);
    if(simple){a.compare(22,0,false);
        if(!a.equal()){a.load(3,15,0x1a4);a.value(4,0x1f49);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
            a.load<1>(6,15,0x2f);a.load<1>(7,15,0x27);a.load<1>(8,22,0x2f);a.load<1>(9,22,0x27);a.call(0x10201180);a.jump(noncarrier_phase);}
        else {a.at(0x10233864);a.load(3,15,0x1a4);a.value(4,0x1f48);a.load<1>(7,15,0x27);a.value(8,UINT32_MAX);
            a.load(3,3,0x79f4);a.value(9,UINT32_MAX);a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);a.jump(noncarrier_phase);}}
    else {a.load(3,2,-0x7bb8);a.offset(4,14,0x63);a.sign16(5,18);a.call(0x1077b720);a.offset(3,14,0x8f);a.offset(6,1,0x80);a.offset(7,1,0x58);
        a.value(4,0);a.value(5,0);a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.load(3,2,-0x7ba8);a.offset(4,14,0x9c);
        a.offset(6,1,0x80);a.offset(7,1,0x58);a.value(8,0xfce);a.call(0x1077b720);a.load(4,2,-0x7ba8);a.offset(3,14,0xa9);a.value(5,0);a.call(0x10769b60);
        a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.jump(noncarrier_phase);}
    state.pc=noncarrier_phase;
}
void continue_carrier_repeated_contact(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    // The unread first load also permits a precise development capture after
    // the original register-only null check; no step is replayed or removed.
    const bool checked=state.pc==0x10233900;
    Call a(state,carrier_repeated_contact_phase,dependencies,observer);
    if(checked)a.at(0x10233900);
    else {a.compare(22,0,false);if(a.equal()){state.pc=carrier_null_contact_phase;return;}}
    if(repeat_warning_needed(a)){if(describe_repeat_warning(a))state.pc=carrier_transfer_phase;}
    else state.pc=describe_repeat_fallback(a);
}
}
