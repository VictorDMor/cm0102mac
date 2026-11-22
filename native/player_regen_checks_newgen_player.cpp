// PLAYER_REGEN.CPP, block `newgen_player`: cases for 0x102e4530, the creation of a new young player in place of a retired one. No capture reaches
// it, so every case starts from the career's real records (season-day): a real person, a real player record (the slot the regen object's
// pool names), the real nations, edited over every byte the routine reads to branch (the nation's reputation word +0x6c, flag +0x120 and word
// +0x74, the plan's nation indices, the player record's potential and position ratings and the bytes it reads before it writes them, the
// statistics' threshold byte +0x61, the date, the kind-table marker, the person's club), with the pooled generator in every position and with
// many contents. The callees run for real on both sides (the checker does not answer a routine of this module: the club pick and the signing are
// steered through the world), except the ones outside the module that a case answers to force an outcome (0x10186ea0, which opens a file, always).
//
// The pool. The checker compares the generator's whole state after the call (cursor, salt, end, state word, the sample under the cursor), so a
// missing, extra or reordered draw fails a case; a refill is traced by its return address, and only a traced case (the checker traces the first 32
// cases of an entry; a case with a poke on a callee every case calls, `traced`, is traced too) pairs them. Three ways of driving it, as in newgen_staff: `every_draw` (every draw refills: one
// case pairs every refill site of its path), `scripted` (the cursor `before` draws short of the end, the next samples given: with salt 0 a
// draw's remainder is its sample's, so a test draw can be forced) and the snapshot's own position with random samples.
// The default run is a subset that still reaches every draw site, every outcome of every `if` and the controls; CM_NEWGEN_PLAYER_FULL=1 adds
// the exhaustive sweeps (the whole range of the potential and of every byte the routine tests), run once by hand on the final code.
// Labels start with "newgen_player:".
#include "player_regen_checks.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <stdexcept>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t player_entry = 0x102e4530, trace_callee = 0x1014b690, news_check = 0x102cdbf0, forget_history = 0x10186ea0;
constexpr std::int32_t regen_slot = -0x7b28, kind_slot = -0x7b9c, players_slot = -0x7b38, nations_slot = -0x7b98, nation_count_slot = -0x7b04,
                       human_nation_slot = -0x7a18, other_nation_slot = -0x6678, date_slot = -0x7b80, cursor_slot = -0x7b84, end_slot = -0x7b88,
                       salt_slot = -0x7b90, start_slot = -0x7b8c;

struct Ring {
    std::uint32_t cursor_cell, end_cell, salt_cell, start, end;
    explicit Ring(const Scenario& s)
        : cursor_cell(s.toc(cursor_slot)), end_cell(s.toc(end_slot)), salt_cell(s.toc(salt_slot)), start(s.toc(start_slot)), end(s.r32(end_cell)) {}
    std::vector<Write> every_draw(std::uint32_t sample, std::uint32_t salt = 0) const {
        return {w32(end_cell, start), w32(cursor_cell, start), w32(salt_cell, salt), w16(start, sample)};
    }
    // The cursor `before` draws short of the end; draw n (from 0) reads samples[n]: before the wrap at the cursor's next cells, the draw
    // `before` refills and reads the pool's start, and the draws after it the cells after the start.
    std::vector<Write> scripted(std::uint32_t before, std::uint32_t salt, const std::vector<std::uint32_t>& samples) const {
        const auto position = end - 2 * before;
        std::vector<Write> memory{w32(cursor_cell, position), w32(salt_cell, salt)};
        for (std::size_t n = 0; n < samples.size(); ++n)
            memory.push_back(w16(n < before ? position + 2 * std::uint32_t(n + 1) : start + 2 * std::uint32_t(n - before), samples[n]));
        return memory;
    }
};

struct World {
    const Scenario& s;
    std::uint32_t regen, plan, players, nations, kinds;
    std::int32_t nation_count;
    std::vector<std::uint32_t> people, slots, low_nations, mid_nations, high_nations;
    explicit World(const Scenario& scenario)
        : s(scenario), regen(scenario.global(regen_slot)), plan(0), players(scenario.global(players_slot)), nations(scenario.global(nations_slot)),
          kinds(scenario.global(kind_slot)), nation_count(std::int32_t(scenario.global(nation_count_slot))) {
        if (regen == 0) throw std::runtime_error("newgen_player: the career has no regen object");
        plan = s.r32(regen + 0x3c);
        if (plan == 0) throw std::runtime_error("newgen_player: the regen object's player pool has no block");
        for (std::uint32_t n = 0; n + 16 < s.people(); ++n) {
            const auto p = s.person(n);
            const auto record = s.r32(p + 0x14);
            if (record < players || (record - players) % 0x48 != 0 || s.r32(p + 0x1c) != 0) continue;
            people.push_back(p);
            slots.push_back((record - players) / 0x48);
        }
        for (std::int32_t n = 0; n < nation_count && n < 400; ++n) {
            const auto nation = nations + std::uint32_t(n) * 0x128;
            const auto reputation = std::int32_t(s.r32(nation + 0x6c));
            (reputation < 100 ? low_nations : reputation < 350 ? mid_nations : high_nations).push_back(std::uint32_t(n));
        }
        if (people.size() < 200 || low_nations.empty() || mid_nations.empty() || high_nations.empty())
            throw std::runtime_error("newgen_player: the career has too few players or nations of a kind");
    }
    std::uint32_t nation(std::uint32_t index) const { return nations + index * 0x128; }
    std::uint32_t record(std::uint32_t slot) const { return players + slot * 0x48; }
    std::uint32_t row(std::uint32_t person) const { return kinds + s.r32(person) * 100u; }
};

std::string number(std::int64_t n) { return std::to_string(n); }
// CM_NEWGEN_PLAYER_FULL=1: every family at its full size (run once by hand); the default keeps a subset that still reaches every site and outcome.
bool full() { return std::getenv("CM_NEWGEN_PLAYER_FULL") != nullptr; }
std::vector<Case::Poke> traced() { return {{trace_callee, {}}}; }

// What a case sets up: which real person and player record slot, the plan's nation indices and the edits.
struct Setup {
    std::uint32_t index = 0;           // which real person (and slot)
    std::uint32_t nation = 0, second = 0;
    std::int32_t reputation = 1000;    // the planned nation's reputation word
    std::uint32_t flag = 0, word_74 = 1;
    std::vector<Write> more;
    std::uint32_t kind_class = 1;      // the marker bits 2..3 of the kind row
    std::uint32_t club = 0;
};

