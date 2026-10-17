#include "actor_attributes.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_attributes {
namespace {
using Call=SemanticGuestCall;

void capacity_percentage(Call& a) {
    a.load(3,30,0);a.value(0,0x43300000);a.load(4,2,-0x543c);a.multiply(5,3,0x24);
    a.load(6,29,0);a.load(3,2,-0x5444);a.store(1,0x40,0);a.add(5,6,5);a.floating_load(2,4,8);
    a.load<2>(0,5,10,true);a.floating_load(0,3,0,true);a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);
    a.floating_load(1,1,0x40);a.floating_subtract(1,1,2,true);a.floating_difference(0,1,0,true,true);
    a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load(3,1,0x3c);a.jump(0x101878b4);
}

void adjust_depleted_scalar(Call& a) {
    a.value(3,0x43300000);a.exclusive_flags(0,5,0x80000000);a.store(1,0x4c,0);a.sign16(0,6);
    a.exclusive_flags(0,0,0x80000000);a.floating_load(3,29,8);a.store(1,0x48,3);a.floating_load(4,29,0x10);
    a.floating_load(0,1,0x48);a.store(1,0x44,0);a.floating_subtract(2,0,3);a.floating_load(1,29,0x18);
    a.store(1,0x40,3);a.floating_load(0,1,0x40);a.floating_product_difference(1,4,2,1);a.floating_subtract(0,0,3);
    a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load(0,1,0x3c);
    a.add(0,5,0);a.store<2>(7,12,0);a.jump(0x10186cbc);
}

void wide_assignment_draw(Call& a) {
    a.at(0x10186b98);a.value(4,0x43300000);a.exclusive_flags(0,28,0x80000000);a.store(1,0x3c,0);a.value(5,0x80010000);
    a.load(7,31,0);a.value(3,0x10000);a.store(1,0x38,4);a.offset(6,5,-0x7fff);a.floating_load(2,29,8);
    a.multiply(0,28,10);a.floating_load(1,1,0x38);a.offset(5,3,-1);a.floating_load(0,29,0);a.floating_subtract(1,1,2);
    a.multiply_high(3,6,0);a.load<2>(8,7,0);a.load(7,30,0);a.floating_difference(1,1,0,true);a.store(1,0x40,4);
    a.add(7,8,7);a.multiply_high(4,6,7);a.add(0,3,0);a.add(3,4,7);a.shift_right(3,3,15);a.mask(4,3,1,false,1);
    a.shift_right(0,0,15);a.add(3,3,4);a.product(4,3,5);a.mask(3,0,1,false,1);a.add(3,0,3);a.subtract(0,4,7);
    a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);a.floating_load(0,1,0x40);a.floating_subtract(0,0,2);
    a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x48);a.load(29,1,0x4c);a.call(0x101864a0);
    a.value(4,0x66660000);a.offset(0,3,5);a.offset(3,4,0x6667);a.offset(4,28,-1);a.multiply_high(0,3,0);
    a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.add(29,29,0);a.compare_registers(29,4);
    if(a.nonpositive())return;
    a.move(29,4);a.jump(0x10186c98);
}

void assignment_scalar_draw(Call& a) {
    a.sign16(28,6,true);
    if(a.equal()) {a.value(29,0);a.jump(0x10186c98);return;}
    a.at(0x10186b48);a.load(3,2,-0x7b88);a.load(4,31,0);a.load(0,3,0);a.offset(3,4,2);a.compare_registers(3,0,false);a.store(31,0,3);
    if(!a.nonpositive()) {a.load(0,2,-0x7b8c);a.store(31,0,0);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(30,0,0);}
    a.at(0x10186b78);a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(28,0);bool wide=a.positive();
    if(!wide) {a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(28,0);wide=!a.at_least();}
    if(wide) {wide_assignment_draw(a);return;}
    a.at(0x10186c6c);a.load(3,31,0);a.compare(28,0);a.load(0,30,0);a.load<2>(3,3,0);a.add(3,3,0);
    a.divide(0,3,28);a.product(0,0,28);a.subtract(0,0,3);a.negate(29,0);if(!a.nonpositive())a.move(29,0);
}

