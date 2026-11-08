// MATCH_MAN.CPP selection block (match_man_selection.hpp): the rating and the picks of one place.
#include "match_man_selection.hpp"
#include "match_man_selection_detail.hpp"
#include "match_man_adapter.hpp"
#include "guest_values.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::match_man {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Fpu;
using guest::Frame;
using guest::Kept;
using guest::nonzero_logical;

constexpr std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }
constexpr std::int32_t s16(std::uint32_t value) { return std::int16_t(value); }
constexpr std::uint32_t reg(std::int32_t value) { return std::uint32_t(value); }

constexpr std::uint32_t exponent_word = 0x43300000;
double raw_double(std::uint32_t low) { return std::bit_cast<double>(std::uint64_t(exponent_word) << 32 | low); }
// The compiler's int-to-double idiom: the words 0x43300000 and `low` stored at
// `at` and `at + 4` (the stack image keeps them), loaded as a double.
double stored_double(Cpu& c, std::uint32_t at, std::uint32_t low) {
    c.put32(at, exponent_word);
    c.put32(at + 4, low);
    return raw_double(low);
}
// fctiwz of `value` into FPR `n` and the stfd of those bits at `at`; the low
// word is what the original reads back.
std::uint32_t truncated_into(Cpu& c, Fpu& fpu, unsigned n, std::uint32_t at, double value) {
    const auto bits = fpu.fctiwz(n, value);
    c.put64(at, bits);
    return std::uint32_t(bits);
}
// The original's mulhw by 0x66666667, srawi 3 and the sign added back, then
// mulli 20 and subf: the remainder of x over 20, truncating (the quotient is
// x / 20 for every int32, checked). Also the sign bit the original leaves in r3.
struct Twenty { std::int32_t remainder; std::uint32_t sign; };
Twenty over_twenty(std::int32_t x) {
    const std::int32_t high = std::int32_t((std::int64_t(0x66666667) * x) >> 32) >> 3;
    const std::uint32_t sign = std::uint32_t(high) >> 31;
    const std::int32_t quotient = high + std::int32_t(sign);
    return {x - quotient * 20, sign};
}
// 0x100ca010 and 0x100c7d00 (DATABASE.CPP), 0x10082f90 (CONTRACT_MANAGER.CPP), 0x10718130 (DATE.CPP).
constexpr std::uint32_t first_rating = 0x100ca010, club_flag = 0x100c7d00, contract_entry = 0x10082f90, date_difference = 0x10718130;
constexpr std::uint32_t selection_rating_entry = 0x10210800;
}

