// 0x1066b600 and 0x1066bc50: a club's rating changes after a story, and the press item that reports it.
#include "media_values_detail.hpp"

namespace cm::aot::media {
using namespace detail;

namespace {
// The shared middle of both routines: the change in r21 is scaled by one of the pool's rates (0x70 or
// 0x88) depending on the two club queries, limited to +-1000 (a pool rate and offset past that), applied to the
// club's rating row (a halfword clamped to 1..10000), then reported when it exceeds +-500.
struct Change {
    Cpu& c;
    Fpu& fpu;
    const Frame& frame;
    std::uint32_t doubles, ratings_slot, row_offset;
    std::uint32_t site_below, site_above;  // the fmadd sites of the two limits

    double pooled(std::uint32_t offset) const { return c.f64(doubles + offset); }
    std::int32_t change() const { return half_word(c.r(21)); }
    // The club's rating row: the 76-byte rows of the table behind the slot.
    std::uint32_t row(std::uint32_t club) const { return c.u32(c.u32(ratings_slot)) + c.u32(club) * 0x4c; }
    // f0 = rate * (change as a double); the result's low word is the new change.
    void scale(std::uint32_t rate) {
        const auto signed_change = signed_word(change());
        c.set(3, signed_change);
        const double bias = pooled(8), rate_value = pooled(rate);
        const auto top = fpu.fsub(stacked_integer(c, frame.at(0x11c), signed_change), bias);
        c.set_f(1, bias);
        const auto bits = fpu.fctiwz(0, fpu.fmul(rate_value, top));
        c.put64(frame.at(0x124), bits);
        c.set(21, c.u32(frame.at(0x128)));
    }
    // Which rate: the two queries (called with their own backs), then the sign of the change.
    template <class First, class Second>
    void apply_rates(First&& first, Second&& second) {
        if (nonzero(c, byte_word(first()))) {
            if (compare(c, change()) < 0) scale(0x70);
            else scale(0x88);
        } else if (nonzero(c, byte_word(second()))) {
            if (compare(c, change()) > 0) scale(0x70);
            else scale(0x88);
        }
    }
    // Past +-1000 the excess goes through a fused multiply-add: rate * excess + the pool's offset.
    void limit() {
        const double bias = pooled(8);
        const auto beyond = [&](std::int32_t excess, std::uint32_t site, std::uint32_t offset) {
            c.set(3, std::uint32_t(excess));
            const auto top = fpu.fsub(stacked_integer(c, frame.at(0x11c), signed_word(excess)), bias);
            c.set_f(1, top);
            const auto bits = fpu.fctiwz(0, fpu.multiply_add(site, pooled(0x90), top, pooled(offset)));
            c.put64(frame.at(0x124), bits);
            c.set(21, c.u32(frame.at(0x128)));
        };
        if (compare(c, change(), -0x3e8) < 0) beyond(change() + 0x3e8, site_below, 0x58);
        else if (compare(c, change(), 0x3e8) > 0) beyond(change() - 0x3e8, site_above, 0x50);
    }
    // The rating row takes the change and stays within 1..10000. Leaves the row in r3 and the value in r4.
    void apply(std::uint32_t club) {
        const std::uint32_t at = c.u32(ratings_slot);
        c.set(3, at);
        if (!nonzero_logical(c, club)) return;
        c.put16(row(club) + row_offset, std::uint32_t(c.i16(row(club) + row_offset)) + c.r(21));
        std::uint32_t value;
        if (compare(c, c.i16(row(club) + row_offset), 1) < 0) value = 1;
        else if (compare(c, c.i16(row(club) + row_offset), 0x2710) > 0) value = 0x2710;
        else value = extended(c.i16(row(club) + row_offset));
        leave(c, {{3, row(club)}, {4, value}});
        c.put16(row(club) + row_offset, value);
    }
    // False (nothing to report) for a change within +-500.
    bool reportable() {
        std::uint32_t small = 0;
        if (compare(c, change(), -0x1f4) >= 0 && compare(c, change(), 0x1f4) <= 0) small = 1;
        c.set(3, small);
        return !nonzero(c, std::int32_t(small));
    }
};
}

// 0x1066b600: `club`'s rating moves by `change` plus a share of its distance from 5000 (the first rate),
// then item 0xbe0 reports it with `other` (a rival club) and the kind and grade.
void change_rating_a(Cpu& c, std::uint32_t club, std::uint32_t other, std::uint32_t kind_register, std::uint32_t change_register,
                     std::uint32_t grade_register) {
    Frame frame(c, 0x1066b600, 0x170, {.first_gpr = 20});
    Fpu fpu(c);
    const auto rng = Stream::world(c);
    keep(c, {{20, kind_register}, {21, change_register}, {22, grade_register}, {23, 0}, {24, c.toc(-0x79f0)}, {25, c.toc(slot::floats)},
             {26, rng.seed}, {27, rng.start}, {28, rng.end}, {29, rng.cursor}, {30, club}, {31, other}});
    compare(c, std::int32_t(club));
    Change work{c, fpu, frame, c.toc(slot::floats), c.toc(-0x79f0), 8, 0x1066b820, 0x1066b86c};
    c.set(3, c.u32(c.r(24)));
    const std::int32_t rating = club ? half_word(c.u16(work.row(club) + 8)) : 0;
    const auto offset = std::int32_t(std::uint32_t(rating) - 0x1388);
    // The rating's distance from 5000 over the pool's divisor, truncated, added to the change; the sum
    // (as a halfword) scaled by the pool's first rate. The temporaries are the original's stack pairs.
    c.put32(frame.at(0x124), 0x43300000);
    const double bias = work.pooled(8);
    const auto distance = fpu.fsub(stacked_integer(c, frame.at(0x134), signed_word(offset)), bias);
    const auto manager = c.u32(club + 0x90);
    const auto share = fpu.fctiwz(0, fpu.fdiv(distance, work.pooled(0x48)));
    c.put64(frame.at(0x12c), share);
    c.set(21, c.r(21) + std::uint32_t(share));
    const double top = fpu.fsub(stacked_integer(c, frame.at(0x124), signed_word(work.change())), bias);
    c.set_f(1, bias);
    const auto scaled = fpu.fctiwz(0, fpu.fmul(work.pooled(0x80), top));
    c.put64(frame.at(0x11c), scaled);
    c.set(21, c.u32(frame.at(0x120)));
    leave(c, {{5, signed_word(offset)}});
    work.apply_rates([&] { return Program{c}.call(0x100d9480, 0x1066b6e0, {manager, club}); },
                     [&] { return Program{c}.call(0x100d94d0, 0x1066b76c, {c.u32(club + 0x90), club}); });
    work.limit();
    work.apply(club);
    if (!work.reportable()) return;
    const Item item{c, frame.at(0x38)};
    item.begin(0xbe0, 0x1066b94c);
    const auto field = [&](std::uint32_t value, std::uint32_t back) {
        const auto index = c.r(23);
        c.set(23, index + 1);
        item.put(index, value, back);
    };
    field(0, 0x1066b960);
    field(std::uint32_t(half_word(c.r(20))), 0x1066b974);
    field(std::uint32_t(byte_word(c.r(22))), 0x1066b988);
    rng.advance(0x1066b9a8);
    field(std::uint32_t(std::int32_t(rng.sample()) % 4), 0x1066b9e4);
    field(c.u32(club), 0x1066b9f8);
    if (compare(c, work.change(), -0x2ee) < 0) field(5, 0x1066ba18);
    else if (compare(c, work.change()) < 0) field(4, 0x1066ba38);
    else if (compare(c, work.change(), 0x2ee) > 0) field(2, 0x1066ba5c);
    else field(3, 0x1066ba74);
    const auto boss = c.u32(club + 0x90);
    field(c.u32(boss), 0x1066ba8c);
    field(linked(c, boss, 0x28), 0x1066baa8);
    field(linked(c, boss, 0x2c), 0x1066bac4);
    field(linked(c, boss, 0x30), 0x1066bae0);
    if (nonzero_logical(c, other)) {
        field(c.u32(other), 0x1066bafc);
        field(linked(c, other, 0x28), 0x1066bb14);
        field(linked(c, other, 0x2c), 0x1066bb2c);
        field(linked(c, other, 0x30), 0x1066bb44);
        if (nonzero(c, byte_word(Program{c}.call(0x100d9480, 0x1066bb50, {other, club})))) field(6, 0x1066bb6c);
        else if (nonzero(c, byte_word(Program{c}.call(0x100d94d0, 0x1066bb7c, {other, club})))) field(7, 0x1066bb98);
        else field(8, 0x1066bbb0);
    } else {
        field(0xffffffffu, 0x1066bbc8);
    }
    field(0xffffffffu, 0x1066bbdc);
    compare_logical(c, club);
    field(club ? extended(c.i16(work.row(club) + 8)) : 0, 0x1066bc18);
    item.put(c.r(23), 0, 0x1066bc28);
    item.post(club, 0x1066bc3c);
}

// 0x1066bc50: the same for the other table column (+6): `club`'s rating moves by `change` plus a twentieth of
// its distance from 6000 (halfword), and item 0xbe0 reports it, with `other`, the kind and grade and more flags
// about `other`.
void change_rating_b(Cpu& c, std::uint32_t club, std::uint32_t other, std::uint32_t kind_register, std::uint32_t change_register,
                     std::uint32_t grade_register, std::uint32_t flag_register) {
    Frame frame(c, 0x1066bc50, 0x170, {.first_gpr = 17});
    Fpu fpu(c);
    const auto rng = Stream::world(c);
    keep(c, {{17, rng.start}, {18, rng.end}, {19, rng.cursor}, {20, kind_register}, {21, change_register}, {22, grade_register}, {23, flag_register},
             {24, c.toc(-0x79f0)}, {25, rng.seed}, {26, c.toc(-0x7b7c)}, {27, c.toc(-0x7b3c)}, {28, c.toc(slot::floats)}, {29, club}, {30, other}, {31, 0}});
    compare(c, std::int32_t(club));
    Change work{c, fpu, frame, c.toc(slot::floats), c.toc(-0x79f0), 6, 0x1066be68, 0x1066beb4};
    c.set(3, c.u32(c.r(24)));
    const std::int32_t rating = club ? half_word(c.u16(work.row(club) + 6)) : 0;
    const auto distance = std::int32_t(std::uint32_t(rating) - 0x1770);
    c.put32(frame.at(0x124), 0x43300000);
    // A twentieth of the distance (rounding toward zero) as a halfword, added to the change.
    const auto high = multiply_high(0x66666667, distance) >> 3;
    const auto twentieth = std::uint32_t(high) >> 31;
    const auto share = std::int32_t(high) + std::int32_t(twentieth);
    const auto manager = c.u32(club + 0x90), rival = c.u32(club + 0x80);
    c.set(21, c.r(21) + std::uint32_t(half_word(std::uint32_t(share))));
    const double bias = work.pooled(8);
    const double top = fpu.fsub(stacked_integer(c, frame.at(0x124), signed_word(work.change())), bias);
    c.set_f(1, bias);
    const auto scaled = fpu.fctiwz(0, fpu.fmul(work.pooled(0x80), top));
    c.put64(frame.at(0x11c), scaled);
    c.set(21, c.u32(frame.at(0x120)));
    leave(c, {{5, twentieth}});
    work.apply_rates([&] { return Program{c}.call(0x100d9520, 0x1066bd28, {manager, rival}); },
                     [&] { return Program{c}.call(0x100d9580, 0x1066bdb4, {c.u32(club + 0x90), c.u32(club + 0x80)}); });
    work.limit();
    work.apply(club);
    if (!work.reportable()) return;
    const Item item{c, frame.at(0x38)};
    item.begin(0xbe0, 0x1066bf94);
    const auto field = [&](std::uint32_t value, std::uint32_t back) {
        const auto index = c.r(31);
        c.set(31, index + 1);
        item.put(index, value, back);
    };
    // The rating row's halfword at `column`, or 0 without a club.
    const auto rated = [&](std::uint32_t column) {
        compare_logical(c, club);
        c.set(3, c.u32(c.r(24)));
        return club ? half_word(c.u16(work.row(club) + column)) : 0;
    };
    field(1, 0x1066bfa8);
    field(std::uint32_t(half_word(c.r(20))), 0x1066bfbc);
    field(std::uint32_t(byte_word(c.r(22))), 0x1066bfd0);
    rng.advance(0x1066bff0);
    field(std::uint32_t(std::int32_t(rng.sample()) % 4), 0x1066c02c);
    field(c.u32(club), 0x1066c040);
    // Falling hard from a weak rating (5), falling from a strong one (9) or any (4); rising strongly from a strong
    // one (2), otherwise 3.
    if (compare(c, work.change(), -0x2ee) < 0 && compare(c, rated(6), 0x1194) < 0) field(5, 0x1066c094);
    else if (compare(c, work.change()) < 0 && compare(c, rated(6), 0x186a) > 0) field(9, 0x1066c0e8);
    else if (compare(c, work.change()) < 0) field(4, 0x1066c108);
    else if (compare(c, work.change(), 0x2ee) > 0 && compare(c, rated(6), 0x186a) > 0) field(2, 0x1066c160);
    else field(3, 0x1066c178);
    const auto boss = c.u32(club + 0x90);
    field(c.u32(boss), 0x1066c190);
    field(linked(c, boss, 0x28), 0x1066c1ac);
    field(linked(c, boss, 0x2c), 0x1066c1c8);
    field(linked(c, boss, 0x30), 0x1066c1e4);
    if (nonzero_logical(c, other)) {
        field(c.u32(other), 0x1066c200);
        field(linked(c, other, 0x28), 0x1066c218);
        field(linked(c, other, 0x2c), 0x1066c230);
        field(linked(c, other, 0x30), 0x1066c248);
        if (nonzero(c, byte_word(Program{c}.call(0x100d9520, 0x1066c254, {other, c.u32(club + 0x80)})))) field(6, 0x1066c270);
        else if (nonzero(c, byte_word(Program{c}.call(0x100d9580, 0x1066c280, {other, c.u32(club + 0x80)})))) field(7, 0x1066c29c);
        else field(8, 0x1066c2b4);
    } else {
        field(0xffffffffu, 0x1066c2cc);
    }
    if (nonzero_logical(c, c.u32(club + 0x80))) {
        const auto rival_record = c.u32(club + 0x80);
        field(c.u32(rival_record), 0x1066c2f0);
        field(linked(c, rival_record, 0x28), 0x1066c30c);
        field(linked(c, rival_record, 0x2c), 0x1066c328);
        field(linked(c, rival_record, 0x30), 0x1066c344);
    } else {
        field(0xffffffffu, 0x1066c35c);
    }
    field(std::uint32_t(rated(6)), 0x1066c398);
    if (nonzero(c, byte_word(c.r(23)))) {
        field(1, 0x1066c3b4);
        field(std::uint32_t(rated(0xe)), 0x1066c3f0);
    } else {
        field(0, 0x1066c408);
    }
    // Flags about `other`: each starts false and turns on under its tests.
    c.set(17, 0);
    compare_logical(c, other);
    if (other && nonzero_logical(c, c.u32(other + 0x14))) {
        const auto standing = Program{c}.call(0x10082f20, 0x1066c42c, {c.u32(c.r(26)), other});
        if (nonzero_logical(c, standing) && compare(c, byte_word(std::uint32_t(c.i16(other + 0x5c))), 0x19) > 0) {
            if (compare(c, std::int32_t(c.u8(standing + 0x60) & 0xf), 2) == 0 || compare(c, std::int32_t(c.u8(standing + 0x60) & 0xf), 1) == 0) c.set(17, 1);
        }
    }
    field(c.r(17), 0x1066c47c);
    compare_logical(c, other);
    c.set(17, 0);
    if (other && nonzero_logical(c, c.u32(other + 0x14))) {
        const auto world = c.u32(c.r(27));
        const auto grade = byte_word(Program{c}.call(0x10303090, 0x1066c4a0, {world, c.u32(other + 0x14)}));
        const auto reading = c.u32(c.r(27));
        const double measured = fpu.fsub(stacked_integer(c, frame.at(0x11c), signed_word(grade)), work.pooled(8));
        fpu.fcmp(0, c.f64(reading + 0x18), work.pooled(0x28));
        const bool unit = c.cr(0) & 2;
        c.set_f(1, unit ? c.f64(reading + 0x18) : c.f64(reading + 0x10));
        const double ratio = unit ? work.pooled(0x20) : fpu.fdiv(c.f64(reading + 0x10), c.f64(reading + 0x18));
        fpu.fcmp(0, measured, fpu.fmul(work.pooled(0x30), ratio));
        if (c.cr(0) & 4) c.set(17, 1);
    }
    field(std::uint32_t(byte_word(c.r(17))), 0x1066c510);
    for (const std::uint32_t column : {0x65u, 0x67u}) {
        std::uint32_t flag = 0;
        compare_logical(c, other);
        if (other && compare(c, byte_word(c.u8(other + column)), 0xe) > 0) flag = 1;
        c.set(5, flag);
        field(std::uint32_t(byte_word(flag)), column == 0x65 ? 0x1066c544 : 0x1066c578);
    }
    compare_logical(c, other);
    c.set(17, 0);
    if (other) {
        const auto distance_away = Program{c}.call(0x10718130, 0x1066c594, {c.toc(-0x7b80), c.u32(other + 0x4c), c.u32(other + 0x50)});
        if (compare(c, std::int32_t(distance_away), 0x5dc) > 0) c.set(17, 1);
    }
    field(std::uint32_t(byte_word(c.r(17))), 0x1066c5b8);
    compare_logical(c, other);
    c.set(17, 0);
    if (other) {
        const auto cost = Program{c}.call(0x10084be0, 0x1066c5e0, {c.u32(c.r(26)), other, club, 0, 0xffffffffu, 0});
        c.set(4, 0x44aa20);
        if (compare(c, std::int32_t(cost), 0x44aa20) > 0) c.set(17, 1);
    }
    field(std::uint32_t(byte_word(c.r(17))), 0x1066c60c);
    std::uint32_t last = 0;
    compare_logical(c, other);
    if (other && nonzero_logical(c, c.u32(other + 0x14)) && compare(c, std::int32_t(c.i16(c.u32(other + 0x14) + 4)), 0x8c) > 0) last = 1;
    item.put(c.r(31), std::uint32_t(byte_word(last)), 0x1066c648);
    item.post(club, 0x1066c65c);
}
}
