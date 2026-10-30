#include "cash_detail.hpp"
#include <span>

namespace cm::aot::cash {
using namespace detail;
namespace {
enum class Cached {none,small,medium,large,scale};
struct Band {
    unsigned threshold,threshold_fpr,conversion_back,result_back;
    Cached cache,result;
    unsigned constant;
};
constexpr std::array<Band,8> large_bands{{
    {0x148,30,0x1000c7ac,0x1000c7d0,Cached::medium,Cached::none,0xc0},
    {0x150,0,0x1000c7e0,0x1000c800,Cached::none,Cached::none,0x158},
    {0x160,29,0x1000c810,0x1000c830,Cached::small,Cached::none,0x168},
    {0x170,28,0x1000c840,0x1000c85c,Cached::large,Cached::medium,0},
    {0x138,30,0x1000c86c,0x1000c88c,Cached::medium,Cached::none,0x178},
    {0x180,0,0x1000c89c,0x1000c8b8,Cached::none,Cached::small,0},
    {0x188,0,0x1000c8c8,0x1000c8e4,Cached::none,Cached::large,0},
    {0x190,0,0x1000c8f4,0x1000c914,Cached::none,Cached::none,0x198}
}};
constexpr std::array<Band,13> fine_bands{{
    {0x60,28,0x1000c930,0x1000c950,Cached::large,Cached::scale,0},
    {0x40,29,0x1000c960,0x1000c980,Cached::small,Cached::none,0xf0},
    {0x118,30,0x1000c990,0x1000c9ac,Cached::medium,Cached::large,0},
    {0xc0,0,0x1000c9bc,0x1000c9dc,Cached::none,Cached::none,0x1a0},
    {0x168,28,0x1000c9ec,0x1000ca08,Cached::large,Cached::small,0},
    {0x1a8,0,0x1000ca18,0x1000ca34,Cached::none,Cached::medium,0},
    {0x1b0,0,0x1000ca44,0x1000ca64,Cached::none,Cached::none,0x1b8},
    {0x160,29,0x1000ca74,0x1000ca90,Cached::small,Cached::large,0},
    {0x170,28,0x1000caa0,0x1000cac0,Cached::large,Cached::none,0x148},
    {0x138,30,0x1000cad0,0x1000caf0,Cached::medium,Cached::none,0x178},
    {0x180,0,0x1000cb00,0x1000cb1c,Cached::none,Cached::small,0},
    {0x188,0,0x1000cb2c,0x1000cb48,Cached::none,Cached::large,0},
    {0x190,0,0x1000cb58,0x1000cb78,Cached::none,Cached::none,0x198}
}};
}
std::uint64_t quantum(Cpu& c,std::uint64_t amount,unsigned flags) {
    Frame frame(c,0x1000c770,0x70,{.first_gpr=29,.first_fpr=28});Program p{c};Fpu fp(c);
    const auto numbers=c.toc(-0x59d4);
    keep(c,{{31,numbers},{30,unsigned(amount>>32)},{29,unsigned(amount)}});
    c.set(5,signed_byte(flags));
    const bool large=compare(c,c.r(5)&4)!=0;
    if(!large && compare(c,c.r(5)&8)==0)return result_pair(c,1000);
    const std::span<const Band> bands=large?std::span<const Band>(large_bands):std::span<const Band>(fine_bands);
    double scale=0,small_step=0,medium_step=0,large_step=0;
    for(unsigned index=0;index<bands.size();++index) {
        const auto& band=bands[index];
        p.call(0x10773c80,band.conversion_back,{unsigned(amount>>32),unsigned(amount)});
        c.set_f(0,c.f64(c.r(31)+band.threshold));
        if(index==0){scale=c.f64(c.r(2)-0x3afc);c.set_f(31,scale);}
        const auto threshold=fp.fmul(c.f(0),scale);c.set_f(band.threshold_fpr,threshold);
        switch(band.cache) {
        case Cached::small:small_step=threshold;break;
        case Cached::medium:medium_step=threshold;break;
        case Cached::large:large_step=threshold;break;
        default:break;
        }
        fp.fcmp(0,c.f(1),threshold);const auto cr=c.cr(0);
        c.set_cr(0,(cr&~2u)|((cr&10u)?2u:0u));
        if(!(c.cr(0)&2))continue;
        double step=0;
        switch(band.result) {
        case Cached::small:step=small_step;break;
        case Cached::medium:step=medium_step;break;
        case Cached::large:step=large_step;break;
        case Cached::scale:step=scale;break;
        case Cached::none:c.set_f(0,c.f64(c.r(31)+band.constant));step=fp.fmul(c.f(0),scale);break;
        }
        c.set_f(1,step);p.call(0x10773de0,band.result_back);return returned_pair(c);
    }
    c.set_f(1,medium_step);p.call(0x10773de0,large?0x1000c920:0x1000cb84);
    return returned_pair(c);
}
}
