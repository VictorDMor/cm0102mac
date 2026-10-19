#include "match_event.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_event {
using Call = SemanticGuestCall;
namespace {
void transition_rule_1(Call&);
void transition_rule_2(Call&);
void transition_rule_3(Call&);
void transition_rule_4(Call&);
void transition_rule_5(Call&);
void transition_rule_6(Call&);
void transition_rule_7(Call&);
void transition_rule_8(Call&);
void transition_rule_9(Call&);
void transition_rule_10(Call&);
void transition_rule_11(Call&);
void transition_rule_12(Call&);
void transition_rule_13(Call&);
void transition_rule_14(Call&);
void transition_rule_15(Call&);
void transition_rule_16(Call&);
void transition_rule_17(Call&);
void transition_rule_18(Call&);
void transition_rule_19(Call&);
void transition_rule_20(Call&);
void transition_rule_21(Call&);
void transition_rule_22(Call&);
void transition_rule_23(Call&);
void transition_rule_24(Call&);
void transition_rule_25(Call&);
void transition_rule_26(Call&);
void transition_rule_27(Call&);
void transition_rule_28(Call&);
void transition_rule_29(Call&);
void transition_rule_30(Call&);
void transition_rule_31(Call&);
void transition_rule_32(Call&);
void transition_rule_33(Call&);
void transition_rule_34(Call&);
void transition_rule_35(Call&);
void transition_rule_36(Call&);
void transition_rule_37(Call&);
void transition_rule_38(Call&);
void transition_rule_39(Call&);
void transition_rule_40(Call&);
void transition_rule_41(Call&);
void transition_rule_42(Call&);
void transition_rule_43(Call&);
void transition_rule_44(Call&);
void transition_rule_45(Call&);
void transition_rule_46(Call&);
void transition_rule_47(Call&);
void transition_rule_48(Call&);
void transition_rule_49(Call&);
void transition_rule_50(Call&);
void transition_rule_51(Call&);
void transition_rule_52(Call&);
void transition_rule_53(Call&);
void transition_rule_54(Call&);
void transition_rule_55(Call&);
void transition_rule_56(Call&);
void transition_rule_57(Call&);
void transition_rule_58(Call&);
void transition_rule_59(Call&);
void transition_rule_60(Call&);
void transition_rule_61(Call&);
void transition_rule_62(Call&);
void transition_rule_63(Call&);
void transition_rule_64(Call&);
void transition_rule_65(Call&);
void transition_rule_66(Call&);
void transition_rule_67(Call&);
void transition_rule_68(Call&);
void transition_rule_69(Call&);
void transition_rule_70(Call&);
void transition_rule_71(Call&);
void transition_rule_72(Call&);
void transition_rule_73(Call&);
void transition_rule_74(Call&);
void transition_rule_75(Call&);
void transition_rule_76(Call&);
void transition_rule_77(Call&);
void transition_rule_78(Call&);
void transition_rule_79(Call&);
void transition_rule_80(Call&);
void transition_rule_81(Call&);
void transition_rule_82(Call&);
void transition_rule_83(Call&);
void transition_rule_84(Call&);
void transition_rule_85(Call&);
void transition_rule_86(Call&);
void transition_rule_87(Call&);
void transition_rule_88(Call&);
void transition_rule_89(Call&);
void transition_rule_90(Call&);
void transition_rule_91(Call&);
void transition_rule_92(Call&);
void transition_rule_93(Call&);
void transition_rule_94(Call&);
void transition_rule_95(Call&);
void transition_rule_96(Call&);
void transition_rule_97(Call&);
void transition_rule_98(Call&);
void transition_rule_99(Call&);
void transition_rule_99(Call& a) {
    a.at(0x10201060); a.value(3,0x0);
}
void transition_rule_98(Call& a) {
    a.at(0x10201030); a.load<1>(0,25,0x5); a.move(3,24); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
    a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8); a.store<1>(22,0x8,0);
    a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
}
void transition_rule_97(Call& a) {
    a.at(0x10201020); a.compare(23,8006);
    if (a.equal()) {
        a.at(0x10201030);
        transition_rule_98(a);
    } else {
        a.at(0x10201028); a.compare(23,8007);
        if (!a.equal()) {
            a.at(0x10201060);
            transition_rule_99(a);
        } else {
            a.at(0x10201030);
            transition_rule_98(a);
        }
    }
}
void transition_rule_96(Call& a) {
    a.at(0x10201018); a.compare(24,8064);
    if (!a.equal()) {
        a.at(0x10201060);
        transition_rule_99(a);
    } else {
        a.at(0x10201020);
        transition_rule_97(a);
    }
}
void transition_rule_95(Call& a) {
    a.at(0x10201008); a.compare(24,8389);
    if (a.less()) {
        a.at(0x10201018);
        transition_rule_96(a);
    } else {
        a.at(0x10201010); a.compare(24,8400);
        if (a.equal()) {
            a.at(0x10201020);
            transition_rule_97(a);
        } else {
            a.at(0x10201018);
            transition_rule_96(a);
        }
    }
}
void transition_rule_94(Call& a) {
    a.at(0x10200ff0); a.compare(24,8051);
    if (a.equal()) {
        a.at(0x10201020);
        transition_rule_97(a);
    } else {
        a.at(0x10200ff8); a.compare(24,8377);
        if (a.less()) {
            a.at(0x10201008);
            transition_rule_95(a);
        } else {
            a.at(0x10201000); a.compare(24,8388);
            if (a.nonpositive()) {
                a.at(0x10201020);
                transition_rule_97(a);
            } else {
                a.at(0x10201008);
                transition_rule_95(a);
            }
        }
    }
}
void transition_rule_93(Call& a) {
    a.at(0x10200fb8); a.compare(23,8006);
    if (!a.equal()) {
        a.at(0x10200ff0);
        transition_rule_94(a);
    } else {
        a.at(0x10200fc0); a.load<1>(0,25,0x5); a.move(3,24); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
        a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8); a.store<1>(22,0x8,0);
        a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
    }
}
void transition_rule_92(Call& a) {
    a.at(0x10200fa0); a.compare(24,8064);
    if (a.equal()) {
        a.at(0x10200fb8);
        transition_rule_93(a);
    } else {
        a.at(0x10200fa8); a.compare(24,8497);
        if (a.equal()) {
            a.at(0x10200fb8);
            transition_rule_93(a);
        } else {
            a.at(0x10200fb0); a.compare(24,8496);
            if (!a.equal()) {
                a.at(0x10200ff0);
                transition_rule_94(a);
            } else {
                a.at(0x10200fb8);
                transition_rule_93(a);
            }
        }
    }
}
void transition_rule_91(Call& a) {
    a.at(0x10200f70); a.compare(24,8558);
    if (!a.equal()) {
        a.at(0x10200fa0);
        transition_rule_92(a);
    } else {
        a.at(0x10200f78); a.compare(23,8004);
        if (!a.equal()) {
            a.at(0x10200fa0);
            transition_rule_92(a);
        } else {
            a.at(0x10200f80); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
            if (a.equal()) {
                a.at(0x10200fa0);
                transition_rule_92(a);
            } else {
                a.at(0x10200f98); a.value(3,0x216e); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_90(Call& a) {
    a.at(0x10200f44); a.load(3,31,0x0); a.load(0,28,0x0); a.load<2>(3,3,0x0); a.add(0,3,0);
    a.mask(3,0,0x1,false,1); a.mask(0,0,0x1,false,0); a.combine(0,0,3,false,true); a.subtract(0,3,0,true);
    if (a.equal()) {
        a.at(0x10200f70);
        transition_rule_91(a);
    } else {
        a.at(0x10200f68); a.value(3,0xffffde97); a.jump(0x10201064);
    }
}
void transition_rule_89(Call& a) {
    a.at(0x10200f1c); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(31,0x0,3);
    if (a.positive()) {
        a.at(0x10200f34); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(28,0x0,0);
    }
    a.at(0x10200f44);
    transition_rule_90(a);
}
void transition_rule_88(Call& a) {
    a.at(0x10200ef4); a.offset(0,23,-0x1f4b); a.mask(0,0,0xffff,false,0); a.compare(0,1,false);
    if (a.nonpositive()) {
        a.at(0x10200f1c);
        transition_rule_89(a);
    } else {
        a.at(0x10200f04); a.offset(0,23,-0x1f4e); a.mask(0,0,0xffff,false,0); a.compare(0,4,false);
        if (a.nonpositive()) {
            a.at(0x10200f1c);
            transition_rule_89(a);
        } else {
            a.at(0x10200f14); a.compare(23,8019);
            if (!a.equal()) {
                a.at(0x10200f70);
                transition_rule_91(a);
            } else {
                a.at(0x10200f1c);
                transition_rule_89(a);
            }
        }
    }
}
void transition_rule_87(Call& a) {
    a.at(0x10200ecc); a.offset(0,24,-0x1f4b); a.mask(0,0,0xffff,false,0); a.compare(0,1,false);
    if (a.nonpositive()) {
        a.at(0x10200ef4);
        transition_rule_88(a);
    } else {
        a.at(0x10200edc); a.offset(0,24,-0x1f4e); a.mask(0,0,0xffff,false,0); a.compare(0,4,false);
        if (a.nonpositive()) {
            a.at(0x10200ef4);
            transition_rule_88(a);
        } else {
            a.at(0x10200eec); a.compare(24,8019);
            if (!a.equal()) {
                a.at(0x10200f70);
                transition_rule_91(a);
            } else {
                a.at(0x10200ef4);
                transition_rule_88(a);
            }
        }
    }
}
void transition_rule_86(Call& a) {
    a.at(0x10200e8c); a.compare(24,8528);
    if (!a.equal()) {
        a.at(0x10200ecc);
        transition_rule_87(a);
    } else {
        a.at(0x10200e94); a.compare(23,8053);
        if (!a.equal()) {
            a.at(0x10200ecc);
            transition_rule_87(a);
        } else {
            a.at(0x10200e9c); a.load<1>(0,25,0x5); a.value(3,0x2151); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_85(Call& a) {
    a.at(0x10200e34); a.compare(24,8048);
    if (!a.equal()) {
        a.at(0x10200e8c);
        transition_rule_86(a);
    } else {
        a.at(0x10200e3c); a.compare(23,8054);
        if (!a.equal()) {
            a.at(0x10200e8c);
            transition_rule_86(a);
        } else {
            a.at(0x10200e44); a.load<1>(3,25,0x6); a.load<1>(0,22,0x6); a.sign8(3,3); a.sign8(0,0);
            a.compare_registers(3,0);
            if (a.equal()) {
                a.at(0x10200e8c);
                transition_rule_86(a);
            } else {
                a.at(0x10200e5c); a.load<1>(0,25,0x5); a.value(3,0x2165); a.store<1>(22,0x5,0);
                a.load<1>(0,25,0x6); a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0);
                a.load<1>(0,25,0x8); a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_84(Call& a) {
    a.at(0x10200e24); a.compare(23,8048);
    if (!a.equal()) {
        a.at(0x10200e34);
        transition_rule_85(a);
    } else {
        a.at(0x10200e2c); a.value(3,0xffffdea9); a.jump(0x10201064);
    }
}
void transition_rule_83(Call& a) {
    a.at(0x10200dfc); a.offset(0,24,-0x215a); a.mask(0,0,0xffff,false,0); a.compare(0,2,false);
    if (a.nonpositive()) {
        a.at(0x10200e24);
        transition_rule_84(a);
    } else {
        a.at(0x10200e0c); a.offset(0,24,-0x215e); a.mask(0,0,0xffff,false,0); a.compare(0,1,false);
        if (a.nonpositive()) {
            a.at(0x10200e24);
            transition_rule_84(a);
        } else {
            a.at(0x10200e1c); a.compare(24,8541);
            if (!a.equal()) {
                a.at(0x10200e34);
                transition_rule_85(a);
            } else {
                a.at(0x10200e24);
                transition_rule_84(a);
            }
        }
    }
}
void transition_rule_82(Call& a) {
    a.at(0x10200dcc); a.compare(0,0);
    if (a.equal()) {
        a.at(0x10200dfc);
        transition_rule_83(a);
    } else {
        a.at(0x10200dd4); a.compare(24,8467);
        if (!a.equal()) {
            a.at(0x10200dfc);
            transition_rule_83(a);
        } else {
            a.at(0x10200ddc); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
            if (a.equal()) {
                a.at(0x10200dfc);
                transition_rule_83(a);
            } else {
                a.at(0x10200df4); a.value(3,0xffffdea7); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_81(Call& a) {
    a.at(0x10200db4); a.compare(23,8310); a.value(0,0x0);
    if (a.at_least()) {
        a.at(0x10200dc0); a.compare(23,8400);
        if (a.positive()) {
        } else {
            a.at(0x10200dc8); a.value(0,0x1);
        }
    }
    a.at(0x10200dcc);
    transition_rule_82(a);
}
void transition_rule_80(Call& a) {
    a.at(0x10200d84); a.compare(0,0);
    if (a.equal()) {
        a.at(0x10200db4);
        transition_rule_81(a);
    } else {
        a.at(0x10200d8c); a.compare(23,8048);
        if (!a.equal()) {
            a.at(0x10200db4);
            transition_rule_81(a);
        } else {
            a.at(0x10200d94); a.load<1>(3,25,0x6); a.load<1>(0,22,0x6); a.sign8(3,3); a.sign8(0,0);
            a.compare_registers(3,0);
            if (!a.equal()) {
                a.at(0x10200db4);
                transition_rule_81(a);
            } else {
                a.at(0x10200dac); a.value(3,0xffffdea9); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_79(Call& a) {
    a.at(0x10200d6c); a.compare(24,8225); a.value(0,0x0);
    if (a.at_least()) {
        a.at(0x10200d78); a.compare(24,8400);
        if (a.positive()) {
        } else {
            a.at(0x10200d80); a.value(0,0x1);
        }
    }
    a.at(0x10200d84);
    transition_rule_80(a);
}
void transition_rule_78(Call& a) {
    a.at(0x10200d64); a.value(3,0xffffdead); a.jump(0x10201064);
}
void transition_rule_77(Call& a) {
    a.at(0x10200d4c); a.compare(24,8467);
    if (!a.equal()) {
        a.at(0x10200d6c);
        transition_rule_79(a);
    } else {
        a.at(0x10200d54); a.compare(23,8034);
        if (a.equal()) {
            a.at(0x10200d64);
            transition_rule_78(a);
        } else {
            a.at(0x10200d5c); a.compare(23,8528);
            if (!a.equal()) {
                a.at(0x10200d6c);
                transition_rule_79(a);
            } else {
                a.at(0x10200d64);
                transition_rule_78(a);
            }
        }
    }
}
void transition_rule_76(Call& a) {
    a.at(0x10200d34); a.compare(24,8021);
    if (!a.equal()) {
        a.at(0x10200d4c);
        transition_rule_77(a);
    } else {
        a.at(0x10200d3c); a.compare(23,8000);
        if (!a.equal()) {
            a.at(0x10200d4c);
            transition_rule_77(a);
        } else {
            a.at(0x10200d44); a.value(3,0xffffdeae); a.jump(0x10201064);
        }
    }
}
void transition_rule_75(Call& a) {
    a.at(0x10200d2c); a.value(3,0xffffdead); a.jump(0x10201064);
}
void transition_rule_74(Call& a) {
    a.at(0x10200d0c); a.compare(24,8225);
    if (!a.equal()) {
        a.at(0x10200d34);
        transition_rule_76(a);
    } else {
        a.at(0x10200d14); a.offset(0,23,-0x1f62); a.mask(0,0,0xffff,false,0); a.compare(0,1,false);
        if (a.nonpositive()) {
            a.at(0x10200d2c);
            transition_rule_75(a);
        } else {
            a.at(0x10200d24); a.compare(23,8528);
            if (!a.equal()) {
                a.at(0x10200d34);
                transition_rule_76(a);
            } else {
                a.at(0x10200d2c);
                transition_rule_75(a);
            }
        }
    }
}
void transition_rule_73(Call& a) {
    a.at(0x10200ccc); a.compare(24,8000);
    if (!a.equal()) {
        a.at(0x10200d0c);
        transition_rule_74(a);
    } else {
        a.at(0x10200cd4); a.compare(23,8093);
        if (!a.equal()) {
            a.at(0x10200d0c);
            transition_rule_74(a);
        } else {
            a.at(0x10200cdc); a.load<1>(0,25,0x5); a.value(3,0x1f9d); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_72(Call& a) {
    a.at(0x10200c94); a.compare(23,8053);
    if (!a.equal()) {
        a.at(0x10200ccc);
        transition_rule_73(a);
    } else {
        a.at(0x10200c9c); a.load<1>(0,25,0x5); a.value(3,0x2151); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
        a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8); a.store<1>(22,0x8,0);
        a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
    }
}
void transition_rule_71(Call& a) {
    a.at(0x10200c7c); a.compare(24,8510);
    if (a.equal()) {
        a.at(0x10200c94);
        transition_rule_72(a);
    } else {
        a.at(0x10200c84); a.compare(24,8049);
        if (a.equal()) {
            a.at(0x10200c94);
            transition_rule_72(a);
        } else {
            a.at(0x10200c8c); a.compare(24,8050);
            if (!a.equal()) {
                a.at(0x10200ccc);
                transition_rule_73(a);
            } else {
                a.at(0x10200c94);
                transition_rule_72(a);
            }
        }
    }
}
void transition_rule_70(Call& a) {
    a.at(0x10200c3c); a.compare(24,8527);
    if (!a.equal()) {
        a.at(0x10200c7c);
        transition_rule_71(a);
    } else {
        a.at(0x10200c44); a.compare(23,8082);
        if (!a.equal()) {
            a.at(0x10200c7c);
            transition_rule_71(a);
        } else {
            a.at(0x10200c4c); a.load<1>(0,25,0x5); a.value(3,0x214f); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_69(Call& a) {
    a.at(0x10200bd4); a.compare(26,0);
    if (a.equal()) {
        a.at(0x10200c3c);
        transition_rule_70(a);
    } else {
        a.at(0x10200bdc); a.load<1>(3,25,0x7); a.load<1>(0,22,0x5); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x10200c3c);
            transition_rule_70(a);
        } else {
            a.at(0x10200bf4); a.load<1>(3,25,0x8); a.load<1>(0,22,0x6); a.sign8(3,3); a.sign8(0,0);
            a.compare_registers(3,0);
            if (!a.equal()) {
                a.at(0x10200c3c);
                transition_rule_70(a);
            } else {
                a.at(0x10200c0c); a.load<1>(0,25,0x5); a.move(3,24); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
                a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
                a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_68(Call& a) {
    a.at(0x10200ad0); a.value(4,0x43300000); a.exclusive_flags(0,27,0x80000000); a.load(3,2,-0x52a4);
    a.value(6,0x80010000); a.store(1,0x4c,0); a.value(5,0x10000); a.floating_load(2,3,8); a.offset(6,6,-0x7fff);
    a.store(1,0x48,4); a.multiply(0,27,0xa); a.load(7,31,0x0); a.offset(5,5,-0x1); a.floating_load(1,1,72);
    a.floating_load(0,3,0); a.multiply_high(3,6,0); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0);
    a.load(7,28,0x0); a.store(1,0x40,4); a.floating_difference(1,1,0,true); a.add(7,8,7); a.add(0,3,0);
    a.multiply_high(3,6,7); a.add(3,3,7); a.shift_right(3,3,0xf); a.mask(4,3,0x1,false,1); a.shift_right(0,0,0xf);
    a.add(3,3,4); a.product(4,3,5); a.mask(3,0,0x1,false,1); a.add(3,0,3); a.subtract(0,4,7);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0x44,0); a.floating_load(0,1,64); a.floating_subtract(0,0,2);
    a.floating_product(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,56); a.load(26,1,0x3c);
    a.call(0x101ff5f0); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,27,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(26,26,3);
    a.compare_registers(26,0);
    if (a.positive()) {
        a.at(0x10200ba0); a.move(26,0); a.jump(0x10200bd4);
    }
    a.at(0x10200bd4);
    transition_rule_69(a);
}
void transition_rule_67(Call& a) {
    a.at(0x10200ab0); a.value(3,0x10000); a.offset(0,3,-0x1); a.compare_registers(27,0);
    if (a.positive()) {
        a.at(0x10200ad0);
        transition_rule_68(a);
    } else {
        a.at(0x10200ac0); a.value(3,0xffff0000); a.offset(0,3,0x1); a.compare_registers(27,0);
        if (a.at_least()) {
            a.at(0x10200ba8); a.load(3,31,0x0); a.compare(27,0); a.load(0,28,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,27); a.product(0,0,27); a.subtract(0,0,3); a.negate(26,0);
            if (a.positive()) {
                a.at(0x10200bd0); a.move(26,0);
            }
            a.at(0x10200bd4);
            transition_rule_69(a);
        } else {
            a.at(0x10200ad0);
            transition_rule_68(a);
        }
    }
}
void transition_rule_66(Call& a) {
    a.at(0x10200a64); a.negate(0,3); a.combine(0,0,3); a.mask(0,0,0x1,true,1);
    if (a.equal()) {
        a.at(0x10200c3c);
        transition_rule_70(a);
    } else {
        a.at(0x10200a74); a.subtract_from(27,4,0x16); a.compare(27,0);
        if (!a.equal()) {
            a.at(0x10200a88); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(31,0x0,3);
            if (a.positive()) {
                a.at(0x10200aa0); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(28,0x0,0);
            }
            a.at(0x10200ab0);
            transition_rule_67(a);
        } else {
            a.at(0x10200a80); a.value(26,0x0); a.jump(0x10200bd4);
            transition_rule_69(a);
        }
    }
}
void transition_rule_65(Call& a) {
    a.at(0x10200a38); a.compare(0,0);
    if (a.equal()) {
        a.at(0x10200c3c);
        transition_rule_70(a);
    } else {
        a.at(0x10200a40); a.compare(23,8007);
        if (!a.equal()) {
            a.at(0x10200c3c);
            transition_rule_70(a);
        } else {
            a.at(0x10200a48); a.load(4,25,0xc); a.value(3,0x0); a.compare(4,1);
            if (a.at_least()) {
                a.at(0x10200a58); a.compare(4,20);
                if (a.positive()) {
                } else {
                    a.at(0x10200a60); a.value(3,0x1);
                }
            }
            a.at(0x10200a64);
            transition_rule_66(a);
        }
    }
}
void transition_rule_64(Call& a) {
    a.at(0x10200a20); a.compare(24,8225); a.value(0,0x0);
    if (a.at_least()) {
        a.at(0x10200a2c); a.compare(24,8400);
        if (a.positive()) {
        } else {
            a.at(0x10200a34); a.value(0,0x1);
        }
    }
    a.at(0x10200a38);
    transition_rule_65(a);
}
void transition_rule_63(Call& a) {
    a.at(0x102009b4); a.negate(0,3); a.combine(0,0,3); a.mask(0,0,0x1,true,1);
    if (a.equal()) {
        a.at(0x10200a20);
        transition_rule_64(a);
    } else {
        a.at(0x102009c4); a.load<1>(3,22,0x5); a.load<1>(0,25,0x7); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x10200a20);
            transition_rule_64(a);
        } else {
            a.at(0x102009dc); a.load<1>(0,22,0x7); a.load<1>(4,25,0x5); a.sign8(3,0); a.sign8(0,4);
            a.compare_registers(3,0);
            if (!a.equal()) {
                a.at(0x10200a20);
                transition_rule_64(a);
            } else {
                a.at(0x102009f4); a.store<1>(22,0x5,4); a.value(3,0x214e); a.load<1>(0,25,0x6);
                a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
                a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_62(Call& a) {
    a.at(0x10200988); a.negate(0,3); a.combine(0,0,3); a.mask(0,0,0x1,true,1);
    if (a.equal()) {
        a.at(0x10200a20);
        transition_rule_64(a);
    } else {
        a.at(0x10200998); a.load(0,25,0xc); a.value(3,0x0); a.compare(0,1);
        if (a.at_least()) {
            a.at(0x102009a8); a.compare(0,20);
            if (a.positive()) {
            } else {
                a.at(0x102009b0); a.value(3,0x1);
            }
        }
        a.at(0x102009b4);
        transition_rule_63(a);
    }
}
void transition_rule_61(Call& a) {
    a.at(0x10200964); a.compare(0,0);
    if (a.equal()) {
        a.at(0x10200a20);
        transition_rule_64(a);
    } else {
        a.at(0x1020096c); a.load(0,22,0xc); a.value(3,0x0); a.compare(0,1);
        if (a.at_least()) {
            a.at(0x1020097c); a.compare(0,20);
            if (a.positive()) {
            } else {
                a.at(0x10200984); a.value(3,0x1);
            }
        }
        a.at(0x10200988);
        transition_rule_62(a);
    }
}
void transition_rule_60(Call& a) {
    a.at(0x10200944); a.compare(0,0);
    if (a.equal()) {
        a.at(0x10200a20);
        transition_rule_64(a);
    } else {
        a.at(0x1020094c); a.compare(23,8225); a.value(0,0x0);
        if (a.at_least()) {
            a.at(0x10200958); a.compare(23,8400);
            if (a.positive()) {
            } else {
                a.at(0x10200960); a.value(0,0x1);
            }
        }
        a.at(0x10200964);
        transition_rule_61(a);
    }
}
void transition_rule_59(Call& a) {
    a.at(0x1020092c); a.compare(24,8225); a.value(0,0x0);
    if (a.at_least()) {
        a.at(0x10200938); a.compare(24,8400);
        if (a.positive()) {
        } else {
            a.at(0x10200940); a.value(0,0x1);
        }
    }
    a.at(0x10200944);
    transition_rule_60(a);
}
void transition_rule_58(Call& a) {
    a.at(0x102008fc); a.load<1>(0,25,0x5); a.value(3,0x214d); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
    a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8); a.store<1>(22,0x8,0);
    a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
}
void transition_rule_57(Call& a) {
    a.at(0x102008e4); a.compare(24,8524);
    if (!a.equal()) {
        a.at(0x1020092c);
        transition_rule_59(a);
    } else {
        a.at(0x102008ec); a.compare(23,8032);
        if (a.equal()) {
            a.at(0x102008fc);
            transition_rule_58(a);
        } else {
            a.at(0x102008f4); a.compare(23,8031);
            if (!a.equal()) {
                a.at(0x1020092c);
                transition_rule_59(a);
            } else {
                a.at(0x102008fc);
                transition_rule_58(a);
            }
        }
    }
}
void transition_rule_56(Call& a) {
    a.at(0x102008b4); a.load<1>(0,25,0x5); a.value(3,0x214b); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
    a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8); a.store<1>(22,0x8,0);
    a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
}
void transition_rule_55(Call& a) {
    a.at(0x1020089c); a.compare(24,8522);
    if (!a.equal()) {
        a.at(0x102008e4);
        transition_rule_57(a);
    } else {
        a.at(0x102008a4); a.compare(23,8032);
        if (a.equal()) {
            a.at(0x102008b4);
            transition_rule_56(a);
        } else {
            a.at(0x102008ac); a.compare(23,8031);
            if (!a.equal()) {
                a.at(0x102008e4);
                transition_rule_57(a);
            } else {
                a.at(0x102008b4);
                transition_rule_56(a);
            }
        }
    }
}
void transition_rule_54(Call& a) {
    a.at(0x1020086c); a.load<1>(0,25,0x5); a.value(3,0x2149); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
    a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8); a.store<1>(22,0x8,0);
    a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
}
void transition_rule_53(Call& a) {
    a.at(0x10200854); a.compare(24,8520);
    if (!a.equal()) {
        a.at(0x1020089c);
        transition_rule_55(a);
    } else {
        a.at(0x1020085c); a.compare(23,8032);
        if (a.equal()) {
            a.at(0x1020086c);
            transition_rule_54(a);
        } else {
            a.at(0x10200864); a.compare(23,8031);
            if (!a.equal()) {
                a.at(0x1020089c);
                transition_rule_55(a);
            } else {
                a.at(0x1020086c);
                transition_rule_54(a);
            }
        }
    }
}
void transition_rule_52(Call& a) {
    a.at(0x102007c0); a.load(5,31,0x0); a.value(3,0x66660000); a.load(4,28,0x0); a.offset(0,3,0x6667);
    a.load<2>(3,5,0x0); a.add(4,3,4); a.multiply_high(0,0,4); a.shift_right(0,0,0x1); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x5); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x10200854);
        transition_rule_53(a);
    } else {
        a.at(0x102007f4); a.load<1>(3,25,0x6); a.load<1>(0,22,0x6); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x10200854);
            transition_rule_53(a);
        } else {
            a.at(0x1020080c); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
            if (!a.equal()) {
                a.at(0x10200854);
                transition_rule_53(a);
            } else {
                a.at(0x10200824); a.load<1>(0,22,0x5); a.value(3,0x2147); a.store<1>(22,0x7,0);
                a.load<1>(0,22,0x6); a.store<1>(22,0x8,0); a.load<1>(0,25,0x5); a.store<1>(22,0x5,0);
                a.load<1>(0,25,0x6); a.store<1>(22,0x6,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_51(Call& a) {
    a.at(0x10200790); a.compare(23,8006);
    if (!a.equal()) {
        a.at(0x10200854);
        transition_rule_53(a);
    } else {
        a.at(0x10200798); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(31,0x0,3);
        if (a.positive()) {
            a.at(0x102007b0); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(28,0x0,0);
        }
        a.at(0x102007c0);
        transition_rule_52(a);
    }
}
void transition_rule_50(Call& a) {
    a.at(0x10200780); a.compare(24,8510);
    if (a.equal()) {
        a.at(0x10200790);
        transition_rule_51(a);
    } else {
        a.at(0x10200788); a.compare(24,8030);
        if (!a.equal()) {
            a.at(0x10200854);
            transition_rule_53(a);
        } else {
            a.at(0x10200790);
            transition_rule_51(a);
        }
    }
}
void transition_rule_49(Call& a) {
    a.at(0x10200714); a.load(3,31,0x0); a.load(0,28,0x0); a.load<2>(3,3,0x0); a.add(0,3,0);
    a.mask(3,0,0x1,false,1); a.mask(0,0,0x1,false,0); a.combine(0,0,3,false,true); a.subtract(0,3,0,true);
    if (a.equal()) {
        a.at(0x10200780);
        transition_rule_50(a);
    } else {
        a.at(0x10200738); a.load<1>(3,25,0x6); a.load<1>(0,22,0x6); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (a.equal()) {
            a.at(0x10200780);
            transition_rule_50(a);
        } else {
            a.at(0x10200750); a.load<1>(0,22,0x5); a.value(3,0x2146); a.store<1>(22,0x7,0); a.load<1>(0,22,0x6);
            a.store<1>(22,0x8,0); a.load<1>(0,25,0x5); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_48(Call& a) {
    a.at(0x102006dc); a.compare(24,8064);
    if (!a.equal()) {
        a.at(0x10200780);
        transition_rule_50(a);
    } else {
        a.at(0x102006e4); a.compare(23,8006);
        if (!a.equal()) {
            a.at(0x10200780);
            transition_rule_50(a);
        } else {
            a.at(0x102006ec); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(31,0x0,3);
            if (a.positive()) {
                a.at(0x10200704); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(28,0x0,0);
            }
            a.at(0x10200714);
            transition_rule_49(a);
        }
    }
}
void transition_rule_47(Call& a) {
    a.at(0x1020067c); a.load(5,31,0x0); a.value(3,0x55550000); a.load(4,28,0x0); a.offset(0,3,0x5556);
    a.load<2>(3,5,0x0); a.add(4,3,4); a.multiply_high(3,0,4); a.mask(0,3,0x1,false,1); a.add(0,3,0);
    a.multiply(0,0,0x3); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x102006dc);
        transition_rule_48(a);
    } else {
        a.at(0x102006ac); a.load<1>(0,25,0x5); a.value(3,0x2145); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
        a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8); a.store<1>(22,0x8,0);
        a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
    }
}
void transition_rule_46(Call& a) {
    a.at(0x10200644); a.compare(24,8069);
    if (!a.equal()) {
        a.at(0x102006dc);
        transition_rule_48(a);
    } else {
        a.at(0x1020064c); a.compare(23,8053);
        if (!a.equal()) {
            a.at(0x102006dc);
            transition_rule_48(a);
        } else {
            a.at(0x10200654); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(31,0x0,3);
            if (a.positive()) {
                a.at(0x1020066c); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(28,0x0,0);
            }
            a.at(0x1020067c);
            transition_rule_47(a);
        }
    }
}
void transition_rule_45(Call& a) {
    a.at(0x102005e4); a.load(5,31,0x0); a.value(3,0x55550000); a.load(4,28,0x0); a.offset(0,3,0x5556);
    a.load<2>(3,5,0x0); a.add(4,3,4); a.multiply_high(3,0,4); a.mask(0,3,0x1,false,1); a.add(0,3,0);
    a.multiply(0,0,0x3); a.subtract(0,0,4,true);
    if (a.equal()) {
        a.at(0x10200644);
        transition_rule_46(a);
    } else {
        a.at(0x10200614); a.load<1>(0,25,0x5); a.value(3,0x2144); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
        a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8); a.store<1>(22,0x8,0);
        a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
    }
}
void transition_rule_44(Call& a) {
    a.at(0x102005ac); a.compare(24,8068);
    if (!a.equal()) {
        a.at(0x10200644);
        transition_rule_46(a);
    } else {
        a.at(0x102005b4); a.compare(23,8053);
        if (!a.equal()) {
            a.at(0x10200644);
            transition_rule_46(a);
        } else {
            a.at(0x102005bc); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(31,0x0,3);
            if (a.positive()) {
                a.at(0x102005d4); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(28,0x0,0);
            }
            a.at(0x102005e4);
            transition_rule_45(a);
        }
    }
}
void transition_rule_43(Call& a) {
    a.at(0x1020056c); a.compare(24,8006);
    if (!a.equal()) {
        a.at(0x102005ac);
        transition_rule_44(a);
    } else {
        a.at(0x10200574); a.compare(23,8048);
        if (!a.equal()) {
            a.at(0x102005ac);
            transition_rule_44(a);
        } else {
            a.at(0x1020057c); a.load<1>(0,25,0x5); a.value(3,0x1f70); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_42(Call& a) {
    a.at(0x1020052c); a.compare(24,8032);
    if (!a.equal()) {
        a.at(0x1020056c);
        transition_rule_43(a);
    } else {
        a.at(0x10200534); a.compare(23,8047);
        if (!a.equal()) {
            a.at(0x1020056c);
            transition_rule_43(a);
        } else {
            a.at(0x1020053c); a.load<1>(0,25,0x5); a.value(3,0x1f6f); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_41(Call& a) {
    a.at(0x102004b4); a.compare(24,8225);
    if (a.less()) {
        a.at(0x1020052c);
        transition_rule_42(a);
    } else {
        a.at(0x102004bc); a.compare(24,8400);
        if (a.positive()) {
            a.at(0x1020052c);
            transition_rule_42(a);
        } else {
            a.at(0x102004c4); a.compare(23,8006);
            if (!a.equal()) {
                a.at(0x1020052c);
                transition_rule_42(a);
            } else {
                a.at(0x102004cc); a.load<1>(3,25,0x7); a.load<1>(0,22,0x5); a.sign8(3,3); a.sign8(0,0);
                a.compare_registers(3,0);
                if (!a.equal()) {
                    a.at(0x1020052c);
                    transition_rule_42(a);
                } else {
                    a.at(0x102004e4); a.load<1>(3,25,0x8); a.load<1>(0,22,0x6); a.sign8(3,3); a.sign8(0,0);
                    a.compare_registers(3,0);
                    if (!a.equal()) {
                        a.at(0x1020052c);
                        transition_rule_42(a);
                    } else {
                        a.at(0x102004fc); a.load<1>(0,25,0x5); a.move(3,24); a.store<1>(22,0x5,0);
                        a.load<1>(0,25,0x6); a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0);
                        a.load<1>(0,25,0x8); a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0);
                        a.jump(0x10201064);
                    }
                }
            }
        }
    }
}
void transition_rule_40(Call& a) {
    a.at(0x10200474); a.compare(24,8029);
    if (!a.equal()) {
        a.at(0x102004b4);
        transition_rule_41(a);
    } else {
        a.at(0x1020047c); a.compare(23,8047);
        if (!a.equal()) {
            a.at(0x102004b4);
            transition_rule_41(a);
        } else {
            a.at(0x10200484); a.load<1>(0,25,0x5); a.value(3,0x2143); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_39(Call& a) {
    a.at(0x10200424); a.compare(23,8006);
    if (!a.equal()) {
        a.at(0x10200474);
        transition_rule_40(a);
    } else {
        a.at(0x1020042c); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x10200474);
            transition_rule_40(a);
        } else {
            a.at(0x10200444); a.load<1>(0,25,0x5); a.move(3,24); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_38(Call& a) {
    a.at(0x1020040c); a.offset(0,24,-0x1f62); a.mask(0,0,0xffff,false,0); a.compare(0,1,false);
    if (a.nonpositive()) {
        a.at(0x10200424);
        transition_rule_39(a);
    } else {
        a.at(0x1020041c); a.compare(24,8528);
        if (!a.equal()) {
            a.at(0x10200474);
            transition_rule_40(a);
        } else {
            a.at(0x10200424);
            transition_rule_39(a);
        }
    }
}
void transition_rule_37(Call& a) {
    a.at(0x102003c4); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x1020040c);
        transition_rule_38(a);
    } else {
        a.at(0x102003dc); a.load<1>(0,25,0x5); a.move(3,23); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
        a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8); a.store<1>(22,0x8,0);
        a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
    }
}
void transition_rule_36(Call& a) {
    a.at(0x102003ac); a.compare(24,8007);
    if (!a.equal()) {
        a.at(0x1020040c);
        transition_rule_38(a);
    } else {
        a.at(0x102003b4); a.compare(23,8005);
        if (a.equal()) {
            a.at(0x102003c4);
            transition_rule_37(a);
        } else {
            a.at(0x102003bc); a.compare(23,8414);
            if (!a.equal()) {
                a.at(0x1020040c);
                transition_rule_38(a);
            } else {
                a.at(0x102003c4);
                transition_rule_37(a);
            }
        }
    }
}
void transition_rule_35(Call& a) {
    a.at(0x10200358); a.load(3,31,0x0); a.load(0,28,0x0); a.load<2>(3,3,0x0); a.add(0,3,0);
    a.mask(3,0,0x1,false,1); a.mask(0,0,0x1,false,0); a.combine(0,0,3,false,true); a.subtract(0,3,0,true);
    if (a.equal()) {
        a.at(0x102003ac);
        transition_rule_36(a);
    } else {
        a.at(0x1020037c); a.load<1>(0,25,0x5); a.value(3,0x1f43); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
        a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8); a.store<1>(22,0x8,0);
        a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
    }
}
void transition_rule_34(Call& a) {
    a.at(0x10200320); a.compare(24,8003);
    if (!a.equal()) {
        a.at(0x102003ac);
        transition_rule_36(a);
    } else {
        a.at(0x10200328); a.compare(23,8093);
        if (!a.equal()) {
            a.at(0x102003ac);
            transition_rule_36(a);
        } else {
            a.at(0x10200330); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(31,0x0,3);
            if (a.positive()) {
                a.at(0x10200348); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(28,0x0,0);
            }
            a.at(0x10200358);
            transition_rule_35(a);
        }
    }
}
void transition_rule_33(Call& a) {
    a.at(0x102002b4); a.load(3,31,0x0); a.load(0,28,0x0); a.load<2>(3,3,0x0); a.add(0,3,0);
    a.mask(3,0,0x1,false,1); a.mask(0,0,0x1,false,0); a.combine(0,0,3,false,true); a.subtract(0,3,0,true);
    if (a.equal()) {
        a.at(0x10200320);
        transition_rule_34(a);
    } else {
        a.at(0x102002d8); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x102010c0); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x10200320);
            transition_rule_34(a);
        } else {
            a.at(0x102002f0); a.load<1>(0,25,0x5); a.value(3,0x1f4a); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_32(Call& a) {
    a.at(0x1020027c); a.compare(24,8013);
    if (!a.equal()) {
        a.at(0x10200320);
        transition_rule_34(a);
    } else {
        a.at(0x10200284); a.compare(23,8010);
        if (!a.equal()) {
            a.at(0x10200320);
            transition_rule_34(a);
        } else {
            a.at(0x1020028c); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(31,0x0,3);
            if (a.positive()) {
                a.at(0x102002a4); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(28,0x0,0);
            }
            a.at(0x102002b4);
            transition_rule_33(a);
        }
    }
}
void transition_rule_31(Call& a) {
    a.at(0x10200224); a.compare(24,8007);
    if (!a.equal()) {
        a.at(0x1020027c);
        transition_rule_32(a);
    } else {
        a.at(0x1020022c); a.compare(23,8003);
        if (!a.equal()) {
            a.at(0x1020027c);
            transition_rule_32(a);
        } else {
            a.at(0x10200234); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
            if (a.equal()) {
                a.at(0x1020027c);
                transition_rule_32(a);
            } else {
                a.at(0x1020024c); a.load<1>(0,25,0x5); a.value(3,0x1f43); a.store<1>(22,0x5,0);
                a.load<1>(0,25,0x6); a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0);
                a.load<1>(0,25,0x8); a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_30(Call& a) {
    a.at(0x102001cc); a.compare(24,8007);
    if (!a.equal()) {
        a.at(0x10200224);
        transition_rule_31(a);
    } else {
        a.at(0x102001d4); a.compare(23,8004);
        if (!a.equal()) {
            a.at(0x10200224);
            transition_rule_31(a);
        } else {
            a.at(0x102001dc); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
            if (a.equal()) {
                a.at(0x10200224);
                transition_rule_31(a);
            } else {
                a.at(0x102001f4); a.load<1>(0,25,0x5); a.value(3,0x1f44); a.store<1>(22,0x5,0);
                a.load<1>(0,25,0x6); a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0);
                a.load<1>(0,25,0x8); a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_29(Call& a) {
    a.at(0x10200154); a.load(5,31,0x0); a.value(3,0x55550000); a.load(4,28,0x0); a.offset(0,3,0x5556);
    a.load<2>(3,5,0x0); a.add(4,3,4); a.multiply_high(3,0,4); a.mask(0,3,0x1,false,1); a.add(0,3,0);
    a.multiply(0,0,0x3); a.subtract(0,0,4,true);
    if (!a.equal()) {
        a.at(0x102001cc);
        transition_rule_30(a);
    } else {
        a.at(0x10200184); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x102001cc);
            transition_rule_30(a);
        } else {
            a.at(0x1020019c); a.load<1>(0,25,0x5); a.move(3,23); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_28(Call& a) {
    a.at(0x10200114); a.compare(24,8467);
    if (!a.equal()) {
        a.at(0x102001cc);
        transition_rule_30(a);
    } else {
        a.at(0x1020011c); a.compare(23,8070);
        if (a.less()) {
            a.at(0x102001cc);
            transition_rule_30(a);
        } else {
            a.at(0x10200124); a.compare(23,8126);
            if (a.positive()) {
                a.at(0x102001cc);
                transition_rule_30(a);
            } else {
                a.at(0x1020012c); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(31,0x0,3);
                if (a.positive()) {
                    a.at(0x10200144); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(28,0x0,0);
                }
                a.at(0x10200154);
                transition_rule_29(a);
            }
        }
    }
}
void transition_rule_27(Call& a) {
    a.at(0x102000d4); a.compare(24,8001);
    if (!a.equal()) {
        a.at(0x10200114);
        transition_rule_28(a);
    } else {
        a.at(0x102000dc); a.compare(23,8006);
        if (!a.equal()) {
            a.at(0x10200114);
            transition_rule_28(a);
        } else {
            a.at(0x102000e4); a.load<1>(0,25,0x5); a.value(3,0x1f46); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_26(Call& a) {
    a.at(0x10200094); a.compare(24,8510);
    if (!a.equal()) {
        a.at(0x102000d4);
        transition_rule_27(a);
    } else {
        a.at(0x1020009c); a.compare(23,8001);
        if (!a.equal()) {
            a.at(0x102000d4);
            transition_rule_27(a);
        } else {
            a.at(0x102000a4); a.load<1>(0,25,0x5); a.value(3,0x213e); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_25(Call& a) {
    a.at(0x10200044); a.compare(23,8006);
    if (!a.equal()) {
        a.at(0x10200094);
        transition_rule_26(a);
    } else {
        a.at(0x1020004c); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x10200094);
            transition_rule_26(a);
        } else {
            a.at(0x10200064); a.load<1>(0,25,0x5); a.value(3,0x1f46); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_24(Call& a) {
    a.at(0x10200034); a.compare(24,8510);
    if (a.equal()) {
        a.at(0x10200044);
        transition_rule_25(a);
    } else {
        a.at(0x1020003c); a.compare(24,8049);
        if (!a.equal()) {
            a.at(0x10200094);
            transition_rule_26(a);
        } else {
            a.at(0x10200044);
            transition_rule_25(a);
        }
    }
}
void transition_rule_23(Call& a) {
    a.at(0x101fffdc); a.compare(24,8167);
    if (!a.equal()) {
        a.at(0x10200034);
        transition_rule_24(a);
    } else {
        a.at(0x101fffe4); a.compare(23,8006);
        if (!a.equal()) {
            a.at(0x10200034);
            transition_rule_24(a);
        } else {
            a.at(0x101fffec); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
            if (a.equal()) {
                a.at(0x10200034);
                transition_rule_24(a);
            } else {
                a.at(0x10200004); a.load<1>(0,25,0x5); a.value(3,0x2142); a.store<1>(22,0x5,0);
                a.load<1>(0,25,0x6); a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0);
                a.load<1>(0,25,0x8); a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_22(Call& a) {
    a.at(0x101fff84); a.compare(24,8166);
    if (!a.equal()) {
        a.at(0x101fffdc);
        transition_rule_23(a);
    } else {
        a.at(0x101fff8c); a.compare(23,8006);
        if (!a.equal()) {
            a.at(0x101fffdc);
            transition_rule_23(a);
        } else {
            a.at(0x101fff94); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
            if (a.equal()) {
                a.at(0x101fffdc);
                transition_rule_23(a);
            } else {
                a.at(0x101fffac); a.load<1>(0,25,0x5); a.value(3,0x2142); a.store<1>(22,0x5,0);
                a.load<1>(0,25,0x6); a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0);
                a.load<1>(0,25,0x8); a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_21(Call& a) {
    a.at(0x101fff2c); a.compare(24,8463);
    if (!a.equal()) {
        a.at(0x101fff84);
        transition_rule_22(a);
    } else {
        a.at(0x101fff34); a.compare(23,8006);
        if (!a.equal()) {
            a.at(0x101fff84);
            transition_rule_22(a);
        } else {
            a.at(0x101fff3c); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
            if (a.equal()) {
                a.at(0x101fff84);
                transition_rule_22(a);
            } else {
                a.at(0x101fff54); a.load<1>(0,25,0x5); a.value(3,0x2141); a.store<1>(22,0x5,0);
                a.load<1>(0,25,0x6); a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0);
                a.load<1>(0,25,0x8); a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_20(Call& a) {
    a.at(0x101ffed4); a.compare(24,8465);
    if (!a.equal()) {
        a.at(0x101fff2c);
        transition_rule_21(a);
    } else {
        a.at(0x101ffedc); a.compare(23,8006);
        if (!a.equal()) {
            a.at(0x101fff2c);
            transition_rule_21(a);
        } else {
            a.at(0x101ffee4); a.move(3,21); a.move(4,25); a.move(5,22); a.call(0x10201080); a.sign8(0,3,true);
            if (a.equal()) {
                a.at(0x101fff2c);
                transition_rule_21(a);
            } else {
                a.at(0x101ffefc); a.load<1>(0,25,0x5); a.value(3,0x2140); a.store<1>(22,0x5,0);
                a.load<1>(0,25,0x6); a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0);
                a.load<1>(0,25,0x8); a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_19(Call& a) {
    a.at(0x101ffe7c); a.compare(24,8047);
    if (!a.equal()) {
        a.at(0x101ffed4);
        transition_rule_20(a);
    } else {
        a.at(0x101ffe84); a.compare(23,8006);
        if (!a.equal()) {
            a.at(0x101ffed4);
            transition_rule_20(a);
        } else {
            a.at(0x101ffe8c); a.load<1>(3,22,0x6); a.load<1>(0,25,0x6); a.sign8(3,3); a.sign8(0,0);
            a.compare_registers(3,0);
            if (a.equal()) {
                a.at(0x101ffed4);
                transition_rule_20(a);
            } else {
                a.at(0x101ffea4); a.sign8(0,26,true);
                if (a.equal()) {
                    a.at(0x101ffed4);
                    transition_rule_20(a);
                } else {
                    a.at(0x101ffeac); a.load<1>(0,22,0x5); a.value(3,0x2177); a.store<1>(22,0x7,0);
                    a.load<1>(0,22,0x6); a.store<1>(22,0x8,0); a.load<1>(0,25,0x5); a.store<1>(22,0x5,0);
                    a.load<1>(0,25,0x6); a.store<1>(22,0x6,0); a.jump(0x10201064);
                }
            }
        }
    }
}
void transition_rule_18(Call& a) {
    a.at(0x101ffe74); a.value(3,0xffffde88); a.jump(0x10201064);
}
void transition_rule_17(Call& a) {
    a.at(0x101ffe60); a.sign8(0,26); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x101ffe7c);
        transition_rule_19(a);
    } else {
        a.at(0x101ffe6c); a.sign8(0,4,true);
        if (!a.equal()) {
            a.at(0x101ffe7c);
            transition_rule_19(a);
        } else {
            a.at(0x101ffe74);
            transition_rule_18(a);
        }
    }
}
void transition_rule_16(Call& a) {
    a.at(0x101ffe34); a.load<1>(0,22,0x6); a.load<1>(4,25,0x6); a.sign8(0,0); a.sign8(3,4);
    a.compare_registers(0,3);
    if (a.equal()) {
        a.at(0x101ffe7c);
        transition_rule_19(a);
    } else {
        a.at(0x101ffe4c); a.sign8(0,26); a.compare(0,-1);
        if (!a.equal()) {
            a.at(0x101ffe60);
            transition_rule_17(a);
        } else {
            a.at(0x101ffe58); a.compare(3,1);
            if (a.equal()) {
                a.at(0x101ffe74);
                transition_rule_18(a);
            } else {
                a.at(0x101ffe60);
                transition_rule_17(a);
            }
        }
    }
}
void transition_rule_15(Call& a) {
    a.at(0x101ffe0c); a.compare(0,0);
    if (a.equal()) {
        a.at(0x101ffe7c);
        transition_rule_19(a);
    } else {
        a.at(0x101ffe14); a.compare(23,8006);
        if (a.equal()) {
            a.at(0x101ffe34);
            transition_rule_16(a);
        } else {
            a.at(0x101ffe1c); a.compare(23,8510);
            if (a.equal()) {
                a.at(0x101ffe34);
                transition_rule_16(a);
            } else {
                a.at(0x101ffe24); a.compare(23,8032);
                if (a.equal()) {
                    a.at(0x101ffe34);
                    transition_rule_16(a);
                } else {
                    a.at(0x101ffe2c); a.compare(23,8031);
                    if (!a.equal()) {
                        a.at(0x101ffe7c);
                        transition_rule_19(a);
                    } else {
                        a.at(0x101ffe34);
                        transition_rule_16(a);
                    }
                }
            }
        }
    }
}
void transition_rule_14(Call& a) {
    a.at(0x101ffdf4); a.compare(24,8225); a.value(0,0x0);
    if (a.at_least()) {
        a.at(0x101ffe00); a.compare(24,8400);
        if (a.positive()) {
        } else {
            a.at(0x101ffe08); a.value(0,0x1);
        }
    }
    a.at(0x101ffe0c);
    transition_rule_15(a);
}
void transition_rule_13(Call& a) {
    a.at(0x101ffdbc); a.load(5,31,0x0); a.value(3,0x55550000); a.load(4,28,0x0); a.offset(0,3,0x5556);
    a.load<2>(3,5,0x0); a.add(4,3,4); a.multiply_high(3,0,4); a.mask(0,3,0x1,false,1); a.add(0,3,0);
    a.multiply(0,0,0x3); a.subtract(0,0,4,true);
    if (!a.equal()) {
        a.at(0x101ffdf4);
        transition_rule_14(a);
    } else {
        a.at(0x101ffdec); a.value(3,0x2198); a.jump(0x10201064);
    }
}
void transition_rule_12(Call& a) {
    a.at(0x101ffd84); a.compare(24,8041);
    if (!a.equal()) {
        a.at(0x101ffdf4);
        transition_rule_14(a);
    } else {
        a.at(0x101ffd8c); a.compare(23,8093);
        if (!a.equal()) {
            a.at(0x101ffdf4);
            transition_rule_14(a);
        } else {
            a.at(0x101ffd94); a.load(3,31,0x0); a.load(0,30,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(31,0x0,3);
            if (a.positive()) {
                a.at(0x101ffdac); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(28,0x0,0);
            }
            a.at(0x101ffdbc);
            transition_rule_13(a);
        }
    }
}
void transition_rule_11(Call& a) {
    a.at(0x101ffd44); a.compare(24,8004);
    if (!a.equal()) {
        a.at(0x101ffd84);
        transition_rule_12(a);
    } else {
        a.at(0x101ffd4c); a.compare(23,8003);
        if (!a.equal()) {
            a.at(0x101ffd84);
            transition_rule_12(a);
        } else {
            a.at(0x101ffd54); a.load<1>(0,25,0x5); a.value(3,0x213f); a.store<1>(22,0x5,0); a.load<1>(0,25,0x6);
            a.store<1>(22,0x6,0); a.load<1>(0,25,0x7); a.store<1>(22,0x7,0); a.load<1>(0,25,0x8);
            a.store<1>(22,0x8,0); a.load(0,25,0xc); a.store(22,0xc,0); a.jump(0x10201064);
        }
    }
}
void transition_rule_10(Call& a) {
    a.at(0x101ffd1c); a.compare(23,8070);
    if (a.nonpositive()) {
        a.at(0x101ffd44);
        transition_rule_11(a);
    } else {
        a.at(0x101ffd24); a.compare(23,8126);
        if (a.at_least()) {
            a.at(0x101ffd44);
            transition_rule_11(a);
        } else {
            a.at(0x101ffd2c); a.compare(24,8000);
            if (!a.equal()) {
                a.at(0x101ffd44);
                transition_rule_11(a);
            } else {
                a.at(0x101ffd34); a.value(0,0x219b); a.value(3,0x0); a.store<2>(25,0x0,0); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_9(Call& a) {
    a.at(0x101ffcf0); a.load(4,31,0x0); a.value(3,0x0); a.load(0,28,0x0); a.load<2>(4,4,0x0); a.add(0,4,0);
    a.mask(4,0,0x1,false,1); a.mask(0,0,0x1,false,0); a.combine(0,0,4,false,true); a.subtract(0,4,0);
    a.store<2>(25,0x2,0); a.jump(0x10201064);
}
void transition_rule_8(Call& a) {
    a.at(0x101ffcb8); a.compare(24,8062);
    if (!a.equal()) {
        a.at(0x101ffd1c);
        transition_rule_10(a);
    } else {
        a.at(0x101ffcc0); a.value(0,0x219c); a.store<2>(25,0x0,0); a.load(3,31,0x0); a.load(0,30,0x0);
        a.offset(3,3,0x2); a.compare_registers(3,0,false); a.store(31,0x0,3);
        if (a.positive()) {
            a.at(0x101ffce0); a.store(31,0x0,29); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(28,0x0,0);
        }
        a.at(0x101ffcf0);
        transition_rule_9(a);
    }
}
void transition_rule_7(Call& a) {
    a.at(0x101ffca8); a.compare(23,8031);
    if (!a.equal()) {
        a.at(0x101ffd1c);
        transition_rule_10(a);
    } else {
        a.at(0x101ffcb0); a.sign8(0,26,true);
        if (a.equal()) {
            a.at(0x101ffd1c);
            transition_rule_10(a);
        } else {
            a.at(0x101ffcb8);
            transition_rule_8(a);
        }
    }
}
void transition_rule_6(Call& a) {
    a.at(0x101ffc98); a.compare(23,8070);
    if (a.nonpositive()) {
        a.at(0x101ffca8);
        transition_rule_7(a);
    } else {
        a.at(0x101ffca0); a.compare(23,8126);
        if (a.less()) {
            a.at(0x101ffcb8);
            transition_rule_8(a);
        } else {
            a.at(0x101ffca8);
            transition_rule_7(a);
        }
    }
}
void transition_rule_5(Call& a) {
    a.at(0x101ffc68); a.compare(24,8061);
    if (!a.equal()) {
        a.at(0x101ffc98);
        transition_rule_6(a);
    } else {
        a.at(0x101ffc70); a.load<1>(3,22,0x6); a.load<1>(0,25,0x6); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (a.equal()) {
            a.at(0x101ffc98);
            transition_rule_6(a);
        } else {
            a.at(0x101ffc88); a.compare(23,8006);
            if (!a.equal()) {
                a.at(0x101ffc98);
                transition_rule_6(a);
            } else {
                a.at(0x101ffc90); a.value(3,0xffffde63); a.jump(0x10201064);
            }
        }
    }
}
void transition_rule_4(Call& a) {
    a.at(0x101ffc60); a.value(3,0xffffe064); a.jump(0x10201064);
}
void transition_rule_3(Call& a) {
    a.at(0x101ffc48); a.compare(23,8082);
    if (!a.equal()) {
        a.at(0x101ffc68);
        transition_rule_5(a);
    } else {
        a.at(0x101ffc50); a.compare(24,8044);
        if (a.equal()) {
            a.at(0x101ffc60);
            transition_rule_4(a);
        } else {
            a.at(0x101ffc58); a.compare(24,8046);
            if (!a.equal()) {
                a.at(0x101ffc68);
                transition_rule_5(a);
            } else {
                a.at(0x101ffc60);
                transition_rule_4(a);
            }
        }
    }
}
void transition_rule_2(Call& a) {
    a.at(0x101ffc10); a.sign8(0,0); a.compare(23,8047); a.multiply(3,0,0x14); a.offset(25,3,0x30); a.add(25,21,25);
    a.load<2>(24,25,0x0,true);
    if (!a.equal()) {
        a.at(0x101ffc48);
        transition_rule_3(a);
    } else {
        a.at(0x101ffc2c); a.load<1>(0,22,0x6); a.sign8(3,27); a.sign8(0,0); a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x101ffc48);
            transition_rule_3(a);
        } else {
            a.at(0x101ffc40); a.value(3,0x21bb); a.jump(0x10201064);
        }
    }
}
void transition_rule_1(Call& a) {
    a.at(0x101ffbe4); a.load<2>(3,21,0x8,true); a.load<2>(0,21,0x6,true); a.compare_registers(3,0);
    if (!a.equal()) {
        a.at(0x101ffbfc); a.sign16(0,3,true); a.value(0,0xb);
        if (!a.equal()) {
            a.at(0x101ffc08); a.offset(0,3,-0x1); a.sign8(0,0);
        }
        a.at(0x101ffc10);
        transition_rule_2(a);
    } else {
        a.at(0x101ffbf4); a.value(3,0x0); a.jump(0x10201064);
    }
}
}
void select_transition(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101ffb70,dependencies,observer);
    a.at(0x101ffb70); a.link(); a.multiple(21,false,0); a.move(24,5,true); a.load(28,2,-0x7b90);
    a.load(29,2,-0x7b8c); a.move(22,4); a.load(30,2,-0x7b88); a.move(21,3); a.load(31,2,-0x7b84); a.save_link();
    a.frame(128); a.load<2>(23,4,0x0,true);
    if (a.equal()) {
        a.at(0x101ffbe0); a.value(26,0x0);
    } else {
        a.at(0x101ffba4); a.load<1>(27,24,0x7921); a.move(3,24); a.call(0x101fea10); a.load<1>(5,24,0x792f);
        a.move(26,3); a.load<1>(6,24,0x791c); a.move(3,24); a.value(4,0x0); a.call(0x101eea30);
        a.load<1>(5,24,0x792f); a.move(3,24); a.load<1>(6,24,0x791c); a.value(4,0x1); a.call(0x101eea30);
        a.jump(0x101ffbe4);
    }
    a.at(0x101ffbe4);
    transition_rule_1(a);
    a.at(0x10201064); a.pop(128); a.multiple(21,true,0); a.finish();
}
}
