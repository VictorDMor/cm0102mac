#include "contract_manager_negotiation_detail.hpp"

namespace cm::aot::contract_manager::negotiation {
namespace {
void add_rating(Context& n, std::uint32_t amount) {
    n.special_club_or_floor = std::uint32_t(n.special_club_or_floor) + amount;
}

void integer_skill_bonus(Context& n, std::int32_t skill, unsigned baseline) {
    auto& c=n.c;
    const auto excess=std::uint32_t(skill)-baseline;c.set(0,excess);
    const auto square=excess*excess;c.set(0,square);
    const auto bonus=extended(std::int16_t(square));c.set(0,bonus);add_rating(n,bonus);
}

void fractional_skill_bonus(Context& n, std::int32_t skill, unsigned factor_offset, bool first) {
    auto& c=n.c;auto& fp=n.fp;
    const auto excess=std::uint32_t(skill)-15;
    if(first)c.set(4,excess);else c.set(3,excess);
    c.set(0,0x43300000);const auto square=excess*excess;
    if(first)c.set(4,square);else c.set(3,square);
    c.put32(n.frame.at(0x44),0x43300000);
    const auto bias=c.f64(n.constants+8);c.set_f(1,bias);
    const auto factor=c.f64(n.constants+factor_offset);c.set_f(2,factor);
    c.set(0,square^0x80000000u);c.put32(n.frame.at(0x48),c.r(0));
    const auto encoded=c.f64(n.frame.at(0x44));c.set_f(0,encoded);
    const auto value=fp.fsub(encoded,bias);c.set_f(0,value);
    const auto bonus=fp.fmul(factor,value);c.set_f(0,bonus);
    c.put64(n.frame.at(0x4c),fp.fctiwz(0,bonus));
    const auto integer=c.u32(n.frame.at(0x50));c.set(0,integer);add_rating(n,integer);
}

void profile_step(Context& n, std::initializer_list<std::pair<int,int>> steps) {
    auto& c=n.c;
    const auto profile=std::int8_t(std::uint32_t(n.national_context_or_role_score));
    c.set(0,std::uint32_t(std::int32_t(profile)));
    for(auto [threshold,bonus]:steps)if(compare(c,profile,threshold)>0){if(bonus)add_rating(n,std::uint32_t(bonus));return;}
}

void role_bonuses(Context& n, std::int32_t second_skill, int full_role_order) {
    auto& c=n.c;const auto first=std::int32_t(std::uint32_t(n.today));
    const auto role=std::int32_t(std::uint32_t(n.option));
    if(full_role_order==0){
        profile_step(n,{{85,15},{75,10},{65,8},{50,5},{35,3}});
        if(compare(c,first,15)>0)integer_skill_bonus(n,first,13);
        if(compare(c,second_skill,15)>0)integer_skill_bonus(n,second_skill,14);
        return;
    }
    if(compare(c,role,34)>0){
        profile_step(n,{{75,8},{65,5},{50,3},{35,1}});
        if(compare(c,first,15)>0)integer_skill_bonus(n,first,15);
        if(compare(c,second_skill,15)>0)fractional_skill_bonus(n,second_skill,0x170,false);
        return;
    }
    if(compare(c,role,25)>0){
        profile_step(n,{{75,5},{65,3},{50,1},{35,0}});
        const auto profile=std::int8_t(std::uint32_t(n.national_context_or_role_score));
        if(profile<35)add_rating(n,std::uint32_t(-3));
        if(compare(c,first,15)>0)fractional_skill_bonus(n,first,0x170,true);
        if(compare(c,second_skill,15)>0)fractional_skill_bonus(n,second_skill,0xb8,false);
        return;
    }
    if(compare(c,role,15)<0){
        const auto profile=std::int8_t(std::uint32_t(n.national_context_or_role_score));c.set(0,std::uint32_t(std::int32_t(profile)));
        if(compare(c,profile,50)>0)add_rating(n,std::uint32_t(-5));
        else if(compare(c,profile,35)>0)add_rating(n,std::uint32_t(-8));
        else add_rating(n,std::uint32_t(-10));
    }
}

void outfield_profile(Context& n, std::uint32_t attributes) {
    auto& c=n.c;auto& fp=n.fp;
    const auto raw_a=c.u8(attributes+0x3c);c.set(3,raw_a);c.set(0,0x43300000);
    c.set(6,std::uint32_t(n.age)^0x80000000u);
    const auto raw_b=c.u8(attributes+0x44);c.set(4,raw_b);
    const auto a=std::int8_t(raw_a);c.set(3,std::uint32_t(std::int32_t(a)));
    const auto raw_c=c.u8(attributes+0x3d);c.set(7,raw_c);c.set(3,std::uint32_t(std::int32_t(a))^0x80000000u);
    const auto b=std::int8_t(raw_b);c.set(4,std::uint32_t(std::int32_t(b)));c.put32(n.frame.at(0x48),c.r(6));c.set(6,std::uint32_t(std::int32_t(b))^0x80000000u);
    const auto profile_c=std::int8_t(raw_c);c.set(4,std::uint32_t(std::int32_t(profile_c)));
    const auto raw_d=c.u8(attributes+0x3f);c.set(8,raw_d);c.put32(n.frame.at(0x44),0x43300000);c.set(7,std::uint32_t(std::int32_t(profile_c))^0x80000000u);
    const auto bias=c.f64(n.constants+8);c.set_f(6,bias);
    const auto d=std::int8_t(raw_d);c.set(4,std::uint32_t(std::int32_t(d)));
    const auto age_encoded=c.f64(n.frame.at(0x44));c.set_f(0,age_encoded);c.set(8,std::uint32_t(std::int32_t(d))^0x80000000u);c.put32(n.frame.at(0x70),c.r(3));c.set(3,n.person);
    const auto age=fp.fsub(age_encoded,bias);c.set_f(3,age);
    const auto age_slope=c.f64(n.constants+0x3b8);c.set_f(4,age_slope);c.put32(n.frame.at(0x6c),0x43300000);c.set(4,11);
    const auto age_intercept=c.f64(n.constants+0x3b0);c.set_f(1,age_intercept);
    const auto a_encoded=c.f64(n.frame.at(0x6c));c.set_f(0,a_encoded);
    const auto weight=c.f64(n.constants+0x20);c.set_f(2,weight);
    n.age_factor=fp.multiply_add(0x10088c5c,age_slope,age,age_intercept);c.set_f(22,n.age_factor);c.put32(n.frame.at(0x68),c.r(6));
    const auto av=fp.fsub(a_encoded,bias);c.set_f(0,av);
    const auto raw_e=c.u8(attributes+0x2e);c.set(6,raw_e);c.put32(n.frame.at(0x64),0x43300000);
    const auto e=std::int8_t(raw_e);c.set(6,std::uint32_t(std::int32_t(e)));
    auto weighted=fp.fmul(weight,av);c.set_f(0,weighted);
    const auto b_encoded=c.f64(n.frame.at(0x64));c.set_f(1,b_encoded);c.put32(n.frame.at(0x60),c.r(7));c.set(6,std::uint32_t(std::int32_t(e))^0x80000000u);
    const auto bv=fp.fsub(b_encoded,bias);c.set_f(1,bv);
    const auto d_weight=c.f64(n.constants+0x170);c.set_f(4,d_weight);
    weighted=fp.fmul(weighted,n.age_factor);c.set_f(0,weighted);c.put32(n.frame.at(0x5c),0x43300000);
    const auto e_weight=c.f64(n.constants+0x2c0);c.set_f(5,e_weight);
    const auto c_encoded=c.f64(n.frame.at(0x5c));c.set_f(3,c_encoded);c.put32(n.frame.at(0x58),c.r(8));
    weighted=fp.multiply_add(0x10088ca0,weight,bv,weighted);c.set_f(0,weighted);c.put32(n.frame.at(0x54),0x43300000);
    const auto cv=fp.fsub(c_encoded,bias);c.set_f(1,cv);
    const auto first_role=c.u8(attributes+0x36);c.set(7,first_role);
    const auto d_encoded=c.f64(n.frame.at(0x54));c.set_f(2,d_encoded);c.put32(n.frame.at(0x50),c.r(6));
    weighted=fp.multiply_add(0x10088cb8,cv,n.age_factor,weighted);c.set_f(0,weighted);
    const auto second_role=c.u8(attributes+0x1b);c.set(5,second_role);c.put32(n.frame.at(0x4c),0x43300000);
    const auto dv=fp.fsub(d_encoded,bias);c.set_f(1,dv);c.set(0,std::uint32_t(first_role)+second_role);
    const auto e_encoded=c.f64(n.frame.at(0x4c));c.set_f(2,e_encoded);
    weighted=fp.multiply_add(0x10088cd0,d_weight,dv,weighted);c.set_f(0,weighted);
    n.option=std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(first_role)+second_role)));
    const auto ev=fp.fsub(e_encoded,bias);c.set_f(1,ev);
    weighted=fp.multiply_add(0x10088cdc,e_weight,ev,weighted);c.set_f(0,weighted);
    c.put64(n.frame.at(0x74),fp.fctiwz(0,weighted));n.national_context_or_role_score=c.u32(n.frame.at(0x78));
    const auto first_result=n.call(0x100d1cb0,0x10088cf0);
    const auto first=std::int8_t(first_result);c.set(3,std::uint32_t(std::int32_t(first)));c.set(0,0x43300000);c.set(3,std::uint32_t(std::int32_t(first))^0x80000000u);c.put32(n.frame.at(0x7c),0x43300000);
    const auto first_bias=c.f64(n.constants+8);c.set_f(1,first_bias);c.set(4,20);c.put32(n.frame.at(0x80),c.r(3));
    const auto first_encoded=c.f64(n.frame.at(0x7c));c.set_f(0,first_encoded);
    auto first_value=fp.fsub(first_encoded,first_bias);c.set_f(0,first_value);first_value=fp.fmul(n.age_factor,first_value);c.set_f(0,first_value);
    c.put64(n.frame.at(0x84),fp.fctiwz(0,first_value));const auto first_integer=c.u32(n.frame.at(0x88));c.set(0,first_integer);
    const auto first_skill=std::int8_t(first_integer);c.set(0,std::uint32_t(std::int32_t(first_skill)));
    const auto first_cap=compare(c,first_skill,20)<0?std::int32_t(first_skill):20;if(first_cap<20)c.set(4,std::uint32_t(first_cap));
    c.set(3,n.person);n.today=std::uint32_t(std::int32_t(std::int8_t(first_cap)));c.set(4,12);
    const auto second_result=n.call(0x100d1cb0,0x10088d44);
    const auto second=std::int8_t(second_result);c.set(3,std::uint32_t(std::int32_t(second)));c.set(0,0x43300000);c.set(4,std::uint32_t(std::int32_t(second))^0x80000000u);c.put32(n.frame.at(0x8c),0x43300000);
    const auto second_bias=c.f64(n.constants+8);c.set_f(1,second_bias);c.set(3,20);c.put32(n.frame.at(0x90),c.r(4));
    const auto second_encoded=c.f64(n.frame.at(0x8c));c.set_f(0,second_encoded);
    auto second_value=fp.fsub(second_encoded,second_bias);c.set_f(0,second_value);second_value=fp.fmul(n.age_factor,second_value);c.set_f(0,second_value);
    c.put64(n.frame.at(0x94),fp.fctiwz(0,second_value));const auto second_integer=c.u32(n.frame.at(0x98));c.set(0,second_integer);
    const auto second_skill=std::int8_t(second_integer);c.set(0,std::uint32_t(std::int32_t(second_skill)));
    const auto second_cap=compare(c,second_skill,20)<0?std::int32_t(second_skill):20;if(second_cap<20)c.set(3,std::uint32_t(second_cap));
    const auto role_order=compare(c,std::int32_t(std::uint32_t(n.option)),40);c.set(3,std::uint32_t(std::int32_t(std::int8_t(second_cap))));
    role_bonuses(n,std::int8_t(second_cap),role_order);
}

