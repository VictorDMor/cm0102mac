#include "shortlist_assessment_detail.hpp"

namespace cm::aot::shortlist_assessment::detail {
void compare_interest_country_fields(Call& a) {
    a.at(0x1036c5a8);a.load<4>(4,24,0x2c,false);a.compare(4,0x0,false);
    if(a.equal()) {
        return adjust_interest_contract_flags(a);
    } else {
        a.at(0x1036c5b4);a.load<1>(0,6,0x11f,false);a.sign8(3,0,false);a.compare(3,0xa,true);
        if(a.at_least()) {
            return calculate_country_field_difference(a);
        } else {
            a.at(0x1036c5c4);a.load<1>(0,4,0x11f,false);a.sign8(0,0,false);a.compare_registers(3,0,true);
            if(a.less()) {
                return adjust_interest_contract_flags(a);
            } else {
                return calculate_country_field_difference(a);
            }
        }
    }
}

void calculate_country_field_difference(Call& a) {
    a.at(0x1036c5d4);a.sign16(3,15,false);a.sign16(0,31,false);a.subtract(3,3,0);
    a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xe4,0);a.floating_load(1,16,0x8,false);
    a.store<4>(1,0xe8,3);a.floating_load(2,16,0x68,false);a.floating_load(0,1,0xe4,false);a.floating_subtract(0,0,1,false);
    a.floating_product(3,2,0);a.floating_round_single(3,3);a.jump(0x1036c8bc);
    return finish_interest_score(a);
}

void adjust_interest_contract_flags(Call& a) {
    a.at(0x1036c60c);a.compare(27,0x0,false);
    if(a.equal()) {
        return compare_interest_reference_values(a);
    } else {
        a.at(0x1036c614);a.load<4>(0,27,0x50,false);a.mask(0,0,0x10,true,0);
        if(a.equal()) {
            return compare_interest_reference_values(a);
        } else {
            a.at(0x1036c620);a.sign16(3,15,false);a.sign16(0,31,false);a.compare_registers(3,0,true);
            if(a.nonpositive()) {
                return compare_interest_reference_values(a);
            } else {
                a.at(0x1036c630);a.subtract(0,0,3);a.compare(0,0x1388,true);
                if(a.at_least()) {
                    return compare_interest_reference_values(a);
                } else {
                    a.at(0x1036c63c);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0xe8,3);
                    a.floating_load(2,16,0x8,false);a.store<4>(1,0xe4,0);a.floating_load(1,16,0x448,false);a.floating_load(0,1,0xe4,false);
                    a.store<4>(1,0xf0,3);a.floating_subtract(0,0,2,true);a.store<4>(1,0xec,0);a.floating_product(1,1,0);
                    a.floating_load(0,1,0xec,false);a.floating_subtract(0,0,2,true);a.floating_round_single(1,1);a.floating_product_single(3,1,0);
                    a.jump(0x1036c8bc);
                }
            }
        }
    }
    return finish_interest_score(a);
}

void compare_interest_reference_values(Call& a) {
    a.at(0x1036c67c);a.sign16(4,31,false);a.sign16(3,15,false);a.compare_registers(4,3,true);
    if(a.nonpositive()) {
        a.at(0x1036c730);
        if(a.positive()) {
            return choose_interest_reference_scale(a);
        } else {
            a.at(0x1036c734);a.sign8(0,20,false);a.compare(0,0x3,true);
            if(a.nonpositive()) {
                return choose_interest_reference_scale(a);
            } else {
                a.at(0x1036c740);a.subtract(3,3,4);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xe4,0);a.floating_load(1,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(2,16,0x178,false);
                a.floating_load(0,1,0xe4,false);a.floating_subtract(0,0,1,false);a.floating_product(3,2,0);a.floating_round_single(3,3);
                a.jump(0x1036c8bc);
            }
        }
    } else {
        a.at(0x1036c68c);a.compare(27,0x0,false);
        if(a.equal()) {
            return calculate_primary_reference_difference(a);
        } else {
            a.at(0x1036c694);a.load<4>(0,27,0x50,false);a.mask(0,0,0x1,true,0);
            if(a.equal()) {
                return calculate_primary_reference_difference(a);
            } else {
                a.at(0x1036c6a0);a.subtract(3,3,4);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xe4,0);a.floating_load(1,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(2,16,0xe0,false);
                a.floating_load(0,1,0xe4,false);a.floating_subtract(0,0,1,false);a.floating_product(3,2,0);a.floating_round_single(3,3);
                a.jump(0x1036c8bc);
            }
        }
    }
    return finish_interest_score(a);
}

void calculate_primary_reference_difference(Call& a) {
    a.at(0x1036c6d0);a.load<1>(0,17,0x63,false);a.value(4,0x43300000);a.store<4>(1,0xe4,4);
    a.sign16(3,15,false);a.sign8(5,0,false);a.sign16(0,31,false);a.exclusive_flags(5,5,0x80000000);
    a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,5);a.subtract(0,3,0);a.exclusive_flags(0,0,0x80000000);
    a.floating_load(0,16,0x388,false);a.floating_load(1,1,0xe4,false);a.store<4>(1,0xf0,0);a.floating_subtract(1,1,2,false);
    a.floating_load(3,16,0xa0,false);a.store<4>(1,0xec,4);a.floating_difference(1,1,0,true,false);a.floating_load(0,1,0xec,false);
    a.floating_sum(1,3,1);a.floating_subtract(0,0,2,false);a.floating_product(3,1,0);a.floating_round_single(3,3);
    a.jump(0x1036c8bc);
    return finish_interest_score(a);
}

