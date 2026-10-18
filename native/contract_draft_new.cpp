#include "contract_draft_detail.hpp"

namespace cm::aot::contract_draft {
void prepare_new_contract(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,prepare_new_contract_entry,d,o);
    a.link();a.store(1,-4,31);a.move(31,3);a.value(7,0);a.store(1,-8,30);a.move(30,5);a.store(1,-12,29);a.move(29,4);a.store(1,8,0);a.frame(0x50);
    a.load(0,4,0);a.load(4,2,-0x7b80);a.store(3,0,0);a.value(0,200);a.offset(3,1,0x38);a.load(8,5,0);a.mask(5,6,0xff);
    a.store(31,4,8);a.store(31,8,7);a.store(31,12,0);a.load<2>(0,4,2);a.store<2>(31,0x2a,0);a.load<2>(0,4,0,true);a.store<2>(31,0x28,0);a.load<1>(0,4,4);a.store<1>(31,0x2c,0);
    a.call(0x10717a10);a.load<2>(3,1,0x3a);a.value(8,5);a.value(6,200);a.value(0,0);a.store<2>(31,0x32,3);a.move(3,31);a.move(4,29);a.move(5,30);
    a.load<2>(7,1,0x38,true);a.store<2>(31,0x30,7);a.load<1>(7,1,0x3c);a.store<1>(31,0x34,7);a.load<1>(7,31,0x38);a.insert_mask(7,8,0xfc,2);a.store<1>(31,0x38,7);
    a.store(31,0x3c,6);a.store<1>(31,0x40,0);a.call(0x1037f050);a.value(5,0);a.value(0,0xffffffff);a.store<1>(31,0x54,5);a.move(3,31);a.value(4,0);a.store(31,0x58,0);a.store<2>(31,0x42,5);
    detail::clear_clause_tail(a,31);a.pop(0x50);a.load(31,1,-4);a.load(30,1,-8);a.load(29,1,-12);a.finish();
}
}
