// MATCH_ENG.CPP, shots: picking a player of a side by a weighted random score.
#include "match_eng_shots.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace shots_detail;

namespace {
constexpr std::uint32_t starting_score = 0xc, magic_at = 0x60;  // pool 1 +0xc; pool 2 +0x60 (2^52+2^31)
constexpr std::uint32_t flags_of = 0x10289560, trait_of = 0x102875f0;
}

// 0x101e1640
std::uint32_t choose_candidate_weighted(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t reduced) {
    Frame frame(c, 0x101e1640, 0x90, {.first_gpr = 24, .gpr_offset = -0x40, .first_fpr = 29});
    c.put32(frame.at(0xa8), match);
    c.put8(frame.at(0xaf), side);
    c.put8(frame.at(0xb3), reduced);
    Fpu fp(c);
    const auto pool = c.toc(-0x52c8), table = c.toc(-0x52c4);
    keep(c, {{31, pool}, {30, table}});
    c.set_f(29, c.f32(pool + starting_score));
    Kept<26> best(c, UINT32_MAX);
    c.set(4, c.u32(frame.at(0xa8)));
    Kept<29> player(c, c.r(4) + u(s8(c.u8(frame.at(0xaf))) * std::int32_t(side_size)) + players_at);
    Kept<27> index(c, 0);
    const auto scale = [&](std::uint32_t at) { c.set_f(31, fp.fmuls(c.f(31), c.f32(pool + at))); };
    const auto load = [&](std::uint32_t at) { c.set_f(31, c.f32(pool + at)); };
    // The stack holds the integer under the magic exponent; f1 holds the pool's reduction constant.
    const auto reduction = [&] { const double magic = c.f64(table + magic_at); c.set_f(1, magic); return magic; };
    const auto visit = [&] {
        c.set(3, player);
        if (compare(c, s8(position(c, player)), 0) < 0) return;
        c.set(3, player);
        c.call(flags_of, 0x101e16b4);
        keep(c, {{28, c.r(3)}});
        const std::uint32_t flags = c.r(28);
        if (flagged(c, flags, 1)) return;  // flag 1: not a candidate
        if (flagged(c, flags, 2)) load(0x98);
        else if (flagged(c, flags, 4)) load(0x9c);
        else if (flagged(c, flags, 8)) load(0x54);
        else if (flagged(c, flags, 0x10)) load(0x44);
        else if (flagged(c, flags, 0x20)) load(0x84);
        else if (flagged(c, flags, 0x40) && (flagged(c, flags, 0x100) || flagged(c, flags, 0x200) || flagged(c, flags, 0x400))) load(0x70);
        else if (flagged(c, flags, 0x40)) load(0x10);
        // A candidate with none of the flags keeps the previous candidate's factor (f31 is never reset).
        if (compare_logical(c, c.u32(player + 0x70), 0) != 0) {
            const auto stat = [&] { return std::int32_t(c.i16(c.u32(player + 0x70) + 4)); };
            if (compare(c, stat(), 0x8c) > 0) scale(0xa0);
            else if (compare(c, stat(), 0x3c) < 0) scale(0xa4);
            else {
                const double wide = c.f64(table + 0x1f8);
                const double magic = reduction();
                const auto level = from_int_single(c, fp, frame.at(0x40), u(stat()), magic);
                c.set_f(31, fp.fmuls(c.f(31), fp.frsp(fp.fmul(wide, level))));
            }
            keep(c, {{24, draw(c, 20, 0x101e181c)}});
            c.set(3, u(s8(c.u8(c.u32(player + 0x70) + 0x21))));
            if (compare(c, std::int32_t(draw(c, c.r(3), 0x101e1830)), std::int32_t(c.r(24))) < 0) scale(0xa8);
        }
        if (compare(c, s8(c.u8(frame.at(0xb3))), 0) != 0) {
            const auto bound = truncated(c, fp, frame.at(0x40), c.f32(player + 0xcc));
            draw(c, bound, 0x101e1864);
            c.set_f(30, from_int_single(c, fp, frame.at(0x38), c.r(3), reduction()));
        } else if (compare(c, std::int32_t(draw(c, 7, 0x101e188c)), 0) == 0) {
            const double bias = c.f32(pool + 0x3c);
            c.set_f(1, bias);
            const auto bound = truncated(c, fp, frame.at(0x38), fp.fadds(bias, c.f32(player + 0xa4)));
            draw(c, bound, 0x101e18b0);
            c.set_f(30, from_int_single(c, fp, frame.at(0x40), c.r(3), reduction()));
        } else if (compare(c, std::int32_t(draw(c, 3, 0x101e18d8)), 0) != 0) {
            const double f9 = c.f64(table + 0x258), f7 = c.f64(table + 0x1c0), f5 = c.f64(table + 0x260);
            const double f3 = c.f64(table + 0x1e0);
            c.set_f(1, c.f64(table + 0x1e0));
            double sum = fp.fmul(c.f64(table + 0x1e0), c.f32(player + 0xb4));
            sum = fp.multiply_add(0x101e190c, f3, c.f32(player + 0x80), sum);
            sum = fp.multiply_add(0x101e1910, f5, c.f32(player + 0x98), sum);
            sum = fp.multiply_add(0x101e1914, f7, c.f32(player + 0xa0), sum);
            sum = fp.fadd(c.f32(player + 0x9c), sum);
            sum = fp.fadd(f9, sum);
            draw(c, truncated(c, fp, frame.at(0x38), sum), 0x101e1930);
            c.set_f(30, fp.fmuls(c.f(31), from_int_single(c, fp, frame.at(0x40), c.r(3), reduction())));
        } else {
            const double base = c.f32(pool + 0x28);
            c.set_f(1, c.f64(table + 0xe0));
            const double first = fp.frsp(fp.fmul(c.f64(table + 0xe0), c.f32(player + 0xdc)));
            c.set_f(1, c.f64(table + 0x118));
            const double second = fp.frsp(fp.fmul(c.f64(table + 0x118), c.f32(player + 0xb0)));
            double sum = fp.fadds(c.f32(player + 0xac), second);
            sum = fp.fadds(first, sum);
            sum = fp.fadds(base, sum);
            draw(c, truncated(c, fp, frame.at(0x38), sum), 0x101e1998);
            c.set_f(30, fp.fmuls(c.f(31), from_int_single(c, fp, frame.at(0x40), c.r(3), reduction())));
        }
        const double limit = c.f32(pool + 0xac);
        fp.fcmp(0, c.f(30), limit);
        if (c.f(30) > limit) {
            draw(c, 5, 0x101e19cc);
            c.set_f(30, fp.fsubs(c.f(30), from_int_single(c, fp, frame.at(0x38), c.r(3), reduction())));
        }
        if (compare(c, s8(best), -1) != 0) {
            fp.fcmp(0, c.f(30), c.f(29));
            if (!(c.f(30) > c.f(29))) return;
        }
        best = index;
        c.set_f(29, c.f(30));
    };
    for (; compare(c, s8(index), 0x14) < 0; index = index + 1, keep(c, {{25, player}}), player = player + player_size) visit();
    return result(c, best);
}

