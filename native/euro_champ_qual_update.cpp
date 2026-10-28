#include "euro_champ_qual_detail.hpp"
namespace cm::aot::euro_champ_qual {
using namespace detail;
std::uint32_t update(Cpu& c,std::uint32_t object,std::uint32_t date,std::uint32_t active) {
    Kept<24> group_index(c,c.r(24)); Kept<25> entrant_offset(c,c.r(25)); Kept<29> ranking(c,c.r(29)); Kept<30> ranking_kind(c,c.r(30)); Kept<31> ranking_configuration(c,c.r(31));

    Frame f(c,0x104fdae0,0x170,{.first_gpr=24}); Program p{c};
    keep(c,{{26,object},{27,date},{28,active},{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(slot::pool)}});
    c.put16(f.at(0xbc),c.u16(date)); c.put16(f.at(0xbe),c.u16(date+2)); c.put8(f.at(0xc0),c.u8(date+4));
    p.call(0x107156b0,0x104fdb3c,{f.at(0xb4),signed_half(c.u16(object+0xb6)),std::uint16_t(c.i16(object+0xb8)+c.i16(object+0x40))});
    const auto first=c.u32(date),second=c.u32(date+4);
    c.put32(f.at(0x94),first); c.put32(f.at(0x7c),first); c.put32(f.at(0x80),second); c.put32(f.at(0x98),second);
    c.set(3,first); c.set(4,c.u16(f.at(0x96))); c.set(5,c.u16(f.at(0xb6)));
    const auto date_cmp=compare_logical(c,c.r(4),c.r(5));
    bool before=date_cmp<0;
    if(date_cmp==0) { c.set(4,signed_half(c.u16(f.at(0xb4)))); before=c.i16(f.at(0xb4))>c.i16(f.at(0x94)); }
    if(compare(c,before?1:0)!=0) return 0;
    c.put32(f.at(0x8c),c.u32(f.at(0x7c))); c.put32(f.at(0x90),c.u32(f.at(0x80)));
    c.set(3,c.u32(f.at(0x80))); c.set(4,0);
    bool equal=false;
    if(compare_logical(c,c.u16(f.at(0x8e)),c.r(5))==0) {
        c.set(3,signed_half(c.u16(f.at(0x8c)))); equal=compare(c,c.r(3),c.i16(f.at(0xb4)))==0;
    }
    c.set(4,equal?1:0);
    bool initialize=compare(c,c.r(4))!=0;
    if(initialize) initialize=compare(c,c.i8(object+0x45))==0;
    if(initialize) initialize=compare(c,std::int8_t(active))!=0;
    if(initialize) {
        enter(c,0x104fdc00,{object}); result(c,select_entrants(c,object));
        enter(c,0x104fdc08,{object}); draw_groups(c,object);
        enter(c,0x104fdc14,{object,~0u}); c.put16(object+0x3e,result(c,group_size(c,object,~0u)));
        c.put32(object+0xb0,p.call(0x10775130,0x104fdc24,{std::uint32_t(c.i16(object+0x3e))*0x44}));
        bool root_ready=compare_logical(c,c.r(3))!=0;
        if(!root_ready) assertion(c,f,0x104fdc44,0xc4,0x84,0x2d1);
        else {
            c.set(3,std::uint32_t(c.i16(object+0x3e))-1); c.put16(object+0xa4,c.r(3)*std::uint32_t(c.i16(object+0x3c)));
            group_index=0; entrant_offset=0;
            for(;;) {
                c.set(3,signed_byte(std::uint32_t(group_index))); if(compare(c,std::int8_t(std::uint32_t(group_index)),c.i16(object+0x3e))>=0) break;
                p.call(0x104a60e0,0x104fdcc0,{object,std::uint32_t(group_index),c.u32(c.u32(object+0xf8)+std::uint32_t(entrant_offset)),0,0});
                entrant_offset=std::uint32_t(entrant_offset)+4; group_index=std::uint32_t(group_index)+1;
            }
            p.call(0x104a0e40,0x104fdce0,{object});
        }
        if(compare(c,root_ready?1:0)==0) { assertion(c,f,0x104fdd00,0x124,0xac,0x5a7); return 0; }
        if(compare(c,p.call(0x104a00a0,0x104fdd4c,{object}))==0) { assertion(c,f,0x104fdd68,0x104,0xa4,0x5ae); return 0; }
        if(compare(c,p.call(0x1049eb30,0x104fddb8,{object,0}))==0) { assertion(c,f,0x104fddd4,0xe4,0x9c,0x5b5); return 0; }
        group_index=0;
        do {
            enter(c,0x104fde34,{object,std::uint32_t(group_index),0,6}); result(c,create_group(c,object,std::uint32_t(group_index),0,6));
            group_index=std::uint32_t(group_index)+1;
        } while(compare(c,std::uint32_t(group_index),8)<0);
        p.call(0x10775170,0x104fde48,{c.u32(object+0xf8)}); c.put32(object+0xf8,0);
        c.put8(f.at(0x78),1); c.put8(f.at(0x79),2); c.put8(f.at(0x7a),5); c.put8(f.at(0x7b),0); c.put8(object+0xc0,0);
        leave(c,{{4,2},{5,1},{6,0}}); ranking=p.call(0x10773520,0x104fde78,{0xec});
        if(compare(c,std::uint32_t(ranking))!=0) {
            entrant_offset=f.at(0x78); ranking_configuration=object+0xc0; ranking_kind=1020;
            c.put32(f.at(0x38),c.u8(object+0xc4)); c.put32(f.at(0x3c),c.u8(object+0xc5));
            group_index=c.u8(object+0xc6); c.put32(f.at(0x40),std::uint32_t(group_index));
            c.put32(f.at(0x44),std::uint32_t(entrant_offset)); c.put32(f.at(0x48),std::uint32_t(ranking_configuration));
            ranking_configuration=signed_half(c.u16(object+0x40)); c.put32(f.at(0x4c),std::uint32_t(ranking_configuration));
            ranking_configuration=c.u8(object+0xfe); c.put32(f.at(0x50),std::uint32_t(ranking_configuration)); c.put32(f.at(0x54),std::uint32_t(ranking_kind));
            for(unsigned off:{0x58,0x60,0x6c}) c.put32(f.at(off),0);
            c.put32(f.at(0x5c),1); c.put32(f.at(0x64),2); c.put32(f.at(0x68),~0u); c.put32(f.at(0x70),2);
            p.call(0x1056dc60,0x104fdf0c,{std::uint32_t(ranking),object,9,0,0,c.u32(object+4),0,0});
        }
        const auto phase=c.i8(object+0xfe); c.set(4,std::uint32_t(phase)*4); c.set(5,c.u32(object+0xc)); c.set(3,object);
        c.put32(c.r(5)+c.r(4),std::uint32_t(ranking)); c.put8(object+0xc0,1); c.put32(object+0x30,std::uint32_t(phase));
        enter(c,0x104fdf3c); refresh_dates(c,object);
        c.set(3,object); p.virtual_call(object,0x64,0x104fdf4c);
        p.call(0x10775170,0x104fdf58,{c.u32(object+0xec)});
        c.set(3,0); c.put32(object+0xec,0); c.put32(object+0xf0,0); c.put8(object+0x45,1);
    } else {
        p.call(0x1048d460,0x104fdf88,{c.u32(c.toc(-0x7a60)),f.at(0xbc),c.u32(c.u32(object+4))});
        if(compare(c,std::int8_t(c.r(3)))!=0) { enter(c,0x104fdf98,{object}); refresh_dates(c,object); }
    }
    return p.call(0x104a1540,0x104fdfa8,{object,date,active});
}
}
