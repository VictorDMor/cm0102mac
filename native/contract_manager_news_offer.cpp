#include "contract_manager_random.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
// Contract kind occupies the signed high six bits. Only bit one can be
// shifted out after the original mask, including when the kind is negative.
std::int32_t offer_kind(Cpu& c,std::uint32_t flags,unsigned destination) {
    const auto kind=std::int32_t(std::int8_t(flags))>>2;
    if(destination==5)c.set(0,(std::uint32_t(flags)<<24)&0xfe000000u);
    c.set(destination,std::uint32_t(kind));
    c.s.wr<1>(guest::reg::xer_ca,(flags&0x80)&&(flags&2));
    return kind;
}

bool expiry_after(Cpu& c,const Frame& f,std::uint32_t expiry,std::uint32_t cutoff,bool first) {
    std::uint32_t first_word,second_word;
    if(first) {
        first_word=c.u32(expiry);c.set(3,first_word);second_word=c.u32(expiry+4);c.set(0,second_word);
        c.put32(f.at(0x4c),first_word);
        const auto year=c.u16(cutoff+2),expiry_year=c.u16(f.at(0x4e));
        c.set(4,year);c.set(5,expiry_year);
        c.put32(f.at(0x3c),first_word);compare_logical(c,expiry_year,year);
        c.put32(f.at(0x40),second_word);c.put32(f.at(0x50),second_word);
        if(expiry_year!=year){c.set(0,expiry_year>year);return nonzero(c,expiry_year>year);}
    } else {
        first_word=c.u32(f.at(0x3c));c.set(3,first_word);
        second_word=c.u32(f.at(0x40));c.set(0,second_word);c.put32(f.at(0x44),first_word);
        const auto year=c.u16(cutoff+2),expiry_year=c.u16(f.at(0x46));
        c.set(3,year);c.set(4,expiry_year);c.put32(f.at(0x48),second_word);
        compare_logical(c,expiry_year,year);
        if(expiry_year!=year){c.set(0,expiry_year>year);return nonzero(c,expiry_year>year);}
    }
    const auto day=c.i16(f.at(first?0x4c:0x44)),cutoff_day=c.i16(cutoff);
    c.set(4,extended(day));c.set(0,extended(cutoff_day));
    const auto difference=extended(day)^extended(cutoff_day);
    c.set(3,std::uint32_t(std::int32_t(difference)>>1));
    c.s.wr<1>(guest::reg::xer_ca,std::int32_t(difference)<0&&(difference&1));
    c.set(0,day>cutoff_day);return nonzero(c,day>cutoff_day);
}
}

