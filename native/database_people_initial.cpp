// DATABASE.CPP's people block, part "initial": 0x100b2b80 (shuffle_person_rows), the shuffle of the generated
// people table (database_people_initial.hpp).
//
// The original is one frame of 0x180 bytes saving r18-r31. Registers it holds across its calls (each callee's
// prologue saves them into the stack image): r22 the count's address, r23 the first index (the count while it
// is drawn), r24 the number of trials, r25/r27/r28/r29 the pool's end, salt, start and cursor words, r26 the
// trial, r30 the table's address, r31 the constant pool, r21 the second index (the first draw's result while
// it is drawn), r20 the count of the second draw and later the first row, r18/r19 the two detail records and
// later the rows. Volatile registers are mirrored wherever a path can leave them: before a call and at the
// exits; the long swap sets only what its last writers leave (every path through it ends in the same writes).
#include "database_generation.hpp"
#include "database_generation_detail.hpp"
#include "database_lookups.hpp"
#include "database_people_detail.hpp"
#include "database_people_initial.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::database {
namespace {
namespace row = people::person_row;
using guest::compare;
using guest::compare_logical;
using people::extsb;
using people::load32;
using people::mulhw;

constexpr std::uint32_t entry = 0x100b2b80;
constexpr std::uint32_t date_copy = 0x10082c10, date_default = 0x10715880;

// Fields of a people row (0x70 bytes) beyond those of person_row; meanings inferred from the uses.
constexpr std::uint32_t row_player = 0x14;   // the player record (its halfwords at +0x4 and +0x6 swap back)
constexpr std::uint32_t row_staff = 0x1c;    // a record that is present for some rows only
constexpr std::uint32_t row_detail = 0x24;   // a record whose +0x2c (a club) and +0x30 words decide the gates
constexpr std::uint32_t detail_club = 0x2c, detail_other = 0x30;
constexpr std::uint32_t first_date = 0x34, date_step = 8, date_count = 5;  // five 5-byte dates
constexpr std::uint32_t tail_half = 0x5c, tail_bytes = 0x5e, tail_byte_count = 16;
constexpr std::uint32_t head_words = 13;     // the words +0x0..+0x30 that move with a swap
constexpr std::uint32_t trials_factor_slot = 0x20;  // the constant pool's +0x20: 16.0, a double

// The frame's locals (offsets from the new stack pointer): the dates the trial copies through, the first
// row's words, the first row's dates (the scratch the date copy passes through) and its tail.
namespace local {
constexpr std::uint32_t copy_of_first = 0x88, copy_of_second = 0x60, copy_back = 0x38;  // 5 dates each, 8 apart
constexpr std::uint32_t scratch_dates = 0xec;                                          // 5 dates, 8 apart
constexpr std::uint32_t spare_date = 0xb0, saved_words = 0xb8, saved_half = 0x114, saved_bytes = 0x116;
constexpr std::uint32_t count_double = 0x138, trial_count = 0x130;
}

constexpr std::uint32_t conversion_high = 0x43300000, reciprocal = 0x80008001, word_range = 0xffff;

// One shuffle: the registers and the frame its methods share.
class Shuffle {
public:
    Shuffle(Cpu& cpu, guest::Frame& f, const generation::Pool& p, std::uint32_t table, std::uint32_t count)
        : c(cpu), frame(f), pool(p), table_at(table), count_at(count) {}

    void run();

private:
    // Where a draw keeps its frame words (offsets of the two conversion doubles and of the result).
    struct IndexDraw {
        unsigned bound_register;
        std::uint32_t pool_back, range_back;  // return addresses of the pool's random call and range_sample
        std::uint32_t first_double, second_double, result;
    };
    static constexpr IndexDraw first_draw{23, entry + 0xcc, entry + 0x1a4, 0x130, 0x138, 0x128};
    static constexpr IndexDraw second_draw{20, entry + 0x240, entry + 0x318, 0x128, 0x130, 0x138};

