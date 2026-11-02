#include "bra_reg_parana_detail.hpp"

namespace cm::aot::bra_reg_parana {
using namespace detail;

void advance_stage(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f80a0);
    Frame f(c, 0x106f80a0, 0x70, {.first_gpr = 30});
    keep(c, {{31, c.toc(slot::pool)}, {30, c.toc(slot::message)}});
    const Competition league{c, address}; Program p{c};
    const std::uint32_t count = league.stage_count().get(), last = league.last_stage().get();
    c.set({{4, count}, {5, last}});
    if (compare(c, std::int32_t(last), std::int32_t(count - 1)) >= 0) {
        assertion(c, f, 0x106f80e4, 0x40, 0x38, 0x202);
        c.set(3, c.toc(slot::flag)); return;
    }
    league.last_stage() = std::int32_t(last + 1);
    if (compare(c, league.last_stage(), 2) == 0)
        p.owned(0x106f8160, 0x106f8144, [&] { build_knockout(c, address); });
}

void build_knockout(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f8160);
    Frame f(c, 0x106f8160, 0xd0, {.first_gpr = 25});
    keep(c, {{27, address}, {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    c.set(4, c.u16(c.r(2) - 0x3620));
    c.put16(f.at(0x5c), c.r(4)); c.put16(f.at(0x5e), c.u16(c.r(2) - 0x361e));
    Kept<28> finalists(c, p.call(0x10775130, 0x106f8198, {8}));
    if (compare(c, finalists) == 0) {
        assertion(c, f, 0x106f81b4, 0x90, 0x68, 0x226); return;
    }
    const auto turn_table = [&](unsigned turn) { return c.u32(c.u32(std::uint32_t(league.stages()) + turn * 4) + 0xb0); };
    c.put32(finalists, c.u32(turn_table(0)));
    c.put32(std::uint32_t(finalists) + 4, c.u32(turn_table(1)));
    c.set(3, c.u32(finalists));
    if (compare_logical(c, c.r(3), c.u32(std::uint32_t(finalists) + 4)) == 0) {
        // The same club won both turns: the original abandons the allocated
        // finalists array here, and awards the title without constructing a final.
        league.last_stage() = 1; league.stage_count() = 2;
        c.set({{4, 5}, {7, 0}, {5, 0}});
        c.put8(turn_table(0) + 0x40, 5); c.put8(turn_table(1) + 0x40, 5);
        std::uint32_t row = 0, offset = 0;
        while (compare(c, result(c, extended(std::int16_t(row))), league.club_count()) < 0) {
            const auto standing = std::uint32_t(league.standings()) + offset;
            c.set(6, standing); c.set(3, c.u32(standing));
            if (compare_logical(c, c.r(3), c.u32(finalists)) == 0) c.put8(standing + 0x40, 5);
            offset += standing_size; ++row; c.set({{5, offset}, {7, row}});
        }
        return;
    }
    c.set({{3, address}, {4, 2}, {5, f.at(0x5a)}, {6, f.at(0x58)}, {7, 0}});
    Kept<26> schedule(c, p.virtual_call(address, 0x44, 0x106f82c4));
    Kept<25> cup(c, p.call(0x10773520, 0x106f82d8, {0xb4}));
    if (compare(c, cup) != 0) {
        for (const auto [offset, value] : {std::pair{0x38u, 2u}, {0x3c, 1}, {0x40, extended(c.i16(f.at(0x58)))},
                {0x44, 0x14}, {0x48, 0}, {0x4c, 1}, {0x50, 0}, {0x54, f.at(0x5c)}})
            c.put32(f.at(offset), value);
        c.set({{4, address}, {5, 2}, {6, finalists}, {7, extended(c.i16(f.at(0x5a)))},
               {8, league.definition()}, {9, schedule}, {10, extended(league.year())}});
        c.call(0x104bc3c0, 0x106f8338);
    }
    c.put32(std::uint32_t(league.stages()) + 8, cup);
    c.set(3, c.u32(std::uint32_t(league.stages()) + 8));
    if (compare_logical(c, c.r(3)) == 0) {
        assertion(c, f, 0x106f8364, 0x70, 0x60, 0x250); return;
    }
    c.set(4, 0); c.call(0x1047ef50, 0x106f83ac);
    p.call(0x10775170, 0x106f83b4, {schedule});
    p.call(0x10775170, 0x106f83bc, {finalists});
    league.last_stage() = 2;
}

void champion(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f8720);
    Frame f(c, 0x106f8720, 0x40, {.first_gpr = 30});
    Kept<31> database(c, c.toc(-0x7ab8)); Kept<30> object(c, address);
    Program p{c}; const Competition league{c, address};
    if (compare(c, league.last_stage(), 2) == 0) {
        const auto final = c.u32(std::uint32_t(league.stages()) + 8);
        c.set(3, final); p.virtual_call(final, 0x38, 0x106f875c);
    } else {
        c.call(0x104a1350, 0x106f8768);
        const auto award = [&](unsigned row, std::uint32_t money, unsigned position,
                               std::uint32_t lookup_back, std::uint32_t money_back, std::uint32_t news_back) {
            const auto club = c.u32(std::uint32_t(league.standings()) + row * standing_size);
            p.call(0x10103510, lookup_back, {c.u32(database), club});
            c.set(4, money); c.call(0x100f71f0, money_back);
            p.call(0x10452010, news_back, {object, c.u32(std::uint32_t(league.standings()) + row * standing_size),
                    0, money, position, 0, 0, 0xfffffffe});
        };
        award(0, 175000, 1, 0x106f8778, 0x106f8784, 0x106f87b0);
        award(1, 100000, 2, 0x106f87c0, 0x106f87cc, 0x106f87f8);
    }
    c.call(0x10517310, 0x106f87fc);
}

void seed_clubs(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f8820);
    Frame f(c, 0x106f8820, 0xa0, {.first_gpr = 27});
    Kept<27> seed(c, c.toc(slot::flag)); Kept<28> position(c, c.toc(slot::message));
    keep(c, {{29, c.toc(slot::pool)}, {30, address}});
    Program p{c}; const Competition league{c, address};
    if (compare_logical(c, league.fixtures()) == 0) {
        assertion(c, f, 0x106f8860, 0x68, 0x40, 0x33f); return;
    }
    Kept<31> order(c, p.call(0x10775130, 0x106f88ac, {std::uint32_t(std::int32_t(league.club_count())) * 4}));
    if (compare(c, order) == 0) { assertion(c, f, 0x106f88c8, 0x48, 0x38, 0x347); return; }
    std::uint32_t copied = 0;
    while (compare(c, std::int16_t(copied), league.club_count()) < 0) {
        c.put32(std::uint32_t(order) + copied * 4, c.u32(std::uint32_t(league.standings()) + copied * standing_size)); ++copied;
    }
    c.set({{4, extended(league.club_count())}, {5, copied * standing_size}, {6, copied}});
    p.call(0x10477b40, 0x106f8944, {order});
    const auto assign = [&](std::uint32_t at, std::uint32_t value, std::uint32_t back) {
        p.call(0x10457490, back, {league.fixtures(), c.u32(std::uint32_t(order) + at * 4), value});
    };
    assign(0, 1, 0x106f895c); position = 2; assign(1, 2, 0x106f8978); seed = 3;
    while (compare(c, result(c, extended(std::int16_t(std::uint32_t(seed)))), league.club_count()) <= 0) {
        const auto at = extended(std::int16_t(std::uint32_t(position)));
        position = std::uint32_t(position) + 1;
        assign(at, std::uint32_t(seed) & 255, 0x106f899c); seed = std::uint32_t(seed) + 1;
    }
    p.call(0x10775170, 0x106f89b8, {order});
}

void fixture_times(Cpu& c, std::uint32_t address, std::uint32_t stage, std::uint32_t weekday,
                   std::uint32_t first_time, std::uint32_t second_time) {
    observe_entry(c, 0x106f89d0);
    Frame f(c, 0x106f89d0, 0x50, {.first_gpr = 28});
    keep(c, {{28, stage}, {29, weekday}, {30, first_time}, {31, second_time}});
    Program p{c};
    const auto fixture = p.call(0x10459b50, 0x106f8a04, {c.u32(address + 8)});
    if (compare(c, std::int8_t(stage), -1) == 0) {
        if (compare_logical(c, first_time & 255, 3) < 0) first_time = 1;
        if (compare_logical(c, second_time & 255, 3) < 0) second_time = 2;
    }
    c.put8(fixture + 0x76, weekday); c.put8(fixture + 0x77, first_time); c.put8(fixture + 0x78, second_time);
}
}
