#include "contract_manager_random.hpp"
#include <bit>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
std::uint32_t quotient(Cpu& c,std::int32_t value,std::int32_t multiplier,unsigned shift) {
    const auto high=multiply_high(multiplier,value),floor=high>>shift;
    c.s.wr<1>(guest::reg::xer_ca,high<0&&(std::uint32_t(high)&((1u<<shift)-1)));
    return std::uint32_t(floor)+(std::uint32_t(floor)>>31);
}
}

void publish_transfer_unrest(Cpu& c,std::uint32_t owner_address,std::uint32_t person_address,
                              std::uint32_t club_address) {
    c.set(0,c.lr());Frame f(c,0x1008cd20,0x200,{.first_gpr=17});
    Kept<27> person(c,person_address);const bool present=nonzero(c,std::int32_t(person_address));
    Kept<19> finance(c,c.toc(-0x7ab8));Kept<30> registry(c,c.toc(-0x7b40));Kept<17> owner(c,owner_address);
    Kept<31> system(c,c.toc(-0x7bf0));Kept<18> club(c,club_address);Kept<20> clubs(c,c.toc(-0x7be8));
    Kept<21> constants(c,c.toc(-0x571c));Kept<22> salt(c,c.toc(-0x7b90));Kept<23> base(c,c.toc(-0x7b8c));
    Kept<24> end(c,c.toc(-0x7b88));Kept<25> cursor(c,c.toc(-0x7b84));Program p(c);guest::Fpu fp(c);
    if(!present)return;
    const auto attributes=c.u32(person+0x14);c.set(0,attributes);if(!nonzero_logical(c,attributes))return;
    if(!nonzero_logical(c,club))return;const auto manager=c.u32(club+0x90);c.set(0,manager);if(!nonzero_logical(c,manager))return;
    Kept<29> selected(c,0);Kept<26> contract(c,p.call(0x10082f90,0x1008cd90,{owner,person,club}));
    p.call(0x10738c00,0x1008cd9c,{f.at(0x38)});
    if(!nonzero_logical(c,contract)){p.call(0x10738c20,0x1008cdb0,{f.at(0x38),0xffffffff});return;}
    const auto message=f.at(0xc0);p.call(0x102bdf90,0x1008cdc4,{message,0xbea,0});
    p.call(0x102cb4b0,0x1008cdd4,{message,0,c.u32(club)});
    p.call(0x102cb4b0,0x1008cde4,{message,1,c.u32(person)});
    for(unsigned field=2;field<=4;++field){const auto competition=c.u32(person+0x20+field*4);p.call(0x102cb4b0,0x1008cde4+(field-1)*0x14,{message,field,c.u32(competition+0x20)});}
    const auto manager_now=c.u32(club+0x90);c.set(5,manager_now);p.call(0x102cb4b0,0x1008ce34,{message,5,c.u32(manager_now)});
    for(unsigned field=6;field<=8;++field){const auto current=c.u32(club+0x90);c.set(5,current);const auto competition=c.u32(current+0x10+field*4);p.call(0x102cb4b0,0x1008ce34+(field-5)*0x18,{message,field,c.u32(competition+0x20)});}
    p.call(0x102cb4b0,0x1008ce8c,{message,9,c.u32(contract+0x50)});
    const auto raw_role=c.u8(contract+0x60);c.set(0,raw_role);p.call(0x102cb4b0,0x1008cea0,{message,10,raw_role&15u});
    const auto position=p.call(0x10117490,0x1008cea8,{c.u32(person+0x14)});
    p.call(0x102cb4b0,0x1008ceb8,{message,11,extended(std::int16_t(position))});
    const auto advance=[&](std::uint32_t back,bool second){const auto position=c.u32(cursor);c.set(second?4:3,position);const auto limit=c.u32(end);c.set(0,limit);
        const auto next=position+2;c.set(3,next);const bool wraps=compare_logical(c,next,limit)>0;c.put32(cursor,next);
        if(wraps){c.put32(cursor,base);const auto entropy=p.call(0x1077b770,back);c.set(0,entropy&0xffff);c.put32(salt,entropy&0xffff);}};
    advance(0x1008ced8,false);
    const auto first_position=c.u32(cursor);c.set(5,first_position);const auto first_salt=c.u32(salt);c.set(0,first_salt);
    const auto first_word=c.u16(first_position);c.set(5,first_word);const auto first_draw=first_word+first_salt;
    const auto sign=first_draw>>31;const auto parity=(first_draw&1u)^sign;c.set(0,parity);
    p.call(0x102cb4b0,0x1008cf0c,{message,12,parity-sign});
    const auto listed=p.call(0x100d3040,0x1008cf18,{person,club});const auto listed_byte=std::int8_t(listed);
    c.set(0,std::uint32_t(std::int32_t(listed_byte)));c.set(0,listed_byte==0?32u:std::countl_zero(std::uint32_t(std::int32_t(listed_byte))));c.set(0,listed_byte==0);
    Kept<28> not_listed(c,listed_byte==0);
    const auto human=p.call(0x1014c2e0,0x1008cf40,{club,0,1,0,0});const auto human_byte=std::int8_t(human);c.set(0,std::uint32_t(std::int32_t(human_byte)));bool direct=false;
    if(nonzero(c,human_byte)){c.set(0,std::uint32_t(not_listed));if(!nonzero(c,std::int8_t(std::uint32_t(not_listed))))direct=true;
        else {const auto role=c.u8(contract+0x60)&15u;c.set(0,role);direct=compare(c,std::int32_t(role),1)==0;}}
    Kept<22> fee(c,c.r(22));
    if(direct)fee=c.u32(person+8);
    else {
        Kept<26> quote(c,p.call(0x10085860,0x1008cf88,{owner,club,0,person,0,1,1}));advance(0x1008cfac,true);
        const auto position=c.u32(cursor);c.set(4,position);c.set(5,0x43300000);c.set(3,0x63e70000);
        const auto encoded_quote=std::uint32_t(quote)^0x80000000u;c.set(6,encoded_quote);const auto word=c.u16(position);c.set(4,word);c.set(0,0x63e7063f);
        const auto fresh_salt=c.u32(salt);c.set(3,fresh_salt);c.put32(f.at(0x1ac),0x43300000);const auto draw=word+fresh_salt;c.set(4,draw);
        const auto bias=c.f64(constants+8);c.set_f(4,bias);const auto high=multiply_high(0x63e7063f,std::int32_t(draw));c.set(0,std::uint32_t(high));
        const auto hundred=c.f64(constants+0x228);c.set_f(0,hundred);c.put32(f.at(0x1b8),encoded_quote);const auto low=c.f64(constants+0x170);c.set_f(2,low);c.put32(f.at(0x1b4),0x43300000);
        const auto floor=high>>4;const auto sign=std::uint32_t(floor)>>31;c.set(0,std::uint32_t(floor));c.set(3,sign);c.s.wr<1>(guest::reg::xer_ca,high<0&&(std::uint32_t(high)&15));
        const auto converted_quote=c.f64(f.at(0x1b4));c.set_f(1,converted_quote);const auto q=std::uint32_t(floor)+sign;c.set(0,q*41);
        const auto quote_amount=fp.fsub(converted_quote,bias);c.set_f(3,quote_amount);const auto remainder=draw-q*41,encoded=remainder^0x80000000u;c.set(0,encoded);c.put32(f.at(0x1b0),encoded);
        const auto converted_draw=c.f64(f.at(0x1ac));c.set_f(1,converted_draw);const auto variation=fp.fsub(converted_draw,bias);c.set_f(1,variation);
        const auto fraction=fp.fdiv(variation,hundred);c.set_f(0,fraction);const auto scale=fp.fadd(low,fraction);c.set_f(0,scale);const auto amount=fp.fmul(quote_amount,scale);c.set_f(0,amount);c.s.pc=0x1008d028;
        c.put64(f.at(0x1a4),fp.fctiwz(0,amount));fee=c.u32(f.at(0x1a8));
        bool small=false;
        if(compare(c,std::int32_t(std::uint32_t(fee)),10000)>0){c.set(3,0x30000);c.set(0,200000);small=compare(c,std::int32_t(std::uint32_t(fee)),200000)<0;}
        if(small){const auto q=quotient(c,std::int32_t(std::uint32_t(fee)),0x68db8bad,12);c.set(0,q);c.set(3,q>>31);fee=q*10000;}
        else {c.set(3,0xf0000);c.set(4,1000000);if(compare(c,std::int32_t(std::uint32_t(fee)),1000000)<0){const auto q=quotient(c,std::int32_t(std::uint32_t(fee)),0x14f8b589,13);c.set(0,100000);fee=q*100000;}
            else {c.set(3,0x4c0000);c.set(0,5000000);if(compare(c,std::int32_t(std::uint32_t(fee)),5000000)<0){const auto q=quotient(c,std::int32_t(std::uint32_t(fee)),0x431bde83,16);c.set(0,250000);fee=q*250000;}
                else {const auto q=quotient(c,std::int32_t(std::uint32_t(fee)),0x431bde83,18);c.set(0,q);fee=q*1000000;}}}
    }
    p.call(0x102cb4b0,0x1008d10c,{message,13,fee});c.set(3,0x80000);c.set(0,500000);
    if(compare(c,std::int32_t(std::uint32_t(fee)),500000)<0){p.call(0x10738c20,0x1008d128,{f.at(0x38),0xffffffff});return;}
    const auto object=c.u32(registry);c.set(3,object);p.call(0x10362ef0,0x1008d14c,{c.u32(object+0x988),person,f.at(0x38),1,1,0});
    const auto head=c.u32(f.at(0x3c));c.set(0,head);c.put32(f.at(0x44),head);
    for(;;){const auto next=p.call(0x10738e00,0x1008d220,{f.at(0x38)});owner=next;if(!nonzero(c,std::int32_t(next)))break;
        const auto count=std::int8_t(std::uint32_t(selected));c.set(0,std::uint32_t(std::int32_t(count)));if(compare(c,count,30)>=0)break;
        const auto id=c.u32(owner+4);c.set(0,id);const auto records=c.u32(clubs);c.set(3,records);c.set(0,id*0x248);
        const auto human=p.call(0x1014c2e0,0x1008d17c,{records+id*0x248,0,1,0,0});const auto byte=std::int8_t(human);c.set(0,std::uint32_t(std::int32_t(byte)));bool affordable=true;
        if(nonzero(c,byte)){const auto fresh_id=c.u32(owner+4);c.set(0,fresh_id);const auto fresh_records=c.u32(clubs);c.set(4,fresh_records);c.set(0,fresh_id*0x248);
            const auto account=p.call(0x10103510,0x1008d19c,{c.u32(finance),fresh_records+fresh_id*0x248});if(!nonzero_logical(c,account))continue;
            p.call(0x100f5770,0x1008d1bc,{account,0,0,0,1,0});p.call(0x10773c80,0x1008d1c0);
            const auto balance=c.f(1);const auto encoded=std::uint32_t(fee)^0x80000000u;c.set(3,encoded);c.set(0,0x43300000);c.put32(f.at(0x1a8),encoded);
            const auto bias=c.f64(constants+8);c.set_f(2,bias);c.put32(f.at(0x1a4),0x43300000);const auto markup=c.f64(constants+0x28);c.set_f(3,markup);
            const auto converted=c.f64(f.at(0x1a4));c.set_f(0,converted);const auto amount=fp.fsub(converted,bias);c.set_f(0,amount);const auto needed=fp.fmul(markup,amount);c.set_f(0,needed);fp.fcmp(0,balance,needed);affordable=!(balance<needed);
        }
        if(!affordable)continue;
        const auto fresh_id=c.u32(owner+4);c.set(0,fresh_id);if(compare(c,std::int32_t(fresh_id),-1)<=0)continue;
        const auto offset=fresh_id*0x248;c.set(4,offset);const auto index=std::int8_t(std::uint32_t(selected));c.set(0,std::uint32_t(std::int32_t(index)));const auto fresh_records=c.u32(clubs);c.set(5,fresh_records);c.set(3,f.at(0x48));c.set(0,std::uint32_t(std::int32_t(index))*4);
        c.set(4,fresh_records+offset);c.put32(f.at(0x48)+std::uint32_t(std::int32_t(index))*4,fresh_records+offset);selected=selected+1;
    }
    Kept<20> count(c,std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(selected)))));const auto array=f.at(0x48);
    c.set(10,array);c.set(3,array);c.set(11,0);c.set(0,std::uint32_t(count)-1);
    // Reproduce the original pairwise swaps, including their order for ties.
    for(std::uint32_t first=0;;++first){const auto outer=std::int8_t(first);c.set(4,std::uint32_t(std::int32_t(outer)));if(compare(c,outer,std::int32_t(std::uint32_t(count)-1))>=0)break;
        const auto first_at=array+first*4;std::uint32_t other=std::uint32_t(std::int32_t(outer))+1;c.set(4,other);c.set(12,std::uint32_t(std::int32_t(std::int8_t(other))));c.set(7,other*4);c.set(4,array+other*4);
        for(;;){const auto index=std::int8_t(other);c.set(5,std::uint32_t(std::int32_t(index)));if(compare(c,index,std::int32_t(std::uint32_t(count)))>=0)break;
            const auto left=c.u32(first_at),right=c.u32(array+other*4);c.set(8,left);c.set(9,right);const auto left_reputation=c.i16(left+0x1ec),right_reputation=c.i16(right+0x1ec);c.set(6,extended(left_reputation));c.set(5,extended(right_reputation));
            if(compare(c,left_reputation,right_reputation)<0){c.put32(first_at,right);c.put32(array+other*4,left);}
            ++other;c.set(7,other*4);c.set(4,array+other*4);c.set(12,other);
        }
        c.set(3,first_at+4);c.set(11,first+1);
    }
    const auto top_order=compare(c,std::int32_t(std::uint32_t(count)),3);Kept<22> next_field(c,15);
    p.call(0x102cb4b0,0x1008d2cc,{message,14,top_order<0?std::uint32_t(count):3u});
    Kept<21> candidate(c,0);Kept<18> bound(c,std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(selected)))));Kept<19> offset(c,0);Kept<17> row(c,array);
    for(;;){const auto order=compare(c,std::int32_t(std::uint32_t(bound)),3);const auto index=std::int8_t(std::uint32_t(candidate));c.set(4,std::uint32_t(std::int32_t(index)));const auto limit=order<0?std::uint32_t(count):3u;c.set(0,limit);if(compare(c,index,std::int32_t(limit))>=0)break;
        c.set(5,std::uint32_t(std::int32_t(index)));const auto second=compare(c,index,1);
        if(second==0){c.set(3,array);const auto first=c.u32(array);c.set(0,first);const auto current=c.u32(array+std::uint32_t(offset));c.set(3,current);
            if(compare_logical(c,current,first)==0){const auto field=std::uint32_t(next_field)-std::uint32_t(std::int32_t(index))-1;c.set(0,field);p.call(0x102cb4b0,0x1008d318,{message,std::uint32_t(std::int32_t(std::int8_t(field))),std::uint32_t(std::int32_t(index))});break;}}
        c.set(0,std::uint32_t(std::int32_t(index)));
        if(compare(c,index,2)==0){c.set(3,array);const auto first=c.u32(array);c.set(0,first);const auto current=c.u32(array+std::uint32_t(offset));c.set(3,current);bool duplicate=compare_logical(c,current,first)==0;
            if(!duplicate){const auto second=c.u32(array+4);c.set(0,second);duplicate=compare_logical(c,current,second)==0;}
            if(duplicate){const auto field=std::uint32_t(next_field)-std::uint32_t(std::int32_t(index))-1;c.set(0,field);p.call(0x102cb4b0,0x1008d364,{message,std::uint32_t(std::int32_t(std::int8_t(field))),std::uint32_t(std::int32_t(index))});break;}}
        const auto current=c.u32(row);c.set(5,current);const auto field=std::uint32_t(next_field);next_field=next_field+1;p.call(0x102cb4b0,0x1008d384,{message,field,c.u32(current)});
        offset=offset+4;row=row+4;candidate=candidate+1;
    }
    p.call(0x102cb4b0,0x1008d3bc,{message,next_field,not_listed});
    const auto registry_object=c.u32(registry);c.set(3,registry_object);std::uint32_t recipients=0;
    if(nonzero_logical(c,registry_object)){const auto attributes=c.u32(person+0x14);c.set(0,attributes);if(nonzero_logical(c,attributes))recipients=p.call(0x103fc710,0x1008d3e8,{registry_object,person,0x21a,array,30});else c.set(3,0);}else c.set(3,0);
    Kept<18> recipients_count(c,std::uint32_t(std::int32_t(std::int8_t(recipients))));row=array;Kept<19> delivered(c,0);
    for(;;){const auto index=std::int8_t(std::uint32_t(delivered));c.set(0,std::uint32_t(std::int32_t(index)));if(compare(c,index,std::int32_t(std::uint32_t(recipients_count)))>=0)break;
        p.call(0x102cc140,0x1008d410,{c.u32(system),message,c.u32(row)});row=row+4;delivered=delivered+1;}
    p.call(0x10738c20,0x1008d430,{f.at(0x38),0xffffffff});
}
}