// 0x10210800. The pool offsets carry no meaning the code shows; the age bands
// (18, 21, 24, 26) and the limits are the original's.
std::int32_t selection_rating(Cpu& c, std::uint32_t person, std::uint32_t match_player, std::uint32_t club) {
    constexpr std::uint32_t entry = 0x10210800;
    // r29-r31 at -0x2c.. and f28-f31 at -0x20.. (the original's save helper), LR at +8.
    Frame frame(c, entry, 0xa0, {.first_gpr = 29, .gpr_offset = -0x2c, .first_fpr = 28, .fpr_offset = -0x20});
    Fpu fpu(c);
    const auto pool_address = c.toc(-0x5278);
    const auto k = [&](std::uint32_t offset) { return c.f64(pool_address + offset); };
    Kept<31> pool_register(c, pool_address);
    Kept<30> club_register(c, club);
    Kept<29> subject(c, person);
    double f29 = k(0x0), f28 = k(0x178);
    c.set_f(29, f29);
    c.set_f(28, f28);
    if (nonzero_logical(c, match_player)) subject = c.u32(match_player + match_player::person);
    if (!nonzero_logical(c, subject)) return guest::result(c, reg(-10000));

    // The first rating (0x100ca010, DATABASE.CPP) of the person for the club, as a signed byte.
    c.set({{3, subject}, {4, club}});
    c.call(first_rating, 0x1021085c);
    const std::int32_t first = s8(c.r(3));
    const auto data = c.u32(subject + person::player);
    const std::int32_t form = c.i16(club + selection_club::form);
    const std::int32_t difference = std::int32_t(c.i16(data + selection_data::ability)) - form;
    c.set({{3, reg(form)}, {4, reg(first) ^ 0x80000000u}, {5, exponent_word}, {6, reg(first)}});
    const double f3 = k(0x90);
    // f1 = (difference - 2^52 - 2^31) / pool, plus a pool value: the clamped argument f31.
    double f1 = fpu.fsub(stored_double(c, frame.at(0x58), reg(difference) ^ 0x80000000u), f3);
    double f0 = fpu.fdiv(f1, k(0x2b0));
    f0 = fpu.fadd(k(0x20), f0);
    f1 = stored_double(c, frame.at(0x60), reg(first) ^ 0x80000000u);
    const double f30 = fpu.fsub(f1, f3);
    c.set_f(30, f30);
    // f31 = f0 clamped into [pool 0x2c0, pool 0x2b8]: fcmpo, ble, fcmpo, bge (a NaN falls through to f0).
    double f31 = k(0x2b8);
    fpu.fcmp(0, f31, f0);
    if (!(f31 > f0)) {
        f31 = k(0x2c0);
        fpu.fcmp(0, f31, f0);
        if (!(f31 < f0)) f31 = f0;
    }
    c.set_f(31, f31);

    // The person's contract (0x10082f90, CONTRACT_MANAGER.CPP): a signed byte at +0x4c scaled, then
    // the low nibble of +0x60 picks an adjustment (the jump table at 0x108cd0fc: cases 0..8).
    c.set({{3, c.u32(c.toc(-0x7b7c))}, {4, subject}, {5, club}, {6, reg(first)}});
    c.call(contract_entry, 0x102108e8);
    const auto contract = c.r(3);
    if (nonzero_logical(c, contract)) {
        const auto byte_4c = c.u8(contract + 0x4c);
        const auto flags = c.u8(contract + 0x60) & 0xf;
        c.set({{5, byte_4c}, {4, exponent_word}, {3, reg(s8(byte_4c)) ^ 0x80000000u}});
        f1 = fpu.fsub(stored_double(c, frame.at(0x58), reg(s8(byte_4c)) ^ 0x80000000u), k(0x90));
        f28 = fpu.fadd(k(0x210), fpu.fdiv(f1, k(0x1c8)));
        compare_logical(c, flags, 8);
        if (flags <= 8) {
            switch (flags) {
                case 0: f28 = fpu.fsub(f28, k(0x88)); break;
                case 1: f28 = fpu.fadd(f28, k(0x80)); break;
                case 2: f28 = fpu.fadd(f28, k(0x78)); break;
                case 3: break;
                case 4: f28 = fpu.fsub(f28, k(0x268)); break;
                case 5: f28 = fpu.fsub(f28, k(0x88)); break;
                case 6: f28 = fpu.fsub(f28, k(0x8)); break;
                case 7: f28 = fpu.fsub(f28, k(0xc0)); break;
                case 8: f28 = fpu.fsub(f28, k(0x1b8)); break;
            }
        }
    }
    c.set_f(28, f28);

    // The club's flag picks the person's byte +0x5e scaled, else the day count from 0x10718130.
    c.set(3, club);
    c.call(club_flag, 0x102109ac);
    double span;  // f0 of the original from here on
    if (compare(c, s8(c.r(3))) != 0) {
        const auto byte_5e = c.u8(subject + selection_person::byte_5e);
        c.set(3, byte_5e);
        f1 = k(0x2e0);
        span = fpu.fmul(k(0x1c8), fpu.fsub(stored_double(c, frame.at(0x58), byte_5e), f1));
    } else {
        c.set({{3, c.toc(-0x7b80)}, {4, c.u32(subject + selection_person::date_low)}, {5, c.u32(subject + selection_person::date_high)}});
        c.call(date_difference, 0x102109ec);
        const auto days = c.r(3) ^ 0x80000000u;
        c.set(3, days);
        f1 = k(0x90);
        span = fpu.fsub(stored_double(c, frame.at(0x60), days), f1);
    }
    f1 = k(0x2c8);
    fpu.fcmp(0, span, f1);
    if (span > f1) {
        const double over = fpu.fsub(span, f1);
        f1 = fpu.fdiv(over, k(0x2d0));
        f29 = fpu.fadd(f29, f1);
        c.set_f(29, f29);
    }
    const double square_plus = fpu.multiply_add(0x10210a30, f30, f30, k(0x100));
    const auto byte_2f = s8(c.u8(c.u32(subject + person::player) + selection_data::byte_2f));
    const std::int32_t age = s8(c.u16(subject + selection_person::age));
    c.set({{4, exponent_word}, {3, reg(byte_2f) ^ 0x80000000u}});
    const double scale = fpu.fdiv(square_plus, k(0x88));
    const double f4 = k(0x90);
    const double base = fpu.fsub(stored_double(c, frame.at(0x50), reg(byte_2f) ^ 0x80000000u), f4);
    compare(c, age, 0x12);
    const double f2 = k(0x210);
    double total = fpu.multiply_add(0x10210a6c, base, f29, scale);
    total = fpu.fadd(f31, total);
    total = fpu.fadd(f28, total);
    total = fpu.fadd(f2, total);
    std::uint32_t value = truncated_into(c, fpu, 1, frame.at(0x48), total);
    // Age bands scale the value again: under 18, 18-20, 21-23, and 26 or more (24 and 25 keep it).
    const auto scaled = [&](double factor) {
        const double shown = fpu.fsub(stored_double(c, frame.at(0x48), reg(s16(value)) ^ 0x80000000u), f4);
        value = truncated_into(c, fpu, 1, frame.at(0x50), fpu.fmul(factor, shown));
    };
    if (age < 0x12) scaled(f2);
    else if (compare(c, age, 0x15) < 0) scaled(k(0x190));
    else if (compare(c, age, 0x18) < 0) scaled(k(0x198));
    else if (compare(c, age, 0x19) > 0) scaled(k(0x2d8));
    // The limit 500 - 10 * (byte +0x65) against the span: under it, the value is scaled by pool 0x198.
    const std::int32_t limit_byte = s8(c.u8(subject + selection_person::byte_65));
    const double limit = fpu.fsub(stored_double(c, frame.at(0x48), reg(500 - limit_byte * 10) ^ 0x80000000u), f4);
    fpu.fcmp(0, span, limit);
    f1 = limit;
    if (span < limit) {
        f1 = k(0x198);
        const double shown = fpu.fsub(stored_double(c, frame.at(0x50), reg(s16(value)) ^ 0x80000000u), f4);
        value = truncated_into(c, fpu, 0, frame.at(0x58), fpu.fmul(f1, shown));
    }
    // A byte under 10 stretches the value by 1 + byte / pool 0x2a8 (the original's second conversion).
    const std::int32_t stretch_byte = s8(c.u8(subject + selection_person::byte_67));
    if (compare(c, stretch_byte, 10) < 0) {
        const double f3b = k(0x90);
        const double byte_value = fpu.fsub(stored_double(c, frame.at(0x40), reg(stretch_byte) ^ 0x80000000u), f3b);
        const double shown_b = fpu.fdiv(byte_value, k(0x2a8));
        f1 = fpu.fsub(stored_double(c, frame.at(0x60), reg(s16(value)) ^ 0x80000000u), f3b);
        const double adjusted = fpu.fadd(k(0x198), shown_b);
        value = truncated_into(c, fpu, 0, frame.at(0x38), fpu.fmul(f1, adjusted));
    }
    c.set_f(1, f1);
    return guest::result(c, value);
}

