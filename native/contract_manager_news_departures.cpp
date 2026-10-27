#include "contract_manager_news_detail.hpp"
#include "contract_manager_news_primary.hpp"
#include <bit>

namespace cm::aot::contract_manager {
using namespace detail;
void publish_departure_status(Cpu& c,std::uint32_t owner_address,std::uint32_t person_address,
                              std::uint32_t kind_value,std::uint32_t important_value) {
    c.set(0,c.lr());Frame f(c,0x10090a00,0x160,{.first_gpr=21});
    Kept<27> employer(c,c.toc(-0x7b7c));Kept<22> person(c,person_address);nonzero(c,std::int32_t(person_address));
    Kept<25> registry(c,c.toc(-0x7b40));Kept<21> owner(c,owner_address);Kept<26> system(c,c.toc(-0x7bf0));
    Kept<23> kind(c,kind_value);Kept<28> cache(c,c.toc(-0x7b78));Kept<24> important(c,important_value);
    Kept<29> clubs(c,c.toc(-0x7be8));Kept<30> index(c,c.toc(-0x7b9c));Kept<31> count(c,c.toc(-0x7be4));Program p(c);
    const auto club_for=[&](bool first,unsigned result_register){
        const auto row=news::primary(c,person,employer,index,count,cache,first);std::uint32_t club=0;
        if(row){const auto id=c.u32(row+4);c.set(0,id);const auto records=c.u32(clubs);c.set(3,records);c.set(0,id*0x248);club=records+id*0x248;}
        c.set(result_register,club);return club;};
    if(!nonzero_logical(c,club_for(true,0)))return;
    p.call(0x10738c00,0x10090ac8,{f.at(0x38)});p.call(0x102bdf90,0x10090ad8,{f.at(0x48),0xbb9,0});
    p.call(0x102cb4b0,0x10090b68,{f.at(0x48),0,c.u32(club_for(false,3))});
    p.call(0x102cb4b0,0x10090b78,{f.at(0x48),2,c.u32(person)});
    for(unsigned field=3;field<=5;++field){const auto competition=c.u32(person+0x1c+field*4);p.call(0x102cb4b0,0x10090b78+(field-2)*0x14,{f.at(0x48),field,c.u32(competition+0x20)});}
    p.call(0x102cb4b0,0x10090bc4,{f.at(0x48),6,std::uint32_t(std::int32_t(std::int8_t(kind_value)))});
    const auto current=club_for(false,27);employer=current;const auto event_kind=std::int8_t(std::uint32_t(kind));c.set(0,std::uint32_t(std::int32_t(event_kind)));
    if(compare(c,event_kind,6)==0){const auto row=news::primary(c,person,0,index,count,0,false,owner);
        p.call(0x102cb4b0,0x10090cb8,{f.at(0x48),7,c.u32(row+4)});
    }else p.call(0x102cb4b0,0x10090ccc,{f.at(0x48),7,0xffffffff});
    p.call(0x102cb4b0,0x10090cdc,{f.at(0x48),8,std::uint32_t(std::int32_t(std::int8_t(important_value)))});
    const auto needs_attention=[&](unsigned offset){const auto byte=std::int8_t(std::uint32_t(important));c.set(0,std::uint32_t(std::int32_t(byte)));
        if(!nonzero(c,byte))return false;const auto attributes=c.u32(person+0x14);c.set(3,attributes);if(!nonzero_logical(c,attributes))return false;
        const auto reputation=c.i16(attributes+offset);c.set(0,extended(reputation));return compare(c,reputation,7000)>0;};
    if(needs_attention(0xc))p.call(0x102cc8e0,0x10090d08,{c.u32(system),f.at(0x48)});
    else {
        const bool international=needs_attention(0xa);
        if(international){p.call(0x102cc400,0x10090d3c,{c.u32(system),f.at(0x48),c.u32(person+0xc)});
            const auto nationality=c.u32(person+0xc);c.set(0,nationality);const auto home=c.u32(employer+0x2c);c.set(5,home);
            if(compare_logical(c,nationality,home)!=0)p.call(0x102cc400,0x10090d58,{c.u32(system),f.at(0x48)});
            const auto club=c.u32(person+0x24);c.set(3,club);
            if(nonzero_logical(c,club)&&compare_logical(c,club,employer)!=0){const auto division=c.u32(club+0x2c);c.set(5,division);const auto owner_division=c.u32(employer+0x2c);c.set(0,owner_division);
                if(compare_logical(c,division,owner_division)!=0){const auto nationality=c.u32(person+0xc);c.set(0,nationality);if(compare_logical(c,division,nationality)!=0)p.call(0x102cc400,0x10090d94,{c.u32(system),f.at(0x48)});}}
        } else {
            p.call(0x102cc140,0x10090e54,{c.u32(system),f.at(0x48),employer});
            const auto byte=std::int8_t(std::uint32_t(kind));c.set(0,std::uint32_t(std::int32_t(byte)));
            if(compare(c,byte,6)==0)p.call(0x102cc140,0x10090e70,{c.u32(system),f.at(0x48),c.u32(person+0x24)});
        }
        const auto registry_object=c.u32(registry);c.set(3,registry_object);
        p.call(0x10362ef0,international?0x10090db4:0x10090e90,{c.u32(registry_object+0x988),person,f.at(0x38),1,1,0});
        const auto head=c.u32(f.at(0x3c));c.set(0,head);
        if(!international)owner=std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(kind))));c.put32(f.at(0x44),head);
        for(;;){const auto row=p.call(0x10738e00,international?0x10090e38:0x10090eec,{f.at(0x38)});if(!nonzero_logical(c,row))break;
            const auto id=c.u32(row+4);c.set(4,id);const auto employer_id=c.u32(employer);c.set(0,employer_id);if(compare(c,std::int32_t(id),std::int32_t(employer_id))==0)continue;
            if(international){const auto byte=std::int8_t(std::uint32_t(kind));c.set(0,std::uint32_t(std::int32_t(byte)));if(compare(c,byte,6)==0){const auto club=c.u32(person+0x24);c.set(3,club);const auto current_id=c.u32(club);c.set(0,current_id);if(compare(c,std::int32_t(id),std::int32_t(current_id))==0)continue;}}
            else if(compare(c,std::int32_t(std::uint32_t(owner)),6)==0){const auto club=c.u32(person+0x24);c.set(3,club);const auto current_id=c.u32(club);c.set(0,current_id);if(compare(c,std::int32_t(id),std::int32_t(current_id))==0)continue;}
            if(international){const auto offset=id*0x248;c.set(3,offset);const auto records=c.u32(clubs);c.set(4,records);const auto nationality=c.u32(person+0xc);c.set(0,nationality);
                const auto target=records+offset;c.set(5,target);const auto division=c.u32(target+0x2c);c.set(4,division);if(compare_logical(c,division,nationality)==0)continue;
                const auto club=c.u32(person+0x24);c.set(3,club);const auto other=c.u32(club+0x2c);c.set(0,other);if(compare_logical(c,division,other)==0)continue;
                const auto home=c.u32(employer+0x2c);c.set(0,home);if(compare_logical(c,division,home)==0)continue;
                p.call(0x102cc140,0x10090e30,{c.u32(system),f.at(0x48)});
            } else {const auto offset=id*0x248;c.set(0,offset);const auto records=c.u32(clubs);c.set(5,records);p.call(0x102cc140,0x10090ee4,{c.u32(system),f.at(0x48),records+offset});}
        }
    }
    p.call(0x10738c20,0x10090f00,{f.at(0x38),0xffffffff});
}

