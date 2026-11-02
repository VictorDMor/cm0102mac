#include "bra_reg_parana_detail.hpp"

namespace cm::aot::bra_reg_parana {
using namespace detail;

std::uint32_t handle_fixture(Cpu& c, std::uint32_t address, std::uint32_t fixture, std::uint32_t context) {
    observe_entry(c, 0x106f8a60);
    Frame f(c, 0x106f8a60, 0x50, {.first_gpr = 28});
    keep(c, {{30, context}, {29, fixture}, {28, address}});
    Program p{c}; const Competition league{c, address};
    c.call(0x104a3930, 0x106f8a8c);
    if (compare(c, c.i8(fixture + 0x42), -1) == 0) {
        c.put8(fixture + 0x42, compare(c, c.i16(fixture + 0x34), 11) < 0 ? 0 : 1);
        while (compare(c, result(c, std::uint32_t(league.last_stage().get())), c.i8(fixture + 0x42)) < 0) {
            const auto turn = std::uint32_t(std::int32_t(std::int8_t(c.r(3) + 1)));
            c.set({{3, address}, {4, turn}});
            p.owned(0x106f8b50, 0x106f8ad4, [&] { build_turn(c, address, turn); });
        }
        Kept<31> fixtures(c, league.fixtures());
        // The delegated turn must see the parent's fixture pointer temporarily
        // detached; restore it and the fixture's sentinel only after it returns.
        league.fixtures() = 0;
        const auto turn = c.u32(std::uint32_t(league.stages()) + std::uint32_t(std::int32_t(c.i8(fixture + 0x42))) * 4);
        c.set({{3, turn}, {4, fixture}, {5, context}});
        p.virtual_call(turn, 0xc, 0x106f8b1c);
        league.fixtures() = fixtures; c.put8(fixture + 0x42, missing);
    }
    return 1;
}

void build_turn(Cpu& c, std::uint32_t address, std::uint32_t turn) {
    observe_entry(c, 0x106f8b50);
    Frame f(c, 0x106f8b50, 0xd0, {.first_gpr = 24});
    Kept<26> settings(c, c.toc(slot::message)); Kept<25> matches(c, c.toc(slot::pool));
    keep(c, {{29, address}, {30, turn}});
    Program p{c}; const Competition league{c, address};
    c.set(5, c.u16(c.r(2) - 0x361c));
    c.put16(f.at(0x78), c.r(5)); c.put16(f.at(0x7a), c.u16(c.r(2) - 0x361a));
    Kept<31> clubs(c, p.call(0x10775130, 0x106f8b8c, {std::uint32_t(std::int32_t(league.club_count())) * 4}));
    if (compare(c, clubs) == 0) {
        assertion(c, f, 0x106f8ba8, 0x84, 0x7c, 0x3d5); c.set(3, c.toc(slot::flag)); return;
    }
    std::uint32_t count = 0;
    while (compare(c, std::int16_t(count), league.club_count()) < 0) {
        c.put32(std::uint32_t(clubs) + count * 4, c.u32(std::uint32_t(league.standings()) + count * standing_size)); ++count;
    }
    c.set({{4, extended(std::int16_t(count))}, {5, count * standing_size}, {6, count}});
    Kept<24> title(c, compare(c, std::int8_t(turn)) == 0 ? 0x439 : 0x43a);
    matches = 11;
    Kept<28> stage(c, p.call(0x10773520, 0x106f8c48, {0xec}));
    if (compare(c, stage) != 0) {
        settings = address + 0xc7;
        Kept<27> date(c, f.at(0x78));
        c.put32(f.at(0x38), c.u8(address + 0xc4)); c.put32(f.at(0x3c), c.u8(address + 0xc5));
        const auto match_count = std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(matches))));
        matches = c.u8(address + 0xc6);
        c.put32(f.at(0x40), matches); c.put32(f.at(0x44), settings); c.put32(f.at(0x48), date);
        date = extended(league.year());
        for (const auto [offset, value] : {std::pair{0x4cu, std::uint32_t(date)}, {0x50, turn}, {0x54, std::uint32_t(title)},
                {0x58, 0}, {0x5c, 1}, {0x60, 0}, {0x64, 8}, {0x68, match_count}, {0x6c, 0}, {0x70, 2}})
            c.put32(f.at(offset), value);
        p.call(0x1056dc60, 0x106f8cd8, {stage, address, extended(league.club_count()), clubs, 1, league.definition(), 0, 0});
    }
    const auto index = std::uint32_t(std::int32_t(std::int8_t(turn)));
    c.set({{4, index}, {5, league.stages()}});
    c.put32(c.r(5) + index * 4, stage); league.last_stage() = std::int32_t(index);
    p.call(0x10775170, 0x106f8cf4, {clubs});
}
}
