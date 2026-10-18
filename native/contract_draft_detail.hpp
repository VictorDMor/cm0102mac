#pragma once
#include "contract_draft.hpp"
#include "semantic_guest_call.hpp"
namespace cm::aot::contract_draft::detail {
using Call=SemanticGuestCall;
// The first option arguments are prepared on opposite sides of other stores
// by the two callers. Preserve that ordering before sharing the common tail.
inline void clear_clause_tail(Call& a,unsigned record) {
    a.call(0x10374b60);
    for(unsigned option=1;option<5;++option){a.move(3,record);a.value(4,option);a.call(0x10374b60);}
    for(unsigned bonus=0;bonus<3;++bonus){a.move(3,record);a.value(4,bonus);a.call(0x10374b80);}
    a.move(3,record);a.value(4,0);a.call(0x10380700);
}
void report_missing_contract(Call&);
}
