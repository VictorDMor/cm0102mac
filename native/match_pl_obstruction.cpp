#include "match_pl_obstruction.hpp"

namespace cm::aot::match_pl {
using namespace guest;

PlayerObstruction::PlayerObstruction(Cpu& cpu)
    : c(cpu), frame(c, 0x10272240, 0xd0, {.first_gpr = 18, .gpr_offset = -0x48, .first_fpr = 31}),
      fp(c), actor{c, c.r(3)} {
    c.set(0, c.lr());
    c.set_f(31, c.f(1));
    c.set({{31, actor.address}, {21, c.r(4)}, {30, c.toc(-0x7ba8)}, {27, c.toc(-0x51e8)},
           {28, c.toc(-0x7b90)}, {26, c.r(6)}, {25, c.toc(-0x7b8c)}, {20, c.r(7)},
           {24, c.toc(-0x7b88)}, {22, 0}, {29, c.toc(-0x7b84)}, {23, c.toc(-0x51e4)}});
}

// After this stage, r30 is the signed starting threshold or r3 is a returned verdict.
bool PlayerObstruction::prepare() {
    c.set(3, actor.match());
    c.set(4, actor.side());
    c.call(0x101f1e10, 0x10272294);
    if (compare_logical(c, c.r(3), actor.address) == 0) return reject();
    c.set(3, actor.address);
    c.call(0x102775c0, 0x102722ac);
    const auto holds_ball = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(holds_ball));
    if (compare(c, holds_ball) != 0) return reject();
    c.set(3, actor.match());
    c.set(0, actor.side());
    c.set(4, c.u8(c.r(3) + 0x7924));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, std::uint32_t(std::int8_t(c.r(4))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) != 0) {
        c.set(0, std::uint32_t(c.i8(actor.address + 0x110)));
        if (compare(c, std::int32_t(c.r(0))) == 0) return reject();
    }

    c.set(3, actor.match());
    const auto phase = c.i8(c.r(3) + 0x791f);
    c.set(0, std::uint32_t(phase));
    struct Range { unsigned bound, shift; std::int32_t magic; std::uint32_t back; };
    constexpr Range ranges[]{
        {300, 5, 0x1b4e81b5, 0x10272344}, {400, 7, 0x51eb851f, 0x102723a4},
        {500, 5, 0x10624dd3, 0x10272404}, {1000, 6, 0x10624dd3, 0x10272464},
        {1800, 10, std::int32_t(0x91a2b3c5u), 0x102724c4},
    };
    if (compare(c, phase, 3) > 0) {
        if (compare(c, phase, 5) >= 0) return invalid_phase();
    } else if (phase < 3) {
        if (compare(c, phase, 1) < 0 && compare(c, phase, 0) < 0) return invalid_phase();
    }
    const auto [bound, shift, magic, back] = ranges[unsigned(phase)];
    first_draw(bound, shift, magic, back);
    scale();
    return true;
}

