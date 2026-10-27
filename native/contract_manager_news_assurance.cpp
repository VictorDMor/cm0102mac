#include "contract_manager_news_detail.hpp"
#include <array>

namespace cm::aot::contract_manager {
using namespace detail;
void render_withdrawn_request(Cpu& c,std::uint32_t address,std::uint32_t mode,std::uint32_t out) {
    c.set(0,c.lr());Frame f(c,0x1008f7b0,0x1b0,{.first_gpr=19});
    Kept<19> message(c,address);Kept<27> clubs(c,c.toc(-0x7be8));Kept<28> buffer(c,c.toc(-0x7bb8));
    Kept<21> reason(c,mode);Kept<29> staff(c,c.toc(-0x7be0));Kept<22> output(c,out);Kept<30> pool(c,c.toc(slot::pool));Program p(c);
    Kept<31> club(c,p.call(0x102cb570,0x1008f7e8,{message,0}));
    auto value=p.call(0x102cb570,0x1008f7f8,{message,2});c.set(0,value);Kept<26> person(c,value);
    value=p.call(0x102cb570,0x1008f80c,{message,3});c.set(0,value);Kept<25> first(c,value);
    value=p.call(0x102cb570,0x1008f820,{message,4});c.set(0,value);Kept<24> second(c,value);
    value=p.call(0x102cb570,0x1008f834,{message,5});c.set(0,value);Kept<23> third(c,value);
    Kept<20> flags(c,p.call(0x102cb570,0x1008f848,{message,6}));
    value=p.call(0x102cb570,0x1008f858,{message,7});
    const auto display=std::int8_t(mode);c.set(0,std::uint32_t(std::int32_t(display)));const bool full=nonzero(c,display);reason=value;
    p.call(0x100cb590,full?0x1008f884:0x1008f9cc,{first,second,third,f.at(0x40),0x12c,full?1u:5u,person});
    if(full) {
        const auto bigger=std::uint32_t(flags)&1;c.set(0,bigger);const bool ambition=nonzero_logical(c,bigger);
        bool playing=false;
        if(!ambition){const auto unhappy=std::uint32_t(flags)&0x14;c.set(0,unhappy);if(nonzero_logical(c,unhappy))playing=compare(c,std::int32_t(std::uint32_t(reason)))!=0;}
        const auto offset=std::uint32_t(club)*0x248;c.set(6,offset);const auto records=c.u32(clubs);c.set(0,records);
        const auto gender=std::uint32_t(std::int32_t(c.i8(records+offset+0x242))),name=records+offset+0x1ee;
        if(ambition){c.put32(f.at(0x38),name);p.call(0x10727890,0x1008f8c8,{buffer,pool+0x5507,gender,gender,gender,gender,f.at(0x40),name});}
        else p.call(0x10727890,playing?0x1008f90c:0x1008f940,{buffer,pool+(playing?0x5629:0x5707),gender,gender,f.at(0x40),name});
        p.call(0x1074b830,0x1008f94c,{output,buffer});c.set(0,offset);const auto current=c.u32(clubs);c.set(4,current);
        p.call(0x1074bab0,0x1008f968,{output,current+offset+0x1ee,0x7d5,current+offset});
    } else {p.call(0x10727890,0x1008f9dc,{buffer,pool+0x57b1,f.at(0x40)});p.call(0x1074b830,0x1008f9e8,{output,buffer});}
    flags=std::uint32_t(person);
    news::person_link(c,p,f,flags,staff,output,first,second,third,0x40,full?0x1008f988:0x1008fa08,full?0x1008f9a8:0x1008fa28);
}

void render_assurance_request(Cpu& c,std::uint32_t address,std::uint32_t mode,std::uint32_t out) {
    c.set(0,c.lr());Frame f(c,0x1008fcf0,0x1b0,{.first_gpr=16});
    Kept<17> message(c,address);Kept<24> buffer(c,c.toc(-0x7bb8));Kept<25> staff(c,c.toc(-0x7be0));
    Kept<18> display(c,mode);Kept<26> pool(c,c.toc(slot::pool));Kept<19> output(c,out);Program p(c);
    Kept<27> club(c,p.call(0x102cb570,0x1008fd24,{message,0}));
    auto value=p.call(0x102cb570,0x1008fd34,{message,2});c.set(0,value);Kept<23> person(c,value);
    value=p.call(0x102cb570,0x1008fd48,{message,3});c.set(0,value);Kept<22> first(c,value);
    value=p.call(0x102cb570,0x1008fd5c,{message,4});c.set(0,value);Kept<21> second(c,value);
    value=p.call(0x102cb570,0x1008fd70,{message,5});c.set(0,value);Kept<20> third(c,value);
    Kept<16> changed(c,p.call(0x102cb570,0x1008fd84,{message,6}));
    Kept<28> role(c,p.call(0x102cb570,0x1008fd94,{message,7}));
    Kept<29> before(c,p.call(0x102cb570,0x1008fda4,{message,8}));
    Kept<30> after(c,p.call(0x102cb570,0x1008fdb4,{message,9}));
    Kept<31> confused(c,p.call(0x102cb570,0x1008fdc4,{message,10}));
    p.call(0x102cb570,0x1008fdd4,{message,11});
    const auto byte=std::int8_t(mode);c.set(0,std::uint32_t(std::int32_t(byte)));const bool full=nonzero(c,byte);
    p.call(0x100cb590,full?0x1008fdfc:0x10090294,{first,second,third,f.at(0x38),0x12c,full?1u:5u,person});
    if(full) {
        p.call(0x10727890,0x1008fe0c,{buffer,pool+0x57dc,f.at(0x38)});p.call(0x1074b830,0x1008fe18,{output,buffer});
        if(compare(c,std::int32_t(std::uint32_t(changed)))!=0&&compare(c,std::int32_t(std::uint32_t(after)),-1)!=0) {
            unsigned policy;
            if(compare(c,std::int32_t(std::uint32_t(before)),std::int32_t(std::uint32_t(after)))==0)policy=5;
            else {
                auto bits=std::uint32_t(after)&2;c.set(0,bits);
                if(nonzero_logical(c,bits))policy=0;
                else {bits=std::uint32_t(after)&1;c.set(0,bits);
                    if(nonzero_logical(c,bits))policy=1;
                    else {bits=std::uint32_t(before)&1;c.set(0,bits);
                        if(nonzero_logical(c,bits))policy=2;
                        else {bits=std::uint32_t(before)&2;c.set(0,bits);policy=nonzero_logical(c,bits)?3:4;}}}
            }
            if(compare_logical(c,role,7)<=0) {
                constexpr int slots[]={-0x5750,-0x5754,-0x5758,-0x575c,-0x5760,-0x5764};
                const auto table=c.toc(slots[policy]);c.set(3,table);c.set(0,std::uint32_t(role)*4);
                const auto target=c.u32(table+std::uint32_t(role)*4);c.set(3,target);c.s.wr<4>(guest::reg::ctr,target);
                constexpr unsigned offsets[][7]={
                    {0x5837,0x587a,0x58d2,0x5930,0x5980,0x59e6,0x5a48},
                    {0x5a95,0x5ad9,0x5b32,0x5b91,0x5be2,0x5c4c,0x5cb2},
                    {0x5d03,0x5d4d,0x5dac,0x5e11,0x5e68,0x5ed5,0x5f3e},
                    {0x5f93,0x5fd9,0x6034,0x6095,0x60e8,0x6151,0x61b6},
                    {0x6206,0x622d,0x6269,0x62ab,0x62df,0x6329,0x636f},
                    {0x6206,0x622d,0x6269,0x62ab,0x62df,0x6329,0x636f}};
                constexpr std::uint32_t first_back[]={0x1008fe60,0x1008fef4,0x1008ff88,0x1009001c,0x100900a8,0x10090134};
                if(std::uint32_t(role)!=0)p.call(0x10727890,first_back[policy]+(std::uint32_t(role)-1)*0x10,{buffer,pool+offsets[policy][std::uint32_t(role)-1]});
            }
            p.call(0x1074b960,0x100901a0,{output,buffer});
            if(compare(c,std::int32_t(std::uint32_t(confused)))!=0) {
                const auto bits=std::uint32_t(after)&1;c.set(0,bits);
                if(nonzero_logical(c,bits))p.call(0x10727890,0x100901bc,{buffer,pool+0x63a0});
                else if(compare(c,std::int32_t(std::uint32_t(role)),1)==0)p.call(0x10727890,0x100901d4,{buffer,pool+0x63ee});
                else if(compare(c,std::int32_t(std::uint32_t(role)),3)==0||compare(c,std::int32_t(std::uint32_t(role)),2)==0)p.call(0x10727890,0x100901f4,{buffer,pool+0x6414});
                else p.call(0x10727890,0x10090204,{buffer,pool+0x64b9});
                p.call(0x1074b960,0x10090210,{output,buffer});
            }
        }
        const auto holder=c.toc(-0x7be8);c.set(4,holder);const auto offset=std::uint32_t(club)*0x248;c.set(0,offset);const auto records=c.u32(holder);c.set(4,records);
        p.call(0x1074bab0,0x10090230,{output,records+offset+0x1ee,0x7d5,records+offset});
    } else {p.call(0x10727890,0x100902a4,{buffer,pool+0x650b,f.at(0x38)});p.call(0x1074b830,0x100902b0,{output,buffer});}
    changed=std::uint32_t(person);
    news::person_link(c,p,f,changed,staff,output,first,second,third,0x38,full?0x10090250:0x100902d0,full?0x10090270:0x100902f0);
}
}
