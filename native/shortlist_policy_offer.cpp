#include "shortlist_policy_detail.hpp"

namespace cm::aot::shortlist_policy::detail {
void prepare_offer_policy(Call& a) {
    a.at(0x1036f170);a.link();a.multiple(20,false,0x0);a.load<4>(28,2,-0x7b7c,false);a.move(30,4);a.move(31,5);a.load<4>(27,2,-0x7a54,false);
    a.load<4>(29,2,-0x4ddc,false);a.move(21,6);a.move(20,7);a.move(22,8);a.move(24,9);a.move(23,10);a.move(4,31);a.move(5,30);a.store<4>(1,0x8,0);
    a.frame(0x80);a.load<1>(25,1,0xbb,false);a.load<4>(3,28,0x0,false);a.call(0x10082f90);a.compare(30,0x0,false);a.move(26,3);
    if(a.equal()) {
        a.at(0x1036f1dc);a.value(3,0x0);a.jump(0x1036f574);
    } else {
        a.at(0x1036f1c8);a.compare(26,0x0,false);
        if(a.equal()) {
            a.at(0x1036f1e4);a.load<4>(0,30,0x90,false);a.compare_registers(0,31,false);
            if(!a.equal()) {
                return check_offer_contract_mode(a);
            } else {
                a.at(0x1036f1f0);a.value(3,0x2);a.jump(0x1036f574);
            }
        } else {
            a.at(0x1036f1d0);a.load<1>(0,26,0x54,false);a.sign8(0,0,true);
            if(a.equal()) {
                a.at(0x1036f1e4);a.load<4>(0,30,0x90,false);a.compare_registers(0,31,false);
                if(!a.equal()) {
                    return check_offer_contract_mode(a);
                } else {
                    a.at(0x1036f1f0);a.value(3,0x2);a.jump(0x1036f574);
                }
            } else {
                a.at(0x1036f1dc);a.value(3,0x0);a.jump(0x1036f574);
            }
        }
    }
}

void check_offer_contract_mode(Call& a) {
    a.at(0x1036f1f8);a.compare(26,0x0,false);
    if(a.equal()) {
        a.at(0x1036f218);a.sign8(0,25,true);
        if(a.equal()) {
            a.at(0x1036f228);a.value(3,0x0);a.jump(0x1036f574);
        } else {
            a.at(0x1036f220);a.value(3,0x2);a.jump(0x1036f574);
        }
    } else {
        a.at(0x1036f200);a.load<1>(0,26,0x38,false);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);a.sign8(0,0,false);a.compare(0,0x2,true);
        if(a.equal()) {
            return check_requested_offer_mode(a);
        } else {
            a.at(0x1036f218);a.sign8(0,25,true);
            if(a.equal()) {
                a.at(0x1036f228);a.value(3,0x0);a.jump(0x1036f574);
            } else {
                a.at(0x1036f220);a.value(3,0x2);a.jump(0x1036f574);
            }
        }
    }
}

void check_requested_offer_mode(Call& a) {
    a.at(0x1036f230);a.sign8(0,25,true);
    if(a.equal()) {
        return resolve_existing_offer_value(a);
    } else {
        a.at(0x1036f238);a.move(3,30);a.value(4,0x0);a.value(5,0x1);a.value(6,0x0);a.value(7,0x0);a.call(0x1014c2e0);a.sign8(0,3,true);
        if(!a.equal()) {
            a.at(0x1036f2b8);a.value(3,0x0);a.jump(0x1036f574);
        } else {
            a.at(0x1036f258);a.move(3,31);a.value(4,0x0);a.call(0x100d3230);a.sign8(0,3,true);
            if(a.equal()) {
                return check_requested_profile_limits(a);
            } else {
                a.at(0x1036f26c);a.load<2>(0,22,0x1ec,true);a.value(3,0x43300000);a.load<4>(4,31,0x14,false);a.exclusive_flags(0,0,0x80000000);
                a.store<4>(1,0x38,3);a.load<2>(4,4,0xa,true);a.store<4>(1,0x3c,0);a.floating_load(3,29,0x8,false);a.exclusive_flags(0,4,0x80000000);
                a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,0);a.floating_subtract(0,0,3,false);a.floating_load(1,29,0xc8,false);
                a.store<4>(1,0x40,3);a.floating_load(2,1,0x40,false);a.floating_product(0,1,0);a.floating_subtract(1,2,3,false);a.floating_compare(1,0);
                if(a.at_least()) {
                    return check_requested_profile_limits(a);
                } else {
                    a.at(0x1036f2b8);a.value(3,0x0);a.jump(0x1036f574);
                }
            }
        }
    }
}

