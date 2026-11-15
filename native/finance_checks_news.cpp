// FINANCE.CPP checker cases for the news block (finance_news.hpp): the nine posters and the nine texts of the
// board and stadium stories.
//
// The posters run over the career's real club records; a case changes the words they read (a club's second
// recipient, its stadium pointer, the sample-table cursor and base the draws use). The news desk and the item
// routines run for real. The texts read an item built in the scratch area (a kind at +0, the word n at +8+4n, as
// 0x102bdf90 and 0x102cb4b0 lay it out) and write into a scratch text object; the formatters, the amount routines,
// the text and link routines run for real, so the strings and link rows are compared byte for byte.
#include "finance_checks.hpp"
#include <string>

namespace cm::aot::finance_checks {
namespace {
constexpr std::uint32_t board_event = 0x10101ff0, board_event_text = 0x101020c0, stand_closed = 0x10102590,
                        stand_closed_text = 0x10102660, expansion = 0x10102820, expansion_text = 0x10102910,
                        financial_crisis = 0x10102a70, financial_crisis_text = 0x10102b50, dividend = 0x10102c90,
                        dividend_text = 0x10102d20, stadium_works = 0x10102e00, stadium_works_text = 0x10102ea0,
                        stadium_link = 0x101030c0, relocation_text = 0x10103180, receivers_appeased = 0x10103270,
                        receivers_appeased_text = 0x101032e0, competition_stadium = 0x10103380,
                        competition_stadium_text = 0x10103420;
constexpr std::uint32_t item_at = scratch_base + 0x9000, out_at = scratch_base + 0xa000;
constexpr std::uint32_t club_reputation = 0x1ec;

std::string number(std::int64_t value) { return std::to_string(value); }

// A text case: the item of `kind` with the given words, the headline (`body` 0) or the body text.
Case text_case(std::uint32_t entry, std::uint32_t kind, bool body, std::initializer_list<std::uint32_t> words,
               const std::string& label, std::vector<Write> memory = {}) {
    Case c{.entry = entry, .registers = {{4, item_at}, {5, body ? 1u : 0u}, {6, out_at}}, .memory = std::move(memory),
           .label = std::string(body ? "body: " : "headline: ") + label};
    c.memory.push_back({4, item_at, kind});
    unsigned n = 0;
    for (const auto word : words) c.memory.push_back({4, item_at + 8 + 4 * n++, word});
    return c;
}
}

FINANCE_CASES("news posters") {
    if (scenario.match) return;
    const auto clubs = scenario.global(-0x7b50);
    const std::uint32_t indexes[] = {0, 3, clubs / 2, clubs - 1};
    const auto club = [&](unsigned slot) { return scenario.club(indexes[slot]); };
    const std::uint32_t cursor_cell = scenario.toc(-0x7b84), base_cell = scenario.toc(-0x7b90);
    const std::uint32_t last_entry = scenario.global(-0x7b88);
    const auto stadium_word = [&](std::uint32_t at) { return scenario.r32(at + 0x3c); };

    // The board's money events: every event, the second recipient present or not.
    for (const std::uint32_t event : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 0xffu, 0x102u, 0x107u, 0xfffffff9u}) {
        for (const std::uint32_t recipient : {0u, club(3)}) {
            if (recipient && event != 2 && event != 7 && event != 0x102 && event != 0x107 && event != 3) continue;
            cases.push_back({.entry = board_event, .registers = {{4, club(1)}, {5, 1'500'000}, {6, 0x80000001u}, {7, event}},
                             .memory = {{4, club(1) + 0x2c, recipient}},
                             .label = "board event " + number(std::int8_t(event)) + " recipient " + (recipient ? "yes" : "no")});
        }
    }
    cases.push_back({.entry = board_event, .registers = {{4, club(2)}, {5, 0}, {6, 0xffffffffu}, {7, 2}},
                     .memory = {{4, club(2) + 0x2c, club(0)}}, .label = "board event 2, recipient is a club, real desk",
                     .answers = {}});

    // A stand closed: with and without a stadium record.
    for (const std::uint32_t stadium : {0u, stadium_word(club(1))})
        for (const std::uint32_t seats : {0u, 1200u, 0xffffu, 0x18000u})
            for (const std::uint32_t terracing : {0u, 1u, 0x80u, 0x1ffu})
                cases.push_back({.entry = stand_closed, .registers = {{4, club(1)}, {5, seats}, {6, terracing}},
                                 .memory = {{4, club(1) + 0x3c, stadium}},
                                 .label = "stand closed, stadium " + std::string(stadium ? "yes" : "no") + " seats " + number(seats) +
                                          " terracing " + number(terracing)});

