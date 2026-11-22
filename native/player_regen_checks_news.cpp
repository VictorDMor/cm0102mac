// PLAYER_REGEN.CPP, blocks `news_posting` and `news_texts`: cases for the three posting routines (0x102df780, 0x102e0090, 0x102e13c0) and the
// three text builders (0x102dfbc0, 0x102e0510, 0x102e15b0). No capture reaches 0x102df780, 0x102e13c0 or the text builders; the season-day
// capture reaches 0x102e0090 and the pairs run it. Every case starts from real people, clubs and nations of the season-day world (the
// people's name records are real: the name builder 0x100cb590 and the position and job titles run for real) and edits the fields the original reads
// over its thresholds. Every callee runs for real (the news manager's posting routines, the contract manager, the name and title routines, the
// localized format and the output item's appenders against a scratch item) except where a case answers one (the contract manager's holder,
// the readers' count with the list a poke writes into the routine's own frame, the coaches' scores, the club's news filter): then the
// answered callee's arguments are still compared by the call trace. Every case carries a poke on the item setter, which makes the checker
// trace its calls (return address and arguments of each format call, name builder and appender).
// Labels start with "news:" (CM_PLAYER_REGEN_BLOCK=news).
#include "player_regen_checks.hpp"
#include <array>
#include <cstdio>
#include <random>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t youth = 0x102df780, retirement = 0x102e0090, staff_retirement = 0x102e13c0, youth_text = 0x102dfbc0, retirement_text = 0x102e0510,
                        staff_text = 0x102e15b0;
constexpr std::uint32_t language_byte = 0x108bba90u - 0x35ac;
constexpr std::uint32_t set_word = 0x102cb4b0, get_word = 0x102cb570, scores = 0x10219e40, readers = 0x103fc710, contract_holder = 0x10082f20, club_filter = 0x102cdbf0;
constexpr std::uint32_t item_at = scratch_base + 0x30000, out_at = scratch_base + 0x31000;
constexpr std::int32_t salt_slot = -0x7b90, cursor_slot = -0x7b84, end_slot = -0x7b88, start_slot = -0x7b8c, club_count_slot = -0x7b50, nation_count_slot = -0x7b04;

struct Pool {
    std::uint32_t cursor_cell, end_cell, salt_cell, start, end;
    explicit Pool(const Scenario& s)
        : cursor_cell(s.toc(cursor_slot)), end_cell(s.toc(end_slot)), salt_cell(s.toc(salt_slot)), start(s.toc(start_slot)), end(s.r32(end_cell)) {}
    // The cursor `before` draws short of the pool's end (0: the next draw refills), the salt and the samples the next draws read.
    std::vector<Write> at(std::uint32_t before, std::uint32_t salt, const std::vector<std::uint32_t>& samples = {}) const {
        const auto position = end - 2 * before;
        std::vector<Write> memory{w32(cursor_cell, position), w32(salt_cell, salt)};
        std::uint32_t next = position;
        for (const auto sample : samples) {
            next += 2;
            memory.push_back(w16(next > end ? start : next, sample));
        }
        return memory;
    }
};

struct World {
    const Scenario& s;
    std::vector<std::uint32_t> players_club, players_free, staff_club, staff_free, both_club, clubs_full, clubs;
    explicit World(const Scenario& scenario) : s(scenario) {
        for (std::uint32_t n = 0; n + 16 < s.people(); ++n) {
            const auto p = s.person(n);
            if (s.r32(p + 0x28) == 0 || s.r32(p + 0x2c) == 0 || s.r32(p + 0x30) == 0) continue;
            const bool player = s.r32(p + 0x14) != 0, staff = s.r32(p + 0x1c) != 0, club = s.r32(p + 0x24) != 0;
            if (player && staff && club) both_club.push_back(p);
            else if (player) (club ? players_club : players_free).push_back(p);
            else if (staff) (club ? staff_club : staff_free).push_back(p);
        }
        for (std::uint32_t n = 0; n < s.global(club_count_slot); ++n) {
            const auto club = s.club(n);
            clubs.push_back(club);
            if (s.r32(club + 0x94) != 0 && s.r32(club + 0x160) != 0 && s.r32(club + 0x164) != 0) clubs_full.push_back(club);
        }
    }
    static std::uint32_t at(const std::vector<std::uint32_t>& list, std::size_t n) { return list.empty() ? 0 : list[(n * 2654435761u + 7) % list.size()]; }
    std::uint32_t any_person(std::size_t n) const {
        switch (n % 5) {
            case 0: return at(players_club, n);
            case 1: return at(staff_club, n);
            case 2: return at(both_club, n);
            case 3: return at(players_free, n);
            default: return at(staff_free, n);
        }
    }
};