void update_assignment_scalar(Call& a) {
    a.load(0,27,0);a.load(3,26,0);a.multiply(0,0,0x24);a.add(7,3,0);a.load<2>(5,7,12,true);a.compare(5,-1000);
    if(!a.at_least()) {adjust_depleted_scalar(a);return;}
    a.at(0x10186aec);a.load(3,27,0x14);a.load<1>(4,27,0x67);a.load<1>(0,3,0x44);a.load<1>(3,3,0x3c);
    a.sign8(4,4);a.sign8(0,0);a.sign8(3,3);a.add(0,0,4);a.add(0,3,0);a.sign16(0,0);a.subtract_from(0,0,1200);a.compare_registers(5,0);
    if(!a.at_least()) {a.sign16(0,6);a.add(0,5,0);a.store<2>(7,12,0);a.jump(0x10186cbc);return;}
    a.at(0x10186b30);a.compare(5,1250);if(a.at_least())return;
    assignment_scalar_draw(a);
    a.at(0x10186c98);a.load(3,27,0);a.offset(0,29,1);a.load(4,26,0);a.sign16(0,0);a.multiply(3,3,0x24);
    a.add(4,4,3);a.load<2>(3,4,12,true);a.add(0,3,0);a.store<2>(4,12,0);
}

void report_missing_retirement_record(Call& a) {
    a.offset(3,28,0x53);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0);a.value(5,0);a.call(0x10739890);a.call(0x10768810);
    a.move(5,3);a.move(3,27);a.offset(4,28,0x5e);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(8,0xc70);a.call(0x1077b720);
    a.move(4,27);a.offset(3,28,0x6b);a.value(5,0);a.call(0x10769b60);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.jump(0x1018bb38);
}

void retire_live_injury(Call& a) {
    a.at(0x1018ba88);a.load(0,30,0);a.load(3,29,0);a.multiply(0,0,0x24);a.add(5,3,0);a.load(0,5,0x10);a.compare(0,-1);
    if(a.equal())return;
    a.multiply(0,0,0x38);a.load(3,29,4);a.add(27,3,0);a.load<1>(0,27,0);a.compare(0,0x51);
    if(!a.equal()) {a.load(3,29,12);a.move(6,27);a.offset(4,5,0x18);a.offset(5,5,0x1c);a.call(0x10748ce0);}
    a.at(0x1018bad0);a.sign8(0,31,true);
    if(!a.equal()) {
        a.load(3,30,0x24);a.compare(3,0,false);
        if(!a.equal()) {
            a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
            if(!a.equal()) {a.load<1>(6,27,0);a.move(3,29);a.move(4,30);a.value(5,1);a.value(7,0);a.value(8,0);a.call(0x1018dcb0);}
        }
    }
    a.at(0x1018bb1c);a.value(4,0xffffffff);a.store<1>(27,0,4);a.load(0,30,0);a.load(3,29,0);a.multiply(0,0,0x24);a.add(3,3,0);a.store(3,0x10,4);
}

void remove_queued_record_updates(Call& a) {
    a.load(3,27,0x28);a.compare(3,0,false);if(a.equal())return;
    a.load(0,3,4);a.store(3,12,0);a.jump(0x10186eec);
    for(;;) {
        a.at(0x10186eec);a.load(3,27,0x28);a.call(0x10738e00);a.move(4,3,true);if(a.equal())return;
        a.at(0x10186ed8);a.load(0,4,0);a.compare_registers(0,28);
        if(a.equal()) {a.load(3,27,0x28);a.call(0x10738e60);}
    }
}

void advance_scalar_pool(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()) {a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(start+0x28);
}

