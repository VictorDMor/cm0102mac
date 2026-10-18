#include "contract_draft_detail.hpp"

namespace cm::aot::contract_draft {
void initialize_draft(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,initialize_draft_entry,d,o);
    a.link();a.store(1,-4,31);a.move(31,3);a.store(1,8,0);a.offset(3,31,0x28);a.frame(0x40);a.call(0x10715880);
    a.offset(3,31,0x30);a.call(0x10715880);a.value(4,0xffffffff);a.value(0,0);a.store(31,4,4);a.move(3,31);
    a.store<1>(31,0x54,0);a.store(31,0x58,4);a.store<2>(31,0x42,0);a.store<1>(31,0x5c,0);
    a.store(31,0x48,0);a.store(31,0x50,0);a.store(31,8,0);a.pop(0x40);a.load(31,1,-4);a.finish();
}
}
