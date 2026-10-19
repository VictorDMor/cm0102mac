#include "match_selection.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_selection {
using Call = SemanticGuestCall;
namespace {
void role_weight(Call&);
void role_random_score(Call&);
void keep_role_winner(Call&);
void keep_role_winner(Call& a) {
    a.at(0x101e2280); a.sign8(0,27); a.compare(0,-1);
    bool replace=a.equal();
    if (!replace) {a.floating_compare(30,29);replace=!a.nonpositive();}
    if (replace) {a.at(0x101e2294);a.move(27,28);a.floating_move(29,30);}
}
void role_random_score(Call& a) {
    a.at(0x101e2254); a.value(3,0x19); a.call(0x101d3c90); a.load(4,2,-0x52c4); a.floating_load(1,4,96);
    a.exclusive_flags(0,3,0x80000000); a.store(1,0x3c,0); a.value(0,0x43300000); a.store(1,0x38,0);
    a.floating_load(0,1,56); a.floating_subtract(0,0,1,true); a.floating_product_single(30,31,0);
}
void role_weight(Call& a) {
    a.at(0x101e2180);a.sign16(0,30);a.mask(0,0,0x2,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e2190);a.floating_load(31,31,0xbc,true);a.jump(0x101e2254);return;}
    a.at(0x101e2198);a.sign16(0,30);a.mask(0,0,0x4,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e21a8);a.floating_load(31,31,0xbc,true);a.jump(0x101e2254);return;}
    a.at(0x101e21b0);a.sign16(0,30);a.mask(0,0,0x8,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e21c0);a.floating_load(31,31,0xb0,true);a.jump(0x101e2254);return;}
    a.at(0x101e21c8);a.sign16(0,30);a.mask(0,0,0x10,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e21d8);a.floating_load(31,31,0x54,true);a.jump(0x101e2254);return;}
    a.at(0x101e21e0);a.sign16(0,30);a.mask(0,0,0x20,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e21f0);a.floating_load(31,31,0x44,true);a.jump(0x101e2254);return;}
    a.at(0x101e21f8);a.sign16(0,30);a.mask(0,0,0x40,false,0);a.compare(0,0);
    if(!a.equal()){
        a.at(0x101e2208);a.sign16(0,30);a.mask(0,0,0x100,false,0);a.compare(0,0);
        bool qualified=!a.equal();
        if(!qualified){a.at(0x101e2218);a.sign16(0,30);a.mask(0,0,0x200,false,0);a.compare(0,0);qualified=!a.equal();}
        if(!qualified){a.at(0x101e2228);a.sign16(0,30);a.mask(0,0,0x400,false,0);a.compare(0,0);qualified=!a.equal();}
        if(qualified){a.at(0x101e2238);a.floating_load(31,31,0x58,true);a.jump(0x101e2254);return;}
    }
    a.at(0x101e2240);a.sign16(0,30);a.mask(0,0,0x40,false,0);a.compare(0,0);
    if(!a.equal()){a.at(0x101e2250);a.floating_load(31,31,0x10,true);}
}
}
void select_role_weighted_actor(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101e2100,dependencies,observer);
    a.link(); a.floating_store(31,1,-8); a.floating_store(30,1,-16); a.floating_store(29,1,-24);
    a.multiple(26,false,-32); a.save_link(); a.frame(128); a.store(1,0x98,3); a.store<1>(1,0x9f,4);
    a.load(31,2,-0x52c8); a.floating_load(29,31,12,true); a.value(27,0xffffffff); a.load(4,1,0x98);
    a.load<1>(0,1,0x9f); a.sign8(0,0); a.multiply(3,0,0x2350); a.offset(29,3,0x311c); a.add(29,4,29);
    a.value(28,0x0); a.jump(0x101e22a8);
    for (;;) {
        a.at(0x101e22a8); a.sign8(0,28); a.compare(0,20);
        if (!a.less()) break;
        a.at(0x101e2150); a.move(3,29); a.call(0x101d3b50); a.sign8(0,3); a.compare(0,0);
        if (a.at_least()) {
            a.at(0x101e2164); a.move(3,29); a.call(0x10289560); a.move(30,3); a.sign16(0,30);
            a.mask(0,0,0x1,false,0); a.compare(0,0);
            if (a.equal()) {
                a.at(0x101e2180);
                role_weight(a);
                role_random_score(a);
                keep_role_winner(a);
            }
            a.at(0x101e229c);
        }
        a.at(0x101e229c); a.offset(28,28,0x1); a.move(26,29); a.offset(29,29,0x1c4);
    }
    a.at(0x101e22b4); a.move(3,27); a.load(0,1,0x88); a.offset(1,1,0x80); a.floating_load(31,1,-8);
    a.floating_load(30,1,-16); a.floating_load(29,1,-24); a.restore_link(); a.multiple(26,true,-32); a.finish();
}
}