// 0x101e1a50
std::uint32_t choose_candidate_by_mode(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t mode) {
    Frame frame(c, 0x101e1a50, 0x80, {.first_gpr = 26, .gpr_offset = -0x38, .first_fpr = 29});
    c.put32(frame.at(0x98), match);
    c.put8(frame.at(0x9f), side);
    c.put8(frame.at(0xa3), mode);
    Fpu fp(c);
    const auto pool = c.toc(-0x52c8);
    keep(c, {{31, pool}});
    c.set_f(29, c.f32(pool + starting_score));
    Kept<28> best(c, UINT32_MAX);
    c.set(4, c.u32(frame.at(0x98)));
    Kept<30> player(c, c.r(4) + u(s8(c.u8(frame.at(0x9f))) * std::int32_t(side_size)) + players_at);
    Kept<29> index(c, 0);
    const auto visit = [&] {
        c.set(3, player);
        if (compare(c, s8(position(c, player)), 0) < 0) return;
        c.set(3, player);
        c.call(flags_of, 0x101e1ac0);
        keep(c, {{27, c.r(3)}});
        c.set_f(31, c.f32(pool + (flagged(c, c.r(27), 1) ? 0xb0 : 0x10)));
        // 2: the trait (0x102875f0 with 0) plus 25 is drawn from; any other mode: plus 5.
        const bool two = compare(c, s8(c.u8(frame.at(0xa3))), 2) == 0;
        c.set(3, player);
        c.set(4, 0);
        c.call(trait_of, two ? 0x101e1afc : 0x101e1b3c);
        draw(c, u(s8(c.r(3)) + (two ? 0x19 : 5)), two ? 0x101e1b08 : 0x101e1b48);
        const auto table = c.toc(-0x52c4);
        c.set(4, table);
        const double magic = c.f64(table + magic_at);
        c.set_f(1, magic);
        c.set_f(30, fp.fmuls(c.f(31), from_int_single(c, fp, frame.at(0x38), c.r(3), magic)));
        if (compare(c, s8(best), -1) != 0) {
            fp.fcmp(0, c.f(30), c.f(29));
            if (!(c.f(30) > c.f(29))) return;
        }
        best = index;
        c.set_f(29, c.f(30));
    };
    for (; compare(c, s8(index), 0x14) < 0; index = index + 1, keep(c, {{26, player}}), player = player + player_size) visit();
    return result(c, best);
}

