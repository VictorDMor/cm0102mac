#include "news_message_detail.hpp"

namespace cm::aot::news::detail {
std::uint32_t Message::local(unsigned type) const {
    switch(type) {
    case 2000:return injury_article();
    case 9:return tournament_invitation();case 10:return tournament_reply();
    case 8:return friendly_cancelled();case 11:return tournament_cancelled();
    case 6:return friendly_proposal();case 13:return fixture_list();
    case 1:return rearranged_fixture();case 3:return television_fixture();
    case 4:return hosting_bids();case 5:return host_selected();case 7:return friendly_reply();
    case 12:return virtual_message();case 16:return decoded_virtual_message();
    case 17:case 18:return playoff_place();case 19:return geographical_reshuffle();
    case 21:return group_draw();case 20:return cup_draw();case 38:return pending_cup_draw();case 39:return draw_today();
    case 22:return intertoto_application();case 23:return intertoto_selection();case 26:return conditional_competition_message();
    case 40:return cup_bye();case 1001:return competition_record();case 5000:return manager_message();
    default:throw std::logic_error("Unknown local NEWS message");
    }
}

std::uint32_t Message::virtual_message() const {
    const auto object=competition(value(1));
    leave(c,{{4,value(0)},{5,std::uint32_t(byte(3))},{6,std::uint32_t(byte(2))},
             {7,std::uint32_t(short_value(4))},{8,value(1)},{10,value(5)}});
    if(compare_logical(c,object)==0) {
        assertion(c,frame,0x102c0f2c,0x220,0x118,0x61f);c.set(4,c.toc(slot::success));return 0;
    }
    c.put32(frame.at(0x38),0);c.put32(frame.at(0x3c),detail);c.put32(frame.at(0x40),output);
    c.set(8,0);c.set(9,0);c.set(12,c.u32(c.u32(object)+0x50));
    Program{c}.call(0x10781320,0x102c0fa4,{competition(value(1))});
    c.set(2,c.u32(frame.at(0x14)));return 1;
}

std::uint32_t Message::decoded_virtual_message() const {
    keep(c,{{13,value(0)},{14,std::uint32_t(byte(2))},{19,value(1)},
            {20,std::uint32_t(byte(3))},{18,value(4)}});
    c.put8(frame.at(0x81),5);
    Program{c}.call(0x10463dc0,0x102c1590,{item,frame.at(0x4fc),frame.at(0x81)});
    const auto cursor=c.i8(frame.at(0x81));c.set(4,std::uint32_t(cursor)*4);
    const auto object=competition(c.r(19));
    c.set(3,c.u32(item+8+std::uint32_t(cursor)*4));c.set(9,extended(std::int16_t(c.r(3))));
    if(compare_logical(c,object)==0)return 0;
    c.put32(frame.at(0x38),c.r(20));c.put32(frame.at(0x3c),detail);c.put32(frame.at(0x40),output);
    leave(c,{{4,c.r(13)},{5,c.r(14)},{6,c.u8(frame.at(0x53e))},{7,extended(c.i16(frame.at(0x52e)))},
             {8,frame.at(0x4fc)},{10,c.r(18)}});
    c.set(12,c.u32(c.u32(object)+0x50));
    Program{c}.call(0x10781320,0x102c1600,{competition(c.r(19))});
    c.set(2,c.u32(frame.at(0x14)));return 1;
}

std::uint32_t Message::conditional_competition_message() const {
    c.set(4,value(0));const auto object=competition(c.r(4));c.set(3,c.u32(c.r(24)));
    if(compare_logical(c,object)==0)return 0;
    Program{c}.call(0x10452290,0x102c3b94,{competition(value(0)),item,detail,output});return 1;
}

std::uint32_t Message::competition_record() const {
    const auto full=compare(c,std::int8_t(detail))!=0;const auto id=value(0);c.set(14,id);
    if(full) {
        const auto object=competition(id);c.set(3,object);
        if(compare_logical(c,object)==0 || compare_logical(c,c.u32(object+8))==0) {
            assertion(c,frame,0x102c3ec4,0x1c0,0xe0,0xb51);c.set(4,c.toc(slot::success));return 0;
        }
        Program{c}.call(0x1045c9e0,0x102c3f1c,{c.u32(object+8),item,output});
    } else {
        const auto competition=competition_name(id);
        // r8 retains the table base; the formatter's vararg homes expose it.
        c.set(8,c.u32(c.r(30)));
        format(scratch(),0x2aa1,0x102c3f64,{std::uint32_t(std::int32_t(c.i8(competition+0x67))),
            std::uint32_t(std::int32_t(c.i8(competition+0x67))),competition+0x4d});
        assign(scratch(),0x102c3f70);
        c.set(7,c.u32(c.r(30)));link(competition_name(id)+0x4d,0x7d0,competition_name(id),0x102c3f98);
    }
    return 1;
}

std::uint32_t Message::playoff_place() const {
    const bool quarters=c.i32(item)==18;
    const auto full=compare(c,std::int8_t(detail))!=0;
    const auto team=club(0),competition=competition_name(value(2));const auto section=byte(1);
    keep(c,{{13,team},{14,competition}});
    leave(c,{{3,c.u32(c.r(30))},{4,std::uint32_t(section)},{5,value(0)*0x248},{6,c.u32(c.r(28))}});
    if(full) {
        unsigned phrase;std::uint32_t back;
        if(compare(c,section,1)==0){phrase=quarters?0x1698:0x14af;back=quarters?0x102c1800:0x102c1688;}
        else if(compare(c,section,2)==0){phrase=quarters?0x1730:0x1545;back=quarters?0x102c1844:0x102c16cc;}
        else if(quarters&&compare(c,section,3)==0){phrase=0x17c9;back=0x102c1888;}
        else{phrase=quarters?0x1861:0x15dc;back=quarters?0x102c18c0:0x102c1704;}
        format(scratch(),phrase,back,{std::uint32_t(std::int32_t(c.i8(team+0x242))),
            std::uint32_t(std::int32_t(c.i8(team+0x242))),std::uint32_t(std::int32_t(c.i8(competition+0x6a))),
            std::uint32_t(std::int32_t(c.i8(competition+0x6a))),team+0x1ee,competition+0x1a});
        assign(scratch(),quarters?0x102c18cc:0x102c1710);
        link(team+0x1ee,0x7d5,team,quarters?0x102c18e0:0x102c1724);
        link(competition+0x1a,0x7d0,competition,quarters?0x102c18f4:0x102c1738);
    } else {
        format(scratch(),0x1671,quarters?0x102c1918:0x102c175c,
            {std::uint32_t(std::int32_t(c.i8(team+0x23b))),std::uint32_t(std::int32_t(c.i8(team+0x23b))),team+0x221});
        assign(scratch(),quarters?0x102c1924:0x102c1768);
        link(team+0x221,0x7d5,team,quarters?0x102c1938:0x102c177c);
    }
    return 1;
}

std::uint32_t Message::manager_message() const {
    const auto private_message=byte(1);const auto reply=value(2),sender=value(0);
    keep(c,{{13,reply},{14,std::uint32_t(private_message)}});
    Program program{c};
    if(compare(c,std::int8_t(detail))!=0) {
        if(compare(c,reply)!=0 && compare(c,c.i8(frame.at(0x25eb)))==0) {
            const auto temporary_item=frame.at(0x8d0),temporary_text=frame.at(0xc8);
            program.call(0x10715880,0x102c68e0,{temporary_item+0xd0});
            c.put32(temporary_item,0);c.put8(temporary_item+4,0);
            program.call(0x10778300,0x102c6900,{temporary_item+8,0,Item::payload_size});
            c.put32(temporary_item+0xdc,0);
            c.set(6,c.u16(c.toc(slot::today)+2));c.put16(temporary_item+0xd2,c.r(6));
            c.set(7,extended(c.i16(c.toc(slot::today))));c.put16(temporary_item+0xd0,c.r(7));
            c.set(4,c.u8(c.toc(slot::today)+4));c.put8(temporary_item+0xd4,c.r(4));
            c.set(6,c.u8(c.toc(slot::language)));c.put8(temporary_item+0xe0,0);c.put8(temporary_item+0xd8,c.r(6));c.set(5,0);
            program.call(0x1074b6e0,0x102c694c,{temporary_text,0});
            const auto queue=c.u32(c.toc(-0x7bf0));
            enter(c,0x102c6964,{queue,0,reply,temporary_item});c.set(3,find_item(c,queue,0,reply,temporary_item));
            if(compare(c,std::int8_t(c.r(3)))!=0) {
                enter(c,0x102c6984,{temporary_item,manager(),temporary_text,1,1});
                c.set(3,render_message(c,temporary_item,manager(),temporary_text,1,1));
                append(pool(0x3ebf),0x102c6990);append(c.u32(temporary_text),0x102c699c);append(pool(0x3ec2),0x102c69a8);
            }
            program.call(0x1074b7c0,0x102c69b4,{temporary_text,0xffffffff});
        }
        enter(c,0x102c69c8,{item,3,scratch(),2000});c.set(3,text(c,item,3,scratch(),2000));
        append(scratch(),0x102c69d4);
    } else {
        program.call(0x100caeb0,0x102c69f8,{c.u32(c.toc(-0x7be0))+sender*0x70,frame.at(0x808),200,1});
        const auto personal=compare(c,private_message)!=0;
        format(scratch(),personal?0x3ec6:0x3eef,personal?0x102c6a10:0x102c6a24,{frame.at(0x808)});
        assign(scratch(),0x102c6a30);
    }
    return 1;
}
}
