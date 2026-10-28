#include "news_message_detail.hpp"

namespace cm::aot::news::detail {
std::uint32_t Message::fixture_list() const {
    const auto id=value(0),season=value(2)&0xffff;c.set(13,id);c.set(14,season);c.set(4,value(2));
    Program p{c};const auto division=p.call(0x10192130,0x102c0fd8,{competition_name(id)});
    const auto division_row=c.toc(-0x7b44)+extended(std::int8_t(division))*0x4c;
    c.set(3,division_row);
    const auto special=compare(c,c.i8(division_row+0x18))!=0;
    const auto name=competition_name(id);
    if(special) {
        // The signed divide-by-100 sequence is exact on the 1..65536 range.
        const auto number=season+1,century=number/100,remainder=number-century*100;
        c.set(10,century*100);c.s.wr<1>(reg::xer_ca,false);
        format(frame.at(0x324),0x12a7,0x102c1064,{extended(c.i8(name+0x67)),extended(c.i8(name+0x67)),season,remainder,name+0x4d});
    } else {
        c.set(9,c.u32(c.r(30)));
        format(frame.at(0x324),0x12f5,0x102c10b0,{extended(c.i8(name+0x67)),extended(c.i8(name+0x67)),season,name+0x4d});
    }
    const auto full=compare(c,std::int8_t(detail))!=0;
    const auto team=[&](std::uint32_t index){return c.u32(c.r(28))+index*0x248;};
    const auto grammar=[&](std::uint32_t index){return extended(c.i8(team(index)+0x23b));};
    if(full) {
        p.call(0x10715880,0x102c10c0,{frame.at(0x100)});const auto club_id=value(1);c.set(14,club_id);
        for(unsigned n=0;n<3;++n) {
            const unsigned cursor=3+n*4;const auto year=value(cursor),day=value(cursor+1),opponent=value(cursor+2),home=value(cursor+3)&0xffff;
            c.set(19,year);c.set(18,n*2);c.put16(frame.at(0x110+n*2),year);c.put16(frame.at(0x108+n*2),day);
            c.put32(frame.at(0x194+n*4),opponent);c.put8(frame.at(0x84+n),home);
        }
        for(unsigned n=0;n<3;++n) {
            c.set(17,n);c.set(3,frame.at(0x84));const auto home=compare(c,c.i8(frame.at(0x84+n)))!=0;
            const auto opponent=c.u32(frame.at(0x194+n*4));
            leave(c,{{8,c.u32(c.r(28))},{9,c.u32(c.r(28))},{10,n}});
            format(frame.at(0xefc+n*0x3d),home?0x1331:0x135a,home?0x102c121c:0x102c12a0,{grammar(opponent),grammar(opponent),team(opponent)+0x221});
            const auto year=c.u16(frame.at(0x110+n*2)),day=c.u16(frame.at(0x108+n*2));c.set(6,year);
            p.call(0x107156b0,0x102c12cc,{frame.at(0x88),extended(std::int16_t(year)),day});
            c.put16(frame.at(0x102),c.u16(frame.at(0x8a)));c.put16(frame.at(0x100),c.u16(frame.at(0x88)));c.put8(frame.at(0x104),c.u8(frame.at(0x8c)));
            p.call(0x10716eb0,0x102c1304,{frame.at(0x100),frame.at(0xe64+n*50),50,255});
        }
        c.set(17,3);compare(c,3,3);c.set(15,team(club_id));
        format(scratch(),0x1383,0x102c13ec,{grammar(club_id),grammar(club_id),grammar(c.u32(frame.at(0x194))),grammar(c.u32(frame.at(0x198))),grammar(c.u32(frame.at(0x19c))),100,
            100,100,team(club_id)+0x221,frame.at(0xefc),frame.at(0xf39),frame.at(0xf76),frame.at(0xe64),frame.at(0xe96),frame.at(0xec8)});
        assign(scratch(),0x102c13f8);c.set(7,c.u32(c.r(30)));link(competition_name(id)+0x4d,0x7d0,competition_name(id),0x102c1420);
        c.set(7,c.u32(c.r(28)));link(team(club_id)+0x221,0x7d5,team(club_id),0x102c1448);
        for(unsigned n=0;n<3;++n){const auto opponent=c.u32(frame.at(0x194+n*4));c.set(7,c.u32(c.r(28)));link(team(opponent)+0x221,0x7d5,team(opponent),0x102c1478+n*0x30);}
    } else {
        format(scratch(),0x1467,0x102c1514,{extended(c.i8(name+0x67)),extended(c.i8(name+0x67)),frame.at(0x324)});
        assign(scratch(),0x102c1520);c.set(7,c.u32(c.r(30)));link(competition_name(id)+0x4d,0x7d0,competition_name(id),0x102c1548);
    }
    return 1;
}
}
