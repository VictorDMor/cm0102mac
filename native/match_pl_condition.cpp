#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void refresh_condition(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x102393c0, 0x60, {.first_gpr = 25});
    keep(c, {{30, c.toc(-0x51e8)}, {29, player}, {25, c.toc(-0x7b90)}, {26, c.toc(-0x7b8c)},
             {27, c.toc(-0x7b88)}, {28, c.toc(-0x7b84)}, {31, c.toc(-0x51e0)}});
    enter(c, 0x102393f0, {player}); refresh_attributes(c, player);
    const auto seed = c.r(25), begin = c.r(26), end = c.r(27), cursor = c.r(28);
    const auto next = c.u32(cursor) + 2;
    c.set(3, next); const auto ordering = compare_logical(c, next, c.u32(end));
    c.put32(cursor, next);
    if (ordering > 0) {
        c.put32(cursor, begin); c.call(0x1077b770, 0x10239410); c.put32(seed, c.r(3) & 0xffff);
    }
    const auto at = c.u32(cursor), bias = c.u32(seed);
    const auto sum = std::uint32_t(c.u16(at)) + bias;
    const auto product = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(sum)) * 0x2e8ba2e9) >> 32);
    c.s.wr<1>(reg::xer_ca, product < 0 && (std::uint32_t(product) & 1));
    leave(c, {{3, std::int32_t(sum) < 0 ? 1u : 0u}, {4, sum}});
    if (compare(c, std::int32_t(sum) % 11) != 0) return;

    const auto drift_trait = c.i8(player + 0x103);
    const auto profile = c.u32(player + 0x70);
    const auto exertion = c.i8(player + 0x102);
    const auto raw_stamina = c.u8(profile + 0x25);
    const auto stamina = std::int8_t(raw_stamina);
    const auto combined = std::int32_t(stamina) + std::int32_t(drift_trait) / 2;
    const auto target = (combined + 10) / 4;
    const auto quarter = combined + 10;
    // addze overflows only when an arithmetic -1 receives the shift carry.
    c.s.wr<1>(reg::xer_ca, quarter < 0 && quarter > -4);
    leave(c, {{3, std::uint32_t(combined)}, {4, raw_stamina}, {5, std::uint32_t(std::int32_t(exertion))}});
    if (compare(c, exertion, target) > 0) {
        c.set(3, c.u8(player + 0x102)); c.put8(player + 0x102, c.r(3) - 1);
    }
    const auto doubles = c.r(30), singles = c.r(31);
    Fpu fp(c);
    c.set_f(2, c.f32(player + 0x50));
    const auto drift = c.f32(player + 0x5c);
    c.set_f(1, c.f32(singles + 0x34));
    c.put_f32(player + 0x50, float(fp.fadds(c.f(2), drift)));
    fp.fcmp(0, c.f(1), c.f32(player + 0x5c));
    if (c.cr(0) & 2) c.put32(player + 0x5c, c.u32(singles));
    else {
        c.set_f(1, c.f32(player + 0x5c));
        c.put_f32(player + 0x5c, float(fp.frsp(fp.fmul(c.f(1), c.f64(doubles + 0x28)))));
    }
    if (compare_logical(c, c.u8(player + 1), 50) > 0) {
        const auto load = c.u8(player + 1);
        c.set(3, load);
        c.put32(frame.at(0x38), 0x43300000); c.put32(frame.at(0x3c), (std::uint32_t(load) - 50) ^ 0x80000000);
        c.set_f(1, fp.fsub(c.f64(frame.at(0x38)), c.f64(doubles + 0x80)));
        c.set_f(2, c.f64(doubles + 0x168));
        c.call(0x10780fc0, 0x10239510);
        c.set(2, c.u32(frame.at(0x14)));
        c.set_f(1, fp.frsp(c.f(1))); c.set_f(2, c.f32(singles + 0x70));
        c.set_f(1, fp.fmuls(c.f(2), c.f(1)));
        fp.fcmp(0, c.f(1), c.f64(doubles + 0x2d8));
        if (c.cr(0) & 4) c.set_f(1, c.f32(singles + 0xb4));
        c.put_f32(player + 0x50, float(fp.fsubs(c.f32(player + 0x50), c.f(1))));
    }
    c.set_f(1, c.f64(doubles + 0x10));
    auto condition = double(c.f32(player + 0x50));
    fp.fcmp(0, c.f(1), condition);
    if (c.cr(0) & 4) condition = c.f(1);
    else {
        c.set_f(1, c.f64(doubles + 0x2e0));
        fp.fcmp(0, c.f(1), c.f32(player + 0x50));
        condition = c.cr(0) & 8 ? c.f(1) : c.f32(player + 0x50);
    }
    c.put_f32(player + 0x50, float(fp.frsp(condition)));
}

