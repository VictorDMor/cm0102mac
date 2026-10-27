#include "screen_manager_detail.hpp"
#include <array>

namespace cm::aot::screen_manager {
using namespace detail;
std::uint32_t create_control(Cpu& c,unsigned kind) {
    struct Site {std::uint32_t entry,size;unsigned first;};
    constexpr std::array sites{Site{0x10744250,0x70,31},Site{0x107442f0,0x70,31},Site{0x10744390,0x80,27},
                              Site{0x10744430,0x90,25},Site{0x107444e0,0x80,28},Site{0x107445a0,0x70,30}};
    const auto site=sites[kind];
    const auto address=c.r(3),arg4=c.r(4),arg5=c.r(5),arg6=c.r(6),arg7=c.r(7),arg8=c.r(8),arg9=c.r(9),arg10=c.r(10);
    Frame frame(c,site.entry,site.size,{.first_gpr=site.first});
    auto word=[&](unsigned offset,std::uint32_t value){c.put32(frame.at(offset),value);};
    if(kind==0 || kind==1) {
        keep(c,{{31,0}});
        for(unsigned off:{0x40,0x54,0x58,0x5c})word(off,0);
        word(0x44,12);word(0x60,missing);
        if(kind==0) {
            word(0x38,0x30);word(0x3c,arg4);word(0x48,7);word(0x4c,arg5);word(0x50,arg6);
            c.set({{4,1},{5,100},{6,10},{7,0x316},{8,70},{9,0},{10,0}});
        } else {
            word(0x38,2);word(0x3c,0);word(0x48,6);word(0x4c,c.u16(c.toc(-0x7bd0)));word(0x50,arg4);
            c.set({{4,1},{5,100},{6,80},{7,0x316},{8,125},{9,0},{10,0}});
        }
    } else if(kind==2 || kind==3) {
        const bool multi=kind==3;
        if(multi) {
            keep(c,{{25,arg5},{26,arg6},{27,arg7},{28,extended(c.i16(frame.caller()+0x3a))},
                    {29,c.u16(frame.caller()+0x3e)},{30,c.u32(frame.caller()+0x40)},{31,c.u32(frame.caller()+0x44)}});
        } else {
            keep(c,{{27,arg5},{28,arg6},{29,arg7},{30,extended(c.i16(frame.caller()+0x3a))},{31,c.u16(frame.caller()+0x3e)}});
        }
        word(0x38,arg8);word(0x3c,arg9);word(0x40,multi?arg9:0);word(0x44,arg10);
        word(0x48,c.r(multi?28:30));word(0x4c,c.r(multi?29:31));word(0x50,c.u32(frame.caller()+0x40));
        word(0x54,0);word(0x58,multi?c.r(31):0);word(0x5c,multi?c.u32(frame.caller()+0x48):0);word(0x60,missing);
        c.set({{4,multi?2u:1u},{5,arg4},{6,arg5},{7,arg6},{8,arg7},{9,0},{10,0}});
    } else {
        const bool multi=kind==4;
        if(multi)keep(c,{{29,arg6},{28,arg5},{30,c.u16(frame.caller()+0x3a)},{31,c.u32(frame.caller()+0x3c)}});
        else keep(c,{{31,arg6},{30,arg5}});
        word(0x38,arg7);word(0x3c,arg8);word(0x40,multi?arg8:0);word(0x44,arg9);word(0x48,arg10);
        word(0x4c,c.u16(frame.caller()+0x3a));word(0x50,c.u32(frame.caller()+0x3c));word(0x54,0);
        word(0x58,multi?c.u32(frame.caller()+0x40):0);word(0x5c,multi?c.u32(frame.caller()+0x44):0);word(0x60,arg4);
        c.set({{4,multi?2u:1u},{5,0},{6,0},{7,0},{8,0},{9,arg5},{10,arg6}});
    }
    return Program{c}.call(0x10719740,site.entry+(kind==0?0x80:kind==1?0x88:kind==2?0x8c:kind==3?0x94:kind==4?0x94:0x84),
                           {c.u32(Manager{c,address}.group()+0x24)});
}
}
