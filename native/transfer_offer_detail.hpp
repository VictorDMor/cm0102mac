#pragma once
#include "transfer_offer_queries.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::transfer_offer_queries::detail {
using Call=SemanticGuestCall;
inline void diagnostic(Call& a,std::uint32_t pc,unsigned table,unsigned buffer,unsigned first,unsigned second,unsigned line) {
    a.at(pc);a.offset(3,table,0x37);a.offset(6,1,first);a.offset(7,1,second);a.value(4,0);a.value(5,0);
    a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,buffer);a.offset(4,table,0x6ff);
    a.offset(6,1,first);a.offset(7,1,second);a.value(8,line);a.call(0x1077b720);
    a.move(4,buffer);a.offset(3,table,0x70c);a.value(5,0);a.call(0x10769b60);
}
}
