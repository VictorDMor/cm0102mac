#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void open_loan_recall(Cpu& c,std::uint32_t person_address) {
    Frame f(c,0x1042f670,0x380,{.first_gpr=27});
    Kept<27> person(c,person_address);
    Kept<28> manager(c,c.r(28));
    Kept<29> text(c,c.toc(-0x7bb8));
    Kept<30> pool(c,c.toc(slot::pool));
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    manager=c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4);
    const auto eligible=p.call(0x103fa850,0x1042f6c0,{indirect(c,slot::transfers),c.u32(manager+0x24),person,text,c.toc(slot::transfer_context),0});
    if (!byte_result(c,eligible)) {
        p.call(0x100caeb0,0x1042f6dc,{person,f.at(0x1cc),400,5});
        c.set(3,c.u32(manager+0x24));
        if (compare_logical(c,c.r(3),c.u32(person+0x24))==0)
            p.call(0x10727890,0x1042f700,{f.at(0x3c),pool+0x331a,100,f.at(0x1cc)});
        else p.call(0x10727890,0x1042f714,{f.at(0x3c),pool+0x3356,f.at(0x1cc)});
        p.call(0x100c7a50,0x1042f728,{c.u32(manager+0x24),1,f.at(0x3a),f.at(0x38)});
        p.call(0x100f05a0,0x1042f754,{c.u32(manager+0x24)+0x1ee,f.at(0x3c),text,0,
            std::uint32_t(std::int32_t(c.i16(f.at(0x38)))),std::uint32_t(std::int32_t(c.i16(f.at(0x3a)))),0});
    } else if (byte_result(c,p.call(0x1073e4b0,0x1042f774,{screens,c.toc(-0x4b80),c.toc(-0x4b84),1,0,0}))) {
        p.history(0x1042f790,screens,0,person);
        p.history(0x1042f7a4,screens,1,c.u32(person+0x24));
    }
}

void render_loan_recall(Cpu& c) {
    Frame f(c,0x1042f7c0,0x1f0,{.first_gpr=23});
    Kept<23> rows(c,c.r(23));
    Kept<24> error_or_club_or_grid(c,c.toc(-0x7ba8));
    Kept<25> owner(c,c.r(25));
    Kept<26> person(c,c.r(26));
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
    c.put16(f.at(0x60),c.u16(c.r(2)-0x377c));
    person=p.call(0x1073ef70,0x1042f810,{screens,0});
    owner=p.call(0x1073ef70,0x1042f824,{screens,1});
    p.call(0x107136d0,0x1042f83c,{person,0,0});
    c.set(3,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4));
    if (compare(c,c.r(3))==0) {
        report_assertion(c,{0x1042f870,pool,pool+0x15,pool+0x22,f.at(0x70),f.at(0x68),0x27cc});
        c.set(3,c.toc(-0x7bac)); return;
    }
    error_or_club_or_grid=c.u32(c.r(3)+0x24);
    p.call(0x100c7a50,0x1042f8c8,{error_or_club_or_grid,1,f.at(0x66),f.at(0x64)});
    p.call(0x10744250,0x1042f8dc,{screens,c.u16(f.at(0x64)),c.u16(f.at(0x66)),error_or_club_or_grid+0x1ee});
    p.call(0x100caeb0,0x1042f8f0,{person,f.at(0x90),300,5});
    if (compare_logical(c,error_or_club_or_grid,c.u32(person+0x24))==0)
        p.call(0x10727890,0x1042f910,{text,pool+0x3388,0,f.at(0x90)});
    else p.call(0x10727890,0x1042f924,{text,pool+0x33bf,f.at(0x90)});
    p.call(0x107442f0,0x1042f930,{screens,text});
    rows=std::uint32_t(std::int32_t(std::int8_t(divide_signed<4>(405,p.call(0x1076d470,0x1042f938,{3})))));
    p.call(0x100caeb0,0x1042f958,{person,f.at(0x90),300,1});
    const bool terminating=compare_logical(c,error_or_club_or_grid,c.u32(person+0x24))==0;
    const auto gender=std::uint32_t(std::int32_t(c.i8(owner+0x23b)));
    if (terminating) p.call(0x10727890,0x1042f98c,{text,pool+0x33f4,0,gender,gender,f.at(0x90),owner+0x221});
    else p.call(0x10727890,0x1042f9b4,{text,pool+0x345a,gender,gender,f.at(0x90),owner+0x221});
    stack_args({c.u16(foreground),text,f.at(0x90),0x7d7,person,owner+0x221,0x7d5,owner,0});
    p.call(0x1074a350,0x1042fa14,{110,140,780,545,2,0,std::uint32_t(std::int32_t(std::int8_t(rows))),3});
    p.call(0x102a1070,0x1042fa20,{4,0});
    stack_args({0,1,0,missing});
    error_or_club_or_grid=p.call(0x107198c0,0x1042fa70,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,2,f.at(0x60),1});
    p.call(0x10727890,0x1042fa84,{text,pool+0x5b2});
    stack_args({c.u16(foreground),text,1,0});
    p.call(0x107444e0,0x1042fac4,{screens,error_or_club_or_grid,0,0,0x30,c.u16(disabled_colour),12,3});
    p.call(0x10727890,0x1042fad0,{text,pool+0x5b5});
    stack_args({c.u16(foreground),text,2,0});
    p.call(0x107444e0,0x1042fb10,{screens,error_or_club_or_grid,1,0,0x30,c.u16(disabled_colour),12,3});
}

