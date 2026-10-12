#include "player_movement.hpp"
#include "semantic_guest_call.hpp"
#include "player_movement_internal.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_followup_random(Call& a,std::uint32_t site) {
    a.at(site);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(site+0x28);
}
void describe_major_contact(Call& a) {
    a.sign16(0,17);a.compare(0,0x1f7a);
    if(a.equal()){a.load(3,15,0x1a4);a.move(4,17);a.load<1>(7,15,0x27);a.value(6,UINT32_MAX);a.load(3,3,0x79f4);
        a.value(8,UINT32_MAX);a.load(5,15,0x1a4);a.value(9,UINT32_MAX);a.value(10,0);a.call(0x10201180);}
}
bool report_eligible(Call& a) {
    a.load(3,15,0x1a4);a.load(0,3,0x3114);a.compare(0,0,false);if(a.equal())return false;
    a.load<1>(0,1,0x47);a.compare(0,5);
    if(!a.equal()){a.load<1>(0,1,0x47);a.compare(0,4);if(!a.equal())return false;}
    a.at(0x10230abc);a.load<1>(0,1,0x42);a.compare(0,10);
    if(!a.equal()){a.load<1>(0,1,0x42);a.compare(0,5);
        if(!a.equal()){a.load<1>(0,1,0x42);a.compare(0,11);if(!a.equal())return false;}}
    a.at(0x10230ae0);a.load(0,22,0x6c);a.compare(0,0,false);if(a.equal())return false;
    a.load(0,22,0x6c);a.compare(0,0,false);if(a.equal())return false;
    a.load(0,22,0x6c);a.compare(0,0,false);
    if(!a.equal()){a.load(3,2,-0x7be4);a.load(4,22,0x6c);a.load(0,3,0);a.load(3,4,0);a.compare_registers(3,0);if(a.at_least())return false;}
    a.at(0x10230b1c);a.load(0,15,0x6c);a.compare(0,0,false);
    if(!a.equal()){a.load(3,2,-0x7be4);a.load(4,15,0x6c);a.load(0,3,0);a.load(3,4,0);a.compare_registers(3,0);if(a.at_least())return false;}
    a.at(0x10230b40);a.load(0,22,0xfc);a.compare(0,0,false);if(a.equal())return false;
    a.load(3,22,0xfc);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);if(!a.equal())return false;
    a.load(4,22,0x6c);a.sign8(3,13);a.offset(0,3,10);a.load<1>(3,4,0x67);a.sign8(3,3);a.compare_registers(3,0);return !a.at_least();
}
bool passes_report_attribute(Call& a,std::uint32_t site,std::int32_t field) {
    advance_followup_random(a,site);a.load(5,30,0);a.value(4,0x66660000);a.load(3,22,0x70);a.offset(4,4,0x6667);
    a.load<2>(6,5,0);a.load(5,27,0);a.load<1>(0,3,field);a.add(6,6,5);a.multiply_high(4,4,6);a.sign8(3,0);
    a.shift_right(4,4,1);a.mask(5,4,1,false,1);a.subtract_from(0,13,15);a.add(4,4,5);a.multiply(4,4,5);
    a.subtract(4,4,6);a.add(0,0,4);a.sign8(0,0);a.compare_registers(3,0);return !a.nonpositive();
}
bool passes_report_severity(Call& a) {
    advance_followup_random(a,0x10230c78);a.load(5,30,0);a.value(4,0x25730000);a.load(3,22,0x70);a.offset(4,4,-0x4f9);
    a.load<2>(6,5,0);a.load(5,27,0);a.load<2>(3,3,10,true);a.add(5,6,5);a.multiply_high(4,4,5);a.shift_right(0,4,9);
    a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,3500);a.subtract(4,0,5);a.offset(0,4,3000);a.sign16(0,0);
    a.compare_registers(3,0);return !a.nonpositive();
}
bool passes_report_rating(Call& a) {
    advance_followup_random(a,0x10230ce8);a.load(3,30,0);a.value(4,0x55550000);a.load(0,27,0);a.offset(4,4,0x5556);
    a.load<2>(5,3,0);a.sign8(3,13);a.add(5,5,0);a.multiply_high(4,4,5);a.mask(0,4,1,false,1);a.add(0,4,0);
    a.multiply(0,0,3);a.subtract(4,0,5);a.offset(0,4,1);a.compare_registers(3,0);return !a.nonpositive();
}
void record_contact_incident(Call& a) {
    a.load(4,22,0x6c);a.value(0,0);a.move(3,15);a.load(4,4,0);a.store(1,0xa0,4);a.load(4,22,0xfc);
    a.load(4,4,0);a.store(1,0xa4,4);a.load(4,15,0x6c);a.load(4,4,0);a.store(1,0xb8,4);a.store<1>(1,0xac,0);
    a.call(0x10283690);a.value(8,0);a.value(7,UINT32_MAX);a.value(6,0x82);a.value(0,0);a.store<1>(1,0xad,3);
    a.offset(4,1,0xa0);a.value(5,0x1c);a.store<1>(1,0xae,8);a.store(1,0xb0,7);a.store<2>(1,0xb4,6);
    a.store(1,0xa8,0);a.load(3,15,0x1a4);a.load(3,3,0x3114);a.call(0x10738d10);
}
void consider_contact_report(Call& a) {
    a.load(3,2,-0x7b3c);a.value(5,11);a.load(4,22,0x70);a.value(6,2);a.load(3,3,0);a.floating_load(1,31,4,true);
    a.call(0x10303560);a.floating_integer(0,1);a.load(0,15,0x1a4);a.compare(0,0,false);a.floating_store(0,1,0xbc);a.load(13,1,0xc0);
    if(a.equal())return;
    if(report_eligible(a) && passes_report_attribute(a,0x10230b88,0x1c) && passes_report_attribute(a,0x10230c00,0x25) && passes_report_severity(a) && passes_report_rating(a))record_contact_incident(a);
}
void describe_direction(Call& a,std::uint32_t site,std::uint32_t code,bool branch) {
    a.at(site);a.load(3,15,0x1a4);a.value(4,code);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
    a.load<1>(6,15,0x2f);a.load<1>(7,15,0x27);a.load<1>(8,22,0x2f);a.load<1>(9,22,0x27);a.call(0x10201180);
    if(branch)a.jump(0x10230e3c);
}
void note_contact_direction(Call& a) {
    a.at(0x10230db8);a.load<1>(0,1,0x46);a.compare(0,44);bool applicable=a.equal();
    if(!applicable){a.load<1>(0,1,0x46);a.compare(0,47);applicable=a.equal();}if(!applicable)return;
    a.at(0x10230dd0);a.load<1>(0,1,0x40);a.sign8(0,0,true);if(a.equal())return;
    a.load<1>(0,1,0x46);a.compare(0,47);
    if(a.equal())describe_direction(a,0x10230de8,0x21da,true);else describe_direction(a,0x10230e14,0x21d9,false);
    a.load<1>(3,1,0x47);a.offset(0,3,-3);a.mask(0,0,0xff);a.compare(0,3,false);bool mark=a.nonpositive();
    if(!mark){a.sign16(0,17);a.compare(0,0x1f7a);mark=a.equal();}
    if(mark){a.at(0x10230e5c);a.value(0,1);a.store<1>(22,0x16,0);}
}
void sample_commentary_interest_wide(Call& a,std::uint32_t delta) {
    a.at(0x10230f10+delta);a.exclusive_flags(3,14,0x80000000);a.value(0,0x43300000);a.store(1,0xc0,3);a.value(6,0x80010000);
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
    if(!a.nonpositive()){a.offset(13,14,-1);a.jump(0x10231024+delta);}
}
void sample_interest_bound(Call& a,std::uint32_t delta) {
    advance_followup_random(a,0x10230ec8+delta);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(14,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(14,3);wide=!a.at_least();}
    if(wide)sample_commentary_interest_wide(a,delta);
    else {a.at(0x10230ff4+delta);a.load(3,30,0);a.compare(14,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
        a.divide(0,3,14);a.product(0,0,14);a.subtract(13,0,3);a.negate(0,13);if(!a.nonpositive())a.move(0,13);
        a.at(0x10231020+delta);a.move(13,0);}
}
void sample_commentary_interest(Call& a) {
    a.load<1>(14,1,0x41);a.sign8(14,14,true);
    if(a.equal()){a.value(13,0);a.jump(0x10231024);return;}
    sample_interest_bound(a,0);
}
void select_opponent_club(Call& a,std::uint32_t site) {
    a.at(site);a.load<1>(0,22,0x27);a.load(3,15,0x1a4);a.sign8(0,0);a.leading_zeroes(0,0);
    a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0,true);
    if(!a.equal()){a.load(3,3,0x3118);a.load(3,3,0x20);a.jump(site+0x30);}
    else {a.at(site+0x28);a.load(3,3,0x3118);a.load(3,3,0x1c);}
}
void describe_opponent_context(Call& a) {
    select_opponent_club(a,0x10231084);a.load(0,3,0x2c);a.compare(0,0,false);if(a.equal())return;
    select_opponent_club(a,0x102310c0);a.load(3,3,0x2c);a.load<1>(0,3,0x119);a.compare(0,8);if(!a.equal())return;
    a.load(3,15,0x1a4);a.value(4,0x20f5);a.load(5,15,0x1a4);a.value(6,0);a.load(3,3,0x79f4);
    a.value(7,0);a.value(8,UINT32_MAX);a.value(9,UINT32_MAX);a.value(10,0);a.call(0x10201180);
}
void describe_contact_interest(Call& a) {
    advance_followup_random(a,0x10230e64);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
    a.mask(0,3,0xc0000000,false,30);a.mask(3,3,1,false,1);a.subtract(0,3,0);a.mask(0,0,0xffffffff,false,2);
    a.add_record(0,0,3);if(!a.equal())return;
    sample_commentary_interest(a);advance_followup_random(a,0x10231024);a.load(4,30,0);a.value(3,0x66660000);
    a.load(0,27,0);a.offset(3,3,0x6667);a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,3);
    a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,20);a.subtract(0,0,4);a.compare_registers(13,0);
    if(!a.nonpositive())describe_opponent_context(a);
    a.at(0x10231128);a.load(3,15,0x1a4);a.value(4,0x2109);a.load<1>(7,22,0x27);a.value(8,UINT32_MAX);
    a.load(3,3,0x79f4);a.value(9,UINT32_MAX);a.load<1>(6,22,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);
}
void finish_contact_events(Call& a) {
    a.at(0x10231150);a.load(3,15,0x1a4);a.move(4,16);a.load<1>(7,22,0x27);a.value(8,UINT32_MAX);a.load(3,3,0x79f4);
    a.value(9,UINT32_MAX);a.load<1>(6,22,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);
    a.load<1>(0,1,0x47);a.compare(0,6);bool notify=a.equal();
    if(!notify){a.load<1>(3,1,0x47);a.offset(0,3,-3);a.mask(0,0,0xff);a.compare(0,1,false);notify=a.nonpositive();
        if(!notify){a.load<1>(0,1,0x47);a.compare(0,5);notify=a.equal();}}
    if(notify){a.at(0x102311a4);a.load(3,15,0x1a4);a.move(4,22);a.value(5,0);a.call(0x101f1530);}
}
}
void sample_carrier_interest(SemanticGuestCall& a,std::uint32_t delta){sample_interest_bound(a,delta);}
void continue_carrier_contact_followup(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_contact_followup_phase,dependencies,observer);describe_major_contact(a);a.at(0x10230a54);a.sign16(0,16);a.compare(0,-1);
    if(!a.nonpositive()){consider_contact_report(a);note_contact_direction(a);describe_contact_interest(a);finish_contact_events(a);}
    state.pc=carrier_restart_selection_phase;
}
}
