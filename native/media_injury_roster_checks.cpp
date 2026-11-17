// Targeted checks of the intentional squad-injury fix. The original oracle is
// changed only at the two call arguments; injury/date bodies execute normally.
#include "media_checks.hpp"
#include "guest_abi.hpp"
#include "match_replay.hpp"
#include "media_adapter.hpp"
#include "routine_check.hpp"
#include "tactics_leaves.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unistd.h>

namespace cm::aot::media_checks {
namespace {
constexpr std::uint32_t injury_query = 0x10187680, story = 0x10649fb0, reaction = 0x106620b0;
constexpr std::uint32_t story_query = 0x1064c88c, reaction_query = 0x10667e50;
constexpr std::uint32_t contract = scratch_base + 0x1000, reporter = scratch_base + 0x2000;
constexpr std::uint32_t samples = scratch_base + 0x4000, trace_at = scratch_base + 0x8000;
constexpr std::array<unsigned, 6> occupied{0, 4, 9, 21, 38, 49};
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }

struct Fixture {
    std::uint32_t club = 0, player = 0, manager = 0, today = 0;
    std::array<std::uint32_t, 6> members{};
    unsigned count = 0;
    bool due = false;
};
const Fixture* active = nullptr;
bool corrected_reference = true;
bool corrupt_medical = false;
unsigned queries = 0;
bool count_seen = false, reaction_seen = false;

// Both branches execute real injury queries. Unrelated media services are
// answered so this fixture isolates the count and its four-injury threshold.
bool services(State& s) {
    guest::Cpu c(s);
    const auto back = std::uint32_t(s.r<4>(4128));
    if (s.pc == injury_query && (back == story_query || back == reaction_query)) {
        if (media_declining && corrected_reference) correct_injury_roster_reference(s);
        require(queries < active->members.size(), "Too many squad injury queries");
        if (!media_declining || corrected_reference)
            require(c.r(4) == active->members[queries], "Injury query did not follow occupied squad slots");
        ++queries;
        return false;
    }
    const bool direct = (back >= story && back < 0x1064cf80) || (back >= reaction && back < 0x1066b5f4);
    if (!direct) return false;
    // Only these unrelated services are answered. All other calls, including
    // ABI save/restore leaves and typed media helpers, execute normally. Local
    // return blocks are not service entries and must never be answered.
    std::uint32_t answer = 0;
    switch (s.pc) {
    case 0x10082f90: answer = c.r(4) == active->player ? contract : 0u; break;
    case 0x106487d0: answer = UINT32_MAX; break;
    case 0x10303560: c.set_f(1, 10.0); break;
    case 0x102cb4b0:
        if (back == 0x1064c8c8) {
            require(c.r(4) == 0x21 && c.r(5) == active->count, "News item has the wrong squad injury count");
            count_seen = true;
        }
        break;
    case 0x1037fa80:
        require(back == 0x1066801c && c.r(3) == contract && c.r(4) == 0x8000000 && c.r(5) == 0,
            "Wrong contract reaction");
        reaction_seen = true;
        break;
    case 0x100d71a0: case 0x100d8260: case 0x100d9520: case 0x100d9580:
    case 0x100db1b0: case 0x10117490: case 0x1014c2e0: case 0x102bdf90:
    case 0x10658370: case 0x106584d0: case 0x10677d60: case 0x1069e190:
    case 0x1069e700: case 0x1069eb10: case 0x106a95c0:
        break;
    default: return false;
    }
    c.set(3, answer);
    s.pc = back;
    return true;
}

struct RngWrites final : MemoryObserver {
    std::uint32_t cursor, seed;
    MemoryObserver* previous;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> bytes;
    RngWrites(std::uint32_t cursor_, std::uint32_t seed_, MemoryObserver* previous_)
        : cursor(cursor_), seed(seed_), previous(previous_) {}
    void mapped(std::uint32_t address, std::uint64_t size) override {
        if (previous) previous->mapped(address, size);
    }
    void cleared(std::uint32_t address, std::uint64_t size) override {
        if (previous) previous->cleared(address, size);
    }
    void read(std::uint32_t address, const std::uint8_t* data, unsigned size) override {
        if (previous) previous->read(address, data, size);
    }
    void written(std::uint32_t address, const std::uint8_t* data, std::size_t size) override {
        if (previous && previous->observes_write(address, unsigned(size))) previous->written(address, data, size);
        for (std::size_t i = 0; i < size; ++i)
            if ((address + i >= cursor && address + i < cursor + 4) ||
                (address + i >= seed && address + i < seed + 4))
                bytes.emplace_back(address + std::uint32_t(i), data[i]);
    }
};

void run(State& s, std::uint32_t entry, bool original) {
    queries = 0; count_seen = reaction_seen = false;
    // Decline dependency owners where supported. Injury/date services execute
    // real bodies on both sides; unrelated media services are answered above.
    RoutineChecker dependencies([](std::uint32_t) { return false; }, 0, 0);
    guest::Cpu c(s);
    RngWrites rng(c.toc(-0x7b84), c.toc(-0x7b90), s.memory.date_observer);
    struct Restore {
        State& s;
        MemoryObserver* observer;
        RoutineChecker* checker;
        bool (*probe)(State&);
        bool tactics_original;
        ~Restore() {
            s.memory.date_observer = observer;
            routine_checker = checker;
            dispatch_probe = probe;
            tactics_declining = tactics_original;
            alarm(0);
        }
    } restore{s, s.memory.date_observer, routine_checker, dispatch_probe, tactics_declining};
    // The older tactics inline hook does not consult routine_checker. Decline
    // it explicitly so both callers reach the same fixture service boundary.
    tactics_declining = true;
    routine_checker = &dependencies;
    dispatch_probe = services;
    s.memory.date_observer = &rng;
    s.budget = s.executed + 1'000'000;
    alarm(10);
    if (original) media_run_original(s, entry);
    else media_run_rewrite(s, entry);
    require(queries == active->members.size(), "Squad injury loop was not reached completely");
    if (entry == story) require(count_seen, "The news injury-count field was not emitted");
    else {
        require(reaction_seen == (active->count < 4), "Reaction did not use the four-injury threshold");
    }
    const auto rows = std::uint32_t(s.memory.read<4>(active->manager));
    for (const auto member : active->members) {
        const auto row = rows + std::uint32_t(s.memory.read<4>(member)) * 0x24;
        require(s.memory.read<4>(row) == s.memory.read<4>(active->today), "A roster member's pending injury update was skipped");
    }
    if (corrupt_medical && !original) {
        const auto row = rows + std::uint32_t(s.memory.read<4>(active->members[0])) * 0x24;
        s.memory.write<2>(row + 8, s.memory.read<2>(row + 8) ^ 1);
    }
    require(rng.bytes.size() < 3000, "RNG trace capacity exceeded");
    // compare_routine_call compares this complete ordered write transcript as
    // well as final guest memory; equal final seeds alone would be insufficient.
    s.memory.write<4>(trace_at, rng.bytes.size());
    for (std::size_t i = 0; i < rng.bytes.size(); ++i) {
        s.memory.write<4>(trace_at + 4 + 8 * i, rng.bytes[i].first);
        s.memory.write<4>(trace_at + 8 + 8 * i, rng.bytes[i].second);
    }
}
void original_run(State& s, std::uint32_t entry) { run(s, entry, true); }
void rewrite_run(State& s, std::uint32_t entry) { run(s, entry, false); }

Fixture prepare(State& s, unsigned injured, unsigned subject, bool due, std::uint32_t entry) {
    Scenario data{s};
    Fixture f;
    f.count = injured; f.due = due;
    f.manager = data.global(-0x7ba0); f.today = data.toc(-0x7b80);
    for (unsigned team = 0; team < 600 && !f.club; ++team) {
        const auto club = data.club(team);
        if (!data.r32(club + 0x2c) || !data.r32(club + 0x3c) || !data.r32(data.r32(club + 0x3c) + 0x10) || !data.r32(club + 0x90)) continue;
        unsigned found = 0;
        for (unsigned i = 0; i < 50 && found < f.members.size(); ++i) {
            const auto p = data.r32(club + 0x98 + i * 4);
            if (!p || !data.r32(p + 0x14) || !data.r32(p + 0xc) || !data.r32(p + 0x28) || !data.r32(p + 0x2c) || !data.r32(p + 0x30)) continue;
            if (std::int16_t(s.memory.read<2>(data.r32(p + 0x14) + 0xa)) < 3500) continue;
            f.members[found++] = p;
        }
        if (found == f.members.size()) f.club = club;
    }
    require(f.club != 0, "No six-player club fixture in the existing snapshot");
    f.player = f.members[subject];
    const auto day = std::uint32_t(s.memory.read<2>(f.today));
    require(day > 3, "Snapshot date too early for pending-update fixture");
    const auto rows = data.r32(f.manager), injuries = data.r32(f.manager + 4);
    require(data.r32(f.manager + 8) >= 6, "Snapshot has too few injury records");
    for (unsigned slot = 0; slot < 50; ++slot) s.memory.write<4>(f.club + 0x98 + slot * 4, 0);
    s.memory.write<4>(f.club + 0x80, 0);
    for (unsigned i = 0; i < f.members.size(); ++i) {
        const auto person = f.members[i];
        s.memory.write<4>(f.club + 0x98 + occupied[i] * 4, person);
        // No season-calendar branches are needed to exercise the real pending
        // status calculation. The media contract finder is answered above.
        s.memory.write<4>(person + 0x20, 0);
        s.memory.write<4>(person + 0x24, 0);
        s.memory.write<1>(person + 0x65, 0); // loyalty draw is deterministically zero
        s.memory.write<1>(person + 0x67, 10);
        const auto row = rows + data.r32(person) * 0x24;
        s.memory.copy_in(row, s.memory.copy_out(f.today, 8));
        s.memory.write<2>(row, day - (due ? 1 : 0));
        s.memory.write<2>(row + 8, 7000);
        s.memory.write<2>(row + 10, 7000);
        s.memory.write<2>(row + 12, 7000);
        s.memory.write<4>(row + 0x10, i < injured ? i : UINT32_MAX);
        const auto injury = injuries + i * 0x38;
        s.memory.copy_in(injury + 0x28, s.memory.copy_out(f.today, 8));
        s.memory.write<2>(injury + 0x28, day - 2);
        s.memory.write<2>(injury + 0x30, 30);
        s.memory.write<2>(injury + 0x32, 10);
    }
    s.memory.write<4>(contract + 0x50, entry == reaction ? 0x02000000 : 0);
    for (const auto offset : {0x44u, 0x4cu, 0x60u}) s.memory.write<1>(contract + offset, 0);
    s.memory.write<4>(data.toc(-0x7b40), 0); // no news subscribers in this fixture
    s.memory.write<4>(data.toc(-0x7b84), samples);
    s.memory.write<4>(data.toc(-0x7b88), samples + 2000);
    s.memory.write<4>(data.toc(-0x7b8c), samples);
    s.memory.write<4>(data.toc(-0x7b90), 0);
    guest::Cpu c(s);
    if (entry == story) {
        c.set(3, reporter); c.set(4, f.player); c.set(5, f.club); c.set(6, 0x13); c.set(7, 0); c.set(8, 0); c.set(30, contract);
    } else {
        c.set(3, reporter); c.set(4, f.club); c.set(5, 0x13); c.set(6, 0x100); c.set(8, 1); c.set(9, 0); c.set(10, f.player); c.set_f(1, 1.0);
    }
    c.set_lr(return_sentinel);
    return f;
}
}

