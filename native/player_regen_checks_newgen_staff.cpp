// PLAYER_REGEN.CPP, block `newgen_staff`: cases for 0x102e27e0, the regeneration of a retired non-player staff person. No capture reaches
// it, so every case starts from the career's real people (season-day): real staff with and without a club and with a national link, edited
// over every byte the routine reads to branch (the job bytes +0x62/+0x60 and the caller's r20 over 0..255, the staff record's +0x26 over
// the bound's thresholds, the club's reputation across 3250 and 6250 and its flag byte, the statistics' values and the threshold byte
// +0x61, the game's date, the contract test's answer) and run with the pooled generator in every position and with many contents.
//
// The pool. The checker compares the generator's whole state after the call (cursor, salt, end, state word, the sample under the cursor),
// so a missing, extra or reordered draw fails a case; and a refill is traced by its return address. Three ways of driving the pool:
//   * `every_draw`: the pool's end cell is set to its start, so every draw refills (cursor start, salt renewed by the generator): one
//     case then pairs the refill site of every draw of its path (the traced calls are paired one by one, only the first 32 cases of an
//     entry and every 64th are traced, so every case here carries a poke on a callee every case calls, which makes it traced);
//   * `scripted`: the cursor sits `before` draws short of the end and the samples each draw reads are given: draw n reads the n-th
//     sample (the salt chosen), so a remainder can be forced (a test draw to 0 or not), and the draw number `before` + 1 refills;
//   * the snapshot's own position with random samples.
// Labels start with "newgen_staff:".
#include "player_regen_checks.hpp"
#include <array>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t staff_entry = 0x102e27e0, trace_callee = 0x1014b690, contract_test = 0x1007e4f0, contract_clear = 0x10084a80,
                        contract_holder = 0x10082f20;
constexpr std::int32_t date_slot = -0x7b80, cursor_slot = -0x7b84, end_slot = -0x7b88, salt_slot = -0x7b90, start_slot = -0x7b8c;
// What the checker puts in r20 when a case gives none: its low byte (0x14) is above the jump table's 10 (the default arm).
constexpr std::uint32_t default_r20 = 0x5a000000u + 20 * 0x10101u;

struct Ring {
    std::uint32_t cursor_cell, end_cell, salt_cell, start, end;
    explicit Ring(const Scenario& s)
        : cursor_cell(s.toc(cursor_slot)), end_cell(s.toc(end_slot)), salt_cell(s.toc(salt_slot)), start(s.toc(start_slot)), end(s.r32(end_cell)) {}
    // Every draw refills: the end cell is the start, the sample under the cursor `sample`.
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
    std::vector<std::uint32_t> staff_club, staff_free, staff_nation;
    explicit World(const Scenario& scenario) : s(scenario) {
        for (std::uint32_t n = 0; n + 16 < s.people(); ++n) {
            const auto p = s.person(n);
            if (s.r32(p + 0x14) != 0 || s.r32(p + 0x1c) == 0 || s.r32(p + 0x0c) == 0) continue;  // a non-player with a staff record and a nation
            if (s.r32(p + 0x24) != 0) staff_club.push_back(p);
            else if (s.r32(p + 0x20) != 0) staff_nation.push_back(p);
            else staff_free.push_back(p);
        }
    }
    static std::uint32_t at(const std::vector<std::uint32_t>& list, std::size_t n) { return list[(n * 2654435761u + 17) % list.size()]; }
};

std::string number(std::int64_t n) { return std::to_string(n); }
// A case that carries a poke on a callee every case calls (0x1014b690, the human managers' hook) is traced by the checker, so its refills' return
// addresses are paired and not only those of the first 32 cases. The cases of the families that position the pool (every draw refilling, a refill at each
// draw, the forced branches, the contract block) are traced: together they pair all 61 sites; the sweeps are not (a traced case costs 1-2 s, an untraced
// one milliseconds), and the pool's whole state is compared after every case either way, so a missing, extra or reordered draw fails them too.
std::vector<Case::Poke> traced() { return {{trace_callee, {}}}; }
Case make(const std::string& label, std::uint32_t person, std::vector<Write> memory, std::uint32_t r20 = default_r20, bool trace = false) {
    return {.entry = staff_entry, .registers = {{4, person}, {20, r20}}, .memory = std::move(memory), .label = "newgen_staff: " + label,
            .pokes = trace ? traced() : std::vector<Case::Poke>{}};
}
}

