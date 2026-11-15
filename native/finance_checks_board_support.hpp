#pragma once
// Support for the board block's checker cases: the club's fields the board's rules read, the records around it that
// the cases build in the scratch area, and scripts for the shared sample table (so that each draw's value is chosen).
#include "finance_checks.hpp"
#include <bit>
#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot::finance_checks::board {
struct Memory {
    std::vector<Write> writes;
    void put(std::uint32_t at, std::uint32_t value, unsigned size = 4) { writes.push_back({size, at, value}); }
    void put64(std::uint32_t at, std::uint64_t value) {
        put(at, std::uint32_t(value >> 32));
        put(at + 4, std::uint32_t(value));
    }
    void put_double(std::uint32_t at, double value) { put64(at, std::bit_cast<std::uint64_t>(value)); }
};

inline constexpr std::uint32_t S = scratch_base + 0x70000;
inline constexpr std::uint32_t division = S, owner_record = S + 0x100, person = S + 0x200, league = S + 0x300, board_record = S + 0x400,
                               league_table = S + 0x500, owner_source = S + 0x1000;
// The cells whose words the rules compare the division's kind with: index in `kind` below.
inline constexpr std::int32_t kind_slots[8] = {-0x757c, -0x7574, -0x7578, -0x7504, -0x750c, -0x7514, -0x7510, -0x7a14};

// One situation of a club (club index `club` of the career) for the board's routines.
struct Setup {
    unsigned club = 9;
    int band = 99;                  // the wealth band the answered 0x100f55b0 gives; 99: the real routine
    std::int32_t reputation = 3000;
    std::int64_t balance = 1'000'000'000;
    int kind = 99;                  // the division record's kind: the index of the cell it equals; 99: none; -1: no division
    int national = 0, works = 0, refused = 0, patience = 10;
    int owner = 1;                  // -1 no owner record, 0 a record without person, 1 with
    int owner_byte = 20;            // +0x63
    int b2f = 10, b36 = 10, b3d = 5, b40 = 10;
    int has_league = 1;
    std::int32_t power = 100;
    int table_entry = 1;            // the league table's row for the league
    int has_board = 1, board_flag = 1;
    std::int32_t funds = 5000, owed = 3000, held = 0;
    int stage = 0, counter = 60;
    std::int32_t owners = 5;        // the count of owners in the table of owners (global -0x7b04)
    std::vector<std::pair<std::uint32_t, double>> pool = {};  // overrides of the double pool
    std::vector<Write> extra = {};
};

inline Memory memory_of(const Scenario& sc, const Setup& s) {
    Memory m;
    const auto club = sc.club(s.club);
    const auto manager = sc.global(-0x7ab8);
    const auto account = sc.r32(manager) + s.club * 0x168;
    m.put(club + 0x1ec, std::uint32_t(s.reputation), 2);
    m.put64(account, std::uint64_t(s.balance));
    m.put(account + 0x165, std::uint32_t(s.stage), 1);
    m.put(account + 0x166, std::uint32_t(s.counter), 1);
    for (int k = 0; k < 8; ++k) m.put(sc.toc(kind_slots[k]), 1000u + unsigned(k));
    m.put(club + 0x2c, s.kind < 0 ? 0 : division);
    m.put(division + 0x68, s.kind >= 0 && s.kind < 8 ? 1000u + unsigned(s.kind) : 4242u);
    m.put(club + 0x23d, std::uint32_t(s.national), 1);
    m.put(club + 0x23e, std::uint32_t(s.works), 1);
    m.put(club + 0x240, std::uint32_t(s.patience), 1);
    m.put(club + 0x241, std::uint32_t(s.refused), 1);
    m.put(club + 0x80, s.owner < 0 ? 0 : owner_record);
    m.put(owner_record + 0x1c, s.owner == 1 ? person : 0);
    m.put(owner_record + 0x63, std::uint32_t(s.owner_byte), 1);
    m.put(person + 0x2f, std::uint32_t(s.b2f), 1);
    m.put(person + 0x36, std::uint32_t(s.b36), 1);
    m.put(person + 0x3d, std::uint32_t(s.b3d), 1);
    m.put(person + 0x40, std::uint32_t(s.b40), 1);
    m.put(club + 0x30, s.has_league ? league : 0);
    m.put(league, 3);
    m.put(league + 0x18, std::uint32_t(s.power), 2);
    m.put(league_table + 12, std::uint32_t(s.table_entry));
    m.put(sc.toc(-0x7b5c), league_table);
    m.put(club + 0x3c, s.has_board ? board_record : 0);
    m.put(board_record + 4, std::uint32_t(s.funds));
    m.put(board_record + 8, std::uint32_t(s.owed));
    m.put(board_record + 0xc, std::uint32_t(s.held));
    m.put(board_record + 0x4d, std::uint32_t(s.board_flag), 1);
    m.put(sc.toc(-0x7b04), std::uint32_t(s.owners));
    m.put(sc.toc(-0x7b98), owner_source);
    const auto pool = sc.toc(-0x55a0);
    for (const auto& [offset, value] : s.pool) m.put_double(pool + offset, value);
    for (const auto& write : s.extra) m.writes.push_back(write);
    return m;
}

