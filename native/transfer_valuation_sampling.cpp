#include "transfer_valuation.hpp"
#include "range_sampling_detail.hpp"

namespace cm::aot::transfer_valuation {
void sample_offer_range(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    range_sampling_detail::sample_range(s,d,o,{sample_offer_range_entry,-0x4b8c,0xa0});
}
}
