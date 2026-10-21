#include "shortlist_assessment_detail.hpp"

namespace cm::aot::shortlist_assessment::detail {
void prepare_contribution_context(Call& a) {
    a.at(0x10361490);a.link();a.move(11,1);a.call(0x10773808);
    a.multiple(13,false,-0x20);a.store<4>(1,0x8,0);a.frame(0xe0);a.move(27,6);
    a.move(31,4);a.load<4>(0,6,0x8,false);a.move(25,3);a.load<4>(13,2,-0x7b9c,false);
    a.move(29,5);a.compare(0,0x1,true);a.load<4>(30,2,-0x4ddc,false);a.load<4>(16,2,-0x7b90,false);
    a.move(28,7);a.load<4>(15,2,-0x7b84,false);a.move(26,8);a.load<4>(20,4,0x90,false);
    a.move(14,9);
    if(a.nonpositive()) {
        a.at(0x103614ec);a.value(21,0x1);
    } else {
        a.at(0x103614e4);a.move(21,0);a.jump(0x103614f0);
    }
    a.at(0x103614f0);a.compare(20,0x0,false);a.value(22,0x4);a.value(24,0x0);
    if(!a.equal()) {
        a.at(0x10361500);a.load<4>(0,20,0x1c,false);a.compare(0,0x0,false);
        if(a.equal()) {
            a.at(0x1036150c);a.load<4>(3,2,-0x79f4,false);a.load<4>(17,3,0x0,false);a.compare(17,0x0,true);
            if(!a.equal()) {
                a.at(0x10361524);a.load<4>(3,2,-0x7b88,false);a.load<4>(4,15,0x0,false);a.load<4>(0,3,0x0,false);
                a.offset(3,4,0x2);a.compare_registers(3,0,false);a.store<4>(15,0x0,3);
                if(!a.nonpositive()) {
                    a.at(0x10361540);a.load<4>(0,2,-0x7b8c,false);a.store<4>(15,0x0,0);a.call(0x1077b770);
                    a.mask(0,3,0xffff,false,0);a.store<4>(16,0x0,0);
                }
                a.at(0x10361554);a.value(3,0x10000);a.offset(0,3,-0x1);a.compare_registers(17,0,true);
                if(a.positive()) {
                    a.at(0x10361574);a.value(0,0x43300000);a.exclusive_flags(3,17,0x80000000);a.store<4>(1,0x6c,3);
                    a.value(3,0x80010000);a.load<4>(7,15,0x0,false);a.value(4,0x10000);a.store<4>(1,0x68,0);
                    a.offset(6,3,-0x7fff);a.floating_load(2,30,0x8,false);a.multiply(3,17,0xa);a.floating_load(1,1,0x68,false);
                    a.offset(5,4,-0x1);a.floating_load(0,30,0x0,false);a.floating_subtract(1,1,2,false);a.multiply_high(4,6,3);
                    a.load<2>(8,7,0x0,false);a.load<4>(7,16,0x0,false);a.floating_difference(1,1,0,true,false);a.store<4>(1,0x60,0);
                    a.add(0,8,7);a.multiply_high(6,6,0);a.add(3,4,3);a.add(4,6,0);
                    a.shift_right(4,4,15);a.mask(6,4,0x1,false,1);a.shift_right(3,3,15);a.add(4,4,6);
                    a.product(5,4,5);a.mask(4,3,0x1,false,1);a.add(3,3,4);a.subtract(0,5,0);
                    a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x64,0);a.floating_load(0,1,0x60,false);a.floating_subtract(0,0,2,false);
                    a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x58);a.load<4>(15,1,0x5c,false);
                    a.call(0x10356e40);a.value(4,0x66660000);a.offset(3,3,0x5);a.offset(4,4,0x6667);
                    a.offset(0,17,-0x1);a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,0x1,false,1);
                    a.add(3,3,4);a.add(15,15,3);a.compare_registers(15,0,true);
                    if(!a.nonpositive()) {
                        a.at(0x10361640);a.move(15,0);a.jump(0x10361674);
                    }
                } else {
                    a.at(0x10361564);a.value(3,0xffff0000);a.offset(0,3,0x1);a.compare_registers(17,0,true);
                    if(a.at_least()) {
                        a.at(0x10361648);a.load<4>(3,15,0x0,false);a.compare(17,0x0,true);a.load<4>(0,16,0x0,false);
                        a.load<2>(3,3,0x0,false);a.add(3,3,0);a.divide(0,3,17);a.product(0,0,17);
                        a.subtract(0,0,3);a.negate(15,0);
                        if(!a.nonpositive()) {
                            a.at(0x10361670);a.move(15,0);
                        }
                    } else {
                        a.at(0x10361574);a.value(0,0x43300000);a.exclusive_flags(3,17,0x80000000);a.store<4>(1,0x6c,3);
                        a.value(3,0x80010000);a.load<4>(7,15,0x0,false);a.value(4,0x10000);a.store<4>(1,0x68,0);
                        a.offset(6,3,-0x7fff);a.floating_load(2,30,0x8,false);a.multiply(3,17,0xa);a.floating_load(1,1,0x68,false);
                        a.offset(5,4,-0x1);a.floating_load(0,30,0x0,false);a.floating_subtract(1,1,2,false);a.multiply_high(4,6,3);
                        a.load<2>(8,7,0x0,false);a.load<4>(7,16,0x0,false);a.floating_difference(1,1,0,true,false);a.store<4>(1,0x60,0);
                        a.add(0,8,7);a.multiply_high(6,6,0);a.add(3,4,3);a.add(4,6,0);
                        a.shift_right(4,4,15);a.mask(6,4,0x1,false,1);a.shift_right(3,3,15);a.add(4,4,6);
                        a.product(5,4,5);a.mask(4,3,0x1,false,1);a.add(3,3,4);a.subtract(0,5,0);
                        a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x64,0);a.floating_load(0,1,0x60,false);a.floating_subtract(0,0,2,false);
                        a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x58);a.load<4>(15,1,0x5c,false);
                        a.call(0x10356e40);a.value(4,0x66660000);a.offset(3,3,0x5);a.offset(4,4,0x6667);
                        a.offset(0,17,-0x1);a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,0x1,false,1);
                        a.add(3,3,4);a.add(15,15,3);a.compare_registers(15,0,true);
                        if(!a.nonpositive()) {
                            a.at(0x10361640);a.move(15,0);a.jump(0x10361674);
                        }
                    }
                }
            } else {
                a.at(0x1036151c);a.value(15,0x0);a.jump(0x10361674);
            }
            a.at(0x10361674);a.load<4>(3,2,-0x79f8,false);a.multiply(0,15,0x44);a.load<4>(3,3,0x0,false);
            a.add(0,3,0);a.store<4>(20,0x1c,0);
        }
    }
    return choose_contribution_qualifications(a);
}

