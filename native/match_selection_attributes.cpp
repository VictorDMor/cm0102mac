#include "match_selection.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_selection {
using Call = SemanticGuestCall;
namespace {
void attribute_role_weight(Call&);
void profile_attribute_weight(Call&);
void attribute_random_score(Call&);
void keep_attribute_winner(Call&);
void keep_attribute_winner(Call& a) {
    a.at(0x101e2084); a.sign8(0,25); a.compare(0,-1);
    bool replace=a.equal();
    if (!replace) {a.floating_compare(30,29);replace=!a.nonpositive();}
    if (replace) {a.at(0x101e2098);a.move(25,26);a.floating_move(29,30);}
}
void attribute_random_score(Call& a) {
    a.at(0x101e1f90); a.sign8(0,23); a.compare(0,0);
    if (a.equal()) {
        a.at(0x101e1fe8); a.value(3,0x5); a.call(0x101d3c90); a.compare(3,0);
        if (a.equal()) {
            a.at(0x101e2048); a.floating_load(1,31,8,true); a.floating_load(0,30,172,true);
            a.floating_sum_single(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,56); a.load(3,1,0x3c);
            a.call(0x101d3c90); a.floating_load(1,28,96); a.exclusive_flags(0,3,0x80000000); a.store(1,0x44,0);
            a.value(0,0x43300000); a.store(1,0x40,0); a.floating_load(0,1,64); a.floating_subtract(0,0,1,true);
            a.floating_product_single(30,31,0);
        } else {
            a.at(0x101e1ff8); a.floating_load(3,31,8,true); a.floating_load(2,30,144,true);
            a.floating_load(1,30,244,true); a.floating_load(0,30,200,true); a.floating_sum_single(0,1,0);
            a.floating_sum_single(0,2,0); a.floating_sum_single(0,3,0); a.floating_integer(0,0);
            a.floating_store(0,1,56); a.load(3,1,0x3c); a.call(0x101d3c90); a.floating_load(1,28,96);
            a.exclusive_flags(0,3,0x80000000); a.store(1,0x44,0); a.value(0,0x43300000); a.store(1,0x40,0);
            a.floating_load(0,1,64); a.floating_subtract(0,0,1,true); a.floating_product_single(30,31,0);
            a.jump(0x101e2084);
        }
        a.at(0x101e2084);
    } else {
        a.at(0x101e1f9c); a.floating_load(2,31,44,true); a.floating_load(1,30,164,true);
        a.floating_load(0,30,156,true); a.floating_subtract(1,1,0,true); a.floating_load(0,30,176,true);
        a.floating_subtract(0,1,0,true); a.floating_sum_single(0,2,0); a.floating_integer(0,0);
        a.floating_store(0,1,64); a.load(3,1,0x44); a.call(0x101d3c90); a.floating_load(1,28,96);
        a.exclusive_flags(0,3,0x80000000); a.store(1,0x3c,0); a.value(0,0x43300000); a.store(1,0x38,0);
        a.floating_load(0,1,56); a.floating_subtract(30,0,1,true); a.jump(0x101e2084);
    }
    a.at(0x101e2084);
}
void profile_attribute_weight(Call& a) {
    a.at(0x101e1d9c); a.load(0,30,0x70); a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x101e1da8); a.load(3,30,0x70); a.load<2>(0,3,0x4,true); a.compare(0,140);
        if (a.nonpositive()) {
            a.at(0x101e1dc4); a.load(3,30,0x70); a.load<2>(0,3,0x4,true); a.compare(0,60);
            if (a.at_least()) {
                a.at(0x101e1de0); a.floating_load(2,28,504); a.load(3,30,0x70); a.load<2>(0,3,0x4,true);
                a.floating_load(1,28,96); a.exclusive_flags(0,0,0x80000000); a.store(1,0x44,0);
                a.value(0,0x43300000); a.store(1,0x40,0); a.floating_load(0,1,64); a.floating_subtract(0,0,1,true);
                a.floating_product(0,2,0); a.floating_round_single(0,0); a.floating_product_single(31,31,0);
            } else {
                a.at(0x101e1dd4); a.floating_load(0,31,164,true); a.floating_product_single(31,31,0);
                a.jump(0x101e1e14);
            }
            a.at(0x101e1e14);
        } else {
            a.at(0x101e1db8); a.floating_load(0,31,160,true); a.floating_product_single(31,31,0);
            a.jump(0x101e1e14);
        }
        a.at(0x101e1e14); a.load(3,30,0x70); a.load<1>(0,3,0x37); a.sign8(0,0); a.compare(0,15);
        if (a.nonpositive()) {
            a.at(0x101e1e34); a.load(3,30,0x70); a.load<1>(0,3,0x37); a.sign8(0,0); a.compare(0,6);
            if (a.at_least()) {
                a.at(0x101e1e54); a.load(3,30,0x70); a.load<1>(0,3,0x37); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                    a.at(0x101e1e68); a.floating_load(0,31,92,true); a.floating_product_single(31,31,0);
                }
                a.at(0x101e1e70);
            } else {
                a.at(0x101e1e48); a.floating_load(0,31,120,true); a.floating_product_single(31,31,0);
                a.jump(0x101e1e70);
            }
            a.at(0x101e1e70);
        } else {
            a.at(0x101e1e28); a.floating_load(0,31,112,true); a.floating_product_single(31,31,0);
            a.jump(0x101e1e70);
        }
        a.at(0x101e1e70); a.load(3,30,0x70); a.load<1>(0,3,0x43); a.sign8(0,0); a.compare(0,15);
        if (a.nonpositive()) {
            a.at(0x101e1e90); a.load(3,30,0x70); a.load<1>(0,3,0x43); a.sign8(0,0); a.compare(0,6);
            if (a.at_least()) {
                a.at(0x101e1eb0); a.load(3,30,0x70); a.load<1>(0,3,0x43); a.sign8(0,0); a.compare(0,10);
                if (a.less()) {
                    a.at(0x101e1ec4); a.floating_load(0,31,92,true); a.floating_product_single(31,31,0);
                }
                a.at(0x101e1ecc);
            } else {
                a.at(0x101e1ea4); a.floating_load(0,31,120,true); a.floating_product_single(31,31,0);
                a.jump(0x101e1ecc);
            }
            a.at(0x101e1ecc);
        } else {
            a.at(0x101e1e84); a.floating_load(0,31,112,true); a.floating_product_single(31,31,0);
            a.jump(0x101e1ecc);
        }
        a.at(0x101e1ecc); a.sign8(0,23); a.compare(0,0);
        if (a.equal()) {
            a.at(0x101e1ed8); a.load(3,30,0x70); a.load<1>(6,3,0x40); a.load(3,30,0x70); a.load<1>(5,3,0x26);
            a.load(3,30,0x70); a.load<1>(4,3,0x36); a.load(3,30,0x70); a.load<1>(0,3,0x1b); a.add(0,0,5);
            a.add(0,0,6); a.add(0,4,0); a.sign8(27,0); a.compare(27,75);
            if (a.nonpositive()) {
                a.at(0x101e1f1c); a.sign8(0,27); a.compare(0,60);
                if (a.nonpositive()) {
                    a.at(0x101e1f34); a.sign8(0,27); a.compare(0,25);
                    if (a.at_least()) {
                        a.at(0x101e1f50); a.sign8(0,27); a.compare(0,40);
                        if (a.less()) {
                            a.at(0x101e1f5c); a.floating_load(0,31,92,true); a.floating_product_single(31,31,0);
                        }
                        a.at(0x101e1f64);
                    } else {
                        a.at(0x101e1f40); a.floating_load(0,28,280); a.floating_product(31,31,0);
                        a.floating_round_single(31,31); a.jump(0x101e1f64);
                    }
                    a.at(0x101e1f64);
                } else {
                    a.at(0x101e1f28); a.floating_load(0,31,88,true); a.floating_product_single(31,31,0);
                    a.jump(0x101e1f64);
                }
                a.at(0x101e1f64);
            } else {
                a.at(0x101e1f10); a.floating_load(0,31,144,true); a.floating_product_single(31,31,0);
                a.jump(0x101e1f64);
            }
            a.at(0x101e1f64);
        }
        a.at(0x101e1f64); a.value(3,0x14); a.call(0x101d3c90); a.move(22,3); a.load(3,30,0x70);
        a.load<1>(3,3,0x21); a.sign8(3,3); a.call(0x101d3c90); a.compare_registers(3,22);
        if (a.less()) {
            a.at(0x101e1f88); a.floating_load(0,31,68,true); a.floating_product_single(31,31,0);
        }
        a.at(0x101e1f90);
    }
    a.at(0x101e1f90);
}
void attribute_role_weight(Call& a) {
    a.at(0x101e1c58);a.sign16(0,29);a.mask(0,0,0x2,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e1c68);a.floating_load(31,31,0xb4,true);a.jump(0x101e1d9c);return;}
    a.at(0x101e1c70);a.sign16(0,29);a.mask(0,0,0x4,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e1c80);a.floating_load(31,31,0xb0,true);a.jump(0x101e1d9c);return;}
    a.at(0x101e1c88);a.sign16(0,29);a.mask(0,0,0x8,false,0);a.compare(0,0);
    if(!a.equal()){
        a.at(0x101e1c98);a.sign16(0,29);a.mask(0,0,0x80,false,0);a.compare(0,0);
        bool qualified=!a.equal();
        if(!qualified){a.at(0x101e1ca8);a.sign16(0,29);a.mask(0,0,0x800,false,0);a.compare(0,0);qualified=!a.equal();}
        if(qualified){a.at(0x101e1cb8);a.floating_load(31,31,0x54,true);a.jump(0x101e1d9c);return;}
    }
    a.at(0x101e1cc0);a.sign16(0,29);a.mask(0,0,0x8,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e1cd0);a.floating_load(31,31,0xb8,true);a.jump(0x101e1d9c);return;}
    a.at(0x101e1cd8);a.sign16(0,29);a.mask(0,0,0x10,false,0);a.compare(0,0);
    if(!a.equal()){
        a.at(0x101e1ce8);a.sign16(0,29);a.mask(0,0,0x80,false,0);a.compare(0,0);
        bool qualified=!a.equal();
        if(!qualified){a.at(0x101e1cf8);a.sign16(0,29);a.mask(0,0,0x800,false,0);a.compare(0,0);qualified=!a.equal();}
        if(qualified){a.at(0x101e1d08);a.floating_load(31,31,0x40,true);a.jump(0x101e1d9c);return;}
    }
    a.at(0x101e1d10);a.sign16(0,29);a.mask(0,0,0x10,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e1d20);a.floating_load(31,31,0x78,true);a.jump(0x101e1d9c);return;}
    a.at(0x101e1d28);a.sign16(0,29);a.mask(0,0,0x20,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e1d38);a.floating_load(31,31,0x58,true);a.jump(0x101e1d9c);return;}
    a.at(0x101e1d40);a.sign16(0,29);a.mask(0,0,0x40,false,0);a.compare(0,0);
    if(!a.equal()){
        a.at(0x101e1d50);a.sign16(0,29);a.mask(0,0,0x100,false,0);a.compare(0,0);
        bool qualified=!a.equal();
        if(!qualified){a.at(0x101e1d60);a.sign16(0,29);a.mask(0,0,0x200,false,0);a.compare(0,0);qualified=!a.equal();}
        if(!qualified){a.at(0x101e1d70);a.sign16(0,29);a.mask(0,0,0x400,false,0);a.compare(0,0);qualified=!a.equal();}
        if(qualified){a.at(0x101e1d80);a.floating_load(31,31,0x5c,true);a.jump(0x101e1d9c);return;}
    }
    a.at(0x101e1d88);a.sign16(0,29);a.mask(0,0,0x40,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e1d98);a.floating_load(31,31,0x84,true);}
}
}
void select_attribute_weighted_actor(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101e1bd0,dependencies,observer);
    a.link(); a.floating_store(31,1,-8); a.floating_store(30,1,-16); a.floating_store(29,1,-24);
    a.multiple(22,false,-32); a.save_link(); a.frame(144); a.store(1,0xa8,3); a.store<1>(1,0xaf,4); a.move(23,5);
    a.load(31,2,-0x52c8); a.load(28,2,-0x52c4); a.floating_load(29,31,12,true); a.value(25,0xffffffff);
    a.load(4,1,0xa8); a.load<1>(0,1,0xaf); a.sign8(0,0); a.multiply(3,0,0x2350); a.offset(30,3,0x311c);
    a.add(30,4,30); a.value(26,0x0); a.jump(0x101e20ac);
    for (;;) {
        a.at(0x101e20ac); a.sign8(0,26); a.compare(0,20);
        if (!a.less()) break;
        a.at(0x101e1c28); a.move(3,30); a.call(0x101d3b50); a.sign8(0,3); a.compare(0,0);
        if (a.at_least()) {
            a.at(0x101e1c3c); a.move(3,30); a.call(0x10289560); a.move(29,3); a.sign16(0,29);
            a.mask(0,0,0x1,false,0); a.compare(0,0);
            if (a.equal()) {
                a.at(0x101e1c58);
                attribute_role_weight(a);
                profile_attribute_weight(a);
                attribute_random_score(a);
                keep_attribute_winner(a);
            }
            a.at(0x101e20a0);
        }
        a.at(0x101e20a0); a.offset(26,26,0x1); a.move(24,30); a.offset(30,30,0x1c4);
    }
    a.at(0x101e20b8); a.at(0x101e20b8); a.value(3,0x5); a.call(0x101d3c90); a.compare(3,0);
    if (a.equal()) {
        a.at(0x101e20d0); a.value(3,0xffffffff);
    } else {
        a.at(0x101e20c8); a.move(3,25); a.jump(0x101e20d4);
    }
    a.at(0x101e20d4); a.load(0,1,0x98); a.offset(1,1,0x90); a.floating_load(31,1,-8); a.floating_load(30,1,-16);
    a.floating_load(29,1,-24); a.restore_link(); a.multiple(22,true,-32); a.finish();
}
}
