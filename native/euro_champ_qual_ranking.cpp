#include "euro_champ_qual_detail.hpp"
namespace cm::aot::euro_champ_qual {
using namespace detail;
namespace {
void copy_date(Cpu& c,std::uint32_t to,std::uint32_t from) {
    c.set(4,c.u16(from+2)); c.set(3,signed_half(c.u16(from)));
    c.put16(to+2,c.r(4)); c.put16(to,c.r(3)); c.put8(to+4,c.u8(from+4));
}
void copy_standing(Cpu& c,std::uint32_t to,std::uint32_t from) {
    for(unsigned offset:{0u,8u,12u,16u,32u,48u}) c.put32(to+offset,c.u32(from+offset));
    for(unsigned offset:{4u,20u,21u,22u,23u,36u,37u,38u,39u,52u,53u,54u,55u,64u,65u,66u,67u}) c.put8(to+offset,c.u8(from+offset));
    for(unsigned offset:{24u,26u,28u,40u,42u,44u,56u,58u,60u}) c.put16(to+offset,c.u16(from+offset));
}
}
void refresh_dates(Cpu& c,std::uint32_t object) {
    Kept<20> entrant_index(c,c.r(20)); Kept<21> candidate_index(c,c.r(21)); Kept<22> ranking(c,c.r(22)); Kept<24> entrant_ids(c,c.r(24)); Kept<25> reference_club(c,c.r(25)); Kept<27> stage(c,c.r(27));

    Frame f(c,0x104fd680,0xe0,{.first_gpr=20}); Program p{c};
    keep(c,{{23,object},{30,c.toc(-0x7ba8)},{31,c.toc(slot::pool)}});
    p.call(0x10715880,0x104fd6a4,{f.at(0x80)}); p.call(0x10715880,0x104fd6ac,{f.at(0x78)});
    c.set(5,signed_half(c.u16(object+0x40)));
    if(compare(c,c.r(5),1998)==0) { p.call(0x107156f0,0x104fd6d0,{f.at(0x68),1,5,1998,~0u}); copy_date(c,f.at(0x80),f.at(0x68)); }
    else {
        const auto calendar=c.u32(object+0xbc); p.call(0x107156b0,0x104fd708,{f.at(0x60),signed_half(c.u16(calendar)),std::uint16_t(c.u16(calendar+2)+c.r(5))});
        copy_date(c,f.at(0x80),f.at(0x60));
    }
    const auto last=c.u32(object+0xbc)+std::uint32_t(c.i16(object+0xa6))*0x4c-0x4c;
    p.call(0x107156b0,0x104fd74c,{f.at(0x50),signed_half(c.u16(last)),std::uint16_t(c.i16(object+0x40)+c.u16(last+2))});
    p.call(0x10717a10,0x104fd75c,{f.at(0x58),c.r(3),1}); copy_date(c,f.at(0x78),f.at(0x58));
    Kept<26> group(c,~0u); Kept<29> group_offset(c,~3u); Kept<28> standing_offset(c,0xffffffbc);
    do {
        if(compare(c,group,-1)<=0) stage=object; else stage=c.u32(c.u32(object+0xc)+std::uint32_t(group_offset));
        p.call(0x1049eb30,0x104fd7a4,{std::uint32_t(stage),0}); p.call(0x1049ea00,0x104fd7b0,{std::uint32_t(stage),1});
        reference_club=c.u32(c.r(3)); entrant_ids=p.call(0x10775130,0x104fd7bc,{0x10});
        if(compare(c,std::uint32_t(entrant_ids))==0) { assertion(c,f,0x104fd7d8,0x88,0x70,0x55a); c.set(3,c.toc(-0x7bac)); return; }
        ranking=std::uint32_t(entrant_ids); entrant_index=0; candidate_index=0;
        do {
            p.call(0x1049ea00,0x104fd838,{std::uint32_t(stage),std::uint32_t(entrant_index)}); c.set(3,c.u32(c.r(3)));
            entrant_index=std::uint32_t(entrant_index)+1; compare(c,std::uint32_t(entrant_index),4); candidate_index=std::uint32_t(candidate_index)+4;
            c.put32(std::uint32_t(ranking),c.u32(c.r(3))); ranking=std::uint32_t(ranking)+4;
        } while(std::uint32_t(entrant_index)<4);
        ranking=p.call(0x10773520,0x104fd860,{0xec});
        if(compare(c,std::uint32_t(ranking))!=0) {
            c.put32(f.at(0x38),c.u8(object+0xc4)); c.put32(f.at(0x3c),c.u8(object+0xc5));
            c.put32(f.at(0x40),object+0xc7);
            const auto count=c.i16(std::uint32_t(stage)+0x3e); c.put32(f.at(0x44),std::uint32_t(count-1)*std::uint32_t(count)*9);
            c.put32(f.at(0x48),c.u32(object+0x10));
            p.call(0x1058db20,0x104fd8c0,{std::uint32_t(ranking),4,std::uint32_t(entrant_ids),~0u,~0u,f.at(0x80),f.at(0x78),c.u32(c.u32(object+4))});
        }
        candidate_index=0;
        do {
            p.call(0x1049ea00,0x104fd8d4,{std::uint32_t(ranking),std::uint32_t(candidate_index)});
            if(compare_logical(c,c.u32(c.r(3)),std::uint32_t(reference_club))==0) {
                const auto destination_group=c.u32(c.u32(object+0xc)+std::uint32_t(c.i8(object+0xfe))*4);
                const auto source=c.u32(std::uint32_t(ranking)+0xb0)+std::uint32_t(candidate_index)*0x44;
                const auto destination=c.u32(destination_group+0xb0)+(std::uint32_t(group)+1)*0x44;
                c.set({{8,c.u32(destination_group+0xb0)},{9,c.u32(std::uint32_t(ranking)+0xb0)},
                    {5,std::uint32_t(standing_offset)+0x50},{6,0}});
                copy_standing(c,destination,source);
                c.set(7,c.u8(source+0x42)); c.set(4,c.u8(source+0x43));
                c.set(3,c.u32(destination_group+0xb0));
                c.put32(c.r(3)+std::uint32_t(standing_offset)+0x50,0); c.put32(c.r(3)+std::uint32_t(standing_offset)+0x4c,0);
                break;
            }
            candidate_index=std::uint32_t(candidate_index)+1;
        } while(compare(c,std::uint32_t(candidate_index),4)<0);
        if(compare_logical(c,std::uint32_t(ranking))!=0) { c.set(3,std::uint32_t(ranking)); c.set(4,1); p.virtual_call(std::uint32_t(ranking),8,0x104fda84); }
        p.call(0x10775170,0x104fda90,{std::uint32_t(entrant_ids)});
        group=std::uint32_t(group)+1; standing_offset=std::uint32_t(standing_offset)+0x44;
        compare(c,group,8); group_offset=std::uint32_t(group_offset)+4;
    } while(std::int32_t(std::uint32_t(group))<8);
    p.call(0x1049eb30,0x104fdac0,{c.u32(c.u32(object+0xc)+std::uint32_t(c.i8(object+0xfe))*4),0});
}
}