void render_departure_status(Cpu& c,std::uint32_t address,std::uint32_t mode,std::uint32_t out) {
    c.set(0,c.lr());Frame f(c,0x10090f20,0x1b0,{.first_gpr=17});
    Kept<17> message(c,address);Kept<24> clubs(c,c.toc(-0x7be8));Kept<25> context_or_flag(c,c.toc(-0x7bb0));Kept<18> display(c,mode);
    Kept<26> buffer(c,c.toc(-0x7bb8));Kept<31> output(c,out);Kept<27> staff(c,c.toc(-0x7be0));Kept<28> pool(c,c.toc(slot::pool));Program p(c);
    Kept<29> club(c,p.call(0x102cb570,0x10090f5c,{message,0}));
    auto value=p.call(0x102cb570,0x10090f6c,{message,2});c.set(0,value);Kept<23> person(c,value);
    value=p.call(0x102cb570,0x10090f80,{message,3});c.set(0,value);Kept<22> first(c,value);
    value=p.call(0x102cb570,0x10090f94,{message,4});c.set(0,value);Kept<21> second(c,value);
    value=p.call(0x102cb570,0x10090fa8,{message,5});c.set(0,value);Kept<20> third(c,value);
    value=p.call(0x102cb570,0x10090fbc,{message,6});Kept<19> kind(c,std::uint32_t(std::int32_t(std::int8_t(value))));
    Kept<30> lender(c,p.call(0x102cb570,0x10090fcc,{message,7}));
    const auto importance=p.call(0x102cb570,0x10090fdc,{message,8});const auto selected=c.i16(context_or_flag+0x2c58);c.set(0,extended(selected));
    const auto difference=1u-importance;c.set(3,difference);c.s.wr<1>(guest::reg::xer_ca,1u>=importance);c.set(3,difference==0?32u:std::countl_zero(difference));
    c.set(0,std::uint32_t(std::int32_t(selected))*0x2c0);c.set(4,importance==1);const auto context=std::uint32_t(context_or_flag)+std::uint32_t(std::int32_t(selected))*0x2c0;c.set(3,context);
    context_or_flag=importance==1;const auto manager=c.u32(context+4);c.set(3,manager);bool own_club=false;
    if(nonzero_logical(c,manager)){const auto team=c.u32(manager+0x24);c.set(3,team);if(nonzero_logical(c,team)){const auto id=c.u32(team);c.set(0,id);const auto delta=id-std::uint32_t(club);c.set(0,delta);c.set(0,delta==0?32u:std::countl_zero(delta));c.set(0,delta==0);own_club=delta==0;}}
    message=own_club;p.call(0x100cb590,0x1009104c,{first,second,third,f.at(0x38),0x12c,1,person});
    const auto full_byte=std::int8_t(mode);c.set(0,std::uint32_t(std::int32_t(full_byte)));const bool full=nonzero(c,full_byte);
    const auto format_club=[&](std::uint32_t id,unsigned text,std::uint32_t back,bool extra_count){const auto offset=id*0x248;c.set(6,offset);const auto records=c.u32(clubs);c.set(0,records);
        const auto gender=std::uint32_t(std::int32_t(c.i8(records+offset+0x23b))),name=records+offset+0x221;
        if(extra_count)p.call(0x10727890,back,{buffer,pool+text,gender,gender,0x64,name,f.at(0x38)});
        else p.call(0x10727890,back,{buffer,pool+text,gender,gender,f.at(0x38),name});};
    if(!full){const auto flagged=std::int8_t(std::uint32_t(context_or_flag));c.set(0,std::uint32_t(std::int32_t(flagged)));bool leaving=false;
        if(nonzero(c,flagged)){const auto own=std::int8_t(std::uint32_t(message));c.set(0,std::uint32_t(std::int32_t(own)));leaving=!nonzero(c,own);}
        if(leaving)format_club(club,0x6b24,0x10091094,false);
        else if(compare(c,std::int32_t(std::uint32_t(kind)),5)==0)p.call(0x10727890,0x100910b4,{buffer,pool+0x6b70,0x64,f.at(0x38)});
        else if(compare(c,std::int32_t(std::uint32_t(kind)),6)==0)p.call(0x10727890,0x100910d4,{buffer,pool+0x6ba5,0x64,f.at(0x38)});
        else {const bool free=compare(c,std::int32_t(std::uint32_t(kind)),1)==0;bool flagged_now=false;
            if(!free){c.set(0,std::uint32_t(context_or_flag));flagged_now=nonzero(c,std::int8_t(std::uint32_t(context_or_flag)));}
            if(free||flagged_now)p.call(0x10727890,0x100910f8,{buffer,pool+0x6bd9,f.at(0x38)});
            else p.call(0x10727890,0x10091110,{buffer,pool+0x6c08,0x64,f.at(0x38)});}
    } else {
        if(compare(c,std::int32_t(std::uint32_t(kind)),5)==0)p.call(0x10727890,0x1009119c,{buffer,pool+0x6c3f,f.at(0x38)});
        else if(compare(c,std::int32_t(std::uint32_t(kind)),6)==0)format_club(lender,0x6c96,0x100911d8,false);
        else if(compare(c,std::int32_t(std::uint32_t(kind)),1)==0)p.call(0x10727890,0x100911f8,{buffer,pool+0x6d09,0x64,f.at(0x38)});
        else {c.set(0,std::uint32_t(context_or_flag));if(nonzero(c,std::int8_t(std::uint32_t(context_or_flag)))){
                c.set(0,std::uint32_t(message));if(nonzero(c,std::int8_t(std::uint32_t(message))))p.call(0x10727890,0x10091220,{buffer,pool+0x6d93,0x64,f.at(0x38)});
                else format_club(club,0x6dfe,0x10091258,true);
            }else p.call(0x10727890,0x10091270,{buffer,pool+0x6e8c,0x64,f.at(0x38)});}
    }
    p.call(0x1074b830,full?0x1009127c:0x1009111c,{output,buffer});
    message=std::uint32_t(person);news::person_link(c,p,f,message,staff,output,first,second,third,0x38,full?0x1009129c:0x1009113c,full?0x100912bc:0x1009115c,0x7da);
    const auto link=[&](std::uint32_t id,std::uint32_t back){const auto offset=id*0x248;c.set(0,offset);const auto records=c.u32(clubs);c.set(4,records);p.call(0x1074bab0,back,{output,records+offset+0x221,0x7d5,records+offset});};
    if(full&&compare(c,std::int32_t(std::uint32_t(lender)),-1)!=0)link(lender,0x100912e0);
    if(compare(c,std::int32_t(std::uint32_t(club)),-1)!=0)link(club,full?0x10091304:0x10091180);
}
}
