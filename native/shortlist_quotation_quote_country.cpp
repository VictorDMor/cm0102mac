#include "shortlist_quotation_detail.hpp"

namespace cm::aot::shortlist_quotation::detail {
void check_quote_home_country(Call& a) {
    a.at(0x1037d690);a.compare(4,0x0,false);
    if(a.equal()) {
        a.at(0x1037d698);a.load<4>(0,15,0xc,false);a.compare_registers(0,3,false);
        if(a.equal()) {
            return apply_quote_country_rules(a);
        }
    }
    return finish_quote_contract_limits(a);
}

void apply_quote_country_rules(Call& a) {
    a.at(0x1037d6a4);a.value(4,0x51ec0000);a.load<2>(0,16,0x1ec,true);a.offset(5,4,-0x7ae1);
    a.load<4>(4,2,-0x7a24,false);a.multiply_high(5,5,0);a.load<4>(0,3,0x68,false);a.load<4>(4,4,0x0,false);
    a.compare_registers(0,4,true);a.shift_right(4,5,4);a.mask(5,4,0x1,false,1);a.add(4,4,5);
    a.sign16(4,4,false);
    if(!a.equal()) {
        a.at(0x1037d734);a.load<4>(5,13,0x0,false);a.compare_registers(0,5,true);
        if(!a.equal()) {
            a.at(0x1037d740);a.load<4>(5,14,0x0,false);a.compare_registers(0,5,true);
            if(!a.equal()) {
                a.at(0x1037d74c);a.load<4>(5,28,0x0,false);a.compare_registers(0,5,true);
                if(!a.equal()) {
                    a.at(0x1037d758);a.load<4>(5,2,-0x7a20,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                    if(!a.equal()) {
                        a.at(0x1037d870);a.load<4>(5,2,-0x7a1c,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                        if(!a.equal()) {
                            a.at(0x1037d9e4);a.load<4>(5,2,-0x7af4,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                            if(a.equal()) {
                                return calculate_first_country_scale(a);
                            } else {
                                a.at(0x1037d9f4);a.load<4>(5,2,-0x7a00,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                if(a.equal()) {
                                    return calculate_first_country_scale(a);
                                } else {
                                    a.at(0x1037da04);a.load<4>(5,2,-0x7a28,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                    if(!a.equal()) {
                                        a.at(0x1037dabc);a.load<4>(5,2,-0x7514,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                        if(!a.equal()) {
                                            a.at(0x1037dba4);a.load<4>(5,2,-0x757c,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                            if(a.equal()) {
                                                return choose_first_country_cap(a);
                                            } else {
                                                a.at(0x1037dbb4);a.load<4>(5,2,-0x7594,false);a.load<4>(6,5,0x0,false);a.compare_registers(0,6,true);
                                                if(a.equal()) {
                                                    return choose_first_country_cap(a);
                                                } else {
                                                    a.at(0x1037dbc4);a.load<4>(5,2,-0x7574,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                                    if(a.equal()) {
                                                        return choose_first_country_cap(a);
                                                    } else {
                                                        a.at(0x1037dbd4);a.load<4>(5,2,-0x7578,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                                        if(a.equal()) {
                                                            return choose_first_country_cap(a);
                                                        } else {
                                                            a.at(0x1037dbe4);a.load<4>(5,2,-0x7650,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                                            if(!a.equal()) {
                                                                a.at(0x1037dc40);a.load<4>(5,2,-0x750c,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                                                if(a.equal()) {
                                                                    return calculate_second_country_scale(a);
                                                                } else {
                                                                    a.at(0x1037dc50);a.load<4>(5,2,-0x7504,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                                                    if(!a.equal()) {
                                                                        a.at(0x1037dd34);a.load<4>(5,2,-0x7510,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                                                        if(!a.equal()) {
                                                                            a.at(0x1037de18);a.compare_registers(0,6,true);
                                                                            if(!a.equal()) {
                                                                                a.at(0x1037def4);a.load<4>(5,2,-0x79fc,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                                                                if(a.equal()) {
                                                                                    return calculate_third_country_scale(a);
                                                                                } else {
                                                                                    a.at(0x1037df04);a.load<4>(5,2,-0x7590,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                                                                    if(a.equal()) {
                                                                                        return calculate_third_country_scale(a);
                                                                                    } else {
                                                                                        a.at(0x1037df14);a.load<4>(5,2,-0x75a8,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                                                                        if(!a.equal()) {
                                                                                            a.at(0x1037dff8);a.load<4>(5,2,-0x7b68,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                                                                            if(a.equal()) {
                                                                                                return calculate_final_country_scale(a);
                                                                                            } else {
                                                                                                a.at(0x1037e008);a.load<4>(5,2,-0x7508,false);a.load<4>(5,5,0x0,false);a.compare_registers(0,5,true);
                                                                                                if(!a.equal()) {
                                                                                                    a.at(0x1037e0c0);a.floating_load(0,29,0x1e8,false);a.floating_load(2,29,0x230,false);a.floating_product(31,31,0);
                                                                                                    a.floating_round_single(31,31);a.floating_compare(31,2);
                                                                                                    if(!a.nonpositive()) {
                                                                                                        a.at(0x1037e0d8);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x48,false);a.floating_product_sum(31,1,0,2,false);
                                                                                                        a.floating_round_single(31,31);
                                                                                                    }
                                                                                                } else {
                                                                                                    return calculate_final_country_scale(a);
                                                                                                }
                                                                                            }
                                                                                        } else {
                                                                                            return calculate_third_country_scale(a);
                                                                                        }
                                                                                    }
                                                                                }
                                                                            } else {
                                                                                a.at(0x1037de20);a.load<1>(0,3,0x11f,false);a.floating_load(5,29,0xf8,false);a.sign8(5,0,false);
                                                                                a.multiply(0,5,0xa);a.compare_registers(4,0,true);
                                                                                if(!a.nonpositive()) {
                                                                                    a.at(0x1037de38);a.exclusive_flags(0,4,0x80000000);a.value(3,0x43300000);a.store<4>(1,0x3c,0);
                                                                                    a.compare(5,0x1,true);a.floating_load(1,29,0xc0,false);a.value(0,0x1);a.store<4>(1,0x38,3);
                                                                                    a.floating_load(0,1,0x38,false);a.floating_subtract(4,0,1,false);
                                                                                    if(!a.less()) {
                                                                                        a.at(0x1037de60);a.move(0,5);
                                                                                    }
                                                                                    a.at(0x1037de64);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x44,3);
                                                                                    a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,0);a.floating_load(1,29,0x170,false);a.floating_load(2,1,0x40,false);
                                                                                    a.floating_load(0,29,0x108,false);a.floating_subtract(2,2,3,false);a.floating_difference(2,4,2,true,false);a.floating_difference(1,2,1,true,false);
                                                                                    a.floating_product(5,5,1);a.floating_compare(5,0);
                                                                                    if(!a.nonpositive()) {
                                                                                        a.at(0x1037de9c);a.floating_move(5,0);
                                                                                    }
                                                                                }
                                                                                a.at(0x1037dea0);a.floating_product(31,31,5);a.load<1>(0,16,0x23d,false);a.compare(0,0x1,true);
                                                                                a.floating_round_single(31,31);
                                                                                if(a.equal()) {
                                                                                    a.at(0x1037ded4);a.floating_load(2,29,0x40,false);a.floating_compare(31,2);
                                                                                    if(!a.nonpositive()) {
                                                                                        a.at(0x1037dee0);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0xa0,false);a.floating_product_sum(31,1,0,2,false);
                                                                                        a.floating_round_single(31,31);a.jump(0x1037e0e8);
                                                                                    }
                                                                                } else {
                                                                                    a.at(0x1037deb4);a.floating_load(2,29,0x68,false);a.floating_compare(31,2);
                                                                                    if(!a.nonpositive()) {
                                                                                        a.at(0x1037dec0);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0xa0,false);a.floating_product_sum(31,1,0,2,false);
                                                                                        a.floating_round_single(31,31);a.jump(0x1037e0e8);
                                                                                    }
                                                                                }
                                                                            }
                                                                        } else {
                                                                            a.at(0x1037dd44);a.load<1>(0,3,0x11f,false);a.floating_load(5,29,0xf8,false);a.sign8(5,0,false);
                                                                            a.multiply(0,5,0xa);a.compare_registers(4,0,true);
                                                                            if(!a.nonpositive()) {
                                                                                a.at(0x1037dd5c);a.exclusive_flags(0,4,0x80000000);a.value(3,0x43300000);a.store<4>(1,0x3c,0);
                                                                                a.compare(5,0x1,true);a.floating_load(1,29,0xc0,false);a.value(0,0x1);a.store<4>(1,0x38,3);
                                                                                a.floating_load(0,1,0x38,false);a.floating_subtract(4,0,1,false);
                                                                                if(!a.less()) {
                                                                                    a.at(0x1037dd84);a.move(0,5);
                                                                                }
                                                                                a.at(0x1037dd88);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x44,3);
                                                                                a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,0);a.floating_load(1,29,0x170,false);a.floating_load(2,1,0x40,false);
                                                                                a.floating_load(0,29,0x108,false);a.floating_subtract(2,2,3,false);a.floating_difference(2,4,2,true,false);a.floating_difference(1,2,1,true,false);
                                                                                a.floating_product(5,5,1);a.floating_compare(5,0);
                                                                                if(!a.nonpositive()) {
                                                                                    a.at(0x1037ddc0);a.floating_move(5,0);
                                                                                }
                                                                            }
                                                                            a.at(0x1037ddc4);a.floating_product(31,31,5);a.load<1>(0,16,0x23d,false);a.compare(0,0x1,true);
                                                                            a.floating_round_single(31,31);
                                                                            if(a.equal()) {
                                                                                a.at(0x1037ddf8);a.floating_load(2,29,0x40,false);a.floating_compare(31,2);
                                                                                if(!a.nonpositive()) {
                                                                                    a.at(0x1037de04);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0xa0,false);a.floating_product_sum(31,1,0,2,false);
                                                                                    a.floating_round_single(31,31);a.jump(0x1037e0e8);
                                                                                }
                                                                            } else {
                                                                                a.at(0x1037ddd8);a.floating_load(2,29,0x68,false);a.floating_compare(31,2);
                                                                                if(!a.nonpositive()) {
                                                                                    a.at(0x1037dde4);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0xa0,false);a.floating_product_sum(31,1,0,2,false);
                                                                                    a.floating_round_single(31,31);a.jump(0x1037e0e8);
                                                                                }
                                                                            }
                                                                        }
                                                                    } else {
                                                                        return calculate_second_country_scale(a);
                                                                    }
                                                                }
                                                            } else {
                                                                return choose_first_country_cap(a);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            a.at(0x1037dacc);a.load<1>(0,3,0x11f,false);a.floating_load(5,29,0x48,false);a.sign8(5,0,false);
                                            a.multiply(0,5,0xa);a.compare_registers(4,0,true);
                                            if(!a.nonpositive()) {
                                                a.at(0x1037dae4);a.exclusive_flags(0,4,0x80000000);a.value(3,0x43300000);a.store<4>(1,0x3c,0);
                                                a.compare(5,0x1,true);a.floating_load(1,29,0xc0,false);a.value(0,0x1);a.store<4>(1,0x38,3);
                                                a.floating_load(0,1,0x38,false);a.floating_subtract(4,0,1,false);
                                                if(!a.less()) {
                                                    a.at(0x1037db0c);a.move(0,5);
                                                }
                                                a.at(0x1037db10);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x44,3);
                                                a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,0);a.floating_load(1,29,0x170,false);a.floating_load(2,1,0x40,false);
                                                a.floating_load(0,29,0x108,false);a.floating_subtract(2,2,3,false);a.floating_difference(2,4,2,true,false);a.floating_difference(1,2,1,true,false);
                                                a.floating_product(5,5,1);a.floating_compare(5,0);
                                                if(!a.nonpositive()) {
                                                    a.at(0x1037db48);a.floating_move(5,0);
                                                }
                                            }
                                            a.at(0x1037db4c);a.floating_product(31,31,5);a.load<1>(0,16,0x23d,false);a.compare(0,0x1,true);
                                            a.floating_round_single(31,31);
                                            if(a.equal()) {
                                                a.at(0x1037db80);a.floating_load(2,29,0x40,false);a.floating_compare(31,2);
                                                if(!a.nonpositive()) {
                                                    a.at(0x1037db8c);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0xa0,false);a.floating_round_single(0,0);
                                                    a.floating_product_sum(31,1,0,2,false);a.floating_round_single(31,31);a.jump(0x1037e0e8);
                                                }
                                            } else {
                                                a.at(0x1037db60);a.floating_load(2,29,0x220,false);a.floating_compare(31,2);
                                                if(!a.nonpositive()) {
                                                    a.at(0x1037db6c);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0xa0,false);a.floating_product_sum(31,1,0,2,false);
                                                    a.floating_round_single(31,31);a.jump(0x1037e0e8);
                                                }
                                            }
                                        }
                                    } else {
                                        return calculate_first_country_scale(a);
                                    }
                                }
                            }
                        } else {
                            a.at(0x1037d880);a.load<1>(0,3,0x11f,false);a.floating_load(5,29,0x48,false);a.sign8(5,0,false);
                            a.multiply(6,5,0xa);a.compare_registers(4,6,true);
                            if(!a.nonpositive()) {
                                a.at(0x1037d898);a.exclusive_flags(0,4,0x80000000);a.value(3,0x43300000);a.store<4>(1,0x3c,0);
                                a.compare(5,0x1,true);a.floating_load(1,29,0xc0,false);a.value(0,0x1);a.store<4>(1,0x38,3);
                                a.floating_load(0,1,0x38,false);a.floating_subtract(4,0,1,false);
                                if(!a.less()) {
                                    a.at(0x1037d8c0);a.move(0,5);
                                }
                                a.at(0x1037d8c4);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x44,3);
                                a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,0);a.floating_load(1,29,0x170,false);a.floating_load(2,1,0x40,false);
                                a.floating_load(0,29,0x108,false);a.floating_subtract(2,2,3,false);a.floating_difference(2,4,2,true,false);a.floating_difference(1,2,1,true,false);
                                a.floating_product(5,5,1);a.floating_compare(5,0);
                                if(!a.nonpositive()) {
                                    a.at(0x1037d8fc);a.floating_move(5,0);
                                }
                            }
                            a.at(0x1037d900);a.floating_product(31,31,5);a.compare_registers(4,6,true);a.floating_round_single(31,31);
                            if(a.nonpositive()) {
                                a.at(0x1037d930);a.compare(24,0x0,false);
                                if(a.equal()) {
                                    a.at(0x1037d9c4);a.floating_load(2,29,0x240,false);a.floating_compare(31,2);
                                    if(!a.nonpositive()) {
                                        a.at(0x1037d9d0);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x48,false);a.floating_product_sum(31,1,0,2,false);
                                        a.floating_round_single(31,31);a.jump(0x1037e0e8);
                                    }
                                } else {
                                    a.at(0x1037d938);a.load<2>(3,24,0xc,true);a.compare(3,0x4e2,true);
                                    if(a.at_least()) {
                                        a.at(0x1037d964);a.load<2>(0,15,0x5c,true);a.sign8(0,0,false);a.compare(0,0x14,true);
                                        if(a.less()) {
                                            return check_quote_country_band(a);
                                        } else {
                                            a.at(0x1037d974);a.compare(0,0x20,true);
                                            if(a.nonpositive()) {
                                                return compress_high_country_quote(a);
                                            } else {
                                                return check_quote_country_band(a);
                                            }
                                        }
                                    } else {
                                        a.at(0x1037d944);a.floating_load(2,29,0x98,false);a.floating_compare(31,2);
                                        if(!a.nonpositive()) {
                                            a.at(0x1037d950);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x48,false);a.floating_product_sum(31,1,0,2,false);
                                            a.floating_round_single(31,31);a.jump(0x1037e0e8);
                                        }
                                    }
                                }
                            } else {
                                a.at(0x1037d910);a.floating_load(2,29,0x58,false);a.floating_compare(31,2);
                                if(!a.nonpositive()) {
                                    a.at(0x1037d91c);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x48,false);a.floating_product_sum(31,1,0,2,false);
                                    a.floating_round_single(31,31);a.jump(0x1037e0e8);
                                }
                            }
                        }
                    } else {
                        a.at(0x1037d768);a.floating_load(0,29,0x30,false);a.load<4>(5,16,0x30,false);a.floating_product(31,31,0);
                        a.compare(5,0x0,false);a.floating_round_single(31,31);
                        if(a.equal()) {
                            return check_quote_local_connection(a);
                        } else {
                            a.at(0x1037d780);a.load<4>(4,2,-0x6778,false);a.load<4>(5,5,0x0,false);a.load<4>(0,4,0x0,false);
                            a.compare_registers(5,0,true);
                            if(!a.equal()) {
                                return check_quote_local_connection(a);
                            } else {
                                a.at(0x1037d794);a.floating_load(2,29,0x68,false);a.floating_compare(31,2);
                                if(!a.nonpositive()) {
                                    a.at(0x1037d7a0);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x78,false);a.floating_product_sum(31,1,0,2,false);
                                    a.floating_round_single(31,31);a.jump(0x1037e0e8);
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        a.at(0x1037d6d8);a.product(3,21,21);a.value(0,0x43300000);a.store<4>(1,0x38,0);
        a.floating_load(1,29,0xc0,false);a.multiply(3,3,0xa);a.exclusive_flags(3,3,0x80000000);a.store<4>(1,0x3c,3);
        a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,true);a.floating_compare(31,0);
        if(!a.at_least()) {
            a.at(0x1037d704);a.store<4>(1,0x44,3);a.store<4>(1,0x40,0);a.floating_load(0,1,0x40,false);
            a.floating_subtract(31,0,1,true);
        }
        a.at(0x1037d714);a.floating_load(2,29,0x210,false);a.floating_compare(31,2);
        if(!a.nonpositive()) {
            a.at(0x1037d720);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x48,false);a.floating_product_sum(31,1,0,2,false);
            a.floating_round_single(31,31);a.jump(0x1037e0e8);
        }
    }
    return finish_quote_contract_limits(a);
}

void check_quote_local_connection(Call& a) {
    a.at(0x1037d7b4);a.load<4>(0,15,0xc,false);a.compare_registers(0,3,false);
    if(!a.equal()) {
        a.at(0x1037d850);a.floating_load(2,29,0xb0,false);a.floating_compare(31,2);
        if(!a.nonpositive()) {
            a.at(0x1037d85c);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x48,false);a.floating_product_sum(31,1,0,2,false);
            a.floating_round_single(31,31);a.jump(0x1037e0e8);
        }
    } else {
        a.at(0x1037d7c0);a.load<2>(0,15,0x5c,true);a.sign8(0,0,false);a.compare(0,0x1f,true);
        if(a.nonpositive()) {
            a.at(0x1037d7f0);a.compare(0,0x18,true);
            if(a.at_least()) {
                return compress_local_connection_quote(a);
            } else {
                a.at(0x1037d7f8);a.load<4>(3,15,0x14,false);a.compare(3,0x0,false);
                if(a.equal()) {
                    return compress_local_connection_quote(a);
                } else {
                    a.at(0x1037d804);a.load<2>(0,3,0xc,true);a.compare(0,0xea6,true);
                    if(a.at_least()) {
                        return compress_local_connection_quote(a);
                    } else {
                        a.at(0x1037d810);a.floating_load(2,29,0x98,false);a.floating_compare(31,2);
                        if(!a.nonpositive()) {
                            a.at(0x1037d81c);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0xa0,false);a.floating_product_sum(31,1,0,2,false);
                            a.floating_round_single(31,31);a.jump(0x1037e0e8);
                        }
                    }
                }
            }
        } else {
            a.at(0x1037d7d0);a.floating_load(2,29,0x98,false);a.floating_compare(31,2);
            if(!a.nonpositive()) {
                a.at(0x1037d7dc);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0xa0,false);a.floating_product_sum(31,1,0,2,false);
                a.floating_round_single(31,31);a.jump(0x1037e0e8);
            }
        }
    }
    return finish_quote_contract_limits(a);
}

void compress_local_connection_quote(Call& a) {
    a.at(0x1037d830);a.floating_load(2,29,0xa8,false);a.floating_compare(31,2);
    if(!a.nonpositive()) {
        a.at(0x1037d83c);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0xa0,false);a.floating_product_sum(31,1,0,2,false);
        a.floating_round_single(31,31);a.jump(0x1037e0e8);
    }
    return finish_quote_contract_limits(a);
}

void check_quote_country_band(Call& a) {
    a.at(0x1037d97c);a.compare(3,0xabe,true);
    if(a.at_least()) {
        return compress_high_country_quote(a);
    } else {
        a.at(0x1037d984);a.floating_load(2,29,0xb0,false);a.floating_compare(31,2);
        if(!a.nonpositive()) {
            a.at(0x1037d990);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x48,false);a.floating_product_sum(31,1,0,2,false);
            a.floating_round_single(31,31);a.jump(0x1037e0e8);
        }
    }
    return finish_quote_contract_limits(a);
}

void compress_high_country_quote(Call& a) {
    a.at(0x1037d9a4);a.floating_load(2,29,0x240,false);a.floating_compare(31,2);
    if(!a.nonpositive()) {
        a.at(0x1037d9b0);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x48,false);a.floating_product_sum(31,1,0,2,false);
        a.floating_round_single(31,31);a.jump(0x1037e0e8);
    }
    return finish_quote_contract_limits(a);
}

void calculate_first_country_scale(Call& a) {
    a.at(0x1037da14);a.load<1>(0,3,0x11f,false);a.floating_load(5,29,0x30,false);a.sign8(5,0,false);
    a.multiply(0,5,0xa);a.compare_registers(4,0,true);
    if(!a.nonpositive()) {
        a.at(0x1037da2c);a.exclusive_flags(0,4,0x80000000);a.value(3,0x43300000);a.store<4>(1,0x3c,0);
        a.compare(5,0x1,true);a.floating_load(1,29,0xc0,false);a.value(0,0x1);a.store<4>(1,0x38,3);
        a.floating_load(0,1,0x38,false);a.floating_subtract(4,0,1,false);
        if(!a.less()) {
            a.at(0x1037da54);a.move(0,5);
        }
        a.at(0x1037da58);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x44,3);
        a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,0);a.floating_load(1,29,0x170,false);a.floating_load(2,1,0x40,false);
        a.floating_load(0,29,0x108,false);a.floating_subtract(2,2,3,false);a.floating_difference(2,4,2,true,false);a.floating_difference(1,2,1,true,false);
        a.floating_product(5,5,1);a.floating_compare(5,0);
        if(!a.nonpositive()) {
            a.at(0x1037da90);a.floating_move(5,0);
        }
    }
    return apply_first_country_scale(a);
}

void apply_first_country_scale(Call& a) {
    a.at(0x1037da94);a.floating_product(31,31,5);a.floating_load(2,29,0x60,false);a.floating_round_single(31,31);
    a.floating_compare(31,2);
    if(!a.nonpositive()) {
        a.at(0x1037daa8);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x48,false);a.floating_product_sum(31,1,0,2,false);
        a.floating_round_single(31,31);a.jump(0x1037e0e8);
    }
    return finish_quote_contract_limits(a);
}

