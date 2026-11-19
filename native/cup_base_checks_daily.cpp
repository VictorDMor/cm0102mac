// CUP.CPP, block `daily`: cases for the daily update 0x10482ca0, whether a cup is finished 0x10485410, a tie's and a
// fixture's outcome 0x10482640 / 0x1047e540, the screens' fixture 0x1047bbb0, the two-leg bookkeeping 0x104864d0 and
// the calendar setters 0x104854f0 / 0x10485570 / 0x10485600. A scratch cup (a copy of a real cup's first bytes with its
// vtable, stages, rounds and match rows built in the scratch area) steers each branch; its virtual calls land on
// dummy routines of other modules that a case answers, so the derived cups' code is not what is compared. The career's
// own cups run too, with the real virtuals. Labels start with "daily:" (CM_CUP_BLOCK=daily).
#include "cup_base_checks.hpp"
#include <cstdio>
#include <set>
#include <string>

namespace cm::aot::cup_base_checks {
namespace {
constexpr std::uint32_t daily_update = 0x10482ca0, finished = 0x10485410, tie_outcome = 0x10482640,
                        fixture_winner = 0x1047e540, screen_fixture = 0x1047bbb0, legs = 0x104864d0,
                        draw_date = 0x104854f0, fixture_date = 0x10485570, round_slot = 0x10485600;
constexpr std::int32_t competitions_slot = -0x7b5c, competition_count_slot = -0x7b60;
constexpr std::uint32_t area = scratch_base + 0x80000;
// Routines of other modules the scratch cup's virtual slots point at: a case answers them (the answer is r3).
constexpr std::uint32_t stage_done = 0x100c76c0, stage_open = 0x10192130, other_virtual = 0x102aed80;
constexpr std::uint32_t rounds_draw = 0x1047e980, rounds_advance = 0x1047ef50, fix_man = 0x1048aea0;
std::string hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}
Write w8(std::uint32_t at, std::uint32_t value) { return {1, at, value & 0xff}; }
Write w16(std::uint32_t at, std::uint32_t value) { return {2, at, value & 0xffff}; }
Write w32(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }
void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }
std::vector<Write> joined(std::vector<Write> a, const std::vector<Write>& b) { append(a, b); return a; }
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;

struct World {
    const Scenario& s;
    std::vector<std::uint32_t> cups;
    explicit World(const Scenario& scenario) : s(scenario) {
        const auto table = s.global(competitions_slot);
        const auto count = s.global(competition_count_slot);
        std::set<std::uint32_t> seen;
        const auto add = [&](std::uint32_t object) { if (object && is_cup(object) && seen.insert(object).second) cups.push_back(object); };
        for (std::uint32_t id = 0; table && id < count && id < 4096; ++id) {
            const auto object = s.r32(table + id * 4);
            if (!object) continue;
            add(object);
            const auto last = std::int32_t(s.r32(object + 0x30));
            const auto stages = s.r32(object + 0xc);
            for (std::int32_t n = 0; stages && n <= last && n < 64; ++n) add(s.r32(stages + std::uint32_t(n) * 4));
        }
    }
    bool is_cup(std::uint32_t object) const {
        if (s.r32(object) < 0x108b0000 || s.r32(object) >= 0x10a00000 || (s.r32(object + 0x40) & 0xff) != 1) return false;
        for (std::uint32_t slot = 0x08; slot <= 0xa4; slot += 4) {
            const auto descriptor = s.r32(s.r32(object) + slot);
            if (descriptor < 0x108b0000 || descriptor >= 0x108c0000) return false;
            const auto code = s.r32(descriptor);
            if (code >= module_start && code < module_end) return true;
        }
        return false;
    }
};

// ---- The scratch cup ------------------------------------------------------------------------------------------------
// Layout in the scratch area: the date the day arrives with, the cup (a copy of 0x1a0 bytes of a real one), its vtable
// (the base class's, four slots redirected to dummy descriptors), two stage vtables, the stages, the rounds (0x78
// each), the per-round match arrays and rows, a statistics object and the entrant table.
constexpr std::uint32_t when_at = area, cup_at = area + 0x100, vtable_at = area + 0x400, descriptors_at = area + 0x500,
                        stage_vtable_done = area + 0x600, stage_vtable_open = area + 0x700, stages_at = area + 0x800,
                        stage_objects = area + 0x840, rounds_at = area + 0x900, match_pointers = area + 0xd00,
                        match_rows = area + 0x4000, stats_at = area + 0xf00, entrants_at = area + 0x1000,
                        clubs_at = area + 0x1200, fixtures_at = area + 0x1400, ties_at = area + 0x1800;
constexpr std::uint32_t real_toc = 0x108bba90;

