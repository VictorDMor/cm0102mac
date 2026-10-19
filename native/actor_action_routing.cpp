#include "actor_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_action {
using Call = SemanticGuestCall;
namespace {
bool update_close_contacts(Call&,State&);
bool update_pressured_contacts(Call&,State&);
bool update_set_piece_contacts(Call&,State&);
bool update_temporary_contacts(Call&,State&);
bool update_possession_contacts(Call&,State&);
void routing_phase_1(Call&,State&);
void routing_phase_2(Call&,State&);
void routing_phase_3(Call&,State&);
void routing_phase_4(Call&,State&);
void routing_phase_5(Call&,State&);
void routing_phase_6(Call&,State&);
void routing_phase_7(Call&,State&);
void routing_phase_8(Call&,State&);
void routing_phase_9(Call&,State&);
void routing_phase_10(Call&,State&);
void routing_phase_11(Call&,State&);
void routing_phase_12(Call&,State&);
void routing_phase_13(Call&,State&);
void routing_phase_14(Call&,State&);
void routing_phase_15(Call&,State&);
void routing_phase_16(Call&,State&);
void routing_phase_17(Call&,State&);
void routing_phase_18(Call&,State&);
void routing_phase_19(Call&,State&);
void routing_phase_20(Call&,State&);
void routing_phase_21(Call&,State&);
bool update_close_contacts(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x102777f8); a.load<1>(0,31,0x104); a.sign8(3,24); a.sign8(0,0); a.compare_registers(3,0);
        if (a.at_least()) return true;
        a.at(0x1027773c); a.sign8(0,24); a.load(3,31,0x1a4); a.mask(0,0,0xfffffffc,false,2); a.load<1>(5,31,0x105);
        a.add(4,31,0); a.load<1>(6,31,0x106); a.load(4,4,0x1b4); a.call(0x101d8120); a.sign8(0,3,true);
        if (!a.equal()) {
            a.at(0x10277764); a.load(3,30,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.load<1>(25,31,0x107);
            a.load<1>(26,31,0x10a); a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10277784); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x10277794); a.load(3,30,0x0); a.value(4,0x91a30000); a.load(5,29,0x0); a.offset(4,4,-0x4c3b);
            a.load<2>(6,3,0x0); a.sign8(3,25); a.sign8(0,26); a.add(6,6,5); a.multiply_high(4,4,6); a.add(4,4,6);
            a.shift_right(4,4,0x9); a.mask(5,4,0x1,false,1); a.add(4,4,5); a.multiply(4,4,0x384); a.product(3,3,0);
            a.subtract(4,4,6); a.offset(0,4,0x32); a.compare_registers(3,0);
            if (a.positive()) {
            } else {
                a.at(0x102777e0); a.sign8(0,24); a.mask(0,0,0xfffffffc,false,2); a.add(3,31,0); a.load(3,3,0x1b4);
                a.call(0x102836a0);
            }
        }
        a.at(0x102777f4); a.offset(24,24,0x1);
        continue;
    }
}
bool update_pressured_contacts(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x10277a30); a.load<1>(0,31,0x104); a.sign8(3,24); a.sign8(0,0); a.compare_registers(3,0);
        if (a.at_least()) return true;
        a.at(0x10277974); a.sign8(0,24); a.load(3,31,0x1a4); a.mask(0,0,0xfffffffc,false,2); a.load<1>(5,31,0x105);
        a.add(4,31,0); a.load<1>(6,31,0x106); a.load(4,4,0x1b4); a.call(0x101d8120); a.sign8(0,3,true);
        if (!a.equal()) {
            a.at(0x1027799c); a.load(3,30,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.load<1>(25,31,0x107);
            a.load<1>(26,31,0x10a); a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x102779bc); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x102779cc); a.load(3,30,0x0); a.value(4,0x91a30000); a.load(5,29,0x0); a.offset(4,4,-0x4c3b);
            a.load<2>(6,3,0x0); a.sign8(3,25); a.sign8(0,26); a.add(6,6,5); a.multiply_high(4,4,6); a.add(4,4,6);
            a.shift_right(4,4,0x9); a.mask(5,4,0x1,false,1); a.add(4,4,5); a.multiply(4,4,0x384); a.product(3,3,0);
            a.subtract(4,4,6); a.offset(0,4,0x32); a.compare_registers(3,0);
            if (a.positive()) {
            } else {
                a.at(0x10277a18); a.sign8(0,24); a.mask(0,0,0xfffffffc,false,2); a.add(3,31,0); a.load(3,3,0x1b4);
                a.call(0x102836a0);
            }
        }
        a.at(0x10277a2c); a.offset(24,24,0x1);
        continue;
    }
}
bool update_set_piece_contacts(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x10277ccc); a.load<1>(0,31,0x104); a.sign8(3,24); a.sign8(0,0); a.compare_registers(3,0);
        if (a.at_least()) return true;
        a.at(0x10277c10); a.sign8(0,24); a.load(3,31,0x1a4); a.mask(0,0,0xfffffffc,false,2); a.load<1>(5,31,0x105);
        a.add(4,31,0); a.load<1>(6,31,0x106); a.load(4,4,0x1b4); a.call(0x101d8120); a.sign8(0,3,true);
        if (!a.equal()) {
            a.at(0x10277c38); a.load(3,30,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.load<1>(25,31,0x107);
            a.load<1>(26,31,0x10a); a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10277c58); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x10277c68); a.load(3,30,0x0); a.value(4,0x91a30000); a.load(5,29,0x0); a.offset(4,4,-0x4c3b);
            a.load<2>(6,3,0x0); a.sign8(3,25); a.sign8(0,26); a.add(6,6,5); a.multiply_high(4,4,6); a.add(4,4,6);
            a.shift_right(4,4,0x9); a.mask(5,4,0x1,false,1); a.add(4,4,5); a.multiply(4,4,0x384); a.product(3,3,0);
            a.subtract(4,4,6); a.offset(0,4,0x32); a.compare_registers(3,0);
            if (a.positive()) {
            } else {
                a.at(0x10277cb4); a.sign8(0,24); a.mask(0,0,0xfffffffc,false,2); a.add(3,31,0); a.load(3,3,0x1b4);
                a.call(0x102836a0);
            }
        }
        a.at(0x10277cc8); a.offset(24,24,0x1);
        continue;
    }
}
bool update_temporary_contacts(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x10277dd0); a.load<1>(0,31,0x104); a.sign8(3,24); a.sign8(0,0); a.compare_registers(3,0);
        if (a.at_least()) return true;
        a.at(0x10277d14); a.sign8(0,24); a.load(3,31,0x1a4); a.mask(0,0,0xfffffffc,false,2); a.load<1>(5,31,0x105);
        a.add(4,31,0); a.load<1>(6,31,0x106); a.load(4,4,0x1b4); a.call(0x101d8120); a.sign8(0,3,true);
        if (!a.equal()) {
            a.at(0x10277d3c); a.load(3,30,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.load<1>(25,31,0x107);
            a.load<1>(26,31,0x10a); a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10277d5c); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x10277d6c); a.load(3,30,0x0); a.value(4,0x91a30000); a.load(5,29,0x0); a.offset(4,4,-0x4c3b);
            a.load<2>(6,3,0x0); a.sign8(3,25); a.sign8(0,26); a.add(6,6,5); a.multiply_high(4,4,6); a.add(4,4,6);
            a.shift_right(4,4,0x9); a.mask(5,4,0x1,false,1); a.add(4,4,5); a.multiply(4,4,0x384); a.product(3,3,0);
            a.subtract(4,4,6); a.offset(0,4,0x32); a.compare_registers(3,0);
            if (a.positive()) {
            } else {
                a.at(0x10277db8); a.sign8(0,24); a.mask(0,0,0xfffffffc,false,2); a.add(3,31,0); a.load(3,3,0x1b4);
                a.call(0x102836a0);
            }
        }
        a.at(0x10277dcc); a.offset(24,24,0x1);
        continue;
    }
}
bool update_possession_contacts(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x10277ec8); a.load<1>(0,31,0x104); a.sign8(3,24); a.sign8(0,0); a.compare_registers(3,0);
        if (a.at_least()) return true;
        a.at(0x10277e0c); a.sign8(0,24); a.load(3,31,0x1a4); a.mask(0,0,0xfffffffc,false,2); a.load<1>(5,31,0x105);
        a.add(4,31,0); a.load<1>(6,31,0x106); a.load(4,4,0x1b4); a.call(0x101d8120); a.sign8(0,3,true);
        if (!a.equal()) {
            a.at(0x10277e34); a.load(3,30,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.load<1>(25,31,0x107);
            a.load<1>(23,31,0x10a); a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10277e54); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x10277e64); a.load(3,30,0x0); a.value(4,0x91a30000); a.load(5,29,0x0); a.offset(4,4,-0x4c3b);
            a.load<2>(6,3,0x0); a.sign8(3,25); a.sign8(0,23); a.add(6,6,5); a.multiply_high(4,4,6); a.add(4,4,6);
            a.shift_right(4,4,0x9); a.mask(5,4,0x1,false,1); a.add(4,4,5); a.multiply(4,4,0x384); a.product(3,3,0);
            a.subtract(4,4,6); a.offset(0,4,0x32); a.compare_registers(3,0);
            if (a.positive()) {
            } else {
                a.at(0x10277eb0); a.sign8(0,24); a.mask(0,0,0xfffffffc,false,2); a.add(3,31,0); a.load(3,3,0x1b4);
                a.call(0x102836a0);
            }
        }
        a.at(0x10277ec4); a.offset(24,24,0x1);
        continue;
    }
}
void routing_phase_21(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102781e4); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x102781fc); a.move(3,31); a.call(0x102836a0);
    } else {
        a.at(0x102781ec); a.move(3,31); a.value(4,0x1); a.call(0x10281af0); a.jump(0x10278204);
    }
}
void routing_phase_20(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102781e0); a.value(0,0x0);
    routing_phase_21(a,state);
}
void routing_phase_19(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102781a0); a.sign8(0,4); a.load<1>(5,24,0x791d); a.leading_zeroes(0,0); a.load<1>(6,24,0x791e);
    a.mask(0,0,0x7ffffff,false,27); a.move(3,24); a.sign8(4,0); a.call(0x101eea30); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x102781e0);
        routing_phase_20(a,state);
    } else {
        a.at(0x102781c8); a.load<1>(0,24,0x791f); a.sign8(0,0); a.compare(0,-1);
        if (a.equal()) {
            a.at(0x102781e0);
            routing_phase_20(a,state);
        } else {
            a.at(0x102781d8); a.value(0,0x1); a.jump(0x102781e4);
            routing_phase_21(a,state);
        }
    }
}
void routing_phase_18(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102780e0); a.load(4,30,0x0); a.value(3,0x66660000); a.load(0,29,0x0); a.offset(3,3,0x6667);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x1); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x5); a.subtract(0,0,4); a.compare_registers(24,0);
    if (a.nonpositive()) {
    } else {
        a.at(0x10278118); a.move(3,31); a.call(0x102775e0); a.jump(0x10278204);
    }
}
void routing_phase_17(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102780b8); a.load(3,30,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(30,0x0,3);
    if (a.positive()) {
        a.at(0x102780d0); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(29,0x0,0);
    }
    a.at(0x102780e0);
    routing_phase_18(a,state);
}
void routing_phase_16(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102780b4); a.move(24,0);
    routing_phase_17(a,state);
}
void routing_phase_15(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10277fa4); a.exclusive_flags(3,25,0x80000000); a.value(0,0x43300000); a.store(1,0x44,3);
    a.value(6,0x80010000); a.load(7,30,0x0); a.value(4,0x43300000); a.store(1,0x40,0); a.value(5,0x10000);
    a.floating_load(2,26,128); a.value(3,0x80010000); a.floating_load(1,1,64); a.offset(6,6,-0x7fff);
    a.floating_load(0,26,0); a.offset(5,5,-0x1); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0); a.load(7,29,0x0);
    a.multiply(0,25,0xa); a.offset(3,3,-0x7fff); a.store(1,0x48,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,26,128); a.add(4,4,7); a.shift_right(4,4,0xf);
    a.mask(6,4,0x1,false,1); a.add(4,4,6); a.product(4,4,5); a.subtract(24,4,7);
    a.exclusive_flags(4,24,0x80000000); a.store(1,0x4c,4); a.multiply_high(3,3,0); a.floating_load(0,1,72);
    a.floating_subtract(0,0,1); a.add(0,3,0); a.shift_right(0,0,0xf); a.floating_product(0,2,0);
    a.mask(3,0,0x1,false,1); a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,56); a.load(24,1,0x3c);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,25,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(24,24,3);
    a.compare_registers(24,0);
    if (a.positive()) {
        a.at(0x10278080); a.offset(24,25,-0x1); a.jump(0x102780b8);
    }
    a.at(0x102780b8);
    routing_phase_17(a,state);
}
void routing_phase_14(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10277f84); a.value(3,0x10000); a.offset(3,3,-0x1); a.compare_registers(25,3);
    if (a.positive()) {
        a.at(0x10277fa4);
        routing_phase_15(a,state);
    } else {
        a.at(0x10277f94); a.value(3,0xffff0000); a.offset(3,3,0x1); a.compare_registers(25,3);
        if (a.at_least()) {
            a.at(0x10278088); a.load(3,30,0x0); a.compare(25,0); a.load(0,29,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,25); a.product(0,0,25); a.subtract(24,0,3); a.negate(0,24);
            if (a.positive()) {
                a.at(0x102780b0); a.move(0,24);
            }
            a.at(0x102780b4);
            routing_phase_16(a,state);
        } else {
            a.at(0x10277fa4);
            routing_phase_15(a,state);
        }
    }
}
void routing_phase_13(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10277df0); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load(0,3,-0x2000);
    a.compare_registers(31,0,false);
    if (!a.equal()) {
        a.at(0x10278124); a.load<1>(4,31,0x27); a.load(24,31,0x1a4); a.sign8(0,4); a.load<1>(3,24,0x7924);
        a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27); a.sign8(3,3); a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x10278150); a.load<1>(0,24,0x7924); a.sign8(3,4); a.sign8(0,0); a.compare_registers(3,0);
            if (!a.equal()) {
                a.at(0x1027816c); a.offset(3,24,0x10000); a.load(0,3,-0x2008); a.compare(0,0,false);
                if (a.equal()) {
                    a.at(0x102781a0);
                    routing_phase_19(a,state);
                } else {
                    a.at(0x1027817c); a.offset(3,24,0x10000); a.sign8(0,4); a.load(3,3,-0x2008);
                    a.load<1>(3,3,0x27); a.sign8(3,3); a.compare_registers(3,0);
                    if (!a.equal()) {
                        a.at(0x102781a0);
                        routing_phase_19(a,state);
                    } else {
                        a.at(0x10278198); a.value(0,0x1); a.jump(0x102781e4);
                        routing_phase_21(a,state);
                    }
                }
            } else {
                a.at(0x10278164); a.value(0,0x1); a.jump(0x102781e4);
                routing_phase_21(a,state);
            }
        } else {
            a.at(0x10278148); a.value(0,0x0); a.jump(0x102781e4);
            routing_phase_21(a,state);
        }
    } else {
        a.at(0x10277e04); a.value(24,0x0); a.jump(0x10277ec8);
        if (update_possession_contacts(a,state)) {
            a.at(0x10277edc); a.move(3,31); a.value(4,0x0); a.call(0x10278220); a.load(3,31,0x1a4); a.value(0,0x0);
            a.store<1>(3,0x1c0,0); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load(0,3,-0x2000);
            a.compare_registers(31,0,false);
            if (a.equal()) {
            } else {
                a.at(0x10277f08); a.load<2>(0,31,0x19c,true); a.compare(0,109);
                if (a.equal()) {
                } else {
                    a.at(0x10277f14); a.load<1>(0,31,0x2c); a.sign8(0,0,true);
                    if (!a.equal()) {
                    } else {
                        a.at(0x10277f20); a.load(3,31,0x1a4); a.load<1>(0,3,0x7928); a.sign8(0,0,true);
                        if (!a.equal()) {
                        } else {
                            a.at(0x10277f30); a.load<1>(0,31,0x10a); a.sign8(0,0,true);
                            if (a.equal()) {
                            } else {
                                a.at(0x10277f3c); a.floating_load(0,31,128,true); a.floating_integer(0,0);
                                a.floating_store(0,1,64); a.load(25,1,0x44); a.compare(25,0);
                                if (!a.equal()) {
                                    a.at(0x10277f5c); a.load(3,30,0x0); a.load(0,27,0x0); a.offset(3,3,0x2);
                                    a.compare_registers(3,0,false); a.store(30,0x0,3);
                                    if (a.positive()) {
                                        a.at(0x10277f74); a.store(30,0x0,28); a.call(0x1077b770);
                                        a.mask(0,3,0xffff,false,0); a.store(29,0x0,0);
                                    }
                                    a.at(0x10277f84);
                                    routing_phase_14(a,state);
                                } else {
                                    a.at(0x10277f54); a.value(24,0x0); a.jump(0x102780b8);
                                    routing_phase_17(a,state);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void routing_phase_12(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10277cec); a.load<1>(0,31,0x2c); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10277df0);
        routing_phase_13(a,state);
    } else {
        a.at(0x10277cf8); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load(0,3,-0x2000);
        a.compare_registers(31,0,false);
        if (a.equal()) {
            a.at(0x10277df0);
            routing_phase_13(a,state);
        } else {
            a.at(0x10277d0c); a.value(24,0x0); a.jump(0x10277dd0);
            if (update_temporary_contacts(a,state)) {
                a.at(0x10277de4); a.value(0,0x67); a.store<2>(31,0x19c,0); a.jump(0x10278204);
            }
        }
    }
}
void routing_phase_11(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10277bcc); a.load(5,30,0x0); a.value(3,0x66660000); a.load(4,29,0x0); a.offset(3,3,0x6667);
    a.load<2>(5,5,0x0); a.sign8(0,24); a.add(5,5,4); a.multiply_high(3,3,5); a.shift_right(3,3,0x2);
    a.mask(4,3,0x1,false,1); a.add(3,3,4); a.multiply(3,3,0xa); a.subtract(3,3,5); a.compare_registers(0,3);
    if (a.at_least()) {
        a.at(0x10277cec);
        routing_phase_12(a,state);
    } else {
        a.at(0x10277c08); a.value(24,0x0); a.jump(0x10277ccc);
        if (update_set_piece_contacts(a,state)) {
            a.at(0x10277ce0); a.move(3,31); a.call(0x10235db0); a.jump(0x10278204);
        }
    }
}
void routing_phase_10(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10277b4c); a.load(5,30,0x0); a.value(3,0x2aab0000); a.value(0,0x43300000); a.load(4,29,0x0);
    a.load<2>(5,5,0x0); a.offset(3,3,-0x5555); a.store(1,0x40,0); a.add(4,5,4); a.floating_load(1,26,128);
    a.multiply_high(0,3,4); a.shift_right(0,0,0x1); a.mask(3,0,0x1,false,1); a.add(0,0,3); a.multiply(0,0,0xc);
    a.subtract(0,0,4); a.exclusive_flags(0,0,0x80000000); a.store(1,0x44,0); a.floating_load(0,1,64);
    a.floating_subtract(0,0,1,true); a.floating_compare(31,0);
    if (a.at_least()) {
        a.at(0x10277cec);
        routing_phase_12(a,state);
    } else {
        a.at(0x10277ba0); a.load(3,30,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.load<1>(24,31,0x110);
        a.compare_registers(3,0,false); a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x10277bbc); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(29,0x0,0);
        }
        a.at(0x10277bcc);
        routing_phase_11(a,state);
    }
}
void routing_phase_9(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10277b24); a.load(3,30,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(30,0x0,3);
    if (a.positive()) {
        a.at(0x10277b3c); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(29,0x0,0);
    }
    a.at(0x10277b4c);
    routing_phase_10(a,state);
}
void routing_phase_8(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10277a5c); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load(0,3,-0x2000);
    a.compare_registers(31,0,false);
    if (a.equal()) {
        a.at(0x10277cec);
        routing_phase_12(a,state);
    } else {
        a.at(0x10277a70); a.load<1>(0,31,0x27); a.load(3,31,0x1a4); a.sign8(0,0); a.add(3,3,0);
        a.load<1>(0,3,0x1c4); a.compare(0,5);
        if (!a.equal()) {
            a.at(0x10277cec);
            routing_phase_12(a,state);
        } else {
            a.at(0x10277a8c); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x1,true,0);
            if (!a.equal()) {
                a.at(0x10277cec);
                routing_phase_12(a,state);
            } else {
                a.at(0x10277a9c); a.load<1>(0,31,0x27); a.sign8(0,0,true);
                if (!a.equal()) {
                    a.at(0x10277ae8); a.load<1>(0,31,0x105); a.sign8(3,0); a.offset(3,3,-0x4); a.call(0x10775540);
                    a.move(0,3); a.load<1>(3,31,0x106); a.sign8(24,0); a.sign8(3,3); a.call(0x10775540);
                    a.sign8(3,3); a.sign8(0,24); a.multiply(3,3,0x24); a.mask(0,0,0xfffffffc,false,2);
                    a.add(0,3,0); a.floating_indexed_single(31,25,0);
                } else {
                    a.at(0x10277aa8); a.load<1>(0,31,0x105); a.sign8(3,0); a.offset(3,3,-0x4); a.call(0x10775540);
                    a.load<1>(0,31,0x106); a.sign8(24,3); a.sign8(3,0); a.offset(3,3,-0xb); a.call(0x10775540);
                    a.sign8(3,3); a.sign8(0,24); a.multiply(3,3,0x24); a.mask(0,0,0xfffffffc,false,2);
                    a.add(0,3,0); a.floating_indexed_single(31,25,0); a.jump(0x10277b24);
                }
                a.at(0x10277b24);
                routing_phase_9(a,state);
            }
        }
    }
}
void routing_phase_7(Call& a,[[maybe_unused]] State& state) {
    a.at(0x1027792c); a.load(4,30,0x0); a.value(3,0x66660000); a.load(0,29,0x0); a.offset(3,3,0x6667);
    a.load<2>(4,4,0x0); a.add(4,4,0); a.multiply_high(0,3,4); a.shift_right(0,0,0x3); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x14); a.subtract(0,0,4,true);
    if (!a.equal()) {
        a.at(0x10277a5c);
        routing_phase_8(a,state);
    } else {
        a.at(0x10277960); a.load<1>(0,31,0x110); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x10277a5c);
            routing_phase_8(a,state);
        } else {
            a.at(0x1027796c); a.value(24,0x0); a.jump(0x10277a30);
            if (update_pressured_contacts(a,state)) {
                a.at(0x10277a44); a.load<2>(4,31,0x36,true); a.move(3,31); a.offset(0,4,-0xf);
                a.store<2>(31,0x36,0); a.call(0x10235db0); a.jump(0x10278204);
            }
        }
    }
}
void routing_phase_6(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102778fc); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10277a5c);
        routing_phase_8(a,state);
    } else {
        a.at(0x10277904); a.load(3,30,0x0); a.load(0,27,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
        a.store(30,0x0,3);
        if (a.positive()) {
            a.at(0x1027791c); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
            a.store(29,0x0,0);
        }
        a.at(0x1027792c);
        routing_phase_7(a,state);
    }
}
void routing_phase_5(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102778f8); a.value(0,0x0);
    routing_phase_6(a,state);
}
void routing_phase_4(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102778b8); a.sign8(0,4); a.load<1>(5,24,0x791d); a.leading_zeroes(0,0); a.load<1>(6,24,0x791e);
    a.mask(0,0,0x7ffffff,false,27); a.move(3,24); a.sign8(4,0); a.call(0x101eea30); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x102778f8);
        routing_phase_5(a,state);
    } else {
        a.at(0x102778e0); a.load<1>(0,24,0x791f); a.sign8(0,0); a.compare(0,-1);
        if (a.equal()) {
            a.at(0x102778f8);
            routing_phase_5(a,state);
        } else {
            a.at(0x102778f0); a.value(0,0x1); a.jump(0x102778fc);
            routing_phase_6(a,state);
        }
    }
}
void routing_phase_3(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10277818); a.load(3,31,0x1a4); a.offset(3,3,0x10000); a.load(0,3,-0x2000);
    a.compare_registers(31,0,false);
    if (a.equal()) {
        a.at(0x10277a5c);
        routing_phase_8(a,state);
    } else {
        a.at(0x1027782c); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x1,true,0);
        if (!a.equal()) {
            a.at(0x10277a5c);
            routing_phase_8(a,state);
        } else {
            a.at(0x1027783c); a.load<1>(4,31,0x27); a.load(24,31,0x1a4); a.sign8(0,4); a.load<1>(3,24,0x7924);
            a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27); a.sign8(3,3); a.compare_registers(3,0);
            if (!a.equal()) {
                a.at(0x10277868); a.load<1>(0,24,0x7924); a.sign8(3,4); a.sign8(0,0); a.compare_registers(3,0);
                if (!a.equal()) {
                    a.at(0x10277884); a.offset(3,24,0x10000); a.load(0,3,-0x2008); a.compare(0,0,false);
                    if (a.equal()) {
                        a.at(0x102778b8);
                        routing_phase_4(a,state);
                    } else {
                        a.at(0x10277894); a.offset(3,24,0x10000); a.sign8(0,4); a.load(3,3,-0x2008);
                        a.load<1>(3,3,0x27); a.sign8(3,3); a.compare_registers(3,0);
                        if (!a.equal()) {
                            a.at(0x102778b8);
                            routing_phase_4(a,state);
                        } else {
                            a.at(0x102778b0); a.value(0,0x1); a.jump(0x102778fc);
                            routing_phase_6(a,state);
                        }
                    }
                } else {
                    a.at(0x1027787c); a.value(0,0x1); a.jump(0x102778fc);
                    routing_phase_6(a,state);
                }
            } else {
                a.at(0x10277860); a.value(0,0x0); a.jump(0x102778fc);
                routing_phase_6(a,state);
            }
        }
    }
}
void routing_phase_2(Call& a,[[maybe_unused]] State& state) {
    a.at(0x102776b8); a.load(3,30,0x0); a.value(5,0x66660000); a.value(0,0x43300000);
    a.exclusive_flags(4,24,0x80000000); a.load<2>(7,3,0x0); a.value(3,0x43300000); a.load(6,29,0x0);
    a.offset(5,5,0x6667); a.store(1,0x40,0); a.add(6,7,6); a.floating_load(3,26,128); a.multiply_high(0,5,6);
    a.store(1,0x48,3); a.floating_load(1,26,128); a.store(1,0x4c,4); a.floating_load(2,26,888);
    a.floating_load(0,1,72); a.shift_right(0,0,0x2); a.floating_subtract(3,0,3); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0xa); a.subtract(0,0,6); a.exclusive_flags(0,0,0x80000000); a.store(1,0x44,0);
    a.floating_load(0,1,64); a.floating_subtract(0,0,1); a.floating_product_sum(0,31,2,0); a.floating_compare(3,0);
    a.compare_greater_or_equal_bit();
    if (!a.equal()) {
        a.at(0x10277818);
        routing_phase_3(a,state);
    } else {
        a.at(0x10277734); a.value(24,0x0); a.jump(0x102777f8);
        if (update_close_contacts(a,state)) {
            a.at(0x1027780c); a.value(0,0x67); a.store<2>(31,0x19c,0); a.jump(0x10278204);
        }
    }
}
void routing_phase_1(Call& a,[[maybe_unused]] State& state) {
    a.at(0x10277660); a.load(5,30,0x0); a.value(4,0x7e080000); a.load(3,30,0x0); a.offset(4,4,-0x1f81);
    a.load<2>(6,5,0x0); a.load(5,29,0x0); a.offset(3,3,0x2); a.load(0,27,0x0); a.add(5,6,5); a.store(30,0x0,3);
    a.multiply_high(4,4,5); a.compare_registers(3,0,false); a.shift_right(0,4,0x5); a.mask(3,0,0x1,false,1);
    a.add(0,0,3); a.multiply(0,0,0x41); a.subtract(24,0,5);
    if (a.positive()) {
        a.at(0x102776a8); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(29,0x0,0);
    }
    a.at(0x102776b8);
    routing_phase_2(a,state);
}
}
void route_actor_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x102775e0,dependencies,observer);
    a.at(0x102775e0); a.link(); a.floating_store(31,1,-8); a.multiple(23,false,-16); a.move(31,3);
    a.load(25,2,-0x6424); a.load(26,2,-0x51e8); a.load(29,2,-0x7b90); a.load(28,2,-0x7b8c); a.load(27,2,-0x7b88);
    a.load(30,2,-0x7b84); a.save_link(); a.frame(144); a.load(3,3,0x1a4); a.offset(3,3,0x10000);
    a.load(0,3,-0x2000); a.compare_registers(31,0,false);
    if (a.equal()) {
        a.at(0x10277818);
        routing_phase_3(a,state);
    } else {
        a.at(0x10277624); a.move(3,31); a.call(0x10289560); a.mask(0,3,0x1,true,0);
        if (!a.equal()) {
            a.at(0x10277818);
            routing_phase_3(a,state);
        } else {
            a.at(0x10277634); a.load(3,30,0x0); a.load(0,27,0x0); a.offset(3,3,0x2);
            a.floating_load(31,31,80,true); a.compare_registers(3,0,false); a.store(30,0x0,3);
            if (a.positive()) {
                a.at(0x10277650); a.store(30,0x0,28); a.call(0x1077b770); a.mask(0,3,0xffff,false,0);
                a.store(29,0x0,0);
            }
            a.at(0x10277660);
            routing_phase_1(a,state);
        }
    }
    a.at(0x10278204); a.load(0,1,0x98); a.offset(1,1,0x90); a.floating_load(31,1,-8); a.restore_link();
    a.multiple(23,true,-16); a.finish();
}
}
