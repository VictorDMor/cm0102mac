#include "euro_champ_qual_detail.hpp"
namespace cm::aot::euro_champ_qual {
using namespace detail;
void insert_fixture(Cpu& c,std::uint32_t object,std::uint32_t phase,std::uint32_t day,std::uint32_t month,std::uint32_t year,std::uint32_t home,std::uint32_t home_score,std::uint32_t away,std::uint32_t away_score) {
    Frame f(c,0x105022a0,0x110,{.first_gpr=25}); Program p{c};
    keep(c,{{25,object},{26,home},{27,away},{28,c.toc(-0x7b98)},{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(slot::pool)}});
    const auto groups=c.u32(object+0xc); const auto stage=std::int8_t(phase); bool valid=compare_logical(c,groups)!=0;
    if(valid) {
        c.set(3,c.u32(object+0x30)); c.set(5,signed_byte(phase));
        valid=compare(c,stage,c.r(3))<=0;
        if(valid && compare(c,stage,-1)!=0) { c.set(3,c.u32(groups+std::uint32_t(stage)*4)); valid=compare_logical(c,c.r(3))!=0; }
    }
    if(!valid) { assertion(c,f,0x1050231c,0x78,0x50,0x935); return; }
    c.set(3,year&0xffff);
    if(compare_logical(c,c.r(3),1998)<0 || compare_logical(c,c.r(3),1999)>0 ||
       (c.set(10,signed_byte(phase)),compare(c,stage,-1)<0) || compare(c,stage,7)>0) {
        assertion(c,f,0x10502398,0x58,0x48,0x93c); return;
    }
    compare(c,stage,-1); c.set(5,c.u8(f.at(0xe6)));
    c.put8(f.at(0xe6),(c.r(5)&15)|((c.u8(object+0x46)<<4)&0xf0));
    c.put8(f.at(0xe6),(c.u8(f.at(0xe6))&0xf0)|(c.u8(object+0x47)&15));
    const auto definition=c.u32(object+4); c.put32(f.at(0x98),c.u32(definition)); c.put32(f.at(0xac),definition);
    c.put16(f.at(0xc4),c.u16(object+0x40)); c.put16(f.at(0xc8),0);
    const auto competition=stage==-1?object:c.u32(groups+std::uint32_t(stage)*4);
    c.put16(f.at(0xca),c.u16(competition+0x3a));
    c.set(3,signed_half(c.u16(competition+0x3a))); c.set(5,stage==-1?signed_half(c.u16(object+0x40)):groups);
    c.put8(f.at(0xd2),c.u8(object+0xc6)); c.set(29,~0u); c.set(30,signed_half(c.u16(object+0xde)));
    c.put16(f.at(0xce),c.r(30)); c.set(30,signed_half(c.u16(object+0xa8))); c.put16(f.at(0xd0),c.r(30));
    c.put8(f.at(0xda),phase); c.put8(f.at(0xdb),home_score); c.put8(f.at(0xdc),away_score);
    for(unsigned off=0xdd;off<=0xe4;++off) c.put8(f.at(off),0xff);
    c.put32(f.at(0xa0),~0u); c.put8(f.at(0xd5),0); c.put8(f.at(0xd6),0); c.put8(f.at(0xd3),0xff);
    c.put8(f.at(0xd8),1); c.put8(f.at(0xd9),1);
    const auto flags=std::uint32_t(c.u8(f.at(0xe7))&15); c.put8(f.at(0xe7),flags);
    p.call(0x107156f0,0x105024e4,{f.at(0x40),day,month,year,~0u,flags,home_score,1});
    c.set(4,signed_half(c.u16(f.at(0x40)))); c.put16(f.at(0xc2),c.r(4)); c.put16(f.at(0xc0),c.u16(f.at(0x42)));
    p.call(0x10715b20,0x105024fc,{f.at(0x40)});
    bool weekend=compare(c,std::int16_t(c.r(3)),5)==0;
    if(!weekend) { p.call(0x10715b20,0x10502510,{f.at(0x40)}); weekend=compare(c,std::int16_t(c.r(3)),6)==0; }
    c.put8(f.at(0xd7),weekend?1:2);
    c.set(6,c.u16(f.at(0xc2))); c.set(5,c.u16(f.at(0xc0))); c.set(4,0);
    c.put16(f.at(0xbe),c.r(6)); c.put16(f.at(0xbc),c.r(5)); c.put16(f.at(0xcc),0);
    c.put32(f.at(0xb4),p.call(0x100c76c0,0x10502558,{c.u32(c.r(28))+home*0x128}));
    c.set(4,c.u32(c.r(28))); c.put32(f.at(0xb8),p.call(0x100c76c0,0x1050256c,{c.r(4)+away*0x128}));
    c.set(8,c.u32(f.at(0xb4))); c.set(7,c.toc(-0x742c));
    c.put32(f.at(0xa4),c.u32(c.r(8))); c.put32(f.at(0xa8),c.u32(c.r(3)));
    p.call(0x104aff20,0x1050259c,{c.u32(c.r(7)),0,f.at(0x98),~0u});
    c.set(4,c.toc(-0x7a9c)); c.set(5,c.u16(f.at(0x42)));
    c.put32(f.at(0x38),c.u32(c.r(4))); c.put32(f.at(0x3c),c.u32(c.r(4)+4));
    c.set(3,c.u16(f.at(0x3a))); const auto cmp=compare_logical(c,c.r(3),c.r(5));
    bool future=cmp<0;
    if(cmp==0) { c.set(4,signed_half(c.u16(f.at(0x40)))); future=c.i16(f.at(0x40))>c.i16(f.at(0x38)); }
    compare(c,future?1:0);
    if(future) { c.put8(f.at(0xdb),0xff); c.put8(f.at(0xdc),0xff); }
    p.call(0x104888f0,0x1050261c,{c.u32(c.toc(-0x7a60)),f.at(0x98),1});
    if(compare(c,c.i8(f.at(0xdb)),-1)!=0) {
        leave(c,{{3,object},{4,f.at(0x98)},{5,0}}); p.virtual_call(object,0xc,0x10502644);
        const auto actual_stage=c.i8(f.at(0xda));
        if(compare(c,actual_stage,-1)==0) p.call(0x1049eb30,0x10502664,{object,0});
        else p.call(0x1049eb30,0x1050267c,{c.u32(c.u32(object+0xc)+std::uint32_t(actual_stage)*4),0});
    }
    enter(c,0x10502684,{object}); refresh_dates(c,object);
}
}
