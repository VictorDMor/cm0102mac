#include "player_movement.hpp"
#include "player_movement_internal.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void prepare_kind18_action(Call& a) {
    a.load<1>(0,15,0x27);a.move(3,15);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);
    a.sign8(4,0);a.call(0x10239d10);a.sign8(0,3,true);bool restricted=!a.equal();
    if(!restricted){a.sign8(0,20);a.compare(0,4);restricted=a.equal();}
    if(restricted){a.at(0x10232e60);a.value(19,0);}
}
void describe_kind18_space(Call& a,std::uint32_t site,std::uint32_t code) {
    a.at(site);a.load(3,15,0x1a4);a.value(4,code);a.load<1>(7,15,0x27);a.value(8,UINT32_MAX);a.load(3,3,0x79f4);
    a.value(9,UINT32_MAX);a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);a.jump(noncarrier_phase);
}
void consider_kind18_space(Call& a) {
    a.at(0x10233158);a.sign8(0,19,true);if(a.equal())return;
    a.move(3,15);a.call(0x10272100);a.sign8(0,3,true);if(a.equal())return;
    a.load<1>(0,15,0x27);a.value(7,1);a.load(3,15,0x1a4);a.sign8(0,0);a.load<1>(5,15,0x105);a.leading_zeroes(0,0);
    a.load<1>(6,15,0x106);a.mask(0,0,0x07ffffff,false,27);a.sign8(4,0);a.call(0x101e3f10);a.floating_load(0,26,0x90);
    a.floating_compare(1,0);if(a.nonpositive())return;
    a.load<1>(0,15,0x27);a.load(3,15,0x1a4);a.sign8(0,0);a.leading_zeroes(0,0);a.offset(3,3,0x10000);
    a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0);a.multiply(0,0,0x18e0);a.add(3,3,0);a.load(0,3,-0x7e0c);a.mask(0,0,0x400,true);
    if(!a.equal()){
        a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
        if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
        a.at(0x102331fc);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(0,3,0);a.mask(3,0,1,false,1);
        a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0,true);
        if(a.equal()){describe_kind18_space(a,0x10233220,0x20de);return;}
    }
    describe_kind18_space(a,0x1023324c,0x1f45);
}
}
void continue_carrier_kind18(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_kind18_phase,dependencies,observer);prepare_kind18_action(a);a.at(0x10232e64);a.compare(22,0,false);
    if(a.equal())complete_carrier_clear_position(a,0xa00);
    else {describe_carrier_opposition(a,0xa68);consider_kind18_space(a);}
    state.pc=noncarrier_phase;
}
}
