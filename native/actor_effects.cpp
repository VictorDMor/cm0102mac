#include "actor_effects.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_effects {
using Call = SemanticGuestCall;
namespace {
void balance_phase_1(Call&,State&);
void balance_phase_2(Call&,State&);
void balance_phase_3(Call&,State&);
void balance_phase_4(Call&,State&);
void balance_phase_5(Call&,State&);
void balance_phase_6(Call&,State&);
void balance_phase_7(Call&,State&);
void balance_phase_8(Call&,State&);
void balance_phase_9(Call&,State&);
void balance_phase_10(Call&,State&);
void balance_phase_11(Call&,State&);
void balance_phase_12(Call&,State&);
void balance_phase_13(Call&,State&);
void balance_phase_14(Call&,State&);
void balance_phase_15(Call&,State&);
void balance_phase_16(Call&,State&);
void balance_phase_17(Call&,State&);
void balance_phase_18(Call&,State&);
void balance_phase_19(Call&,State&);
void balance_phase_20(Call&,State&);
void balance_phase_21(Call&,State&);
void balance_phase_22(Call&,State&);
void balance_phase_23(Call&,State&);
void balance_phase_24(Call&,State&);
void balance_phase_25(Call&,State&);
void balance_phase_26(Call&,State&);
void balance_phase_27(Call&,State&);
void balance_phase_28(Call&,State&);
void balance_phase_29(Call&,State&);
void balance_phase_30(Call&,State&);
void balance_phase_31(Call&,State&);
void balance_phase_32(Call&,State&);
void balance_phase_33(Call&,State&);
void balance_phase_34(Call&,State&);
void balance_phase_35(Call&,State&);
void balance_phase_36(Call&,State&);
void balance_phase_37(Call&,State&);
void balance_phase_38(Call&,State&);
void balance_phase_39(Call&,State&);
void balance_phase_40(Call&,State&);
void balance_phase_41(Call&,State&);
void balance_phase_42(Call&,State&);
void balance_phase_43(Call&,State&);
void balance_phase_44(Call&,State&);
void balance_phase_45(Call&,State&);
void balance_phase_46(Call&,State&);
void balance_phase_47(Call&,State&);
void balance_phase_48(Call&,State&);
void balance_phase_49(Call&,State&);
void balance_phase_50(Call&,State&);
void balance_phase_51(Call&,State&);
void balance_phase_52(Call&,State&);
void balance_phase_53(Call&,State&);
void balance_phase_54(Call&,State&);
void balance_phase_55(Call&,State&);
void balance_phase_56(Call&,State&);
void balance_phase_57(Call&,State&);
void balance_phase_58(Call&,State&);
void balance_phase_59(Call&,State&);
void balance_phase_60(Call&,State&);
void balance_phase_61(Call&,State&);
void balance_phase_62(Call&,State&);
void balance_phase_63(Call&,State&);
void balance_phase_64(Call&,State&);
void balance_phase_65(Call&,State&);
void balance_phase_66(Call&,State&);
void balance_phase_67(Call&,State&);
void balance_phase_68(Call&,State&);
void balance_phase_69(Call&,State&);
void balance_phase_69(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10239364); a.load<1>(0,31,0xc); a.compare(0,0,false);
    if (a.equal()) {
    } else {
        a.at(0x10239370); a.load<1>(0,31,0xc); a.load<2>(4,31,0x36,true); a.multiply(3,0,0x1f4);
        a.offset(0,3,0x1a5e); a.compare_registers(4,0);
        if (a.at_least()) {
        } else {
            a.at(0x10239388); a.load<1>(0,31,0xc); a.load<2>(3,31,0x36,true); a.multiply(0,0,0x5); a.sign16(0,0);
            a.add(0,3,0); a.store<2>(31,0x36,0);
        }
    }
}
void balance_phase_68(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10239348); a.load<2>(0,31,0x36,true); a.compare(0,7750);
    if (a.nonpositive()) {
    } else {
        a.at(0x10239354); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x5); a.store<2>(31,0x36,0); a.jump(0x102393a0);
    }
}
void balance_phase_67(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102392f0); a.load<1>(0,31,0xc); a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x10239364);
        balance_phase_69(a,state);
    } else {
        a.at(0x102392fc); a.load<1>(0,31,0x10); a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x10239364);
            balance_phase_69(a,state);
        } else {
            a.at(0x10239308); a.load<1>(0,31,0xb); a.compare(0,0,false);
            if (!a.equal()) {
                a.at(0x10239364);
                balance_phase_69(a,state);
            } else {
                a.at(0x10239314); a.load<1>(0,31,0x3); a.compare(0,0,false);
                if (!a.equal()) {
                    a.at(0x10239348);
                    balance_phase_68(a,state);
                } else {
                    a.at(0x10239320); a.load<1>(0,31,0x9); a.compare(0,0,false);
                    if (!a.equal()) {
                        a.at(0x10239348);
                        balance_phase_68(a,state);
                    } else {
                        a.at(0x1023932c); a.load<2>(0,31,0x36,true); a.compare(0,7250);
                        if (a.nonpositive()) {
                        } else {
                            a.at(0x10239338); a.load<2>(3,31,0x36,true); a.offset(0,3,-0xa); a.store<2>(31,0x36,0);
                            a.jump(0x102393a0);
                        }
                    }
                }
            }
        }
    }
}
void balance_phase_66(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102392b0); a.load(4,30,0x0); a.value(3,0x1b4f0000); a.load(0,29,0x0); a.offset(3,3,-0x7e4b);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x4); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x96); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x102392e4); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x1); a.store<2>(31,0x36,0);
    }
    a.at(0x102392f0);
    balance_phase_67(a,state);
}
void balance_phase_65(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10239238); a.load<1>(0,31,0xc); a.compare(0,0,false);
    if (a.equal()) {
    } else {
        a.at(0x10239244); a.load<1>(0,31,0xc); a.load<2>(4,31,0x36,true); a.multiply(3,0,0x1f4);
        a.offset(0,3,0x1a5e); a.compare_registers(4,0);
        if (a.at_least()) {
        } else {
            a.at(0x1023925c); a.load<1>(0,31,0xc); a.load<2>(3,31,0x36,true); a.multiply(0,0,0x5); a.sign16(0,0);
            a.add(0,3,0); a.store<2>(31,0x36,0); a.jump(0x102393a0);
        }
    }
}
void balance_phase_64(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1023921c); a.load<2>(0,31,0x36,true); a.compare(0,7250);
    if (a.nonpositive()) {
    } else {
        a.at(0x10239228); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x5); a.store<2>(31,0x36,0); a.jump(0x102393a0);
    }
}
void balance_phase_63(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102391d0); a.load<1>(0,31,0xc); a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x10239238);
        balance_phase_65(a,state);
    } else {
        a.at(0x102391dc); a.load<1>(0,31,0x10); a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x10239238);
            balance_phase_65(a,state);
        } else {
            a.at(0x102391e8); a.load<1>(0,31,0x3); a.compare(0,0,false);
            if (!a.equal()) {
                a.at(0x1023921c);
                balance_phase_64(a,state);
            } else {
                a.at(0x102391f4); a.load<1>(0,31,0xb); a.compare(0,0,false);
                if (!a.equal()) {
                    a.at(0x1023921c);
                    balance_phase_64(a,state);
                } else {
                    a.at(0x10239200); a.load<2>(0,31,0x36,true); a.compare(0,7250);
                    if (a.nonpositive()) {
                    } else {
                        a.at(0x1023920c); a.load<2>(3,31,0x36,true); a.offset(0,3,-0xa); a.store<2>(31,0x36,0);
                        a.jump(0x102393a0);
                    }
                }
            }
        }
    }
}
void balance_phase_62(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10239190); a.load(4,30,0x0); a.value(3,0x10620000); a.load(0,29,0x0); a.offset(3,3,0x4dd3);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x5); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x1f4); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x102391c4); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x1); a.store<2>(31,0x36,0);
    }
    a.at(0x102391d0);
    balance_phase_63(a,state);
}
void balance_phase_61(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1023913c); a.load<2>(0,31,0x36,true); a.compare(0,7250);
    if (a.nonpositive()) {
    } else {
        a.at(0x10239148); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x5); a.store<2>(31,0x36,0); a.jump(0x102393a0);
    }
}
void balance_phase_60(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102390f0); a.load<1>(0,31,0xc); a.compare(0,0,false);
    if (!a.equal()) {
    } else {
        a.at(0x102390fc); a.load<1>(0,31,0x10); a.compare(0,0,false);
        if (!a.equal()) {
        } else {
            a.at(0x10239108); a.load<1>(0,31,0x3); a.compare(0,0,false);
            if (!a.equal()) {
                a.at(0x1023913c);
                balance_phase_61(a,state);
            } else {
                a.at(0x10239114); a.load<1>(0,31,0xb); a.compare(0,0,false);
                if (!a.equal()) {
                    a.at(0x1023913c);
                    balance_phase_61(a,state);
                } else {
                    a.at(0x10239120); a.load<2>(0,31,0x36,true); a.compare(0,7250);
                    if (a.nonpositive()) {
                    } else {
                        a.at(0x1023912c); a.load<2>(3,31,0x36,true); a.offset(0,3,-0xa); a.store<2>(31,0x36,0);
                        a.jump(0x102393a0);
                    }
                }
            }
        }
    }
}
void balance_phase_59(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102390b0); a.load(4,30,0x0); a.value(3,0x10620000); a.load(0,29,0x0); a.offset(3,3,0x4dd3);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x6); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x3e8); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x102390e4); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x1); a.store<2>(31,0x36,0);
    }
    a.at(0x102390f0);
    balance_phase_60(a,state);
}
void balance_phase_58(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10239024); a.load<2>(0,31,0x36,true); a.compare(0,5750);
    if (a.at_least()) {
        a.at(0x10239040); a.load<2>(0,31,0x36,true); a.compare(0,6750);
        if (a.at_least()) {
        } else {
            a.at(0x1023904c); a.load<2>(3,31,0x36,true); a.offset(0,3,0x1); a.store<2>(31,0x36,0);
            a.jump(0x102393a0);
        }
    } else {
        a.at(0x10239030); a.load<2>(3,31,0x36,true); a.offset(0,3,0x5); a.store<2>(31,0x36,0); a.jump(0x102393a0);
    }
}
void balance_phase_57(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238fec); a.sign8(0,0,true);
    if (!a.equal()) {
    } else {
        a.at(0x10238ff4); a.load<1>(0,31,0x6); a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x10239024);
            balance_phase_58(a,state);
        } else {
            a.at(0x10239000); a.load<1>(0,31,0x9); a.compare(0,0,false);
            if (!a.equal()) {
                a.at(0x10239024);
                balance_phase_58(a,state);
            } else {
                a.at(0x1023900c); a.load<1>(0,31,0x3); a.compare(0,0,false);
                if (!a.equal()) {
                    a.at(0x10239024);
                    balance_phase_58(a,state);
                } else {
                    a.at(0x10239018); a.load<1>(0,31,0x10); a.compare(0,0,false);
                    if (a.equal()) {
                        a.at(0x1023905c); a.load<2>(0,31,0x36,true); a.compare(0,6250);
                        if (a.at_least()) {
                        } else {
                            a.at(0x10239068); a.load<2>(3,31,0x36,true); a.offset(0,3,0x1); a.store<2>(31,0x36,0);
                            a.jump(0x102393a0);
                        }
                    } else {
                        a.at(0x10239024);
                        balance_phase_58(a,state);
                    }
                }
            }
        }
    }
}
void balance_phase_56(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238fbc); a.load<1>(0,31,0x27); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10238fdc); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1f8d); a.sign8(0,0);
    } else {
        a.at(0x10238fc8); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1fc3); a.sign8(0,0);
        a.jump(0x10238fec);
    }
    a.at(0x10238fec);
    balance_phase_57(a,state);
}
void balance_phase_55(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238f80); a.load<1>(0,31,0x6); a.compare(0,0,false);
    if (a.equal()) {
        a.at(0x10238f8c); a.load<1>(0,31,0x9); a.compare(0,0,false);
        if (!a.equal()) {
        } else {
            a.at(0x10238f98); a.load<1>(0,31,0x3); a.compare(0,0,false);
            if (!a.equal()) {
            } else {
                a.at(0x10238fa4); a.load<2>(0,31,0x36,true); a.compare(0,7500);
                if (a.nonpositive()) {
                } else {
                    a.at(0x10238fb0); a.load<2>(3,31,0x36,true); a.offset(0,3,-0xa); a.store<2>(31,0x36,0);
                }
            }
        }
    }
    a.at(0x10238fbc);
    balance_phase_56(a,state);
}
void balance_phase_54(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238f40); a.load(4,30,0x0); a.value(3,0x10620000); a.load(0,29,0x0); a.offset(3,3,0x4dd3);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x5); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x1f4); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x10238f74); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x1); a.store<2>(31,0x36,0);
    }
    a.at(0x10238f80);
    balance_phase_55(a,state);
}
void balance_phase_53(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238ef0); a.sign8(3,0); a.load<2>(0,31,0x36,true); a.multiply(3,3,0x5); a.subtract(0,3,0);
    a.store<2>(31,0x36,0); a.jump(0x102393a0);
}
void balance_phase_52(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238eac); a.load<2>(0,31,0x36,true); a.sign8(3,3); a.subtract(0,3,0); a.store<2>(31,0x36,0);
    a.jump(0x102393a0);
}
void balance_phase_51(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238e7c); a.load<1>(0,31,0x27); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10238e9c); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1f8d); a.sign8(3,0);
    } else {
        a.at(0x10238e88); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1fc3); a.sign8(3,0);
        a.jump(0x10238eac);
    }
    a.at(0x10238eac);
    balance_phase_52(a,state);
}
void balance_phase_50(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238e2c); a.load<2>(0,31,0x36,true); a.sign8(3,3); a.subtract(0,3,0); a.store<2>(31,0x36,0);
    a.jump(0x102393a0);
}
void balance_phase_49(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238dec); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x1); a.store<2>(31,0x36,0); a.jump(0x102393a0);
}
void balance_phase_48(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238d8c); a.load<2>(3,31,0x36,true); a.sign16(0,0); a.compare_registers(3,0);
    if (a.nonpositive()) {
    } else {
        a.at(0x10238d9c); a.move(3,31); a.call(0x10289560); a.sign16(0,3); a.mask(0,0,0x880,true);
        if (a.equal()) {
            a.at(0x10238e40); a.load<1>(0,31,0x6); a.compare(0,0,false);
            if (!a.equal()) {
                a.at(0x10238e7c);
                balance_phase_51(a,state);
            } else {
                a.at(0x10238e4c); a.load<1>(0,31,0x9); a.compare(0,0,false);
                if (!a.equal()) {
                    a.at(0x10238e7c);
                    balance_phase_51(a,state);
                } else {
                    a.at(0x10238e58); a.load<1>(0,31,0x3); a.compare(0,0,false);
                    if (!a.equal()) {
                        a.at(0x10238e7c);
                        balance_phase_51(a,state);
                    } else {
                        a.at(0x10238e64); a.load<1>(0,31,0x10); a.compare(0,0,false);
                        if (!a.equal()) {
                            a.at(0x10238e7c);
                            balance_phase_51(a,state);
                        } else {
                            a.at(0x10238e70); a.load<1>(0,31,0xc); a.compare(0,0,false);
                            if (a.equal()) {
                                a.at(0x10238ec0); a.load<1>(0,31,0x27); a.sign8(0,0,true);
                                if (a.equal()) {
                                    a.at(0x10238ee0); a.load(3,31,0x1a4); a.offset(3,3,0x10000);
                                    a.load<1>(0,3,-0x1f8d); a.sign8(0,0);
                                } else {
                                    a.at(0x10238ecc); a.load(3,31,0x1a4); a.offset(3,3,0x10000);
                                    a.load<1>(0,3,-0x1fc3); a.sign8(0,0); a.jump(0x10238ef0);
                                }
                                a.at(0x10238ef0);
                                balance_phase_53(a,state);
                            } else {
                                a.at(0x10238e7c);
                                balance_phase_51(a,state);
                            }
                        }
                    }
                }
            }
        } else {
            a.at(0x10238db0); a.load<1>(0,31,0x6); a.compare(0,0,false);
            if (!a.equal()) {
                a.at(0x10238dec);
                balance_phase_49(a,state);
            } else {
                a.at(0x10238dbc); a.load<1>(0,31,0x9); a.compare(0,0,false);
                if (!a.equal()) {
                    a.at(0x10238dec);
                    balance_phase_49(a,state);
                } else {
                    a.at(0x10238dc8); a.load<1>(0,31,0x3); a.compare(0,0,false);
                    if (!a.equal()) {
                        a.at(0x10238dec);
                        balance_phase_49(a,state);
                    } else {
                        a.at(0x10238dd4); a.load<1>(0,31,0x10); a.compare(0,0,false);
                        if (!a.equal()) {
                            a.at(0x10238dec);
                            balance_phase_49(a,state);
                        } else {
                            a.at(0x10238de0); a.load<1>(0,31,0xc); a.compare(0,0,false);
                            if (a.equal()) {
                                a.at(0x10238dfc); a.load<1>(0,31,0x27); a.sign8(0,0,true);
                                if (a.equal()) {
                                    a.at(0x10238e1c); a.load(3,31,0x1a4); a.offset(3,3,0x10000);
                                    a.load<1>(0,3,-0x1f8d); a.sign8(3,0);
                                } else {
                                    a.at(0x10238e08); a.load(3,31,0x1a4); a.offset(3,3,0x10000);
                                    a.load<1>(0,3,-0x1fc3); a.sign8(3,0); a.jump(0x10238e2c);
                                }
                                a.at(0x10238e2c);
                                balance_phase_50(a,state);
                            } else {
                                a.at(0x10238dec);
                                balance_phase_49(a,state);
                            }
                        }
                    }
                }
            }
        }
    }
}
void balance_phase_47(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238d6c); a.sign8(0,0); a.multiply(0,0,0xfa); a.sign16(0,0); a.subtract_from(0,0,0x1e46);
    a.sign16(0,0); a.compare(0,6750);
    if (a.less()) {
        a.at(0x10238d88); a.value(0,0x1a5e);
    }
    a.at(0x10238d8c);
    balance_phase_48(a,state);
}
void balance_phase_46(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238d04); a.load<2>(0,31,0x36,true); a.compare(0,6250);
    if (a.at_least()) {
    } else {
        a.at(0x10238d10); a.load<2>(3,31,0x36,true); a.offset(0,3,0x1); a.store<2>(31,0x36,0); a.jump(0x102393a0);
    }
}
void balance_phase_45(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238cbc); a.sign8(0,0); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x10238d3c); a.load<1>(0,31,0x27); a.sign8(0,0,true);
        if (a.equal()) {
            a.at(0x10238d5c); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1f8d); a.sign8(0,0);
        } else {
            a.at(0x10238d48); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1fc3); a.sign8(0,0);
            a.jump(0x10238d6c);
        }
        a.at(0x10238d6c);
        balance_phase_47(a,state);
    } else {
        a.at(0x10238cc8); a.load<1>(0,31,0x6); a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x10238d04);
            balance_phase_46(a,state);
        } else {
            a.at(0x10238cd4); a.load<1>(0,31,0x9); a.compare(0,0,false);
            if (!a.equal()) {
                a.at(0x10238d04);
                balance_phase_46(a,state);
            } else {
                a.at(0x10238ce0); a.load<1>(0,31,0x3); a.compare(0,0,false);
                if (!a.equal()) {
                    a.at(0x10238d04);
                    balance_phase_46(a,state);
                } else {
                    a.at(0x10238cec); a.load<1>(0,31,0x10); a.compare(0,0,false);
                    if (!a.equal()) {
                        a.at(0x10238d04);
                        balance_phase_46(a,state);
                    } else {
                        a.at(0x10238cf8); a.load<1>(0,31,0xc); a.compare(0,0,false);
                        if (a.equal()) {
                            a.at(0x10238d20); a.load<2>(0,31,0x36,true); a.compare(0,7250);
                            if (a.nonpositive()) {
                            } else {
                                a.at(0x10238d2c); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x1);
                                a.store<2>(31,0x36,0); a.jump(0x102393a0);
                            }
                        } else {
                            a.at(0x10238d04);
                            balance_phase_46(a,state);
                        }
                    }
                }
            }
        }
    }
}
void balance_phase_44(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238b90); a.load<2>(0,31,0x36,true); a.compare(0,5750);
    if (a.at_least()) {
        a.at(0x10238bac); a.load<2>(0,31,0x36,true); a.compare(0,6750);
        if (a.at_least()) {
        } else {
            a.at(0x10238bb8); a.load<2>(3,31,0x36,true); a.offset(0,3,0x1); a.store<2>(31,0x36,0);
            a.jump(0x102393a0);
        }
    } else {
        a.at(0x10238b9c); a.load<2>(3,31,0x36,true); a.offset(0,3,0x5); a.store<2>(31,0x36,0); a.jump(0x102393a0);
    }
}
void balance_phase_43(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238b4c); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10238c8c); a.load<1>(0,31,0x27); a.sign8(0,0,true);
        if (a.equal()) {
            a.at(0x10238cac); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1f8d); a.sign8(0,0);
        } else {
            a.at(0x10238c98); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1fc3); a.sign8(0,0);
            a.jump(0x10238cbc);
        }
        a.at(0x10238cbc);
        balance_phase_45(a,state);
    } else {
        a.at(0x10238b54); a.load<1>(0,31,0x6); a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x10238b90);
            balance_phase_44(a,state);
        } else {
            a.at(0x10238b60); a.load<1>(0,31,0x9); a.compare(0,0,false);
            if (!a.equal()) {
                a.at(0x10238b90);
                balance_phase_44(a,state);
            } else {
                a.at(0x10238b6c); a.load<1>(0,31,0x3); a.compare(0,0,false);
                if (!a.equal()) {
                    a.at(0x10238b90);
                    balance_phase_44(a,state);
                } else {
                    a.at(0x10238b78); a.load<1>(0,31,0x10); a.compare(0,0,false);
                    if (!a.equal()) {
                        a.at(0x10238b90);
                        balance_phase_44(a,state);
                    } else {
                        a.at(0x10238b84); a.load<1>(0,31,0xc); a.compare(0,0,false);
                        if (a.equal()) {
                            a.at(0x10238bc8); a.load<1>(0,31,0x2); a.compare(0,20,false);
                            if (a.nonpositive()) {
                                a.at(0x10238c70); a.load<2>(0,31,0x36,true); a.compare(0,5750);
                                if (a.at_least()) {
                                } else {
                                    a.at(0x10238c7c); a.load<2>(3,31,0x36,true); a.offset(0,3,0x1);
                                    a.store<2>(31,0x36,0); a.jump(0x102393a0);
                                }
                            } else {
                                a.at(0x10238bd4); a.load<1>(5,31,0x1); a.value(4,0x43300000); a.load<1>(3,31,0x2);
                                a.value(0,0x43300000); a.store(1,0x3c,5); a.floating_load(1,28,240);
                                a.store(1,0x38,4); a.floating_load(2,28,240); a.floating_load(0,1,56);
                                a.store(1,0x44,3); a.floating_subtract(3,0,1,true); a.floating_load(0,28,48);
                                a.store(1,0x40,0); a.floating_load(1,1,64); a.floating_subtract(1,1,2,true);
                                a.floating_difference(1,3,1,true,true); a.floating_compare(1,0);
                                if (a.nonpositive()) {
                                    a.at(0x10238c54); a.load<2>(0,31,0x36,true); a.compare(0,5750);
                                    if (a.at_least()) {
                                    } else {
                                        a.at(0x10238c60); a.load<2>(3,31,0x36,true); a.offset(0,3,0x1);
                                        a.store<2>(31,0x36,0); a.jump(0x102393a0);
                                    }
                                } else {
                                    a.at(0x10238c1c); a.load<2>(0,31,0x36,true); a.compare(0,5750);
                                    if (a.at_least()) {
                                        a.at(0x10238c38); a.load<2>(0,31,0x36,true); a.compare(0,6750);
                                        if (a.at_least()) {
                                        } else {
                                            a.at(0x10238c44); a.load<2>(3,31,0x36,true); a.offset(0,3,0x1);
                                            a.store<2>(31,0x36,0); a.jump(0x102393a0);
                                        }
                                    } else {
                                        a.at(0x10238c28); a.load<2>(3,31,0x36,true); a.offset(0,3,0x5);
                                        a.store<2>(31,0x36,0); a.jump(0x102393a0);
                                    }
                                }
                            }
                        } else {
                            a.at(0x10238b90);
                            balance_phase_44(a,state);
                        }
                    }
                }
            }
        }
    }
}
void balance_phase_42(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238b1c); a.load<1>(0,31,0x27); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10238b3c); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1f8d); a.sign8(0,0);
    } else {
        a.at(0x10238b28); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1fc3); a.sign8(0,0);
        a.jump(0x10238b4c);
    }
    a.at(0x10238b4c);
    balance_phase_43(a,state);
}
void balance_phase_41(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238ad4); a.load<1>(0,31,0x6); a.compare(0,0,false);
    if (a.equal()) {
        a.at(0x10238ae0); a.load<1>(0,31,0x9); a.compare(0,0,false);
        if (!a.equal()) {
        } else {
            a.at(0x10238aec); a.load<1>(0,31,0x3); a.compare(0,0,false);
            if (!a.equal()) {
            } else {
                a.at(0x10238af8); a.load<1>(0,31,0x10); a.compare(0,0,false);
                if (!a.equal()) {
                } else {
                    a.at(0x10238b04); a.load<2>(0,31,0x36,true); a.compare(0,7500);
                    if (a.nonpositive()) {
                    } else {
                        a.at(0x10238b10); a.load<2>(3,31,0x36,true); a.offset(0,3,-0xa); a.store<2>(31,0x36,0);
                    }
                }
            }
        }
    }
    a.at(0x10238b1c);
    balance_phase_42(a,state);
}
void balance_phase_40(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238a90); a.floating_load(0,28,712); a.floating_compare(31,0);
    if (a.at_least()) {
        a.at(0x10238ab4); a.floating_load(0,28,720); a.floating_compare(31,0);
        if (a.nonpositive()) {
        } else {
            a.at(0x10238ac0); a.load<2>(3,31,0x36,true); a.sign16(0,4); a.compare_registers(3,0);
            if (a.at_least()) {
            } else {
                a.at(0x10238ad0); a.store<2>(31,0x36,4);
            }
        }
    } else {
        a.at(0x10238a9c); a.load<2>(3,31,0x36,true); a.sign16(0,4); a.compare_registers(3,0);
        if (a.nonpositive()) {
        } else {
            a.at(0x10238aac); a.store<2>(31,0x36,4); a.jump(0x10238ad4);
        }
    }
    a.at(0x10238ad4);
    balance_phase_41(a,state);
}
void balance_phase_39(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238a70); a.load<1>(0,31,0x10); a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x10238a7c); a.load<1>(0,31,0x10); a.multiply(0,0,0xfa); a.sign16(0,0); a.add(0,4,0); a.sign16(4,0);
    }
    a.at(0x10238a90);
    balance_phase_40(a,state);
}
void balance_phase_38(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238a40); a.sign8(3,0); a.load<1>(0,31,0xc); a.multiply(3,3,0xfa); a.compare(0,0,false);
    a.subtract_from(0,3,0x1c52); a.sign16(4,0);
    if (!a.equal()) {
        a.at(0x10238a5c); a.load<1>(0,31,0xc); a.multiply(0,0,0x1f4); a.sign16(0,0); a.add(0,4,0); a.sign16(4,0);
    }
    a.at(0x10238a70);
    balance_phase_39(a,state);
}
void balance_phase_37(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238a10); a.load<1>(0,31,0x27); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10238a30); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1f8d); a.sign8(0,0);
    } else {
        a.at(0x10238a1c); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1fc3); a.sign8(0,0);
        a.jump(0x10238a40);
    }
    a.at(0x10238a40);
    balance_phase_38(a,state);
}
void balance_phase_36(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238974); a.load<1>(0,31,0x7); a.compare(0,4,false);
    if (a.positive()) {
        a.at(0x10238980); a.load<1>(5,31,0x8); a.value(4,0x43300000); a.load<1>(3,31,0x7); a.value(0,0x43300000);
        a.store(1,0x3c,5); a.floating_load(1,28,240); a.store(1,0x38,4); a.floating_load(2,28,240);
        a.floating_load(0,1,56); a.store(1,0x44,3); a.floating_subtract(3,0,1,true); a.floating_load(0,28,512);
        a.store(1,0x40,0); a.floating_load(1,1,64); a.floating_subtract(1,1,2,true);
        a.floating_difference(30,3,1,true,true); a.floating_compare(30,0);
        if (a.at_least()) {
        } else {
            a.at(0x102389c8); a.move(3,31); a.call(0x10289560); a.sign16(0,3); a.mask(0,0,0x880,true);
            if (a.equal()) {
                a.at(0x102389f8); a.floating_load(0,28,512); a.floating_load(1,28,272);
                a.floating_subtract(0,30,0); a.floating_product(0,1,0); a.floating_round_single(0,0);
                a.floating_subtract(31,31,0,true);
            } else {
                a.at(0x102389dc); a.floating_load(0,28,512); a.floating_load(1,28,312);
                a.floating_subtract(0,30,0); a.floating_product(0,1,0); a.floating_round_single(0,0);
                a.floating_subtract(31,31,0,true); a.jump(0x10238a10);
            }
        }
    }
    a.at(0x10238a10);
    balance_phase_37(a,state);
}
void balance_phase_35(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102388d8); a.load<1>(0,31,0x4); a.compare(0,4,false);
    if (a.positive()) {
        a.at(0x102388e4); a.load<1>(5,31,0x5); a.value(4,0x43300000); a.load<1>(3,31,0x4); a.value(0,0x43300000);
        a.store(1,0x3c,5); a.floating_load(1,28,240); a.store(1,0x38,4); a.floating_load(2,28,240);
        a.floating_load(0,1,56); a.store(1,0x44,3); a.floating_subtract(3,0,1,true); a.floating_load(0,28,512);
        a.store(1,0x40,0); a.floating_load(1,1,64); a.floating_subtract(1,1,2,true);
        a.floating_difference(30,3,1,true,true); a.floating_compare(30,0);
        if (a.at_least()) {
        } else {
            a.at(0x1023892c); a.move(3,31); a.call(0x10289560); a.sign16(0,3); a.mask(0,0,0x880,true);
            if (a.equal()) {
                a.at(0x1023895c); a.floating_load(0,28,512); a.floating_load(1,28,272);
                a.floating_subtract(0,30,0); a.floating_product(0,1,0); a.floating_round_single(0,0);
                a.floating_subtract(31,31,0,true);
            } else {
                a.at(0x10238940); a.floating_load(0,28,512); a.floating_load(1,28,704);
                a.floating_subtract(0,30,0); a.floating_product(0,1,0); a.floating_round_single(0,0);
                a.floating_subtract(31,31,0,true); a.jump(0x10238974);
            }
        }
    }
    a.at(0x10238974);
    balance_phase_36(a,state);
}
void balance_phase_34(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238828); a.load<1>(3,31,0x4); a.load<1>(0,31,0x7); a.add(0,3,0); a.compare(0,9);
    if (a.nonpositive()) {
        a.at(0x10238ad4);
        balance_phase_41(a,state);
    } else {
        a.at(0x1023883c); a.load<1>(7,31,0x5); a.value(5,0x43300000); a.load<1>(6,31,0x8); a.value(0,0x43300000);
        a.load<1>(4,31,0x4); a.load<1>(3,31,0x7); a.add(6,7,6); a.exclusive_flags(6,6,0x80000000);
        a.store(1,0x38,5); a.add(3,4,3); a.floating_load(2,28,128); a.exclusive_flags(3,3,0x80000000);
        a.store(1,0x3c,6); a.floating_load(1,28,128); a.floating_load(0,1,56); a.store(1,0x44,3);
        a.floating_subtract(2,0,2,true); a.load<1>(3,31,0x9); a.store(1,0x40,0); a.load<1>(0,31,0x6);
        a.floating_load(0,1,64); a.add_record(0,3,0); a.floating_subtract(0,0,1,true);
        a.floating_difference(31,2,0,true,true);
        if (!a.equal()) {
            a.at(0x102388a0); a.load<1>(4,31,0x9); a.value(0,0x43300000); a.load<1>(3,31,0x6); a.store(1,0x48,0);
            a.add(0,4,3); a.floating_load(1,28,128); a.exclusive_flags(0,0,0x80000000); a.floating_load(2,28,696);
            a.store(1,0x4c,0); a.floating_load(0,1,72); a.floating_subtract(0,0,1,true); a.floating_product(0,2,0);
            a.floating_round_single(0,0); a.floating_sum_single(31,31,0);
        }
        a.at(0x102388d8);
        balance_phase_35(a,state);
    }
}
void balance_phase_33(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102387e8); a.load(4,30,0x0); a.value(3,0x1b4f0000); a.load(0,29,0x0); a.offset(3,3,-0x7e4b);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x3); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x4b); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x1023881c); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x1); a.store<2>(31,0x36,0);
    }
    a.at(0x10238828);
    balance_phase_34(a,state);
}
void balance_phase_32(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1023878c); a.sign8(0,0,true);
    if (!a.equal()) {
    } else {
        a.at(0x10238794); a.load<2>(0,31,0x36,true); a.compare(0,5750);
        if (a.at_least()) {
        } else {
            a.at(0x102387a0); a.load<2>(3,31,0x36,true); a.offset(0,3,0x1); a.store<2>(31,0x36,0);
            a.jump(0x102393a0);
        }
    }
}
void balance_phase_31(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1023875c); a.load<1>(0,31,0x27); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x1023877c); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1f8d); a.sign8(0,0);
    } else {
        a.at(0x10238768); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load<1>(0,3,-0x1fc3); a.sign8(0,0);
        a.jump(0x1023878c);
    }
    a.at(0x1023878c);
    balance_phase_32(a,state);
}
void balance_phase_30(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238714); a.load<1>(0,31,0x6); a.compare(0,0,false);
    if (a.equal()) {
        a.at(0x10238720); a.load<1>(0,31,0x9); a.compare(0,0,false);
        if (!a.equal()) {
        } else {
            a.at(0x1023872c); a.load<1>(0,31,0x3); a.compare(0,0,false);
            if (!a.equal()) {
            } else {
                a.at(0x10238738); a.load<1>(0,31,0x10); a.compare(0,0,false);
                if (!a.equal()) {
                } else {
                    a.at(0x10238744); a.load<2>(0,31,0x36,true); a.compare(0,7500);
                    if (a.nonpositive()) {
                    } else {
                        a.at(0x10238750); a.load<2>(3,31,0x36,true); a.offset(0,3,-0xa); a.store<2>(31,0x36,0);
                    }
                }
            }
        }
    }
    a.at(0x1023875c);
    balance_phase_31(a,state);
}
void balance_phase_29(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102386d4); a.load(4,30,0x0); a.value(3,0x1b4f0000); a.load(0,29,0x0); a.offset(3,3,-0x7e4b);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x3); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x4b); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x10238708); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x1); a.store<2>(31,0x36,0);
    }
    a.at(0x10238714);
    balance_phase_30(a,state);
}
void balance_phase_28(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238684); a.compare(25,0);
    if (!a.equal()) {
    } else {
        a.at(0x1023868c); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x1); a.store<2>(31,0x36,0); a.jump(0x102393a0);
    }
}
void balance_phase_27(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238680); a.move(25,0);
    balance_phase_28(a,state);
}
void balance_phase_26(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102384b0); a.load(3,30,0x0); a.value(6,0x80010000); a.value(7,0x2e8c0000); a.value(0,0x43300000);
    a.load<2>(9,3,0x0); a.value(5,0x43300000); a.load(4,29,0x0); a.value(8,0x10000); a.offset(6,6,-0x7fff);
    a.value(3,0x2e8c0000); a.add(10,9,4); a.value(4,0x80010000); a.multiply_high(9,6,10); a.sign16(6,24);
    a.offset(7,7,-0x5d17); a.store(1,0x38,0); a.floating_load(3,28,128); a.offset(8,8,-0x1); a.add(0,9,10);
    a.store(1,0x40,5); a.shift_right(9,0,0xf); a.floating_load(0,28,0); a.multiply_high(6,7,6); a.sign16(0,24);
    a.mask(7,9,0x1,false,1); a.floating_load(1,28,128); a.offset(3,3,-0x5d17); a.add(7,9,7);
    a.shift_right(5,6,0x1); a.offset(4,4,-0x7fff); a.mask(6,5,0x1,false,1); a.add(5,5,6);
    a.subtract_from(5,5,0x5a); a.exclusive_flags(5,5,0x80000000); a.store(1,0x3c,5); a.product(5,7,8);
    a.floating_load(2,1,56); a.floating_subtract(2,2,3); a.multiply_high(0,3,0); a.subtract(25,5,10);
    a.floating_difference(2,2,0,true); a.exclusive_flags(3,25,0x80000000); a.store(1,0x44,3);
    a.shift_right(0,0,0x1); a.floating_load(0,1,64); a.mask(3,0,0x1,false,1); a.floating_subtract(0,0,1);
    a.add(0,0,3); a.subtract_from(0,0,0x5a); a.multiply(0,0,0xa); a.floating_product(0,2,0);
    a.multiply_high(3,4,0); a.floating_integer(0,0); a.floating_store(0,1,72); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.load(25,1,0x4c); a.add(3,0,3); a.call(0x10226520);
    a.value(5,0x66660000); a.value(4,0x2e8c0000); a.offset(3,3,0x5); a.sign16(0,24); a.offset(5,5,0x6667);
    a.offset(4,4,-0x5d17); a.multiply_high(3,5,3); a.multiply_high(0,4,0); a.shift_right(3,3,0x2);
    a.mask(4,3,0x1,false,1); a.add(3,3,4); a.shift_right(0,0,0x1); a.add(25,25,3); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.subtract_from(3,0,0x5a); a.offset(0,3,-0x1); a.compare_registers(25,0);
    if (a.positive()) {
        a.at(0x102385ec); a.value(3,0x2e8c0000); a.sign16(0,24); a.offset(3,3,-0x5d17); a.multiply_high(0,3,0);
        a.shift_right(0,0,0x1); a.mask(3,0,0x1,false,1); a.add(0,0,3); a.subtract_from(3,0,0x5a);
        a.offset(25,3,-0x1); a.jump(0x10238684);
    }
    a.at(0x10238684);
    balance_phase_28(a,state);
}
void balance_phase_25(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238450); a.value(4,0x2e8c0000); a.value(3,0x10000); a.sign16(0,24); a.offset(4,4,-0x5d17);
    a.offset(3,3,-0x1); a.multiply_high(0,4,0); a.shift_right(0,0,0x1); a.mask(4,0,0x1,false,1); a.add(0,0,4);
    a.subtract_from(0,0,0x5a); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x102384b0);
        balance_phase_26(a,state);
    } else {
        a.at(0x10238480); a.value(4,0x2e8c0000); a.value(3,0xffff0000); a.sign16(0,24); a.offset(4,4,-0x5d17);
        a.offset(3,3,0x1); a.multiply_high(0,4,0); a.shift_right(0,0,0x1); a.mask(4,0,0x1,false,1); a.add(0,0,4);
        a.subtract_from(0,0,0x5a); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x10238614); a.value(4,0x2e8c0000); a.load(5,30,0x0); a.sign16(0,24); a.value(3,0x2e8c0000);
            a.offset(4,4,-0x5d17); a.load<2>(6,5,0x0); a.multiply_high(4,4,0); a.load(5,29,0x0); a.sign16(0,24);
            a.offset(3,3,-0x5d17); a.add(6,6,5); a.shift_right(4,4,0x1); a.mask(5,4,0x1,false,1); a.add(4,4,5);
            a.subtract_from(5,4,0x5a); a.divide(4,6,5); a.multiply_high(0,3,0); a.product(4,4,5);
            a.shift_right(0,0,0x1); a.mask(3,0,0x1,false,1); a.add(0,0,3); a.subtract_from(0,0,0x5a);
            a.subtract(25,4,6); a.compare(0,0); a.negate(0,25);
            if (a.positive()) {
                a.at(0x1023867c); a.move(0,25);
            }
            a.at(0x10238680);
            balance_phase_27(a,state);
        } else {
            a.at(0x102384b0);
            balance_phase_26(a,state);
        }
    }
}
void balance_phase_24(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102383b0); a.load<1>(0,31,0x27); a.load(3,31,0x1a4); a.sign8(0,0); a.load<2>(4,31,0x36,true);
    a.leading_zeroes(0,0); a.offset(3,3,0x10000); a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0);
    a.offset(3,3,-0x1fc4); a.multiply(0,0,0x36); a.add(3,3,0); a.load<1>(0,3,0x3); a.multiply(3,0,0xfa);
    a.offset(0,3,0x186a); a.compare_registers(4,0);
    if (a.nonpositive()) {
    } else {
        a.at(0x102383f0); a.load(4,31,0x1a4); a.value(3,0x2e8c0000); a.offset(3,3,-0x5d17);
        a.load<2>(24,4,0x7948,true); a.sign16(0,24); a.multiply_high(0,3,0); a.shift_right(0,0,0x1);
        a.mask(3,0,0x1,false,1); a.add(0,0,3); a.subtract_from(0,0,0x5a); a.compare(0,0);
        if (!a.equal()) {
            a.at(0x10238428); a.load(3,30,0x0); a.load(0,26,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10238440); a.store(30,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x10238450);
            balance_phase_25(a,state);
        } else {
            a.at(0x10238420); a.value(25,0x0); a.jump(0x10238684);
            balance_phase_28(a,state);
        }
    }
}
void balance_phase_23(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238364); a.load<2>(0,31,0x36,true); a.compare(0,8750);
    if (a.nonpositive()) {
        a.at(0x10238380); a.load<2>(0,31,0x36,true); a.compare(0,7250);
        if (a.nonpositive()) {
            a.at(0x102383a0); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x1); a.store<2>(31,0x36,0);
            a.jump(0x102393a0);
        } else {
            a.at(0x1023838c); a.load<1>(3,31,0xf); a.load<2>(0,31,0x36,true); a.subtract(0,3,0);
            a.store<2>(31,0x36,0); a.jump(0x102393a0);
        }
    } else {
        a.at(0x10238370); a.load<2>(3,31,0x36,true); a.offset(0,3,-0x19); a.store<2>(31,0x36,0);
        a.jump(0x102393a0);
    }
}
void balance_phase_22(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238338); a.compare(24,0);
    if (a.equal()) {
        a.at(0x102383b0);
        balance_phase_24(a,state);
    } else {
        a.at(0x10238340); a.load<1>(0,31,0xf); a.compare(0,2,false);
        if (a.nonpositive()) {
            a.at(0x10238364);
            balance_phase_23(a,state);
        } else {
            a.at(0x1023834c); a.load<2>(0,31,0x36,true); a.compare(0,7750);
            if (a.nonpositive()) {
                a.at(0x10238364);
                balance_phase_23(a,state);
            } else {
                a.at(0x10238358); a.value(0,0x1e46); a.store<2>(31,0x36,0); a.jump(0x102393a0);
            }
        }
    }
}
void balance_phase_21(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238334); a.move(24,0);
    balance_phase_22(a,state);
}
void balance_phase_20(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238224); a.exclusive_flags(3,25,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0x38,0); a.value(5,0x10000);
    a.floating_load(2,28,128); a.value(3,0x80010000); a.floating_load(1,1,56); a.offset(6,6,-0x7fff);
    a.floating_load(0,28,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,29,0x0);
    a.multiply(0,25,0xa); a.offset(3,3,-0x7fff); a.store(1,0x40,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,28,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(24,4,7);
    a.exclusive_flags(4,24,0x80000000); a.store(1,0x44,4); a.multiply_high(3,3,0); a.floating_load(0,1,64);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,72); a.load(24,1,0x4c);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,25,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(24,24,3);
    a.compare_registers(24,0);
    if (a.positive()) {
        a.at(0x10238300); a.offset(24,25,-0x1); a.jump(0x10238338);
    }
    a.at(0x10238338);
    balance_phase_22(a,state);
}
void balance_phase_19(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238204); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(25,3);
    if (a.positive()) {
        a.at(0x10238224);
        balance_phase_20(a,state);
    } else {
        a.at(0x10238214); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(25,3);
        if (a.at_least()) {
            a.at(0x10238308); a.load(3,30,0x0); a.compare(25,0); a.load(0,29,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,25); a.product(0,0,25); a.subtract(24,0,3); a.negate(0,24);
            if (a.positive()) {
                a.at(0x10238330); a.move(0,24);
            }
            a.at(0x10238334);
            balance_phase_21(a,state);
        } else {
            a.at(0x10238224);
            balance_phase_20(a,state);
        }
    }
}
void balance_phase_18(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1023818c); a.multiply(3,0,0xfa); a.load<2>(4,31,0x36,true); a.offset(0,3,0x1b58);
    a.compare_registers(4,0);
    if (a.at_least()) {
    } else {
        a.at(0x102381a0); a.load<2>(3,31,0x36,true); a.offset(0,3,0x1); a.store<2>(31,0x36,0); a.jump(0x102393a0);
    }
}
void balance_phase_17(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238120); a.store<2>(31,0x36,0); a.load<1>(0,31,0x27); a.load(3,31,0x1a4); a.sign8(0,0);
    a.leading_zeroes(0,0); a.offset(3,3,0x10000); a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0);
    a.offset(3,3,-0x1fc4); a.multiply(0,0,0x36); a.add(3,3,0); a.load<1>(0,3,0x3); a.compare(0,2,false);
    if (a.at_least()) {
        a.at(0x10238188); a.value(0,0x2);
    } else {
        a.at(0x10238158); a.load<1>(0,31,0x27); a.load(3,31,0x1a4); a.sign8(0,0); a.leading_zeroes(0,0);
        a.offset(3,3,0x10000); a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0); a.offset(3,3,-0x1fc4);
        a.multiply(0,0,0x36); a.add(3,3,0); a.load<1>(0,3,0x3); a.jump(0x1023818c);
    }
    a.at(0x1023818c);
    balance_phase_18(a,state);
}
void balance_phase_16(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238094); a.store<2>(31,0x36,0); a.load<1>(0,31,0xf); a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x102381b0); a.load<2>(0,31,0x36,true); a.compare(0,4250);
        if (a.nonpositive()) {
        } else {
            a.at(0x102381bc); a.load<1>(0,31,0xf); a.compare(0,1,false);
            if (a.nonpositive()) {
                a.at(0x102383b0);
                balance_phase_24(a,state);
            } else {
                a.at(0x102381c8); a.load<1>(25,31,0xf); a.compare(25,0);
                if (!a.equal()) {
                    a.at(0x102381dc); a.load(3,30,0x0); a.load(0,26,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x102381f4); a.store(30,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(29,0x0,0);
                    }
                    a.at(0x10238204);
                    balance_phase_19(a,state);
                } else {
                    a.at(0x102381d4); a.value(24,0x0); a.jump(0x10238338);
                    balance_phase_22(a,state);
                }
            }
        }
    } else {
        a.at(0x102380a4); a.load<1>(0,31,0x27); a.load(4,31,0x1a4); a.sign8(3,0); a.load<2>(0,31,0x36,true);
        a.leading_zeroes(3,3); a.offset(4,4,0x10000); a.mask(3,3,0x7ffffff,false,27); a.sign8(3,3);
        a.offset(4,4,-0x1fc4); a.multiply(3,3,0x36); a.add(3,4,3); a.load<1>(3,3,0x3); a.multiply(3,3,0x12c);
        a.offset(3,3,0x1770); a.compare_registers(3,0);
        if (a.nonpositive()) {
            a.at(0x1023811c); a.load<2>(0,31,0x36,true);
        } else {
            a.at(0x102380e4); a.load<1>(0,31,0x27); a.load(3,31,0x1a4); a.sign8(0,0); a.leading_zeroes(0,0);
            a.offset(3,3,0x10000); a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0); a.offset(3,3,-0x1fc4);
            a.multiply(0,0,0x36); a.add(3,3,0); a.load<1>(0,3,0x3); a.multiply(3,0,0x12c); a.offset(0,3,0x1770);
            a.jump(0x10238120);
        }
        a.at(0x10238120);
        balance_phase_17(a,state);
    }
}
void balance_phase_15(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10238018); a.load<1>(0,31,0x27); a.load(4,31,0x1a4); a.sign8(3,0); a.load<2>(0,31,0x36,true);
    a.leading_zeroes(3,3); a.offset(4,4,0x10000); a.mask(3,3,0x7ffffff,false,27); a.sign8(3,3);
    a.offset(4,4,-0x1fc4); a.multiply(3,3,0x36); a.add(3,4,3); a.load<1>(3,3,0x3); a.multiply(3,3,0x1f4);
    a.offset(3,3,0x1b58); a.compare_registers(3,0);
    if (a.at_least()) {
        a.at(0x10238090); a.load<2>(0,31,0x36,true);
    } else {
        a.at(0x10238058); a.load<1>(0,31,0x27); a.load(3,31,0x1a4); a.sign8(0,0); a.leading_zeroes(0,0);
        a.offset(3,3,0x10000); a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0); a.offset(3,3,-0x1fc4);
        a.multiply(0,0,0x36); a.add(3,3,0); a.load<1>(0,3,0x3); a.multiply(3,0,0x1f4); a.offset(0,3,0x1b58);
        a.jump(0x10238094);
    }
    a.at(0x10238094);
    balance_phase_16(a,state);
}
void balance_phase_14(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237fd0); a.load<1>(0,31,0x1f); a.sign8(0,0); a.compare(0,-1);
    if (a.equal()) {
        a.at(0x10238018);
        balance_phase_15(a,state);
    } else {
        a.at(0x10237fe0); a.load<1>(0,31,0x1f); a.sign8(0,0); a.compare(0,30);
        if (a.nonpositive()) {
            a.at(0x10238018);
            balance_phase_15(a,state);
        } else {
            a.at(0x10237ff0); a.load<1>(0,31,0xf); a.compare(0,0,false);
            if (!a.equal()) {
            } else {
                a.at(0x10237ffc); a.load<2>(0,31,0x36,true); a.compare(0,7000);
                if (a.at_least()) {
                } else {
                    a.at(0x10238008); a.load<2>(3,31,0x36,true); a.offset(0,3,0x1); a.store<2>(31,0x36,0);
                    a.jump(0x102393a0);
                }
            }
        }
    }
}
void balance_phase_13(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237fb0); a.compare(25,0);
    if (a.equal()) {
        a.at(0x10237fb8); a.load<2>(0,31,0x36,true); a.compare(0,7350);
        if (a.at_least()) {
        } else {
            a.at(0x10237fc4); a.load<2>(3,31,0x36,true); a.offset(0,3,0x1); a.store<2>(31,0x36,0);
        }
    }
    a.at(0x10237fd0);
    balance_phase_14(a,state);
}
void balance_phase_12(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237fac); a.move(25,0);
    balance_phase_13(a,state);
}
void balance_phase_11(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237e9c); a.exclusive_flags(3,24,0x80000000); a.value(0,0x43300000); a.store(1,0x44,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0x40,0); a.value(5,0x10000);
    a.floating_load(2,28,128); a.value(3,0x80010000); a.floating_load(1,1,64); a.offset(6,6,-0x7fff);
    a.floating_load(0,28,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,29,0x0);
    a.multiply(0,24,0xa); a.offset(3,3,-0x7fff); a.store(1,0x48,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,28,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(25,4,7);
    a.exclusive_flags(4,25,0x80000000); a.store(1,0x4c,4); a.multiply_high(3,3,0); a.floating_load(0,1,72);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,56); a.load(25,1,0x3c);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,24,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(25,25,3);
    a.compare_registers(25,0);
    if (a.positive()) {
        a.at(0x10237f78); a.offset(25,24,-0x1); a.jump(0x10237fb0);
    }
    a.at(0x10237fb0);
    balance_phase_13(a,state);
}
void balance_phase_10(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237e7c); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(24,3);
    if (a.positive()) {
        a.at(0x10237e9c);
        balance_phase_11(a,state);
    } else {
        a.at(0x10237e8c); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(24,3);
        if (a.at_least()) {
            a.at(0x10237f80); a.load(3,30,0x0); a.compare(24,0); a.load(0,29,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,24); a.product(0,0,24); a.subtract(25,0,3); a.negate(0,25);
            if (a.positive()) {
                a.at(0x10237fa8); a.move(0,25);
            }
            a.at(0x10237fac);
            balance_phase_12(a,state);
        } else {
            a.at(0x10237e9c);
            balance_phase_11(a,state);
        }
    }
}
void balance_phase_9(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237e2c); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x1,true,0);
    if (a.equal()) {
        a.at(0x1023869c); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x2,true,0);
        if (a.equal()) {
            a.at(0x102387b0); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x4,true,0);
            if (a.equal()) {
                a.at(0x10238f08); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x8,true,0);
                if (a.equal()) {
                    a.at(0x10239078); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x10,true,0);
                    if (a.equal()) {
                        a.at(0x10239158); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x20,true,0);
                        if (a.equal()) {
                            a.at(0x10239278); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x40,true,0);
                            if (a.equal()) {
                            } else {
                                a.at(0x10239288); a.load(3,30,0x0); a.load(0,26,0x0); a.offset(3,3,0x2);
                                a.compare_registers(3,0,false); a.store(30,0x0,3);
                                if (a.positive()) {
                                    a.at(0x102392a0); a.store(30,0x0,27); a.call(0x1077b770);
                                    a.mask(0,3,0xffff,false,0); a.store(29,0x0,0);
                                }
                                a.at(0x102392b0);
                                balance_phase_66(a,state);
                            }
                        } else {
                            a.at(0x10239168); a.load(3,30,0x0); a.load(0,26,0x0); a.offset(3,3,0x2);
                            a.compare_registers(3,0,false); a.store(30,0x0,3);
                            if (a.positive()) {
                                a.at(0x10239180); a.store(30,0x0,27); a.call(0x1077b770);
                                a.mask(0,3,0xffff,false,0); a.store(29,0x0,0);
                            }
                            a.at(0x10239190);
                            balance_phase_62(a,state);
                        }
                    } else {
                        a.at(0x10239088); a.load(3,30,0x0); a.load(0,26,0x0); a.offset(3,3,0x2);
                        a.compare_registers(3,0,false); a.store(30,0x0,3);
                        if (a.positive()) {
                            a.at(0x102390a0); a.store(30,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                            a.store(29,0x0,0);
                        }
                        a.at(0x102390b0);
                        balance_phase_59(a,state);
                    }
                } else {
                    a.at(0x10238f18); a.load(3,30,0x0); a.load(0,26,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x10238f30); a.store(30,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(29,0x0,0);
                    }
                    a.at(0x10238f40);
                    balance_phase_54(a,state);
                }
            } else {
                a.at(0x102387c0); a.load(3,30,0x0); a.load(0,26,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x102387d8); a.store(30,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(29,0x0,0);
                }
                a.at(0x102387e8);
                balance_phase_33(a,state);
            }
        } else {
            a.at(0x102386ac); a.load(3,30,0x0); a.load(0,26,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x102386c4); a.store(30,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x102386d4);
            balance_phase_29(a,state);
        }
    } else {
        a.at(0x10237e3c); a.load<1>(3,31,0xf); a.load<1>(0,31,0xf);
        a.product(24,3,0); state.wr<1>(2304,(state.r<1>(2304)&0xf0)|(std::int32_t(a.r(24))<0?8:a.r(24)>0?4:2)|(state.r<1>(1024)&1));
        if (!a.equal()) {
            a.at(0x10237e54); a.load(3,30,0x0); a.load(0,26,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10237e6c); a.store(30,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x10237e7c);
            balance_phase_10(a,state);
        } else {
            a.at(0x10237e4c); a.value(25,0x0); a.jump(0x10237fb0);
            balance_phase_13(a,state);
        }
    }
}
void balance_phase_8(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237e28); a.store<2>(31,0x36,0);
    balance_phase_9(a,state);
}
void balance_phase_7(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237dcc); a.load<1>(0,31,0xc); a.compare(0,0,false);
    if (a.equal()) {
        a.at(0x10237e2c);
        balance_phase_9(a,state);
    } else {
        a.at(0x10237dd8); a.load<1>(3,31,0xc); a.load<1>(0,31,0x10); a.add(0,3,0); a.compare(0,1);
        if (a.nonpositive()) {
            a.at(0x10237e2c);
            balance_phase_9(a,state);
        } else {
            a.at(0x10237dec); a.load<1>(4,31,0xc); a.load<1>(3,31,0x10); a.load<2>(0,31,0x36,true); a.add(3,4,3);
            a.multiply(3,3,0x177); a.offset(3,3,0x1964); a.compare_registers(3,0);
            if (a.nonpositive()) {
                a.at(0x10237e24); a.load<2>(0,31,0x36,true);
            } else {
                a.at(0x10237e0c); a.load<1>(3,31,0xc); a.load<1>(0,31,0x10); a.add(0,3,0); a.multiply(3,0,0x177);
                a.offset(0,3,0x1964); a.jump(0x10237e28);
            }
            a.at(0x10237e28);
            balance_phase_8(a,state);
        }
    }
}
void balance_phase_6(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237dc8); a.store<2>(31,0x36,0);
    balance_phase_7(a,state);
}
void balance_phase_5(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237d90); a.load<1>(0,31,0x10); a.compare(0,0,false);
    if (a.equal()) {
        a.at(0x10237dcc);
        balance_phase_7(a,state);
    } else {
        a.at(0x10237d9c); a.load<1>(3,31,0x10); a.load<2>(0,31,0x36,true); a.multiply(3,3,0x1a9);
        a.offset(3,3,0x1770); a.compare_registers(3,0);
        if (a.nonpositive()) {
            a.at(0x10237dc4); a.load<2>(0,31,0x36,true);
        } else {
            a.at(0x10237db4); a.load<1>(0,31,0x10); a.multiply(3,0,0x1a9); a.offset(0,3,0x1770);
            a.jump(0x10237dc8);
        }
        a.at(0x10237dc8);
        balance_phase_6(a,state);
    }
}
void balance_phase_4(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237cf8); a.store<2>(31,0x36,3); a.load<1>(0,31,0x2); a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x10237d08); a.load<1>(5,31,0x1); a.value(4,0x43300000); a.load<1>(3,31,0x2); a.value(0,0x43300000);
        a.store(1,0x44,5); a.floating_load(1,28,240); a.store(1,0x40,4); a.floating_load(2,28,240);
        a.floating_load(0,1,64); a.store(1,0x4c,3); a.floating_subtract(3,0,1,true); a.floating_load(0,28,48);
        a.store(1,0x48,0); a.floating_load(1,1,72); a.floating_subtract(1,1,2,true);
        a.floating_difference(4,3,1,true,true); a.floating_compare(4,0);
        if (a.nonpositive()) {
        } else {
            a.at(0x10237d50); a.load<2>(3,31,0x36,true); a.value(0,0x43300000); a.store(1,0x40,0);
            a.exclusive_flags(0,3,0x80000000); a.floating_load(3,28,128); a.store(1,0x44,0);
            a.floating_load(1,25,176,true); a.floating_load(2,1,64); a.floating_load(0,25,172,true);
            a.floating_subtract(2,2,3,true); a.floating_product_sum(0,1,4,0); a.floating_compare(2,0);
            if (a.at_least()) {
            } else {
                a.at(0x10237d84); a.load<2>(3,31,0x36,true); a.offset(0,3,0x5); a.store<2>(31,0x36,0);
            }
        }
    }
    a.at(0x10237d90);
    balance_phase_5(a,state);
}
void balance_phase_3(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237ca0); a.load<1>(0,31,0x1); a.compare(0,50,false);
    if (a.nonpositive()) {
        a.at(0x10237d90);
        balance_phase_5(a,state);
    } else {
        a.at(0x10237cac); a.load<1>(3,31,0x1); a.load<1>(4,31,0x3); a.offset(3,3,-0x32); a.load<2>(0,31,0x36,true);
        a.multiply(4,4,0x190); a.multiply(3,3,0x14); a.add(3,4,3); a.offset(3,3,0x1964); a.compare_registers(3,0);
        if (a.nonpositive()) {
            a.at(0x10237cf4); a.load<2>(3,31,0x36,true);
        } else {
            a.at(0x10237cd4); a.load<1>(3,31,0x1); a.load<1>(4,31,0x3); a.offset(0,3,-0x32); a.multiply(3,4,0x190);
            a.multiply(0,0,0x14); a.add(3,3,0); a.offset(3,3,0x1964); a.jump(0x10237cf8);
        }
        a.at(0x10237cf8);
        balance_phase_4(a,state);
    }
}
void balance_phase_2(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237c20); a.load(3,30,0x0); a.value(4,0x5760000); a.load(0,29,0x0); a.offset(4,4,0x19f1);
    a.load<2>(5,3,0x0); a.load<2>(3,31,0x36,true); a.add(5,5,0); a.multiply_high(0,4,5); a.shift_right(0,0,0x5);
    a.mask(4,0,0x1,false,1); a.add(0,0,4); a.multiply(0,0,0x5dc); a.subtract(4,0,5); a.offset(0,4,0x203a);
    a.compare_registers(3,0);
    if (a.positive()) {
        a.at(0x10237c60); a.load<2>(3,31,0x36,true); a.value(0,0x43300000); a.store(1,0x48,0);
        a.offset(0,3,-0x203a); a.floating_load(2,28,128); a.exclusive_flags(0,0,0x80000000);
        a.floating_load(3,28,688); a.store(1,0x4c,0); a.floating_load(0,28,680); a.floating_load(1,1,72);
        a.floating_subtract(1,1,2); a.floating_product_sum(0,3,1,0); a.floating_integer(0,0);
        a.floating_store(0,1,64); a.load(0,1,0x44); a.store<2>(31,0x36,0);
    }
    a.at(0x10237ca0);
    balance_phase_3(a,state);
}
void balance_phase_1(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10237bb4); a.floating_load(1,25,16,true); a.move(3,31); a.call(0x102865a0); a.load<1>(3,31,0xc);
    a.load<1>(0,31,0x10); a.add(0,3,0); a.compare(0,2);
    if (a.at_least()) {
        a.at(0x10237ca0);
        balance_phase_3(a,state);
    } else {
        a.at(0x10237bd4); a.load<1>(0,31,0x6); a.compare(0,7,false);
        if (a.at_least()) {
            a.at(0x10237ca0);
            balance_phase_3(a,state);
        } else {
            a.at(0x10237be0); a.load<1>(0,31,0x3); a.compare(0,4,false);
            if (a.at_least()) {
                a.at(0x10237ca0);
                balance_phase_3(a,state);
            } else {
                a.at(0x10237bec); a.load<1>(0,31,0x11); a.compare(0,6,false);
                if (a.at_least()) {
                    a.at(0x10237ca0);
                    balance_phase_3(a,state);
                } else {
                    a.at(0x10237bf8); a.load(3,30,0x0); a.load(0,26,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                    if (a.positive()) {
                        a.at(0x10237c10); a.store(30,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(29,0x0,0);
                    }
                    a.at(0x10237c20);
                    balance_phase_2(a,state);
                }
            }
        }
    }
}
}
void update_balance(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10237b50,dependencies,observer);
    a.at(0x10237b50); a.link(); a.floating_store(31,1,-8); a.floating_store(30,1,-16); a.multiple(24,false,-16);
    a.move(31,3); a.load(28,2,-0x51e8); a.load(29,2,-0x7b90); a.load(27,2,-0x7b8c); a.load(26,2,-0x7b88);
    a.load(30,2,-0x7b84); a.load(25,2,-0x51e0); a.save_link(); a.frame(128); a.load<1>(0,3,0x2c);
    a.sign8(0,0,true);
    if (a.nonpositive()) {
        a.at(0x10237ba0); a.load<1>(0,31,0x2c); a.sign8(0,0,true);
        if (a.at_least()) {
        } else {
            a.at(0x10237bac); a.value(0,0x0); a.store<1>(31,0x2c,0);
        }
    } else {
        a.at(0x10237b90); a.load<1>(3,31,0x2c); a.offset(0,3,-0x1); a.store<1>(31,0x2c,0); a.jump(0x10237bb4);
    }
    a.at(0x10237bb4);
    balance_phase_1(a,state);
    a.at(0x102393a0); a.load(0,1,0x88); a.offset(1,1,0x80); a.floating_load(31,1,-8); a.floating_load(30,1,-16);
    a.restore_link(); a.multiple(24,true,-16); a.finish();
}
void add_counter(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10285ed0,dependencies,observer);
    a.at(0x10285ed0); a.load<1>(0,3,0x2c); a.add(0,0,4); a.store<1>(3,0x2c,0); a.finish();
}
void assign_counter(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10285ee0,dependencies,observer);
    a.at(0x10285ee0); a.store<1>(3,0x2c,4); a.finish();
}
}
