#include "match_selection_support.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_selection_support {
using Call = SemanticGuestCall;
namespace {
void context_role_weight(Call&);
void context_profile_weight(Call&);
void context_random_score(Call&);
void limit_context_score(Call&);
void keep_context_winner(Call&);
void keep_context_winner(Call& a) {
    a.at(0x101e19ec); a.sign8(0,26); a.compare(0,-1);
    bool replace=a.equal();
    if (!replace) {a.floating_compare(30,29);replace=!a.nonpositive();}
    if (replace) {a.at(0x101e1a00);a.move(26,27);a.floating_move(29,30);}
}
void limit_context_score(Call& a) {
    a.at(0x101e19b8); a.floating_load(0,31,172,true); a.floating_compare(30,0);
    if (a.positive()) {
        a.at(0x101e19c4); a.value(3,0x5); a.call(0x101d3c90); a.floating_load(1,30,96);
        a.exclusive_flags(0,3,0x80000000); a.store(1,0x3c,0); a.value(0,0x43300000); a.store(1,0x38,0);
        a.floating_load(0,1,56); a.floating_subtract(0,0,1,true); a.floating_subtract(30,30,0,true);
    }
    a.at(0x101e19ec);
}
void context_random_score(Call& a) {
    a.at(0x101e1840); a.load<1>(0,1,0xb3); a.sign8(0,0); a.compare(0,0);
    if (a.equal()) {
        a.at(0x101e1884); a.value(3,0x7); a.call(0x101d3c90); a.compare(3,0);
        if (!a.equal()) {
            a.at(0x101e18d0); a.value(3,0x3); a.call(0x101d3c90); a.compare(3,0);
            if (a.equal()) {
                a.at(0x101e1954); a.floating_load(4,31,40,true); a.floating_load(1,30,224);
                a.floating_load(0,29,220,true); a.floating_product(0,1,0); a.floating_round_single(3,0);
                a.floating_load(2,29,172,true); a.floating_load(1,30,280); a.floating_load(0,29,176,true);
                a.floating_product(0,1,0); a.floating_round_single(0,0); a.floating_sum_single(0,2,0);
                a.floating_sum_single(0,3,0); a.floating_sum_single(0,4,0); a.floating_integer(0,0);
                a.floating_store(0,1,56); a.load(3,1,0x3c); a.call(0x101d3c90); a.floating_load(1,30,96);
                a.exclusive_flags(0,3,0x80000000); a.store(1,0x44,0); a.value(0,0x43300000); a.store(1,0x40,0);
                a.floating_load(0,1,64); a.floating_subtract(0,0,1,true); a.floating_product_single(30,31,0);
            } else {
                a.at(0x101e18e0); a.floating_load(9,30,600); a.floating_load(8,29,156,true);
                a.floating_load(7,30,448); a.floating_load(6,29,160,true); a.floating_load(5,30,608);
                a.floating_load(4,29,152,true); a.floating_load(3,30,480); a.floating_load(2,29,128,true);
                a.floating_load(1,30,480); a.floating_load(0,29,180,true); a.floating_product(0,1,0);
                a.floating_product_sum(0,3,2,0); a.floating_product_sum(0,5,4,0); a.floating_product_sum(0,7,6,0);
                a.floating_sum(0,8,0); a.floating_sum(0,9,0); a.floating_integer(0,0); a.floating_store(0,1,56);
                a.load(3,1,0x3c); a.call(0x101d3c90); a.floating_load(1,30,96); a.exclusive_flags(0,3,0x80000000);
                a.store(1,0x44,0); a.value(0,0x43300000); a.store(1,0x40,0); a.floating_load(0,1,64);
                a.floating_subtract(0,0,1,true); a.floating_product_single(30,31,0); a.jump(0x101e19b8);
            }
            a.at(0x101e19b8);
        } else {
            a.at(0x101e1894); a.floating_load(1,31,60,true); a.floating_load(0,29,164,true);
            a.floating_sum_single(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,56); a.load(3,1,0x3c);
            a.call(0x101d3c90); a.floating_load(1,30,96); a.exclusive_flags(0,3,0x80000000); a.store(1,0x44,0);
            a.value(0,0x43300000); a.store(1,0x40,0); a.floating_load(0,1,64); a.floating_subtract(30,0,1,true);
            a.jump(0x101e19b8);
        }
        a.at(0x101e19b8);
    } else {
        a.at(0x101e1850); a.floating_load(0,29,204,true); a.floating_integer(0,0); a.floating_store(0,1,64);
        a.load(3,1,0x44); a.call(0x101d3c90); a.floating_load(1,30,96); a.exclusive_flags(0,3,0x80000000);
        a.store(1,0x3c,0); a.value(0,0x43300000); a.store(1,0x38,0); a.floating_load(0,1,56);
        a.floating_subtract(30,0,1,true); a.jump(0x101e19b8);
    }
    a.at(0x101e19b8);
}
void context_profile_weight(Call& a) {
    a.at(0x101e179c); a.load(0,29,0x70); a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x101e17a8); a.load(3,29,0x70); a.load<2>(0,3,0x4,true); a.compare(0,140);
        if (a.nonpositive()) {
            a.at(0x101e17c4); a.load(3,29,0x70); a.load<2>(0,3,0x4,true); a.compare(0,60);
            if (a.at_least()) {
                a.at(0x101e17e0); a.floating_load(2,30,504); a.load(3,29,0x70); a.load<2>(0,3,0x4,true);
                a.floating_load(1,30,96); a.exclusive_flags(0,0,0x80000000); a.store(1,0x44,0);
                a.value(0,0x43300000); a.store(1,0x40,0); a.floating_load(0,1,64); a.floating_subtract(0,0,1,true);
                a.floating_product(0,2,0); a.floating_round_single(0,0); a.floating_product_single(31,31,0);
            } else {
                a.at(0x101e17d4); a.floating_load(0,31,164,true); a.floating_product_single(31,31,0);
                a.jump(0x101e1814);
            }
            a.at(0x101e1814);
        } else {
            a.at(0x101e17b8); a.floating_load(0,31,160,true); a.floating_product_single(31,31,0);
            a.jump(0x101e1814);
        }
        a.at(0x101e1814); a.value(3,0x14); a.call(0x101d3c90); a.move(24,3); a.load(3,29,0x70);
        a.load<1>(3,3,0x21); a.sign8(3,3); a.call(0x101d3c90); a.compare_registers(3,24);
        if (a.less()) {
            a.at(0x101e1838); a.floating_load(0,31,168,true); a.floating_product_single(31,31,0);
        }
        a.at(0x101e1840);
    }
    a.at(0x101e1840);
}
void context_role_weight(Call& a) {
    a.at(0x101e16c8);a.sign16(0,28);a.mask(0,0,0x2,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e16d8);a.floating_load(31,31,0x98,true);a.jump(0x101e179c);return;}
    a.at(0x101e16e0);a.sign16(0,28);a.mask(0,0,0x4,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e16f0);a.floating_load(31,31,0x9c,true);a.jump(0x101e179c);return;}
    a.at(0x101e16f8);a.sign16(0,28);a.mask(0,0,0x8,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e1708);a.floating_load(31,31,0x54,true);a.jump(0x101e179c);return;}
    a.at(0x101e1710);a.sign16(0,28);a.mask(0,0,0x10,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e1720);a.floating_load(31,31,0x44,true);a.jump(0x101e179c);return;}
    a.at(0x101e1728);a.sign16(0,28);a.mask(0,0,0x20,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e1738);a.floating_load(31,31,0x84,true);a.jump(0x101e179c);return;}
    a.at(0x101e1740);a.sign16(0,28);a.mask(0,0,0x40,false,0);a.compare(0,0);
    if(!a.equal()){
        a.at(0x101e1750);a.sign16(0,28);a.mask(0,0,0x100,false,0);a.compare(0,0);
        bool qualified=!a.equal();
        if(!qualified){a.at(0x101e1760);a.sign16(0,28);a.mask(0,0,0x200,false,0);a.compare(0,0);qualified=!a.equal();}
        if(!qualified){a.at(0x101e1770);a.sign16(0,28);a.mask(0,0,0x400,false,0);a.compare(0,0);qualified=!a.equal();}
        if(qualified){a.at(0x101e1780);a.floating_load(31,31,0x70,true);a.jump(0x101e179c);return;}
    }
    a.at(0x101e1788);a.sign16(0,28);a.mask(0,0,0x40,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e1798);a.floating_load(31,31,0x10,true);}
}
}
void select_context_weighted_actor(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101e1640,dependencies,observer);
    a.link(); a.floating_store(31,1,-8); a.floating_store(30,1,-16); a.floating_store(29,1,-24);
    a.multiple(24,false,-32); a.save_link(); a.frame(144); a.store(1,0xa8,3); a.store<1>(1,0xaf,4);
    a.store<1>(1,0xb3,5); a.load(31,2,-0x52c8); a.load(30,2,-0x52c4); a.floating_load(29,31,12,true);
    a.value(26,0xffffffff); a.load(4,1,0xa8); a.load<1>(0,1,0xaf); a.sign8(0,0); a.multiply(3,0,0x2350);
    a.offset(29,3,0x311c); a.add(29,4,29); a.value(27,0x0); a.jump(0x101e1a14);
    for (;;) {
        a.at(0x101e1a14); a.sign8(0,27); a.compare(0,20);
        if (!a.less()) break;
        a.at(0x101e1698); a.move(3,29); a.call(0x101d3b50); a.sign8(0,3); a.compare(0,0);
        if (a.at_least()) {
            a.at(0x101e16ac); a.move(3,29); a.call(0x10289560); a.move(28,3); a.sign16(0,28);
            a.mask(0,0,0x1,false,0); a.compare(0,0);
            if (a.equal()) {
                a.at(0x101e16c8);
                context_role_weight(a);
                context_profile_weight(a);
                context_random_score(a);
                limit_context_score(a);
                keep_context_winner(a);
            }
            a.at(0x101e1a08);
        }
        a.at(0x101e1a08); a.offset(27,27,0x1); a.move(25,29); a.offset(29,29,0x1c4);
    }
    a.at(0x101e1a20); a.move(3,26); a.load(0,1,0x98); a.offset(1,1,0x90); a.floating_load(31,1,-8);
    a.floating_load(30,1,-16); a.floating_load(29,1,-24); a.restore_link(); a.multiple(24,true,-32); a.finish();
}
}
