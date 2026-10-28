#include "euro_champ_qual_detail.hpp"
namespace cm::aot::euro_champ_qual {
using namespace detail;
std::uint32_t load(Cpu& c,std::uint32_t object,std::uint32_t file) {
    Frame f(c,0x104fe1d0,0x1d0,{.first_gpr=23}); Program p{c};
    keep(c,{{23,object},{24,file},{26,c.toc(-0x7548)},{27,c.toc(-0x7be8)},{28,c.toc(-0x7bbc)},
        {29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(slot::pool)}});
    const auto read=[&](std::uint32_t at,unsigned size,std::uint32_t back,std::uint32_t assertion_at,unsigned name,unsigned extension,unsigned line) {
        const auto count=p.call(0x10775fe0,back,{at,size,1,file});
        if(compare_logical(c,count,1)==0) return true;
        assertion(c,f,assertion_at,name,extension,line); return false;
    };
    if(!read(f.at(0x38),4,0x104fe214,0x104fe230,0x134,0x6c,0x649)) return 0;
    Kept<25> record(c,0);
    while(compare(c,record,c.i32(f.at(0x38)))<0) {
        if(!read(f.at(0x154),0x50,0x104fe290,0x104fe2ac,0x114,0x64,0x651)) return 0;
        auto index=c.i32(f.at(0x160));
        if(compare(c,index,-1)==0) { assertion(c,f,0x104fe310,0xf4,0x5c,0x659); return 0; }
        c.set(4,c.u32(c.r(27))); c.set(3,c.r(4)+std::uint32_t(index)*0x248); c.put32(f.at(0x170),c.r(3));
        index=c.i32(f.at(0x164));
        if(compare(c,index,-1)==0) { assertion(c,f,0x104fe388,0xd4,0x54,0x660); return 0; }
        c.set(3,c.r(4)+std::uint32_t(index)*0x248); c.put32(f.at(0x174),c.r(3));
        index=c.i32(f.at(0x154));
        if(compare(c,index,-1)==0) { assertion(c,f,0x104fe3fc,0xb4,0x4c,0x668); return 0; }
        c.set(4,c.u32(c.r(28))); c.set(3,c.r(4)+std::uint32_t(index)*0x6c); c.put32(f.at(0x168),c.r(3));
        const auto venue=c.i32(f.at(0x158));
        if(compare(c,venue,-1)==0) c.put32(f.at(0x16c),0);
        else { c.set(3,c.u32(c.r(26))); c.put32(f.at(0x16c),c.r(3)+std::uint32_t(venue)*0x50); }
        p.call(0x10738d10,0x104fe488,{c.u32(object+0x10),f.at(0x154),0x50}); record=std::uint32_t(record)+1;
    }
    if(!read(object+0xf4,1,0x104fe4ac,0x104fe4c8,0x94,0x44,0x682)) return 0;
    if(!read(object+0xfc,1,0x104fe520,0x104fe53c,0x74,0x3c,0x689)) return 0;
    return 1;
}
void save(Cpu& c,std::uint32_t object,std::uint32_t file) {
    Frame f(c,0x104fe5a0,0x1a0,{.first_gpr=27}); Program p{c};
    keep(c,{{27,object},{28,file},{29,c.toc(-0x7ba8)},{30,c.toc(slot::pool)},{31,c.toc(-0x7bac)}});
    c.put32(f.at(0x40),0);
    const auto position=[&](std::uint32_t call,std::uint32_t back,unsigned at,std::uint32_t assertion_at,unsigned name,unsigned extension,unsigned line) {
        if(compare(c,p.call(call,back,{file,f.at(at)}))==0) return true;
        assertion(c,f,assertion_at,name,extension,line); return false;
    };
    const auto write=[&](std::uint32_t at,unsigned size,std::uint32_t back,std::uint32_t assertion_at,unsigned name,unsigned extension,unsigned line) {
        if(compare_logical(c,p.call(0x10776360,back,{at,size,1,file}),1)==0) return true;
        assertion(c,f,assertion_at,name,extension,line); return false;
    };
    if(!position(0x10777ce0,0x104fe5d8,0x3c,0x104fe5f4,0x164,0x7c,0x6a1)) return;
    if(!write(f.at(0x40),4,0x104fe648,0x104fe664,0x144,0x74,0x6ab)) return;
    const auto list=c.u32(object+0x10); c.put32(list+0xc,c.u32(list+4));
    for(;;) {
        const auto record=p.call(0x10738e00,0x104fe6dc,{c.u32(object+0x10)});
        if(compare_logical(c,record)==0) break;
        p.call(0x10776360,0x104fe6c8,{record,0x50,1,file});
        c.set(3,c.u32(f.at(0x40))); c.put32(f.at(0x40),c.r(3)+1);
    }
    if(!position(0x10777ce0,0x104fe6f0,0x38,0x104fe70c,0x124,0x6c,0x6ba)) return;
    if(!position(0x10777f30,0x104fe758,0x3c,0x104fe774,0x104,0x64,0x6c1)) return;
    if(!write(f.at(0x40),4,0x104fe7c8,0x104fe7e4,0xe4,0x5c,0x6c6)) return;
    if(!position(0x10777f30,0x104fe830,0x38,0x104fe84c,0xc4,0x54,0x6cd)) return;
    if(!write(object+0xf4,1,0x104fe8a0,0x104fe8bc,0xa4,0x4c,0x6d4)) return;
    write(object+0xfc,1,0x104fe910,0x104fe92c,0x84,0x44,0x6db);
}
}
