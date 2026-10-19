#include "match_selection_support.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_selection_support {
using Call = SemanticGuestCall;
namespace {
void score_role_weight(Call&);
void mode_random_score(Call&);
void keep_score_winner(Call&);
void keep_score_winner(Call& a) {
    a.at(0x101e1b6c); a.sign8(0,28); a.compare(0,-1);
    bool replace=a.equal();
    if (!replace) {a.floating_compare(30,29);replace=!a.nonpositive();}
    if (replace) {a.at(0x101e1b80);a.move(28,29);a.floating_move(29,30);}
}
void mode_random_score(Call& a) {
    a.at(0x101e1ae0); a.load<1>(0,1,0xa3); a.sign8(0,0); a.compare(0,2);
    if (!a.equal()) {
        a.at(0x101e1b30); a.move(3,30); a.value(4,0x0); a.call(0x102875f0); a.sign8(3,3); a.offset(3,3,0x5);
        a.call(0x101d3c90); a.load(4,2,-0x52c4); a.floating_load(1,4,96); a.exclusive_flags(0,3,0x80000000);
        a.store(1,0x3c,0); a.value(0,0x43300000); a.store(1,0x38,0); a.floating_load(0,1,56);
        a.floating_subtract(0,0,1,true); a.floating_product_single(30,31,0);
    } else {
        a.at(0x101e1af0); a.move(3,30); a.value(4,0x0); a.call(0x102875f0); a.sign8(3,3); a.offset(3,3,0x19);
        a.call(0x101d3c90); a.load(4,2,-0x52c4); a.floating_load(1,4,96); a.exclusive_flags(0,3,0x80000000);
        a.store(1,0x3c,0); a.value(0,0x43300000); a.store(1,0x38,0); a.floating_load(0,1,56);
        a.floating_subtract(0,0,1,true); a.floating_product_single(30,31,0); a.jump(0x101e1b6c);
    }
    a.at(0x101e1b6c);
}
void score_role_weight(Call& a) {
    a.at(0x101e1ac4); a.sign16(0,27); a.mask(0,0,0x1,false,0); a.compare(0,0);
    if (a.equal()) {
        a.at(0x101e1adc); a.floating_load(31,31,16,true);
    } else {
        a.at(0x101e1ad4); a.floating_load(31,31,176,true); a.jump(0x101e1ae0);
    }
    a.at(0x101e1ae0);
}
}
void select_score_weighted_actor(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101e1a50,dependencies,observer);
    a.link(); a.floating_store(31,1,-8); a.floating_store(30,1,-16); a.floating_store(29,1,-24);
    a.multiple(26,false,-32); a.save_link(); a.frame(128); a.store(1,0x98,3); a.store<1>(1,0x9f,4);
    a.store<1>(1,0xa3,5); a.load(31,2,-0x52c8); a.floating_load(29,31,12,true); a.value(28,0xffffffff);
    a.load(4,1,0x98); a.load<1>(0,1,0x9f); a.sign8(0,0); a.multiply(3,0,0x2350); a.offset(30,3,0x311c);
    a.add(30,4,30); a.value(29,0x0); a.jump(0x101e1b94);
    for (;;) {
        a.at(0x101e1b94); a.sign8(0,29); a.compare(0,20);
        if (!a.less()) break;
        a.at(0x101e1aa4); a.move(3,30); a.call(0x101d3b50); a.sign8(0,3); a.compare(0,0);
        if (a.at_least()) {
            a.at(0x101e1ab8); a.move(3,30); a.call(0x10289560); a.move(27,3);
            score_role_weight(a);
            mode_random_score(a);
            keep_score_winner(a);
        }
        a.at(0x101e1b88); a.offset(29,29,0x1); a.move(26,30); a.offset(30,30,0x1c4);
    }
    a.at(0x101e1ba0); a.move(3,28); a.load(0,1,0x88); a.offset(1,1,0x80); a.floating_load(31,1,-8);
    a.floating_load(30,1,-16); a.floating_load(29,1,-24); a.restore_link(); a.multiple(26,true,-32); a.finish();
}
}
