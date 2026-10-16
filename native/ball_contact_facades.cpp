#include "ball_contact.hpp"
#include "semantic_guest_call.hpp"
#include "match_discipline.hpp"

namespace cm::aot::ball_contact {
namespace {
using Call=SemanticGuestCall;
void enter(Call& a){a.link();a.save_link();a.frame(0x40);}
void match_context(Call& a){a.load(3,1,0x58);a.offset(3,3,0x10000);a.offset(3,3,-0x1fd0);}
void leave(Call& a){a.load(0,1,0x48);a.offset(1,1,0x40);a.restore_link();a.finish();}
}
void screen(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,facade_entries[0],dependencies,observer);enter(a);
    a.store(1,0x58,3);a.store(1,0x5c,4);a.store(1,0x60,5);a.store<1>(1,0x67,6);a.store<1>(1,0x6b,7);
    a.store(1,0x6c,8);a.store(1,0x70,9);a.store(1,0x74,10);match_context(a);
    a.load(4,1,0x5c);a.load(5,1,0x60);a.load<1>(6,1,0x67);a.load<1>(7,1,0x6b);a.load(8,1,0x6c);a.load(9,1,0x70);a.load(10,1,0x74);
    a.call(0x1021c260);leave(a);
}
void attempt(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,facade_entries[1],dependencies,observer);enter(a);
    a.store(1,0x58,3);a.store(1,0x5c,4);a.store<2>(1,0x62,5);a.store(1,0x64,6);a.store(1,0x68,7);match_context(a);
    a.load(4,1,0x5c);a.load<2>(5,1,0x62,true);a.load(6,1,0x64);a.load(7,1,0x68);a.call(0x1021b860);leave(a);
}
void apply(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    match_discipline::match_contact_penalty(state,dependencies,observer);
}
void resolve(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    match_discipline::match_single_actor_penalty(state,dependencies,observer);
}
}
