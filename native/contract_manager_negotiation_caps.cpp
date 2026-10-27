#include "contract_manager_negotiation_detail.hpp"

namespace cm::aot::contract_manager::negotiation {
namespace {
void age_cap(Context& n) {
    auto& c=n.c;const auto attributes=c.u32(n.person+0x14);c.set(3,attributes);const auto goalkeeper=c.u8(attributes+0xf);c.set(0,goalkeeper);
    if(compare(c,goalkeeper,20)==0)n.age=std::uint32_t(n.age)-3;
    const auto age=std::int8_t(std::uint32_t(n.age));c.set(3,std::uint32_t(std::int32_t(age)));
    if(compare(c,age,28)<=0)return;
    const auto index=std::uint32_t(std::int32_t(age))-29;c.set(0,index);if(compare_logical(c,index,16)>0)return;
    const auto table=c.toc(-0x5748);c.set(3,table);c.set(0,index*4);const auto destination=c.u32(table+index*4);c.set(3,destination);c.s.wr<4>(guest::reg::ctr,destination);
    // The seventeen original cases implement nine age-dependent additions,
    // then the base value for ages38..45. Ages outside that table bypass it.
    constexpr std::uint32_t additions[]={5000000,4000000,3250000,2500000,1750000,1000000,500000,200000,100000};
    const auto base=std::uint32_t(n.manager_or_base),price=std::uint32_t(n.price);
    std::uint32_t selected;
    if(index<9){const auto increment=additions[index];c.set(3,base+((increment+0x8000u)&0xffff0000u));const auto limit=base+increment;c.set(0,limit);const auto below=compare(c,std::int32_t(price),std::int32_t(limit))<0;selected=below?price:limit;if(below)c.set(0,price);}
    else {const auto below=compare(c,std::int32_t(price),std::int32_t(base))<0;c.set(0,base);selected=below?price:base;if(below)c.set(0,price);}
    n.price=selected;
}

void country_adjustment(Context& n) {
    auto& c=n.c;auto& fp=n.fp;if(!nonzero_logical(c,n.employer))return;
    const auto division=c.u32(n.employer+0x2c);c.set(3,division);if(!nonzero_logical(c,division))return;
    const auto country=c.u32(division+0x68);c.set(3,country);const auto local_country=c.u32(n.country);c.set(0,local_country);if(compare(c,std::int32_t(country),std::int32_t(local_country))!=0)return;
    if(compare(c,std::int32_t(std::uint32_t(n.price)),std::int32_t(std::uint32_t(n.manager_or_base)))<=0)return;
    c.set(3,0x5c0000);c.set(0,6000000);
    if(compare(c,std::int32_t(std::uint32_t(n.price)),6000000)<0){
        const auto price_word=std::uint32_t(n.price)^0x80000000u,base_word=std::uint32_t(n.manager_or_base)^0x80000000u;
        c.set(3,0x43300000);c.set(0,price_word);c.put32(n.frame.at(0x58),price_word);c.set(4,base_word);const auto bias=c.f64(n.constants+8);c.set_f(6,bias);c.put32(n.frame.at(0x54),0x43300000);
        const auto divisor=c.f64(n.constants+0x1b8);c.set_f(0,divisor);const auto first_encoded=c.f64(n.frame.at(0x54));c.set_f(1,first_encoded);c.put32(n.frame.at(0x60),price_word);const auto first=fp.fsub(first_encoded,bias);c.set_f(1,first);
        const auto coefficient=c.f64(n.constants+0x160);c.set_f(2,coefficient);c.put32(n.frame.at(0x5c),0x43300000);const auto slope=c.f64(n.constants+0x268);c.set_f(4,slope);auto fraction=fp.fdiv(first,divisor);c.set_f(1,fraction);
        const auto second_encoded=c.f64(n.frame.at(0x5c));c.set_f(0,second_encoded);c.put32(n.frame.at(0x50),price_word);c.put32(n.frame.at(0x4c),0x43300000);const auto third_encoded=c.f64(n.frame.at(0x4c));c.set_f(3,third_encoded);c.put32(n.frame.at(0x48),base_word);fraction=fp.fmul(coefficient,fraction);c.set_f(1,fraction);c.put32(n.frame.at(0x44),0x43300000);
        const auto second=fp.fsub(second_encoded,bias);c.set_f(0,second);const auto base_encoded=c.f64(n.frame.at(0x44));c.set_f(5,base_encoded);const auto third=fp.fsub(third_encoded,bias);c.set_f(2,third);const auto base=fp.fsub(base_encoded,bias);c.set_f(3,base);
        const auto curved=fp.fmul(fraction,second);c.set_f(0,curved);auto quote=fp.multiply_add(0x10089970,slope,third,curved);c.set_f(0,quote);fp.fcmp(0,base,quote);
        if(base>quote){c.put32(n.frame.at(0x68),base_word);c.put32(n.frame.at(0x64),0x43300000);const auto selected=c.f64(n.frame.at(0x64));c.set_f(0,selected);quote=fp.fsub(selected,bias);c.set_f(0,quote);}
        c.put64(n.frame.at(0x6c),fp.fctiwz(0,quote));n.price=c.u32(n.frame.at(0x70));
    }else{
        c.set(3,0x43300000);c.set(0,std::uint32_t(n.price)^0x80000000u);c.put32(n.frame.at(0x50),c.r(0));c.set(0,std::uint32_t(n.manager_or_base)^0x80000000u);const auto bias=c.f64(n.constants+8);c.set_f(3,bias);c.put32(n.frame.at(0x4c),0x43300000);
        const auto factor=c.f64(n.constants+0x168);c.set_f(1,factor);const auto price_encoded=c.f64(n.frame.at(0x4c));c.set_f(0,price_encoded);c.put32(n.frame.at(0x48),c.r(0));const auto price=fp.fsub(price_encoded,bias);c.set_f(0,price);c.put32(n.frame.at(0x44),0x43300000);const auto base_encoded=c.f64(n.frame.at(0x44));c.set_f(2,base_encoded);auto quote=fp.fmul(factor,price);c.set_f(1,quote);const auto base=fp.fsub(base_encoded,bias);c.set_f(0,base);fp.fcmp(0,base,quote);
        if(base>quote){c.put32(n.frame.at(0x58),c.r(0));c.put32(n.frame.at(0x54),0x43300000);const auto selected=c.f64(n.frame.at(0x54));c.set_f(0,selected);quote=fp.fsub(selected,bias);c.set_f(1,quote);}
        c.put64(n.frame.at(0x5c),fp.fctiwz(0,quote));n.price=c.u32(n.frame.at(0x60));
    }
}
}

std::uint32_t final_price(Context& n) {
    age_cap(n);country_adjustment(n);auto& c=n.c;
    const auto reported=n.call(0x10380520,0x10089a00,{n.contract});const auto active=std::int8_t(reported);c.set(0,std::uint32_t(std::int32_t(active)));
    auto selected=std::uint32_t(n.price);
    if(nonzero(c,active)){const auto clause_limit=c.u32(n.contract+0x24);c.set(3,clause_limit);if(compare(c,std::int32_t(std::uint32_t(n.price)),std::int32_t(clause_limit))>=0)selected=clause_limit;}
    c.set(3,selected);return selected;
}
}
