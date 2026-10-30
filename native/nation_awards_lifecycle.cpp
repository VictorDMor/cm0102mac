#include "nation_awards_detail.hpp"

namespace cm::aot::nation_awards {
using namespace detail;
std::uint32_t construct(Cpu& c,std::uint32_t manager) {
    c.set(4,c.toc(-0x61cc)); c.put32(manager,c.r(4)); c.put32(manager+4,0);
    c.put16(manager+8,0); c.put32(manager+12,0);
    return result(c,manager);
}
std::uint32_t destroy(Cpu& c,std::uint32_t manager,std::uint32_t deleting) {
    Frame frame(c,0x104443b0,0x50,{.first_gpr=28}); Program p{c};
    keep(c,{{28,manager},{29,deleting}});
    if (compare(c,manager)!=0) {
        c.put32(manager,c.toc(-0x61cc)); keep(c,{{30,0},{31,0}});
        for (unsigned index=0;compare(c,std::int16_t(index),c.i16(manager+8))<0;++index) {
            c.set(3,c.u32(c.u32(manager+4)+index*4));
            if (compare_logical(c,c.r(3))!=0) {
                c.set(4,1); p.virtual_call(c.r(3),8,0x10444410);
            }
            keep(c,{{30,index+1},{31,(index+1)*4}});
        }
        c.set(3,c.u32(manager+4));
        if (compare_logical(c,c.r(3))!=0) p.call(0x10775170,0x1044443c);
        c.set(3,c.u32(manager+12));
        if (compare_logical(c,c.r(3))!=0) p.call(0x104438b0,0x10444450,{c.r(3),1});
        if (compare(c,std::int16_t(deleting))>0) p.call(0x107735a0,0x10444460,{manager});
    }
    return result(c,manager);
}
void update(Cpu& c,std::uint32_t manager) {
    Frame frame(c,0x10444490,0x50,{.first_gpr=30}); Program p{c};
    c.set(5,1); c.set(30,manager);
    p.call(0x10717890,0x104444b4,{frame.at(0x38)});
    leave(c,{{4,std::uint32_t(c.i16(frame.at(0x38)))},{3,c.u16(frame.at(0x3a))}});
    c.put16(frame.at(0x40),c.r(4)); c.put16(frame.at(0x42),c.r(3));
    c.put8(frame.at(0x44),c.u8(frame.at(0x3c)));
    c.set(3,c.u32(manager+12));
    if (compare_logical(c,c.r(3))==0) return;
    c.set(31,c.u8(c.r(3)+12));
    p.call(0x10718360,0x104444e4,{frame.at(0x40)});
    c.set(3,std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c,std::int8_t(c.r(31)),c.r(3))==0) {
        c.set(3,std::uint32_t(c.i16(c.u32(manager+12)+14)));
        if (compare(c,c.r(3),c.u16(frame.at(0x42)))==0) return;
    }
    c.set(3,manager); p.virtual_call(manager,0x14,0x1044451c);
    p.call(0x10718360,0x10444528,{frame.at(0x40)});
    p.call(0x10443f00,0x10444538,{c.u32(manager+12),std::uint32_t(std::int8_t(c.r(3))),c.u16(frame.at(0x42))});
}
void collect_month(Cpu& c,std::uint32_t manager) {
    Frame frame(c,0x10444550,0x40,{}); Program p{c};
    c.set(3,c.u32(manager+12));
    if (compare_logical(c,c.r(3))!=0) p.call(0x10443e20,0x1044456c);
}
}
