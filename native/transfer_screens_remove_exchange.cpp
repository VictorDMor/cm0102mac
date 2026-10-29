#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_remove_exchange_player(Cpu& c) {
    Frame f(c,0x10422b50,0x480,{.first_gpr=23});
    Kept<23> manager_club(c,c.r(23));
    Kept<24> people_or_person_or_grid(c,c.toc(slot::people));
    Kept<25> exchange_person(c,c.r(25));
    Kept<26> offer(c,c.r(26));
    Kept<27> disabled_colour(c,c.toc(-0x7bc4));
    Kept<28> foreground(c,c.toc(-0x7bc8));
    Kept<29> text(c,c.toc(-0x7bb8));
    Kept<30> pool(c,c.toc(slot::pool));
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38; for(auto value:values) { c.put32(f.at(offset),value); offset+=4; }
    };
    c.set(5,c.u16(foreground));
    c.put16(f.at(0x64),c.r(5)); c.put16(f.at(0x66),c.u16(c.toc(-0x7b14)));
    c.put16(f.at(0x60),c.u16(c.r(2)-0x37b4));
    c.set(4,p.call(0x1073ef70,0x10422ba0,{screens,0})); offer=c.u32(c.r(4));
    const auto selected=p.call(0x1073ef70,0x10422bb4,{screens,1});
    c.set({{8,selected*0x70},{7,c.u32(people_or_person_or_grid)},{6,c.u32(offer+12)*0x70}});
    exchange_person=c.u32(people_or_person_or_grid)+c.r(8);
    people_or_person_or_grid=c.r(7)+c.r(6);
    const auto manager=c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4);
    manager_club=c.u32(manager+0x24);
    p.call(0x107136d0,0x10422bf4,{people_or_person_or_grid,0,0});
    p.call(0x100c7a50,0x10422c08,{manager_club,1,f.at(0x66),f.at(0x64)});
    p.call(0x10744250,0x10422c1c,{screens,c.u16(f.at(0x64)),c.u16(f.at(0x66)),manager_club+0x1ee});
    p.call(0x100caeb0,0x10422c30,{exchange_person,f.at(0x2c4),400,5});
    p.call(0x10727890,0x10422c40,{text,pool+0x170a,f.at(0x2c4)});
    p.call(0x107442f0,0x10422c4c,{screens,text});
    p.call(0x100caeb0,0x10422c60,{exchange_person,f.at(0x2c4),400,1});
    p.call(0x100caeb0,0x10422c74,{people_or_person_or_grid,f.at(0x134),400,1});
    c.set(5,c.u32(exchange_person+8)); c.put32(f.at(0x68),c.r(5)+c.u32(offer+0x20));
    p.call(0x10009f20,0x10422c90,{f.at(0x68),6});
    p.call(0x1000b410,0x10422ca8,{f.at(0x6c),200,c.r(3),c.r(4),6});
    p.call(0x10727890,0x10422cc0,{text,pool+0x1747,f.at(0x2c4),f.at(0x134),f.at(0x6c)});
    const auto rows=std::uint32_t(std::int32_t(std::int8_t(divide_signed<4>(405,p.call(0x1076d470,0x10422cc8,{3})))));
    stack_args({c.u16(foreground),text,f.at(0x2c4),0x7d7,exchange_person,f.at(0x134),0x7d7,people_or_person_or_grid,0});
    p.call(0x1074a350,0x10422d30,{110,140,780,545,2,0,rows,3});
    p.call(0x102a1070,0x10422d3c,{4,0});
    stack_args({0,1,0,missing});
    people_or_person_or_grid=p.call(0x107198c0,0x10422d8c,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,2,f.at(0x60),1});
    p.call(0x10727890,0x10422da0,{text,pool+0x5b2});
    stack_args({c.u16(foreground),text,1,0});
    p.call(0x107444e0,0x10422de0,{screens,people_or_person_or_grid,0,0,0x30,c.u16(disabled_colour),12,3});
    p.call(0x10727890,0x10422dec,{text,pool+0x5b5});
    stack_args({c.u16(foreground),text,2,0});
    p.call(0x107444e0,0x10422e2c,{screens,people_or_person_or_grid,1,0,0x30,c.u16(disabled_colour),12,3});
}

std::uint32_t remove_exchange_player_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x10422e40,0x50,{.first_gpr=26});
    Kept<26> person_id(c,c.r(26));
    Kept<27> bidder(c,c.r(27));
    Kept<28> control_or_slot(c,control_value);
    Kept<29> people(c,c.toc(slot::people));
    Kept<30> transfers(c,c.toc(slot::transfers));
    Kept<31> screens_or_offer(c,c.toc(slot::screens));
    Program p{c};
    if (compare(c,control_action(c,screens_or_offer,control_or_slot),1)==0) return std::uint32_t(-9);
    if (compare(c,control_action(c,screens_or_offer,control_or_slot),2)==0) {
        control_or_slot=p.call(0x1073ef70,0x10422f24,{screens_or_offer,0});
        const auto screens=std::uint32_t(screens_or_offer);
        screens_or_offer=c.u32(control_or_slot); c.set(5,indirect(c,slot::clubs));
        bidder=c.r(5)+c.u32(screens_or_offer+8)*0x248;
        person_id=p.call(0x1073ef70,0x10422f4c,{screens,1});
        c.set({{4,c.u32(people)},{5,c.u32(screens_or_offer+0x20)}});
        c.set(3,c.r(4)+std::uint32_t(person_id)*0x70);
        // The fee is restored even when the exchange ID is absent from the list.
        c.put32(screens_or_offer+0x20,c.r(5)+c.u32(c.r(3)+8));
        c.set(3,c.u32(screens_or_offer+4)); c.put32(c.r(3)+12,c.u32(c.r(3)+4));
        for (;;) {
            const auto item=p.call(0x10738e00,0x10422fe8,{c.u32(screens_or_offer+4)});
            if (compare_logical(c,item)==0) return std::uint32_t(-10);
            if (compare(c,c.u32(item),person_id)!=0) continue;
            p.call(0x103e95a0,0x10422fa8,{c.u32(transfers),c.u32(people)+std::uint32_t(person_id)*0x70,screens_or_offer,1,0});
            const auto controlled=p.call(0x1014c2e0,0x10422fc0,{bidder,0,1,0,0});
            if (!byte_result(c,controlled)) p.call(0x103e4d70,0x10422fd8,{c.u32(transfers),control_or_slot,1});
            return std::uint32_t(-10);
        }
    }
    const auto command=p.call(0x102a54b0,0x10423000,{control_or_slot});
    if (compare(c,command)!=0) return command;
    const auto fallback=p.call(0x102a83a0,0x10423014,{control_or_slot});
    c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0); return fallback;
}
}
