#include "euro_champ_qual_detail.hpp"
#include <array>
namespace cm::aot::euro_champ_qual {
using namespace detail;
void create_playoffs(Cpu& c,std::uint32_t object) {
    Kept<15> stage(c,c.r(15)); Kept<16> selection_cursor(c,c.r(16)); Kept<17> selection_count(c,c.r(17)); Kept<18> entrant_index(c,c.r(18)); Kept<19> standing_offset(c,c.r(19)); Kept<28> playoff_count(c,c.r(28)); Kept<29> entrant_list(c,c.r(29)); Kept<30> qualified(c,c.r(30)); Kept<31> entrant_number(c,c.r(31));

    Frame f(c,0x104fcbc0,0x100,{.first_gpr=15}); Program p{c};
    keep(c,{{20,c.toc(-0x7b80)},{21,c.toc(-0x7b98)},{22,c.toc(-0x7a64)},{23,c.toc(-0x7b48)},
        {24,c.toc(-0x7bac)},{25,c.toc(-0x7ba8)},{26,c.toc(slot::pool)},{27,object}});
    if(compare(c,c.i8(object+0xfe),-1)==0) {
        Kept<18> group(c,~0u); Kept<17> offset(c,~3u);
        while(compare(c,group,c.i8(object+0xfd))<=0) {
            stage=compare(c,group,-1)==0?object:c.u32(c.u32(object+0xc)+std::uint32_t(offset));
            Kept<19> index(c,0); Kept<16> record_offset(c,0);
            while(compare(c,index,c.i16(std::uint32_t(stage)+0x3e))<0) {
                const auto record=c.u32(std::uint32_t(stage)+0xb0)+std::uint32_t(record_offset); c.set(4,record);
                if(compare(c,c.u8(record+0x40),7)!=0) {
                    p.call(0x10384820,0x104fcc5c,{c.u32(c.r(23)),c.u32(record),c.u32(object+4),0x41c,100});
                    p.call(0x102b71e0,0x104fcc70,{c.u32(c.r(22)),c.u32(c.u32(c.u32(std::uint32_t(stage)+0xb0)+std::uint32_t(record_offset))+0x2c)});
                }
                record_offset=std::uint32_t(record_offset)+0x44; index=std::uint32_t(index)+1;
            }
            offset=std::uint32_t(offset)+4; group=std::uint32_t(group)+1;
        }
        return;
    }
    c.set(3,compare(c,c.u8(object+0xf4),1)==0?3:1); playoff_count=signed_byte(c.r(3));
    selection_cursor=0; Kept<17> group(c,~0u); Kept<29> group_offset(c,~3u);
    for(;;) {
        if(compare(c,group,c.i8(object+0xfd))>0) break;
        stage=compare(c,group,-1)==0?object:c.u32(c.u32(object+0xc)+std::uint32_t(group_offset));
        Kept<18> entrant(c,0);
        while(compare(c,entrant,c.i16(std::uint32_t(stage)+0x3e))<0) {
            Kept<19> rank(c,0);
            while(compare(c,std::int8_t(std::uint32_t(rank)),std::uint32_t(playoff_count))<0) {
                p.call(0x1049ea00,0x104fcd0c,{c.u32(c.u32(object+0xc)+std::uint32_t(c.i8(object+0xfe))*4),signed_byte(rank)});
                qualified=c.u32(c.r(3)); p.call(0x1049ea00,0x104fcd1c,{std::uint32_t(stage),entrant});
                if(compare_logical(c,c.u32(c.r(3)),std::uint32_t(qualified))==0) {
                    rank=7; p.call(0x1049ea00,0x104fcd38,{std::uint32_t(stage),entrant}); c.put8(c.r(3)+0x40,rank);
                    p.call(0x1049ea00,0x104fcd48,{std::uint32_t(stage),entrant});
                    leave(c,{{3,object},{4,c.u32(c.r(3))},{5,7},{6,c.u8(object+0xfe)},{7,0},{8,0}});
                    p.virtual_call(object,0x48,0x104fcd70); c.set(3,2);
                    c.put8(std::uint32_t(stage)+0xc0,2); selection_cursor=std::uint32_t(selection_cursor)+1; c.put8(std::uint32_t(stage)+0xc1,0); break;
                }
                rank=std::uint32_t(rank)+1;
            }
            entrant=std::uint32_t(entrant)+1;
        }
        if(compare(c,std::uint32_t(selection_cursor),std::uint32_t(playoff_count))==0) break;
        group_offset=std::uint32_t(group_offset)+4; group=std::uint32_t(group)+1;
    }
    c.set(3,compare(c,c.u8(object+0xf4),1)==0?6:8);
    selection_count=signed_half(c.r(3)); playoff_count=c.r(3); entrant_list=p.call(0x10775130,0x104fcdf4,{std::uint32_t(selection_count)*4});
    if(compare(c,std::uint32_t(entrant_list))==0) { assertion(c,f,0x104fce10,0x94,0x6c,0x440); return; }
    selection_cursor=std::uint32_t(entrant_list); stage=c.u32(c.u32(object+0xc)+std::uint32_t(c.i8(object+0xfe))*4);
    entrant_index=std::uint32_t(c.i16(std::uint32_t(stage)+0x3e))-std::uint32_t(selection_count);
    while(compare(c,std::uint32_t(entrant_index),c.i16(std::uint32_t(stage)+0x3e))<0) {
        p.call(0x1049ea00,0x104fce80,{std::uint32_t(stage),std::uint32_t(entrant_index)}); entrant_index=std::uint32_t(entrant_index)+1; c.put32(std::uint32_t(selection_cursor),c.u32(c.r(3))); selection_cursor=std::uint32_t(selection_cursor)+4;
    }
    selection_cursor=~0u; entrant_index=~3u;
    while(compare(c,std::uint32_t(selection_cursor),c.i8(object+0xfd))<=0) {
        stage=compare(c,std::uint32_t(selection_cursor),-1)==0?object:c.u32(c.u32(object+0xc)+std::uint32_t(entrant_index));
        entrant_number=0; standing_offset=0;
        while(compare(c,std::uint32_t(entrant_number),c.i16(std::uint32_t(stage)+0x3e))<0) {
            qualified=0; auto selection_entry=std::uint32_t(entrant_list); c.set(3,selection_entry); c.set(6,0);
            if(compare(c,std::uint32_t(selection_count))>0) {
                for(unsigned i=0;i<std::uint32_t(selection_count);++i) {
                    c.set(4,c.u32(std::uint32_t(stage)+0xb0)); c.set(5,c.u32(selection_entry));
                    if(compare_logical(c,c.r(5),c.u32(c.r(4)+std::uint32_t(standing_offset)))==0) { qualified=1; break; }
                    c.set(6,(i+1)*4); selection_entry+=4; c.set(3,selection_entry);
                }
            }
            if(compare(c,std::uint32_t(qualified))!=0) {
                c.put8(c.u32(std::uint32_t(stage)+0xb0)+std::uint32_t(standing_offset)+0x40,1);
                p.call(0x10384500,0x104fcf4c,{c.u32(c.r(23)),c.u32(c.u32(std::uint32_t(stage)+0xb0)+std::uint32_t(standing_offset)),c.u32(object+4),0,160,30});
            }
            if(compare(c,std::uint32_t(qualified))==0) {
                const auto record=c.u32(std::uint32_t(stage)+0xb0)+std::uint32_t(standing_offset); c.set(4,record);
                if(compare(c,c.u8(record+0x40),7)!=0) {
                    p.call(0x10384820,0x104fcf80,{c.u32(c.r(23)),c.u32(record),c.u32(object+4),0x41c,100});
                    p.call(0x102b71e0,0x104fcf94,{c.u32(c.r(22)),c.u32(c.u32(c.u32(std::uint32_t(stage)+0xb0)+std::uint32_t(standing_offset))+0x2c)});
                }
            }
            standing_offset=std::uint32_t(standing_offset)+0x44; entrant_number=std::uint32_t(entrant_number)+1;
        }
        entrant_index=std::uint32_t(entrant_index)+4; selection_cursor=std::uint32_t(selection_cursor)+1;
    }
    entrant_index=0; selection_cursor=0;
    while(compare(c,std::uint32_t(entrant_index),std::int32_t(std::uint32_t(c.i16(std::uint32_t(stage)+0x3e))-std::uint32_t(selection_count)))<0) {
        leave(c,{{3,object},{4,c.u32(c.u32(std::uint32_t(stage)+0xb0)+std::uint32_t(selection_cursor))},{5,~0u},{6,1},{7,1},{8,1}});
        p.virtual_call(object,0x70,0x104fcff4); selection_cursor=std::uint32_t(selection_cursor)+0x44; entrant_index=std::uint32_t(entrant_index)+1;
    }
    leave(c,{{3,object},{4,c.u8(object+0xff)},{5,f.at(0x5a)},{6,f.at(0x58)},{7,0}});
    stage=p.virtual_call(object,0x44,0x104fd030);
    c.set(4,c.toc(-0x7a9c)); c.set(5,c.u16(c.r(20)+2));
    c.put32(f.at(0x5c),c.u32(c.r(4))); c.put32(f.at(0x60),c.u32(c.r(4)+4)); c.set(3,c.u16(f.at(0x5e)));
    const auto date_cmp=compare_logical(c,c.r(3),c.r(5));
    bool historical=date_cmp>0;
    if(date_cmp==0) { c.set(4,signed_half(c.u16(f.at(0x5c)))); historical=c.i16(f.at(0x5c))>c.i16(c.r(20)); }
    compare(c,historical?1:0);
    if(historical) {
        constexpr std::array slots{-0x7a1c,-0x7a10,-0x759c,-0x750c,-0x66a8,-0x7658,-0x7574,-0x75b0};
        for(unsigned i=0;i<slots.size();++i) {
            c.set(4,c.u32(c.r(21))); p.call(0x100c76c0,0x104fd0b0+i*0x1c,{c.r(4)+c.u32(c.toc(slots[i]))*0x128}); c.put32(std::uint32_t(entrant_list)+i*4,c.r(3));
        }
        c.put16(std::uint32_t(stage)+0xe,c.u16(std::uint32_t(stage)+0xe)|0x200);
    }
    selection_cursor=p.call(0x10773520,historical?0x104fd18c:0x104fd208,{0xb4});
    if(compare(c,std::uint32_t(selection_cursor))!=0) {
        c.put32(f.at(0x38),c.u8(object+0xff)); c.put32(f.at(0x3c),4); c.put32(f.at(0x40),0); c.put32(f.at(0x44),20);
        c.put32(f.at(0x48),historical?1:0); c.put32(f.at(0x4c),0); c.put32(f.at(0x50),0); c.put32(f.at(0x54),0);
        p.call(0x104bc3c0,historical?0x104fd1e8:0x104fd260,{std::uint32_t(selection_cursor),object,std::uint32_t(playoff_count),std::uint32_t(entrant_list),signed_half(c.u16(f.at(0x5a))),c.u32(object+4),std::uint32_t(stage),signed_half(c.u16(object+0x40))});
    }
    c.set(3,c.u32(object+0xc)); c.put32(c.r(3)+std::uint32_t(c.i8(object+0xff))*4,std::uint32_t(selection_cursor));
    p.call(0x10775170,0x104fd27c,{std::uint32_t(entrant_list)}); p.call(0x10775170,0x104fd284,{std::uint32_t(stage)});
    c.set(3,c.u32(c.u32(object+0xc)+std::uint32_t(c.i8(object+0xff))*4));
    if(compare_logical(c,c.r(3))==0) { assertion(c,f,0x104fd2b4,0x74,0x64,0x4a2); return; }
    p.call(0x1047ef50,0x104fd2fc,{c.r(3),0}); c.put32(object+0x30,std::uint32_t(c.i8(object+0xff)));
}
}