struct RoundSpec {
    std::int32_t label = 0, day = 100, year_byte = 0;
    std::uint32_t matches_byte = 1;  // byte +0x34 of the round's first match row
};
struct CupSpec {
    std::int32_t owner = -1, b0 = 1, active = 1, closed = 0, last = 1, total = 3, a8 = 0, a9 = 0, year = 2001;
    std::vector<bool> stage_finished = {true, true};
    bool stats = true;
    std::vector<RoundSpec> rounds;
    std::int32_t count_override = 0x7fffffff;
};
std::vector<Write> make_cup(const Scenario& s, std::uint32_t base, const CupSpec& spec) {
    std::vector<Write> memory;
    for (std::uint32_t offset = 0; offset < 0x1a0; offset += 4) memory.push_back(w32(cup_at + offset, s.r32(base + offset)));
    const auto real_vtable = s.r32(base);
    for (std::uint32_t offset = 0; offset < 0xa8; offset += 4) {
        memory.push_back(w32(vtable_at + offset, s.r32(real_vtable + offset)));
        memory.push_back(w32(stage_vtable_done + offset, s.r32(real_vtable + offset)));
        memory.push_back(w32(stage_vtable_open + offset, s.r32(real_vtable + offset)));
    }
    // Descriptors: {code, toc} for a finished stage, an open stage and an ordinary virtual.
    const std::uint32_t codes[3] = {stage_done, stage_open, other_virtual};
    for (std::uint32_t n = 0; n < 3; ++n) {
        memory.push_back(w32(descriptors_at + n * 8, codes[n]));
        memory.push_back(w32(descriptors_at + n * 8 + 4, real_toc));
    }
    for (const auto slot : {0x18u, 0x30u, 0x38u}) memory.push_back(w32(vtable_at + slot, descriptors_at + 16));
    memory.push_back(w32(vtable_at + 0x1c, descriptors_at + 16));
    memory.push_back(w32(stage_vtable_done + 0x18, descriptors_at));
    memory.push_back(w32(stage_vtable_open + 0x18, descriptors_at + 8));
    for (const auto table : {stage_vtable_done, stage_vtable_open}) memory.push_back(w32(table + 0x1c, descriptors_at + 16));
    memory.push_back(w32(cup_at, vtable_at));
    memory.push_back(w32(cup_at + 0x08, spec.stats ? stats_at : 0));
    memory.push_back(w32(cup_at + 0x0c, stages_at));
    memory.push_back(w32(cup_at + 0x2c, std::uint32_t(spec.total)));
    memory.push_back(w32(cup_at + 0x30, std::uint32_t(spec.last)));
    const auto count = spec.count_override != 0x7fffffff ? std::uint32_t(spec.count_override) : std::uint32_t(spec.rounds.size());
    memory.push_back(w16(cup_at + 0x3c, count));
    memory.push_back(w16(cup_at + 0x40, std::uint32_t(spec.year)));
    memory.push_back(w8(cup_at + 0x44, std::uint32_t(spec.owner)));
    memory.push_back(w8(cup_at + 0x45, std::uint32_t(spec.active)));
    memory.push_back(w8(cup_at + 0x49, std::uint32_t(spec.closed)));
    memory.push_back(w32(cup_at + 0xa0, rounds_at));
    memory.push_back(w8(cup_at + 0xa8, std::uint32_t(spec.a8)));
    memory.push_back(w8(cup_at + 0xa9, std::uint32_t(spec.a9)));
    memory.push_back(w32(cup_at + 0xac, match_pointers));
    memory.push_back(w8(cup_at + 0xb0, std::uint32_t(spec.b0)));
    for (std::int32_t n = 0; n < 8; ++n) {
        const bool done = n < std::int32_t(spec.stage_finished.size()) ? spec.stage_finished[std::size_t(n)] : true;
        memory.push_back(w32(stage_objects + std::uint32_t(n) * 0x20, done ? stage_vtable_done : stage_vtable_open));
        memory.push_back(w32(stages_at + std::uint32_t(n) * 4, stage_objects + std::uint32_t(n) * 0x20));
    }
    for (std::size_t n = 0; n < spec.rounds.size() && n < 8; ++n) {
        const auto at = rounds_at + std::uint32_t(n) * 0x78;
        memory.push_back(w16(at, std::uint32_t(spec.rounds[n].day)));
        memory.push_back(w8(at + 2, std::uint32_t(spec.rounds[n].year_byte)));
        memory.push_back(w16(at + 8, std::uint32_t(spec.rounds[n].label)));
        memory.push_back(w32(match_pointers + std::uint32_t(n) * 4, match_rows + std::uint32_t(n) * 0x100));
        memory.push_back(w8(match_rows + std::uint32_t(n) * 0x100 + 0x34, spec.rounds[n].matches_byte));
    }
    return memory;
}
std::vector<Write> date_word(std::uint32_t at, std::int32_t day, std::int32_t year, std::uint32_t leap = 0) {
    return {w16(at, std::uint32_t(day)), w16(at + 2, std::uint32_t(year)), w8(at + 4, leap), w8(at + 5, 0x5a), w8(at + 6, 0xa5),
            w8(at + 7, 0x3c)};
}
const Answers virtual_answers = {{stage_done, 1}, {stage_open, 0}, {other_virtual, 0}, {0x1000cfc0, 0}, {0x10456290, 0},
                                 {0x104573c0, 0}, {rounds_draw, 0}, {rounds_advance, 0}};
