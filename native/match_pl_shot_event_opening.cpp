#include "guest_fpu.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

constexpr std::uint32_t entry = 0x102576e0;
constexpr std::uint32_t setup_publication = 0x102578d8;
constexpr std::int32_t clock_multiplier = 0x2e8ba2e9;

static std::int32_t shift_clock_product(Cpu& c, std::uint32_t value) {
    const auto high = std::int32_t((std::int64_t(std::int32_t(value)) * clock_multiplier) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 1));
    return high >> 1;
}

struct ZoneSites {
    std::uint32_t home_column, home_row, away_column, away_row;
    unsigned column_register, destination_fpr;
};
static void load_shot_zone(Cpu& c, std::uint32_t actor, bool home, const ZoneSites& sites) {
    c.set(0, c.u8(actor + 0x105));
    c.set(3, std::uint32_t(std::int8_t(c.r(0)) - 4));
    c.call(0x10775540, home ? sites.home_column : sites.away_column);
    if (!home) c.set(0, c.r(3));
    const auto horizontal = std::int8_t(c.r(home ? 3 : 0));
    if (home) c.set(0, c.u8(actor + 0x106));
    else c.set(3, c.u8(actor + 0x106));
    c.set(sites.column_register, std::uint32_t(horizontal));
    c.set(3, std::uint32_t(std::int8_t(c.r(home ? 0 : 3)) - (home ? 11 : 0)));
    c.call(0x10775540, home ? sites.home_row : sites.away_row);
    const auto offset = std::int8_t(c.r(3)) * 0x24 + std::int8_t(c.r(sites.column_register)) * 4;
    c.set(0, std::uint32_t(offset));
    c.set(3, c.toc(-0x6424));
    c.set_f(sites.destination_fpr, c.f32(c.r(3) + c.r(0)));
}

static void publish_setup_time(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    c.set(5, c.u32(actor + 0x1a4));
    c.set({{6, 0x2e8c0000}, {4, 0x2e8c0000}, {0, 0x43300000}});
    c.set(5, std::uint32_t(c.i16(c.r(5) + 0x7948)));
    c.set(6, clock_multiplier);
    c.set(8, c.u8(actor + 0x27));
    c.set(4, clock_multiplier);
    c.set(7, c.u32(actor + 0x1a4));
    const auto initial_clock = c.r(5);
    c.set(5, std::uint32_t((std::int64_t(std::int32_t(initial_clock)) * clock_multiplier) >> 32));
    c.put8(stack + 0x11d, c.r(3));
    c.set_f(2, c.f64(c.r(27) + 0x80));
    c.put8(c.r(7) + 0x77c4, c.r(8));
    c.set_f(0, c.f64(c.r(27) + 0x350));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(5, std::uint32_t(shift_clock_product(c, initial_clock)));
    c.put32(stack + 0x114, c.r(0));
    const auto live_clock = std::uint32_t(c.i16(c.r(3) + 0x7948));
    c.set(0, c.r(5) + (c.r(5) >> 31));
    c.set(6, c.u32(actor + 0x1a4));
    c.set(3, std::uint32_t((std::int64_t(std::int32_t(live_clock)) * clock_multiplier) >> 32));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.put8(stack + 0x11c, c.r(0));
    c.set(0, std::uint32_t(shift_clock_product(c, live_clock)));
    c.set(3, c.r(0) >> 31);
    c.set(0, c.r(0) + c.r(3));
    c.s.pc = setup_publication;
    c.put16(c.r(6) + 0x77c2, c.r(0));
    c.s.pc = setup_publication + 4;
}

void open(Cpu& c, GuestNativeScope& scope) {
    c.s.step(entry);
    const auto caller = c.sp();
    c.set(0, c.lr());
    c.put64(caller - 8, c.s.r<8>(reg::fpr(31)));
    c.put64(caller - 0x10, c.s.r<8>(reg::fpr(30)));
    c.put64(caller - 0x18, c.s.r<8>(reg::fpr(29)));
    auto saved = caller - 0x6c;
    for (unsigned reg = 13; reg != 32; ++reg, saved += 4) {
        c.s.wr<4>(2432, saved);
        c.put32(saved, c.r(reg));
    }
    c.s.wr<4>(2432, saved);
    c.set(15, c.r(3));
    c.set(27, c.toc(-0x51e8));
    c.set(16, c.r(5));
    c.set(28, c.toc(-0x7b90));
    c.set(17, c.r(7));
    c.set(29, c.toc(-0x7b8c));
    c.set(18, c.r(8));
    c.set(30, c.toc(-0x7b88));
    c.set(19, c.r(9));
    c.set(31, c.toc(-0x7b84));
    c.set(23, 0);
    c.put32(caller + 8, c.r(0));
    const auto stack = caller - 0x1b0;
    c.put32(stack, caller);
    c.set(1, stack);
    scope.frame(stack);
    c.put8(stack + 0x1cf, c.r(4));
    c.put16(stack + 0x1d6, c.r(6));

    const auto actor = c.r(15);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.u32(actor + 0x1a4));
    const auto side = c.i8(actor + 0x27);
    c.set(3, c.u32(c.r(3) + 0x310c));
    c.set(4, c.u32(c.r(4) + 0x3108));
    c.set(0, std::uint32_t(side));
    c.compare(0, side, 0);
    c.put32(stack + 0x138, c.r(3));
    c.put32(stack + 0x134, c.r(4));
    load_shot_zone(c, actor, side == 0, {0x10257764, 0x10257778, 0x102577a8, 0x102577bc, 13, 0});

    c.set(0, std::uint32_t(c.i16(actor + 0x3a)));
    c.set({{4, 0}, {3, 0}});
    c.s.wr<8>(reg::fpr(30), c.s.r<8>(reg::fpr(0)));
    c.put16(stack + 0x12e, c.r(0));
    c.set(13, 0xf0);
    c.set(0, c.u8(actor + 0x105));
    c.set(20, 6);
    c.put16(stack + 0x130, c.r(16));
    c.put8(stack + 0x12a, c.r(0));
    c.set(0, c.u8(actor + 0x106));
    c.put8(stack + 0x42, c.r(4));
    c.set(4, actor);
    c.put8(stack + 0x41, c.r(3));
    c.set(3, c.toc(-0x51e0));
    c.put8(stack + 0x129, c.r(0));
    c.set(0, 0);
    c.set_f(29, c.f32(c.r(3) + 0x34));
    c.put8(stack + 0x128, 0);
    c.set(0, std::uint32_t(c.i16(actor + 0x19c)));
    c.set(3, c.u32(actor + 0x1a4));
    c.put16(stack + 0x11e, c.r(0));
    c.set(0, 0);
    for (unsigned offset = 0x126; offset >= 0x120; --offset)
        c.put8(stack + offset, 0);
    c.call(0x101feba0, 0x1025786c);
    publish_setup_time(c);
}

