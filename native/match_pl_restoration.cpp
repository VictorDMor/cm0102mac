#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void restore_condition(Cpu& c, std::uint32_t player, std::int8_t projection) {
    Frame frame(c, 0x10239790, 0x70, {.first_gpr = 27, .gpr_offset = -0x24, .first_fpr = 31});
    const auto doubles = c.toc(-0x51e8), singles = c.toc(-0x51e0), manager = c.toc(-0x7ba0);
    keep(c, {{28, player}, {30, doubles}, {27, c.r(4)}, {29, manager}, {31, singles}});
    c.set_f(31, c.f64(doubles + 0x10));
    if (compare_logical(c, c.u32(player + 0x70)) == 0) return;
    Fpu fp(c);
    const auto read_scale = [&](std::uint32_t back) {
        leave(c, {{3, c.u32(manager)}, {4, c.u32(c.u32(player + 0x6c))}});
        c.call(0x10187650, back);
    };
    read_scale(0x102397d8);
    if (compare(c, std::int16_t(c.r(3)), 1000) > 0) {
        read_scale(0x102397f4);
        const auto scale = std::int16_t(c.r(3)); c.set(3, std::uint32_t(std::int32_t(scale)));
        c.put32(frame.at(0x40), 0x43300000); c.put32(frame.at(0x44), std::uint32_t(std::int32_t(scale)) ^ 0x80000000);
        c.set_f(3, c.f64(doubles + 0x80)); c.set_f(1, c.f64(doubles + 0x308));
        c.set_f(2, fp.fsub(c.f64(frame.at(0x40)), c.f(3)));
        c.set_f(31, fp.multiply_add(0x1023981c, c.f(1), c.f(2), c.f64(doubles + 0x300)));
    }
    const auto convert = [&](std::int32_t value, unsigned offset) {
        c.put32(frame.at(offset), 0x43300000);
        c.put32(frame.at(offset + 4), std::uint32_t(value) ^ 0x80000000);
        return fp.fsub(c.f64(frame.at(offset)), c.f64(doubles + 0x80));
    };
    compare(c, projection);
    if (!projection) {
        enter(c, 0x10239830, {player}); current_role(c, player);
        const auto keeper = compare(c, std::int16_t(c.r(3)), 1) == 0;
        if (keeper || compare(c, c.i8(player + 0x19)) >= 0) {
            const auto returning = !keeper && compare(c, c.i8(player + 0x1f), -1) == 0;
            const auto product = c.i8(c.u32(player + 0x70) + 0x34) * (returning ? 12 : 10);
            c.set(3, std::uint32_t(product));
            const auto amount = convert(product + (returning ? 325 : 150), 0x40);
            if (keeper) {
                c.set_f(3, c.f64(doubles + 0x80)); c.set_f(2, amount);
                c.set_f(1, fp.frsp(fp.multiply_add(0x10239874, c.f(31), amount, c.f32(player + 0x50))));
                c.put_f32(player + 0x50, float(c.f(1)));
            } else {
                c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, amount);
                c.put_f32(player + 0x50, float(fp.frsp(fp.multiply_add(returning ? 0x102398e8 : 0x1023992c,
                    c.f(31), amount, c.f32(player + 0x50)))));
                c.set_f(1, c.f32(player + 0x50));
            }
            fp.fcmp(0, c.f(1), c.f64(doubles + 0x2e0));
            if (c.cr(0) & 4) c.put32(player + 0x50, c.u32(singles + 0xd0));
        }
    } else {
        auto minute = c.i8(player + 0x20);
        auto path = 0u;
        if (compare(c, minute, -1) <= 0) {
            minute = c.i8(player + 0x21); path = 1;
            if (compare(c, minute, -1) <= 0) path = 2;
        }
        if (path == 2 && compare(c, c.i8(player + 0x19)) < 0) {
            c.put32(player + 0x54, c.u32(player + 0x50));
        } else {
            bool interpolate = false;
            if (path != 2 && compare(c, minute, 45) > 0) interpolate = compare(c, minute, 65) < 0;
            const auto base = c.i8(c.u32(player + 0x70) + 0x34) * 12 + 85;
            if (interpolate) {
                const auto first = path == 0 ? 0x40u : 0x38u, second = path == 0 ? 0x38u : 0x40u;
                c.set(5, std::uint32_t(std::int32_t(minute))); c.set(4, c.r(5) ^ 0x80000000);
                c.set_f(1, convert(minute, first)); c.set_f(2, c.f64(doubles + 0x310));
                const auto fraction = fp.multiply_add(path == 0 ? 0x102399d0 : 0x10239b2c,
                    c.f(2), c.f(1), c.f64(doubles + 0x318));
                c.put32(frame.at(second), 0x43300000); c.set(3, std::uint32_t(base) ^ 0x80000000);
                c.put32(frame.at(second + 4), c.r(3));
                c.set_f(2, c.f64(doubles + 0x80)); c.set_f(3, fp.frsp(fraction));
                c.set_f(1, fp.fsubs(c.f64(frame.at(second)), c.f(2)));
                c.set_f(1, fp.fmuls(c.f(3), c.f(1)));
                c.put_f32(player + 0x54, float(fp.frsp(fp.multiply_add(path == 0 ? 0x102399f0 : 0x10239b4c,
                    c.f(31), c.f(1), c.f32(player + 0x50)))));
            } else {
                c.set(3, std::uint32_t(base - 85)); c.set_f(2, c.f64(doubles + 0x80));
                c.set_f(1, convert(base, 0x38));
                const auto site = path == 0 ? 0x10239a34u : path == 1 ? 0x10239b90u : 0x10239c4cu;
                c.put_f32(player + 0x54, float(fp.frsp(fp.multiply_add(site, c.f(31), c.f(1), c.f32(player + 0x50)))));
            }
            if (compare(c, c.i8(player + 0x1c)) == 0 && compare_logical(c, c.u32(player + 0x6c)) != 0) {
                leave(c, {{3, c.u32(manager)}, {4, c.u32(player + 0x6c)}, {5, 0}});
                c.call(0x101878d0, path == 0 ? 0x10239a68 : path == 1 ? 0x10239bc4 : 0x10239c80);
                if (compare(c, std::int16_t(c.r(3)), 6500) > 0) c.set(3, 6500);
                c.set(3, std::uint32_t(std::int32_t(std::int16_t(c.r(3)))) ^ 0x80000000);
                c.put32(frame.at(0x38), 0x43300000); c.put32(frame.at(0x3c), c.r(3));
                c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f32(player + 0x54));
                const auto available = fp.fsubs(c.f64(frame.at(0x38)), c.f(1));
                fp.fcmp(0, c.f(2), available);
                if (c.cr(0) & 8) c.put32(player + 0x54, c.u32(singles + 0xd4));
            }
        }
    }
    c.set_f(2, c.f32(player + 0x54));
    c.set_f(1, fp.fadds(c.f(2), c.f32(player + 0x5c)));
    c.put_f32(player + 0x54, float(c.f(1)));
    fp.fcmp(0, c.f(1), c.f64(doubles + 0x2e0));
    if (c.cr(0) & 4) c.put32(player + 0x54, c.u32(singles + 0xd0));
}
}
