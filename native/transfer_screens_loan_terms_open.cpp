#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
namespace {
std::uint32_t loan_percentage(Cpu& c,std::uint32_t numerator,std::uint32_t salary) {
    // Unlike the accepted-offer opener, these screens narrow the quotient
    // to a signed byte before truncating to a multiple of ten, without bias.
    const auto ratio=std::int8_t(divide_signed<4>(numerator,salary));
    const auto high=std::int32_t((std::int64_t(ratio)*0x66666667LL)>>32);
    c.s.wr<1>(reg::xer_ca,high<0 && (std::uint32_t(high)&3u)!=0);
    return std::uint32_t(std::int32_t(std::int8_t((int(ratio)/10)*10)));
}
}
std::uint32_t open_loan_offer_terms(Cpu& c,std::uint32_t person_address) {
    Frame f(c,0x1042c2c0,0x390,{.first_gpr=25});
    Kept<25> person_or_percentage(c,person_address);
    Kept<26> text_or_contract(c,c.toc(-0x7bb8));
    Kept<27> manager(c,c.r(27));
    Kept<28> transfers_or_proposed(c,c.toc(slot::transfers));
    Kept<29> contracts(c,c.toc(slot::contracts));
    Kept<30> current_contract(c,c.toc(slot::current_contract));
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    manager=c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4);
    const auto eligible=p.call(0x103fa290,0x1042c30c,{c.u32(transfers_or_proposed),c.u32(manager+0x24),person_or_percentage,text_or_contract});
    if (!byte_result(c,eligible)) {
        p.call(0x100caeb0,0x1042c328,{person_or_percentage,f.at(0x1d4),400,5});
        p.call(0x10727890,0x1042c33c,{f.at(0x44),c.toc(slot::pool)+0x65d,f.at(0x1d4)});
        p.call(0x100c7a50,0x1042c350,{c.u32(manager+0x24),1,f.at(0x3c),f.at(0x3a)});
        p.call(0x100f05a0,0x1042c37c,{c.u32(manager+0x24)+0x1ee,f.at(0x44),text_or_contract,0,
            std::uint32_t(std::int32_t(c.i16(f.at(0x3a)))),std::uint32_t(std::int32_t(c.i16(f.at(0x3c)))),0});
        return 0;
    }
    if (!byte_result(c,p.call(0x1073e4b0,0x1042c3a0,{screens,c.toc(-0x4b6c),c.toc(-0x4b70),1,0,0}))) return 0;
    p.history(0x1042c3c4,screens,0,person_or_percentage);
    text_or_contract=p.call(0x10082f20,0x1042c3d0,{c.u32(contracts),person_or_percentage});
    p.history(0x1042c3ec,screens,1,c.u32(text_or_contract+12));
    const auto board=p.call(0x10103510,0x1042c3fc,{indirect(c,-0x7ab8),c.u32(manager+0x24)});
    const auto wage_limit=p.call(0x100f2680,0x1042c408,{board,person_or_percentage,missing});
    p.history(0x1042c41c,screens,2,wage_limit);
    const auto record=p.call(0x10082f20,0x1042c428,{c.u32(contracts),person_or_percentage});
    compare_logical(c,record); c.put32(current_contract,record);
    if (record) {
        c.set(3,c.u32(current_contract)); c.set(4,c.toc(slot::clubs));
        c.set(5,c.u32(c.r(4))+c.u32(c.r(3)+4)*0x248);
    } else c.set(5,0);
    p.call(0x103fa8e0,0x1042c474,{c.u32(transfers_or_proposed),c.u32(manager+0x24),c.r(5),person_or_percentage,
        f.at(0x39),f.at(0x40),f.at(0x38),f.at(0x3e)});
    p.history(0x1042c48c,screens,3,std::uint32_t(std::int32_t(c.i8(f.at(0x39)))));
    p.history(0x1042c4a0,screens,4,c.u16(f.at(0x40)));
    p.history(0x1042c4b8,screens,5,std::uint32_t(std::int32_t(c.i8(f.at(0x38)))));
    p.history(0x1042c4cc,screens,6,c.u16(f.at(0x3e)));
    p.history(0x1042c4e0,screens,7,0); p.history(0x1042c4f4,screens,8,0);
    p.history(0x1042c508,screens,11,0);
    p.history(0x1042c520,screens,12,std::uint32_t(std::int32_t(c.i8(f.at(0x39)))));
    p.history(0x1042c534,screens,13,c.u16(f.at(0x40)));
    p.history(0x1042c548,screens,14,1); p.history(0x1042c55c,screens,15,1);
    transfers_or_proposed=p.call(0x1007f720,0x1042c570,{c.u32(contracts),c.u32(person_or_percentage),c.u32(c.u32(manager+0x24))});
    if (compare(c,transfers_or_proposed)!=0) {
        const auto encoded=(std::uint32_t(c.u8(transfers_or_proposed+0x38))<<24)&0xfe000000u;
        c.s.wr<1>(reg::xer_ca,std::int32_t(encoded)<0 && (encoded&0x03ffffffu)!=0);
        if (compare(c,std::int32_t(encoded)>>26,6)==0) {
            c.set(7,c.u32(transfers_or_proposed+12)*100u);
            person_or_percentage=loan_percentage(c,c.r(7),c.u32(text_or_contract+12));
            p.history(0x1042c5d8,screens,11,person_or_percentage);
            auto month=p.call(0x10718360,0x1042c5e0,{transfers_or_proposed+0x30});
            p.history(0x1042c5f8,screens,12,std::uint32_t(std::int32_t(std::int8_t(month))));
            p.history(0x1042c60c,screens,13,c.u16(transfers_or_proposed+0x32));
            p.history(0x1042c624,screens,14,std::uint32_t(std::int32_t(c.i8(transfers_or_proposed+0x1c))));
            p.history(0x1042c63c,screens,15,std::uint32_t(std::int32_t(c.i8(transfers_or_proposed+0x1d))));
            p.history(0x1042c650,screens,16,person_or_percentage);
            month=p.call(0x10718360,0x1042c658,{transfers_or_proposed+0x30});
            p.history(0x1042c670,screens,17,std::uint32_t(std::int32_t(std::int8_t(month))));
            p.history(0x1042c684,screens,18,c.u16(transfers_or_proposed+0x32));
            p.history(0x1042c69c,screens,19,std::uint32_t(std::int32_t(c.i8(transfers_or_proposed+0x1c))));
            p.history(0x1042c6b4,screens,20,std::uint32_t(std::int32_t(c.i8(transfers_or_proposed+0x1d))));
        }
    }
    return 1;
}