void choose_contribution_qualifications(Call& a) {
    a.at(0x10361688);a.load<1>(0,27,0x28,false);a.mask(0,0,0xf,false,29);a.compare(0,0x2,false);
    if(a.equal()) {
        a.at(0x103616a0);a.load<4>(3,27,0x10,false);a.value(23,0x5882);a.load<4>(0,29,0x0,false);
        a.compare_registers(3,0,true);
        if(a.equal()) {
            a.at(0x103616b4);a.load<4>(0,29,0x24,false);a.compare_registers(0,31,false);
            if(a.equal()) {
                a.at(0x103616c0);a.move(3,31);a.value(4,0x1);a.value(5,0x1);
                a.value(6,0x0);a.value(7,0x0);a.call(0x1014c2e0);a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x103616e0);a.flags(23,23,0x810000);
                }
            }
        }
        a.at(0x103616e4);a.load<4>(4,31,0x90,false);a.move(3,29);a.call(0x100ce0a0);
        a.sign8(0,3,false);a.compare(0,0x2,true);
        if(a.equal()) {
            a.at(0x10361730);a.flags(23,23,0x10);a.jump(0x103617bc);
        } else {
            a.at(0x103616fc);
            if(a.at_least()) {
                a.at(0x10361710);a.compare(0,0x4,true);
                if(a.equal()) {
                    a.at(0x10361740);a.flags(23,23,0x20);a.jump(0x103617bc);
                } else {
                    a.at(0x10361718);
                    if(!a.at_least()) {
                        a.at(0x1036171c);a.jump(0x10361738);a.at(0x10361738);a.flags(23,23,0x30);
                        a.jump(0x103617bc);
                    }
                }
            } else {
                a.at(0x10361700);a.compare(0,0x0,true);
                if(a.equal()) {
                    a.at(0x10361720);a.flags(23,23,0x40);a.jump(0x103617bc);
                } else {
                    a.at(0x10361708);
                    if(a.at_least()) {
                        a.at(0x10361728);a.flags(23,23,0x50);a.jump(0x103617bc);
                    } else {
                        a.at(0x1036170c);a.jump(0x103617bc);
                    }
                }
            }
        }
    } else {
        a.at(0x10361698);a.compare(0,0x3,false);
        if(!a.equal()) {
            a.at(0x10361748);a.compare(0,0x4,false);
            if(!a.equal()) {
                a.at(0x103617b8);a.value(23,0x5902);
            } else {
                a.at(0x10361750);a.load<4>(4,31,0x90,false);a.move(3,29);a.value(23,0x5982);
                a.call(0x100ce0a0);a.sign8(0,3,false);a.compare(0,0x2,true);
                if(a.equal()) {
                    a.at(0x103617a0);a.flags(23,23,0x10);a.jump(0x103617bc);
                } else {
                    a.at(0x1036176c);
                    if(a.at_least()) {
                        a.at(0x10361780);a.compare(0,0x4,true);
                        if(a.equal()) {
                            a.at(0x103617b0);a.flags(23,23,0x20);a.jump(0x103617bc);
                        } else {
                            a.at(0x10361788);
                            if(!a.at_least()) {
                                a.at(0x1036178c);a.jump(0x103617a8);a.at(0x103617a8);a.flags(23,23,0x30);
                                a.jump(0x103617bc);
                            }
                        }
                    } else {
                        a.at(0x10361770);a.compare(0,0x0,true);
                        if(a.equal()) {
                            a.at(0x10361790);a.flags(23,23,0x40);a.jump(0x103617bc);
                        } else {
                            a.at(0x10361778);
                            if(a.at_least()) {
                                a.at(0x10361798);a.flags(23,23,0x50);a.jump(0x103617bc);
                            } else {
                                a.at(0x1036177c);a.jump(0x103617bc);
                            }
                        }
                    }
                }
            }
        } else {
            a.at(0x103616a0);a.load<4>(3,27,0x10,false);a.value(23,0x5882);a.load<4>(0,29,0x0,false);
            a.compare_registers(3,0,true);
            if(a.equal()) {
                a.at(0x103616b4);a.load<4>(0,29,0x24,false);a.compare_registers(0,31,false);
                if(a.equal()) {
                    a.at(0x103616c0);a.move(3,31);a.value(4,0x1);a.value(5,0x1);
                    a.value(6,0x0);a.value(7,0x0);a.call(0x1014c2e0);a.sign8(0,3,true);
                    if(!a.equal()) {
                        a.at(0x103616e0);a.flags(23,23,0x810000);
                    }
                }
            }
            a.at(0x103616e4);a.load<4>(4,31,0x90,false);a.move(3,29);a.call(0x100ce0a0);
            a.sign8(0,3,false);a.compare(0,0x2,true);
            if(a.equal()) {
                a.at(0x10361730);a.flags(23,23,0x10);a.jump(0x103617bc);
            } else {
                a.at(0x103616fc);
                if(a.at_least()) {
                    a.at(0x10361710);a.compare(0,0x4,true);
                    if(a.equal()) {
                        a.at(0x10361740);a.flags(23,23,0x20);a.jump(0x103617bc);
                    } else {
                        a.at(0x10361718);
                        if(!a.at_least()) {
                            a.at(0x1036171c);a.jump(0x10361738);a.at(0x10361738);a.flags(23,23,0x30);
                            a.jump(0x103617bc);
                        }
                    }
                } else {
                    a.at(0x10361700);a.compare(0,0x0,true);
                    if(a.equal()) {
                        a.at(0x10361720);a.flags(23,23,0x40);a.jump(0x103617bc);
                    } else {
                        a.at(0x10361708);
                        if(a.at_least()) {
                            a.at(0x10361728);a.flags(23,23,0x50);a.jump(0x103617bc);
                        } else {
                            a.at(0x1036170c);a.jump(0x103617bc);
                        }
                    }
                }
            }
        }
    }
    a.at(0x103617bc);a.load<1>(0,27,0x28,false);a.mask(0,0,0x1,true,25);
    if(!a.equal()) {
        a.at(0x103617c8);a.move(3,29);a.value(4,0x0);a.call(0x100d3230);
        a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x103617e4);a.value(22,0x6);
        } else {
            a.at(0x103617dc);a.value(22,0x10);a.jump(0x103617e8);
        }
    }
    a.at(0x103617e8);a.move(3,31);a.value(4,0x1);a.value(5,0x1);
    a.value(6,0x0);a.value(7,0x0);a.call(0x1014c2e0);a.sign8(0,3,true);
    if(a.equal()) {
        a.at(0x10361948);a.mask(0,23,0x100,true,0);
        if(a.equal()) {
            a.at(0x10361a50);a.mask(0,23,0x80,true,0);
            if(!a.equal()) {
                a.at(0x10361a58);a.compare(20,0x0,false);a.value(15,0x0);
                if(!a.equal()) {
                    a.at(0x10361a64);a.load<4>(3,20,0x1c,false);a.value(4,0x5);a.call(0x100d2f50);
                    a.offset(0,3,0x5);a.move(3,29);a.move(4,20);a.sign8(15,0,false);
                    a.call(0x100ce0a0);a.sign8(0,3,false);a.compare(0,0x4,true);
                    if(a.equal()) {
                        a.at(0x10361aa8);a.offset(15,15,-0xa);
                    } else {
                        a.at(0x10361a90);
                        if(!a.at_least()) {
                            a.at(0x10361a94);a.compare(0,0x3,true);
                            if(a.at_least()) {
                                a.at(0x10361aa0);a.offset(15,15,-0x5);a.jump(0x10361aac);
                            } else {
                                a.at(0x10361a9c);a.jump(0x10361aac);
                            }
                        }
                    }
                }
                a.at(0x10361aac);a.value(19,0x0);a.move(18,19);a.flags(0,0,0x0);
                do {
                    a.at(0x10361ab8);a.offset(17,18,0x174);a.indexed_load<4>(3,31,17);a.compare(3,0x0,false);
                    if(!a.equal()) {
                        a.at(0x10361ac8);a.load<4>(3,3,0x1c,false);a.compare(3,0x0,false);
                        if(!a.equal()) {
                            a.at(0x10361ad4);a.value(4,0x5);a.call(0x100d2f50);a.indexed_load<4>(4,31,17);
                            a.sign8(16,3,false);a.move(3,29);a.call(0x100ce0a0);a.sign8(0,3,false);
                            a.compare(0,0x4,true);
                            if(a.equal()) {
                                a.at(0x10361b10);a.offset(16,16,-0xa);
                            } else {
                                a.at(0x10361af8);
                                if(!a.at_least()) {
                                    a.at(0x10361afc);a.compare(0,0x3,true);
                                    if(a.at_least()) {
                                        a.at(0x10361b08);a.offset(16,16,-0x5);a.jump(0x10361b14);
                                    } else {
                                        a.at(0x10361b04);a.jump(0x10361b14);
                                    }
                                }
                            }
                            a.at(0x10361b14);a.sign8(3,16,false);a.sign8(0,15,false);a.compare_registers(3,0,true);
                            if(!a.nonpositive()) {
                                a.at(0x10361b24);a.move(15,16);a.indexed_load<4>(20,31,17);
                            }
                        }
                    }
                    a.at(0x10361b2c);a.offset(19,19,0x1);a.offset(18,18,0x4);a.compare(19,0x7,true);
                } while(a.less());
            }
        } else {
            a.at(0x10361950);a.compare(20,0x0,false);a.value(19,0x0);
            if(!a.equal()) {
                a.at(0x1036195c);a.load<4>(3,20,0x1c,false);a.compare(3,0x0,false);
                if(!a.equal()) {
                    a.at(0x10361968);a.value(4,0x6);a.call(0x100d2f50);a.sign8(19,3,false);
                    a.move(3,29);a.move(4,20);a.call(0x100ce0a0);a.sign8(0,3,false);
                    a.compare(0,0x4,true);
                    if(a.equal()) {
                        a.at(0x103619a4);a.offset(19,19,-0xa);
                    } else {
                        a.at(0x1036198c);
                        if(!a.at_least()) {
                            a.at(0x10361990);a.compare(0,0x3,true);
                            if(a.at_least()) {
                                a.at(0x1036199c);a.offset(19,19,-0x5);a.jump(0x103619a8);
                            } else {
                                a.at(0x10361998);a.jump(0x103619a8);
                            }
                        }
                    }
                }
            }
            a.at(0x103619a8);a.value(16,0x0);a.value(18,0x0);
            do {
                a.at(0x103619b0);a.offset(17,18,0x174);a.indexed_load<4>(3,31,17);a.compare(3,0x0,false);
                if(!a.equal()) {
                    a.at(0x103619c0);a.load<4>(3,3,0x1c,false);a.compare(3,0x0,false);
                    if(!a.equal()) {
                        a.at(0x103619cc);a.value(4,0x6);a.call(0x100d2f50);a.indexed_load<4>(4,31,17);
                        a.sign8(15,3,false);a.move(3,29);a.call(0x100ce0a0);a.sign8(0,3,false);
                        a.compare(0,0x4,true);
                        if(a.equal()) {
                            a.at(0x10361a14);a.load<2>(0,31,0x1ec,true);a.compare(0,0xdac,true);
                            if(!a.less()) {
                                a.at(0x10361a20);a.offset(15,15,-0xa);a.at(0x10361a24);a.sign8(3,15,false);
                                a.sign8(0,19,false);a.compare_registers(3,0,true);
                                if(!a.nonpositive()) {
                                    a.at(0x10361a34);a.move(19,15);a.indexed_load<4>(20,31,17);
                                }
                            }
                        } else {
                            a.at(0x103619f0);
                            if(a.at_least()) {
                                a.at(0x10361a24);a.sign8(3,15,false);a.sign8(0,19,false);a.compare_registers(3,0,true);
                                if(!a.nonpositive()) {
                                    a.at(0x10361a34);a.move(19,15);a.indexed_load<4>(20,31,17);
                                }
                            } else {
                                a.at(0x103619f4);a.compare(0,0x3,true);
                                if(a.at_least()) {
                                    a.at(0x10361a00);a.load<2>(0,31,0x1ec,true);a.compare(0,0xdac,true);
                                    if(!a.less()) {
                                        a.at(0x10361a0c);a.offset(15,15,-0x5);a.jump(0x10361a24);a.at(0x10361a24);
                                        a.sign8(3,15,false);a.sign8(0,19,false);a.compare_registers(3,0,true);
                                        if(!a.nonpositive()) {
                                            a.at(0x10361a34);a.move(19,15);a.indexed_load<4>(20,31,17);
                                        }
                                    }
                                } else {
                                    a.at(0x103619fc);a.jump(0x10361a24);a.at(0x10361a24);a.sign8(3,15,false);
                                    a.sign8(0,19,false);a.compare_registers(3,0,true);
                                    if(!a.nonpositive()) {
                                        a.at(0x10361a34);a.move(19,15);a.indexed_load<4>(20,31,17);
                                    }
                                }
                            }
                        }
                    }
                }
                a.at(0x10361a3c);a.offset(16,16,0x1);a.offset(18,18,0x4);a.compare(16,0x7,true);
            } while(a.less());
            a.at(0x10361a4c);a.jump(0x10361b3c);
        }
        return calculate_contribution_threshold(a);
    } else {
        a.at(0x10361808);a.move(3,31);a.call(0x100c96e0);a.load<4>(6,2,-0x7b1c,false);
        a.move(5,3);a.move(4,29);a.load<4>(3,6,0x0,false);a.call(0x1030fbf0);
        a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x103618c0);a.move(3,31);a.call(0x1014c5f0);a.value(7,0x43300000);
            a.exclusive_flags(4,26,0x80000000);a.store<4>(1,0x54,4);a.sign8(0,22,false);a.exclusive_flags(0,0,0x80000000);
            a.move(8,3);a.store<4>(1,0x50,7);a.move(3,29);a.floating_load(3,30,0x8,false);
            a.move(5,23);a.floating_load(0,1,0x50,false);a.move(6,31);a.store<4>(1,0x5c,4);
            a.floating_subtract(1,0,3,false);a.floating_load(2,30,0x190,false);a.store<4>(1,0x58,7);a.load<2>(4,27,0x2,true);
            a.floating_load(0,1,0x58,false);a.floating_product(2,2,1);a.store<4>(1,0x64,0);a.floating_subtract(1,0,3,false);
            a.store<4>(1,0x60,7);a.floating_load(0,1,0x60,false);a.floating_product(1,2,1);a.floating_subtract(0,0,3,false);
            a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);a.floating_store(0,1,0x68);a.load<4>(7,1,0x6c,false);
            a.call(0x10216830);a.store<4>(28,0x0,3);a.jump(0x10361c20);
        } else {
            a.at(0x1036182c);a.move(3,31);a.call(0x1014c5f0);a.load<2>(4,27,0x2,true);
            a.move(8,3);a.move(3,29);a.move(5,23);a.move(6,31);
            a.value(7,0xffffffff);a.call(0x10216830);a.store<4>(28,0x0,3);a.value(3,0x43300000);
            a.floating_load(3,30,0x8,false);a.load<4>(4,28,0x0,false);a.store<4>(1,0x60,3);a.exclusive_flags(0,4,0x80000000);
            a.offset(4,4,0x3a98);a.store<4>(1,0x64,0);a.exclusive_flags(0,4,0x80000000);a.floating_load(1,30,0x178,false);
            a.floating_load(0,1,0x60,false);a.store<4>(1,0x5c,0);a.floating_subtract(0,0,3,false);a.store<4>(1,0x58,3);
            a.floating_load(2,1,0x58,false);a.floating_product(1,1,0);a.floating_subtract(0,2,3,false);a.floating_compare(0,1);
            if(!a.nonpositive()) {
                a.at(0x1036189c);a.store<4>(1,0x6c,0);a.store<4>(1,0x68,3);a.floating_load(0,1,0x68,false);
                a.floating_subtract(1,0,3,false);
            }
            a.at(0x103618ac);a.floating_integer(0,1);a.floating_store(0,1,0x50);a.load<4>(0,1,0x54,false);
            a.store<4>(28,0x0,0);a.jump(0x10361c20);
        }
    }
    a.at(0x10361c20);a.load<4>(15,28,0x0,false);a.compare(15,0x0,true);
    if(a.less()) {
        a.at(0x10361c44);a.load<1>(0,27,0x28,false);a.mask(0,0,0x1,true,25);
        if(!a.equal()) {
            a.at(0x10361c50);a.move(3,29);a.value(4,0x0);a.call(0x100d3230);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x10361c64);a.load<4>(4,28,0x0,false);a.value(3,0x43300000);a.store<4>(1,0x58,3);
                a.exclusive_flags(0,4,0x80000000);a.offset(4,4,0x1388);a.store<4>(1,0x5c,0);a.exclusive_flags(0,4,0x80000000);
                a.floating_load(3,30,0x8,false);a.floating_load(0,1,0x58,false);a.store<4>(1,0x54,0);a.floating_subtract(0,0,3,false);
                a.floating_load(1,30,0x170,false);a.store<4>(1,0x50,3);a.floating_load(2,1,0x50,false);a.floating_product(1,1,0);
                a.floating_subtract(0,2,3,false);a.floating_compare(0,1);
                if(!a.nonpositive()) {
                    a.at(0x10361cac);a.store<4>(1,0x64,0);a.store<4>(1,0x60,3);a.floating_load(0,1,0x60,false);
                    a.floating_subtract(1,0,3,false);
                }
                a.at(0x10361cbc);a.floating_integer(0,1);a.floating_store(0,1,0x68);a.load<4>(0,1,0x6c,false);
                a.store<4>(28,0x0,0);
            }
        }
        a.at(0x10361ccc);a.value(3,0x0);a.jump(0x10362d50);
        return;
    }
    a.at(0x10361c2c);a.offset(0,26,-0x2710);a.compare_registers(15,0,true);
    if(a.less()) {
        a.at(0x10361c44);a.load<1>(0,27,0x28,false);a.mask(0,0,0x1,true,25);
        if(!a.equal()) {
            a.at(0x10361c50);a.move(3,29);a.value(4,0x0);a.call(0x100d3230);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x10361c64);a.load<4>(4,28,0x0,false);a.value(3,0x43300000);a.store<4>(1,0x58,3);
                a.exclusive_flags(0,4,0x80000000);a.offset(4,4,0x1388);a.store<4>(1,0x5c,0);a.exclusive_flags(0,4,0x80000000);
                a.floating_load(3,30,0x8,false);a.floating_load(0,1,0x58,false);a.store<4>(1,0x54,0);a.floating_subtract(0,0,3,false);
                a.floating_load(1,30,0x170,false);a.store<4>(1,0x50,3);a.floating_load(2,1,0x50,false);a.floating_product(1,1,0);
                a.floating_subtract(0,2,3,false);a.floating_compare(0,1);
                if(!a.nonpositive()) {
                    a.at(0x10361cac);a.store<4>(1,0x64,0);a.store<4>(1,0x60,3);a.floating_load(0,1,0x60,false);
                    a.floating_subtract(1,0,3,false);
                }
                a.at(0x10361cbc);a.floating_integer(0,1);a.floating_store(0,1,0x68);a.load<4>(0,1,0x6c,false);
                a.store<4>(28,0x0,0);
            }
        }
        a.at(0x10361ccc);a.value(3,0x0);a.jump(0x10362d50);
        return;
    }
    a.at(0x10361c38);a.load<1>(0,27,0x28,false);a.mask(0,0,0x1,true,25);
    if(!a.equal()) {
        a.at(0x10361c44);a.load<1>(0,27,0x28,false);a.mask(0,0,0x1,true,25);
        if(!a.equal()) {
            a.at(0x10361c50);a.move(3,29);a.value(4,0x0);a.call(0x100d3230);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x10361c64);a.load<4>(4,28,0x0,false);a.value(3,0x43300000);a.store<4>(1,0x58,3);
                a.exclusive_flags(0,4,0x80000000);a.offset(4,4,0x1388);a.store<4>(1,0x5c,0);a.exclusive_flags(0,4,0x80000000);
                a.floating_load(3,30,0x8,false);a.floating_load(0,1,0x58,false);a.store<4>(1,0x54,0);a.floating_subtract(0,0,3,false);
                a.floating_load(1,30,0x170,false);a.store<4>(1,0x50,3);a.floating_load(2,1,0x50,false);a.floating_product(1,1,0);
                a.floating_subtract(0,2,3,false);a.floating_compare(0,1);
                if(!a.nonpositive()) {
                    a.at(0x10361cac);a.store<4>(1,0x64,0);a.store<4>(1,0x60,3);a.floating_load(0,1,0x60,false);
                    a.floating_subtract(1,0,3,false);
                }
                a.at(0x10361cbc);a.floating_integer(0,1);a.floating_store(0,1,0x68);a.load<4>(0,1,0x6c,false);
                a.store<4>(28,0x0,0);
            }
        }
        a.at(0x10361ccc);a.value(3,0x0);a.jump(0x10362d50);
        return;
    }
    return adjust_for_existing_interest(a);
}

