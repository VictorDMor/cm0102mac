#pragma once
#include "club_shortlist_storage.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::club_shortlist_storage::detail {
using Call=SemanticGuestCall;
inline void diagnostic(Call& a,std::uint32_t pc,unsigned table,unsigned buffer,unsigned first,unsigned second,unsigned line) {
    a.at(pc);a.move(3,table);a.offset(6,1,first);a.offset(7,1,second);a.value(4,0);a.value(5,0);
    a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,buffer);a.offset(4,table,0x16);
    a.offset(6,1,first);a.offset(7,1,second);a.value(8,line);a.call(0x1077b720);
    a.move(4,buffer);a.offset(3,table,0x23);a.value(5,0);a.call(0x10769b60);
}
inline void fatal_allocation(Call& a,std::uint32_t pc,unsigned table,unsigned buffer,unsigned flag,unsigned first,unsigned second,unsigned line) {
    diagnostic(a,pc,table,buffer,first,second,line);a.value(0,0);a.value(3,0xffffffff);a.store<1>(flag,0,0);a.call(0x1076f7d0);
}
}
