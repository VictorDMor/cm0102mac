#include "ball_contact.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_contact {
namespace {
using Call=SemanticGuestCall;
void load_weight(Call& a,unsigned source,unsigned index,unsigned base) {
    a.sign8(source,31);a.mask(index,source,0xfffffffc,false,2);a.offset(base,1,0x40);a.floating_indexed_single(2,base,index);
}
void load_opponent(Call& a,unsigned index,unsigned zeroes,unsigned opposite,unsigned offset,unsigned base,unsigned player) {
    a.sign8(index,31);a.leading_zeroes(zeroes,index);a.mask(opposite,zeroes,0x07ffffff,false,27);a.mask(offset,opposite,0xfffffffc,false,2);
    a.offset(base,1,0x48);a.indexed_load(player,base,offset);
}
void scale_opponent_weight(Call& a,unsigned player,unsigned constant,unsigned base,unsigned index,bool multiply=false) {
    a.floating_load(1,player,0xdc,true);a.floating_load(0,30,constant);
    if(multiply)a.floating_product(0,1,0);else a.floating_difference(0,1,0,true);
    a.floating_round_single(0,0);a.floating_product_single(0,2,0);a.floating_indexed_store_single(0,base,index);
}
void double_sample(Call& a,unsigned index,unsigned base,unsigned scaled) {
    a.sign8(index,31);a.offset(base,1,0x38);a.indexed_load<1>(0,base,index);a.mask(scaled,0,0xfffffffe,false,1);a.sign8(0,scaled);a.indexed_store<1>(0,base,index);
}
void scale_sample_fraction(Call& a) {
    a.floating_load(2,30,0x2c8);a.sign8(0,31);a.offset(7,1,0x38);a.indexed_load<1>(6,7,0);a.sign8(0,6);a.floating_load(1,30,0x60);
    a.exclusive_flags(0,0,0x80000000);a.store(1,0x5c,0);a.value(0,0x43300000);a.store(1,0x58,0);a.floating_load(0,1,0x58);a.floating_subtract(0,0,1);
    a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x50);a.load(0,1,0x54);a.sign8(7,31);a.offset(6,1,0x38);a.indexed_store<1>(0,6,7);
}
}
void adjust_description_weight(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,description_weight_phase,dependencies,observer);auto target=state.pc;
    if(target==description_weight_phase) {
        a.sign8(6,31);a.mask(5,6,0xfffffffc,false,2);a.offset(6,1,0x48);a.indexed_load(7,6,5);a.compare(7,0,false);
        if(a.equal()){state.pc=description_next_phase;return;}
        a.sign8(6,31);a.mask(0,6,0xfffffffc,false,2);a.offset(7,1,0x48);a.indexed_load(0,7,0);a.offset(8,29,0x10000);a.load(7,8,-0x2000);a.compare_registers(0,7,false);
        if(!a.equal()){state.pc=description_assessment_phase;return;}
        a.sign8(0,31);a.leading_zeroes(0,0);a.mask(7,0,0x07ffffff,false,27);a.mask(0,7,0xfffffffc,false,2);a.offset(6,1,0x48);a.indexed_load(0,6,0);a.compare(0,0,false);
        if(a.equal()){state.pc=description_assessment_phase;return;}
        a.sign8(7,31);a.leading_zeroes(0,7);a.mask(0,0,0x07ffffff,false,27);a.mask(7,0,0xfffffffe,false,1);a.offset(6,1,0x3c);a.indexed_load<2>(5,6,7,true);a.compare(5,47,false);
        if(a.positive())target=0x101f5e64;
        else {a.load(6,2,-0x52e0);a.mask(7,5,0xfffffffc,false,2);a.indexed_load(6,6,7);a.count_destination(6);target=a.count_branch();}
    }
    a.at(target);bool jump=true;
    switch(target) {
    case 0x101f5ab8:load_weight(a,6,10,9);load_opponent(a,0,7,8,0,6,7);scale_opponent_weight(a,7,0x48,9,10);break;
    case 0x101f5afc:load_weight(a,6,10,9);load_opponent(a,7,7,0,8,7,6);scale_opponent_weight(a,6,0x130,9,10);break;
    case 0x101f5b40:load_weight(a,0,8,7);load_opponent(a,0,3,6,0,5,6);scale_opponent_weight(a,6,0x2b0,7,8);double_sample(a,7,6,5);break;
    case 0x101f5b9c:load_weight(a,0,9,8);load_opponent(a,5,0,0,7,6,5);scale_opponent_weight(a,5,0x2b8,8,9,true);break;
    case 0x101f5be0:load_weight(a,0,8,7);load_opponent(a,0,0,5,0,6,3);scale_opponent_weight(a,3,0x2c0,7,8);double_sample(a,5,4,3);break;
    case 0x101f5c3c:load_weight(a,0,7,4);load_opponent(a,0,0,3,0,5,6);scale_opponent_weight(a,6,0x48,4,7);break;
    case 0x101f5c80:load_weight(a,0,9,8);load_opponent(a,0,0,0,7,6,5);scale_opponent_weight(a,5,0xc0,8,9,true);scale_sample_fraction(a);break;
    case 0x101f5d10:load_weight(a,5,10,9);load_opponent(a,6,0,0,0,8,7);scale_opponent_weight(a,7,0xc0,9,10,true);break;
    case 0x101f5d54:load_weight(a,0,5,9);load_opponent(a,0,0,6,0,7,8);scale_opponent_weight(a,8,0x2d0,9,5);break;
    case 0x101f5d98:load_weight(a,7,8,5);load_opponent(a,0,6,0,0,7,7);scale_opponent_weight(a,7,0x2d8,5,8);break;
    case 0x101f5ddc:load_weight(a,0,9,8);load_opponent(a,5,0,0,6,7,7);scale_opponent_weight(a,7,0x48,8,9);break;
    case 0x101f5e20:load_weight(a,0,5,6);load_opponent(a,0,0,7,0,7,8);scale_opponent_weight(a,8,0x2e0,6,5);break;
    case 0x101f5e64:load_weight(a,7,0,5);load_opponent(a,6,6,7,8,7,7);scale_opponent_weight(a,7,0x30,5,0);jump=false;break;
    default:state.pc=target;return;
    }
    if(jump)a.jump(description_assessment_phase);state.pc=description_assessment_phase;
}
}
