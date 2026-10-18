#include "shortlist_assessment_detail.hpp"

namespace cm::aot::shortlist_assessment {
void assess_candidate_contribution(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,assess_candidate_contribution_entry,d,o);detail::prepare_contribution_context(a);
    a.at(0x10362d50);a.load<4>(0,1,0xe8,false);a.offset(1,1,0xe0);a.move(11,1);
    a.call(0x10773858);a.restore_link();a.multiple(13,true,-0x20);a.finish();
}
void evaluate_club_interest(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,evaluate_club_interest_entry,d,o);detail::prepare_interest_context(a);
    a.at(0x1036d21c);a.load<4>(0,1,0x1d8,false);a.offset(1,1,0x1d0);a.move(11,1);
    a.call(0x1077384c);a.restore_link();a.multiple(13,true,-0x40);a.finish();
}
}
