// PLAYER_REGEN.CPP, block `club_placement`: cases for the two club scores (0x102ea670, 0x102e9b90), the club choices (0x102e99f0 over all
// clubs, 0x102ea3c0 over a nation's club list), the per-nation lists (0x102e98a0) and the signing (0x102dc000).
//
// The career's real players, clubs, managers, contract records and nation lists are the starting point (season-day). The scores' many
// branches are driven by edits over real (player, club) pairs with a fixed pseudo-random generator: the player's halfwords (+0xa,
// +6, +4) across the thresholds 50, 80, 120 and the mod tests (101..120), the club's reputation across 2, 4250, 5750 and the
// signs, its kind byte (+0x240: 0, 0x14, negative bytes), the manager links and records, the nation words, the special club ids and
// nation keys (the TOC cells written to the club's own values), and the pool's position and samples (every draw refilling in turn).
// Human-club answers (0x1014c2e0) come from nth answers, so each call of a path takes either answer. Labels start with
// "club_placement:" (CM_PLAYER_REGEN_BLOCK=club_placement).
#include "player_regen_checks.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <random>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t rate_simple = 0x102ea670, rate_full = 0x102e9b90, best_destination = 0x102e99f0, best_of_list = 0x102ea3c0,
                        init_lists = 0x102e98a0, sign = 0x102dc000, human_club = 0x1014c2e0, next_in_list = 0x10738e00, members = 0x100c9070,
                        contract_record = 0x10082f20, allocate_block = 0x10775130;
constexpr std::int32_t cursor_slot = -0x7b84, end_slot = -0x7b88, salt_slot = -0x7b90, start_slot = -0x7b8c, regen_slot = -0x7b28,
                       club_table_slot = -0x7be8, club_count_slot = -0x7b50, nation_count_slot = -0x7b04, english_slot = -0x6678,
                       table_slot = -0x7b5c, holder_slot = -0x7984, date_slot = -0x7b80;
constexpr std::uint32_t area = scratch_base + 0x30000, list_object = area, key_row = area + 0x100;

struct Pool {
    std::uint32_t cursor_cell, end_cell, salt_cell, start, cursor, end;
    explicit Pool(const Scenario& s)
        : cursor_cell(s.toc(cursor_slot)), end_cell(s.toc(end_slot)), salt_cell(s.toc(salt_slot)), start(s.toc(start_slot)),
          cursor(s.r32(cursor_cell)), end(s.r32(end_cell)) {}
    std::vector<Write> at(std::uint32_t position, std::uint32_t salt, const std::vector<std::uint32_t>& samples = {}) const {
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
    std::vector<std::uint32_t> players;  // people with a player record, a club and a nation (indices into the people table)
    std::vector<std::uint32_t> clubs;    // clubs with a manager link (+0x30), a manager (+0x90) and a nation record (+0x2c)
    explicit World(const Scenario& scenario) : s(scenario) {
        for (std::uint32_t n = 0; n + 16 < s.people(); ++n) {
            const auto p = s.person(n);
            if (s.r32(p + 0x14) != 0 && s.r32(p + 0x24) != 0 && s.r32(p + 0xc) != 0) players.push_back(p);
        }
        const auto count = s.global(club_count_slot);
        for (std::uint32_t n = 0; n < count; ++n) {
            const auto c = s.global(club_table_slot) + n * 0x248;
            if (s.r32(c + 0x30) != 0 && s.r32(c + 0x90) != 0 && s.r32(c + 0x2c) != 0) clubs.push_back(c);
        }
    }
    std::uint32_t player(std::size_t n) const { return players[(n * 7919 + 13) % players.size()]; }
    std::uint32_t club(std::size_t n) const { return clubs[(n * 104729 + 7) % clubs.size()]; }
};

std::string numbered(const char* what, std::size_t n) { return std::string("club_placement: ") + what + " " + std::to_string(n); }
}

