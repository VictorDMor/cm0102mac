#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_transfer_listing_confirmation(Cpu& c) {
    Frame f(c,0x1042a810,0x220,{.first_gpr=21});
    Kept<21> listed_or_grid(c,c.r(21));
    Kept<22> loan(c,c.r(22));
    Kept<23> contracts(c,c.toc(slot::contracts));
    Kept<24> current_contract(c,c.toc(slot::current_contract));
    Kept<25> clubs(c,c.toc(slot::clubs));
    Kept<26> screens(c,c.toc(slot::screens));
    Kept<27> person(c,c.r(27));
    Kept<28> disabled_colour(c,c.toc(-0x7bc4));
    Kept<29> foreground(c,c.toc(-0x7bc8));
    Kept<30> text(c,c.toc(-0x7bb8));
    Kept<31> pool(c,c.toc(slot::pool));
    Program p{c};
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38; for(auto value:values) { c.put32(f.at(offset),value); offset+=4; }
    };
    c.set(5,c.u16(foreground));
    c.put16(f.at(0x54),c.r(5)); c.put16(f.at(0x56),c.u16(c.toc(-0x7b14)));
    c.put16(f.at(0x50),c.u16(c.r(2)-0x3794));
    person=p.call(0x1073ef70,0x1042a868,{screens,0});
    loan=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042a87c,{screens,1}))));
    if (compare(c,loan)!=0) {
        c.set(3,std::uint32_t(std::int32_t(std::int8_t(p.call(0x100d3230,0x1042a890,{person,0})))));
    } else {
        c.set(3,std::uint32_t(std::int32_t(std::int8_t(p.call(0x100d3040,0x1042a8b4,{person,0})))));
    }
    listed_or_grid=c.r(3)!=0;
    p.call(0x107136d0,0x1042a8d8,{person,0,0});
    const auto owner=[&](std::uint32_t back,unsigned destination) {
        const auto record=p.call(0x10082f20,back,{c.u32(contracts),person});
        compare_logical(c,record); c.put32(current_contract,record);
        if (record) { c.set({{3,c.u32(current_contract)},{4,c.u32(clubs)}}); c.set(destination,c.r(4)+c.u32(c.r(3)+4)*0x248); }
        else c.set(destination,0);
    };
    owner(0x1042a8e4,3);
    p.call(0x100c7a50,0x1042a91c,{c.r(3),1,f.at(0x56),f.at(0x54)});
    owner(0x1042a928,6);
    p.call(0x10744250,0x1042a964,{screens,c.u16(f.at(0x54)),c.u16(f.at(0x56)),c.r(6)+0x1ee});
    p.call(0x100caeb0,0x1042a978,{person,f.at(0x58),400,5});
    if (compare(c,std::int8_t(listed_or_grid))!=0)
        p.call(0x10727890,0x1042a990,{text,pool+0x2ba6,f.at(0x58)});
    else if (compare(c,std::int8_t(loan))!=0)
        p.call(0x10727890,0x1042a9ac,{text,pool+0x2bdc,f.at(0x58)});
    else p.call(0x10727890,0x1042a9c0,{text,pool+0x2c18,f.at(0x58)});
    p.call(0x107442f0,0x1042a9cc,{screens,text});
    p.call(0x100caeb0,0x1042a9e0,{person,f.at(0x58),400,1});
    if (compare(c,std::int8_t(listed_or_grid))!=0) {
        if (compare(c,std::int8_t(loan))!=0) p.call(0x10727890,0x1042aa00,{text,pool+0x2c4b,f.at(0x58)});
        else p.call(0x10727890,0x1042aa14,{text,pool+0x2ca4,f.at(0x58)});
    } else {
        if (compare(c,std::int8_t(loan))!=0) p.call(0x10727890,0x1042aa30,{text,pool+0x2d01,f.at(0x58)});
        else p.call(0x10727890,0x1042aa44,{text,pool+0x2d5d,f.at(0x58)});
    }
    const auto rows=std::uint32_t(std::int32_t(std::int8_t(divide_signed<4>(405,p.call(0x1076d470,0x1042aa4c,{3})))));
    stack_args({c.u16(foreground),text,f.at(0x58),0x7d7,person,0});
    p.call(0x1074a350,0x1042aaa0,{110,140,780,545,2,0,rows,3});
    p.call(0x102a1070,0x1042aaac,{4,0});
    stack_args({0,1,0,missing});
    listed_or_grid=p.call(0x107198c0,0x1042aafc,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,2,f.at(0x50),1});
    p.call(0x10727890,0x1042ab10,{text,pool+0x5b2});
    stack_args({c.u16(foreground),text,1,0});
    p.call(0x107444e0,0x1042ab50,{screens,listed_or_grid,0,0,0x30,c.u16(disabled_colour),12,3});
    p.call(0x10727890,0x1042ab5c,{text,pool+0x5b5});
    stack_args({c.u16(foreground),text,2,0});
    p.call(0x107444e0,0x1042ab9c,{screens,listed_or_grid,1,0,0x30,c.u16(disabled_colour),12,3});
}

