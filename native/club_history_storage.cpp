#include "club_history_detail.hpp"
namespace cm::aot::club_history {
using namespace detail;
std::uint32_t construct(Cpu& c,std::uint32_t address,std::uint32_t path) {
    Frame frame(c,0x1000cbc0,0x3e0,{.first_gpr=23});
    keep(c,{{27,c.toc(-0x7bac)},{24,address},{28,c.toc(-0x7ba8)},{25,path},
            {29,c.toc(slot::pool)},{30,c.toc(-0x59c4)},{31,c.toc(-0x59c0)}});
    Program p{c};History h{c,address};
    // Recreate the original interleaved Pascal string and retry dialog.
    do {
        c.set(23,frame.at(0x51));c.set(26,0);
        c.set({{10,c.u32(c.r(31))},{9,c.u32(c.r(31)+4)},{8,c.u32(c.r(31)+8)},
               {7,c.u32(c.r(31)+12)},{6,c.u8(c.r(31)+16)},
               {5,c.u32(c.r(30))},{4,c.u32(c.r(30)+4)},{3,c.u32(c.r(30)+8)}});
        for(unsigned i=0;i<4;++i)c.put32(frame.at(0xc0+i*4),c.r(10-i));
        c.put8(frame.at(0xd0),c.r(6));
        c.put32(frame.at(0x50),c.r(5));c.put32(frame.at(0x54),c.r(4));
        c.put32(frame.at(0x58),c.r(3));c.put32(frame.at(0x5c),c.u32(c.r(30)+12));
        auto source=frame.at(0xc1),out=frame.at(0x11d);
        while(compare(c,c.i8(source))!=0) {
            c.set(3,c.u8(source++));c.put8(out,c.r(3));c.put8(out+1,c.u8(c.r(23)));
            c.set(23,c.r(23)+1);out+=2;
        }
        c.set(7,c.u8(frame.at(0xc0)));
        c.put8(frame.at(0x11c),c.r(7)+c.u8(frame.at(0x50)));
        p.call(0x1077fee0,0x1000cc94,{0,0,frame.at(0x11c),frame.at(0xd4)});
        c.set(2,c.u32(frame.at(0x14)));
        if(compare(c,std::int16_t(c.r(3)))==0)c.set(26,1);
        if(compare(c,c.r(26))==0) {
            p.call(0x10727890,0x1000ccb8,{frame.at(0x2e4),c.r(29)});
            p.call(0x10727890,0x1000ccc4,{frame.at(0x21c),c.r(29)+0xa});
            p.call(0x10769b60,0x1000ccd4,{frame.at(0x2e4),frame.at(0x21c),1});
        }
    } while(compare(c,c.r(26))==0);
    auto file=[&](unsigned offset,unsigned text,std::uint32_t alloc_back,std::uint32_t path_back,std::uint32_t init_back,unsigned size) {
        c.set(23,p.call(0x10773520,alloc_back,{8}));
        if(compare(c,c.r(23))!=0) {
            auto name=p.call(0x10746280,path_back,{c.r(29)+0x37,c.r(29)+text});
            p.call(0x10748bb0,init_back,{c.r(23),name,size});
        }
        c.put32(address+offset,c.r(23));
        return compare_logical(c,c.u32(address+offset))!=0;
    };
    if(!file(0,0x40,0x1000cce4,0x1000ccf8,0x1000cd08,24)) {
        assertion(c,frame,0x1000cd2c,0xa0,0x48,0x41);p.call(0x1076f7d0,0x1000cd70,{missing});return address;
    }
    if(!file(4,0x7c,0x1000cd80,0x1000cd94,0x1000cda4,12)) {
        assertion(c,frame,0x1000cdc8,0x80,0x40,0x49);p.call(0x1076f7d0,0x1000ce0c,{missing});return address;
    }
    if(compare_logical(c,path)!=0) {
        enter(c,0x1000ce28,{address,path});result(c,load(c,address,path));
        if(byte_nonzero(c,c.r(3)))return address;
    }
    c.set(3,c.toc(slot::clubs));h.count()=c.i32(c.r(3));
    h.index()=p.call(0x10775130,0x1000ce50,{std::uint32_t(h.count().get())*16});
    if(compare_logical(c,h.index())==0) {
        assertion(c,frame,0x1000ce74,0x60,0x38,0x5e);p.call(0x1076f7d0,0x1000ceb8,{missing});h.count()=0;return address;
    }
    c.set({{5,0},{4,0},{3,missing}});
    while(compare(c,c.r(5),h.count())<0) {
        c.set(5,c.r(5)+1);c.set(6,h.index().get()+c.r(4));c.set(4,c.r(4)+16);
        for(unsigned i=0;i<16;i+=4)c.put32(c.r(6)+i,missing);
    }
    return address;
}
std::uint32_t destroy(Cpu& c,std::uint32_t address,std::uint32_t deleting) {
    Frame f(c,0x1000cf20,0x40,{.first_gpr=30});keep(c,{{31,deleting},{30,address}});Program p{c};
    if(compare(c,address)!=0) {
        for(unsigned offset:{0u,4u}) {
            c.set(3,c.u32(address+offset));
            if(compare_logical(c,c.r(3))!=0) {p.call(0x10748c80,offset?0x1000cf70:0x1000cf54,{c.r(3),1});c.put32(address+offset,0);}
        }
        c.set(3,c.u32(address+8));
        if(compare_logical(c,c.r(3))!=0) {p.call(0x10775170,0x1000cf88);c.put32(address+8,0);c.put32(address+12,0);}
        if(compare(c,std::int16_t(deleting))>0)p.call(0x107735a0,0x1000cfa4,{address});
    }
    return address;
}
std::uint32_t load(Cpu& c,std::uint32_t address,std::uint32_t path) {
    Frame f(c,0x1000dda0,0x120,{.first_gpr=28});
    keep(c,{{31,c.toc(-0x7bac)},{30,c.toc(slot::pool)},{29,c.toc(-0x7ba8)},{28,address}});Program p{c};History h{c,address};
    if(compare_logical(c,path)==0) {assertion(c,f,0x1000dde8,0xe4,0x5c,0x2ee);return 0;}
    auto file=p.call(0x107475b0,0x1000de44,{path,c.r(30)+0x91,0,0x16,4});
    compare_logical(c,file);c.put32(f.at(0x38),file);
    if(!file){assertion(c,f,0x1000de64,0xc4,0x54,0x2f6);return 0;}
    auto fail=[&](unsigned at,unsigned name,unsigned ext,unsigned line,unsigned back) {
        assertion(c,f,at,name,ext,line);p.call(0x1076f7d0,back,{missing});
        p.call(0x10747900,back+8,{f.at(0x38)});return 0u;
    };
    if(compare(c,p.call(0x10747a80,0x1000debc,{address+12,4,1,file}),1)!=0)
        return fail(0x1000ded8,0xa4,0x4c,0x2fd,0x1000df1c);
    h.index()=p.call(0x10775130,0x1000df38,{std::uint32_t(h.count().get())*16});
    c.set(3,h.index());
    if(compare_logical(c,c.r(3))==0)return fail(0x1000df5c,0x84,0x44,0x306,0x1000dfa0);
    p.call(0x10747a80,0x1000dfc0,{h.index(),16,std::uint32_t(h.count().get()),c.u32(f.at(0x38))});
    if(compare(c,h.count(),c.r(3))!=0)return fail(0x1000dfe0,0x64,0x3c,0x30e,0x1000e024);
    p.call(0x10747900,0x1000e03c,{f.at(0x38)});return 1;
}
std::uint32_t save(Cpu& c,std::uint32_t address,std::uint32_t path) {
    Frame f(c,0x1000e060,0xf0,{.first_gpr=28});
    keep(c,{{31,c.toc(slot::pool)},{30,c.toc(-0x7ba8)},{29,c.toc(-0x7bac)},{28,address}});Program p{c};History h{c,address};
    if(compare_logical(c,path)==0){assertion(c,f,0x1000e0a8,0xbc,0x54,0x329);return 0;}
    auto file=p.call(0x107475b0,0x1000e104,{path,c.r(31)+0x91,1,0x16,4});
    compare_logical(c,file);c.put32(f.at(0x38),file);
    if(!file){assertion(c,f,0x1000e124,0x9c,0x4c,0x331);p.call(0x1076f7d0,0x1000e168,{missing});return 0;}
    auto fail=[&](unsigned at,unsigned name,unsigned ext,unsigned line,unsigned back) {
        assertion(c,f,at,name,ext,line);p.call(0x1076f7d0,back,{missing});p.call(0x10747900,back+8,{f.at(0x38)});return 0u;
    };
    if(compare(c,p.call(0x10747cc0,0x1000e184,{address+12,4,1,file}),1)!=0)
        return fail(0x1000e1a0,0x7c,0x44,0x338,0x1000e1e4);
    p.call(0x10747cc0,0x1000e208,{h.index(),16,std::uint32_t(h.count().get()),c.u32(f.at(0x38))});
    if(compare(c,h.count(),c.r(3))!=0)return fail(0x1000e228,0x5c,0x3c,0x340,0x1000e26c);
    p.call(0x10747900,0x1000e284,{f.at(0x38)});return 1;
}
std::uint32_t compare_ids(Cpu& c,std::uint32_t a,std::uint32_t b) {
    const auto left=c.i32(a),right=c.i32(b);c.set(4,std::uint32_t(right));
    if(compare(c,left,right)>0)return 1;
    // The original comparator returns 1 for unequal ids in either direction.
    const auto bits=std::uint32_t(left)^std::uint32_t(right);
    return ((std::uint32_t(std::int32_t(bits)>>1)-(bits&std::uint32_t(right)))>>31)&1;
}
}
