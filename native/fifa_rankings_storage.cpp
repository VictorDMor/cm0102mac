#include "fifa_rankings_detail.hpp"

namespace cm::aot::fifa_rankings {
using namespace detail;
std::uint32_t construct(Cpu& c,unsigned manager,unsigned file) {
    Frame frame(c,0x104866f0,0x50,{.first_gpr=29}); Program p{c};
    keep(c,{{29,manager},{30,file},{31,c.toc(-0x7a9c)}});
    p.call(0x10715880,0x1048671c,{manager+8});
    p.call(0x10718360,0x10486724,{c.r(31)});
    c.put8(manager+4,c.r(3)); compare_logical(c,file);
    copy_date(c,manager+8,c.r(31));
    if (file) {
        enter(c,0x10486754,{manager,file}); c.set(3,load(c,manager,file));
        if (compare(c,c.r(3))!=0) return result(c,manager);
    }
    enter(c,0x1048676c,{manager}); initialize(c,manager);
    return result(c,manager);
}
std::uint32_t destroy(Cpu& c,unsigned manager,unsigned deleting) {
    Frame frame(c,0x10486790,0x50,{.first_gpr=27}); Program p{c}; Manager m{c,manager};
    compare(c,manager); keep(c,{{27,manager},{28,deleting},{31,c.toc(-0x7b04)}});
    if (manager) {
        if (compare_logical(c,m.records())!=0) {
            keep(c,{{29,0},{30,0}});
            for (unsigned index=0;compare(c,index,c.i32(c.r(31)))<0;++index) {
                c.set(3,m.at(index).matches());
                if (compare_logical(c,c.r(3))!=0) p.call(0x10775170,0x104867e0);
                keep(c,{{29,index+1},{30,(index+1)*Ranking::size}});
            }
            p.call(0x10775170,0x104867fc,{m.records()});
        }
        if (compare(c,std::int16_t(deleting))>0) p.call(0x107735a0,0x1048680c,{manager});
    }
    return result(c,manager);
}
std::uint32_t load(Cpu& c,unsigned manager,unsigned file) {
    Frame frame(c,0x104871e0,0x1a0,{.first_gpr=25}); Program p{c}; Manager m{c,manager};
    keep(c,{{25,manager},{28,c.toc(-0x7bac)},{29,c.toc(-0x7ba8)},
            {30,c.toc(-0x47b4)},{31,c.toc(-0x7b04)}});
    const auto fail=[&](unsigned at,unsigned name,unsigned ext,unsigned line) {
        assertion(c,frame,at,name,ext,line); return result(c,0);
    };
    p.call(0x107475b0,0x1048721c,{file,c.r(30)+0x25,0,0x16,4});
    compare_logical(c,c.r(3)); c.put32(frame.at(0x38),c.r(3));
    if (!c.r(3)) return fail(0x10487228,0x15c,0x74,0x23a);
    c.put32(manager,p.call(0x10775130,0x1048728c,{c.u32(c.r(31))*Ranking::size}));
    c.set(3,m.records());
    if (compare_logical(c,c.r(3))==0) return fail(0x1048729c,0x13c,0x6c,0x242);
    p.call(0x10747a80,0x10487304,{c.r(3),Ranking::size,c.u32(c.r(31)),c.u32(frame.at(0x38))});
    if (compare(c,c.i32(c.r(31)),c.r(3))!=0) return fail(0x10487310,0x11c,0x64,0x249);
    keep(c,{{26,0},{27,0}});
    for (unsigned index=0;compare(c,index,c.i32(c.r(31)))<0;++index) {
        const auto row=m.at(index);
        p.call(0x10747a80,0x10487390,{row.address+8,2,1,c.u32(frame.at(0x38))});
        if (compare(c,c.r(3),1)!=0) return fail(0x10487398,0xfc,0x5c,0x253);
        row.count()=std::int16_t(row.capacity()); c.set(4,row.address); c.set(3,extended(row.capacity()));
        if (compare(c,std::int16_t(c.r(3)))!=0) {
            row.matches()=p.call(0x10775130,0x1048741c,{c.r(3)*Ranking::match_size});
            c.set(4,m.at(index).address); c.set(3,m.at(index).matches());
            if (compare_logical(c,c.r(3))==0) return fail(0x1048743c,0xdc,0x54,0x261);
            p.call(0x10747a80,0x104874a4,{c.r(3),Ranking::match_size,extended(m.at(index).capacity()),c.u32(frame.at(0x38))});
            c.set(4,m.records());
            if (compare(c,m.at(index).capacity(),c.r(3))!=0) return fail(0x104874b8,0xbc,0x4c,0x268);
        } else row.matches()=0;
        keep(c,{{26,index+1},{27,(index+1)*Ranking::size}});
    }
    p.call(0x10747a80,0x10487540,{manager+4,1,1,c.u32(frame.at(0x38))});
    if (compare(c,c.r(3),1)!=0) return fail(0x10487548,0x9c,0x44,0x276);
    p.call(0x10747a80,0x104875b4,{manager+8,8,1,c.u32(frame.at(0x38))});
    if (compare(c,c.r(3),1)!=0) return fail(0x104875bc,0x7c,0x3c,0x27d);
    p.call(0x10747900,0x1048761c,{frame.at(0x38)});
    return result(c,1);
}
void save(Cpu& c,unsigned manager,unsigned file) {
    Frame frame(c,0x10487640,0x150,{.first_gpr=25}); Program p{c}; Manager m{c,manager};
    keep(c,{{28,manager},{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},
            {31,c.toc(-0x47b4)},{27,c.toc(-0x7b04)}});
    p.call(0x107475b0,0x1048767c,{file,c.r(31)+0x25,1,0x16,4});
    c.set(6,c.r(3)); compare(c,c.r(6)); c.put32(frame.at(0x38),c.r(3));
    if (!c.r(6)) { assertion(c,frame,0x10487688,0x10c,0x64,0x299); return; }
    p.call(0x10747cc0,0x104876ec,{m.records(),Ranking::size,c.u32(c.r(27))});
    if (compare(c,c.i32(c.r(27)),c.r(3))!=0) { assertion(c,frame,0x104876f8,0xec,0x5c,0x2a0); return; }
    keep(c,{{25,0},{26,0}});
    for (unsigned index=0;compare(c,index,c.i32(c.r(27)))<0;++index) {
        p.call(0x10747cc0,0x10487774,{m.at(index).address+10,2,1,c.u32(frame.at(0x38))});
        if (compare(c,c.r(3),1)!=0) { assertion(c,frame,0x1048777c,0xcc,0x54,0x2aa); return; }
        p.call(0x10747cc0,0x104877ec,{m.at(index).matches(),Ranking::match_size,extended(m.at(index).count()),c.u32(frame.at(0x38))});
        c.set(4,m.records());
        if (compare(c,m.at(index).count(),c.r(3))!=0) { assertion(c,frame,0x10487800,0xac,0x4c,0x2b1); return; }
        keep(c,{{25,index+1},{26,(index+1)*Ranking::size}});
    }
    p.call(0x10747cc0,0x10487880,{manager+4,1,1,c.u32(frame.at(0x38))});
    if (compare(c,c.r(3),1)!=0) { assertion(c,frame,0x10487888,0x8c,0x44,0x2b9); return; }
    p.call(0x10747cc0,0x104878f0,{manager+8,8,1,c.u32(frame.at(0x38))});
    if (compare(c,c.r(3),1)!=0) { assertion(c,frame,0x104878f8,0x6c,0x3c,0x2c0); return; }
    p.call(0x10747900,0x10487954,{frame.at(0x38)});
}
void append(Cpu& c,unsigned manager,unsigned nation,unsigned points,unsigned match) {
    Frame frame(c,0x10487ae0,0x60,{.first_gpr=27}); Program p{c}; Manager m{c,manager};
    keep(c,{{27,manager},{28,nation},{29,points},{30,match}});
    const auto row=m.nation(nation); c.set(3,m.records());c.set(4,row.address);
    if (compare(c,row.count(),row.capacity())==0) {
        row.capacity()=std::int16_t(unsigned(row.capacity())+5);
        c.set(4,row.address);c.set(3,row.matches());
        if (compare_logical(c,c.r(3))!=0)
            row.matches()=p.call(0x107751b0,0x10487b50,{c.r(3),extended(row.capacity())*6});
        else row.matches()=p.call(0x10775130,0x10487b74,{extended(row.capacity())*6});
        c.set(4,m.nation(nation).address);
    }
    c.set(3,frame.at(0x38));c.set(4,m.records());c.set(5,row.address);
    const auto index=std::uint32_t(std::int32_t(row.count()));
    row.count()=std::int16_t(index+1); c.set(31,index*6);
    c.set(4,row.matches());c.put16(c.r(4)+index*6,points);
    p.call(0x107156b0,0x10487bd4,{frame.at(0x38),extended(std::int16_t(c.u16(match+0x2a))),c.u16(match+0x28)});
    p.call(0x10718360,0x10487bd8);
    c.set(4,row.matches()+index*6);c.put8(c.r(4)+2,c.r(3));
    c.set(3,row.matches()+index*6);c.set(4,c.u16(match+0x28));c.put16(c.r(3)+4,c.r(4));
}
}
