#include "dispute_detail.hpp"

namespace cm::aot::dispute {
using namespace detail;
std::uint32_t construct(Cpu& c,std::uint32_t object,std::uint32_t file) {
    Frame frame(c,0x1070bd00,0x40,{.first_gpr=30});
    Program p{c}; Manager manager{c,object};
    keep(c,{{30,object},{31,c.toc(-0x7be4)}});
    compare_logical(c,file); manager.flags()=0; manager.records()=0;
    if (file) {
        enter(c,0x1070bd34,{object,file}); c.set(3,load(c,object,file));
        if (compare(c,std::int8_t(c.r(3)))!=0) return result(c,object);
        c.set(3,manager.flags());
        if (compare_logical(c,c.r(3))!=0) { p.call(0x10775170,0x1070bd4c); manager.flags()=0; }
        c.set(3,manager.records());
        if (compare_logical(c,c.r(3))!=0) { p.call(0x10738c20,0x1070bd68,{c.r(3),1}); manager.records()=0; }
    }
    const auto calloc_back=file?0x1070bd7c:0x1070bda8;
    manager.flags()=p.call(0x10775200,calloc_back,{c.u32(c.r(31)),1});
    const auto list=p.call(0x10773520,file?0x1070bd88:0x1070bdb4,{16});
    c.set(31,list);
    if (compare(c,list)!=0) p.call(0x10738c00,file?0x1070bd94:0x1070bdc0);
    manager.records()=c.r(31);
    return result(c,object);
}
std::uint32_t destroy(Cpu& c,std::uint32_t object,std::uint32_t deleting) {
    Frame frame(c,0x1070bde0,0x40,{.first_gpr=30}); Program p{c};
    keep(c,{{30,object},{31,deleting}});
    if (compare(c,object)!=0) {
        Manager manager{c,object}; c.set(3,manager.flags());
        if (compare_logical(c,c.r(3))!=0) { p.call(0x10775170,0x1070be10); manager.flags()=0; }
        c.set(3,manager.records());
        if (compare_logical(c,c.r(3))!=0) { p.call(0x10738c20,0x1070be2c,{c.r(3),1}); manager.records()=0; }
        if (compare(c,std::int16_t(deleting))>0) p.call(0x107735a0,0x1070be44,{object});
    }
    return result(c,object);
}
std::uint32_t load(Cpu& c,std::uint32_t object,std::uint32_t file) {
    Frame frame(c,0x1070be60,0x60,{.first_gpr=30}); Program p{c}; Manager manager{c,object};
    keep(c,{{30,object},{31,file}});
    p.call(0x10715880,0x1070be84,{frame.at(0x4c)});
    p.call(0x10775550,0x1070be90,{indirect(c,-0x7b74)});
    c.set(4,0x40000);
    if (compare(c,c.r(3),0x3e046)<0) return result(c,0);
    if (compare_logical(c,file)==0) return result(c,0);
    p.call(0x107475b0,0x1070bed0,{file,c.toc(-0x4004),0,0x16,4});
    compare_logical(c,c.r(3)); c.put32(frame.at(0x40),c.r(3));
    if (!c.r(3)) return result(c,0);
    const auto close=[&](unsigned back,unsigned value) {
        p.call(0x10747900,back,{frame.at(0x40)}); return result(c,value);
    };
    p.call(0x10747a80,0x1070bef8,{frame.at(0x3c),4,1,c.r(3)});
    if (compare(c,c.r(3),1)!=0) return close(0x1070bf08,0);
    c.set(3,c.u32(frame.at(0x3c)));
    if (compare(c,c.r(3))==0) return close(0x1070bf24,0);
    manager.flags()=p.call(0x10775130,0x1070bf30);
    c.set(3,manager.flags());
    if (compare_logical(c,c.r(3))==0) return close(0x1070bf48,0);
    p.call(0x10747a80,0x1070bf60,{c.r(3),1,c.u32(frame.at(0x3c)),c.u32(frame.at(0x40))});
    if (compare(c,c.u32(frame.at(0x3c)),c.r(3))!=0) return close(0x1070bf74,0);
    p.call(0x10747a80,0x1070bf90,{frame.at(0x38),4,1,c.u32(frame.at(0x40))});
    if (compare(c,c.r(3),1)!=0) return close(0x1070bfa0,0);
    c.set(31,p.call(0x10773520,0x1070bfb0,{16}));
    if (compare(c,c.r(31))!=0) p.call(0x10738c00,0x1070bfbc);
    manager.records()=c.r(31); c.set(31,0);
    while (compare(c,c.r(31),c.u32(frame.at(0x38)))<0) {
        p.call(0x10747a80,0x1070bfdc,{frame.at(0x44),20,1,c.u32(frame.at(0x40))});
        if (compare(c,c.r(3),1)!=0) return close(0x1070bfec,0);
        p.call(0x10738d10,0x1070c008,{manager.records(),frame.at(0x44),20});
        c.set(31,c.r(31)+1);
    }
    return close(0x1070c020,1);
}
std::uint32_t save(Cpu& c,std::uint32_t object,std::uint32_t file) {
    Frame frame(c,0x1070c040,0x170,{.first_gpr=27}); Program p{c}; Manager manager{c,object};
    keep(c,{{27,object},{28,c.toc(-0x7be4)},{29,c.toc(-0x7bac)},
            {30,c.toc(-0x7ba8)},{31,c.toc(-0x4004)}});
    const auto fail=[&](unsigned at,int name,int extension,unsigned line,unsigned close_back=0) {
        assertion(c,frame,at,name,extension,line);
        if (close_back) p.call(0x10747900,close_back,{frame.at(0x3c)});
        return result(c,0);
    };
    if (compare_logical(c,file)==0) return fail(0x1070c080,0x138,0x70,0xe9);
    p.call(0x107475b0,0x1070c0dc,{file,c.r(31),1,0x16,4});
    c.set(6,c.r(3)); compare(c,c.r(6)); c.put32(frame.at(0x3c),c.r(3));
    if (!c.r(6)) return fail(0x1070c0fc,0x118,0x68,0xf1);
    p.call(0x10747cc0,0x1070c150,{c.r(28),4,1});
    if (compare(c,c.r(3),1)!=0) return fail(0x1070c16c,0xf8,0x60,0xf8,0x1070c1b0);
    p.call(0x10747cc0,0x1070c1cc,{manager.flags(),1,c.u32(c.r(28)),c.u32(frame.at(0x3c))});
    if (compare(c,c.u32(c.r(28)),c.r(3))!=0) return fail(0x1070c1ec,0xd8,0x58,0xff,0x1070c230);
    c.set(5,manager.records()); c.put32(frame.at(0x38),std::uint32_t(c.i16(c.r(5))));
    p.call(0x10747cc0,0x1070c258,{frame.at(0x38),4,1,c.u32(frame.at(0x3c))});
    if (compare(c,c.r(3),1)!=0) return fail(0x1070c274,0xb8,0x50,0x108,0x1070c2b8);
    c.set(3,manager.records()); c.set(28,0); c.put32(c.r(3)+12,c.u32(c.r(3)+4));
    while (true) {
        c.set(3,manager.records());
        if (compare(c,c.r(28),c.i16(c.r(3)))>=0) break;
        p.call(0x10738e00,0x1070c2d8);
        if (compare_logical(c,c.r(3))==0) return fail(0x1070c2f4,0x98,0x48,0x111,0x1070c338);
        p.call(0x10747cc0,0x1070c354,{c.r(3),20,1,c.u32(frame.at(0x3c))});
        if (compare(c,c.r(3),1)!=0) return fail(0x1070c370,0x78,0x40,0x117,0x1070c3b4);
        c.set(28,c.r(28)+1);
    }
    p.call(0x10747900,0x1070c3d8,{frame.at(0x3c)});
    return result(c,1);
}
}
