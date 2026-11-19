// Cases for CUP.CPP block `result` (cup_base_result*.cpp): CUP::process_fixture (0x1047bfb0) on the career's real
// cups. A case is a real cup, one of its real match rows and a fixture built in the scratch area from that row's
// real clubs (the home club's id first word is what 0x1047ba50 looks the row up by); the goals, flags, legs and
// the next tie's state are written over the real rows for the call and undone afterwards (Case::memory), so each
// branch of the routine is reached from a real object. The cup's own deciders (vt +0xa0, +0xa4: the base class's
// 0x1047e540 and 0x10482640 in every cup of the career) give the outcomes; an outcome they cannot give (an error
// or an unknown code) is answered on both sides. Labels start with "result:" (CM_CUP_BLOCK=result).
#include "cup_base_checks.hpp"
#include <algorithm>
#include <cstdio>
#include <optional>
#include <set>
#include <string>

namespace cm::aot::cup_base_checks {
namespace {
constexpr std::uint32_t process = 0x1047bfb0;
constexpr std::uint32_t decider_tie = 0x1047e540, decider_leg = 0x10482640;
constexpr std::uint32_t fixtures_new = 0x1048aea0;
[[maybe_unused]] constexpr std::uint32_t new_fixture_at = scratch_base + 0x1900;
constexpr std::uint32_t fixture_at = scratch_base + 0x1800, context_at = scratch_base + 0x5000;
// The fixture manager's and the stadium manager's routines read files the snapshot has no host for (they fault on
// HGetVol), so they are answered on both sides: their r3 only, as every other block's cases do.
constexpr std::uint32_t fixtures_add = 0x10489900, fixtures_update = 0x104888f0, stadium_book = 0x104aff20;
// COMP_STATS (0x104537b0) reads MATCH_DAY's match object through the context (+0x34..+0x132) and asserts, through the
// error dialog, on one a scratch area cannot imitate: it is answered too.
constexpr std::uint32_t comp_stats = 0x104537b0, pairings = 0x10481ad0, entrant_news = 0x1047b360, news_on = 0x102cdbf0, news_start = 0x102bdf90, news_set = 0x102cb4b0,
                        news_file = 0x102cc140;
// The club's history file (0x10748ce0: FSMakeFSSpec / HGetVol, which the native bridge does not implement), written
// by the places the entrant virtual (+0x48) records, is answered as the other units' cases do.
constexpr std::uint32_t history_file = 0x10748ce0;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60, today_slot = -0x7b80;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}

// Reads of the career's memory (the scenario's state, never changed).
struct Peek {
    const Scenario& s;
    std::uint32_t u8(std::uint32_t at) const { return s.r32(at & ~3u) >> (24 - 8 * (at & 3)) & 0xff; }
    std::uint32_t u16(std::uint32_t at) const { return u8(at) << 8 | u8(at + 1); }
    std::int32_t s8(std::uint32_t at) const { return std::int8_t(u8(at)); }
    std::int32_t s16(std::uint32_t at) const { return std::int16_t(u16(at)); }
    std::uint32_t u32(std::uint32_t at) const { return s.r32(at); }
};

struct Memory {
    std::vector<Write> w;
    Memory& b(std::uint32_t at, std::uint32_t v) { w.push_back({1, at, v & 0xff}); return *this; }
    Memory& h(std::uint32_t at, std::uint32_t v) { w.push_back({2, at, v & 0xffff}); return *this; }
    Memory& l(std::uint32_t at, std::uint32_t v) { w.push_back({4, at, v}); return *this; }
    Memory& add(const Memory& other) { w.insert(w.end(), other.w.begin(), other.w.end()); return *this; }
};

// A real cup with its tables.
struct RealCup {
    std::uint32_t object;
    std::int32_t rounds, clubs;
    std::uint32_t round_table, entrant_table, match_table, record;
    std::int32_t year, owner;
};

struct World {
    const Scenario& s;
    Peek p;
    std::vector<RealCup> cups;
    std::vector<std::uint32_t> parents;  // cups with stages (owner byte -1, last_stage >= 0)
    explicit World(const Scenario& scenario) : s(scenario), p{scenario} {
        const auto table = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        std::set<std::uint32_t> seen;
        const auto add = [&](std::uint32_t object) {
            if (!object || !seen.insert(object).second || p.u8(object + 0x43) != 1) return;
            const auto vt = s.r32(object);
            if (vt < 0x108b0000 || vt >= 0x10a00000) return;
            // The cup's own deciders (base class's 0x1047e540 and 0x10482640, the news 0x1047b360 and COMP's +0x84).
            const auto code = [&](std::uint32_t slot) { return s.r32(s.r32(vt + slot)); };
            if (code(0xa0) != decider_tie || code(0xa4) != decider_leg) return;
            RealCup cup{object, p.s16(object + 0x3c), p.s16(object + 0x3e), s.r32(object + 0xa0), s.r32(object + 0xa4),
                        s.r32(object + 0xac), s.r32(object + 4), p.s16(object + 0x40), p.s8(object + 0x44)};
            if (cup.rounds >= 1 && cup.rounds <= 12 && cup.round_table && cup.entrant_table && cup.match_table && cup.clubs >= 2 && cup.record)
                cups.push_back(cup);
        };
        for (std::uint32_t id = 0; table && id < count && id < 4096; ++id) {
            const auto object = s.r32(table + id * 4);
            if (!object) continue;
            add(object);
            const auto last = std::int32_t(s.r32(object + 0x30));
            const auto stages = s.r32(object + 0xc);
            if (p.u8(object + 0x43) == 1 && last >= 0 && stages && p.s8(object + 0x44) == -1) parents.push_back(object);
            for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) add(s.r32(stages + std::uint32_t(n) * 4));
        }
    }
    std::uint32_t round_row(const RealCup& c, std::int32_t r) const { return c.round_table + std::uint32_t(r) * 0x78; }
    std::uint32_t match_rows(const RealCup& c, std::int32_t r) const { return s.r32(c.match_table + std::uint32_t(r) * 4); }
    std::uint32_t match_row(const RealCup& c, std::int32_t r, std::int32_t m) const { return match_rows(c, r) + std::uint32_t(m) * 0x36; }
    std::uint32_t entrant(const RealCup& c, std::int32_t e) const { return c.entrant_table + std::uint32_t(e) * 8; }
    std::uint32_t club(const RealCup& c, std::int32_t e) const { return s.r32(entrant(c, e)); }
    std::uint32_t today_day() const { return p.u16(s.toc(today_slot)); }
    std::uint32_t today_year() const { return p.u16(s.toc(today_slot) + 2); }
};

