#include "actor_attributes.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_attributes {
namespace {
using Call=SemanticGuestCall;

void report_missing_record(Call& a) {
    a.offset(3,31,0x53); a.offset(6,1,0x48); a.offset(7,1,0x40); a.value(4,0); a.value(5,0);
    a.call(0x10739890); a.call(0x10768810); a.move(5,3); a.move(3,30); a.offset(4,31,0x5e);
    a.offset(6,1,0x48); a.offset(7,1,0x40); a.value(8,0x53f); a.call(0x1077b720);
    a.move(4,30); a.offset(3,31,0x6b); a.value(5,0); a.call(0x10769b60);
    a.load(4,2,-0x7bac); a.value(0,0); a.value(3,0); a.store<1>(4,0,0); a.jump(0x101877bc);
}

void select_record_availability(Call& a) {
    a.at(0x1018772c); a.move(3,28); a.move(4,29); a.call(0x10187a40);
    a.load(0,29,0); a.load(3,28,0); a.multiply(0,0,0x24); a.add(3,3,0);
    a.load(0,3,0x10); a.compare(0,-1);
    if (a.equal()) { a.at(0x101877b8); a.value(3,0); return; }
    a.multiply(0,0,0x38); a.load(5,28,4); a.load(4,2,-0x7b80); a.offset(3,1,0x38);
    a.add(30,5,0); a.load(5,30,0x28); a.load(6,30,0x2c); a.call(0x10717dd0);
    a.load<2>(0,1,0x3a); a.load<2>(4,1,0x38,true); a.multiply(3,0,365); a.load<2>(0,30,0x32,true);
    a.add(3,4,3); a.compare_registers(3,0);
    if (!a.at_least()) { a.value(3,2); a.jump(0x101877bc); return; }
    a.at(0x10187798); a.load<2>(4,30,0x30,true); a.combine(0,4,3,false,true); a.shift_right(3,0,1);
    a.intersect(0,0,4); a.subtract(0,0,3); a.mask(0,0,1,false,1); a.sign8(3,0); a.jump(0x101877bc);
}

void condition_record_product(Call& a) {
    a.load(5,30,0); a.value(4,0x68dc0000); a.sign8(0,3); a.load(6,29,0);
    a.multiply(3,5,0x24); a.offset(5,4,-0x7453); a.compare(0,1); a.add(4,6,3);
    a.load<2>(3,4,8,true); a.load<2>(0,4,10,true); a.product(0,3,0); a.multiply_high(0,5,0);
    a.shift_right(0,0,12); a.mask(3,0,1,false,1); a.add(0,0,3); a.sign16(3,0);
    if (a.equal()) {
        a.compare(3,9000); a.value(0,9000); if (!a.at_least()) a.move(0,3);
        a.at(0x1018798c); a.sign16(3,0);
    }
    a.at(0x10187990); a.sign16(0,3); a.value(3,100); a.compare(0,100);
    if (!a.nonpositive()) a.move(3,0);
    a.at(0x101879a4); a.sign8(0,31,true); a.sign16(3,3);
}

void condition_as_percentage(Call& a) {
    a.exclusive_flags(3,3,0x80000000); a.value(0,0x43300000); a.load(4,2,-0x543c);
    a.store(1,0x44,3); a.floating_load(2,4,8); a.store(1,0x40,0); a.load(3,2,-0x5444);
    a.floating_load(1,1,0x40); a.floating_load(0,3,0,true); a.floating_subtract(1,1,2,true);
    a.floating_load(2,4,0x28); a.floating_difference(0,1,0,true,true); a.floating_sum(0,2,0);
    a.floating_integer(0,0); a.floating_store(0,1,0x38); a.load(3,1,0x3c); a.sign16(0,3); a.compare(0,100);
    if (!a.nonpositive()) a.value(3,100);
}
}

void record_condition_scale(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,record_scale_entry,dependencies,observer);
    a.load(5,2,-0x7be4); a.load(0,5,0); a.compare_registers(4,0);
    if (a.less()) { a.at(0x10187668); a.multiply(0,4,0x24); a.load(3,3,0); a.add(3,3,0); a.load<2>(0,3,0xc,true); }
    else { a.value(0,0); a.jump(0x10187678); }
    a.at(0x10187678); a.sign16(3,0); a.finish();
}

void record_availability(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,availability_entry,dependencies,observer);
    a.link(); a.save(31); a.load(31,2,-0x5438); a.save(30); a.load(30,2,-0x7ba8);
    a.save(29); a.move(29,4,true); a.save(28); a.move(28,3); a.save_link(); a.frame(0x80);
    if (a.equal()) report_missing_record(a);
    else {
        a.at(0x1018770c); bool valid=true;
        if (!a.equal()) {
            a.load(3,2,-0x7be4); a.load(4,29,0); a.load(0,3,0); a.compare_registers(4,0);
            valid=a.less(); if (!valid) { a.value(3,0); a.jump(0x101877bc); }
        }
        if (valid) select_record_availability(a);
    }
    a.at(0x101877bc); a.pop(0x80); a.restore(31); a.restore(30); a.restore(29); a.restore(28); a.finish();
}

void effective_condition(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,effective_condition_entry,dependencies,observer);
    a.link(); a.save(31); a.move(31,5); a.save(30); a.move(30,4,true); a.save(29); a.move(29,3);
    a.save_link(); a.frame(0x60); bool valid=true;
    if (a.equal()) { a.value(3,0); a.jump(0x10187a00); valid=false; }
    else {
        a.at(0x10187900);
        if (!a.equal()) {
            a.load(3,2,-0x7be4); a.load(4,30,0); a.load(0,3,0); a.compare_registers(4,0);
            if (!a.less()) {
                a.sign8(0,31,true); a.value(3,10000);
                if (!a.equal()) { a.value(3,100); a.jump(0x10187a00); }
                valid=false;
            }
        }
        if (valid) {
            a.at(0x1018792c); a.move(3,29); a.move(4,30); a.call(availability_entry);
            condition_record_product(a);
            if (!a.equal()) condition_as_percentage(a);
        }
    }
    a.at(0x10187a00); a.pop(0x60); a.restore(31); a.restore(30); a.restore(29); a.finish();
}
}
