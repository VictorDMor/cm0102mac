// COMP.CPP news block's cases (comp_news.cpp, comp_texts*.cpp), from the real career (--world): the game's
// competition objects and records, its clubs, the news manager. Items the news screens would hold are built in the
// scratch area (an item's field n is the word at +8+4n), the text object the renderings fill is two zero words
// (text pointer, link list): the real 0x1074b830 / 0x1074bab0 run on it, so the register save areas the link rows
// copy are compared too. The three item routines write news records: every branch has a case.
#include "comp_checks.hpp"
#include <cstdio>
#include <string>

namespace cm::aot::comp_checks {
namespace {
constexpr std::uint32_t prize_news = 0x10452010, year_news = 0x104527c0, movement_news = 0x10452a60, fate_text = 0x10450030,
                        position_text = 0x10450580, place_text = 0x10451ac0, prize_text = 0x10452290, money_text = 0x10452880,
                        movement_text = 0x10452bb0;
constexpr std::uint32_t news_enabled = 0x102cdbf0, fate_by_flag = 0x10469710, fate_by_stage = 0x10464d00;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60, club_count_slot = -0x7b50, rules_slot = -0x7b54,
                       records_slot = -0x7bbc, clubs_slot = -0x7be8;
// Scratch layout of this block (a case's writes are undone afterwards).
constexpr std::uint32_t out = scratch_base + 0x20000, buffer = scratch_base + 0x21000, item = scratch_base + 0x22000,
                        other = scratch_base + 0x23000, other_cell = scratch_base + 0x23100, other_word = scratch_base + 0x23200,
                        copy = scratch_base + 0x24000;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }
struct World {
    const Scenario& s;
    std::vector<std::uint32_t> objects;  // every competition object the career has
    std::uint32_t clubs = 0, club_count = 0, records = 0, record_count = 0;
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto table = s.global(competitions_slot);
        record_count = s.global(competition_count_slot);
        for (std::uint32_t id = 0; table && id < record_count && id < 4096; ++id)
            if (const auto object = s.r32(table + id * 4)) objects.push_back(object);
        clubs = s.global(clubs_slot);
        club_count = s.global(club_count_slot);
        records = s.global(records_slot);
    }
    std::uint32_t club(std::uint32_t n) const { return clubs + n * 0x248; }
    std::uint32_t record(std::uint32_t n) const { return records + n * 0x6c; }
    std::uint32_t record_of(std::uint32_t object) const { return s.r32(object + 4); }
    std::uint32_t last_stage(std::uint32_t object) const { return s.r32(object + 0x30); }
};
// The text object the renderings fill: no text yet, no link list.
std::vector<Write> empty_out() { return {{4, out, 0}, {4, out + 4, 0}}; }
// A news item's fields (field n at +8+4n).
std::vector<Write> item_fields(std::initializer_list<std::pair<unsigned, std::uint32_t>> fields) {
    std::vector<Write> writes;
    for (const auto& [n, value] : fields) writes.push_back({4, item + 8 + 4 * n, value});
    return writes;
}
// The text buffer a routine formats into: a pattern, so a stray store shows.
std::vector<Write> patterned_buffer() {
    std::vector<Write> writes;
    for (std::uint32_t offset = 0; offset < 0x200; offset += 4) writes.push_back({4, buffer + offset, 0x41424344u + offset});
    return writes;
}
}