    std::uint32_t table() { return c.u32(table_at); }  // lwz rX,0(r30)
    std::uint32_t row_at(std::uint32_t index) { return table() + index * row::stride; }
    // mulli r0,index,0x70; lwz r3,0(r30); add r3,r3,r0; lwz r0,offset(r3)
    std::uint32_t row_field(std::uint32_t index, std::uint32_t offset) {
        c.set(0, index * row::stride);
        c.set(3, table());
        c.set(3, c.r(3) + c.r(0));
        return load32(c, 0, c.r(3) + offset);
    }
    bool listed_staff(std::uint32_t index) { return compare_logical(c, row_field(index, row_staff)) != 0; }

    std::uint32_t draw_index(const IndexDraw& draw, std::int32_t bound);
    std::uint32_t wide_index(const IndexDraw& draw, std::int32_t bound);
    bool may_exchange(std::uint32_t first, std::uint32_t second);
    void exchange(std::uint32_t first, std::uint32_t second);
    void date_copy_through(std::uint32_t to, std::uint32_t through, std::uint32_t from, std::uint32_t back) {
        c.set({{3, to}, {4, through}, {5, from}});
        c.call(date_copy, back);
    }
    Cpu& c;
    guest::Frame& frame;
    const generation::Pool& pool;
    std::uint32_t table_at, count_at;
};

// The draw's wide arm (a count beyond +-65535): the sample plus salt, reduced modulo 65535, scales
// count / 65535 in floating point (the result in r21); the next level is range_sample(count * 10 / 65535),
// whose answer adds (answer + 5) / 10; the index is clamped to count - 1. Leaves the registers and
// f0-f2 as the compiler did; XER[CA] comes from the two srawi.
std::uint32_t Shuffle::wide_index(const IndexDraw& draw, std::int32_t bound) {
    using guest::Fpu;
    const auto count = std::uint32_t(bound);
    Fpu fpu(c);
    // The pool's sample plus salt, reduced modulo 65535 (multiply-high by the reciprocal, then a shift).
    const auto sample = c.u16(c.u32(pool.cursor_at));
    const auto sum = sample + c.u32(pool.salt_at);
    const auto shifted = generation::srawi(c, mulhw(reciprocal, sum) + sum, 15);
    const auto remainder = sum - (shifted + (shifted >> 31)) * word_range;
    // count and remainder as doubles through the frame (high word 0x43300000, the signed word biased by
    // 0x80000000; the constant pool's +0x18 is the bias to subtract): count / divisor * remainder.
    const auto bias = c.f64(pool.constants + 0x18), divisor = c.f64(pool.constants + 0x10);
    const auto count_double = frame.at(draw.first_double), remainder_double = frame.at(draw.second_double);
    c.put32(count_double + 4, count ^ 0x80000000u);
    c.put32(count_double, conversion_high);
    const auto count_scaled = fpu.fdiv(fpu.fsub(c.f64(count_double), bias), divisor);
    c.put32(remainder_double, conversion_high);
    c.put32(remainder_double + 4, remainder ^ 0x80000000u);
    const auto next_bound = mulhw(reciprocal, count * 10) + count * 10;
    const auto next_shifted = generation::srawi(c, next_bound, 15);
    const auto fraction = fpu.fsub(c.f64(remainder_double), bias);
    c.set_f(2, count_scaled);
    c.set_f(1, bias);
    c.set_f(0, fpu.fmul(count_scaled, fraction));
    c.put64(frame.at(draw.result), fpu.fctiwz(0, c.f(0)));
    c.set(21, c.u32(frame.at(draw.result) + 4));
    // The next level, range_sample(count * 10 / 65535). What it and the later code see in r3-r8 and r0.
    c.set({{3, next_shifted + (next_shifted >> 31)}, {4, remainder ^ 0x80000000u}, {5, word_range},
           {6, shifted >> 31}, {7, sum}, {8, sample}, {0, next_shifted}});
    guest::enter(c, draw.range_back, {c.r(3)});
    range_sample(c, std::int32_t(c.r(3)));
    // (answer + 5) / 10 by multiply-high, added to the scaled index; the index is clamped to count - 1.
    const auto tenth = generation::srawi(c, mulhw(0x66666667, c.r(3) + 5), 2);
    c.set({{3, tenth + (tenth >> 31)}, {4, tenth >> 31}, {0, count - 1}});
    c.set(21, c.r(21) + c.r(3));
    if (compare(c, std::int32_t(c.r(21)), std::int32_t(count - 1)) > 0) c.set(21, count - 1);
    return c.r(21);
}

// An index below the count held in `draw.bound_register` (not 0): the pool advances (a wrap draws), then
// |(sample + salt) % count| for a count within +-65535 (the remainder takes the sign of the sum, negated for a
// negative count: a negative index is possible), else the wide arm. The result is in r21 (and r0 when narrow).
std::uint32_t Shuffle::draw_index(const IndexDraw& draw, std::int32_t bound) {
    generation::advance(c, pool, draw.pool_back);
    c.set(3, generation::narrow_limit);
    bool narrow = compare(c, bound, generation::narrow_limit) <= 0;
    if (narrow) {
        c.set(3, 0xffff0001);
        narrow = compare(c, bound, -generation::narrow_limit) >= 0;
    }
    if (!narrow) return wide_index(draw, bound);
    const auto drawn = std::uint32_t(generation::draw_below(c, pool, bound, 3));
    c.set(0, drawn);
    c.set(21, drawn);
    return drawn;
}

// The gates a pair must pass before its rows swap; leaves the registers of whichever test stops it.
bool Shuffle::may_exchange(std::uint32_t first, std::uint32_t second) {
    compare(c, std::int32_t(first), std::int32_t(second));
    if (first == second) return false;
    for (const auto index : {first, second})
        if (compare_logical(c, row_field(index, row_player)) == 0) return false;
    // Both rows or neither have a staff record.
    bool check_both = listed_staff(first);
    if (!check_both) check_both = listed_staff(second);
    if (check_both && (!listed_staff(first) || !listed_staff(second))) return false;
    // The detail records (r18, r19).
    c.set(0, first * row::stride);
    c.set(4, table());
    c.set(3, table());
    c.set(4, c.r(4) + c.r(0));
    c.set(0, second * row::stride);
    const auto first_detail = load32(c, 18, c.r(4) + row_detail);
    compare_logical(c, first_detail);
    c.set(3, c.r(3) + c.r(0));
    const auto second_detail = load32(c, 19, c.r(3) + row_detail);
    if (first_detail == 0) return false;
    if (compare_logical(c, second_detail) == 0) return false;
    // Rows whose detail records share both words skip the gates.
    auto left = load32(c, 3, first_detail + detail_club);
    auto right = load32(c, 0, second_detail + detail_club);
    compare_logical(c, left, right);
    bool shared = left == right;
    if (shared) {
        left = load32(c, 3, first_detail + detail_other);
        right = load32(c, 0, second_detail + detail_other);
        compare_logical(c, left, right);
        shared = left == right;
    }
    if (shared) return true;
    // person_exempt_from_club_rule (club 0) must answer alike for both (compared as signed bytes).
    c.set(0, second * row::stride);
    c.set(3, table());
    c.set(4, 0);
    c.set(3, c.r(3) + c.r(0));
    guest::enter(c, entry + 0x478, {c.r(3), 0});
    person_exempt_from_club_rule(c, c.r(3), 0);
    c.set(0, first * row::stride);
    c.set(5, table());
    c.set(20, extsb(c.r(3)));
    c.set(4, 0);
    c.set(3, c.r(5) + c.r(0));
    guest::enter(c, entry + 0x490, {c.r(3), 0});
    person_exempt_from_club_rule(c, c.r(3), 0);
    c.set(0, extsb(c.r(3)));
    if (compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(20))) != 0) return false;
    // person_club_gate (flag 1): each person must be let into the club of their OWN detail record (+0x2c).
    const std::uint32_t order[2][3] = {{first, 18, entry + 0x4b4}, {second, 19, entry + 0x4d4}};
    for (const auto& [index, detail_register, back] : order) {
        c.set(0, index * row::stride);
        c.set(3, table());
        c.set(4, c.u32(c.r(detail_register) + detail_club));
        c.set(5, 1);
        c.set(3, c.r(3) + c.r(0));
        guest::enter(c, back, {c.r(3), c.r(4), 1});
        person_club_gate(c, c.r(3), c.r(4), 1);
        c.set(0, extsb(c.r(3)));
        if (compare(c, std::int32_t(c.r(0))) == 0) return false;
    }
    return true;
}