std::uint32_t loan_recall_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x1042fb30,0x450,{.first_gpr=26});
    Kept<26> text_or_result(c,c.toc(-0x7bb8));
    Kept<27> pool(c,c.toc(slot::pool));
    Kept<28> contracts_or_manager_club(c,c.toc(slot::contracts));
    Kept<29> current_contract_or_owner(c,c.toc(slot::current_contract));
    Kept<30> screens(c,c.toc(slot::screens));
    Kept<31> control_or_person(c,control_value);
    Program p{c};
    if (compare(c,c.i8(screens+0x2c5f))!=0) {
        c.set(3,screens+0x260000);
        if (compare(c,c.u32(c.r(3)-0x7e60),3)==0) {
            c.put32(c.r(3)-0x7e60,0); c.set(4,screens+0x260000); c.put32(c.r(4)-0x7e5c,0);
            return std::uint32_t(-9);
        }
    }
    if (compare(c,control_action(c,screens,control_or_person),1)==0) return std::uint32_t(-9);
    if (compare(c,control_action(c,screens,control_or_person),2)==0) {
        control_or_person=p.call(0x1073ef70,0x1042fc58,{screens,0});
        const auto contract=p.call(0x10082f20,0x1042fc6c,{c.u32(contracts_or_manager_club),control_or_person});
        compare_logical(c,contract); c.put32(current_contract_or_owner,contract);
        if (contract) {
            c.set(3,c.u32(current_contract_or_owner)); c.set(4,c.toc(slot::clubs));
            c.set(3,c.u32(c.r(4))+c.u32(c.r(3)+4)*0x248);
        } else c.set(3,0);
        current_contract_or_owner=c.r(3);
        c.set(3,c.u32(contracts_or_manager_club));
        c.set(5,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4));
        contracts_or_manager_club=c.u32(c.r(5)+0x24);
        p.call(0x1007fb20,0x1042fcc0,{c.r(3),control_or_person,contracts_or_manager_club});
        p.call(0x100c7a50,0x1042fcd4,{contracts_or_manager_club,1,f.at(0x3a),f.at(0x38)});
        p.call(0x100caeb0,0x1042fce8,{control_or_person,f.at(0x44),400,5});
        if (compare_logical(c,contracts_or_manager_club,c.u32(control_or_person+0x24))==0)
            p.call(0x10727890,0x1042fd08,{f.at(0x1d4),pool+0x34b6,0,f.at(0x44)});
        else p.call(0x10727890,0x1042fd1c,{f.at(0x1d4),pool+0x34e9,f.at(0x44)});
        p.call(0x100caeb0,0x1042fd30,{control_or_person,f.at(0x44),400,1});
        const auto gender=std::uint32_t(std::int32_t(c.i8(current_contract_or_owner+0x23b)));
        p.call(0x10727890,0x1042fd54,{text_or_result,pool+0x351c,gender,gender,f.at(0x44),current_contract_or_owner+0x221});
        p.call(0x1074b6e0,0x1042fd60,{f.at(0x3c),text_or_result});
        p.call(0x1074bab0,0x1042fd74,{f.at(0x3c),f.at(0x44),0x7d7,control_or_person});
        p.call(0x1074bab0,0x1042fd88,{f.at(0x3c),contracts_or_manager_club+0x221,0x7d5,contracts_or_manager_club});
        p.call(0x100f0720,0x1042fdb0,{contracts_or_manager_club+0x1ee,f.at(0x1d4),f.at(0x3c),0,
            std::uint32_t(std::int32_t(c.i16(f.at(0x3a)))),std::uint32_t(std::int32_t(c.i16(f.at(0x38)))),3});
        text_or_result=std::uint32_t(-4);
        p.call(0x1074b7c0,0x1042fdc0,{f.at(0x3c),missing});
        return text_or_result;
    }
    const auto command=p.call(0x102a54b0,0x1042fdd0,{control_or_person});
    if (compare(c,command)!=0) return command;
    const auto fallback=p.call(0x102a83a0,0x1042fde4,{control_or_person});
    c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0); return fallback;
}
}
