// MATCH_ENG.CPP, match setup: constructor, destructor and the small helpers.
#include "match_eng_setup.hpp"

namespace cm::aot::match_eng {
using namespace guest;

std::uint32_t construct_match(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101d16a0, 0x40, {});
    c.put32(frame.at(0x58), match);
    // Array constructions: (address, element constructor, destructor, size, count).
    const auto construct = [&](std::uint32_t at, std::int32_t constructor_slot, std::int32_t destructor_slot,
                               std::uint32_t size, std::uint32_t count, std::uint32_t back) {
        c.set({{3, at}, {4, c.toc(constructor_slot)}, {5, c.toc(destructor_slot)}, {6, size}, {7, count}});
        c.call(0x10773320, back);
    };
    construct(match + players_at, -0x6404, -0x6408, 0x1c4, 0x28, 0x101d16cc);
    construct(match + side_stats_at, -0x640c, -0x6410, 0xd8, 2, 0x101d16e8);
    construct(match + team_tactics, -0x6414, -0x6418, team_tactics_size, 2, 0x101d1704);
    construct(match + 0x10000 - 0x51d4, -0x6414, -0x6418, team_tactics_size, 2, 0x101d1724);
    c.set(3, match + 0x10000 - 0x1fd0);
    c.call(0x1021b010, 0x101d1734);
    c.put32(match + 0x3114, 0);  // the message object slot
    return result(c, match);
}

std::uint32_t side_player(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101d3b60, 0x50, {.first_gpr = 29});
    keep(c, {{30, match}, {31, side}});
    c.set({{3, match}, {4, side}});
    fixture_flag_40(c, match, side);
    keep(c, {{29, c.r(3)}});
    c.set({{3, match}, {4, side}});
    const auto team = fixture_team(c, match, side);
    c.set({{3, c.u32(c.toc(-0x7a6c))}, {4, team}, {5, c.r(29)}, {6, 0}});
    c.call(0x103b0a00, 0x101d3bb4);
    // The slot the roster lookup found, in the side's block of players.
    const auto slot = u(s8(c.r(3)) * std::int32_t(player_size));
    const auto block = u(s8(side) * std::int32_t(side_size));
    return result(c, block + slot + players_at + match);
}

