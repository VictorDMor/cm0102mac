#include "euro_champ_qual_detail.hpp"
namespace cm::aot::euro_champ_qual {
using namespace detail;
std::uint32_t group_size(Cpu& c,std::uint32_t object,std::uint32_t group) {
    Frame f(c,0x104faf40,0x70,{.first_gpr=30});
    keep(c,{{31,c.toc(slot::pool)},{30,c.toc(-0x7ba8)}});
    const auto hosts=c.i8(object+0xf4), index=std::int8_t(group);
    if(compare(c,hosts,2)==0) {
        if(compare(c,index)==0 || compare(c,index,2)==0 || compare(c,index,5)==0 || compare(c,index,7)==0) return 6;
        return 5;
    }
    if(hosts<2) {
        if(compare(c,hosts,1)>=0) {
            if(compare(c,index)==0 || compare(c,index,2)==0 || compare(c,index,5)==0) return 6;
            return 5;
        }
    }
    assertion(c,f,0x104faff0,0x40,0x38,0x244); c.set(4,c.toc(-0x7bac)); return 5;
}
std::uint32_t configure(Cpu& c,std::uint32_t object) {
    Frame f(c,0x104fc3c0,0x50,{.first_gpr=30}); Program p{c};
    keep(c,{{30,object},{31,2}}); c.put16(object+0x3c,2);
    for(auto [offset,value]:{std::pair{0xc4u,3u},{0xc5,1},{0xc6,10},{0x43,2},{0xc7,1},{0xc8,2},{0xc9,4},{0xca,5},{0xc0,1},{0xc1,1},{0xc2,0},{0xc3,0},{0x46,7},{0x47,3}}) c.put8(object+offset,value);
    c.put16(object+0xa4,0xffff);
    p.call(0x107156f0,0x104fc458,{f.at(0x38),10,2,c.u16(object+0x40),5,0,5,4});
    c.put16(object+0xb6,c.u16(f.at(0x38))); c.put16(object+0xb8,0);
    leave(c,{{3,object},{4,~0u},{5,object+0xa6},{6,object+0x3a},{7,0}});
    c.put32(object+0xbc,p.virtual_call(object,0x44,0x104fc488)); return 1;
}
std::uint32_t mark_active(Cpu& c,std::uint32_t object) { c.put8(object+0xb4,1); return 1; }
void next_season(Cpu& c,std::uint32_t object) {
    Frame f(c,0x104fc5e0,0x80,{.first_gpr=26}); Program p{c}; Competition cup{c,object};
    keep(c,{{28,c.toc(-0x7b80)},{29,object},{30,c.toc(-0x7ba8)},{31,c.toc(slot::pool)}});
    c.put8(object+0x49,0); p.call(0x104a6660,0x104fc610,{object,0});
    c.set(3,c.u32(object+0xbc));
    if(compare_logical(c,c.r(3))!=0) { p.call(0x10775170,0x104fc620); c.put32(object+0xbc,0); }
    Kept<26> stage(c,0); Kept<27> offset(c,0);
    while(compare(c,stage,cup.current_stage())<=0) {
        const auto group=c.u32(std::uint32_t(cup.groups())+std::uint32_t(offset)); c.set(3,group);
        if(compare_logical(c,group)!=0) { c.set(4,1); p.virtual_call(group,8,0x104fc658); }
        offset=std::uint32_t(offset)+4; stage=std::uint32_t(stage)+1;
    }
    cup.current_stage()=-1; c.set(3,c.u32(object+0x10));
    if(compare_logical(c,c.r(3))!=0) p.call(0x10738ca0,0x104fc688);
    c.set(3,cup.entrants());
    if(compare_logical(c,c.r(3))!=0) { p.call(0x10775170,0x104fc698); cup.entrants()=0; c.put32(object+0xf0,0); }
    c.set(3,signed_half(c.u16(object+0x40)));
    while(compare(c,c.i16(object+0x40),c.u16(c.r(28)+2))<0) {
        c.set(3,signed_half(c.u16(object+0x40))); cup.year()=std::int16_t(std::uint32_t(c.r(3))+4);
    }
    for(unsigned offset:{0x45,0xb4,0xfc}) c.put8(object+offset,0);
    c.set(3,c.u32(object+8)); if(compare_logical(c,c.r(3))!=0) p.call(0x104564a0,0x104fc6e8,{c.r(3),1});
    c.set(3,object);
    if(compare(c,p.virtual_call(object,0x94,0x104fc6f8))==0) {
        assertion(c,f,0x104fc718,0x40,0x38,0x377); c.set(3,c.toc(-0x7bac));
    }
}
void next_round(Cpu& c,std::uint32_t object) {
    Frame f(c,0x104fc770,0x70,{.first_gpr=30});
    keep(c,{{31,c.toc(slot::pool)},{30,c.toc(-0x7ba8)}});
    const auto count=c.u32(object+0x2c); const auto stage=c.i32(object+0x30);
    leave(c,{{4,count},{5,std::uint32_t(stage)}});
    if(compare(c,stage,std::int32_t(count-1))>=0) { assertion(c,f,0x104fc7b4,0x40,0x38,0x38a); c.set(3,c.toc(-0x7bac)); return; }
    c.put32(object+0x30,std::uint32_t(stage)+1); c.set(4,std::uint32_t(stage)+1);
    if(compare(c,c.r(4),c.i8(object+0xfe))!=0 && compare(c,c.r(4),c.i8(object+0xff))!=0) return;
    enter(c,0x104fc828); create_playoffs(c,object);
}
void record_stage(Cpu& c,std::uint32_t object,std::uint32_t stage,std::uint32_t record) {
    Frame f(c,0x104fdfc0,0x50,{.first_gpr=29}); Program p{c}; keep(c,{{29,object},{30,stage},{31,record}});
    c.set(3,signed_byte(c.u8(stage+0x42)));
    if(compare(c,c.i8(stage+0x42),c.i8(object+0xfd))<=0) p.call(0x10738d10,0x104fe008,{c.u32(object+0x10),stage,0x50});
    p.call(0x104a3930,0x104fe018,{object,stage,record});
}
void clear_record(Cpu& c,std::uint32_t object) {
    Frame f(c,0x104fab00,0x40,{}); Program p{c}; c.set(5,c.toc(-0x7a64));
    p.call(0x102b7750,0x104fab1c,{c.u32(c.r(5)),c.u32(object+4)});
}
std::uint32_t definition(Cpu& c) {
    c.set(3,c.toc(-0x6948)); c.set(4,c.toc(-0x7bbc)); return c.u32(c.r(4))+c.u32(c.r(3))*0x6c;
}
void finalize(Cpu& c,std::uint32_t object) {
    Frame f(c,0x104fe040,0x40,{.first_gpr=31}); Kept<31> saved_object(c,object); Program p{c};
    p.call(0x104a1c30,0x104fe058); if(compare(c,c.i8(object+0xfc))!=0) return;
    leave(c,{{3,object},{4,1},{5,0}}); p.virtual_call(object,0x18,0x104fe07c);
    if(compare(c,std::int8_t(c.r(3)))==0) return;
    enter(c,0x104fe090,{object}); publish_qualified(c,object);
}
void publish_qualified(Cpu& c,std::uint32_t object) {
    Frame f(c,0x104fe0b0,0x60,{.first_gpr=24}); Kept<27> saved_object(c,object); Program p{c};
    if(compare(c,c.i8(object+0xfc))!=0) return;
    c.put8(object+0xfc,1); Kept<29> group(c,~0u); Kept<31> offset(c,~3u);
    while(compare(c,group,c.i8(object+0xfd))<=0) {
        if(compare(c,group,-1)==0) c.set(30,object); else c.set(30,c.u32(c.u32(object+0xc)+std::uint32_t(offset)));
        Kept<28> entrant(c,0); Kept<25> entrant_offset(c,0);
        for(;;) {
            const auto finals=c.u32(c.u32(object+0xc)+std::uint32_t(c.i8(object+0xff))*4); c.set(3,finals);
            if(compare(c,entrant,c.i16(finals+0x3e))>=0) break;
            Kept<26> selected(c,c.u32(finals+0xa4)+std::uint32_t(entrant_offset));
            if(compare(c,c.u8(std::uint32_t(selected)+5),1)==0) {
                Kept<24> candidate(c,0);
                while(compare(c,candidate,c.i16(c.r(30)+0x3e))<0) {
                    p.call(0x1049ea00,0x104fe134,{c.r(30),candidate}); c.set(3,c.u32(c.r(3)));
                    if(compare_logical(c,c.r(3),c.u32(selected))==0) {
                        selected=7; p.call(0x1049ea00,0x104fe154,{c.r(30),candidate}); c.put8(c.r(3)+0x40,selected); break;
                    }
                    candidate=std::uint32_t(candidate)+1;
                }
            }
            entrant_offset=std::uint32_t(entrant_offset)+8; entrant=std::uint32_t(entrant)+1;
        }
        offset=std::uint32_t(offset)+4; group=std::uint32_t(group)+1;
    }
}
}
