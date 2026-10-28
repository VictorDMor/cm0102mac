#include "euro_champ_qual_detail.hpp"
#include <array>
namespace cm::aot::euro_champ_qual {
using namespace detail;
namespace {
// These two assert expansions reload TOC strings, unlike the common macro
// layout: sprintf returns at +0x28 and the alert at +0x3c.
void bootstrap_assertion(Cpu& c,const Frame& frame,std::uint32_t at,unsigned name,unsigned extension,unsigned line) {
    Program p{c}; const auto pool=c.toc(slot::pool),buffer=c.toc(-0x7ba8);
    p.call(0x10739890,at+4,{pool,0,0,frame.at(name),frame.at(extension)});
    p.call(0x10768810,at+8);
    p.call(0x1077b720,at+0x28,{buffer,pool+0x14,c.r(3),frame.at(name),frame.at(extension),line});
    p.call(0x10769b60,at+0x3c,{pool+0x21,buffer,0});
    c.put8(c.toc(-0x7bac),0);
}
constexpr std::array<std::array<int,6>,9> historical_groups{{
    {{-0x7a14,-0x750c,-0x74f4,-0x757c,-0x6508,0}},
    {{-0x7504,-0x66a8,-0x6600,-0x7598,-0x65a4,-0x7584}},
    {{-0x7af4,-0x75b0,-0x7594,-0x7578,-0x663c,0}},
    {{-0x7658,-0x758c,-0x7a00,-0x75c4,-0x64ec,-0x64e0}},
    {{-0x7514,-0x75a8,-0x7a10,-0x6528,-0x6618,0}},
    {{-0x7a18,-0x759c,-0x74f0,-0x6564,-0x6690,0}},
    {{-0x79fc,-0x75ac,-0x75c0,-0x75a0,-0x64f4,-0x75c8}},
    {{-0x74ec,-0x7574,-0x6598,-0x7590,-0x6630,0}},
    {{-0x74e0,-0x7a1c,-0x651c,-0x6614,-0x658c,-0x6594}}
}};
struct HistoricalFixture { int stage; unsigned day,month,year; int home; unsigned home_score; int away; unsigned away_score; std::uint32_t back; };
#include "euro_champ_qual_fixture_data.inc"
struct BootstrapSpill { unsigned offset; int slot; };
constexpr BootstrapSpill bootstrap_spills[]{
    BootstrapSpill{0x238,-0x7578}, {0x1bc,-0x7a00}, {0x1c0,-0x75c4}, {0x1c4,-0x64ec}, {0x1c8,-0x64e0},
    {0x1cc,-0x7514}, {0x1d0,-0x75a8}, {0x1d4,-0x7a10}, {0x1d8,-0x6528}, {0x1dc,-0x6618},
    {0x1e0,-0x7a18}, {0x1e4,-0x759c}, {0x1e8,-0x74f0}, {0x1ec,-0x6564}, {0x1f0,-0x6690},
    {0x1f4,-0x79fc}, {0x1f8,-0x75ac}, {0x1fc,-0x75c0}, {0x200,-0x75a0}, {0x204,-0x64f4},
    {0x208,-0x75c8}, {0x20c,-0x74ec}, {0x210,-0x7574}, {0x214,-0x6598}, {0x218,-0x7590},
    {0x21c,-0x6630}, {0x220,-0x74e0}, {0x224,-0x7a1c}, {0x228,-0x651c}, {0x22c,-0x6614},
    {0x230,-0x658c}, {0x234,-0x6594}
};
}
void advance_stage(Cpu& c,std::uint32_t object) {
    Kept<16> stage_storage(c,c.r(16)); Kept<17> group_index(c,c.r(17)); Kept<18> entrant_count(c,c.r(18)); Kept<19> seed_position(c,c.r(19)); Kept<20> group_offset(c,c.r(20)); Kept<21> seed_index(c,c.r(21));

    Frame f(c,0x104ff090,0x290,{.first_gpr=13}); Program p{c};
    const auto nation_id=[&](int slot) { return c.u32(c.toc(slot)); };
    keep(c,{{13,c.toc(-0x75c0)},{14,c.toc(-0x79fc)},{15,object},{16,nation_id(-0x74f4)},
        {17,nation_id(-0x750c)},{18,2},{19,nation_id(-0x6594)},{20,nation_id(-0x758c)},
        {21,nation_id(-0x7658)},{22,c.toc(-0x74e0)},{23,c.toc(-0x658c)},{24,c.toc(-0x6614)},
        {25,c.toc(-0x651c)},{26,c.toc(-0x6594)},{27,c.toc(-0x7a1c)},{28,c.toc(-0x64f4)},
        {29,c.toc(-0x75ac)},{30,c.toc(-0x7b98)},{31,nation_id(-0x663c)}});
    for(unsigned group=0;group<historical_groups.size();++group) for(unsigned i=0;i<6;++i) {
        const auto slot=historical_groups[group][i]; c.put32(f.at(0xe4+(group*6+i)*4),slot?nation_id(slot):~0u);
    }
    // The original spills the nation IDs while assembling its table. Later
    // guest calls can read these stack bytes, so keep this ABI image too.
    for(const auto spill:bootstrap_spills) c.put32(f.at(spill.offset),nation_id(spill.slot));
    leave(c,{{4,nation_id(-0x7af4)},{5,nation_id(-0x7584)},{6,nation_id(-0x65a4)},{7,nation_id(-0x7598)},
        {8,nation_id(-0x6600)},{9,nation_id(-0x66a8)},{10,nation_id(-0x7504)}});
    c.set(11,nation_id(-0x6508)); c.set(12,nation_id(-0x757c));
    c.put8(object+0xf4,2); c.put32(object+0xf0,51);
    c.put32(object+0xec,p.call(0x10775130,0x104ff43c,{204}));
    if(compare_logical(c,c.u32(object+0xec))==0) { bootstrap_assertion(c,f,0x104ff460,0xac,0x84,0x7f8); c.set(3,c.toc(-0x7bac)); return; }
    group_offset=0; c.set(31,f.at(0xe4)); c.put32(object+0xf0,0); group_index=0;
    while(compare(c,std::int8_t(std::uint32_t(group_index)),9)<0) {
        entrant_count=0; stage_storage=0; seed_index=0;
        while(compare(c,std::int8_t(std::uint32_t(stage_storage)),6)<0) {
            seed_position=std::uint32_t(group_offset)+std::uint32_t(seed_index); const auto id=c.i32(c.r(31)+std::uint32_t(seed_position));
            if(compare(c,id,-1)!=0) {
                p.call(0x100c76c0,0x104ff4f4,{c.u32(c.r(30))+std::uint32_t(id)*0x128});
                c.set(4,c.u32(c.r(30))); c.set(5,f.at(0xcc)); c.set(6,signed_byte(std::uint32_t(entrant_count))*4);
                c.put32(c.r(5)+c.r(6),c.r(3)); entrant_count=std::uint32_t(entrant_count)+1;
                p.call(0x100c76c0,0x104ff51c,{c.r(4)+std::uint32_t(id)*0x128});
                c.set(5,c.u32(object+0xf0)); c.set(6,c.u32(object+0xec)); c.set(4,c.r(5)+1);
                c.put32(object+0xf0,c.r(4)); c.put32(c.r(6)+c.r(5)*4,c.r(3));
            }
            seed_index=std::uint32_t(seed_index)+4; stage_storage=std::uint32_t(stage_storage)+1;
        }
        if(compare(c,std::int8_t(std::uint32_t(group_index)))==0) {
            c.put16(object+0x3e,signed_byte(std::uint32_t(entrant_count)));
            c.put32(object+0xb0,p.call(0x10775130,0x104ff564,{std::uint32_t(c.i16(object+0x3e))*0x44}));
            if(compare_logical(c,c.r(3))==0) { bootstrap_assertion(c,f,0x104ff584,0x8c,0x7c,0x814); c.set(3,c.toc(-0x7bac)); return; }
            stage_storage=f.at(0xcc); seed_position=0; entrant_count=0;
            for(;;) {
                c.set(3,signed_byte(std::uint32_t(seed_position))); if(compare(c,std::int8_t(std::uint32_t(seed_position)),c.i16(object+0x3e))>=0) break;
                p.call(0x104a60e0,0x104ff5f8,{object,std::uint32_t(seed_position),c.u32(std::uint32_t(stage_storage)),0,0});
                entrant_count=std::uint32_t(entrant_count)+4; stage_storage=std::uint32_t(stage_storage)+4; seed_position=std::uint32_t(seed_position)+1;
            }
        } else {
            enter(c,0x104ff630,{object,signed_byte(std::uint32_t(group_index)-1),f.at(0xcc),signed_byte(std::uint32_t(entrant_count))});
            result(c,create_group(c,object,std::uint32_t(group_index)-1,f.at(0xcc),signed_byte(std::uint32_t(entrant_count))));
        }
        group_offset=std::uint32_t(group_offset)+0x18; group_index=std::uint32_t(group_index)+1;
    }
    c.put8(f.at(0x78),1); c.put8(f.at(0x79),2); c.put8(f.at(0x7a),5); c.put8(f.at(0x7b),0); c.put8(object+0xc0,0);
    leave(c,{{4,5},{5,2}}); stage_storage=p.call(0x10773520,0x104ff670,{0xec});
    if(compare(c,std::uint32_t(stage_storage))!=0) {
        seed_position=f.at(0x78); entrant_count=object+0xc0; group_index=1020;
        c.put32(f.at(0x38),c.u8(object+0xc4)); c.put32(f.at(0x3c),c.u8(object+0xc5));
        group_offset=c.u8(object+0xc6); c.put32(f.at(0x40),std::uint32_t(group_offset)); c.put32(f.at(0x44),std::uint32_t(seed_position)); c.put32(f.at(0x48),std::uint32_t(entrant_count));
        entrant_count=signed_half(c.u16(object+0x40)); c.put32(f.at(0x4c),std::uint32_t(entrant_count));
        entrant_count=c.u8(object+0xfe); c.put32(f.at(0x50),std::uint32_t(entrant_count)); c.put32(f.at(0x54),std::uint32_t(group_index));
        for(unsigned off:{0x58,0x60,0x6c}) c.put32(f.at(off),0);
        c.put32(f.at(0x5c),1); c.put32(f.at(0x64),2); c.put32(f.at(0x68),~0u); c.put32(f.at(0x70),2);
        p.call(0x1056dc60,0x104ff704,{std::uint32_t(stage_storage),object,9,0,0,c.u32(object+4),0,0});
    }
    const auto stage=c.i8(object+0xfe); c.set(4,std::uint32_t(stage)*4); c.set(5,c.u32(object+0xc)); c.set(3,object);
    c.put32(c.r(5)+c.r(4),std::uint32_t(stage_storage)); c.put8(object+0xc0,1); c.put32(object+0x30,std::uint32_t(stage));
    enter(c,0x104ff734); refresh_dates(c,object);
    c.set(3,object); p.virtual_call(object,0x64,0x104ff744); p.call(0x10775170,0x104ff750,{c.u32(object+0xec)});
    c.put32(object+0xec,0); c.put8(object+0x45,1);
    for(const auto& fixture:historical_fixtures) {
        const auto home=nation_id(fixture.home),away=nation_id(fixture.away);
        c.put32(f.at(0x38),fixture.away_score);
        enter(c,fixture.back,{object,std::uint32_t(fixture.stage),fixture.day,fixture.month,fixture.year,home,fixture.home_score,away});
        insert_fixture(c,object,std::uint32_t(fixture.stage),fixture.day,fixture.month,fixture.year,home,fixture.home_score,away,fixture.away_score);
    }
}
}