std::int32_t reputation_adjustment(Context& n, unsigned shift, bool narrow=true, bool with_rating=false) {
    auto& c=n.c;c.set(3,0x10620000);const auto reputation=c.i16(n.employer+0x1ec);c.set(0,extended(reputation));c.set(3,0x10624dd3);
    if(with_rating)c.set(4,extended(std::int16_t(std::uint32_t(n.special_club_or_floor))));
    const auto high=multiply_high(0x10624dd3,reputation);c.set(0,std::uint32_t(high));const auto floor=high>>shift;c.set(0,std::uint32_t(floor));
    c.s.wr<1>(guest::reg::xer_ca,high<0&&(std::uint32_t(high)&((1u<<shift)-1)));
    const auto sign=std::uint32_t(floor)>>31;c.set(3,sign);const auto quotient=std::uint32_t(floor)+sign;c.set(0,quotient);
    if(narrow){c.set(0,extended(std::int16_t(quotient)));return std::int16_t(quotient);}return std::int32_t(quotient);
}

std::int32_t older_reputation_adjustment(Context& n) {
    auto& c=n.c;c.set(3,0xc3a10000);const auto reputation=c.i16(n.employer+0x1ec);c.set(0,extended(reputation));c.set(3,0xc3a13de7);
    const auto high=multiply_high(std::int32_t(0xc3a13de7u),reputation);c.set(3,std::uint32_t(high));const auto combined=std::uint32_t(high)+extended(reputation);c.set(0,combined);
    const auto floor=std::int32_t(combined)>>9;c.set(0,std::uint32_t(floor));c.s.wr<1>(guest::reg::xer_ca,std::int32_t(combined)<0&&(combined&511));
    const auto sign=std::uint32_t(floor)>>31;c.set(3,sign);const auto quotient=std::uint32_t(floor)+sign;c.set(0,quotient);const auto narrowed=std::int16_t(quotient);c.set(0,extended(narrowed));return narrowed;
}

