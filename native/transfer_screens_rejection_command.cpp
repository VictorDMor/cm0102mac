#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
std::uint32_t bid_rejection_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x104204f0,0x80,{.first_gpr=23});
    Kept<23> amount_or_draft(c,c.r(23));
    Kept<24> person_or_original(c,c.r(24));
    Kept<25> people_or_original_or_person(c,c.toc(slot::people));
    Kept<26> clubs_or_owner(c,c.toc(slot::clubs));
    Kept<27> contracts(c,c.toc(slot::contracts));
    Kept<28> current_contract(c,c.toc(slot::current_contract));
    Kept<29> transfers(c,c.toc(slot::transfers));
    Kept<30> screens_or_draft_or_reason(c,c.toc(slot::screens));
    Kept<31> control_or_reason_or_amount(c,control_value);
    Program p{c};
    const auto owner=[&](std::uint32_t person,std::uint32_t back,unsigned destination) {
        const auto record=p.call(0x10082f20,back,{c.u32(contracts),person});
        compare_logical(c,record); c.put32(current_contract,record);
        if(record) { c.set({{3,c.u32(current_contract)},{4,c.u32(clubs_or_owner)}}); c.set(destination,c.r(4)+c.u32(c.r(3)+4)*0x248); }
        else c.set(destination,0);
    };
    if(compare(c,control_action(c,screens_or_draft_or_reason,control_or_reason_or_amount),1)==0) {
        const bool no_control=compare(c,std::int16_t(control_or_reason_or_amount),-1)==0;
        c.set(3,c.u32(screens_or_draft_or_reason+std::uint32_t(c.i16(screens_or_draft_or_reason+0x2c58))*0x2c0+0x24));
        if(no_control) c.set(5,0);
        else { c.set(3,c.r(3)+0xc0000+std::uint32_t(std::int16_t(control_or_reason_or_amount))*0x17c); c.set(5,c.u32(c.r(3)-0x5754)); }
        p.history(0x104205bc,screens_or_draft_or_reason,2,c.r(5)); return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens_or_draft_or_reason,control_or_reason_or_amount),4)==0) {
        p.call(0x10738c00,0x10420624,{f.at(0x3c)});
        const auto reason=p.call(0x1073ef70,0x10420630,{screens_or_draft_or_reason,2});
        control_or_reason_or_amount=std::uint32_t(std::int32_t(std::int16_t(reason)));
        const auto draft=p.call(0x1073ef70,0x10420644,{screens_or_draft_or_reason,0});
        c.set(3,screens_or_draft_or_reason); screens_or_draft_or_reason=draft;
        const auto original=p.call(0x1073ef70,0x10420658,{c.r(3),1});
        c.set({{4,c.u32(screens_or_draft_or_reason)},{6,c.u32(people_or_original_or_person)}});
        people_or_original_or_person=original;
        person_or_original=c.r(6)+c.u32(c.r(4)+12)*0x70;
        amount_or_draft=p.call(0x103ea760,0x10420680,{c.u32(transfers),person_or_original,control_or_reason_or_amount});
        c.set({{4,c.u32(people_or_original_or_person)},{5,c.u32(screens_or_draft_or_reason)}});
        c.set(3,c.u32(c.r(5)+0x20));
        if(compare(c,c.r(3),c.u32(c.r(4)+0x20))!=0) {
            owner(person_or_original,0x104206a8,26);
            const auto total=p.call(0x10416710,0x104206e8,{c.u32(screens_or_draft_or_reason),1,0,0,1});
            p.call(0x103f6240,0x1042070c,{c.u32(transfers),person_or_original,clubs_or_owner,total,f.at(0x39),amount_or_draft,1,1});
        } else {
            owner(person_or_original,0x1042071c,5);
            p.call(0x103f6240,0x10420764,{c.u32(transfers),person_or_original,c.r(5),c.u32(person_or_original+8),f.at(0x39),amount_or_draft,0,1});
        }
        p.call(0x103eaea0,0x10420778,{c.u32(transfers),person_or_original,f.at(0x3c),0});
        c.put32(f.at(0x48),c.u32(f.at(0x40)));
        for(;;) {
            const auto node=p.call(0x10738e00,0x104207bc,{f.at(0x3c)}); c.set(4,node);
            if(compare(c,node)==0) break;
            const auto link=p.call(0x103eb440,0x10420790,{c.u32(transfers),c.u32(node)});
            c.set(4,link); c.set(3,c.u32(link));
            if(compare(c,c.i8(c.r(3)+0x34))==0)
                p.call(0x103e4130,0x104207b4,{c.u32(transfers),c.r(4),0,control_or_reason_or_amount});
        }
        if(compare_logical(c,screens_or_draft_or_reason,people_or_original_or_person)!=0) {
            p.call(0x10411f10,0x104207d8,{c.u32(screens_or_draft_or_reason),1}); c.put32(screens_or_draft_or_reason,0);
        }
        amount_or_draft=std::uint32_t(-10);
        p.call(0x10738c20,0x104207f0,{f.at(0x3c),missing}); return amount_or_draft;
    }
    if(compare(c,control_action(c,screens_or_draft_or_reason,control_or_reason_or_amount),2)==0) {
        amount_or_draft=p.call(0x1073ef70,0x1042085c,{screens_or_draft_or_reason,0});
        person_or_original=p.call(0x1073ef70,0x10420870,{screens_or_draft_or_reason,1});
        const auto reason=p.call(0x1073ef70,0x10420884,{screens_or_draft_or_reason,2});
        c.set(4,c.u32(amount_or_draft)); screens_or_draft_or_reason=std::uint32_t(std::int32_t(std::int16_t(reason)));
        c.set(6,c.u32(people_or_original_or_person));
        people_or_original_or_person=c.r(6)+c.u32(c.r(4)+12)*0x70;
        const auto value=p.call(0x103ea760,0x104208ac,{c.u32(transfers),people_or_original_or_person,screens_or_draft_or_reason});
        control_or_reason_or_amount=value;
        const bool negotiated=c.i8(c.u32(amount_or_draft)+0x35)==2;
        p.call(0x103e4130,0x104208e8,{c.u32(transfers),person_or_original,std::uint32_t(negotiated),screens_or_draft_or_reason});
        c.set({{3,c.u32(person_or_original)},{4,c.u32(c.u32(amount_or_draft)+0x20)}});
        if(compare(c,c.r(4),c.u32(c.r(3)+0x20))!=0) {
            owner(people_or_original_or_person,0x1042090c,26);
            const auto total=p.call(0x10416710,0x1042094c,{c.u32(amount_or_draft),1,0,0,1});
            p.call(0x103f6240,0x10420970,{c.u32(transfers),people_or_original_or_person,clubs_or_owner,total,f.at(0x38),control_or_reason_or_amount,0,1});
        } else {
            owner(people_or_original_or_person,0x10420980,5);
            p.call(0x103f6240,0x104209c8,{c.u32(transfers),people_or_original_or_person,c.r(5),c.u32(people_or_original_or_person+8),f.at(0x38),control_or_reason_or_amount,0,1});
        }
        if(compare_logical(c,amount_or_draft)!=0 && compare_logical(c,amount_or_draft,person_or_original)!=0) {
            p.call(0x10411f10,0x104209e4,{c.u32(amount_or_draft),1}); c.put32(amount_or_draft,0);
        }
        return std::uint32_t(-10);
    }
    if(compare(c,control_action(c,screens_or_draft_or_reason,control_or_reason_or_amount),3)==0) return std::uint32_t(-9);
    const auto command=p.call(0x102a54b0,0x10420a5c,{control_or_reason_or_amount});
    if(compare(c,command)!=0)return command;
    const auto fallback=p.call(0x102a83a0,0x10420a70,{control_or_reason_or_amount});
    c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0); return fallback;
}
}