// 0x101e1bd0
std::uint32_t choose_candidate_ranked(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t mode) {
    Frame frame(c, 0x101e1bd0, 0x90, {.first_gpr = 22, .gpr_offset = -0x48, .first_fpr = 29});
    c.put32(frame.at(0xa8), match);
    c.put8(frame.at(0xaf), side);
    Fpu fp(c);
    keep(c, {{23, mode}});
    const auto pool = c.toc(-0x52c8), table = c.toc(-0x52c4);
    keep(c, {{31, pool}, {28, table}});
    c.set_f(29, c.f32(pool + starting_score));
    Kept<25> best(c, UINT32_MAX);
    c.set(4, c.u32(frame.at(0xa8)));
    Kept<30> player(c, c.r(4) + u(s8(c.u8(frame.at(0xaf))) * std::int32_t(side_size)) + players_at);
    Kept<26> index(c, 0);
    const auto scale = [&](std::uint32_t at) { c.set_f(31, fp.fmuls(c.f(31), c.f32(pool + at))); };
    const auto load = [&](std::uint32_t at) { c.set_f(31, c.f32(pool + at)); };
    const auto reduction = [&] { const double magic = c.f64(table + magic_at); c.set_f(1, magic); return magic; };
    const auto visit = [&] {
        c.set(3, player);
        if (compare(c, s8(position(c, player)), 0) < 0) return;
        c.set(3, player);
        c.call(flags_of, 0x101e1c44);
        keep(c, {{29, c.r(3)}});
        const std::uint32_t flags = c.r(29);
        if (flagged(c, flags, 1)) return;
        if (flagged(c, flags, 2)) load(0xb4);
        else if (flagged(c, flags, 4)) load(0xb0);
        else if (flagged(c, flags, 8) && (flagged(c, flags, 0x80) || flagged(c, flags, 0x800))) load(0x54);
        else if (flagged(c, flags, 8)) load(0xb8);
        else if (flagged(c, flags, 0x10) && (flagged(c, flags, 0x80) || flagged(c, flags, 0x800))) load(0x40);
        else if (flagged(c, flags, 0x10)) load(0x78);
        else if (flagged(c, flags, 0x20)) load(0x58);
        else if (flagged(c, flags, 0x40) && (flagged(c, flags, 0x100) || flagged(c, flags, 0x200) || flagged(c, flags, 0x400))) load(0x5c);
        else if (flagged(c, flags, 0x40)) load(0x84);
        // A candidate with none of the flags keeps the previous candidate's factor (f31 is never reset).
        if (compare_logical(c, c.u32(player + 0x70), 0) != 0) {
            const auto stat = [&] { return std::int32_t(c.i16(c.u32(player + 0x70) + 4)); };
            if (compare(c, stat(), 0x8c) > 0) scale(0xa0);
            else if (compare(c, stat(), 0x3c) < 0) scale(0xa4);
            else {
                const double wide = c.f64(table + 0x1f8);
                const double magic = reduction();
                const auto level = from_int_single(c, fp, frame.at(0x40), u(stat()), magic);
                c.set_f(31, fp.fmuls(c.f(31), fp.frsp(fp.fmul(wide, level))));
            }
            for (const std::uint32_t offset : {0x37u, 0x43u}) {
                const auto value = [&] { return std::int32_t(s8(c.u8(c.u32(player + 0x70) + offset))); };
                if (compare(c, value(), 0xf) > 0) scale(0x70);
                else if (compare(c, value(), 6) < 0) scale(0x78);
                else if (compare(c, value(), 0xa) < 0) scale(0x5c);
            }
            if (compare(c, s8(c.r(23)), 0) == 0) {
                // The four attribute bytes summed as a byte (r4-r6 keep three of them).
                const auto sub = [&] { return c.u32(player + 0x70); };
                c.set(6, c.u8(sub() + 0x40));
                c.set(5, c.u8(sub() + 0x26));
                c.set(4, c.u8(sub() + 0x36));
                const std::uint32_t sum = c.u8(sub() + 0x1b) + c.r(5) + c.r(6);
                keep(c, {{27, u(s8(c.r(4) + sum))}});
                const auto total = std::int32_t(c.r(27));
                if (compare(c, total, 0x4b) > 0) scale(0x90);
                else if (compare(c, s8(c.r(27)), 0x3c) > 0) scale(0x58);
                else if (compare(c, s8(c.r(27)), 0x19) < 0) {
                    c.set_f(31, fp.frsp(fp.fmul(c.f(31), c.f64(table + 0x118))));
                } else if (compare(c, s8(c.r(27)), 0x28) < 0) scale(0x5c);
            }
            keep(c, {{22, draw(c, 20, 0x101e1f6c)}});
            c.set(3, u(s8(c.u8(c.u32(player + 0x70) + 0x21))));
            if (compare(c, std::int32_t(draw(c, c.r(3), 0x101e1f80)), std::int32_t(c.r(22))) < 0) scale(0x44);
        }
        if (compare(c, s8(c.r(23)), 0) != 0) {
            const double lead = c.f32(pool + 0x2c);
            c.set_f(1, fp.fsubs(c.f32(player + 0xa4), c.f32(player + 0x9c)));
            const double rest = fp.fsubs(c.f(1), c.f32(player + 0xb0));
            const auto bound = truncated(c, fp, frame.at(0x40), fp.fadds(lead, rest));
            draw(c, bound, 0x101e1fc8);
            c.set_f(30, from_int_single(c, fp, frame.at(0x38), c.r(3), reduction()));
        } else if (compare(c, std::int32_t(draw(c, 5, 0x101e1ff0)), 0) != 0) {
            const double lead = c.f32(pool + 0x8);
            double sum = fp.fadds(c.f32(player + 0xf4), c.f32(player + 0xc8));
            c.set_f(1, c.f32(player + 0xf4));
            sum = fp.fadds(c.f32(player + 0x90), sum);
            sum = fp.fadds(lead, sum);
            draw(c, truncated(c, fp, frame.at(0x38), sum), 0x101e2024);
            c.set_f(30, fp.fmuls(c.f(31), from_int_single(c, fp, frame.at(0x40), c.r(3), reduction())));
        } else {
            const double lead = c.f32(pool + 0x8);
            c.set_f(1, lead);
            const double sum = fp.fadds(lead, c.f32(player + 0xac));
            draw(c, truncated(c, fp, frame.at(0x38), sum), 0x101e2064);
            c.set_f(30, fp.fmuls(c.f(31), from_int_single(c, fp, frame.at(0x40), c.r(3), reduction())));
        }
        if (compare(c, s8(best), -1) != 0) {
            fp.fcmp(0, c.f(30), c.f(29));
            if (!(c.f(30) > c.f(29))) return;
        }
        best = index;
        c.set_f(29, c.f(30));
    };
    for (; compare(c, s8(index), 0x14) < 0; index = index + 1, keep(c, {{24, player}}), player = player + player_size) visit();
    // Even then the choice stands only one time in five.
    if (compare(c, std::int32_t(draw(c, 5, 0x101e20c0)), 0) != 0) c.set(3, best);
    else c.set(3, UINT32_MAX);
    return c.r(3);
}

