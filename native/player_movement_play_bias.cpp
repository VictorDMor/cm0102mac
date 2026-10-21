#include "player_movement.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_bias_random(Call& a,std::uint32_t site) {
    a.at(site);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(site+0x28);
}
void clamp_play_bias(Call& a,std::uint32_t site,std::uint32_t done) {
    a.at(site);a.load<2>(0,15,0x1a0,true);a.compare(0,200);
    if(!a.nonpositive()){a.value(0,200);a.store<2>(15,0x1a0,0);a.jump(done);}
    else {a.at(site+0x18);a.load<2>(0,15,0x1a0,true);a.compare(0,-200);if(!a.at_least()){a.value(0,std::uint32_t(-200));a.store<2>(15,0x1a0,0);}}
}
void adjust_play_bias(Call& a) {
    a.load<1>(0,15,0x27);a.load(3,15,0x1a4);a.sign8(0,0);a.add(4,3,0);a.load<1>(3,4,0x77bc);
    a.offset(0,3,5);a.store<1>(4,0x77bc,0);a.load<2>(3,1,0x48,true);a.offset(0,3,50);a.compare(0,250);
    if(!a.nonpositive()){a.load<2>(3,1,0x48,true);a.offset(0,3,50);a.jump(0x10231e9c);}else {a.at(0x10231e98);a.value(0,250);}
    a.at(0x10231e9c);a.sign16(4,0);a.sign16(3,21);a.value(0,0x43300000);a.floating_load(1,26,0x80);a.subtract(3,4,3);
    a.store(1,0xbc,0);a.exclusive_flags(0,3,0x80000000);a.floating_load(3,15,0xe8,true);a.store(1,0xc0,0);
    a.floating_load(2,15,0x98,true);a.floating_load(0,1,0xbc);a.floating_subtract(0,0,1,true);a.floating_load(1,31,0,true);
    a.floating_product_sum(0,3,2,0);a.floating_compare(1,0);
    if(!a.at_least()){a.floating_load(0,31,0,true);a.jump(0x10231f18);}
    else {a.at(0x10231ee4);a.sign16(3,4);a.sign16(0,21);a.subtract(3,3,0);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
        a.store(1,0xc4,0);a.floating_load(1,26,0x80);a.store(1,0xc8,3);a.floating_load(3,15,0xe8,true);
        a.floating_load(0,1,0xc4);a.floating_load(2,15,0x98,true);a.floating_subtract(0,0,1,true);a.floating_product_sum(0,3,2,0);}
    a.at(0x10231f18);a.floating_integer(0,0);a.load<2>(0,15,0x1a0,true);a.floating_store(0,1,0xcc);a.load(4,1,0xd0);
    a.add(0,0,4);a.store<2>(15,0x1a0,0);clamp_play_bias(a,0x10231f30,0x10231f5c);
}
void adjust_unopposed_cell_bias(Call& a) {
    a.at(0x10231f5c);a.load<1>(0,15,0x105);a.load<1>(3,15,0x106);a.sign8(4,0);a.load<1>(0,15,0x27);a.sign8(3,3);
    a.load(5,15,0x1a4);a.multiply(4,4,0x450);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);
    a.multiply(3,3,0x5c);a.add(4,5,4);a.sign8(0,0);a.add(3,4,3);a.add(3,3,0);a.load<1>(0,3,0xa60);a.sign8(0,0,true);
    if(!a.equal())return;a.load<2>(3,15,0x1a0,true);a.offset(0,3,50);a.store<2>(15,0x1a0,0);clamp_play_bias(a,0x10231fb0,0x10231fdc);
}
void increment_match_totals(Call& a,std::uint32_t site,std::uint32_t amount,bool branch=false) {
    a.at(site);a.load(4,15,0x1a4);a.load(3,4,0x3108);a.offset(0,3,amount);a.store(4,0x3108,0);
    a.load(4,15,0x1a4);a.load(3,4,0x310c);a.offset(0,3,amount);a.store(4,0x310c,0);if(branch)a.jump(0x10232320);
}
void sample_success_reaction_wide(Call& a) {
    a.at(0x10232120);a.exclusive_flags(3,21,0x80000000);a.value(0,0x43300000);a.store(1,0xc0,3);a.value(6,0x80010000);
    a.load(7,30,0);a.value(4,0x43300000);a.store(1,0xbc,0);a.value(5,0x10000);a.floating_load(2,26,0x80);
    a.value(3,0x80010000);a.floating_load(1,1,0xbc);a.offset(6,6,-0x7fff);a.floating_load(0,26,0);
    a.offset(5,5,-1);a.floating_subtract(1,1,2);a.load<2>(8,7,0);a.load(7,27,0);a.multiply(0,21,10);
    a.offset(3,3,-0x7fff);a.store(1,0xc4,4);a.floating_difference(2,1,0,true);a.add(7,8,7);a.multiply_high(4,6,7);
    a.floating_load(1,26,0x80);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.add(4,4,6);
    a.product(4,4,5);a.subtract(13,4,7);a.exclusive_flags(4,13,0x80000000);a.store(1,0xc8,4);a.multiply_high(3,3,0);
    a.floating_load(0,1,0xc4);a.floating_subtract(0,0,1);a.add(0,3,0);a.shift_right(0,0,15);a.floating_product(0,2,0);
    a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,0xcc);a.load(13,1,0xd0);
    a.call(0x10226520);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,21,-1);
    a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(13,13,3);a.compare_registers(13,0);
    if(!a.nonpositive()){a.offset(13,21,-1);a.jump(0x10232234);}
}
void sample_success_reaction(Call& a) {
    a.load<1>(0,15,0x17);a.sign8(0,0);a.multiply(3,0,20);a.offset_record(21,3,20);
    if(a.equal()){a.value(13,0);a.jump(0x10232234);return;}
    advance_bias_random(a,0x102320d8);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(21,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(21,3);wide=!a.at_least();}
    if(wide)sample_success_reaction_wide(a);
    else {a.at(0x10232204);a.load(3,30,0);a.compare(21,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
        a.divide(0,3,21);a.product(0,0,21);a.subtract(13,0,3);a.negate(0,13);if(!a.nonpositive())a.move(0,13);
        a.at(0x10232230);a.move(13,0);}
}
void apply_success_reaction(Call& a) {
    sample_success_reaction(a);a.at(0x10232234);a.compare(13,0);if(!a.equal())return;
    advance_bias_random(a,0x1023223c);a.load(3,30,0);a.value(4,0x66660000);a.load(0,27,0);a.offset(4,4,0x6667);
    a.load<2>(5,3,0);a.move(3,15);a.add(5,5,0);a.multiply_high(0,4,5);a.shift_right(0,0,1);a.mask(4,0,1,false,1);
    a.add(0,0,4);a.multiply(0,0,5);a.subtract(0,0,5);a.sign8(4,0);a.call(0x10288f30);a.sign8(0,3,true);if(a.equal())return;
    a.move(3,15);a.value(4,0);a.call(0x1022ba10);a.jump(0x10232320);
}
void update_contact_totals(Call& a) {
    a.at(0x10231fdc);a.compare(22,0,false);
    if(a.equal()){a.at(0x102322f4);a.load<2>(3,15,0x36,true);a.offset(0,3,9);a.store<2>(15,0x36,0);increment_match_totals(a,0x10232300,36);return;}
    a.sign16(0,18);a.compare(0,15);
    if(!a.equal()){a.load<1>(0,1,0x44);a.load<1>(3,22,0x2c);a.sign8(0,0);a.compare(0,2);
        if(!a.nonpositive()){a.load<1>(0,1,0x44);a.sign8(0,0);a.jump(0x10232014);}else {a.at(0x10232010);a.value(0,2);}
        a.at(0x10232014);a.sign8(0,0);a.add(0,3,0);a.store<1>(22,0x2c,0);a.jump(0x10232034);
    } else {a.at(0x10232024);a.load<1>(3,22,0x2c);a.load<1>(0,1,0x44);a.add(0,3,0);a.store<1>(22,0x2c,0);}
    a.at(0x10232034);a.sign16(0,18);a.compare(0,15);
    if(a.equal()){a.at(0x102322b8);a.load<1>(3,15,0x11);a.offset(0,3,1);a.store<1>(15,0x11,0);
        a.load<2>(3,15,0x36,true);a.offset(0,3,75);a.store<2>(15,0x36,0);increment_match_totals(a,0x102322d0,300,true);return;}
    a.move(3,22);a.call(0x10289560);a.sign16(0,3);a.compare(0,1);
    if(a.equal()){a.load(3,15,0x1a4);a.value(0,1);a.store<1>(3,0x1bd,0);increment_match_totals(a,0x10232060,200);a.jump(0x102320a4);}
    else increment_match_totals(a,0x10232084,400);
    a.at(0x102320a4);a.load<1>(3,15,0x11);a.offset(0,3,1);a.store<1>(15,0x11,0);a.load<2>(3,15,0x36,true);
    a.offset(0,3,100);a.store<2>(15,0x36,0);apply_success_reaction(a);
}
std::uint32_t choose_play_kind(Call& a) {
    a.at(0x10232320);a.sign16(0,18);a.load(4,15,0x1a4);a.value(3,0);a.compare(0,17);a.store<1>(4,0x791f,3);
    if(a.equal())return carrier_kind17_phase;
    if(a.at_least()){a.at(0x10232348);a.compare(0,19);if(a.at_least())return carrier_other_kind_phase;a.jump(carrier_kind18_phase);return carrier_kind18_phase;}
    a.compare(0,15);if(a.equal())return carrier_kind15_phase;a.jump(carrier_other_kind_phase);return carrier_other_kind_phase;
}
}
void continue_carrier_play_bias(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_play_ready_phase,dependencies,observer);adjust_play_bias(a);adjust_unopposed_cell_bias(a);update_contact_totals(a);state.pc=choose_play_kind(a);
}
}
