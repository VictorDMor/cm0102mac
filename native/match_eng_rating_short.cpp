// MATCH_ENG.CPP, rating: the same ratings from a record's person (0x101fb580) and a side's tendency (0x101fc670).
#include "match_eng_rating.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace rating_detail;

namespace {
constexpr std::uint32_t magic_at = 0x60;  // pool 2 +0x60: 2^52+2^31
constexpr std::uint16_t cup_codes[] = {0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
                                       0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b};
}

// 0x101fb580
void compute_record_ratings(Cpu& c, std::uint32_t record_, std::uint32_t club_, std::uint32_t opponent, std::uint32_t fixture_,
                            double weight, std::uint32_t flag_a, std::uint32_t flag_b) {
    // Quirk: r5 (the opponent) is parked above the frame, in the caller's area; r7 is not used.
    Frame frame(c, 0x101fb580, 0x170, {.first_gpr = 16, .gpr_offset = -0xa0, .first_fpr = 21});
    const auto at = [&](std::uint32_t offset) { return frame.at(std::int32_t(offset)); };
    Kept<18> record(c, record_);
    Kept<20> club(c, club_);
    c.put32(at(0x190), opponent);
    Kept<30> fixture(c, fixture_);
    c.set_f(21, weight);
    c.put8(at(0x19f), flag_a);
    c.put8(at(0x1a3), flag_b);
    const auto pool1 = c.toc(-0x52c8), pool2 = c.toc(-0x52c4);
    keep(c, {{24, pool1}, {25, pool2}});
    Kept<16> divisor_c(c, 2);  // the divisor of rating_c's limit, 1 for a national fixture
    c.set(3, 2);
    c.put16(at(0x62), 2);  // and of rating_b's
    Fpu fp(c);
    const auto wide = [&](std::uint32_t slot, std::uint32_t value) { return widen(c, at(slot), value); };
    const auto trunc = [&](std::uint32_t slot, double value) {
        c.put64(at(slot), fp.fctiwz(0, value));
        return c.u32(at(slot) + 4);
    };
    const auto d = [&](std::uint32_t offset) { return c.f64(pool2 + offset); };
    const auto s = [&](std::uint32_t offset) { return double(c.f32(pool1 + offset)); };
    const auto lt = [&](double a, double b) { fp.fcmp(0, a, b); return (c.cr(0) & 8) != 0; };
    const auto gt = [&](double a, double b) { fp.fcmp(0, a, b); return (c.cr(0) & 4) != 0; };
    const auto i16v = [](std::uint32_t value) { return std::int32_t(std::int16_t(value)); };
    const auto quotient = [](std::uint32_t numerator, std::uint32_t denominator) {
        return std::uint32_t(divide_signed<4>(numerator, denominator));
    };
    c.set(3, record);
    match_eng::call(c, 0x101da750, 0x101fb5d0);
    Kept<31> person(c, c.r(3));
    if (compare_logical(c, person, 0) == 0 || compare_logical(c, c.u32(person + 0x1c), 0) == 0) return;
    const auto sheet = [&] { return c.u32(person + 0x1c); };
    const auto strength = [&] { return u(std::int32_t(c.i16(c.u32(at(0x190)) + 0x1ec))); };
    const auto tier = [&] { return std::int32_t(c.i16(c.u32(fixture + 0x14) + 0x18)); };
    const auto check_club = [&](std::uint32_t back) {
        c.set(3, club);
        c.call(0x100c7d00, back);
        return compare(c, s8(c.r(3)), 0) != 0;
    };

    Kept<29> level(c, c.r(29));
    if (compare(c, c.i32(fixture), -2) == 0) {
        level = 5;
    } else if (compare(c, s8(c.u8(fixture + 0x40)), 0) != 0) {
        const auto sum = u(tier()) + u(divide_by_magic(i16v(strength()), 5));
        level = x8(u(std::int32_t(sum + (sum >> 31)) >> 1));
    } else {
        level = x8(u(tier()));
    }
    if (compare(c, s8(level), 0x14) > 0) level = 0x14;
    // The ceiling of a sheet value: its thousands plus 16, at least 22 (or 21 for the last one).
    c.put8(at(0x38), x8(u(divide_by_magic(c.i16(sheet() + 0x2a), 6)) + 0x10));
    if (compare(c, s8(c.u8(at(0x38))), 0x16) < 0) c.put8(at(0x38), 0x16);
    c.put16(at(0x64), x16(u(s8(c.u8(at(0x38)))) - u(s8(level))));
    Kept<21> band(c, c.r(21));
    Kept<17> span_b(c, c.r(17));
    Kept<19> span_c(c, c.r(19));
    Kept<23> band_c(c, c.r(23));
    bool own = check_club(0x101fb6d0);
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
    const auto ceiling_b = [&] {
        band = x8(u(divide_by_magic(c.i16(sheet() + 0x28), 6)) + 0x10);
        if (compare(c, s8(band), 0x16) < 0) band = 0x16;
    };
    if (own) {
        ceiling_b();
        span_b = x16(u(s8(band)) - u(s8(level)));
        c.put16(at(0x62), 1);
    } else {
        ceiling_b();
        c.put16(at(0x60), x16(0xca - u(s8(level)) * 10));
        c.put16(at(0x5e), x16(u(s8(band)) - u(s8(level))));
        const auto sum = u(std::int32_t(c.i16(at(0x60)))) + u(std::int32_t(c.i16(at(0x5e))));
        span_b = x16(u(std::int32_t(sum + (sum >> 31)) >> 1));
        if (compare(c, i16v(span_b), 4) < 0) span_b = 4;
    }
    const auto band_of_c = [&] {
        band_c = x8(u(divide_by_magic(c.i16(sheet() + 0x2c), 6)) + 0x10);
        if (compare(c, s8(band_c), 0x15) < 0) band_c = 0x15;
    };
    if (check_club(0x101fb810)) {
        band_of_c();
        span_c = x16(u(s8(band_c)) - u(s8(level)));
        divisor_c = 1;
    } else if (compare(c, s8(c.u8(fixture + 0x3a)), 8) == 0) {
        divisor_c = 1;
        bool final_round = false;
        for (std::int32_t code = 0x41f; code <= 0x426 && !final_round; ++code) final_round = compare(c, std::int32_t(c.i16(fixture + 0x32)), code) == 0;
        if (final_round) {
            band_of_c();
            span_c = x16(u(s8(band_c)) - u(s8(level)) + 1);
        } else {
            bool cup = false;
            for (const auto code : cup_codes) {
                if (cup) break;
                cup = compare(c, std::int32_t(c.i16(fixture + 0x30)), code) == 0;
            }
            if (cup) {
                band_of_c();
                level = x8(u(tier()));
                span_c = x16(u(s8(band_c)) - u(s8(level)));
            } else {
                span_c = x16(0xca - u(s8(level)) * 10);
            }
        }
    } else {
        span_c = x16(0xca - u(s8(level)) * 10);
    }

    // The league rating against the weight, as in 0x101f9470.
    const auto league_record = [&] { return c.u32(c.toc(-0x7b3c)); };
    const auto rate = [&](std::uint32_t back) {
        const auto holder = league_record();
        enter(c, back, {holder, c.u8(at(0x19f))});
        return side_rate(c, holder, c.u8(at(0x19f)));
    };
    c.set_f(31, fp.frsp(fp.fsub(c.f(21), rate(0x101fba78))));
    c.set_f(31, fp.fmuls(c.f(31), s(0x128)));
    // One rating from the level and a sheet value, as 0x101f9470's `component`.
    const auto component = [&](std::uint32_t level_slot, std::uint32_t value_slot, std::uint32_t store_slot, std::uint32_t field,
                               std::uint32_t site_a, std::uint32_t site_b) {
        const double scale = fp.fmul(d(0x130), c.f(31));
        const double level_value = fp.fsubs(wide(level_slot, x8(level)), d(magic_at));
        const double slope = fp.frsp(fp.multiply_add(site_a, level_value, d(0x90), d(0x30)));
        const double value = fp.fsubs(wide(value_slot, u(std::int32_t(c.i16(sheet() + field)))), d(magic_at));
        return trunc(store_slot, fp.multiply_add(site_b, scale, slope, value));
    };
    Kept<27> rating_b(c, component(0xc0, 0xb8, 0xb0, 0x28, 0x101fbab8, 0x101fbae4));
    Kept<26> rating_a(c, component(0xa8, 0xa0, 0x98, 0x2a, 0x101fbb24, 0x101fbb50));
    Kept<28> rating_c(c, component(0x90, 0x88, 0x80, 0x2c, 0x101fbb90, 0x101fbbbc));
    if (compare(c, s8(c.u8(at(0x1a3))), 0) != 0) {
        Kept<22> lift(c, c.r(22));
        const double league = rate(0x101fbbec);
        const double scaled = fp.fdiv(fp.fmuls(c.f(21), fp.fsubs(wide(0x80, u(s8(level)) * 0xfa), d(magic_at))), league);
        lift = trunc(0x88, scaled);
        if (lt(c.f(31), s(0xc))) {
            const double again = rate(0x101fbc40);
            const double value = fp.fmuls(fp.fsubs(wide(0x80, x16(lift)), d(magic_at)), c.f(21));
            lift = trunc(0x88, fp.fdiv(value, again));
        }
        if (compare(c, i16v(lift), i16v(rating_a)) > 0) c.put16(at(0x56), lift);
        else c.put16(at(0x56), rating_a);
        rating_a = u(std::int32_t(c.i16(at(0x56))));
        if (compare(c, i16v(lift), i16v(rating_b)) > 0) c.put16(at(0x54), lift);
        else c.put16(at(0x54), rating_b);
        rating_b = u(std::int32_t(c.i16(at(0x54))));
        if (compare(c, i16v(lift), i16v(rating_c)) > 0) c.put16(at(0x52), lift);
        else c.put16(at(0x52), rating_c);
        rating_c = u(std::int32_t(c.i16(at(0x52))));
    }
    // Each rating is averaged with the room left under its ceiling.
    rating_a = x16(quotient(u(i16v(rating_a)) + u(c.i16(at(0x64)) - 1) * u(c.i16(sheet() + 0x2a)), u(c.i16(at(0x64)))));
    rating_b = x16(quotient(u(i16v(rating_b)) + u(i16v(span_b) - 1) * u(c.i16(sheet() + 0x28)), u(i16v(span_b))));
    rating_c = x16(quotient(u(i16v(rating_c)) + u(i16v(span_c) - 1) * u(c.i16(sheet() + 0x2c)), u(i16v(span_c))));
    const auto shrunk = [&](std::uint32_t slot, std::uint32_t field) {
        const double ratio = fp.fdiv(fp.fsubs(wide(slot, u(std::int32_t(c.i16(sheet() + field)))), d(magic_at)), d(0x38));
        return fp.frsp(fp.fsub(d(0x98), ratio));
    };
    c.set_f(30, shrunk(0x80, 0x2a));
    c.set_f(29, shrunk(0x88, 0x28));
    c.set_f(28, shrunk(0x90, 0x2c));

    const auto sheet_value = [&](std::uint32_t slot, std::uint32_t field) {
        return fp.fsubs(wide(slot, u(std::int32_t(c.i16(sheet() + field)))), d(magic_at));
    };
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
        rating = u(std::int32_t(c.i16(at(keep_at))));
    };
    // Within a few spreads of the sheet value, in double precision here.
    reach(rating_a, 0x2a, 0x50, 0x4c, 0x4e);
    {
        const double lower = fp.multiply_add(0x101fbed8, s(0xfc), c.f(30), sheet_value(0x80, 0x2a));
        if (gt(lower, fp.fsubs(wide(0x88, x16(rating_a)), d(magic_at)))) {
            c.set_f(27, fp.multiply_add(0x101fbf2c, s(0xfc), c.f(30), sheet_value(0x80, 0x2a)));
        } else {
            const double upper = fp.multiply_add(0x101fbf5c, s(0x120), c.f(30), sheet_value(0x80, 0x2a));
            if (lt(upper, fp.fsubs(wide(0x88, x16(rating_a)), d(magic_at)))) {
                c.set_f(26, fp.multiply_add(0x101fbfb0, s(0x120), c.f(30), sheet_value(0x80, 0x2a)));
            } else {
                c.set_f(26, fp.fsubs(wide(0x80, x16(rating_a)), d(magic_at)));
            }
            c.set_f(27, c.f(26));
        }
        rating_a = trunc(0x80, c.f(27));
    }
    reach(rating_b, 0x28, 0x4a, 0x46, 0x48);
    {
        const double lower = fp.multiply_add(0x101fc0c0, s(0xfc), c.f(29), sheet_value(0x80, 0x28));
        if (gt(lower, fp.fsubs(wide(0x88, x16(rating_b)), d(magic_at)))) {
            c.set_f(25, fp.multiply_add(0x101fc114, s(0xfc), c.f(29), sheet_value(0x80, 0x28)));
        } else {
            const double upper = fp.multiply_add(0x101fc144, s(0xf8), c.f(29), sheet_value(0x80, 0x28));
            if (lt(upper, fp.fsubs(wide(0x88, x16(rating_b)), d(magic_at)))) {
                c.set_f(24, fp.multiply_add(0x101fc198, s(0xf8), c.f(29), sheet_value(0x80, 0x28)));
            } else {
                c.set_f(24, fp.fsubs(wide(0x80, x16(rating_b)), d(magic_at)));
            }
            c.set_f(25, c.f(24));
        }
        rating_b = trunc(0x80, c.f(25));
    }
    reach(rating_c, 0x2c, 0x44, 0x40, 0x42);
    {
        const double lower = fp.multiply_add(0x101fc2a8, s(0xf0), c.f(28), sheet_value(0x80, 0x2c));
        if (gt(lower, fp.fsubs(wide(0x88, x16(rating_c)), d(magic_at)))) {
            c.set_f(23, fp.multiply_add(0x101fc2fc, s(0xf0), c.f(28), sheet_value(0x80, 0x2c)));
        } else {
            const double hundreds = fp.fsubs(wide(0x80, u(s8(level)) * 0x64), d(magic_at));
            const double upper = fp.multiply_add(0x101fc34c, c.f(28), hundreds, sheet_value(0x88, 0x2c));
            if (lt(upper, fp.fsubs(wide(0x90, x16(rating_c)), d(magic_at)))) {
                const double again = fp.fsubs(wide(0x80, u(s8(level)) * 0x64), d(magic_at));
                c.set_f(22, fp.multiply_add(0x101fc3c0, c.f(28), again, sheet_value(0x88, 0x2c)));
            } else {
                c.set_f(22, fp.fsubs(wide(0x80, x16(rating_c)), d(magic_at)));
            }
            c.set_f(23, c.f(22));
        }
        rating_c = trunc(0x80, c.f(23));
    }

    // The most each rating may reach: 400 a level (divided as given), between the floor and 10000.
    const auto limit = [&](bool divided, std::uint32_t divisor, std::int32_t floor_value, std::uint32_t outer, std::uint32_t inner) {
        const auto base = u(s8(level)) * 0x190;
        const auto scaled = [&] { return divided ? quotient(base, divisor) : base; };
        if (compare(c, std::int32_t(scaled()), floor_value) < 0) {
            c.put32(at(outer), u(floor_value));
        } else {
            const auto again = scaled();
            if (compare(c, std::int32_t(again), 0x2710) > 0) c.put32(at(inner), 0x2710);
            else c.put32(at(inner), again);
            c.put32(at(outer), c.u32(at(inner)));
        }
    };
    // The rating then goes no higher than that limit, taken at least as high as the sheet value.
    const auto settle = [&](auto& rating, std::uint32_t limit_slot, std::uint32_t outer, std::uint32_t field, std::uint32_t temp) {
        c.put16(at(limit_slot), c.u32(at(outer)));
        c.set({{4, u(std::int32_t(c.i16(at(limit_slot))))}, {5, u(std::int32_t(c.i16(sheet() + field)))}});
        if (compare(c, c.i16(at(limit_slot)), c.i16(sheet() + field)) > 0) {
            c.set(3, u(std::int32_t(c.i16(at(limit_slot)))));
            c.put16(at(temp), c.u16(at(limit_slot)));
        } else {
            c.set(4, u(std::int32_t(c.i16(sheet() + field))));
            c.put16(at(temp), c.u16(sheet() + field));
        }
        c.put16(at(limit_slot), c.u16(at(temp)));
        if (compare(c, i16v(rating), c.i16(at(limit_slot))) > 0) rating = u(std::int32_t(c.i16(at(limit_slot))));
        c.set(3, x16(rating));
        if (compare(c, i16v(rating), 1) < 0) rating = 1;
    };
    limit(true, u(std::int32_t(c.i16(at(0x62)))), 0x1388, 0x7c, 0x78);
    settle(rating_b, 0x5c, 0x7c, 0x28, 0x3e);
    limit(false, 0, 0x1388, 0x74, 0x70);
    settle(rating_a, 0x5a, 0x74, 0x2a, 0x3c);
    limit(true, x16(divisor_c), 0xbb8, 0x6c, 0x68);
    settle(rating_c, 0x58, 0x6c, 0x2c, 0x3a);
    c.set_f(1, d(magic_at));  // quirk: f1 leaves holding the pool's 2^52+2^31, the last widening's constant
    c.put16(record, rating_a);
    c.put16(record + 2, rating_b);
    c.put16(record + 4, rating_c);
}

