// DISCIPLINE.CPP review block cases (discipline_checks.hpp): the ruling of a player after a match (0x100e8780) from
// the career snapshot's real people, bans and ruling bodies, and the module's range sampler (0x100e8d10). Case::memory
// writes the match entry (in the scratch area), the conduct bytes, the standing and the draw table's cursor, base and
// entries, which reach every side of each draw.
#include "discipline_checks.hpp"
#include <algorithm>
#include <array>
#include <string>

namespace {
using namespace cm::aot::discipline_checks;
constexpr std::uint32_t review_player = 0x100e8780, sample_range = 0x100e8d10;
constexpr std::uint32_t news_ruling = 0x106a15f0, human_flag = 0x1014c2e0, person_standing = 0x100dbbe0, date_gap = 0x10718130;
constexpr std::uint32_t entry_at = scratch_base + 0x2000, draw_at = scratch_base + 0x1000, argument_at = scratch_base + 0x3000;
constexpr unsigned body_count = 42;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;

// A person of the career with the objects the ruling reaches.
struct Person {
    std::uint32_t id, record, human, conduct, human_id;
    std::int16_t reputation;
};
struct Held {
    std::uint32_t person, entry;
};

void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }

// The match entry RULING_BODY.CPP hands over: person at +0, the flag bytes at +0x2e/+0x2f, the cautioned byte at +0x48.
std::vector<Write> entry_writes(std::uint32_t person, unsigned first, unsigned second, unsigned cautioned) {
    return {{4, entry_at, person}, {4, entry_at + 4, 1}, {1, entry_at + 0x2e, first}, {1, entry_at + 0x2f, second}, {1, entry_at + 0x48, cautioned}};
}

// The draw table with room for three draws, the cursor `wrap_at` - 1 draws before the last entry so the draw numbered
// `wrap_at` (1..3; 0 none) wraps to the real table and redraws the base. `values` are the entries the draws before
// it read, in order; `base` is the base word.
std::vector<Write> draw_writes(const Scenario& s, std::uint32_t base, const std::vector<std::uint16_t>& values, unsigned wrap_at = 0) {
    const auto last = draw_at + 0x40;
    const auto start = wrap_at ? last - 2 * (wrap_at - 1) : draw_at;
    std::vector<Write> w{{4, s.toc(-0x7b84), start}, {4, s.toc(-0x7b88), last}, {4, s.toc(-0x7b90), base}};
    for (std::size_t k = 0; k < values.size(); ++k)
        if (!wrap_at || k + 1 < wrap_at) w.push_back({2, start + 2 * std::uint32_t(k + 1), values[k]});
    return w;
}

// The conduct bytes (+0x66, +0x67, +0x63 of the conduct record), reputation, and the two tables the ruling reads by the
// human manager's own id.
struct Chain {
    int reputation = 0x2000, first = 20, second = 1, limit = 20, standing = -5, club = 0;
};
std::vector<Write> chain_writes(const Scenario& s, const Person& p, const Chain& k) {
    const auto standing = s.global(-0x7984), clubs = s.r32(s.global(-0x79f0));
    return {{2, p.human + 0x1ec, std::uint32_t(k.reputation)}, {1, p.conduct + 0x66, std::uint32_t(k.first)},
            {1, p.conduct + 0x67, std::uint32_t(k.second)}, {1, p.conduct + 0x63, std::uint32_t(k.limit)},
            {1, standing + p.human_id * 8 + 4, std::uint32_t(k.standing)}, {2, clubs + p.human_id * 0x4c + 6, std::uint32_t(k.club)}};
}

