#include "match_selection.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_selection {
using Call = SemanticGuestCall;
namespace {
void profile_role_weight(Call&);
void profile_live_modifiers(Call&);
void profile_random_score(Call&);
void keep_profile_winner(Call&);
void keep_profile_winner(Call& a) {
    a.at(0x101e2540); a.sign8(0,27); a.compare(0,-1);
    bool replace=a.equal();
    if (!replace) {a.floating_compare(30,29);replace=!a.nonpositive();}
    if (replace) {a.at(0x101e2554);a.move(27,28);a.floating_move(29,30);}
}
void profile_random_score(Call& a) {
    a.at(0x101e2514); a.value(3,0x19); a.call(0x101d3c90); a.load(4,2,-0x52c4); a.floating_load(1,4,96);
    a.exclusive_flags(0,3,0x80000000); a.store(1,0x3c,0); a.value(0,0x43300000); a.store(1,0x38,0);
    a.floating_load(0,1,56); a.floating_subtract(0,0,1,true); a.floating_product_single(30,31,0);
}
void profile_live_modifiers(Call& a) {
    a.at(0x101e2434); a.load(0,30,0x70); a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x101e2440); a.load(3,30,0x70); a.load<1>(0,3,0x44); a.sign8(0,0); a.compare(0,15);
        if (a.nonpositive()) {
            a.at(0x101e2460); a.load(3,30,0x70); a.load<1>(0,3,0x44); a.sign8(0,0); a.compare(0,10);
            if (a.nonpositive()) {
                a.at(0x101e2480); a.load(3,30,0x70); a.load<1>(0,3,0x44); a.sign8(0,0); a.compare(0,5);
                if (a.less()) {
                    a.at(0x101e2494); a.floating_load(0,31,92,true); a.floating_product_single(31,31,0);
                }
                a.at(0x101e249c);
            } else {
                a.at(0x101e2474); a.floating_load(0,31,88,true); a.floating_product_single(31,31,0);
                a.jump(0x101e249c);
            }
            a.at(0x101e249c);
        } else {
            a.at(0x101e2454); a.floating_load(0,31,112,true); a.floating_product_single(31,31,0);
            a.jump(0x101e249c);
        }
        a.at(0x101e249c); a.load(3,30,0x70); a.load<1>(0,3,0x39); a.sign8(0,0); a.compare(0,15);
        if (a.nonpositive()) {
            a.at(0x101e24bc); a.load(3,30,0x70); a.load<1>(0,3,0x39); a.sign8(0,0); a.compare(0,5);
            if (a.less()) {
                a.at(0x101e24d0); a.floating_load(0,31,92,true); a.floating_product_single(31,31,0);
            }
            a.at(0x101e24d8);
        } else {
            a.at(0x101e24b0); a.floating_load(0,31,112,true); a.floating_product_single(31,31,0);
            a.jump(0x101e24d8);
        }
        a.at(0x101e24d8); a.load(3,30,0x70); a.load<1>(0,3,0x3e); a.sign8(0,0); a.compare(0,15);
        if (a.nonpositive()) {
            a.at(0x101e24f8); a.load(3,30,0x70); a.load<1>(0,3,0x3e); a.sign8(0,0); a.compare(0,5);
            if (a.less()) {
                a.at(0x101e250c); a.floating_load(0,31,92,true); a.floating_product_single(31,31,0);
            }
            a.at(0x101e2514);
        } else {
            a.at(0x101e24ec); a.floating_load(0,31,112,true); a.floating_product_single(31,31,0);
            a.jump(0x101e2514);
        }
        a.at(0x101e2514);
    }
    a.at(0x101e2514);
}
void profile_role_weight(Call& a) {
    a.at(0x101e2360);a.sign16(0,29);a.mask(0,0,0x2,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e2370);a.floating_load(31,31,0x8c,true);a.jump(0x101e2434);return;}
    a.at(0x101e2378);a.sign16(0,29);a.mask(0,0,0x4,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e2388);a.floating_load(31,31,0x8c,true);a.jump(0x101e2434);return;}
    a.at(0x101e2390);a.sign16(0,29);a.mask(0,0,0x8,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e23a0);a.floating_load(31,31,0x40,true);a.jump(0x101e2434);return;}
    a.at(0x101e23a8);a.sign16(0,29);a.mask(0,0,0x10,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e23b8);a.floating_load(31,31,0x54,true);a.jump(0x101e2434);return;}
    a.at(0x101e23c0);a.sign16(0,29);a.mask(0,0,0x20,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e23d0);a.floating_load(31,31,0xc0,true);a.jump(0x101e2434);return;}
    a.at(0x101e23d8);a.sign16(0,29);a.mask(0,0,0x40,false,0);a.compare(0,0);
    if(!a.equal()){
        a.at(0x101e23e8);a.sign16(0,29);a.mask(0,0,0x100,false,0);a.compare(0,0);
        bool qualified=!a.equal();
        if(!qualified){a.at(0x101e23f8);a.sign16(0,29);a.mask(0,0,0x200,false,0);a.compare(0,0);qualified=!a.equal();}
        if(!qualified){a.at(0x101e2408);a.sign16(0,29);a.mask(0,0,0x400,false,0);a.compare(0,0);qualified=!a.equal();}
        if(qualified){a.at(0x101e2418);a.floating_load(31,31,0xc4,true);a.jump(0x101e2434);return;}
    }
    a.at(0x101e2420);a.sign16(0,29);a.mask(0,0,0x40,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e2430);a.floating_load(31,31,0xb4,true);}
}
}
void select_profile_weighted_actor(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101e22e0,dependencies,observer);
    a.link(); a.floating_store(31,1,-8); a.floating_store(30,1,-16); a.floating_store(29,1,-24);
    a.multiple(26,false,-32); a.save_link(); a.frame(128); a.store(1,0x98,3); a.store<1>(1,0x9f,4);
    a.load(31,2,-0x52c8); a.floating_load(29,31,12,true); a.value(27,0xffffffff); a.load(4,1,0x98);
    a.load<1>(0,1,0x9f); a.sign8(0,0); a.multiply(3,0,0x2350); a.offset(30,3,0x311c); a.add(30,4,30);
    a.value(28,0x0); a.jump(0x101e2568);
    for (;;) {
        a.at(0x101e2568); a.sign8(0,28); a.compare(0,20);
        if (!a.less()) break;
        a.at(0x101e2330); a.move(3,30); a.call(0x101d3b50); a.sign8(0,3); a.compare(0,0);
        if (a.at_least()) {
            a.at(0x101e2344); a.move(3,30); a.call(0x10289560); a.move(29,3); a.sign16(0,29);
            a.mask(0,0,0x1,false,0); a.compare(0,0);
            if (a.equal()) {
                a.at(0x101e2360);
                profile_role_weight(a);
                profile_live_modifiers(a);
                profile_random_score(a);
                keep_profile_winner(a);
            }
            a.at(0x101e255c);
        }
        a.at(0x101e255c); a.offset(28,28,0x1); a.move(26,30); a.offset(30,30,0x1c4);
    }
    a.at(0x101e2574); a.move(3,27); a.load(0,1,0x88); a.offset(1,1,0x80); a.floating_load(31,1,-8);
    a.floating_load(30,1,-16); a.floating_load(29,1,-24); a.restore_link(); a.multiple(26,true,-32); a.finish();
}
}