std::string number(std::int64_t n) { return std::to_string(n); }
// The poke that makes the checker trace a case (it writes nothing).
std::vector<Case::Poke> traced(std::uint32_t callee = set_word) { return {{callee, {}}}; }
}

// ---- 0x102df780: the youth promotion message ----
PLAYER_REGEN_CASES("news") {
    const World w(scenario);
    const Pool pool(scenario);
    if (w.players_club.size() < 50 || w.clubs_full.size() < 20) throw std::runtime_error("news: the career has too few players or clubs with a full staff");
    std::mt19937 random(20261121);
    const auto pick = [&](auto&& values) { return values[random() % values.size()]; };
    const auto constants = scenario.toc(-0x50a8);
    std::fprintf(stderr, "news: youth promotion fmadd constant (+0x98) bits %08x%08x\n", scenario.r32(constants + 0x98), scenario.r32(constants + 0x9c));
    // The first cases refill the pool in the first draw (the traced site of the draw), the rest random.
    const std::array<std::int32_t, 12> reputations{0, 49, 50, 99, 100, 149, 150, 5000, 10000, 32767, -50, -32768};
    const std::array<std::int32_t, 9> kinds{0, 1, 14, 15, 16, 20, 127, -1, -128};
    for (std::size_t n = 0; n < 24; ++n) {
        const auto person = World::at(w.players_club, n), club = World::at(w.clubs_full, n * 3);
        auto memory = pool.at(n % 2 == 0 ? 0 : 1, 100 + std::uint32_t(n), {40000, 1234});
        std::vector<Case::NthAnswer> answers;
        if (n >= 8) for (unsigned call = 1; call <= 12; ++call) answers.push_back({scores, call, call % 3 == 0 ? 400u : 20u});
        cases.push_back({.entry = youth, .registers = {{3, 0}, {4, person}, {5, club}}, .memory = memory, .nth_answers = answers, .pokes = traced(),
                         .label = "news: youth promotion of a real pair " + number(std::int64_t(n))});
    }
    for (std::size_t n = 0; n < 700; ++n) {
        const auto person = World::at(w.players_club, n * 5 + 1), club = World::at(w.clubs_full, n * 11 + 2);
        const std::int32_t reputation = n < 100 ? reputations[n % reputations.size()] : pick(reputations);
        const auto kind = pick(kinds);
        std::vector<Write> memory = pool.at(std::uint32_t(random() % 4), random(), {std::uint32_t(random() & 0xffff), std::uint32_t(random() & 0xffff), std::uint32_t(random() & 0xffff),
                                                                                  std::uint32_t(random() & 0xffff), std::uint32_t(random() & 0xffff), std::uint32_t(random() & 0xffff)});
        append(memory, {w16(club + 0x1ec, std::uint32_t(reputation)), w8(club + 0x240, std::uint32_t(kind))});
        // Which of the assistant manager and the five coaches exist.
        const auto mask = random() % 64;
        const std::uint32_t slots[6] = {club + 0x94, club + 0x160, club + 0x164, club + 0x168, club + 0x16c, club + 0x170};
        for (unsigned k = 0; k < 6; ++k) if (!(mask >> k & 1)) memory.push_back(w32(slots[k], 0));
        if (random() % 4 == 0) memory.push_back(w16(person + 0x5c, std::uint32_t(random())));
        // The coaches' scores (the first call of each pair is the "great" test, the second the "good"): around the two bounds.
        const std::int32_t great = (reputation * 0x51eb851fLL >> 32 >> 4) + ((reputation * 0x51eb851fLL >> 32 >> 4) < 0) + 0x4b;
        const std::int32_t good = (reputation * 0x51eb851fLL >> 32 >> 5) + ((reputation * 0x51eb851fLL >> 32 >> 5) < 0) + 0x32;
        const std::array<std::int32_t, 10> results{great + 1, great, great - 1, good + 1, good, good - 1, 0, -1, 0x12345 + great, great - 0x10000};
        std::vector<Case::NthAnswer> answers;
        const bool scripted = n % 5 != 0;
        if (scripted) for (unsigned call = 1; call <= 12; ++call) answers.push_back({scores, call, std::uint32_t(pick(results))});
        cases.push_back({.entry = youth, .registers = {{3, std::uint32_t(random())}, {4, person}, {5, club}}, .memory = memory, .nth_answers = answers,
                         .pokes = traced(), .label = "news: youth promotion, reputation " + number(reputation) + " kind " + number(kind) + " staff mask " + number(mask) +
                                                      (scripted ? " scripted scores" : " real scores"),
                         .clear_fpscr = n % 2 == 0});
    }
    // A null person or club posts nothing.
    for (const auto zero : {0, 1}) cases.push_back({.entry = youth, .registers = {{4, zero ? 0 : World::at(w.players_club, 3)}, {5, zero ? World::at(w.clubs_full, 1) : 0}},
                                                     .label = "news: youth promotion with a null argument " + number(zero)});
}