// 0x101e2100
std::uint32_t choose_candidate_simple(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101e2100, 0x80, {.first_gpr = 26, .gpr_offset = -0x38, .first_fpr = 29});
    c.put32(frame.at(0x98), match);
    c.put8(frame.at(0x9f), side);
    Fpu fp(c);
    const auto pool = c.toc(-0x52c8);
    keep(c, {{31, pool}});
    c.set_f(29, c.f32(pool + starting_score));
    Kept<27> best(c, UINT32_MAX);
    c.set(4, c.u32(frame.at(0x98)));
    Kept<29> player(c, c.r(4) + u(s8(c.u8(frame.at(0x9f))) * std::int32_t(side_size)) + players_at);
    Kept<28> index(c, 0);
    const auto load = [&](std::uint32_t at) { c.set_f(31, c.f32(pool + at)); };
    const auto visit = [&] {
        c.set(3, player);
        if (compare(c, s8(position(c, player)), 0) < 0) return;
        c.set(3, player);
        c.call(flags_of, 0x101e216c);
        keep(c, {{30, c.r(3)}});
        const std::uint32_t flags = c.r(30);
        if (flagged(c, flags, 1)) return;
        if (flagged(c, flags, 2)) load(0xbc);
        else if (flagged(c, flags, 4)) load(0xbc);
        else if (flagged(c, flags, 8)) load(0xb0);
        else if (flagged(c, flags, 0x10)) load(0x54);
        else if (flagged(c, flags, 0x20)) load(0x44);
        else if (flagged(c, flags, 0x40) && (flagged(c, flags, 0x100) || flagged(c, flags, 0x200) || flagged(c, flags, 0x400))) load(0x58);
        else if (flagged(c, flags, 0x40)) load(0x10);
        const auto drawn = draw(c, 0x19, 0x101e225c);
        const auto table = c.toc(-0x52c4);
        c.set(4, table);
        const double magic = c.f64(table + magic_at);
        c.set_f(1, magic);
        c.set_f(30, fp.fmuls(c.f(31), from_int_single(c, fp, frame.at(0x38), drawn, magic)));
        if (compare(c, s8(best), -1) != 0) {
            fp.fcmp(0, c.f(30), c.f(29));
            if (!(c.f(30) > c.f(29))) return;
        }
        best = index;
        c.set_f(29, c.f(30));
    };
    for (; compare(c, s8(index), 0x14) < 0; index = index + 1, keep(c, {{26, player}}), player = player + player_size) visit();
    return result(c, best);
}