void subtract_random_scalar(Call& a) {
    advance_scalar_pool(a,0x10187568);
    a.load(5,30,0);a.value(3,0x4ec50000);a.load(0,24,0);a.offset(4,3,-0x13b1);a.load<2>(6,5,0);a.load(5,27,0);
    a.add(3,0,26);a.load<2>(0,3,12,true);a.add(6,6,5);a.multiply_high(4,4,6);a.shift_right(4,4,2);a.mask(5,4,1,false,1);
    a.add(4,4,5);a.multiply(4,4,13);a.subtract(4,4,6);a.sign16(4,4);a.subtract(0,4,0);a.store<2>(3,12,0);a.jump(0x101875f0);
}

void decay_condition_scalar(Call& a) {
    a.at(0x101874e4);a.load(0,24,0);a.add(3,0,26);a.load<2>(0,3,12,true);a.compare(0,-500);
    if(!a.less()) {
        advance_scalar_pool(a,0x101874f8);
        a.load(5,30,0);a.value(3,0x51ec0000);a.load(4,27,0);a.offset(0,3,-0x7ae1);a.load<2>(3,5,0);
        a.offset(5,25,0x5c);a.load(6,31,0);a.add(4,3,4);a.multiply_high(0,0,4);a.indexed_load<2>(3,6,5,true);a.sign8(5,3);
        a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,25);a.subtract(0,0,4);a.compare_registers(5,0);
        if(a.at_least()) {
            a.at(0x101875dc);a.load(0,24,0);a.add(4,0,26);a.load<2>(3,4,12,true);a.offset(0,3,-13);a.store<2>(4,12,0);return;
        }
    }
    subtract_random_scalar(a);
}

void cap_special_scalar(Call& a) {
    a.at(0x101875f0);a.load(0,31,0);a.add(3,0,25);a.load(3,3,0x14);a.compare(3,0,false);if(a.equal())return;
    a.load<1>(0,3,15);a.compare(0,20);if(!a.equal())return;
    a.load(0,24,0);a.value(3,1050);a.add(4,0,26);a.load<2>(0,4,12,true);a.compare(0,1050);
    if(!a.at_least())a.move(3,0);
    a.at(0x1018762c);a.store<2>(4,12,3);
}
}

void record_capacity(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,capacity_entry,dependencies,observer);
    a.link();a.save(31);a.move(31,5);a.save(30);a.move(30,4,true);a.save(29);a.move(29,3);a.save_link();a.frame(96);
    bool valid=true;
    if(a.equal()) {a.value(3,0);a.jump(0x101878b4);valid=false;}
    else {
        a.at(0x10187810);
        if(!a.equal()) {
            a.load(3,2,-0x7be4);a.load(4,30,0);a.load(0,3,0);a.compare_registers(4,0);
            if(!a.less()) {a.sign8(0,31,true);a.value(3,10000);if(!a.equal()) {a.value(3,100);a.jump(0x101878b4);}valid=false;}
        }
        if(valid) {
            a.at(0x1018783c);a.move(3,29);a.move(4,30);a.call(record_refresh_entry);a.sign8(0,31,true);
            if(!a.equal())capacity_percentage(a);
            else {a.at(0x101878a0);a.load(0,30,0);a.load(3,29,0);a.multiply(0,0,0x24);a.add(3,3,0);a.load<2>(3,3,10,true);}
        }
    }
    a.at(0x101878b4);a.pop(96);a.restore(31);a.restore(30);a.restore(29);a.finish();
}

void assign_capacity(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,capacity_assignment_entry,dependencies,observer);
    a.link();a.multiple(26);a.move(27,4,true);a.load(29,2,-0x543c);a.load(30,2,-0x7b90);a.move(26,3);a.load(31,2,-0x7b84);a.save_link();a.frame(112);
    bool valid=!a.equal();
    if(valid && !a.equal()) {a.load(3,2,-0x7be4);a.load(4,27,0);a.load(0,3,0);a.compare_registers(4,0);valid=!a.at_least();}
    if(valid) {a.at(0x10186a60);a.load(0,27,0);a.load(3,26,0);a.multiply(0,0,0x24);a.add(3,3,0);a.store<2>(3,10,5);update_assignment_scalar(a);}
    a.at(0x10186cbc);a.pop(112);a.multiple(26,true);a.finish();
}

