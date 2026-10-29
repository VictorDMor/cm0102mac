#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void open_transfer_feedback(Cpu& c, std::uint32_t player, std::uint32_t club,
                            std::uint32_t reason_value, std::uint32_t kind_value) {
    Frame f(c, 0x10420f00, 0x60, {.first_gpr = 26});
    Kept<26> person(c,player);
    Kept<27> recipient(c,club);
    Kept<28> reason(c,reason_value);
    Kept<29> kind(c,kind_value);
    Kept<30> transfers(c,c.toc(slot::transfers));
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    if (!byte_result(c,p.call(0x1073e4b0,0x10420f44,{screens,c.toc(-0x4af0),c.toc(-0x4af4),1,0,0}))) return;
    p.history(0x10420f60,screens,0,person);
    p.history(0x10420f74,screens,1,recipient);
    p.history(0x10420f88,screens,2,extended(std::int16_t(std::uint32_t(reason))));
    p.history(0x10420f9c,screens,4,std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(kind)))));
    if (compare(c,std::int16_t(std::uint32_t(reason)),18) != 0 &&
        compare(c,std::int16_t(std::uint32_t(reason)),32) != 0) return;
    p.call(0x10738c00,0x10420fbc,{f.at(0x38)});
    p.call(0x103eaea0,0x10420fd0,{c.u32(transfers),person,f.at(0x38),0});
    c.put32(f.at(0x44),c.u32(f.at(0x3c)));
    for (;;) {
        c.set(4,p.call(0x10738e00,0x10421044,{f.at(0x38)}));
        if (compare(c,c.r(4)) == 0) break;
        p.call(0x103eb440,0x10420fe8,{c.u32(transfers),c.u32(c.r(4))});
        c.set(3,c.u32(c.r(3)));
        bool completed = false;
        for (int status : {11,12,13,14,19}) {
            if (compare(c,c.u8(c.r(3) + 0x34),status) == 0) { completed = true; break; }
        }
        if (completed) p.history(0x1042103c,screens,3,c.u32(c.r(3) + 8));
    }
    p.call(0x10738c20,0x10421058,{f.at(0x38),missing});
}
}
