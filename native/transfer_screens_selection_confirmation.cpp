#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_comparison_confirmation(Cpu& c) {
    Frame f(c,0x104249e0,0x210,{.first_gpr=23});
    Kept<23> manager(c,c.r(23));
    Kept<24> club(c,c.r(24));
    Kept<25> first_player(c,c.r(25));
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
    c.put16(f.at(0x50),c.u16(c.r(2)-0x37a8));
    const auto person_id=p.call(0x1073ef70,0x10424a2c,{screens,0});
    c.set({{4,c.toc(slot::people)},{5,person_id*0x70}});
    c.set(3,c.u32(c.r(4))); person_or_grid=c.r(3)+c.r(5);
    manager=c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4);
    club=c.u32(manager+0x24);
    if (compare_logical(c,club)==0) club=c.u32(manager+0x20);
    first_player=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x10424a68,{screens,1}))));
    p.call(0x107136d0,0x10424a80,{person_or_grid,0,0});
    if (compare_logical(c,club)!=0) p.call(0x100c7a50,0x10424a9c,{club,1,f.at(0x56),f.at(0x54)});
    p.call(0x100caeb0,0x10424ab0,{manager,text,2000,1});
    p.call(0x10744250,0x10424ac4,{screens,c.u16(f.at(0x54)),c.u16(f.at(0x56)),text});
    if (compare(c,std::int8_t(first_player))!=0) p.call(0x10727890,0x10424ad8,{text,pool+0x26b7});
    else p.call(0x10727890,0x10424ae8,{text,pool+0x26d8});
    p.call(0x107442f0,0x10424af4,{screens,text});
    p.call(0x100caeb0,0x10424b08,{person_or_grid,f.at(0x58),400,1});
    if (compare(c,std::int8_t(first_player))!=0) p.call(0x10727890,0x10424b20,{text,pool+0x26f9,f.at(0x58)});
    else p.call(0x10727890,0x10424b34,{text,pool+0x273e,f.at(0x58)});
    const auto rows=std::uint32_t(std::int32_t(std::int8_t(divide_signed<4>(405,p.call(0x1076d470,0x10424b3c,{3})))));
    stack_args({c.u16(foreground),text,f.at(0x58),0x7d7,person_or_grid,0});
    p.call(0x1074a350,0x10424b90,{110,140,780,545,2,0,rows,3});
    p.call(0x102a1070,0x10424b9c,{4,0});
    stack_args({0,1,0,missing});
    person_or_grid=p.call(0x107198c0,0x10424bec,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,2,f.at(0x50),1});
    p.call(0x10727890,0x10424c00,{text,pool+0x5b2});
    if (compare(c,std::int8_t(first_player))!=0) {
        stack_args({c.u16(foreground),text,2,0});
        p.call(0x107444e0,0x10424c48,{screens,person_or_grid,0,0,0x30,c.u16(disabled_colour),12,3});
        p.call(0x10727890,0x10424c54,{text,pool+0x5b5});
        stack_args({c.u16(foreground),text,1,0});
        p.call(0x107444e0,0x10424c94,{screens,person_or_grid,1,0,0x30,c.u16(disabled_colour),12,3});
    } else {
        stack_args({c.u16(foreground),text,11,0});
        p.call(0x107444e0,0x10424cd8,{screens,person_or_grid,0,0,0x30,c.u16(disabled_colour),12,3});
        p.call(0x10727890,0x10424ce4,{text,pool+0x5b5});
        stack_args({c.u16(foreground),text,10,0});
        p.call(0x107444e0,0x10424d24,{screens,person_or_grid,1,0,0x30,c.u16(disabled_colour),12,3});
    }
}

