#include "news_message_detail.hpp"

namespace cm::aot::news::detail {
std::uint32_t Message::friendly_reply() const {
    Program program{c};program.call(0x10715880,0x102bfe64,{frame.at(0x140)});
    const auto first=value(0),second=value(1),requester=value(2),year=value(3)&0xffff,day=value(4)&0xffff;
    const auto status=byte(5);const auto opponent=compare(c,requester,first)==0?second:first;
    keep(c,{{13,first},{14,second},{18,requester},{19,std::uint32_t(status)},{15,opponent}});c.set(6,year);
    program.call(0x107156b0,0x102bfea8,{frame.at(0xb0),extended(std::int16_t(year)),day});
    const auto full=compare(c,std::int8_t(detail))!=0;
    c.put16(frame.at(0x142),c.u16(frame.at(0xb2)));c.put16(frame.at(0x140),c.u16(frame.at(0xb0)));c.put8(frame.at(0x144),c.u8(frame.at(0xb4)));
    if(full) {
        program.call(0x10716eb0,0x102bfedc,{frame.at(0x140),frame.at(0x6dc),100,255});
        const auto accepted=compare(c,status,3)==0;
        if(accepted||compare(c,status,4)==0) {
            const auto a=c.u32(c.r(28))+opponent*0x248,b=c.u32(c.r(28))+requester*0x248;
            format(scratch(),accepted?0xa26:0xab2,accepted?0x102bff74:0x102c0010,
                {std::uint32_t(std::int32_t(c.i8(a+0x242))),std::uint32_t(std::int32_t(c.i8(a+0x242))),
                 std::uint32_t(std::int32_t(c.i8(b+0x242))),std::uint32_t(std::int32_t(c.i8(b+0x242))),100,a+0x1ee,b+0x1ee,frame.at(0x6dc)});
        }
        // Other states deliberately assign the pre-existing shared scratch.
        assign(scratch(),0x102c001c);c.set(7,c.u32(c.r(28)));
        link(c.u32(c.r(28))+first*0x248+0x1ee,0x7d5,c.u32(c.r(28))+first*0x248,0x102c0044);
        c.set(7,c.u32(c.r(28)));link(c.u32(c.r(28))+second*0x248+0x1ee,0x7d5,c.u32(c.r(28))+second*0x248,0x102c006c);
    } else {
        const auto accepted=compare(c,status,3)==0;
        if(accepted||compare(c,status,4)==0) {
            const auto team=c.u32(c.r(28))+opponent*0x248;c.set(8,c.u32(c.r(28)));
            format(scratch(),accepted?0xb3e:0xb78,accepted?0x102c00c0:0x102c0114,
                {std::uint32_t(std::int32_t(c.i8(team+0x23b))),std::uint32_t(std::int32_t(c.i8(team+0x23b))),team+0x221});
        }
        assign(scratch(),0x102c0120);c.set(7,c.u32(c.r(28)));
        link(c.u32(c.r(28))+opponent*0x248+0x221,0x7d5,c.u32(c.r(28))+opponent*0x248,0x102c0148);
    }
    return 1;
}

std::uint32_t Message::friendly_cancelled() const {
    Program program{c};program.call(0x10715880,0x102c0164,{frame.at(0x138)});
    const auto first=value(0),second=value(1),year=value(2)&0xffff,day=value(3)&0xffff;
    keep(c,{{13,first},{14,second}});
    program.call(0x107156b0,0x102c0188,{frame.at(0xa8),extended(std::int16_t(year)),day});
    const auto full=compare(c,std::int8_t(detail))!=0;
    c.put16(frame.at(0x13a),c.u16(frame.at(0xaa)));c.put16(frame.at(0x138),c.u16(frame.at(0xa8)));
    c.put8(frame.at(0x13c),c.u8(frame.at(0xac)));
    if(full) {
        program.call(0x10716eb0,0x102c01bc,{frame.at(0x138),frame.at(0x678),100,255});
        const auto a=c.u32(c.r(28))+first*0x248,b=c.u32(c.r(28))+second*0x248;
        format(scratch(),0xbb2,0x102c0248,{std::uint32_t(std::int32_t(c.i8(a+0x23b))),std::uint32_t(std::int32_t(c.i8(a+0x23b))),
            std::uint32_t(std::int32_t(c.i8(b+0x23b))),std::uint32_t(std::int32_t(c.i8(b+0x23b))),100,a+0x221,b+0x221,frame.at(0x678)});
        assign(scratch(),0x102c0254);c.set(7,c.u32(c.r(28)));
        link(c.u32(c.r(28))+first*0x248+0x221,0x7d5,c.u32(c.r(28))+first*0x248,0x102c027c);
        c.set(7,c.u32(c.r(28)));link(c.u32(c.r(28))+second*0x248+0x221,0x7d5,c.u32(c.r(28))+second*0x248,0x102c02a4);
    } else {
        const auto a=c.u32(c.r(28))+first*0x248,b=c.u32(c.r(28))+second*0x248;
        format(scratch(),0xc67,0x102c0324,{std::uint32_t(std::int32_t(c.i8(a+0x23b))),std::uint32_t(std::int32_t(c.i8(a+0x23b))),
            std::uint32_t(std::int32_t(c.i8(b+0x23b))),std::uint32_t(std::int32_t(c.i8(b+0x23b))),a+0x221,b+0x221});
        assign(scratch(),0x102c0330);c.set(7,c.u32(c.r(28)));
        link(c.u32(c.r(28))+first*0x248+0x221,0x7d5,c.u32(c.r(28))+first*0x248,0x102c0358);
        c.set(7,c.u32(c.r(28)));link(c.u32(c.r(28))+second*0x248+0x221,0x7d5,c.u32(c.r(28))+second*0x248,0x102c0380);
    }
    return 1;
}

std::uint32_t Message::tournament_cancelled() const {
    Program program{c};program.call(0x10715880,0x102c0d7c,{frame.at(0x120)});
    const auto club_id=value(0),year=value(1)&0xffff,day=value(2)&0xffff;c.set(13,club_id);
    program.call(0x107156b0,0x102c0d9c,{frame.at(0x90),extended(std::int16_t(year)),day});
    const auto full=compare(c,std::int8_t(detail))!=0;
    c.put16(frame.at(0x122),c.u16(frame.at(0x92)));c.put16(frame.at(0x120),c.u16(frame.at(0x90)));
    c.put8(frame.at(0x124),c.u8(frame.at(0x94)));
    if(full) {
        program.call(0x10716eb0,0x102c0dd0,{frame.at(0x120),frame.at(0x54c),100,255});
        const auto team=c.u32(c.r(28))+club_id*0x248;
        format(scratch(),0x11e7,0x102c0e1c,{std::uint32_t(std::int32_t(c.i8(team+0x23b))),
            std::uint32_t(std::int32_t(c.i8(team+0x23b))),100,team+0x221,frame.at(0x54c)});
        assign(scratch(),0x102c0e28);c.set(7,c.u32(c.r(28)));
        link(c.u32(c.r(28))+club_id*0x248+0x221,0x7d5,c.u32(c.r(28))+club_id*0x248,0x102c0e50);
    } else {
        const auto team=c.u32(c.r(28))+club_id*0x248;c.set(8,c.u32(c.r(28)));
        format(scratch(),0x1266,0x102c0e98,{std::uint32_t(std::int32_t(c.i8(team+0x23b))),
            std::uint32_t(std::int32_t(c.i8(team+0x23b))),team+0x221});
        assign(scratch(),0x102c0ea4);c.set(7,c.u32(c.r(28)));
        link(c.u32(c.r(28))+club_id*0x248+0x221,0x7d5,c.u32(c.r(28))+club_id*0x248,0x102c0ecc);
    }
    return 1;
}
}

