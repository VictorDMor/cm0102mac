#include "match_pl_player_response.hpp"
#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;

namespace {
std::uint32_t gap(Cpu& c, int rating, unsigned ceiling = 20) {
    c.s.wr<1>(reg::xer_ca, std::uint32_t(rating) <= ceiling);
    return ceiling - std::uint32_t(rating);
}
}

PlayerResponse::PlayerResponse(Cpu& cpu, std::uint32_t player, std::uint32_t count_side)
    : c(cpu), frame(c, 0x1022ba10, 0x4f0, {.first_gpr = 26, .gpr_offset = -0x58, .first_fpr = 24}),
      fp(c), actor{c, player}, doubles(c.toc(-0x51e8)) {
    const auto floats = c.toc(-0x51e0);
    keep(c, {{29, floats}, {31, player}, {30, doubles}, {27, count_side}});
    c.set_f(31, c.f32(floats + 0x34));
}

bool PlayerResponse::role_has(std::uint32_t back, unsigned mask, bool secondary) {
    enter(c, back, {actor.address});
    if (secondary) secondary_role(c, actor.address);
    else current_role(c, actor.address);
    return compare(c, c.r(3) & mask) != 0;
}

void PlayerResponse::reputation() {
    c.set(3, c.u32(actor.address + 0x70));
    if (compare(c, c.i8(c.r(3) + 0xf), 15) > 0) {
        keep(c, {{28, 20}});
        return;
    }
    c.set(3, c.u32(actor.address + 0x70));
    c.put32(at(0x488), 0x43300000);
    const auto rating = c.i8(c.r(3) + 0xf);
    c.set_f(2, c.f64(doubles + 0x80));
    c.set_f(3, c.f64(doubles + 0x30));
    c.set_f(0, c.f64(doubles + 0x18));
    c.put32(at(0x48c), std::uint32_t(std::int32_t(rating)) ^ 0x80000000);
    c.set_f(1, c.f64(at(0x488)));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(0, fp.multiply_add(0x1022ba8c, c.f(3), c.f(1), c.f(0)));
    c.put64(at(0x480), fp.fctiwz(0, c.f(0)));
    keep(c, {{28, c.u32(at(0x484))}});
}

void PlayerResponse::subtract_scaled_gap() {
    c.set_f(0, c.f64(at(0x480)));
    c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    c.put64(at(0x488), fp.fctiwz(0, c.f(0)));
    c.set(3, c.u32(at(0x48c)));
}

void PlayerResponse::subtract_scaled(unsigned field, unsigned multiplier, bool count_side) {
    c.set(3, c.u32(actor.address + 0x70));
    c.put32(at(0x480), 0x43300000);
    if (count_side) compare(c, std::int8_t(c.r(27)));
    const auto rating = c.i8(c.r(3) + field);
    c.set_f(1, c.f64(doubles + 0x80));
    c.set_f(2, c.f64(doubles + multiplier));
    c.set(3, gap(c, rating));
    const auto accumulated = c.i16(actor.address + 0x38);
    c.set(3, c.r(3) ^ 0x80000000);
    c.put32(at(0x484), c.r(3));
    subtract_scaled_gap();
    c.put16(actor.address + 0x38, std::uint32_t(accumulated) - c.r(3));
}

void PlayerResponse::subtract_adjacent(unsigned multiplier) {
    c.set(4, c.u32(actor.address + 0x70));
    c.set(3, c.u32(actor.address + 0x70));
    c.set(4, c.u8(c.r(4) + 0x14));
    const auto central = c.i8(c.r(3) + 0x13);
    c.set(3, std::int32_t(std::int8_t(c.r(4))));
    c.set_f(2, c.f64(doubles + multiplier));
    c.set(3, c.r(3) - 5);
    const bool adjacent = compare(c, std::int32_t(c.r(3)), central) > 0;
    c.set(3, c.u32(actor.address + 0x70));
    const auto rating = c.i8(c.r(3) + (adjacent ? 0x14 : 0x13)) - (adjacent ? 5 : 0);
    c.set(3, gap(c, rating) ^ 0x80000000);
    c.put32(at(0x480), 0x43300000);
    c.set_f(1, c.f64(doubles + 0x80));
    c.put32(at(0x484), c.r(3));
    const auto accumulated = c.i16(actor.address + 0x38);
    subtract_scaled_gap();
    c.put16(actor.address + 0x38, std::uint32_t(accumulated) - c.r(3));
}