void publish_contract_offer_result(Cpu& c,std::uint32_t owner_address,
                                   std::uint32_t proposal_address,std::uint32_t notify_value) {
    c.set(0,c.lr());Frame f(c,0x1008a350,0x220,{.first_gpr=18});
    Kept<23> registry(c,c.toc(-0x7b40));Kept<20> primary(c,owner_address);
    Kept<24> system(c,c.toc(-0x7bf0));Kept<18> proposal(c,proposal_address);
    Kept<25> clubs(c,c.toc(-0x7be8));Kept<19> notify(c,notify_value);
    Kept<26> salt(c,c.toc(-0x7b90));Kept<27> cursor(c,c.toc(-0x7b84));
    Kept<28> errors(c,c.toc(-0x7ba8));Kept<29> later_cutoff(c,c.toc(-0x79b8));
    Kept<30> first_cutoff(c,c.toc(-0x79a8));Kept<31> pool(c,c.toc(slot::pool));Program p(c);
    p.call(0x10738c00,0x1008a398,{f.at(0x5c)});
    const auto person_id=c.u32(proposal),club_id=c.u32(proposal+4);c.set(4,person_id);c.set(3,primary);c.set(0,club_id);
    const auto staff_holder=c.toc(-0x7be0);c.set(6,staff_holder);const auto person_offset=person_id*112;c.set(5,person_offset);
    const auto club_records=c.u32(clubs);c.set(4,club_records);const auto staff=c.u32(staff_holder);c.set(6,staff);c.set(0,club_id*0x248);
    Kept<22> person(c,staff+person_offset);Kept<21> club(c,club_records+club_id*0x248);
    const auto contract=p.call(0x10082f90,0x1008a3cc,{primary,person,club});c.set(0,contract);primary=contract;
    const auto message=f.at(0x104);
    p.call(0x102bdf90,0x1008a3e4,{message,0xbb8,0});
    p.call(0x102cb4b0,0x1008a3f4,{message,0,c.u32(person)});
    for(unsigned field=1;field<=3;++field){const auto competition=c.u32(person+0x24+field*4);p.call(0x102cb4b0,0x1008a3f4+field*0x14,{message,field,c.u32(competition+0x20)});}
    p.call(0x102cb4b0,0x1008a440,{message,4,c.u32(proposal+4)});
    p.call(0x102cb4b0,0x1008a454,{message,5,std::uint32_t(std::int32_t(c.i8(proposal+0x64)))});
    p.call(0x102cb4b0,0x1008a464,{message,6,extended(c.i16(proposal+0x30))});
    p.call(0x102cb4b0,0x1008a474,{message,7,c.u16(proposal+0x32)});
    p.call(0x102cb4b0,0x1008a484,{message,8,c.u32(proposal+0x70)});
    offer_kind(c,c.u8(proposal+0x38),5);p.call(0x102cb4b0,0x1008a49c,{message,10});
    const auto reason=p.call(0x100c7a20,0x1008a4a4,{c.u8(proposal+0x78)});const auto reason_byte=std::int8_t(reason);c.set(0,std::uint32_t(std::int32_t(reason_byte)));
    const bool familiar=nonzero(c,reason_byte);
    p.call(0x102cb4b0,familiar?0x1008a4bc:0x1008a4d0,{message,11,familiar?0u:1u});
    const auto origin=c.u32(proposal+0x70);c.set(4,origin);
    if(compare(c,std::int32_t(origin),-1)!=0) {
        const auto holder=p.call(0x103eb440,0x1008a4e4,{c.u32(registry),origin});const auto source=c.u32(holder);c.set(3,source);
        if(!nonzero_logical(c,source)) {
            p.call(0x10739890,0x1008a508,{pool+0x37,0,0,f.at(0x6c),f.at(0x54)});
            const auto version=p.call(0x10768810,0x1008a50c);
            p.call(0x1077b720,0x1008a528,{errors,pool+0x23b,version,f.at(0x6c),f.at(0x54),0x2094});
            p.call(0x10769b60,0x1008a538,{pool+0x248,errors,0});
            const auto success=c.toc(-0x7bac);c.set(5,success);c.set(0,0);c.put8(success,0);
            p.call(0x10738c20,0x1008a550,{f.at(0x5c),0xffffffff});return;
        }
        p.call(0x102cb4b0,0x1008a564,{message,9,c.u32(source+0x2c)});
    }
    const auto flags=c.u8(proposal+0x38);c.set(3,flags);c.set(4,person);const auto registry_object=c.u32(registry);c.set(6,registry_object);c.set(5,club);
    const auto role=c.u8(proposal+0x60);c.set(0,role);const auto kind=offer_kind(c,flags,7);
    const auto shortlist=c.u32(registry_object+0x988);
    p.call(0x10369080,0x1008a594,{shortlist,person,club,0,std::uint32_t(kind),role&15u,f.at(0x38)});
    p.call(0x102cb4b0,0x1008a5a4,{message,12,c.u32(f.at(0x38))});
    const auto manager=c.u32(club+0x90);c.set(3,manager);
    if(nonzero_logical(c,manager)) {
        p.call(0x102cb4b0,0x1008a5c0,{message,13,c.u32(manager)});
        for(unsigned field=14;field<=16;++field){const auto fresh=c.u32(club+0x90);c.set(5,fresh);const auto competition=c.u32(fresh+0x28+(field-14)*4);p.call(0x102cb4b0,0x1008a5c0+(field-13)*0x18,{message,field,c.u32(competition+0x20)});}
    } else for(unsigned field=13;field<=16;++field)p.call(0x102cb4b0,0x1008a61c+(field-13)*0x10,{message,field,0xffffffff});
    const auto notify_byte=std::int8_t(std::uint32_t(notify));c.set(0,std::uint32_t(std::int32_t(notify_byte)));
    std::uint32_t timing=0,back=0x1008a764;
    if(nonzero(c,notify_byte)) {
        if(!expiry_after(c,f,primary+0x30,first_cutoff,true)){timing=999;back=0x1008a6c8;}
        else if(!expiry_after(c,f,primary+0x30,later_cutoff,false)){timing=1000;back=0x1008a738;}
        else {notify=0;back=0x1008a750;}
    }
    p.call(0x102cb4b0,back,{message,17,timing});
    p.call(0x102cb4b0,0x1008a778,{message,18,0x420e9});
    const auto end_holder=c.toc(-0x7b88);c.set(3,end_holder);const auto old_cursor=c.u32(cursor);c.set(4,old_cursor);const auto end=c.u32(end_holder);c.set(0,end);
    const auto next=old_cursor+2;c.set(3,next);const bool wraps=compare_logical(c,next,end)>0;c.put32(cursor,next);
    if(wraps){const auto base=c.toc(-0x7b8c);c.set(0,base);c.put32(cursor,base);const auto entropy=p.call(0x1077b770,0x1008a7a0);c.set(0,entropy&0xffff);c.put32(salt,entropy&0xffff);}
    const auto position=c.u32(cursor);c.set(4,position);c.set(3,0x55550000);const auto random_salt=c.u32(salt);c.set(5,random_salt);c.set(0,0x55555556);
    const auto value=c.u16(position);c.set(6,value);const auto draw=value+random_salt;const auto high=multiply_high(0x55555556,std::int32_t(draw));
    const auto quotient=std::uint32_t(high)+(std::uint32_t(high)>>31);c.set(6,draw);c.set(0,quotient*3);
    p.call(0x102cb4b0,0x1008a7e0,{message,19,draw-quotient*3});
    bool renewal=false;
    if(nonzero_logical(c,club)&&nonzero_logical(c,primary)){const auto outcome=c.u8(proposal+0x64);c.set(0,outcome);renewal=compare(c,outcome,2)==0;}
    p.call(0x102cb4b0,0x1008a810,{message,20,renewal});
    p.call(0x102cb4b0,0x1008a824,{message,21,std::uint32_t(std::int32_t(c.i8(proposal+0x78)))});
    p.call(0x102cb4b0,0x1008a834,{message,22,c.u32(proposal+0xc)});
    p.call(0x102cb4b0,0x1008a844,{message,23,c.u32(proposal+8)});
    p.call(0x102cb4b0,0x1008a854,{message,24,extended(c.i16(proposal+0x28))});
    p.call(0x102cb4b0,0x1008a864,{message,25,c.u16(proposal+0x2a)});
    const auto attributes=c.u32(person+0x14);c.set(3,attributes);bool retiring=false;std::uint32_t retirement_back=0x1008a9a4;
    if(nonzero_logical(c,attributes)) {
        if(compare(c,offer_kind(c,c.u8(proposal+0x38),0),2)==0) {
            const auto goalkeeper=c.u8(attributes+0xf);c.set(0,goalkeeper);
            if(compare(c,goalkeeper,20)==0){const auto age=p.call(0x100c6a10,0x1008a8a0,{person,proposal+0x30});c.set(0,std::uint32_t(std::int32_t(std::int8_t(age))));retiring=compare(c,std::int8_t(age),37)>0;if(retiring)retirement_back=0x1008a8bc;}
            for(unsigned position_index=0;position_index<3&&!retiring;++position_index){
                const auto fresh=c.u32(person+0x14);c.set(3,fresh);const auto proficiency=c.i8(fresh+0x11+position_index*2);c.set(0,std::uint32_t(std::int32_t(proficiency)));
                if(compare(c,proficiency,15)>=0){const auto age=p.call(0x100c6a10,0x1008a8e0+position_index*0x40,{person,proposal+0x30});c.set(0,std::uint32_t(std::int32_t(std::int8_t(age))));retiring=compare(c,std::int8_t(age),36-std::int32_t(position_index))>0;if(retiring)retirement_back=0x1008a8fc+position_index*0x40;}
            }
            if(!retiring)retirement_back=0x1008a990;
        }
    }
    p.call(0x102cb4b0,retirement_back,{message,26,retiring});
    const auto id=c.u32(person);c.set(0,id);const auto index_holder=c.toc(-0x7b9c);c.set(6,index_holder);c.set(5,id*100);const auto indices=c.u32(index_holder);c.set(6,indices);c.set(0,id*100+12);
    const auto index_flags=c.u8(indices+id*100+12);c.set(0,index_flags);
    p.call(0x102cb4b0,0x1008a9cc,{message,27,index_flags&1u});
    const auto notification=std::int8_t(std::uint32_t(notify));c.set(0,std::uint32_t(std::int32_t(notification)));bool distribute=nonzero(c,notification);
    if(!distribute&&nonzero_logical(c,club)&&nonzero_logical(c,primary)) {
        const auto days=p.call(0x10374d10,0x1008a9ec,{primary});c.set(0,extended(std::int16_t(days)));
        if(compare(c,std::int16_t(days),180)<0){const auto outcome=c.u8(proposal+0x64);c.set(0,outcome);distribute=compare(c,outcome,2)==0;}
    }
    if(distribute) {
        const auto registry_object=c.u32(registry);c.set(3,registry_object);std::uint32_t recipients=0;
        if(nonzero_logical(c,registry_object)){const auto traits=c.u32(person+0x14);c.set(0,traits);if(nonzero_logical(c,traits))recipients=p.call(0x103fc710,0x1008aa30,{registry_object,person,0x21a,f.at(0x8c),30});else c.set(3,0);}else c.set(3,0);
        Kept<19> count(c,std::uint32_t(std::int32_t(std::int8_t(recipients))));Kept<18> recipient(c,f.at(0x8c));Kept<20> delivered(c,0);
        for(;;){const auto index=std::int8_t(std::uint32_t(delivered));c.set(0,std::uint32_t(std::int32_t(index)));if(compare(c,index,std::int32_t(std::uint32_t(count)))>=0)break;
            p.call(0x102cc140,0x1008aa58,{c.u32(system),message,c.u32(recipient)});recipient=recipient+4;delivered=delivered+1;}
    } else {
        const auto id=c.u32(proposal+4);c.set(0,id);const auto records=c.u32(clubs);c.set(5,records);c.set(0,id*0x248);
        p.call(0x102cc140,0x1008aa8c,{c.u32(system),message,records+id*0x248});
        const auto registry_object=c.u32(registry);c.set(3,registry_object);
        p.call(0x10362ef0,0x1008aaac,{c.u32(registry_object+0x988),person,f.at(0x5c),1,1,0});
        const auto outcome=c.u8(proposal+0x64);c.set(0,outcome);
        if(compare(c,outcome,2)==0&&compare(c,offer_kind(c,c.u8(proposal+0x38),0),2)==0) {
            const auto head=c.u32(f.at(0x60));c.set(0,head);c.put32(f.at(0x68),head);
            for(;;){const auto row=p.call(0x10738e00,0x1008ab0c,{f.at(0x5c)});if(!nonzero_logical(c,row))break;
                const auto id=c.u32(row+4);c.set(3,id);const auto own=c.u32(proposal+4);c.set(0,own);if(compare(c,std::int32_t(id),std::int32_t(own))==0)continue;
                const auto offset=id*0x248;c.set(0,offset);const auto records=c.u32(clubs);c.set(5,records);p.call(0x102cc140,0x1008ab04,{c.u32(system),message,records+offset});}
        }
    }
    p.call(0x10738c20,0x1008ab20,{f.at(0x5c),0xffffffff});
}
}
