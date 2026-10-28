#include "news_message_detail.hpp"

namespace cm::aot::news::detail {
std::uint32_t Message::television_fixture() const {
    const auto full=compare(c,std::int8_t(detail))!=0;
    const auto first=value(0),second=value(1),competition_id=value(2);
    const auto round=short_value(3),stage=short_value(4),old_year=short_value(5),new_year=short_value(8);
    const auto old_slot=value(7)&255,new_day=value(9)&0xffff,new_slot=value(10)&255;
    keep(c,{{13,extended(round)},{18,extended(stage)},{19,extended(old_year)},{20,extended(new_year)},
        {22,old_slot},{21,new_day},{23,new_slot},{14,first},{17,second},{15,competition_id}});
    const auto team=[&](std::uint32_t id){return c.u32(c.r(28))+id*0x248;};
    const auto grammar=[&](std::uint32_t id,unsigned offset){return extended(c.i8(team(id)+offset));};
    Program p{c};
    if(full) {
        p.call(0x104769d0,0x102bea50,{competition_name(competition_id),extended(round),extended(stage),2,frame.at(0x107c),200});
        const auto first_word=p.call(0x1077dc90,0x102bea5c,{frame.at(0x107c),pool(0x1c)});
        if(compare(c,first_word)==0)c.put8(frame.at(0x107c),'f');
        else if(compare(c,p.call(0x1077dc90,0x102bea7c,{frame.at(0x107c),pool(0x25)}))==0)c.put8(frame.at(0x107c),'r');
        const auto grammatical=p.call(0x10476ec0,0x102beaa8,{competition_name(competition_id),extended(round),extended(stage),2});c.set(18,grammatical);
        if(compare(c,new_year,old_year)!=0) {
            p.call(0x107156b0,0x102beacc,{frame.at(0x150),extended(new_year),new_day});
            p.call(0x10717270,0x102beae0,{frame.at(0x150),frame.at(0xfb4),200,255});
            format(scratch(),0x28f,0x102beb84,{grammar(first,0x242),grammar(first,0x242),grammar(second,0x242),grammar(second,0x242),
                extended(std::int8_t(grammatical)),extended(std::int8_t(grammatical)),100,team(first)+0x1ee,team(second)+0x1ee,frame.at(0x107c),frame.at(0xfb4)});
            assign(scratch(),0x102beb90);
        } else if(compare_logical(c,new_slot,old_slot)!=0) {
            const auto time=p.call(0x10718570,0x102bebb0,{new_slot,0});c.set(13,time);
            const auto time_grammar=p.call(0x10718650,0x102bebbc,{new_slot});
            format(scratch(),0x320,0x102bec5c,{grammar(first,0x242),grammar(first,0x242),grammar(second,0x242),grammar(second,0x242),
                extended(std::int8_t(grammatical)),extended(std::int8_t(grammatical)),extended(std::int8_t(time_grammar)),team(first)+0x1ee,team(second)+0x1ee,frame.at(0x107c),time});
            assign(scratch(),0x102bec68);
        } else {
            format(scratch(),0x3c9,0x102bed00,{grammar(first,0x242),grammar(first,0x242),grammar(second,0x242),grammar(second,0x242),
                extended(std::int8_t(grammatical)),extended(std::int8_t(grammatical)),team(first)+0x1ee,team(second)+0x1ee,frame.at(0x107c)});
            assign(scratch(),0x102bed0c);
        }
        c.set(7,c.u32(c.r(28)));link(team(first)+0x1ee,0x7d5,team(first),0x102bed34);
        c.set(7,c.u32(c.r(28)));link(team(second)+0x1ee,0x7d5,team(second),0x102bed5c);
        link(frame.at(0x107c),0x7d0,competition_name(competition_id),0x102bed78);
    } else {
        format(scratch(),0x44a,0x102bedf8,{grammar(first,0x23b),grammar(first,0x23b),grammar(second,0x23b),grammar(second,0x23b),team(first)+0x221,team(second)+0x221});
        assign(scratch(),0x102bee04);
        c.set(7,c.u32(c.r(28)));link(team(first)+0x221,0x7d5,team(first),0x102bee2c);
        c.set(7,c.u32(c.r(28)));link(team(second)+0x221,0x7d5,team(second),0x102bee54);
    }
    return 1;
}

std::uint32_t Message::rearranged_fixture() const {
    const auto full=compare(c,std::int8_t(detail))!=0;
    const auto first=value(0),second=value(1),competition_id=value(2),old_first=value(7),old_second=value(8),old_competition=value(9);
    const auto round=short_value(3),stage=short_value(4),year=short_value(5),old_year=short_value(12);
    const auto day=value(6)&0xffff,old_day=value(13)&0xffff;
    keep(c,{{23,extended(round)},{24,extended(stage)},{25,extended(year)},{21,day},{20,extended(old_year)},
        {22,old_day},{13,first},{14,second},{17,competition_id},{18,old_first},{19,old_second},{15,old_competition}});
    const auto team=[&](std::uint32_t id){return c.u32(c.r(28))+id*0x248;};
    const auto grammar=[&](std::uint32_t id,unsigned offset){return extended(c.i8(team(id)+offset));};
    Program p{c};
    if(full) {
        p.call(0x104769d0,0x102be514,{competition_name(competition_id),extended(round),extended(stage),2,frame.at(0x12d4),200});
        if(compare(c,p.call(0x1077dc90,0x102be520,{frame.at(0x12d4),pool(0x1c)}))==0)c.put8(frame.at(0x12d4),'f');
        else if(compare(c,p.call(0x1077dc90,0x102be540,{frame.at(0x12d4),pool(0x25)}))==0)c.put8(frame.at(0x12d4),'r');
        const auto grammatical=p.call(0x10476ec0,0x102be56c,{competition_name(competition_id),extended(round),extended(stage),2});c.set(23,grammatical);
        p.call(0x107156b0,0x102be580,{frame.at(0x160),extended(year),day});
        p.call(0x10717270,0x102be594,{frame.at(0x160),frame.at(0x120c),200,255});
        p.call(0x107156b0,0x102be5a4,{frame.at(0x158),extended(old_year),old_day});
        p.call(0x10717270,0x102be5b8,{frame.at(0x158),frame.at(0x1144),200,255});
        if(compare(c,old_first,-1)!=0) {
            // These registers become persistent offsets while the large legacy
            // vararg tuple is assembled. External formatters can observe them.
            keep(c,{{22,frame.at(0x1144)},{25,old_first*0x248+0x221},{24,old_second*0x248+0x221},
                {23,old_competition*0x6c+0x4d},{26,extended(c.i8(competition_name(old_competition)+0x67))},
                {21,first*0x248},{20,first*0x248},{27,c.u32(c.r(30))}});
            format(scratch(),0x2d,0x102be740,{grammar(first,0x242),grammar(first,0x242),grammar(second,0x242),grammar(second,0x242),
                extended(std::int8_t(grammatical)),extended(std::int8_t(grammatical)),100,grammar(old_first,0x23b),grammar(old_first,0x23b),grammar(old_second,0x23b),grammar(old_second,0x23b),
                extended(c.i8(competition_name(old_competition)+0x67)),extended(c.i8(competition_name(old_competition)+0x67)),100,
                team(first)+0x1ee,team(second)+0x1ee,frame.at(0x12d4),frame.at(0x120c),team(old_first)+0x221,team(old_second)+0x221,competition_name(old_competition)+0x4d,frame.at(0x1144)});
        } else {
            format(scratch(),0x12a,0x102be7f8,{extended(std::int8_t(grammatical)),extended(std::int8_t(grammatical)),grammar(first,0x242),grammar(first,0x242),grammar(second,0x242),grammar(second,0x242),
                100,100,frame.at(0x12d4),team(first)+0x1ee,team(second)+0x1ee,frame.at(0x1144),frame.at(0x120c)});
        }
        assign(scratch(),0x102be804);
        c.set(7,c.u32(c.r(28)));link(team(first)+0x1ee,0x7d5,team(first),0x102be82c);
        c.set(7,c.u32(c.r(28)));link(team(second)+0x1ee,0x7d5,team(second),0x102be854);
        link(frame.at(0x12d4),0x7d0,competition_name(competition_id),0x102be870);
        if(compare(c,old_first,-1)!=0) {
            c.set(7,c.u32(c.r(28)));link(team(old_first)+0x221,0x7d5,team(old_first),0x102be8a0);
            c.set(7,c.u32(c.r(28)));link(team(old_second)+0x221,0x7d5,team(old_second),0x102be8c8);
            c.set(7,c.u32(c.r(30)));link(competition_name(old_competition)+0x4d,0x7d0,competition_name(old_competition),0x102be8f0);
        }
    } else {
        format(scratch(),0x24c,0x102be970,{grammar(first,0x23b),grammar(first,0x23b),grammar(second,0x23b),grammar(second,0x23b),team(first)+0x221,team(second)+0x221});
        assign(scratch(),0x102be97c);
        c.set(7,c.u32(c.r(28)));link(team(first)+0x221,0x7d5,team(first),0x102be9a4);
        c.set(7,c.u32(c.r(28)));link(team(second)+0x221,0x7d5,team(second),0x102be9cc);
    }
    return 1;
}
}