// 0x10210430. Left as the original leaves them: r3 the index, r4-r10 what the last
// rating left, cr0 the last loop test (the index against 20).
std::int32_t best_player_index(Cpu& c, std::uint32_t side) {
    constexpr std::uint32_t entry = 0x10210430;
    Frame frame(c, entry, 0x50, {.first_gpr = 27});
    Kept<27> side_register(c, side);
    Kept<30> best(c, reg(-1));        // index of the best player so far
    Kept<29> best_rating(c, 0);
    Kept<31> index(c, 0);
    Kept<28> player(c, c.u32(side + selection_side::players));
    for (;;) {
        if (compare(c, s8(index), match_side::player_count) >= 0) break;
        if (compare(c, s8(c.u8(player + selection_player::unavailable))) >= 0) {
            c.set({{5, c.u32(side + selection_side::club)}, {4, player}, {3, 0}});
            match_man::call(c, selection_rating_entry, 0x10210474);
            const std::int32_t rating = s16(c.r(3));
            c.set(3, reg(rating));
            if (compare(c, s8(best), -1) == 0 || compare(c, rating, s16(best_rating)) > 0) {
                best_rating = reg(rating);
                best = index;
            }
        }
        index = index + 1;
        player = player + match_player::size;
    }
    return std::int32_t(guest::result(c, best));
}

