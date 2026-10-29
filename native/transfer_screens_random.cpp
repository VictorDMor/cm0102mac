#include "transfer_screens_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
std::uint32_t sample_transfer_range(Cpu& c,std::uint32_t bound_word) {
    Frame frame(c,0x10425980,0x80,{.first_gpr=23});
    Kept<24> salt(c,c.toc(-0x7b90));
    Kept<25> table(c,c.toc(-0x7b8c));
    Kept<26> end(c,c.toc(-0x7b88));
    Kept<27> cursor(c,c.toc(-0x7b84));
    Kept<28> constants(c,c.toc(-0x4ad4));
    Kept<29> bound(c,bound_word);
    Kept<30> coarse(c,c.r(30));
    Kept<31> correction(c,c.r(31));
    if(compare(c,std::int32_t(bound_word))==0) return 0;
    const auto advance=[&](std::uint32_t back) {
        const auto next=c.u32(cursor)+2;
        c.set(3,next);
        const auto ordering=compare_logical(c,next,c.u32(end));
        c.put32(cursor,next);
        if(ordering>0) {
            c.put32(cursor,table); c.call(0x1077b770,back); c.put32(salt,c.r(3)&0xffff);
        }
    };
    advance(0x104259d4);
    c.set(3,65535);
    const bool wide_above=compare(c,std::int32_t(bound_word),65535)>0;
    bool wide=wide_above;
    if(!wide_above) { c.set(3,std::uint32_t(-65535)); wide=compare(c,std::int32_t(bound_word),-65535)<0; }
    if(!wide) {
        compare(c,std::int32_t(bound_word));
        c.set(3,c.u16(c.u32(cursor))+c.u32(salt));
        const auto quotient=std::uint32_t(divide_signed<4>(c.r(3),bound_word));
        coarse=c.r(3)-quotient*bound_word;
        return std::int32_t(bound_word)>0 ? std::uint32_t(coarse) : 0u-std::uint32_t(coarse);
    }
    Fpu fp(c);
    c.put32(frame.at(0x4c),bound_word^0x80000000); c.put32(frame.at(0x48),0x43300000);
    const auto position=c.u32(cursor);
    c.set_f(2,c.f64(constants+0x10)); c.set_f(1,c.f64(frame.at(0x48))); c.set_f(0,c.f64(constants+0x18));
    c.set_f(1,fp.fsub(c.f(1),c.f(2)));
    const auto word=c.u16(position); const auto bias=c.u32(salt);
    c.put32(frame.at(0x40),0x43300000);
    c.set_f(2,fp.fdiv(c.f(1),c.f(0))); c.set_f(1,c.f64(constants+0x10));
    const auto sum=std::uint32_t(word)+bias;
    const auto residual=std::uint32_t(std::int32_t(sum)%65535);
    c.put32(frame.at(0x44),residual^0x80000000);
    c.set_f(0,fp.fsub(c.f64(frame.at(0x40)),c.f(1)));
    const auto wrapped_product=std::int32_t(bound_word*10u);
    const auto reduced=wrapped_product/65535;
    const auto high=std::int32_t((std::int64_t(std::int32_t(0x80008001u))*wrapped_product)>>32);
    const auto adjusted=std::int32_t(std::uint32_t(high)+std::uint32_t(wrapped_product));
    c.s.wr<1>(reg::xer_ca,adjusted<0 && (std::uint32_t(adjusted)&0x7fff)!=0);
    c.set_f(0,fp.fmul(c.f(2),c.f(0))); compare(c,reduced);
    c.put64(frame.at(0x38),fp.fctiwz(0,c.f(0))); coarse=c.u32(frame.at(0x3c));
    c.set({{3,std::uint32_t(reduced)<0x80000000u ? 0u : 1u},{4,residual^0x80000000},
        {5,65535},{6,std::int32_t(sum)<0 ? 1u : 0u},{7,sum},{8,word}});

    // The wrapped signed32 product divided by 65535 is in [-32768,32768].
    // The original's deeper wide arms and recursive call are unreachable;
    // its refinement either consumes one narrow draw or skips a zero bound.
    if(reduced==0) correction=0;
    else {
        advance(0x10425ad4);
        c.set(3,65535); compare(c,reduced,65535);
        c.set(3,std::uint32_t(-65535)); compare(c,reduced,-65535);
        const auto value=std::uint32_t(c.u16(c.u32(cursor)))+c.u32(salt);
        const auto quotient=std::uint32_t(divide_signed<4>(value,std::uint32_t(reduced)));
        const auto rest=value-std::uint32_t(quotient)*std::uint32_t(reduced);
        c.s.wr<1>(reg::xer_ca,adjusted<0 && (std::uint32_t(adjusted)&0x7fff)!=0);
        compare(c,reduced);
        correction=reduced>0 ? rest : 0u-rest;
        c.set({{3,std::uint32_t((adjusted>>15)<0 ? 1u : 0u)},{4,std::uint32_t(quotient)*std::uint32_t(reduced)},
            {5,std::uint32_t(reduced)},{6,value}});
    }
    const auto numerator=std::int32_t(std::uint32_t(correction)+5);
    const auto tenth_high=std::int32_t((std::int64_t(numerator)*0x66666667)>>32);
    c.s.wr<1>(reg::xer_ca,tenth_high<0 && (std::uint32_t(tenth_high)&3u)!=0);
    c.set(4,tenth_high<0 ? 1u : 0u);
    coarse=std::uint32_t(coarse)+std::uint32_t(numerator/10);
    const auto maximum=bound_word-1;
    if(compare(c,std::int32_t(std::uint32_t(coarse)),std::int32_t(maximum))>0) coarse=maximum;
    return coarse;
}
}