std::int32_t current_age(Context& n) {
    auto& c=n.c;const auto stored=c.i16(n.person+0x5c);c.set(0,extended(stored));const auto age=std::int8_t(stored);c.set(0,std::uint32_t(std::int32_t(age)));return age;
}

void contextual_rating(Context& n) {
    auto& c=n.c;auto& fp=n.fp;
    if(nonzero_logical(c,n.tenure_or_shortlist)){
        const auto candidate=c.u32(n.tenure_or_shortlist+0x10);c.set(3,candidate);const auto person=c.u32(n.person);c.set(0,person);
        if(compare(c,std::int32_t(candidate),std::int32_t(person))==0)add_rating(n,std::uint32_t(reputation_adjustment(n,6)));
    }
    const auto squad_role=c.u8(n.contract+0x60)&15u;c.set(0,squad_role);
    if(compare(c,std::int32_t(squad_role),1)==0){
        bool minimum=false;if(compare(c,current_age(n),19)<0){const auto rating=std::int16_t(std::uint32_t(n.special_club_or_floor));const auto adjustment=reputation_adjustment(n,5,false,true);const auto combined=extended(rating)+std::uint32_t(adjustment);c.set(0,combined);if(compare(c,std::int32_t(combined),121)<0){n.special_club_or_floor=121;minimum=true;}}
        if(!minimum)add_rating(n,std::uint32_t(reputation_adjustment(n,5)));
    }else{
        c.set(3,squad_role);const auto second_order=compare(c,std::int32_t(squad_role),2);
        if(second_order==0||compare(c,std::int32_t(squad_role),3)==0){
            if(compare(c,current_age(n),19)<0)add_rating(n,std::uint32_t(reputation_adjustment(n,5)));
            else add_rating(n,std::uint32_t(older_reputation_adjustment(n)));
        }else if(compare(c,current_age(n),19)<0&&compare(c,std::int32_t(squad_role),4)==0)add_rating(n,std::uint32_t(older_reputation_adjustment(n)));
    }
    if(compare(c,current_age(n),20)>0){
        const auto attributes=c.u32(n.person+0x14);c.set(3,attributes);const auto trait=c.i16(attributes+4);c.set(0,extended(trait));
        if(compare(c,trait,45)<0)add_rating(n,0u-std::uint32_t(reputation_adjustment(n,6)));
    }
    if(compare(c,std::int32_t(std::uint32_t(n.suitability)),120)>0){
        const auto raw=c.u8(n.employer+0x240);c.set(0,raw);const auto ambition=std::int8_t(raw);c.set(3,std::uint32_t(std::int32_t(ambition)));
        if(compare(c,ambition,10)>0){const auto bonus=std::uint32_t(std::int32_t(ambition))-10;c.set(0,extended(std::int16_t(bonus)));add_rating(n,extended(std::int16_t(bonus)));}
    }
    const auto manager=c.u32(n.employer+0x90);c.set(3,manager);if(!nonzero_logical(c,manager))return;
    const auto attributes=c.u32(manager+0x1c);c.set(0,attributes);if(!nonzero_logical(c,attributes))return;
    const auto controlled=n.call(0x1014c2e0,0x100891cc,{n.employer,1,1,0,0});const auto human=std::int8_t(controlled);c.set(0,std::uint32_t(std::int32_t(human)));
    if(!nonzero(c,human)){
        const auto latest=c.u32(n.employer+0x90);c.set(3,latest);const auto traits=c.u32(latest+0x1c);c.set(3,traits);const auto raw=c.u8(traits+0x42);c.set(0,raw);const auto trait=std::int8_t(raw);c.set(3,std::uint32_t(std::int32_t(trait)));
        const auto bonus=extended(std::int16_t(std::uint32_t(std::int32_t(trait))-10));c.set(0,bonus);c.set(0,std::uint32_t(n.special_club_or_floor)+bonus);n.special_club_or_floor=extended(std::int16_t(std::uint32_t(n.special_club_or_floor)+bonus));
    }
    const auto latest=c.u32(n.employer+0x90);c.set(4,latest);const auto joined_year=c.u16(n.person+0x4e);c.set(5,joined_year);
    const auto manager_date=c.u32(latest+0x4c);c.set(3,manager_date);const auto date_extra=c.u32(latest+0x50);c.set(0,date_extra);c.put32(n.frame.at(0x3c),manager_date);
    const auto manager_year=c.u16(n.frame.at(0x3e));c.set(3,manager_year);c.put32(n.frame.at(0x40),date_extra);
    const auto year_order=compare_logical(c,manager_year,joined_year);bool joined_after;
    if(year_order<0){joined_after=true;c.set(0,1);}else if(year_order>0){joined_after=false;c.set(0,0);}else{
        const auto manager_day=c.i16(n.frame.at(0x3c));c.set(0,extended(manager_day));const auto joined_day=c.i16(n.person+0x4c);c.set(4,extended(joined_day));
        const auto day_xor=extended(joined_day)^extended(manager_day);c.set(0,day_xor);const auto half=std::int32_t(day_xor)>>1;c.set(3,std::uint32_t(half));c.s.wr<1>(guest::reg::xer_ca,std::int32_t(day_xor)<0&&(day_xor&1));
        const auto shared_sign=day_xor&extended(joined_day);c.set(0,shared_sign);const auto difference=std::uint32_t(half)-shared_sign;c.set(0,difference);joined_after=(difference>>31)!=0;c.set(0,joined_after);
    }
    c.set(0,joined_after);if(!nonzero(c,joined_after))return;
    const auto player_attributes=c.u32(n.person+0x14);c.set(3,player_attributes);const auto rating=std::int16_t(std::uint32_t(n.special_club_or_floor));c.set(0,extended(rating));const auto ability=c.i16(player_attributes+6);c.set(3,extended(ability));
    if(compare(c,rating,ability)>=0)return;
    const auto difference=extended(ability)-extended(rating);c.set(3,difference);c.set(0,0x43300000);c.set(3,difference^0x80000000u);c.put32(n.frame.at(0x44),0x43300000);
    const auto bias=c.f64(n.constants+8);c.set_f(1,bias);c.put32(n.frame.at(0x48),c.r(3));const auto half=c.f64(n.constants+0xb8);c.set_f(2,half);const auto encoded=c.f64(n.frame.at(0x44));c.set_f(0,encoded);
    const auto value=fp.fsub(encoded,bias);c.set_f(0,value);const auto bonus=fp.fmul(half,value);c.set_f(0,bonus);c.put64(n.frame.at(0x4c),fp.fctiwz(0,bonus));const auto integer=c.u32(n.frame.at(0x50));c.set(0,integer);add_rating(n,integer);
}

