#include "match_selection.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_selection {
using Call = SemanticGuestCall;
namespace {
void home_owner_target(Call&);
void home_opponent_target(Call&);
void away_owner_target(Call&);
void away_opponent_target(Call&);
void neutral_owner_target(Call&);
void neutral_owner_target(Call& a) {
    a.at(0x101e280c); a.move(3,31); a.call(0x101d3b50); a.sign8(0,3); a.multiply(4,0,0xd8); a.load<1>(0,31,0x27);
    a.sign8(0,0); a.mask(3,0,0xfffffffc,false,2); a.offset(3,3,0x10000); a.offset(3,3,-0x2014);
    a.indexed_load(3,30,3); a.offset(0,4,0x35); a.indexed_load<1>(0,3,0); a.mask(0,0,0xf,false,0); a.sign8(0,0);
    a.store<1>(28,0x0,0); a.move(3,31); a.call(0x101d3b50); a.sign8(0,3); a.multiply(4,0,0xd8);
    a.load<1>(0,31,0x27); a.sign8(0,0); a.mask(3,0,0xfffffffc,false,2); a.offset(3,3,0x10000);
    a.offset(3,3,-0x2014); a.indexed_load(3,30,3); a.offset(0,4,0x35); a.indexed_load<1>(0,3,0);
    a.mask(0,0,0xf0,false,0); a.shift_right(0,0,0x4); a.sign8(0,0); a.store<1>(29,0x0,0);
}
void away_opponent_target(Call& a) {
    a.at(0x101e278c); a.move(3,31); a.call(0x101d3b50); a.sign8(0,3); a.multiply(4,0,0xd8); a.load<1>(0,31,0x27);
    a.sign8(0,0); a.mask(3,0,0xfffffffc,false,2); a.offset(3,3,0x10000); a.offset(3,3,-0x2014);
    a.indexed_load(3,30,3); a.offset(0,4,0x3b); a.indexed_load<1>(0,3,0); a.mask(0,0,0xf,false,0); a.sign8(0,0);
    a.store<1>(28,0x0,0); a.move(3,31); a.call(0x101d3b50); a.sign8(0,3); a.multiply(4,0,0xd8);
    a.load<1>(0,31,0x27); a.sign8(0,0); a.mask(3,0,0xfffffffc,false,2); a.offset(3,3,0x10000);
    a.offset(3,3,-0x2014); a.indexed_load(3,30,3); a.offset(0,4,0x3b); a.indexed_load<1>(0,3,0);
    a.mask(0,0,0xf0,false,0); a.shift_right(0,0,0x4); a.sign8(0,0); a.store<1>(29,0x0,0); a.jump(0x101e2888);
}
void away_owner_target(Call& a) {
    a.at(0x101e270c); a.move(3,31); a.call(0x101d3b50); a.sign8(0,3); a.multiply(4,0,0xd8); a.load<1>(0,31,0x27);
    a.sign8(0,0); a.mask(3,0,0xfffffffc,false,2); a.offset(3,3,0x10000); a.offset(3,3,-0x2014);
    a.indexed_load(3,30,3); a.offset(0,4,0xa4); a.indexed_load<1>(0,3,0); a.mask(0,0,0xf,false,0); a.sign8(0,0);
    a.store<1>(28,0x0,0); a.move(3,31); a.call(0x101d3b50); a.sign8(0,3); a.multiply(4,0,0xd8);
    a.load<1>(0,31,0x27); a.sign8(0,0); a.mask(3,0,0xfffffffc,false,2); a.offset(3,3,0x10000);
    a.offset(3,3,-0x2014); a.indexed_load(3,30,3); a.offset(0,4,0xa4); a.indexed_load<1>(0,3,0);
    a.mask(0,0,0xf0,false,0); a.shift_right(0,0,0x4); a.sign8(0,0); a.store<1>(29,0x0,0); a.jump(0x101e2888);
}
void home_opponent_target(Call& a) {
    a.at(0x101e266c); a.move(3,31); a.call(0x101d3b50); a.sign8(0,3); a.multiply(4,0,0xd8); a.load<1>(0,31,0x27);
    a.sign8(0,0); a.mask(3,0,0xfffffffc,false,2); a.offset(3,3,0x10000); a.offset(3,3,-0x2014);
    a.indexed_load(3,30,3); a.offset(0,4,0x30); a.indexed_load<1>(0,3,0); a.mask(0,0,0xf,false,0); a.sign8(0,0);
    a.store<1>(28,0x0,0); a.move(3,31); a.call(0x101d3b50); a.sign8(0,3); a.multiply(4,0,0xd8);
    a.load<1>(0,31,0x27); a.sign8(0,0); a.mask(3,0,0xfffffffc,false,2); a.offset(3,3,0x10000);
    a.offset(3,3,-0x2014); a.indexed_load(3,30,3); a.offset(0,4,0x30); a.indexed_load<1>(0,3,0);
    a.mask(0,0,0xf0,false,0); a.shift_right(0,0,0x4); a.sign8(0,0); a.store<1>(29,0x0,0); a.jump(0x101e2888);
}
void home_owner_target(Call& a) {
    a.at(0x101e25ec); a.move(3,31); a.call(0x101d3b50); a.sign8(0,3); a.multiply(4,0,0xd8); a.load<1>(0,31,0x27);
    a.sign8(0,0); a.mask(3,0,0xfffffffc,false,2); a.offset(3,3,0x10000); a.offset(3,3,-0x2014);
    a.indexed_load(3,30,3); a.offset(0,4,0x9f); a.indexed_load<1>(0,3,0); a.mask(0,0,0xf,false,0); a.sign8(0,0);
    a.store<1>(28,0x0,0); a.move(3,31); a.call(0x101d3b50); a.sign8(0,3); a.multiply(4,0,0xd8);
    a.load<1>(0,31,0x27); a.sign8(0,0); a.mask(3,0,0xfffffffc,false,2); a.offset(3,3,0x10000);
    a.offset(3,3,-0x2014); a.indexed_load(3,30,3); a.offset(0,4,0x9f); a.indexed_load<1>(0,3,0);
    a.mask(0,0,0xf0,false,0); a.shift_right(0,0,0x4); a.sign8(0,0); a.store<1>(29,0x0,0); a.jump(0x101e2888);
}
}
void preset_target_cell(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101e25a0,dependencies,observer);
    a.at(0x101e25a0); a.link(); a.store(1,-0x4,31); a.store(1,-0x8,30); a.store(1,-0xc,29); a.store(1,-0x10,28);
    a.save_link(); a.frame(80); a.move(30,3); a.move(31,4); a.move(28,5); a.move(29,6); a.load<1>(0,30,0x7924);
    a.sign8(0,0); a.compare(0,0);
    if (!a.equal()) {
        a.at(0x101e26ec); a.load<1>(0,30,0x7924); a.sign8(0,0); a.compare(0,1);
        if (!a.equal()) {
            a.at(0x101e280c);
            neutral_owner_target(a);
        } else {
            a.at(0x101e26fc); a.load<1>(0,31,0x27); a.sign8(0,0); a.compare(0,1);
            if (!a.equal()) {
                a.at(0x101e278c);
                away_opponent_target(a);
            } else {
                a.at(0x101e270c);
                away_owner_target(a);
            }
            a.at(0x101e2888);
        }
        a.at(0x101e2888);
    } else {
        a.at(0x101e25dc); a.load<1>(0,31,0x27); a.sign8(0,0); a.compare(0,0);
        if (!a.equal()) {
            a.at(0x101e266c);
            home_opponent_target(a);
        } else {
            a.at(0x101e25ec);
            home_owner_target(a);
        }
        a.at(0x101e2888);
    }
    a.at(0x101e2888); a.pop(80); a.load(31,1,-0x4); a.load(30,1,-0x8); a.load(29,1,-0xc); a.load(28,1,-0x10);
    a.finish();
}
}