// This side returns immediately. The other side continues into the spatial checks.
bool PlayerObstruction::finish_protected_side() {
    c.set(3, actor.match());
    c.set(0, actor.side());
    c.set(4, c.u8(c.r(3) + 0x7921));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, std::uint32_t(std::int8_t(c.r(4))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) != 0) return false;

    const auto threshold = std::int32_t(std::int16_t(c.r(30))) + 100;
    c.set(3, std::uint32_t(threshold - 100));
    c.set(0, std::uint32_t(threshold));
    c.s.wr<1>(reg::xer_ca, threshold >= 0 && threshold < 100);
    if (compare(c, threshold) == 0) c.set(20, 0);
    else {
        refill(0x10272600);
        // The wide original path is outside the range of a signed halfword plus 100.
        c.set(4, std::uint32_t(threshold - 100));
        c.set(3, std::uint32_t(-65535));
        c.set(0, std::uint32_t(threshold));
        compare(c, threshold, -65535);
        c.set(6, c.u32(c.r(29)));
        c.set(3, std::uint32_t(threshold - 100));
        c.set(5, c.u32(c.r(28)));
        c.set(6, c.u16(c.r(6)));
        c.set(4, std::uint32_t(threshold));
        c.s.wr<1>(reg::xer_ca, threshold >= 0 && threshold < 100);
        compare(c, threshold);
        c.set(3, c.r(6) + c.r(5));
        const auto remainder = narrow_remainder(c.r(3), threshold);
        c.set(20, remainder);
        c.set(0, threshold > 0 ? remainder : 0u - remainder);
        c.set(20, c.r(0));
    }
    if (compare(c, std::int32_t(c.r(20))) != 0) {
        reject();
        return true;
    }

    c.set(3, c.u32(actor.match() + 0x79f4));
    c.set({{4, 0x1f5e}, {6, c.u8(actor.address + 0x2f)}, {7, actor.side()},
           {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}, {5, actor.match()}});
    c.call(0x10201180, 0x102727a0);
    c.set(4, actor.match());
    c.set(3, c.u32(c.r(4) + 0x3108));
    c.set(0, c.r(3) - 300);
    c.put32(c.r(4) + 0x3108, c.r(0));
    c.set(3, actor.match());
    c.set(20, c.u8(c.r(3) + 0x7920));
    const auto clock = std::int32_t(std::int8_t(c.r(20)));
    c.set(0, std::uint32_t(clock));
    if (compare(c, clock) == 0) c.set(18, 0);
    else {
        refill(0x102727e8);
        // The clock is an extended byte, so the second wide path is unreachable too.
        c.set(3, std::uint32_t(-65535));
        c.set(0, std::uint32_t(clock));
        compare(c, clock, -65535);
        c.set(5, c.u32(c.r(29)));
        c.set(3, std::uint32_t(clock));
        c.set(4, c.u32(c.r(28)));
        compare(c, clock);
        c.set(0, c.u16(c.r(5)));
        c.set(4, c.r(0) + c.r(4));
        const auto remainder = narrow_remainder(c.r(4), clock);
        c.set(18, remainder);
        c.set(0, clock > 0 ? remainder : 0u - remainder);
        c.set(18, c.r(0));
    }
    c.set(0, c.r(18) + 1);
    c.set({{3, actor.match()}, {4, std::uint32_t(std::uint8_t(actor.column()))}, {10, actor.address},
           {5, std::uint32_t(std::uint8_t(actor.row()))}, {6, std::uint32_t(std::int8_t(c.r(0)))},
           {7, actor.side()}, {8, 0}, {9, 1}});
    c.call(0x101f1100, 0x10272968);
    c.set(3, 1);
    return true;
}

bool PlayerObstruction::in_goal_region(bool away, unsigned scratch, const Player& candidate) {
    const auto column = [&] { c.set(scratch, std::uint32_t(candidate.column())); return std::int32_t(c.r(scratch)); };
    const auto row = [&] { c.set(scratch, std::uint32_t(candidate.row())); return std::int32_t(c.r(scratch)); };
    return compare(c, column(), 2) >= 0 && compare(c, column(), 6) <= 0 &&
           compare(c, row(), away ? 10 : 0) >= 0 && compare(c, row(), away ? 11 : 1) <= 0;
}

bool PlayerObstruction::role_is_keeper(std::uint32_t back) {
    c.set(3, actor.match());
    c.set(4, actor.address);
    c.call(0x101e3d50, back);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0)), 1) == 0;
}

void PlayerObstruction::select_attribute() {
    c.set(0, actor.side());
    const bool away = compare(c, std::int32_t(c.r(0)), 1) == 0;
    c.set(3, 0);
    const bool inside = in_goal_region(away, 0, actor);
    c.set(3, unsigned(inside));
    c.set(0, c.r(3));
    compare(c, std::int32_t(c.r(0)));
    if (!inside || !role_is_keeper(0x10272a30)) {
        c.set(3, c.u32(actor.address + 0x6c));
        c.set(0, actor.side());
        c.set(3, std::uint32_t(c.i8(c.r(3) + 0x68)));
        const bool other_away = compare(c, std::int8_t(c.r(0)), 1) == 0;
        c.set(0, c.r(3) * 200);
        c.set(4, 0);
        const bool other_inside = in_goal_region(other_away, 3, actor);
        c.set(4, unsigned(other_inside));
        c.set(3, c.r(4) * 500);
        const auto subtotal = c.r(0) + c.r(3);
        const auto bound = std::int32_t(subtotal + 100);
        c.s.wr<1>(reg::xer_ca, subtotal > UINT32_MAX - 100);
        c.set(19, std::uint32_t(bound));
        if (compare(c, bound) == 0) c.set(23, 0);
        else {
            refill(0x10272b30);
            // 200 times an extended byte, plus at most 600, stays narrow.
            c.set(3, std::uint32_t(-65535));
            compare(c, bound, -65535);
            c.set(3, c.u32(c.r(29)));
            compare(c, bound);
            c.set(0, c.u32(c.r(28)));
            c.set(3, c.u16(c.r(3)) + c.r(0));
            const auto remainder = narrow_remainder(c.r(3), bound);
            c.set(23, remainder);
            c.set(0, bound > 0 ? remainder : 0u - remainder);
            c.set(23, c.r(0));
        }
        if (compare(c, std::int32_t(c.r(23))) == 0) {
            load_attribute(37);
            c.set(0, c.r(18) * 2);
            c.set(18, std::uint32_t(std::int16_t(c.r(0))));
            return;
        }
    }

    c.set(3, actor.match());
    c.set(0, c.u8(c.r(3) + 0x791f));
    if (compare(c, std::int32_t(c.r(0)), 4) == 0) {
        load_attribute(role_is_keeper(0x10272cb0) ? 1 : 21);
    } else if (!role_is_keeper(0x10272cf8)) load_attribute(36);
    else {
        c.set(3, actor.match());
        c.set(0, c.u8(c.r(3) + 0x791f));
        bool use_keeper = false;
        if (compare(c, std::int32_t(c.r(0)), 3) == 0) {
            refill(0x10272d4c);
            const auto value = draw_word();
            c.set(3, value >> 31);
            c.set(0, std::uint32_t(std::int32_t(value) % 4));
            use_keeper = compare(c, std::int32_t(c.r(0))) != 0;
        }
        load_attribute(use_keeper ? 1 : 41);
    }
}

