#include "contract_draft_detail.hpp"

namespace cm::aot::contract_draft::detail {
void report_missing_contract(Call& a) {
    a.at(0x103723fc);a.move(3,24);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0);a.value(5,0);a.call(0x10739890);a.call(0x10768810);
    a.move(5,3);a.move(3,23);a.offset(4,24,0x14);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(8,0xf5);a.call(0x1077b720);
    a.move(4,23);a.offset(3,24,0x21);a.value(5,0);a.call(0x10769b60);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.jump(0x10372578);
}
}
namespace cm::aot::contract_draft {
void prepare_from_primary_contract(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,prepare_from_primary_contract_entry,d,o);
    a.link();a.multiple(22);a.load(10,2,-0x7b7c);a.move(25,3);a.load(31,2,-0x7b80);a.move(26,4);a.load(23,2,-0x7ba8);a.move(27,5);a.load(24,2,-0x4da4);a.move(28,6);a.move(22,7);a.move(29,8);a.move(30,9);a.store(1,8,0);a.frame(0x90);
    a.load(3,10,0);a.call(0x10082f20);a.compare(3,0,false);
    if(a.equal())detail::report_missing_contract(a);
    else {
        a.at(0x10372454);a.load(0,26,0);a.value(4,0x51ec0000);a.offset(9,4,-0x7ae1);a.value(10,0);a.store(25,0,0);a.sign8(6,22);a.value(7,6);a.value(0,0xffffffff);
        a.load(8,27,0);a.move(4,26);a.move(5,27);a.store(25,4,8);a.store(25,8,10);a.load(8,3,12);a.move(3,25);
        // Preserve the wrapped low-word product before signed division by100.
        a.product(6,8,6);a.multiply_high(6,9,6);a.shift_right(6,6,5);a.mask(8,6,1,false,1);a.add(6,6,8);a.store(25,12,6);
        a.load<2>(6,31,2);a.store<2>(25,0x2a,6);a.load<2>(6,31,0,true);a.store<2>(25,0x28,6);a.load<1>(6,31,4);a.store<1>(25,0x2c,6);
        a.load<2>(6,28,2);a.store<2>(25,0x32,6);a.load<2>(6,28,0,true);a.store<2>(25,0x30,6);a.load<1>(6,28,4);a.store<1>(25,0x34,6);
        a.load<1>(6,25,0x38);a.insert_mask(6,7,0xfc,2);a.store<1>(25,0x38,6);a.load(6,25,12);a.store(25,0x3c,6);a.store<1>(25,0x40,10);a.store<1>(25,0x54,10);a.store(25,0x58,0);a.call(0x1037f050);
        a.value(0,0);a.move(3,25);a.store<2>(25,0x42,0);a.value(4,0);detail::clear_clause_tail(a,25);a.store<1>(25,0x1c,29);a.store<1>(25,0x1d,30);
    }
    a.at(0x10372578);a.pop(0x90);a.multiple(22,true);a.finish();
}
}
