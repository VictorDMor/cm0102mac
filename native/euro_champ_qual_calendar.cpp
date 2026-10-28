#include "euro_champ_qual_detail.hpp"
#include <array>
namespace cm::aot::euro_champ_qual {
using namespace detail;
std::uint32_t calendar(Cpu& c,std::uint32_t address,std::uint32_t phase,std::uint32_t count,std::uint32_t kind,std::uint32_t flag) {
    Frame f(c,0x104fa3d0,0xd0,{.first_gpr=27}); Program p{c};
    Kept<28> object(c,address); keep(c,{{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(slot::pool)}});
    const auto stage=std::int8_t(phase); c.set(3,signed_byte(phase));
    if(compare(c,stage,c.i8(address+0xfd))<=0) {
        if(compare_logical(c,flag)!=0) c.put8(flag,1);
        c.put16(count,10); c.put16(kind,std::uint32_t(stage)+1001);
        Kept<27> schedule(c,p.call(0x10775130,0x104fa430,{0x2f8}));
        if(compare(c,schedule)==0) { assertion(c,f,0x104fa44c,0x98,0x50,0xbb); return 0; }
        constexpr std::array days{2u,6u,7u,11u,24u,28u,6u,2u,6u,7u};
        constexpr std::array months{8u,8u,9u,9u,2u,2u,5u,8u,8u,9u};
        for(unsigned round=0;round<days.size();++round) {
            c.put32(f.at(0x38),0);
            p.call(0x104a9810,0x104fa4b8+round*0x4c,{schedule,round,days[round],months[round],round<4?0u:1u,
                round==0||round==2||round==4||round==7||round==9?5u:2u,round==0||round==2||round==4||round==7||round==9?1u:2u,signed_half(c.u16(address+0x40))});
            p.call(0x104a9890,0x104fa4d8+round*0x4c,{schedule,round,0,~0u,~0u,~0u,0});
        }
        return schedule;
    }
    if(compare(c,stage,c.i8(address+0xfe))==0) return 0;
    if(compare(c,stage,c.i8(address+0xff))!=0) return 0;
    if(compare_logical(c,flag)!=0) c.put8(flag,0);
    c.put16(count,1); c.put16(kind,0);
    Kept<27> schedule(c,p.call(0x10775130,0x104fa7dc,{0x78}));
    if(compare(c,schedule)==0) { assertion(c,f,0x104fa7f8,0x78,0x48,0xf5); return 0; }
    p.call(0x104854f0,0x104fa858,{schedule,0,11,9,1,2,signed_half(c.u16(address+0x40))});
    c.put32(f.at(0x38),0);
    p.call(0x10485570,0x104fa884,{schedule,0,11,10,1,5,1,signed_half(c.u16(address+0x40))});
    const auto at=std::uint32_t(schedule);
    c.put16(at+8,160);
    for(unsigned offset:{10,12,14,32}) c.put16(at+offset,0);
    c.put8(at+24,10); c.put16(at+16,3); c.put8(at+34,0); c.put8(at+35,2); c.put8(at+36,4);
    for(unsigned offset:{0x6c,0x70,0x74}) c.put32(at+offset,0);
    leave(c,{{3,4},{4,3},{5,10},{6,0}});
    const auto hosts=c.i8(address+0xf4);
    if(compare(c,hosts,1)==0) { c.put16(at+26,6); c.put16(at+28,3); c.put16(at+30,6); }
    else if(compare(c,hosts,2)==0) { c.put16(at+26,8); c.put16(at+28,4); c.put16(at+30,8); }
    else assertion(c,f,0x104fa924,0x58,0x40,0x11a);
    return schedule;
}
}