CUP_CASES("daily") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.cups.size() < 3) throw std::runtime_error("daily: the career has fewer than three cup objects");
    std::fprintf(stderr, "daily: %zu cup objects\n", w.cups.size());
    const auto base = w.cups.front();
    const auto today = scenario.toc(-0x7b80);
    const auto phase_byte = scenario.toc(-0x79ec);
    const auto label = [](const std::string& text) { return "daily: " + text; };

    // ---- the retirement hook: a retired Inter-American Cup returns 0 and touches nothing ----------------------------
    {
        const auto vtable = scenario.toc(-0x5cc0);
        auto retired = make_cup(scenario, base, CupSpec{.owner = 0, .active = 0, .rounds = {{0x8c, 100, 0}}});
        retired.push_back(w32(cup_at, vtable));
        for (const auto phase : {0u, 1u})
            for (const auto year : {2001, 1999, 1998, 1990}) {
                auto memory = joined(retired, joined(date_word(when_at, 100, year), {w16(today + 2, std::uint32_t(year)), w8(phase_byte, 1)}));
                cases.push_back({.entry = daily_update, .registers = {{3, cup_at}, {4, when_at}, {5, phase}}, .memory = memory,
                                 .answers = virtual_answers, .label = label("retired object, year " + std::to_string(year) + " phase " + std::to_string(phase))});
            }
        // The same date with an object of another vtable: the ordinary path.
        auto ordinary = make_cup(scenario, base, CupSpec{.rounds = {{0x8c, 100, 0}}});
        cases.push_back({.entry = daily_update, .registers = {{3, cup_at}, {4, when_at}, {5, 0}},
                         .memory = joined(ordinary, joined(date_word(when_at, 100, 2001), {w16(today + 2, 2001), w8(phase_byte, 1)})),
                         .answers = virtual_answers, .label = label("not the retired vtable")});
    }

    // ---- the stages: finished or not, the statistics, the closing ---------------------------------------------------
    {
        const std::vector<std::vector<bool>> finishes = {{true, true, true}, {false, true, true}, {true, false, true}, {true, true, false}};
        for (const auto owner : {-1, 0})
            for (const auto b0 : {0, 1})
                for (const auto last : {-1, 0, 1, 2})
                    for (const auto extra : {0, 1})
                        for (const auto closed : {0, 1})
                            for (const auto stats : {true, false})
                                for (std::size_t pattern = 0; pattern < (last < 0 ? 1 : last == 0 ? 2 : finishes.size()); ++pattern) {
                                    CupSpec spec{.owner = owner, .b0 = b0, .closed = closed, .last = last, .total = last + 1 + extra,
                                                 .stage_finished = finishes[pattern], .stats = stats};
                                    cases.push_back({.entry = daily_update, .registers = {{3, cup_at}, {4, when_at}, {5, pattern & 1}},
                                                     .memory = joined(make_cup(scenario, base, spec), joined(date_word(when_at, 100, 2001), {w8(phase_byte, 0)})),
                                                     .answers = virtual_answers,
                                                     .label = label("stages owner " + std::to_string(owner) + " b0 " + std::to_string(b0) + " last " +
                                                                    std::to_string(last) + " total " + std::to_string(spec.total) + " closed " +
                                                                    std::to_string(closed) + " stats " + std::to_string(stats) + " pattern " +
                                                                    std::to_string(pattern))});
                                }
        // A stage count above the table's (the loop's bound is the word +0x30, signed) and a negative total.
        for (const auto total : {0, -1, 5, 0x7fffffff})
            cases.push_back({.entry = daily_update, .registers = {{3, cup_at}, {4, when_at}, {5, 0}},
                             .memory = joined(make_cup(scenario, base, CupSpec{.last = 1, .total = total}), joined(date_word(when_at, 100, 2001), {w8(phase_byte, 0)})),
                             .answers = virtual_answers, .label = label("stages total " + std::to_string(total))});
    }

    // ---- the final round's day and the morning/evening loops ---------------------------------------------------------
    // A cup with `rounds`; today's date relative to the last round's (the same day, the day before/after, another year).
    {
        const std::int32_t labels[] = {0x8c, 0x1, 0x8d};
        struct Today { const char* name; std::int32_t day_shift, year_shift; };
        const Today days[] = {{"same", 0, 0}, {"day before", -1, 0}, {"day after", 1, 0}, {"year before", 0, -1}, {"year after", 0, 1},
                              {"other both", 5, 1}};
        for (const auto phase : {0u, 0x100u, 1u, 0xffu}) {
            for (const auto flag : {1u, 0u}) {
                for (const auto count : {-1, 0, 1, 3}) {
                    for (const auto last_label : labels) {
                        for (const auto b0 : {0, 1}) {
                            for (const auto& today_spec : days) {
                                CupSpec spec{.b0 = b0, .last = -1, .total = 0, .stage_finished = {}};
                                const auto rows = std::max(count, 0);
                                for (std::int32_t n = 0; n < rows; ++n)
                                    spec.rounds.push_back({n + 1 == rows ? last_label : n + 1, 50 + 30 * n, n >= 2 ? 1 : 0, 1});
                                spec.count_override = count;
                                if (rows) { spec.rounds.back().day = 200; spec.rounds.back().year_byte = 0; }
                                auto memory = make_cup(scenario, base, spec);
                                const auto day = rows ? 200 + today_spec.day_shift : 100 + today_spec.day_shift;
                                append(memory, date_word(when_at, day, spec.year + today_spec.year_shift));
                                memory.push_back(w8(phase_byte, flag));
                                cases.push_back({.entry = daily_update, .registers = {{3, cup_at}, {4, when_at}, {5, phase}}, .memory = memory,
                                                 .answers = virtual_answers,
                                                 .label = label("final round count " + std::to_string(count) + " label " + hex(std::uint32_t(last_label)) +
                                                                " b0 " + std::to_string(b0) + " today " + today_spec.name + " phase " + hex(phase) + " flag " + hex(flag))});
                            }
                        }
                    }
                }
            }
        }
    }

    // The morning loop: rounds a9.. before today draw (the draw advances a9 by a poke); rounds after stop it.
    {
        struct Layout { std::vector<std::int32_t> days; std::vector<std::int32_t> years; };
        const std::vector<Layout> layouts = {{{50, 80, 120, 150}, {0, 0, 0, 0}}, {{50, 80, 120, 150}, {-1, -1, 0, 1}},
                                             {{150, 120, 80, 50}, {1, 0, 0, 0}}, {{100, 100, 100, 100}, {0, 0, 0, 0}},
                                             {{200, 40, 40, 40}, {-1, 0, 0, 0}}, {{100, 99, 101, 100}, {0, 0, 0, 0}}};
        for (const auto& layout : layouts) {
            for (const auto start : {0, 1, 3, -1}) {
                for (const auto after : {start + 1, 4, start}) {
                    for (const auto today_day : {100, 99, 101, -300}) {
                        for (const auto today_year : {2001, 2000, 2002}) {
                            CupSpec spec{.b0 = 0, .last = -1, .total = 0, .a9 = start, .stage_finished = {}};
                            for (std::size_t n = 0; n < 4; ++n) spec.rounds.push_back({std::int32_t(n + 1), layout.days[n], layout.years[n], 1});
                            auto memory = joined(make_cup(scenario, base, spec), joined(date_word(when_at, today_day, today_year), {w8(phase_byte, 0)}));
                            cases.push_back({.entry = daily_update, .registers = {{3, cup_at}, {4, when_at}, {5, 0}}, .memory = memory,
                                             .answers = virtual_answers, .pokes = {{rounds_draw, {w8(cup_at + 0xa9, std::uint32_t(after))}}},
                                             .label = label("morning rounds from " + std::to_string(start) + " to " + std::to_string(after) +
                                                            " today " + std::to_string(today_day) + "/" + std::to_string(today_year) + " layout " +
                                                            std::to_string(layout.days[0]) + "," + std::to_string(layout.years[0]))});
                        }
                    }
                }
            }
        }
    }

    // The evening loop: a round's day (or any day while the round's matches are not all played) advances the cup.
    {
        const std::vector<std::vector<std::int32_t>> day_layouts = {{50, 80, 120, 150}, {100, 100, 100, 100}, {150, 120, 80, 50}};
        for (const auto& days : day_layouts) {
            for (const auto start : {0, 1, 2, 3}) {
                for (const auto after : {start + 1, 4, start}) {
                    for (const auto today_day : {100, 120, 50, 99}) {
                        for (const auto today_year : {2001, 2002}) {
                            for (const auto played : {0u, 1u, 0x80u}) {
                                CupSpec spec{.b0 = 0, .last = -1, .total = 0, .a8 = start, .stage_finished = {}};
                                for (std::size_t n = 0; n < 4; ++n) spec.rounds.push_back({std::int32_t(n + 1), days[n], std::int32_t(n >= 3 ? 1 : 0), played});
                                auto memory = joined(make_cup(scenario, base, spec), joined(date_word(when_at, today_day, today_year), {w8(phase_byte, 0)}));
                                cases.push_back({.entry = daily_update, .registers = {{3, cup_at}, {4, when_at}, {5, 1}}, .memory = memory,
                                                 .answers = virtual_answers, .pokes = {{rounds_advance, {w8(cup_at + 0xa8, std::uint32_t(after))}}},
                                                 .label = label("evening rounds from " + std::to_string(start) + " to " + std::to_string(after) +
                                                                " today " + std::to_string(today_day) + "/" + std::to_string(today_year) + " played " + hex(played) +
                                                                " layout " + std::to_string(days[0]))});
                            }
                        }
                    }
                }
            }
        }
        // The evening's inactive cup and a cup finished before the loop.
        for (const auto active : {0, 1, 0x80})
            for (const auto b0 : {0, 1}) {
                CupSpec spec{.b0 = b0, .active = active, .last = -1, .total = 0, .stage_finished = {}, .rounds = {{0x8c, 100, 0, 1}, {0x8c, 100, 0, 1}}};
                cases.push_back({.entry = daily_update, .registers = {{3, cup_at}, {4, when_at}, {5, 1}},
                                 .memory = joined(make_cup(scenario, base, spec), joined(date_word(when_at, 100, 2001), {w8(phase_byte, 1)})),
                                 .answers = virtual_answers, .label = label("evening active " + std::to_string(active) + " b0 " + std::to_string(b0))});
            }
    }

    // The career's own cups: the real virtuals, both passes, today's real date (and the date of the cup's last round).
    for (const auto cup : w.cups) {
        for (const auto phase : {0u, 1u}) {
            auto memory = date_word(when_at, std::int32_t(scenario.r32(today) >> 16), std::int32_t(scenario.r32(today) & 0xffff));
            cases.push_back({.entry = daily_update, .registers = {{3, cup}, {4, when_at}, {5, phase}}, .memory = memory,
                             .label = label("career cup " + hex(cup) + " phase " + std::to_string(phase))});
        }
    }
}