void monetary_floor(Context& n) {
    auto& c=n.c;auto& fp=n.fp;
    const auto reputation=c.i16(n.employer+0x1ec);c.set(0,extended(reputation));c.set(3,0x43300000);c.put32(n.frame.at(0x44),0x43300000);c.set(3,extended(reputation)^0x80000000u);
    const auto bias=c.f64(n.constants+8);c.set_f(2,bias);c.put32(n.frame.at(0x48),c.r(3));const auto slope=c.f64(n.constants+0x3d0);c.set_f(3,slope);const auto encoded=c.f64(n.frame.at(0x44));c.set_f(1,encoded);
    const auto intercept=c.f64(n.constants+0x228);c.set_f(0,intercept);const auto rep=fp.fsub(encoded,bias);c.set_f(1,rep);auto limit=c.f64(n.constants+0x3c0);c.set_f(2,limit);
    const auto candidate=fp.multiply_add(0x100892d8,slope,rep,intercept);c.set_f(0,candidate);fp.fcmp(0,limit,candidate);
    if(!(limit>candidate)){limit=c.f64(n.constants+0x3c8);c.set_f(2,limit);fp.fcmp(0,limit,candidate);if(!(limit<candidate)){limit=candidate;c.set_f(2,limit);}}
    const auto limit_bits=fp.fctiwz(0,limit);const auto rating=std::int16_t(std::uint32_t(n.special_club_or_floor));c.set(4,extended(rating));c.put64(n.frame.at(0x4c),limit_bits);
    const auto integer_limit=c.u32(n.frame.at(0x50));c.set(5,integer_limit);const auto narrowed_limit=std::int16_t(integer_limit);c.set(3,extended(narrowed_limit));auto chosen=integer_limit;c.set(6,chosen);
    if(compare(c,rating,narrowed_limit)<0){chosen=n.special_club_or_floor;c.set(6,chosen);}const auto score=std::int16_t(chosen);c.set(4,extended(score));
    if(compare(c,score,150)>0&&nonzero_logical(c,n.tenure_or_shortlist)&&compare(c,reputation,2500)>=0){
        const auto division=c.u32(n.employer+0x2c);c.set(3,division);const auto raw=c.u8(division+0x11f);c.set(3,raw);const auto level=std::int8_t(raw);c.set(3,std::uint32_t(std::int32_t(level)));
        if(compare(c,level,10)>0){
            const auto excess=extended(score)-150;c.set(3,excess);c.set(5,200);const auto scaled=excess*4;c.set(3,scaled);const auto units=compare(c,std::int32_t(scaled),200)<0?scaled:200u;if(units<200)c.set(5,units);
            c.set(4,0x43300000);c.set(3,extended(reputation)^0x80000000u);c.put32(n.frame.at(0x50),c.r(3));c.set(5,extended(std::int16_t(units)));const auto intermediate=extended(std::int16_t(units))*250;c.set(3,intermediate);
            const auto local_bias=c.f64(n.constants+8);c.set_f(2,local_bias);c.put32(n.frame.at(0x4c),0x43300000);const auto divisor=c.f64(n.constants+0x3d8);c.set_f(0,divisor);const auto rep_encoded=c.f64(n.frame.at(0x4c));c.set_f(1,rep_encoded);
            const auto square=extended(std::int16_t(units))*intermediate;c.set(3,square);c.put32(n.frame.at(0x44),0x43300000);const auto rep_value=fp.fsub(rep_encoded,local_bias);c.set_f(1,rep_value);const auto fraction=fp.fdiv(rep_value,divisor);c.set_f(0,fraction);
            c.set(3,square^0x80000000u);c.put32(n.frame.at(0x48),c.r(3));const auto square_encoded=c.f64(n.frame.at(0x44));c.set_f(1,square_encoded);const auto square_value=fp.fsub(square_encoded,local_bias);c.set_f(1,square_value);const auto extra=fp.fmul(square_value,fraction);c.set_f(0,extra);c.put64(n.frame.at(0x54),fp.fctiwz(0,extra));n.transfers_or_age_gap=c.u32(n.frame.at(0x58));
        }
    }
    c.set(7,extended(score));c.set(5,0x43300000);auto square=extended(score)*250;c.set(4,square);c.set(6,std::uint32_t(n.transfers_or_age_gap)^0x80000000u);const auto division=c.u32(n.employer+0x2c);c.set(3,division);c.put32(n.frame.at(0x4c),0x43300000);square*=extended(score);c.set(4,square);
    const auto raw_level=c.u8(division+0x11f);c.set(7,raw_level);const auto floor_bias=c.f64(n.constants+8);c.set_f(3,floor_bias);const auto level=std::int8_t(raw_level);c.set(3,std::uint32_t(std::int32_t(level)));const auto divisor=c.f64(n.constants+0x3e0);c.set_f(0,divisor);c.put32(n.frame.at(0x48),c.r(6));
    c.set(4,square-0x370000);c.put32(n.frame.at(0x44),0x43300000);const auto quadratic=square-3600000u;c.set(4,quadratic);const auto level_order=compare(c,level,13);c.set(3,quadratic^0x80000000u);
    const auto extra_encoded=c.f64(n.frame.at(0x44));c.set_f(1,extra_encoded);c.put32(n.frame.at(0x50),c.r(3));const auto extra_value=fp.fsub(extra_encoded,floor_bias);c.set_f(2,extra_value);const auto quadratic_encoded=c.f64(n.frame.at(0x4c));c.set_f(1,quadratic_encoded);const auto quadratic_value=fp.fsub(quadratic_encoded,floor_bias);c.set_f(1,quadratic_value);
    const auto adjusted=fp.fdiv(quadratic_value,divisor);c.set_f(0,adjusted);const auto amount=fp.fadd(extra_value,adjusted);c.set_f(0,amount);c.put64(n.frame.at(0x54),fp.fctiwz(0,amount));n.transfers_or_age_gap=c.u32(n.frame.at(0x58));
    bool scale_reputation=level_order<0;if(!scale_reputation){const auto kind=c.u8(n.employer+0x23d);c.set(3,kind);scale_reputation=compare(c,kind,3)==0;}
    if(!scale_reputation)return;
    const auto kind=c.u8(n.employer+0x23d);c.set(3,kind);std::uint32_t adjusted_rep=extended(reputation);c.set(4,adjusted_rep);
    if(compare(c,kind,3)==0&&compare(c,reputation,5250)<0){
        c.set(3,extended(reputation)^0x80000000u);c.set(0,0x43300000);c.put32(n.frame.at(0x48),c.r(3));const auto local_bias=c.f64(n.constants+8);c.set_f(1,local_bias);c.put32(n.frame.at(0x44),0x43300000);const auto coefficient=c.f64(n.constants+0x2c0);c.set_f(2,coefficient);const auto encoded_rep=c.f64(n.frame.at(0x44));c.set_f(0,encoded_rep);const auto value=fp.fsub(encoded_rep,local_bias);c.set_f(0,value);const auto scaled=fp.fmul(coefficient,value);c.set_f(0,scaled);c.put64(n.frame.at(0x4c),fp.fctiwz(0,scaled));adjusted_rep=c.u32(n.frame.at(0x50));c.set(4,adjusted_rep);
    }
    c.set(0,std::uint32_t(std::int32_t(level)));
    if(compare(c,level,5)<0){
        const auto narrowed=std::int16_t(adjusted_rep);c.set(3,extended(narrowed));if(compare(c,narrowed,4000)>0){
            const auto excess=extended(narrowed)-4000;c.set(3,excess);c.set(0,0x43300000);c.set(3,excess^0x80000000u);c.put32(n.frame.at(0x44),0x43300000);const auto local_bias=c.f64(n.constants+8);c.set_f(1,local_bias);c.put32(n.frame.at(0x48),c.r(3));const auto factor=c.f64(n.constants+0x3e8);c.set_f(2,factor);const auto excess_encoded=c.f64(n.frame.at(0x44));c.set_f(0,excess_encoded);const auto value=fp.fsubs(excess_encoded,local_bias);c.set_f(0,value);const auto scaled=fp.fmul(factor,value);c.set_f(0,scaled);c.put64(n.frame.at(0x4c),fp.fctiwz(0,scaled));const auto integer=c.u32(n.frame.at(0x50));c.set(3,integer);c.set(0,integer+4000);adjusted_rep=extended(std::int16_t(integer+4000));c.set(4,adjusted_rep);
        }
    }
    const auto narrowed=std::int16_t(adjusted_rep);c.set(0,extended(narrowed));if(compare(c,narrowed,5250)>=0)return;
    c.set(3,0x43300000);c.set(0,extended(narrowed)^0x80000000u);c.put32(n.frame.at(0x50),c.r(0));c.set(0,std::uint32_t(n.transfers_or_age_gap)^0x80000000u);const auto local_bias=c.f64(n.constants+8);c.set_f(2,local_bias);c.put32(n.frame.at(0x4c),0x43300000);const auto scale=c.f64(n.constants+0x3f0);c.set_f(0,scale);const auto rep_encoded=c.f64(n.frame.at(0x4c));c.set_f(1,rep_encoded);c.put32(n.frame.at(0x48),c.r(0));const auto rep_value=fp.fsub(rep_encoded,local_bias);c.set_f(1,rep_value);c.put32(n.frame.at(0x44),0x43300000);const auto fraction=fp.fdiv(rep_value,scale);c.set_f(0,fraction);const auto amount_encoded=c.f64(n.frame.at(0x44));c.set_f(1,amount_encoded);const auto value=fp.fsub(amount_encoded,local_bias);c.set_f(1,value);const auto scaled=fp.fmul(value,fraction);c.set_f(0,scaled);c.put64(n.frame.at(0x54),fp.fctiwz(0,scaled));n.transfers_or_age_gap=c.u32(n.frame.at(0x58));
}

