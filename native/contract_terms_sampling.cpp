#include "contract_terms.hpp"
#include "range_sampling_detail.hpp"

namespace cm::aot::contract_terms {
void sample_contract_range(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    range_sampling_detail::sample_range(s,d,o,{sample_contract_range_entry,-0x4dac,0});
}
}