PLAYER_REGEN_CASES("club_placement") {
    // First: every refill site of the scores, traced (the draws each refill in turn on one path that takes all of them).
    const World w(scenario);
    const Pool pool(scenario);
    if (w.players.size() < 50 || w.clubs.size() < 50) throw std::runtime_error("club_placement: the career has too few players or clubs");
    const auto person = w.player(1);
    const auto club = w.club(1);
    const auto record = scenario.r32(person + 0x14);
    const auto manager = scenario.r32(club + 0x90);
    // The simple score: value 5000 + 100 for a nonzero bound; the refill of its only draw.
    for (std::uint32_t before = 0; before <= 1; ++before) {
        auto memory = pool.at(pool.end - 2 * before, 17, {5});
        append(memory, {w16(club + 0x1ec, 1000), w16(record + 0xa, 3000), w16(record + 6, 20), w16(record + 4, 20)});
        cases.push_back({.entry = rate_simple, .registers = {{3, 0}, {4, person}, {5, club}}, .memory = memory,
                         .label = "club_placement: traced refill of the simple score, position " + std::to_string(before)});
    }
    // The full score along a path with all five draws: value_6 119 (> the mod-20 + 100), kind byte 5 (a draw below it and a second
    // mod-20 test), a manager with attributes (the mod-75 test) and no human club.
    for (std::uint32_t before = 0; before <= 6; ++before) {
        auto memory = pool.at(pool.end - 2 * before, 3, {1, 7, 2, 3, 4, 5});
        append(memory, {w16(record + 6, 119), w16(record + 0xa, 3000), w16(record + 4, 30), w16(club + 0x1ec, 3000), w8(club + 0x240, 5),
                        w32(club + 0x90, manager)});
        cases.push_back({.entry = rate_full, .registers = {{3, 0}, {4, person}, {5, club}, {6, 30}}, .memory = memory,
                         .nth_answers = {{human_club, 1, 0}, {human_club, 2, 0}, {human_club, 3, 0}},
                         .label = "club_placement: traced refill of the full score, draw " + std::to_string(before)});
    }
}

PLAYER_REGEN_CASES("club_placement") {
    // 0x102ea670: the simple score on real pairs, then the thresholds of its bound.
    const World w(scenario);
    const Pool pool(scenario);
    for (std::size_t n = 0; n < 80; ++n)
        cases.push_back({.entry = rate_simple, .registers = {{3, 0}, {4, w.player(n)}, {5, w.club(n * 3)}}, .label = numbered("simple score of a real pair", n)});
    const auto person = w.player(5);
    const auto club = w.club(5);
    const auto record = scenario.r32(person + 0x14);
    const std::array<std::int32_t, 9> halves{0, 1, -1, 5000, 4999, 32767, -32768, 25000, -25000};
    for (const auto rep : halves)
        for (const auto a : halves)
            for (const auto b : {0, 1, 100, -100, 327, -328, 32767, -32768}) {
                auto memory = pool.at(pool.cursor + 0x800 < pool.end ? pool.cursor : pool.start + 0x200, 100, {12345});
                append(memory, {w16(club + 0x1ec, std::uint32_t(rep)), w16(record + 0xa, std::uint32_t(a)), w16(record + 6, std::uint32_t(b)), w16(record + 4, std::uint32_t(b / 2))});
                cases.push_back({.entry = rate_simple, .registers = {{3, 0}, {4, person}, {5, club}}, .memory = memory,
                                 .label = "club_placement: simple score rep " + std::to_string(rep) + " a " + std::to_string(a) + " b " + std::to_string(b)});
            }
    // A bound of exactly 0 (no draw): the value 5000 with a zero reputation and zero halfwords.
    cases.push_back({.entry = rate_simple, .registers = {{3, 0}, {4, person}, {5, club}},
                     .memory = {w16(club + 0x1ec, 0), w16(record + 0xa, 5000), w16(record + 6, 0), w16(record + 4, 0)}, .label = "club_placement: simple score zero bound"});
}

