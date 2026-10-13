#include "ball_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

// Four adjustments use the same schedule but different live halfword inputs.
// It differs from word-interval sampling in both conversion and spill order.
void sample_adjustment(Call& a,std::uint32_t start,unsigned interval,unsigned result) {
    const auto done=start+0x188;
    a.at(start);a.sign16(0,interval,true);
    if(a.equal()){a.value(result,0);a.jump(done);return;}
    a.at(start+0x10);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(start+0x38);a.value(3,0x10000);a.sign16(0,interval);a.offset(3,3,-1);a.compare_registers(0,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.sign16(0,interval);a.offset(3,3,1);a.compare_registers(0,3);wide=!a.at_least();}
    if(wide) {
        a.at(start+0x60);a.sign16(3,interval);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store(1,0x78,0);
        a.load(7,30,0);a.value(6,0x80010000);a.store(1,0x7c,3);a.value(4,0x43300000);a.floating_load(2,26,0x80);
        a.value(5,0x10000);a.floating_load(1,1,0x78);a.sign16(0,interval);a.load<2>(8,7,0);a.value(3,0x80010000);
        a.floating_subtract(1,1,2);a.floating_load(0,26,0);a.load(7,27,0);a.offset(6,6,-0x7fff);a.store(1,0x80,4);
        a.offset(5,5,-1);a.floating_difference(2,1,0,true);a.add(7,8,7);a.multiply_high(4,6,7);a.floating_load(1,26,0x80);
        a.offset(3,3,-0x7fff);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.add(4,4,6);
        a.product(4,4,5);a.subtract(result,4,7);a.exclusive_flags(4,result,0x80000000);a.store(1,0x84,4);a.multiply(0,0,10);
        a.floating_load(0,1,0x80);a.multiply_high(3,3,0);a.floating_subtract(0,0,1);a.floating_product(0,2,0);a.add(0,3,0);
        a.shift_right(0,0,15);a.mask(3,0,1,false,1);a.floating_integer(0,0);a.add(3,0,3);a.floating_store(0,1,0x88);
        a.load(result,1,0x8c);a.call(0x10226520);a.value(4,0x66660000);a.offset(0,3,5);a.offset(4,4,0x6667);
        a.sign16(3,interval);a.multiply_high(4,4,0);a.offset(0,3,-1);a.shift_right(3,4,2);a.mask(4,3,1,false,1);
        a.add(3,3,4);a.add(result,result,3);a.compare_registers(result,0);
        if(!a.nonpositive()){a.sign16(3,interval);a.offset(result,3,-1);a.jump(done);}
    } else {
        a.at(start+0x154);a.load(5,30,0);a.sign16(3,interval);a.load(4,27,0);a.sign16(0,interval,true);a.load<2>(0,5,0);
        a.add(4,0,4);a.divide(0,4,3);a.product(0,0,3);a.subtract(result,0,4);a.negate(0,result);
        if(!a.nonpositive())a.move(0,result);
        a.at(start+0x184);a.move(result,0);
    }
}
}

void sample_selected_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,selected_action_phase,dependencies,observer);
    sample_adjustment(a,selected_action_phase,17,14);sample_adjustment(a,0x1023e58c,16,17);
    sample_adjustment(a,0x1023e714,21,18);sample_adjustment(a,0x1023e89c,20,19);
    a.at(0x1023ea24);a.add(0,14,17);a.floating_load(1,31,0xe0,true);a.subtract(0,18,0);a.move(3,15);
    a.subtract(0,19,0);a.sign8(5,13);a.sign16(17,0);a.call(0x10239590);state.pc=contact_selection_phase;
}
}
