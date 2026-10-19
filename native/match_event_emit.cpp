#include "match_event.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_event {
using Call = SemanticGuestCall;
namespace {
void choose_event_variant(Call&);
void find_history_start(Call&);
bool append_pending_history(Call&);
void discard_stale_context(Call&);
void emit_rule_1(Call&);
void emit_rule_2(Call&);
void emit_rule_3(Call&);
void emit_rule_4(Call&);
void emit_rule_5(Call&);
void emit_rule_6(Call&);
void emit_rule_7(Call&);
void emit_rule_8(Call&);
void emit_rule_9(Call&);
void emit_rule_10(Call&);
void emit_rule_11(Call&);
void emit_rule_12(Call&);
void emit_rule_13(Call&);
void emit_rule_14(Call&);
void emit_rule_15(Call&);
void emit_rule_16(Call&);
void emit_rule_17(Call&);
void emit_rule_18(Call&);
void emit_rule_19(Call&);
void emit_rule_20(Call&);
void emit_rule_21(Call&);
void emit_rule_22(Call&);
void emit_rule_23(Call&);
void emit_rule_24(Call&);
void emit_rule_25(Call&);
void emit_rule_26(Call&);
void emit_rule_27(Call&);
void emit_rule_28(Call&);
void emit_rule_29(Call&);
void emit_rule_30(Call&);
void emit_rule_31(Call&);
void emit_rule_32(Call&);
void emit_rule_33(Call&);
void emit_rule_34(Call&);
void emit_rule_35(Call&);
void emit_rule_36(Call&);
void emit_rule_37(Call&);
void choose_event_variant(Call& a) {
    for (;;) {
        a.at(0x10201684); a.load<2>(0,3,-0x70f4,true); a.sign16(4,6); a.compare_registers(4,0);
        if (!a.less()) break;
        a.at(0x10201654); a.load(4,3,-0x70f8); a.indexed_load<2>(0,4,5,true); a.add(7,7,0);
        a.compare_registers(17,7);
        if (a.at_least()) {
            a.at(0x1020167c); a.offset(5,5,0x10c); a.offset(6,6,0x1);
            continue;
        } else {
            a.at(0x10201668); a.load<2>(0,31,0x8,true); a.multiply(0,0,0x14); a.add(3,31,0); a.store<2>(3,0x32,6);
            a.jump(0x10201694);
            break;
        }
    }
}
void find_history_start(Call& a) {
    for (;;) {
        a.at(0x10201794); a.offset(3,19,-0x20000); a.sign16(5,7); a.offset(3,3,-0x70fc); a.indexed_load<1>(0,23,3);
        a.sign8(0,0); a.compare_registers(5,0);
        if (!a.less()) break;
        a.at(0x10201770); a.load<2>(3,31,0xe,true); a.sign16(0,4); a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x10201780); a.offset(4,4,-0x1); a.sign16(0,4,true);
            if (a.at_least()) {
            } else {
                a.at(0x1020178c); a.value(4,0xb);
            }
        }
        a.at(0x10201790); a.offset(7,7,0x1);
        continue;
    }
}
bool append_pending_history(Call& a) {
    for (;;) {
        a.at(0x102017e0); a.load<2>(6,31,0x6,true); a.compare(6,700);
        if (a.at_least()) return true;
        a.at(0x102017ec); a.sign16(0,4); a.multiply(3,0,0x14); a.add(3,31,3); a.load<2>(5,3,0x30,true);
        a.compare(5,8000);
        if (a.less()) {
            a.at(0x1020180c); a.sign16(0,5,true);
            if (!a.equal()) {
                return false;
            } else {
                a.at(0x10201814); a.offset(4,4,0x1); a.sign16(0,4); a.compare(0,12);
                if (a.less()) {
                    continue;
                } else {
                    a.at(0x10201824); a.value(4,0x0); a.jump(0x102017e0);
                    continue;
                }
            }
        } else {
            a.at(0x10201804); a.compare(5,8677);
            if (a.less()) {
                a.at(0x10201830); a.multiply(6,6,0x14); a.load<1>(7,3,0x34); a.value(5,0xffffffff);
                a.offset(6,6,0x97c); a.indexed_store<1>(7,31,6); a.load<2>(6,31,0x6,true);
                a.load<2>(7,3,0x30,true); a.multiply(6,6,0x14); a.offset(6,6,0x978); a.indexed_store<2>(7,31,6);
                a.load<2>(6,31,0x6,true); a.load<1>(7,3,0x35); a.multiply(6,6,0x14); a.offset(6,6,0x97d);
                a.indexed_store<1>(7,31,6); a.load<2>(6,31,0x6,true); a.load<1>(7,3,0x36); a.multiply(6,6,0x14);
                a.offset(6,6,0x97e); a.indexed_store<1>(7,31,6); a.load<2>(6,31,0x6,true); a.load<1>(7,3,0x37);
                a.multiply(6,6,0x14); a.offset(6,6,0x97f); a.indexed_store<1>(7,31,6); a.load<2>(6,31,0x6,true);
                a.load<1>(7,3,0x38); a.multiply(6,6,0x14); a.offset(6,6,0x980); a.indexed_store<1>(7,31,6);
                a.load<2>(6,31,0x6,true); a.load(7,3,0x3c); a.multiply(6,6,0x14); a.offset(6,6,0x984);
                a.indexed_store(7,31,6); a.load<2>(6,31,0x6,true); a.load<2>(7,3,0x32,true); a.multiply(3,6,0x14);
                a.offset(3,3,0x97a); a.indexed_store<2>(7,31,3); a.load<2>(3,31,0x6,true); a.multiply(3,3,0x14);
                a.offset(3,3,0x988); a.indexed_store<1>(5,31,3); a.load<2>(3,31,0x6,true); a.offset(3,3,0x1);
                a.store<2>(31,0x6,3); a.load<2>(3,31,0x8,true); a.compare_registers(0,3);
                if (a.equal()) {
                    return true;
                } else {
                    a.at(0x102018f8); a.offset(4,4,0x1); a.sign16(0,4); a.compare(0,12);
                    if (a.less()) {
                        continue;
                    } else {
                        a.at(0x10201908); a.value(4,0x0); a.jump(0x102017e0);
                        continue;
                    }
                }
            } else {
                a.at(0x1020180c); a.sign16(0,5,true);
                if (!a.equal()) {
                    return false;
                } else {
                    a.at(0x10201814); a.offset(4,4,0x1); a.sign16(0,4); a.compare(0,12);
                    if (a.less()) {
                        continue;
                    } else {
                        a.at(0x10201824); a.value(4,0x0); a.jump(0x102017e0);
                        continue;
                    }
                }
            }
        }
    }
}
void discard_stale_context(Call& a) {
    for (;;) {
        a.at(0x10201a50); a.sign8(0,0,true);
        if (!a.equal()) break;
        a.at(0x10201978); a.sign16(0,3); a.value(4,0x0); a.multiply(5,0,0x14); a.offset(0,5,0x30);
        a.indexed_load<2>(7,31,0,true); a.mask(5,7,0x7fff,false,0); a.offset(5,5,-0x1f40); a.sign16(5,5,true);
        if (a.at_least()) {
            a.at(0x1020199c); a.compare(5,677);
            if (a.at_least()) {
            } else {
                a.at(0x102019a4); a.value(4,0x1);
            }
        }
        a.at(0x102019a8); a.compare(4,0);
        if (a.equal()) {
            a.at(0x102019c0); a.value(4,0x0);
        } else {
            a.at(0x102019b0); a.multiply(4,5,0x14); a.indexed_load<1>(4,23,4); a.sign8(4,4); a.jump(0x102019c4);
        }
        a.at(0x102019c4); a.sign8(4,4); a.compare(4,1);
        if (a.equal()) {
            break;
        } else {
            a.at(0x102019d0); a.mask(5,7,0x7fff,false,0); a.value(4,0x0); a.offset(5,5,-0x1f40);
            a.sign16(5,5,true);
            if (a.at_least()) {
                a.at(0x102019e4); a.compare(5,677);
                if (a.at_least()) {
                } else {
                    a.at(0x102019ec); a.value(4,0x1);
                }
            }
            a.at(0x102019f0); a.compare(4,0);
            if (a.equal()) {
                a.at(0x10201a08); a.value(4,0x0);
            } else {
                a.at(0x102019f8); a.multiply(4,5,0x14); a.indexed_load<1>(4,23,4); a.sign8(4,4);
                a.jump(0x10201a0c);
            }
            a.at(0x10201a0c); a.sign8(4,4); a.compare(4,2);
            if (a.equal()) {
                break;
            } else {
                a.at(0x10201a18); a.load<1>(4,31,0x0); a.sign8(4,4,true);
                if (a.equal()) {
                    break;
                } else {
                    a.at(0x10201a24); a.sign16(4,3); a.value(7,0x0); a.subtract(4,4,6); a.offset(3,3,-0x1);
                    a.leading_zeroes(5,4); a.indexed_store<2>(7,31,0); a.sign16(4,3,true);
                    a.mask(4,5,0x7ffffff,false,27); a.sign8(0,4);
                    if (a.at_least()) {
                        continue;
                    } else {
                        a.at(0x10201a4c); a.value(3,0xb);
                        continue;
                    }
                }
            }
        }
    }
}
void emit_rule_37(Call& a) {
    a.at(0x10201df8); a.load<1>(6,1,0x45); a.move(3,31); a.load<1>(7,1,0x46); a.move(5,30); a.value(4,0x21c0);
    a.value(8,0xffffffff); a.value(9,0xffffffff); a.value(10,0x0); a.call(0x10201180);
}
void emit_rule_36(Call& a) {
    a.at(0x10201d40); a.load<1>(6,1,0x45); a.move(3,31); a.load<1>(7,1,0x46); a.move(5,30); a.value(4,0x21c0);
    a.value(8,0xffffffff); a.value(9,0xffffffff); a.value(10,0x0); a.call(0x10201180); a.jump(0x10201e1c);
}
void emit_rule_35(Call& a) {
    a.at(0x10201c68); a.sign16(0,24); a.compare(0,8639);
    if (a.equal()) {
    } else {
        a.at(0x10201c74); a.compare(0,8640);
        if (a.equal()) {
        } else {
            a.at(0x10201c7c); a.load<1>(0,31,0x5); a.sign8(0,0,true);
            if (a.equal()) {
            } else {
                a.at(0x10201c88); a.load<1>(0,31,0x4); a.sign8(0,0,true);
                if (a.equal()) {
                } else {
                    a.at(0x10201c94); a.load<2>(3,31,0xa,true); a.load<2>(0,31,0x8,true); a.compare_registers(3,0);
                    if (!a.equal()) {
                    } else {
                        a.at(0x10201ca4); a.move(3,31); a.move(4,24); a.call(0x10201e30); a.sign8(0,3,true);
                        if (a.equal()) {
                        } else {
                            a.at(0x10201cb8);
                            if (a.at_least()) {
                                a.at(0x10201ccc); a.compare(0,2);
                                if (a.at_least()) {
                                } else {
                                    a.at(0x10201cd4); a.jump(0x10201d90); a.compare(30,0,false);
                                    if (a.equal()) {
                                        a.at(0x10201df8);
                                        emit_rule_37(a);
                                    } else {
                                        a.at(0x10201d98); a.value(3,0x2e8c0000); a.load<2>(0,30,0x7948,true);
                                        a.offset(3,3,-0x5d17); a.load<2>(4,30,0x77c2,true); a.multiply_high(0,3,0);
                                        a.shift_right(0,0,0x1); a.mask(3,0,0x1,false,1); a.add(0,0,3);
                                        a.compare_registers(4,0);
                                        if (!a.equal()) {
                                            a.at(0x10201df8);
                                            emit_rule_37(a);
                                        } else {
                                            a.at(0x10201dc0); a.load<1>(0,30,0x77c4); a.move(3,31); a.move(5,30);
                                            a.value(4,0x21e3); a.sign8(0,0); a.value(6,0xffffffff);
                                            a.leading_zeroes(0,0); a.value(8,0xffffffff);
                                            a.mask(0,0,0x7ffffff,false,27); a.value(9,0xffffffff); a.sign8(7,0);
                                            a.value(10,0x0); a.call(0x10201180); a.jump(0x10201e1c);
                                        }
                                    }
                                }
                            } else {
                                a.at(0x10201cbc); a.compare(0,-2);
                                if (a.equal()) {
                                    a.at(0x10201cd8); a.compare(30,0,false);
                                    if (a.equal()) {
                                        a.at(0x10201d40);
                                        emit_rule_36(a);
                                    } else {
                                        a.at(0x10201ce0); a.value(3,0x2e8c0000); a.load<2>(0,30,0x7948,true);
                                        a.offset(3,3,-0x5d17); a.load<2>(4,30,0x77c2,true); a.multiply_high(0,3,0);
                                        a.shift_right(0,0,0x1); a.mask(3,0,0x1,false,1); a.add(0,0,3);
                                        a.compare_registers(4,0);
                                        if (!a.equal()) {
                                            a.at(0x10201d40);
                                            emit_rule_36(a);
                                        } else {
                                            a.at(0x10201d08); a.load<1>(0,30,0x77c4); a.move(3,31); a.move(5,30);
                                            a.value(4,0x21e3); a.sign8(0,0); a.value(6,0xffffffff);
                                            a.leading_zeroes(0,0); a.value(8,0xffffffff);
                                            a.mask(0,0,0x7ffffff,false,27); a.value(9,0xffffffff); a.sign8(7,0);
                                            a.value(10,0x0); a.call(0x10201180); a.jump(0x10201e1c);
                                        }
                                    }
                                } else {
                                    a.at(0x10201cc4);
                                    if (a.at_least()) {
                                        a.at(0x10201d68); a.load<1>(6,1,0x45); a.move(3,31); a.load<1>(7,1,0x46);
                                        a.move(5,30); a.value(4,0x21bf); a.value(8,0xffffffff);
                                        a.value(9,0xffffffff); a.value(10,0x0); a.call(0x10201180);
                                        a.jump(0x10201e1c);
                                    } else {
                                        a.at(0x10201cc8); a.jump(0x10201e1c);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void emit_rule_34(Call& a) {
    a.at(0x10201bd0); a.sign16(0,24); a.compare(0,8531);
    if (a.equal()) {
        a.at(0x10201bdc); a.load<1>(4,1,0x46); a.move(3,30); a.load<1>(5,1,0x45); a.call(0x101e3180);
        a.value(4,0x219f); a.call(0x10289d50); a.sign8(0,3,true);
        if (a.equal()) {
            a.at(0x10201c24); a.load<1>(4,1,0x46); a.move(3,30); a.load<1>(5,1,0x45); a.call(0x101e3180);
            a.value(4,0x21a0); a.call(0x10289d50); a.sign8(0,3,true);
            if (a.equal()) {
            } else {
                a.at(0x10201c44); a.load<1>(6,1,0x45); a.move(3,31); a.load<1>(7,1,0x46); a.move(5,30);
                a.value(4,0x21a0); a.value(8,0xffffffff); a.value(9,0xffffffff); a.value(10,0x0);
                a.call(0x10201180);
            }
        } else {
            a.at(0x10201bfc); a.load<1>(6,1,0x45); a.move(3,31); a.load<1>(7,1,0x46); a.move(5,30);
            a.value(4,0x219f); a.value(8,0xffffffff); a.value(9,0xffffffff); a.value(10,0x0); a.call(0x10201180);
            a.jump(0x10201c68);
        }
    }
    a.at(0x10201c68);
    emit_rule_35(a);
}
void emit_rule_33(Call& a) {
    a.at(0x10201b98); a.load<1>(0,31,0x1); a.sign8(0,0); a.compare(0,-1);
    if (!a.equal()) {
        a.at(0x10201ba8); a.value(4,0x1); a.sign8(3,26); a.store<1>(31,0x5,4); a.load<2>(0,31,0x8,true);
        a.store<2>(31,0xa,0); a.load<1>(0,31,0x1); a.sign8(0,0); a.compare_registers(3,0);
        if (a.equal()) {
        } else {
            a.at(0x10201bcc); a.store<1>(31,0x4,4);
        }
    }
    a.at(0x10201bd0);
    emit_rule_34(a);
}
void emit_rule_32(Call& a) {
    a.at(0x10201b7c); a.load<1>(0,31,0x4); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10201b88); a.value(3,0xffffffff); a.value(0,0x0); a.store<1>(31,0x1,3); a.store<1>(31,0x4,0);
    }
    a.at(0x10201b98);
    emit_rule_33(a);
}
void emit_rule_31(Call& a) {
    a.at(0x10201b4c); a.value(3,0x1); a.offset(0,23,-0x20000); a.store<1>(31,0x5,3); a.add(3,0,19);
    a.load<2>(0,31,0x8,true); a.store<2>(31,0xa,0); a.load<1>(0,3,-0x70fe); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10201b70); a.store<1>(31,0x1,26); a.value(0,0x0); a.store<1>(31,0x4,0);
    }
    a.at(0x10201b7c);
    emit_rule_32(a);
}
void emit_rule_30(Call& a) {
    a.at(0x10201b30); a.compare(20,2);
    if (a.equal()) {
        a.at(0x10201b4c);
        emit_rule_31(a);
    } else {
        a.at(0x10201b38); a.offset(0,23,-0x20000); a.add(3,0,19); a.load<1>(0,3,-0x70fe); a.sign8(0,0,true);
        if (a.equal()) {
            a.at(0x10201b7c);
            emit_rule_32(a);
        } else {
            a.at(0x10201b4c);
            emit_rule_31(a);
        }
    }
}
void emit_rule_29(Call& a) {
    a.at(0x10201b10); a.load<1>(3,31,0x0); a.sign8(0,3,true);
    if (a.equal()) {
        a.at(0x10201b4c);
        emit_rule_31(a);
    } else {
        a.at(0x10201b1c); a.sign8(0,3); a.compare(0,3);
        if (!a.equal()) {
            a.at(0x10201b30);
            emit_rule_30(a);
        } else {
            a.at(0x10201b28); a.compare(20,1);
            if (a.equal()) {
                a.at(0x10201b4c);
                emit_rule_31(a);
            } else {
                a.at(0x10201b30);
                emit_rule_30(a);
            }
        }
    }
}
void emit_rule_28(Call& a) {
    a.at(0x10201ae4); a.load<2>(3,31,0xe,true); a.load<2>(0,31,0x8,true); a.compare_registers(3,0);
    if (a.equal()) {
        a.at(0x10201af4); a.offset(3,3,0x1); a.sign16(0,3); a.store<2>(31,0xe,3); a.compare(0,12);
        if (a.less()) {
        } else {
            a.at(0x10201b08); a.value(0,0x0); a.store<2>(31,0xe,0);
        }
    }
    a.at(0x10201b10);
    emit_rule_29(a);
}
void emit_rule_27(Call& a) {
    a.at(0x10201abc); a.load<2>(3,31,0x8,true); a.offset(3,3,0x1); a.sign16(0,3); a.store<2>(31,0x8,3);
    a.compare(0,12);
    if (a.at_least()) {
        a.at(0x10201ad4); a.value(3,0x0); a.value(0,0x1); a.store<2>(31,0x8,3); a.store<1>(31,0x2,0);
    }
    a.at(0x10201ae4);
    emit_rule_28(a);
}
void emit_rule_26(Call& a) {
    a.at(0x10201a90); a.load<2>(0,31,0xc,true); a.value(4,0xffffffff); a.multiply(0,0,0x14); a.add(3,31,0);
    a.store<1>(3,0x40,4); a.load<2>(0,31,0xc,true); a.multiply(0,0,0x14); a.add(3,31,0); a.load<2>(0,3,0x30,true);
    a.mask(0,0,0x7fff,false,0); a.store<2>(3,0x30,0);
    emit_rule_27(a);
}
void emit_rule_25(Call& a) {
    a.at(0x10201a58); a.load<2>(3,31,0xc,true); a.load<2>(0,31,0x8,true); a.compare_registers(3,0);
    if (!a.equal()) {
        a.at(0x10201abc);
        emit_rule_27(a);
    } else {
        a.at(0x10201a68); a.load<1>(0,31,0x2); a.sign8(0,0,true);
        if (a.equal()) {
            a.at(0x10201abc);
            emit_rule_27(a);
        } else {
            a.at(0x10201a74); a.offset(3,3,0x1); a.sign16(0,3); a.store<2>(31,0xc,3); a.compare(0,12);
            if (a.at_least()) {
                a.at(0x10201a88); a.value(0,0x0); a.store<2>(31,0xc,0);
            }
            a.at(0x10201a90);
            emit_rule_26(a);
        }
    }
}
void emit_rule_24(Call& a) {
    a.at(0x1020196c); a.sign8(0,0); a.sign16(6,7); a.jump(0x10201a50);
    discard_stale_context(a);
    a.at(0x10201a58);
    emit_rule_25(a);
}
void emit_rule_23(Call& a) {
    a.at(0x10201954); a.sign8(0,4,true); a.value(0,0x0);
    if (a.equal()) {
        a.at(0x10201960); a.compare_registers(6,5);
        if (!a.equal()) {
        } else {
            a.at(0x10201968); a.value(0,0x1);
        }
    }
    a.at(0x1020196c);
    emit_rule_24(a);
}
void emit_rule_22(Call& a) {
    a.at(0x1020193c); a.load<1>(4,31,0x2); a.load<2>(6,31,0xc,true); a.sign8(0,4,true); a.move(7,6);
    if (a.equal()) {
        a.at(0x10201950); a.value(7,0x0);
    }
    a.at(0x10201954);
    emit_rule_23(a);
}
void emit_rule_21(Call& a) {
    a.at(0x10201914); a.offset(0,23,-0x20000); a.add(3,0,19); a.load<1>(0,3,-0x70f9); a.sign8(0,0,true);
    if (a.equal()) {
        a.at(0x10201a58);
        emit_rule_25(a);
    } else {
        a.at(0x10201928); a.load<2>(5,31,0x8,true); a.offset(0,5,-0x1); a.sign16(3,0,true);
        if (a.less()) {
            a.at(0x10201938); a.value(3,0xb);
        }
        a.at(0x1020193c);
        emit_rule_22(a);
    }
}
void emit_rule_20(Call& a) {
    a.at(0x10201910); a.store<2>(31,0xe,4);
    emit_rule_21(a);
}
void emit_rule_19(Call& a) {
    a.at(0x102017dc); a.flags(0,0,0x0);
    if (append_pending_history(a)) {
        a.at(0x10201910);
        emit_rule_20(a);
    }
}
void emit_rule_18(Call& a) {
    a.at(0x1020175c); a.load<2>(6,31,0x8,true); a.value(7,0x1); a.move(4,6); a.jump(0x10201794);
    find_history_start(a);
    a.at(0x102017b0); a.load<2>(3,31,0xe,true); a.sign16(0,4); a.compare_registers(3,0);
    if (a.equal()) {
        a.at(0x102017c0); a.compare_registers(0,6);
        if (a.equal()) {
        } else {
            a.at(0x102017c8); a.offset(4,4,0x1); a.sign16(0,4); a.compare(0,12);
            if (a.less()) {
            } else {
                a.at(0x102017d8); a.value(4,0x0);
            }
        }
    }
    a.at(0x102017dc);
    emit_rule_19(a);
}
void emit_rule_17(Call& a) {
    a.at(0x1020172c); a.offset(0,23,-0x20000); a.add(3,0,19); a.load<1>(0,3,-0x70fc); a.sign8(0,0,true);
    if (a.positive()) {
        a.at(0x1020175c);
        emit_rule_18(a);
    } else {
        a.at(0x10201740); a.load<1>(0,3,-0x70fe); a.sign8(0,0,true);
        if (!a.equal()) {
            a.at(0x1020175c);
            emit_rule_18(a);
        } else {
            a.at(0x1020174c); a.load<1>(0,31,0x1); a.sign8(0,0); a.compare(0,-1);
            if (a.equal()) {
                a.at(0x10201914);
                emit_rule_21(a);
            } else {
                a.at(0x1020175c);
                emit_rule_18(a);
            }
        }
    }
}
void emit_rule_16(Call& a) {
    a.at(0x10201694); a.load<2>(0,31,0x8,true); a.value(4,0xffffffff); a.multiply(0,0,0x14); a.add(3,31,0);
    a.store<2>(3,0x30,24); a.load<2>(0,31,0x8,true); a.multiply(0,0,0x14); a.add(3,31,0); a.store<1>(3,0x35,25);
    a.load<2>(0,31,0x8,true); a.multiply(0,0,0x14); a.add(3,31,0); a.store<1>(3,0x36,26); a.load<2>(0,31,0x8,true);
    a.multiply(0,0,0x14); a.add(3,31,0); a.store<1>(3,0x37,27); a.load<2>(0,31,0x8,true); a.multiply(0,0,0x14);
    a.add(3,31,0); a.store<1>(3,0x38,28); a.load<2>(0,31,0x8,true); a.multiply(0,0,0x14); a.add(3,31,0);
    a.store(3,0x3c,29); a.load<2>(0,31,0x8,true); a.multiply(0,0,0x14); a.add(3,31,0); a.store<1>(3,0x40,4);
    a.load<1>(0,31,0x3); a.sign8(0,0,true);
    if (!a.equal()) {
        a.at(0x10201714); a.load<2>(0,31,0x8,true); a.move(3,30); a.multiply(4,0,0xb2); a.offset(4,4,0x120);
        a.add(4,31,4); a.call(0x101f7c10);
    }
    a.at(0x1020172c);
    emit_rule_17(a);
}
void emit_rule_15(Call& a) {
    a.at(0x1020163c); a.add(3,23,19); a.value(7,0x0); a.offset(3,3,-0x20000); a.value(6,0x0); a.value(5,0x0);
    a.jump(0x10201684);
    choose_event_variant(a);
    a.at(0x10201694);
    emit_rule_16(a);
}
void emit_rule_14(Call& a) {
    a.at(0x10201538); a.value(4,0x43300000); a.exclusive_flags(0,18,0x80000000); a.load(3,2,-0x52a4);
    a.value(6,0x80010000); a.store(1,0x88,0); a.value(5,0x10000); a.floating_load(2,3,8); a.offset(6,6,-0x7fff);
    a.store(1,0x84,4); a.multiply(0,18,0xa); a.load(7,22,0x0); a.offset(5,5,-0x1); a.floating_load(1,1,132);
    a.floating_load(0,3,0); a.multiply_high(3,6,0); a.floating_subtract(1,1,2); a.load<2>(8,7,0x0);
    a.load(7,21,0x0); a.store(1,0x7c,4); a.floating_difference(1,1,0,true); a.add(7,8,7); a.add(0,3,0);
    a.multiply_high(3,6,7); a.add(3,3,7); a.shift_right(3,3,0xf); a.mask(4,3,0x1,false,1); a.shift_right(0,0,0xf);
    a.add(3,3,4); a.product(4,3,5); a.mask(3,0,0x1,false,1); a.add(3,0,3); a.subtract(0,4,7);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0x80,0); a.floating_load(0,1,124); a.floating_subtract(0,0,2);
    a.floating_product(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,116); a.load(17,1,0x78);
    a.call(0x101ff5f0); a.value(4,0x66660000); a.offset(3,3,0x5); a.offset(4,4,0x6667); a.offset(0,18,-0x1);
    a.multiply_high(3,4,3); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(17,17,3);
    a.compare_registers(17,0);
    if (a.positive()) {
        a.at(0x10201608); a.move(17,0); a.jump(0x1020163c);
    }
    a.at(0x1020163c);
    emit_rule_15(a);
}
void emit_rule_13(Call& a) {
    a.at(0x10201518); a.value(3,0x10000); a.offset(0,3,-0x1); a.compare_registers(18,0);
    if (a.positive()) {
        a.at(0x10201538);
        emit_rule_14(a);
    } else {
        a.at(0x10201528); a.value(3,0xffff0000); a.offset(0,3,0x1); a.compare_registers(18,0);
        if (a.at_least()) {
            a.at(0x10201610); a.load(3,22,0x0); a.compare(18,0); a.load(0,21,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,18); a.product(0,0,18); a.subtract(0,0,3); a.negate(17,0);
            if (a.positive()) {
                a.at(0x10201638); a.move(17,0);
            }
            a.at(0x1020163c);
            emit_rule_15(a);
        } else {
            a.at(0x10201538);
            emit_rule_14(a);
        }
    }
}
void emit_rule_12(Call& a) {
    a.at(0x1020149c); a.load<2>(3,31,0x8,true); a.multiply(19,0,0x14); a.offset(0,23,-0x20000); a.value(17,0x0);
    a.multiply(4,3,0x14); a.add(3,0,19); a.add(4,31,4); a.store<1>(4,0x34,5); a.load<2>(0,31,0x8,true);
    a.multiply(0,0,0x14); a.add(4,31,0); a.store<2>(4,0x32,17); a.load<2>(0,3,-0x70f4,true); a.compare(0,1);
    if (a.nonpositive()) {
        a.at(0x10201694);
        emit_rule_16(a);
    } else {
        a.at(0x102014d8); a.load(18,3,-0x70f0); a.compare(18,0);
        if (!a.equal()) {
            a.at(0x102014e8); a.load(3,2,-0x7b88); a.load(4,22,0x0); a.load(0,3,0x0); a.offset(3,4,0x2);
            a.compare_registers(3,0,false); a.store(22,0x0,3);
            if (a.positive()) {
                a.at(0x10201504); a.load(0,2,-0x7b8c); a.store(22,0x0,0); a.call(0x1077b770);
                a.mask(0,3,0xffff,false,0); a.store(21,0x0,0);
            }
            a.at(0x10201518);
            emit_rule_13(a);
        } else {
            a.at(0x102014e4); a.jump(0x1020163c);
            emit_rule_15(a);
        }
    }
}
void emit_rule_11(Call& a) {
    a.at(0x10201488); a.sign8(3,19); a.move(5,4); a.compare_registers(3,4);
    if (a.less()) {
        a.at(0x10201498); a.move(5,3);
    }
    a.at(0x1020149c);
    emit_rule_12(a);
}
void emit_rule_10(Call& a) {
    a.at(0x10201420); a.compare(5,990);
    if (a.equal()) {
        a.at(0x1020146c); a.value(19,0x59); a.jump(0x10201488);
    } else {
        a.at(0x10201428);
        if (a.at_least()) {
            a.at(0x1020144c); a.compare(5,1320);
            if (a.equal()) {
                a.at(0x1020147c); a.value(19,0x77); a.jump(0x10201488);
            } else {
                a.at(0x10201454);
                if (a.at_least()) {
                } else {
                    a.at(0x10201458); a.compare(5,1155);
                    if (a.equal()) {
                        a.at(0x10201474); a.value(19,0x68); a.jump(0x10201488);
                    } else {
                        a.at(0x10201460); a.jump(0x10201488);
                    }
                }
            }
        } else {
            a.at(0x1020142c); a.compare(5,495);
            if (a.equal()) {
                a.at(0x10201464); a.value(19,0x2c); a.jump(0x10201488);
            } else {
                a.at(0x10201434);
                if (a.at_least()) {
                } else {
                    a.at(0x10201438); a.compare(5,2);
                    if (a.at_least()) {
                    } else {
                        a.at(0x10201440); a.compare(5,0);
                        if (a.at_least()) {
                            a.at(0x10201484); a.sign8(19,4);
                        } else {
                            a.at(0x10201448); a.jump(0x10201488);
                        }
                    }
                }
            }
        }
    }
    a.at(0x10201488);
    emit_rule_11(a);
}
void emit_rule_9(Call& a) {
    a.at(0x102013f4); a.load(3,31,0x4028); a.mask(4,4,0xff,false,0); a.compare_registers(5,3);
    if (!a.equal()) {
        a.at(0x10201404); a.value(3,0x0); a.store<2>(31,0x8,3); a.store<2>(31,0xc,3); a.store<2>(31,0xe,3);
        a.store<1>(31,0x5,3); a.store<1>(31,0x2,3); a.store(31,0x4028,5);
    }
    a.at(0x10201420);
    emit_rule_10(a);
}
void emit_rule_8(Call& a) {
    a.at(0x102013a4); a.sign8(3,26); a.value(4,0x1); a.multiply(3,3,0x3e8); a.sign8(20,5); a.add(3,30,3);
    a.add(3,3,0); a.store<1>(3,-0x1d08,4); a.load<1>(3,30,0x0); a.load<2>(5,30,0x794c,true); a.sign8(3,3,true);
    if (a.equal()) {
        a.at(0x102013d8); a.value(3,0x2e8c0000); a.load<2>(4,30,0x7948,true); a.offset(3,3,-0x5d17);
        a.multiply_high(3,3,4); a.shift_right(3,3,0x1); a.mask(4,3,0x1,false,1); a.add(4,3,4);
    } else {
        a.at(0x102013d0); a.load<2>(4,30,0x7948,true); a.jump(0x102013f4);
    }
    a.at(0x102013f4);
    emit_rule_9(a);
}
void emit_rule_7(Call& a) {
    a.at(0x10201388); a.compare(3,0);
    if (a.equal()) {
        a.at(0x102013a0); a.value(5,0x0);
    } else {
        a.at(0x10201390); a.multiply(3,4,0x14); a.indexed_load<1>(5,23,3); a.sign8(5,5); a.jump(0x102013a4);
    }
    a.at(0x102013a4);
    emit_rule_8(a);
}
void emit_rule_6(Call& a) {
    a.at(0x10201364); a.sign16(0,24); a.value(3,0x0); a.mask(4,0,0x7fff,false,0); a.offset(4,4,-0x1f40);
    a.sign16(4,4,true);
    if (a.at_least()) {
        a.at(0x1020137c); a.compare(4,677);
        if (a.at_least()) {
        } else {
            a.at(0x10201384); a.value(3,0x1);
        }
    }
    a.at(0x10201388);
    emit_rule_7(a);
}
void emit_rule_5(Call& a) {
    a.at(0x1020134c); a.load(5,30,0x30d8); a.move(3,31); a.load<1>(6,30,0x1); a.offset(4,1,0x40);
    a.call(0x101ff1d0); a.move(24,3);
    emit_rule_6(a);
}
void emit_rule_4(Call& a) {
    a.at(0x10201328); a.load(3,30,0x3118); a.compare(3,0,false);
    if (a.equal()) {
        a.at(0x10201364);
        emit_rule_6(a);
    } else {
        a.at(0x10201334); a.load(3,3,0x18); a.compare(3,0,false);
        if (a.equal()) {
            a.at(0x1020134c);
            emit_rule_5(a);
        } else {
            a.at(0x10201340); a.load<1>(0,3,0x4c); a.sign8(0,0,true);
            if (!a.equal()) {
                a.at(0x10201364);
                emit_rule_6(a);
            } else {
                a.at(0x1020134c);
                emit_rule_5(a);
            }
        }
    }
}
void emit_rule_3(Call& a) {
    a.at(0x10201308); a.move(3,31); a.move(5,30); a.call(0x10201180); a.jump(0x10201e1c);
}
void emit_rule_2(Call& a) {
    a.at(0x102012d0); a.load<2>(11,31,0x6,true); a.sign16(0,11,true);
    if (a.positive()) {
        a.at(0x102012dc); a.load<2>(0,31,0x8,true); a.multiply(3,11,0x14); a.multiply(0,0,0x14); a.add(3,31,3);
        a.load<2>(5,3,0x964,true); a.add(3,31,0); a.load<2>(0,3,0x30,true); a.compare_registers(5,0);
        if (!a.equal()) {
        } else {
            a.at(0x10201300); a.offset(0,11,-0x1); a.store<2>(31,0x6,0);
        }
    }
    a.at(0x10201308);
    emit_rule_3(a);
}
void emit_rule_1(Call& a) {
    a.at(0x102011ec); a.offset(3,20,0x18); a.offset(6,1,0x54); a.offset(7,1,0x38); a.value(4,0x0); a.value(5,0x0);
    a.call(0x10739890); a.call(0x10768810); a.move(5,3); a.move(3,18); a.offset(4,20,0x29); a.offset(6,1,0x54);
    a.offset(7,1,0x38); a.value(8,0x48d); a.call(0x1077b720); a.move(4,18); a.offset(3,20,0x36); a.value(5,0x0);
    a.call(0x10769b60); a.load(3,2,-0x7bac); a.value(0,0x0); a.store<1>(3,0x0,0); a.jump(0x10201e1c);
}
}
void emit_event(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10201180,dependencies,observer);
    a.at(0x10201180); a.link(); a.multiple(17,false,0); a.move(26,7); a.load(18,2,-0x7ba8); a.load(21,2,-0x7b90);
    a.move(30,5); a.load(22,2,-0x7b84); a.move(31,3); a.load(20,2,-0x52a0); a.move(24,4); a.load(23,2,-0x5298);
    a.move(25,6); a.move(27,8); a.move(28,9); a.move(29,10); a.save_link(); a.sign8(0,26,true); a.frame(208);
    if (a.less()) {
        a.at(0x102011ec);
        emit_rule_1(a);
    } else {
        a.at(0x102011cc); a.sign8(0,26); a.compare(0,1);
        if (a.positive()) {
            a.at(0x102011ec);
            emit_rule_1(a);
        } else {
            a.at(0x102011d8); a.sign8(0,28); a.compare(0,-1);
            if (a.less()) {
                a.at(0x102011ec);
                emit_rule_1(a);
            } else {
                a.at(0x102011e4); a.compare(0,1);
                if (a.nonpositive()) {
                    a.at(0x10201244); a.sign16(0,24); a.compare(0,8000);
                    if (a.less()) {
                    } else {
                        a.at(0x10201250); a.compare(0,8677);
                        if (a.less()) {
                            a.at(0x1020125c); a.value(0,0xffffffff); a.store<2>(1,0x40,24); a.store<1>(1,0x45,25);
                            a.store<1>(1,0x46,26); a.store<1>(1,0x47,27); a.store<1>(1,0x48,28);
                            a.store(1,0x4c,29); a.store<1>(1,0x50,0); a.load<1>(0,30,0x7929); a.compare(0,2);
                            if (a.equal()) {
                                a.at(0x10201328);
                                emit_rule_4(a);
                            } else {
                                a.at(0x10201288); a.offset(4,1,0x40); a.call(0x101ffb70); a.move(4,3);
                                a.sign16(0,4,true);
                                if (a.nonpositive()) {
                                    a.at(0x10201318); a.sign16(0,4,true);
                                    if (a.less()) {
                                        a.at(0x10201320); a.negate(0,4); a.sign16(24,0);
                                    }
                                    a.at(0x10201328);
                                    emit_rule_4(a);
                                } else {
                                    a.at(0x1020129c); a.load<2>(3,31,0x8,true); a.load(10,1,0x4c);
                                    a.sign16(0,3,true); a.load<1>(9,1,0x48); a.load<1>(8,1,0x47);
                                    a.load<1>(7,1,0x46); a.load<1>(6,1,0x45);
                                    if (!a.equal()) {
                                        a.at(0x102012c8); a.offset(0,3,-0x1); a.store<2>(31,0x8,0);
                                    } else {
                                        a.at(0x102012bc); a.value(0,0xb); a.store<2>(31,0x8,0); a.jump(0x102012d0);
                                    }
                                    a.at(0x102012d0);
                                    emit_rule_2(a);
                                }
                            }
                        } else {
                            a.at(0x10201258); a.jump(0x10201e1c);
                        }
                    }
                } else {
                    a.at(0x102011ec);
                    emit_rule_1(a);
                }
            }
        }
    }
    a.at(0x10201e1c); a.pop(208); a.multiple(17,true,0); a.finish();
}
}
