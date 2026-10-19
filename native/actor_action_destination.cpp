#include "actor_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_action {
using Call = SemanticGuestCall;
namespace {
void destination_phase_1(Call&,State&);
void destination_phase_2(Call&,State&);
void destination_phase_3(Call&,State&);
void destination_phase_3(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102818d4); a.value(3,0x0);
}
void destination_phase_2(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10281820); a.load(3,28,0x1a4); a.move(27,0); a.load<1>(6,28,0x105); a.move(4,29); a.load<1>(7,28,0x106);
    a.move(5,30); a.call(0x101ee610); a.sign16(0,27); a.sign16(3,3); a.load<1>(27,28,0x10f); a.subtract(3,3,0);
    a.call(0x10775540); a.sign8(4,27); a.offset(0,4,0xa); a.compare_registers(3,0);
    if (a.at_least()) {
        a.at(0x102818d4);
        destination_phase_3(a,state);
    } else {
        a.at(0x10281860); a.load<1>(0,28,0x105); a.sign8(3,29); a.sign8(0,0); a.subtract(3,3,0);
        a.call(0x10775540); a.load<1>(0,28,0x106); a.sign8(4,30); a.sign8(27,3); a.sign8(0,0); a.subtract(3,4,0);
        a.call(0x10775540); a.sign8(3,3); a.sign8(0,27); a.multiply(6,3,0x24); a.load(8,2,-0x6424);
        a.mask(0,0,0xfffffffc,false,2); a.load(3,28,0x1a4); a.move(4,29); a.add(0,6,0);
        a.floating_indexed_single(31,8,0); a.move(5,30); a.move(7,31); a.value(6,0x4); a.call(0x101ef8d0);
        a.floating_compare(31,1); a.compare_less_or_equal_bit();
        if (!a.equal()) {
            a.at(0x102818d4);
            destination_phase_3(a,state);
        } else {
            a.at(0x102818cc); a.value(3,0x1); a.jump(0x102818d8);
        }
    }
}
void destination_phase_1(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102817d0); a.load<1>(0,28,0x27); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10281800); a.value(31,0xb); a.load(3,28,0x1a4); a.move(4,29); a.move(5,30); a.move(7,31);
        a.value(6,0x4); a.call(0x101ee610); a.move(0,3);
    } else {
        a.at(0x102817dc); a.value(31,0x0); a.load(3,28,0x1a4); a.move(4,29); a.move(5,30); a.move(7,31);
        a.value(6,0x4); a.call(0x101ee610); a.move(0,3); a.jump(0x10281820);
    }
    a.at(0x10281820);
    destination_phase_2(a,state);
}
}
void can_reach_destination(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10281780,dependencies,observer);
    a.at(0x10281780); a.link(); a.floating_store(31,1,-8); a.multiple(27,false,-16); a.move(29,4); a.move(28,3);
    a.move(30,5); a.save_link(); a.frame(96); a.load<1>(0,3,0x105); a.sign8(3,29); a.sign8(0,0);
    a.compare_registers(3,0);
    if (!a.equal()) {
        a.at(0x102817d0);
        destination_phase_1(a,state);
    } else {
        a.at(0x102817b4); a.load<1>(0,28,0x106); a.sign8(3,30); a.sign8(0,0); a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x102817d0);
            destination_phase_1(a,state);
        } else {
            a.at(0x102817c8); a.value(3,0x1); a.jump(0x102818d8);
        }
    }
    a.at(0x102818d8); a.load(0,1,0x68); a.offset(1,1,0x60); a.floating_load(31,1,-8); a.restore_link();
    a.multiple(27,true,-16); a.finish();
}
}
