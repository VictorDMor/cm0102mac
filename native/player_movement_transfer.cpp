#include "player_movement.hpp"
#include "player_movement_internal.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_transfer_random(Call& a) {
    a.at(0x10234128);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(0x10234150);
}
void penalize_zone_contact(Call& a) {
    a.load<2>(3,15,0x36,true);a.offset(0,3,-250);a.store<2>(15,0x36,0);a.load(3,15,0x6c);
    a.load<1>(0,3,0x66);a.sign8(0,0);a.multiply(3,0,30);a.offset_record(16,3,20);
    if(a.equal()){a.value(14,0);a.jump(0x10234120);}else sample_carrier_condition(a,0x690);
    a.at(0x10234120);a.compare(14,0);if(!a.equal())return;
    advance_transfer_random(a);a.load(3,30,0);a.value(4,0x55550000);a.load(0,27,0);a.offset(4,4,0x5556);
    a.load<2>(5,3,0);a.move(3,15);a.add(5,5,0);a.multiply_high(4,4,5);a.mask(0,4,1,false,1);a.add(0,4,0);
    a.multiply(0,0,3);a.subtract(0,0,5);a.negate(0,0);a.sign8(4,0);a.call(0x10288f30);a.sign8(0,3,true);if(a.equal())return;
    a.move(3,15);a.value(4,0);a.call(0x1022ba10);a.jump(0x102341d0);
}
void penalize_other_contact(Call& a) {
    a.at(0x102341a4);a.move(3,15);a.call(0x10272050);a.sign8(0,3,true);
    if(!a.equal()){a.load<2>(3,15,0x36,true);a.offset(0,3,-100);a.store<2>(15,0x36,0);a.jump(0x102341d0);}
    else {a.at(0x102341c4);a.load<2>(3,15,0x36,true);a.offset(0,3,-20);a.store<2>(15,0x36,0);}
}
void clamp_opponent_bias(Call& a) {
    a.at(0x102341d0);a.load<2>(0,1,0x48,true);a.subtract(3,21,0);a.offset(0,3,-200);a.store<2>(22,0x1a0,0);
    a.load<2>(0,22,0x1a0,true);a.compare(0,200);
    if(!a.nonpositive()){a.value(0,200);a.store<2>(22,0x1a0,0);a.jump(0x1023420c);}
    else {a.at(0x102341f8);a.load<2>(0,22,0x1a0,true);a.compare(0,-200);if(!a.at_least()){a.value(0,std::uint32_t(-200));a.store<2>(22,0x1a0,0);}}
}
void choose_transfer_threshold(Call& a) {
    a.at(0x1023420c);a.load<1>(0,1,0x46);a.value(16,0);a.value(4,100);a.sign8(0,0);a.compare(0,33);
    if(a.equal()){a.at(0x10234240);a.value(4,250);a.jump(0x1023424c);}
    else if(!a.at_least()){a.compare(0,19);if(a.equal()){a.at(0x10234248);a.value(4,400);}else a.jump(0x1023424c);}
    else {a.at(0x10234234);a.compare(0,45);if(a.equal()){a.at(0x10234240);a.value(4,250);a.jump(0x1023424c);}else a.jump(0x1023424c);}
}
bool transfer_statistics_required(Call& a) {
    a.at(0x1023424c);a.load<2>(3,22,0x1a0,true);a.sign16(0,4);a.compare_registers(3,0);if(a.at_least())return true;
    a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.compare(0,4);if(a.equal())return true;
    a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,-1);return a.equal();
}
}
void continue_carrier_transfer(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_transfer_phase,dependencies,observer);a.move(3,22);a.call(0x1022e3a0);
    a.load<1>(4,15,0x27);a.move(3,15);a.call(0x10239d10);a.sign8(0,3,true);
    if(a.equal())penalize_other_contact(a);else penalize_zone_contact(a);
    clamp_opponent_bias(a);choose_transfer_threshold(a);
    if(transfer_statistics_required(a)){state.pc=carrier_statistics_phase;return;}
    a.load<1>(0,15,0x27);a.value(6,1);a.load(3,15,0x1a4);a.value(8,0);a.sign8(0,0);a.load<1>(4,15,0x105);
    a.leading_zeroes(0,0);a.load<1>(5,15,0x106);a.mask(0,0,0x07ffffff,false,27);a.value(9,1);a.sign8(7,0);a.value(10,0);
    a.call(0x101f1100);a.load(3,15,0x1a4);a.load<1>(0,3,0x7928);a.sign8(0,0,true);
    state.pc=a.equal()?carrier_distance_phase:0x102346e4;
}
}
