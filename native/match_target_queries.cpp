#include "match_target.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_target {
using Call = SemanticGuestCall;
void target_advance_enabled(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101d5760,dependencies,observer);
    a.at(0x101d5760); a.link(); a.save_link(); a.frame(64); a.store(1,0x58,3); a.store<1>(1,0x5f,4);
    a.load(4,1,0x58); a.load<1>(0,1,0x5f); a.sign8(0,0); a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c);
    a.add(3,4,3); a.call(0x101d56f0); a.mask(0,3,0x1000,false,0); a.compare(0,0);
    if (a.equal()) {
        a.at(0x101d57a4); a.value(3,0x0);
    } else {
        a.at(0x101d579c); a.value(3,0x1); a.jump(0x101d57a8);
    }
    a.at(0x101d57a8); a.pop(64); a.finish();
}
namespace {
void region_side_control(Call&);
void no_side_target_control(Call&);
void no_side_target_control(Call& a) {
    a.at(0x101e3134); a.value(3,0x0);
}
void region_side_control(Call& a) {
    a.at(0x101e30f0); a.move(3,31); a.sign8(0,30); a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27);
    a.sign8(4,0); a.load<1>(5,31,0x791d); a.load<1>(6,31,0x791e); a.call(0x101eea30); a.sign8(0,3); a.compare(0,0);
    if (a.equal()) {
        a.at(0x101e3134);
        no_side_target_control(a);
    } else {
        a.at(0x101e311c); a.load<1>(0,31,0x791f); a.sign8(0,0); a.compare(0,-1);
        if (a.equal()) {
            a.at(0x101e3134);
            no_side_target_control(a);
        } else {
            a.at(0x101e312c); a.value(3,0x1); a.jump(0x101e3138);
        }
        a.at(0x101e3138);
    }
    a.at(0x101e3138);
}
}
void side_target_control(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101e3060,dependencies,observer);
    a.at(0x101e3060); a.link(); a.store(1,-0x4,31); a.store(1,-0x8,30); a.save_link(); a.frame(64); a.move(31,3);
    a.move(30,4); a.load<1>(0,31,0x7924); a.sign8(3,0); a.sign8(0,30); a.leading_zeroes(0,0);
    a.mask(0,0,0x7ffffff,false,27); a.compare_registers(3,0);
    if (!a.equal()) {
        a.at(0x101e30a0); a.sign8(3,30); a.load<1>(0,31,0x7924); a.sign8(0,0); a.compare_registers(3,0);
        if (!a.equal()) {
            a.at(0x101e30bc); a.offset(3,31,0x10000); a.load(0,3,-0x2008); a.compare(0,0,false);
            if (a.equal()) {
                a.at(0x101e30f0);
                region_side_control(a);
            } else {
                a.at(0x101e30cc); a.offset(3,31,0x10000); a.load(3,3,-0x2008); a.load<1>(0,3,0x27); a.sign8(3,0);
                a.sign8(0,30); a.compare_registers(3,0);
                if (!a.equal()) {
                    a.at(0x101e30f0);
                    region_side_control(a);
                } else {
                    a.at(0x101e30e8); a.value(3,0x1); a.jump(0x101e3138);
                }
                a.at(0x101e3138);
            }
            a.at(0x101e3138);
        } else {
            a.at(0x101e30b4); a.value(3,0x1); a.jump(0x101e3138);
        }
        a.at(0x101e3138);
    } else {
        a.at(0x101e3098); a.value(3,0x0); a.jump(0x101e3138);
    }
    a.at(0x101e3138); a.pop(64); a.load(31,1,-0x4); a.load(30,1,-0x8); a.finish();
}
void normalized_actor_side(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101e3150,dependencies,observer);
    a.at(0x101e3150); a.load<1>(0,3,0x27); a.sign8(4,0); a.value(0,0x1); a.sign8(0,0); a.subtract(0,4,0);
    a.leading_zeroes(0,0); a.mask(0,0,0x7ffffff,false,27); a.sign8(3,0); a.finish();
}
namespace {
void no_controlled_override(Call&);
void no_controlled_override(Call& a) {
    a.at(0x10286190); a.value(3,0x0);
}
}
void controlled_target_override(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x102860c0,dependencies,observer);
    a.at(0x102860c0); a.link(); a.store(1,-0x4,31); a.move(31,3); a.save_link(); a.frame(64); a.load<1>(0,3,0x27);
    a.load(5,3,0x1a4); a.sign8(0,0); a.load<1>(4,3,0x19); a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c);
    a.add(3,5,3); a.call(0x10113e10); a.value(6,0x20000); a.value(7,0x0); a.offset(6,6,0x0); a.value(5,0x0);
    a.intersect(6,4,6); a.intersect(4,3,7); a.value(0,0x0); a.combine(3,6,5,false,true);
    a.combine(0,4,0,false,true); a.combine(0,3,0,true);
    if (a.equal()) {
        a.at(0x10286128); a.move(3,31); a.call(0x10289560); a.sign16(0,3); a.compare(0,1);
        if (a.equal()) {
            a.at(0x10286190);
            no_controlled_override(a);
        } else {
            a.at(0x1028613c); a.load<1>(0,31,0x27); a.load(5,31,0x1a4); a.sign8(0,0); a.load<1>(4,31,0x19);
            a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x10113e10); a.value(6,0x40000000);
            a.value(7,0x0); a.offset(6,6,0x0); a.value(5,0x0); a.intersect(6,4,6); a.intersect(4,3,7);
            a.value(0,0x0); a.combine(3,6,5,false,true); a.combine(0,4,0,false,true); a.combine(0,3,0,true);
            if (a.equal()) {
                a.at(0x10286190);
                no_controlled_override(a);
            } else {
                a.at(0x10286188); a.value(3,0x2); a.jump(0x10286194);
            }
            a.at(0x10286194);
        }
        a.at(0x10286194);
    } else {
        a.at(0x10286120); a.value(3,0x1); a.jump(0x10286194);
    }
    a.at(0x10286194); a.pop(64); a.load(31,1,-0x4); a.finish();
}
namespace {
void secondary_uncontrolled_override(Call&);
void no_uncontrolled_override(Call&);
void no_uncontrolled_override(Call& a) {
    a.at(0x10286290); a.value(3,0x0);
}
void secondary_uncontrolled_override(Call& a) {
    a.at(0x10286228); a.move(3,31); a.call(0x10289560); a.sign16(0,3); a.compare(0,1);
    if (a.equal()) {
        a.at(0x10286290);
        no_uncontrolled_override(a);
    } else {
        a.at(0x1028623c); a.load<1>(0,31,0x27); a.load(5,31,0x1a4); a.sign8(0,0); a.load<1>(4,31,0x19);
        a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x10113e10); a.value(6,0x80000);
        a.value(7,0x0); a.offset(6,6,0x0); a.value(5,0x0); a.intersect(6,4,6); a.intersect(4,3,7); a.value(0,0x0);
        a.combine(3,6,5,false,true); a.combine(0,4,0,false,true); a.combine(0,3,0,true);
        if (a.equal()) {
            a.at(0x10286290);
            no_uncontrolled_override(a);
        } else {
            a.at(0x10286288); a.value(3,0x4); a.jump(0x10286294);
        }
        a.at(0x10286294);
    }
    a.at(0x10286294);
}
}
void uncontrolled_target_override(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x102861b0,dependencies,observer);
    a.at(0x102861b0); a.link(); a.store(1,-0x4,31); a.move(31,3); a.save_link(); a.frame(64); a.call(0x10289560);
    a.sign16(0,3); a.compare(0,1);
    if (a.equal()) {
        a.at(0x10286228);
        secondary_uncontrolled_override(a);
    } else {
        a.at(0x102861d4); a.load<1>(0,31,0x27); a.load(5,31,0x1a4); a.sign8(0,0); a.load<1>(4,31,0x19);
        a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,5,3); a.call(0x10113e10); a.value(6,0x80000000);
        a.value(7,0x0); a.offset(6,6,0x0); a.value(5,0x0); a.intersect(6,4,6); a.intersect(4,3,7); a.value(0,0x0);
        a.combine(3,6,5,false,true); a.combine(0,4,0,false,true); a.combine(0,3,0,true);
        if (a.equal()) {
            a.at(0x10286228);
            secondary_uncontrolled_override(a);
        } else {
            a.at(0x10286220); a.value(3,0x3); a.jump(0x10286294);
        }
        a.at(0x10286294);
    }
    a.at(0x10286294); a.pop(64); a.load(31,1,-0x4); a.finish();
}
void strategy_flags(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10113e10,dependencies,observer);
    a.at(0x10113e10); a.sign8(0,4); a.mask(0,0,0xfffffff8,false,3); a.add(4,3,0); a.load(3,4,0x1888);
    a.load(4,4,0x188c); a.finish();
}
}