// A fixture for a match row: the clubs (the row's first club is the home club unless `swap`), the round, goals,
// flags, the stage byte and the date.
struct Fixture {
    std::uint32_t home = 0, away = 0, round = 0, flags = 0, stage = 0xff;
    std::uint32_t home_goals = 0, away_goals = 0, extra_home = 0, extra_away = 0, penalties_home = 0, penalties_away = 0;
    std::uint32_t year = 0, day = 0, competition = 0;
};
Memory fixture_memory(const World& w, const Fixture& f) {
    Memory m;
    m.l(fixture_at + 0x00, f.competition).l(fixture_at + 0x0c, f.home ? w.s.r32(f.home) : 0).l(fixture_at + 0x10, f.away ? w.s.r32(f.away) : 0)
        .l(fixture_at + 0x1c, f.home).l(fixture_at + 0x20, f.away).h(fixture_at + 0x28, f.year).h(fixture_at + 0x2a, f.day)
        .h(fixture_at + 0x34, f.round).h(fixture_at + 0x38, f.flags).b(fixture_at + 0x3d, 0).b(fixture_at + 0x3e, 0)
        .b(fixture_at + 0x42, f.stage).b(fixture_at + 0x43, f.home_goals).b(fixture_at + 0x44, f.away_goals)
        .b(fixture_at + 0x47, f.extra_home).b(fixture_at + 0x48, f.extra_away).b(fixture_at + 0x49, f.penalties_home)
        .b(fixture_at + 0x4a, f.penalties_away);
    return m;
}

struct Spec {
    RealCup cup;
    std::int32_t round = 0, match = 0;
    std::int32_t home = 0, away = 1;  // entrant indexes of the fixture's clubs
    std::uint32_t home_goals = 0, away_goals = 0, flags = 0;
    std::uint32_t extra_home = 0, extra_away = 0, penalties_home = 0, penalties_away = 0;
    std::uint32_t day = 0;                  // 0: the game's day
    std::int32_t year_shift = 0;            // added to the cup's year for the fixture's
    std::optional<std::uint32_t> stage;     // the fixture's stage byte (default: the cup's owner byte)
    std::optional<std::uint32_t> home_id;   // the fixture's home id word (default: the home club's first word)
    std::uint32_t context = context_at;     // MATCH_DAY's third argument: COMP_STATS asserts on 0
    Memory pokes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
    std::vector<Case::Poke> events;         // memory written when a callee is dispatched
    bool null_fixture = false;
    std::uint32_t marks_home = 0, marks_away = 0;  // the fixture's +0x3d/+0x3e (a series' marks the leg decider counts down)
};

Case make(const World& w, const Spec& spec, const std::string& label) {
    Fixture f;
    f.home = w.club(spec.cup, spec.home);
    f.away = w.club(spec.cup, spec.away);
    f.round = std::uint32_t(spec.round);
    f.flags = spec.flags;
    f.stage = spec.stage ? *spec.stage : spec.cup.owner < 0 ? 0xff : std::uint32_t(spec.cup.owner);
    f.home_goals = spec.home_goals;
    f.away_goals = spec.away_goals;
    f.extra_home = spec.extra_home;
    f.extra_away = spec.extra_away;
    f.penalties_home = spec.penalties_home;
    f.penalties_away = spec.penalties_away;
    f.year = std::uint32_t(spec.cup.year + spec.year_shift);
    f.day = spec.day ? spec.day : w.today_day();
    f.competition = w.p.u32(spec.cup.record);
    Memory memory = fixture_memory(w, f);
    memory.l(fixture_at + 0x14, spec.cup.record);  // the competition's record, as MATCH_DAY's fixtures carry it
    memory.b(fixture_at + 0x3d, spec.marks_home).b(fixture_at + 0x3e, spec.marks_away);
    if (spec.home_id) memory.l(fixture_at + 0x0c, *spec.home_id);
    memory.add(spec.pokes);
    Case item{.entry = process, .registers = {{3, spec.cup.object}, {4, spec.null_fixture ? 0 : fixture_at}, {5, spec.context}},
              .memory = std::move(memory.w)};
    item.answers = {{fixtures_add, 0}, {fixtures_update, 0}, {stadium_book, 0}, {fixtures_new, 0}, {comp_stats, 0}, {history_file, 0}};
    for (const auto& answer : spec.answers) {
        const auto found = std::find_if(item.answers.begin(), item.answers.end(), [&](const auto& a) { return a.first == answer.first; });
        if (found != item.answers.end()) found->second = answer.second; else item.answers.push_back(answer);
    }
    // The 0x50 bytes handed to the fixture manager (0x104888f0, 0x10489900) are copied where the comparison sees them.
    item.captures = {{fixtures_update, 4, 0x50, scratch_base + 0xe000}, {fixtures_add, 4, 0x50, scratch_base + 0xe100}};
    item.pokes = spec.events;
    item.label = "result: " + label + " cup " + hex(spec.cup.object) + " r" + std::to_string(spec.round) + " m" + std::to_string(spec.match);
    return item;
}

