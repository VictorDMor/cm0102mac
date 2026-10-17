#pragma once
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_bootstrap::detail {
using Call=SemanticGuestCall;
inline void report_allocation_failure(Call& a,std::uint32_t start,unsigned strings,unsigned log,std::int32_t first,std::int32_t second,unsigned line) {
    a.at(start);a.offset(3,strings,0x53);a.offset(6,1,first);a.offset(7,1,second);a.value(4,0);a.value(5,0);a.call(0x10739890);a.call(0x10768810);
    a.move(5,3);a.move(3,log);a.offset(4,strings,0x5e);a.offset(6,1,first);a.offset(7,1,second);a.value(8,line);a.call(0x1077b720);
    a.move(4,log);a.offset(3,strings,0x6b);a.value(5,0);a.call(0x10769b60);
}
}
