// MANAGER_MANAGER.CPP checker cases for the transfer-deal verdict block (manager_manager_deal.hpp):
// 0x101a0f50, the board's verdict on a deal for a player.
//
// Arguments are the career's real records: the module object (TOC -0x79f0), a real club (r5), a
// real person as the player (r4) and a price. A share of the cases runs on the real world as it
// is (other modules' routines run); the randomized ones write values over real clubs, people and
// board rows (undone afterwards), build the staff, deal, league and region records they point to in
// the scratch area, and answer the other modules' routines (the staff lookups, ratings, valuations,
// the deal record) so that every branch of the rules is reached, with a fixed seed. The draw table is
// a table of halfwords in the scratch area, as the review's cases do. The captured week adds the
// real calls.
#include "manager_manager_checks.hpp"
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <string>
#include <vector>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t verdict = 0x101a0f50;
constexpr std::int32_t manager_object = -0x79f0, club_total = -0x7b50, pool_slot = -0x536c,
                       home_league_slot = -0x7a0c, base_slot = -0x7b90, cursor_slot = -0x7b84, last_slot = -0x7b88;
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
// The scratch area: the records the case points the routine at.
constexpr std::uint32_t staff_people = scratch_base + 0x1000,  // three people of 0x40 bytes
    staff_records = scratch_base + 0x1100,                      // three staff records of 0x80 bytes
    lookup_record = scratch_base + 0x1300, deal_record = scratch_base + 0x1400, region_record = scratch_base + 0x1500,
    league_record = scratch_base + 0x1700, career_record = scratch_base + 0x1800, table_area = scratch_base + 0x9000;
// The other modules' routines the verdict calls.
constexpr std::uint32_t staff_lookup = 0x1018c340,
                        staff_flag = 0x102de3f0, deal_of = 0x10082f20,
                        buyer = 0x10103510, club_state = 0x1014c2e0,
                        player_value = 0x100f5770, level_of = 0x100f55b0, redraw = 0x1077b770;
// The module's pool: offsets of the doubles the verdict reads.
constexpr std::uint32_t pool_zero = 0x8, pool_reputation = 0x60, pool_ask = 0x98, pool_floor = 0x100,
                        pool_exponent = 0x120, pool_offset = 0x128, pool_big_ask = 0x130, pool_two = 0x138,
                        pool_gain = 0x140, pool_one = 0x148, pool_other = 0x150, pool_limit = 0x158;

// How a batch leans: percentages steering the random values toward rules a uniform draw rarely reaches.
struct Bias {
    unsigned flagged = 50, lookup_null = 8, figure_low = 10, staff_missing = 15, flag_answers_zero = 20,
             ratings_zero = 10, wide_glue = 12, zero_glue = 8, table_threes = 0, deal_null = 6, answered = 85,
             rich = 20, region_set = 55, home_same = 25, reputation_edge = 20, real_flag = 10, staff_all_null = 0,
             fraction_glue = 0, edge_glue = 0, overflow_glue = 0, ratios = 35, buyer_null = 15, player_null = 8, small_fee = 0, rating_pool = 25, real_player = 60, rich_clubs = 0, rich_players = 10, best_players = 40, star_player = 0, star_club = 0, last_small = 25, fee_edge = 0, zero_gains = 0, big_levers = 0, zero_figure = 0;
};

