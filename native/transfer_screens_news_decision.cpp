#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_offer_news_decision(Cpu& c) {
    Frame f(c,0x10421fb0,0x1b0,{.first_gpr=21});
    Kept<21> manager_or_rows_or_grid(c,c.r(21));
    Kept<22> person(c,c.r(22));
    Kept<23> offer_or_owner(c,c.r(23));
    Kept<24> disabled_colour(c,c.toc(-0x7bc4));
    Kept<25> foreground(c,c.toc(-0x7bc8));
    Kept<26> text(c,c.toc(-0x7bb8));
    Kept<27> pool(c,c.toc(slot::pool));
    Kept<28> current_contract_or_bidder(c,c.toc(slot::current_contract));
    Kept<29> clubs(c,c.toc(slot::clubs));
    Kept<30> screens(c,c.toc(slot::screens));
    Kept<31> valid_contract(c,c.r(31));
    Program p{c};
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38; for(auto value:values) { c.put32(f.at(offset),value); offset+=4; }
    };
    c.set({{5,screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0},
        {6,c.u16(c.r(2)-0x37b8)},{7,c.u16(foreground)},{8,c.u16(c.toc(-0x7b14))}});
    manager_or_rows_or_grid=c.u32(c.r(5)+4);
    c.put16(f.at(0x48),c.r(7)); c.put16(f.at(0x4a),c.r(8));
    c.put16(f.at(0x4c),c.r(6)); c.put8(f.at(0x4e),c.u8(c.r(2)-0x37b6));
    offer_or_owner=c.u32(p.call(0x1073ef70,0x1042201c,{screens,2}));
    valid_contract=0;
    person=indirect(c,slot::people)+c.u32(offer_or_owner+12)*0x70;
    const auto contract=p.call(0x10082f20,0x10422048,{indirect(c,slot::contracts),person});
    compare_logical(c,contract); c.put32(current_contract_or_bidder,contract);
    if (contract) {
        c.set(3,c.u32(current_contract_or_bidder));
        if (compare(c,c.i8(c.r(3)+0x38)>>2,5)!=0) valid_contract=1;
    }
    if (compare(c,valid_contract)!=0) {
        c.set({{3,c.u32(current_contract_or_bidder)},{4,c.u32(clubs)}});
        c.set(3,c.r(4)+c.u32(c.r(3)+4)*0x248);
    } else c.set(3,0);
    const auto bidder_id=c.u32(offer_or_owner+8);
    offer_or_owner=c.r(3); c.set(6,c.u32(clubs)); current_contract_or_bidder=c.r(6)+bidder_id*0x248;
    p.call(0x107136d0,0x104220bc,{person,0,0});
    p.call(0x100c7a50,0x104220d0,{c.u32(manager_or_rows_or_grid+0x24),1,f.at(0x4a),f.at(0x48)});
    p.call(0x10744250,0x104220e8,{screens,c.u16(f.at(0x48)),c.u16(f.at(0x4a)),c.u32(manager_or_rows_or_grid+0x24)+0x1ee});
    p.call(0x100caeb0,0x104220fc,{person,f.at(0x50),300,1});
    p.call(0x10727890,0x1042210c,{text,pool+0x1483,f.at(0x50)});
    p.call(0x107442f0,0x10422118,{screens,text});
    const auto height=p.call(0x1076d470,0x10422120,{3});
    manager_or_rows_or_grid=std::uint32_t(std::int32_t(std::int8_t(divide_signed<4>(405,height))));
    const auto owner_gender=std::uint32_t(std::int32_t(c.i8(offer_or_owner+0x23b)));
    const auto bidder_gender=std::uint32_t(std::int32_t(c.i8(current_contract_or_bidder+0x23b)));
    stack_args({offer_or_owner+0x1ee,current_contract_or_bidder+0x1ee});
    p.call(0x10727890,0x10422170,{text,pool+0x14b8,100,owner_gender,owner_gender,bidder_gender,bidder_gender,f.at(0x50)});
    stack_args({c.u16(foreground),text,0});
    p.call(0x1074a350,0x104221a8,{110,140,780,545,2,0,
        std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(manager_or_rows_or_grid)))),3});
    p.call(0x102a1070,0x104221b4,{4,0});
    stack_args({0,1,0,missing});
    manager_or_rows_or_grid=p.call(0x107198c0,0x10422204,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,3,f.at(0x4c),1});
    struct Button { unsigned caption,column,action; std::uint32_t format_back,button_back; };
    static constexpr Button buttons[]{
        {0x5ed,0,1,0x10422218,0x10422258},{0x608,1,2,0x10422264,0x104222a4},{0x619,2,3,0x104222b0,0x104222f0}};
    for (const auto& button:buttons) {
        p.call(0x10727890,button.format_back,{text,pool+button.caption});
        stack_args({c.u16(foreground),text,button.action,0});
        p.call(0x107444e0,button.button_back,{screens,manager_or_rows_or_grid,button.column,0,0x30,c.u16(disabled_colour),12,3});
    }
}

std::uint32_t offer_news_decision_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x10422310,0x50,{.first_gpr=26});
    Kept<26> item(c,c.r(26));
    Kept<27> manager(c,c.r(27));
    Kept<28> news(c,c.toc(-0x7bf0));
    Kept<29> transfers(c,c.toc(slot::transfers));
    Kept<30> screens(c,c.toc(slot::screens));
    Kept<31> control(c,control_value);
    Program p{c};
    manager=p.call(0x1073ef70,0x1042233c,{screens,0});
    item=p.call(0x1073ef70,0x10422350,{screens,1});
    c.set(4,std::uint32_t(c.i16(screens+0x2c58))*0x2c0);
    if (compare(c,control_action(c,screens,control),3)==0) {
        p.call(0x102cd0c0,0x104223c8,{c.u32(news),manager,item,7,1,0});
        p.call(0x102cd0c0,0x104223e4,{c.u32(news),manager,item,8,1,0});
        return std::uint32_t(-9);
    }
    if (compare(c,control_action(c,screens,control),2)==0) {
        p.call(0x102cd0c0,0x10422460,{c.u32(news),manager,item,7,1,0});
        p.call(0x102cd0c0,0x1042247c,{c.u32(news),manager,item,8,0,0});
        c.set(4,p.call(0x1073ef70,0x10422488,{screens,2}));
        c.set(3,c.u32(c.r(4)));
        const bool pending=compare(c,c.u8(c.r(3)+0x34),17)==0;
        p.call(pending?0x103e44b0:0x103e4130,pending?0x104224ac:0x104224c0,{c.u32(transfers),c.r(4),0,6});
        return std::uint32_t(-9);
    }
    if (compare(c,control_action(c,screens,control),1)==0) return std::uint32_t(-9);
    const auto command=p.call(0x102a54b0,0x10422530,{control});
    if (compare(c,command)!=0) return command;
    const auto fallback=p.call(0x102a83a0,0x10422544,{control});
    c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0); return fallback;
}
}