COMP_CASES("news: items") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.objects.empty() || w.club_count < 4) return;
    const std::vector<std::pair<unsigned, std::uint32_t>> enabled = {{news_enabled, 1}}, disabled = {{news_enabled, 0}};
    // 0x10452010: the 64-bit threshold of 50 times the club's halfword +0x1ec, both sides of it, for each sign of the
    // halfword; and the news switch: real, forced on, forced off.
    for (std::size_t n = 0; n < w.objects.size() && n < 3; ++n) {
        const auto object = w.objects[n];
        const auto club = w.club(n);
        for (const std::uint32_t half : {0u, 100u, 0xff9cu, 0x7fffu, 0x8000u}) {
            const std::int64_t threshold = std::int64_t(std::int32_t(std::int16_t(half)) * 50);
            const std::int64_t amounts[] = {threshold - 1, threshold, threshold + 1, 0, -1, 0x100000000ll, -0x100000000ll,
                                            0x7fffffffffffffffll, std::int64_t(0x8000000000000000ull), 0x80000000ll, -0x80000001ll};
            for (const auto amount : amounts) {
                const auto high = std::uint32_t(std::uint64_t(amount) >> 32), low = std::uint32_t(amount);
                const std::vector<Write> memory = {{2, club + 0x1ec, half}};
                const std::string name = "news: prize " + hex(object) + " half " + hex(half) + " amount " + hex(high) + ":" + hex(low);
                cases.push_back({.entry = prize_news, .registers = {{3, object}, {4, club}, {5, high}, {6, low}, {7, 3}, {8, 0x12}, {9, 0}, {10, 1}},
                                 .memory = memory, .answers = enabled, .label = name});
                if (n == 0 && (amount == threshold || amount == -1))
                    for (const bool on : {false, true})
                        cases.push_back({.entry = prize_news, .registers = {{3, object}, {4, club}, {5, high}, {6, low}, {7, 3}, {8, 0x12}, {9, 0}, {10, 1}},
                                         .memory = memory, .answers = on ? enabled : disabled, .label = name + (on ? " forced on" : " forced off")});
            }
        }
    }
    // The real news switch of the career, the sign extensions of place (byte), round (halfword), flag (byte).
    {
        const auto object = w.objects.front(), club = w.club(1);
        for (const std::uint32_t place : {0u, 1u, 0x7fu, 0x80u, 0xffu, 0x1234567fu, 0x12345680u})
            cases.push_back({.entry = prize_news, .registers = {{3, object}, {4, club}, {5, 0}, {6, 1000000}, {7, place}, {8, place * 0x10001}, {9, 0}, {10, place ^ 0xff}},
                             .memory = {{2, club + 0x1ec, 1}}, .answers = enabled, .label = "news: prize extensions " + hex(place)});
        cases.push_back({.entry = prize_news, .registers = {{3, object}, {4, club}, {5, 0}, {6, 1000000}, {7, 2}, {8, 3}, {9, 0}, {10, 1}},
                         .memory = {{2, club + 0x1ec, 1}}, .label = "news: prize real switch"});
    }
    // The other competition's two sides: its first club the arguments' club (field 5 from +0x20) or not (from +0x1c),
    // and the flag bytes in each order, with the signed byte compares at both ends.
    {
        const auto object = w.objects.front(), club = w.club(2), other_club = w.club(3);
        const std::uint32_t flags[][2] = {{1, 2}, {2, 1}, {2, 2}, {0x80, 1}, {1, 0x80}, {0x7f, 0x80}};
        for (const bool same : {true, false})
            for (const std::uint32_t flag_a : {0u, 1u, 0x80u})  // +0x3d (same) / +0x3e (other)
                for (const auto& pair_b : flags)               // +0x4b, +0x4c
                    for (const auto& pair_c : flags) {          // +0x43, +0x44
                        if (flag_a != 0 && (&pair_b != &flags[0] || &pair_c != &flags[0])) continue;
                        std::vector<Write> memory = {{2, club + 0x1ec, 1}, {4, other + 0x1c, same ? club : other_club}, {4, other + 0x20, other_cell},
                                                     {4, other_cell, other_word}, {4, other_word, 0x5a5a}, {1, other + 0x3d, same ? flag_a : 7},
                                                     {1, other + 0x3e, same ? 7 : flag_a}, {1, other + 0x4b, pair_b[0]}, {1, other + 0x4c, pair_b[1]},
                                                     {1, other + 0x43, pair_c[0]}, {1, other + 0x44, pair_c[1]}};
                        cases.push_back({.entry = prize_news, .registers = {{3, object}, {4, club}, {5, 0}, {6, 1000}, {7, 1}, {8, 2}, {9, other}, {10, 3}},
                                         .memory = memory, .answers = enabled,
                                         .label = std::string("news: prize other ") + (same ? "same " : "else ") + hex(flag_a) + " " + hex(pair_b[0]) + "," + hex(pair_b[1]) +
                                                  " " + hex(pair_c[0]) + "," + hex(pair_c[1])});
                    }
    }

    // 0x10452a60 (kind 0x23: a club's movement), and 0x104527c0 (kind 0x1f).
    for (std::size_t n = 0; n < w.objects.size() && n < 4; ++n) {
        const auto object = w.objects[n];
        const auto record = w.record_of(object);
        cases.push_back({.entry = year_news, .registers = {{3, object}, {4, w.club(n)}, {5, 0x1234 + std::uint32_t(n)}}, .answers = enabled, .label = "news: year " + hex(object)});
        if (n < 2) {
            cases.push_back({.entry = year_news, .registers = {{3, object}, {4, w.club(n)}, {5, 7}}, .answers = disabled, .label = "news: year disabled " + hex(object)});
            cases.push_back({.entry = year_news, .registers = {{3, object}, {4, w.club(n)}, {5, 7}}, .label = "news: year real switch " + hex(object)});
        }
        for (const bool second : {false, true})
            for (const bool third : {false, true})
                cases.push_back({.entry = movement_news, .registers = {{3, object}, {4, third ? record : 0}, {5, w.club(n)}, {6, second ? w.club(n + 1) : 0}, {7, 0x101 + std::uint32_t(n)}},
                                 .label = std::string("news: movement ") + hex(object) + (second ? " second" : "") + (third ? " third" : "")});
    }
    {
        const auto object = w.objects.front();
        cases.push_back({.entry = movement_news, .registers = {{3, object}, {4, 0}, {5, 0}, {6, 0}, {7, 1}}, .label = "news: movement no club"});
        cases.push_back({.entry = movement_news, .registers = {{3, object}, {4, w.record(1)}, {5, 0}, {6, w.club(1)}, {7, 1}}, .label = "news: movement no club, with others"});
        for (const std::uint32_t kind : {0u, 1u, 0x7fu, 0x80u, 0xffu, 0x1ffu})
            cases.push_back({.entry = movement_news, .registers = {{3, object}, {4, w.record(2)}, {5, w.club(2)}, {6, w.club(3)}, {7, kind}}, .label = "news: movement kind " + hex(kind)});
    }
}