void calculate_contribution_threshold(Call& a) {
    a.at(0x10361b3c);a.compare(26,-0x1,true);a.load<2>(4,27,0x2,true);a.move(3,29);
    a.move(5,23);a.move(6,31);
    if(a.nonpositive()) {
        a.at(0x10361bb8);a.value(7,0xffffffff);
    } else {
        a.at(0x10361b54);a.value(7,0x43300000);a.exclusive_flags(8,26,0x80000000);a.store<4>(1,0x54,8);
        a.sign8(0,22,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(3,30,0x8,false);a.store<4>(1,0x50,7);
        a.floating_load(2,30,0x190,false);a.floating_load(0,1,0x50,false);a.store<4>(1,0x5c,8);a.floating_subtract(1,0,3,false);
        a.store<4>(1,0x58,7);a.floating_load(0,1,0x58,false);a.floating_product(2,2,1);a.store<4>(1,0x64,0);
        a.floating_subtract(1,0,3,false);a.store<4>(1,0x60,7);a.floating_load(0,1,0x60,false);a.floating_product(1,2,1);
        a.floating_subtract(0,0,3,false);a.floating_difference(0,1,0,true,false);a.floating_integer(0,0);a.floating_store(0,1,0x68);
        a.load<4>(7,1,0x6c,false);a.jump(0x10361bbc);
    }
    a.at(0x10361bbc);a.move(8,20);a.call(0x10216830);a.store<4>(28,0x0,3);
    a.load<4>(0,29,0x24,false);a.compare_registers(31,0,false);
    if(!a.equal()) {
        a.at(0x10361bd4);a.move(3,29);a.value(4,0x0);a.call(0x100d3150);
        a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x10361be8);a.load<4>(3,28,0x0,false);a.value(0,0x43300000);a.store<4>(1,0x50,0);
            a.exclusive_flags(0,3,0x80000000);a.floating_load(1,30,0x8,false);a.store<4>(1,0x54,0);a.floating_load(2,30,0x120,false);
            a.floating_load(0,1,0x50,false);a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_integer(0,0);
            a.floating_store(0,1,0x58);a.load<4>(0,1,0x5c,false);a.store<4>(28,0x0,0);
        }
    }
    a.at(0x10361c20);a.load<4>(15,28,0x0,false);a.compare(15,0x0,true);
    if(a.less()) {
        a.at(0x10361c44);a.load<1>(0,27,0x28,false);a.mask(0,0,0x1,true,25);
        if(!a.equal()) {
            a.at(0x10361c50);a.move(3,29);a.value(4,0x0);a.call(0x100d3230);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x10361c64);a.load<4>(4,28,0x0,false);a.value(3,0x43300000);a.store<4>(1,0x58,3);
                a.exclusive_flags(0,4,0x80000000);a.offset(4,4,0x1388);a.store<4>(1,0x5c,0);a.exclusive_flags(0,4,0x80000000);
                a.floating_load(3,30,0x8,false);a.floating_load(0,1,0x58,false);a.store<4>(1,0x54,0);a.floating_subtract(0,0,3,false);
                a.floating_load(1,30,0x170,false);a.store<4>(1,0x50,3);a.floating_load(2,1,0x50,false);a.floating_product(1,1,0);
                a.floating_subtract(0,2,3,false);a.floating_compare(0,1);
                if(!a.nonpositive()) {
                    a.at(0x10361cac);a.store<4>(1,0x64,0);a.store<4>(1,0x60,3);a.floating_load(0,1,0x60,false);
                    a.floating_subtract(1,0,3,false);
                }
                a.at(0x10361cbc);a.floating_integer(0,1);a.floating_store(0,1,0x68);a.load<4>(0,1,0x6c,false);
                a.store<4>(28,0x0,0);
            }
        }
        a.at(0x10361ccc);a.value(3,0x0);a.jump(0x10362d50);
        return;
    }
    a.at(0x10361c2c);a.offset(0,26,-0x2710);a.compare_registers(15,0,true);
    if(a.less()) {
        a.at(0x10361c44);a.load<1>(0,27,0x28,false);a.mask(0,0,0x1,true,25);
        if(!a.equal()) {
            a.at(0x10361c50);a.move(3,29);a.value(4,0x0);a.call(0x100d3230);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x10361c64);a.load<4>(4,28,0x0,false);a.value(3,0x43300000);a.store<4>(1,0x58,3);
                a.exclusive_flags(0,4,0x80000000);a.offset(4,4,0x1388);a.store<4>(1,0x5c,0);a.exclusive_flags(0,4,0x80000000);
                a.floating_load(3,30,0x8,false);a.floating_load(0,1,0x58,false);a.store<4>(1,0x54,0);a.floating_subtract(0,0,3,false);
                a.floating_load(1,30,0x170,false);a.store<4>(1,0x50,3);a.floating_load(2,1,0x50,false);a.floating_product(1,1,0);
                a.floating_subtract(0,2,3,false);a.floating_compare(0,1);
                if(!a.nonpositive()) {
                    a.at(0x10361cac);a.store<4>(1,0x64,0);a.store<4>(1,0x60,3);a.floating_load(0,1,0x60,false);
                    a.floating_subtract(1,0,3,false);
                }
                a.at(0x10361cbc);a.floating_integer(0,1);a.floating_store(0,1,0x68);a.load<4>(0,1,0x6c,false);
                a.store<4>(28,0x0,0);
            }
        }
        a.at(0x10361ccc);a.value(3,0x0);a.jump(0x10362d50);
        return;
    }
    a.at(0x10361c38);a.load<1>(0,27,0x28,false);a.mask(0,0,0x1,true,25);
    if(!a.equal()) {
        a.at(0x10361c44);a.load<1>(0,27,0x28,false);a.mask(0,0,0x1,true,25);
        if(!a.equal()) {
            a.at(0x10361c50);a.move(3,29);a.value(4,0x0);a.call(0x100d3230);
            a.sign8(0,3,true);
            if(!a.equal()) {
                a.at(0x10361c64);a.load<4>(4,28,0x0,false);a.value(3,0x43300000);a.store<4>(1,0x58,3);
                a.exclusive_flags(0,4,0x80000000);a.offset(4,4,0x1388);a.store<4>(1,0x5c,0);a.exclusive_flags(0,4,0x80000000);
                a.floating_load(3,30,0x8,false);a.floating_load(0,1,0x58,false);a.store<4>(1,0x54,0);a.floating_subtract(0,0,3,false);
                a.floating_load(1,30,0x170,false);a.store<4>(1,0x50,3);a.floating_load(2,1,0x50,false);a.floating_product(1,1,0);
                a.floating_subtract(0,2,3,false);a.floating_compare(0,1);
                if(!a.nonpositive()) {
                    a.at(0x10361cac);a.store<4>(1,0x64,0);a.store<4>(1,0x60,3);a.floating_load(0,1,0x60,false);
                    a.floating_subtract(1,0,3,false);
                }
                a.at(0x10361cbc);a.floating_integer(0,1);a.floating_store(0,1,0x68);a.load<4>(0,1,0x6c,false);
                a.store<4>(28,0x0,0);
            }
        }
        a.at(0x10361ccc);a.value(3,0x0);a.jump(0x10362d50);
        return;
    }
    return adjust_for_existing_interest(a);
}

