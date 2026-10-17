#include "actor_attributes.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_attributes {
namespace {
using Call=SemanticGuestCall;

void advance_injury_pool(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,31,0);a.load(0,30,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(31,0,3);
    if(!a.nonpositive()) {a.store(31,0,29);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(28,0,0);}
}

bool current_injury_allows_selection(Call& a) {
    a.at(0x10188980);a.compare(3,0,false);if(a.equal())return true;
    a.load<1>(3,3,0);a.sign8(0,3,true);if(a.equal())return true;
    a.sign8(0,3);a.compare(0,1);if(a.equal())return true;
    for(auto kind:{2,3,4,0x4c,0x4d,0x4f,0x4e,0x50,0x51}) {a.compare(0,kind);if(a.equal())return true;}
    // An ineligible current injury returns its raw byte, not the -1 sentinel.
    a.jump(0x10188ca8);return false;
}

void find_previous_injury(Call& a) {
    a.at(0x101889ec);a.compare(15,0,false);if(a.equal())return;
    a.load(0,15,0);a.load(3,17,0);a.multiply(0,0,0x24);a.add(3,3,0);a.load<1>(4,3,0x14);
    a.sign8(0,4);a.compare(0,-1);if(a.equal())return;
    a.move(3,17);a.call(injury_category_entry);a.sign8(3,3);a.sign8(0,16);a.compare_registers(0,3);if(!a.equal())return;
    a.load(0,15,0);a.load(3,17,0);a.multiply(0,0,0x24);a.add(3,3,0);a.load<1>(21,3,0x14);
}

void repeated_injury_score(Call& a) {
    advance_injury_pool(a,0x10188aa8);
    a.at(0x10188ad0);a.load(5,31,0);a.value(3,0x55550000);a.load(4,28,0);a.offset(0,3,0x5556);
    a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(3,0,4);a.mask(0,3,1,false,1);a.add(0,3,0);
    a.multiply(0,0,3);a.subtract(0,0,4);a.sign8(3,0);a.offset(0,3,-1);a.sign8(4,0);a.jump(0x10188c70);
}

void scaled_injury_score(Call& a) {
    a.at(0x10188b6c);a.value(0,0x43300000);a.exclusive_flags(3,16,0x80000000);a.store(1,0x4c,3);
    a.value(3,0x80010000);a.load(7,31,0);a.value(4,0x10000);a.store(1,0x48,0);a.offset(6,3,-0x7fff);
    a.floating_load(2,27,8);a.multiply(3,16,10);a.floating_load(1,1,0x48);a.offset(5,4,-1);a.floating_load(0,27,0);
    a.floating_subtract(1,1,2);a.multiply_high(4,6,3);a.load<2>(8,7,0);a.load(7,28,0);a.floating_difference(1,1,0,true);
    a.store(1,0x40,0);a.add(0,8,7);a.multiply_high(6,6,0);a.add(3,4,3);a.add(4,6,0);a.shift_right(4,4,15);
    a.mask(6,4,1,false,1);a.shift_right(3,3,15);a.add(4,4,6);a.product(5,4,5);a.mask(4,3,1,false,1);
    a.add(3,3,4);a.subtract(0,5,0);a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);a.floating_load(0,1,0x40);
    a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load(15,1,0x3c);
    a.call(sampling_entry);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,16,-1);
    a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(15,15,3);
    a.compare_registers(15,0);if(!a.nonpositive()) {a.move(15,0);a.jump(0x10188c6c);}
}

void new_injury_score(Call& a) {
    a.at(0x10188b0c);a.offset(0,24,2);a.indexed_load<1>(16,25,0);a.sign8(16,16,true);
    if(a.equal()) {a.value(15,0);a.jump(0x10188c6c);}
    else {
        advance_injury_pool(a,0x10188b24);
        a.at(0x10188b4c);a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(16,0);
        bool wide=a.positive();
        if(!wide) {a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(16,0);wide=!a.at_least();}
        if(wide)scaled_injury_score(a);
        else {
            a.at(0x10188c40);a.load(3,31,0);a.compare(16,0);a.load(0,28,0);a.load<2>(3,3,0);a.add(3,3,0);
            a.divide(0,3,16);a.product(0,0,16);a.subtract(0,0,3);a.negate(15,0);if(!a.nonpositive())a.move(15,0);
        }
    }
    a.at(0x10188c6c);a.sign8(4,15);
}

