#include "bra_reg_gaucho_detail.hpp"
namespace cm::aot::bra_reg_gaucho {
using namespace detail;
void announce(Cpu& c, std::uint32_t address, std::uint32_t club, std::uint32_t event, std::uint32_t stage) {
    observe_entry(c, 0x106f2630);
    Frame f(c, 0x106f2630, 0x60, {.first_gpr = 27});
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
        if (compare(c, outcome, 6) != 0) { forward(0x106f2744); return; }
        if (compare(c, std::int8_t(personal)) != 0) {
            format_with_competition(pool_text::qualified, club_offset, 0x106f26c0);
        } else {
            const auto form = club_form(club_offset);
            c.set({{4, pool + pool_text::qualified_short}, {5, form}, {6, form},
                   {7, c.u32(clubs) + club_offset + 0x221}});
            p.call(0x10727890, 0x106f26f0, {text});
        }
        c.set(4, text); p.call(0x1074b830, 0x106f26fc, {news});
        link_club(club_offset, 0x106f2718);
        const auto definition = std::uint32_t(league.definition());
        c.set({{4, definition + 0x1a}, {5, 0x7d0}, {6, definition}});
        p.call(0x1074bab0, 0x106f272c, {news});
        return;
    }
    if (compare(c, stage_id) != 0) return;
    if (compare(c, std::int8_t(personal)) != 0) { forward(0x106f276c); return; }
    if (compare(c, outcome, 1) != 0) { forward(0x106f27f8); return; }
    keep(c, {{28, club_offset}});
    format_with_competition(pool_text::champions, club_offset, 0x106f27bc);
    c.set(4, text); p.call(0x1074b830, 0x106f27c8, {news});
    link_club(club_offset, 0x106f27e0);
}

std::uint32_t record_result(Cpu& c, std::uint32_t address, std::uint32_t club, std::uint32_t event,
                            std::uint32_t stage, std::uint32_t match) {
    observe_entry(c, 0x106f24d0);
    Frame f(c, 0x106f24d0, 0x50, {.first_gpr = 27});
    Kept<31> history(c, c.toc(slot::history));
    keep(c, {{27, address}, {28, club}, {29, match}});
    Program p{c}; const Competition league{c, address};
    const auto outcome = std::int32_t(std::int8_t(event));
    if (compare(c, std::int8_t(stage), -1) == 0) {
        if (compare(c, outcome, 6) == 0) {
            c.set({{6, 0}, {5, league.definition()}, {7, 0x82}, {8, 0x32}});
            p.call(0x10384500, 0x106f2520, {c.u32(history)});
        } else if (compare(c, outcome, 3) == 0) {
            c.set(5, league.definition());
            p.call(0x10382e50, 0x106f2538, {c.u32(history)});
        }
        return 0;
    }
    if (compare(c, std::int8_t(stage)) != 0) return 0;
    c.set({{3, std::uint32_t(outcome)}, {5, 0}});
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
                p.call(0x10381b70, 0x106f2580, {c.u32(history)});
                mark(5);
                return 0;
            }
            if (compare(c, outcome, 2) == 0 || compare(c, outcome, -1) == 0) {
                c.set(5, extended(c.i16(match + 0x34)));
                c.set({{4, 0x32}, {6, 2}});
                const auto sampled = p.call(0x10476780, 0x106f25c0, {0x1e});
                c.set({{8, sampled}, {5, league.definition()}, {4, club},
                       {6, extended(c.i16(match + 0x32))}, {7, extended(c.i16(match + 0x30))}});
                p.call(0x103846e0, 0x106f25dc, {c.u32(history)});
                mark(0xfffffffe);
                return 0;
            }
        }
        offset += standing_size; c.set(5, offset);
        row = std::uint32_t(row) + 1;
    }
    return 0;
}

}