PLAYER_REGEN_CASES("club_placement") {
    // 0x102e9b90: the full score on real pairs, then randomised edits over its branches.
    const World w(scenario);
    const Pool pool(scenario);
    for (std::size_t n = 0; n < 150; ++n)
        cases.push_back({.entry = rate_full, .registers = {{3, 0}, {4, w.player(n)}, {5, w.club(n * 5)}, {6, std::uint32_t(30 + n % 20)}},
                         .label = numbered("full score of a real pair", n)});
    std::mt19937 random(20261121);
    const auto pick = [&](auto&& values) { return values[random() % values.size()]; };
    const std::array<std::int32_t, 14> reps{0, 1, 2, 3, 4250, 4251, 4249, 5750, 5751, 5749, 1000, 30000, -1, -32768};
    const std::array<std::int32_t, 15> values_6{0, 1, 50, 51, 80, 81, 100, 101, 110, 119, 120, 121, 125, 127, 200};
    const std::array<std::uint32_t, 10> kinds{0, 0x14, 1, 5, 0x13, 0x15, 0x7f, 0x80, 0xff, 0xec};
    const std::array<std::uint32_t, 10> levels{0, 34, 35, 39, 40, 44, 45, 50, 0x7f, 0x80};
    const auto people = scenario.people();
    const auto table = scenario.global(table_slot);
    const auto holder = scenario.global(holder_slot);
    std::size_t made = 0;
    for (std::size_t n = 0; n < 900; ++n) {
        const auto person = w.player(n * 11 + 3), club = w.club(n * 7 + 1);
        const auto record = scenario.r32(person + 0x14), manager = scenario.r32(club + 0x90), staff = manager ? scenario.r32(manager + 0x1c) : 0;
        const auto link = scenario.r32(club + 0x30);
        std::vector<Write> memory;
        std::string what = "random edit " + std::to_string(n);
        // The player's halfwords and the club's reputation and kind.
        append(memory, {w16(record + 6, std::uint32_t(pick(values_6))), w16(record + 0xa, std::uint32_t(pick(reps))), w16(record + 4, std::uint32_t(pick(values_6))),
                        w16(club + 0x1ec, std::uint32_t(pick(reps))), w8(club + 0x240, pick(kinds))});
        // The manager link and record: absent, present without the staff link (+0x1c), present with it.
        switch (random() % 4) {
            case 0: append(memory, {w32(club + 0x90, 0)}); break;
            case 1: append(memory, {w32(manager + 0x1c, 0)}); break;
            case 2: append(memory, {w32(club + 0x30, 0)}); break;
            default: break;
        }
        if (staff) append(memory, {w8(staff + 0x42, std::uint32_t(random() % 40)), w8(staff + 0x38, std::uint32_t(random() % 20))});
        if (link && random() % 3 == 0) append(memory, {w32(table + scenario.r32(link) * 4, random() % 2)});
        // The human-club marker table row of the club (+4: negative, zero or positive).
        if (holder && random() % 2) append(memory, {w8(holder + scenario.r32(club) * 8 + 4, pick(std::array<std::uint32_t, 3>{0, 0x80, 5}))});
        // Nations and special clubs.
        const auto nation = scenario.r32(person + 0xc), club_nation = scenario.r32(club + 0x2c);
        // (A person with no nation reads the word at address 0x68: kept apart, with none of the special ids or keys written.)
        if (random() % 12 == 0) {
            append(memory, {w32(person + 0xc, 0)});
        } else {
            if (random() % 3 == 0) append(memory, {w32(person + 0xc, club_nation)});
            const std::array<std::int32_t, 3> id_slots{-0x7abc, -0x7ac0, -0x7ac4};
            if (random() % 3 == 0) append(memory, {w32(scenario.toc(pick(id_slots)), scenario.r32(club))});
            const std::array<std::int32_t, 4> key_slots{-0x757c, -0x7a10, -0x7a1c, -0x7574};
            if (random() % 2 && nation && club_nation) {
                append(memory, {w32(scenario.toc(pick(key_slots)), scenario.r32(club_nation + 0x68))});
                if (random() % 2) append(memory, {w32(scenario.toc(pick(key_slots)), scenario.r32(nation + 0x68))});
            }
        }
        // The pool: a refill at a random draw, random samples and salt.
        append(memory, pool.at(pool.end - 2 * (random() % 8), random() % 65536, {std::uint32_t(random() % 65536), std::uint32_t(random() % 65536), std::uint32_t(random() % 65536),
                                                                                  std::uint32_t(random() % 65536), std::uint32_t(random() % 65536)}));
        std::vector<Case::NthAnswer> answers;
        for (unsigned call = 1; call <= 3; ++call) answers.push_back({human_club, call, std::uint32_t(random() % 2)});
        cases.push_back({.entry = rate_full, .registers = {{3, 0}, {4, person}, {5, club}, {6, pick(levels)}}, .memory = memory, .nth_answers = answers, .label = "club_placement: full score, " + what});
        ++made;
    }
    (void)people;
    std::fprintf(stderr, "club_placement: %zu players, %zu clubs with a manager; %zu edited cases of the full score\n", w.players.size(), w.clubs.size(), made);
}

