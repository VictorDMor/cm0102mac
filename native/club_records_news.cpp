#include "club_records_detail.hpp"
#include "guest_fpu.hpp"
#include <array>

namespace cm::aot::club_records {
using namespace detail;
namespace {
constexpr std::array notice_sites{
#include "club_records_notice_sites.inc"
};
struct NewsPresentation {
    Cpu& c;const Frame& frame;std::uint32_t club,record,output;unsigned kind;Program p{c};
    std::uint32_t local(unsigned distance) const {return frame.caller()-distance;}
    std::uint32_t pool(unsigned offset) const {return c.toc(slot::pool)+offset;}
    std::uint32_t call(unsigned index,std::uint32_t target,std::initializer_list<std::uint32_t> args={}) const {
        return p.call(target,notice_sites[kind][index],args);
    }
    std::uint32_t byte(unsigned offset) const {return std::uint32_t(std::int32_t(c.i8(record+offset)));}
    std::uint32_t half(unsigned offset) const {return std::uint32_t(std::int32_t(c.i16(record+offset)));}
    static std::uint32_t gender(std::uint32_t value) {return std::uint32_t(std::int32_t(std::int8_t(value)));}
    std::uint32_t club_at(unsigned offset) const {return c.u32(c.r(25))+c.u32(record+offset)*0x248;}
    std::uint32_t competition_at(unsigned offset) const {return c.u32(c.r(26))+c.u32(record+offset)*index_size;}
    void subject(unsigned index,unsigned buffer) const {call(index,0x1077dbf0,{local(buffer),club+0x221});}
    void opponent(unsigned index,unsigned offset,unsigned buffer) const {
        c.set(5,c.u32(c.r(25)));call(index,0x1077dbf0,{local(buffer),c.r(5)+c.u32(record+offset)*0x248+0x221});
    }
    void append(unsigned index) const {call(index,0x1074b830,{output,local(0x820)});}
    void link(unsigned index,unsigned buffer,unsigned type,std::uint32_t identity) const {
        call(index,0x1074bab0,{output,local(buffer),type,identity});
    }
    void player(unsigned index,unsigned offset,unsigned buffer,bool include_id=true) const {
        if(include_id)call(index,0x100cb590,{c.u32(record+offset),c.u32(record+offset+4),c.u32(record+offset+8),local(buffer),200,1,c.u32(record+offset+12)});
        else call(index,0x100cb590,{c.u32(record+offset),c.u32(record+offset+4),c.u32(record+offset+8),local(buffer),200,1});
    }
    void player_link(unsigned check,unsigned add,unsigned offset,unsigned buffer) const {
        const auto person=c.u32(c.r(28))+c.u32(record+offset+12)*0x70;
        if(nonzero_byte(c,call(check,0x100cbde0,{person,c.u32(record+offset),c.u32(record+offset+4),c.u32(record+offset+8),record+offset+16})))
            link(add,buffer,2007,c.u32(c.r(28))+c.u32(record+offset+12)*0x70);
    }
    std::uint32_t stage(unsigned text,unsigned agreement,unsigned offset,unsigned buffer,unsigned opponent_offset) const {
        c.set(9,club_at(opponent_offset));c.set(22,c.u8(c.r(9)+0x23b));
        call(text,0x104769d0,{competition_at(offset+8),half(offset+12),half(offset+14),2,local(buffer),200});
        return gender(call(agreement,0x10476ec0,{competition_at(offset+8),half(offset+12),half(offset+14),2}));
    }
    void rank() const {
        const auto offset=kind==1?0u:12u;
        if(compare(c,c.i32(record+offset+4),-1)<=0)return;
        const auto name=kind==1?0x9b0u:0xb40u,ordinal=kind==1?0x4bc4u:0x4bd8u,competition=kind==1?0xa78u:0xc08u;
        subject(0,name);c.set(21,c.u8(club+0x23b));
        const auto value=call(1,0x107467a0,{c.u8(record+offset)+1u,missing});call(2,0x1077dbf0,{local(ordinal),value});
        c.set(5,c.u32(c.r(26)));call(3,0x1077dbf0,{local(competition),c.r(5)+c.u32(record+offset+4)*index_size+0x4d});
        const auto agreement=gender(c.u8(competition_at(offset+4)+0x67));c.put32(frame.at(0x38),local(competition));
        call(4,0x10727890,{local(0x820),pool(kind==1?0xcd2:0xd62),gender(c.r(21)),gender(c.r(21)),agreement,agreement,local(name),local(ordinal)});
        append(5);link(6,name,2005,club);link(7,competition,2000,competition_at(offset+4));
    }
    void attendance() const {
        const auto offset=kind==3?0x18u:kind==4?0x40u:0x68u;
        if(compare(c,c.i32(record+offset),-1)<=0)return;
        const auto name=kind==3?0xcd0u:kind==4?0xf28u:0x1248u,other=kind==3?0xd98u:kind==4?0xff0u:0x1310u,
            stage_buffer=kind==3?0xe60u:kind==4?0x10b8u:0x13d8u;
        unsigned first=0;std::uint32_t amount=c.u32(record+offset);
        if(kind==4){c.put32(local(0x4c6c),amount);call(0,0x10009f20,{local(0x4c6c),0x22});call(1,0x1000b410,{local(0x1180),200,c.r(3),c.r(4),0x22});amount=local(0x1180);first=2;}
        subject(first,name);c.set(21,c.u8(club+0x23b));opponent(first+1,offset+28,other);
        const auto agreement=stage(first+2,first+3,offset+4,stage_buffer,offset+28);
        c.put32(frame.at(0x38),local(name));c.put32(frame.at(0x3c),amount);c.put32(frame.at(0x40),local(other));c.put32(frame.at(0x44),local(stage_buffer));
        call(first+4,0x10727890,{local(0x820),pool(kind==3?0xdf1:kind==4?0xe95:0xf4c),gender(c.r(21)),gender(c.r(21)),gender(c.r(22)),gender(c.r(22)),agreement,agreement});
        append(first+5);link(first+6,name,2005,club);link(first+7,other,2005,club_at(offset+28));link(first+8,stage_buffer,2000,competition_at(offset+12));
    }
    void match_record() const {
        const auto n=kind-7,offset=0x98+n*0x24,name=0x1568+n*600,other=name+200,stage_buffer=other+200,score=0x4c04+n*12;
        if(compare(c,c.i32(record+offset+8),-1)<=0)return;
        subject(0,name);c.set(21,c.u8(club+0x23b));unsigned next=2;
        if(kind<10)call(1,0x1077b720,{local(score),pool(0x53b),byte(offset+(kind==9?29:28)),byte(offset+(kind==9?28:29))});
        else {
            c.set(5,byte(offset+28));c.set(6,byte(offset+29));
            if(compare(c,c.r(5),c.r(6))<0)call(1,0x1077b720,{local(score),pool(0x53b),c.r(6),c.r(5)});
            else call(2,0x1077b720,{local(score),pool(0x53b),c.r(5),c.r(6)});
            next=3;
        }
        opponent(next,offset+24,other);const auto agreement=stage(next+1,next+2,offset,stage_buffer,offset+24);
        unsigned choice=0;
        if(kind<10){c.set(0,byte(offset+16));choice=compare(c,c.r(0),1)==0?0:compare(c,c.r(0),2)==0?1:2;}
        else {c.set(0,byte(offset+29));c.set(3,byte(offset+28));choice=compare(c,c.r(3),c.r(0))>0?0:compare(c,c.r(3),c.r(0))<0?1:2;}
        constexpr unsigned formats[5][3]={{0x1058,0x10eb,0x117b},{0x120b,0x12a7,0x1340},{0x13d9,0x1472,0x1508},{0x159e,0x163c,0x16dd},{0x1779,0x1820,0x18ca}};
        c.put32(frame.at(0x38),local(name));c.put32(frame.at(0x3c),local(score));c.put32(frame.at(0x40),local(other));c.put32(frame.at(0x44),local(stage_buffer));
        call(next+3+choice,0x10727890,{local(0x820),pool(formats[n][choice]),gender(c.r(21)),gender(c.r(21)),gender(c.r(22)),gender(c.r(22)),agreement,agreement});
        append(next+6);link(next+7,name,2005,club);link(next+8,other,2005,club_at(offset+24));link(next+9,stage_buffer,2000,competition_at(offset+8));
        if(nonzero_byte(c,call(next+10,0x1029eb20,{c.u32(c.r(24)),c.u32(record+offset+32),c.u32(record+offset+20),c.u32(record+offset+24),c.u16(record+offset),c.u16(record+offset+2)})))link(next+11,score,2003,c.u32(record+offset+32));
    }
    void streak() const {
        const auto offset=0x14c+(kind-12)*20;
        if(compare(c,c.i32(record+offset),-1)<=0 || compare(c,c.i32(record+offset),1)<=0)return;
        constexpr unsigned names[]{0x2120,0x2378,0x25d0,0x2760},dates[]{0x21e8,0x2440,0x2698,0x2828},formats[]{0x196f,0x19f4,0x1a7b,0x1ae7};
        const auto n=kind-12;subject(0,names[n]);c.set(21,c.u8(club+0x23b));
        call(1,0x10717270,{record+offset+4,local(dates[n]),200,255});
        if(kind<14){call(2,0x10717270,{record+offset+12,local(dates[n]+200),200,255});c.put32(frame.at(0x38),local(dates[n]));c.put32(frame.at(0x3c),local(dates[n]+200));
            call(3,0x10727890,{local(0x820),pool(formats[n]),gender(c.r(21)),gender(c.r(21)),100,100,local(names[n]),c.u32(record+offset)});append(4);link(5,names[n],2005,club);
        }else {call(2,0x10727890,{local(0x820),pool(formats[n]),gender(c.r(21)),gender(c.r(21)),100,local(names[n]),c.u32(record+offset),local(dates[n])});append(3);link(4,names[n],2005,club);}
    }
    void player_total() const {
        struct Shape{unsigned offset,person,name,format;};
        constexpr Shape shapes[]={{0x19c,0x28f0,0x29b8,0x1b5b},{0x1bc,0x2a80,0x2b48,0x1bba},{0x21c,0x2f30,0x2ff8,0x1e3f},
            {0x23c,0x30c0,0x3188,0x1ead},{0x25c,0x3250,0x3318,0x1f4e},{0x27c,0x33e0,0x34a8,0x1fd5},
            {0x29c,0x3570,0x3638,0x2061},{0x2b8,0x3700,0x37c8,0x20dc},{0x35c,0x3ed0,0x3f98,0x2378}};
        const auto& s=shapes[kind<=17?kind-16:kind<=24?kind-17:8];
        if(kind==20){Fpu fp(c);c.set(3,c.toc(slot::doubles));c.set_f(1,c.f32(record+s.offset));fp.fcmp(0,c.f(1),c.f64(c.r(3)+0x48));if(!(c.cr(0)&4))return;}
        else if(compare(c,kind>=23?c.i16(record+s.offset):c.i8(record+s.offset),-1)<=0)return;
        player(0,s.offset+4,s.person);subject(1,s.name);const auto agreement=gender(c.u8(club+0x23b));
        if(kind==20){c.put32(frame.at(0x38),byte(0x258));c.put32(frame.at(0x3c),local(s.name));c.put_f64(frame.at(0x2c),c.f32(record+s.offset));call(2,0x10727890,{local(0x820),pool(s.format),100,agreement,agreement,c.u32(frame.at(0x2c)),c.u32(frame.at(0x30)),local(s.person)});}
        else if(kind==27)call(2,0x10727890,{local(0x820),pool(s.format),agreement,agreement,local(s.person),local(s.name),half(s.offset)});
        else if(kind==22){c.put32(frame.at(0x38),local(s.name));call(2,0x10727890,{local(0x820),pool(s.format),100,agreement,agreement,local(s.person),byte(s.offset),byte(s.offset+1)});}
        else if(kind<=17)call(2,0x10727890,{local(0x820),pool(s.format),100,agreement,agreement,local(s.person),byte(s.offset),local(s.name)});
        else call(2,0x10727890,{local(0x820),pool(s.format),100,agreement,agreement,kind>=23?half(s.offset):byte(s.offset),local(s.person),local(s.name)});
        append(3);link(4,s.name,2005,club);player_link(5,6,s.offset+4,s.person);
    }
    void player_match() const {
        const bool age=kind!=18;const auto offset=age?(kind==25?0x2d4u:0x318u):0x1dcu,match=offset+(age?32:28),
            person=kind==18?0x2c10u:kind==25?0x3890u:0x3bb0u,name=person+200,other=name+200,stage_buffer=other+200,age_buffer=kind==25?0x4c54u:0x4c5cu;
        if(compare(c,age?c.i32(record+match+8):c.i8(record+offset),-1)<=0)return;
        player(0,offset+(age?8:4),person);unsigned next=1;
        if(age){call(1,0x100cd7c0,{record+match,record+offset,local(age_buffer),5});next=2;}
        subject(next,name);c.set(21,c.u8(club+0x23b));opponent(next+1,match+24,other);const auto agreement=stage(next+2,next+3,match,stage_buffer,match+24);
        c.set(0,byte(match+16));const auto choice=compare(c,c.r(0),1)==0?0u:compare(c,c.r(0),2)==0?1u:2u;
        constexpr unsigned goal_formats[]{0x1c20,0x1cd7,0x1d8b},age_formats[]{0x214d,0x2208,0x22c0};
        c.put32(frame.at(0x38),agreement);c.put32(frame.at(0x3c),age?local(person):byte(offset));c.put32(frame.at(0x40),age?local(age_buffer):local(person));
        c.put32(frame.at(0x44),local(name));c.put32(frame.at(0x48),local(other));c.put32(frame.at(0x4c),local(stage_buffer));
        call(next+4+choice,0x10727890,{local(0x820),pool(age?age_formats[choice]:goal_formats[choice]),100,gender(c.r(21)),gender(c.r(21)),gender(c.r(22)),gender(c.r(22)),agreement});
        append(next+7);link(next+8,name,2005,club);link(next+9,other,2005,club_at(match+24));link(next+10,stage_buffer,2000,competition_at(match+8));player_link(next+11,next+12,offset+(age?8:4),person);
    }
    void transfer() const {
        const auto offset=kind==28?0x378u:0x3a4u,person=kind==28?0x4060u:0x4380u,name=kind==28?0x4128u:0x4510u,
            other=kind==28?0x41f0u:0x4448u,money=kind==28?0x42b8u:0x45d8u,temporary=kind==28?0x4c70u:0x4c74u;
        if(compare(c,c.i32(record+offset),-1)<=0)return;
        player(0,offset+4,person);subject(1,name);c.set(21,c.u8(club+0x23b));opponent(2,offset+28,other);
        c.set(6,c.u32(c.r(25)));c.set(5,club_at(offset+28));c.set(22,c.u8(c.r(5)+0x23b));c.put32(local(temporary),c.u32(record+offset));call(3,0x10009f20,{local(temporary),6});call(4,0x1000b410,{local(money),200,c.r(3),c.r(4),6});
        const auto conditional=compare(c,c.i8(record+offset+40))!=0;
        c.put32(frame.at(0x38),local(person));c.put32(frame.at(0x3c),local(kind==28?other:name));c.put32(frame.at(0x40),local(kind==28?name:other));
        const auto first=gender(c.r(kind==28?22:21)),second=gender(c.r(kind==28?21:22));
        call(conditional?5:6,0x10727890,{local(0x820),pool(kind==28?(conditional?0x23ed:0x247d):(conditional?0x24ff:0x258b)),100,first,first,second,second,local(money)});
        append(7);link(8,name,2005,club);link(9,other,2005,club_at(offset+28));player_link(10,11,offset+4,person);
    }
    void transfer_total() const {
        const auto offset=kind==30?0x3d0u:0x3d8u,name=kind==30?0x46a0u:0x4830u,money=name+200,temporary=kind==30?0x4c78u:0x4c7cu;
        if(compare(c,c.i32(record+offset),-1)<=0)return;
        subject(0,name);c.set(21,c.u8(club+0x23b));c.put32(local(temporary),c.u32(record+offset));call(1,0x10009f20,{local(temporary),6});call(2,0x1000b410,{local(money),200,c.r(3),c.r(4),6});
        call(3,0x10727890,{local(0x820),pool(kind==30?0x2609:0x2660),gender(c.r(21)),gender(c.r(21)),local(money),local(name)});append(4);link(5,name,2005,club);
    }
    void player_of_year() const {
        c.set(9,c.u32(record+0x3ec));if(compare(c,c.r(9),-1)<=0)return;
        player(0,0x3e0,0x49c0,false);call(1,0x1077dbf0,{local(0x4a88),club+0x1ee});c.set(21,c.u8(club+0x242));
        call(2,0x10727890,{local(0x4b50),pool(0x26ba)});call(3,0x10727890,{local(0x820),pool(0x26cd),gender(c.r(21)),gender(c.r(21)),100,local(0x49c0),local(0x4a88),local(0x4b50)});
        append(4);link(5,0x4a88,2005,club);link(6,0x4b50,2022,club);player_link(7,8,0x3e0,0x49c0);
    }
    void render() const {
        if(kind<=2)rank();else if(kind<=5)attendance();
        else if(kind==6){if(compare(c,c.i32(record+0x90),-1)>0){subject(0,0x14a0);const auto g=gender(c.u8(club+0x23b));call(1,0x10727890,{local(0x820),pool(0xfef),g,g,local(0x14a0),c.u32(record+0x90)});append(2);link(3,0x14a0,2005,club);}}
        else if(kind<=11)match_record();else if(kind<=15)streak();else if(kind==18||kind==25||kind==26)player_match();
        else if(kind<=27)player_total();else if(kind<=29)transfer();else if(kind<=31)transfer_total();else player_of_year();
    }
};
}
std::uint32_t format_notice(Cpu& c,std::uint32_t message,std::uint32_t details,std::uint32_t output) {
    Frame frame(c,0x100163f0,0x4cd0,{.first_gpr=20});Program p{c};
    keep(c,{{24,c.toc(-0x7b4c)},{20,message},{25,c.toc(slot::clubs)},{30,details},{26,c.toc(-0x7bbc)},{31,output},
            {21,c.toc(-0x7bb8)},{28,c.toc(-0x7be0)},{22,c.toc(slot::success)},{23,c.toc(slot::message)},{29,c.toc(slot::pool)}});
    constexpr std::pair<unsigned,std::uint32_t> constructors[]={{0x48b0,0x100196e0},{0x48d8,0x100196a0},{0x4900,0x10019660},{0x4930,0x10019630},
        {0x4954,0x10019600},{0x4978,0x100195d0},{0x499c,0x100195a0},{0x49c0,0x10019570},{0x49e4,0x10019530},{0x49f8,0x100194f0},
        {0x4a0c,0x100194b0},{0x4a20,0x10019470},{0x4a34,0x10019430},{0x4a54,0x100193f0},{0x4a74,0x100193b0},{0x4ab4,0x10019370},
        {0x4ad4,0x10019330},{0x4af4,0x100192f0},{0x4b14,0x100192b0},{0x4b34,0x10019270},{0x4b50,0x10019230},{0x4b6c,0x100191f0},
        {0x4bb0,0x100191b0},{0x4bf4,0x10019170},{0x4c10,0x10019130},{0x4c3c,0x100190f0},{0x4c78,0x100190b0}};
    auto back=0x10016434u;for(const auto& [offset,entry]:constructors){p.call(entry,back,{frame.at(offset)});back+=8;}
    const auto output_missing=compare_logical(c,output)==0;c.set(27,frame.at(0x4898));
    if(output_missing){assertion(c,frame,0x10016524,0x160,0x94,0xee7);c.set(3,0);return 0;}
    p.call(0x1074c570,0x10016570,{output});p.call(0x1074b830,0x1001657c,{output,c.toc(slot::pool)+0xc1f});
    if(compare_logical(c,message)==0){assertion(c,frame,0x10016598,0x140,0x8c,0xef3);c.set(3,0);return 0;}
    p.call(0x1001cb30,0x100165f0,{message,frame.at(0x68),c.r(27),frame.at(0x50)});
    c.set(4,c.u32(frame.at(0x68)));bool valid=compare(c,c.r(4))>=0;
    if(valid){c.set(3,c.u32(c.toc(-0x7b50)));valid=compare(c,c.r(4),c.r(3)-1)<=0;}
    if(!valid){assertion(c,frame,0x10016624,0x120,0x84,0xefe);c.set(3,0);return 0;}
    c.set(3,c.r(4)*0x248);c.set(4,c.u32(c.r(25)));const bool full=compare(c,std::int8_t(details))!=0;c.set(30,c.r(4)+c.r(3));
    const auto club=c.r(30);const auto kind=c.u8(frame.at(0x50));
    if(!full){
        const bool award=compare(c,kind,32)==0;
        auto form=0xc38u;
        if(award)p.call(0x10727890,0x10016694,{frame.at(0x43e8),c.toc(slot::pool)+0xc20});
        else {const auto national=nonzero_byte(c,p.call(0x100c7d00,0x100166f8,{club}));form=national?0xc8d:0xcad;}
        const auto g=NewsPresentation::gender(c.u8(club+0x23b));
        if(award)p.call(0x10727890,0x100166b4,{c.r(21),c.toc(slot::pool)+form,g,g,club+0x221,frame.at(0x43e8)});
        else p.call(0x10727890,form==0xc8d?0x1001671c:0x1001673c,{c.r(21),c.toc(slot::pool)+form,g,g,club+0x221});
        p.call(0x1074b830,award?0x100166c0:0x10016748,{output,c.r(21)});
        p.call(0x1074bab0,award?0x100166d4:0x1001675c,{output,club+0x221,2005,club});
        if(award)p.call(0x1074bab0,0x100166e8,{output,frame.at(0x43e8),2022,club});
    }else {
        const auto which=std::uint32_t(std::int32_t(std::int8_t(kind)));
        if(compare_logical(c,which,32)>0 || which==0){assertion(c,frame,0x1001905c,0xd8,0x6c,0x14a7);c.set(3,0);return 0;}
        NewsPresentation{c,frame,club,c.r(27),output,which}.render();
    }
    c.set(3,1);return 1;
}
}
