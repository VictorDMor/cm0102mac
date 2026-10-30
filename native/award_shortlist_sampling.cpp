#include "award_shortlist_random.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::award_shortlist {
using namespace detail;
std::uint32_t sample(Cpu& c,std::uint32_t bound) {
    Frame frame(c,0x104394c0,0x80,{.first_gpr=20});
    keep(c,{{25,c.toc(-0x4a70)},{26,c.toc(-0x7b90)},{27,c.toc(-0x7b8c)},
            {28,c.toc(-0x7b88)},{29,c.toc(-0x7b84)},{30,bound}});
    if (compare(c,bound)==0) return result(c,0);
    const SamplePool pool{c,c.r(26),c.r(27),c.r(28),c.r(29)};
    pool.advance(0x10439514);
    c.set(3,0x10000);
    const auto above=compare(c,bound,65535)>0;
    if (!above) c.set(3,0xffff0000);
    const auto wide=above || compare(c,bound,-65535)<0;
    if (!wide) {
        c.set(3,pool.position()); compare(c,bound);
        c.set(3,c.u16(c.r(3))+pool.bias());
        return result(c,oriented(c.r(3),std::int32_t(bound)));
    }
    Fpu fp(c);
    c.put32(frame.at(0x4c),bound^0x80000000);
    const auto at=pool.position();
    c.set(3,at); c.set(9,0x43300000);
    c.put32(frame.at(0x48),0x43300000);
    c.set_f(2,c.f64(c.r(25)+8)); c.set_f(1,c.f64(frame.at(0x48)));
    c.set_f(0,c.f64(c.r(25))); c.set_f(1,fp.fsub(c.f(1),c.f(2)));
    const auto word=c.u16(at);
    const auto bias=pool.bias();
    const auto sum=std::uint32_t(word)+bias;
    c.set_f(1,fp.fdiv(c.f(1),c.f(0))); c.put32(frame.at(0x40),0x43300000);
    const auto rest=remainder(sum,65535);
    const auto reduced=std::int32_t(bound*10u)/65535;
    c.set(22,std::uint32_t(reduced)); compare(c,reduced);
    c.put32(frame.at(0x44),rest^0x80000000);
    c.set_f(0,c.f64(frame.at(0x40))); c.set_f(0,fp.fsub(c.f(0),c.f(2)));
    c.set_f(0,fp.fmul(c.f(1),c.f(0)));
    c.put64(frame.at(0x38),fp.fctiwz(0,c.f(0))); c.set(31,c.u32(frame.at(0x3c)));
    leave(c,{{4,std::int32_t(bound*10u)<0?1u:0u},{5,sum-rest},
              {6,std::int32_t(sum)<0?1u:0u},{7,sum},{8,word}});
    // wrap32(bound*10)/65535 is always in [-32768,32768]. The original
    // deeper unrolled wide arms (including its recursive call) are unreachable.
    if (reduced) {
        pool.advance(0x10439600);
        c.set(3,0x10000); compare(c,reduced,65535);
        c.set(3,0xffff0000); compare(c,reduced,-65535);
        c.set(3,pool.position()); compare(c,reduced);
        c.set(3,c.u16(c.r(3))+pool.bias());
        c.set(24,oriented(c.r(3),reduced));
    } else c.set(24,0);
    const auto numerator=std::int32_t(c.r(24)+5);
    c.set(4,numerator<0?1:0);
    c.set(3,std::uint32_t(numerator/10));
    c.set(31,c.r(31)+c.r(3));
    const auto upper=bound-1;
    if (compare(c,c.r(31),upper)>0) c.set(31,upper);
    return result(c,c.r(31));
}
}
