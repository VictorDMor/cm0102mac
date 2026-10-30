#include "award_shortlist_random.hpp"

namespace cm::aot::award_shortlist {
using namespace detail;
std::uint32_t vote(Cpu& c,std::uint32_t object,std::uint32_t output,std::uint32_t limit) {
    Frame frame(c,0x104386b0,0x90,{.first_gpr=16});Program p{c};Shortlist list{c,object};
    keep(c,{{18,c.toc(-0x7be8)},{19,limit},{20,c.toc(-0x7b04)},
            {21,c.toc(-0x7be0)},{22,c.toc(-0x7b50)},{23,output},
            {25,c.toc(-0x7b88)},{26,c.toc(-0x7b8c)},{27,c.toc(-0x4a70)},
            {28,c.toc(-0x7b90)},{29,c.toc(-0x7b84)},{30,0},{31,object}});
    const auto domestic_count=c.toc(-0x7b04),total_count=c.toc(-0x7b50),team_table=c.toc(-0x7be8);
    c.set(4,std::uint32_t(list.count().get()));c.set(3,c.u32(domestic_count));
    const auto empty=compare(c,std::int16_t(c.r(4)))==0;
    c.set(3,c.r(3)*2);const auto first_team_count=c.u32(total_count)-c.r(3);c.set(24,first_team_count);
    if (empty) return result(c,0);
    p.call(0x10768300,0x10438728,{list.candidates(),c.r(4),16,c.toc(-0x4a6c)});
    c.set(3,c.u8(object+0x28));const auto electorate=std::int8_t(c.r(3));
    const SamplePool pool{c,c.r(28),c.r(26),c.r(25),c.r(29)};
    std::uint32_t prefix_count=0;
    // The prefix count is sign-extended from sixteen bits at every draw.
    // It can never enter the original's |bound| > 65535 floating-point arm.
    const auto cast_vote=[&](unsigned bound_reg,unsigned selected_reg,unsigned wrap_back) {
        const auto prefix=std::int16_t(prefix_count);c.set(bound_reg,std::uint32_t(prefix));
        std::uint32_t selected=0;
        if (compare(c,prefix)!=0) {
            pool.advance(wrap_back);
            c.set(3,0x10000);compare(c,prefix,65535);
            c.set(3,0xffff0000);compare(c,prefix,-65535);
            c.set(3,pool.position());compare(c,prefix);
            c.set(3,c.u16(c.r(3))+pool.bias());selected=oriented(c.r(3),prefix);
        }
        c.set(selected_reg,selected);
        const auto offset=std::uint32_t(std::int32_t(std::int16_t(selected)))*16;
        const auto storage=list.candidates().get();
        leave(c,{{6,storage},{5,offset+12},{4,std::uint32_t(c.i16(storage+offset+12))}});
        ++prefix_count;c.set(30,prefix_count);c.set(3,std::uint32_t(std::int32_t(std::int16_t(prefix_count))));
        c.put16(storage+offset+12,c.r(4)+1);
        if (compare(c,c.r(3),list.count())>=0) { prefix_count=0;c.set(30,0); }
    };
    const auto controlled=[&](std::uint32_t club,unsigned back) {
        p.call(0x1014c2e0,back,{club,0,1,0,0});return compare(c,std::int8_t(c.r(3)))!=0;
    };
    const auto first=compare(c,electorate)==0;
    if (first || electorate==1 || electorate==2) {
        if (!first) { compare(c,electorate,1);if (electorate==2) compare(c,electorate,2); }
        auto club=c.u32(team_table);std::uint32_t team_index=0;
        const unsigned club_reg=first?18:20,index_reg=first?20:17;
        c.set(club_reg,club);c.set(index_reg,team_index);
        while (compare(c,team_index,first_team_count)<0) {
            auto relation=c.u32(club+(first?0x30:0x2c));c.set(3,relation);
            bool eligible=compare_logical(c,relation)!=0;
            if (eligible && electorate==2) {
                relation=c.u32(relation+0x74);c.set(3,relation);eligible=compare_logical(c,relation)!=0;
            }
            if (eligible) {
                const auto identity=c.u32(relation+(first?0:electorate==1?0x68:8));c.set(3,identity);
                eligible=compare(c,identity,c.u32(object+(first?0x2c:electorate==1?0x30:0x34)))==0;
            }
            const auto service_back=first?0x10438778:electorate==1?0x10438970:0x10438b6c;
            if (eligible && !controlled(club,service_back))
                cast_vote(first?22:18,first?17:22,first?0x104387b0:electorate==1?0x104389a8:0x10438ba4);
            ++team_index;club+=0x248;c.set(index_reg,team_index);c.set(club_reg,club);
        }
    } else {
        compare(c,electorate,1);compare(c,electorate,2);
        if (compare(c,electorate,3)==0) {
            auto team_index=first_team_count,club=c.u32(team_table)+team_index*0x248;
            c.set(18,club);
            while (true) {
                c.set(3,c.u32(domestic_count));
                const auto end=c.u32(total_count)-c.r(3);
                if (compare(c,team_index,end)>=0) break;
                auto region=c.u32(club+0x2c);c.set(3,region);
                bool eligible=compare_logical(c,region)!=0;
                if (eligible) { region=c.u32(region+0x74);c.set(3,region);eligible=compare_logical(c,region)!=0; }
                if (eligible) { c.set(3,c.u32(region+8));eligible=compare(c,c.r(3),c.u32(object+0x34))==0; }
                if (eligible && !controlled(club,0x10438d6c)) cast_vote(17,16,0x10438da4);
                ++team_index;club+=0x248;c.set(24,team_index);c.set(18,club);
            }
        } else {
            const auto all_first=compare(c,electorate,4)==0;
            bool all_second=false,all=false;
            if (!all_first) { all_second=compare(c,electorate,5)==0;if (!all_second) all=compare(c,electorate,6)==0; }
            if (all_first || all_second || all) {
                auto team_index=all_second?first_team_count:0;c.set(18,team_index);
                while (true) {
                    auto end=first_team_count;
                    if (all_second) { c.set(3,c.u32(domestic_count));end=c.u32(total_count)-c.r(3); }
                    else if (all) end=c.u32(total_count);
                    if (compare(c,team_index,end)>=0) break;
                    cast_vote(17,16,all_first?0x10438f64:all_second?0x10439110:0x104392cc);
                    ++team_index;c.set(18,team_index);
                }
            }
        }
    }
    p.call(0x10768300,0x1043944c,{list.candidates(),std::uint32_t(list.count().get()),16,c.toc(-0x4a74)});
    leave(c,{{8,0},{4,std::uint32_t(std::int32_t(std::int16_t(limit)))},{7,0},{3,0}});
    std::uint32_t written=0,offset=0;
    while (true) {
        if (compare(c,written,std::int16_t(limit))>=0 || compare(c,written,list.count())>=0) break;
        const auto id=c.u32(list.candidates().get()+offset);
        c.set(6,c.u32(c.r(21)));c.set(5,c.r(6)+id*0x70);
        c.put32(output+std::uint32_t(std::int32_t(std::int16_t(written)))*4,c.r(5));
        offset+=16;++written;c.set(7,offset);c.set(8,written);c.set(3,written);
    }
    return written;
}
}