PLAYER_REGEN_CASES("club_placement") {
    // 0x102e99f0: the choice over all clubs (the scan's length limited through the club count word), with the special ids and
    // nations, the gate's and the member count's answers per club, and the pool refilling in the scores.
    const World w(scenario);
    const Pool pool(scenario);
    const auto nations = scenario.global(nation_count_slot);
    const auto first_club = scenario.global(club_table_slot);
    std::mt19937 random(7);
    for (std::size_t n = 0; n < 24; ++n) {
        const auto person = w.player(n * 13 + 5);
        for (const std::uint32_t extra : {0u, 1u, 3u, 12u, 60u}) {
            if (extra > 12 && n % 4 != 0) continue;
            std::vector<Write> memory{w32(scenario.toc(club_count_slot), nations * 2 + extra)};
            std::vector<Case::NthAnswer> answers;
            std::string what = "extra " + std::to_string(extra);
            const auto variant = (n + extra) % 6;
            if (variant == 1) {  // English-exclusion: the second nation record's key is the English one
                append(memory, {w32(person + 0x10, scenario.r32(person + 0xc)), w32(scenario.toc(english_slot), scenario.r32(scenario.r32(person + 0xc) + 0x68))});
                what += ", English nation";
            }
            if (variant == 2 || variant == 3) {  // ... and one of the five named clubs among the first ones
                append(memory, {w32(person + 0x10, scenario.r32(person + 0xc)), w32(scenario.toc(english_slot), scenario.r32(scenario.r32(person + 0xc) + 0x68))});
                const std::array<std::int32_t, 5> slots{-0x75cc, -0x75d0, -0x7724, -0x7864, -0x773c};
                for (std::size_t k = 0; k < 5; ++k)
                    append(memory, {w32(scenario.toc(slots[k]), scenario.r32(first_club + std::uint32_t((k + n) % (extra + 1)) * 0x248))});
                what += ", named clubs";
            }
            if (variant == 4) append(memory, {w32(person + 0x10, 0)});
            if (variant == 5) {  // the gate refuses some clubs and the member count of others is 50 or more
                for (unsigned call = 1; call <= 6; ++call) answers.push_back({0x100cab10, call, call % 3 == 0});
                for (unsigned call = 1; call <= 6; ++call) answers.push_back({members, call, call % 2 ? 0x32u : 0x10u});
                what += ", answered gates";
            }
            append(memory, pool.at(pool.end - 2 * (random() % 6), random() % 65536, {std::uint32_t(random() % 65536), std::uint32_t(random() % 65536), std::uint32_t(random() % 65536)}));
            cases.push_back({.entry = best_destination, .registers = {{3, list_object}, {4, person}}, .memory = memory, .nth_answers = answers,
                             .label = "club_placement: best destination of person " + std::to_string(n) + ", " + what});
        }
    }
    // The club code answers a club with the found flag 0 (the flag byte is in the routine's frame, poked before each answer), and the
    // transfer manager's gate (0x103f83c0) refuses some clubs.
    {
        const auto frame_byte = scenario.sp() - 0x80 + 0x38;
        const auto person = w.player(31);
        for (const std::uint32_t flag : {0u})
            cases.push_back({.entry = best_destination, .registers = {{3, list_object}, {4, person}}, .memory = {w32(scenario.toc(club_count_slot), nations * 2 + 8)},
                             .answers = {{0x100ce320, 1}, {0x100cab10, 0}}, .pokes = {{0x100ce320, {w8(frame_byte, flag)}}},
                             .label = "club_placement: best destination, club code found flag " + std::to_string(flag)});
        std::vector<Case::NthAnswer> refused;
        for (unsigned call = 1; call <= 8; ++call) refused.push_back({0x103f83c0, call, call % 3 == 1 ? 0u : 1u});
        cases.push_back({.entry = best_destination, .registers = {{3, list_object}, {4, person}}, .memory = {w32(scenario.toc(club_count_slot), nations * 2 + 8)},
                         .nth_answers = refused, .answers = {{0x100cab10, 0}}, .label = "club_placement: best destination, the transfer gate refuses some clubs"});
    }
    // The real, full scan once (the whole table).
    cases.push_back({.entry = best_destination, .registers = {{3, list_object}, {4, w.player(3)}}, .label = "club_placement: best destination, every club"});
}

