#include "target_acceptance.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::target_acceptance {
using Call = SemanticGuestCall;
namespace {
bool scan_vertical_corridor(Call&,State&);
bool scan_horizontal_corridor(Call&,State&);
bool scan_x_major_corridor(Call&,State&);
bool scan_y_major_corridor(Call&,State&);
void route_phase_1(Call&,State&);
void route_phase_2(Call&,State&);
void route_phase_3(Call&,State&);
void route_phase_4(Call&,State&);
void route_phase_5(Call&,State&);
void route_phase_6(Call&,State&);
void route_phase_7(Call&,State&);
bool scan_vertical_corridor(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x101eeff4); a.sign8(3,30); a.sign8(0,24); a.compare_registers(3,0);
        if (a.at_least()) return true;
        a.at(0x101eef38); a.sign8(0,26); a.compare(0,0);
        if (a.less()) {
            a.at(0x101eef68); a.move(3,28); a.jump(0x101ef3a4);
            return false;
        } else {
            a.at(0x101eef44); a.sign8(0,26); a.compare(0,8);
            if (a.positive()) {
                a.at(0x101eef68); a.move(3,28); a.jump(0x101ef3a4);
                return false;
            } else {
                a.at(0x101eef50); a.sign8(0,30); a.compare(0,0);
                if (a.less()) {
                    a.at(0x101eef68); a.move(3,28); a.jump(0x101ef3a4);
                    return false;
                } else {
                    a.at(0x101eef5c); a.sign8(0,30); a.compare(0,11);
                    if (a.nonpositive()) {
                        a.at(0x101eef70); a.sign8(0,26); a.multiply(3,0,0x450); a.sign8(0,30);
                        a.multiply(0,0,0x5c); a.add(3,3,0); a.offset(19,3,0xa08); a.add(19,29,19); a.sign8(3,27);
                        a.sign8(0,30); a.subtract(0,3,0); a.load(3,2,-0x52c4); a.floating_load(1,3,96);
                        a.exclusive_flags(0,0,0x80000000); a.store(1,0x40,0); a.value(0,0x43300000);
                        a.store(1,0x3c,0); a.floating_load(0,1,60); a.floating_subtract(31,0,1,true); a.move(3,29);
                        a.load<1>(0,29,0x7924); a.sign8(0,0); a.leading_zeroes(0,0);
                        a.mask(0,0,0x7ffffff,false,27); a.sign8(4,0); a.move(5,19); a.floating_move(1,31);
                        a.value(7,0xffffffff); a.value(8,0xffffffff); a.value(9,0xffffffff); a.value(10,0x2);
                        a.call(0x101f09d0); a.add(28,28,3); a.offset(30,30,0x1);
                        continue;
                    } else {
                        a.at(0x101eef68); a.move(3,28); a.jump(0x101ef3a4);
                        return false;
                    }
                }
            }
        }
    }
}
bool scan_horizontal_corridor(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x101ef0f8); a.sign8(3,31); a.sign8(0,23); a.compare_registers(3,0);
        if (a.at_least()) return true;
        a.at(0x101ef03c); a.sign8(0,31); a.compare(0,0);
        if (a.less()) {
            a.at(0x101ef06c); a.move(3,28); a.jump(0x101ef3a4);
            return false;
        } else {
            a.at(0x101ef048); a.sign8(0,31); a.compare(0,8);
            if (a.positive()) {
                a.at(0x101ef06c); a.move(3,28); a.jump(0x101ef3a4);
                return false;
            } else {
                a.at(0x101ef054); a.sign8(0,27); a.compare(0,0);
                if (a.less()) {
                    a.at(0x101ef06c); a.move(3,28); a.jump(0x101ef3a4);
                    return false;
                } else {
                    a.at(0x101ef060); a.sign8(0,27); a.compare(0,11);
                    if (a.nonpositive()) {
                        a.at(0x101ef074); a.sign8(0,31); a.multiply(3,0,0x450); a.sign8(0,27);
                        a.multiply(0,0,0x5c); a.add(3,3,0); a.offset(19,3,0xa08); a.add(19,29,19); a.sign8(3,26);
                        a.sign8(0,31); a.subtract(0,3,0); a.load(3,2,-0x52c4); a.floating_load(1,3,96);
                        a.exclusive_flags(0,0,0x80000000); a.store(1,0x40,0); a.value(0,0x43300000);
                        a.store(1,0x3c,0); a.floating_load(0,1,60); a.floating_subtract(30,0,1,true); a.move(3,29);
                        a.load<1>(0,29,0x7924); a.sign8(0,0); a.leading_zeroes(0,0);
                        a.mask(0,0,0x7ffffff,false,27); a.sign8(4,0); a.move(5,19); a.floating_move(1,30);
                        a.value(7,0xffffffff); a.value(8,0xffffffff); a.value(9,0xffffffff); a.value(10,0x2);
                        a.call(0x101f09d0); a.add(28,28,3); a.offset(31,31,0x1);
                        continue;
                    } else {
                        a.at(0x101ef06c); a.move(3,28); a.jump(0x101ef3a4);
                        return false;
                    }
                }
            }
        }
    }
}
bool scan_x_major_corridor(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x101ef278); a.sign8(3,31); a.sign8(0,23); a.compare_registers(3,0);
        if (a.positive()) return true;
        a.at(0x101ef1ac); a.sign8(3,25); a.sign8(0,21); a.compare_registers(3,0);
        if (a.at_least()) {
            a.at(0x101ef1bc); a.subtract(25,21,25); a.add(30,30,17);
        }
        a.at(0x101ef1c4); a.sign8(0,31); a.compare(0,0);
        if (a.less()) {
            a.at(0x101ef1f4); a.move(3,28); a.jump(0x101ef3a4);
            return false;
        } else {
            a.at(0x101ef1d0); a.sign8(0,31); a.compare(0,8);
            if (a.positive()) {
                a.at(0x101ef1f4); a.move(3,28); a.jump(0x101ef3a4);
                return false;
            } else {
                a.at(0x101ef1dc); a.sign8(0,30); a.compare(0,0);
                if (a.less()) {
                    a.at(0x101ef1f4); a.move(3,28); a.jump(0x101ef3a4);
                    return false;
                } else {
                    a.at(0x101ef1e8); a.sign8(0,30); a.compare(0,11);
                    if (a.nonpositive()) {
                        a.at(0x101ef1fc); a.sign8(0,31); a.multiply(0,0,0x450); a.sign8(3,30);
                        a.multiply(3,3,0x5c); a.add(3,0,3); a.offset(19,3,0xa08); a.add(19,29,19); a.move(3,29);
                        a.move(4,31); a.move(5,30); a.move(6,26); a.move(7,27); a.call(0x101ef8d0);
                        a.floating_move(29,1); a.move(3,29); a.load<1>(0,29,0x7924); a.sign8(0,0);
                        a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27); a.sign8(4,0); a.move(5,19);
                        a.floating_move(1,29); a.value(7,0xffffffff); a.value(8,0xffffffff); a.value(9,0xffffffff);
                        a.value(10,0x2); a.call(0x101f09d0); a.add(28,28,3); a.offset(31,31,0x1); a.add(25,25,20);
                        a.store<1>(1,0x3a,25);
                        continue;
                    } else {
                        a.at(0x101ef1f4); a.move(3,28); a.jump(0x101ef3a4);
                        return false;
                    }
                }
            }
        }
    }
}
bool scan_y_major_corridor(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x101ef390); a.sign8(3,30); a.sign8(0,24); a.compare_registers(3,0);
        if (a.positive()) return true;
        a.at(0x101ef2c4); a.sign8(3,25); a.sign8(0,20); a.compare_registers(3,0);
        if (a.at_least()) {
            a.at(0x101ef2d4); a.subtract(25,20,25); a.add(31,31,18);
        }
        a.at(0x101ef2dc); a.sign8(0,31); a.compare(0,0);
        if (a.less()) {
            a.at(0x101ef30c); a.move(3,28); a.jump(0x101ef3a4);
            return false;
        } else {
            a.at(0x101ef2e8); a.sign8(0,31); a.compare(0,8);
            if (a.positive()) {
                a.at(0x101ef30c); a.move(3,28); a.jump(0x101ef3a4);
                return false;
            } else {
                a.at(0x101ef2f4); a.sign8(0,30); a.compare(0,0);
                if (a.less()) {
                    a.at(0x101ef30c); a.move(3,28); a.jump(0x101ef3a4);
                    return false;
                } else {
                    a.at(0x101ef300); a.sign8(0,30); a.compare(0,11);
                    if (a.nonpositive()) {
                        a.at(0x101ef314); a.sign8(0,31); a.multiply(0,0,0x450); a.sign8(3,30);
                        a.multiply(3,3,0x5c); a.add(3,0,3); a.offset(19,3,0xa08); a.add(19,29,19); a.move(3,29);
                        a.move(4,31); a.move(5,30); a.move(6,26); a.move(7,27); a.call(0x101ef8d0);
                        a.floating_move(28,1); a.move(3,29); a.load<1>(0,29,0x7924); a.sign8(0,0);
                        a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27); a.sign8(4,0); a.move(5,19);
                        a.floating_move(1,28); a.value(7,0xffffffff); a.value(8,0xffffffff); a.value(9,0xffffffff);
                        a.value(10,0x2); a.call(0x101f09d0); a.add(28,28,3); a.offset(30,30,0x1); a.add(25,25,21);
                        a.store<1>(1,0x38,25);
                        continue;
                    } else {
                        a.at(0x101ef30c); a.move(3,28); a.jump(0x101ef3a4);
                        return false;
                    }
                }
            }
        }
    }
}
void route_phase_7(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101ef2b0); a.move(31,26); a.move(30,27); a.value(25,0x0); a.store<1>(1,0x39,25); a.jump(0x101ef390);
    if (scan_y_major_corridor(a,state)) {
        a.at(0x101ef3a0); a.move(3,28);
    }
}
void route_phase_6(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101ef198); a.move(31,26); a.move(30,27); a.value(25,0x0); a.move(16,25); a.jump(0x101ef278);
    if (scan_x_major_corridor(a,state)) {
        a.at(0x101ef288); a.move(3,28); a.jump(0x101ef3a4);
    }
}
void route_phase_5(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101ef168); a.sign8(3,21); a.sign8(0,20); a.compare_registers(3,0);
    if (a.less()) {
        a.at(0x101ef290); a.sign8(3,26); a.sign8(0,23); a.subtract(0,3,0); a.compare(0,0);
        if (a.at_least()) {
            a.at(0x101ef2ac); a.value(18,0x1);
        } else {
            a.at(0x101ef2a4); a.value(18,0xffffffff); a.jump(0x101ef2b0);
        }
        a.at(0x101ef2b0);
        route_phase_7(a,state);
    } else {
        a.at(0x101ef178); a.sign8(3,27); a.sign8(0,24); a.subtract(0,3,0); a.compare(0,0);
        if (a.at_least()) {
            a.at(0x101ef194); a.value(17,0x1);
        } else {
            a.at(0x101ef18c); a.value(17,0xffffffff); a.jump(0x101ef198);
        }
        a.at(0x101ef198);
        route_phase_6(a,state);
    }
}
void route_phase_4(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101ef150); a.move(22,26); a.move(26,23); a.move(23,22); a.move(22,27); a.move(27,24); a.move(24,22);
    route_phase_5(a,state);
}
void route_phase_3(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101ef130); a.sign8(3,20); a.sign8(0,21); a.compare_registers(3,0);
    if (a.nonpositive()) {
        a.at(0x101ef168);
        route_phase_5(a,state);
    } else {
        a.at(0x101ef140); a.sign8(3,27); a.sign8(0,24); a.compare_registers(3,0);
        if (a.nonpositive()) {
            a.at(0x101ef168);
            route_phase_5(a,state);
        } else {
            a.at(0x101ef150);
            route_phase_4(a,state);
        }
    }
}
void route_phase_2(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101ef034); a.move(31,26); a.jump(0x101ef0f8);
    if (scan_horizontal_corridor(a,state)) {
        a.at(0x101ef108); a.move(3,28); a.jump(0x101ef3a4);
    }
}
void route_phase_1(Call& a,[[maybe_unused]] State& state) {
    a.at(0x101eef30); a.move(30,27); a.jump(0x101eeff4);
    if (scan_vertical_corridor(a,state)) {
        a.at(0x101ef004); a.move(3,28); a.jump(0x101ef3a4);
    }
}
}
void route_obstruction(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101eeeb0,dependencies,observer);
    a.at(0x101eeeb0); a.link(); a.move(11,1); a.call(0x10773808); a.multiple(16,false,-32); a.save_link();
    a.frame(176); a.move(29,3); a.move(26,4); a.move(27,5); a.move(23,6); a.move(24,7); a.value(28,0x0);
    a.sign8(3,26); a.sign8(0,23); a.subtract(3,3,0); a.call(0x10775540); a.sign8(21,3); a.sign8(3,27);
    a.sign8(0,24); a.subtract(3,3,0); a.call(0x10775540); a.sign8(20,3); a.sign8(0,21); a.compare(0,0);
    if (!a.equal()) {
        a.at(0x101ef00c); a.sign8(0,20); a.compare(0,0);
        if (!a.equal()) {
            a.at(0x101ef110); a.sign8(3,21); a.sign8(0,20); a.compare_registers(3,0);
            if (a.less()) {
                a.at(0x101ef130);
                route_phase_3(a,state);
            } else {
                a.at(0x101ef120); a.sign8(3,26); a.sign8(0,23); a.compare_registers(3,0);
                if (a.positive()) {
                    a.at(0x101ef150);
                    route_phase_4(a,state);
                } else {
                    a.at(0x101ef130);
                    route_phase_3(a,state);
                }
            }
        } else {
            a.at(0x101ef018); a.sign8(3,26); a.sign8(0,23); a.compare_registers(3,0);
            if (a.positive()) {
                a.at(0x101ef028); a.move(22,26); a.move(26,23); a.move(23,22);
            }
            a.at(0x101ef034);
            route_phase_2(a,state);
        }
    } else {
        a.at(0x101eef14); a.sign8(3,27); a.sign8(0,24); a.compare_registers(3,0);
        if (a.positive()) {
            a.at(0x101eef24); a.move(22,27); a.move(27,24); a.move(24,22);
        }
        a.at(0x101eef30);
        route_phase_1(a,state);
    }
    a.at(0x101ef3a4); a.load(0,1,0xb8); a.offset(1,1,0xb0); a.move(11,1); a.call(0x10773858); a.restore_link();
    a.multiple(16,true,-32); a.finish();
}
}