void PlayerObstruction::load_attribute(unsigned index) {
    c.set(23, index);
    c.set(0, index * 2);
    c.set(3, actor.address + c.r(0));
    c.set(18, std::uint32_t(c.i16(c.r(3) + 0x11e)));
}

std::uint32_t PlayerObstruction::draw_word() {
    c.set(3, c.u32(c.r(29)));
    c.set(0, c.u32(c.r(28)));
    c.set(3, c.u16(c.r(3)) + c.r(0));
    return c.r(3);
}

// A nonnegative threshold proceeds to the contested-action selection.
bool PlayerObstruction::finish_insufficient_threshold() {
    c.set(0, std::uint32_t(std::int8_t(c.r(20))));
    if (compare(c, std::int32_t(c.r(0))) != 0) {
        c.set(3, std::uint32_t(std::int16_t(c.r(18))) ^ 0x80000000);
        c.set(0, 0x43300000);
        c.put32(frame.at(0x70), c.r(0));
        c.set_f(1, c.f64(c.r(27) + 0x80));
        c.put32(frame.at(0x74), c.r(3));
        c.set_f(2, c.f64(c.r(27) + 0x1a0));
        c.set_f(0, c.f64(frame.at(0x70)));
        c.set_f(0, fp.fsub(c.f(0), c.f(1)));
        c.set_f(0, fp.fmul(c.f(2), c.f(0)));
        c.put64(frame.at(0x78), fp.fctiwz(0, c.f(0)));
        c.set(18, c.u32(frame.at(0x7c)));
    }
    const auto bound = std::int32_t(std::int16_t(c.r(18)));
    c.set(0, std::uint32_t(bound));
    if (compare(c, bound) == 0) c.set(19, 0);
    else {
        refill(0x10272e14);
        // The explicit sign extension of r18 bounds both wide checks.
        c.set(3, std::uint32_t(-65535));
        c.set(0, std::uint32_t(bound));
        compare(c, bound, -65535);
        c.set(5, c.u32(c.r(29)));
        c.set(3, std::uint32_t(bound));
        c.set(4, c.u32(c.r(28)));
        compare(c, bound);
        c.set(0, c.u16(c.r(5)));
        c.set(4, c.r(0) + c.r(4));
        const auto remainder = narrow_remainder(c.r(4), bound);
        c.set(19, remainder);
        c.set(0, bound > 0 ? remainder : 0u - remainder);
        c.set(19, c.r(0));
    }
    const auto context = std::int32_t(std::int16_t(c.r(21)));
    const auto high = std::int32_t((std::int64_t(context) * 0x55555556) >> 32);
    c.set(3, std::uint32_t(high));
    c.set(0, c.r(19) - std::uint32_t(high + (high < 0)) - c.r(30));
    c.set(18, std::uint32_t(std::int16_t(c.r(0))));
    if (compare(c, std::int32_t(c.r(18)), -100) < 0) {
        refill(0x10272fb8);
        c.set(4, c.u32(c.r(29)));
        c.set(3, 0);
        c.set(0, c.u32(c.r(28)));
        c.set(5, c.u16(c.r(4)));
        c.set(4, c.u8(actor.address + 0x2c));
        const auto value = c.r(5) + c.r(0);
        c.set(5, value >> 31);
        c.set(0, c.r(4) + std::uint32_t(std::int32_t(value) % 2));
        c.put8(actor.address + 0x2c, c.r(0));
        return true;
    }
    c.set(0, c.r(18));
    if (compare(c, std::int32_t(c.r(0))) < 0) {
        reject();
        return true;
    }
    return false;
}