// The first cases: the managers' four arms with every draw refilling (one case pairs all the sites of the arm), the ages by every job, then
// a refill at each draw position in turn.
PLAYER_REGEN_CASES("newgen_staff") {
    const World w(scenario);
    const Ring ring(scenario);
    if (w.staff_club.size() < 50 || w.staff_free.size() < 50)
        throw std::runtime_error("newgen_staff: the career has too few staff of a kind");
    struct Arm { const char* name; std::int32_t reputation; std::uint32_t flag; };
    const std::array<Arm, 4> arms{{{"A", 7000, 1}, {"B", 7000, 0}, {"C", 5000, 1}, {"D", 1000, 0}}};
    std::size_t n = 0;
    for (const auto& arm : arms)
        for (std::uint32_t variant = 0; variant < 6; ++variant) {
            const auto person = World::at(w.staff_club, ++n);
            const auto club = scenario.r32(person + 0x24);
            auto memory = ring.every_draw(variant * 7 + 1 + (variant % 2) * 4999);
            append(memory, {w8(person + 0x62, 1), w16(club + 0x1ec, std::uint32_t(arm.reputation)), w8(club + 0x241, arm.flag),
                            w16(scenario.r32(person + 0x1c) + 0x26, 1000 + variant)});
            cases.push_back(make(std::string("manager arm ") + arm.name + " every draw refills, sample " + number(variant), person, memory, default_r20, true));
        }
    for (std::uint32_t job = 0; job < 14; ++job) {
        const std::uint32_t value = job < 11 ? job : std::array<std::uint32_t, 3>{0x80, 0xff, 0x10b}[job - 11];
        const auto person = World::at(w.staff_free, job + 100);
        auto memory = ring.every_draw(300 + job);
        append(memory, {w32(person + 0x24, 0), w32(person + 0x20, 0), w16(scenario.r32(person + 0x1c) + 0x26, 500)});
        cases.push_back(make("job from r20 " + number(value) + ", every draw refills", person, memory, value, true));
    }
    // A refill at each draw position (0..23) for a manager of each arm and for a free staff member of a job with an age arm.
    for (std::uint32_t before = 0; before < 24; ++before)
        for (const auto& arm : arms) {
            const auto person = World::at(w.staff_club, 1000 + before * 4 + n++);
            const auto club = scenario.r32(person + 0x24);
            std::vector<std::uint32_t> samples;
            for (unsigned k = 0; k < 30; ++k) samples.push_back((before * 977 + k * 1237 + 31) & 0xffff);
            auto memory = ring.scripted(before, 17 * before + 3, samples);
            append(memory, {w8(person + 0x62, 1), w16(club + 0x1ec, std::uint32_t(arm.reputation)), w8(club + 0x241, arm.flag),
                            w16(scenario.r32(person + 0x1c) + 0x26, 2000 + before)});
            cases.push_back(make(std::string("manager arm ") + arm.name + " refill at draw " + number(before), person, memory, default_r20, true));
        }
    for (std::uint32_t before = 0; before < 8; ++before)
        for (const std::uint32_t job : {0u, 2u, 4u, 6u, 7u, 8u, 10u, 5u}) {
            const auto person = World::at(w.staff_free, 2000 + before * 8 + job);
            std::vector<std::uint32_t> samples;
            for (unsigned k = 0; k < 12; ++k) samples.push_back((before * 313 + k * 4001 + job * 17) & 0xffff);
            auto memory = ring.scripted(before, 5 * before, samples);
            append(memory, {w32(person + 0x24, 0), w32(person + 0x20, 0), w16(scenario.r32(person + 0x1c) + 0x26, std::uint32_t(before == 3 ? 0 : 900 + before))});
            cases.push_back(make("job " + number(job) + " free, refill at draw " + number(before), person, memory, job, true));
        }
}

