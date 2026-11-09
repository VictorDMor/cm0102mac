// MANAGER_MANAGER.CPP checker cases for the board reaction block (manager_manager_board.hpp):
// 0x101a05c0, the two sets of flags the board draws from a job record, and 0x101a2eb0, the reaction
// that moves the clubs' rows by them.
//
// The job record (react's r4, the caller's r23 at 0x103efb54) is built in the scratch area with a real
// club index at +0x8, a real person index at +0xc and the byte at +0x24; the callers' records
// are of that shape and nothing else of them is read. Persons and clubs are the career's real records:
// real calls (no answers) run the callees as they are; the randomized cases answer the callees of
// other modules (club state, valuation, the four pair queries, the club flag, the employer
// record, the notification) with chosen values, so every ladder rung, the thresholds' edges, the
// reputation's limit and the row clamps are reached. A case that ends judge at once (+0x24 is 1)
// leaves react reading the stack as it holds it, so those cases write the flag halfwords to the
// stack where react's frame will have them.
#include "manager_manager_checks.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t judge = 0x101a05c0, react = 0x101a2eb0;
constexpr std::int32_t object_slot = -0x79f0, club_total = -0x7b50, people_total = -0x7be4, pool_slot = -0x536c;
constexpr std::uint32_t record_area = scratch_base + 0xc000, flags_area = scratch_base + 0xc100,
                        employer_area = scratch_base + 0xc200;
constexpr std::uint32_t club_state = 0x1014c2e0, valuation = 0x10084be0, pair_a = 0x100d9480,
                        pair_b = 0x100d94d0, assistant_a = 0x100d9520, assistant_b = 0x100d9580,
                        club_flag = 0x100c7d00, employer_record = 0x10103510, notify = 0x100d9e10;

struct Rng {
    std::uint64_t state;
    std::uint32_t next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return std::uint32_t(state >> 33);
    }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    bool chance(unsigned percent) { return below(100) < percent; }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + below(unsigned(values.size()))); }
};

std::string number(std::int64_t value) { return std::to_string(value); }

// The world: where its clubs and people are, and which people have a club to employ them.
struct World {
    const Scenario& sc;
    unsigned clubs, people;
    std::uint32_t object;
    double scale;
    std::vector<unsigned> employed, free_agents;
    explicit World(const Scenario& sc)
        : sc(sc), clubs(sc.global(club_total)), people(sc.global(people_total)), object(sc.global(object_slot)) {
        const auto bits = (std::uint64_t(sc.r32(sc.toc(pool_slot) + 0x60)) << 32) | sc.r32(sc.toc(pool_slot) + 0x64);
        scale = std::bit_cast<double>(bits);
        for (unsigned p = 0; p < people; ++p) {
            const auto record = sc.person(p);
            if (!sc.r32(record + 0x14)) continue;
            const auto employer = sc.r32(record + 0x24);
            const bool club = employer >= sc.club(0) && employer < sc.club(clubs) && (employer - sc.club(0)) % 0x248 == 0;
            (club ? employed : free_agents).push_back(p);
        }
    }
    bool usable() const { return clubs && people && !employed.empty(); }
    // What fctiwz leaves of the reputation's square: the limit of a flag's weak side.
    std::int32_t limit(std::int32_t reputation) const {
        const double scaled = scale * double(std::int16_t(reputation));
        const double square = scaled * scaled;
        return square >= 2147483648.0 ? 0x7fffffff : std::int32_t(square);
    }
};

// A case under construction: the writes and the answers.
struct Builder {
    Case item;
    void put(unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); }
    void put64(std::uint32_t at, std::uint64_t value) { put(4, at, std::uint32_t(value >> 32)); put(4, at + 4, std::uint32_t(value)); }
    void answer(std::uint32_t entry, std::uint32_t value) { item.answers.push_back({entry, value}); }
};

constexpr std::uint64_t bits_of(double value) { return std::bit_cast<std::uint64_t>(value); }

// The ratio thresholds the ladders compare with (pool doubles) and the empty-valuation ratio.
constexpr double thresholds[] = {0.2, 0.5, 2.0, 6.0, 0.125, 8.0, 0.25, 4.0};

struct Shape {
    unsigned answered = 100;     // percent of cases that answer the callees
    unsigned closed = 6;         // percent that end the judging at once
    unsigned free_agent = 8;     // percent whose person has no employer
    unsigned small_reputation = 60;
    unsigned edge = 70;          // percent that put the standing on an edge
    bool react_rules = false;    // react: row figures at the clamps and the employer's answers
};