void adjust_for_existing_interest(Call& a) {
    a.at(0x10361cd4);a.load<4>(0,29,0x0,false);a.load<4>(4,13,0x0,false);a.multiply(3,0,0x64);
    a.offset(0,3,0xc);a.indexed_load<1>(0,4,0);a.mask(3,0,0xf,true,28);
    if(!a.equal()) {
        a.at(0x10361cf0);a.value(0,0x43300000);a.store<4>(1,0x54,3);a.floating_load(1,30,0x2b8,false);
        a.store<4>(1,0x50,0);a.floating_load(2,30,0xe8,false);a.floating_load(0,1,0x50,false);a.floating_subtract(1,0,1,false);
        a.call(0x10780fc0);a.load<4>(2,1,0x14,false);a.floating_load(0,30,0x30,false);a.value(5,0x43300000);
        a.exclusive_flags(3,15,0x80000000);a.offset(0,15,0x1388);a.floating_difference(3,1,0,true,false);a.floating_load(1,30,0x80,false);
        a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x5c,3);a.floating_load(2,30,0x8,false);a.store<4>(1,0x58,5);
        a.floating_load(0,1,0x58,false);a.floating_sum(3,1,3);a.store<4>(1,0x64,0);a.floating_subtract(1,0,2,false);
        a.store<4>(1,0x60,5);a.floating_load(0,1,0x60,false);a.floating_product(1,1,3);a.floating_subtract(0,0,2,false);
        a.floating_compare(0,1);
        if(a.at_least()) {
            a.at(0x10361d78);a.load<4>(0,29,0x0,false);a.load<4>(4,13,0x0,false);a.multiply(3,0,0x64);
            a.store<4>(1,0x48,5);a.floating_load(1,30,0x2b8,false);a.floating_load(2,30,0xe8,false);a.offset(0,3,0xc);
            a.indexed_load<1>(0,4,0);a.mask(0,0,0xf,false,28);a.store<4>(1,0x4c,0);a.floating_load(0,1,0x48,false);
            a.floating_subtract(1,0,1,false);a.call(0x10780fc0);a.load<4>(2,1,0x14,false);a.floating_load(0,30,0x30,false);
            a.value(0,0x43300000);a.load<4>(3,28,0x0,false);a.floating_difference(3,1,0,true,false);a.floating_load(2,30,0x80,false);
            a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x40,0);a.floating_load(1,30,0x8,false);a.store<4>(1,0x44,3);
            a.floating_load(0,1,0x40,false);a.floating_sum(2,2,3);a.floating_subtract(0,0,1,false);a.floating_product(0,0,2);
        } else {
            a.at(0x10361d64);a.store<4>(1,0x6c,0);a.store<4>(1,0x68,5);a.floating_load(0,1,0x68,false);
            a.floating_subtract(0,0,2,false);a.jump(0x10361de4);
        }
        a.at(0x10361de4);a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load<4>(0,1,0x3c,false);
        a.store<4>(28,0x0,0);
    }
    return choose_contribution_coefficients(a);
}

