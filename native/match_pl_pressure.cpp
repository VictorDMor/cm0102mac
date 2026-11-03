#include "match_pl_pressure.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void CellPressure::refill(std::uint32_t back) {
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(25)));
    c.set(3, c.r(3) + 2);
    const bool wrapped = compare_logical(c, c.r(3), c.r(0)) > 0;
    c.put32(c.r(30), c.r(3));
    if (wrapped) {
        c.put32(c.r(30), c.r(26));
        c.call(0x1077b770, back);
        c.set(0, c.r(3) & 0xffff);
        c.put32(c.r(28), c.r(0));
    }
}

void CellPressure::refill_after_eligibility_read() {
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(25)));
    c.set(3, c.r(3) + 2);
    c.set(19, c.u8(actor.address + 0x110));
    const bool wrapped = compare_logical(c, c.r(3), c.r(0)) > 0;
    c.put32(c.r(30), c.r(3));
    if (wrapped) {
        c.put32(c.r(30), c.r(26));
        c.call(0x1077b770, 0x10239fac);
        c.set(0, c.r(3) & 0xffff);
        c.put32(c.r(28), c.r(0));
    }
}

bool CellPressure::reject() { c.set(3, 0); return false; }

bool CellPressure::eligible() {
    c.set(3, actor.match() + 0x10000);
    c.set(0, c.u32(c.r(3) - 0x200c));
    if (compare_logical(c, c.r(0), actor.address) == 0) return reject();
    c.set(3, actor.match());
    c.set(0, actor.side());
    c.set(4, c.u8(c.r(3) + 0x7924));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, std::uint32_t(std::int8_t(c.r(4))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0) return true;

    refill_after_eligibility_read();
    c.set(5, c.u32(c.r(30)));
    c.set(0, std::uint32_t(std::int8_t(c.r(19))));
    c.set(4, c.u32(c.r(28)));
    c.set(5, c.u16(c.r(5)));
    c.set(0, c.r(0) * 2);
    c.set(5, c.r(5) + c.r(4));
    const auto high = std::int32_t((std::int64_t(std::int32_t(c.r(5))) * 0x2aaaaaab) >> 32);
    c.set(4, std::uint32_t(high));
    c.set(3, c.r(5) - 6 * std::uint32_t(high + (high < 0)));
    if (compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(3))) >= 0) return true;
    c.set(0, actor.side());
    c.set(3, actor.match());
    c.set(5, c.u8(actor.address + 0x106));
    c.set(0, std::int8_t(c.r(0)) == 0);
    c.set(4, c.r(0));
    c.call(0x101ee9f0, 0x1023a010);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) == 0) return reject();
    return true;
}

bool CellPressure::keeper_in_region() {
    c.set({{3, actor.match()}, {4, actor.address}});
    c.call(0x101e3d50, 0x1023a02c);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0)), 1) != 0) return false;
    c.set(0, actor.side());
    const bool away = compare(c, std::int32_t(c.r(0)), 1) == 0;
    c.set(0, unsigned(inside_goal_region(away, 0, 3)));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

bool CellPressure::inside_goal_region(bool away, unsigned coordinate, unsigned result) {
    return inside_goal_region(actor, away, coordinate, result);
}

bool CellPressure::inside_goal_region(Player subject, bool away, unsigned coordinate, unsigned result) {
    c.set(result, 0);
    const auto column = [&] {
        c.set(coordinate, std::uint32_t(subject.column()));
        return std::int32_t(c.r(coordinate));
    };
    const auto row = [&] {
        c.set(coordinate, std::uint32_t(subject.row()));
        return std::int32_t(c.r(coordinate));
    };
    const bool inside = compare(c, column(), 2) >= 0 && compare(c, column(), 6) <= 0 &&
        compare(c, row(), away ? 10 : 0) >= 0 && compare(c, row(), away ? 11 : 1) <= 0;
    c.set(result, unsigned(inside));
    return inside;
}

std::int32_t CellPressure::shifted_product(std::uint32_t value, std::int32_t magic, unsigned shift) {
    auto high = std::uint32_t((std::int64_t(std::int32_t(value)) * magic) >> 32);
    if (magic < 0) high += value;
    c.s.wr<1>(reg::xer_ca, std::int32_t(high) < 0 && (high & ((1u << shift) - 1)));
    return std::int32_t(high) >> shift;
}