void choose_first_country_cap(Call& a) {
    a.at(0x1037dbf4);a.load<1>(0,16,0x23d,false);a.compare(0,0x1,true);
    if(a.equal()) {
        a.at(0x1037dc20);a.floating_load(2,29,0x80,false);a.floating_compare(31,2);
        if(!a.nonpositive()) {
            a.at(0x1037dc2c);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x78,false);a.floating_product_sum(31,1,0,2,false);
            a.floating_round_single(31,31);a.jump(0x1037e0e8);
        }
    } else {
        a.at(0x1037dc00);a.floating_load(2,29,0x228,false);a.floating_compare(31,2);
        if(!a.nonpositive()) {
            a.at(0x1037dc0c);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x78,false);a.floating_product_sum(31,1,0,2,false);
            a.floating_round_single(31,31);a.jump(0x1037e0e8);
        }
    }
    return finish_quote_contract_limits(a);
}

void calculate_second_country_scale(Call& a) {
    a.at(0x1037dc60);a.load<1>(0,3,0x11f,false);a.floating_load(5,29,0x28,false);a.sign8(5,0,false);
    a.multiply(0,5,0xa);a.compare_registers(4,0,true);
    if(!a.nonpositive()) {
        a.at(0x1037dc78);a.exclusive_flags(0,4,0x80000000);a.value(3,0x43300000);a.store<4>(1,0x3c,0);
        a.compare(5,0x1,true);a.floating_load(1,29,0xc0,false);a.value(0,0x1);a.store<4>(1,0x38,3);
        a.floating_load(0,1,0x38,false);a.floating_subtract(4,0,1,false);
        if(!a.less()) {
            a.at(0x1037dca0);a.move(0,5);
        }
        a.at(0x1037dca4);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x44,3);
        a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,0);a.floating_load(1,29,0x170,false);a.floating_load(2,1,0x40,false);
        a.floating_load(0,29,0x108,false);a.floating_subtract(2,2,3,false);a.floating_difference(2,4,2,true,false);a.floating_difference(1,2,1,true,false);
        a.floating_product(5,5,1);a.floating_compare(5,0);
        if(!a.nonpositive()) {
            a.at(0x1037dcdc);a.floating_move(5,0);
        }
    }
    return apply_second_country_scale(a);
}