void choose_contribution_coefficients(Call& a) {
    a.at(0x10361df4);a.load<2>(4,27,0x2,true);a.move(3,25);a.call(0x10359980);
    a.sign8(0,3,false);a.value(23,0x1c);a.compare(0,0x15,false);
    if(a.positive()) {
        a.at(0x10361e60);a.floating_load(30,30,0x1c8,false);a.value(23,0x20);a.floating_load(31,30,0x1d0,false);
    } else {
        a.at(0x10361e10);a.load<4>(3,2,-0x4dec,false);a.mask(0,0,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
        a.count_destination(3);a.count_branch();
        switch(a.r(0)>>2) {
            case 0:case 15:case 16: {
                a.at(0x10361e60);a.floating_load(30,30,0x1c8,false);a.value(23,0x20);a.floating_load(31,30,0x1d0,false);
                break;
            }
            case 1:case 2:case 3:case 4:case 17: {
                a.at(0x10361e54);a.floating_load(30,30,0x1c0,false);a.floating_load(31,30,0x180,false);a.jump(0x10361e6c);
                break;
            }
            case 5:case 6:case 7:case 19: {
                a.at(0x10361e48);a.floating_load(30,30,0x1b0,false);a.floating_load(31,30,0x1b8,false);a.jump(0x10361e6c);
                break;
            }
            case 8:case 9:case 10:case 18: {
                a.at(0x10361e3c);a.floating_load(30,30,0x1a8,false);a.floating_load(31,30,0xa0,false);a.jump(0x10361e6c);
                break;
            }
            case 11:case 12:case 13:case 21: {
                a.at(0x10361e30);a.floating_load(30,30,0x1a0,false);a.floating_load(31,30,0xf8,false);a.jump(0x10361e6c);
                break;
            }
            case 14:case 20: {
                a.at(0x10361e24);a.floating_load(30,30,0x198,false);a.floating_load(31,30,0x80,false);a.jump(0x10361e6c);
                break;
            }
        }
    }
    return calculate_contribution_baseline(a);
}

void calculate_contribution_baseline(Call& a) {
    a.at(0x10361e6c);a.value(3,0x10000);a.load<4>(17,28,0x0,false);a.offset(0,3,-0x7748);
    a.compare_registers(17,0,true);
    if(a.less()) {
        a.at(0x10361ea0);a.load<4>(0,27,0xc,false);a.value(3,0x43300000);a.load<4>(4,28,0x0,false);
        a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x40,3);a.exclusive_flags(4,4,0x80000000);a.floating_load(3,30,0x8,false);
        a.store<4>(1,0x44,0);a.floating_load(1,30,0x18,false);a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,4);
        a.floating_subtract(0,0,3,false);a.floating_load(4,30,0xa0,false);a.store<4>(1,0x38,3);a.floating_load(5,30,0x1d8,false);
        a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_product_sum(0,4,1,0);
        a.floating_product(1,5,0);a.call(0x10773780);a.move(15,3);a.jump(0x10361f6c);
    } else {
        a.at(0x10361e80);a.shift_right(0,17,3);a.load<4>(4,27,0xc,false);a.add_carry(0,0);
        a.compare_registers(4,0,true);
        if(a.less()) {
            a.at(0x10361ea0);a.load<4>(0,27,0xc,false);a.value(3,0x43300000);a.load<4>(4,28,0x0,false);
            a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x40,3);a.exclusive_flags(4,4,0x80000000);a.floating_load(3,30,0x8,false);
            a.store<4>(1,0x44,0);a.floating_load(1,30,0x18,false);a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,4);
            a.floating_subtract(0,0,3,false);a.floating_load(4,30,0xa0,false);a.store<4>(1,0x38,3);a.floating_load(5,30,0x1d8,false);
            a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_product_sum(0,4,1,0);
            a.floating_product(1,5,0);a.call(0x10773780);a.move(15,3);a.jump(0x10361f6c);
        } else {
            a.at(0x10361e94);a.load<2>(0,27,0x2,true);a.compare(0,0x80,true);
            if(a.at_least()) {
                a.at(0x10361efc);a.value(3,0x43300000);a.exclusive_flags(0,17,0x80000000);a.store<4>(1,0x4c,0);
                a.exclusive_flags(4,4,0x80000000);a.floating_load(3,30,0x8,false);a.value(24,0x1);a.store<4>(1,0x48,3);
                a.floating_load(2,30,0x1e0,false);a.floating_load(1,1,0x48,false);a.floating_load(0,30,0x80,false);a.floating_subtract(1,1,3,true);
                a.store<4>(1,0x44,0);a.floating_load(4,30,0x18,false);a.store<4>(1,0x40,3);a.floating_product_difference(0,2,1,0);
                a.floating_load(5,30,0x1d8,false);a.floating_load(1,1,0x40,false);a.store<4>(1,0x3c,4);a.floating_subtract(1,1,3,true);
                a.store<4>(1,0x38,3);a.floating_round_single(0,0);a.floating_load(2,1,0x38,false);a.floating_subtract(2,2,3,false);
                a.floating_product_single(0,1,0);a.floating_product_sum(0,4,2,0);a.floating_product(1,5,0);a.call(0x10773780);
                a.move(15,3);
            } else {
                a.at(0x10361ea0);a.load<4>(0,27,0xc,false);a.value(3,0x43300000);a.load<4>(4,28,0x0,false);
                a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x40,3);a.exclusive_flags(4,4,0x80000000);a.floating_load(3,30,0x8,false);
                a.store<4>(1,0x44,0);a.floating_load(1,30,0x18,false);a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,4);
                a.floating_subtract(0,0,3,false);a.floating_load(4,30,0xa0,false);a.store<4>(1,0x38,3);a.floating_load(5,30,0x1d8,false);
                a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_product_sum(0,4,1,0);
                a.floating_product(1,5,0);a.call(0x10773780);a.move(15,3);a.jump(0x10361f6c);
            }
        }
    }
    a.at(0x10361f6c);a.compare(15,0x1,false);
    if(!a.at_least()) {
        a.at(0x10361f74);a.value(15,0x1);
    }
    a.at(0x10361f78);a.load<1>(0,27,0x28,false);a.load<2>(3,29,0x5c,true);a.mask(0,0,0xf,false,29);
    a.compare(0,0x4,true);a.sign8(22,3,false);
    if(a.equal()) {
        a.at(0x10361fb8);a.value(0,0x43300000);a.store<4>(1,0x3c,15);a.floating_load(2,30,0x2b8,false);
        a.store<4>(1,0x38,0);a.floating_load(0,30,0x68,false);a.floating_load(1,1,0x38,false);a.floating_subtract(1,1,2,false);
        a.floating_product(1,1,0);a.call(0x10773780);a.move(15,3);a.jump(0x103624dc);
    } else {
        a.at(0x10361f90);
        if(a.at_least()) {
            a.at(0x10361fa8);a.compare(0,0x6,true);
            if(a.equal()) {
                a.at(0x103624b4);a.value(0,0x43300000);a.store<4>(1,0x3c,15);a.floating_load(2,30,0x2b8,false);
                a.store<4>(1,0x38,0);a.floating_load(0,30,0x208,false);a.floating_load(1,1,0x38,false);a.floating_subtract(1,1,2,false);
                a.floating_difference(1,1,0,true,false);a.call(0x10773780);a.move(15,3);
            } else {
                a.at(0x10361fb0);
                if(!a.at_least()) {
                    a.at(0x10361fb4);a.jump(0x10361fe4);a.at(0x10361fe4);a.value(0,0x43300000);
                    a.store<4>(1,0x3c,15);a.floating_load(2,30,0x2b8,false);a.store<4>(1,0x38,0);a.floating_load(0,30,0x140,false);
                    a.floating_load(1,1,0x38,false);a.floating_subtract(1,1,2,false);a.floating_difference(1,1,0,true,false);a.call(0x10773780);
                    a.move(15,3);a.offset(23,23,-0x9);a.jump(0x103624dc);
                }
            }
        } else {
            a.at(0x10361f94);a.compare(0,0x3,true);
            if(a.at_least()) {
                a.at(0x10362234);a.load<2>(0,27,0x2,true);a.compare(0,0x80,true);
                if(a.at_least()) {
                    a.at(0x10362268);a.value(0,0x43300000);a.store<4>(1,0x44,15);a.floating_load(2,30,0x2b8,false);
                    a.store<4>(1,0x40,0);a.floating_load(0,30,0x200,false);a.floating_load(1,1,0x40,false);a.floating_subtract(1,1,2,false);
                    a.floating_difference(1,1,0,true,false);a.call(0x10773780);
                } else {
                    a.at(0x10362240);a.value(0,0x43300000);a.store<4>(1,0x3c,15);a.floating_load(2,30,0x2b8,false);
                    a.store<4>(1,0x38,0);a.floating_load(0,30,0x178,false);a.floating_load(1,1,0x38,false);a.floating_subtract(1,1,2,false);
                    a.floating_difference(1,1,0,true,false);a.call(0x10773780);a.jump(0x1036228c);
                }
                a.at(0x1036228c);a.compare(22,0x1b,true);a.move(15,3);
                if(!a.at_least()) {
                    a.at(0x10362298);a.compare(17,0x4e20,true);
                    if(!a.nonpositive()) {
                        a.at(0x103622a0);a.load<4>(0,28,0x0,false);a.move(4,29);a.load<4>(3,31,0x90,false);
                        a.move(5,31);a.shift_right(0,0,2);a.value(6,0x1);a.add_carry(0,0);
                        a.sign16(7,0,false);a.call(0x10219e40);a.sign16(0,3,false);a.value(4,0x43300000);
                        a.exclusive_flags(3,0,0x80000000);a.load<4>(0,28,0x0,false);a.store<4>(1,0x3c,3);a.exclusive_flags(3,0,0x80000000);
                        a.floating_load(0,30,0x180,false);a.store<4>(1,0x38,4);a.floating_load(2,30,0x8,false);a.floating_sum(3,0,31);
                        a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,3);a.floating_subtract(4,0,2,false);a.floating_load(1,30,0x1e8,false);
                        a.store<4>(1,0x40,4);a.floating_load(0,1,0x40,false);a.floating_product(1,1,4);a.floating_subtract(0,0,2,false);
                        a.floating_difference(4,1,0,true,false);a.floating_compare(4,3);
                        if(a.at_least()) {
                            a.at(0x1036231c);a.floating_move(4,3);
                        } else {
                            a.at(0x10362318);a.jump(0x10362320);
                        }
                        a.at(0x10362320);a.compare(22,0x19,true);
                        if(a.equal()) {
                            a.at(0x10362354);a.floating_load(0,30,0x68,false);a.floating_product_sum(4,0,4,0);a.jump(0x1036236c);
                        } else {
                            a.at(0x10362328);
                            if(a.at_least()) {
                                a.at(0x10362338);a.compare(22,0x1b,true);
                                if(!a.at_least()) {
                                    a.at(0x10362340);a.jump(0x10362360);a.at(0x10362360);a.floating_load(1,30,0x1f8,false);
                                    a.floating_load(0,30,0x1f0,false);a.floating_product_sum(4,1,4,0);
                                }
                            } else {
                                a.at(0x1036232c);a.compare(22,0x18,true);
                                if(a.at_least()) {
                                    a.at(0x10362344);a.floating_load(1,30,0x1f0,false);a.floating_load(0,30,0x1f8,false);a.floating_product_sum(4,1,4,0);
                                    a.jump(0x1036236c);
                                } else {
                                    a.at(0x10362334);a.jump(0x1036236c);
                                }
                            }
                        }
                        a.at(0x1036236c);a.value(4,0x43300000);a.exclusive_flags(3,26,0x80000000);a.store<4>(1,0x44,3);
                        a.exclusive_flags(3,0,0x80000000);a.floating_load(3,30,0x8,false);a.store<4>(1,0x40,4);a.floating_load(1,30,0x120,false);
                        a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,3);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,4);
                        a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_compare(1,0);
                        if(a.positive()) {
                            a.at(0x103623b4);a.floating_load(0,30,0x80,false);a.floating_compare(4,0);
                            if(!a.nonpositive()) {
                                a.at(0x103623c0);a.load<2>(4,31,0x1ec,true);a.value(3,0x3e8);a.compare(4,0x3e8,true);
                                if(!a.less()) {
                                    a.at(0x103623d0);a.move(3,4);
                                }
                                a.at(0x103623d4);a.load<4>(5,31,0x0,false);a.value(4,0x43300000);a.exclusive_flags(3,3,0x80000000);
                                a.load<4>(6,25,0xdc,false);a.multiply(5,5,0xc);a.store<4>(1,0x38,4);a.floating_load(2,30,0x8,false);
                                a.store<4>(1,0x44,3);a.add(3,6,5);a.floating_load(3,30,0x80,false);a.load<2>(3,3,0x6,true);
                                a.store<4>(1,0x40,4);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x40,false);
                                a.floating_load(1,1,0x38,false);a.floating_subtract(0,0,2,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);
                                a.floating_compare(3,0);
                                if(a.nonpositive()) {
                                    a.at(0x1036242c);a.floating_load(3,30,0x170,false);a.floating_compare(3,0);
                                    if(a.at_least()) {
                                        a.at(0x1036243c);a.floating_move(3,0);
                                    } else {
                                        a.at(0x10362438);a.jump(0x10362440);
                                    }
                                } else {
                                    a.at(0x10362428);a.jump(0x10362440);
                                }
                                a.at(0x10362440);a.floating_product(4,4,3);
                            }
                        } else {
                            a.at(0x103623ac);a.sign8(3,24,true);
                            if(!a.equal()) {
                                a.at(0x103623b4);a.floating_load(0,30,0x80,false);a.floating_compare(4,0);
                                if(!a.nonpositive()) {
                                    a.at(0x103623c0);a.load<2>(4,31,0x1ec,true);a.value(3,0x3e8);a.compare(4,0x3e8,true);
                                    if(!a.less()) {
                                        a.at(0x103623d0);a.move(3,4);
                                    }
                                    a.at(0x103623d4);a.load<4>(5,31,0x0,false);a.value(4,0x43300000);a.exclusive_flags(3,3,0x80000000);
                                    a.load<4>(6,25,0xdc,false);a.multiply(5,5,0xc);a.store<4>(1,0x38,4);a.floating_load(2,30,0x8,false);
                                    a.store<4>(1,0x44,3);a.add(3,6,5);a.floating_load(3,30,0x80,false);a.load<2>(3,3,0x6,true);
                                    a.store<4>(1,0x40,4);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x40,false);
                                    a.floating_load(1,1,0x38,false);a.floating_subtract(0,0,2,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);
                                    a.floating_compare(3,0);
                                    if(a.nonpositive()) {
                                        a.at(0x1036242c);a.floating_load(3,30,0x170,false);a.floating_compare(3,0);
                                        if(a.at_least()) {
                                            a.at(0x1036243c);a.floating_move(3,0);
                                        } else {
                                            a.at(0x10362438);a.jump(0x10362440);
                                        }
                                    } else {
                                        a.at(0x10362428);a.jump(0x10362440);
                                    }
                                    a.at(0x10362440);a.floating_product(4,4,3);
                                }
                            }
                        }
                        a.at(0x10362444);a.floating_load(0,30,0x80,false);a.floating_compare(4,0);
                        if(!a.nonpositive()) {
                            a.at(0x10362450);a.value(3,0x43300000);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x4c,0);
                            a.floating_product(31,31,4);a.floating_load(0,30,0x68,false);a.store<4>(1,0x48,3);a.floating_load(1,30,0x8,false);
                            a.floating_product_sum(3,4,0,0);a.floating_load(0,1,0x48,false);a.store<4>(1,0x3c,15);a.floating_subtract(0,0,1,false);
                            a.floating_load(2,30,0x2b8,false);a.store<4>(1,0x38,3);a.floating_load(1,1,0x38,false);a.floating_product(0,0,3);
                            a.floating_subtract(1,1,2,false);a.floating_integer(0,0);a.floating_product(1,1,3);a.floating_store(0,1,0x50);
                            a.floating_integer(0,1);a.load<4>(0,1,0x54,false);a.store<4>(28,0x0,0);a.floating_store(0,1,0x40);
                            a.load<4>(15,1,0x44,false);a.jump(0x103624dc);
                        }
                    }
                }
            } else {
                a.at(0x10361f9c);a.compare(0,0x1,true);
                if(a.at_least()) {
                    a.at(0x10362014);a.compare(22,0x1b,true);
                    if(!a.at_least()) {
                        a.at(0x1036201c);a.compare(17,0x4e20,true);
                        if(!a.nonpositive()) {
                            a.at(0x10362024);a.load<4>(0,28,0x0,false);a.move(4,29);a.load<4>(3,31,0x90,false);
                            a.move(5,31);a.shift_right(0,0,2);a.value(6,0x1);a.add_carry(0,0);
                            a.sign16(7,0,false);a.call(0x10219e40);a.sign16(0,3,false);a.value(4,0x43300000);
                            a.exclusive_flags(3,0,0x80000000);a.load<4>(0,28,0x0,false);a.store<4>(1,0x3c,3);a.exclusive_flags(3,0,0x80000000);
                            a.floating_load(0,30,0x180,false);a.store<4>(1,0x38,4);a.floating_load(2,30,0x8,false);a.floating_sum(3,0,31);
                            a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,3);a.floating_subtract(4,0,2,false);a.floating_load(1,30,0x1e8,false);
                            a.store<4>(1,0x40,4);a.floating_load(0,1,0x40,false);a.floating_product(1,1,4);a.floating_subtract(0,0,2,false);
                            a.floating_difference(4,1,0,true,false);a.floating_compare(4,3);
                            if(a.at_least()) {
                                a.at(0x103620a0);a.floating_move(4,3);
                            } else {
                                a.at(0x1036209c);a.jump(0x103620a4);
                            }
                            a.at(0x103620a4);a.compare(22,0x19,true);
                            if(a.equal()) {
                                a.at(0x103620d4);a.floating_load(0,30,0x68,false);a.floating_product_sum(4,0,4,0);a.jump(0x103620ec);
                            } else {
                                a.at(0x103620ac);
                                if(a.at_least()) {
                                    a.at(0x103620bc);a.compare(22,0x1b,true);
                                    if(!a.at_least()) {
                                        a.at(0x103620c4);a.floating_load(1,30,0x1f8,false);a.floating_load(0,30,0x1f0,false);a.floating_product_sum(4,1,4,0);
                                        a.jump(0x103620ec);
                                    }
                                } else {
                                    a.at(0x103620b0);a.compare(22,0x18,true);
                                    if(a.at_least()) {
                                        a.at(0x103620e0);a.floating_load(1,30,0x1f0,false);a.floating_load(0,30,0x1f8,false);a.floating_product_sum(4,1,4,0);
                                    } else {
                                        a.at(0x103620b8);a.jump(0x103620ec);
                                    }
                                }
                            }
                            a.at(0x103620ec);a.value(4,0x43300000);a.exclusive_flags(3,26,0x80000000);a.store<4>(1,0x44,3);
                            a.exclusive_flags(3,0,0x80000000);a.floating_load(3,30,0x8,false);a.store<4>(1,0x40,4);a.floating_load(1,30,0x120,false);
                            a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,3);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,4);
                            a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_compare(1,0);
                            if(a.positive()) {
                                a.at(0x10362134);a.floating_load(0,30,0x80,false);a.floating_compare(4,0);
                                if(!a.nonpositive()) {
                                    a.at(0x10362140);a.load<2>(4,31,0x1ec,true);a.value(3,0x3e8);a.compare(4,0x3e8,true);
                                    if(!a.less()) {
                                        a.at(0x10362150);a.move(3,4);
                                    }
                                    a.at(0x10362154);a.load<4>(5,31,0x0,false);a.value(4,0x43300000);a.exclusive_flags(3,3,0x80000000);
                                    a.load<4>(6,25,0xdc,false);a.multiply(5,5,0xc);a.store<4>(1,0x38,4);a.floating_load(2,30,0x8,false);
                                    a.store<4>(1,0x44,3);a.add(3,6,5);a.floating_load(3,30,0x80,false);a.load<2>(3,3,0x6,true);
                                    a.store<4>(1,0x40,4);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x40,false);
                                    a.floating_load(1,1,0x38,false);a.floating_subtract(0,0,2,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);
                                    a.floating_compare(3,0);
                                    if(a.nonpositive()) {
                                        a.at(0x103621ac);a.floating_load(3,30,0x170,false);a.floating_compare(3,0);
                                        if(a.at_least()) {
                                            a.at(0x103621bc);a.floating_move(3,0);
                                        } else {
                                            a.at(0x103621b8);a.jump(0x103621c0);
                                        }
                                    } else {
                                        a.at(0x103621a8);a.jump(0x103621c0);
                                    }
                                    a.at(0x103621c0);a.floating_product(4,4,3);
                                }
                            } else {
                                a.at(0x1036212c);a.sign8(3,24,true);
                                if(!a.equal()) {
                                    a.at(0x10362134);a.floating_load(0,30,0x80,false);a.floating_compare(4,0);
                                    if(!a.nonpositive()) {
                                        a.at(0x10362140);a.load<2>(4,31,0x1ec,true);a.value(3,0x3e8);a.compare(4,0x3e8,true);
                                        if(!a.less()) {
                                            a.at(0x10362150);a.move(3,4);
                                        }
                                        a.at(0x10362154);a.load<4>(5,31,0x0,false);a.value(4,0x43300000);a.exclusive_flags(3,3,0x80000000);
                                        a.load<4>(6,25,0xdc,false);a.multiply(5,5,0xc);a.store<4>(1,0x38,4);a.floating_load(2,30,0x8,false);
                                        a.store<4>(1,0x44,3);a.add(3,6,5);a.floating_load(3,30,0x80,false);a.load<2>(3,3,0x6,true);
                                        a.store<4>(1,0x40,4);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x40,false);
                                        a.floating_load(1,1,0x38,false);a.floating_subtract(0,0,2,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);
                                        a.floating_compare(3,0);
                                        if(a.nonpositive()) {
                                            a.at(0x103621ac);a.floating_load(3,30,0x170,false);a.floating_compare(3,0);
                                            if(a.at_least()) {
                                                a.at(0x103621bc);a.floating_move(3,0);
                                            } else {
                                                a.at(0x103621b8);a.jump(0x103621c0);
                                            }
                                        } else {
                                            a.at(0x103621a8);a.jump(0x103621c0);
                                        }
                                        a.at(0x103621c0);a.floating_product(4,4,3);
                                    }
                                }
                            }
                            a.at(0x103621c4);a.floating_load(0,30,0x80,false);a.floating_compare(4,0);
                            if(!a.nonpositive()) {
                                a.at(0x103621d0);a.value(3,0x43300000);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x4c,0);
                                a.floating_product(31,31,4);a.floating_load(0,30,0x68,false);a.store<4>(1,0x48,3);a.floating_load(1,30,0x8,false);
                                a.floating_product_sum(3,4,0,0);a.floating_load(0,1,0x48,false);a.store<4>(1,0x3c,15);a.floating_subtract(0,0,1,false);
                                a.floating_load(2,30,0x2b8,false);a.store<4>(1,0x38,3);a.floating_load(1,1,0x38,false);a.floating_product(0,0,3);
                                a.floating_subtract(1,1,2,false);a.floating_integer(0,0);a.floating_product(1,1,3);a.floating_store(0,1,0x50);
                                a.floating_integer(0,1);a.load<4>(0,1,0x54,false);a.store<4>(28,0x0,0);a.floating_store(0,1,0x40);
                                a.load<4>(15,1,0x44,false);a.jump(0x103624dc);
                            }
                        }
                    }
                } else {
                    a.at(0x10361fa4);a.jump(0x103624dc);
                }
            }
        }
    }
    a.at(0x103624dc);a.sign8(16,23,false);a.compare_registers(22,16,true);
    if(!a.nonpositive()) {
        a.at(0x103624e8);a.subtract(13,16,22);a.value(4,0x43300000);a.exclusive_flags(0,13,0x80000000);
        a.load<4>(3,29,0x14,false);a.store<4>(1,0x3c,0);a.load<2>(0,3,0xc,true);a.store<4>(1,0x38,4);
        a.floating_load(3,30,0x8,false);a.compare(0,0x1770,true);a.floating_load(0,1,0x38,false);a.floating_load(2,30,0x1f8,false);
        a.floating_subtract(1,0,3,false);a.floating_load(0,30,0xf8,false);a.floating_load(4,30,0x80,false);a.floating_load(29,30,0x158,false);
        a.floating_product_sum(0,2,1,0);a.floating_difference(28,4,0,true,false);
        if(!a.nonpositive()) {
            a.at(0x10362530);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x40,4);a.floating_load(2,30,0x210,false);
            a.store<4>(1,0x44,0);a.floating_load(0,30,0x178,false);a.floating_load(1,1,0x40,false);a.floating_subtract(1,1,3,false);
            a.floating_product_difference(29,2,1,0);
        }
        a.at(0x10362550);a.load<4>(4,31,0x2c,false);a.move(3,29);a.value(5,0x1);
        a.call(0x100cab10);a.sign8(0,3,true);
        if(a.equal()) {
            a.at(0x10362598);a.offset(0,16,0x2);a.compare_registers(22,0,true);
            if(!a.nonpositive()) {
                a.at(0x103625a4);a.load<1>(0,27,0x28,false);a.mask(0,0,0xf,false,29);a.compare(0,0x4,false);
                if(a.equal()) {
                    a.at(0x103625bc);a.load<4>(3,29,0x14,false);a.load<2>(0,31,0x1ec,true);a.load<2>(3,3,0xa,true);
                    a.compare_registers(3,0,true);
                    if(!a.at_least()) {
                        a.at(0x103625d0);a.offset(3,13,-0x2);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                        a.store<4>(1,0x38,0);a.floating_load(2,30,0x8,false);a.store<4>(1,0x3c,3);a.floating_load(0,30,0x218,false);
                        a.floating_load(1,1,0x38,false);a.floating_load(3,30,0x80,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);
                        a.floating_subtract(0,3,0,false);a.floating_product(28,28,0);
                    }
                } else {
                    a.at(0x103625b4);a.compare(0,0x3,false);
                    if(a.equal()) {
                        a.at(0x103625bc);a.load<4>(3,29,0x14,false);a.load<2>(0,31,0x1ec,true);a.load<2>(3,3,0xa,true);
                        a.compare_registers(3,0,true);
                        if(!a.at_least()) {
                            a.at(0x103625d0);a.offset(3,13,-0x2);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                            a.store<4>(1,0x38,0);a.floating_load(2,30,0x8,false);a.store<4>(1,0x3c,3);a.floating_load(0,30,0x218,false);
                            a.floating_load(1,1,0x38,false);a.floating_load(3,30,0x80,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);
                            a.floating_subtract(0,3,0,false);a.floating_product(28,28,0);
                        }
                    }
                }
            }
        } else {
            a.at(0x10362568);a.exclusive_flags(3,13,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
            a.floating_load(2,30,0x8,false);a.store<4>(1,0x38,0);a.floating_load(3,30,0x38,false);a.floating_load(1,1,0x38,false);
            a.floating_load(0,30,0x168,false);a.floating_subtract(1,1,2,false);a.floating_product_sum(0,3,1,0);a.floating_difference(28,28,0,true,false);
            a.jump(0x10362604);
        }
        a.at(0x10362604);a.load<1>(0,27,0x28,false);a.mask(0,0,0xf,false,29);a.compare(0,0x5,false);
        if(a.equal()) {
            a.at(0x10362614);a.load<4>(3,28,0x0,false);a.value(0,0x43300000);a.floating_load(0,30,0x80,false);
            a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x38,0);a.floating_subtract(0,0,29,false);a.floating_load(2,30,0x8,false);
            a.store<4>(1,0x3c,3);a.floating_load(1,1,0x38,false);a.floating_product_sum(0,28,0,29);a.floating_subtract(1,1,2,false);
            a.floating_product(1,1,0);a.call(0x10773780);a.store<4>(28,0x0,3);
        }
        a.at(0x1036264c);a.floating_load(0,30,0x80,false);a.value(0,0x43300000);a.store<4>(1,0x3c,15);
        a.floating_subtract(0,0,29,false);a.floating_load(2,30,0x2b8,false);a.store<4>(1,0x38,0);a.floating_load(1,1,0x38,false);
        a.floating_product_sum(0,28,0,29);a.floating_subtract(1,1,2,false);a.floating_product(1,1,0);a.call(0x10773780);
        a.move(15,3);
    }
    a.at(0x1036267c);a.load<4>(3,29,0x14,false);a.value(0,0x43300000);a.store<4>(1,0x38,0);
    a.load<1>(0,3,0x2d,false);a.floating_load(1,30,0x8,false);a.sign8(0,0,false);a.floating_load(2,30,0xe8,false);
    a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(1,0,1,false);
    a.call(0x10780fc0);a.load<4>(2,1,0x14,false);a.floating_load(2,30,0x220,false);a.value(4,0x43300000);
    a.floating_load(0,30,0x228,false);a.floating_difference(4,1,2,true,false);a.floating_load(3,30,0x80,false);a.load<1>(0,27,0x28,false);
    a.store<4>(1,0x44,15);a.mask(3,0,0xf,false,29);a.floating_load(2,30,0x2b8,false);a.store<4>(1,0x40,4);
    a.floating_difference(1,30,0,true,false);a.compare(3,0x6,false);a.floating_load(0,1,0x40,false);a.store<4>(1,0x50,4);
    a.floating_subtract(3,3,4,false);a.floating_subtract(0,0,2,false);a.floating_product(0,0,3);a.floating_integer(0,0);
    a.floating_store(0,1,0x48);a.load<4>(0,1,0x4c,false);a.product(0,0,0);a.store<4>(1,0x54,0);
    a.floating_load(0,1,0x50,false);a.floating_subtract(0,0,2,false);a.floating_product(0,1,0);
    if(!a.equal()) {
        a.at(0x103627bc);a.compare(3,0x5,false);
        if(a.equal()) {
            a.at(0x103627c4);a.load<2>(3,31,0x1ec,true);a.compare(3,0x1388,true);
            if(a.nonpositive()) {
                a.at(0x1036280c);a.exclusive_flags(0,3,0x80000000);a.store<4>(1,0x38,4);a.floating_load(3,30,0x8,false);
                a.store<4>(1,0x3c,0);a.floating_load(4,30,0x48,false);a.floating_load(1,1,0x38,false);a.store<4>(1,0x44,0);
                a.floating_subtract(2,1,3,false);a.store<4>(1,0x40,4);a.floating_load(1,1,0x40,false);a.floating_product(2,4,2);
                a.floating_subtract(1,1,3,false);a.floating_product(1,2,1);a.floating_product(1,4,1);a.floating_compare(0,1);
                if(!a.nonpositive()) {
                    a.at(0x1036284c);a.floating_move(0,1);
                }
            } else {
                a.at(0x103627d0);a.offset(0,3,-0x1388);a.store<4>(1,0x38,4);a.exclusive_flags(0,0,0x80000000);
                a.floating_load(3,30,0x8,false);a.store<4>(1,0x3c,0);a.floating_load(4,30,0x58,false);a.floating_load(2,1,0x38,false);
                a.floating_load(1,30,0x28,false);a.floating_subtract(2,2,3,false);a.floating_product_sum(1,4,2,1);a.floating_product(1,1,1);
                a.floating_compare(0,1);
                if(!a.nonpositive()) {
                    a.at(0x10362804);a.floating_move(0,1);a.jump(0x10362850);
                }
            }
        }
    } else {
        a.at(0x10362718);a.load<2>(3,31,0x1ec,true);a.compare(3,0x1388,true);
        if(a.nonpositive()) {
            a.at(0x10362778);a.multiply(0,3,0xa);a.store<4>(1,0x38,4);a.floating_load(2,30,0x8,false);
            a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x3c,0);a.floating_load(1,1,0x38,false);a.floating_subtract(1,1,2,false);
            a.floating_compare(0,1);
            if(!a.nonpositive()) {
                a.at(0x1036279c);a.exclusive_flags(0,3,0x80000000);a.store<4>(1,0x38,4);a.floating_load(1,30,0x150,false);
                a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,2,false);a.floating_product(0,1,0);
                a.jump(0x10362850);
            }
        } else {
            a.at(0x10362724);a.offset(0,3,-0x1388);a.store<4>(1,0x38,4);a.exclusive_flags(0,0,0x80000000);
            a.floating_load(3,30,0x8,false);a.store<4>(1,0x3c,0);a.floating_load(4,30,0x238,false);a.floating_load(1,1,0x38,false);
            a.floating_load(2,30,0x230,false);a.floating_subtract(1,1,3,false);a.floating_product_sum(1,4,1,2);a.floating_compare(0,1);
            if(!a.nonpositive()) {
                a.at(0x10362754);a.offset(3,3,-0x20000);a.store<4>(1,0x38,4);a.offset(0,3,0x7960);
                a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x3c,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,3,false);
                a.floating_sum(0,2,0);a.jump(0x10362850);
            }
        }
    }
    a.at(0x10362850);a.load<4>(3,29,0x14,false);a.load<2>(0,3,0xc,true);a.compare(0,0x1388,true);
    if(a.positive()) {
        a.at(0x1036287c);a.compare(0,0x1f40,true);
        if(a.positive()) {
            a.at(0x103628c4);a.compare(0,0x2328,true);
            if(a.at_least()) {
                a.at(0x10362914);a.floating_load(0,30,0xe0,false);a.floating_load(30,30,0x228,false);a.floating_product(31,31,0);
            } else {
                a.at(0x103628cc);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
                a.floating_load(3,30,0x8,false);a.store<4>(1,0x38,0);a.floating_load(4,30,0x250,false);a.floating_load(2,1,0x38,false);
                a.floating_load(1,30,0x258,false);a.floating_subtract(2,2,3,false);a.floating_product_difference(1,4,2,1);a.floating_product(30,31,1);
                a.floating_compare(30,0);
                if(a.at_least()) {
                    a.at(0x10362904);a.floating_move(30,0);
                } else {
                    a.at(0x10362900);a.jump(0x10362908);
                }
                a.at(0x10362908);a.floating_load(0,30,0x178,false);a.floating_product(31,31,0);a.jump(0x10362920);
            }
        } else {
            a.at(0x10362884);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
            a.floating_load(1,30,0x248,false);a.store<4>(1,0x38,0);a.floating_load(3,30,0x8,false);a.floating_product(4,1,31);
            a.floating_load(2,1,0x38,false);a.floating_load(1,30,0x240,false);a.floating_subtract(2,2,3,false);a.floating_product_sum(30,4,2,1);
            a.floating_compare(30,0);
            if(a.at_least()) {
                a.at(0x103628bc);a.floating_move(30,0);a.jump(0x10362920);
            } else {
                a.at(0x103628b8);a.jump(0x10362920);
            }
        }
    } else {
        a.at(0x10362860);a.floating_load(1,30,0x1a8,false);a.floating_product(30,1,31);a.floating_compare(30,0);
        if(a.at_least()) {
            a.at(0x10362874);a.floating_move(30,0);a.jump(0x10362920);
        } else {
            a.at(0x10362870);a.jump(0x10362920);
        }
    }
    return apply_staff_value_curve(a);
}

