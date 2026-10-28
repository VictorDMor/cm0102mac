#include "oceania_nations_detail.hpp"
namespace cm::aot::oceania_nations {
using namespace detail;
void update(Cpu& c, std::uint32_t object, std::uint32_t date, std::uint32_t active) {
    Frame f(c,0x106eb7a0,0x120,{.first_gpr=24}); Program p{c};
    c.set({{28,active},{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{26,object},{31,c.toc(-0x4094)},{27,date}});
    c.compare(0,std::int8_t(active),0);
    if (!std::int8_t(active)) { p.call(0x104a1540,0x106eb7d4); return; }
    p.call(0x107156b0,0x106eb7f4,{f.at(0x70),signed_half(c.u16(object+0xb6)),
        (signed_half(c.u16(object+0xb8))+signed_half(c.u16(object+0x40)))&0xffff});
    const auto date_head=c.u32(date), date_tail=c.u32(date+4);
    c.set({{3,date_head},{5,c.u16(f.at(0x72))},{4,date_head&0xffff}});
    c.put32(f.at(0x50),date_head); c.put32(f.at(0x38),date_head); c.put32(f.at(0x3c),date_tail); c.put32(f.at(0x54),date_tail);
    const auto year=c.u16(date+2), threshold_year=c.u16(f.at(0x72)); c.compare_logical(0,year,threshold_year);
    const auto day=c.i16(date), threshold_day=c.i16(f.at(0x70));
    if (year==threshold_year) c.set(4,signed_half(std::uint16_t(threshold_day)));
    const bool earlier=year<threshold_year || (year==threshold_year && day<threshold_day);
    c.compare(0,earlier?1:0,0);
    if (earlier) { c.set(3,0); return; }
    c.put32(f.at(0x48),date_head); c.put32(f.at(0x4c),date_tail); c.set({{4,0},{3,date_tail}});
    c.compare_logical(0,year,threshold_year);
    bool equal=false;
    if (year==threshold_year) { c.compare(0,day,threshold_day); c.set(3,signed_half(std::uint16_t(day))); equal=day==threshold_day; }
    c.set(4,equal?1:0); c.compare(0,equal?1:0,0);
    if (equal) {
        const auto prepared=c.i8(object+0x45); c.compare(0,prepared,0);
        if (!prepared) {
            c.set(3,object); c.link(0x106eb8b0); c.set(3,select_entrants(c,object));
            c.set(3,object); p.virtual_call(object,0x64,0x106eb8c0);
            c.put16(object+0x3e,3); const auto storage=p.call(0x10775130,0x106eb8d8,{3*0x44});
            c.compare_logical(0,storage,0); c.put32(object+0xb0,storage);
            if (!storage) assertion(c,f,0x106eb8f8,0x78,0x40,0x246);
            else {
                c.set({{25,0},{24,0}});
                for (unsigned index=0;;++index) {
                    c.set(3,signed_byte(index)); c.compare(0,std::int8_t(index),c.i16(object+0x3e));
                    if (std::int8_t(index)>=c.i16(object+0x3e)) break;
                    p.call(0x104a60e0,0x106eb960,{object,index,c.u32(c.u32(object+0xec)+index*4),0,0});
                    c.set({{24,(index+1)*4},{25,index+1}});
                }
                p.call(0x104a0e40,0x106eb980,{object});
            }
            c.compare(0,storage?1:0,0);
            if (!storage) { assertion(c,f,0x106eb9a0,0xd8,0x68,0x405); c.set(3,0); return; }
            p.call(0x104a00a0,0x106eb9ec,{object}); c.compare(0,std::int32_t(c.r(3)),0);
            if (!c.r(3)) { assertion(c,f,0x106eba08,0xb8,0x60,0x40b); c.set(3,0); return; }
            p.call(0x1049eb30,0x106eba58,{object,0}); c.compare(0,std::int32_t(c.r(3)),0);
            if (!c.r(3)) { assertion(c,f,0x106eba74,0x98,0x58,0x412); c.set(3,0); return; }
            c.set({{3,object},{4,0}}); c.link(0x106ebac4); c.set(3,create_group(c,object,0));
            p.call(0x10775170,0x106ebacc,{c.u32(object+0xec)}); c.put32(object+0xec,0); c.put16(object+0xf0,0); c.put8(object+0x45,1);
        }
    }
    p.call(0x104a1540,0x106ebaf0,{object,date,active});
}
}
