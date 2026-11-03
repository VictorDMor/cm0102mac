#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerInfluence::profile_ratings() {
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        enter(c, 0x10229c04, {actor.address});
        current_role(c, actor.address);
        bool outfield = compare(c, c.r(3) & 0x78) != 0;
        if (!outfield) {
            enter(c, 0x10229c14, {actor.address});
            secondary_role(c, actor.address);
            outfield = compare(c, c.r(3) & 0x78) != 0;
        }
        if (outfield) {
            c.set_f(0, c.f64(doubles + 0x1b8));
            c.set_f(25, fp.frsp(fp.fmul(c.f(0), c.f(25))));
        }
    }
    c.set(3, std::int32_t(c.i16(actor.address + 0x3c)));
    c.put32(at(0x38), 0x43300000);
    c.set_f(1, c.f64(doubles + 0x80));
    c.put32(at(0x3c), c.r(3) ^ 0x80000000);
    c.set_f(3, c.f64(doubles + 0xc8));
    c.set_f(0, c.f64(at(0x38)));
    c.set_f(4, c.f64(doubles + 0x178));
    c.set_f(2, fp.fsub(c.f(0), c.f(1)));
    c.set_f(1, c.f64(doubles + 0xb8));
    c.set_f(0, c.f64(doubles + 0x88));
    c.set_f(2, fp.multiply_add(0x10229c58, c.f(3), c.f(25), c.f(2)));
    c.set_f(2, fp.frsp(fp.multiply_add(0x10229c5c, c.f(4), c.f(27), c.f(2))));
    c.set_f(1, fp.frsp(fp.fdiv(c.f(2), c.f(1))));
    c.put_f32(actor.address + 0xf4, float(c.f(1)));
    floor_attribute(0xf4);
    soften_attribute(0xf4, 0x1e8, 0x190, 0x10229cb0);

    // Both attributes share the fixture-type caps. The 0xc4 attribute uses
    // its own thresholds in type 32 and a different subtraction before pow.
    c.set(3, actor.match());
    c.set(4, c.u32(actor.address + 0x70));
    const auto kind = c.i8(c.r(3) + 1);
    c.set_f(25, c.f32(floats + 0x58));
    publish_fixture_rating(fixture_rating(c.i8(c.r(4) + 0x1b), kind, false), 0x74, 0x10229e38);
    base_profile_ratings();

    c.set(3, actor.match());
    c.set(4, c.u32(actor.address + 0x70));
    c.set(3, c.u8(c.r(3) + 1));
    const auto rating = c.i8(c.r(4) + 0x36);
    publish_fixture_rating(fixture_rating(rating, std::int8_t(c.r(3)), true), 0xc4, 0x1022a3a8);
}

std::int8_t PlayerInfluence::fixture_rating(std::int8_t rating, std::int8_t kind, bool secondary) {
    c.set(3, std::int32_t(kind));
    bool capped = compare(c, kind, 8) == 0;
    if (!capped)
        capped = kind < 8 ? compare(c, kind, 1) == 0 : compare(c, kind, 32) == 0;
    if (!capped) return rating;
    const auto profile_sum = [&] {
        c.set(4, c.u32(actor.address + 0x70));
        c.set(3, c.u32(actor.address + 0x70));
        c.set(4, c.u8(c.r(4) + 0x3d));
        c.set(3, c.u8(c.r(3) + 0x44));
        c.set(3, std::int32_t(std::int8_t(c.r(4) + c.r(3))));
        return std::int32_t(c.r(3));
    };
    const auto clamp = [&](std::int8_t limit) {
        c.set(3, std::int32_t(rating));
        return compare(c, rating, limit) > 0 ? limit : rating;
    };
    if (compare(c, profile_sum(), 40) == 0) {
        c.set(3, std::int32_t(rating));
        return compare(c, rating, 20) == 0 ? 19 : rating;
    }
    if (secondary && kind == 32) {
        if (compare(c, profile_sum(), 35) > 0) return clamp(18);
        if (compare(c, profile_sum(), 30) > 0) return clamp(17);
        if (compare(c, profile_sum(), 20) < 0) return clamp(16);
        if (compare(c, profile_sum(), 15) < 0) return clamp(15);
        return clamp(14);
    }
    if (compare(c, profile_sum(), 30) > 0) return clamp(18);
    if (compare(c, profile_sum(), 25) > 0) return clamp(17);
    if (compare(c, profile_sum(), 15) < 0) return clamp(16);
    return clamp(15);
}