// Candidate match rows of a real cup: both clubs placed and inside the table.
std::vector<std::pair<std::int32_t, std::int32_t>> placed_rows(const World& w, const RealCup& c) {
    std::vector<std::pair<std::int32_t, std::int32_t>> rows;
    for (std::int32_t r = 0; r < c.rounds; ++r) {
        const auto ties = w.p.s16(w.round_row(c, r) + 0x1c);
        for (std::int32_t m = 0; m < ties && m < 40; ++m) {
            const auto row = w.match_row(c, r, m);
            const auto a = w.p.s16(row), b = w.p.s16(row + 2);
            if (a >= 0 && b >= 0 && a < c.clubs && b < c.clubs && w.club(c, a) && w.club(c, b)) rows.emplace_back(r, m);
        }
    }
    return rows;
}

// Writes over a real cup's tables, so each branch's condition is set rather than hoped for.
struct Tables {
    Tables(const World& world, const RealCup& cup) : w(world), c(cup) {}
    const World& w;
    RealCup c;
    Memory m;
    std::uint32_t round_at(std::int32_t r) const { return w.round_row(c, r); }
    std::uint32_t row_at(std::int32_t r, std::int32_t mm) const { return w.match_row(c, r, mm); }
    // A round's fields: its label, the legs rule (+0x23), the legs a tie needs (+0x22), the day and year offset it
    // starts, the number of ties.
    Tables& round(std::int32_t r, std::uint32_t label, std::uint32_t rule, std::uint32_t needed, std::uint32_t start = 0, std::uint32_t year_offset = 0, std::uint32_t ties = 4) {
        const auto at = round_at(r);
        m.h(at + 0x08, label).b(at + 0x23, rule).b(at + 0x22, needed).h(at + 0x04, start ? start : w.today_day() + 20).b(at + 0x06, year_offset).h(at + 0x1c, ties);
        return *this;
    }
    // A match row: its clubs, the next tie's slot and round, the side word, the legs played, the second-leg flag and
    // the state byte the next tie waits on.
    Tables& row(std::int32_t r, std::int32_t mm, std::int32_t home, std::int32_t away, std::int32_t slot, std::int32_t next, std::int32_t side,
                std::uint32_t legs = 0, std::uint32_t second = 0, std::uint32_t state = 3) {
        const auto at = row_at(r, mm);
        m.h(at + 0, std::uint32_t(home)).h(at + 2, std::uint32_t(away)).h(at + 6, std::uint32_t(slot)).h(at + 8, std::uint32_t(side))
            .h(at + 0x12, std::uint32_t(next)).b(at + 0x32, legs).b(at + 0x33, second).b(at + 0x34, state);
        return *this;
    }
    // The next round, ready to take the winner: its row `slot` waiting on two placed clubs.
    Tables& ready(std::int32_t r, std::int32_t slot, std::int32_t next_round) {
        row(r, 0, 0, 1, slot, next_round, 1);
        row(next_round, slot, 2, 3, -1, -1, 0, 0, 0, 3);
        m.b(c.object + 0xa9, std::uint32_t(c.rounds));
        return *this;
    }
};

struct Picks {
    std::vector<RealCup> multi, single, staged;
    std::vector<std::uint32_t> parents;
};
Picks pick(const World& w) {
    Picks p;
    for (const auto& c : w.cups) {
        if (c.owner < 0 && c.rounds >= 4 && c.clubs >= 8 && p.multi.size() < 3) p.multi.push_back(c);
        if (c.owner < 0 && c.rounds == 1 && c.clubs >= 2 && p.single.size() < 2) p.single.push_back(c);
        if (c.owner >= 0 && c.rounds >= 2 && c.clubs >= 4 && p.staged.size() < 2) p.staged.push_back(c);
    }
    p.parents = w.parents;
    return p;
}

