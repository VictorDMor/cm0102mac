#pragma once
#include "ball_contact_adapter.hpp"
#include "match_discipline_adapter.hpp"
#include "semantic_io_checks.hpp"

namespace cm::ball_contact_checks {
using namespace cm::aot;
inline unsigned replacement_index(unsigned index){return index<2?index:index-2;}
template<class F> decltype(auto) diagnostics(State& state,unsigned index,F&& use) {
    if(index==2||index==3)return use(match_discipline_diagnostics(state),index-2);
    return use(ball_contact_diagnostics(state),replacement_index(index));
}
inline void original_block(State& state) {
    ball_contact_diagnostics(state).original=true;
    auto& d=match_discipline_diagnostics(state);d.entry_original[0]=d.entry_original[1]=true;
}
inline bool invoke(State& state,unsigned index) {
    const auto entry=ball_contact::entries[index];
    return index==2||index==3?try_modern_match_discipline(state,entry):try_modern_ball_contact(state,entry);
}
struct Observer:BallContactBoundaryObserver,MatchDisciplineBoundaryObserver {
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State&,std::uint32_t,bool,BallContactOutcome)override{}
    void leave(State&,std::uint32_t,bool,MatchDisciplineOutcome)override{}
};
inline void observe(State& state,Observer* observer) {
    ball_contact_diagnostics(state).observer=observer;
    match_discipline_diagnostics(state).observer=observer;
}
}
