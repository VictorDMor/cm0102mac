#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_temporary_offer_confirmation(Cpu& c) {
    Frame f(c,0x1042efa0,0x1e0,{.first_gpr=22});
    Kept<22> error_or_club_or_rows_or_grid(c,c.toc(-0x7ba8));
    Kept<23> offer(c,c.r(23));
    Kept<24> weeks(c,c.r(24));
    Kept<25> person(c,c.r(25));
    Kept<26> disabled_colour(c,c.toc(-0x7bc4));
    Kept<27> foreground(c,c.toc(-0x7bc8));
    Kept<28> text(c,c.toc(-0x7bb8));
    Kept<29> pool(c,c.toc(slot::pool));
    Kept<30> clubs(c,c.toc(slot::clubs));
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38; for(auto value:values) { c.put32(f.at(offset),value); offset+=4; }
    };
    c.set(5,c.u16(foreground));
    c.put16(f.at(0x54),c.r(5)); c.put16(f.at(0x56),c.u16(c.toc(-0x7b14)));
    c.put16(f.at(0x50),c.u16(c.r(2)-0x3780));
    person=p.call(0x1073ef70,0x1042eff4,{screens,0});
    weeks=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042f008,{screens,1}))));
    offer=p.call(0x1073ef70,0x1042f01c,{screens,7});
    p.call(0x107136d0,0x1042f034,{person,0,0});
    c.set(3,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4));
    if (compare(c,c.r(3))==0) {
        report_assertion(c,{0x1042f068,pool,pool+0x15,pool+0x22,f.at(0x60),f.at(0x58),0x26d4});
        c.set(3,c.toc(-0x7bac)); return;
    }
    error_or_club_or_rows_or_grid=c.u32(c.r(3)+0x24);
    p.call(0x100c7a50,0x1042f0c0,{error_or_club_or_rows_or_grid,1,f.at(0x56),f.at(0x54)});
    p.call(0x10744250,0x1042f0d4,{screens,c.u16(f.at(0x54)),c.u16(f.at(0x56)),error_or_club_or_rows_or_grid+0x1ee});
    p.call(0x100caeb0,0x1042f0e8,{person,f.at(0x80),300,5});
    if (compare(c,std::int8_t(weeks),-1)!=0) p.call(0x10727890,0x1042f104,{text,pool+0x313f,f.at(0x80)});
    else if (compare_logical(c,offer)!=0) p.call(0x10727890,0x1042f120,{text,pool+0x3174,f.at(0x80)});
    else p.call(0x10727890,0x1042f134,{text,pool+0x31b1,f.at(0x80)});
    p.call(0x107442f0,0x1042f140,{screens,text});
    const auto rows=divide_signed<4>(405,p.call(0x1076d470,0x1042f148,{3}));
    const bool trial=compare(c,std::int8_t(weeks),-1)!=0;
    error_or_club_or_rows_or_grid=std::uint32_t(std::int32_t(std::int8_t(rows)));
    if (trial) {
        p.call(0x100caeb0,0x1042f174,{person,f.at(0x80),300,1});
        if (compare(c,std::int8_t(weeks),1)==0) p.call(0x10727890,0x1042f190,{text,pool+0x31ec,f.at(0x80)});
        else p.call(0x10727890,0x1042f1a8,{text,pool+0x322a,std::uint32_t(std::int32_t(std::int8_t(weeks))),f.at(0x80)});
    } else if (compare_logical(c,offer)!=0) {
        // The three club-array loads precede the formatter; no external call
        // occurs between them. Preserve its trailing register arguments too.
        const auto bidder=c.u32(offer+8)*0x248;
        c.set({{10,c.u32(clubs)},{9,c.u32(clubs)}});
        const auto gender=std::uint32_t(std::int32_t(c.i8(c.u32(clubs)+bidder+0x23b)));
        p.call(0x10727890,0x1042f208,{text,pool+0x327b,gender,gender,c.r(9)+bidder+0x221,f.at(0x80)});
    } else p.call(0x10727890,0x1042f21c,{text,pool+0x32de,f.at(0x80)});
    stack_args({c.u16(foreground),text,f.at(0x80),0x7d7,person,0});
    p.call(0x1074a350,0x1042f268,{110,140,780,545,2,0,std::uint32_t(std::int32_t(std::int8_t(error_or_club_or_rows_or_grid))),3});
    p.call(0x102a1070,0x1042f274,{4,0});
    stack_args({0,1,0,missing});
    error_or_club_or_rows_or_grid=p.call(0x107198c0,0x1042f2c4,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,2,f.at(0x50),1});
    p.call(0x10727890,0x1042f2d8,{text,pool+0x5b2});
    stack_args({c.u16(foreground),text,1,0});
    p.call(0x107444e0,0x1042f318,{screens,error_or_club_or_rows_or_grid,0,0,0x30,c.u16(disabled_colour),12,3});
    p.call(0x10727890,0x1042f324,{text,pool+0x5b5});
    stack_args({c.u16(foreground),text,2,0});
    p.call(0x107444e0,0x1042f364,{screens,error_or_club_or_rows_or_grid,1,0,0x30,c.u16(disabled_colour),12,3});
}