// ---- Small routines: finished, the calendar setters -----------------------------------------------------------------
struct Random {
    std::uint32_t state;
    std::uint32_t next() { state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state; }
    std::uint32_t pick(std::initializer_list<std::uint32_t> values) { return *(values.begin() + next() % values.size()); }
};

CUP_CASES("daily") {
    if (scenario.match) return;
    const World w(scenario);
    const auto base = w.cups.front();
    const auto label = [](const std::string& text) { return "daily: " + text; };
    const auto phase_byte = scenario.toc(-0x79ec);
    (void)phase_byte;

    // 0x10485410: finished. The byte +0xb0, the mode (compared whole with 1), the owner, the stages' answers.
    {
        const std::vector<std::vector<bool>> finishes = {{true, true, true}, {false, true, true}, {true, false, true}, {true, true, false}};
        for (const auto b0 : {0, 1, 0x80})
            for (const auto mode : {0u, 1u, 2u, 0xffffffffu, 0x10001u})
                for (const auto owner : {-1, 0})
                    for (const auto last : {-1, 0, 1, 2})
                        for (const auto extra : {0, 1})
                            for (std::size_t pattern = 0; pattern < (last < 0 ? 1 : last == 0 ? 2 : finishes.size()); ++pattern) {
                                CupSpec spec{.owner = owner, .b0 = b0, .last = last, .total = last + 1 + extra, .stage_finished = finishes[pattern]};
                                cases.push_back({.entry = finished, .registers = {{3, cup_at}, {4, mode}}, .memory = make_cup(scenario, base, spec),
                                                 .answers = virtual_answers,
                                                 .label = label("finished b0 " + std::to_string(b0) + " mode " + hex(mode) + " owner " + std::to_string(owner) +
                                                                " last " + std::to_string(last) + " extra " + std::to_string(extra) + " pattern " + std::to_string(pattern))});
                            }
        for (const auto cup : w.cups)
            for (const auto mode : {0u, 1u})
                cases.push_back({.entry = finished, .registers = {{3, cup}, {4, mode}}, .label = label("finished career cup " + hex(cup) + " mode " + std::to_string(mode))});
    }

    // The calendar setters: rows of a scratch table filled with a pattern, so a stray store shows.
    {
        std::vector<Write> pattern;
        for (std::uint32_t offset = 0; offset < 0x500; offset += 4) pattern.push_back(w32(rounds_at - 0x100 + offset, 0xa5c3e17bu + offset * 0x01010101u));
        const std::uint32_t days[] = {1, 15, 28, 29, 30, 31, 0, 32, 0xffffffffu, 0x10001u, 0x8000u};
        const std::uint32_t months[] = {0, 5, 11, 12, 0xffffffffu, 0x100u, 0x1ffu, 3};
        const std::uint32_t years[] = {2001, 2002, 1999, 0xffffu, 0x1f7d1u, 0u, 0x8000u};
        const std::uint32_t weekdays[] = {0xffffffffu, 0, 3, 6, 7, 0xffffu, 0x10003u, 0x8000u, 0xfffffff9u};
        const std::uint32_t shifts[] = {0, 1, 2, 0xffffu, 0xffffffffu, 3, 0x10001u, 0x8000u};
        const std::uint32_t rounds[] = {0, 1, 2, 3, 0xffffu, 0x10002u, 0xffffffffu, 5};
        Random random{0x9e3779b9u};
        for (unsigned n = 0; n < 160; ++n) {
            const auto round = rounds[n % 8];
            const auto day = n < 40 ? 1 + n % 28 : days[random.next() % 11], month = n < 40 ? n % 12 : months[random.next() % 8];
            const auto year = years[random.next() % 7], weekday = weekdays[random.next() % 9], shift = shifts[random.next() % 8];
            const auto name = " round " + hex(round) + " day " + hex(day) + " month " + hex(month) + " year " + hex(year) + " weekday " + hex(weekday) + " shift " + hex(shift);
            cases.push_back({.entry = draw_date, .registers = {{3, rounds_at}, {4, round}, {5, day}, {6, month}, {7, year}, {8, weekday}, {9, shift}},
                             .memory = pattern, .label = label("draw date" + name)});
            const auto kind = random.pick({0, 1, 2, 0xff, 0x100, 0x7f}), tail = random.next();
            const auto stack = scenario.sp() + 0x38;
            cases.push_back({.entry = fixture_date, .registers = {{3, rounds_at}, {4, round}, {5, day}, {6, month}, {7, year}, {8, weekday}, {9, kind}, {10, shift}},
                             .memory = joined(pattern, {w32(stack, tail)}), .label = label("fixture date" + name + " kind " + hex(kind))});
        }
        const std::uint32_t slots[] = {0, 1, 3, 7, 8, 0xffffffffu, 0x10005u, 0x8000u, 0xfff9u, 0x10008u};
        const std::uint32_t firsts[] = {0xffffffffu, 0, 3, 6, 7, 0xfffeu, 0x10006u, 0x8000u, 0xffffu};
        const std::uint32_t seconds[] = {0xffffffffu, 0, 1, 2, 3, 0xfeu, 0x102u, 0x180u, 0x80u};
        const std::uint32_t thirds[] = {0xffffffffu, 0, 1, 3, 4, 0xfeu, 0x103u, 0x180u, 0x80u};
        for (unsigned n = 0; n < 120; ++n) {
            const auto round = rounds[random.next() % 8];
            // Mostly valid arguments; a share of each range's failures one at a time.
            auto slot = slots[random.next() % 4], first = firsts[1 + random.next() % 4], second = seconds[1 + random.next() % 3], third = thirds[1 + random.next() % 3];
            if (n % 5 == 1) slot = slots[random.next() % 10];
            if (n % 5 == 2) first = firsts[random.next() % 9];
            if (n % 5 == 3) second = seconds[random.next() % 9];
            if (n % 5 == 4) third = thirds[random.next() % 9];
            const auto tail = random.next();
            cases.push_back({.entry = round_slot, .registers = {{3, rounds_at}, {4, round}, {5, slot}, {6, first}, {7, second}, {8, third}, {9, tail}},
                             .memory = pattern,
                             .label = label("slot round " + hex(round) + " slot " + hex(slot) + " " + hex(first) + " " + hex(second) + " " + hex(third))});
        }
    }
}

