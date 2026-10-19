#include "range_sampling.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::range_sampling {
namespace {
using Call = SemanticGuestCall;
void advance_match_pool(Call&);
void scaled_match_range(Call&);
void first_match_correction(Call&);
void first_match_scaled_correction(Call&);
void second_match_correction(Call&);
void second_match_scaled_correction(Call&);
void third_match_correction(Call&);
void third_match_scaled_correction(Call&);
void signed_match_range(Call&);

// Inlined correction levels keep their original captured slots and scratch.
// Recomputing a bound or reloading a cursor earlier would change alias behavior.
void signed_match_range(Call& a) {
    a.at(0x102270a4); a.load(3,28,0x0); a.compare(29,0); a.load(0,25,0x0); a.load<2>(3,3,0x0); a.add(3,3,0);
    a.divide(0,3,29); a.product(0,0,29); a.subtract(30,0,3); a.negate(3,30);
    if (a.positive()) {
        a.at(0x102270cc); a.move(3,30);
    }
    a.at(0x102270d0);
}
void third_match_scaled_correction(Call& a) {
    a.at(0x10226adc); a.load(3,28,0x0); a.value(7,0x80010000); a.value(5,0x80010000); a.value(11,0x80010000);
    a.load<2>(6,3,0x0); a.value(4,0x80010000); a.load(0,25,0x0); a.multiply(3,29,0xa); a.offset(7,7,-0x7fff);
    a.floating_load(3,24,0x80); a.add(6,6,0); a.floating_load(0,24,0x0); a.multiply_high(8,7,6);
    a.value(0,0x43300000); a.floating_load(1,24,0x80); a.offset(5,5,-0x7fff); a.store(1,0x38,0);
    a.value(7,0x80010000); a.multiply_high(0,5,3); a.add(5,8,6); a.value(8,0x80010000); a.shift_right(12,5,0xf);
    a.value(5,0x10000); a.value(9,0x80010000); a.add(0,0,3); a.offset(11,11,-0x7fff); a.shift_right(3,0,0xf);
    a.offset(4,4,-0x7fff); a.mask(10,3,0x1,false,1); a.value(0,0x43300000); a.add(10,3,10); a.value(3,0x80010000);
    a.multiply(10,10,0xa); a.offset(7,7,-0x7fff); a.offset(8,8,-0x7fff); a.store(1,0x40,0);
    a.mask(24,12,0x1,false,1); a.multiply_high(11,11,10); a.add(0,12,24); a.offset(5,5,-0x1);
    a.offset(9,9,-0x7fff); a.offset(3,3,-0x7fff); a.add(10,11,10); a.shift_right(10,10,0xf);
    a.mask(11,10,0x1,false,1); a.add(10,10,11); a.multiply(10,10,0xa); a.multiply_high(4,4,10); a.add(4,4,10);
    a.shift_right(4,4,0xf); a.mask(10,4,0x1,false,1); a.add(4,4,10); a.exclusive_flags(4,4,0x80000000);
    a.store(1,0x3c,4); a.multiply(4,29,0xa); a.floating_load(2,1,0x38); a.multiply_high(7,7,4);
    a.floating_difference(2,2,3); a.floating_difference(2,2,0,true); a.add(4,7,4); a.shift_right(4,4,0xf);
    a.mask(7,4,0x1,false,1); a.add(4,4,7); a.multiply(4,4,0xa); a.multiply_high(7,8,4); a.add(4,7,4);
    a.shift_right(4,4,0xf); a.product(5,0,5); a.mask(7,4,0x1,false,1); a.add(0,4,7); a.multiply(0,0,0xa);
    a.subtract(24,5,6); a.multiply_high(4,9,0); a.exclusive_flags(5,24,0x80000000); a.store(1,0x44,5);
    a.floating_load(0,1,0x40); a.add(0,4,0); a.floating_difference(0,0,1); a.shift_right(0,0,0xf);
    a.mask(4,0,0x1,false,1); a.add(0,0,4); a.floating_product(0,2,0); a.multiply(0,0,0xa); a.multiply_high(3,3,0);
    a.floating_integer(0,0); a.floating_store(0,1,0x48); a.add(0,3,0); a.load(24,1,0x4c); a.shift_right(0,0,0xf);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.call(0x10226520); a.multiply(0,29,0xa); a.value(6,0x66660000);
    a.value(4,0x80010000); a.offset(3,3,0x5); a.offset(6,6,0x6667); a.offset(4,4,-0x7fff); a.multiply_high(7,6,3);
    a.value(5,0x80010000); a.value(6,0x80010000); a.offset(5,5,-0x7fff); a.offset(6,6,-0x7fff);
    a.multiply_high(3,4,0); a.shift_right(4,7,0x2); a.mask(7,4,0x1,false,1); a.add(0,3,0); a.add(4,4,7);
    a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.add(24,24,4); a.add(0,0,3); a.multiply(0,0,0xa);
    a.multiply_high(3,5,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.add(0,0,3);
    a.multiply(0,0,0xa); a.multiply_high(3,6,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
    a.add(3,0,3); a.offset(0,3,-0x1); a.compare_registers(24,0);
    if (a.positive()) {
        a.at(0x10226cd8); a.multiply(0,29,0xa); a.value(3,0x80010000); a.value(4,0x80010000);
        a.offset(3,3,-0x7fff); a.multiply_high(3,3,0); a.value(5,0x80010000); a.offset(4,4,-0x7fff);
        a.offset(5,5,-0x7fff); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.add(0,0,3);
        a.multiply(0,0,0xa); a.multiply_high(3,4,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
        a.add(0,0,3); a.multiply(0,0,0xa); a.multiply_high(3,5,0); a.add(0,3,0); a.shift_right(0,0,0xf);
        a.mask(3,0,0x1,false,1); a.add(3,0,3); a.offset(24,3,-0x1); a.jump(0x10226e2c);
    }
    a.at(0x10226e2c);
}
void third_match_correction(Call& a) {
    a.at(0x102269d4); a.load(3,28,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(28,0x0,3);
    if (a.positive()) {
        a.at(0x102269ec); a.store(28,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(25,0x0,0);
    }
    a.at(0x102269fc); a.multiply(0,29,0xa); a.value(3,0x80010000); a.value(5,0x80010000); a.offset(3,3,-0x7fff);
    a.multiply_high(4,3,0); a.value(6,0x80010000); a.value(3,0x10000); a.offset(5,5,-0x7fff);
    a.offset(6,6,-0x7fff); a.offset(3,3,-0x1); a.add(0,4,0); a.shift_right(0,0,0xf); a.mask(4,0,0x1,false,1);
    a.add(0,0,4); a.multiply(0,0,0xa); a.multiply_high(4,5,0); a.add(0,4,0); a.shift_right(0,0,0xf);
    a.mask(4,0,0x1,false,1); a.add(0,0,4); a.multiply(0,0,0xa); a.multiply_high(4,6,0); a.add(0,4,0);
    a.shift_right(0,0,0xf); a.mask(4,0,0x1,false,1); a.add(0,0,4); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x10226adc);
        third_match_scaled_correction(a);
    } else {
        a.at(0x10226a6c); a.multiply(0,29,0xa); a.value(3,0x80010000); a.value(5,0x80010000);
        a.offset(3,3,-0x7fff); a.multiply_high(4,3,0); a.value(6,0x80010000); a.value(3,0xffff0000);
        a.offset(5,5,-0x7fff); a.offset(6,6,-0x7fff); a.offset(3,3,0x1); a.add(0,4,0); a.shift_right(0,0,0xf);
        a.mask(4,0,0x1,false,1); a.add(0,0,4); a.multiply(0,0,0xa); a.multiply_high(4,5,0); a.add(0,4,0);
        a.shift_right(0,0,0xf); a.mask(4,0,0x1,false,1); a.add(0,0,4); a.multiply(0,0,0xa); a.multiply_high(4,6,0);
        a.add(0,4,0); a.shift_right(0,0,0xf); a.mask(4,0,0x1,false,1); a.add(0,0,4); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x10226d40); a.multiply(0,29,0xa); a.value(3,0x80010000); a.value(7,0x80010000); a.load(5,28,0x0);
            a.offset(3,3,-0x7fff); a.load(9,25,0x0); a.multiply_high(3,3,0); a.value(8,0x80010000);
            a.value(4,0x80010000); a.load<2>(10,5,0x0); a.value(5,0x80010000); a.value(6,0x80010000); a.add(0,3,0);
            a.offset(7,7,-0x7fff); a.shift_right(0,0,0xf); a.offset(8,8,-0x7fff); a.mask(3,0,0x1,false,1);
            a.offset(4,4,-0x7fff); a.add(3,0,3); a.add(0,10,9); a.multiply(3,3,0xa); a.offset(5,5,-0x7fff);
            a.offset(6,6,-0x7fff); a.multiply_high(7,7,3); a.add(3,7,3); a.shift_right(3,3,0xf);
            a.mask(7,3,0x1,false,1); a.add(3,3,7); a.multiply(7,3,0xa); a.multiply(3,29,0xa);
            a.multiply_high(8,8,7); a.multiply_high(4,4,3); a.add(7,8,7); a.shift_right(7,7,0xf); a.add(3,4,3);
            a.mask(8,7,0x1,false,1); a.shift_right(3,3,0xf); a.mask(4,3,0x1,false,1); a.add(8,7,8); a.add(3,3,4);
            a.multiply(3,3,0xa); a.multiply_high(4,5,3); a.divide(7,0,8); a.add(3,4,3); a.shift_right(3,3,0xf);
            a.mask(4,3,0x1,false,1); a.add(3,3,4); a.multiply(3,3,0xa); a.multiply_high(4,6,3); a.product(5,7,8);
            a.add(3,4,3); a.shift_right(3,3,0xf); a.mask(4,3,0x1,false,1); a.subtract(24,5,0); a.add_record(0,3,4);
            a.negate(0,24);
            if (a.positive()) {
                a.at(0x10226e24); a.move(0,24);
            }
            a.at(0x10226e28); a.move(24,0);
        } else {
            a.at(0x10226adc);
            third_match_scaled_correction(a);
        }
        a.at(0x10226e2c);
    }
    a.at(0x10226e2c);
}
void second_match_scaled_correction(Call& a) {
    a.at(0x1022689c); a.load(4,28,0x0); a.value(3,0x80010000); a.value(5,0x80010000); a.value(0,0x43300000);
    a.load<2>(6,4,0x0); a.value(10,0x80010000); a.load(4,25,0x0); a.multiply(8,29,0xa); a.offset(3,3,-0x7fff);
    a.store(1,0x38,0); a.add(4,6,4); a.floating_load(3,24,0x80); a.multiply_high(7,3,4); a.offset(5,5,-0x7fff);
    a.floating_load(0,24,0x0); a.value(6,0x80010000); a.floating_load(1,24,0x80); a.value(3,0x10000);
    a.multiply_high(0,5,8); a.add(5,7,4); a.value(7,0x80010000); a.shift_right(11,5,0xf); a.value(5,0x43300000);
    a.offset(10,10,-0x7fff); a.add(0,0,8); a.store(1,0x40,5); a.shift_right(0,0,0xf); a.value(8,0x80010000);
    a.mask(9,0,0x1,false,1); a.offset(6,6,-0x7fff); a.add(9,0,9); a.mask(12,11,0x1,false,1); a.multiply(9,9,0xa);
    a.offset(3,3,-0x1); a.add(0,11,12); a.offset(7,7,-0x7fff); a.multiply_high(5,10,9); a.offset(8,8,-0x7fff);
    a.add(5,5,9); a.shift_right(5,5,0xf); a.mask(9,5,0x1,false,1); a.add(5,5,9); a.exclusive_flags(5,5,0x80000000);
    a.store(1,0x3c,5); a.multiply(5,29,0xa); a.floating_load(2,1,0x38); a.multiply_high(6,6,5);
    a.floating_difference(2,2,3); a.floating_difference(2,2,0,true); a.add(5,6,5); a.shift_right(5,5,0xf);
    a.mask(6,5,0x1,false,1); a.add(5,5,6); a.product(3,0,3); a.multiply(5,5,0xa); a.subtract(23,3,4);
    a.exclusive_flags(3,23,0x80000000); a.multiply_high(0,7,5); a.store(1,0x44,3); a.floating_load(0,1,0x40);
    a.floating_difference(0,0,1); a.add(0,0,5); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
    a.floating_product(0,2,0); a.add(0,0,3); a.multiply(0,0,0xa); a.floating_integer(0,0); a.multiply_high(3,8,0);
    a.floating_store(0,1,0x48); a.load(23,1,0x4c); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
    a.add_record(0,0,3);
    if (!a.equal()) {
        a.at(0x102269d4);
        third_match_correction(a);
    } else {
        a.at(0x102269cc); a.value(24,0x0); a.jump(0x10226e2c);
    }
    a.at(0x10226e2c); a.multiply(0,29,0xa); a.value(5,0x66660000); a.value(3,0x80010000); a.offset(4,24,0x5);
    a.offset(5,5,0x6667); a.offset(3,3,-0x7fff); a.multiply_high(5,5,4); a.value(4,0x80010000);
    a.offset(4,4,-0x7fff); a.multiply_high(3,3,0); a.shift_right(5,5,0x2); a.mask(6,5,0x1,false,1); a.add(0,3,0);
    a.add(5,5,6); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.add(23,23,5); a.add(0,0,3);
    a.multiply(0,0,0xa); a.multiply_high(3,4,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1);
    a.add(3,0,3); a.offset(0,3,-0x1); a.compare_registers(23,0);
    if (a.positive()) {
        a.at(0x10226e98); a.multiply(0,29,0xa); a.value(3,0x80010000); a.value(4,0x80010000);
        a.offset(3,3,-0x7fff); a.multiply_high(3,3,0); a.offset(4,4,-0x7fff); a.add(0,3,0); a.shift_right(0,0,0xf);
        a.mask(3,0,0x1,false,1); a.add(0,0,3); a.multiply(0,0,0xa); a.multiply_high(3,4,0); a.add(0,3,0);
        a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.add(3,0,3); a.offset(23,3,-0x1); a.jump(0x10226f8c);
    }
    a.at(0x10226f8c);
}
void second_match_correction(Call& a) {
    a.at(0x102267d4); a.load(3,28,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(28,0x0,3);
    if (a.positive()) {
        a.at(0x102267ec); a.store(28,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(25,0x0,0);
    }
    a.at(0x102267fc); a.multiply(0,29,0xa); a.value(3,0x80010000); a.value(5,0x80010000); a.offset(3,3,-0x7fff);
    a.multiply_high(4,3,0); a.offset(5,5,-0x7fff); a.value(3,0x10000); a.offset(3,3,-0x1); a.add(0,4,0);
    a.shift_right(0,0,0xf); a.mask(4,0,0x1,false,1); a.add(0,0,4); a.multiply(0,0,0xa); a.multiply_high(4,5,0);
    a.add(0,4,0); a.shift_right(0,0,0xf); a.mask(4,0,0x1,false,1); a.add(0,0,4); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x1022689c);
        second_match_scaled_correction(a);
    } else {
        a.at(0x1022684c); a.multiply(0,29,0xa); a.value(3,0x80010000); a.value(5,0x80010000);
        a.offset(3,3,-0x7fff); a.multiply_high(4,3,0); a.offset(5,5,-0x7fff); a.value(3,0xffff0000);
        a.offset(3,3,0x1); a.add(0,4,0); a.shift_right(0,0,0xf); a.mask(4,0,0x1,false,1); a.add(0,0,4);
        a.multiply(0,0,0xa); a.multiply_high(4,5,0); a.add(0,4,0); a.shift_right(0,0,0xf); a.mask(4,0,0x1,false,1);
        a.add(0,0,4); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x10226ee0); a.multiply(3,29,0xa); a.value(4,0x80010000); a.value(7,0x80010000); a.load(5,28,0x0);
            a.offset(4,4,-0x7fff); a.load(0,25,0x0); a.multiply_high(6,4,3); a.load<2>(8,5,0x0);
            a.value(4,0x80010000); a.value(5,0x80010000); a.offset(7,7,-0x7fff); a.add(0,8,0); a.add(3,6,3);
            a.offset(4,4,-0x7fff); a.shift_right(3,3,0xf); a.offset(5,5,-0x7fff); a.mask(6,3,0x1,false,1);
            a.add(3,3,6); a.multiply(3,3,0xa); a.multiply_high(6,7,3); a.add(3,6,3); a.shift_right(3,3,0xf);
            a.mask(6,3,0x1,false,1); a.add(7,3,6); a.multiply(3,29,0xa); a.multiply_high(4,4,3); a.add(3,4,3);
            a.shift_right(3,3,0xf); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.multiply(3,3,0xa);
            a.multiply_high(4,5,3); a.divide(6,0,7); a.add(3,4,3); a.shift_right(3,3,0xf); a.mask(4,3,0x1,false,1);
            a.product(5,6,7); a.subtract(23,5,0); a.add_record(0,3,4); a.negate(0,23);
            if (a.positive()) {
                a.at(0x10226f84); a.move(0,23);
            }
            a.at(0x10226f88); a.move(23,0);
        } else {
            a.at(0x1022689c);
            second_match_scaled_correction(a);
        }
        a.at(0x10226f8c);
    }
    a.at(0x10226f8c);
}
void first_match_scaled_correction(Call& a) {
    a.at(0x102266dc); a.load(5,28,0x0); a.value(3,0x80010000); a.value(4,0x80010000); a.value(0,0x43300000);
    a.load<2>(8,5,0x0); a.value(7,0x10000); a.load(6,25,0x0); a.multiply(5,29,0xa); a.offset(3,3,-0x7fff);
    a.store(1,0x38,0); a.add(9,8,6); a.floating_load(3,24,0x80); a.multiply_high(8,3,9); a.offset(4,4,-0x7fff);
    a.floating_load(0,24,0x0); a.value(0,0x43300000); a.floating_load(1,24,0x80); a.value(3,0x80010000);
    a.multiply_high(6,4,5); a.add(8,8,9); a.store(1,0x40,0); a.value(4,0x80010000); a.shift_right(8,8,0xf);
    a.offset(7,7,-0x1); a.add(0,6,5); a.mask(6,8,0x1,false,1); a.shift_right(0,0,0xf); a.offset(3,3,-0x7fff);
    a.mask(5,0,0x1,false,1); a.add(6,8,6); a.add(0,0,5); a.offset(4,4,-0x7fff); a.exclusive_flags(0,0,0x80000000);
    a.store(1,0x3c,0); a.multiply(0,29,0xa); a.floating_load(2,1,0x38); a.product(5,6,7);
    a.floating_difference(2,2,3); a.floating_difference(2,2,0,true); a.multiply_high(3,3,0); a.subtract(31,5,9);
    a.exclusive_flags(5,31,0x80000000); a.store(1,0x44,5); a.add(0,3,0); a.shift_right(0,0,0xf);
    a.floating_load(0,1,0x40); a.mask(3,0,0x1,false,1); a.add(0,0,3); a.floating_difference(0,0,1);
    a.multiply(0,0,0xa); a.floating_product(0,2,0); a.multiply_high(3,4,0); a.floating_integer(0,0); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.floating_store(0,1,0x48); a.mask(3,0,0x1,false,1); a.add_record(0,0,3);
    a.load(31,1,0x4c);
    if (!a.equal()) {
        a.at(0x102267d4);
        second_match_correction(a);
    } else {
        a.at(0x102267cc); a.value(23,0x0); a.jump(0x10226f8c);
    }
    a.at(0x10226f8c); a.multiply(0,29,0xa); a.value(5,0x66660000); a.value(3,0x80010000); a.offset(4,23,0x5);
    a.offset(5,5,0x6667); a.offset(3,3,-0x7fff); a.multiply_high(4,5,4); a.multiply_high(3,3,0);
    a.shift_right(4,4,0x2); a.mask(5,4,0x1,false,1); a.add(0,3,0); a.add(4,4,5); a.shift_right(0,0,0xf);
    a.mask(3,0,0x1,false,1); a.add(31,31,4); a.add(3,0,3); a.offset(0,3,-0x1); a.compare_registers(31,0);
    if (a.positive()) {
        a.at(0x10226fd8); a.multiply(0,29,0xa); a.value(3,0x80010000); a.offset(3,3,-0x7fff);
        a.multiply_high(3,3,0); a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.add(3,0,3);
        a.offset(31,3,-0x1); a.jump(0x1022706c);
    }
    a.at(0x1022706c);
}
void first_match_correction(Call& a) {
    a.at(0x10226654); a.load(3,28,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(28,0x0,3);
    if (a.positive()) {
        a.at(0x1022666c); a.store(28,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(25,0x0,0);
    }
    a.at(0x1022667c); a.multiply(0,29,0xa); a.value(4,0x80010000); a.value(3,0x10000); a.offset(4,4,-0x7fff);
    a.multiply_high(4,4,0); a.offset(3,3,-0x1); a.add(0,4,0); a.shift_right(0,0,0xf); a.mask(4,0,0x1,false,1);
    a.add(0,0,4); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x102266dc);
        first_match_scaled_correction(a);
    } else {
        a.at(0x102266ac); a.multiply(0,29,0xa); a.value(4,0x80010000); a.value(3,0xffff0000);
        a.offset(4,4,-0x7fff); a.multiply_high(4,4,0); a.offset(3,3,0x1); a.add(0,4,0); a.shift_right(0,0,0xf);
        a.mask(4,0,0x1,false,1); a.add(0,0,4); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x10227000); a.multiply(0,29,0xa); a.value(4,0x80010000); a.load(6,28,0x0); a.value(3,0x80010000);
            a.offset(4,4,-0x7fff); a.load(5,25,0x0); a.multiply_high(4,4,0); a.load<2>(6,6,0x0);
            a.offset(3,3,-0x7fff); a.add(6,6,5); a.add(0,4,0); a.shift_right(0,0,0xf); a.mask(4,0,0x1,false,1);
            a.add(5,0,4); a.divide(4,6,5); a.multiply(0,29,0xa); a.multiply_high(3,3,0); a.product(4,4,5);
            a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.add_record(0,0,3); a.subtract(31,4,6);
            a.negate(0,31);
            if (a.positive()) {
                a.at(0x10227064); a.move(0,31);
            }
            a.at(0x10227068); a.move(31,0);
        } else {
            a.at(0x102266dc);
            first_match_scaled_correction(a);
        }
        a.at(0x1022706c);
    }
    a.at(0x1022706c);
}
void scaled_match_range(Call& a) {
    a.at(0x1022659c); a.exclusive_flags(3,29,0x80000000); a.value(0,0x43300000); a.store(1,0x4c,3);
    a.value(6,0x80010000); a.load(7,28,0x0); a.value(4,0x43300000); a.store(1,0x48,0); a.value(5,0x10000);
    a.floating_load(2,24,0x80); a.value(3,0x80010000); a.floating_load(1,1,0x48); a.offset(6,6,-0x7fff);
    a.floating_load(0,24,0x0); a.offset(5,5,-0x1); a.floating_difference(1,1,2); a.load<2>(8,7,0x0);
    a.load(7,25,0x0); a.multiply(0,29,0xa); a.offset(3,3,-0x7fff); a.store(1,0x40,4);
    a.floating_difference(2,1,0,true); a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,24,0x80);
    a.add(4,4,7); a.shift_right(4,4,0xf); a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5);
    a.subtract(30,4,7); a.exclusive_flags(4,30,0x80000000); a.store(1,0x44,4); a.multiply_high(3,3,0);
    a.floating_load(0,1,0x40); a.floating_difference(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf);
    a.floating_product(0,2,0); a.mask(3,0,0x1,false,1); a.add_record(0,0,3); a.floating_integer(0,0);
    a.floating_store(0,1,0x38); a.load(30,1,0x3c);
    if (!a.equal()) {
        a.at(0x10226654);
        first_match_correction(a);
    } else {
        a.at(0x1022664c); a.value(31,0x0); a.jump(0x1022706c);
    }
    a.at(0x1022706c); a.value(4,0x66660000); a.offset(3,31,0x5); a.offset(4,4,0x6667); a.offset(0,29,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(30,30,3);
    a.compare_registers(30,0);
    if (a.positive()) {
        a.at(0x10227098); a.offset(30,29,-0x1);
    }
    a.at(0x1022709c); a.move(3,30); a.jump(0x102270d0);
}
void advance_match_pool(Call& a) {
    a.at(0x10226554); a.load(3,28,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(28,0x0,3);
    if (a.positive()) {
        a.at(0x1022656c); a.store(28,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(25,0x0,0);
    }
    a.at(0x1022657c);
}
}

void sample_match(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,0x10226520,dependencies,observer);
    a.at(0x10226520); a.link(); a.multiple(23); a.move(29,3,true); a.load(24,2,-0x51e8); a.load(25,2,-0x7b90);
    a.load(26,2,-0x7b8c); a.load(27,2,-0x7b88); a.load(28,2,-0x7b84); a.save_link(); a.frame(128);
    if (!a.equal()) {
        a.at(0x10226554);
        advance_match_pool(a);
        a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(29,3);
        if (a.positive()) {
            a.at(0x1022659c);
            scaled_match_range(a);
        } else {
            a.at(0x1022658c); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(29,3);
            if (a.at_least()) {
                a.at(0x102270a4);
                signed_match_range(a);
            } else {
                a.at(0x1022659c);
                scaled_match_range(a);
            }
            a.at(0x102270d0);
        }
        a.at(0x102270d0);
    } else {
        a.at(0x1022654c); a.value(3,0x0); a.jump(0x102270d0);
    }
    a.at(0x102270d0); a.pop(128); a.multiple(23,true); a.finish();
}
}
