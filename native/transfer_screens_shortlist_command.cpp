#include "transfer_screens_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
std::uint32_t shortlist_confirmation_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x10425240,0xc0,{.first_gpr=19});
    Kept<19> manager_or_bound_or_fee(c,c.r(19));
    Kept<20> transfers(c,c.toc(slot::transfers));
    Kept<21> current_contract_or_record(c,c.toc(slot::current_contract));
    Kept<22> contracts(c,c.toc(slot::contracts));
    Kept<23> random_end(c,c.toc(-0x7b88));
    Kept<24> random_table(c,c.toc(-0x7b8c));
    Kept<25> constants(c,c.toc(-0x4ad4));
    Kept<26> screens_or_draw_or_owner(c,c.toc(slot::screens));
    Kept<27> random_salt(c,c.toc(-0x7b90));
    Kept<28> control_or_board(c,control_value);
    Kept<29> shortlist_or_club(c,c.toc(-0x7b1c));
    Kept<30> person(c,c.r(30));
    Kept<31> random_cursor(c,c.toc(-0x7b84));
    Program p{c}; Fpu fp(c);
    const auto advance=[&](std::uint32_t back,bool previous_in_r4=false) {
        const auto previous=c.u32(random_cursor),next=previous+2;
        c.set(3,next); if(previous_in_r4)c.set(4,previous);
        const auto order=compare_logical(c,next,c.u32(random_end)); c.put32(random_cursor,next);
        if(order>0) { c.put32(random_cursor,random_table); c.call(0x1077b770,back); c.put32(random_salt,c.r(3)&0xffff); }
    };
    const auto quotient=[&](std::uint32_t value,std::int32_t magic,unsigned shift) {
        const auto high=std::int32_t((std::int64_t(std::int32_t(value))*magic)>>32);
        c.s.wr<1>(reg::xer_ca,high<0 && (std::uint32_t(high)&((1u<<shift)-1))!=0);
        const auto floor=high>>shift;
        return std::int32_t(std::uint32_t(floor)+(floor<0));
    };
    const auto positive_money=[&](std::uint32_t high,std::uint32_t low,unsigned scratch) {
        const bool positive=std::int32_t(high)>0 || (high==0 && low!=0);
        c.set(3,positive ? 1u : 0u); c.set(scratch,0x80000000);
        c.s.wr<1>(reg::xer_ca,!positive); compare(c,c.r(3)); return positive;
    };
    const auto at_least=[&](double first,double second) {
        fp.fcmp(0,first,second); const auto cr=c.cr(0);
        c.set_cr(0,(cr&~2u)|((cr&(4u|2u)) ? 2u : 0u));
        return (c.cr(0)&2u)!=0;
    };
    if(compare(c,control_action(c,screens_or_draw_or_owner,control_or_board),2)==0) return std::uint32_t(-9);
    if(compare(c,control_action(c,screens_or_draw_or_owner,control_or_board),1)!=0) {
        const auto command=p.call(0x102a54b0,0x10425944,{control_or_board});
        if(compare(c,command)!=0) return command;
        const auto fallback=p.call(0x102a83a0,0x10425958,{control_or_board});
        c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0); return fallback;
    }
    const auto id=p.call(0x1073ef70,0x10425340,{screens_or_draw_or_owner,0});
    c.set(5,id*0x70); person=indirect(c,slot::people)+c.r(5);
    const auto adding=p.call(0x1073ef70,0x1042535c,{screens_or_draw_or_owner,1});
    if(compare_logical(c,adding)==0) {
        p.call(0x10310310,0x10425934,{c.u32(shortlist_or_club),person,0}); return std::uint32_t(-9);
    }
    p.call(0x1030fda0,0x10425374,{c.u32(shortlist_or_club),person,0});
    c.set(4,screens_or_draw_or_owner+std::uint32_t(c.i16(screens_or_draw_or_owner+0x2c58))*0x2c0);
    manager_or_bound_or_fee=c.u32(c.r(4)+4); shortlist_or_club=c.u32(manager_or_bound_or_fee+0x24);
    p.call(0x10715880,0x10425390,{f.at(0x58)});
    if(compare_logical(c,shortlist_or_club)==0) shortlist_or_club=c.u32(manager_or_bound_or_fee+0x20);
    if(compare_logical(c,shortlist_or_club)==0) return std::uint32_t(-9);
    control_or_board=p.call(0x10103510,0x104253b4,{indirect(c,-0x7ab8),shortlist_or_club});
    if(compare(c,control_or_board)==0) return std::uint32_t(-9);
    if(byte_result(c,p.call(0x100c7d00,0x104253c4,{shortlist_or_club}))) return std::uint32_t(-9);
    const auto rating=std::int32_t(c.i16(c.u32(person+0x14)+10));
    const auto scaled=quotient(std::uint32_t(rating),0x1b4e81b5,5);
    manager_or_bound_or_fee=37u-std::uint32_t(scaled);
    c.set({{3,scaled<0 ? 1u : 0u},{4,0x1b4e81b5}});
    c.s.wr<1>(reg::xer_ca,37u>=std::uint32_t(scaled));
    if(compare(c,manager_or_bound_or_fee)==0) screens_or_draw_or_owner=0;
    else {
        advance(0x10425420);
        // The rating is signed16: 37-trunc(rating/300) is in[-72,146].
        // Therefore the original's inline wide draw is unreachable.
        c.set(3,65535); compare(c,manager_or_bound_or_fee,65535);
        c.set(3,std::uint32_t(-65535)); compare(c,manager_or_bound_or_fee,-65535);
        compare(c,manager_or_bound_or_fee);
        c.set(3,c.u16(c.u32(random_cursor))+c.u32(random_salt));
        const auto q=std::uint32_t(divide_signed<4>(c.r(3),manager_or_bound_or_fee));
        const auto rest=c.r(3)-q*std::uint32_t(manager_or_bound_or_fee);
        screens_or_draw_or_owner=std::int32_t(std::uint32_t(manager_or_bound_or_fee))>0 ? rest : 0u-rest;
    }
    if(compare(c,screens_or_draw_or_owner)!=0) return std::uint32_t(-9);
    advance(0x10425584);
    const auto chance=c.u16(c.u32(random_cursor))+c.u32(random_salt);
    const auto current_rating=std::int32_t(c.i16(c.u32(person+0x14)+10));
    const auto chance_q=quotient(chance,0x10624dd3,7);
    const auto threshold=chance-std::uint32_t(chance_q)*2000u;
    c.set({{3,std::uint32_t(current_rating)},{4,threshold},{5,chance}});
    if(compare(c,current_rating,std::int32_t(threshold+5000))<=0) {
        if(compare_logical(c,c.u32(person+0x24))==0) return std::uint32_t(-9);
        if(!byte_result(c,p.call(0x1014c2e0,0x104255f4,{c.u32(person+0x24),0,1,0,0}))) return std::uint32_t(-9);
    }
    p.call(0x10369080,0x10425620,{c.u32(c.u32(transfers)+0x988),person,shortlist_or_club,0,0,1,0});
    c.set_f(0,c.f64(constants+0x20));
    if(!at_least(c.f(1),c.f(0))) return std::uint32_t(-9);
    screens_or_draw_or_owner=0;
    const auto first=p.call(0x10082f20,0x10425640,{c.u32(contracts),person});
    compare_logical(c,first); c.put32(current_contract_or_record,first);
    if(first) {
        c.set(3,c.u32(current_contract_or_record));
        const auto encoded=(std::uint32_t(c.u8(c.r(3)+0x38))<<24)&0xfe000000u;
        c.s.wr<1>(reg::xer_ca,std::int32_t(encoded)<0 && (encoded&0x03ffffffu)!=0);
        if(compare(c,std::int8_t(std::int32_t(encoded)>>26),5)!=0) screens_or_draw_or_owner=1;
    }
    if(compare(c,screens_or_draw_or_owner)!=0) {
        c.set({{3,indirect(c,slot::clubs)},{4,c.toc(slot::clubs)}});
        screens_or_draw_or_owner=c.r(3)+c.u32(c.u32(current_contract_or_record)+4)*0x248;
    } else screens_or_draw_or_owner=0;
    current_contract_or_record=p.call(0x10082f20,0x104256a4,{c.u32(contracts),person});
    advance(0x104256c8,true);
    const auto word=c.u16(c.u32(random_cursor)); const auto salt=c.u32(random_salt);
    const auto first_noise=std::uint32_t(word)+salt;
    const auto next=c.u32(random_cursor)+2;
    c.put32(random_cursor,next); const auto wrap=compare_logical(c,next,c.u32(random_end))>0;
    const auto noise_q=quotient(first_noise,0x68db8bad,13);
    c.set({{3,std::int32_t(first_noise)<0 ? 1u : 0u},{4,std::uint32_t((std::int64_t(std::int32_t(first_noise))*0x68db8bad)>>32)},
        {5,first_noise},{6,word}});
    c.put32(f.at(0x48),std::uint32_t(std::int32_t(std::int16_t(first_noise-std::uint32_t(noise_q)*20000u))));
    if(wrap) { c.put32(random_cursor,random_table); c.call(0x1077b770,0x10425728); c.put32(random_salt,c.r(3)&0xffff); }
    const auto second_noise=std::uint32_t(c.u16(c.u32(random_cursor)))+c.u32(random_salt);
    const auto second_q=quotient(second_noise,0x68db8bad,14);
    c.put32(f.at(0x40),std::uint32_t(std::int32_t(std::int16_t(second_noise-std::uint32_t(second_q)*40000u))));
    c.set(10,std::int32_t(second_noise)<0 ? 1u : 0u);
    p.call(0x100f5770,0x1042578c,{control_or_board,person,1,0,1,0,40000});
    c.set(6,0);
    const bool affordable=positive_money(c.r(3),c.r(4),5);
    if(!affordable) c.put32(f.at(0x48),std::uint32_t(-10000));
    else {
        c.set(6,c.u32(control_or_board+4)); c.set(5,0);
        if(!positive_money(c.u32(control_or_board),c.r(6),4)) c.put32(f.at(0x48),std::uint32_t(-10000));
    }
    const auto free_move=p.call(0x1036fbf0,0x10425800,{c.u32(c.u32(transfers)+0x988),f.at(0x3c),current_contract_or_record,shortlist_or_club,f.at(0x38)});
    const bool owner_present=compare_logical(c,screens_or_draw_or_owner)!=0;
    if(!owner_present || byte_result(c,free_move)) manager_or_bound_or_fee=0;
    else {
        const auto controlled=p.call(0x1014c2e0,0x10425830,{screens_or_draw_or_owner,0,1,0,0});
        if(byte_result(c,controlled)) manager_or_bound_or_fee=p.call(0x10089a40,0x1042584c,{c.u32(contracts),screens_or_draw_or_owner,shortlist_or_club,person});
        else manager_or_bound_or_fee=p.call(0x10085860,0x10425874,{c.u32(contracts),screens_or_draw_or_owner,shortlist_or_club,person,0,1,1});
    }
    p.call(0x100f5770,0x10425894,{control_or_board,person,1,0,1,0});
    p.call(0x10773c80,0x10425898);
    const auto fee_fraction=[&] {
        c.set(3,std::uint32_t(manager_or_bound_or_fee)^0x80000000);
        c.put32(f.at(0x6c),c.r(3)); c.set_f(2,c.f64(constants+0x10));
        c.put32(f.at(0x68),0x43300000); c.set_f(3,c.f64(constants+0x28));
        c.set_f(0,fp.fsub(c.f64(f.at(0x68)),c.f(2))); c.set_f(0,fp.fmul(c.f(3),c.f(0)));
    };
    fee_fraction();
    bool approved=at_least(c.f(1),c.f(0));
    if(!approved) {
        p.call(0x10773c80,0x104258d4,{c.u32(control_or_board),c.u32(control_or_board+4)});
        fee_fraction(); approved=at_least(c.f(1),c.f(0));
    }
    if(approved) {
        c.set_f(1,c.f64(constants+0x30));
        p.call(0x10698bb0,0x10425920,{indirect(c,-0x7b94),person,shortlist_or_club,manager_or_bound_or_fee});
    }
    return std::uint32_t(-9);
}
}
