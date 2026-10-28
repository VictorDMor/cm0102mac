#include "euro_champ_qual_detail.hpp"
namespace cm::aot::euro_champ_qual {
using namespace detail;
std::uint32_t notify(Cpu& c,std::uint32_t object,std::uint32_t club,std::uint32_t event,std::uint32_t phase,std::uint32_t output,std::uint32_t record) {
    Frame f(c,0x104fa980,0x50,{.first_gpr=26}); Program p{c};
    keep(c,{{26,c.toc(-0x7bbc)},{27,club},{28,output},{29,c.toc(-0x7a64)},{30,c.toc(-0x7b5c)},{31,c.toc(-0x6948)}});
    c.set(9,object); c.set(3,c.toc(-0x7b48)); const auto stage=std::int8_t(phase),kind=std::int8_t(event);
    const bool early=compare(c,stage,c.i8(object+0xff))<0;
    if(early) {
        if(compare(c,kind,7)==0) {
            p.call(0x10384500,0x104fa9f4,{c.u32(c.r(3)),club,c.u32(c.r(26))+c.u32(c.r(31))*0x6c,0,0,100});
            p.call(0x104f8ee0,0x104faa0c,{c.u32(c.u32(c.r(30))+c.u32(c.r(31))*4),club});
            if(compare_logical(c,output)!=0) c.put8(output,3);
            p.call(0x102bacc0,0x104faa38,{c.u32(c.r(29)),club,c.u32(c.r(26))+c.u32(c.r(31))*0x6c}); return ~0u;
        }
        if(compare(c,kind,1)==0 || compare(c,kind,-2)==0) return ~0u;
        return 0;
    }
    if(compare(c,kind,1)==0) {
        p.call(0x10384500,0x104faa90,{c.u32(c.r(3)),club,c.u32(c.r(26))+c.u32(c.r(31))*0x6c,0,0,40});
        p.call(0x104f8ee0,0x104faaa8,{c.u32(c.u32(c.r(30))+c.u32(c.r(31))*4),club});
        if(compare_logical(c,output)!=0) c.put8(output,3);
    } else if(compare(c,kind,2)==0) {
        p.call(0x103846e0,0x104faadc,{c.u32(c.r(3)),club,c.u32(object+4),signed_half(c.u16(record+0x32)),signed_half(c.u16(record+0x30)),40});
        p.call(0x102b71e0,0x104faae8,{c.u32(c.r(29)),c.u32(club+0x2c)});
    }
    return 0;
}
void describe_event(Cpu& c,std::uint32_t object,std::uint32_t club_id,std::uint32_t event,std::uint32_t,std::uint32_t phase,std::uint32_t record,std::uint32_t context,std::uint32_t,std::uint32_t narrow,std::uint32_t live,std::uint32_t buffer) {
    Frame f(c,0x104fab30,0x60,{.first_gpr=25}); Program p{c};
    keep(c,{{25,object},{26,club_id},{27,c.toc(-0x7bbc)},{28,c.toc(-0x6948)},{29,c.toc(-0x7be8)},{30,c.toc(-0x7bb8)},{31,buffer}});
    c.set(6,event); c.set(4,c.toc(slot::pool)); c.set(10,live); c.set(11,narrow);
    const auto pool=c.r(4); const auto code=std::int16_t(phase); bool qualification=compare(c,code,1000)>=0;
    if(qualification) qualification=compare(c,code,1008)<=0;
    const auto generic=[&](unsigned text,std::uint32_t back,bool retain_offset) {
        const auto club_offset=club_id*0x248;
        if(retain_offset) c.set(25,club_offset);
        const auto clubs=c.u32(c.r(29)),competitions=c.u32(c.r(27)); const auto comp_offset=c.u32(c.r(28))*0x6c;
        p.call(0x10727890,back,{c.r(30),pool+text,signed_byte(c.u8(clubs+club_offset+0x23b)),signed_byte(c.u8(clubs+club_offset+0x23b)),
            signed_byte(c.u8(competitions+comp_offset+0x67)),signed_byte(c.u8(competitions+comp_offset+0x67)),clubs+club_offset+0x221,competitions+comp_offset+0x4d});
    };
    const auto links=[&](std::uint32_t back,std::uint32_t club_back,std::uint32_t competition_back,bool own_definition) {
        p.call(0x1074b830,back,{buffer,c.r(30)});
        c.set(6,c.u32(c.r(29))+club_id*0x248);
        p.call(0x1074bab0,club_back,{buffer,c.u32(c.r(29))+club_id*0x248+0x221,2005});
        const auto name=own_definition?c.u32(object+4)+0x1a:c.u32(c.r(27))+c.u32(c.r(28))*0x6c+0x4d;
        // r6 retains the containing record while the link routine is called.
        c.set(6,own_definition?c.u32(object+4):name-0x4d);
        p.call(0x1074bab0,competition_back,{buffer,name,2000});
    };
    if(qualification) {
        if(compare(c,std::int8_t(event),7)==0) {
            if(compare(c,std::int8_t(live))!=0) p.call(0x104698c0,0x104fabac,{club_id,phase,c.u32(object+4),event,live,buffer});
            else { generic(0x27,0x104fac00,true); links(0x104fac0c,0x104fac24,0x104fac44,false); }
        } else if(compare(c,std::int8_t(event),-2)==0) p.call(0x104698c0,0x104fac68,{club_id,phase,c.u32(object+4),event,live,buffer});
        return;
    }
    if(compare(c,code,1020)==0 && compare(c,std::int8_t(event),7)==0) {
        if(compare(c,std::int8_t(live))!=0) {
            const auto competition=c.u32(object+4),club=c.u32(c.r(29))+club_id*0x248;
            const auto text=compare(c,c.u8(object+0xf4),1)==0?0x7fu:0x120u;
            p.call(0x10727890,text==0x7f?0x104facd8:0x104fad1c,{c.r(30),pool+text,signed_byte(c.u8(club+0x23b)),signed_byte(c.u8(club+0x23b)),
                signed_byte(c.u8(competition+0x6a)),signed_byte(c.u8(competition+0x6a)),club+0x221,competition+0x1a});
            links(0x104fad28,0x104fad44,0x104fad58,true);
        } else { generic(0x27,0x104fadac,true); links(0x104fadb8,0x104fadd0,0x104fadf0,false); }
        return;
    }
    if(compare_logical(c,record)==0 || compare(c,c.i16(record+0x30),164)!=0) return;
    if(compare(c,std::int8_t(live))!=0) p.call(0x10464d00,0x104fae28,{club_id,record,narrow,event,context,buffer});
    else {
        const auto text=compare(c,std::int8_t(event),1)==0?0x27u:0x1b9u;
        generic(text,text==0x27?0x104fae88:0x104faedc,false); links(0x104faee8,0x104faf04,0x104faf24,false);
    }
}
}
