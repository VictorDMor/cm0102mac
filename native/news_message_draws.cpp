#include "news_message_detail.hpp"

namespace cm::aot::news::detail {
std::uint32_t Message::cup_draw() const {
    const auto full=compare(c,std::int8_t(detail))!=0;
    Program p{c};const auto id=value(0);const auto round=short_value(1),stage=short_value(2);
    const auto normalize=[&](std::uint32_t buffer,std::uint32_t first_back,std::uint32_t second_back) {
        if(compare(c,p.call(0x1077dc90,first_back,{buffer,pool(0x1c)}))==0)c.put8(buffer,'f');
        else if(compare(c,p.call(0x1077dc90,second_back,{buffer,pool(0x25)}))==0)c.put8(buffer,'r');
    };
    if(full) {
        const auto first=value(3),second=value(4),venue=value(6);const auto flag=byte(5);
        keep(c,{{13,id},{14,extended(round)},{17,extended(stage)},{18,first},{20,second},{21,extended(flag)},{15,venue}});
        leave(c,{{6,value(2)},{7,value(1)}});p.call(0x107156b0,0x102c1b64,{frame.at(0xf8),extended(short_value(7)),value(8)&0xffff});
        p.call(0x104769d0,0x102c1b88,{competition_name(id),extended(round),extended(stage),2,frame.at(0xd9c),200});
        normalize(frame.at(0xd9c),0x102c1b94,0x102c1bb4);
        const auto grammatical=p.call(0x10476ec0,0x102c1be0,{competition_name(id),extended(round),extended(stage),2});
        const auto team=[&](std::uint32_t index){return c.u32(c.r(28))+index*0x248;};
        const auto grammar=[&](std::uint32_t index){return extended(c.i8(team(index)+0x23b));};
        format(scratch(),0x19e0,0x102c1c78,{grammar(first),grammar(first),grammar(second),grammar(second),extended(std::int8_t(grammatical)),extended(std::int8_t(grammatical)),team(first)+0x221,team(second)+0x221,frame.at(0xd9c)});
        assign(scratch(),0x102c1c84);c.set(7,c.u32(c.r(28)));link(team(first)+0x221,0x7d5,team(first),0x102c1cac);
        c.set(7,c.u32(c.r(28)));link(team(second)+0x221,0x7d5,team(second),0x102c1cd4);
        link(frame.at(0xd9c),0x7d0,competition_name(id),0x102c1cf0);
        p.call(0x10716eb0,0x102c1d04,{frame.at(0xf8),frame.at(0x498),100,255});
        if(compare(c,venue,-1)!=0) {
            bool first_language=compare(c,std::int8_t(p.call(0x10727880,0x102c1d10)))==0;
            if(!first_language)first_language=compare(c,std::int8_t(p.call(0x10727880,0x102c1d1c)),1)==0;
            const auto place=[&]{return c.u32(c.r(19))+venue*0x50;};
            const auto origin=[&]{return c.u32(place()+0x10);};
            bool composed=false;
            if(compare_logical(c,origin())!=0) {
                leave(c,{{5,venue*0x50},{6,c.u32(c.r(19))}});
                if(compare(c,p.call(0x1077dc90,first_language?0x102c1d68:0x102c1e58,{place()+0x18,origin()+0x1c}))!=0 &&
                   compare(c,p.call(0x1077dc90,first_language?0x102c1d88:0x102c1e78,{place()+0x18,pool(0x1a65)}))!=0) {
                    leave(c,{{7,venue*0x50},{8,c.u32(c.r(19))}});
                    p.call(0x1077b720,first_language?0x102c1dc0:0x102c1eb0,{frame.at(0x14c8),pool(first_language?0x1a6d:0x1a74),place()+0x18,origin()+0x1c});composed=true;
                }
            }
            if(!composed) {
                if(compare_logical(c,origin())!=0)p.call(0x1077dbf0,first_language?0x102c1df8:0x102c1ee8,{frame.at(0x14c8),origin()+0x1c});
                else p.call(0x1077dbf0,first_language?0x102c1e14:0x102c1f04,{frame.at(0x14c8),place()+0x18});
            }
            const auto selected=compare(c,flag)!=0;
            format(scratch(),selected?0x1a7c:0x1aff,selected?0x102c1f4c:0x102c1f90,
                {extended(c.i8(place()+0x4b)),extended(c.i8(place()+0x4b)),100,frame.at(0x14c8),frame.at(0x498)});
        } else {
            const auto selected=compare(c,flag)!=0;
            format(scratch(),selected?0x1b77:0x1bd1,selected?0x102c1fb0:0x102c1fc8,{100,frame.at(0x498)});
        }
        append(scratch(),0x102c1fd4);
    } else {
        keep(c,{{13,id},{14,extended(round)},{15,extended(stage)}});
        p.call(0x104769d0,0x102c2010,{competition_name(id),extended(round),extended(stage),3,frame.at(0xcd4),200});
        normalize(frame.at(0xcd4),0x102c201c,0x102c203c);
        const auto grammatical=p.call(0x10476ec0,0x102c2068,{competition_name(id),extended(round),extended(stage),3});
        format(scratch(),0x1c1e,0x102c2084,{extended(std::int8_t(grammatical)),extended(std::int8_t(grammatical)),frame.at(0xcd4)});
        assign(scratch(),0x102c2090);link(frame.at(0xcd4),0x7d0,competition_name(id),0x102c20ac);
    }
    return 1;
}

std::uint32_t Message::pending_cup_draw() const {
    const auto full=compare(c,std::int8_t(detail))!=0;
    Program p{c};const auto id=value(0);const auto round=short_value(1),stage=short_value(2);
    const auto normalize=[&](std::uint32_t buffer,std::uint32_t first_back,std::uint32_t second_back) {
        if(compare(c,p.call(0x1077dc90,first_back,{buffer,pool(0x1c)}))==0)c.put8(buffer,'f');
        else if(compare(c,p.call(0x1077dc90,second_back,{buffer,pool(0x25)}))==0)c.put8(buffer,'r');
    };
    if(full) {
        keep(c,{{13,id},{14,extended(round)},{17,extended(stage)}});
        for(unsigned n=0;n<4;++n)c.put32(frame.at(0x184+n*4),value(n+3));
        leave(c,{{9,value(6)},{10,value(5)}});
        p.call(0x104769d0,0x102c2120,{competition_name(id),extended(round),extended(stage),2,frame.at(0xc0c),200});
        normalize(frame.at(0xc0c),0x102c212c,0x102c214c);
        const auto grammatical=p.call(0x10476ec0,0x102c2178,{competition_name(id),extended(round),extended(stage),2});
        const auto member=[&](unsigned n){return c.u32(frame.at(0x184+n*4));};
        const auto offset=[&](unsigned n){return member(n)*0x248;};
        const auto team=[&](unsigned n){return c.u32(c.r(28))+offset(n);};
        const auto grammar=[&](unsigned n){return extended(c.i8(team(n)+0x23b));};
        const bool second=compare(c,member(1),-1)!=0;
        bool fourth=false;if(second)fourth=compare(c,member(3),-1)!=0;
        if(second&&fourth) {
            keep(c,{{19,extended(std::int8_t(grammatical))},{15,offset(2)+0x221},{18,offset(0)+0x221},{17,offset(1)+0x221},{14,team(0)}});
            format(scratch(),0x1c5a,0x102c22dc,{grammar(0),grammar(0),grammar(1),grammar(1),grammar(2),grammar(2),grammar(3),grammar(3),
                extended(std::int8_t(grammatical)),extended(std::int8_t(grammatical)),team(0)+0x221,team(1)+0x221,team(2)+0x221,team(3)+0x221,frame.at(0xc0c)});
        } else {
            compare(c,member(1),-1);c.set(14,c.u32(c.r(28)));
            const unsigned middle=second?1:2,last=second?2:3;
            format(scratch(),second?0x1d25:0x1dcc,second?0x102c23e8:0x102c24e8,
                {grammar(0),grammar(0),grammar(middle),grammar(middle),grammar(last),grammar(last),extended(std::int8_t(grammatical)),extended(std::int8_t(grammatical)),
                 team(0)+0x221,team(middle)+0x221,team(last)+0x221,frame.at(0xc0c)});
        }
        assign(scratch(),0x102c24f4);c.set(7,c.u32(c.r(28)));link(team(0)+0x221,0x7d5,team(0),0x102c2524);
        c.set(7,c.u32(c.r(28)));link(team(2)+0x221,0x7d5,team(2),0x102c2554);
        if(compare(c,member(1),-1)!=0){c.set(7,c.u32(c.r(28)));link(team(1)+0x221,0x7d5,team(1),0x102c2590);}
        if(compare(c,member(3),-1)!=0){c.set(7,c.u32(c.r(28)));link(team(3)+0x221,0x7d5,team(3),0x102c25cc);}
        c.set(5,c.u32(c.r(30)));link(frame.at(0xc0c),0x7d0,competition_name(id),0x102c25e8);
    } else {
        keep(c,{{13,id},{14,extended(round)},{15,extended(stage)}});
        p.call(0x104769d0,0x102c2624,{competition_name(id),extended(round),extended(stage),3,frame.at(0xb44),200});
        normalize(frame.at(0xb44),0x102c2630,0x102c2650);
        const auto grammatical=p.call(0x10476ec0,0x102c267c,{competition_name(id),extended(round),extended(stage),3});
        format(scratch(),0x1c1e,0x102c2698,{extended(std::int8_t(grammatical)),extended(std::int8_t(grammatical)),frame.at(0xb44)});
        assign(scratch(),0x102c26a4);link(frame.at(0xb44),0x7d0,competition_name(id),0x102c26c0);
    }
    return 1;
}
}