void scale_floor(Context& n, unsigned factor_offset, unsigned input=0x44, unsigned output=0x4c) {
    auto& c=n.c;auto& fp=n.fp;c.set(3,std::uint32_t(n.transfers_or_age_gap)^0x80000000u);c.set(0,0x43300000);c.put32(n.frame.at(input+4),c.r(3));const auto bias=c.f64(n.constants+8);c.set_f(1,bias);c.put32(n.frame.at(input),0x43300000);const auto factor=c.f64(n.constants+factor_offset);c.set_f(2,factor);const auto encoded=c.f64(n.frame.at(input));c.set_f(0,encoded);const auto value=fp.fsub(encoded,bias);c.set_f(0,value);const auto scaled=fp.fmul(factor,value);c.set_f(0,scaled);c.put64(n.frame.at(output),fp.fctiwz(0,scaled));n.transfers_or_age_gap=c.u32(n.frame.at(output+4));
}

bool selected_flag(Cpu& c, std::uint32_t flags, std::uint32_t mask) {const auto selected=flags&mask;c.set(0,selected);return nonzero(c,std::int32_t(selected));}

void floor_terms_and_profile(Context& n) {
    auto& c=n.c;auto terms=c.u32(n.contract+0x50);c.set(4,terms);
    if(nonzero(c,std::int32_t(terms))){
        const auto promise=c.i8(n.contract+0x4c);c.set(0,std::uint32_t(std::int32_t(promise)));if(compare(c,promise,-50)<0)scale_floor(n,0x2c0);
        if(selected_flag(c,terms,0x800000))scale_floor(n,0x170,0x54,0x5c);
        const auto flags=n.call(0x1037fa70,0x100895b4,{n.contract});if(selected_flag(c,flags,8))scale_floor(n,0x3f8,0x64,0x6c);
        terms=c.u32(n.contract+0x50);c.set(4,terms);if(selected_flag(c,terms,1))scale_floor(n,0x168,0x74,0x7c);if(selected_flag(c,terms,0x40))scale_floor(n,0x168);
    }
    const auto raw_first=c.u8(n.person+0x65);c.set(3,raw_first);const auto first=std::int8_t(raw_first);c.set(0,std::uint32_t(std::int32_t(first)));
    bool weak=false;if(compare(c,first,6)<0){const auto raw_second=c.u8(n.person+0x67);c.set(0,raw_second);const auto second=std::int8_t(raw_second);c.set(0,std::uint32_t(std::int32_t(second)));weak=compare(c,second,10)<0;}
    if(weak){
        const auto promise=c.i8(n.contract+0x4c);c.set(0,std::uint32_t(std::int32_t(promise)));if(nonzero(c,promise)&&promise<0)scale_floor(n,0x168);
        if(!nonzero_logical(c,n.buyer))return;if(!nonzero_logical(c,n.employer))return;
        const auto buyer_rep=c.i16(n.buyer+0x1ec);c.set(3,extended(buyer_rep));const auto employer_rep=c.i16(n.employer+0x1ec);c.set(0,extended(employer_rep));const auto adjusted=extended(buyer_rep)-750;c.set(3,adjusted);
        if(compare(c,std::int32_t(adjusted),employer_rep)>0)scale_floor(n,0x168);
        return;
    }
    c.set(0,std::uint32_t(std::int32_t(first)));if(compare(c,first,15)<=0)return;
    const auto promise=c.i8(n.contract+0x4c);c.set(0,std::uint32_t(std::int32_t(promise)));nonzero(c,promise);if(promise>=0)scale_floor(n,0x28);
}
}

