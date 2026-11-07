// DATABASE.CPP's people block, part "swaps": 0x100b3710 (swap_player_attributes_within_clubs) and 0x100b3c70
// (settle_person_counters), see database_people_swaps.hpp. Every pool draw is the compiler's inline copy: advance the
// cursor (a wrap draws from rng_core), then the sample plus the salt, reduced as each site's code does. The
// original's arms for a bound beyond +-65535 (floating point, range_sample 0x100a0d80) cannot run here, every such
// bound being a byte, and are not written (as in database_generation_draws.cpp).
// Registers: r0, r3-r10, cr0 and XER[CA] are left as the last writer on each path leaves them; the non-volatile
// ones are set where the original sets them (the callees save them into the stack image).
#include "database_generation_detail.hpp"
#include "database_people_attributes.hpp"
#include "database_people_detail.hpp"
#include "database_people_swaps.hpp"
#include "database_records.hpp"
#include <bit>

namespace cm::aot::database {
using namespace generation;
using people::mulhw;
using guest::extended;
namespace {
constexpr std::uint32_t abs_entry = 0x10775540, unlisted_pool_constants = 0;

// The pool as these two routines hold it (their registers differ, the TOC words do not).
Pool pool_from_toc(Cpu& c) {
    return {c.toc(pool_salt_slot), c.toc(pool_start_slot), c.toc(pool_end_slot), c.toc(pool_cursor_slot),
            unlisted_pool_constants};
}
void advance_with_scratch_3_0(Cpu& c, const Pool& pool, std::uint32_t back) {
    advance_pool(c, {3, 0}, pool.cursor_at, pool.end_at, pool.salt_at, pool.start, back);
}

// ---- 0x100b3710 ----
namespace club_row {
constexpr std::uint32_t stride = 0x248, squad = 0x98, trials = 100;  // squad: 50 person pointers (inferred)
constexpr std::int32_t table_slot = -0x7be8, count_slot = -0x7b50;
}
namespace person_fields {  // rows of the people table (stride 0x70)
constexpr std::uint32_t player = 0x14, flag = 0x5e, closeness = 0x5c;
}
namespace player_fields {
constexpr std::uint32_t rating = 4, rating_second = 6, kind = 0x11;
}
constexpr std::int32_t people_table_slot = -0x7be0, people_count_slot = -0x7be4;
constexpr unsigned squad_size = 50;

// The original's squad slot of a sum: sum modulo 50 by the multiply-high reciprocal (srawi sets XER[CA]).
std::uint32_t squad_index(Cpu& c, std::uint32_t sum) {
    const auto quotient = std::uint32_t(mulhw(0x51eb851f, sum));
    const auto shifted = srawi(c, quotient, 4);
    const auto rounded = shifted + (shifted >> 31);
    return sum - rounded * squad_size;
}

struct Squad {
    Cpu& c;
    std::uint32_t club;
    std::uint32_t slot(std::uint32_t index) const { return club + 4 * index; }
    std::uint32_t person(std::uint32_t index) const { return c.u32(slot(index) + club_row::squad); }
    std::uint32_t player(std::uint32_t index) const { return c.u32(person(index) + person_fields::player); }
};

// The pool's parity draw (one advance, then the sample plus salt reduced to its low bit with the sign of the
// 32-bit sum): left in r0 (also compared into cr0) with r3 the sign bit. True when the result is not 0.
bool parity_draw(Cpu& c, const Pool& pool, std::uint32_t back) {
    advance_with_scratch_3_0(c, pool, back);
    const auto sample = c.u16(load32(c, 3, pool.cursor_at));
    const auto sum = sample + load32(c, 0, pool.salt_at);
    const auto sign = sum >> 31;
    const auto result = ((sum & 1) ^ sign) - sign;
    c.set(3, sign);
    c.set(0, result);
    return compare(c, std::int32_t(result)) != 0;
}

// Swaps the bytes at `first` and at `second` of the two players (A's gets B's). Leaves the registers of the two
// loads, stores and pointer chains of the original.
void swap_two_bytes(Cpu& c, const Squad& squad, std::uint32_t a, std::uint32_t b, std::uint32_t first,
                    std::uint32_t second) {
    const auto player_a = squad.player(a), player_b = squad.player(b);
    const auto a_first = c.u8(player_a + first), b_first = c.u8(player_b + first);
    c.put8(player_a + first, b_first);
    c.put8(player_b + first, a_first);
    const auto a_second = c.u8(player_a + second), b_second = c.u8(player_b + second);
    c.put8(player_a + second, b_second);
    c.put8(player_b + second, a_second);
    c.set({{0, a_second}, {3, player_b}, {4, player_a}, {5, b_second}, {6, player_a}, {7, player_b},
           {8, player_a}, {9, b_first}, {10, player_a}});
}
void swap_one_byte(Cpu& c, const Squad& squad, std::uint32_t a, std::uint32_t b, std::uint32_t at) {
    const auto player_a = squad.player(a), player_b = squad.player(b);
    const auto from_a = c.u8(player_a + at), from_b = c.u8(player_b + at);
    c.put8(player_a + at, from_b);
    c.put8(player_b + at, from_a);
    c.set({{0, from_a}, {3, player_b}, {4, player_a}, {5, from_b}, {6, player_a}});
}
// The two ratings (+0x4 and +0x6 halfwords) exchange as the original does: A's +0x4 gets B's, A's +0x6 gets B's
// (read after the first store), then B gets what A had.
void swap_ratings(Cpu& c, const Squad& squad, std::uint32_t a, std::uint32_t b) {
    const auto player_a = squad.player(a), player_b = squad.player(b);
    const auto a_second = c.i16(player_a + player_fields::rating_second);
    const auto a_rating = c.i16(player_a + player_fields::rating);
    const auto b_rating = c.i16(player_b + player_fields::rating);
    c.put16(player_a + player_fields::rating, std::uint16_t(b_rating));
    const auto b_second = c.i16(player_b + player_fields::rating_second);
    c.put16(player_a + player_fields::rating_second, std::uint16_t(b_second));
    c.put16(player_b + player_fields::rating, std::uint16_t(a_rating));
    c.put16(player_b + player_fields::rating_second, std::uint16_t(a_second));
    c.set({{0, extended(a_second)}, {3, player_b}, {4, player_b}, {5, player_a}, {6, extended(b_second)},
           {7, player_a}, {8, extended(b_rating)}, {9, extended(a_rating)}, {10, player_a}});
}

// One trial of a club (0x100b3750-0x100b3be8). r24/r22 the two slots, r20/r19 their people.
void squad_trial(Cpu& c, const Pool& pool, std::uint32_t club) {
    constexpr std::uint32_t entry = 0x100b3710;
    const Squad squad{c, club};
    advance_with_scratch_3_0(c, pool, entry + 0x60);
    // Slot A, from the sample at the cursor; then the cursor moves on (a wrap re-salts after A's sum is taken).
    const auto cursor = c.u32(pool.cursor_at);
    const auto sample_a = c.u16(cursor);
    const auto sum_a = sample_a + c.u32(pool.salt_at);
    const auto end = c.u32(pool.end_at);
    const auto next = cursor + 2;
    const auto a = squad_index(c, sum_a);
    c.set(24, a);
    const auto person_a = c.u32(squad.slot(a) + club_row::squad);
    c.set(20, person_a);
    c.set({{0, 4 * a}, {3, next}, {4, squad.slot(a)}, {5, sum_a}, {6, sample_a}});
    const bool wrapped = compare_logical(c, next, end) > 0;
    c.put32(pool.cursor_at, next);
    if (wrapped) {
        c.put32(pool.cursor_at, pool.start);
        c.call(random_entry, entry + 0xc8);
        c.set(0, c.r(3) & 0xffff);
        c.put32(pool.salt_at, c.r(0));
    }
    // Slot B from the next sample.
    const auto sample_b = c.u16(c.u32(pool.cursor_at));
    const auto sum_b = sample_b + c.u32(pool.salt_at);
    c.set(4, sum_b);
    compare_logical(c, person_a);
    const auto b = squad_index(c, sum_b);
    c.set(22, b);
    c.set(0, 4 * b);
    c.set(3, squad.slot(b));
    const auto person_b = c.u32(squad.slot(b) + club_row::squad);
    c.set(19, person_b);
    if (person_a == 0) return;
    const auto player_a = load32(c, 0, person_a + person_fields::player);
    if (compare_logical(c, player_a) == 0) return;
    if (compare_logical(c, person_b) == 0) return;
    const auto player_b = load32(c, 0, person_b + person_fields::player);
    if (compare_logical(c, player_b) == 0) return;
    // Close ratings are only required when either person has its flag byte set.
    bool check_rating = compare_logical(c, load8(c, 0, person_a + person_fields::flag)) != 0;
    if (!check_rating) check_rating = compare_logical(c, load8(c, 0, person_b + person_fields::flag)) != 0;
    if (check_rating) {
        const auto rating_b = extended(c.i16(c.u32(person_b + person_fields::player) + player_fields::rating));
        const auto rating_a = extended(c.i16(c.u32(person_a + person_fields::player) + player_fields::rating));
        c.set({{4, rating_b}, {0, rating_a}, {3, rating_a - rating_b}});
        c.call(abs_entry, entry + 0x168);
        if (compare(c, std::int32_t(c.r(3)), 10) > 0) return;
    }
    // The low byte of the halfword at +0x5c (the byte at +0x5d, signed).
    const auto closeness_b = extsb(c.u16(person_b + person_fields::closeness));
    const auto closeness_a = extsb(c.u16(person_a + person_fields::closeness));
    c.set({{0, closeness_a}, {3, closeness_a - closeness_b}});
    c.call(abs_entry, entry + 0x188);
    if (compare(c, std::int32_t(c.r(3)), 4) > 0) return;
    if (parity_draw(c, pool, entry + 0x1b0)) swap_two_bytes(c, squad, a, b, 0x36, 0x1b);
    if (parity_draw(c, pool, entry + 0x29c)) swap_one_byte(c, squad, a, b, 0x3c);
    // The original tests the first player's byte twice (the same address both times).
    bool kind_is_20 = false;
    for (unsigned test = 0; test < 2 && !kind_is_20; ++test) {
        const auto player = squad.player(a);
        c.set(3, player);
        kind_is_20 = compare(c, std::int32_t(load8(c, 0, player + player_fields::kind)), 0x14) == 0;
    }
    if (!kind_is_20 && parity_draw(c, pool, entry + 0x370)) swap_two_bytes(c, squad, a, b, 0x2e, 0x2b);
    swap_ratings(c, squad, a, b);
}
}

// 0x100b3710. A frame of 0x70 saving r19-r31. r27 the club, r25 its index, r26 the trial, r21/r23 the counts'
// addresses (people, clubs), r28-r31 the pool's salt, start, end and cursor words.
void swap_player_attributes_within_clubs(Cpu& c) {
    constexpr std::uint32_t entry = 0x100b3710;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x70, {.first_gpr = 19});
    const auto clubs_at = c.toc(club_row::table_slot);
    c.set(3, clubs_at);
    c.set(25, 0);
    const auto pool = pool_from_toc(c);
    c.set({{28, pool.salt_at}, {29, pool.start}, {30, pool.end_at}, {31, pool.cursor_at}});
    const auto club_count_at = c.toc(club_row::count_slot);
    c.set(23, club_count_at);
    const auto people_count_at = c.toc(people_count_slot);
    c.set(21, people_count_at);
    auto club = load32(c, 27, clubs_at);
    for (std::uint32_t index = 0;; ++index, club += club_row::stride) {
        if (index != 0) {
            c.set(25, index);
            c.set(27, club);
        }
        if (compare(c, std::int32_t(index), std::int32_t(load32(c, 0, club_count_at))) >= 0) break;
        c.set(26, 0);
        for (std::uint32_t trial = 0;; ++trial) {
            if (trial != 0) c.set(26, trial);
            c.set(0, extsb(trial));
            if (compare(c, std::int32_t(std::int8_t(trial)), club_row::trials) >= 0) break;
            squad_trial(c, pool, club);
        }
    }
    const auto table_at = c.toc(people_table_slot);
    c.set(3, table_at);
    c.set(25, 0);
    auto person = load32(c, 19, table_at);
    for (std::uint32_t index = 0;; ++index, person += 0x70) {
        if (index != 0) {
            c.set(25, index);
            c.set(19, person);
        }
        if (compare(c, std::int32_t(index), std::int32_t(load32(c, 0, people_count_at))) >= 0) break;
        if (compare_logical(c, load32(c, 0, person + person_fields::player)) == 0) continue;
        guest::enter(c, entry + 0x524, {person});
        settle_player_attributes_weighted(c, person);
        guest::enter(c, entry + 0x52c, {person});
        settle_player_attributes(c, person);
    }
}

