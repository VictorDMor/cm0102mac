#pragma once
#include "fifa_rankings.hpp"
#include "guest_values.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::fifa_rankings::detail {
using namespace guest;
struct Ranking {
    Cpu& c;
    std::uint32_t address;
    static constexpr unsigned size=44, match_size=6;
    Field<std::uint32_t> matches() const { return {c,address+4}; }
    Field<std::int16_t> capacity() const { return {c,address+8}; }
    Field<std::int16_t> count() const { return {c,address+10}; }
    Field<std::int16_t> position() const { return {c,address+16}; }
};
struct Manager {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t records() const { return c.u32(address); }
    Ranking at(unsigned index) const { return {c,records()+index*Ranking::size}; }
    Ranking nation(std::uint32_t country) const { return at(c.u32(country+0x68)); }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target,std::uint32_t back,
                       std::initializer_list<std::uint32_t> arguments={}) const {
        unsigned r=3;
        for (auto value:arguments) c.set(r++,value);
        c.call(target,back);
        return c.r(3);
    }
};
inline void assertion(Cpu& c,const Frame& frame,unsigned setup,unsigned name,unsigned extension,unsigned line) {
    const auto pool=c.toc(-0x47b4);
    report_assertion(c,{setup+0x14,pool,pool+0x12,pool+0x1f,frame.at(name),frame.at(extension),line},
                     [&](unsigned target,unsigned back) { c.call(target,back); });
}
inline void copy_date(Cpu& c,unsigned to,unsigned from) {
    // The original copies the year before the signed day and weekday.
    const auto year=c.u16(from+2); const auto day=c.i16(from);
    c.put16(to+2,year); c.put16(to,day); c.put8(to+4,c.u8(from+4));
}
inline std::int32_t signed_divide(std::uint32_t a,std::int32_t b) {
    return std::int32_t(std::int64_t(std::int32_t(a))/b);
}
}