void apply_second_country_scale(Call& a) {
    a.at(0x1037dce0);a.floating_product(31,31,5);a.load<1>(0,16,0x23d,false);a.compare(0,0x1,true);
    a.floating_round_single(31,31);
    if(a.equal()) {
        a.at(0x1037dd14);a.floating_load(2,29,0x90,false);a.floating_compare(31,2);
        if(!a.nonpositive()) {
            a.at(0x1037dd20);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x88,false);a.floating_product_sum(31,1,0,2,false);
            a.floating_round_single(31,31);a.jump(0x1037e0e8);
        }
    } else {
        a.at(0x1037dcf4);a.floating_load(2,29,0x68,false);a.floating_compare(31,2);
        if(!a.nonpositive()) {
            a.at(0x1037dd00);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x88,false);a.floating_product_sum(31,1,0,2,false);
            a.floating_round_single(31,31);a.jump(0x1037e0e8);
        }
    }
    return finish_quote_contract_limits(a);
}

void calculate_third_country_scale(Call& a) {
    a.at(0x1037df24);a.load<1>(0,3,0x11f,false);a.floating_load(5,29,0x28,false);a.sign8(5,0,false);
    a.multiply(0,5,0xa);a.compare_registers(4,0,true);
    if(!a.nonpositive()) {
        a.at(0x1037df3c);a.exclusive_flags(0,4,0x80000000);a.value(3,0x43300000);a.store<4>(1,0x3c,0);
        a.compare(5,0x1,true);a.floating_load(1,29,0xc0,false);a.value(0,0x1);a.store<4>(1,0x38,3);
        a.floating_load(0,1,0x38,false);a.floating_subtract(4,0,1,false);
        if(!a.less()) {
            a.at(0x1037df64);a.move(0,5);
        }
        a.at(0x1037df68);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x44,3);
        a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,0);a.floating_load(1,29,0x170,false);a.floating_load(2,1,0x40,false);
        a.floating_load(0,29,0x108,false);a.floating_subtract(2,2,3,false);a.floating_difference(2,4,2,true,false);a.floating_difference(1,2,1,true,false);
        a.floating_product(5,5,1);a.floating_compare(5,0);
        if(!a.nonpositive()) {
            a.at(0x1037dfa0);a.floating_move(5,0);
        }
    }
    return apply_third_country_scale(a);
}

