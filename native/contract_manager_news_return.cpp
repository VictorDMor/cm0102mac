#include "contract_manager_news_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;

void render_cancelled_trial(Cpu& c, std::uint32_t address, std::uint32_t mode, std::uint32_t out) {
    c.set(0,c.lr()); Frame f(c,0x100913f0,0x1a0,{.first_gpr=22});
    Kept<22> clubs(c,c.toc(-0x7be8)); Kept<26> message(c,address); Kept<30> buffer(c,c.toc(-0x7bb8));
    Kept<25> output(c,out); Kept<31> staff(c,c.toc(-0x7be0)); Kept<23> pool(c,c.toc(slot::pool)); Program p(c);
    const auto display=std::int8_t(mode);c.set(0,std::uint32_t(std::int32_t(display)));
    if(nonzero(c,display)) {
        auto value=p.call(0x102cb570,0x1009142c,{message,0});c.set(0,value);Kept<29> person(c,value);
        value=p.call(0x102cb570,0x10091440,{message,1});c.set(0,value);Kept<28> first(c,value);
        value=p.call(0x102cb570,0x10091454,{message,2});c.set(0,value);Kept<27> second(c,value);
        value=p.call(0x102cb570,0x10091468,{message,3});c.set(0,value);message=value;
        Kept<24> club(c,p.call(0x102cb570,0x1009147c,{address,4}));
        p.call(0x100cb590,0x100914a0,{first,second,message,f.at(0x40),0x12c,1,person});
        club=std::uint32_t(club)*0x248;const auto records=c.u32(clubs);c.set(7,records);
        const auto gender=c.i8(records+std::uint32_t(club)+0x242);const auto name=records+std::uint32_t(club)+0x1ee;
        c.set(0,name);c.put32(f.at(0x38),name);c.put32(f.at(0x3c),name);
        const auto g=std::uint32_t(std::int32_t(gender));
        p.call(0x10727890,0x100914e4,{buffer,pool+0x6eff,0x64,g,g,g,g,f.at(0x40)});
        p.call(0x1074b830,0x100914f0,{output,buffer});
        const auto current=c.u32(clubs);c.set(0,current);
        p.call(0x1074bab0,0x10091508,{output,current+std::uint32_t(club)+0x1ee,0x7d5,current+std::uint32_t(club)});
        clubs=std::uint32_t(person);
        news::person_link(c,p,f,clubs,staff,output,first,second,message,0x40,0x10091528,0x10091548);
    } else {
        auto value=p.call(0x102cb570,0x10091558,{message,0});c.set(0,value);clubs=value;
        value=p.call(0x102cb570,0x1009156c,{message,1});c.set(0,value);Kept<24> first(c,value);
        value=p.call(0x102cb570,0x10091580,{message,2});c.set(0,value);message=value;
        value=p.call(0x102cb570,0x10091594,{address,3});c.set(0,value);Kept<27> third(c,value);
        p.call(0x100cb590,0x100915bc,{first,message,third,f.at(0x40),0x12c,1,clubs});
        p.call(0x10727890,0x100915d0,{buffer,pool+0x6fbd,0x64,f.at(0x40)});
        p.call(0x1074b830,0x100915dc,{output,buffer});
        news::person_link(c,p,f,clubs,staff,output,first,message,third,0x40,0x100915fc,0x1009161c);
    }
}

