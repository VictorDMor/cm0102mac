#include "euro_champ_qual_detail.hpp"
#include <array>
namespace cm::aot::euro_champ_qual {
using namespace detail;
namespace {
struct SeededNation { int slot; unsigned offset; std::uint32_t back; };
#include "euro_champ_qual_draw_data.inc"
std::uint32_t draw_index(Cpu& c,const Program& p,std::uint32_t bound,std::uint32_t back) {
    if(compare(c,bound)==0) return 0;
    const auto cursor=c.u32(c.r(28))+2; c.set(3,cursor); c.put32(c.r(28),cursor);
    if(compare_logical(c,cursor,c.u32(c.r(27)))>0) {
        c.put32(c.r(28),c.r(26)); p.call(0x1077b770,back); c.put32(c.r(25),c.r(3)&0xffff);
    }
    c.set(3,0x10000); compare(c,bound,65535); c.set(3,0xffff0000); compare(c,bound,-65535);
    c.set(3,c.u32(c.r(28))); compare(c,bound);
    const auto sum=c.u16(c.r(3))+c.u32(c.r(25)); c.set(3,sum);
    const auto remainder=std::int64_t(std::int32_t(sum))%std::int32_t(bound);
    return std::uint32_t(std::int32_t(bound)>0?remainder:-remainder);
}
}
void draw_groups(Cpu& c,std::uint32_t object) {
    Kept<31> entrant_cursor(c,c.r(31));

    Kept<20> group_index(c,c.r(20)); Kept<21> group_count(c,c.r(21)); Kept<22> draw_choice(c,c.r(22)); Kept<23> group_token(c,c.r(23)); Kept<29> draw_order(c,c.r(29));

    Frame f(c,0x104fb050,0xd0,{.first_gpr=19}); Program p{c};
    keep(c,{{19,c.toc(slot::pool)},{20,c.toc(-0x7ba8)},{24,c.toc(-0x459c)},
        {25,c.toc(-0x7b90)},{26,c.toc(-0x7b8c)},{27,c.toc(-0x7b88)},{28,c.toc(-0x7b84)},
        {29,c.toc(-0x7b98)},{30,object},{31,0}});
    const auto sizes=c.toc(-0x4594),order=c.toc(-0x4598);
    group_count=signed_byte(c.u8(object+0xfd)+2);
    c.put32(f.at(0x4c),c.u32(sizes)); c.put32(f.at(0x50),c.u32(sizes+4)); c.put8(f.at(0x54),c.u8(sizes+8));
    c.put32(f.at(0x40),c.u32(order)); c.put32(f.at(0x44),c.u32(order+4)); c.put8(f.at(0x48),c.u8(order+8));
    leave(c,{{4,c.u8(order+8)},{5,c.u32(order+4)},{6,c.u32(order)},{7,c.u8(sizes+8)},{8,c.u32(sizes+4)},{9,c.u32(sizes)}});
    c.put32(object+0xf8,p.call(0x10775130,0x104fb0dc,{signed_byte(std::uint32_t(group_count)*6)*4}));
    if(compare_logical(c,c.u32(object+0xf8))==0) {
        assertion(c,f,0x104fb100,0x58,0x38,0x261); c.set(3,c.toc(-0x7bac)); return;
    }
    if(compare(c,c.i16(object+0x40),1998)==0) {
        c.set(4,c.u32(std::uint32_t(draw_order)));
        for(unsigned i=0;i<historical_draw.size();++i) {
            const auto row=historical_draw[i];
            p.call(0x100c76c0,row.back,{c.u32(std::uint32_t(draw_order))+c.u32(c.toc(row.slot))*0x128});
            if(i+1<historical_draw.size()) { c.set(5,c.u32(object+0xec)); c.set(4,c.toc(historical_draw[i+1].slot)); c.put32(c.r(5)+i*4,c.r(3)); c.set(5,c.u32(object+0xf8)); c.put32(c.r(5)+row.offset,c.r(3)); }
            else { c.set(4,c.u32(object+0xec)); c.put32(c.r(4)+i*4,c.r(3)); c.set(4,c.u32(object+0xf8)); c.put32(c.r(4)+row.offset,c.r(3)); }
        }
        return;
    }
    draw_order=f.at(0x40); Kept<19> pot(c,0);
    for(;;) {
        if(compare(c,std::int8_t(std::uint32_t(pot)),6)>=0) break;
        c.set(3,signed_byte(std::uint32_t(entrant_cursor))); if(compare(c,std::int8_t(std::uint32_t(entrant_cursor)),c.i32(object+0xf0))>=0) break;
        Kept<23> shuffle(c,0);
        while(compare(c,std::int8_t(std::uint32_t(shuffle)),20)<0) {
            draw_choice=draw_index(c,p,std::uint32_t(group_count),0x104fbac8);
            compare(c,std::uint32_t(group_count)); group_index=signed_byte(std::uint32_t(draw_choice));
            // The bound is sign8(fd+2), so the wide RNG branches cannot run.
            draw_choice=draw_index(c,p,std::uint32_t(group_count),0x104fbc24); c.set(3,signed_byte(std::uint32_t(draw_choice)));
            c.set(4,c.u8(std::uint32_t(draw_order)+std::uint32_t(group_index))); const auto other=c.u8(std::uint32_t(draw_order)+c.r(3));
            shuffle=std::uint32_t(shuffle)+1; c.put8(std::uint32_t(draw_order)+std::uint32_t(group_index),other); c.put8(std::uint32_t(draw_order)+c.r(3),c.r(4));
        }
        Kept<20> group(c,0);
        for(;;) {
            if(compare(c,std::int8_t(std::uint32_t(group)),std::uint32_t(group_count))>=0) break;
            c.set(3,signed_byte(std::uint32_t(entrant_cursor))); if(compare(c,std::int8_t(std::uint32_t(entrant_cursor)),c.i32(object+0xf0))>=0) break;
            group_token=c.u8(std::uint32_t(draw_order)+signed_byte(group)); draw_choice=signed_byte(std::uint32_t(group_token));
            if(compare(c,std::uint32_t(draw_choice),std::uint32_t(group_count))<0) {
                enter(c,0x104fbd9c,{object,signed_byte(std::uint32_t(draw_choice)-1)}); result(c,group_size(c,object,std::uint32_t(draw_choice)-1));
                c.set(7,std::uint32_t(draw_choice)); c.set(8,f.at(0x4c)); c.set(4,signed_byte(c.u8(c.r(8)+c.r(7))));
                if(compare(c,c.r(4),std::int16_t(c.r(3)))<0) {
                    const auto target=(std::uint32_t(draw_choice)*6+c.r(4))*4;
                    c.set(6,c.u32(object+0xec)); c.set(5,c.u32(c.r(6)+signed_byte(std::uint32_t(entrant_cursor))*4)); entrant_cursor=std::uint32_t(entrant_cursor)+1;
                    c.set(3,target); c.set(4,c.u32(object+0xf8));
                    c.put8(c.r(8)+c.r(7),c.u8(c.r(8)+c.r(7))+1); c.put32(c.r(4)+target,c.r(5));
                }
            }
            group=std::uint32_t(group)+1;
        }
        pot=std::uint32_t(pot)+1;
    }
}
}
