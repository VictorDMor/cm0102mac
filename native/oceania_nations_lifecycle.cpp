#include "oceania_nations_detail.hpp"
namespace cm::aot::oceania_nations {
using namespace detail;
std::uint32_t construct(Cpu& c, std::uint32_t object, std::uint32_t year, std::uint32_t definition) {
    Frame f(c, 0x106e9940, 0xa0, {.first_gpr=27}); Program p{c}; Cup cup{c,object};
    c.set({{27,year},{28,definition},{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(-0x4094)}});
    c.put32(f.at(0xb8),object); p.call(0x1049e4f0,0x106e9970,{object});
    c.set({{6,c.toc(-0x5cc4)},{5,4},{3,object},{4,~0u}});
    c.put32(object,c.r(6)); cup.year()=year; c.put32(object+4,definition);
    c.put8(object+0x42,4); c.put32(object+0x1c,~0u); c.put32(object+0x20,~0u); c.put8(object+0x4a,0);
    c.compare(0,c.i16(object+0x40),2002); if (c.i16(object+0x40)<2002) cup.year()=2002;
    // The original aligns the competition to even years, including signed halfword wrapping.
    while (true) {
        c.set(5,object); const auto y=c.i16(object+0x40); c.set(3,std::uint32_t(y));
        const auto remainder=(std::int32_t(y)-1996)%2; c.compare(0,remainder,0);
        if (!remainder) break;
        cup.year()=std::uint32_t(y)+1;
    }
    c.put8(object+0x4b,15); c.set({{3,object},{4,1}});
    p.virtual_call(object,0x20,0x106e9a1c); c.compare(0,std::int32_t(c.r(3)),0);
    if (c.r(3)) return object;
    c.set({{3,object},{5,0},{4,~0u}}); c.put16(object+0xdc,40); c.put8(object+0x45,0);
    c.put8(object+0x44,0xff); cup.current_stage()=-1; c.put32(object+0x2c,2);
    c.compare(0,2,0); p.call(0x10775130,0x106e9a80,{8}); c.set(4,object); cup.stages()=c.r(3);
    c.set(3,object); p.virtual_call(object,0x94,0x106e9aa0); c.compare(0,std::int32_t(c.r(3)),0);
    if (!c.r(3)) { assertion(c,f,0x106e9ac0,0x68,0x40,0x57); return object; }
    p.call(0x10773520,0x106e9b0c,{0x634}); const auto fixtures=c.r(3); c.set(27,fixtures); c.compare(0,std::int32_t(fixtures),0);
    if (fixtures) p.call(0x104533a0,0x106e9b1c,{fixtures,object});
    c.set(3,object); cup.fixtures()=fixtures; c.compare_logical(0,fixtures,0);
    if (!fixtures) assertion(c,f,0x106e9b48,0x48,0x38,0x5e);
    return object;
}
std::uint32_t destroy(Cpu& c, std::uint32_t object, std::uint32_t deleting) {
    Frame f(c,0x106e9ba0,0x50,{.first_gpr=28}); Program p{c};
    c.set({{28,object},{29,deleting}}); c.compare(0,std::int32_t(object),0);
    if (object) {
        c.put32(object,c.toc(-0x5cc4)); p.call(0x104a6660,0x106e9bd8,{object,0});
        c.set(3,c.u32(object+0xbc)); c.compare_logical(0,c.r(3),0);
        if (c.r(3)) { p.call(0x10775170,0x106e9be8); c.put32(object+0xbc,0); }
        c.set({{30,0},{31,0}});
        for (std::uint32_t index=0;;++index) {
            c.compare(0,std::int32_t(index),c.i32(object+0x30)); if (std::int32_t(index)>c.i32(object+0x30)) break;
            const auto stage=c.u32(c.u32(object+0xc)+index*4); c.set(3,stage); c.compare_logical(0,stage,0);
            if (stage) { c.set(4,1); p.virtual_call(stage,8,0x106e9c20); }
            c.set({{30,index+1},{31,(index+1)*4}});
        }
        c.set(3,c.u32(object+0xc)); c.compare_logical(0,c.r(3),0);
        if (c.r(3)) { p.call(0x10775170,0x106e9c48); c.put32(object+0xc,0); }
        c.set(3,c.u32(object+8)); c.compare_logical(0,c.r(3),0);
        if (c.r(3)) p.call(0x104536c0,0x106e9c64,{c.r(3),1});
        p.call(0x1049e600,0x106e9c70,{object,0}); c.compare(0,std::int16_t(deleting),0);
        if (std::int16_t(deleting)>0) p.call(0x107735a0,0x106e9c80,{object});
    }
    return object;
}
std::uint32_t configure(Cpu& c, std::uint32_t object) {
    Frame f(c,0x106eaaf0,0x50,{.first_gpr=31}); Program p{c}; c.set(31,object);
    c.set({{11,1},{10,3},{9,10},{8,2},{6,0},{4,5},{5,2},{7,5}});
    c.put16(object+0x3c,1);
    for (auto [offset,value]: {std::pair{0xc4,3}, {0xc5,1},{0xc6,10},{0x43,2},{0xc7,1},{0xc8,2},{0xc9,3},
                              {0xc0,2},{0xc1,0},{0xc2,0},{0xc3,0},{0x46,7},{0x47,3}}) c.put8(object+offset,value);
    c.set(6,c.u16(object+0x40)); p.call(0x107156f0,0x106eab70,{f.at(0x38)});
    c.set({{4,signed_half(c.u16(f.at(0x38)))},{3,object},{5,object+0xa6},{6,object+0x3a},{7,0}});
    c.put16(object+0xb6,c.r(4)); c.put16(object+0xb8,0); c.set(4,~0u);
    p.virtual_call(object,0x44,0x106eaba0); c.put32(object+0xbc,c.r(3)); return 1;
}
void finalize(Cpu& c, std::uint32_t object) {
    Frame f(c,0x106eabc0,0x40,{}); Program p{c}; c.set(3,c.u32(c.u32(object+0xc)+4)); p.virtual_call(c.r(3),0x38,0x106eabe0);
}
void next_season(Cpu& c, std::uint32_t object) {
    Frame f(c,0x106eada0,0x80,{.first_gpr=27}); Program p{c};
    c.set({{30,c.toc(-0x7ba8)},{27,object},{31,c.toc(-0x4094)},{4,0}}); c.put8(object+0x49,0);
    p.call(0x104a6660,0x106eadcc); c.set(3,c.u32(object+0xbc)); c.compare_logical(0,c.r(3),0);
    if (c.r(3)) { p.call(0x10775170,0x106eaddc); c.put32(object+0xbc,0); }
    c.set(3,c.u32(object+8)); c.compare_logical(0,c.r(3),0);
    if (c.r(3)) p.call(0x104564a0,0x106eadf8,{c.r(3),1});
    c.set({{28,0},{29,0}});
    for (std::uint32_t index=0;;++index) {
        c.compare(0,std::int32_t(index),c.i32(object+0x30)); if (std::int32_t(index)>c.i32(object+0x30)) break;
        c.set(3,c.u32(c.u32(object+0xc)+index*4)); c.compare_logical(0,c.r(3),0);
        if (c.r(3)) { c.set(4,1); p.virtual_call(c.r(3),8,0x106eae24); }
        c.set({{29,(index+1)*4},{28,index+1}});
    }
    c.set(3,~0u); c.put32(object+0x30,~0u); c.set({{3,object},{4,std::uint32_t(std::int32_t(c.i16(object+0x40)))+2}});
    c.put16(object+0x40,c.r(4)); c.put8(object+0x45,0); p.virtual_call(object,0x94,0x106eae68);
    c.compare(0,std::int32_t(c.r(3)),0);
    if (!c.r(3)) { assertion(c,f,0x106eae88,0x40,0x38,0x2fd); c.set(3,c.toc(-0x7bac)); c.put8(c.r(3),0); }
}
void advance_stage(Cpu& c, std::uint32_t object) {
    Frame f(c,0x106eaee0,0x70,{.first_gpr=30}); c.set({{31,c.toc(-0x4094)},{30,c.toc(-0x7ba8)}});
    c.set({{4,c.u32(object+0x2c)},{5,c.u32(object+0x30)}});
    const auto last=c.r(4)-1; c.compare(0,std::int32_t(c.r(5)),std::int32_t(last));
    if (std::int32_t(c.r(5))>=std::int32_t(last)) {
        assertion(c,f,0x106eaf24,0x40,0x38,0x30f); c.set(3,c.toc(-0x7bac)); c.put8(c.r(3),0); return;
    }
    const auto next=c.r(5)+1; c.put32(object+0x30,next); c.compare(0,std::int32_t(next),1);
    if (next==1) { c.link(0x106eaf84); create_finals(c,object); }
}
}