void CellPressure::keeper_wide_draw(std::int32_t bound) {
    c.set(3, std::uint32_t(bound) ^ 0x80000000);
    c.set(0, 0x43300000);
    c.put32(frame.at(0x84), c.r(3));
    c.set(7, c.u32(c.r(30)));
    c.set(4, 0x43300000);
    c.put32(frame.at(0x80), c.r(0));
    c.set_f(2, c.f64(c.r(27) + 0x80));
    c.set_f(1, c.f64(frame.at(0x80)));
    c.set_f(0, c.f64(c.r(27)));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set(8, c.u16(c.r(7)));
    c.set(7, c.u32(c.r(28)));
    c.set(0, std::uint32_t(bound) * 10);
    c.put32(frame.at(0x78), c.r(4));
    c.set_f(2, fp.fdiv(c.f(1), c.f(0)));
    c.set(7, c.r(8) + c.r(7));
    const auto scaled = shifted_product(c.r(7), std::int32_t(0x80008001u), 15);
    const auto whole = scaled + (scaled < 0);
    c.set_f(1, c.f64(c.r(27) + 0x80));
    c.set({{6, unsigned(std::int32_t(c.r(7)) < 0)}, {5, 65535}});
    c.set(19, c.r(7) - std::uint32_t(whole) * c.r(5));
    c.set(4, c.r(19) ^ 0x80000000);
    c.put32(frame.at(0x7c), c.r(4));
    c.set_f(0, c.f64(frame.at(0x78)));
    c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    const auto correction = shifted_product(c.r(0), std::int32_t(0x80008001u), 15);
    c.set(0, std::uint32_t(correction));
    c.set(3, std::uint32_t(correction + (correction < 0)));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    c.put64(frame.at(0x70), fp.fctiwz(0, c.f(0)));
    c.set(19, c.u32(frame.at(0x74)));
    c.call(0x10226520, 0x1023a208);
    const auto numerator = c.r(3) + 5;
    c.set(0, std::uint32_t(bound) - 1);
    const auto adjustment = shifted_product(numerator, 0x66666667, 2);
    c.set(4, unsigned(adjustment < 0));
    c.set(3, std::uint32_t(adjustment + (adjustment < 0)));
    c.set(19, c.r(19) + c.r(3));
    if (compare(c, std::int32_t(c.r(19)), std::int32_t(c.r(0))) > 0) c.set(19, c.r(0));
}

void CellPressure::keeper_threshold() {
    c.set_f(2, c.f32(c.r(21) + 0x80));
    c.set_f(1, c.f32(actor.address + 0xa8));
    c.set_f(0, c.f32(c.r(21) + 0xd8));
    c.set_f(0, fp.multiply_add(0x1023a0f0, c.f(2), c.f(1), c.f(0)));
    c.put64(frame.at(0x80), fp.fctiwz(0, c.f(0)));
    c.set(23, c.u32(frame.at(0x84)));
    const auto bound = std::int32_t(c.r(23));
    if (compare(c, bound) == 0) c.set(19, 0);
    else {
        refill(0x1023a130);
        c.set(3, 65535);
        bool wide = compare(c, bound, 65535) > 0;
        if (!wide) {
            c.set(3, std::uint32_t(-65535));
            wide = compare(c, bound, -65535) < 0;
        }
        if (wide) keeper_wide_draw(bound);
        else {
            c.set(3, c.u32(c.r(30)));
            compare(c, bound);
            c.set(0, c.u32(c.r(28)));
            c.set(3, c.u16(c.r(3)) + c.r(0));
            const auto remainder = std::uint32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
            c.set(19, remainder);
            c.set(0, bound > 0 ? remainder : 0u - remainder);
            c.set(19, c.r(0));
        }
    }
    c.set(29, std::uint32_t(std::int16_t(c.r(19))));
}