PLAYER_REGEN_CASES("newgen_staff") {
    // The refills whose site is reached only by a forced outcome: the zero branches of the tested attributes (the test draw's sample is chosen
    // for a remainder of 0 and the next draw refills: arm C's +0x3d and +0x40 at draws 11/12 and 14/15, arm D's +0x3d and +0x40 at 11/12 and
    // 15/16), and the contract block's two draws (the contract test answered 1, a refill at each draw position of a manager of every arm and
    // of club persons of other jobs: the draws of the day below 28 and the year below 6 are the last of the path).
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(777);
    std::size_t n = 0;
    for (const bool arm_d : {false, true})
        for (const std::uint32_t test : {0u, 1u})
            for (const bool second_test : {false, true}) {
                const auto person = World::at(w.staff_club, 20000 + ++n);
                const auto club = scenario.r32(person + 0x24);
                const std::uint32_t test_at = second_test ? (arm_d ? 15u : 14u) : 11u, modulus = second_test ? (arm_d ? 100u : 10u) : (arm_d ? 10u : 5u);
                std::vector<std::uint32_t> samples;
                for (unsigned k = 0; k < 30; ++k) samples.push_back(random() & 0xffff);
                samples[test_at] = test == 0 ? modulus * (1 + random() % 500) : modulus * (1 + random() % 500) + 1;
                auto memory = ring.scripted(test_at + 1, 0, samples);  // the draw after the test refills
                append(memory, {w8(person + 0x62, 1), w16(club + 0x1ec, arm_d ? 1000u : 5000u), w8(club + 0x241, 1), w16(scenario.r32(person + 0x1c) + 0x26, 800)});
                cases.push_back(make(std::string("manager arm ") + (arm_d ? "D" : "C") + " refill after the test " + number(second_test ? 2 : 1) + " outcome " + number(test),
                                     person, memory, default_r20, true));
            }
    for (std::uint32_t before = 0; before < 28; ++before)
        for (std::uint32_t variant = 0; variant < 6; ++variant) {
            const auto person = World::at(w.staff_club, 21000 + ++n);
            const auto club = scenario.r32(person + 0x24);
            std::vector<std::uint32_t> samples;
            for (unsigned k = 0; k < 30; ++k) samples.push_back(random() & 0xffff);
            auto memory = ring.scripted(before, random() & 0xffff, samples);
            append(memory, {w16(scenario.r32(person + 0x1c) + 0x26, 700)});
            if (variant < 4) append(memory, {w8(person + 0x62, 1), w16(club + 0x1ec, variant == 0 ? 7000u : variant == 1 ? 7000u : variant == 2 ? 5000u : 1000u), w8(club + 0x241, variant == 0)});
            else append(memory, {w8(person + 0x62, variant == 4 ? 4u : 0u)});
            auto item = make("contract draws, refill at draw " + number(before) + " variant " + number(variant), person, memory, default_r20, true);
            item.answers = {{contract_test, 1}};
            cases.push_back(item);
        }
}

PLAYER_REGEN_CASES("newgen_staff") {
    // The contract applied: the contract block's last step (0x103727c0 with the person's contract record) runs only when the contract manager's lookup
    // (0x10082f20) still finds a record after the block cleared the person's contract (0x10084a80), which a real person's contract never does (the lookup
    // then gives 0). With the clearing answered (skipped), the lookup finds the person's real record and the step runs on it.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(9090);
    for (unsigned n = 0; n < 40; ++n) {
        const auto person = World::at(w.staff_club, 40000 + n);
        std::vector<std::uint32_t> samples;
        for (unsigned k = 0; k < 30; ++k) samples.push_back(random() & 0xffff);
        auto memory = n % 4 == 0 ? ring.every_draw(random() & 0xffff) : ring.scripted(n % 4 == 1 ? 40 : random() % 12, random() & 0xffff, samples);
        append(memory, {w8(person + 0x62, n % 3 == 0 ? 1u : n % 3 == 1 ? 4u : std::uint32_t(n))});
        auto item = make("contract applied " + number(n), person, memory, default_r20, true);
        item.answers = {{contract_test, 1}, {contract_clear, 0}};
        cases.push_back(item);
    }
}

