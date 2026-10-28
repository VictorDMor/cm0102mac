#pragma once
#include "club_records.hpp"
#include "club_records_adapter.hpp"
#include "guest_values.hpp"

namespace cm::aot::club_records::detail {
using namespace guest;
constexpr std::uint32_t record_size=0x3fc, index_size=0x6c, missing=0xffffffff;
namespace slot {
constexpr int pool=-0x599c, floats=-0x59a4, doubles=-0x59a0,
    message=-0x7ba8, success=-0x7bac, clubs=-0x7be8, staff_count=-0x7be4,
    news=-0x7bf0, stats=-0x7b3c;
}
struct Manager {
    Cpu& c;std::uint32_t address;
    Field<std::uint32_t> history_file() const {return {c,address};}
    Field<std::uint32_t> modern() const {return {c,address+4};}
    Field<std::uint32_t> modern_capacity() const {return {c,address+8};}
    Field<std::uint32_t> modern_count() const {return {c,address+12};}
    Field<std::uint32_t> season() const {return {c,address+16};}
    Field<std::uint32_t> season_capacity() const {return {c,address+20};}
    Field<std::uint32_t> season_count() const {return {c,address+24};}
    Field<std::uint32_t> indexes() const {return {c,address+28};}
    Field<std::uint32_t> pending() const {return {c,address+36};}
    Field<std::uint32_t> pending_capacity() const {return {c,address+40};}
    Field<std::uint32_t> pending_count() const {return {c,address+44};}
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target,std::uint32_t back,std::initializer_list<std::uint32_t> args={}) const {
        unsigned reg=3;for(auto value:args)c.set(reg++,value);
        // Owned callees enter their typed C++ hook directly. The decline is
        // only for explicit original-mode or single-entry differential checks.
        if(club_records_lists(target)) {
            c.link(back);
            if(try_modern_club_records(c.s,target))return c.r(3);
        }
        c.call(target,back);return c.r(3);
    }
};
inline void assertion(Cpu& c,const Frame& frame,std::uint32_t site,unsigned name,unsigned extension,
                      unsigned line,bool reload_flag=false) {
    const auto pool=c.toc(slot::pool);
    report_assertion(c,{site,pool+0x59,pool+0x6a,pool+0x77,frame.at(name),frame.at(extension),line});
    if(reload_flag)c.set(4,c.toc(slot::success));
}
inline bool nonzero_byte(Cpu& c,std::uint32_t value) {return compare(c,std::int8_t(value))!=0;}
// Original archive/export rows assign each subrecord through its typed copy
// operator. These boundaries leave the last source/destination and LR intact.
inline void copy_archive_record(Cpu& c,std::uint32_t destination,std::uint32_t source,
                                std::uint32_t first_back) {
    constexpr std::pair<unsigned,std::uint32_t> fields[]{
        {0,0x10012d10},{0xc,0x10012cf0},{0x18,0x10012c80},{0x40,0x10012c10},
        {0x68,0x10012ba0},{0x90,0x10012b80},{0x98,0x10012b10},{0xbc,0x10012aa0},
        {0xe0,0x10012a30},{0x104,0x100129c0},{0x128,0x10012950},{0x14c,0x10012910},
        {0x160,0x100128d0},{0x174,0x10012890},{0x188,0x10012850},{0x19c,0x10012800},
        {0x1bc,0x100127b0},{0x1dc,0x10012700},{0x21c,0x100126b0},{0x23c,0x10012650},
        {0x25c,0x10012600},{0x27c,0x100125a0},{0x29c,0x10012550},{0x2b8,0x10012500},
        {0x2d4,0x10012440},{0x318,0x10012380},{0x35c,0x10012330},{0x378,0x100122c0},
        {0x3a4,0x10012250},{0x3d0,0x10012230},{0x3d8,0x10012210},{0x3e0,0x100121c0}};
    Program p{c};auto back=first_back;
    for(const auto& [offset,entry]:fields){p.call(entry,back,{destination+offset,source+offset});back+=12;}
}
}