void retire_injury(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,retirement_entry,dependencies,observer);
    a.link();a.multiple(27);a.move(30,4,true);a.load(27,2,-0x7ba8);a.load(28,2,-0x5438);a.move(29,3);a.move(31,5);a.save_link();a.frame(128);
    if(a.equal())report_missing_retirement_record(a);
    else {
        a.at(0x1018ba70);bool valid=true;
        if(!a.equal()) {a.load(3,2,-0x7be4);a.load(4,30,0);a.load(0,3,0);a.compare_registers(4,0);valid=!a.at_least();}
        if(valid)retire_live_injury(a);
    }
    a.at(0x1018bb38);a.pop(128);a.multiple(27,true);a.finish();
}

void reset_condition_record(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,reset_entry,dependencies,observer);
    a.link();a.multiple(27);a.move(27,3);a.load(30,2,-0x7be0);a.load(31,2,-0x7b80);a.move(28,4);a.save_link();a.frame(80);
    remove_queued_record_updates(a);
    a.at(0x10186efc);a.multiply(0,28,0x24);a.load(3,27,0);a.add(29,3,0);a.load(0,29,0x10);a.compare(0,-1);
    if(!a.equal()) {a.multiply(0,28,0x70);a.load(4,30,0);a.move(3,27);a.value(5,0);a.add(4,4,0);a.call(retirement_entry);}
    a.at(0x10186f2c);a.multiply(0,28,0x70);a.load(4,30,0);a.move(3,27);a.add(4,4,0);a.call(0x1018fdf0);
    a.load<2>(0,31,2);a.multiply(3,28,100);a.value(6,10000);a.load(4,2,-0x7b9c);a.store<2>(29,2,0);a.value(5,0);a.value(0,0xffffffff);
    a.load<2>(7,31,0,true);a.offset(3,3,13);a.store<2>(29,0,7);a.load<1>(7,31,4);a.store<1>(29,4,7);a.store<2>(29,8,6);a.store<2>(29,10,6);
    a.store<2>(29,12,5);a.store(29,0x10,0);a.store<1>(29,0x14,0);a.store(29,0x18,0);a.store(29,0x1c,0);a.store<1>(29,0x21,0);
    a.load(4,4,0);a.indexed_load<1>(0,4,3);a.insert_mask(0,5,0x80,7);a.indexed_store<1>(0,4,3);
    a.pop(80);a.multiple(27,true);a.finish();
}

void maintain_condition_scalar(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,scalar_maintenance_entry,dependencies,observer);
    a.link();a.multiple(24);a.load(5,2,-0x7be4);a.move(24,3);a.load(27,2,-0x7b90);a.load(28,2,-0x7b8c);
    a.load(29,2,-0x7b88);a.load(30,2,-0x7b84);a.load(31,2,-0x7be0);a.save_link();a.frame(96);
    a.load(0,5,0);a.compare_registers(4,0);
    if(!a.at_least()) {
        a.multiply(26,4,0x24);a.load(0,24,0);a.add(5,0,26);a.load<2>(3,5,12,true);a.compare(3,-1000);
        if(!a.at_least()) {a.offset(0,3,-1);a.store<2>(5,12,0);}
        a.at(0x101874a8);a.multiply(25,4,0x70);a.load(3,31,0);a.offset(0,25,0x5c);a.indexed_load<2>(0,3,0,true);a.sign8(0,0);a.compare(0,19);
        bool reset=false;
        if(!a.at_least()) {
            a.load(0,24,0);a.add(3,0,26);a.load<2>(0,3,12,true);a.compare(0,0);
            if(!a.at_least()) {a.value(0,0);a.store<2>(3,12,0);a.jump(0x101875f0);reset=true;}
        }
        if(!reset)decay_condition_scalar(a);
        cap_special_scalar(a);
    }
    a.at(0x10187630);a.pop(96);a.multiple(24,true);a.finish();
}
}
