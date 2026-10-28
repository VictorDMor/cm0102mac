#include "euro_champ_qual_detail.hpp"
#include "guest_fpu.hpp"
namespace cm::aot::euro_champ_qual {
using namespace detail;
namespace {
void advance_random(Cpu& c,const Program& p,std::uint32_t cursor_variable,std::uint32_t end_variable,
                    std::uint32_t start,std::uint32_t seed_variable,std::uint32_t back) {
    const auto next=c.u32(cursor_variable)+2; c.set(3,next); c.put32(cursor_variable,next);
    if(compare_logical(c,next,c.u32(end_variable))>0) {
        c.put32(cursor_variable,start); p.call(0x1077b770,back); c.put32(seed_variable,c.r(3)&0xffff);
    }
}
std::uint32_t small_sample(Cpu& c,std::uint32_t bound,std::uint32_t cursor_variable,std::uint32_t seed_variable) {
    c.set(3,c.u32(cursor_variable)); compare(c,bound); const auto sum=c.u16(c.r(3))+c.u32(seed_variable);
    c.set(3,sum);
    const auto remainder=std::int32_t(std::int64_t(std::int32_t(sum))%std::int32_t(bound));
    return std::int32_t(bound)>0?std::uint32_t(remainder):0u-std::uint32_t(remainder);
}
}
std::uint32_t random_below(Cpu& c,std::uint32_t bound) {
    Frame f(c,0x104fbe40,0x80,{.first_gpr=20}); Program p{c}; Fpu fp(c);
    Kept<30> limit(c,bound); keep(c,{{25,c.toc(-0x459c)},{26,c.toc(-0x7b90)},{27,c.toc(-0x7b8c)},
        {28,c.toc(-0x7b88)},{29,c.toc(-0x7b84)}});
    if(compare(c,bound)==0) return 0;
    advance_random(c,p,c.r(29),c.r(28),c.r(27),c.r(26),0x104fbe94);
    c.set(3,0x10000);
    const bool above=compare(c,bound,65535)>0;
    if(!above) { c.set(3,0xffff0000); if(compare(c,bound,-65535)>=0) return small_sample(c,bound,c.r(29),c.r(26)); }
    const auto constants=c.r(25), cursor=c.u32(c.r(29));
    c.put32(f.at(0x4c),bound^0x80000000u); c.put32(f.at(0x48),0x43300000);
    c.set_f(2,c.f64(constants+8));
    c.set_f(1,fp.fsub(c.f64(f.at(0x48)),c.f(2)));
    c.set_f(1,fp.fdiv(c.f(1),c.f64(constants)));
    const auto sample=c.u16(cursor); const auto sum=std::uint32_t(sample)+c.u32(c.r(26));
    const auto wrapped_product=std::int32_t(bound*10u);
    Kept<22> reduced(c,std::uint32_t(wrapped_product/65535));
    const auto quotient=std::int32_t(sum)/65535;
    const auto remainder=std::uint32_t(std::int32_t(sum)%65535);
    c.put32(f.at(0x40),0x43300000); c.put32(f.at(0x44),remainder^0x80000000u);
    c.set_f(0,fp.fsub(c.f64(f.at(0x40)),c.f(2))); c.set_f(0,fp.fmul(c.f(1),c.f(0)));
    c.put64(f.at(0x38),fp.fctiwz(0,c.f(0))); Kept<31> sampled(c,c.u32(f.at(0x3c)));
    // Explicit wrapping multiplication bounds the reduced range to
    // [-32768,32768]; all deeper inlined wide-range branches are unreachable.
    const auto q_pre=std::int32_t((std::int64_t(std::int32_t(0x80008001u))*std::int32_t(sum))>>32);
    const auto q_sum=std::int32_t(std::uint32_t(q_pre)+sum);
    const auto q_shift=q_sum>>15;
    const auto reduced_pre=std::int32_t((std::int64_t(std::int32_t(0x80008001u))*wrapped_product)>>32);
    const auto reduced_shift=std::int32_t(std::uint32_t(reduced_pre)+std::uint32_t(wrapped_product))>>15;
    leave(c,{{3,cursor},{4,std::uint32_t(reduced_shift)<0x80000000u?0u:1u},{5,std::uint32_t(quotient)*65535u},
        {6,std::uint32_t(q_shift)<0x80000000u?0u:1u},{7,sum},{8,sample},{9,0x43300000}});
    std::uint32_t additional=0;
    if(compare(c,reduced)!=0) {
        advance_random(c,p,c.r(29),c.r(28),c.r(27),c.r(26),0x104fbf80);
        c.set(3,0x10000); compare(c,reduced,65535); c.set(3,0xffff0000); compare(c,reduced,-65535);
        additional=small_sample(c,reduced,c.r(29),c.r(26));
    }
    c.set(24,additional);
    const auto correction=std::int32_t(std::uint32_t(additional)+5)/10;
    sampled=std::uint32_t(sampled)+std::uint32_t(correction); c.set(4,std::uint32_t(additional)+5);
    if(compare(c,sampled,std::int32_t(bound-1))>0) sampled=bound-1;
    // The division-by-ten sequence leaves the quotient sign correction.
    const auto signed_add=std::int32_t(std::uint32_t(additional)+5);
    const auto high=std::int32_t((std::int64_t(0x66666667)*signed_add)>>32)>>2;
    c.set(4,std::uint32_t(high)>>31);
    return sampled;
}
}