// One randomized case over real records.
Case random_case(Rng& rng, const World& w, std::uint32_t entry, const Shape& shape, const std::string& label) {
    const auto& sc = w.sc;
    Builder b{.item = {.entry = entry}};
    // The club, the person and the employer.
    const unsigned club_index = rng.below(w.clubs);
    const bool free_agent = rng.chance(shape.free_agent) && !w.free_agents.empty();
    const unsigned person_index = free_agent ? w.free_agents[rng.below(unsigned(w.free_agents.size()))]
                                             : w.employed[rng.below(unsigned(w.employed.size()))];
    const auto club = sc.club(club_index), person = sc.person(person_index);
    const auto closed_byte = rng.chance(shape.closed) ? rng.pick<std::uint32_t>({1, 1, 1, 2, 0xff, 0x101}) : 0u;
    b.put(4, record_area + 8, club_index);
    b.put(4, record_area + 0xc, person_index);
    b.put(1, record_area + 0x24, closed_byte);
    b.put(2, flags_area, 0xbeef);
    b.put(2, flags_area + 2, 0xcafe);
    if (rng.chance(5)) b.put(4, person + 0x14, 0);
    // The employer: the person's own, none, or the club itself.
    std::uint32_t employer_club = free_agent ? 0 : sc.r32(person + 0x24);
    if (rng.chance(8)) { employer_club = club; b.put(4, person + 0x24, club); }
    else if (rng.chance(15) && !free_agent) { employer_club = sc.club(rng.below(w.clubs)); b.put(4, person + 0x24, employer_club); }
    else if (rng.chance(4)) { employer_club = 0; b.put(4, person + 0x24, 0); }
    // The reputations, the valuation and the standing.
    const auto reputation = [&] {
        if (rng.chance(shape.small_reputation)) return rng.pick<std::uint32_t>({0, 1, 2, 3, 5, 10, 0xffff, 0xfffe});
        return rng.pick<std::uint32_t>({100, 1000, 5000, 20000, 0x7fff, 0x8000, 0x8001, 0xc000, 4500, 12345});
    };
    const auto rep_club = reputation(), rep_employer = reputation();
    b.put(2, club + 0x1ec, rep_club);
    if (employer_club) b.put(2, employer_club + 0x1ec, rep_employer);
    const auto rep_of_club = employer_club == club ? rep_employer : rep_club;
    if (employer_club == club) b.put(2, club + 0x1ec, rep_employer);
    const std::int32_t value = rng.pick<std::int32_t>({1, 2, 3, 5, 8, 16, 100, 1000, 1024, 65536, 249999, 250000, 250001, 262144,
                                                         1000000, 0x7fffffff, 1 << 30, 0, 0, -1, -5, std::int32_t(0x80000000u),
                                                         int(rng.below(2000000)), int(rng.below(300)) * 1000, 1 << 24, (1 << 24) + 1});
    std::int32_t stand;
    const auto kind = rng.below(100);
    const auto reps = std::array<std::int32_t, 2>{std::int32_t(std::int16_t(rep_of_club)), std::int32_t(std::int16_t(rep_employer))};
    const std::int32_t delta = rng.pick<std::int32_t>({-1, 0, 1, 0, 0, 2, -2});
    if (kind < shape.edge) {
        // On a ratio edge of the valuation, or on a reputation limit.
        if (rng.chance(65)) {
            const double threshold = rng.pick<double>({thresholds[0], thresholds[1], thresholds[2], thresholds[3], thresholds[4],
                                                       thresholds[5], thresholds[6], thresholds[7], 0.5, 0.2, 0.25});
            stand = std::int32_t(std::int64_t(std::llround(threshold * double(value))) + delta);
        } else {
            stand = w.limit(reps[rng.below(2)]) + delta;
        }
    } else {
        stand = rng.pick<std::int32_t>({0, 1, -1, 7, 100, 12345, 0x7fffffff, std::int32_t(0x80000000u), 1000000, int(rng.below(100000)), 2 * value, value / 2});
    }
    // The employer's valuation (the same answer) and the rows' drifts.
    for (const auto record : {club, employer_club}) {
        if (!record) continue;
        const auto id = sc.r32(record);
        const auto row = w.object ? sc.r32(w.object) + id * 0x4c : 0;
        if (!row) continue;
        const std::uint64_t drift = rng.pick<std::uint64_t>({0, bits_of(1.0), bits_of(-5.5), bits_of(1e10), bits_of(1e300), bits_of(-1e300),
                                                              0x7ff0000000000000ULL, 0xfff0000000000000ULL, 0x7ff8000000000000ULL,
                                                              0x7ff0000000000001ULL, bits_of(123456.75), 0x8000000000000000ULL,
                                                              (std::uint64_t(rng.next()) << 32) | rng.next()});
        b.put64(row + 0x38, drift);
    }
    if (shape.react_rules) {
        for (const auto record : {club, employer_club}) {
            if (!record) continue;
            const auto row = sc.r32(w.object) + sc.r32(record) * 0x4c;
            const auto figure = [&] {
                return rng.pick<std::uint32_t>({0, 1, 2, 0x7d, 0x7e, 0x2710, 0x2711, 0x2700, 0x7fff, 0x8000, 0x8001, 0xffff, 0xd8f0,
                                                0xd8ef, 0xd8f1, 0xd8f2, 0xd8ff, 0x7fff - 0x19, 0x100, 1000, 5000, 0x9c, 0xfe0c, rng.below(0x10000)});
            };
            b.put(2, row + 6, figure());
            b.put(2, row + 0x14, figure());
            b.put(2, row + 0x16, figure());
        }
    }
    // The standing (0x10416710) is not answered: it sums the record's own words (+0x20, the positive +0x40
    // and +0x48, and the byte at +0x50 times the person's figure over 100), and runs as it is.
    b.put(4, record_area + 0x20, std::uint32_t(stand));
    if (rng.chance(12)) {
        b.put(4, record_area + 0x40, rng.pick<std::uint32_t>({1, 100, 0xffffffff, 5000, 0x7fffffff}));
        b.put(4, record_area + 0x48, rng.pick<std::uint32_t>({1, 100, 0xffffffff, 5000, 0x7fffffff}));
        b.put(1, record_area + 0x50, rng.pick<std::uint32_t>({0, 1, 5, 0x80, 0xff, 100}));
    }
    const bool answered = rng.chance(shape.answered);
    if (answered) {
        b.answer(club_state, rng.chance(50));
        b.answer(valuation, std::uint32_t(value));
        for (const auto entry : {pair_a, pair_b, assistant_a, assistant_b}) b.answer(entry, rng.chance(30));
    }
    if (shape.react_rules) {
        b.answer(club_flag, rng.chance(25));
        const auto record = employer_area;
        b.put(1, record + 0x165, rng.pick<std::uint32_t>({0, 1, 2, 0xff, 0x101}));
        b.answer(employer_record, rng.chance(35) ? 0 : record);
        if (answered) b.answer(notify, 0);
    }
    if (entry == judge) b.item.registers = {{3, w.object}, {4, record_area}, {5, flags_area}, {6, flags_area + 2}};
    else b.item.registers = {{3, w.object}, {4, record_area}};
    b.item.label = label + " club " + number(club_index) + " person " + number(person_index) + " v " + number(value) + " s " + number(stand) + (answered ? "" : " real");
    return b.item;
}
}

