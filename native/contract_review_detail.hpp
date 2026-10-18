#pragma once
#include "contract_review.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::contract_review::detail {
using Call = SemanticGuestCall;
// The three complete diagnostics share these eighteen original instructions
// after table/buffer register and diagnostic-line normalization.
inline void report_diagnostic(Call& a,std::uint32_t pc,unsigned table,unsigned buffer,unsigned line) {
    a.at(pc);a.move(3,table);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0);a.value(5,0);
    a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,buffer);a.offset(4,table,0x14);
    a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(8,line);a.call(0x1077b720);
    a.move(4,buffer);a.offset(3,table,0x21);a.value(5,0);a.call(0x10769b60);
}
}
