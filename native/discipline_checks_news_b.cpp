// DISCIPLINE.CPP news block B cases (discipline_checks.hpp): 0x100e59b0, the text of a ban story (news message
// type 2003). The message record is a scratch array of words (0x102cb570 reads the word at +8+4n, see the field
// layout in discipline_news_b.cpp); the person key, the person's date and the fixture are real rows of the career's
// snapshot (so 0x100cbde0 and 0x1029eb20 answer for real) or the same with a value moved off; the clubs are the
// fixture's. A case varies what the rules read: the ban kind, its length (a date or a number of matches), the count
// byte, the clubs' goals (three comparisons), the context flag, whether the person and the fixture are found, and
// the standing bytes. Text objects and links run for real over a zeroed scratch object.
#include "discipline_checks.hpp"
#include <cstdint>
#include <string>

namespace {
using namespace cm::aot::discipline_checks;
constexpr std::uint32_t render = 0x100e59b0;
constexpr std::uint32_t message_at = scratch_base + 0x9000, object_at = scratch_base + 0xa000;

struct Story {
    std::uint32_t flag = 1;  // r5: 0 headline, else body
    std::int32_t length = 2, days = 40;
    std::int8_t bodies[4] = {3, 5, -1, -1};
    std::int32_t kind = 0, count = 1, disc = 0, context = 0;
    bool person = true, fixture = true;
    std::int32_t home_goals = 2, away_goals = 1;
    std::uint32_t standing_home = 0, standing_away = 0;  // 0: leave the clubs' bytes
};

struct World {
    std::uint32_t manager = 0, person_index = 0, key[3] = {}, person_date = 0, fixture_home = 0, fixture_away = 0, fixture_date = 0, fixture_index = 0;
    bool has_person = false, has_fixture = false;
};

World world_of(const Scenario& sc) {
    World w;
    w.manager = sc.global(-0x7aa4);
    for (unsigned p = 0; p < sc.people() && !w.has_person; ++p) {
        const auto row = sc.person(p);
        if (!sc.r32(row + 0x14) && !sc.r32(row + 0x1c)) continue;
        for (unsigned k = 0; k < 3; ++k) {
            const auto name = sc.r32(row + 0x28 + k * 4);
            w.key[k] = name ? sc.r32(name + 0x20) : 0;
        }
        w.person_index = p;
        w.person_date = sc.r32(row + 0x34);
        w.has_person = true;
    }
    const auto table = sc.global(-0x7b4c);
    const auto clubs = sc.global(-0x7b50);
    if (table) {
        const auto count = sc.r32(table + 0x1c), entries = sc.r32(table);
        for (unsigned k = 0; k < count && !w.has_fixture; ++k) {
            const auto entry = entries + k * 12;
            if (sc.r32(entry) >= clubs || sc.r32(entry + 4) >= clubs || sc.r32(entry) == sc.r32(entry + 4)) continue;
            // Two clubs with a name each (the texts print them) and different standing bytes.
            const auto home = sc.club(sc.r32(entry)), away = sc.club(sc.r32(entry + 4));
            if (!sc.s.memory.read<1>(home + 0x221) || !sc.s.memory.read<1>(away + 0x221) ||
                sc.s.memory.read<1>(home + 0x23b) == sc.s.memory.read<1>(away + 0x23b))
                continue;
            w.fixture_index = k;
            w.fixture_home = sc.r32(entry);
            w.fixture_away = sc.r32(entry + 4);
            w.fixture_date = sc.r32(entry + 8);
            w.has_fixture = true;
        }
    }
    return w;
}

Case make(const Scenario& sc, const World& w, const Story& s, std::string label) {
    Case item{.entry = render, .registers = {{3, w.manager}, {4, message_at}, {5, s.flag}, {6, object_at}, {28, sc.toc(-0x7be8)}},
              .label = "news_b: " + std::move(label)};
    const auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    const auto field = [&](unsigned index, std::uint32_t value) { put(4, message_at + 8 + index * 4, value); };
    field(0, std::uint32_t(s.context));
    field(2, std::uint32_t(s.length));
    field(3, std::uint32_t(s.days));
    for (unsigned k = 0; k < 4; ++k) field(6 + k, std::uint32_t(std::int32_t(s.bodies[k])));
    field(10, std::uint32_t(s.kind));
    field(11, std::uint32_t(s.count));
    field(12, std::uint32_t(s.disc));
    for (unsigned k = 0; k < 3; ++k) field(17 + k, w.key[k] + (s.person ? 0u : 1u));
    field(20, w.person_index);
    field(21, w.person_date >> 16);
    field(22, w.person_date & 0xffff);
    const auto home = w.has_fixture ? w.fixture_home : 5, away = w.has_fixture ? w.fixture_away : 7;
    field(24, w.fixture_date >> 16);
    field(25, (w.fixture_date & 0xffff) + (s.fixture ? 0u : 1u));
    field(30, home);
    field(31, away);
    field(32, std::uint32_t(s.home_goals));
    field(33, std::uint32_t(s.away_goals));
    field(34, w.fixture_index);
    if (s.standing_home) put(1, sc.club(home) + 0x23b, s.standing_home);
    if (s.standing_away) put(1, sc.club(away) + 0x23b, s.standing_away);
    return item;
}
}

