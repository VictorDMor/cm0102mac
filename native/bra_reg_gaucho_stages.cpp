#include "bra_reg_gaucho_detail.hpp"

namespace cm::aot::bra_reg_gaucho {
using namespace detail;

void advance_stage(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f2210);
    Frame f(c, 0x106f2210, 0x70, {.first_gpr = 30});
    keep(c, {{31, c.toc(slot::pool)}, {30, c.toc(slot::message)}});
    const Competition league{c, address}; Program p{c};
    const std::uint32_t count = league.stage_count().get(), last = league.last_stage().get();
    c.set({{4, count}, {5, last}});
    if (compare(c, std::int32_t(last), std::int32_t(count - 1)) >= 0) {
        assertion(c, f, 0x106f2254, 0x40, 0x38, 0x1f4);
        c.set(3, c.toc(slot::flag)); return;
    }
    league.last_stage() = std::int32_t(last + 1);
    if (compare(c, league.last_stage()) == 0)
        p.owned(0x106f22d0, 0x106f22b4, [&] { build_knockout(c, address); });
}

void build_knockout(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f22d0);
    Frame f(c, 0x106f22d0, 0xd0, {.first_gpr = 25});
    keep(c, {{27, address}, {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    c.set(4, c.u16(c.r(2) - 0x362c));
    c.put16(f.at(0x5c), c.r(4)); c.put16(f.at(0x5e), c.u16(c.r(2) - 0x362a));
    Kept<28> finalists(c, p.call(0x10775130, 0x106f2308, {16}));
    if (compare(c, finalists) == 0) {
        assertion(c, f, 0x106f2324, 0x90, 0x68, 0x217);
        return;
    }
    constexpr unsigned semifinal_order[] = {0, 3, 1, 2};
    for (unsigned i = 0; i < 4; ++i) {
        const auto table = std::uint32_t(league.standings());
        c.put32(std::uint32_t(finalists) + i * 4, c.u32(table + semifinal_order[i] * standing_size));
        if (i != 0) c.set(8, table);
    }
    c.set({{3, address}, {4, 0}, {5, f.at(0x5a)}, {6, f.at(0x58)}, {7, 0}});
    Kept<26> schedule(c, p.virtual_call(address, 0x44, 0x106f23b4));
    Kept<25> cup(c, p.call(0x10773520, 0x106f23c8, {0xb4}));
    if (compare(c, cup) != 0) {
        for (const auto [offset, value] : {std::pair{0x38u, 0u}, {0x3c, 1}, {0x40, extended(c.i16(f.at(0x58)))},
                {0x44, 0x14}, {0x48, 0}, {0x4c, 1}, {0x50, 0}, {0x54, f.at(0x5c)}})
            c.put32(f.at(offset), value);
        c.set({{4, address}, {5, 4}, {6, finalists}, {7, extended(c.i16(f.at(0x5a)))},
               {8, league.definition()}, {9, schedule}, {10, extended(league.year())}});
        c.call(0x104bc3c0, 0x106f2424);
    }
    c.put32(league.stages(), cup);
    c.set(3, c.u32(league.stages()));
    if (compare_logical(c, c.r(3)) == 0) {
        assertion(c, f, 0x106f2450, 0x70, 0x60, 0x22b);
        return;
    }
    c.set(4, 0); c.call(0x1047ef50, 0x106f2498);
    p.call(0x10775170, 0x106f24a0, {schedule});
    p.call(0x10775170, 0x106f24a8, {finalists});
    league.last_stage() = 0;
}

void champion(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f2810);
    Frame f(c, 0x106f2810, 0x40, {});
    const auto cup = c.u32(Competition{c, address}.stages());
    c.set(3, cup);
    Program{c}.virtual_call(cup, 0x38, 0x106f2830);
    c.call(0x10517310, 0x106f2838);
}

void seed_clubs(Cpu& c, std::uint32_t address) {
    observe_entry(c, 0x106f2850);
    Frame f(c, 0x106f2850, 0xa0, {.first_gpr = 27});
    Kept<27> seed(c, c.toc(slot::flag)); Kept<28> position(c, c.toc(slot::message));
    keep(c, {{29, c.toc(slot::pool)}, {30, address}});
    Program p{c}; const Competition league{c, address};
    if (compare_logical(c, league.fixtures()) == 0) {
        assertion(c, f, 0x106f2890, 0x68, 0x40, 0x303); return;
    }
    Kept<31> order(c, p.call(0x10775130, 0x106f28dc, {std::uint32_t(std::int32_t(league.club_count())) * 4}));
    if (compare(c, order) == 0) { assertion(c, f, 0x106f28f8, 0x48, 0x38, 0x30b); return; }
    std::uint32_t copied = 0;
    while (compare(c, std::int16_t(copied), league.club_count()) < 0) {
        c.put32(std::uint32_t(order) + copied * 4, c.u32(std::uint32_t(league.standings()) + copied * standing_size)); ++copied;
    }
    c.set({{4, extended(league.club_count())}, {5, copied * standing_size}, {6, copied}});
    p.call(0x10477b40, 0x106f2974, {order});
    const auto assign = [&](std::uint32_t at, std::uint32_t value, std::uint32_t back) {
        p.call(0x10457490, back, {league.fixtures(), c.u32(std::uint32_t(order) + at * 4), value});
    };
    assign(0, 1, 0x106f298c); assign(1, 2, 0x106f29a4);
    assign(2, 3, 0x106f29bc); position = 4; assign(3, 3, 0x106f29d8); seed = 5;
    while (compare(c, result(c, extended(std::int16_t(std::uint32_t(seed)))), league.club_count()) <= 0) {
        const auto at = extended(std::int16_t(std::uint32_t(position)));
        position = std::uint32_t(position) + 1;
        assign(at, std::uint32_t(seed) & 255, 0x106f29fc); seed = std::uint32_t(seed) + 1;
    }
    p.call(0x10775170, 0x106f2a18, {order});
}

}
