#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
struct AttributeDraws {
    Cpu& c;
    std::uint32_t seed, begin, end, cursor;
    std::int32_t draw(std::int32_t bound, unsigned bound_register, unsigned result_register, std::uint32_t back) const {
        // Registers here describe the ABI boundary at a possible reseed call;
        // the signed sampling decision and result use ordinary local values.
        c.set(bound_register, std::uint32_t(bound));
        if (compare(c, bound) == 0) { c.set(result_register, 0); return 0; }
        const auto next = c.u32(cursor) + 2;
        c.set(3, next); const auto ordering = compare_logical(c, next, c.u32(end));
        c.put32(cursor, next);
        if (ordering > 0) {
            c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff);
        }
        const auto at = c.u32(cursor); compare(c, bound);
        const auto bias = c.u32(seed), sum = std::uint32_t(c.u16(at)) + bias;
        const auto quotient = std::uint32_t(divide_signed<4>(sum, std::uint32_t(bound)));
        const auto residual = sum - quotient * std::uint32_t(bound);
        const auto result = bound > 0 ? residual : 0u - residual;
        c.set(3, sum); c.set(result_register, result);
        return std::int32_t(result);
    }
};
}

void refresh_attributes(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10235ea0, 0x80, {.first_gpr = 23});
    const auto singles = c.toc(-0x51e0);
    keep(c, {{25, singles}, {28, player}, {30, c.toc(-0x51e8)}, {29, c.toc(-0x7b90)},
             {27, c.toc(-0x7b8c)}, {26, c.toc(-0x7b88)}, {31, c.toc(-0x7b84)}});
    const AttributeDraws draws{c, c.r(29), c.r(27), c.r(26), c.r(31)};
    Fpu fp(c);
    // The condition quotient narrows to signed16 before forming the bound.
    // Its range plus a signed-byte trait stays inside [-32921,32869]. All
    // other bounds are signed bytes, so the original inline wide arms cannot run.
    const auto adjusted_bound = [&](unsigned stack_offset) {
        c.set_f(1, c.f32(player + 0x50));
        const auto divisor = c.f32(singles + 0xa8);
        const auto baseline = c.i8(player + 0x111);
        const auto scaled = fp.fdivs(c.f(1), divisor);
        c.put64(frame.at(stack_offset), fp.fctiwz(0, scaled));
        const auto narrowed = std::int16_t(c.u32(frame.at(stack_offset + 4)));
        const auto loss = 25 - std::int32_t(narrowed);
        c.set(3, std::uint32_t(loss));
        c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(narrowed)) <= 25);
        return std::int32_t(baseline) - loss;
    };
    const auto first = draws.draw(adjusted_bound(0x48), 23, 24, 0x10235f24);
    const auto second = draws.draw(adjusted_bound(0x38), 25, 23, 0x102360b8);
    enter(c, 0x10236200, {std::uint32_t(first), std::uint32_t(second)});
    c.put8(player + 0x107, std::uint32_t(larger_value(c, first, second)));

    const auto pair = [&](std::uint32_t start, unsigned source, unsigned destination) {
        const auto a = draws.draw(c.i8(player + source), 24, 23, start + 0x34);
        const auto b = draws.draw(c.i8(player + source), 25, 24, start + 0x1a4);
        enter(c, start + 0x2ec, {std::uint32_t(a), std::uint32_t(b)});
        c.put8(player + destination, std::uint32_t(larger_value(c, a, b)));
    };
    const auto single = [&](std::uint32_t start, unsigned source, unsigned destination) {
        c.put8(player + destination, std::uint32_t(draws.draw(c.i8(player + source), 24, 23, start + 0x34)));
    };
    pair(0x10236204, 0x113, 0x10d);
    single(0x102364f4, 0x114, 0x10a);
    single(0x10236668, 0x115, 0x10b);
    single(0x102367dc, 0x116, 0x10c);
    pair(0x10236950, 0x117, 0x10e);
    pair(0x10236c40, 0x112, 0x109);
    single(0x10236f30, 0x118, 0x108);
    single(0x102370a4, 0x119, 0x110);
    c.put8(player + 0x10f, std::uint32_t(draws.draw(c.i8(player + 0x11a), 23, 24, 0x1023724c)));
}
}