PLAYER_REGEN_CASES("newgen_staff") {
    // The manager's sub-branches: the test draws forced to a remainder of 0 or not. Arm C tests at draws 11 (+0x3d, below 5) and 14 (+0x40,
    // below 10), arm D at 11 (below 10) and 15 (below 100); the draws are numbered from 0 with the bound draw (+0x26 nonzero) first.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(5150);
    std::size_t n = 0;
    for (const bool arm_d : {false, true})
        for (const std::uint32_t first : {0u, 1u})
            for (const std::uint32_t second : {0u, 1u})
                for (std::uint32_t salt : {0u, 17u}) {
                    const auto person = World::at(w.staff_club, 3000 + ++n);
                    const auto club = scenario.r32(person + 0x24);
                    std::vector<std::uint32_t> samples;
                    for (unsigned k = 0; k < 24; ++k) samples.push_back(random() & 0xffff);
                    const auto first_at = 11u, second_at = arm_d ? 15u : 14u;
                    // With the salt added, the remainder is (sample + salt) % modulus: the sample is chosen for it.
                    const std::uint32_t first_modulus = arm_d ? 10 : 5, second_modulus = arm_d ? 100 : 10;
                    const auto pick = [&](std::uint32_t outcome, std::uint32_t modulus) {
                        const auto base = modulus * (1 + random() % 500);
                        return (outcome == 0 ? base - salt % modulus : base + 1 - salt % modulus) & 0xffff;
                    };
                    samples[first_at] = pick(first, first_modulus);
                    samples[second_at] = pick(second, second_modulus);
                    auto memory = ring.scripted(40, salt, samples);
                    append(memory, {w8(person + 0x62, 1), w16(club + 0x1ec, arm_d ? 1000u : 5000u), w8(club + 0x241, 1),
                                    w16(scenario.r32(person + 0x1c) + 0x26, 800)});
                    cases.push_back(make(std::string("manager arm ") + (arm_d ? "D" : "C") + " tests " + number(first) + number(second) + " salt " + number(salt),
                                         person, memory));
                }
    // The same arms' draws sweeping every sample the sub-branches can take (the modulus tests at 5, 10 and 100): samples 0..119 for the
    // first test and 0..109 for the second.
    for (const bool arm_d : {false, true})
        for (std::uint32_t sample = 0; sample < 120; ++sample) {
            const auto person = World::at(w.staff_club, 4000 + sample + (arm_d ? 500 : 0));
            const auto club = scenario.r32(person + 0x24);
            std::vector<std::uint32_t> samples;
            for (unsigned k = 0; k < 24; ++k) samples.push_back(random() & 0xffff);
            samples[11] = sample;
            samples[arm_d ? 15 : 14] = 119 - sample;
            auto memory = ring.scripted(40, 0, samples);
            append(memory, {w8(person + 0x62, 1), w16(club + 0x1ec, arm_d ? 3250u : 3251u), w8(club + 0x241, 0), w16(scenario.r32(person + 0x1c) + 0x26, 800)});
            cases.push_back(make(std::string("manager arm ") + (arm_d ? "D" : "C") + " sample " + number(sample), person, memory));
        }
}