MANAGER_MANAGER_CASES("board judge") {
    if (scenario.match) return;
    const World w(scenario);
    if (!w.usable()) return;
    // Real calls: the persons spread over the table with the clubs they work for, and free ones.
    Rng rng{0xb0a4d};
    const auto spread = std::max<std::size_t>(1, w.employed.size() / 150);
    for (std::size_t n = 0; n < w.employed.size(); n += spread) {
        const auto p = w.employed[n];
        const auto person = scenario.person(p);
        for (const auto club : {scenario.r32(person + 0x24), scenario.club(rng.below(w.clubs))}) {
            const unsigned club_index = (club - scenario.club(0)) / 0x248;
            cases.push_back({.entry = judge, .registers = {{3, w.object}, {4, record_area}, {5, flags_area}, {6, flags_area + 2}},
                             .memory = {{4, record_area + 8, club_index}, {4, record_area + 0xc, p}, {1, record_area + 0x24, 0}},
                             .label = "judge real club " + number(club_index) + " person " + number(p)});
        }
    }
    for (std::size_t n = 0; n < w.free_agents.size(); n += std::max<std::size_t>(1, w.free_agents.size() / 20)) {
        const auto p = w.free_agents[n];
        cases.push_back({.entry = judge, .registers = {{3, w.object}, {4, record_area}, {5, flags_area}, {6, flags_area + 2}},
                         .memory = {{4, record_area + 8, rng.below(w.clubs)}, {4, record_area + 0xc, p}},
                         .label = "judge real free person " + number(p)});
    }
    // Ends at once: the record's byte is 1; and a person without a career.
    for (const auto closed : {1u, 2u, 0xffu})
        cases.push_back({.entry = judge, .registers = {{3, w.object}, {4, record_area}, {5, flags_area}, {6, flags_area + 2}},
                         .memory = {{4, record_area + 8, 0}, {4, record_area + 0xc, w.employed[0]}, {1, record_area + 0x24, closed},
                                    {2, flags_area, 0x1234}, {2, flags_area + 2, 0x5678}},
                         .label = "judge closed " + number(closed)});
    cases.push_back({.entry = judge, .registers = {{3, w.object}, {4, record_area}, {5, flags_area}, {6, flags_area + 2}},
                     .memory = {{4, record_area + 8, 0}, {4, record_area + 0xc, w.employed[0]}, {4, scenario.person(w.employed[0]) + 0x14, 0},
                                {2, flags_area, 0x1234}, {2, flags_area + 2, 0x5678}},
                     .label = "judge person without a career"});
    // Randomized over the real records with the callees answered.
    for (unsigned k = 0; k < 1600; ++k) cases.push_back(random_case(rng, w, judge, {}, "judge random"));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(random_case(rng, w, judge, {.closed = 0, .free_agent = 25, .small_reputation = 90, .edge = 95}, "judge edges"));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(random_case(rng, w, judge, {.closed = 0, .free_agent = 0, .small_reputation = 20, .edge = 80}, "judge limits"));
    // Partly real: the callees run as they are, with the records' fields changed.
    for (unsigned k = 0; k < 150; ++k) cases.push_back(random_case(rng, w, judge, {.answered = 0, .closed = 3}, "judge unanswered"));
}

