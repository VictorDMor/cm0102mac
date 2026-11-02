#include "cup_stage_detail.hpp"
#include <array>

namespace cm::aot::cup_stage {
using namespace detail;
namespace {
struct FieldRead {unsigned offset,size,line;};
constexpr std::array fields{
    FieldRead{0x28,4,0xe9},FieldRead{0x4b,1,0xef},FieldRead{0x46,1,0xf5},
    FieldRead{0x47,1,0xfb},FieldRead{0xb0,1,0x101},FieldRead{0x3c,2,0x107},
    FieldRead{0x3e,2,0x10d},FieldRead{0x40,2,0x113},FieldRead{0xa8,1,0x11a},
    FieldRead{0xa9,1,0x121},FieldRead{0x44,1,0x128},FieldRead{0x42,1,0x12f},
    FieldRead{0x3a,2,0x136},FieldRead{0x1c,4,0x13d},FieldRead{0x20,4,0x144}};
// Convert saved database indices back to guest pointers, retaining null sentinels.
void restore_clubs(Cpu& c,const Stage& stage,std::uint32_t database_slot) {
    leave(c,{{3,0},{6,0},{7,0}});
    for(std::uint32_t index=0;compare(c,index,stage.entrant_count())<0;++index) {
        const auto table=stage.entrants(), offset=index*8;
        c.set(5,table);const auto id=c.u32(table+offset);
        if(compare(c,id,-1)!=0) {
            const auto database=c.u32(database_slot);c.set(4,database);
            c.put32(table+offset,database+id*0x248);
        } else c.put32(table+offset,0);
        leave(c,{{6,(index+1)*8},{7,index+1}});
    }
}
}
std::uint32_t load(Cpu& c,std::uint32_t address,std::uint32_t file) {
    Frame f(c,0x104bcb20,0x360,{.first_gpr=24});Stage stage{c,address};Program p{c};
    const auto database_slot=c.toc(slot::clubs);
    keep(c,{{26,address},{27,file},{28,database_slot},{29,c.toc(slot::flag)},
            {30,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    const auto fail=[&](std::uint32_t at,unsigned name,unsigned extension,unsigned line,std::uint32_t back) {
        assertion(c,f,at,name,extension,line);p.call(0x1076f7d0,back,{0xffffffff});return result(c,0);
    };
    for(unsigned i=0;i<fields.size();++i) {
        const auto field=fields[i]; const auto back=0x104bcb5cu+i*0x7c;
        if(compare(c,p.call(0x10775fe0,back,{address+field.offset,field.size,1,file}),1)!=0)
            return fail(back+0x1c,0x314-i*0x20,0xcc-i*8,field.line,back+0x60);
    }
    if(compare(c,p.call(0x10775fe0,0x104bd2a0,{f.at(0x38),4,1,file}),1)!=0)
        return fail(0x104bd2bc,0x134,0x54,0x14b,0x104bd300);
    const auto definition=c.u32(f.at(0x38));
    if(compare(c,definition,-1)!=0) {
        const auto database=c.u32(c.toc(slot::definitions));c.set(3,database);
        c.put32(address+4,database+definition*0x6c);
    } else c.put32(address+4,0);
    const auto read_rounds=p.call(0x10775fe0,0x104bd348,{stage.rounds(),0x78,extended(stage.round_count()),file});
    keep(c,{{25,read_rounds}});
    if(compare(c,read_rounds,stage.round_count())!=0)
        return fail(0x104bd36c,0x114,0x4c,0x15b,0x104bd3b0);
    p.call(0x10775fe0,0x104bd3cc,{stage.entrants(),8,extended(stage.entrant_count()),file});
    // Original checks the preceding round-read result again, ignoring this read's result.
    if(compare(c,read_rounds,stage.round_count())!=0)
        return fail(0x104bd3ec,0xf4,0x44,0x163,0x104bd430);
    restore_clubs(c,stage,database_slot);
    keep(c,{{24,0},{25,0},{28,0}});
    for(std::uint32_t round=0;compare(c,round,stage.round_count())<0;++round) {
        const auto bytes=std::uint32_t(std::int32_t(c.i16(stage.round(round)+0x1c)))*0x36;
        if(compare(c,p.call(0x10775fe0,0x104bd4b8,{c.u32(stage.matches()+round*4),bytes,1,file}),1)!=0)
            return fail(0x104bd4d4,0xd4,0x3c,0x17a,0x104bd518);
        keep(c,{{25,(round+1)*0x78},{28,(round+1)*4},{24,round+1}});
    }
    return result(c,1);
}

void save(Cpu& c,std::uint32_t address,std::uint32_t file) {
    Frame f(c,0x104bd550,0xa0,{.first_gpr=26});Stage stage{c,address};Program p{c};
    const auto database_slot=c.toc(slot::clubs);
    keep(c,{{26,c.toc(slot::flag)},{27,c.toc(slot::message)},{28,c.toc(slot::pool)},
            {29,address},{30,file},{31,database_slot}});
    if(compare_logical(c,stage.rounds())==0 || compare_logical(c,stage.entrants())==0 ||
       compare_logical(c,stage.matches())==0) {assertion(c,f,0x104bd5b0,0x68,0x40,0x19a);return;}
    for(unsigned i=0;i<fields.size();++i)
        p.call(0x10776360,0x104bd604+i*0x14,{address+fields[i].offset,fields[i].size,1,file});
    const auto definition=c.u32(address+4);c.set(3,definition);
    if(compare_logical(c,definition)!=0) p.call(0x10776360,0x104bd738,{definition,4,1,file});
    else {
        assertion(c,f,0x104bd750,0x48,0x38,0x1b9);
        p.call(0x10776360,0x104bd7a0,{c.toc(-0x4678),4,1,file});
    }
    p.call(0x10776360,0x104bd7b4,{stage.rounds(),0x78,extended(stage.round_count()),file});
    leave(c,{{7,0},{6,0}});
    std::uint32_t index=0;
    for(;;) {
        c.set(5,extended(stage.entrant_count()));
        if(compare(c,index,stage.entrant_count())>=0)break;
        const auto table=stage.entrants(),club=c.u32(table+index*8);c.set(4,table);c.set(3,club);
        if(compare_logical(c,club)!=0) {const auto id=c.u32(club);c.set(3,id);c.put32(table+index*8,id);}
        else c.put32(table+index*8,0xffffffff);
        ++index;leave(c,{{6,index*8},{7,index}});
    }
    p.call(0x10776360,0x104bd80c,{stage.entrants(),8,c.r(5),file});
    restore_clubs(c,stage,database_slot);
    keep(c,{{28,0},{26,0},{27,0}});
    for(std::uint32_t round=0;compare(c,round,stage.round_count())<0;++round) {
        const auto bytes=std::uint32_t(std::int32_t(c.i16(stage.round(round)+0x1c)))*0x36;
        p.call(0x10776360,0x104bd88c,{c.u32(stage.matches()+round*4),bytes,1,file});
        keep(c,{{26,(round+1)*0x78},{27,(round+1)*4},{28,round+1}});
    }
}
}
