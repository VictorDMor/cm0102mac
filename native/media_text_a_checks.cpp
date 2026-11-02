// The press column's cases: a story item in scratch memory (its values at +8, four bytes each, read
// through 0x102cb570), real clubs and persons from the career, and an output text that starts empty.
#include "media_checks.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

using namespace cm::aot::media_checks;

namespace {
constexpr std::uint32_t item_at = scratch_base + 0x1000, output_at = scratch_base + 0x800, entry = 0x1064cf80;
using Values = std::vector<std::uint32_t>;

struct Known { std::uint32_t index, first, second, third; };
// Person rows that the key matches (0x100cbde0): under contract or at a club, with the ids of the three linked records.
std::vector<Known> known_persons(const Scenario& s, unsigned count) {
    std::vector<Known> found;
    const auto base = s.global(-0x7be0);
    for (std::uint32_t i = 1; found.size() < count && i < 20000; ++i) {
        const auto row = base + i * 0x70;
        const auto link = [&](std::uint32_t at) { return s.r32(row + at) ? s.r32(s.r32(row + at) + 0x20) : 0xffffffffu; };
        if (s.r32(row + 0x14) && s.r32(row + 0x28) && s.r32(row + 0x2c) && s.r32(row + 0x30) && link(0x28) < 40000 && link(0x2c) < 40000)
            found.push_back({i, link(0x28), link(0x2c), link(0x30)});
    }
    return found;
}

// What the world gives the stories: who the user is, the nation his side plays in, and clubs at home and abroad.
struct World {
    std::uint32_t user_row, manager, user_nation;
    std::uint32_t rows;
    std::vector<std::uint32_t> home_clubs, abroad_clubs;
    std::uint32_t manager_row = 0;  // the row index that is the user's manager (0: none found)
};
World world_of(const Scenario& s) {
    World w{};
    const auto base = s.toc(-0x7bb0);
    w.user_row = base + std::uint32_t(std::int32_t(std::int16_t(s.s.memory.read<2>(base + 0x2c58)))) * 0x2c0;
    w.manager = s.r32(w.user_row + 4);
    w.rows = s.global(-0x7be0);
    const auto team = w.manager ? s.r32(w.manager + 0x24) : 0;
    w.user_nation = w.manager ? (team ? s.r32(team + 0x2c) : s.r32(w.manager + 0xc)) : 0;
    if (w.manager >= w.rows && (w.manager - w.rows) % 0x70 == 0 && (w.manager - w.rows) / 0x70 < 40000) w.manager_row = (w.manager - w.rows) / 0x70;
    for (unsigned k = 1; k < 400; ++k) {
        const auto nation = s.r32(s.club(k) + 0x2c);
        if (!nation) continue;
        (nation == w.user_nation ? w.home_clubs : w.abroad_clubs).push_back(k);
    }
    std::fprintf(stderr, "text_a: user manager %#x nation %#x row %u home %zu abroad %zu\n", w.manager, w.user_nation, w.manager_row,
                 w.home_clubs.size(), w.abroad_clubs.size());
    return w;
}

// The long form's item: player (0), his keys (1-3), club (4), tone (5), medium (6), the person the outlet quotes (7), team (8),
// kind (9), variant (10), second person (11) and keys (12-14), answer (16), level (17), traits (18), manager (23) and keys (24-26),
// the other club (27) and whether it is a match story (28), then the match fields or the third person.
struct Column {
    Values v = Values(0x50, 0);
    Column(const Known& player, const Known& manager, std::uint32_t club) {
        v[0] = player.index; v[1] = player.first; v[2] = player.second; v[3] = player.third;
        v[4] = club; v[5] = 0; v[6] = 1; v[7] = 5; v[8] = 3; v[9] = 0; v[10] = 1; v[11] = 0xffffffffu;
        v[0x10] = 4; v[0x11] = 0x20; v[0x12] = 0; v[0x17] = manager.index; v[0x18] = manager.first; v[0x19] = manager.second;
        v[0x1a] = manager.third; v[0x1b] = 7; v[0x1c] = 0; v[0x23] = 0xffffffffu; v[0x24] = v[0x25] = v[0x26] = 0xffffffffu;
    }
    const Values& values() const { return v; }
    Column& raw(std::size_t index, std::uint32_t value) { v[index] = value; return *this; }
    Column& kind(std::uint32_t k) { v[9] = k; return *this; }
    Column& tone(std::uint32_t t) { v[5] = t; return *this; }
    Column& medium(std::uint32_t m) { v[6] = m; return *this; }
    Column& variant(std::uint32_t x) { v[10] = x; return *this; }
    Column& answer(std::uint32_t a) { v[0x10] = a; return *this; }
    Column& level(std::uint32_t l) { v[0x11] = l; return *this; }
    Column& traits(std::uint32_t t) { v[0x12] = t; return *this; }
    Column& second(const Known& k) { v[11] = k.index; v[12] = k.first; v[13] = k.second; v[14] = k.third; return *this; }
    Column& third(const Known& k) { v[0x23] = k.index; v[0x24] = k.first; v[0x25] = k.second; v[0x26] = k.third; return *this; }
    // A match story: its fields follow the position 0x1d; goals at 0x25 and 0x26.
    Column& match(std::uint32_t rival, std::uint32_t home, std::uint32_t away, std::uint32_t result) {
        v[0x1c] = 1; v[0x1d] = 2001; v[0x1e] = 40; v[0x1f] = 3; v[0x20] = 1; v[0x21] = 2; v[0x22] = 3; v[0x23] = 4;
        v[0x24] = rival; v[0x25] = home; v[0x26] = away; v[0x27] = 9; v[0x28] = result; v[0x29] = 11; v[0x2a] = 12; v[0x2b] = 13;
        return *this;
    }
};

Case story(const Values& values, std::uint32_t full, std::string label, std::vector<Write> extra = {}) {
    std::vector<Write> writes;
    for (std::size_t i = 0; i < values.size(); ++i) writes.push_back({4, item_at + 8 + 4 * std::uint32_t(i), values[i]});
    writes.push_back({4, output_at, 0});
    writes.insert(writes.end(), extra.begin(), extra.end());
    return {.entry = entry, .registers = {{3, 0x31313131}, {4, item_at}, {5, full}, {6, output_at}}, .memory = writes, .f1 = {}, .label = std::move(label)};
}
}  // namespace

