#include "contract_manager_news_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
void publish_new_contract_request(Cpu& c,std::uint32_t address) {
    c.set(0,c.lr());Frame f(c,0x10090310,0x160,{.first_gpr=28});
    Kept<31> contract(c,address);const bool present=nonzero(c,std::int32_t(address));
    Kept<30> person(c,c.toc(slot::pool));Kept<29> club(c,c.toc(-0x7ba8));Kept<28> registry(c,c.toc(-0x7b40));Program p(c);
    if(!present){p.call(0x10739890,0x10090358,{person+0x37,0,0,f.at(0x40),f.at(0x38)});const auto version=p.call(0x10768810,0x1009035c);
        p.call(0x1077b720,0x10090378,{club,person+0x23b,version,f.at(0x40),f.at(0x38),0x2d4d});
        p.call(0x10769b60,0x10090388,{person+0x248,club,0});const auto success=c.toc(-0x7bac);c.set(3,success);c.set(0,0);c.put8(success,0);return;}
    const auto person_id=c.u32(contract);c.set(0,person_id);const auto club_id=c.u32(contract+4);c.set(4,club_id);
    const auto staff_holder=c.toc(-0x7be0);c.set(3,staff_holder);c.set(0,person_id*112);const auto clubs_holder=c.toc(-0x7be8);c.set(6,clubs_holder);
    const auto staff=c.u32(staff_holder);c.set(3,staff);c.set(5,club_id*0x248);const auto clubs=c.u32(clubs_holder);c.set(6,clubs);
    person=staff+person_id*112;const auto system=c.u32(registry);c.set(3,system);club=clubs+club_id*0x248;
    auto blocked=p.call(0x103e6470,0x100903cc,{system,person});const auto byte=std::int8_t(blocked);c.set(0,std::uint32_t(std::int32_t(byte)));if(nonzero(c,byte))return;
    blocked=p.call(0x103eaea0,0x100903e8,{c.u32(registry),person,0,0});if(compare(c,std::int32_t(blocked))!=0)return;
    blocked=p.call(0x103eb020,0x10090400,{c.u32(registry),person,0});if(compare(c,std::int32_t(blocked))!=0)return;
    p.call(0x102bdf90,0x1009041c,{f.at(0x60),0xbc2,0});p.call(0x102cb4b0,0x1009042c,{f.at(0x60),0,c.u32(club)});
    p.call(0x102cb4b0,0x1009043c,{f.at(0x60),2,c.u32(person)});
    for(unsigned field=3;field<=5;++field){const auto competition=c.u32(person+0x1c+field*4);p.call(0x102cb4b0,0x1009043c+(field-2)*0x14,{f.at(0x60),field,c.u32(competition+0x20)});}
    const auto raw_role=c.u8(contract+0x60);c.set(0,raw_role);p.call(0x102cb4b0,0x1009048c,{f.at(0x60),6,raw_role&15u});
    p.call(0x102cb4b0,0x1009049c,{f.at(0x60),7,0});p.call(0x102cb4b0,0x100904ac,{f.at(0x60),8,extended(c.i16(contract+0x30))});
    p.call(0x102cb4b0,0x100904bc,{f.at(0x60),9,c.u16(contract+0x32)});p.call(0x102cb4b0,0x100904cc,{f.at(0x60),10,c.u32(contract+0xc)});
    auto trait=c.i8(person+0x65);c.set(0,std::uint32_t(std::int32_t(trait)));bool agent=false;
    if(compare(c,trait,10)<0){trait=c.i8(person+0x67);c.set(0,std::uint32_t(std::int32_t(trait)));
        if(compare(c,trait,5)<0){const auto attributes=c.u32(person+0x14);c.set(3,attributes);
            if(nonzero_logical(c,attributes)){const auto reputation=c.i16(attributes+0xc);c.set(0,extended(reputation));agent=compare(c,reputation,4750)>0;}}}
    p.call(0x102cb4b0,agent?0x10090514:0x10090528,{f.at(0x60),11,agent});
    const auto news_holder=c.toc(-0x7bf0);c.set(3,news_holder);p.call(0x102cc140,0x1009053c,{c.u32(news_holder),f.at(0x60),club});
}

