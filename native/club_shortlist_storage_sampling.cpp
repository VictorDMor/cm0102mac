#include "club_shortlist_storage.hpp"
#include "range_sampling_detail.hpp"
namespace cm::aot::club_shortlist_storage {
void sample_shortlist_range(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    range_sampling_detail::sample_range(s,d,o,{sample_shortlist_range_entry,-0x4ddc,0,8});
}
}