// A bound beyond +-0xffff and a table number (every halfword the same, the base 0) for which the
// floating-point stage's total passes bound - 1: the share (ratio times the number modulo 0xffff,
// truncated) plus a tenth (rounded) of the second draw. Found by the stage's own arithmetic.
struct Overflow { std::int32_t bound; std::uint32_t halfword; };
const std::vector<Overflow>& overflows() {
    static const std::vector<Overflow> found = [] {
        std::vector<Overflow> list;
        const std::int32_t bounds[] = {65536, 65537, 70000, 100000, 131072, 1000000, 123456789, 0x40000000, 0x7fffffff,
                                       -65536, -65537, -100000, -1000000, -123456789, int(0x80000000u) + 1, int(0x80000000u)};
        for (const auto bound : bounds) {
            unsigned kept = 0;
            for (std::uint32_t h = 0; h < 0x10000 && kept < 3; ++h) {
                const std::int32_t x = std::int32_t(h);
                const std::int32_t rest = x % 0xffff;
                const double ratio = double(bound) / 65535.0;
                const double share_d = std::trunc(ratio * double(rest));
                if (!(share_d > -2147483648.0 && share_d < 2147483648.0)) continue;
                const std::int32_t share = std::int32_t(share_d);
                const std::int32_t next = std::int32_t(std::uint32_t(bound) * 10u) / 0xffff;
                std::int32_t drawn = 0;
                if (next != 0) {
                    const std::int32_t r = std::int32_t(x % next);
                    drawn = next > 0 ? r : -r;
                }
                const std::int32_t total = std::int32_t(std::uint32_t(share) + std::uint32_t((std::int32_t(std::uint32_t(drawn) + 5u)) / 10));
                if (total > std::int32_t(std::uint32_t(bound) - 1u)) { list.push_back({bound, h}); ++kept; }
            }
        }
        return list;
    }();
    return found;
}
}

// Real records as the career has them, nothing answered: clubs spread over the table, a few
// people each, both flag values.
MANAGER_MANAGER_CASES("deal real") {
    if (scenario.match) return;
    const auto clubs = scenario.global(club_total);
    if (!clubs) return;
    const auto object = scenario.global(manager_object);
    const unsigned stride = clubs > 60 ? clubs / 60 : 1, people = scenario.people();
    unsigned k = 0;
    for (unsigned n = 0; n < clubs; n += stride, ++k) {
        for (const std::uint32_t flag : {0u, 1u}) {
            const auto player = scenario.person((k * 977 + flag * 131) % people);
            cases.push_back({.entry = verdict, .registers = {{3, object}, {4, player}, {5, scenario.club(n)},
                                                              {6, 100000 + 777 * k}, {7, flag}},
                             .label = "real club " + std::to_string(n) + " flag " + std::to_string(flag)});
        }
    }
    cases.push_back({.entry = verdict, .registers = {{3, object}, {4, scenario.person(3)}, {5, scenario.club(1)},
                                                      {6, 5000000}, {7, 0x100}},
                     .label = "flag 0x100 (byte 0)"});
    cases.push_back({.entry = verdict, .registers = {{3, object}, {4, scenario.person(3)}, {5, scenario.club(1)},
                                                      {6, 5000000}, {7, 0xff}},
                     .label = "flag 0xff (byte -1)"});
}

