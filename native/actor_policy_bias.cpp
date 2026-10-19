#include "actor_policy.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_policy {
using Call = SemanticGuestCall;
namespace {
void bias_phase_1(Call&,State&);
void bias_phase_2(Call&,State&);
void bias_phase_3(Call&,State&);
void bias_phase_4(Call&,State&);
void bias_phase_5(Call&,State&);
void bias_phase_6(Call&,State&);
void bias_phase_7(Call&,State&);
void bias_phase_8(Call&,State&);
void bias_phase_9(Call&,State&);
void bias_phase_10(Call&,State&);
void bias_phase_11(Call&,State&);
void bias_phase_12(Call&,State&);
void bias_phase_13(Call&,State&);
void bias_phase_14(Call&,State&);
void bias_phase_15(Call&,State&);
void bias_phase_16(Call&,State&);
void bias_phase_17(Call&,State&);
void bias_phase_18(Call&,State&);
void bias_phase_19(Call&,State&);
void bias_phase_20(Call&,State&);
void bias_phase_21(Call&,State&);
void bias_phase_22(Call&,State&);
void bias_phase_23(Call&,State&);
void bias_phase_24(Call&,State&);
void bias_phase_25(Call&,State&);
void bias_phase_25(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285eac); a.sign8(3,24);
}
void bias_phase_24(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285e90); a.value(3,0x55550000); a.sign8(0,28); a.offset(3,3,0x5556); a.multiply_high(3,3,0);
    a.mask(0,3,0x1,false,1); a.add(0,3,0); a.sign8(24,0);
    bias_phase_25(a,state);
}
void bias_phase_23(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285e88); a.sign8(0,24); a.add(28,28,0);
    bias_phase_24(a,state);
}
void bias_phase_22(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285e7c); a.sign8(0,3); a.add(28,28,0); a.jump(0x10285e90);
    bias_phase_24(a,state);
}
void bias_phase_21(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285e30); a.load(4,31,0x0); a.value(3,0x63e70000); a.load(0,30,0x0); a.offset(3,3,0x63f);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x4); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x29); a.subtract(0,0,4); a.compare_registers(22,0);
    if (a.nonpositive()) {
        a.at(0x10285e88);
        bias_phase_23(a,state);
    } else {
        a.at(0x10285e68); a.load<1>(0,29,0x27); a.value(3,0x7); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x10285e78); a.value(3,0x1);
        }
        a.at(0x10285e7c);
        bias_phase_22(a,state);
    }
}
void bias_phase_20(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285e08); a.load(3,31,0x0); a.load(0,25,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(31,0x0,3);
    if (a.positive()) {
        a.at(0x10285e20); a.store(31,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(30,0x0,0);
    }
    a.at(0x10285e30);
    bias_phase_21(a,state);
}
void bias_phase_19(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285e04); a.move(22,0);
    bias_phase_20(a,state);
}
void bias_phase_18(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285cf4); a.exclusive_flags(3,23,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
    a.value(6,0x80010000); a.load(7,31,0x0); a.value(4,0x43300000); a.store(1,0x38,0); a.value(5,0x10000);
    a.floating_load(2,27,128); a.value(3,0x80010000); a.floating_load(1,1,56); a.offset(6,6,-0x7fff);
    a.floating_load(0,27,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,30,0x0);
    a.multiply(0,23,0xa); a.offset(3,3,-0x7fff); a.store(1,0x40,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,27,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(22,4,7);
    a.exclusive_flags(4,22,0x80000000); a.store(1,0x44,4); a.multiply_high(3,3,0); a.floating_load(0,1,64);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,72); a.load(22,1,0x4c);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,23,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(22,22,3);
    a.compare_registers(22,0);
    if (a.positive()) {
        a.at(0x10285dd0); a.offset(22,23,-0x1); a.jump(0x10285e08);
    }
    a.at(0x10285e08);
    bias_phase_20(a,state);
}
void bias_phase_17(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285cd4); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(23,3);
    if (a.positive()) {
        a.at(0x10285cf4);
        bias_phase_18(a,state);
    } else {
        a.at(0x10285ce4); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(23,3);
        if (a.at_least()) {
            a.at(0x10285dd8); a.load(3,31,0x0); a.compare(23,0); a.load(0,30,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,23); a.product(0,0,23); a.subtract(22,0,3); a.negate(0,22);
            if (a.positive()) {
                a.at(0x10285e00); a.move(0,22);
            }
            a.at(0x10285e04);
            bias_phase_19(a,state);
        } else {
            a.at(0x10285cf4);
            bias_phase_18(a,state);
        }
    }
}
void bias_phase_16(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285c84); a.move(3,29); a.call(0x10289560); a.mask(0,3,0x800,true,0);
    if (!a.equal()) {
        a.at(0x10285e88);
        bias_phase_23(a,state);
    } else {
        a.at(0x10285c94); a.load(3,29,0x70); a.load<1>(23,3,0x17); a.sign8(23,23,true);
        if (!a.equal()) {
            a.at(0x10285cac); a.load(3,31,0x0); a.load(0,25,0x0); a.offset(3,3,0x2);
            a.compare_registers(3,0,false); a.store(31,0x0,3);
            if (a.positive()) {
                a.at(0x10285cc4); a.store(31,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(30,0x0,0);
            }
            a.at(0x10285cd4);
            bias_phase_17(a,state);
        } else {
            a.at(0x10285ca4); a.value(22,0x0); a.jump(0x10285e08);
            bias_phase_20(a,state);
        }
    }
}
void bias_phase_15(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285c7c); a.sign8(0,24); a.add(28,28,0);
    bias_phase_16(a,state);
}
void bias_phase_14(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285c3c); a.load(4,31,0x0); a.value(3,0x63e70000); a.load(0,30,0x0); a.offset(3,3,0x63f);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x4); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x29); a.subtract(0,0,4); a.compare_registers(23,0);
    if (a.nonpositive()) {
        a.at(0x10285c7c);
        bias_phase_15(a,state);
    } else {
        a.at(0x10285c74); a.offset(28,28,0x4); a.jump(0x10285c84);
        bias_phase_16(a,state);
    }
}
void bias_phase_13(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285c14); a.load(3,31,0x0); a.load(0,25,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(31,0x0,3);
    if (a.positive()) {
        a.at(0x10285c2c); a.store(31,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(30,0x0,0);
    }
    a.at(0x10285c3c);
    bias_phase_14(a,state);
}
void bias_phase_12(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285c10); a.move(23,0);
    bias_phase_13(a,state);
}
void bias_phase_11(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285b00); a.exclusive_flags(3,22,0x80000000); a.value(0,0x43300000); a.store(1,0x3c,3);
    a.value(6,0x80010000); a.load(7,31,0x0); a.value(4,0x43300000); a.store(1,0x38,0); a.value(5,0x10000);
    a.floating_load(2,27,128); a.value(3,0x80010000); a.floating_load(1,1,56); a.offset(6,6,-0x7fff);
    a.floating_load(0,27,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,30,0x0);
    a.multiply(0,22,0xa); a.offset(3,3,-0x7fff); a.store(1,0x40,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,27,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(23,4,7);
    a.exclusive_flags(4,23,0x80000000); a.store(1,0x44,4); a.multiply_high(3,3,0); a.floating_load(0,1,64);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,72); a.load(23,1,0x4c);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,22,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(23,23,3);
    a.compare_registers(23,0);
    if (a.positive()) {
        a.at(0x10285bdc); a.offset(23,22,-0x1); a.jump(0x10285c14);
    }
    a.at(0x10285c14);
    bias_phase_13(a,state);
}
void bias_phase_10(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285ae0); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(22,3);
    if (a.positive()) {
        a.at(0x10285b00);
        bias_phase_11(a,state);
    } else {
        a.at(0x10285af0); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(22,3);
        if (a.at_least()) {
            a.at(0x10285be4); a.load(3,31,0x0); a.compare(22,0); a.load(0,30,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,22); a.product(0,0,22); a.subtract(23,0,3); a.negate(0,23);
            if (a.positive()) {
                a.at(0x10285c0c); a.move(0,23);
            }
            a.at(0x10285c10);
            bias_phase_12(a,state);
        } else {
            a.at(0x10285b00);
            bias_phase_11(a,state);
        }
    }
}
void bias_phase_9(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285a70); a.move(3,29); a.call(0x10289560); a.mask(0,3,0x200,true,0);
    if (!a.equal()) {
        a.at(0x10285c7c);
        bias_phase_15(a,state);
    } else {
        a.at(0x10285a80); a.move(3,29); a.call(0x10289560); a.mask(0,3,0x100,true,0);
        if (!a.equal()) {
            a.at(0x10285c7c);
            bias_phase_15(a,state);
        } else {
            a.at(0x10285a90); a.move(3,29); a.call(0x10289560); a.mask(0,3,0x400,true,0);
            if (!a.equal()) {
                a.at(0x10285c7c);
                bias_phase_15(a,state);
            } else {
                a.at(0x10285aa0); a.load(3,29,0x70); a.load<1>(22,3,0x19); a.sign8(22,22,true);
                if (!a.equal()) {
                    a.at(0x10285ab8); a.load(3,31,0x0); a.load(0,25,0x0); a.offset(3,3,0x2);
                    a.compare_registers(3,0,false); a.store(31,0x0,3);
                    if (a.positive()) {
                        a.at(0x10285ad0); a.store(31,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                        a.store(30,0x0,0);
                    }
                    a.at(0x10285ae0);
                    bias_phase_10(a,state);
                } else {
                    a.at(0x10285ab0); a.value(23,0x0); a.jump(0x10285c14);
                    bias_phase_13(a,state);
                }
            }
        }
    }
}
void bias_phase_8(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285a6c); a.sign8(28,24);
    bias_phase_9(a,state);
}
void bias_phase_7(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285a64); a.sign8(28,3); a.jump(0x10285a70);
    bias_phase_9(a,state);
}
void bias_phase_6(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10285a18); a.load(4,31,0x0); a.value(3,0x63e70000); a.load(0,30,0x0); a.offset(3,3,0x63f);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x4); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x29); a.subtract(0,0,4); a.compare_registers(28,0);
    if (a.nonpositive()) {
        a.at(0x10285a6c);
        bias_phase_8(a,state);
    } else {
        a.at(0x10285a50); a.load<1>(0,29,0x27); a.value(3,0x1); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x10285a60); a.value(3,0x7);
        }
        a.at(0x10285a64);
        bias_phase_7(a,state);
    }
}
void bias_phase_5(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102859f0); a.load(3,31,0x0); a.load(0,25,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(31,0x0,3);
    if (a.positive()) {
        a.at(0x10285a08); a.store(31,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(30,0x0,0);
    }
    a.at(0x10285a18);
    bias_phase_6(a,state);
}
void bias_phase_4(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102859ec); a.move(28,0);
    bias_phase_5(a,state);
}
void bias_phase_3(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102858dc); a.exclusive_flags(3,23,0x80000000); a.value(0,0x43300000); a.store(1,0x4c,3);
    a.value(6,0x80010000); a.load(7,31,0x0); a.value(4,0x43300000); a.store(1,0x48,0); a.value(5,0x10000);
    a.floating_load(2,27,128); a.value(3,0x80010000); a.floating_load(1,1,72); a.offset(6,6,-0x7fff);
    a.floating_load(0,27,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,30,0x0);
    a.multiply(0,23,0xa); a.offset(3,3,-0x7fff); a.store(1,0x40,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,27,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(28,4,7);
    a.exclusive_flags(4,28,0x80000000); a.store(1,0x44,4); a.multiply_high(3,3,0); a.floating_load(0,1,64);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,56); a.load(28,1,0x3c);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,23,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(28,28,3);
    a.compare_registers(28,0);
    if (a.positive()) {
        a.at(0x102859b8); a.offset(28,23,-0x1); a.jump(0x102859f0);
    }
    a.at(0x102859f0);
    bias_phase_5(a,state);
}
void bias_phase_2(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102858bc); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(23,3);
    if (a.positive()) {
        a.at(0x102858dc);
        bias_phase_3(a,state);
    } else {
        a.at(0x102858cc); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(23,3);
        if (a.at_least()) {
            a.at(0x102859c0); a.load(3,31,0x0); a.compare(23,0); a.load(0,30,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,23); a.product(0,0,23); a.subtract(28,0,3); a.negate(0,28);
            if (a.positive()) {
                a.at(0x102859e8); a.move(0,28);
            }
            a.at(0x102859ec);
            bias_phase_4(a,state);
        } else {
            a.at(0x102858dc);
            bias_phase_3(a,state);
        }
    }
}
void bias_phase_1(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1028582c); a.load(5,31,0x0); a.value(3,0x30c30000); a.sign8(0,22); a.load(4,30,0x0); a.load<2>(5,5,0x0);
    a.offset(3,3,0xc31); a.mask(0,0,0xfffffffe,false,1); a.add(5,5,4); a.multiply_high(3,3,5);
    a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.multiply(3,3,0x15); a.subtract(3,3,5);
    a.compare_registers(0,3);
    if (a.at_least()) {
        a.at(0x10285eac);
        bias_phase_25(a,state);
    } else {
        a.at(0x1028586c); a.move(3,29); a.call(0x10289560); a.mask(0,3,0x80,true,0);
        if (!a.equal()) {
            a.at(0x10285a6c);
            bias_phase_8(a,state);
        } else {
            a.at(0x1028587c); a.load(3,29,0x70); a.load<1>(23,3,0x18); a.sign8(23,23,true);
            if (!a.equal()) {
                a.at(0x10285894); a.load(3,31,0x0); a.load(0,25,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(31,0x0,3);
                if (a.positive()) {
                    a.at(0x102858ac); a.store(31,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(30,0x0,0);
                }
                a.at(0x102858bc);
                bias_phase_2(a,state);
            } else {
                a.at(0x1028588c); a.value(28,0x0); a.jump(0x102859f0);
                bias_phase_5(a,state);
            }
        }
    }
}
}
void adjust_action_bias(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x102857c0,dependencies,observer);
    a.at(0x102857c0); a.link(); a.multiple(22,false,0); a.load(27,2,-0x51e8); a.move(29,3); a.load(30,2,-0x7b90);
    a.move(24,4); a.load(26,2,-0x7b8c); a.load(25,2,-0x7b88); a.load(31,2,-0x7b84); a.save_link(); a.frame(128);
    a.call(0x10289560); a.mask(0,3,0x1,true,0);
    if (a.equal()) {
        a.at(0x10285800); a.load(3,31,0x0); a.load(0,25,0x0); a.offset(3,3,0x2); a.load<1>(22,29,0x10d);
        a.compare_registers(3,0,false); a.store(31,0x0,3);
        if (a.positive()) {
            a.at(0x1028581c); a.store(31,0x0,26); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(30,0x0,0);
        }
        a.at(0x1028582c);
        bias_phase_1(a,state);
    } else {
        a.at(0x102857f8); a.move(3,24); a.jump(0x10285eb0);
    }
    a.at(0x10285eb0); a.load(0,1,0x88); a.offset(1,1,0x80); a.restore_link(); a.multiple(22,true,0); a.finish();
}
}
