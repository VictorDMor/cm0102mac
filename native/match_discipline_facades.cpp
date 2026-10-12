#include "match_discipline.hpp"
#include "match_discipline_internal.hpp"

namespace cm::aot::match_discipline {
namespace {
void facade_frame(DisciplineCall& a) { a.link(); a.save_link(); a.frame(0x40); }
void facade_return(DisciplineCall& a, std::uint32_t site) {
    a.at(site); a.load(0,1,0x48); a.offset(1,1,0x40); a.restore_link(); a.finish();
}
void context_from_match(DisciplineCall& a) {
    a.load(3,1,0x58); a.offset(3,3,0x10000); a.offset(3,3,-0x1fd0);
}
}
void match_contact_penalty(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    DisciplineCall a(state,entries[0],dependencies,observer); facade_frame(a);
    a.store(1,0x58,3); a.store(1,0x5c,4); a.store(1,0x60,5); a.store(1,0x64,6);
    a.store<1>(1,0x6b,7); a.store<1>(1,0x6f,8); a.store<1>(1,0x73,9); a.store(1,0x74,10);
    a.load<1>(0,1,0x7b); a.store(1,0x38,0); context_from_match(a);
    a.load(4,1,0x5c); a.load(5,1,0x60); a.load(6,1,0x64); a.load<1>(7,1,0x6b);
    a.load<1>(8,1,0x6f); a.load<1>(9,1,0x73); a.load(10,1,0x74); a.call(entries[4]);
    facade_return(a,0x101f5170);
}
void match_single_actor_penalty(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    DisciplineCall a(state,entries[1],dependencies,observer); facade_frame(a);
    a.store(1,0x58,3); a.store(1,0x5c,4); a.store(1,0x60,5); a.store<1>(1,0x67,6);
    a.store<1>(1,0x6b,7); a.store<1>(1,0x6f,8); a.store(1,0x70,9); context_from_match(a);
    a.load(4,1,0x5c); a.load(5,1,0x60); a.load<1>(6,1,0x67); a.load<1>(7,1,0x6b);
    a.load<1>(8,1,0x6f); a.load(9,1,0x70); a.call(entries[3]); facade_return(a,0x101f51d0);
}
void match_referee_response(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    DisciplineCall a(state,entries[2],dependencies,observer); facade_frame(a);
    a.store(1,0x58,3); a.store(1,0x5c,4); a.store(1,0x60,5); a.store<1>(1,0x67,6);
    a.store<1>(1,0x6b,7); context_from_match(a); a.load(4,1,0x5c); a.load(5,1,0x60);
    a.load<1>(6,1,0x67); a.load<1>(7,1,0x6b); a.call(entries[5]); facade_return(a,0x101f5220);
}
}
