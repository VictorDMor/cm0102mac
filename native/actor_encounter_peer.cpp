#include "actor_encounter.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_encounter {
using Call = SemanticGuestCall;
namespace {
void scan_peer_candidates(Call&);
void scan_both_teams(Call&);
void prepare_team_responses(Call&);
void update_existing_contact(Call&);
void update_existing_contact(Call& a) {
    for (;;) {
        a.at(0x10288104); a.load(3,31,0x1a4); a.sign8(4,6); a.load<1>(0,3,0x1bc); a.sign8(0,0);
        a.compare_registers(4,0);
        if (a.at_least()) break;
        a.at(0x102880c0); a.sign8(0,6); a.load(3,31,0x1a4); a.multiply(0,0,0xc); a.add(3,3,0); a.load(0,3,0xb4);
        a.compare_registers(0,31,false);
        if (a.equal()) {
            a.at(0x102880dc); a.sign8(0,6); a.load(4,31,0x1a4); a.multiply(3,0,0xc); a.load<1>(0,1,0x39);
            a.move(5,6); a.add(4,4,3); a.load<1>(3,4,0xbc); a.add(0,3,0); a.store<1>(4,0xbc,0);
        }
        a.at(0x10288100); a.offset(6,6,0x1);
    }
}
void prepare_team_responses(Call& a) {
    for (;;) {
        a.at(0x1028806c); a.sign8(0,20); a.compare(0,20);
        if (a.at_least()) break;
        a.at(0x10288008); a.load<1>(0,25,0x19); a.sign8(0,0,true);
        if (a.at_least()) {
            a.at(0x10288014); a.load<1>(0,25,0x19); a.sign8(0,0); a.compare(0,10);
            if (a.nonpositive()) {
                a.at(0x10288024); a.load<1>(0,25,0x2c); a.sign8(0,0); a.compare(0,2);
                if (a.less()) {
                    a.at(0x10288034); a.compare_registers(25,31,false);
                    if (!a.equal()) {
                        a.at(0x1028803c); a.move(3,25); a.value(4,0x1); a.value(5,0x0); a.call(0x10286650);
                        a.sign8(0,3,true);
                        if (!a.equal()) {
                            a.at(0x10288054); a.load<1>(4,31,0x105); a.move(3,25); a.load<1>(5,31,0x106);
                            a.call(0x102374f0);
                        }
                        a.at(0x10288064);
                    }
                    a.at(0x10288064);
                }
                a.at(0x10288064);
            }
            a.at(0x10288064);
        }
        a.at(0x10288064); a.offset(20,20,0x1); a.offset(25,25,0x1c4);
    }
}
void scan_both_teams(Call& a) {
    for (;;) {
        a.at(0x1028807c); a.sign8(0,19); a.compare(0,2);
        if (a.at_least()) break;
        a.at(0x10287fec); a.sign8(0,19); a.load(3,31,0x1a4); a.multiply(0,0,0x2350); a.value(20,0x0);
        a.offset(3,3,0x311c); a.add(25,3,0); a.jump(0x1028806c);
        prepare_team_responses(a);
        a.at(0x10288078); a.offset(19,19,0x1);
    }
}
void scan_peer_candidates(Call& a) {
    for (;;) {
        a.at(0x10287e68); a.load<1>(0,31,0x27); a.sign8(4,23); a.sign8(0,0); a.leading_zeroes(0,0);
        a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0); a.add(3,21,0); a.load<1>(0,3,0x58); a.sign8(0,0);
        a.compare_registers(4,0);
        if (a.at_least()) break;
        a.sign16(0,22); a.compare(0,-1);
        if (!a.equal()) break;
        a.at(0x10287730); a.load<1>(3,31,0x27); a.sign8(0,23); a.mask(0,0,0xfffffffc,false,2); a.sign8(3,3);
        a.leading_zeroes(3,3); a.mask(3,3,0x7ffffff,false,27); a.sign8(3,3); a.multiply(3,3,0x2c); a.add(0,3,0);
        a.indexed_load(25,21,0); a.compare(25,0,false);
        if (!a.equal()) {
            a.at(0x10287760); a.load(0,25,0x6c); a.compare(0,0,false);
            if (!a.equal()) {
                a.at(0x1028776c); a.load(0,25,0x70); a.compare(0,0,false);
                if (!a.equal()) {
                    a.at(0x10287778); a.move(3,31); a.move(5,25); a.value(4,0x4); a.call(0x10286650);
                    a.sign8(0,3,true);
                    if (a.equal()) {
                        a.at(0x10287888); a.move(3,31); a.move(5,25); a.value(4,0x3); a.call(0x10286650);
                        a.sign8(0,3,true);
                        if (a.equal()) {
                            a.at(0x10287ce4); a.move(3,31); a.move(5,25); a.value(4,0x2); a.call(0x10286650);
                            a.sign8(0,3,true);
                            if (!a.equal()) {
                                a.at(0x10287cfc); a.load(3,29,0x0); a.load(0,19,0x0); a.offset(3,3,0x2);
                                a.compare_registers(3,0,false); a.store(29,0x0,3);
                                if (a.positive()) {
                                    a.at(0x10287d14); a.store(29,0x0,20); a.call(0x1077b770);
                                    a.mask(0,3,0xffff,false,0); a.store(28,0x0,0);
                                }
                                a.at(0x10287d24); a.load(3,29,0x0); a.load(0,28,0x0); a.load<2>(3,3,0x0);
                                a.add(0,3,0); a.mask(3,0,0x1,false,1); a.mask(0,0,0x1,false,0);
                                a.combine(0,0,3,false,true); a.subtract(0,3,0,true);
                                if (a.equal()) {
                                    a.at(0x10287d4c); a.load<2>(0,31,0x19c,true); a.compare(0,95);
                                    if (!a.equal()) {
                                        a.at(0x10287d64); a.value(22,0x210b); a.value(27,0x5f); a.jump(0x10287d78);
                                    } else {
                                        a.at(0x10287d58); a.value(22,0x20e2); a.value(27,0x60); a.jump(0x10287d78);
                                    }
                                    a.at(0x10287d78);
                                } else {
                                    a.at(0x10287d48); a.jump(0x10287d70); a.value(22,0x20e2); a.value(27,0x60);
                                }
                                a.at(0x10287d78); a.move(30,25); a.load(3,31,0x1a4); a.load<1>(4,25,0x27);
                                a.floating_load(1,24,164,true); a.call(0x101d6860); a.load(4,30,0x6c);
                                a.value(3,0x43300000); a.value(0,0x43300000); a.store(1,0x3c,3);
                                a.load<1>(3,4,0x69); a.floating_load(3,26,128); a.sign8(3,3);
                                a.floating_load(2,26,184); a.exclusive_flags(3,3,0x80000000);
                                a.floating_load(4,26,896); a.store(1,0x40,3); a.floating_load(0,30,424,true);
                                a.floating_load(1,1,60); a.store(1,0x44,0); a.floating_subtract(3,1,3);
                                a.floating_load(1,26,128); a.floating_difference(2,3,2,true);
                                a.floating_subtract(2,4,2); a.floating_round_single(2,2);
                                a.floating_product_single(0,0,2); a.floating_store_single(0,30,424);
                                a.load(3,30,0x6c); a.floating_load(2,30,424,true); a.load<1>(0,3,0x69);
                                a.sign8(0,0); a.mask(0,0,0xfffffffe,false,1); a.subtract_from(0,0,0x32);
                                a.exclusive_flags(0,0,0x80000000); a.store(1,0x48,0); a.floating_load(0,1,68);
                                a.floating_subtract(0,0,1,true); a.floating_compare(2,0);
                                if (a.nonpositive()) {
                                    a.at(0x10287e18); a.load(3,30,0x6c); a.value(0,0x43300000); a.store(1,0x4c,0);
                                    a.load<1>(0,3,0x69); a.floating_load(1,26,128); a.sign8(0,0);
                                    a.mask(0,0,0xfffffffe,false,1); a.subtract_from(0,0,0x32);
                                    a.exclusive_flags(0,0,0x80000000); a.store(1,0x50,0); a.floating_load(0,1,76);
                                    a.floating_subtract(0,0,1,true);
                                } else {
                                    a.at(0x10287e10); a.floating_load(0,30,424,true); a.jump(0x10287e48);
                                }
                                a.at(0x10287e48); a.floating_store_single(0,30,424); a.floating_load(0,26,936);
                                a.floating_load(1,30,424,true); a.floating_compare(1,0);
                                if (a.positive()) {
                                    a.at(0x10287e5c); a.floating_load(0,24,28,true);
                                    a.floating_store_single(0,30,424);
                                }
                                a.at(0x10287e64);
                            }
                            a.at(0x10287e64);
                        } else {
                            a.at(0x102878a0); a.load(4,31,0x6c); a.value(0,0x8); a.sign8(3,0); a.value(0,0x2bc);
                            a.load(4,4,0xc); a.load<1>(4,4,0x119); a.sign8(5,4); a.subtract(4,3,5);
                            a.subtract(3,5,3); a.combine(3,4,3); a.shift_right(3,3,0x1f); a.intersect(22,0,3);
                            a.subtract_from(0,22,0x3e8); a.compare(0,0);
                            if (!a.equal()) {
                                a.at(0x102878e4); a.load(3,29,0x0); a.load(0,19,0x0); a.offset(3,3,0x2);
                                a.compare_registers(3,0,false); a.store(29,0x0,3);
                                if (a.positive()) {
                                    a.at(0x102878fc); a.store(29,0x0,20); a.call(0x1077b770);
                                    a.mask(0,3,0xffff,false,0); a.store(28,0x0,0);
                                }
                                a.at(0x1028790c); a.value(3,0x10000); a.subtract_from(0,22,0x3e8);
                                a.offset(3,3,-0x1); a.compare_registers(0,3);
                                if (a.positive()) {
                                    a.at(0x10287934); a.load(3,29,0x0); a.value(6,0x80010000);
                                    a.value(4,0x43300000); a.value(0,0x43300000); a.load<2>(8,3,0x0);
                                    a.value(5,0x10000); a.load(7,28,0x0); a.value(3,0x80010000);
                                    a.offset(6,6,-0x7fff); a.store(1,0x3c,4); a.add(7,8,7);
                                    a.floating_load(3,26,128); a.multiply_high(4,6,7); a.store(1,0x44,0);
                                    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_load(1,26,128);
                                    a.offset(3,3,-0x7fff); a.add(0,4,7); a.shift_right(4,0,0xf);
                                    a.subtract_from(0,22,0x3e8); a.exclusive_flags(0,0,0x80000000);
                                    a.mask(6,4,0x1,false,1); a.store(1,0x40,0); a.add(0,4,6); a.product(4,0,5);
                                    a.floating_load(2,1,60); a.subtract_from(0,22,0x3e8);
                                    a.floating_subtract(2,2,3); a.subtract(27,4,7);
                                    a.floating_difference(2,2,0,true); a.exclusive_flags(4,27,0x80000000);
                                    a.store(1,0x48,4); a.multiply(0,0,0xa); a.floating_load(0,1,68);
                                    a.multiply_high(3,3,0); a.floating_subtract(0,0,1); a.add(0,3,0);
                                    a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.floating_product(0,2,0);
                                    a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,76);
                                    a.load(27,1,0x50); a.call(0x10226520); a.value(4,0x66660000);
                                    a.offset(0,3,0x5); a.offset(4,4,0x6667); a.multiply_high(0,4,0);
                                    a.shift_right(0,0,0x2); a.mask(4,0,0x1,false,1); a.subtract_from(3,22,0x3e8);
                                    a.add(0,0,4); a.add(27,27,0); a.offset(0,3,-0x1); a.compare_registers(27,0);
                                    if (a.positive()) {
                                        a.at(0x10287a1c); a.subtract_from(3,22,0x3e8); a.offset(27,3,-0x1);
                                        a.jump(0x10287a60);
                                    }
                                    a.at(0x10287a60);
                                } else {
                                    a.at(0x10287920); a.value(3,0xffff0000); a.subtract_from(0,22,0x3e8);
                                    a.offset(3,3,0x1); a.compare_registers(0,3);
                                    if (a.at_least()) {
                                        a.at(0x10287a28); a.load(5,29,0x0); a.subtract_from(3,22,0x3e8);
                                        a.subtract_from(0,22,0x3e8); a.load(4,28,0x0); a.load<2>(5,5,0x0);
                                        a.compare(0,0); a.add(4,5,4); a.divide(0,4,3); a.product(0,0,3);
                                        a.subtract(27,0,4); a.negate(0,27);
                                        if (a.positive()) {
                                            a.at(0x10287a58); a.move(0,27);
                                        }
                                        a.at(0x10287a5c); a.move(27,0);
                                    } else {
                                        a.at(0x10287934); a.load(3,29,0x0); a.value(6,0x80010000);
                                        a.value(4,0x43300000); a.value(0,0x43300000); a.load<2>(8,3,0x0);
                                        a.value(5,0x10000); a.load(7,28,0x0); a.value(3,0x80010000);
                                        a.offset(6,6,-0x7fff); a.store(1,0x3c,4); a.add(7,8,7);
                                        a.floating_load(3,26,128); a.multiply_high(4,6,7); a.store(1,0x44,0);
                                        a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_load(1,26,128);
                                        a.offset(3,3,-0x7fff); a.add(0,4,7); a.shift_right(4,0,0xf);
                                        a.subtract_from(0,22,0x3e8); a.exclusive_flags(0,0,0x80000000);
                                        a.mask(6,4,0x1,false,1); a.store(1,0x40,0); a.add(0,4,6); a.product(4,0,5);
                                        a.floating_load(2,1,60); a.subtract_from(0,22,0x3e8);
                                        a.floating_subtract(2,2,3); a.subtract(27,4,7);
                                        a.floating_difference(2,2,0,true); a.exclusive_flags(4,27,0x80000000);
                                        a.store(1,0x48,4); a.multiply(0,0,0xa); a.floating_load(0,1,68);
                                        a.multiply_high(3,3,0); a.floating_subtract(0,0,1); a.add(0,3,0);
                                        a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.floating_product(0,2,0);
                                        a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,76);
                                        a.load(27,1,0x50); a.call(0x10226520); a.value(4,0x66660000);
                                        a.offset(0,3,0x5); a.offset(4,4,0x6667); a.multiply_high(0,4,0);
                                        a.shift_right(0,0,0x2); a.mask(4,0,0x1,false,1);
                                        a.subtract_from(3,22,0x3e8); a.add(0,0,4); a.add(27,27,0);
                                        a.offset(0,3,-0x1); a.compare_registers(27,0);
                                        if (a.positive()) {
                                            a.at(0x10287a1c); a.subtract_from(3,22,0x3e8); a.offset(27,3,-0x1);
                                            a.jump(0x10287a60);
                                        }
                                        a.at(0x10287a60);
                                    }
                                    a.at(0x10287a60);
                                }
                                a.at(0x10287a60);
                            } else {
                                a.at(0x102878dc); a.value(27,0x0); a.jump(0x10287a60);
                            }
                            a.at(0x10287a60); a.compare(27,10);
                            if (a.at_least()) {
                                a.at(0x10287b5c); a.load(3,29,0x0); a.load(0,19,0x0); a.offset(3,3,0x2);
                                a.compare_registers(3,0,false); a.store(29,0x0,3);
                                if (a.positive()) {
                                    a.at(0x10287b74); a.store(29,0x0,20); a.call(0x1077b770);
                                    a.mask(0,3,0xffff,false,0); a.store(28,0x0,0);
                                }
                                a.at(0x10287b84); a.load(4,29,0x0); a.value(3,0x66660000); a.load(0,28,0x0);
                                a.offset(3,3,0x6667); a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4);
                                a.shift_right(0,0,0x2); a.mask(3,0,0x1,false,1); a.add(0,0,3); a.multiply(0,0,0xa);
                                a.subtract(0,0,4); a.compare(0,4);
                                if (a.at_least()) {
                                    a.at(0x10287bc8); a.compare(0,9);
                                    if (a.at_least()) {
                                        a.at(0x10287bec); a.value(22,0x20e5); a.value(27,0x2f);
                                    } else {
                                        a.at(0x10287bd0); a.jump(0x10287be0); a.value(22,0x20e1); a.value(27,0x30);
                                        a.jump(0x10287bf4);
                                    }
                                    a.at(0x10287bf4);
                                } else {
                                    a.at(0x10287bbc); a.compare(0,0);
                                    if (a.at_least()) {
                                        a.at(0x10287bd4); a.value(22,0x20e4); a.value(27,0x32); a.jump(0x10287bf4);
                                    } else {
                                        a.at(0x10287bc4); a.jump(0x10287bec); a.value(22,0x20e5); a.value(27,0x2f);
                                    }
                                    a.at(0x10287bf4);
                                }
                                a.at(0x10287bf4); a.load(3,31,0x1a4); a.load<1>(4,25,0x27);
                                a.floating_load(1,24,44,true); a.call(0x101d6860); a.load(4,25,0x6c);
                                a.value(3,0x43300000); a.value(0,0x43300000); a.store(1,0x3c,3);
                                a.load<1>(3,4,0x69); a.floating_load(3,26,128); a.sign8(3,3);
                                a.floating_load(2,26,976); a.exclusive_flags(3,3,0x80000000);
                                a.floating_load(4,26,168); a.store(1,0x40,3); a.floating_load(0,25,424,true);
                                a.floating_load(1,1,60); a.store(1,0x44,0); a.floating_subtract(3,1,3);
                                a.floating_load(1,26,128); a.floating_difference(2,3,2,true);
                                a.floating_subtract(2,4,2); a.floating_round_single(2,2);
                                a.floating_product_single(0,0,2); a.floating_store_single(0,25,424);
                                a.load(3,25,0x6c); a.floating_load(2,25,424,true); a.load<1>(0,3,0x69);
                                a.sign8(0,0); a.mask(0,0,0xfffffffe,false,1); a.subtract_from(0,0,0x32);
                                a.exclusive_flags(0,0,0x80000000); a.store(1,0x48,0); a.floating_load(0,1,68);
                                a.floating_subtract(0,0,1,true); a.floating_compare(2,0);
                                if (a.nonpositive()) {
                                    a.at(0x10287c90); a.load(3,25,0x6c); a.value(0,0x43300000); a.store(1,0x4c,0);
                                    a.load<1>(0,3,0x69); a.floating_load(1,26,128); a.sign8(0,0);
                                    a.mask(0,0,0xfffffffe,false,1); a.subtract_from(0,0,0x32);
                                    a.exclusive_flags(0,0,0x80000000); a.store(1,0x50,0); a.floating_load(0,1,76);
                                    a.floating_subtract(0,0,1,true);
                                } else {
                                    a.at(0x10287c88); a.floating_load(0,25,424,true); a.jump(0x10287cc0);
                                }
                                a.at(0x10287cc0); a.floating_store_single(0,25,424); a.floating_load(0,26,936);
                                a.floating_load(1,25,424,true); a.floating_compare(1,0);
                                if (a.positive()) {
                                    a.at(0x10287cd4); a.floating_load(0,24,28,true);
                                    a.floating_store_single(0,25,424);
                                }
                                a.at(0x10287cdc);
                            } else {
                                a.at(0x10287a68); a.load(3,31,0x1a4); a.value(22,0x2108); a.load<1>(4,25,0x27);
                                a.value(27,0x61); a.floating_load(1,24,64,true); a.call(0x101d6860);
                                a.load(4,25,0x6c); a.value(3,0x43300000); a.value(0,0x43300000); a.store(1,0x3c,3);
                                a.load<1>(3,4,0x69); a.floating_load(3,26,128); a.sign8(3,3);
                                a.floating_load(2,26,976); a.exclusive_flags(3,3,0x80000000);
                                a.floating_load(4,26,168); a.store(1,0x40,3); a.floating_load(0,25,424,true);
                                a.floating_load(1,1,60); a.store(1,0x44,0); a.floating_subtract(3,1,3);
                                a.floating_load(1,26,128); a.floating_difference(2,3,2,true);
                                a.floating_subtract(2,4,2); a.floating_round_single(2,2);
                                a.floating_product_single(0,0,2); a.floating_store_single(0,25,424);
                                a.load(3,25,0x6c); a.floating_load(2,25,424,true); a.load<1>(0,3,0x69);
                                a.sign8(0,0); a.mask(0,0,0xfffffffe,false,1); a.subtract_from(0,0,0x32);
                                a.exclusive_flags(0,0,0x80000000); a.store(1,0x48,0); a.floating_load(0,1,68);
                                a.floating_subtract(0,0,1,true); a.floating_compare(2,0);
                                if (a.nonpositive()) {
                                    a.at(0x10287b0c); a.load(3,25,0x6c); a.value(0,0x43300000); a.store(1,0x4c,0);
                                    a.load<1>(0,3,0x69); a.floating_load(1,26,128); a.sign8(0,0);
                                    a.mask(0,0,0xfffffffe,false,1); a.subtract_from(0,0,0x32);
                                    a.exclusive_flags(0,0,0x80000000); a.store(1,0x50,0); a.floating_load(0,1,76);
                                    a.floating_subtract(0,0,1,true);
                                } else {
                                    a.at(0x10287b04); a.floating_load(0,25,424,true); a.jump(0x10287b3c);
                                }
                                a.at(0x10287b3c); a.floating_store_single(0,25,424); a.floating_load(0,26,936);
                                a.floating_load(1,25,424,true); a.floating_compare(1,0);
                                if (a.positive()) {
                                    a.at(0x10287b50); a.floating_load(0,24,28,true);
                                    a.floating_store_single(0,25,424); a.jump(0x10287cdc);
                                }
                                a.at(0x10287cdc);
                            }
                            a.at(0x10287cdc); a.move(30,25); a.jump(0x10287e64);
                        }
                        a.at(0x10287e64);
                    } else {
                        a.at(0x10287790); a.load(3,31,0x1a4); a.value(22,0x20e3); a.load<1>(4,25,0x27);
                        a.value(27,0x31); a.floating_load(1,24,272,true); a.call(0x101d6860); a.load(4,25,0x6c);
                        a.value(3,0x43300000); a.value(0,0x43300000); a.store(1,0x4c,3); a.load<1>(3,4,0x69);
                        a.floating_load(3,26,128); a.sign8(3,3); a.floating_load(2,26,976);
                        a.exclusive_flags(3,3,0x80000000); a.floating_load(4,26,168); a.store(1,0x50,3);
                        a.floating_load(0,25,424,true); a.floating_load(1,1,76); a.store(1,0x44,0);
                        a.floating_subtract(3,1,3); a.floating_load(1,26,128); a.floating_difference(2,3,2,true);
                        a.floating_subtract(2,4,2); a.floating_round_single(2,2); a.floating_product_single(0,0,2);
                        a.floating_store_single(0,25,424); a.load(3,25,0x6c); a.floating_load(2,25,424,true);
                        a.load<1>(0,3,0x69); a.sign8(0,0); a.mask(0,0,0xfffffffe,false,1);
                        a.subtract_from(0,0,0x32); a.exclusive_flags(0,0,0x80000000); a.store(1,0x48,0);
                        a.floating_load(0,1,68); a.floating_subtract(0,0,1,true); a.floating_compare(2,0);
                        if (a.nonpositive()) {
                            a.at(0x10287834); a.load(3,25,0x6c); a.value(0,0x43300000); a.store(1,0x3c,0);
                            a.load<1>(0,3,0x69); a.floating_load(1,26,128); a.sign8(0,0);
                            a.mask(0,0,0xfffffffe,false,1); a.subtract_from(0,0,0x32);
                            a.exclusive_flags(0,0,0x80000000); a.store(1,0x40,0); a.floating_load(0,1,60);
                            a.floating_subtract(0,0,1,true);
                        } else {
                            a.at(0x1028782c); a.floating_load(0,25,424,true); a.jump(0x10287864);
                        }
                        a.at(0x10287864); a.floating_store_single(0,25,424); a.floating_load(0,26,936);
                        a.floating_load(1,25,424,true); a.floating_compare(1,0);
                        if (a.positive()) {
                            a.at(0x10287878); a.floating_load(0,24,28,true); a.floating_store_single(0,25,424);
                        }
                        a.at(0x10287880); a.move(30,25); a.jump(0x10287e64);
                    }
                    a.at(0x10287e64);
                }
                a.at(0x10287e64);
            }
            a.at(0x10287e64);
        }
        a.at(0x10287e64); a.offset(23,23,0x1);
    }
}
}
void resolve_peer(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x102876c0,dependencies,observer);
    a.at(0x102876c0); a.link(); a.multiple(18,false,0); a.move(31,3); a.move(18,4); a.load(26,2,-0x51e8);
    a.value(30,0x0); a.load(28,2,-0x7b90); a.value(22,0xffffffff); a.load(20,2,-0x7b8c); a.value(23,0x0);
    a.load(19,2,-0x7b88); a.load(29,2,-0x7b84); a.load(24,2,-0x51e0); a.save_link(); a.value(0,0x0); a.frame(144);
    a.store<1>(1,0x38,0); a.load<1>(3,3,0x105); a.load<1>(0,31,0x106); a.sign8(3,3); a.load(4,31,0x1a4);
    a.sign8(0,0); a.multiply(3,3,0x450); a.multiply(0,0,0x5c); a.add(3,3,0); a.offset(21,3,0xa08); a.add(21,4,21);
    a.jump(0x10287e68);
    scan_peer_candidates(a);
    a.at(0x10287ea0); a.compare(30,0,false);
    if (a.equal()) {
        a.at(0x10288210); a.value(3,0x0);
    } else {
        a.at(0x10287ea8); a.sign8(0,18); a.sign8(3,27); a.compare(0,1); a.store<2>(31,0x19c,3);
        if (a.nonpositive()) {
            a.at(0x10287fb4); a.sign8(0,18,true);
            if (a.at_least()) {
                a.at(0x10287fbc); a.load(3,31,0x1a4); a.move(4,22); a.load(5,31,0x1a4); a.value(10,0x0);
                a.load(3,3,0x79f4); a.load<1>(6,31,0x2f); a.load<1>(7,31,0x27); a.load<1>(8,30,0x2f);
                a.load<1>(9,30,0x27); a.call(0x10201180);
            }
            a.at(0x10287fe4);
        } else {
            a.at(0x10287ebc); a.load(3,29,0x0); a.load(0,19,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(29,0x0,3);
            if (a.positive()) {
                a.at(0x10287ed4); a.store(29,0x0,20); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(28,0x0,0);
            }
            a.at(0x10287ee4); a.load(4,29,0x0); a.value(3,0x66660000); a.load(0,28,0x0); a.offset(3,3,0x6667);
            a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x1);
            a.mask(3,0,0x1,false,1); a.add(0,0,3); a.multiply(0,0,0x5); a.subtract(0,0,4,true);
            if (a.equal()) {
                a.at(0x10287fb4); a.sign8(0,18,true);
                if (a.at_least()) {
                    a.at(0x10287fbc); a.load(3,31,0x1a4); a.move(4,22); a.load(5,31,0x1a4); a.value(10,0x0);
                    a.load(3,3,0x79f4); a.load<1>(6,31,0x2f); a.load<1>(7,31,0x27); a.load<1>(8,30,0x2f);
                    a.load<1>(9,30,0x27); a.call(0x10201180);
                }
                a.at(0x10287fe4);
            } else {
                a.at(0x10287f18); a.load(3,29,0x0); a.load(0,19,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(29,0x0,3);
                if (a.positive()) {
                    a.at(0x10287f30); a.store(29,0x0,20); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(28,0x0,0);
                }
                a.at(0x10287f40); a.load(5,29,0x0); a.value(4,0x55550000); a.load(3,31,0x1a4);
                a.offset(4,4,0x5556); a.load<2>(6,5,0x0); a.load(5,28,0x0); a.load<1>(0,3,0x1bc); a.add(5,6,5);
                a.multiply_high(4,4,5); a.sign8(3,0); a.mask(0,4,0x1,false,1); a.add(0,4,0); a.multiply(0,0,0x3);
                a.subtract(4,0,5); a.offset(0,4,0x2); a.compare_registers(3,0);
                if (a.less()) {
                    a.at(0x10287fb4); a.sign8(0,18,true);
                    if (a.at_least()) {
                        a.at(0x10287fbc); a.load(3,31,0x1a4); a.move(4,22); a.load(5,31,0x1a4); a.value(10,0x0);
                        a.load(3,3,0x79f4); a.load<1>(6,31,0x2f); a.load<1>(7,31,0x27); a.load<1>(8,30,0x2f);
                        a.load<1>(9,30,0x27); a.call(0x10201180);
                    }
                    a.at(0x10287fe4);
                } else {
                    a.at(0x10287f84); a.load(3,31,0x1a4); a.value(4,0x210c); a.load<1>(7,31,0x27);
                    a.value(6,0xffffffff); a.load(3,3,0x79f4); a.value(8,0xffffffff); a.load(5,31,0x1a4);
                    a.value(9,0xffffffff); a.value(10,0x0); a.call(0x10201180); a.value(18,0xffffffce);
                    a.jump(0x10287fe4);
                }
                a.at(0x10287fe4);
            }
            a.at(0x10287fe4);
        }
        a.at(0x10287fe4); a.value(19,0x0); a.jump(0x1028807c);
        scan_both_teams(a);
        a.at(0x10288088); a.load(3,31,0x1a4); a.move(4,31); a.move(5,30); a.move(6,27); a.offset(8,1,0x3a);
        a.offset(9,1,0x39); a.offset(10,1,0x38); a.value(7,0xffffffff); a.call(0x101f5050); a.sign8(0,3,true);
        if (!a.equal()) {
            a.at(0x102880b4); a.value(5,0xffffffff); a.value(6,0x0); a.jump(0x10288104);
            update_existing_contact(a);
            a.at(0x1028811c); a.sign8(0,5); a.compare(0,-1);
            if (a.equal()) {
                a.at(0x10288128); a.load(3,31,0x1a4); a.load(4,31,0x1a4); a.load<1>(0,3,0x1bc); a.sign8(0,0);
                a.multiply(0,0,0xc); a.add(3,4,0); a.store(3,0xb4,31); a.load(3,31,0x1a4); a.load(4,31,0x1a4);
                a.load<1>(0,3,0x1bc); a.sign8(0,0); a.multiply(0,0,0xc); a.add(3,4,0); a.store(3,0xb8,30);
                a.load(3,31,0x1a4); a.load(4,31,0x1a4); a.load<1>(0,3,0x1bc); a.load<1>(5,1,0x3a); a.sign8(0,0);
                a.multiply(0,0,0xc); a.add(3,4,0); a.store<1>(3,0xbe,5); a.load(3,31,0x1a4); a.load(4,31,0x1a4);
                a.load<1>(0,3,0x1bc); a.sign8(0,0); a.multiply(0,0,0xc); a.add(3,4,0); a.store<1>(3,0xbd,27);
                a.load(5,31,0x1a4); a.load<1>(7,1,0x39); a.load<1>(4,5,0x1bc); a.load(6,31,0x1a4); a.sign8(0,4);
                a.offset(3,4,0x1); a.multiply(0,0,0xc); a.store<1>(5,0x1bc,3); a.add(3,6,0); a.store<1>(3,0xbc,7);
            }
            a.at(0x102881c4);
        }
        a.at(0x102881c4); a.sign8(0,18); a.compare(0,10);
        if (a.at_least()) {
            a.at(0x102881d8); a.sign8(0,18); a.compare(0,-40);
            if (a.less()) {
                a.at(0x102881e4); a.load(3,31,0x1a4); a.load<1>(0,3,0x1bc); a.sign8(0,0); a.compare(0,5);
                if (a.less()) {
                    a.at(0x102881f8); a.offset(0,18,0x1); a.move(3,30); a.sign8(4,0); a.call(0x102876c0);
                }
                a.at(0x10288208);
            }
            a.at(0x10288208);
        } else {
            a.at(0x102881d0); a.sign8(0,18,true);
            if (a.at_least()) {
                a.at(0x102881e4); a.load(3,31,0x1a4); a.load<1>(0,3,0x1bc); a.sign8(0,0); a.compare(0,5);
                if (a.less()) {
                    a.at(0x102881f8); a.offset(0,18,0x1); a.move(3,30); a.sign8(4,0); a.call(0x102876c0);
                }
                a.at(0x10288208);
            } else {
                a.at(0x102881d8); a.sign8(0,18); a.compare(0,-40);
                if (a.less()) {
                    a.at(0x102881e4); a.load(3,31,0x1a4); a.load<1>(0,3,0x1bc); a.sign8(0,0); a.compare(0,5);
                    if (a.less()) {
                        a.at(0x102881f8); a.offset(0,18,0x1); a.move(3,30); a.sign8(4,0); a.call(0x102876c0);
                    }
                    a.at(0x10288208);
                }
                a.at(0x10288208);
            }
            a.at(0x10288208);
        }
        a.at(0x10288208); a.value(3,0x1); a.jump(0x10288214);
    }
    a.at(0x10288214); a.pop(144); a.multiple(18,true,0); a.finish();
}
}
