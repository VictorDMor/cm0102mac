#pragma once
#include "awol_records.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::awol::message_detail {
using Call=SemanticGuestCall;

inline void copy_field_then_read(Call& a,unsigned message,unsigned destination,unsigned field,bool temporary=true) {
    // The temporary form copies the message argument before overwriting a
    // destination that may itself be the original message register.
    if(temporary){a.move(0,3);a.move(3,message);a.move(destination,0);}
    else {a.move(destination,3);a.move(3,message);}
    a.value(4,field);a.call(0x102cb570);
}
inline void format_name(Call& a,unsigned id,unsigned first,unsigned middle,unsigned last,std::int32_t buffer,unsigned style) {
    a.move(3,first);a.move(4,middle);a.move(5,last);a.move(9,id);a.offset(6,1,buffer);a.value(7,400);a.value(8,style);a.call(0x100cb590);
}
inline bool selected_club_matches(Call& a,std::uint32_t start,unsigned context,unsigned club) {
    a.at(start);a.load<2>(0,context,0x2c58,true);a.multiply(0,0,0x2c0);a.add(3,context,0);a.load(3,3,4);a.compare(3,0,false);bool valid=!a.equal();
    if(valid){a.load(3,3,0x24);a.compare(3,0,false);valid=!a.equal();}
    if(valid){a.load(0,3,0);a.subtract(0,club,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0);a.jump(start+0x40);}
    else {a.at(start+0x3c);a.value(0,0);}
    a.at(start+0x40);a.sign8(0,0,true);return !a.equal();
}
inline void format_player_line(Call& a,std::uint32_t start,unsigned output,unsigned text,std::int32_t token,std::int32_t name,std::uint32_t join) {
    a.at(start);a.move(3,output);a.offset(4,text,token);a.offset(5,1,name);a.call(0x10727890);a.jump(join);
}
inline bool add_staff_link(Call& a,std::uint32_t start,unsigned offset,unsigned id,unsigned table,unsigned first,unsigned middle,unsigned last,unsigned output,std::int32_t name) {
    a.at(start);a.multiply(offset,id,0x70);a.load(0,table,0);a.move(4,first);a.move(5,middle);a.move(6,last);a.value(7,0);a.add(3,0,offset);a.call(0x100cbde0);a.sign8(0,3,true);
    if(a.equal())return false;
    a.load(0,table,0);a.move(3,output);a.offset(4,1,name);a.value(5,0x7da);a.add(6,0,offset);a.call(0x1074bab0);return true;
}
inline void add_club_link(Call& a,std::uint32_t start,unsigned table,unsigned club,unsigned output,std::int32_t name=0x1ee) {
    a.at(start);a.multiply(0,club,0x248);a.load(4,table,0);a.move(3,output);a.value(5,0x7d5);a.add(6,4,0);a.offset(4,6,name);a.call(0x1074bab0);
}
}