namespace cm::aot::news::detail {
std::uint32_t Message::friendly_proposal() const {
    Program p{c};p.call(0x10715880,0x102bf66c,{frame.at(0x148)});
    const auto first=value(0),second=value(1),requester=value(2),year=value(3)&0xffff,day=value(4)&0xffff;const auto status=byte(5);
    keep(c,{{13,year},{14,day},{18,first},{19,second},{15,requester},{20,extended(status)}});
    p.call(0x107156b0,0x102bf6a0,{frame.at(0xb8),extended(std::int16_t(year)),day});
    const auto full=compare(c,std::int8_t(detail))!=0;
    c.put16(frame.at(0x14a),c.u16(frame.at(0xba)));c.put16(frame.at(0x148),c.u16(frame.at(0xb8)));c.put8(frame.at(0x14c),c.u8(frame.at(0xbc)));
    const auto team=[&](std::uint32_t id){return c.u32(c.r(28))+id*0x248;};
    const auto grammar=[&](std::uint32_t id){return extended(c.i8(team(id)+0x23b));};
    if(full) {
        c.put8(frame.at(0x1784),0);
        const auto slot=p.call(0x10497570,0x102bf6e4,{c.u32(c.toc(-0x7b54)),frame.at(0x148),team(requester)});c.set(17,slot);
        p.call(0x10716eb0,0x102bf700,{frame.at(0x148),frame.at(0x7a4),100,255});
        const bool home=compare(c,first,requester)==0;
        const auto subject=home?first:second,opponent=home?second:first;
        const auto stadium=c.u32(team(first)+0x3c);
        if(compare_logical(c,stadium)!=0) {
            p.call(0x1077b720,home?0x102bf73c:0x102bfa70,{frame.at(0x740),c.u32(team(first)+0x3c)+0x18});
            format(frame.at(0x1f24),home?0x732:0x8ba,home?0x102bf808:0x102bfb3c,
                {grammar(subject),grammar(subject),grammar(opponent),grammar(opponent),extended(c.i8(c.u32(team(first)+0x3c)+0x4b)),extended(c.i8(c.u32(team(first)+0x3c)+0x4b)),
                 100,team(subject)+0x221,team(opponent)+0x221,frame.at(0x740),frame.at(0x7a4)});
        } else {
            format(frame.at(0x1f24),home?0x7c5:0x953,home?0x102bf898:0x102bfbcc,
                {grammar(subject),grammar(subject),grammar(opponent),grammar(opponent),100,team(subject)+0x221,team(opponent)+0x221,frame.at(0x7a4)});
        }
        const bool accepted=compare(c,status,3)==0;
        if(accepted||compare(c,status,4)==0) {
            c.set(8,c.u32(c.r(28)));
            format(frame.at(0x1784),accepted?0x83d:0x87f,home?(accepted?0x102bf8e8:0x102bf950):(accepted?0x102bfc1c:0x102bfc84),
                {grammar(opponent),grammar(opponent),team(opponent)+0x221});
            p.call(0x1077b720,home?(accepted?0x102bf8fc:0x102bf964):(accepted?0x102bfc30:0x102bfc98),
                {scratch(),pool(0x878),frame.at(0x1f24),frame.at(0x1784)});
        } else {
            c.put32(frame.at(0x38),400);
            const auto result=p.call(0x10498f70,home?0x102bf9b0:0x102bfce4,{c.u32(c.toc(-0x7b54)),team(first),team(second),team(opponent),year,day,slot,frame.at(0x1784)});
            if(compare(c,std::int8_t(result))==0)p.call(0x1077b720,home?0x102bf9cc:0x102bfd00,{scratch(),pool(0x878),frame.at(0x1f24),frame.at(0x1784)});
            else p.call(0x1077dbf0,home?0x102bf9dc:0x102bfd10,{scratch(),frame.at(0x1f24)});
        }
        assign(scratch(),home?0x102bf9e8:0x102bfd1c);
        c.set(7,c.u32(c.r(28)));link(team(first)+0x221,0x7d5,team(first),home?0x102bfa10:0x102bfd44);
        c.set(7,c.u32(c.r(28)));link(team(second)+0x221,0x7d5,team(second),home?0x102bfa38:0x102bfd6c);
    } else {
        format(scratch(),0x9d1,0x102bfdec,{grammar(first),grammar(first),grammar(second),grammar(second),team(first)+0x221,team(second)+0x221});
        assign(scratch(),0x102bfdf8);c.set(7,c.u32(c.r(28)));link(team(first)+0x221,0x7d5,team(first),0x102bfe20);
        c.set(7,c.u32(c.r(28)));link(team(second)+0x221,0x7d5,team(second),0x102bfe48);
    }
    return 1;
}
}