// ---- 0x102e0090 and 0x102e13c0: the retirement messages ----
// The readers' list is written into the routine's own frame (the list sits at sp+0x38 of the frame, whose size is the original's) when
// 0x103fc710 is answered with a count: the poke writes real club rows' pointers.
namespace {
void readers_case(const Scenario& scenario, const World& w, std::vector<Case>& cases, std::uint32_t entry, std::uint32_t frame_size, std::size_t n,
                  std::vector<Write> memory, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers,
                  const std::string& label, std::uint32_t readers_count) {
    const auto list = scenario.sp() - frame_size + 0x38;
    std::vector<Write> poke;
    for (std::uint32_t k = 0; k < 30; ++k) poke.push_back(w32(list + k * 4, World::at(w.clubs, n * 31 + k * 7)));
    answers.push_back({readers, readers_count});
    cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .answers = std::move(answers),
                     .pokes = {{set_word, {}}, {readers, poke}}, .label = label});
}
}

PLAYER_REGEN_CASES("news") {
    const World w(scenario);
    const Pool pool(scenario);
    std::mt19937 random(20261122);
    const auto pick = [&](auto&& values) { return values[random() % values.size()]; };
    const std::array<std::uint32_t, 9> reasons{8, 1, 2, 0x10, 0x20, 0x7f, 0x80, 0xff, 5};
    const std::array<std::uint32_t, 8> jobs{0, 1, 5, 8, 0xd, 0xf, 0x10, 0xb};
    const std::array<std::uint32_t, 7> player_values{0, 0x1d4c, 0x1d4d, 0x2134, 0x2135, 0x7fff, 0x8000};
    const std::array<std::uint32_t, 6> ability{0, 0x109a, 0x109b, 0x2000, 0x7fff, 0x8001};
    const std::array<std::uint32_t, 6> staff_a{0x222e, 0x222f, 0x2230, 0, 0x7fff, 0x8000};
    const std::array<std::uint32_t, 5> staff_b{0x1c52, 0x1c53, 0x1c54, 0, 0x8000};
    const std::array<std::uint32_t, 5> nation_flags{0, 8, 9, 0x40, 0xff};
    for (const auto entry : {retirement, staff_retirement}) {
        const std::uint32_t frame = entry == retirement ? 0x1c0 : 0x1b0;
        const std::string name = entry == retirement ? "retirement" : "staff retirement";
        for (std::size_t n = 0; n < (entry == retirement ? 1400u : 500u); ++n) {
            const auto person = w.any_person(n);
            if (person == 0) continue;
            std::vector<Write> memory;
            const auto record = scenario.r32(person + 0x14), staff = scenario.r32(person + 0x1c), club = scenario.r32(person + 0x24), nation = scenario.r32(person + 0x0c);
            std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
            if (n >= 60) {
                if (random() % 3 == 0) append(memory, {w8(person + 0x62, pick(jobs))});
                if (random() % 3 == 0) append(memory, {w8(person + 0x60, pick(jobs))});
                if (random() % 4 == 0) append(memory, {w8(person + 0x5e, pick(std::array<std::uint32_t, 3>{0, 1, 7}))});
                if (random() % 5 == 0) append(memory, {w16(person + 0x5c, std::uint32_t(random()))});
                if (random() % 6 == 0 && club == 0) append(memory, {w32(person + 0x24, World::at(w.clubs, n))});
                if (nation) append(memory, {w8(nation + 0x11f, pick(nation_flags))});
                if (record) append(memory, {w16(record + 0xc, pick(player_values)), w16(record + 0xa, pick(ability))});
                if (staff) append(memory, {w16(staff + 0x2c, pick(staff_a)), w16(staff + 0x2a, pick(staff_b))});
                const auto home = scenario.r32(person + 0x24);
                if (home && random() % 3 == 0) append(memory, {w32(home + 0x30, random() % 2 ? 0 : home), w32(home + 0x2c, random() % 2 ? nation : 0)});
                if (random() % 3 == 0) answers.push_back({club_filter, 1});
                else if (random() % 7 == 0) answers.push_back({club_filter, 0});
                if (random() % 4 == 0) answers.push_back({contract_holder, random() % 2 ? 0 : scenario.r32(scenario.toc(-0x7b78))});
            }
            const auto reason = n < 40 ? (n % 2 ? 8u : 1u) : pick(reasons);
            const auto flag = pick(std::array<std::uint32_t, 3>{0, 1, 0xff});
            auto registers = std::vector<std::pair<unsigned, std::uint32_t>>{{3, std::uint32_t(random())}, {4, person}, {5, reason}};
            if (entry == retirement) registers.push_back({6, flag});
            const std::string label = "news: " + name + " " + number(std::int64_t(n)) + " reason " + number(reason);
            const auto readers_count = std::array<std::uint32_t, 5>{0, 1, 3, 30, 0x1e1}[random() % 5];
            if (n % 3 == 0) readers_case(scenario, w, cases, entry, frame, n, memory, registers, answers, label + " readers " + number(readers_count), readers_count);
            else cases.push_back({.entry = entry, .registers = registers, .memory = memory, .answers = answers, .pokes = traced(), .label = label});
        }
        // The club of the person is cleared by the first callee (the item's setter): the word for "no club" and what follows with none.
        for (std::size_t n = 0; n < 6 && entry == retirement; ++n) {
            const auto person = World::at(w.staff_club, n + 3);
            cases.push_back({.entry = entry, .registers = {{4, person}, {5, n % 2 ? 8u : 1u}, {6, 1}}, .answers = {{club_filter, 1}},
                             .pokes = {{set_word, {w32(person + 0x24, 0)}}}, .label = "news: retirement, club cleared during the call " + number(std::int64_t(n))});
        }
        // A null person posts nothing; the returns of the filters.
        cases.push_back({.entry = entry, .registers = {{4, 0}, {5, 8}, {6, 1}}, .label = "news: " + name + " of nobody"});
    }
}

