#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerInfluence::age_and_fixture_ratings() {
    c.set(4, c.u32(actor.address + 0x70));
    c.set(3, 0x43300000);
    const auto age = std::int8_t(c.r(26));
    c.put32(at(0x38), c.r(3));
    c.set(3, c.u8(c.r(4) + 0x3d));
    const bool oldest = compare(c, age, 38) > 0;
    c.set_f(2, c.f64(doubles + 0x80));
    c.set_f(0, c.f64(doubles + 0x28));
    c.put32(at(0x3c), std::uint32_t(std::int32_t(std::int8_t(c.r(3)))) ^ 0x80000000);
    c.set_f(1, c.f64(at(0x38)));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(0, fp.frsp(fp.multiply_add(0x1022a3f0, c.f(1), c.f(0), c.f(25))));
    c.put_f32(actor.address + 0xdc, float(c.f(0)));

    unsigned age_offset;
    std::uint32_t age_site;
    if (oldest) { age_offset = 0x1f8; age_site = 0x1022a430; }
    else if (compare(c, age, 35) > 0) { age_offset = 0x208; age_site = 0x1022a47c; }
    else if (compare(c, age, 33) > 0) { age_offset = 0x210; age_site = 0x1022a4c8; }
    else if (compare(c, age, 20) < 0) { age_offset = 0x218; age_site = 0x1022a514; }
    else if (compare(c, age, 24) < 0) { age_offset = 0x220; age_site = 0x1022a560; }
    else if (compare(c, age, 30) < 0) { age_offset = 0x228; age_site = 0x1022a5ac; }
    else { age_offset = 0x70; age_site = 0x1022a5ec; }

    c.set(3, c.u32(actor.address + 0x70));
    c.put32(at(0x38), 0x43300000);
    const auto ability = c.i8(c.r(3) + 0x3c);
    c.set_f(2, c.f64(doubles + 0x80));
    c.set_f(3, c.f64(doubles + 0x200));
    c.set_f(0, c.f64(doubles + age_offset));
    c.put32(at(0x3c), std::uint32_t(std::int32_t(ability)) ^ 0x80000000);
    c.set_f(1, c.f64(at(0x38)));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(0, fp.frsp(fp.multiply_add(age_site, c.f(3), c.f(1), c.f(0))));
    c.put_f32(actor.address + 0xd8, float(c.f(0)));

    c.set(3, actor.match());
    const auto kind = c.i8(c.r(3) + 1);
    unsigned fixture_penalty = 0;
    if (compare(c, kind, 8) == 0) fixture_penalty = 0x30;
    else if (kind < 8) { if (compare(c, kind, 1) == 0) fixture_penalty = 0x28; }
    else if (compare(c, kind, 32) == 0) fixture_penalty = 0x110;
    if (fixture_penalty) {
        c.set_f(1, c.f32(actor.address + 0xd8));
        c.set_f(0, c.f64(doubles + fixture_penalty));
        c.set_f(0, fp.frsp(fp.fsub(c.f(1), c.f(0))));
        c.put_f32(actor.address + 0xd8, float(c.f(0)));
    }

    c.set(3, actor.match());
    const bool young = compare(c, age, 32) < 0;
    const auto fixture = c.u32(c.r(3) + 0x3118);
    keep(c, {{20, fixture}});
    if (young && compare_logical(c, fixture) != 0 && compare_logical(c, c.u32(fixture + 0x1c)) != 0) {
        c.set(3, c.u32(fixture + 0x1c));
        if (compare_logical(c, c.u32(c.r(3) + 0x30)) != 0) {
            c.set(3, c.u32(fixture + 0x1c));
            c.set(4, c.u32(fixture + 0x14));
            if (compare_logical(c, c.r(4), c.u32(c.r(3) + 0x30)) == 0) {
                const auto read_rank = [&] {
                    c.set(3, c.u32(fixture + 0x1c));
                    c.set(3, c.u32(c.r(3) + 0x30));
                    return c.i16(c.r(3) + 0x18);
                };
                unsigned bonus = 0;
                if (compare(c, read_rank(), 5) < 0) bonus = 0x168;
                else if (compare(c, read_rank(), 9) < 0) bonus = 0x10;
                if (bonus) {
                    c.set_f(1, c.f64(doubles + bonus));
                    c.set_f(0, c.f32(actor.address + 0xd8));
                    c.set_f(0, fp.frsp(fp.fadd(c.f(1), c.f(0))));
                    c.put_f32(actor.address + 0xd8, float(c.f(0)));
                }
            }
        }
    }

    c.set(3, c.u32(actor.address + 0x70));
    c.put32(at(0x38), 0x43300000);
    const auto trait = c.i8(c.r(3) + 0x3f);
    c.set_f(1, c.f64(doubles + 0x80));
    c.put32(at(0x3c), std::uint32_t(std::int32_t(trait)) ^ 0x80000000);
    c.set_f(0, c.f64(at(0x38)));
    c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
    c.put_f32(actor.address + 0xe4, float(c.f(0)));

    c.set(3, actor.match());
    c.set(4, c.u32(actor.address + 0x70));
    const auto live_kind = c.i8(c.r(3) + 1);
    auto rating = c.i8(c.r(4) + 0x40);
    c.set(3, std::int32_t(live_kind));
    if (compare(c, live_kind, 8) == 0) {
        c.set(3, std::int32_t(rating));
        if (compare(c, rating, 17) > 0) rating = 17;
    } else if (live_kind < 8) {
        if (compare(c, live_kind, 1) == 0) {
            c.set(3, std::int32_t(rating));
            if (compare(c, rating, 20) == 0) rating = 19;
            else if (compare(c, rating, 18) > 0) rating = 18;
        }
    } else if (compare(c, live_kind, 32) == 0) {
        c.set(3, std::int32_t(rating));
        if (compare(c, rating, 15) > 0) rating = 15;
    }

    c.set(4, 0x43300000);
    c.put32(at(0x3c), std::uint32_t(std::int32_t(rating)) ^ 0x80000000);
    c.set(3, actor.address);
    c.set_f(2, c.f64(doubles + 0x80));
    c.put32(at(0x38), c.r(4));
    c.set_f(1, c.f64(doubles + 0x80));
    c.set_f(0, c.f64(at(0x38)));
    c.put32(at(0x40), 0x43300000);
    c.set_f(0, fp.fsubs(c.f(0), c.f(2)));
    c.put_f32(actor.address + 0xe8, float(c.f(0)));
    c.set(4, c.u32(actor.address + 0x70));
    c.put32(at(0x44), std::uint32_t(std::int32_t(c.i8(c.r(4) + 0x42))) ^ 0x80000000);
    c.set_f(0, c.f64(at(0x40)));
    c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
    c.put_f32(actor.address + 0xf0, float(c.f(0)));
    enter(c, 0x1022a814, {actor.address});
    current_role(c, actor.address);
}
}
