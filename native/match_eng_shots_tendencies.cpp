// MATCH_ENG.CPP, shots: a side's attacking tendencies (0x101df110). For every player on the pitch the
// routine derives eight ratings from his record (position flags, attributes, fitness, random draws),
// adds the ones his flag pattern selects to the side's seven tendency halfwords, and finally scales
// those by the competition and the gap between the two teams' ratings.
#include "match_eng_shots.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace shots_detail;

namespace {
constexpr std::uint32_t magic_at = 0x60;  // table +0x60: 2^52+2^31, the int-to-double constant
constexpr std::uint32_t tendencies_at = 0x1f4, morale_at = 0x36;
// Registers the original leaves in r3-r10 / r15-r22 (0: not kept; the writes nobody can observe are left out).
struct Triple { unsigned pointer = 0, byte = 0, extended = 0; };
struct Half { bool indexed = false; std::uint32_t offset = 0; unsigned side = 0, doubled = 0, address = 0; };
struct Regs { unsigned old = 0, xoris = 0, lis = 0, low = 0, coef_f = 0, magic_f = 0, conv_f = 0, sub_f = 0, mul_f = 0; };
struct Bump { unsigned side = 0, doubled = 0, address = 0, old = 0, low = 0, sum = 0, coef_f = 0, product_f = 0, truncated_f = 0; };
}