// The swap of the two rows' contents. The locals keep the first row whole, the dates go through
// 0x10082c10 (source to scratch to destination), and what stays with the position is the first word and the
// detail pointer.
void Shuffle::exchange(std::uint32_t first, std::uint32_t second) {
    const auto first_offset = first * row::stride, second_offset = second * row::stride;
    // The first row's words, dates (copied through the scratch dates) and tail go to the frame.
    const auto first_row = row_at(first);
    c.set(20, first_row);
    for (std::uint32_t word = 0; word < head_words; ++word)
        c.put32(frame.at(local::saved_words) + 4 * word, c.u32(first_row + 4 * word));
    for (std::uint32_t n = 0; n < date_count; ++n)
        date_copy_through(frame.at(local::copy_of_first + date_step * n), frame.at(local::scratch_dates + date_step * n),
                          first_row + first_date + date_step * n, entry + 0x560 + 0x10 * n);
    // The second row (r18) overwrites the first (r19), then the saved first row goes to the second.
    c.set(18, row_at(second));
    c.set(19, row_at(first));
    c.put16(frame.at(local::saved_half), c.u16(first_row + tail_half));
    for (std::uint32_t n = 0; n < tail_byte_count; ++n)
        c.put8(frame.at(local::saved_bytes) + n, c.u8(first_row + tail_bytes + n));
    for (std::uint32_t word = 0; word < head_words; ++word) c.put32(c.r(19) + 4 * word, c.u32(c.r(18) + 4 * word));
    for (std::uint32_t n = 0; n < date_count; ++n)
        date_copy_through(frame.at(local::copy_of_second + date_step * n), c.r(19) + first_date + date_step * n,
                          c.r(18) + first_date + date_step * n, entry + 0x6b8 + 0x10 * n);
    c.put16(c.r(19) + tail_half, c.u16(c.r(18) + tail_half));
    for (std::uint32_t n = 0; n < tail_byte_count; ++n) c.put8(c.r(19) + tail_bytes + n, c.u8(c.r(18) + tail_bytes + n));
    c.set(18, row_at(second));
    for (std::uint32_t word = 0; word < head_words; ++word)
        c.put32(c.r(18) + 4 * word, c.u32(frame.at(local::saved_words) + 4 * word));
    for (std::uint32_t n = 0; n < date_count; ++n)
        date_copy_through(frame.at(local::copy_back + date_step * n), c.r(18) + first_date + date_step * n,
                          frame.at(local::scratch_dates + date_step * n), entry + 0x804 + 0x10 * n);
    c.put16(c.r(18) + tail_half, c.u16(frame.at(local::saved_half)));
    for (std::uint32_t n = 0; n < tail_byte_count; ++n)
        c.put8(c.r(18) + tail_bytes + n, c.u8(frame.at(local::saved_bytes) + n));

    // The first word and the detail pointer stay with the position: swap them back (r9-r11 keep the words moved).
    const auto first_at = row_at(first), second_at = row_at(second);
    const auto first_head = c.u32(first_at), second_head = c.u32(second_at);
    c.put32(first_at, second_head);
    c.put32(second_at, first_head);
    const auto first_detail = c.u32(first_at + row_detail), second_detail = c.u32(second_at + row_detail);
    c.put32(first_at + row_detail, second_detail);
    c.put32(second_at + row_detail, first_detail);
    c.set({{9, second_head}, {10, first_head}, {11, table()}});

    // A pool sample's parity: (sum & 1 ^ sign) - sign, non-zero for an odd sum.
    generation::advance(c, pool, entry + 0x95c);
    const auto sum = c.u16(c.u32(pool.cursor_at)) + c.u32(pool.salt_at);
    const auto sign = sum >> 31;
    if (compare(c, std::int32_t(((sum & 1) ^ sign) - sign)) != 0) {
        // The first dates swap again (halfword, halfword, byte) through the spare date.
        const auto a = row_at(first) + first_date, b = row_at(second) + first_date;
        const auto spare = frame.at(local::spare_date);
        c.put16(spare + 2, c.u16(a + 2));
        c.put16(spare, c.u16(a));
        c.put8(spare + 4, c.u8(a + 4));
        c.put16(a + 2, c.u16(b + 2));
        c.put16(a, c.u16(b));
        c.put8(a + 4, c.u8(b + 4));
        c.put16(b + 2, c.u16(spare + 2));
        c.put16(b, c.u16(spare));
        c.put8(b + 4, c.u8(spare + 4));
    }

    // The player records' halfwords at +0x4 and +0x6 swap back (the rows' player pointers have just moved).
    const auto player = [&](std::uint32_t offset) { return c.u32(table() + offset + row_player); };
    const auto first_player = [&] { return player(first_offset); };
    const auto second_player = [&] { return player(second_offset); };
    const auto s16 = [&](std::uint32_t at) { return std::uint32_t(std::int32_t(c.i16(at))); };
    const auto first_four = s16(first_player() + 4);
    const auto first_six = s16(first_player() + 6);
    const auto second_four = s16(second_player() + 4);
    c.put16(first_player() + 4, second_four);
    const auto second_six = s16(second_player() + 6);
    c.put16(first_player() + 6, second_six);
    c.put16(second_player() + 4, first_four);
    c.put16(second_player() + 6, first_six);
    c.set({{0, first_six}, {3, second_player()}, {4, second_offset}, {5, second_player()}, {6, table()},
           {7, second_six}, {8, table()}});
}