void render_new_contract_request(Cpu& c,std::uint32_t manager_address,std::uint32_t address,std::uint32_t mode,std::uint32_t out) {
    c.set(0,c.lr());Frame f(c,0x10090560,0x1c0,{.first_gpr=16});
    Kept<19> message(c,address);Kept<18> manager(c,manager_address);Kept<16> news_system(c,c.toc(-0x7bf0));Kept<21> agent(c,mode);
    Kept<26> context(c,c.toc(-0x7bb0));Kept<20> output(c,out);Kept<27> buffer(c,c.toc(-0x7bb8));Kept<28> staff(c,c.toc(-0x7be0));Kept<29> pool(c,c.toc(slot::pool));Program p(c);
    Kept<30> club(c,p.call(0x102cb570,0x100905a0,{message,0}));
    auto value=p.call(0x102cb570,0x100905b0,{message,2});c.set(0,value);Kept<25> person(c,value);
    value=p.call(0x102cb570,0x100905c4,{message,3});c.set(0,value);Kept<24> first(c,value);
    value=p.call(0x102cb570,0x100905d8,{message,4});c.set(0,value);Kept<23> second(c,value);
    value=p.call(0x102cb570,0x100905ec,{message,5});c.set(0,value);Kept<22> third(c,value);
    Kept<31> role(c,p.call(0x102cb570,0x10090600,{message,6}));
    const auto validation=p.call(0x102cb570,0x10090610,{message,7});Kept<17> validation_or_year(c,std::uint32_t(std::int32_t(std::int8_t(validation))));
    const auto has_agent=p.call(0x102cb570,0x10090620,{message,11});const auto display=std::int8_t(mode);c.set(0,std::uint32_t(std::int32_t(display)));
    const bool full=nonzero(c,display);agent=std::uint32_t(std::int32_t(std::int8_t(has_agent)));
    if(full){const auto skip=std::int8_t(std::uint32_t(validation_or_year));c.set(0,std::uint32_t(std::int32_t(skip)));
        if(!nonzero(c,skip)){
            validation_or_year=p.call(0x102cb570,0x10090640,{message,9})&0xffff;
            const auto day=p.call(0x102cb570,0x10090650,{message,8})&0xffff;c.set(0,day);
            p.call(0x107156b0,0x10090664,{f.at(0x40),extended(std::int16_t(day)),validation_or_year});
            const auto wage=p.call(0x102cb570,0x10090670,{message,10});c.set(0,std::uint32_t(person)*112);
            const auto records=c.u32(staff);c.set(4,records);const auto manager_holder=c.toc(-0x7b7c);c.set(5,manager_holder);
            const auto player=records+std::uint32_t(person)*112;c.set(4,player);const bool present=nonzero(c,std::int32_t(player));
            const auto global_manager=c.u32(manager_holder);c.set(7,global_manager);std::uint32_t contract=0;
            if(present){const auto count_holder=c.toc(-0x7be4);c.set(5,count_holder);const auto id=c.u32(player);c.set(6,id);const auto count=c.u32(count_holder);c.set(0,count);
                if(compare(c,std::int32_t(id),std::int32_t(count))<0){const auto fresh_id=c.u32(player);c.set(0,fresh_id);const auto holder=c.toc(-0x7b9c);c.set(5,holder);c.set(0,fresh_id*100);const auto indices=c.u32(holder);c.set(5,indices);
                    const auto index=c.u32(indices+fresh_id*100);c.set(0,index);
                    if(compare(c,std::int32_t(index))>=0){c.set(0,index*100);const auto rows=c.u32(global_manager);c.set(5,rows);contract=rows+index*100;}}}
            c.set(7,contract);bool matches=false;
            if(nonzero_logical(c,contract)){const auto current_wage=c.u32(contract+0xc);c.set(0,current_wage);
                if(compare(c,std::int32_t(current_wage),std::int32_t(wage))==0){const auto date_word=c.u32(f.at(0x40));c.set(0,date_word);c.set(5,0);const auto tail=c.u32(f.at(0x44));c.set(6,tail);c.put32(f.at(0x38),date_word);
                    const auto year=c.u16(contract+0x32);c.set(0,year);const auto wanted_year=c.u16(f.at(0x3a));c.set(3,wanted_year);c.put32(f.at(0x3c),tail);
                    if(compare_logical(c,wanted_year,year)==0){const auto wanted_day=c.i16(f.at(0x38));c.set(3,extended(wanted_day));const auto current_day=c.i16(contract+0x30);c.set(0,extended(current_day));if(compare(c,wanted_day,current_day)==0){matches=true;c.set(5,1);}}
                    compare(c,matches);
                }}
            const auto mark_request_flag=[&](std::uint32_t back){const auto selected=c.i16(context+0x2c58);c.set(0,extended(selected));c.set(0,std::uint32_t(std::int32_t(selected))*0x2c0);
                p.call(0x102cd0c0,back,{c.u32(news_system),c.u32(context+std::uint32_t(std::int32_t(selected))*0x2c0+4),c.u32(message+0xdc),7,1,message});};
            if(!matches)mark_request_flag(0x1009075c);
            else {const auto list=c.u32(manager+0x34);c.set(3,list);validation_or_year=c.u32(player);std::uint32_t found=0;
                if(nonzero_logical(c,list)){const auto head=c.u32(list+4);c.set(0,head);c.put32(list+0xc,head);
                    for(;;){const auto row=p.call(0x10738e00,0x100907a4,{c.u32(manager+0x34)});if(!nonzero_logical(c,row))break;
                        const auto id=c.u32(row);c.set(0,id);if(compare(c,std::int32_t(id),std::int32_t(std::uint32_t(validation_or_year)))!=0)continue;
                        const auto employer=c.u32(row+4);c.set(0,employer);if(compare(c,std::int32_t(employer),std::int32_t(std::uint32_t(club)))==0){found=row;break;}}}
                c.set(3,found);if(nonzero_logical(c,found))mark_request_flag(0x100907e0);
            }
        }
    }
    p.call(0x100cb590,full?0x10090800:0x1009098c,{first,second,third,f.at(0x48),0x12c,full?1u:5u,person});
    if(full){const auto byte=std::int8_t(std::uint32_t(agent));c.set(0,std::uint32_t(std::int32_t(byte)));const bool represented=nonzero(c,byte);
        unsigned selected=3;const auto value=std::int32_t(std::uint32_t(role));const auto middle=compare(c,value,2);
        if(middle==0)selected=1;else if(middle<0){if(compare(c,value,1)>=0)selected=0;}else if(compare(c,value,4)<0)selected=2;
        constexpr unsigned with_agent[]={0x6539,0x6604,0x66d3,0x67a0},without_agent[]={0x6855,0x6904,0x69b7,0x6a68};
        constexpr std::uint32_t with_back[]={0x10090840,0x10090858,0x10090870,0x10090888},without_back[]={0x100908c0,0x100908d4,0x100908e8,0x100908fc};
        if(represented)p.call(0x10727890,with_back[selected],{buffer,pool+with_agent[selected],0x64,f.at(0x48)});
        else p.call(0x10727890,without_back[selected],{buffer,pool+without_agent[selected],f.at(0x48)});
        p.call(0x1074b830,0x10090908,{output,buffer});const auto holder=c.toc(-0x7be8);c.set(4,holder);const auto offset=std::uint32_t(club)*0x248;c.set(0,offset);const auto records=c.u32(holder);c.set(4,records);
        p.call(0x1074bab0,0x10090928,{output,records+offset+0x1ee,0x7d5,records+offset});
    }else{p.call(0x10727890,0x1009099c,{buffer,pool+0x6af4,f.at(0x48)});p.call(0x1074b830,0x100909a8,{output,buffer});}
    news_system=std::uint32_t(person);news::person_link(c,p,f,news_system,staff,output,first,second,third,0x48,full?0x10090948:0x100909c8,full?0x10090968:0x100909e8);
}
}
