#include "contract_manager_random.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
struct ReasonTemplate {std::uint32_t flag;unsigned text;std::uint32_t back;};
std::uint32_t request_third(std::uint32_t value) {
    const auto high=multiply_high(0x55555556,std::int32_t(value));
    return std::uint32_t(high)+(std::uint32_t(high)>>31);
}
}

void render_transfer_request(Cpu& c,std::uint32_t address,std::uint32_t mode,std::uint32_t out) {
    c.set(0,c.lr());Frame f(c,0x1008ec00,0x670,{.first_gpr=13});
    Kept<22> message(c,address);Kept<15> clubs(c,c.toc(-0x7be8));Kept<16> buffer(c,c.toc(-0x7bb8));
    Kept<14> display(c,mode);Kept<13> staff(c,c.toc(-0x7be0));Kept<21> output(c,out);Kept<17> pool(c,c.toc(slot::pool));Program p(c);
    Kept<18> club(c,p.call(0x102cb570,0x1008ec38,{message,0}));
    auto value=p.call(0x102cb570,0x1008ec48,{message,2});c.set(0,value);Kept<30> person(c,value);
    value=p.call(0x102cb570,0x1008ec5c,{message,3});c.set(0,value);Kept<29> first(c,value);
    value=p.call(0x102cb570,0x1008ec70,{message,4});c.set(0,value);Kept<28> second(c,value);
    value=p.call(0x102cb570,0x1008ec84,{message,5});c.set(0,value);Kept<27> third(c,value);
    Kept<19> reasons(c,p.call(0x102cb570,0x1008ec98,{message,6}));
    const auto signed_byte=[](std::uint32_t word){return std::uint32_t(std::int32_t(std::int8_t(word)));};
    Kept<26> changed(c,signed_byte(p.call(0x102cb570,0x1008eca8,{message,7})));
    Kept<25> accepted(c,signed_byte(p.call(0x102cb570,0x1008ecb8,{message,8})));
    Kept<24> agent(c,signed_byte(p.call(0x102cb570,0x1008ecc8,{message,9})));
    Kept<20> nationality(c,p.call(0x102cb570,0x1008ecd8,{message,10}));
    Kept<23> retiring(c,signed_byte(p.call(0x102cb570,0x1008ece8,{message,11})));
    value=p.call(0x102cb570,0x1008ecf8,{message,12});
    const auto mode_byte=std::int8_t(std::uint32_t(display));c.set(0,std::uint32_t(std::int32_t(mode_byte)));const bool full=nonzero(c,mode_byte);Kept<14> status(c,value);
    const auto byte=[&](std::uint32_t word){const auto narrowed=std::int8_t(word);c.set(0,std::uint32_t(std::int32_t(narrowed)));return nonzero(c,narrowed);};
    const auto name=[&](unsigned offset,unsigned variant,std::uint32_t back){p.call(0x100cb590,back,{first,second,third,f.at(offset),300,variant,person});};
    if(full){
        c.set(0,0);Kept<31> bigger(c,std::uint32_t(reasons)&1u);const bool big=nonzero(c,std::int32_t(std::uint32_t(bigger)));
        c.put8(f.at(0x234),0);Kept<22> reason_count(c,0);for(unsigned at:{0x40u,0xa4u,0x108u,0x16cu,0x1d0u})c.put8(f.at(at),0);
        const auto count=[&](unsigned reg){const auto n=std::int32_t(std::int8_t(std::uint32_t(reason_count)));c.set(reg,std::uint32_t(n));return n;};
        const auto flag=[&](std::uint32_t source,std::uint32_t mask){const auto bits=source&mask;c.set(0,bits);return nonzero(c,std::int32_t(bits));};
        const auto reason=[&](unsigned text,std::uint32_t back,std::int32_t n){c.set(0,std::uint32_t(n)*100);p.call(0x10727890,back,{f.at(0x40)+std::uint32_t(n)*100,pool+text});reason_count=reason_count+1;};
        const auto simple=[&](ReasonTemplate item){const auto n=count(3);if(compare(c,n,5)<0&&flag(reasons,item.flag))reason(item.text,item.back,n);};
        if(big){p.call(0x10727890,0x1008ed38,{f.at(0x40),pool+0x3367});reason_count=1;}
        auto n=count(3);
        if(compare(c,n,5)<0&&flag(reasons,2)){
            if(byte(agent))reason(0x4a92,0x1008ed6c,n);
            else {const bool pay=flag(reasons,0x01000000);reason(pay?0x3386:0x33b8,pay?0x1008ed8c:0x1008eda4,n);}
        }
        for(auto item:{ReasonTemplate{4,0x4ad4,0x1008edd0},ReasonTemplate{0x100,0x33dd,0x1008edfc},ReasonTemplate{8,0x4b04,0x1008ee28},ReasonTemplate{0x10,0x340a,0x1008ee54}})simple(item);
        // Scaling and reputation loading happen even when five reasons exist.
        club=std::uint32_t(club)*0x248;const auto records=c.u32(clubs);c.set(0,records);c.set(3,records+std::uint32_t(club));
        const auto reputation=c.i16(records+std::uint32_t(club)+0x1ec);c.set(0,extended(reputation));
        if(compare(c,reputation,6750)<0){n=count(3);if(compare(c,n,5)<0&&flag(reasons,0x20000000))reason(0x4b2a,0x1008ee98,n);}
        n=count(4);
        if(compare(c,n,5)<0&&flag(reasons,0x20)){
            const auto records=c.u32(clubs);c.set(0,records);c.set(3,records+std::uint32_t(club));const auto nation=c.u32(records+std::uint32_t(club)+0x2c);c.set(7,nation);
            if(nonzero_logical(c,nation)){const auto gender=std::uint32_t(std::int32_t(c.i8(nation+0x115)));c.set(0,std::uint32_t(n)*100);
                p.call(0x10727890,0x1008eee8,{f.at(0x40)+std::uint32_t(n)*100,pool+0x4b51,gender,gender,nation+0xae});reason_count=reason_count+1;}
        }
        // The original deliberately tests 0x800 twice for two distinct texts.
        for(auto item:{ReasonTemplate{0x40,0x4b9b,0x1008ef14},ReasonTemplate{0x80,0x4bbe,0x1008ef40},ReasonTemplate{0x200,0x4bfc,0x1008ef6c},ReasonTemplate{0x4000,0x4c1f,0x1008ef98},
                       ReasonTemplate{0x400,0x4c56,0x1008efc4},ReasonTemplate{0x800,0x4c93,0x1008eff0},ReasonTemplate{0x2000,0x4cb1,0x1008f01c},ReasonTemplate{0x800,0x4cda,0x1008f048}})simple(item);
        n=count(5);
        if(compare(c,n,5)<0&&flag(reasons,0x20000)){
            const bool home=byte(retiring)&&compare(c,std::int32_t(std::uint32_t(nationality)),-1)!=0;
            if(home){const auto holder=c.toc(-0x7b98);c.set(4,holder);const auto offset=std::uint32_t(nationality)*0x128;c.set(6,offset);
                const auto nations=c.u32(holder);c.set(8,nations);const auto gender=std::uint32_t(std::int32_t(c.i8(nations+offset+0x116)));c.set(0,std::uint32_t(n)*100);
                p.call(0x10727890,0x1008f0a8,{f.at(0x40)+std::uint32_t(n)*100,pool+0x4cf9,gender,gender,nations+offset+0xe1});reason_count=reason_count+1;
            }else reason(0x4d39,0x1008f0c4,n);
        }
        for(auto item:{ReasonTemplate{0x400000,0x4d45,0x1008f0f0},ReasonTemplate{0x2000000,0x4d76,0x1008f11c},ReasonTemplate{0x4000000,0x4d9e,0x1008f148},ReasonTemplate{0x200000,0x4dd2,0x1008f174}})simple(item);
        n=count(3);if(compare(c,n,5)<0&&flag(reasons,0x40000)&&compare(c,std::int32_t(std::uint32_t(bigger)))==0)reason(0x4e0d,0x1008f1a8,n);
        n=count(3);if(compare(c,n,5)<0&&flag(status,0x80))reason(0x4e31,0x1008f1d4,n);
        Kept<14> selected_count(c,std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(reason_count)))));
        const auto summary_order=compare(c,std::int32_t(std::uint32_t(selected_count)),3);
        if(summary_order==0)p.call(0x10727890,0x1008f24c,{f.at(0x234),pool+0x4e7c,f.at(0x40),f.at(0xa4),f.at(0x108)});
        else if(summary_order<0){const auto small=compare(c,std::int32_t(std::uint32_t(selected_count)),1);
            if(small==0)p.call(0x1077b720,0x1008f218,{f.at(0x234),pool+0x923,f.at(0x40)});
            else if(small>0)p.call(0x10727890,0x1008f230,{f.at(0x234),pool+0x926,f.at(0x40),f.at(0xa4)});
        }else {const auto large=compare(c,std::int32_t(std::uint32_t(selected_count)),5);
            if(large==0)p.call(0x10727890,0x1008f298,{f.at(0x234),pool+0x4ef8});
            else if(large<0){const auto sign=std::uint32_t(person)>>31;c.set(3,sign);const auto parity=((std::uint32_t(person)&1u)^sign)-sign;
                c.set(7,parity*100);c.set(0,(parity+2)*100);p.call(0x10727890,0x1008f288,{f.at(0x234),pool+0x4eb1,f.at(0x40)+parity*100,f.at(0x40)+(parity+2)*100});}
        }
        name(0x4f0,1,0x1008f2b8);name(0x3c4,5,0x1008f2d8);
        struct ClubName {std::uint32_t records,gender,address;};
        const auto club_name=[&]{const auto records=c.u32(clubs);return ClubName{records,std::uint32_t(std::int32_t(c.i8(records+std::uint32_t(club)+0x242))),records+std::uint32_t(club)+0x1ee};};
        if(byte(reason_count)){
            c.set(3,0x55550000);c.set(5,0x55555556);const auto high=multiply_high(0x55555556,std::int32_t(std::uint32_t(person)));c.set(3,std::uint32_t(high));
            const auto threshold=std::uint32_t(person)-request_third(person)*3+2;c.set(3,threshold-2);c.set(0,threshold);
            const bool severe=compare(c,std::int32_t(std::uint32_t(selected_count)),std::int32_t(threshold))>0;bool agent_story=false;
            if(byte(agent)){const auto difference=std::uint32_t(reasons)-std::uint32_t(person);c.set(4,difference);
                const auto high=multiply_high(0x55555556,std::int32_t(difference));c.set(3,std::uint32_t(high));const auto remainder=difference-request_third(difference)*3;c.set(0,remainder);
                const bool differs=nonzero(c,std::int32_t(remainder));agent_story=severe?differs:!differs;}
            if(agent_story){const auto small_count=std::int32_t(std::int8_t(std::uint32_t(reason_count)));c.set(0,std::uint32_t(small_count));const bool expanded=compare(c,small_count,4)<0;
                const auto own=club_name();c.set(0,expanded?f.at(0x234):own.records);if(expanded)c.put32(f.at(0x38),f.at(0x234));
                const auto text=severe?(expanded?0x4f4d:0x502d):(expanded?0x51bf:0x5294);const auto back=severe?(expanded?0x1008f374:0x1008f3ac):(expanded?0x1008f450:0x1008f488);
                p.call(0x10727890,back,{buffer,pool+text,100,own.gender,own.gender,f.at(0x4f0),own.address,f.at(0x3c4)});
            }else {const auto own=club_name();c.set(0,own.records);p.call(0x10727890,severe?0x1008f3e0:0x1008f4bc,{buffer,pool+(severe?0x512a:0x539e),own.gender,own.gender,f.at(0x4f0),own.address,f.at(0x234)});}
        }else {const auto own=club_name();c.set(0,own.records);p.call(0x10727890,0x1008f4ec,{buffer,pool+0x5423,own.gender,own.gender,f.at(0x4f0),own.address});}
        p.call(0x1074b830,0x1008f4f8,{output,buffer});
        if(byte(changed)){const bool granted=compare(c,std::int32_t(std::uint32_t(accepted)),1)==0;p.call(0x10727890,granted?0x1008f514:0x1008f524,{buffer,pool+(granted?0x5498:0x54b8)});p.call(0x1074b960,0x1008f530,{output,buffer});}
        const auto current=c.u32(clubs);c.set(0,current);p.call(0x1074bab0,0x1008f548,{output,current+std::uint32_t(club)+0x1ee,0x7d5,current+std::uint32_t(club)});
    }else {name(0x4f0,5,0x1008f5ac);p.call(0x10727890,0x1008f5bc,{buffer,pool+0x54dc,f.at(0x4f0)});p.call(0x1074b830,0x1008f5c8,{output,buffer});}
    Kept<14> person_offset(c,std::uint32_t(person)*112);const auto records=c.u32(staff);c.set(0,records);
    const auto available=p.call(0x100cbde0,full?0x1008f568:0x1008f5e8,{records+std::uint32_t(person_offset),first,second,third,0});
    if(byte(available)){const auto fresh=c.u32(staff);c.set(0,fresh);p.call(0x1074bab0,full?0x1008f588:0x1008f608,{output,f.at(0x4f0),0x7d7,fresh+std::uint32_t(person_offset)});}
}
}
