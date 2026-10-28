#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::int32_t readiness_rating(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10227140, 0x80, {.first_gpr = 30, .gpr_offset = -0x28, .first_fpr = 28});
    const auto doubles = c.toc(-0x51e8);
    keep(c, {{30, player}, {31, doubles}});
    c.set_f(30, c.f64(doubles + 0x10)); c.set_f(28, c.f64(doubles + 0x88));
    Fpu fp(c);
    const auto club = [&] {
        const auto side = c.i8(player + 0x27); compare(c, side);
        return c.u32(c.u32(c.u32(player + 0x1a4) + 0x3118) + (side ? 0x20 : 0x1c));
    };
    const auto convert = [&](std::int32_t value, unsigned at) {
        c.put32(frame.at(at), 0x43300000); c.put32(frame.at(at + 4), std::uint32_t(value) ^ 0x80000000);
        return fp.fsub(c.f64(frame.at(at)), c.f64(doubles + 0x80));
    };
    c.set(4, club()); c.set(3, c.u32(player + 0x6c)); c.call(0x100ca010, 0x10227198);
    c.set_f(31, convert(std::int8_t(c.r(3)), 0x50)); c.set_f(1, c.f64(doubles + 0x80));
    const auto team = club();
    const auto height = c.i16(c.u32(player + 0x70) + 0x0c), team_height = c.i16(team + 0x1ec);
    c.set(3, std::uint32_t(std::int32_t(height))); c.set(4, std::uint32_t(std::int32_t(team_height)));
    c.set_f(3, c.f64(doubles + 0x80)); c.set_f(2, convert(height - team_height, 0x48));
    c.set_f(1, fp.fdiv(c.f(2), c.f64(doubles + 0x98)));
    c.set_f(29, fp.fadd(c.f64(doubles + 0x90), c.f(1)));
    fp.fcmp(0, c.f64(doubles + 0xa0), c.f(29));
    if (c.cr(0) & 4) c.set_f(29, c.f64(doubles + 0xa0));
    else { fp.fcmp(0, c.f64(doubles + 0xa8), c.f(29)); if (c.cr(0) & 8) c.set_f(29, c.f64(doubles + 0xa8)); }
    c.set(5, club()); c.set(4, c.u32(player + 0x6c)); c.set(3, c.u32(c.toc(-0x7b7c)));
    c.call(0x10082f90, 0x10227270);
    if (compare_logical(c, c.r(3)) != 0) {
        const auto history = c.r(3); const auto raw_trait = c.u8(history + 0x4c);
        c.set(5, raw_trait); c.set(4, 0x43300000);
        c.set(3, std::uint32_t(std::int32_t(std::int8_t(raw_trait))) ^ 0x80000000);
        c.set_f(1, convert(std::int8_t(raw_trait), 0x48)); c.set_f(2, c.f64(doubles + 0x28));
        c.set_f(28, fp.fadd(c.f(2), fp.fdiv(c.f(1), c.f64(doubles + 0xb0))));
        const auto category = c.u8(history + 0x60) & 15;
        if (compare_logical(c, category, 8) <= 0) {
            const auto destination = c.u32(c.toc(-0x51ec) + category * 4);
            c.set(3, destination); c.s.wr<4>(reg::ctr, destination);
            const auto adjustment = [&]() -> std::pair<unsigned, bool> {
                switch (category) {
                case 0: case 5: return {0xb8, false};
                case 1: return {0xc0, true};
                case 2: return {0xc8, true};
                case 4: return {0x08, false};
                case 6: return {0xd0, false};
                case 7: return {0x40, false};
                case 8: return {0xd8, false};
                default: return {0, false};
                }
            }();
            if (adjustment.first) c.set_f(28, adjustment.second ? fp.fadd(c.f(28), c.f64(doubles + adjustment.first)) :
                fp.fsub(c.f(28), c.f64(doubles + adjustment.first)));
        }
    }
    c.set(3, club()); c.call(0x100c7d00, 0x10227354);
    double age;
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        c.set(3, c.u32(player + 0x6c));
        c.put32(frame.at(0x48), 0x43300000); c.put32(frame.at(0x4c), c.u8(c.r(3) + 0x5e));
        c.set_f(1, c.f64(doubles + 0xf0)); c.set_f(2, c.f64(doubles + 0xb0));
        age = fp.fmul(c.f(2), fp.fsub(c.f64(frame.at(0x48)), c.f(1)));
    } else {
        const auto staff = c.u32(player + 0x6c);
        leave(c, {{3, c.toc(-0x7b80)}, {4, c.u32(staff + 0x4c)}, {5, c.u32(staff + 0x50)}});
        c.call(0x10718130, 0x1022739c);
        c.set(3, c.r(3) ^ 0x80000000); c.put32(frame.at(0x50), 0x43300000); c.put32(frame.at(0x54), c.r(3));
        c.set_f(1, c.f64(doubles + 0x80)); age = fp.fsub(c.f64(frame.at(0x50)), c.f(1));
    }
    c.set_f(2, age); fp.fcmp(0, age, c.f64(doubles + 0xe0));
    if (c.cr(0) & 4) {
        c.set_f(1, fp.fsub(age, c.f64(doubles + 0xe0)));
        c.set_f(30, fp.fadd(c.f(30), fp.fdiv(c.f(1), c.f64(doubles + 0xe8))));
    }
    const auto trait = c.i8(c.u32(player + 0x70) + 0x2f); c.set(3, c.u8(c.u32(player + 0x70) + 0x2f));
    c.set_f(1, fp.fdiv(fp.multiply_add(0x102273e8, c.f(31), c.f(31), c.f64(doubles + 0x18)), c.f64(doubles + 0xb8)));
    c.set_f(3, c.f64(doubles + 0x80)); c.set_f(2, convert(trait, 0x40));
    c.set_f(1, fp.multiply_add(0x10227418, c.f(2), c.f(30), c.f(1)));
    c.set_f(1, fp.fadd(c.f(29), c.f(1))); c.set_f(31, fp.fadd(c.f(28), c.f(1)));
    c.put64(frame.at(0x38), fp.fctiwz(0, fp.fadd(c.f64(doubles + 0x28), c.f(31))));
    return std::int32_t(result(c, c.u32(frame.at(0x3c))));
}
}
