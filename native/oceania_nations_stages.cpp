#include "oceania_nations_detail.hpp"
namespace cm::aot::oceania_nations {
using namespace detail;
std::uint32_t create_group(Cpu& c, std::uint32_t object, std::uint32_t group) {
    Frame f(c,0x106eafa0,0x100,{.first_gpr=21}); Program p{c};
    c.set({{25,object},{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{26,group},{31,c.toc(-0x4094)}});
    const auto entrants=p.call(0x10775130,0x106eafcc,{12}); c.set(28,entrants); c.compare(0,std::int32_t(entrants),0);
    if (!entrants) { assertion(c,f,0x106eafe8,0xac,0x84,0x331); return 0; }
    const auto source=c.u32(object+0xec);
    c.set({{3,object},{4,signed_byte(group)},{5,f.at(0x7a)},{6,f.at(0x78)},{7,0}});
    for (unsigned i=0;i<3;++i) { c.put32(entrants+i*4,c.u32(source+12+i*4)); c.set(8,source); }
    p.virtual_call(object,0x44,0x106eb070); const auto schedule=c.r(3); c.set(27,schedule);
    const auto stage=p.call(0x10773520,0x106eb084,{0xec}); c.set(24,stage); c.compare(0,std::int32_t(stage),0);
    if (stage) {
        c.set({{22,object+0xc7},{23,object+0xc0},{12,signed_byte(group)},{10,4},{11,0},{8,~0u},{4,object},{6,entrants},{9,schedule},{5,3},{7,1}});
        c.put32(f.at(0x38),c.u8(object+0xc4)); c.put32(f.at(0x3c),c.u8(object+0xc5));
        c.set(21,c.u8(object+0xc6)); c.put32(f.at(0x40),c.r(21)); c.put32(f.at(0x44),c.r(22)); c.put32(f.at(0x48),c.r(23));
        c.set(23,signed_half(c.u16(object+0x40))); c.put32(f.at(0x4c),c.r(23)); c.put32(f.at(0x50),signed_byte(group));
        c.put32(f.at(0x54),signed_half(c.u16(f.at(0x78)))); c.put32(f.at(0x58),c.u8(object+0x4b));
        c.put32(f.at(0x5c),4); c.put32(f.at(0x60),0); c.put32(f.at(0x64),signed_half(c.u16(object+0xdc)));
        c.put32(f.at(0x68),~0u); c.put32(f.at(0x6c),0); c.put32(f.at(0x70),2);
        c.set({{8,c.u32(object+4)},{10,signed_half(c.u16(f.at(0x7a)))}}); p.call(0x1056dc60,0x106eb120);
    }
    const auto offset=group*4; c.set({{4,c.u32(object+0xc)},{21,offset},{3,entrants}}); c.put32(c.r(4)+offset,stage);
    p.call(0x10775170,0x106eb134); p.call(0x10775170,0x106eb13c,{schedule});
    c.set(3,c.u32(object+0xc)); const auto result=c.u32(c.r(3)+offset); c.compare_logical(0,result,0);
    if (!result) { assertion(c,f,0x106eb160,0x8c,0x7c,0x347); return 0; }
    c.put32(object+0x30,group); p.call(0x104a0e40,0x106eb1b4,{result}); return 1;
}
void create_finals(Cpu& c, std::uint32_t object) {
    Frame f(c,0x106eb1d0,0xd0,{.first_gpr=25}); Program p{c};
    c.set({{27,object},{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(-0x4094)}});
    const auto entrants=p.call(0x10775130,0x106eb1f8,{16}); c.set(28,entrants); c.compare(0,std::int32_t(entrants),0);
    if (!entrants) { assertion(c,f,0x106eb214,0x8c,0x64,0x366); return; }
    p.call(0x1049ea00,0x106eb260,{object,0}); c.set({{3,c.u32(c.r(3))},{4,1}}); c.put32(entrants,c.r(3));
    p.call(0x1049ea00,0x106eb280,{c.u32(c.u32(object+0xc))}); c.set({{4,c.u32(c.r(3))},{3,object}}); c.put32(entrants+4,c.r(4));
    p.call(0x1049ea00,0x106eb29c,{object,1}); c.set({{3,c.u32(c.r(3))},{4,0}}); c.put32(entrants+8,c.r(3));
    p.call(0x1049ea00,0x106eb2bc,{c.u32(c.u32(object+0xc))}); c.set(4,c.u32(c.r(3))); c.put32(entrants+12,c.r(4));
    c.set({{3,object},{5,f.at(0x5a)},{6,f.at(0x58)},{4,1},{7,0}}); p.virtual_call(object,0x44,0x106eb2ec);
    const auto schedule=c.r(3); c.set(26,schedule); const auto stage=p.call(0x10773520,0x106eb300,{0xb4}); c.set(25,stage); c.compare(0,std::int32_t(stage),0);
    if (stage) {
        c.put32(f.at(0x38),1); c.put32(f.at(0x3c),4); c.put32(f.at(0x40),signed_half(c.u16(f.at(0x58)))); c.put32(f.at(0x44),20);
        for (unsigned off=0x48;off<=0x54;off+=4) c.put32(f.at(off),0);
        c.set({{4,object},{6,entrants},{9,schedule},{5,4},{7,signed_half(c.u16(f.at(0x5a)))},{8,c.u32(object+4)},{10,signed_half(c.u16(object+0x40))}});
        p.call(0x104bc3c0,0x106eb35c);
    }
    c.set({{4,c.u32(object+0xc)},{3,entrants}}); c.put32(c.r(4)+4,stage); p.call(0x10775170,0x106eb36c);
    p.call(0x10775170,0x106eb374,{schedule}); c.set(3,c.u32(c.u32(object+0xc)+4)); c.compare_logical(0,c.r(3),0);
    if (!c.r(3)) { assertion(c,f,0x106eb398,0x6c,0x5c,0x37d); return; }
    p.call(0x1047ef50,0x106eb3e0,{c.r(3),0}); c.put32(object+0x30,1);
}
std::uint32_t select_entrants(Cpu& c, std::uint32_t object) {
    Frame f(c,0x106eb400,0xf0,{.first_gpr=18}); Program p{c};
    c.set({{23,c.toc(-0x7b90)},{18,object},{24,c.toc(-0x7b8c)},{20,0},{25,c.toc(-0x7b88)},{19,0},{26,c.toc(-0x7b84)},
           {27,c.toc(-0x7b04)},{28,c.toc(-0x7654)},{29,c.toc(-0x7650)},{30,c.toc(-0x7970)},{31,c.toc(-0x7b98)},{22,c.toc(-0x7ba8)},{21,c.toc(-0x4094)}});
    c.put16(object+0xf0,6); const auto entrants=p.call(0x10775130,0x106eb458,{24}); c.put32(object+0xec,entrants); c.compare_logical(0,entrants,0);
    if (!entrants) { assertion(c,f,0x106eb47c,0x40,0x38,0x39e); c.set(4,c.toc(-0x7bac)); c.put8(c.r(4),0); return 0; }
    c.set({{21,0},{22,0}}); unsigned first_count=0, second_count=0;
    // National records are 0x128 bytes. Keep the original ten-slot caps and
    // uninitialized spare slots: the original later selects two from each pool.
    for (std::uint32_t index=0;;++index) {
        const auto nation=std::int16_t(index); c.set(6,std::uint32_t(std::int32_t(nation)));
        const auto total=c.i32(c.u32(c.r(2)-0x7b04)); c.compare(0,nation,total); if (nation>=total) break;
        const auto record=c.u32(c.r(31))+index*0x128; c.set({{3,record},{5,c.u32(record+0x74)}}); c.compare_logical(0,c.r(5),0);
        if (c.r(5)) {
            c.set(4,c.u32(c.r(30))); c.compare(0,std::int32_t(c.r(4)),c.i32(c.r(5)+8));
            if (c.r(4)==c.u32(c.r(5)+8)) {
                c.compare(0,nation,c.i32(c.r(29)));
                if (nation!=c.i32(c.r(29))) {
                    c.compare(0,nation,c.i32(c.r(28)));
                    if (nation!=c.i32(c.r(28))) {
                        p.call(0x100caa10,0x106eb514,{record}); c.compare(0,std::int8_t(c.r(3)),0);
                        bool first=false;
                        if (std::int8_t(c.r(3))) { c.compare(0,std::int8_t(first_count),10); first=std::int8_t(first_count)<10; }
                        if (first) {
                            p.call(0x100c76c0,0x106eb534,{c.u32(c.r(31))+index*0x128}); c.set(4,f.at(0x88));
                            c.put32(f.at(0x88)+first_count*4,c.r(3)); c.set(20,++first_count);
                        } else {
                            p.call(0x100caa90,0x106eb558,{c.u32(c.r(31))+index*0x128}); c.compare(0,std::int8_t(c.r(3)),0);
                            if (std::int8_t(c.r(3))) {
                                c.compare(0,std::int8_t(second_count),10);
                                if (std::int8_t(second_count)<10) {
                                    p.call(0x100c76c0,0x106eb578,{c.u32(c.r(31))+index*0x128}); c.set(4,f.at(0x60));
                                    c.put32(f.at(0x60)+second_count*4,c.r(3)); c.set(19,++second_count);
                                }
                            }
                        }
                    }
                }
            }
        }
        c.set({{22,(index+1)*0x128},{21,index+1}});
    }
    const auto odd_draw=[&](unsigned back) {
        const auto cursor_slot=c.r(26),end_slot=c.r(25), seed_slot=c.r(23);
        auto cursor=c.u32(cursor_slot)+2; c.put32(cursor_slot,cursor); c.set(3,cursor); c.compare_logical(0,cursor,c.u32(end_slot));
        if (cursor>c.u32(end_slot)) { c.put32(cursor_slot,c.r(24)); p.call(0x1077b770,back); c.put32(seed_slot,c.r(3)&0xffff); }
        cursor=c.u32(cursor_slot); const auto value=c.u16(cursor); c.set(3,value);
        const auto sum=value+c.u32(seed_slot); const auto rem=std::int32_t(sum)%2; c.compare(0,rem,0); return rem!=0;
    };
    const auto first_id=c.i32(c.r(29)), second_id=c.i32(c.r(28));
    const bool first_odd=odd_draw(0x106eb5c4);
    p.call(0x100c76c0,first_odd?0x106eb604:0x106eb640,{c.u32(c.r(31))+std::uint32_t(first_id)*0x128}); c.set(4,c.u32(object+0xec)); c.put32(c.r(4)+(first_odd?0:12),c.r(3));
    p.call(0x100c76c0,first_odd?0x106eb620:0x106eb65c,{c.u32(c.r(31))+std::uint32_t(second_id)*0x128}); c.set(4,c.u32(object+0xec)); c.put32(c.r(4)+(first_odd?12:0),c.r(3));
    p.call(0x104635b0,0x106eb674,{f.at(0x88),signed_byte(first_count),2}); p.call(0x104635b0,0x106eb684,{f.at(0x60),signed_byte(second_count),2});
    const auto distribute=[&](bool odd,unsigned source,unsigned a,unsigned b) {
        c.set(3,c.u32(object+0xec)); c.put32(c.r(3)+(odd?a:b),c.u32(f.at(source)));
        c.set(3,c.u32(object+0xec)); c.put32(c.r(3)+(odd?b:a),c.u32(f.at(source+4)));
    };
    distribute(odd_draw(0x106eb6a4),0x88,4,16); distribute(odd_draw(0x106eb724),0x60,8,20); return 1;
}
}
