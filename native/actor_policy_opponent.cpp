#include "actor_policy.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_policy {
using Call = SemanticGuestCall;
namespace {
bool scan_linked_opponents(Call&,State&);
bool scan_linked_opponents(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x10286094); a.sign8(0,5); a.compare(0,20);
        if (a.at_least()) return true;
        a.at(0x10285f8c); a.load<1>(0,31,0x19); a.sign8(0,0,true);
        if (a.less()) {
            a.at(0x1028608c); a.offset(5,5,0x1); a.offset(31,31,0x1c4);
            continue;
        } else {
            a.at(0x10285f98); a.load(0,31,0x6c); a.compare_registers(0,3,false);
            if (!a.equal()) {
                a.at(0x1028608c); a.offset(5,5,0x1); a.offset(31,31,0x1c4);
                continue;
            } else {
                a.at(0x10285fa4); a.load<1>(0,30,0x27); a.load(5,30,0x1a4); a.sign8(0,0); a.load<1>(4,30,0x19);
                a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x101149f0);
                a.mask(0,3,0x10,true,0);
                if (a.equal()) {
                    a.at(0x10286084); a.move(3,31); a.jump(0x102860a4);
                    return false;
                } else {
                    a.at(0x10285fcc); a.load<1>(0,30,0x27); a.load(4,30,0x1a4); a.sign8(0,0);
                    a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0); a.multiply(3,0,0x18e0);
                    a.offset(3,3,0x7c6c); a.add(3,4,3); a.load(0,3,0x1880); a.mask(0,0,0x10000000,true,0);
                    if (!a.equal()) {
                        a.at(0x10286028); a.load<1>(0,30,0x109); a.sign8(0,0,true);
                        if (a.equal()) {
                            a.at(0x1028603c); a.move(3,31); a.call(0x10289560); a.move(0,3); a.move(3,31);
                            a.sign16(29,0); a.call(0x102895e0); a.sign16(0,3); a.compare_registers(0,29);
                            if (a.equal()) {
                                a.at(0x10286084); a.move(3,31); a.jump(0x102860a4);
                                return false;
                            } else {
                                a.at(0x10286060); a.move(3,31); a.call(0x102895e0); a.mask(0,3,0x40,true,0);
                                if (!a.equal()) {
                                    a.at(0x10286084); a.move(3,31); a.jump(0x102860a4);
                                    return false;
                                } else {
                                    a.at(0x10286070); a.load<1>(0,30,0x109); a.sign8(0,0,true);
                                    if (a.equal()) {
                                        a.at(0x10286084); a.move(3,31); a.jump(0x102860a4);
                                        return false;
                                    } else {
                                        a.at(0x1028607c); a.value(3,0x0); a.jump(0x102860a4);
                                        return false;
                                    }
                                }
                            }
                        } else {
                            a.at(0x10286034); a.value(3,0x0); a.jump(0x102860a4);
                            return false;
                        }
                    } else {
                        a.at(0x10285ffc); a.load<1>(0,30,0x27); a.load(3,30,0x1a4); a.sign8(0,0);
                        a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0); a.add(3,3,0);
                        a.load<1>(0,3,0xaf); a.sign8(0,0); a.compare(0,1);
                        if (a.nonpositive()) {
                            a.at(0x1028603c); a.move(3,31); a.call(0x10289560); a.move(0,3); a.move(3,31);
                            a.sign16(29,0); a.call(0x102895e0); a.sign16(0,3); a.compare_registers(0,29);
                            if (a.equal()) {
                                a.at(0x10286084); a.move(3,31); a.jump(0x102860a4);
                                return false;
                            } else {
                                a.at(0x10286060); a.move(3,31); a.call(0x102895e0); a.mask(0,3,0x40,true,0);
                                if (!a.equal()) {
                                    a.at(0x10286084); a.move(3,31); a.jump(0x102860a4);
                                    return false;
                                } else {
                                    a.at(0x10286070); a.load<1>(0,30,0x109); a.sign8(0,0,true);
                                    if (a.equal()) {
                                        a.at(0x10286084); a.move(3,31); a.jump(0x102860a4);
                                        return false;
                                    } else {
                                        a.at(0x1028607c); a.value(3,0x0); a.jump(0x102860a4);
                                        return false;
                                    }
                                }
                            }
                        } else {
                            a.at(0x10286028); a.load<1>(0,30,0x109); a.sign8(0,0,true);
                            if (a.equal()) {
                                a.at(0x1028603c); a.move(3,31); a.call(0x10289560); a.move(0,3); a.move(3,31);
                                a.sign16(29,0); a.call(0x102895e0); a.sign16(0,3); a.compare_registers(0,29);
                                if (a.equal()) {
                                    a.at(0x10286084); a.move(3,31); a.jump(0x102860a4);
                                    return false;
                                } else {
                                    a.at(0x10286060); a.move(3,31); a.call(0x102895e0); a.mask(0,3,0x40,true,0);
                                    if (!a.equal()) {
                                        a.at(0x10286084); a.move(3,31); a.jump(0x102860a4);
                                        return false;
                                    } else {
                                        a.at(0x10286070); a.load<1>(0,30,0x109); a.sign8(0,0,true);
                                        if (a.equal()) {
                                            a.at(0x10286084); a.move(3,31); a.jump(0x102860a4);
                                            return false;
                                        } else {
                                            a.at(0x1028607c); a.value(3,0x0); a.jump(0x102860a4);
                                            return false;
                                        }
                                    }
                                }
                            } else {
                                a.at(0x10286034); a.value(3,0x0); a.jump(0x102860a4);
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
}
}
void find_linked_opponent(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10285ef0,dependencies,observer);
    a.at(0x10285ef0); a.link(); a.store(1,-0x4,31); a.store(1,-0x8,30); a.move(30,3); a.store(1,-0xc,29);
    a.save_link(); a.frame(80); a.load<1>(0,3,0x27); a.load(5,3,0x1a4); a.sign8(0,0); a.load<1>(4,3,0x19);
    a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x101149f0); a.mask(0,3,0x4,true,0);
    if (a.equal()) {
        a.at(0x10285f58); a.value(3,0x0); a.jump(0x102860a4);
    } else {
        a.at(0x10285f34); a.load<1>(0,30,0x27); a.load(5,30,0x1a4); a.sign8(0,0); a.load<1>(4,30,0x19);
        a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x10114b10); a.jump(0x10285f60);
        a.load<1>(0,30,0x27); a.value(5,0x0); a.load(4,30,0x1a4); a.sign8(0,0); a.leading_zeroes(0,0);
        a.offset(4,4,0x311c); a.mask(0,0,0x7ffffff,false,27); a.sign8(0,0); a.multiply(0,0,0x2350); a.add(31,4,0);
        a.jump(0x10286094);
        if (scan_linked_opponents(a,state)) {
            a.at(0x102860a0); a.value(3,0x0);
        }
    }
    a.at(0x102860a4); a.load(0,1,0x58); a.offset(1,1,0x50); a.restore_link(); a.load(31,1,-0x4); a.load(30,1,-0x8);
    a.load(29,1,-0xc); a.finish();
}
}
