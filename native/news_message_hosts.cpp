#include "news_message_detail.hpp"

namespace cm::aot::news::detail {
std::uint32_t Message::host_selected() const {
    const auto full=compare(c,std::int8_t(detail))!=0;
    const auto competition_id=value(0),year=value(1)&0xffff,first=value(2),second=value(3);
    keep(c,{{13,competition_id},{14,first},{15,second}});c.set(8,year);
    const auto nation=[&](std::uint32_t id){return c.u32(c.r(27))+id*0x128;};
    const auto nation_link=[&](std::uint32_t id,std::uint32_t getter_back,std::uint32_t link_back){
        const auto club=Program{c}.call(0x100c76c0,getter_back,{nation(id)});link(nation(id)+0xae,0x7d5,club,link_back);
    };
    if(full) {
        if(compare(c,first,-1)>0&&compare(c,second,-1)>0) {
            const auto a=nation(first),b=nation(second),competition=competition_name(competition_id);
            format(scratch(),0x622,0x102bf470,{std::uint32_t(std::int32_t(c.i8(a+0x115))),std::uint32_t(std::int32_t(c.i8(a+0x115))),
                std::uint32_t(std::int32_t(c.i8(b+0x115))),std::uint32_t(std::int32_t(c.i8(b+0x115))),
                std::uint32_t(std::int32_t(c.i8(competition+0x6a))),std::uint32_t(std::int32_t(c.i8(competition+0x6a))),
                a+0xae,b+0xae,year,competition+0x1a});
            assign(scratch(),0x102bf47c);nation_link(first,0x102bf48c,0x102bf4ac);nation_link(second,0x102bf4bc,0x102bf4dc);
        } else if(compare(c,first,-1)>0) {
            const auto team=nation(first),competition=competition_name(competition_id);
            format(scratch(),0x6a1,0x102bf56c,{std::uint32_t(std::int32_t(c.i8(team+0x115))),std::uint32_t(std::int32_t(c.i8(team+0x115))),
                std::uint32_t(std::int32_t(c.i8(competition+0x6a))),std::uint32_t(std::int32_t(c.i8(competition+0x6a))),
                team+0xae,year,competition+0x1a});
            assign(scratch(),0x102bf578);nation_link(first,0x102bf588,0x102bf5a8);
        }
        c.set(7,c.u32(c.r(30)));link(competition_name(competition_id)+0x1a,0x7d0,competition_name(competition_id),0x102bf5d0);
    } else {
        const auto competition=competition_name(competition_id);c.set(9,c.u32(c.r(30)));
        format(scratch(),0x6fe,0x102bf61c,{std::uint32_t(std::int32_t(c.i8(competition+0x67))),
            std::uint32_t(std::int32_t(c.i8(competition+0x67))),year,competition+0x4d});
        assign(scratch(),0x102bf628);c.set(7,c.u32(c.r(30)));
        link(competition_name(competition_id)+0x4d,0x7d0,competition_name(competition_id),0x102bf650);
    }
    return 1;
}
}

namespace cm::aot::news::detail {
std::uint32_t Message::hosting_bids() const {
    const auto full=compare(c,std::int8_t(detail))!=0;
    const auto id=value(0),year=value(1)&0xffff;
    leave(c,{{3,value(6)},{4,value(5)},{5,value(4)},{6,value(3)},{7,value(2)},{8,value(1)}});
    keep(c,{{13,id},{14,year}});
    for(unsigned n=0;n<6;++n)c.put32(frame.at(0x240+n*4),value(n+2));
    const auto nation=[&](std::uint32_t index){return c.u32(c.r(27))+index*0x128;};
    const auto nation_id=[&](unsigned n){return c.u32(frame.at(0x240+n*4));};
    const auto grammar=[&](unsigned n){return extended(c.i8(nation(nation_id(n))+0x115));};
    Program p{c};
    if(full) {
        p.call(0x10778300,0x102beebc,{frame.at(0x230c),0,600});
        for(unsigned bid=0;;++bid) {
            c.set(15,bid);if(compare(c,bid,3)>=0)break;
            if(compare(c,bid)==0) {
                c.set(3,frame.at(0x240));
                if(compare(c,nation_id(0),-1)>0) {
                    c.set(3,frame.at(0x244));
                    if(compare(c,nation_id(1),-1)>0) {
                        const auto first=p.call(0x100c76c0,0x102bef10,{nation(nation_id(0))});
                        c.set(7,c.u32(c.r(27)));link(nation(nation_id(0))+0xae,0x7d5,first,0x102bef3c);
                        const auto second=p.call(0x100c76c0,0x102bef58,{nation(nation_id(1))});
                        c.set(7,c.u32(c.r(27)));link(nation(nation_id(1))+0xae,0x7d5,second,0x102bef84);continue;
                    }
                }
            }
            c.set(3,frame.at(0x240));
            if(compare(c,nation_id(bid*2),-1)>0) {
                const auto team=p.call(0x100c76c0,0x102befb8,{nation(nation_id(bid*2))});
                c.set(7,c.u32(c.r(27)));link(nation(nation_id(bid*2))+0xae,0x7d5,team,0x102befe4);
            }
        }
        const auto competition=competition_name(id);
        if(compare(c,nation_id(1),-1)==0) {
            keep(c,{{15,nation_id(4)*0x128+0xae},{17,nation_id(2)*0x128+0xae},{14,nation(nation_id(0))}});
            format(scratch(),0x48b,0x102bf130,{grammar(0),grammar(0),grammar(2),grammar(2),grammar(4),grammar(4),
                extended(c.i8(competition+0x6a)),extended(c.i8(competition+0x6a)),nation(nation_id(0))+0xae,nation(nation_id(2))+0xae,nation(nation_id(4))+0xae,year,competition+0x1a});
        } else {
            keep(c,{{19,nation_id(0)*0x128+0xae},{18,nation_id(1)*0x128+0xae},{17,nation_id(2)*0x128+0xae},
                {15,nation_id(4)*0x128+0xae},{14,nation(nation_id(0))}});
            format(scratch(),0x524,0x102bf2b4,{grammar(0),grammar(0),grammar(1),grammar(1),grammar(2),grammar(2),grammar(4),grammar(4),
                extended(c.i8(competition+0x6a)),extended(c.i8(competition+0x6a)),nation(nation_id(0))+0xae,nation(nation_id(1))+0xae,nation(nation_id(2))+0xae,nation(nation_id(4))+0xae,year,competition+0x1a});
        }
        assign(scratch(),0x102bf2c0);c.set(7,c.u32(c.r(30)));link(competition_name(id)+0x1a,0x7d0,competition_name(id),0x102bf2e8);
    } else {
        const auto competition=competition_name(id);c.set(9,c.u32(c.r(30)));
        format(scratch(),0x5e6,0x102bf334,{extended(c.i8(competition+0x67)),extended(c.i8(competition+0x67)),year,competition+0x4d});
        assign(scratch(),0x102bf340);c.set(7,c.u32(c.r(30)));link(competition_name(id)+0x4d,0x7d0,competition_name(id),0x102bf368);
    }
    return 1;
}
}