// 0x101e22e0
std::uint32_t choose_candidate_by_traits(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101e22e0, 0x80, {.first_gpr = 26, .gpr_offset = -0x38, .first_fpr = 29});
    c.put32(frame.at(0x98), match);
    c.put8(frame.at(0x9f), side);
    Fpu fp(c);
    const auto pool = c.toc(-0x52c8);
    keep(c, {{31, pool}});
    c.set_f(29, c.f32(pool + starting_score));
    Kept<27> best(c, UINT32_MAX);
    c.set(4, c.u32(frame.at(0x98)));
    Kept<30> player(c, c.r(4) + u(s8(c.u8(frame.at(0x9f))) * std::int32_t(side_size)) + players_at);
    Kept<28> index(c, 0);
    const auto scale = [&](std::uint32_t at) { c.set_f(31, fp.fmuls(c.f(31), c.f32(pool + at))); };
    const auto load = [&](std::uint32_t at) { c.set_f(31, c.f32(pool + at)); };
    const auto visit = [&] {
        c.set(3, player);
        if (compare(c, s8(position(c, player)), 0) < 0) return;
        c.set(3, player);
        c.call(flags_of, 0x101e234c);
        keep(c, {{29, c.r(3)}});
        const std::uint32_t flags = c.r(29);
        if (flagged(c, flags, 1)) return;
        if (flagged(c, flags, 2)) load(0x8c);
        else if (flagged(c, flags, 4)) load(0x8c);
        else if (flagged(c, flags, 8)) load(0x40);
        else if (flagged(c, flags, 0x10)) load(0x54);
        else if (flagged(c, flags, 0x20)) load(0xc0);
        else if (flagged(c, flags, 0x40) && (flagged(c, flags, 0x100) || flagged(c, flags, 0x200) || flagged(c, flags, 0x400))) load(0xc4);
        else if (flagged(c, flags, 0x40)) load(0xb4);
        if (compare_logical(c, c.u32(player + 0x70), 0) != 0) {
            const auto value = [&](std::uint32_t offset) { return std::int32_t(s8(c.u8(c.u32(player + 0x70) + offset))); };
            if (compare(c, value(0x44), 0xf) > 0) scale(0x70);
            else if (compare(c, value(0x44), 0xa) > 0) scale(0x58);
            else if (compare(c, value(0x44), 5) < 0) scale(0x5c);
            for (const std::uint32_t offset : {0x39u, 0x3eu}) {
                if (compare(c, value(offset), 0xf) > 0) scale(0x70);
                else if (compare(c, value(offset), 5) < 0) scale(0x5c);
            }
        }
        const auto drawn = draw(c, 0x19, 0x101e251c);
        const auto table = c.toc(-0x52c4);
        c.set(4, table);
        const double magic = c.f64(table + magic_at);
        c.set_f(1, magic);
        c.set_f(30, fp.fmuls(c.f(31), from_int_single(c, fp, frame.at(0x38), drawn, magic)));
        if (compare(c, s8(best), -1) != 0) {
            fp.fcmp(0, c.f(30), c.f(29));
            if (!(c.f(30) > c.f(29))) return;
        }
        best = index;
        c.set_f(29, c.f(30));
    };
    for (; compare(c, s8(index), 0x14) < 0; index = index + 1, keep(c, {{26, player}}), player = player + player_size) visit();
    return result(c, best);
}

