#include "cup_stage_detail.hpp"

namespace cm::aot::cup_stage {
using namespace detail;

std::uint32_t allocate_for_load(Cpu& c,std::uint32_t stage,std::int16_t entrants,
                               std::int16_t rounds,std::uint32_t match_counts) {
    Frame f(c,0x104bc750,0x100,{.first_gpr=25});Program p{c};
    keep(c,{{25,match_counts},{26,c.r(5)},{27,c.r(4)},{29,c.toc(slot::flag)},
            {30,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    c.put32(f.at(0x118),stage);
    p.call(0x10478f20,0x104bc784,{stage});
    c.put32(stage,c.toc(slot::vtable));c.set(4,stage);c.set(5,c.toc(slot::vtable));
    auto allocation=p.call(0x10775130,0x104bc79c,{std::uint32_t(std::int32_t(entrants))*8});
    c.set(4,stage);compare_logical(c,allocation);c.put32(stage+0xa4,allocation);
    if(!allocation){assertion(c,f,0x104bc7c0,0xb8,0x50,0x7e);return result(c,stage);}
    keep(c,{{26,extended(rounds)}});
    allocation=p.call(0x10775130,0x104bc810,{std::uint32_t(std::int32_t(rounds))*0x78});
    c.set(4,stage);compare_logical(c,allocation);c.put32(stage+0xa0,allocation);
    if(!allocation){assertion(c,f,0x104bc834,0x98,0x48,0x85);return result(c,stage);}
    allocation=p.call(0x10775130,0x104bc880,{std::uint32_t(std::int32_t(rounds))*4});
    c.set(4,stage);compare_logical(c,allocation);c.put32(stage+0xac,allocation);
    if(!allocation){assertion(c,f,0x104bc8a4,0x78,0x40,0x8d);return result(c,stage);}
    keep(c,{{28,match_counts},{25,0},{27,0}});
    for(std::uint32_t round=0;compare(c,round,rounds)<0;++round) {
        allocation=p.call(0x10775130,0x104bc904,{std::uint32_t(std::int32_t(c.i16(match_counts+round*2)))*0x36});
        const auto table=c.u32(stage+0xac);c.set(4,table);c.put32(table+round*4,allocation);c.set(3,c.u32(stage+0xac));
        if(compare_logical(c,c.u32(c.r(3)+round*4))==0) {
            assertion(c,f,0x104bc938,0x58,0x38,0x97);return result(c,stage);
        }
        keep(c,{{28,match_counts+(round+1)*2},{27,(round+1)*4},{25,round+1}});
    }
    return result(c,stage);
}

std::uint32_t construct(Cpu& c,std::uint32_t stage,std::uint32_t parent,
                        std::uint32_t entrant_count,std::uint32_t clubs,
                        std::uint32_t round_count,std::uint32_t definition,
                        std::uint32_t schedule,std::uint32_t year) {
    Frame f(c,0x104bc3c0,0x150,{.first_gpr=13});Program p{c};
    const auto competition_type=c.u8(f.at(0x18b)), extra=c.u8(f.at(0x18f));
    const auto extra_rounds=extended(c.i16(f.at(0x192))); const auto region=c.u8(f.at(0x197));
    const auto custom_draw=c.u32(f.at(0x198)); const auto numbered=c.u8(f.at(0x19f));
    const auto identifier=c.u32(f.at(0x1a0)), seeds=c.u32(f.at(0x1a4));
    keep(c,{{13,seeds},{14,numbered},{15,custom_draw},{16,parent},{17,entrant_count},
        {18,clubs},{19,round_count},{20,definition},{21,schedule},{22,year},
        {23,competition_type},{24,extra},{25,extra_rounds},{26,region},{27,identifier},
        {29,c.toc(slot::flag)},{30,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    c.put32(f.at(0x168),stage);p.call(0x10478f20,0x104bc424,{stage});
    const auto schedule_bytes=std::uint32_t(std::int32_t(std::int16_t(round_count)))*0x78;
    keep(c,{{28,schedule_bytes}});
    c.put32(stage,c.toc(slot::vtable));
    c.put16(stage+0x40,year);c.put32(stage+0x30,0xffffffff);c.put32(stage+0x2c,0xffffffff);
    c.put8(stage+0x43,1);c.put8(stage+0xb0,0);c.put8(stage+0xa8,0);c.put8(stage+0xa9,0);
    c.put16(stage+0x3c,round_count);c.put32(stage+4,definition);
    c.put8(stage+0x44,competition_type);c.put8(stage+0x42,extra);c.put16(stage+0x3a,extra_rounds);
    c.put8(stage+0x4b,region);c.put8(stage+0x46,c.u8(parent+0x46));c.put8(stage+0x47,c.u8(parent+0x47));
    c.put32(stage+0x28,identifier);
    leave(c,{{4,stage},{5,1},{6,0xffffffff}});
    auto allocation=p.call(0x10775130,0x104bc4d4,{schedule_bytes});
    c.set(4,stage);c.put32(stage+0xa0,allocation);c.set(3,stage);c.set(4,c.u32(stage+0xa0));
    if(compare_logical(c,c.r(4))==0) {assertion(c,f,0x104bc500,0xe0,0x58,0x48);return result(c,stage);}
    p.call(0x1077fc28,0x104bc550,{schedule,c.r(4),schedule_bytes});c.set(2,c.u32(f.at(20)));
    c.put16(stage+0x3e,entrant_count);
    allocation=p.call(0x10775200,0x104bc56c,{extended(c.i16(stage+0x3e)),8});
    c.set(4,stage);compare_logical(c,allocation);c.put32(stage+0xa4,allocation);
    if(!allocation){assertion(c,f,0x104bc590,0xc0,0x50,0x53);return result(c,stage);}
    enter(c,0x104bc5ec,{stage,extended(c.i16(stage+0x3e)),clubs,numbered,seeds});
    // Checker parents and excluded child entries use dispatch; normal calls stay typed.
    if(routine_checker && (!routine_check_child() || !routine_checker->wants(0x104bca90)))
        c.call(0x104bca90,0x104bc5ec);
    else initialize_entrants(c,stage,c.i16(stage+0x3e),clubs,std::int8_t(numbered),seeds);
    if(compare(c,c.r(3))==0){assertion(c,f,0x104bc608,0xa0,0x48,0x59);return result(c,stage);}
    if(compare(c,custom_draw)!=0) {
        c.set(3,stage);const auto status=p.virtual_call(stage,0x9c,0x104bc664);
        if(compare(c,status)<0)assertion(c,f,0x104bc684,0x80,0x40,0x62);
    } else {
        const auto status=p.call(0x10479100,0x104bc6d0,{stage});
        if(compare(c,status)<0)assertion(c,f,0x104bc6ec,0x60,0x38,0x6a);
    }
    return result(c,stage);
}
}