// The sample table steered: the cursor and base set so that the draws that follow read `values` (halfwords, base 0
// unless given). With `redraw_at` n (from 1) the n-th draw steps past the table's end and takes the redraw path.
inline void script(const Scenario& sc, Memory& m, const std::vector<std::uint32_t>& values, unsigned redraw_at = 0,
                   std::uint32_t base = 0) {
    const auto first = sc.toc(-0x7b8c), last = sc.global(-0x7b88);
    const std::uint32_t last_index = (last - first) / 2;
    const std::uint32_t start = redraw_at == 0 ? 200 : last_index - (redraw_at - 1);
    const auto cursor = first + 2 * start;
    for (std::size_t k = 0; k < values.size(); ++k) {
        const auto at = cursor + 2 * std::uint32_t(1 + k);
        if (at <= last) m.put(at, values[k], 2);
    }
    m.put(sc.toc(-0x7b84), cursor);
    m.put(sc.toc(-0x7b90), base);
}
inline std::vector<std::uint32_t> random_values(std::mt19937& rng, unsigned count) {
    static const std::uint32_t alphabet[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 7, 8, 8, 12, 16, 24, 100, 999, 1000, 2500, 40000, 65535};
    std::vector<std::uint32_t> values(count);
    for (auto& value : values) value = alphabet[rng() % (sizeof alphabet / sizeof alphabet[0])];
    return values;
}

// Pushes cases for one routine: each setup with `scripts` random sample scripts and nine with a redraw at the 1st to
// 9th draw. `fifth` is the value of r5 (the account when it is not given).
// A balance that puts the club in wealth band `band` for reputation `rep`, as 0x100f55b0 reads them (see
// finance_ledger.cpp `wealth`): the coordinator's runs check every routine of the unit, so a band cannot be answered there.
inline std::int64_t balance_for_band(int band, std::int32_t rep) {
    const std::int64_t r = rep;
    const std::int64_t shortfall = r < 5000 ? -(r * 250) : -((r - 5000) * 1000 + 1'250'000);
    switch (band) {
    case 2: return r * 2500 + 5'000'000 + 1000;
    case 1: return r * 1000 + 3'500'000 + 1000;
    case 0: return shortfall > 0 ? shortfall + 1000 : 0;
    case -1: return shortfall - 1000;
    default: return -2'500'000 - r * 1500 - 1000;
    }
}

// Pushes cases for one routine: each setup with `scripts` random sample scripts and nine with a redraw at the 1st to
// 9th draw. A setup with a band is pushed twice: with the band answered (when the wealth routine is not under check) and
// with the balance that makes the real wealth routine give the band. `fifth` is the value of r5 (the account when not given).
class Pusher {
public:
    Pusher(const Scenario& sc, std::vector<Case>& cases, std::uint32_t entry, std::uint32_t seed)
        : sc(sc), cases(cases), entry(entry), rng(seed) {}
    void operator()(const Setup& setup, const std::string& label, unsigned scripts = 8,
                    std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}, std::int64_t fifth = -1,
                    std::vector<Case::Poke> pokes = {}) {
        if (setup.band >= -2 && setup.band <= 2) {
            Setup real = setup;
            real.band = 99;
            real.balance = balance_for_band(setup.band, setup.reputation);
            one(real, label + " (real band " + std::to_string(setup.band) + ")", scripts / 2, answers, fifth, pokes);
        }
        one(setup, label, scripts, std::move(answers), fifth, std::move(pokes));
    }

private:
    void one(const Setup& setup, const std::string& label, unsigned scripts,
             std::vector<std::pair<std::uint32_t, std::uint32_t>> answers, std::int64_t fifth, std::vector<Case::Poke> pokes) {
        const auto manager = sc.global(-0x7ab8);
        const auto club = sc.club(setup.club);
        const auto account = sc.r32(manager) + setup.club * 0x168;
        if (setup.band != 99) answers.push_back({0x100f55b0, std::uint32_t(setup.band)});
        for (unsigned n = 0; n < scripts + 9; ++n) {
            auto memory = memory_of(sc, setup);
            script(sc, memory, random_values(rng, 12), n < scripts ? 0 : n - scripts + 1, n % 5 == 4 ? 7 : 0);
            cases.push_back({.entry = entry,
                             .registers = {{3, manager}, {4, club}, {5, fifth < 0 ? account : std::uint32_t(fifth)}},
                             .memory = std::move(memory.writes),
                             .label = label + " #" + std::to_string(serial++),
                             .answers = answers,
                             .pokes = pokes});
        }
    }

    const Scenario& sc;
    std::vector<Case>& cases;
    std::uint32_t entry;
    std::mt19937 rng;
    unsigned serial = 0;
};
}