PLAYER_REGEN_CASES("newgen_staff") {
    // Field sweeps. The club's reputation across both thresholds and its flag byte; the job bytes and r20 over every byte value; the
    // staff record's +0x26 (the first draw's bound) and +0x24; the statistics' values and the threshold byte +0x61; the date.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(8128);
    const auto samples_of = [&](unsigned count) {
        std::vector<std::uint32_t> samples;
        for (unsigned k = 0; k < count; ++k) samples.push_back(random() & 0xffff);
        return samples;
    };
    std::size_t n = 0;
    for (const std::int32_t reputation : {-32768, -1, 0, 1, 3249, 3250, 3251, 6249, 6250, 6251, 9999, 32767})
        for (const std::uint32_t flag : {0u, 1u, 0x80u, 0xffu, 0x100u}) {
            const auto person = World::at(w.staff_club, 5000 + ++n);
            const auto club = scenario.r32(person + 0x24);
            auto memory = ring.scripted(random() % 6 + 30, random() & 0xffff, samples_of(30));
            append(memory, {w8(person + 0x62, 1), w16(club + 0x1ec, std::uint32_t(reputation)), w8(club + 0x241, flag)});
            cases.push_back(make("reputation " + number(reputation) + " flag " + number(flag), person, memory));
        }
    // The job bytes: the club side over every value, then the national side (no club), then r20 (neither).
    for (std::uint32_t job = 0; job < 256; ++job) {
        const auto club_person = World::at(w.staff_club, 6000 + job);
        auto memory = ring.scripted(40, random() & 0xffff, samples_of(30));
        append(memory, {w8(club_person + 0x62, job), w8(club_person + 0x60, 255 - job), w16(scenario.r32(club_person + 0x1c) + 0x26, 700 + job)});
        const auto club = scenario.r32(club_person + 0x24);
        append(memory, {w16(club + 0x1ec, 1000 + job * 30), w8(club + 0x241, job & 1)});
        cases.push_back(make("club job byte " + number(job), club_person, memory));
        const auto nation_person = World::at(w.staff_nation.empty() ? w.staff_free : w.staff_nation, 7000 + job);
        auto nation_memory = ring.scripted(40, random() & 0xffff, samples_of(30));
        append(nation_memory, {w32(nation_person + 0x24, 0), w8(nation_person + 0x60, job), w8(nation_person + 0x62, 255 - job)});
        if (w.staff_nation.empty()) append(nation_memory, {w32(nation_person + 0x20, scenario.r32(nation_person + 0x0c))});
        cases.push_back(make("national job byte " + number(job), nation_person, nation_memory, 255 - job));
        const auto free_person = World::at(w.staff_free, 8000 + job);
        auto free_memory = ring.scripted(40, random() & 0xffff, samples_of(30));
        append(free_memory, {w32(free_person + 0x24, 0), w32(free_person + 0x20, 0), w8(free_person + 0x60, 255 - job), w8(free_person + 0x62, job)});
        cases.push_back(make("r20 " + number(job), free_person, free_memory, job | (job % 3 == 0 ? 0x5a000000u : job % 3 == 1 ? 0xffffff00u : 0x100u)));
    }
    // The bound of the first draw: trunc(0.75 * the halfword +0x26), 0 (no draw) for -1, 0, 1, and the comparison with half of it.
    const std::array<std::int32_t, 28> potentials{-32768, -32767, -30000, -5000, -100, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 99, 100, 101, 1000, 10000, 20000, 21845,
                                                  21846, 30000, 32766, 32767, 32768};
    for (const auto potential : potentials)
        for (unsigned variant = 0; variant < 4; ++variant) {
            const auto person = World::at(w.staff_free, 9000 + ++n);
            auto memory = ring.scripted(variant == 3 ? 0 : 40, random() & 0xffff, samples_of(30));
            const auto record = scenario.r32(person + 0x1c);
            append(memory, {w16(record + 0x26, std::uint32_t(potential)), w16(record + 0x24, random() & 0xffff), w32(person + 0x24, 0), w32(person + 0x20, 0)});
            auto item = make("potential " + number(potential) + " variant " + number(variant), person, memory, std::uint32_t(variant));
            item.clear_fpscr = variant % 2 == 1;  // the snapshot's FPSCR has the sticky inexact bit set, which hides fctiwz's inexact result
            cases.push_back(item);
        }
    // The drawn ability halfwords against their bounds: the staff record's +0x28 and +0x2c compared for the swap; every sample class of the
    // 750, 200 and 50 draws.
    for (unsigned k = 0; k < 300; ++k) {
        const auto person = World::at(w.staff_free, 10000 + k);
        auto samples = samples_of(30);
        samples[1] = k * 3 % 750 + (random() % 2) * 750 * (random() % 80);
        samples[2] = (k * 7) % 200 + 200 * (random() % 300);
        samples[0] = (k * 11) % 9000;
        auto memory = ring.scripted(40, 0, samples);
        append(memory, {w16(scenario.r32(person + 0x1c) + 0x26, 200 + (k % 5) * 3), w32(person + 0x24, 0), w32(person + 0x20, 0)});
        cases.push_back(make("ability swap " + number(k), person, memory, k % 11));
    }
    // The statistics: the threshold byte +0x61 (every statistic whose value reaches 200 - 10 * the byte is raised by (excess / 75), at most 10) over
    // every signed value of interest; the statistics themselves are the person's real ones (answering 0x103094b0 with a constant makes the level
    // routine 0x10309680 loop until its budget ends on both sides: a timed-out case compares nothing). A byte of 127 gives the threshold -1070, so
    // the excess of any real value is above 825 and the cap of 10 shows.
    for (const std::int32_t byte : {-128, -100, -80, -20, -2, -1, 0, 1, 2, 5, 10, 12, 15, 19, 20, 21, 30, 50, 100, 126, 127})
        for (unsigned variant = 0; variant < 6; ++variant) {
            const auto person = World::at(w.staff_free, 11000 + ++n);
            auto memory = ring.scripted(40, 0, samples_of(30));
            append(memory, {w8(person + 0x61, std::uint32_t(byte)), w32(person + 0x24, 0), w32(person + 0x20, 0)});
            cases.push_back(make("statistic threshold byte " + number(byte) + " person " + number(variant), person, memory, std::uint32_t(n)));
        }
    // The date (a halfword day and an unsigned halfword year: the birth year is the year minus the age, the contract's year the year plus
    // draws) and the contract test's answers (a nonzero byte runs the contract block, 0x100 is zero as a signed byte) with the holder's
    // lookup real or answered 0.
    const auto date = scenario.toc(date_slot);
    for (const std::uint32_t year : {0u, 1u, 1999u, 2001u, 2003u, 65530u, 65535u})
        for (const std::uint32_t day : {0u, 1u, 200u, 364u, 365u, 366u, 0x7fffu, 0x8000u, 0xffffu}) {
            const auto person = World::at(w.staff_club, 12000 + ++n);
            auto memory = ring.scripted(random() % 8 + 30, random() & 0xffff, samples_of(30));
            append(memory, {w16(date, day), w16(date + 2, year), w8(person + 0x62, 1)});
            cases.push_back(make("date " + number(day) + "/" + number(year), person, memory));
        }
    for (const std::uint32_t answer : {0u, 1u, 0x80u, 0xffu, 0x100u, 0x101u, 0xffffff00u, 0x12345600u})
        for (const bool holder_none : {false, true})
            for (unsigned variant = 0; variant < 12; ++variant) {
                const auto person = World::at(w.staff_club, 13000 + ++n);
                auto memory = ring.scripted(variant < 4 ? variant * 5 : 40, random() & 0xffff, samples_of(30));
                append(memory, {w8(person + 0x62, variant % 3 == 0 ? 1u : std::uint32_t(variant))});
                auto item = make("contract test answer " + number(answer) + (holder_none ? " no holder" : " real holder") + " variant " + number(variant), person, memory);
                item.answers = {{contract_test, answer}};
                if (holder_none) item.answers.push_back({contract_holder, 0});
                cases.push_back(item);
            }
}