void update_side_tendencies(Cpu& c, std::uint32_t match, std::uint32_t side_argument) {
    Frame frame(c, 0x101df110, 0x120, {.first_gpr = 15, .first_fpr = 24});
    Fpu fp(c);
    keep(c, {{29, match}, {27, side_argument}});
    const auto assertion_text = c.toc(-0x52cc), pool = c.toc(-0x52c8), table = c.toc(-0x52c4);
    keep(c, {{23, c.toc(-0x7bac)}, {24, c.toc(-0x7ba8)}, {26, assertion_text}, {31, pool}, {30, table}});
    const auto side_byte = [&] { return u(s8(c.r(27))); };

    // The side's seven tendencies (one halfword per side at +4 intervals) start at zero.
    const auto doubled_side = side_byte() << 1;
    for (unsigned stat = 0; stat < 7; ++stat) c.put16(match + doubled_side + tendencies_at + 4 * stat, 0);
    c.set(6, doubled_side + 0x1fc);
    c.set(5, 0);
    c.set(4, side_byte());
    keep(c, {{15, side_byte() * side_size}});
    Kept<28> player(c, match + (c.r(15) + players_at));
    Kept<20> index(c, 0);

    // One tendency stat of the side, += trunc(coefficient * source rating).
    const auto bump = [&](unsigned stat, std::uint32_t coefficient, unsigned source, std::uint32_t slot, Bump r) {
        const auto side_extended = side_byte();
        if (r.side) c.set(r.side, side_extended);
        const auto doubled = side_extended << 1;
        if (r.doubled) c.set(r.doubled, doubled);
        const auto address = doubled + tendencies_at + 4 * stat;
        if (r.address) c.set(r.address, address);
        const auto old = u(std::int32_t(c.i16(match + address)));
        if (r.old) c.set(r.old, old);
        double term = c.f(source);
        if (coefficient) {  // 0: the rating itself
            const double coef = c.f64(table + coefficient);
            if (r.coef_f) c.set_f(r.coef_f, coef);
            term = fp.fmul(coef, c.f(source));
            if (r.product_f) c.set_f(r.product_f, term);
        }
        c.put64(frame.at(std::int32_t(slot)), fp.fctiwz(r.truncated_f, term));
        const auto low = c.u32(frame.at(std::int32_t(slot + 4)));
        if (r.low) c.set(r.low, low);
        const auto sum = old + low;
        if (r.sum) c.set(r.sum, sum);
        c.put16(match + address, sum);
    };
    // A tendency halfword read at `from` and written, scaled, at `to`: (int) (coefficient * (stat - 2^52-2^31 trick)).
    const auto half_address = [&](const Half& half) {
        if (!half.indexed) return half.offset;
        const auto side_extended = side_byte();
        if (half.side) c.set(half.side, side_extended);
        const auto doubled = side_extended << 1;
        if (half.doubled) c.set(half.doubled, doubled);
        const auto address = doubled + half.offset;
        if (half.address) c.set(half.address, address);
        return address;
    };
    const auto scale_half = [&](bool double_coefficient, const Half& from, const Half& to, std::uint32_t coefficient_at,
                                bool single_difference, bool single_product, std::uint32_t conversion, std::uint32_t slot,
                                unsigned truncated_f, Regs r) {
        const auto old = u(std::int32_t(c.i16(match + half_address(from))));
        if (r.old) c.set(r.old, old);
        const double coefficient = double_coefficient ? c.f64(table + coefficient_at) : double(c.f32(pool + coefficient_at));
        if (r.coef_f) c.set_f(r.coef_f, coefficient);
        const double magic = c.f64(table + magic_at);
        if (r.magic_f) c.set_f(r.magic_f, magic);
        const auto flipped = old ^ 0x80000000;
        if (r.xoris) c.set(r.xoris, flipped);
        if (r.lis) c.set(r.lis, 0x43300000);
        c.put32(frame.at(std::int32_t(conversion + 4)), flipped);
        c.put32(frame.at(std::int32_t(conversion)), 0x43300000);
        const double raw = c.f64(frame.at(std::int32_t(conversion)));
        if (r.conv_f) c.set_f(r.conv_f, raw);
        const double difference = single_difference ? fp.fsubs(raw, magic) : fp.fsub(raw, magic);
        if (r.sub_f) c.set_f(r.sub_f, difference);
        const double product = single_product ? fp.fmuls(coefficient, difference) : fp.fmul(coefficient, difference);
        if (r.mul_f) c.set_f(r.mul_f, product);
        c.put64(frame.at(std::int32_t(slot)), fp.fctiwz(truncated_f, product));
        const auto low = c.u32(frame.at(std::int32_t(slot + 4)));
        if (r.low) c.set(r.low, low);
        c.put16(match + half_address(to), low);
    };
    const auto scale_half_double = [&](const Half& from, const Half& to, std::uint32_t at, bool sub_single, bool mul_single,
                                       std::uint32_t conversion, std::uint32_t slot, unsigned truncated_f, Regs r = {}) {
        scale_half(true, from, to, at, sub_single, mul_single, conversion, slot, truncated_f, r);
    };
    const auto scale_half_single = [&](const Half& from, const Half& to, std::uint32_t at, bool sub_single, bool mul_single,
                                       std::uint32_t conversion, std::uint32_t slot, unsigned truncated_f, Regs r = {}) {
        scale_half(false, from, to, at, sub_single, mul_single, conversion, slot, truncated_f, r);
    };
    // A rating (f29 / f31 / f30) times a constant of the pool; `in_f1`: the constant is also left in f1.
    const auto scale = [&](unsigned rating, std::uint32_t at, bool in_f1 = false) {
        const double factor = c.f32(pool + at);
        if (in_f1) c.set_f(1, factor);
        c.set_f(rating, fp.fmuls(c.f(rating), factor));
    };
    // A byte of the player's record behind +0x70 compared with `limit` (lwz, lbz, extsb, cmpwi).
    const auto rung = [&](std::uint32_t offset, std::int32_t limit, Triple r = {}) {
        const auto record = c.u32(player + 0x70);
        if (r.pointer) c.set(r.pointer, record);
        const auto byte = c.u8(record + offset);
        if (r.byte) c.set(r.byte, byte);
        const auto value = s8(byte);
        if (r.extended) c.set(r.extended, u(value));
        return compare(c, value, limit);
    };
    const auto record_of = [&] { return c.u32(player + 0x70); };
    const auto stat_of = [&] { return std::int32_t(c.i16(record_of() + 4)); };
    // The flags (r25) the player's mode word returned, one bit at a time; `kept`: the register the original leaves the masked word in.
    const auto test = [&](std::uint32_t mask, unsigned kept = 0) {
        const auto value = u(std::int32_t(std::int16_t(c.r(25)))) & mask;
        if (kept) c.set(kept, value);
        return compare(c, std::int32_t(value), 0) != 0;
    };
    // Two draws, 20 against the record's byte 0x21: the second below the first lowers the rating.
    const auto lucky = [&](unsigned rating, std::uint32_t back_first, std::uint32_t back_second, unsigned record_register) {
        draw(c, 20, back_first);
        keep(c, {{15, c.r(3)}});
        if (record_register) c.set(record_register, record_of());
        draw(c, u(s8(c.u8(record_of() + 0x21))), back_second);
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(15))) < 0) scale(rating, 0x44);
    };
    // The player's weight in the squad sum: the fitness divided by the pool constant.
    const auto fitness_share = [&] {
        c.set(3, player);
        fitness(c, player);
        return fp.fdiv(c.f(1), c.f64(table + 0x28));
    };

    const auto visit = [&]() -> bool {
        c.set(3, player);
        const auto position_byte = s8(position(c, player));
        c.set(3, u(position_byte));
        if (compare(c, position_byte, 0) < 0) return true;
        if (compare_logical(c, c.u32(player + 0x6c), 0) == 0) {
            // The assert macro: a player without a person record.
            report_assertion(c, {.at = 0x101df21c, .file = assertion_text + 0x45, .format = assertion_text + 0x53,
                                 .title = assertion_text + 0x60, .name = frame.at(0x41), .extension = frame.at(0x3c), .line = 0x181f});
            c.set(5, 0);  // li r5,0 at 0x101df250, for the flag the macro clears
            return false;
        }

        // --- rating 1 (f29) ---
        const auto b3c = c.u8(record_of() + 0x3c);
        c.set(6, b3c);
        const auto gap = u(0x1e - s8(b3c)), flipped_gap = gap ^ 0x80000000;
        const double magic = c.f64(table + magic_at);
        c.set(4, flipped_gap);
        c.put32(frame.at(0x94), flipped_gap);
        c.put32(frame.at(0x90), 0x43300000);
        c.set_f(1, fp.fsubs(c.f64(frame.at(0x90)), magic));
        c.set(5, UINT32_MAX);
        c.set(3, player);
        c.call(0x10239590, 0x101df294);
        c.set(3, player);
        c.call(0x10289560, 0x101df29c);
        keep(c, {{25, c.r(3)}});
        const double share = fitness_share();
        c.set_f(1, c.f32(pool + 0x54));
        {
            double sum = fp.fadds(c.f32(player + 0xd0), c.f32(player + 0xe0));
            sum = fp.fadds(c.f32(player + 0x80), sum);
            sum = fp.fadds(c.f32(player + 0xc4), sum);
            sum = fp.fadds(c.f32(pool + 0x2c), sum);
            double blend = fp.multiply_add(0x101df2dc, sum, c.f32(pool + 0x54), c.f32(pool + 0x50));
            blend = fp.fadd(blend, share);
            c.set_f(29, fp.frsp(fp.fsub(blend, c.f64(table + 0x28))));
        }
        if (compare_logical(c, record_of(), 0) != 0) {
            const int trait = s8(c.u8(record_of() + 0x2f)) >> 2;
            c.set_f(29, fp.fadds(c.f(29), from_int_single(c, fp, frame.at(0x90), u(trait), c.f64(table + magic_at))));
            // The player's stat (halfword +4 of the record) times a coefficient the flags choose.
            const auto term = [&](std::uint32_t coefficient_at) { return std::pair{c.f64(table + coefficient_at), c.f64(table + magic_at)}; };
            if (test(4)) {
                const auto [coef, magic_value] = term(0x1c0);
                const auto converted = from_int(c, fp, frame.at(0x90), u(stat_of()), magic_value);
                c.set_f(1, c.f64(frame.at(0x90)));
                c.set_f(24, fp.fmul(coef, converted));
                c.set_f(29, fp.fadds(c.f(29), fp.frsp(c.f(24))));
            } else if (test(2)) {
                const auto [coef, magic_value] = term(0x1c0);
                const auto converted = from_int(c, fp, frame.at(0x90), u(stat_of()), magic_value);
                c.set_f(1, fp.frsp(fp.fmul(coef, converted)));
                c.set_f(29, fp.fadds(c.f(29), c.f(1)));
            } else if (test(8)) {
                const auto [coef, magic_value] = term(0x1f0);
                const auto converted = from_int(c, fp, frame.at(0x90), u(stat_of()), magic_value);
                c.set_f(1, fp.frsp(fp.fmul(coef, converted)));
                c.set_f(29, fp.fadds(c.f(29), c.f(1)));
            } else {
                const auto [coef, magic_value] = term(0x1f8);
                const auto converted = from_int(c, fp, frame.at(0x90), u(stat_of()), magic_value);
                c.set_f(1, fp.fmul(coef, converted));
                c.set_f(29, fp.fadds(c.f(29), fp.frsp(c.f(1))));
            }
            if (rung(0x32, 0xf) > 0) scale(29, 0x58);
            else if (rung(0x32, 0x6) < 0) scale(29, 0x5c);
            else if (rung(0x32, 0xa) < 0) scale(29, 0x60);
            if (rung(0x3c, 0xf) > 0) scale(29, 0x58, 1);
            else if (rung(0x3c, 0x6) < 0) scale(29, 0x64);
            else if (rung(0x3c, 0xa) < 0) scale(29, 0x5c);
            if (rung(0x44, 0xf) > 0) scale(29, 0x68, 1);
            else if (rung(0x44, 0x6) < 0) scale(29, 0x5c);
            else if (rung(0x44, 0xa, {0, 0, 6}) < 0) scale(29, 0x60, 1);
            if (rung(0x3e, 0xf) > 0) scale(29, 0x58);
            else if (rung(0x3e, 0x6) < 0) scale(29, 0x5c);
            else if (rung(0x3e, 0xa) < 0) scale(29, 0x60);
            if (rung(0x1e, 0xf) > 0) scale(29, 0x58);
            else if (rung(0x1e, 0x6) < 0) scale(29, 0x5c);
            else if (rung(0x1e, 0xa) < 0) scale(29, 0x60);
            if (rung(0x2e, 0xf) > 0) scale(29, 0x68, 1);
            else if (rung(0x2e, 0x6) < 0) scale(29, 0x5c);
            else if (rung(0x2e, 0xa) < 0) scale(29, 0x60);
            if (rung(0x2b, 0xf) > 0) scale(29, 0x68, 1);
            else if (rung(0x2b, 0x6) < 0) scale(29, 0x5c);
            else if (rung(0x2b, 0xa, {0, 0, 15}) < 0) scale(29, 0x60);
            // The sum of two attributes as a byte.
            {
                const auto record = record_of();
                c.set(5, record);
                const auto first = c.u8(record + 0x36);
                const auto second = c.u8(record_of() + 0x1b);
                c.set(4, second);
                keep(c, {{19, u(s8(first + second))}});
                const auto total = s8(c.r(19));
                if (compare(c, total, 0x23) > 0) scale(29, 0x68, true);
                else if (compare(c, s8(c.r(19)), 0xa) < 0) scale(29, 0x64);
                else if (compare(c, s8(c.r(19)), 0x14) < 0) scale(29, 0x60);
            }
            lucky(29, 0x101df71c, 0x101df730, 0);
        }

        // --- rating 2 (f31) ---
        {
            const double share2 = fitness_share();
            double sum = fp.fmuls(c.f32(pool + 0x18), c.f32(player + 0xc8));
            sum = fp.multiply_add(0x101df778, c.f32(pool + 0x18), c.f32(player + 0xf4), sum);
            sum = fp.fadds(c.f32(player + 0xa0), sum);
            sum = fp.fadds(c.f32(player + 0xac), sum);
            sum = fp.multiply_add(0x101df784, c.f64(table + 0x1b8), c.f32(player + 0x90), sum);
            sum = fp.fadd(c.f64(table + 0x130), sum);
            double blend = fp.multiply_add(0x101df794, sum, c.f64(table + 0x90), c.f64(table + 0x200));
            blend = fp.fadd(blend, share2);
            c.set_f(1, blend);
            c.set_f(31, fp.frsp(fp.fsub(blend, c.f64(table + 0x28))));
        }
        if (compare_logical(c, record_of(), 0) != 0) {
            // The flags choose the coefficient for the stat term.
            const auto flagged_term = [&](std::uint32_t coefficient_at) {
                const double coef = c.f64(table + coefficient_at);
                const double raw_magic = c.f64(table + magic_at);
                const auto converted = from_int(c, fp, frame.at(0x90), u(stat_of()), raw_magic);
                c.set_f(1, converted);
                c.set_f(31, fp.fadds(c.f(31), fp.frsp(fp.fmul(coef, converted))));
            };
            if (test(0x40)) flagged_term(0x1c0);
            else if (test(0x20)) flagged_term(0x208);
            else if (test(0x10)) flagged_term(0x210);
            else if (test(8)) flagged_term(0x218);
            else flagged_term(0x1f8);
            if (rung(0x44, 0xf) > 0) scale(31, 0x58);
            else if (rung(0x44, 0x5) <= 0) scale(31, 0x6c);
            if (rung(0x3c, 0xf) > 0) scale(31, 0x68);
            else if (rung(0x3c, 0x5) <= 0) scale(31, 0x5c);
            if (rung(0x35, 0xf) > 0) scale(31, 0x70);
            else if (rung(0x35, 0x5) <= 0) scale(31, 0x64);
            if (rung(0x37, 0xf) > 0) scale(31, 0x74);
            else if (rung(0x37, 0x6) < 0) scale(31, 0x78);
            else if (rung(0x37, 0xa) < 0) scale(31, 0x6c);
            if (rung(0x43, 0xf) > 0) scale(31, 0x74);
            else if (rung(0x43, 0x6) < 0) scale(31, 0x78);
            else if (rung(0x43, 0xa) < 0) scale(31, 0x6c);
            // The sum of four attributes as a byte.
            {
                const auto b40 = c.u8(record_of() + 0x40), b26 = c.u8(record_of() + 0x26), b36 = c.u8(record_of() + 0x36), b1b = c.u8(record_of() + 0x1b);
                keep(c, {{22, u(s8(b1b + b26 + b40 + b36))}});
                if (compare(c, s8(c.r(22)), 0x4b) > 0) scale(31, 0x7c);
                else if (compare(c, s8(c.r(22)), 0x41) > 0) scale(31, 0x58);
                else if (compare(c, s8(c.r(22)), 0x32) > 0) scale(31, 0x68);
                else if (compare(c, s8(c.r(22)), 0x23) > 0) {}
                else if (compare(c, s8(c.r(22)), 0x19) > 0) scale(31, 0x60);
                else scale(31, 0x6c);
            }
            if (rung(0x1e, 0xf) > 0) scale(31, 0x70);
            else if (rung(0x1e, 0xa) < 0) scale(31, 0x5c);
            else if (rung(0x1e, 0x6) < 0) scale(31, 0x64);
            if (rung(0x33, 0xf, {6, 0, 4}) > 0) scale(31, 0x58);
            else if (rung(0x33, 0x6) < 0) scale(31, 0x5c);
            else if (rung(0x33, 0xa, {4, 0, 0}) < 0) scale(31, 0x60);
            if (rung(0x3f, 0xf, {5, 0, 0}) > 0) scale(31, 0x68);
            else if (rung(0x3f, 0xa, {4, 5, 0}) < 0) scale(31, 0x5c);
            else if (rung(0x3f, 0x5, {0, 0, 4}) < 0) scale(31, 0x64);
            lucky(31, 0x101dfc40, 0x101dfc54, 0);
        }

        // --- rating 3 (f30) ---
        {
            const double share3 = fitness_share();
            double sum = fp.fadds(c.f32(player + 0x98), c.f32(player + 0x74));
            sum = fp.multiply_add(0x101dfca8, c.f64(table + 0x1b8), c.f32(player + 0x80), sum);
            sum = fp.fadd(fp.fmuls(c.f32(pool + 0x18), c.f32(player + 0x9c)), sum);
            sum = fp.multiply_add(0x101dfcb0, c.f64(table + 0x1b8), c.f32(player + 0xbc), sum);
            sum = fp.fadd(c.f32(player + 0xac), sum);
            sum = fp.fadd(c.f32(player + 0xc4), sum);
            sum = fp.fadd(c.f64(table + 0x130), sum);
            double blend = fp.multiply_add(0x101dfcc8, sum, c.f64(table + 0x90), c.f64(table + 0x220));
            blend = fp.fadd(blend, share3);
            c.set_f(1, blend);
            c.set_f(30, fp.frsp(fp.fsub(blend, c.f64(table + 0x28))));
        }
        c.set(4, record_of());
        if (compare_logical(c, record_of(), 0) != 0) {
            const auto flagged_term = [&](std::uint32_t coefficient_at) {
                const double coef = c.f64(table + coefficient_at);
                const double raw_magic = c.f64(table + magic_at);
                c.set_f(1, raw_magic);
                const auto converted = from_int(c, fp, frame.at(0x90), u(stat_of()), raw_magic);
                c.set_f(30, fp.fadds(c.f(30), fp.frsp(fp.fmul(coef, converted))));
            };
            if (test(0x40)) flagged_term(0x168);
            else if (test(0x20)) flagged_term(0x228);
            else if (test(0x10)) flagged_term(0x230);
            else if (test(8)) flagged_term(0x238);
            else {
                const double coef = c.f64(table + 0x1f8);
                const double raw_magic = c.f64(table + magic_at);
                c.set_f(1, raw_magic);
                const auto converted = from_int(c, fp, frame.at(0x90), u(stat_of()), raw_magic);
                c.set_f(1, fp.fmul(coef, converted));
                c.set_f(30, fp.fadds(c.f(30), fp.frsp(c.f(1))));
            }
            if (rung(0x44, 0xf) > 0) scale(30, 0x68);
            else if (rung(0x44, 0x6) < 0) scale(30, 0x5c);
            else if (rung(0x44, 0xa) < 0) scale(30, 0x60);
            if (rung(0x3c, 0xf) > 0) scale(30, 0x68);
            else if (rung(0x3c, 0x6) < 0) scale(30, 0x5c, 1);
            else if (rung(0x3c, 0xa) < 0) scale(30, 0x60);
            if (rung(0x35, 0xf) > 0) scale(30, 0x58);
            else if (rung(0x35, 0xa) > 0) scale(30, 0x68);
            else if (rung(0x35, 0x6) < 0) scale(30, 0x6c);
            if (rung(0x31, 0xf) > 0) scale(30, 0x80);
            else if (rung(0x31, 0xa) > 0) scale(30, 0x68, 1);
            else if (rung(0x31, 0x6) < 0) scale(30, 0x60);
            // The sum of four attributes as a byte.
            {
                const auto b40 = c.u8(record_of() + 0x40);
                c.set(6, b40);
                const auto b26 = c.u8(record_of() + 0x26), b36 = c.u8(record_of() + 0x36);
                const auto record = record_of();
                c.set(5, record);
                const auto b1b = c.u8(record + 0x1b);
                keep(c, {{21, u(s8(b1b + b26 + b40 + b36))}});
                if (compare(c, s8(c.r(21)), 0x4b) > 0) scale(30, 0x7c);
                else if (compare(c, s8(c.r(21)), 0x41) > 0) scale(30, 0x58);
                else {
                    c.set(5, u(s8(c.r(21))));
                    if (compare(c, s8(c.r(21)), 0x32) > 0) scale(30, 0x68);
                    else if (compare(c, s8(c.r(21)), 0xa) < 0) scale(30, 0x40);
                    else if (compare(c, s8(c.r(21)), 0x19) < 0) scale(30, 0x84, true);
                    else scale(30, 0x5c);
                }
            }
            if (rung(0x1e, 0xf) > 0) scale(30, 0x7c);
            else if (rung(0x1e, 0x6, {5, 0, 0}) < 0) scale(30, 0x64, 1);
            else if (rung(0x1e, 0xa) < 0) scale(30, 0x88);
            if (rung(0x33, 0xf, {0, 4, 0}) > 0) scale(30, 0x7c);
            else if (rung(0x33, 0xa) > 0) scale(30, 0x68);
            else if (rung(0x33, 0x6, {0, 4, 0}) < 0) scale(30, 0x6c);
            if (rung(0x40, 0xf) > 0) scale(30, 0x58);
            lucky(30, 0x101e0134, 0x101e0148, 5);
        }

        // --- ratings 4 to 8 (f28, f24, f27, f25, f26) ---
        {
            const double share4 = fitness_share();
            double sum = fp.fadds(c.f32(player + 0xa8), c.f32(player + 0xd4));
            sum = fp.fadds(c.f32(player + 0xd0), sum);
            sum = fp.fadds(c.f32(pool + 0x2c), sum);
            c.set_f(1, c.f32(pool + 0x54));
            double blend = fp.multiply_add(0x101e018c, sum, c.f32(pool + 0x54), c.f32(pool + 0x2c));
            blend = fp.fadd(blend, share4);
            c.set_f(28, fp.frsp(fp.fsub(blend, c.f64(table + 0x28))));
        }
        c.set_f(24, fitness_share());
        c.set(3, player);
        c.set(4, 0);
        c.call(0x102875f0, 0x101e01bc);
        c.set(4, u(s8(c.r(3))));
        {
            const double magic4 = c.f64(table + magic_at);
            double sum = fp.fadds(c.f32(player + 0x78), from_int_single(c, fp, frame.at(0x90), c.r(4), magic4));
            sum = fp.fadds(c.f32(pool + 0x2c), sum);
            double blend = fp.multiply_add(0x101e01f8, sum, c.f32(pool + 0x54), c.f32(pool + 0x2c));
            blend = fp.fadd(blend, c.f(24));
            c.set_f(1, blend);
            c.set_f(27, fp.frsp(fp.fsub(blend, c.f64(table + 0x28))));
        }
        {
            const double share5 = fitness_share();
            double sum = fp.fadds(c.f32(player + 0xac), c.f32(player + 0xb0));
            sum = fp.fadds(c.f32(player + 0xd0), sum);
            sum = fp.fadds(c.f32(pool + 0x2c), sum);
            double blend = fp.multiply_add(0x101e0240, sum, c.f32(pool + 0x54), c.f32(pool + 0x2c));
            blend = fp.fadd(blend, share5);
            c.set_f(1, blend);
            c.set_f(25, fp.frsp(fp.fsub(blend, c.f64(table + 0x28))));
        }
        {
            const double share6 = fitness_share();
            double sum = fp.fadds(c.f32(player + 0xa8), c.f32(player + 0xb0));
            sum = fp.fadds(c.f32(player + 0xd0), sum);
            sum = fp.fadds(c.f32(pool + 0x2c), sum);
            double blend = fp.multiply_add(0x101e0288, sum, c.f32(pool + 0x54), c.f32(pool + 0x2c));
            blend = fp.fadd(blend, share6);
            c.set_f(1, blend);
            c.set_f(26, fp.frsp(fp.fsub(blend, c.f64(table + 0x28))));
        }

        // --- the flags choose which tendencies this player feeds, and what it does to his morale ---
        const auto adjust = [&](unsigned loaded, std::uint32_t delta, std::uint32_t factor_at, std::uint32_t back) {
            const auto morale = u(std::int32_t(c.i16(player + morale_at)));
            if (loaded) c.set(loaded, morale);
            c.put16(player + morale_at, morale - delta);
            c.set(3, match);
            c.set(4, player);
            c.set_f(1, c.f32(pool + factor_at));
            call(c, 0x101e15e0, back);
        };
        if (test(1)) {
            adjust(15, 3, 0x8c, 0x101e02c8);
            bump(0, 0x190, 29, 0x90, {.truncated_f=1});  // 101e02c8
            bump(3, 0x98, 28, 0x88, {.truncated_f=8});  // 101e02f4
            bump(4, 0x1c0, 27, 0x80, {.low=5, .truncated_f=5});  // 101e0338
            bump(6, 0x190, 26, 0x78, {.old=4, .sum=6, .truncated_f=2});  // 101e0364
        } else if (test(2)) {
            adjust(5, 0xd, 0x64, 0x101e03cc);
            bump(0, 0x198, 29, 0x78, {.truncated_f=0});  // 101e03cc
            bump(1, 0x168, 31, 0x80, {.truncated_f=0});  // 101e03f8
            bump(2, 0x1c0, 30, 0x88, {.truncated_f=0});  // 101e0424
            bump(3, 0xe0, 28, 0x90, {.truncated_f=1});  // 101e0450
            bump(4, 0x90, 27, 0x70, {.truncated_f=0});  // 101e047c
            bump(5, 0x118, 25, 0x68, {.low=6, .product_f=1, .truncated_f=0});  // 101e04a8
            bump(6, 0x90, 26, 0x60, {.doubled=5, .sum=4, .truncated_f=0});  // 101e04d4
        } else if (test(4, 5)) {
            adjust(0, 0xd, 0x64, 0x101e0530);
            bump(0, 0x0, 29, 0x60, {.truncated_f=2});  // 101e0530
            bump(1, 0x170, 31, 0x68, {.truncated_f=0});  // 101e0554
            bump(2, 0x1c0, 30, 0x70, {.truncated_f=2});  // 101e0580
            bump(3, 0x168, 28, 0x78, {.truncated_f=0});  // 101e05ac
            bump(4, 0x90, 27, 0x80, {.old=15, .truncated_f=2});  // 101e05d8
            bump(5, 0x118, 25, 0x88, {.sum=6, .coef_f=1, .truncated_f=0});  // 101e0604
            bump(6, 0x90, 26, 0x90, {.address=4, .old=5, .truncated_f=2});  // 101e0630
        } else if (test(8)) {
            adjust(0, 0xc, 0x84, 0x101e068c);
            bump(0, 0x1a0, 29, 0x60, {.coef_f=1, .truncated_f=0});  // 101e068c
            bump(1, 0x178, 31, 0x68, {.truncated_f=0});  // 101e06b8
            bump(2, 0x1c8, 30, 0x70, {.truncated_f=0});  // 101e06e4
            bump(3, 0x170, 28, 0x78, {.truncated_f=0});  // 101e0710
            bump(4, 0x90, 27, 0x80, {.truncated_f=0});  // 101e073c
            bump(5, 0x118, 25, 0x88, {.truncated_f=0});  // 101e0768
            bump(6, 0x90, 26, 0x90, {.low=5, .sum=4, .truncated_f=0});  // 101e0794
        } else if (test(0x10)) {
            adjust(0, 8, 0x5c, 0x101e07f0);
            bump(0, 0x1a8, 29, 0x60, {.low=6, .truncated_f=0});  // 101e07f0
            bump(1, 0x118, 31, 0x68, {.truncated_f=0});  // 101e081c
            bump(2, 0x1d0, 30, 0x70, {.truncated_f=0});  // 101e0848
            bump(3, 0x1b0, 28, 0x78, {.truncated_f=0});  // 101e0874
            bump(4, 0x90, 27, 0x80, {.truncated_f=8});  // 101e08a0
            bump(5, 0x118, 25, 0x88, {.sum=4, .truncated_f=2});  // 101e08cc
            bump(6, 0x90, 26, 0x90, {.low=15, .sum=5, .coef_f=1, .truncated_f=4});  // 101e08f8
        } else if (test(0x20, 4)) {
            adjust(4, 0xb, 0x58, 0x101e0954);
            bump(0, 0x170, 29, 0x60, {.old=15, .truncated_f=1});  // 101e0954
            bump(1, 0x180, 31, 0x68, {.truncated_f=1});  // 101e0980
            bump(2, 0x1d8, 30, 0x70, {.truncated_f=1});  // 101e09ac
            bump(3, 0x1e8, 28, 0x78, {.truncated_f=1});  // 101e09d8
            bump(4, 0x90, 27, 0x80, {.truncated_f=0});  // 101e0a04
            bump(5, 0x118, 25, 0x88, {.product_f=1, .truncated_f=0});  // 101e0a30
            bump(6, 0x1e0, 26, 0x90, {.doubled=6, .low=5, .sum=4, .truncated_f=0});  // 101e0a5c
        } else if (test(0x40)) {
            adjust(0, 0x19, 0x74, 0x101e0ab8);
            bump(0, 0x1b0, 29, 0x60, {.truncated_f=0});  // 101e0ab8
            bump(1, 0x188, 31, 0x68, {.truncated_f=0});  // 101e0ae4
            bump(2, 0x1b8, 30, 0x70, {.truncated_f=0});  // 101e0b10
            bump(3, 0x1e8, 28, 0x78, {.truncated_f=0});  // 101e0b3c
            bump(4, 0x90, 27, 0x80, {.truncated_f=0});  // 101e0b68
            bump(5, 0x1b8, 25, 0x88, {.side=6, .truncated_f=0});  // 101e0b94
            bump(6, 0x168, 26, 0x90, {.doubled=4, .old=5, .product_f=1, .truncated_f=0});  // 101e0bc0
        }
        return true;
    };

    for (; compare(c, s8(index), 0x14) < 0; ) {
        const bool more = visit();
        if (!more) return;
        index = index + 1;
        c.set(3, player);
        const std::uint32_t previous = player;
        player = player + player_size;
        c.put32(frame.at(0x38), previous);
    }

    // --- the fixture's tendencies are scaled by the competition and the teams' ratings ---
    const auto fixture_record = [&] { return c.u32(match + fixture_at); };
    const auto team_one = [&] { return c.u32(fixture_record() + 0x1c); };
    const auto team_two = [&] { return c.u32(fixture_record() + 0x20); };
    const auto competition_id = [&] { return std::int32_t(c.u32(c.u32(team_one() + 0x2c) + 0x68)); };
    const auto well_known = [&](std::int32_t at) { return std::int32_t(c.u32(c.toc(at))); };  // a competition id behind a pool pointer
    const auto is_competition = [&](std::int32_t at) { return compare(c, competition_id(), well_known(at)) == 0; };
    const auto rating_of = [&](std::uint32_t team) { return std::int32_t(c.i16(team + 0x1ec)); };
    const auto flag_at = [&](std::int32_t offset) { return c.u8(match + 0x10000 + u(offset)); };

    // The competition decides how strongly the second tendency is scaled. Quirk: it is read at side 0's
    // halfword (+0x1f8) and written at this side's, so the second side overwrites its own with side 0's scaled value.
    do {
        if (compare_logical(c, fixture_record(), 0) == 0) break;
        if (compare_logical(c, team_one(), 0) == 0) break;
        const auto first_reference = c.u32(fixture_record() + 0x14);
        if (compare_logical(c, first_reference, 0) == 0) break;
        const auto cup_team = team_one();
        c.set(6, cup_team);
        if (compare_logical(c, c.u32(first_reference + 8), c.u32(cup_team + 0x2c)) != 0) break;
        if (compare_logical(c, c.u32(team_one() + 0x2c), 0) == 0) break;
        if (!is_competition(-0x7af4) && !is_competition(-0x7a1c)) {
            keep(c, {{15, team_one()}});
            if (!is_competition(-0x7510)) {
                if (is_competition(-0x7514)) {
                    scale_half_single({.offset=0x1f8}, {.indexed=true, .offset=0x1f8}, 0x90, true, true, 0x60, 0x68, 0);  // 101e0d30
                } else if (is_competition(-0x7504) || is_competition(-0x7a10)) {
                    scale_half_single({.offset=0x1f8}, {.indexed=true, .offset=0x1f8}, 0x58, true, true, 0x60, 0x68, 0);  // 101e0db8
                } else if (is_competition(-0x7a28) || is_competition(-0x750c) || is_competition(-0x79fc) || is_competition(-0x7a00)) {
                    scale_half_single({.offset=0x1f8}, {.indexed=true, .offset=0x1f8}, 0x70, true, true, 0x60, 0x68, 0, {.xoris=6});  // 101e0e80
                }
                break;
            }
        }
        scale_half_single({.offset=0x1f8}, {.indexed=true, .offset=0x1f8}, 0x74, true, true, 0x60, 0x68, 0, {.low=6});  // 101e0cc8
    } while (false);
    scale_half_single({.indexed=true, .offset=0x1f8}, {.indexed=true, .offset=0x1f8}, 0x58, true, true, 0x60, 0x68, 5);  // 101e0ec4
    scale_half_single({.indexed=true, .offset=0x1fc, .address=5}, {.indexed=true, .offset=0x1fc}, 0x68, true, true, 0x70, 0x78, 4, {.sub_f=1});  // 101e0f14

    // A knock-out reference lets the first tendency through; otherwise the check at 0x10494d20 decides.
    {
        bool scaled = true;
        if (compare_logical(c, c.u32(fixture_record() + 0x14), 0) != 0) {
            const auto handle = c.toc(-0x7b54);
            if (compare_logical(c, c.u32(handle), 0) == 0) scaled = false;
            else {
                c.set(3, c.u32(handle));
                c.set(4, c.u32(fixture_record() + 0x14));
                c.call(0x10494d20, 0x101e0f98);
                const auto verdict = s8(c.r(3));
                c.set(3, u(verdict));
                if (compare(c, verdict, 0) == 0) scaled = false;
            }
        }
        if (scaled) {
            scale_half_single({.offset=0x1f8}, {.indexed=true, .offset=0x1f8}, 0x70, true, true, 0x60, 0x68, 0, {.lis=15, .conv_f=1, .low=5});  // 101e0fa4
        }
    }

    // Two teams, both rated below 2500: the second and third tendencies are scaled.
    do {
        if (compare_logical(c, team_one(), 0) == 0) break;
        if (compare_logical(c, team_two(), 0) == 0) break;
        keep(c, {{15, fixture_record()}});
        if (compare(c, rating_of(team_one()), 0x9c4) >= 0) break;
        if (compare(c, rating_of(team_one()), 0x9c4) >= 0) break;
        scale_half_single({.indexed=true, .offset=0x1f8}, {.indexed=true, .offset=0x1f8}, 0x80, true, true, 0x60, 0x68, 0);  // 101e1030
        scale_half_single({.indexed=true, .offset=0x1fc}, {.indexed=true, .offset=0x1fc, .doubled=5}, 0x74, true, true, 0x70, 0x78, 0, {.coef_f=1, .conv_f=24});  // 101e1080
    } while (false);

    // The first side's tendencies follow the gap between the two teams' ratings.
    do {
        c.set(3, fixture_record());
        if (compare_logical(c, fixture_record(), 0) == 0) break;
        c.set(3, side_byte());
        if (compare(c, s8(c.r(27)), 0) != 0) break;
        c.set(3, team_one());
        if (compare_logical(c, team_one(), 0) == 0) break;
        if (compare_logical(c, team_two(), 0) == 0) break;
        c.set(6, u(rating_of(team_two())));
        c.set(5, u(rating_of(team_one())));
        const auto difference = u(rating_of(team_one()) - rating_of(team_two()));
        keep(c, {{17, u(std::int32_t(std::int16_t(difference)))}});
        if (compare(c, std::int32_t(c.r(17)), 0x3e8) > 0) {
            scale_half_double({.offset=0x1f8}, {.offset=0x1f8}, 0x188, true, false, 0x60, 0x68, 0);  // 101e1130
            scale_half_double({.offset=0x1fc}, {.offset=0x1fc}, 0x188, true, false, 0x70, 0x78, 4);  // 101e1168
            scale_half_double({.offset=0x1f4}, {.offset=0x1f4}, 0x188, true, false, 0x80, 0x88, 0, {.old=3, .conv_f=1});  // 101e11a0
        } else if (compare(c, std::int32_t(std::int16_t(c.r(17))), -0xfa) > 0) {
            scale_half_double({.offset=0x1f8}, {.offset=0x1f8}, 0x188, true, false, 0x60, 0x68, 0);  // 101e11e8
            scale_half_double({.offset=0x1fc}, {.offset=0x1fc}, 0x240, true, false, 0x70, 0x78, 0, {.low=3});  // 101e1220
            scale_half_double({.offset=0x1f4}, {.offset=0x1f4}, 0x240, true, false, 0x80, 0x88, 0, {.mul_f=1});  // 101e1258
        } else {
            scale_half_double({.offset=0x1f4}, {.offset=0x1f4}, 0x188, true, false, 0x60, 0x68, 1);  // 101e1294
            scale_half_double({.offset=0x1f8}, {.offset=0x1f8}, 0x248, true, false, 0x70, 0x78, 5, {.xoris=3});  // 101e12cc
        }
    } while (false);

    // The same for the second side, and the knock-out guard bytes.
    c.set(4, fixture_record());
    if (compare_logical(c, fixture_record(), 0) == 0) return;
    c.set(6, fixture_record());
    c.set(4, team_one());
    if (compare_logical(c, team_one(), 0) == 0) return;
    c.set(4, fixture_record());
    c.set(5, team_two());
    if (compare_logical(c, team_two(), 0) == 0) return;
    if (compare(c, s8(c.r(27)), 0) == 0) {
        c.set(4, u(rating_of(team_two())));
        c.set(5, team_one());
        const auto gap = u(rating_of(team_one()) - rating_of(team_two()));
        keep(c, {{18, u(std::int32_t(std::int16_t(gap)))}});
        if (compare(c, std::int32_t(c.r(18)), 0x4e2) > 0) {
            if (compare_logical(c, flag_at(-0x1fc3), 0) == 0) {
                scale_half_double({.offset=0x1fc}, {.offset=0x1fc}, 0x188, true, false, 0x60, 0x68, 0);  // 101e1374
            }
            scale_half_double({.offset=0x1f4}, {.offset=0x1f4}, 0x198, true, false, 0x60, 0x68, 0, {.xoris=3, .lis=4, .mul_f=1, .low=5});  // 101e13ac
            return;
        }
        c.set(3, c.r(18));
        if (compare(c, std::int32_t(c.r(3)), 0x2ee) > 0) {
            c.set(6, match + 0x10000);
            if (compare_logical(c, flag_at(-0x1fc3), 0) == 0) {
                scale_half_double({.offset=0x1fc}, {.offset=0x1fc}, 0x188, true, false, 0x60, 0x68, 0, {.low=3});  // 101e1404
            }
            scale_half_double({.offset=0x1f4}, {.offset=0x1f4}, 0x250, true, false, 0x60, 0x68, 0, {.old=4, .magic_f=1, .low=5});  // 101e143c
            return;
        }
        c.set(3, c.r(18));
        if (compare(c, std::int32_t(c.r(3)), 0xfa) <= 0) return;
        if (compare_logical(c, flag_at(-0x1fc3), 0) == 0) {
            scale_half_double({.offset=0x1fc}, {.offset=0x1fc}, 0x240, true, false, 0x60, 0x68, 0, {.old=5});  // 101e1494
        }
        scale_half_double({.offset=0x1f4}, {.offset=0x1f4}, 0x188, true, false, 0x60, 0x68, 0, {.old=4, .magic_f=1, .xoris=15, .low=3});  // 101e14cc
        return;
    }
    c.set(4, s8(c.r(27)));
    if (compare(c, s8(c.r(27)), 1) != 0) return;
    c.set(3, team_two());
    c.set(5, u(rating_of(team_two())));
    c.set(6, team_one());
    const auto away_gap = u(rating_of(team_one()) - rating_of(team_two()));
    c.set(4, away_gap);
    keep(c, {{16, u(std::int32_t(std::int16_t(away_gap)))}});
    if (compare(c, std::int32_t(c.r(16)), -0x2ee) >= 0) return;
    c.set(4, match + 0x10000);
    c.set(5, flag_at(-0x1f8d));
    if (compare_logical(c, c.r(5), 0) != 0) return;
    scale_half_double({.offset=0x1fe}, {.offset=0x1fe}, 0x188, true, false, 0x60, 0x68, 0, {.lis=4});  // 101e154c
    scale_half_double({.offset=0x1f6}, {.offset=0x1f6}, 0x240, true, false, 0x70, 0x78, 0, {.magic_f=1, .low=3});  // 101e1584
}
}