// 0x101e25a0
void role_nibbles(Cpu& c, std::uint32_t match, std::uint32_t player, std::uint32_t low_out, std::uint32_t high_out) {
    Frame frame(c, 0x101e25a0, 0x50, {.first_gpr = 28});
    keep(c, {{30, match}, {31, player}, {28, low_out}, {29, high_out}});
    // The offset of the rating byte in the side's table: by match mode 0 and 1 the side decides, any other mode has one.
    std::uint32_t offset = 0x35;
    const auto mode = s8(c.u8(match + possession_side));
    const auto side = [&] { return s8(c.u8(player + 0x27)); };
    if (compare(c, mode, 0) == 0) offset = compare(c, side(), 0) == 0 ? 0x9f : 0x30;
    else if (compare(c, mode, 1) == 0) offset = compare(c, side(), 1) == 0 ? 0xa4 : 0x3b;
    const auto rating = [&] {
        c.set(3, player);
        const auto slot = u(s8(position(c, player))) * 0xd8;
        c.set(4, slot);
        c.set(3, c.u32(match + u(side()) * 4 + 0xdfec));
        return c.u8(c.r(3) + slot + offset);
    };
    c.put8(low_out, s8(rating() & 0xf) & 0xff);
    c.put8(high_out, s8((rating() & 0xf0) >> 4) & 0xff);
}
}