PLAYER_REGEN_CASES("newgen_staff") {
    // Real staff unedited (their own jobs, clubs, reputations, nations, potentials), with the snapshot's pool position and with every draw
    // refilling; then a random sweep over everything above together.
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(424242);
    const auto samples_of = [&](unsigned count) {
        std::vector<std::uint32_t> samples;
        for (unsigned k = 0; k < count; ++k) samples.push_back(random() & 0xffff);
        return samples;
    };
    for (std::size_t n = 0; n < 150; ++n) {
        const auto& list = n % 3 == 0 ? w.staff_club : n % 3 == 1 ? w.staff_free : w.staff_nation.empty() ? w.staff_free : w.staff_nation;
        const auto person = World::at(list, n * 5 + 3);
        cases.push_back(make("real staff " + number(std::int64_t(n)), person, n % 2 ? ring.every_draw(std::uint32_t(random() & 0xffff)) : std::vector<Write>{}));
    }
    const std::array<std::uint32_t, 14> jobs{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0x80, 0xff};
    const std::array<std::int32_t, 9> reputations{-1, 0, 3250, 3251, 6250, 6251, 8000, 1500, 32767};
    const std::array<std::int32_t, 12> potentials{-30000, -100, -1, 0, 1, 3, 50, 100, 1000, 5000, 20000, 32767};
    const auto date = scenario.toc(date_slot);
    for (std::size_t n = 0; n < 1800; ++n) {
        const auto kind = n % 4;  // staff with a club, without, with a national link only, a manager
        const auto person = World::at(kind == 0 || kind == 3 ? w.staff_club : kind == 1 ? w.staff_free : w.staff_nation.empty() ? w.staff_free : w.staff_nation, n * 3 + 11);
        const auto record = scenario.r32(person + 0x1c);
        auto memory = n % 5 == 0 ? ring.every_draw(random() & 0xffff) : ring.scripted(random() % 26, random() & 0xffff, samples_of(30));
        append(memory, {w16(record + 0x26, std::uint32_t(potentials[random() % potentials.size()]))});
        if (kind == 1) append(memory, {w32(person + 0x24, 0), w32(person + 0x20, 0)});
        if (kind == 2 && w.staff_nation.empty()) append(memory, {w32(person + 0x24, 0), w32(person + 0x20, scenario.r32(person + 0x0c))});
        if (kind == 2 && !w.staff_nation.empty()) append(memory, {w32(person + 0x24, 0)});
        const auto club = scenario.r32(person + 0x24);
        if (kind == 3 && club != 0) append(memory, {w8(person + 0x62, 1), w16(club + 0x1ec, std::uint32_t(reputations[random() % reputations.size()])), w8(club + 0x241, random() % 3 == 0)});
        else if (random() % 3 == 0) append(memory, {w8(person + 0x62, jobs[random() % jobs.size()]), w8(person + 0x60, jobs[random() % jobs.size()])});
        if (random() % 7 == 0) append(memory, {w16(date, random() % 400), w16(date + 2, 1990 + random() % 30)});
        if (random() % 9 == 0) append(memory, {w8(person + 0x61, random() & 0xff)});
        auto item = make("random " + number(std::int64_t(n)) + " kind " + number(kind), person, memory, jobs[random() % jobs.size()] | (random() % 2 ? 0x5a000000u : 0u));
        item.clear_fpscr = n % 2 == 1;
        if (random() % 6 == 0) item.answers = {{contract_test, std::array<std::uint32_t, 4>{0, 1, 0x100, 7}[random() % 4]}};
        cases.push_back(item);
    }
}

