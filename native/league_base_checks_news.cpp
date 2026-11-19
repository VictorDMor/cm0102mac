// LEAGUE.CPP, block `news`: cases for the prizes, the news items and the remaining queries, from the real career
// (--world): its league objects (standings rows, rounds, statistics), clubs, news manager. What the career lacks is
// written into a case's own memory (undone afterwards): a club count, a date and weekday slots in a round, a
// club's rating bytes, flags and links. Other modules' routines are answered where their result steers a branch
// (the news manager's switch, the fixture rules, a status and its out words through a poke) and run for real
// otherwise. The items and prizes write news records and club cash: every branch has a case.
#include "league_base_checks.hpp"
#include <algorithm>
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::league_base_checks {
namespace {
constexpr std::uint32_t prize_total = 0x1049f540, prize_news = 0x1049f6d0, first_round_news = 0x104a0e40, settle_places = 0x104a1350,
                        result_news = 0x104a44f0, record_place = 0x104a47f0, record_positions = 0x104aa0c0, touch_clubs = 0x104aa3a0,
                        team_points = 0x104aa620;
constexpr std::uint32_t news_enabled = 0x102cdbf0, fixture_rules = 0x10494d20, new_object = 0x10773520, new_array = 0x10775130,
                        new_rows = 0x107735f0, assertion = 0x107755c0, status_call = 0x1049d1c0, club_cash = 0x10103510, touch_call = 0x101b5b20,
                        place_call = 0x10463990, row_call = 0x100c76c0;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60, clubs_slot = -0x7be8,
                       records_slot = -0x7bbc, cash_slot = -0x7ab8, touch_slot = -0x79f0, world_slot = -0x7b98;
constexpr std::uint32_t row_size = 0x44, club_size = 0x248;

std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
Write w8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write w16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

struct World {
    const Scenario& s;
    std::vector<std::uint32_t> leagues;
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto table = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        std::set<std::uint32_t> seen;
        const auto add = [&](std::uint32_t object) {
            if (!object || !seen.insert(object).second) return;
            if (is_league(object)) leagues.push_back(object);
        };
        for (std::uint32_t id = 0; table && id < count && id < 4096; ++id) {
            const auto object = s.r32(table + id * 4);
            if (!object) continue;
            add(object);
            const auto last = std::int32_t(s.r32(object + 0x30));
            const auto stages = s.r32(object + 0xc);
            for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) add(s.r32(stages + std::uint32_t(n) * 4));
        }
    }
    std::uint32_t u8(std::uint32_t at) const { return (s.r32(at & ~3u) >> (24 - 8 * (at & 3))) & 0xff; }
    std::int32_t i16(std::uint32_t at) const { return std::int16_t(u8(at) << 8 | u8(at + 1)); }
    std::uint32_t u16(std::uint32_t at) const { return u8(at) << 8 | u8(at + 1); }
    bool is_league(std::uint32_t object) const {
        if (s.r32(object) < 0x108b0000 || s.r32(object) >= 0x10a00000 || u8(object + 0x43) != 2) return false;
        for (std::uint32_t slot = 0x88; slot <= 0xb8; slot += 4) {
            const auto descriptor = s.r32(s.r32(object) + slot);
            if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return false;
            const auto code = s.r32(descriptor);
            if (code >= 0x1049e4f0 && code < 0x104aac14) return true;
        }
        return false;
    }
    std::int32_t clubs(std::uint32_t league) const { return i16(league + 0x3e); }
    std::uint32_t rows(std::uint32_t league) const { return s.r32(league + 0xb0); }
    std::uint32_t row(std::uint32_t league, unsigned i) const { return rows(league) + i * row_size; }
    std::uint32_t club_of(std::uint32_t league, unsigned i) const { return s.r32(row(league, i)); }
    std::uint32_t record(std::uint32_t league) const { return s.r32(league + 4); }
    std::uint32_t club(unsigned index) const { return s.global(clubs_slot) + index * club_size; }
};