PLAYER_REGEN_CASES("club_placement") {
    // 0x102ea3c0: the choice over a nation's club list (the regen object's: word +0 holds the per-nation list pointers).
    const World w(scenario);
    const Pool pool(scenario);
    const auto regen = scenario.global(regen_slot);
    const auto array = scenario.r32(regen);
    std::fprintf(stderr, "club_placement: the regen object %x, its nation lists at %x\n", regen, array);
    std::mt19937 random(11);
    const auto key_of = [&](std::uint32_t person) { return scenario.r32(scenario.r32(person + 0xc) + 0x68); };
    for (std::size_t n = 0; n < 60; ++n) {
        const auto person = w.player(n * 17 + 2);
        std::vector<Write> memory = pool.at(pool.end - 2 * (random() % 3), random() % 65536, {std::uint32_t(random() % 65536), std::uint32_t(random() % 65536)});
        std::string what = "person " + std::to_string(n);
        std::vector<Case::NthAnswer> answers;
        switch (n % 8) {
            case 1: append(memory, {w32(scenario.toc(-0x757c), key_of(person))}); what += ", home nation (first)"; break;
            case 2: append(memory, {w32(scenario.toc(-0x7574), key_of(person))}); what += ", home nation (second)"; break;
            case 3: case 4: append(memory, {w32(scenario.toc(-0x7578), key_of(person))}); what += ", home nation by a draw"; break;
            case 5: append(memory, {w32(person + 0x10, scenario.r32(person + 0xc)), w32(scenario.toc(english_slot), key_of(person))}); what += ", English nation"; break;
            case 6:
                for (unsigned call = 1; call <= 4; ++call) answers.push_back({members, call, call % 2 ? 0x32u : 0u});
                what += ", answered member counts";
                break;
            case 7:
                for (unsigned call = 1; call <= 4; ++call) answers.push_back({0x103f83c0, call, call % 2});
                what += ", answered gates";
                break;
            default: break;
        }
        cases.push_back({.entry = best_of_list, .registers = {{3, regen}, {4, person}}, .memory = memory, .nth_answers = answers, .label = "club_placement: best club of the list, " + what});
    }
    const auto person = w.player(9);
    const auto slot = array + key_of(person) * 4;
    const auto list = scenario.r32(slot);
    // No list, an empty list, and a missing club (the iterator answers null): the assertion 0x19e9.
    cases.push_back({.entry = best_of_list, .registers = {{3, regen}, {4, person}}, .memory = {w32(slot, 0)}, .label = "club_placement: best club, no list"});
    if (list) {
        cases.push_back({.entry = best_of_list, .registers = {{3, regen}, {4, person}}, .memory = {w16(list, 0)}, .label = "club_placement: best club, empty list"});
        cases.push_back({.entry = best_of_list, .registers = {{3, regen}, {4, person}}, .answers = {{next_in_list, 0}}, .label = "club_placement: best club, the iterator answers null"});
    }
}

PLAYER_REGEN_CASES("club_placement") {
    // 0x102e98a0: the per-nation club lists (into a scratch object, leaving the regen object's own lists alone).
    const World w(scenario);
    const auto clubs = scenario.global(club_count_slot);
    for (const std::uint32_t count : {clubs, clubs / 2, 40u, 0u})
        cases.push_back({.entry = init_lists, .registers = {{3, list_object}}, .memory = {w32(scenario.toc(club_count_slot), count)},
                         .label = "club_placement: init the nation lists, clubs " + std::to_string(count)});
    cases.push_back({.entry = init_lists, .registers = {{3, list_object}}, .memory = {w32(scenario.toc(nation_count_slot), 0), w32(scenario.toc(club_count_slot), 0)},
                     .label = "club_placement: init the nation lists, no nations and no clubs"});
    cases.push_back({.entry = init_lists, .registers = {{3, list_object}}, .memory = {w32(scenario.toc(club_count_slot), 200)}, .answers = {{0x10773520, 0}},
                     .label = "club_placement: init the nation lists, no object"});
    cases.push_back({.entry = init_lists, .registers = {{3, list_object}}, .answers = {{allocate_block, 0}}, .original_owners = {allocate_block},
                     .label = "club_placement: init the nation lists, the block allocation fails"});
}

