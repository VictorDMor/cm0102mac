#include "news_message_detail.hpp"

namespace cm::aot::news::detail {
std::uint32_t Message::tournament_reply() const {
    Program p{c};p.call(0x10715880,0x102c08b0,{frame.at(0x128)});
    const auto first=value(0),second=value(1),year=value(2)&0xffff,day=value(3)&0xffff;
    const auto status=byte(4),tournament_number=byte(5);
    keep(c,{{13,first},{14,second},{15,extended(status)},{18,extended(tournament_number)}});c.set(6,year);
    p.call(0x107156b0,0x102c08e4,{frame.at(0x98),extended(std::int16_t(year)),day});
    const auto full=compare(c,std::int8_t(detail))!=0;
    c.put16(frame.at(0x12a),c.u16(frame.at(0x9a)));c.put16(frame.at(0x128),c.u16(frame.at(0x98)));c.put8(frame.at(0x12c),c.u8(frame.at(0x9c)));
    const auto team=[&](std::uint32_t id){return c.u32(c.r(28))+id*0x248;};
    const auto grammar=[&](std::uint32_t id,unsigned at){return extended(c.i8(team(id)+at));};
    if(full) {
        const auto tournament=p.call(0x104976b0,0x102c0918,{c.u32(c.toc(-0x7b54)),extended(tournament_number),frame.at(0x128)});c.set(17,tournament);
        p.call(0x10717620,0x102c0934,{frame.at(0x128),frame.at(0x5b0),100,255});
        const bool found=compare_logical(c,tournament)!=0;
        const bool accepted=compare(c,status,3)==0;
        if(accepted||compare(c,status,4)==0) {
            if(found)format(scratch(),accepted?0xe9b:0xf58,accepted?0x102c09dc:0x102c0a80,
                {grammar(second,0x242),grammar(second,0x242),grammar(first,0x242),grammar(first,0x242),100,team(second)+0x1ee,team(first)+0x1ee,tournament+0x2e,frame.at(0x5b0)});
            else format(scratch(),accepted?0x1015:0x10b8,accepted?0x102c0b8c:0x102c0c28,
                {grammar(second,0x242),grammar(second,0x242),grammar(first,0x242),grammar(first,0x242),100,team(second)+0x1ee,team(first)+0x1ee,frame.at(0x5b0)});
        }
        assign(scratch(),found?0x102c0a8c:0x102c0c34);
        c.set(7,c.u32(c.r(28)));link(team(second)+0x1ee,0x7d5,team(second),found?0x102c0ab4:0x102c0c5c);
        c.set(7,c.u32(c.r(28)));link(team(first)+0x1ee,0x7d5,team(first),found?0x102c0adc:0x102c0c84);
        if(found)link(tournament+0x2e,0x7d1,tournament,0x102c0af0);
    } else {
        const bool accepted=compare(c,status,3)==0;
        if(accepted||compare(c,status,4)==0){c.set(8,c.u32(c.r(28)));format(scratch(),accepted?0x115b:0x11a1,accepted?0x102c0cd8:0x102c0d2c,
            {grammar(second,0x23b),grammar(second,0x23b),team(second)+0x221});}
        assign(scratch(),0x102c0d38);c.set(7,c.u32(c.r(28)));link(team(second)+0x221,0x7d5,team(second),0x102c0d60);
    }
    return 1;
}

std::uint32_t Message::tournament_invitation() const {
    Program p{c};p.call(0x10715880,0x102c039c,{frame.at(0x130)});
    const auto full=compare(c,std::int8_t(detail))!=0;
    const auto first=value(0),second=value(1),year=value(5)&0xffff,day=value(6)&0xffff;
    const auto tournament_number=byte(2),status=byte(3);
    keep(c,{{13,extended(tournament_number)},{17,extended(status)},{14,first},{15,second}});leave(c,{{4,year},{5,day}});
    const auto team=[&](std::uint32_t id){return c.u32(c.r(28))+id*0x248;};
    const auto grammar=[&](std::uint32_t id){return extended(c.i8(team(id)+0x23b));};
    if(full) {
        p.call(0x107156b0,0x102c03d8,{frame.at(0xa0),extended(std::int16_t(year)),day});
        c.put16(frame.at(0x132),c.u16(frame.at(0xa2)));c.put16(frame.at(0x130),c.u16(frame.at(0xa0)));c.put8(frame.at(0x134),c.u8(frame.at(0xa4)));
        const auto tournament=p.call(0x104976b0,0x102c0404,{c.u32(c.toc(-0x7b54)),extended(tournament_number),frame.at(0x130)});c.set(13,tournament);
        const bool found=compare(c,tournament)!=0;
        p.call(0x10716eb0,found?0x102c0420:0x102c065c,{found?tournament+4:frame.at(0x130),frame.at(0x614),100,255});
        if(found)format(frame.at(0x1b3c),0xcbd,0x102c04b4,{grammar(first),grammar(first),grammar(second),grammar(second),100,team(first)+0x221,team(second)+0x221,tournament+0x2e,frame.at(0x614)});
        else format(frame.at(0x1b3c),0xdd1,0x102c06e8,{grammar(first),grammar(first),grammar(second),grammar(second),100,team(first)+0x221,team(second)+0x221,frame.at(0x614)});
        const bool accepted=compare(c,status,3)==0;
        if(accepted||compare(c,status,4)==0) {
            c.set(8,c.u32(c.r(28)));format(frame.at(0x15f4),accepted?0xd57:0xd94,found?(accepted?0x102c0504:0x102c056c):(accepted?0x102c0738:0x102c078c),
                {grammar(second),grammar(second),team(second)+(found?0x221:0x1ee)});
            if(found)p.call(0x1077b720,accepted?0x102c0518:0x102c0580,{scratch(),pool(0x878),frame.at(0x1b3c),frame.at(0x15f4)});
        } else if(found) {
            const auto result=p.call(0x104991b0,0x102c05a8,{c.u32(c.toc(-0x7b54)),team(second),tournament,frame.at(0x15f4),400});
            if(compare(c,std::int8_t(result))==0)p.call(0x1077b720,0x102c05c4,{scratch(),pool(0x878),frame.at(0x1b3c),frame.at(0x15f4)});
            else p.call(0x1077dbf0,0x102c05d4,{scratch(),frame.at(0x1b3c)});
        }
        if(!found) {
            if(compare(c,status,2)==0)p.call(0x1077dbf0,0x102c07a4,{scratch(),frame.at(0x1b3c)});
            else p.call(0x1077b720,0x102c07bc,{scratch(),pool(0x878),frame.at(0x1b3c),frame.at(0x15f4)});
        }
        assign(scratch(),found?0x102c05e0:0x102c07c8);
        c.set(7,c.u32(c.r(28)));link(team(first)+0x221,0x7d5,team(first),found?0x102c0608:0x102c07f0);
        c.set(7,c.u32(c.r(28)));link(team(second)+0x221,0x7d5,team(second),found?0x102c0630:0x102c0818);
        if(found)link(tournament+0x2e,0x7d1,tournament,0x102c0644);
    } else {
        // The headline path leaves r8 holding the table base.
        c.set(8,c.u32(c.r(28)));
        format(scratch(),0xe5b,0x102c0860,{grammar(first),grammar(first),team(first)+0x221});
        assign(scratch(),0x102c086c);c.set(7,c.u32(c.r(28)));link(team(first)+0x221,0x7d5,team(first),0x102c0894);
    }
    return 1;
}
}