// ---- The decisions on a tie and a fixture ----------------------------------------------------------------------------
CUP_CASES("daily") {
    if (scenario.match) return;
    const World w(scenario);
    const auto base = w.cups.front();
    const auto label = [](const std::string& text) { return "daily: " + text; };
    Random random{0x1234567u};
    constexpr std::uint32_t figures[] = {0, 1, 2, 3, 0xffu, 0xfeu, 0x7fu, 0x80u};
    const auto small = [&] { return figures[random.next() % 4]; };
    const std::uint32_t bits[] = {0x10, 0x2000, 0x800, 0x1000, 2, 0x4000, 4, 1, 0x20};
    const auto tie = ties_at, fixture = fixtures_at;
    // 0x10482640: the tie's figures (+0x14..+0x25) and the fixture's flags and figures (+0x43..+0x4a).
    for (const auto kind : {1u, 1u, 0u, 2u, 0xffu}) {
        for (std::uint32_t combo = 0; combo < 512; ++combo) {
            if (kind != 1 && (combo & 0x3) != combo) continue;  // the other kinds read only the first two flags
            std::uint32_t flags = 0;
            for (unsigned bit = 0; bit < 9; ++bit) if (combo >> bit & 1) flags |= bits[bit];
            for (unsigned variant = 0; variant < (kind == 1 ? 3u : 6u); ++variant) {
                std::vector<Write> memory = {w8(tie + 0x33, kind), w16(fixture + 0x38, flags | (variant == 5 ? 0x8000u : 0u))};
                for (std::uint32_t at = 0x14; at <= 0x25; ++at) memory.push_back(w8(tie + at, variant == 0 ? 0 : small()));
                for (const auto at : {0x43u, 0x44u, 0x47u, 0x48u, 0x49u, 0x4au}) memory.push_back(w8(fixture + at, variant == 0 ? 0 : small()));
                memory.push_back(w8(fixture + 0x3d, random.pick({0, 1, 2, 3, 0xfa, 0xfd})));
                memory.push_back(w8(fixture + 0x3e, random.pick({0, 1, 2, 3, 0xfa, 0xfd})));
                cases.push_back({.entry = tie_outcome, .registers = {{3, cup_at}, {4, tie}, {5, fixture}}, .memory = memory,
                                 .label = label("tie kind " + hex(kind) + " flags " + hex(flags) + " variant " + std::to_string(variant))});
            }
        }
    }
    // Figures chosen for the ladders' deep equalities: sums equal, the leg figures equal, the away rule's equal sums.
    for (std::uint32_t pattern = 0; pattern < 3 * 3 * 3 * 3; ++pattern) {
        const std::uint32_t digit[4] = {pattern % 3, pattern / 3 % 3, pattern / 9 % 3, pattern / 27 % 3};
        for (const auto flags : {0u, 2u, 0x4002u, 4u, 0x4000u, 1u, 0x21u, 0x20u, 0x2000u, 0x10u, 0x800u, 0x1000u, 0x4003u, 0x6u}) {
            std::vector<Write> memory = {w8(tie + 0x33, 1), w16(fixture + 0x38, flags)};
            // First side's figures b14,b1f and the second's b15,b1e are 1 each unless a digit lifts one.
            const std::uint32_t fig[16] = {1 + digit[0] % 2, 1 + digit[0] / 2, 1, 1, 1, 1, 1 + digit[1] % 2, 1 + digit[1] / 2, 1, 1, 1 + digit[2] % 2,
                                           1 + digit[2] / 2, 1 + digit[3] % 2, 1 + digit[3] / 2, 1, 1};
            const std::uint32_t at[16] = {0x14, 0x15, 0x18, 0x19, 0x1a, 0x1b, 0x1e, 0x1f, 0x22, 0x23, 0x24, 0x25, 0x22, 0x23, 0x24, 0x25};
            for (unsigned n = 0; n < 12; ++n) memory.push_back(w8(tie + at[n], fig[n] * ((n * 7 + pattern) % 5 == 0 ? 0xffu : 1u)));
            for (const auto place : {0x43u, 0x44u, 0x47u, 0x48u, 0x49u, 0x4au}) memory.push_back(w8(fixture + place, pattern % 4));
            cases.push_back({.entry = tie_outcome, .registers = {{3, cup_at}, {4, tie}, {5, fixture}}, .memory = memory,
                             .label = label("tie ladders " + std::to_string(pattern) + " flags " + hex(flags))});
        }
    }
    // The one-sided kinds' ladders (+0x14/0x15, +0x18/0x19, +0x1a/0x1b), every order of the three pairs.
    for (const auto kind : {0u, 2u, 0x80u})
        for (const auto flags : {0x10u, 0x2000u, 0x2010u, 0u})
            for (std::uint32_t pattern = 0; pattern < 27; ++pattern) {
                std::vector<Write> memory = {w8(tie + 0x33, kind), w16(fixture + 0x38, flags), w8(fixture + 0x3d, 5), w8(fixture + 0x3e, 6)};
                const std::uint32_t order[3] = {pattern % 3, pattern / 3 % 3, pattern / 9 % 3};
                for (unsigned pair = 0; pair < 3; ++pair) {
                    memory.push_back(w8(tie + 0x14 + 4 * pair - (pair == 0 ? 0 : 0), 0));
                    const std::uint32_t first = pair == 0 ? 0x14 : pair == 1 ? 0x18 : 0x1a;
                    memory.push_back(w8(tie + first, order[pair] == 0 ? 2 : 1));
                    memory.push_back(w8(tie + first + 1, order[pair] == 1 ? 2 : 1));
                }
                cases.push_back({.entry = tie_outcome, .registers = {{3, cup_at}, {4, tie}, {5, fixture}}, .memory = memory,
                                 .label = label("tie one-sided kind " + hex(kind) + " flags " + hex(flags) + " ladder " + std::to_string(pattern))});
            }
    // The away rule's equal sums: the second leg's figure above, below or level with the first side's.
    for (const auto flags : {0x4002u, 0x4003u, 0x4022u, 0x4006u})
        for (const auto first_leg : {0u, 1u, 2u, 0xffu})
            for (const auto second_leg : {0u, 1u, 2u, 0x3u})
                for (const auto figure_14 : {1u, 0u}) {
                    std::vector<Write> memory = {w8(tie + 0x33, 1), w16(fixture + 0x38, flags), w8(tie + 0x14, figure_14), w8(tie + 0x15, 1),
                                                 w8(tie + 0x1e, 1), w8(tie + 0x1f, figure_14), w8(tie + 0x22, first_leg), w8(tie + 0x23, second_leg),
                                                 w8(tie + 0x24, 1), w8(tie + 0x25, 1)};
                    cases.push_back({.entry = tie_outcome, .registers = {{3, cup_at}, {4, tie}, {5, fixture}}, .memory = memory,
                                     .label = label("tie away rule flags " + hex(flags) + " legs " + hex(first_leg) + " " + hex(second_leg) + " fig " + hex(figure_14))});
                }
    // 0x1047e540: a fixture's winner. The cup's entrants answer the replay's tie-break.
    const std::uint32_t club_a = clubs_at, club_b = clubs_at + 0x40, club_c = clubs_at + 0x80;
    cases.push_back({.entry = fixture_winner, .registers = {{3, cup_at}, {4, 0}}, .memory = make_cup(scenario, base, CupSpec{.rounds = {{1, 1, 1}}}),
                     .label = label("winner no fixture")});
    for (std::uint32_t combo = 0; combo < 64; ++combo) {
        const std::uint32_t winner_bits[] = {0x10, 0x2000, 2, 1, 0x800, 0x1000};
        std::uint32_t flags = 0;
        for (unsigned bit = 0; bit < 6; ++bit) if (combo >> bit & 1) flags |= winner_bits[bit];
        for (unsigned variant = 0; variant < 14; ++variant) {
            std::vector<Write> memory = make_cup(scenario, base, CupSpec{.rounds = {{1, 1, 1}}});
            const auto count = random.pick({0, 1, 2, 3, 0xffff});
            const std::uint32_t entrant_states[3] = {random.pick({0, 1, 2, 3, 0xff}), random.pick({0, 1, 2, 3, 0xff}), random.pick({0, 1, 2, 3, 0xff})};
            append(memory, {w16(cup_at + 0x3e, count), w32(cup_at + 0xa4, entrants_at), w16(fixture + 0x38, flags)});
            const std::uint32_t clubs[3] = {club_a, club_b, club_c};
            for (unsigned n = 0; n < 3; ++n) {
                memory.push_back(w32(entrants_at + n * 8, clubs[n]));
                memory.push_back(w8(entrants_at + n * 8 + 4, entrant_states[n]));
            }
            memory.push_back(w32(fixture + 0x1c, variant % 3 == 0 ? club_a : variant % 3 == 1 ? club_c : club_b));
            memory.push_back(w32(fixture + 0x20, variant % 2 ? club_b : club_a));
            for (const auto place : {0x43u, 0x44u, 0x47u, 0x48u, 0x49u, 0x4au}) memory.push_back(w8(fixture + place, variant < 3 ? 0 : small()));
            memory.push_back(w8(fixture + 0x3d, random.pick({0, 1, 2, 3, 4, 0xfd})));
            memory.push_back(w8(fixture + 0x3e, random.pick({0, 1, 2, 3, 4, 0xfd})));
            if (variant >= 11) { memory.push_back(w8(fixture + 0x3d, 4)); memory.push_back(w8(fixture + 0x3e, 4)); }
            cases.push_back({.entry = fixture_winner, .registers = {{3, cup_at}, {4, fixture}}, .memory = memory,
                             .label = label("winner flags " + hex(flags) + " variant " + std::to_string(variant))});
        }
    }
}