void PlayerInfluence::publish_fixture_rating(std::int8_t rating, unsigned offset, std::uint32_t back) {
    c.set(3, std::uint32_t(std::int32_t(rating)) ^ 0x80000000);
    c.put32(at(0x38), 0x43300000);
    c.set_f(3, c.f64(doubles + 0x80));
    c.put32(at(0x3c), c.r(3));
    c.set_f(1, c.f64(doubles + 0x90));
    c.set_f(2, c.f64(at(0x38)));
    c.set_f(0, c.f64(doubles + 0x1f0));
    c.set_f(2, fp.fsub(c.f(2), c.f(3)));
    c.set_f(1, fp.fdiv(c.f(2), c.f(1)));
    c.set_f(1, fp.frsp(fp.fadd(c.f(25), c.f(1))));
    c.put_f32(actor.address + offset, float(c.f(1)));
    c.set_f(1, c.f32(actor.address + offset));
    fp.fcmp(0, c.f(1), c.f(0));
    if (!(c.cr(0) & 4)) return;
    c.set_f(1, c.f32(actor.address + offset));
    const bool secondary = offset == 0xc4;
    c.set_f(0, secondary ? c.f32(floats + 0x64) : c.f64(doubles + 0x1f0));
    c.set_f(2, c.f64(doubles + 0x168));
    c.set_f(1, secondary ? fp.fsubs(c.f(1), c.f(0)) : fp.fsub(c.f(1), c.f(0)));
    c.call(0x10780fc0, back);
    c.set(2, c.u32(at(0x14)));
    c.set_f(0, c.f64(doubles + 0x1f0));
    c.set_f(0, fp.frsp(fp.fadd(c.f(0), c.f(1))));
    c.put_f32(actor.address + offset, float(c.f(0)));
}