void render_loan_return(Cpu& c,std::uint32_t address,std::uint32_t mode,std::uint32_t out) {
    c.set(0,c.lr());Frame f(c,0x100918b0,0x1b0,{.first_gpr=20});
    Kept<26> clubs(c,c.toc(-0x7be8));Kept<20> message(c,address);Kept<27> buffer(c,c.toc(-0x7bb8));
    Kept<21> output(c,out);Kept<28> staff(c,c.toc(-0x7be0));Kept<29> pool(c,c.toc(slot::pool));Program p(c);
    const auto display=std::int8_t(mode);c.set(0,std::uint32_t(std::int32_t(display)));
    if(nonzero(c,display)) {
        auto value=p.call(0x102cb570,0x100918ec,{message,0});c.set(0,value);Kept<25> person(c,value);
        value=p.call(0x102cb570,0x10091900,{message,1});c.set(0,value);Kept<24> first(c,value);
        value=p.call(0x102cb570,0x10091914,{message,2});c.set(0,value);Kept<23> second(c,value);
        value=p.call(0x102cb570,0x10091928,{message,3});c.set(0,value);Kept<22> third(c,value);
        Kept<30> employer(c,p.call(0x102cb570,0x1009193c,{message,4}));
        Kept<31> lender(c,p.call(0x102cb570,0x1009194c,{message,5}));
        message=p.call(0x102cb570,0x1009195c,{message,6});
        p.call(0x100cb590,0x10091980,{first,second,third,f.at(0x48),0x12c,1,person});
        const bool recalled=compare(c,std::int32_t(std::uint32_t(message)),std::int32_t(std::uint32_t(lender)))==0;
        const auto lender_offset=std::uint32_t(lender)*0x248;c.set(3,lender_offset);
        const auto records=c.u32(clubs);const auto employer_offset=std::uint32_t(employer)*0x248;
        const auto lender_name=records+lender_offset+0x221,employer_name=records+employer_offset+0x221;
        const auto lender_gender=std::uint32_t(std::int32_t(c.i8(records+lender_offset+0x242)));
        const auto employer_gender=std::uint32_t(std::int32_t(c.i8(records+employer_offset+0x242)));
        if(recalled) {
            c.set(11,lender_name);c.set(0,employer_name);
            c.put32(f.at(0x38),lender_name);c.put32(f.at(0x3c),f.at(0x48));
            c.put32(f.at(0x40),employer_name);c.put32(f.at(0x44),lender_name);
            p.call(0x10727890,0x100919e8,{buffer,pool+0x6ff6,lender_gender,lender_gender,
                employer_gender,employer_gender,lender_gender,lender_gender});
        } else {
            c.set(0,lender_name);c.put32(f.at(0x38),f.at(0x48));c.put32(f.at(0x3c),lender_name);
            p.call(0x10727890,0x10091a40,{buffer,pool+0x70ca,employer_gender,employer_gender,
                0,lender_gender,lender_gender,employer_name});
        }
        p.call(0x1074b830,0x10091a4c,{output,buffer});
        c.set(0,employer_offset);auto current=c.u32(clubs);c.set(4,current);
        p.call(0x1074bab0,0x10091a68,{output,current+employer_offset+0x221,0x7d5,current+employer_offset});
        c.set(0,lender_offset);current=c.u32(clubs);c.set(4,current);
        p.call(0x1074bab0,0x10091a84,{output,current+lender_offset+0x221,0x7d5,current+lender_offset});
        message=std::uint32_t(person);
        news::person_link(c,p,f,message,staff,output,first,second,third,0x48,0x10091aa4,0x10091ac4);
    } else {
        auto value=p.call(0x102cb570,0x10091ad4,{message,0});c.set(0,value);Kept<23> person(c,value);
        value=p.call(0x102cb570,0x10091ae8,{message,1});c.set(0,value);Kept<24> first(c,value);
        value=p.call(0x102cb570,0x10091afc,{message,2});c.set(0,value);Kept<25> second(c,value);
        value=p.call(0x102cb570,0x10091b10,{message,3});c.set(0,value);clubs=value;
        Kept<22> lender(c,p.call(0x102cb570,0x10091b24,{address,5}));
        message=p.call(0x102cb570,0x10091b34,{address,6});
        p.call(0x100cb590,0x10091b58,{first,second,clubs,f.at(0x48),0x12c,1,person});
        if(compare(c,std::int32_t(std::uint32_t(message)),std::int32_t(std::uint32_t(lender)))==0)
            p.call(0x10727890,0x10091b70,{buffer,pool+0x7174,f.at(0x48)});
        else p.call(0x10727890,0x10091b88,{buffer,pool+0x71ae,0x64,f.at(0x48)});
        p.call(0x1074b830,0x10091b94,{output,buffer});
        message=std::uint32_t(person);
        news::person_link(c,p,f,message,staff,output,first,second,clubs,0x48,0x10091bb4,0x10091bd4);
    }
}
}