namespace {
double pool_value(const Scenario& sc, std::uint32_t offset) {
    return std::bit_cast<double>(sc.s.memory.read<8>(sc.toc(pool_slot) + offset));
}

// One randomized call over a real club and a real person.
Case randomized(Rng& rng, const Scenario& sc, const Bias& bias) {
    const auto clubs = sc.global(club_total);
    const auto object = sc.global(manager_object);
    const unsigned n = rng.chance(bias.star_club) && 7289 < clubs ? 7289 : rng.below(clubs);
    // Mostly a real player (a person with a player record and a club): the valuations of the
    // other modules run over him as he is.
    static std::vector<std::uint32_t> players;
    if (players.empty()) {
        for (unsigned k = 0; k < sc.people() && players.size() < 4000; ++k)
            if (sc.r32(sc.person(k) + 0x14) && sc.r32(sc.person(k) + 0x24)) players.push_back(sc.person(k));
    }
    // The star: a player this career's valuations put above 5000000 (found by a search over the
    // snapshot; any other snapshot just gets an ordinary person).
    constexpr unsigned star_person = 4452;
    const auto club = sc.club(n);
    const auto player = rng.chance(bias.star_player) && star_person < sc.people() ? sc.person(star_person)
                      : !players.empty() && rng.chance(bias.real_player) ? players[rng.below(unsigned(players.size()))]
                                                                         : sc.person(rng.below(sc.people()));
    const bool flagged = rng.chance(bias.flagged);
    Case item{.entry = verdict, .registers = {{3, object}, {4, player}, {5, club}, {7, flagged ? 1u : 0u}}};
    item.label = std::string("deal club ") + std::to_string(n) + (flagged ? " flagged" : " plain");
    auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    auto put_double = [&](std::uint32_t at, double value) {
        const auto bits = std::bit_cast<std::uint64_t>(value);
        put(4, at, std::uint32_t(bits >> 32));
        put(4, at + 4, std::uint32_t(bits));
    };
    const auto answer = [&](std::uint32_t entry, std::uint32_t value) { item.answers.push_back({entry, value}); };
    const bool answered = rng.chance(bias.answered);
    item.label += answered ? " answered" : " real";

    // The draw table: halfwords in the scratch area, the base, and where the table ends.
    const bool threes = rng.chance(bias.table_threes);
    for (unsigned k = 0; k < 24; ++k)
        put(2, table_area + k * 2, threes ? 3 * rng.below(600) : rng.chance(50) ? rng.below(24) : rng.below(2000));
    put(4, sc.toc(cursor_slot), table_area - 2 + 2 * rng.below(3));
    put(4, sc.toc(last_slot), rng.chance(bias.last_small) ? table_area + 2 * rng.below(14) : table_area + 46);
    put(4, sc.toc(base_slot), bias.zero_figure ? 0xfffff000u : threes ? 0 : rng.pick<std::uint32_t>({0, 0, 1, 2, 3, 7, 20, 0xffff, 0x10001, 0x7fff8000, 0x80000000u, 0xfffff000u}));

    // The club: the staff people (scratch records), its region, league and reputation.
    if (rng.chance(bias.staff_missing + 60)) {
        for (unsigned k = 0; k < 3; ++k) {
            const auto person_at = staff_people + k * 0x40, record_at = staff_records + k * 0x80;
            const auto mode = rng.below(100);
            put(4, club + 0x190 + 4 * k, mode < bias.staff_missing || rng.chance(bias.staff_all_null) ? 0 : person_at);
            put(4, person_at, rng.pick<std::uint32_t>({0, 1, 7, 100, 0xffffffffu, 0x10001, 4000}));
            put(4, person_at + 0x1c, mode >= bias.staff_missing && mode < 2 * bias.staff_missing ? 0 : record_at);
            put(1, record_at + 0x3e, rng.pick<std::uint32_t>({0, 1, 2, 5, 10, 20, 0x7f, 0x80, 0xff, 0xfb, 3, 4}));
        }
    }
    if (rng.chance(bias.region_set)) {
        put(4, club + 0x2c, region_record);
        put(1, region_record + 0x120, rng.pick<std::uint32_t>({0, 1, 0xff, 0x80, 5}));
    } else if (rng.chance(30)) {
        put(4, club + 0x2c, 0);
    }
    const auto home_cell = sc.toc(home_league_slot);
    const auto home_id = rng.pick<std::uint32_t>({0, 1, 5, 40});
    put(4, home_cell, home_id);
    if (rng.chance(80)) {
        put(4, club + 0x30, league_record);
        put(2, league_record + 0x18, rng.chance(bias.rich_clubs) ? rng.pick<std::uint32_t>({0x7fff, 3000, 1000, 0x4000, 0x2000, 0x7000}) : rng.pick<std::uint32_t>({0, 1, 5, 20, 100, 1000, 0x7fff, 0x8000, 300, 3000}));
        put(4, league_record, rng.chance(bias.home_same) ? home_id : rng.pick<std::uint32_t>({0, 1, 5, 40, 7, 0xffffffffu}));
    } else if (rng.chance(40)) {
        put(4, club + 0x30, 0);
    }
    put(2, club + 0x1ec, rng.chance(bias.rich_clubs) ? rng.pick<std::uint32_t>({0x7fff, 0x7000, 0x6000, 0x5000, 0x4000}) :
        rng.chance(bias.reputation_edge) ? rng.pick<std::uint32_t>({0, 1, 0x7fff, 0x8000, 0xffff, 0xfff0, 3, 10})
                                         : rng.pick<std::uint32_t>({0x100, 0x400, 0x800, 0x1000, 0x1388, 0x2000, 0x2710, 0x3000, 0x4000, 0x6000, 0x7000}));
    // The player: his record at +0x14 (a byte at +0x2d), his link and his id.
    const auto career_byte = rng.pick<std::uint32_t>({0, 1, 0xff, 5, 0x7f, 0x80, 10, 100, 20});
    if (sc.r32(player + 0x14) && rng.chance(85)) {
        put(1, sc.r32(player + 0x14) + 0x2d, career_byte);
        // The player's attribute bytes (1 to 20; the valuations pick their constants by them).
        if (rng.chance(bias.rich_players)) {
            const bool best = rng.chance(bias.best_players);
            for (std::uint32_t at = 0x0c; at < 0x2d; ++at)
                if (best || rng.chance(70)) put(1, sc.r32(player + 0x14) + at, best ? 20 : rng.pick<std::uint32_t>({1, 10, 14, 15, 17, 20, 20, 20, 18, 0xff, 0}));
        }
    } else if (rng.chance(80)) {
        put(4, player + 0x14, career_record);
        put(1, career_record + 0x2d, career_byte);
    }
    if (rng.chance(bias.player_null)) put(4, player + 0x24, 0);
    // The board's row of the club.
    const auto row = sc.r32(object) + sc.r32(club) * 0x4c;
    for (const std::uint32_t at : {6u, 0x14u, 0x16u})
        put(2, row + at, rng.pick<std::uint32_t>({0, 1, 2, 0x4e1, 0x4e2, 0x4e3, 0x1f4, 0x1f5, 0xfa, 0xfb, 0x1000, 0x7fff, 0x8000, 0xffff, 0x2710, 0x8001, 0x4e2 + 1}));
    // The pool: the glue's exponent and offset (wide ones reach the draw's floating-point stage).
    const auto exponent = pool_value(sc, pool_exponent);
    const auto big = rng.chance(bias.wide_glue);
    const bool fractional = rng.chance(bias.fraction_glue), edge = rng.chance(bias.edge_glue),
               overflowing = rng.chance(bias.overflow_glue) && !overflows().empty();
    if (fractional || edge || overflowing) {
        // pow(x, 0) is 1: the glue's number is the offset plus 1, whatever the rating.
        put_double(sc.toc(pool_slot) + pool_exponent, 0.0);
        if (fractional) {
            put_double(sc.toc(pool_slot) + pool_offset, rng.pick<double>({-0.5, -1.5, -1.999, -0.001, -1.0, -1.0, -0.75, 0.0 - 1.0}));
        } else if (edge) {
            const auto bound = rng.pick<std::int32_t>({65534, 65535, 65536, -65534, -65535, -65536, -65537, 1, -1, 2, 0x7fffffff, int(0x80000000u)});
            put_double(sc.toc(pool_slot) + pool_offset, double(bound) - 1.0);
        } else {
            const auto& pick = overflows()[rng.below(unsigned(overflows().size()))];
            put_double(sc.toc(pool_slot) + pool_offset, double(pick.bound) - 1.0);
            for (unsigned k = 0; k < 24; ++k) put(2, table_area + k * 2, pick.halfword);
            put(4, sc.toc(cursor_slot), table_area - 2);
            put(4, sc.toc(last_slot), table_area + 46);
            put(4, sc.toc(base_slot), 0);
        }
    } else if (big) put_double(sc.toc(pool_slot) + pool_offset,
                        rng.pick<double>({1e5, 70000.0, -70000.0, 65535.0, 65536.0, -65535.0, -65536.0, 1e9, -1e9, 3e9, 0.5, -0.5, 1e18, 32767.5, 65535.99, 4294967296.0, 12345.6}));
    else if (rng.chance(bias.zero_glue)) put_double(sc.toc(pool_slot) + pool_offset, 0);
    else if (rng.chance(10)) put_double(sc.toc(pool_slot) + pool_offset, rng.pick<double>({0.5, 0.25, -0.75, 1e-3, 1.0, -1.0, 3.0, 100.0}));
    if (!(fractional || edge || overflowing) && rng.chance(10))
        put_double(sc.toc(pool_slot) + pool_exponent, rng.pick<double>({0.0, 1.0, 2.0, -1.0, 0.5, 3.0, 8.0}));
    (void)exponent;
    // The scale of the reputation (the price's clamp), the gains of the level rules and the zero
    // the glue's number is compared with: now and then other values, so the clamps and the
    // compare against zero turn.
    if (rng.chance(25))
        put_double(sc.toc(pool_slot) + pool_reputation, rng.pick<double>({0.0, 1e-4, 0.01, 1.0, 10.0, -3.0, 0.05, 1e3, 1e-2 * 7}));
    for (const auto offset : {pool_gain, pool_two, pool_one, pool_other})
        if (rng.chance(15)) put_double(sc.toc(pool_slot) + offset, rng.pick<double>({0.0, 1.0, -1.0, 1e5, 1e6, 0.5, 3e6, -2e6, 1e-3, 5e9, 123456.5}));
    // The ratios of the price compares: the ones the fee is weighed with (a NaN or an infinity
    // makes the compare unordered; the real ones put the turn at a fee the real valuations reach).
    for (const auto offset : {pool_ask, pool_floor, pool_big_ask, pool_limit})
        if (rng.chance(bias.ratios))
            put_double(sc.toc(pool_slot) + offset,
                       rng.pick<double>({0.0, 1e-6, 1e-3, 0.01, 0.1, 0.5, 1.0, 2.0, 10.0, 100.0, -1.0, 1e9, std::nan(""),
                                         std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), 0.3, 3.0, 1e-4, 50.0}));
    if (rng.chance(4)) put_double(sc.toc(pool_slot) + pool_zero, rng.pick<double>({1.0, -1.0, 0.5, 3.0}));

