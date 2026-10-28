#include "euro_champ_qual_detail.hpp"
namespace cm::aot::euro_champ_qual {
using namespace detail;
std::uint32_t create_group(Cpu& c,std::uint32_t object,std::uint32_t phase,std::uint32_t entrants,std::uint32_t count) {
    Frame f(c,0x104fc840,0x100,{.first_gpr=20}); Program p{c};
    keep(c,{{20,c.r(20)},{22,count},{26,c.toc(-0x7bac)},{27,c.toc(-0x7ba8)},{28,c.toc(slot::pool)},
        {29,object},{30,phase},{31,entrants}});
    leave(c,{{3,object},{4,signed_byte(phase)},{5,f.at(0x78)},{6,f.at(0x7a)},{7,0}});
    Kept<23> schedule(c,p.virtual_call(object,0x44,0x104fc888));
    std::uint32_t entrant_count=count;
    compare_logical(c,entrants);
    if(!entrants) {
        enter(c,0x104fc8a4,{object,signed_byte(phase)}); c.set(22,result(c,group_size(c,object,phase)));
        entrant_count=c.r(22);
        Kept<25> size(c,signed_half(entrant_count));
        c.set(24,p.call(0x10775130,0x104fc8b4,{std::uint32_t(size)*4}));
        if(compare(c,c.r(24))==0) { assertion(c,f,0x104fc8d0,0xac,0x84,0x3b3); return 0; }
        compare(c,size); c.set(5,0);
        if(std::int32_t(std::uint32_t(size))>0) {
            // group_size is owned and returns only five or six; the original
            // compiler's eight-element unroll is therefore unreachable.
            compare(c,size,8);
            const auto first=(phase+1)*6, storage=c.r(24);
            c.set({{6,std::uint32_t(size)-8},{3,phase+1},{4,first}}); compare(c,0,size);
            for(unsigned i=0;i<std::uint32_t(size);++i) c.put32(storage+i*4,c.u32(c.u32(object+0xf8)+(first+i)*4));
            c.set({{3,storage+std::uint32_t(size)*4},{5,size},{6,c.u32(object+0xf8)}});
        }
    } else {
        c.set(24,entrants); if(compare_logical(c,schedule)!=0) p.call(0x10775170,0x104fca50);
        schedule=0;
    }
    Kept<25> group(c,p.call(0x10773520,0x104fca5c,{0xec}));
    if(compare(c,group)!=0) {
        keep(c,{{20,object+0xc7},{21,object+0xc0}});
        c.put32(f.at(0x38),c.u8(object+0xc4)); c.put32(f.at(0x3c),c.u8(object+0xc5));
        c.set(22,c.u8(object+0xc6)); c.put32(f.at(0x40),c.r(22));
        c.put32(f.at(0x44),c.r(20)); c.put32(f.at(0x48),c.r(21));
        c.set(22,signed_half(c.u16(object+0x40))); c.put32(f.at(0x4c),c.r(22));
        c.put32(f.at(0x50),signed_byte(phase)); c.put32(f.at(0x54),signed_half(c.u16(f.at(0x7a))));
        c.put32(f.at(0x58),c.u8(object+0x4b)); c.put32(f.at(0x5c),4); c.put32(f.at(0x60),0);
        c.put32(f.at(0x64),signed_half(c.u16(object+0xdc))); c.put32(f.at(0x68),~0u);
        c.put32(f.at(0x6c),0); c.put32(f.at(0x70),2);
        // The count is replaced by two ABI temporaries only AFTER it has
        // been passed in r5; the constructor's prologue spills r22 as year.
        p.call(0x1056dc60,0x104fcaf8,{group,object,entrant_count,c.r(24),2,c.u32(object+4),schedule,signed_half(c.u16(f.at(0x78)))});
    }
    c.set(3,c.u32(object+0xc)); c.set(20,phase*4); c.put32(c.r(3)+c.r(20),group);
    c.set(3,c.u32(object+0xc));
    if(compare_logical(c,c.u32(c.r(3)+c.r(20)))==0) { assertion(c,f,0x104fcb28,0x8c,0x7c,0x3cf); return 0; }
    if(compare_logical(c,entrants)==0) p.call(0x10775170,0x104fcb7c,{c.r(24)});
    if(compare_logical(c,schedule)!=0) p.call(0x10775170,0x104fcb8c,{schedule});
    c.put32(object+0x30,phase); p.call(0x104a0e40,0x104fcb9c,{c.u32(c.u32(object+0xc)+c.r(20))}); return 1;
}
std::uint32_t select_entrants(Cpu& c,std::uint32_t object) {
    Kept<22> entrant_offset(c,c.r(22)); Kept<24> entrant_count(c,c.r(24)); Kept<28> nation(c,c.r(28));

    Frame f(c,0x104fd320,0x110,{.first_gpr=22}); Program p{c};
    keep(c,{{22,0},{23,object},{24,0},{26,c.toc(-0x7b04)},{27,c.toc(-0x7a70)},
        {28,c.toc(-0x7b98)},{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(slot::pool)}});
    c.set(5,c.toc(-0x45a0)); c.put32(object+0xf0,51); c.put8(object+0xf4,0);
    c.set(3,c.u32(c.toc(-0x7424))); c.set(4,c.u32(c.r(5)));
    c.put32(f.at(0x58),c.r(4)); c.put32(f.at(0x5c),c.u32(c.r(5)+4));
    if(compare_logical(c,c.r(3))==0) assertion(c,f,0x104fd39c,0xc0,0x50,0x4c2);
    else {
        const auto host_count=p.call(0x1049d1c0,0x104fd400,{c.r(3),c.u32(c.toc(-0x6948)),std::uint16_t(c.i16(object+0x40)+2),f.at(0x58),f.at(0x5c),1});
        auto host_entry=f.at(0x58); std::uint32_t host_index=0;
        c.set(3,host_entry); c.set(6,host_index);
        for(;;) {
            c.set(4,signed_byte(host_index));
            if(compare(c,std::int8_t(host_index),std::int8_t(host_count))>=0) break;
            const auto host_id=c.u32(host_entry); c.set(4,host_id);
            if(compare(c,host_id,-1)==0) break;
            const auto nations=c.u32(std::uint32_t(nation)); c.set(5,nations);
            const auto continent=c.u32(nations+host_id*0x128+0x74); c.set(4,continent);
            if(compare_logical(c,continent)!=0) {
                const auto europe=c.u32(c.r(27)),host_continent=c.u32(continent+8);
                c.set(5,europe); c.set(4,host_continent);
                if(compare(c,europe,host_continent)==0) {
                    const auto hosts=c.u8(object+0xf4)+1; c.set(4,hosts); c.put8(object+0xf4,hosts);
                }
            }
            host_entry+=4; ++host_index; c.set(3,host_entry); c.set(6,host_index);
        }
    }
    c.set(3,c.u8(object+0xf4));
    if(compare(c,std::int8_t(c.r(3)))<0 || compare(c,std::int8_t(c.r(3)),2)>0) {
        assertion(c,f,0x104fd494,0xa0,0x48,0x4dc); c.set(3,0); c.put8(object+0xf4,1);
    }
    c.put32(object+0xf0,51-std::uint32_t(c.i8(object+0xf4)));
    c.set(3,c.u32(object+0xec)); if(compare_logical(c,c.r(3))!=0) p.call(0x10775170,0x104fd4f8);
    c.put32(object+0xec,p.call(0x10775130,0x104fd504,{c.u32(object+0xf0)*4}));
    if(compare_logical(c,c.u32(object+0xec))==0) { assertion(c,f,0x104fd528,0x80,0x40,0x4eb); return 0; }
    nation=c.u32(std::uint32_t(nation)); Kept<25> nation_index(c,0);
    for(;;) {
        c.set(3,signed_half(nation_index)); if(compare(c,std::int16_t(std::uint32_t(nation_index)),c.i32(c.r(26)))>=0) break;
        const auto nation_address=std::uint32_t(nation); c.set(3,c.u32(nation_address+0x74));
        if(compare_logical(c,c.r(3))!=0) {
            c.set(3,c.u32(c.r(3)+8));
            if(compare(c,c.r(3),c.i32(c.r(27)))==0 && compare(c,std::uint32_t(entrant_count),c.i32(object+0xf0))<0) {
                p.call(0x100c76c0,0x104fd5ac,{nation_address}); c.set(4,c.u32(nation_address+0x68));
                if(compare(c,c.r(4),c.i32(f.at(0x58)))!=0 && compare(c,c.r(4),c.i32(f.at(0x5c)))!=0) {
                    c.set(4,c.u32(object+0xec)); c.put32(c.r(4)+std::uint32_t(entrant_offset),c.r(3)); entrant_count=std::uint32_t(entrant_count)+1; entrant_offset=std::uint32_t(entrant_offset)+4;
                }
            }
        }
        nation_index=std::uint32_t(nation_index)+1; nation=std::uint32_t(nation)+0x128;
    }
    if(compare(c,std::uint32_t(entrant_count),c.i32(object+0xf0))<0) { assertion(c,f,0x104fd610,0x60,0x38,0x505); return 0; }
    p.call(0x10768300,0x104fd668,{c.u32(object+0xec),std::uint32_t(entrant_count),4,c.toc(-0x6188)}); return 1;
}
}
