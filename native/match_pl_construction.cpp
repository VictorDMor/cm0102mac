#include "match_pl_construction.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
std::int32_t signed_high(std::uint32_t value, std::uint32_t multiplier) {
    return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * std::int32_t(multiplier)) >> 32);
}
int arithmetic_shift(Cpu& c, int value, unsigned bits) {
    c.s.wr<1>(reg::xer_ca, value < 0 && (std::uint32_t(value) & ((1u << bits) - 1)));
    return value >> bits;
}
}

PlayerConstruction::PlayerConstruction(Cpu& cpu, std::uint32_t player, std::uint32_t match,
    std::uint32_t staff_address, std::uint32_t side, std::uint32_t initial, std::uint32_t initial_slot)
    : c(cpu), frame(c, 0x10223ac0, 0xd0, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 30}),
      fp(c), actor{c, player}, staff(staff_address), side_argument(side), slot(initial_slot),
      doubles(c.toc(-0x51e8)), floats(c.toc(-0x51e0)), seed(c.toc(-0x7b90)),
      begin(c.toc(-0x7b8c)), end(c.toc(-0x7b88)), cursor(c.toc(-0x7b84)), staff_count(c.toc(-0x7be4)) {
    keep(c, {{16, staff}, {18, slot}, {23, c.toc(-0x7b3c)}, {14, c.toc(-0x7b80)},
        {15, player}, {24, c.toc(-0x7b5c)}, {17, side_argument}, {13, c.toc(-0x7ba0)},
        {20, 0}, {25, staff_count}, {19, 0}, {26, doubles}, {27, seed}, {28, begin},
        {29, end}, {30, cursor}, {31, floats}});
    c.put32(player + 0x1a4, match); c.put32(player + 0x6c, staff);
    c.put8(player, initial); c.put8(player + 0x2f, slot);
}

void PlayerConstruction::advance(std::uint32_t back) {
    const auto previous = c.u32(cursor); c.set(3, previous);
    const auto limit = c.u32(end), next = previous + 2;
    c.set(3, next);
    const auto order = compare_logical(c, next, limit);
    c.put32(cursor, next);
    if (order > 0) {
        c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff);
    }
}