Case make(const World& w, const std::string& label, const Setup& setup, std::vector<Write> memory, bool trace = true) {
    const auto person = w.people[setup.index % w.people.size()];
    const auto slot = w.slots[setup.index % w.slots.size()];
    const auto nation = w.nation(setup.nation);
    append(memory, {w32(w.plan, slot), w32(w.plan + 4, setup.nation), w32(w.plan + 8, setup.second), w32(w.regen + 0x40, 1),
                    w32(nation + 0x6c, std::uint32_t(setup.reputation)), w8(nation + 0x120, setup.flag), w32(nation + 0x74, setup.word_74),
                    w8(w.row(person) + 0xc, setup.kind_class << 2), w32(person + 0x24, setup.club)});
    append(memory, setup.more);
    // The routine that forgets the person's history (0x10186ea0) opens a file for some people (CarbonLib::HGetVol, not implemented here): answered, with
    // the same answer on both sides, so the cases compare this routine's own calls, order and stores.
    return {.entry = player_entry, .registers = {{3, w.regen}, {4, person}}, .memory = std::move(memory), .label = "newgen_player: " + label,
            .answers = {{forget_history, 0}}, .pokes = trace ? traced() : std::vector<Case::Poke>{}};
}

// A remainder of `want` below `modulus` at salt 0, as a sample (a random multiple added).
std::uint32_t sample_for(std::mt19937& random, std::uint32_t modulus, std::uint32_t want) {
    return (want % modulus) + modulus * (random() % (0x10000 / modulus));
}
std::vector<std::uint32_t> samples_of(std::mt19937& random, unsigned count) {
    std::vector<std::uint32_t> samples;
    for (unsigned k = 0; k < count; ++k) samples.push_back(random() & 0xffff);
    return samples;
}
void append_registers(Case& item, std::initializer_list<std::pair<unsigned, std::uint32_t>> more) { for (const auto& r : more) item.registers.push_back(r); }
template <typename T, std::size_t N> const T& pick(std::mt19937& random, const std::array<T, N>& list) { return list[random() % N]; }

// Edits of the player record the slot names: the potential, the ability, the reputations, the position ratings and the personality bytes the
// routine reads before it writes them.
std::vector<Write> record_edits(const World& w, std::mt19937& random, std::uint32_t slot, bool ratings = true) {
    static const std::array<std::int32_t, 24> potentials{-32768, -20000, -100, -3, -2, -1, 0, 1, 2, 3, 50, 100, 101, 139, 140, 141, 150, 200, 1000, 5000, 20000, 32767, 120, 141};
    static const std::array<std::uint32_t, 10> position{0, 1, 10, 14, 15, 16, 20, 20, 0x80, 0xff};
    static const std::array<std::uint32_t, 12> byte_values{0, 1, 4, 5, 9, 10, 11, 15, 16, 20, 0x7f, 0xf0};
    const auto record = w.record(slot);
    std::vector<Write> memory{w16(record + 6, std::uint32_t(pick(random, potentials))), w16(record + 4, random() & 0xffff)};
    if (random() % 3 == 0) memory.push_back(w16(record + 6, random() & 0xffff));
    for (const auto offset : {0x1eu, 0x21u, 0x24u, 0x2cu, 0x2fu}) memory.push_back(w8(record + offset, pick(random, byte_values)));
    if (ratings)
        for (const auto offset : {0xfu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x19u}) memory.push_back(w8(record + offset, pick(random, position)));
    return memory;
}

// A random setup over everything the routine reads to branch.
Setup random_setup(const World& w, std::mt19937& random) {
    static const std::array<std::int32_t, 26> reputations{-2147483647 - 1, -70000, -65536, -65535, -1000, -1, 0, 1, 50, 99, 100, 101, 200, 349, 350, 351, 1000, 5000, 9999, 65535, 65536, 70000, 2147483647, 0, 349, 350};
    Setup s;
    s.index = random();
    s.reputation = pick(random, reputations);
    if (random() % 4 == 0) s.reputation = std::int32_t(random() % 700) - 100;
    s.flag = random() % 5 == 0 ? (random() % 2 ? 0x80 : 1) : 0;
    s.word_74 = random() % 3 ? 1 : 0;
    const auto kind = random() % 8;
    const auto& list = kind < 3 ? w.low_nations : kind < 6 ? w.mid_nations : w.high_nations;
    s.nation = list[random() % list.size()];
    const auto choice = random() % 8;
    s.second = choice == 0 ? 0 : choice == 1 ? std::uint32_t(w.nation_count) : choice == 2 ? 0xffffffffu : choice == 3 ? std::uint32_t(w.nation_count) - 1 : random() % std::uint32_t(w.nation_count);
    return s;
}
}

