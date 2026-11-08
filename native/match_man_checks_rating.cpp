// MATCH_MAN.CPP checker cases for the rating block (match_man_rating.hpp).
#include "match_man_checks.hpp"
#include "match_man_rating_detail.hpp"

namespace cm::aot::match_man_checks {
namespace {
using namespace match_man;
constexpr std::uint32_t entry = 0x10219e40;

struct Pair { std::uint32_t judge, person, club; };

// The people table: 0x70-byte rows at the word TOC -0x7be0 points to, counted at -0x7be4
// (database_load_detail.hpp). Scenario::person reads another table (0x128-byte rows).
constexpr std::uint32_t person_size = 0x70;
std::uint32_t person_at(const Scenario& scenario, unsigned index) { return scenario.global(-0x7be0) + index * person_size; }

// Every kind and estimate the callers pass (kind 2 or 1, an estimate -1 or a
// value they computed), and the ones that reach the branches: above 2550 the
// estimate passes 50, a raw kind with high bits shows the signed-byte test.
void add(std::vector<Case>& cases, const Pair& pair, const std::string& label,
         std::initializer_list<std::pair<std::uint32_t, std::uint32_t>> kinds,
         std::initializer_list<std::int32_t> knowns) {
    for (const auto& [kind, name] : kinds)
        for (const auto known : knowns)
            cases.push_back({.entry = entry,
                             .registers = {{3, pair.judge}, {4, pair.person}, {5, pair.club}, {6, kind}, {7, std::uint32_t(known)}},
                             .label = label + " kind " + std::to_string(name) + " known " + std::to_string(known)});
}
}

MATCH_MAN_CASES("rating") {
    const std::initializer_list<std::pair<std::uint32_t, std::uint32_t>> kinds{{2, 2}, {1, 1}, {0x102, 0x102}};
    if (scenario.match) {
        // The real call of the match: the side's judge (+0x20), its club (+0x34),
        // each player's person.
        for (unsigned side = 0; side < 2; ++side) {
            const auto record = scenario.side(side);
            const Pair base{scenario.r32(record + 0x20), 0, scenario.r32(record + 0x34)};
            for (unsigned index = 0; index < 20; index += 3) {
                const auto person = scenario.r32(scenario.player(side, index) + 0x6c);
                add(cases, {base.judge, person, base.club}, "match side " + std::to_string(side) + " player " + std::to_string(index),
                    kinds, {-1, 700, 3000});
                add(cases, {0, person, base.club}, "match, no judge, player " + std::to_string(index), kinds, {-1, 700});
            }
        }
        return;
    }
    // The career: players are people with player data (+0x14), judges people with
    // staff data (+0x1c). Pointers come from the records themselves.
    const auto count = scenario.global(-0x7be4);
    std::vector<std::uint32_t> players, judges, others, both;
    for (unsigned index = 0; index < count && (players.size() < 400 || judges.size() < 200); ++index) {
        const auto person = person_at(scenario, index);
        const bool plays = scenario.r32(person + person::player) != 0, coaches = scenario.r32(person + person::staff) != 0;
        if (plays) players.push_back(person);
        if (plays && coaches) both.push_back(person);
        if (coaches) judges.push_back(person);
        else if (others.size() < 8) others.push_back(person);
    }
    if (players.empty() || judges.empty()) return;
    // A club whose +0x90 is a person of the table: the fallback judge.
    std::vector<std::uint32_t> clubs;
    const auto first = person_at(scenario, 0), last = person_at(scenario, count);
    for (unsigned index = 0; index < 400 && clubs.size() < 12; ++index) {
        const auto club = scenario.club(index);
        const auto judge = scenario.r32(club + rating_club::default_judge);
        if (judge >= first && judge < last) clubs.push_back(club);
    }
    // Pairs spread over the tables, so the seeds (id + global + judge id) reach
    // each remainder test.
    for (unsigned n = 0; n < 60; ++n) {
        const auto person = players[(n * 7) % players.size()], judge = judges[(n * 13 + 1) % judges.size()];
        add(cases, {judge, person, 0}, "pair " + std::to_string(n), n % 3 ? std::initializer_list<std::pair<std::uint32_t, std::uint32_t>>{{2, 2}, {1, 1}} : kinds,
            {-1, 700, 2550, 2600, 12000});
    }
    for (unsigned n = 0; n < others.size(); ++n)
        add(cases, {others[n], players[(n * 5) % players.size()], 0}, "judge without staff data " + std::to_string(n), kinds, {-1, 3000});
    for (unsigned n = 0; n < clubs.size(); ++n)
        add(cases, {0, players[(n * 11) % players.size()], clubs[n]}, "club's judge " + std::to_string(n), kinds, {-1, 700});
    // The pool's tuning value (single at +0x24 of the TOC -0x527c pool, 20.0 in the game) is
    // all the roll-in-ten path shows. The clamp to 20 is out of reach with the game's value
    // (0.5 + noise * 20 truncates to 20 at most), so edge values are written for the pairs
    // whose seed is a multiple of ten: 100.0 reaches the clamp, -3.0 a negative skill.
    const auto tuning = scenario.toc(-0x527c) + 0x24;
    unsigned shown = 0;
    for (unsigned n = 0; n < 4000 && shown < 6; ++n) {
        const auto person = players[(n * 3) % players.size()], judge = judges[(n * 17 + 5) % judges.size()];
        const auto seed = std::int32_t(scenario.r32(person) + scenario.global(-0x7450) + scenario.r32(judge));
        if (seed % 10 != 0) continue;
        ++shown;
        for (const auto [value, name] : {std::pair<std::uint32_t, const char*>{0x42c80000, "100.0"}, {0xc0400000, "-3.0"}}) {
            const auto before = cases.size();
            add(cases, {judge, person, 0}, std::string("seed multiple of ten, tuning ") + name, kinds, {-1, 700});
            for (auto at = before; at < cases.size(); ++at) cases[at].memory.push_back({4, tuning, value});
        }
    }
    // The person is the judge: the same id twice in the seed.
    if (!both.empty()) add(cases, {both[0], both[0], 0}, "self-judge", kinds, {-1, 3000});
}
}
