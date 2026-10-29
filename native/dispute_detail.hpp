#pragma once
#include "dispute.hpp"
#include "guest_values.hpp"

namespace cm::aot::dispute::detail {
using namespace guest;
struct Manager {
    Cpu& c;
    std::uint32_t address;
    Field<std::uint32_t> records() const { return {c,address}; }
    Field<std::uint32_t> flags() const { return {c,address+4}; }
};
struct Record {
    Cpu& c;
    std::uint32_t address;
    static constexpr unsigned size=20;
    Field<std::int32_t> person() const { return {c,address}; }
    Field<std::int32_t> club() const { return {c,address+4}; }
    Field<std::uint8_t> temporary() const { return {c,address+16}; }
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
inline std::uint32_t indirect(Cpu& c,int slot) { return c.u32(c.toc(slot)); }
inline void assertion(Cpu& c,const Frame& frame,std::uint32_t at,
                      int name,int extension,unsigned line) {
    const auto pool=c.toc(-0x4004);
    report_assertion(c,{at,pool+12,pool+24,pool+37,frame.at(name),frame.at(extension),line});
}
}