// ---- 0x100b3c70 ----
namespace {
namespace row_fields {  // rows of the people table (stride 0x70)
constexpr std::uint32_t used = 0xc, player = 0x14, first_clear = 4, second_clear = 8, closeness = 0x5c,
                        first_counter = 0x5e, second_counter = 0x5f;
}
namespace counter_player_fields {
constexpr std::uint32_t rating = 4, wide_rating = 8, kind_a = 0x14, kind_b = 0x15;
}

// A draw below a byte-sized `bound` once the pool has advanced, as the narrow arm leaves the registers: r3 the
// sum, r0 and `result_register` the remainder (cr0 compares the bound with 0). Bound 0 leaves 0 with nothing drawn.
std::uint32_t bounded_draw(Cpu& c, const Pool& pool, std::uint32_t bound, unsigned result_register,
                           std::uint32_t back) {
    advance_with_scratch_3_0(c, pool, back);
    const auto value = std::uint32_t(draw_below(c, pool, std::int32_t(bound), 3));
    c.set(0, value);
    c.set(result_register, value);
    return value;
}

// The three-way draw some sites make: sample plus salt, by 3 with the multiply-high reciprocal. Leaves the
// registers as the three copies do (they differ in which hold the quotient, the sum and the remainder).
struct ThreeWay {
    std::uint32_t sum, quotient, rounded, remainder;
};
ThreeWay three_way(std::uint32_t sum) {
    const auto quotient = std::uint32_t(mulhw(0x55555556, sum));
    const auto rounded = quotient + (quotient >> 31);
    return {sum, quotient, rounded, sum - rounded * 3};
}

struct Settler {
    Cpu& c;
    const Pool& pool;
    std::uint32_t row, date;
    static constexpr std::uint32_t entry = 0x100b3c70;