void PlayerInfluence::base_profile_ratings() {
    aged_profile_rating(0x1d, 1);
    c.set_f(0, c.f64(doubles + 0x28));
    c.set({{6, 0x43300000}, {5, 0x43300000}, {4, 0x43300000}});
    c.set_f(0, fp.multiply_add(0x10229f0c, c.f(1), c.f(0), c.f(25)));
    c.set(3, 0x43300000);
    c.put32(at(0x38), c.r(6));
    c.set_f(8, c.f64(doubles + 0x80));
    const auto age = std::int8_t(c.r(26));
    const bool veteran = compare(c, age, 36) > 0;
    c.set_f(0, fp.frsp(c.f(0)));
    c.set_f(5, c.f64(doubles + 0x28));
    c.put32(at(0x40), c.r(5));
    c.set_f(6, c.f64(doubles + 0x80));
    c.put_f32(actor.address + 0x7c, float(c.f(0)));
    c.set_f(3, c.f64(doubles + 0x28));
    c.set(5, c.u32(actor.address + 0x70));
    c.put32(at(0x48), c.r(4));
    const auto first = c.i8(c.r(5) + 0x1c);
    c.set_f(4, c.f64(doubles + 0x80));
    c.set_f(1, c.f64(doubles + 0x28));
    c.put32(at(0x50), c.r(3));
    c.set_f(2, c.f64(doubles + 0x80));
    c.put32(at(0x3c), std::uint32_t(std::int32_t(first)) ^ 0x80000000);
    c.set_f(0, c.f64(doubles + 0x28));
    c.set_f(7, c.f64(at(0x38)));
    c.set_f(7, fp.fsub(c.f(7), c.f(8)));
    c.set_f(5, fp.frsp(fp.fmul(c.f(7), c.f(5))));
    c.put_f32(actor.address + 0x78, float(c.f(5)));

    struct Rating { unsigned profile, player, staging, input, bias, output; std::uint32_t site; };
    constexpr Rating ratings[]{
        {0x1f, 0x84, 0x40, 5, 6, 3, 0x10229f98},
        {0x20, 0x88, 0x48, 3, 4, 1, 0x10229fc0},
        {0x22, 0x8c, 0x50, 1, 2, 0, 0x10229fe8},
    };
    for (const auto& row : ratings) {
        c.set(3, c.u32(actor.address + 0x70));
        c.put32(at(row.staging + 4), std::uint32_t(std::int32_t(c.i8(c.r(3) + row.profile))) ^ 0x80000000);
        c.set_f(row.input, c.f64(at(row.staging)));
        c.set_f(row.input, fp.fsub(c.f(row.input), c.f(row.bias)));
        c.set_f(row.output, fp.frsp(fp.multiply_add(row.site, c.f(row.input), c.f(row.output), c.f(25))));
        c.put_f32(actor.address + row.player, float(c.f(row.output)));
    }

    unsigned weight = 0;
    if (veteran) weight = 0x28;
    else if (compare(c, age, 34) > 0) weight = 0x30;
    c.set(3, c.u32(actor.address + 0x70));
    c.put32(at(0x38), 0x43300000);
    const auto trait = c.i8(c.r(3) + 0x28);
    c.set_f(1, c.f64(doubles + 0x80));
    if (weight) c.set_f(2, c.f64(doubles + weight));
    c.put32(at(0x3c), std::uint32_t(std::int32_t(trait)) ^ 0x80000000);
    c.set_f(0, c.f64(at(0x38)));
    if (weight) {
        c.set_f(0, fp.fsub(c.f(0), c.f(1)));
        c.set_f(5, fp.frsp(fp.fmul(c.f(2), c.f(0))));
    } else c.set_f(5, fp.fsubs(c.f(0), c.f(1)));

    c.set_f(1, c.f32(floats + 0x5c));
    c.set(3, 0x43300000);
    c.set_f(0, c.f32(floats + 0x60));
    c.set_f(3, fp.fdivs(c.f(27), c.f(1)));
    c.put32(at(0x38), c.r(3));
    c.set_f(4, c.f64(doubles + 0x80));
    c.set_f(1, c.f64(doubles + 0x118));
    c.put32(at(0x40), 0x43300000);
    c.set_f(2, c.f64(doubles + 0x80));
    c.set_f(3, fp.fsubs(c.f(5), c.f(3)));
    c.set_f(0, fp.fdivs(c.f(27), c.f(0)));
    c.put_f32(actor.address + 0xa0, float(c.f(3)));
    c.set(3, c.u32(actor.address + 0x70));
    c.put32(at(0x3c), std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x29))) ^ 0x80000000);
    c.set_f(3, c.f64(at(0x38)));
    c.set_f(3, fp.fsub(c.f(3), c.f(4)));
    c.set_f(1, fp.fdiv(c.f(3), c.f(1)));
    c.set_f(1, fp.frsp(fp.fadd(c.f(25), c.f(1))));
    c.put_f32(actor.address + 0xa4, float(c.f(1)));
    c.set(3, c.u32(actor.address + 0x70));
    c.put32(at(0x44), std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x2e))) ^ 0x80000000);
    c.set_f(1, c.f64(at(0x40)));
    c.set_f(1, fp.fsubs(c.f(1), c.f(2)));
    c.set_f(0, fp.fsubs(c.f(1), c.f(0)));
    c.put_f32(actor.address + 0xb0, float(c.f(0)));
}
}
