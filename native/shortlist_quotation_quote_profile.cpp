#include "shortlist_quotation_detail.hpp"

namespace cm::aot::shortlist_quotation::detail {
void prepare_reference_quote(Call& a) {
    a.at(0x1037b6f0);a.link();a.floating_store(31,1,-0x8);a.floating_store(30,1,-0x10);
    a.multiple(13,false,-0x10);a.move(15,4);a.load<4>(31,2,-0x7b7c,false);a.move(16,5);
    a.load<4>(26,2,-0x7a68,false);a.move(17,6);a.load<4>(27,2,-0x7ab8,false);a.move(18,7);
    a.load<4>(21,2,-0x7b78,false);a.move(22,8);a.load<4>(28,2,-0x7a18,false);a.move(19,9);
    a.load<4>(14,2,-0x7a14,false);a.value(20,0x0);a.load<4>(13,2,-0x7a10,false);a.load<4>(29,2,-0x4dac,false);
    a.load<4>(30,2,-0x4da8,false);a.store<4>(1,0x8,0);a.frame(0xc0);a.load<4>(3,31,0x0,false);
    a.call(0x10082f20);a.compare(3,0x0,false);a.store<4>(21,0x0,3);
    if(!a.equal()) {
        a.at(0x1037b75c);a.load<1>(0,3,0x38,false);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);
        a.sign8(0,0,false);a.compare(0,0x5,true);
        if(!a.equal()) {
            a.at(0x1037b774);a.value(20,0x1);
        }
    }
    a.at(0x1037b778);a.compare(20,0x0,true);
    if(a.equal()) {
        a.at(0x1037b79c);a.value(25,0x0);
    } else {
        a.at(0x1037b780);a.load<4>(3,21,0x0,false);a.load<4>(4,2,-0x7be8,false);a.load<4>(0,3,0x4,false);
        a.load<4>(3,4,0x0,false);a.multiply(0,0,0x248);a.add(25,3,0);a.jump(0x1037b7a0);
    }
    a.at(0x1037b7a0);a.load<4>(3,2,-0x7be4,false);a.value(4,0x43300000);a.load<4>(7,15,0x0,false);
    a.compare(16,0x0,false);a.load<4>(3,3,0x0,false);a.exclusive_flags(5,7,0x80000000);a.load<2>(0,15,0x5c,true);
    a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x54,5);a.floating_load(2,29,0xc0,false);a.sign8(20,0,false);
    a.store<4>(1,0x50,4);a.load<4>(24,15,0x14,false);a.floating_load(0,1,0x50,false);a.store<4>(1,0x4c,3);
    a.floating_subtract(1,0,2,true);a.store<4>(1,0x48,4);a.floating_load(0,1,0x48,false);a.floating_subtract(0,0,2,true);
    a.floating_difference(30,1,0,true,true);
    if(a.equal()) {
        a.at(0x1037b850);a.compare(24,0x0,false);
        if(a.equal()) {
            a.at(0x1037b890);a.load<4>(4,15,0x1c,false);a.compare(4,0x0,false);
            if(a.equal()) {
                a.at(0x1037b8d4);a.value(21,0x1);
            } else {
                a.at(0x1037b89c);a.value(3,0x10620000);a.load<2>(0,4,0x2c,true);a.offset(3,3,0x4dd3);
                a.value(4,0x1);a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);
                a.add(0,0,3);a.compare(0,0x1,true);
                if(!a.less()) {
                    a.at(0x1037b8c4);a.move(4,0);
                }
                a.at(0x1037b8c8);a.sign8(0,4,false);a.move(21,0);a.jump(0x1037b8d8);
            }
        } else {
            a.at(0x1037b858);a.value(3,0x10620000);a.load<2>(0,24,0xc,true);a.offset(3,3,0x4dd3);
            a.value(4,0x1);a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);
            a.add(0,0,3);a.compare(0,0x1,true);
            if(!a.less()) {
                a.at(0x1037b880);a.move(4,0);
            }
            a.at(0x1037b884);a.sign8(0,4,false);a.move(21,0);a.jump(0x1037b8d8);
        }
    } else {
        a.at(0x1037b7f4);a.value(3,0x10620000);a.load<2>(0,16,0x1ec,true);a.offset(3,3,0x4dd3);
        a.compare(25,0x0,false);a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);
        a.add(3,0,3);a.offset(0,3,0x1);a.sign8(3,0,false);a.move(21,3);
        if(!a.equal()) {
            a.at(0x1037b824);a.load<1>(0,16,0x23d,false);a.compare(0,0x2,true);
            if(a.equal()) {
                a.at(0x1037b830);a.offset(21,3,-0x1);a.compare(21,0x1,true);
                if(!a.at_least()) {
                    a.at(0x1037b83c);a.value(21,0x1);
                }
                a.at(0x1037b840);a.compare(21,0x4,true);
                if(!a.nonpositive()) {
                    a.at(0x1037b848);a.value(21,0x4);a.jump(0x1037b8d8);
                }
            }
        }
    }
    a.at(0x1037b8d8);a.compare(25,0x0,false);
    if(a.equal()) {
        a.at(0x1037b8f4);a.load<4>(0,15,0x20,false);a.compare(0,0x0,false);
        if(a.equal()) {
            a.at(0x1037b914);a.load<1>(3,15,0x62,false);a.sign8(0,3,true);
            if(a.equal()) {
                a.at(0x1037b928);a.compare(24,0x0,false);
                if(a.equal()) {
                    a.at(0x1037b938);a.load<1>(3,15,0x60,false);a.sign8(0,3,true);
                    if(a.equal()) {
                        a.at(0x1037b94c);a.value(0,0x9);
                    } else {
                        a.at(0x1037b944);a.move(0,3);a.jump(0x1037b950);
                    }
                } else {
                    a.at(0x1037b930);a.value(0,0xb);a.jump(0x1037b950);
                }
            } else {
                a.at(0x1037b920);a.move(0,3);a.jump(0x1037b950);
            }
        } else {
            a.at(0x1037b900);a.load<1>(3,15,0x60,false);a.sign8(0,3,true);
            if(a.equal()) {
                a.at(0x1037b914);a.load<1>(3,15,0x62,false);a.sign8(0,3,true);
                if(a.equal()) {
                    a.at(0x1037b928);a.compare(24,0x0,false);
                    if(a.equal()) {
                        a.at(0x1037b938);a.load<1>(3,15,0x60,false);a.sign8(0,3,true);
                        if(a.equal()) {
                            a.at(0x1037b94c);a.value(0,0x9);
                        } else {
                            a.at(0x1037b944);a.move(0,3);a.jump(0x1037b950);
                        }
                    } else {
                        a.at(0x1037b930);a.value(0,0xb);a.jump(0x1037b950);
                    }
                } else {
                    a.at(0x1037b920);a.move(0,3);a.jump(0x1037b950);
                }
            } else {
                a.at(0x1037b90c);a.move(0,3);a.jump(0x1037b950);
            }
        }
    } else {
        a.at(0x1037b8e0);a.load<1>(3,15,0x62,false);a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x1037b8f4);a.load<4>(0,15,0x20,false);a.compare(0,0x0,false);
            if(a.equal()) {
                a.at(0x1037b914);a.load<1>(3,15,0x62,false);a.sign8(0,3,true);
                if(a.equal()) {
                    a.at(0x1037b928);a.compare(24,0x0,false);
                    if(a.equal()) {
                        a.at(0x1037b938);a.load<1>(3,15,0x60,false);a.sign8(0,3,true);
                        if(a.equal()) {
                            a.at(0x1037b94c);a.value(0,0x9);
                        } else {
                            a.at(0x1037b944);a.move(0,3);a.jump(0x1037b950);
                        }
                    } else {
                        a.at(0x1037b930);a.value(0,0xb);a.jump(0x1037b950);
                    }
                } else {
                    a.at(0x1037b920);a.move(0,3);a.jump(0x1037b950);
                }
            } else {
                a.at(0x1037b900);a.load<1>(3,15,0x60,false);a.sign8(0,3,true);
                if(a.equal()) {
                    a.at(0x1037b914);a.load<1>(3,15,0x62,false);a.sign8(0,3,true);
                    if(a.equal()) {
                        a.at(0x1037b928);a.compare(24,0x0,false);
                        if(a.equal()) {
                            a.at(0x1037b938);a.load<1>(3,15,0x60,false);a.sign8(0,3,true);
                            if(a.equal()) {
                                a.at(0x1037b94c);a.value(0,0x9);
                            } else {
                                a.at(0x1037b944);a.move(0,3);a.jump(0x1037b950);
                            }
                        } else {
                            a.at(0x1037b930);a.value(0,0xb);a.jump(0x1037b950);
                        }
                    } else {
                        a.at(0x1037b920);a.move(0,3);a.jump(0x1037b950);
                    }
                } else {
                    a.at(0x1037b90c);a.move(0,3);a.jump(0x1037b950);
                }
            }
        } else {
            a.at(0x1037b8ec);a.move(0,3);a.jump(0x1037b950);
        }
    }
    a.at(0x1037b950);a.sign8(3,0,false);a.offset(0,3,-0x5);a.compare(0,0xa,false);
    if(a.positive()) {
        a.at(0x1037cfe8);a.floating_load(31,30,0xa8,true);
    } else {
        a.at(0x1037b960);a.load<4>(3,2,-0x4dbc,false);a.mask(0,0,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
        a.count_destination(3);a.count_branch();
        switch(a.r(0)>>2) {
            case 0: {
                a.at(0x1037b974);a.compare(25,0x0,false);
                if(a.equal()) {
                    a.at(0x1037b9ac);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1037b9b8);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037b9c4);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                            if(!a.equal()) {
                                a.at(0x1037b9d0);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                                a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                                a.offset(0,3,0x1);a.compare_registers(0,21,true);
                                if(!a.nonpositive()) {
                                    a.at(0x1037b9f8);a.move(21,0);
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x1037b97c);a.value(3,0x10620000);a.load<2>(0,25,0x1ec,true);a.offset(3,3,0x4dd3);
                    a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                    a.offset(0,3,0x1);a.compare_registers(0,21,true);
                    if(a.nonpositive()) {
                        a.at(0x1037b9ac);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037b9b8);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                            if(!a.equal()) {
                                a.at(0x1037b9c4);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                                if(!a.equal()) {
                                    a.at(0x1037b9d0);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                                    a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                                    a.offset(0,3,0x1);a.compare_registers(0,21,true);
                                    if(!a.nonpositive()) {
                                        a.at(0x1037b9f8);a.move(21,0);
                                    }
                                }
                            }
                        }
                    } else {
                        a.at(0x1037b9a4);a.move(21,0);a.jump(0x1037b9fc);
                    }
                }
                a.at(0x1037b9fc);a.load<4>(3,15,0x1c,false);a.compare(3,0x0,false);
                if(a.equal()) {
                    a.at(0x1037ba68);a.floating_load(1,30,0x40,true);a.floating_load(0,30,0x58,true);a.floating_load(2,30,0xd0,true);
                    a.floating_product_sum(1,1,30,0);a.floating_load(0,29,0x58,false);a.floating_product_single(1,2,1);a.floating_difference(3,1,0,true,false);
                    a.floating_round_single(3,3);
                } else {
                    a.at(0x1037ba08);a.load<2>(3,3,0x2a,true);a.value(0,0x43300000);a.store<4>(1,0x50,0);
                    a.exclusive_flags(3,3,0x80000000);a.floating_load(4,29,0xc0,false);a.store<4>(1,0x54,3);a.floating_load(5,30,0xd0,true);
                    a.floating_load(0,1,0x50,false);a.store<4>(1,0x44,3);a.floating_subtract(2,0,4,false);a.floating_load(0,29,0x1a8,false);
                    a.store<4>(1,0x40,0);a.floating_load(1,1,0x40,false);a.store<4>(1,0x4c,3);a.floating_subtract(1,1,4,false);
                    a.store<4>(1,0x48,0);a.floating_load(3,1,0x48,false);a.floating_product(1,2,1);a.floating_subtract(2,3,4,false);
                    a.floating_product(1,2,1);a.floating_product(1,5,1);a.floating_difference(3,1,0,true,false);a.floating_round_single(3,3);
                    a.jump(0x1037ba88);
                }
                a.at(0x1037ba88);a.exclusive_flags(3,21,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
                a.floating_load(1,29,0xc0,false);a.store<4>(1,0x38,0);a.floating_load(2,29,0x48,false);a.floating_load(0,1,0x38,false);
                a.floating_load(31,30,0xd4,true);a.floating_subtract(0,0,1,true);a.floating_product_single(0,3,0);a.floating_product(0,2,0);
                a.floating_round_single(0,0);a.floating_compare(31,0);
                if(a.nonpositive()) {
                    a.at(0x1037bac4);a.floating_move(31,0);
                } else {
                    a.at(0x1037bac0);a.jump(0x1037bac8);
                }
                a.at(0x1037bac8);a.floating_load(0,30,0xd0,true);a.floating_compare(31,0);
                if(!a.nonpositive()) {
                    a.at(0x1037bad4);a.floating_load(31,30,0xd8,true);a.jump(0x1037cfec);
                }
                break;
            }
            case 1: {
                a.at(0x1037badc);a.compare(25,0x0,false);
                if(a.equal()) {
                    a.at(0x1037bb14);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1037bb20);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037bb2c);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                            a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                            a.offset(3,3,0x1);a.compare_registers(3,21,true);
                            if(!a.nonpositive()) {
                                a.at(0x1037bb54);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                                if(!a.equal()) {
                                    a.at(0x1037bb60);a.move(21,3);
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x1037bae4);a.value(3,0x10620000);a.load<2>(0,25,0x1ec,true);a.offset(3,3,0x4dd3);
                    a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                    a.offset(0,3,0x1);a.compare_registers(0,21,true);
                    if(a.nonpositive()) {
                        a.at(0x1037bb14);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037bb20);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                            if(!a.equal()) {
                                a.at(0x1037bb2c);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                                a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                                a.offset(3,3,0x1);a.compare_registers(3,21,true);
                                if(!a.nonpositive()) {
                                    a.at(0x1037bb54);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                                    if(!a.equal()) {
                                        a.at(0x1037bb60);a.move(21,3);
                                    }
                                }
                            }
                        }
                    } else {
                        a.at(0x1037bb0c);a.move(21,0);a.jump(0x1037bb64);
                    }
                }
                a.at(0x1037bb64);a.load<4>(3,15,0x1c,false);a.compare(3,0x0,false);
                if(a.equal()) {
                    a.at(0x1037bbd0);a.floating_load(1,30,0x40,true);a.floating_load(0,30,0x58,true);a.floating_load(2,30,0x14,true);
                    a.floating_product_sum(1,1,30,0);a.floating_load(0,29,0x58,false);a.floating_product_single(1,2,1);a.floating_difference(3,1,0,true,false);
                    a.floating_round_single(3,3);
                } else {
                    a.at(0x1037bb70);a.load<2>(3,3,0x2a,true);a.value(0,0x43300000);a.store<4>(1,0x40,0);
                    a.exclusive_flags(3,3,0x80000000);a.floating_load(4,29,0xc0,false);a.store<4>(1,0x44,3);a.floating_load(5,30,0x14,true);
                    a.floating_load(0,1,0x40,false);a.store<4>(1,0x4c,3);a.floating_subtract(2,0,4,false);a.floating_load(0,29,0x1a8,false);
                    a.store<4>(1,0x48,0);a.floating_load(1,1,0x48,false);a.store<4>(1,0x3c,3);a.floating_subtract(1,1,4,false);
                    a.store<4>(1,0x38,0);a.floating_load(3,1,0x38,false);a.floating_product(1,2,1);a.floating_subtract(2,3,4,false);
                    a.floating_product(1,2,1);a.floating_product(1,5,1);a.floating_difference(3,1,0,true,false);a.floating_round_single(3,3);
                    a.jump(0x1037bbf0);
                }
                a.at(0x1037bbf0);a.exclusive_flags(3,21,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x54,3);
                a.floating_load(1,29,0xc0,false);a.store<4>(1,0x50,0);a.floating_load(2,29,0x48,false);a.floating_load(0,1,0x50,false);
                a.floating_load(31,30,0xa4,true);a.floating_subtract(0,0,1,true);a.floating_product_single(0,3,0);a.floating_product(0,2,0);
                a.floating_round_single(0,0);a.floating_compare(31,0);
                if(a.nonpositive()) {
                    a.at(0x1037bc2c);a.floating_move(31,0);
                } else {
                    a.at(0x1037bc28);a.jump(0x1037bc30);
                }
                a.at(0x1037bc30);a.compare(25,0x0,false);
                if(a.equal()) {
                    a.at(0x1037bca0);a.sign8(0,22,false);a.compare(0,0xb,true);
                    if(!a.equal()) {
                        a.at(0x1037bcac);a.compare(0,0x6,true);
                        if(!a.equal()) {
                            a.at(0x1037bcb4);a.floating_load(0,30,0xd0,true);a.floating_compare(31,0);
                            if(!a.nonpositive()) {
                                a.at(0x1037bcc0);a.floating_load(31,30,0xd8,true);a.jump(0x1037cfec);
                            }
                        }
                    }
                } else {
                    a.at(0x1037bc38);a.compare_registers(25,16,false);
                    if(a.equal()) {
                        a.at(0x1037bca0);a.sign8(0,22,false);a.compare(0,0xb,true);
                        if(!a.equal()) {
                            a.at(0x1037bcac);a.compare(0,0x6,true);
                            if(!a.equal()) {
                                a.at(0x1037bcb4);a.floating_load(0,30,0xd0,true);a.floating_compare(31,0);
                                if(!a.nonpositive()) {
                                    a.at(0x1037bcc0);a.floating_load(31,30,0xd8,true);a.jump(0x1037cfec);
                                }
                            }
                        }
                    } else {
                        a.at(0x1037bc40);a.sign8(0,22,false);a.compare(0,0xb,true);
                        if(a.equal()) {
                            a.at(0x1037bc54);a.value(3,0x10620000);a.load<2>(0,25,0x1ec,true);a.offset(3,3,0x4dd3);
                            a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                            a.offset(0,3,0x4);a.compare_registers(0,21,true);
                            if(a.nonpositive()) {
                                a.at(0x1037bc84);a.floating_load(0,30,0x80,true);a.floating_compare(31,0);
                                if(!a.at_least()) {
                                    a.at(0x1037bc90);a.floating_load(0,29,0x148,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                    a.jump(0x1037cfec);
                                }
                            } else {
                                a.at(0x1037bc7c);a.floating_load(31,30,0xd8,true);a.jump(0x1037cfec);
                            }
                        } else {
                            a.at(0x1037bc4c);a.compare(0,0x6,true);
                            if(!a.equal()) {
                                a.at(0x1037bc7c);a.floating_load(31,30,0xd8,true);a.jump(0x1037cfec);
                            } else {
                                a.at(0x1037bc54);a.value(3,0x10620000);a.load<2>(0,25,0x1ec,true);a.offset(3,3,0x4dd3);
                                a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                                a.offset(0,3,0x4);a.compare_registers(0,21,true);
                                if(a.nonpositive()) {
                                    a.at(0x1037bc84);a.floating_load(0,30,0x80,true);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037bc90);a.floating_load(0,29,0x148,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                        a.jump(0x1037cfec);
                                    }
                                } else {
                                    a.at(0x1037bc7c);a.floating_load(31,30,0xd8,true);a.jump(0x1037cfec);
                                }
                            }
                        }
                    }
                }
                break;
            }
            case 2: {
                a.at(0x1037bcc8);a.compare(25,0x0,false);
                if(a.equal()) {
                    a.at(0x1037bd00);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1037bd0c);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037bd18);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                            a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                            a.offset(3,3,0x1);a.compare_registers(3,21,true);
                            if(!a.nonpositive()) {
                                a.at(0x1037bd40);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                                if(!a.equal()) {
                                    a.at(0x1037bd4c);a.move(21,3);
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x1037bcd0);a.value(3,0x10620000);a.load<2>(0,25,0x1ec,true);a.offset(3,3,0x4dd3);
                    a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                    a.offset(0,3,0x1);a.compare_registers(0,21,true);
                    if(a.nonpositive()) {
                        a.at(0x1037bd00);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037bd0c);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                            if(!a.equal()) {
                                a.at(0x1037bd18);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                                a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                                a.offset(3,3,0x1);a.compare_registers(3,21,true);
                                if(!a.nonpositive()) {
                                    a.at(0x1037bd40);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                                    if(!a.equal()) {
                                        a.at(0x1037bd4c);a.move(21,3);
                                    }
                                }
                            }
                        }
                    } else {
                        a.at(0x1037bcf8);a.move(21,0);a.jump(0x1037bd50);
                    }
                }
                a.at(0x1037bd50);a.load<4>(3,15,0x1c,false);a.compare(3,0x0,false);
                if(a.equal()) {
                    a.at(0x1037bdbc);a.floating_load(1,30,0x40,true);a.floating_load(0,30,0x58,true);a.floating_load(2,30,0x14,true);
                    a.floating_product_sum(1,1,30,0);a.floating_load(0,29,0x58,false);a.floating_product_single(1,2,1);a.floating_difference(3,1,0,true,false);
                    a.floating_round_single(3,3);
                } else {
                    a.at(0x1037bd5c);a.load<2>(3,3,0x2a,true);a.value(0,0x43300000);a.store<4>(1,0x40,0);
                    a.exclusive_flags(3,3,0x80000000);a.floating_load(4,29,0xc0,false);a.store<4>(1,0x44,3);a.floating_load(5,30,0x14,true);
                    a.floating_load(0,1,0x40,false);a.store<4>(1,0x4c,3);a.floating_subtract(2,0,4,false);a.floating_load(0,29,0x1a8,false);
                    a.store<4>(1,0x48,0);a.floating_load(1,1,0x48,false);a.store<4>(1,0x3c,3);a.floating_subtract(1,1,4,false);
                    a.store<4>(1,0x38,0);a.floating_load(3,1,0x38,false);a.floating_product(1,2,1);a.floating_subtract(2,3,4,false);
                    a.floating_product(1,2,1);a.floating_product(1,5,1);a.floating_difference(3,1,0,true,false);a.floating_round_single(3,3);
                    a.jump(0x1037bddc);
                }
                a.at(0x1037bddc);a.exclusive_flags(3,21,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x54,3);
                a.floating_load(1,29,0xc0,false);a.store<4>(1,0x50,0);a.floating_load(2,29,0x48,false);a.floating_load(0,1,0x50,false);
                a.floating_load(31,30,0xa4,true);a.floating_subtract(0,0,1,true);a.floating_product_single(0,3,0);a.floating_product(0,2,0);
                a.floating_round_single(0,0);a.floating_compare(31,0);
                if(a.nonpositive()) {
                    a.at(0x1037be18);a.floating_move(31,0);
                } else {
                    a.at(0x1037be14);a.jump(0x1037be1c);
                }
                a.at(0x1037be1c);a.compare(25,0x0,false);
                if(a.equal()) {
                    a.at(0x1037be84);a.sign8(0,22,false);a.compare(0,0xb,true);
                    if(!a.equal()) {
                        a.at(0x1037be90);a.compare(0,0x6,true);
                        if(!a.equal()) {
                            a.at(0x1037be98);a.floating_load(0,30,0xd0,true);a.floating_compare(31,0);
                            if(!a.nonpositive()) {
                                a.at(0x1037bea4);a.floating_load(31,30,0xd8,true);a.jump(0x1037cfec);
                            }
                        }
                    }
                } else {
                    a.at(0x1037be24);a.compare_registers(25,16,false);
                    if(a.equal()) {
                        a.at(0x1037be84);a.sign8(0,22,false);a.compare(0,0xb,true);
                        if(!a.equal()) {
                            a.at(0x1037be90);a.compare(0,0x6,true);
                            if(!a.equal()) {
                                a.at(0x1037be98);a.floating_load(0,30,0xd0,true);a.floating_compare(31,0);
                                if(!a.nonpositive()) {
                                    a.at(0x1037bea4);a.floating_load(31,30,0xd8,true);a.jump(0x1037cfec);
                                }
                            }
                        }
                    } else {
                        a.at(0x1037be2c);a.value(3,0x10620000);a.load<2>(0,25,0x1ec,true);a.offset(3,3,0x4dd3);
                        a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                        a.offset(0,3,0x4);a.compare_registers(0,21,true);
                        if(a.nonpositive()) {
                            a.at(0x1037be5c);a.sign8(0,22,false);a.compare(0,0xb,true);
                            if(!a.equal()) {
                                a.at(0x1037be68);a.compare(0,0x6,true);
                                if(!a.equal()) {
                                    a.at(0x1037be70);a.floating_load(0,30,0xd0,true);a.floating_compare(31,0);
                                    if(!a.nonpositive()) {
                                        a.at(0x1037be7c);a.floating_load(31,30,0xd8,true);a.jump(0x1037cfec);
                                    }
                                }
                            }
                        } else {
                            a.at(0x1037be54);a.floating_load(31,30,0xd8,true);a.jump(0x1037cfec);
                        }
                    }
                }
                break;
            }
            case 3: {
                a.at(0x1037beac);a.compare(25,0x0,false);
                if(a.equal()) {
                    a.at(0x1037bee4);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1037bef0);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037befc);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                            a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                            a.offset(3,3,0x1);a.compare_registers(3,21,true);
                            if(!a.nonpositive()) {
                                a.at(0x1037bf24);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                                if(!a.equal()) {
                                    a.at(0x1037bf30);a.move(21,3);
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x1037beb4);a.value(3,0x10620000);a.load<2>(0,25,0x1ec,true);a.offset(3,3,0x4dd3);
                    a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                    a.offset(0,3,0x1);a.compare_registers(0,21,true);
                    if(a.nonpositive()) {
                        a.at(0x1037bee4);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037bef0);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                            if(!a.equal()) {
                                a.at(0x1037befc);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                                a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                                a.offset(3,3,0x1);a.compare_registers(3,21,true);
                                if(!a.nonpositive()) {
                                    a.at(0x1037bf24);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                                    if(!a.equal()) {
                                        a.at(0x1037bf30);a.move(21,3);
                                    }
                                }
                            }
                        }
                    } else {
                        a.at(0x1037bedc);a.move(21,0);a.jump(0x1037bf34);
                    }
                }
                a.at(0x1037bf34);a.load<4>(3,15,0x1c,false);a.compare(3,0x0,false);
                if(a.equal()) {
                    a.at(0x1037bfa0);a.floating_load(1,30,0x40,true);a.floating_load(0,30,0x58,true);a.floating_load(2,30,0x2c,true);
                    a.floating_product_sum(1,1,30,0);a.floating_load(0,29,0x58,false);a.floating_product_single(1,2,1);a.floating_difference(4,1,0,true,false);
                    a.floating_round_single(4,4);
                } else {
                    a.at(0x1037bf40);a.load<2>(3,3,0x2a,true);a.value(0,0x43300000);a.store<4>(1,0x40,0);
                    a.exclusive_flags(3,3,0x80000000);a.floating_load(4,29,0xc0,false);a.store<4>(1,0x44,3);a.floating_load(5,30,0x2c,true);
                    a.floating_load(0,1,0x40,false);a.store<4>(1,0x4c,3);a.floating_subtract(2,0,4,false);a.floating_load(0,29,0x1a8,false);
                    a.store<4>(1,0x48,0);a.floating_load(1,1,0x48,false);a.store<4>(1,0x3c,3);a.floating_subtract(1,1,4,false);
                    a.store<4>(1,0x38,0);a.floating_load(3,1,0x38,false);a.floating_product(1,2,1);a.floating_subtract(2,3,4,false);
                    a.floating_product(1,2,1);a.floating_product(1,5,1);a.floating_difference(4,1,0,true,false);a.floating_round_single(4,4);
                    a.jump(0x1037bfc0);
                }
                a.at(0x1037bfc0);a.exclusive_flags(3,21,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x54,3);
                a.floating_load(1,29,0xc0,false);a.store<4>(1,0x50,0);a.floating_load(2,29,0x48,false);a.floating_load(0,1,0x50,false);
                a.floating_load(3,30,0xdc,true);a.floating_subtract(0,0,1,true);a.floating_product_single(0,4,0);a.floating_product(0,2,0);
                a.floating_round_single(0,0);a.floating_compare(3,0);
                if(a.nonpositive()) {
                    a.at(0x1037bffc);a.floating_move(3,0);
                } else {
                    a.at(0x1037bff8);a.jump(0x1037c000);
                }
                a.at(0x1037c000);a.floating_load(31,30,0x2c,true);a.floating_compare(31,3);
                if(a.at_least()) {
                    a.at(0x1037c010);a.floating_move(31,3);
                } else {
                    a.at(0x1037c00c);a.jump(0x1037c014);
                }
                a.at(0x1037c014);a.compare(25,0x0,false);
                if(!a.equal()) {
                    a.at(0x1037c01c);a.compare_registers(25,16,false);
                    if(!a.equal()) {
                        a.at(0x1037c024);a.sign8(0,22,false);a.compare(0,0x6,true);
                        if(!a.equal()) {
                            a.at(0x1037c030);a.compare(0,0xb,true);
                            if(!a.equal()) {
                                a.at(0x1037c038);a.value(3,0x10620000);a.load<2>(0,25,0x1ec,true);a.offset(3,3,0x4dd3);
                                a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                                a.offset(0,3,0x4);a.compare_registers(0,21,true);
                                if(!a.nonpositive()) {
                                    a.at(0x1037c060);a.floating_load(31,30,0xd8,true);a.jump(0x1037cfec);
                                }
                            }
                        }
                    }
                }
                break;
            }
            case 4: {
                a.at(0x1037c068);a.compare(25,0x0,false);
                if(a.equal()) {
                    a.at(0x1037c0a0);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1037c0ac);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037c0b8);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                            a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                            a.offset(3,3,0x1);a.compare_registers(3,21,true);
                            if(!a.nonpositive()) {
                                a.at(0x1037c0e0);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                                if(!a.equal()) {
                                    a.at(0x1037c0ec);a.move(21,3);
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x1037c070);a.value(3,0x10620000);a.load<2>(0,25,0x1ec,true);a.offset(3,3,0x4dd3);
                    a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                    a.offset(0,3,0x1);a.compare_registers(0,21,true);
                    if(a.nonpositive()) {
                        a.at(0x1037c0a0);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037c0ac);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                            if(!a.equal()) {
                                a.at(0x1037c0b8);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                                a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                                a.offset(3,3,0x1);a.compare_registers(3,21,true);
                                if(!a.nonpositive()) {
                                    a.at(0x1037c0e0);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                                    if(!a.equal()) {
                                        a.at(0x1037c0ec);a.move(21,3);
                                    }
                                }
                            }
                        }
                    } else {
                        a.at(0x1037c098);a.move(21,0);a.jump(0x1037c0f0);
                    }
                }
                a.at(0x1037c0f0);a.load<4>(3,15,0x1c,false);a.compare(3,0x0,false);
                if(a.equal()) {
                    a.at(0x1037c15c);a.floating_load(1,30,0x40,true);a.floating_load(0,30,0x58,true);a.floating_load(2,30,0xa8,true);
                    a.floating_product_sum(1,1,30,0);a.floating_load(0,29,0x58,false);a.floating_product_single(1,2,1);a.floating_difference(4,1,0,true,false);
                    a.floating_round_single(4,4);
                } else {
                    a.at(0x1037c0fc);a.load<2>(3,3,0x2a,true);a.value(0,0x43300000);a.store<4>(1,0x40,0);
                    a.exclusive_flags(3,3,0x80000000);a.floating_load(4,29,0xc0,false);a.store<4>(1,0x44,3);a.floating_load(5,30,0xa8,true);
                    a.floating_load(0,1,0x40,false);a.store<4>(1,0x4c,3);a.floating_subtract(2,0,4,false);a.floating_load(0,29,0x1a8,false);
                    a.store<4>(1,0x48,0);a.floating_load(1,1,0x48,false);a.store<4>(1,0x3c,3);a.floating_subtract(1,1,4,false);
                    a.store<4>(1,0x38,0);a.floating_load(3,1,0x38,false);a.floating_product(1,2,1);a.floating_subtract(2,3,4,false);
                    a.floating_product(1,2,1);a.floating_product(1,5,1);a.floating_difference(4,1,0,true,false);a.floating_round_single(4,4);
                    a.jump(0x1037c17c);
                }
                a.at(0x1037c17c);a.exclusive_flags(3,21,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x54,3);
                a.floating_load(1,29,0xc0,false);a.store<4>(1,0x50,0);a.floating_load(2,29,0x48,false);a.floating_load(0,1,0x50,false);
                a.floating_load(3,30,0xe0,true);a.floating_subtract(0,0,1,true);a.floating_product_single(0,4,0);a.floating_product(0,2,0);
                a.floating_round_single(0,0);a.floating_compare(3,0);
                if(a.nonpositive()) {
                    a.at(0x1037c1b8);a.floating_move(3,0);
                } else {
                    a.at(0x1037c1b4);a.jump(0x1037c1bc);
                }
                a.at(0x1037c1bc);a.floating_load(31,30,0xa8,true);a.floating_compare(31,3);
                if(a.at_least()) {
                    a.at(0x1037c1cc);a.floating_move(31,3);a.jump(0x1037cfec);
                } else {
                    a.at(0x1037c1c8);a.jump(0x1037cfec);
                }
                break;
            }
            case 5: {
                a.at(0x1037c1d4);a.compare(25,0x0,false);
                if(a.equal()) {
                    a.at(0x1037c20c);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1037c218);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037c224);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                            a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                            a.offset(3,3,0x1);a.compare_registers(3,21,true);
                            if(!a.nonpositive()) {
                                a.at(0x1037c24c);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                                if(!a.equal()) {
                                    a.at(0x1037c258);a.move(21,3);
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x1037c1dc);a.value(3,0x10620000);a.load<2>(0,25,0x1ec,true);a.offset(3,3,0x4dd3);
                    a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                    a.offset(0,3,0x1);a.compare_registers(0,21,true);
                    if(a.nonpositive()) {
                        a.at(0x1037c20c);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037c218);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
                            if(!a.equal()) {
                                a.at(0x1037c224);a.value(3,0x10620000);a.load<2>(0,4,0x1ec,true);a.offset(3,3,0x4dd3);
                                a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(3,0,3);
                                a.offset(3,3,0x1);a.compare_registers(3,21,true);
                                if(!a.nonpositive()) {
                                    a.at(0x1037c24c);a.load<1>(0,15,0x60,false);a.compare(0,0xb,true);
                                    if(!a.equal()) {
                                        a.at(0x1037c258);a.move(21,3);
                                    }
                                }
                            }
                        }
                    } else {
                        a.at(0x1037c204);a.move(21,0);a.jump(0x1037c25c);
                    }
                }
                a.at(0x1037c25c);a.load<4>(3,15,0x1c,false);a.compare(3,0x0,false);
                if(a.equal()) {
                    a.at(0x1037c2c8);a.floating_load(1,30,0x40,true);a.floating_load(0,30,0x58,true);a.floating_load(2,30,0xa8,true);
                    a.floating_product_sum(1,1,30,0);a.floating_load(0,29,0x58,false);a.floating_product_single(1,2,1);a.floating_difference(4,1,0,true,false);
                    a.floating_round_single(4,4);
                } else {
                    a.at(0x1037c268);a.load<2>(3,3,0x2a,true);a.value(0,0x43300000);a.store<4>(1,0x40,0);
                    a.exclusive_flags(3,3,0x80000000);a.floating_load(4,29,0xc0,false);a.store<4>(1,0x44,3);a.floating_load(5,30,0xa8,true);
                    a.floating_load(0,1,0x40,false);a.store<4>(1,0x4c,3);a.floating_subtract(2,0,4,false);a.floating_load(0,29,0x1a8,false);
                    a.store<4>(1,0x48,0);a.floating_load(1,1,0x48,false);a.store<4>(1,0x3c,3);a.floating_subtract(1,1,4,false);
                    a.store<4>(1,0x38,0);a.floating_load(3,1,0x38,false);a.floating_product(1,2,1);a.floating_subtract(2,3,4,false);
                    a.floating_product(1,2,1);a.floating_product(1,5,1);a.floating_difference(4,1,0,true,false);a.floating_round_single(4,4);
                    a.jump(0x1037c2e8);
                }
                a.at(0x1037c2e8);a.exclusive_flags(3,21,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x54,3);
                a.floating_load(1,29,0xc0,false);a.store<4>(1,0x50,0);a.floating_load(2,29,0x48,false);a.floating_load(0,1,0x50,false);
                a.floating_load(3,30,0xe4,true);a.floating_subtract(0,0,1,true);a.floating_product_single(0,4,0);a.floating_product(0,2,0);
                a.floating_round_single(0,0);a.floating_compare(3,0);
                if(a.nonpositive()) {
                    a.at(0x1037c324);a.floating_move(3,0);
                } else {
                    a.at(0x1037c320);a.jump(0x1037c328);
                }
                a.at(0x1037c328);a.floating_load(31,30,0xa8,true);a.floating_compare(31,3);
                if(a.at_least()) {
                    a.at(0x1037c338);a.floating_move(31,3);a.jump(0x1037cfec);
                } else {
                    a.at(0x1037c334);a.jump(0x1037cfec);
                }
                break;
            }
            case 6:case 7:case 8:case 10: {
                a.at(0x1037c340);a.compare(24,0x0,false);
                if(a.equal()) {
                    a.at(0x1037cf78);a.load<4>(4,2,-0x4da4,false);a.load<4>(3,2,-0x7bb8,false);a.load<4>(5,15,0x28,false);
                    a.offset(4,4,0x233);a.load<4>(6,15,0x2c,false);a.call(0x1077b720);a.call(0x1077b770);
                    a.shift_right(4,3,31);a.value(0,0x43300000);a.mask(5,4,0xfffffffc,false,2);a.mask(6,3,0xfffffffc,false,2);
                    a.insert_mask(5,3,0x3,2);a.mask(4,3,0xfff9ffff,false,19);a.move(3,5);a.store<4>(1,0x38,0);
                    a.insert_mask(3,6,0x7fff,0);a.insert_mask(4,5,0xfffe0000,17);a.shift_right(3,3,15);a.floating_load(2,29,0xc0,false);
                    a.add_carry(4,4);a.floating_load(0,30,0x12c,true);a.exclusive_flags(0,4,0x80000000);a.add_carry(3,3);
                    a.store<4>(1,0x3c,0);a.floating_load(1,1,0x38,false);a.floating_subtract(1,1,2,true);a.floating_product_single(31,0,1);
                    a.jump(0x1037cfec);
                } else {
                    a.at(0x1037c348);a.sign8(0,18,false);a.compare(0,-0x1,true);
                    if(a.equal()) {
                        a.at(0x1037c354);a.compare(16,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037c35c);a.load<2>(5,24,0xa,true);a.load<2>(4,16,0x1ec,true);a.compare_registers(5,4,true);
                            if(a.nonpositive()) {
                                a.at(0x1037c384);a.value(3,0x43300000);a.exclusive_flags(0,4,0x80000000);a.store<4>(1,0x44,0);
                                a.exclusive_flags(0,5,0x80000000);a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,3);a.floating_load(1,29,0x30,false);
                                a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,0);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);
                                a.floating_load(2,1,0x38,false);a.floating_product(4,1,0);a.floating_subtract(0,2,3,false);a.floating_compare(0,4);
                                if(a.nonpositive()) {
                                    a.at(0x1037c3d4);a.value(3,0x43300000);a.exclusive_flags(0,4,0x80000000);a.store<4>(1,0x44,0);
                                    a.exclusive_flags(0,5,0x80000000);a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,3);a.floating_load(1,29,0x28,false);
                                    a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,0);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);
                                    a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_compare(1,0);
                                    if(a.nonpositive()) {
                                        return choose_profile_quote_baseline(a);
                                    } else {
                                        a.at(0x1037c414);a.compare(20,0x11,true);
                                        if(a.less()) {
                                            return choose_profile_quote_baseline(a);
                                        } else {
                                            a.at(0x1037c41c);a.value(18,0x4);a.jump(0x1037c474);
                                        }
                                    }
                                } else {
                                    a.at(0x1037c3c4);a.compare(20,0x15,true);
                                    if(a.less()) {
                                        a.at(0x1037c3d4);a.value(3,0x43300000);a.exclusive_flags(0,4,0x80000000);a.store<4>(1,0x44,0);
                                        a.exclusive_flags(0,5,0x80000000);a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,3);a.floating_load(1,29,0x28,false);
                                        a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,0);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);
                                        a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_compare(1,0);
                                        if(a.nonpositive()) {
                                            return choose_profile_quote_baseline(a);
                                        } else {
                                            a.at(0x1037c414);a.compare(20,0x11,true);
                                            if(a.less()) {
                                                return choose_profile_quote_baseline(a);
                                            } else {
                                                a.at(0x1037c41c);a.value(18,0x4);a.jump(0x1037c474);
                                            }
                                        }
                                    } else {
                                        a.at(0x1037c3cc);a.value(18,0x2);a.jump(0x1037c474);
                                    }
                                }
                            } else {
                                a.at(0x1037c36c);a.compare(20,0x18,true);
                                if(a.less()) {
                                    a.at(0x1037c384);a.value(3,0x43300000);a.exclusive_flags(0,4,0x80000000);a.store<4>(1,0x44,0);
                                    a.exclusive_flags(0,5,0x80000000);a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,3);a.floating_load(1,29,0x30,false);
                                    a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,0);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);
                                    a.floating_load(2,1,0x38,false);a.floating_product(4,1,0);a.floating_subtract(0,2,3,false);a.floating_compare(0,4);
                                    if(a.nonpositive()) {
                                        a.at(0x1037c3d4);a.value(3,0x43300000);a.exclusive_flags(0,4,0x80000000);a.store<4>(1,0x44,0);
                                        a.exclusive_flags(0,5,0x80000000);a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,3);a.floating_load(1,29,0x28,false);
                                        a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,0);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);
                                        a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_compare(1,0);
                                        if(a.nonpositive()) {
                                            return choose_profile_quote_baseline(a);
                                        } else {
                                            a.at(0x1037c414);a.compare(20,0x11,true);
                                            if(a.less()) {
                                                return choose_profile_quote_baseline(a);
                                            } else {
                                                a.at(0x1037c41c);a.value(18,0x4);a.jump(0x1037c474);
                                            }
                                        }
                                    } else {
                                        a.at(0x1037c3c4);a.compare(20,0x15,true);
                                        if(a.less()) {
                                            a.at(0x1037c3d4);a.value(3,0x43300000);a.exclusive_flags(0,4,0x80000000);a.store<4>(1,0x44,0);
                                            a.exclusive_flags(0,5,0x80000000);a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,3);a.floating_load(1,29,0x28,false);
                                            a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,0);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);
                                            a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_compare(1,0);
                                            if(a.nonpositive()) {
                                                return choose_profile_quote_baseline(a);
                                            } else {
                                                a.at(0x1037c414);a.compare(20,0x11,true);
                                                if(a.less()) {
                                                    return choose_profile_quote_baseline(a);
                                                } else {
                                                    a.at(0x1037c41c);a.value(18,0x4);a.jump(0x1037c474);
                                                }
                                            }
                                        } else {
                                            a.at(0x1037c3cc);a.value(18,0x2);a.jump(0x1037c474);
                                        }
                                    }
                                } else {
                                    a.at(0x1037c374);a.compare(20,0x22,true);
                                    if(a.at_least()) {
                                        a.at(0x1037c384);a.value(3,0x43300000);a.exclusive_flags(0,4,0x80000000);a.store<4>(1,0x44,0);
                                        a.exclusive_flags(0,5,0x80000000);a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,3);a.floating_load(1,29,0x30,false);
                                        a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,0);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);
                                        a.floating_load(2,1,0x38,false);a.floating_product(4,1,0);a.floating_subtract(0,2,3,false);a.floating_compare(0,4);
                                        if(a.nonpositive()) {
                                            a.at(0x1037c3d4);a.value(3,0x43300000);a.exclusive_flags(0,4,0x80000000);a.store<4>(1,0x44,0);
                                            a.exclusive_flags(0,5,0x80000000);a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,3);a.floating_load(1,29,0x28,false);
                                            a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,0);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);
                                            a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_compare(1,0);
                                            if(a.nonpositive()) {
                                                return choose_profile_quote_baseline(a);
                                            } else {
                                                a.at(0x1037c414);a.compare(20,0x11,true);
                                                if(a.less()) {
                                                    return choose_profile_quote_baseline(a);
                                                } else {
                                                    a.at(0x1037c41c);a.value(18,0x4);a.jump(0x1037c474);
                                                }
                                            }
                                        } else {
                                            a.at(0x1037c3c4);a.compare(20,0x15,true);
                                            if(a.less()) {
                                                a.at(0x1037c3d4);a.value(3,0x43300000);a.exclusive_flags(0,4,0x80000000);a.store<4>(1,0x44,0);
                                                a.exclusive_flags(0,5,0x80000000);a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,3);a.floating_load(1,29,0x28,false);
                                                a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,0);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);
                                                a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_compare(1,0);
                                                if(a.nonpositive()) {
                                                    return choose_profile_quote_baseline(a);
                                                } else {
                                                    a.at(0x1037c414);a.compare(20,0x11,true);
                                                    if(a.less()) {
                                                        return choose_profile_quote_baseline(a);
                                                    } else {
                                                        a.at(0x1037c41c);a.value(18,0x4);a.jump(0x1037c474);
                                                    }
                                                }
                                            } else {
                                                a.at(0x1037c3cc);a.value(18,0x2);a.jump(0x1037c474);
                                            }
                                        }
                                    } else {
                                        a.at(0x1037c37c);a.value(18,0x1);a.jump(0x1037c474);
                                    }
                                }
                            }
                        }
                    }
                    return apply_quote_mode(a);
                }
                break;
            }
            case 9: {
                a.at(0x1037cfe8);a.floating_load(31,30,0xa8,true);
                break;
            }
        }
    }
    a.at(0x1037cfec);a.compare(16,0x0,false);
    if(!a.equal()) {
        a.at(0x1037cff4);a.compare(24,0x0,false);
        if(!a.equal()) {
            a.at(0x1037cffc);a.load<1>(0,16,0x23d,false);a.compare(0,0x1,true);
            if(a.equal()) {
                a.at(0x1037d064);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                a.exclusive_flags(0,3,0x80000000);a.floating_load(1,29,0xc0,false);a.store<4>(1,0x3c,0);a.floating_load(2,29,0x1d0,false);
                a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(1,2,0);
            } else {
                a.at(0x1037d008);a.load<2>(0,16,0x1ec,true);a.compare(0,0x8ca,true);
                if(a.nonpositive()) {
                    a.at(0x1037d03c);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
                    a.floating_load(1,29,0xc0,false);a.store<4>(1,0x38,0);a.floating_load(2,29,0xe0,false);a.floating_load(0,1,0x38,false);
                    a.floating_subtract(0,0,1,false);a.floating_product(1,2,0);a.jump(0x1037d08c);
                } else {
                    a.at(0x1037d014);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
                    a.floating_load(1,29,0xc0,false);a.store<4>(1,0x38,0);a.floating_load(2,29,0x1c8,false);a.floating_load(0,1,0x38,false);
                    a.floating_subtract(0,0,1,false);a.floating_product(1,2,0);a.jump(0x1037d08c);
                }
            }
            a.at(0x1037d08c);a.floating_load(2,29,0x1d8,false);a.call(0x10780fc0);a.load<4>(2,1,0x14,false);
            a.floating_move(30,1);a.floating_load(0,29,0x108,false);a.floating_compare(30,0);
            if(!a.nonpositive()) {
                a.at(0x1037d0a8);a.floating_move(30,0);
            }
            a.at(0x1037d0ac);a.sign8(19,18,false);a.compare(19,0x4,true);
            if(a.equal()) {
                a.at(0x1037d3e4);a.compare(20,0x14,true);
                if(a.at_least()) {
                    a.at(0x1037d438);a.compare(20,0x18,true);
                    if(a.less()) {
                        a.at(0x1037d450);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                        a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x78,false);a.exclusive_flags(0,0,0x80000000);
                        a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                        a.floating_product(0,30,0);a.floating_compare(31,0);
                        if(!a.at_least()) {
                            a.at(0x1037d488);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                            a.floating_round_single(31,31);a.jump(0x1037d57c);
                        }
                    } else {
                        a.at(0x1037d440);a.move(3,16);a.call(0x100de260);a.sign8(0,3,true);
                        if(!a.equal()) {
                            a.at(0x1037d49c);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                            a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e8,false);a.exclusive_flags(0,0,0x80000000);
                            a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                            a.floating_product(0,30,0);a.floating_compare(31,0);
                            if(!a.at_least()) {
                                a.at(0x1037d4d4);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                            }
                        } else {
                            a.at(0x1037d450);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                            a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x78,false);a.exclusive_flags(0,0,0x80000000);
                            a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                            a.floating_product(0,30,0);a.floating_compare(31,0);
                            if(!a.at_least()) {
                                a.at(0x1037d488);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                            }
                        }
                    }
                } else {
                    a.at(0x1037d3ec);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x88,false);a.exclusive_flags(0,0,0x80000000);
                    a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                    a.floating_product(0,30,0);a.floating_compare(31,0);
                    if(!a.at_least()) {
                        a.at(0x1037d424);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                    }
                }
            } else {
                a.at(0x1037d0b8);
                if(a.at_least()) {
                    a.at(0x1037d0d4);a.compare(19,0x6,true);
                    if(a.equal()) {
                        a.at(0x1037d534);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                        a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1f0,false);a.exclusive_flags(0,0,0x80000000);
                        a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                        a.floating_product(0,30,0);a.floating_compare(31,0);
                        if(!a.at_least()) {
                            a.at(0x1037d56c);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                            a.floating_round_single(31,31);
                        }
                    } else {
                        a.at(0x1037d0dc);
                        if(!a.at_least()) {
                            a.at(0x1037d0e0);a.jump(0x1037d4e8);a.at(0x1037d4e8);a.load<2>(3,16,0x1ec,true);
                            a.value(0,0x43300000);a.store<4>(1,0x38,0);a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);
                            a.floating_load(2,29,0xd8,false);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);
                            a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_product(0,30,0);a.floating_compare(31,0);
                            if(!a.at_least()) {
                                a.at(0x1037d520);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                            }
                        }
                    }
                } else {
                    a.at(0x1037d0bc);a.compare(19,0x2,true);
                    if(a.equal()) {
                        a.at(0x1037d1e8);a.compare(20,0x14,true);
                        if(a.at_least()) {
                            a.at(0x1037d23c);a.compare(20,0x18,true);
                            if(a.less()) {
                                a.at(0x1037d254);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e0,false);a.exclusive_flags(0,0,0x80000000);
                                a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                a.floating_product(0,30,0);a.floating_compare(31,0);
                                if(!a.at_least()) {
                                    a.at(0x1037d28c);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                    a.floating_round_single(31,31);a.jump(0x1037d57c);
                                }
                            } else {
                                a.at(0x1037d244);a.move(3,16);a.call(0x100de260);a.sign8(0,3,true);
                                if(!a.equal()) {
                                    a.at(0x1037d2a0);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x3c,0);
                                    a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,30,0);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037d2d0);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                                    }
                                } else {
                                    a.at(0x1037d254);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e0,false);a.exclusive_flags(0,0,0x80000000);
                                    a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                    a.floating_product(0,30,0);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037d28c);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                                    }
                                }
                            }
                        } else {
                            a.at(0x1037d1f0);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                            a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e8,false);a.exclusive_flags(0,0,0x80000000);
                            a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                            a.floating_product(0,30,0);a.floating_compare(31,0);
                            if(!a.at_least()) {
                                a.at(0x1037d228);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                            }
                        }
                    } else {
                        a.at(0x1037d0c4);
                        if(a.at_least()) {
                            a.at(0x1037d2e4);a.compare(20,0x14,true);
                            if(!a.at_least()) {
                                a.at(0x1037d2ec);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x48,false);a.exclusive_flags(0,0,0x80000000);
                                a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                a.floating_product(0,30,0);a.floating_compare(31,0);
                                if(!a.at_least()) {
                                    a.at(0x1037d324);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                    a.floating_round_single(31,31);
                                }
                            }
                            a.at(0x1037d334);a.compare(20,0x18,true);
                            if(a.less()) {
                                a.at(0x1037d34c);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e8,false);a.exclusive_flags(0,0,0x80000000);
                                a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                a.floating_product(0,30,0);a.floating_compare(31,0);
                                if(!a.at_least()) {
                                    a.at(0x1037d384);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                    a.floating_round_single(31,31);a.jump(0x1037d57c);
                                }
                            } else {
                                a.at(0x1037d33c);a.move(3,16);a.call(0x100de260);a.sign8(0,3,true);
                                if(!a.equal()) {
                                    a.at(0x1037d398);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e0,false);a.exclusive_flags(0,0,0x80000000);
                                    a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                    a.floating_product(0,30,0);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037d3d0);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                                    }
                                } else {
                                    a.at(0x1037d34c);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e8,false);a.exclusive_flags(0,0,0x80000000);
                                    a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                    a.floating_product(0,30,0);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037d384);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                                    }
                                }
                            }
                        } else {
                            a.at(0x1037d0c8);a.compare(19,0x1,true);
                            if(a.at_least()) {
                                a.at(0x1037d0e4);a.compare(20,0x14,true);
                                if(a.at_least()) {
                                    a.at(0x1037d138);a.compare(20,0x18,true);
                                    if(a.less()) {
                                        a.at(0x1037d150);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                        a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x118,false);a.exclusive_flags(0,0,0x80000000);
                                        a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                        a.floating_product(0,30,0);a.floating_compare(31,0);
                                        if(!a.at_least()) {
                                            a.at(0x1037d188);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                            a.floating_round_single(31,31);a.jump(0x1037d57c);
                                        }
                                    } else {
                                        a.at(0x1037d140);a.move(3,16);a.call(0x100de260);a.sign8(0,3,true);
                                        if(!a.equal()) {
                                            a.at(0x1037d19c);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                            a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x130,false);a.exclusive_flags(0,0,0x80000000);
                                            a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                            a.floating_product(0,30,0);a.floating_compare(31,0);
                                            if(!a.at_least()) {
                                                a.at(0x1037d1d4);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                                            }
                                        } else {
                                            a.at(0x1037d150);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                            a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x118,false);a.exclusive_flags(0,0,0x80000000);
                                            a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                            a.floating_product(0,30,0);a.floating_compare(31,0);
                                            if(!a.at_least()) {
                                                a.at(0x1037d188);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                                            }
                                        }
                                    }
                                } else {
                                    a.at(0x1037d0ec);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e0,false);a.exclusive_flags(0,0,0x80000000);
                                    a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                    a.floating_product(0,30,0);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037d124);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                                    }
                                }
                            } else {
                                a.at(0x1037d0d0);a.jump(0x1037d57c);
                            }
                        }
                    }
                }
            }
            a.at(0x1037d57c);a.load<4>(3,31,0x0,false);a.move(4,15);a.call(0x10082f20);
            a.compare(3,0x0,false);
            if(!a.equal()) {
                a.at(0x1037d590);a.load<4>(0,3,0x50,false);a.mask(0,0,0x10,true,0);
                if(!a.equal()) {
                    a.at(0x1037d59c);a.compare(19,0x4,true);
                    if(a.equal()) {
                        a.at(0x1037d5ec);a.floating_load(0,29,0x200,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                        a.jump(0x1037d618);
                    } else {
                        a.at(0x1037d5a4);
                        if(a.at_least()) {
                            a.at(0x1037d5bc);a.compare(19,0x6,true);
                            if(a.equal()) {
                                a.at(0x1037d60c);a.floating_load(0,29,0x208,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                            } else {
                                a.at(0x1037d5c4);
                                if(!a.at_least()) {
                                    a.at(0x1037d5c8);a.jump(0x1037d5fc);a.at(0x1037d5fc);a.floating_load(0,29,0x110,false);
                                    a.floating_product(31,0,31);a.floating_round_single(31,31);a.jump(0x1037d618);
                                }
                            }
                        } else {
                            a.at(0x1037d5a8);a.compare(19,0x3,true);
                            if(a.at_least()) {
                                a.at(0x1037d5dc);a.floating_load(0,29,0x38,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                a.jump(0x1037d618);
                            } else {
                                a.at(0x1037d5b0);a.compare(19,0x1,true);
                                if(a.at_least()) {
                                    a.at(0x1037d5cc);a.floating_load(0,29,0x1f8,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                    a.jump(0x1037d618);
                                } else {
                                    a.at(0x1037d5b8);a.jump(0x1037d618);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    a.at(0x1037d618);a.compare(16,0x0,false);
    if(!a.equal()) {
        a.at(0x1037d620);a.load<4>(3,16,0x2c,false);a.compare(3,0x0,false);
        if(!a.equal()) {
            a.at(0x1037d62c);a.load<4>(4,15,0x24,false);a.compare(4,0x0,false);
            if(a.equal()) {
                a.at(0x1037d650);a.compare(4,0x0,false);
                if(a.equal()) {
                    return check_quote_home_country(a);
                } else {
                    a.at(0x1037d658);a.load<4>(0,4,0x2c,false);a.compare_registers(0,3,false);
                    if(!a.equal()) {
                        return check_quote_home_country(a);
                    } else {
                        a.at(0x1037d664);a.load<4>(0,15,0xc,false);a.compare_registers(0,3,false);
                        if(a.equal()) {
                            return check_quote_home_country(a);
                        } else {
                            a.at(0x1037d670);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0x3,true);
                            if(a.positive()) {
                                return apply_quote_country_rules(a);
                            } else {
                                a.at(0x1037d680);a.load<1>(0,15,0x63,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                                if(a.less()) {
                                    return apply_quote_country_rules(a);
                                } else {
                                    return check_quote_home_country(a);
                                }
                            }
                        }
                    }
                }
            } else {
                a.at(0x1037d638);a.load<4>(0,4,0x2c,false);a.compare_registers(0,3,false);
                if(!a.equal()) {
                    a.at(0x1037d650);a.compare(4,0x0,false);
                    if(a.equal()) {
                        return check_quote_home_country(a);
                    } else {
                        a.at(0x1037d658);a.load<4>(0,4,0x2c,false);a.compare_registers(0,3,false);
                        if(!a.equal()) {
                            return check_quote_home_country(a);
                        } else {
                            a.at(0x1037d664);a.load<4>(0,15,0xc,false);a.compare_registers(0,3,false);
                            if(a.equal()) {
                                return check_quote_home_country(a);
                            } else {
                                a.at(0x1037d670);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0x3,true);
                                if(a.positive()) {
                                    return apply_quote_country_rules(a);
                                } else {
                                    a.at(0x1037d680);a.load<1>(0,15,0x63,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                                    if(a.less()) {
                                        return apply_quote_country_rules(a);
                                    } else {
                                        return check_quote_home_country(a);
                                    }
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x1037d644);a.load<4>(0,15,0xc,false);a.compare_registers(0,3,false);
                    if(a.equal()) {
                        return apply_quote_country_rules(a);
                    } else {
                        a.at(0x1037d650);a.compare(4,0x0,false);
                        if(a.equal()) {
                            return check_quote_home_country(a);
                        } else {
                            a.at(0x1037d658);a.load<4>(0,4,0x2c,false);a.compare_registers(0,3,false);
                            if(!a.equal()) {
                                return check_quote_home_country(a);
                            } else {
                                a.at(0x1037d664);a.load<4>(0,15,0xc,false);a.compare_registers(0,3,false);
                                if(a.equal()) {
                                    return check_quote_home_country(a);
                                } else {
                                    a.at(0x1037d670);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0x3,true);
                                    if(a.positive()) {
                                        return apply_quote_country_rules(a);
                                    } else {
                                        a.at(0x1037d680);a.load<1>(0,15,0x63,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                                        if(a.less()) {
                                            return apply_quote_country_rules(a);
                                        } else {
                                            return check_quote_home_country(a);
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
    return finish_quote_contract_limits(a);
}

void choose_profile_quote_baseline(Call& a) {
    a.at(0x1037c424);a.compare(20,0x18,true);
    if(a.positive()) {
        return choose_profile_quote_category(a);
    } else {
        a.at(0x1037c42c);a.load<2>(3,24,0x6,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
        a.multiply(0,3,0x32);a.floating_load(1,29,0xc0,false);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x3c,0);
        a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_compare(0,4);
        if(a.nonpositive()) {
            return choose_profile_quote_category(a);
        } else {
            a.at(0x1037c458);a.value(18,0x5);a.jump(0x1037c474);
        }
    }
    return apply_quote_mode(a);
}

void choose_profile_quote_category(Call& a) {
    a.at(0x1037c460);a.compare(20,0x18,true);
    if(a.positive()) {
        a.at(0x1037c470);a.value(18,0x4);
    } else {
        a.at(0x1037c468);a.value(18,0x6);a.jump(0x1037c474);
    }
    return apply_quote_mode(a);
}

void apply_quote_mode(Call& a) {
    a.at(0x1037c474);a.sign8(0,19,false);a.compare(0,0x3,true);
    if(!a.equal()) {
        a.at(0x1037c488);a.compare(0,0x1,true);
        if(!a.equal()) {
            a.at(0x1037c4c0);a.compare(0,0x2,true);
            if(!a.equal()) {
                a.at(0x1037c4e8);a.sign8(0,22,false);a.compare(0,0xd,true);
                if(a.equal()) {
                    a.at(0x1037c4fc);a.compare(20,0x23,true);
                    if(a.nonpositive()) {
                        a.at(0x1037c50c);a.sign8(0,18,false);a.compare(0,0x6,true);
                        if(a.equal()) {
                            a.at(0x1037c520);a.load<2>(0,24,0xa,true);a.compare(0,0x9c4,true);
                            if(a.at_least()) {
                                return check_quote_category_threshold(a);
                            } else {
                                a.at(0x1037c52c);a.floating_load(31,30,0x13c,true);a.jump(0x1037c6ac);
                            }
                        } else {
                            a.at(0x1037c518);a.compare(0,0x5,true);
                            if(!a.equal()) {
                                return check_quote_category_threshold(a);
                            } else {
                                a.at(0x1037c520);a.load<2>(0,24,0xa,true);a.compare(0,0x9c4,true);
                                if(a.at_least()) {
                                    return check_quote_category_threshold(a);
                                } else {
                                    a.at(0x1037c52c);a.floating_load(31,30,0x13c,true);a.jump(0x1037c6ac);
                                }
                            }
                        }
                    } else {
                        a.at(0x1037c504);a.floating_load(31,30,0x14,true);a.jump(0x1037c6ac);
                    }
                } else {
                    a.at(0x1037c4f4);a.compare(0,0xf,true);
                    if(!a.equal()) {
                        a.at(0x1037c50c);a.sign8(0,18,false);a.compare(0,0x6,true);
                        if(a.equal()) {
                            a.at(0x1037c520);a.load<2>(0,24,0xa,true);a.compare(0,0x9c4,true);
                            if(a.at_least()) {
                                return check_quote_category_threshold(a);
                            } else {
                                a.at(0x1037c52c);a.floating_load(31,30,0x13c,true);a.jump(0x1037c6ac);
                            }
                        } else {
                            a.at(0x1037c518);a.compare(0,0x5,true);
                            if(!a.equal()) {
                                return check_quote_category_threshold(a);
                            } else {
                                a.at(0x1037c520);a.load<2>(0,24,0xa,true);a.compare(0,0x9c4,true);
                                if(a.at_least()) {
                                    return check_quote_category_threshold(a);
                                } else {
                                    a.at(0x1037c52c);a.floating_load(31,30,0x13c,true);a.jump(0x1037c6ac);
                                }
                            }
                        }
                    } else {
                        a.at(0x1037c4fc);a.compare(20,0x23,true);
                        if(a.nonpositive()) {
                            a.at(0x1037c50c);a.sign8(0,18,false);a.compare(0,0x6,true);
                            if(a.equal()) {
                                a.at(0x1037c520);a.load<2>(0,24,0xa,true);a.compare(0,0x9c4,true);
                                if(a.at_least()) {
                                    return check_quote_category_threshold(a);
                                } else {
                                    a.at(0x1037c52c);a.floating_load(31,30,0x13c,true);a.jump(0x1037c6ac);
                                }
                            } else {
                                a.at(0x1037c518);a.compare(0,0x5,true);
                                if(!a.equal()) {
                                    return check_quote_category_threshold(a);
                                } else {
                                    a.at(0x1037c520);a.load<2>(0,24,0xa,true);a.compare(0,0x9c4,true);
                                    if(a.at_least()) {
                                        return check_quote_category_threshold(a);
                                    } else {
                                        a.at(0x1037c52c);a.floating_load(31,30,0x13c,true);a.jump(0x1037c6ac);
                                    }
                                }
                            }
                        } else {
                            a.at(0x1037c504);a.floating_load(31,30,0x14,true);a.jump(0x1037c6ac);
                        }
                    }
                }
            } else {
                a.at(0x1037c4c8);a.compare(20,0x16,true);
                if(a.nonpositive()) {
                    a.at(0x1037c4e0);a.floating_load(31,30,0x138,true);a.jump(0x1037c6ac);
                } else {
                    a.at(0x1037c4d0);a.compare(20,0x20,true);
                    if(a.at_least()) {
                        a.at(0x1037c4e0);a.floating_load(31,30,0x138,true);a.jump(0x1037c6ac);
                    } else {
                        a.at(0x1037c4d8);a.floating_load(31,30,0x6c,true);a.jump(0x1037c6ac);
                    }
                }
            }
        } else {
            a.at(0x1037c490);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0xf,true);
            if(a.at_least()) {
                a.at(0x1037c4b8);a.floating_load(31,30,0x6c,true);a.jump(0x1037c6ac);
            } else {
                a.at(0x1037c4a0);a.load<1>(0,15,0x66,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                if(a.nonpositive()) {
                    a.at(0x1037c4b8);a.floating_load(31,30,0x6c,true);a.jump(0x1037c6ac);
                } else {
                    a.at(0x1037c4b0);a.floating_load(31,30,0x134,true);a.jump(0x1037c6ac);
                }
            }
        }
    } else {
        a.at(0x1037c480);a.floating_load(31,30,0x18,true);a.jump(0x1037c6ac);
    }
    return adjust_quote_club_context(a);
}

void check_quote_category_threshold(Call& a) {
    a.at(0x1037c534);a.sign8(0,18,false);a.compare(0,0x6,true);
    if(a.equal()) {
        return check_quote_profile_floor(a);
    } else {
        a.at(0x1037c540);a.compare(0,0x5,true);
        if(!a.equal()) {
            return compare_primary_profile_thresholds(a);
        } else {
            return check_quote_profile_floor(a);
        }
    }
}

void check_quote_profile_floor(Call& a) {
    a.at(0x1037c548);a.load<2>(0,24,0xa,true);a.compare(0,0x186a,true);
    if(a.at_least()) {
        return compare_primary_profile_thresholds(a);
    } else {
        a.at(0x1037c554);a.floating_load(31,30,0x10c,true);a.jump(0x1037c6ac);
    }
    return adjust_quote_club_context(a);
}

void compare_primary_profile_thresholds(Call& a) {
    a.at(0x1037c55c);a.load<1>(0,24,0x15,false);a.sign8(0,0,false);a.compare(0,0xf,true);
    if(a.less()) {
        a.at(0x1037c5a8);a.load<1>(0,24,0x12,false);a.sign8(0,0,false);a.compare(0,0xf,true);
        if(a.at_least()) {
            return check_secondary_profile_thresholds(a);
        } else {
            a.at(0x1037c5b8);a.load<1>(0,24,0x14,false);a.sign8(0,0,false);a.compare(0,0xf,true);
            if(a.at_least()) {
                return check_secondary_profile_thresholds(a);
            } else {
                a.at(0x1037c5c8);a.load<1>(0,24,0x13,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                if(a.less()) {
                    a.at(0x1037c614);a.load<1>(0,24,0xf,false);a.compare(0,0x14,true);
                    if(!a.equal()) {
                        a.at(0x1037c654);a.load<1>(0,24,0x10,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                        if(a.at_least()) {
                            return check_specialist_profile_thresholds(a);
                        } else {
                            a.at(0x1037c664);a.load<1>(0,24,0x11,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                            if(a.less()) {
                                a.at(0x1037c6a8);a.floating_load(31,30,0x158,true);
                            } else {
                                return check_specialist_profile_thresholds(a);
                            }
                        }
                    } else {
                        a.at(0x1037c620);a.load<2>(0,24,0xc,true);a.compare(0,0x1c52,true);
                        if(a.nonpositive()) {
                            return use_specialist_profile_quote(a);
                        } else {
                            a.at(0x1037c62c);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0x3,true);
                            if(a.at_least()) {
                                return use_specialist_profile_quote(a);
                            } else {
                                a.at(0x1037c63c);a.compare(20,0x22,true);
                                if(a.at_least()) {
                                    return use_specialist_profile_quote(a);
                                } else {
                                    a.at(0x1037c644);a.floating_load(31,30,0x150,true);a.jump(0x1037c6ac);
                                }
                            }
                        }
                    }
                } else {
                    return check_secondary_profile_thresholds(a);
                }
            }
        }
    } else {
        a.at(0x1037c56c);a.load<2>(0,24,0xa,true);a.compare(0,0x1a5e,true);
        if(a.nonpositive()) {
            return use_primary_profile_quote(a);
        } else {
            a.at(0x1037c578);a.compare(20,0x1f,true);
            if(a.at_least()) {
                return use_primary_profile_quote(a);
            } else {
                a.at(0x1037c580);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                if(a.at_least()) {
                    a.at(0x1037c598);a.floating_load(31,30,0x140,true);a.jump(0x1037c6ac);
                } else {
                    a.at(0x1037c590);a.floating_load(31,30,0x138,true);a.jump(0x1037c6ac);
                }
            }
        }
    }
    return adjust_quote_club_context(a);
}

void use_primary_profile_quote(Call& a) {
    a.at(0x1037c5a0);a.floating_load(31,30,0xf0,true);a.jump(0x1037c6ac);
    return adjust_quote_club_context(a);
}

void check_secondary_profile_thresholds(Call& a) {
    a.at(0x1037c5d8);a.load<2>(0,24,0xa,true);a.compare(0,0x1c52,true);
    if(a.nonpositive()) {
        return use_secondary_profile_quote(a);
    } else {
        a.at(0x1037c5e4);a.compare(20,0x1f,true);
        if(a.at_least()) {
            return use_secondary_profile_quote(a);
        } else {
            a.at(0x1037c5ec);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0xe,true);
            if(a.at_least()) {
                a.at(0x1037c604);a.floating_load(31,30,0x148,true);a.jump(0x1037c6ac);
            } else {
                a.at(0x1037c5fc);a.floating_load(31,30,0x144,true);a.jump(0x1037c6ac);
            }
        }
    }
    return adjust_quote_club_context(a);
}

void use_secondary_profile_quote(Call& a) {
    a.at(0x1037c60c);a.floating_load(31,30,0x14c,true);a.jump(0x1037c6ac);
    return adjust_quote_club_context(a);
}

void use_specialist_profile_quote(Call& a) {
    a.at(0x1037c64c);a.floating_load(31,30,0x154,true);a.jump(0x1037c6ac);
    return adjust_quote_club_context(a);
}

void check_specialist_profile_thresholds(Call& a) {
    a.at(0x1037c674);a.load<2>(0,24,0xa,true);a.compare(0,0x1d4c,true);
    if(a.nonpositive()) {
        return use_fallback_profile_quote(a);
    } else {
        a.at(0x1037c680);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0x6,true);
        if(a.at_least()) {
            return use_fallback_profile_quote(a);
        } else {
            a.at(0x1037c690);a.compare(20,0x21,true);
            if(a.at_least()) {
                return use_fallback_profile_quote(a);
            } else {
                a.at(0x1037c698);a.floating_load(31,30,0xf4,true);a.jump(0x1037c6ac);
            }
        }
    }
    return adjust_quote_club_context(a);
}

void use_fallback_profile_quote(Call& a) {
    a.at(0x1037c6a0);a.floating_load(31,30,0x158,true);a.jump(0x1037c6ac);
    return adjust_quote_club_context(a);
}

void adjust_quote_club_context(Call& a) {
    a.at(0x1037c6ac);a.compare(16,0x0,false);a.value(5,0x1);
    if(!a.equal()) {
        a.at(0x1037c6b8);a.load<4>(3,16,0x2c,false);a.compare(3,0x0,false);
        if(!a.equal()) {
            a.at(0x1037c6c4);a.load<2>(0,16,0x1ec,true);a.compare(0,0x1676,true);
            if(a.less()) {
                a.at(0x1037c6e0);a.load<4>(4,15,0x24,false);a.compare_registers(4,16,false);
                if(!a.equal()) {
                    a.at(0x1037c6f4);a.compare(4,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1037c6fc);a.load<4>(0,4,0x2c,false);a.compare_registers(0,3,false);
                        if(a.equal()) {
                            a.at(0x1037c708);a.value(5,0x0);
                        }
                    }
                } else {
                    a.at(0x1037c6ec);a.value(5,0x0);a.jump(0x1037c70c);
                }
            } else {
                a.at(0x1037c6d0);a.load<1>(0,3,0x11f,false);a.sign8(0,0,false);a.compare(0,0xc,true);
                if(!a.at_least()) {
                    a.at(0x1037c6e0);a.load<4>(4,15,0x24,false);a.compare_registers(4,16,false);
                    if(!a.equal()) {
                        a.at(0x1037c6f4);a.compare(4,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1037c6fc);a.load<4>(0,4,0x2c,false);a.compare_registers(0,3,false);
                            if(a.equal()) {
                                a.at(0x1037c708);a.value(5,0x0);
                            }
                        }
                    } else {
                        a.at(0x1037c6ec);a.value(5,0x0);a.jump(0x1037c70c);
                    }
                }
            }
        }
    }
    a.at(0x1037c70c);a.compare(20,0x12,true);
    if(!a.nonpositive()) {
        a.at(0x1037c714);a.compare(20,0x20,true);
        if(!a.at_least()) {
            a.at(0x1037c71c);a.sign8(0,5,true);
            if(!a.equal()) {
                a.at(0x1037c724);a.load<1>(3,15,0x63,false);a.load<1>(0,15,0x65,false);a.sign8(3,3,false);
                a.load<2>(4,24,0xc,true);a.multiply(3,3,0xa);a.sign8(0,0,false);a.multiply(0,0,0x19);
                a.subtract_from(3,3,0x1676);a.add(0,3,0);a.compare_registers(4,0,true);
                if(!a.nonpositive()) {
                    a.at(0x1037c750);a.load<1>(0,24,0xf,false);a.compare(0,0x14,true);
                    if(!a.equal()) {
                        a.at(0x1037c770);a.floating_load(1,30,0x134,true);a.floating_compare(1,31);
                        if(!a.equal()) {
                            a.at(0x1037c784);a.floating_load(0,30,0x6c,true);a.floating_compare(0,31);
                            if(!a.equal()) {
                                a.at(0x1037c7a8);a.compare(4,0x1c52,true);
                                if(a.nonpositive()) {
                                    a.at(0x1037c7c0);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037c7c8);a.floating_move(31,0);
                                    }
                                } else {
                                    a.at(0x1037c7b0);a.floating_compare(31,1);
                                    if(!a.at_least()) {
                                        a.at(0x1037c7b8);a.floating_move(31,1);a.jump(0x1037c7cc);
                                    }
                                }
                            } else {
                                a.at(0x1037c790);a.compare(4,0x1c52,true);
                                if(a.nonpositive()) {
                                    a.at(0x1037c7a0);a.floating_move(31,1);a.jump(0x1037c7cc);
                                } else {
                                    a.at(0x1037c798);a.floating_load(31,30,0x1c,true);a.jump(0x1037c7cc);
                                }
                            }
                        } else {
                            a.at(0x1037c77c);a.floating_load(31,30,0x1c,true);a.jump(0x1037c7cc);
                        }
                    } else {
                        a.at(0x1037c75c);a.floating_load(0,30,0x6c,true);a.floating_compare(31,0);
                        if(!a.at_least()) {
                            a.at(0x1037c768);a.floating_move(31,0);a.jump(0x1037c7cc);
                        }
                    }
                }
            }
        }
    }
    a.at(0x1037c7cc);a.load<1>(0,26,0x0,false);a.compare(0,0x1,true);
    if(a.equal()) {
        a.at(0x1037c7d8);a.floating_load(0,30,0x10c,true);a.floating_compare(31,0);
        if(!a.nonpositive()) {
            a.at(0x1037c7e4);a.compare(16,0x0,false);
            if(!a.equal()) {
                a.at(0x1037c7ec);a.move(3,16);a.call(0x100de260);a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x1037c7fc);a.load<2>(0,24,0xa,true);a.compare(0,0x1a5e,true);
                    if(a.nonpositive()) {
                        a.at(0x1037c850);a.compare(0,0x109a,true);
                        if(!a.nonpositive()) {
                            a.at(0x1037c858);a.floating_load(0,29,0x1b8,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                        }
                    } else {
                        a.at(0x1037c808);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0x1,true);
                        if(!a.equal()) {
                            a.at(0x1037c828);a.compare(0,0x6,true);
                            if(a.at_least()) {
                                a.at(0x1037c840);a.floating_load(0,29,0x158,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                a.jump(0x1037c864);
                            } else {
                                a.at(0x1037c830);a.floating_load(0,29,0x1b0,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                a.jump(0x1037c864);
                            }
                        } else {
                            a.at(0x1037c818);a.floating_load(0,29,0x150,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                            a.jump(0x1037c864);
                        }
                    }
                }
            }
        }
    }
    a.at(0x1037c864);a.load<2>(3,24,0xa,true);a.load<2>(0,24,0x8,true);a.move(4,3);
    a.compare_registers(0,3,true);
    if(!a.nonpositive()) {
        a.at(0x1037c878);a.move(4,0);
    }
    a.at(0x1037c87c);a.load<2>(23,24,0xc,true);a.sign16(5,4,false);a.compare_registers(23,5,true);
    if(!a.nonpositive()) {
        a.at(0x1037c88c);a.move(4,23);
    }
    a.at(0x1037c890);a.compare(23,0x6d6,true);
    if(!a.at_least()) {
        a.at(0x1037c898);a.sign16(5,4,false);a.compare(5,0xcb2,true);
        if(!a.nonpositive()) {
            a.at(0x1037c8a4);a.exclusive_flags(5,5,0x80000000);a.value(4,0x43300000);a.store<4>(1,0x3c,5);
            a.floating_load(2,29,0xc0,false);a.store<4>(1,0x38,4);a.floating_load(0,29,0xb0,false);a.floating_load(1,1,0x38,false);
            a.floating_load(3,29,0xa0,false);a.floating_subtract(1,1,2,false);a.floating_subtract(0,1,0,false);a.floating_product(0,3,0);
            a.floating_integer(0,0);a.floating_store(0,1,0x40);a.load<4>(4,1,0x44,false);a.offset(4,4,0xcb2);
            a.sign16(4,4,false);
        }
    }
    a.at(0x1037c8e4);a.sign8(5,19,false);a.compare(5,0x1,true);
    if(!a.equal()) {
        a.at(0x1037c904);a.compare(5,0x2,true);
        if(!a.equal()) {
            a.at(0x1037c920);a.offset(5,5,-0x10000);a.compare(5,0x4244,false);
            if(a.equal()) {
                a.at(0x1037c92c);a.sign16(5,4,false);a.compare(5,0x1964,true);
                if(!a.at_least()) {
                    a.at(0x1037c938);a.value(4,0x1964);
                }
            }
        } else {
            a.at(0x1037c90c);a.sign16(5,4,false);a.compare(5,0x1a5e,true);
            if(!a.at_least()) {
                a.at(0x1037c918);a.value(4,0x1a5e);a.jump(0x1037c93c);
            }
        }
    } else {
        a.at(0x1037c8f0);a.sign16(5,4,false);a.compare(5,0x1c52,true);
        if(!a.at_least()) {
            a.at(0x1037c8fc);a.value(4,0x1c52);a.jump(0x1037c93c);
        }
    }
    a.at(0x1037c93c);a.floating_load(0,30,0x6c,true);a.floating_compare(0,31);
    if(a.equal()) {
        a.at(0x1037c960);a.sign16(5,4,false);a.compare(5,0x1c52,true);
        if(!a.at_least()) {
            a.at(0x1037c96c);a.value(4,0x1c52);
        }
    } else {
        a.at(0x1037c948);a.floating_load(0,30,0x134,true);a.floating_compare(0,31);
        if(a.equal()) {
            a.at(0x1037c960);a.sign16(5,4,false);a.compare(5,0x1c52,true);
            if(!a.at_least()) {
                a.at(0x1037c96c);a.value(4,0x1c52);
            }
        } else {
            a.at(0x1037c954);a.floating_load(0,30,0x1c,true);a.floating_compare(0,31);
            if(a.equal()) {
                a.at(0x1037c960);a.sign16(5,4,false);a.compare(5,0x1c52,true);
                if(!a.at_least()) {
                    a.at(0x1037c96c);a.value(4,0x1c52);
                }
            }
        }
    }
    a.at(0x1037c970);a.load<2>(6,24,0x4,true);a.sign16(5,4,false);a.load<2>(8,24,0x6,true);
    a.value(4,0x38e40000);a.multiply(7,6,0x32);a.add(6,0,23);a.mask(0,5,0xfffffffc,false,2);
    a.floating_load(1,30,0x110,true);a.floating_load(0,30,0x40,true);a.offset(5,4,-0x71c7);a.add(0,6,0);
    a.product_difference_single_at(0x1037c99c,1,1,30,0);a.multiply(4,8,0x32);a.add(0,0,7);a.add(0,0,4);
    a.add(0,3,0);a.multiply_high(0,5,0);a.shift_right(0,0,1);a.mask(3,0,0x1,false,1);
    a.add(19,0,3);a.call(0x10773780);a.load<4>(22,15,0xc,false);a.add(19,19,3);
    a.compare(22,0x0,false);
    if(!a.equal()) {
        a.at(0x1037c9d4);a.load<4>(3,16,0x2c,false);a.compare(3,0x0,false);
        if(!a.equal()) {
            a.at(0x1037c9e0);a.load<1>(0,22,0x118,false);a.load<1>(3,3,0x118,false);a.sign8(0,0,false);
            a.sign8(3,3,false);a.compare_registers(0,3,true);
            if(!a.equal()) {
                a.at(0x1037c9f8);a.compare(25,0x0,false);
                if(a.equal()) {
                    a.at(0x1037ca1c);a.compare(19,0x5dc,false);
                    if(a.at_least()) {
                        a.at(0x1037ca2c);a.value(0,0x43300000);a.store<4>(1,0x3c,19);a.floating_load(1,29,0xc8,false);
                        a.store<4>(1,0x38,0);a.floating_load(2,29,0x198,false);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);
                        a.floating_product(1,2,0);a.call(0x10773780);a.move(19,3);
                    } else {
                        a.at(0x1037ca24);a.value(19,0x5dc);a.jump(0x1037ca54);
                    }
                } else {
                    a.at(0x1037ca00);a.load<4>(4,25,0x2c,false);a.compare(4,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1037ca0c);a.load<1>(0,4,0x118,false);a.sign8(0,0,false);a.compare_registers(0,3,true);
                        if(!a.equal()) {
                            a.at(0x1037ca1c);a.compare(19,0x5dc,false);
                            if(a.at_least()) {
                                a.at(0x1037ca2c);a.value(0,0x43300000);a.store<4>(1,0x3c,19);a.floating_load(1,29,0xc8,false);
                                a.store<4>(1,0x38,0);a.floating_load(2,29,0x198,false);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);
                                a.floating_product(1,2,0);a.call(0x10773780);a.move(19,3);
                            } else {
                                a.at(0x1037ca24);a.value(19,0x5dc);a.jump(0x1037ca54);
                            }
                        }
                    }
                }
            }
        }
    }
    a.at(0x1037ca54);a.compare(19,0x1,false);a.value(0,0x1);
    if(!a.nonpositive()) {
        a.at(0x1037ca60);a.move(0,19);
    }
    a.at(0x1037ca64);a.floating_load(0,29,0x1c0,false);a.product(3,0,0);a.value(0,0x43300000);
    a.floating_load(2,29,0xc8,false);a.floating_difference(3,31,0,true,false);a.store<4>(1,0x38,0);a.floating_load(0,30,0x114,true);
    a.store<4>(1,0x3c,3);a.floating_load(1,1,0x38,false);a.floating_subtract(1,1,2,false);a.floating_product(31,3,1);
    a.floating_round_single(31,31);a.floating_compare(31,0);
    if(!a.at_least()) {
        a.at(0x1037ca9c);a.floating_move(31,0);
    }
    a.at(0x1037caa0);a.compare(25,0x0,false);
    if(a.equal()) {
        a.at(0x1037cabc);a.compare(23,0x109a,true);
        if(a.nonpositive()) {
            a.at(0x1037cacc);a.compare(22,0x0,false);
            if(a.equal()) {
                a.at(0x1037cadc);a.value(0,0x3);
            } else {
                a.at(0x1037cad4);a.load<1>(0,22,0x11c,false);a.jump(0x1037cae0);
            }
        } else {
            a.at(0x1037cac4);a.value(0,0x1);a.jump(0x1037cae0);
        }
    } else {
        a.at(0x1037caa8);a.load<4>(3,25,0x2c,false);a.compare(3,0x0,false);
        if(a.equal()) {
            a.at(0x1037cabc);a.compare(23,0x109a,true);
            if(a.nonpositive()) {
                a.at(0x1037cacc);a.compare(22,0x0,false);
                if(a.equal()) {
                    a.at(0x1037cadc);a.value(0,0x3);
                } else {
                    a.at(0x1037cad4);a.load<1>(0,22,0x11c,false);a.jump(0x1037cae0);
                }
            } else {
                a.at(0x1037cac4);a.value(0,0x1);a.jump(0x1037cae0);
            }
        } else {
            a.at(0x1037cab4);a.load<1>(0,3,0x11c,false);a.jump(0x1037cae0);
        }
    }
    a.at(0x1037cae0);a.sign8(0,0,false);a.compare(0,0x2,true);
    if(a.equal()) {
        a.at(0x1037cb04);a.floating_load(1,30,0x54,true);a.floating_load(0,30,0x0,true);a.floating_product_single(1,1,31);
        a.floating_product_single(31,1,0);a.jump(0x1037cb20);
    } else {
        a.at(0x1037caec);
        if(a.at_least()) {
            a.at(0x1037cb18);a.floating_load(0,30,0x98,true);a.floating_product_single(31,31,0);
        } else {
            a.at(0x1037caf0);a.compare(0,0x1,true);
            if(!a.at_least()) {
                a.at(0x1037caf8);a.jump(0x1037cb18);a.at(0x1037cb18);a.floating_load(0,30,0x98,true);
                a.floating_product_single(31,31,0);
            }
        }
    }
    a.at(0x1037cb20);a.compare(16,0x0,false);
    if(!a.equal()) {
        a.at(0x1037cb28);a.load<2>(3,16,0x1ec,true);a.sign16(0,3,true);
        if(!a.equal()) {
            a.at(0x1037cb34);a.compare(3,0x9c4,true);
            if(a.at_least()) {
                a.at(0x1037cb64);a.multiply(0,21,0x1f4);a.value(3,0x43300000);a.exclusive_flags(4,23,0x80000000);
                a.store<4>(1,0x40,3);a.floating_load(2,29,0xc0,false);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x44,4);
                a.floating_load(0,1,0x40,false);a.store<4>(1,0x4c,0);a.floating_subtract(1,0,2,true);a.store<4>(1,0x48,3);
                a.floating_load(0,1,0x48,false);a.floating_subtract(0,0,2,true);a.floating_difference(1,1,0,true,true);
            } else {
                a.at(0x1037cb3c);a.exclusive_flags(3,23,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
                a.floating_load(2,29,0xc0,false);a.store<4>(1,0x38,0);a.floating_load(0,30,0x80,true);a.floating_load(1,1,0x38,false);
                a.floating_subtract(1,1,2,true);a.floating_difference(1,1,0,true,true);a.jump(0x1037cb9c);
            }
            a.at(0x1037cb9c);a.floating_load(0,29,0x38,false);a.floating_compare(1,0);
            if(!a.at_least()) {
                a.at(0x1037cba8);a.floating_load(1,30,0x118,true);
            }
            a.at(0x1037cbac);a.floating_load(0,29,0x150,false);a.floating_compare(1,0);
            if(!a.nonpositive()) {
                a.at(0x1037cbb8);a.floating_load(1,30,0x4c,true);
            }
            a.at(0x1037cbbc);a.floating_product_single(31,31,1);
        }
    }
    a.at(0x1037cbc0);a.compare(21,0xa,true);
    if(a.at_least()) {
        a.at(0x1037cc04);a.compare(21,0xf,true);
        if(a.at_least()) {
            a.at(0x1037cc48);a.load<4>(3,16,0x2c,false);a.compare(3,0x0,false);
            if(a.equal()) {
                a.at(0x1037cc7c);a.multiply(3,21,0xcb2);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                a.floating_load(3,29,0xc0,false);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x38,false);
                a.floating_subtract(0,0,3,true);a.floating_compare(31,0);
                if(!a.nonpositive()) {
                    a.at(0x1037cca4);a.store<4>(1,0x3c,3);a.floating_load(2,29,0x88,false);a.store<4>(1,0x38,0);
                    a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,3);a.floating_subtract(1,0,3,true);a.store<4>(1,0x40,0);
                    a.floating_load(0,1,0x40,false);a.floating_subtract(1,31,1,true);a.floating_subtract(0,0,3,false);a.floating_product_sum(31,2,1,0);
                    a.floating_round_single(31,31);a.jump(0x1037cdb0);
                }
            } else {
                a.at(0x1037cc54);a.load<4>(3,3,0x68,false);a.load<4>(0,13,0x0,false);a.compare_registers(3,0,true);
                if(a.equal()) {
                    a.at(0x1037ccd8);a.load<1>(0,16,0x241,false);a.sign8(0,0,true);
                    if(a.equal()) {
                        a.at(0x1037cd40);a.multiply(3,21,0x1d4c);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                        a.floating_load(3,29,0xc0,false);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x38,false);
                        a.floating_subtract(0,0,3,true);a.floating_compare(31,0);
                        if(!a.nonpositive()) {
                            a.at(0x1037cd68);a.store<4>(1,0x3c,3);a.store<4>(1,0x38,0);a.floating_load(0,1,0x38,false);
                            a.floating_subtract(0,0,3,true);a.floating_compare(31,0);
                            if(!a.nonpositive()) {
                                a.at(0x1037cd80);a.store<4>(1,0x3c,3);a.floating_load(2,29,0x88,false);a.store<4>(1,0x38,0);
                                a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,3);a.floating_subtract(1,0,3,true);a.store<4>(1,0x40,0);
                                a.floating_load(0,1,0x40,false);a.floating_subtract(1,31,1,true);a.floating_subtract(0,0,3,false);a.floating_product_sum(31,2,1,0);
                                a.floating_round_single(31,31);
                            }
                        }
                    } else {
                        a.at(0x1037cce4);a.multiply(3,21,0x1482);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                        a.floating_load(3,29,0xc0,false);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x38,false);
                        a.floating_subtract(0,0,3,true);a.floating_compare(31,0);
                        if(!a.nonpositive()) {
                            a.at(0x1037cd0c);a.store<4>(1,0x3c,3);a.floating_load(2,29,0x88,false);a.store<4>(1,0x38,0);
                            a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,3);a.floating_subtract(1,0,3,true);a.store<4>(1,0x40,0);
                            a.floating_load(0,1,0x40,false);a.floating_subtract(1,31,1,true);a.floating_subtract(0,0,3,false);a.floating_product_sum(31,2,1,0);
                            a.floating_round_single(31,31);a.jump(0x1037cdb0);
                        }
                    }
                } else {
                    a.at(0x1037cc64);a.load<4>(0,28,0x0,false);a.compare_registers(3,0,true);
                    if(a.equal()) {
                        a.at(0x1037ccd8);a.load<1>(0,16,0x241,false);a.sign8(0,0,true);
                        if(a.equal()) {
                            a.at(0x1037cd40);a.multiply(3,21,0x1d4c);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                            a.floating_load(3,29,0xc0,false);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x38,false);
                            a.floating_subtract(0,0,3,true);a.floating_compare(31,0);
                            if(!a.nonpositive()) {
                                a.at(0x1037cd68);a.store<4>(1,0x3c,3);a.store<4>(1,0x38,0);a.floating_load(0,1,0x38,false);
                                a.floating_subtract(0,0,3,true);a.floating_compare(31,0);
                                if(!a.nonpositive()) {
                                    a.at(0x1037cd80);a.store<4>(1,0x3c,3);a.floating_load(2,29,0x88,false);a.store<4>(1,0x38,0);
                                    a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,3);a.floating_subtract(1,0,3,true);a.store<4>(1,0x40,0);
                                    a.floating_load(0,1,0x40,false);a.floating_subtract(1,31,1,true);a.floating_subtract(0,0,3,false);a.floating_product_sum(31,2,1,0);
                                    a.floating_round_single(31,31);
                                }
                            }
                        } else {
                            a.at(0x1037cce4);a.multiply(3,21,0x1482);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                            a.floating_load(3,29,0xc0,false);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x38,false);
                            a.floating_subtract(0,0,3,true);a.floating_compare(31,0);
                            if(!a.nonpositive()) {
                                a.at(0x1037cd0c);a.store<4>(1,0x3c,3);a.floating_load(2,29,0x88,false);a.store<4>(1,0x38,0);
                                a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,3);a.floating_subtract(1,0,3,true);a.store<4>(1,0x40,0);
                                a.floating_load(0,1,0x40,false);a.floating_subtract(1,31,1,true);a.floating_subtract(0,0,3,false);a.floating_product_sum(31,2,1,0);
                                a.floating_round_single(31,31);a.jump(0x1037cdb0);
                            }
                        }
                    } else {
                        a.at(0x1037cc70);a.load<4>(0,14,0x0,false);a.compare_registers(3,0,true);
                        if(a.equal()) {
                            a.at(0x1037ccd8);a.load<1>(0,16,0x241,false);a.sign8(0,0,true);
                            if(a.equal()) {
                                a.at(0x1037cd40);a.multiply(3,21,0x1d4c);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                a.floating_load(3,29,0xc0,false);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x38,false);
                                a.floating_subtract(0,0,3,true);a.floating_compare(31,0);
                                if(!a.nonpositive()) {
                                    a.at(0x1037cd68);a.store<4>(1,0x3c,3);a.store<4>(1,0x38,0);a.floating_load(0,1,0x38,false);
                                    a.floating_subtract(0,0,3,true);a.floating_compare(31,0);
                                    if(!a.nonpositive()) {
                                        a.at(0x1037cd80);a.store<4>(1,0x3c,3);a.floating_load(2,29,0x88,false);a.store<4>(1,0x38,0);
                                        a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,3);a.floating_subtract(1,0,3,true);a.store<4>(1,0x40,0);
                                        a.floating_load(0,1,0x40,false);a.floating_subtract(1,31,1,true);a.floating_subtract(0,0,3,false);a.floating_product_sum(31,2,1,0);
                                        a.floating_round_single(31,31);
                                    }
                                }
                            } else {
                                a.at(0x1037cce4);a.multiply(3,21,0x1482);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                a.floating_load(3,29,0xc0,false);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x38,false);
                                a.floating_subtract(0,0,3,true);a.floating_compare(31,0);
                                if(!a.nonpositive()) {
                                    a.at(0x1037cd0c);a.store<4>(1,0x3c,3);a.floating_load(2,29,0x88,false);a.store<4>(1,0x38,0);
                                    a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,3);a.floating_subtract(1,0,3,true);a.store<4>(1,0x40,0);
                                    a.floating_load(0,1,0x40,false);a.floating_subtract(1,31,1,true);a.floating_subtract(0,0,3,false);a.floating_product_sum(31,2,1,0);
                                    a.floating_round_single(31,31);a.jump(0x1037cdb0);
                                }
                            }
                        } else {
                            a.at(0x1037cc7c);a.multiply(3,21,0xcb2);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                            a.floating_load(3,29,0xc0,false);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x38,false);
                            a.floating_subtract(0,0,3,true);a.floating_compare(31,0);
                            if(!a.nonpositive()) {
                                a.at(0x1037cca4);a.store<4>(1,0x3c,3);a.floating_load(2,29,0x88,false);a.store<4>(1,0x38,0);
                                a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,3);a.floating_subtract(1,0,3,true);a.store<4>(1,0x40,0);
                                a.floating_load(0,1,0x40,false);a.floating_subtract(1,31,1,true);a.floating_subtract(0,0,3,false);a.floating_product_sum(31,2,1,0);
                                a.floating_round_single(31,31);a.jump(0x1037cdb0);
                            }
                        }
                    }
                }
            }
        } else {
            a.at(0x1037cc0c);a.multiply(3,21,0xabe);a.value(0,0x43300000);a.store<4>(1,0x38,0);
            a.floating_load(1,29,0xc0,false);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x38,false);
            a.floating_subtract(0,0,1,true);a.floating_compare(31,0);
            if(!a.nonpositive()) {
                a.at(0x1037cc34);a.store<4>(1,0x3c,3);a.store<4>(1,0x38,0);a.floating_load(0,1,0x38,false);
                a.floating_subtract(31,0,1,true);a.jump(0x1037cdb0);
            }
        }
    } else {
        a.at(0x1037cbc8);a.multiply(3,21,0x9c4);a.value(0,0x43300000);a.store<4>(1,0x38,0);
        a.floating_load(1,29,0xc0,false);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x38,false);
        a.floating_subtract(0,0,1,true);a.floating_compare(31,0);
        if(!a.nonpositive()) {
            a.at(0x1037cbf0);a.store<4>(1,0x3c,3);a.store<4>(1,0x38,0);a.floating_load(0,1,0x38,false);
            a.floating_subtract(31,0,1,true);a.jump(0x1037cdb0);
        }
    }
    a.at(0x1037cdb0);a.load<1>(0,24,0x15,false);a.compare(0,0x14,true);
    if(!a.equal()) {
        a.at(0x1037cdc4);a.load<1>(0,24,0x14,false);a.compare(0,0x14,true);
        if(a.equal()) {
            a.at(0x1037cddc);a.floating_load(30,30,0x7c,true);a.jump(0x1037ce10);
        } else {
            a.at(0x1037cdd0);a.load<1>(0,24,0x13,false);a.compare(0,0x14,true);
            if(!a.equal()) {
                a.at(0x1037cde4);a.load<1>(0,24,0x12,false);a.compare(0,0x14,true);
                if(!a.equal()) {
                    a.at(0x1037cdf8);a.load<1>(0,24,0x11,false);a.compare(0,0x14,true);
                    if(!a.equal()) {
                        a.at(0x1037ce0c);a.floating_load(30,30,0x160,true);
                    } else {
                        a.at(0x1037ce04);a.floating_load(30,30,0x15c,true);a.jump(0x1037ce10);
                    }
                } else {
                    a.at(0x1037cdf0);a.floating_load(30,30,0x7c,true);a.jump(0x1037ce10);
                }
            } else {
                a.at(0x1037cddc);a.floating_load(30,30,0x7c,true);a.jump(0x1037ce10);
            }
        }
    } else {
        a.at(0x1037cdbc);a.floating_load(30,30,0xd8,true);a.jump(0x1037ce10);
    }
    a.at(0x1037ce10);a.load<1>(0,26,0x0,false);a.compare(0,0x1,true);
    if(a.equal()) {
        a.at(0x1037ce1c);a.compare(16,0x0,false);
        if(!a.equal()) {
            a.at(0x1037ce24);a.move(3,16);a.call(0x100de260);a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x1037ce34);a.load<2>(0,24,0xa,true);a.compare(0,0x1c52,true);
                if(!a.nonpositive()) {
                    a.at(0x1037ce40);a.floating_load(0,29,0x158,false);a.floating_product(30,0,30);a.floating_round_single(30,30);
                }
            }
        }
    }
    a.at(0x1037ce4c);a.floating_compare(31,30);
    if(!a.nonpositive()) {
        a.at(0x1037ce54);a.floating_subtract(0,31,30,true);a.floating_load(1,29,0xa0,false);a.floating_product_sum(31,1,0,30);
        a.floating_round_single(31,31);
    }
    a.at(0x1037ce64);a.load<4>(0,15,0x24,false);a.compare(0,0x0,false);
    if(a.equal()) {
        a.at(0x1037ce70);a.compare(20,0x21,true);
        if(a.less()) {
            a.at(0x1037ced8);a.load<4>(4,15,0x4c,false);a.load<4>(3,2,-0x7b80,false);a.load<4>(5,15,0x50,false);
            a.call(0x10718130);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
            a.floating_load(2,29,0xc0,false);a.store<4>(1,0x38,0);a.floating_load(0,30,0xe0,true);a.floating_load(1,1,0x38,false);
            a.floating_subtract(30,1,2,true);a.floating_compare(30,0);
            if(!a.nonpositive()) {
                a.at(0x1037cf10);a.floating_move(30,0);
            }
            a.at(0x1037cf14);a.load<4>(4,2,-0x7b80,false);a.move(3,16);a.call(0x100d9440);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x1037cf28);a.floating_load(0,30,0x11c,true);a.floating_sum_single(30,30,0);
            }
            a.at(0x1037cf30);a.floating_load(0,30,0x164,true);a.floating_compare(30,0);
            if(!a.nonpositive()) {
                a.at(0x1037cf3c);a.floating_subtract(1,30,0,true);a.floating_load(2,29,0xa0,false);a.floating_load(0,29,0x238,false);
                a.floating_product_sum(30,2,1,0);a.floating_round_single(30,30);
            }
            a.at(0x1037cf50);a.floating_load(1,30,0x128,true);a.floating_load(0,30,0x124,true);a.floating_load(2,30,0x120,true);
            a.negative_product_difference_single_at(0x1037cf5c,0,1,30,0);a.floating_compare(2,0);
            if(a.nonpositive()) {
                a.at(0x1037cf6c);a.floating_move(2,0);
            } else {
                a.at(0x1037cf68);a.jump(0x1037cf70);
            }
            a.at(0x1037cf70);a.floating_product_single(31,31,2);a.jump(0x1037cfec);
        } else {
            a.at(0x1037ce78);a.load<1>(0,15,0x5e,false);a.compare(0,0x0,false);
            if(a.equal()) {
                a.at(0x1037ced8);a.load<4>(4,15,0x4c,false);a.load<4>(3,2,-0x7b80,false);a.load<4>(5,15,0x50,false);
                a.call(0x10718130);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
                a.floating_load(2,29,0xc0,false);a.store<4>(1,0x38,0);a.floating_load(0,30,0xe0,true);a.floating_load(1,1,0x38,false);
                a.floating_subtract(30,1,2,true);a.floating_compare(30,0);
                if(!a.nonpositive()) {
                    a.at(0x1037cf10);a.floating_move(30,0);
                }
                a.at(0x1037cf14);a.load<4>(4,2,-0x7b80,false);a.move(3,16);a.call(0x100d9440);
                a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x1037cf28);a.floating_load(0,30,0x11c,true);a.floating_sum_single(30,30,0);
                }
                a.at(0x1037cf30);a.floating_load(0,30,0x164,true);a.floating_compare(30,0);
                if(!a.nonpositive()) {
                    a.at(0x1037cf3c);a.floating_subtract(1,30,0,true);a.floating_load(2,29,0xa0,false);a.floating_load(0,29,0x238,false);
                    a.floating_product_sum(30,2,1,0);a.floating_round_single(30,30);
                }
                a.at(0x1037cf50);a.floating_load(1,30,0x128,true);a.floating_load(0,30,0x124,true);a.floating_load(2,30,0x120,true);
                a.negative_product_difference_single_at(0x1037cf5c,0,1,30,0);a.floating_compare(2,0);
                if(a.nonpositive()) {
                    a.at(0x1037cf6c);a.floating_move(2,0);
                } else {
                    a.at(0x1037cf68);a.jump(0x1037cf70);
                }
                a.at(0x1037cf70);a.floating_product_single(31,31,2);a.jump(0x1037cfec);
            } else {
                a.at(0x1037ce84);a.load<1>(3,15,0x64,false);a.load<1>(0,24,0x44,false);a.sign8(3,3,false);
                a.sign8(0,0,false);a.add(0,3,0);a.compare(0,0x1e,true);
                if(a.positive()) {
                    a.at(0x1037ced8);a.load<4>(4,15,0x4c,false);a.load<4>(3,2,-0x7b80,false);a.load<4>(5,15,0x50,false);
                    a.call(0x10718130);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
                    a.floating_load(2,29,0xc0,false);a.store<4>(1,0x38,0);a.floating_load(0,30,0xe0,true);a.floating_load(1,1,0x38,false);
                    a.floating_subtract(30,1,2,true);a.floating_compare(30,0);
                    if(!a.nonpositive()) {
                        a.at(0x1037cf10);a.floating_move(30,0);
                    }
                    a.at(0x1037cf14);a.load<4>(4,2,-0x7b80,false);a.move(3,16);a.call(0x100d9440);
                    a.sign8(0,3,true);
                    if(!a.equal()) {
                        a.at(0x1037cf28);a.floating_load(0,30,0x11c,true);a.floating_sum_single(30,30,0);
                    }
                    a.at(0x1037cf30);a.floating_load(0,30,0x164,true);a.floating_compare(30,0);
                    if(!a.nonpositive()) {
                        a.at(0x1037cf3c);a.floating_subtract(1,30,0,true);a.floating_load(2,29,0xa0,false);a.floating_load(0,29,0x238,false);
                        a.floating_product_sum(30,2,1,0);a.floating_round_single(30,30);
                    }
                    a.at(0x1037cf50);a.floating_load(1,30,0x128,true);a.floating_load(0,30,0x124,true);a.floating_load(2,30,0x120,true);
                    a.negative_product_difference_single_at(0x1037cf5c,0,1,30,0);a.floating_compare(2,0);
                    if(a.nonpositive()) {
                        a.at(0x1037cf6c);a.floating_move(2,0);
                    } else {
                        a.at(0x1037cf68);a.jump(0x1037cf70);
                    }
                    a.at(0x1037cf70);a.floating_product_single(31,31,2);a.jump(0x1037cfec);
                } else {
                    a.at(0x1037cea0);a.value(3,0x88890000);a.load<4>(4,15,0x0,false);a.offset(0,3,-0x7777);
                    a.multiply_high(0,0,4);a.add(0,0,4);a.shift_right(0,0,3);a.mask(3,0,0x1,false,1);
                    a.add(0,0,3);a.multiply(0,0,0xf);a.subtract(0,0,4,true);
                    if(a.equal()) {
                        a.at(0x1037ced8);a.load<4>(4,15,0x4c,false);a.load<4>(3,2,-0x7b80,false);a.load<4>(5,15,0x50,false);
                        a.call(0x10718130);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
                        a.floating_load(2,29,0xc0,false);a.store<4>(1,0x38,0);a.floating_load(0,30,0xe0,true);a.floating_load(1,1,0x38,false);
                        a.floating_subtract(30,1,2,true);a.floating_compare(30,0);
                        if(!a.nonpositive()) {
                            a.at(0x1037cf10);a.floating_move(30,0);
                        }
                        a.at(0x1037cf14);a.load<4>(4,2,-0x7b80,false);a.move(3,16);a.call(0x100d9440);
                        a.sign8(0,3,true);
                        if(!a.equal()) {
                            a.at(0x1037cf28);a.floating_load(0,30,0x11c,true);a.floating_sum_single(30,30,0);
                        }
                        a.at(0x1037cf30);a.floating_load(0,30,0x164,true);a.floating_compare(30,0);
                        if(!a.nonpositive()) {
                            a.at(0x1037cf3c);a.floating_subtract(1,30,0,true);a.floating_load(2,29,0xa0,false);a.floating_load(0,29,0x238,false);
                            a.floating_product_sum(30,2,1,0);a.floating_round_single(30,30);
                        }
                        a.at(0x1037cf50);a.floating_load(1,30,0x128,true);a.floating_load(0,30,0x124,true);a.floating_load(2,30,0x120,true);
                        a.negative_product_difference_single_at(0x1037cf5c,0,1,30,0);a.floating_compare(2,0);
                        if(a.nonpositive()) {
                            a.at(0x1037cf6c);a.floating_move(2,0);
                        } else {
                            a.at(0x1037cf68);a.jump(0x1037cf70);
                        }
                        a.at(0x1037cf70);a.floating_product_single(31,31,2);a.jump(0x1037cfec);
                    } else {
                        a.at(0x1037cecc);a.load<2>(0,16,0x1ec,true);a.compare(0,0x1194,true);
                        if(!a.nonpositive()) {
                            a.at(0x1037ced8);a.load<4>(4,15,0x4c,false);a.load<4>(3,2,-0x7b80,false);a.load<4>(5,15,0x50,false);
                            a.call(0x10718130);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
                            a.floating_load(2,29,0xc0,false);a.store<4>(1,0x38,0);a.floating_load(0,30,0xe0,true);a.floating_load(1,1,0x38,false);
                            a.floating_subtract(30,1,2,true);a.floating_compare(30,0);
                            if(!a.nonpositive()) {
                                a.at(0x1037cf10);a.floating_move(30,0);
                            }
                            a.at(0x1037cf14);a.load<4>(4,2,-0x7b80,false);a.move(3,16);a.call(0x100d9440);
                            a.sign8(0,3,true);
                            if(!a.equal()) {
                                a.at(0x1037cf28);a.floating_load(0,30,0x11c,true);a.floating_sum_single(30,30,0);
                            }
                            a.at(0x1037cf30);a.floating_load(0,30,0x164,true);a.floating_compare(30,0);
                            if(!a.nonpositive()) {
                                a.at(0x1037cf3c);a.floating_subtract(1,30,0,true);a.floating_load(2,29,0xa0,false);a.floating_load(0,29,0x238,false);
                                a.floating_product_sum(30,2,1,0);a.floating_round_single(30,30);
                            }
                            a.at(0x1037cf50);a.floating_load(1,30,0x128,true);a.floating_load(0,30,0x124,true);a.floating_load(2,30,0x120,true);
                            a.negative_product_difference_single_at(0x1037cf5c,0,1,30,0);a.floating_compare(2,0);
                            if(a.nonpositive()) {
                                a.at(0x1037cf6c);a.floating_move(2,0);
                            } else {
                                a.at(0x1037cf68);a.jump(0x1037cf70);
                            }
                            a.at(0x1037cf70);a.floating_product_single(31,31,2);a.jump(0x1037cfec);
                        }
                    }
                }
            }
        }
    }
    a.at(0x1037cfec);a.compare(16,0x0,false);
    if(!a.equal()) {
        a.at(0x1037cff4);a.compare(24,0x0,false);
        if(!a.equal()) {
            a.at(0x1037cffc);a.load<1>(0,16,0x23d,false);a.compare(0,0x1,true);
            if(a.equal()) {
                a.at(0x1037d064);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                a.exclusive_flags(0,3,0x80000000);a.floating_load(1,29,0xc0,false);a.store<4>(1,0x3c,0);a.floating_load(2,29,0x1d0,false);
                a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(1,2,0);
            } else {
                a.at(0x1037d008);a.load<2>(0,16,0x1ec,true);a.compare(0,0x8ca,true);
                if(a.nonpositive()) {
                    a.at(0x1037d03c);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
                    a.floating_load(1,29,0xc0,false);a.store<4>(1,0x38,0);a.floating_load(2,29,0xe0,false);a.floating_load(0,1,0x38,false);
                    a.floating_subtract(0,0,1,false);a.floating_product(1,2,0);a.jump(0x1037d08c);
                } else {
                    a.at(0x1037d014);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
                    a.floating_load(1,29,0xc0,false);a.store<4>(1,0x38,0);a.floating_load(2,29,0x1c8,false);a.floating_load(0,1,0x38,false);
                    a.floating_subtract(0,0,1,false);a.floating_product(1,2,0);a.jump(0x1037d08c);
                }
            }
            a.at(0x1037d08c);a.floating_load(2,29,0x1d8,false);a.call(0x10780fc0);a.load<4>(2,1,0x14,false);
            a.floating_move(30,1);a.floating_load(0,29,0x108,false);a.floating_compare(30,0);
            if(!a.nonpositive()) {
                a.at(0x1037d0a8);a.floating_move(30,0);
            }
            a.at(0x1037d0ac);a.sign8(19,18,false);a.compare(19,0x4,true);
            if(a.equal()) {
                a.at(0x1037d3e4);a.compare(20,0x14,true);
                if(a.at_least()) {
                    a.at(0x1037d438);a.compare(20,0x18,true);
                    if(a.less()) {
                        a.at(0x1037d450);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                        a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x78,false);a.exclusive_flags(0,0,0x80000000);
                        a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                        a.floating_product(0,30,0);a.floating_compare(31,0);
                        if(!a.at_least()) {
                            a.at(0x1037d488);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                            a.floating_round_single(31,31);a.jump(0x1037d57c);
                        }
                    } else {
                        a.at(0x1037d440);a.move(3,16);a.call(0x100de260);a.sign8(0,3,true);
                        if(!a.equal()) {
                            a.at(0x1037d49c);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                            a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e8,false);a.exclusive_flags(0,0,0x80000000);
                            a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                            a.floating_product(0,30,0);a.floating_compare(31,0);
                            if(!a.at_least()) {
                                a.at(0x1037d4d4);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                            }
                        } else {
                            a.at(0x1037d450);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                            a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x78,false);a.exclusive_flags(0,0,0x80000000);
                            a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                            a.floating_product(0,30,0);a.floating_compare(31,0);
                            if(!a.at_least()) {
                                a.at(0x1037d488);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                            }
                        }
                    }
                } else {
                    a.at(0x1037d3ec);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x88,false);a.exclusive_flags(0,0,0x80000000);
                    a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                    a.floating_product(0,30,0);a.floating_compare(31,0);
                    if(!a.at_least()) {
                        a.at(0x1037d424);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                    }
                }
            } else {
                a.at(0x1037d0b8);
                if(a.at_least()) {
                    a.at(0x1037d0d4);a.compare(19,0x6,true);
                    if(a.equal()) {
                        a.at(0x1037d534);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                        a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1f0,false);a.exclusive_flags(0,0,0x80000000);
                        a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                        a.floating_product(0,30,0);a.floating_compare(31,0);
                        if(!a.at_least()) {
                            a.at(0x1037d56c);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                            a.floating_round_single(31,31);
                        }
                    } else {
                        a.at(0x1037d0dc);
                        if(!a.at_least()) {
                            a.at(0x1037d0e0);a.jump(0x1037d4e8);a.at(0x1037d4e8);a.load<2>(3,16,0x1ec,true);
                            a.value(0,0x43300000);a.store<4>(1,0x38,0);a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);
                            a.floating_load(2,29,0xd8,false);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);
                            a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_product(0,30,0);a.floating_compare(31,0);
                            if(!a.at_least()) {
                                a.at(0x1037d520);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                            }
                        }
                    }
                } else {
                    a.at(0x1037d0bc);a.compare(19,0x2,true);
                    if(a.equal()) {
                        a.at(0x1037d1e8);a.compare(20,0x14,true);
                        if(a.at_least()) {
                            a.at(0x1037d23c);a.compare(20,0x18,true);
                            if(a.less()) {
                                a.at(0x1037d254);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e0,false);a.exclusive_flags(0,0,0x80000000);
                                a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                a.floating_product(0,30,0);a.floating_compare(31,0);
                                if(!a.at_least()) {
                                    a.at(0x1037d28c);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                    a.floating_round_single(31,31);a.jump(0x1037d57c);
                                }
                            } else {
                                a.at(0x1037d244);a.move(3,16);a.call(0x100de260);a.sign8(0,3,true);
                                if(!a.equal()) {
                                    a.at(0x1037d2a0);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x3c,0);
                                    a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,30,0);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037d2d0);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                                    }
                                } else {
                                    a.at(0x1037d254);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e0,false);a.exclusive_flags(0,0,0x80000000);
                                    a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                    a.floating_product(0,30,0);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037d28c);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                                    }
                                }
                            }
                        } else {
                            a.at(0x1037d1f0);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                            a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e8,false);a.exclusive_flags(0,0,0x80000000);
                            a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                            a.floating_product(0,30,0);a.floating_compare(31,0);
                            if(!a.at_least()) {
                                a.at(0x1037d228);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                            }
                        }
                    } else {
                        a.at(0x1037d0c4);
                        if(a.at_least()) {
                            a.at(0x1037d2e4);a.compare(20,0x14,true);
                            if(!a.at_least()) {
                                a.at(0x1037d2ec);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x48,false);a.exclusive_flags(0,0,0x80000000);
                                a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                a.floating_product(0,30,0);a.floating_compare(31,0);
                                if(!a.at_least()) {
                                    a.at(0x1037d324);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                    a.floating_round_single(31,31);
                                }
                            }
                            a.at(0x1037d334);a.compare(20,0x18,true);
                            if(a.less()) {
                                a.at(0x1037d34c);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e8,false);a.exclusive_flags(0,0,0x80000000);
                                a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                a.floating_product(0,30,0);a.floating_compare(31,0);
                                if(!a.at_least()) {
                                    a.at(0x1037d384);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                    a.floating_round_single(31,31);a.jump(0x1037d57c);
                                }
                            } else {
                                a.at(0x1037d33c);a.move(3,16);a.call(0x100de260);a.sign8(0,3,true);
                                if(!a.equal()) {
                                    a.at(0x1037d398);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e0,false);a.exclusive_flags(0,0,0x80000000);
                                    a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                    a.floating_product(0,30,0);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037d3d0);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                                    }
                                } else {
                                    a.at(0x1037d34c);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e8,false);a.exclusive_flags(0,0,0x80000000);
                                    a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                    a.floating_product(0,30,0);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037d384);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                                    }
                                }
                            }
                        } else {
                            a.at(0x1037d0c8);a.compare(19,0x1,true);
                            if(a.at_least()) {
                                a.at(0x1037d0e4);a.compare(20,0x14,true);
                                if(a.at_least()) {
                                    a.at(0x1037d138);a.compare(20,0x18,true);
                                    if(a.less()) {
                                        a.at(0x1037d150);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                        a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x118,false);a.exclusive_flags(0,0,0x80000000);
                                        a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                        a.floating_product(0,30,0);a.floating_compare(31,0);
                                        if(!a.at_least()) {
                                            a.at(0x1037d188);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                            a.floating_round_single(31,31);a.jump(0x1037d57c);
                                        }
                                    } else {
                                        a.at(0x1037d140);a.move(3,16);a.call(0x100de260);a.sign8(0,3,true);
                                        if(!a.equal()) {
                                            a.at(0x1037d19c);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                            a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x130,false);a.exclusive_flags(0,0,0x80000000);
                                            a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                            a.floating_product(0,30,0);a.floating_compare(31,0);
                                            if(!a.at_least()) {
                                                a.at(0x1037d1d4);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                                            }
                                        } else {
                                            a.at(0x1037d150);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                            a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x118,false);a.exclusive_flags(0,0,0x80000000);
                                            a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                            a.floating_product(0,30,0);a.floating_compare(31,0);
                                            if(!a.at_least()) {
                                                a.at(0x1037d188);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                                a.floating_round_single(31,31);a.jump(0x1037d57c);
                                            }
                                        }
                                    }
                                } else {
                                    a.at(0x1037d0ec);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                                    a.multiply(0,3,0x3);a.floating_load(1,29,0xc0,false);a.floating_load(2,29,0x1e0,false);a.exclusive_flags(0,0,0x80000000);
                                    a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                    a.floating_product(0,30,0);a.floating_compare(31,0);
                                    if(!a.at_least()) {
                                        a.at(0x1037d124);a.floating_sum(1,31,0);a.floating_load(0,29,0x28,false);a.floating_product(31,1,0);
                                        a.floating_round_single(31,31);a.jump(0x1037d57c);
                                    }
                                }
                            } else {
                                a.at(0x1037d0d0);a.jump(0x1037d57c);
                            }
                        }
                    }
                }
            }
            a.at(0x1037d57c);a.load<4>(3,31,0x0,false);a.move(4,15);a.call(0x10082f20);
            a.compare(3,0x0,false);
            if(!a.equal()) {
                a.at(0x1037d590);a.load<4>(0,3,0x50,false);a.mask(0,0,0x10,true,0);
                if(!a.equal()) {
                    a.at(0x1037d59c);a.compare(19,0x4,true);
                    if(a.equal()) {
                        a.at(0x1037d5ec);a.floating_load(0,29,0x200,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                        a.jump(0x1037d618);
                    } else {
                        a.at(0x1037d5a4);
                        if(a.at_least()) {
                            a.at(0x1037d5bc);a.compare(19,0x6,true);
                            if(a.equal()) {
                                a.at(0x1037d60c);a.floating_load(0,29,0x208,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                            } else {
                                a.at(0x1037d5c4);
                                if(!a.at_least()) {
                                    a.at(0x1037d5c8);a.jump(0x1037d5fc);a.at(0x1037d5fc);a.floating_load(0,29,0x110,false);
                                    a.floating_product(31,0,31);a.floating_round_single(31,31);a.jump(0x1037d618);
                                }
                            }
                        } else {
                            a.at(0x1037d5a8);a.compare(19,0x3,true);
                            if(a.at_least()) {
                                a.at(0x1037d5dc);a.floating_load(0,29,0x38,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                a.jump(0x1037d618);
                            } else {
                                a.at(0x1037d5b0);a.compare(19,0x1,true);
                                if(a.at_least()) {
                                    a.at(0x1037d5cc);a.floating_load(0,29,0x1f8,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                    a.jump(0x1037d618);
                                } else {
                                    a.at(0x1037d5b8);a.jump(0x1037d618);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    a.at(0x1037d618);a.compare(16,0x0,false);
    if(!a.equal()) {
        a.at(0x1037d620);a.load<4>(3,16,0x2c,false);a.compare(3,0x0,false);
        if(!a.equal()) {
            a.at(0x1037d62c);a.load<4>(4,15,0x24,false);a.compare(4,0x0,false);
            if(a.equal()) {
                a.at(0x1037d650);a.compare(4,0x0,false);
                if(a.equal()) {
                    return check_quote_home_country(a);
                } else {
                    a.at(0x1037d658);a.load<4>(0,4,0x2c,false);a.compare_registers(0,3,false);
                    if(!a.equal()) {
                        return check_quote_home_country(a);
                    } else {
                        a.at(0x1037d664);a.load<4>(0,15,0xc,false);a.compare_registers(0,3,false);
                        if(a.equal()) {
                            return check_quote_home_country(a);
                        } else {
                            a.at(0x1037d670);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0x3,true);
                            if(a.positive()) {
                                return apply_quote_country_rules(a);
                            } else {
                                a.at(0x1037d680);a.load<1>(0,15,0x63,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                                if(a.less()) {
                                    return apply_quote_country_rules(a);
                                } else {
                                    return check_quote_home_country(a);
                                }
                            }
                        }
                    }
                }
            } else {
                a.at(0x1037d638);a.load<4>(0,4,0x2c,false);a.compare_registers(0,3,false);
                if(!a.equal()) {
                    a.at(0x1037d650);a.compare(4,0x0,false);
                    if(a.equal()) {
                        return check_quote_home_country(a);
                    } else {
                        a.at(0x1037d658);a.load<4>(0,4,0x2c,false);a.compare_registers(0,3,false);
                        if(!a.equal()) {
                            return check_quote_home_country(a);
                        } else {
                            a.at(0x1037d664);a.load<4>(0,15,0xc,false);a.compare_registers(0,3,false);
                            if(a.equal()) {
                                return check_quote_home_country(a);
                            } else {
                                a.at(0x1037d670);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0x3,true);
                                if(a.positive()) {
                                    return apply_quote_country_rules(a);
                                } else {
                                    a.at(0x1037d680);a.load<1>(0,15,0x63,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                                    if(a.less()) {
                                        return apply_quote_country_rules(a);
                                    } else {
                                        return check_quote_home_country(a);
                                    }
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x1037d644);a.load<4>(0,15,0xc,false);a.compare_registers(0,3,false);
                    if(a.equal()) {
                        return apply_quote_country_rules(a);
                    } else {
                        a.at(0x1037d650);a.compare(4,0x0,false);
                        if(a.equal()) {
                            return check_quote_home_country(a);
                        } else {
                            a.at(0x1037d658);a.load<4>(0,4,0x2c,false);a.compare_registers(0,3,false);
                            if(!a.equal()) {
                                return check_quote_home_country(a);
                            } else {
                                a.at(0x1037d664);a.load<4>(0,15,0xc,false);a.compare_registers(0,3,false);
                                if(a.equal()) {
                                    return check_quote_home_country(a);
                                } else {
                                    a.at(0x1037d670);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0x3,true);
                                    if(a.positive()) {
                                        return apply_quote_country_rules(a);
                                    } else {
                                        a.at(0x1037d680);a.load<1>(0,15,0x63,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                                        if(a.less()) {
                                            return apply_quote_country_rules(a);
                                        } else {
                                            return check_quote_home_country(a);
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
    return finish_quote_contract_limits(a);
}
}
