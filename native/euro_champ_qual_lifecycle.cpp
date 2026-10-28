#include "euro_champ_qual_detail.hpp"
namespace cm::aot::euro_champ_qual {
using namespace detail;
std::uint32_t construct(Cpu& c,std::uint32_t address,std::uint32_t year,std::uint32_t definition) {
    Frame f(c,0x104f9fa0,0xa0,{.first_gpr=27});
    Kept<28> requested_year(c,year); Kept<27> saved_definition(c,definition);
    keep(c,{{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(slot::pool)}});
    c.put32(f.at(0xb8),address); Program p{c}; Competition cup{c,address};
    p.call(0x1049e4f0,0x104f9fd0,{address});
    leave(c,{{4,8},{5,7},{6,1},{7,0},{8,~0u},{9,4}});
    c.put32(address,c.toc(slot::vtable)); cup.year()=std::int16_t(year); c.put32(address+4,saved_definition);
    c.put8(address+0x42,4); c.put32(address+0x1c,~0u); c.put32(address+0x20,~0u);
    for(unsigned offset:{0x45,0xfc,0x4a}) c.put8(address+offset,0);
    for(unsigned offset:{0xf8,0xec,0xf0}) c.put32(address+offset,0);
    c.put8(address+0xf4,1); c.put8(address+0xfd,7); c.put8(address+0xfe,8); c.put8(address+0xff,9);
    cup.year()=std::int16_t(year-1);
    // The original aligns the previous year to the 1990 four-year cycle.
    while((std::int32_t(cup.year())-1990)%4!=0) cup.year()=std::int16_t(std::int32_t(cup.year())+1);
    compare(c,0);
    c.set(4,address); c.set(3,std::uint32_t(std::int32_t(cup.year())-1990)); c.put16(address+0xdc,36);
    saved_definition=p.call(0x10773520,0x104fa0c4,{0x10});
    if(compare(c,saved_definition)!=0) p.call(0x10738c00,0x104fa0d0);
    c.put32(address+0x10,saved_definition); c.put8(address+0x4b,15);
    leave(c,{{3,address},{4,1}});
    if(compare(c,p.virtual_call(address,0x20,0x104fa0f8))!=0) return address;
    c.set(3,address);
    if(compare(c,p.virtual_call(address,0x94,0x104fa11c))==0) {
        assertion(c,f,0x104fa13c,0x68,0x40,0x5e); return address;
    }
    saved_definition=p.call(0x10773520,0x104fa188,{0x634});
    if(compare(c,saved_definition)!=0) p.call(0x104533a0,0x104fa198,{saved_definition,address});
    c.set(3,address); c.put32(address+8,saved_definition);
    if(compare_logical(c,c.u32(address+8))==0) {
        assertion(c,f,0x104fa1c4,0x48,0x38,0x66); return address;
    }
    c.put8(address+0x44,0xff); cup.current_stage()=-1; c.put32(address+0x2c,10);
    leave(c,{{3,40},{4,~0u}}); compare(c,10);
    cup.groups()=p.call(0x10775130,0x104fa23c); c.set(4,address);
    c.set(3,address); c.set(4,signed_half(requested_year));
    if(compare(c,std::int16_t(std::uint32_t(requested_year)),cup.year())>0) {
        enter(c,0x104fa268,{address}); advance_stage(c,address);
    }
    c.set(4,address); c.set(3,signed_half(requested_year));
    if(compare(c,std::int32_t(cup.year())-std::int16_t(std::uint32_t(requested_year)),2)>0) c.put8(address+0x45,0);
    return address;
}
std::uint32_t destroy(Cpu& c,std::uint32_t address,std::uint32_t deleting) {
    Frame f(c,0x104fa2a0,0x50,{.first_gpr=28});
    Kept<28> object(c,address); Kept<29> delete_flag(c,deleting); Program p{c}; Competition cup{c,address};
    if(compare(c,address)!=0) {
        c.put32(address,c.toc(slot::vtable)); p.call(0x104a6660,0x104fa2d8,{address,0});
        c.set(3,c.u32(address+0xbc));
        if(compare_logical(c,c.r(3))!=0) { p.call(0x10775170,0x104fa2e8); c.put32(address+0xbc,0); }
        Kept<30> group(c,0); Kept<31> offset(c,0);
        while(compare(c,group,cup.current_stage())<=0) {
            const auto stage=c.u32(std::uint32_t(cup.groups())+std::uint32_t(offset)); c.set(3,stage);
            if(compare_logical(c,stage)!=0) { c.set(4,1); p.virtual_call(stage,8,0x104fa320); }
            offset=std::uint32_t(offset)+4; group=std::uint32_t(group)+1;
        }
        c.set(3,cup.groups());
        if(compare_logical(c,c.r(3))!=0) { p.call(0x10775170,0x104fa348); cup.groups()=0; }
        c.set(3,c.u32(address+0x10)); if(compare_logical(c,c.r(3))!=0) p.call(0x10738c20,0x104fa364,{c.r(3),1});
        c.set(3,c.u32(address+8)); if(compare_logical(c,c.r(3))!=0) p.call(0x104536c0,0x104fa378,{c.r(3),1});
        c.set(3,cup.entrants()); if(compare_logical(c,c.r(3))!=0) p.call(0x10775170,0x104fa388);
        p.call(0x1049e600,0x104fa394,{object,0});
        if(compare(c,std::int16_t(std::uint32_t(delete_flag)))>0) p.call(0x107735a0,0x104fa3a4,{object});
    }
    return object;
}
}
