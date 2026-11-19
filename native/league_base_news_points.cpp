// LEAGUE.CPP, block `news`: the "team_points" listing (vtable +0x8c, 0x104aa620). For a club of the league it
// builds a 16-byte row per club (club, a rate, a projected total, the games played), projects the totals with the
// win-points byte, sorts by reputation and by the projection, then gives the club's position under the plain
// projection, under 1.x and under /x scalings (the doubles at TOC -0x471c +0x20 and +0x28) to the virtual +0x70.
// Single precision throughout, through guest::Fpu so FPSCR is the original's.
#include "guest_fpu.hpp"
#include "league_base_news_detail.hpp"
#include <bit>

namespace cm::aot::league_base {
using namespace news;

namespace {
// Row of the projection array (16 bytes, one per club; inferred meanings).
namespace projection {
inline constexpr std::uint32_t club = 0, rate = 4 /* float */, played = 0xc /* halfword */, base = 8 /* halfword */,
                               total = 0xa /* halfword */, size = 0x10;
}
// The double the original builds with `lis 0x4330` and `xoris x,0x8000`: 2^52 + 2^31 + x.
double magic(std::uint32_t x) { return std::bit_cast<double>((std::uint64_t(0x43300000u) << 32) | (x ^ 0x80000000u)); }
}

std::uint32_t team_points(Cpu& c, std::uint32_t league, std::uint32_t club) {
    const Frame frame(c, 0x104aa620, 0x80, {.first_gpr = 21});
    Held h(c);
    guest::Fpu fpu(c);
    const auto by_half = c.toc(toc::by_half_a), first_rate = c.toc(toc::constant_4724), doubles = c.toc(toc::constant_471c);
    const auto pool = c.toc(toc::pool);
    h.v[31] = club; h.v[26] = by_half; h.v[27] = first_rate; h.v[30] = league; h.v[28] = doubles; h.v[29] = pool;
    const auto clubs = [&] { return std::int32_t(c.i16(league + competition::club_count)); };
    const auto rows = c.u32(league + league::standings);
    const double bias = c.f64(doubles + 8);
    // The club's row.
    std::uint32_t found = 0;
    for (std::int32_t i = 0, at = 0; std::int16_t(i) < clubs(); ++i, at += league::standing_row)
        if (c.u32(rows + std::uint32_t(at)) == club) { found = rows + std::uint32_t(std::int16_t(i)) * league::standing_row; break; }
    h.v[25] = found;
    if (found == 0) return 0;
    const auto n = std::uint32_t(clubs());
    h.publish();
    const auto array = invoke(c, 0x107735f0, 0x104aa6b4, {n * projection::size});
    h.v[24] = array;
    if (array == 0) {
        h.publish();
        // 0x107755c0 reports the failed allocation (line 0x1965); the original goes on with the null array.
        invoke(c, 0x107755c0, 0x104aa6cc, {pool + text::team_points, pool, 0x1965});
    }
    // One row per club (the standings' order), with its rate: points per game (halfword +0x1c over the byte +0x14)
    // when games were played, else the constant at TOC -0x4724.
    std::uint32_t r7 = 0, r8 = 0, r9 = 0;
    for (std::int32_t i = 0; std::int16_t(i) < clubs(); ++i) {
        const auto row = c.u32(league + league::standings) + std::uint32_t(i) * league::standing_row;
        const auto out = array + std::uint32_t(i) * projection::size;
        c.put32(out + projection::club, c.u32(row));
        c.put16(out + projection::base, c.u16(row + 0x1c));
        const auto games = std::int8_t(c.u8(row + 0x14));
        c.put16(out + projection::played, std::uint32_t(std::int32_t(games)));
        r7 = row; r9 = c.u8(row + 0x14);
        if (games > 0) {
            const auto points = std::uint32_t(std::int32_t(c.i16(row + 0x1c)));
            r8 = points ^ 0x80000000u;
            const double f1 = fpu.fsubs(magic(points), bias);
            c.set_f(1, f1);  // f1 is left as the original's last write (a caller's replay compares it)
            const double f0 = fpu.fsubs(magic(std::uint32_t(std::int32_t(games))), bias);
            c.put_f32(out + projection::rate, float(fpu.fdivs(f1, f0)));
        } else {
            r8 = c.u32(league + league::standings);
            c.put32(out + projection::rate, c.u32(first_rate));
        }
    }
    // The registers the sorts see besides their four arguments (the original's leftovers from the loop).
    c.set({{7, r7}, {8, r8}, {9, r9}});
    const auto year_half = [&] { return std::int32_t(c.i16(league + 0xa4)); };
    // The projected total of a row: base + (a4 - played) * rate, truncated.
    const auto project = [&](std::uint32_t out) {
        const double f1 = fpu.fsubs(magic(std::uint32_t(year_half() - std::int32_t(c.i16(out + projection::played)))), bias);
        c.set_f(1, f1);
        const double f0 = fpu.fmuls(f1, c.f32(out + projection::rate));
        const auto low = std::uint32_t(fpu.fctiwz(0, f0));
        c.put16(out + projection::total, std::uint32_t(std::int32_t(c.i16(out + projection::base))) + low);
    };
    const auto sort = [&](std::uint32_t descriptor, std::uint32_t back) {
        h.publish();
        invoke(c, comp::callee::sort, back, {array, n, projection::size, descriptor});
    };
    if (std::int32_t(std::int8_t(c.u8(found + 0x14))) * 3 < year_half()) {
        sort(c.toc(toc::by_reputation), 0x104aa7b4);
        // A rate falling linearly from the first club's down to 0: step = (w*c10 - w*c18) / n.
        const auto w = std::uint32_t(std::int32_t(c.i8(league + league::win_points)));
        const double f2 = fpu.fsub(magic(w), bias);
        const double f0a = fpu.fsub(magic(w), bias);
        double f4 = fpu.fmul(c.f64(doubles + 0x10), f2);
        double f0 = fpu.fmul(c.f64(doubles + 0x18), f0a);
        f4 = fpu.frsp(f4);
        f0 = fpu.frsp(f0);
        const double f1 = fpu.fsubs(f4, f0);
        c.set_f(1, f1);
        for (std::int32_t i = 0; std::int16_t(i) < clubs(); ++i) {
            double step = fpu.fsubs(magic(std::uint32_t(clubs())), bias);
            step = fpu.fdivs(f1, step);
            const double place = fpu.fsubs(magic(std::uint32_t(std::int32_t(std::int16_t(i)))), bias);
            step = fpu.multiply_add(0x104aa850, place, step, f4);
            c.put_f32(array + std::uint32_t(std::int16_t(i)) * projection::size + projection::rate, float(step));
        }
    }
    for (std::int32_t i = 0; std::int16_t(i) < clubs(); ++i) project(array + std::uint32_t(i) * projection::size);
    c.set(7, n);
    sort(by_half, 0x104aa8e8);
    // The club's row in each ordering; its rate scaled (x doubles+0x20, then / doubles+0x28) and its total again.
    const auto position = [&](std::uint32_t& held, auto&& scale) {
        for (std::int32_t j = 0; std::int16_t(j) < clubs(); ++j) {
            const auto row = array + std::uint32_t(j) * projection::size;
            if (c.u32(row) != club) continue;
            held = (std::uint32_t(j) + 1) & 0xff;
            scale(row);
            project(row);
            return;
        }
    };
    position(h.v[21], [&](std::uint32_t row) {
        const double f0 = fpu.fmul(c.f64(doubles + 0x20), c.f32(row + projection::rate));
        c.put_f32(row + projection::rate, float(fpu.frsp(f0)));
    });
    sort(by_half, 0x104aa9a4);
    position(h.v[23], [&](std::uint32_t row) {
        const double f0 = fpu.fdiv(c.f32(row + projection::rate), c.f64(doubles + 0x28));
        c.put_f32(row + projection::rate, float(fpu.frsp(f0)));
    });
    sort(by_half, 0x104aaa60);
    // The last pass keeps going: a later row of the same club wins.
    for (std::int32_t j = 0; std::int16_t(j) < clubs(); ++j)
        if (c.u32(array + std::uint32_t(j) * projection::size) == club) h.v[22] = (std::uint32_t(j) + 1) & 0xff;
    h.publish();
    invoke(c, callee::release_array, 0x104aaaa4, {array});
    h.publish();
    virtual_call(c, 0x70, 0x104aaac8, {league, club, c.u8(league + competition::owner), h.v[21], h.v[23], h.v[22]});
    return 1;
}
}
