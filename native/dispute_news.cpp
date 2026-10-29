#include "dispute_detail.hpp"

namespace cm::aot::dispute {
using namespace detail;
void publish(Cpu& c,std::uint32_t object,std::uint32_t person,std::uint32_t club,
             std::uint32_t phase,std::uint32_t reason,std::uint32_t temporary) {
    Frame frame(c,0x1070d650,0x140,{.first_gpr=26}); Program p{c};
    keep(c,{{26,club},{27,phase},{28,reason},{29,temporary},{30,c.toc(-0x7bf0)},{31,person}});
    if (compare(c,person)==0) return;
    if (compare_logical(c,c.u32(person+0x24))==0) return;
    if (compare_logical(c,c.u32(person+0x14))==0) return;
    if (compare_logical(c,club)==0) return;
    (void)object;
    const auto item=frame.at(0x38);
    p.call(0x102bdf90,0x1070d6ac,{item,3049,0});
    const auto set=[&](unsigned index,unsigned value,unsigned back) { p.call(0x102cb4b0,back,{item,index,value}); };
    set(0,c.u32(person),0x1070d6bc);
    set(1,c.u32(c.u32(person+0x28)+0x20),0x1070d6d0);
    set(2,c.u32(c.u32(person+0x2c)+0x20),0x1070d6e4);
    set(3,c.u32(c.u32(person+0x30)+0x20),0x1070d6f8);
    set(4,c.u32(club),0x1070d708);
    c.set(3,c.u32(person+0x24));
    if (compare_logical(c,c.r(3))!=0) set(5,c.u32(c.r(3)),0x1070d724);
    else set(5,0xffffffff,0x1070d738);
    set(6,std::uint32_t(std::int32_t(std::int8_t(reason))),0x1070d748);
    set(7,std::uint32_t(std::int32_t(std::int8_t(temporary))),0x1070d758);
    c.set(3,c.u32(person+0x24));
    if (compare_logical(c,c.r(3))!=0) {
        c.set(3,c.u32(c.r(3)+0x2c));
        if (compare_logical(c,c.r(3))!=0) set(8,c.u32(c.r(3)+0x68),0x1070d780);
        else set(8,0xffffffff,0x1070d794);
    } else set(8,0xffffffff,0x1070d794);
    set(9,std::uint32_t(std::int32_t(std::int8_t(phase))),0x1070d7a4);
    c.set(3,c.u32(person+0x14));
    if (compare(c,c.i16(c.r(3)+12),7500)>0) {
        p.call(0x102cc8e0,0x1070d7c0,{c.u32(c.r(30)),item}); return;
    }
    c.set(3,c.u32(person+0x24));
    if (compare_logical(c,c.r(3))!=0) {
        c.set(5,c.u32(c.r(3)+0x2c));
        if (compare_logical(c,c.r(5))!=0) p.call(0x102cc400,0x1070d7e8,{c.u32(c.r(30)),item});
    }
    c.set(3,c.u32(person+0x24)); c.set(5,c.u32(person+12));
    if (compare_logical(c,c.u32(c.r(3)+0x2c),c.r(5))!=0)
        p.call(0x102cc400,0x1070d808,{c.u32(c.r(30)),item});
}
void render(Cpu& c,std::uint32_t object,std::uint32_t item,std::uint32_t detailed,std::uint32_t output) {
    Frame frame(c,0x1070d820,0x200,{.first_gpr=19}); Program p{c};
    keep(c,{{20,item},{25,c.toc(-0x4004)},{27,output},{28,c.toc(-0x7b50)},
            {29,c.toc(-0x7bb8)},{30,c.toc(-0x7be0)},{31,c.toc(-0x7be8)}});
    (void)object;
    const auto detailed_view=compare(c,std::int8_t(detailed))!=0;
    const auto get=[&](unsigned field,unsigned back) { return p.call(0x102cb570,back,{item,field}); };
    if (detailed_view) {
        c.set(24,get(0,0x1070d860)); c.set(23,get(1,0x1070d874));
        c.set(22,get(2,0x1070d888)); c.set(21,get(3,0x1070d89c));
        c.set(26,get(4,0x1070d8b0)); get(5,0x1070d8c0);
        c.set(19,get(6,0x1070d8cc)); get(7,0x1070d8dc); get(8,0x1070d8e8);
        c.set(20,get(9,0x1070d8f4));
        c.set(9,c.r(24));
        p.call(0x100cb590,0x1070d918,{c.r(23),c.r(22),c.r(21),frame.at(0x38),400,1});
    } else {
        c.set(21,get(0,0x1070db64)); c.set(22,get(1,0x1070db78));
        c.set(23,get(2,0x1070db8c)); c.set(24,get(3,0x1070dba0));
        c.set(26,get(4,0x1070dbb4)); get(5,0x1070dbc4);
        get(6,0x1070dbd0); get(6,0x1070dbdc); // The original reads slot six twice.
        c.set(20,get(9,0x1070dbe8)); c.set(9,c.r(21));
        p.call(0x100cb590,0x1070dc0c,{c.r(22),c.r(23),c.r(24),frame.at(0x38),400,5});
    }
    const auto phase=std::int32_t(c.r(20));
    unsigned category;
    if (compare(c,phase,2)==0) category=3;
    else if (phase<2) {
        if (compare(c,phase)==0) category=0;
        else category=phase>=0?1:3;
    } else category=compare(c,phase,4)>=0?3:2;
    unsigned pool_offset=0,format_back=0;
    if (detailed_view) {
        if (category==0) {
            const auto reason=std::int32_t(c.r(19));
            if (compare(c,reason,3)==0) { pool_offset=0x2a5; format_back=0x1070d9f8; }
            else if (reason<3) {
                if (compare(c,reason,1)==0) { pool_offset=0x2b; format_back=0x1070d990; }
                else if (reason>=1) { pool_offset=0x163; format_back=0x1070d9c4; }
                else { pool_offset=0x3cd; format_back=0x1070da2c; }
            } else { pool_offset=0x3cd; format_back=0x1070da2c; }
        } else if (category==1) { pool_offset=0x50b; format_back=0x1070da64; }
        else if (category==2) { pool_offset=0x5ec; format_back=0x1070da9c; }
        else { pool_offset=0x6e4; format_back=0x1070dad4; }
    } else {
        constexpr unsigned offsets[]={0x7c1,0x802,0x85d,0x8c0};
        constexpr unsigned backs[]={0x1070dc44,0x1070dc78,0x1070dcac,0x1070dce0};
        pool_offset=offsets[category]; format_back=backs[category];
    }
    if (!detailed_view && category==0) p.call(0x10727890,format_back,{c.r(29),c.r(25)+pool_offset,frame.at(0x38)});
    else {
        const auto club_offset=c.r(26)*0x248;
        const auto club_base=c.u32(c.r(31));
        const auto gender=std::uint32_t(std::int32_t(c.i8(club_base+club_offset+0x23b)));
        if (detailed_view && category==2) {
            c.set(9,club_base+club_offset+0x221);
            p.call(0x10727890,format_back,{c.r(29),c.r(25)+pool_offset,100,gender,gender,frame.at(0x38)});
        } else {
            const auto extra=detailed_view && category!=0;
            if (extra) c.set(9,frame.at(0x38));
            p.call(0x10727890,format_back,{c.r(29),c.r(25)+pool_offset,gender,gender,
                       extra?100:frame.at(0x38),club_base+club_offset+0x221});
        }
    }
    p.call(0x1074b830,detailed_view?0x1070dae0:0x1070dcec,{output,c.r(29)});
    const auto person_index=c.r(detailed_view?24:21);
    c.set(19,person_index*0x70);
    p.call(0x100cbde0,detailed_view?0x1070db00:0x1070dd0c,
           {c.u32(c.r(30))+c.r(19),c.r(detailed_view?23:22),c.r(detailed_view?22:23),c.r(detailed_view?21:24),0});
    if (compare(c,std::int8_t(c.r(3)))!=0)
        p.call(0x1074bab0,detailed_view?0x1070db20:0x1070dd2c,{output,frame.at(0x38),2010,c.u32(c.r(30))+c.r(19)});
    if (compare(c,c.r(26),-1)>0) {
        c.set(3,c.u32(c.r(28)));
        if (compare(c,c.r(26),c.r(3)-1)<0) {
            const auto club=c.u32(c.r(31))+c.r(26)*0x248;
            p.call(0x1074bab0,detailed_view?0x1070db54:0x1070dd60,{output,club+0x1ee,2005,club});
        }
    }
}
}