COMP_CASES("news: league fate text") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.objects.empty() || w.club_count < 8) return;
    const auto sp = scenario.sp();
    const auto frame_words = [&](std::uint32_t byte_a, std::uint32_t form, std::uint32_t output) {
        return std::vector<Write>{{1, sp + 0x3b, byte_a}, {1, sp + 0x3f, form}, {4, sp + 0x40, output}};
    };
    // Every kind the original tells apart (0..3, 5, the default: 4, 6, negative bytes, a kind whose low byte is one of
    // them) in both forms, over several competitions and clubs (the texts differ by the record's name and gender).
    for (std::size_t n = 0; n < w.objects.size() && n < 6; ++n)
        for (const std::uint32_t club : {0u, 1u, 7u})
            for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 0xffu, 0x100u, 0x105u, 0x80u})
                for (const std::uint32_t form : {0u, 1u, 0x100u, 0x80u}) {
                    if (form >= 0x80 && (n > 1 || club != 0)) continue;
                    cases.push_back({.entry = fate_text, .registers = {{3, w.objects[n]}, {4, club}, {5, kind}, {8, 0}, {9, 0x77}},
                                     .memory = joined(frame_words(0x55, form, out), empty_out()),
                                     .label = "news: fate " + hex(w.objects[n]) + " club " + std::to_string(club) + " kind " + hex(kind) + " form " + hex(form)});
                }
    // Kind 5 with the competition's name: filed unless the fixture rules (0x10494d20) claim the competition.
    for (const std::uint32_t answer : {0u, 1u, 0x100u, 0x80u})
        cases.push_back({.entry = fate_text, .registers = {{3, w.objects[0]}, {4, 2}, {5, 5}, {8, 0}, {9, 0x77}},
                         .memory = joined(frame_words(0x55, 1, out), empty_out()), .answers = {{0x10494d20, answer}},
                         .label = "news: fate kind 5, fixture rules answer " + hex(answer)});
    // The hand-over to 0x10469710 (form 0) and 0x10464d00, answered (the real routines fault on these arguments on both sides).
    for (const std::uint32_t mode : {1u, 0x80u, 0x10000u})
        for (const std::uint32_t form : {0u, 1u}) {
            const std::pair<unsigned, std::uint32_t> answer = {form ? fate_by_stage : fate_by_flag, 9};
            cases.push_back({.entry = fate_text, .registers = {{3, w.objects[0]}, {4, 3}, {5, 2}, {8, mode}, {9, 0x77}},
                             .memory = joined(frame_words(0x55, form, out), empty_out()), .answers = {answer},
                             .label = "news: fate hand-over mode " + hex(mode) + " form " + std::to_string(form)});
        }
}