void apply_third_country_scale(Call& a) {
    a.at(0x1037dfa4);a.floating_product(31,31,5);a.load<1>(0,16,0x23d,false);a.compare(0,0x1,true);
    a.floating_round_single(31,31);
    if(a.equal()) {
        a.at(0x1037dfd8);a.floating_load(2,29,0x40,false);a.floating_compare(31,2);
        if(!a.nonpositive()) {
            a.at(0x1037dfe4);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0xa0,false);a.floating_product_sum(31,1,0,2,false);
            a.floating_round_single(31,31);a.jump(0x1037e0e8);
        }
    } else {
        a.at(0x1037dfb8);a.floating_load(2,29,0x68,false);a.floating_compare(31,2);
        if(!a.nonpositive()) {
            a.at(0x1037dfc4);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0xa0,false);a.floating_product_sum(31,1,0,2,false);
            a.floating_round_single(31,31);a.jump(0x1037e0e8);
        }
    }
    return finish_quote_contract_limits(a);
}

void calculate_final_country_scale(Call& a) {
    a.at(0x1037e018);a.load<1>(0,3,0x11f,false);a.floating_load(5,29,0xf8,false);a.sign8(5,0,false);
    a.multiply(0,5,0xa);a.compare_registers(4,0,true);
    if(!a.nonpositive()) {
        a.at(0x1037e030);a.exclusive_flags(0,4,0x80000000);a.value(3,0x43300000);a.store<4>(1,0x3c,0);
        a.compare(5,0x1,true);a.floating_load(1,29,0xc0,false);a.value(0,0x1);a.store<4>(1,0x38,3);
        a.floating_load(0,1,0x38,false);a.floating_subtract(4,0,1,false);
        if(!a.less()) {
            a.at(0x1037e058);a.move(0,5);
        }
        a.at(0x1037e05c);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x44,3);
        a.floating_load(3,29,0xc0,false);a.store<4>(1,0x40,0);a.floating_load(1,29,0x170,false);a.floating_load(2,1,0x40,false);
        a.floating_load(0,29,0x108,false);a.floating_subtract(2,2,3,false);a.floating_difference(2,4,2,true,false);a.floating_difference(1,2,1,true,false);
        a.floating_product(5,5,1);a.floating_compare(5,0);
        if(!a.nonpositive()) {
            a.at(0x1037e094);a.floating_move(5,0);
        }
    }
    return apply_final_country_scale(a);
}