PLAYER_REGEN_CASES("club_placement") {
    // 0x102dc000: signing a player to a club: the squad's free slot, the contract's value, the date draws, the human flags.
    const World w(scenario);
    const Pool pool(scenario);
    const auto date = scenario.toc(date_slot);
    const auto year = scenario.u16(date + 2);
    std::mt19937 random(13);
    for (std::size_t n = 0; n < 30; ++n) {
        const auto person = w.player(n * 19 + 4), club = w.club(n * 5 + 3);
        std::vector<Write> memory;
        std::vector<Case::NthAnswer> answers;
        std::vector<std::pair<std::uint32_t, std::uint32_t>> fixed;
        std::string what = "pair " + std::to_string(n);
        // The squad: the first free slot at 0, a few places in, the last, or none.
        const std::array<std::uint32_t, 5> free_at{0, 7, 49, 50, 25};
        const auto slot = free_at[n % 5];
        for (std::uint32_t k = 0; k < 50; ++k) memory.push_back(w32(club + 0x98 + k * 4, k == slot ? 0 : 0x1000 + k));
        what += ", free slot " + std::to_string(slot);
        const std::array<std::uint32_t, 8> ages{17, 18, 19, 20, 0xff, 0x80, 0x112, 35};
        append(memory, {w16(person + 0x5c, ages[(n / 5) % 8]), w8(club + 0x23d, n % 3 == 0 ? 3 : 2)});
        append(memory, pool.at(pool.end - 2 * (random() % 4), random() % 65536, {std::uint32_t(random() % 65536), std::uint32_t(random() % 65536), std::uint32_t(random() % 65536)}));
        // The answers: the contract value (the cap of 5000), the human club's, the contract record's absence.
        const std::array<std::uint32_t, 6> values{4999, 5000, 5001, 0x7fffffff, 100, 0xffffffffu};
        answers.push_back({0x10084be0, 1, values[n % 6]});
        for (unsigned call = 1; call <= 2; ++call) answers.push_back({human_club, call, std::uint32_t((n >> call) & 1)});
        if (n % 7 == 3) for (unsigned call = 1; call <= 3; ++call) answers.push_back({contract_record, call, 0});
        answers.push_back({0x104ae570, 1, std::uint32_t(n % 2)});
        cases.push_back({.entry = sign, .registers = {{3, 0}, {4, person}, {5, club}}, .memory = memory, .nth_answers = answers, .label = "club_placement: sign, " + what});
    }
    // Unanswered, the real callees run on a real pair (one with a free slot), and the record's expiry around the year + 5.
    for (std::size_t n = 0; n < 6; ++n) {
        const auto person = w.player(n * 23 + 8), club = w.club(n * 9 + 2);
        std::vector<Write> memory{w32(club + 0x98, 0), w16(person + 0x5c, std::uint32_t(18 + n))};
        cases.push_back({.entry = sign, .registers = {{3, 0}, {4, person}, {5, club}}, .memory = memory, .label = "club_placement: sign, real callees " + std::to_string(n)});
    }
    // The contract record (answered as a scratch row): the expiry word against the year + 5 and the human club's flags.
    for (const std::uint32_t expiry : {0u, year + 4, year + 5, year + 6, 0xffffu})
        for (const std::uint32_t human : {0u, 1u}) {
            const auto person = w.player(77), club = w.club(41);
            cases.push_back({.entry = sign, .registers = {{3, 0}, {4, person}, {5, club}},
                             .memory = {w32(club + 0x98, 0), w16(key_row + 0x32, expiry), w16(key_row + 0x30, 0x77), w8(key_row + 0x60, 0xab), w8(key_row + 0x38, 0xcd), w8(key_row + 0x40, 9)},
                             .answers = {{contract_record, key_row}, {0x10378d30, 0}, {0x1007ee50, 0}, {human_club, human}},
                             .label = "club_placement: sign, record expiry " + std::to_string(expiry) + " human " + std::to_string(human)});
        }
}
}