void CellPressure::ordinary_draw(std::uint32_t back, unsigned divisor, std::uint32_t magic, unsigned shift) {
    refill(back);
    c.set(4, c.u32(c.r(30)));
    c.set(3, magic);
    c.set(0, c.u32(c.r(28)));
    c.set(4, c.u16(c.r(4)) + c.r(0));
    const auto scaled = shifted_product(c.r(4), std::int32_t(c.r(3)), shift);
    c.set(3, unsigned(scaled < 0));
    c.set(0, c.r(4) - divisor * std::uint32_t(scaled + (scaled < 0)));
    c.set(29, std::uint32_t(std::int16_t(c.r(0))));
}

void CellPressure::expansive_draw() {
    refill(0x1023a4b0);
    c.set(3, c.u32(c.r(30)));
    c.set(0, 0x43300000);
    c.set(7, c.u16(c.r(3)));
    c.set(6, c.u32(c.r(28)));
    c.set(4, 65535);
    c.put32(frame.at(0x70), c.r(0));
    c.set(3, 76);
    c.set(6, c.r(7) + c.r(6));
    c.set_f(1, c.f64(c.r(27) + 0x80));
    c.set_f(2, c.f64(c.r(27) + 0x320));
    const auto scaled = shifted_product(c.r(6), std::int32_t(0x80008001u), 15);
    c.set(5, unsigned(scaled < 0));
    c.set(0, 65535u * std::uint32_t(scaled + (scaled < 0)));
    c.set(19, c.r(6) - c.r(0));
    c.set(0, c.r(19) ^ 0x80000000);
    c.put32(frame.at(0x74), c.r(0));
    c.set_f(0, c.f64(frame.at(0x70)));
    c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    c.put64(frame.at(0x78), fp.fctiwz(0, c.f(0)));
    c.set(19, c.u32(frame.at(0x7c)));
    c.call(0x10226520, 0x1023a52c);
    const auto numerator = c.r(3) + 5;
    c.set(4, 0x66666667);
    c.set(3, 0x7a11f);
    const auto adjustment = shifted_product(numerator, 0x66666667, 2);
    c.set(4, unsigned(adjustment < 0));
    c.set(0, std::uint32_t(adjustment + (adjustment < 0)));
    c.set(19, c.r(19) + c.r(0));
    if (compare(c, std::int32_t(c.r(19)), std::int32_t(c.r(3))) > 0) c.set(19, 0x7a11f);
    c.set(0, c.r(19) + 200);
    c.set(24, 4);
    c.set(29, std::uint32_t(std::int16_t(c.r(0))));
}

bool CellPressure::invalid_mode() {
    c.set({{3, c.r(23) + 0x8f}, {6, frame.at(0x50)}, {7, frame.at(0x48)}, {4, 0}, {5, 0}});
    c.call(0x10739890, 0x1023a58c);
    c.call(0x10768810, 0x1023a590);
    c.set({{5, c.r(3)}, {3, c.r(29)}, {4, c.r(23) + 0x9c}, {6, frame.at(0x50)},
           {7, frame.at(0x48)}, {8, 0x17d6}});
    c.call(0x1077b720, 0x1023a5ac);
    c.set({{4, c.r(29)}, {3, c.r(23) + 0xa9}, {5, 0}});
    c.call(0x10769b60, 0x1023a5bc);
    c.set({{4, c.toc(-0x7bac)}, {0, 0}, {3, 0}});
    c.put8(c.r(4), 0);
    return false;
}

bool CellPressure::ordinary_threshold() {
    const auto mode = std::int32_t(std::int8_t(c.r(24)));
    c.set(0, std::uint32_t(mode));
    if (compare(c, mode, 3) == 0) ordinary_draw(0x1023a3ec, 1000, 0x10624dd3, 6);
    else if (mode < 3) {
        if (compare(c, mode, 1) == 0) ordinary_draw(0x1023a32c, 600, 0x1b4e81b5, 6);
        else if (mode > 1) ordinary_draw(0x1023a38c, 700, 0x5d9f7391, 8);
        else if (compare(c, mode) >= 0) ordinary_draw(0x1023a2cc, 500, 0x10624dd3, 5);
        else return invalid_mode();
    } else {
        if (compare(c, mode, 5) == 0) expansive_draw();
        else if (mode < 5) ordinary_draw(0x1023a44c, 1800, 0x91a2b3c5, 10);
        else return invalid_mode();
    }
    return true;
}