std::uint32_t PlayerConstruction::fallback_index(std::uint32_t count) {
    if (compare(c, std::int32_t(count)) == 0) return 0;
    advance(0x10223bb8);
    c.set(3, 65535);
    bool wide = compare(c, std::int32_t(count), 65535) > 0;
    if (!wide) {
        c.set(3, std::uint32_t(-65535)); wide = compare(c, std::int32_t(count), -65535) < 0;
    }
    if (!wide) {
        const auto draw_at = c.u32(cursor); c.set(3, draw_at);
        const bool positive = compare(c, std::int32_t(count)) > 0;
        const auto bias = c.u32(seed), draw = std::uint32_t(c.u16(draw_at)) + bias;
        c.set(3, draw);
        const auto quotient = std::uint32_t(divide_signed<4>(draw, count));
        const auto residual = draw - quotient * count;
        return positive ? residual : 0u - residual;
    }

    // The count is a full word here. Its wide arm is reachable, unlike the
    // byte-bounded draws in periodic balance. Keep the original signed wrap
    // before reduction and execute the owned rounding draw directly.
    c.set(3, count ^ 0x80000000); c.put32(at(0x60), c.r(3));
    c.set(6, 0x80010000); c.set(3, c.u32(cursor)); c.set(5, 0x10000);
    c.put32(at(0x5c), 0x43300000);
    c.set_f(2, c.f64(doubles + 0x80)); c.set(4, 0x80010000);
    c.set_f(1, c.f64(at(0x5c))); c.set(6, 0x80008001);
    c.set_f(0, c.f64(doubles)); c.set(5, 65535);
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    const auto word = c.u16(c.r(3)); c.set(8, word);
    const auto bias = c.u32(seed); c.set(7, bias); c.set(3, count * 10);
    c.put32(at(0x54), 0x43300000); c.set(4, 0x80008001);
    c.set_f(2, fp.fdiv(c.f(1), c.f(0)));
    const auto draw = std::uint32_t(word) + bias;
    const auto product = std::uint32_t(signed_high(draw, 0x80008001)) + draw;
    c.set_f(1, c.f64(doubles + 0x80));
    const auto shifted = arithmetic_shift(c, std::int32_t(product), 15);
    c.set(7, shifted < 0); c.set(6, std::uint32_t(shifted) + c.r(7));
    c.set(5, c.r(6) * 65535);
    const auto residual = draw - c.r(5); keep(c, {{22, residual}});
    c.put32(at(0x58), residual ^ 0x80000000);
    const auto scaled = count * 10;
    const auto reduction_product = std::uint32_t(signed_high(scaled, 0x80008001)) + scaled;
    c.set_f(0, c.f64(at(0x54))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    const auto reduction_shift = arithmetic_shift(c, std::int32_t(reduction_product), 15);
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    const auto correction_bound = std::uint32_t(reduction_shift) + unsigned(reduction_shift < 0);
    c.set(3, correction_bound);
    c.put64(at(0x4c), fp.fctiwz(0, c.f(0)));
    Kept<22> coarse(c, c.u32(at(0x50)));
    enter(c, 0x10223c90); sample_range(c, correction_bound);
    c.set(4, 0x66660000);
    const auto rounded = c.r(3) + 5; c.set(3, rounded); c.set(4, 0x66666667);
    const auto rounding_shift = arithmetic_shift(c, signed_high(rounded, 0x66666667), 2);
    c.set(3, rounding_shift); c.set(4, rounding_shift < 0);
    const auto correction = std::uint32_t(rounding_shift) + c.r(4); c.set(3, correction);
    const auto candidate = std::uint32_t(coarse) + correction;
    return compare(c, std::int32_t(candidate), std::int32_t(count - 1)) > 0 ? count - 1 : candidate;
}

void PlayerConstruction::choose_record() {
    if (compare_logical(c, c.u32(staff + 0x14)) != 0) {
        c.put32(actor.address + 0x70, c.u32(staff + 0x14));
    } else {
        const bool away = compare(c, std::int8_t(side_argument)) != 0;
        c.set(3, c.u32(actor.match() + 0x3118)); c.set(5, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
        c.set(8, c.r(5) + 0x1ee); c.set(3, c.toc(-0x7bb8)); c.set(4, c.toc(-0x51e4));
        c.set(5, c.u32(staff)); c.set(6, c.u32(staff + 0x28)); c.set(7, c.u32(staff + 0x2c));
        c.call(0x1077b720, 0x10223b80);
        c.set(3, c.toc(-0x74d4));
        Kept<21> count(c, c.u32(c.r(3))); Kept<22> selected(c, fallback_index(count));
        c.set(3, c.toc(-0x7b38)); c.set(3, c.u32(c.r(3)));
        c.put32(actor.address + 0x70, c.r(3) + std::uint32_t(selected) * 0x48);
    }
    bool synthetic = false;
    if (compare_logical(c, c.u32(actor.address + 0x6c)) != 0) {
        c.set(3, c.u32(actor.address + 0x6c));
        const auto count = c.i32(staff_count); c.set(3, c.u32(c.r(3)));
        synthetic = compare(c, std::int32_t(c.r(3)), count) >= 0;
    }
    if (synthetic) c.put8(actor.address + 0x17, 10);
    else { c.set(3, c.u32(actor.address + 0x70)); c.put8(actor.address + 0x17, c.u8(c.r(3) + 0x45)); }
    bool known = true;
    if (compare_logical(c, staff) != 0) {
        c.set(3, c.u32(staff)); known = compare(c, std::int32_t(c.r(3)), c.i32(staff_count)) < 0;
    }
    if (known) {
        // This intentionally reads the preexisting side. The supplied side
        // does not replace it until the later name/preference phase.
        const auto old_side = actor.side(); c.set(3, actor.match());
        const bool away = compare(c, std::int8_t(old_side)) != 0;
        c.set(3, c.u32(c.r(3) + 0x3118)); c.set(5, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
        c.set(3, c.toc(-0x7b7c)); c.set(4, staff); c.set(3, c.u32(c.r(3)));
        c.call(0x10082f90, 0x10223d8c); keep(c, {{20, c.r(3)}});
    }
}

void PlayerConstruction::copy_attributes_and_clubs() {
    c.set(3, c.u32(actor.address + 0x70));
    const bool away = compare(c, std::int8_t(side_argument)) != 0;
    c.put16(actor.address + 0x48, extended(c.i16(c.r(3) + 0xa)));
    for (const auto [source, destination] : {std::pair{8u, 0x4au}, {0xcu, 0x4cu}}) {
        c.set(3, c.u32(actor.address + 0x70)); c.put16(actor.address + destination, extended(c.i16(c.r(3) + source)));
    }
    c.set(3, c.u32(actor.match() + 0x3118));
    c.put32(actor.address + 0xfc, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    c.set(3, c.u32(actor.match() + 0x3118));
    keep(c, {{21, c.u32(c.r(3) + (away ? 0x1c : 0x20))}});
}

void PlayerConstruction::prepare_record() {
    choose_record(); copy_attributes_and_clubs();
    c.set(3, c.u32(c.r(13))); c.set(4, staff); c.set(5, 0);
    c.call(0x101878d0, 0x10223e08);
    throw std::logic_error("MATCH_PL constructor suffix is not reconstructed");
}
}
