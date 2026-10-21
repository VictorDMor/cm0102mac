#include "shortlist_assessment_detail.hpp"

namespace cm::aot::shortlist_assessment::detail {
void prepare_interest_context(Call& a) {
    a.at(0x10369080);a.link();a.move(11,1);a.call(0x107737fc);
    a.multiple(13,false,-0x40);a.store<4>(1,0x8,0);a.frame(0x1d0);a.store<4>(1,0x1e8,3);
    a.move(17,4);a.load<4>(25,2,-0x7590,false);a.move(18,5);a.load<4>(15,2,-0x7a08,false);
    a.move(19,7);a.store<1>(1,0x1f7,6);a.move(20,8);a.load<4>(13,2,-0x7b9c,false);
    a.move(21,9);a.load<4>(23,2,-0x7b78,false);a.offset(3,1,0x78);a.load<4>(16,2,-0x4ddc,false);
    a.value(30,0x0);a.value(14,0x0);a.value(28,0x0);a.call(0x10715880);
    a.offset(3,1,0x70);a.call(0x10715880);a.compare(21,0x0,false);a.floating_load(27,16,0x80,false);
    if(!a.equal()) {
        a.at(0x103690f0);a.value(0,0x0);a.store<4>(21,0x0,0);
    }
    a.at(0x103690f8);a.load<4>(0,17,0x14,false);a.compare(0,0x0,false);
    if(a.equal()) {
        a.at(0x1036911c);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
        return;
    }
    a.at(0x10369104);a.load<4>(0,18,0x2c,false);a.compare(0,0x0,false);
    if(a.equal()) {
        a.at(0x1036911c);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
        return;
    }
    a.at(0x10369110);a.load<4>(0,17,0xc,false);a.compare(0,0x0,false);
    if(a.equal()) {
        a.at(0x1036911c);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
        return;
    }
    a.at(0x10369124);a.load<4>(3,2,-0x7b40,false);a.move(4,17);a.value(5,0x0);
    a.load<4>(3,3,0x0,false);a.call(0x103f43b0);a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x10369140);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
        return;
    }
    a.at(0x10369148);a.load<4>(0,17,0x0,false);a.load<4>(4,13,0x0,false);a.multiply(3,0,0x64);
    a.offset(0,3,0xc);a.indexed_load<1>(0,4,0);a.mask(0,0,0x3,true,30);
    if(!a.equal()) {
        a.at(0x10369164);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
        return;
    }
    a.at(0x1036916c);a.load<1>(5,17,0x63,false);a.value(3,0x43300000);a.load<1>(0,17,0x67,false);
    a.move(4,17);a.sign8(5,5,false);a.load<2>(6,17,0x5c,true);a.exclusive_flags(5,5,0x80000000);
    a.sign8(0,0,false);a.store<4>(1,0x128,5);a.exclusive_flags(0,0,0x80000000);a.floating_load(9,16,0x8,false);
    a.sign8(22,6,false);a.store<4>(1,0x124,3);a.floating_load(8,16,0x98,false);a.floating_load(0,1,0x124,false);
    a.store<4>(1,0x118,5);a.floating_subtract(1,0,9,false);a.floating_load(3,16,0xb8,false);a.store<4>(1,0x114,3);
    a.floating_load(0,16,0x238,false);a.floating_difference(7,1,8,true,false);a.floating_load(1,1,0x114,false);a.store<4>(1,0x120,0);
    a.floating_load(10,16,0x2d0,false);a.store<4>(1,0x11c,3);a.floating_load(5,16,0xf8,false);a.store<4>(1,0x10c,3);
    a.floating_subtract(4,1,9,false);a.floating_load(2,1,0x11c,false);a.floating_subtract(7,10,7,false);a.store<4>(1,0x110,5);
    a.floating_subtract(6,2,9,false);a.floating_load(2,16,0x68,false);a.floating_load(1,1,0x10c,false);a.floating_difference(3,4,3,true,false);
    a.load<4>(3,2,-0x7b7c,false);a.load<4>(3,3,0x0,false);a.floating_subtract(1,1,9,false);a.floating_difference(6,6,8,true,false);
    a.floating_difference(0,1,0,true,false);a.floating_subtract(28,7,6,false);a.floating_sum(29,5,3);a.floating_sum(30,2,0);
    a.floating_round_single(28,28);a.floating_round_single(29,29);a.floating_round_single(30,30);a.call(0x10082f20);
    a.compare(3,0x0,false);a.store<4>(23,0x0,3);
    if(a.equal()) {
        a.at(0x1036924c);a.value(3,0x0);
    } else {
        a.at(0x10369234);a.load<4>(0,3,0x4,false);a.load<4>(3,2,-0x7be8,false);a.multiply(0,0,0x248);
        a.load<4>(3,3,0x0,false);a.add(3,3,0);a.jump(0x10369250);
    }
    a.at(0x10369250);a.compare(3,0x0,false);a.move(24,3);
    if(!a.equal()) {
        a.at(0x1036925c);a.load<4>(0,3,0x2c,false);a.compare(0,0x0,false);
        if(a.equal()) {
            a.at(0x10369268);a.floating_load(1,16,0x160,false);a.jump(0x1036d21c);
            return;
        }
    }
    a.at(0x10369270);a.compare(3,0x0,false);
    if(!a.equal()) {
        a.at(0x10369278);a.load<4>(3,2,-0x7b7c,false);a.move(4,17);a.load<4>(3,3,0x0,false);
        a.call(0x10082f20);a.load<1>(0,3,0x38,false);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);
        a.sign8(0,0,false);a.compare(0,0x5,true);
        if(a.equal()) {
            a.at(0x103692a0);a.value(24,0x0);
        }
    }
    a.at(0x103692a4);a.load<4>(3,2,-0x7b7c,false);a.move(4,17);a.move(5,24);
    a.load<4>(3,3,0x0,false);a.call(0x10082f90);a.combine(27,3,3,true,false);
    if(a.equal()) {
        a.at(0x103692c8);a.value(23,0x0);
    } else {
        a.at(0x103692c0);a.load<4>(23,27,0x50,false);a.jump(0x103692cc);
    }
    a.at(0x103692cc);a.compare(18,0x0,false);
    if(!a.equal()) {
        a.at(0x103692d4);a.compare_registers(18,24,false);
        if(a.equal()) {
            a.at(0x103692dc);a.compare(27,0x0,false);
            if(!a.equal()) {
                a.at(0x103692e4);a.load<4>(3,2,-0x7b80,false);a.load<4>(5,2,-0x7b80,false);a.load<4>(4,3,0x0,false);
                a.offset(3,27,0x30);a.load<4>(5,5,0x4,false);a.call(0x10718130);a.load<4>(4,17,0x14,false);
                a.value(6,0x43300000);a.mask(0,23,0x10,true,0);a.load<2>(5,18,0x1ec,true);a.load<2>(4,4,0xa,true);
                a.store<4>(1,0x10c,6);a.subtract(0,4,5);a.floating_load(1,16,0x8,false);a.exclusive_flags(0,0,0x80000000);
                a.floating_load(2,16,0x68,false);a.store<4>(1,0x110,0);a.floating_load(0,1,0x10c,false);a.floating_subtract(0,0,1,true);
                a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x114);a.load<4>(5,1,0x118,false);
                if(!a.equal()) {
                    a.at(0x10369344);a.sign16(0,5,false);a.store<4>(1,0x11c,6);a.exclusive_flags(0,0,0x80000000);
                    a.store<4>(1,0x120,0);a.floating_load(0,1,0x11c,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                    a.floating_integer(0,0);a.floating_store(0,1,0x124);a.load<4>(5,1,0x128,false);
                }
                a.at(0x1036936c);a.sign16(0,5,true);
                if(a.at_least()) {
                    a.at(0x1036937c);a.load<1>(0,17,0x65,false);a.sign16(7,5,false);a.sign8(0,0,false);
                    a.multiply(6,0,0xa);a.offset(0,6,0x465);a.compare_registers(7,0,true);
                    if(!a.nonpositive()) {
                        a.at(0x10369398);a.sign16(5,0,false);
                    }
                } else {
                    a.at(0x10369374);a.value(5,0x0);a.jump(0x1036939c);
                }
                a.at(0x1036939c);a.value(6,0x10000);a.offset(0,6,0x61a8);a.intersect_record(0,23,0);
                if(a.equal()) {
                    a.at(0x103693ac);a.value(6,0xea0f0000);a.load<4>(0,17,0x0,false);a.offset(6,6,-0x5f15);
                    a.multiply_high(6,6,0);a.add(6,6,0);a.shift_right(6,6,5);a.mask(7,6,0x1,false,1);
                    a.add(6,6,7);a.multiply(6,6,0x23);a.subtract(6,6,0);a.offset(6,6,0x14);
                    a.compare_registers(3,6,true);
                    if(!a.at_least()) {
                        a.at(0x103693e0);a.multiply(3,0,0x64);a.load<4>(6,13,0x0,false);a.offset(0,3,0xc);
                        a.indexed_load<1>(0,6,0);a.mask(0,0,0xf,true,28);
                        if(a.equal()) {
                            a.at(0x103693f8);a.load<1>(0,17,0x65,false);a.sign16(3,5,false);a.load<1>(5,17,0x63,false);
                            a.sign8(0,0,false);a.sign8(5,5,false);a.multiply(0,0,0x19);a.multiply(5,5,0x14);
                            a.add(3,0,3);a.add(4,4,5);a.offset(0,3,0x109a);a.compare_registers(4,0,true);
                            if(!a.at_least()) {
                                a.at(0x10369428);a.floating_load(1,16,0x160,false);a.jump(0x1036d21c);
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
    a.at(0x10369430);a.compare_registers(18,24,false);
    if(a.equal()) {
        a.at(0x103695d4);a.compare(27,0x0,false);
        if(!a.equal()) {
            a.at(0x103695dc);a.compare(23,0x0,true);
            if(!a.equal()) {
                a.at(0x103695e4);a.compare(23,0x2,true);
                if(!a.equal()) {
                    a.at(0x103695ec);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                    return;
                }
            }
        }
        a.at(0x103695f4);a.floating_load(31,16,0x160,false);
    } else {
        a.at(0x10369438);a.compare(27,0x0,false);
        if(!a.equal()) {
            a.at(0x10369440);a.compare(24,0x0,false);
            if(!a.equal()) {
                a.at(0x10369448);a.load<4>(4,24,0x2c,false);a.compare(4,0x0,false);
                if(!a.equal()) {
                    a.at(0x10369454);a.load<4>(3,18,0x2c,false);a.compare(3,0x0,false);
                    if(!a.equal()) {
                        a.at(0x10369460);a.load<1>(3,3,0x119,false);a.load<1>(0,4,0x119,false);a.sign8(3,3,false);
                        a.sign8(0,0,false);a.compare_registers(3,0,true);
                        if(a.equal()) {
                            a.at(0x10369478);a.mask(0,23,0x20,true,0);
                            if(!a.equal()) {
                                a.at(0x10369480);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                                return;
                            }
                        }
                    }
                }
            }
        }
        a.at(0x10369488);a.compare(22,0x10,true);
        if(!a.at_least()) {
            a.at(0x10369490);a.load<4>(4,18,0x2c,false);a.move(3,17);a.value(5,0x1);
            a.call(0x100cab10);a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x103694a8);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                return;
            }
        }
        a.at(0x103694b0);a.compare(27,0x0,false);a.floating_load(31,16,0x2c0,false);
        if(a.equal()) {
            a.at(0x10369560);a.compare(24,0x0,false);
            if(a.equal()) {
                a.at(0x10369568);a.load<4>(3,2,-0x7b80,false);a.value(0,0x43300000);a.load<2>(5,17,0x4e,false);
                a.load<2>(3,3,0x2,false);a.load<2>(4,17,0x4c,true);a.subtract(5,5,3);a.load<4>(3,2,-0x7b80,false);
                a.multiply(5,5,0x16d);a.store<4>(1,0x10c,0);a.load<2>(3,3,0x0,true);a.floating_load(2,16,0x8,false);
                a.subtract(0,4,3);a.floating_load(0,16,0x2e8,false);a.add(5,5,0);a.floating_load(3,16,0x2e0,false);
                a.product(0,5,5);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x110,0);a.floating_load(1,1,0x10c,false);
                a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_compare(3,0);
                if(a.at_least()) {
                    a.at(0x103695c8);a.floating_move(3,0);
                } else {
                    a.at(0x103695c4);a.jump(0x103695cc);
                }
                a.at(0x103695cc);a.floating_sum(31,31,3);a.jump(0x103695f8);
            }
        } else {
            a.at(0x103694bc);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
            if(a.equal()) {
                a.at(0x10369560);a.compare(24,0x0,false);
                if(a.equal()) {
                    a.at(0x10369568);a.load<4>(3,2,-0x7b80,false);a.value(0,0x43300000);a.load<2>(5,17,0x4e,false);
                    a.load<2>(3,3,0x2,false);a.load<2>(4,17,0x4c,true);a.subtract(5,5,3);a.load<4>(3,2,-0x7b80,false);
                    a.multiply(5,5,0x16d);a.store<4>(1,0x10c,0);a.load<2>(3,3,0x0,true);a.floating_load(2,16,0x8,false);
                    a.subtract(0,4,3);a.floating_load(0,16,0x2e8,false);a.add(5,5,0);a.floating_load(3,16,0x2e0,false);
                    a.product(0,5,5);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x110,0);a.floating_load(1,1,0x10c,false);
                    a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_compare(3,0);
                    if(a.at_least()) {
                        a.at(0x103695c8);a.floating_move(3,0);
                    } else {
                        a.at(0x103695c4);a.jump(0x103695cc);
                    }
                    a.at(0x103695cc);a.floating_sum(31,31,3);a.jump(0x103695f8);
                }
            } else {
                a.at(0x103694c8);a.load<4>(3,2,-0x7b80,false);a.value(4,0x43300000);a.load<2>(0,27,0x32,false);
                a.compare(21,0x0,false);a.load<2>(5,3,0x2,false);a.load<2>(3,3,0x0,true);a.subtract(5,5,0);
                a.load<2>(0,27,0x30,true);a.multiply(5,5,0x16d);a.store<4>(1,0x10c,4);a.subtract(0,3,0);
                a.floating_load(3,16,0x8,false);a.floating_load(2,16,0x3b8,false);a.add(5,5,0);a.exclusive_flags(0,5,0x80000000);
                a.floating_load(0,16,0x200,false);a.store<4>(1,0x110,0);a.floating_load(1,1,0x10c,false);a.floating_subtract(1,1,3,false);
                a.floating_negative_product_difference(4,2,1,0);
                if(!a.equal()) {
                    a.at(0x1036951c);a.load<1>(0,17,0x66,false);a.store<4>(1,0x10c,4);a.sign8(0,0,false);
                    a.floating_load(2,16,0x3c0,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(0,16,0x180,false);a.store<4>(1,0x110,0);
                    a.floating_load(1,1,0x10c,false);a.floating_subtract(1,1,3,false);a.floating_product_sum(0,2,1,0);a.floating_compare(4,0);
                    if(!a.at_least()) {
                        a.at(0x1036954c);a.load<4>(0,21,0x0,false);a.flags(0,0,0x1);a.store<4>(21,0x0,0);
                    }
                }
                a.at(0x10369558);a.floating_product(31,31,4);a.jump(0x103695f8);
            }
        }
    }
    a.at(0x103695f8);a.sign8(0,19,true);
    if(!a.equal()) {
        a.at(0x10369600);a.load<4>(4,17,0x14,false);a.compare(4,0x0,false);
        if(!a.equal()) {
            a.at(0x1036960c);a.load<2>(0,4,0xc,true);a.compare(0,0x128e,true);
            if(!a.at_least()) {
                a.at(0x10369618);a.compare(18,0x0,false);
                if(!a.equal()) {
                    a.at(0x10369620);a.compare(24,0x0,false);
                    if(!a.equal()) {
                        a.at(0x10369628);a.compare_registers(18,24,false);
                        if(!a.equal()) {
                            a.at(0x10369630);a.mask(0,23,0x10,true,0);
                            if(!a.equal()) {
                                a.at(0x10369638);a.load<2>(5,18,0x1ec,true);a.load<2>(0,24,0x1ec,true);a.compare_registers(5,0,true);
                                if(a.nonpositive()) {
                                    a.at(0x10369698);a.compare(5,0xabe,true);
                                    if(a.nonpositive()) {
                                        a.at(0x103696f8);a.compare(5,0x109a,true);
                                        if(!a.nonpositive()) {
                                            a.at(0x10369700);a.load<4>(3,18,0x2c,false);a.load<4>(0,24,0x2c,false);a.compare_registers(3,0,false);
                                            if(a.equal()) {
                                                a.at(0x10369710);a.load<2>(0,4,0xa,true);a.compare(0,0x186a,true);
                                                if(a.nonpositive()) {
                                                    return select_profile_context_scale(a);
                                                } else {
                                                    a.at(0x1036971c);a.compare(5,0x1482,true);
                                                    if(a.at_least()) {
                                                        return select_profile_context_scale(a);
                                                    } else {
                                                        a.at(0x10369724);a.floating_load(0,16,0x3d0,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        a.at(0x103696a0);a.load<4>(3,18,0x2c,false);a.load<4>(0,24,0x2c,false);a.compare_registers(3,0,false);
                                        if(a.equal()) {
                                            a.at(0x103696b8);a.load<2>(0,4,0xa,true);a.compare_registers(5,0,true);
                                            if(a.nonpositive()) {
                                                a.at(0x103696f8);a.compare(5,0x109a,true);
                                                if(!a.nonpositive()) {
                                                    a.at(0x10369700);a.load<4>(3,18,0x2c,false);a.load<4>(0,24,0x2c,false);a.compare_registers(3,0,false);
                                                    if(a.equal()) {
                                                        a.at(0x10369710);a.load<2>(0,4,0xa,true);a.compare(0,0x186a,true);
                                                        if(a.nonpositive()) {
                                                            return select_profile_context_scale(a);
                                                        } else {
                                                            a.at(0x1036971c);a.compare(5,0x1482,true);
                                                            if(a.at_least()) {
                                                                return select_profile_context_scale(a);
                                                            } else {
                                                                a.at(0x10369724);a.floating_load(0,16,0x3d0,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                a.at(0x103696c4);a.compare(0,0x1c52,true);
                                                if(a.positive()) {
                                                    a.at(0x103696e0);a.floating_load(0,16,0x3c8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                } else {
                                                    a.at(0x103696cc);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
                                                    a.sign8(0,3,true);
                                                    if(!a.equal()) {
                                                        a.at(0x103696ec);a.floating_load(0,16,0x378,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                    } else {
                                                        a.at(0x103696e0);a.floating_load(0,16,0x3c8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                    }
                                                }
                                            }
                                        } else {
                                            a.at(0x103696b0);a.compare(5,0x1676,true);
                                            if(a.nonpositive()) {
                                                a.at(0x103696f8);a.compare(5,0x109a,true);
                                                if(!a.nonpositive()) {
                                                    a.at(0x10369700);a.load<4>(3,18,0x2c,false);a.load<4>(0,24,0x2c,false);a.compare_registers(3,0,false);
                                                    if(a.equal()) {
                                                        a.at(0x10369710);a.load<2>(0,4,0xa,true);a.compare(0,0x186a,true);
                                                        if(a.nonpositive()) {
                                                            return select_profile_context_scale(a);
                                                        } else {
                                                            a.at(0x1036971c);a.compare(5,0x1482,true);
                                                            if(a.at_least()) {
                                                                return select_profile_context_scale(a);
                                                            } else {
                                                                a.at(0x10369724);a.floating_load(0,16,0x3d0,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                a.at(0x103696b8);a.load<2>(0,4,0xa,true);a.compare_registers(5,0,true);
                                                if(a.nonpositive()) {
                                                    a.at(0x103696f8);a.compare(5,0x109a,true);
                                                    if(!a.nonpositive()) {
                                                        a.at(0x10369700);a.load<4>(3,18,0x2c,false);a.load<4>(0,24,0x2c,false);a.compare_registers(3,0,false);
                                                        if(a.equal()) {
                                                            a.at(0x10369710);a.load<2>(0,4,0xa,true);a.compare(0,0x186a,true);
                                                            if(a.nonpositive()) {
                                                                return select_profile_context_scale(a);
                                                            } else {
                                                                a.at(0x1036971c);a.compare(5,0x1482,true);
                                                                if(a.at_least()) {
                                                                    return select_profile_context_scale(a);
                                                                } else {
                                                                    a.at(0x10369724);a.floating_load(0,16,0x3d0,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                                }
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    a.at(0x103696c4);a.compare(0,0x1c52,true);
                                                    if(a.positive()) {
                                                        a.at(0x103696e0);a.floating_load(0,16,0x3c8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                    } else {
                                                        a.at(0x103696cc);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
                                                        a.sign8(0,3,true);
                                                        if(!a.equal()) {
                                                            a.at(0x103696ec);a.floating_load(0,16,0x378,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                        } else {
                                                            a.at(0x103696e0);a.floating_load(0,16,0x3c8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    a.at(0x10369648);a.load<4>(3,18,0x2c,false);a.load<4>(0,24,0x2c,false);a.compare_registers(3,0,false);
                                    if(a.equal()) {
                                        a.at(0x10369660);a.load<2>(0,4,0xa,true);a.compare(0,0x1c52,true);
                                        if(a.positive()) {
                                            a.at(0x10369680);a.floating_load(0,16,0xc8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                        } else {
                                            a.at(0x1036966c);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
                                            a.sign8(0,3,true);
                                            if(!a.equal()) {
                                                a.at(0x1036968c);a.floating_load(0,16,0x178,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                            } else {
                                                a.at(0x10369680);a.floating_load(0,16,0xc8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                            }
                                        }
                                    } else {
                                        a.at(0x10369658);a.compare(5,0x1676,true);
                                        if(a.nonpositive()) {
                                            a.at(0x10369698);a.compare(5,0xabe,true);
                                            if(a.nonpositive()) {
                                                a.at(0x103696f8);a.compare(5,0x109a,true);
                                                if(!a.nonpositive()) {
                                                    a.at(0x10369700);a.load<4>(3,18,0x2c,false);a.load<4>(0,24,0x2c,false);a.compare_registers(3,0,false);
                                                    if(a.equal()) {
                                                        a.at(0x10369710);a.load<2>(0,4,0xa,true);a.compare(0,0x186a,true);
                                                        if(a.nonpositive()) {
                                                            return select_profile_context_scale(a);
                                                        } else {
                                                            a.at(0x1036971c);a.compare(5,0x1482,true);
                                                            if(a.at_least()) {
                                                                return select_profile_context_scale(a);
                                                            } else {
                                                                a.at(0x10369724);a.floating_load(0,16,0x3d0,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                            }
                                                        }
                                                    }
                                                }
                                            } else {
                                                a.at(0x103696a0);a.load<4>(3,18,0x2c,false);a.load<4>(0,24,0x2c,false);a.compare_registers(3,0,false);
                                                if(a.equal()) {
                                                    a.at(0x103696b8);a.load<2>(0,4,0xa,true);a.compare_registers(5,0,true);
                                                    if(a.nonpositive()) {
                                                        a.at(0x103696f8);a.compare(5,0x109a,true);
                                                        if(!a.nonpositive()) {
                                                            a.at(0x10369700);a.load<4>(3,18,0x2c,false);a.load<4>(0,24,0x2c,false);a.compare_registers(3,0,false);
                                                            if(a.equal()) {
                                                                a.at(0x10369710);a.load<2>(0,4,0xa,true);a.compare(0,0x186a,true);
                                                                if(a.nonpositive()) {
                                                                    return select_profile_context_scale(a);
                                                                } else {
                                                                    a.at(0x1036971c);a.compare(5,0x1482,true);
                                                                    if(a.at_least()) {
                                                                        return select_profile_context_scale(a);
                                                                    } else {
                                                                        a.at(0x10369724);a.floating_load(0,16,0x3d0,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    } else {
                                                        a.at(0x103696c4);a.compare(0,0x1c52,true);
                                                        if(a.positive()) {
                                                            a.at(0x103696e0);a.floating_load(0,16,0x3c8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                        } else {
                                                            a.at(0x103696cc);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
                                                            a.sign8(0,3,true);
                                                            if(!a.equal()) {
                                                                a.at(0x103696ec);a.floating_load(0,16,0x378,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                            } else {
                                                                a.at(0x103696e0);a.floating_load(0,16,0x3c8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    a.at(0x103696b0);a.compare(5,0x1676,true);
                                                    if(a.nonpositive()) {
                                                        a.at(0x103696f8);a.compare(5,0x109a,true);
                                                        if(!a.nonpositive()) {
                                                            a.at(0x10369700);a.load<4>(3,18,0x2c,false);a.load<4>(0,24,0x2c,false);a.compare_registers(3,0,false);
                                                            if(a.equal()) {
                                                                a.at(0x10369710);a.load<2>(0,4,0xa,true);a.compare(0,0x186a,true);
                                                                if(a.nonpositive()) {
                                                                    return select_profile_context_scale(a);
                                                                } else {
                                                                    a.at(0x1036971c);a.compare(5,0x1482,true);
                                                                    if(a.at_least()) {
                                                                        return select_profile_context_scale(a);
                                                                    } else {
                                                                        a.at(0x10369724);a.floating_load(0,16,0x3d0,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    } else {
                                                        a.at(0x103696b8);a.load<2>(0,4,0xa,true);a.compare_registers(5,0,true);
                                                        if(a.nonpositive()) {
                                                            a.at(0x103696f8);a.compare(5,0x109a,true);
                                                            if(!a.nonpositive()) {
                                                                a.at(0x10369700);a.load<4>(3,18,0x2c,false);a.load<4>(0,24,0x2c,false);a.compare_registers(3,0,false);
                                                                if(a.equal()) {
                                                                    a.at(0x10369710);a.load<2>(0,4,0xa,true);a.compare(0,0x186a,true);
                                                                    if(a.nonpositive()) {
                                                                        return select_profile_context_scale(a);
                                                                    } else {
                                                                        a.at(0x1036971c);a.compare(5,0x1482,true);
                                                                        if(a.at_least()) {
                                                                            return select_profile_context_scale(a);
                                                                        } else {
                                                                            a.at(0x10369724);a.floating_load(0,16,0x3d0,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        } else {
                                                            a.at(0x103696c4);a.compare(0,0x1c52,true);
                                                            if(a.positive()) {
                                                                a.at(0x103696e0);a.floating_load(0,16,0x3c8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                            } else {
                                                                a.at(0x103696cc);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
                                                                a.sign8(0,3,true);
                                                                if(!a.equal()) {
                                                                    a.at(0x103696ec);a.floating_load(0,16,0x378,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                                } else {
                                                                    a.at(0x103696e0);a.floating_load(0,16,0x3c8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            a.at(0x10369660);a.load<2>(0,4,0xa,true);a.compare(0,0x1c52,true);
                                            if(a.positive()) {
                                                a.at(0x10369680);a.floating_load(0,16,0xc8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                            } else {
                                                a.at(0x1036966c);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
                                                a.sign8(0,3,true);
                                                if(!a.equal()) {
                                                    a.at(0x1036968c);a.floating_load(0,16,0x178,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                } else {
                                                    a.at(0x10369680);a.floating_load(0,16,0xc8,false);a.floating_product(31,31,0);a.jump(0x10369760);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return adjust_interest_relationships(a);
}

void select_profile_context_scale(Call& a) {
    a.at(0x10369730);a.compare(0,0x1c52,true);
    if(a.positive()) {
        return apply_profile_context_scale(a);
    } else {
        a.at(0x10369738);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
        a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x10369758);a.floating_load(0,16,0xc8,false);a.floating_product(31,31,0);
        } else {
            return apply_profile_context_scale(a);
        }
    }
    return adjust_interest_relationships(a);
}

void apply_profile_context_scale(Call& a) {
    a.at(0x1036974c);a.floating_load(0,16,0x200,false);a.floating_product(31,31,0);a.jump(0x10369760);
    return adjust_interest_relationships(a);
}

void adjust_interest_relationships(Call& a) {
    a.at(0x10369760);a.move(3,17);a.move(4,18);a.call(0x100da050);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x10369774);a.floating_load(0,16,0x2f0,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
        if(!a.equal()) {
            a.at(0x10369784);a.load<4>(0,21,0x0,false);a.flags(0,0,0x2);a.store<4>(21,0x0,0);
        }
    }
    a.at(0x10369790);a.move(3,17);a.move(4,24);a.call(0x100da050);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x103697a4);a.load<1>(4,17,0x66,false);a.load<1>(0,17,0x67,false);a.load<1>(3,17,0x65,false);
        a.add(0,0,4);a.add(0,3,0);a.sign8(0,0,false);a.compare(0,0x2d,true);
        if(a.nonpositive()) {
            a.at(0x103697d0);a.compare(0,0x19,true);
            if(a.nonpositive()) {
                a.at(0x103697e4);a.floating_load(0,16,0xe8,false);a.floating_product(31,31,0);
            } else {
                a.at(0x103697d8);a.floating_load(0,16,0x178,false);a.floating_product(31,31,0);a.jump(0x103697ec);
            }
        } else {
            a.at(0x103697c4);a.floating_load(0,16,0xc8,false);a.floating_product(31,31,0);a.jump(0x103697ec);
        }
        a.at(0x103697ec);a.compare(21,0x0,false);
        if(!a.equal()) {
            a.at(0x103697f4);a.load<4>(0,21,0x0,false);a.flags(0,0,0x4);a.store<4>(21,0x0,0);
        }
    }
    a.at(0x10369800);a.move(3,17);a.move(4,24);a.call(0x100da0b0);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x10369814);a.move(3,17);a.move(4,24);a.call(0x100d3150);
        a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x10369828);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
            if(a.equal()) {
                a.at(0x10369834);a.floating_load(0,16,0x68,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                if(!a.equal()) {
                    a.at(0x10369844);a.load<4>(0,21,0x0,false);a.flags(0,0,0x8);a.store<4>(21,0x0,0);
                }
            }
        }
    }
    a.at(0x10369850);a.move(3,17);a.move(4,18);a.call(0x100da0b0);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x10369864);a.compare(21,0x0,false);
        if(!a.equal()) {
            a.at(0x1036986c);a.load<4>(0,21,0x0,false);a.flags(0,0,0x10);a.store<4>(21,0x0,0);
        }
        a.at(0x10369878);a.floating_load(0,16,0x140,false);a.floating_product(31,31,0);
    }
    a.at(0x10369880);a.compare(22,0x22,true);
    if(!a.nonpositive()) {
        a.at(0x10369888);a.compare_registers(24,18,false);
        if(!a.equal()) {
            a.at(0x10369890);a.load<4>(3,18,0x2c,false);a.load<4>(0,17,0xc,false);a.compare_registers(3,0,false);
            if(!a.equal()) {
                a.at(0x103698a0);a.floating_load(0,16,0xa0,false);a.floating_product(31,31,0);
            }
        }
    }
    a.at(0x103698a8);a.compare(24,0x0,false);
    if(!a.equal()) {
        a.at(0x103698b0);a.compare_registers(24,18,false);
        if(!a.equal()) {
            a.at(0x103698b8);a.load<4>(3,2,-0x7a04,false);a.load<4>(4,18,0x0,false);a.load<4>(3,3,0x0,false);
            a.compare_registers(4,3,true);
            if(a.equal()) {
                a.at(0x103698d8);a.load<4>(4,24,0x0,false);a.compare_registers(4,3,true);
                if(a.equal()) {
                    a.at(0x103698f0);a.load<1>(0,17,0x65,false);a.value(3,0x1);a.sign8(0,0,false);
                    a.compare(0,0x1,true);
                    if(!a.less()) {
                        a.at(0x10369904);a.move(3,0);
                    }
                    a.at(0x10369908);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x110,3);
                    a.compare(21,0x0,false);a.floating_load(1,16,0x8,false);a.store<4>(1,0x10c,0);a.floating_load(2,16,0x2a8,false);
                    a.floating_load(0,1,0x10c,false);a.floating_subtract(0,0,1,false);a.floating_difference(0,2,0,true,false);a.floating_product(31,31,0);
                    if(!a.equal()) {
                        a.at(0x10369938);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20);a.store<4>(21,0x0,0);
                        a.jump(0x10369b80);
                    }
                } else {
                    a.at(0x103698e4);a.load<4>(0,15,0x0,false);a.compare_registers(4,0,true);
                    if(!a.equal()) {
                        a.at(0x10369948);a.load<4>(4,18,0x2c,false);a.load<4>(3,2,-0x74ec,false);a.load<4>(0,4,0x68,false);
                        a.load<4>(5,3,0x0,false);a.compare_registers(0,5,true);
                        if(!a.equal()) {
                            return compare_interest_country_context(a);
                        } else {
                            a.at(0x10369960);a.load<4>(3,24,0x2c,false);a.load<4>(4,25,0x0,false);a.load<4>(3,3,0x68,false);
                            a.compare_registers(3,4,true);
                            if(!a.equal()) {
                                return compare_interest_country_context(a);
                            } else {
                                a.at(0x10369974);a.load<4>(3,17,0xc,false);a.load<4>(3,3,0x68,false);a.compare_registers(3,4,true);
                                if(!a.equal()) {
                                    return compare_interest_country_context(a);
                                } else {
                                    a.at(0x10369984);a.floating_load(0,16,0x300,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                    if(!a.equal()) {
                                        a.at(0x10369994);a.load<4>(0,21,0x0,false);a.flags(0,0,0x2);a.store<4>(21,0x0,0);
                                        a.jump(0x10369b80);
                                    }
                                }
                            }
                        }
                    } else {
                        a.at(0x103698f0);a.load<1>(0,17,0x65,false);a.value(3,0x1);a.sign8(0,0,false);
                        a.compare(0,0x1,true);
                        if(!a.less()) {
                            a.at(0x10369904);a.move(3,0);
                        }
                        a.at(0x10369908);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x110,3);
                        a.compare(21,0x0,false);a.floating_load(1,16,0x8,false);a.store<4>(1,0x10c,0);a.floating_load(2,16,0x2a8,false);
                        a.floating_load(0,1,0x10c,false);a.floating_subtract(0,0,1,false);a.floating_difference(0,2,0,true,false);a.floating_product(31,31,0);
                        if(!a.equal()) {
                            a.at(0x10369938);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20);a.store<4>(21,0x0,0);
                            a.jump(0x10369b80);
                        }
                    }
                }
            } else {
                a.at(0x103698cc);a.load<4>(0,15,0x0,false);a.compare_registers(4,0,true);
                if(!a.equal()) {
                    a.at(0x10369948);a.load<4>(4,18,0x2c,false);a.load<4>(3,2,-0x74ec,false);a.load<4>(0,4,0x68,false);
                    a.load<4>(5,3,0x0,false);a.compare_registers(0,5,true);
                    if(!a.equal()) {
                        return compare_interest_country_context(a);
                    } else {
                        a.at(0x10369960);a.load<4>(3,24,0x2c,false);a.load<4>(4,25,0x0,false);a.load<4>(3,3,0x68,false);
                        a.compare_registers(3,4,true);
                        if(!a.equal()) {
                            return compare_interest_country_context(a);
                        } else {
                            a.at(0x10369974);a.load<4>(3,17,0xc,false);a.load<4>(3,3,0x68,false);a.compare_registers(3,4,true);
                            if(!a.equal()) {
                                return compare_interest_country_context(a);
                            } else {
                                a.at(0x10369984);a.floating_load(0,16,0x300,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                if(!a.equal()) {
                                    a.at(0x10369994);a.load<4>(0,21,0x0,false);a.flags(0,0,0x2);a.store<4>(21,0x0,0);
                                    a.jump(0x10369b80);
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x103698d8);a.load<4>(4,24,0x0,false);a.compare_registers(4,3,true);
                    if(a.equal()) {
                        a.at(0x103698f0);a.load<1>(0,17,0x65,false);a.value(3,0x1);a.sign8(0,0,false);
                        a.compare(0,0x1,true);
                        if(!a.less()) {
                            a.at(0x10369904);a.move(3,0);
                        }
                        a.at(0x10369908);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x110,3);
                        a.compare(21,0x0,false);a.floating_load(1,16,0x8,false);a.store<4>(1,0x10c,0);a.floating_load(2,16,0x2a8,false);
                        a.floating_load(0,1,0x10c,false);a.floating_subtract(0,0,1,false);a.floating_difference(0,2,0,true,false);a.floating_product(31,31,0);
                        if(!a.equal()) {
                            a.at(0x10369938);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20);a.store<4>(21,0x0,0);
                            a.jump(0x10369b80);
                        }
                    } else {
                        a.at(0x103698e4);a.load<4>(0,15,0x0,false);a.compare_registers(4,0,true);
                        if(!a.equal()) {
                            a.at(0x10369948);a.load<4>(4,18,0x2c,false);a.load<4>(3,2,-0x74ec,false);a.load<4>(0,4,0x68,false);
                            a.load<4>(5,3,0x0,false);a.compare_registers(0,5,true);
                            if(!a.equal()) {
                                return compare_interest_country_context(a);
                            } else {
                                a.at(0x10369960);a.load<4>(3,24,0x2c,false);a.load<4>(4,25,0x0,false);a.load<4>(3,3,0x68,false);
                                a.compare_registers(3,4,true);
                                if(!a.equal()) {
                                    return compare_interest_country_context(a);
                                } else {
                                    a.at(0x10369974);a.load<4>(3,17,0xc,false);a.load<4>(3,3,0x68,false);a.compare_registers(3,4,true);
                                    if(!a.equal()) {
                                        return compare_interest_country_context(a);
                                    } else {
                                        a.at(0x10369984);a.floating_load(0,16,0x300,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                        if(!a.equal()) {
                                            a.at(0x10369994);a.load<4>(0,21,0x0,false);a.flags(0,0,0x2);a.store<4>(21,0x0,0);
                                            a.jump(0x10369b80);
                                        }
                                    }
                                }
                            }
                        } else {
                            a.at(0x103698f0);a.load<1>(0,17,0x65,false);a.value(3,0x1);a.sign8(0,0,false);
                            a.compare(0,0x1,true);
                            if(!a.less()) {
                                a.at(0x10369904);a.move(3,0);
                            }
                            a.at(0x10369908);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x110,3);
                            a.compare(21,0x0,false);a.floating_load(1,16,0x8,false);a.store<4>(1,0x10c,0);a.floating_load(2,16,0x2a8,false);
                            a.floating_load(0,1,0x10c,false);a.floating_subtract(0,0,1,false);a.floating_difference(0,2,0,true,false);a.floating_product(31,31,0);
                            if(!a.equal()) {
                                a.at(0x10369938);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20);a.store<4>(21,0x0,0);
                                a.jump(0x10369b80);
                            }
                        }
                    }
                }
            }
        }
    }
    return adjust_interest_club_services(a);
}

void compare_interest_country_context(Call& a) {
    a.at(0x103699a4);a.load<4>(3,25,0x0,false);a.compare_registers(0,3,true);
    if(!a.equal()) {
        return compare_linked_club_context(a);
    } else {
        a.at(0x103699b0);a.load<4>(3,24,0x2c,false);a.load<4>(3,3,0x68,false);a.compare_registers(3,5,true);
        if(!a.equal()) {
            return compare_linked_club_context(a);
        } else {
            a.at(0x103699c0);a.load<4>(3,17,0xc,false);a.load<4>(3,3,0x68,false);a.compare_registers(3,5,true);
            if(!a.equal()) {
                return compare_linked_club_context(a);
            } else {
                a.at(0x103699d0);a.floating_load(0,16,0x48,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                if(!a.equal()) {
                    a.at(0x103699e0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x2);a.store<4>(21,0x0,0);
                    a.jump(0x10369b80);
                }
            }
        }
    }
    return adjust_interest_club_services(a);
}

void compare_linked_club_context(Call& a) {
    a.at(0x103699f0);a.load<4>(3,24,0x74,false);a.compare_registers(18,3,false);
    if(a.equal()) {
        return calculate_linked_club_scale(a);
    } else {
        a.at(0x103699fc);a.load<4>(3,24,0x78,false);a.compare_registers(18,3,false);
        if(a.equal()) {
            return calculate_linked_club_scale(a);
        } else {
            a.at(0x10369a08);a.load<4>(3,24,0x7c,false);a.compare_registers(18,3,false);
            if(a.equal()) {
                return calculate_linked_club_scale(a);
            }
        }
    }
    return apply_club_pair_scale(a);
}

void calculate_linked_club_scale(Call& a) {
    a.at(0x10369a14);a.load<4>(3,2,-0x7a14,false);a.load<4>(3,3,0x0,false);a.compare_registers(0,3,true);
    if(!a.equal()) {
        a.at(0x10369a5c);a.load<1>(0,17,0x65,false);a.value(3,0x1);a.sign8(0,0,false);
        a.compare(0,0x1,true);
        if(!a.less()) {
            a.at(0x10369a70);a.move(3,0);
        }
        a.at(0x10369a74);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x110,3);
        a.floating_load(1,16,0x8,false);a.store<4>(1,0x10c,0);a.floating_load(2,16,0x58,false);a.floating_load(0,1,0x10c,false);
        a.floating_subtract(0,0,1,false);a.floating_difference(1,2,0,true,false);
    } else {
        a.at(0x10369a24);a.load<1>(3,17,0x65,false);a.value(0,0x43300000);a.store<4>(1,0x10c,0);
        a.sign8(0,3,false);a.floating_load(2,16,0x8,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(0,16,0xb8,false);
        a.store<4>(1,0x110,0);a.floating_load(3,16,0x80,false);a.floating_load(1,1,0x10c,false);a.floating_subtract(1,1,2,false);
        a.floating_difference(0,1,0,true,false);a.floating_subtract(1,3,0,false);a.jump(0x10369a98);
    }
    return apply_linked_club_scale(a);
}

void apply_linked_club_scale(Call& a) {
    a.at(0x10369a98);a.floating_product(31,31,1);a.compare(21,0x0,false);
    if(!a.equal()) {
        a.at(0x10369aa4);a.floating_load(0,16,0x1f0,false);a.floating_compare(1,0);
        if(!a.at_least()) {
            a.at(0x10369ab0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20);a.store<4>(21,0x0,0);
        }
    }
    return apply_club_pair_scale(a);
}

void apply_club_pair_scale(Call& a) {
    a.at(0x10369abc);a.move(3,18);a.move(4,24);a.call(0x10464160);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x10369ad0);a.load<4>(3,2,-0x7a14,false);a.load<4>(4,18,0x2c,false);a.load<4>(0,3,0x0,false);
        a.load<4>(3,4,0x68,false);a.compare_registers(3,0,true);
        if(!a.equal()) {
            a.at(0x10369b20);a.load<1>(0,17,0x65,false);a.value(3,0x1);a.sign8(0,0,false);
            a.compare(0,0x1,true);
            if(!a.less()) {
                a.at(0x10369b34);a.move(3,0);
            }
            a.at(0x10369b38);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x110,3);
            a.floating_load(1,16,0x8,false);a.store<4>(1,0x10c,0);a.floating_load(2,16,0xa0,false);a.floating_load(0,1,0x10c,false);
            a.floating_subtract(0,0,1,false);a.floating_difference(1,2,0,true,false);
        } else {
            a.at(0x10369ae8);a.load<1>(3,17,0x65,false);a.value(0,0x43300000);a.store<4>(1,0x10c,0);
            a.sign8(0,3,false);a.floating_load(2,16,0x8,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(0,16,0xb8,false);
            a.store<4>(1,0x110,0);a.floating_load(3,16,0x80,false);a.floating_load(1,1,0x10c,false);a.floating_subtract(1,1,2,false);
            a.floating_difference(0,1,0,true,false);a.floating_subtract(1,3,0,false);a.jump(0x10369b5c);
        }
        a.at(0x10369b5c);a.floating_product(31,31,1);a.compare(21,0x0,false);
        if(!a.equal()) {
            a.at(0x10369b68);a.floating_load(0,16,0x1f0,false);a.floating_compare(1,0);
            if(!a.at_least()) {
                a.at(0x10369b74);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20);a.store<4>(21,0x0,0);
            }
        }
    }
    return adjust_interest_club_services(a);
}

void adjust_interest_club_services(Call& a) {
    a.at(0x10369b80);a.load<4>(3,18,0x90,false);a.move(4,17);a.call(0x100d9520);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x10369b94);a.floating_load(0,16,0xe8,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
        if(!a.equal()) {
            a.at(0x10369ba4);a.load<4>(0,21,0x0,false);a.flags(0,0,0x40);a.store<4>(21,0x0,0);
        }
    }
    a.at(0x10369bb0);a.load<4>(3,18,0x90,false);a.move(4,17);a.call(0x100d9580);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x10369bc4);a.floating_load(0,16,0x2f0,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
        if(!a.equal()) {
            a.at(0x10369bd4);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80);a.store<4>(21,0x0,0);
        }
    }
    a.at(0x10369be0);a.compare(24,0x0,false);
    if(!a.equal()) {
        a.at(0x10369be8);a.load<4>(3,24,0x90,false);a.move(4,17);a.call(0x100d9580);
        a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x10369bfc);a.floating_load(0,16,0xe8,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
            if(!a.equal()) {
                a.at(0x10369c0c);a.load<4>(0,21,0x0,false);a.flags(0,0,0x100);a.store<4>(21,0x0,0);
            }
        }
    }
    a.at(0x10369c18);a.compare(24,0x0,false);
    if(!a.equal()) {
        a.at(0x10369c20);a.load<4>(3,24,0x90,false);a.move(4,17);a.call(0x100d9520);
        a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x10369c34);a.floating_load(0,16,0x68,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
            if(!a.equal()) {
                a.at(0x10369c44);a.load<1>(0,27,0x5c,false);a.mask(0,0,0x8,true,0);
                if(a.equal()) {
                    a.at(0x10369c50);a.load<4>(0,21,0x0,false);a.flags(0,0,0x200);a.store<4>(21,0x0,0);
                }
            }
        }
    }
    a.at(0x10369c5c);a.compare(27,0x0,false);
    if(!a.equal()) {
        a.at(0x10369c84);a.compare_registers(18,24,false);
        if(!a.equal()) {
            a.at(0x10369d64);a.load<1>(3,27,0x4c,false);a.value(0,0x43300000);a.store<4>(1,0x10c,0);
            a.sign8(0,3,false);a.floating_load(3,16,0x8,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(1,16,0x308,false);
            a.store<4>(1,0x110,0);a.floating_load(4,16,0x80,false);a.floating_load(2,1,0x10c,false);a.floating_load(0,16,0x3d8,false);
            a.floating_subtract(2,2,3,false);a.floating_difference(1,2,1,true,false);a.floating_subtract(1,4,1,false);a.floating_product(31,31,1);
            a.floating_compare(1,0);
            if(!a.nonpositive()) {
                a.at(0x10369da8);a.floating_product(31,31,1);
            }
            a.at(0x10369dac);a.mask(0,23,0x10,true,0);
            if(!a.equal()) {
                a.at(0x10369db4);a.floating_load(0,16,0x178,false);a.floating_product(29,29,0);a.floating_round_single(29,29);
            }
            a.at(0x10369dc0);a.mask(0,23,0x100,true,0);
            if(!a.equal()) {
                a.at(0x10369dc8);a.floating_load(0,16,0x178,false);a.floating_product(29,29,0);a.floating_round_single(29,29);
            }
            a.at(0x10369dd4);a.mask(0,23,0x1,true,0);
            if(!a.equal()) {
                a.at(0x10369ddc);a.floating_load(0,16,0x178,false);a.floating_product(30,30,0);a.floating_round_single(30,30);
            }
            a.at(0x10369de8);a.compare(21,0x0,false);
            if(!a.equal()) {
                a.at(0x10369df0);a.floating_load(0,16,0x178,false);a.floating_compare(1,0);
                if(a.nonpositive()) {
                    a.at(0x10369e18);a.floating_load(0,16,0x68,false);a.floating_compare(1,0);
                    if(!a.at_least()) {
                        a.at(0x10369e24);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800);a.store<4>(21,0x0,0);
                    }
                } else {
                    a.at(0x10369dfc);a.load<1>(0,27,0x5c,false);a.mask(0,0,0x8,true,0);
                    if(!a.equal()) {
                        a.at(0x10369e18);a.floating_load(0,16,0x68,false);a.floating_compare(1,0);
                        if(!a.at_least()) {
                            a.at(0x10369e24);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800);a.store<4>(21,0x0,0);
                        }
                    } else {
                        a.at(0x10369e08);a.load<4>(0,21,0x0,false);a.flags(0,0,0x400);a.store<4>(21,0x0,0);
                        a.jump(0x10369e30);
                    }
                }
            }
            a.at(0x10369e30);a.compare(24,0x0,false);
            if(a.equal()) {
                a.at(0x10369ed0);a.load<1>(0,27,0x60,false);a.mask(0,0,0xf,false,0);a.move(26,0);
            } else {
                a.at(0x10369e38);a.load<4>(0,24,0x90,false);a.compare(0,0x0,false);
                if(a.equal()) {
                    a.at(0x10369ed0);a.load<1>(0,27,0x60,false);a.mask(0,0,0xf,false,0);a.move(26,0);
                } else {
                    a.at(0x10369e44);a.load<4>(4,2,-0x7b80,false);a.offset(3,1,0x60);a.value(5,0x96);
                    a.call(0x10717b20);a.load<4>(0,1,0x60,false);a.load<4>(3,24,0x90,false);a.store<4>(1,0x48,0);
                    a.load<4>(0,1,0x64,false);a.load<2>(5,1,0x4a,false);a.load<2>(4,3,0x4e,false);a.store<4>(1,0x4c,0);
                    a.compare_registers(5,4,false);
                    if(a.nonpositive()) {
                        a.at(0x10369e80);
                        if(a.at_least()) {
                            a.at(0x10369e8c);a.load<2>(4,1,0x48,true);a.load<2>(0,3,0x4c,true);a.combine(0,4,0,false,true);
                            a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,0x1,false,1);
                            a.sign8(0,0,false);
                        } else {
                            a.at(0x10369e84);a.value(0,0x0);a.jump(0x10369eac);
                        }
                    } else {
                        a.at(0x10369e78);a.value(0,0x1);a.jump(0x10369eac);
                    }
                    a.at(0x10369eac);a.sign8(0,0,true);
                    if(a.equal()) {
                        a.at(0x10369ed0);a.load<1>(0,27,0x60,false);a.mask(0,0,0xf,false,0);a.move(26,0);
                    } else {
                        a.at(0x10369eb4);a.load<4>(3,2,-0x7b7c,false);a.move(4,17);a.move(5,27);
                        a.load<4>(3,3,0x0,false);a.call(0x1007c140);a.move(26,3);a.jump(0x10369edc);
                    }
                }
            }
            a.at(0x10369edc);a.load<1>(0,27,0x5c,false);a.mask(0,0,0x8,true,0);
            if(!a.equal()) {
                a.at(0x10369ee8);a.sign8(0,26,false);a.compare(0,0x1,true);
                if(a.equal()) {
                    a.at(0x10369f04);a.value(26,0x3);a.jump(0x10369f18);
                } else {
                    a.at(0x10369ef4);
                    if(a.less()) {
                        a.at(0x10369f14);a.value(26,0x7);
                    } else {
                        a.at(0x10369ef8);a.compare(0,0x4,true);
                        if(a.at_least()) {
                            a.at(0x10369f14);a.value(26,0x7);
                        } else {
                            a.at(0x10369f00);a.jump(0x10369f0c);a.at(0x10369f0c);a.value(26,0x4);
                            a.jump(0x10369f18);
                        }
                    }
                }
            }
        } else {
            a.at(0x10369c8c);a.load<1>(4,27,0x4c,false);a.value(3,0x43300000);a.load<4>(0,18,0x90,false);
            a.sign8(4,4,false);a.store<4>(1,0x10c,3);a.exclusive_flags(3,4,0x80000000);a.floating_load(2,16,0x8,false);
            a.store<4>(1,0x110,3);a.compare(0,0x0,false);a.floating_load(0,16,0x308,false);a.floating_load(1,1,0x10c,false);
            a.floating_load(3,16,0x80,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_sum(0,3,0);
            a.floating_product(31,31,0);
            if(a.equal()) {
                a.at(0x10369d5c);a.move(26,20);a.jump(0x10369f18);
            } else {
                a.at(0x10369cd0);a.load<4>(4,2,-0x7b80,false);a.offset(3,1,0x68);a.value(5,0x96);
                a.call(0x10717b20);a.load<4>(0,1,0x68,false);a.load<4>(3,18,0x90,false);a.store<4>(1,0x50,0);
                a.load<4>(0,1,0x6c,false);a.load<2>(5,1,0x52,false);a.load<2>(4,3,0x4e,false);a.store<4>(1,0x54,0);
                a.compare_registers(5,4,false);
                if(a.nonpositive()) {
                    a.at(0x10369d0c);
                    if(a.at_least()) {
                        a.at(0x10369d18);a.load<2>(4,1,0x50,true);a.load<2>(0,3,0x4c,true);a.combine(0,4,0,false,true);
                        a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,0x1,false,1);
                        a.sign8(0,0,false);
                    } else {
                        a.at(0x10369d10);a.value(0,0x0);a.jump(0x10369d38);
                    }
                } else {
                    a.at(0x10369d04);a.value(0,0x1);a.jump(0x10369d38);
                }
                a.at(0x10369d38);a.sign8(0,0,true);
                if(a.equal()) {
                    a.at(0x10369d5c);a.move(26,20);a.jump(0x10369f18);
                } else {
                    a.at(0x10369d40);a.load<4>(3,2,-0x7b7c,false);a.move(4,17);a.move(5,27);
                    a.load<4>(3,3,0x0,false);a.call(0x1007c140);a.move(26,3);a.jump(0x10369f18);
                }
            }
        }
        a.at(0x10369f18);a.load<1>(25,27,0x44,false);
    } else {
        a.at(0x10369c64);a.compare_registers(18,24,false);
        if(!a.equal()) {
            a.at(0x10369c78);a.value(26,0x7);a.value(25,0xffffff9c);a.jump(0x10369f1c);
        } else {
            a.at(0x10369c6c);a.move(26,20);a.value(25,0x0);a.jump(0x10369f1c);
        }
    }
    a.at(0x10369f1c);a.compare_registers(18,24,false);
    if(a.equal()) {
        a.at(0x1036b710);a.move(3,18);a.value(4,0x0);a.value(5,0x1);
        a.value(6,0x0);a.value(7,0x0);a.call(0x1014c2e0);a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x1036b730);a.load<4>(4,2,-0x7b80,false);a.offset(3,1,0x58);a.value(5,0x96);
            a.call(0x10717b20);a.load<4>(0,1,0x58,false);a.load<4>(3,18,0x90,false);a.store<4>(1,0x40,0);
            a.load<4>(0,1,0x5c,false);a.load<2>(5,1,0x42,false);a.load<2>(4,3,0x4e,false);a.store<4>(1,0x44,0);
            a.compare_registers(5,4,false);
            if(a.nonpositive()) {
                a.at(0x1036b76c);
                if(a.at_least()) {
                    a.at(0x1036b778);a.load<2>(4,1,0x40,true);a.load<2>(0,3,0x4c,true);a.combine(0,4,0,false,true);
                    a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,0x1,false,1);
                    a.sign8(0,0,false);
                } else {
                    a.at(0x1036b770);a.value(0,0x0);a.jump(0x1036b798);
                }
            } else {
                a.at(0x1036b764);a.value(0,0x1);a.jump(0x1036b798);
            }
            a.at(0x1036b798);a.sign8(0,0,true);
            if(!a.equal()) {
                a.at(0x1036b7a0);a.load<1>(0,17,0x65,false);a.sign8(3,25,false);a.subtract_from(0,0,-0x32);
                a.sign8(0,0,false);a.compare_registers(3,0,true);
                if(a.at_least()) {
                    a.at(0x1036b7d4);a.sign8(0,25,true);
                    if(a.at_least()) {
                        a.at(0x1036b7f8);a.sign8(0,25,false);a.compare(0,0x32,true);
                        if(a.at_least()) {
                            a.at(0x1036b820);a.compare(0,0x4b,true);
                            if(a.at_least()) {
                                a.at(0x1036b844);a.sign8(0,26,false);a.value(3,0x1);a.compare(0,0x1,true);
                                if(!a.less()) {
                                    a.at(0x1036b854);a.move(3,0);
                                }
                                a.at(0x1036b858);a.sign8(26,3,false);
                            } else {
                                a.at(0x1036b828);a.sign8(0,26,false);a.value(3,0x2);a.compare(0,0x2,true);
                                if(!a.less()) {
                                    a.at(0x1036b838);a.move(3,0);
                                }
                                a.at(0x1036b83c);a.sign8(26,3,false);a.jump(0x1036b85c);
                            }
                        } else {
                            a.at(0x1036b804);a.sign8(0,26,false);a.value(3,0x3);a.compare(0,0x3,true);
                            if(!a.less()) {
                                a.at(0x1036b814);a.move(3,0);
                            }
                            a.at(0x1036b818);a.sign8(26,3,false);a.jump(0x1036b85c);
                        }
                    } else {
                        a.at(0x1036b7dc);a.sign8(0,26,false);a.value(3,0x4);a.compare(0,0x4,true);
                        if(!a.less()) {
                            a.at(0x1036b7ec);a.move(3,0);
                        }
                        a.at(0x1036b7f0);a.sign8(26,3,false);a.jump(0x1036b85c);
                    }
                } else {
                    a.at(0x1036b7b8);a.sign8(0,26,false);a.compare(0,0x5,true);
                    if(a.equal()) {
                        a.at(0x1036b7d4);a.sign8(0,25,true);
                        if(a.at_least()) {
                            a.at(0x1036b7f8);a.sign8(0,25,false);a.compare(0,0x32,true);
                            if(a.at_least()) {
                                a.at(0x1036b820);a.compare(0,0x4b,true);
                                if(a.at_least()) {
                                    a.at(0x1036b844);a.sign8(0,26,false);a.value(3,0x1);a.compare(0,0x1,true);
                                    if(!a.less()) {
                                        a.at(0x1036b854);a.move(3,0);
                                    }
                                    a.at(0x1036b858);a.sign8(26,3,false);
                                } else {
                                    a.at(0x1036b828);a.sign8(0,26,false);a.value(3,0x2);a.compare(0,0x2,true);
                                    if(!a.less()) {
                                        a.at(0x1036b838);a.move(3,0);
                                    }
                                    a.at(0x1036b83c);a.sign8(26,3,false);a.jump(0x1036b85c);
                                }
                            } else {
                                a.at(0x1036b804);a.sign8(0,26,false);a.value(3,0x3);a.compare(0,0x3,true);
                                if(!a.less()) {
                                    a.at(0x1036b814);a.move(3,0);
                                }
                                a.at(0x1036b818);a.sign8(26,3,false);a.jump(0x1036b85c);
                            }
                        } else {
                            a.at(0x1036b7dc);a.sign8(0,26,false);a.value(3,0x4);a.compare(0,0x4,true);
                            if(!a.less()) {
                                a.at(0x1036b7ec);a.move(3,0);
                            }
                            a.at(0x1036b7f0);a.sign8(26,3,false);a.jump(0x1036b85c);
                        }
                    } else {
                        a.at(0x1036b7c4);a.compare(0,0x6,true);
                        if(a.equal()) {
                            a.at(0x1036b7d4);a.sign8(0,25,true);
                            if(a.at_least()) {
                                a.at(0x1036b7f8);a.sign8(0,25,false);a.compare(0,0x32,true);
                                if(a.at_least()) {
                                    a.at(0x1036b820);a.compare(0,0x4b,true);
                                    if(a.at_least()) {
                                        a.at(0x1036b844);a.sign8(0,26,false);a.value(3,0x1);a.compare(0,0x1,true);
                                        if(!a.less()) {
                                            a.at(0x1036b854);a.move(3,0);
                                        }
                                        a.at(0x1036b858);a.sign8(26,3,false);
                                    } else {
                                        a.at(0x1036b828);a.sign8(0,26,false);a.value(3,0x2);a.compare(0,0x2,true);
                                        if(!a.less()) {
                                            a.at(0x1036b838);a.move(3,0);
                                        }
                                        a.at(0x1036b83c);a.sign8(26,3,false);a.jump(0x1036b85c);
                                    }
                                } else {
                                    a.at(0x1036b804);a.sign8(0,26,false);a.value(3,0x3);a.compare(0,0x3,true);
                                    if(!a.less()) {
                                        a.at(0x1036b814);a.move(3,0);
                                    }
                                    a.at(0x1036b818);a.sign8(26,3,false);a.jump(0x1036b85c);
                                }
                            } else {
                                a.at(0x1036b7dc);a.sign8(0,26,false);a.value(3,0x4);a.compare(0,0x4,true);
                                if(!a.less()) {
                                    a.at(0x1036b7ec);a.move(3,0);
                                }
                                a.at(0x1036b7f0);a.sign8(26,3,false);a.jump(0x1036b85c);
                            }
                        } else {
                            a.at(0x1036b7cc);a.value(26,0x7);a.jump(0x1036b85c);
                        }
                    }
                }
            }
        }
        a.at(0x1036b85c);a.sign8(0,26,false);a.compare(0,0x7,false);
        if(!a.positive()) {
            a.at(0x1036b868);a.load<4>(3,2,-0x4e10,false);a.mask(0,0,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
            a.count_destination(3);a.count_branch();
            switch(a.r(0)>>2) {
                case 0:case 2: {
                    break;
                }
                case 1: {
                    a.at(0x1036b87c);a.floating_load(0,16,0x170,false);a.floating_product(31,31,0);a.jump(0x1036b92c);
                    break;
                }
                case 3: {
                    a.at(0x1036b8f0);a.floating_load(0,16,0x168,false);a.floating_product(31,31,0);a.jump(0x1036b92c);
                    break;
                }
                case 4: {
                    a.at(0x1036b888);a.floating_load(0,16,0x2f8,false);a.compare(22,0x17,true);a.floating_product(31,31,0);
                    if(!a.nonpositive()) {
                        a.at(0x1036b898);a.compare(22,0x20,true);
                        if(!a.at_least()) {
                            a.at(0x1036b8a0);a.floating_load(0,16,0x68,false);a.floating_product(31,31,0);
                        }
                    }
                    a.at(0x1036b8a8);a.compare(22,0x20,true);
                    if(!a.at_least()) {
                        a.at(0x1036b8b0);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036b8bc);a.floating_load(0,16,0x68,false);a.floating_product(31,31,0);a.jump(0x1036b92c);
                        }
                    }
                    break;
                }
                case 5: {
                    a.at(0x1036b8c8);a.floating_load(0,16,0xa0,false);a.compare(22,0x20,true);a.floating_product(31,31,0);
                    if(!a.at_least()) {
                        a.at(0x1036b8d8);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036b8e4);a.floating_load(0,16,0x68,false);a.floating_product(31,31,0);a.jump(0x1036b92c);
                        }
                    }
                    break;
                }
                case 6: {
                    a.at(0x1036b8fc);a.floating_load(0,16,0x1f0,false);a.compare(22,0x20,true);a.floating_product(31,31,0);
                    if(!a.at_least()) {
                        a.at(0x1036b90c);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036b918);a.floating_load(0,16,0x18,false);a.floating_product(31,31,0);a.jump(0x1036b92c);
                        }
                    }
                    break;
                }
                case 7: {
                    a.at(0x1036b924);a.floating_load(0,16,0x148,false);a.floating_product(31,31,0);
                    break;
                }
            }
        }
    } else {
        a.at(0x10369f24);a.move(3,18);a.value(4,0x0);a.value(5,0x1);
        a.value(6,0x0);a.value(7,0x0);a.call(0x1014c2e0);a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x10369f44);a.load<4>(3,18,0x90,false);a.value(0,0x43300000);a.store<4>(1,0x10c,0);
            a.compare(21,0x0,false);a.load<1>(0,3,0x65,false);a.floating_load(2,16,0x8,false);a.sign8(0,0,false);
            a.floating_load(0,16,0x98,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(3,16,0x68,false);a.store<4>(1,0x110,0);
            a.floating_load(1,1,0x10c,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_sum(1,3,0);
            a.floating_product(31,31,1);
            if(!a.equal()) {
                a.at(0x10369f88);a.floating_load(0,16,0x3e0,false);a.floating_compare(1,0);
                if(!a.at_least()) {
                    a.at(0x10369f94);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20000000);a.store<4>(21,0x0,0);
                }
            }
        }
        a.at(0x10369fa0);a.sign8(0,26,false);a.compare(0,0x7,false);
        if(!a.positive()) {
            a.at(0x10369fac);a.load<4>(3,2,-0x4e08,false);a.mask(0,0,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
            a.count_destination(3);a.count_branch();
            switch(a.r(0)>>2) {
                case 0: {
                    break;
                }
                case 1: {
                    a.at(0x10369fc0);a.floating_load(0,16,0x1b8,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                    if(!a.equal()) {
                        a.at(0x10369fd0);a.load<4>(0,27,0x50,false);a.compare(0,0x0,true);
                        if(a.equal()) {
                            a.at(0x10369fdc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x1000);a.store<4>(21,0x0,0);
                            a.jump(0x1036a0ec);
                        }
                    }
                    break;
                }
                case 2: {
                    a.at(0x10369fec);a.floating_load(0,16,0xa0,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                    if(!a.equal()) {
                        a.at(0x10369ffc);a.load<4>(0,27,0x50,false);a.compare(0,0x0,true);
                        if(a.equal()) {
                            a.at(0x1036a008);a.load<4>(0,21,0x0,false);a.flags(0,0,0x1000);a.store<4>(21,0x0,0);
                            a.jump(0x1036a0ec);
                        }
                    }
                    break;
                }
                case 3: {
                    a.at(0x1036a09c);a.floating_load(0,16,0x168,false);a.floating_product(31,31,0);a.jump(0x1036a0ec);
                    break;
                }
                case 4: {
                    a.at(0x1036a018);a.floating_load(0,16,0xf0,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                    if(!a.equal()) {
                        a.at(0x1036a028);a.load<4>(0,21,0x0,false);a.flags(0,0,0x2000);a.store<4>(21,0x0,0);
                    }
                    a.at(0x1036a034);a.compare(22,0x17,true);
                    if(!a.nonpositive()) {
                        a.at(0x1036a03c);a.compare(22,0x20,true);
                        if(!a.at_least()) {
                            a.at(0x1036a044);a.floating_load(0,16,0xe8,false);a.floating_product(31,31,0);
                        }
                    }
                    a.at(0x1036a04c);a.compare(22,0x20,true);
                    if(!a.at_least()) {
                        a.at(0x1036a054);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036a060);a.floating_load(0,16,0xe8,false);a.floating_product(31,31,0);a.jump(0x1036a0ec);
                        }
                    }
                    break;
                }
                case 5: {
                    a.at(0x1036a06c);a.sign8(0,19,true);
                    if(a.equal()) {
                        a.at(0x1036a074);a.floating_load(0,16,0xa0,false);a.floating_product(31,31,0);
                    }
                    a.at(0x1036a07c);a.compare(22,0x20,true);
                    if(!a.at_least()) {
                        a.at(0x1036a084);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036a090);a.floating_load(0,16,0xe8,false);a.floating_product(31,31,0);a.jump(0x1036a0ec);
                        }
                    }
                    break;
                }
                case 6: {
                    a.at(0x1036a0a8);a.floating_load(0,16,0x200,false);a.compare(22,0x20,true);a.floating_product(31,31,0);
                    if(!a.at_least()) {
                        a.at(0x1036a0b8);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036a0c4);a.floating_load(0,16,0x3e8,false);a.floating_product(31,31,0);a.jump(0x1036a0ec);
                        }
                    }
                    break;
                }
                case 7: {
                    a.at(0x1036a0d0);a.floating_load(0,16,0x3f0,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                    if(!a.equal()) {
                        a.at(0x1036a0e0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x2000);a.store<4>(21,0x0,0);
                    }
                    break;
                }
            }
        }
        a.at(0x1036a0ec);a.sign8(15,20,false);a.compare(15,0x7,false);
        if(!a.positive()) {
            a.at(0x1036a0f8);a.load<4>(3,2,-0x4e0c,false);a.mask(0,15,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
            a.count_destination(3);a.count_branch();
            switch(a.r(0)>>2) {
                case 0: {
                    break;
                }
                case 1: {
                    a.at(0x1036a10c);a.compare(27,0x0,false);
                    if(a.equal()) {
                        a.at(0x1036a140);a.floating_load(0,16,0x1b8,false);a.floating_difference(31,31,0,true,false);
                    } else {
                        a.at(0x1036a114);a.load<4>(0,27,0x50,false);a.mask(0,0,0x10,true,0);
                        if(a.equal()) {
                            a.at(0x1036a140);a.floating_load(0,16,0x1b8,false);a.floating_difference(31,31,0,true,false);
                        } else {
                            a.at(0x1036a120);a.floating_load(0,16,0x68,false);a.compare(21,0x0,false);a.floating_difference(31,31,0,true,false);
                            if(!a.equal()) {
                                a.at(0x1036a130);a.load<4>(0,21,0x0,false);a.flags(0,0,0x4000);a.store<4>(21,0x0,0);
                                a.jump(0x1036a148);
                            }
                        }
                    }
                    a.at(0x1036a148);a.load<4>(3,2,-0x4de8,false);a.floating_load(0,3,0x14,true);a.floating_product_single(30,30,0);
                    a.jump(0x1036a37c);
                    break;
                }
                case 2: {
                    a.at(0x1036a158);a.compare(27,0x0,false);
                    if(a.equal()) {
                        a.at(0x1036a18c);a.floating_load(0,16,0xa0,false);a.floating_difference(31,31,0,true,false);
                    } else {
                        a.at(0x1036a160);a.load<4>(0,27,0x50,false);a.mask(0,0,0x10,true,0);
                        if(a.equal()) {
                            a.at(0x1036a18c);a.floating_load(0,16,0xa0,false);a.floating_difference(31,31,0,true,false);
                        } else {
                            a.at(0x1036a16c);a.floating_load(0,16,0x180,false);a.compare(21,0x0,false);a.floating_difference(31,31,0,true,false);
                            if(!a.equal()) {
                                a.at(0x1036a17c);a.load<4>(0,21,0x0,false);a.flags(0,0,0x4000);a.store<4>(21,0x0,0);
                                a.jump(0x1036a194);
                            }
                        }
                    }
                    a.at(0x1036a194);a.load<4>(3,2,-0x4de8,false);a.floating_load(0,3,0x18,true);a.floating_product_single(30,30,0);
                    a.jump(0x1036a37c);
                    break;
                }
                case 3: {
                    a.at(0x1036a1a4);a.compare(27,0x0,false);
                    if(a.equal()) {
                        a.at(0x1036a1b8);a.floating_load(0,16,0x168,false);a.floating_difference(31,31,0,true,false);
                    } else {
                        a.at(0x1036a1ac);a.load<4>(0,27,0x50,false);a.mask(0,0,0x10,true,0);
                        if(a.equal()) {
                            a.at(0x1036a1b8);a.floating_load(0,16,0x168,false);a.floating_difference(31,31,0,true,false);
                        }
                    }
                    a.at(0x1036a1c0);a.load<4>(3,17,0x14,false);a.load<1>(0,3,0xf,false);a.compare(0,0x14,true);
                    if(a.equal()) {
                        a.at(0x1036a1d0);a.floating_load(0,16,0xf8,false);a.floating_product(31,31,0);a.jump(0x1036a37c);
                    }
                    break;
                }
                case 4: {
                    a.at(0x1036a1dc);a.mask(0,23,0x10,true,0);
                    if(a.equal()) {
                        a.at(0x1036a21c);a.load<4>(3,17,0x14,false);a.load<1>(0,3,0xf,false);a.compare(0,0x14,true);
                        if(!a.equal()) {
                            a.at(0x1036a238);a.floating_load(0,16,0x400,false);a.floating_product(31,31,0);
                        } else {
                            a.at(0x1036a22c);a.floating_load(0,16,0x38,false);a.floating_product(31,31,0);a.jump(0x1036a240);
                        }
                    } else {
                        a.at(0x1036a1e4);a.floating_load(0,16,0x3f8,false);a.load<4>(0,17,0x1c,false);a.floating_product(31,31,0);
                        a.compare(0,0x0,false);
                        if(a.equal()) {
                            a.at(0x1036a204);a.compare(21,0x0,false);
                            if(!a.equal()) {
                                a.at(0x1036a20c);a.load<4>(0,21,0x0,false);a.flags(0,0,0x8000);a.store<4>(21,0x0,0);
                                a.jump(0x1036a240);
                            }
                        } else {
                            a.at(0x1036a1f8);a.floating_load(0,16,0x400,false);a.floating_difference(31,31,0,true,false);a.jump(0x1036a240);
                        }
                    }
                    a.at(0x1036a240);a.compare(22,0x17,true);
                    if(!a.nonpositive()) {
                        a.at(0x1036a248);a.compare(22,0x20,true);
                        if(!a.at_least()) {
                            a.at(0x1036a250);a.floating_load(0,16,0x68,false);a.floating_product(31,31,0);
                        }
                    }
                    a.at(0x1036a258);a.compare(22,0x20,true);
                    if(!a.at_least()) {
                        a.at(0x1036a260);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036a26c);a.floating_load(0,16,0x68,false);a.floating_product(31,31,0);
                        }
                    }
                    a.at(0x1036a274);a.load<4>(3,2,-0x4de8,false);a.floating_load(0,3,0x1c,true);a.floating_product_single(30,30,0);
                    a.jump(0x1036a37c);
                    break;
                }
                case 5: {
                    a.at(0x1036a284);a.mask(0,23,0x10,true,0);
                    if(a.equal()) {
                        a.at(0x1036a2ac);a.load<4>(3,17,0x14,false);a.load<1>(0,3,0xf,false);a.compare(0,0x14,true);
                        if(!a.equal()) {
                            a.at(0x1036a2c8);a.floating_load(0,16,0x348,false);a.floating_product(31,31,0);
                        } else {
                            a.at(0x1036a2bc);a.floating_load(0,16,0x58,false);a.floating_product(31,31,0);a.jump(0x1036a2d0);
                        }
                    } else {
                        a.at(0x1036a28c);a.floating_load(0,16,0x408,false);a.compare(21,0x0,false);a.floating_difference(31,31,0,true,false);
                        if(!a.equal()) {
                            a.at(0x1036a29c);a.load<4>(0,21,0x0,false);a.flags(0,0,0x8000);a.store<4>(21,0x0,0);
                            a.jump(0x1036a2d0);
                        }
                    }
                    a.at(0x1036a2d0);a.compare(22,0x20,true);
                    if(!a.at_least()) {
                        a.at(0x1036a2d8);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036a2e4);a.floating_load(0,16,0x68,false);a.floating_product(31,31,0);
                        }
                    }
                    a.at(0x1036a2ec);a.load<4>(3,2,-0x4de8,false);a.floating_load(0,3,0x20,true);a.floating_product_single(30,30,0);
                    a.jump(0x1036a37c);
                    break;
                }
                case 6: {
                    a.at(0x1036a2fc);a.mask(0,23,0x10,true,0);
                    if(a.equal()) {
                        a.at(0x1036a324);a.load<4>(3,17,0x14,false);a.load<1>(0,3,0xf,false);a.compare(0,0x14,true);
                        if(!a.equal()) {
                            a.at(0x1036a340);a.floating_load(0,16,0x38,false);a.floating_product(31,31,0);
                        } else {
                            a.at(0x1036a334);a.floating_load(0,16,0x48,false);a.floating_product(31,31,0);a.jump(0x1036a348);
                        }
                    } else {
                        a.at(0x1036a304);a.floating_load(0,16,0x3c0,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                        if(!a.equal()) {
                            a.at(0x1036a314);a.load<4>(0,21,0x0,false);a.flags(0,0,0x8000);a.store<4>(21,0x0,0);
                            a.jump(0x1036a348);
                        }
                    }
                    a.at(0x1036a348);a.compare(22,0x20,true);
                    if(!a.at_least()) {
                        a.at(0x1036a350);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036a35c);a.floating_load(0,16,0x18,false);a.floating_product(31,31,0);
                        }
                    }
                    a.at(0x1036a364);a.load<4>(3,2,-0x4de8,false);a.floating_load(0,3,0x4,true);a.floating_product_single(30,30,0);
                    a.jump(0x1036a37c);
                    break;
                }
                case 7: {
                    a.at(0x1036a374);a.floating_load(0,16,0x3b8,false);a.floating_product(31,31,0);
                    break;
                }
            }
        }
        return adjust_interest_contract_preferences(a);
    }
    return calculate_interest_financial_context(a);
}
}
