#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_previous_offer(Cpu& c) {
    Frame f(c,0x10422600,0x1e0,{.first_gpr=20});
    Kept<20> bidder(c,c.r(20));
    Kept<21> manager_or_rows(c,c.r(21));
    Kept<22> person(c,c.r(22));
    Kept<23> revision(c,c.r(23));
    Kept<24> offer_or_caption(c,c.r(24));
    Kept<25> foreground(c,c.toc(-0x7bc8));
    Kept<26> text(c,c.toc(-0x7bb8));
    Kept<27> pool(c,c.toc(slot::pool));
    Kept<28> current_contract_or_owner(c,c.toc(slot::current_contract));
    Kept<29> clubs(c,c.toc(slot::clubs));
    Kept<30> screens(c,c.toc(slot::screens));
    Kept<31> valid_contract(c,c.r(31));
    Program p{c};
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38; for (auto value:values) { c.put32(f.at(offset),value); offset+=4; }
    };
    c.put16(f.at(0x68),c.u16(foreground)); c.set(5,c.u16(c.toc(-0x7b14)));
    c.put16(f.at(0x6a),c.r(5));
    p.call(0x1074b6e0,0x10422648,{f.at(0x70),0});
    c.put32(f.at(0x6c),0);
    offer_or_caption=p.call(0x1073ef70,0x1042265c,{screens,0});
    revision=p.call(0x1073ef70,0x10422670,{screens,1});
    c.set(6,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4));
    manager_or_rows=c.u32(c.r(6)+0x24);
    person=indirect(c,slot::people)+c.u32(offer_or_caption+12)*0x70;
    p.call(0x107136d0,0x104226ac,{person,0,0});
    valid_contract=0; c.set(5,c.u32(clubs));
    bidder=c.r(5)+c.u32(offer_or_caption+8)*0x248;
    const auto contract=p.call(0x10082f20,0x104226d0,{indirect(c,slot::contracts),person});
    compare_logical(c,contract); c.put32(current_contract_or_owner,contract);
    if (contract) {
        c.set(3,c.u32(current_contract_or_owner));
        if (compare(c,c.i8(c.r(3)+0x38)>>2,5)!=0) valid_contract=1;
    }
    if (compare(c,valid_contract)!=0) {
        c.set({{3,c.u32(current_contract_or_owner)},{4,c.u32(clubs)}});
        current_contract_or_owner=c.r(4)+c.u32(c.r(3)+4)*0x248;
    } else current_contract_or_owner=0;
    p.call(0x100c7a50,0x10422738,{manager_or_rows,1,f.at(0x6a),f.at(0x68)});
    p.call(0x10744250,0x1042274c,{screens,c.u16(f.at(0x68)),c.u16(f.at(0x6a)),manager_or_rows+0x1ee});
    p.call(0x100caeb0,0x10422760,{person,f.at(0x78),300,1});
    p.call(0x10727890,0x10422770,{text,pool+0x1483,f.at(0x78)});
    p.call(0x1074b830,0x1042277c,{f.at(0x70),text});
    p.call(0x1074bab0,0x10422790,{f.at(0x70),f.at(0x78),0x7d7,person});
    stack_args({0,2,0,missing});
    const auto grid=p.call(0x107198c0,0x104227e0,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,80,790,125,1,0,1});
    stack_args({0});
    p.call(0x1074bcd0,0x10422810,{f.at(0x70),grid,100,790,f.at(0x6c),6,c.u16(c.toc(-0x7bd0)),1});
    c.set(3,divide_signed<4>(405,p.call(0x1076d470,0x10422818,{3})));
    const bool changed=compare(c,c.u32(offer_or_caption+0x2c),revision)!=0;
    manager_or_rows=std::uint32_t(std::int32_t(std::int8_t(c.r(3))));
    const auto format=[&](std::uint32_t club,unsigned message,std::uint32_t back) {
        const auto gender=std::uint32_t(std::int32_t(c.i8(club+0x23b)));
        p.call(0x10727890,back,{text,pool+message,gender,gender,club+0x221,f.at(0x78)});
    };
    if (changed) format(bidder,0x1558,0x10422854);
    else if (compare(c,c.u8(offer_or_caption+0x34),2)==0) format(current_contract_or_owner,0x15c4,0x10422888);
    else if (compare(c,c.u8(offer_or_caption+0x34),5)==0 || compare(c,c.u8(offer_or_caption+0x34),6)==0)
        format(current_contract_or_owner,0x162d,0x104228c8);
    else if (compare(c,c.u8(offer_or_caption+0x34),3)==0) format(bidder,0x1697,0x104228fc);
    offer_or_caption=bidder+0x221; revision=0x7d5;
    stack_args({c.u16(foreground),text,current_contract_or_owner+0x221,0x7d5,current_contract_or_owner,
        offer_or_caption,revision,bidder,f.at(0x78),0x7d7,person,0});
    p.call(0x1074a350,0x10422970,{110,140,780,545,2,0,
        std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(manager_or_rows)))),3});
    p.call(0x102a1070,0x1042297c,{4,0});
    p.call(0x10727890,0x10422988,{text,pool+0x1705});
    stack_args({3,c.u16(foreground),text,3,0});
    p.call(0x10744430,0x104229d4,{screens,100,555,790,590,0x30,c.u16(c.toc(-0x7bc4)),12});
    p.call(0x1074b7c0,0x104229e0,{f.at(0x70),missing});
}
}
