#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_trial_duration(Cpu& c) {
    Frame f(c,0x1042b580,0x290,{.first_gpr=21});
    Kept<21> club_or_top(c,c.r(21));
    Kept<22> person_or_bottom(c,c.r(22));
    Kept<23> error(c,c.toc(-0x7ba8));
    Kept<24> weeks(c,c.r(24));
    Kept<25> disabled(c,c.toc(-0x7bc4));
    Kept<26> background(c,c.toc(-0x7bdc));
    Kept<27> foreground(c,c.toc(-0x7bc8));
    Kept<28> text(c,c.toc(-0x7bb8));
    Kept<29> assertion_flag_or_grid(c,c.toc(-0x7bac));
    Kept<30> pool(c,c.toc(slot::pool));
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38; for(auto value:values) { c.put32(f.at(offset),value); offset+=4; }
    };
    c.set(5,c.toc(-0x4b68)); c.set(6,c.u16(c.r(5)+4));
    c.put32(f.at(0x68),c.u32(c.r(5))); c.set(5,c.u8(c.r(5)+6));
    c.put16(f.at(0x6c),c.r(6)); c.put8(f.at(0x6e),c.r(5)); c.put16(f.at(0x48),c.u16(c.r(2)-0x378c));
    person_or_bottom=p.call(0x1073ef70,0x1042b5e0,{screens,0});
    const auto duration=p.call(0x1073ef70,0x1042b5f4,{screens,1});
    compare_logical(c,person_or_bottom); weeks=std::uint32_t(std::int32_t(std::int8_t(duration)));
    if (!person_or_bottom) {
        report_assertion(c,{0x1042b614,pool,pool+0x15,pool+0x22,f.at(0xb0),f.at(0x60),0x218a}); return;
    }
    c.set(3,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4));
    if (compare(c,c.r(3))==0) {
        report_assertion(c,{0x1042b688,pool,pool+0x15,pool+0x22,f.at(0x90),f.at(0x58),0x2198}); return;
    }
    club_or_top=c.u32(c.r(3)+0x24);
    if (compare_logical(c,club_or_top)==0) {
        report_assertion(c,{0x1042b6e4,pool,pool+0x15,pool+0x22,f.at(0x70),f.at(0x50),0x219f}); return;
    }
    p.call(0x107136d0,0x1042b734,{person_or_bottom,0,0});
    p.call(0x100c7a50,0x1042b748,{club_or_top,1,f.at(0x4e),f.at(0x4c)});
    p.call(0x10744250,0x1042b75c,{screens,c.u16(f.at(0x4c)),c.u16(f.at(0x4e)),club_or_top+0x221});
    p.call(0x100caeb0,0x1042b770,{person_or_bottom,f.at(0xd0),400,5});
    p.call(0x10727890,0x1042b780,{text,pool+0x2f00,f.at(0xd0)});
    p.call(0x107442f0,0x1042b78c,{screens,text});
    club_or_top=140; person_or_bottom=545;
    const auto height=p.call(0x1076d470,0x1042b79c,{3});
    stack_args({0,2,0,missing});
    assertion_flag_or_grid=p.call(0x107198c0,0x1042b7f4,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),110,club_or_top,780,person_or_bottom,7,f.at(0x68),std::uint8_t(divide_signed<4>(405,height))});
    p.call(0x10727890,0x1042b808,{text,pool+0x2f2f});
    stack_args({c.u16(foreground),text,0,0});
    p.call(0x107444e0,0x1042b848,{screens,assertion_flag_or_grid,1,0,1,0,1,3});
    if (compare(c,std::int8_t(weeks),1)==0) {
        stack_args({c.u16(disabled),pool+0x4a9});
        p.call(0x107445a0,0x1042b888,{screens,assertion_flag_or_grid,2,0,0x30,c.u16(background),12,3});
    } else {
        stack_args({c.u16(foreground),pool+0x4a9,1,0});
        p.call(0x107444e0,0x1042b8d0,{screens,assertion_flag_or_grid,2,0,0x30,c.u16(background),12,3});
    }
    p.history(0x1042b8e8,screens,2,std::uint32_t(std::int32_t(std::int16_t(c.r(3)))));
    if (compare(c,std::int8_t(weeks),1)==0) p.call(0x10727890,0x1042b900,{text,pool+0x2f3c});
    else p.call(0x10727890,0x1042b914,{text,pool+0x2f43,std::uint32_t(std::int32_t(std::int8_t(weeks)))});
    stack_args({c.u16(foreground),text});
    p.call(0x107445a0,0x1042b948,{screens,assertion_flag_or_grid,3,0,1,c.u16(c.toc(-0x7bcc)),12,3});
    if (compare(c,std::int8_t(weeks),4)==0) {
        stack_args({c.u16(disabled),pool+0x4ab});
        p.call(0x107445a0,0x1042b988,{screens,assertion_flag_or_grid,4,0,0x30,c.u16(background),12,3});
    } else {
        stack_args({c.u16(foreground),pool+0x4ab,2,0});
        p.call(0x107444e0,0x1042b9d0,{screens,assertion_flag_or_grid,4,0,0x30,c.u16(background),12,3});
    }
    p.call(0x102a1070,0x1042b9dc,{4,0});
    stack_args({0,1,c.u16(f.at(0x4c)),missing});
    assertion_flag_or_grid=p.call(0x107198c0,0x1042ba2c,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,2,f.at(0x48),1});
    p.call(0x10727890,0x1042ba40,{text,pool+0x5ed});
    stack_args({c.u16(foreground),text,3,0});
    p.call(0x107444e0,0x1042ba80,{screens,assertion_flag_or_grid,0,0,0x30,c.u16(disabled),12,3});
    p.call(0x10727890,0x1042ba8c,{text,pool+0x602});
    stack_args({c.u16(foreground),text,4,0});
    p.call(0x107444e0,0x1042bacc,{screens,assertion_flag_or_grid,1,0,0x30,c.u16(disabled),12,3});
}