// ---- the text builders ----
namespace {
// The item of one case: its words (the person, the three name ids from the person's records, then the message's own) and the edits of the
// records the sentence reads back.
struct TextCase {
    std::uint32_t entry, kind;
    std::uint32_t person;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> words;  // index, value, after the person's five
    std::vector<Write> memory;
    std::int32_t language = -1;
    std::string label;
};
void build_text_case(const Scenario& scenario, std::vector<Case>& cases, const TextCase& t) {
    std::vector<Write> memory = t.memory;
    const auto people_base = scenario.global(-0x7be0);
    const auto index = (t.person - people_base) / 0x70;
    const auto name = [&](std::uint32_t at) { return scenario.r32(scenario.r32(t.person + at) + 0x20); };
    memory.push_back(w32(item_at + 8, index));
    memory.push_back(w32(item_at + 12, name(0x28)));
    memory.push_back(w32(item_at + 16, name(0x2c)));
    memory.push_back(w32(item_at + 20, name(0x30)));
    for (const auto& [at, value] : t.words) memory.push_back(w32(item_at + 8 + at * 4, value));
    if (t.language >= 0) memory.push_back(w8(language_byte, std::uint32_t(t.language)));
    cases.push_back({.entry = t.entry, .registers = {{3, 0x1234}, {4, item_at}, {5, t.kind}, {6, out_at}}, .memory = memory, .pokes = traced(get_word), .label = t.label});
}
}

