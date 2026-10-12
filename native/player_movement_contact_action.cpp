#include "player_movement.hpp"
#include "semantic_guest_call.hpp"
#include "player_movement_internal.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_action_random(Call& a,std::uint32_t site) {
    a.at(site);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(site+0x28);
}
bool describe_action(Call& a,std::int32_t toc_slot=-0x51f4,std::uint32_t delta=0,std::uint32_t direct=0) {
    std::uint32_t destination=direct;
    if(!direct){a.load<1>(0,1,0x46);a.sign8(0,0);a.compare(0,46,false);destination=0x10230604+delta;
        if(!a.positive()){a.load(3,2,toc_slot);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);
            a.count_destination(3);destination=a.count_branch();}}
    std::uint32_t code=0;
    switch(destination-delta){
    case 0x10230420:code=0x1fea;break;case 0x1023044c:code=0x1feb;break;
    case 0x10230478:code=0x1fec;break;case 0x102304a4:code=0x1fed;break;
    case 0x102304d0:code=0x1fee;break;case 0x102304fc:code=0x1ff2;break;
    case 0x10230528:code=0x1fef;break;case 0x10230554:code=0x1ff1;break;
    case 0x10230580:code=0x1ff0;break;case 0x102305ac:code=0x1ff3;break;
    case 0x102305d8:code=0x1ff4;break;case 0x10230604:code=0x1fe9;break;
    // A development continuation preserves an unrecognized live destination.
    // It neither invents a default action nor invokes the original own body.
    default:return false;
    }
    a.at(destination);a.load(3,15,0x1a4);a.value(4,code);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
    a.load<1>(6,22,0x2f);a.load<1>(7,22,0x27);a.load<1>(8,15,0x2f);a.load<1>(9,15,0x27);a.call(0x10201180);
    if(destination!=0x10230604+delta)a.jump(0x1023062c+delta);return true;
}
void select_contact_result(Call& a) {
    a.at(0x1023062c);a.load<1>(0,1,0x47);a.value(16,UINT32_MAX);a.sign8(0,0);a.compare(0,4);
    std::uint32_t site=0,code=0;
    if(a.equal()){site=0x10230674;code=0x1ff8;}
    else if(a.at_least()){a.at(0x10230654);a.compare(0,6);if(!a.at_least()){site=0x1023065c;code=0x1ff6;}}
    else {a.compare(0,2);if(a.equal()){site=0x1023066c;code=0x1ff9;}
        else if(a.at_least()){site=0x10230664;code=0x1ff7;}else a.jump(0x1023067c);}
    if(site){a.at(site);a.value(16,code);a.jump(0x102306f0);return;}
    advance_action_random(a,0x1023067c);a.load(3,30,0);a.value(4,0x92490000);a.load(5,27,0);a.offset(4,4,0x2493);
    a.load<2>(6,3,0);a.load<1>(0,1,0x41);a.add(5,6,5);a.sign8(3,0);a.multiply_high(0,4,5);a.add(0,0,5);
    a.shift_right(0,0,2);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,7);a.subtract(4,0,5);
    a.offset(0,4,2);a.compare_registers(3,0);if(!a.nonpositive())a.value(16,0x210a);
}
void describe_random_contact(Call& a) {
    advance_action_random(a,0x10230740);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
    a.mask(0,3,0xc0000000,false,30);a.mask(3,3,1,false,1);a.subtract(0,3,0);a.mask(0,0,0xffffffff,false,2);
    a.add(0,0,3);a.compare(0,1);std::uint32_t site=0,code=0;
    if(a.equal()){site=0x102307d0;code=0x20ec;}
    else if(!a.at_least()){a.compare(0,0);if(a.at_least()){site=0x102307a4;code=0x20e9;}else a.jump(0x102307f8);}
    if(!site)return;
    a.at(site);a.load(3,15,0x1a4);a.value(4,code);a.load<1>(7,15,0x27);a.value(6,UINT32_MAX);a.load(3,3,0x79f4);
    a.value(8,UINT32_MAX);a.load(5,15,0x1a4);a.value(9,UINT32_MAX);a.value(10,0);a.call(0x10201180);
    if(site==0x102307a4)a.jump(0x102307f8);
}
void sample_contact_reaction_wide(Call& a) {
    a.at(0x10230880);a.exclusive_flags(3,14,0x80000000);a.value(0,0x43300000);a.store(1,0xc0,3);a.value(6,0x80010000);
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
    if(!a.nonpositive()){a.offset(13,14,-1);a.jump(0x10230994);}
}
void sample_contact_reaction(Call& a) {
    a.load(3,22,0x6c);a.load<1>(0,3,0x66);a.sign8(3,0);a.offset_record(14,3,6);
    if(a.equal()){a.value(13,0);a.jump(0x10230994);return;}
    advance_action_random(a,0x10230838);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(14,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(14,3);wide=!a.at_least();}
    if(wide)sample_contact_reaction_wide(a);
    else {a.at(0x10230964);a.load(3,30,0);a.compare(14,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
        a.divide(0,3,14);a.product(0,0,14);a.subtract(13,0,3);a.negate(0,13);if(!a.nonpositive())a.move(0,13);
        a.at(0x10230990);a.move(13,0);}
}
void apply_major_contact(Call& a) {
    a.at(0x102307f8);a.load(3,15,0x1a4);a.value(0,5);a.value(17,0x1f7a);a.store<1>(3,0x7928,0);
    a.load(3,15,0x1a4);a.store(3,0x3110,15);a.load<2>(3,22,0x36,true);a.offset(0,3,-500);a.store<2>(22,0x36,0);
    sample_contact_reaction(a);a.at(0x10230994);a.compare(13,0);if(!a.equal())return;
    advance_action_random(a,0x1023099c);a.load(4,30,0);a.move(3,22);a.load(0,27,0);a.load<2>(4,4,0);a.add(4,4,0);
    a.mask(0,4,0xe0000000,false,29);a.mask(4,4,1,false,1);a.subtract(0,4,0);a.mask(0,0,0xffffffff,false,3);
    a.add(0,0,4);a.negate(0,0);a.sign8(4,0);a.call(0x10288f30);a.sign8(0,3,true);if(a.equal())return;
    a.move(3,22);a.value(4,0);a.call(0x1022ba10);a.jump(carrier_contact_followup_phase);
}
void choose_contact_action(Call& a) {
    a.at(0x102306f0);a.load<1>(0,1,0x47);a.compare(0,7);
    if(a.equal()){a.load(3,15,0x1a4);a.value(0,7);a.value(17,0x1f79);a.store<1>(3,0x7928,0);a.jump(carrier_contact_followup_phase);return;}
    a.at(0x10230710);a.load<1>(0,15,0x27);a.move(3,15);a.sign8(0,0);a.leading_zeroes(0,0);
    a.mask(0,0,0x07ffffff,false,27);a.sign8(4,0);a.call(0x10239d10);a.sign8(0,3,true);
    if(a.equal()){a.at(0x10230a10);a.load(3,15,0x1a4);a.value(0,6);a.value(17,0x1f79);a.store<1>(3,0x7928,0);return;}
    a.sign16(0,16);a.compare(0,-1);if(a.equal())describe_random_contact(a);apply_major_contact(a);
}
}
bool describe_carrier_direction(SemanticGuestCall& a,std::int32_t toc_slot,std::uint32_t delta,std::uint32_t direct){return describe_action(a,toc_slot,delta,direct);}
void continue_carrier_contact_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    const auto direct=state.pc==carrier_contact_decision_phase?0:state.pc;Call a(state,carrier_contact_decision_phase,dependencies,observer);
    if(!describe_action(a,-0x51f4,0,direct))return;
    select_contact_result(a);choose_contact_action(a);state.pc=carrier_contact_followup_phase;
}
}
