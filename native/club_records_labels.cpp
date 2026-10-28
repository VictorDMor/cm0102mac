#include "club_records_detail.hpp"
#include <array>

namespace cm::aot::club_records {
using namespace detail;
namespace {
constexpr std::array<unsigned,32> label_offsets{
    0xca,0xe2,0xf9,0x10c,0x122,0x134,0x147,0x153,0x166,0x175,0x18a,
    0x1a6,0x1bc,0x1d3,0x1ed,0x208,0x217,0x22d,0x241,0x24e,0x265,
    0x277,0x288,0x2a2,0x2bd,0x2cd,0x2db,0x2ee,0x308,0x326,0x33e,0x354};
}
std::uint32_t period_label(Cpu& c,std::uint32_t period,std::uint32_t output,std::uint32_t capacity) {
    Frame frame(c,0x100137c0,0xa0,{.first_gpr=29});Program p{c};
    keep(c,{{31,c.toc(slot::pool)},{30,c.toc(slot::message)},{29,c.toc(slot::success)}});
    if(compare_logical(c,output)==0 || compare(c,capacity,50)<0) {
        assertion(c,frame,0x10013808,0x68,0x40,0x766);c.set(3,0);return 0;
    }
    const auto which=std::int8_t(period);c.set(3,0);c.put8(output,0);
    if(compare(c,which,2)==0) {
        p.call(0x10727890,0x10013890,{output,c.toc(slot::pool)+0xbe});c.set(3,1);return 1;
    }
    if(which<2 && compare(c,which,1)>=0) {
        p.call(0x10727890,0x1001387c,{output,c.toc(slot::pool)+0xb3});c.set(3,1);return 1;
    }
    assertion(c,frame,0x100138ac,0x48,0x38,0x780);c.set(3,0);return 0;
}

std::uint32_t record_label(Cpu& c,std::uint32_t kind,std::uint32_t output,std::uint32_t capacity) {
    Frame frame(c,0x10013910,0xa0,{.first_gpr=29});Program p{c};
    keep(c,{{31,c.toc(slot::pool)},{30,c.toc(slot::message)},{29,c.toc(slot::success)}});
    if(compare_logical(c,output)==0 || compare(c,capacity,50)<0) {
        assertion(c,frame,0x10013958,0x68,0x40,0x793);c.set(3,0);return 0;
    }
    const auto which=std::uint32_t(std::int32_t(std::int8_t(kind)));
    c.set(3,0);c.put8(output,0);
    if(compare_logical(c,which,32)<=0 && which!=0) {
        p.call(0x10727890,0x100139d0+(which-1)*0x14,{output,c.toc(slot::pool)+label_offsets[which-1]});
        c.set(3,1);return 1;
    }
    assertion(c,frame,0x10013c58,0x48,0x38,0x87f);c.set(3,0);return 0;
}

std::uint32_t record_available(Cpu& c,std::uint32_t kind,std::uint32_t club,std::uint32_t period) {
    Frame frame(c,0x10013cc0,0x70,{.first_gpr=30});Program p{c};
    keep(c,{{31,c.toc(slot::pool)},{30,c.toc(slot::message)}});
    if(compare_logical(c,club)==0) {
        assertion(c,frame,0x10013cf8,0x40,0x38,0x892,true);c.set(3,0);return 0;
    }
    const auto which=std::uint32_t(std::int32_t(std::int8_t(kind)));
    if(compare_logical(c,which,32)>0) {c.set(3,1);return 1;}
    std::uint32_t back=0;
    switch(which) {
    case 1: back=0x10013d68;break;
    case 2: back=0x10013d80;break;
    case 6: back=0x10013d98;break;
    case 8: back=0x10013db0;break;
    case 11: back=0x10013dc8;break;
    case 17: back=0x10013de0;break;
    case 23: back=0x10013df8;break;
    case 24: back=0x10013e10;break;
    case 27: back=0x10013e28;break;
    case 28: back=0x10013e40;break;
    case 29: back=0x10013e58;break;
    case 30: back=0x10013e70;break;
    case 31: back=0x10013e88;break;
    case 32:
        if(compare(c,std::int8_t(period),1)==0){c.set(3,0);return 0;}
        back=0x10013eb4;break;
    default:c.set(3,1);return 1;
    }
    const bool national=nonzero_byte(c,p.call(0x100c7d00,back,{club}));
    const auto result=std::uint32_t(which==27?national:!national);c.set(3,result);return result;
}
}