void open_loan_terms_from_news(Cpu& c,std::uint32_t manager_address,std::uint32_t item) {
    Frame f(c,0x1042bf70,0x170,{.first_gpr=24});
    Kept<24> item_or_revision_or_proposed(c,item);
    Kept<25> manager_or_link_or_percentage(c,manager_address);
    Kept<26> previous_command_or_person(c,c.toc(-0x4b04));
    Kept<27> previous_screen(c,c.toc(-0x4b00));
    Kept<28> error_buffer(c,c.toc(-0x7ba8));
    Kept<29> pool(c,c.toc(slot::pool));
    Kept<30> transfers(c,c.toc(slot::transfers));
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    p.call(0x102bdf90,0x1042bfb0,{f.at(0x60),0,0});
    p.call(0x102ccdd0,0x1042bfc8,{indirect(c,-0x7bf0),manager_or_link_or_percentage,item_or_revision_or_proposed,f.at(0x60)});
    manager_or_link_or_percentage=p.call(0x102cb570,0x1042bfd4,{f.at(0x60),6});
    item_or_revision_or_proposed=p.call(0x102cb570,0x1042bfe8,{f.at(0x60),7});
    manager_or_link_or_percentage=p.call(0x103eb440,0x1042bffc,{c.u32(transfers),manager_or_link_or_percentage});
    c.set(3,c.u32(manager_or_link_or_percentage));
    bool previous=compare(c,c.u32(c.r(3)+0x2c),item_or_revision_or_proposed)!=0;
    if (!previous) { c.set(3,c.u32(manager_or_link_or_percentage)); previous=compare(c,c.u8(c.r(3)+0x34),3)==0; }
    if (!previous) { c.set(3,c.u32(manager_or_link_or_percentage)); previous=compare(c,c.u8(c.r(3)+0x34),2)==0; }
    if (previous) {
        manager_or_link_or_percentage=c.u32(manager_or_link_or_percentage);
        if (byte_result(c,p.call(0x1073e4b0,0x1042c050,{screens,previous_screen,previous_command_or_person,1,0,0}))) {
            p.history(0x1042c06c,screens,0,manager_or_link_or_percentage);
            p.history(0x1042c080,screens,1,item_or_revision_or_proposed);
        }
        return;
    }
    c.set(3,c.u32(manager_or_link_or_percentage)); c.set(4,c.toc(slot::people));
    previous_command_or_person=c.u32(c.r(4))+c.u32(c.r(3)+12)*0x70;
    enter(c,0x1042c0a4,{previous_command_or_person});
    c.set(3,open_loan_offer_terms(c,previous_command_or_person));
    if (!byte_result(c,c.r(3))) {
        p.call(0x103e44b0,0x1042c0c0,{c.u32(transfers),manager_or_link_or_percentage,0,0}); return;
    }
    c.set(3,c.u32(manager_or_link_or_percentage)); c.set(5,indirect(c,slot::clubs));
    item_or_revision_or_proposed=p.call(0x103ecb80,0x1042c0e8,
        {c.u32(transfers),previous_command_or_person,c.r(5)+c.u32(c.r(3)+8)*0x248});
    if (compare(c,item_or_revision_or_proposed)==0) {
        report_assertion(c,{0x1042c104,pool,pool+0x15,pool+0x22,f.at(0x40),f.at(0x38),0x22b9});
        c.set(3,c.toc(-0x7bac)); return;
    }
    p.history(0x1042c15c,screens,7,c.u32(manager_or_link_or_percentage));
    p.history(0x1042c170,screens,10,item_or_revision_or_proposed);
    const auto current=p.call(0x10082f20,0x1042c180,{indirect(c,slot::contracts),previous_command_or_person});
    manager_or_link_or_percentage=loan_percentage(c,c.u32(item_or_revision_or_proposed+12)*100u,c.u32(current+12));
    p.history(0x1042c1c8,screens,11,manager_or_link_or_percentage);
    auto month=p.call(0x10718360,0x1042c1d0,{item_or_revision_or_proposed+0x30});
    p.history(0x1042c1e8,screens,12,std::uint32_t(std::int32_t(std::int8_t(month))));
    p.history(0x1042c1fc,screens,13,c.u16(item_or_revision_or_proposed+0x32));
    p.history(0x1042c214,screens,14,std::uint32_t(std::int32_t(c.i8(item_or_revision_or_proposed+0x1c))));
    p.history(0x1042c22c,screens,15,std::uint32_t(std::int32_t(c.i8(item_or_revision_or_proposed+0x1d))));
    p.history(0x1042c240,screens,21,manager_or_link_or_percentage);
    month=p.call(0x10718360,0x1042c248,{item_or_revision_or_proposed+0x30});
    p.history(0x1042c260,screens,22,std::uint32_t(std::int32_t(std::int8_t(month))));
    p.history(0x1042c274,screens,23,c.u16(item_or_revision_or_proposed+0x32));
    p.history(0x1042c28c,screens,24,std::uint32_t(std::int32_t(c.i8(item_or_revision_or_proposed+0x1c))));
    p.history(0x1042c2a4,screens,25,std::uint32_t(std::int32_t(c.i8(item_or_revision_or_proposed+0x1d))));
}
}