void apply_staff_value_curve(Call& a) {
    a.at(0x10362920);a.offset(0,22,-0x10);a.compare(0,0x18,false);
    if(!a.positive()) {
        a.at(0x1036292c);a.load<4>(3,2,-0x4df0,false);a.mask(0,0,0xfffffffc,false,2);a.indexed_load<4>(3,3,0);
        a.count_destination(3);a.count_branch();
        switch(a.r(0)>>2) {
            case 0: {
                a.at(0x10362a60);a.floating_load(0,30,0x290,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x10362a74);a.floating_move(0,30);
                } else {
                    a.at(0x10362a70);a.jump(0x10362a78);
                }
                a.at(0x10362a78);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
            case 1: {
                a.at(0x10362a80);a.floating_load(0,30,0x1c8,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x10362a94);a.floating_move(0,30);
                } else {
                    a.at(0x10362a90);a.jump(0x10362a98);
                }
                a.at(0x10362a98);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
            case 2: {
                a.at(0x10362aa0);a.floating_load(0,30,0x1c0,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x10362ab4);a.floating_move(0,30);
                } else {
                    a.at(0x10362ab0);a.jump(0x10362ab8);
                }
                a.at(0x10362ab8);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
            case 3: {
                a.at(0x10362ac0);a.floating_load(0,30,0x280,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x10362ad4);a.floating_move(0,30);
                } else {
                    a.at(0x10362ad0);a.jump(0x10362ad8);
                }
                a.at(0x10362ad8);a.floating_move(30,0);
                break;
            }
            case 4:case 5:case 6:case 7:case 8:case 9:case 10:case 11: {
                break;
            }
            case 12: {
                a.at(0x10362a40);a.floating_load(0,30,0x288,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x10362a54);a.floating_move(0,30);
                } else {
                    a.at(0x10362a50);a.jump(0x10362a58);
                }
                a.at(0x10362a58);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
            case 13: {
                a.at(0x10362a20);a.floating_load(0,30,0x280,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x10362a34);a.floating_move(0,30);
                } else {
                    a.at(0x10362a30);a.jump(0x10362a38);
                }
                a.at(0x10362a38);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
            case 14: {
                a.at(0x10362a00);a.floating_load(0,30,0x1a8,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x10362a14);a.floating_move(0,30);
                } else {
                    a.at(0x10362a10);a.jump(0x10362a18);
                }
                a.at(0x10362a18);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
            case 15: {
                a.at(0x103629e0);a.floating_load(0,30,0x1c0,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x103629f4);a.floating_move(0,30);
                } else {
                    a.at(0x103629f0);a.jump(0x103629f8);
                }
                a.at(0x103629f8);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
            case 16: {
                a.at(0x103629c0);a.floating_load(0,30,0x278,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x103629d4);a.floating_move(0,30);
                } else {
                    a.at(0x103629d0);a.jump(0x103629d8);
                }
                a.at(0x103629d8);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
            case 17: {
                a.at(0x103629a0);a.floating_load(0,30,0x128,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x103629b4);a.floating_move(0,30);
                } else {
                    a.at(0x103629b0);a.jump(0x103629b8);
                }
                a.at(0x103629b8);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
            case 18: {
                a.at(0x10362980);a.floating_load(0,30,0x270,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x10362994);a.floating_move(0,30);
                } else {
                    a.at(0x10362990);a.jump(0x10362998);
                }
                a.at(0x10362998);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
            case 19: {
                a.at(0x10362960);a.floating_load(0,30,0x268,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x10362974);a.floating_move(0,30);
                } else {
                    a.at(0x10362970);a.jump(0x10362978);
                }
                a.at(0x10362978);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
            case 20:case 21:case 22:case 23:case 24: {
                a.at(0x10362940);a.floating_load(0,30,0x260,false);a.floating_product(0,0,31);a.floating_compare(0,30);
                if(a.at_least()) {
                    a.at(0x10362954);a.floating_move(0,30);
                } else {
                    a.at(0x10362950);a.jump(0x10362958);
                }
                a.at(0x10362958);a.floating_move(30,0);a.jump(0x10362adc);
                break;
            }
        }
    }
    return apply_contribution_country_scales(a);
}

