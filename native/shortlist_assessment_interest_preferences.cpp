#include "shortlist_assessment_detail.hpp"

namespace cm::aot::shortlist_assessment::detail {
void adjust_interest_contract_preferences(Call& a) {
    a.at(0x1036a37c);a.load<4>(3,18,0x2c,false);a.load<1>(0,3,0x11c,false);a.sign8(30,0,false);
    a.compare(30,0x1,true);
    if(a.less()) {
        a.at(0x1036a398);a.value(30,0x3);
    } else {
        a.at(0x1036a390);a.compare(30,0x3,true);
        if(!a.nonpositive()) {
            a.at(0x1036a398);a.value(30,0x3);
        }
    }
    a.at(0x1036a39c);a.product(0,30,30);a.load<4>(3,18,0x3c,false);a.compare(3,0x0,false);
    a.sign8(30,0,false);
    if(a.equal()) {
        a.at(0x1036a474);a.load<2>(3,18,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x10c,0);
        a.exclusive_flags(0,3,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0x110,0);a.floating_load(0,16,0x160,false);
        a.floating_load(1,1,0x10c,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);
        a.floating_store(0,1,0x114);a.load<4>(0,1,0x118,false);a.store<2>(1,0x134,0);
    } else {
        a.at(0x1036a3b0);a.load<4>(3,3,0x10,false);a.compare(3,0x0,false);
        if(a.equal()) {
            a.at(0x1036a474);a.load<2>(3,18,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x10c,0);
            a.exclusive_flags(0,3,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0x110,0);a.floating_load(0,16,0x160,false);
            a.floating_load(1,1,0x10c,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);
            a.floating_store(0,1,0x114);a.load<4>(0,1,0x118,false);a.store<2>(1,0x134,0);
        } else {
            a.at(0x1036a3bc);a.load<1>(3,3,0x37,false);a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x1036a3e4);a.load<2>(6,18,0x1ec,true);a.value(4,0x43300000);a.sign8(0,3,false);
                a.value(3,0x88890000);a.exclusive_flags(5,6,0x80000000);a.store<4>(1,0x10c,4);a.floating_load(2,16,0x8,false);
                a.product(0,0,6);a.store<4>(1,0x110,5);a.offset(3,3,-0x7777);a.floating_load(0,16,0x160,false);
                a.floating_load(1,1,0x10c,false);a.store<4>(1,0x114,4);a.floating_subtract(1,1,2,false);a.multiply_high(3,3,0);
                a.floating_difference(1,1,0,true,false);a.add(0,3,0);a.shift_right(0,0,3);a.mask(3,0,0x1,false,1);
                a.add(0,0,3);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x118,0);a.floating_load(0,1,0x114,false);
                a.floating_subtract(0,0,2,false);a.floating_compare(1,0);
                if(a.nonpositive()) {
                    a.at(0x1036a450);a.store<4>(1,0x120,0);a.store<4>(1,0x11c,4);a.floating_load(0,1,0x11c,false);
                    a.floating_subtract(1,0,2,false);
                } else {
                    a.at(0x1036a44c);a.jump(0x1036a460);
                }
                a.at(0x1036a460);a.floating_integer(0,1);a.floating_store(0,1,0x124);a.load<4>(0,1,0x128,false);
                a.store<2>(1,0x134,0);a.jump(0x1036a4ac);
            } else {
                a.at(0x1036a3c8);a.load<2>(3,18,0x1ec,true);a.mask(0,3,0x1,false,1);a.add(0,0,3);
                a.shift_right(0,0,1);a.sign16(0,0,false);a.store<2>(1,0x134,0);a.jump(0x1036a4ac);
            }
        }
    }
    a.at(0x1036a4ac);a.load<4>(5,18,0x30,false);a.compare(5,0x0,false);
    if(a.equal()) {
        a.at(0x1036a530);a.load<2>(0,18,0x1ec,true);a.store<2>(1,0x130,0);
    } else {
        a.at(0x1036a4b8);a.load<2>(6,18,0x1ec,true);a.compare(6,0x1d4c,true);
        if(a.less()) {
            a.at(0x1036a4f8);a.load<2>(4,5,0x18,true);a.mask(0,6,0x1,false,1);a.add(3,0,6);
            a.multiply(0,4,0xfa);a.shift_right(3,3,1);a.multiply(4,4,0x1f4);a.add(0,3,0);
            a.compare_registers(4,0,true);
            if(a.nonpositive()) {
                a.at(0x1036a520);a.move(4,0);
            } else {
                a.at(0x1036a51c);a.jump(0x1036a524);
            }
            a.at(0x1036a524);a.sign16(0,4,false);a.store<2>(1,0x130,0);a.jump(0x1036a538);
        } else {
            a.at(0x1036a4c4);a.value(3,0x10000);a.load<4>(4,18,0x8,false);a.offset(0,3,-0x7748);
            a.compare_registers(4,0,true);
            if(a.nonpositive()) {
                a.at(0x1036a4f8);a.load<2>(4,5,0x18,true);a.mask(0,6,0x1,false,1);a.add(3,0,6);
                a.multiply(0,4,0xfa);a.shift_right(3,3,1);a.multiply(4,4,0x1f4);a.add(0,3,0);
                a.compare_registers(4,0,true);
                if(a.nonpositive()) {
                    a.at(0x1036a520);a.move(4,0);
                } else {
                    a.at(0x1036a51c);a.jump(0x1036a524);
                }
                a.at(0x1036a524);a.sign16(0,4,false);a.store<2>(1,0x130,0);a.jump(0x1036a538);
            } else {
                a.at(0x1036a4d8);a.load<2>(0,5,0x18,true);a.multiply(0,0,0x1f4);a.compare_registers(0,6,true);
                if(!a.nonpositive()) {
                    a.at(0x1036a4e8);a.move(6,0);
                }
                a.at(0x1036a4ec);a.sign16(0,6,false);a.store<2>(1,0x130,0);a.jump(0x1036a538);
            }
        }
    }
    a.at(0x1036a538);a.load<4>(4,18,0x14,false);a.compare(4,-0x1,true);
    if(a.nonpositive()) {
        a.at(0x1036a574);a.load<4>(3,2,-0x7984,false);a.load<4>(0,18,0x0,false);a.load<4>(3,3,0x0,false);
        a.mask(0,0,0xfffffff8,false,3);a.indexed_load<4>(0,3,0);a.compare(0,-0x1,true);
        if(!a.nonpositive()) {
            a.at(0x1036a590);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,0,0x6c);a.load<4>(3,3,0x0,false);
            a.add(14,3,0);
        }
    } else {
        a.at(0x1036a544);a.load<4>(3,2,-0x7a2c,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
        if(!a.equal()) {
            a.at(0x1036a560);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,4,0x6c);a.load<4>(3,3,0x0,false);
            a.add(14,3,0);a.jump(0x1036a5a0);
        } else {
            a.at(0x1036a554);a.load<2>(0,18,0x1ec,true);a.compare(0,0x1388,true);
            if(a.at_least()) {
                a.at(0x1036a574);a.load<4>(3,2,-0x7984,false);a.load<4>(0,18,0x0,false);a.load<4>(3,3,0x0,false);
                a.mask(0,0,0xfffffff8,false,3);a.indexed_load<4>(0,3,0);a.compare(0,-0x1,true);
                if(!a.nonpositive()) {
                    a.at(0x1036a590);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,0,0x6c);a.load<4>(3,3,0x0,false);
                    a.add(14,3,0);
                }
            } else {
                a.at(0x1036a560);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,4,0x6c);a.load<4>(3,3,0x0,false);
                a.add(14,3,0);a.jump(0x1036a5a0);
            }
        }
    }
    a.at(0x1036a5a0);a.compare(14,0x0,false);
    if(a.equal()) {
        a.at(0x1036a66c);a.load<2>(3,1,0x130,true);a.value(0,0x43300000);a.store<4>(1,0x10c,0);
        a.exclusive_flags(3,3,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0x110,3);a.floating_load(3,16,0xf8,false);
        a.floating_load(1,1,0x10c,false);a.floating_load(0,16,0x320,false);a.floating_subtract(1,1,2,false);a.floating_product(1,3,1);
        a.floating_compare(1,0);
        if(a.at_least()) {
            a.at(0x1036a6a4);a.floating_move(1,0);
        } else {
            a.at(0x1036a6a0);a.jump(0x1036a6a8);
        }
        a.at(0x1036a6a8);a.floating_integer(0,1);a.floating_store(0,1,0x114);a.load<4>(13,1,0x118,false);
    } else {
        a.at(0x1036a5a8);a.load<4>(3,2,-0x7a30,false);a.load<4>(4,14,0x0,false);a.load<4>(0,3,0x0,false);
        a.compare_registers(4,0,true);
        if(!a.equal()) {
            a.at(0x1036a5c4);a.load<2>(0,14,0x18,true);a.multiply(0,0,0x258);a.sign16(13,0,false);
        } else {
            a.at(0x1036a5bc);a.value(13,0x3a98);a.jump(0x1036a5d0);
        }
        a.at(0x1036a5d0);a.load<4>(3,2,-0x7b5c,false);a.mask(0,4,0xfffffffc,false,2);a.move(4,18);
        a.load<4>(3,3,0x0,false);a.indexed_load<4>(3,3,0);a.load<4>(3,3,0x8,false);a.call(0x10459b50);
        a.compare(3,0x0,false);
        if(!a.equal()) {
            a.at(0x1036a5f4);a.load<1>(0,3,0x4,false);a.compare(0,0x0,false);
            if(a.equal()) {
                a.at(0x1036a600);a.load<1>(4,3,0x75,false);a.compare(4,0x1,false);
                if(!a.less()) {
                    a.at(0x1036a60c);a.compare(4,0x10,false);
                    if(!a.positive()) {
                        a.at(0x1036a614);a.value(0,0x43300000);a.sign16(3,13,false);a.store<4>(1,0x118,4);
                        a.exclusive_flags(3,3,0x80000000);a.floating_load(2,16,0x2b8,false);a.store<4>(1,0x114,0);a.floating_load(0,16,0xb8,false);
                        a.floating_load(1,1,0x114,false);a.store<4>(1,0x110,3);a.floating_subtract(1,1,2,false);a.floating_load(3,16,0x8,false);
                        a.store<4>(1,0x10c,0);a.floating_load(2,16,0x410,false);a.floating_difference(0,1,0,true,false);a.floating_load(1,1,0x10c,false);
                        a.floating_subtract(1,1,3,false);a.floating_subtract(0,2,0,false);a.floating_product(0,1,0);a.floating_integer(0,0);
                        a.floating_store(0,1,0x11c);a.load<4>(13,1,0x120,false);a.jump(0x1036a6b4);
                    }
                }
            }
        }
    }
    a.at(0x1036a6b4);a.load<4>(3,18,0x90,false);a.compare(3,0x0,false);
    if(!a.equal()) {
        a.at(0x1036a6c8);a.load<4>(0,3,0x1c,false);a.compare(0,0x0,false);
        if(!a.equal()) {
            a.at(0x1036a6dc);a.move(4,17);a.value(5,0x1);a.call(0x100cdef0);
            a.load<4>(4,2,-0x79f0,false);a.move(31,3);a.load<4>(4,4,0x0,false);a.compare(4,0x0,false);
            if(!a.equal()) {
                a.at(0x1036a6fc);a.compare(18,0x0,false);
                if(a.equal()) {
                    a.at(0x1036a71c);a.value(5,0x0);
                } else {
                    a.at(0x1036a704);a.load<4>(0,18,0x0,false);a.load<4>(3,4,0x0,false);a.multiply(0,0,0x4c);
                    a.add(3,3,0);a.load<2>(5,3,0xa,true);a.jump(0x1036a720);
                }
                a.at(0x1036a720);a.compare(18,0x0,false);
                if(a.equal()) {
                    a.at(0x1036a740);a.value(0,0x0);
                } else {
                    a.at(0x1036a728);a.load<4>(0,18,0x0,false);a.load<4>(3,4,0x0,false);a.multiply(0,0,0x4c);
                    a.add(3,3,0);a.load<2>(0,3,0x6,true);a.jump(0x1036a744);
                }
                a.at(0x1036a744);a.sign16(5,5,false);a.sign16(0,0,false);a.value(3,0x43300000);
                a.floating_load(1,16,0x8,false);a.exclusive_flags(5,5,0x80000000);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x110,5);
                a.compare(18,0x0,false);a.floating_load(4,16,0x18,false);a.store<4>(1,0x10c,3);a.floating_load(2,16,0x68,false);
                a.floating_load(0,1,0x10c,false);a.store<4>(1,0x118,0);a.floating_subtract(3,0,1,false);a.store<4>(1,0x114,3);
                a.floating_load(0,1,0x114,false);a.floating_subtract(1,0,1,false);
                if(a.equal()) {
                    a.at(0x1036a7a4);a.value(0,0x0);
                } else {
                    a.at(0x1036a78c);a.load<4>(0,18,0x0,false);a.load<4>(3,4,0x0,false);a.multiply(0,0,0x4c);
                    a.add(3,3,0);a.load<2>(0,3,0x8,true);a.jump(0x1036a7a8);
                }
                a.at(0x1036a7a8);a.sign16(0,0,false);a.value(4,0x43300000);a.exclusive_flags(0,0,0x80000000);
                a.store<4>(1,0x11c,4);a.floating_load(5,16,0x8,false);a.store<4>(1,0x120,0);a.floating_load(0,1,0x11c,false);
                a.floating_subtract(0,0,5,false);a.floating_product(0,4,0);a.floating_product_sum(0,2,1,0,false);a.floating_product_sum(0,4,3,0,false);
                a.floating_integer(0,0);a.floating_store(0,1,0x124);a.load<4>(0,1,0x128,false);a.sign16(0,0,false);
                a.compare(0,0x1388,true);
                if(a.at_least()) {
                    a.at(0x1036a860);a.exclusive_flags(3,0,0x80000000);a.sign16(0,31,false);a.store<4>(1,0x110,3);
                    a.exclusive_flags(0,0,0x80000000);a.floating_load(4,16,0x338,false);a.store<4>(1,0x10c,4);a.floating_load(2,16,0x68,false);
                    a.floating_load(0,1,0x10c,false);a.store<4>(1,0x118,0);a.floating_subtract(3,0,5,false);a.floating_load(1,16,0x330,false);
                    a.store<4>(1,0x114,4);a.floating_load(0,1,0x114,false);a.floating_product_sum(3,4,3,2,false);a.store<4>(1,0x120,0);
                    a.floating_subtract(2,0,5,false);a.store<4>(1,0x11c,4);a.floating_load(0,1,0x11c,false);a.floating_product(2,2,3);
                    a.floating_subtract(0,0,5,false);a.floating_product_sum(0,1,3,0,false);a.floating_compare(2,0);
                    if(a.at_least()) {
                        a.at(0x1036a8c0);a.floating_move(2,0);
                    } else {
                        a.at(0x1036a8bc);a.jump(0x1036a8c4);
                    }
                    a.at(0x1036a8c4);a.floating_integer(0,2);a.floating_store(0,1,0x124);a.load<4>(31,1,0x128,false);
                } else {
                    a.at(0x1036a7ec);a.exclusive_flags(3,0,0x80000000);a.sign16(0,31,false);a.store<4>(1,0x110,3);
                    a.exclusive_flags(0,0,0x80000000);a.floating_load(4,16,0x328,false);a.store<4>(1,0x10c,4);a.floating_load(2,16,0x208,false);
                    a.floating_load(0,1,0x10c,false);a.store<4>(1,0x118,0);a.floating_subtract(3,0,5,false);a.floating_load(1,16,0x330,false);
                    a.store<4>(1,0x114,4);a.floating_load(0,1,0x114,false);a.floating_negative_product_difference(3,4,3,2);a.store<4>(1,0x120,0);
                    a.floating_subtract(0,0,5,false);a.store<4>(1,0x11c,4);a.floating_difference(2,0,3,true,false);a.floating_load(0,1,0x11c,false);
                    a.floating_subtract(0,0,5,false);a.floating_negative_product_difference(0,1,3,0);a.floating_compare(2,0);
                    if(a.nonpositive()) {
                        a.at(0x1036a84c);a.floating_move(2,0);
                    } else {
                        a.at(0x1036a848);a.jump(0x1036a850);
                    }
                    a.at(0x1036a850);a.floating_integer(0,2);a.floating_store(0,1,0x124);a.load<4>(31,1,0x128,false);
                    a.jump(0x1036a8d0);
                }
            }
        } else {
            a.at(0x1036a6d4);a.value(31,0x0);a.jump(0x1036a8d0);
        }
    } else {
        a.at(0x1036a6c0);a.value(31,0x0);a.jump(0x1036a8d0);
    }
    a.at(0x1036a8d0);a.load<4>(3,2,-0x7ab8,false);a.move(4,18);a.load<2>(29,18,0x1ec,true);
    a.load<4>(3,3,0x0,false);a.call(0x10103510);a.compare(3,0x0,false);
    if(!a.equal()) {
        a.at(0x1036a8ec);a.compare(30,0x1,true);
        if(a.equal()) {
            a.at(0x1036a8f4);a.call(0x100f55b0);a.sign8(0,3,false);a.compare(0,0x2,true);
            if(a.equal()) {
                a.at(0x1036a914);a.value(3,0x43300000);a.exclusive_flags(0,29,0x80000000);a.store<4>(1,0x110,0);
                a.offset(0,29,0x5dc);a.exclusive_flags(0,0,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0x10c,3);
                a.floating_load(3,16,0x178,false);a.floating_load(0,1,0x10c,false);a.store<4>(1,0x118,0);a.floating_subtract(1,0,2,false);
                a.store<4>(1,0x114,3);a.floating_load(0,1,0x114,false);a.floating_product(1,3,1);a.floating_subtract(0,0,2,false);
                a.floating_compare(1,0);
                if(a.nonpositive()) {
                    a.at(0x1036a95c);a.store<4>(1,0x120,0);a.store<4>(1,0x11c,3);a.floating_load(0,1,0x11c,false);
                    a.floating_subtract(1,0,2,false);
                } else {
                    a.at(0x1036a958);a.jump(0x1036a96c);
                }
                a.at(0x1036a96c);a.floating_integer(0,1);a.floating_store(0,1,0x124);a.load<4>(29,1,0x128,false);
                a.jump(0x1036a9e0);
            } else {
                a.at(0x1036a904);
                if(!a.at_least()) {
                    a.at(0x1036a908);a.compare(0,0x1,true);
                    if(a.at_least()) {
                        a.at(0x1036a97c);a.value(3,0x43300000);a.exclusive_flags(0,29,0x80000000);a.store<4>(1,0x110,0);
                        a.offset(0,29,0x2ee);a.exclusive_flags(0,0,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0x10c,3);
                        a.floating_load(3,16,0xc8,false);a.floating_load(0,1,0x10c,false);a.store<4>(1,0x118,0);a.floating_subtract(1,0,2,false);
                        a.store<4>(1,0x114,3);a.floating_load(0,1,0x114,false);a.floating_product(1,3,1);a.floating_subtract(0,0,2,false);
                        a.floating_compare(1,0);
                        if(a.nonpositive()) {
                            a.at(0x1036a9c4);a.store<4>(1,0x120,0);a.store<4>(1,0x11c,3);a.floating_load(0,1,0x11c,false);
                            a.floating_subtract(1,0,2,false);
                        } else {
                            a.at(0x1036a9c0);a.jump(0x1036a9d4);
                        }
                        a.at(0x1036a9d4);a.floating_integer(0,1);a.floating_store(0,1,0x124);a.load<4>(29,1,0x128,false);
                    } else {
                        a.at(0x1036a910);a.jump(0x1036a9e0);
                    }
                }
            }
        }
    }
    a.at(0x1036a9e0);a.load<4>(0,18,0x0,false);a.sign16(4,29,false);a.load<4>(3,1,0x1e8,false);
    a.multiply(0,0,0xc);a.load<4>(3,3,0xdc,false);a.add(3,3,0);a.load<2>(0,3,0x6,true);
    a.compare_registers(4,0,true);
    if(!a.nonpositive()) {
        a.at(0x1036aa04);a.move(0,29);
    }
    a.at(0x1036aa08);a.sign16(0,0,false);a.value(4,0x43300000);a.exclusive_flags(0,0,0x80000000);
    a.store<4>(1,0x114,4);a.sign16(3,29,false);a.floating_load(6,16,0x8,false);a.store<4>(1,0x118,0);
    a.exclusive_flags(0,3,0x80000000);a.load<4>(3,2,-0x4de8,false);a.floating_load(1,1,0x114,false);a.floating_load(2,3,0xc,true);
    a.store<4>(1,0x110,0);a.floating_subtract(3,1,6,false);a.floating_load(4,16,0x418,false);a.floating_sum_single(0,2,28);
    a.store<4>(1,0x10c,4);a.floating_product(3,4,3);a.floating_load(7,16,0x1b8,false);a.floating_load(5,1,0x10c,false);
    a.floating_sum_single(1,29,0);a.floating_load(0,16,0x158,false);a.floating_subtract(4,5,6,false);a.floating_sum_single(1,30,1);
    a.floating_product_sum(3,7,4,3,false);a.floating_sum_single(26,2,1);a.floating_integer(1,3);a.floating_compare(26,0);
    a.floating_store(1,1,0x11c);a.load<4>(3,1,0x120,false);a.compare_less_or_equal_bit();
    if(a.equal()) {
        a.at(0x1036aa84);a.load<4>(4,2,-0x4de8,false);a.floating_load(26,4,0x4,true);
    }
    a.at(0x1036aa8c);a.load<2>(4,1,0x134,true);a.value(0,0x43300000);a.sign16(5,31,false);
    a.sign16(7,13,false);a.exclusive_flags(4,4,0x80000000);a.store<4>(1,0xec,0);a.exclusive_flags(5,5,0x80000000);
    a.exclusive_flags(7,7,0x80000000);a.store<4>(1,0xf0,4);a.sign16(3,3,false);a.floating_load(3,16,0x8,false);
    a.floating_load(0,1,0xec,false);a.load<2>(4,1,0x130,true);a.store<4>(1,0xf8,5);a.floating_subtract(0,0,3,true);
    a.exclusive_flags(6,4,0x80000000);a.load<4>(4,2,-0x4de8,false);a.store<4>(1,0xf4,0);a.exclusive_flags(5,3,0x80000000);
    a.floating_product_single(0,28,0);a.floating_load(1,1,0xf4,false);a.store<4>(1,0x100,6);a.floating_subtract(1,1,3,true);
    a.floating_load(4,4,0xc,true);a.store<4>(1,0xfc,0);a.load<1>(3,18,0x23d,false);a.floating_load(2,1,0xfc,false);
    a.floating_product_sum(0,4,1,0,true);a.store<4>(1,0x108,7);a.compare(3,0x1,true);a.floating_subtract(1,2,3,true);
    a.store<4>(1,0x104,0);a.floating_load(2,1,0x104,false);a.floating_product_sum(0,29,1,0,true);a.store<4>(1,0x128,5);
    a.floating_subtract(1,2,3,true);a.store<4>(1,0x124,0);a.floating_load(2,1,0x124,false);a.floating_product_sum(0,30,1,0,true);
    a.floating_subtract(1,2,3,true);a.floating_product_sum(0,4,1,0,true);a.floating_difference(0,0,26,true,true);a.floating_integer(0,0);
    a.floating_store(0,1,0xe4);a.load<4>(31,1,0xe8,false);
    if(!a.equal()) {
        a.at(0x1036ab44);a.compare(27,0x0,false);
        if(a.equal()) {
            a.at(0x1036ab64);a.sign16(4,31,false);a.mask(0,4,0x1,false,1);a.add(0,0,4);
            a.offset(4,4,-0x7d0);a.shift_right(0,0,1);a.compare_registers(0,4,true);
            if(a.at_least()) {
                a.at(0x1036ab84);a.move(0,4);
            } else {
                a.at(0x1036ab80);a.jump(0x1036ab88);
            }
            a.at(0x1036ab88);a.sign16(31,0,false);
        } else {
            a.at(0x1036ab4c);a.load<4>(0,27,0x50,false);a.mask(0,0,0x10,true,0);
            if(a.equal()) {
                a.at(0x1036ab64);a.sign16(4,31,false);a.mask(0,4,0x1,false,1);a.add(0,0,4);
                a.offset(4,4,-0x7d0);a.shift_right(0,0,1);a.compare_registers(0,4,true);
                if(a.at_least()) {
                    a.at(0x1036ab84);a.move(0,4);
                } else {
                    a.at(0x1036ab80);a.jump(0x1036ab88);
                }
                a.at(0x1036ab88);a.sign16(31,0,false);
            } else {
                a.at(0x1036ab58);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                if(!a.equal()) {
                    a.at(0x1036ab64);a.sign16(4,31,false);a.mask(0,4,0x1,false,1);a.add(0,0,4);
                    a.offset(4,4,-0x7d0);a.shift_right(0,0,1);a.compare_registers(0,4,true);
                    if(a.at_least()) {
                        a.at(0x1036ab84);a.move(0,4);
                    } else {
                        a.at(0x1036ab80);a.jump(0x1036ab88);
                    }
                    a.at(0x1036ab88);a.sign16(31,0,false);
                }
            }
        }
        a.at(0x1036ab8c);a.sign8(0,3,false);a.compare(0,0x3,true);
        if(a.equal()) {
            a.at(0x1036ab98);a.sign16(3,31,false);a.mask(0,3,0x1,false,1);a.add(0,0,3);
            a.offset(3,3,-0x7d0);a.shift_right(0,0,1);a.compare_registers(0,3,true);
            if(a.at_least()) {
                a.at(0x1036abb8);a.move(0,3);
            } else {
                a.at(0x1036abb4);a.jump(0x1036abbc);
            }
            a.at(0x1036abbc);a.sign16(31,0,false);
        }
    }
    a.at(0x1036abc0);a.sign16(0,31,false);a.value(4,0x43300000);a.exclusive_flags(0,0,0x80000000);
    a.load<4>(6,18,0x2c,false);a.store<4>(1,0xe8,0);a.floating_load(3,16,0x8,false);a.compare(6,0x0,false);
    a.store<4>(1,0xe4,4);a.floating_load(2,16,0x178,false);a.floating_load(1,1,0xe4,false);a.floating_load(0,16,0x350,false);
    a.floating_subtract(1,1,3,false);a.floating_product_difference(0,2,1,0);a.floating_integer(0,0);a.floating_store(0,1,0xec);
    a.load<4>(31,1,0xf0,false);
    if(!a.equal()) {
        a.at(0x1036ac04);a.sign16(3,31,false);a.store<4>(1,0xe4,4);a.offset(0,3,0x1388);
        a.load<4>(3,2,-0x7b3c,false);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xec,4);a.floating_load(1,16,0x358,false);
        a.move(4,17);a.store<4>(1,0xe8,0);a.move(5,18);a.load<4>(3,3,0x0,false);
        a.value(7,0x1);a.floating_load(0,1,0xe4,false);a.store<4>(1,0xf0,0);a.floating_subtract(2,0,3,false);
        a.load<1>(6,6,0x119,false);a.floating_load(0,1,0xec,false);a.floating_difference(1,2,1,true,false);a.floating_subtract(0,0,3,false);
        a.floating_product(25,1,0);a.call(0x103092f0);a.sign8(3,3,false);a.value(0,0x43300000);
        a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xf4,0);a.floating_load(1,16,0x8,false);a.compare(22,0x12,true);
        a.store<4>(1,0xf8,3);a.floating_load(0,1,0xf4,false);a.floating_subtract(3,0,1,false);
        if(!a.at_least()) {
            a.at(0x1036ac80);a.floating_load(0,16,0x208,false);a.floating_compare(3,0);
            if(!a.at_least()) {
                a.at(0x1036ac8c);a.load<4>(4,17,0xc,false);a.load<4>(3,18,0x2c,false);a.load<4>(4,4,0x74,false);
                a.load<4>(0,3,0x74,false);a.compare_registers(4,0,false);
                if(!a.equal()) {
                    a.at(0x1036aca4);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                    return;
                }
            }
        }
        a.at(0x1036acac);a.floating_load(1,16,0x40,false);a.floating_load(0,16,0x360,false);a.floating_load(2,16,0x150,false);
        a.floating_negative_product_difference(0,1,3,0);a.floating_difference(1,3,2,true,false);a.floating_difference(0,25,0,true,false);a.floating_compare(1,0);
        if(a.nonpositive()) {
            a.at(0x1036acd0);a.floating_move(1,0);
        } else {
            a.at(0x1036accc);a.jump(0x1036acd4);
        }
        a.at(0x1036acd4);a.floating_load(0,16,0x58,false);a.compare(21,0x0,false);a.floating_sum(1,0,1);
        a.floating_product(31,31,1);
        if(!a.equal()) {
            a.at(0x1036ace8);a.floating_load(0,16,0x180,false);a.floating_compare(1,0);
            if(!a.at_least()) {
                a.at(0x1036acf4);a.load<4>(0,21,0x0,false);a.flags(0,0,0x10000);a.store<4>(21,0x0,0);
            }
        }
    }
    a.at(0x1036ad00);a.load<4>(3,17,0x24,false);a.compare(3,0x0,false);
    if(a.equal()) {
        a.at(0x1036af0c);a.load<4>(4,18,0x2c,false);a.compare(4,0x0,false);
        if(a.equal()) {
            a.at(0x1036b024);a.load<4>(3,18,0x90,false);a.compare(3,0x0,false);
            if(a.equal()) {
                return check_additional_country_scale(a);
            } else {
                a.at(0x1036b030);a.load<4>(4,3,0xc,false);a.compare(4,0x0,false);
                if(a.equal()) {
                    return check_additional_country_scale(a);
                } else {
                    a.at(0x1036b03c);a.move(3,17);a.value(5,0x1);a.call(0x100cab10);
                    a.sign8(0,3,true);
                    if(!a.equal()) {
                        return check_additional_country_scale(a);
                    } else {
                        a.at(0x1036b050);a.load<4>(3,18,0x2c,false);a.compare(3,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036b05c);a.load<4>(4,3,0x68,false);a.load<4>(3,2,-0x7af4,false);a.load<4>(0,3,0x0,false);
                            a.compare_registers(4,0,true);
                            if(a.equal()) {
                                a.at(0x1036b070);a.floating_load(0,16,0xa0,false);a.floating_product(31,31,0);
                            }
                        }
                        a.at(0x1036b078);a.sign8(0,19,true);
                        if(a.equal()) {
                            a.at(0x1036b0a0);a.compare(15,0x4,true);
                            if(a.equal()) {
                                a.at(0x1036b0cc);a.floating_load(0,16,0x68,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                if(!a.equal()) {
                                    a.at(0x1036b0dc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                    a.jump(0x1036b3e0);
                                }
                            } else {
                                a.at(0x1036b0a8);
                                if(a.at_least()) {
                                    a.at(0x1036b0bc);a.compare(15,0x6,true);
                                    if(a.equal()) {
                                        a.at(0x1036b1d4);a.floating_load(0,16,0x58,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                        if(!a.equal()) {
                                            a.at(0x1036b1e4);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                            a.jump(0x1036b3e0);
                                        }
                                    } else {
                                        a.at(0x1036b0c4);
                                        if(!a.at_least()) {
                                            a.at(0x1036b0c8);a.jump(0x1036b0ec);a.at(0x1036b0ec);a.floating_load(0,16,0x348,false);
                                            a.compare(21,0x0,false);a.floating_product(31,31,0);
                                            if(!a.equal()) {
                                                a.at(0x1036b0fc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                                a.jump(0x1036b3e0);
                                            }
                                        }
                                    }
                                } else {
                                    a.at(0x1036b0ac);a.compare(15,0x2,true);
                                    if(a.equal()) {
                                        a.at(0x1036b10c);a.load<1>(4,17,0x63,false);a.value(3,0x43300000);a.load<1>(0,17,0x61,false);
                                        a.sign8(4,4,false);a.store<4>(1,0xe4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);
                                        a.store<4>(1,0xe8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(4,16,0x8,false);a.floating_load(0,1,0xe4,false);
                                        a.store<4>(1,0xf0,0);a.floating_subtract(3,0,4,false);a.floating_load(2,16,0xb8,false);a.store<4>(1,0xec,3);
                                        a.floating_load(0,16,0x40,false);a.floating_load(1,1,0xec,false);a.floating_difference(2,3,2,true,false);a.floating_load(3,16,0xf8,false);
                                        a.floating_subtract(1,1,4,false);a.floating_sum(2,3,2);a.floating_difference(0,1,0,true,false);a.floating_sum(0,2,0);
                                        a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                    } else {
                                        a.at(0x1036b0b4);
                                        if(a.at_least()) {
                                            a.at(0x1036b170);a.load<1>(4,17,0x63,false);a.value(3,0x43300000);a.load<1>(0,17,0x61,false);
                                            a.sign8(4,4,false);a.store<4>(1,0xe4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);
                                            a.store<4>(1,0xe8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(4,16,0x8,false);a.floating_load(0,1,0xe4,false);
                                            a.store<4>(1,0xf0,0);a.floating_subtract(3,0,4,false);a.floating_load(2,16,0xb8,false);a.store<4>(1,0xec,3);
                                            a.floating_load(0,16,0x40,false);a.floating_load(1,1,0xec,false);a.floating_difference(2,3,2,true,false);a.floating_load(3,16,0x1f0,false);
                                            a.floating_subtract(1,1,4,false);a.floating_sum(2,3,2);a.floating_difference(0,1,0,true,false);a.floating_sum(0,2,0);
                                            a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                        } else {
                                            a.at(0x1036b0b8);a.jump(0x1036b3e0);
                                        }
                                    }
                                }
                            }
                        } else {
                            a.at(0x1036b080);a.floating_load(0,16,0x310,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                            if(!a.equal()) {
                                a.at(0x1036b090);a.load<4>(0,21,0x0,false);a.flags(0,0,0x40000);a.store<4>(21,0x0,0);
                                a.jump(0x1036b3e0);
                            }
                        }
                    }
                }
            }
        } else {
            a.at(0x1036af18);a.move(3,17);a.value(5,0x1);a.call(0x100cab10);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x1036b024);a.load<4>(3,18,0x90,false);a.compare(3,0x0,false);
                if(a.equal()) {
                    return check_additional_country_scale(a);
                } else {
                    a.at(0x1036b030);a.load<4>(4,3,0xc,false);a.compare(4,0x0,false);
                    if(a.equal()) {
                        return check_additional_country_scale(a);
                    } else {
                        a.at(0x1036b03c);a.move(3,17);a.value(5,0x1);a.call(0x100cab10);
                        a.sign8(0,3,true);
                        if(!a.equal()) {
                            return check_additional_country_scale(a);
                        } else {
                            a.at(0x1036b050);a.load<4>(3,18,0x2c,false);a.compare(3,0x0,false);
                            if(!a.equal()) {
                                a.at(0x1036b05c);a.load<4>(4,3,0x68,false);a.load<4>(3,2,-0x7af4,false);a.load<4>(0,3,0x0,false);
                                a.compare_registers(4,0,true);
                                if(a.equal()) {
                                    a.at(0x1036b070);a.floating_load(0,16,0xa0,false);a.floating_product(31,31,0);
                                }
                            }
                            a.at(0x1036b078);a.sign8(0,19,true);
                            if(a.equal()) {
                                a.at(0x1036b0a0);a.compare(15,0x4,true);
                                if(a.equal()) {
                                    a.at(0x1036b0cc);a.floating_load(0,16,0x68,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                    if(!a.equal()) {
                                        a.at(0x1036b0dc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                        a.jump(0x1036b3e0);
                                    }
                                } else {
                                    a.at(0x1036b0a8);
                                    if(a.at_least()) {
                                        a.at(0x1036b0bc);a.compare(15,0x6,true);
                                        if(a.equal()) {
                                            a.at(0x1036b1d4);a.floating_load(0,16,0x58,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                            if(!a.equal()) {
                                                a.at(0x1036b1e4);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                                a.jump(0x1036b3e0);
                                            }
                                        } else {
                                            a.at(0x1036b0c4);
                                            if(!a.at_least()) {
                                                a.at(0x1036b0c8);a.jump(0x1036b0ec);a.at(0x1036b0ec);a.floating_load(0,16,0x348,false);
                                                a.compare(21,0x0,false);a.floating_product(31,31,0);
                                                if(!a.equal()) {
                                                    a.at(0x1036b0fc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                                    a.jump(0x1036b3e0);
                                                }
                                            }
                                        }
                                    } else {
                                        a.at(0x1036b0ac);a.compare(15,0x2,true);
                                        if(a.equal()) {
                                            a.at(0x1036b10c);a.load<1>(4,17,0x63,false);a.value(3,0x43300000);a.load<1>(0,17,0x61,false);
                                            a.sign8(4,4,false);a.store<4>(1,0xe4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);
                                            a.store<4>(1,0xe8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(4,16,0x8,false);a.floating_load(0,1,0xe4,false);
                                            a.store<4>(1,0xf0,0);a.floating_subtract(3,0,4,false);a.floating_load(2,16,0xb8,false);a.store<4>(1,0xec,3);
                                            a.floating_load(0,16,0x40,false);a.floating_load(1,1,0xec,false);a.floating_difference(2,3,2,true,false);a.floating_load(3,16,0xf8,false);
                                            a.floating_subtract(1,1,4,false);a.floating_sum(2,3,2);a.floating_difference(0,1,0,true,false);a.floating_sum(0,2,0);
                                            a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                        } else {
                                            a.at(0x1036b0b4);
                                            if(a.at_least()) {
                                                a.at(0x1036b170);a.load<1>(4,17,0x63,false);a.value(3,0x43300000);a.load<1>(0,17,0x61,false);
                                                a.sign8(4,4,false);a.store<4>(1,0xe4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);
                                                a.store<4>(1,0xe8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(4,16,0x8,false);a.floating_load(0,1,0xe4,false);
                                                a.store<4>(1,0xf0,0);a.floating_subtract(3,0,4,false);a.floating_load(2,16,0xb8,false);a.store<4>(1,0xec,3);
                                                a.floating_load(0,16,0x40,false);a.floating_load(1,1,0xec,false);a.floating_difference(2,3,2,true,false);a.floating_load(3,16,0x1f0,false);
                                                a.floating_subtract(1,1,4,false);a.floating_sum(2,3,2);a.floating_difference(0,1,0,true,false);a.floating_sum(0,2,0);
                                                a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                            } else {
                                                a.at(0x1036b0b8);a.jump(0x1036b3e0);
                                            }
                                        }
                                    }
                                }
                            } else {
                                a.at(0x1036b080);a.floating_load(0,16,0x310,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                if(!a.equal()) {
                                    a.at(0x1036b090);a.load<4>(0,21,0x0,false);a.flags(0,0,0x40000);a.store<4>(21,0x0,0);
                                    a.jump(0x1036b3e0);
                                }
                            }
                        }
                    }
                }
            } else {
                a.at(0x1036af2c);a.sign8(0,19,true);
                if(!a.equal()) {
                    a.at(0x1036af34);a.compare(24,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1036af3c);a.load<4>(3,18,0x3c,false);a.compare(3,0x0,false);
                        if(a.equal()) {
                            a.at(0x1036b018);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                        } else {
                            a.at(0x1036af48);a.load<4>(4,3,0x10,false);a.compare(4,0x0,false);
                            if(a.equal()) {
                                a.at(0x1036b018);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                            } else {
                                a.at(0x1036af54);a.load<4>(3,24,0x3c,false);a.compare(3,0x0,false);
                                if(a.equal()) {
                                    a.at(0x1036b018);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                } else {
                                    a.at(0x1036af60);a.load<4>(3,3,0x10,false);a.compare(3,0x0,false);
                                    if(a.equal()) {
                                        a.at(0x1036b018);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                    } else {
                                        a.at(0x1036af6c);a.floating_load(1,4,0x0,false);a.floating_load(0,3,0x0,false);a.floating_load(2,4,0x8,false);
                                        a.floating_subtract(3,1,0,false);a.floating_load(0,3,0x8,false);a.floating_subtract(1,2,0,false);a.floating_product(0,3,3);
                                        a.floating_product_sum(1,1,1,0,false);a.call(0x10780fd8);a.load<4>(2,1,0x14,false);a.floating_load(2,16,0x420,false);
                                        a.floating_load(0,16,0x98,false);a.floating_product(2,2,1);a.floating_compare(2,0);
                                        if(a.at_least()) {
                                            a.at(0x1036afc8);a.floating_load(0,16,0x88,false);a.floating_compare(2,0);
                                            if(a.at_least()) {
                                                a.at(0x1036b00c);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                            } else {
                                                a.at(0x1036afd4);a.floating_load(0,16,0x398,false);a.compare(21,0x0,false);a.floating_load(1,16,0x430,false);
                                                a.floating_difference(0,2,0,true,false);a.floating_subtract(1,1,0,false);a.floating_product(31,31,1);
                                                if(!a.equal()) {
                                                    a.at(0x1036aff0);a.floating_load(0,16,0x438,false);a.floating_compare(1,0);
                                                    if(!a.nonpositive()) {
                                                        a.at(0x1036affc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20000);a.store<4>(21,0x0,0);
                                                        a.jump(0x1036b3e0);
                                                    }
                                                }
                                            }
                                        } else {
                                            a.at(0x1036afa8);a.floating_load(0,16,0x428,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                            if(!a.equal()) {
                                                a.at(0x1036afb8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20000);a.store<4>(21,0x0,0);
                                                a.jump(0x1036b3e0);
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
    } else {
        a.at(0x1036ad0c);a.load<4>(3,3,0x2c,false);a.load<4>(0,18,0x2c,false);a.compare_registers(3,0,false);
        if(!a.equal()) {
            a.at(0x1036af0c);a.load<4>(4,18,0x2c,false);a.compare(4,0x0,false);
            if(a.equal()) {
                a.at(0x1036b024);a.load<4>(3,18,0x90,false);a.compare(3,0x0,false);
                if(a.equal()) {
                    return check_additional_country_scale(a);
                } else {
                    a.at(0x1036b030);a.load<4>(4,3,0xc,false);a.compare(4,0x0,false);
                    if(a.equal()) {
                        return check_additional_country_scale(a);
                    } else {
                        a.at(0x1036b03c);a.move(3,17);a.value(5,0x1);a.call(0x100cab10);
                        a.sign8(0,3,true);
                        if(!a.equal()) {
                            return check_additional_country_scale(a);
                        } else {
                            a.at(0x1036b050);a.load<4>(3,18,0x2c,false);a.compare(3,0x0,false);
                            if(!a.equal()) {
                                a.at(0x1036b05c);a.load<4>(4,3,0x68,false);a.load<4>(3,2,-0x7af4,false);a.load<4>(0,3,0x0,false);
                                a.compare_registers(4,0,true);
                                if(a.equal()) {
                                    a.at(0x1036b070);a.floating_load(0,16,0xa0,false);a.floating_product(31,31,0);
                                }
                            }
                            a.at(0x1036b078);a.sign8(0,19,true);
                            if(a.equal()) {
                                a.at(0x1036b0a0);a.compare(15,0x4,true);
                                if(a.equal()) {
                                    a.at(0x1036b0cc);a.floating_load(0,16,0x68,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                    if(!a.equal()) {
                                        a.at(0x1036b0dc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                        a.jump(0x1036b3e0);
                                    }
                                } else {
                                    a.at(0x1036b0a8);
                                    if(a.at_least()) {
                                        a.at(0x1036b0bc);a.compare(15,0x6,true);
                                        if(a.equal()) {
                                            a.at(0x1036b1d4);a.floating_load(0,16,0x58,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                            if(!a.equal()) {
                                                a.at(0x1036b1e4);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                                a.jump(0x1036b3e0);
                                            }
                                        } else {
                                            a.at(0x1036b0c4);
                                            if(!a.at_least()) {
                                                a.at(0x1036b0c8);a.jump(0x1036b0ec);a.at(0x1036b0ec);a.floating_load(0,16,0x348,false);
                                                a.compare(21,0x0,false);a.floating_product(31,31,0);
                                                if(!a.equal()) {
                                                    a.at(0x1036b0fc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                                    a.jump(0x1036b3e0);
                                                }
                                            }
                                        }
                                    } else {
                                        a.at(0x1036b0ac);a.compare(15,0x2,true);
                                        if(a.equal()) {
                                            a.at(0x1036b10c);a.load<1>(4,17,0x63,false);a.value(3,0x43300000);a.load<1>(0,17,0x61,false);
                                            a.sign8(4,4,false);a.store<4>(1,0xe4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);
                                            a.store<4>(1,0xe8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(4,16,0x8,false);a.floating_load(0,1,0xe4,false);
                                            a.store<4>(1,0xf0,0);a.floating_subtract(3,0,4,false);a.floating_load(2,16,0xb8,false);a.store<4>(1,0xec,3);
                                            a.floating_load(0,16,0x40,false);a.floating_load(1,1,0xec,false);a.floating_difference(2,3,2,true,false);a.floating_load(3,16,0xf8,false);
                                            a.floating_subtract(1,1,4,false);a.floating_sum(2,3,2);a.floating_difference(0,1,0,true,false);a.floating_sum(0,2,0);
                                            a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                        } else {
                                            a.at(0x1036b0b4);
                                            if(a.at_least()) {
                                                a.at(0x1036b170);a.load<1>(4,17,0x63,false);a.value(3,0x43300000);a.load<1>(0,17,0x61,false);
                                                a.sign8(4,4,false);a.store<4>(1,0xe4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);
                                                a.store<4>(1,0xe8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(4,16,0x8,false);a.floating_load(0,1,0xe4,false);
                                                a.store<4>(1,0xf0,0);a.floating_subtract(3,0,4,false);a.floating_load(2,16,0xb8,false);a.store<4>(1,0xec,3);
                                                a.floating_load(0,16,0x40,false);a.floating_load(1,1,0xec,false);a.floating_difference(2,3,2,true,false);a.floating_load(3,16,0x1f0,false);
                                                a.floating_subtract(1,1,4,false);a.floating_sum(2,3,2);a.floating_difference(0,1,0,true,false);a.floating_sum(0,2,0);
                                                a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                            } else {
                                                a.at(0x1036b0b8);a.jump(0x1036b3e0);
                                            }
                                        }
                                    }
                                }
                            } else {
                                a.at(0x1036b080);a.floating_load(0,16,0x310,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                if(!a.equal()) {
                                    a.at(0x1036b090);a.load<4>(0,21,0x0,false);a.flags(0,0,0x40000);a.store<4>(21,0x0,0);
                                    a.jump(0x1036b3e0);
                                }
                            }
                        }
                    }
                }
            } else {
                a.at(0x1036af18);a.move(3,17);a.value(5,0x1);a.call(0x100cab10);
                a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x1036b024);a.load<4>(3,18,0x90,false);a.compare(3,0x0,false);
                    if(a.equal()) {
                        return check_additional_country_scale(a);
                    } else {
                        a.at(0x1036b030);a.load<4>(4,3,0xc,false);a.compare(4,0x0,false);
                        if(a.equal()) {
                            return check_additional_country_scale(a);
                        } else {
                            a.at(0x1036b03c);a.move(3,17);a.value(5,0x1);a.call(0x100cab10);
                            a.sign8(0,3,true);
                            if(!a.equal()) {
                                return check_additional_country_scale(a);
                            } else {
                                a.at(0x1036b050);a.load<4>(3,18,0x2c,false);a.compare(3,0x0,false);
                                if(!a.equal()) {
                                    a.at(0x1036b05c);a.load<4>(4,3,0x68,false);a.load<4>(3,2,-0x7af4,false);a.load<4>(0,3,0x0,false);
                                    a.compare_registers(4,0,true);
                                    if(a.equal()) {
                                        a.at(0x1036b070);a.floating_load(0,16,0xa0,false);a.floating_product(31,31,0);
                                    }
                                }
                                a.at(0x1036b078);a.sign8(0,19,true);
                                if(a.equal()) {
                                    a.at(0x1036b0a0);a.compare(15,0x4,true);
                                    if(a.equal()) {
                                        a.at(0x1036b0cc);a.floating_load(0,16,0x68,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                        if(!a.equal()) {
                                            a.at(0x1036b0dc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                            a.jump(0x1036b3e0);
                                        }
                                    } else {
                                        a.at(0x1036b0a8);
                                        if(a.at_least()) {
                                            a.at(0x1036b0bc);a.compare(15,0x6,true);
                                            if(a.equal()) {
                                                a.at(0x1036b1d4);a.floating_load(0,16,0x58,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                                if(!a.equal()) {
                                                    a.at(0x1036b1e4);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                                    a.jump(0x1036b3e0);
                                                }
                                            } else {
                                                a.at(0x1036b0c4);
                                                if(!a.at_least()) {
                                                    a.at(0x1036b0c8);a.jump(0x1036b0ec);a.at(0x1036b0ec);a.floating_load(0,16,0x348,false);
                                                    a.compare(21,0x0,false);a.floating_product(31,31,0);
                                                    if(!a.equal()) {
                                                        a.at(0x1036b0fc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                                                        a.jump(0x1036b3e0);
                                                    }
                                                }
                                            }
                                        } else {
                                            a.at(0x1036b0ac);a.compare(15,0x2,true);
                                            if(a.equal()) {
                                                a.at(0x1036b10c);a.load<1>(4,17,0x63,false);a.value(3,0x43300000);a.load<1>(0,17,0x61,false);
                                                a.sign8(4,4,false);a.store<4>(1,0xe4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);
                                                a.store<4>(1,0xe8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(4,16,0x8,false);a.floating_load(0,1,0xe4,false);
                                                a.store<4>(1,0xf0,0);a.floating_subtract(3,0,4,false);a.floating_load(2,16,0xb8,false);a.store<4>(1,0xec,3);
                                                a.floating_load(0,16,0x40,false);a.floating_load(1,1,0xec,false);a.floating_difference(2,3,2,true,false);a.floating_load(3,16,0xf8,false);
                                                a.floating_subtract(1,1,4,false);a.floating_sum(2,3,2);a.floating_difference(0,1,0,true,false);a.floating_sum(0,2,0);
                                                a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                            } else {
                                                a.at(0x1036b0b4);
                                                if(a.at_least()) {
                                                    a.at(0x1036b170);a.load<1>(4,17,0x63,false);a.value(3,0x43300000);a.load<1>(0,17,0x61,false);
                                                    a.sign8(4,4,false);a.store<4>(1,0xe4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);
                                                    a.store<4>(1,0xe8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(4,16,0x8,false);a.floating_load(0,1,0xe4,false);
                                                    a.store<4>(1,0xf0,0);a.floating_subtract(3,0,4,false);a.floating_load(2,16,0xb8,false);a.store<4>(1,0xec,3);
                                                    a.floating_load(0,16,0x40,false);a.floating_load(1,1,0xec,false);a.floating_difference(2,3,2,true,false);a.floating_load(3,16,0x1f0,false);
                                                    a.floating_subtract(1,1,4,false);a.floating_sum(2,3,2);a.floating_difference(0,1,0,true,false);a.floating_sum(0,2,0);
                                                    a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                                } else {
                                                    a.at(0x1036b0b8);a.jump(0x1036b3e0);
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    a.at(0x1036b080);a.floating_load(0,16,0x310,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                    if(!a.equal()) {
                                        a.at(0x1036b090);a.load<4>(0,21,0x0,false);a.flags(0,0,0x40000);a.store<4>(21,0x0,0);
                                        a.jump(0x1036b3e0);
                                    }
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x1036af2c);a.sign8(0,19,true);
                    if(!a.equal()) {
                        a.at(0x1036af34);a.compare(24,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036af3c);a.load<4>(3,18,0x3c,false);a.compare(3,0x0,false);
                            if(a.equal()) {
                                a.at(0x1036b018);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                            } else {
                                a.at(0x1036af48);a.load<4>(4,3,0x10,false);a.compare(4,0x0,false);
                                if(a.equal()) {
                                    a.at(0x1036b018);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                } else {
                                    a.at(0x1036af54);a.load<4>(3,24,0x3c,false);a.compare(3,0x0,false);
                                    if(a.equal()) {
                                        a.at(0x1036b018);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                    } else {
                                        a.at(0x1036af60);a.load<4>(3,3,0x10,false);a.compare(3,0x0,false);
                                        if(a.equal()) {
                                            a.at(0x1036b018);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                        } else {
                                            a.at(0x1036af6c);a.floating_load(1,4,0x0,false);a.floating_load(0,3,0x0,false);a.floating_load(2,4,0x8,false);
                                            a.floating_subtract(3,1,0,false);a.floating_load(0,3,0x8,false);a.floating_subtract(1,2,0,false);a.floating_product(0,3,3);
                                            a.floating_product_sum(1,1,1,0,false);a.call(0x10780fd8);a.load<4>(2,1,0x14,false);a.floating_load(2,16,0x420,false);
                                            a.floating_load(0,16,0x98,false);a.floating_product(2,2,1);a.floating_compare(2,0);
                                            if(a.at_least()) {
                                                a.at(0x1036afc8);a.floating_load(0,16,0x88,false);a.floating_compare(2,0);
                                                if(a.at_least()) {
                                                    a.at(0x1036b00c);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                                } else {
                                                    a.at(0x1036afd4);a.floating_load(0,16,0x398,false);a.compare(21,0x0,false);a.floating_load(1,16,0x430,false);
                                                    a.floating_difference(0,2,0,true,false);a.floating_subtract(1,1,0,false);a.floating_product(31,31,1);
                                                    if(!a.equal()) {
                                                        a.at(0x1036aff0);a.floating_load(0,16,0x438,false);a.floating_compare(1,0);
                                                        if(!a.nonpositive()) {
                                                            a.at(0x1036affc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20000);a.store<4>(21,0x0,0);
                                                            a.jump(0x1036b3e0);
                                                        }
                                                    }
                                                }
                                            } else {
                                                a.at(0x1036afa8);a.floating_load(0,16,0x428,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                                if(!a.equal()) {
                                                    a.at(0x1036afb8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20000);a.store<4>(21,0x0,0);
                                                    a.jump(0x1036b3e0);
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
        } else {
            a.at(0x1036ad1c);a.sign8(0,19,true);
            if(!a.equal()) {
                a.at(0x1036ad24);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
                a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x1036ad38);a.load<2>(0,17,0x5c,true);a.sign8(0,0,false);a.compare(0,0x18,true);
                    if(!a.at_least()) {
                        a.at(0x1036ad48);a.load<4>(3,17,0x14,false);a.compare(3,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036ad54);a.load<2>(0,3,0xa,true);a.compare(0,0x109a,true);
                            if(!a.at_least()) {
                                a.at(0x1036ad60);a.load<1>(0,17,0x67,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                                if(!a.nonpositive()) {
                                    a.at(0x1036ad70);a.load<1>(0,17,0x66,false);a.sign8(0,0,false);a.compare(0,0xa,true);
                                    if(!a.nonpositive()) {
                                        a.at(0x1036ad80);a.compare(27,0x0,false);
                                        if(!a.equal()) {
                                            a.at(0x1036ad88);a.sign8(0,26,false);a.compare(0,0x4,true);
                                            if(!a.equal()) {
                                                a.at(0x1036ad94);a.compare(0,0x1,true);
                                                if(!a.equal()) {
                                                    a.at(0x1036ad9c);a.compare(0,0x2,true);
                                                    if(!a.equal()) {
                                                        a.at(0x1036ada4);a.compare(0,0x3,true);
                                                        if(!a.equal()) {
                                                            a.at(0x1036adac);a.floating_load(0,16,0xe0,false);a.floating_product(31,31,0);
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
                a.at(0x1036adb4);a.compare(24,0x0,false);
                if(!a.equal()) {
                    a.at(0x1036adbc);a.load<4>(3,18,0x2c,false);a.compare(3,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1036adc8);a.load<4>(4,3,0x68,false);a.load<4>(3,2,-0x7a14,false);a.load<4>(0,3,0x0,false);
                        a.compare_registers(4,0,true);
                        if(a.equal()) {
                            a.at(0x1036addc);a.load<2>(0,17,0x5c,true);a.sign8(0,0,false);a.compare(0,0x18,true);
                            if(!a.at_least()) {
                                a.at(0x1036adec);a.load<4>(4,24,0x30,false);a.compare(4,0x0,false);
                                if(!a.equal()) {
                                    a.at(0x1036adf8);a.load<4>(3,2,-0x7488,false);a.load<4>(4,4,0x0,false);a.load<4>(0,3,0x0,false);
                                    a.compare_registers(4,0,true);
                                    if(a.equal()) {
                                        a.at(0x1036ae0c);a.floating_load(0,16,0x140,false);a.floating_product(31,31,0);
                                    }
                                }
                            }
                        }
                    }
                }
                a.at(0x1036ae14);a.compare(18,0x0,false);
                if(a.equal()) {
                    a.at(0x1036af00);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                } else {
                    a.at(0x1036ae1c);a.load<4>(3,18,0x3c,false);a.compare(3,0x0,false);
                    if(a.equal()) {
                        a.at(0x1036af00);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                    } else {
                        a.at(0x1036ae28);a.load<4>(4,3,0x10,false);a.compare(4,0x0,false);
                        if(a.equal()) {
                            a.at(0x1036af00);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                        } else {
                            a.at(0x1036ae34);a.compare(24,0x0,false);
                            if(a.equal()) {
                                a.at(0x1036af00);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                            } else {
                                a.at(0x1036ae3c);a.load<4>(3,24,0x3c,false);a.compare(3,0x0,false);
                                if(a.equal()) {
                                    a.at(0x1036af00);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                } else {
                                    a.at(0x1036ae48);a.load<4>(3,3,0x10,false);a.compare(3,0x0,false);
                                    if(a.equal()) {
                                        a.at(0x1036af00);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                    } else {
                                        a.at(0x1036ae54);a.floating_load(1,4,0x0,false);a.floating_load(0,3,0x0,false);a.floating_load(2,4,0x8,false);
                                        a.floating_subtract(1,1,0,false);a.floating_load(0,3,0x8,false);a.floating_subtract(2,2,0,false);a.floating_product(0,1,1);
                                        a.floating_product_sum(1,2,2,0,false);a.call(0x10780fd8);a.load<4>(2,1,0x14,false);a.floating_load(2,16,0x420,false);
                                        a.floating_load(0,16,0x98,false);a.floating_product(2,2,1);a.floating_compare(2,0);
                                        if(a.at_least()) {
                                            a.at(0x1036aeb0);a.floating_load(0,16,0x88,false);a.floating_compare(2,0);
                                            if(a.at_least()) {
                                                a.at(0x1036aef4);a.floating_load(0,16,0x70,false);a.floating_product(31,31,0);a.jump(0x1036b3e0);
                                            } else {
                                                a.at(0x1036aebc);a.floating_load(0,16,0x398,false);a.compare(21,0x0,false);a.floating_load(1,16,0x430,false);
                                                a.floating_difference(0,2,0,true,false);a.floating_subtract(1,1,0,false);a.floating_product(31,31,1);
                                                if(!a.equal()) {
                                                    a.at(0x1036aed8);a.floating_load(0,16,0x438,false);a.floating_compare(1,0);
                                                    if(!a.nonpositive()) {
                                                        a.at(0x1036aee4);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20000);a.store<4>(21,0x0,0);
                                                        a.jump(0x1036b3e0);
                                                    }
                                                }
                                            }
                                        } else {
                                            a.at(0x1036ae90);a.floating_load(0,16,0x428,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                            if(!a.equal()) {
                                                a.at(0x1036aea0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x20000);a.store<4>(21,0x0,0);
                                                a.jump(0x1036b3e0);
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
    return calculate_interest_contribution(a);
}

void check_additional_country_scale(Call& a) {
    a.at(0x1036b1f4);a.load<4>(3,18,0x2c,false);a.compare(3,0x0,false);
    if(!a.equal()) {
        a.at(0x1036b200);a.load<4>(4,3,0x68,false);a.load<4>(3,2,-0x7af4,false);a.load<4>(0,3,0x0,false);
        a.compare_registers(4,0,true);
        if(a.equal()) {
            a.at(0x1036b214);a.floating_load(0,16,0xa0,false);a.floating_product(31,31,0);
        }
    }
    return adjust_interest_request_options(a);
}

void adjust_interest_request_options(Call& a) {
    a.at(0x1036b21c);a.sign8(0,19,true);
    if(a.equal()) {
        a.at(0x1036b244);a.compare(15,0x4,true);
        if(a.equal()) {
            a.at(0x1036b2d0);a.floating_load(0,16,0x18,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
            if(!a.equal()) {
                a.at(0x1036b2e0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                a.jump(0x1036b3e0);
            }
        } else {
            a.at(0x1036b24c);
            if(a.at_least()) {
                a.at(0x1036b260);a.compare(15,0x6,true);
                if(a.equal()) {
                    a.at(0x1036b3c4);a.floating_load(0,16,0x2a8,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                    if(!a.equal()) {
                        a.at(0x1036b3d4);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                    }
                } else {
                    a.at(0x1036b268);
                    if(!a.at_least()) {
                        a.at(0x1036b26c);a.jump(0x1036b2f0);a.at(0x1036b2f0);a.load<1>(4,17,0x63,false);
                        a.value(3,0x43300000);a.load<1>(0,17,0x61,false);a.compare(21,0x0,false);a.sign8(4,4,false);
                        a.store<4>(1,0xe4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);a.store<4>(1,0xe8,4);
                        a.exclusive_flags(0,0,0x80000000);a.floating_load(3,16,0x8,false);a.floating_load(0,1,0xe4,false);a.store<4>(1,0xf0,0);
                        a.floating_subtract(1,0,3,false);a.floating_load(2,16,0x20,false);a.store<4>(1,0xec,3);a.floating_load(4,16,0x18,false);
                        a.floating_load(0,1,0xec,false);a.floating_difference(1,1,2,true,false);a.floating_subtract(0,0,3,false);a.floating_sum(1,4,1);
                        a.floating_difference(0,0,2,true,false);a.floating_sum(0,1,0);a.floating_product(31,31,0);
                        if(!a.equal()) {
                            a.at(0x1036b354);a.load<4>(0,21,0x0,false);a.flags(0,0,0x80000);a.store<4>(21,0x0,0);
                            a.jump(0x1036b3e0);
                        }
                    }
                }
            } else {
                a.at(0x1036b250);a.compare(15,0x2,true);
                if(a.equal()) {
                    a.at(0x1036b270);a.load<1>(4,17,0x63,false);a.value(3,0x43300000);a.load<1>(0,17,0x61,false);
                    a.sign8(4,4,false);a.store<4>(1,0xe4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);
                    a.store<4>(1,0xe8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(3,16,0x8,false);a.floating_load(0,1,0xe4,false);
                    a.store<4>(1,0xf0,0);a.floating_subtract(1,0,3,false);a.floating_load(2,16,0x30,false);a.store<4>(1,0xec,3);
                    a.floating_load(4,16,0x1f0,false);a.floating_load(0,1,0xec,false);a.floating_difference(1,1,2,true,false);a.floating_subtract(0,0,3,false);
                    a.floating_sum(1,4,1);a.floating_difference(0,0,2,true,false);a.floating_sum(0,1,0);a.floating_product(31,31,0);
                    a.jump(0x1036b3e0);
                } else {
                    a.at(0x1036b258);
                    if(a.at_least()) {
                        a.at(0x1036b364);a.load<1>(4,17,0x63,false);a.value(3,0x43300000);a.load<1>(0,17,0x61,false);
                        a.sign8(4,4,false);a.store<4>(1,0xe4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);
                        a.store<4>(1,0xe8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(3,16,0x8,false);a.floating_load(0,1,0xe4,false);
                        a.store<4>(1,0xf0,0);a.floating_subtract(1,0,3,false);a.floating_load(2,16,0x30,false);a.store<4>(1,0xec,3);
                        a.floating_load(4,16,0x180,false);a.floating_load(0,1,0xec,false);a.floating_difference(1,1,2,true,false);a.floating_subtract(0,0,3,false);
                        a.floating_sum(1,4,1);a.floating_difference(0,0,2,true,false);a.floating_sum(0,1,0);a.floating_product(31,31,0);
                        a.jump(0x1036b3e0);
                    } else {
                        a.at(0x1036b25c);a.jump(0x1036b3e0);
                    }
                }
            }
        }
    } else {
        a.at(0x1036b224);a.floating_load(0,16,0x440,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
        if(!a.equal()) {
            a.at(0x1036b234);a.load<4>(0,21,0x0,false);a.flags(0,0,0x40000);a.store<4>(21,0x0,0);
            a.jump(0x1036b3e0);
        }
    }
    return calculate_interest_contribution(a);
}

void calculate_interest_contribution(Call& a) {
    a.at(0x1036b3e0);a.sign8(0,19,true);
    if(!a.equal()) {
        a.at(0x1036b460);a.sign8(0,19,true);
        if(!a.equal()) {
            a.at(0x1036b468);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x1036b47c);a.sign8(3,25,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xe4,0);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(0,16,0x30,false);
                a.floating_load(1,1,0xe4,false);a.floating_load(3,16,0x178,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);
                a.floating_subtract(0,3,0,false);a.floating_product(31,31,0);
            }
        }
    } else {
        a.at(0x1036b3e8);a.compare(27,0x0,false);
        if(a.equal()) {
            a.at(0x1036b3fc);a.move(3,17);a.value(4,0x0);a.call(0x100d3040);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x1036b410);a.floating_load(1,16,0xe0,false);a.floating_load(0,16,0xe8,false);a.floating_product(31,31,1);
                a.floating_compare(31,0);
                if(!a.at_least()) {
                    a.at(0x1036b424);a.load<2>(3,18,0x1ec,true);a.compare(3,0x1a5e,true);
                    if(a.positive()) {
                        a.at(0x1036b43c);a.floating_load(0,16,0x80,false);a.floating_load(1,16,0xe8,false);a.floating_sum(0,0,31);
                        a.floating_compare(1,0);
                        if(a.at_least()) {
                            a.at(0x1036b454);a.floating_move(1,0);
                        } else {
                            a.at(0x1036b450);a.jump(0x1036b458);
                        }
                        a.at(0x1036b458);a.floating_move(31,1);a.jump(0x1036b4b0);
                    } else {
                        a.at(0x1036b430);a.load<2>(0,24,0x1ec,true);a.compare_registers(3,0,true);
                        if(!a.nonpositive()) {
                            a.at(0x1036b43c);a.floating_load(0,16,0x80,false);a.floating_load(1,16,0xe8,false);a.floating_sum(0,0,31);
                            a.floating_compare(1,0);
                            if(a.at_least()) {
                                a.at(0x1036b454);a.floating_move(1,0);
                            } else {
                                a.at(0x1036b450);a.jump(0x1036b458);
                            }
                            a.at(0x1036b458);a.floating_move(31,1);a.jump(0x1036b4b0);
                        }
                    }
                }
            }
        } else {
            a.at(0x1036b3f0);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
            if(!a.equal()) {
                a.at(0x1036b410);a.floating_load(1,16,0xe0,false);a.floating_load(0,16,0xe8,false);a.floating_product(31,31,1);
                a.floating_compare(31,0);
                if(!a.at_least()) {
                    a.at(0x1036b424);a.load<2>(3,18,0x1ec,true);a.compare(3,0x1a5e,true);
                    if(a.positive()) {
                        a.at(0x1036b43c);a.floating_load(0,16,0x80,false);a.floating_load(1,16,0xe8,false);a.floating_sum(0,0,31);
                        a.floating_compare(1,0);
                        if(a.at_least()) {
                            a.at(0x1036b454);a.floating_move(1,0);
                        } else {
                            a.at(0x1036b450);a.jump(0x1036b458);
                        }
                        a.at(0x1036b458);a.floating_move(31,1);a.jump(0x1036b4b0);
                    } else {
                        a.at(0x1036b430);a.load<2>(0,24,0x1ec,true);a.compare_registers(3,0,true);
                        if(!a.nonpositive()) {
                            a.at(0x1036b43c);a.floating_load(0,16,0x80,false);a.floating_load(1,16,0xe8,false);a.floating_sum(0,0,31);
                            a.floating_compare(1,0);
                            if(a.at_least()) {
                                a.at(0x1036b454);a.floating_move(1,0);
                            } else {
                                a.at(0x1036b450);a.jump(0x1036b458);
                            }
                            a.at(0x1036b458);a.floating_move(31,1);a.jump(0x1036b4b0);
                        }
                    }
                }
            } else {
                a.at(0x1036b3fc);a.move(3,17);a.value(4,0x0);a.call(0x100d3040);
                a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x1036b410);a.floating_load(1,16,0xe0,false);a.floating_load(0,16,0xe8,false);a.floating_product(31,31,1);
                    a.floating_compare(31,0);
                    if(!a.at_least()) {
                        a.at(0x1036b424);a.load<2>(3,18,0x1ec,true);a.compare(3,0x1a5e,true);
                        if(a.positive()) {
                            a.at(0x1036b43c);a.floating_load(0,16,0x80,false);a.floating_load(1,16,0xe8,false);a.floating_sum(0,0,31);
                            a.floating_compare(1,0);
                            if(a.at_least()) {
                                a.at(0x1036b454);a.floating_move(1,0);
                            } else {
                                a.at(0x1036b450);a.jump(0x1036b458);
                            }
                            a.at(0x1036b458);a.floating_move(31,1);a.jump(0x1036b4b0);
                        } else {
                            a.at(0x1036b430);a.load<2>(0,24,0x1ec,true);a.compare_registers(3,0,true);
                            if(!a.nonpositive()) {
                                a.at(0x1036b43c);a.floating_load(0,16,0x80,false);a.floating_load(1,16,0xe8,false);a.floating_sum(0,0,31);
                                a.floating_compare(1,0);
                                if(a.at_least()) {
                                    a.at(0x1036b454);a.floating_move(1,0);
                                } else {
                                    a.at(0x1036b450);a.jump(0x1036b458);
                                }
                                a.at(0x1036b458);a.floating_move(31,1);a.jump(0x1036b4b0);
                            }
                        }
                    }
                }
            }
        }
    }
    a.at(0x1036b4b0);a.sign8(0,19,true);
    if(!a.equal()) {
        a.at(0x1036b4b8);a.sign8(3,25,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
        a.store<4>(1,0xe4,0);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(3,16,0x140,false);
        a.floating_load(0,1,0xe4,false);a.floating_load(1,16,0x40,false);a.floating_subtract(2,0,2,false);a.floating_load(4,16,0x80,false);
        a.floating_load(0,16,0x48,false);a.floating_product(2,3,2);a.floating_difference(1,2,1,true,false);a.floating_subtract(1,4,1,false);
        a.floating_compare(1,0);
        if(!a.at_least()) {
            a.at(0x1036b4fc);a.floating_move(1,0);
        }
        a.at(0x1036b500);a.floating_product(31,31,1);a.compare(21,0x0,false);
        if(!a.equal()) {
            a.at(0x1036b50c);a.floating_load(0,16,0x180,false);a.floating_compare(1,0);
            if(!a.at_least()) {
                a.at(0x1036b518);a.load<4>(0,21,0x0,false);a.flags(0,0,0x40000);a.store<4>(21,0x0,0);
            }
        }
        a.at(0x1036b524);a.load<4>(3,17,0x14,false);a.load<1>(0,3,0xf,false);a.compare(0,0x14,true);
        if(!a.equal()) {
            a.at(0x1036b558);a.subtract_from(3,22,0x1c);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xe4,0);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(0,16,0x158,false);
            a.floating_load(1,1,0xe4,false);a.floating_subtract(1,1,2,false);a.floating_compare(1,0);
            if(!a.at_least()) {
                a.at(0x1036b584);a.floating_load(0,16,0xe0,false);a.floating_difference(1,1,0,true,false);
            }
        } else {
            a.at(0x1036b534);a.subtract_from(3,22,0x21);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xe4,0);a.floating_load(1,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(0,1,0xe4,false);
            a.floating_subtract(1,0,1,false);a.jump(0x1036b58c);
        }
        a.at(0x1036b58c);a.absolute_at(0x1036b58c,2,1);a.floating_load(0,16,0x2e0,false);a.floating_load(1,16,0x18,false);
        a.compare(21,0x0,false);a.floating_difference(0,2,0,true,false);a.floating_sum(1,1,0);a.floating_product(31,31,1);
        if(!a.equal()) {
            a.at(0x1036b5ac);a.floating_load(0,16,0x180,false);a.floating_compare(1,0);
            if(!a.at_least()) {
                a.at(0x1036b5b8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x40000);a.store<4>(21,0x0,0);
            }
        }
    }
    a.at(0x1036b5c4);a.compare(24,0x0,false);
    if(!a.equal()) {
        a.at(0x1036b5d8);a.move(3,17);a.value(4,0x0);a.call(0x100d3150);
        a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x1036b5ec);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
            a.sign8(0,3,true);
            if(a.equal()) {
                a.at(0x1036b600);a.compare(27,0x0,false);
                if(a.equal()) {
                    a.at(0x1036b614);a.load<4>(3,2,-0x7b80,false);a.sign8(0,19,true);a.load<2>(5,17,0x4e,false);
                    a.load<2>(0,3,0x2,false);a.load<2>(4,17,0x4c,true);a.subtract(0,5,0);a.load<2>(3,3,0x0,true);
                    a.multiply(0,0,0x16d);a.subtract(3,4,3);a.add(0,0,3);
                    if(a.equal()) {
                        a.at(0x1036b668);a.load<4>(3,27,0x50,false);a.compare(3,0x0,true);
                        if(a.equal()) {
                            a.at(0x1036b674);a.load<1>(3,17,0x65,false);a.sign8(3,3,false);a.multiply(3,3,0xf);
                            a.offset(3,3,0x78);a.compare_registers(0,3,true);
                            if(!a.at_least()) {
                                a.at(0x1036b68c);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                                return;
                            }
                        }
                        a.at(0x1036b694);a.load<1>(3,17,0x65,false);a.multiply(0,0,0x14);a.value(4,0x1);
                        a.sign8(3,3,false);a.offset(3,3,0xa);a.compare(3,0x1,true);
                        if(!a.less()) {
                            a.at(0x1036b6b0);a.move(4,3);
                        }
                        a.at(0x1036b6b4);a.divide(0,0,4);
                    } else {
                        a.at(0x1036b640);a.value(3,0x66660000);a.sign8(4,25,false);a.offset(3,3,0x6667);
                        a.multiply_high(3,3,4);a.shift_right(3,3,1);a.mask(4,3,0x1,false,1);a.add(3,3,4);
                        a.subtract_from(3,3,0x14);a.product(0,0,3);a.jump(0x1036b6b8);
                    }
                    a.at(0x1036b6b8);a.compare(0,0x0,true);
                    if(a.at_least()) {
                        a.at(0x1036b6c8);a.compare(0,0x190,true);a.value(3,0x190);
                        if(!a.positive()) {
                            a.at(0x1036b6d4);a.move(3,0);
                        }
                    } else {
                        a.at(0x1036b6c0);a.value(3,0x0);a.jump(0x1036b6d8);
                    }
                    a.at(0x1036b6d8);a.compare(3,0x0,true);
                    if(a.equal()) {
                        a.at(0x1036b6e0);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                        return;
                    }
                    a.at(0x1036b6e8);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xe8,3);
                    a.floating_load(2,16,0x8,false);a.store<4>(1,0xe4,0);a.floating_load(0,16,0x308,false);a.floating_load(1,1,0xe4,false);
                    a.floating_subtract(1,1,2,false);a.floating_difference(27,1,0,true,false);a.jump(0x1036b92c);
                } else {
                    a.at(0x1036b608);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
                    if(a.equal()) {
                        a.at(0x1036b614);a.load<4>(3,2,-0x7b80,false);a.sign8(0,19,true);a.load<2>(5,17,0x4e,false);
                        a.load<2>(0,3,0x2,false);a.load<2>(4,17,0x4c,true);a.subtract(0,5,0);a.load<2>(3,3,0x0,true);
                        a.multiply(0,0,0x16d);a.subtract(3,4,3);a.add(0,0,3);
                        if(a.equal()) {
                            a.at(0x1036b668);a.load<4>(3,27,0x50,false);a.compare(3,0x0,true);
                            if(a.equal()) {
                                a.at(0x1036b674);a.load<1>(3,17,0x65,false);a.sign8(3,3,false);a.multiply(3,3,0xf);
                                a.offset(3,3,0x78);a.compare_registers(0,3,true);
                                if(!a.at_least()) {
                                    a.at(0x1036b68c);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                                    return;
                                }
                            }
                            a.at(0x1036b694);a.load<1>(3,17,0x65,false);a.multiply(0,0,0x14);a.value(4,0x1);
                            a.sign8(3,3,false);a.offset(3,3,0xa);a.compare(3,0x1,true);
                            if(!a.less()) {
                                a.at(0x1036b6b0);a.move(4,3);
                            }
                            a.at(0x1036b6b4);a.divide(0,0,4);
                        } else {
                            a.at(0x1036b640);a.value(3,0x66660000);a.sign8(4,25,false);a.offset(3,3,0x6667);
                            a.multiply_high(3,3,4);a.shift_right(3,3,1);a.mask(4,3,0x1,false,1);a.add(3,3,4);
                            a.subtract_from(3,3,0x14);a.product(0,0,3);a.jump(0x1036b6b8);
                        }
                        a.at(0x1036b6b8);a.compare(0,0x0,true);
                        if(a.at_least()) {
                            a.at(0x1036b6c8);a.compare(0,0x190,true);a.value(3,0x190);
                            if(!a.positive()) {
                                a.at(0x1036b6d4);a.move(3,0);
                            }
                        } else {
                            a.at(0x1036b6c0);a.value(3,0x0);a.jump(0x1036b6d8);
                        }
                        a.at(0x1036b6d8);a.compare(3,0x0,true);
                        if(a.equal()) {
                            a.at(0x1036b6e0);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                            return;
                        }
                        a.at(0x1036b6e8);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xe8,3);
                        a.floating_load(2,16,0x8,false);a.store<4>(1,0xe4,0);a.floating_load(0,16,0x308,false);a.floating_load(1,1,0xe4,false);
                        a.floating_subtract(1,1,2,false);a.floating_difference(27,1,0,true,false);a.jump(0x1036b92c);
                    }
                }
            }
        }
    } else {
        a.at(0x1036b5cc);a.floating_load(0,16,0x3a8,false);a.floating_product(31,31,0);a.jump(0x1036b92c);
    }
    return calculate_interest_financial_context(a);
}

void calculate_interest_financial_context(Call& a) {
    a.at(0x1036b92c);a.compare(24,0x0,false);
    if(!a.equal()) {
        a.at(0x1036b9c4);a.load<4>(3,24,0x2c,false);a.compare(3,0x0,false);
        if(a.equal()) {
            a.at(0x1036b9f0);a.value(29,0x3);
        } else {
            a.at(0x1036b9d0);a.load<1>(0,3,0x11c,false);a.sign8(29,0,false);a.compare(29,0x1,true);
            if(a.less()) {
                a.at(0x1036b9e8);a.value(29,0x3);a.jump(0x1036b9f4);
            } else {
                a.at(0x1036b9e0);a.compare(29,0x3,true);
                if(!a.nonpositive()) {
                    a.at(0x1036b9e8);a.value(29,0x3);a.jump(0x1036b9f4);
                }
            }
        }
        a.at(0x1036b9f4);a.product(0,29,29);a.load<4>(3,24,0x3c,false);a.compare(3,0x0,false);
        a.sign8(29,0,false);
        if(a.equal()) {
            a.at(0x1036bacc);a.load<2>(3,24,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0xe4,0);
            a.exclusive_flags(0,3,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,0);a.floating_load(0,16,0x160,false);
            a.floating_load(1,1,0xe4,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);
            a.floating_store(0,1,0xec);a.load<4>(0,1,0xf0,false);a.store<2>(1,0x132,0);
        } else {
            a.at(0x1036ba08);a.load<4>(3,3,0x10,false);a.compare(3,0x0,false);
            if(a.equal()) {
                a.at(0x1036bacc);a.load<2>(3,24,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0xe4,0);
                a.exclusive_flags(0,3,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,0);a.floating_load(0,16,0x160,false);
                a.floating_load(1,1,0xe4,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);
                a.floating_store(0,1,0xec);a.load<4>(0,1,0xf0,false);a.store<2>(1,0x132,0);
            } else {
                a.at(0x1036ba14);a.load<1>(3,3,0x37,false);a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x1036ba3c);a.load<2>(6,24,0x1ec,true);a.value(4,0x43300000);a.sign8(0,3,false);
                    a.value(3,0x88890000);a.exclusive_flags(5,6,0x80000000);a.store<4>(1,0xe4,4);a.floating_load(2,16,0x8,false);
                    a.product(0,0,6);a.store<4>(1,0xe8,5);a.offset(3,3,-0x7777);a.floating_load(0,16,0x160,false);
                    a.floating_load(1,1,0xe4,false);a.store<4>(1,0xec,4);a.floating_subtract(1,1,2,false);a.multiply_high(3,3,0);
                    a.floating_difference(1,1,0,true,false);a.add(0,3,0);a.shift_right(0,0,3);a.mask(3,0,0x1,false,1);
                    a.add(0,0,3);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xf0,0);a.floating_load(0,1,0xec,false);
                    a.floating_subtract(0,0,2,false);a.floating_compare(1,0);
                    if(a.nonpositive()) {
                        a.at(0x1036baa8);a.store<4>(1,0xf8,0);a.store<4>(1,0xf4,4);a.floating_load(0,1,0xf4,false);
                        a.floating_subtract(1,0,2,false);
                    } else {
                        a.at(0x1036baa4);a.jump(0x1036bab8);
                    }
                    a.at(0x1036bab8);a.floating_integer(0,1);a.floating_store(0,1,0xfc);a.load<4>(0,1,0x100,false);
                    a.store<2>(1,0x132,0);a.jump(0x1036bb04);
                } else {
                    a.at(0x1036ba20);a.load<2>(3,24,0x1ec,true);a.mask(0,3,0x1,false,1);a.add(0,0,3);
                    a.shift_right(0,0,1);a.sign16(0,0,false);a.store<2>(1,0x132,0);a.jump(0x1036bb04);
                }
            }
        }
        a.at(0x1036bb04);a.load<4>(5,24,0x30,false);a.compare(5,0x0,false);
        if(a.equal()) {
            a.at(0x1036bb88);a.load<2>(0,24,0x1ec,true);a.store<2>(1,0x12e,0);
        } else {
            a.at(0x1036bb10);a.load<2>(6,24,0x1ec,true);a.compare(6,0x1d4c,true);
            if(a.less()) {
                a.at(0x1036bb50);a.load<2>(4,5,0x18,true);a.mask(0,6,0x1,false,1);a.add(3,0,6);
                a.multiply(0,4,0xfa);a.shift_right(3,3,1);a.multiply(4,4,0x1f4);a.add(0,3,0);
                a.compare_registers(4,0,true);
                if(a.nonpositive()) {
                    a.at(0x1036bb78);a.move(4,0);
                } else {
                    a.at(0x1036bb74);a.jump(0x1036bb7c);
                }
                a.at(0x1036bb7c);a.sign16(0,4,false);a.store<2>(1,0x12e,0);a.jump(0x1036bb90);
            } else {
                a.at(0x1036bb1c);a.value(3,0x10000);a.load<4>(4,24,0x8,false);a.offset(0,3,-0x7748);
                a.compare_registers(4,0,true);
                if(a.nonpositive()) {
                    a.at(0x1036bb50);a.load<2>(4,5,0x18,true);a.mask(0,6,0x1,false,1);a.add(3,0,6);
                    a.multiply(0,4,0xfa);a.shift_right(3,3,1);a.multiply(4,4,0x1f4);a.add(0,3,0);
                    a.compare_registers(4,0,true);
                    if(a.nonpositive()) {
                        a.at(0x1036bb78);a.move(4,0);
                    } else {
                        a.at(0x1036bb74);a.jump(0x1036bb7c);
                    }
                    a.at(0x1036bb7c);a.sign16(0,4,false);a.store<2>(1,0x12e,0);a.jump(0x1036bb90);
                } else {
                    a.at(0x1036bb30);a.load<2>(0,5,0x18,true);a.multiply(0,0,0x1f4);a.compare_registers(0,6,true);
                    if(!a.nonpositive()) {
                        a.at(0x1036bb40);a.move(6,0);
                    }
                    a.at(0x1036bb44);a.sign16(0,6,false);a.store<2>(1,0x12e,0);a.jump(0x1036bb90);
                }
            }
        }
        a.at(0x1036bb90);a.load<4>(4,24,0x14,false);a.compare(4,-0x1,true);
        if(a.nonpositive()) {
            a.at(0x1036bbcc);a.load<4>(3,2,-0x7984,false);a.load<4>(0,24,0x0,false);a.load<4>(3,3,0x0,false);
            a.mask(0,0,0xfffffff8,false,3);a.indexed_load<4>(0,3,0);a.compare(0,-0x1,true);
            if(!a.nonpositive()) {
                a.at(0x1036bbe8);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,0,0x6c);a.load<4>(3,3,0x0,false);
                a.add(28,3,0);
            }
        } else {
            a.at(0x1036bb9c);a.load<4>(3,2,-0x7a2c,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
            if(!a.equal()) {
                a.at(0x1036bbb8);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,4,0x6c);a.load<4>(3,3,0x0,false);
                a.add(28,3,0);a.jump(0x1036bbf8);
            } else {
                a.at(0x1036bbac);a.load<2>(0,24,0x1ec,true);a.compare(0,0x1388,true);
                if(a.at_least()) {
                    a.at(0x1036bbcc);a.load<4>(3,2,-0x7984,false);a.load<4>(0,24,0x0,false);a.load<4>(3,3,0x0,false);
                    a.mask(0,0,0xfffffff8,false,3);a.indexed_load<4>(0,3,0);a.compare(0,-0x1,true);
                    if(!a.nonpositive()) {
                        a.at(0x1036bbe8);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,0,0x6c);a.load<4>(3,3,0x0,false);
                        a.add(28,3,0);
                    }
                } else {
                    a.at(0x1036bbb8);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,4,0x6c);a.load<4>(3,3,0x0,false);
                    a.add(28,3,0);a.jump(0x1036bbf8);
                }
            }
        }
        a.at(0x1036bbf8);a.compare(28,0x0,false);
        if(a.equal()) {
            a.at(0x1036bcd0);a.load<2>(3,1,0x12e,true);a.value(0,0x43300000);a.store<4>(1,0xe4,0);
            a.exclusive_flags(3,3,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(3,16,0xf8,false);
            a.floating_load(1,1,0xe4,false);a.floating_load(0,16,0x320,false);a.floating_subtract(1,1,2,false);a.floating_product(1,3,1);
            a.floating_compare(1,0);
            if(a.at_least()) {
                a.at(0x1036bd08);a.floating_move(1,0);
            } else {
                a.at(0x1036bd04);a.jump(0x1036bd0c);
            }
            a.at(0x1036bd0c);a.floating_integer(0,1);a.floating_store(0,1,0xec);a.load<4>(0,1,0xf0,false);
            a.store<2>(1,0x12c,0);
        } else {
            a.at(0x1036bc00);a.load<4>(3,2,-0x7a30,false);a.load<4>(4,28,0x0,false);a.load<4>(0,3,0x0,false);
            a.compare_registers(4,0,true);
            if(!a.equal()) {
                a.at(0x1036bc20);a.load<2>(0,28,0x18,true);a.multiply(0,0,0x258);a.sign16(0,0,false);
                a.store<2>(1,0x12c,0);
            } else {
                a.at(0x1036bc14);a.value(0,0x3a98);a.store<2>(1,0x12c,0);a.jump(0x1036bc30);
            }
            a.at(0x1036bc30);a.load<4>(3,2,-0x7b5c,false);a.mask(0,4,0xfffffffc,false,2);a.move(4,24);
            a.load<4>(3,3,0x0,false);a.indexed_load<4>(3,3,0);a.load<4>(3,3,0x8,false);a.call(0x10459b50);
            a.compare(3,0x0,false);
            if(!a.equal()) {
                a.at(0x1036bc54);a.load<1>(0,3,0x4,false);a.compare(0,0x0,false);
                if(a.equal()) {
                    a.at(0x1036bc60);a.load<1>(4,3,0x75,false);a.compare(4,0x1,false);
                    if(!a.less()) {
                        a.at(0x1036bc6c);a.compare(4,0x10,false);
                        if(!a.positive()) {
                            a.at(0x1036bc74);a.value(3,0x43300000);a.load<2>(0,1,0x12c,true);a.store<4>(1,0xf0,4);
                            a.exclusive_flags(0,0,0x80000000);a.floating_load(2,16,0x2b8,false);a.store<4>(1,0xec,3);a.floating_load(0,16,0xb8,false);
                            a.floating_load(1,1,0xec,false);a.store<4>(1,0xe8,0);a.floating_subtract(1,1,2,false);a.floating_load(3,16,0x8,false);
                            a.store<4>(1,0xe4,3);a.floating_load(2,16,0x410,false);a.floating_difference(0,1,0,true,false);a.floating_load(1,1,0xe4,false);
                            a.floating_subtract(1,1,3,false);a.floating_subtract(0,2,0,false);a.floating_product(0,1,0);a.floating_integer(0,0);
                            a.floating_store(0,1,0xf4);a.load<4>(0,1,0xf8,false);a.store<2>(1,0x12c,0);a.jump(0x1036bd1c);
                        }
                    }
                }
            }
        }
        a.at(0x1036bd1c);a.load<4>(3,24,0x90,false);a.compare(3,0x0,false);
        if(!a.equal()) {
            a.at(0x1036bd60);a.load<4>(4,3,0x1c,false);a.compare(4,0x0,false);
            if(!a.equal()) {
                a.at(0x1036bda4);a.load<4>(3,2,-0x79f0,false);a.load<2>(13,4,0x2a,true);a.load<4>(3,3,0x0,false);
                a.compare(3,0x0,false);
                if(!a.equal()) {
                    a.at(0x1036bdb8);a.compare(24,0x0,false);
                    if(a.equal()) {
                        a.at(0x1036bdd8);a.value(5,0x0);
                    } else {
                        a.at(0x1036bdc0);a.load<4>(0,24,0x0,false);a.load<4>(4,3,0x0,false);a.multiply(0,0,0x4c);
                        a.add(4,4,0);a.load<2>(5,4,0xa,true);a.jump(0x1036bddc);
                    }
                    a.at(0x1036bddc);a.compare(24,0x0,false);
                    if(a.equal()) {
                        a.at(0x1036bdfc);a.value(0,0x0);
                    } else {
                        a.at(0x1036bde4);a.load<4>(0,24,0x0,false);a.load<4>(4,3,0x0,false);a.multiply(0,0,0x4c);
                        a.add(4,4,0);a.load<2>(0,4,0x6,true);a.jump(0x1036be00);
                    }
                    a.at(0x1036be00);a.sign16(5,5,false);a.sign16(0,0,false);a.value(4,0x43300000);
                    a.floating_load(1,16,0x8,false);a.exclusive_flags(5,5,0x80000000);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xe8,5);
                    a.compare(24,0x0,false);a.floating_load(4,16,0x18,false);a.store<4>(1,0xe4,4);a.floating_load(2,16,0x68,false);
                    a.floating_load(0,1,0xe4,false);a.store<4>(1,0xf0,0);a.floating_subtract(3,0,1,false);a.store<4>(1,0xec,4);
                    a.floating_load(0,1,0xec,false);a.floating_subtract(1,0,1,false);
                    if(a.equal()) {
                        a.at(0x1036be60);a.value(0,0x0);
                    } else {
                        a.at(0x1036be48);a.load<4>(0,24,0x0,false);a.load<4>(3,3,0x0,false);a.multiply(0,0,0x4c);
                        a.add(3,3,0);a.load<2>(0,3,0x8,true);a.jump(0x1036be64);
                    }
                    a.at(0x1036be64);a.sign16(0,0,false);a.value(4,0x43300000);a.exclusive_flags(0,0,0x80000000);
                    a.store<4>(1,0xf4,4);a.floating_load(5,16,0x8,false);a.store<4>(1,0xf8,0);a.floating_load(0,1,0xf4,false);
                    a.floating_subtract(0,0,5,false);a.floating_product(0,4,0);a.floating_product_sum(0,2,1,0,false);a.floating_product_sum(0,4,3,0,false);
                    a.floating_integer(0,0);a.floating_store(0,1,0xfc);a.load<4>(0,1,0x100,false);a.sign16(0,0,false);
                    a.compare(0,0x1388,true);
                    if(a.at_least()) {
                        a.at(0x1036bf18);a.exclusive_flags(3,0,0x80000000);a.exclusive_flags(0,13,0x80000000);a.store<4>(1,0xe8,3);
                        a.floating_load(4,16,0x338,false);a.store<4>(1,0xe4,4);a.floating_load(2,16,0x68,false);a.floating_load(0,1,0xe4,false);
                        a.store<4>(1,0xf0,0);a.floating_subtract(3,0,5,false);a.floating_load(1,16,0x330,false);a.store<4>(1,0xec,4);
                        a.floating_load(0,1,0xec,false);a.floating_product_sum(3,4,3,2,false);a.store<4>(1,0xf8,0);a.floating_subtract(2,0,5,false);
                        a.store<4>(1,0xf4,4);a.floating_load(0,1,0xf4,false);a.floating_product(2,2,3);a.floating_subtract(0,0,5,false);
                        a.floating_product_sum(0,1,3,0,false);a.floating_compare(2,0);
                        if(a.at_least()) {
                            a.at(0x1036bf74);a.floating_move(2,0);
                        } else {
                            a.at(0x1036bf70);a.jump(0x1036bf78);
                        }
                        a.at(0x1036bf78);a.floating_integer(0,2);a.floating_store(0,1,0xfc);a.load<4>(13,1,0x100,false);
                    } else {
                        a.at(0x1036bea8);a.exclusive_flags(3,0,0x80000000);a.exclusive_flags(0,13,0x80000000);a.store<4>(1,0xe8,3);
                        a.floating_load(4,16,0x328,false);a.store<4>(1,0xe4,4);a.floating_load(2,16,0x208,false);a.floating_load(0,1,0xe4,false);
                        a.store<4>(1,0xf0,0);a.floating_subtract(3,0,5,false);a.floating_load(1,16,0x330,false);a.store<4>(1,0xec,4);
                        a.floating_load(0,1,0xec,false);a.floating_negative_product_difference(3,4,3,2);a.store<4>(1,0xf8,0);a.floating_subtract(0,0,5,false);
                        a.store<4>(1,0xf4,4);a.floating_difference(2,0,3,true,false);a.floating_load(0,1,0xf4,false);a.floating_subtract(0,0,5,false);
                        a.floating_negative_product_difference(0,1,3,0);a.floating_compare(2,0);
                        if(a.nonpositive()) {
                            a.at(0x1036bf04);a.floating_move(2,0);
                        } else {
                            a.at(0x1036bf00);a.jump(0x1036bf08);
                        }
                        a.at(0x1036bf08);a.floating_integer(0,2);a.floating_store(0,1,0xfc);a.load<4>(13,1,0x100,false);
                        a.jump(0x1036bf84);
                    }
                }
            } else {
                a.at(0x1036bd6c);a.load<2>(3,1,0x12e,true);a.value(0,0x43300000);a.store<4>(1,0xe4,0);
                a.exclusive_flags(3,3,0x80000000);a.floating_load(1,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(2,16,0xa0,false);
                a.floating_load(0,1,0xe4,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
                a.floating_store(0,1,0xec);a.load<4>(13,1,0xf0,false);a.jump(0x1036bf84);
            }
        } else {
            a.at(0x1036bd28);a.load<2>(3,1,0x12e,true);a.value(0,0x43300000);a.store<4>(1,0xe4,0);
            a.exclusive_flags(3,3,0x80000000);a.floating_load(1,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(2,16,0xa0,false);
            a.floating_load(0,1,0xe4,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
            a.floating_store(0,1,0xec);a.load<4>(13,1,0xf0,false);a.jump(0x1036bf84);
        }
        a.at(0x1036bf84);a.load<4>(3,2,-0x7ab8,false);a.move(4,24);a.load<2>(15,24,0x1ec,true);
        a.load<4>(3,3,0x0,false);a.call(0x10103510);a.compare(3,0x0,false);
        if(!a.equal()) {
            a.at(0x1036bfa0);a.compare(29,0x1,true);
            if(a.equal()) {
                a.at(0x1036bfa8);a.compare(27,0x0,false);
                if(!a.equal()) {
                    a.at(0x1036bfb0);a.load<4>(0,27,0x50,false);a.compare(0,0x0,true);
                    if(a.equal()) {
                        a.at(0x1036bfbc);a.call(0x100f55b0);a.sign8(0,3,false);a.compare(0,0x1,true);
                        if(a.equal()) {
                            a.at(0x1036c04c);a.value(3,0x43300000);a.exclusive_flags(0,15,0x80000000);a.store<4>(1,0xe8,0);
                            a.offset(0,15,0x2ee);a.exclusive_flags(0,0,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe4,3);
                            a.floating_load(3,16,0xc8,false);a.floating_load(0,1,0xe4,false);a.store<4>(1,0xf0,0);a.floating_subtract(1,0,2,false);
                            a.store<4>(1,0xec,3);a.floating_load(0,1,0xec,false);a.floating_product(1,3,1);a.floating_subtract(0,0,2,false);
                            a.floating_compare(1,0);
                            if(a.nonpositive()) {
                                a.at(0x1036c094);a.store<4>(1,0xf8,0);a.store<4>(1,0xf4,3);a.floating_load(0,1,0xf4,false);
                                a.floating_subtract(1,0,2,false);
                            } else {
                                a.at(0x1036c090);a.jump(0x1036c0a4);
                            }
                            a.at(0x1036c0a4);a.floating_integer(0,1);a.floating_store(0,1,0xfc);a.load<4>(15,1,0x100,false);
                            a.jump(0x1036c0c8);
                        } else {
                            a.at(0x1036bfcc);
                            if(a.at_least()) {
                                a.at(0x1036bfdc);a.compare(0,0x3,true);
                                if(!a.at_least()) {
                                    a.at(0x1036bfe4);a.value(3,0x43300000);a.exclusive_flags(0,15,0x80000000);a.store<4>(1,0xe8,0);
                                    a.offset(0,15,0x5dc);a.exclusive_flags(0,0,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe4,3);
                                    a.floating_load(3,16,0x178,false);a.floating_load(0,1,0xe4,false);a.store<4>(1,0xf0,0);a.floating_subtract(1,0,2,false);
                                    a.store<4>(1,0xec,3);a.floating_load(0,1,0xec,false);a.floating_product(1,3,1);a.floating_subtract(0,0,2,false);
                                    a.floating_compare(1,0);
                                    if(a.nonpositive()) {
                                        a.at(0x1036c02c);a.store<4>(1,0xf8,0);a.store<4>(1,0xf4,3);a.floating_load(0,1,0xf4,false);
                                        a.floating_subtract(1,0,2,false);
                                    } else {
                                        a.at(0x1036c028);a.jump(0x1036c03c);
                                    }
                                    a.at(0x1036c03c);a.floating_integer(0,1);a.floating_store(0,1,0xfc);a.load<4>(15,1,0x100,false);
                                    a.jump(0x1036c0c8);
                                }
                            } else {
                                a.at(0x1036bfd0);a.compare(0,-0x2,true);
                                if(a.equal()) {
                                    a.at(0x1036c0b4);a.compare(21,0x0,false);
                                    if(!a.equal()) {
                                        a.at(0x1036c0bc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x40000000);a.store<4>(21,0x0,0);
                                    }
                                } else {
                                    a.at(0x1036bfd8);a.jump(0x1036c0c8);
                                }
                            }
                        }
                    }
                }
            }
        }
        a.at(0x1036c0c8);a.load<4>(0,24,0x0,false);a.sign16(4,15,false);a.load<4>(3,1,0x1e8,false);
        a.multiply(0,0,0xc);a.load<4>(3,3,0xdc,false);a.add(3,3,0);a.load<2>(0,3,0x6,true);
        a.compare_registers(4,0,true);
        if(!a.nonpositive()) {
            a.at(0x1036c0ec);a.move(0,15);
        }
        a.at(0x1036c0f0);a.sign16(3,0,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
        a.sign16(4,15,false);a.store<4>(1,0xf0,3);a.exclusive_flags(6,4,0x80000000);a.load<2>(3,1,0x132,true);
        a.sign16(5,13,false);a.store<4>(1,0xec,0);a.exclusive_flags(5,5,0x80000000);a.floating_load(4,16,0x8,false);
        a.exclusive_flags(4,3,0x80000000);a.floating_load(0,1,0xec,false);a.store<4>(1,0xe8,6);a.floating_subtract(0,0,4,false);
        a.floating_load(1,16,0x418,false);a.store<4>(1,0xe4,0);a.load<2>(3,1,0x12e,true);a.floating_load(2,1,0xe4,false);
        a.floating_product(0,1,0);a.floating_load(5,16,0x1b8,false);a.exclusive_flags(6,3,0x80000000);a.floating_subtract(1,2,4,false);
        a.store<4>(1,0x120,4);a.load<2>(3,1,0x12c,true);a.floating_product_sum(1,5,1,0,false);a.store<4>(1,0x11c,0);
        a.exclusive_flags(7,3,0x80000000);a.sign8(3,19,true);a.floating_load(0,1,0x11c,false);a.load<4>(3,2,-0x4de8,false);
        a.floating_subtract(0,0,4,true);a.store<4>(1,0x118,5);a.floating_integer(2,1);a.floating_load(3,3,0xc,true);
        a.store<4>(1,0x114,0);a.floating_product_single(0,28,0);a.floating_load(1,1,0x114,false);a.floating_store(2,1,0xf4);
        a.floating_subtract(1,1,4,true);a.load<4>(3,1,0xf8,false);a.store<4>(1,0x110,6);a.sign16(3,3,false);
        a.floating_product_sum(0,3,1,0,true);a.store<4>(1,0x10c,0);a.exclusive_flags(3,3,0x80000000);a.floating_load(1,1,0x10c,false);
        a.store<4>(1,0x108,7);a.floating_subtract(1,1,4,true);a.store<4>(1,0x104,0);a.floating_load(2,1,0x104,false);
        a.floating_product_sum(0,29,1,0,true);a.store<4>(1,0x100,3);a.floating_subtract(1,2,4,true);a.store<4>(1,0xfc,0);
        a.floating_load(2,1,0xfc,false);a.floating_product_sum(0,30,1,0,true);a.floating_subtract(1,2,4,true);a.floating_product_sum(0,3,1,0,true);
        a.floating_difference(0,0,26,true,true);a.floating_integer(0,0);a.floating_store(0,1,0x124);a.load<4>(15,1,0x128,false);
        if(a.equal()) {
            a.at(0x1036c1f0);a.move(3,17);a.value(4,0x0);a.call(0x100d3040);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x1036c204);a.sign16(3,15,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xe4,0);a.floating_load(1,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(2,16,0x168,false);
                a.floating_load(0,1,0xe4,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
                a.floating_store(0,1,0xec);a.load<4>(15,1,0xf0,false);
            }
        }
        a.at(0x1036c238);a.load<1>(4,24,0x23d,false);a.compare(4,0x1,true);
        if(!a.equal()) {
            a.at(0x1036c244);a.compare(27,0x0,false);
            if(a.equal()) {
                a.at(0x1036c264);a.sign16(3,15,false);a.mask(0,3,0x1,false,1);a.add(0,0,3);
                a.offset(3,3,-0x7d0);a.shift_right(0,0,1);a.compare_registers(0,3,true);
                if(a.at_least()) {
                    a.at(0x1036c284);a.move(0,3);
                } else {
                    a.at(0x1036c280);a.jump(0x1036c288);
                }
                a.at(0x1036c288);a.sign16(15,0,false);
            } else {
                a.at(0x1036c24c);a.load<4>(0,27,0x50,false);a.mask(0,0,0x10,true,0);
                if(a.equal()) {
                    a.at(0x1036c264);a.sign16(3,15,false);a.mask(0,3,0x1,false,1);a.add(0,0,3);
                    a.offset(3,3,-0x7d0);a.shift_right(0,0,1);a.compare_registers(0,3,true);
                    if(a.at_least()) {
                        a.at(0x1036c284);a.move(0,3);
                    } else {
                        a.at(0x1036c280);a.jump(0x1036c288);
                    }
                    a.at(0x1036c288);a.sign16(15,0,false);
                } else {
                    a.at(0x1036c258);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1036c264);a.sign16(3,15,false);a.mask(0,3,0x1,false,1);a.add(0,0,3);
                        a.offset(3,3,-0x7d0);a.shift_right(0,0,1);a.compare_registers(0,3,true);
                        if(a.at_least()) {
                            a.at(0x1036c284);a.move(0,3);
                        } else {
                            a.at(0x1036c280);a.jump(0x1036c288);
                        }
                        a.at(0x1036c288);a.sign16(15,0,false);
                    }
                }
            }
            a.at(0x1036c28c);a.sign8(0,4,false);a.compare(0,0x3,true);
            if(a.equal()) {
                a.at(0x1036c298);a.sign16(3,15,false);a.mask(0,3,0x1,false,1);a.add(0,0,3);
                a.offset(3,3,-0x7d0);a.shift_right(0,0,1);a.compare_registers(0,3,true);
                if(a.at_least()) {
                    a.at(0x1036c2b8);a.move(0,3);
                } else {
                    a.at(0x1036c2b4);a.jump(0x1036c2bc);
                }
                a.at(0x1036c2bc);a.sign16(15,0,false);
            }
        }
        a.at(0x1036c2c0);a.load<4>(3,2,-0x7b7c,false);a.move(4,17);a.move(5,24);
        a.load<4>(3,3,0x0,false);a.call(0x1007e820);a.sign8(0,3,false);a.compare(0,0x3,true);
        if(a.equal()) {
            a.at(0x1036c2e0);a.sign16(3,15,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xe4,0);a.floating_load(1,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(2,16,0x1f0,false);
            a.floating_load(0,1,0xe4,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
            a.floating_store(0,1,0xec);a.load<4>(15,1,0xf0,false);
        }
        a.at(0x1036c314);a.sign16(0,15,false);a.value(4,0x43300000);a.exclusive_flags(0,0,0x80000000);
        a.load<4>(6,24,0x2c,false);a.store<4>(1,0xf8,0);a.floating_load(3,16,0x8,false);a.compare(6,0x0,false);
        a.store<4>(1,0xf4,4);a.floating_load(2,16,0x178,false);a.floating_load(1,1,0xf4,false);a.floating_load(0,16,0x350,false);
        a.floating_subtract(1,1,3,false);a.floating_product_difference(0,2,1,0);a.floating_integer(0,0);a.floating_store(0,1,0xfc);
        a.load<4>(15,1,0x100,false);
        if(!a.equal()) {
            a.at(0x1036c358);a.compare_registers(18,24,false);
            if(!a.equal()) {
                a.at(0x1036c360);a.sign16(3,15,false);a.store<4>(1,0xe4,4);a.offset(0,3,0x1388);
                a.load<4>(3,2,-0x7b3c,false);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xec,4);a.floating_load(1,16,0x358,false);
                a.move(4,17);a.store<4>(1,0xe8,0);a.move(5,24);a.load<4>(3,3,0x0,false);
                a.value(7,0x1);a.floating_load(0,1,0xe4,false);a.store<4>(1,0xf0,0);a.floating_subtract(2,0,3,false);
                a.load<1>(6,6,0x119,false);a.floating_load(0,1,0xec,false);a.floating_difference(1,2,1,true,false);a.floating_subtract(0,0,3,false);
                a.floating_product(25,1,0);a.call(0x103092f0);a.sign8(3,3,false);a.value(0,0x43300000);
                a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xf4,0);a.floating_load(3,16,0x8,false);a.store<4>(1,0xf8,3);
                a.floating_load(1,16,0x40,false);a.floating_load(2,1,0xf4,false);a.floating_load(0,16,0x360,false);a.floating_subtract(3,2,3,false);
                a.floating_load(2,16,0x150,false);a.floating_negative_product_difference(0,1,3,0);a.floating_difference(1,3,2,true,false);
                a.floating_difference(0,25,0,true,false);a.floating_compare(1,0);
                if(a.nonpositive()) {
                    a.at(0x1036c3f8);a.floating_move(1,0);
                } else {
                    a.at(0x1036c3f4);a.jump(0x1036c3fc);
                }
                a.at(0x1036c3fc);a.floating_load(0,16,0x58,false);a.floating_sum(0,0,1);a.floating_difference(31,31,0,true,false);
            }
        }
    } else {
        a.at(0x1036b934);a.load<4>(3,17,0xc,false);a.load<1>(0,3,0x11c,false);a.sign8(29,0,false);
        a.compare(29,0x1,true);
        if(a.less()) {
            a.at(0x1036b950);a.value(29,0x3);
        } else {
            a.at(0x1036b948);a.compare(29,0x3,true);
            if(!a.nonpositive()) {
                a.at(0x1036b950);a.value(29,0x3);
            }
        }
        a.at(0x1036b954);a.load<4>(5,17,0x14,false);a.value(3,0x55550000);a.value(0,0x43300000);
        a.floating_load(2,16,0x8,false);a.load<2>(6,5,0xc,true);a.product(8,29,29);a.load<2>(4,5,0x8,true);
        a.offset(7,3,0x5556);a.load<2>(5,5,0xa,true);a.add(3,4,6);a.store<4>(1,0xe4,0);
        a.add(0,5,3);a.floating_load(3,16,0x178,false);a.multiply_high(3,7,0);a.floating_load(0,16,0x350,false);
        a.sign8(29,8,false);a.mask(0,3,0x1,false,1);a.add(0,3,0);a.sign16(0,0,false);
        a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xe8,0);a.floating_load(1,1,0xe4,false);a.floating_subtract(1,1,2,false);
        a.floating_product_difference(0,3,1,0);a.floating_integer(0,0);a.floating_store(0,1,0xec);a.load<4>(15,1,0xf0,false);
        a.jump(0x1036c408);
    }
    a.at(0x1036c408);a.compare_registers(18,24,false);
    if(!a.equal()) {
        a.at(0x1036c410);a.load<4>(4,18,0x30,false);a.compare(4,0x0,false);
        if(a.equal()) {
            a.at(0x1036c430);a.floating_load(0,16,0x48,false);a.floating_product(31,31,0);a.jump(0x1036c48c);
        } else {
            a.at(0x1036c41c);a.load<4>(3,2,-0x7a0c,false);a.load<4>(4,4,0x0,false);a.load<4>(0,3,0x0,false);
            a.compare_registers(4,0,true);
            if(!a.equal()) {
                a.at(0x1036c43c);a.load<4>(0,17,0x24,false);a.compare_registers(0,18,false);
                if(a.equal()) {
                    a.at(0x1036c448);a.load<4>(3,17,0x14,false);a.compare(3,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1036c454);a.load<1>(0,3,0x45,false);a.sign8(0,0,false);a.compare(0,0xa,true);
                        if(!a.nonpositive()) {
                            a.at(0x1036c464);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xe8,3);
                            a.floating_load(2,16,0x8,false);a.store<4>(1,0xe4,0);a.floating_load(0,16,0x150,false);a.floating_load(1,1,0xe4,false);
                            a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_product(31,31,0);
                        }
                    }
                }
            } else {
                a.at(0x1036c430);a.floating_load(0,16,0x48,false);a.floating_product(31,31,0);a.jump(0x1036c48c);
            }
        }
        a.at(0x1036c48c);a.load<4>(6,17,0x14,false);a.value(5,0x55550000);a.value(3,0x43300000);
        a.load<2>(4,18,0x1ec,true);a.load<2>(7,6,0xc,true);a.offset(8,5,0x5556);a.load<2>(5,6,0x8,true);
        a.sign8(0,19,true);a.load<2>(6,6,0xa,true);a.add(0,5,7);a.store<4>(1,0xe4,3);
        a.add(0,6,0);a.floating_load(2,16,0x8,false);a.multiply_high(3,8,0);a.floating_load(0,16,0x380,false);
        a.floating_load(3,16,0x80,false);a.mask(0,3,0x1,false,1);a.add(0,3,0);a.subtract(0,0,4);
        a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xe8,0);a.floating_load(1,1,0xe4,false);a.floating_subtract(1,1,2,false);
        a.floating_difference(0,1,0,true,false);a.floating_sum(28,3,0);
        if(a.equal()) {
            return adjust_interest_contract_flags(a);
        } else {
            a.at(0x1036c4f4);a.compare(27,0x0,false);
            if(a.equal()) {
                a.at(0x1036c508);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
                a.sign8(0,3,true);
                if(a.equal()) {
                    return adjust_interest_contract_flags(a);
                } else {
                    a.at(0x1036c51c);a.load<4>(4,17,0x14,false);a.load<2>(0,24,0x1ec,true);a.load<2>(3,4,0x4,true);
                    a.multiply(3,3,0x32);a.compare_registers(3,0,true);
                    if(a.at_least()) {
                        return adjust_interest_contract_flags(a);
                    } else {
                        a.at(0x1036c534);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            return adjust_interest_contract_flags(a);
                        } else {
                            a.at(0x1036c540);a.compare(22,0x18,true);
                            if(a.less()) {
                                a.at(0x1036c550);a.load<2>(0,18,0x1ec,true);a.compare(0,0xdac,true);
                                if(a.nonpositive()) {
                                    return adjust_interest_contract_flags(a);
                                } else {
                                    a.at(0x1036c55c);a.load<2>(0,4,0xa,true);a.compare(0,0x157c,true);
                                    if(a.at_least()) {
                                        return adjust_interest_contract_flags(a);
                                    } else {
                                        a.at(0x1036c568);a.load<4>(6,18,0x2c,false);a.compare(6,0x0,false);
                                        if(a.equal()) {
                                            return adjust_interest_contract_flags(a);
                                        } else {
                                            a.at(0x1036c574);a.load<4>(3,18,0x30,false);a.compare(3,0x0,false);
                                            if(a.equal()) {
                                                return adjust_interest_contract_flags(a);
                                            } else {
                                                a.at(0x1036c580);a.load<4>(4,24,0x30,false);a.compare(4,0x0,false);
                                                if(a.equal()) {
                                                    return adjust_interest_contract_flags(a);
                                                } else {
                                                    a.at(0x1036c58c);a.load<2>(5,3,0x18,true);a.compare(5,0x6,true);
                                                    if(a.positive()) {
                                                        return compare_interest_country_fields(a);
                                                    } else {
                                                        a.at(0x1036c598);a.load<2>(3,4,0x18,true);a.offset(0,3,-0x3);a.compare_registers(5,0,true);
                                                        if(a.nonpositive()) {
                                                            return adjust_interest_contract_flags(a);
                                                        } else {
                                                            return compare_interest_country_fields(a);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                a.at(0x1036c548);a.compare(22,0x20,true);
                                if(a.nonpositive()) {
                                    return adjust_interest_contract_flags(a);
                                } else {
                                    a.at(0x1036c550);a.load<2>(0,18,0x1ec,true);a.compare(0,0xdac,true);
                                    if(a.nonpositive()) {
                                        return adjust_interest_contract_flags(a);
                                    } else {
                                        a.at(0x1036c55c);a.load<2>(0,4,0xa,true);a.compare(0,0x157c,true);
                                        if(a.at_least()) {
                                            return adjust_interest_contract_flags(a);
                                        } else {
                                            a.at(0x1036c568);a.load<4>(6,18,0x2c,false);a.compare(6,0x0,false);
                                            if(a.equal()) {
                                                return adjust_interest_contract_flags(a);
                                            } else {
                                                a.at(0x1036c574);a.load<4>(3,18,0x30,false);a.compare(3,0x0,false);
                                                if(a.equal()) {
                                                    return adjust_interest_contract_flags(a);
                                                } else {
                                                    a.at(0x1036c580);a.load<4>(4,24,0x30,false);a.compare(4,0x0,false);
                                                    if(a.equal()) {
                                                        return adjust_interest_contract_flags(a);
                                                    } else {
                                                        a.at(0x1036c58c);a.load<2>(5,3,0x18,true);a.compare(5,0x6,true);
                                                        if(a.positive()) {
                                                            return compare_interest_country_fields(a);
                                                        } else {
                                                            a.at(0x1036c598);a.load<2>(3,4,0x18,true);a.offset(0,3,-0x3);a.compare_registers(5,0,true);
                                                            if(a.nonpositive()) {
                                                                return adjust_interest_contract_flags(a);
                                                            } else {
                                                                return compare_interest_country_fields(a);
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
            } else {
                a.at(0x1036c4fc);a.load<4>(0,27,0x50,false);a.mask(0,0,0x10,true,0);
                if(!a.equal()) {
                    a.at(0x1036c51c);a.load<4>(4,17,0x14,false);a.load<2>(0,24,0x1ec,true);a.load<2>(3,4,0x4,true);
                    a.multiply(3,3,0x32);a.compare_registers(3,0,true);
                    if(a.at_least()) {
                        return adjust_interest_contract_flags(a);
                    } else {
                        a.at(0x1036c534);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            return adjust_interest_contract_flags(a);
                        } else {
                            a.at(0x1036c540);a.compare(22,0x18,true);
                            if(a.less()) {
                                a.at(0x1036c550);a.load<2>(0,18,0x1ec,true);a.compare(0,0xdac,true);
                                if(a.nonpositive()) {
                                    return adjust_interest_contract_flags(a);
                                } else {
                                    a.at(0x1036c55c);a.load<2>(0,4,0xa,true);a.compare(0,0x157c,true);
                                    if(a.at_least()) {
                                        return adjust_interest_contract_flags(a);
                                    } else {
                                        a.at(0x1036c568);a.load<4>(6,18,0x2c,false);a.compare(6,0x0,false);
                                        if(a.equal()) {
                                            return adjust_interest_contract_flags(a);
                                        } else {
                                            a.at(0x1036c574);a.load<4>(3,18,0x30,false);a.compare(3,0x0,false);
                                            if(a.equal()) {
                                                return adjust_interest_contract_flags(a);
                                            } else {
                                                a.at(0x1036c580);a.load<4>(4,24,0x30,false);a.compare(4,0x0,false);
                                                if(a.equal()) {
                                                    return adjust_interest_contract_flags(a);
                                                } else {
                                                    a.at(0x1036c58c);a.load<2>(5,3,0x18,true);a.compare(5,0x6,true);
                                                    if(a.positive()) {
                                                        return compare_interest_country_fields(a);
                                                    } else {
                                                        a.at(0x1036c598);a.load<2>(3,4,0x18,true);a.offset(0,3,-0x3);a.compare_registers(5,0,true);
                                                        if(a.nonpositive()) {
                                                            return adjust_interest_contract_flags(a);
                                                        } else {
                                                            return compare_interest_country_fields(a);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                a.at(0x1036c548);a.compare(22,0x20,true);
                                if(a.nonpositive()) {
                                    return adjust_interest_contract_flags(a);
                                } else {
                                    a.at(0x1036c550);a.load<2>(0,18,0x1ec,true);a.compare(0,0xdac,true);
                                    if(a.nonpositive()) {
                                        return adjust_interest_contract_flags(a);
                                    } else {
                                        a.at(0x1036c55c);a.load<2>(0,4,0xa,true);a.compare(0,0x157c,true);
                                        if(a.at_least()) {
                                            return adjust_interest_contract_flags(a);
                                        } else {
                                            a.at(0x1036c568);a.load<4>(6,18,0x2c,false);a.compare(6,0x0,false);
                                            if(a.equal()) {
                                                return adjust_interest_contract_flags(a);
                                            } else {
                                                a.at(0x1036c574);a.load<4>(3,18,0x30,false);a.compare(3,0x0,false);
                                                if(a.equal()) {
                                                    return adjust_interest_contract_flags(a);
                                                } else {
                                                    a.at(0x1036c580);a.load<4>(4,24,0x30,false);a.compare(4,0x0,false);
                                                    if(a.equal()) {
                                                        return adjust_interest_contract_flags(a);
                                                    } else {
                                                        a.at(0x1036c58c);a.load<2>(5,3,0x18,true);a.compare(5,0x6,true);
                                                        if(a.positive()) {
                                                            return compare_interest_country_fields(a);
                                                        } else {
                                                            a.at(0x1036c598);a.load<2>(3,4,0x18,true);a.offset(0,3,-0x3);a.compare_registers(5,0,true);
                                                            if(a.nonpositive()) {
                                                                return adjust_interest_contract_flags(a);
                                                            } else {
                                                                return compare_interest_country_fields(a);
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
                } else {
                    a.at(0x1036c508);a.move(3,17);a.value(4,0x0);a.call(0x100d3230);
                    a.sign8(0,3,true);
                    if(a.equal()) {
                        return adjust_interest_contract_flags(a);
                    } else {
                        a.at(0x1036c51c);a.load<4>(4,17,0x14,false);a.load<2>(0,24,0x1ec,true);a.load<2>(3,4,0x4,true);
                        a.multiply(3,3,0x32);a.compare_registers(3,0,true);
                        if(a.at_least()) {
                            return adjust_interest_contract_flags(a);
                        } else {
                            a.at(0x1036c534);a.load<1>(0,17,0x5e,false);a.compare(0,0x0,false);
                            if(!a.equal()) {
                                return adjust_interest_contract_flags(a);
                            } else {
                                a.at(0x1036c540);a.compare(22,0x18,true);
                                if(a.less()) {
                                    a.at(0x1036c550);a.load<2>(0,18,0x1ec,true);a.compare(0,0xdac,true);
                                    if(a.nonpositive()) {
                                        return adjust_interest_contract_flags(a);
                                    } else {
                                        a.at(0x1036c55c);a.load<2>(0,4,0xa,true);a.compare(0,0x157c,true);
                                        if(a.at_least()) {
                                            return adjust_interest_contract_flags(a);
                                        } else {
                                            a.at(0x1036c568);a.load<4>(6,18,0x2c,false);a.compare(6,0x0,false);
                                            if(a.equal()) {
                                                return adjust_interest_contract_flags(a);
                                            } else {
                                                a.at(0x1036c574);a.load<4>(3,18,0x30,false);a.compare(3,0x0,false);
                                                if(a.equal()) {
                                                    return adjust_interest_contract_flags(a);
                                                } else {
                                                    a.at(0x1036c580);a.load<4>(4,24,0x30,false);a.compare(4,0x0,false);
                                                    if(a.equal()) {
                                                        return adjust_interest_contract_flags(a);
                                                    } else {
                                                        a.at(0x1036c58c);a.load<2>(5,3,0x18,true);a.compare(5,0x6,true);
                                                        if(a.positive()) {
                                                            return compare_interest_country_fields(a);
                                                        } else {
                                                            a.at(0x1036c598);a.load<2>(3,4,0x18,true);a.offset(0,3,-0x3);a.compare_registers(5,0,true);
                                                            if(a.nonpositive()) {
                                                                return adjust_interest_contract_flags(a);
                                                            } else {
                                                                return compare_interest_country_fields(a);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    a.at(0x1036c548);a.compare(22,0x20,true);
                                    if(a.nonpositive()) {
                                        return adjust_interest_contract_flags(a);
                                    } else {
                                        a.at(0x1036c550);a.load<2>(0,18,0x1ec,true);a.compare(0,0xdac,true);
                                        if(a.nonpositive()) {
                                            return adjust_interest_contract_flags(a);
                                        } else {
                                            a.at(0x1036c55c);a.load<2>(0,4,0xa,true);a.compare(0,0x157c,true);
                                            if(a.at_least()) {
                                                return adjust_interest_contract_flags(a);
                                            } else {
                                                a.at(0x1036c568);a.load<4>(6,18,0x2c,false);a.compare(6,0x0,false);
                                                if(a.equal()) {
                                                    return adjust_interest_contract_flags(a);
                                                } else {
                                                    a.at(0x1036c574);a.load<4>(3,18,0x30,false);a.compare(3,0x0,false);
                                                    if(a.equal()) {
                                                        return adjust_interest_contract_flags(a);
                                                    } else {
                                                        a.at(0x1036c580);a.load<4>(4,24,0x30,false);a.compare(4,0x0,false);
                                                        if(a.equal()) {
                                                            return adjust_interest_contract_flags(a);
                                                        } else {
                                                            a.at(0x1036c58c);a.load<2>(5,3,0x18,true);a.compare(5,0x6,true);
                                                            if(a.positive()) {
                                                                return compare_interest_country_fields(a);
                                                            } else {
                                                                a.at(0x1036c598);a.load<2>(3,4,0x18,true);a.offset(0,3,-0x3);a.compare_registers(5,0,true);
                                                                if(a.nonpositive()) {
                                                                    return adjust_interest_contract_flags(a);
                                                                } else {
                                                                    return compare_interest_country_fields(a);
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
                }
            }
        }
    }
    a.at(0x1036cde8);a.compare(24,0x0,false);
    if(!a.equal()) {
        a.at(0x1036cdf0);a.compare_registers(24,18,false);
        if(!a.equal()) {
            a.at(0x1036cdf8);a.load<4>(3,17,0x14,false);a.load<2>(4,3,0x6,true);a.compare(4,0x82,true);
            if(!a.nonpositive()) {
                a.at(0x1036ce08);a.value(3,0x88890000);a.load<2>(0,24,0x1ec,true);a.offset(3,3,-0x7777);
                a.multiply_high(3,3,0);a.add(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);
                a.add(0,0,3);a.compare_registers(4,0,true);
                if(!a.nonpositive()) {
                    a.at(0x1036ce30);a.compare(22,0x1a,true);
                    if(!a.at_least()) {
                        a.at(0x1036ce38);a.move(3,24);a.offset(4,1,0x78);a.offset(5,1,0x70);
                        a.call(0x101929b0);a.sign8(0,3,true);
                        if(!a.equal()) {
                            a.at(0x1036ce50);a.sign8(0,25,true);
                            if(!a.nonpositive()) {
                                a.at(0x1036ce58);a.compare(27,0x0,false);
                                if(!a.equal()) {
                                    a.at(0x1036ce60);a.load<4>(3,1,0x70,false);a.load<4>(0,1,0x74,false);a.store<4>(1,0x38,3);
                                    a.load<2>(3,27,0x32,false);a.load<2>(4,1,0x3a,false);a.store<4>(1,0x3c,0);a.compare_registers(4,3,false);
                                    if(a.at_least()) {
                                        a.at(0x1036ce88);
                                        if(a.nonpositive()) {
                                            a.at(0x1036ce94);a.load<2>(0,1,0x38,true);a.load<2>(4,27,0x30,true);a.combine(0,4,0,false,true);
                                            a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,0x1,false,1);
                                            a.sign8(0,0,false);
                                        } else {
                                            a.at(0x1036ce8c);a.value(0,0x0);a.jump(0x1036ceb4);
                                        }
                                    } else {
                                        a.at(0x1036ce80);a.value(0,0x1);a.jump(0x1036ceb4);
                                    }
                                    a.at(0x1036ceb4);a.sign8(0,0,true);
                                    if(!a.equal()) {
                                        a.at(0x1036cebc);a.load<4>(0,27,0x50,false);a.compare(0,0x0,true);
                                        if(a.equal()) {
                                            a.at(0x1036cec8);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
                                            if(a.equal()) {
                                                a.at(0x1036ced4);a.move(3,18);a.value(4,0x1);a.value(5,0x1);
                                                a.value(6,0x0);a.value(7,0x0);a.call(0x1014c2e0);a.sign8(0,3,true);
                                                if(a.equal()) {
                                                    a.at(0x1036cf04);a.load<4>(3,2,-0x7b80,false);a.load<4>(5,2,-0x7b80,false);a.load<4>(4,3,0x0,false);
                                                    a.offset(3,1,0x70);a.load<4>(5,5,0x4,false);a.call(0x10718130);a.compare(3,0x96,true);
                                                    if(!a.at_least()) {
                                                        a.at(0x1036cf24);a.compare(21,0x0,false);
                                                        if(!a.equal()) {
                                                            a.at(0x1036cf2c);a.value(3,0x80000000);a.load<4>(4,21,0x0,false);a.offset(0,3,0x0);
                                                            a.combine(0,4,0,false,false);a.store<4>(21,0x0,0);
                                                        }
                                                        a.at(0x1036cf40);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                                                        return;
                                                    }
                                                } else {
                                                    a.at(0x1036cef4);a.load<1>(0,17,0x66,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                                                    if(!a.nonpositive()) {
                                                        a.at(0x1036cf04);a.load<4>(3,2,-0x7b80,false);a.load<4>(5,2,-0x7b80,false);a.load<4>(4,3,0x0,false);
                                                        a.offset(3,1,0x70);a.load<4>(5,5,0x4,false);a.call(0x10718130);a.compare(3,0x96,true);
                                                        if(!a.at_least()) {
                                                            a.at(0x1036cf24);a.compare(21,0x0,false);
                                                            if(!a.equal()) {
                                                                a.at(0x1036cf2c);a.value(3,0x80000000);a.load<4>(4,21,0x0,false);a.offset(0,3,0x0);
                                                                a.combine(0,4,0,false,false);a.store<4>(21,0x0,0);
                                                            }
                                                            a.at(0x1036cf40);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                                                            return;
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
        }
    }
    a.at(0x1036cf48);a.load<1>(0,1,0x1f7,false);a.sign8(0,0,true);
    if(a.equal()) {
        a.at(0x1036cf68);a.floating_move(1,31);a.jump(0x1036d21c);
        return;
    }
    a.at(0x1036cf54);a.sign8(0,19,true);
    if(!a.equal()) {
        a.at(0x1036cf68);a.floating_move(1,31);a.jump(0x1036d21c);
        return;
    }
    a.at(0x1036cf5c);a.floating_load(0,16,0x3a8,false);a.floating_compare(31,0);
    if(!a.at_least()) {
        a.at(0x1036cf68);a.floating_move(1,31);a.jump(0x1036d21c);
        return;
    }
    a.at(0x1036cf70);a.offset(3,1,0x80);a.call(0x103721f0);a.load<4>(3,2,-0x7ab8,false);
    a.move(4,18);a.load<4>(3,3,0x0,false);a.call(0x10103510);a.combine(13,3,3,true,false);
    if(a.equal()) {
        a.at(0x1036cf90);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
        return;
    }
    a.at(0x1036cf98);a.move(4,17);a.move(5,18);a.move(7,20);
    a.offset(3,1,0x80);a.value(6,0x0);a.value(8,0xb);a.value(9,0x0);
    a.call(0x1037b6f0);a.floating_move(27,1);a.move(3,13);a.move(4,17);
    a.move(5,20);a.call(0x100f2680);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);
    a.store<4>(1,0xe8,3);a.floating_load(0,16,0x48,false);a.store<4>(1,0xe4,0);a.floating_load(2,16,0x8,false);
    a.floating_compare(0,31);a.floating_load(1,1,0xe4,false);a.floating_load(28,16,0x158,false);a.floating_subtract(26,1,2,false);
    a.floating_load(1,16,0x160,false);
    if(a.nonpositive()) {
        a.at(0x1036d000);a.floating_move(0,31);
    } else {
        a.at(0x1036cffc);a.jump(0x1036d004);
    }
    a.at(0x1036d004);a.floating_difference(29,1,0,true,false);a.floating_load(0,16,0x168,false);a.floating_compare(29,0);
    if(!a.at_least()) {
        a.at(0x1036d014);a.floating_move(29,0);
    }
    a.at(0x1036d018);a.floating_product(27,29,27);a.compare(27,0x0,false);
    if(!a.equal()) {
        a.at(0x1036d024);a.mask(0,23,0x100,true,0);
        if(!a.equal()) {
            a.at(0x1036d02c);a.floating_load(0,16,0x150,false);a.floating_sum(28,28,0);
        }
        a.at(0x1036d034);a.move(3,17);a.move(4,24);a.call(0x100d3150);
        a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x1036d048);a.floating_load(0,16,0x150,false);a.floating_sum(28,28,0);
        }
        a.at(0x1036d050);a.load<1>(3,27,0x44,false);a.sign8(0,3,true);
        if(!a.at_least()) {
            a.at(0x1036d05c);a.sign8(3,3,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xe4,0);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(0,16,0x458,false);
            a.floating_load(1,1,0xe4,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_sum(28,28,0);
        }
        a.at(0x1036d088);a.compare(23,0x0,true);
        if(!a.equal()) {
            a.at(0x1036d090);a.load<1>(3,27,0x4c,false);a.value(0,0x43300000);a.store<4>(1,0xec,0);
            a.sign8(0,3,false);a.floating_load(2,16,0x8,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(0,16,0x458,false);
            a.store<4>(1,0xf0,0);a.floating_load(1,1,0xec,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);
            a.floating_sum(28,28,0);
        }
        a.at(0x1036d0c0);a.floating_load(0,16,0x158,false);a.floating_compare(0,28);
        if(a.nonpositive()) {
            a.at(0x1036d0d4);a.floating_load(0,16,0x238,false);a.floating_compare(0,28);
            if(!a.at_least()) {
                a.at(0x1036d0e0);a.floating_move(28,0);
            }
        } else {
            a.at(0x1036d0cc);a.floating_move(28,0);a.jump(0x1036d0e4);
        }
    }
    a.at(0x1036d0e4);a.floating_load(0,16,0x1b8,false);a.floating_product(0,0,27);a.floating_compare(0,26);
    if(!a.nonpositive()) {
        a.at(0x1036d0f4);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
        return;
    }
    a.at(0x1036d0fc);a.floating_compare(27,26);
    if(!a.nonpositive()) {
        a.at(0x1036d104);a.floating_load(0,16,0x68,false);a.floating_product(0,27,0);a.floating_compare(26,0);
        if(!a.nonpositive()) {
            a.at(0x1036d114);a.floating_move(0,26);
        }
        a.at(0x1036d118);a.floating_load(2,16,0x238,false);a.floating_product(0,28,0);a.floating_subtract(1,2,28,false);
        a.floating_product_sum(0,1,27,0,false);a.floating_difference(27,0,2,true,false);a.floating_round_single(27,27);
    }
    a.at(0x1036d130);a.floating_load(0,16,0x3a8,false);a.floating_compare(31,0);
    if(!a.at_least()) {
        a.at(0x1036d13c);a.floating_load(0,16,0x350,false);a.floating_sum(27,27,0);
    }
    a.at(0x1036d144);a.floating_compare(27,26);
    if(!a.nonpositive()) {
        a.at(0x1036d14c);a.floating_load(0,16,0x158,false);a.floating_compare(27,0);
        if(!a.nonpositive()) {
            a.at(0x1036d158);a.floating_difference(1,26,27,true,false);a.floating_load(0,16,0x1f0,false);a.floating_compare(1,0);
            if(!a.at_least()) {
                a.at(0x1036d168);a.floating_load(1,16,0x48,false);a.jump(0x1036d21c);
                return;
            }
            a.at(0x1036d170);a.floating_product(31,31,1);a.floating_move(27,26);
        }
    }
    a.at(0x1036d178);a.compare(27,0x0,false);
    if(!a.equal()) {
        a.at(0x1036d180);a.load<4>(3,27,0xc,false);a.value(0,0x43300000);a.store<4>(1,0xe4,0);
        a.exclusive_flags(0,3,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,0);a.floating_load(0,16,0x150,false);
        a.floating_load(1,1,0xe4,false);a.floating_subtract(1,1,2,false);a.floating_compare(1,0);
        if(!a.at_least()) {
            a.at(0x1036d1ac);a.floating_move(1,0);
        }
        a.at(0x1036d1b0);a.compare_registers(24,18,false);
        if(!a.equal()) {
            a.at(0x1036d1c0);a.floating_difference(0,27,1,true,false);
        } else {
            a.at(0x1036d1b8);a.floating_difference(0,1,27,true,false);a.jump(0x1036d1c4);
        }
        a.at(0x1036d1c4);a.floating_load(2,16,0x48,false);a.floating_compare(2,0);
        if(a.nonpositive()) {
            a.at(0x1036d1d4);a.floating_load(2,16,0x150,false);a.floating_compare(2,0);
            if(a.at_least()) {
                a.at(0x1036d1e4);a.floating_move(2,0);
            } else {
                a.at(0x1036d1e0);a.jump(0x1036d1e8);
            }
        } else {
            a.at(0x1036d1d0);a.jump(0x1036d1e8);
        }
        a.at(0x1036d1e8);a.floating_load(0,16,0x80,false);a.floating_compare(2,0);a.compare_greater_or_equal_bit();
        if(!a.equal()) {
            a.at(0x1036d200);a.floating_load(1,16,0x238,false);a.floating_subtract(0,1,28,false);a.floating_product_sum(0,2,0,28,false);
            a.floating_difference(0,0,1,true,false);a.floating_difference(0,0,29,true,false);a.floating_product(31,31,0);
        } else {
            a.at(0x1036d1f8);a.floating_product(31,31,2);a.jump(0x1036d218);
        }
    }
    a.at(0x1036d218);a.floating_move(1,31);
    return;
}
}