// 0x102102d0. r3 the best person (r4-r6 as the last iteration leaves them).
std::uint32_t best_person(Cpu& c, std::uint32_t side, std::uint32_t people, std::int8_t count_byte) {
    constexpr std::uint32_t entry = 0x102102d0;
    Frame frame(c, entry, 0x80, {.first_gpr = 23});
    Fpu fpu(c);
    const std::uint32_t count = reg(std::int32_t(count_byte));  // extsb r23
    const auto pool_address = c.toc(-0x5278);
    const auto k = [&](std::uint32_t offset) { return c.f64(pool_address + offset); };
    Kept<25> side_register(c, side);
    Kept<24> list(c, people);
    Kept<23> total(c, count);
    Kept<31> pool_register(c, pool_address);
    Kept<30> best(c, 0);               // the best person so far
    Kept<26> best_score(c, 0);
    Kept<28> index(c, 0);
    Kept<29> candidate(c, c.r(29));
    Kept<27> score(c, c.r(27));
    for (;;) {
        if (compare(c, s8(index), std::int32_t(count)) >= 0) break;
        candidate = c.u32(list);
        if (nonzero_logical(c, candidate) && nonzero_logical(c, c.u32(candidate + person::player))) {
            c.set({{5, c.u32(side + selection_side::club)}, {3, candidate}, {4, 0}});
            match_man::call(c, selection_rating_entry, 0x10210328);
            score = reg(s16(c.r(3)));
            const auto staff = c.u32(side + selection_side::manager_staff);
            c.set(5, staff);
            if (nonzero_logical(c, staff)) {
                c.set({{4, c.u8(staff + staff_data::judging_current)}, {3, staff}});
                c.call(selection_calls::attribute_rating, 0x10210344);
                const std::int32_t rating = s8(c.r(3));
                const std::int32_t id = c.i32(candidate + selection_person::id);
                const auto by_id = over_twenty(id);
                c.set({{4, reg(rating)}, {5, reg(id)}, {6, 0x66666667}, {3, by_id.sign}});
                if (compare(c, rating, by_id.remainder) < 0) {
                    // The staff pointer itself, as a signed number, is rolled the same way (the original's choice).
                    const std::int32_t seed = c.i32(side + selection_side::manager_staff);
                    const auto by_seed = over_twenty(seed);
                    c.set({{4, reg(seed)}, {3, by_seed.sign}, {5, score ^ 0x80000000u}, {6, 0x66666667}});
                    const double conversion = k(0x90);
                    const double exact = fpu.fsub(stored_double(c, frame.at(0x48), score ^ 0x80000000u), conversion);
                    const double roll = fpu.fsubs(stored_double(c, frame.at(0x40), reg(by_seed.remainder) ^ 0x80000000u), conversion);
                    c.put32(frame.at(0x44), reg(by_seed.remainder) ^ 0x80000000u);
                    c.set_f(1, roll);
                    double f0 = fpu.fdiv(roll, k(0x2a8));
                    f0 = fpu.fsub(k(0x40), f0);
                    f0 = fpu.fmul(exact, f0);
                    score = truncated_into(c, fpu, 0, frame.at(0x38), f0);
                }
            }
            bool better = true;
            if (nonzero_logical(c, best)) {
                c.set(3, reg(s16(score)));
                better = compare(c, s16(score), s16(best_score)) > 0;
            }
            if (better) {
                best_score = score;
                best = candidate;
            }
        }
        list = list + 4;
        index = index + 1;
    }
    return guest::result(c, best);
}
}