    // Expansion news: no club; the fixed second word; the draw (the cursor in the table, at its end so that the
    // generator is asked again, and the base of every sign and parity).
    cases.push_back({.entry = expansion, .registers = {{4, 0}, {5, 7}, {6, 2}}, .label = "expansion, no club"});
    for (const std::uint32_t second : {2u, 0x10002u, 0u, 1u, 3u, 0xfffeu})
        cases.push_back({.entry = expansion, .registers = {{4, club(1)}, {5, 0x12345u + second}, {6, second}},
                         .label = "expansion, second " + number(second)});
    for (const std::uint32_t base : {0u, 1u, 2u, 0x7fffffffu, 0x80000000u, 0x80000001u, 0xffffffffu, 0xfffffffeu})
        cases.push_back({.entry = expansion, .registers = {{4, club(0)}, {5, 0x8000u}, {6, 0}},
                         .memory = {{4, base_cell, base}}, .label = "expansion, base " + number(base)});
    cases.push_back({.entry = expansion, .registers = {{4, club(0)}, {5, 5}, {6, 0}}, .memory = {{4, cursor_cell, last_entry}},
                     .label = "expansion, draw past the table"});
    cases.push_back({.entry = expansion, .registers = {{4, club(0)}, {5, 5}, {6, 2}}, .memory = {{4, cursor_cell, last_entry}},
                     .label = "expansion, fixed word, cursor at the end"});

    // Financial crisis: the draw in every state.
    cases.push_back({.entry = financial_crisis, .registers = {{4, club(2)}}, .label = "financial crisis"});
    for (const std::uint32_t base : {0u, 1u, 0x7fffffffu, 0x80000000u, 0x80000001u, 0xffffffffu, 0xfffffffeu})
        cases.push_back({.entry = financial_crisis, .registers = {{4, club(3)}}, .memory = {{4, base_cell, base}},
                         .label = "financial crisis, base " + number(base)});
    cases.push_back({.entry = financial_crisis, .registers = {{4, club(1)}}, .memory = {{4, cursor_cell, last_entry}},
                     .label = "financial crisis, draw past the table"});

    // Dividends: the byte of the club record the text tests.
    for (const std::uint32_t flag : {0u, 1u, 0x7fu, 0x80u, 0xffu})
        for (const std::uint32_t amount : {0u, 4'000'000u, 0xffffffffu})
            cases.push_back({.entry = dividend, .registers = {{4, club(1)}, {5, amount}}, .memory = {{1, club(1) + 0x241, flag}},
                             .label = "dividend " + number(amount) + " flag " + number(flag)});

