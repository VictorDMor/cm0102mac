#pragma once
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_fitness::detail {
using Call=SemanticGuestCall;
inline void advance_random_input(Call& a,std::uint32_t start,unsigned seed,unsigned words,unsigned end,unsigned cursor) {
    a.at(start);a.load(3,cursor,0);a.load(0,end,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(cursor,0,3);
    if(!a.nonpositive()){a.store(cursor,0,words);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(seed,0,0);}
    a.at(start+0x28);
}
inline void report_missing_staff(Call& a) {
    a.at(0x101876b0);a.offset(3,31,0x53);a.offset(6,1,0x48);a.offset(7,1,0x40);a.value(4,0);a.value(5,0);a.call(0x10739890);a.call(0x10768810);
    a.move(5,3);a.move(3,30);a.offset(4,31,0x5e);a.offset(6,1,0x48);a.offset(7,1,0x40);a.value(8,0x53f);a.call(0x1077b720);a.move(4,30);a.offset(3,31,0x6b);a.value(5,0);a.call(0x10769b60);
    a.load(4,2,-0x7bac);a.value(0,0);a.value(3,0);a.store<1>(4,0,0);a.jump(0x101877bc);
}
}
