#include "club_records_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::club_records {
using namespace detail;
namespace {
// A notice serializes a selected domain record into the news integer fields.
// Aggregate player/match/date layouts belong to PLAYER_STATS and remain calls
// to its existing codecs. The cursor is an original byte in the caller frame.
template<bool Writing> struct Notice {
    Cpu& c;const Frame& frame;std::uint32_t record,message;Program p{c};
    unsigned advance() const {const auto index=c.u8(frame.at(0x38));c.put8(frame.at(0x38),index+1);return index;}
    std::uint32_t get(std::uint32_t back) const {
        const auto index=advance();return p.call(0x102cb570,back,{message,index});
    }
    void put(std::uint32_t value,std::uint32_t back) const {
        const auto index=advance();p.call(0x102cb4b0,back,{message,index,value});
    }
    void scalar(unsigned offset,unsigned width,bool signed_value,std::uint32_t write_back,std::uint32_t read_back) const {
        if constexpr(Writing) {
            const auto index=advance();
            const auto address=record+offset;
            auto value=width==1?std::uint32_t(c.u8(address)):width==2?std::uint32_t(c.u16(address)):c.u32(address);
            if(signed_value)value=width==1?std::uint32_t(std::int32_t(std::int8_t(value))):std::uint32_t(std::int32_t(std::int16_t(value)));
            p.call(0x102cb4b0,write_back,{message,index,value});
        }else {
            const auto value=get(read_back);
            if(width==1)c.put8(record+offset,value);else if(width==2)c.put16(record+offset,value);else c.put32(record+offset,value);
        }
    }
    void stats(unsigned offset,std::uint32_t write_back,std::uint32_t read_back) const {
        aggregate(offset,0x1030e110,0x1030e2a0,write_back,read_back);
    }
    void match(unsigned offset,std::uint32_t write_back,std::uint32_t read_back) const {
        aggregate(offset,0x1030dcd0,0x1030dee0,write_back,read_back);
    }
    void dated_total(unsigned offset,std::uint32_t write_back,std::uint32_t read_back) const {
        aggregate(offset,0x1030e450,0x1030e5d0,write_back,read_back);
    }
    void aggregate(unsigned offset,std::uint32_t encode,std::uint32_t decode,std::uint32_t write_back,std::uint32_t read_back) const {
        if constexpr(Writing)p.call(encode,write_back,{record+offset,message,frame.at(0x38)});
        else p.call(decode,read_back,{message,frame.at(0x38),record+offset});
    }
    void date(unsigned offset,std::uint32_t write_day,std::uint32_t write_year,
              std::uint32_t read_day,std::uint32_t read_year,std::uint32_t date_back,unsigned temporary) const {
        if constexpr(Writing) {
            scalar(offset,2,true,write_day,0);scalar(offset+2,2,false,write_year,0);
        }else {
            c.set(27,get(read_day)&0xffff);const auto year=get(read_year)&0xffff;
            p.call(0x107156b0,date_back,{frame.at(temporary),std::uint32_t(std::int32_t(std::int16_t(c.r(27)))),year});
            c.put16(record+offset+2,c.u16(frame.at(temporary+2)));c.put16(record+offset,c.u16(frame.at(temporary)));
            c.put8(record+offset+4,c.u8(frame.at(temporary+4)));
        }
    }
    void average() const {
        Fpu fp(c);
        if constexpr(Writing) {
            const auto index=advance();c.set(5,c.toc(slot::floats));c.set_f(1,c.f32(c.r(5)+8));
            const auto scaled=fp.fmuls(c.f(1),c.f32(record+0x23c));
            c.put64(frame.at(0xb4),fp.fctiwz(0,scaled));
            p.call(0x102cb4b0,0x1001c768,{message,index,c.u32(frame.at(0xb8))});
        }else {
            const auto value=get(0x1001cfdc);c.set(6,c.toc(slot::doubles));
            c.put32(frame.at(0xc8),value^0x80000000);c.put32(frame.at(0xc4),0x43300000);
            c.set_f(1,fp.fsub(c.f64(frame.at(0xc4)),c.f64(c.r(6)+0x38)));
            c.put_f32(record+0x23c,float(fp.frsp(fp.fdiv(c.f(1),c.f64(c.r(6)+0x88)))));
        }
    }
    void payload(unsigned kind) const {
        switch(kind) {
        case 1:scalar(0,1,false,0x1001c3e8,0x1001cc84);scalar(4,4,false,0x1001c400,0x1001cc9c);scalar(8,2,false,0x1001c418,0x1001ccb4);break;
        case 2:scalar(12,1,false,0x1001c438,0x1001ccd4);scalar(16,4,false,0x1001c450,0x1001ccec);scalar(20,2,false,0x1001c468,0x1001cd04);break;
        case 3:scalar(0x18,4,false,0x1001c488,0x1001cd24);match(0x1c,0x1001c498,0x1001cd38);break;
        case 4:scalar(0x40,4,false,0x1001c4b8,0x1001cd54);match(0x44,0x1001c4c8,0x1001cd68);break;
        case 5:scalar(0x68,4,false,0x1001c4e8,0x1001cd84);match(0x6c,0x1001c4f8,0x1001cd98);break;
        case 6:scalar(0x90,4,false,0x1001c518,0x1001cdb4);scalar(0x94,2,false,0x1001c530,0x1001cdcc);break;
        case 7:match(0x98,0x1001c548,0x1001cde8);break;
        case 8:match(0xbc,0x1001c560,0x1001ce00);break;
        case 9:match(0xe0,0x1001c578,0x1001ce18);break;
        case 10:match(0x104,0x1001c590,0x1001ce30);break;
        case 11:match(0x128,0x1001c5a8,0x1001ce48);break;
        case 12:dated_total(0x14c,0x1001c5c0,0x1001ce60);break;
        case 13:dated_total(0x160,0x1001c5d8,0x1001ce78);break;
        case 14:dated_total(0x174,0x1001c5f0,0x1001ce90);break;
        case 15:dated_total(0x188,0x1001c608,0x1001cea8);break;
        case 16:scalar(0x19c,1,true,0x1001c62c,0x1001cec4);stats(0x1a0,0x1001c63c,0x1001ced8);scalar(0x1b8,2,false,0x1001c654,0x1001ceec);break;
        case 17:scalar(0x1bc,1,true,0x1001c678,0x1001cf0c);stats(0x1c0,0x1001c688,0x1001cf20);scalar(0x1d8,2,false,0x1001c6a0,0x1001cf34);break;
        case 18:scalar(0x1dc,1,true,0x1001c6c4,0x1001cf54);stats(0x1e0,0x1001c6d4,0x1001cf68);match(0x1f8,0x1001c6e4,0x1001cf78);break;
        case 19:scalar(0x21c,1,true,0x1001c708,0x1001cf94);stats(0x220,0x1001c718,0x1001cfa8);scalar(0x238,2,false,0x1001c730,0x1001cfbc);break;
        case 20:average();stats(0x240,0x1001c778,0x1001d01c);scalar(0x258,1,true,0x1001c794,0x1001d030);scalar(0x25a,2,false,0x1001c7ac,0x1001d048);break;
        case 21:scalar(0x25c,1,true,0x1001c7d0,0x1001d068);stats(0x260,0x1001c7e0,0x1001d07c);scalar(0x278,2,false,0x1001c7f8,0x1001d090);break;
        case 22:scalar(0x27c,1,true,0x1001c81c,0x1001d0b0);scalar(0x27d,1,true,0x1001c838,0x1001d0c8);stats(0x280,0x1001c848,0x1001d0dc);scalar(0x298,2,false,0x1001c860,0x1001d0f0);break;
        case 23:scalar(0x29c,2,true,0x1001c880,0x1001d110);stats(0x2a0,0x1001c890,0x1001d124);break;
        case 24:scalar(0x2b8,2,true,0x1001c8b0,0x1001d140);stats(0x2bc,0x1001c8c0,0x1001d154);break;
        case 25:date(0x2d4,0x1001c8e0,0x1001c8f8,0x1001d170,0x1001d188,0x1001d198,0x44);stats(0x2dc,0x1001c908,0x1001d1c0);match(0x2f4,0x1001c918,0x1001d1d0);break;
        case 26:date(0x318,0x1001c938,0x1001c950,0x1001d1ec,0x1001d204,0x1001d214,0x3c);stats(0x320,0x1001c960,0x1001d23c);match(0x338,0x1001c970,0x1001d24c);break;
        case 27:scalar(0x35c,2,true,0x1001c990,0x1001d268);stats(0x360,0x1001c9a0,0x1001d27c);break;
        case 28:scalar(0x378,4,false,0x1001c9c0,0x1001d298);stats(0x37c,0x1001c9d0,0x1001d2ac);scalar(0x394,4,false,0x1001c9e8,0x1001d2c0);scalar(0x3a0,1,true,0x1001ca04,0x1001d2d8);break;
        case 29:scalar(0x3a4,4,false,0x1001ca24,0x1001d2f8);stats(0x3a8,0x1001ca34,0x1001d30c);scalar(0x3c0,4,false,0x1001ca4c,0x1001d320);scalar(0x3cc,1,true,0x1001ca68,0x1001d338);break;
        case 30:scalar(0x3d0,4,false,0x1001ca88,0x1001d358);break;
        case 31:scalar(0x3d8,4,false,0x1001caa8,0x1001d378);break;
        case 32:stats(0x3e0,0x1001cac0,0x1001d394);break;
        }
    }
};
}