void PlayerResponse::subtract_wide(std::uint32_t site, unsigned field) {
    c.set(4, c.u32(actor.address + 0x70));
    c.set(3, c.u32(actor.address + 0x70));
    c.set(4, c.u8(c.r(4) + 0x14));
    c.set(3, c.u8(c.r(3) + field));
    c.set(4, std::int32_t(std::int8_t(c.r(4))));
    c.put32(at(0x480), 0x43300000);
    c.set_f(2, c.f64(doubles + 0x80));
    c.set(3, c.r(4) * std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
    c.set_f(3, c.f64(doubles + 0x238));
    c.set_f(0, c.f64(doubles + 0x40));
    c.set_f(4, c.f64(doubles + 0x28));
    c.set_f(5, c.f64(doubles + 0x90));
    const auto accumulated = c.i16(actor.address + 0x38);
    c.set(3, c.r(3) ^ 0x80000000);
    c.put32(at(0x484), c.r(3));
    c.set_f(1, c.f64(at(0x480)));
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(0, fp.multiply_add(site, c.f(3), c.f(1), c.f(0)));
    c.set_f(0, fp.fadd(c.f(4), c.f(0)));
    c.set_f(0, fp.fmul(c.f(5), c.f(0)));
    c.put64(at(0x488), fp.fctiwz(0, c.f(0)));
    c.set(3, c.u32(at(0x48c)));
    c.put16(actor.address + 0x38, std::uint32_t(accumulated) - c.r(3));
}

void PlayerResponse::primary_position() {
    if (role_has(0x1022baac, 1)) {
        const auto penalty = std::int16_t(gap(c, std::int8_t(c.r(28))) * 2);
        c.set(3, std::int32_t(penalty));
        c.put16(actor.address + 0x38, std::uint32_t(c.i16(actor.address + 0x38)) - c.r(3));
        return;
    }
    if (role_has(0x1022badc, 2)) { subtract_scaled(0x10, 0x90); return; }
    if (role_has(0x1022bb3c, 4)) { subtract_scaled(0x11, 0x90); return; }
    if (role_has(0x1022bb9c, 8)) {
        if (!(role_has(0x1022bbac, 0x800) || role_has(0x1022bbbc, 0x80))) {
            subtract_scaled(0x12, 0x90);
            return;
        }
        if ((role_has(0x1022bbcc, 0x800, true) || role_has(0x1022bbdc, 0x80, true)) &&
            (role_has(0x1022bbec, 8, true) || role_has(0x1022bbfc, 0x10, true) ||
             role_has(0x1022bc0c, 0x20, true) || role_has(0x1022bc1c, 0x40, true))) {
            subtract_scaled(0x16, 0x90);
        } else {
            const bool defensive = (role_has(0x1022bc7c, 0x800, true) || role_has(0x1022bc8c, 0x80, true)) &&
                (role_has(0x1022bc9c, 4, true) || role_has(0x1022bcac, 2, true));
            subtract_scaled(defensive ? 0x11 : 0x12, 0x90);
        }
        return;
    }
    if (role_has(0x1022bdac, 0x10)) { subtract_adjacent(0x90); return; }
    if (role_has(0x1022be44, 0x20)) {
        if (role_has(0x1022be54, 0x800) || role_has(0x1022be64, 0x80)) subtract_scaled(0x14, 0x90);
        else {
            enter(c, 0x1022bec4, {actor.address});
            strategy_bit21(c, actor.address);
            const bool attacking = compare(c, std::int8_t(c.r(3))) != 0;
            subtract_wide(attacking ? 0x1022bf18 : 0x1022bf88, attacking ? 0x1a : 0x15);
        }
        return;
    }
    if (role_has(0x1022bfb4, 0x40)) subtract_scaled(0x15, 0x90);
}

void PlayerResponse::subtract_specialty(unsigned field) {
    c.set(3, c.u32(actor.address + 0x70));
    const auto accumulated = c.i16(actor.address + 0x38);
    c.set(3, std::int32_t(c.i8(c.r(3) + field)));
    c.set(3, std::int32_t(std::int16_t(gap(c, std::int32_t(c.r(3))))));
    c.put16(actor.address + 0x38, std::uint32_t(accumulated) - c.r(3));
}

void PlayerResponse::subtract_hybrid_specialty(unsigned half_field) {
    c.set(3, c.u32(actor.address + 0x70));
    c.set(4, c.u32(actor.address + 0x70));
    const auto candidate = c.i8(c.r(3) + half_field);
    c.set(4, c.u8(c.r(4) + 0x19));
    c.set(3, std::int32_t(candidate));
    c.set(4, std::int32_t(std::int8_t(c.r(4))));
    const bool use_full = compare(c, std::int32_t(c.r(4)), candidate / 2) > 0;
    c.set(3, c.u32(actor.address + 0x70));
    const auto rating = c.i8(c.r(3) + (use_full ? 0x19 : half_field));
    if (!use_full) c.set(3, std::int32_t(rating));
    c.set(3, gap(c, use_full ? rating : rating / 2));
    const auto accumulated = c.i16(actor.address + 0x38);
    c.set(3, std::int32_t(std::int16_t(c.r(3))));
    c.put16(actor.address + 0x38, std::uint32_t(accumulated) - c.r(3));
}

void PlayerResponse::primary_specialty() {
    if (role_has(0x1022c010, 0x80)) subtract_specialty(0x18);
    else if (role_has(0x1022c044, 0x100)) subtract_hybrid_specialty(0x18);
    else if (role_has(0x1022c0c0, 0x200)) subtract_specialty(0x19);
    else if (role_has(0x1022c0f4, 0x400)) subtract_hybrid_specialty(0x17);
    else if (role_has(0x1022c170, 0x800)) subtract_specialty(0x17);
    if (role_has(0x1022c1a0, 0x1000)) subtract_specialty(0x1a);
}

void PlayerResponse::increment_side_counter(unsigned amount) {
    const auto side = std::int8_t(actor.side());
    c.set(3, actor.match());
    c.set(4, c.r(3) + std::uint32_t(std::int32_t(side)));
    c.set(3, c.u8(c.r(4) + 0xaf));
    c.put8(c.r(4) + 0xaf, c.r(3) + amount);
}

void PlayerResponse::secondary_position() {
    enter(c, 0x1022c1d0, {actor.address});
    current_role(c, actor.address);
    const auto primary = std::int16_t(c.r(3));
    c.set(3, actor.address);
    keep(c, {{26, std::uint32_t(std::int32_t(primary))}});
    enter(c, 0x1022c1e0, {actor.address});
    secondary_role(c, actor.address);
    if (compare(c, std::int16_t(c.r(3)), primary) == 0) return;

    if (role_has(0x1022c1f4, 2, true)) { subtract_scaled(0x10, 0x30); return; }
    if (role_has(0x1022c254, 4, true)) { subtract_scaled(0x11, 0x30); return; }
    if (role_has(0x1022c2b4, 8, true)) { subtract_scaled(0x12, 0x30); return; }
    if (role_has(0x1022c314, 0x10, true)) { subtract_adjacent(0x30); return; }
    if (role_has(0x1022c3ac, 0x20, true)) {
        subtract_scaled(0x14, 0x30, true);
        if (!(c.cr(0) & 2) && !role_has(0x1022c410, 0x40) && !role_has(0x1022c420, 0x20) &&
            (role_has(0x1022c430, 0x100, true) || role_has(0x1022c440, 0x200, true) || role_has(0x1022c450, 0x400, true)))
            increment_side_counter(1);
    } else if (role_has(0x1022c480, 0x40, true)) {
        subtract_scaled(0x15, 0x30, true);
        if (!(c.cr(0) & 2) && !role_has(0x1022c4e4, 0x40) &&
            (role_has(0x1022c4f4, 0x100, true) || role_has(0x1022c504, 0x200, true) || role_has(0x1022c514, 0x400, true)))
            increment_side_counter(2);
    }
}

void PlayerResponse::run() {
    enter(c, 0x1022ba40, {actor.address});
    {
        PlayerInfluence influence(c, actor.address);
        influence.run();
    }
    reputation();
    c.set(3, actor.address);
    c.put16(actor.address + 0x38, 0);
    primary_position();
    primary_specialty();
    secondary_position();
    secondary_specialty();
    publish_initial_ratings();
    publish_remaining_bytes();
    publish_reputation_composite();
    publish_physical_composites();
    publish_composite_attributes();
    normalize_attributes();
}
}