    // Prices, and the other modules' answers. The routines a modern owner runs in place
    // (0x100d2f50, 0x10084be0, 0x10085860, 0x100c9070, 0x1018c270, 0x1077f900) are called by the original
    // without the dispatcher, so an answer would reach only the rewrite: they run as they are,
    // steered through the records they read.
    std::int32_t fee = rng.pick<std::int32_t>({0, 1, 100, 0x249f0, 1000000, 2000000, 0x1e8480, 5000000, 0x7fffffff, -1, int(0x80000000u), 40000,
                                                 20000, 300000, 750000, 1500000, 3000000, 8000000, 12000000, 60000000});
    if (rng.chance(30)) fee = std::int32_t(std::exp(rng.below(1000) / 1000.0 * 19.0 + 0.5));
    if (rng.chance(bias.small_fee)) fee = rng.pick<std::int32_t>({0, 1, 7, 100, 1000, 5000, 20000, 50000, 120000, 149999, 150000});
    // The rating (0x100d2f50) is clamped between two constants of the database's pool (+0xc8 and
    // +0xb8, also the divisor): other values give the ratings 0, 41 or below 0 the real ones never
    // give.
    if (rng.chance(bias.rating_pool)) {
        put_double(sc.toc(-0x5614) + 0xb8, rng.pick<double>({0.0, 41.0, -1.0, -2.0, 5.0, 127.0, 300.0, 2.0, 0.5, std::nan(""), 20.0}));
        put_double(sc.toc(-0x5614) + 0xc8, rng.pick<double>({0.0, -5.0, 30.0, 1.0, std::nan(""), -1.0, 100.0}));
    }
    // The staff-flag routine (0x102de3f0) as it is: the entries of the object's table for the player.
    const bool real_flag_used = rng.chance(bias.real_flag);
    if (real_flag_used) {
        const auto flags = sc.global(-0x7b28);
        const auto entries = scratch_base + 0x2000;
        const unsigned count = rng.below(4);
        put(4, flags + 4, entries);
        put(4, flags + 8, count);
        for (unsigned k = 0; k < count; ++k) {
            put(4, entries + k * 0x1c, sc.r32(player) + (rng.chance(75) ? 0 : 1));
            put(1, entries + k * 0x1c + 0x18, rng.pick<std::uint32_t>({8, 8, 7, 0, 0xff}));
            put(1, entries + k * 0x1c + 0x19, rng.pick<std::uint32_t>({0, 1, 0xff, 0x7f, 0}));
        }
    }
    // The staff check (0x1018c270) reads the player's row of the staff owner's table: bytes +0x14 and +0x15.
    const auto owner_table = sc.r32(sc.global(-0x7ba0));
    const auto player_id = sc.r32(player);
    if (owner_table) {
        put(1, owner_table + player_id * 0x24 + 0x14, rng.pick<std::uint32_t>({0, 1, 0xff, 0x7f, 5, 3, 0x80}));
        put(1, owner_table + player_id * 0x24 + 0x15, rng.pick<std::uint32_t>({0, 1, 2, 0xff, 0x7f, 6, 0x80}));
    }
    // The rating (0x100d2f50) reads a halfword of the staff record at +0x24 as well as its byte.
    for (unsigned k = 0; k < 3; ++k)
        put(2, staff_records + k * 0x80 + 0x24, rng.pick<std::uint32_t>({0, 1, 20, 50, 100, 1000, 5000, 0x7fff, 0x8000, 0xffff, 10, 3}));
    if (answered) {
        put(2, lookup_record + 0x30, bias.zero_figure ? 0u : bias.figure_low > rng.below(100) ? rng.pick<std::uint32_t>({0, 1, 0x14, 0xffff, 0x8000, 5}) :
                                     rng.pick<std::uint32_t>({0x15, 0x16, 0x30, 0x64, 0x7fff, 0xc8, 0x100, 60, 500}));
        answer(staff_lookup, rng.chance(bias.lookup_null) ? 0 : lookup_record);
        if (!real_flag_used) answer(staff_flag, rng.chance(bias.flag_answers_zero) ? 0 : rng.pick<std::uint32_t>({1, 1, 2, 0x100}));
        answer(redraw, rng.next());
        answer(deal_of, rng.chance(bias.deal_null) ? 0 : deal_record);
        put(4, deal_record + 4, rng.below(clubs));
        put(1, deal_record + 0x38, rng.below(256));
        put(1, deal_record + 0x44, rng.pick<std::uint32_t>({0, 1, 0x19, 0x1a, 0x7f, 0x80, 0xff, 30, 10}));
        answer(buyer, rng.chance(bias.buyer_null) ? 0 : club);
        const auto value_high = rng.pick<std::uint32_t>({0, 0, 0, 0xffffffffu, 1});
        answer(player_value, value_high);
        // The value (high word 0, the low word the player's address, as the answered call leaves
        // r4) times 12 over 10, against the price: the price at, below and above it.
        if (value_high == 0 && rng.chance(bias.fee_edge))
            fee = std::int32_t((std::int64_t(player) * 12) / 10 + rng.pick<int>({-1, 0, 1}));
        answer(level_of, rng.pick<std::uint32_t>({0, 1, 2, 3, 0x102, 0xff, 5}));
        answer(club_state, rng.chance(15) ? 0 : rng.pick<std::uint32_t>({1, 0x100, 0xff, 2}));
    }
    if (rng.chance(bias.zero_gains)) {
        // No gain and no constants: the target is the valuation itself, which is often 0, as the
        // limit is with a ratio of 0: the compare is then equal.
        for (const auto offset : {pool_gain, pool_two, pool_one, pool_other, pool_limit}) put_double(sc.toc(pool_slot) + offset, 0.0);
    }
    item.registers.push_back({6, std::uint32_t(fee)});
    if (rng.chance(bias.big_levers)) {
        // For the price above 5000000: the first ratio out of reach, the second within, a deal level
        // above 0 (the writes come after the generic ones, which they override).
        put_double(sc.toc(pool_slot) + pool_ask, rng.pick<double>({1e9, std::nan(""), std::numeric_limits<double>::infinity(), 1e3}));
        put_double(sc.toc(pool_slot) + pool_big_ask, rng.pick<double>({0.0, 1e-3, 0.1, 1e-6, 0.01, 1e9, std::nan("")}));
        put(1, deal_record + 0x44, rng.pick<std::uint32_t>({1, 2, 50, 0x7f, 5, 0x19, 0x1a, 0, 0x80, 0xff}));
    }
    return item;
}
}

