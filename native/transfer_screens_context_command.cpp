#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
std::uint32_t offer_context_command(Cpu& c, std::uint32_t control_value) {
    Frame f(c,0x1041f3a0,0x60,{.first_gpr=24});
    Kept<24> original_link(c,c.r(24));
    Kept<25> draft_link(c,c.r(25));
    Kept<26> manager_club_or_person(c,c.r(26));
    Kept<27> reason_or_owner(c,c.r(27));
    Kept<28> transfers(c,c.toc(slot::transfers));
    Kept<29> current_contract(c,c.toc(slot::current_contract));
    Kept<30> screens(c,c.toc(slot::screens));
    Kept<31> control(c,control_value);
    Program p{c};
    reason_or_owner=p.call(0x1073ef70,0x1041f3cc,{screens,1});
    c.set(4,std::uint32_t(c.i16(screens+0x2c58))*0x2c0);
    if (compare(c,control_action(c,screens,control),3)==0) {
        const auto manager=c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4);
        c.set(5,manager);
        manager_club_or_person=c.u32(manager+0x24);
        draft_link=p.call(0x1073ef70,0x1041f448,{screens,0});
        original_link=p.call(0x1073ef70,0x1041f45c,{screens,2});
        c.set(5,c.u32(compare_logical(c,draft_link)!=0 ? std::uint32_t(draft_link) : std::uint32_t(original_link)));
        const auto dispose_draft=[&](std::uint32_t back) {
            if (compare_logical(c,draft_link)!=0 && compare_logical(c,draft_link,original_link)!=0) {
                p.call(0x10411f10,back,{c.u32(draft_link),1}); c.put32(draft_link,0);
            }
        };
        if (compare(c,reason_or_owner,35)==0) {
            p.call(0x103e8500,0x1041f490,{c.u32(transfers),c.u32(original_link),manager_club_or_person,8});
            dispose_draft(0x1041f4ac); return std::uint32_t(-10);
        }
        if (compare(c,reason_or_owner,2)==0) {
            c.set(3,c.u32(draft_link));
            compare_logical(c,original_link);
            c.set(3,std::uint32_t(std::int32_t(c.i8(c.r(3)+0x35))));
            c.set(5,c.r(3)==2);
            if (std::uint32_t(original_link)) {
                bool is_bidder=false;
                if (compare_logical(c,manager_club_or_person)!=0) {
                    c.set({{3,c.u32(original_link)},{4,c.u32(manager_club_or_person)}});
                    is_bidder=compare(c,c.r(4),c.u32(c.r(3)+8))==0;
                }
                p.call(is_bidder ? 0x103e44b0 : 0x103e4130,
                    is_bidder ? 0x1041f51c : 0x1041f530,{c.u32(transfers),original_link,c.r(5),0});
            }
            dispose_draft(0x1041f54c); return std::uint32_t(-10);
        }
        if (compare(c,reason_or_owner,1)==0) {
            p.call(0x103e25d0,0x1041f574,{c.u32(transfers),c.u32(draft_link),0});
            dispose_draft(0x1041f590); return std::uint32_t(-10);
        }
        if (compare(c,reason_or_owner,27)==0) {
            const auto response=c.i8(c.r(5)+0x35);
            p.call(0x103e4390,0x1041f5d8,{c.u32(transfers),original_link,std::uint32_t(response==2),0});
            dispose_draft(0x1041f5f4); return std::uint32_t(-10);
        }
        if (compare(c,reason_or_owner,28)==0) {
            const auto draft=c.u32(draft_link);
            c.set({{7,c.u32(draft+12)*0x70},{8,indirect(c,slot::people)}});
            manager_club_or_person=c.r(8)+c.r(7);
            p.call(0x103e2db0,0x1041f658,{c.u32(transfers),c.u32(original_link),draft,std::uint32_t(c.i8(draft+0x35)==2)});
            c.set({{4,c.u32(c.u32(draft_link)+0x20)},{3,c.u32(original_link)}});
            if (compare(c,c.r(4),c.u32(c.r(3)+0x20))!=0) {
                const auto contract=p.call(0x10082f20,0x1041f680,{indirect(c,slot::contracts),manager_club_or_person});
                compare_logical(c,contract); c.put32(current_contract,contract);
                if (contract) {
                    c.set({{3,c.u32(current_contract)},{4,c.toc(slot::clubs)}});
                    const auto id=c.u32(c.r(3)+4); c.set(3,c.u32(c.r(4)));
                    reason_or_owner=c.r(3)+id*0x248;
                } else reason_or_owner=0;
                const auto amount=p.call(0x10416710,0x1041f6c4,{c.u32(draft_link),1,0,0,1});
                p.call(0x103f6240,0x1041f6e8,{c.u32(transfers),manager_club_or_person,reason_or_owner,amount,
                    f.at(0x38),missing,0,1});
            }
            dispose_draft(0x1041f704); return std::uint32_t(-10);
        }
        if (compare(c,reason_or_owner,29)==0) {
            const auto response=c.i8(c.r(5)+0x35);
            const auto loan=c.u8(c.r(5)+0x24);
            const auto temporary=compare(c,loan,1)==0;
            p.call(temporary ? 0x103e4c90 : 0x103e4740,temporary ? 0x1041f754 : 0x1041f764,
                {c.u32(transfers),original_link,std::uint32_t(response==2)});
            dispose_draft(0x1041f780); return std::uint32_t(-10);
        }
    }
    if (compare(c,control_action(c,screens,control),2)==0) return std::uint32_t(-9);
    const auto command=p.call(0x102a54b0,0x1041f7f8,{control});
    if (compare(c,command)!=0) return command;
    const auto fallback=p.call(0x102a83a0,0x1041f80c,{control});
    c.s.wr<1>(reg::xer_ca,(fallback & 0x7fffffff)!=0); return fallback;
}
}
