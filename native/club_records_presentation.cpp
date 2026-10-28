#include "club_records_detail.hpp"
#include "guest_fpu.hpp"
#include <array>

namespace cm::aot::club_records {
using namespace detail;
namespace {
constexpr std::array sites{
#include "club_records_format_sites.inc"
};
struct Presentation {
    Cpu& c;const Frame& frame;std::uint32_t club,record,output;unsigned kind;Program p{c};
    std::uint32_t local(unsigned below_caller) const {return frame.caller()-below_caller;}
    std::uint32_t pool(unsigned offset) const {return c.toc(slot::pool)+offset;}
    std::uint32_t call(unsigned index,std::uint32_t target,std::initializer_list<std::uint32_t> args={}) const {
        return p.call(target,sites[kind][index],args);
    }
    std::uint32_t signed_byte(unsigned offset) const {return std::uint32_t(std::int32_t(c.i8(record+offset)));}
    std::uint32_t signed_half(unsigned offset) const {return std::uint32_t(std::int32_t(c.i16(record+offset)));}
    void append(unsigned index) const {call(index,0x1074b830,{output,local(0x228)});}
    void link(unsigned index,std::uint32_t text,unsigned type,std::uint32_t identity) const {
        call(index,0x1074bab0,{output,text,type,identity});
    }
    std::uint32_t club_at(unsigned id_offset) const {return c.u32(c.r(29))+c.u32(record+id_offset)*0x248;}
    std::uint32_t competition_at(unsigned id_offset) const {return c.u32(c.r(28))+c.u32(record+id_offset)*index_size;}
    void club_name(unsigned index,unsigned id_offset,unsigned buffer) const {
        c.set(5,c.u32(c.r(29)));call(index,0x1077dbf0,{local(buffer),c.r(5)+c.u32(record+id_offset)*0x248+0x221});
    }
    void date(unsigned index,unsigned offset,unsigned buffer) const {call(index,0x10715e00,{record+offset,local(buffer),20,255});}
    void year(unsigned index,unsigned offset,unsigned buffer) const {call(index,0x10192350,{c.u16(record+offset),club,local(buffer),20});}
    void player_name(unsigned index,unsigned offset,unsigned buffer,unsigned style=1) const {
        call(index,0x100cb590,{c.u32(record+offset),c.u32(record+offset+4),c.u32(record+offset+8),
                    local(buffer),200,style,c.u32(record+offset+12)});
    }
    void player_link(unsigned check,unsigned add,unsigned offset,unsigned buffer) const {
        const auto player=c.u32(c.r(26))+c.u32(record+offset+12)*0x70;
        if(nonzero_byte(c,call(check,0x100cbde0,{player,c.u32(record+offset),c.u32(record+offset+4),
                                                    c.u32(record+offset+8),record+offset+16})))
            link(add,local(buffer),2007,c.u32(c.r(26))+c.u32(record+offset+12)*0x70);
    }
    void stage(unsigned text_call,unsigned gender_call,unsigned match_offset,unsigned buffer,
               unsigned opponent_offset,unsigned opponent_gender,unsigned stage_gender) const {
        const auto opponent=club_at(opponent_offset);c.set(9,opponent);c.set(opponent_gender,c.u8(opponent+0x23b));
        call(text_call,0x104769d0,{competition_at(match_offset+8),signed_half(match_offset+12),
                signed_half(match_offset+14),3,local(buffer),200});
        c.set(stage_gender,call(gender_call,0x10476ec0,{competition_at(match_offset+8),signed_half(match_offset+12),signed_half(match_offset+14),3}));
    }
    void rank() const {
        const auto offset=kind==1?0u:12u;
        if(compare(c,c.i32(record+offset+4),-1)<=0)return;
        const auto ordinal=kind==1?0x266cu:0x2694u,name=kind==1?0x2f0u:0x3b8u,season=kind==1?0x2680u:0x26a8u;
        const auto position=call(0,0x107467a0,{c.u8(record+offset)+1u,missing});call(1,0x1077dbf0,{local(ordinal),position});
        c.set(5,c.u32(c.r(28)));call(2,0x1077dbf0,{local(name),c.r(5)+c.u32(record+offset+4)*index_size+0x4d});
        c.set(3,c.u16(record+offset+8));c.set(4,competition_at(offset+4));
        compare_logical(c,c.r(3));c.set(21,c.u8(c.r(4)+0x67));
        const auto gender=std::uint32_t(std::int32_t(std::int8_t(c.r(21))));
        if(c.r(3)) {year(3,offset+8,season);call(4,0x10727890,{local(0x228),pool(0x36e),gender,gender,local(ordinal),local(name),local(season)});}
        else call(5,0x10727890,{local(0x228),pool(0x3d5),gender,gender,local(ordinal),local(name)});
        append(6);link(7,local(name),2000,competition_at(offset+4));
    }
    void attendance() const {
        const unsigned offset=kind==3?0x18:kind==4?0x40:0x68;
        if(compare(c,c.i32(record+offset),-1)<=0)return;
        const auto opponent=kind==3?0x480u:kind==4?0x610u:0x868u;
        const auto stage_buffer=kind==3?0x548u:kind==4?0x6d8u:0x930u;
        const auto date_buffer=kind==3?0x26bcu:kind==4?0x26d0u:0x26e4u;
        club_name(0,offset+28,opponent);stage(1,2,offset+4,stage_buffer,offset+28,22,21);date(3,offset+4,date_buffer);
        std::uint32_t value=c.u32(record+offset);unsigned formatted=4,appended=5,club_link=6,competition_link=7;
        if(kind==4) {
            c.put32(local(0x2988),value);call(4,0x10009f20,{local(0x2988),6});
            call(5,0x1000b410,{local(0x7a0),200,c.r(3),c.r(4),6});value=local(0x7a0);
            formatted=6;appended=7;club_link=8;competition_link=9;
        }
        c.put32(frame.at(0x38),local(stage_buffer));c.put32(frame.at(0x3c),local(date_buffer));
        const auto gender=std::uint32_t(std::int32_t(std::int8_t(c.r(21))));
        call(formatted,0x10727890,{local(0x228),pool(kind==4?0x482:0x41c),std::uint32_t(std::int32_t(std::int8_t(c.r(22)))),
                gender,gender,100,value,local(opponent)});
        append(appended);link(club_link,local(opponent),2005,club_at(offset+28));
        link(competition_link,local(stage_buffer),2000,competition_at(offset+12));
    }
    void match_record() const {
        const auto number=kind-7,offset=0x98+number*0x24;
        if(compare(c,c.i32(record+offset+8),-1)<=0)return;
        const auto score=0x28dc+number*12,venue=0x2924+number*8,
            opponent=0x9f8+number*400,stage_buffer=0xac0+number*400,date_buffer=0x270c+number*20;
        call(0,0x1077b720,{local(score),pool(0x53b),signed_byte(offset+28),signed_byte(offset+29)});
        call(1,0x1030db50,{c.u8(record+offset+16),local(venue),5});club_name(2,offset+24,opponent);
        stage(3,4,offset,stage_buffer,offset+24,23,21);date(5,offset,date_buffer);
        c.put32(frame.at(0x38),local(venue));c.put32(frame.at(0x3c),local(stage_buffer));c.put32(frame.at(0x40),local(date_buffer));
        const auto gender=std::uint32_t(std::int32_t(std::int8_t(c.r(21))));
        call(6,0x10727890,{local(0x228),pool(0x541),std::uint32_t(std::int32_t(std::int8_t(c.r(23)))),gender,gender,100,local(score),local(opponent)});
        append(7);link(8,local(opponent),2005,club_at(offset+24));link(9,local(stage_buffer),2000,competition_at(offset+8));
        if(nonzero_byte(c,call(10,0x1029eb20,{c.u32(c.r(22)),c.u32(record+offset+32),c.u32(record+offset+20),
                                                  c.u32(record+offset+24),c.u16(record+offset),c.u16(record+offset+2)})))
            link(11,local(score),2003,c.u32(record+offset+32));
    }
    void streak() const {
        const auto offset=0x14c+(kind-12)*20;
        c.set(5,c.u32(record+offset));if(compare(c,c.r(5),-1)<=0)return;
        const bool multiple=compare(c,c.r(5),1)>0;
        const auto first=0x2770+(kind-12)*40,second=first+20;
        if(multiple) {
            date(0,offset+4,first);
            if(kind<14)date(1,offset+12,second);
            else {
                const auto index_field=kind==14?0x40u:0x54u;
                c.set(3,c.u32(record+offset));bool active=false;
                if(compare(c,c.r(3),c.i32(c.r(21)+index_field))==0) {
                    const auto date_word=c.u32(c.r(21)+index_field+4),date_tail=c.u32(c.r(21)+index_field+8);
                    const auto temporary=kind==14?0x297cu:0x2984u;
                    c.set(4,0);c.set(5,date_tail);c.put32(local(temporary),date_word);c.put32(local(temporary-4),date_tail);
                    active=compare_logical(c,date_word&0xffff,c.u16(record+offset+6))==0 &&
                           compare(c,std::int16_t(date_word>>16),c.i16(record+offset+4))==0;
                    c.set(4,active);compare(c,c.r(4));
                }
                if(active)call(1,0x1077dbf0,{local(second),pool(0x638)});else date(2,offset+12,second);
            }
            const auto format_offset=kind==12?0x5b7u:kind==13?0x5f7u:kind==14?0x63au:0x67eu;
            call(kind<14?2:3,0x10727890,{local(0x228),pool(format_offset),c.u32(record+offset),local(first),local(second)});
        }else call(kind<14?3:4,0x1077b720,{local(0x228),pool(0x537)});
        append(kind<14?4:5);
    }
    void player_total() const {
        struct Shape {unsigned offset,name,year,with_year,without_year;bool name_first;};
        constexpr std::array shapes{
            Shape{0x19c,0x11c8,0x2810,0x6c3,0x700,true},Shape{0x1bc,0x1290,0x2824,0x6c3,0x700,true},
            Shape{0x21c,0x15b0,0x284c,0x7ad,0x7ec,false},Shape{0x23c,0x1678,0x2860,0x810,0x873,false},
            Shape{0x25c,0x1740,0x2874,0x8bb,0x8ff,false},Shape{0x27c,0x1808,0x2888,0x928,0x996,true}};
        const auto& shape=shapes[kind<18?kind-16:kind-17];
        if(kind==20) {
            Fpu fp(c);c.set(3,c.toc(slot::doubles));c.set_f(1,c.f32(record+shape.offset));
            fp.fcmp(0,c.f(1),c.f64(c.r(3)+0x48));if(!(c.cr(0)&4))return;
        }else if(compare(c,c.i8(record+shape.offset),-1)<=0)return;
        player_name(0,shape.offset+4,shape.name);
        c.set(3,c.u16(record+shape.offset+(kind==20?30:kind==22?28:28)));
        const bool dated=compare_logical(c,c.r(3))!=0;
        const auto year_offset=shape.offset+(kind==20?30:28);
        if(dated)year(1,year_offset,shape.year);
        if(kind==20) {
            const double average=c.f32(record+shape.offset);c.put_f64(frame.at(0x20),average);
            const auto high=c.u32(frame.at(0x20)),low=c.u32(frame.at(0x24));
            if(dated)call(2,0x10727890,{local(0x228),pool(shape.with_year),high,low,local(shape.name),signed_byte(0x258),local(shape.year)});
            else call(3,0x10727890,{local(0x228),pool(shape.without_year),high,low,local(shape.name),signed_byte(0x258)});
        }else if(kind==22) {
            if(dated)call(2,0x10727890,{local(0x228),pool(shape.with_year),local(shape.name),signed_byte(shape.offset),signed_byte(shape.offset+1),local(shape.year)});
            else call(3,0x10727890,{local(0x228),pool(shape.without_year),local(shape.name),signed_byte(shape.offset),signed_byte(shape.offset+1)});
        }else if(shape.name_first) {
            if(dated)call(2,0x10727890,{local(0x228),pool(shape.with_year),local(shape.name),signed_byte(shape.offset),local(shape.year)});
            else call(3,0x10727890,{local(0x228),pool(shape.without_year),local(shape.name),signed_byte(shape.offset)});
        }else {
            if(dated)call(2,0x10727890,{local(0x228),pool(shape.with_year),signed_byte(shape.offset),local(shape.name),local(shape.year)});
            else call(3,0x10727890,{local(0x228),pool(shape.without_year),signed_byte(shape.offset),local(shape.name)});
        }
        append(4);player_link(5,6,shape.offset+4,shape.name);
    }
    void career_total() const {
        const auto offset=kind==23?0x29cu:kind==24?0x2b8u:0x35cu;
        const auto buffer=kind==23?0x18d0u:kind==24?0x1998u:0x1f10u;
        if(compare(c,c.i16(record+offset),-1)<=0)return;
        player_name(0,offset+4,buffer);
        if(kind==27)call(1,0x10727890,{local(0x228),pool(0xabc),local(buffer),signed_half(offset)});
        else call(1,0x10727890,{local(0x228),pool(kind==23?0x9e9:0xa11),signed_half(offset),local(buffer)});
        append(2);player_link(3,4,offset+4,buffer);
    }
    void player_match() const {
        const bool age=kind==25||kind==26;
        const auto offset=kind==18?0x1dcu:kind==25?0x2d4u:0x318u;
        const auto person=kind==18?0x1358u:kind==25?0x1a60u:0x1cb8u;
        const auto opponent=kind==18?0x1420u:kind==25?0x1b28u:0x1d80u;
        const auto stage_buffer=kind==18?0x14e8u:kind==25?0x1bf0u:0x1e48u;
        const auto date_buffer=kind==18?0x2838u:kind==25?0x289cu:0x28b0u;
        const auto venue=kind==18?0x294cu:kind==25?0x295cu:0x296cu;
        const auto match_offset=offset+(age?32:28);
        if(age) {if(compare(c,c.i32(record+match_offset+8),-1)<=0)return;}
        else if(compare(c,c.i8(record+offset),-1)<=0)return;
        player_name(0,offset+(age?8:4),person,2);
        const auto age_buffer=kind==25?0x2954u:0x2964u;
        if(age)call(1,0x100cd7c0,{record+match_offset,record+offset,local(age_buffer),5});
        club_name(age?2:1,match_offset+24,opponent);
        c.set(6,c.u32(c.r(29)));c.set(6,c.r(6)+c.u32(record+match_offset+24)*0x248);c.set(21,c.u8(c.r(6)+0x23b));
        call(age?3:2,0x1030db50,{c.u8(record+match_offset+16),local(venue),5});
        // Player-match display keeps the opponent/stage genders in r21/r22.
        call(age?4:3,0x104769d0,{competition_at(match_offset+8),signed_half(match_offset+12),signed_half(match_offset+14),3,local(stage_buffer),200});
        c.set(22,call(age?5:4,0x10476ec0,{competition_at(match_offset+8),signed_half(match_offset+12),signed_half(match_offset+14),3}));
        date(age?6:5,match_offset,date_buffer);
        c.put32(frame.at(0x38),local(opponent));c.put32(frame.at(0x3c),local(venue));
        c.put32(frame.at(0x40),local(stage_buffer));c.put32(frame.at(0x44),local(date_buffer));
        const auto opponent_gender=std::uint32_t(std::int32_t(std::int8_t(c.r(21)))),stage_gender=std::uint32_t(std::int32_t(std::int8_t(c.r(22))));
        if(age)call(7,0x10727890,{local(0x228),pool(0xa33),opponent_gender,stage_gender,stage_gender,100,local(person),local(age_buffer)});
        else call(6,0x10727890,{local(0x228),pool(0x722),opponent_gender,stage_gender,stage_gender,100,signed_byte(offset),local(person)});
        append(age?8:7);link(age?9:8,local(opponent),2005,club_at(match_offset+24));
        link(age?10:9,local(stage_buffer),2000,competition_at(match_offset+8));
        player_link(age?11:10,age?12:11,offset+(age?8:4),person);
    }
    void transfer() const {
        const auto offset=kind==28?0x378u:0x3a4u,person=kind==28?0x1fd8u:0x2230u,
            money=kind==28?0x20a0u:0x22f8u,opponent=kind==28?0x2168u:0x23c0u,
            date_buffer=kind==28?0x25b4u:0x2618u,temporary=kind==28?0x298cu:0x2990u;
        if(compare(c,c.i32(record+offset),-1)<=0)return;
        player_name(0,offset+4,person);call(1,0x10715e00,{record+offset+32,local(date_buffer),100,255});
        c.put32(local(temporary),c.u32(record+offset));call(2,0x10009f20,{local(temporary),6});
        call(3,0x1000b410,{local(money),200,c.r(3),c.r(4),6});club_name(4,offset+28,opponent);
        const auto gender=std::uint32_t(std::int32_t(c.i8(club_at(offset+28)+0x23b)));
        const bool conditional=compare(c,c.i8(record+offset+40))!=0;c.put32(frame.at(0x38),local(date_buffer));
        const auto format_offset=kind==28?(conditional?0xaddu:0xb27u):(conditional?0xb6du:0xbb5u);
        call(conditional?5:6,0x10727890,{local(0x228),pool(format_offset),gender,gender,100,local(money),local(person),local(opponent)});
        append(7);link(8,local(opponent),2005,club_at(offset+28));player_link(9,10,offset+4,person);
    }
    void transfer_total() const {
        const auto offset=kind==30?0x3d0u:0x3d8u,temporary=kind==30?0x2994u:0x2998u,buffer=kind==30?0x2488u:0x2550u;
        const auto amount=c.u32(record+offset);
        if(compare(c,amount,-1)<=0)return;
        c.put32(local(temporary),amount);
        call(0,0x10009f20,{local(temporary),6});call(1,0x1000b410,{local(buffer),200,c.r(3),c.r(4),6});
        c.set(6,c.u16(record+offset+4));const bool dated=compare_logical(c,c.r(6))!=0;
        call(dated?2:3,0x10727890,{local(0x228),pool(dated?0xbf9:0xc13),local(buffer)});append(4);
    }
    void render() const {
        if(kind<=2)rank();
        else if(kind<=5)attendance();
        else if(kind==6) {
            c.set(5,c.u32(record+0x90));
            if(compare(c,c.r(5),-1)>0) {
                c.set(3,c.u16(record+0x94));const bool dated=compare_logical(c,c.r(3))!=0;
                if(dated){year(0,0x94,0x26f8);call(1,0x10727890,{local(0x228),pool(0x4fa),c.u32(record+0x90),local(0x26f8)});}
                else call(2,0x1077b720,{local(0x228),pool(0x537)});
                append(3);
            }
        }else if(kind<=11)match_record();
        else if(kind<=15)streak();
        else if(kind==18||kind==25||kind==26)player_match();
        else if(kind<=22)player_total();
        else if(kind==23||kind==24||kind==27)career_total();
        else if(kind<=29)transfer();
        else if(kind<=31)transfer_total();
        else {c.set(9,c.u32(record+0x3ec));if(compare(c,c.r(9),-1)>0) {
            call(0,0x100cb590,{c.u32(record+0x3e0),c.u32(record+0x3e4),c.u32(record+0x3e8),local(0x228),200,1});
            append(1);player_link(2,3,0x3e0,0x228);
        }}
    }
};
}

std::uint32_t format(Cpu& c,std::uint32_t manager,std::uint32_t club,std::uint32_t record,
                      std::uint32_t kind,std::uint32_t output) {
    Frame frame(c,0x10014080,0x29e0,{.first_gpr=20});Program p{c};
    keep(c,{{30,output},{22,c.toc(-0x7b4c)},{29,c.toc(slot::clubs)},{20,manager},{28,c.toc(-0x7bbc)},
            {25,club},{26,c.toc(-0x7be0)},{31,record},{23,c.toc(slot::success)},{21,kind},
            {24,c.toc(slot::message)},{27,c.toc(slot::pool)}});
    if(compare(c,output)==0){assertion(c,frame,0x100140d8,0x3a8,0xcc,0x99a);c.set(3,0);return 0;}
    p.call(0x1074c570,0x10014124,{output});p.call(0x1074b830,0x10014130,{output,c.toc(slot::pool)+0x36c});
    if(compare_logical(c,club)==0){assertion(c,frame,0x1001414c,0x388,0xc4,0x9a6);c.set(3,0);return 0;}
    if(compare_logical(c,record)==0){c.set(3,0);return 0;}
    c.set(3,c.u32(club)*index_size);c.set(4,c.u32(manager+28));c.set(21,c.r(4)+c.r(3));
    const auto which=std::uint32_t(std::int32_t(std::int8_t(kind)));
    if(compare_logical(c,which,32)>0 || which==0){assertion(c,frame,0x1001635c,0x110,0x6c,0xecf);c.set(3,0);return 0;}
    Presentation{c,frame,club,record,output,which}.render();c.set(3,1);return 1;
}
}