LEAGUE_CASES("news") {
    if (scenario.match) return;
    const World w(scenario);
    std::vector<std::uint32_t> leagues;  // usable: active standings of at least eight clubs, rounds, statistics
    for (const auto league : w.leagues)
        if (w.rows(league) && w.clubs(league) >= 8 && w.club_of(league, 0) && w.club_of(league, 1) && w.club_of(league, 2) && leagues.size() < 40) leagues.push_back(league);
    if (leagues.empty()) throw std::runtime_error("The career has no league with eight clubs");
    std::uint32_t with_rounds = 0, with_stats = 0;
    for (const auto league : leagues) {
        with_rounds += scenario.r32(league + 0xbc) != 0;
        with_stats += scenario.r32(league + 8) != 0;
    }
    std::fprintf(stderr, "news: %zu leagues (%zu usable, %u with rounds, %u with statistics); cash cell %x, touch cell %x, news cell %x\n", w.leagues.size(),
                 leagues.size(), with_rounds, with_stats, scenario.global(cash_slot), scenario.global(touch_slot), scenario.global(-0x7bf0));
    const auto lab = [&](const char* what, std::uint32_t league) { return std::string("news: ") + what + " " + hex(league); };
    const auto league0 = leagues.front();
    const std::vector<std::uint32_t> few(leagues.begin(), leagues.begin() + std::min<std::size_t>(leagues.size(), 3));

    // 0x1049f540: the sum of (place + 1) * amount, for the club counts around the unrolled loop's edges, wrapping and
    // negative counts; no clubs asserts.
    for (const auto league : few)
        for (const std::uint32_t amount : {0u, 1u, 5000u, 0xffffffffu, 0x80000000u})
            cases.push_back({.entry = prize_total, .registers = {{3, league}, {4, amount}}, .label = lab(("prize_total " + hex(amount)).c_str(), league)});
    for (const std::uint32_t n : {0u, 1u, 2u, 7u, 8u, 9u, 10u, 15u, 16u, 17u, 33u, 0x7fffu, 0x8000u, 0xffffu})
        for (const std::uint32_t amount : {3u, 0x12345678u})
            cases.push_back({.entry = prize_total, .registers = {{3, league0}, {4, amount}}, .memory = {w16(league0 + 0x3e, n)},
                             .label = lab(("prize_total clubs " + hex(n) + " amount " + hex(amount)).c_str(), league0)});

    // 0x1049f6d0: every club, or the clubs whose +0x34 is the filter; the cash object real, absent, and the callees answered.
    {
        const auto cash = scenario.toc(cash_slot);
        const auto filter = scenario.r32(w.club_of(league0, 1) + 0x34);
        for (const auto league : few)
            for (const std::uint32_t only : {0u, filter, 0x1234u})
                cases.push_back({.entry = prize_news, .registers = {{3, league}, {4, 777}, {5, only}}, .label = lab(("prize_news filter " + hex(only)).c_str(), league)});
        cases.push_back({.entry = prize_news, .registers = {{3, league0}, {4, 777}, {5, 0}}, .memory = {w32(cash, 0)}, .label = lab("prize_news no cash object", league0)});
        cases.push_back({.entry = prize_news, .registers = {{3, league0}, {4, 777}, {5, 0}}, .memory = {w32(cash, w.club(2))},
                         .answers = {{club_cash, w.club(5)}, {0x100f5730, 0}}, .label = lab("prize_news answered cash", league0)});
        for (const std::uint32_t n : {0u, 1u, 2u, 0xffffu})
            cases.push_back({.entry = prize_news, .registers = {{3, league0}, {4, 777}, {5, 0}}, .memory = {w16(league0 + 0x3e, n)},
                             .label = lab(("prize_news clubs " + hex(n)).c_str(), league0)});
    }

    // 0x104a0e40: the first round's date and the earliest of its weekday slots; the news switch per club.
    for (const auto league : leagues) {
        const auto rounds = scenario.r32(league + 0xbc);
        if (!rounds) continue;
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> on = {{news_enabled, 1}}, off = {{news_enabled, 0}};
        const auto slots = [&](std::uint32_t first_weekday, std::uint32_t used, std::uint32_t flag) {
            std::vector<Write> writes;
            for (std::uint32_t k = 0; k < 8; ++k) {
                writes.push_back(w8(rounds + 0xc + 8 * k, (first_weekday + k) % 7));
                writes.push_back(w8(rounds + 0xe + 8 * k, k < used ? flag : 0xff));
            }
            return writes;
        };
        const auto date = [&](std::uint32_t day, std::uint32_t year_offset) { return std::vector<Write>{w16(rounds, day), w16(rounds + 2, year_offset)}; };
        for (const std::uint32_t n_clubs : {1u, 2u, 3u, 8u})
            cases.push_back({.entry = first_round_news, .registers = {{3, league}}, .memory = joined(joined(date(30, 0), slots(0, 3, 0)), {w16(league + 0x3e, n_clubs)}),
                             .answers = on, .label = lab(("first_round_news clubs " + hex(n_clubs)).c_str(), league)});
        if (league != league0) continue;
        // The dates: days at the year's edges and in the middle, each weekday alone and all seven, so the nearest weekday is
        // before, on and after the first date in the same and the previous or next year.
        for (const std::uint32_t day : {0u, 1u, 2u, 3u, 4u, 100u, 361u, 362u, 363u, 364u, 365u})
            for (const std::uint32_t year_offset : {0u, 1u}) {
                for (std::uint32_t weekday = 0; weekday < 7; ++weekday)
                    cases.push_back({.entry = first_round_news, .registers = {{3, league}},
                                     .memory = joined(joined(date(day, year_offset), slots(weekday, 1, 0)), {w16(league + 0x3e, 2)}), .answers = on,
                                     .label = lab(("first_round_news day " + hex(day) + "+" + hex(year_offset) + " weekday " + hex(weekday)).c_str(), league)});
                cases.push_back({.entry = first_round_news, .registers = {{3, league}}, .memory = joined(joined(date(day, year_offset), slots(0, 8, 0)), {w16(league + 0x3e, 2)}),
                                 .answers = on, .label = lab(("first_round_news day " + hex(day) + "+" + hex(year_offset) + " all weekdays").c_str(), league)});
                cases.push_back({.entry = first_round_news, .registers = {{3, league}}, .memory = joined(joined(date(day, year_offset), slots(3, 7, 0x7f)), {w16(league + 0x3e, 2)}),
                                 .answers = on, .label = lab(("first_round_news day " + hex(day) + "+" + hex(year_offset) + " seven slots").c_str(), league)});
            }
        // No slots at all; a negative year offset; the league's own year.
        cases.push_back({.entry = first_round_news, .registers = {{3, league}}, .memory = joined(date(40, 0xffff), slots(0, 0, 0)), .answers = on, .label = lab("first_round_news no slots", league)});
        cases.push_back({.entry = first_round_news, .registers = {{3, league}}, .memory = joined(date(0x8000, 0x8000), slots(1, 2, 0)), .answers = on,
                         .label = lab("first_round_news negative day and year offset", league)});
        // The news switch: off everywhere; real; only the k-th club's on (the rest off), only the k-th off.
        cases.push_back({.entry = first_round_news, .registers = {{3, league}}, .answers = off, .label = lab("first_round_news switch off", league)});
        cases.push_back({.entry = first_round_news, .registers = {{3, league}}, .memory = {w16(league + 0x3e, 8)}, .label = lab("first_round_news real switch, 8 clubs", league)});
        for (unsigned k = 1; k <= 3; ++k) {
            cases.push_back({.entry = first_round_news, .registers = {{3, league}}, .memory = {w16(league + 0x3e, 4)}, .answers = off,
                             .nth_answers = {{news_enabled, k, 1}}, .label = lab(("first_round_news only club " + std::to_string(k) + " on").c_str(), league)});
            cases.push_back({.entry = first_round_news, .registers = {{3, league}}, .memory = {w16(league + 0x3e, 4)}, .answers = on,
                             .nth_answers = {{news_enabled, k, 0}}, .label = lab(("first_round_news only club " + std::to_string(k) + " off").c_str(), league)});
        }
        for (const std::uint32_t count : {0u, 9u, 0xffffu, 0x80u})
            cases.push_back({.entry = first_round_news, .registers = {{3, league}}, .memory = {w16(league + 0x3e, count)}, .answers = on,
                             .label = lab(("first_round_news clubs " + hex(count)).c_str(), league)});
        cases.push_back({.entry = first_round_news, .registers = {{3, league}}, .memory = {w16(league + 0x3e, 3)}, .answers = on,
                         .nth_answers = {{new_object, 1, 0}}, .label = lab("first_round_news allocation fails", league)});
    }

    // 0x104a1350 (vt +0x38).
    {
        const auto base = scenario.global(world_slot);
        const auto sp = scenario.sp();
        const auto out_a = sp - 0x60 + 0x38, out_b = sp - 0x60 + 0x3c;
        const auto virtual90 = [&](std::uint32_t league) { return scenario.r32(scenario.r32(scenario.r32(scenario.global(competitions_slot) + scenario.r32(w.record(league)) * 4)) + 0x90); };
        const auto target90 = scenario.r32(virtual90(league0));
        std::fprintf(stderr, "news: virtual +0x90 of the first league -> %x\n", target90);
        const auto record = w.record(league0);
        const auto first_club = w.club_of(league0, 0);
        for (const auto league : few)
            cases.push_back({.entry = settle_places, .registers = {{3, league}}, .label = lab("settle_places real", league)});
        const auto place = [&](const char* what, std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers,
                               std::vector<Case::Poke> pokes = {}) {
            cases.push_back({.entry = settle_places, .registers = {{3, league0}}, .memory = std::move(memory), .answers = std::move(answers), .pokes = std::move(pokes),
                             .label = lab(what, league0)});
        };
        // The first club points at the league's record: the clubs' +0x34 equal to it are cleared.
        const std::vector<Write> linked = {w32(first_club + 0x30, record), w32(w.club(0) + 0x34, record), w32(w.club(3) + 0x34, record), w32(w.club(4) + 0x34, 7)};
        for (const std::uint32_t n : {1u, 2u, 3u, 9u})
            place(("settle_places linked, clubs " + std::to_string(n)).c_str(), joined(linked, {w16(league0 + 0x3e, n)}), {});
        for (const std::uint32_t answer : {0u, 1u, 0x80u})
            place(("settle_places virtual answers " + hex(answer)).c_str(), {}, {{target90, answer}, {status_call, 0}});
        // The status: 0, 1, 2 with each order of the two halves at +0xac, and others.
        const auto status = [&](const char* what, std::uint32_t answer, std::uint32_t first, std::uint32_t second, std::uint32_t half_first,
                                std::uint32_t half_second, std::vector<std::pair<std::uint32_t, std::uint32_t>> more = {}) {
            std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {{target90, 1}, {status_call, answer}};
            answers.insert(answers.end(), more.begin(), more.end());
            place(what, {w16(base + 0xac + first * 0x128, half_first), w16(base + 0xac + second * 0x128, half_second)}, answers,
                  {{status_call, {w32(out_a, first), w32(out_b, second)}}});
        };
        status("settle_places status 0", 0, 1, 2, 5, 6);
        status("settle_places status 1", 1, 3, 2, 5, 6);
        status("settle_places status 3", 3, 3, 2, 5, 6);
        status("settle_places status 0xff", 0xff, 3, 2, 5, 6);
        status("settle_places status 2, first fewer", 2, 1, 2, 5, 6);
        status("settle_places status 2, second fewer", 2, 1, 2, 6, 5);
        status("settle_places status 2, equal", 2, 1, 2, 5, 5);
        status("settle_places status 2, negative halves", 2, 2, 0, 0xffff, 1);
        status("settle_places status 2, same row", 2, 1, 1, 5, 5);
        status("settle_places status 2, rows answered", 2, 1, 2, 5, 6, {{row_call, 0x5555}, {place_call, 0}});
        status("settle_places status 1, answered", 1, 1, 2, 5, 6, {{row_call, 0x4444}, {place_call, 0}});
    }

    // 0x104a44f0 (vt +0x48) and 0x104a47f0 (vt +0x4c): a result's news item and where it is filed.
    {
        const auto sp = scenario.sp();
        const auto out_byte = sp - 0x150 + 0x38;
        const auto league = league0;
        const auto club = w.club_of(league, 1);
        const auto record = w.record(league);
        const auto records = scenario.global(records_slot);
        const auto objects = scenario.global(competitions_slot);
        const auto own = scenario.r32(objects + scenario.r32(record) * 4);
        const auto stages = scratch_base + 0x20000;
        const auto result = [&](const std::string& what, std::uint32_t kind, std::uint32_t stage, std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers,
                                std::vector<Case::Poke> pokes = {}, std::vector<Case::NthAnswer> nth = {}) {
            cases.push_back({.entry = result_news, .registers = {{3, league}, {4, club}, {5, kind}, {6, stage}}, .memory = std::move(memory), .answers = std::move(answers),
                             .pokes = std::move(pokes), .nth_answers = std::move(nth), .label = lab(("result_news " + what).c_str(), league)});
        };
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> history = {{0x10381b70, 0}, {0x103822e0, 0}, {0x10382e50, 0}};
        auto rules_ok = history;
        rules_ok.push_back({fixture_rules, 0});
        const auto rules_ok_with = [&](std::uint32_t answer) { auto list = history; list.push_back({fixture_rules, answer}); return list; };
        std::fprintf(stderr, "news: own %x (vtable +0x4c -> %x), +0x1c %x +0x20 %x, league +0x44 %x +0x1c %x\n", own, scenario.r32(scenario.r32(scenario.r32(own) + 0x4c)),
                     scenario.r32(own + 0x1c), scenario.r32(own + 0x20), w.u8(league + 0x44), scenario.r32(league + 0x1c));
        for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 0xfeu, 0xffu, 0x80u, 0x105u})
            result("kind " + hex(kind), kind, 0xff, {}, rules_ok);
        result("fixture rules refuse", 0, 0xff, {}, {{fixture_rules, 1}});
        result("fixture rules answer 0x100 (low byte 0)", 0, 0xff, {}, rules_ok_with(0x100));
        // The virtual's second answer: the league's own +0x4c returns 0; -1 needs another object in the table.
        {
            const std::uint32_t other = [&] {
                for (const auto candidate : w.leagues) if (candidate != own && scenario.r32(scenario.r32(candidate) + 0x4c) != scenario.r32(scenario.r32(own) + 0x4c)) return candidate;
                return std::uint32_t(0);
            }();
            if (other) {
                const auto target = scenario.r32(scenario.r32(scenario.r32(other) + 0x4c));
                for (const std::uint32_t answer : {0xffffffffu, 7u, 0u})
                    result("virtual answers " + hex(answer), 0, 0xff, {w32(objects + scenario.r32(record) * 4, other)}, {{fixture_rules, 0}, {target, answer}, {0x10381b70, 0}, {0x103822e0, 0}, {0x10382e50, 0}});
            }
        }
        // The out byte: -1 keeps the kinds' filings, anything else files with the competition (holder with and without a +0x74).
        for (const std::uint32_t out : {0u, 1u, 0x7fu, 0x80u})
            for (const std::uint32_t holder : {0u, 1u}) {
                const auto holder_object = scratch_base + 0x21000;
                result("out byte " + hex(out) + (holder ? " with holder" : " no holder"), 0, 0xff,
                       {w32(club + 0x2c, holder ? holder_object : 0), w32(holder_object + 0x74, 0x7777)}, rules_ok, {{fixture_rules, {w8(out_byte, out)}}});
            }
        // The owner: -1 with a stage argument reads the stage's code; another owner keeps the league's.
        {
            std::vector<Write> stage_objects = {w32(league + 0xc, stages), w32(stages, league), w32(stages + 4, w.leagues.back())};
            for (const std::uint32_t owner : {0xffu, 0u})
                for (const std::uint32_t stage : {0xffu, 0u, 1u})
                    result("owner " + hex(owner) + " stage " + hex(stage), 3, stage, joined(stage_objects, {w8(league + 0x44, owner)}), rules_ok);
        }
        // Kind 5 with and without a parent (+0x1c) and kinds 0 and 3 with and without the related competition.
        for (const std::uint32_t parent : {0xffffffffu, 3u})
            result("kind 5 parent " + hex(parent), 5, 0xff, {w32(league + 0x1c, parent)}, rules_ok);
        for (const std::uint32_t kind : {0u, 3u})
            for (const std::uint32_t related : {0xffffffffu, 1u, 2u}) {
                result("kind " + hex(kind) + " related " + hex(related), kind, 0xff, {w32(own + (kind == 0 ? 0x1c : 0x20), related)}, rules_ok);
            }
        (void)records;
        // The record place (0x104a47f0): the three kinds that call the club-history object, and the others.
        for (const std::uint32_t kind : {0u, 3u, 5u, 1u, 2u, 4u, 0x80u, 0x105u, 0xfeu})
            cases.push_back({.entry = record_place, .registers = {{3, league}, {4, club}, {5, kind}}, .answers = history, .label = lab(("record_place kind " + hex(kind)).c_str(), league)});
    }

    // 0x104aa0c0 (vt +0x64): the positions into the statistics object.
    for (const auto league : leagues) {
        if (!scenario.r32(league + 8)) continue;
        cases.push_back({.entry = record_positions, .registers = {{3, league}}, .label = lab("record_positions", league)});
        if (league != league0 && league != leagues.back()) continue;
        cases.push_back({.entry = record_positions, .registers = {{3, league}}, .memory = {w32(league + 8, 0)}, .label = lab("record_positions no statistics", league)});
        cases.push_back({.entry = record_positions, .registers = {{3, league}}, .answers = {{new_array, 0}}, .label = lab("record_positions allocation fails", league)});
        cases.push_back({.entry = record_positions, .registers = {{3, league}}, .memory = {w32(w.row(league, 1), 0), w32(w.row(league, 3), 0)}, .label = lab("record_positions null clubs", league)});
        for (const std::uint32_t n : {0u, 1u, 2u, 0xffffu})
            cases.push_back({.entry = record_positions, .registers = {{3, league}}, .memory = {w16(league + 0x3e, n)}, .label = lab(("record_positions clubs " + hex(n)).c_str(), league)});
    }

    // 0x104aa3a0: a call per club when the global is set.
    {
        const auto cell = scenario.toc(touch_slot);
        for (const auto league : few) cases.push_back({.entry = touch_clubs, .registers = {{3, league}}, .label = lab("touch_clubs real", league)});
        cases.push_back({.entry = touch_clubs, .registers = {{3, league0}}, .memory = {w32(cell, 0)}, .label = lab("touch_clubs cell empty", league0)});
        for (const std::uint32_t n : {0u, 1u, 3u, 0xffffu})
            cases.push_back({.entry = touch_clubs, .registers = {{3, league0}}, .memory = {w32(cell, w.club(1)), w16(league0 + 0x3e, n)}, .answers = {{touch_call, 0}},
                             .label = lab(("touch_clubs answered, clubs " + hex(n)).c_str(), league0)});
    }

    // 0x104aa620 (vt +0x8c): the projection and the club's positions.
    {
        const auto points = [&](std::uint32_t league, std::uint32_t club, std::vector<Write> memory, const std::string& what,
                                std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}, std::vector<Case::NthAnswer> nth = {}) {
            cases.push_back({.entry = team_points, .registers = {{3, league}, {4, club}}, .memory = std::move(memory), .answers = std::move(answers),
                             .nth_answers = std::move(nth), .label = lab(("team_points " + what).c_str(), league)});
        };
        for (const auto league : leagues) {
            const auto n = w.clubs(league);
            points(league, w.club_of(league, 0), {}, "first");
            points(league, w.club_of(league, n / 2), {}, "middle");
            points(league, w.club_of(league, n - 1), {}, "last");
        }
        const auto league = league0;
        const auto club = w.club_of(league, 2);
        {
            std::uint32_t outside = 0;
            for (unsigned k = 0; k < 200 && !outside; ++k) {
                bool in = false;
                for (int i = 0; i < w.clubs(league); ++i) in |= w.club_of(league, i) == w.club(k);
                if (!in) outside = w.club(k);
            }
            points(league, outside, {}, "club outside the league");
        }
        points(league, 0, {}, "no club");
        points(league, club, {w16(league + 0x3e, 0)}, "no clubs");
        // The rows' games (+0x14 signed byte) and points (+0x1c halfword): none, one, negative, large; the club's own games decide the
        // sort; the history counter (+0xa4) either side of three times them; win points (+0xc4).
        const auto rows = w.rows(league);
        const auto n = w.clubs(league);
        const auto set_rows = [&](std::uint32_t games, std::uint32_t half) {
            std::vector<Write> writes;
            for (int i = 0; i < n; ++i) {
                writes.push_back(w8(rows + i * row_size + 0x14, i % 2 ? games : games + 1));
                writes.push_back(w16(rows + i * row_size + 0x1c, half + i * 3));
            }
            return writes;
        };
        for (const std::uint32_t games : {0u, 1u, 5u, 0x7fu, 0x80u, 0xffu, 0x40u})
            for (const std::uint32_t played : {0u, 3u, 15u, 16u, 0x7fffu, 0x8000u, 0xffffu})
                points(league, club, joined(set_rows(games, 20), {w16(league + 0xa4, played)}), "games " + hex(games) + " history " + hex(played));
        for (const std::uint32_t win : {0u, 2u, 3u, 0x7fu, 0x80u, 0xffu})
            points(league, club, joined(set_rows(2, 9), {w16(league + 0xa4, 38), w8(league + 0xc4, win)}), "win points " + hex(win));
        for (const std::uint32_t half : {0u, 0xffffu, 0x7fffu, 0x8000u, 100u})
            points(league, club, joined(set_rows(3, half), {w16(league + 0xa4, 38)}), "points " + hex(half));
        // The largest rates: one game and the largest points, the history counter at its largest (the product nears 2^30).
        for (const std::uint32_t win : {3u, 0x7fu})
            points(league, club, joined(set_rows(1, 0x7ff0), {w16(league + 0xa4, 0x7fff), w8(league + 0xc4, win)}), "largest rates win " + hex(win));
        // The club twice in the rows: the first match decides the sorts' rows, the last the final pass.
        points(league, club, {w32(rows, club), w32(rows + row_size, club)}, "club in two rows");
        points(league, club, {w32(rows + 3 * row_size, club)}, "club in two rows (3)");
        // One club; two; the allocation failing (and its assertion answered: the original goes on with the null array).
        for (const std::uint32_t count : {1u, 2u, 3u})
            points(league, w.club_of(league, 0), joined(set_rows(4, 12), {w16(league + 0x3e, count), w16(league + 0xa4, 20)}), "clubs " + hex(count));
        points(league, club, {}, "allocation fails", {{new_rows, 0}, {assertion, 0}});
        // The scenario of the sorts: equal points (ties are the qsort's).
        points(league, club, joined(set_rows(0, 0), {w16(league + 0xa4, 0)}), "all equal");
    }
}
}
}
