#include "bra_reg_central_detail.hpp"
#include <array>

namespace cm::aot::bra_reg_central {
using namespace detail;

void advance_stage(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x106d8800, 0x70, {.first_gpr = 30});
    keep(c, {{31, c.toc(slot::pool)}, {30, c.toc(slot::message)}});
    const Competition league{c, address};
    const std::uint32_t count = league.stage_count().get(), last = league.last_stage().get();
    c.set({{4, count}, {5, last}});
    if (compare(c, std::int32_t(last), std::int32_t(count - 1)) >= 0) {
        assertion(c, f, 0x106d8844, 0x40, 0x38, 0x1f9);
        c.set(3, c.toc(slot::flag));
        return;
    }
    league.last_stage() = std::int32_t(last + 1);
    if (compare(c, league.last_stage()) == 0) {
        enter(c, 0x106d88a4, {address});
        build_knockout(c, address);
    }
}

void build_knockout(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x106d88c0, 0xe0, {.first_gpr = 25});
    keep(c, {{27, address}, {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    Program p{c}; const Competition league{c, address};
    const auto bracket = c.toc(slot::bracket);
    const auto bracket_head = c.u32(bracket);
    c.put32(f.at(0x6c), bracket_head); c.put32(f.at(0x70), c.u32(bracket + 4));
    c.set({{4, bracket_head}, {5, bracket}});
    Kept<28> seats(c, p.call(0x10775130, 0x106d88fc, {0x20}));
    if (compare(c, seats) == 0) {
        assertion(c, f, 0x106d8918, 0x94, 0x64, 0x21c); return;
    }
    // Quarter-final seats: league positions in the original's written order
    // (the same pairing as the bracket bytes copied above).
    static constexpr std::array<std::uint32_t, 8> seeded{0, 7, 2, 5, 6, 1, 4, 3};
    for (unsigned seat = 0; seat < seeded.size(); ++seat)
        c.put32(std::uint32_t(seats) + seat * 4, c.u32(std::uint32_t(league.standings()) + seeded[seat] * standing_size));
    c.set({{3, address}, {4, 0}, {5, f.at(0x5a)}, {6, f.at(0x58)}, {7, 0}, {8, league.standings()}});
    Kept<26> schedule(c, p.virtual_call(address, 0x44, 0x106d89d8));
    Kept<25> cup(c, p.call(0x10773520, 0x106d89ec, {0xb4}));
    if (compare(c, cup) != 0) {
        const auto extra = extended(c.i16(f.at(0x58)));
        for (const auto [offset, value] : {std::pair{0x38u, 0u}, {0x3c, 1}, {0x40, extra}, {0x44, 0x14},
                 {0x48, 0}, {0x4c, 1}, {0x50, 0}, {0x54, f.at(0x6c)}})
            c.put32(f.at(offset), value);
        c.set({{4, address}, {5, 8}, {6, seats}, {7, extended(c.i16(f.at(0x5a)))}, {8, league.definition()},
               {9, schedule}, {10, extended(league.year())}});
        c.call(0x104bc3c0, 0x106d8a48);
    }
    c.put32(league.stages(), cup);
    c.set(3, c.u32(league.stages()));
    if (compare_logical(c, c.r(3)) == 0) {
        assertion(c, f, 0x106d8a74, 0x74, 0x5c, 0x234); return;
    }
    c.set(4, 0); c.call(0x1047ef50, 0x106d8abc);
    p.call(0x10775170, 0x106d8ac4, {schedule});
    p.call(0x10775170, 0x106d8acc, {seats});
    league.last_stage() = 0;
}

std::uint32_t record_result(Cpu& c, std::uint32_t address, std::uint32_t club, std::uint32_t event,
                            std::uint32_t stage, std::uint32_t match) {
    Frame f(c, 0x106d8af0, 0x50, {.first_gpr = 27});
    Kept<31> history(c, c.toc(slot::history));
    keep(c, {{27, address}, {28, club}, {29, match}});
    Program p{c}; const Competition league{c, address};
    const auto outcome = std::int32_t(std::int8_t(event));
    if (compare(c, std::int8_t(stage), -1) == 0) {
        if (compare(c, outcome, 6) == 0) {
            c.set({{6, 0}, {5, league.definition()}, {7, 0x82}, {8, 0x32}});
            p.call(0x10384500, 0x106d8b40, {c.u32(history)});
        } else if (compare(c, outcome, 3) == 0) {
            c.set(5, league.definition());
            p.call(0x10382e50, 0x106d8b58, {c.u32(history)});
        }
        return 0;
    }
    if (compare(c, std::int8_t(stage)) != 0) return 0;
    c.set(5, 0);
    Kept<30> row(c, 0);
    std::uint32_t offset = 0;
    while (compare(c, row, league.club_count()) < 0) {
        c.set(4, league.standings());
        if (compare_logical(c, c.u32(c.r(4) + offset), club) == 0) {
            const auto mark = [&](std::uint32_t value) {
                c.set(4, std::uint32_t(league.standings()) + std::uint32_t(row) * standing_size);
                c.set(5, value);
                c.put8(c.r(4) + 0x40, value);
            };
            if (compare(c, outcome, 1) == 0) {
                c.set({{4, club}, {5, league.definition()}});
                p.call(0x10381b70, 0x106d8ba0, {c.u32(history)});
                mark(5);
                return 0;
            }
            if (compare(c, outcome, 2) == 0 || compare(c, outcome, -1) == 0) {
                c.set(5, extended(c.i16(match + 0x34)));
                c.set({{4, 0x32}, {6, 3}});
                const auto sampled = p.call(0x10476780, 0x106d8be0, {0x1e});
                c.set({{8, sampled}, {5, league.definition()}, {4, club},
                       {6, extended(c.i16(match + 0x32))}, {7, extended(c.i16(match + 0x30))}});
                p.call(0x103846e0, 0x106d8bfc, {c.u32(history)});
                mark(0xfffffffe);
                return 0;
            }
        }
        offset += standing_size; c.set(5, offset);
        row = std::uint32_t(row) + 1;
    }
    return 0;
}

void announce(Cpu& c, std::uint32_t address, std::uint32_t club, std::uint32_t event, std::uint32_t stage) {
    Frame f(c, 0x106d8c50, 0x60, {.first_gpr = 27});
    Kept<30> text(c, c.toc(slot::text));
    Kept<31> clubs(c, c.toc(slot::clubs));
    keep(c, {{27, address}, {28, club}});
    const auto pool = c.toc(slot::pool);
    c.set(10, pool);
    Program p{c}; const Competition league{c, address};
    const auto outcome = std::int32_t(std::int8_t(event));
    const auto stage_id = std::int32_t(std::int8_t(stage));
    compare(c, stage_id, -1);
    const auto importance = c.u8(f.at(0x9b)), personal = c.u8(f.at(0x9f));
    Kept<29> news(c, c.u32(f.at(0xa0)));
    const std::uint32_t club_offset = club * club_size;
    const auto club_form = [&](std::uint32_t offset) {
        return std::uint32_t(std::int32_t(c.i8(c.u32(clubs) + offset + 0x23b)));
    };
    // Base-class news: the game's generic message for this result.
    const auto forward = [&](std::uint32_t back) {
        c.put32(f.at(0x38), importance); c.set(10, 0);
        c.put32(f.at(0x3c), personal); c.put32(f.at(0x40), news);
        c.call(0x10450030, back);
    };
    // "{club} ... the {competition} ...", with each name's two format arguments.
    const auto format_with_competition = [&](unsigned format, std::uint32_t offset, std::uint32_t back) {
        const auto definition = std::uint32_t(league.definition());
        const auto competition_form = std::uint32_t(std::int32_t(c.i8(definition + 0x67)));
        const auto form = club_form(offset);
        c.set({{4, pool + format}, {5, form}, {6, form}, {7, competition_form}, {8, competition_form},
               {9, c.u32(clubs) + offset + 0x221}, {10, definition + 0x1a}});
        p.call(0x10727890, back, {text});
    };
    const auto link_club = [&](std::uint32_t offset, std::uint32_t back) {
        const auto club_record = c.u32(clubs) + offset;
        c.set({{4, club_record + 0x221}, {5, 0x7d5}, {6, club_record}});
        p.call(0x1074bab0, back, {news});
    };
    if (stage_id == -1) {
        if (compare(c, outcome, 6) != 0) { forward(0x106d8d64); return; }
        if (compare(c, std::int8_t(personal)) != 0) {
            format_with_competition(pool_text::qualified, club_offset, 0x106d8ce0);
        } else {
            const auto form = club_form(club_offset);
            c.set({{4, pool + pool_text::qualified_short}, {5, form}, {6, form},
                   {7, c.u32(clubs) + club_offset + 0x221}});
            p.call(0x10727890, 0x106d8d10, {text});
        }
        c.set(4, text); p.call(0x1074b830, 0x106d8d1c, {news});
        link_club(club_offset, 0x106d8d38);
        const auto definition = std::uint32_t(league.definition());
        c.set({{4, definition + 0x1a}, {5, 0x7d0}, {6, definition}});
        p.call(0x1074bab0, 0x106d8d4c, {news});
        return;
    }
    if (compare(c, stage_id) != 0) return;
    if (compare(c, std::int8_t(personal)) != 0) { forward(0x106d8d8c); return; }
    if (compare(c, outcome, 1) != 0) { forward(0x106d8e18); return; }
    keep(c, {{28, club_offset}});
    format_with_competition(pool_text::champions, club_offset, 0x106d8ddc);
    c.set(4, text); p.call(0x1074b830, 0x106d8de8, {news});
    link_club(club_offset, 0x106d8e00);
}

void champion(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x106d8e30, 0x40, {});
    Program p{c};
    const auto first_stage = c.u32(c.u32(address + 0xc));
    c.set(3, first_stage);
    p.virtual_call(first_stage, 0x38, 0x106d8e50);
    c.call(0x10517310, 0x106d8e58);
}

void seed_clubs(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x106d8e70, 0xa0, {.first_gpr = 27});
    Kept<27> seed(c, c.toc(slot::flag));
    Kept<28> position(c, c.toc(slot::message));
    Kept<29> pool(c, c.toc(slot::pool));
    Kept<30> object(c, address);
    Program p{c}; const Competition league{c, address};
    if (compare_logical(c, league.fixtures()) == 0) {
        assertion(c, f, 0x106d8eb0, 0x68, 0x40, 0x30c); return;
    }
    Kept<31> order(c, p.call(0x10775130, 0x106d8efc, {std::uint32_t(std::int32_t(league.club_count())) << 2}));
    if (compare(c, order) == 0) {
        assertion(c, f, 0x106d8f18, 0x48, 0x38, 0x314); return;
    }
    std::uint32_t copied = 0;
    while (compare(c, std::int16_t(copied), league.club_count()) < 0) {
        c.put32(std::uint32_t(order) + copied * 4, c.u32(std::uint32_t(league.standings()) + copied * standing_size));
        ++copied;
    }
    c.set({{4, extended(league.club_count())}, {5, copied * standing_size}, {6, copied}});
    p.call(0x10477b40, 0x106d8f94, {order});
    const auto assign = [&](std::uint32_t at, std::uint32_t value, std::uint32_t back) {
        c.set({{4, c.u32(std::uint32_t(order) + at * 4)}, {5, value}});
        p.call(0x10457490, back, {league.fixtures()});
    };
    // The first eight share seeds as a knockout bracket does; the rest count up from 9.
    assign(0, 1, 0x106d8fac); assign(1, 2, 0x106d8fc4); assign(2, 3, 0x106d8fdc); assign(3, 3, 0x106d8ff4);
    assign(4, 5, 0x106d900c); assign(5, 5, 0x106d9024); assign(6, 5, 0x106d903c);
    position = 8;
    assign(7, 5, 0x106d9058);
    seed = 9;
    while (compare(c, std::int16_t(std::uint32_t(seed)), league.club_count()) <= 0 &&
           compare(c, std::int16_t(std::uint32_t(position)), league.club_count()) < 0) {
        const auto at = std::uint32_t(std::int32_t(std::int16_t(std::uint32_t(position))));
        const auto value = std::uint32_t(seed) & 0xff;
        position = std::uint32_t(position) + 1;
        assign(at, value, 0x106d907c);
        seed = std::uint32_t(seed) + 1;
    }
    p.call(0x10775170, 0x106d90a4, {order});
}

void fixture_times(Cpu& c, std::uint32_t address, std::uint32_t stage, std::uint32_t weekday,
                   std::uint32_t first_time, std::uint32_t second_time) {
    Frame f(c, 0x106d90c0, 0x50, {.first_gpr = 28});
    keep(c, {{28, stage}, {29, weekday}, {30, first_time}, {31, second_time}});
    Program p{c};
    const auto fixture = p.call(0x10459b50, 0x106d90f4, {c.u32(address + 8)});
    if (compare(c, std::int8_t(stage), -1) == 0) {
        if (compare_logical(c, first_time & 0xff, 9) < 0) first_time = 1;
        if (compare_logical(c, second_time & 0xff, 9) < 0) second_time = 5;
    }
    c.put8(fixture + 0x76, weekday); c.put8(fixture + 0x77, first_time); c.put8(fixture + 0x78, second_time);
}
}