std::uint32_t trial_duration_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x1042bae0,0x60,{.first_gpr=25});
    Kept<25> duration_or_control(c,c.r(25));
    Kept<26> disabled_or_person(c,c.toc(-0x7bc4));
    Kept<27> foreground(c,c.toc(-0x7bc8));
    Kept<28> pool(c,c.toc(slot::pool));
    Kept<29> text(c,c.toc(-0x7bb8));
    Kept<30> control_or_duration(c,control_value);
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    if (compare(c,control_action(c,screens,control_or_duration),3)==0) return std::uint32_t(-9);
    if (compare(c,control_action(c,screens,control_or_duration),4)==0) {
        disabled_or_person=p.call(0x1073ef70,0x1042bbcc,{screens,0});
        const auto duration=p.call(0x1073ef70,0x1042bbe0,{screens,1});
        enter(c,0x1042bbf0,{disabled_or_person,std::uint32_t(std::int32_t(std::int8_t(duration)))});
        open_temporary_offer_confirmation(c,disabled_or_person,std::uint32_t(std::int32_t(std::int8_t(duration))));
        return std::uint32_t(-4);
    }
    if (compare(c,control_action(c,screens,control_or_duration),1)==0) {
        const auto duration=p.call(0x1073ef70,0x1042bc5c,{screens,1});
        control_or_duration=std::uint32_t(std::int32_t(std::int8_t(std::int8_t(duration)-1)));
        p.history(0x1042bc80,screens,1,control_or_duration);
        duration_or_control=std::uint32_t(std::int32_t(std::int16_t(p.call(0x1073ef70,0x1042bc8c,{screens,2}))));
        const bool minimum=compare(c,std::int8_t(control_or_duration),1)==0;
        p.call(0x10740620,minimum ? 0x1042bcac : 0x1042bce0,{screens,duration_or_control,minimum ? 6u : 2u});
        p.call(0x10741a80,minimum ? 0x1042bcbc : 0x1042bcf0,{screens,duration_or_control,c.u16(minimum ? std::uint32_t(disabled_or_person) : std::uint32_t(foreground))});
        p.call(0x10742100,minimum ? 0x1042bccc : 0x1042bd00,{screens,duration_or_control,minimum ? 0u : 1u});
        const bool singular=compare(c,std::int8_t(control_or_duration),1)==0;
        duration_or_control=duration_or_control+1;
        if(singular) p.call(0x10727890,0x1042bd1c,{text,pool+0x2f3c});
        else p.call(0x10727890,0x1042bd30,{text,pool+0x2f43,std::uint32_t(std::int32_t(std::int8_t(control_or_duration)))});
        p.call(0x10741da0,0x1042bd40,{screens,duration_or_control,text});
        duration_or_control=duration_or_control+1;
        p.call(0x10740620,0x1042bd54,{screens,duration_or_control,2});
        p.call(0x10741a80,0x1042bd64,{screens,duration_or_control,c.u16(foreground)});
        p.call(0x10742100,0x1042bd74,{screens,duration_or_control,2});
        p.call(0x10742ec0,0x1042bd84,{screens,0,0}); return 0;
    }
    if (compare(c,control_action(c,screens,control_or_duration),2)==0) {
        const auto duration=p.call(0x1073ef70,0x1042bdf0,{screens,1});
        duration_or_control=std::uint32_t(std::int32_t(std::int8_t(std::int8_t(duration)+1)));
        p.history(0x1042be14,screens,1,duration_or_control);
        control_or_duration=std::uint32_t(std::int32_t(std::int16_t(p.call(0x1073ef70,0x1042be20,{screens,2}))));
        p.call(0x10740620,0x1042be38,{screens,control_or_duration,2});
        p.call(0x10741a80,0x1042be48,{screens,control_or_duration,c.u16(foreground)});
        p.call(0x10742100,0x1042be58,{screens,control_or_duration,1});
        const bool singular=compare(c,std::int8_t(duration_or_control),1)==0;
        control_or_duration=control_or_duration+1;
        if(singular) p.call(0x10727890,0x1042be74,{text,pool+0x2f3c});
        else p.call(0x10727890,0x1042be88,{text,pool+0x2f43,std::uint32_t(std::int32_t(std::int8_t(duration_or_control)))});
        p.call(0x10741da0,0x1042be98,{screens,control_or_duration,text});
        control_or_duration=control_or_duration+1;
        const bool maximum=compare(c,std::int8_t(duration_or_control),4)==0;
        p.call(0x10740620,maximum ? 0x1042beb8 : 0x1042beec,{screens,control_or_duration,maximum ? 6u : 2u});
        p.call(0x10741a80,maximum ? 0x1042bec8 : 0x1042befc,{screens,control_or_duration,c.u16(maximum ? std::uint32_t(disabled_or_person) : std::uint32_t(foreground))});
        p.call(0x10742100,maximum ? 0x1042bed8 : 0x1042bf0c,{screens,control_or_duration,maximum ? 0u : 2u});
        p.call(0x10742ec0,0x1042bf1c,{screens,0,0}); return 0;
    }
    const auto command=p.call(0x102a54b0,0x1042bf2c,{control_or_duration});
    if(compare(c,command)!=0) return command;
    const auto fallback=p.call(0x102a83a0,0x1042bf40,{control_or_duration});
    c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0); return fallback;
}
}