PLAYER_REGEN_CASES("news") {
    const World w(scenario);
    std::mt19937 random(20261123);
    const auto pick = [&](auto&& values) { return values[random() % values.size()]; };
    const auto clubs = scenario.global(club_count_slot), nations = scenario.global(nation_count_slot);
    const std::array<std::int32_t, 8> languages{-1, -1, 0, 1, 2, 3, 4, 5};
    const std::array<std::uint32_t, 5> kinds{1, 0x7f, 0xff, 0x101, 0x80};
    const std::array<std::uint32_t, 3> headline_kinds{0, 0x100, 0x2300};
    const std::array<std::uint32_t, 18> jobs{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11};
    const std::array<std::uint32_t, 10> person_jobs{0, 0x10, 0xb, 0xc, 0xd, 0xe, 0xf, 5, 0x11, 0x80};
    const std::array<std::uint32_t, 5> ages{0, 0x1a, 0x1b, 0x1c, 0x7f};
    const auto person_edits = [&](std::uint32_t person) {
        std::vector<Write> memory;
        if (random() % 3) memory.push_back(w8(person + 0x62, pick(person_jobs)));
        if (random() % 3) memory.push_back(w8(person + 0x60, pick(person_jobs)));
        if (random() % 2) memory.push_back(w16(person + 0x5c, pick(ages) | (random() & 1) << 8));
        if (random() % 3 == 0) memory.push_back(w32(person + 0x24, random() % 2 ? 0 : World::at(w.clubs, random())));
        if (random() % 3 == 0) memory.push_back(w32(person + 0x20, random() % 2 ? 0 : World::at(w.clubs, random())));
        return memory;
    };
    const auto club_index = [&] { const auto r = random() % 6; return r == 0 ? 0xffffffffu : r == 1 ? 1u : std::uint32_t(random() % clubs); };
    const auto nation_index = [&] { return random() % 5 == 0 ? 0xffffffffu : std::uint32_t(random() % nations); };
    // 0x102dfbc0: the youth promotion text: flags, discoverer present or not, grades.
    for (std::size_t n = 0; n < 900; ++n) {
        const auto person = World::at(w.players_club, n * 3);
        const bool body = n % 4 != 0;
        TextCase t{youth_text, body ? pick(kinds) : pick(headline_kinds), person, {}, {}, pick(languages), "news: youth text " + number(std::int64_t(n))};
        const auto discoverer = World::at(w.staff_club, n * 7 + 1);
        const std::int32_t d_index = random() % 5 == 0 ? -1 : std::int32_t((discoverer - scenario.global(-0x7be0)) / 0x70);
        t.words = {{4, club_index()}, {5, 99}, {6, std::uint32_t(random() % 3)}, {7, std::uint32_t(random() % 3)}, {8, std::uint32_t(d_index)},
                   {9, scenario.r32(scenario.r32(discoverer + 0x28) + 0x20)}, {10, scenario.r32(scenario.r32(discoverer + 0x2c) + 0x20)},
                   {11, scenario.r32(scenario.r32(discoverer + 0x30) + 0x20)}, {12, std::uint32_t(random() % 4)}, {13, std::uint32_t(random() % 4)}};
        t.memory = person_edits(person);
        t.label += body ? " body" : " headline";
        build_text_case(scenario, cases, t);
    }
    // 0x102e0510: the retirement text. Word 4 the reason (8 injury, 0x10, 0x20), 5 a flag, 6 and 7 the club and nation (-1: none), 8 the job,
    // 9 a byte, 0xb, 0xd and 0xe flags, 0xc the age.
    const std::array<std::uint32_t, 8> reasons{8, 8, 0x10, 0x20, 0x20, 1, 5, 0x7f};
    for (std::size_t n = 0; n < 4200; ++n) {
        const auto person = w.any_person(n);
        const bool body = n % 6 != 0;
        TextCase t{retirement_text, body ? pick(kinds) : pick(headline_kinds), person, {}, {}, pick(languages), "news: retirement text " + number(std::int64_t(n))};
        const auto reason = n < 12 ? 8u : pick(reasons);
        t.words = {{4, reason}, {5, std::uint32_t(random() % 3 == 0 ? 0xff : random() % 2)}, {6, club_index()}, {7, nation_index()}, {8, pick(jobs)},
                   {9, std::uint32_t(random() % 2 ? 0 : random() % 3 + 0x100)}, {10, std::uint32_t(random())}, {11, std::uint32_t(random() % 3 == 0)},
                   {12, pick(ages)}, {13, std::uint32_t(random() % 3 == 0)}, {14, std::uint32_t(random() % 3 == 0)}};
        t.memory = person_edits(person);
        // Draws: the parity of the pooled draw, with the cursor at the end of the pool now and then (the refill).
        const Pool pool(scenario);
        append(t.memory, pool.at(std::uint32_t(random() % 3), random(), {std::uint32_t(random() & 0xffff), std::uint32_t(random() & 0xffff)}));
        build_text_case(scenario, cases, t);
    }
    // The branch the random draws rarely reach: no flag, a job to name, a club and a nation (or none of one), every job of the person.
    {
        std::size_t n = 0;
        for (const std::uint32_t reason : {0x20u, 5u})
            for (const std::uint32_t job : {5u, 7u, 0xcu, 0xeu, 0xfu, 0x11u})
                for (const auto club : {0xffffffffu, 3u})
                    for (const auto nation : {0xffffffffu, 2u})
                        for (const std::uint32_t person_job : {0xcu, 0xdu, 0xeu, 0xfu, 5u, 0u}) {
                            const auto person = w.any_person(++n);
                            TextCase t{retirement_text, 1, person, {}, {}, -1, "news: retirement text, no flag, job " + number(job) + " person job " + number(person_job) + " reason " + number(reason)};
                            t.words = {{4, reason}, {5, 0}, {6, club}, {7, nation}, {8, job}, {9, 1}, {10, 0}, {11, 0}, {12, 0x1c}, {13, 1}, {14, 0}};
                            t.memory = {w8(person + 0x62, person_job)};
                            build_text_case(scenario, cases, t);
                        }
    }
    // 0x102e15b0: the staff-side text. Word 4 the reason (0x10, 1, 0x20, 2), 5 the club (-1: none), 6 an age bound.
    const auto humans = scenario.toc(-0x7bb0);
    const auto human_word = scenario.r32(humans + std::uint32_t(std::int32_t(std::int16_t(scenario.u16(humans + 0x2c58))) * 0x2c0) + 4);
    const std::array<std::uint32_t, 7> staff_reasons{0x10, 1, 0x20, 2, 5, 0x10, 1};
    for (std::size_t n = 0; n < 3000; ++n) {
        const auto person = w.any_person(n + 1);
        const bool body = n % 5 != 0;
        TextCase t{staff_text, body ? pick(kinds) : pick(headline_kinds), person, {}, {}, pick(languages), "news: staff text " + number(std::int64_t(n))};
        const auto club = random() % 5 == 0 ? 0xffffffffu : std::uint32_t(random() % clubs);
        t.words = {{4, pick(staff_reasons)}, {5, club}, {6, std::uint32_t(0x15 + random() % 20)}};
        t.memory = person_edits(person);
        // The club is the human manager's, a manager of another club, or none.
        if (club != 0xffffffffu) append(t.memory, {w32(scenario.club(club) + 0x90, random() % 2 ? human_word : random() % 2 ? 0 : human_word + 0x70)});
        build_text_case(scenario, cases, t);
    }
}
}