void CellPressure::rescale_threshold() {
    c.set_f(1, c.f64(c.r(27) + 0x10));
    c.set_f(0, c.f64(c.r(27) + 0x88));
    c.set_f(1, fp.fadd(c.f(1), c.f(31)));
    fp.fcmp(0, c.f(1), c.f(0));
    if (!(c.cr(0) & 4)) return;

    c.set(3, actor.match());
    c.set(0, 0x43300000);
    c.set_f(0, c.f64(c.r(27) + 0x10));
    c.set(4, std::uint32_t(std::int16_t(c.r(29))));
    c.set(3, c.u8(c.r(3) + 0x7920));
    c.put32(frame.at(0x70), c.r(0));
    c.set_f(0, fp.fadd(c.f(0), c.f(31)));
    c.set(3, std::uint32_t(std::int8_t(c.r(3))));
    c.set_f(2, c.f64(c.r(27) + 0x80));
    c.set(0, ((c.r(3) + 1) * c.r(4)) ^ 0x80000000);
    c.put32(frame.at(0x74), c.r(0));
    c.set_f(1, c.f64(frame.at(0x70)));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0)));
    c.put64(frame.at(0x78), fp.fctiwz(0, c.f(0)));
    c.set(29, c.u32(frame.at(0x7c)));
}

bool CellPressure::side_already_in_possession() {
    c.set(3, actor.match());
    c.set(0, actor.side());
    c.set(4, c.u8(c.r(3) + 0x7921));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, std::uint32_t(std::int8_t(c.r(4))));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0;
}

bool CellPressure::restart_chance() {
    c.set(3, std::uint32_t(std::int16_t(c.r(29))));
    const auto bound = std::int32_t(c.r(3)) + 100;
    c.set(0, std::uint32_t(bound));
    c.s.wr<1>(reg::xer_ca, c.r(3) > UINT32_MAX - 100);
    if (compare(c, bound) == 0) c.set(19, 0);
    else {
        refill(0x1023a6a0);
        c.set({{4, std::uint32_t(std::int16_t(c.r(29)))}, {0, std::uint32_t(bound)}, {3, 65535}});
        compare(c, bound, 65535);
        c.set(3, std::uint32_t(-65535));
        compare(c, bound, -65535);
        // The source sign-extends r29 before adding 100, so the wide sampler cannot run.
        c.set(6, c.u32(c.r(30)));
        c.set(3, c.r(4));
        c.set(5, c.u32(c.r(28)));
        c.set(6, c.u16(c.r(6)));
        c.set(4, std::uint32_t(bound));
        c.s.wr<1>(reg::xer_ca, c.r(3) > UINT32_MAX - 100);
        compare(c, bound);
        c.set(3, c.r(6) + c.r(5));
        const auto remainder = std::uint32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
        c.set(0, bound > 0 ? remainder : 0u - remainder);
        c.set(19, c.r(0));
    }
    return compare(c, std::int32_t(c.r(19))) == 0;
}

void CellPressure::restart_with_pressure() {
    announce_actor(0x1f5e, 0x1023a840);
    c.set(4, actor.match());
    c.set(3, c.u32(c.r(4) + 0x3108));
    c.set(0, c.r(3) - 300);
    c.put32(c.r(4) + 0x3108, c.r(0));

    c.set(3, actor.match());
    c.set(19, c.u8(c.r(3) + 0x7920));
    const auto bound = std::int32_t(std::int8_t(c.r(19)));
    c.set(0, std::uint32_t(bound));
    if (compare(c, bound) == 0) c.set(18, 0);
    else {
        refill(0x1023a888);
        c.set({{3, 65535}, {0, std::uint32_t(bound)}});
        compare(c, bound, 65535);
        c.set(3, std::uint32_t(-65535));
        compare(c, bound, -65535);
        // The original compares a signed clock byte, which cannot reach its wide sampler.
        c.set(5, c.u32(c.r(30)));
        c.set(3, std::uint32_t(bound));
        c.set(4, c.u32(c.r(28)));
        compare(c, bound);
        c.set(0, c.u16(c.r(5)));
        c.set(4, c.r(0) + c.r(4));
        const auto remainder = std::uint32_t(std::int64_t(std::int32_t(c.r(4))) % bound);
        c.set(0, bound > 0 ? remainder : 0u - remainder);
        c.set(18, c.r(0));
    }
    c.set({{0, c.r(18) + 1}, {3, actor.match()}, {4, c.u8(actor.address + 0x105)}, {10, actor.address},
           {5, c.u8(actor.address + 0x106)}, {6, std::uint32_t(std::int8_t(c.r(18) + 1))},
           {7, actor.side()}, {8, 0}, {9, 1}});
    c.call(0x101f1100, 0x1023aa08);
    c.set(3, 1);
}

