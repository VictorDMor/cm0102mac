#include "shortlist_policy_detail.hpp"

namespace cm::aot::shortlist_policy::detail {
void prepare_row_contract_gate(Call& a) {
    a.at(0x1036fbf0);a.link();a.multiple(25,false,0x0);a.combine(27,5,5,true,false);a.load<4>(29,2,-0x7b40,false);a.load<4>(30,2,-0x4ddc,false);
    a.move(28,7);a.load<4>(31,2,-0x7be0,false);a.move(26,4);a.move(25,6);a.store<4>(1,0x8,0);a.value(0,0x0);a.frame(0x70);a.store<4>(7,0x0,0);
    if(a.equal()) {
        a.at(0x1036fc3c);a.value(3,0x1);a.jump(0x1036feec);
    } else {
        a.at(0x1036fc28);a.compare(26,0x0,false);
        if(a.equal()) {
            a.at(0x1036fc3c);a.value(3,0x1);a.jump(0x1036feec);
        } else {
            a.at(0x1036fc30);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
            if(a.equal()) {
                a.at(0x1036fc44);a.move(3,27);a.move(4,25);a.value(5,0x0);a.value(6,0x0);a.call(0x10380f20);a.sign8(0,3,true);
                if(!a.equal()) {
                    a.at(0x1036fcb8);a.load<4>(0,27,0x0,false);a.load<4>(4,31,0x0,false);a.multiply(0,0,0x70);a.load<4>(3,29,0x0,false);a.add(4,4,0);
                    a.call(0x103faff0);a.sign8(0,3,true);
                    if(!a.equal()) {
                        a.at(0x1036fce0);a.load<4>(0,27,0x0,false);a.move(5,25);a.load<4>(4,31,0x0,false);a.value(6,0x1);a.multiply(0,0,0x70);
                        a.load<4>(3,29,0x0,false);a.add(4,4,0);a.call(0x103fac70);a.store<4>(28,0x0,3);a.load<4>(0,27,0x0,false);
                        a.load<4>(3,31,0x0,false);a.multiply(0,0,0x70);a.load<4>(4,28,0x0,false);a.add(3,3,0);a.load<4>(0,3,0x8,false);
                        a.compare_registers(4,0,true);
                        if(a.nonpositive()) {
                            a.at(0x1036fd2c);a.move(3,27);a.call(0x10380710);a.sign8(0,3,false);a.compare(0,0x5,true);
                            if(a.at_least()) {
                                a.at(0x1036fd58);a.compare(0,0x9,true);
                                if(a.at_least()) {
                                    a.at(0x1036fd6c);a.compare(0,0xf,true);
                                    if(a.at_least()) {
                                        a.at(0x1036fee8);a.value(3,0x0);
                                    } else {
                                        a.at(0x1036fd74);a.jump(0x1036fee0);a.at(0x1036fee0);a.value(3,0x1);a.jump(0x1036feec);
                                    }
                                } else {
                                    a.at(0x1036fd60);a.compare(0,0x7,true);
                                    if(a.at_least()) {
                                        a.at(0x1036fe88);a.load<4>(0,26,0x4,false);a.value(3,0x43300000);a.load<4>(4,26,0xc,false);
                                        a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x40,3);a.exclusive_flags(4,4,0x80000000);
                                        a.floating_load(3,30,0x8,false);a.store<4>(1,0x44,0);a.floating_load(1,30,0x168,false);
                                        a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,4);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);
                                        a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);
                                        a.floating_compare(1,0);
                                        if(a.at_least()) {
                                            a.at(0x1036fed8);a.value(3,0x0);a.jump(0x1036feec);
                                        } else {
                                            a.at(0x1036fed0);a.value(3,0x1);a.jump(0x1036feec);
                                        }
                                    } else {
                                        a.at(0x1036fd68);a.jump(0x1036fe30);a.at(0x1036fe30);a.load<4>(0,26,0x4,false);a.value(3,0x43300000);
                                        a.load<4>(4,26,0xc,false);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x40,3);
                                        a.exclusive_flags(4,4,0x80000000);a.floating_load(3,30,0x8,false);a.store<4>(1,0x44,0);
                                        a.floating_load(1,30,0xf8,false);a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,4);
                                        a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);
                                        a.floating_subtract(1,2,3,false);a.floating_compare(1,0);
                                        if(a.at_least()) {
                                            a.at(0x1036fe80);a.value(3,0x0);a.jump(0x1036feec);
                                        } else {
                                            a.at(0x1036fe78);a.value(3,0x1);a.jump(0x1036feec);
                                        }
                                    }
                                }
                            } else {
                                a.at(0x1036fd40);a.compare(0,0x0,true);
                                if(a.equal()) {
                                    a.at(0x1036fd78);a.value(3,0x0);a.jump(0x1036feec);
                                } else {
                                    a.at(0x1036fd48);
                                    if(a.less()) {
                                        a.at(0x1036fee8);a.value(3,0x0);
                                    } else {
                                        a.at(0x1036fd4c);a.compare(0,0x3,true);
                                        if(a.at_least()) {
                                            a.at(0x1036fdd8);a.load<4>(0,26,0x4,false);a.value(3,0x43300000);a.load<4>(4,26,0xc,false);
                                            a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x40,3);a.exclusive_flags(4,4,0x80000000);
                                            a.floating_load(3,30,0x8,false);a.store<4>(1,0x44,0);a.floating_load(1,30,0x1b8,false);
                                            a.floating_load(0,1,0x40,false);a.store<4>(1,0x3c,4);a.floating_subtract(0,0,3,false);a.store<4>(1,0x38,3);
                                            a.floating_load(2,1,0x38,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);
                                            a.floating_compare(1,0);
                                            if(a.at_least()) {
                                                a.at(0x1036fe28);a.value(3,0x0);a.jump(0x1036feec);
                                            } else {
                                                a.at(0x1036fe20);a.value(3,0x1);a.jump(0x1036feec);
                                            }
                                        } else {
                                            a.at(0x1036fd54);a.jump(0x1036fd80);a.at(0x1036fd80);a.load<4>(0,26,0x4,false);a.value(3,0x43300000);
                                            a.load<4>(4,26,0xc,false);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x38,3);
                                            a.exclusive_flags(4,4,0x80000000);a.floating_load(3,30,0x8,false);a.store<4>(1,0x3c,0);
                                            a.floating_load(1,30,0x68,false);a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,4);
                                            a.floating_subtract(0,0,3,false);a.store<4>(1,0x40,3);a.floating_load(2,1,0x40,false);
                                            a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_compare(1,0);
                                            if(a.at_least()) {
                                                a.at(0x1036fdd0);a.value(3,0x0);a.jump(0x1036feec);
                                            } else {
                                                a.at(0x1036fdc8);a.value(3,0x1);a.jump(0x1036feec);
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            a.at(0x1036fd24);a.value(3,0x0);a.jump(0x1036feec);
                        }
                    } else {
                        a.at(0x1036fcd8);a.value(3,0x0);a.jump(0x1036feec);
                    }
                } else {
                    a.at(0x1036fc60);a.load<4>(0,27,0x0,false);a.move(5,25);a.load<4>(4,31,0x0,false);a.value(6,0x0);a.multiply(0,0,0x70);
                    a.load<4>(3,29,0x0,false);a.add(4,4,0);a.call(0x103fac70);a.store<4>(28,0x0,3);a.load<4>(0,27,0x0,false);a.load<4>(3,31,0x0,false);
                    a.multiply(0,0,0x70);a.load<4>(5,28,0x0,false);a.add(3,3,0);a.load<4>(4,3,0x8,false);a.combine(0,4,5,false,true);a.shift_right(3,0,1);
                    a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,0x1,false,1);a.sign8(3,0,false);a.jump(0x1036feec);
                }
            } else {
                a.at(0x1036fc3c);a.value(3,0x1);a.jump(0x1036feec);
            }
        }
    }
}
}
namespace cm::aot::shortlist_policy {
void evaluate_row_contract_gate(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,evaluate_row_contract_gate_entry,d,o);detail::prepare_row_contract_gate(a);
    a.at(0x1036feec);
    a.load<4>(0,1,0x78,false);
    a.offset(1,1,0x70);
    a.restore_link();
    a.multiple(25,true,0x0);
    a.finish();
}
}
