#include "club_records_detail.hpp"
#include "guest_fpu.hpp"
#include <algorithm>
#include <array>

namespace cm::aot::club_records {
using namespace detail;
namespace {
void match_error(Cpu& c,const Frame& frame,std::uint32_t site,unsigned name,unsigned extension,unsigned line,bool reload_flag=false) {
    const auto pool=c.toc(slot::pool);
    report_assertion(c,{site,pool+0x59,pool+0x6a,pool+0x77,frame.at(name),frame.at(extension),line},[&](std::uint32_t target,std::uint32_t back){c.call(target,back+(target==0x1077b720?4:target==0x10769b60?8:0));});
    if(reload_flag)c.set(4,c.toc(slot::success));
}
void date_copy(Cpu& c,std::uint32_t to,std::uint32_t from) {
    c.put16(to+2,c.u16(from+2));c.put16(to,c.u16(from));c.put8(to+4,c.u8(from+4));
}
void result_copy(Cpu& c,std::uint32_t to,std::uint32_t from,bool home=false) {
    date_copy(c,to,from);c.put32(to+8,c.u32(from+8));c.put16(to+12,c.u16(from+12));c.put16(to+14,c.u16(from+14));
    c.put8(to+16,home?1:c.u8(from+16));c.put32(to+20,c.u32(from+20));c.put32(to+24,c.u32(from+24));
    c.put8(to+28,c.u8(from+28));c.put8(to+29,c.u8(from+29));c.put32(to+32,c.u32(from+32));
}
struct MatchUpdate {
    Cpu& c;const Frame& frame;std::uint32_t manager,match,statistics;Program p{c};
    std::uint32_t club() const {return c.r(21);}
    std::uint32_t season() const {return c.r(27);}
    std::uint32_t modern() const {return c.r(28);}
    std::uint32_t index() const {return c.r(19);}
    std::uint32_t result() const {return frame.at(0x140);}
    bool flag(std::uint32_t target,std::uint32_t back,std::initializer_list<std::uint32_t> args) const {return nonzero_byte(c,p.call(target,back,args));}
    bool available(unsigned kind,std::uint32_t back) const {return flag(0x10013cc0,back,{manager,kind,club(),1});}
    bool pending(unsigned kind,std::uint32_t back) const {return flag(0x1001be60,back,{manager,c.u32(club()),kind});}
    void queue(unsigned kind,std::uint32_t check,std::uint32_t append) const {if(!pending(kind,check))p.call(0x1001bd60,append,{manager,c.u32(club()),kind});}
    bool improves(std::uint32_t address,std::uint32_t value,bool lower=false) const {
        c.set(3,c.u32(address));return compare(c,c.r(3),-1)==0 || (lower?compare(c,value,c.r(3))<0:compare(c,value,c.r(3))>0);
    }
    void attendance() const {
        struct Shape{unsigned kind,offset,input;std::uint32_t available,check,append;bool lower;};
        constexpr Shape shapes[]={{3,0x18,0x34,0x1000ee84,0x1000ef5c,0x1000ef74,false},
            {4,0x40,0x38,0x1000eff4,0x1000f0cc,0x1000f0e4,false},{5,0x68,0x34,0x1000f164,0x1000f23c,0x1000f254,true}};
        for(const auto& s:shapes) {
            if(!available(s.kind,s.available) || compare(c,c.i8(result()+16),1)!=0 || compare(c,c.i32(statistics+s.input))<=0)continue;
            const auto value=c.u32(statistics+s.input);
            if(!improves(season()+s.offset,value,s.lower))continue;
            c.put32(season()+s.offset,value);result_copy(c,season()+s.offset+4,result(),true);
            if(improves(modern()+s.offset,value,s.lower)) {
                if(compare(c,c.i16(index()+0x12),49)>0)queue(s.kind,s.check,s.append);
                c.put32(modern()+s.offset,c.u32(statistics+s.input));result_copy(c,modern()+s.offset+4,result());
            }
        }
    }
    void scores() const {
        struct Shape{unsigned kind,offset;std::uint32_t available,competition,first,second,prior,check,append;};
        constexpr Shape shapes[]={{7,0x98,0x1000f2d4,0,0x1000f30c,0x1000f38c,0,0x1000f3c0,0x1000f3d8},
            {8,0xbc,0x1000f44c,0x1000f45c,0x1000f494,0x1000f514,0x1000f548,0x1000f560,0x1000f578},
            {9,0xe0,0x1000f5ec,0,0x1000f624,0x1000f6a4,0,0x1000f6d8,0x1000f6f0},
            {10,0x104,0x1000f764,0,0x1000f79c,0x1000f81c,0,0x1000f850,0x1000f868},
            {11,0x128,0x1000f8dc,0x1000f8ec,0x1000f924,0x1000f9a4,0x1000f9d8,0x1000f9f0,0x1000fa08}};
        for(const auto& s:shapes) {
            if(!available(s.kind,s.available))continue;
            if(s.competition && !flag(0x104646d0,s.competition,{c.u32(match)}))continue;
            const auto own=c.i8(result()+28),other=c.i8(result()+29);
            if(s.kind<=8 ? compare(c,own,other)<=0 : s.kind==9 ? compare(c,own,other)>=0 : compare(c,own+other)<=0)continue;
            const auto comparator=s.kind<=8?0x1030d860u:s.kind==9?0x1030d960u:0x1030da60u;
            if(compare(c,c.i32(season()+s.offset+8),-1)!=0 && !flag(comparator,s.first,{result(),season()+s.offset}))continue;
            result_copy(c,season()+s.offset,result());
            if(compare(c,c.i32(modern()+s.offset+8),-1)!=0 && !flag(comparator,s.second,{result(),modern()+s.offset}))continue;
            const bool noteworthy=compare(c,s.kind<=8?own-other:s.kind==9?other-own:own+other,s.kind<=9?5:7)>0;
            if(noteworthy && (!s.prior || !pending(s.kind-1,s.prior)))queue(s.kind,s.check,s.append);
            result_copy(c,modern()+s.offset,result());
        }
    }
    void streak(unsigned kind,unsigned row_offset,std::uint32_t available_back,std::uint32_t check_back,std::uint32_t append_back,unsigned temporary) const {
        const auto row=index()+row_offset,offset=0x14c+(kind-12)*20;
        c.put32(row,c.u32(row)+1);
        if(compare(c,c.i32(row),1)==0)date_copy(c,row+4,c.toc(-0x7b80));
        c.set(7,c.toc(-0x7b80));date_copy(c,row+12,c.r(7));
        if(!available(kind,available_back) || !improves(season()+offset,c.u32(row)))return;
        c.put32(season()+offset,c.u32(row));date_copy(c,season()+offset+4,row+4);date_copy(c,season()+offset+12,row+12);
        if(!improves(modern()+offset,c.u32(row)))return;
        if(compare(c,c.i32(row),kind<14?5:11)>0) {
            const auto date=c.u32(modern()+offset+4);c.put32(frame.at(temporary),date);c.put32(frame.at(temporary+4),c.u32(modern()+offset+8));
            if((compare(c,std::int16_t(date),c.i16(row+6))!=0 || compare(c,std::int16_t(date>>16),c.i16(row+4))!=0))queue(kind,check_back,append_back);
        }
        c.put32(modern()+offset,c.u32(row));date_copy(c,modern()+offset+4,row+4);date_copy(c,modern()+offset+12,row+12);
    }
    void team_records() const {
        c.set(18,p.call(0x10192610,0x1000edfc,{club()}));c.put8(index()+16,c.u8(index()+16)+1);
        p.call(0x1030d4a0,0x1000ee1c,{match,club(),result()});
        if(compare(c,c.i8(result()+16),1)==0) {
            c.put8(index()+17,c.u8(index()+17)+1);c.put16(index()+18,c.u16(index()+18)+1);c.put32(index()+20,c.u32(index()+20)+c.u32(statistics+0x34));
            c.put32(season()+0x90,std::uint32_t(divide_signed<4>(c.u32(index()+20),std::uint32_t(std::int32_t(c.i8(index()+17))))));c.put16(season()+0x94,0);
        }
        attendance();scores();
        if(compare(c,c.i8(result()+28),c.i8(result()+29))>0){streak(12,0x18,0x1000fae8,0x1000fbb4,0x1000fbcc,0x70);c.put32(index()+0x2c,0);c.put32(index()+0x54,0);}
        else if(compare(c,c.i8(result()+28),c.i8(result()+29))<0){streak(13,0x2c,0x1000fc80,0x1000fd4c,0x1000fd64,0x68);c.put32(index()+0x18,0);c.put32(index()+0x40,0);}
        if(compare(c,c.i8(result()+28),c.i8(result()+29))>=0){streak(14,0x40,0x1000fe28,0x1000fef4,0x1000ff0c,0x60);c.put32(index()+0x2c,0);}
        if(compare(c,c.i8(result()+28),c.i8(result()+29))<=0){streak(15,0x54,0x1000ffcc,0x10010098,0x100100b0,0x58);c.put32(index()+0x18,0);}
    }
    void player_records() const;
};

void MatchUpdate::player_records() const {
    Fpu fp(c);
    const auto integer=[&](double value,unsigned at){c.put64(frame.at(at),fp.fctiwz(0,value));return c.u32(frame.at(at+4));};
    const auto number=[&](std::int32_t value,unsigned at){c.put32(frame.at(at),0x43300000);c.put32(frame.at(at+4),std::uint32_t(value)^0x80000000);return fp.fsub(c.f64(frame.at(at)),c.f64(c.r(26)+0x38));};
    const auto power=[&](unsigned value,unsigned exponent,std::uint32_t back,unsigned at){c.set(3,value^0x80000000);c.set_f(1,number(std::int32_t(value),at));c.set_f(2,c.f64(c.r(26)+exponent));p.call(0x10780fc0,back);c.set(2,c.u32(frame.at(0x14)));return integer(c.f(1),back==0x10010788?0x184:0x18c);};
    const auto total=[&](unsigned category,std::uint32_t back,unsigned at){c.set_f(1,c.f32(c.toc(slot::floats)));p.call(0x10303980,back,{c.u32(c.r(13)),c.r(15),category});return integer(c.f(1),at);};
    const auto player_stats=[&](std::uint32_t to,std::uint32_t back){p.call(0x1030d6e0,back,{c.r(16),to});};
    const auto new_player_year=[&](unsigned offset,unsigned year){return compare(c,c.i32(modern()+offset),c.i32(c.r(16)))!=0 || compare_logical(c,c.u16(modern()+year),c.r(18)&0xffff)!=0;};
    const auto byte_improves=[&](std::uint32_t address,unsigned value){c.set(3,std::uint32_t(std::int32_t(c.i8(address))));return compare(c,c.r(3),-1)==0 || compare(c,value&0xff,c.r(3))>0;};
    // Each shape keeps the separate season/modern copy and notification sites.
    struct TotalShape{unsigned kind,offset,year,temporary,bonus;std::uint32_t available,season_copy,check,append,modern_copy,prior;};
    const auto update_total=[&](const TotalShape& shape,unsigned value){
        if(!available(shape.kind,shape.available) || !byte_improves(season()+shape.offset,value))return;
        c.put8(frame.at(shape.temporary),value);c.put8(season()+shape.offset,value);player_stats(season()+shape.offset+4,shape.season_copy);c.put16(season()+shape.year,0);
        if(!byte_improves(modern()+shape.offset,value))return;
        bool noteworthy=true;
        if(shape.kind==21)noteworthy=compare(c,c.i8(season()+shape.offset),3)>0;
        if(noteworthy)noteworthy=compare(c,c.i16(index()+0x68),3)>=0;
        if(noteworthy && shape.kind==16)noteworthy=compare_logical(c,value&0xff,3)>0;
        if(noteworthy && new_player_year(shape.offset+16,shape.year) && (!shape.prior || !pending(shape.kind-1,shape.prior))) {
            queue(shape.kind,shape.check,shape.append);c.set(31,c.r(31)+shape.bonus);
        }
        c.put8(modern()+shape.offset,c.u8(frame.at(shape.temporary)));player_stats(modern()+shape.offset+4,shape.modern_copy);c.put16(modern()+shape.year,c.r(18));
    };
    do {
        c.set(31,0);const auto position=c.u32(frame.at(0x19c));
        c.set(17,statistics+c.u32(frame.at(0x1ac))+c.u32(frame.at(0x1b0))+0x194);
        bool used=compare(c,position,10)<=0 || compare(c,c.i8(c.r(17)+0x32),-1)!=0;
        if(used && compare(c,c.i32(c.r(17)),-1)!=0 && (c.set(5,c.u32(c.r(17)+4)),compare(c,c.r(5),-1)!=0)) {
            c.set(5,c.u32(c.r(17)+4));c.set(4,c.u32(c.toc(-0x7be0)));c.set(16,c.r(4)+c.u32(c.r(17))*0x70);compare(c,c.r(16));
            c.set(3,c.u32(c.toc(-0x7b38)));c.set(14,c.r(3)+c.r(5)*0x48);
            if(c.r(16)!=0)c.set(4,c.u32(c.r(16)));
            if(c.r(16)==0 || compare(c,c.r(4),c.i32(c.toc(slot::staff_count)))<0) {
                if(flag(0x100c7d00,0x10010190,{club()}) && !flag(0x100ce250,0x100101a0,{club()}) && compare(c,c.i8(match+0x40))!=0 && available(27,0x100101c8)) {
                    c.set(3,std::uint32_t(std::int32_t(c.i16(modern()+0x35c))));
                    if(compare(c,c.r(3),-1)==0 || compare(c,c.u8(c.r(16)+0x5e),c.r(3))>0) {
                        if(compare(c,c.i32(modern()+0x36c),c.i32(c.r(16)))!=0)queue(27,0x10010208,0x10010220);
                        c.put16(modern()+0x35c,c.u8(c.r(16)+0x5e));player_stats(modern()+0x360,0x10010234);
                    }
                }
                if(!flag(0x10494d20,0x10010244,{c.u32(c.toc(-0x7b54)),c.u32(match+20)})) {
                    const bool national=std::int8_t(p.call(0x100c7d00,0x10010254,{club()}))!=0;
                    c.set(15,p.call(0x103054e0,0x10010274,{c.u32(c.r(13)),c.r(14),national?5u:6u}));
                    if(compare(c,c.r(15))==0) {
                        const auto competition=c.u32(match+20);compare_logical(c,competition);
                        p.call(0x1077b720,0x100102ac,{c.toc(-0x7bb8),c.toc(slot::pool)+0x8e,club()+0x1ee,competition?competition+0x1a:c.toc(slot::pool)+0x7d});
                        match_error(c,frame,0x100102c4,0xe0,0xc8,0x31a);
                    }
                    if(compare_logical(c,c.u8(c.r(17)+0x35),7)>0)c.set(31,power(c.u8(c.r(17)+0x35)-6,8,0x10010340,0x184));
                    if(compare_logical(c,c.u8(c.r(17)+0x21))!=0) {
                        const auto bonus=power(c.u8(c.r(17)+0x21)+1,8,0x10010384,0x184);c.set(31,c.r(31)+bonus);
                        c.set(20,total(2,0x100103b0,0x17c));
                        update_total({16,0x19c,0x1b8,0x1a9,100,0x100103d0,0x1001040c,0x10010478,0x10010490,0x100104a8,0},c.r(20));
                        if(flag(0x104646d0,0x100104b4,{c.u32(match)})) {
                            c.set_f(1,c.f32(c.toc(slot::floats)));p.call(0x10303560,0x100104d8,{c.u32(c.r(13)),c.r(14),2,2});c.set(20,integer(c.f(1),0x17c));
                            update_total({17,0x1bc,0x1d8,0x1a8,100,0x100104f8,0x10010534,0x100105ac,0x100105c4,0x100105dc,0x10010594},c.r(20));
                        }
                        if(available(18,0x100105f4) && byte_improves(season()+0x1dc,c.u8(c.r(17)+0x21))) {
                            c.put8(season()+0x1dc,c.u8(c.r(17)+0x21));player_stats(season()+0x1e0,0x1001062c);result_copy(c,season()+0x1f8,result());
                            if(byte_improves(modern()+0x1dc,c.u8(c.r(17)+0x21))) {
                                if(compare_logical(c,c.u8(c.r(17)+0x21),4)>=0){queue(18,0x100106c4,0x100106dc);c.set(31,c.r(31)+50);}
                                c.put8(modern()+0x1dc,c.u8(c.r(17)+0x21));player_stats(modern()+0x1e0,0x100106f4);result_copy(c,modern()+0x1f8,result());
                            }
                        }
                    }
                    if(compare_logical(c,c.u8(c.r(17)+0x27))!=0) {
                        const auto bonus=power(c.u8(c.r(17)+0x27)+1,0x10,0x10010788,0x17c);c.set(31,c.r(31)+bonus);c.set(20,total(5,0x100107b4,0x18c));
                        update_total({19,0x21c,0x238,0x1a7,50,0x100107d4,0x10010810,0x10010870,0x10010888,0x100108a0,0},c.r(20));
                    }
                    c.set(4,c.u32(c.r(17)+4));if(compare(c,c.r(4),c.i32(statistics+0x3c))==0) {
                        c.set(20,total(12,0x100108d0,0x17c));c.set(31,c.r(31)+30);
                        update_total({21,0x25c,0x278,0x1a6,50,0x100108f4,0x10010930,0x100109a0,0x100109b8,0x100109d0,0},c.r(20));
                    }
                    if(compare_logical(c,c.u8(c.r(17)+0x2c))!=0 || compare_logical(c,c.u8(c.r(17)+0x2d))!=0) {
                        const auto stats=c.r(15);c.set(15,total(10,0x10010a04,0x17c));
                        c.set_f(1,c.f32(c.toc(slot::floats)));p.call(0x10303980,0x10010a28,{c.u32(c.r(13)),stats,11});c.set(20,integer(c.f(1),0x184));
                        c.set(7,c.r(15)&0xff);c.put32(frame.at(0x198),(c.r(15)&0xff)+(c.r(20)&0xff)*3);
                        if(available(22,0x10010a5c)) {
                            const auto better=[&](std::uint32_t row){c.set(3,std::uint32_t(std::int32_t(c.i8(row))));return compare(c,c.r(3),-1)==0 || compare(c,c.i32(frame.at(0x198)),c.i8(row)+c.i8(row+1)*3)>0;};
                            if(better(season()+0x27c)) {
                                c.put8(frame.at(0x1a5),c.r(15));c.put8(season()+0x27c,c.r(15));c.put8(frame.at(0x1a4),c.r(20));c.put8(season()+0x27d,c.r(20));player_stats(season()+0x280,0x10010ab4);c.put16(season()+0x298,0);
                                if(better(modern()+0x27c)) {
                                    if(compare(c,c.i16(index()+0x68),3)>=0 && compare_logical(c,c.r(15)&0xff,2)>=0 && compare_logical(c,c.r(20)&0xff,2)>=0 && new_player_year(0x28c,0x298))queue(22,0x10010b3c,0x10010b54);
                                    c.put8(modern()+0x27c,c.u8(frame.at(0x1a5)));c.put8(modern()+0x27d,c.u8(frame.at(0x1a4)));player_stats(modern()+0x280,0x10010b70);c.put16(modern()+0x298,c.r(18));
                                }
                            }
                        }
                    }
                    if(flag(0x104646d0,0x10010b7c,{c.u32(match)})) {
                        struct Career{unsigned kind,offset,category;std::uint32_t total,available,season,check,append,modern;};
                        constexpr Career careers[]={{23,0x29c,1,0x10010b94,0x10010bb4,0x10010be4,0x10010c28,0x10010c40,0x10010c54},
                            {24,0x2b8,2,0x10010c70,0x10010c90,0x10010cc0,0x10010d04,0x10010d1c,0x10010d30}};
                        for(const auto& s:careers) {
                            if(s.kind==24 && compare(c,c.i8(c.r(17)+0x21))==0)continue;
                            p.call(0x10306a80,s.total,{c.u32(c.r(13)),c.r(14),s.category});const auto value_reg=s.kind==23?15u:14u;c.set(value_reg,integer(c.f(1),0x17c));
                            if(!available(s.kind,s.available))continue;
                            const auto value=std::int16_t(c.r(value_reg));
                            if(compare(c,c.i16(season()+s.offset),-1)!=0 && compare(c,value,c.i16(season()+s.offset))<=0)continue;
                            c.put16(season()+s.offset,value);player_stats(season()+s.offset+4,s.season);
                            if(compare(c,c.i16(modern()+s.offset),-1)!=0 && compare(c,value,c.i16(modern()+s.offset))<=0)continue;
                            if(compare(c,c.i16(index()+0x68),3)>=0 && compare(c,c.i32(modern()+s.offset+16),c.i32(c.r(16)))!=0){queue(s.kind,s.check,s.append);c.set(31,c.r(31)+100);}
                            c.put16(modern()+s.offset,value);player_stats(modern()+s.offset+4,s.modern);
                        }
                    }
                    // Compare age at the recorded match, then age today. The
                    // original compares unsigned years before signed day parts.
                    struct Age{unsigned kind,offset,old_season,new_season,old_modern,new_modern,announcement;std::uint32_t available,season_age,new_age,season_copy,modern_age,current_age,check_age,check,append,modern_copy;};
                    constexpr Age ages[]={{25,0x2d4,0xb8,0xc0,0xa8,0xb0,0xa0,0x10010d44,0x10010d6c,0x10010d90,0x10010e00,0x10010e80,0x10010ea4,0x10010f10,0x10010f3c,0x10010f54,0x10010f78},
                        {26,0x318,0x90,0x98,0x80,0x88,0x78,0x10010fec,0x10011014,0x10011038,0x100110a8,0x10011128,0x1001114c,0x100111b8,0x100111e4,0x100111fc,0x10011254}};
                    for(const auto& s:ages) {
                        if(!available(s.kind,s.available))continue;
                        const auto age=[&](std::uint32_t date,std::uint32_t birth,unsigned out,std::uint32_t back){p.call(0x10717dd0,back,{frame.at(out),date,c.u32(birth),c.u32(birth+4)});};
                        const auto better=[&](std::uint32_t row,unsigned old_at,unsigned current_at,std::uint32_t old_back,std::uint32_t current_back,unsigned scratch){
                            if(compare(c,c.i32(row+40),-1)==0)return true;
                            age(row+32,row,old_at,old_back);c.put32(frame.at(scratch),c.u32(frame.at(old_at)));c.put32(frame.at(scratch+4),c.u32(frame.at(old_at+4)));
                            age(c.toc(-0x7b80),c.r(16)+0x34,current_at,current_back);
                            const auto old_year=c.u16(frame.at(scratch+2)),new_year=c.u16(frame.at(current_at+2));
                            const auto order=compare_logical(c,old_year,new_year);
                            bool improved=order?(s.kind==25?order>0:order<0):(s.kind==25?c.i16(frame.at(current_at))<c.i16(frame.at(scratch)):c.i16(frame.at(scratch))<c.i16(frame.at(current_at)));
                            compare(c,improved);return improved;
                        };
                        if(!better(season()+s.offset,s.old_season,s.new_season,s.season_age,s.new_age,s.kind==25?0x50:0x40))continue;
                        date_copy(c,season()+s.offset,c.r(16)+0x34);player_stats(season()+s.offset+8,s.season_copy);result_copy(c,season()+s.offset+32,result());
                        if(!better(modern()+s.offset,s.old_modern,s.new_modern,s.modern_age,s.current_age,s.kind==25?0x48:0x38))continue;
                        if(compare(c,c.i16(index()+0x68),3)>=0) {
                            age(c.toc(-0x7b80),c.r(16)+0x34,s.announcement,s.check_age);const auto years=c.u16(frame.at(s.announcement+2));
                            if((s.kind==25?compare_logical(c,years,20)<0:compare_logical(c,years,35)>0) && compare(c,c.i32(modern()+s.offset+20),c.i32(c.r(16)))!=0) {
                                queue(s.kind,s.check,s.append);
                                if(s.kind==26)c.set(31,integer(fp.fmul(c.f64(c.r(26)+0x18),number(std::int16_t(c.r(31)),0x17c)),0x184));
                            }
                        }
                        date_copy(c,modern()+s.offset,c.r(16)+0x34);player_stats(modern()+s.offset+8,s.modern_copy);result_copy(c,modern()+s.offset+32,result());
                    }
                    // Recognition draws advance once for each eligible person,
                    // with the original wrap/reseed order before reading a draw.
                    if(compare(c,std::int16_t(c.r(31)),40)>=0 && (compare_logical(c,c.r(16))==0 || compare(c,c.i32(c.r(16)),c.i32(c.toc(slot::staff_count)))<0) && compare(c,c.i16(frame.at(0x196)),5)>=0) {
                        c.set(3,c.u32(c.toc(-0x7b40)));bool eligible=compare_logical(c,c.r(3))==0;
                        if(!eligible)eligible=!flag(0x103f43b0,0x10011304,{c.r(3),c.r(16),0});
                        if(eligible) {
                            if(compare(c,std::int16_t(c.r(31)))!=0) {
                                const auto scale=[&](unsigned first,unsigned second,unsigned result){c.set(4,std::uint32_t(std::int32_t(std::int16_t(c.r(31)))));const auto rep=number(c.i16(frame.at(0x196)),first),score=number(std::int16_t(c.r(31)),second);const auto factor=fp.fadd(c.f64(c.r(26)+0x20),fp.fdiv(rep,c.f64(c.r(26)+0x28)));c.set(31,integer(fp.fmul(score,factor),result));};
                                if(compare(c,c.i8(frame.at(0x195)))!=0)scale(0x184,0x17c,0x18c);
                                if(compare(c,c.i8(frame.at(0x194)))!=0)scale(0x16c,0x174,0x164);
                                if(compare(c,c.i8(frame.at(0x194)))==0 && compare(c,c.i8(frame.at(0x195)))==0) {
                                    // The reseed call observes these live arguments even
                                    // when the reputation adjustment is skipped.
                                    c.set(5,std::uint32_t(std::int32_t(c.i16(club()+0x1ec))));
                                    c.set(4,std::uint32_t(std::int32_t(c.i16(frame.at(0x196)))));
                                    if(compare(c,c.r(4),std::int32_t(c.r(5))/500)<0) {
                                    const auto ratio=std::int32_t(divide_signed<4>(std::uint32_t(c.i16(frame.at(0x196))*500),c.r(5)));
                                    const auto factor=number(ratio,0x164);fp.fcmp(0,c.f64(c.r(26)+0x30),factor);
                                    double bounded=c.f64(c.r(26)+0x30);
                                    if(c.cr(0)&(8|2)){fp.fcmp(0,c.f64(c.r(26)+0x20),factor);bounded=(c.cr(0)&(4|2))?factor:c.f64(c.r(26)+0x20);}
                                    c.set(31,integer(fp.fmul(number(std::int16_t(c.r(31)),0x17c),bounded),0x184));
                                    }
                                }
                            }
                            const auto draw=[&](unsigned modulus,std::uint32_t back){
                                c.set(3,c.u32(c.r(25))+2);const auto wrapped=compare_logical(c,c.r(3),c.u32(c.r(24)))>0;c.put32(c.r(25),c.r(3));
                                if(wrapped){c.put32(c.r(25),c.r(23));p.call(0x1077b770,back);c.put32(c.r(22),c.r(3)&0xffff);}
                                c.set(5,c.u32(c.r(25)));const auto value=c.u16(c.r(5))+c.u32(c.r(22));c.set(4,value);return std::uint32_t(std::int32_t(value)%std::int32_t(modulus))+250;
                            };
                            auto threshold=draw(4000,0x100114d0);c.set(15,std::uint32_t(std::int32_t(std::int16_t(c.r(31)))));
                            if(compare(c,c.r(15),threshold)>0)p.call(0x100d9960,0x10011528,{c.u32(club()+0x80),c.r(16),1});
                            threshold=draw(4000,0x10011548);if(compare(c,c.r(15),threshold)>0)p.call(0x100d9960,0x1001159c,{c.u32(club()+0x90),c.r(16),1});
                            threshold=draw(4000,0x100115bc);if(compare(c,c.r(15),threshold)>0)p.call(0x100d95e0,0x10011610,{club(),c.r(16),1});
                            c.set(17,0);c.set(14,0);
                            do {c.set(20,c.u32(club()+c.r(14)+0x98));
                                if(compare_logical(c,c.r(20))!=0 && compare_logical(c,c.u32(c.r(20)+20))!=0 && compare_logical(c,c.r(20),c.r(16))!=0) {
                                    threshold=draw(7000,0x10011660);if(compare(c,c.r(15),threshold)>0)p.call(0x100d9960,0x100116b4,{c.r(20),c.r(16),1});
                                }
                                c.set(17,c.r(17)+1);c.set(14,c.r(14)+4);compare(c,c.r(17),50);
                            }while(c.r(17)<50);
                        }
                    }
                }
            }
        }
        c.set(3,c.u32(frame.at(0x19c))+1);c.put32(frame.at(0x19c),c.r(3));const auto next=c.r(3);c.set(3,c.u32(frame.at(0x1b0))+0x4c);c.put32(frame.at(0x1b0),c.r(3));compare(c,next,20);
    }while(c.u32(frame.at(0x19c))<20);
}
}
std::uint32_t update_match(Cpu& c,std::uint32_t manager,std::uint32_t match,std::uint32_t statistics) {
    Frame frame(c,0x1000eb40,0x210,{.first_gpr=13});Program p{c};Fpu fp(c);
    keep(c,{{22,c.toc(-0x7b90)},{29,manager},{23,c.toc(-0x7b8c)},{30,match},{24,c.toc(-0x7b88)},{25,c.toc(-0x7b84)},
            {13,c.toc(slot::stats)},{26,c.toc(slot::doubles)}});
    c.put32(frame.at(0x230),statistics);p.call(0x10715880,0x1000eb7c,{frame.at(0x140)});c.put16(frame.at(0x196),1);
    if(compare_logical(c,match)==0){match_error(c,frame,0x1000eba4,0x120,0xd8,0x10c,true);c.set(3,0);return 0;}
    if(nonzero_byte(c,p.call(0x104abf40,0x1000ec04,{c.u32(c.toc(-0x7b34)),c.u32(match+20)}))){c.set(3,0);return 0;}
    if(compare_logical(c,statistics)==0){match_error(c,frame,0x1000ec38,0x100,0xd0,0x11b,true);c.set(3,0);return 0;}
    c.set(4,c.u32(match+20));
    if(compare_logical(c,c.r(4))!=0) {
        c.put16(frame.at(0x196),c.u16(c.r(4)+24));
        if(compare(c,c.i16(match+0x32))==0 && compare(c,c.i16(match+0x30),150)==0) {
            c.put32(frame.at(0x190),std::uint32_t(std::int32_t(c.i16(frame.at(0x196))))^0x80000000);c.put32(frame.at(0x18c),0x43300000);
            const auto reputation=fp.fsub(c.f64(frame.at(0x18c)),c.f64(c.r(26)+0x38));
            c.put64(frame.at(0x184),fp.fctiwz(0,fp.fmul(c.f64(c.r(26)),reputation)));c.put16(frame.at(0x196),c.u32(frame.at(0x188)));
        }
        if(compare(c,c.i16(frame.at(0x196)),1)<0)c.put16(frame.at(0x196),1);
    }
    c.put32(frame.at(0x1a0),0);c.put32(frame.at(0x1ac),0);
    MatchUpdate update{c,frame,manager,match,statistics};
    do {
        const auto side=c.u32(frame.at(0x1a0));compare(c,side);c.set(21,c.u32(match+(side==0?28:32)));
        compare(c,side,1);c.set(4,c.u32(match+(side==1?28:32)));c.set(5,0);
        for(unsigned offset:{0x74,0x78,0x7c})if(compare_logical(c,c.u32(c.r(21)+offset),c.r(4))==0){c.set(5,1);break;}
        c.put8(frame.at(0x195),c.r(5));const auto established=std::int8_t(p.call(0x10464160,0x1000ed7c,{c.r(21)}));c.put8(frame.at(0x194),established>0);
        c.set(27,p.call(0x1001b9c0,0x1000eda4,{manager,c.r(21),1}));c.set(28,p.call(0x1001b830,0x1000edb8,{manager,c.r(21),1}));
        c.set(5,c.u32(manager+28));c.set(19,c.r(5)+c.u32(c.r(21))*index_size);
        if(!update.flag(0x10494d20,0x1000eddc,{c.u32(c.toc(-0x7b54)),c.u32(match+20)}) || update.flag(0x100c7d00,0x1000edec,{c.r(21)}))update.team_records();
        c.put32(frame.at(0x19c),0);c.put32(frame.at(0x1b0),0);update.player_records();
        if(!update.flag(0x10494d20,0x100116f8,{c.u32(c.toc(-0x7b54)),c.u32(match+20)}))c.put32(c.r(27)+0x23c,c.u32(c.toc(slot::floats)+4));
        c.set(3,c.u32(frame.at(0x1a0))+1);c.put32(frame.at(0x1a0),c.r(3));const auto next=c.r(3);
        c.set(3,c.u32(frame.at(0x1ac))+0x664);c.put32(frame.at(0x1ac),c.r(3));compare(c,next,2);
    }while(c.u32(frame.at(0x1a0))<2);
    c.set(3,1);return 1;
}
}
