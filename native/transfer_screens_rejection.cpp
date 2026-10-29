#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_bid_rejection(Cpu& c) {
    Frame f(c,0x1041f8f0,0x200,{.first_gpr=19});
    Kept<19> selected_colour(c,c.toc(-0x7bd0));
    Kept<20> link_or_person(c,c.r(20));
    Kept<21> disabled(c,c.toc(-0x7bc4));
    Kept<22> pool(c,c.toc(slot::pool));
    Kept<23> all_eligible(c,c.r(23));
    Kept<24> foreground(c,c.toc(-0x7bc8));
    Kept<25> pending_count(c,c.r(25));
    Kept<26> transfers(c,c.toc(slot::transfers));
    Kept<27> reason(c,c.r(27));
    Kept<28> screens(c,c.toc(slot::screens));
    Kept<29> text(c,c.toc(-0x7bb8));
    Kept<30> offer(c,c.r(30));
    Kept<31> error_or_club_or_grid(c,c.toc(-0x7ba8));
    Program p{c};
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38; for(auto value:values) { c.put32(f.at(offset),value); offset+=4; }
    };
    c.set({{4,c.u16(c.r(2)-0x37bc)},{5,c.u16(c.r(2)-0x37c0)},{6,c.u16(c.r(2)-0x37c4)},
        {7,c.u16(foreground)},{8,c.u16(c.toc(-0x7b14))}});
    c.put16(f.at(0x52),c.r(8)); c.put16(f.at(0x50),c.r(7));
    c.put16(f.at(0x4c),c.r(6)); c.put16(f.at(0x48),c.r(5));
    c.put16(f.at(0x58),c.r(4)); c.put8(f.at(0x5a),c.u8(c.r(2)-0x37ba));
    p.call(0x10738c00,0x1041f95c,{f.at(0x6c)});
    c.put32(f.at(0x54),0); pending_count=0; all_eligible=1;
    link_or_person=p.call(0x1073ef70,0x1041f978,{screens,0});
    const auto selected=p.call(0x1073ef70,0x1041f98c,{screens,2});
    compare_logical(c,link_or_person); reason=std::uint32_t(std::int32_t(std::int16_t(selected)));
    if(!link_or_person) link_or_person=p.call(0x1073ef70,0x1041f9a4,{screens,1});
    offer=c.u32(link_or_person); link_or_person=indirect(c,slot::people)+c.u32(offer+12)*0x70;
    p.call(0x107136d0,0x1041f9d0,{link_or_person,0,0});
    c.set(3,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4));
    if(compare(c,c.r(3))==0) {
        report_assertion(c,{0x1041fa04,pool,pool+0x15,pool+0x22,f.at(0x7c),f.at(0x64),0xee9});
        c.set(5,c.toc(-0x7bac)); p.call(0x10738c20,0x1041fa50,{f.at(0x6c),missing}); return;
    }
    error_or_club_or_grid=c.u32(c.r(3)+0x24);
    p.call(0x100c7a50,0x1041fa68,{error_or_club_or_grid,1,f.at(0x52),f.at(0x50)});
    p.call(0x10744250,0x1041fa7c,{screens,c.u16(f.at(0x50)),c.u16(f.at(0x52)),error_or_club_or_grid+0x1ee});
    p.call(0x100caeb0,0x1041fa90,{link_or_person,f.at(0x9c),300,1});
    p.call(0x10727890,0x1041faa0,{text,pool+0x626,f.at(0x9c)});
    p.call(0x107442f0,0x1041faac,{screens,text});
    const auto rows=std::uint8_t(divide_signed<4>(405,p.call(0x1076d470,0x1041fab4,{3})));
    stack_args({0,2,0,missing});
    error_or_club_or_grid=p.call(0x107198c0,0x1041fb0c,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),110,140,780,545,2,f.at(0x4c),rows});
    p.call(0x10727890,0x1041fb24,{text,pool+0xa88,f.at(0x9c)});
    p.call(0x1074b6e0,0x1041fb30,{f.at(0x5c),text});
    stack_args({0});
    p.call(0x1074bcd0,0x1041fb5c,{f.at(0x5c),error_or_club_or_grid,125,775,f.at(0x54),3,c.u16(foreground),0});
    c.set(5,c.u32(f.at(0x54))); c.put32(f.at(0x54),c.r(5)+2);
    p.call(0x10727890,0x1041fb74,{text,pool+0xac6});
    p.call(0x1074c570,0x1041fb7c,{f.at(0x5c)});
    p.call(0x1074b830,0x1041fb88,{f.at(0x5c),text});
    stack_args({0});
    p.call(0x1074bcd0,0x1041fbb4,{f.at(0x5c),error_or_club_or_grid,125,775,f.at(0x54),3,c.u16(foreground),0});
    c.put32(f.at(0x54),c.u32(f.at(0x54))+2);
    const auto choice=[&](std::uint32_t value,std::uint32_t format_back,std::uint32_t label_back,std::uint32_t button_back) {
        p.call(0x103ea600,format_back,{c.u32(transfers),value,text,2000});
        const bool selected=compare(c,std::int16_t(reason),std::int32_t(value))==0;
        if(selected) stack_args({c.u16(selected_colour),text});
        else stack_args({c.u16(foreground),text,1,value});
        const auto row=c.u32(f.at(0x54)); c.put32(f.at(0x54),row+1);
        p.call(selected ? 0x107445a0 : 0x107444e0,selected ? label_back : button_back,
            {screens,error_or_club_or_grid,1,row,1,0,1,3});
    };
    choice(0,0x1041fbd4,0x1041fc14,0x1041fc60);
    choice(0x200,0x1041fc74,0x1041fcb8,0x1041fd04);
    choice(0x400,0x1041fd18,0x1041fd5c,0x1041fda8);
    struct RestrictedReason { std::uint32_t value,check,format,label,button,disabled_format,disabled_label; };
    constexpr RestrictedReason options[]{
        {0x2000,0x1041fdb8,0x1041fdd4,0x1041fe18,0x1041fe64,0x1041fe7c,0x1041feb4},
        {0x1000,0x1041fec4,0x1041fee0,0x1041ff24,0x1041ff70,0x1041ff88,0x1041ffc0},
        {10,0x1041ffd0,0x1041ffec,0x10420030,0x1042007c,0x10420094,0x104200cc},
        {0x800,0x104200dc,0x104200f8,0x1042013c,0x10420188,0x104201a0,0x104201d8}};
    for(const auto& option:options) {
        if(byte_result(c,p.call(0x103ea440,option.check,{c.u32(transfers),option.value,offer})))
            choice(option.value,option.format,option.label,option.button);
        else {
            p.call(0x103ea600,option.disabled_format,{c.u32(transfers),option.value,text,2000});
            stack_args({c.u16(disabled),text});
            const auto row=c.u32(f.at(0x54)); c.put32(f.at(0x54),row+1);
            p.call(0x107445a0,option.disabled_label,{screens,error_or_club_or_grid,1,row,1,0,1,3});
        }
    }
    p.call(0x102a1070,0x104201e4,{4,0});
    p.call(0x103eaea0,0x104201f8,{c.u32(transfers),link_or_person,f.at(0x6c),0});
    c.put32(f.at(0x78),c.u32(f.at(0x70)));
    for(;;) {
        const auto node=p.call(0x10738e00,0x10420248,{f.at(0x6c)}); c.set(4,node);
        if(compare(c,node)==0) break;
        const auto link=p.call(0x103eb440,0x10420210,{c.u32(transfers),c.u32(node)});
        offer=c.u32(link);
        if(compare(c,c.i8(offer+0x34))==0) {
            pending_count=pending_count+1;
            if(!byte_result(c,p.call(0x103ea440,0x10420234,{c.u32(transfers),reason,offer}))) all_eligible=0;
        }
    }
    const bool multiple=compare(c,pending_count,1)>0;
    stack_args({0,1,0,missing});
    error_or_club_or_grid=p.call(0x107198c0,multiple ? 0x104202a8 : 0x10420424,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,multiple ? 3u : 2u,f.at(multiple ? 0x58 : 0x48),1});
    p.call(0x10727890,multiple ? 0x104202bc : 0x10420438,{text,pool+0x5b2});
    stack_args({c.u16(foreground),text,3,0});
    p.call(0x107444e0,multiple ? 0x104202fc : 0x10420478,{screens,error_or_club_or_grid,0,0,0x30,c.u16(disabled),12,3});
    if(multiple) {
        p.call(0x10727890,0x10420308,{text,pool+0xacf});
        if(byte_result(c,all_eligible)) {
            stack_args({c.u16(foreground),text,4,0});
            p.call(0x107444e0,0x10420350,{screens,error_or_club_or_grid,1,0,0x30,c.u16(disabled),12,3});
        } else {
            stack_args({c.u16(foreground),text});
            p.call(0x107445a0,0x10420384,{screens,error_or_club_or_grid,1,0,0x30,c.u16(disabled),44,3});
        }
    }
    p.call(0x10727890,multiple ? 0x10420390 : 0x10420484,{text,pool+0x5b5});
    stack_args({c.u16(foreground),text,2,0});
    p.call(0x107444e0,multiple ? 0x104203d0 : 0x104204c4,
        {screens,error_or_club_or_grid,multiple ? 2u : 1u,0,0x30,c.u16(disabled),12,3});
    p.call(0x1074b7c0,0x104204d0,{f.at(0x5c),missing});
    p.call(0x10738c20,0x104204dc,{f.at(0x6c),missing});
}
}
