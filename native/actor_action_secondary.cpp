#include "actor_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_action {
using Call = SemanticGuestCall;
namespace {
void secondary_phase_1(Call&,State&);
void secondary_phase_2(Call&,State&);
void secondary_phase_3(Call&,State&);
void secondary_phase_4(Call&,State&);
void secondary_phase_5(Call&,State&);
void secondary_phase_6(Call&,State&);
void secondary_phase_7(Call&,State&);
void secondary_phase_8(Call&,State&);
void secondary_phase_9(Call&,State&);
void secondary_phase_10(Call&,State&);
void secondary_phase_11(Call&,State&);
void secondary_phase_12(Call&,State&);
void secondary_phase_13(Call&,State&);
void secondary_phase_14(Call&,State&);
void secondary_phase_15(Call&,State&);
void secondary_phase_16(Call&,State&);
void secondary_phase_17(Call&,State&);
void secondary_phase_18(Call&,State&);
void secondary_phase_19(Call&,State&);
void secondary_phase_20(Call&,State&);
void secondary_phase_21(Call&,State&);
void secondary_phase_22(Call&,State&);
void secondary_phase_23(Call&,State&);
void secondary_phase_24(Call&,State&);
void secondary_phase_25(Call&,State&);
void secondary_phase_26(Call&,State&);
void secondary_phase_27(Call&,State&);
void secondary_phase_28(Call&,State&);
void secondary_phase_29(Call&,State&);
void secondary_phase_30(Call&,State&);
void secondary_phase_31(Call&,State&);
void secondary_phase_32(Call&,State&);
void secondary_phase_33(Call&,State&);
void secondary_phase_34(Call&,State&);
void secondary_phase_35(Call&,State&);
void secondary_phase_36(Call&,State&);
void secondary_phase_37(Call&,State&);
void secondary_phase_38(Call&,State&);
void secondary_phase_39(Call&,State&);
void secondary_phase_40(Call&,State&);
void secondary_phase_41(Call&,State&);
void secondary_phase_42(Call&,State&);
void secondary_phase_43(Call&,State&);
void secondary_phase_44(Call&,State&);
void secondary_phase_45(Call&,State&);
void secondary_phase_46(Call&,State&);
void secondary_phase_47(Call&,State&);
void secondary_phase_48(Call&,State&);
void secondary_phase_49(Call&,State&);
void secondary_phase_50(Call&,State&);
void secondary_phase_51(Call&,State&);
void secondary_phase_52(Call&,State&);
void secondary_phase_53(Call&,State&);
void secondary_phase_54(Call&,State&);
void secondary_phase_55(Call&,State&);
void secondary_phase_56(Call&,State&);
void secondary_phase_57(Call&,State&);
void secondary_phase_58(Call&,State&);
void secondary_phase_59(Call&,State&);
void secondary_phase_60(Call&,State&);
void secondary_phase_61(Call&,State&);
void secondary_phase_62(Call&,State&);
void secondary_phase_63(Call&,State&);
void secondary_phase_64(Call&,State&);
void secondary_phase_65(Call&,State&);
void secondary_phase_66(Call&,State&);
void secondary_phase_67(Call&,State&);
void secondary_phase_68(Call&,State&);
void secondary_phase_69(Call&,State&);
void secondary_phase_70(Call&,State&);
void secondary_phase_71(Call&,State&);
void secondary_phase_72(Call&,State&);
void secondary_phase_73(Call&,State&);
void secondary_phase_74(Call&,State&);
void secondary_phase_75(Call&,State&);
void secondary_phase_76(Call&,State&);
void secondary_phase_77(Call&,State&);
void secondary_phase_78(Call&,State&);
void secondary_phase_79(Call&,State&);
void secondary_phase_80(Call&,State&);
void secondary_phase_81(Call&,State&);
void secondary_phase_82(Call&,State&);
void secondary_phase_82(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102834dc); a.value(0,0x68); a.move(3,31); a.store<2>(31,0x19c,0); a.value(6,0xffffffff); a.value(7,0x0);
    a.value(8,0x0); a.load<1>(4,1,0x3b); a.load<1>(5,1,0x3a); a.call(0x1022e7e0); a.jump(0x10283528);
}
void secondary_phase_81(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10283460); a.load<1>(0,31,0x104); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x102834dc);
        secondary_phase_82(a,state);
    } else {
        a.at(0x1028346c); a.mask(0,27,0x4,true,0);
        if (!a.equal()) {
            a.at(0x102834dc);
            secondary_phase_82(a,state);
        } else {
            a.at(0x10283474); a.mask(0,27,0x2,true,0);
            if (!a.equal()) {
                a.at(0x102834dc);
                secondary_phase_82(a,state);
            } else {
                a.at(0x1028347c); a.load<1>(0,31,0x10a); a.sign8(0,0,true);
                if (a.equal()) {
                    a.at(0x102834dc);
                    secondary_phase_82(a,state);
                } else {
                    a.at(0x10283488); a.load(3,31,0x1a4); a.move(4,31); a.offset(5,1,0x39); a.offset(6,1,0x38);
                    a.value(7,0x4); a.call(0x101d8260); a.sign8(0,3,true);
                    if (a.equal()) {
                        a.at(0x102834dc);
                        secondary_phase_82(a,state);
                    } else {
                        a.at(0x102834a8); a.value(0,0x69); a.move(3,31); a.store<2>(31,0x19c,0);
                        a.value(6,0xffffffff); a.value(7,0x0); a.value(8,0x0); a.load<2>(4,31,0x36,true);
                        a.offset(0,4,0x7); a.store<2>(31,0x36,0); a.load<1>(4,1,0x39); a.load<1>(5,1,0x38);
                        a.call(0x1022e7e0); a.jump(0x10283528);
                    }
                }
            }
        }
    }
}
void secondary_phase_80(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102833f4); a.sign8(0,20); a.compare_registers(0,19);
    if (a.nonpositive()) {
        a.at(0x10283504); a.value(0,0x68); a.move(3,31); a.store<2>(31,0x19c,0); a.value(6,0xffffffff);
        a.value(7,0x0); a.value(8,0x0); a.load<1>(4,1,0x3b); a.load<1>(5,1,0x3a); a.call(0x1022e7e0);
    } else {
        a.at(0x10283400); a.load<1>(0,31,0x10a); a.sign8(0,0,true);
        if (a.equal()) {
            a.at(0x10283460);
            secondary_phase_81(a,state);
        } else {
            a.at(0x1028340c); a.load(3,31,0x1a4); a.move(4,31); a.offset(5,1,0x39); a.offset(6,1,0x38);
            a.value(7,0x4); a.call(0x101d8260); a.sign8(0,3,true);
            if (a.equal()) {
                a.at(0x10283460);
                secondary_phase_81(a,state);
            } else {
                a.at(0x1028342c); a.value(0,0x6a); a.move(3,31); a.store<2>(31,0x19c,0); a.value(6,0xffffffff);
                a.value(7,0x0); a.value(8,0x0); a.load<2>(4,31,0x36,true); a.offset(0,4,0x7);
                a.store<2>(31,0x36,0); a.load<1>(4,1,0x39); a.load<1>(5,1,0x38); a.call(0x1022e7e0);
                a.jump(0x10283528);
            }
        }
    }
}
void secondary_phase_79(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102833f0); a.move(19,0);
    secondary_phase_80(a,state);
}
void secondary_phase_78(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102832e0); a.exclusive_flags(3,21,0x80000000); a.value(0,0x43300000); a.store(1,0x68,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0x64,0); a.value(5,0x10000);
    a.floating_load(2,28,128); a.value(3,0x80010000); a.floating_load(1,1,100); a.offset(6,6,-0x7fff);
    a.floating_load(0,28,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,29,0x0);
    a.multiply(0,21,0xa); a.offset(3,3,-0x7fff); a.store(1,0x6c,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,28,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(19,4,7);
    a.exclusive_flags(4,19,0x80000000); a.store(1,0x70,4); a.multiply_high(3,3,0); a.floating_load(0,1,108);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,116); a.load(19,1,0x78);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,21,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(19,19,3);
    a.compare_registers(19,0);
    if (a.positive()) {
        a.at(0x102833bc); a.offset(19,21,-0x1); a.jump(0x102833f4);
    }
    a.at(0x102833f4);
    secondary_phase_80(a,state);
}
void secondary_phase_77(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102832c0); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(21,3);
    if (a.positive()) {
        a.at(0x102832e0);
        secondary_phase_78(a,state);
    } else {
        a.at(0x102832d0); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(21,3);
        if (a.at_least()) {
            a.at(0x102833c4); a.load(3,30,0x0); a.compare(21,0); a.load(0,29,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,21); a.product(0,0,21); a.subtract(19,0,3); a.negate(0,19);
            if (a.positive()) {
                a.at(0x102833ec); a.move(0,19);
            }
            a.at(0x102833f0);
            secondary_phase_79(a,state);
        } else {
            a.at(0x102832e0);
            secondary_phase_78(a,state);
        }
    }
}
void secondary_phase_76(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10283268); a.load(3,2,-0x51e0); a.floating_load(0,31,228,true); a.floating_load(1,3,92,true);
    a.load<1>(20,31,0x10c); a.floating_sum_single(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,100);
    a.load(21,1,0x68); a.compare(21,0);
    if (!a.equal()) {
        a.at(0x10283298); a.load(3,30,0x0); a.load(0,25,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x102832b0); a.store(30,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(29,0x0,0);
        }
        a.at(0x102832c0);
        secondary_phase_77(a,state);
    } else {
        a.at(0x10283290); a.value(19,0x0); a.jump(0x102833f4);
        secondary_phase_80(a,state);
    }
}
void secondary_phase_75(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10283240); a.value(0,0x76); a.move(3,31); a.store<2>(31,0x19c,0); a.value(6,0xffffffff); a.value(7,0x0);
    a.value(8,0x0); a.load<1>(4,31,0x105); a.load<1>(5,31,0x106); a.call(0x1022e7e0); a.jump(0x10283528);
}
void secondary_phase_74(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102831c4); a.load<1>(0,31,0x104); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10283240);
        secondary_phase_75(a,state);
    } else {
        a.at(0x102831d0); a.mask(0,27,0x4,true,0);
        if (!a.equal()) {
            a.at(0x10283240);
            secondary_phase_75(a,state);
        } else {
            a.at(0x102831d8); a.mask(0,27,0x2,true,0);
            if (!a.equal()) {
                a.at(0x10283240);
                secondary_phase_75(a,state);
            } else {
                a.at(0x102831e0); a.load<1>(0,31,0x10a); a.sign8(0,0,true);
                if (a.equal()) {
                    a.at(0x10283240);
                    secondary_phase_75(a,state);
                } else {
                    a.at(0x102831ec); a.load(3,31,0x1a4); a.move(4,31); a.offset(5,1,0x39); a.offset(6,1,0x38);
                    a.value(7,0x4); a.call(0x101d8260); a.sign8(0,3,true);
                    if (a.equal()) {
                        a.at(0x10283240);
                        secondary_phase_75(a,state);
                    } else {
                        a.at(0x1028320c); a.value(0,0x69); a.move(3,31); a.store<2>(31,0x19c,0);
                        a.value(6,0xffffffff); a.value(7,0x0); a.value(8,0x0); a.load<2>(4,31,0x36,true);
                        a.offset(0,4,0x7); a.store<2>(31,0x36,0); a.load<1>(4,1,0x39); a.load<1>(5,1,0x38);
                        a.call(0x1022e7e0); a.jump(0x10283528);
                    }
                }
            }
        }
    }
}
void secondary_phase_73(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028313c); a.value(0,0x76); a.move(3,31); a.store<2>(31,0x19c,0); a.value(6,0xffffffff); a.value(7,0x0);
    a.value(8,0x0); a.load<1>(4,31,0x105); a.load<1>(5,31,0x106); a.call(0x1022e7e0); a.jump(0x10283528);
}
void secondary_phase_72(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028311c); a.load<1>(0,31,0x10d); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x1028313c);
        secondary_phase_73(a,state);
    } else {
        a.at(0x10283128); a.load(3,31,0x1a4); a.move(4,31); a.call(0x101f0dd0); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x10283164); a.load<1>(0,31,0x10a); a.sign8(0,0,true);
            if (a.equal()) {
                a.at(0x102831c4);
                secondary_phase_74(a,state);
            } else {
                a.at(0x10283170); a.load(3,31,0x1a4); a.move(4,31); a.offset(5,1,0x39); a.offset(6,1,0x38);
                a.value(7,0x4); a.call(0x101d8260); a.sign8(0,3,true);
                if (a.equal()) {
                    a.at(0x102831c4);
                    secondary_phase_74(a,state);
                } else {
                    a.at(0x10283190); a.value(0,0x6a); a.move(3,31); a.store<2>(31,0x19c,0); a.value(6,0xffffffff);
                    a.value(7,0x0); a.value(8,0x0); a.load<2>(4,31,0x36,true); a.offset(0,4,0x7);
                    a.store<2>(31,0x36,0); a.load<1>(4,1,0x39); a.load<1>(5,1,0x38); a.call(0x1022e7e0);
                    a.jump(0x10283528);
                }
            }
        } else {
            a.at(0x1028313c);
            secondary_phase_73(a,state);
        }
    }
}
void secondary_phase_71(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102830ac); a.load<1>(3,31,0x105); a.load<1>(0,1,0x3b); a.sign8(3,3); a.sign8(0,0);
    a.compare_registers(3,0);
    if (!a.equal()) {
        a.at(0x10283268);
        secondary_phase_76(a,state);
    } else {
        a.at(0x102830c4); a.load<1>(3,31,0x106); a.load<1>(0,1,0x3a); a.sign8(3,3); a.sign8(0,0);
        a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x10283268);
            secondary_phase_76(a,state);
        } else {
            a.at(0x102830dc); a.sign16(0,27); a.compare(0,1);
            if (!a.equal()) {
                a.at(0x1028311c);
                secondary_phase_72(a,state);
            } else {
                a.at(0x102830e8); a.load<1>(0,31,0x10d); a.sign8(0,0,true);
                if (a.equal()) {
                    a.at(0x1028311c);
                    secondary_phase_72(a,state);
                } else {
                    a.at(0x102830f4); a.value(0,0x76); a.move(3,31); a.store<2>(31,0x19c,0); a.value(6,0xffffffff);
                    a.value(7,0x0); a.value(8,0x0); a.load<1>(4,31,0x105); a.load<1>(5,31,0x106);
                    a.call(0x1022e7e0); a.jump(0x10283528);
                }
            }
        }
    }
}
void secondary_phase_70(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10283058); a.load(5,30,0x0); a.value(3,0x55550000); a.sign8(0,20); a.load(4,29,0x0); a.load<2>(5,5,0x0);
    a.offset(3,3,0x5556); a.add(0,0,21); a.add(5,5,4); a.multiply_high(4,3,5); a.mask(3,4,0x1,false,1);
    a.add(3,4,3); a.multiply(3,3,0x3); a.subtract(3,3,5); a.compare_registers(0,3);
    if (a.nonpositive()) {
        a.at(0x102830ac);
        secondary_phase_71(a,state);
    } else {
        a.at(0x10283094); a.value(0,0x6b); a.move(3,31); a.store<2>(31,0x19c,0); a.value(4,0x0);
        a.call(0x10239e50); a.jump(0x10283528);
    }
}
void secondary_phase_69(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10283030); a.load(3,30,0x0); a.load(0,25,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(30,0x0,3);
    if (a.positive()) {
        a.at(0x10283048); a.store(30,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(29,0x0,0);
    }
    a.at(0x10283058);
    secondary_phase_70(a,state);
}
void secondary_phase_68(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028302c); a.move(21,0);
    secondary_phase_69(a,state);
}
void secondary_phase_67(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282ef0); a.sign8(3,19); a.value(0,0x43300000); a.multiply(3,3,0x5); a.store(1,0x64,0);
    a.load(7,30,0x0); a.value(6,0x80010000); a.value(4,0x43300000); a.floating_load(2,28,128);
    a.exclusive_flags(0,3,0x80000000); a.load<2>(8,7,0x0); a.store(1,0x68,0); a.value(5,0x10000); a.load(7,29,0x0);
    a.sign8(0,19); a.floating_load(1,1,100); a.value(3,0x80010000); a.floating_load(0,28,0); a.add(7,8,7);
    a.floating_subtract(2,1,2); a.offset(6,6,-0x7fff); a.store(1,0x6c,4); a.multiply_high(4,6,7);
    a.offset(5,5,-0x1); a.floating_load(1,28,128); a.floating_difference(2,2,0,true); a.offset(3,3,-0x7fff);
    a.add(4,4,7); a.shift_right(4,4,0xf); a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5);
    a.subtract(21,4,7); a.exclusive_flags(4,21,0x80000000); a.store(1,0x70,4); a.multiply(0,0,0x32);
    a.floating_load(0,1,108); a.multiply_high(3,3,0); a.floating_subtract(0,0,1); a.floating_product(0,2,0);
    a.add(0,3,0); a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.floating_integer(0,0); a.add(3,0,3);
    a.floating_store(0,1,116); a.load(21,1,0x78); a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5);
    a.offset(4,4,0x6667); a.sign8(0,19); a.multiply_high(3,4,3); a.shift_right(4,3,0x2); a.multiply(3,0,0x5);
    a.mask(0,4,0x1,false,1); a.add(0,4,0); a.add(21,21,0); a.offset(0,3,-0x1); a.compare_registers(21,0);
    if (a.positive()) {
        a.at(0x10282fe0); a.sign8(0,19); a.multiply(3,0,0x5); a.offset(21,3,-0x1); a.jump(0x10283030);
    }
    a.at(0x10283030);
    secondary_phase_69(a,state);
}
void secondary_phase_66(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282ec0); a.sign8(0,19); a.value(3,0x10000); a.multiply(0,0,0x5); a.offset(3,3,-0x1);
    a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x10282ef0);
        secondary_phase_67(a,state);
    } else {
        a.at(0x10282ed8); a.sign8(0,19); a.value(3,0xffff0000); a.multiply(0,0,0x5); a.offset(3,3,0x1);
        a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x10282ff0); a.load(4,30,0x0); a.sign8(3,19); a.sign8(0,19); a.load(5,29,0x0); a.load<2>(6,4,0x0);
            a.multiply(4,3,0x5); a.add(5,6,5); a.divide(3,5,4); a.product(3,3,4); a.multiply(0,0,0x5);
            a.subtract(21,3,5); a.compare(0,0); a.negate(0,21);
            if (a.positive()) {
                a.at(0x10283028); a.move(0,21);
            }
            a.at(0x1028302c);
            secondary_phase_68(a,state);
        } else {
            a.at(0x10282ef0);
            secondary_phase_67(a,state);
        }
    }
}
void secondary_phase_65(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282e64); a.load(3,31,0x1a4); a.move(4,31); a.call(0x101e4e10); a.move(19,3); a.sign8(0,19,true);
    if (a.less()) {
        a.at(0x102830ac);
        secondary_phase_71(a,state);
    } else {
        a.at(0x10282e7c); a.sign8(0,19); a.load<1>(20,31,0x10a); a.multiply(0,0,0x5); a.compare(0,0);
        if (!a.equal()) {
            a.at(0x10282e98); a.load(3,30,0x0); a.load(0,25,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10282eb0); a.store(30,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x10282ec0);
            secondary_phase_66(a,state);
        } else {
            a.at(0x10282e90); a.value(21,0x0); a.jump(0x10283030);
            secondary_phase_69(a,state);
        }
    }
}
void secondary_phase_64(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282e44); a.move(3,31); a.move(4,20); a.move(5,21); a.value(6,0xffffffff); a.value(7,0x0);
    a.value(8,0x0); a.call(0x1022e7e0); a.jump(0x10283528);
}
void secondary_phase_63(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282e2c); a.sign8(0,20); a.compare(0,6);
    if (a.at_least()) {
        a.at(0x10282e38); a.sign8(0,20,true);
        if (a.nonpositive()) {
        } else {
            a.at(0x10282e40); a.offset(20,20,-0x1);
        }
    }
    a.at(0x10282e44);
    secondary_phase_64(a,state);
}
void secondary_phase_62(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282e0c); a.sign8(0,20); a.compare(0,2);
    if (a.positive()) {
        a.at(0x10282e2c);
        secondary_phase_63(a,state);
    } else {
        a.at(0x10282e18); a.sign8(0,20); a.compare(0,8);
        if (a.at_least()) {
            a.at(0x10282e2c);
            secondary_phase_63(a,state);
        } else {
            a.at(0x10282e24); a.offset(20,20,0x1); a.jump(0x10282e44);
            secondary_phase_64(a,state);
        }
    }
}
void secondary_phase_61(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282df0); a.load<1>(0,31,0x27); a.compare(0,1);
    if (a.equal()) {
        a.at(0x10282dfc); a.sign8(0,21); a.compare(0,11);
        if (a.at_least()) {
        } else {
            a.at(0x10282e08); a.offset(21,21,0x1);
        }
    }
    a.at(0x10282e0c);
    secondary_phase_62(a,state);
}
void secondary_phase_60(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282d78); a.load(5,30,0x0); a.value(4,0x30c30000); a.value(0,0x43300000); a.load(3,31,0x1b0);
    a.load<2>(6,5,0x0); a.offset(4,4,0xc31); a.load(5,29,0x0); a.store(1,0x64,0); a.add(5,6,5);
    a.floating_load(2,3,176,true); a.multiply_high(0,4,5); a.floating_load(1,28,128); a.shift_right(0,0,0x2);
    a.mask(3,0,0x1,false,1); a.add(0,0,3); a.multiply(0,0,0x15); a.subtract(0,0,5);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0x68,0); a.floating_load(0,1,100);
    a.floating_subtract(0,0,1,true); a.floating_compare(2,0);
    if (a.at_least()) {
        a.at(0x10282e44);
        secondary_phase_64(a,state);
    } else {
        a.at(0x10282dd4); a.load<1>(0,31,0x27); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x10282df0);
            secondary_phase_61(a,state);
        } else {
            a.at(0x10282de0); a.sign8(0,21,true);
            if (a.nonpositive()) {
                a.at(0x10282df0);
                secondary_phase_61(a,state);
            } else {
                a.at(0x10282de8); a.offset(21,21,-0x1); a.jump(0x10282e0c);
                secondary_phase_62(a,state);
            }
        }
    }
}
void secondary_phase_59(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282cfc); a.value(0,0x105); a.store<2>(31,0x19c,0); a.load<1>(0,31,0x10d); a.load(4,31,0x1b0);
    a.load(3,31,0x1b0); a.sign8(0,0,true); a.load<1>(20,4,0x105); a.load<1>(21,3,0x106);
    if (a.equal()) {
        a.at(0x10282e64);
        secondary_phase_65(a,state);
    } else {
        a.at(0x10282d20); a.load<1>(0,31,0x27); a.move(5,21); a.load(3,31,0x1a4); a.sign8(0,0);
        a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27); a.sign8(4,0); a.call(0x101ee9f0); a.sign8(0,3,true);
        if (!a.equal()) {
            a.at(0x10282e44);
            secondary_phase_64(a,state);
        } else {
            a.at(0x10282d48); a.sign8(0,24,true);
            if (!a.equal()) {
                a.at(0x10282e44);
                secondary_phase_64(a,state);
            } else {
                a.at(0x10282d50); a.load(3,30,0x0); a.load(0,25,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(30,0x0,3);
                if (a.positive()) {
                    a.at(0x10282d68); a.store(30,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(29,0x0,0);
                }
                a.at(0x10282d78);
                secondary_phase_60(a,state);
            }
        }
    }
}
void secondary_phase_58(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282ca4); a.offset(3,23,0x8f); a.offset(6,1,0x44); a.offset(7,1,0x3c); a.value(4,0x0); a.value(5,0x0);
    a.call(0x10739890); a.call(0x10768810); a.move(5,3); a.move(3,22); a.offset(4,23,0x9c); a.offset(6,1,0x44);
    a.offset(7,1,0x3c); a.value(8,0x521a); a.call(0x1077b720); a.move(4,22); a.offset(3,23,0xa9); a.value(5,0x0);
    a.call(0x10769b60); a.load(3,2,-0x7bac); a.value(0,0x0); a.store<1>(3,0x0,0); a.jump(0x10282e64);
    secondary_phase_65(a,state);
}
void secondary_phase_57(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282c64); a.load(3,31,0x1a4); a.move(4,31); a.call(0x101e3d50); a.load(0,31,0x1b0); a.move(27,3);
    a.compare(0,0,false);
    if (a.equal()) {
        a.at(0x10282e64);
        secondary_phase_65(a,state);
    } else {
        a.at(0x10282c80); a.load(3,31,0x1b0); a.load<1>(0,3,0x19); a.sign8(0,0); a.compare(0,-1);
        if (a.equal()) {
            a.at(0x10282ca4);
            secondary_phase_58(a,state);
        } else {
            a.at(0x10282c94);
            if (a.at_least()) {
                a.at(0x10282cfc);
                secondary_phase_59(a,state);
            } else {
                a.at(0x10282c98); a.compare(0,-5);
                if (a.equal()) {
                    a.at(0x10282ca4);
                    secondary_phase_58(a,state);
                } else {
                    a.at(0x10282ca0); a.jump(0x10282cfc);
                    secondary_phase_59(a,state);
                }
            }
        }
    }
}
void secondary_phase_56(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282c14); a.load<1>(0,31,0x10a); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10282c20); a.load(3,31,0x1a4); a.load<1>(4,31,0x27); a.load<1>(5,1,0x3a); a.call(0x101e4ea0);
        a.move(19,3); a.sign8(0,19,true);
        if (a.less()) {
        } else {
            a.at(0x10282c3c); a.load<1>(0,31,0x27); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x10282c58); a.load<1>(0,1,0x3a); a.add(0,0,19); a.store<1>(1,0x3a,0);
            } else {
                a.at(0x10282c48); a.load<1>(0,1,0x3a); a.subtract(0,19,0); a.store<1>(1,0x3a,0);
                a.jump(0x10282c64);
            }
        }
    }
    a.at(0x10282c64);
    secondary_phase_57(a,state);
}
void secondary_phase_55(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282bd8); a.load<1>(0,1,0x3b); a.value(4,0xb); a.load<1>(5,31,0x27); a.sign8(3,0); a.sign8(0,5);
    a.leading_zeroes(5,0); a.offset(3,3,0x4); a.mask(0,3,0x1,false,1); a.add(0,0,3); a.mask(5,5,0x1,false,27);
    a.negate(3,5); a.shift_right(0,0,0x1); a.intersect(3,4,3); a.store<1>(1,0x3b,0); a.store<1>(1,0x3a,3);
    secondary_phase_56(a,state);
}
void secondary_phase_54(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282b88); a.load(3,30,0x0); a.value(5,0x55550000); a.load(0,29,0x0); a.offset(5,5,0x5556);
    a.load<2>(4,3,0x0); a.value(6,0x1); a.load(3,31,0x1a4); a.add(8,4,0); a.load<1>(4,31,0x106);
    a.multiply_high(7,5,8); a.load<1>(5,31,0x27); a.mask(0,7,0x1,false,1); a.add(0,7,0); a.multiply(0,0,0x3);
    a.subtract(19,0,8); a.call(0x101ee680); a.offset(0,19,0x1); a.sign8(3,3); a.compare_registers(3,0);
    if (a.at_least()) {
        a.at(0x10282c14);
        secondary_phase_56(a,state);
    } else {
        a.at(0x10282bd8);
        secondary_phase_55(a,state);
    }
}
void secondary_phase_53(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282b54); a.load<1>(0,31,0x10c); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10282c14);
        secondary_phase_56(a,state);
    } else {
        a.at(0x10282b60); a.load(3,30,0x0); a.load(0,25,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10282b78); a.store(30,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(29,0x0,0);
        }
        a.at(0x10282b88);
        secondary_phase_54(a,state);
    }
}
void secondary_phase_52(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282b1c); a.load<1>(0,31,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x10282c14);
        secondary_phase_56(a,state);
    } else {
        a.at(0x10282b28); a.load(3,31,0x1a4); a.load<1>(0,31,0x106); a.load<1>(3,3,0x791e); a.sign8(0,0);
        a.sign8(3,3); a.compare_registers(3,0);
        if (a.at_least()) {
            a.at(0x10282c14);
            secondary_phase_56(a,state);
        } else {
            a.at(0x10282b44); a.load<1>(0,31,0x106); a.sign8(0,0); a.compare(0,4);
            if (a.at_least()) {
                a.at(0x10282c14);
                secondary_phase_56(a,state);
            } else {
                a.at(0x10282b54);
                secondary_phase_53(a,state);
            }
        }
    }
}
void secondary_phase_51(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282ae4); a.load<1>(0,31,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10282b1c);
        secondary_phase_52(a,state);
    } else {
        a.at(0x10282af0); a.load(3,31,0x1a4); a.load<1>(0,31,0x106); a.load<1>(3,3,0x791e); a.sign8(0,0);
        a.sign8(3,3); a.compare_registers(3,0);
        if (a.nonpositive()) {
            a.at(0x10282b1c);
            secondary_phase_52(a,state);
        } else {
            a.at(0x10282b0c); a.load<1>(0,31,0x106); a.sign8(0,0); a.compare(0,7);
            if (a.positive()) {
                a.at(0x10282b54);
                secondary_phase_53(a,state);
            } else {
                a.at(0x10282b1c);
                secondary_phase_52(a,state);
            }
        }
    }
}
void secondary_phase_50(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282ad8); a.sign8(0,20); a.compare_registers(0,21);
    if (a.positive()) {
        a.at(0x10282bd8);
        secondary_phase_55(a,state);
    } else {
        a.at(0x10282ae4);
        secondary_phase_51(a,state);
    }
}
void secondary_phase_49(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282ad4); a.move(21,0);
    secondary_phase_50(a,state);
}
void secondary_phase_48(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102829b0); a.sign8(3,27); a.value(0,0x43300000); a.exclusive_flags(3,3,0x80000000); a.store(1,0x64,0);
    a.load(7,30,0x0); a.value(6,0x80010000); a.store(1,0x68,3); a.value(4,0x43300000); a.floating_load(2,28,128);
    a.value(5,0x10000); a.floating_load(1,1,100); a.sign8(0,27); a.load<2>(8,7,0x0); a.value(3,0x80010000);
    a.floating_subtract(1,1,2); a.floating_load(0,28,0); a.load(7,29,0x0); a.offset(6,6,-0x7fff);
    a.store(1,0x6c,4); a.offset(5,5,-0x1); a.floating_difference(2,1,0,true); a.add(7,8,7); a.multiply_high(4,6,7);
    a.floating_load(1,28,128); a.offset(3,3,-0x7fff); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(21,4,7);
    a.exclusive_flags(4,21,0x80000000); a.store(1,0x70,4); a.multiply(0,0,0xa); a.floating_load(0,1,108);
    a.multiply_high(3,3,0); a.floating_subtract(0,0,1); a.floating_product(0,2,0); a.add(0,3,0);
    a.shift_right(0,0,0xf); a.mask(3,0,0x1,false,1); a.floating_integer(0,0); a.add(3,0,3);
    a.floating_store(0,1,116); a.load(21,1,0x78); a.call(0x10226520); a.value(4,0x66660000); a.offset(0,3,0x5);
    a.offset(4,4,0x6667); a.sign8(3,27); a.multiply_high(4,4,0); a.offset(0,3,-0x1); a.shift_right(3,4,0x2);
    a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(21,21,3); a.compare_registers(21,0);
    if (a.positive()) {
        a.at(0x10282a98); a.sign8(3,27); a.offset(21,3,-0x1); a.jump(0x10282ad8);
    }
    a.at(0x10282ad8);
    secondary_phase_50(a,state);
}
void secondary_phase_47(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282988); a.value(3,0x10000); a.sign8(0,27); a.offset(3,3,-0x1); a.compare_registers(0,3);
    if (a.positive()) {
        a.at(0x102829b0);
        secondary_phase_48(a,state);
    } else {
        a.at(0x1028299c); a.value(3,0xffff0000); a.sign8(0,27); a.offset(3,3,0x1); a.compare_registers(0,3);
        if (a.at_least()) {
            a.at(0x10282aa4); a.load(5,30,0x0); a.sign8(3,27); a.load(4,29,0x0); a.sign8(0,27,true);
            a.load<2>(0,5,0x0); a.add(4,0,4); a.divide(0,4,3); a.product(0,0,3); a.subtract(21,0,4);
            a.negate(0,21);
            if (a.positive()) {
                a.at(0x10282ad0); a.move(0,21);
            }
            a.at(0x10282ad4);
            secondary_phase_49(a,state);
        } else {
            a.at(0x102829b0);
            secondary_phase_48(a,state);
        }
    }
}
void secondary_phase_46(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028293c); a.move(3,31); a.call(0x102864f0); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x10282ae4);
        secondary_phase_51(a,state);
    } else {
        a.at(0x1028294c); a.sign8(0,27,true); a.load<1>(20,31,0x10a);
        if (!a.equal()) {
            a.at(0x10282960); a.load(3,30,0x0); a.load(0,25,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10282978); a.store(30,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x10282988);
            secondary_phase_47(a,state);
        } else {
            a.at(0x10282958); a.value(21,0x0); a.jump(0x10282ad8);
            secondary_phase_50(a,state);
        }
    }
}
void secondary_phase_45(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102828e8); a.load(3,31,0x1a4); a.move(4,31); a.value(27,0x0); a.call(0x101e3d50);
    a.mask(0,3,0x200,true,0);
    if (a.equal()) {
        a.at(0x10282938); a.value(27,0x10);
    } else {
        a.at(0x10282900); a.load(3,31,0x1a4); a.move(4,31); a.call(0x101e3d50); a.mask(0,3,0x8,true,0);
        if (a.equal()) {
            a.at(0x1028291c); a.load(3,31,0x1a4); a.move(4,31); a.call(0x101e3d50); a.mask(0,3,0x4,true,0);
            if (a.equal()) {
            } else {
                a.at(0x10282930); a.value(27,0x14); a.jump(0x1028293c);
            }
        } else {
            a.at(0x10282914); a.value(27,0x12); a.jump(0x1028293c);
        }
    }
    a.at(0x1028293c);
    secondary_phase_46(a,state);
}
void secondary_phase_44(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102828bc); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x102828c4); a.load(3,31,0x1a4); a.load<1>(0,3,0x791d); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
        } else {
            a.at(0x102828d8); a.value(3,0x0); a.value(0,0x0); a.store<1>(1,0x3b,3); a.store<1>(1,0x3a,0);
        }
    }
    a.at(0x102828e8);
    secondary_phase_45(a,state);
}
void secondary_phase_43(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102828b8); a.sign8(0,3);
    secondary_phase_44(a,state);
}
void secondary_phase_42(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028287c); a.sign8(0,3); a.jump(0x102828bc);
    secondary_phase_44(a,state);
}
void secondary_phase_41(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102827f8); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x800,true,0);
    if (a.equal()) {
        a.at(0x102828e8);
        secondary_phase_45(a,state);
    } else {
        a.at(0x10282808); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x78,true,0);
        if (a.equal()) {
            a.at(0x102828e8);
            secondary_phase_45(a,state);
        } else {
            a.at(0x10282818); a.load<1>(0,31,0x27); a.load(3,31,0x1a4); a.sign8(0,0); a.load(4,31,0x1a4);
            a.leading_zeroes(0,0); a.load<1>(5,3,0x791d); a.mask(0,0,0x7ffffff,false,27); a.load<1>(4,4,0x791e);
            a.sign8(0,0); a.compare(0,1);
            if (!a.equal()) {
                a.at(0x10282884); a.sign8(0,5); a.value(3,0x0); a.compare(0,2);
                if (a.at_least()) {
                    a.at(0x10282894); a.sign8(0,5); a.compare(0,6);
                    if (a.positive()) {
                    } else {
                        a.at(0x102828a0); a.sign8(0,4,true);
                        if (a.less()) {
                        } else {
                            a.at(0x102828a8); a.sign8(0,4); a.compare(0,1);
                            if (a.positive()) {
                            } else {
                                a.at(0x102828b4); a.value(3,0x1);
                            }
                        }
                    }
                }
                a.at(0x102828b8);
                secondary_phase_43(a,state);
            } else {
                a.at(0x10282844); a.sign8(0,5); a.value(3,0x0); a.compare(0,2);
                if (a.at_least()) {
                    a.at(0x10282854); a.sign8(0,5); a.compare(0,6);
                    if (a.positive()) {
                    } else {
                        a.at(0x10282860); a.sign8(0,4); a.compare(0,10);
                        if (a.less()) {
                        } else {
                            a.at(0x1028286c); a.sign8(0,4); a.compare(0,11);
                            if (a.positive()) {
                            } else {
                                a.at(0x10282878); a.value(3,0x1);
                            }
                        }
                    }
                }
                a.at(0x1028287c);
                secondary_phase_42(a,state);
            }
        }
    }
}
void secondary_phase_40(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102827c8); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x102827d0); a.load(3,31,0x1a4); a.load<1>(0,3,0x791d); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
        } else {
            a.at(0x102827e4); a.value(3,0x8); a.value(0,0x0); a.store<1>(1,0x3b,3); a.store<1>(1,0x3a,0);
            a.jump(0x102828e8);
        }
    }
    a.at(0x102828e8);
    secondary_phase_45(a,state);
}
void secondary_phase_39(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102827c4); a.sign8(0,3);
    secondary_phase_40(a,state);
}
void secondary_phase_38(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282788); a.sign8(0,3); a.jump(0x102827c8);
    secondary_phase_40(a,state);
}
void secondary_phase_37(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102826d4); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x102826dc); a.load(3,31,0x1a4); a.load<1>(0,3,0x791d); a.sign8(0,0); a.compare(0,2);
        if (a.at_least()) {
        } else {
            a.at(0x102826f0); a.value(3,0x8); a.value(0,0xb); a.store<1>(1,0x3b,3); a.store<1>(1,0x3a,0);
            a.jump(0x102828e8);
        }
    }
    a.at(0x102828e8);
    secondary_phase_45(a,state);
}
void secondary_phase_36(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102826d0); a.sign8(0,3);
    secondary_phase_37(a,state);
}
void secondary_phase_35(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282694); a.sign8(0,3); a.jump(0x102826d4);
    secondary_phase_37(a,state);
}
void secondary_phase_34(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282610); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x800,true,0);
    if (a.equal()) {
        a.at(0x102828e8);
        secondary_phase_45(a,state);
    } else {
        a.at(0x10282620); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x78,true,0);
        if (a.equal()) {
            a.at(0x102828e8);
            secondary_phase_45(a,state);
        } else {
            a.at(0x10282630); a.load<1>(0,31,0x27); a.load(3,31,0x1a4); a.sign8(0,0); a.load(4,31,0x1a4);
            a.leading_zeroes(0,0); a.load<1>(5,3,0x791d); a.mask(0,0,0x7ffffff,false,27); a.load<1>(4,4,0x791e);
            a.sign8(0,0); a.compare(0,1);
            if (!a.equal()) {
                a.at(0x1028269c); a.sign8(0,5); a.value(3,0x0); a.compare(0,2);
                if (a.at_least()) {
                    a.at(0x102826ac); a.sign8(0,5); a.compare(0,6);
                    if (a.positive()) {
                    } else {
                        a.at(0x102826b8); a.sign8(0,4,true);
                        if (a.less()) {
                        } else {
                            a.at(0x102826c0); a.sign8(0,4); a.compare(0,1);
                            if (a.positive()) {
                            } else {
                                a.at(0x102826cc); a.value(3,0x1);
                            }
                        }
                    }
                }
                a.at(0x102826d0);
                secondary_phase_36(a,state);
            } else {
                a.at(0x1028265c); a.sign8(0,5); a.value(3,0x0); a.compare(0,2);
                if (a.at_least()) {
                    a.at(0x1028266c); a.sign8(0,5); a.compare(0,6);
                    if (a.positive()) {
                    } else {
                        a.at(0x10282678); a.sign8(0,4); a.compare(0,10);
                        if (a.less()) {
                        } else {
                            a.at(0x10282684); a.sign8(0,4); a.compare(0,11);
                            if (a.positive()) {
                            } else {
                                a.at(0x10282690); a.value(3,0x1);
                            }
                        }
                    }
                }
                a.at(0x10282694);
                secondary_phase_35(a,state);
            }
        }
    }
}
void secondary_phase_33(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102825e0); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x102825e8); a.load(3,31,0x1a4); a.load<1>(0,3,0x791d); a.sign8(0,0); a.compare(0,6);
        if (a.at_least()) {
        } else {
            a.at(0x102825fc); a.value(3,0x0); a.value(0,0xb); a.store<1>(1,0x3b,3); a.store<1>(1,0x3a,0);
            a.jump(0x102828e8);
        }
    }
    a.at(0x102828e8);
    secondary_phase_45(a,state);
}
void secondary_phase_32(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102825dc); a.sign8(0,3);
    secondary_phase_33(a,state);
}
void secondary_phase_31(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102825a0); a.sign8(0,3); a.jump(0x102825e0);
    secondary_phase_33(a,state);
}
void secondary_phase_30(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102824b4); a.load(3,31,0x1a4); a.move(4,31); a.offset(5,1,0x3b); a.offset(6,1,0x3a); a.value(7,0x1);
    a.call(0x101e28b0); a.load<1>(4,1,0x3b); a.move(3,31); a.call(0x102857c0); a.move(0,3); a.move(3,31);
    a.store<1>(1,0x3b,0); a.call(0x102864b0); a.value(5,0x0); a.value(0,0x4); a.intersect(6,4,5); a.value(4,0x0);
    a.intersect(5,3,0); a.value(0,0x0); a.combine(3,6,4,false,true); a.combine(0,5,0,false,true);
    a.combine(0,3,0,true);
    if (a.equal()) {
        a.at(0x102828e8);
        secondary_phase_45(a,state);
    } else {
        a.at(0x10282510); a.load<1>(0,31,0x27); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x10282704); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x80,true,0);
            if (a.equal()) {
                a.at(0x102827f8);
                secondary_phase_41(a,state);
            } else {
                a.at(0x10282714); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x78,true,0);
                if (a.equal()) {
                    a.at(0x102827f8);
                    secondary_phase_41(a,state);
                } else {
                    a.at(0x10282724); a.load<1>(0,31,0x27); a.load(3,31,0x1a4); a.sign8(0,0); a.load(4,31,0x1a4);
                    a.leading_zeroes(0,0); a.load<1>(5,3,0x791d); a.mask(0,0,0x7ffffff,false,27);
                    a.load<1>(4,4,0x791e); a.sign8(0,0); a.compare(0,1);
                    if (!a.equal()) {
                        a.at(0x10282790); a.sign8(0,5); a.value(3,0x0); a.compare(0,2);
                        if (a.at_least()) {
                            a.at(0x102827a0); a.sign8(0,5); a.compare(0,6);
                            if (a.positive()) {
                            } else {
                                a.at(0x102827ac); a.sign8(0,4,true);
                                if (a.less()) {
                                } else {
                                    a.at(0x102827b4); a.sign8(0,4); a.compare(0,1);
                                    if (a.positive()) {
                                    } else {
                                        a.at(0x102827c0); a.value(3,0x1);
                                    }
                                }
                            }
                        }
                        a.at(0x102827c4);
                        secondary_phase_39(a,state);
                    } else {
                        a.at(0x10282750); a.sign8(0,5); a.value(3,0x0); a.compare(0,2);
                        if (a.at_least()) {
                            a.at(0x10282760); a.sign8(0,5); a.compare(0,6);
                            if (a.positive()) {
                            } else {
                                a.at(0x1028276c); a.sign8(0,4); a.compare(0,10);
                                if (a.less()) {
                                } else {
                                    a.at(0x10282778); a.sign8(0,4); a.compare(0,11);
                                    if (a.positive()) {
                                    } else {
                                        a.at(0x10282784); a.value(3,0x1);
                                    }
                                }
                            }
                        }
                        a.at(0x10282788);
                        secondary_phase_38(a,state);
                    }
                }
            }
        } else {
            a.at(0x1028251c); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x80,true,0);
            if (a.equal()) {
                a.at(0x10282610);
                secondary_phase_34(a,state);
            } else {
                a.at(0x1028252c); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x78,true,0);
                if (a.equal()) {
                    a.at(0x10282610);
                    secondary_phase_34(a,state);
                } else {
                    a.at(0x1028253c); a.load<1>(0,31,0x27); a.load(3,31,0x1a4); a.sign8(0,0); a.load(4,31,0x1a4);
                    a.leading_zeroes(0,0); a.load<1>(5,3,0x791d); a.mask(0,0,0x7ffffff,false,27);
                    a.load<1>(4,4,0x791e); a.sign8(0,0); a.compare(0,1);
                    if (!a.equal()) {
                        a.at(0x102825a8); a.sign8(0,5); a.value(3,0x0); a.compare(0,2);
                        if (a.at_least()) {
                            a.at(0x102825b8); a.sign8(0,5); a.compare(0,6);
                            if (a.positive()) {
                            } else {
                                a.at(0x102825c4); a.sign8(0,4,true);
                                if (a.less()) {
                                } else {
                                    a.at(0x102825cc); a.sign8(0,4); a.compare(0,1);
                                    if (a.positive()) {
                                    } else {
                                        a.at(0x102825d8); a.value(3,0x1);
                                    }
                                }
                            }
                        }
                        a.at(0x102825dc);
                        secondary_phase_32(a,state);
                    } else {
                        a.at(0x10282568); a.sign8(0,5); a.value(3,0x0); a.compare(0,2);
                        if (a.at_least()) {
                            a.at(0x10282578); a.sign8(0,5); a.compare(0,6);
                            if (a.positive()) {
                            } else {
                                a.at(0x10282584); a.sign8(0,4); a.compare(0,10);
                                if (a.less()) {
                                } else {
                                    a.at(0x10282590); a.sign8(0,4); a.compare(0,11);
                                    if (a.positive()) {
                                    } else {
                                        a.at(0x1028259c); a.value(3,0x1);
                                    }
                                }
                            }
                        }
                        a.at(0x102825a0);
                        secondary_phase_31(a,state);
                    }
                }
            }
        }
    }
}
void secondary_phase_29(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102823d8); a.load<2>(0,31,0x36,true); a.value(7,0x43300000); a.load<1>(5,31,0x104);
    a.value(4,0x43300000); a.exclusive_flags(0,0,0x80000000); a.exclusive_flags(3,27,0x80000000);
    a.store(1,0x68,0); a.value(0,0x43300000); a.floating_load(2,28,128); a.sign8(6,5); a.store(1,0x64,7);
    a.sign8(5,20); a.product(5,6,5); a.floating_load(0,28,920); a.floating_load(1,1,100);
    a.floating_load(3,28,192); a.floating_subtract(1,1,2); a.store(1,0x6c,4); a.floating_difference(2,1,0,true);
    a.exclusive_flags(4,5,0x80000000); a.store(1,0x70,4); a.floating_load(1,28,128); a.floating_load(0,1,108);
    a.store(1,0x78,3); a.floating_subtract(3,3,2); a.store(1,0x74,0); a.floating_subtract(2,0,1,true);
    a.floating_load(1,28,128); a.floating_load(0,1,116); a.floating_round_single(3,3);
    a.floating_subtract(0,0,1,true); a.floating_product_single(1,3,2); a.floating_compare(1,0);
    if (a.nonpositive()) {
        a.at(0x102824b4);
        secondary_phase_30(a,state);
    } else {
        a.at(0x10282460); a.load(3,31,0x1a4); a.move(4,31); a.offset(5,1,0x39); a.offset(6,1,0x38); a.value(7,0x4);
        a.call(0x101d8260); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x102824b4);
            secondary_phase_30(a,state);
        } else {
            a.at(0x10282480); a.value(0,0x6a); a.move(3,31); a.store<2>(31,0x19c,0); a.value(6,0xffffffff);
            a.value(7,0x0); a.value(8,0x0); a.load<2>(4,31,0x36,true); a.offset(0,4,0x7); a.store<2>(31,0x36,0);
            a.load<1>(4,1,0x39); a.load<1>(5,1,0x38); a.call(0x1022e7e0); a.jump(0x10283528);
        }
    }
}
void secondary_phase_28(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102823d4); a.move(27,0);
    secondary_phase_29(a,state);
}
void secondary_phase_27(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102822c4); a.exclusive_flags(3,21,0x80000000); a.value(0,0x43300000); a.store(1,0x68,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0x64,0); a.value(5,0x10000);
    a.floating_load(2,28,128); a.value(3,0x80010000); a.floating_load(1,1,100); a.offset(6,6,-0x7fff);
    a.floating_load(0,28,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,29,0x0);
    a.multiply(0,21,0xa); a.offset(3,3,-0x7fff); a.store(1,0x6c,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,28,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(27,4,7);
    a.exclusive_flags(4,27,0x80000000); a.store(1,0x70,4); a.multiply_high(3,3,0); a.floating_load(0,1,108);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,116); a.load(27,1,0x78);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,21,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(27,27,3);
    a.compare_registers(27,0);
    if (a.positive()) {
        a.at(0x102823a0); a.offset(27,21,-0x1); a.jump(0x102823d8);
    }
    a.at(0x102823d8);
    secondary_phase_29(a,state);
}
void secondary_phase_26(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102822a4); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(21,3);
    if (a.positive()) {
        a.at(0x102822c4);
        secondary_phase_27(a,state);
    } else {
        a.at(0x102822b4); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(21,3);
        if (a.at_least()) {
            a.at(0x102823a8); a.load(3,30,0x0); a.compare(21,0); a.load(0,29,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,21); a.product(0,0,21); a.subtract(27,0,3); a.negate(0,27);
            if (a.positive()) {
                a.at(0x102823d0); a.move(0,27);
            }
            a.at(0x102823d4);
            secondary_phase_28(a,state);
        } else {
            a.at(0x102822c4);
            secondary_phase_27(a,state);
        }
    }
}
void secondary_phase_25(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028226c); a.compare(21,0);
    if (!a.equal()) {
        a.at(0x1028227c); a.load(3,30,0x0); a.load(0,25,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10282294); a.store(30,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(29,0x0,0);
        }
        a.at(0x102822a4);
        secondary_phase_26(a,state);
    } else {
        a.at(0x10282274); a.value(27,0x0); a.jump(0x102823d8);
        secondary_phase_29(a,state);
    }
}
void secondary_phase_24(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282250); a.load<1>(20,31,0x10a); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x40,true,0);
    a.value(21,0x32);
    if (!a.equal()) {
        a.at(0x10282268); a.value(21,0x14);
    }
    a.at(0x1028226c);
    secondary_phase_25(a,state);
}
void secondary_phase_23(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282174); a.load<2>(0,31,0x36,true); a.value(7,0x43300000); a.load<1>(5,31,0x104);
    a.value(4,0x43300000); a.exclusive_flags(0,0,0x80000000); a.exclusive_flags(3,20,0x80000000);
    a.store(1,0x68,0); a.value(0,0x43300000); a.floating_load(2,28,128); a.sign8(6,5); a.store(1,0x64,7);
    a.sign8(5,21); a.product(5,6,5); a.floating_load(0,28,920); a.floating_load(1,1,100);
    a.floating_load(3,28,192); a.floating_subtract(1,1,2); a.store(1,0x6c,4); a.floating_difference(2,1,0,true);
    a.exclusive_flags(4,5,0x80000000); a.store(1,0x70,4); a.floating_load(1,28,128); a.floating_load(0,1,108);
    a.store(1,0x78,3); a.floating_subtract(3,3,2); a.store(1,0x74,0); a.floating_subtract(2,0,1,true);
    a.floating_load(1,28,128); a.floating_load(0,1,116); a.floating_round_single(3,3);
    a.floating_subtract(0,0,1,true); a.floating_product_single(1,3,2); a.floating_compare(1,0);
    if (a.nonpositive()) {
        a.at(0x102824b4);
        secondary_phase_30(a,state);
    } else {
        a.at(0x102821fc); a.load(3,31,0x1a4); a.move(4,31); a.offset(5,1,0x39); a.offset(6,1,0x38); a.value(7,0x4);
        a.call(0x101d8260); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x102824b4);
            secondary_phase_30(a,state);
        } else {
            a.at(0x1028221c); a.value(0,0x6a); a.move(3,31); a.store<2>(31,0x19c,0); a.value(6,0xffffffff);
            a.value(7,0x0); a.value(8,0x0); a.load<2>(4,31,0x36,true); a.offset(0,4,0x7); a.store<2>(31,0x36,0);
            a.load<1>(4,1,0x39); a.load<1>(5,1,0x38); a.call(0x1022e7e0); a.jump(0x10283528);
        }
    }
}
void secondary_phase_22(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282170); a.move(20,0);
    secondary_phase_23(a,state);
}
void secondary_phase_21(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282060); a.exclusive_flags(3,27,0x80000000); a.value(0,0x43300000); a.store(1,0x68,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0x64,0); a.value(5,0x10000);
    a.floating_load(2,28,128); a.value(3,0x80010000); a.floating_load(1,1,100); a.offset(6,6,-0x7fff);
    a.floating_load(0,28,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,29,0x0);
    a.multiply(0,27,0xa); a.offset(3,3,-0x7fff); a.store(1,0x6c,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,28,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(20,4,7);
    a.exclusive_flags(4,20,0x80000000); a.store(1,0x70,4); a.multiply_high(3,3,0); a.floating_load(0,1,108);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,116); a.load(20,1,0x78);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,27,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(20,20,3);
    a.compare_registers(20,0);
    if (a.positive()) {
        a.at(0x1028213c); a.offset(20,27,-0x1); a.jump(0x10282174);
    }
    a.at(0x10282174);
    secondary_phase_23(a,state);
}
void secondary_phase_20(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282040); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(27,3);
    if (a.positive()) {
        a.at(0x10282060);
        secondary_phase_21(a,state);
    } else {
        a.at(0x10282050); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(27,3);
        if (a.at_least()) {
            a.at(0x10282144); a.load(3,30,0x0); a.compare(27,0); a.load(0,29,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,27); a.product(0,0,27); a.subtract(20,0,3); a.negate(0,20);
            if (a.positive()) {
                a.at(0x1028216c); a.move(0,20);
            }
            a.at(0x10282170);
            secondary_phase_22(a,state);
        } else {
            a.at(0x10282060);
            secondary_phase_21(a,state);
        }
    }
}
void secondary_phase_19(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10282008); a.compare(27,0);
    if (!a.equal()) {
        a.at(0x10282018); a.load(3,30,0x0); a.load(0,25,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10282030); a.store(30,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(29,0x0,0);
        }
        a.at(0x10282040);
        secondary_phase_20(a,state);
    } else {
        a.at(0x10282010); a.value(20,0x0); a.jump(0x10282174);
        secondary_phase_23(a,state);
    }
}
void secondary_phase_18(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281fec); a.load<1>(21,31,0x10a); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x40,true,0);
    a.value(27,0x32);
    if (!a.equal()) {
        a.at(0x10282004); a.value(27,0x14);
    }
    a.at(0x10282008);
    secondary_phase_19(a,state);
}
void secondary_phase_17(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281f6c); a.load(4,30,0x0); a.value(3,0x66660000); a.load(0,29,0x0); a.offset(3,3,0x6667);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x1); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x5); a.subtract(0,0,4); a.compare_registers(27,0);
    if (a.less()) {
        a.at(0x10281fec);
        secondary_phase_18(a,state);
    } else {
        a.at(0x10281fa4); a.load(3,31,0x1a4); a.move(4,31); a.offset(5,1,0x39); a.offset(6,1,0x38); a.value(7,0xc);
        a.call(0x101d8260); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x10281fec);
            secondary_phase_18(a,state);
        } else {
            a.at(0x10281fc4); a.value(0,0x6a); a.move(3,31); a.store<2>(31,0x19c,0); a.value(6,0xffffffff);
            a.value(7,0x0); a.value(8,0x0); a.load<1>(4,1,0x39); a.load<1>(5,1,0x38); a.call(0x1022e7e0);
            a.jump(0x10283528);
        }
    }
}
void secondary_phase_16(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281f44); a.load(3,30,0x0); a.load(0,25,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(30,0x0,3);
    if (a.positive()) {
        a.at(0x10281f5c); a.store(30,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(29,0x0,0);
    }
    a.at(0x10281f6c);
    secondary_phase_17(a,state);
}
void secondary_phase_15(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281f40); a.move(27,0);
    secondary_phase_16(a,state);
}
void secondary_phase_14(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281e30); a.exclusive_flags(3,21,0x80000000); a.value(0,0x43300000); a.store(1,0x78,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0x74,0); a.value(5,0x10000);
    a.floating_load(2,28,128); a.value(3,0x80010000); a.floating_load(1,1,116); a.offset(6,6,-0x7fff);
    a.floating_load(0,28,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,29,0x0);
    a.multiply(0,21,0xa); a.offset(3,3,-0x7fff); a.store(1,0x6c,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,28,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(27,4,7);
    a.exclusive_flags(4,27,0x80000000); a.store(1,0x70,4); a.multiply_high(3,3,0); a.floating_load(0,1,108);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,100); a.load(27,1,0x68);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,21,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(27,27,3);
    a.compare_registers(27,0);
    if (a.positive()) {
        a.at(0x10281f0c); a.offset(27,21,-0x1); a.jump(0x10281f44);
    }
    a.at(0x10281f44);
    secondary_phase_16(a,state);
}
void secondary_phase_13(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281e10); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(21,3);
    if (a.positive()) {
        a.at(0x10281e30);
        secondary_phase_14(a,state);
    } else {
        a.at(0x10281e20); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(21,3);
        if (a.at_least()) {
            a.at(0x10281f14); a.load(3,30,0x0); a.compare(21,0); a.load(0,29,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,21); a.product(0,0,21); a.subtract(27,0,3); a.negate(0,27);
            if (a.positive()) {
                a.at(0x10281f3c); a.move(0,27);
            }
            a.at(0x10281f40);
            secondary_phase_15(a,state);
        } else {
            a.at(0x10281e30);
            secondary_phase_14(a,state);
        }
    }
}
void secondary_phase_12(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281dc0); a.move(3,31); a.call(0x102862b0); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x10281fec);
        secondary_phase_18(a,state);
    } else {
        a.at(0x10281dd0); a.load(3,31,0x70); a.load<1>(21,3,0x1a); a.sign8(21,21,true);
        if (!a.equal()) {
            a.at(0x10281de8); a.load(3,30,0x0); a.load(0,25,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10281e00); a.store(30,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x10281e10);
            secondary_phase_13(a,state);
        } else {
            a.at(0x10281de0); a.value(27,0x0); a.jump(0x10281f44);
            secondary_phase_16(a,state);
        }
    }
}
void secondary_phase_11(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281d50); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10282250);
        secondary_phase_24(a,state);
    } else {
        a.at(0x10281d58); a.load(3,31,0x1a4); a.load(4,31,0x1a4); a.load<1>(3,3,0x791d); a.load<1>(0,31,0x105);
        a.load<1>(19,4,0x791e); a.sign8(3,3); a.sign8(0,0); a.subtract(3,3,0); a.call(0x10775540);
        a.load<1>(0,31,0x106); a.sign8(4,19); a.sign8(19,3); a.sign8(0,0); a.subtract(3,4,0); a.call(0x10775540);
        a.sign8(3,3); a.sign8(0,19); a.multiply(3,3,0x24); a.load(4,2,-0x6424); a.mask(0,0,0xfffffffc,false,2);
        a.floating_load(0,28,200); a.add(0,3,0); a.floating_indexed_single(1,4,0); a.floating_compare(1,0);
        a.compare_less_or_equal_bit();
        if (!a.equal()) {
            a.at(0x10282250);
            secondary_phase_24(a,state);
        } else {
            a.at(0x10281dc0);
            secondary_phase_12(a,state);
        }
    }
}
void secondary_phase_10(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281d44); a.value(0,0x1); a.jump(0x10281d50);
    secondary_phase_11(a,state);
}
void secondary_phase_9(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281d1c); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10281dc0);
        secondary_phase_12(a,state);
    } else {
        a.at(0x10281d24); a.load<1>(0,31,0x105); a.sign8(0,0); a.compare(0,2);
        if (a.less()) {
            a.at(0x10281d44);
            secondary_phase_10(a,state);
        } else {
            a.at(0x10281d34); a.load<1>(0,31,0x105); a.sign8(0,0); a.compare(0,6);
            if (a.nonpositive()) {
                a.at(0x10281d4c); a.value(0,0x0);
                secondary_phase_11(a,state);
            } else {
                a.at(0x10281d44);
                secondary_phase_10(a,state);
            }
        }
    }
}
void secondary_phase_8(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281d18); a.value(0,0x0);
    secondary_phase_9(a,state);
}
void secondary_phase_7(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281d10); a.value(0,0x1); a.jump(0x10281d1c);
    secondary_phase_9(a,state);
}
void secondary_phase_6(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281cf4); a.load<1>(0,31,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10281d18);
        secondary_phase_8(a,state);
    } else {
        a.at(0x10281d00); a.load<1>(0,31,0x106); a.sign8(0,0); a.compare(0,8);
        if (a.nonpositive()) {
            a.at(0x10281d18);
            secondary_phase_8(a,state);
        } else {
            a.at(0x10281d10);
            secondary_phase_7(a,state);
        }
    }
}
void secondary_phase_5(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281cd8); a.load<1>(0,31,0x27); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x10281cf4);
        secondary_phase_6(a,state);
    } else {
        a.at(0x10281ce4); a.load<1>(0,31,0x106); a.sign8(0,0); a.compare(0,3);
        if (a.less()) {
            a.at(0x10281d10);
            secondary_phase_7(a,state);
        } else {
            a.at(0x10281cf4);
            secondary_phase_6(a,state);
        }
    }
}
void secondary_phase_4(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281c70); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load(0,3,-0x2008); a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x10281cd8);
        secondary_phase_5(a,state);
    } else {
        a.at(0x10281c84); a.load(3,31,0x1a4); a.move(4,31); a.call(0x101f5280); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x10281cd8);
            secondary_phase_5(a,state);
        } else {
            a.at(0x10281c98); a.load(3,31,0x1a4); a.load<1>(0,3,0x1de); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x10281cd8);
                secondary_phase_5(a,state);
            } else {
                a.at(0x10281ca8); a.load(5,31,0x1a4); a.move(3,31); a.load(4,31,0x1a4); a.value(6,0x100);
                a.load<1>(5,5,0x791e); a.value(7,0x0); a.load<1>(4,4,0x791d); a.value(8,0x0); a.call(0x1022e7e0);
                a.value(0,0x100); a.store<2>(31,0x19c,0); a.jump(0x10283528);
            }
        }
    }
}
void secondary_phase_3(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281c04); a.load(5,30,0x0); a.value(3,0x66660000); a.load(4,29,0x0); a.offset(3,3,0x6667);
    a.load<2>(5,5,0x0); a.sign8(0,27); a.add(5,5,4); a.multiply_high(3,3,5); a.shift_right(3,3,0x3);
    a.mask(4,3,0x1,false,1); a.add(3,3,4); a.multiply(3,3,0x14); a.subtract(3,3,5); a.add_record(0,0,3);
    if (a.equal()) {
        a.at(0x10281c70);
        secondary_phase_4(a,state);
    } else {
        a.at(0x10281c40); a.load(5,31,0x1a4); a.move(3,31); a.load(4,31,0x1a4); a.value(6,0x100);
        a.load<1>(5,5,0x791e); a.value(7,0x0); a.load<1>(4,4,0x791d); a.value(8,0x0); a.call(0x1022e7e0);
        a.value(0,0x100); a.store<2>(31,0x19c,0); a.jump(0x10283528);
    }
}
void secondary_phase_2(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281bb0); a.load(3,31,0x1a4); a.load<1>(0,3,0x7924); a.sign8(0,0); a.compare(0,-1);
    if (!a.equal()) {
        a.at(0x10281cd8);
        secondary_phase_5(a,state);
    } else {
        a.at(0x10281bc4); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load(0,3,-0x2008);
        a.compare_registers(0,31,false);
        if (!a.equal()) {
            a.at(0x10281c70);
            secondary_phase_4(a,state);
        } else {
            a.at(0x10281bd8); a.load(3,30,0x0); a.load(0,25,0x0); a.offset(3,3,0x2); a.load<1>(27,31,0x10a);
            a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10281bf4); a.store(30,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x10281c04);
            secondary_phase_3(a,state);
        }
    }
}
void secondary_phase_1(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281b50); a.value(0,0xffffffff); a.store<2>(31,0x19c,0); a.load(3,31,0x1a4); a.load<1>(0,3,0x791f);
    a.sign8(0,0); a.compare(0,-1);
    if (!a.equal()) {
        a.at(0x10281bb0);
        secondary_phase_2(a,state);
    } else {
        a.at(0x10281b6c); a.load(3,31,0x1a4); a.load<1>(0,3,0x7924); a.sign8(0,0); a.compare(0,-1);
        if (a.equal()) {
            a.at(0x10281bb0);
            secondary_phase_2(a,state);
        } else {
            a.at(0x10281b80); a.load(3,31,0x1a4); a.move(4,31); a.offset(5,1,0x39); a.offset(6,1,0x38);
            a.call(0x101e25a0); a.value(0,0x68); a.move(3,31); a.store<2>(31,0x19c,0); a.load<1>(4,1,0x39);
            a.load<1>(5,1,0x38); a.call(0x102374f0); a.jump(0x10283528);
        }
    }
}
}
void select_secondary_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10281af0,dependencies,observer);
    a.at(0x10281af0); a.link(); a.multiple(19,false,0); a.move(31,3); a.load(22,2,-0x7ba8); a.load(28,2,-0x51e8);
    a.value(24,0x0); a.load(29,2,-0x7b90); a.load(26,2,-0x7b8c); a.load(25,2,-0x7b88); a.load(30,2,-0x7b84);
    a.load(23,2,-0x51e4); a.save_link(); a.frame(192); a.load<1>(0,3,0x27); a.load(4,3,0x1a4); a.sign8(0,0);
    a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,4,3); a.load<1>(4,31,0x19); a.call(0x101149f0);
    a.mask(0,3,0x4,true,0);
    if (!a.equal()) {
        a.at(0x10281b4c); a.value(24,0x1);
    }
    a.at(0x10281b50);
    secondary_phase_1(a,state);
    a.at(0x10283528); a.load(0,1,0xc8); a.offset(1,1,0xc0); a.restore_link(); a.multiple(19,true,0); a.finish();
}
}
