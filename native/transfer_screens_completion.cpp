#include "transfer_screens_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_transfer_completion(Cpu& c) {
    Frame f(c,0x10423270,0xea0,{.first_gpr=16});
    Kept<16> clubs_or_grid(c,c.toc(slot::clubs));
    Kept<17> disabled(c,c.toc(-0x7bc4));
    Kept<18> assertion_flag(c,c.toc(-0x7bac));
    Kept<19> foreground(c,c.toc(-0x7bc8));
    Kept<20> people(c,c.toc(slot::people));
    Kept<21> person(c,c.r(21));
    Kept<22> current_contract_or_status(c,c.toc(slot::current_contract));
    Kept<23> error(c,c.toc(-0x7ba8));
    Kept<24> source_slot_or_owner(c,c.r(24));
    Kept<25> draft(c,c.r(25));
    Kept<26> allocated_or_bidder(c,c.r(26));
    Kept<27> screens(c,c.toc(slot::screens));
    Kept<28> club(c,c.r(28));
    Kept<29> text(c,c.toc(-0x7bb8));
    Kept<30> exchange_count(c,0);
    Kept<31> pool(c,c.toc(slot::pool));
    Program p{c}; Fpu fp(c);
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38; for(auto value:values) { c.put32(f.at(offset),value); offset+=4; }
    };
    c.set(3,c.u16(c.toc(-0x7b14))); c.put16(f.at(0x4a),c.u16(foreground)); c.put16(f.at(0x4c),c.r(3));
    source_slot_or_owner=p.call(0x1073ef70,0x104232cc,{screens,0});
    c.set(5,c.toc(-0x7a54)); draft=p.call(0x10148a90,0x104232e0,{c.u32(c.r(5)),0});
    c.set(5,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4)); club=c.u32(c.r(5)+0x24);
    const auto person_id=p.call(0x1073ef70,0x10423304,{screens,5});
    c.set(4,person_id*0x70); c.set(3,c.u32(people)); person=c.r(3)+c.r(4);
    if(compare_logical(c,c.u32(draft))==0) {
        allocated_or_bidder=p.call(0x10773520,0x10423324,{0x54});
        if(compare(c,allocated_or_bidder)!=0)
            p.call(0x10411ea0,0x10423348,{allocated_or_bidder,person,club,0,missing,0});
        c.put32(draft,allocated_or_bidder);
        c.set(4,c.u32(source_slot_or_owner)); c.set(3,c.u32(draft));
        copy_offer_fields(c,c.r(3),c.r(4));
        c.set(5,c.u8(c.r(4)+0x50)); c.set(4,c.u8(c.r(4)+0x51));
        c.set(3,c.u32(draft)); c.put32(c.r(3),0);
        c.set(3,c.u32(draft));
        if(compare_logical(c,c.u32(c.r(3)+4))!=0) {
            allocated_or_bidder=p.call(0x10773520,0x10423450,{16});
            if(compare(c,allocated_or_bidder)!=0) p.call(0x10738c00,0x10423460,{allocated_or_bidder});
            c.put32(c.u32(draft)+4,allocated_or_bidder);
            p.call(0x10739010,0x1042347c,{c.u32(c.u32(draft)+4),c.u32(c.u32(source_slot_or_owner)+4)});
        }
    }
    draft=c.u32(draft); source_slot_or_owner=0;
    c.set(5,c.u32(clubs_or_grid)); allocated_or_bidder=c.r(5)+c.u32(draft+8)*0x248;
    const auto record=p.call(0x10082f20,0x104234a4,{indirect(c,slot::contracts),person});
    compare_logical(c,record); c.put32(current_contract_or_status,record);
    if(record) {
        c.set(3,c.u32(current_contract_or_status));
        const auto encoded=(std::uint32_t(c.u8(c.r(3)+0x38))<<24)&0xfe000000u;
        c.s.wr<1>(reg::xer_ca,std::int32_t(encoded)<0 && (encoded&0x03ffffffu)!=0);
        if(compare(c,std::int8_t(std::int32_t(encoded)>>26),5)!=0) source_slot_or_owner=1;
    }
    if(compare(c,source_slot_or_owner)!=0) {
        c.set(3,c.u32(current_contract_or_status)); c.set(4,c.u32(clubs_or_grid));
        source_slot_or_owner=c.r(4)+c.u32(c.r(3)+4)*0x248;
    } else source_slot_or_owner=0;
    p.call(0x107136d0,0x10423508,{person,0,0});
    p.call(0x100c7a50,0x1042351c,{club,1,f.at(0x4c),f.at(0x4a)});
    p.call(0x10744250,0x10423530,{screens,c.u16(f.at(0x4a)),c.u16(f.at(0x4c)),club+0x1ee});
    current_contract_or_status=p.call(0x103f9d60,0x1042354c,{indirect(c,slot::transfers),draft,club,f.at(0x48),1});
    p.history(0x10423564,screens,3,std::uint32_t(std::int32_t(c.i16(f.at(0x48)))));
    p.history(0x10423578,screens,6,std::uint32_t(std::int32_t(std::int8_t(current_contract_or_status))));
    if(!byte_result(c,current_contract_or_status)) p.call(0x10727890,0x1042358c,{text,pool+0x182f});
    else if(compare(c,std::int8_t(current_contract_or_status),1)==0) p.call(0x10727890,0x104235a8,{text,pool+0x1846});
    else p.call(0x10727890,0x104235b8,{text,pool+0x185b});
    p.call(0x107442f0,0x104235c4,{screens,text});
    p.call(0x100caeb0,0x104235e4,{c.u32(people)+c.u32(draft+12)*0x70,f.at(0x104),400,1});
    if(!byte_result(c,current_contract_or_status)) {
        if(compare_logical(c,c.u32(draft+4))!=0) {
            c.set(3,c.u32(draft+4)); c.put32(c.r(3)+12,c.u32(c.r(3)+4));
            for(;;) {
                const auto payload=p.call(0x10738e00,0x104236c0,{c.u32(draft+4)});
                if(compare_logical(c,payload)==0) break;
                if(compare(c,std::int8_t(exchange_count),5)<0) {
                    const auto index=std::int32_t(std::int8_t(exchange_count));
                    const auto exchanged=c.u32(people)+c.u32(payload)*0x70;
                    c.set(7,exchanged); c.put32(f.at(0x88)+std::uint32_t(index)*4,exchanged);
                    p.call(0x100caeb0,0x10423660,{exchanged,f.at(0x67c)+std::uint32_t(index)*400,400,1});
                    exchange_count=exchange_count+1;
                } else report_assertion(c,{0x1042367c,pool,pool+0x15,pool+0x22,f.at(0x68),f.at(0x60),0x15d9});
            }
            if(compare(c,std::int8_t(exchange_count),1)==0) p.call(0x1077dbf0,0x104236e0,{f.at(0x294),f.at(0x67c)});
            else if(compare(c,std::int8_t(exchange_count),2)==0) p.call(0x10727890,0x10423704,{f.at(0x294),pool+0x186e,f.at(0x67c),f.at(0x80c)});
            else if(compare(c,std::int8_t(exchange_count),3)==0) p.call(0x10727890,0x1042372c,{f.at(0x294),pool+0x18ba,f.at(0x67c),f.at(0x80c),f.at(0x99c)});
            else if(compare(c,std::int8_t(exchange_count),4)==0) p.call(0x10727890,0x10423758,{f.at(0x294),pool+0x1928,f.at(0x67c),f.at(0x80c),f.at(0x99c),f.at(0xb2c)});
            else if(compare(c,std::int8_t(exchange_count),5)==0) p.call(0x10727890,0x10423788,{f.at(0x294),pool+0x19b8,f.at(0x67c),f.at(0x80c),f.at(0x99c),f.at(0xb2c),f.at(0xcbc)});
        }
        // The original wraps the percentage product and rounds both amounts
        // through FSUBS before comparing them, even above 2^24.
        const auto product=c.u32(person+8)*std::uint32_t(std::int32_t(c.i8(draft+0x50)));
        const auto high=std::int32_t((std::int64_t(std::int32_t(product))*0x51eb851f)>>32);
        c.s.wr<1>(reg::xer_ca,high<0 && (std::uint32_t(high)&31u)!=0);
        const auto floor=high>>5; const auto percent=std::uint32_t(floor)+(floor<0);
        const auto base=c.u32(draft+0x20), extra=c.u32(draft+0x40)+c.u32(draft+0x48);
        c.set({{3,percent},{4,c.toc(-0x4ad4)},{5,std::uint32_t(floor)},{6,c.toc(-0x4ad4)},
            {7,std::uint32_t(high)},{8,extra},{9,base},{10,c.u32(draft+0x48)}});
        c.put32(f.at(0xe54),0x43300000); c.put32(f.at(0xe50),base^0x80000000);
        c.set_f(1,c.f64(c.r(4)+0x10)); c.put32(f.at(0xe4c),0x43300000);
        c.set_f(2,c.f64(c.r(6)+0x10)); c.set_f(0,c.f64(f.at(0xe4c)));
        c.put32(f.at(0xe58),(base+extra+percent)^0x80000000);
        c.set_f(0,fp.fsubs(c.f(0),c.f(1))); c.set_f(1,fp.fsubs(c.f64(f.at(0xe54)),c.f(2)));
        fp.fcmp(0,c.f(1),c.f(0));
        const auto club_is_bidder=[&] { c.set(3,c.u32(club)); return compare(c,c.r(3),c.u32(draft+8))==0; };
        // Preserve the variadic width; unused incoming registers remain intact.
        const auto format_description=[&](std::uint32_t who,std::uint32_t format,std::uint32_t back,
                                           std::initializer_list<std::uint32_t> suffix) {
            unsigned reg=9; for(auto value:suffix)c.set(reg++,value);
            p.call(0x10727890,back,{text,pool+format,std::uint32_t(std::int32_t(c.i8(who+0x23b))),
                std::uint32_t(std::int32_t(c.i8(who+0x23b))),f.at(0x104),who+0x221});
        };
        // BLE branches when CR0.GT is clear, including unordered results.
        if((c.cr(0)&4u)!=0) {
            c.put64(f.at(0xe4c),fp.fctiwz(0,c.f(1))); c.put32(f.at(0x5c),c.u32(f.at(0xe50)));
            p.call(0x10009f20,0x10423830,{f.at(0x5c),6});
            p.call(0x1000b410,0x10423848,{f.at(0xd0),50,c.r(3),c.r(4),6});
            if(!byte_result(c,exchange_count)) {
                if(club_is_bidder()) format_description(source_slot_or_owner,0x1a69,0x10423888,{f.at(0xd0)});
                else format_description(allocated_or_bidder,0x1add,0x104238b4,{f.at(0xd0)});
            } else {
                if(club_is_bidder()) format_description(source_slot_or_owner,0x1b4f,0x104238f4,{f.at(0xd0),f.at(0x294)});
                else format_description(allocated_or_bidder,0x1c0c,0x10423924,{f.at(0xd0),f.at(0x294)});
            }
        } else {
            c.put32(f.at(0x58),c.u32(draft+0x20));
            p.call(0x10009f20,0x1042393c,{f.at(0x58),6});
            p.call(0x1000b410,0x10423954,{f.at(0x9c),50,c.r(3),c.r(4),6});
            if(!byte_result(c,exchange_count)) {
                if(compare(c,c.u32(draft+0x20))>0) {
                    if(club_is_bidder()) format_description(source_slot_or_owner,0x1cc7,0x104239a0,{f.at(0x9c)});
                    else format_description(allocated_or_bidder,0x1d37,0x104239cc,{f.at(0x9c)});
                } else if(club_is_bidder()) {
                    if(compare_logical(c,source_slot_or_owner)!=0) format_description(source_slot_or_owner,0x1da5,0x10423a0c,{});
                    else p.call(0x10727890,0x10423a20,{text,pool+0x1df5,f.at(0x104)});
                } else format_description(allocated_or_bidder,0x1e1f,0x10423a48,{});
            } else if(compare(c,c.u32(draft+0x20))>0) {
                if(club_is_bidder()) format_description(source_slot_or_owner,0x1e6d,0x10423a94,{f.at(0x9c),f.at(0x294)});
                else format_description(allocated_or_bidder,0x1f18,0x10423ac4,{f.at(0x9c),f.at(0x294)});
            } else {
                if(club_is_bidder()) format_description(source_slot_or_owner,0x1fc1,0x10423b00,{f.at(0x294)});
                else format_description(allocated_or_bidder,0x205a,0x10423b2c,{f.at(0x294)});
            }
        }
    } else {
        struct ReasonText { int reason; unsigned format,back; };
        constexpr ReasonText messages[]{
            {3,0x20f1,0x10423b64},{4,0x2198,0x10423b94},{0x22,0x222d,0x10423bc4},
            {5,0x2198,0x10423bf4},{0x23,0x222d,0x10423c24},{0x17,0x22c2,0x10423c54},
            {0x18,0x233c,0x10423c84},{0xe,0x23c2,0x10423cb4},{0x14,0x241e,0x10423ce4},
            {0x15,0x2476,0x10423d14},{0xc,0x24ce,0x10423d44},{0xd,0x2532,0x10423d74},
            {0xf,0x259a,0x10423da4},{0x10,0x2605,0x10423dd4},{0x11,0x265c,0x10423e04}};
        for(const auto& message:messages) {
            if(compare(c,c.i16(f.at(0x48)),message.reason)!=0)continue;
            const auto who=message.reason==5 ? std::uint32_t(source_slot_or_owner) :
                (message.reason==3 || message.reason==4 || message.reason==0x22 || message.reason==0x23) ? std::uint32_t(allocated_or_bidder) : std::uint32_t(club);
            if(message.reason==3) p.call(0x10727890,message.back,{text,pool+message.format,
                std::uint32_t(std::int32_t(c.i8(who+0x242))),std::uint32_t(std::int32_t(c.i8(who+0x242))),100,who+0x1ee,f.at(0x104)});
            else p.call(0x10727890,message.back,{text,pool+message.format,
                std::uint32_t(std::int32_t(c.i8(who+0x242))),std::uint32_t(std::int32_t(c.i8(who+0x242))),who+0x1ee});
            break;
        }
    }
    const auto rows=std::int8_t(divide_signed<4>(405,p.call(0x1076d470,0x10423e0c,{3})));
    stack_args({c.u16(foreground),text,0});
    p.call(0x1074a350,0x10423e4c,{110,140,780,545,2,0,std::uint32_t(std::int32_t(rows)),3});
    p.call(0x102a1070,0x10423e58,{4,0});
    const auto grid=[&](std::uint32_t back,unsigned columns,std::uint32_t descriptor) {
        stack_args({0,1,0,missing});
        clubs_or_grid=p.call(0x107198c0,back,{c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,columns,descriptor,1});
    };
    const auto button=[&](std::uint32_t format,std::uint32_t format_back,std::uint32_t button_back,unsigned column,unsigned action) {
        p.call(0x10727890,format_back,{text,pool+format});
        stack_args({c.u16(foreground),text,action,0});
        p.call(0x107444e0,button_back,{screens,clubs_or_grid,column,0,0x30,c.u16(disabled),12,3});
    };
    if(compare(c,std::int8_t(current_contract_or_status),2)==0) {
        grid(0x10423eb4,1,0); button(0x5ed,0x10423ec8,0x10423f08,0,2);
    } else if(compare(c,std::int8_t(current_contract_or_status),1)==0) {
        c.put16(f.at(0x54),c.u16(c.r(2)-0x37b0)); c.put8(f.at(0x56),c.u8(c.r(2)-0x37ae));
        grid(0x10423f78,3,f.at(0x54));
        button(0x5ed,0x10423f8c,0x10423fcc,0,2); button(0x5fb,0x10423fd8,0x10424018,1,1);
        button(0x620,0x10424024,0x10424064,2,4);
    } else {
        c.put16(f.at(0x50),c.u16(c.r(2)-0x37ac)); c.put16(f.at(0x52),c.u16(c.r(2)-0x37aa));
        grid(0x104240c8,4,f.at(0x50));
        button(0x5ed,0x104240dc,0x1042411c,0,2); button(0x5b2,0x10424128,0x10424168,1,1);
        button(0x620,0x10424174,0x104241b4,2,4); button(0x5b5,0x104241c0,0x10424200,3,3);
    }
}
}