void consume_condition(Cpu& c, std::uint32_t player, std::int16_t action, double work) {
    const auto doubles = c.u32(c.r(2) - 0x51e8);
    const auto singles = c.u32(c.r(2) - 0x51e0);
    c.set(7, doubles); compare(c, action, -1); c.set(6, singles);
    Fpu fp(c);
    if (action == -1) {
        unsigned factor = 0xcc;
        for (const auto [threshold, coefficient] : {std::pair{0xb8u, 0x64u},
                 std::pair{0xc0u, 0x5cu}, std::pair{0xc4u, 0xc8u}}) {
            c.set_f(2, c.f32(player + 0x50));
            fp.fcmp(0, c.f(2), c.f32(singles + threshold));
            if (c.cr(0) & 4) { factor = coefficient; break; }
        }
        c.set_f(2, c.f32(singles + factor));
        c.set_f(1, fp.fmuls(c.f(2), work));
        c.set_f(2, c.f32(player + 0x50));
        const auto cost = fp.fdivs(c.f(1), c.f32(singles + 0xbc));
        const auto condition = fp.fsubs(c.f(2), cost);
        c.put_f32(player + 0x50, float(condition));
        c.set_f(1, c.f32(player + 0x50)); fp.fcmp(0, c.f(1), c.f32(singles));
        // bgelr is !LT, including an unordered comparison.
        if (c.cr(0) & 8) c.put32(player + 0x50, c.u32(singles));
        return;
    }

    c.set(4, std::uint32_t(std::int32_t(action)));
    const auto index = std::uint32_t(std::int32_t(action)) - 19;
    auto destination = 0x10239738u;
    if (compare_logical(c, index, 27) <= 0) {
        destination = c.u32(c.u32(c.r(2) - 0x5200) + index * 4);
        c.set(4, destination); c.s.wr<4>(reg::ctr, destination);
    }
    // The pinned original table has exactly these three formula targets.
    // A foreign destination keeps the original tail-transfer contract.
    if (destination != 0x10239690 && destination != 0x102396e4 && destination != 0x10239738) {
        c.s.pc = destination; run_until(c.s, c.lr()); return;
    }
    const auto factor = destination == 0x10239690 ? 0x28u : destination == 0x102396e4 ? 0x170u : 0x2f8u;
    const auto fused_site = destination + 0x10;
    const auto resistance = c.f32(player + 0xd8);
    auto cost = fp.multiply_add(fused_site, c.f64(doubles + 0x2e8), resistance, c.f64(doubles + 0x38));
    c.set_f(3, c.f64(doubles + factor)); c.set_f(4, c.f64(doubles + 0x1f0));
    c.set_f(5, c.f32(player + 0x50));
    cost = fp.fmul(c.f(3), cost); cost = fp.fmul(c.f(4), cost); c.set_f(2, cost);
    c.set_f(1, fp.frsp(fp.fdiv(cost, c.f64(doubles + 0x2f0))));
    c.set_f(1, fp.fsubs(c.f(5), c.f(1))); c.put_f32(player + 0x50, float(c.f(1)));
    c.set_f(1, c.f32(player + 0x50)); fp.fcmp(0, c.f(1), c.f32(singles));
    if (c.cr(0) & 8) c.put32(player + 0x50, c.u32(singles));
}
}