MEDIA_CASES("text_a") {
    // TA_FROM=a TA_TO=b keep cases a..b-1, for looking at one difference.
    const char* from_env = std::getenv("TA_FROM");
    const char* to_env = std::getenv("TA_TO");
    const std::size_t from = from_env ? std::stoul(from_env) : 0, to = to_env ? std::stoul(to_env) : ~std::size_t(0);
    std::size_t ordinal = 0;
    static std::string last_label; const auto add = [&](Case item) { if (std::getenv("TA_LIST") && item.label != last_label) { std::fprintf(stderr, "case %zu: %s\n", ordinal, item.label.c_str()); last_label = item.label; } if (ordinal >= from && ordinal < to) { item.label += " #" + std::to_string(ordinal); cases.push_back(std::move(item)); } ++ordinal; };
    const auto people = known_persons(scenario, 6);
    const auto w = world_of(scenario);
    const auto& player = people.at(0);
    const auto& manager = people.at(1);
    const auto& other = people.at(2);
    const auto& helper = people.at(3);
    const std::uint32_t home = w.home_clubs.empty() ? 5u : w.home_clubs.at(0);
    const std::uint32_t abroad = w.abroad_clubs.empty() ? 6u : w.abroad_clubs.at(0);
    const std::uint32_t none = 0xffffffffu;
    // The story is told at home when the club's nation is the user's; ours differs by the club chosen.
    const std::uint32_t kinds[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xc, 0xd, 0xe, 0x10, 0x11, 0x12, 0x13, 0x1e, 0x20, 0x21, 0x22, 0x23, 0x25, 0x24};
    const std::uint32_t tones[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, none, 0x80000000u};
    // Every kind and tone, at home and abroad, through each medium.
    for (const auto club : {home, abroad})
        for (const auto kind : kinds)
            for (const auto tone : tones)
                for (const std::uint32_t medium : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 0u})
                    if (medium == 1 || tone < 3 || kind == 0 || kind == 0x12 || kind == 1)
                        add(story(Column(player, manager, club).kind(kind).tone(tone).medium(medium).values(), 1, "column"));
    // The traits that pick the line when the tone is above 7.
    const std::uint32_t masks[] = {0, 0x400200, 0x102200, 0x104200, 0x100200, 0x480, 0x500, 0x20820, 0x1020, 0x20810, 0x10001, 1, 0xa, 8, 2, 0xb,
                                   0x4000, 0x20000, 0x40000, 0x60000, 0x10, 0x20, 0x30, 0x800000, 0x3, 0x7fffffff, 0xffffffff, 0x40001, 0x10002, 0x40010, 0x40020, 0x40030, 0x40002, 0x6};
    for (const auto club : {home, abroad})
        for (const auto kind : {0u, 0x12u})
            for (const auto mask : masks)
                for (const auto tone : {8u, 9u, none})
                    add(story(Column(player, manager, club).kind(kind).tone(tone).traits(mask).values(), 1, "column traits"));
    // A tone above 7 that no trait explains is read again as the player's number modulo 8.
    for (const auto index : {0u, 1u, 5u, 7u, 8u, 9u, 15u, 0x7fffffffu})
        add(story([&] { auto c = Column(player, manager, home).kind(0).tone(8).traits(0); c.v[0] = index; return c.values(); }(), 1, "column reseeded"));

    // The reply: told, admitted, ignored or nothing, for the user's own player or another manager's.
    const auto users_manager = [&](bool users) {
        std::vector<Write> writes;
        // The user's manager is one of our rows, with no club and the nation of the home clubs.
        if (users) {
            const auto row = w.rows + manager.index * 0x70;
            writes.push_back({4, w.user_row + 4, row});
            writes.push_back({4, row + 0x24, 0});
            writes.push_back({4, row + 0xc, scenario.r32(scenario.club(home) + 0x2c)});
        }
        return writes;
    };
    for (const bool users : {false, true}) {
        for (const std::uint32_t variant : {0u, 3u, 4u, 5u, none, 0x80000000u})
            add(story(Column(player, manager, home).variant(variant).values(), 1, "reply variant", users_manager(users)));
        for (const std::uint32_t variant : {1u, 2u})
            for (const auto club : {home, abroad})
                for (const auto kind : kinds)
                    for (const std::uint32_t tone : {0u, 3u, 7u, 8u, 9u})
                        add(story(Column(player, manager, club).variant(variant).kind(kind).tone(tone).values(), 1, "reply kinds", users_manager(users)));
        for (const std::uint32_t variant : {1u, 2u})
            for (const auto kind : {0u, 0x12u})
                for (const std::uint32_t tone : {0u, 3u, 8u, 9u})
                    for (const std::uint32_t answer : {none, 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u})
                        for (const std::uint32_t level : {0x10u, 0x17u, 0x18u, 0x1au, 0x1bu})
                            for (const std::uint32_t traits : {0u, 0x800000u})
                                add(story(Column(player, manager, home).variant(variant).kind(kind).tone(tone).answer(answer).level(level).traits(traits).values(), 1,
                                          "reply answers", users_manager(users)));
        for (const std::uint32_t variant : {1u, 2u})
            for (const auto club : {home, abroad})
                for (const auto kind : {0u, 0x12u})
                    for (const auto mask : masks)
                        for (const std::uint32_t tone : {8u, 9u})
                            add(story(Column(player, manager, club).variant(variant).kind(kind).tone(tone).traits(mask).answer(5).values(), 1, "reply traits", users_manager(users)));
        for (const std::uint32_t variant : {1u, 2u})
            for (const std::uint32_t tone : {0u, 8u})
                for (const std::uint32_t b1 : {0u, 3u, 4u, 0x7fu, 0x80u})
                    for (const std::uint32_t b0 : {0u, 1u})
                        for (const std::uint32_t flags : {0u, 1u, 2u, 3u})
                            add(story(Column(player, manager, home).variant(variant).kind(0x13).tone(tone).raw(0x21, b1).raw(0x22, b0).raw(0x20, flags).values(), 1,
                                      "reply bytes", users_manager(users)));
        for (const std::uint32_t variant : {1u, 2u})
            for (const auto kind : {4u, 0x25u})
                for (const std::uint32_t third : {0u, 5u})
                    add(story(Column(player, manager, home).variant(variant).kind(kind).second({other.index, other.first, other.second, third}).raw(0x20, 5).values(), 1,
                              "reply second", users_manager(users)));
    }

    // The people the text links to: a second and a third person, with keys that match or not.
    for (const bool users : {false, true})
        for (const std::uint32_t variant : {1u, 2u})
            for (const bool with_second : {false, true})
                for (const bool with_third : {false, true}) {
                    auto item = Column(player, manager, home).variant(variant).kind(1);
                    if (with_second) item.second(other);
                    if (with_third) item.third(helper);
                    add(story(item.values(), 1, "links", users_manager(users)));
                    if (with_second) { auto bad = item; bad.v[12] += 1; add(story(bad.values(), 1, "links, second key differs", users_manager(users))); }
                    if (with_third) { auto bad = item; bad.v[0x25] += 1; add(story(bad.values(), 1, "links, third key differs", users_manager(users))); }
                }

    // Match stories: the score either way round or level, the result line, and the player-manager.
    const std::pair<std::uint32_t, std::uint32_t> scores[] = {{2, 1}, {1, 1}, {0, 2}, {3, 3}, {0, 0}, {5, 0x80}};
    for (const auto club : {home, abroad})
        for (const auto kind : {0x11u, 0xau, 4u, 0u})
            for (const std::uint32_t tone : {0u, 1u, 2u, 3u, 8u})
                for (const auto& [goals_home, goals_away] : scores)
                    for (const std::uint32_t result : {0u, 1u, 2u, 3u, 4u, 5u}) {
                        if (result > 1 && (tone != 0 || kind != 0x11) && goals_home != goals_away) continue;
                        add(story(Column(player, manager, club).kind(kind).tone(tone).match(7, goals_home, goals_away, result).values(), 1, "match"));
                    }
    for (const auto kind : {0x11u, 0xau})
        for (const std::uint32_t tone : {0u, 1u})
            add(story(Column(player, player, home).kind(kind).tone(tone).match(7, 2, 1, 1).values(), 1, "match, player-manager"));
    add(story(Column(player, manager, home).kind(0x11).tone(1).match(none, 2, 1, 1).values(), 1, "match, no rival"));
    add(story(Column(player, manager, home).kind(1).match(7, 2, 1, 1).values(), 1, "match, other story"));

    // The short form, a headline over the story.
    const auto headline = [&](std::uint32_t kind, std::uint32_t figure, std::uint32_t club, const Known& who, const Known* second, std::string label) {
        Values v{who.index, who.first, who.second, who.third, club, figure, 6, 6, 0, kind, 0, second ? second->index : none,
                 second ? second->first : 0, second ? second->second : 0, second ? second->third : 0};
        v.resize(0x30, 0);
        add(story(v, 0, std::move(label)));
    };
    for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 0xau, 0xcu, 0xdu, 0xeu, 0x11u, 0x13u, 0x21u, 0x22u, 0x23u, 0x25u, 0x24u, none})
        for (const std::uint32_t figure : {0u, 1u, 2u, 3u, 4u, none})
            for (const Known* second : {static_cast<const Known*>(nullptr), &other})
                headline(kind, figure, home, player, second, "headline");
    for (const auto& who : people)
        for (const std::uint32_t club : {5u, 6u, 7u, 8u, 9u, 12u, 100u, none, 0x80000000u})
            for (const std::uint32_t figure : {0u, 1u, 2u, 5u, none})
                headline(0x11, figure, club, who, nullptr, "headline pick");
    headline(0x11, 0, home, player, nullptr, "headline");
    {
        Values v{player.index, player.first + 1, player.second, player.third, home, 0, 6, 6, 0, 1, 0, other.index, other.first + 1, other.second, other.third};
        v.resize(0x30, 0);
        add(story(v, 0, "headline, keys differ"));
    }
    for (const std::uint32_t full : {0x100u, 0x200u, 0x1ffu, 0xffffff00u})
        add(story(Column(player, manager, home).kind(1).values(), full, "form flag"));

    // Match stories with a tone below 0, and a table of eight tones for kind 1.
    for (const auto kind : {0x11u, 0xau}) add(story(Column(player, manager, home).kind(kind).tone(none).match(7, 2, 1, 1).values(), 1, "match negative tone"));
    for (const std::uint32_t tone : {6u, 7u, 8u, 0x80000000u}) add(story(Column(player, manager, home).kind(1).tone(tone).values(), 1, "kind 1 tones"));
    // The fixture the text links: the calendar finds it.
    for (const auto club : {home, abroad})
        add([&] { auto item = story(Column(player, manager, club).kind(0x11).tone(0).match(7, 2, 1, 1).values(), 1, "match fixture found"); item.answers = {{0x1029eb20, 1}}; return item; }());

    // Who the user is decides whether the story is told at home: no manager, a manager without a club, and the sentinels
    // of the item (no club, no medium, no team, no player, no manager).
    const auto user_slot = w.user_row + 4;
    std::uint32_t clubless = 0;  // a person row whose club slot (+0x24) is empty
    for (std::uint32_t i = 1; i < 20000 && !clubless; ++i) if (!scenario.r32(w.rows + i * 0x70 + 0x24)) clubless = i;
    const auto home_nation = scenario.r32(scenario.club(home) + 0x2c);
    for (const bool users : {false, true}) {
        const auto place = [&](std::uint32_t row_pointer, std::vector<Write> more = {}) { more.push_back({4, user_slot, row_pointer}); return more; };
        add(story(Column(player, manager, home).kind(1).values(), 1, "user without manager", place(0)));
        (void)users;
        if (clubless) {
            const auto row = w.rows + clubless * 0x70;
            add(story(Column(player, manager, home).kind(1).values(), 1, "user manager without club", place(row)));
            add(story(Column(player, manager, home).kind(1).values(), 1, "user manager without club, nation", place(row, {{4, row + 0xc, home_nation}})));
            add(story(Column(player, manager, abroad).kind(1).values(), 1, "user manager without club, other nation", place(row, {{4, row + 0xc, home_nation}})));
            add(story(Column(player, Known{clubless, manager.first, manager.second, manager.third}, home).kind(1).variant(1).values(), 1, "user is the manager", place(row)));
            add(story(Column(player, Known{clubless, manager.first, manager.second, manager.third}, home).kind(1).variant(2).values(), 1, "user is the manager", place(row)));
        }
    }
    for (const auto kind : {1u, 3u, 0xdu})
        for (const auto club : {home, abroad}) {
            auto item = [&] { return Column(player, manager, club).kind(kind).variant(1); };
            add(story(item().raw(4, none).values(), 1, "no club"));
            add(story(item().raw(6, none).values(), 1, "no medium"));
            add(story(item().raw(0x1f, 0xffff).values(), 1, "no team"));
            add(story(item().raw(0, none).values(), 1, "no player"));
            add(story(item().raw(0x17, none).values(), 1, "no manager"));
            add(story(item().raw(0x18, manager.first + 1).values(), 1, "manager key differs"));
            add(story(item().raw(1, player.first + 1).values(), 1, "player key differs"));
        }
    add(story(Column(player, manager, home).kind(1).variant(1).values(), 1, "club without nation", {{4, scenario.club(home) + 0x2c, 0}}));
    add(story(Column(player, manager, abroad).kind(1).variant(1).values(), 1, "club without nation", {{4, scenario.club(abroad) + 0x2c, 0}}));
    add(story(Column(player, player, home).kind(1).variant(1).values(), 1, "player is the manager"));
}
