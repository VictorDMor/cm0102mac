#include "shortlist_quotation_detail.hpp"

namespace cm::aot::shortlist_quotation::detail {
void prepare_candidate_context(Call& a) {
    a.at(0x10366470);a.link();a.move(11,1);a.call(0x107737f8);
    a.multiple(13,false,-0x40);a.store<4>(1,0x8,0);a.frame(0x170);a.load<4>(30,2,-0x4ddc,false);
    a.combine(18,8,8,true,false);a.load<4>(23,2,-0x7590,false);a.move(15,4);a.store<4>(1,0x188,3);
    a.move(16,5);a.load<4>(25,2,-0x74ec,false);a.move(17,7);a.store<1>(1,0x197,6);
    a.value(24,0x0);a.load<4>(14,2,-0x7a9c,false);a.value(22,0x0);a.load<4>(13,2,-0x7b80,false);
    a.value(21,0x0);a.load<4>(19,2,-0x4dd8,false);a.floating_load(29,30,0x80,false);
    if(!a.equal()) {
        a.at(0x103664cc);a.value(0,0x0);a.store<4>(18,0x0,0);
    }
    a.at(0x103664d4);a.load<4>(0,15,0x1c,false);a.compare(0,0x0,false);
    if(a.equal()) {
        a.at(0x103664f8);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
        return;
    }
    a.at(0x103664e0);a.load<4>(0,16,0x2c,false);a.compare(0,0x0,false);
    if(a.equal()) {
        a.at(0x103664f8);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
        return;
    }
    a.at(0x103664ec);a.load<4>(0,15,0xc,false);a.compare(0,0x0,false);
    if(a.equal()) {
        a.at(0x103664f8);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
        return;
    }
    a.at(0x10366500);a.move(3,15);a.offset(4,19,0xef);a.call(0x1036ff00);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x1036653c);a.load<4>(0,15,0x24,false);a.compare(0,0x0,false);
        if(!a.equal()) {
            a.at(0x10366548);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
            return;
        }
    } else {
        a.at(0x10366514);a.move(3,15);a.offset(4,19,0xfb);a.call(0x1036ff00);
        a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x1036653c);a.load<4>(0,15,0x24,false);a.compare(0,0x0,false);
            if(!a.equal()) {
                a.at(0x10366548);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                return;
            }
        } else {
            a.at(0x10366528);a.move(3,15);a.offset(4,19,0x105);a.call(0x1036ff00);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x1036653c);a.load<4>(0,15,0x24,false);a.compare(0,0x0,false);
                if(!a.equal()) {
                    a.at(0x10366548);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                    return;
                }
            }
        }
    }
    a.at(0x10366550);a.load<1>(3,15,0x62,false);a.compare(3,0x11,true);
    if(a.equal()) {
        a.at(0x10366568);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
        return;
    }
    a.at(0x1036655c);a.load<1>(0,15,0x60,false);a.compare(0,0x11,true);
    if(a.equal()) {
        a.at(0x10366568);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
        return;
    }
    a.at(0x10366570);a.sign8(4,17,false);a.compare(4,0x5,true);
    if(!a.equal()) {
        a.at(0x1036657c);a.load<4>(4,2,-0x7be4,false);a.load<4>(5,15,0x0,false);a.load<4>(4,4,0x0,false);
        a.offset(4,4,-0x10);a.compare_registers(5,4,true);
        if(!a.less()) {
            a.at(0x10366594);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
            return;
        }
    }
    a.at(0x1036659c);a.load<4>(4,15,0x20,false);a.compare(4,0x0,false);
    if(!a.equal()) {
        a.at(0x103665a8);a.sign8(5,0,false);a.compare(5,0x5,true);
        if(a.equal()) {
            a.at(0x103665b4);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
            return;
        }
    }
    a.at(0x103665bc);a.sign8(5,17,true);
    if(!a.equal()) {
        a.at(0x103665c4);a.sign8(5,17,false);a.compare(5,0x5,true);
        if(!a.equal()) {
            a.at(0x103665d0);a.sign8(5,3,false);a.compare(5,0x5,true);
            if(a.equal()) {
                a.at(0x103665f8);a.load<4>(6,15,0x1c,false);a.load<2>(5,6,0x2a,true);a.compare(5,0x109a,true);
                if(a.positive()) {
                    a.at(0x10366614);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                    return;
                }
                a.at(0x10366608);a.load<2>(5,6,0x24,true);a.compare(5,0x64,true);
                if(!a.nonpositive()) {
                    a.at(0x10366614);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                    return;
                }
            } else {
                a.at(0x103665dc);a.compare(5,0xc,true);
                if(a.equal()) {
                    a.at(0x103665f8);a.load<4>(6,15,0x1c,false);a.load<2>(5,6,0x2a,true);a.compare(5,0x109a,true);
                    if(a.positive()) {
                        a.at(0x10366614);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                        return;
                    }
                    a.at(0x10366608);a.load<2>(5,6,0x24,true);a.compare(5,0x64,true);
                    if(!a.nonpositive()) {
                        a.at(0x10366614);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                        return;
                    }
                } else {
                    a.at(0x103665e4);a.sign8(5,0,false);a.compare(5,0x5,true);
                    if(a.equal()) {
                        a.at(0x103665f8);a.load<4>(6,15,0x1c,false);a.load<2>(5,6,0x2a,true);a.compare(5,0x109a,true);
                        if(a.positive()) {
                            a.at(0x10366614);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                            return;
                        }
                        a.at(0x10366608);a.load<2>(5,6,0x24,true);a.compare(5,0x64,true);
                        if(!a.nonpositive()) {
                            a.at(0x10366614);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                            return;
                        }
                    } else {
                        a.at(0x103665f0);a.compare(5,0xc,true);
                        if(a.equal()) {
                            a.at(0x103665f8);a.load<4>(6,15,0x1c,false);a.load<2>(5,6,0x2a,true);a.compare(5,0x109a,true);
                            if(a.positive()) {
                                a.at(0x10366614);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                                return;
                            }
                            a.at(0x10366608);a.load<2>(5,6,0x24,true);a.compare(5,0x64,true);
                            if(!a.nonpositive()) {
                                a.at(0x10366614);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                                return;
                            }
                        }
                    }
                }
            }
        }
        a.at(0x1036661c);a.sign8(5,17,false);a.compare(5,0xa,true);
        if(a.equal()) {
            a.at(0x10366628);a.sign8(5,3,false);a.compare(5,0xa,true);
            if(!a.equal()) {
                a.at(0x10366634);a.sign8(5,3,true);
                if(!a.equal()) {
                    a.at(0x1036663c);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                    return;
                }
            }
        }
        a.at(0x10366644);a.sign8(5,0,false);a.compare(5,0x5,true);
        if(a.equal()) {
            a.at(0x10366650);a.compare(4,0x0,false);
            if(!a.equal()) {
                a.at(0x10366658);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                return;
            }
        }
        a.at(0x10366660);a.sign8(4,17,false);a.compare(4,0xf,true);
        if(!a.equal()) {
            a.at(0x1036666c);a.compare(4,0xc,true);
            if(!a.equal()) {
                a.at(0x10366674);a.compare(4,0x5,true);
                if(!a.equal()) {
                    a.at(0x1036667c);a.load<4>(4,15,0x14,false);a.compare(4,0x0,false);
                    if(!a.equal()) {
                        a.at(0x10366688);a.sign8(4,3,false);a.compare(4,0xb,true);
                        if(a.equal()) {
                            a.at(0x10366694);a.load<4>(4,15,0x24,false);a.compare(4,0x0,false);
                            if(!a.equal()) {
                                a.at(0x103666a0);a.compare_registers(16,4,false);
                                if(!a.equal()) {
                                    a.at(0x103666a8);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
        a.at(0x103666b0);a.sign8(4,17,false);a.compare(4,0xa,true);
        if(!a.equal()) {
            a.at(0x103666bc);a.sign8(4,3,false);a.compare(4,0xa,true);
            if(a.equal()) {
                a.at(0x103666c8);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                return;
            }
        }
    }
    a.at(0x103666d0);a.compare(16,0x0,false);
    if(!a.equal()) {
        a.at(0x103666d8);a.sign8(4,17,false);a.compare(4,0x5,true);
        if(!a.equal()) {
            a.at(0x103666e4);a.sign8(3,3,false);a.compare(3,0x5,true);
            if(a.equal()) {
                a.at(0x1036670c);a.load<4>(4,15,0x24,false);a.compare(4,0x0,false);
                if(!a.equal()) {
                    a.at(0x10366718);a.load<4>(5,16,0x30,false);a.compare(5,0x0,false);
                    if(!a.equal()) {
                        a.at(0x10366724);a.load<4>(3,4,0x30,false);a.compare(3,0x0,false);
                        if(!a.equal()) {
                            a.at(0x10366730);a.load<2>(3,3,0x18,true);a.load<2>(0,5,0x18,true);a.compare_registers(3,0,true);
                            if(!a.less()) {
                                a.at(0x10366740);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                                return;
                            }
                        }
                    }
                    a.at(0x10366748);a.load<2>(3,16,0x1ec,true);a.load<2>(4,4,0x1ec,true);a.offset(0,3,-0x2ee);
                    a.compare_registers(4,0,true);
                    if(!a.nonpositive()) {
                        a.at(0x1036675c);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                        return;
                    }
                }
            } else {
                a.at(0x103666f0);a.compare(3,0xc,true);
                if(a.equal()) {
                    a.at(0x1036670c);a.load<4>(4,15,0x24,false);a.compare(4,0x0,false);
                    if(!a.equal()) {
                        a.at(0x10366718);a.load<4>(5,16,0x30,false);a.compare(5,0x0,false);
                        if(!a.equal()) {
                            a.at(0x10366724);a.load<4>(3,4,0x30,false);a.compare(3,0x0,false);
                            if(!a.equal()) {
                                a.at(0x10366730);a.load<2>(3,3,0x18,true);a.load<2>(0,5,0x18,true);a.compare_registers(3,0,true);
                                if(!a.less()) {
                                    a.at(0x10366740);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                                    return;
                                }
                            }
                        }
                        a.at(0x10366748);a.load<2>(3,16,0x1ec,true);a.load<2>(4,4,0x1ec,true);a.offset(0,3,-0x2ee);
                        a.compare_registers(4,0,true);
                        if(!a.nonpositive()) {
                            a.at(0x1036675c);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                            return;
                        }
                    }
                } else {
                    a.at(0x103666f8);a.sign8(0,0,false);a.compare(0,0x5,true);
                    if(a.equal()) {
                        a.at(0x1036670c);a.load<4>(4,15,0x24,false);a.compare(4,0x0,false);
                        if(!a.equal()) {
                            a.at(0x10366718);a.load<4>(5,16,0x30,false);a.compare(5,0x0,false);
                            if(!a.equal()) {
                                a.at(0x10366724);a.load<4>(3,4,0x30,false);a.compare(3,0x0,false);
                                if(!a.equal()) {
                                    a.at(0x10366730);a.load<2>(3,3,0x18,true);a.load<2>(0,5,0x18,true);a.compare_registers(3,0,true);
                                    if(!a.less()) {
                                        a.at(0x10366740);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                                        return;
                                    }
                                }
                            }
                            a.at(0x10366748);a.load<2>(3,16,0x1ec,true);a.load<2>(4,4,0x1ec,true);a.offset(0,3,-0x2ee);
                            a.compare_registers(4,0,true);
                            if(!a.nonpositive()) {
                                a.at(0x1036675c);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                                return;
                            }
                        }
                    } else {
                        a.at(0x10366704);a.compare(0,0xc,true);
                        if(a.equal()) {
                            a.at(0x1036670c);a.load<4>(4,15,0x24,false);a.compare(4,0x0,false);
                            if(!a.equal()) {
                                a.at(0x10366718);a.load<4>(5,16,0x30,false);a.compare(5,0x0,false);
                                if(!a.equal()) {
                                    a.at(0x10366724);a.load<4>(3,4,0x30,false);a.compare(3,0x0,false);
                                    if(!a.equal()) {
                                        a.at(0x10366730);a.load<2>(3,3,0x18,true);a.load<2>(0,5,0x18,true);a.compare_registers(3,0,true);
                                        if(!a.less()) {
                                            a.at(0x10366740);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                                            return;
                                        }
                                    }
                                }
                                a.at(0x10366748);a.load<2>(3,16,0x1ec,true);a.load<2>(4,4,0x1ec,true);a.offset(0,3,-0x2ee);
                                a.compare_registers(4,0,true);
                                if(!a.nonpositive()) {
                                    a.at(0x1036675c);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    a.at(0x10366764);a.compare(16,0x0,false);
    if(!a.equal()) {
        a.at(0x1036676c);a.load<2>(0,16,0x1ec,true);a.compare(0,0xabe,true);
        if(!a.at_least()) {
            a.at(0x10366778);a.load<4>(4,16,0x2c,false);a.compare(4,0x0,false);
            if(!a.equal()) {
                a.at(0x10366784);a.load<4>(3,15,0x24,false);a.compare(3,0x0,false);
                if(!a.equal()) {
                    a.at(0x10366790);a.load<4>(0,3,0x2c,false);a.compare_registers(0,4,false);
                    if(a.equal()) {
                        a.at(0x103667b0);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                        return;
                    }
                }
                a.at(0x1036679c);a.move(3,15);a.value(5,0x1);a.call(0x100cab10);
                a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x103667b0);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                    return;
                }
            }
        }
    }
    a.at(0x103667b8);a.sign8(0,17,false);a.load<4>(3,2,-0x4de8,false);a.compare(0,0x5,true);
    a.floating_load(28,3,0xc,true);
    if(a.equal()) {
        a.at(0x103667d4);a.load<4>(3,2,-0x4de8,false);a.floating_load(28,3,0x10,true);
    } else {
        a.at(0x103667cc);a.compare(0,0xc,true);
        if(a.equal()) {
            a.at(0x103667d4);a.load<4>(3,2,-0x4de8,false);a.floating_load(28,3,0x10,true);
        }
    }
    a.at(0x103667dc);a.load<1>(4,15,0x63,false);a.value(3,0x43300000);a.load<1>(0,15,0x67,false);
    a.sign8(4,4,false);a.store<4>(1,0xd4,3);a.exclusive_flags(4,4,0x80000000);a.sign8(0,0,false);
    a.store<4>(1,0xd8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(10,30,0x8,false);a.floating_load(0,1,0xd4,false);
    a.floating_load(4,30,0x98,false);a.floating_subtract(0,0,10,false);a.store<4>(1,0xd0,0);a.floating_load(8,30,0x2d0,false);
    a.store<4>(1,0xcc,3);a.floating_difference(3,0,4,true,false);a.floating_load(5,30,0xb8,false);a.floating_load(2,1,0xcc,false);
    a.store<4>(1,0xc8,4);a.floating_load(1,30,0x238,false);a.store<4>(1,0xc4,3);a.floating_load(0,1,0xc4,false);
    a.floating_subtract(2,2,10,false);a.store<4>(1,0xbc,3);a.floating_subtract(9,8,3,false);a.floating_subtract(6,0,10,false);
    a.floating_load(7,30,0xf8,false);a.floating_difference(8,2,4,true,false);a.store<4>(1,0xc0,4);a.load<4>(3,2,-0x4de8,false);
    a.floating_load(3,1,0xbc,false);a.floating_load(4,30,0x68,false);a.floating_load(2,3,0xc,true);a.floating_difference(5,6,5,true,false);
    a.floating_load(0,30,0x158,false);a.floating_subtract(3,3,10,false);a.floating_subtract(27,9,8,false);a.floating_sum(26,7,5);
    a.floating_difference(1,3,1,true,false);a.floating_sum(30,4,1);a.floating_round_single(27,27);a.floating_round_single(26,26);
    a.floating_round_single(30,30);a.floating_sum_single(1,28,27);a.floating_sum_single(1,26,1);a.floating_sum_single(1,30,1);
    a.floating_sum_single(25,2,1);a.floating_compare(25,0);a.compare_less_or_equal_bit();
    if(a.equal()) {
        a.at(0x103668a8);a.floating_load(25,3,0x4,true);
    }
    a.at(0x103668ac);a.load<4>(20,15,0x24,false);a.load<1>(19,15,0x62,false);a.compare(20,0x0,false);
    if(a.equal()) {
        a.at(0x103668cc);a.load<4>(3,15,0x20,false);a.compare(3,0x0,false);
        if(!a.equal()) {
            a.at(0x103668d8);a.load<1>(4,15,0x60,false);a.compare(4,0xb,true);
            if(!a.equal()) {
                a.at(0x103668e4);a.move(20,3);a.move(19,4);
            }
        }
    } else {
        a.at(0x103668bc);a.move(3,16);a.call(0x100c7d00);a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x103668cc);a.load<4>(3,15,0x20,false);a.compare(3,0x0,false);
            if(!a.equal()) {
                a.at(0x103668d8);a.load<1>(4,15,0x60,false);a.compare(4,0xb,true);
                if(!a.equal()) {
                    a.at(0x103668e4);a.move(20,3);a.move(19,4);
                }
            }
        }
    }
    a.at(0x103668ec);a.compare(20,0x0,false);
    if(!a.equal()) {
        a.at(0x103668f4);a.load<4>(0,20,0x2c,false);a.compare(0,0x0,false);
        if(a.equal()) {
            a.at(0x10366900);a.floating_load(1,30,0x160,false);a.jump(0x10369058);
            return;
        }
    }
    a.at(0x10366908);a.load<4>(3,2,-0x7b7c,false);a.move(4,15);a.move(5,20);
    a.load<4>(3,3,0x0,false);a.call(0x10082f90);a.compare_registers(16,20,false);a.move(31,3);
    if(a.equal()) {
        a.at(0x10366af8);a.floating_load(31,30,0x160,false);
    } else {
        a.at(0x10366928);a.compare(31,0x0,false);
        if(!a.equal()) {
            a.at(0x10366930);a.compare(20,0x0,false);
            if(!a.equal()) {
                a.at(0x10366938);a.load<4>(4,20,0x2c,false);a.compare(4,0x0,false);
                if(!a.equal()) {
                    a.at(0x10366944);a.load<4>(3,16,0x2c,false);a.compare(3,0x0,false);
                    if(!a.equal()) {
                        a.at(0x10366950);a.load<1>(3,3,0x119,false);a.load<1>(0,4,0x119,false);a.sign8(3,3,false);
                        a.sign8(0,0,false);a.compare_registers(3,0,true);
                        if(a.equal()) {
                            a.at(0x10366968);a.load<4>(0,31,0x50,false);a.mask(0,0,0x20,true,0);
                            if(!a.equal()) {
                                a.at(0x10366974);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                                return;
                            }
                        }
                    }
                }
            }
        }
        a.at(0x1036697c);a.sign8(0,17,false);a.compare(0,0x5,true);
        if(a.equal()) {
            a.at(0x10366990);a.floating_load(31,30,0x2d8,false);a.jump(0x1036699c);
        } else {
            a.at(0x10366988);a.compare(0,0xc,true);
            if(!a.equal()) {
                a.at(0x10366998);a.floating_load(31,30,0x178,false);
            } else {
                a.at(0x10366990);a.floating_load(31,30,0x2d8,false);a.jump(0x1036699c);
            }
        }
        a.at(0x1036699c);a.compare(20,0x0,false);
        if(a.equal()) {
            a.at(0x103669a4);a.load<4>(3,14,0x0,false);a.load<4>(0,14,0x4,false);a.store<4>(1,0x40,3);
            a.load<2>(4,13,0x2,false);a.load<2>(3,1,0x42,false);a.store<4>(1,0x44,0);a.compare_registers(3,4,false);
            if(a.nonpositive()) {
                a.at(0x103669cc);
                if(a.at_least()) {
                    a.at(0x103669d8);a.load<2>(5,1,0x40,true);a.load<2>(0,13,0x0,true);a.combine(0,5,0,false,true);
                    a.shift_right(3,0,1);a.intersect(0,0,5);a.subtract(0,0,3);a.mask(0,0,0x1,false,1);
                    a.sign8(0,0,false);
                } else {
                    a.at(0x103669d0);a.value(0,0x0);a.jump(0x103669f8);
                }
            } else {
                a.at(0x103669c4);a.value(0,0x1);a.jump(0x103669f8);
            }
            a.at(0x103669f8);a.sign8(0,0,true);
            if(a.equal()) {
                a.at(0x10366a00);a.load<4>(3,15,0x4c,false);a.load<4>(0,15,0x50,false);a.store<4>(1,0x38,3);
                a.load<2>(5,14,0x2,false);a.load<2>(3,1,0x3a,false);a.store<4>(1,0x3c,0);a.compare_registers(3,5,false);
                a.load<2>(0,15,0x4c,true);a.load<2>(3,15,0x4e,false);
                if(a.at_least()) {
                    a.at(0x10366a30);
                    if(a.nonpositive()) {
                        a.at(0x10366a3c);a.load<2>(6,1,0x38,true);a.load<2>(8,14,0x0,true);a.combine(6,8,6,false,true);
                        a.shift_right(7,6,1);a.intersect(6,6,8);a.subtract(6,6,7);a.mask(6,6,0x1,false,1);
                        a.sign8(6,6,false);
                    } else {
                        a.at(0x10366a34);a.value(6,0x0);a.jump(0x10366a5c);
                    }
                } else {
                    a.at(0x10366a28);a.value(6,0x1);a.jump(0x10366a5c);
                }
                a.at(0x10366a5c);a.sign8(6,6,true);
                if(!a.equal()) {
                    a.at(0x10366a64);a.load<2>(0,14,0x0,true);a.move(3,5);
                }
                a.at(0x10366a6c);a.mask(5,3,0xffff,false,0);a.load<2>(3,13,0x0,true);a.subtract(4,5,4);
                a.sign16(0,0,false);a.multiply(5,4,0x16d);a.subtract(0,0,3);a.add(5,5,0);
                a.compare(5,0x1f4,true);
                if(!a.nonpositive()) {
                    a.at(0x10366a90);a.value(5,0x1f4);
                }
                a.at(0x10366a94);a.load<4>(3,15,0x1c,false);a.value(4,0x43300000);a.product(5,5,5);
                a.store<4>(1,0xc4,4);a.load<2>(0,3,0x2a,true);a.floating_load(3,30,0x8,false);a.exclusive_flags(0,0,0x80000000);
                a.store<4>(1,0xbc,4);a.store<4>(1,0xc8,0);a.exclusive_flags(0,5,0x80000000);a.floating_load(1,30,0x2e8,false);
                a.floating_load(0,1,0xc4,false);a.store<4>(1,0xc0,0);a.floating_subtract(0,0,3,false);a.floating_load(4,30,0x2e0,false);
                a.floating_load(2,1,0xbc,false);a.floating_subtract(2,2,3,false);a.floating_sum(0,1,0);a.floating_difference(0,2,0,true,false);
                a.floating_compare(4,0);
                if(a.at_least()) {
                    a.at(0x10366aec);a.floating_move(4,0);
                } else {
                    a.at(0x10366ae8);a.jump(0x10366af0);
                }
                a.at(0x10366af0);a.floating_sum(31,31,4);a.jump(0x10366afc);
            }
        }
    }
    a.at(0x10366afc);a.move(3,15);a.move(4,16);a.call(0x100da050);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x10366b10);a.floating_load(0,30,0x2f0,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
        if(!a.equal()) {
            a.at(0x10366b20);a.load<4>(0,18,0x0,false);a.flags(0,0,0x2);a.store<4>(18,0x0,0);
        }
    }
    a.at(0x10366b2c);a.move(3,15);a.move(4,20);a.call(0x100da050);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x10366b40);a.floating_load(0,30,0xe8,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
        if(!a.equal()) {
            a.at(0x10366b50);a.load<4>(0,18,0x0,false);a.flags(0,0,0x4);a.store<4>(18,0x0,0);
        }
    }
    a.at(0x10366b5c);a.move(3,15);a.move(4,20);a.call(0x100da0b0);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x10366b70);a.load<1>(0,20,0x23d,false);a.compare(0,0x1,true);
        if(a.equal()) {
            a.at(0x10366b94);a.floating_load(0,30,0x68,false);a.floating_product(31,31,0);
        } else {
            a.at(0x10366b7c);a.load<1>(0,16,0x23d,false);a.compare(0,0x1,true);
            if(!a.equal()) {
                a.at(0x10366b94);a.floating_load(0,30,0x68,false);a.floating_product(31,31,0);
            } else {
                a.at(0x10366b88);a.floating_load(0,30,0x2f8,false);a.floating_product(31,31,0);a.jump(0x10366b9c);
            }
        }
        a.at(0x10366b9c);a.compare(18,0x0,false);
        if(!a.equal()) {
            a.at(0x10366ba4);a.compare(31,0x0,false);
            if(!a.equal()) {
                a.at(0x10366bac);a.load<1>(0,31,0x54,false);a.sign8(0,0,true);
                if(a.equal()) {
                    a.at(0x10366bb8);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8);a.store<4>(18,0x0,0);
                }
            }
        }
    }
    a.at(0x10366bc4);a.move(3,15);a.move(4,16);a.call(0x100da0b0);
    a.sign8(0,3,true);
    if(!a.equal()) {
        a.at(0x10366bd8);a.compare(18,0x0,false);
        if(!a.equal()) {
            a.at(0x10366be0);a.load<4>(0,18,0x0,false);a.flags(0,0,0x10);a.store<4>(18,0x0,0);
        }
        a.at(0x10366bec);a.floating_load(0,30,0x140,false);a.floating_product(31,31,0);
    }
    a.at(0x10366bf4);a.compare(20,0x0,false);
    if(a.equal()) {
        a.at(0x10366e1c);a.compare(20,0x0,false);
        if(a.equal()) {
            return check_linked_country_reason(a);
        } else {
            a.at(0x10366e24);a.load<4>(3,16,0x2c,false);a.load<4>(0,25,0x0,false);a.load<4>(3,3,0x68,false);
            a.compare_registers(3,0,true);
            if(!a.equal()) {
                return check_linked_country_reason(a);
            } else {
                a.at(0x10366e38);a.load<4>(3,20,0x2c,false);a.load<4>(4,23,0x0,false);a.load<4>(0,3,0x68,false);
                a.compare_registers(0,4,true);
                if(!a.equal()) {
                    return check_linked_country_reason(a);
                } else {
                    a.at(0x10366e4c);a.load<4>(3,15,0xc,false);a.load<4>(0,3,0x68,false);a.compare_registers(0,4,true);
                    if(!a.equal()) {
                        return check_linked_country_reason(a);
                    } else {
                        a.at(0x10366e5c);a.floating_load(0,30,0x300,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
                        if(!a.equal()) {
                            a.at(0x10366e6c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x2);a.store<4>(18,0x0,0);
                            a.jump(0x10366ed8);
                        }
                    }
                }
            }
        }
    } else {
        a.at(0x10366bfc);a.compare_registers(20,16,false);
        if(a.equal()) {
            a.at(0x10366e1c);a.compare(20,0x0,false);
            if(a.equal()) {
                return check_linked_country_reason(a);
            } else {
                a.at(0x10366e24);a.load<4>(3,16,0x2c,false);a.load<4>(0,25,0x0,false);a.load<4>(3,3,0x68,false);
                a.compare_registers(3,0,true);
                if(!a.equal()) {
                    return check_linked_country_reason(a);
                } else {
                    a.at(0x10366e38);a.load<4>(3,20,0x2c,false);a.load<4>(4,23,0x0,false);a.load<4>(0,3,0x68,false);
                    a.compare_registers(0,4,true);
                    if(!a.equal()) {
                        return check_linked_country_reason(a);
                    } else {
                        a.at(0x10366e4c);a.load<4>(3,15,0xc,false);a.load<4>(0,3,0x68,false);a.compare_registers(0,4,true);
                        if(!a.equal()) {
                            return check_linked_country_reason(a);
                        } else {
                            a.at(0x10366e5c);a.floating_load(0,30,0x300,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
                            if(!a.equal()) {
                                a.at(0x10366e6c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x2);a.store<4>(18,0x0,0);
                                a.jump(0x10366ed8);
                            }
                        }
                    }
                }
            }
        } else {
            a.at(0x10366c04);a.load<4>(0,20,0x74,false);a.compare_registers(16,0,false);
            if(a.equal()) {
                a.at(0x10366c3c);a.load<4>(3,2,-0x7a04,false);a.load<4>(4,16,0x0,false);a.load<4>(5,3,0x0,false);
                a.compare_registers(4,5,true);
                if(a.equal()) {
                    a.at(0x10366c60);a.load<4>(4,20,0x0,false);a.compare_registers(4,5,true);
                    if(a.equal()) {
                        return calculate_personal_context_factor(a);
                    } else {
                        a.at(0x10366c6c);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                        if(!a.equal()) {
                            return compare_context_country(a);
                        } else {
                            return calculate_personal_context_factor(a);
                        }
                    }
                } else {
                    a.at(0x10366c50);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                    if(!a.equal()) {
                        return compare_context_country(a);
                    } else {
                        a.at(0x10366c60);a.load<4>(4,20,0x0,false);a.compare_registers(4,5,true);
                        if(a.equal()) {
                            return calculate_personal_context_factor(a);
                        } else {
                            a.at(0x10366c6c);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                            if(!a.equal()) {
                                return compare_context_country(a);
                            } else {
                                return calculate_personal_context_factor(a);
                            }
                        }
                    }
                }
            } else {
                a.at(0x10366c10);a.load<4>(0,20,0x78,false);a.compare_registers(16,0,false);
                if(a.equal()) {
                    a.at(0x10366c3c);a.load<4>(3,2,-0x7a04,false);a.load<4>(4,16,0x0,false);a.load<4>(5,3,0x0,false);
                    a.compare_registers(4,5,true);
                    if(a.equal()) {
                        a.at(0x10366c60);a.load<4>(4,20,0x0,false);a.compare_registers(4,5,true);
                        if(a.equal()) {
                            return calculate_personal_context_factor(a);
                        } else {
                            a.at(0x10366c6c);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                            if(!a.equal()) {
                                return compare_context_country(a);
                            } else {
                                return calculate_personal_context_factor(a);
                            }
                        }
                    } else {
                        a.at(0x10366c50);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                        if(!a.equal()) {
                            return compare_context_country(a);
                        } else {
                            a.at(0x10366c60);a.load<4>(4,20,0x0,false);a.compare_registers(4,5,true);
                            if(a.equal()) {
                                return calculate_personal_context_factor(a);
                            } else {
                                a.at(0x10366c6c);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                                if(!a.equal()) {
                                    return compare_context_country(a);
                                } else {
                                    return calculate_personal_context_factor(a);
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x10366c1c);a.load<4>(0,20,0x7c,false);a.compare_registers(16,0,false);
                    if(a.equal()) {
                        a.at(0x10366c3c);a.load<4>(3,2,-0x7a04,false);a.load<4>(4,16,0x0,false);a.load<4>(5,3,0x0,false);
                        a.compare_registers(4,5,true);
                        if(a.equal()) {
                            a.at(0x10366c60);a.load<4>(4,20,0x0,false);a.compare_registers(4,5,true);
                            if(a.equal()) {
                                return calculate_personal_context_factor(a);
                            } else {
                                a.at(0x10366c6c);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                                if(!a.equal()) {
                                    return compare_context_country(a);
                                } else {
                                    return calculate_personal_context_factor(a);
                                }
                            }
                        } else {
                            a.at(0x10366c50);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                            if(!a.equal()) {
                                return compare_context_country(a);
                            } else {
                                a.at(0x10366c60);a.load<4>(4,20,0x0,false);a.compare_registers(4,5,true);
                                if(a.equal()) {
                                    return calculate_personal_context_factor(a);
                                } else {
                                    a.at(0x10366c6c);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                                    if(!a.equal()) {
                                        return compare_context_country(a);
                                    } else {
                                        return calculate_personal_context_factor(a);
                                    }
                                }
                            }
                        }
                    } else {
                        a.at(0x10366c28);a.move(3,16);a.move(4,20);a.call(0x10464160);
                        a.sign8(0,3,true);
                        if(a.equal()) {
                            a.at(0x10366e1c);a.compare(20,0x0,false);
                            if(a.equal()) {
                                return check_linked_country_reason(a);
                            } else {
                                a.at(0x10366e24);a.load<4>(3,16,0x2c,false);a.load<4>(0,25,0x0,false);a.load<4>(3,3,0x68,false);
                                a.compare_registers(3,0,true);
                                if(!a.equal()) {
                                    return check_linked_country_reason(a);
                                } else {
                                    a.at(0x10366e38);a.load<4>(3,20,0x2c,false);a.load<4>(4,23,0x0,false);a.load<4>(0,3,0x68,false);
                                    a.compare_registers(0,4,true);
                                    if(!a.equal()) {
                                        return check_linked_country_reason(a);
                                    } else {
                                        a.at(0x10366e4c);a.load<4>(3,15,0xc,false);a.load<4>(0,3,0x68,false);a.compare_registers(0,4,true);
                                        if(!a.equal()) {
                                            return check_linked_country_reason(a);
                                        } else {
                                            a.at(0x10366e5c);a.floating_load(0,30,0x300,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
                                            if(!a.equal()) {
                                                a.at(0x10366e6c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x2);a.store<4>(18,0x0,0);
                                                a.jump(0x10366ed8);
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            a.at(0x10366c3c);a.load<4>(3,2,-0x7a04,false);a.load<4>(4,16,0x0,false);a.load<4>(5,3,0x0,false);
                            a.compare_registers(4,5,true);
                            if(a.equal()) {
                                a.at(0x10366c60);a.load<4>(4,20,0x0,false);a.compare_registers(4,5,true);
                                if(a.equal()) {
                                    return calculate_personal_context_factor(a);
                                } else {
                                    a.at(0x10366c6c);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                                    if(!a.equal()) {
                                        return compare_context_country(a);
                                    } else {
                                        return calculate_personal_context_factor(a);
                                    }
                                }
                            } else {
                                a.at(0x10366c50);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                                if(!a.equal()) {
                                    return compare_context_country(a);
                                } else {
                                    a.at(0x10366c60);a.load<4>(4,20,0x0,false);a.compare_registers(4,5,true);
                                    if(a.equal()) {
                                        return calculate_personal_context_factor(a);
                                    } else {
                                        a.at(0x10366c6c);a.load<4>(3,2,-0x7a08,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
                                        if(!a.equal()) {
                                            return compare_context_country(a);
                                        } else {
                                            return calculate_personal_context_factor(a);
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
    return adjust_context_club_services(a);
}

void calculate_personal_context_factor(Call& a) {
    a.at(0x10366c7c);a.load<1>(3,15,0x65,false);a.value(0,0x43300000);a.store<4>(1,0xbc,0);
    a.sign8(0,3,false);a.floating_load(2,30,0x8,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(0,30,0x98,false);
    a.store<4>(1,0xc0,0);a.floating_load(3,30,0x68,false);a.floating_load(1,1,0xbc,false);a.floating_subtract(1,1,2,false);
    a.floating_difference(0,1,0,true,false);a.floating_subtract(1,3,0,false);a.jump(0x10366df4);
    return apply_context_factor_and_reasons(a);
}

void compare_context_country(Call& a) {
    a.at(0x10366cb4);a.load<4>(4,16,0x2c,false);a.load<4>(3,2,-0x7a14,false);a.load<4>(4,4,0x68,false);
    a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
    if(!a.equal()) {
        a.at(0x10366d04);a.load<4>(0,20,0x74,false);a.compare_registers(16,0,false);
        if(a.equal()) {
            return compare_linked_competition(a);
        } else {
            a.at(0x10366d10);a.load<4>(0,20,0x78,false);a.compare_registers(16,0,false);
            if(a.equal()) {
                return compare_linked_competition(a);
            } else {
                a.at(0x10366d1c);a.load<4>(0,20,0x7c,false);a.compare_registers(16,0,false);
                if(!a.equal()) {
                    return calculate_cross_competition_factor(a);
                } else {
                    return compare_linked_competition(a);
                }
            }
        }
    } else {
        a.at(0x10366ccc);a.load<1>(3,15,0x65,false);a.value(0,0x43300000);a.store<4>(1,0xbc,0);
        a.sign8(0,3,false);a.floating_load(2,30,0x8,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(0,30,0x90,false);
        a.store<4>(1,0xc0,0);a.floating_load(3,30,0x80,false);a.floating_load(1,1,0xbc,false);a.floating_subtract(1,1,2,false);
        a.floating_difference(0,1,0,true,false);a.floating_subtract(1,3,0,false);a.jump(0x10366df4);
    }
    return apply_context_factor_and_reasons(a);
}

void compare_linked_competition(Call& a) {
    a.at(0x10366d28);a.load<4>(3,16,0x30,false);a.load<4>(0,20,0x30,false);a.compare_registers(3,0,false);
    if(!a.equal()) {
        return calculate_cross_competition_factor(a);
    } else {
        a.at(0x10366d38);a.load<2>(3,20,0x1ec,true);a.load<2>(0,16,0x1ec,true);a.subtract(3,3,0);
        a.call(0x10775540);a.compare(3,0x4e2,true);
        if(a.at_least()) {
            a.at(0x10366d88);a.load<1>(3,15,0x65,false);a.value(0,0x43300000);a.store<4>(1,0xbc,0);
            a.sign8(0,3,false);a.floating_load(2,30,0x8,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(0,30,0x90,false);
            a.store<4>(1,0xc0,0);a.floating_load(3,30,0xf8,false);a.floating_load(1,1,0xbc,false);a.floating_subtract(1,1,2,false);
            a.floating_difference(0,1,0,true,false);a.floating_subtract(1,3,0,false);a.jump(0x10366df4);
        } else {
            a.at(0x10366d50);a.load<1>(3,15,0x65,false);a.value(0,0x43300000);a.store<4>(1,0xbc,0);
            a.sign8(0,3,false);a.floating_load(2,30,0x8,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(0,30,0x98,false);
            a.store<4>(1,0xc0,0);a.floating_load(3,30,0xa0,false);a.floating_load(1,1,0xbc,false);a.floating_subtract(1,1,2,false);
            a.floating_difference(0,1,0,true,false);a.floating_subtract(1,3,0,false);a.jump(0x10366df4);
        }
    }
    return apply_context_factor_and_reasons(a);
}

void calculate_cross_competition_factor(Call& a) {
    a.at(0x10366dc0);a.load<1>(3,15,0x65,false);a.value(0,0x43300000);a.store<4>(1,0xbc,0);
    a.sign8(0,3,false);a.floating_load(2,30,0x8,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(0,30,0x98,false);
    a.store<4>(1,0xc0,0);a.floating_load(3,30,0x2f8,false);a.floating_load(1,1,0xbc,false);a.floating_subtract(1,1,2,false);
    a.floating_difference(0,1,0,true,false);a.floating_subtract(1,3,0,false);
    return apply_context_factor_and_reasons(a);
}

void apply_context_factor_and_reasons(Call& a) {
    a.at(0x10366df4);a.floating_product(31,31,1);a.compare(18,0x0,false);
    if(!a.equal()) {
        a.at(0x10366e00);a.floating_load(0,30,0x1f0,false);a.floating_compare(1,0);
        if(!a.at_least()) {
            a.at(0x10366e0c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x20);a.store<4>(18,0x0,0);
            a.jump(0x10366ed8);
        }
    }
    return adjust_context_club_services(a);
}

void check_linked_country_reason(Call& a) {
    a.at(0x10366e7c);a.compare(20,0x0,false);
    if(!a.equal()) {
        a.at(0x10366e84);a.load<4>(3,16,0x2c,false);a.load<4>(0,23,0x0,false);a.load<4>(3,3,0x68,false);
        a.compare_registers(3,0,true);
        if(a.equal()) {
            a.at(0x10366e98);a.load<4>(3,20,0x2c,false);a.load<4>(4,25,0x0,false);a.load<4>(0,3,0x68,false);
            a.compare_registers(0,4,true);
            if(a.equal()) {
                a.at(0x10366eac);a.load<4>(3,15,0xc,false);a.load<4>(0,3,0x68,false);a.compare_registers(0,4,true);
                if(a.equal()) {
                    a.at(0x10366ebc);a.floating_load(0,30,0x48,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
                    if(!a.equal()) {
                        a.at(0x10366ecc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x2);a.store<4>(18,0x0,0);
                    }
                }
            }
        }
    }
    return adjust_context_club_services(a);
}

void adjust_context_club_services(Call& a) {
    a.at(0x10366ed8);a.sign8(0,17,false);a.compare(0,0x5,true);
    if(!a.equal()) {
        a.at(0x10366ee4);a.compare(0,0xc,true);
        if(!a.equal()) {
            a.at(0x10366eec);a.load<4>(3,16,0x90,false);a.move(4,15);a.call(0x100d9520);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x10366f00);a.floating_load(0,30,0xe8,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
                if(!a.equal()) {
                    a.at(0x10366f10);a.load<4>(0,18,0x0,false);a.flags(0,0,0x40);a.store<4>(18,0x0,0);
                }
            }
        }
    }
    a.at(0x10366f1c);a.sign8(0,17,false);a.compare(0,0x5,true);
    if(!a.equal()) {
        a.at(0x10366f28);a.compare(0,0xc,true);
        if(!a.equal()) {
            a.at(0x10366f30);a.load<4>(3,16,0x90,false);a.move(4,15);a.call(0x100d9580);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x10366f44);a.floating_load(0,30,0x2f0,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
                if(!a.equal()) {
                    a.at(0x10366f54);a.load<4>(0,18,0x0,false);a.flags(0,0,0x80);a.store<4>(18,0x0,0);
                }
            }
        }
    }
    a.at(0x10366f60);a.compare(20,0x0,false);
    if(!a.equal()) {
        a.at(0x10366f68);a.load<4>(3,20,0x90,false);a.move(4,15);a.call(0x100d9580);
        a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x10366f7c);a.floating_load(0,30,0xe8,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
            if(!a.equal()) {
                a.at(0x10366f8c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x100);a.store<4>(18,0x0,0);
            }
        }
    }
    a.at(0x10366f98);a.compare(20,0x0,false);
    if(!a.equal()) {
        a.at(0x10366fa0);a.load<4>(3,20,0x90,false);a.move(4,15);a.call(0x100d9520);
        a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x10366fb4);a.floating_load(0,30,0x68,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
            if(!a.equal()) {
                a.at(0x10366fc4);a.load<4>(0,18,0x0,false);a.flags(0,0,0x200);a.store<4>(18,0x0,0);
            }
        }
    }
    a.at(0x10366fd0);a.load<4>(0,15,0x14,false);a.compare(0,0x0,false);
    if(!a.equal()) {
        a.at(0x10366fdc);a.load<4>(3,2,-0x7b40,false);a.move(4,15);a.value(5,0x0);
        a.load<4>(3,3,0x0,false);a.call(0x103f43b0);a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x10366ff8);a.compare(18,0x0,false);
            if(!a.equal()) {
                a.at(0x10367000);a.load<4>(0,18,0x0,false);a.flags(0,0,0x10000000);a.store<4>(18,0x0,0);
            }
            a.at(0x1036700c);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
            return;
        }
    }
    a.at(0x10367014);a.compare(31,0x0,false);
    if(!a.equal()) {
        a.at(0x1036702c);a.compare_registers(16,20,false);
        if(!a.equal()) {
            a.at(0x10367074);a.load<1>(3,31,0x4c,false);a.value(0,0x43300000);a.store<4>(1,0xbc,0);
            a.compare(18,0x0,false);a.sign8(0,3,false);a.floating_load(2,30,0x8,false);a.exclusive_flags(0,0,0x80000000);
            a.floating_load(0,30,0x308,false);a.store<4>(1,0xc0,0);a.floating_load(3,30,0x80,false);a.floating_load(1,1,0xbc,false);
            a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_subtract(1,3,0,false);a.floating_product(31,31,1);
            if(!a.equal()) {
                a.at(0x103670b4);a.load<1>(0,31,0x5c,false);a.mask(0,0,0x8,true,0);
                if(a.equal()) {
                    a.at(0x103670c0);a.floating_load(0,30,0x178,false);a.floating_compare(1,0);
                    if(a.nonpositive()) {
                        a.at(0x103670dc);a.floating_load(0,30,0x68,false);a.floating_compare(1,0);
                        if(!a.at_least()) {
                            a.at(0x103670e8);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800);a.store<4>(18,0x0,0);
                        }
                    } else {
                        a.at(0x103670cc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x400);a.store<4>(18,0x0,0);
                        a.jump(0x103670f4);
                    }
                }
            }
        } else {
            a.at(0x10367034);a.load<1>(3,31,0x4c,false);a.value(0,0x43300000);a.store<4>(1,0xbc,0);
            a.move(19,17);a.sign8(0,3,false);a.floating_load(2,30,0x8,false);a.exclusive_flags(0,0,0x80000000);
            a.floating_load(0,30,0x308,false);a.store<4>(1,0xc0,0);a.floating_load(3,30,0x80,false);a.floating_load(1,1,0xbc,false);
            a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_sum(0,3,0);a.floating_product(31,31,0);
            a.jump(0x103670f4);
        }
    } else {
        a.at(0x1036701c);a.compare_registers(16,20,false);
        if(a.equal()) {
            a.at(0x10367024);a.move(19,17);a.jump(0x103670f4);
        }
    }
    a.at(0x103670f4);a.compare_registers(16,20,false);
    if(!a.equal()) {
        a.at(0x103670fc);a.sign8(0,19,false);a.compare(0,0x10,false);
        if(a.positive()) {
            a.at(0x103672e0);a.floating_load(0,30,0x150,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
            if(!a.equal()) {
                a.at(0x103672f0);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8000000);a.store<4>(18,0x0,0);
            }
        } else {
            a.at(0x10367108);a.load<4>(3,2,-0x4df8,false);a.mask(0,0,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
            a.count_destination(3);a.count_branch();
            switch(a.r(0)>>2) {
                case 0:case 11: {
                    a.at(0x103672e0);a.floating_load(0,30,0x150,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
                    if(!a.equal()) {
                        a.at(0x103672f0);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8000000);a.store<4>(18,0x0,0);
                    }
                    break;
                }
                case 1: {
                    a.at(0x1036711c);a.floating_load(0,30,0x148,false);a.floating_product(31,31,0);a.jump(0x103672fc);
                    break;
                }
                case 2:case 3:case 4: {
                    a.at(0x103671a0);a.sign8(0,17,false);a.compare(0,0x5,true);
                    if(!a.equal()) {
                        a.at(0x103671b8);a.floating_load(0,30,0x148,false);a.floating_product(31,31,0);a.jump(0x103672fc);
                    } else {
                        a.at(0x103671ac);a.load<2>(0,16,0x1ec,true);a.compare(0,0x1770,true);
                        if(a.at_least()) {
                            a.at(0x103671c4);a.floating_load(0,30,0x68,false);a.floating_product(31,31,0);a.jump(0x103672fc);
                        } else {
                            a.at(0x103671b8);a.floating_load(0,30,0x148,false);a.floating_product(31,31,0);a.jump(0x103672fc);
                        }
                    }
                    break;
                }
                case 5:case 12: {
                    a.at(0x10367128);a.compare(20,0x0,false);
                    if(a.equal()) {
                        a.at(0x10367150);a.floating_load(0,30,0x1b8,false);a.floating_product(31,31,0);a.jump(0x103672fc);
                    } else {
                        a.at(0x10367130);a.floating_load(0,30,0x310,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
                        if(!a.equal()) {
                            a.at(0x10367140);a.load<4>(0,18,0x0,false);a.flags(0,0,0x4000000);a.store<4>(18,0x0,0);
                            a.jump(0x103672fc);
                        }
                    }
                    break;
                }
                case 6:case 13: {
                    a.at(0x103671d0);a.floating_load(0,30,0x178,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
                    if(!a.equal()) {
                        a.at(0x103671e0);a.sign8(0,17,false);a.compare(0,0x5,true);
                        if(a.equal()) {
                            a.at(0x103671f4);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8000000);a.store<4>(18,0x0,0);
                            a.jump(0x103672fc);
                        } else {
                            a.at(0x103671ec);a.compare(0,0xc,true);
                            if(a.equal()) {
                                a.at(0x103671f4);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8000000);a.store<4>(18,0x0,0);
                                a.jump(0x103672fc);
                            }
                        }
                    }
                    break;
                }
                case 7:case 14: {
                    a.at(0x10367268);a.floating_load(0,30,0x318,false);a.compare(18,0x0,false);a.floating_product(31,31,0);
                    if(!a.equal()) {
                        a.at(0x10367278);a.sign8(0,17,false);a.compare(0,0x5,true);
                        if(a.equal()) {
                            a.at(0x1036728c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8000000);a.store<4>(18,0x0,0);
                            a.jump(0x103672fc);
                        } else {
                            a.at(0x10367284);a.compare(0,0xc,true);
                            if(a.equal()) {
                                a.at(0x1036728c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8000000);a.store<4>(18,0x0,0);
                                a.jump(0x103672fc);
                            }
                        }
                    }
                    break;
                }
                case 8:case 15: {
                    a.at(0x10367204);a.load<1>(3,15,0x63,false);a.value(0,0x43300000);a.store<4>(1,0xbc,0);
                    a.compare(18,0x0,false);a.sign8(0,3,false);a.floating_load(2,30,0x8,false);a.exclusive_flags(0,0,0x80000000);
                    a.floating_load(0,30,0x238,false);a.store<4>(1,0xc0,0);a.floating_load(3,30,0xf8,false);a.floating_load(1,1,0xbc,false);
                    a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_sum(0,3,0);a.floating_product(31,31,0);
                    if(!a.equal()) {
                        a.at(0x10367244);a.sign8(0,17,false);a.compare(0,0x5,true);
                        if(a.equal()) {
                            a.at(0x10367258);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8000000);a.store<4>(18,0x0,0);
                            a.jump(0x103672fc);
                        } else {
                            a.at(0x10367250);a.compare(0,0xc,true);
                            if(a.equal()) {
                                a.at(0x10367258);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8000000);a.store<4>(18,0x0,0);
                                a.jump(0x103672fc);
                            }
                        }
                    }
                    break;
                }
                case 9:case 10: {
                    a.at(0x1036729c);a.compare(18,0x0,false);
                    if(!a.equal()) {
                        a.at(0x103672a4);a.offset(0,17,-0xc);a.mask(0,0,0xff,false,0);a.compare(0,0x3,false);
                        if(a.nonpositive()) {
                            a.at(0x103672d0);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8000000);a.store<4>(18,0x0,0);
                            a.jump(0x103672fc);
                        } else {
                            a.at(0x103672b4);a.offset(0,17,-0x5);a.mask(0,0,0xff,false,0);a.compare(0,0x2,false);
                            if(a.nonpositive()) {
                                a.at(0x103672d0);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8000000);a.store<4>(18,0x0,0);
                                a.jump(0x103672fc);
                            } else {
                                a.at(0x103672c4);a.sign8(0,17,false);a.compare(0,0x8,true);
                                if(a.equal()) {
                                    a.at(0x103672d0);a.load<4>(0,18,0x0,false);a.flags(0,0,0x8000000);a.store<4>(18,0x0,0);
                                    a.jump(0x103672fc);
                                }
                            }
                        }
                    }
                    break;
                }
                case 16: {
                    a.at(0x1036715c);a.load<1>(0,15,0x63,false);a.value(3,0x1);a.sign8(0,0,false);
                    a.compare(0,0x1,true);
                    if(!a.nonpositive()) {
                        a.at(0x10367170);a.move(3,0);
                    }
                    a.at(0x10367174);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xc0,3);
                    a.floating_load(1,30,0x8,false);a.store<4>(1,0xbc,0);a.floating_load(2,30,0x150,false);a.floating_load(0,1,0xbc,false);
                    a.floating_subtract(0,0,1,false);a.floating_difference(0,2,0,true,false);a.floating_product(31,31,0);a.jump(0x103672fc);
                    break;
                }
            }
        }
        a.at(0x103672fc);a.sign8(14,17,false);a.compare(14,0xf,false);
        if(a.positive()) {
            a.at(0x10367410);a.floating_load(0,30,0x150,false);a.sign8(0,19,false);a.compare(0,0x5,true);
            a.floating_difference(31,31,0,true,false);
            if(a.equal()) {
                a.at(0x1036742c);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);
            } else {
                a.at(0x10367424);a.compare(0,0xc,true);
                if(a.equal()) {
                    a.at(0x1036742c);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);
                }
            }
        } else {
            a.at(0x10367308);a.load<4>(3,2,-0x4dfc,false);a.mask(0,14,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
            a.count_destination(3);a.count_branch();
            switch(a.r(0)>>2) {
                case 0: {
                    a.at(0x1036731c);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
                    return;
                    break;
                }
                case 1: {
                    a.at(0x10367324);a.floating_load(0,30,0x48,false);a.floating_difference(31,31,0,true,false);a.jump(0x10367434);
                    break;
                }
                case 2:case 3:case 4: {
                    a.at(0x10367330);a.floating_load(0,30,0x2f8,false);a.floating_difference(31,31,0,true,false);a.jump(0x10367434);
                    break;
                }
                case 5:case 12: {
                    a.at(0x1036733c);a.floating_load(0,30,0x310,false);a.sign8(0,19,false);a.compare(0,0x5,true);
                    a.floating_difference(31,31,0,true,false);
                    if(a.equal()) {
                        a.at(0x10367358);a.floating_load(0,30,0x178,false);a.floating_product(31,31,0);a.jump(0x10367434);
                    } else {
                        a.at(0x10367350);a.compare(0,0xc,true);
                        if(a.equal()) {
                            a.at(0x10367358);a.floating_load(0,30,0x178,false);a.floating_product(31,31,0);a.jump(0x10367434);
                        }
                    }
                    break;
                }
                case 6:case 13: {
                    a.at(0x10367364);a.floating_load(0,30,0x68,false);a.sign8(0,19,false);a.compare(0,0x5,true);
                    a.floating_product(31,31,0);
                    if(a.equal()) {
                        a.at(0x10367380);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);a.jump(0x10367434);
                    } else {
                        a.at(0x10367378);a.compare(0,0xc,true);
                        if(a.equal()) {
                            a.at(0x10367380);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);a.jump(0x10367434);
                        }
                    }
                    break;
                }
                case 7:case 8:case 14:case 15: {
                    a.at(0x1036738c);a.floating_load(0,30,0xe0,false);a.sign8(0,19,false);a.compare(0,0x5,true);
                    a.floating_difference(31,31,0,true,false);
                    if(a.equal()) {
                        a.at(0x103673a8);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);a.jump(0x10367434);
                    } else {
                        a.at(0x103673a0);a.compare(0,0xc,true);
                        if(a.equal()) {
                            a.at(0x103673a8);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);a.jump(0x10367434);
                        }
                    }
                    break;
                }
                case 9:case 10: {
                    a.at(0x103673b4);a.sign8(0,19,false);a.compare(0,0x9,true);
                    if(a.equal()) {
                        a.at(0x103673c8);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);a.jump(0x103673dc);
                    } else {
                        a.at(0x103673c0);a.compare(0,0xa,true);
                        if(!a.equal()) {
                            a.at(0x103673d4);a.floating_load(0,30,0x18,false);a.floating_product(31,31,0);
                        } else {
                            a.at(0x103673c8);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);a.jump(0x103673dc);
                        }
                    }
                    a.at(0x103673dc);a.load<4>(0,15,0x14,false);a.compare(0,0x0,false);
                    if(!a.equal()) {
                        a.at(0x103673e8);a.floating_load(0,30,0x18,false);a.floating_product(31,31,0);
                    }
                    a.at(0x103673f0);a.sign8(0,19,false);a.compare(0,0x5,true);
                    if(a.equal()) {
                        a.at(0x10367404);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);a.jump(0x10367434);
                    } else {
                        a.at(0x103673fc);a.compare(0,0xc,true);
                        if(a.equal()) {
                            a.at(0x10367404);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);a.jump(0x10367434);
                        }
                    }
                    break;
                }
                case 11: {
                    a.at(0x10367410);a.floating_load(0,30,0x150,false);a.sign8(0,19,false);a.compare(0,0x5,true);
                    a.floating_difference(31,31,0,true,false);
                    if(a.equal()) {
                        a.at(0x1036742c);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);
                    } else {
                        a.at(0x10367424);a.compare(0,0xc,true);
                        if(a.equal()) {
                            a.at(0x1036742c);a.floating_load(0,30,0xe0,false);a.floating_difference(31,31,0,true,false);
                        }
                    }
                    break;
                }
            }
        }
        a.at(0x10367434);a.compare(20,0x0,false);
        if(!a.equal()) {
            a.at(0x1036743c);a.sign8(0,19,false);a.compare(0,0x5,true);
            if(a.equal()) {
                a.at(0x10367450);a.load<4>(3,2,-0x79f0,false);a.load<4>(7,3,0x0,false);a.compare(7,0x0,false);
                if(!a.equal()) {
                    a.at(0x10367460);a.compare(20,0x0,false);
                    if(a.equal()) {
                        a.at(0x10367480);a.value(4,0x0);
                    } else {
                        a.at(0x10367468);a.load<4>(0,20,0x0,false);a.load<4>(3,7,0x0,false);a.multiply(0,0,0x4c);
                        a.add(3,3,0);a.load<2>(4,3,0x6,true);a.jump(0x10367484);
                    }
                    a.at(0x10367484);a.load<1>(3,15,0x64,false);a.compare(20,0x0,false);a.load<1>(0,15,0x65,false);
                    a.sign16(4,4,false);a.sign8(3,3,false);a.sign8(0,0,false);a.multiply(6,3,0x1f4);
                    a.multiply(5,0,0x1f4);
                    if(a.equal()) {
                        a.at(0x103674c0);a.value(0,0x0);
                    } else {
                        a.at(0x103674a8);a.load<4>(0,20,0x0,false);a.load<4>(3,7,0x0,false);a.multiply(0,0,0x4c);
                        a.add(3,3,0);a.load<2>(0,3,0x8,true);a.jump(0x103674c4);
                    }
                    a.at(0x103674c4);a.sign16(0,0,false);a.add(0,0,5);a.add(0,0,6);
                    a.add(0,4,0);a.compare(0,0x1f40,true);
                    if(!a.at_least()) {
                        a.at(0x103674dc);a.floating_load(0,30,0x2d0,false);a.floating_product(31,31,0);
                    }
                }
            } else {
                a.at(0x10367448);a.compare(0,0xc,true);
                if(a.equal()) {
                    a.at(0x10367450);a.load<4>(3,2,-0x79f0,false);a.load<4>(7,3,0x0,false);a.compare(7,0x0,false);
                    if(!a.equal()) {
                        a.at(0x10367460);a.compare(20,0x0,false);
                        if(a.equal()) {
                            a.at(0x10367480);a.value(4,0x0);
                        } else {
                            a.at(0x10367468);a.load<4>(0,20,0x0,false);a.load<4>(3,7,0x0,false);a.multiply(0,0,0x4c);
                            a.add(3,3,0);a.load<2>(4,3,0x6,true);a.jump(0x10367484);
                        }
                        a.at(0x10367484);a.load<1>(3,15,0x64,false);a.compare(20,0x0,false);a.load<1>(0,15,0x65,false);
                        a.sign16(4,4,false);a.sign8(3,3,false);a.sign8(0,0,false);a.multiply(6,3,0x1f4);
                        a.multiply(5,0,0x1f4);
                        if(a.equal()) {
                            a.at(0x103674c0);a.value(0,0x0);
                        } else {
                            a.at(0x103674a8);a.load<4>(0,20,0x0,false);a.load<4>(3,7,0x0,false);a.multiply(0,0,0x4c);
                            a.add(3,3,0);a.load<2>(0,3,0x8,true);a.jump(0x103674c4);
                        }
                        a.at(0x103674c4);a.sign16(0,0,false);a.add(0,0,5);a.add(0,0,6);
                        a.add(0,4,0);a.compare(0,0x1f40,true);
                        if(!a.at_least()) {
                            a.at(0x103674dc);a.floating_load(0,30,0x2d0,false);a.floating_product(31,31,0);
                        }
                    }
                }
            }
        }
        a.at(0x103674e4);a.load<4>(3,16,0x2c,false);a.load<1>(0,3,0x11c,false);a.sign8(24,0,false);
        a.compare(24,0x1,true);
        if(a.less()) {
            a.at(0x10367500);a.value(24,0x3);
        } else {
            a.at(0x103674f8);a.compare(24,0x3,true);
            if(!a.nonpositive()) {
                a.at(0x10367500);a.value(24,0x3);
            }
        }
        a.at(0x10367504);a.product(0,24,24);a.load<4>(3,16,0x3c,false);a.compare(3,0x0,false);
        a.sign8(24,0,false);
        if(a.equal()) {
            a.at(0x103675d4);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0xbc,0);
            a.exclusive_flags(0,3,0x80000000);a.floating_load(2,30,0x8,false);a.store<4>(1,0xc0,0);a.floating_load(0,30,0x160,false);
            a.floating_load(1,1,0xbc,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);
            a.floating_store(0,1,0xc4);a.load<4>(23,1,0xc8,false);
        } else {
            a.at(0x10367518);a.load<4>(3,3,0x10,false);a.compare(3,0x0,false);
            if(a.equal()) {
                a.at(0x103675d4);a.load<2>(3,16,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0xbc,0);
                a.exclusive_flags(0,3,0x80000000);a.floating_load(2,30,0x8,false);a.store<4>(1,0xc0,0);a.floating_load(0,30,0x160,false);
                a.floating_load(1,1,0xbc,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);
                a.floating_store(0,1,0xc4);a.load<4>(23,1,0xc8,false);
            } else {
                a.at(0x10367524);a.load<1>(3,3,0x37,false);a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x10367548);a.load<2>(6,16,0x1ec,true);a.value(4,0x43300000);a.sign8(0,3,false);
                    a.value(3,0x88890000);a.exclusive_flags(5,6,0x80000000);a.store<4>(1,0xbc,4);a.floating_load(2,30,0x8,false);
                    a.product(0,0,6);a.store<4>(1,0xc0,5);a.offset(3,3,-0x7777);a.floating_load(0,30,0x160,false);
                    a.floating_load(1,1,0xbc,false);a.store<4>(1,0xc4,4);a.floating_subtract(1,1,2,false);a.multiply_high(3,3,0);
                    a.floating_difference(1,1,0,true,false);a.add(0,3,0);a.shift_right(0,0,3);a.mask(3,0,0x1,false,1);
                    a.add(0,0,3);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xc8,0);a.floating_load(0,1,0xc4,false);
                    a.floating_subtract(0,0,2,false);a.floating_compare(1,0);
                    if(a.nonpositive()) {
                        a.at(0x103675b4);a.store<4>(1,0xd0,0);a.store<4>(1,0xcc,4);a.floating_load(0,1,0xcc,false);
                        a.floating_subtract(1,0,2,false);
                    } else {
                        a.at(0x103675b0);a.jump(0x103675c4);
                    }
                    a.at(0x103675c4);a.floating_integer(0,1);a.floating_store(0,1,0xd4);a.load<4>(23,1,0xd8,false);
                    a.jump(0x10367608);
                } else {
                    a.at(0x10367530);a.load<2>(3,16,0x1ec,true);a.mask(0,3,0x1,false,1);a.add(0,0,3);
                    a.shift_right(0,0,1);a.sign16(23,0,false);a.jump(0x10367608);
                }
            }
        }
        a.at(0x10367608);a.load<4>(5,16,0x30,false);a.compare(5,0x0,false);
        if(a.equal()) {
            a.at(0x10367684);a.load<2>(29,16,0x1ec,true);
        } else {
            a.at(0x10367614);a.load<2>(6,16,0x1ec,true);a.compare(6,0x1d4c,true);
            if(a.less()) {
                a.at(0x10367650);a.load<2>(4,5,0x18,true);a.mask(0,6,0x1,false,1);a.add(3,0,6);
                a.multiply(0,4,0xfa);a.shift_right(3,3,1);a.multiply(4,4,0x1f4);a.add(0,3,0);
                a.compare_registers(4,0,true);
                if(a.nonpositive()) {
                    a.at(0x10367678);a.move(4,0);
                } else {
                    a.at(0x10367674);a.jump(0x1036767c);
                }
                a.at(0x1036767c);a.sign16(29,4,false);a.jump(0x10367688);
            } else {
                a.at(0x10367620);a.value(3,0x10000);a.load<4>(4,16,0x8,false);a.offset(0,3,-0x7748);
                a.compare_registers(4,0,true);
                if(a.nonpositive()) {
                    a.at(0x10367650);a.load<2>(4,5,0x18,true);a.mask(0,6,0x1,false,1);a.add(3,0,6);
                    a.multiply(0,4,0xfa);a.shift_right(3,3,1);a.multiply(4,4,0x1f4);a.add(0,3,0);
                    a.compare_registers(4,0,true);
                    if(a.nonpositive()) {
                        a.at(0x10367678);a.move(4,0);
                    } else {
                        a.at(0x10367674);a.jump(0x1036767c);
                    }
                    a.at(0x1036767c);a.sign16(29,4,false);a.jump(0x10367688);
                } else {
                    a.at(0x10367634);a.load<2>(0,5,0x18,true);a.multiply(0,0,0x1f4);a.compare_registers(0,6,true);
                    if(!a.nonpositive()) {
                        a.at(0x10367644);a.move(6,0);
                    }
                    a.at(0x10367648);a.sign16(29,6,false);a.jump(0x10367688);
                }
            }
        }
        a.at(0x10367688);a.load<4>(0,16,0x14,false);a.compare(0,-0x1,true);
        if(a.nonpositive()) {
            a.at(0x103676a8);a.load<4>(3,2,-0x7984,false);a.load<4>(0,16,0x0,false);a.load<4>(3,3,0x0,false);
            a.mask(0,0,0xfffffff8,false,3);a.indexed_load<4>(0,3,0);a.compare(0,-0x1,true);
            if(!a.nonpositive()) {
                a.at(0x103676c4);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,0,0x6c);a.load<4>(3,3,0x0,false);
                a.add(22,3,0);
            }
        } else {
            a.at(0x10367694);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,0,0x6c);a.load<4>(3,3,0x0,false);
            a.add(22,3,0);a.jump(0x103676d4);
        }
        a.at(0x103676d4);a.compare(22,0x0,false);
        if(a.equal()) {
            a.at(0x103676ec);a.sign16(3,29,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xbc,0);a.floating_load(2,30,0x8,false);a.store<4>(1,0xc0,3);a.floating_load(3,30,0xf8,false);
            a.floating_load(1,1,0xbc,false);a.floating_load(0,30,0x320,false);a.floating_subtract(1,1,2,false);a.floating_product(1,3,1);
            a.floating_compare(1,0);
            if(a.at_least()) {
                a.at(0x10367724);a.floating_move(1,0);
            } else {
                a.at(0x10367720);a.jump(0x10367728);
            }
            a.at(0x10367728);a.floating_integer(0,1);a.floating_store(0,1,0xc4);a.load<4>(27,1,0xc8,false);
        } else {
            a.at(0x103676dc);a.load<2>(0,22,0x18,true);a.multiply(0,0,0x258);a.sign16(27,0,false);
            a.jump(0x10367734);
        }
        a.at(0x10367734);a.load<4>(3,16,0x90,false);a.compare(3,0x0,false);
        if(!a.equal()) {
            a.at(0x10367748);a.load<4>(0,3,0x1c,false);a.compare(0,0x0,false);
            if(!a.equal()) {
                a.at(0x1036775c);a.move(4,15);a.value(5,0x1);a.call(0x100cdef0);
                a.load<4>(4,2,-0x79f0,false);a.move(26,3);a.load<4>(4,4,0x0,false);a.compare(4,0x0,false);
                if(!a.equal()) {
                    a.at(0x1036777c);a.sign8(0,17,false);a.compare(0,0x5,true);
                    if(!a.equal()) {
                        a.at(0x10367788);a.compare(16,0x0,false);
                        if(a.equal()) {
                            a.at(0x103677a8);a.value(0,0x0);
                        } else {
                            a.at(0x10367790);a.load<4>(0,16,0x0,false);a.load<4>(3,4,0x0,false);a.multiply(0,0,0x4c);
                            a.add(3,3,0);a.load<2>(0,3,0x6,true);a.jump(0x103677ac);
                        }
                        a.at(0x103677ac);a.sign16(3,0,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                        a.store<4>(1,0xbc,0);a.floating_load(2,30,0x8,false);a.compare(16,0x0,false);a.store<4>(1,0xc0,3);
                        a.floating_load(3,30,0x138,false);a.floating_load(0,1,0xbc,false);a.floating_load(1,30,0x2f0,false);a.floating_subtract(2,0,2,false);
                        if(a.equal()) {
                            a.at(0x103677f4);a.value(0,0x0);
                        } else {
                            a.at(0x103677dc);a.load<4>(0,16,0x0,false);a.load<4>(3,4,0x0,false);a.multiply(0,0,0x4c);
                            a.add(3,3,0);a.load<2>(0,3,0x8,true);a.jump(0x103677f8);
                        }
                        a.at(0x103677f8);a.sign16(0,0,false);a.value(4,0x43300000);a.exclusive_flags(0,0,0x80000000);
                        a.store<4>(1,0xc4,4);a.floating_load(5,30,0x8,false);a.store<4>(1,0xc8,0);a.floating_load(0,1,0xc4,false);
                        a.floating_subtract(0,0,5,false);a.floating_product(0,1,0);a.floating_product_sum(0,3,2,0,false);a.floating_integer(0,0);
                        a.floating_store(0,1,0xcc);a.load<4>(0,1,0xd0,false);a.sign16(0,0,false);a.compare(0,0x1388,true);
                        if(a.at_least()) {
                            a.at(0x103678ac);a.exclusive_flags(3,0,0x80000000);a.sign16(0,26,false);a.store<4>(1,0xc0,3);
                            a.exclusive_flags(0,0,0x80000000);a.floating_load(4,30,0x338,false);a.store<4>(1,0xbc,4);a.floating_load(2,30,0x68,false);
                            a.floating_load(0,1,0xbc,false);a.store<4>(1,0xc8,0);a.floating_subtract(3,0,5,false);a.floating_load(1,30,0x330,false);
                            a.store<4>(1,0xc4,4);a.floating_load(0,1,0xc4,false);a.floating_product_sum(3,4,3,2,false);a.store<4>(1,0xd0,0);
                            a.floating_subtract(2,0,5,false);a.store<4>(1,0xcc,4);a.floating_load(0,1,0xcc,false);a.floating_product(2,2,3);
                            a.floating_subtract(0,0,5,false);a.floating_product_sum(0,1,3,0,false);a.floating_compare(2,0);
                            if(a.at_least()) {
                                a.at(0x1036790c);a.floating_move(2,0);
                            } else {
                                a.at(0x10367908);a.jump(0x10367910);
                            }
                            a.at(0x10367910);a.floating_integer(0,2);a.floating_store(0,1,0xd4);a.load<4>(26,1,0xd8,false);
                        } else {
                            a.at(0x10367838);a.exclusive_flags(3,0,0x80000000);a.sign16(0,26,false);a.store<4>(1,0xc0,3);
                            a.exclusive_flags(0,0,0x80000000);a.floating_load(4,30,0x328,false);a.store<4>(1,0xbc,4);a.floating_load(2,30,0x208,false);
                            a.floating_load(0,1,0xbc,false);a.store<4>(1,0xc8,0);a.floating_subtract(3,0,5,false);a.floating_load(1,30,0x330,false);
                            a.store<4>(1,0xc4,4);a.floating_load(0,1,0xc4,false);a.floating_negative_product_difference(3,4,3,2);a.store<4>(1,0xd0,0);
                            a.floating_subtract(0,0,5,false);a.store<4>(1,0xcc,4);a.floating_difference(2,0,3,true,false);a.floating_load(0,1,0xcc,false);
                            a.floating_subtract(0,0,5,false);a.floating_negative_product_difference(0,1,3,0);a.floating_compare(2,0);
                            if(a.nonpositive()) {
                                a.at(0x10367898);a.floating_move(2,0);
                            } else {
                                a.at(0x10367894);a.jump(0x1036789c);
                            }
                            a.at(0x1036789c);a.floating_integer(0,2);a.floating_store(0,1,0xd4);a.load<4>(26,1,0xd8,false);
                            a.jump(0x1036791c);
                        }
                    }
                }
            } else {
                a.at(0x10367754);a.value(26,0x0);a.jump(0x1036791c);
            }
        } else {
            a.at(0x10367740);a.value(26,0x0);a.jump(0x1036791c);
        }
        a.at(0x1036791c);a.load<2>(25,16,0x1ec,true);a.move(3,16);a.call(0x100c7d00);
        a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x10367930);a.load<4>(3,2,-0x7ab8,false);a.move(4,16);a.load<4>(3,3,0x0,false);
            a.call(0x10103510);a.compare(3,0x0,false);
            if(!a.equal()) {
                a.at(0x10367948);a.call(0x100f55b0);a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x10367954);
                    if(a.at_least()) {
                        a.at(0x10367968);a.compare(0,0x2,true);
                        if(a.equal()) {
                            a.at(0x10367978);a.exclusive_flags(3,25,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xc0,3);
                            a.floating_load(1,30,0x8,false);a.store<4>(1,0xbc,0);a.floating_load(2,30,0x178,false);a.floating_load(0,1,0xbc,false);
                            a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0xc4);
                            a.load<4>(25,1,0xc8,false);a.sign16(0,25,false);a.compare(0,0x1676,true);
                            if(!a.at_least()) {
                                a.at(0x103679b4);a.value(25,0x1676);a.jump(0x10367a64);
                            }
                        } else {
                            a.at(0x10367970);
                            if(!a.at_least()) {
                                a.at(0x10367974);a.jump(0x103679bc);a.at(0x103679bc);a.exclusive_flags(3,25,0x80000000);
                                a.value(0,0x43300000);a.store<4>(1,0xc0,3);a.floating_load(1,30,0x8,false);a.store<4>(1,0xbc,0);
                                a.floating_load(2,30,0xc8,false);a.floating_load(0,1,0xbc,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                a.floating_integer(0,0);a.floating_store(0,1,0xc4);a.load<4>(25,1,0xc8,false);a.sign16(0,25,false);
                                a.compare(0,0x109a,true);
                                if(!a.at_least()) {
                                    a.at(0x103679f8);a.value(25,0x109a);a.jump(0x10367a64);
                                }
                            }
                        }
                    } else {
                        a.at(0x10367958);a.compare(0,-0x2,true);
                        if(a.equal()) {
                            a.at(0x10367a34);a.exclusive_flags(3,25,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xc0,3);
                            a.floating_load(1,30,0x8,false);a.store<4>(1,0xbc,0);a.floating_load(2,30,0x1f0,false);a.floating_load(0,1,0xbc,false);
                            a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0xc4);
                            a.load<4>(25,1,0xc8,false);
                        } else {
                            a.at(0x10367960);
                            if(a.at_least()) {
                                a.at(0x10367a00);a.exclusive_flags(3,25,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xc0,3);
                                a.floating_load(1,30,0x8,false);a.store<4>(1,0xbc,0);a.floating_load(2,30,0x2f8,false);a.floating_load(0,1,0xbc,false);
                                a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0xc4);
                                a.load<4>(25,1,0xc8,false);a.jump(0x10367a64);
                            } else {
                                a.at(0x10367964);a.jump(0x10367a64);
                            }
                        }
                    }
                }
            }
        }
        a.at(0x10367a64);a.move(3,16);a.call(0x100c7d00);a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x10367abc);a.load<4>(0,16,0x0,false);a.value(4,0x43300000);a.load<4>(3,1,0x188,false);
            a.sign16(5,25,false);a.multiply(0,0,0xc);a.store<4>(1,0xc4,4);a.load<4>(3,3,0xdc,false);
            a.exclusive_flags(5,5,0x80000000);a.floating_load(3,30,0x8,false);a.add(3,3,0);a.load<2>(0,3,0x6,true);
            a.store<4>(1,0xc0,5);a.exclusive_flags(0,0,0x80000000);a.floating_load(1,30,0x348,false);a.store<4>(1,0xc8,0);
            a.floating_load(4,30,0x180,false);a.floating_load(0,1,0xc4,false);a.store<4>(1,0xbc,4);a.floating_subtract(0,0,3,false);
            a.floating_load(2,1,0xbc,false);a.floating_product(0,1,0);a.floating_subtract(2,2,3,false);a.floating_product_sum(0,4,2,0,false);
            a.floating_integer(0,0);a.floating_store(0,1,0xcc);a.load<4>(7,1,0xd0,false);
        } else {
            a.at(0x10367a74);a.move(3,16);a.call(0x100ce250);a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x10367abc);a.load<4>(0,16,0x0,false);a.value(4,0x43300000);a.load<4>(3,1,0x188,false);
                a.sign16(5,25,false);a.multiply(0,0,0xc);a.store<4>(1,0xc4,4);a.load<4>(3,3,0xdc,false);
                a.exclusive_flags(5,5,0x80000000);a.floating_load(3,30,0x8,false);a.add(3,3,0);a.load<2>(0,3,0x6,true);
                a.store<4>(1,0xc0,5);a.exclusive_flags(0,0,0x80000000);a.floating_load(1,30,0x348,false);a.store<4>(1,0xc8,0);
                a.floating_load(4,30,0x180,false);a.floating_load(0,1,0xc4,false);a.store<4>(1,0xbc,4);a.floating_subtract(0,0,3,false);
                a.floating_load(2,1,0xbc,false);a.floating_product(0,1,0);a.floating_subtract(2,2,3,false);a.floating_product_sum(0,4,2,0,false);
                a.floating_integer(0,0);a.floating_store(0,1,0xcc);a.load<4>(7,1,0xd0,false);
            } else {
                a.at(0x10367a84);a.sign16(3,25,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xbc,0);a.floating_load(1,30,0x8,false);a.store<4>(1,0xc0,3);a.floating_load(2,30,0x340,false);
                a.floating_load(0,1,0xbc,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
                a.floating_store(0,1,0xc4);a.load<4>(7,1,0xc8,false);a.jump(0x10367b24);
            }
        }
        a.at(0x10367b24);a.sign16(0,23,false);a.value(6,0x43300000);a.exclusive_flags(0,0,0x80000000);
        a.sign16(5,27,false);a.store<4>(1,0xb8,0);a.sign16(3,26,false);a.exclusive_flags(0,3,0x80000000);
        a.exclusive_flags(5,5,0x80000000);a.store<4>(1,0xb4,6);a.sign16(3,29,false);a.floating_load(3,30,0x8,false);
        a.exclusive_flags(4,3,0x80000000);a.floating_load(0,1,0xb4,false);a.sign16(3,7,false);a.store<4>(1,0xd8,0);
        a.exclusive_flags(0,3,0x80000000);a.floating_subtract(0,0,3,true);a.load<4>(3,2,-0x4de8,false);a.store<4>(1,0xd4,6);
        a.floating_load(4,3,0xc,true);a.move(3,16);a.floating_load(1,1,0xd4,false);a.store<4>(1,0xd0,4);
        a.floating_product_single(0,27,0);a.floating_subtract(1,1,3,true);a.store<4>(1,0xcc,6);a.floating_load(2,1,0xcc,false);
        a.floating_product_sum(0,28,1,0,true);a.store<4>(1,0xc8,5);a.floating_subtract(1,2,3,true);a.store<4>(1,0xc4,6);
        a.floating_load(2,1,0xc4,false);a.floating_product_sum(0,26,1,0,true);a.store<4>(1,0xc0,0);a.floating_subtract(1,2,3,true);
        a.store<4>(1,0xbc,6);a.floating_load(2,1,0xbc,false);a.floating_product_sum(0,30,1,0,true);a.floating_subtract(1,2,3,true);
        a.floating_product_sum(0,4,1,0,true);a.floating_difference(0,0,25,true,true);a.floating_integer(0,0);a.floating_store(0,1,0xac);
        a.load<4>(26,1,0xb0,false);a.call(0x100c7d00);a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x10367be0);a.load<4>(3,16,0x2c,false);a.load<4>(0,15,0xc,false);a.compare_registers(3,0,false);
            if(a.equal()) {
                a.at(0x10367bf0);a.sign16(3,26,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xac,0);a.floating_load(1,30,0x8,false);a.store<4>(1,0xb0,3);a.floating_load(2,30,0x340,false);
                a.floating_load(0,1,0xac,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
                a.floating_store(0,1,0xb4);a.load<4>(26,1,0xb8,false);
            }
        }
        a.at(0x10367c24);a.move(3,16);a.call(0x100c7d00);a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x10367c34);a.load<1>(6,16,0x23d,false);a.compare(6,0x1,true);
            if(!a.equal()) {
                a.at(0x10367c40);a.load<1>(0,15,0x5e,false);a.sign16(5,26,false);a.mask(4,5,0x1,false,1);
                a.product(3,0,0);a.offset(0,5,-0x7d0);a.add(4,4,5);a.shift_right(4,4,1);
                a.multiply(3,3,0x5);a.subtract(0,3,0);a.compare_registers(4,0,true);
                if(a.at_least()) {
                    a.at(0x10367c70);a.move(4,0);
                } else {
                    a.at(0x10367c6c);a.jump(0x10367c74);
                }
                a.at(0x10367c74);a.sign8(0,6,false);a.sign16(26,4,false);a.compare(0,0x3,true);
                if(a.equal()) {
                    a.at(0x10367c84);a.mask(0,26,0x1,false,1);a.offset(3,26,-0x7d0);a.add(0,0,26);
                    a.shift_right(0,0,1);a.compare_registers(0,3,true);
                    if(a.at_least()) {
                        a.at(0x10367ca0);a.move(0,3);
                    } else {
                        a.at(0x10367c9c);a.jump(0x10367ca4);
                    }
                    a.at(0x10367ca4);a.sign16(26,0,false);
                }
            }
        }
        a.at(0x10367ca8);a.sign16(0,26,false);a.value(4,0x43300000);a.exclusive_flags(0,0,0x80000000);
        a.load<4>(5,16,0x2c,false);a.store<4>(1,0xb0,0);a.floating_load(2,30,0x8,false);a.compare(5,0x0,false);
        a.store<4>(1,0xac,4);a.floating_load(3,30,0x178,false);a.floating_load(1,1,0xac,false);a.floating_load(0,30,0x350,false);
        a.floating_subtract(1,1,2,false);a.floating_product_difference(0,3,1,0);a.floating_integer(0,0);a.floating_store(0,1,0xb4);
        a.load<4>(26,1,0xb8,false);
        if(!a.equal()) {
            a.at(0x10367cec);a.sign16(3,26,false);a.load<4>(0,15,0x14,false);a.offset(3,3,0x1388);
            a.store<4>(1,0xac,4);a.exclusive_flags(3,3,0x80000000);a.floating_load(0,30,0x358,false);a.store<4>(1,0xb0,3);
            a.compare(0,0x0,false);a.floating_load(1,1,0xac,false);a.store<4>(1,0xb8,3);a.floating_subtract(1,1,2,false);
            a.store<4>(1,0xb4,4);a.floating_difference(1,1,0,true,false);a.floating_load(0,1,0xb4,false);a.floating_subtract(0,0,2,false);
            a.floating_product(24,1,0);
            if(a.equal()) {
                a.at(0x10367d70);a.floating_load(3,30,0x150,false);
            } else {
                a.at(0x10367d30);a.load<4>(3,2,-0x7b3c,false);a.move(4,15);a.load<1>(6,5,0x119,false);
                a.move(5,16);a.load<4>(3,3,0x0,false);a.value(7,0x1);a.call(0x103092f0);
                a.sign8(3,3,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xbc,0);
                a.floating_load(1,30,0x8,false);a.store<4>(1,0xc0,3);a.floating_load(0,1,0xbc,false);a.floating_subtract(3,0,1,false);
                a.jump(0x10367d74);
            }
            a.at(0x10367d74);a.floating_load(1,30,0x40,false);a.floating_load(0,30,0x360,false);a.floating_load(2,30,0x150,false);
            a.floating_negative_product_difference(0,1,3,0);a.floating_difference(1,3,2,true,false);a.floating_difference(0,24,0,true,false);a.floating_compare(1,0);
            if(a.nonpositive()) {
                a.at(0x10367d98);a.floating_move(1,0);
            } else {
                a.at(0x10367d94);a.jump(0x10367d9c);
            }
            a.at(0x10367d9c);a.floating_load(0,30,0x58,false);a.compare(18,0x0,false);a.floating_sum(1,0,1);
            a.floating_product(31,31,1);
            if(!a.equal()) {
                a.at(0x10367db0);a.floating_load(0,30,0x180,false);a.floating_compare(1,0);
                if(!a.at_least()) {
                    a.at(0x10367dbc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x10000);a.store<4>(18,0x0,0);
                }
            }
        }
        a.at(0x10367dc8);a.compare(20,0x0,false);
        if(a.equal()) {
            a.at(0x10367df0);a.compare(20,0x0,false);
            if(a.equal()) {
                a.at(0x10367e08);a.sign8(0,17,false);a.compare(0,0x5,true);
                if(a.equal()) {
                    return select_secondary_category_scale(a);
                } else {
                    a.at(0x10367e14);a.compare(0,0xc,true);
                    if(a.equal()) {
                        return select_secondary_category_scale(a);
                    } else {
                        a.at(0x10367e1c);a.load<4>(3,16,0x90,false);a.compare(3,0x0,false);
                        if(a.equal()) {
                            return select_secondary_category_scale(a);
                        } else {
                            a.at(0x10367e28);a.load<4>(4,3,0xc,false);a.compare(4,0x0,false);
                            if(a.equal()) {
                                return select_secondary_category_scale(a);
                            } else {
                                a.at(0x10367e34);a.move(3,15);a.value(5,0x1);a.call(0x100cab10);
                                a.sign8(0,3,true);
                                if(!a.equal()) {
                                    return select_secondary_category_scale(a);
                                } else {
                                    a.at(0x10367e48);a.compare(14,0xf,false);
                                    if(a.positive()) {
                                        return apply_primary_category_fallback(a);
                                    } else {
                                        a.at(0x10367e50);a.load<4>(3,2,-0x4e00,false);a.mask(0,14,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
                                        a.count_destination(3);a.count_branch();
                                        switch(a.r(0)>>2) {
                                            case 0:case 1:case 5:case 11:case 12: {
                                                return apply_primary_category_fallback(a);
                                                break;
                                            }
                                            case 2:case 3:case 4: {
                                                a.at(0x10367e64);a.floating_load(0,30,0x2f8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                break;
                                            }
                                            case 6:case 13: {
                                                a.at(0x10367e70);a.floating_load(0,30,0xa0,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                break;
                                            }
                                            case 7:case 8:case 14:case 15: {
                                                a.at(0x10367e7c);a.floating_load(0,30,0x1b8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                break;
                                            }
                                            case 9:case 10: {
                                                a.at(0x10367e88);a.floating_load(0,30,0x68,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                a.at(0x10367df8);a.load<4>(3,20,0x2c,false);a.load<4>(0,16,0x2c,false);a.compare_registers(3,0,false);
                if(!a.equal()) {
                    a.at(0x10367e08);a.sign8(0,17,false);a.compare(0,0x5,true);
                    if(a.equal()) {
                        return select_secondary_category_scale(a);
                    } else {
                        a.at(0x10367e14);a.compare(0,0xc,true);
                        if(a.equal()) {
                            return select_secondary_category_scale(a);
                        } else {
                            a.at(0x10367e1c);a.load<4>(3,16,0x90,false);a.compare(3,0x0,false);
                            if(a.equal()) {
                                return select_secondary_category_scale(a);
                            } else {
                                a.at(0x10367e28);a.load<4>(4,3,0xc,false);a.compare(4,0x0,false);
                                if(a.equal()) {
                                    return select_secondary_category_scale(a);
                                } else {
                                    a.at(0x10367e34);a.move(3,15);a.value(5,0x1);a.call(0x100cab10);
                                    a.sign8(0,3,true);
                                    if(!a.equal()) {
                                        return select_secondary_category_scale(a);
                                    } else {
                                        a.at(0x10367e48);a.compare(14,0xf,false);
                                        if(a.positive()) {
                                            return apply_primary_category_fallback(a);
                                        } else {
                                            a.at(0x10367e50);a.load<4>(3,2,-0x4e00,false);a.mask(0,14,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
                                            a.count_destination(3);a.count_branch();
                                            switch(a.r(0)>>2) {
                                                case 0:case 1:case 5:case 11:case 12: {
                                                    return apply_primary_category_fallback(a);
                                                    break;
                                                }
                                                case 2:case 3:case 4: {
                                                    a.at(0x10367e64);a.floating_load(0,30,0x2f8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                    break;
                                                }
                                                case 6:case 13: {
                                                    a.at(0x10367e70);a.floating_load(0,30,0xa0,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                    break;
                                                }
                                                case 7:case 8:case 14:case 15: {
                                                    a.at(0x10367e7c);a.floating_load(0,30,0x1b8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                    break;
                                                }
                                                case 9:case 10: {
                                                    a.at(0x10367e88);a.floating_load(0,30,0x68,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                    break;
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
            a.at(0x10367dd0);a.load<4>(4,16,0x2c,false);a.compare(4,0x0,false);
            if(a.equal()) {
                a.at(0x10367df0);a.compare(20,0x0,false);
                if(a.equal()) {
                    a.at(0x10367e08);a.sign8(0,17,false);a.compare(0,0x5,true);
                    if(a.equal()) {
                        return select_secondary_category_scale(a);
                    } else {
                        a.at(0x10367e14);a.compare(0,0xc,true);
                        if(a.equal()) {
                            return select_secondary_category_scale(a);
                        } else {
                            a.at(0x10367e1c);a.load<4>(3,16,0x90,false);a.compare(3,0x0,false);
                            if(a.equal()) {
                                return select_secondary_category_scale(a);
                            } else {
                                a.at(0x10367e28);a.load<4>(4,3,0xc,false);a.compare(4,0x0,false);
                                if(a.equal()) {
                                    return select_secondary_category_scale(a);
                                } else {
                                    a.at(0x10367e34);a.move(3,15);a.value(5,0x1);a.call(0x100cab10);
                                    a.sign8(0,3,true);
                                    if(!a.equal()) {
                                        return select_secondary_category_scale(a);
                                    } else {
                                        a.at(0x10367e48);a.compare(14,0xf,false);
                                        if(a.positive()) {
                                            return apply_primary_category_fallback(a);
                                        } else {
                                            a.at(0x10367e50);a.load<4>(3,2,-0x4e00,false);a.mask(0,14,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
                                            a.count_destination(3);a.count_branch();
                                            switch(a.r(0)>>2) {
                                                case 0:case 1:case 5:case 11:case 12: {
                                                    return apply_primary_category_fallback(a);
                                                    break;
                                                }
                                                case 2:case 3:case 4: {
                                                    a.at(0x10367e64);a.floating_load(0,30,0x2f8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                    break;
                                                }
                                                case 6:case 13: {
                                                    a.at(0x10367e70);a.floating_load(0,30,0xa0,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                    break;
                                                }
                                                case 7:case 8:case 14:case 15: {
                                                    a.at(0x10367e7c);a.floating_load(0,30,0x1b8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                    break;
                                                }
                                                case 9:case 10: {
                                                    a.at(0x10367e88);a.floating_load(0,30,0x68,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x10367df8);a.load<4>(3,20,0x2c,false);a.load<4>(0,16,0x2c,false);a.compare_registers(3,0,false);
                    if(!a.equal()) {
                        a.at(0x10367e08);a.sign8(0,17,false);a.compare(0,0x5,true);
                        if(a.equal()) {
                            return select_secondary_category_scale(a);
                        } else {
                            a.at(0x10367e14);a.compare(0,0xc,true);
                            if(a.equal()) {
                                return select_secondary_category_scale(a);
                            } else {
                                a.at(0x10367e1c);a.load<4>(3,16,0x90,false);a.compare(3,0x0,false);
                                if(a.equal()) {
                                    return select_secondary_category_scale(a);
                                } else {
                                    a.at(0x10367e28);a.load<4>(4,3,0xc,false);a.compare(4,0x0,false);
                                    if(a.equal()) {
                                        return select_secondary_category_scale(a);
                                    } else {
                                        a.at(0x10367e34);a.move(3,15);a.value(5,0x1);a.call(0x100cab10);
                                        a.sign8(0,3,true);
                                        if(!a.equal()) {
                                            return select_secondary_category_scale(a);
                                        } else {
                                            a.at(0x10367e48);a.compare(14,0xf,false);
                                            if(a.positive()) {
                                                return apply_primary_category_fallback(a);
                                            } else {
                                                a.at(0x10367e50);a.load<4>(3,2,-0x4e00,false);a.mask(0,14,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
                                                a.count_destination(3);a.count_branch();
                                                switch(a.r(0)>>2) {
                                                    case 0:case 1:case 5:case 11:case 12: {
                                                        return apply_primary_category_fallback(a);
                                                        break;
                                                    }
                                                    case 2:case 3:case 4: {
                                                        a.at(0x10367e64);a.floating_load(0,30,0x2f8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                        break;
                                                    }
                                                    case 6:case 13: {
                                                        a.at(0x10367e70);a.floating_load(0,30,0xa0,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                        break;
                                                    }
                                                    case 7:case 8:case 14:case 15: {
                                                        a.at(0x10367e7c);a.floating_load(0,30,0x1b8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                        break;
                                                    }
                                                    case 9:case 10: {
                                                        a.at(0x10367e88);a.floating_load(0,30,0x68,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                        break;
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
                a.at(0x10367ddc);a.move(3,15);a.value(5,0x1);a.call(0x100cab10);
                a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x10367df0);a.compare(20,0x0,false);
                    if(a.equal()) {
                        a.at(0x10367e08);a.sign8(0,17,false);a.compare(0,0x5,true);
                        if(a.equal()) {
                            return select_secondary_category_scale(a);
                        } else {
                            a.at(0x10367e14);a.compare(0,0xc,true);
                            if(a.equal()) {
                                return select_secondary_category_scale(a);
                            } else {
                                a.at(0x10367e1c);a.load<4>(3,16,0x90,false);a.compare(3,0x0,false);
                                if(a.equal()) {
                                    return select_secondary_category_scale(a);
                                } else {
                                    a.at(0x10367e28);a.load<4>(4,3,0xc,false);a.compare(4,0x0,false);
                                    if(a.equal()) {
                                        return select_secondary_category_scale(a);
                                    } else {
                                        a.at(0x10367e34);a.move(3,15);a.value(5,0x1);a.call(0x100cab10);
                                        a.sign8(0,3,true);
                                        if(!a.equal()) {
                                            return select_secondary_category_scale(a);
                                        } else {
                                            a.at(0x10367e48);a.compare(14,0xf,false);
                                            if(a.positive()) {
                                                return apply_primary_category_fallback(a);
                                            } else {
                                                a.at(0x10367e50);a.load<4>(3,2,-0x4e00,false);a.mask(0,14,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
                                                a.count_destination(3);a.count_branch();
                                                switch(a.r(0)>>2) {
                                                    case 0:case 1:case 5:case 11:case 12: {
                                                        return apply_primary_category_fallback(a);
                                                        break;
                                                    }
                                                    case 2:case 3:case 4: {
                                                        a.at(0x10367e64);a.floating_load(0,30,0x2f8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                        break;
                                                    }
                                                    case 6:case 13: {
                                                        a.at(0x10367e70);a.floating_load(0,30,0xa0,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                        break;
                                                    }
                                                    case 7:case 8:case 14:case 15: {
                                                        a.at(0x10367e7c);a.floating_load(0,30,0x1b8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                        break;
                                                    }
                                                    case 9:case 10: {
                                                        a.at(0x10367e88);a.floating_load(0,30,0x68,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        a.at(0x10367df8);a.load<4>(3,20,0x2c,false);a.load<4>(0,16,0x2c,false);a.compare_registers(3,0,false);
                        if(!a.equal()) {
                            a.at(0x10367e08);a.sign8(0,17,false);a.compare(0,0x5,true);
                            if(a.equal()) {
                                return select_secondary_category_scale(a);
                            } else {
                                a.at(0x10367e14);a.compare(0,0xc,true);
                                if(a.equal()) {
                                    return select_secondary_category_scale(a);
                                } else {
                                    a.at(0x10367e1c);a.load<4>(3,16,0x90,false);a.compare(3,0x0,false);
                                    if(a.equal()) {
                                        return select_secondary_category_scale(a);
                                    } else {
                                        a.at(0x10367e28);a.load<4>(4,3,0xc,false);a.compare(4,0x0,false);
                                        if(a.equal()) {
                                            return select_secondary_category_scale(a);
                                        } else {
                                            a.at(0x10367e34);a.move(3,15);a.value(5,0x1);a.call(0x100cab10);
                                            a.sign8(0,3,true);
                                            if(!a.equal()) {
                                                return select_secondary_category_scale(a);
                                            } else {
                                                a.at(0x10367e48);a.compare(14,0xf,false);
                                                if(a.positive()) {
                                                    return apply_primary_category_fallback(a);
                                                } else {
                                                    a.at(0x10367e50);a.load<4>(3,2,-0x4e00,false);a.mask(0,14,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
                                                    a.count_destination(3);a.count_branch();
                                                    switch(a.r(0)>>2) {
                                                        case 0:case 1:case 5:case 11:case 12: {
                                                            return apply_primary_category_fallback(a);
                                                            break;
                                                        }
                                                        case 2:case 3:case 4: {
                                                            a.at(0x10367e64);a.floating_load(0,30,0x2f8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                            break;
                                                        }
                                                        case 6:case 13: {
                                                            a.at(0x10367e70);a.floating_load(0,30,0xa0,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                            break;
                                                        }
                                                        case 7:case 8:case 14:case 15: {
                                                            a.at(0x10367e7c);a.floating_load(0,30,0x1b8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                            break;
                                                        }
                                                        case 9:case 10: {
                                                            a.at(0x10367e88);a.floating_load(0,30,0x68,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                                                            break;
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
        return adjust_context_history_and_quote(a);
    }
    a.at(0x10367fd8);a.compare(20,0x0,false);
    if(!a.equal()) {
        a.at(0x10368070);a.load<4>(3,20,0x2c,false);a.compare(3,0x0,false);
        if(a.equal()) {
            a.at(0x1036809c);a.value(23,0x3);
        } else {
            a.at(0x1036807c);a.load<1>(0,3,0x11c,false);a.sign8(23,0,false);a.compare(23,0x1,true);
            if(a.less()) {
                a.at(0x10368094);a.value(23,0x3);a.jump(0x103680a0);
            } else {
                a.at(0x1036808c);a.compare(23,0x3,true);
                if(!a.nonpositive()) {
                    a.at(0x10368094);a.value(23,0x3);a.jump(0x103680a0);
                }
            }
        }
        a.at(0x103680a0);a.product(0,23,23);a.load<4>(3,20,0x3c,false);a.compare(3,0x0,false);
        a.sign8(23,0,false);
        if(a.equal()) {
            a.at(0x10368170);a.load<2>(3,20,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0xac,0);
            a.exclusive_flags(0,3,0x80000000);a.floating_load(2,30,0x8,false);a.store<4>(1,0xb0,0);a.floating_load(0,30,0x160,false);
            a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);
            a.floating_store(0,1,0xb4);a.load<4>(14,1,0xb8,false);
        } else {
            a.at(0x103680b4);a.load<4>(3,3,0x10,false);a.compare(3,0x0,false);
            if(a.equal()) {
                a.at(0x10368170);a.load<2>(3,20,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0xac,0);
                a.exclusive_flags(0,3,0x80000000);a.floating_load(2,30,0x8,false);a.store<4>(1,0xb0,0);a.floating_load(0,30,0x160,false);
                a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);
                a.floating_store(0,1,0xb4);a.load<4>(14,1,0xb8,false);
            } else {
                a.at(0x103680c0);a.load<1>(3,3,0x37,false);a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x103680e4);a.load<2>(6,20,0x1ec,true);a.value(4,0x43300000);a.sign8(0,3,false);
                    a.value(3,0x88890000);a.exclusive_flags(5,6,0x80000000);a.store<4>(1,0xac,4);a.floating_load(2,30,0x8,false);
                    a.product(0,0,6);a.store<4>(1,0xb0,5);a.offset(3,3,-0x7777);a.floating_load(0,30,0x160,false);
                    a.floating_load(1,1,0xac,false);a.store<4>(1,0xb4,4);a.floating_subtract(1,1,2,false);a.multiply_high(3,3,0);
                    a.floating_difference(1,1,0,true,false);a.add(0,3,0);a.shift_right(0,0,3);a.mask(3,0,0x1,false,1);
                    a.add(0,0,3);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xb8,0);a.floating_load(0,1,0xb4,false);
                    a.floating_subtract(0,0,2,false);a.floating_compare(1,0);
                    if(a.nonpositive()) {
                        a.at(0x10368150);a.store<4>(1,0xc0,0);a.store<4>(1,0xbc,4);a.floating_load(0,1,0xbc,false);
                        a.floating_subtract(1,0,2,false);
                    } else {
                        a.at(0x1036814c);a.jump(0x10368160);
                    }
                    a.at(0x10368160);a.floating_integer(0,1);a.floating_store(0,1,0xc4);a.load<4>(14,1,0xc8,false);
                    a.jump(0x103681a4);
                } else {
                    a.at(0x103680cc);a.load<2>(3,20,0x1ec,true);a.mask(0,3,0x1,false,1);a.add(0,0,3);
                    a.shift_right(0,0,1);a.sign16(14,0,false);a.jump(0x103681a4);
                }
            }
        }
        a.at(0x103681a4);a.load<4>(5,20,0x30,false);a.compare(5,0x0,false);
        if(a.equal()) {
            a.at(0x10368220);a.load<2>(28,20,0x1ec,true);
        } else {
            a.at(0x103681b0);a.load<2>(6,20,0x1ec,true);a.compare(6,0x1d4c,true);
            if(a.less()) {
                a.at(0x103681ec);a.load<2>(4,5,0x18,true);a.mask(0,6,0x1,false,1);a.add(3,0,6);
                a.multiply(0,4,0xfa);a.shift_right(3,3,1);a.multiply(4,4,0x1f4);a.add(0,3,0);
                a.compare_registers(4,0,true);
                if(a.nonpositive()) {
                    a.at(0x10368214);a.move(4,0);
                } else {
                    a.at(0x10368210);a.jump(0x10368218);
                }
                a.at(0x10368218);a.sign16(28,4,false);a.jump(0x10368224);
            } else {
                a.at(0x103681bc);a.value(3,0x10000);a.load<4>(4,20,0x8,false);a.offset(0,3,-0x7748);
                a.compare_registers(4,0,true);
                if(a.nonpositive()) {
                    a.at(0x103681ec);a.load<2>(4,5,0x18,true);a.mask(0,6,0x1,false,1);a.add(3,0,6);
                    a.multiply(0,4,0xfa);a.shift_right(3,3,1);a.multiply(4,4,0x1f4);a.add(0,3,0);
                    a.compare_registers(4,0,true);
                    if(a.nonpositive()) {
                        a.at(0x10368214);a.move(4,0);
                    } else {
                        a.at(0x10368210);a.jump(0x10368218);
                    }
                    a.at(0x10368218);a.sign16(28,4,false);a.jump(0x10368224);
                } else {
                    a.at(0x103681d0);a.load<2>(0,5,0x18,true);a.multiply(0,0,0x1f4);a.compare_registers(0,6,true);
                    if(!a.nonpositive()) {
                        a.at(0x103681e0);a.move(6,0);
                    }
                    a.at(0x103681e4);a.sign16(28,6,false);a.jump(0x10368224);
                }
            }
        }
        a.at(0x10368224);a.load<4>(4,20,0x14,false);a.compare(4,-0x1,true);
        if(a.nonpositive()) {
            a.at(0x10368260);a.load<4>(3,2,-0x7984,false);a.load<4>(0,20,0x0,false);a.load<4>(3,3,0x0,false);
            a.mask(0,0,0xfffffff8,false,3);a.indexed_load<4>(0,3,0);a.compare(0,-0x1,true);
            if(!a.nonpositive()) {
                a.at(0x1036827c);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,0,0x6c);a.load<4>(3,3,0x0,false);
                a.add(21,3,0);
            }
        } else {
            a.at(0x10368230);a.load<4>(3,2,-0x7a2c,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
            if(!a.equal()) {
                a.at(0x1036824c);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,4,0x6c);a.load<4>(3,3,0x0,false);
                a.add(21,3,0);a.jump(0x1036828c);
            } else {
                a.at(0x10368240);a.load<2>(0,20,0x1ec,true);a.compare(0,0x1388,true);
                if(a.at_least()) {
                    a.at(0x10368260);a.load<4>(3,2,-0x7984,false);a.load<4>(0,20,0x0,false);a.load<4>(3,3,0x0,false);
                    a.mask(0,0,0xfffffff8,false,3);a.indexed_load<4>(0,3,0);a.compare(0,-0x1,true);
                    if(!a.nonpositive()) {
                        a.at(0x1036827c);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,0,0x6c);a.load<4>(3,3,0x0,false);
                        a.add(21,3,0);
                    }
                } else {
                    a.at(0x1036824c);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,4,0x6c);a.load<4>(3,3,0x0,false);
                    a.add(21,3,0);a.jump(0x1036828c);
                }
            }
        }
        a.at(0x1036828c);a.compare(21,0x0,false);
        if(a.equal()) {
            a.at(0x103682a4);a.sign16(3,28,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);a.store<4>(1,0xb0,3);a.floating_load(3,30,0xf8,false);
            a.floating_load(1,1,0xac,false);a.floating_load(0,30,0x320,false);a.floating_subtract(1,1,2,false);a.floating_product(1,3,1);
            a.floating_compare(1,0);
            if(a.at_least()) {
                a.at(0x103682dc);a.floating_move(1,0);
            } else {
                a.at(0x103682d8);a.jump(0x103682e0);
            }
            a.at(0x103682e0);a.floating_integer(0,1);a.floating_store(0,1,0xb4);a.load<4>(13,1,0xb8,false);
        } else {
            a.at(0x10368294);a.load<2>(0,21,0x18,true);a.multiply(0,0,0x258);a.sign16(13,0,false);
            a.jump(0x103682ec);
        }
        a.at(0x103682ec);a.load<4>(3,20,0x90,false);a.compare(3,0x0,false);
        if(!a.equal()) {
            a.at(0x10368330);a.load<4>(4,3,0x1c,false);a.compare(4,0x0,false);
            if(!a.equal()) {
                a.at(0x10368374);a.load<4>(3,2,-0x79f0,false);a.load<2>(25,4,0x2a,true);a.load<4>(3,3,0x0,false);
                a.compare(3,0x0,false);
                if(!a.equal()) {
                    a.at(0x10368388);a.sign8(0,17,false);a.compare(0,0x5,true);
                    if(!a.equal()) {
                        a.at(0x10368394);a.compare(20,0x0,false);
                        if(a.equal()) {
                            a.at(0x103683b4);a.value(0,0x0);
                        } else {
                            a.at(0x1036839c);a.load<4>(0,20,0x0,false);a.load<4>(4,3,0x0,false);a.multiply(0,0,0x4c);
                            a.add(4,4,0);a.load<2>(0,4,0x6,true);a.jump(0x103683b8);
                        }
                        a.at(0x103683b8);a.sign16(4,0,false);a.value(0,0x43300000);a.exclusive_flags(4,4,0x80000000);
                        a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);a.compare(20,0x0,false);a.store<4>(1,0xb0,4);
                        a.floating_load(3,30,0x138,false);a.floating_load(0,1,0xac,false);a.floating_load(1,30,0x2f0,false);a.floating_subtract(2,0,2,false);
                        if(a.equal()) {
                            a.at(0x10368400);a.value(0,0x0);
                        } else {
                            a.at(0x103683e8);a.load<4>(0,20,0x0,false);a.load<4>(3,3,0x0,false);a.multiply(0,0,0x4c);
                            a.add(3,3,0);a.load<2>(0,3,0x8,true);a.jump(0x10368404);
                        }
                        a.at(0x10368404);a.sign16(0,0,false);a.value(4,0x43300000);a.exclusive_flags(0,0,0x80000000);
                        a.store<4>(1,0xb4,4);a.floating_load(5,30,0x8,false);a.store<4>(1,0xb8,0);a.floating_load(0,1,0xb4,false);
                        a.floating_subtract(0,0,5,false);a.floating_product(0,1,0);a.floating_product_sum(0,3,2,0,false);a.floating_integer(0,0);
                        a.floating_store(0,1,0xbc);a.load<4>(0,1,0xc0,false);a.sign16(0,0,false);a.compare(0,0x1388,true);
                        if(a.at_least()) {
                            a.at(0x103684b4);a.exclusive_flags(3,0,0x80000000);a.exclusive_flags(0,25,0x80000000);a.store<4>(1,0xb0,3);
                            a.floating_load(4,30,0x338,false);a.store<4>(1,0xac,4);a.floating_load(2,30,0x68,false);a.floating_load(0,1,0xac,false);
                            a.store<4>(1,0xb8,0);a.floating_subtract(3,0,5,false);a.floating_load(1,30,0x330,false);a.store<4>(1,0xb4,4);
                            a.floating_load(0,1,0xb4,false);a.floating_product_sum(3,4,3,2,false);a.store<4>(1,0xc0,0);a.floating_subtract(2,0,5,false);
                            a.store<4>(1,0xbc,4);a.floating_load(0,1,0xbc,false);a.floating_product(2,2,3);a.floating_subtract(0,0,5,false);
                            a.floating_product_sum(0,1,3,0,false);a.floating_compare(2,0);
                            if(a.at_least()) {
                                a.at(0x10368510);a.floating_move(2,0);
                            } else {
                                a.at(0x1036850c);a.jump(0x10368514);
                            }
                            a.at(0x10368514);a.floating_integer(0,2);a.floating_store(0,1,0xc4);a.load<4>(25,1,0xc8,false);
                        } else {
                            a.at(0x10368444);a.exclusive_flags(3,0,0x80000000);a.exclusive_flags(0,25,0x80000000);a.store<4>(1,0xb0,3);
                            a.floating_load(4,30,0x328,false);a.store<4>(1,0xac,4);a.floating_load(2,30,0x208,false);a.floating_load(0,1,0xac,false);
                            a.store<4>(1,0xb8,0);a.floating_subtract(3,0,5,false);a.floating_load(1,30,0x330,false);a.store<4>(1,0xb4,4);
                            a.floating_load(0,1,0xb4,false);a.floating_negative_product_difference(3,4,3,2);a.store<4>(1,0xc0,0);a.floating_subtract(0,0,5,false);
                            a.store<4>(1,0xbc,4);a.floating_difference(2,0,3,true,false);a.floating_load(0,1,0xbc,false);a.floating_subtract(0,0,5,false);
                            a.floating_negative_product_difference(0,1,3,0);a.floating_compare(2,0);
                            if(a.nonpositive()) {
                                a.at(0x103684a0);a.floating_move(2,0);
                            } else {
                                a.at(0x1036849c);a.jump(0x103684a4);
                            }
                            a.at(0x103684a4);a.floating_integer(0,2);a.floating_store(0,1,0xc4);a.load<4>(25,1,0xc8,false);
                            a.jump(0x10368520);
                        }
                    }
                }
            } else {
                a.at(0x1036833c);a.sign16(3,28,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xac,0);a.floating_load(1,30,0x8,false);a.store<4>(1,0xb0,3);a.floating_load(2,30,0xa0,false);
                a.floating_load(0,1,0xac,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
                a.floating_store(0,1,0xb4);a.load<4>(25,1,0xb8,false);a.jump(0x10368520);
            }
        } else {
            a.at(0x103682f8);a.sign16(3,28,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xac,0);a.floating_load(1,30,0x8,false);a.store<4>(1,0xb0,3);a.floating_load(2,30,0xa0,false);
            a.floating_load(0,1,0xac,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
            a.floating_store(0,1,0xb4);a.load<4>(25,1,0xb8,false);a.jump(0x10368520);
        }
        a.at(0x10368520);a.load<2>(27,20,0x1ec,true);a.move(3,20);a.call(0x100c7d00);
        a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x10368534);a.load<4>(3,2,-0x7ab8,false);a.move(4,20);a.load<4>(3,3,0x0,false);
            a.call(0x10103510);a.compare(3,0x0,false);
            if(!a.equal()) {
                a.at(0x1036854c);a.call(0x100f55b0);a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x10368558);
                    if(a.at_least()) {
                        a.at(0x1036856c);a.compare(0,0x2,true);
                        if(a.equal()) {
                            a.at(0x1036857c);a.exclusive_flags(3,27,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xb0,3);
                            a.floating_load(1,30,0x8,false);a.store<4>(1,0xac,0);a.floating_load(2,30,0x178,false);a.floating_load(0,1,0xac,false);
                            a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0xb4);
                            a.load<4>(27,1,0xb8,false);a.sign16(0,27,false);a.compare(0,0x1676,true);
                            if(!a.at_least()) {
                                a.at(0x103685b8);a.value(27,0x1676);a.jump(0x10368668);
                            }
                        } else {
                            a.at(0x10368574);
                            if(!a.at_least()) {
                                a.at(0x10368578);a.jump(0x103685c0);a.at(0x103685c0);a.exclusive_flags(3,27,0x80000000);
                                a.value(0,0x43300000);a.store<4>(1,0xb0,3);a.floating_load(1,30,0x8,false);a.store<4>(1,0xac,0);
                                a.floating_load(2,30,0xc8,false);a.floating_load(0,1,0xac,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                a.floating_integer(0,0);a.floating_store(0,1,0xb4);a.load<4>(27,1,0xb8,false);a.sign16(0,27,false);
                                a.compare(0,0x128e,true);
                                if(!a.at_least()) {
                                    a.at(0x103685fc);a.value(27,0x128e);a.jump(0x10368668);
                                }
                            }
                        }
                    } else {
                        a.at(0x1036855c);a.compare(0,-0x2,true);
                        if(a.equal()) {
                            a.at(0x10368638);a.exclusive_flags(3,27,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xb0,3);
                            a.floating_load(1,30,0x8,false);a.store<4>(1,0xac,0);a.floating_load(2,30,0xa0,false);a.floating_load(0,1,0xac,false);
                            a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0xb4);
                            a.load<4>(27,1,0xb8,false);
                        } else {
                            a.at(0x10368564);
                            if(a.at_least()) {
                                a.at(0x10368604);a.exclusive_flags(3,27,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xb0,3);
                                a.floating_load(1,30,0x8,false);a.store<4>(1,0xac,0);a.floating_load(2,30,0x168,false);a.floating_load(0,1,0xac,false);
                                a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0xb4);
                                a.load<4>(27,1,0xb8,false);a.jump(0x10368668);
                            } else {
                                a.at(0x10368568);a.jump(0x10368668);
                            }
                        }
                    }
                }
            }
        }
        a.at(0x10368668);a.move(3,20);a.call(0x100c7d00);a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x103686c0);a.load<4>(0,20,0x0,false);a.value(4,0x43300000);a.load<4>(3,1,0x188,false);
            a.sign16(5,27,false);a.multiply(0,0,0xc);a.store<4>(1,0xb4,4);a.load<4>(3,3,0xdc,false);
            a.exclusive_flags(5,5,0x80000000);a.floating_load(3,30,0x8,false);a.add(3,3,0);a.load<2>(0,3,0x6,true);
            a.store<4>(1,0xb0,5);a.exclusive_flags(0,0,0x80000000);a.floating_load(1,30,0x348,false);a.store<4>(1,0xb8,0);
            a.floating_load(4,30,0x180,false);a.floating_load(0,1,0xb4,false);a.store<4>(1,0xac,4);a.floating_subtract(0,0,3,false);
            a.floating_load(2,1,0xac,false);a.floating_product(0,1,0);a.floating_subtract(2,2,3,false);a.floating_product_sum(0,4,2,0,false);
            a.floating_integer(0,0);a.floating_store(0,1,0xbc);a.load<4>(7,1,0xc0,false);
        } else {
            a.at(0x10368678);a.move(3,20);a.call(0x100ce250);a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x103686c0);a.load<4>(0,20,0x0,false);a.value(4,0x43300000);a.load<4>(3,1,0x188,false);
                a.sign16(5,27,false);a.multiply(0,0,0xc);a.store<4>(1,0xb4,4);a.load<4>(3,3,0xdc,false);
                a.exclusive_flags(5,5,0x80000000);a.floating_load(3,30,0x8,false);a.add(3,3,0);a.load<2>(0,3,0x6,true);
                a.store<4>(1,0xb0,5);a.exclusive_flags(0,0,0x80000000);a.floating_load(1,30,0x348,false);a.store<4>(1,0xb8,0);
                a.floating_load(4,30,0x180,false);a.floating_load(0,1,0xb4,false);a.store<4>(1,0xac,4);a.floating_subtract(0,0,3,false);
                a.floating_load(2,1,0xac,false);a.floating_product(0,1,0);a.floating_subtract(2,2,3,false);a.floating_product_sum(0,4,2,0,false);
                a.floating_integer(0,0);a.floating_store(0,1,0xbc);a.load<4>(7,1,0xc0,false);
            } else {
                a.at(0x10368688);a.sign16(3,27,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xac,0);a.floating_load(1,30,0x8,false);a.store<4>(1,0xb0,3);a.floating_load(2,30,0x378,false);
                a.floating_load(0,1,0xac,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
                a.floating_store(0,1,0xb4);a.load<4>(7,1,0xb8,false);a.jump(0x10368728);
            }
        }
        a.at(0x10368728);a.sign16(0,14,false);a.value(6,0x43300000);a.exclusive_flags(0,0,0x80000000);
        a.sign16(5,13,false);a.store<4>(1,0xd0,0);a.sign16(3,25,false);a.exclusive_flags(0,3,0x80000000);
        a.exclusive_flags(5,5,0x80000000);a.store<4>(1,0xcc,6);a.sign16(3,28,false);a.floating_load(3,30,0x8,false);
        a.exclusive_flags(4,3,0x80000000);a.floating_load(0,1,0xcc,false);a.sign16(3,7,false);a.store<4>(1,0xc8,0);
        a.exclusive_flags(0,3,0x80000000);a.floating_subtract(0,0,3,true);a.load<4>(3,2,-0x4de8,false);a.store<4>(1,0xc4,6);
        a.floating_load(4,3,0xc,true);a.move(3,20);a.floating_load(1,1,0xc4,false);a.store<4>(1,0xc0,4);
        a.floating_product_single(0,27,0);a.floating_subtract(1,1,3,true);a.store<4>(1,0xbc,6);a.floating_load(2,1,0xbc,false);
        a.floating_product_sum(0,28,1,0,true);a.store<4>(1,0xb8,5);a.floating_subtract(1,2,3,true);a.store<4>(1,0xb4,6);
        a.floating_load(2,1,0xb4,false);a.floating_product_sum(0,26,1,0,true);a.store<4>(1,0xb0,0);a.floating_subtract(1,2,3,true);
        a.store<4>(1,0xac,6);a.floating_load(2,1,0xac,false);a.floating_product_sum(0,30,1,0,true);a.floating_subtract(1,2,3,true);
        a.floating_product_sum(0,4,1,0,true);a.floating_difference(0,0,25,true,true);a.floating_integer(0,0);a.floating_store(0,1,0xd4);
        a.load<4>(13,1,0xd8,false);a.call(0x100c7d00);a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x103687e4);a.load<1>(6,20,0x23d,false);a.compare(6,0x1,true);
            if(!a.equal()) {
                a.at(0x103687f0);a.load<1>(0,15,0x5e,false);a.sign16(5,13,false);a.mask(4,5,0x1,false,1);
                a.product(3,0,0);a.offset(0,5,-0x7d0);a.add(4,4,5);a.shift_right(4,4,1);
                a.multiply(3,3,0x5);a.subtract(0,3,0);a.compare_registers(4,0,true);
                if(a.at_least()) {
                    a.at(0x10368820);a.move(4,0);
                } else {
                    a.at(0x1036881c);a.jump(0x10368824);
                }
                a.at(0x10368824);a.sign8(0,6,false);a.sign16(13,4,false);a.compare(0,0x3,true);
                if(a.equal()) {
                    a.at(0x10368834);a.mask(0,13,0x1,false,1);a.offset(3,13,-0x7d0);a.add(0,0,13);
                    a.shift_right(0,0,1);a.compare_registers(0,3,true);
                    if(a.at_least()) {
                        a.at(0x10368850);a.move(0,3);
                    } else {
                        a.at(0x1036884c);a.jump(0x10368854);
                    }
                    a.at(0x10368854);a.sign16(13,0,false);
                }
            }
        }
        a.at(0x10368858);a.sign16(0,13,false);a.value(4,0x43300000);a.exclusive_flags(0,0,0x80000000);
        a.load<4>(5,20,0x2c,false);a.store<4>(1,0xb0,0);a.floating_load(2,30,0x8,false);a.compare(5,0x0,false);
        a.store<4>(1,0xac,4);a.floating_load(3,30,0x178,false);a.floating_load(1,1,0xac,false);a.floating_load(0,30,0x350,false);
        a.floating_subtract(1,1,2,false);a.floating_product_difference(0,3,1,0);a.floating_integer(0,0);a.floating_store(0,1,0xb4);
        a.load<4>(13,1,0xb8,false);
        if(!a.equal()) {
            a.at(0x1036889c);a.compare_registers(16,20,false);
            if(!a.equal()) {
                a.at(0x103688a4);a.sign16(3,13,false);a.load<4>(0,15,0x14,false);a.offset(3,3,0x1388);
                a.store<4>(1,0xac,4);a.exclusive_flags(3,3,0x80000000);a.floating_load(0,30,0x358,false);a.store<4>(1,0xb0,3);
                a.compare(0,0x0,false);a.floating_load(1,1,0xac,false);a.store<4>(1,0xb8,3);a.floating_subtract(1,1,2,false);
                a.store<4>(1,0xb4,4);a.floating_difference(1,1,0,true,false);a.floating_load(0,1,0xb4,false);a.floating_subtract(0,0,2,false);
                a.floating_product(24,1,0);
                if(a.equal()) {
                    a.at(0x10368928);a.floating_load(3,30,0x150,false);
                } else {
                    a.at(0x103688e8);a.load<4>(3,2,-0x7b3c,false);a.move(4,15);a.load<1>(6,5,0x119,false);
                    a.move(5,20);a.load<4>(3,3,0x0,false);a.value(7,0x1);a.call(0x103092f0);
                    a.sign8(3,3,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xbc,0);
                    a.floating_load(1,30,0x8,false);a.store<4>(1,0xc0,3);a.floating_load(0,1,0xbc,false);a.floating_subtract(3,0,1,false);
                    a.jump(0x1036892c);
                }
                a.at(0x1036892c);a.floating_load(1,30,0x40,false);a.floating_load(0,30,0x360,false);a.floating_load(2,30,0x150,false);
                a.floating_negative_product_difference(0,1,3,0);a.floating_difference(1,3,2,true,false);a.floating_difference(0,24,0,true,false);a.floating_compare(1,0);
                if(a.nonpositive()) {
                    a.at(0x10368950);a.floating_move(1,0);
                } else {
                    a.at(0x1036894c);a.jump(0x10368954);
                }
                a.at(0x10368954);a.floating_load(0,30,0x58,false);a.floating_sum(0,0,1);a.floating_difference(31,31,0,true,false);
            }
        }
    } else {
        a.at(0x10367fe0);a.load<4>(3,15,0xc,false);a.load<1>(0,3,0x11c,false);a.sign8(23,0,false);
        a.compare(23,0x1,true);
        if(a.less()) {
            a.at(0x10367ffc);a.value(23,0x3);
        } else {
            a.at(0x10367ff4);a.compare(23,0x3,true);
            if(!a.nonpositive()) {
                a.at(0x10367ffc);a.value(23,0x3);
            }
        }
        a.at(0x10368000);a.load<4>(5,15,0x1c,false);a.value(3,0x55550000);a.value(0,0x43300000);
        a.floating_load(2,30,0x8,false);a.load<2>(6,5,0x2c,true);a.product(8,23,23);a.load<2>(4,5,0x28,true);
        a.offset(7,3,0x5556);a.load<2>(5,5,0x2a,true);a.add(3,4,6);a.store<4>(1,0xac,0);
        a.add(0,5,3);a.floating_load(3,30,0x178,false);a.multiply_high(3,7,0);a.floating_load(0,30,0x350,false);
        a.sign8(23,8,false);a.mask(0,3,0x1,false,1);a.add(0,3,0);a.sign16(0,0,false);
        a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xb0,0);a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,false);
        a.floating_product_difference(0,3,1,0);a.floating_integer(0,0);a.floating_store(0,1,0xb4);a.load<4>(13,1,0xb8,false);
        a.jump(0x10368960);
    }
    a.at(0x10368960);a.compare_registers(16,20,false);
    if(!a.equal()) {
        a.at(0x10368968);a.load<4>(4,16,0x30,false);a.compare(4,0x0,false);
        if(a.equal()) {
            a.at(0x10368988);a.floating_load(0,30,0x48,false);a.floating_product(31,31,0);
        } else {
            a.at(0x10368974);a.load<4>(3,2,-0x7a0c,false);a.load<4>(4,4,0x0,false);a.load<4>(0,3,0x0,false);
            a.compare_registers(4,0,true);
            if(a.equal()) {
                a.at(0x10368988);a.floating_load(0,30,0x48,false);a.floating_product(31,31,0);
            }
        }
        a.at(0x10368990);a.load<4>(4,15,0x1c,false);a.value(3,0x55550000);a.value(6,0x43300000);
        a.load<2>(0,16,0x1ec,true);a.load<2>(8,4,0x2c,true);a.offset(9,3,0x5556);a.load<2>(3,4,0x28,true);
        a.sign16(5,26,false);a.load<2>(7,4,0x2a,true);a.sign16(4,13,false);a.add(3,3,8);
        a.store<4>(1,0xac,6);a.add(3,7,3);a.floating_load(5,30,0x8,false);a.multiply_high(7,9,3);
        a.floating_load(0,30,0x380,false);a.floating_load(2,30,0x80,false);a.compare_registers(5,4,true);a.mask(3,7,0x1,false,1);
        a.add(3,7,3);a.subtract(0,3,0);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xb0,0);
        a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,5,false);a.floating_difference(0,1,0,true,false);a.floating_sum(0,2,0);
        if(a.nonpositive()) {
            a.at(0x10368a50);
            if(a.positive()) {
                a.at(0x10368a90);a.sign16(3,26,false);a.sign16(0,13,false);a.compare_registers(3,0,true);
                if(a.positive()) {
                    a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                    a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                    a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                } else {
                    a.at(0x10368aa0);a.floating_load(1,30,0x150,false);a.floating_compare(31,1);
                    if(a.nonpositive()) {
                        a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                        a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                        a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                    } else {
                        a.at(0x10368aac);a.floating_load(1,30,0x30,false);a.floating_load(3,30,0x200,false);a.floating_difference(2,31,1,true,false);
                        a.floating_load(1,30,0x48,false);a.floating_subtract(2,3,2,false);a.floating_round_single(2,2);a.floating_compare(1,2);
                        if(a.nonpositive()) {
                            a.at(0x10368ad0);a.floating_load(1,30,0x80,false);a.floating_compare(1,2);
                            if(a.at_least()) {
                                a.at(0x10368ae0);a.floating_move(1,2);
                            } else {
                                a.at(0x10368adc);a.jump(0x10368ae4);
                            }
                        } else {
                            a.at(0x10368acc);a.jump(0x10368ae4);
                        }
                        a.at(0x10368ae4);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                        a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                        a.floating_round_single(3,1);a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,true);
                        a.floating_product_single(4,3,1);a.jump(0x10368b40);
                    }
                }
            } else {
                a.at(0x10368a54);a.sign8(0,17,false);a.compare(0,0x5,true);
                if(a.equal()) {
                    a.at(0x10368a90);a.sign16(3,26,false);a.sign16(0,13,false);a.compare_registers(3,0,true);
                    if(a.positive()) {
                        a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                        a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                        a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                    } else {
                        a.at(0x10368aa0);a.floating_load(1,30,0x150,false);a.floating_compare(31,1);
                        if(a.nonpositive()) {
                            a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                            a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                            a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                        } else {
                            a.at(0x10368aac);a.floating_load(1,30,0x30,false);a.floating_load(3,30,0x200,false);a.floating_difference(2,31,1,true,false);
                            a.floating_load(1,30,0x48,false);a.floating_subtract(2,3,2,false);a.floating_round_single(2,2);a.floating_compare(1,2);
                            if(a.nonpositive()) {
                                a.at(0x10368ad0);a.floating_load(1,30,0x80,false);a.floating_compare(1,2);
                                if(a.at_least()) {
                                    a.at(0x10368ae0);a.floating_move(1,2);
                                } else {
                                    a.at(0x10368adc);a.jump(0x10368ae4);
                                }
                            } else {
                                a.at(0x10368acc);a.jump(0x10368ae4);
                            }
                            a.at(0x10368ae4);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                            a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                            a.floating_round_single(3,1);a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,true);
                            a.floating_product_single(4,3,1);a.jump(0x10368b40);
                        }
                    }
                } else {
                    a.at(0x10368a60);a.compare(0,0xc,true);
                    if(a.equal()) {
                        a.at(0x10368a90);a.sign16(3,26,false);a.sign16(0,13,false);a.compare_registers(3,0,true);
                        if(a.positive()) {
                            a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                            a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                            a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                        } else {
                            a.at(0x10368aa0);a.floating_load(1,30,0x150,false);a.floating_compare(31,1);
                            if(a.nonpositive()) {
                                a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                                a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                                a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                            } else {
                                a.at(0x10368aac);a.floating_load(1,30,0x30,false);a.floating_load(3,30,0x200,false);a.floating_difference(2,31,1,true,false);
                                a.floating_load(1,30,0x48,false);a.floating_subtract(2,3,2,false);a.floating_round_single(2,2);a.floating_compare(1,2);
                                if(a.nonpositive()) {
                                    a.at(0x10368ad0);a.floating_load(1,30,0x80,false);a.floating_compare(1,2);
                                    if(a.at_least()) {
                                        a.at(0x10368ae0);a.floating_move(1,2);
                                    } else {
                                        a.at(0x10368adc);a.jump(0x10368ae4);
                                    }
                                } else {
                                    a.at(0x10368acc);a.jump(0x10368ae4);
                                }
                                a.at(0x10368ae4);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                                a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                                a.floating_round_single(3,1);a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,true);
                                a.floating_product_single(4,3,1);a.jump(0x10368b40);
                            }
                        }
                    } else {
                        a.at(0x10368a68);a.subtract(0,4,5);a.store<4>(1,0xac,6);a.exclusive_flags(0,0,0x80000000);
                        a.floating_load(2,30,0x178,false);a.store<4>(1,0xb0,0);a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,5,false);
                        a.floating_product(4,2,1);a.floating_round_single(4,4);a.jump(0x10368b40);
                    }
                }
            }
        } else {
            a.at(0x10368a00);a.load<1>(3,15,0x63,false);a.subtract(0,4,5);a.exclusive_flags(0,0,0x80000000);
            a.store<4>(1,0xac,6);a.sign8(3,3,false);a.floating_load(2,30,0x388,false);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xb8,0);a.floating_load(4,30,0xa0,false);a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);
            a.store<4>(1,0xb4,6);a.floating_subtract(3,1,5,false);a.floating_load(1,1,0xb4,false);a.floating_difference(2,3,2,true,false);
            a.floating_sum(2,4,2);a.floating_subtract(1,1,5,false);a.floating_product(4,2,1);a.floating_round_single(4,4);
            a.jump(0x10368b40);
        }
        a.at(0x10368b40);a.floating_load(1,30,0x158,false);a.floating_compare(4,1);
        if(!a.nonpositive()) {
            a.at(0x10368b4c);a.floating_load(1,30,0x80,false);a.floating_compare(29,1);
            if(!a.at_least()) {
                a.at(0x10368b58);a.floating_load(1,30,0xb8,false);a.floating_load(3,30,0x30,false);a.floating_difference(1,4,1,true,false);
                a.floating_compare(3,1);
                if(a.at_least()) {
                    a.at(0x10368b70);a.floating_move(3,1);
                } else {
                    a.at(0x10368b6c);a.jump(0x10368b74);
                }
                a.at(0x10368b74);a.floating_load(2,30,0x390,false);a.compare(18,0x0,false);a.floating_subtract(1,2,3,false);
                a.floating_product_sum(1,1,29,3,false);a.floating_difference(2,1,2,true,false);a.floating_product(31,31,2);
                if(!a.equal()) {
                    a.at(0x10368b90);a.floating_load(1,30,0x180,false);a.floating_compare(2,1);
                    if(!a.at_least()) {
                        a.at(0x10368b9c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x100000);a.store<4>(18,0x0,0);
                    }
                }
            }
        }
        a.at(0x10368ba8);a.sign8(0,19,false);a.compare(0,0x5,true);
        if(!a.equal()) {
            a.at(0x10368bb4);a.compare(0,0xc,true);
            if(!a.equal()) {
                a.at(0x10368bbc);a.sign8(0,17,false);a.compare(0,0x5,true);
                if(a.equal()) {
                    a.at(0x10368bd0);a.floating_load(1,30,0x158,false);a.floating_compare(4,1);
                    if(!a.at_least()) {
                        a.at(0x10368bdc);a.sign8(0,19,false);a.compare(0,0x8,true);
                        if(a.equal()) {
                            a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                            a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                            if(!a.at_least()) {
                                a.at(0x10368c14);a.floating_move(2,1);
                            }
                            a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                        } else {
                            a.at(0x10368be8);a.compare(0,0xf,true);
                            if(a.equal()) {
                                a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                if(!a.at_least()) {
                                    a.at(0x10368c14);a.floating_move(2,1);
                                }
                                a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                            } else {
                                a.at(0x10368bf0);a.compare(0,0x6,true);
                                if(!a.equal()) {
                                    a.at(0x10368c20);a.floating_load(1,30,0x150,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                    a.floating_load(1,30,0x58,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                    if(!a.at_least()) {
                                        a.at(0x10368c3c);a.floating_move(2,1);
                                    }
                                    a.at(0x10368c40);a.floating_product(4,4,2);
                                } else {
                                    a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                    a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                    if(!a.at_least()) {
                                        a.at(0x10368c14);a.floating_move(2,1);
                                    }
                                    a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x10368bc8);a.compare(0,0xc,true);
                    if(a.equal()) {
                        a.at(0x10368bd0);a.floating_load(1,30,0x158,false);a.floating_compare(4,1);
                        if(!a.at_least()) {
                            a.at(0x10368bdc);a.sign8(0,19,false);a.compare(0,0x8,true);
                            if(a.equal()) {
                                a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                if(!a.at_least()) {
                                    a.at(0x10368c14);a.floating_move(2,1);
                                }
                                a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                            } else {
                                a.at(0x10368be8);a.compare(0,0xf,true);
                                if(a.equal()) {
                                    a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                    a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                    if(!a.at_least()) {
                                        a.at(0x10368c14);a.floating_move(2,1);
                                    }
                                    a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                                } else {
                                    a.at(0x10368bf0);a.compare(0,0x6,true);
                                    if(!a.equal()) {
                                        a.at(0x10368c20);a.floating_load(1,30,0x150,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                        a.floating_load(1,30,0x58,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                        if(!a.at_least()) {
                                            a.at(0x10368c3c);a.floating_move(2,1);
                                        }
                                        a.at(0x10368c40);a.floating_product(4,4,2);
                                    } else {
                                        a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                        a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                        if(!a.at_least()) {
                                            a.at(0x10368c14);a.floating_move(2,1);
                                        }
                                        a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        a.at(0x10368c44);a.floating_load(1,30,0x3a0,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,4,1,true,false);
        a.floating_load(1,30,0x370,false);a.floating_sum(2,3,2);a.floating_compare(2,1);
        if(!a.at_least()) {
            a.at(0x10368c60);a.floating_move(2,1);
        }
        a.at(0x10368c64);a.floating_product(31,31,2);a.compare(18,0x0,false);
        if(!a.equal()) {
            a.at(0x10368c70);a.floating_load(1,30,0x180,false);a.floating_compare(2,1);
            if(a.at_least()) {
                a.at(0x10368cec);a.floating_load(1,30,0x178,false);a.floating_compare(2,1);
                if(!a.nonpositive()) {
                    a.at(0x10368cf8);a.floating_load(1,30,0x170,false);a.floating_compare(30,1);
                    if(!a.nonpositive()) {
                        a.at(0x10368d04);a.compare(21,0x0,false);
                        if(a.equal()) {
                            a.at(0x10368d0c);a.compare(22,0x0,false);
                            if(!a.equal()) {
                                a.at(0x10368d14);a.sign8(0,17,false);a.compare(0,0x5,true);
                                if(a.equal()) {
                                    a.at(0x10368d28);a.compare(31,0x0,false);
                                    if(a.equal()) {
                                        a.at(0x10368d3c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x200000);a.store<4>(18,0x0,0);
                                    } else {
                                        a.at(0x10368d30);a.load<1>(0,31,0x54,false);a.sign8(0,0,true);
                                        if(a.equal()) {
                                            a.at(0x10368d3c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x200000);a.store<4>(18,0x0,0);
                                        }
                                    }
                                } else {
                                    a.at(0x10368d20);a.compare(0,0xc,true);
                                    if(a.equal()) {
                                        a.at(0x10368d28);a.compare(31,0x0,false);
                                        if(a.equal()) {
                                            a.at(0x10368d3c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x200000);a.store<4>(18,0x0,0);
                                        } else {
                                            a.at(0x10368d30);a.load<1>(0,31,0x54,false);a.sign8(0,0,true);
                                            if(a.equal()) {
                                                a.at(0x10368d3c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x200000);a.store<4>(18,0x0,0);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                a.at(0x10368c7c);a.floating_load(1,30,0x170,false);a.floating_compare(30,1);
                if(a.nonpositive()) {
                    a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                    a.jump(0x10368d48);
                } else {
                    a.at(0x10368c88);a.compare(21,0x0,false);
                    if(a.equal()) {
                        a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                        a.jump(0x10368d48);
                    } else {
                        a.at(0x10368c90);a.compare(22,0x0,false);
                        if(!a.equal()) {
                            a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                            a.jump(0x10368d48);
                        } else {
                            a.at(0x10368c98);a.floating_load(1,30,0xc8,false);a.floating_compare(31,1);
                            if(a.nonpositive()) {
                                a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                                a.jump(0x10368d48);
                            } else {
                                a.at(0x10368ca4);a.sign8(0,19,false);a.compare(0,0x5,true);
                                if(a.equal()) {
                                    a.at(0x10368cb8);a.compare(31,0x0,false);
                                    if(a.equal()) {
                                        a.at(0x10368ccc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x400000);a.store<4>(18,0x0,0);
                                        a.jump(0x10368d48);
                                    } else {
                                        a.at(0x10368cc0);a.load<1>(0,31,0x54,false);a.sign8(0,0,true);
                                        if(!a.equal()) {
                                            a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                                            a.jump(0x10368d48);
                                        } else {
                                            a.at(0x10368ccc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x400000);a.store<4>(18,0x0,0);
                                            a.jump(0x10368d48);
                                        }
                                    }
                                } else {
                                    a.at(0x10368cb0);a.compare(0,0xc,true);
                                    if(!a.equal()) {
                                        a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                                        a.jump(0x10368d48);
                                    } else {
                                        a.at(0x10368cb8);a.compare(31,0x0,false);
                                        if(a.equal()) {
                                            a.at(0x10368ccc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x400000);a.store<4>(18,0x0,0);
                                            a.jump(0x10368d48);
                                        } else {
                                            a.at(0x10368cc0);a.load<1>(0,31,0x54,false);a.sign8(0,0,true);
                                            if(!a.equal()) {
                                                a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                                                a.jump(0x10368d48);
                                            } else {
                                                a.at(0x10368ccc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x400000);a.store<4>(18,0x0,0);
                                                a.jump(0x10368d48);
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
        a.at(0x10368d48);a.compare(20,0x0,false);
        if(a.equal()) {
            a.at(0x10368e50);a.compare(20,0x0,false);
            if(!a.equal()) {
                a.at(0x10368e58);a.load<4>(3,15,0xc,false);a.load<4>(4,3,0x74,false);a.compare(4,0x0,false);
                if(!a.equal()) {
                    a.at(0x10368e68);a.load<4>(3,2,-0x7a70,false);a.load<4>(0,4,0x8,false);a.load<4>(4,3,0x0,false);
                    a.compare_registers(0,4,true);
                    if(a.equal()) {
                        a.at(0x10368e7c);a.load<4>(3,16,0x2c,false);a.load<4>(5,3,0x74,false);a.compare(5,0x0,false);
                        if(!a.equal()) {
                            a.at(0x10368e8c);a.load<4>(0,5,0x8,false);a.compare_registers(0,4,true);
                            if(!a.equal()) {
                                a.at(0x10368e98);a.load<4>(3,20,0x2c,false);a.load<4>(0,3,0x74,false);a.compare_registers(5,0,false);
                                if(!a.equal()) {
                                    a.at(0x10368ea8);a.floating_load(1,30,0x348,false);a.compare(18,0x0,false);a.floating_product(31,31,1);
                                    if(!a.equal()) {
                                        a.at(0x10368eb8);a.load<4>(0,18,0x0,false);a.flags(0,0,0x2000000);a.store<4>(18,0x0,0);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            a.at(0x10368d50);a.sign8(3,23,false);a.sign8(4,24,false);a.compare_registers(3,4,true);
            if(a.equal()) {
                a.at(0x10368e50);a.compare(20,0x0,false);
                if(!a.equal()) {
                    a.at(0x10368e58);a.load<4>(3,15,0xc,false);a.load<4>(4,3,0x74,false);a.compare(4,0x0,false);
                    if(!a.equal()) {
                        a.at(0x10368e68);a.load<4>(3,2,-0x7a70,false);a.load<4>(0,4,0x8,false);a.load<4>(4,3,0x0,false);
                        a.compare_registers(0,4,true);
                        if(a.equal()) {
                            a.at(0x10368e7c);a.load<4>(3,16,0x2c,false);a.load<4>(5,3,0x74,false);a.compare(5,0x0,false);
                            if(!a.equal()) {
                                a.at(0x10368e8c);a.load<4>(0,5,0x8,false);a.compare_registers(0,4,true);
                                if(!a.equal()) {
                                    a.at(0x10368e98);a.load<4>(3,20,0x2c,false);a.load<4>(0,3,0x74,false);a.compare_registers(5,0,false);
                                    if(!a.equal()) {
                                        a.at(0x10368ea8);a.floating_load(1,30,0x348,false);a.compare(18,0x0,false);a.floating_product(31,31,1);
                                        if(!a.equal()) {
                                            a.at(0x10368eb8);a.load<4>(0,18,0x0,false);a.flags(0,0,0x2000000);a.store<4>(18,0x0,0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                a.at(0x10368d60);a.sign16(0,29,false);a.value(6,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.exclusive_flags(4,4,0x80000000);a.exclusive_flags(5,0,0x80000000);a.store<4>(1,0xb0,3);a.sign16(0,28,false);
                a.floating_load(5,30,0x8,false);a.store<4>(1,0xac,6);a.compare(0,0x1f4,true);a.value(3,0x1f4);
                a.floating_load(1,1,0xac,false);a.store<4>(1,0xb8,5);a.floating_subtract(3,1,5,false);a.store<4>(1,0xb4,6);
                a.floating_load(1,1,0xb4,false);a.store<4>(1,0xc0,4);a.floating_subtract(2,1,5,false);a.store<4>(1,0xbc,6);
                a.floating_load(1,1,0xbc,false);a.floating_product(4,3,2);a.floating_subtract(3,1,5,false);
                if(!a.nonpositive()) {
                    a.at(0x10368dbc);a.move(3,0);
                }
                a.at(0x10368dc0);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xc8,3);
                a.floating_load(2,30,0x8,false);a.store<4>(1,0xc4,0);a.floating_load(5,30,0x148,false);a.floating_load(1,1,0xc4,false);
                a.floating_subtract(1,1,2,false);a.floating_product(1,3,1);a.floating_difference(1,4,1,true,false);a.floating_compare(5,1);
                if(a.nonpositive()) {
                    a.at(0x10368df4);a.floating_load(5,30,0x150,false);a.floating_compare(5,1);
                    if(a.at_least()) {
                        a.at(0x10368e04);a.floating_move(5,1);
                    } else {
                        a.at(0x10368e00);a.jump(0x10368e08);
                    }
                } else {
                    a.at(0x10368df0);a.jump(0x10368e08);
                }
                a.at(0x10368e08);a.load<4>(4,16,0x2c,false);a.floating_product(31,31,5);a.load<4>(3,20,0x2c,false);
                a.load<4>(4,4,0x74,false);a.load<4>(0,3,0x74,false);a.compare_registers(4,0,false);
                if(!a.equal()) {
                    a.at(0x10368e24);a.floating_load(1,30,0x80,false);a.floating_compare(5,1);
                    if(!a.at_least()) {
                        a.at(0x10368e30);a.floating_load(1,30,0x58,false);a.compare(18,0x0,false);a.floating_product(31,31,1);
                        if(!a.equal()) {
                            a.at(0x10368e40);a.load<4>(0,18,0x0,false);a.flags(0,0,0x2000000);a.store<4>(18,0x0,0);
                            a.jump(0x10368ec4);
                        }
                    }
                }
            }
        }
        a.at(0x10368ec4);a.floating_product(31,31,0);a.compare(18,0x0,false);
        if(!a.equal()) {
            a.at(0x10368ed0);a.floating_load(1,30,0x180,false);a.floating_compare(0,1);
            if(!a.at_least()) {
                a.at(0x10368edc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
            }
        }
    }
    a.at(0x10368ee8);a.load<1>(0,1,0x197,false);a.sign8(0,0,true);
    if(a.equal()) {
        a.at(0x10368f00);a.floating_move(1,31);a.jump(0x10369058);
        return;
    }
    a.at(0x10368ef4);a.floating_load(0,30,0x3a8,false);a.floating_compare(31,0);
    if(!a.at_least()) {
        a.at(0x10368f00);a.floating_move(1,31);a.jump(0x10369058);
        return;
    }
    a.at(0x10368f08);a.offset(3,1,0x48);a.call(0x103721f0);a.load<4>(3,2,-0x7ab8,false);
    a.move(4,16);a.load<4>(3,3,0x0,false);a.call(0x10103510);a.combine(13,3,3,true,false);
    if(a.equal()) {
        a.at(0x10368f28);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
        return;
    }
    a.at(0x10368f30);a.move(4,15);a.move(5,16);a.move(8,17);
    a.offset(3,1,0x48);a.value(6,0x0);a.value(7,0xffffffff);a.value(9,0x0);
    a.call(0x1037b6f0);a.floating_load(0,30,0x48,false);a.floating_load(2,30,0x160,false);a.floating_compare(0,31);
    if(a.nonpositive()) {
        a.at(0x10368f64);a.floating_move(0,31);
    } else {
        a.at(0x10368f60);a.jump(0x10368f68);
    }
    a.at(0x10368f68);a.floating_difference(24,2,0,true,false);a.floating_load(0,30,0x168,false);a.floating_compare(24,0);
    if(!a.at_least()) {
        a.at(0x10368f78);a.floating_move(24,0);
    }
    a.at(0x10368f7c);a.floating_load(0,30,0x3a8,false);a.floating_compare(31,0);
    if(!a.at_least()) {
        a.at(0x10368f88);a.floating_load(0,30,0x3b0,false);a.floating_sum(1,1,0);
    }
    a.at(0x10368f90);a.floating_product(25,24,1);a.move(3,13);a.move(4,17);
    a.call(0x100f4790);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xb0,3);
    a.floating_load(1,30,0x8,false);a.store<4>(1,0xac,0);a.floating_load(0,1,0xac,false);a.floating_subtract(1,0,1,false);
    a.floating_compare(25,1);
    if(!a.nonpositive()) {
        a.at(0x10368fc4);a.floating_load(0,30,0x158,false);a.floating_compare(25,0);
        if(!a.nonpositive()) {
            a.at(0x10368fd0);a.floating_difference(0,1,25,true,false);a.floating_product(31,31,0);a.floating_move(25,1);
        }
    }
    a.at(0x10368fdc);a.compare(31,0x0,false);
    if(!a.equal()) {
        a.at(0x10368fe4);a.load<4>(3,31,0xc,false);a.value(0,0x43300000);a.store<4>(1,0xac,0);
        a.exclusive_flags(0,3,0x80000000);a.floating_load(2,30,0x8,false);a.store<4>(1,0xb0,0);a.floating_load(0,30,0x150,false);
        a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,false);a.floating_compare(1,0);
        if(!a.at_least()) {
            a.at(0x10369010);a.floating_move(1,0);
        }
        a.at(0x10369014);a.floating_difference(0,25,1,true,false);a.floating_load(1,30,0x48,false);a.floating_compare(1,0);
        if(a.nonpositive()) {
            a.at(0x10369028);a.floating_load(1,30,0x150,false);a.floating_compare(1,0);
            if(a.at_least()) {
                a.at(0x10369038);a.floating_move(1,0);
            } else {
                a.at(0x10369034);a.jump(0x1036903c);
            }
        } else {
            a.at(0x10369024);a.jump(0x1036903c);
        }
        a.at(0x1036903c);a.floating_load(0,30,0x80,false);a.floating_compare(1,0);a.compare_greater_or_equal_bit();
        if(a.equal()) {
            a.at(0x1036904c);a.floating_difference(0,1,24,true,false);a.floating_product(31,31,0);
        }
    }
    a.at(0x10369054);a.floating_move(1,31);
    return;
}

void apply_primary_category_fallback(Call& a) {
    a.at(0x10367e94);a.floating_load(0,30,0x148,false);a.floating_product(31,31,0);a.jump(0x10367f00);
    return adjust_context_history_and_quote(a);
}

void select_secondary_category_scale(Call& a) {
    a.at(0x10367ea0);a.compare(14,0xf,false);
    if(a.positive()) {
        return apply_secondary_category_fallback(a);
    } else {
        a.at(0x10367ea8);a.load<4>(3,2,-0x4e04,false);a.mask(0,14,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
        a.count_destination(3);a.count_branch();
        switch(a.r(0)>>2) {
            case 0:case 1:case 11: {
                return apply_secondary_category_fallback(a);
                break;
            }
            case 2:case 3:case 4: {
                a.at(0x10367ec8);a.floating_load(0,30,0x18,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                break;
            }
            case 5:case 12: {
                a.at(0x10367ebc);a.floating_load(0,30,0x1f8,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                break;
            }
            case 6:case 13: {
                a.at(0x10367ed4);a.floating_load(0,30,0x368,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                break;
            }
            case 7:case 8:case 14:case 15: {
                a.at(0x10367ee0);a.floating_load(0,30,0x58,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                break;
            }
            case 9:case 10: {
                a.at(0x10367eec);a.floating_load(0,30,0x58,false);a.floating_product(31,31,0);a.jump(0x10367f00);
                break;
            }
        }
    }
    return adjust_context_history_and_quote(a);
}

void apply_secondary_category_fallback(Call& a) {
    a.at(0x10367ef8);a.floating_load(0,30,0x370,false);a.floating_product(31,31,0);
    return adjust_context_history_and_quote(a);
}

void adjust_context_history_and_quote(Call& a) {
    a.at(0x10367f00);a.compare(20,0x0,false);
    if(!a.equal()) {
        a.at(0x10367f14);a.load<1>(6,15,0x65,false);a.load<2>(5,15,0x4e,false);a.load<2>(0,13,0x2,false);
        a.sign8(6,6,false);a.multiply(3,6,0xf);a.load<2>(4,15,0x4c,true);a.subtract(5,5,0);
        a.load<2>(0,13,0x0,true);a.multiply(7,5,0x16d);a.subtract(4,4,0);a.offset(0,3,0x78);
        a.add(7,7,4);a.compare_registers(7,0,true);
        if(!a.at_least()) {
            a.at(0x10367f4c);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
            return;
        }
        a.at(0x10367f54);a.product(7,7,7);a.value(3,0x66660000);a.offset(5,6,0x6);
        a.offset(3,3,0x6667);a.compare(5,0x1,true);a.value(0,0x1);a.multiply_high(3,3,7);
        a.shift_right(3,3,3);a.mask(4,3,0x1,false,1);a.add(7,3,4);
        if(!a.less()) {
            a.at(0x10367f80);a.move(0,5);
        }
        a.at(0x10367f84);a.divide_record_at(0x10367f84,7,7,0);
        if(a.at_least()) {
            a.at(0x10367f94);a.compare(7,0x190,true);a.value(0,0x190);
            if(!a.positive()) {
                a.at(0x10367fa0);a.move(0,7);
            }
        } else {
            a.at(0x10367f8c);a.value(0,0x0);a.jump(0x10367fa4);
        }
        a.at(0x10367fa4);a.compare(0,0x0,true);
        if(a.equal()) {
            a.at(0x10367fac);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
            return;
        }
        a.at(0x10367fb4);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xb0,3);
        a.floating_load(2,30,0x8,false);a.store<4>(1,0xac,0);a.floating_load(0,30,0x308,false);a.floating_load(1,1,0xac,false);
        a.floating_subtract(1,1,2,false);a.floating_difference(29,1,0,true,false);
    } else {
        a.at(0x10367f08);a.floating_load(0,30,0xe0,false);a.floating_product(31,31,0);a.jump(0x10367fd8);
    }
    a.at(0x10367fd8);a.compare(20,0x0,false);
    if(!a.equal()) {
        a.at(0x10368070);a.load<4>(3,20,0x2c,false);a.compare(3,0x0,false);
        if(a.equal()) {
            a.at(0x1036809c);a.value(23,0x3);
        } else {
            a.at(0x1036807c);a.load<1>(0,3,0x11c,false);a.sign8(23,0,false);a.compare(23,0x1,true);
            if(a.less()) {
                a.at(0x10368094);a.value(23,0x3);a.jump(0x103680a0);
            } else {
                a.at(0x1036808c);a.compare(23,0x3,true);
                if(!a.nonpositive()) {
                    a.at(0x10368094);a.value(23,0x3);a.jump(0x103680a0);
                }
            }
        }
        a.at(0x103680a0);a.product(0,23,23);a.load<4>(3,20,0x3c,false);a.compare(3,0x0,false);
        a.sign8(23,0,false);
        if(a.equal()) {
            a.at(0x10368170);a.load<2>(3,20,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0xac,0);
            a.exclusive_flags(0,3,0x80000000);a.floating_load(2,30,0x8,false);a.store<4>(1,0xb0,0);a.floating_load(0,30,0x160,false);
            a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);
            a.floating_store(0,1,0xb4);a.load<4>(14,1,0xb8,false);
        } else {
            a.at(0x103680b4);a.load<4>(3,3,0x10,false);a.compare(3,0x0,false);
            if(a.equal()) {
                a.at(0x10368170);a.load<2>(3,20,0x1ec,true);a.value(0,0x43300000);a.store<4>(1,0xac,0);
                a.exclusive_flags(0,3,0x80000000);a.floating_load(2,30,0x8,false);a.store<4>(1,0xb0,0);a.floating_load(0,30,0x160,false);
                a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);
                a.floating_store(0,1,0xb4);a.load<4>(14,1,0xb8,false);
            } else {
                a.at(0x103680c0);a.load<1>(3,3,0x37,false);a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x103680e4);a.load<2>(6,20,0x1ec,true);a.value(4,0x43300000);a.sign8(0,3,false);
                    a.value(3,0x88890000);a.exclusive_flags(5,6,0x80000000);a.store<4>(1,0xac,4);a.floating_load(2,30,0x8,false);
                    a.product(0,0,6);a.store<4>(1,0xb0,5);a.offset(3,3,-0x7777);a.floating_load(0,30,0x160,false);
                    a.floating_load(1,1,0xac,false);a.store<4>(1,0xb4,4);a.floating_subtract(1,1,2,false);a.multiply_high(3,3,0);
                    a.floating_difference(1,1,0,true,false);a.add(0,3,0);a.shift_right(0,0,3);a.mask(3,0,0x1,false,1);
                    a.add(0,0,3);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xb8,0);a.floating_load(0,1,0xb4,false);
                    a.floating_subtract(0,0,2,false);a.floating_compare(1,0);
                    if(a.nonpositive()) {
                        a.at(0x10368150);a.store<4>(1,0xc0,0);a.store<4>(1,0xbc,4);a.floating_load(0,1,0xbc,false);
                        a.floating_subtract(1,0,2,false);
                    } else {
                        a.at(0x1036814c);a.jump(0x10368160);
                    }
                    a.at(0x10368160);a.floating_integer(0,1);a.floating_store(0,1,0xc4);a.load<4>(14,1,0xc8,false);
                    a.jump(0x103681a4);
                } else {
                    a.at(0x103680cc);a.load<2>(3,20,0x1ec,true);a.mask(0,3,0x1,false,1);a.add(0,0,3);
                    a.shift_right(0,0,1);a.sign16(14,0,false);a.jump(0x103681a4);
                }
            }
        }
        a.at(0x103681a4);a.load<4>(5,20,0x30,false);a.compare(5,0x0,false);
        if(a.equal()) {
            a.at(0x10368220);a.load<2>(28,20,0x1ec,true);
        } else {
            a.at(0x103681b0);a.load<2>(6,20,0x1ec,true);a.compare(6,0x1d4c,true);
            if(a.less()) {
                a.at(0x103681ec);a.load<2>(4,5,0x18,true);a.mask(0,6,0x1,false,1);a.add(3,0,6);
                a.multiply(0,4,0xfa);a.shift_right(3,3,1);a.multiply(4,4,0x1f4);a.add(0,3,0);
                a.compare_registers(4,0,true);
                if(a.nonpositive()) {
                    a.at(0x10368214);a.move(4,0);
                } else {
                    a.at(0x10368210);a.jump(0x10368218);
                }
                a.at(0x10368218);a.sign16(28,4,false);a.jump(0x10368224);
            } else {
                a.at(0x103681bc);a.value(3,0x10000);a.load<4>(4,20,0x8,false);a.offset(0,3,-0x7748);
                a.compare_registers(4,0,true);
                if(a.nonpositive()) {
                    a.at(0x103681ec);a.load<2>(4,5,0x18,true);a.mask(0,6,0x1,false,1);a.add(3,0,6);
                    a.multiply(0,4,0xfa);a.shift_right(3,3,1);a.multiply(4,4,0x1f4);a.add(0,3,0);
                    a.compare_registers(4,0,true);
                    if(a.nonpositive()) {
                        a.at(0x10368214);a.move(4,0);
                    } else {
                        a.at(0x10368210);a.jump(0x10368218);
                    }
                    a.at(0x10368218);a.sign16(28,4,false);a.jump(0x10368224);
                } else {
                    a.at(0x103681d0);a.load<2>(0,5,0x18,true);a.multiply(0,0,0x1f4);a.compare_registers(0,6,true);
                    if(!a.nonpositive()) {
                        a.at(0x103681e0);a.move(6,0);
                    }
                    a.at(0x103681e4);a.sign16(28,6,false);a.jump(0x10368224);
                }
            }
        }
        a.at(0x10368224);a.load<4>(4,20,0x14,false);a.compare(4,-0x1,true);
        if(a.nonpositive()) {
            a.at(0x10368260);a.load<4>(3,2,-0x7984,false);a.load<4>(0,20,0x0,false);a.load<4>(3,3,0x0,false);
            a.mask(0,0,0xfffffff8,false,3);a.indexed_load<4>(0,3,0);a.compare(0,-0x1,true);
            if(!a.nonpositive()) {
                a.at(0x1036827c);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,0,0x6c);a.load<4>(3,3,0x0,false);
                a.add(21,3,0);
            }
        } else {
            a.at(0x10368230);a.load<4>(3,2,-0x7a2c,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
            if(!a.equal()) {
                a.at(0x1036824c);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,4,0x6c);a.load<4>(3,3,0x0,false);
                a.add(21,3,0);a.jump(0x1036828c);
            } else {
                a.at(0x10368240);a.load<2>(0,20,0x1ec,true);a.compare(0,0x1388,true);
                if(a.at_least()) {
                    a.at(0x10368260);a.load<4>(3,2,-0x7984,false);a.load<4>(0,20,0x0,false);a.load<4>(3,3,0x0,false);
                    a.mask(0,0,0xfffffff8,false,3);a.indexed_load<4>(0,3,0);a.compare(0,-0x1,true);
                    if(!a.nonpositive()) {
                        a.at(0x1036827c);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,0,0x6c);a.load<4>(3,3,0x0,false);
                        a.add(21,3,0);
                    }
                } else {
                    a.at(0x1036824c);a.load<4>(3,2,-0x7bbc,false);a.multiply(0,4,0x6c);a.load<4>(3,3,0x0,false);
                    a.add(21,3,0);a.jump(0x1036828c);
                }
            }
        }
        a.at(0x1036828c);a.compare(21,0x0,false);
        if(a.equal()) {
            a.at(0x103682a4);a.sign16(3,28,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);a.store<4>(1,0xb0,3);a.floating_load(3,30,0xf8,false);
            a.floating_load(1,1,0xac,false);a.floating_load(0,30,0x320,false);a.floating_subtract(1,1,2,false);a.floating_product(1,3,1);
            a.floating_compare(1,0);
            if(a.at_least()) {
                a.at(0x103682dc);a.floating_move(1,0);
            } else {
                a.at(0x103682d8);a.jump(0x103682e0);
            }
            a.at(0x103682e0);a.floating_integer(0,1);a.floating_store(0,1,0xb4);a.load<4>(13,1,0xb8,false);
        } else {
            a.at(0x10368294);a.load<2>(0,21,0x18,true);a.multiply(0,0,0x258);a.sign16(13,0,false);
            a.jump(0x103682ec);
        }
        a.at(0x103682ec);a.load<4>(3,20,0x90,false);a.compare(3,0x0,false);
        if(!a.equal()) {
            a.at(0x10368330);a.load<4>(4,3,0x1c,false);a.compare(4,0x0,false);
            if(!a.equal()) {
                a.at(0x10368374);a.load<4>(3,2,-0x79f0,false);a.load<2>(25,4,0x2a,true);a.load<4>(3,3,0x0,false);
                a.compare(3,0x0,false);
                if(!a.equal()) {
                    a.at(0x10368388);a.sign8(0,17,false);a.compare(0,0x5,true);
                    if(!a.equal()) {
                        a.at(0x10368394);a.compare(20,0x0,false);
                        if(a.equal()) {
                            a.at(0x103683b4);a.value(0,0x0);
                        } else {
                            a.at(0x1036839c);a.load<4>(0,20,0x0,false);a.load<4>(4,3,0x0,false);a.multiply(0,0,0x4c);
                            a.add(4,4,0);a.load<2>(0,4,0x6,true);a.jump(0x103683b8);
                        }
                        a.at(0x103683b8);a.sign16(4,0,false);a.value(0,0x43300000);a.exclusive_flags(4,4,0x80000000);
                        a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);a.compare(20,0x0,false);a.store<4>(1,0xb0,4);
                        a.floating_load(3,30,0x138,false);a.floating_load(0,1,0xac,false);a.floating_load(1,30,0x2f0,false);a.floating_subtract(2,0,2,false);
                        if(a.equal()) {
                            a.at(0x10368400);a.value(0,0x0);
                        } else {
                            a.at(0x103683e8);a.load<4>(0,20,0x0,false);a.load<4>(3,3,0x0,false);a.multiply(0,0,0x4c);
                            a.add(3,3,0);a.load<2>(0,3,0x8,true);a.jump(0x10368404);
                        }
                        a.at(0x10368404);a.sign16(0,0,false);a.value(4,0x43300000);a.exclusive_flags(0,0,0x80000000);
                        a.store<4>(1,0xb4,4);a.floating_load(5,30,0x8,false);a.store<4>(1,0xb8,0);a.floating_load(0,1,0xb4,false);
                        a.floating_subtract(0,0,5,false);a.floating_product(0,1,0);a.floating_product_sum(0,3,2,0,false);a.floating_integer(0,0);
                        a.floating_store(0,1,0xbc);a.load<4>(0,1,0xc0,false);a.sign16(0,0,false);a.compare(0,0x1388,true);
                        if(a.at_least()) {
                            a.at(0x103684b4);a.exclusive_flags(3,0,0x80000000);a.exclusive_flags(0,25,0x80000000);a.store<4>(1,0xb0,3);
                            a.floating_load(4,30,0x338,false);a.store<4>(1,0xac,4);a.floating_load(2,30,0x68,false);a.floating_load(0,1,0xac,false);
                            a.store<4>(1,0xb8,0);a.floating_subtract(3,0,5,false);a.floating_load(1,30,0x330,false);a.store<4>(1,0xb4,4);
                            a.floating_load(0,1,0xb4,false);a.floating_product_sum(3,4,3,2,false);a.store<4>(1,0xc0,0);a.floating_subtract(2,0,5,false);
                            a.store<4>(1,0xbc,4);a.floating_load(0,1,0xbc,false);a.floating_product(2,2,3);a.floating_subtract(0,0,5,false);
                            a.floating_product_sum(0,1,3,0,false);a.floating_compare(2,0);
                            if(a.at_least()) {
                                a.at(0x10368510);a.floating_move(2,0);
                            } else {
                                a.at(0x1036850c);a.jump(0x10368514);
                            }
                            a.at(0x10368514);a.floating_integer(0,2);a.floating_store(0,1,0xc4);a.load<4>(25,1,0xc8,false);
                        } else {
                            a.at(0x10368444);a.exclusive_flags(3,0,0x80000000);a.exclusive_flags(0,25,0x80000000);a.store<4>(1,0xb0,3);
                            a.floating_load(4,30,0x328,false);a.store<4>(1,0xac,4);a.floating_load(2,30,0x208,false);a.floating_load(0,1,0xac,false);
                            a.store<4>(1,0xb8,0);a.floating_subtract(3,0,5,false);a.floating_load(1,30,0x330,false);a.store<4>(1,0xb4,4);
                            a.floating_load(0,1,0xb4,false);a.floating_negative_product_difference(3,4,3,2);a.store<4>(1,0xc0,0);a.floating_subtract(0,0,5,false);
                            a.store<4>(1,0xbc,4);a.floating_difference(2,0,3,true,false);a.floating_load(0,1,0xbc,false);a.floating_subtract(0,0,5,false);
                            a.floating_negative_product_difference(0,1,3,0);a.floating_compare(2,0);
                            if(a.nonpositive()) {
                                a.at(0x103684a0);a.floating_move(2,0);
                            } else {
                                a.at(0x1036849c);a.jump(0x103684a4);
                            }
                            a.at(0x103684a4);a.floating_integer(0,2);a.floating_store(0,1,0xc4);a.load<4>(25,1,0xc8,false);
                            a.jump(0x10368520);
                        }
                    }
                }
            } else {
                a.at(0x1036833c);a.sign16(3,28,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xac,0);a.floating_load(1,30,0x8,false);a.store<4>(1,0xb0,3);a.floating_load(2,30,0xa0,false);
                a.floating_load(0,1,0xac,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
                a.floating_store(0,1,0xb4);a.load<4>(25,1,0xb8,false);a.jump(0x10368520);
            }
        } else {
            a.at(0x103682f8);a.sign16(3,28,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xac,0);a.floating_load(1,30,0x8,false);a.store<4>(1,0xb0,3);a.floating_load(2,30,0xa0,false);
            a.floating_load(0,1,0xac,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
            a.floating_store(0,1,0xb4);a.load<4>(25,1,0xb8,false);a.jump(0x10368520);
        }
        a.at(0x10368520);a.load<2>(27,20,0x1ec,true);a.move(3,20);a.call(0x100c7d00);
        a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x10368534);a.load<4>(3,2,-0x7ab8,false);a.move(4,20);a.load<4>(3,3,0x0,false);
            a.call(0x10103510);a.compare(3,0x0,false);
            if(!a.equal()) {
                a.at(0x1036854c);a.call(0x100f55b0);a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x10368558);
                    if(a.at_least()) {
                        a.at(0x1036856c);a.compare(0,0x2,true);
                        if(a.equal()) {
                            a.at(0x1036857c);a.exclusive_flags(3,27,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xb0,3);
                            a.floating_load(1,30,0x8,false);a.store<4>(1,0xac,0);a.floating_load(2,30,0x178,false);a.floating_load(0,1,0xac,false);
                            a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0xb4);
                            a.load<4>(27,1,0xb8,false);a.sign16(0,27,false);a.compare(0,0x1676,true);
                            if(!a.at_least()) {
                                a.at(0x103685b8);a.value(27,0x1676);a.jump(0x10368668);
                            }
                        } else {
                            a.at(0x10368574);
                            if(!a.at_least()) {
                                a.at(0x10368578);a.jump(0x103685c0);a.at(0x103685c0);a.exclusive_flags(3,27,0x80000000);
                                a.value(0,0x43300000);a.store<4>(1,0xb0,3);a.floating_load(1,30,0x8,false);a.store<4>(1,0xac,0);
                                a.floating_load(2,30,0xc8,false);a.floating_load(0,1,0xac,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);
                                a.floating_integer(0,0);a.floating_store(0,1,0xb4);a.load<4>(27,1,0xb8,false);a.sign16(0,27,false);
                                a.compare(0,0x128e,true);
                                if(!a.at_least()) {
                                    a.at(0x103685fc);a.value(27,0x128e);a.jump(0x10368668);
                                }
                            }
                        }
                    } else {
                        a.at(0x1036855c);a.compare(0,-0x2,true);
                        if(a.equal()) {
                            a.at(0x10368638);a.exclusive_flags(3,27,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xb0,3);
                            a.floating_load(1,30,0x8,false);a.store<4>(1,0xac,0);a.floating_load(2,30,0xa0,false);a.floating_load(0,1,0xac,false);
                            a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0xb4);
                            a.load<4>(27,1,0xb8,false);
                        } else {
                            a.at(0x10368564);
                            if(a.at_least()) {
                                a.at(0x10368604);a.exclusive_flags(3,27,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xb0,3);
                                a.floating_load(1,30,0x8,false);a.store<4>(1,0xac,0);a.floating_load(2,30,0x168,false);a.floating_load(0,1,0xac,false);
                                a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0xb4);
                                a.load<4>(27,1,0xb8,false);a.jump(0x10368668);
                            } else {
                                a.at(0x10368568);a.jump(0x10368668);
                            }
                        }
                    }
                }
            }
        }
        a.at(0x10368668);a.move(3,20);a.call(0x100c7d00);a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x103686c0);a.load<4>(0,20,0x0,false);a.value(4,0x43300000);a.load<4>(3,1,0x188,false);
            a.sign16(5,27,false);a.multiply(0,0,0xc);a.store<4>(1,0xb4,4);a.load<4>(3,3,0xdc,false);
            a.exclusive_flags(5,5,0x80000000);a.floating_load(3,30,0x8,false);a.add(3,3,0);a.load<2>(0,3,0x6,true);
            a.store<4>(1,0xb0,5);a.exclusive_flags(0,0,0x80000000);a.floating_load(1,30,0x348,false);a.store<4>(1,0xb8,0);
            a.floating_load(4,30,0x180,false);a.floating_load(0,1,0xb4,false);a.store<4>(1,0xac,4);a.floating_subtract(0,0,3,false);
            a.floating_load(2,1,0xac,false);a.floating_product(0,1,0);a.floating_subtract(2,2,3,false);a.floating_product_sum(0,4,2,0,false);
            a.floating_integer(0,0);a.floating_store(0,1,0xbc);a.load<4>(7,1,0xc0,false);
        } else {
            a.at(0x10368678);a.move(3,20);a.call(0x100ce250);a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x103686c0);a.load<4>(0,20,0x0,false);a.value(4,0x43300000);a.load<4>(3,1,0x188,false);
                a.sign16(5,27,false);a.multiply(0,0,0xc);a.store<4>(1,0xb4,4);a.load<4>(3,3,0xdc,false);
                a.exclusive_flags(5,5,0x80000000);a.floating_load(3,30,0x8,false);a.add(3,3,0);a.load<2>(0,3,0x6,true);
                a.store<4>(1,0xb0,5);a.exclusive_flags(0,0,0x80000000);a.floating_load(1,30,0x348,false);a.store<4>(1,0xb8,0);
                a.floating_load(4,30,0x180,false);a.floating_load(0,1,0xb4,false);a.store<4>(1,0xac,4);a.floating_subtract(0,0,3,false);
                a.floating_load(2,1,0xac,false);a.floating_product(0,1,0);a.floating_subtract(2,2,3,false);a.floating_product_sum(0,4,2,0,false);
                a.floating_integer(0,0);a.floating_store(0,1,0xbc);a.load<4>(7,1,0xc0,false);
            } else {
                a.at(0x10368688);a.sign16(3,27,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xac,0);a.floating_load(1,30,0x8,false);a.store<4>(1,0xb0,3);a.floating_load(2,30,0x378,false);
                a.floating_load(0,1,0xac,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
                a.floating_store(0,1,0xb4);a.load<4>(7,1,0xb8,false);a.jump(0x10368728);
            }
        }
        a.at(0x10368728);a.sign16(0,14,false);a.value(6,0x43300000);a.exclusive_flags(0,0,0x80000000);
        a.sign16(5,13,false);a.store<4>(1,0xd0,0);a.sign16(3,25,false);a.exclusive_flags(0,3,0x80000000);
        a.exclusive_flags(5,5,0x80000000);a.store<4>(1,0xcc,6);a.sign16(3,28,false);a.floating_load(3,30,0x8,false);
        a.exclusive_flags(4,3,0x80000000);a.floating_load(0,1,0xcc,false);a.sign16(3,7,false);a.store<4>(1,0xc8,0);
        a.exclusive_flags(0,3,0x80000000);a.floating_subtract(0,0,3,true);a.load<4>(3,2,-0x4de8,false);a.store<4>(1,0xc4,6);
        a.floating_load(4,3,0xc,true);a.move(3,20);a.floating_load(1,1,0xc4,false);a.store<4>(1,0xc0,4);
        a.floating_product_single(0,27,0);a.floating_subtract(1,1,3,true);a.store<4>(1,0xbc,6);a.floating_load(2,1,0xbc,false);
        a.floating_product_sum(0,28,1,0,true);a.store<4>(1,0xb8,5);a.floating_subtract(1,2,3,true);a.store<4>(1,0xb4,6);
        a.floating_load(2,1,0xb4,false);a.floating_product_sum(0,26,1,0,true);a.store<4>(1,0xb0,0);a.floating_subtract(1,2,3,true);
        a.store<4>(1,0xac,6);a.floating_load(2,1,0xac,false);a.floating_product_sum(0,30,1,0,true);a.floating_subtract(1,2,3,true);
        a.floating_product_sum(0,4,1,0,true);a.floating_difference(0,0,25,true,true);a.floating_integer(0,0);a.floating_store(0,1,0xd4);
        a.load<4>(13,1,0xd8,false);a.call(0x100c7d00);a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x103687e4);a.load<1>(6,20,0x23d,false);a.compare(6,0x1,true);
            if(!a.equal()) {
                a.at(0x103687f0);a.load<1>(0,15,0x5e,false);a.sign16(5,13,false);a.mask(4,5,0x1,false,1);
                a.product(3,0,0);a.offset(0,5,-0x7d0);a.add(4,4,5);a.shift_right(4,4,1);
                a.multiply(3,3,0x5);a.subtract(0,3,0);a.compare_registers(4,0,true);
                if(a.at_least()) {
                    a.at(0x10368820);a.move(4,0);
                } else {
                    a.at(0x1036881c);a.jump(0x10368824);
                }
                a.at(0x10368824);a.sign8(0,6,false);a.sign16(13,4,false);a.compare(0,0x3,true);
                if(a.equal()) {
                    a.at(0x10368834);a.mask(0,13,0x1,false,1);a.offset(3,13,-0x7d0);a.add(0,0,13);
                    a.shift_right(0,0,1);a.compare_registers(0,3,true);
                    if(a.at_least()) {
                        a.at(0x10368850);a.move(0,3);
                    } else {
                        a.at(0x1036884c);a.jump(0x10368854);
                    }
                    a.at(0x10368854);a.sign16(13,0,false);
                }
            }
        }
        a.at(0x10368858);a.sign16(0,13,false);a.value(4,0x43300000);a.exclusive_flags(0,0,0x80000000);
        a.load<4>(5,20,0x2c,false);a.store<4>(1,0xb0,0);a.floating_load(2,30,0x8,false);a.compare(5,0x0,false);
        a.store<4>(1,0xac,4);a.floating_load(3,30,0x178,false);a.floating_load(1,1,0xac,false);a.floating_load(0,30,0x350,false);
        a.floating_subtract(1,1,2,false);a.floating_product_difference(0,3,1,0);a.floating_integer(0,0);a.floating_store(0,1,0xb4);
        a.load<4>(13,1,0xb8,false);
        if(!a.equal()) {
            a.at(0x1036889c);a.compare_registers(16,20,false);
            if(!a.equal()) {
                a.at(0x103688a4);a.sign16(3,13,false);a.load<4>(0,15,0x14,false);a.offset(3,3,0x1388);
                a.store<4>(1,0xac,4);a.exclusive_flags(3,3,0x80000000);a.floating_load(0,30,0x358,false);a.store<4>(1,0xb0,3);
                a.compare(0,0x0,false);a.floating_load(1,1,0xac,false);a.store<4>(1,0xb8,3);a.floating_subtract(1,1,2,false);
                a.store<4>(1,0xb4,4);a.floating_difference(1,1,0,true,false);a.floating_load(0,1,0xb4,false);a.floating_subtract(0,0,2,false);
                a.floating_product(24,1,0);
                if(a.equal()) {
                    a.at(0x10368928);a.floating_load(3,30,0x150,false);
                } else {
                    a.at(0x103688e8);a.load<4>(3,2,-0x7b3c,false);a.move(4,15);a.load<1>(6,5,0x119,false);
                    a.move(5,20);a.load<4>(3,3,0x0,false);a.value(7,0x1);a.call(0x103092f0);
                    a.sign8(3,3,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xbc,0);
                    a.floating_load(1,30,0x8,false);a.store<4>(1,0xc0,3);a.floating_load(0,1,0xbc,false);a.floating_subtract(3,0,1,false);
                    a.jump(0x1036892c);
                }
                a.at(0x1036892c);a.floating_load(1,30,0x40,false);a.floating_load(0,30,0x360,false);a.floating_load(2,30,0x150,false);
                a.floating_negative_product_difference(0,1,3,0);a.floating_difference(1,3,2,true,false);a.floating_difference(0,24,0,true,false);a.floating_compare(1,0);
                if(a.nonpositive()) {
                    a.at(0x10368950);a.floating_move(1,0);
                } else {
                    a.at(0x1036894c);a.jump(0x10368954);
                }
                a.at(0x10368954);a.floating_load(0,30,0x58,false);a.floating_sum(0,0,1);a.floating_difference(31,31,0,true,false);
            }
        }
    } else {
        a.at(0x10367fe0);a.load<4>(3,15,0xc,false);a.load<1>(0,3,0x11c,false);a.sign8(23,0,false);
        a.compare(23,0x1,true);
        if(a.less()) {
            a.at(0x10367ffc);a.value(23,0x3);
        } else {
            a.at(0x10367ff4);a.compare(23,0x3,true);
            if(!a.nonpositive()) {
                a.at(0x10367ffc);a.value(23,0x3);
            }
        }
        a.at(0x10368000);a.load<4>(5,15,0x1c,false);a.value(3,0x55550000);a.value(0,0x43300000);
        a.floating_load(2,30,0x8,false);a.load<2>(6,5,0x2c,true);a.product(8,23,23);a.load<2>(4,5,0x28,true);
        a.offset(7,3,0x5556);a.load<2>(5,5,0x2a,true);a.add(3,4,6);a.store<4>(1,0xac,0);
        a.add(0,5,3);a.floating_load(3,30,0x178,false);a.multiply_high(3,7,0);a.floating_load(0,30,0x350,false);
        a.sign8(23,8,false);a.mask(0,3,0x1,false,1);a.add(0,3,0);a.sign16(0,0,false);
        a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xb0,0);a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,false);
        a.floating_product_difference(0,3,1,0);a.floating_integer(0,0);a.floating_store(0,1,0xb4);a.load<4>(13,1,0xb8,false);
        a.jump(0x10368960);
    }
    a.at(0x10368960);a.compare_registers(16,20,false);
    if(!a.equal()) {
        a.at(0x10368968);a.load<4>(4,16,0x30,false);a.compare(4,0x0,false);
        if(a.equal()) {
            a.at(0x10368988);a.floating_load(0,30,0x48,false);a.floating_product(31,31,0);
        } else {
            a.at(0x10368974);a.load<4>(3,2,-0x7a0c,false);a.load<4>(4,4,0x0,false);a.load<4>(0,3,0x0,false);
            a.compare_registers(4,0,true);
            if(a.equal()) {
                a.at(0x10368988);a.floating_load(0,30,0x48,false);a.floating_product(31,31,0);
            }
        }
        a.at(0x10368990);a.load<4>(4,15,0x1c,false);a.value(3,0x55550000);a.value(6,0x43300000);
        a.load<2>(0,16,0x1ec,true);a.load<2>(8,4,0x2c,true);a.offset(9,3,0x5556);a.load<2>(3,4,0x28,true);
        a.sign16(5,26,false);a.load<2>(7,4,0x2a,true);a.sign16(4,13,false);a.add(3,3,8);
        a.store<4>(1,0xac,6);a.add(3,7,3);a.floating_load(5,30,0x8,false);a.multiply_high(7,9,3);
        a.floating_load(0,30,0x380,false);a.floating_load(2,30,0x80,false);a.compare_registers(5,4,true);a.mask(3,7,0x1,false,1);
        a.add(3,7,3);a.subtract(0,3,0);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xb0,0);
        a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,5,false);a.floating_difference(0,1,0,true,false);a.floating_sum(0,2,0);
        if(a.nonpositive()) {
            a.at(0x10368a50);
            if(a.positive()) {
                a.at(0x10368a90);a.sign16(3,26,false);a.sign16(0,13,false);a.compare_registers(3,0,true);
                if(a.positive()) {
                    a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                    a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                    a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                } else {
                    a.at(0x10368aa0);a.floating_load(1,30,0x150,false);a.floating_compare(31,1);
                    if(a.nonpositive()) {
                        a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                        a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                        a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                    } else {
                        a.at(0x10368aac);a.floating_load(1,30,0x30,false);a.floating_load(3,30,0x200,false);a.floating_difference(2,31,1,true,false);
                        a.floating_load(1,30,0x48,false);a.floating_subtract(2,3,2,false);a.floating_round_single(2,2);a.floating_compare(1,2);
                        if(a.nonpositive()) {
                            a.at(0x10368ad0);a.floating_load(1,30,0x80,false);a.floating_compare(1,2);
                            if(a.at_least()) {
                                a.at(0x10368ae0);a.floating_move(1,2);
                            } else {
                                a.at(0x10368adc);a.jump(0x10368ae4);
                            }
                        } else {
                            a.at(0x10368acc);a.jump(0x10368ae4);
                        }
                        a.at(0x10368ae4);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                        a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                        a.floating_round_single(3,1);a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,true);
                        a.floating_product_single(4,3,1);a.jump(0x10368b40);
                    }
                }
            } else {
                a.at(0x10368a54);a.sign8(0,17,false);a.compare(0,0x5,true);
                if(a.equal()) {
                    a.at(0x10368a90);a.sign16(3,26,false);a.sign16(0,13,false);a.compare_registers(3,0,true);
                    if(a.positive()) {
                        a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                        a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                        a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                    } else {
                        a.at(0x10368aa0);a.floating_load(1,30,0x150,false);a.floating_compare(31,1);
                        if(a.nonpositive()) {
                            a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                            a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                            a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                        } else {
                            a.at(0x10368aac);a.floating_load(1,30,0x30,false);a.floating_load(3,30,0x200,false);a.floating_difference(2,31,1,true,false);
                            a.floating_load(1,30,0x48,false);a.floating_subtract(2,3,2,false);a.floating_round_single(2,2);a.floating_compare(1,2);
                            if(a.nonpositive()) {
                                a.at(0x10368ad0);a.floating_load(1,30,0x80,false);a.floating_compare(1,2);
                                if(a.at_least()) {
                                    a.at(0x10368ae0);a.floating_move(1,2);
                                } else {
                                    a.at(0x10368adc);a.jump(0x10368ae4);
                                }
                            } else {
                                a.at(0x10368acc);a.jump(0x10368ae4);
                            }
                            a.at(0x10368ae4);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                            a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                            a.floating_round_single(3,1);a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,true);
                            a.floating_product_single(4,3,1);a.jump(0x10368b40);
                        }
                    }
                } else {
                    a.at(0x10368a60);a.compare(0,0xc,true);
                    if(a.equal()) {
                        a.at(0x10368a90);a.sign16(3,26,false);a.sign16(0,13,false);a.compare_registers(3,0,true);
                        if(a.positive()) {
                            a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                            a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                            a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                        } else {
                            a.at(0x10368aa0);a.floating_load(1,30,0x150,false);a.floating_compare(31,1);
                            if(a.nonpositive()) {
                                a.at(0x10368b18);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                                a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                                a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(4,1,2,true);
                            } else {
                                a.at(0x10368aac);a.floating_load(1,30,0x30,false);a.floating_load(3,30,0x200,false);a.floating_difference(2,31,1,true,false);
                                a.floating_load(1,30,0x48,false);a.floating_subtract(2,3,2,false);a.floating_round_single(2,2);a.floating_compare(1,2);
                                if(a.nonpositive()) {
                                    a.at(0x10368ad0);a.floating_load(1,30,0x80,false);a.floating_compare(1,2);
                                    if(a.at_least()) {
                                        a.at(0x10368ae0);a.floating_move(1,2);
                                    } else {
                                        a.at(0x10368adc);a.jump(0x10368ae4);
                                    }
                                } else {
                                    a.at(0x10368acc);a.jump(0x10368ae4);
                                }
                                a.at(0x10368ae4);a.sign16(3,13,false);a.sign16(0,26,false);a.subtract(3,3,0);
                                a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xac,0);a.floating_load(2,30,0x8,false);
                                a.floating_round_single(3,1);a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,true);
                                a.floating_product_single(4,3,1);a.jump(0x10368b40);
                            }
                        }
                    } else {
                        a.at(0x10368a68);a.subtract(0,4,5);a.store<4>(1,0xac,6);a.exclusive_flags(0,0,0x80000000);
                        a.floating_load(2,30,0x178,false);a.store<4>(1,0xb0,0);a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,5,false);
                        a.floating_product(4,2,1);a.floating_round_single(4,4);a.jump(0x10368b40);
                    }
                }
            }
        } else {
            a.at(0x10368a00);a.load<1>(3,15,0x63,false);a.subtract(0,4,5);a.exclusive_flags(0,0,0x80000000);
            a.store<4>(1,0xac,6);a.sign8(3,3,false);a.floating_load(2,30,0x388,false);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xb8,0);a.floating_load(4,30,0xa0,false);a.store<4>(1,0xb0,3);a.floating_load(1,1,0xac,false);
            a.store<4>(1,0xb4,6);a.floating_subtract(3,1,5,false);a.floating_load(1,1,0xb4,false);a.floating_difference(2,3,2,true,false);
            a.floating_sum(2,4,2);a.floating_subtract(1,1,5,false);a.floating_product(4,2,1);a.floating_round_single(4,4);
            a.jump(0x10368b40);
        }
        a.at(0x10368b40);a.floating_load(1,30,0x158,false);a.floating_compare(4,1);
        if(!a.nonpositive()) {
            a.at(0x10368b4c);a.floating_load(1,30,0x80,false);a.floating_compare(29,1);
            if(!a.at_least()) {
                a.at(0x10368b58);a.floating_load(1,30,0xb8,false);a.floating_load(3,30,0x30,false);a.floating_difference(1,4,1,true,false);
                a.floating_compare(3,1);
                if(a.at_least()) {
                    a.at(0x10368b70);a.floating_move(3,1);
                } else {
                    a.at(0x10368b6c);a.jump(0x10368b74);
                }
                a.at(0x10368b74);a.floating_load(2,30,0x390,false);a.compare(18,0x0,false);a.floating_subtract(1,2,3,false);
                a.floating_product_sum(1,1,29,3,false);a.floating_difference(2,1,2,true,false);a.floating_product(31,31,2);
                if(!a.equal()) {
                    a.at(0x10368b90);a.floating_load(1,30,0x180,false);a.floating_compare(2,1);
                    if(!a.at_least()) {
                        a.at(0x10368b9c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x100000);a.store<4>(18,0x0,0);
                    }
                }
            }
        }
        a.at(0x10368ba8);a.sign8(0,19,false);a.compare(0,0x5,true);
        if(!a.equal()) {
            a.at(0x10368bb4);a.compare(0,0xc,true);
            if(!a.equal()) {
                a.at(0x10368bbc);a.sign8(0,17,false);a.compare(0,0x5,true);
                if(a.equal()) {
                    a.at(0x10368bd0);a.floating_load(1,30,0x158,false);a.floating_compare(4,1);
                    if(!a.at_least()) {
                        a.at(0x10368bdc);a.sign8(0,19,false);a.compare(0,0x8,true);
                        if(a.equal()) {
                            a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                            a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                            if(!a.at_least()) {
                                a.at(0x10368c14);a.floating_move(2,1);
                            }
                            a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                        } else {
                            a.at(0x10368be8);a.compare(0,0xf,true);
                            if(a.equal()) {
                                a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                if(!a.at_least()) {
                                    a.at(0x10368c14);a.floating_move(2,1);
                                }
                                a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                            } else {
                                a.at(0x10368bf0);a.compare(0,0x6,true);
                                if(!a.equal()) {
                                    a.at(0x10368c20);a.floating_load(1,30,0x150,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                    a.floating_load(1,30,0x58,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                    if(!a.at_least()) {
                                        a.at(0x10368c3c);a.floating_move(2,1);
                                    }
                                    a.at(0x10368c40);a.floating_product(4,4,2);
                                } else {
                                    a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                    a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                    if(!a.at_least()) {
                                        a.at(0x10368c14);a.floating_move(2,1);
                                    }
                                    a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                                }
                            }
                        }
                    }
                } else {
                    a.at(0x10368bc8);a.compare(0,0xc,true);
                    if(a.equal()) {
                        a.at(0x10368bd0);a.floating_load(1,30,0x158,false);a.floating_compare(4,1);
                        if(!a.at_least()) {
                            a.at(0x10368bdc);a.sign8(0,19,false);a.compare(0,0x8,true);
                            if(a.equal()) {
                                a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                if(!a.at_least()) {
                                    a.at(0x10368c14);a.floating_move(2,1);
                                }
                                a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                            } else {
                                a.at(0x10368be8);a.compare(0,0xf,true);
                                if(a.equal()) {
                                    a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                    a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                    if(!a.at_least()) {
                                        a.at(0x10368c14);a.floating_move(2,1);
                                    }
                                    a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                                } else {
                                    a.at(0x10368bf0);a.compare(0,0x6,true);
                                    if(!a.equal()) {
                                        a.at(0x10368c20);a.floating_load(1,30,0x150,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                        a.floating_load(1,30,0x58,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                        if(!a.at_least()) {
                                            a.at(0x10368c3c);a.floating_move(2,1);
                                        }
                                        a.at(0x10368c40);a.floating_product(4,4,2);
                                    } else {
                                        a.at(0x10368bf8);a.floating_load(1,30,0x398,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,31,1,true,false);
                                        a.floating_load(1,30,0x348,false);a.floating_subtract(2,3,2,false);a.floating_compare(2,1);
                                        if(!a.at_least()) {
                                            a.at(0x10368c14);a.floating_move(2,1);
                                        }
                                        a.at(0x10368c18);a.floating_product(4,4,2);a.jump(0x10368c44);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        a.at(0x10368c44);a.floating_load(1,30,0x3a0,false);a.floating_load(3,30,0x80,false);a.floating_difference(2,4,1,true,false);
        a.floating_load(1,30,0x370,false);a.floating_sum(2,3,2);a.floating_compare(2,1);
        if(!a.at_least()) {
            a.at(0x10368c60);a.floating_move(2,1);
        }
        a.at(0x10368c64);a.floating_product(31,31,2);a.compare(18,0x0,false);
        if(!a.equal()) {
            a.at(0x10368c70);a.floating_load(1,30,0x180,false);a.floating_compare(2,1);
            if(a.at_least()) {
                a.at(0x10368cec);a.floating_load(1,30,0x178,false);a.floating_compare(2,1);
                if(!a.nonpositive()) {
                    a.at(0x10368cf8);a.floating_load(1,30,0x170,false);a.floating_compare(30,1);
                    if(!a.nonpositive()) {
                        a.at(0x10368d04);a.compare(21,0x0,false);
                        if(a.equal()) {
                            a.at(0x10368d0c);a.compare(22,0x0,false);
                            if(!a.equal()) {
                                a.at(0x10368d14);a.sign8(0,17,false);a.compare(0,0x5,true);
                                if(a.equal()) {
                                    a.at(0x10368d28);a.compare(31,0x0,false);
                                    if(a.equal()) {
                                        a.at(0x10368d3c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x200000);a.store<4>(18,0x0,0);
                                    } else {
                                        a.at(0x10368d30);a.load<1>(0,31,0x54,false);a.sign8(0,0,true);
                                        if(a.equal()) {
                                            a.at(0x10368d3c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x200000);a.store<4>(18,0x0,0);
                                        }
                                    }
                                } else {
                                    a.at(0x10368d20);a.compare(0,0xc,true);
                                    if(a.equal()) {
                                        a.at(0x10368d28);a.compare(31,0x0,false);
                                        if(a.equal()) {
                                            a.at(0x10368d3c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x200000);a.store<4>(18,0x0,0);
                                        } else {
                                            a.at(0x10368d30);a.load<1>(0,31,0x54,false);a.sign8(0,0,true);
                                            if(a.equal()) {
                                                a.at(0x10368d3c);a.load<4>(0,18,0x0,false);a.flags(0,0,0x200000);a.store<4>(18,0x0,0);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                a.at(0x10368c7c);a.floating_load(1,30,0x170,false);a.floating_compare(30,1);
                if(a.nonpositive()) {
                    a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                    a.jump(0x10368d48);
                } else {
                    a.at(0x10368c88);a.compare(21,0x0,false);
                    if(a.equal()) {
                        a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                        a.jump(0x10368d48);
                    } else {
                        a.at(0x10368c90);a.compare(22,0x0,false);
                        if(!a.equal()) {
                            a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                            a.jump(0x10368d48);
                        } else {
                            a.at(0x10368c98);a.floating_load(1,30,0xc8,false);a.floating_compare(31,1);
                            if(a.nonpositive()) {
                                a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                                a.jump(0x10368d48);
                            } else {
                                a.at(0x10368ca4);a.sign8(0,19,false);a.compare(0,0x5,true);
                                if(a.equal()) {
                                    a.at(0x10368cb8);a.compare(31,0x0,false);
                                    if(a.equal()) {
                                        a.at(0x10368ccc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x400000);a.store<4>(18,0x0,0);
                                        a.jump(0x10368d48);
                                    } else {
                                        a.at(0x10368cc0);a.load<1>(0,31,0x54,false);a.sign8(0,0,true);
                                        if(!a.equal()) {
                                            a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                                            a.jump(0x10368d48);
                                        } else {
                                            a.at(0x10368ccc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x400000);a.store<4>(18,0x0,0);
                                            a.jump(0x10368d48);
                                        }
                                    }
                                } else {
                                    a.at(0x10368cb0);a.compare(0,0xc,true);
                                    if(!a.equal()) {
                                        a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                                        a.jump(0x10368d48);
                                    } else {
                                        a.at(0x10368cb8);a.compare(31,0x0,false);
                                        if(a.equal()) {
                                            a.at(0x10368ccc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x400000);a.store<4>(18,0x0,0);
                                            a.jump(0x10368d48);
                                        } else {
                                            a.at(0x10368cc0);a.load<1>(0,31,0x54,false);a.sign8(0,0,true);
                                            if(!a.equal()) {
                                                a.at(0x10368cdc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
                                                a.jump(0x10368d48);
                                            } else {
                                                a.at(0x10368ccc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x400000);a.store<4>(18,0x0,0);
                                                a.jump(0x10368d48);
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
        a.at(0x10368d48);a.compare(20,0x0,false);
        if(a.equal()) {
            a.at(0x10368e50);a.compare(20,0x0,false);
            if(!a.equal()) {
                a.at(0x10368e58);a.load<4>(3,15,0xc,false);a.load<4>(4,3,0x74,false);a.compare(4,0x0,false);
                if(!a.equal()) {
                    a.at(0x10368e68);a.load<4>(3,2,-0x7a70,false);a.load<4>(0,4,0x8,false);a.load<4>(4,3,0x0,false);
                    a.compare_registers(0,4,true);
                    if(a.equal()) {
                        a.at(0x10368e7c);a.load<4>(3,16,0x2c,false);a.load<4>(5,3,0x74,false);a.compare(5,0x0,false);
                        if(!a.equal()) {
                            a.at(0x10368e8c);a.load<4>(0,5,0x8,false);a.compare_registers(0,4,true);
                            if(!a.equal()) {
                                a.at(0x10368e98);a.load<4>(3,20,0x2c,false);a.load<4>(0,3,0x74,false);a.compare_registers(5,0,false);
                                if(!a.equal()) {
                                    a.at(0x10368ea8);a.floating_load(1,30,0x348,false);a.compare(18,0x0,false);a.floating_product(31,31,1);
                                    if(!a.equal()) {
                                        a.at(0x10368eb8);a.load<4>(0,18,0x0,false);a.flags(0,0,0x2000000);a.store<4>(18,0x0,0);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            a.at(0x10368d50);a.sign8(3,23,false);a.sign8(4,24,false);a.compare_registers(3,4,true);
            if(a.equal()) {
                a.at(0x10368e50);a.compare(20,0x0,false);
                if(!a.equal()) {
                    a.at(0x10368e58);a.load<4>(3,15,0xc,false);a.load<4>(4,3,0x74,false);a.compare(4,0x0,false);
                    if(!a.equal()) {
                        a.at(0x10368e68);a.load<4>(3,2,-0x7a70,false);a.load<4>(0,4,0x8,false);a.load<4>(4,3,0x0,false);
                        a.compare_registers(0,4,true);
                        if(a.equal()) {
                            a.at(0x10368e7c);a.load<4>(3,16,0x2c,false);a.load<4>(5,3,0x74,false);a.compare(5,0x0,false);
                            if(!a.equal()) {
                                a.at(0x10368e8c);a.load<4>(0,5,0x8,false);a.compare_registers(0,4,true);
                                if(!a.equal()) {
                                    a.at(0x10368e98);a.load<4>(3,20,0x2c,false);a.load<4>(0,3,0x74,false);a.compare_registers(5,0,false);
                                    if(!a.equal()) {
                                        a.at(0x10368ea8);a.floating_load(1,30,0x348,false);a.compare(18,0x0,false);a.floating_product(31,31,1);
                                        if(!a.equal()) {
                                            a.at(0x10368eb8);a.load<4>(0,18,0x0,false);a.flags(0,0,0x2000000);a.store<4>(18,0x0,0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                a.at(0x10368d60);a.sign16(0,29,false);a.value(6,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.exclusive_flags(4,4,0x80000000);a.exclusive_flags(5,0,0x80000000);a.store<4>(1,0xb0,3);a.sign16(0,28,false);
                a.floating_load(5,30,0x8,false);a.store<4>(1,0xac,6);a.compare(0,0x1f4,true);a.value(3,0x1f4);
                a.floating_load(1,1,0xac,false);a.store<4>(1,0xb8,5);a.floating_subtract(3,1,5,false);a.store<4>(1,0xb4,6);
                a.floating_load(1,1,0xb4,false);a.store<4>(1,0xc0,4);a.floating_subtract(2,1,5,false);a.store<4>(1,0xbc,6);
                a.floating_load(1,1,0xbc,false);a.floating_product(4,3,2);a.floating_subtract(3,1,5,false);
                if(!a.nonpositive()) {
                    a.at(0x10368dbc);a.move(3,0);
                }
                a.at(0x10368dc0);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xc8,3);
                a.floating_load(2,30,0x8,false);a.store<4>(1,0xc4,0);a.floating_load(5,30,0x148,false);a.floating_load(1,1,0xc4,false);
                a.floating_subtract(1,1,2,false);a.floating_product(1,3,1);a.floating_difference(1,4,1,true,false);a.floating_compare(5,1);
                if(a.nonpositive()) {
                    a.at(0x10368df4);a.floating_load(5,30,0x150,false);a.floating_compare(5,1);
                    if(a.at_least()) {
                        a.at(0x10368e04);a.floating_move(5,1);
                    } else {
                        a.at(0x10368e00);a.jump(0x10368e08);
                    }
                } else {
                    a.at(0x10368df0);a.jump(0x10368e08);
                }
                a.at(0x10368e08);a.load<4>(4,16,0x2c,false);a.floating_product(31,31,5);a.load<4>(3,20,0x2c,false);
                a.load<4>(4,4,0x74,false);a.load<4>(0,3,0x74,false);a.compare_registers(4,0,false);
                if(!a.equal()) {
                    a.at(0x10368e24);a.floating_load(1,30,0x80,false);a.floating_compare(5,1);
                    if(!a.at_least()) {
                        a.at(0x10368e30);a.floating_load(1,30,0x58,false);a.compare(18,0x0,false);a.floating_product(31,31,1);
                        if(!a.equal()) {
                            a.at(0x10368e40);a.load<4>(0,18,0x0,false);a.flags(0,0,0x2000000);a.store<4>(18,0x0,0);
                            a.jump(0x10368ec4);
                        }
                    }
                }
            }
        }
        a.at(0x10368ec4);a.floating_product(31,31,0);a.compare(18,0x0,false);
        if(!a.equal()) {
            a.at(0x10368ed0);a.floating_load(1,30,0x180,false);a.floating_compare(0,1);
            if(!a.at_least()) {
                a.at(0x10368edc);a.load<4>(0,18,0x0,false);a.flags(0,0,0x800000);a.store<4>(18,0x0,0);
            }
        }
    }
    a.at(0x10368ee8);a.load<1>(0,1,0x197,false);a.sign8(0,0,true);
    if(a.equal()) {
        a.at(0x10368f00);a.floating_move(1,31);a.jump(0x10369058);
        return;
    }
    a.at(0x10368ef4);a.floating_load(0,30,0x3a8,false);a.floating_compare(31,0);
    if(!a.at_least()) {
        a.at(0x10368f00);a.floating_move(1,31);a.jump(0x10369058);
        return;
    }
    a.at(0x10368f08);a.offset(3,1,0x48);a.call(0x103721f0);a.load<4>(3,2,-0x7ab8,false);
    a.move(4,16);a.load<4>(3,3,0x0,false);a.call(0x10103510);a.combine(13,3,3,true,false);
    if(a.equal()) {
        a.at(0x10368f28);a.floating_load(1,30,0x158,false);a.jump(0x10369058);
        return;
    }
    a.at(0x10368f30);a.move(4,15);a.move(5,16);a.move(8,17);
    a.offset(3,1,0x48);a.value(6,0x0);a.value(7,0xffffffff);a.value(9,0x0);
    a.call(0x1037b6f0);a.floating_load(0,30,0x48,false);a.floating_load(2,30,0x160,false);a.floating_compare(0,31);
    if(a.nonpositive()) {
        a.at(0x10368f64);a.floating_move(0,31);
    } else {
        a.at(0x10368f60);a.jump(0x10368f68);
    }
    a.at(0x10368f68);a.floating_difference(24,2,0,true,false);a.floating_load(0,30,0x168,false);a.floating_compare(24,0);
    if(!a.at_least()) {
        a.at(0x10368f78);a.floating_move(24,0);
    }
    a.at(0x10368f7c);a.floating_load(0,30,0x3a8,false);a.floating_compare(31,0);
    if(!a.at_least()) {
        a.at(0x10368f88);a.floating_load(0,30,0x3b0,false);a.floating_sum(1,1,0);
    }
    a.at(0x10368f90);a.floating_product(25,24,1);a.move(3,13);a.move(4,17);
    a.call(0x100f4790);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xb0,3);
    a.floating_load(1,30,0x8,false);a.store<4>(1,0xac,0);a.floating_load(0,1,0xac,false);a.floating_subtract(1,0,1,false);
    a.floating_compare(25,1);
    if(!a.nonpositive()) {
        a.at(0x10368fc4);a.floating_load(0,30,0x158,false);a.floating_compare(25,0);
        if(!a.nonpositive()) {
            a.at(0x10368fd0);a.floating_difference(0,1,25,true,false);a.floating_product(31,31,0);a.floating_move(25,1);
        }
    }
    a.at(0x10368fdc);a.compare(31,0x0,false);
    if(!a.equal()) {
        a.at(0x10368fe4);a.load<4>(3,31,0xc,false);a.value(0,0x43300000);a.store<4>(1,0xac,0);
        a.exclusive_flags(0,3,0x80000000);a.floating_load(2,30,0x8,false);a.store<4>(1,0xb0,0);a.floating_load(0,30,0x150,false);
        a.floating_load(1,1,0xac,false);a.floating_subtract(1,1,2,false);a.floating_compare(1,0);
        if(!a.at_least()) {
            a.at(0x10369010);a.floating_move(1,0);
        }
        a.at(0x10369014);a.floating_difference(0,25,1,true,false);a.floating_load(1,30,0x48,false);a.floating_compare(1,0);
        if(a.nonpositive()) {
            a.at(0x10369028);a.floating_load(1,30,0x150,false);a.floating_compare(1,0);
            if(a.at_least()) {
                a.at(0x10369038);a.floating_move(1,0);
            } else {
                a.at(0x10369034);a.jump(0x1036903c);
            }
        } else {
            a.at(0x10369024);a.jump(0x1036903c);
        }
        a.at(0x1036903c);a.floating_load(0,30,0x80,false);a.floating_compare(1,0);a.compare_greater_or_equal_bit();
        if(a.equal()) {
            a.at(0x1036904c);a.floating_difference(0,1,24,true,false);a.floating_product(31,31,0);
        }
    }
    a.at(0x10369054);a.floating_move(1,31);
    return;
}
}