// 0x101fc670
void compute_side_tendency(Cpu& c, std::uint32_t match_, std::uint32_t side, double weight, std::uint32_t rate_flag_,
                           std::uint32_t flag) {
    Frame frame(c, 0x101fc670, 0xc0, {.first_gpr = 16, .gpr_offset = -0x70, .first_fpr = 27});
    const auto at = [&](std::uint32_t offset) { return frame.at(std::int32_t(offset)); };
    Kept<23> match(c, match_);
    c.put8(at(0xdf), side);
    c.set_f(27, weight);
    Kept<16> rate_flag(c, rate_flag_);
    c.put8(at(0xeb), flag);
    Kept<22> league_slot(c, c.toc(-0x7b3c));
    Kept<25> pool2(c, c.toc(-0x52c4));
    Fpu fp(c);
    const auto wide = [&](std::uint32_t slot, std::uint32_t value) { return widen(c, at(slot), value); };
    const auto trunc = [&](std::uint32_t slot, double value) {
        c.put64(at(slot), fp.fctiwz(0, value));
        return c.u32(at(slot) + 4);
    };
    const auto d = [&](std::uint32_t offset) { return c.f64(pool2 + offset); };
    const auto lt = [&](double a, double b) { fp.fcmp(0, a, b); return (c.cr(0) & 8) != 0; };
    const auto gt = [&](double a, double b) { fp.fcmp(0, a, b); return (c.cr(0) & 4) != 0; };
    const auto i16v = [](std::uint32_t value) { return std::int32_t(std::int16_t(value)); };
    const auto quotient = [](std::uint32_t numerator, std::uint32_t denominator) {
        return std::uint32_t(divide_signed<4>(numerator, denominator));
    };
    c.set(3, match);
    call(c, 0x101f9410, 0x101fc6ac);
    Kept<31> fixture(c, c.r(3));
    c.set({{3, match}, {4, c.u8(at(0xdf))}});
    call(c, 0x101d3c50, 0x101fc6bc);
    Kept<30> team(c, c.r(3));
    c.set({{3, match}, {4, x8(c.u8(at(0xdf)) == 0 ? 1 : 0)}});  // extsb, cntlzw: 1 when the side is 0
    call(c, 0x101d3c50, 0x101fc6dc);
    Kept<21> other(c, c.r(3));

    const auto tier = [&] { return std::int32_t(c.i16(c.u32(fixture + 0x14) + 0x18)); };
    Kept<29> level(c, c.r(29));
    if (compare(c, c.i32(fixture), -2) == 0) {
        level = 5;
    } else if (compare(c, s8(c.u8(fixture + 0x40)), 0) != 0) {
        const auto sum = u(tier()) + u(divide_by_magic(c.i16(other + 0x1ec), 5));
        level = x8(u(std::int32_t(sum + (sum >> 31)) >> 1));
    } else {
        level = x8(u(tier()));
    }
    if (compare(c, s8(level), 0x14) > 0) level = 0x14;
    Kept<27> band(c, c.r(27));
    Kept<26> span(c, c.r(26));
    const auto band_of_team = [&] {
        band = x8(u(divide_by_magic(c.i16(team + 0x1ec), 6)) + 0x10);
        if (compare(c, s8(band), 0x15) < 0) band = 0x15;
    };
    c.set(3, team);
    c.call(0x100c7d00, 0x101fc764);
    if (compare(c, s8(c.r(3)), 0) != 0) {
        band_of_team();
        span = x16(u(s8(band)) - u(s8(level)));
    } else if (compare(c, s8(c.u8(fixture + 0x3a)), 8) == 0) {
        bool final_round = false;
        for (std::int32_t code = 0x41f; code <= 0x426 && !final_round; ++code) final_round = compare(c, std::int32_t(c.i16(fixture + 0x32)), code) == 0;
        if (final_round) {
            band_of_team();
            span = x16(u(s8(band)) - u(s8(level)) + 1);
        } else {
            bool cup = false;
            for (const auto code : cup_codes) {
                if (cup) break;
                cup = compare(c, std::int32_t(c.i16(fixture + 0x30)), code) == 0;
            }
            if (cup) {
                band_of_team();
                level = x8(u(tier()));
                span = x16(u(s8(band)) - u(s8(level)));
            } else {
                span = x16(0xca - u(s8(level)) * 10);
            }
        }
    } else {
        span = x16(0xca - u(s8(level)) * 10);
    }

    const auto rate = [&](std::uint32_t back) {
        const auto record = c.u32(league_slot);
        enter(c, back, {record, rate_flag});
        return side_rate(c, record, rate_flag);
    };
    const double league = rate(0x101fc9b4);
    const double gap = fp.frsp(fp.fsub(c.f(27), league));
    c.set_f(31, gap);
    const double lower_limit = double(c.f32(c.toc(-0x52c8) + 0xc));
    if (lt(c.f(31), lower_limit)) {
        c.set_f(1, c.f(31));
        enter(c, 0x101fc9d4);
        const double magnitude = absolute(c, c.f(31));
        c.set_f(31, fp.fmuls(c.f(31), fp.frsp(magnitude)));
    }
    const auto strength = [&] { return u(std::int32_t(c.i16(team + 0x1ec))); };
    {
        const double scale = fp.fmul(d(0x130), c.f(31));
        const double level_value = fp.fsubs(wide(0x48, x8(level)), d(0x60));
        const double slope = fp.frsp(fp.multiply_add(0x101fca0c, level_value, d(0x90), d(0x30)));
        const double value = fp.fsubs(wide(0x40, strength()), d(0x60));
        Kept<28> tendency(c, trunc(0x38, fp.multiply_add(0x101fca34, scale, slope, value)));
        if (compare(c, s8(c.u8(at(0xeb))), 0) != 0) {
            Kept<24> floor_value(c, c.r(24));
            const double first = rate(0x101fca60);
            const double scaled = fp.fdiv(fp.fmuls(c.f(27), fp.fsubs(wide(0x38, u(s8(level)) * 0xfa), d(0x60))), first);
            floor_value = trunc(0x40, scaled);
            if (lt(c.f(31), double(c.f32(c.toc(-0x52c8) + 0xc)))) {
                const double again = rate(0x101fcab4);
                const double part = fp.fmuls(fp.fsubs(wide(0x38, x16(floor_value)), d(0x60)), c.f(27));
                floor_value = trunc(0x40, fp.fdiv(part, again));
            }
            Kept<20> larger(c, c.r(20));
            if (compare(c, i16v(floor_value), i16v(tendency)) > 0) larger = floor_value;
            else larger = tendency;
            tendency = x16(larger);
        }
        c.set(4, x16(tendency));  // the last r4 the routine leaves
        tendency = x16(quotient(u(i16v(tendency)) + u(i16v(span) - 1) * strength(), u(i16v(span))));
        const double ratio = fp.fdiv(fp.fsubs(wide(0x38, strength()), d(0x60)), d(0x38));
        c.set_f(30, fp.frsp(fp.fsub(d(0x98), ratio)));
        Kept<19> widest(c, c.r(19));
        const auto bound = [&] { return x16(u(s8(level)) * 0x12c + 0xfa0); };
        if (compare(c, i16v(bound()), c.i16(team + 0x1ec)) > 0) widest = x16(bound());
        else widest = u(std::int32_t(c.i16(team + 0x1ec)));
        Kept<18> kept(c, c.r(18));
        if (compare(c, i16v(tendency), i16v(widest)) < 0) {
            kept = tendency;
        } else {
            Kept<17> pick(c, c.r(17));
            if (compare(c, i16v(bound()), c.i16(team + 0x1ec)) > 0) pick = x16(bound());
            else pick = u(std::int32_t(c.i16(team + 0x1ec)));
            kept = pick;
        }
        tendency = x16(kept);
        const double fives = fp.fsubs(wide(0x38, u(s8(level)) * 5), d(0x60));
        const double reach = fp.fsubs(wide(0x40, strength()), d(0x60));
        const double lower = fp.multiply_add(0x101fcc2c, c.f(30), fives, reach);
        const double current = fp.fsubs(wide(0x48, x16(tendency)), d(0x60));
        if (gt(lower, current)) {
            const double times = fp.fsubs(wide(0x38, u(s8(level)) * 5), d(0x60));
            const double again = fp.fsubs(wide(0x40, strength()), d(0x60));
            c.set_f(29, fp.multiply_add(0x101fcc9c, c.f(30), times, again));
        } else {
            const double times = fp.fsubs(wide(0x38, u(s8(level)) * 5), d(0x60));
            const double base = fp.fsubs(wide(0x40, strength()), d(0x60));
            const double upper = fp.multiply_add(0x101fcce8, c.f(30), times, base);
            if (lt(upper, fp.fsubs(wide(0x48, x16(tendency)), d(0x60)))) {
                const double times_again = fp.fsubs(wide(0x38, u(s8(level)) * 5), d(0x60));
                const double base_again = fp.fsubs(wide(0x40, strength()), d(0x60));
                c.set_f(28, fp.multiply_add(0x101fcd58, c.f(30), times_again, base_again));
            } else {
                c.set_f(28, fp.fsubs(wide(0x38, x16(tendency)), d(0x60)));
            }
            c.set_f(29, c.f(28));
        }
        tendency = trunc(0x38, c.f(29));
        if (compare(c, i16v(tendency), 0x2710) > 0) tendency = 0x2710;
        if (compare(c, i16v(tendency), 1) < 0) tendency = 1;
        c.set_f(1, d(0x60));  // the last value the clamp left in f1
        c.set(3, u(s8(c.u8(at(0xdf)))) << 1);
        c.put16(match + ((u(s8(c.u8(at(0xdf)))) << 1) + 0x1f0), tendency);
    }
}
}
