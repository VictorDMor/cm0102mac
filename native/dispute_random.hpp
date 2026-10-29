#pragma once
#include "dispute_detail.hpp"

namespace cm::aot::dispute::detail {
struct SamplePool {
    Cpu& c;
    std::uint32_t bias_address,base,end_address,cursor_address;
    void advance(std::uint32_t back) const {
        const auto next=c.u32(cursor_address)+2;
        c.set(3,next);
        const auto order=compare_logical(c,next,c.u32(end_address));
        c.put32(cursor_address,next);
        if (order>0) {
            c.put32(cursor_address,base);
            c.call(0x1077b770,back);
            c.put32(bias_address,c.r(3)&0xffff);
        }
    }
    std::uint32_t position() const { return c.u32(cursor_address); }
    std::uint32_t bias() const { return c.u32(bias_address); }
};
inline std::uint32_t remainder(std::uint32_t value,std::uint32_t bound) {
    const auto quotient=bound?std::uint32_t(divide_signed<4>(value,bound)):0;
    return value-quotient*bound;
}
inline std::uint32_t oriented(std::uint32_t value,std::int32_t bound) {
    const auto rest=remainder(value,std::uint32_t(bound));
    return bound>0?rest:0u-rest;
}
}