// The first cases: every draw refilling over many setups (a traced case pairs the refill sites of its whole path), then the same with a few
// refills at chosen draws. Together they pair every refill site of the entry.
PLAYER_REGEN_CASES("newgen_player") {
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(2025);
    for (unsigned n = 0; n < (full() ? 140u : 40u); ++n) {
        auto setup = random_setup(w, random);
        const auto slot = w.slots[setup.index % w.slots.size()];
        auto memory = ring.every_draw(random() & 0xffff, random() & 0xff);
        append(memory, record_edits(w, random, slot));
        append(memory, {w8(w.people[setup.index % w.people.size()] + 0x61, random() & 0xff)});
        cases.push_back(make(w, "every draw refills " + number(n), setup, memory));
    }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The refill at each draw position in turn (the draw after `before` samples refills), over setups: the pool's state after the call
    // (cursor, salt, the generator's word) tells a missing, extra or moved draw.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(31337);
    for (std::uint32_t before = 0; before < (full() ? 90u : 62u); ++before)
        for (unsigned variant = 0; variant < (full() ? 4u : 1u); ++variant) {
            auto setup = random_setup(w, random);
            const auto slot = w.slots[setup.index % w.slots.size()];
            auto memory = ring.scripted(before, random() & 0xff, samples_of(random, 100));
            append(memory, record_edits(w, random, slot));
            cases.push_back(make(w, "refill at draw " + number(before) + " variant " + number(variant), setup, memory, variant == 0 && before % 9 == 0));
        }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The gate: a person whose kind row has no regen marker (bits 2..3 of byte +0xc zero), or who has a club, only has the marker cleared.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(11);
    for (std::uint32_t kind = 0; kind < 4; ++kind)
        for (const bool has_club : {false, true}) {
            Setup setup;
            setup.index = kind * 7 + 1;
            setup.kind_class = kind;
            setup.club = has_club ? scenario.club(3) : 0;
            auto memory = ring.scripted(40, 0, samples_of(random, 100));
            cases.push_back(make(w, "gate marker " + number(kind) + (has_club ? " with a club" : " without a club"), setup, memory));
        }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The nation. A planned nation under 100 reputation draws below 4; on 0 a wide draw below the nations' count picks another, taken when it has
    // word +0x74 and a reputation under 100. The second nation by the plan's third index against the count, the human nation and the other cell,
    // with draws below 100 and below 3 in the original's order. Forced through the samples (salt 0): draw 0 is the draw below 4.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(5);
    const auto count_cell = scenario.toc(nation_count_slot), human_cell = scenario.toc(human_nation_slot), other_cell = scenario.toc(other_nation_slot);
    const std::array<std::int32_t, 9> counts{0, 1, 2, 3, 9, 0x10000, 0x10001, -1, -3};
    unsigned n = 0, seen = 0;
    for (const auto count : counts)
        for (const std::uint32_t roll : {0u, 1u})
            for (const std::uint32_t second : {0u, 1u, 2u, 3u, 8u, 0xffffffffu})
                for (const std::uint32_t human : {0u, 1u})
                    for (const std::uint32_t other : {2u, 5u}) {
                        if (!full() && seen++ % 7 != 0) continue;
                        Setup setup;
                        setup.index = 100 + n;
                        setup.nation = (human ? 0u : 1u);  // the human nation cell below is 0 or 7: equal to the planned nation for `human`
                        setup.reputation = std::int32_t(random() % 100) - 10;
                        setup.second = second;
                        auto samples = samples_of(random, 12);
                        samples[0] = sample_for(random, 4, roll == 0 ? 0 : 1);
                        samples[1] = count >= 0x10000 || count < 0 ? random() % 20 : random() % 65535;
                        samples[2] = sample_for(random, 100, (n % 3) == 0 ? 0 : 7);
                        samples[3] = sample_for(random, 3, (n % 2) == 0 ? 0 : 1);
                        auto memory = ring.scripted(40, 0, samples);
                        const bool odd = count >= 0x10000 || count < 0;
                        append(memory, {w32(human_cell, human ? 0u : 7u), w32(other_cell, other)});
                        if (!odd) append(memory, {w32(count_cell, std::uint32_t(count))});
                        // the candidate nations the draw can pick: word +0x74 and a reputation under 100 or not
                        for (std::uint32_t k = 0; k < 3; ++k)
                            append(memory, {w32(w.nation(k) + 0x74, (random() + k) % 2), w32(w.nation(k) + 0x6c, random() % 2 ? 50u : 150u)});
                        auto item = make(w, "nation count " + number(count) + " roll " + number(roll) + " second " + number(second) + " human " + number(human) +
                                                  " other " + number(other), setup, memory, ++n % 9 == 0 || (human == 1 && second >= 1 && second <= 3));
                        // A count of 0x10000 or more, or a negative one, makes the callees that loop over the nations leave their table (and fault on both sides):
                        // it is written when the last callee of the reset (0x10084b10, answered) is dispatched and the real count when the first callee after the
                        // draws (0x100c4400) is, so that only this routine's own draws read it.
                        if (odd) {
                            item.pokes.push_back({0x10084b10, {w32(count_cell, std::uint32_t(count))}});
                            item.pokes.push_back({0x100c4400, {w32(count_cell, std::uint32_t(w.nation_count))}});
                            item.answers.push_back({0x10084b10, 0});
                        }
                        // The three callees after the draws (0x100c4400, 0x100c4cf0, 0x100c53e0) leave r7 and r8 as they like; answered, they leave them as they were, which
                        // shows the registers the entry itself sets (`li r7,0`, `li r8,1`) and the two copies of the wide draw leave for them.
                        if (n % 2 == 0) for (const auto callee : {0x100c4400u, 0x100c4cf0u, 0x100c53e0u}) item.answers.push_back({callee, 0});
                        cases.push_back(item);
                    }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The potential and the position ratings. The strong branch (a nation with a flag, or a reputation of 350 and more) picks its ability arm by the
    // ratings (+0x15 = 20; +0x11 or +0x10 = 20; +0xf = 20; else), the first draw's bound is trunc(factor * potential): 0 draws nothing.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(77);
    const std::array<std::int32_t, 30> potentials{-32768, -32767, -30000, -100, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 99, 100, 101, 139, 140, 141, 150, 200, 1000,
                                                  5000, 20000, 21845, 21846, 32766, 32767, 70};
    unsigned n = 0, seen = 0;
    for (const auto potential : potentials)
        for (std::uint32_t pattern = 0; pattern < 6; ++pattern)
            for (const bool clear : {false, true}) {
                if (!full() && seen++ % 5 != 0) continue;
                Setup setup;
                setup.index = 300 + n;
                setup.nation = w.high_nations[n % w.high_nations.size()];
                setup.reputation = 400;
                const auto slot = w.slots[setup.index % w.slots.size()];
                const auto record = w.record(slot);
                setup.more = {w16(record + 6, std::uint32_t(potential)), w8(record + 0x15, pattern == 0 ? 20u : 3u), w8(record + 0x11, pattern == 1 ? 20u : 3u),
                              w8(record + 0x10, pattern == 2 ? 20u : 3u), w8(record + 0xf, pattern == 3 ? 20u : 3u), w8(record + 0x14, pattern == 4 ? 15u : 3u),
                              w8(record + 0x19, pattern == 5 ? 15u : 3u)};
                auto memory = ring.scripted(40, random() & 0xff, samples_of(random, 100));
                auto item = make(w, "strong potential " + number(potential) + " pattern " + number(pattern), setup, memory, false);
                item.clear_fpscr = clear;  // the snapshot's FPSCR has the sticky inexact bit set, which hides fctiwz's inexact result
                cases.push_back(item);
                ++n;
            }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The strong branch with forced draws (salt 0; the nation's reputation is at least 100, the second nation 0: no draw before the ability).
    // Draw 0 is the wide draw below n (potential 200: n = 120, ability = 50 + the draw in the arm without a natural position), draw 1 the one
    // below 250 (0 takes the other form), then below 750, 20 (not after a 0), 50 and 10000 (the age), the age draw and the ability's cap test.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(123);
    unsigned n = 0, seen = 0;
    for (const std::uint32_t ability : {29u, 30u, 31u, 39u, 40u, 41u, 49u, 50u, 51u, 59u, 60u, 61u})  // the draw: the ability is 50 + it, around the caps 80, 90, 100, 110
        for (const std::uint32_t roll250 : {0u, 5u})
            for (const std::uint32_t age_roll : {0u, 1u, 4999u, 5000u, 9975u, 9976u})
                for (const std::uint32_t age_draw : {0u, 1u, 3u})
                    for (const std::uint32_t cap_draw : {0u, 7u}) {
                        if (!full() && seen++ % 23 != 0) continue;
                        Setup setup;
                        setup.index = 700 + n;
                        setup.nation = w.high_nations[n % w.high_nations.size()];
                        setup.reputation = 400;
                        const auto record = w.record(w.slots[setup.index % w.slots.size()]);
                        setup.more = {w16(record + 6, 200), w8(record + 0x15, 3), w8(record + 0x11, 3), w8(record + 0x10, 3), w8(record + 0xf, 3)};
                        auto samples = samples_of(random, 20);
                        samples[0] = sample_for(random, 120, ability);
                        samples[1] = sample_for(random, 250, roll250);
                        samples[2] = random() & 0xffff;                                          // below 750
                        samples[3] = roll250 == 0 ? sample_for(random, 50, 5) : sample_for(random, 20, 3);  // below 50, or below 20 (then 50 follows)
                        const auto at = roll250 == 0 ? 4u : 5u;
                        samples[at] = sample_for(random, 10000, age_roll);
                        samples[at + 1] = sample_for(random, 12, age_draw);
                        samples[at + 2] = sample_for(random, 250, cap_draw);
                        auto memory = ring.scripted(60, 0, samples);
                        cases.push_back(make(w, "strong forced ability " + number(ability) + " roll " + number(roll250) + " age roll " + number(age_roll) + " age draw " +
                                                   number(age_draw) + " cap " + number(cap_draw), setup, memory, true));
                        ++n;
                    }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The modest branch (a nation without a flag and with a reputation under 350) with forced draws (the nation's reputation 100..349: no
    // draw before the ability): draw 0 the wide draw below the reputation (none for 0), draw 1 the one below 1000 it is compared with, draw 2 the age
    // (below 5 plus 23 when the first is under the second, else below 7 plus 17), draw 3 the wide draw below n, then below 750.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(9);
    unsigned n = 0, seen = 0;
    for (const std::int32_t reputation : {100, 200, 349})
        for (const bool young : {false, true})
            for (std::uint32_t age_draw = 0; age_draw < 7; ++age_draw)
                for (const std::int32_t potential : {60, 141, 200, 300})
                    for (const std::uint32_t target : {79u, 100u, 101u, 119u, 120u, 121u, 139u, 140u, 141u})
                        for (unsigned repeat = 0; repeat < 2; ++repeat) {
                            if (!full() && seen++ % 40 != 0) continue;
                            if (!young && age_draw >= 5) continue;
                            Setup setup;
                            setup.index = 1500 + n;
                            setup.nation = w.mid_nations[n % w.mid_nations.size()];
                            setup.reputation = reputation;
                            const auto record = w.record(w.slots[setup.index % w.slots.size()]);
                            setup.more = {w16(record + 6, std::uint32_t(potential))};
                            auto samples = samples_of(random, 40);
                            samples[0] = young ? sample_for(random, std::uint32_t(reputation), std::uint32_t(reputation) - 1) : 0;  // against the draw below 1000
                            samples[1] = young ? sample_for(random, 1000, 1) : sample_for(random, 1000, 999);
                            samples[2] = sample_for(random, young ? 7 : 5, age_draw);  // below 7 plus 17 (young) or below 5 plus 23
                            // the ability: the first factor is 0.55 above 20 and 0.6 up to 20, the second 0.25: 0.25 * potential + the draw
                            const auto age = (young ? 17u : 23u) + age_draw;
                            const auto bound = std::uint32_t(potential) * (age > 20 ? 55u : 60u) / 100;
                            samples[3] = sample_for(random, bound ? bound : 1, std::int32_t(target) > potential / 4 ? target - std::uint32_t(potential / 4) : 0u);
                            auto memory = ring.scripted(60, 0, samples);
                            auto item = make(w, "modest reputation " + number(reputation) + (young ? " young " : " old ") + number(age_draw) + " potential " + number(potential) +
                                                    " ability " + number(target), setup, memory, true);
                            item.clear_fpscr = repeat == 1;
                            cases.push_back(item);
                            ++n;
                        }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The wide draws (the nations' count, the modest nation's reputation) and the reputations around both thresholds.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(404);
    unsigned n = 0, seen = 0;
    for (const std::int32_t reputation : {-2147483647 - 1, -2147483647, -131072, -70000, -65537, -65536, -65535, -32768, -100, -2, -1, 0, 1, 2, 98, 99, 100, 101,
                                          200, 348, 349, 350, 351, 1000, 65535, 70000})
        for (const std::uint32_t flag : {0u, 1u, 0x80u, 0x100u})
            for (unsigned variant = 0; variant < 3; ++variant) {
                if (!full() && seen++ % 7 != 0) continue;
                Setup setup;
                setup.index = 2500 + n;
                setup.nation = (variant == 0 ? w.low_nations : variant == 1 ? w.mid_nations : w.high_nations)[n % (variant == 0 ? w.low_nations : variant == 1 ? w.mid_nations : w.high_nations).size()];
                setup.reputation = reputation;
                setup.flag = flag;
                setup.second = n % 5;
                auto samples = samples_of(random, 100);
                if (variant == 0) samples[0] = sample_for(random, 4, 0);
                auto memory = ring.scripted(random() % 4 * 5, random() & 0xff, samples);
                append(memory, record_edits(w, random, w.slots[setup.index % w.slots.size()]));
                cases.push_back(make(w, "reputation " + number(reputation) + " flag " + number(flag) + " variant " + number(variant), setup, memory, n % 11 == 0));
                ++n;
            }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The placement: a young player (under 19) of a nation with the flag set is placed on three draws out of four: the club pick (0x102e99f0, run for real:
    // a routine of this module is not answered by the checker), the signing (0x102dc000), the news check (answered here), the youth promotion's news and a
    // release from the club's staff by a draw below 100 against a byte. The byte of every club's link record and the potential are set so each side of the
    // draw's comparison and of the potential's test is reached, whichever club the pick gives.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(1717);
    const auto clubs = std::min<std::uint32_t>(scenario.global(-0x7b50), 6000u);
    unsigned n = 0, seen = 0;
    for (const std::uint32_t news : {0u, 1u})
        for (const std::int32_t potential : {40, 0x50, 0x51, 200})
            for (const std::uint32_t byte : {0x80u, 0u, 1u, 50u, 99u, 100u, 127u})
                for (unsigned repeat = 0; repeat < 3; ++repeat) {
                    if (!full() && seen++ % 13 != 0) continue;
                    Setup setup;
                    setup.index = 3500 + n;
                    setup.nation = w.high_nations[n % w.high_nations.size()];
                    setup.reputation = 400;
                    setup.flag = 1;
                    const auto record = w.record(w.slots[setup.index % w.slots.size()]);
                    setup.more = {w16(record + 6, std::uint32_t(potential))};
                    for (std::uint32_t k = 0; k < clubs; ++k) {
                        const auto link = scenario.r32(scenario.club(k) + 0x80);
                        if (link != 0 && scenario.r32(link + 0x1c) != 0) setup.more.push_back(w8(scenario.r32(link + 0x1c) + 0x36, byte));
                    }
                    auto samples = samples_of(random, 60);
                    samples[1] = sample_for(random, 250, 7);        // the strong path: not the zero form of the reputations
                    samples[5] = sample_for(random, 10000, 100);    // the age roll below 5000: the age 16..18, so he is placed
                    auto memory = ring.scripted(80, 0, samples);
                    auto item = make(w, "placement club news " + number(news) + " potential " + number(potential) + " byte " + number(byte) + " " + number(repeat), setup,
                                     memory, n % 37 == 0);
                    item.answers.push_back({news_check, news});
                    cases.push_back(item);
                    ++n;
                }
}

PLAYER_REGEN_CASES("newgen_player") {
    // Random setups over everything (the default run's bulk; the pool in every position and with every content).
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(424242);
    const auto count = full() ? 12000u : 120u;
    for (unsigned n = 0; n < count; ++n) {
        auto setup = random_setup(w, random);
        const auto slot = w.slots[setup.index % w.slots.size()];
        auto memory = n % 5 == 0 ? ring.every_draw(random() & 0xffff, random() & 0xff) : ring.scripted(random() % 80, random() & 0xff, samples_of(random, 100));
        append(memory, record_edits(w, random, slot));
        if (random() % 6 == 0) append(memory, {w8(w.people[setup.index % w.people.size()] + 0x61, random() & 0xff)});
        if (random() % 8 == 0) append(memory, {w16(scenario.toc(date_slot), random() % 400), w16(scenario.toc(date_slot) + 2, random() % 70000)});
        auto item = make(w, "random " + number(n), setup, memory, n % 6 == 0);
        // The caller's r5..r10 are whatever it left: the entry's callees read r7..r10 as leftovers (its own `li r7,0` / `li r8,1` and the callees' leftovers decide).
        if (n % 4 == 0) append_registers(item, {{7, 0x7a7a7a7a}, {8, 0x8b8b8b8b}, {9, 0x9c9c9c9c}, {10, 0xadadadad}});
        item.clear_fpscr = n % 2 == 1;
        cases.push_back(item);
    }
}

PLAYER_REGEN_CASES("newgen_player") {
    // A null nation: the planned nation index 0 of a nations table whose cell is 0 gives the person nation 0, which the original treats as a strong
    // nation and reads at addresses 0x6c, 0x120 (low memory: the cases fault on both sides unless the checker runs with --null-page).
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(31);
    const auto nations_cell = scenario.toc(nations_slot);
    for (unsigned n = 0; n < 8; ++n) {
        Setup setup;
        setup.index = 4700 + n;
        setup.nation = 0;
        setup.second = n % 2;
        auto memory = ring.scripted(40, 0, samples_of(random, 100));
        append(memory, {w32(nations_cell, 0)});
        append(memory, record_edits(w, random, w.slots[setup.index % w.slots.size()]));
        cases.push_back(make(w, "null nation " + number(n), setup, memory, false));
    }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The exhaustive sweeps (CM_NEWGEN_PLAYER_FULL=1, run once by hand on the final code; CM_NEWGEN_PLAYER_STRIDE=N takes every Nth potential): every potential from -32768 to 32767 in a strong and in a
    // modest nation (the bound of the first draw and the ability's rounding through fmadd), and every value of each position rating byte and of the
    // bytes the routine reads before it writes them.
    if (!std::getenv("CM_NEWGEN_PLAYER_FULL")) return;
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(8128);
    const auto stride = std::getenv("CM_NEWGEN_PLAYER_STRIDE") ? std::int32_t(std::atoi(std::getenv("CM_NEWGEN_PLAYER_STRIDE"))) : 1;  // every value by default
    for (std::int32_t potential = -32768; potential < 32768; potential += stride)
        for (const bool strong : {true, false}) {
            Setup setup;
            setup.index = std::uint32_t(potential + 40000);
            setup.nation = strong ? w.high_nations[std::uint32_t(potential + 40000) % w.high_nations.size()] : w.mid_nations[std::uint32_t(potential + 40000) % w.mid_nations.size()];
            setup.reputation = strong ? 400 : 200;
            const auto record = w.record(w.slots[setup.index % w.slots.size()]);
            setup.more = {w16(record + 6, std::uint32_t(potential))};
            auto memory = ring.scripted(40, random() & 0xff, samples_of(random, 100));
            auto item = make(w, "full potential " + number(potential) + (strong ? " strong" : " modest"), setup, memory, false);
            item.clear_fpscr = potential % 2 != 0;
            cases.push_back(item);
        }
    for (const auto offset : {0xfu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x19u, 0x1eu, 0x21u, 0x24u, 0x2cu, 0x2fu})
        for (std::uint32_t value = 0; value < 256; ++value)
            for (const bool strong : {true, false}) {
                Setup setup = random_setup(w, random);
                setup.nation = strong ? w.high_nations[value % w.high_nations.size()] : w.mid_nations[value % w.mid_nations.size()];
                setup.reputation = strong ? 400 : 200;
                setup.flag = 0;
                setup.second = 0;
                const auto record = w.record(w.slots[setup.index % w.slots.size()]);
                setup.more = {w8(record + offset, value)};
                auto memory = ring.scripted(40, random() & 0xff, samples_of(random, 100));
                cases.push_back(make(w, "full byte " + hex(offset) + " = " + number(value) + (strong ? " strong" : " modest"), setup, memory, false));
            }
}

PLAYER_REGEN_CASES("newgen_player") {
    // A negative sum (the pool's salt cell below -65535: the checker's mutated runs do it too, the refill never does): every remainder is then negative
    // (C's truncating remainder), which reaches outcomes no real salt does: a personality byte below its threshold after the first draw, a wide draw
    // below a negative bound's mirror, a signed byte below zero.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(6502);
    for (unsigned n = 0; n < (full() ? 600u : 45u); ++n) {
        auto setup = random_setup(w, random);
        const auto slot = w.slots[setup.index % w.slots.size()];
        const std::uint32_t salt = 0x80000000u + (random() % 0x7fff0000u);  // -2^31 .. -65537
        auto memory = n % 3 == 0 ? ring.every_draw(random() & 0xffff, salt) : ring.scripted(random() % 80, salt, samples_of(random, 100));
        append(memory, record_edits(w, random, slot));
        auto item = make(w, "negative salt " + number(n), setup, memory, n % 3 == 0);
        item.clear_fpscr = n % 2 == 1;
        cases.push_back(item);
    }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The refill at the draw of a rare outcome, so that the call trace pairs the refill sites only those outcomes reach. The draw numbers of the strong
    // path are known (draw 0 the wide draw below n, 1 below 250, 2 below 750, then below 20 (not after a 0) and 50, below 10000 at `at`, the age at
    // `at` + 1, the cap's test at `at` + 2): the refill is at the draw below 750 of the form after a 0, at the age draw (the age 15 and the ages of
    // the other rolls) and at the cap's test. The modest path (reputation 100..349: draw 0 the wide one, 1 below 1000, 2 the age, 3 the wide draw below n,
    // 4 below 750, then below 4 (the ages 19..21 of a strong ability), below 200, 50, the tiers' tests): the refill at each of draws 5..8.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(2718);
    for (const std::uint32_t roll250 : {0u, 5u})
        for (const std::uint32_t age_roll : {0u, 1u, 4999u, 5000u, 9975u, 9976u})
            for (const std::uint32_t age_draw : {0u, 1u})
                for (unsigned kind = 0; kind < 3; ++kind) {
                    if (!full() && age_roll != 0 && age_roll != 9976) continue;
                    Setup setup;
                    setup.index = 6000 + std::uint32_t(cases.size());
                    setup.nation = w.high_nations[cases.size() % w.high_nations.size()];
                    setup.reputation = 400;
                    const auto record = w.record(w.slots[setup.index % w.slots.size()]);
                    setup.more = {w16(record + 6, 200), w8(record + 0x15, 3), w8(record + 0x11, 3), w8(record + 0x10, 3), w8(record + 0xf, 3)};
                    auto samples = samples_of(random, 20);
                    const std::uint32_t at = roll250 == 0 ? 4 : 5;
                    samples[0] = sample_for(random, 120, 70);  // the ability 120: above every cap
                    samples[1] = sample_for(random, 250, roll250);
                    samples[at] = sample_for(random, 10000, age_roll);
                    samples[at + 1] = sample_for(random, 12, age_draw);
                    samples[at + 2] = sample_for(random, 250, 7);
                    const std::uint32_t before = kind == 0 ? 2u : kind == 1 ? at + 1 : at + 2;
                    cases.push_back(make(w, "strong refill roll " + number(roll250) + " age roll " + number(age_roll) + " age draw " + number(age_draw) + " at " + number(before),
                                         setup, ring.scripted(before, 0, samples), true));
                }
    for (const std::uint32_t age_draw : {0u, 1u, 2u, 3u, 4u})
        for (const std::uint32_t target : {121u, 141u})
            for (const std::uint32_t before : {5u, 6u, 7u, 8u}) {
                if (!full() && target != 141 && !(age_draw >= 2 && age_draw <= 3)) continue;
                Setup setup;
                setup.index = 6500 + std::uint32_t(cases.size());
                setup.nation = w.mid_nations[cases.size() % w.mid_nations.size()];
                setup.reputation = 200;
                const auto record = w.record(w.slots[setup.index % w.slots.size()]);
                setup.more = {w16(record + 6, 200)};
                auto samples = samples_of(random, 30);
                samples[0] = sample_for(random, 200, 199);  // against the draw below 1000: not under it, the age is below 7 plus 17
                samples[1] = sample_for(random, 1000, 1);
                samples[2] = sample_for(random, 7, age_draw);
                samples[3] = sample_for(random, 17 + age_draw > 20 ? 110 : 120, target - 50);  // the ability 50 + the draw (factor 0.25 of the potential 200)
                cases.push_back(make(w, "modest refill age " + number(age_draw) + " ability " + number(target) + " at " + number(before), setup,
                                     ring.scripted(before, 0, samples), true));
            }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The refill at each draw position on both paths with a negative sum (the salt below -65535): the personality bytes below their thresholds after a
    // first draw (the original's `b < 5` tests, which a positive remainder never reaches) and their refills.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(1414);
    for (std::uint32_t before = 8; before < (full() ? 52u : 26u); ++before)
        for (const bool modest : {false, true}) {
            if (modest && before < 20) continue;
            Setup setup;
            setup.index = 7000 + std::uint32_t(cases.size());
            setup.nation = modest ? w.mid_nations[before % w.mid_nations.size()] : w.high_nations[before % w.high_nations.size()];
            setup.reputation = modest ? 200 : 400;
            const auto record = w.record(w.slots[setup.index % w.slots.size()]);
            setup.more = {w16(record + 6, 200)};
            auto samples = samples_of(random, 100);
            if (modest) {
                samples[0] = sample_for(random, 200, 199);
                samples[1] = sample_for(random, 1000, 1);
            }
            const std::uint32_t salt = 0xfffe0000u + (random() & 0xff);  // -131072 + a little
            cases.push_back(make(w, std::string("negative salt refill ") + (modest ? "modest " : "strong ") + number(before), setup, ring.scripted(before, salt, samples), true));
        }
}

PLAYER_REGEN_CASES("newgen_player") {
    // Candidates over a mix of signs (sample + salt), refills at random draws and random records, of which the default run keeps those that pair the
    // refill sites of rare outcomes no family above reaches with a refill there (found by bisecting the search: the personality bytes below their
    // thresholds after a first draw and the age 18 of a strong nation's cap). CM_NEWGEN_PLAYER_SEARCH=N runs the first N candidates (labels
    // "found g=G n=N"; CM_NEWGEN_PLAYER_SEARCH_FROM / _TO limit the range) to search again.
    const auto* search = std::getenv("CM_NEWGEN_PLAYER_SEARCH");
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(99);
    constexpr std::array<unsigned, 6> kept{32, 336, 505, 634, 1074, 1102};
    const unsigned count = search ? unsigned(std::atoi(search)) : 1200;
    for (unsigned n = 0; n < count; ++n) {
        auto setup = random_setup(w, random);
        setup.reputation = random() % 3 == 0 ? 400 : 200;
        setup.nation = (setup.reputation > 349 ? w.high_nations : w.mid_nations)[random() % (setup.reputation > 349 ? w.high_nations.size() : w.mid_nations.size())];
        setup.flag = 0;
        setup.second = 0;
        const auto slot = w.slots[setup.index % w.slots.size()];
        auto edits = record_edits(w, random, slot);
        const auto record = w.record(slot);
        append(edits, {w16(record + 6, 120 + random() % 200)});
        const std::uint32_t salt = random() % 4 == 0 ? 0u : 0xffffffffu - (random() % 40000);
        auto memory = ring.scripted(6 + random() % 50, salt, samples_of(random, 100));
        append(memory, edits);
        const auto* from = std::getenv("CM_NEWGEN_PLAYER_SEARCH_FROM");
        const auto* to = std::getenv("CM_NEWGEN_PLAYER_SEARCH_TO");
        if (from && n < unsigned(std::atoi(from))) continue;
        if (to && n >= unsigned(std::atoi(to))) continue;
        if (!search && std::find(kept.begin(), kept.end(), n) == kept.end()) continue;
        cases.push_back(make(w, "found g=" + number(n % 12) + " n=" + number(n), setup, memory, true));
    }
}

PLAYER_REGEN_CASES("newgen_player") {
    // The refill at each of the first draws of the nation choice (the draw below 4, the wide draw below the count, the draw below 100 of the human
    // nation and the draw below 3): the call trace pairs those sites only with a refill there.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(3);
    const auto human_cell = scenario.toc(human_nation_slot), other_cell = scenario.toc(other_nation_slot);
    for (std::uint32_t before = 0; before < 4; ++before)
        for (const std::uint32_t roll : {0u, 1u})
            for (const std::uint32_t second : {1u, 2u})
                for (const std::uint32_t human : {0u, 1u}) {
                    Setup setup;
                    setup.index = 8000 + std::uint32_t(cases.size());
                    setup.nation = human ? 0u : 1u;
                    setup.reputation = 50;
                    setup.second = second;
                    auto samples = samples_of(random, 12);
                    samples[0] = sample_for(random, 4, roll == 0 ? 0 : 1);
                    samples[1 + (roll == 0 ? 1 : 0)] = sample_for(random, 100, 7);  // the draw below 100 (not 0: the draw below 3 follows)
                    auto memory = ring.scripted(before, 0, samples);
                    append(memory, {w32(human_cell, human ? 0u : 7u), w32(other_cell, 99)});
                    cases.push_back(make(w, "nation refill at " + number(before) + " roll " + number(roll) + " second " + number(second) + " human " + number(human), setup, memory, true));
                }
}

PLAYER_REGEN_CASES("newgen_player") {
    // Outcomes the families above leave to chance or cannot reach with a real person: the human managers' hook cells (the original calls 0x106fd9f0 only when
    // both are set), a person who is also staff (his staff link is cleared), the zero remainders of the ability caps (a draw below 150 or 50 that
    // is 0 leaves the ability), and the placement's outcomes (no link, a link with no record, a signing that fails).
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(555);
    const auto hook_cell = scenario.toc(-0x7978), hook_flag = scenario.toc(-0x797c);
    const auto clubs_all = std::min<std::uint32_t>(scenario.global(-0x7b50), 6000u);
    unsigned n = 0;
    for (const bool cell_set : {false, true})
        for (const bool flag_set : {false, true}) {
            Setup setup;
            setup.index = 9000 + n;
            setup.nation = w.high_nations[n % w.high_nations.size()];
            auto memory = ring.scripted(40, 0, samples_of(random, 100));
            append(memory, {w32(hook_cell, cell_set ? scenario.r32(hook_cell) : 0u), w8(hook_flag, flag_set ? 1u : 0u)});
            cases.push_back(make(w, std::string("hook cell ") + (cell_set ? "set" : "zero") + " flag " + (flag_set ? "set" : "zero"), setup, memory, false));
            ++n;
        }
    std::vector<std::uint32_t> staff;
    for (std::uint32_t k = 0; k + 16 < scenario.people() && staff.size() < 6; ++k)
        if (scenario.r32(scenario.person(k) + 0x1c) != 0 && scenario.r32(scenario.person(k) + 0x14) != 0) staff.push_back(scenario.person(k));
    for (const auto person : staff) {
        Setup setup;
        setup.index = 9100 + n;
        setup.nation = w.high_nations[n % w.high_nations.size()];
        auto memory = ring.scripted(40, 0, samples_of(random, 100));
        auto item = make(w, "a person who is staff " + number(n), setup, memory, false);
        item.registers = {{3, w.regen}, {4, person}};
        item.memory.push_back(w8(w.row(person) + 0xc, 4));
        item.memory.push_back(w32(person + 0x24, 0));
        // the plan's slot: the person's own player record
        item.memory.push_back(w32(w.plan, (scenario.r32(person + 0x14) - w.players) / 0x48));
        cases.push_back(item);
        ++n;
    }
    // The modest branch's cap draws forced to 0 and not 0: draw numbers 7 (age 17 and 18, the cap below 150 / 50) and 8 (age 20, ability 141, below 50).
    for (const std::uint32_t age_draw : {0u, 1u, 3u})
        for (const std::uint32_t cap : {0u, 7u}) {
            Setup setup;
            setup.index = 9200 + n;
            setup.nation = w.mid_nations[n % w.mid_nations.size()];
            setup.reputation = 200;
            const auto record = w.record(w.slots[setup.index % w.slots.size()]);
            setup.more = {w16(record + 6, 200)};
            auto samples = samples_of(random, 30);
            samples[0] = sample_for(random, 200, 199);
            samples[1] = sample_for(random, 1000, 1);
            samples[2] = sample_for(random, 7, age_draw);                    // the age 17, 18 or 20
            samples[3] = sample_for(random, 120, age_draw == 3 ? 91 : 89);  // the ability 141 or 139 (the potential 200: the bound 120 up to the age 20)
            samples[age_draw == 3 ? 8 : 7] = sample_for(random, 150, cap) ;
            cases.push_back(make(w, "modest cap draw " + number(cap) + " age draw " + number(age_draw), setup, ring.scripted(60, 0, samples), false));
            ++n;
        }
    // The modest branch at the age 22 and 23 with an ability of exactly 120 (the arm that applies to ages above 21 only while the ability is not above 120).
    for (const std::uint32_t age_draw : {5u, 6u}) {
        Setup setup;
        setup.index = 9250 + n;
        setup.nation = w.mid_nations[n % w.mid_nations.size()];
        setup.reputation = 200;
        const auto record = w.record(w.slots[setup.index % w.slots.size()]);
        setup.more = {w16(record + 6, 200)};
        auto samples = samples_of(random, 30);
        samples[0] = sample_for(random, 200, 199);
        samples[1] = sample_for(random, 1000, 1);
        samples[2] = sample_for(random, 7, age_draw);
        samples[3] = sample_for(random, 110, 70);  // above 20 the bound is 0.55 * 200 = 110: the ability 50 + 70 = 120
        cases.push_back(make(w, "modest ability 120 age draw " + number(age_draw), setup, ring.scripted(60, 0, samples), false));
        ++n;
    }
    // The potential's threshold of the modest branch's +8 test (above 140), both sides: the age 20, the ability 105.
    for (const std::int32_t potential : {140, 141}) {
        Setup setup;
        setup.index = 9270 + n;
        setup.nation = w.mid_nations[n % w.mid_nations.size()];
        setup.reputation = 200;
        const auto record = w.record(w.slots[setup.index % w.slots.size()]);
        setup.more = {w16(record + 6, std::uint32_t(potential))};
        auto samples = samples_of(random, 30);
        samples[0] = sample_for(random, 200, 199);
        samples[1] = sample_for(random, 1000, 1);
        samples[2] = sample_for(random, 7, 3);                          // the age 20
        samples[3] = sample_for(random, 84, 70);                        // the bound 0.6 * 140 = 84 (0.6 * 141 = 84): the ability 35 + 70
        cases.push_back(make(w, "modest potential " + number(potential) + " age 20", setup, ring.scripted(60, 0, samples), false));
        ++n;
    }
    // The release draw equal to the byte it is compared with (every draw's remainder below 100 is 50; the byte 49 or 50: the original does not release). Every club's
    // link is one chain of records in the scratch area, so whichever club the pick gives is compared.
    for (const std::uint32_t byte : {49u, 50u}) {
        Setup setup;
        setup.index = 9280 + n;
        setup.nation = w.high_nations[n % w.high_nations.size()];
        setup.flag = 1;
        setup.reputation = 400;
        const auto record = w.record(w.slots[setup.index % w.slots.size()]);
        setup.more = {w16(record + 6, 200), w32(scratch_base + 0x1000 + 0x1c, scratch_base + 0x2000), w8(scratch_base + 0x2000 + 0x36, byte)};
        for (std::uint32_t k = 0; k < clubs_all; ++k) setup.more.push_back(w32(scenario.club(k) + 0x80, scratch_base + 0x1000));
        std::vector<std::uint32_t> samples;
        for (unsigned k = 0; k < 80; ++k) samples.push_back(50 + 100 * (random() % 40));  // sum % 100 == 50 for every draw, below 5000
        cases.push_back(make(w, "placement roll equal byte " + number(byte), setup, ring.scripted(90, 0, samples), false));
        ++n;
    }
    // The placement after the real club pick: the signing refused (every club's squad list full: 0x102dc000 gives 0 when its 50 slots are used), a club
    // with no link, a link with no record (0x102dc000 and 0x102e99f0 are routines of this module: the checker does not answer them). The young strong path
    // is forced (the draw below 250 not 0, the age roll below 5000: the age 16..18, so he is placed).
    const auto clubs = std::min<std::uint32_t>(scenario.global(-0x7b50), 6000u);
    for (unsigned variant = 0; variant < 12; ++variant) {
        Setup setup;
        setup.index = 9300 + n;
        setup.nation = w.high_nations[n % w.high_nations.size()];
        setup.flag = 1;
        setup.reputation = 400;
        const auto record = w.record(w.slots[setup.index % w.slots.size()]);
        setup.more = {w16(record + 6, 200)};
        for (std::uint32_t k = 0; k < clubs; ++k) {
            const auto club = scenario.club(k);
            const auto link = scenario.r32(club + 0x80);
            if (variant % 3 == 0) for (std::uint32_t slot = 0; slot < 50; ++slot) setup.more.push_back(w32(club + 0x98 + 4 * slot, scenario.person(slot + 1)));
            if (variant % 3 == 1) setup.more.push_back(w32(club + 0x80, 0));
            if (variant % 3 == 2 && link != 0) setup.more.push_back(w32(link + 0x1c, 0));
        }
        auto samples = samples_of(random, 60);
        samples[1] = sample_for(random, 250, 7);
        samples[5] = sample_for(random, 10000, 100);
        auto memory = ring.scripted(80, 0, samples);
        auto item = make(w, "placement closer " + number(variant), setup, memory, false);
        item.answers.push_back({news_check, variant % 2});
        if (variant % 3 == 0) {  // "can sign" for every club and no members counted: the pick gives a club whose squad is full
            item.answers.push_back({0x103f83c0, 1});
            item.answers.push_back({0x100c9070, 0});
        }
        cases.push_back(item);
        ++n;
    }
}
}