    std::uint32_t player() const { return c.u32(row + row_fields::player); }

    // `bl date_difference(row, date)`: the age's signed byte.
    std::int32_t age(std::uint32_t back) {
        guest::enter(c, entry + back, {row, date});
        date_difference(c, c.r(3), c.r(4));
        c.set(0, extsb(c.r(3)));
        return std::int8_t(c.r(3));
    }
    void clear_first_counter() {
        c.set(0, 0);
        c.put8(row + row_fields::first_counter, 0);
    }
    // The first counter, when it is set: it goes to 0 for a young age (several tiers), may be redrawn, or loses a draw.
    void settle_first_counter_active() {
        if (compare(c, age(0x6c), 0x14) < 0) return clear_first_counter();
        if (compare(c, age(0x90), 0x1e) < 0) {
            const auto rating = extended(c.i16(load32(c, 3, row + row_fields::player) + counter_player_fields::rating));
            c.set(0, rating);
            if (compare(c, std::int32_t(rating), 0x50) < 0) return clear_first_counter();
        }
        if (compare(c, age(0xc4), 0x1a) < 0) {
            const auto rating = extended(c.i16(load32(c, 3, row + row_fields::player) + counter_player_fields::wide_rating));
            c.set(0, rating);
            if (compare(c, std::int32_t(rating), 0x128e) < 0) return clear_first_counter();
        }
        if (compare(c, age(0xf8), 0x19) < 0 && compare_logical(c, load8(c, 0, row + row_fields::first_counter), 10) > 0) {
            // A counter above 10 is replaced by a draw below 5 (a signed byte of the remainder).
            advance_with_scratch_3_0(c, pool, entry + 0x130);
            const auto sample = c.u16(load32(c, 4, pool.cursor_at));
            const auto sum = sample + load32(c, 0, pool.salt_at);
            c.set(4, sum);
            const auto quotient = std::uint32_t(mulhw(0x66666667, sum));
            const auto shifted = srawi(c, quotient, 1);
            const auto rounded = shifted + (shifted >> 31);
            c.set(3, shifted >> 31);
            const auto remainder = extsb(sum - rounded * 5);
            c.set(0, remainder);
            c.put8(row + row_fields::first_counter, remainder);
            return;
        }
        const auto bound = load8(c, 21, row + row_fields::first_counter);
        compare(c, std::int32_t(bound));
        if (bound == 0) c.set(22, 0);
        else bounded_draw(c, pool, bound, 22, entry + 0x1a8);
        const auto counter = load8(c, 0, row + row_fields::first_counter);
        c.set(0, counter - c.r(22));
        c.put8(row + row_fields::first_counter, counter - c.r(22));
    }
    // The counter is 0: a strong player (rating over 140, closeness byte over 20) gets a counter of 0-2 one time in 4.
    void settle_first_counter_idle() {
        const auto rating = extended(c.i16(load32(c, 3, row + row_fields::player) + counter_player_fields::rating));
        c.set(0, rating);
        if (compare(c, std::int32_t(rating), 0x8c) <= 0) return;
        const auto closeness = extsb(c.u16(row + row_fields::closeness));
        c.set(0, closeness);
        if (compare(c, std::int32_t(closeness), 0x14) <= 0) return;
        advance_with_scratch_3_0(c, pool, entry + 0x334);
        const auto sample_at_cursor = c.u16(load32(c, 3, pool.cursor_at));
        const auto sum = sample_at_cursor + load32(c, 0, pool.salt_at);
        c.set(3, sum);
        const auto low_bits = std::rotl(sum, 30) & 0xc0000000u;
        const auto sign = sum >> 31;
        c.set(0, low_bits);
        c.set(3, sign);
        c.set(0, low_bits - sign);
        const auto result = std::rotl(low_bits - sign, 2) + sign;
        c.set(0, result);
        if (compare(c, std::int32_t(result)) != 0) return;
        advance_with_scratch_3_0(c, pool, entry + 0x384);
        const auto sample = c.u16(load32(c, 4, pool.cursor_at));
        load32(c, 0, pool.salt_at);
        const auto drawn = three_way(sample + c.r(0));
        c.set(4, drawn.sum);
        c.set(3, drawn.quotient);
        c.set(0, drawn.remainder);
        c.put8(row + row_fields::first_counter, drawn.remainder);
    }