void check_requested_profile_limits(Call& a) {
    a.at(0x1036f2c0);a.load<1>(0,26,0x60,false);a.mask(0,0,0xf,false,0);a.compare(0,0x6,true);
    if(a.equal())return compare_requested_profile_limits(a);
    a.at(0x1036f2d0);a.sign8(0,0,false);a.compare(0,0x7,true);
    if(a.equal())return compare_requested_profile_limits(a);
    a.at(0x1036f2dc);a.load<4>(3,31,0x14,false);a.load<1>(0,3,0xf,false);a.compare(0,0x14,true);
    if(!a.equal())return reject_requested_offer(a);
    return compare_requested_profile_limits(a);
}

void compare_requested_profile_limits(Call& a) {
    a.at(0x1036f2ec);a.load<4>(4,31,0x14,false);a.value(3,0x43300000);a.load<2>(0,22,0x1ec,true);a.load<2>(4,4,0xa,true);
    a.exclusive_flags(0,0,0x80000000);a.store<4>(1,0x38,3);a.exclusive_flags(4,4,0x80000000);a.floating_load(2,29,0x8,false);a.store<4>(1,0x3c,4);
    a.floating_load(3,29,0xa0,false);a.floating_load(0,1,0x38,false);a.store<4>(1,0x44,0);a.floating_subtract(1,0,2,false);a.store<4>(1,0x40,3);
    a.floating_load(0,1,0x40,false);a.floating_product(1,3,1);a.floating_subtract(0,0,2,false);a.floating_compare(1,0);
    if(a.at_least()) {
        return reject_requested_offer(a);
    } else {
        a.at(0x1036f338);a.load<1>(0,26,0x44,false);a.sign8(0,0,false);a.compare(0,-0x32,true);
        if(a.at_least()) {
            return reject_requested_offer(a);
        } else {
            a.at(0x1036f348);a.load<4>(3,2,-0x7a30,false);a.load<4>(4,30,0x14,false);a.load<4>(0,3,0x0,false);a.compare_registers(4,0,true);
            if(a.equal()) {
                return reject_requested_offer(a);
            } else {
                a.at(0x1036f35c);a.value(3,0x0);a.jump(0x1036f574);
            }
        }
    }
}

void reject_requested_offer(Call& a) {
    a.at(0x1036f364);a.value(3,0x2);a.jump(0x1036f574);
}

void resolve_existing_offer_value(Call& a) {
    a.at(0x1036f36c);a.move(3,30);a.value(4,0x0);a.value(5,0x1);a.value(6,0x0);a.value(7,0x0);a.call(0x1014c2e0);a.sign8(0,3,true);
    if(a.equal()) {
        return resolve_offer_value_pair(a);
    } else {
        a.at(0x1036f38c);a.load<4>(3,28,0x0,false);a.move(4,30);a.move(5,22);a.move(6,31);a.call(0x10089a40);a.exclusive_flags(3,3,0x80000000);
        a.value(0,0x43300000);a.store<4>(1,0x3c,3);a.move(3,26);a.floating_load(2,29,0x8,false);a.store<4>(1,0x38,0);a.floating_load(0,29,0xf8,false);
        a.floating_load(1,1,0x38,false);a.floating_subtract(1,1,2,false);a.floating_product(0,0,1);a.floating_integer(0,0);a.floating_store(0,1,0x40);
        a.load<4>(28,1,0x44,false);a.call(0x10380520);a.sign8(0,3,true);
        if(!a.equal()) {
            return compare_offer_value_limits(a);
        } else {
            a.at(0x1036f3e0);a.move(3,30);a.call(0x100c96e0);a.move(4,3);a.load<4>(3,27,0x0,false);a.call(0x10147e70);a.sign8(0,3,true);
            if(a.equal()) {
                return compare_offer_value_limits(a);
            } else {
                a.at(0x1036f3fc);a.move(3,30);a.call(0x100c96e0);a.move(5,3);a.load<4>(3,27,0x0,false);a.value(4,0x0);a.call(0x10149df0);
                a.sign8(0,3,false);a.compare(0,0x2,true);
                if(a.equal()) {
                    a.at(0x1036f438);a.load<4>(28,31,0x8,false);a.jump(0x1036f4a0);
                    return compare_offer_value_limits(a);
                } else {
                    a.at(0x1036f420);
                    if(a.at_least()) {
                        return compare_offer_value_limits(a);
                    } else {
                        a.at(0x1036f424);a.compare(0,0x1,true);
                        if(a.at_least()) {
                            a.at(0x1036f430);a.value(3,0x2);a.jump(0x1036f574);
                        } else {
                            a.at(0x1036f42c);a.jump(0x1036f4a0);
                            return compare_offer_value_limits(a);
                        }
                    }
                }
            }
        }
    }
}