void correct_injury_roster_reference(State& s) {
    if (!media_declining || s.pc != injury_query) return;
    guest::Cpu c(s);
    const auto back = std::uint32_t(s.r<4>(4128));
    const auto slot = back == story_query ? c.u8(c.sp() + 0x38) : back == reaction_query ? c.r(16) : 50;
    if (slot < 50) c.set(4, c.u32(c.r(24) + 0x98 + slot * 4));
}

int check_injury_roster(const std::string& snapshot) {
    unsigned passed = 0, controls = 0;
    auto state = std::make_unique<State>();
    load_match_snapshot(*state, snapshot);
    state->memory.map(scratch_base, scratch_size);
    // Comparisons run in forked children. The parent only holds fixture writes,
    // all of which prepare overwrites; load the 157 MiB snapshot once, not 40 times.
    for (const auto entry : {story, reaction}) for (const bool due : {false, true})
        for (const unsigned injured : {0u, 1u, 3u, 4u, 6u}) for (const unsigned subject : {0u, 5u}) {
            const auto started = std::chrono::steady_clock::now();
            std::cerr << "injury roster case " << passed + 1 << "/40 entry=" << std::hex << entry << std::dec
                      << " injured=" << injured << " subject=" << subject << " due=" << due << std::endl;
            const auto fixture = prepare(*state, injured, subject, due, entry);
            active = &fixture;
            corrected_reference = true;
            const auto errors = compare_routine_call(*state, entry, original_run, rewrite_run, RoutineChecker::result_r3, false, false);
            if (!errors.empty()) {
                std::cerr << "injury roster entry=" << std::hex << entry << std::dec << " injured=" << injured << " subject=" << subject << " due=" << due << '\n';
                for (const auto& error : errors) std::cerr << error << '\n';
                return 1;
            }
            ++passed;
            if (passed == 1) {
                corrupt_medical = true;
                require(!compare_routine_call(*state, entry, original_run, rewrite_run, RoutineChecker::result_r3, false, false).empty(),
                    "Chained observer missed a deliberately corrupted medical row");
                corrupt_medical = false;
            }
            const bool control = entry == story ? injured == 1 :
                (injured == 1 && subject == 0) || (injured == 4 && subject == 5);
            if (control && !due) {
                corrected_reference = false;
                const auto raw_errors = compare_routine_call(*state, entry, original_run, rewrite_run, RoutineChecker::result_r3, false, false);
                const auto expected_error = entry == story ? "News item has the wrong squad injury count" :
                    "Reaction did not use the four-injury threshold";
                require(std::ranges::any_of(raw_errors, [&](const auto& error) { return error.find(expected_error) != std::string::npos; }),
                    "Raw original did not expose the expected wrong-person outcome");
                ++controls;
            }
            std::cerr << "  passed in " << std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count() << "s\n";
        }
    active = nullptr;
    std::cout << "PASS media squad injuries: " << passed << " corrected-reference cases, " << controls
              << " raw-original controls, medical-observer corruption control; mixed/empty injury sets, both subjects, sparse roster, threshold, pending status and ordered RNG\n";
    return 0;
}
}
