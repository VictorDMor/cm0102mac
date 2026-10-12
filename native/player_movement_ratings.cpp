#include "player_movement.hpp"
#include "player_movement_internal.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_rating_random(Call& a,std::uint32_t site) {
    a.at(site);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(site+0x28);
}
void choose_transfer_rating(Call& a) {
    a.load<1>(4,22,5);a.move(3,22);a.offset(0,4,1);a.store<1>(22,5,0);a.call(0x10289560);a.mask(0,3,4,true);bool role=!a.equal();
    if(!role){a.move(3,22);a.call(0x10289560);a.mask(0,3,2,true);role=!a.equal();}
    if(!role){a.sign8(0,16,true);a.value(0,200);if(!a.equal())a.value(0,120);a.at(0x10234aa0);a.mask(0,0,0xfffffffe,false,1);a.sign16(13,0);a.jump(0x10234ac0);}
    else {a.at(0x10234aac);a.sign8(0,16,true);a.value(0,200);if(!a.equal())a.value(0,120);a.at(0x10234abc);a.sign16(13,0);}
}
void adjust_transfer_rating(Call& a) {
    a.at(0x10234ac0);a.move(3,15);a.call(0x10272050);a.sign8(0,3,true);if(!a.equal())return;
    a.move(3,15);a.call(0x10272100);a.sign8(0,3,true);if(!a.equal())return;
    a.sign16(3,13);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store(1,0xbc,0);a.floating_load(1,26,0x80);
    a.store(1,0xc0,3);a.floating_load(2,26,0x1a0);a.floating_load(0,1,0xbc);a.floating_subtract(0,0,1);a.floating_product(0,2,0);
    a.floating_integer(0,0);a.floating_store(0,1,0xc4);a.load(13,1,0xc8);
}
bool successful_transfer(Call& a) {
    a.at(0x10234b14);a.load<2>(3,22,0x36,true);a.sign8(0,19,true);a.add(0,3,13);a.store<2>(22,0x36,0);if(!a.equal())return true;
    a.move(3,22);a.call(0x10270ba0);a.sign8(0,3,true);return !a.equal();
}
void reward_successful_transfer(Call& a) {
    a.at(0x10234b38);a.load<1>(3,22,6);a.offset(0,3,1);a.store<1>(22,6,0);a.load<2>(3,22,0x36,true);a.offset(0,3,300);a.store<2>(22,0x36,0);
    a.load<1>(0,22,0x17);a.sign8(0,0);a.multiply(3,0,30);a.offset_record(14,3,50);
    if(a.equal()){a.value(13,0);a.jump(0x10234cc8);}else sample_carrier_interest(a,0x3ca4);
    a.at(0x10234cc8);a.compare(13,0);if(!a.equal())return;
    advance_rating_random(a,0x10234cd0);a.load(3,30,0);a.value(4,0x66660000);a.load(0,27,0);a.offset(4,4,0x6667);a.load<2>(5,3,0);
    a.move(3,22);a.add(5,5,0);a.multiply_high(0,4,5);a.shift_right(0,0,1);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,5);a.subtract(0,0,5);
    a.sign8(4,0);a.call(0x10288f30);a.sign8(0,3,true);if(a.equal())return;a.move(3,22);a.value(4,0);a.call(0x1022ba10);
}
void finish_transfer_commentary(Call& a) {
    a.at(0x10234d48);a.load<1>(3,15,0x2c);a.sign8(0,16,true);a.load<1>(0,1,0x45);a.add(0,3,0);a.store<1>(15,0x2c,0);if(a.equal())return;
    advance_rating_random(a,0x10234d60);a.load(4,30,0);a.value(3,0x92490000);a.load(0,27,0);a.offset(3,3,0x2493);a.load<2>(4,4,0);
    a.add(4,4,0);a.multiply_high(0,3,4);a.add(0,0,4);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,7);a.subtract(0,0,4,true);
    if(!a.equal())return;a.load(3,15,0x1a4);a.value(4,0x1f41);a.load<1>(7,22,0x27);a.value(8,UINT32_MAX);a.load(3,3,0x79f4);a.value(9,UINT32_MAX);
    a.load<1>(6,22,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);a.jump(common_return_phase);
}
}
void continue_carrier_ratings(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_ratings_phase,dependencies,observer);choose_transfer_rating(a);adjust_transfer_rating(a);
    if(successful_transfer(a))reward_successful_transfer(a);finish_transfer_commentary(a);state.pc=common_return_phase;
}
void continue_carrier_null_contact(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_null_contact_phase,dependencies,observer);a.offset(3,14,0x8f);a.offset(6,1,0x60);a.offset(7,1,0x50);a.value(4,0);a.value(5,0);a.call(0x10739890);
    a.call(0x10768810);a.move(5,3);a.load(3,2,-0x7ba8);a.offset(4,14,0x9c);a.offset(6,1,0x60);a.offset(7,1,0x50);a.value(8,0x10e9);a.call(0x1077b720);
    a.load(4,2,-0x7ba8);a.offset(3,14,0xa9);a.value(5,0);a.call(0x10769b60);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);state.pc=noncarrier_phase;
}
}
