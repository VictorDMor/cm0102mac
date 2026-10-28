#include "inter_amer_cup_detail.hpp"

namespace cm::aot::inter_amer_cup {
using namespace detail;
std::uint32_t calendar(Cpu& c, std::uint32_t address, std::uint32_t stage,
                       std::uint32_t round_count, std::uint32_t extra, std::uint32_t flag) {
    Frame f(c, 0x106ec1a0, 0x80, {.first_gpr = 28});
    Kept<28> object(c, address);
    keep(c, {{31, c.toc(slot::pool)}, {30, c.toc(-0x7ba8)}});
    Program p{c};
    if (compare(c, std::int8_t(stage), -1) != 0) return 0;
    if (compare_logical(c, flag) != 0) c.put8(flag, 0);
    c.put16(round_count, 1); c.put16(extra, 0);
    Kept<29> schedule(c, p.call(0x10775130, 0x106ec1fc, {0x78}));
    if (compare(c, schedule) == 0) {
        assertion(c, f, 0x106ec218, 0x48, 0x40, 0x9a);
        c.set(4, c.toc(-0x7bac)); return 0;
    }
    p.call(0x104854f0, 0x106ec27c, {schedule, 0, 10, 9, 0, 5, extended(Cup{c, object}.year())});
    c.put32(f.at(0x38), 0);
    p.call(0x10485570, 0x106ec2a8, {schedule, 0, 26, 0, 1, 2, 2, extended(Cup{c, object}.year())});
    const auto round = std::uint32_t(schedule);
    c.put16(round + 8, 0); c.put16(round + 10, 0);
    c.put16(round + 12, 0); c.put16(round + 14, 0);
    c.put8(round + 24, 8); c.put16(round + 26, 2); c.put16(round + 28, 1);
    c.put16(round + 30, 2); c.put16(round + 16, 3); c.put16(round + 32, 0);
    c.put8(round + 34, 0); c.put8(round + 35, 2); c.put8(round + 36, 7);
    c.put32(round + 108, 0); c.put32(round + 112, 0); c.put32(round + 116, 0);
    leave(c, {{4, 3}, {5, 1}, {6, 2}, {7, 8}, {8, 0}});
    return schedule;
}
}