PLAYER_REGEN_CASES("newgen_staff") {
    // A person with a null staff record: the original reads and writes the record's fields at addresses 0x24..0x43 (low memory), which the
    // game's address space does not map, so both sides fault at once unless the checker runs with --null-page (low memory mapped, zero), where they
    // run on: the tests of the record in the manager's and the contract block's entry are then reached (the person is a manager with a club,
    // another job with a club, or free). Without --null-page these cases are "both faulted".
    const World w(scenario);
    const Ring ring(scenario);
    std::mt19937 random(31337);
    for (std::uint32_t kind = 0; kind < 6; ++kind) {
        const auto person = World::at(w.staff_club, 30000 + kind);
        std::vector<std::uint32_t> samples;
        for (unsigned k = 0; k < 30; ++k) samples.push_back(random() & 0xffff);
        auto memory = ring.scripted(40, 0, samples);
        append(memory, {w32(person + 0x1c, 0), w8(person + 0x62, kind % 3 == 0 ? 1u : kind % 3 == 1 ? 4u : 0u)});
        if (kind >= 3) append(memory, {w32(person + 0x24, 0), w32(person + 0x20, 0)});
        auto item = make("null staff record kind " + number(kind), person, memory, 1);
        if (kind % 2 == 0) item.answers = {{contract_test, 1}};
        cases.push_back(item);
    }
}
}
