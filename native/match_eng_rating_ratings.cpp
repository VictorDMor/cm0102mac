// MATCH_ENG.CPP, rating: a person's three ratings for a fixture (0x101f9470).
#include "match_eng_rating.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace rating_detail;

namespace {
constexpr std::uint32_t magic_at = 0x60;  // pool 2 +0x60: 2^52+2^31
constexpr std::uint16_t cup_codes[] = {0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
                                       0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b};
}

// 0x101f9470
void compute_ratings(Cpu& c, std::uint32_t output, std::uint32_t person_, std::uint32_t club_, std::uint32_t opponent_,
                     std::uint32_t fixture_, double weight, std::uint32_t flag_a, std::uint32_t flag_b) {
    Frame frame(c, 0x101f9470, 0x1e0, {.first_gpr = 16, .first_fpr = 18});
    const auto at = [&](std::uint32_t offset) { return frame.at(std::int32_t(offset)); };
    Kept<22> out(c, output);
    Kept<31> person(c, person_);
    Kept<23> club(c, club_);
    Kept<28> opponent(c, opponent_);
    Kept<30> fixture(c, fixture_);
    c.set_f(25, weight);
    c.put8(at(0x213), flag_a);
    c.put8(at(0x217), flag_b);
    const auto pool1 = c.toc(-0x52c8), pool2 = c.toc(-0x52c4);
    keep(c, {{21, pool1}, {29, pool2}});
    // The floors the three ratings are held to at the end (rating_a, rating_b, rating_c).
    Kept<17> floor_a(c, 1);
    Kept<19> floor_b(c, 1);
    Kept<16> floor_c(c, 1);
    Fpu fp(c);
    // The compiler's stack slots: an integer under the magic exponent, and an fctiwz result.
    const auto wide = [&](std::uint32_t slot, std::uint32_t value) { return widen(c, at(slot), value); };
    const auto trunc = [&](std::uint32_t slot, double value, unsigned reg = 0) {
        c.put64(at(slot), fp.fctiwz(reg, value));
        return c.u32(at(slot) + 4);
    };
    c.set_f(31, weight);
    c.set(3, 0);
    c.put8(at(0x38), 0);  // 1 when the person belongs to the fixture's club
    Kept<18> band_c(c, c.r(18));
    Kept<27> level(c, c.r(27));

    const auto d = [&](std::uint32_t offset) { return c.f64(pool2 + offset); };
    const auto s = [&](std::uint32_t offset) { return double(c.f32(pool1 + offset)); };
    const auto strength = [&] { return u(std::int32_t(c.i16(opponent + 0x1ec))); };
    const auto sheet = [&] { return c.u32(person + 0x14); };
    const auto tier = [&] { return std::int32_t(c.i16(c.u32(fixture + 0x14) + 0x18)); };
    const auto lt = [&](double a, double b) { fp.fcmp(0, a, b); return (c.cr(0) & 8) != 0; };
    const auto gt = [&](double a, double b) { fp.fcmp(0, a, b); return (c.cr(0) & 4) != 0; };
    // fcmpo; cror eq,gt,eq; bne: a >= b, false when unordered.
    const auto at_least = [&](double a, double b) {
        fp.fcmp(0, a, b);
        const auto cr = c.cr(0);
        const auto equal = std::uint8_t(((cr >> 1) | cr) & 2);
        c.set_cr(0, std::uint8_t((cr & ~2) | equal));
        return equal != 0;
    };
    const auto check_club = [&](std::uint32_t back) {
        c.set(3, club);
        c.call(0x100c7d00, back);
        return compare(c, s8(c.r(3)), 0) != 0;
    };
    const auto available = [&](std::uint32_t back) {
        c.set({{3, c.u32(c.toc(-0x7a64))}, {4, c.u32(fixture + 0x14)}});
        c.call(0x102aed80, back);
        return compare(c, s8(c.r(3)), 0) != 0;
    };

    // The person's level: the fixture's tier, blended with the opponent's strength in some fixtures.
    if (compare(c, c.i32(fixture), -2) == 0) {
        level = 5;
    } else if (compare(c, s8(c.u8(fixture + 0x40)), 0) != 0) {
        const auto sum = u(tier()) + u(divide_by_magic(c.i16(opponent + 0x1ec), 5));
        level = x8(u(std::int32_t(sum + (sum >> 31)) >> 1));
    } else if (compare_logical(c, c.u32(c.toc(-0x7a64)), 0) != 0) {
        if (available(0x101f9548)) {
            c.put32(at(0xa8), compare(c, tier(), 0xf) > 0 ? u(tier()) : 0xf);
            level = x8(c.u32(at(0xa8)));
        } else {
            level = x8(u(tier()));
        }
    } else {
        level = x8(u(tier()));
    }
    if (compare(c, s8(level), 0x14) > 0) level = 0x14;  // the level is a signed byte: extsb before every use
    if (compare_logical(c, out, 0) != 0 && at_least(c.f(31), d(0x310))) {
        // The output record's flags weaken the weight.
        if (compare_logical(c, c.u8(out + 0xc), 0) == 0) c.set_f(31, fp.fdivs(c.f(31), s(0x70)));
        if (compare_logical(c, c.u8(out + 0x10), 0) == 0) c.set_f(31, fp.fdivs(c.f(31), s(0x58)));
        // The two bytes are compared by their difference, not by equality of the halves.
        if (compare(c, std::int32_t(c.u8(out + 0xd)) - std::int32_t(c.u8(out + 0xe)), 0) != 0) c.set_f(31, fp.fdivs(c.f(31), s(0x58)));
        if (compare(c, s8(c.u8(out + 0x23)), 0) != 0) c.set_f(31, fp.fdivs(c.f(31), s(0x70)));
    }

    // The ceiling of a sheet value in bytes: its thousands plus 16, at least 22 (stack byte `slot`).
    const auto ceiling = [&](std::uint32_t slot, std::uint32_t field) {
        c.put8(at(slot), x8(u(divide_by_magic(c.i16(sheet() + field), 6)) + 0x10));
        if (compare(c, s8(c.u8(at(slot))), 0x16) < 0) c.put8(at(slot), 0x16);
    };
    ceiling(0x3a, 0xa);
    c.put16(at(0x60), x16(u(s8(c.u8(at(0x3a)))) - u(s8(level))));  // room for rating_a
    bool own = check_club(0x101f967c);
    if (!own) {
        if (compare(c, c.i32(fixture), -2) == 0 && compare_logical(c, c.u32(club + 0x2c), 0) != 0 &&
            compare_logical(c, c.u32(club + 0x2c), c.u32(person + 0xc)) == 0)
            own = true;
        else if (compare_logical(c, c.u32(fixture + 0x14), 0) != 0 &&
                 compare_logical(c, c.u32(c.u32(fixture + 0x14) + 8), c.u32(person + 0xc)) == 0)
            own = true;
        else
            own = compare_logical(c, c.u32(club + 0x2c), c.u32(person + 0xc)) == 0;
    }
    if (own) {
        c.put8(at(0x38), 1);
        ceiling(0x39, 8);
        c.put16(at(0x5e), x16(u(s8(c.u8(at(0x39)))) - u(s8(level))));  // room for rating_b
    } else {
        ceiling(0x39, 8);
        c.put16(at(0x56), x16(0xca - u(s8(level)) * 10));
        c.put16(at(0x54), x16(u(s8(c.u8(at(0x39)))) - u(s8(level))));
        const auto sum = u(std::int32_t(c.i16(at(0x56)))) + u(std::int32_t(c.i16(at(0x54))));
        c.put16(at(0x5e), x16(u(std::int32_t(sum + (sum >> 31)) >> 1)));
        if (compare(c, c.i16(at(0x5e)), 4) < 0) c.put16(at(0x5e), 4);
    }

    // (weight - base) * (opponent strength - offset), divided and capped as the pool says: the original
    // evaluates it once to compare with the cap and again to keep it.
    struct Ramp {
        std::uint32_t base, offset, divisor, cap, widen_slot, trunc_slot, spill;
        unsigned freg;
    };
    const auto ramp = [&](const Ramp& r) {
        const auto product = [&] {
            const double high = fp.fsub(c.f(31), d(r.base));
            const double count = fp.fsub(wide(r.widen_slot, strength() - r.offset), d(magic_at));
            double value = fp.fmul(high, count);
            if (r.divisor) value = fp.fdiv(value, d(r.divisor));
            return value;
        };
        double value = product();
        if (r.cap) {
            const double cap = d(r.cap);
            value = lt(value, cap) ? product() : cap;
        }
        if (r.spill) {
            c.put_f64(at(r.spill), value);
            value = c.f64(at(r.spill));
        }
        if (r.freg) c.set_f(r.freg, value);
        return trunc(r.trunc_slot, value);
    };

    // Quirk: the club test runs twice (the first answer only picks the room for rating_b).
    if (check_club(0x101f97f4)) {
        band_c = x8(u(divide_by_magic(c.i16(sheet() + 0xc), 6)) + 0x10);
        if (compare(c, s8(band_c), 0x15) < 0) band_c = 0x15;
        c.put16(at(0x5c), x16(u(s8(band_c)) - u(s8(level))));
        if (compare(c, s8(c.u8(fixture + 0x3a)), 0xa) == 0 && compare_logical(c, c.u32(c.toc(-0x7a64)), 0) != 0 &&
            available(0x101f9878) && compare(c, tier(), 0x11) >= 0 && at_least(c.f(31), d(0xf8)) &&
            compare(c, c.i16(opponent + 0x1ec), 0x157c) > 0) {
            const auto code_32 = [&] { return std::int32_t(c.i16(fixture + 0x32)); };
            const auto code_30 = [&] { return std::int32_t(c.i16(fixture + 0x30)); };
            bool league_cup = false;
            for (std::int32_t code = 0x41f; code <= 0x426 && !league_cup; ++code) league_cup = compare(c, code_32(), code) == 0;
            if (league_cup || compare(c, code_30(), 0x8c) == 0) {
                floor_a = ramp({0xf8, 0x1194, 0, 0x318, 0x128, 0x128, 0, 24});
                if (compare(c, s8(c.u8(at(0x38))), 0) != 0) floor_b = ramp({0xf8, 0x1194, 0, 0x318, 0x128, 0x128, 0, 23});
                else floor_b = ramp({0xf8, 0x1194, 0x1b8, 0x320, 0x128, 0x128, 0, 22});
                floor_c = ramp({0xf8, 0x1194, 0x1b8, 0x320, 0x128, 0x128, 0, 21});
            } else if (compare(c, code_30(), 0x14) == 0) {
                // Quirk: in these kinds all three floors take the one value.
                floor_c = ramp({0x328, 0x1194, 0, 0x330, 0x128, 0x128, 0, 20});
                floor_b = floor_c;
                floor_a = floor_b;
            } else if (compare(c, code_30(), 0x78) == 0) {
                floor_c = ramp({0x328, 0xfa0, 0, 0x338, 0x128, 0x128, 0, 19});
                floor_b = floor_c;
                floor_a = floor_b;
            } else if (compare(c, code_30(), 0x82) == 0) {
                floor_c = ramp({0x328, 0xfa0, 0, 0x340, 0x128, 0x128, 0, 18});
                floor_b = floor_c;
                floor_a = floor_b;
            } else if (compare(c, code_30(), 0x96) == 0) {
                floor_c = ramp({0x88, 0xfa0, 0, 0x348, 0x128, 0x128, 0xe0, 0});
                floor_b = floor_c;
                floor_a = floor_b;
            }
        }
    } else if (compare(c, s8(c.u8(fixture + 0x3a)), 8) == 0) {
        const auto code_32 = [&] { return std::int32_t(c.i16(fixture + 0x32)); };
        bool final_round = false;
        for (std::int32_t code = 0x43d; code <= 0x448 && !final_round; ++code) final_round = compare(c, code_32(), code) == 0;
        if (final_round) {
            band_c = x8(u(divide_by_magic(c.i16(sheet() + 0xc), 6)) + 0x10);
            if (compare(c, s8(band_c), 0x15) < 0) band_c = 0x15;
            c.put16(at(0x5c), x16(u(s8(band_c)) - u(s8(level)) + 1));
            if (at_least(c.f(31), d(0x310)) && compare(c, c.i16(opponent + 0x1ec), 0x157c) > 0) {
                floor_a = ramp({0x310, 0x157c, 0x2c0, 0, 0x128, 0x120, 0, 0});
                if (compare(c, s8(c.u8(at(0x38))), 0) != 0) floor_b = ramp({0x310, 0x157c, 0x2c0, 0, 0x120, 0x128, 0, 0});
                else floor_b = ramp({0x310, 0x157c, 0x40, 0, 0x120, 0x128, 0, 0});
                floor_c = ramp({0x310, 0x157c, 0x40, 0, 0x120, 0x128, 0, 0});
            }
        } else {
            bool cup = false;
            for (const auto code : cup_codes) {
                if (cup) break;
                cup = compare(c, std::int32_t(c.i16(fixture + 0x30)), code) == 0;
            }
            if (cup) {
                band_c = x8(u(divide_by_magic(c.i16(sheet() + 0xc), 6)) + 0x10);
                if (compare(c, s8(band_c), 0x15) < 0) band_c = 0x15;
                level = x8(u(tier()));  // quirk: the level is replaced by the tier here, and kept for the rest of the routine
                c.put16(at(0x5c), x16(u(s8(band_c)) - u(s8(level))));
                if (at_least(c.f(31), d(0xa0)) && compare(c, c.i16(opponent + 0x1ec), 0x157c) > 0) {
                    floor_a = ramp({0xf8, 0x1194, 0, 0x318, 0x120, 0x120, 0xd8, 0});
                    if (compare(c, s8(c.u8(at(0x38))), 0) != 0) floor_b = ramp({0xf8, 0x1194, 0, 0x318, 0x120, 0x120, 0xd0, 0});
                    else floor_b = ramp({0xf8, 0x1194, 0x1b8, 0x320, 0x120, 0x120, 0xc8, 0});
                    floor_c = ramp({0xf8, 0x1194, 0x1b8, 0x320, 0x120, 0x120, 0xc0, 0});
                }
            } else {
                c.put16(at(0x5c), x16(0xca - u(s8(level)) * 10));
            }
        }
    } else {
        c.put16(at(0x5c), x16(0xca - u(s8(level)) * 10));
    }

    // The rating of the league in which the person's team plays, compared with the weight.
    const auto record = [&] { return c.u32(c.toc(-0x7b3c)); };
    const auto rate = [&](std::uint32_t back) {
        const auto holder = record();
        enter(c, back, {holder, c.u8(at(0x213))});
        return side_rate(c, holder, c.u8(at(0x213)));
    };
    c.set_f(29, fp.frsp(fp.fsub(c.f(25), rate(0x101fa3cc))));
    if (lt(c.f(29), s(0xc))) {
        c.set_f(1, c.f(29));
        enter(c, 0x101fa3e8);
        const double magnitude = absolute(c, c.f(29));
        c.set_f(29, fp.fmuls(c.f(29), fp.frsp(magnitude)));
    }
    // One rating from the level and the sheet value: (pool +0x130 * f29) * (level * pool +0x90 + pool +0x30) + value.
    const auto component = [&](std::uint32_t level_slot, std::uint32_t value_slot, std::uint32_t store_slot, std::uint32_t field,
                               std::uint32_t site_a, std::uint32_t site_b) {
        const double scale = fp.fmul(d(0x130), c.f(29));
        const double level_value = fp.fsubs(wide(level_slot, x8(level)), d(magic_at));
        const double slope = fp.frsp(fp.multiply_add(site_a, level_value, d(0x90), d(0x30)));
        const double value = fp.fsubs(wide(value_slot, u(std::int32_t(c.i16(sheet() + field)))), d(magic_at));
        return trunc(store_slot, fp.multiply_add(site_b, scale, slope, value)) ;
    };
    Kept<24> rating_b(c, component(0x120, 0x128, 0x118, 8, 0x101fa420, 0x101fa44c));
    Kept<26> rating_a(c, component(0x110, 0x108, 0x100, 0xa, 0x101fa48c, 0x101fa4b8));
    Kept<25> rating_c(c, component(0xf8, 0xf0, 0xe8, 0xc, 0x101fa4f8, 0x101fa524));

    const auto i16v = [](std::uint32_t value) { return std::int32_t(std::int16_t(value)); };
    const auto load16 = [&](std::uint32_t offset) { return u(std::int32_t(c.i16(at(offset)))); };
    const auto quotient = [](std::uint32_t numerator, std::uint32_t denominator) {
        return std::uint32_t(divide_signed<4>(numerator, denominator));
    };
    if (compare(c, s8(c.u8(at(0x217))), 0) != 0) {
        // The weight against the league rating, for a floor under each rating.
        const double league = rate(0x101fa554);
        const double scaled = fp.fdiv(fp.fmuls(c.f(25), fp.fsubs(wide(0xe8, u(s8(level)) * 0xfa), d(magic_at))), league);
        c.put16(at(0x5a), trunc(0xf0, scaled));
        if (lt(c.f(29), s(0xc))) {
            const double again = rate(0x101fa5ac);
            const double value = fp.fmuls(fp.fsubs(wide(0xe8, load16(0x5a)), d(magic_at)), c.f(25));
            c.put16(at(0x5a), trunc(0xf0, fp.fdiv(value, again)));
        }
        if (compare(c, c.i16(at(0x5a)), i16v(rating_a)) > 0) c.put16(at(0x52), c.u16(at(0x5a)));
        else c.put16(at(0x52), rating_a);
        rating_a = load16(0x52);
        if (compare(c, c.i16(at(0x5a)), i16v(rating_b)) > 0) c.put16(at(0x50), c.u16(at(0x5a)));
        else c.put16(at(0x50), rating_b);
        rating_b = load16(0x50);
        if (compare(c, c.i16(at(0x5a)), i16v(rating_c)) > 0) c.put16(at(0x4e), c.u16(at(0x5a)));
        else c.put16(at(0x4e), rating_c);
        rating_c = load16(0x4e);
    }
    // Each rating is averaged with the room left under its ceiling.
    rating_a = x16(quotient(u(i16v(rating_a)) + u(c.i16(at(0x60)) - 1) * u(c.i16(sheet() + 0xa)), u(c.i16(at(0x60)))));
    rating_b = x16(quotient(u(i16v(rating_b)) + u(c.i16(at(0x5e)) - 1) * u(c.i16(sheet() + 8)), u(c.i16(at(0x5e)))));
    rating_c = x16(quotient(u(i16v(rating_c)) + u(c.i16(at(0x5c)) - 1) * u(c.i16(sheet() + 0xc)), u(c.i16(at(0x5c)))));
    c.put32(at(0xa4), compare(c, c.i16(sheet() + 0xa), 1) > 0 ? u(std::int32_t(c.i16(sheet() + 0xa))) : 1);
    const auto sheet_value = [&](std::uint32_t slot, std::uint32_t field) {
        return fp.fsubs(wide(slot, u(std::int32_t(c.i16(sheet() + field)))), d(magic_at));
    };
    // The change to rating_a: a share of its distance from the sheet value, then a random part of it.
    const double lean = fp.fmul(d(0x350), fp.fsubs(wide(0xe8, x16(rating_a) - u(std::int32_t(c.i16(sheet() + 0xa)))), d(magic_at)));
    const double span = fp.fsubs(wide(0xf0, c.u32(at(0xa4))), d(magic_at));
    const double shift = fp.fadd(d(0x90), fp.multiply_add(0x101fa768, c.f(25), d(0x90), fp.fdiv(lean, span)));
    Kept<20> adjust(c, trunc(0xf8, shift));
    if (compare(c, i16v(adjust), 0) > 0) {
        const auto room = x16(rating_a) - u(std::int32_t(c.i16(sheet() + 0xa)));
        if (compare(c, i16v(adjust), std::int32_t(room)) < 0) c.put32(at(0xa0), x16(adjust));
        else c.put32(at(0xa0), room);
        const auto roll = draw(c, 3, 0x101fa7cc);
        adjust = x16(roll + c.u32(at(0xa0)));
    } else if (compare(c, i16v(adjust), 0) < 0) {
        const double down = fp.fmul(d(0x1b8), fp.fsub(wide(0xe8, x16(adjust)), d(magic_at)));
        const double room = fp.fsub(wide(0xf0, x16(rating_a) - u(std::int32_t(c.i16(sheet() + 0xa)))), d(magic_at));
        double bound;
        if (gt(down, room)) bound = fp.fmul(d(0x1b8), fp.fsub(wide(0xe8, x16(adjust)), d(magic_at)));
        else bound = fp.fsub(wide(0xe8, x16(rating_a) - u(std::int32_t(c.i16(sheet() + 0xa)))), d(magic_at));
        c.put_f64(at(0xb8), bound);
        const auto roll = draw(c, 3, 0x101fa8ac);
        const double part = fp.fsub(wide(0xe8, roll), d(magic_at));
        adjust = trunc(0xf0, fp.fsub(c.f64(at(0xb8)), part));
    }
    if (compare(c, i16v(adjust), 0) < 0) {
        // Quirk: the draw's bound is the signed byte at person+0x64; the result plus one divides the adjustment.
        const auto count = draw(c, x8(c.u8(person + 0x64)), 0x101fa8f4);
        c.put_f32(at(0xb0), float(fp.fsubs(wide(0xe8, count + 1), d(magic_at))));
        c.put_f32(at(0xac), float(fp.frsp(fp.fmul(c.f32(at(0xb0)), d(0xe0)))));
        const double numerator = fp.fsubs(wide(0xf0, x16(adjust)), d(magic_at));
        adjust = trunc(0xf8, fp.fdivs(numerator, c.f32(at(0xac))));
    }
    // Quirk: the adjustment, held to +-20, is the bound of a draw (negative bounds included); the draw is the new adjustment.
    if (compare(c, i16v(adjust), -0x14) < 0) {
        c.put32(at(0x9c), u(-0x14));
    } else {
        if (compare(c, i16v(adjust), 0x14) > 0) c.put32(at(0x98), 0x14);
        else c.put32(at(0x98), x16(adjust));
        c.put32(at(0x9c), c.u32(at(0x98)));
    }
    adjust = x16(draw(c, c.u32(at(0x9c)), 0x101fa9ac));
    if (compare_logical(c, out, 0) != 0) {
        c.set({{3, out}, {4, x8(adjust)}});
        c.call(0x10288f30, 0x101fa9c4);
    } else {
        c.set({{3, person}, {4, x8(adjust)}, {5, 1}});
        c.call(0x100d3570, 0x101fa9d8);
    }
    c.set_f(28, fp.frsp(fp.fsub(d(0x98), fp.fdiv(sheet_value(0xe8, 0xa), d(0x38)))));
    c.set_f(27, fp.frsp(fp.fsub(d(0x98), fp.fdiv(sheet_value(0xf0, 8), d(0x38)))));
    c.set_f(26, fp.frsp(fp.fsub(d(0x98), fp.fdiv(sheet_value(0xf8, 0xc), d(0x38)))));

    // Each rating stays within the level's reach of the sheet value (halfwords through stack temporaries).
    const auto level_bound = [&] { return x16(u(s8(level)) * 0x12c + 0xfa0); };
    const auto reach = [&](auto& rating, std::uint32_t field, std::uint32_t first, std::uint32_t second, std::uint32_t keep_at) {
        const auto widest = [&](std::uint32_t slot) {
            const auto bound = level_bound();
            if (compare(c, i16v(bound), c.i16(sheet() + field)) > 0) c.put16(at(slot), bound);
            else c.put16(at(slot), u(std::int32_t(c.i16(sheet() + field))));
        };
        widest(first);
        if (compare(c, i16v(rating), c.i16(at(first))) < 0) {
            c.put16(at(keep_at), rating);
        } else {
            widest(second);
            c.put16(at(keep_at), c.u16(at(second)));
        }
        rating = load16(keep_at);
    };
    // The sheet's level also bounds the rating to a band around (sheet level - 75 .. + 50) * 50.
    const auto range = [&](auto& rating, std::uint32_t low_offset, std::uint32_t high_offset, std::uint32_t final_slot, std::uint32_t temp_slot) {
        const auto base = std::int32_t(c.i16(sheet() + 4));
        const auto low = u(base - std::int32_t(low_offset)) * 0x32;
        if (compare(c, std::int32_t(low), i16v(rating)) > 0) {
            c.put32(at(final_slot), low);
        } else {
            const auto high = u(base + std::int32_t(high_offset)) * 0x32;
            if (compare(c, std::int32_t(high), i16v(rating)) < 0) c.put32(at(temp_slot), high);
            else c.put32(at(temp_slot), x16(rating));
            c.put32(at(final_slot), c.u32(at(temp_slot)));
        }
        rating = x16(c.u32(at(final_slot)));
    };
    reach(rating_a, 0xa, 0x4c, 0x48, 0x4a);
    range(rating_a, 0x4b, 0x32, 0x94, 0x90);
    // Then within a few spreads of the sheet value, in single precision.
    {
        const double lower = fp.multiply_add(0x101fabd8, s(0xfc), c.f(28), sheet_value(0xe8, 0xa));
        const double current = fp.fsubs(wide(0xf0, x16(rating_a)), d(magic_at));
        if (gt(lower, current)) {
            c.put_f32(at(0x8c), float(fp.multiply_add(0x101fac2c, s(0xfc), c.f(28), sheet_value(0xe8, 0xa))));
        } else {
            const double upper = fp.multiply_add(0x101fac60, s(0x120), c.f(28), sheet_value(0xe8, 0xa));
            if (lt(upper, fp.fsubs(wide(0xf0, x16(rating_a)), d(magic_at))))
                c.put_f32(at(0x88), float(fp.multiply_add(0x101facb4, s(0x120), c.f(28), sheet_value(0xe8, 0xa))));
            else
                c.put_f32(at(0x88), float(fp.fsubs(wide(0xe8, x16(rating_a)), d(magic_at))));
            c.put_f32(at(0x8c), c.f32(at(0x88)));
        }
        rating_a = trunc(0xe8, c.f32(at(0x8c)));
    }
    reach(rating_b, 8, 0x46, 0x42, 0x44);
    range(rating_b, 0x4b, 0x32, 0x84, 0x80);
    {
        const double lower = fp.multiply_add(0x101fae54, s(0xfc), c.f(27), sheet_value(0xe8, 8));
        const double current = fp.fsubs(wide(0xf0, x16(rating_b)), d(magic_at));
        if (gt(lower, current)) {
            c.put_f32(at(0x7c), float(fp.multiply_add(0x101faea8, s(0xfc), c.f(27), sheet_value(0xe8, 8))));
        } else {
            const double upper = fp.multiply_add(0x101faedc, s(0xf8), c.f(27), sheet_value(0xe8, 8));
            if (lt(upper, fp.fsubs(wide(0xf0, x16(rating_b)), d(magic_at))))
                c.put_f32(at(0x78), float(fp.multiply_add(0x101faf30, s(0xf8), c.f(27), sheet_value(0xe8, 8))));
            else
                c.put_f32(at(0x78), float(fp.fsubs(wide(0xe8, x16(rating_b)), d(magic_at))));
            c.put_f32(at(0x7c), c.f32(at(0x78)));
        }
        rating_b = trunc(0xe8, c.f32(at(0x7c)));
    }
    reach(rating_c, 0xc, 0x40, 0x3c, 0x3e);
    range(rating_c, 0xc8, 0x19, 0x74, 0x70);
    {
        c.set(4, u(std::int32_t(c.i16(sheet() + 0xc))));  // lha r4: the last r4 unless the branches below set another
        const double lower = fp.multiply_add(0x101fb0d0, s(0xf0), c.f(26), sheet_value(0xe8, 0xc));
        const double current = fp.fsubs(wide(0xf0, x16(rating_c)), d(magic_at));
        if (gt(lower, current)) {
            c.put_f32(at(0x6c), float(fp.multiply_add(0x101fb124, s(0xf0), c.f(26), sheet_value(0xe8, 0xc))));
        } else {
            const double hundreds = fp.fsubs(wide(0xe8, u(s8(level)) * 0x64), d(magic_at));
            const double upper = fp.multiply_add(0x101fb178, c.f(26), hundreds, sheet_value(0xf0, 0xc));
            if (lt(upper, fp.fsubs(wide(0xf8, x16(rating_c)), d(magic_at)))) {
                const double again = fp.fsubs(wide(0xe8, u(s8(level)) * 0x64), d(magic_at));
                c.put_f32(at(0x68), float(fp.multiply_add(0x101fb1ec, c.f(26), again, sheet_value(0xf0, 0xc))));
            } else {
                c.set(4, 0x43300000);  // lis r4 in the widening
                c.put_f32(at(0x68), float(fp.fsubs(wide(0xe8, x16(rating_c)), d(magic_at))));
            }
            c.put_f32(at(0x6c), c.f32(at(0x68)));
        }
        rating_c = trunc(0xe8, c.f32(at(0x6c)), 1);  // f1 keeps the fctiwz result
    }
    // A club that is not a national side: the person's form pulls rating_a up.
    if (!check_club(0x101fb23c) && compare(c, s8(level), 0xa) > 0) {
        c.set({{3, c.u32(c.toc(-0x7b7c))}, {4, person}, {5, club}});
        c.call(0x10082f90, 0x101fb268);
        c.put32(at(0xb4), c.r(3));
        if (compare_logical(c, c.u32(at(0xb4)), 0) != 0) {
            c.set(4, u(std::int32_t(c.i16(person + 0x5c))));
            // Quirk: the halfword at person+0x5c is read through extsb, so only its low byte counts.
            const double form = fp.fsubs(wide(0xe8, x8(c.u16(person + 0x5c))), d(magic_at));
            c.set_f(30, form);
            const double cap = s(0x124);
            if (lt(cap, c.f(30))) {
                c.set_f(1, cap);
                c.put_f32(at(0x64), float(cap));
            } else {
                c.put_f32(at(0x64), float(c.f(30)));
            }
            c.set_f(30, c.f32(at(0x64)));
            c.set_f(1, fp.fdiv(c.f(30), d(0x358)));
            c.set_f(2, d(0x40));
            c.call(0x10780fc0, 0x101fb2cc);
            c.set(2, c.u32(frame.at(0x14)));
            c.set_f(30, fp.frsp(fp.fmul(c.f(1), d(0x90))));
            c.set(3, c.u32(at(0xb4)));
            c.link(0x101fb2e4);
            const auto kind = s8(low_nibble_60(c, c.u32(at(0xb4))));
            const auto raw_strength = [&] { return wide(0xe8, u(std::int32_t(c.i16(club + 0x1ec)))); };
            const auto stored = [&](double value, unsigned reg = 0) { c.put16(at(0x58), trunc(0xf0, value, reg)); };
            // f1 ends up as the original leaves it in each case.
            const auto weighted = [&](std::uint32_t multiplier, int f1_is) {
                const double raw = raw_strength();
                const double strength = fp.fsubs(raw, d(magic_at));
                const double times = fp.fmuls(c.f(30), strength);
                const double result = multiplier ? fp.fmul(d(multiplier), times) : times;
                c.set_f(1, f1_is == 0 ? strength : f1_is == 1 ? result : f1_is == 2 ? times : raw);
                stored(result);
            };
            const int order = compare(c, kind, 3);
            if (order == 0) weighted(0, 2);
            else if (order > 0) {
                const int upper = compare(c, kind, 5);
                if (upper == 0) weighted(0, 3);
                else if (upper > 0) c.put16(at(0x58), 0);
                else { weighted(0x108, 1); c.set(4, 0x43300000); }
            } else {
                const int lower = compare(c, kind, 1);
                if (lower == 0) weighted(0x180, 0);
                else if (lower > 0) weighted(0x2c8, 1);
                else c.put16(at(0x58), 0);
            }
            if (compare(c, i16v(rating_a), c.i16(at(0x58))) < 0) {
                const double current = fp.fsub(wide(0xe8, x16(rating_a)), d(magic_at));
                const double part = fp.fsub(wide(0xf0, u(std::int32_t(c.i16(at(0x58))))), d(magic_at));
                const double mixed = fp.fmul(d(0x1c0), part);
                rating_a = trunc(0xf8, fp.multiply_add(0x101fb4a8, d(0x360), current, mixed), 1);
            }
        }
    }
    if (compare(c, i16v(rating_b), 0x2710) > 0) rating_b = 0x2710;
    if (compare(c, i16v(rating_b), i16v(floor_b)) < 0) rating_b = floor_b;
    if (compare(c, i16v(rating_a), 0x2710) > 0) rating_a = 0x2710;
    if (compare(c, i16v(rating_a), i16v(floor_a)) < 0) rating_a = floor_a;
    if (compare(c, i16v(rating_c), 0x2710) > 0) rating_c = 0x2710;
    if (compare(c, i16v(rating_c), i16v(floor_c)) < 0) rating_c = floor_c;
    c.set(3, x16(floor_c));  // extsh r3,r16 in the last comparison
    if (compare_logical(c, out, 0) != 0) {
        c.put16(out + 0x48, rating_a);
        c.put16(out + 0x4a, rating_b);
        c.put16(out + 0x4c, rating_c);
    } else {
        c.set(3, sheet());
        c.put16(sheet() + 0xa, rating_a);
        c.set(3, sheet());
        c.put16(sheet() + 8, rating_b);
        c.set(3, sheet());
        c.put16(sheet() + 0xc, rating_c);
    }
}
}
