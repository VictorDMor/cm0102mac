#include "oceania_nations_detail.hpp"
#include <array>
namespace cm::aot::oceania_nations {
using namespace detail;
namespace {
void round_fields(Cpu& c, std::uint32_t schedule, unsigned round) {
    const auto at=schedule+round*0x78;
    const unsigned type=round==0?130:round==1?150:140;
    c.put16(at+8,type); c.put16(at+10,round); c.put16(at+12,0); c.put16(at+14,round?3:0x303);
    c.put8(at+24,10); c.put16(at+26,round?2:4); c.put16(at+28,round?1:2); c.put16(at+30,round?0:4);
    c.put16(at+16,0); c.put16(at+32,0); c.put8(at+34,0); c.put8(at+35,1); c.put8(at+36,0);
    c.put32(at+0x6c,0); c.put32(at+0x70,0); c.put32(at+0x74,0);
}
}
std::uint32_t calendar(Cpu& c, std::uint32_t object, std::uint32_t stage,
                       std::uint32_t count, std::uint32_t kind, std::uint32_t flag) {
    Frame f(c,0x106e9cb0,0x100,{.first_gpr=26}); Program p{c};
    c.set({{27,object},{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(-0x4094)}});
    const auto year=c.i16(object+0x40); const bool october=year%4==2;
    c.set(3,std::uint32_t(std::int32_t(year))); c.compare(0,year%4,2);
    const auto phase=std::int8_t(stage); c.compare(0,phase,1);
    if (phase<1) {
        c.compare_logical(0,flag,0); if (flag) c.put8(flag,1);
        c.set(28,std::uint32_t(std::int32_t(phase))); c.put16(count,3); c.put16(kind,std::uint32_t(std::int32_t(phase))+1001);
        const auto schedule=p.call(0x10775130,october?0x106e9d28:0x106ea264,{0xe4}); c.set(26,schedule); c.compare(0,std::int32_t(schedule),0);
        if (!schedule) { assertion(c,f,october?0x106e9d44:0x106ea280,october?0xc0:0x80,october?0x58:0x48,october?0xa0:0x11f); return 0; }
        const std::array<unsigned,3> days=october?std::array<unsigned,3>{10,13,15}:std::array<unsigned,3>{19,21,23};
        const std::array<unsigned,3> weekdays=october?std::array<unsigned,3>{4,0,2}:std::array<unsigned,3>{0,2,4};
        const auto month=october?10u:5u;
        const std::array<unsigned,3> date_back=october?std::array<unsigned,3>{0x106e9db0,0x106e9dfc,0x106e9e48}:std::array<unsigned,3>{0x106ea2ec,0x106ea338,0x106ea384};
        const std::array<unsigned,3> clear_back=october?std::array<unsigned,3>{0x106e9dd0,0x106e9e1c,0x106e9e68}:std::array<unsigned,3>{0x106ea30c,0x106ea358,0x106ea3a4};
        for (unsigned round=0;round<3;++round) {
            c.put32(f.at(0x38),1);
            p.call(0x104a9810,date_back[round],{schedule,round,days[round],month,0,weekdays[round],1,signed_half(c.u16(object+0x40))});
            p.call(0x104a9890,clear_back[round],{schedule,round,1,~0u,~0u,~0u,0});
        }
        c.compare(0,phase,0);
        if (phase!=0) { c.compare(0,phase,-1); }
        if (phase==-1 || phase==0) {
            const unsigned offset=october?4:0;
            const std::array<unsigned,3> back=october?
                (phase==-1?std::array<unsigned,3>{0x106e9ea0,0x106e9ec0,0x106e9ee0}:std::array<unsigned,3>{0x106e9f04,0x106e9f24,0x106e9f44}):
                (phase==-1?std::array<unsigned,3>{0x106ea3dc,0x106ea3fc,0x106ea41c}:std::array<unsigned,3>{0x106ea440,0x106ea460,0x106ea480});
            for (unsigned round=0;round<3;++round)
                p.call(0x104a9890,back[round],{schedule,round,0,(offset+round*2)%6,phase==-1?1u:2u,3,
                    ((round==1)==(phase==-1))?0x40u:4u});
        }
        c.set(3,schedule); return schedule;
    }
    if (phase!=1) { c.set(3,0); return 0; }
    c.compare_logical(0,flag,0); if (flag) c.put8(flag,0);
    c.put16(count,3); c.put16(kind,0);
    const auto schedule=p.call(0x10775130,october?0x106e9f78:0x106ea4b4,{0x168}); c.set(28,schedule); c.compare(0,std::int32_t(schedule),0);
    if (!schedule) { assertion(c,f,october?0x106e9f94:0x106ea4d0,october?0xa0:0x60,october?0x50:0x40,october?0xcc:0x14b); return 0; }
    const unsigned month=october?10:5;
    const std::array<unsigned,3> start=october?std::array<unsigned,3>{16,18,18}:std::array<unsigned,3>{24,26,26};
    const std::array<unsigned,3> end=october?std::array<unsigned,3>{17,19,19}:std::array<unsigned,3>{25,28,28};
    const std::array<unsigned,3> weekday=october?std::array<unsigned,3>{3,5,5}:std::array<unsigned,3>{5,0,0};
    const std::array<unsigned,3> end_weekday=october?std::array<unsigned,3>{4,6,6}:std::array<unsigned,3>{6,2,2};
    const std::array<unsigned,3> start_back=october?std::array<unsigned,3>{0x106e9ff4,0x106ea0fc,0x106ea1a0}:std::array<unsigned,3>{0x106ea530,0x106ea638,0x106ea6dc};
    const std::array<unsigned,3> end_back=october?std::array<unsigned,3>{0x106ea020,0x106ea128,0x106ea1cc}:std::array<unsigned,3>{0x106ea55c,0x106ea664,0x106ea708};
    for (unsigned round=0;round<3;++round) {
        p.call(0x104854f0,start_back[round],{schedule,round,start[round],month,0,weekday[round],signed_half(c.u16(object+0x40))});
        c.put32(f.at(0x38),round==0?1:round==1?4:0x40);
        p.call(0x10485570,end_back[round],{schedule,round,end[round],month,0,end_weekday[round],round==1?2u:1u,signed_half(c.u16(object+0x40))});
        if (round==0) {
            const auto base=october?0x106ea040u:0x106ea57cu;
            p.call(0x10485600,base,{schedule,0,0,october?4u:6u,2,3,0x40});
            p.call(0x10485600,base+0x20,{schedule,0,1,october?4u:6u,1,3,4});
            p.call(0x10485600,base+0x40,{schedule,0,2,~0u,~0u,~0u,0});
            c.set({{12,0},{7,0x303},{11,10},{10,4},{9,2}});
        } else if (round==1) c.set({{11,1},{10,0},{8,3},{9,10}});
        else c.set({{7,2},{6,0},{5,3},{4,10}});
        round_fields(c,schedule,round);
    }
    c.set(3,schedule); return schedule;
}
}