void apply_final_country_scale(Call& a) {
    a.at(0x1037e098);a.floating_product(31,31,5);a.floating_load(2,29,0xb8,false);a.floating_round_single(31,31);
    a.floating_compare(31,2);
    if(!a.nonpositive()) {
        a.at(0x1037e0ac);a.floating_subtract(0,31,2,false);a.floating_load(1,29,0x48,false);a.floating_product_sum(31,1,0,2,false);
        a.floating_round_single(31,31);a.jump(0x1037e0e8);
    }
    return finish_quote_contract_limits(a);
}

void finish_quote_contract_limits(Call& a) {
    a.at(0x1037e0e8);a.compare(16,0x0,false);
    if(!a.equal()) {
        a.at(0x1037e0f0);a.compare(24,0x0,false);
        if(!a.equal()) {
            a.at(0x1037e0f8);a.compare(25,0x0,false);
            if(!a.equal()) {
                a.at(0x1037e100);a.compare_registers(16,25,false);
                if(!a.equal()) {
                    a.at(0x1037e108);a.load<4>(3,31,0x0,false);a.move(4,15);a.move(5,25);
                    a.call(0x10082f90);a.compare(3,0x0,false);
                    if(!a.equal()) {
                        a.at(0x1037e120);a.load<1>(0,3,0x60,false);a.mask(0,0,0xf,false,0);a.compare(0,0x7,true);
                        if(a.equal()) {
                            a.at(0x1037e130);a.load<4>(3,3,0xc,false);a.value(0,0x43300000);a.store<4>(1,0x38,0);
                            a.exclusive_flags(3,3,0x80000000);a.floating_load(1,29,0xc0,false);a.store<4>(1,0x3c,3);a.floating_load(0,1,0x38,false);
                            a.floating_subtract(0,0,1,true);a.floating_compare(31,0);
                            if(!a.nonpositive()) {
                                a.at(0x1037e158);a.store<4>(1,0x3c,3);a.store<4>(1,0x38,0);a.floating_load(0,1,0x38,false);
                                a.floating_subtract(31,0,1,true);
                            }
                        }
                    }
                }
            }
            a.at(0x1037e168);a.sign8(0,17,true);
            if(!a.equal()) {
                a.at(0x1037e188);a.load<4>(3,27,0x0,false);a.move(4,16);a.call(0x10103510);
                a.compare(3,0x0,false);
                if(a.equal()) {
                    a.at(0x1037e19c);a.floating_move(1,31);a.jump(0x1037e3bc);
                    return;
                }
                a.at(0x1037e1a4);a.move(4,15);a.move(5,18);a.call(0x100f2680);
                a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);a.floating_load(1,29,0xc0,false);
                a.store<4>(1,0x38,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,true);a.floating_compare(0,31);
                if(!a.at_least()) {
                    a.at(0x1037e1d4);a.floating_move(31,0);
                }
            } else {
                a.at(0x1037e170);a.compare(25,0x0,false);
                if(a.equal()) {
                    a.at(0x1037e178);a.load<4>(3,15,0x14,false);a.load<2>(0,3,0xa,true);a.compare(0,0x8ca,true);
                    if(!a.at_least()) {
                        a.at(0x1037e188);a.load<4>(3,27,0x0,false);a.move(4,16);a.call(0x10103510);
                        a.compare(3,0x0,false);
                        if(a.equal()) {
                            a.at(0x1037e19c);a.floating_move(1,31);a.jump(0x1037e3bc);
                            return;
                        }
                        a.at(0x1037e1a4);a.move(4,15);a.move(5,18);a.call(0x100f2680);
                        a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);a.floating_load(1,29,0xc0,false);
                        a.store<4>(1,0x38,0);a.floating_load(0,1,0x38,false);a.floating_subtract(0,0,1,true);a.floating_compare(0,31);
                        if(!a.at_least()) {
                            a.at(0x1037e1d4);a.floating_move(31,0);
                        }
                    }
                }
            }
            a.at(0x1037e1d8);a.compare(25,0x0,false);
            if(!a.equal()) {
                a.at(0x1037e1e0);a.sign8(0,17,true);
                if(a.equal()) {
                    a.at(0x1037e1e8);a.load<1>(0,15,0x63,false);a.sign8(0,0,false);a.compare(0,0xa,true);
                    if(!a.nonpositive()) {
                        a.at(0x1037e1f8);a.load<1>(0,15,0x65,false);a.sign8(0,0,false);a.compare(0,0x6,true);
                        if(!a.at_least()) {
                            a.at(0x1037e208);a.load<2>(0,24,0xa,true);a.compare(0,0x1a5e,true);
                            if(!a.nonpositive()) {
                                a.at(0x1037e214);a.load<1>(0,24,0x15,false);a.compare(0,0x14,true);
                                if(a.equal()) {
                                    a.at(0x1037e238);a.load<2>(0,15,0x5c,true);a.sign8(0,0,false);a.compare(0,0x14,true);
                                    if(!a.nonpositive()) {
                                        a.at(0x1037e248);a.load<4>(3,27,0x0,false);a.move(4,16);a.call(0x10103510);
                                        a.compare(3,0x0,false);
                                        if(a.equal()) {
                                            a.at(0x1037e25c);a.floating_move(1,31);a.jump(0x1037e3bc);
                                            return;
                                        }
                                        a.at(0x1037e264);a.move(4,15);a.move(5,18);a.call(0x100f2680);
                                        a.exclusive_flags(0,3,0x80000000);a.value(3,0x43300000);a.store<4>(1,0x3c,0);a.load<1>(0,24,0x14,false);
                                        a.store<4>(1,0x38,3);a.floating_load(1,29,0xc0,false);a.compare(0,0x14,true);a.floating_load(0,1,0x38,false);
                                        a.floating_subtract(1,0,1,true);
                                        if(!a.equal()) {
                                            a.at(0x1037e2c4);a.load<1>(0,24,0x13,false);a.compare(0,0x14,true);
                                            if(!a.equal()) {
                                                a.at(0x1037e30c);a.floating_load(0,29,0x118,false);a.floating_product(0,0,31);a.floating_compare(1,0);
                                                if(!a.nonpositive()) {
                                                    a.at(0x1037e31c);a.load<1>(0,15,0x65,false);a.compare(0,0x1,true);
                                                    if(!a.equal()) {
                                                        a.at(0x1037e338);a.floating_load(0,29,0x150,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                    } else {
                                                        a.at(0x1037e328);a.floating_load(0,29,0x128,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                        a.jump(0x1037e344);
                                                    }
                                                }
                                            } else {
                                                a.at(0x1037e2d0);a.floating_load(0,29,0x118,false);a.floating_product(0,0,31);a.floating_compare(1,0);
                                                if(!a.nonpositive()) {
                                                    a.at(0x1037e2e0);a.load<1>(0,15,0x65,false);a.compare(0,0x1,true);
                                                    if(!a.equal()) {
                                                        a.at(0x1037e2fc);a.floating_load(0,29,0x198,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                        a.jump(0x1037e344);
                                                    } else {
                                                        a.at(0x1037e2ec);a.floating_load(0,29,0x158,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                        a.jump(0x1037e344);
                                                    }
                                                }
                                            }
                                        } else {
                                            a.at(0x1037e298);a.load<1>(0,15,0x65,false);a.compare(0,0x1,true);
                                            if(!a.equal()) {
                                                a.at(0x1037e2b4);a.floating_load(0,29,0x158,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                a.jump(0x1037e344);
                                            } else {
                                                a.at(0x1037e2a4);a.floating_load(0,29,0x150,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                a.jump(0x1037e344);
                                            }
                                        }
                                    }
                                } else {
                                    a.at(0x1037e220);a.load<1>(0,24,0x14,false);a.compare(0,0x14,true);
                                    if(a.equal()) {
                                        a.at(0x1037e238);a.load<2>(0,15,0x5c,true);a.sign8(0,0,false);a.compare(0,0x14,true);
                                        if(!a.nonpositive()) {
                                            a.at(0x1037e248);a.load<4>(3,27,0x0,false);a.move(4,16);a.call(0x10103510);
                                            a.compare(3,0x0,false);
                                            if(a.equal()) {
                                                a.at(0x1037e25c);a.floating_move(1,31);a.jump(0x1037e3bc);
                                                return;
                                            }
                                            a.at(0x1037e264);a.move(4,15);a.move(5,18);a.call(0x100f2680);
                                            a.exclusive_flags(0,3,0x80000000);a.value(3,0x43300000);a.store<4>(1,0x3c,0);a.load<1>(0,24,0x14,false);
                                            a.store<4>(1,0x38,3);a.floating_load(1,29,0xc0,false);a.compare(0,0x14,true);a.floating_load(0,1,0x38,false);
                                            a.floating_subtract(1,0,1,true);
                                            if(!a.equal()) {
                                                a.at(0x1037e2c4);a.load<1>(0,24,0x13,false);a.compare(0,0x14,true);
                                                if(!a.equal()) {
                                                    a.at(0x1037e30c);a.floating_load(0,29,0x118,false);a.floating_product(0,0,31);a.floating_compare(1,0);
                                                    if(!a.nonpositive()) {
                                                        a.at(0x1037e31c);a.load<1>(0,15,0x65,false);a.compare(0,0x1,true);
                                                        if(!a.equal()) {
                                                            a.at(0x1037e338);a.floating_load(0,29,0x150,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                        } else {
                                                            a.at(0x1037e328);a.floating_load(0,29,0x128,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                            a.jump(0x1037e344);
                                                        }
                                                    }
                                                } else {
                                                    a.at(0x1037e2d0);a.floating_load(0,29,0x118,false);a.floating_product(0,0,31);a.floating_compare(1,0);
                                                    if(!a.nonpositive()) {
                                                        a.at(0x1037e2e0);a.load<1>(0,15,0x65,false);a.compare(0,0x1,true);
                                                        if(!a.equal()) {
                                                            a.at(0x1037e2fc);a.floating_load(0,29,0x198,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                            a.jump(0x1037e344);
                                                        } else {
                                                            a.at(0x1037e2ec);a.floating_load(0,29,0x158,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                            a.jump(0x1037e344);
                                                        }
                                                    }
                                                }
                                            } else {
                                                a.at(0x1037e298);a.load<1>(0,15,0x65,false);a.compare(0,0x1,true);
                                                if(!a.equal()) {
                                                    a.at(0x1037e2b4);a.floating_load(0,29,0x158,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                    a.jump(0x1037e344);
                                                } else {
                                                    a.at(0x1037e2a4);a.floating_load(0,29,0x150,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                    a.jump(0x1037e344);
                                                }
                                            }
                                        }
                                    } else {
                                        a.at(0x1037e22c);a.load<1>(0,24,0x13,false);a.compare(0,0x14,true);
                                        if(a.equal()) {
                                            a.at(0x1037e238);a.load<2>(0,15,0x5c,true);a.sign8(0,0,false);a.compare(0,0x14,true);
                                            if(!a.nonpositive()) {
                                                a.at(0x1037e248);a.load<4>(3,27,0x0,false);a.move(4,16);a.call(0x10103510);
                                                a.compare(3,0x0,false);
                                                if(a.equal()) {
                                                    a.at(0x1037e25c);a.floating_move(1,31);a.jump(0x1037e3bc);
                                                    return;
                                                }
                                                a.at(0x1037e264);a.move(4,15);a.move(5,18);a.call(0x100f2680);
                                                a.exclusive_flags(0,3,0x80000000);a.value(3,0x43300000);a.store<4>(1,0x3c,0);a.load<1>(0,24,0x14,false);
                                                a.store<4>(1,0x38,3);a.floating_load(1,29,0xc0,false);a.compare(0,0x14,true);a.floating_load(0,1,0x38,false);
                                                a.floating_subtract(1,0,1,true);
                                                if(!a.equal()) {
                                                    a.at(0x1037e2c4);a.load<1>(0,24,0x13,false);a.compare(0,0x14,true);
                                                    if(!a.equal()) {
                                                        a.at(0x1037e30c);a.floating_load(0,29,0x118,false);a.floating_product(0,0,31);a.floating_compare(1,0);
                                                        if(!a.nonpositive()) {
                                                            a.at(0x1037e31c);a.load<1>(0,15,0x65,false);a.compare(0,0x1,true);
                                                            if(!a.equal()) {
                                                                a.at(0x1037e338);a.floating_load(0,29,0x150,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                            } else {
                                                                a.at(0x1037e328);a.floating_load(0,29,0x128,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                                a.jump(0x1037e344);
                                                            }
                                                        }
                                                    } else {
                                                        a.at(0x1037e2d0);a.floating_load(0,29,0x118,false);a.floating_product(0,0,31);a.floating_compare(1,0);
                                                        if(!a.nonpositive()) {
                                                            a.at(0x1037e2e0);a.load<1>(0,15,0x65,false);a.compare(0,0x1,true);
                                                            if(!a.equal()) {
                                                                a.at(0x1037e2fc);a.floating_load(0,29,0x198,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                                a.jump(0x1037e344);
                                                            } else {
                                                                a.at(0x1037e2ec);a.floating_load(0,29,0x158,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                                a.jump(0x1037e344);
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    a.at(0x1037e298);a.load<1>(0,15,0x65,false);a.compare(0,0x1,true);
                                                    if(!a.equal()) {
                                                        a.at(0x1037e2b4);a.floating_load(0,29,0x158,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                        a.jump(0x1037e344);
                                                    } else {
                                                        a.at(0x1037e2a4);a.floating_load(0,29,0x150,false);a.floating_product(31,0,31);a.floating_round_single(31,31);
                                                        a.jump(0x1037e344);
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
    a.at(0x1037e344);a.compare(16,0x0,false);
    if(!a.equal()) {
        a.at(0x1037e34c);a.load<1>(0,16,0x23d,false);a.compare(0,0x1,true);
        if(a.equal()) {
            a.at(0x1037e38c);a.compare(20,0x14,true);
            if(a.at_least()) {
                a.at(0x1037e3a8);a.floating_load(0,30,0x130,true);a.floating_compare(31,0);
                if(!a.at_least()) {
                    a.at(0x1037e3b4);a.floating_move(31,0);
                }
            } else {
                a.at(0x1037e394);a.floating_load(0,30,0xc4,true);a.floating_compare(31,0);
                if(a.at_least()) {
                    a.at(0x1037e3a8);a.floating_load(0,30,0x130,true);a.floating_compare(31,0);
                    if(!a.at_least()) {
                        a.at(0x1037e3b4);a.floating_move(31,0);
                    }
                } else {
                    a.at(0x1037e3a0);a.floating_move(31,0);a.jump(0x1037e3b8);
                }
            }
        } else {
            a.at(0x1037e358);a.load<2>(0,16,0x1ec,true);a.compare(0,0x8ca,true);
            if(a.nonpositive()) {
                a.at(0x1037e378);a.floating_load(0,30,0xcc,true);a.floating_compare(31,0);
                if(!a.at_least()) {
                    a.at(0x1037e384);a.floating_move(31,0);a.jump(0x1037e3b8);
                }
            } else {
                a.at(0x1037e364);a.floating_load(0,30,0xc4,true);a.floating_compare(31,0);
                if(a.at_least()) {
                    a.at(0x1037e378);a.floating_load(0,30,0xcc,true);a.floating_compare(31,0);
                    if(!a.at_least()) {
                        a.at(0x1037e384);a.floating_move(31,0);a.jump(0x1037e3b8);
                    }
                } else {
                    a.at(0x1037e370);a.floating_move(31,0);a.jump(0x1037e3b8);
                }
            }
        }
    }
    a.at(0x1037e3b8);a.floating_move(1,31);
    return;
}
}
