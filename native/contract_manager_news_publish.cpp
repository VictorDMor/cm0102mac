#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
// The loan notification inlines this lookup three times. Keep the repeated
// reads: message callbacks may change the index or the underlying table.
std::uint32_t loan_contract(Cpu& c,std::uint32_t person,std::uint32_t manager_holder,
                           std::uint32_t index_holder,std::uint32_t count_holder,
                           std::uint32_t cache,bool already_tested=false) {
    if(!already_tested)compare_logical(c,person);
    const auto manager=c.u32(manager_holder);c.set(4,manager);
    std::uint32_t row=0;
    if(person) {
        const auto id=c.u32(person);c.set(3,id);const auto count=c.u32(count_holder);c.set(0,count);
        if(compare(c,std::int32_t(id),std::int32_t(count))<0) {
            const auto current_id=c.u32(person);c.set(0,current_id);const auto indices=c.u32(index_holder);c.set(3,indices);
            const auto offset=current_id*100;c.set(0,offset);const auto index=c.u32(indices+offset);c.set(0,index);
            if(compare(c,std::int32_t(index))>=0){const auto displacement=index*100;c.set(0,displacement);const auto rows=c.u32(manager);c.set(3,rows);row=rows+displacement;}
        }
    }
    c.set(3,row);compare_logical(c,row);c.put32(cache,row);return row;
}
}

void publish_loan_return(Cpu& c,std::uint32_t person_address,std::uint32_t related_address) {
    c.set(0,c.lr());Frame f(c,0x10091630,0x140,{.first_gpr=24});
    Kept<27> manager(c,c.toc(-0x7b7c));Kept<24> person(c,person_address);nonzero(c,std::int32_t(person_address));
    Kept<26> news(c,c.toc(-0x7bf0));Kept<25> related(c,related_address);Kept<28> cache(c,c.toc(-0x7b78));
    Kept<29> clubs(c,c.toc(-0x7be8));Kept<30> index(c,c.toc(-0x7b9c));Kept<31> count(c,c.toc(-0x7be4));Program p(c);
    const auto owner=[&](bool first,bool result_in_five) {
        const auto row=loan_contract(c,person,manager,index,count,cache,first);std::uint32_t club=0;
        if(row){const auto id=c.u32(row+4);c.set(0,id);const auto records=c.u32(clubs);c.set(3,records);const auto offset=id*0x248;c.set(0,offset);club=records+offset;}
        c.set(first?0:result_in_five?5:3,club);return club;
    };
    if(!nonzero_logical(c,owner(true,false)))return;
    p.call(0x102bdf90,0x100916f4,{f.at(0x38),0xfb8,0});
    p.call(0x102cb4b0,0x10091704,{f.at(0x38),0,c.u32(person)});
    for(unsigned field=1;field<=3;++field){const auto competition=c.u32(person+0x24+field*4);p.call(0x102cb4b0,0x10091704+field*0x14,{f.at(0x38),field,c.u32(competition+0x20)});}
    const auto current=c.u32(person+0x24);c.set(5,current);
    p.call(0x102cb4b0,0x10091754,{f.at(0x38),4,c.u32(current)});
    const auto lender=owner(false,false);p.call(0x102cb4b0,0x100917e4,{f.at(0x38),5,c.u32(lender)});
    p.call(0x102cb4b0,0x100917f4,{f.at(0x38),6,c.u32(related)});
    const auto employer=c.u32(person+0x24);c.set(5,employer);
    if(compare_logical(c,related,employer)==0) {
        const auto recipient=owner(false,true);
        p.call(0x102cc140,0x1009188c,{c.u32(news),f.at(0x38),recipient});
    } else p.call(0x102cc140,0x1009189c,{c.u32(news),f.at(0x38),employer});
}

