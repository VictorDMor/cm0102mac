#include "contract_manager_random.hpp"
#include <bit>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
std::int32_t shifted_quotient(Cpu& c,std::int32_t dividend,std::int32_t multiplier,
                              unsigned shift,bool add_dividend) {
    const auto high=multiply_high(multiplier,dividend);
    const auto numerator=std::uint32_t(high)+(add_dividend?std::uint32_t(dividend):0u);
    const auto floor=std::int32_t(numerator)>>shift;
    c.s.wr<1>(guest::reg::xer_ca,std::int32_t(numerator)<0&&(numerator&((1u<<shift)-1)));
    return std::int32_t(std::uint32_t(floor)+(std::uint32_t(floor)>>31));
}
}

void render_contract_offer_result(Cpu& c,std::uint32_t address,std::uint32_t mode,std::uint32_t out) {
    c.set(0,c.lr());Frame f(c,0x1008ab40,0xe50,{.first_gpr=13});
    Kept<15> message(c,address);Kept<26> clubs(c,c.toc(-0x7be8));Kept<27> buffer(c,c.toc(-0x7bb8));
    Kept<28> pool(c,c.toc(slot::pool));c.put8(f.at(0xe73),mode);c.put32(f.at(0xe74),out);Program p(c);
    const auto first=p.call(0x102cb570,0x1008ab74,{message,0});c.set(0,first);Kept<25> person(c,first);
    for(unsigned field=1;field<=3;++field){const auto name=p.call(0x102cb570,0x1008ab74+field*0x14,{message,field});c.set(0,name);c.put32(f.at(0xdf8-field*4),name);}
    Kept<29> club(c,p.call(0x102cb570,0x1008abc4,{message,4}));
    const auto raw_outcome=p.call(0x102cb570,0x1008abd4,{message,5});const auto person_offset=std::uint32_t(person)*112;c.set(0,person_offset);
    Kept<24> outcome(c,std::uint32_t(std::int32_t(std::int8_t(raw_outcome))));const auto staff_holder=c.toc(-0x7be0);c.set(3,staff_holder);
    const auto staff=c.u32(staff_holder);c.set(6,staff);c.set(5,person_offset);c.put32(f.at(0xdf8),person_offset);c.set(0,person_offset+0x5c);
    const auto age=std::int8_t(c.i16(staff+person_offset+0x5c));c.set(0,std::uint32_t(std::int32_t(age)));c.put8(f.at(0xde8),std::uint32_t(std::int32_t(age)));
    Kept<13> expiry_year(c,p.call(0x102cb570,0x1008ac08,{message,7})&0xffff);
    const auto expiry_day=p.call(0x102cb570,0x1008ac18,{message,6})&0xffff;c.set(0,expiry_day);
    p.call(0x107156b0,0x1008ac2c,{f.at(0x5c),extended(std::int16_t(expiry_day)),expiry_year});
    Kept<23> kind(c,std::uint32_t(std::int32_t(std::int8_t(p.call(0x102cb570,0x1008ac38,{message,10})))));
    Kept<30> reasons(c,p.call(0x102cb570,0x1008ac48,{message,12}));
    const auto manager_id=p.call(0x102cb570,0x1008ac58,{message,13});c.set(0,manager_id);Kept<22> manager(c,manager_id);
    for(unsigned field=14;field<=16;++field){const auto name=p.call(0x102cb570,0x1008ac58+(field-13)*0x14,{message,field});c.set(0,name);c.put32(f.at(0xde4-(field-14)*4),name);}
    Kept<13> timing(c,p.call(0x102cb570,0x1008aca8,{message,17}));
    const auto marker=p.call(0x102cb570,0x1008acb8,{message,18});const auto expected=0x420e9u;c.set(5,expected-marker);c.set(0,((expected-marker)|(marker-expected))>>31);
    Kept<21> generic(c,((expected-marker)|(marker-expected))>>31);
    Kept<31> variation(c,p.call(0x102cb570,0x1008ace0,{message,19}));
    Kept<20> renewal(c,std::uint32_t(std::int32_t(std::int8_t(p.call(0x102cb570,0x1008acf0,{message,20})))));
    c.put32(f.at(0xdfc),p.call(0x102cb570,0x1008ad00,{message,21}));
    Kept<14> wage(c,p.call(0x102cb570,0x1008ad10,{message,22}));
    Kept<17> bonus(c,p.call(0x102cb570,0x1008ad20,{message,23}));
    Kept<16> start_year(c,p.call(0x102cb570,0x1008ad30,{message,25})&0xffff);
    const auto start_day=p.call(0x102cb570,0x1008ad40,{message,24})&0xffff;c.set(0,start_day);
    p.call(0x107156b0,0x1008ad54,{f.at(0x54),extended(std::int16_t(start_day)),start_year});
    const auto from_year=c.u16(f.at(0x56)),to_year=c.u16(f.at(0x5e));c.set(5,from_year);c.set(0,to_year);
    const auto from_day=c.i16(f.at(0x54)),to_day=c.i16(f.at(0x5c));c.set(6,extended(from_day));c.set(5,extended(to_day));
    Kept<16> span(c,(std::uint32_t(to_year)-from_year)*365+extended(to_day)-extended(from_day));
    const auto weeks=shifted_quotient(c,std::int32_t(std::uint32_t(span)),std::int32_t(0x92492493),2,true);
    Kept<19> value(c,std::uint32_t(bonus)+std::uint32_t(weeks)*std::uint32_t(wage));
    c.set(0,3000000);const auto large=compare(c,std::int32_t(std::uint32_t(value)),3000000)>=0;
    if(large){const auto q=shifted_quotient(c,std::int32_t(std::uint32_t(value)),0x431bde83,18,false);value=std::uint32_t(q)*1000000;c.set(0,1000000);}
    else {c.set(0,1000000);if(compare(c,std::int32_t(std::uint32_t(value)),1000000)>0){const auto q=shifted_quotient(c,std::int32_t(std::uint32_t(value)),0x431bde83,17,false);value=std::uint32_t(q)*500000;c.set(0,500000);}}
    c.put32(f.at(0x48),value);const auto converted=p.call(0x10009f20,0x1008ae14,{f.at(0x48),6});const auto converted_low=c.r(4);
    p.call(0x1000b410,0x1008ae2c,{f.at(0x84),100,converted,converted_low,6});
    const auto encoded=std::uint32_t(span)^0x80000000u;c.set(3,encoded);c.set(0,0x43300000);c.put32(f.at(0xdd8),encoded);
    const auto constants=c.toc(-0x571c);c.set(3,message);c.set(4,constants);c.put32(f.at(0xdd4),0x43300000);c.set(5,constants);
    guest::Fpu fp(c);const auto bias=c.f64(constants+8);c.set_f(2,bias);c.set(4,26);const auto as_double=c.f64(f.at(0xdd4));c.set_f(1,as_double);
    const auto days_per_year=c.f64(constants+0xf8);c.set_f(0,days_per_year);const auto duration=fp.fsub(as_double,bias);c.set_f(1,duration);
    const auto half=c.f64(constants+0xb8);c.set_f(3,half);const auto years=fp.fdiv(duration,days_per_year);c.set_f(0,years);
    const auto rounded=fp.fadd(half,years);c.set_f(0,rounded);c.s.pc=0x1008ae68;
    c.put64(f.at(0xdcc),fp.fctiwz(0,rounded));Kept<18> term(c,c.u32(f.at(0xdd0)));
    Kept<17> career_end(c,std::uint32_t(std::int32_t(std::int8_t(p.call(0x102cb570,0x1008ae78,{message,26})))));
    const auto index_flag=p.call(0x102cb570,0x1008ae88,{message,27});const auto ui=c.toc(-0x7bb0);c.set(4,ui);
    Kept<16> previous_rejection(c,std::uint32_t(std::int32_t(std::int8_t(index_flag))));const auto selected=c.i16(ui+0x2c58);c.set(0,extended(selected));
    c.set(3,std::uint32_t(std::int32_t(selected))*0x2c0);c.set(0,ui);const auto context=c.u32(ui+std::uint32_t(std::int32_t(selected))*0x2c0+4);c.set(3,context);bool own=false;
    if(nonzero_logical(c,context)){const auto team=c.u32(context+0x24);c.set(3,team);if(nonzero_logical(c,team)){const auto id=c.u32(team);c.set(0,id);const auto delta=id-std::uint32_t(club);c.set(0,delta);c.set(0,delta==0?32u:std::countl_zero(delta));c.set(0,delta==0);own=delta==0;}}
    Kept<15> own_club(c,own);
    const auto person_name=[&](unsigned offset,std::uint32_t variant,std::uint32_t back,std::uint32_t length){p.call(0x100cb590,back,{c.u32(f.at(0xdf4)),c.u32(f.at(0xdf0)),c.u32(f.at(0xdec)),f.at(offset),length,variant,person});};
    person_name(0xc3c,1,0x1008aef4,400);person_name(0xaac,5,0x1008af14,400);
    if(compare(c,std::int32_t(std::uint32_t(manager)),-1)>0)p.call(0x100cb590,0x1008af3c,{c.u32(f.at(0xde4)),c.u32(f.at(0xde0)),c.u32(f.at(0xddc)),f.at(0x91c),400,1,manager});
    p.call(0x10716eb0,0x1008af50,{f.at(0x5c),f.at(0x7f0),300,255});
    const auto mode_byte=std::int8_t(c.u8(f.at(0xe73)));c.set(0,std::uint32_t(std::int32_t(mode_byte)));const bool full=nonzero(c,mode_byte);
    const auto byte=[&](std::uint32_t v){const auto signed_value=std::int8_t(v);c.set(0,std::uint32_t(std::int32_t(signed_value)));return nonzero(c,signed_value);};
    struct Names {std::uint32_t gender,name;};
    const auto names=[&](bool alternate=false){const auto offset=std::uint32_t(club)*0x248;c.set(6,offset);const auto records=c.u32(clubs);c.set(0,records);return Names{std::uint32_t(std::int32_t(c.i8(records+offset+(alternate?0x242:0x23b)))),records+offset+(alternate?0x1ee:0x221)};};
    const auto title_person=[&](unsigned text,std::uint32_t back){p.call(0x10727890,back,{buffer,pool+text,f.at(0xaac)});};
    enum class Title {person_club,person_years_club,club_person};
    const auto title_club=[&](unsigned text,std::uint32_t back,Title layout){const auto n=names();
        if(layout==Title::person_club)p.call(0x10727890,back,{buffer,pool+text,n.gender,n.gender,f.at(0xaac),n.name});
        else if(layout==Title::person_years_club)p.call(0x10727890,back,{buffer,pool+text,n.gender,n.gender,f.at(0xaac),term,n.name});
        else p.call(0x10727890,back,{buffer,pool+text,n.gender,n.gender,n.name,f.at(0xaac)});};
    const auto expiry_text=[&](unsigned text,std::uint32_t back,bool alternate=false){const auto n=names(alternate);p.call(0x10727890,back,{buffer,pool+text,n.gender,n.gender,100,f.at(0xc3c),n.name,f.at(0x7f0)});};
    enum class Deal {person_club_years_money,club_person_years_money,person_years_club_money,club_person_years,person_years_club};
    const auto deal_text=[&](unsigned text,std::uint32_t back,Deal layout,bool alternate=false){const auto n=names(alternate);
        if(layout==Deal::person_club_years_money)p.call(0x10727890,back,{buffer,pool+text,n.gender,n.gender,f.at(0xc3c),n.name,term,f.at(0x84)});
        else if(layout==Deal::club_person_years_money)p.call(0x10727890,back,{buffer,pool+text,n.gender,n.gender,n.name,f.at(0xc3c),term,f.at(0x84)});
        else if(layout==Deal::person_years_club_money)p.call(0x10727890,back,{buffer,pool+text,n.gender,n.gender,f.at(0xc3c),term,n.name,f.at(0x84)});
        else if(layout==Deal::club_person_years)p.call(0x10727890,back,{buffer,pool+text,n.gender,n.gender,n.name,f.at(0xc3c),term});
        else p.call(0x10727890,back,{buffer,pool+text,n.gender,n.gender,f.at(0xc3c),term,n.name});};
    bool invalid_title=false;
    if(!full){
        if(compare(c,std::int32_t(std::uint32_t(outcome)),2)==0){
            if(compare(c,std::int32_t(std::uint32_t(kind)),5)==0)title_person(0x43a,0x1008af7c);
            else if(byte(generic))title_person(0x46e,0x1008af98);
            else {const auto reason=c.u32(f.at(0xdfc));c.set(0,reason);bool distinctive=compare(c,std::int32_t(reason),11)==0;if(distinctive)distinctive=byte(renewal);
                if(distinctive){const auto order=compare(c,std::int32_t(std::uint32_t(variation)),1);if(order==0)title_club(0x4ec,0x1008b030,Title::person_years_club);else if(order<0&&compare(c,std::int32_t(std::uint32_t(variation)))>=0)title_club(0x4a5,0x1008aff8,Title::person_club);else title_club(0x550,0x1008b064,Title::club_person);}
                else if(!byte(own_club))title_club(0x598,0x1008b0a0,Title::person_club);else title_person(0x46e,0x1008b0b4);}
        } else {const auto status=std::int32_t(std::uint32_t(outcome));bool rejected=compare(c,status,1)==0;if(!rejected)rejected=compare(c,status,3)==0;
            if(!rejected)invalid_title=true;
            else if(compare(c,std::int32_t(std::uint32_t(kind)),5)==0)title_person(0x5de,0x1008b0e0);
            else if(byte(generic))title_person(0x612,0x1008b0fc);
            else {const auto reason=c.u32(f.at(0xdfc));c.set(0,reason);bool distinctive=compare(c,std::int32_t(reason),11)==0;if(distinctive)distinctive=byte(renewal);
                if(distinctive){if(byte(previous_rejection))title_person(0x649,0x1008b12c);else {const auto order=compare(c,std::int32_t(std::uint32_t(variation)),1);if(order==0)title_club(0x6df,0x1008b1b0,Title::person_years_club);else if(order<0&&compare(c,std::int32_t(std::uint32_t(variation)))>=0)title_club(0x690,0x1008b178,Title::person_club);else title_person(0x745,0x1008b1c4);}}
                else if(!byte(own_club))title_club(0x774,0x1008b200,Title::person_club);else title_person(0x612,0x1008b214);}
        }
    } else if(compare(c,std::int32_t(std::uint32_t(outcome)),2)==0) {
        if(compare(c,std::int32_t(std::uint32_t(kind)),5)==0)expiry_text(0x7ba,0x1008b2b8);
        else {
            c.set(0,0);Kept<13> love(c,std::uint32_t(reasons)&8u);const bool loves=nonzero(c,std::int32_t(std::uint32_t(love)));c.put8(f.at(0x660),0);Kept<15> happiness(c,0);
            if(loves){p.call(0x10727890,0x1008b2dc,{f.at(0x46c),pool+0x865});c.set(0,std::uint32_t(reasons)&64u);const bool admires=nonzero(c,std::int32_t(std::uint32_t(reasons)&64u));happiness=1;
                if(admires&&compare(c,std::int32_t(std::uint32_t(manager)),-1)>0){c.set(0,100);p.call(0x10727890,0x1008b30c,{f.at(0x4d0),pool+0x882,f.at(0x91c)});happiness=2;}}
            else {c.set(0,std::uint32_t(reasons)&64u);if(nonzero(c,std::int32_t(std::uint32_t(reasons)&64u))){const auto young=c.u8(f.at(0xde8));c.set(0,young);if(compare(c,young,24)<0&&compare(c,std::int32_t(std::uint32_t(manager)),-1)>0){p.call(0x10727890,0x1008b340,{f.at(0x46c),pool+0x8c9,f.at(0x91c)});happiness=1;}}}
            const auto count=std::int8_t(std::uint32_t(happiness));c.set(0,std::uint32_t(std::int32_t(count)));const auto order=compare(c,count,2);
            if(order==0)p.call(0x10727890,0x1008b388,{f.at(0x660),pool+0x926,f.at(0x46c),f.at(0x4d0)});
            else if(order<0&&compare(c,count,1)>=0)p.call(0x1077b720,0x1008b370,{f.at(0x660),pool+0x923,f.at(0x46c)});
            if(byte(generic))expiry_text(0x94a,0x1008b3c8);
            else if(byte(happiness)){
                c.set(0,std::uint32_t(reasons)&64u);bool career_story=nonzero(c,std::int32_t(std::uint32_t(reasons)&64u));if(career_story)career_story=byte(career_end);if(career_story)career_story=compare(c,std::int32_t(std::uint32_t(manager)),-1)>0;
                if(career_story){const auto threshold=(std::uint32_t(variation)+1)*1000000;c.set(3,threshold);c.set(0,threshold);
                    if(compare(c,std::int32_t(std::uint32_t(value)),std::int32_t(threshold))>0){const auto n=names();c.set(0,f.at(0x660));c.put32(f.at(0x38),f.at(0x660));p.call(0x10727890,0x1008b448,{buffer,pool+0xa00,n.gender,n.gender,f.at(0xc3c),n.name,term,f.at(0x84)});}
                    else {const auto n=names();c.set(0,f.at(0x660));c.put32(f.at(0x38),f.at(0x660));p.call(0x10727890,0x1008b48c,{buffer,pool+0xafc,n.gender,n.gender,100,f.at(0xc3c),n.name,f.at(0x7f0)});}}
                else if(byte(career_end)){const auto threshold=(std::uint32_t(variation)+1)*1000000;c.set(3,threshold);c.set(0,threshold);
                    if(compare(c,std::int32_t(std::uint32_t(value)),std::int32_t(threshold))>0){const bool previous=byte(previous_rejection);deal_text(previous?0xbe9:0xcdb,previous?0x1008b4f4:0x1008b530,Deal::person_club_years_money);}
                    else expiry_text(0xdb5,0x1008b56c);}
                else if(compare(c,std::int32_t(std::uint32_t(love)))!=0&&([&]{const auto n=std::int8_t(std::uint32_t(happiness));c.set(0,std::uint32_t(std::int32_t(n)));return compare(c,n,1)==0;})()){
                    const auto order=compare(c,std::int32_t(std::uint32_t(variation)),1);
                    if(order<0&&compare(c,std::int32_t(std::uint32_t(variation)))>=0){const bool renewed=byte(renewal);expiry_text(renewed?0xe73:0xf2b,renewed?0x1008b5dc:0x1008b618,true);}
                    else if(order==0){c.set(3,0x000f0000);c.set(0,1000000);if(compare(c,std::int32_t(std::uint32_t(value)),1000000)>0){
                            if(byte(renewal)){const bool previous=byte(previous_rejection);const auto n=names(true);c.set(0,f.at(0xaac));c.put32(f.at(0x38),f.at(0xaac));p.call(0x10727890,previous?0x1008b67c:0x1008b6c0,{buffer,pool+(previous?0xfe3:0x112b),n.gender,n.gender,n.name,f.at(0xc3c),term,f.at(0x84)});}
                            else deal_text(0x1249,0x1008b6fc,Deal::person_years_club_money,true);
                        }else {const bool renewed=byte(renewal);deal_text(renewed?0x1341:0x1423,renewed?0x1008b73c:0x1008b774,renewed?Deal::club_person_years:Deal::person_years_club,true);}}
                    else if(byte(renewal)){
                        if(byte(previous_rejection)){const bool one=compare(c,std::int32_t(std::uint32_t(term)),1)==0;const auto n=names(true);c.set(0,f.at(0xaac));c.put32(f.at(0x38),f.at(0xc3c));
                            c.put32(f.at(0x3c),one?f.at(0xaac):std::uint32_t(term));c.put32(f.at(0x40),one?n.name:f.at(0xaac));if(!one)c.put32(f.at(0x44),n.name);
                            p.call(0x10727890,one?0x1008b7dc:0x1008b830,{buffer,pool+(one?0x14b7:0x15d6),n.gender,n.gender,100,n.gender,n.gender,n.name});}
                        else {const bool one=compare(c,std::int32_t(std::uint32_t(term)),1)==0;if(one){const auto n=names(true);p.call(0x10727890,0x1008b86c,{buffer,pool+0x1708,n.gender,n.gender,n.name,f.at(0xc3c)});}else deal_text(0x17a1,0x1008b8a4,Deal::club_person_years,true);}}
                    else {const auto high=multiply_high(0x66666667,std::int32_t(std::uint32_t(person))),floor=high>>2;const auto q=std::uint32_t(floor)+(std::uint32_t(floor)>>31);c.s.wr<1>(guest::reg::xer_ca,high<0&&(std::uint32_t(high)&3));c.set(3,std::uint32_t(floor)>>31);c.set(0,std::uint32_t(person)-q*10);
                        if(!nonzero(c,std::int32_t(std::uint32_t(person)-q*10))&&compare(c,std::int32_t(std::uint32_t(wage)),8000)>=0){const auto n=names(true);p.call(0x10727890,0x1008b90c,{buffer,pool+0x184d,100,n.gender,n.gender,f.at(0xc3c),term,n.name});}
                        else deal_text(0x18f7,0x1008b944,Deal::person_years_club,true);}
                } else {
                    if(byte(previous_rejection)&&compare(c,std::int32_t(std::uint32_t(wage)),5000)>=0){const auto n=names(true);p.call(0x10727890,0x1008b990,{buffer,pool+0x1985,n.gender,n.gender,f.at(0xc3c),term,n.name,f.at(0x660)});}
                    else {const auto n=names(true);p.call(0x10727890,0x1008b9c8,{buffer,pool+0x1a34,n.gender,n.gender,f.at(0xc3c),n.name,f.at(0x660)});}}
            } else {
                bool ending=byte(career_end);if(ending)ending=byte(renewal);
                if(ending)expiry_text(0x1abd,0x1008ba14);
                else {const bool renewed=byte(renewal);const auto threshold=(std::uint32_t(variation)+1)*1000000;c.set(3,threshold);c.set(0,threshold);
                    if(compare(c,std::int32_t(std::uint32_t(value)),std::int32_t(threshold))>0){const auto high=multiply_high(0x55555556,std::int32_t(std::uint32_t(person)));const auto q=std::uint32_t(high)+(std::uint32_t(high)>>31);const auto remainder=std::uint32_t(person)-q*3;c.set(3,std::uint32_t(high));c.set(0,remainder);
                        const bool first=compare(c,std::int32_t(std::uint32_t(variation)),std::int32_t(remainder))>=0;
                        if(renewed)deal_text(first?0x1b7c:0x1c39,first?0x1008ba98:0x1008bad4,first?Deal::person_club_years_money:Deal::club_person_years_money);
                        else deal_text(first?0x1cec:0x1d96,first?0x1008bb8c:0x1008bbc8,first?Deal::person_club_years_money:Deal::person_years_club_money);
                    }else expiry_text(renewed?0x94a:0x1e4a,renewed?0x1008bb10:0x1008bc04);}
            }
        }
    } else if(compare(c,std::int32_t(std::uint32_t(outcome)),1)==0) {
        if(compare(c,std::int32_t(std::uint32_t(kind)),5)==0){const auto n=names();p.call(0x10727890,0x1008bc48,{buffer,pool+0x1eef,n.gender,n.gender,f.at(0xc3c),n.name});}
        else {
            c.set(3,0);c.set(0,std::uint32_t(reasons)&256u);const bool unhappy=nonzero(c,std::int32_t(std::uint32_t(reasons)&256u));c.put8(f.at(0x2dc),0);Kept<14> complaints(c,0);
            if(unhappy&&compare(c,std::int32_t(std::uint32_t(manager)),-1)>0){p.call(0x10727890,0x1008bc78,{f.at(0xe8),pool+0x1f60,f.at(0x91c)});complaints=1;}
            const auto before=std::int8_t(std::uint32_t(complaints));c.set(3,std::uint32_t(std::int32_t(before)));
            if(compare(c,before,2)<0){c.set(0,std::uint32_t(reasons)&4u);if(nonzero(c,std::int32_t(std::uint32_t(reasons)&4u))){c.set(0,std::uint32_t(std::int32_t(before))*100);p.call(0x10727890,0x1008bca4,{f.at(0xe8)+std::uint32_t(std::int32_t(before))*100,pool+0x1fad});complaints=complaints+1;}}
            const auto count=std::int8_t(std::uint32_t(complaints));c.set(0,std::uint32_t(std::int32_t(count)));const auto order=compare(c,count,2);
            if(order==0)p.call(0x10727890,0x1008bcec,{f.at(0x2dc),pool+0x926,f.at(0xe8),f.at(0x14c)});
            else if(order<0&&compare(c,count,1)>=0)p.call(0x1077b720,0x1008bcd4,{f.at(0x2dc),pool+0x923,f.at(0xe8)});
            person_name(0xc3c,1,0x1008bd0c,300);
            bool short_window=!byte(own_club);if(short_window)short_window=compare(c,std::int32_t(std::uint32_t(timing)),999)==0;
            if(short_window){const bool reason=byte(complaints);const auto n=names(true);if(reason)p.call(0x10727890,0x1008bd58,{buffer,pool+0x1fe2,n.gender,n.gender,f.at(0xc3c),n.name,f.at(0x2dc)});else p.call(0x10727890,0x1008bd8c,{buffer,pool+0x209f,n.gender,n.gender,f.at(0xc3c),n.name});}
            else {bool longer_window=!byte(own_club);if(longer_window)longer_window=compare(c,std::int32_t(std::uint32_t(timing)),1000)==0;
                if(longer_window){const bool reason=byte(complaints);const auto n=names(true);if(reason)p.call(0x10727890,0x1008bde0,{buffer,pool+0x2138,100,n.gender,n.gender,f.at(0xc3c),n.name,f.at(0x2dc)});else p.call(0x10727890,0x1008be18,{buffer,pool+0x21eb,100,n.gender,n.gender,f.at(0xc3c),n.name});}
                else {const bool reason=byte(complaints);const auto n=names(true);if(reason)p.call(0x10727890,0x1008be58,{buffer,pool+0x227a,n.gender,n.gender,f.at(0xc3c),n.name,f.at(0x2dc)});else p.call(0x10727890,0x1008be8c,{buffer,pool+0x230c,n.gender,n.gender,f.at(0xc3c),n.name});}}
        }
    } else if(compare(c,std::int32_t(std::uint32_t(outcome)),3)==0){const auto n=names();p.call(0x10727890,0x1008bec8,{buffer,pool+0x237a,n.gender,n.gender,f.at(0xc3c),n.name});}
    if(invalid_title){p.call(0x10739890,0x1008b230,{pool+0x37,0,0,f.at(0x64),f.at(0x4c)});const auto version=p.call(0x10768810,0x1008b234);const auto errors=c.toc(-0x7ba8);p.call(0x1077b720,0x1008b250,{errors,pool+0x23b,version,f.at(0x64),f.at(0x4c),0x220c});p.call(0x10769b60,0x1008b260,{pool+0x248,c.toc(-0x7ba8),0});const auto success=c.toc(-0x7bac);c.set(3,success);c.set(0,0);c.put8(success,0);return;}
    p.call(0x1074b830,0x1008bed4,{c.u32(f.at(0xe74)),buffer});
    const auto club_offset=std::uint32_t(club)*0x248;c.set(0,club_offset);const auto records=c.u32(clubs);c.set(4,records);
    p.call(0x1074bab0,0x1008bef0,{c.u32(f.at(0xe74)),records+club_offset+0x221,0x7d5,records+club_offset});
    const auto lookup_names=[&](std::uint32_t back){const auto holder=c.toc(-0x7be0);c.set(3,holder);c.set(7,0);const auto offset=c.u32(f.at(0xdf8));c.set(0,offset);const auto staff=c.u32(holder);c.set(3,staff);p.call(0x100cbde0,back,{staff+offset,c.u32(f.at(0xdf4)),c.u32(f.at(0xdf0)),c.u32(f.at(0xdec)),0});};
    lookup_names(0x1008bf14);const auto available=std::int8_t(c.r(3));c.set(0,std::uint32_t(std::int32_t(available)));
    if(nonzero(c,available)){for(unsigned n=0;n<2;++n){const auto holder=c.toc(-0x7be0);c.set(3,holder);const auto offset=c.u32(f.at(0xdf8));c.set(0,offset);const auto staff=c.u32(holder);c.set(6,staff);p.call(0x1074bab0,n==0?0x1008bf3c:0x1008bf5c,{c.u32(f.at(0xe74)),f.at(n==0?0xc3c:0xaac),0x7d7,staff+offset});}}
    if(compare(c,std::int32_t(std::uint32_t(manager)),-1)>0){const auto holder=c.toc(-0x7be0);c.set(3,holder);Kept<13> manager_offset(c,std::uint32_t(manager)*112);const auto staff=c.u32(holder);c.set(0,staff);
        const auto found=p.call(0x100cbde0,0x1008bf88,{staff+std::uint32_t(manager_offset),c.u32(f.at(0xde4)),c.u32(f.at(0xde0)),c.u32(f.at(0xddc)),0});const auto byte_found=std::int8_t(found);c.set(0,std::uint32_t(std::int32_t(byte_found)));
        if(nonzero(c,byte_found)){const auto fresh_holder=c.toc(-0x7be0);c.set(3,fresh_holder);const auto fresh=c.u32(fresh_holder);c.set(0,fresh);p.call(0x1074bab0,0x1008bfac,{c.u32(f.at(0xe74)),f.at(0x91c),0x7d7,fresh+std::uint32_t(manager_offset)});}}
}
}