void choose_interest_reference_scale(Call& a) {
    a.at(0x1036c770);a.sign16(4,31,false);a.sign16(3,15,false);a.compare_registers(4,3,true);
    if(a.positive()) {
        return calculate_fallback_reference_difference(a);
    } else {
        a.at(0x1036c780);a.floating_load(5,16,0x150,false);a.floating_compare(31,5);
        if(a.nonpositive()) {
            return calculate_fallback_reference_difference(a);
        } else {
            a.at(0x1036c78c);a.compare(3,0x1b58,true);
            if(a.nonpositive()) {
                return calculate_lower_reference_scale(a);
            } else {
                a.at(0x1036c794);a.compare(4,0x7d0,true);
                if(a.at_least()) {
                    return calculate_lower_reference_scale(a);
                } else {
                    a.at(0x1036c79c);a.sign8(0,19,true);
                    if(a.equal()) {
                        return calculate_lower_reference_scale(a);
                    } else {
                        a.at(0x1036c7a4);a.subtract(3,4,3);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                        a.floating_load(0,16,0xb8,false);a.store<4>(1,0xe8,3);a.load<4>(3,2,-0x4de8,false);a.floating_compare(31,0);
                        a.store<4>(1,0xe4,0);a.floating_load(3,16,0x8,false);a.floating_load(2,1,0xe4,false);a.floating_load(4,3,0x28,true);
                        a.floating_subtract(3,2,3,true);a.floating_load(1,3,0x24,true);a.floating_load(2,16,0x200,false);
                        a.negative_product_difference_single_at(0x1036c7dc,1,4,3,1);
                        if(!a.at_least()) {
                            a.at(0x1036c7e4);a.floating_move(0,31);
                        }
                        a.at(0x1036c7e8);a.floating_product(0,1,0);a.floating_compare(5,0);
                        if(a.nonpositive()) {
                            a.at(0x1036c7fc);a.floating_load(0,16,0xb8,false);a.floating_compare(31,0);
                            if(!a.at_least()) {
                                a.at(0x1036c808);a.floating_move(0,31);
                            }
                            a.at(0x1036c80c);a.floating_product(0,1,0);
                        } else {
                            a.at(0x1036c7f4);a.floating_load(0,16,0x150,false);a.jump(0x1036c810);
                        }
                        a.at(0x1036c810);a.floating_round_single(1,0);a.floating_load(0,16,0x30,false);a.floating_difference(0,1,0,true,false);
                        a.floating_subtract(1,2,0,false);a.floating_round_single(1,1);a.jump(0x1036c83c);
                    }
                }
            }
            return limit_interest_reference_scale(a);
        }
    }
}

void calculate_lower_reference_scale(Call& a) {
    a.at(0x1036c828);a.floating_load(0,16,0x30,false);a.floating_load(1,16,0x200,false);a.floating_difference(0,31,0,true,false);
    a.floating_subtract(1,1,0,false);a.floating_round_single(1,1);
    return limit_interest_reference_scale(a);
}

void limit_interest_reference_scale(Call& a) {
    a.at(0x1036c83c);a.floating_load(0,16,0x48,false);a.floating_compare(0,1);
    if(a.nonpositive()) {
        a.at(0x1036c84c);a.floating_load(0,16,0x80,false);a.floating_compare(0,1);
        if(a.at_least()) {
            a.at(0x1036c85c);a.floating_move(0,1);
        } else {
            a.at(0x1036c858);a.jump(0x1036c860);
        }
    } else {
        a.at(0x1036c848);a.jump(0x1036c860);
    }
    a.at(0x1036c860);a.sign16(3,15,false);a.sign16(0,31,false);a.subtract(3,3,0);
    a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xe4,0);a.floating_load(1,16,0x8,false);
    a.floating_round_single(2,0);a.store<4>(1,0xe8,3);a.floating_load(0,1,0xe4,false);a.floating_subtract(0,0,1,true);
    a.floating_product_single(3,2,0);a.jump(0x1036c8bc);
    return finish_interest_score(a);
}

void calculate_fallback_reference_difference(Call& a) {
    a.at(0x1036c894);a.sign16(3,15,false);a.sign16(0,31,false);a.subtract(3,3,0);
    a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0xe4,0);a.floating_load(1,16,0x8,false);
    a.store<4>(1,0xe8,3);a.floating_load(0,1,0xe4,false);a.floating_subtract(3,0,1,true);
    return finish_interest_score(a);
}