void assess_opening(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    Fpu fp(c);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, c.u32(c.r(3) + 0x310c) ^ 0x80000000u);
    c.put32(stack + 0x118, c.r(0));
    c.set_f(1, fp.fsub(c.f64(stack + 0x114), c.f(2)));
    fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 4) c.set_cr(0, c.cr(0) | 2);
    if (c.cr(0) & 2) {
        c.set(0, 1);
        c.put8(stack + 0x124, 1);
    }

    c.set(3, c.toc(-0x51e0));
    c.set(0, c.u8(stack + 0x1cf));
    c.set_f(0, c.f32(c.r(3) + 0x8c));
    c.set(3, 0x66660000);
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set_f(1, c.f64(c.r(27) + 0xc0));
    c.set_f(0, fp.fmuls(c.f(0), c.f(30)));
    c.compare(0, std::int32_t(c.r(0)), 0x39);
    c.set(3, 0x66666667);
    c.set_f(0, fp.fadd(c.f(1), c.f(0)));
    c.put64(stack + 0x10c, fp.fctiwz(0, c.f(0)));
    const auto converted = std::int8_t(c.u32(stack + 0x110));
    const auto high = std::int32_t((std::int64_t(converted) * 0x66666667) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 1));
    const auto shifted = std::uint32_t(high >> 1);
    c.set(3, shifted >> 31);
    c.set(0, std::uint32_t(std::int8_t((shifted + c.r(3)) * 5)));
    c.put8(stack + 0x127, c.r(0));
    if (!(c.cr(0) & 2)) {
        c.set(0, c.u8(stack + 0x1cf));
        c.compare(0, std::int32_t(c.r(0)), 3);
    }
    if (c.cr(0) & 2) c.set(20, 5);
    c.set(0, c.u8(stack + 0x1cf));
    c.compare(0, std::int32_t(c.r(0)), 0x33);
    if (c.cr(0) & 2) {
        const auto power = std::uint32_t(std::int16_t(c.r(13)));
        c.set(0, power >> 31);
        const auto dividend = std::int32_t(power + c.r(0));
        c.set(3, std::uint32_t(dividend));
        c.s.wr<1>(reg::xer_ca, dividend < 0 && (std::uint32_t(dividend) & 1));
        c.set(0, std::uint32_t(dividend >> 1));
        c.set(13, std::uint32_t(std::int16_t(c.r(0))));
    }

    c.set(4, c.u8(actor + 0x27));
    c.set(3, 1);
    c.set(0, c.u8(actor + 0x27));
    c.set(5, 1);
    c.set(6, std::uint32_t(std::int8_t(c.r(4))));
    c.set(3, 11);
    c.set(4, c.r(6) - c.r(5));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(0)), 0);
    const auto side_mask = std::int32_t(~(c.r(4) | (c.r(5) - c.r(6))));
    c.s.wr<1>(reg::xer_ca, side_mask < 0 && (std::uint32_t(side_mask) & 0x7fffffff));
    c.set(0, std::uint32_t(side_mask >> 31));
    c.set(14, c.r(3) & ~c.r(0));
    load_shot_zone(c, actor, (c.cr(0) & 2) != 0, {0x102579d4, 0x102579e8, 0x10257a18, 0x10257a2c, 21, 1});
    c.set_f(0, c.f64(c.r(27) + 0x10));
    c.set(3, actor);
    c.set(4, c.u8(stack + 0x1cf));
    c.set(8, std::uint32_t(std::int8_t(c.r(14))));
    c.set_f(1, fp.fadd(c.f(0), c.f(1)));
    c.set(5, c.u8(actor + 0x105));
    c.set(6, c.u8(actor + 0x106));
    c.set(4, std::uint32_t(std::int8_t(c.r(4))));
    c.set(10, stack + 0x50);
    c.set(7, 4);
    c.set_f(1, fp.frsp(c.f(1)));
    c.call(0x102818f0, 0x10257a78);
}
}
