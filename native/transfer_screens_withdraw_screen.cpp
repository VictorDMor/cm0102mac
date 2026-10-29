#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_withdraw_bid(Cpu& c) {
    Frame f(c,0x104265c0,0x210,{.first_gpr=25});
    Kept<25> bidder(c,c.r(25));
    Kept<26> person_or_grid(c,c.r(26));
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
    c.put16(f.at(0x54),c.r(5)); c.put16(f.at(0x56),c.u16(c.toc(-0x7b14)));
    c.put16(f.at(0x50),c.u16(c.r(2)-0x37a0));
    const auto offer=c.u32(p.call(0x1073ef70,0x1042660c,{screens,0}));
    c.set({{7,c.u32(offer+12)*0x70},{8,indirect(c,slot::people)},{6,indirect(c,slot::clubs)}});
    person_or_grid=c.r(8)+c.r(7); bidder=c.r(6)+c.u32(offer+8)*0x248;
    p.call(0x107136d0,0x10426648,{person_or_grid,0,0});
    p.call(0x100c7a50,0x1042665c,{bidder,1,f.at(0x56),f.at(0x54)});
    p.call(0x10744250,0x10426670,{screens,c.u16(f.at(0x54)),c.u16(f.at(0x56)),bidder+0x1ee});
    p.call(0x100caeb0,0x10426684,{person_or_grid,f.at(0x58),400,5});
    p.call(0x10727890,0x10426694,{text,pool+0x2829,f.at(0x58)});
    p.call(0x107442f0,0x104266a0,{screens,text});
    p.call(0x100caeb0,0x104266b4,{person_or_grid,f.at(0x58),400,1});
    p.call(0x10727890,0x104266c4,{text,pool+0x2864,f.at(0x58)});
    const auto rows=std::uint32_t(std::int32_t(std::int8_t(divide_signed<4>(405,p.call(0x1076d470,0x104266cc,{3})))));
    stack_args({c.u16(foreground),text,f.at(0x58),0x7d7,person_or_grid,0});
    p.call(0x1074a350,0x10426720,{110,140,780,545,2,0,rows,3});
    p.call(0x102a1070,0x1042672c,{4,0});
    stack_args({0,1,0,missing});
    person_or_grid=p.call(0x107198c0,0x1042677c,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,2,f.at(0x50),1});
    p.call(0x10727890,0x10426790,{text,pool+0x5b2});
    stack_args({c.u16(foreground),text,1,0});
    p.call(0x107444e0,0x104267d0,{screens,person_or_grid,0,0,0x30,c.u16(disabled_colour),12,3});
    p.call(0x10727890,0x104267dc,{text,pool+0x5b5});
    stack_args({c.u16(foreground),text,2,0});
    p.call(0x107444e0,0x1042681c,{screens,person_or_grid,1,0,0x30,c.u16(disabled_colour),12,3});
}
}
