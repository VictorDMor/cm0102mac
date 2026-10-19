#include "match_event.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_event {
using Call = SemanticGuestCall;
namespace {
void qualifier_rule_1(Call&);
void qualifier_rule_2(Call&);
void qualifier_rule_3(Call&);
void qualifier_rule_4(Call&);
void qualifier_rule_5(Call&);
void qualifier_rule_6(Call&);
void qualifier_rule_7(Call&);
void qualifier_rule_8(Call&);
void qualifier_rule_8(Call& a) {
    a.at(0x101ff5d4); a.load<2>(3,24,0x0,true);
}
void qualifier_rule_7(Call& a) {
    a.at(0x101ff5c0); a.load<2>(0,24,0x0,true); a.compare(0,8011);
    if (!a.equal()) {
        a.at(0x101ff5d4);
        qualifier_rule_8(a);
    } else {
        a.at(0x101ff5cc); a.value(3,0x217e); a.jump(0x101ff5d8);
    }
}
void qualifier_rule_6(Call& a) {
    a.at(0x101ff5a4); a.offset(0,27,-0x3); a.mask(0,0,0xff,false,0); a.compare(0,1,false);
    if (a.nonpositive()) {
        a.at(0x101ff5c0);
        qualifier_rule_7(a);
    } else {
        a.at(0x101ff5b4); a.sign8(0,27); a.compare(0,5);
        if (!a.equal()) {
            a.at(0x101ff5d4);
            qualifier_rule_8(a);
        } else {
            a.at(0x101ff5c0);
            qualifier_rule_7(a);
        }
    }
}
void qualifier_rule_5(Call& a) {
    a.at(0x101ff548); a.sign8(0,26,true);
    if (a.equal()) {
        a.at(0x101ff5a4);
        qualifier_rule_6(a);
    } else {
        a.at(0x101ff550); a.load<2>(0,24,0x0,true); a.compare(0,8466);
        if (!a.equal()) {
            a.at(0x101ff564); a.compare(0,8102);
            if (!a.equal()) {
                a.at(0x101ff574); a.compare(0,8075);
                if (!a.equal()) {
                    a.at(0x101ff584); a.compare(0,8127);
                    if (!a.equal()) {
                        a.at(0x101ff594); a.compare(0,8479);
                        if (!a.equal()) {
                            a.at(0x101ff5a4);
                            qualifier_rule_6(a);
                        } else {
                            a.at(0x101ff59c); a.value(3,0x217f); a.jump(0x101ff5d8);
                        }
                    } else {
                        a.at(0x101ff58c); a.value(3,0x218c); a.jump(0x101ff5d8);
                    }
                } else {
                    a.at(0x101ff57c); a.value(3,0x218b); a.jump(0x101ff5d8);
                }
            } else {
                a.at(0x101ff56c); a.value(3,0x2194); a.jump(0x101ff5d8);
            }
        } else {
            a.at(0x101ff55c); a.value(3,0x2196); a.jump(0x101ff5d8);
        }
    }
}
void qualifier_rule_4(Call& a) {
    a.at(0x101ff4c4); a.load(5,31,0x0); a.value(3,0x55550000); a.load(4,28,0x0); a.offset(0,3,0x5556);
    a.load<2>(3,5,0x0); a.add(4,3,4); a.multiply_high(3,0,4); a.mask(0,3,0x1,false,1); a.add(0,3,0);
    a.multiply(0,0,0x3); a.subtract(0,0,4,true);
    if (!a.equal()) {
        a.at(0x101ff548);
        qualifier_rule_5(a);
    } else {
        a.at(0x101ff4f4); a.load<2>(0,24,0x0,true); a.compare(0,8559);
        if (!a.equal()) {
            a.at(0x101ff508); a.compare(0,8174);
            if (!a.equal()) {
                a.at(0x101ff518); a.compare(0,8002);
                if (!a.equal()) {
                    a.at(0x101ff528); a.compare(0,8011);
                    if (!a.equal()) {
                        a.at(0x101ff538); a.compare(0,8008);
                        if (!a.equal()) {
                            a.at(0x101ff548);
                            qualifier_rule_5(a);
                        } else {
                            a.at(0x101ff540); a.value(3,0x2186); a.jump(0x101ff5d8);
                        }
                    } else {
                        a.at(0x101ff530); a.value(3,0x217d); a.jump(0x101ff5d8);
                    }
                } else {
                    a.at(0x101ff520); a.value(3,0x217c); a.jump(0x101ff5d8);
                }
            } else {
                a.at(0x101ff510); a.value(3,0x218d); a.jump(0x101ff5d8);
            }
        } else {
            a.at(0x101ff500); a.value(3,0x2199); a.jump(0x101ff5d8);
        }
    }
}
void qualifier_rule_3(Call& a) {
    a.at(0x101ff288); a.sign8(0,25); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x101ff318); a.compare(0,2);
        if (!a.equal()) {
            a.at(0x101ff3c4); a.compare(0,8);
            if (!a.equal()) {
                a.at(0x101ff470); a.compare(0,64);
                if (a.equal()) {
                    a.at(0x101ff548);
                    qualifier_rule_5(a);
                } else {
                    a.at(0x101ff478); a.compare(0,16);
                    if (!a.equal()) {
                        a.at(0x101ff494); a.compare(0,32);
                        if (!a.equal()) {
                            a.at(0x101ff548);
                            qualifier_rule_5(a);
                        } else {
                            a.at(0x101ff49c); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2);
                            a.compare_registers(3,0,false); a.store(31,0x0,3);
                            if (a.positive()) {
                                a.at(0x101ff4b4); a.store(31,0x0,29); a.call(0x1077b770);
                                a.mask(0,3,0xffff,false,0); a.store(28,0x0,0);
                            }
                            a.at(0x101ff4c4);
                            qualifier_rule_4(a);
                        }
                    } else {
                        a.at(0x101ff480); a.load<2>(0,24,0x0,true); a.compare(0,8008);
                        if (!a.equal()) {
                            a.at(0x101ff548);
                            qualifier_rule_5(a);
                        } else {
                            a.at(0x101ff48c); a.value(3,0x2185); a.jump(0x101ff5d8);
                        }
                    }
                }
            } else {
                a.at(0x101ff3cc); a.load<2>(0,24,0x0,true); a.compare(0,8559);
                if (!a.equal()) {
                    a.at(0x101ff3e0); a.compare(0,8528);
                    if (!a.equal()) {
                        a.at(0x101ff3f0); a.compare(0,8466);
                        if (!a.equal()) {
                            a.at(0x101ff400); a.compare(0,8166);
                            if (!a.equal()) {
                                a.at(0x101ff410); a.compare(0,8174);
                                if (!a.equal()) {
                                    a.at(0x101ff420); a.compare(0,8282);
                                    if (!a.equal()) {
                                        a.at(0x101ff430); a.compare(0,8283);
                                        if (!a.equal()) {
                                            a.at(0x101ff440); a.compare(0,8011);
                                            if (!a.equal()) {
                                                a.at(0x101ff450); a.compare(0,8001);
                                                if (!a.equal()) {
                                                    a.at(0x101ff460); a.compare(0,8010);
                                                    if (!a.equal()) {
                                                        a.at(0x101ff548);
                                                        qualifier_rule_5(a);
                                                    } else {
                                                        a.at(0x101ff468); a.value(3,0x2188); a.jump(0x101ff5d8);
                                                    }
                                                } else {
                                                    a.at(0x101ff458); a.value(3,0x2184); a.jump(0x101ff5d8);
                                                }
                                            } else {
                                                a.at(0x101ff448); a.value(3,0x2183); a.jump(0x101ff5d8);
                                            }
                                        } else {
                                            a.at(0x101ff438); a.value(3,0x218f); a.jump(0x101ff5d8);
                                        }
                                    } else {
                                        a.at(0x101ff428); a.value(3,0x218e); a.jump(0x101ff5d8);
                                    }
                                } else {
                                    a.at(0x101ff418); a.value(3,0x218d); a.jump(0x101ff5d8);
                                }
                            } else {
                                a.at(0x101ff408); a.value(3,0x2193); a.jump(0x101ff5d8);
                            }
                        } else {
                            a.at(0x101ff3f8); a.value(3,0x2195); a.jump(0x101ff5d8);
                        }
                    } else {
                        a.at(0x101ff3e8); a.value(3,0x2197); a.jump(0x101ff5d8);
                    }
                } else {
                    a.at(0x101ff3d8); a.value(3,0x2199); a.jump(0x101ff5d8);
                }
            }
        } else {
            a.at(0x101ff320); a.load<2>(0,24,0x0,true); a.compare(0,8559);
            if (!a.equal()) {
                a.at(0x101ff334); a.compare(0,8528);
                if (!a.equal()) {
                    a.at(0x101ff344); a.compare(0,8466);
                    if (!a.equal()) {
                        a.at(0x101ff354); a.compare(0,8166);
                        if (!a.equal()) {
                            a.at(0x101ff364); a.compare(0,8174);
                            if (!a.equal()) {
                                a.at(0x101ff374); a.compare(0,8008);
                                if (!a.equal()) {
                                    a.at(0x101ff384); a.compare(0,8001);
                                    if (!a.equal()) {
                                        a.at(0x101ff394); a.compare(0,8004);
                                        if (!a.equal()) {
                                            a.at(0x101ff3a4); a.compare(0,8070);
                                            if (!a.equal()) {
                                                a.at(0x101ff3b4); a.compare(0,8098);
                                                if (!a.equal()) {
                                                    a.at(0x101ff548);
                                                    qualifier_rule_5(a);
                                                } else {
                                                    a.at(0x101ff3bc); a.value(3,0x218a); a.jump(0x101ff5d8);
                                                }
                                            } else {
                                                a.at(0x101ff3ac); a.value(3,0x2189); a.jump(0x101ff5d8);
                                            }
                                        } else {
                                            a.at(0x101ff39c); a.value(3,0x2187); a.jump(0x101ff5d8);
                                        }
                                    } else {
                                        a.at(0x101ff38c); a.value(3,0x2180); a.jump(0x101ff5d8);
                                    }
                                } else {
                                    a.at(0x101ff37c); a.value(3,0x2181); a.jump(0x101ff5d8);
                                }
                            } else {
                                a.at(0x101ff36c); a.value(3,0x218d); a.jump(0x101ff5d8);
                            }
                        } else {
                            a.at(0x101ff35c); a.value(3,0x2193); a.jump(0x101ff5d8);
                        }
                    } else {
                        a.at(0x101ff34c); a.value(3,0x2195); a.jump(0x101ff5d8);
                    }
                } else {
                    a.at(0x101ff33c); a.value(3,0x2197); a.jump(0x101ff5d8);
                }
            } else {
                a.at(0x101ff32c); a.value(3,0x2199); a.jump(0x101ff5d8);
            }
        }
    } else {
        a.at(0x101ff294); a.load<2>(0,24,0x0,true); a.compare(0,8559);
        if (!a.equal()) {
            a.at(0x101ff2a8); a.compare(0,8528);
            if (!a.equal()) {
                a.at(0x101ff2b8); a.compare(0,8466);
                if (!a.equal()) {
                    a.at(0x101ff2c8); a.compare(0,8174);
                    if (!a.equal()) {
                        a.at(0x101ff2d8); a.compare(0,8009);
                        if (!a.equal()) {
                            a.at(0x101ff2e8); a.compare(0,8282);
                            if (!a.equal()) {
                                a.at(0x101ff2f8); a.compare(0,8283);
                                if (!a.equal()) {
                                    a.at(0x101ff308); a.compare(0,8313);
                                    if (!a.equal()) {
                                        a.at(0x101ff548);
                                        qualifier_rule_5(a);
                                    } else {
                                        a.at(0x101ff310); a.value(3,0x2192); a.jump(0x101ff5d8);
                                    }
                                } else {
                                    a.at(0x101ff300); a.value(3,0x2191); a.jump(0x101ff5d8);
                                }
                            } else {
                                a.at(0x101ff2f0); a.value(3,0x2190); a.jump(0x101ff5d8);
                            }
                        } else {
                            a.at(0x101ff2e0); a.value(3,0x2182); a.jump(0x101ff5d8);
                        }
                    } else {
                        a.at(0x101ff2d0); a.value(3,0x218d); a.jump(0x101ff5d8);
                    }
                } else {
                    a.at(0x101ff2c0); a.value(3,0x2195); a.jump(0x101ff5d8);
                }
            } else {
                a.at(0x101ff2b0); a.value(3,0x2197); a.jump(0x101ff5d8);
            }
        } else {
            a.at(0x101ff2a0); a.value(3,0x2199); a.jump(0x101ff5d8);
        }
    }
}
void qualifier_rule_2(Call& a) {
    a.at(0x101ff284); a.value(26,0x1);
    qualifier_rule_3(a);
}
void qualifier_rule_1(Call& a) {
    a.at(0x101ff22c); a.load(5,31,0x0); a.value(3,0x55550000); a.load(4,28,0x0); a.offset(0,3,0x5556);
    a.load<2>(3,5,0x0); a.add(4,3,4); a.multiply_high(3,0,4); a.mask(0,3,0x1,false,1); a.add(0,3,0);
    a.multiply(0,0,0x3); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x101ff264); a.load<1>(3,1,0x80); a.offset(0,3,-0x3); a.mask(0,0,0xff,false,0); a.compare(0,1,false);
        if (a.nonpositive()) {
            a.at(0x101ff284);
            qualifier_rule_2(a);
        } else {
            a.at(0x101ff278); a.sign8(0,3); a.compare(0,2);
            if (!a.equal()) {
                a.at(0x101ff288);
                qualifier_rule_3(a);
            } else {
                a.at(0x101ff284);
                qualifier_rule_2(a);
            }
        }
    } else {
        a.at(0x101ff25c); a.load<2>(3,24,0x0,true); a.jump(0x101ff5d8);
    }
}
}
void qualify_event(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101ff1d0,dependencies,observer);
    a.at(0x101ff1d0); a.link(); a.multiple(24,false,0); a.load(31,2,-0x7b84); a.move(24,4); a.load(30,2,-0x7b88);
    a.move(25,6); a.load(28,2,-0x7b90); a.value(26,0x0); a.load(29,2,-0x7b8c); a.save_link(); a.frame(96);
    a.store(1,0x80,5); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2); a.load<1>(27,1,0x81);
    a.compare_registers(3,0,false); a.store(31,0x0,3);
    if (a.positive()) {
        a.at(0x101ff21c); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(28,0x0,0);
    }
    a.at(0x101ff22c);
    qualifier_rule_1(a);
    a.at(0x101ff5d8); a.pop(96); a.multiple(24,true,0); a.finish();
}
}