void consider_injury(Call& a) {
    a.at(0x10188a68);a.compare(17,-1);
    if(!a.equal()) {
        a.offset(0,24,10);a.indexed_load<1>(0,25,0);a.sign8(0,0);a.compare_registers(17,0);
        if(!a.equal()) {a.compare(0,-1);if(!a.equal())return;}
    }
    a.at(0x10188a8c);a.sign8(0,19);a.compare(0,3);if(a.equal())return;
    a.sign8(3,20);a.sign8(0,21);a.compare_registers(3,0);
    if(a.equal())repeated_injury_score(a);else new_injury_score(a);
    a.at(0x10188c70);a.sign8(3,4);a.sign8(0,23);a.compare_registers(3,0);if(a.at_least())return;
    a.move(23,4);a.move(22,20);
}
}

void select_injury(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,injury_selection_entry,dependencies,observer);
    a.link();a.multiple(15);a.move(15,6);a.move(16,4);a.load(25,2,-0x64d4);a.move(17,3);a.load(26,2,-0x64d0);
    a.move(19,5);a.load(27,2,-0x543c);a.move(4,15);a.load(28,2,-0x7b90);a.value(23,100);a.load(29,2,-0x7b8c);
    a.value(22,0xffffffff);a.load(30,2,-0x7b88);a.value(21,0xffffffff);a.load(31,2,-0x7b84);a.save_link();a.frame(160);
    a.call(current_injury_entry);
    if(current_injury_allows_selection(a)) {
        find_previous_injury(a);
        a.at(0x10188a40);a.sign8(0,16);a.sign8(17,19);a.multiply(4,0,3);a.add(3,26,4);a.load<1>(20,3,1);
        a.offset(18,4,2);a.sign8(0,20);a.multiply(24,0,12);a.jump(0x10188c90);
        for(;;) {
            // The end byte is live on every iteration, including after callbacks.
            a.at(0x10188c90);a.indexed_load<1>(0,26,18);a.sign8(3,20);a.sign8(0,0);a.compare_registers(3,0);if(!a.nonpositive())break;
            consider_injury(a);a.at(0x10188c88);a.offset(24,24,12);a.offset(20,20,1);
        }
        a.at(0x10188ca4);a.move(3,22);
    }
    a.at(0x10188ca8);a.pop(160);a.multiple(15,true);a.finish();
}

void injury_category(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,injury_category_entry,dependencies,observer);
    a.sign8(0,4);a.load(3,2,-0x64d4);a.multiply(0,0,12);a.add(3,3,0);a.load<1>(3,3,1);a.finish();
}

void current_injury(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,current_injury_entry,dependencies,observer);
    a.link();a.store(1,-4,31);a.load(31,2,-0x5438);a.store(1,-8,30);a.load(30,2,-0x7ba8);a.store(1,-12,29);
    a.move(29,4,true);a.store(1,-16,28);a.move(28,3);a.save_link();a.frame(112);
    if(a.equal()) {
        a.offset(3,31,0x53);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0);a.value(5,0);a.call(0x10739890);a.call(0x10768810);
        a.move(5,3);a.move(3,30);a.offset(4,31,0x5e);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(8,0xe23);a.call(0x1077b720);
        a.move(4,30);a.offset(3,31,0x6b);a.value(5,0);a.call(0x10769b60);a.load(4,2,-0x7bac);a.value(0,0);a.value(3,0);a.store<1>(4,0,0);a.jump(0x1018c428);
    } else {
        a.at(0x1018c3cc);bool valid=a.equal();
        if(!valid) {a.load(3,2,-0x7be4);a.load(4,29,0);a.load(0,3,0);a.compare_registers(4,0);valid=a.less();}
        if(!valid) {a.value(3,0);a.jump(0x1018c428);}
        else {
            a.at(0x1018c3ec);a.move(3,28);a.move(4,29);a.call(record_refresh_entry);a.load(0,29,0);a.load(3,28,0);
            a.multiply(0,0,0x24);a.add(3,3,0);a.load(0,3,0x10);a.compare(0,-1);
            if(a.equal()) {a.at(0x1018c424);a.value(3,0);}
            else {a.multiply(0,0,0x38);a.load(3,28,4);a.add(3,3,0);a.jump(0x1018c428);}
        }
    }
    a.at(0x1018c428);a.pop(112);a.load(31,1,-4);a.load(30,1,-8);a.load(29,1,-12);a.load(28,1,-16);a.finish();
}
}
