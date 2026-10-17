#include "injury_club_events.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_club_events {
namespace {
template<unsigned Field>
void compare_queued_date(State& state,std::uint32_t entry,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    SemanticGuestCall a(state,entry,dependencies,observer);
    a.load(5,4,Field);a.load(0,4,Field+4);a.store(1,-8,5);a.load<2>(6,3,Field+2);
    a.load<2>(4,1,-6);a.store(1,-24,5);a.compare_registers(4,6,false);a.store(1,-20,0);a.store(1,-4,0);
    if(!a.nonpositive()) {a.value(0,1);a.jump(entry+0x5c);}
    else {
        a.at(entry+0x30);
        if(!a.at_least()) {a.value(0,0);a.jump(entry+0x5c);}
        else {
            a.at(entry+0x3c);a.load<2>(5,1,-8,true);a.load<2>(0,3,Field,true);a.combine(0,5,0,false,true);
            a.shift_right(4,0,1);a.intersect(0,0,5);a.subtract(0,0,4);a.mask(0,0,1,false,1);a.sign8(0,0);
        }
    }
    a.at(entry+0x5c);a.sign8(0,0,true);
    if(!a.equal()) {a.value(3,0xffffffff);a.finish();return;}
    a.at(entry+0x6c);a.load(4,1,-24);a.load(0,1,-20);a.store(1,-16,4);a.load<2>(4,1,-14);a.store(1,-12,0);
    a.compare_registers(4,6,false);
    if(!a.at_least()) {a.value(0,1);a.jump(entry+0xbc);}
    else {
        a.at(entry+0x90);
        if(!a.nonpositive()) {a.value(0,0);a.jump(entry+0xbc);}
        else {
            a.at(entry+0x9c);a.load<2>(0,1,-16,true);a.load<2>(4,3,Field,true);a.combine(0,4,0,false,true);
            a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);
        }
    }
    a.at(entry+0xbc);a.sign8(3,0);a.negate(0,3);a.combine(0,0,3);a.mask(3,0,1,false,1);a.finish();
}
}
void compare_event_date(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {compare_queued_date<0>(s,compare_entry,d,o);}
}
