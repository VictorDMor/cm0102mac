#include "guest_abi.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

void announce_goal(Cpu& c) {
    const auto actor = c.r(15);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x201a);
    c.set(7, c.u8(actor + 0x27));
    c.set(8, -1);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, -1);
    c.set(6, c.u8(actor + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, 0x10263700);
}

void publish_goal(Cpu& c) {
    const auto actor = c.r(15);
    c.set(5, c.toc(-0x51e0));
    c.set(4, actor);
    c.set(3, c.u32(actor + 0x1a4));
    c.set_f(1, c.f32(c.r(5) + 0x4c));
    c.call(0x101d6780, 0x10267168);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(5, 0x96);
    c.set(4, c.u8(actor + 0x27));
    c.call(0x101f4860, 0x10267178);
    c.set(0, c.u8(actor + 0x27));
    c.set(5, -0x87);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, std::int8_t(c.r(0)) == 0);
    c.set(4, c.r(0));
    c.call(0x101f4860, 0x10267198);
    c.set(0, c.u8(actor + 0x27));
    c.set(5, 0x21a3);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(6, 0);
    c.set(0, std::uint32_t(std::int8_t(c.r(0)) * 0x36));
    c.set(3, c.r(3) + 0x10000);
    c.set(4, c.r(3) + c.r(0));
    const auto score = c.r(4) - 0x1fc3;
    c.set(3, c.u8(score));
    c.set(0, c.r(3) + 1);
    c.s.pc = 0x102671c0;
    c.put8(score, c.r(0));
    c.s.pc = 0x102671c4;
}

void return_shot(Cpu& c) {
    const auto caller = c.sp() + 0x1b0;
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.s.wr<8>(reg::fpr(31), c.u64(caller - 8));
    c.s.wr<8>(reg::fpr(30), c.u64(caller - 0x10));
    c.s.wr<8>(reg::fpr(29), c.u64(caller - 0x18));
    c.set_lr(c.r(0));
    auto saved = caller - 0x6c;
    for (unsigned reg = 13; reg != 32; ++reg, saved += 4) {
        c.s.wr<4>(2432, saved);
        c.set(reg, c.u32(saved));
    }
    c.s.wr<4>(2432, saved);
    c.finish();
}
}
