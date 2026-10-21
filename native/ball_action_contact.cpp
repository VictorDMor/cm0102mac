#include "ball_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_action {
namespace {
using Call = SemanticGuestCall;

void contact_interval(Call& a,std::uint32_t start,bool upper) {
    a.at(start);a.floating_load(1,31,0x3c,true);a.value(3,upper?0x10000:0xffff0000);a.floating_load(0,31,0x2c,true);
    a.offset(3,3,upper?-1:1);a.floating_product_sum(0,1,29,0);a.floating_integer(0,0);a.floating_store(0,1,0x78);
    a.load(0,1,0x7c);a.compare_registers(0,3);
}

void wide_contact_sample(Call& a) {
    a.at(0x1023eb04);a.floating_load(1,31,0x3c,true);a.value(4,0x43300000);a.floating_load(0,31,0x2c,true);
    a.value(6,0x80010000);a.load(7,30,0);a.value(0,0x43300000);a.floating_product_sum(0,1,29,0);a.store(1,0x80,4);
    a.value(5,0x10000);a.value(3,0x80010000);a.load<2>(7,7,0);a.offset(6,6,-0x7fff);a.floating_integer(0,0);
    a.load(4,27,0);a.floating_load(4,26,0x80);a.offset(5,5,-1);a.add(7,7,4);a.floating_load(2,26,0);
    a.floating_store(0,1,0x78);a.multiply_high(6,6,7);a.floating_load(1,31,0x3c,true);a.offset(3,3,-0x7fff);
    a.load(4,1,0x7c);a.store(1,0x88,0);a.exclusive_flags(4,4,0x80000000);a.store(1,0x84,4);a.add(0,6,7);
    a.shift_right(0,0,15);a.floating_load(0,31,0x2c,true);a.floating_load(3,1,0x80);a.mask(4,0,1,false,1);
    a.floating_product_sum(0,1,29,0);a.add(0,0,4);a.floating_subtract(3,3,4);a.product(0,0,5);
    a.floating_load(1,26,0x80);a.floating_integer(0,0);a.floating_difference(2,3,2,true);a.subtract(14,0,7);
    a.floating_store(0,1,0x98);a.exclusive_flags(4,14,0x80000000);a.store(1,0x8c,4);a.load(0,1,0x9c);
    a.floating_load(0,1,0x88);a.multiply(0,0,10);a.floating_subtract(0,0,1);a.multiply_high(3,3,0);a.floating_product(0,2,0);
    a.add(0,3,0);a.floating_integer(0,0);a.shift_right(0,0,15);a.mask(3,0,1,false,1);a.add(3,0,3);
    a.floating_store(0,1,0x90);a.load(14,1,0x94);a.call(0x10226520);
    a.floating_load(1,31,0x3c,true);a.value(4,0x66660000);a.floating_load(0,31,0x2c,true);a.offset(0,3,5);
    a.offset(4,4,0x6667);a.floating_product_sum(0,1,29,0);a.multiply_high(0,4,0);a.floating_integer(0,0);
    a.shift_right(0,0,2);a.floating_store(0,1,0xa0);a.mask(4,0,1,false,1);a.add(0,0,4);a.load(3,1,0xa4);
    a.add(14,14,0);a.offset(0,3,-1);a.compare_registers(14,0);
    if(!a.nonpositive()) {
        a.floating_load(1,31,0x3c,true);a.floating_load(0,31,0x2c,true);a.floating_product_sum(0,1,29,0);
        a.floating_integer(0,0);a.floating_store(0,1,0xa8);a.load(3,1,0xac);a.offset(14,3,-1);a.jump(0x1023eca8);
    }
}

void narrow_contact_sample(Call& a) {
    a.at(0x1023ec48);a.floating_load(1,31,0x3c,true);a.floating_load(0,31,0x2c,true);a.load(3,30,0);
    a.floating_product_sum(0,1,29,0);a.load(0,27,0);a.load<2>(3,3,0);a.floating_load(1,31,0x3c,true);
    a.floating_integer(2,0);a.floating_load(0,31,0x2c,true);a.add(5,3,0);a.floating_product_sum(0,1,29,0);
    a.floating_store(2,1,0x78);a.load(4,1,0x7c);a.floating_integer(0,0);a.divide(3,5,4);a.floating_store(0,1,0x80);
    a.load(0,1,0x84);a.compare(0,0);a.product(0,3,4);a.subtract(14,0,5);a.negate(0,14);
    if(!a.nonpositive())a.move(0,14);
    a.at(0x1023eca4);a.move(14,0);
}

void sample_contact(Call& a) {
    a.floating_load(2,31,0x3c,true);a.floating_load(1,31,0x2c,true);a.floating_load(0,31,0,true);
    a.floating_product_sum(1,2,29,1);
    // The pinned indexed fcmpu has the same architectural writes as fcmpo.
    a.floating_compare(1,0);
    if(a.equal()){a.value(14,0);a.jump(0x1023eca8);return;}
    a.at(0x1023ea8c);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    contact_interval(a,0x1023eab4,true);bool wide=a.positive();
    if(!wide){contact_interval(a,0x1023eadc,false);wide=!a.at_least();}
    if(wide)wide_contact_sample(a);else narrow_contact_sample(a);
}

void emit_contact_action(Call& a) {
    a.load<1>(0,1,0x44);a.move(5,15);a.sign8(4,13);a.value(6,0);a.store(1,0x38,0);a.value(7,0);
    a.load<1>(0,1,0x43);a.store(1,0x3c,0);a.load(3,15,0x1a4);a.load<1>(8,1,0x41);
    a.load<1>(9,15,0x105);a.load<1>(10,15,0x106);a.call(0x101d68f0);a.jump(action_execution_phase);
}
}

void prepare_contact(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,contact_selection_phase,dependencies,observer);a.load(3,15,0x1a4);a.move(4,15);a.sign8(5,13);
    a.offset(6,1,0x44);a.offset(7,1,0x43);a.call(0x101f50c0);a.store<1>(1,0x41,3);a.load<1>(0,1,0x41);a.sign8(0,0,true);
    if(!a.equal()) {
        sample_contact(a);a.at(0x1023eca8);a.compare(14,0);
        if(a.equal()){emit_contact_action(a);state.pc=action_execution_phase;return;}
    }
    a.at(0x1023ece8);a.load<1>(0,1,0x41);a.sign8(0,0,true);
    state.pc=a.equal()?action_execution_phase:contact_resolution_phase;
}
}