bool CellPressure::role_is_keeper(std::uint32_t back) {
    c.set({{3, actor.match()}, {4, actor.address}});
    c.call(0x101e3d50, back);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0)), 1) == 0;
}

bool CellPressure::pressure_attribute_draw() {
    c.set(3, c.u32(actor.address + 0x6c));
    c.set(0, actor.side());
    c.set(3, std::uint32_t(c.i8(c.r(3) + 0x68)));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    const bool away = compare(c, std::int32_t(c.r(0)), 1) == 0;
    c.set(0, c.r(3) * 200);
    c.set(3, unsigned(inside_goal_region(away, 3, 4)) * 500);
    const auto base = c.r(0) + c.r(3);
    const auto bound = std::int32_t(base + 100);
    c.set(23, std::uint32_t(bound));
    c.s.wr<1>(reg::xer_ca, base > UINT32_MAX - 100);
    if (compare(c, bound) == 0) c.set(19, 0);
    else {
        refill(0x1023abd0);
        c.set(3, 65535);
        compare(c, bound, 65535);
        c.set(3, std::uint32_t(-65535));
        compare(c, bound, -65535);
        // The signed staff byte and binary field position keep |bound| below 65535.
        c.set(3, c.u32(c.r(30)));
        compare(c, bound);
        c.set(0, c.u32(c.r(28)));
        c.set(3, c.u16(c.r(3)) + c.r(0));
        const auto remainder = std::uint32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
        c.set(0, bound > 0 ? remainder : 0u - remainder);
        c.set(19, c.r(0));
    }
    return compare(c, std::int32_t(c.r(19))) == 0;
}

void CellPressure::load_pressure_attribute(unsigned index) {
    c.set({{23, index}, {0, index * 2}, {3, actor.address + index * 2}});
    c.set(18, std::uint32_t(c.i16(c.r(3) + 0x11e)));
}

void CellPressure::select_pressure_attribute() {
    c.set(0, actor.side());
    const bool away = compare(c, std::int32_t(c.r(0)), 1) == 0;
    c.set(0, unsigned(inside_goal_region(away, 0, 3)));
    const bool local_keeper = compare(c, std::int32_t(c.r(0))) != 0 && role_is_keeper(0x1023aad0);
    if (!local_keeper && pressure_attribute_draw()) {
        load_pressure_attribute(37);
        return;
    }
    const auto mode = std::int32_t(std::int8_t(c.r(24)));
    c.set(0, std::uint32_t(mode));
    if (compare(c, mode, 4) == 0) {
        load_pressure_attribute(role_is_keeper(0x1023ad44) ? 1 : 21);
        return;
    }
    if (!role_is_keeper(0x1023ad8c)) {
        load_pressure_attribute(36);
        return;
    }
    c.set(0, std::uint32_t(mode));
    if (compare(c, mode, 3) == 0) {
        refill(0x1023addc);
        c.set(3, c.u32(c.r(30)));
        c.set(0, c.u32(c.r(28)));
        const auto draw = c.u16(c.r(3)) + c.r(0);
        c.set(3, draw >> 31);
        c.set(0, std::uint32_t(std::int32_t(draw) % 4));
        if (compare(c, std::int32_t(c.r(0))) != 0) {
            load_pressure_attribute(1);
            return;
        }
    }
    load_pressure_attribute(41);
}