    // Stadium works: every kind, a count or a stadium index.
    for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 0x80u, 0xffu, 0x104u})
        for (const std::uint32_t quantity : {0u, 12'000u, 0xffffffffu})
            cases.push_back({.entry = stadium_works, .registers = {{4, club(2)}, {5, quantity}, {6, kind}},
                             .label = "stadium works kind " + number(std::int8_t(kind)) + " quantity " + number(quantity)});

    // A stadium link: no club, no record, a stadium pointer or none (the original then reads the word at 0x10).
    cases.push_back({.entry = stadium_link, .registers = {{4, 0}, {5, club(0)}}, .label = "stadium link, no club"});
    cases.push_back({.entry = stadium_link, .registers = {{4, club(1)}, {5, 0}}, .label = "stadium link, no record"});
    cases.push_back({.entry = stadium_link, .registers = {{4, club(1)}, {5, club(2)}}, .label = "stadium link"});
    cases.push_back({.entry = stadium_link, .registers = {{4, club(1)}, {5, club(2)}}, .memory = {{4, club(1) + 0x3c, 0}},
                     .label = "stadium link, no stadium (address 0x10 is unmapped: both sides fault at the same read)"});

    // Receivers appeased and the competition story: the club by its index.
    for (const auto index : indexes) {
        cases.push_back({.entry = receivers_appeased, .registers = {{4, index}}, .label = "receivers appeased, club " + number(index)});
        for (const std::uint32_t redevelop : {0u, 1u, 2u, 0x101u, 0xffu})
            cases.push_back({.entry = competition_stadium, .registers = {{4, index}, {5, 7}, {6, redevelop}},
                             .label = "competition stadium, club " + number(index) + " redevelop " + number(redevelop)});
    }
    // The caller's callee-saved registers reach the callees' frames: one case of each shape with them set.
    for (const auto entry : {board_event, stand_closed, expansion, financial_crisis, dividend, stadium_works,
                             receivers_appeased, competition_stadium, stadium_link}) {
        const bool by_index = entry == receivers_appeased || entry == competition_stadium;
        Case c{.entry = entry, .registers = {{4, by_index ? indexes[1] : club(1)}, {5, entry == stadium_link ? club(2) : 5u}, {6, 2}, {7, 2}},
               .label = "caller registers"};
        for (unsigned r = 13; r < 32; ++r) c.registers.push_back({r, 0xc0de0000u + r});
        cases.push_back(std::move(c));
    }
}

FINANCE_CASES("news texts") {
    if (scenario.match) return;
    const auto clubs = scenario.global(-0x7b50);
    const std::uint32_t indexes[] = {0, 3, clubs / 2, clubs - 1};
    const auto club = [&](unsigned slot) { return scenario.club(indexes[slot]); };
    const auto stadiums_cell = scenario.toc(-0x7548);
    const auto stadium = [&](std::uint32_t index) { return scenario.r32(stadiums_cell) + index * 0x50; };

    // ---- the board's money events (kind 0x1779): words are the club, the injection, the budget and the event.
    for (const bool body : {false, true})
        for (const std::uint32_t event : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 0xffffffffu, 0x80000000u})
            cases.push_back(text_case(board_event_text, 0x1779, body, {indexes[1], 2'500'000, 1'200'000, event},
                                      "board event " + number(std::int32_t(event))));
    // The budget against the club's reputation (limit = reputation squared by a scale) and against 50000.
    for (const std::int32_t reputation : {0, 1, 100, 3000, 9000, 32767, -1, -32768})
        for (const std::int32_t budget : {-5, 0, 1, 49'999, 50'000, 50'001, 5'000'000, 0x7fffffff, std::int32_t(0x80000000)})
            for (const std::int32_t injection : {0, 49'999, 50'001})
                for (const std::uint32_t event : {1u, 2u}) {
                    if (event == 1 && injection != 0) continue;
                    cases.push_back(text_case(board_event_text, 0x1779, true,
                                              {indexes[1], std::uint32_t(injection), std::uint32_t(budget), event},
                                              "event " + number(event) + " reputation " + number(reputation) + " budget " + number(budget) +
                                                  " injection " + number(injection),
                                              {{2, club(1) + club_reputation, std::uint32_t(reputation) & 0xffff}}));
                }
    for (const std::uint32_t amount : {0u, 1u, 999u, 1'000'000u, 0xffffffffu, 0x80000000u})
        for (const bool body : {false, true})
            for (const std::uint32_t event : {3u, 4u, 5u, 6u, 7u})
                cases.push_back(text_case(board_event_text, 0x1779, body, {indexes[2], amount, amount ^ 0x5555, event},
                                          "event " + number(event) + " amount " + number(amount)));

    // ---- a stand closed (kind 0x177a): the stadium (-1: none) and its row, the seats and whether it is a stand.
    for (const bool body : {false, true})
        for (const std::uint32_t index : {0xffffffffu, 0u, 1u, 7u, 0xfffffffeu})
            for (const std::uint32_t flag : {0u, 1u, 0x100u, 0xffu})
                for (const std::uint32_t standing : {0u, 1u}) {
                    cases.push_back(text_case(stand_closed_text, 0x177a, body, {0, index, 600, flag},
                                              "stand closed, stadium " + number(std::int32_t(index)) + " flag " + number(flag) +
                                                  " standing " + number(standing),
                                              {{4, stadium(index) + 8, standing}, {4, stadium(index) + 4, 40'000}}));
                }
    for (const bool body : {false, true})
        // A null table: the row is then null and the terracing text takes its "closed" wording (a stand would read
        // the row's word at +8, a fault on both sides).
        cases.push_back(text_case(stand_closed_text, 0x177a, body, {0, 0, 600, 0}, "terracing, no stadium table",
                                  {{4, stadiums_cell, 0}}));
    for (const std::uint32_t seats : {0u, 41'000u, 0xffffffffu})
        cases.push_back(text_case(stand_closed_text, 0x177a, true, {0, 2, seats, 0}, "terracing, seats " + number(seats),
                                  {{4, stadium(2) + 4, 40'000}}));

    // ---- expansion plans shelved (the kind and reason words 1 and 2).
    for (const bool body : {false, true})
        for (const std::int32_t kind : {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x7fffffff})
            for (const std::int32_t reason : {-3, -1, 0, 1, 2, 3})
                cases.push_back(text_case(expansion_text, 0x177e, body, {0, std::uint32_t(kind), std::uint32_t(reason)},
                                          "expansion text kind " + number(kind) + " reason " + number(reason)));

    // ---- a financial crisis (word 0 the club, 1 the state).
    for (const bool body : {false, true})
        for (const std::uint32_t index : indexes)
            for (const std::uint32_t state : {0u, 1u, 2u, 0xffffffffu, 0x10000u, 0x10001u, 0x8000u})
                cases.push_back(text_case(financial_crisis_text, 0x177b, body, {index, state},
                                          "crisis, club " + number(index) + " state " + number(state)));

    // ---- dividends (word 1 the total, 2 the flag the text tests).
    for (const bool body : {false, true})
        for (const std::uint32_t total : {0u, 1u, 2'000'000u, 0xffffffffu, 0x80000000u})
            for (const std::uint32_t flag : {0u, 1u, 0x80u, 0x100u, 0xffffffffu})
                cases.push_back(text_case(dividend_text, 0x177c, body, {0, total, flag},
                                          "dividend " + number(total) + " flag " + number(flag)));

    // ---- stadium works (word 1 a count or a stadium index, 2 the kind).
    for (const bool body : {false, true})
        for (const std::uint32_t quantity : {0u, 12'000u, 3u, 0xffffffffu})
            for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 0x80u, 0xffu, 0x100u, 0x104u, 0xfffffffeu})
                cases.push_back(text_case(stadium_works_text, 0x177d, body, {0, quantity, kind},
                                          "works kind " + number(std::int32_t(kind)) + " quantity " + number(quantity)));
    for (const std::uint32_t index : {0u, 1u, 5u, 9u})
        for (const std::uint32_t kind : {4u, 5u, 6u})
            for (const std::uint32_t gender : {0u, 1u, 0xffu})
                cases.push_back(text_case(stadium_works_text, 0x177d, true, {0, index, kind},
                                          "works of stadium " + number(index) + " kind " + number(kind) + " gender " + number(gender),
                                          {{1, stadium(index) + 0x4b, gender}}));

    // ---- club relocation (word 0 the club, 2 the stadium).
    for (const bool body : {false, true})
        for (const std::uint32_t index : indexes)
            for (const std::uint32_t where : {0u, 3u, 11u})
                cases.push_back(text_case(relocation_text, 0x1781, body, {index, 9, where},
                                          "relocation, club " + number(index) + " stadium " + number(where),
                                          {{1, stadium(where) + 0x4b, where & 1}}));

    // ---- receivers appeased (word 0 the club).
    for (const bool body : {false, true})
        for (const std::uint32_t index : indexes)
            cases.push_back(text_case(receivers_appeased_text, 0x1783, body, {index}, "appeased, club " + number(index)));

    // ---- the competition story (word 1 the competition, 2 enlarge or redevelop).
    for (const bool body : {false, true})
        for (const std::uint32_t competition : {0u, 1u, 5u, 20u})
            for (const std::uint32_t enlarge : {0u, 1u, 2u, 0xffu, 0x100u, 0x101u, 0xffffffffu})
                cases.push_back(text_case(competition_stadium_text, 0x1780, body, {0, competition, enlarge},
                                          "competition story " + number(competition) + " enlarge " + number(std::int32_t(enlarge))));

    // The caller's callee-saved registers reach the callees' frames: one case of each text with them set.
    struct Shape { std::uint32_t entry; std::uint32_t kind; std::initializer_list<std::uint32_t> words; };
    const Shape shapes[] = {{board_event_text, 0x1779, {1, 400'000, 90'000, 2}}, {board_event_text, 0x1779, {1, 400'000, 90'000, 1}},
                            {stand_closed_text, 0x177a, {0, 1, 600, 1}}, {expansion_text, 0x177e, {0, 2, 1}},
                            {financial_crisis_text, 0x177b, {1, 1}}, {dividend_text, 0x177c, {0, 100, 1}},
                            {stadium_works_text, 0x177d, {0, 1, 5}}, {relocation_text, 0x1781, {1, 0, 1}},
                            {receivers_appeased_text, 0x1783, {1}}, {competition_stadium_text, 0x1780, {0, 1, 1}}};
    for (const auto& shape : shapes)
        for (const bool body : {false, true}) {
            auto c = text_case(shape.entry, shape.kind, body, shape.words, "caller registers");
            for (unsigned r = 13; r < 32; ++r) c.registers.push_back({r, 0xc0de0000u + r});
            cases.push_back(std::move(c));
        }
}
}
