#include "shortlist_quotation_detail.hpp"

namespace cm::aot::shortlist_quotation {
void evaluate_candidate_context(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,evaluate_candidate_context_entry,d,o);detail::prepare_candidate_context(a);
    a.at(0x10369058);a.load<4>(0,1,0x178,false);a.offset(1,1,0x170);a.move(11,1);
    a.call(0x10773848);a.restore_link();a.multiple(13,true,-0x40);a.finish();
}
void calculate_reference_quote(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,calculate_reference_quote_entry,d,o);detail::prepare_reference_quote(a);
    a.at(0x1037e3bc);a.load<4>(0,1,0xc8,false);a.offset(1,1,0xc0);a.floating_load(31,1,-0x8,false);
    a.floating_load(30,1,-0x10,false);a.restore_link();a.multiple(13,true,-0x10);a.finish();
}
}