void finish_interest_score(Call& a) {
    a.at(0x1036c8bc);a.floating_load(0,16,0x158,false);a.floating_compare(3,0);
    if(!a.nonpositive()) {
        a.at(0x1036c8c8);a.floating_load(0,16,0x80,false);a.floating_compare(27,0);
        if(!a.at_least()) {
            a.at(0x1036c8d4);a.floating_load(0,16,0xb8,false);a.floating_load(2,16,0x30,false);a.floating_difference(0,3,0,true,false);
            a.floating_compare(2,0);
            if(a.at_least()) {
                a.at(0x1036c8ec);a.floating_move(2,0);
            } else {
                a.at(0x1036c8e8);a.jump(0x1036c8f0);
            }
            a.at(0x1036c8f0);a.floating_load(1,16,0x30,false);a.compare(21,0x0,false);a.floating_subtract(0,1,2,false);
            a.floating_product_sum(0,0,27,2);a.floating_difference(1,0,1,true,false);a.floating_product(31,31,1);
            if(!a.equal()) {
                a.at(0x1036c90c);a.floating_load(0,16,0x180,false);a.floating_compare(1,0);
                if(!a.at_least()) {
                    a.at(0x1036c918);a.load<4>(0,21,0x0,false);a.flags(0,0,0x100000);a.store<4>(21,0x0,0);
                }
            }
        }
    }
    a.at(0x1036c924);a.floating_load(0,16,0x450,false);a.floating_load(2,16,0x80,false);a.floating_difference(1,3,0,true,false);
    a.floating_load(0,16,0x370,false);a.floating_sum(1,2,1);a.floating_compare(1,0);
    if(!a.at_least()) {
        a.at(0x1036c940);a.floating_move(1,0);
    }
    a.at(0x1036c944);a.compare(27,0x0,false);
    if(!a.equal()) {
        a.at(0x1036c94c);a.load<4>(0,27,0x50,false);a.mask(0,0,0x100,true,0);
        if(!a.equal()) {
            a.at(0x1036c958);a.floating_load(0,16,0x178,false);a.floating_product(1,1,0);
        }
    }
    a.at(0x1036c960);a.floating_product(31,31,1);a.compare(21,0x0,false);
    if(!a.equal()) {
        a.at(0x1036c96c);a.floating_load(0,16,0x180,false);a.floating_compare(1,0);
        if(a.at_least()) {
            a.at(0x1036c9e8);a.floating_load(0,16,0x178,false);a.floating_compare(1,0);
            if(!a.nonpositive()) {
                a.at(0x1036c9f4);a.floating_load(0,16,0x170,false);a.floating_compare(30,0);
                if(!a.nonpositive()) {
                    a.at(0x1036ca00);a.compare(28,0x0,false);
                    if(a.equal()) {
                        a.at(0x1036ca08);a.compare(14,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036ca10);a.compare(24,0x0,false);
                            if(a.equal()) {
                                a.at(0x1036ca24);a.sign8(0,20,false);a.compare(0,0x4,true);
                                if(!a.at_least()) {
                                    a.at(0x1036ca30);a.sign8(0,20,true);
                                    if(!a.nonpositive()) {
                                        a.at(0x1036ca38);a.compare(27,0x0,false);
                                        if(a.equal()) {
                                            a.at(0x1036ca4c);a.load<4>(0,21,0x0,false);a.flags(0,0,0x200000);a.store<4>(21,0x0,0);
                                        } else {
                                            a.at(0x1036ca40);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
                                            if(a.equal()) {
                                                a.at(0x1036ca4c);a.load<4>(0,21,0x0,false);a.flags(0,0,0x200000);a.store<4>(21,0x0,0);
                                            }
                                        }
                                    }
                                }
                            } else {
                                a.at(0x1036ca18);a.load<2>(0,24,0x1ec,true);a.compare(0,0x1d4c,true);
                                if(!a.at_least()) {
                                    a.at(0x1036ca24);a.sign8(0,20,false);a.compare(0,0x4,true);
                                    if(!a.at_least()) {
                                        a.at(0x1036ca30);a.sign8(0,20,true);
                                        if(!a.nonpositive()) {
                                            a.at(0x1036ca38);a.compare(27,0x0,false);
                                            if(a.equal()) {
                                                a.at(0x1036ca4c);a.load<4>(0,21,0x0,false);a.flags(0,0,0x200000);a.store<4>(21,0x0,0);
                                            } else {
                                                a.at(0x1036ca40);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
                                                if(a.equal()) {
                                                    a.at(0x1036ca4c);a.load<4>(0,21,0x0,false);a.flags(0,0,0x200000);a.store<4>(21,0x0,0);
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
        } else {
            a.at(0x1036c978);a.floating_load(0,16,0x170,false);a.floating_compare(30,0);
            if(a.nonpositive()) {
                a.at(0x1036c9d8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                a.jump(0x1036ca58);
            } else {
                a.at(0x1036c984);a.compare(28,0x0,false);
                if(a.equal()) {
                    a.at(0x1036c9d8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                    a.jump(0x1036ca58);
                } else {
                    a.at(0x1036c98c);a.compare(14,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1036c9d8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                        a.jump(0x1036ca58);
                    } else {
                        a.at(0x1036c994);a.floating_load(0,16,0xc8,false);a.floating_compare(31,0);
                        if(a.nonpositive()) {
                            a.at(0x1036c9d8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                            a.jump(0x1036ca58);
                        } else {
                            a.at(0x1036c9a0);a.sign8(0,26,false);a.compare(0,0x4,true);
                            if(a.at_least()) {
                                a.at(0x1036c9d8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                                a.jump(0x1036ca58);
                            } else {
                                a.at(0x1036c9ac);a.sign8(0,26,true);
                                if(a.nonpositive()) {
                                    a.at(0x1036c9d8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                                    a.jump(0x1036ca58);
                                } else {
                                    a.at(0x1036c9b4);a.compare(27,0x0,false);
                                    if(a.equal()) {
                                        a.at(0x1036c9c8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x400000);a.store<4>(21,0x0,0);
                                        a.jump(0x1036ca58);
                                    } else {
                                        a.at(0x1036c9bc);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
                                        if(!a.equal()) {
                                            a.at(0x1036c9d8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                                            a.jump(0x1036ca58);
                                        } else {
                                            a.at(0x1036c9c8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x400000);a.store<4>(21,0x0,0);
                                            a.jump(0x1036ca58);
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
    a.at(0x1036ca58);a.sign8(0,19,true);
    if(a.equal()) {
        a.at(0x1036ca60);a.floating_product(31,31,1);
    }
    a.at(0x1036ca64);a.compare(27,0x0,false);
    if(!a.equal()) {
        a.at(0x1036ca6c);a.load<4>(0,27,0x50,false);a.mask(0,0,0x1,true,0);
        if(!a.equal()) {
            a.at(0x1036ca78);a.floating_product(31,31,1);a.compare(21,0x0,false);
            if(!a.equal()) {
                a.at(0x1036ca84);a.floating_load(0,16,0xf8,false);a.floating_compare(1,0);
                if(a.at_least()) {
                    a.at(0x1036cb00);a.floating_load(0,16,0xe8,false);a.floating_compare(1,0);
                    if(!a.nonpositive()) {
                        a.at(0x1036cb0c);a.floating_load(0,16,0x170,false);a.floating_compare(30,0);
                        if(!a.nonpositive()) {
                            a.at(0x1036cb18);a.compare(28,0x0,false);
                            if(a.equal()) {
                                a.at(0x1036cb20);a.compare(14,0x0,false);
                                if(!a.equal()) {
                                    a.at(0x1036cb28);a.compare(24,0x0,false);
                                    if(a.equal()) {
                                        a.at(0x1036cb3c);a.sign8(0,20,false);a.compare(0,0x4,true);
                                        if(!a.at_least()) {
                                            a.at(0x1036cb48);a.sign8(0,20,true);
                                            if(!a.nonpositive()) {
                                                a.at(0x1036cb50);a.compare(27,0x0,false);
                                                if(a.equal()) {
                                                    a.at(0x1036cb64);a.load<4>(0,21,0x0,false);a.flags(0,0,0x200000);a.store<4>(21,0x0,0);
                                                } else {
                                                    a.at(0x1036cb58);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
                                                    if(a.equal()) {
                                                        a.at(0x1036cb64);a.load<4>(0,21,0x0,false);a.flags(0,0,0x200000);a.store<4>(21,0x0,0);
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        a.at(0x1036cb30);a.load<2>(0,24,0x1ec,true);a.compare(0,0x1d4c,true);
                                        if(!a.at_least()) {
                                            a.at(0x1036cb3c);a.sign8(0,20,false);a.compare(0,0x4,true);
                                            if(!a.at_least()) {
                                                a.at(0x1036cb48);a.sign8(0,20,true);
                                                if(!a.nonpositive()) {
                                                    a.at(0x1036cb50);a.compare(27,0x0,false);
                                                    if(a.equal()) {
                                                        a.at(0x1036cb64);a.load<4>(0,21,0x0,false);a.flags(0,0,0x200000);a.store<4>(21,0x0,0);
                                                    } else {
                                                        a.at(0x1036cb58);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
                                                        if(a.equal()) {
                                                            a.at(0x1036cb64);a.load<4>(0,21,0x0,false);a.flags(0,0,0x200000);a.store<4>(21,0x0,0);
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
                } else {
                    a.at(0x1036ca90);a.floating_load(0,16,0x170,false);a.floating_compare(30,0);
                    if(a.nonpositive()) {
                        a.at(0x1036caf0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                        a.jump(0x1036cb70);
                    } else {
                        a.at(0x1036ca9c);a.compare(28,0x0,false);
                        if(a.equal()) {
                            a.at(0x1036caf0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                            a.jump(0x1036cb70);
                        } else {
                            a.at(0x1036caa4);a.compare(14,0x0,false);
                            if(!a.equal()) {
                                a.at(0x1036caf0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                                a.jump(0x1036cb70);
                            } else {
                                a.at(0x1036caac);a.floating_load(0,16,0xc8,false);a.floating_compare(31,0);
                                if(a.nonpositive()) {
                                    a.at(0x1036caf0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                                    a.jump(0x1036cb70);
                                } else {
                                    a.at(0x1036cab8);a.sign8(0,26,false);a.compare(0,0x4,true);
                                    if(a.at_least()) {
                                        a.at(0x1036caf0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                                        a.jump(0x1036cb70);
                                    } else {
                                        a.at(0x1036cac4);a.sign8(0,26,true);
                                        if(a.nonpositive()) {
                                            a.at(0x1036caf0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                                            a.jump(0x1036cb70);
                                        } else {
                                            a.at(0x1036cacc);a.compare(27,0x0,false);
                                            if(a.equal()) {
                                                a.at(0x1036cae0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x400000);a.store<4>(21,0x0,0);
                                                a.jump(0x1036cb70);
                                            } else {
                                                a.at(0x1036cad4);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
                                                if(!a.equal()) {
                                                    a.at(0x1036caf0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
                                                    a.jump(0x1036cb70);
                                                } else {
                                                    a.at(0x1036cae0);a.load<4>(0,21,0x0,false);a.flags(0,0,0x400000);a.store<4>(21,0x0,0);
                                                    a.jump(0x1036cb70);
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
        }
    }
    a.at(0x1036cb70);a.sign8(0,19,true);
    if(!a.equal()) {
        a.at(0x1036cb78);a.load<4>(4,17,0x14,false);a.sign16(3,15,false);a.load<2>(0,4,0xa,true);
        a.compare_registers(0,3,true);
        if(!a.nonpositive()) {
            a.at(0x1036cb8c);a.sign16(0,31,false);a.compare_registers(3,0,true);
            if(!a.nonpositive()) {
                a.at(0x1036cb98);a.subtract(3,3,0);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xe4,0);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(1,16,0x350,false);
                a.floating_load(0,1,0xe4,false);a.floating_load(3,16,0x80,false);a.floating_subtract(2,0,2,false);a.floating_load(0,16,0x48,false);
                a.floating_difference(1,2,1,true,false);a.floating_sum(1,3,1);a.floating_product(28,28,1);a.floating_compare(28,0);
                if(!a.at_least()) {
                    a.at(0x1036cbd8);a.floating_move(28,0);
                }
            }
        }
    }
    a.at(0x1036cbdc);a.sign8(0,19,true);
    if(a.equal()) {
        a.at(0x1036cc48);a.compare(24,0x0,false);
        if(a.equal()) {
            a.at(0x1036cd50);a.compare(24,0x0,false);
            if(!a.equal()) {
                a.at(0x1036cd58);a.load<4>(3,17,0xc,false);a.load<4>(4,3,0x74,false);a.compare(4,0x0,false);
                if(!a.equal()) {
                    a.at(0x1036cd68);a.load<4>(3,2,-0x7a70,false);a.load<4>(0,4,0x8,false);a.load<4>(4,3,0x0,false);
                    a.compare_registers(0,4,true);
                    if(a.equal()) {
                        a.at(0x1036cd7c);a.load<4>(3,18,0x2c,false);a.load<4>(5,3,0x74,false);a.compare(5,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036cd8c);a.load<4>(0,5,0x8,false);a.compare_registers(0,4,true);
                            if(!a.equal()) {
                                a.at(0x1036cd98);a.load<4>(3,24,0x2c,false);a.load<4>(0,3,0x74,false);a.compare_registers(5,0,false);
                                if(!a.equal()) {
                                    a.at(0x1036cda8);a.floating_load(0,16,0x348,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                    if(!a.equal()) {
                                        a.at(0x1036cdb8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x2000000);a.store<4>(21,0x0,0);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            a.at(0x1036cc50);a.sign8(3,29,false);a.sign8(4,30,false);a.compare_registers(3,4,true);
            if(a.equal()) {
                a.at(0x1036cd50);a.compare(24,0x0,false);
                if(!a.equal()) {
                    a.at(0x1036cd58);a.load<4>(3,17,0xc,false);a.load<4>(4,3,0x74,false);a.compare(4,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1036cd68);a.load<4>(3,2,-0x7a70,false);a.load<4>(0,4,0x8,false);a.load<4>(4,3,0x0,false);
                        a.compare_registers(0,4,true);
                        if(a.equal()) {
                            a.at(0x1036cd7c);a.load<4>(3,18,0x2c,false);a.load<4>(5,3,0x74,false);a.compare(5,0x0,false);
                            if(!a.equal()) {
                                a.at(0x1036cd8c);a.load<4>(0,5,0x8,false);a.compare_registers(0,4,true);
                                if(!a.equal()) {
                                    a.at(0x1036cd98);a.load<4>(3,24,0x2c,false);a.load<4>(0,3,0x74,false);a.compare_registers(5,0,false);
                                    if(!a.equal()) {
                                        a.at(0x1036cda8);a.floating_load(0,16,0x348,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                                        if(!a.equal()) {
                                            a.at(0x1036cdb8);a.load<4>(0,21,0x0,false);a.flags(0,0,0x2000000);a.store<4>(21,0x0,0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                a.at(0x1036cc60);a.value(6,0x43300000);a.load<2>(0,1,0x130,true);a.exclusive_flags(3,3,0x80000000);
                a.store<4>(1,0xe4,6);a.exclusive_flags(5,0,0x80000000);a.exclusive_flags(4,4,0x80000000);a.store<4>(1,0xe8,3);
                a.value(3,0x1f4);a.floating_load(4,16,0x8,false);a.floating_load(0,1,0xe4,false);a.store<4>(1,0xf0,5);
                a.floating_subtract(2,0,4,false);a.load<2>(0,1,0x12e,true);a.store<4>(1,0xec,6);a.compare(0,0x1f4,true);
                a.floating_load(0,1,0xec,false);a.store<4>(1,0xf8,4);a.floating_subtract(1,0,4,false);a.store<4>(1,0xf4,6);
                a.floating_load(0,1,0xf4,false);a.floating_product(3,2,1);a.floating_subtract(2,0,4,false);
                if(!a.nonpositive()) {
                    a.at(0x1036ccbc);a.move(3,0);
                }
                a.at(0x1036ccc0);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x100,3);
                a.floating_load(1,16,0x8,false);a.store<4>(1,0xfc,0);a.floating_load(4,16,0x148,false);a.floating_load(0,1,0xfc,false);
                a.floating_subtract(0,0,1,false);a.floating_product(0,2,0);a.floating_difference(0,3,0,true,false);a.floating_compare(4,0);
                if(a.nonpositive()) {
                    a.at(0x1036ccf4);a.floating_load(4,16,0x150,false);a.floating_compare(4,0);
                    if(a.at_least()) {
                        a.at(0x1036cd04);a.floating_move(4,0);
                    } else {
                        a.at(0x1036cd00);a.jump(0x1036cd08);
                    }
                } else {
                    a.at(0x1036ccf0);a.jump(0x1036cd08);
                }
                a.at(0x1036cd08);a.load<4>(4,18,0x2c,false);a.floating_product(31,31,4);a.load<4>(3,24,0x2c,false);
                a.load<4>(4,4,0x74,false);a.load<4>(0,3,0x74,false);a.compare_registers(4,0,false);
                if(!a.equal()) {
                    a.at(0x1036cd24);a.floating_load(0,16,0x80,false);a.floating_compare(4,0);
                    if(!a.at_least()) {
                        a.at(0x1036cd30);a.floating_load(0,16,0x58,false);a.compare(21,0x0,false);a.floating_product(31,31,0);
                        if(!a.equal()) {
                            a.at(0x1036cd40);a.load<4>(0,21,0x0,false);a.flags(0,0,0x2000000);a.store<4>(21,0x0,0);
                            a.jump(0x1036cdc4);
                        }
                    }
                }
            }
        }
    } else {
        a.at(0x1036cbe4);a.sign8(4,29,false);a.sign8(0,30,false);a.value(3,0x43300000);
        a.floating_load(2,16,0x8,false);a.exclusive_flags(4,4,0x80000000);a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0xe8,4);
        a.compare(21,0x0,false);a.store<4>(1,0xe4,3);a.floating_load(0,1,0xe4,false);a.store<4>(1,0xf0,0);
        a.floating_subtract(1,0,2,false);a.store<4>(1,0xec,3);a.floating_load(0,1,0xec,false);a.floating_subtract(0,0,2,false);
        a.floating_difference(1,1,0,true,false);a.floating_product(31,31,1);
        if(!a.equal()) {
            a.at(0x1036cc2c);a.floating_load(0,16,0x180,false);a.floating_compare(1,0);
            if(!a.at_least()) {
                a.at(0x1036cc38);a.load<4>(0,21,0x0,false);a.flags(0,0,0x1000000);a.store<4>(21,0x0,0);
                a.jump(0x1036cdc4);
            }
        }
    }
    a.at(0x1036cdc4);a.floating_product(31,31,28);a.compare(21,0x0,false);
    if(!a.equal()) {
        a.at(0x1036cdd0);a.floating_load(0,16,0x180,false);a.floating_compare(28,0);
        if(!a.at_least()) {
            a.at(0x1036cddc);a.load<4>(0,21,0x0,false);a.flags(0,0,0x800000);a.store<4>(21,0x0,0);
        }
    }
    a.at(0x1036cde8);a.compare(24,0x0,false);
    if(!a.equal()) {
        a.at(0x1036cdf0);a.compare_registers(24,18,false);
        if(!a.equal()) {
            a.at(0x1036cdf8);a.load<4>(3,17,0x14,false);a.load<2>(4,3,0x6,true);a.compare(4,0x82,true);
            if(!a.nonpositive()) {
                a.at(0x1036ce08);a.value(3,0x88890000);a.load<2>(0,24,0x1ec,true);a.offset(3,3,-0x7777);
                a.multiply_high(3,3,0);a.add(0,3,0);a.shift_right(0,0,5);a.mask(3,0,0x1,false,1);
                a.add(0,0,3);a.compare_registers(4,0,true);
                if(!a.nonpositive()) {
                    a.at(0x1036ce30);a.compare(22,0x1a,true);
                    if(!a.at_least()) {
                        a.at(0x1036ce38);a.move(3,24);a.offset(4,1,0x78);a.offset(5,1,0x70);
                        a.call(0x101929b0);a.sign8(0,3,true);
                        if(!a.equal()) {
                            a.at(0x1036ce50);a.sign8(0,25,true);
                            if(!a.nonpositive()) {
                                a.at(0x1036ce58);a.compare(27,0x0,false);
                                if(!a.equal()) {
                                    a.at(0x1036ce60);a.load<4>(3,1,0x70,false);a.load<4>(0,1,0x74,false);a.store<4>(1,0x38,3);
                                    a.load<2>(3,27,0x32,false);a.load<2>(4,1,0x3a,false);a.store<4>(1,0x3c,0);a.compare_registers(4,3,false);
                                    if(a.at_least()) {
                                        a.at(0x1036ce88);
                                        if(a.nonpositive()) {
                                            a.at(0x1036ce94);a.load<2>(0,1,0x38,true);a.load<2>(4,27,0x30,true);a.combine(0,4,0,false,true);
                                            a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,0x1,false,1);
                                            a.sign8(0,0,false);
                                        } else {
                                            a.at(0x1036ce8c);a.value(0,0x0);a.jump(0x1036ceb4);
                                        }
                                    } else {
                                        a.at(0x1036ce80);a.value(0,0x1);a.jump(0x1036ceb4);
                                    }
                                    a.at(0x1036ceb4);a.sign8(0,0,true);
                                    if(!a.equal()) {
                                        a.at(0x1036cebc);a.load<4>(0,27,0x50,false);a.compare(0,0x0,true);
                                        if(a.equal()) {
                                            a.at(0x1036cec8);a.load<1>(0,27,0x54,false);a.sign8(0,0,true);
                                            if(a.equal()) {
                                                a.at(0x1036ced4);a.move(3,18);a.value(4,0x1);a.value(5,0x1);
                                                a.value(6,0x0);a.value(7,0x0);a.call(0x1014c2e0);a.sign8(0,3,true);
                                                if(a.equal()) {
                                                    a.at(0x1036cf04);a.load<4>(3,2,-0x7b80,false);a.load<4>(5,2,-0x7b80,false);a.load<4>(4,3,0x0,false);
                                                    a.offset(3,1,0x70);a.load<4>(5,5,0x4,false);a.call(0x10718130);a.compare(3,0x96,true);
                                                    if(!a.at_least()) {
                                                        a.at(0x1036cf24);a.compare(21,0x0,false);
                                                        if(!a.equal()) {
                                                            a.at(0x1036cf2c);a.value(3,0x80000000);a.load<4>(4,21,0x0,false);a.offset(0,3,0x0);
                                                            a.combine(0,4,0,false,false);a.store<4>(21,0x0,0);
                                                        }
                                                        a.at(0x1036cf40);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                                                        return;
                                                    }
                                                } else {
                                                    a.at(0x1036cef4);a.load<1>(0,17,0x66,false);a.sign8(0,0,false);a.compare(0,0xf,true);
                                                    if(!a.nonpositive()) {
                                                        a.at(0x1036cf04);a.load<4>(3,2,-0x7b80,false);a.load<4>(5,2,-0x7b80,false);a.load<4>(4,3,0x0,false);
                                                        a.offset(3,1,0x70);a.load<4>(5,5,0x4,false);a.call(0x10718130);a.compare(3,0x96,true);
                                                        if(!a.at_least()) {
                                                            a.at(0x1036cf24);a.compare(21,0x0,false);
                                                            if(!a.equal()) {
                                                                a.at(0x1036cf2c);a.value(3,0x80000000);a.load<4>(4,21,0x0,false);a.offset(0,3,0x0);
                                                                a.combine(0,4,0,false,false);a.store<4>(21,0x0,0);
                                                            }
                                                            a.at(0x1036cf40);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
                                                            return;
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
                }
            }
        }
    }
    a.at(0x1036cf48);a.load<1>(0,1,0x1f7,false);a.sign8(0,0,true);
    if(a.equal()) {
        a.at(0x1036cf68);a.floating_move(1,31);a.jump(0x1036d21c);
        return;
    }
    a.at(0x1036cf54);a.sign8(0,19,true);
    if(!a.equal()) {
        a.at(0x1036cf68);a.floating_move(1,31);a.jump(0x1036d21c);
        return;
    }
    a.at(0x1036cf5c);a.floating_load(0,16,0x3a8,false);a.floating_compare(31,0);
    if(!a.at_least()) {
        a.at(0x1036cf68);a.floating_move(1,31);a.jump(0x1036d21c);
        return;
    }
    a.at(0x1036cf70);a.offset(3,1,0x80);a.call(0x103721f0);a.load<4>(3,2,-0x7ab8,false);
    a.move(4,18);a.load<4>(3,3,0x0,false);a.call(0x10103510);a.combine(13,3,3,true,false);
    if(a.equal()) {
        a.at(0x1036cf90);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
        return;
    }
    a.at(0x1036cf98);a.move(4,17);a.move(5,18);a.move(7,20);
    a.offset(3,1,0x80);a.value(6,0x0);a.value(8,0xb);a.value(9,0x0);
    a.call(0x1037b6f0);a.floating_move(27,1);a.move(3,13);a.move(4,17);
    a.move(5,20);a.call(0x100f2680);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);
    a.store<4>(1,0xe8,3);a.floating_load(0,16,0x48,false);a.store<4>(1,0xe4,0);a.floating_load(2,16,0x8,false);
    a.floating_compare(0,31);a.floating_load(1,1,0xe4,false);a.floating_load(28,16,0x158,false);a.floating_subtract(26,1,2,false);
    a.floating_load(1,16,0x160,false);
    if(a.nonpositive()) {
        a.at(0x1036d000);a.floating_move(0,31);
    } else {
        a.at(0x1036cffc);a.jump(0x1036d004);
    }
    a.at(0x1036d004);a.floating_difference(29,1,0,true,false);a.floating_load(0,16,0x168,false);a.floating_compare(29,0);
    if(!a.at_least()) {
        a.at(0x1036d014);a.floating_move(29,0);
    }
    a.at(0x1036d018);a.floating_product(27,29,27);a.compare(27,0x0,false);
    if(!a.equal()) {
        a.at(0x1036d024);a.mask(0,23,0x100,true,0);
        if(!a.equal()) {
            a.at(0x1036d02c);a.floating_load(0,16,0x150,false);a.floating_sum(28,28,0);
        }
        a.at(0x1036d034);a.move(3,17);a.move(4,24);a.call(0x100d3150);
        a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x1036d048);a.floating_load(0,16,0x150,false);a.floating_sum(28,28,0);
        }
        a.at(0x1036d050);a.load<1>(3,27,0x44,false);a.sign8(0,3,true);
        if(!a.at_least()) {
            a.at(0x1036d05c);a.sign8(3,3,false);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);
            a.store<4>(1,0xe4,0);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,3);a.floating_load(0,16,0x458,false);
            a.floating_load(1,1,0xe4,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);a.floating_sum(28,28,0);
        }
        a.at(0x1036d088);a.compare(23,0x0,true);
        if(!a.equal()) {
            a.at(0x1036d090);a.load<1>(3,27,0x4c,false);a.value(0,0x43300000);a.store<4>(1,0xec,0);
            a.sign8(0,3,false);a.floating_load(2,16,0x8,false);a.exclusive_flags(0,0,0x80000000);a.floating_load(0,16,0x458,false);
            a.store<4>(1,0xf0,0);a.floating_load(1,1,0xec,false);a.floating_subtract(1,1,2,false);a.floating_difference(0,1,0,true,false);
            a.floating_sum(28,28,0);
        }
        a.at(0x1036d0c0);a.floating_load(0,16,0x158,false);a.floating_compare(0,28);
        if(a.nonpositive()) {
            a.at(0x1036d0d4);a.floating_load(0,16,0x238,false);a.floating_compare(0,28);
            if(!a.at_least()) {
                a.at(0x1036d0e0);a.floating_move(28,0);
            }
        } else {
            a.at(0x1036d0cc);a.floating_move(28,0);a.jump(0x1036d0e4);
        }
    }
    a.at(0x1036d0e4);a.floating_load(0,16,0x1b8,false);a.floating_product(0,0,27);a.floating_compare(0,26);
    if(!a.nonpositive()) {
        a.at(0x1036d0f4);a.floating_load(1,16,0x158,false);a.jump(0x1036d21c);
        return;
    }
    a.at(0x1036d0fc);a.floating_compare(27,26);
    if(!a.nonpositive()) {
        a.at(0x1036d104);a.floating_load(0,16,0x68,false);a.floating_product(0,27,0);a.floating_compare(26,0);
        if(!a.nonpositive()) {
            a.at(0x1036d114);a.floating_move(0,26);
        }
        a.at(0x1036d118);a.floating_load(2,16,0x238,false);a.floating_product(0,28,0);a.floating_subtract(1,2,28,false);
        a.floating_product_sum(0,1,27,0);a.floating_difference(27,0,2,true,false);a.floating_round_single(27,27);
    }
    a.at(0x1036d130);a.floating_load(0,16,0x3a8,false);a.floating_compare(31,0);
    if(!a.at_least()) {
        a.at(0x1036d13c);a.floating_load(0,16,0x350,false);a.floating_sum(27,27,0);
    }
    a.at(0x1036d144);a.floating_compare(27,26);
    if(!a.nonpositive()) {
        a.at(0x1036d14c);a.floating_load(0,16,0x158,false);a.floating_compare(27,0);
        if(!a.nonpositive()) {
            a.at(0x1036d158);a.floating_difference(1,26,27,true,false);a.floating_load(0,16,0x1f0,false);a.floating_compare(1,0);
            if(!a.at_least()) {
                a.at(0x1036d168);a.floating_load(1,16,0x48,false);a.jump(0x1036d21c);
                return;
            }
            a.at(0x1036d170);a.floating_product(31,31,1);a.floating_move(27,26);
        }
    }
    a.at(0x1036d178);a.compare(27,0x0,false);
    if(!a.equal()) {
        a.at(0x1036d180);a.load<4>(3,27,0xc,false);a.value(0,0x43300000);a.store<4>(1,0xe4,0);
        a.exclusive_flags(0,3,0x80000000);a.floating_load(2,16,0x8,false);a.store<4>(1,0xe8,0);a.floating_load(0,16,0x150,false);
        a.floating_load(1,1,0xe4,false);a.floating_subtract(1,1,2,false);a.floating_compare(1,0);
        if(!a.at_least()) {
            a.at(0x1036d1ac);a.floating_move(1,0);
        }
        a.at(0x1036d1b0);a.compare_registers(24,18,false);
        if(!a.equal()) {
            a.at(0x1036d1c0);a.floating_difference(0,27,1,true,false);
        } else {
            a.at(0x1036d1b8);a.floating_difference(0,1,27,true,false);a.jump(0x1036d1c4);
        }
        a.at(0x1036d1c4);a.floating_load(2,16,0x48,false);a.floating_compare(2,0);
        if(a.nonpositive()) {
            a.at(0x1036d1d4);a.floating_load(2,16,0x150,false);a.floating_compare(2,0);
            if(a.at_least()) {
                a.at(0x1036d1e4);a.floating_move(2,0);
            } else {
                a.at(0x1036d1e0);a.jump(0x1036d1e8);
            }
        } else {
            a.at(0x1036d1d0);a.jump(0x1036d1e8);
        }
        a.at(0x1036d1e8);a.floating_load(0,16,0x80,false);a.floating_compare(2,0);a.compare_greater_or_equal_bit();
        if(!a.equal()) {
            a.at(0x1036d200);a.floating_load(1,16,0x238,false);a.floating_subtract(0,1,28,false);a.floating_product_sum(0,2,0,28);
            a.floating_difference(0,0,1,true,false);a.floating_difference(0,0,29,true,false);a.floating_product(31,31,0);
        } else {
            a.at(0x1036d1f8);a.floating_product(31,31,2);a.jump(0x1036d218);
        }
    }
    a.at(0x1036d218);a.floating_move(1,31);
    return;
}
}