std::uint32_t encode_notice(Cpu& c,std::uint32_t club,std::uint32_t record,std::uint32_t kind,std::uint32_t message) {
    Frame frame(c,0x1001c290,0xe0,{.first_gpr=26});
    keep(c,{{30,record},{29,c.toc(slot::success)},{27,c.toc(slot::message)},
            {28,c.toc(slot::pool)},{26,kind},{31,message}});c.set(4,club);
    if(compare(c,record)==0){assertion(c,frame,0x1001c2d4,0x94,0x4c,0x1829);c.set(3,0);return 0;}
    if(compare_logical(c,message)==0){assertion(c,frame,0x1001c334,0x74,0x44,0x1831);c.set(3,0);return 0;}
    c.put8(frame.at(0x38),0);const Notice<true> notice{c,frame,record,message};
    notice.put(club,0x1001c398);notice.put(std::uint32_t(std::int32_t(std::int8_t(kind))),0x1001c3b0);
    const auto which=std::uint32_t(std::int32_t(std::int8_t(kind)));
    if(compare_logical(c,which,32)>0 || which==0){assertion(c,frame,0x1001cadc,0x54,0x3c,0x1989);c.set(3,0);return 0;}
    notice.payload(which);c.set(3,1);return 1;
}
std::uint32_t decode_notice(Cpu& c,std::uint32_t message,std::uint32_t club_out,std::uint32_t record,std::uint32_t kind_out) {
    Frame frame(c,0x1001cb30,0xf0,{.first_gpr=25});
    keep(c,{{31,message},{29,c.toc(slot::success)},{27,c.toc(slot::message)},{25,club_out},
            {28,c.toc(slot::pool)},{30,record},{26,kind_out}});
    if(compare(c,message)==0){assertion(c,frame,0x1001cb74,0xa4,0x5c,0x19a0);c.set(3,0);return 0;}
    if(compare_logical(c,record)==0){assertion(c,frame,0x1001cbd4,0x84,0x54,0x19a8);c.set(3,0);return 0;}
    c.put8(frame.at(0x38),0);const Notice<false> notice{c,frame,record,message};
    c.put32(club_out,notice.get(0x1001cc30));c.put8(kind_out,notice.get(0x1001cc48));
    const auto kind=std::uint32_t(std::int32_t(c.i8(kind_out)));
    if(compare_logical(c,kind,32)>0 || kind==0){assertion(c,frame,0x1001d3b0,0x64,0x4c,0x1b02);c.set(3,0);return 0;}
    notice.payload(kind);c.set(3,1);return 1;
}
}