std::uint32_t temporary_offer_confirmation_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x1042f380,0xe0,{.first_gpr=21});
    Kept<21> control_or_offer(c,control_value);
    Kept<22> parent_option(c,c.r(22));
    Kept<23> cup_option(c,c.r(23));
    Kept<24> person(c,c.r(24));
    Kept<25> year(c,c.r(25));
    Kept<26> month(c,c.r(26));
    Kept<27> percentage(c,c.r(27));
    Kept<28> weeks(c,c.r(28));
    Kept<29> today(c,c.toc(slot::transfer_context));
    Kept<30> screens_or_manager(c,c.toc(slot::screens));
    Kept<31> transfers(c,c.toc(slot::transfers));
    Program p{c};
    if (compare(c,control_action(c,screens_or_manager,control_or_offer),1)==0) return std::uint32_t(-9);
    if (compare(c,control_action(c,screens_or_manager,control_or_offer),2)==0) {
        person=p.call(0x1073ef70,0x1042f464,{screens_or_manager,0});
        weeks=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042f478,{screens_or_manager,1}))));
        month=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042f48c,{screens_or_manager,2}))));
        year=std::uint16_t(p.call(0x1073ef70,0x1042f4a0,{screens_or_manager,3}));
        percentage=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042f4b4,{screens_or_manager,4}))));
        cup_option=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042f4c8,{screens_or_manager,5}))));
        parent_option=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042f4dc,{screens_or_manager,6}))));
        const auto existing=p.call(0x1073ef70,0x1042f4f0,{screens_or_manager,7});
        c.set(4,std::uint32_t(c.i16(screens_or_manager+0x2c58))*0x2c0);
        const bool trial=compare(c,std::int8_t(weeks),-1)!=0;
        control_or_offer=existing; c.set(3,screens_or_manager+c.r(4)); screens_or_manager=c.u32(c.r(3)+4);
        if (trial) {
            p.call(0x103721f0,0x1042f518,{f.at(0x48)});
            p.call(0x10372260,0x1042f52c,{f.at(0x48),person,c.u32(screens_or_manager+0x24),weeks});
            p.call(0x1007f200,0x1042f554,{indirect(c,slot::contracts),f.at(0x48),missing,missing,11,0,1,0});
            return std::uint32_t(-10);
        }
        if (compare_logical(c,control_or_offer)!=0) {
            auto day=std::int16_t(p.call(0x107182f0,0x1042f56c,{today}));
            if (compare(c,day,28)>0) day=28;
            else day=std::int16_t(p.call(0x107182f0,0x1042f588,{today}));
            p.call(0x107156f0,0x1042f5a4,{f.at(0x40),std::uint32_t(std::int32_t(day)),month,year,missing});
            p.call(0x103e3240,0x1042f5c0,{c.u32(transfers),control_or_offer,f.at(0x40),percentage,cup_option,parent_option});
        } else {
            auto day=std::int16_t(p.call(0x107182f0,0x1042f5d0,{today}));
            if (compare(c,day,28)>0) day=28;
            else day=std::int16_t(p.call(0x107182f0,0x1042f5ec,{today}));
            p.call(0x107156f0,0x1042f608,{f.at(0x38),std::uint32_t(std::int32_t(day)),month,year,missing});
            p.call(0x103e3170,0x1042f628,{c.u32(transfers),person,c.u32(screens_or_manager+0x24),f.at(0x38),percentage,cup_option,parent_option});
        }
        return std::uint32_t(-10);
    }
    const auto command=p.call(0x102a54b0,0x1042f638,{control_or_offer});
    if (compare(c,command)!=0) return command;
    const auto fallback=p.call(0x102a83a0,0x1042f64c,{control_or_offer});
    c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0); return fallback;
}
}