std::int32_t random_below(Cpu& c, std::int32_t limit) {
    Frame frame(c, 0x101d3c90, 0x70, {.first_gpr = 27});
    Fpu fp(c);
    keep(c, {{30, u(limit)}, {28, c.toc(-0x7b90)}, {29, c.toc(-0x7b84)}});
    const auto seed_at = c.r(28), cursor_at = c.r(29);
    if (compare(c, limit) == 0) return std::int32_t(result(c, 0));
    // The cursor steps two bytes along the table; past its end the table's seed is redrawn.
    c.set(4, c.u32(cursor_at) + 2);
    c.put32(cursor_at, c.r(4));
    if (compare_logical(c, c.r(4), c.u32(c.toc(-0x7b88))) > 0) {
        c.put32(cursor_at, c.toc(-0x7b8c));
        c.call(0x1077b770, 0x101d3ce4);
        c.put32(seed_at, c.r(3) & 0xffff);
    }
    const auto draw = [&] { return std::uint32_t(c.u16(c.u32(cursor_at))) + c.u32(seed_at); };
    // Above 65535 in magnitude the draw is scaled; compare with the first bound, then the second.
    const bool scaled = compare(c, limit, 0xffff) > 0 || compare(c, limit, -0xffff) < 0;
    if (!scaled) {
        const auto sum = draw();
        const auto remainder = std::int32_t(sum - u(std::int32_t(sum) / limit) * u(limit));
        keep(c, {{31, u(remainder)}});
        if (compare(c, limit) > 0) c.set(27, u(remainder));
        else c.set(27, u(-remainder));
        return std::int32_t(result(c, c.r(27)));
    }
    // r4 = draw modulo 65535 (a multiplication by 0x80008001 for the quotient), then the
    // fraction of `limit` that draw represents, plus a recursive tenth-scale correction.
    const auto sum = draw();
    c.set(4, sum);
    const auto high = [](std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * b) >> 32); };
    auto quotient = (high(std::int32_t(0x80008001u), std::int32_t(sum)) + std::int32_t(sum)) >> 15;
    quotient += std::int32_t(std::uint32_t(quotient) >> 31);
    const auto residue = sum - u(quotient) * 0xffffu;
    const auto pool = c.toc(-0x52c4);
    const auto widen = [&](std::int32_t at, std::uint32_t value) {
        c.put32(frame.at(at + 4), value ^ 0x80000000);
        c.put32(frame.at(at), 0x43300000);
        return c.f64(frame.at(at));
    };
    c.set_f(1, c.f64(pool + 0x60));
    const double scale = fp.fdiv(fp.fsub(widen(0x48, u(limit)), c.f64(pool + 0x60)), c.f64(pool + 0x70));
    c.set_f(1, c.f64(pool + 0x60));
    const double fraction = fp.fmul(scale, fp.fsub(widen(0x40, residue), c.f64(pool + 0x60)));
    c.put64(frame.at(0x38), fp.fctiwz(0, fraction));
    c.set(31, c.u32(frame.at(0x3c)));
    // limit * 10 / 65535, drawn recursively: a coarser draw added to the fraction.
    const auto tenfold = std::int32_t(u(limit) * 10);
    auto coarse = (high(std::int32_t(0x80008001u), tenfold) + tenfold) >> 15;
    coarse += std::int32_t(std::uint32_t(coarse) >> 31);
    enter(c, 0x101d3dc8, {u(coarse)});
    const auto inner = random_below(c, coarse);
    auto tenth = high(0x66666667, std::int32_t(u(inner) + 5)) >> 2;
    tenth += std::int32_t(std::uint32_t(tenth) >> 31);
    c.set(31, c.r(31) + u(tenth));
    if (compare(c, std::int32_t(c.r(31)), std::int32_t(u(limit) - 1)) > 0) c.set(31, u(limit) - 1);
    return std::int32_t(result(c, c.r(31)));
}

std::uint32_t destroy_match(Cpu& c, std::uint32_t match, std::uint32_t free_memory) {
    Frame frame(c, 0x101d3e50, 0x40, {.first_gpr = 31});
    keep(c, {{31, match}});
    c.put16(frame.at(0x5e), free_memory);
    if (compare_logical(c, match) == 0) return result(c, match);
    if (compare(c, s8(c.u8(match + 0x7913))) != 0 && compare_logical(c, c.u32(match + message_target)) != 0) {
        c.set({{3, c.u32(match + message_target)}, {4, 1}});
        c.call(0x101ff140, 0x101d3e98);
        c.put32(match + message_target, 0);
        c.put8(match + 0x7913, 0);
    }
    if (compare_logical(c, c.u32(match + 0x3114)) != 0) {
        c.set({{3, c.u32(match + 0x3114)}, {4, 1}});
        c.call(0x10738c20, 0x101d3ec0);
        c.put32(match + 0x3114, 0);
    }
    c.set({{3, match + 0x10000 - 0x1fd0}, {4, u(-1)}});
    c.call(0x1021b820, 0x101d3ed8);
    // Array destructions: (address, destructor, size, count).
    const auto destroy = [&](std::uint32_t at, std::int32_t slot, std::uint32_t size, std::uint32_t count, std::uint32_t back) {
        c.set({{3, at}, {4, c.toc(slot)}, {5, size}, {6, count}});
        c.call(0x10773420, back);
    };
    destroy(match + 0x10000 - 0x51d4, -0x6418, team_tactics_size, 2, 0x101d3ef0);
    destroy(match + team_tactics, -0x6418, team_tactics_size, 2, 0x101d3f04);
    destroy(match + side_stats_at, -0x6410, 0xd8, 2, 0x101d3f18);
    destroy(match + players_at, -0x6408, 0x1c4, 0x28, 0x101d3f2c);
    if (compare(c, c.i16(frame.at(0x5e))) > 0) {
        c.set(3, match);
        c.call(0x107735a0, 0x101d3f40);
    }
    return result(c, match);
}
}