void young_value_floor(Context& n) {
    auto& c=n.c;if(compare(c,std::int32_t(std::uint32_t(n.age)),24)>=0)return;
    const auto enabled=std::int8_t(std::uint32_t(n.option));c.set(0,std::uint32_t(std::int32_t(enabled)));if(!nonzero(c,enabled))return;
    if(compare(c,std::int32_t(std::uint32_t(n.suitability)),110)<=0)return;
    const auto division=c.u32(n.employer+0x2c);c.set(0,division);if(!nonzero_logical(c,division))return;
    const auto squad_role=c.u8(n.contract+0x60)&15u;c.set(0,squad_role);if(compare(c,std::int32_t(squad_role),7)==0)return;
    const auto attributes=c.u32(n.person+0x14);c.set(5,attributes);n.special_club_or_floor=n.suitability;n.transfers_or_age_gap=0;
    const auto goalkeeper=c.u8(attributes+0xf);c.set(0,goalkeeper);if(compare(c,goalkeeper,20)!=0)outfield_profile(n,attributes);
    contextual_rating(n);
    const auto rating=std::int16_t(std::uint32_t(n.special_club_or_floor));c.set(0,extended(rating));if(compare(c,rating,120)<=0)return;
    monetary_floor(n);floor_terms_and_profile(n);
    if(compare(c,std::int32_t(std::uint32_t(n.price)),std::int32_t(std::uint32_t(n.transfers_or_age_gap)))>=0)return;
    const auto report=nonzero_logical(c,n.diagnostics);n.price=n.transfers_or_age_gap;if(report){const auto flags=extended(c.i16(n.diagnostics))|0x2000u;c.set(0,flags);c.put16(n.diagnostics,flags);}
}
}