// ---- The fixture manager's lookups: the screens' fixture and the two legs ------------------------------------------------
CUP_CASES("daily") {
    if (scenario.match) return;
    const World w(scenario);
    const auto base = w.cups.front();
    const auto label = [](const std::string& text) { return "daily: " + text; };
    const auto day_byte = scenario.toc(-0x79ec);
    // The scratch cup with entrants, clubs, rounds and match rows: tie t of round r pairs entrants (2t, 2t+1).
    const auto build = [&](std::int32_t rounds_count) {
        CupSpec spec{.last = -1, .total = 0, .stage_finished = {}};
        for (std::int32_t n = 0; n < rounds_count; ++n) spec.rounds.push_back({n + 3, 50 + 10 * n, n & 1, 1});
        auto memory = make_cup(scenario, base, spec);
        append(memory, {w32(cup_at + 0xa4, entrants_at), w16(cup_at + 0x3e, 8)});
        for (std::uint32_t n = 0; n < 8; ++n) {
            memory.push_back(w32(entrants_at + n * 8, clubs_at + n * 0x40));
            memory.push_back(w32(clubs_at + n * 0x40, 100 + n));
        }
        for (std::int32_t r = 0; r < rounds_count; ++r)
            for (std::uint32_t t = 0; t < 3; ++t) {
                const auto row = match_rows + std::uint32_t(r) * 0x100 + t * 0x36;
                memory.push_back(w16(row, 2 * t % 8));
                memory.push_back(w16(row + 2, (2 * t + 1) % 8));
            }
        return memory;
    };
    // Fixture rows the manager is made to answer with: played (+0x43 = -1) or not.
    const auto fixture_row = [&](std::uint32_t n, bool played) {
        const auto at = fixtures_at + n * 0x60;
        return std::vector<Write>{w8(at + 0x43, played ? 0xff : 0), w16(at + 0x28, 2001), w16(at + 0x2a, 120 + n), w16(at + 0x30, 0x5a5a)};
    };
    for (const auto round : {0u, 1u, 2u, 0xffffffffu, 0x80000000u, 5u})
        for (const auto tie : {0u, 1u, 0xffffffffu, 2u})
            for (const auto start : {0u, 1u, 2u}) {
                for (unsigned script = 0; script < 7; ++script) {
                    // script: 0 none (answer 0), 1 an open fixture, 2 played then open, 3 played x2 then open, 4 played x4 then 0,
                    // 5 played x3 then open (the date steps back), 6 a null first answer after a played one
                    std::vector<Write> memory = build(3);
                    std::vector<Case::NthAnswer> nth;
                    const auto add = [&](unsigned count, std::uint32_t n, bool played) {
                        append(memory, fixture_row(n, played));
                        nth.push_back({fix_man, count, fixtures_at + n * 0x60});
                    };
                    switch (script) {
                    case 1: add(1, 0, false); break;
                    case 2: add(1, 0, true); add(2, 1, false); break;
                    case 3: add(1, 0, true); add(2, 1, true); add(3, 2, false); break;
                    case 4: add(1, 0, true); add(2, 1, true); add(3, 2, true); add(4, 3, true); break;
                    case 5: add(1, 0, true); add(2, 1, true); add(3, 2, true); add(4, 3, false); break;
                    case 6: add(1, 0, true); break;
                    default: break;
                    }
                    memory.push_back(w8(day_byte, start));
                    cases.push_back({.entry = screen_fixture, .registers = {{3, cup_at}, {4, round}, {5, tie}}, .memory = memory,
                                     .answers = {{fix_man, 0}}, .nth_answers = nth,
                                     .label = label("screen fixture round " + hex(round) + " tie " + hex(tie) + " start " + std::to_string(start) + " script " + std::to_string(script))});
                }
            }

    // 0x104864d0: the two legs. The rows' +4 day, +6 year byte, +8 label; the manager's first answer and second.
    for (const auto round : {0u, 1u, 2u, 0xffffffffu, 0x10001u}) {
        for (const auto seed : {0u, 7u, 0x12345678u}) {
            for (unsigned script = 0; script < 5; ++script) {
                std::vector<Write> memory = build(3);
                for (std::uint32_t r = 0; r < 3; ++r) {
                    memory.push_back(w16(rounds_at + r * 0x78 + 4, 120 + 30 * r));
                    memory.push_back(w8(rounds_at + r * 0x78 + 6, r));
                    memory.push_back(w16(rounds_at + r * 0x78 + 8, 7 + r));
                }
                memory.push_back(w16(rounds_at - 0x78 + 8, 0x77));
                std::vector<Case::NthAnswer> nth;
                const auto add = [&](unsigned count, std::uint32_t n, std::uint32_t day, std::uint32_t year) {
                    const auto at = fixtures_at + n * 0x60;
                    append(memory, {w16(at + 0x28, year), w16(at + 0x2a, day), w16(at + 0x30, 0x5a5a), w8(at + 0x43, 0xff)});
                    nth.push_back({fix_man, count, at});
                };
                if (script >= 1) add(1, 0, 130 + script, 2001);
                if (script >= 2) add(2, 1, 140, 2002);
                if (script == 3) add(2, 1, 365, 2000);
                if (script == 4) { nth.pop_back(); add(1, 0, 0x7fff, 0xffff); add(2, 1, 0, 0); }
                cases.push_back({.entry = legs, .registers = {{3, cup_at}, {4, round}, {5, seed}, {6, 0x31337u}}, .memory = memory,
                                 .answers = {{fix_man, 0}}, .nth_answers = nth,
                                 .label = label("legs round " + hex(round) + " seed " + hex(seed) + " script " + std::to_string(script))});
            }
        }
    }
}
}
}
