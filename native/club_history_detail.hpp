#pragma once
#include "club_history.hpp"
#include "guest_values.hpp"
namespace cm::aot::club_history::detail {
using namespace guest;
constexpr std::uint32_t missing = 0xffffffff;
namespace slot {
constexpr int pool=-0x59c8, clubs=-0x7b50, competitions=-0x7b5c, competition_count=-0x7b60,
    record_count=-0x7b64, league_exception=-0x7b58, excluded_competitions=-0x7b54,
    current_date=-0x7b80, special_nation=-0x7b68, single_stage=-0x7b6c, base_league=-0x7b70,
    compare_tv=-0x59cc;
}
struct History {
    Cpu& c; std::uint32_t address;
    Field<std::uint32_t> league_file() const { return {c,address}; }
    Field<std::uint32_t> cup_file() const { return {c,address+4}; }
    Field<std::uint32_t> index() const { return {c,address+8}; }
    Field<std::int32_t> count() const { return {c,address+12}; }
    std::uint32_t club_index(std::uint32_t club) const { return index().get()+c.u32(club)*16; }
};
// The archive stores 24-byte league rows and 12-byte cup rows. Padding is
// deliberately left untouched, including the last two bytes of a league row.
inline void copy_standing(Cpu& c,std::uint32_t to,std::uint32_t from) {
    c.put32(to,c.u32(from));
    for(unsigned i=4;i<8;++i)c.put8(to+i,c.u8(from+i));
    for(unsigned i=8;i<14;i+=2)c.put16(to+i,c.u16(from+i));
}
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target,std::uint32_t back,std::initializer_list<std::uint32_t> args={}) const {
        unsigned n=3;for(auto value:args)c.set(n++,value);
        c.call(target,back);return c.r(3);
    }
    std::uint32_t virtual_call(std::uint32_t object,unsigned offset,std::uint32_t back) const {
        c.set(12,c.u32(c.u32(object)+offset));
        c.call(0x10781320,back);c.set(2,c.u32(c.sp()+0x14));return c.r(3);
    }
};
inline void assertion(Cpu& c,const Frame& f,std::uint32_t at,unsigned name,unsigned ext,unsigned line,bool reload_flag=false) {
    const auto pool=c.toc(slot::pool);
    report_assertion(c,{at,pool+0x58,pool+0x69,pool+0x76,f.at(name),f.at(ext),line});
    if(reload_flag)c.set(4,c.toc(-0x7bac));
}
inline bool byte_nonzero(Cpu& c,std::uint32_t value) { return compare(c,std::int8_t(value))!=0; }
}