DISCIPLINE_CASES("news_b") {
    if (scenario.match) return;
    const auto w = world_of(scenario);
    if (!w.has_person) return;
    const auto add = [&](const Story& s, const std::string& label) { cases.push_back(make(scenario, w, s, label)); };
    const auto name = [](const char* what, std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d) {
        return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c) + "/" + std::to_string(d);
    };
    // The headline: length by date (negative), 0, 1, many; the person found or not.
    for (std::int32_t length : {-128, -3, -1, 0, 1, 2, 127})
        for (bool person : {true, false})
            add({.flag = 0, .length = length, .person = person}, name("headline length", length, person, 0, 0));
    // The headline path's flag is a signed byte: the high bits do not count.
    add({.flag = 0x100, .length = 2}, "headline with only high bits in the flag (a body)");
    add({.flag = 0x80, .length = 2}, "flag byte 0x80 (a body)");
    // The body: every kind (0 to 4 own texts; others none) by length class, count class, goals comparison.
    for (std::int32_t kind : {0, 1, 2, 3, 4, 5, -1, 0x100 + 2})
        for (std::int32_t length : {-5, 0, 1, 2})
            for (std::int32_t count : {1, 0, 2})
                for (const auto goals : {std::pair{2, 1}, std::pair{1, 2}, std::pair{1, 1}})
                    add({.length = length, .kind = kind, .count = count, .home_goals = goals.first, .away_goals = goals.second},
                        name("body kind", kind, length, count, goals.first * 10 + goals.second));
    // Context flag, person and fixture found or not, on a text with clubs and one without.
    for (std::int32_t kind : {0, 2})
        for (std::int32_t context : {0, 1, -1})
            for (bool person : {true, false})
                for (bool fixture : {true, false})
                    add({.length = 2, .kind = kind, .context = context, .person = person, .fixture = fixture},
                        name("body links kind", kind, context, person, fixture));
    // Goals as signed bytes (the compare is signed), the standing bytes of the clubs (signed).
    for (const auto goals : {std::pair{-1, 1}, std::pair{127, -128}, std::pair{-128, -128}, std::pair{200, 3}, std::pair{0, 0}})
        add({.length = 3, .kind = 2, .home_goals = goals.first, .away_goals = goals.second}, name("goals", goals.first, goals.second, 0, 0));
    for (std::uint32_t standing : {0x80u, 0xffu, 0x7fu, 1u})
        add({.length = 3, .kind = 3, .standing_home = standing, .standing_away = 0x81}, name("standing", standing, 0, 0, 0));
    // r28 is the real caller's (the clubs table's TOC cell address), as in the original: not varied.
    for (std::int32_t kind : {0, 1, 2, 3, 4})
        add({.length = -2, .kind = kind}, name("negative length", kind, 0, 0, 0));
    // Bodies named: all four, one, and none (7490's class is then read from the word before the array: left out).
    for (const auto bodies : {std::array<std::int8_t, 4>{3, 5, 8, 9}, {3, -1, -1, -1}, {-1, 5, -1, -1}, {-1, -1, -1, 9}, {3, 5, 8, -1}})
        for (std::int32_t length : {1, 4}) {
            Story s{.length = length, .kind = 0};
            for (unsigned k = 0; k < 4; ++k) s.bodies[k] = bodies[k];
            add(s, name("bodies", bodies[0], bodies[1], bodies[2], length));
        }
    // Another story (field 12 not 0): 0x100e0b70 renders it.
    for (std::int32_t disc : {1, -1, 0x80, 0x100})
        for (std::uint32_t flag : {0u, 1u})
            add({.flag = flag, .disc = disc}, name("other story", disc, flag, 0, 0));
}