void PlayerObstruction::run() {
    if (!prepare() || finish_protected_side()) return;
    select_attribute();
    if (finish_insufficient_threshold()) return;
    if (set_up_contest()) {
        finish_restart();
        return;
    }
    c.set(0, std::uint32_t(std::int16_t(c.r(18))));
    if (compare(c, std::int32_t(c.r(0)), 400) < 0) finish_light_disruption();
    else finish_strong_disruption();
}

std::uint32_t PlayerObstruction::narrow_remainder(std::uint32_t word, std::int32_t bound) {
    return word - std::uint32_t(divide_signed<4>(word, std::uint32_t(bound))) * std::uint32_t(bound);
}

bool PlayerObstruction::reject() { c.set(3, 0); return false; }

bool PlayerObstruction::invalid_phase() {
    c.set({{3, c.r(23) + 0x8f}, {6, frame.at(0x50)}, {7, frame.at(0x48)}, {4, 0}, {5, 0}});
    c.call(0x10739890, 0x10272520);
    c.call(0x10768810, 0x10272524);
    c.set({{5, c.r(3)}, {3, c.r(30)}, {4, c.r(23) + 0x9c},
           {6, frame.at(0x50)}, {7, frame.at(0x48)}, {8, 0x4749}});
    c.call(0x1077b720, 0x10272540);
    c.set({{4, c.r(30)}, {3, c.r(23) + 0xa9}, {5, 0}});
    c.call(0x10769b60, 0x10272550);
    c.set(4, c.toc(-0x7bac));
    c.set(0, 0);
    c.set(3, 0);
    c.put8(c.r(4), 0);
    return false;
}

void PlayerObstruction::refill(std::uint32_t back) {
    c.set(3, c.u32(c.r(29)));
    c.set(0, c.u32(c.r(24)));
    c.set(3, c.r(3) + 2);
    const bool wrapped = compare_logical(c, c.r(3), c.r(0)) > 0;
    c.put32(c.r(29), c.r(3));
    if (wrapped) {
        c.put32(c.r(29), c.r(25));
        c.call(0x1077b770, back);
        c.set(0, c.r(3) & 0xffff);
        c.put32(c.r(28), c.r(0));
    }
}

void PlayerObstruction::first_draw(unsigned bound, unsigned shift, std::int32_t magic, std::uint32_t back) {
    refill(back);
    c.set(4, c.u32(c.r(29)));
    c.set(0, c.u32(c.r(28)));
    const auto value = c.u16(c.r(4)) + c.r(0);
    c.set(4, value);
    auto high = std::uint32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * magic) >> 32);
    if (bound == 1800) high += value;
    if (shift) c.s.wr<1>(reg::xer_ca, std::int32_t(high) < 0 && (high & ((1u << shift) - 1)));
    const auto quotient = std::int32_t(high) >> shift;
    c.set(3, unsigned(quotient < 0));
    c.set(0, value - (std::uint32_t(quotient) + c.r(3)) * bound);
    c.set(30, std::uint32_t(std::int16_t(c.r(0))));
}

void PlayerObstruction::scale() {
    c.set_f(1, c.f64(c.r(27) + 0x10));
    c.set_f(0, c.f64(c.r(27) + 0x88));
    c.set_f(1, fp.fadd(c.f(1), c.f(31)));
    fp.fcmp(0, c.f(1), c.f(0));
    if ((c.cr(0) & 4) == 0) return;
    c.set(3, std::uint32_t(std::int16_t(c.r(30))) ^ 0x80000000);
    c.set(0, 0x43300000);
    c.set_f(0, c.f64(c.r(27) + 0x10));
    c.put32(frame.at(0x84), c.r(3));
    c.set_f(2, c.f64(c.r(27) + 0x80));
    c.set_f(0, fp.fadd(c.f(0), c.f(31)));
    c.put32(frame.at(0x80), c.r(0));
    c.set_f(1, c.f64(frame.at(0x80)));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0)));
    c.put64(frame.at(0x78), fp.fctiwz(0, c.f(0)));
    c.set(30, c.u32(frame.at(0x7c)));
}
}