MANAGER_MANAGER_CASES("board react") {
    if (scenario.match) return;
    const World w(scenario);
    if (!w.usable()) return;
    Rng rng{0x5eac7};
    const auto sp = std::uint32_t(scenario.s.r<4>(4));
    const auto flags_a = sp - 0x18, flags_b = sp - 0x16;  // react's frame (0x50): its halfwords at +0x38 and +0x3a
    // Real calls.
    const auto spread = std::max<std::size_t>(1, w.employed.size() / 120);
    for (std::size_t n = 0; n < w.employed.size(); n += spread) {
        const auto p = w.employed[n];
        const auto person = scenario.person(p);
        for (const auto club : {scenario.r32(person + 0x24), scenario.club(rng.below(w.clubs))}) {
            const unsigned club_index = (club - scenario.club(0)) / 0x248;
            cases.push_back({.entry = react, .registers = {{3, w.object}, {4, record_area}},
                             .memory = {{4, record_area + 8, club_index}, {4, record_area + 0xc, p}, {1, record_area + 0x24, 0}},
                             .label = "react real club " + number(club_index) + " person " + number(p)});
        }
    }
    // Garbage flags: the judging ends at once, so react reads the stack as the case wrote it.
    // Every combination of the first halfword's bits, then of the second's, with the other random.
    const std::uint32_t bits[] = {1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x8000, 0x4000};
    const auto garbage = [&](std::uint32_t a, std::uint32_t b2, const std::string& label, unsigned answered_percent) {
        Shape shape{.answered = answered_percent, .closed = 100, .free_agent = 10, .react_rules = true};
        auto item = random_case(rng, w, react, shape, label);
        item.memory.push_back({2, flags_a, a});
        item.memory.push_back({2, flags_b, b2});
        item.memory.push_back({1, record_area + 0x24, 1});
        item.label += " flags " + number(a) + "/" + number(b2);
        cases.push_back(std::move(item));
    };
    for (std::uint32_t mask = 0; mask < 512; ++mask) garbage(mask, rng.below(0x10000) & (rng.chance(70) ? 0x1ff : 0xffff), "react garbage a", 100);
    for (std::uint32_t mask = 0; mask < 512; ++mask) garbage(rng.below(0x10000) & (rng.chance(70) ? 0x1ff : 0xffff), mask, "react garbage b", 100);
    for (unsigned k = 0; k < 300; ++k) {
        std::uint32_t a = 0, b2 = 0;
        for (const auto bit : bits) { if (rng.chance(30)) a |= bit; if (rng.chance(30)) b2 |= bit; }
        garbage(a, b2, "react garbage mix", 100);
    }
    // Judge runs first, its callees answered: the flags come out of the rules.
    for (unsigned k = 0; k < 1200; ++k)
        cases.push_back(random_case(rng, w, react, {.closed = 4, .react_rules = true}, "react random"));
    for (unsigned k = 0; k < 400; ++k)
        cases.push_back(random_case(rng, w, react, {.closed = 0, .free_agent = 3, .small_reputation = 90, .edge = 90, .react_rules = true}, "react edges"));
    // Partly real: the callees run as they are over real records with the rows moved.
    for (unsigned k = 0; k < 150; ++k) cases.push_back(random_case(rng, w, react, {.answered = 0, .closed = 3, .react_rules = true}, "react unanswered"));
}
}