void CellPressure::sample_pressure_attribute() {
    const auto bound = std::int32_t(std::int16_t(c.r(18)));
    c.set(0, std::uint32_t(bound));
    if (compare(c, bound) == 0) {
        c.set(19, 0);
        return;
    }

    refill(0x1023ae68);
    c.set({{3, 65535}, {0, std::uint32_t(bound)}});
    compare(c, bound, 65535);
    c.set(3, std::uint32_t(-65535));
    compare(c, bound, -65535);
    // The original sign-extends the selected halfword in all three checks.
    c.set(5, c.u32(c.r(30)));
    c.set(3, std::uint32_t(bound));
    c.set(4, c.u32(c.r(28)));
    compare(c, bound);
    c.set(0, c.u16(c.r(5)));
    c.set(4, c.r(0) + c.r(4));
    const auto remainder = std::uint32_t(std::int64_t(std::int32_t(c.r(4))) % bound);
    c.set(0, bound > 0 ? remainder : 0u - remainder);
    c.set(19, c.r(0));
}

void CellPressure::adjust_contact_count(std::uint32_t back, unsigned verdict) {
    refill(back);
    c.set(4, c.u32(c.r(30)));
    c.set(3, verdict);
    c.set(0, c.u32(c.r(28)));
    c.set(5, c.u16(c.r(4)));
    c.set(4, c.u8(actor.address + 0x2c));
    c.set(0, c.r(5) + c.r(0));
    c.set(5, c.r(0) >> 31);
    c.set(0, c.r(4) + std::uint32_t(std::int32_t(c.r(0)) % 2));
    c.put8(actor.address + 0x2c, c.r(0));
}

bool CellPressure::pressure_exceeds_threshold() {
    c.set(4, std::uint32_t(std::int16_t(c.r(20))));
    c.set(0, c.r(19) - c.r(29));
    c.set(3, (c.r(4) >> 31) + c.r(4));
    c.s.wr<1>(reg::xer_ca, std::int32_t(c.r(3)) < 0 && (c.r(3) & 1));
    c.set(3, std::uint32_t(std::int32_t(c.r(3)) >> 1));
    c.set(0, c.r(0) - c.r(3));
    const auto margin = std::int32_t(std::int16_t(c.r(0)));
    c.set(18, std::uint32_t(margin));
    if (compare(c, margin, -100) < 0) {
        adjust_contact_count(0x1023b004, 0);
        return false;
    }
    c.set(0, std::uint32_t(margin));
    if (compare(c, margin, 150) < 0) return reject();
    return true;
}

CellPressure::CellPressure(Cpu& cpu)
    : c(cpu), frame(c, 0x10239f10, 0xd0, {.first_gpr = 18, .gpr_offset = -0x48, .first_fpr = 31}),
      fp(c), actor{c, c.r(3)} {
    c.set(0, c.lr());
    c.set_f(31, c.f(1));
    c.set({{31, actor.address}, {29, c.toc(-0x7ba8)}, {20, c.r(4)}, {27, c.toc(-0x51e8)},
           {28, c.toc(-0x7b90)}, {24, c.r(5)}, {26, c.toc(-0x7b8c)}, {22, c.r(7)},
           {25, c.toc(-0x7b88)}, {30, c.toc(-0x7b84)}, {23, c.toc(-0x51e4)}, {21, c.toc(-0x51e0)}});
}

bool CellPressure::prepare() {
    if (!eligible()) return false;
    if (keeper_in_region()) keeper_threshold();
    else if (!ordinary_threshold()) return false;
    c.set_f(0, c.f64(c.r(27) + 0x90));
    fp.fcmp(0, c.f(31), c.f(0));
    if (c.cr(0) & 8) {
        c.set(0, std::uint32_t(std::int8_t(c.r(24))));
        if (compare(c, std::int32_t(c.r(0)), 4) >= 0) return reject();
    }
    return true;
}

bool CellPressure::prepare_attribute() {
    rescale_threshold();
    if (side_already_in_possession()) {
        if (restart_chance()) restart_with_pressure();
        else reject();
        return false;
    }
    select_pressure_attribute();
    return true;
}

bool CellPressure::evaluate_pressure() {
    sample_pressure_attribute();
    if (!pressure_exceeds_threshold()) return false;
    c.set({{3, actor.match()}, {4, actor.address}, {5, c.r(23)}, {6, frame.at(0x42)}, {7, frame.at(0x41)}});
    c.call(0x101f50c0, 0x1023b070);
    return true;
}

void CellPressure::run() {
    if (!prepare() || !prepare_attribute() || !evaluate_pressure()) return;
    if (publish_sanction(route_sanction())) {
        announce_published_sanction();
        finish_published_sanction();
    } else finish_unsettled_contact();
}
}