std::uint32_t transfer_listing_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x1042abb0,0x50,{.first_gpr=29});
    Kept<29> control_or_contract(c,control_value);
    Kept<30> transfers(c,c.toc(slot::transfers));
    Kept<31> screens_or_person(c,c.toc(slot::screens));
    Program p{c};
    if (compare(c,control_action(c,screens_or_person,control_or_contract),1)==0) return std::uint32_t(-10);
    if (compare(c,control_action(c,screens_or_person,control_or_contract),2)==0) {
        const auto person=p.call(0x1073ef70,0x1042ac98,{screens_or_person,0});
        const auto screens=std::uint32_t(screens_or_person);
        screens_or_person=person;
        const auto loan=p.call(0x1073ef70,0x1042acac,{screens,1});
        if (byte_result(c,loan)) {
            if (byte_result(c,p.call(0x100d3230,0x1042acc0,{screens_or_person,0})))
                p.call(0x10365cc0,0x1042acd8,{c.u32(c.u32(transfers)+0x988),screens_or_person});
            else p.call(0x10365c80,0x1042acec,{c.u32(c.u32(transfers)+0x988),screens_or_person});
        } else if (byte_result(c,p.call(0x100d3040,0x1042acfc,{screens_or_person,0}))) {
            p.call(0x10365c30,0x1042ad14,{c.u32(c.u32(transfers)+0x988),screens_or_person});
            control_or_contract=p.call(0x10082f90,0x1042ad28,{indirect(c,slot::contracts),screens_or_person,c.u32(screens_or_person+0x24)});
            if (compare_logical(c,c.u32(control_or_contract+0x50)&0x10000u)==0) {
                if (byte_result(c,p.call(0x1037f110,0x1042ad40,{control_or_contract})) &&
                    compare(c,c.i8(screens_or_person+0x65),15)<0) {
                    c.set(3,std::uint32_t(std::int32_t(c.i8(control_or_contract+0x4c))));
                    if (compare(c,c.r(3)-50u,100)>0) c.put8(control_or_contract+0x4c,100);
                    else {
                        c.set(3,std::uint32_t(std::int32_t(c.i8(control_or_contract+0x4c))));
                        if (compare(c,c.r(3)-50u,-100)<0) c.put8(control_or_contract+0x4c,std::uint8_t(-100));
                        else { c.set(3,c.u8(control_or_contract+0x4c)); c.put8(control_or_contract+0x4c,c.r(3)-50u); }
                    }
                    p.call(0x1037fa80,0x1042adb8,{control_or_contract,0x10000,0});
                }
            }
        } else p.call(0x10363130,0x1042add0,{c.u32(c.u32(transfers)+0x988),screens_or_person,1});
        return std::uint32_t(-10);
    }
    const auto command=p.call(0x102a54b0,0x1042ade0,{control_or_contract});
    if (compare(c,command)!=0) return command;
    const auto fallback=p.call(0x102a83a0,0x1042adf4,{control_or_contract});
    c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0); return fallback;
}
}