MANAGER_MANAGER_CASES("deal randomized") {
    if (scenario.match || !scenario.global(club_total)) return;
    Rng rng{0x0f50de41};
    for (unsigned k = 0; k < 1500; ++k) cases.push_back(randomized(rng, scenario, {}));
    // Leaning batches for the rules a uniform draw rarely reaches.
    // The staff screening through to the glue's draw: no refusal before it, a glue number that is
    // a fraction (the bound 0), zero (no draw), near the +-0xffff range test, or beyond it.
    for (unsigned k = 0; k < 500; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 100, .lookup_null = 0, .figure_low = 0, .staff_missing = 0, .flag_answers_zero = 70, .fraction_glue = 100, .last_small = 60}));
    for (unsigned k = 0; k < 500; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 100, .lookup_null = 0, .figure_low = 0, .staff_missing = 0, .flag_answers_zero = 70, .edge_glue = 100, .last_small = 60}));
    for (unsigned k = 0; k < 900; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 100, .lookup_null = 0, .figure_low = 0, .staff_missing = 0, .flag_answers_zero = 80, .overflow_glue = 100}));
    for (unsigned k = 0; k < 400; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 100, .lookup_null = 0, .figure_low = 0, .staff_missing = 0, .flag_answers_zero = 60, .zero_glue = 100, .real_flag = 50, .last_small = 60}));
    // The staff-flag routine as it is (its answer depends on the mode asked).
    for (unsigned k = 0; k < 400; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 100, .lookup_null = 0, .figure_low = 0, .staff_missing = 0, .real_flag = 100, .rating_pool = 40}));
    // The price of a player not flagged, and of a flagged one whose staff are not asked.
    for (unsigned k = 0; k < 9000; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 0, .deal_null = 0, .answered = 100, .region_set = 10, .ratios = 60, .player_null = 3, .small_fee = 70, .rich_clubs = 60, .rich_players = 80}));
    // The best players for the rich clubs: valuations above 5000000.
    for (unsigned k = 0; k < 3000; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 0, .deal_null = 0, .answered = 100, .region_set = 10, .ratios = 60, .player_null = 0, .small_fee = 60, .real_player = 100, .rich_clubs = 100, .rich_players = 100, .best_players = 100}));
    for (unsigned k = 0; k < 1500; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 0, .deal_null = 0, .answered = 100, .region_set = 10, .ratios = 60, .player_null = 0, .small_fee = 60, .real_player = 100, .rich_clubs = 70, .rich_players = 100, .best_players = 50, .star_player = 100, .big_levers = 70}));
    for (unsigned k = 0; k < 4000; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 0, .deal_null = 0, .answered = 100, .region_set = 0, .ratios = 60, .player_null = 0, .small_fee = 60, .real_player = 100, .rich_clubs = 100, .rich_players = 100, .best_players = 50, .star_player = 100, .star_club = 100, .big_levers = 100}));
    for (unsigned k = 0; k < 3000; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 100, .lookup_null = 100, .deal_null = 0, .answered = 100, .ratios = 60, .buyer_null = 5, .player_null = 3, .small_fee = 30, .rich_clubs = 50, .rich_players = 70, .fee_edge = 30, .zero_gains = 25}));
    // A deal without a record (0x10082f20 answers 0) on the price path.
    for (unsigned k = 0; k < 300; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 0, .deal_null = 100, .answered = 100, .ratios = 60}));
    // The second read of the staff figure at 0 after the first passed the table (a negative base makes the
    // table's remainder negative, so a figure of 0 passes it): the draw on a zero figure is skipped.
    for (unsigned k = 0; k < 400; ++k)
        cases.push_back(randomized(rng, scenario, {.flagged = 100, .lookup_null = 0, .figure_low = 100, .staff_missing = 0, .flag_answers_zero = 70, .zero_figure = 100}));
}
}
