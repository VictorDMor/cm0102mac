#pragma once
#include "award_shortlist.hpp"
#include "guest_values.hpp"

namespace cm::aot::award_shortlist::detail {
using namespace guest;
struct Shortlist {
    Cpu& c;
    std::uint32_t address;
    static constexpr unsigned candidate_size=16, voter_count=16;
    Field<std::uint32_t> candidates() const { return {c,address}; }
    Field<std::int16_t> count() const { return {c,address+4}; }
    Field<std::int16_t> capacity() const { return {c,address+6}; }
    Field<std::int8_t> electorate() const { return {c,address+0x28}; }
    std::uint32_t candidate(std::int32_t index) const { return candidates()+std::uint32_t(index)*candidate_size; }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target,std::uint32_t back,
                       std::initializer_list<std::uint32_t> arguments={}) const {
        unsigned reg=3;
        for (auto argument:arguments) c.set(reg++,argument);
        c.call(target,back);
        return c.r(3);
    }
};
inline void assertion(Cpu& c,const Frame& frame,std::uint32_t at,
                      int name,int extension,unsigned line) {
    const auto pool=c.toc(-0x4a64);
    report_assertion(c,{at,pool,pool+0x14,pool+0x21,frame.at(name),frame.at(extension),line});
}
}