void apply_contribution_country_scales(Call& a) {
    a.at(0x10362adc);a.floating_load(0,30,0x298,false);a.floating_compare(30,0);
    if(!a.nonpositive()) {
        a.at(0x10362ae8);a.load<4>(0,28,0x0,false);a.move(4,29);a.load<4>(3,31,0x90,false);
        a.move(5,31);a.shift_right(0,0,2);a.value(6,0x2);a.add_carry(0,0);
        a.sign16(7,0,false);a.call(0x10219e40);a.sign16(3,3,false);a.value(0,0x43300000);
        a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x38,0);a.floating_load(2,30,0x8,false);a.store<4>(1,0x3c,3);
        a.floating_load(0,30,0x2a0,false);a.floating_load(1,1,0x38,false);a.floating_subtract(3,1,2,false);a.floating_compare(3,0);
        if(!a.at_least()) {
            a.at(0x10362b38);a.floating_load(1,30,0x2a8,false);a.floating_load(0,30,0x2b0,false);a.floating_load(2,30,0x158,false);
            a.floating_product_difference(0,1,3,0);a.floating_compare(2,0);
            if(a.nonpositive()) {
                a.at(0x10362b54);a.floating_move(2,0);
            } else {
                a.at(0x10362b50);a.jump(0x10362b58);
            }
            a.at(0x10362b58);a.floating_load(1,30,0x298,false);a.floating_subtract(0,30,1,false);a.floating_product_sum(30,0,2,1);
        }
    }
    a.at(0x10362b64);a.load<4>(3,31,0x30,false);a.compare(3,0x0,false);
    if(!a.equal()) {
        a.at(0x10362b70);a.load<2>(5,3,0x18,true);a.compare(5,0x10,true);
        if(!a.at_least()) {
            a.at(0x10362b7c);a.value(3,0x10620000);a.load<2>(0,31,0x1ec,true);a.offset(3,3,0x4dd3);
            a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);a.add(0,0,3);
            a.compare_registers(0,5,true);
            if(!a.nonpositive()) {
                a.at(0x10362ba0);a.move(5,0);
            }
            a.at(0x10362ba4);a.value(4,0xa0000);a.value(3,0xf0000);a.offset(0,4,0x3930);
            a.product(0,5,0);a.offset(4,3,0x4240);a.compare_registers(0,4,true);
            if(!a.nonpositive()) {
                a.at(0x10362bc0);a.move(4,0);
            }
            a.at(0x10362bc4);a.load<4>(3,29,0x14,false);a.load<2>(3,3,0xc,true);a.compare(3,0x1f40,true);
            if(!a.nonpositive()) {
                a.at(0x10362bd4);a.offset(0,3,-0x1f40);a.multiply(0,0,0x2710);a.add(4,4,0);
            }
            a.at(0x10362be0);a.exclusive_flags(3,4,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
            a.floating_load(2,30,0x8,false);a.store<4>(1,0x38,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,2,false);
            a.floating_compare(30,0);
            if(!a.nonpositive()) {
                a.at(0x10362c04);a.store<4>(1,0x3c,3);a.floating_load(0,30,0x80,false);a.store<4>(1,0x38,0);
                a.floating_compare(0,31);a.floating_load(1,1,0x38,false);a.floating_subtract(1,1,2,false);
                if(a.at_least()) {
                    a.at(0x10362c24);a.floating_move(0,31);
                } else {
                    a.at(0x10362c20);a.jump(0x10362c28);
                }
                a.at(0x10362c28);a.floating_product(0,1,0);a.value(0,0x43300000);a.store<4>(1,0x48,0);
                a.floating_load(1,30,0x8,false);a.floating_integer(0,0);a.floating_store(0,1,0x40);a.load<4>(0,1,0x44,false);
                a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x4c,0);a.floating_load(0,1,0x48,false);a.floating_subtract(30,0,1,false);
            }
        }
    }
    a.at(0x10362c54);a.load<4>(4,31,0x2c,false);a.load<4>(3,2,-0x79fc,false);a.load<4>(4,4,0x68,false);
    a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
    if(!a.equal()) {
        a.at(0x10362ca0);a.load<4>(3,2,-0x7a00,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
        if(a.equal()) {
            a.at(0x10362cb0);a.floating_load(0,30,0xf8,false);a.value(0,0x43300000);a.store<4>(1,0x40,0);
            a.floating_product(0,0,30);a.floating_load(1,30,0x8,false);a.floating_integer(0,0);a.floating_store(0,1,0x38);
            a.load<4>(0,1,0x3c,false);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x44,0);a.floating_load(0,1,0x40,false);
            a.floating_subtract(30,0,1,false);
        }
    } else {
        a.at(0x10362c6c);a.floating_load(0,30,0x1c8,false);a.floating_compare(30,0);
        if(a.at_least()) {
            a.at(0x10362c94);a.floating_load(0,30,0x168,false);a.floating_product(30,0,30);a.jump(0x10362ce0);
        } else {
            a.at(0x10362c78);a.floating_difference(0,30,0,true,false);a.floating_load(1,30,0x180,false);a.floating_load(2,30,0x1f8,false);
            a.floating_product(0,1,0);a.floating_product(0,0,30);a.floating_product_sum(30,2,30,0);a.jump(0x10362ce0);
        }
    }
    return finish_contribution_value(a);
}

void finish_contribution_value(Call& a) {
    a.at(0x10362ce0);a.sign8(0,14,true);
    if(!a.equal()) {
        a.at(0x10362ce8);a.exclusive_flags(3,21,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
        a.floating_load(1,30,0x8,false);a.store<4>(1,0x38,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,false);
        a.floating_compare(0,30);
        if(!a.at_least()) {
            a.at(0x10362d0c);a.store<4>(1,0x44,3);a.store<4>(1,0x40,0);a.floating_load(0,1,0x40,false);
            a.floating_subtract(30,0,1,false);
        }
        a.at(0x10362d1c);a.floating_integer(0,30);a.value(0,0x43300000);a.store<4>(1,0x50,0);
        a.floating_load(1,30,0x8,false);a.floating_store(0,1,0x48);a.load<4>(0,1,0x4c,false);a.exclusive_flags(0,0,0x80000000);
        a.store<4>(1,0x54,0);a.floating_load(0,1,0x50,false);a.floating_subtract(30,0,1,false);
    }
    a.at(0x10362d44);a.floating_integer(0,30);a.floating_store(0,1,0x38);a.at(0x10362d4c);
    a.load<4>(3,1,0x3c,false);
    return;
}
}