COMP_CASES("news: position and place texts") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.objects.empty() || w.club_count < 4) return;
    // 0x10450580: the assertion (position < 30, signed), then each code (a real competition's, 0, -1, the halfword limits)
    // against each kind (0..8 and the sign/low-byte cases), with and without a stage group.
    for (const std::uint32_t position : {0u, 29u, 0xffffffffu, 0x80000000u})
        cases.push_back({.entry = position_text, .registers = {{4, buffer}, {5, position}, {6, 1}, {7, 2}, {8, 1}, {9, w.club(0)}},
                         .memory = patterned_buffer(), .label = "news: position assertion " + hex(position)});
    std::vector<std::uint32_t> codes = {0, 1, 2, 0xffff, 0x8000, 0x3fc, 0x10000, 0x12345};
    for (std::size_t n = 0; n < w.objects.size() && n < 40; ++n) codes.push_back(scenario.r32(w.objects[n] + 0x3a) >> 16);
    for (const auto code : codes)
        for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 0x100u, 0x106u, 0x108u, 0x80u, 0xffu})
            for (const std::uint32_t second : {0u, 3u})
                for (const std::uint32_t position : {30u, 31u, 0x7fffffffu}) {
                    if ((second != 0 || position != 30) && kind > 3 && kind != 6 && kind != 8) continue;
                    cases.push_back({.entry = position_text, .registers = {{4, buffer}, {5, position}, {6, code}, {7, second}, {8, kind}, {9, w.club(1)}},
                                     .memory = patterned_buffer(),
                                     .label = "news: position code " + hex(code) + " kind " + hex(kind) + " second " + hex(second) + " position " + hex(position)});
                }

    // 0x10451ac0: the empty text, the fixture rules' competition (no stage lookup), every stage index of the real
    // competitions with each club of their entries, the assertion on both sides of the stage range, and the
    // other kinds (type 1: the club's +0x30 text, type 2: +0x2c, else nothing).
    const auto rules_cell = scenario.toc(rules_slot);
    for (std::size_t n = 0; n < w.objects.size() && n < 24; ++n) {
        const auto object = w.objects[n];
        const auto last = std::int32_t(w.last_stage(object));
        const auto club = w.club(n % w.club_count);
        for (std::int32_t stage = -3; stage <= last + 1 && stage < 6; ++stage)
            cases.push_back({.entry = place_text, .registers = {{3, object}, {4, buffer}, {5, club}, {6, std::uint32_t(stage)}},
                             .memory = patterned_buffer(), .label = "news: place " + hex(object) + " stage " + std::to_string(stage)});
        if (n < 4) {
            cases.push_back({.entry = place_text, .registers = {{3, object}, {4, buffer}, {5, club}, {6, 0x7fffffffu}}, .memory = patterned_buffer(), .label = "news: place largest stage " + hex(object)});
            cases.push_back({.entry = place_text, .registers = {{3, object}, {4, buffer}, {5, club}, {6, 0x80000000u}}, .memory = patterned_buffer(), .label = "news: place most negative stage " + hex(object)});
            cases.push_back({.entry = place_text, .registers = {{3, object}, {4, buffer}, {5, club}, {6, 0xffffffffu}},
                             .memory = joined(patterned_buffer(), {{4, rules_cell, object}}), .label = "news: place by rules " + hex(object)});
        }
    }
    // Without a fixture rules object; a knockout's entries with the club at each row (the loop's break), at none; and
    // a league object whose type byte (+0x42) is 1, 2 or another, with the club's text pointers set and null.
    std::size_t typed = 0, knockouts = 0;
    for (std::size_t n = 0; n < w.objects.size() && n < 200; ++n) {
        const auto object = w.objects[n];
        if (n < 24)
            cases.push_back({.entry = place_text, .registers = {{3, object}, {4, buffer}, {5, w.club(1)}, {6, 0xffffffffu}},
                             .memory = joined(patterned_buffer(), {{4, rules_cell, 0}}), .label = "news: place without rules " + hex(object)});
        if (scenario.r32(object + 0x43) >> 24 == 2) {
            if (knockouts++ >= 8) continue;
            const auto entries = scenario.r32(object + 0xb0);
            const auto count = std::int16_t(scenario.r32(object + 0x3e) >> 16);
            for (const std::int32_t row : {0, 1, count - 1})
                if (row >= 0 && row < count)
                    cases.push_back({.entry = place_text, .registers = {{3, object}, {4, buffer}, {5, scenario.r32(entries + std::uint32_t(row) * 0x44)}, {6, 0xffffffffu}},
                                     .memory = joined(patterned_buffer(), {{4, rules_cell, 0}}),
                                     .label = "news: place knockout " + hex(object) + " row " + std::to_string(row)});
            cases.push_back({.entry = place_text, .registers = {{3, object}, {4, buffer}, {5, 0x12345678}, {6, 0xffffffffu}},
                             .memory = joined(patterned_buffer(), {{4, rules_cell, 0}}), .label = "news: place knockout no club " + hex(object)});
        } else if (typed++ < 3) {
            for (const std::uint32_t type : {1u, 2u, 3u, 0x81u})
                for (const std::uint32_t pointer : {0u, buffer + 0x300})
                    cases.push_back({.entry = place_text, .registers = {{3, object}, {4, buffer}, {5, w.club(2)}, {6, 0xffffffffu}},
                                     .memory = joined(patterned_buffer(), {{4, rules_cell, 0}, {1, object + 0x42, type}, {4, w.club(2) + 0x30, pointer},
                                                                          {4, w.club(2) + 0x2c, pointer}, {4, buffer + 0x314, 0x58595a00}, {4, buffer + 0x3a4, 0x61626300}}),
                                     .label = "news: place type " + hex(type) + (pointer ? " with texts " : " without texts ") + hex(object)});
        }
    }
}