DISCIPLINE_CASES("review") {
    if (scenario.match) return;
    const auto m = scenario.global(-0x7aa4);
    const auto bodies = scenario.r32(m), rows = scenario.r32(m + 0x14), records = scenario.r32(m + 4);
    const auto ban_count = std::int16_t(scenario.r32(m + 8));
    const auto row_of = [&](std::uint32_t person) { return rows + person * 0x2c; };
    const auto no_slots = [&](std::uint32_t person) {
        for (unsigned slot = 0; slot < 5; ++slot)
            if ((scenario.r32(row_of(person) + slot * 8) >> 24) != 0xff) return false;
        return true;
    };

    // People: with a human manager that has a conduct record and reputation above the floor and no bans ("chain"), without a
    // human manager, with one of low reputation; and the persons holding a ban in one of the first four slots.
    std::vector<Person> chain, low;
    std::vector<std::uint32_t> without, held_people;
    std::vector<Held> held;
    for (std::uint32_t id = 0; id < scenario.people(); ++id) {
        const auto record = scenario.person(id), human = scenario.r32(record + 0x24);
        if (!human) {
            if (no_slots(id) && without.size() < 2) without.push_back(id);
            continue;
        }
        const auto conduct = scenario.r32(human + 0x90);
        const auto reputation = std::int16_t(scenario.r32(human + 0x1ec) >> 16);
        if (!conduct || !no_slots(id)) continue;
        const Person p{id, record, human, conduct, scenario.r32(human), reputation};
        if (reputation > 0x1482 && chain.size() < 3 && (chain.empty() || chain.back().human_id != p.human_id)) chain.push_back(p);
        if (reputation <= 0x1482 && low.size() < 1) low.push_back(p);
    }
    for (unsigned body = 0; body < body_count; ++body) {
        const auto object = scenario.r32(bodies + 4 * body);
        if (!object) continue;
        const auto entries = scenario.r32(object + 0xc);
        const auto slot = scenario.r32(object + 4) >> 24, body_byte = scenario.r32(object + 4) >> 16 & 0xff;
        if (slot >= 4) continue;
        for (std::int32_t at = 0; at < std::int32_t(scenario.r32(object + 0x14)) && held.size() < 6; ++at) {
            const auto entry = entries + std::uint32_t(at) * 0x10;
            const auto person = scenario.r32(entry);
            const auto index = std::int16_t(scenario.r32(entry + 4) & 0xffff);
            if (index < 0 || index >= ban_count || person >= scenario.people()) continue;
            const auto row = row_of(person) + slot * 8;
            if ((scenario.r32(row) >> 24) != body_byte || scenario.r32(row + 4) != std::uint32_t(at)) continue;
            if (std::find(held_people.begin(), held_people.end(), person) == held_people.end()) held_people.push_back(person);
            held.push_back({person, entry});
        }
    }
    (void)records;
    if (chain.size() < 2 || without.empty() || low.empty() || held.empty()) return;

    const auto& a = chain[0];
    const auto& b = chain[1];
    const Answers pass_tests{{news_ruling, 0}, {human_flag, 0}, {person_standing, 1}};
    // The third argument is a pointer or null: 0x100e0700 reads a halfword at +0x2e of it when it is not null.
    std::uint32_t argument = 0;
    const auto add = [&](const std::string& what, std::uint32_t person, unsigned first, unsigned second, unsigned cautioned,
                         std::vector<Write> memory, Answers answers = {{news_ruling, 0}}) {
        append(memory, entry_writes(person, first, second, cautioned));
        if (argument) append(memory, {{2, argument + 0x2e, 0x77}});
        cases.push_back({.entry = review_player, .registers = {{3, m}, {4, entry_at}, {5, argument}}, .memory = std::move(memory),
                         .label = "review: " + what, .answers = std::move(answers)});
    };
    const auto chain_case = [&](const std::string& what, const Person& p, const Chain& k, std::vector<Write> draws, Answers answers,
                                unsigned cautioned = 1) {
        auto memory = chain_writes(scenario, p, k);
        append(memory, draws);
        add(what, p.id, 12, 0, cautioned, std::move(memory), std::move(answers));
    };

    // ---- Not a ruling case, and which flag byte makes it one.
    add("neither flag byte is 12", a.id, 0, 0, 1, {});
    add("neither flag byte is 12 (13 and 11)", held[0].person, 13, 11, 0, {});
    add("first flag byte 12", a.id, 12, 0, 1, {});
    add("second flag byte 12", a.id, 0, 12, 1, {});
    add("both flag bytes 12", a.id, 12, 12, 0, {});
    argument = argument_at;
    add("argument pointer", a.id, 12, 0, 1, {});
    argument = 0;

    // ---- The bans the person holds are cleared: real holders (slots 0..3), a slot without an entry, an entry
    // without a ban, a person with no slots.
    for (std::size_t k = 0; k < held.size(); ++k)
        add("held ban " + std::to_string(k), held[k].person, 12, 0, 1, {});
    for (const auto& person : held_people) add("held person " + std::to_string(person), person, 0, 12, 0, {});
    add("held ban, entry index -1 (not cleared)", held[0].person, 12, 0, 1, {{2, held[0].entry + 6, 0xffff}});
    add("slot 0 names a body without an entry of the person", a.id, 12, 0, 1,
        {{1, row_of(a.id), 7}, {4, row_of(a.id) + 4, 0}});
    add("slot 3 names a body without an entry of the person", a.id, 12, 0, 1, {{1, row_of(a.id) + 24, 9}, {4, row_of(a.id) + 28, 0}});
    add("no slots, no human manager", without[0], 12, 0, 1, {});
    add("no slots, no human manager, second person", without.back(), 0, 12, 0, {});
    add("held ban, the ban's person without a human manager", held[0].person, 12, 0, 1, {{4, scenario.person(held[0].person) + 0x24, 0}});

    // ---- The ban built in the frame: 90 or 180 days by the date gap, the cautioned byte, today's date in the ban.
    add("gap below 0x50 (180 days)", a.id, 12, 0, 1, {}, {{news_ruling, 0}, {date_gap, 0x4f}});
    add("gap 0x50 (90 days)", a.id, 12, 0, 1, {}, {{news_ruling, 0}, {date_gap, 0x50}});
    add("gap negative", a.id, 12, 0, 0, {}, {{news_ruling, 0}, {date_gap, 0xffffffff}});
    add("gap real, cautioned 0", a.id, 12, 0, 0, {});
    add("gap real, cautioned 1, real news", a.id, 12, 0, 1, {}, {});
    add("gap real, cautioned 255", held[0].person, 12, 0, 255, {});
    add("today at the end of a month", a.id, 12, 0, 1, {{2, scenario.toc(-0x7b80), 0x7fff}, {2, scenario.toc(-0x7b80) + 2, 0xffff}, {1, scenario.toc(-0x7b80) + 4, 0xff}});

    // ---- The human manager's tests, one exit each, then the passing combination.
    const Chain open;  // reputation above the floor, the standing -5, the club field 0
    const auto exits = [&](const std::string& what, const Chain& k, Answers answers = {{news_ruling, 0}, {human_flag, 0}, {person_standing, 1}}) {
        chain_case("exit: " + what, a, k, draw_writes(scenario, 0, {20}), std::move(answers));
    };
    add("no human manager, ban part only", without[0], 12, 0, 1, draw_writes(scenario, 0, {20}));
    chain_case("low reputation (the person's own)", low[0], open, draw_writes(scenario, 0, {20}), pass_tests);
    {
        Chain k = open;
        k.reputation = 0x1482;
        exits("reputation = floor", k);
        k.reputation = -1;
        exits("reputation negative", k);
        k.reputation = 0x1483;
        exits("reputation floor + 1 (passes the test)", k);
        k = open;
        auto memory = chain_writes(scenario, a, k);
        memory.push_back({4, a.human + 0x90, 0});
        append(memory, draw_writes(scenario, 0, {20}));
        add("exit: no conduct record", a.id, 12, 0, 1, memory, pass_tests);
        exits("human flag set (r3 = 1)", open, {{news_ruling, 0}, {human_flag, 1}, {person_standing, 1}});
        exits("human flag set (r3 = 0x80)", open, {{news_ruling, 0}, {human_flag, 0x80}, {person_standing, 1}});
        exits("human flag clear in the low byte only (r3 = 0x100)", open, {{news_ruling, 0}, {human_flag, 0x100}, {person_standing, 1}});
        exits("person standing 0", open, {{news_ruling, 0}, {human_flag, 0}, {person_standing, 0}});
        exits("person standing 0x100 (low byte 0)", open, {{news_ruling, 0}, {human_flag, 0}, {person_standing, 0x100}});
        exits("person standing 0xff", open, {{news_ruling, 0}, {human_flag, 0}, {person_standing, 0xff}});
        for (int standing : {-3, -2, 0, 127, -4, -128}) {
            k = open;
            k.standing = standing;
            exits("standing " + std::to_string(standing), k);
        }
        for (int club : {0x109a, 0x109b, 0x1099, 0x7fff, 0x8000, 0xffff}) {
            k = open;
            k.club = club;
            exits("club field " + std::to_string(std::int16_t(club)), k);
        }
    }
    chain_case("real tests, real draws (person a)", a, open, {}, {{news_ruling, 0}});
    chain_case("real tests, real draws (person b)", b, open, {}, {});

    // ---- The draws. Sums are entry + base; a sum that is a multiple of 20 writes up the person.
    // First draw: multiple of 20, and not.
    for (int standing : {-4, -100})
        chain_case("draw 1 multiple of 20 (standing " + std::to_string(standing) + ")", a, Chain{.standing = standing},
                   draw_writes(scenario, 0, {40}), pass_tests);
    chain_case("draw 1 multiple of 20 through the base", a, open, draw_writes(scenario, 13, {7}), pass_tests);
    chain_case("draw 1 multiple of 20, 180-day gap", a, open, draw_writes(scenario, 0, {0}), {{news_ruling, 0}, {human_flag, 0}, {person_standing, 1}, {date_gap, 0x10}});
    chain_case("draw 1 entry 0, base 0 (0 is a multiple)", b, open, draw_writes(scenario, 0, {0}), pass_tests);
    chain_case("draw 1 multiple of 20, real news", a, open, draw_writes(scenario, 0, {60}), {{human_flag, 0}, {person_standing, 1}});
    chain_case("draw 1 sum 19", a, open, draw_writes(scenario, 0, {19, 1}), pass_tests);
    chain_case("draw 1 sum 21", a, open, draw_writes(scenario, 0, {21, 1}), pass_tests);
    chain_case("draw 1 sum 0xffff + base 0xffff (not a multiple)", a, open, draw_writes(scenario, 0xffff, {0xffff, 1}), pass_tests);
    chain_case("draw 1 sum 0x10000 + 0xffff (wraps no more)", a, open, draw_writes(scenario, 0xffff, {1, 1}), pass_tests);
    chain_case("draw 1 negative base word", a, open, draw_writes(scenario, 0xffffffecu, {7, 1}), pass_tests);
    chain_case("draw 1 negative base word, sum -20", a, open, draw_writes(scenario, 0xffffffecu, {0, 1}), pass_tests);
    chain_case("draw 1 negative sum", a, open, draw_writes(scenario, 0x80000000u, {5, 1}), pass_tests);
    // Second draw (conduct byte +0x66 plus the standing below 10): the bound is the sum of the two conduct bytes as a signed byte.
    struct Second { std::string what; Chain k; std::vector<std::uint16_t> values; };
    const std::vector<Second> seconds{
        {"bound 5, draw 7 (rem 2)", {.first = 6, .second = 3, .limit = 20, .standing = -5}, {7, 7, 1}},
        {"bound 5, draw 10 (rem 0: written up)", {.first = 6, .second = 3, .limit = 20, .standing = -5}, {7, 10, 1}},
        {"bound -3, draw 7 (rem -1)", {.first = 0xfd, .second = 0, .limit = 20, .standing = -5}, {7, 7, 1}},
        {"bound -3, draw 6 (rem 0: written up)", {.first = 0xfd, .second = 0, .limit = 20, .standing = -5}, {7, 6, 1}},
        {"bound 0 (no draw: written up)", {.first = 12, .second = 0xf4, .limit = 20, .standing = -5}, {7, 7, 1}},
        {"bound 0 from two bytes of 0", {.first = 0, .second = 0, .limit = 20, .standing = -5}, {7, 7, 1}},
        {"bound wraps to 11", {.first = 100, .second = 0xff, .limit = 20, .standing = -120}, {7, 22, 1}},
        {"bound wraps to -128", {.first = 0x80, .second = 0, .limit = 20, .standing = -120}, {7, 129, 1}},
        {"bound 127", {.first = 0, .second = 127, .limit = 20, .standing = -4}, {7, 0x1ffff & 0xfffe, 1}},
        {"bound 1 (always 0: written up)", {.first = 1, .second = 0, .limit = 20, .standing = -4}, {7, 12345, 1}},
        {"bound -1 (always 0: written up)", {.first = 0xff, .second = 0, .limit = 20, .standing = -4}, {7, 12345, 1}},
        {"bound 13, sum 26 (rem 0)", {.first = 13, .second = 0, .limit = 20, .standing = -4}, {7, 26, 1}},
        {"sum below 10 by one (14 + -5 = 9), bound 14", {.first = 14, .second = 0, .limit = 20, .standing = -5}, {7, 3, 1}},
    };
    for (const auto& s : seconds) chain_case("draw 2: " + s.what, a, s.k, draw_writes(scenario, 0, s.values), pass_tests);
    // Skipped second draw: the sum reaches 10.
    for (auto [first, standing] : {std::pair{15, -5}, {14, -4}, {127, -4}, {50, -40}})
        chain_case("sum " + std::to_string(first + standing) + ": no second draw", a, Chain{.first = first, .standing = standing, .limit = 20},
                   draw_writes(scenario, 0, {7, 7, 1}), pass_tests);
    // Third draw: tests of the two conduct bytes (read again: signed bytes above 10) and the draw.
    chain_case("draw 3 multiple of 20 (no second draw)", a, Chain{.first = 20, .standing = -5, .limit = 20}, draw_writes(scenario, 0, {7, 40}), pass_tests);
    chain_case("draw 3 not a multiple (no second draw)", a, Chain{.first = 20, .standing = -5, .limit = 20}, draw_writes(scenario, 0, {7, 41}), pass_tests);
    chain_case("draw 3 after a second draw, multiple of 20", a, Chain{.first = 12, .second = 0, .limit = 20, .standing = -5},
               draw_writes(scenario, 0, {7, 7, 60}), pass_tests);
    chain_case("draw 3 after a second draw, not a multiple", a, Chain{.first = 12, .second = 0, .limit = 20, .standing = -5},
               draw_writes(scenario, 0, {7, 7, 61}), pass_tests);
    chain_case("third draw: first conduct byte 10 (no third draw)", a, Chain{.first = 10, .second = 0, .limit = 20, .standing = -5},
               draw_writes(scenario, 0, {7, 7, 40}), pass_tests);
    chain_case("third draw: first conduct byte 11", a, Chain{.first = 11, .second = 0, .limit = 20, .standing = -5},
               draw_writes(scenario, 0, {7, 7, 40}), pass_tests);
    chain_case("third draw: first conduct byte negative", a, Chain{.first = 0xf0, .second = 0, .limit = 20, .standing = -5},
               draw_writes(scenario, 0, {7, 7, 40}), pass_tests);
    chain_case("third draw: limit byte 10 (no third draw)", a, Chain{.first = 20, .second = 0, .limit = 10, .standing = -5},
               draw_writes(scenario, 0, {7, 40}), pass_tests);
    chain_case("third draw: limit byte 11", a, Chain{.first = 20, .second = 0, .limit = 11, .standing = -5},
               draw_writes(scenario, 0, {7, 40}), pass_tests);
    chain_case("third draw: limit byte negative", a, Chain{.first = 20, .second = 0, .limit = 0x90, .standing = -5},
               draw_writes(scenario, 0, {7, 40}), pass_tests);
    // The cursor at the last entry: each draw in turn wraps to the real table and redraws the base.
    for (unsigned wrap = 1; wrap <= 3; ++wrap) {
        chain_case("wrap at draw " + std::to_string(wrap) + " (second draw taken)", a, Chain{.first = 12, .second = 0, .limit = 20, .standing = -5},
                   draw_writes(scenario, 0, {7, 7}, wrap), pass_tests);
        if (wrap < 3)
            chain_case("wrap at draw " + std::to_string(wrap) + " (second draw skipped)", b, Chain{.first = 20, .limit = 20, .standing = -5},
                       draw_writes(scenario, 0, {7, 7}, wrap), pass_tests);
    }
    chain_case("wrap at draw 1, real news", b, open, draw_writes(scenario, 0, {}, 1), {{human_flag, 0}, {person_standing, 1}});
    // The table's last entry lies before the cursor already (unsigned compare).
    chain_case("cursor above the last entry", a, open, {{4, scenario.toc(-0x7b84), 0xfffffff0u}, {4, scenario.toc(-0x7b88), draw_at}, {4, scenario.toc(-0x7b90), 0}}, pass_tests);
    chain_case("cursor equal to last - 2 (no wrap)", a, Chain{.first = 20, .standing = -5, .limit = 20},
               {{4, scenario.toc(-0x7b84), draw_at + 0x3e}, {4, scenario.toc(-0x7b88), draw_at + 0x40}, {4, scenario.toc(-0x7b90), 0},
                {2, draw_at + 0x40, 40}, {2, draw_at + 0x42, 40}}, pass_tests);
    // A ban held and a ruling written up in one call (the person has slots and a human manager).
    {
        const auto person = held[0].person;
        const auto record = scenario.person(person), human = scenario.r32(record + 0x24);
        if (human && scenario.r32(human + 0x90)) {
            Person p{person, record, human, scenario.r32(human + 0x90), scenario.r32(human), 0};
            chain_case("held ban and a write-up", p, open, draw_writes(scenario, 0, {40}), pass_tests);
            chain_case("held ban, third draw", p, Chain{.first = 20, .standing = -5, .limit = 20}, draw_writes(scenario, 0, {7, 40}), pass_tests);
        }
    }

    // ---- 0x100e8d10, the range sampler: every arm, with the draw table controlled and real.
    const std::uint32_t int_min = 0x80000000u;
    const auto sample = [&](const std::string& what, std::uint32_t bound, std::vector<Write> memory) {
        cases.push_back({.entry = sample_range, .registers = {{3, bound}}, .memory = std::move(memory), .label = "review: sampler " + what});
    };
    sample("bound 0 (no draw)", 0, draw_writes(scenario, 0, {5, 5}));
    sample("bound 1", 1, draw_writes(scenario, 0, {5, 5}));
    sample("bound -1", 0xffffffffu, draw_writes(scenario, 0, {5, 5}));
    sample("bound 7, draw 100", 7, draw_writes(scenario, 3, {100, 5}));
    sample("bound -7, draw 100", 0xfffffff9u, draw_writes(scenario, 3, {100, 5}));
    sample("bound 7, draw a multiple of 7", 7, draw_writes(scenario, 0, {14, 5}));
    sample("bound -7, draw a multiple of 7", 0xfffffff9u, draw_writes(scenario, 0, {14, 5}));
    sample("bound 65535", 65535, draw_writes(scenario, 0xffff, {0xffff, 5}));
    sample("bound -65535", 0xffff0001u, draw_writes(scenario, 0xffff, {0xffff, 5}));
    sample("bound 65535 and a negative base word", 65535, draw_writes(scenario, 0x80000000u, {7, 5}));
    sample("bound 65535, wrap at the draw", 65535, draw_writes(scenario, 0, {}, 1));
    sample("bound 7, real table", 7, {});
    for (std::uint32_t bound : {65536u, 0xffff0000u, 100000u, 0xfffe7960u, 104867u, 655350u, 12345678u, 0x7fffffffu, int_min, 0x1999999Au,
                                0x19999999u, 0x4ccccccdu, 0xe6666666u, 70000u, 0xfffeffffu})
        for (const auto& values : std::vector<std::vector<std::uint16_t>>{{0, 0}, {1, 3}, {65534, 15}, {65535, 7}, {40000, 12}, {12345, 65534}})
            sample("wide bound " + std::to_string(std::int32_t(bound)) + ", draws " + std::to_string(values[0]) + "/" + std::to_string(values[1]),
                   bound, draw_writes(scenario, values[0] == 65535 ? 5 : 0, values));
    sample("wide bound, reduced 0 (no second draw)", 0x1999999Au, draw_writes(scenario, 0, {777, 5}));
    sample("wide bound with a clamp (positive)", 104867, draw_writes(scenario, 0, {65534, 15}));
    sample("wide bound with a clamp (positive, more)", 105015, draw_writes(scenario, 0, {65534, 15}));
    sample("wide bound, negative base word", 100000, draw_writes(scenario, 0xfffffff0u, {7, 5}));
    for (unsigned wrap : {1u, 2u}) {
        sample("wide bound, wrap at draw " + std::to_string(wrap), 100000, draw_writes(scenario, 0, {70, 70}, wrap));
        sample("wide negative bound, wrap at draw " + std::to_string(wrap), 0xfffe7960u, draw_writes(scenario, 0, {70, 70}, wrap));
        sample("small bound, wrap at draw " + std::to_string(wrap), 1000, draw_writes(scenario, 0, {70, 70}, wrap));
    }
    sample("wide bound, real table", 100000, {});
    sample("wide negative bound, real table", 0xfffe7960u, {});
    sample("wide bound int_max, real table", 0x7fffffff, {});
}
}
