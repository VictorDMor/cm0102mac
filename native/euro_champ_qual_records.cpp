#include "euro_champ_qual_detail.hpp"
#include "guest_fpu.hpp"
#include <array>
namespace cm::aot::euro_champ_qual {
using namespace detail;
namespace {
// Keep the original decision tree: its final comparison is visible to callers.
unsigned group_position(Cpu& c,unsigned value,unsigned unchanged,unsigned second) {
    if(compare(c,value,4)==0) return 28;
    if(value>=4) {
        if(compare(c,value,6)==0) return 46;
        if(value>=6) return unchanged;
        return 37;
    }
    if(compare(c,value,2)==0) return second;
    if(value>=2) return 19;
    if(compare(c,value,1)>=0) return 1;
    return unchanged;
}
unsigned playoff_position(Cpu& c,unsigned value,unsigned unchanged) {
    if(compare(c,value,2)==0) return 16;
    if(value>=2) return unchanged;
    if(compare(c,value,1)>=0) return 13;
    return unchanged;
}
}
void update_record(Cpu& c,std::uint32_t object,std::uint32_t index,std::uint32_t phase,std::uint32_t first,std::uint32_t second,std::uint32_t third) {
    Frame f(c,0x104fed60,0x50,{.first_gpr=27}); Program p{c};
    keep(c,{{27,object},{28,phase},{29,first},{30,second},{31,third}});
    const auto record=p.call(0x10459b50,0x104fed8c,{c.u32(object+8),index});
    c.set(4,signed_byte(phase));
    if(compare(c,std::int8_t(phase),c.i8(object+0xfd))<=0) {
        c.set(30,group_position(c,second&255,second,10));
        c.set(31,group_position(c,third&255,third,16));
        c.set(29,group_position(c,first&255,first,16));
    } else if(compare(c,std::int8_t(phase),c.i8(object+0xff))==0) {
        c.set(30,playoff_position(c,second&255,second));
        c.set(31,playoff_position(c,third&255,third));
        c.set(29,playoff_position(c,first&255,first));
    }
    c.put8(record+0x76,c.r(29)); c.put8(record+0x77,c.r(30)); c.put8(record+0x78,c.r(31));
}
std::uint32_t phase_category(Cpu& c,std::uint32_t object,std::uint32_t index) {
    Frame f(c,0x104fef80,0x40,{}); Program p{c}; Fpu fp(c);
    p.call(0x10457d00,0x104fef98,{c.u32(object+8),index,18});
    c.put64(f.at(0x38),fp.fctiwz(0,c.f(1))); c.set(4,14);
    return c.i8(f.at(0x3f))<14?std::uint32_t(-2):std::uint32_t(-3);
}
std::uint32_t compare_categories(Cpu& c,std::uint32_t object,std::uint32_t index) {
    Frame f(c,0x104fefe0,0x50,{.first_gpr=30}); Program p{c}; Fpu fp(c);
    Kept<30> saved_object(c,object); Kept<31> other(c,index);
    p.call(0x10457d00,0x104ff008,{c.u32(object+8),index,19});
    c.put64(f.at(0x40),fp.fctiwz(0,c.f(1))); other=c.u32(f.at(0x44));
    p.call(0x10457d00,0x104ff024,{c.u32(object+8),index,18});
    c.put64(f.at(0x38),fp.fctiwz(0,c.f(1)));
    if(compare(c,c.i8(f.at(0x3f)),14)<0) {
        c.set(4,signed_byte(other)); return std::int8_t(std::uint32_t(other))<14?0:~0u;
    }
    c.set(4,14); return std::int8_t(std::uint32_t(other))<14?1:0;
}
void seed_rankings(Cpu& c,std::uint32_t object) {
    Kept<29> entrant_cursor(c,c.r(29));

    Frame f(c,0x104fe980,0x80,{.first_gpr=27}); Program p{c};
    keep(c,{{27,object},{28,c.toc(slot::pool)},{29,0},{30,c.toc(-0x7ba8)},{31,c.toc(-0x7a64)}});
    if(compare_logical(c,c.u32(object+8))==0) { assertion(c,f,0x104fe9c4,0x40,0x38,0x6f2); c.set(3,c.toc(-0x7bac)); return; }
    p.call(0x10768300,0x104fea1c,{c.u32(object+0xec),c.u32(object+0xf0),4,c.toc(-0x6188)});
    p.call(0x102b7280,0x104fea40,{c.u32(c.r(31)),c.u32(c.toc(-0x7520))+c.u32(c.toc(-0x7a70))*0xc8,1});
    Kept<28> rank(c,0); Kept<30> offset(c,0);
    for(;;) {
        c.set(3,signed_half(rank)); if(compare(c,std::int16_t(std::uint32_t(rank)),c.i32(object+0xf0))>=0) break;
        const auto hosts=c.i8(object+0xf4); unsigned position=0,back=0;
        if(compare(c,hosts,1)==0) {
            constexpr std::array bounds{9,12,15,18,27,36,45};
            constexpr std::array positions{1u,10u,13u,16u,19u,28u,37u,46u};
            constexpr std::array returns{0x104fea8c,0x104feab8,0x104feae4,0x104feb10,0x104feb3c,0x104feb68,0x104feb94,0x104febb8};
            unsigned i=0; for(;i<bounds.size();++i) if(compare(c,std::int16_t(std::uint32_t(rank)),bounds[i])<0) break;
            position=positions[i]; back=returns[i];
        } else if(compare(c,hosts,2)==0) {
            constexpr std::array bounds{9,10,14,18,27,36,45};
            constexpr std::array positions{1u,10u,11u,15u,19u,28u,37u,46u};
            constexpr std::array returns{0x104febf0,0x104fec1c,0x104fec48,0x104fec74,0x104feca0,0x104feccc,0x104fecf8,0x104fed1c};
            unsigned i=0; for(;i<bounds.size();++i) if(compare(c,std::int16_t(std::uint32_t(rank)),bounds[i])<0) break;
            position=positions[i]; back=returns[i];
        }
        if(back) {
            const auto entrant=c.u32(c.u32(object+0xec)+std::uint32_t(std::int16_t(std::uint32_t(entrant_cursor)))*4);
            entrant_cursor=std::uint32_t(entrant_cursor)+1; p.call(0x10457490,back,{c.u32(object+8),entrant,position});
        }
        const auto entrant=c.u32(c.u32(object+0xec)+std::uint32_t(offset));
        p.call(0x102b7490,0x104fed34,{c.u32(c.r(31)),c.u32(entrant+0x2c),c.u32(object+4)});
        offset=std::uint32_t(offset)+4; rank=std::uint32_t(rank)+1;
    }
}
void describe_stage(Cpu& c,std::uint32_t,std::uint32_t buffer,std::uint32_t capacity,std::uint32_t phase,std::uint32_t,std::uint32_t event,std::uint32_t club) {
    Frame f(c,0x104fc4b0,0x70,{.first_gpr=30}); Program p{c};
    keep(c,{{31,c.toc(slot::pool)},{30,c.toc(-0x7ba8)}});
    if(compare(c,capacity,30)<0) { assertion(c,f,0x104fc4e8,0x40,0x38,0x31c); c.set(3,c.toc(-0x7bac)); return; }
    const auto pool=c.r(31); bool qualified=false;
    if(compare(c,std::int16_t(phase),1000)>=0 && compare(c,std::int16_t(phase),1013)<=0) qualified=compare(c,std::int8_t(event),7)==0;
    else qualified=compare(c,std::int8_t(event),1)==0;
    if(qualified) p.call(0x10727890,std::int16_t(phase)>=1000 && std::int16_t(phase)<=1013?0x104fc564:0x104fc59c,{buffer,pool+0x20c,signed_byte(c.u8(club+0x242)),pool+0x230});
    else p.call(0x10727890,std::int16_t(phase)>=1000 && std::int16_t(phase)<=1013?0x104fc574:0x104fc5ac,{buffer,pool+(std::int16_t(phase)>=1000 && std::int16_t(phase)<=1013?0x231:0x23d)});
}
}
