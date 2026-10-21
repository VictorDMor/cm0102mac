#include "target_acceptance.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::target_acceptance {
using Call = SemanticGuestCall;
namespace {
void facing_phase_1(Call&,State&);
void facing_phase_2(Call&,State&);
void facing_phase_3(Call&,State&);
void facing_phase_4(Call&,State&);
void facing_phase_5(Call&,State&);
void facing_phase_6(Call&,State&);
void facing_phase_7(Call&,State&);
void facing_phase_8(Call&,State&);
void facing_phase_8(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10245028); a.value(3,0x1); a.jump(0x10245034);
}
void facing_phase_7(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10245014); a.sign8(0,25); a.compare_registers(0,19);
    if (a.at_least()) {
        a.at(0x10245028);
        facing_phase_8(a,state);
    } else {
        a.at(0x10245020); a.value(3,0x0); a.jump(0x10245034);
    }
}
void facing_phase_6(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10245010); a.move(19,0);
    facing_phase_7(a,state);
}
void facing_phase_5(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10244f00); a.exclusive_flags(3,26,0x80000000); a.value(0,0x43300000); a.store(1,0x44,3);
    a.value(6,0x80010000); a.load(7,24,0x0); a.value(4,0x43300000); a.store(1,0x40,0); a.value(5,0x10000);
    a.floating_load(2,30,128); a.value(3,0x80010000); a.floating_load(1,1,64); a.offset(6,6,-0x7fff);
    a.floating_load(0,30,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,31,0x0);
    a.multiply(0,26,0xa); a.offset(3,3,-0x7fff); a.store(1,0x48,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,30,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(19,4,7);
    a.exclusive_flags(4,19,0x80000000); a.store(1,0x4c,4); a.multiply_high(3,3,0); a.floating_load(0,1,72);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,56); a.load(19,1,0x3c);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,26,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(19,19,3);
    a.compare_registers(19,0);
    if (a.positive()) {
        a.at(0x10244fdc); a.offset(19,26,-0x1); a.jump(0x10245014);
    }
    a.at(0x10245014);
    facing_phase_7(a,state);
}
void facing_phase_4(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10244ee0); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(26,3);
    if (a.positive()) {
        a.at(0x10244f00);
        facing_phase_5(a,state);
    } else {
        a.at(0x10244ef0); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(26,3);
        if (a.at_least()) {
            a.at(0x10244fe4); a.load(3,24,0x0); a.compare(26,0); a.load(0,31,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,26); a.product(0,0,26); a.subtract(19,0,3); a.negate(0,19);
            if (a.positive()) {
                a.at(0x1024500c); a.move(0,19);
            }
            a.at(0x10245010);
            facing_phase_6(a,state);
        } else {
            a.at(0x10244f00);
            facing_phase_5(a,state);
        }
    }
}
void facing_phase_3(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10244e60); a.sign16(3,0); a.sign16(0,20); a.compare_registers(3,0);
    if (a.at_least()) {
        a.at(0x10245030); a.value(3,0x0);
    } else {
        a.at(0x10244e70); a.compare(29,0,false);
        if (a.equal()) {
            a.at(0x10245028);
            facing_phase_8(a,state);
        } else {
            a.at(0x10244e78); a.load(3,26,0x1a4); a.move(6,27); a.load<1>(4,26,0x105); a.move(7,28);
            a.load<1>(5,26,0x106); a.call(0x101eeeb0); a.store<1>(29,0x0,3); a.load<1>(3,29,0x0);
            a.load<1>(0,29,0x0); a.sign8(3,3); a.load<1>(25,26,0x10e); a.sign8(0,0);
            a.product(26,3,0); state.wr<1>(2304,(state.r<1>(2304)&0xf0)|(std::int32_t(a.r(26))<0?8:a.r(26)>0?4:2)|(state.r<1>(1024)&1));
            if (!a.equal()) {
                a.at(0x10244eb8); a.load(3,24,0x0); a.load(0,23,0x0); a.offset(3,3,0x2);
                a.compare_registers(3,0,false); a.store(24,0x0,3);
                if (a.positive()) {
                    a.at(0x10244ed0); a.store(24,0x0,22); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                    a.store(31,0x0,0);
                }
                a.at(0x10244ee0);
                facing_phase_4(a,state);
            } else {
                a.at(0x10244eb0); a.value(19,0x0); a.jump(0x10245014);
                facing_phase_7(a,state);
            }
        }
    }
}
void facing_phase_2(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10244e3c); a.load<2>(0,26,0x19e,true); a.sign16(3,21); a.subtract(3,3,0); a.call(0x10775540);
    a.sign16(0,3); a.compare(0,180);
    if (a.positive()) {
        a.at(0x10244e58); a.subtract_from(0,0,0x168); a.sign16(0,0);
    }
    a.at(0x10244e60);
    facing_phase_3(a,state);
}
void facing_phase_1(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10244da8); a.load<1>(0,26,0x105); a.sign8(3,27); a.sign8(0,0); a.subtract(3,3,0); a.call(0x10775540);
    a.load<1>(0,26,0x106); a.sign8(4,28); a.sign8(19,3); a.sign8(0,0); a.subtract(3,4,0); a.call(0x10775540);
    a.sign8(3,3); a.sign8(0,19); a.multiply(3,3,0x24); a.load(4,2,-0x6424); a.mask(0,0,0xfffffffc,false,2);
    a.floating_load(1,25,240,true); a.floating_load(0,25,236,true); a.add(0,3,0); a.floating_indexed_single(2,4,0);
    a.floating_subtract(2,2,31,true);
    a.floating_negative_product_difference(0,1,2,0);
    a.floating_integer(0,0); a.floating_store(0,1,64); a.load(0,1,0x44); a.add(20,20,0); a.sign16(0,20);
    a.compare(0,30);
    if (a.at_least()) {
        a.at(0x10244e24); a.sign16(0,20); a.compare(0,135);
        if (a.nonpositive()) {
        } else {
            a.at(0x10244e30); a.value(20,0x87); a.jump(0x10244e3c);
        }
    } else {
        a.at(0x10244e1c); a.value(20,0x1e); a.jump(0x10244e3c);
    }
    a.at(0x10244e3c);
    facing_phase_2(a,state);
}
}
void can_face_target(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10244c60,dependencies,observer);
    a.at(0x10244c60); a.link(); a.floating_store(31,1,-8); a.floating_move(31,2); a.floating_store(30,1,-16);
    a.floating_move(30,1); a.multiple(19,false,-16); a.move(27,4); a.move(26,3); a.move(28,5);
    a.load(30,2,-0x51e8); a.move(29,9); a.load(31,2,-0x7b90); a.load(22,2,-0x7b8c); a.load(23,2,-0x7b88);
    a.load(24,2,-0x7b84); a.load(25,2,-0x51e0); a.save_link(); a.frame(160); a.load<1>(0,3,0x105);
    a.load<1>(3,3,0x106); a.subtract(4,0,27); a.load<2>(0,26,0x19e,true); a.sign8(4,4); a.subtract(5,3,28);
    a.move(3,4); a.multiply(4,3,0x30); a.sign8(7,5); a.load(5,2,-0x641c); a.sign16(3,0);
    a.mask(0,7,0xfffffffe,false,1); a.add(4,5,4); a.add(4,4,0); a.load<2>(21,4,0x1c8,true); a.sign16(0,21);
    a.compare_registers(3,0);
    if (!a.equal()) {
        a.at(0x10244cf8); a.sign8(0,6,true);
        if (a.equal()) {
            a.at(0x10244e38); a.value(20,0x4b);
            facing_phase_2(a,state);
        } else {
            a.at(0x10244d00); a.load(3,26,0x1a4); a.move(4,26); a.move(5,27); a.move(6,28); a.call(0x101eeac0);
            a.sign8(0,3,true);
            if (a.equal()) {
                a.at(0x10244d64); a.load<1>(3,26,0x10e); a.value(0,0x43300000); a.store(1,0x40,0); a.sign8(0,3);
                a.floating_load(2,25,92,true); a.multiply(0,0,0x5); a.floating_load(0,25,128,true);
                a.floating_load(1,30,128); a.floating_product_sum(2,2,30,0);
                a.exclusive_flags(0,0,0x80000000); a.store(1,0x44,0); a.floating_load(0,1,64);
                a.floating_subtract(0,0,1,true); a.floating_sum_single(0,2,0); a.floating_integer(0,0);
                a.floating_store(0,1,72); a.load(20,1,0x4c);
            } else {
                a.at(0x10244d1c); a.load<1>(3,26,0x10e); a.value(0,0x43300000); a.store(1,0x48,0); a.sign8(0,3);
                a.floating_load(2,25,92,true); a.mask(0,0,0xfffffffc,false,2); a.floating_load(0,25,232,true);
                a.exclusive_flags(0,0,0x80000000); a.floating_load(1,30,128); a.store(1,0x4c,0);
                a.floating_product_sum(2,2,30,0); a.floating_load(0,1,72); a.floating_subtract(0,0,1,true);
                a.floating_sum_single(0,2,0); a.floating_integer(0,0); a.floating_store(0,1,64); a.load(20,1,0x44);
                a.jump(0x10244da8);
            }
            a.at(0x10244da8);
            facing_phase_1(a,state);
        }
    } else {
        a.at(0x10244cf0); a.value(3,0x1); a.jump(0x10245034);
    }
    a.at(0x10245034); a.load(0,1,0xa8); a.offset(1,1,0xa0); a.floating_load(31,1,-8); a.floating_load(30,1,-16);
    a.restore_link(); a.multiple(19,true,-16); a.finish();
}
}
