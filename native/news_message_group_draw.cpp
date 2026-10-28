#include "news_message_detail.hpp"

namespace cm::aot::news::detail {
std::uint32_t Message::group_draw() const {
    const auto full=compare(c,std::int8_t(detail))!=0;
    Program p{c};const auto id=value(0);const auto round=short_value(1);
    const auto team=[&](std::uint32_t index){return c.u32(c.r(28))+index*0x248;};
    const auto grammar=[&](std::uint32_t index){return extended(c.i8(team(index)+0x23b));};
    if(full) {
        const auto club_id=value(2);keep(c,{{14,id},{17,extended(round)},{18,club_id}});
        p.call(0x107156b0,0x102c28e0,{frame.at(0xf0),extended(short_value(3)),value(4)&0xffff});
        int count=byte(5);c.set(19,extended(count));
        if(compare(c,count,8)>0){assertion(c,frame,0x102c2904,0x1e0,0xe8,0x942);count=8;c.set(19,8);}
        unsigned copied=0;
        for(;;++copied) {
            c.set(13,copied);c.set(3,copied);if(compare(c,copied,count)>=0)break;
            if(compare(c,copied,10)>=0)break;
            const auto member=c.u32(item+0x20+copied*4);c.set(4,member);c.set(3,frame.at(0x200));c.put32(frame.at(0x200+copied*4),member);
        }
        const auto member=[&](unsigned n){return c.u32(frame.at(0x200+n*4));};
        const auto offset=[&](unsigned n){return member(n)*0x248;};
        // Each list length is a distinct localized phrase. The first three
        // clubs supply register varargs; remaining grammar and names spill.
        constexpr std::array<unsigned,7> phrases{0x1fab,0x1ff0,0x2059,0x20e4,0x2199,0x226c,0x2360};
        constexpr std::array<std::uint32_t,7> backs{0x102c2a34,0x102c2b28,0x102c2c6c,0x102c2e00,0x102c2fe4,0x102c3218,0x102c34a4};
        bool compose=false;
        for(int n=2;n<=8;++n)if(compare(c,count,n)==0){compose=true;break;}
        if(compose) {
            switch(count) {
            case 2:keep(c,{{15,c.u32(c.r(28))},{13,offset(1)}});break;
            case 3:break;
            case 4:keep(c,{{20,offset(0)+0x221},{15,offset(1)+0x221},{13,team(member(0))}});break;
            case 5:keep(c,{{20,c.u32(c.r(28))},{15,offset(4)+0x221},{13,team(member(0))},{21,offset(4)},{22,c.u32(c.r(28))}});break;
            case 6:keep(c,{{21,team(member(0))},{20,offset(0)},{15,offset(1)},{13,offset(1)},{22,offset(5)},{23,c.u8(team(member(5))+0x23b)},{24,c.u32(c.r(28))}});break;
            case 7:keep(c,{{13,offset(2)},{15,member(2)},{20,offset(2)},{21,offset(1)},{22,offset(1)},{23,c.u32(c.r(28))},{24,grammar(member(6))},{25,offset(6)},{26,offset(5)}});break;
            case 8:c.put32(frame.at(0x256c),member(1));keep(c,{{15,team(member(0))},{13,team(member(0))},{20,offset(6)+0x221},{21,member(2)},{22,offset(2)},{23,member(0)},{24,offset(0)},{25,member(7)},{26,offset(5)+0x221},{27,member(1)}});break;
            }
            std::vector<std::uint32_t> arguments;
            for(int n=0;n<count;++n){arguments.push_back(grammar(member(n)));arguments.push_back(grammar(member(n)));}
            for(int n=0;n<count;++n)arguments.push_back(team(member(n))+0x221);
            format_values(frame.at(0x139c),phrases[count-2],backs[count-2],arguments);
        }
        p.call(0x10469bb0,0x102c34b8,{frame.at(0x434),100,extended(round),0});
        const auto name=competition_name(id);
        format(scratch(),0x2474,0x102c354c,{grammar(club_id),grammar(club_id),100,extended(c.i8(name+0x67)),extended(c.i8(name+0x67)),team(club_id)+0x221,frame.at(0x434),name+0x4d,frame.at(0x139c)});
        assign(scratch(),0x102c3558);
        if(compare(c,c.u16(frame.at(0xf2)))!=0) {
            p.call(0x10716eb0,0x102c3578,{frame.at(0xf0),frame.at(0x3d0),100,255});
            format(scratch(),0x2548,0x102c358c,{100,frame.at(0x3d0)});append(scratch(),0x102c3598);
        }
        c.set(7,c.u32(c.r(28)));link(team(club_id)+0x221,0x7d5,team(club_id),0x102c35c0);
        c.set(7,c.u32(c.r(30)));link(competition_name(id)+0x4d,0x7d0,competition_name(id),0x102c35e8);
        const auto identity=((extended(round)<<11)&0x003ff800)|((id<<22)&0xffc00000);
        link(frame.at(0x434),0x7e5,identity,0x102c3600);
        for(unsigned n=0;;++n) {
            c.set(13,n);c.set(3,n);if(compare(c,n,count)>=0)break;
            c.set(7,c.u32(c.r(28)));link(team(member(n))+0x221,0x7d5,team(member(n)),0x102c3650);
        }
    } else {
        c.set(13,id);const auto normalized=p.call(0x10464010,0x102c367c,{extended(round)});
        constexpr std::array<int,6> stages{0x41c,10,20,0x82,0x449,0x44a};
        constexpr std::array<unsigned,6> phrases{0x2594,0x25d3,0x261c,0x2665,0x26af,0x26f6};
        constexpr std::array<std::uint32_t,6> backs{0x102c36c8,0x102c3718,0x102c3768,0x102c37b8,0x102c3808,0x102c3858};
        for(unsigned n=0;n<stages.size();++n)if(compare(c,normalized,stages[n])==0) {
            const auto name=competition_name(id);c.set(8,c.u32(c.r(30)));
            format(scratch(),phrases[n],backs[n],{extended(c.i8(name+0x67)),extended(c.i8(name+0x67)),name+0x4d});break;
        }
        assign(scratch(),0x102c3864);c.set(7,c.u32(c.r(30)));link(competition_name(id)+0x4d,0x7d0,competition_name(id),0x102c388c);
    }
    return 1;
}
}