COMP_CASES("news: item texts") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.objects.empty() || w.club_count < 8 || w.record_count < 6) return;
    const auto object = w.objects.front();
    // 0x10452290: form 0 (a club's amount), and form 1: a place (field 3 > 0), a stage (field 4 != 0), the two
    // wordings of kind (field 7) 1 and 2, and two clubs (field 5 a club or -1, field 6 each).
    for (const std::uint32_t amount : {0u, 123456u, 0xffffffffu})
        cases.push_back({.entry = prize_text, .registers = {{3, object}, {4, item}, {5, 0}, {6, out}},
                         .memory = joined(joined(item_fields({{1, amount}, {2, 3}}), empty_out()), {}), .label = "news: prize text form 0 amount " + hex(amount)});
    for (const std::uint32_t place : {1u, 2u, 7u, 0x7fu, 0x101u})
        for (const std::uint32_t amount : {5000u, 0u})
            cases.push_back({.entry = prize_text, .registers = {{3, object}, {4, item}, {5, 1}, {6, out}},
                             .memory = joined(item_fields({{1, amount}, {2, 5}, {3, place}, {4, 0}, {5, 0xffffffffu}, {6, 0}, {7, 0}}), empty_out()),
                             .label = "news: prize text place " + hex(place) + " amount " + hex(amount)});
    for (const std::uint32_t stage : {1u, 0xfffeu, 0x8000u, 0x10005u})
        for (const std::uint32_t third : {0u, 0x80u, 0xffu})
            cases.push_back({.entry = prize_text, .registers = {{3, object}, {4, item}, {5, 1}, {6, out}},
                             .memory = joined(item_fields({{1, 99999}, {2, 6}, {3, third}, {4, stage}, {5, 1}, {6, 1}, {7, 1}}), empty_out()),
                             .label = "news: prize text stage " + hex(stage) + " third " + hex(third)});
    for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 0x101u, 0x102u, 0xffu})
        for (const std::uint32_t other_club : {0xffffffffu, 0u, 4u, 0x80000000u, 0xfffffffeu})
            for (const std::uint32_t flag : {0u, 1u, 0x100u}) {
                if (flag != 0 && kind > 3) continue;
                cases.push_back({.entry = prize_text, .registers = {{3, object}, {4, item}, {5, 1}, {6, out}},
                                 .memory = joined(item_fields({{1, 4242}, {2, 2}, {3, 0}, {4, 0}, {5, other_club}, {6, flag}, {7, kind}}), empty_out()),
                                 .label = "news: prize text kind " + hex(kind) + " other " + hex(other_club) + " flag " + hex(flag)});
            }
    for (const std::uint32_t form : {0x100u, 0x80u})  // only the low byte is the form
        cases.push_back({.entry = prize_text, .registers = {{3, object}, {4, item}, {5, form}, {6, out}},
                         .memory = joined(item_fields({{1, 4242}, {2, 2}, {3, 0}, {4, 0}, {5, 1}, {6, 0}, {7, 1}}), empty_out()),
                         .label = "news: prize text form " + hex(form)});

    // 0x10452880: both forms; the form 1 text of a club's 0x10192350 text.
    for (const std::uint32_t form : {0u, 1u, 0x100u})
        for (const std::uint32_t club : {0u, 5u})
            for (const std::uint32_t amount : {0u, 1000000u})
                for (const std::uint32_t third : {0u, 1u, 0x10005u, 0xffffu}) {
                    if (form != 1 && third != 0) continue;
                    cases.push_back({.entry = money_text, .registers = {{3, object}, {4, item}, {5, form}, {6, out}},
                                     .memory = joined(item_fields({{0, club}, {2, amount}, {3, third}}), empty_out()),
                                     .label = "news: money text form " + hex(form) + " club " + std::to_string(club) + " amount " + hex(amount) + " third " + hex(third)});
                }

    // 0x10452bb0: kind (field 4) 1 and 2 and others, in both forms; kind 1 form 1 names two clubs and two records.
    for (std::size_t n = 0; n < w.objects.size() && n < 3; ++n)
        for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 0x101u, 0xffu})
            for (const std::uint32_t form : {0u, 1u, 0x100u})
                cases.push_back({.entry = movement_text, .registers = {{3, w.objects[n]}, {4, item}, {5, form}, {6, out}},
                                 .memory = joined(item_fields({{1, 2 + std::uint32_t(n)}, {2, 4}, {3, 3}, {4, kind}}), empty_out()),
                                 .label = "news: movement text " + hex(w.objects[n]) + " kind " + hex(kind) + " form " + hex(form)});
    cases.push_back({.entry = movement_text, .registers = {{3, object}, {4, item}, {5, 1}, {6, out}},
                     .memory = joined(item_fields({{1, 0}, {2, 5}, {3, 4}, {4, 1}}), empty_out()), .label = "news: movement text kind 1 form 1 second records"});
    (void)copy;
}
}