std::uint32_t comparison_confirmation_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x10424d40,0x50,{.first_gpr=29});
    Kept<29> shortlists(c,c.toc(-0x7b1c));
    Kept<30> people(c,c.toc(slot::people));
    Kept<31> control(c,control_value);
    c.set(4,c.toc(slot::screens)); Program p{c};
    const auto screens=c.r(4);
    if (compare(c,control_action(c,screens,control),2)==0 || compare(c,control_action(c,screens,control),11)==0)
        return std::uint32_t(-9);
    if (compare(c,control_action(c,screens,control),1)==0) {
        const auto id=p.call(0x1073ef70,0x10424e84,{screens,0});
        p.call(0x1030ffa0,0x10424ea0,{c.u32(shortlists),c.u32(people)+id*0x70,0,1});
        return std::uint32_t(-9);
    }
    if (compare(c,control_action(c,screens,control),10)==0) {
        const auto id=p.call(0x1073ef70,0x10424f0c,{screens,0});
        p.call(0x1030ffa0,0x10424f28,{c.u32(shortlists),c.u32(people)+id*0x70,0,2});
        return std::uint32_t(-9);
    }
    const auto command=p.call(0x102a54b0,0x10424f38,{control});
    if (compare(c,command)!=0) return command;
    const auto fallback=p.call(0x102a83a0,0x10424f4c,{control});
    c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0); return fallback;
}

void render_shortlist_confirmation(Cpu& c) {
    Frame f(c,0x10424f80,0x210,{.first_gpr=23});
    Kept<23> add(c,c.r(23));
    Kept<24> manager(c,c.r(24));
    Kept<25> club(c,c.r(25));
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
    c.put16(f.at(0x50),c.u16(c.r(2)-0x37a4));
    const auto person_id=p.call(0x1073ef70,0x10424fcc,{screens,0});
    c.set({{4,c.toc(slot::people)},{5,person_id*0x70}});
    c.set(3,c.u32(c.r(4))); person_or_grid=c.r(3)+c.r(5);
    manager=c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4);
    club=c.u32(manager+0x24);
    if (compare_logical(c,club)==0) club=c.u32(manager+0x20);
    add=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x10425008,{screens,1}))));
    p.call(0x107136d0,0x10425020,{person_or_grid,0,0});
    if (compare_logical(c,club)!=0) p.call(0x100c7a50,0x1042503c,{club,1,f.at(0x56),f.at(0x54)});
    p.call(0x100caeb0,0x10425050,{manager,text,2000,1});
    p.call(0x10744250,0x10425064,{screens,c.u16(f.at(0x54)),c.u16(f.at(0x56)),text});
    if (compare(c,std::int8_t(add))!=0) p.call(0x10727890,0x10425078,{text,pool+0x2783});
    else p.call(0x10727890,0x10425088,{text,pool+0x279b});
    p.call(0x107442f0,0x10425094,{screens,text});
    p.call(0x100caeb0,0x104250a8,{person_or_grid,f.at(0x58),400,1});
    if (compare(c,std::int8_t(add))!=0) p.call(0x10727890,0x104250c0,{text,pool+0x27b8,f.at(0x58)});
    else p.call(0x10727890,0x104250d4,{text,pool+0x27ee,f.at(0x58)});
    const auto rows=std::uint32_t(std::int32_t(std::int8_t(divide_signed<4>(405,p.call(0x1076d470,0x104250dc,{3})))));
    stack_args({c.u16(foreground),text,f.at(0x58),0x7d7,person_or_grid,0});
    p.call(0x1074a350,0x10425130,{110,140,780,545,2,0,rows,3});
    p.call(0x102a1070,0x1042513c,{4,0});
    stack_args({0,1,0,missing});
    person_or_grid=p.call(0x107198c0,0x1042518c,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,2,f.at(0x50),1});
    p.call(0x10727890,0x104251a0,{text,pool+0x5b2});
    stack_args({c.u16(foreground),text,2,0});
    p.call(0x107444e0,0x104251e0,{screens,person_or_grid,0,0,0x30,c.u16(disabled_colour),12,3});
    p.call(0x10727890,0x104251ec,{text,pool+0x5b5});
    stack_args({c.u16(foreground),text,1,0});
    p.call(0x107444e0,0x1042522c,{screens,person_or_grid,1,0,0x30,c.u16(disabled_colour),12,3});
}
}