CUP_CASES("result") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.cups.empty()) throw std::runtime_error("result: the career has no real cup with tables");
    const auto picks = pick(w);
    if (picks.multi.empty() || picks.single.empty() || picks.staged.empty()) throw std::runtime_error("result: the career lacks a multi-round, a single-round or a stage cup");
    std::fprintf(stderr, "result: %zu real cups, %zu multi-round, %zu single-round, %zu stages, %zu parents\n", w.cups.size(), picks.multi.size(),
                 picks.single.size(), picks.staged.size(), picks.parents.size());
    const auto add = [&](const Spec& spec, const std::string& label) { cases.push_back(make(w, spec, label)); };
    const auto& multi = picks.multi.front();
    [[maybe_unused]] const auto& single = picks.single.front();
    const auto& staged = picks.staged.front();

    // Natural cases: real rows of real cups, the base class's own deciders on goals 2-0, 0-2 and 1-1.
    {
        std::size_t taken = 0;
        for (const auto& cup : w.cups) {
            const auto rows = placed_rows(w, cup);
            if (rows.empty() || taken >= 10) continue;
            ++taken;
            for (std::size_t n = 0; n < rows.size() && n < 2; ++n) {
                const auto [r, m] = rows[n];
                const auto row = w.match_row(cup, r, m);
                for (const auto& [hg, ag] : {std::pair{2u, 0u}, {0u, 2u}, {1u, 1u}}) {
                    Spec spec{.cup = cup, .round = r, .match = m, .home = w.p.s16(row), .away = w.p.s16(row + 2), .home_goals = hg, .away_goals = ag};
                    add(spec, "natural " + std::to_string(hg) + "-" + std::to_string(ag));
                }
            }
        }
    }

    // The arguments (0x1047bff4..): no fixture; a stage's cup with a stage above its last, below -1 and valid; a fixture
    // of another stage; a cup without a stage.
    {
        Spec spec{.cup = multi, .home_goals = 2};
        spec.null_fixture = true;
        add(spec, "no fixture");
        for (const auto parent : picks.parents) {
            for (const std::uint32_t stage_byte : {std::uint32_t(0), std::uint32_t(w.p.u32(parent + 0x30)) + 1, std::uint32_t(0xfe), std::uint32_t(w.p.u32(parent + 0x30))}) {
                RealCup c{parent, w.p.s16(parent + 0x3c), w.p.s16(parent + 0x3e), w.s.r32(parent + 0xa0), w.s.r32(parent + 0xa4), w.s.r32(parent + 0xac),
                          w.s.r32(parent + 4), w.p.s16(parent + 0x40), w.p.s8(parent + 0x44)};
                Spec stage_spec{.cup = c, .home = 0, .away = 1, .home_goals = 2};
                stage_spec.stage = stage_byte;
                // The parent's own tables may be empty: the stage virtual or the assertion decides.
                cases.push_back([&] {
                    auto item = make(w, stage_spec, "stage byte " + hex(stage_byte) + " parent");
                    return item;
                }());
            }
        }
        Spec other{.cup = staged, .home_goals = 2};
        other.stage = std::uint32_t(staged.owner) + 1;
        add(other, "fixture of another stage");
        Spec other_none{.cup = staged, .home_goals = 2};
        other_none.stage = 0xff;
        add(other_none, "fixture without a stage for a stage's cup");
        Spec other_own{.cup = multi, .home_goals = 2};
        other_own.stage = 3;
        add(other_own, "own cup, fixture with a stage");
    }

    // The statistics and the year: a competition with no object or no statistics; a fixture of an earlier year.
    {
        std::uint32_t empty_id = 0;
        const auto table = scenario.global(competitions_slot);
        const auto count = scenario.global(competition_count_slot);
        for (std::uint32_t id = 1; id < count && !empty_id; ++id) if (!scenario.r32(table + id * 4)) empty_id = id;
        if (empty_id) {
            Tables t(w, multi);
            t.m.l(scratch_base + 0x6000, empty_id).l(multi.object + 4, scratch_base + 0x6000);
            Spec spec{.cup = multi, .home_goals = 2, .pokes = t.m};
            add(spec, "no competition object");
        }
        const auto record_id = w.p.u32(multi.record);
        const auto object = scenario.r32(table + record_id * 4);
        if (object) {
            Spec spec{.cup = multi, .home_goals = 2};
            spec.pokes.l(object + 8, 0);
            add(spec, "no statistics");
        }
        Spec old{.cup = multi, .home_goals = 2};
        old.year_shift = -1;
        add(old, "fixture of an earlier year");
    }

    // Prize money (round +0x6c): paid, none, negative; clubs not in the cup (no row); the match row's leg counters.
    {
        for (const std::uint32_t prize : {std::uint32_t(100000), std::uint32_t(0), std::uint32_t(0xfffffff0)}) {
            Tables t(w, multi);
            t.m.l(t.round_at(0) + 0x6c, prize);
            t.round(0, 0x78, 2, 0).ready(0, 0, 1);
            Spec spec{.cup = multi, .home_goals = 2, .pokes = t.m};
            add(spec, "prize " + hex(prize));
        }
        Spec lost{.cup = multi, .home_goals = 2};
        lost.home_id = 0x12345;
        add(lost, "no match row");
        const std::pair<std::uint32_t, std::uint32_t> counters[] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}, {2, 0}, {0, 2}, {2, 1}, {5, 0}, {0xff, 0}, {0xfe, 0}, {0, 0xff}, {0x7f, 0x7f}};
        for (const auto& [played, second] : counters) {
            Tables t(w, multi);
            t.ready(0, 0, 1).round(0, 0x78, 2, 0);
            t.row(0, 0, 0, 1, 0, 1, 1, played, second);
            Spec spec{.cup = multi, .home_goals = 2, .pokes = t.m};
            add(spec, "legs " + std::to_string(played) + "+" + std::to_string(second));
        }
    }

    // Outcomes from the deciders on both sides; outcomes they cannot give, answered.
    {
        for (const std::uint32_t code : {std::uint32_t(0xffffffff), std::uint32_t(5), std::uint32_t(0x100), std::uint32_t(3), std::uint32_t(4), std::uint32_t(0), std::uint32_t(2), std::uint32_t(1)}) {
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).ready(0, 0, 1);
            Spec spec{.cup = multi, .home_goals = 1, .away_goals = 1, .pokes = t.m};
            spec.answers = {{decider_tie, code}};
            add(spec, "tie decider answers " + hex(code));
            Tables u(w, multi);
            u.round(0, 0x78, 2, 0).ready(0, 0, 1);
            Spec leg{.cup = multi, .home_goals = 1, .away_goals = 1, .pokes = u.m};
            leg.answers = {{decider_leg, code}};
            add(leg, "leg decider answers " + hex(code));
        }
    }

    // A replay (outcome 3): the fixture manager's new fixture (answered with a scratch record) takes the goals swapped
    // or the score; and its refusal.
    {
        for (const std::uint32_t flags : {std::uint32_t(0), std::uint32_t(0x10), std::uint32_t(0x2000), std::uint32_t(0x2010)}) {
            Tables t(w, multi);
            t.round(0, 0x78, 2, 0).ready(0, 0, 1);
            Spec spec{.cup = multi, .home_goals = 1, .away_goals = 1, .flags = flags, .pokes = t.m};
            spec.answers = {{decider_leg, 3}, {fixtures_new, new_fixture_at}};
            add(spec, "replay flags " + hex(flags));
        }
        Tables t(w, multi);
        t.round(0, 0x78, 2, 0).ready(0, 0, 1);
        Spec refused{.cup = multi, .home_goals = 1, .away_goals = 1, .pokes = t.m};
        refused.answers = {{decider_leg, 3}, {fixtures_new, 0}};
        add(refused, "replay refused");
    }

    // One round (+0x3c = 1): ties not played enough go on (level and postponed); a decided tie ends the cup (the
    // entrants' result bytes), one with a next slot is an error.
    {
        for (const auto& [rule, needed, slot, label] : {std::tuple{1u, 0u, -1, "decided"}, {1u, 2u, -1, "legs not played"}, {2u, 0u, -1, "rule 2"}, {1u, 0u, 0, "drawn game"}}) {
            for (const std::uint32_t code : {std::uint32_t(0), std::uint32_t(4)}) {
                Tables t(w, multi);
                t.round(0, 0x78, rule, needed).row(0, 0, 0, 1, slot, slot < 0 ? -1 : 1, 1, 0, 0, 3);
                t.m.h(multi.object + 0x3c, 1);
                Spec spec{.cup = multi, .home_goals = 1, .away_goals = 1, .pokes = t.m};
                spec.answers = {{decider_tie, code}, {decider_leg, code}};
                add(spec, std::string("one round, ") + label + ", decider " + std::to_string(code));
            }
        }
        // An entrant without a result keeps the cup open: three entrants counted, the third without result.
        for (const std::uint32_t code : {std::uint32_t(0), std::uint32_t(4)}) {
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).row(0, 0, 0, 1, -1, -1, 1, 0, 0, 3);
            t.m.h(multi.object + 0x3c, 1).h(t.round_at(0) + 0x1e, 3).b(w.entrant(multi, 2) + 5, 0);
            Spec spec{.cup = multi, .home_goals = 1, .away_goals = 1, .pokes = t.m};
            spec.answers = {{decider_tie, code}};
            add(spec, "one round, an entrant without result, decider " + std::to_string(code));
        }
    }

    // Levels on a later leg: the first, second and third match of the tie (legs played, the replay byte), the round
    // asking one or two legs, postponed the same with its two orders and its week or four days.
    {
        for (const auto& [played, second, needed] : {std::tuple{0u, 0u, 2u}, {1u, 0u, 1u}, {0u, 1u, 1u}, {1u, 1u, 0u}, {2u, 0u, 0u}, {3u, 0u, 0u}}) {
            for (const std::uint32_t code : {std::uint32_t(0), std::uint32_t(4)}) {
                for (const std::uint32_t flags : {std::uint32_t(0), std::uint32_t(0x2000)}) {
                    if (code == 0 && flags != 0) continue;
                    Tables t(w, multi);
                    t.round(0, 0x78, 1, needed).ready(0, 0, 1);
                    t.row(0, 0, 0, 1, 0, 1, 1, played, second);
                    Spec spec{.cup = multi, .home_goals = 1, .away_goals = 1, .flags = flags, .pokes = t.m};
                    spec.answers = {{decider_tie, code}};
                    add(spec, "later leg " + std::to_string(played) + "+" + std::to_string(second) + " needing " + std::to_string(needed) + " decider " +
                                  std::to_string(code) + " flags " + hex(flags));
                }
            }
        }
    }

    // Dates: the fixture's day before, on and after the game's, over a week (the weekday kind), for outcomes 0 and 4.
    {
        for (std::uint32_t day = w.today_day() - 3; day < w.today_day() + 9; ++day) {
            for (const std::uint32_t code : {std::uint32_t(0), std::uint32_t(4)}) {
                Tables t(w, multi);
                t.round(0, 0x78, 1, 0).ready(0, 0, 1);
                Spec spec{.cup = multi, .home_goals = 1, .away_goals = 1, .day = day, .pokes = t.m};
                spec.answers = {{decider_tie, code}};
                add(spec, "day " + std::to_string(day) + " decider " + std::to_string(code));
            }
        }
    }

    // Finals: the tie has no next slot; either side goes through, the row's first club the home club or the away, the
    // round the final (label 0x8c) or not; and the cup's other entrants having results or not.
    {
        for (const bool home : {true, false}) {
            for (const std::uint32_t label : {std::uint32_t(0x8c), std::uint32_t(0x78)}) {
                for (const bool first_is_home : {true, false}) {
                    Tables t(w, multi);
                    t.round(0, label, 1, 0).row(0, 0, 0, 1, -1, -1, 0, 0, 0, 3);
                    Spec spec{.cup = multi, .home = first_is_home ? 0 : 1, .away = first_is_home ? 1 : 0, .home_goals = home ? 2u : 0u, .away_goals = home ? 0u : 2u, .pokes = t.m};
                    add(spec, std::string("final, ") + (home ? "home" : "away") + " through, label " + hex(label) + (first_is_home ? ", first is home" : ", first is away"));
                }
            }
        }
    }

    // The entrant count of the rounds: none (the loop that ends a decided cup does not run) and a negative sum.
    {
        for (const std::uint32_t entrants : {std::uint32_t(0), std::uint32_t(0xfff0), std::uint32_t(2)}) {
            Tables t(w, multi);
            t.round(0, 0x8c, 1, 0).row(0, 0, 0, 1, -1, -1, 0, 0, 0, 3);
            for (std::int32_t r = 0; r < multi.rounds; ++r) t.m.h(t.round_at(r) + 0x1e, entrants);
            Spec spec{.cup = multi, .home_goals = 2, .pokes = t.m};
            add(spec, "final with entrant counts " + hex(entrants));
        }
        // The third-place round exists (the last round labelled 0x8c) but the fixture is not in the round before the semi-finals.
        Tables third(w, multi);
        third.round(0, 0x78, 1, 0).ready(0, 0, 1).round(multi.rounds - 1, 0x8c, 1, 0);
        Spec elsewhere{.cup = multi, .home_goals = 2, .pokes = third.m};
        add(elsewhere, "tie with a third-place round elsewhere");
    }

    // Ties with a next slot: the winner goes into the next round, the row's side word 0 or not, the row's first club the
    // fixture's home club or not, the third-place round present (the last round labelled 0x8c, this the one before
    // the semi-finals) or not, and the semi-finals' round with no such final round.
    {
        const auto last = multi.rounds - 1, semi = multi.rounds - 3;
        for (const bool home : {true, false}) {
            for (const std::int32_t side : {0, 1}) {
                for (const bool first_is_home : {true, false}) {
                    for (const int third : {0, 1, 2}) {
                        const auto round = third ? semi : 0;
                        Tables t(w, multi);
                        t.round(round, 0x78, 1, 0).round(round + 1, 0x78, 1, 0).ready(round, 0, round + 1);
                        t.row(round, 0, 0, 1, 0, round + 1, side);
                        if (third) t.round(last, third == 1 ? 0x8c : 0x78, 1, 0).row(last, 0, 0, 1, -1, -1, 0);
                        Spec spec{.cup = multi, .round = round, .home = first_is_home ? 0 : 1, .away = first_is_home ? 1 : 0,
                                  .home_goals = home ? 2u : 0u, .away_goals = home ? 0u : 2u, .pokes = t.m};
                        add(spec, std::string("tie, ") + (home ? "home" : "away") + " through, side " + std::to_string(side) + (first_is_home ? ", first is home" : ", first is away") +
                                      (third == 1 ? ", third place" : third == 2 ? ", semi-final without a final" : ""));
                    }
                }
            }
        }
    }

    // The deciders asked by the round's rule and the legs played: a two-legged round asks the leg decider except for the third
    // match; the base decider's own outcomes on a two-legged round (goals and flags).
    {
        for (const auto& [played, second, rule] : {std::tuple{1u, 1u, 2u}, {2u, 0u, 2u}, {0u, 0u, 2u}, {1u, 0u, 2u}, {0u, 1u, 2u}, {0u, 0u, 1u}}) {
            for (const auto& [hg, ag] : {std::pair{2u, 0u}, {0u, 2u}, {1u, 1u}}) {
                for (const std::uint32_t flags : {std::uint32_t(0), std::uint32_t(0x2), std::uint32_t(0x4), std::uint32_t(0x10), std::uint32_t(0x800), std::uint32_t(0x1000), std::uint32_t(0x2000), std::uint32_t(0x21)}) {
                    Tables t(w, multi);
                    t.round(0, 0x78, rule, 0).ready(0, 0, 1);
                    t.row(0, 0, 0, 1, 0, 1, 1, played, second);
                    Spec spec{.cup = multi, .home_goals = hg, .away_goals = ag, .flags = flags, .pokes = t.m};
                    spec.extra_home = 1;
                    spec.penalties_home = 4;
                    spec.penalties_away = 2;
                    add(spec, "rule " + std::to_string(rule) + " legs " + std::to_string(played) + "+" + std::to_string(second) + " goals " + std::to_string(hg) + "-" +
                                  std::to_string(ag) + " flags " + hex(flags));
                }
            }
        }
    }

    // The cup's year and the fixture's: dates in an earlier year than the game's (a cup whose year is before it).
    {
        for (const std::uint32_t year : {std::uint32_t(1999), std::uint32_t(2000), std::uint32_t(2001), std::uint32_t(2002)}) {
            for (const std::uint32_t code : {std::uint32_t(0), std::uint32_t(4), std::uint32_t(1)}) {
                Tables t(w, multi);
                t.round(0, 0x78, 1, 0).ready(0, 0, 1);
                t.m.h(multi.object + 0x40, year);
                t.round(1, 0x78, 2, 0, 0x100, 0);
                Spec spec{.cup = multi, .home_goals = 1, .away_goals = 1, .pokes = t.m};
                spec.answers = {{decider_tie, code}};
                add(spec, "cup year " + std::to_string(year) + " decider " + std::to_string(code));
            }
        }
    }

    // The next tie's scheduling (0x1047da74..): no next round; the next round past the last; the next tie not waiting
    // (state) or missing a club; no competition object; the record gone; the stronger-side flag; the bit 0x20; the
    // last round; the date (the round starting before, at or after two days from today).
    {
        const auto home_through = [&](Tables& t, const std::string& label, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {},
                                      std::vector<Case::Poke> events = {}) {
            Spec spec{.cup = multi, .home_goals = 2, .pokes = t.m};
            spec.answers = std::move(answers);
            spec.events = std::move(events);
            add(spec, label);
        };
        {
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).round(1, 0x78, 1, 0).ready(0, 0, 1);
            t.m.b(multi.object + 0xa9, 1);
            home_through(t, "next: a9 = 1");
        }
        {
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).round(1, 0x78, 1, 0).ready(0, 0, 1);
            t.row(0, 0, 0, 1, 0, 3, 1);
            t.m.b(multi.object + 0xa9, 2);
            home_through(t, "next: target past a9");
        }
        {
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).round(1, 0x78, 1, 0).ready(0, 0, 1);
            t.row(1, 0, 2, 3, -1, -1, 0, 0, 0, 0);
            home_through(t, "next: state 0");
        }
        {
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).round(1, 0x78, 1, 0).ready(0, 0, 1);
            t.row(0, 0, 0, 1, 0, 1, 0);  // side word 0: the winner goes into the next row's second club
            t.row(1, 0, -1, 3, -1, -1, 0);
            home_through(t, "next: first club missing");
        }
        {
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).round(1, 0x78, 1, 0).ready(0, 0, 1);
            t.row(1, 0, 2, -1, -1, -1, 0);
            home_through(t, "next: second club missing");
        }
        {
            // The competition's object gone from the table when the pairings were made: nobody is told.
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).round(1, 0x78, 1, 0).ready(0, 0, 1);
            const auto table = scenario.global(competitions_slot);
            home_through(t, "next: no competition object", {}, {{pairings, {{4, table + w.p.u32(multi.record) * 4, 0}}}});
        }
        {
            // The record pointer gone when the pairings were made: the copy's id and record are -1 and 0 (the news
            // is answered as not followed, so nothing reads the record again).
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).round(1, 0x78, 1, 0).ready(0, 0, 1);
            home_through(t, "next: record gone", {{news_on, 0}}, {{pairings, {{4, multi.object + 4, 0}}}});
        }
        for (const bool second : {false, true}) {
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).round(1, 0x78, 1, 0).ready(0, 0, 1);
            home_through(t, std::string("next: ") + (second ? "second" : "first") + " club null", {{news_on, 0}},
                         {{pairings, {{4, w.entrant(multi, second ? 3 : 0), 0}}}});  // the winner (entrant 0) is the next tie's first club
        }
        for (const bool stronger_first : {true, false}) {
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).round(1, 0x78, 1, 0).ready(0, 0, 1);
            // The winner (entrant 0) is the next tie's first club, entrant 3 its second.
            t.m.h(t.round_at(1) + 0xe, 0x800).b(w.entrant(multi, 0) + 4, stronger_first ? 1 : 5).b(w.entrant(multi, 3) + 4, stronger_first ? 5 : 1);
            home_through(t, std::string("next: stronger ") + (stronger_first ? "second" : "first"));
        }
        for (const std::uint32_t rule : {std::uint32_t(1), std::uint32_t(2)}) {
            for (const std::uint32_t needed : {std::uint32_t(0), std::uint32_t(2)}) {
                for (const std::uint32_t flags : {std::uint32_t(0), std::uint32_t(0x800)}) {
                    Tables t(w, multi);
                    t.round(0, 0x78, 1, 0).round(1, 0x78, rule, needed).ready(0, 0, 1);
                    t.m.h(t.round_at(1) + 0xe, flags);
                    home_through(t, "next: rule " + std::to_string(rule) + " needing " + std::to_string(needed) + " flags " + hex(flags));
                }
            }
        }
        {
            Tables t(w, multi);
            t.round(multi.rounds - 2, 0x78, 1, 0).round(multi.rounds - 1, 0x78, 1, 0).ready(multi.rounds - 2, 0, multi.rounds - 1);
            Spec spec{.cup = multi, .round = multi.rounds - 2, .home_goals = 2, .pokes = t.m};
            add(spec, "next: into the last round");
        }
        {
            // The round's year is the cup's plus its offset byte: offsets that make it the game's year, the one before and the
            // one after (the cup's year is not always the game's).
            const auto offset = [&](std::int32_t year_difference) { return std::uint32_t(std::int32_t(w.today_year()) - multi.year + year_difference) & 0xff; };
            for (const auto& [start, year_offset, name] : {std::tuple{std::uint32_t(100), offset(0), "starts before"}, {std::uint32_t(0x164), offset(0), "starts after"},
                                                           {std::uint32_t(0x162), offset(0), "starts today"}, {std::uint32_t(0x164), offset(-1), "year before"},
                                                           {std::uint32_t(100), offset(1), "year after"}, {std::uint32_t(0x165), offset(0), "starts in two days"}}) {
                for (const std::uint32_t rule : {std::uint32_t(1), std::uint32_t(2)}) {
                    Tables t(w, multi);
                    t.round(0, 0x78, 1, 0).round(1, 0x78, rule, 0, start, year_offset).ready(0, 0, 1);
                    home_through(t, std::string("next: round ") + name + " rule " + std::to_string(rule));
                }
            }
        }
        // A two-legged next round's dates over a week (the weekday of the leg's date).
        for (std::uint32_t start = 0x164; start < 0x164 + 8; ++start) {
            Tables t(w, multi);
            t.round(0, 0x78, 1, 0).round(1, 0x78, 2, 1, start, 0).ready(0, 0, 1);
            const auto offset = std::uint32_t(std::int32_t(w.today_year()) - multi.year) & 0xff;
            t.m.b(t.round_at(1) + 6, offset);
            home_through(t, "next: second leg starting " + std::to_string(start));
        }
        // The news of the tie: followed by the first club, the second, neither; in either order; answered or filed for real.
        for (const auto& [first, second, order, name] : {std::tuple{1u, 0u, 1u, "first followed, in order"}, {1u, 0u, 0u, "first followed, swapped"},
                                                         {0u, 1u, 1u, "second followed, in order"}, {0u, 1u, 0u, "second followed, swapped"}}) {
            for (const bool filed : {false, true}) {
                Tables t(w, multi);
                t.round(0, 0x78, 1, 0).round(1, 0x78, 1, 0).ready(0, 0, 1);
                Spec spec{.cup = multi, .home_goals = 2, .pokes = t.m};
                // The entrant virtual (0x1047b360) asks the news manager too: answered, so the dispatches counted are this routine's.
                spec.answers = {{news_on, 1}, {entrant_news, 0}};
                if (!filed) spec.answers.insert(spec.answers.end(), {{news_start, 0}, {news_set, 0}, {news_file, 0}});
                auto item = make(w, spec, std::string("news ") + name + (filed ? ", filed" : ", answered"));
                item.answers.erase(std::remove_if(item.answers.begin(), item.answers.end(), [](const auto& a) { return a.first == news_on; }), item.answers.end());
                // The dispatches of the news query: the first club, the second (only when the first is not followed), then
                // the order question about the first club.
                // The prize routine (0x10485930, called before) asks once: the dispatches counted start after it.
                const unsigned skip = 1;
                if (first) item.nth_answers = {{news_on, skip + 1, 1}, {news_on, skip + 2, order}};
                else item.nth_answers = {{news_on, skip + 1, 0}, {news_on, skip + 2, second}, {news_on, skip + 3, order}};
                cases.push_back(std::move(item));
            }
        }
    }

    // Series fixtures (flags 0x2000): the marks at +0x3d/+0x3e that the leg decider (0x10482640) counts down, with
    // the outcomes it answers around postponed (4: all marks above -5; 1 or 2 once one is below -5) and the other
    // deciders' outcomes. The 0x50 bytes handed to 0x104888f0 are captured and compared with the original's.
    {
        for (const std::uint32_t flags : {std::uint32_t(0x2000), std::uint32_t(0x2010), std::uint32_t(0x2002)}) {
            for (const auto& [mh, ma] : {std::pair{0u, 0u}, {3u, 5u}, {0xfdu, 0u}, {0xfbu, 0xfcu}, {0xfau, 0xfau}, {0xf9u, 1u}, {0u, 0xfau}, {7u, 7u}}) {
                for (const auto& [hg, ag] : {std::pair{1u, 1u}, {2u, 0u}, {0u, 2u}}) {
                    for (const std::uint32_t forced : {std::uint32_t(0xff), std::uint32_t(4), std::uint32_t(0)}) {
                        const bool answered = forced != 0xff;
                        Tables t(w, multi);
                        t.round(0, 0x78, 2, 0).ready(0, 0, 1);
                        t.row(0, 0, 0, 1, 0, 1, 1, 1, 1);  // legs = 2: the third match asks the tie decider; try legs 1 below
                        if (!answered) t.row(0, 0, 0, 1, 0, 1, 1, 0, 0);
                        Spec spec{.cup = multi, .home_goals = hg, .away_goals = ag, .flags = flags, .pokes = t.m};
                        spec.marks_home = mh;
                        spec.marks_away = ma;
                        // answered: the tie decider is forced to 4 (postponed) or 0 (level) whatever the marks give.
                        if (answered) spec.answers = {{decider_tie, forced}};
                        add(spec, std::string("series flags ") + hex(flags) + " marks " + hex(mh) + "/" + hex(ma) + " goals " + std::to_string(hg) + "-" +
                                      std::to_string(ag) + (answered ? " forced " + std::to_string(forced) : ""));
                    }
                }
            }
        }
    }
}
}
}
