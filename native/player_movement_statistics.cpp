#include "player_movement.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_statistics_random(Call& a,std::uint32_t site) {
    a.at(site);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(site+0x28);
}
void position_after_transfer(Call& a) {
    a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,4);
    if(!a.at_least()){a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,-1);
        if(!a.nonpositive()){a.load(3,15,0x1a4);a.value(0,0);a.store<1>(3,0x791f,0);}}
    a.at(0x1023472c);a.load<1>(0,22,0x27);a.sign8(0,0,true);bool toward_end=a.equal();
    if(toward_end){a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,-1);toward_end=!a.equal();}
    if(toward_end){a.load<1>(0,15,0x106);a.move(3,22);a.load<1>(4,15,0x105);a.value(6,4);a.sign8(0,0);a.load<1>(5,15,0x106);a.compare(0,9);
        if(!a.at_least()){a.load<1>(0,15,0x106);a.sign8(7,0);a.offset(0,7,3);a.jump(0x10234780);}else {a.at(0x1023477c);a.value(0,11);}
        a.sign8(7,0);a.call(0x10237830);a.jump(0x102347c8);
    }else {a.at(0x1023478c);a.load<1>(0,15,0x106);a.move(3,22);a.load<1>(4,15,0x105);a.value(6,4);a.sign8(0,0);a.load<1>(5,15,0x106);a.compare(0,3);
        if(!a.nonpositive()){a.load<1>(0,15,0x106);a.sign8(7,0);a.offset(0,7,-3);a.jump(0x102347c0);}else {a.at(0x102347bc);a.value(0,0);}
        a.sign8(7,0);a.call(0x10237830);}
}
void compute_transfer_ratio(Call& a,std::uint32_t site,std::int32_t denominator,std::int32_t numerator) {
    a.at(site);a.load<1>(0,22,denominator);a.compare(0,0,false);
    if(!a.equal()){a.load<1>(5,22,numerator);a.value(4,0x43300000);a.load<1>(3,22,denominator);a.value(0,0x43300000);
        a.store(1,0xc0,5);a.floating_load(2,26,0xf0);a.store(1,0xbc,4);a.floating_load(1,26,0xf0);a.floating_load(0,1,0xbc);
        a.store(1,0xc8,3);a.floating_subtract(2,0,2,true);a.store(1,0xc4,0);a.floating_load(0,1,0xc4);a.floating_subtract(0,0,1,true);
        a.floating_difference(30,2,0,true,true);a.jump(site+0x50);
    }else {a.at(site+0x4c);a.floating_load(30,31,0,true);}
}
bool transfer_commentary_due(Call& a,std::uint32_t site,std::int32_t minimum) {
    advance_statistics_random(a,site);a.load(3,30,0);a.value(4,0x66660000);a.load(0,27,0);a.offset(4,4,0x6667);a.load<2>(5,3,0);a.sign8(3,13);
    a.add(5,5,0);a.multiply_high(0,4,5);a.shift_right(0,0,3);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,20);a.subtract(4,0,5);
    a.offset(0,4,minimum);a.sign8(0,0);a.compare_registers(3,0);if(a.nonpositive())return false;
    a.move(3,22);a.call(0x10237b10);a.sign8(0,3,true);if(!a.equal())return false;
    a.floating_load(0,26,0x288);a.floating_compare(30,0);return !a.nonpositive();
}
void describe_transfer_ratio(Call& a,std::uint32_t site,std::uint32_t code,bool branch) {
    a.at(site);a.load(3,15,0x1a4);a.value(4,code);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
    a.load<1>(6,22,0x2f);a.load<1>(7,22,0x27);a.load<1>(8,15,0x2f);a.load<1>(9,15,0x27);a.call(0x10201180);
    a.value(0,1);a.store<1>(22,0x100,0);if(branch)a.jump(carrier_ratings_phase);
}
void consider_transfer_ratio_commentary(Call& a,bool shot) {
    compute_transfer_ratio(a,shot?0x102347fc:0x10234938,shot?7:4,shot?8:5);
    if(!transfer_commentary_due(a,shot?0x1023484c:0x10234988,shot?55:65))return;
    if(shot){a.load<1>(0,22,8);a.compare(0,3,false);if(a.nonpositive())return;a.load<1>(0,22,9);a.compare(0,0,false);if(a.equal())return;}
    else {a.load<1>(0,22,6);a.compare(0,1,false);if(a.nonpositive())return;}
    a.load<2>(0,22,0x36,true);a.compare(0,6750);if(a.nonpositive())return;
    a.load<1>(0,22,0x100);a.sign8(0,0,true);if(!a.equal())return;
    describe_transfer_ratio(a,shot?0x10234904:0x10234a34,shot?0x21ce:0x21cd,shot);
}
}
void continue_carrier_statistics(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_statistics_phase,dependencies,observer);position_after_transfer(a);
    a.at(0x102347c8);a.load(3,15,0x1a4);a.value(4,0x2e8c0000);a.load<1>(0,1,0x46);a.offset(4,4,-0x5d17);
    a.load<2>(3,3,0x7948,true);a.sign8(0,0);a.compare(0,21);a.multiply_high(0,4,3);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.sign8(13,0);
    consider_transfer_ratio_commentary(a,a.equal());state.pc=carrier_ratings_phase;
}
}
