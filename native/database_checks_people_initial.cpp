// Fixtures of the people block, part "initial": 0x100b2b80 (shuffle_person_rows).
//
// World: a table of 0x70-byte rows (indexes -100..100 around the base, so negative indexes of a negative count
// and the small indexes of a wide count stay mapped), each with a player record (+0x14), maybe a staff record
// (+0x1c), a detail record (+0x24) with a club (+0x2c) and a word (+0x30), and clubs (some above 0x80000000)
// whose nations come from the globals the two gates compare with. The sample pool is tiny in many cases, so
// it wraps (a draw of rng_core, scripted or real) every few advances.
// Cases: counts that run many trials with random samples; a count word that changes at a chosen draw (a stub
// of rng_core writes it) to reach a count of 0, a negative count, the +-65535 limits and wide counts, with
// samples small enough that wide indexes stay on mapped rows; and an all-zero pool for the overflowing counts.
// Callees: the date copy, the default date and the gates run for real; the contract manager's lookup and the
// date of 0x100cab10 (0x10082f20, 0x107156f0) are scripted.
#include "database_checks_people_detail.hpp"
#include <iterator>

namespace cm::aot::database_checks {
namespace {
using namespace people_checks;

constexpr std::uint32_t entry = 0x100b2b80, contract_lookup = 0x10082f20, date_today = 0x107156f0;
constexpr std::uint32_t table_cell = data + 0x100, count_cell = data + 0x104, manager_cell = data + 0x108,
                        salt_cell = data + 0x110, cursor_cell = data + 0x114, end_cell = data + 0x118,
                        cells = data + 0x1000, pool_start = data + 0x1400, contract = data + 0x2000,
                        date_words = data + 0x2100, nations = data + 0x2200, rows_base = data + 0x6000,
                        low_clubs = data + 0x9000, low_players = data + 0xa000, details = data + 0xb000,
                        staff = data + 0xf000, high_clubs = high, high_players = high + 0x1000;
constexpr int first_row = -100, last_row = 100;
constexpr unsigned club_count = 8;
constexpr std::uint32_t row_size = 0x70;

// The globals the gates compare nations with: each slot points at a cell holding a distinct word, odd ones
// above 0x80000000 so that signed and unsigned compares differ.
constexpr std::int32_t gate_slots[] = {-0x74f4, -0x7af4, -0x7a14, -0x750c, -0x75c4, -0x7504, -0x75c8, -0x7a10, -0x7a1c,
                                      -0x757c, -0x7578, -0x7574, -0x7650, -0x7654, -0x7658, -0x79fc};
constexpr std::uint32_t slot_word(unsigned n) { return (0x1000 + 0x11 * n) | (n & 1 ? 0x80000000u : 0u); }
constexpr std::uint32_t miss = 0x7777;

std::uint32_t club_at(unsigned n) { return n < 4 ? low_clubs + 0x400 * n : high_clubs + 0x400 * (n - 4); }
std::uint32_t row_address(int index) { return rows_base + std::uint32_t(index) * row_size; }
unsigned row_number(int index) { return unsigned(index - first_row); }
std::uint32_t player_at(unsigned k) { return k % 2 ? low_players + 0x10 * k : high_players + 0x10 * k; }
std::uint32_t detail_at(unsigned k) { return details + 0x40 * k; }

enum class Pool { random, small, zero };

struct Case {
    std::uint32_t seed;
    std::int32_t count;
    Pool pool;
    unsigned wrap_every;     // pool words between the cursor's end (1-8), 0: the whole 64-word pool
    std::uint32_t salt;
    bool null_rows;          // some rows lack a player, staff or detail record
};

void build(Cpu& c, const Case& t) {
    Mix r{t.seed};
    // The pool: samples as the mode wants, a short end so that it wraps, the cursor anywhere in it.
    c.put32(c.r(2) - 0x7b84, cursor_cell);
    c.put32(c.r(2) - 0x7b90, salt_cell);
    c.put32(c.r(2) - 0x7b88, end_cell);
    c.put32(c.r(2) - 0x7b8c, pool_start);
    const auto words = t.wrap_every ? t.wrap_every : 64;
    c.put32(end_cell, pool_start + 2 * words);
    c.put32(cursor_cell, pool_start + 2 * r.below(words + 2));
    c.put32(salt_cell, t.salt);
    for (std::uint32_t i = 0; i < 0x80; ++i)
        c.put16(pool_start + 2 * i, t.pool == Pool::random ? r.below(0x10000) : t.pool == Pool::small ? r.below(13) : 0);
    // The globals and tables.
    for (unsigned n = 0; n < std::size(gate_slots); ++n) {
        c.put32(c.r(2) + std::uint32_t(gate_slots[n]), cells + 4 * n);
        c.put32(cells + 4 * n, slot_word(n));
    }
    c.put32(c.r(2) - 0x7a70, cells + 0x80);  // the id of the nation row (exempt rule)
    c.put32(cells + 0x80, 3);
    c.put32(c.r(2) - 0x7520, cells + 0x84);  // the base of the 0xc8-byte nation rows
    c.put32(cells + 0x84, nations);
    c.put32(c.r(2) - 0x7b7c, manager_cell);
    c.put32(manager_cell, data + 0x2300);
    c.put32(contract + 0x28, r.pick<std::uint32_t>({0, 1, 0x7ce, 0xffff, 0x8000, 0x7fff}));
    c.put32(contract + 0x2a, r.pick<std::uint32_t>({0, 1, 0x7ce, 0xffff, 0x8000, 0x7fff}));
    c.put16(date_words, r.pick<std::uint32_t>({0, 1, 0x7ce, 0xffff, 0x8000, 0x7fff}));
    c.put16(date_words + 2, (r.below(2) == 0) ? c.u16(contract + 0x2a) : r.pick<std::uint32_t>({0, 1, 0x7ce, 0x8000}));
    c.put32(date_words + 4, 0x1234abcd);
    // Clubs: nations from the gates' globals (or a miss), a status of 2 now and then, a nation record.
    for (unsigned n = 0; n < club_count; ++n) {
        const auto club = club_at(n);
        const auto nation = (r.below(3) == 0) ? miss : slot_word(r.below(std::size(gate_slots)));
        c.put32(club + 0x68, nation);
        c.put8(club + 0x11d, r.pick<std::uint32_t>({0, 1, 2, 2, 3}));
        c.put32(club + 0x74, (r.below(2) == 0) ? nations + 3 * 0xc8 : r.pick<std::uint32_t>({0, nations, high + 0x1800}));
    }
    // The rows.
    c.put32(table_cell, rows_base);
    c.put32(c.r(2) - 0x7be0, table_cell);
    c.put32(count_cell, std::uint32_t(t.count));
    c.put32(c.r(2) - 0x7be4, count_cell);
    for (int index = first_row; index <= last_row; ++index) {
        const auto at = row_address(index), k = row_number(index);
        for (std::uint32_t b = 0; b < row_size; ++b) c.put8(at + b, r.below(256));
        const auto detail = detail_at(k);
        c.put32(at + 0xc, club_at(r.below(club_count)));
        c.put32(at + 0x10, (r.below(3) == 0) ? 0 : club_at(r.below(club_count)));
        c.put32(at + 0x14, t.null_rows && (r.below(8) == 0) ? 0 : player_at(k));
        c.put32(at + 0x1c, (r.below(2) == 0) ? staff : 0);
        c.put32(at + 0x24, t.null_rows && (r.below(10) == 0) ? 0 : detail);
        c.put32(detail + 0x2c, (r.below(8) == 0) ? 0 : club_at(r.below(3)));
        c.put32(detail + 0x30, (r.below(2) == 0) ? 0 : r.below(3));
        for (std::uint32_t b = 0; b < 0x10; ++b) c.put8(player_at(k) + b, r.below(256));
    }
}

// A stub of rng_core's callers' neighbours: the contract lookup and today's date for the gate, and (for
// the cases that change the count) a write of the count word at the draw whose number matches.
Fixture shuffle_case(const std::string& name, const Draws& draws, const Case& t, bool overflow,
                     std::int64_t change_at = -1, std::int32_t changed = 0) {
    Mix r{t.seed ^ 0x77};
    const auto answered = (r.below(5) == 0) ? 0u : contract;
    auto f = make(entry, name, draws, [t](Cpu& c) { build(c, t); }, overflow,
                  {{contract_lookup, {answered}, 2}, {date_today, {date_words}, 5}});
    if (change_at >= 0) {
        auto inner = f.stub;
        f.stub = [inner, change_at, changed](State& s) {
            // Only at a draw that comes after both indexes of a trial's pair were drawn from the old count (the
            // second index's pool advance or the parity draw), so the next trial sees the new count throughout.
            if (s.pc == random_entry) {
                Cpu c(s);
                const bool between = c.lr() == entry + 0x240 || c.lr() == entry + 0x95c;
                if (between && c.u32(draw_counter) >= std::uint32_t(change_at)) c.put32(count_cell, std::uint32_t(changed));
            }
            return inner(s);
        };
        f.description += " count " + std::to_string(changed) + " at draw " + std::to_string(change_at);
    }
    return f;
}

constexpr std::uint32_t salts[] = {0, 1, 2, 0x7fff, 0xffff, 0x80000000u, 0xfffffff0u, 0xfffffffeu, 12345};
Draws small_draws(Mix& mix) {
    Draws d;
    for (unsigned i = 0; i < 12; ++i) d.values.push_back(mix.below(13));
    return d;
}
}

std::vector<Fixture> people_initial_fixtures() {
    std::vector<Fixture> out;
    Mix mix{0xb2b80};
    // Counts that run trials with random samples (16 trials per row); the real generator in one case of four.
    constexpr std::int32_t counts[] = {1, 2, 3, 5, 8, 13, 40, 2, 3, 5};
    for (unsigned n = 0; n < 160; ++n) {
        const auto count = counts[n % std::size(counts)];
        const auto draws = script(mix, 24);
        const Case t{mix.next(), count, Pool::random, unsigned(mix.below(9)), salts[mix.below(std::size(salts))],
                     n % 4 != 3};
        out.push_back(shuffle_case("shuffle count " + std::to_string(count), draws, t, n % 3 == 0));
    }
    // A negative salt that no draw has replaced yet (the pool is long): negative sums, so negative remainders
    // and indexes, and the parity test's sign arm.
    for (unsigned n = 0; n < 90; ++n) {
        constexpr std::uint32_t negative[] = {0x80000000u, 0xfffffff0u, 0xfffffffeu, 0x80000001u, 0xfffffffdu};
        const auto draws = script(mix, 24);
        const Case t{mix.next(), counts[n % 6 + 1], Pool::random, 0, negative[mix.below(5)], false};
        out.push_back(shuffle_case("shuffle negative salt", draws, t, n % 3 == 0));
    }
    // No trials: a count of 0 or one that makes the trial count 0 or negative.
    for (const std::int32_t count : {0, -1, -5, -65535, -70000, std::int32_t(0x80000000), -0x20000000}) {
        const auto draws = script(mix, 4);
        out.push_back(shuffle_case("shuffle no trials count " + std::to_string(count), draws,
                                   {mix.next(), count, Pool::random, 3, 0, false}, count & 1));
    }
    // The count word changes at a draw: to 0 (the zero arms), negative counts (negated remainders, then the
    // wide arm's clamp), counts at the +-65535 limits and wide counts (the floating-point arm and range_sample).
    constexpr std::int32_t changed[] = {0, 1, -1, -3, 7, 65535, -65535, 65536, -65536, 65537, -65537, 70000, -70000,
                                        150000, -150000};
    for (unsigned n = 0; n < 220; ++n) {
        const auto to = changed[n % std::size(changed)];
        const auto draws = small_draws(mix);
        const Case t{mix.next(), std::int32_t(mix.below(3) + 1), Pool::small, 1 + unsigned(mix.below(4)),
                     mix.below(13), n % 4 != 3};
        out.push_back(shuffle_case("shuffle changed count", draws, t, n % 3 == 0, mix.below(6), to));
    }
    // An all-zero pool: every wide draw is index 0 (the same row twice), through the overflowing counts.
    for (const std::int32_t to : {0x20000000, 0x40000000, 0x7fffffff, -0x40000000, std::int32_t(0x80000001), 100000000,
                                  -100000000, 429496730, -429496730, 214748365, 1000000, -1000000}) {
        Draws zero;
        zero.values = {0};
        const Case t{mix.next(), 2, Pool::zero, 2, 0, false};
        out.push_back(shuffle_case("shuffle zero pool", zero, t, to & 1, 1 + mix.below(3), to));
    }
    return out;
}
}