void publish_assurance_request(Cpu& c,std::uint32_t contract_address) {
    c.set(0,c.lr());Frame f(c,0x1008fa40,0x160,{.first_gpr=28});
    Kept<31> club(c,c.toc(-0x7ba8));Kept<30> person(c,c.toc(slot::pool));Kept<29> contract(c,contract_address);
    const bool present=nonzero(c,std::int32_t(contract_address));Kept<28> registry(c,c.toc(-0x7b40));Program p(c);
    if(!present) {
        p.call(0x10739890,0x1008fa88,{person+0x37,0,0,f.at(0x40),f.at(0x38)});
        const auto version=p.call(0x10768810,0x1008fa8c);
        p.call(0x1077b720,0x1008faa8,{club,person+0x23b,version,f.at(0x40),f.at(0x38),0x2bf2});
        p.call(0x10769b60,0x1008fab8,{person+0x248,club,0});
        const auto success=c.toc(-0x7bac);c.set(3,success);c.set(0,0);c.put8(success,0);return;
    }
    const auto person_id=c.u32(contract);c.set(0,person_id);const auto club_id=c.u32(contract+4);c.set(4,club_id);
    const auto staff_holder=c.toc(-0x7be0);c.set(3,staff_holder);const auto person_offset=person_id*112;c.set(0,person_offset);
    const auto clubs_holder=c.toc(-0x7be8);c.set(6,clubs_holder);const auto staff=c.u32(staff_holder);c.set(3,staff);
    const auto club_offset=club_id*0x248;c.set(5,club_offset);const auto clubs=c.u32(clubs_holder);c.set(6,clubs);
    person=staff+person_offset;const auto system=c.u32(registry);c.set(3,system);club=clubs+club_offset;
    auto blocked=p.call(0x103e6470,0x1008fafc,{system,person});const auto byte=std::int8_t(blocked);c.set(0,std::uint32_t(std::int32_t(byte)));
    if(nonzero(c,byte))return;
    blocked=p.call(0x103eaea0,0x1008fb18,{c.u32(registry),person,0,0});if(compare(c,std::int32_t(blocked))!=0)return;
    blocked=p.call(0x103eb020,0x1008fb30,{c.u32(registry),person,0});if(compare(c,std::int32_t(blocked))!=0)return;
    const auto manager_holder=c.toc(-0x7b7c);c.set(3,manager_holder);const bool valid=nonzero_logical(c,person);
    const auto manager=c.u32(manager_holder);c.set(5,manager);std::uint32_t primary=0;
    if(valid) {
        const auto count_holder=c.toc(-0x7be4);c.set(3,count_holder);const auto id=c.u32(person);c.set(4,id);const auto count=c.u32(count_holder);c.set(0,count);
        if(compare(c,std::int32_t(id),std::int32_t(count))<0) {
            const auto current_id=c.u32(person);c.set(0,current_id);const auto holder=c.toc(-0x7b9c);c.set(3,holder);c.set(0,current_id*100);
            const auto indices=c.u32(holder);c.set(3,indices);const auto row=c.u32(indices+current_id*100);c.set(0,row);
            if(compare(c,std::int32_t(row))>=0){c.set(0,row*100);const auto rows=c.u32(manager);c.set(3,rows);primary=rows+row*100;}
        }
    }
    c.set(4,primary);const auto cache=c.toc(-0x7b78);c.set(3,cache);const bool found=nonzero_logical(c,primary);c.put32(cache,primary);
    bool nonplaying=false;
    if(found){const auto raw=c.u8(primary+0x38);const auto role=std::int32_t(std::int8_t(raw))>>2;c.set(0,std::uint32_t(role));c.s.wr<1>(guest::reg::xer_ca,role<0&&(raw&2));const auto difference=5u-std::uint32_t(role);c.set(0,difference);c.s.wr<1>(guest::reg::xer_ca,5u>=std::uint32_t(role));nonplaying=role==5;}
    c.set(0,nonplaying);if(compare(c,nonplaying)!=0)return;
    p.call(0x102bdf90,0x1008fbf0,{f.at(0x60),0xbc1,0});
    p.call(0x102cb4b0,0x1008fc00,{f.at(0x60),0,c.u32(club)});
    p.call(0x102cb4b0,0x1008fc10,{f.at(0x60),2,c.u32(person)});
    for(unsigned field=3;field<=5;++field){const auto competition=c.u32(person+0x1c+field*4);p.call(0x102cb4b0,0x1008fc10+(field-2)*0x14,{f.at(0x60),field,c.u32(competition+0x20)});}
    p.call(0x102cb4b0,0x1008fc5c,{f.at(0x60),6,0});p.call(0x102cb4b0,0x1008fc6c,{f.at(0x60),7,0});
    p.call(0x102cb4b0,0x1008fc80,{f.at(0x60),8,std::uint32_t(std::int32_t(c.i8(contract+0x5c)))});
    p.call(0x102cb4b0,0x1008fc94,{f.at(0x60),9,std::uint32_t(std::int32_t(c.i8(contract+0x5c)))});
    p.call(0x102cb4b0,0x1008fca4,{f.at(0x60),10,0});
    const auto age=c.i16(person+0x5c);c.set(0,extended(age));
    p.call(0x102cb4b0,0x1008fcb8,{f.at(0x60),11,std::uint32_t(std::int32_t(std::int8_t(age)))});
    const auto news_holder=c.toc(-0x7bf0);c.set(3,news_holder);p.call(0x102cc140,0x1008fccc,{c.u32(news_holder),f.at(0x60),club});
}
}
