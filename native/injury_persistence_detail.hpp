#pragma once
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_persistence::detail {
using Call=SemanticGuestCall;
inline void report_failure(Call& a,std::uint32_t start,std::int32_t first,std::int32_t second,unsigned line) {
    a.at(start);a.offset(3,31,0x53);a.offset(6,1,first);a.offset(7,1,second);a.value(4,0);a.value(5,0);a.call(0x10739890);a.call(0x10768810);
    a.move(5,3);a.move(3,30);a.offset(4,31,0x5e);a.offset(6,1,first);a.offset(7,1,second);a.value(8,line);a.call(0x1077b720);
    a.move(4,30);a.offset(3,31,0x6b);a.value(5,0);a.call(0x10769b60);
}
inline void finish_load(Call& a) {a.at(0x10184e44);a.pop(768);a.multiple(25,true);a.finish();}
inline void fail_load(Call& a,std::uint32_t start,std::int32_t first,std::int32_t second,unsigned line,bool opened=true) {
    report_failure(a,start,first,second,line);a.value(0,0);
    if(opened){a.offset(3,1,0x3c);a.store<1>(29,0,0);a.call(0x10747900);a.value(3,0);}
    else {a.value(3,0);a.store<1>(29,0,0);}
    a.jump(0x10184e44);finish_load(a);
}
inline void finish_save(Call& a) {a.at(0x101853f0);a.pop(528);a.multiple(27,true);a.finish();}
inline void fail_save(Call& a,std::uint32_t start,std::int32_t first,std::int32_t second,unsigned line,bool close=true) {
    report_failure(a,start,first,second,line);a.value(0,0);
    if(close)a.offset(3,1,0x3c);
    a.store<1>(29,0,0);if(close)a.call(0x10747900);
    a.jump(0x101853f0);finish_save(a);
}
}
