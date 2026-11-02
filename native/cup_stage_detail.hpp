#pragma once
#include "cup_stage.hpp"
#include "guest_values.hpp"
#include "routine_check.hpp"

namespace cm::aot::cup_stage::detail {
using namespace guest;
namespace slot {
constexpr int pool=-0x4674, doubles=-0x467c, vtable=-0x613c, flag=-0x7bac,
    message=-0x7ba8, clubs=-0x7be8, definitions=-0x7bbc,
    rng_seed=-0x7b90, rng_start=-0x7b8c, rng_end=-0x7b88, rng_cursor=-0x7b84;
}
struct Stage {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t rounds() const { return c.u32(address+0xa0); }
    std::uint32_t entrants() const { return c.u32(address+0xa4); }
    std::uint32_t matches() const { return c.u32(address+0xac); }
    std::int16_t round_count() const { return c.i16(address+0x3c); }
    std::int16_t entrant_count() const { return c.i16(address+0x3e); }
    std::uint32_t round(std::uint32_t index) const { return rounds()+index*0x78; }
    std::uint32_t match(std::uint32_t round, std::uint32_t index) const {
        return c.u32(matches()+round*4)+index*0x36;
    }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target,std::uint32_t back,
                       std::initializer_list<std::uint32_t> args={}) const {
        enter(c,back,args);c.call(target,back);return c.r(3);
    }
    std::uint32_t virtual_call(std::uint32_t object,unsigned offset,std::uint32_t back) const {
        c.set(12,c.u32(c.u32(object)+offset));c.call(0x10781320,back);
        c.set(2,c.u32(c.sp()+20));return c.r(3);
    }
};
inline void assertion(Cpu& c,const Frame& f,std::uint32_t at,unsigned name,unsigned extension,unsigned line) {
    const auto pool=c.toc(slot::pool);
    report_assertion(c,{at,pool,pool+14,pool+27,f.at(name),f.at(extension),line});
}
inline void reloading_assertion(Cpu& c,const Frame& f,std::uint32_t at,unsigned name,unsigned extension,unsigned line) {
    const auto pool=c.toc(slot::pool);
    report_assertion(c,{at,pool,pool+14,pool+27,f.at(name),f.at(extension),line},
        [&](std::uint32_t entry,std::uint32_t back) {
            c.call(entry,back==at+0x24?at+0x28:back==at+0x34?at+0x3c:back);
        });
    c.set(4,c.toc(slot::flag));
}
struct Stream {
    Cpu& c;
    std::uint32_t cursor,end,start,seed;
    void advance(std::uint32_t back) const {
        const auto next=c.u32(cursor)+2;c.set(3,next);c.put32(cursor,next);
        if(compare_logical(c,next,c.u32(end))>0) {
            c.put32(cursor,start);c.call(0x1077b770,back);c.put32(seed,c.r(3)&0xffff);
        }
    }
    std::uint32_t sample() const {return std::uint32_t(c.u16(c.u32(cursor)))+c.u32(seed);}
};
}