void resolve_offer_value_pair(Call& a) {
    a.at(0x1036f440);a.value(0,0xffffffff);a.exclusive_flags(3,24,0x80000000);a.exclusive_flags(4,0,0x80000000);a.subtract_carry_at(0x1036f44c,0,23,0);
    a.subtract_extended(3,3,4);a.subtract_extended(3,4,4);a.negate_record_at(0x1036f458,3,3);
    if(a.equal()) {
        a.at(0x1036f468);a.load<4>(0,31,0x14,false);a.compare(0,0x0,false);
        if(!a.equal()) {
            a.at(0x1036f47c);a.load<4>(3,28,0x0,false);a.move(4,30);a.move(5,22);a.move(6,31);a.value(7,0x0);a.value(8,0x1);a.value(9,0x1);
            a.call(0x10085860);a.move(28,3);
        } else {
            a.at(0x1036f474);a.load<4>(28,31,0x8,false);a.jump(0x1036f4a0);
        }
    } else {
        a.at(0x1036f460);a.move(28,23);a.jump(0x1036f4a0);
    }
    return compare_offer_value_limits(a);
}

void compare_offer_value_limits(Call& a) {
    a.at(0x1036f4a0);a.shift_right(0,28,31);a.exclusive_flags(4,21,0x80000000);a.exclusive_flags(3,0,0x80000000);a.subtract_carry_at(0x1036f4ac,0,28,20);
    a.subtract_extended(3,3,4);a.subtract_extended(3,4,4);a.negate_record_at(0x1036f4b8,3,3);
    if(!a.equal()) {
        a.at(0x1036f4c8);a.move(3,30);a.value(4,0x0);a.value(5,0x1);a.value(6,0x0);a.value(7,0x0);a.call(0x1014c2e0);a.sign8(0,3,true);
        if(a.equal()) {
            return compare_offer_value_ratio(a);
        } else {
            a.at(0x1036f4e8);a.offset(4,28,-0x170000);a.exclusive_flags(3,21,0x80000000);a.offset(5,4,0x1ca0);a.shift_right(0,5,31);
            a.exclusive_flags(4,0,0x80000000);a.subtract_carry_at(0x1036f4fc,0,20,5);a.subtract_extended(3,3,4);a.subtract_extended(3,4,4);
            a.negate_record_at(0x1036f508,3,3);
            if(a.equal()) {
                return compare_offer_value_ratio(a);
            } else {
                a.at(0x1036f510);a.move(3,31);a.value(4,0x0);a.call(0x100d3040);a.sign8(0,3,true);
                if(a.equal()) {
                    return compare_offer_value_ratio(a);
                } else {
                    a.at(0x1036f524);a.value(3,0x1);a.jump(0x1036f574);
                }
            }
        }
    } else {
        a.at(0x1036f4c0);a.value(3,0x0);a.jump(0x1036f574);
    }
}

void compare_offer_value_ratio(Call& a) {
    a.at(0x1036f52c);a.move(3,21);a.move(4,20);a.call(0x10773c80);a.exclusive_flags(3,28,0x80000000);a.value(0,0x43300000);a.store<4>(1,0x3c,3);
    a.floating_load(3,29,0x8,false);a.store<4>(1,0x38,0);a.floating_load(0,29,0x2d0,false);a.floating_load(2,1,0x38,false);
    a.floating_subtract(2,2,3,false);a.floating_difference(0,2,0,true,false);a.floating_compare(1,0);a.compare_greater_or_equal_bit();
    if(!a.equal()) {
        a.at(0x1036f570);a.value(3,0x2);
    } else {
        a.at(0x1036f568);a.value(3,0x1);a.jump(0x1036f574);
    }
}
}
namespace cm::aot::shortlist_policy {
void classify_contract_offer(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,classify_contract_offer_entry,d,o);detail::prepare_offer_policy(a);
    a.at(0x1036f574);
    a.load<4>(0,1,0x88,false);
    a.offset(1,1,0x80);
    a.restore_link();
    a.multiple(20,true,0x0);
    a.finish();
}
}
