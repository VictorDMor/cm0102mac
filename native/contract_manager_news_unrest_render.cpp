#include "contract_manager_random.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
std::uint32_t third_quotient(std::uint32_t value) {
    const auto high=multiply_high(0x55555556,std::int32_t(value));
    return std::uint32_t(high)+(std::uint32_t(high)>>31);
}
std::int32_t parity(std::uint32_t value) {
    return std::int32_t(((value&1u)^(value>>31))-(value>>31));
}
}

void render_transfer_unrest(Cpu& c,std::uint32_t address,std::uint32_t mode,std::uint32_t out) {
    c.set(0,c.lr());Frame f(c,0x1008d450,0x6d0,{.first_gpr=13});
    Kept<15> message(c,address);Kept<23> clubs(c,c.toc(-0x7be8));Kept<24> buffer(c,c.toc(-0x7bb8));
    Kept<16> output(c,out);Kept<25> pool(c,c.toc(slot::pool));c.put8(f.at(0x6f3),mode);Program p(c);
    Kept<26> club(c,p.call(0x102cb570,0x1008d484,{message,0}));
    const auto person_id=p.call(0x102cb570,0x1008d494,{message,1});c.set(0,person_id);Kept<20> person(c,person_id);
    for(unsigned field=2;field<=5;++field){const auto value=p.call(0x102cb570,0x1008d4a8+(field-2)*0x14,{message,field});c.set(0,value);c.put32(f.at(0x67c-(field-2)*4),value);}
    auto value=p.call(0x102cb570,0x1008d4f8,{message,6});c.set(0,value);Kept<19> manager_first(c,value);
    value=p.call(0x102cb570,0x1008d50c,{message,7});c.set(0,value);Kept<18> manager_second(c,value);
    value=p.call(0x102cb570,0x1008d520,{message,8});c.set(0,value);Kept<17> manager_third(c,value);
    Kept<27> reasons(c,p.call(0x102cb570,0x1008d534,{message,9}));
    Kept<14> role(c,p.call(0x102cb570,0x1008d544,{message,10}));
    const auto position=p.call(0x102cb570,0x1008d554,{message,11});
    Kept<28> position_gender(c,p.call(0x100d6300,0x1008d568,{extended(std::int16_t(position)),f.at(0x94),120,1}));
    Kept<13> variation(c,p.call(0x102cb570,0x1008d578,{message,12}));
    Kept<29> fee(c,p.call(0x102cb570,0x1008d588,{message,13}));Kept<22> parameter(c,15);
    Kept<30> interested(c,p.call(0x102cb570,0x1008d59c,{message,14}));Kept<31> write_cursor(c,f.at(0x54));Kept<21> read_count(c,0);
    for(;;){const auto index=std::int8_t(std::uint32_t(read_count));c.set(0,std::uint32_t(std::int32_t(index)));if(compare(c,index,std::int32_t(std::uint32_t(interested)))>=0)break;
        const auto field=std::uint32_t(parameter);parameter=parameter+1;
        const auto id=p.call(0x102cb570,0x1008d5c0,{message,field});const auto offset=id*0x248;c.set(3,offset);const auto records=c.u32(clubs);c.set(0,records);
        read_count=read_count+1;c.set(0,records+offset);c.put32(write_cursor,records+offset);write_cursor=write_cursor+4;}
    const auto flag=p.call(0x102cb570,0x1008d5f0,{message,parameter});Kept<15> not_listed(c,std::uint32_t(std::int32_t(std::int8_t(flag))));
    const auto person_name=[&](unsigned offset,unsigned variant,std::uint32_t back){p.call(0x100cb590,back,{c.u32(f.at(0x67c)),c.u32(f.at(0x678)),c.u32(f.at(0x674)),f.at(offset),120,variant,person});};
    person_name(0x274,1,0x1008d614);person_name(0x1fc,5,0x1008d634);
    p.call(0x100cb590,0x1008d654,{manager_first,manager_second,manager_third,f.at(0x184),120,1,c.u32(f.at(0x670))});
    p.call(0x100cb590,0x1008d674,{manager_first,manager_second,manager_third,f.at(0x10c),120,5,c.u32(f.at(0x670))});
    c.put32(f.at(0x50),fee);const auto money_high=p.call(0x10009f20,0x1008d684,{f.at(0x50),6});const auto money_low=c.r(4);
    p.call(0x1000b410,0x1008d69c,{f.at(0x60),50,money_high,money_low,6});
    const auto byte=[&](std::uint32_t value){const auto narrowed=std::int8_t(value);c.set(0,std::uint32_t(std::int32_t(narrowed)));return nonzero(c,narrowed);};
    struct Names {std::uint32_t gender,name,records;};
    const auto names=[&](std::uint32_t record,bool short_name){return Names{std::uint32_t(std::int32_t(c.i8(record+(short_name?0x242:0x23b)))),record+(short_name?0x1ee:0x221),0};};
    const auto own_names=[&]{const auto records=c.u32(clubs);auto n=names(records+std::uint32_t(club)*0x248,true);n.records=records;return n;};
    const auto spill=[&](std::initializer_list<std::uint32_t> words){unsigned offset=0x38;for(auto word:words){c.put32(f.at(offset),word);c.set(0,word);offset+=4;}};
    const auto write=[&](std::uint32_t back){p.call(0x1074b830,back,{output,buffer});};
    const auto append=[&](std::uint32_t back){p.call(0x1074b960,back,{output,buffer});};
    const auto full_byte=std::int8_t(c.u8(f.at(0x6f3)));c.set(0,std::uint32_t(std::int32_t(full_byte)));const bool full=nonzero(c,full_byte);
    if(full){
        c.set(3,0);const bool explain=byte(not_listed);c.put8(f.at(0x4e0),0);
        Kept<22> second_reason(c,f.at(0x350));Kept<31> third_reason(c,f.at(0x3b4));Kept<21> reason_count(c,0);
        for(unsigned offset:{0x2ecu,0x350u,0x3b4u,0x418u,0x47cu})c.put8(f.at(offset),0);
        const auto mask=[&](std::uint32_t bits,unsigned reg=0){const auto value=std::uint32_t(reasons)&bits;c.set(reg,value);return nonzero(c,std::int32_t(value));};
        const auto count=[&](unsigned reg){const auto value=std::int8_t(std::uint32_t(reason_count));c.set(reg,std::uint32_t(std::int32_t(value)));return std::int32_t(value);};
        const auto reason=[&](unsigned text,std::uint32_t back,std::int32_t index){c.set(0,std::uint32_t(index)*100);p.call(0x10727890,back,{f.at(0x2ec)+std::uint32_t(index)*100,pool+text});reason_count=reason_count+1;};
        if(explain){
            if(mask(1)){p.call(0x10727890,0x1008d6ec,{f.at(0x2ec),pool+0x3367});reason_count=1;}
            auto n=count(3);if(compare(c,n,5)<0&&mask(2)){const bool pay=mask(0x01000000);reason(pay?0x3386:0x33b8,pay?0x1008d720:0x1008d738,n);}
            n=count(3);if(compare(c,n,5)<0&&mask(0x100))reason(0x33dd,0x1008d764,n);
            n=count(0);if(compare(c,n,5)<0&&mask(0x10,3)){
                const auto player_remainder=std::uint32_t(person)-third_quotient(person)*3;
                const auto count_remainder=std::uint32_t(interested)-third_quotient(interested)*3;c.set(5,player_remainder);c.set(3,count_remainder);
                unsigned text=0x340a;std::uint32_t back=0x1008d7c8;
                if(compare(c,std::int32_t(player_remainder),std::int32_t(count_remainder))!=0){const auto other=parity(interested);c.set(4,std::uint32_t(interested)>>31);c.set(3,std::uint32_t(other));const bool same=compare(c,std::int32_t(player_remainder),other)==0;text=same?0x342c:0x345c;back=same?0x1008d7f8:0x1008d810;}
                reason(text,back,n);
            }
            n=count(4);if(compare(c,n,5)<0&&mask(0x20)){const auto offset=std::uint32_t(club)*0x248;c.set(0,offset);const auto records=c.u32(clubs);c.set(3,records+offset);
                const auto nation=c.u32(records+offset+0x2c);c.set(7,nation);if(nonzero_logical(c,nation)){const auto gender=std::uint32_t(std::int32_t(c.i8(nation+0x115)));c.set(0,std::uint32_t(n)*100);
                    p.call(0x10727890,0x1008d864,{f.at(0x2ec)+std::uint32_t(n)*100,pool+0x3484,gender,gender,nation+0xae});reason_count=reason_count+1;}}
            n=count(3);if(compare(c,n,5)<0&&mask(0x4000))reason(0x34cd,0x1008d890,n);
            n=count(6);if(compare(c,n,5)<0&&mask(0x20000)){const auto holder=c.toc(-0x7be0);c.set(3,holder);const auto offset=std::uint32_t(person)*112;c.set(0,offset);const auto staff=c.u32(holder);c.set(5,staff+offset);
                const auto nation=c.u32(staff+offset+0xc);c.set(7,nation);const auto gender=std::uint32_t(std::int32_t(c.i8(nation+0x116)));c.set(0,std::uint32_t(n)*100);
                p.call(0x10727890,0x1008d8e0,{f.at(0x2ec)+std::uint32_t(n)*100,pool+0x3517,gender,gender,nation+0xe1});reason_count=reason_count+1;}
            n=count(0);const auto order=compare(c,n,3);
            if(order==0)p.call(0x10727890,0x1008d958,{f.at(0x4e0),pool+0x35a5,f.at(0x2ec),second_reason,third_reason});
            else if(order<0){const auto small=compare(c,n,1);if(small==0)p.call(0x10727890,0x1008d924,{f.at(0x4e0),pool+0x354c,f.at(0x2ec)});else if(small>0)p.call(0x10727890,0x1008d93c,{f.at(0x4e0),pool+0x3574,f.at(0x2ec),second_reason});}
            else {const auto large=compare(c,n,5);if(large==0){const auto high=multiply_high(0x55555556,std::int32_t(std::uint32_t(fee)));c.set(3,std::uint32_t(high));const auto remainder=std::uint32_t(fee)-third_quotient(fee)*3;c.set(0,remainder);const auto order=compare(c,std::int32_t(remainder),1);unsigned text=0x36b0;std::uint32_t back=0x1008d9f8;
                    if(order==0){text=0x3664;back=0x1008d9e8;}else if(order<0&&compare(c,std::int32_t(remainder))>=0){text=0x3618;back=0x1008d9d8;}
                    p.call(0x10727890,back,{f.at(0x4e0),pool+text});
                }else if(large<0){const auto which=parity(person);c.set(7,std::uint32_t(which)*100);c.set(0,std::uint32_t(which+2)*100);
                    p.call(0x10727890,0x1008d994,{f.at(0x4e0),pool+0x35e2,f.at(0x2ec)+std::uint32_t(which)*100,f.at(0x2ec)+std::uint32_t(which+2)*100});}}
        }
        const auto pos_gender=std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(position_gender))));
        if(compare(c,std::int32_t(std::uint32_t(variation)))==0){
            if(byte(reason_count)){const bool key=compare(c,std::int32_t(std::uint32_t(role)),1)==0;const auto own=own_names();spill({f.at(0x274),f.at(0x184),f.at(0x1fc),f.at(0x4e0)});
                p.call(0x10727890,key?0x1008da6c:0x1008dac8,{buffer,pool+(key?0x3706:0x384f),own.gender,own.gender,pos_gender,pos_gender,own.name,f.at(0x94)});
            }else {const bool key=compare(c,std::int32_t(std::uint32_t(role)),1)==0;const bool unlisted=byte(not_listed);const auto own=own_names();spill({f.at(0x274),f.at(0x184)});
                const auto text=key?(unlisted?0x39a1:0x3a82):(unlisted?0x3b4c:0x3c36);const auto back=key?(unlisted?0x1008db24:0x1008db70):(unlisted?0x1008dbc4:0x1008dc10);
                p.call(0x10727890,back,{buffer,pool+text,own.gender,own.gender,pos_gender,pos_gender,own.name,f.at(0x94)});}
            write(0x1008dc1c);
            if(compare(c,std::int32_t(std::uint32_t(interested)),1)==0){const auto n=names(c.u32(f.at(0x54)),false);p.call(0x10727890,0x1008dc48,{buffer,pool+0x3cff,n.gender,n.gender,n.name,f.at(0x60)});append(0x1008dc54);}
            else if(compare(c,std::int32_t(std::uint32_t(interested)),2)==0){const auto second=names(c.u32(f.at(0x58)),false),first=names(c.u32(f.at(0x54)),false);spill({second.name});
                p.call(0x10727890,0x1008dc9c,{buffer,pool+0x3d9c,first.gender,first.gender,second.gender,second.gender,f.at(0x60),first.name});append(0x1008dca8);}
            else if(compare(c,std::int32_t(std::uint32_t(interested)),3)==0){const auto third=names(c.u32(f.at(0x5c)),false),second=names(c.u32(f.at(0x58)),false),first=names(c.u32(f.at(0x54)),false);
                spill({f.at(0x60),first.name,second.name,third.name});p.call(0x10727890,0x1008dd10,{buffer,pool+0x3e61,first.gender,first.gender,second.gender,second.gender,third.gender,third.gender});append(0x1008dd1c);}
            else {const auto high=multiply_high(0x2aaaaaab,std::int32_t(std::uint32_t(fee)));c.set(3,std::uint32_t(high));const auto q=std::uint32_t(high)+(std::uint32_t(high)>>31),remainder=std::uint32_t(fee)-q*6;c.set(0,remainder);const bool manager=compare(c,std::int32_t(remainder),2)>0;
                p.call(0x10727890,manager?0x1008dd58:0x1008dd70,{buffer,pool+(manager?0x3f42:0x3fa0),f.at(manager?0x10c:0x1fc),f.at(0x60)});append(0x1008dd7c);}
        }else {
            bool lead=compare(c,std::int32_t(std::uint32_t(interested)))!=0;
            if(lead){const auto difference=std::uint32_t(fee)-std::uint32_t(reasons);c.set(3,difference>>31);const auto remainder=parity(difference);c.set(0,std::uint32_t(remainder));lead=nonzero(c,remainder);}
            if(lead){const bool unlisted=byte(not_listed);const auto candidate=names(c.u32(f.at(0x54)),false),own=own_names();
                if(unlisted)spill({candidate.name,f.at(0x60),own.name,f.at(0x94),f.at(0x274)});
                else {c.set(11,f.at(0x94));spill({candidate.name,f.at(0x60),f.at(0x274),own.name,f.at(0x94),f.at(0x184)});}
                p.call(0x10727890,unlisted?0x1008de10:0x1008de84,{buffer,pool+(unlisted?0x4008:0x410c),candidate.gender,candidate.gender,own.gender,own.gender,pos_gender,pos_gender});write(0x1008de90);
            }else {
                if(!byte(not_listed)){const auto own=own_names();spill({f.at(0x94),f.at(0x274),f.at(0x60)});p.call(0x10727890,0x1008deec,{buffer,pool+0x4231,own.gender,own.gender,pos_gender,pos_gender,own.name,f.at(0x184)});}
                else if(compare(c,std::int32_t(std::uint32_t(role)),1)==0){const auto own=own_names();spill({f.at(0x60),f.at(0x274),f.at(0x94)});p.call(0x10727890,0x1008df48,{buffer,pool+0x430d,own.gender,own.gender,pos_gender,pos_gender,own.name,f.at(0x184)});}
                else if(byte(reason_count)){const auto own=own_names();spill({f.at(0x10c),f.at(0x60),f.at(0x4e0)});p.call(0x10727890,0x1008dfa4,{buffer,pool+0x4415,own.gender,own.gender,100,own.name,f.at(0x184),f.at(0x274)});}
                else if(compare(c,std::int32_t(std::uint32_t(reasons)))!=0){const auto own=own_names();c.set(11,own.records);spill({f.at(0x184),f.at(0x274),f.at(0x10c),f.at(0x60),f.at(0x94)});p.call(0x10727890,0x1008e010,{buffer,pool+0x4599,own.gender,own.gender,100,pos_gender,pos_gender,own.name});}
                else {const auto own=own_names();spill({f.at(0x10c),f.at(0x60)});p.call(0x10727890,0x1008e05c,{buffer,pool+0x46e1,own.gender,own.gender,100,own.name,f.at(0x184),f.at(0x274)});}
                write(0x1008e068);
                if(compare(c,std::int32_t(std::uint32_t(interested)),1)==0){const auto n=names(c.u32(f.at(0x54)),false);p.call(0x10727890,0x1008e094,{buffer,pool+0x47ff,n.gender,n.gender,n.name,f.at(0x1fc)});append(0x1008e0a0);}
                else if(compare(c,std::int32_t(std::uint32_t(interested)),2)>=0){const auto second=names(c.u32(f.at(0x58)),false),first=names(c.u32(f.at(0x54)),false);p.call(0x10727890,0x1008e0e0,{buffer,pool+0x4887,first.gender,first.gender,second.gender,second.gender,first.name,second.name});append(0x1008e0ec);}
            }
        }
    }else {
        const auto high=multiply_high(0x66666667,std::int32_t(std::uint32_t(reasons))),floor=high>>1;
        const auto q=std::uint32_t(floor)+(std::uint32_t(floor)>>31),product=std::uint32_t(person)*(std::uint32_t(variation)+q);
        const auto selector=std::int32_t(product)%4;c.set(4,0x66666667);c.set(3,product>>31);c.set(0,std::uint32_t(selector));c.s.wr<1>(guest::reg::xer_ca,high<0&&(std::uint32_t(high)&1));
        const auto order=compare(c,selector,1);bool generic=false;
        if(order==0){const auto own=own_names();c.set(0,own.records);p.call(0x10727890,0x1008e1c8,{buffer,pool+0x4965,own.gender,own.gender,f.at(0x1fc),own.name});}
        else if(order<0){if(compare(c,selector)>=0){bool particular=false;if(!byte(not_listed)){const auto high=multiply_high(0x66666667,std::int32_t(std::uint32_t(person))),floor=high>>3;const auto sign=std::uint32_t(floor)>>31;c.set(3,sign);const auto remainder=std::uint32_t(person)-(std::uint32_t(floor)+sign)*20;c.set(0,remainder);c.s.wr<1>(guest::reg::xer_ca,high<0&&(std::uint32_t(high)&7));particular=compare(c,std::int32_t(remainder),5)>0;}
                p.call(0x10727890,particular?0x1008e180:0x1008e194,{buffer,pool+(particular?0x490f:0x493c),f.at(0x1fc)});
            }else generic=true;
        }else if(compare(c,selector,3)<0){const bool unlisted=byte(not_listed);if(unlisted)p.call(0x10727890,0x1008e1ec,{buffer,pool+0x49b3,100,f.at(0x10c),f.at(0x1fc)});else p.call(0x10727890,0x1008e204,{buffer,pool+0x4a0a,f.at(0x10c),f.at(0x1fc)});}
        else generic=true;
        if(generic)p.call(0x10727890,0x1008e218,{buffer,pool+0x4a5d,f.at(0x1fc)});
        write(0x1008e224);
    }
    const auto club_offset=std::uint32_t(club)*0x248;c.set(0,club_offset);const auto records=c.u32(clubs);c.set(4,records);
    p.call(0x1074bab0,0x1008e240,{output,records+club_offset+0x1ee,0x7d5,records+club_offset});
    const auto holder=c.toc(-0x7be0);c.set(3,holder);Kept<13> offset(c,std::uint32_t(person)*112);const auto staff=c.u32(holder);c.set(0,staff);
    auto found=p.call(0x100cbde0,0x1008e264,{staff+std::uint32_t(offset),c.u32(f.at(0x67c)),c.u32(f.at(0x678)),c.u32(f.at(0x674)),0});
    if(byte(found)){for(unsigned i=0;i<2;++i){const auto fresh_holder=c.toc(-0x7be0);c.set(3,fresh_holder);const auto fresh=c.u32(fresh_holder);c.set(0,fresh);p.call(0x1074bab0,i==0?0x1008e288:0x1008e2a4,{output,f.at(i==0?0x274:0x1fc),0x7d7,fresh+std::uint32_t(offset)});}}
    const auto manager=c.u32(f.at(0x670));c.set(0,manager);const auto manager_holder=c.toc(-0x7be0);c.set(3,manager_holder);offset=manager*112;const auto manager_staff=c.u32(manager_holder);c.set(0,manager_staff);
    found=p.call(0x100cbde0,0x1008e2cc,{manager_staff+std::uint32_t(offset),manager_first,manager_second,manager_third,0});
    if(byte(found)){const auto fresh_holder=c.toc(-0x7be0);c.set(3,fresh_holder);const auto fresh=c.u32(fresh_holder);c.set(0,fresh);p.call(0x1074bab0,0x1008e2f0,{output,f.at(0x184),0x7d7,fresh+std::uint32_t(offset)});}
    offset=f.at(0x54);Kept<15> index(c,0);
    for(;;){const auto count=std::int8_t(std::uint32_t(index));c.set(0,std::uint32_t(std::int32_t(count)));if(compare(c,count,std::int32_t(std::uint32_t(interested)))>=0)break;
        Kept<14> candidate(c,c.u32(offset));p.call(0x1074bab0,0x1008e314,{output,candidate+0x1ee,0x7d5,candidate});p.call(0x1074bab0,0x1008e328,{output,candidate+0x221,0x7d5,candidate});offset=offset+4;index=index+1;}
}
}