void Shuffle::run() {
    // Six default dates in the frame (5-byte records): the five scratch dates and the spare.
    for (std::uint32_t n = 0; n < date_count; ++n) {
        c.set(3, frame.at(local::scratch_dates + date_step * n));
        c.call(date_default, entry + 0x34 + 8 * n);
    }
    c.set(3, frame.at(local::spare_date));
    c.call(date_default, entry + 0x5c);

    // The number of trials: trunc(16.0 * count), through the conversion double on the stack.
    guest::Fpu fpu(c);
    const auto count = c.u32(count_at);
    c.set({{3, count}, {0, conversion_high}, {26, 0}});
    c.put32(frame.at(local::count_double), c.r(0));
    c.set(0, count ^ 0x80000000u);
    c.set_f(1, c.f64(pool.constants + 0x18));
    c.put32(frame.at(local::count_double) + 4, c.r(0));
    c.set_f(2, c.f64(pool.constants + trials_factor_slot));
    c.set_f(0, c.f64(frame.at(local::count_double)));
    c.set_f(0, fpu.fsub(c.f(0), c.f(1)));
    c.set_f(0, fpu.fmul(c.f(2), c.f(0)));
    c.put64(frame.at(local::trial_count), fpu.fctiwz(0, c.f(0)));
    const auto trials = std::int32_t(c.u32(frame.at(local::trial_count) + 4));
    c.set(24, std::uint32_t(trials));

    for (std::uint32_t trial = 0;; ++trial) {
        if (compare(c, std::int32_t(trial), trials) >= 0) break;
        // First index below the count (0 for a count of 0).
        const auto first_count = load32(c, 23, count_at);
        compare(c, std::int32_t(first_count));
        std::uint32_t first = 0, second = 0;
        if (first_count == 0) {
            c.set(21, 0);
        } else {
            first = draw_index(first_draw, std::int32_t(first_count));
        }
        const auto second_count = load32(c, 20, count_at);
        c.set(23, first);
        compare(c, std::int32_t(second_count));
        if (second_count == 0) {
            c.set(21, 0);
        } else {
            second = draw_index(second_draw, std::int32_t(second_count));
        }
        if (may_exchange(first, second)) exchange(first, second);
        c.set(26, trial + 1);
    }
}
}

// 0x100b2b80. The loop above is the original's: each trial reads the count anew.
void shuffle_person_rows(Cpu& c) {
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x180, {.first_gpr = 18});
    const auto pool = generation::load_pool(c);
    const auto table_at = c.toc(row::table_slot), count_at = c.toc(row::count_slot);
    guest::keep(c, {{27, pool.salt_at}, {28, pool.start}, {25, pool.end_at}, {29, pool.cursor_at}, {30, table_at},
                    {22, count_at}, {31, pool.constants}});
    Shuffle(c, frame, pool, table_at, count_at).run();
}
}