    // The second counter, when it is set, against the first.
    void settle_second_counter() {
        if (compare_logical(c, load8(c, 0, row + row_fields::second_counter)) == 0) return;
        if (compare_logical(c, load8(c, 0, row + row_fields::first_counter)) == 0) {
            c.set(0, 0);
            c.put8(row + row_fields::second_counter, 0);
            return;
        }
        if (compare_logical(c, load8(c, 0, row + row_fields::second_counter), 4) > 0) {
            const auto level_b = load8(c, 0, load32(c, 3, row + row_fields::player) + counter_player_fields::kind_b);
            if (compare(c, std::int32_t(level_b), 0x14) != 0) {
                const auto level_a = load8(c, 0, load32(c, 3, row + row_fields::player) + counter_player_fields::kind_a);
                if (compare(c, std::int32_t(level_a), 0x14) != 0) {
                    c.set(0, 1);
                    c.put8(row + row_fields::second_counter, 1);
                    return;
                }
            }
        }
        const auto second = load8(c, 3, row + row_fields::second_counter);
        const auto first = load8(c, 0, row + row_fields::first_counter);
        if (compare_logical(c, second, first) > 0) {
            // The second counter is above the first: with the player's level 20 (+0x15) it becomes a draw below the first.
            const auto level = load8(c, 0, load32(c, 3, row + row_fields::player) + counter_player_fields::kind_b);
            if (compare(c, std::int32_t(level), 0x14) != 0) {
                c.set(0, 0);
                c.put8(row + row_fields::second_counter, 0);
                return;
            }
            const auto bound = load8(c, 22, row + row_fields::first_counter);
            compare(c, std::int32_t(bound));
            std::uint32_t value = 0;
            if (bound == 0) c.set(21, 0);
            else value = bounded_draw(c, pool, bound, 21, entry + 0x478);
            c.put8(row + row_fields::second_counter, value);
            return;
        }
        advance_with_scratch_3_0(c, pool, entry + 0x5dc);
        const auto cursor = load32(c, 4, pool.cursor_at);
        const auto salt = load32(c, 0, pool.salt_at);
        const auto sum = c.u16(cursor) + salt;
        c.set(4, sum);
        const auto drawn = three_way(sum);
        c.set(3, drawn.quotient);
        c.set(0, drawn.rounded);
        c.set(0, drawn.rounded * 3);
        c.set(0, drawn.remainder);
        if (compare(c, std::int32_t(drawn.remainder)) != 0) {
            // Not a multiple of 3: the second counter loses a draw below itself plus 1.
            const auto bound = load8(c, 22, row + row_fields::second_counter);
            compare(c, std::int32_t(bound));
            if (bound == 0) c.set(21, 0);
            else bounded_draw(c, pool, bound, 21, entry + 0x648);
            const auto counter = load8(c, 0, row + row_fields::second_counter);
            c.set(3, c.r(21) + 1);
            c.set(0, counter - c.r(3));
            c.put8(row + row_fields::second_counter, counter - c.r(3));
            return;
        }
        // A multiple of 3: the second counter grows by a draw of 0-2 plus 1.
        advance_with_scratch_3_0(c, pool, entry + 0x7b8);
        const auto cursor_value = load32(c, 5, pool.cursor_at);
        const auto salt_value = load32(c, 4, pool.salt_at);
        const auto grown_sum = c.u16(cursor_value) + salt_value;
        const auto counter = load8(c, 0, row + row_fields::second_counter);
        c.set(5, grown_sum);
        const auto grown = three_way(grown_sum);
        c.set(4, grown.quotient);
        c.set(3, grown.remainder + counter);
        c.set(0, grown.remainder + counter + 1);
        c.put8(row + row_fields::second_counter, grown.remainder + counter + 1);
    }

