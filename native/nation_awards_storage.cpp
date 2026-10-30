#include "nation_awards_detail.hpp"

namespace cm::aot::nation_awards {
using namespace detail;
std::uint32_t load(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x10446b00, 0x80, {.first_gpr = 28}); Program p{c};
    keep(c, {{28,manager},{29,file},{30,c.toc(-0x7ba8)},{31,c.toc(-0x4a18)}});
    p.virtual_call(manager, 8, 0x10446b38);
    p.call(0x10747a80, 0x10446b50, {frame.at(0x38),1,1,file});
    if (compare(c,c.r(3),1) != 0) {
        assertion(c,frame,0x10446b6c,0x44,0x3c,0x80b);
        c.set(4,c.toc(-0x7bac)); c.put8(c.r(4),0);
        return result(c,0);
    }
    if (compare(c,std::int8_t(c.u8(frame.at(0x38)))) != 0) {
        const auto month=p.call(0x10773520,0x10446bc8,{16}); c.set(30,month);
        if (compare(c,month) != 0) p.call(0x10443850,0x10446bd8,{month,file});
        c.put32(manager+12,c.r(30));
    }
    return result(c,1);
}
void unused(Cpu&) {}
void save(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c,0x10446c10,0x80,{.first_gpr=26}); Program p{c};
    keep(c,{{26,manager},{27,file},{28,0},{29,0},{30,c.toc(-0x7ba8)},{31,c.toc(-0x4a18)}});
    c.set(3,c.u32(manager+12)); c.put8(frame.at(0x38),c.r(3)!=0);
    for (unsigned index=0; ; ++index) {
        c.set(3,extended(std::int16_t(index)));
        if (compare(c,c.r(3),c.i16(manager+8))>=0) break;
        const auto award=c.u32(c.u32(manager+4)+index*4);
        c.set(3,award); c.set(4,file); p.virtual_call(award,12,0x10446c68);
        keep(c,{{28,index+1},{29,(index+1)*4}});
    }
    p.call(0x10747cc0,0x10446c98,{frame.at(0x38),1,1,file});
    if (compare(c,c.r(3),1)!=0) {
        assertion(c,frame,0x10446cb4,0x44,0x3c,0x82b);
        c.set(3,c.toc(-0x7bac)); c.put8(c.r(3),0);
    } else {
        c.set(3,c.u32(manager+12));
        if (compare_logical(c,c.r(3))!=0) p.call(0x10443c20,0x10446d0c,{c.r(3),file});
    }
}
std::uint32_t compare_appearances(Cpu& c,std::uint32_t left,std::uint32_t right) {
    return result(c,std::uint32_t(c.u8(right+8))-c.u8(left+8));
}
std::uint32_t compare_score(Cpu& c,std::uint32_t left,std::uint32_t right) {
    Fpu fp(c); c.set_f(1,c.f32(left+4)); c.set_f(0,c.f32(right+4));
    fp.fcmp(0,c.f(1),c.f(0));
    return result(c,(c.cr(0)&8) ? missing : ((c.cr(0)&4)!=0));
}
std::uint32_t compare_score_reverse(Cpu& c,std::uint32_t left,std::uint32_t right) {
    Fpu fp(c); c.set_f(1,c.f32(left+4)); c.set_f(0,c.f32(right+4));
    fp.fcmp(0,c.f(1),c.f(0));
    return result(c,(c.cr(0)&4) ? missing : ((c.cr(0)&8)!=0));
}
}
