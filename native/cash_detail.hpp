#pragma once
#include "cash.hpp"
#include "guest_values.hpp"
#include "guest_fpu.hpp"
#include <array>

namespace cm::aot::cash::detail {
using namespace guest;
inline std::uint32_t signed_byte(std::uint32_t value) {
    return std::uint32_t(std::int32_t(std::int8_t(value)));
}
inline std::uint64_t pair(std::uint32_t high, std::uint32_t low) {
    return std::uint64_t(high)<<32 | low;
}
inline std::uint64_t returned_pair(const Cpu& c) { return pair(c.r(3),c.r(4)); }
inline std::uint64_t result_pair(Cpu& c,std::uint64_t value) {
    c.set(3,std::uint32_t(value>>32));c.set(4,std::uint32_t(value));return value;
}
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target,std::uint32_t back,
                       std::initializer_list<std::uint32_t> args={}) const {
        unsigned n=3;for(auto value:args)c.set(n++,value);
        c.call(target,back);return c.r(3);
    }
};
// CASH caches its diagnostic buffer and, in label(), the success flag before
// calling services. The generic assertion helper deliberately reloads them.
inline void assertion(Cpu& c,const Frame& frame,std::uint32_t first_call,
                       std::uint32_t pool,std::uint32_t buffer,
                       unsigned name,unsigned extension,unsigned line) {
    Program p{c};
    p.call(0x10739890,first_call+4,{pool,0,0,frame.at(name),frame.at(extension)});
    p.call(0x10768810,first_call+8);
    p.call(0x1077b720,first_call+0x24,{buffer,pool+9,c.r(3),frame.at(name),frame.at(extension),line});
    p.call(0x10769b60,first_call+0x34,{pool+0x16,buffer,0});
}
}