    // 0x100b446c's tail: a first counter of 0 clears the second as well; both words after the counters are zeroed.
    void finish() {
        if (compare_logical(c, load8(c, 0, row + row_fields::first_counter)) == 0) {
            c.set(0, 0);
            c.put8(row + row_fields::second_counter, 0);
        }
        c.set(3, 0);
        c.set(0, 0);
        c.put32(row + row_fields::first_clear, 0);
        c.put32(row + row_fields::second_clear, 0);
    }
    void settle() {
        if (compare_logical(c, load32(c, 0, row + row_fields::used)) == 0) return;
        if (compare_logical(c, load32(c, 0, row + row_fields::player)) == 0) return;
        if (compare_logical(c, load8(c, 0, row + row_fields::first_counter)) != 0) settle_first_counter_active();
        else settle_first_counter_idle();
        settle_second_counter();
        finish();
    }
};
}

// 0x100b3c70. A frame of 0x80 saving r21-r31. r31 the row, r25 its index, r23 the count's address, r24 the date
// (the TOC word's value, the second argument of every date_difference), r26-r30 the pool's end, start, salt and
// cursor words (r29 the salt, r27 the start, r26 the end, r30 the cursor), r28 the constant pool (unused:
// the floating-point arms cannot run); r21/r22 hold the last draw's bound and result.
void settle_person_counters(Cpu& c) {
    constexpr std::uint32_t entry = 0x100b3c70;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x80, {.first_gpr = 21});
    const auto table_at = c.toc(people_table_slot);
    c.set(3, table_at);
    c.set(25, 0);
    const auto date = c.toc(-0x7b80);
    c.set(24, date);
    const auto pool = pool_from_toc(c);
    c.set({{29, pool.salt_at}, {27, pool.start}, {26, pool.end_at}, {30, pool.cursor_at}});
    const auto count_at = c.toc(people_count_slot);
    c.set(23, count_at);
    c.set(28, c.toc(-0x5614));
    auto row = load32(c, 31, table_at);
    for (std::uint32_t index = 0;; ++index, row += 0x70) {
        if (index != 0) {
            c.set(25, index);
            c.set(31, row);
        }
        if (compare(c, std::int32_t(index), std::int32_t(load32(c, 0, count_at))) >= 0) break;
        Settler{c, pool, row, date}.settle();
    }
}
}
