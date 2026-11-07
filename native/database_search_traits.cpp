// DATABASE.CPP's search block, part 4: the trait masks of a person (database_search.hpp).
// Both routines build a bit mask from a person's player record: role tests (natural positions), then
// tests of an attribute against `noise % m + k`, where the noise comes from a table the game walks two
// bytes at a time (the pointer restarts at the table, with one draw of the generator, when it passes its
// bound), and tests of player-statistics answers (0x10303560, PLAYER_STATS.CPP) in floating point.
// The two bodies differ in their tests; the machinery is shared below.
//
// What the routines leave beyond the result is part of the contract: the registers each callee saves
// (r13-r31, f30, f31), the argument registers at every call, and at return r0, r3-r10, f0-f5 and the status
// bits of the last operations; the sites set those the way the original's last writes leave them.
//
// Reached from the original's stack: the "assert" sites after `cmplwi r31,0` can never run (both routines
// return before any test when the person is null), so they are not reproduced.
#include "database_records.hpp"
#include "database_search.hpp"
#include "database_search_detail.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include <bit>

namespace cm::aot::database {
namespace {
using namespace search;
using guest::compare;
using guest::compare_logical;
using guest::Fpu;
using guest::Kept;

constexpr std::uint32_t rng_core = 0x1077b770;          // generated: the generator's next draw (15 bits)
constexpr std::uint32_t player_stat = 0x10303560;       // PLAYER_STATS.CPP: (object, record, mode, number, default f1) -> f1
constexpr std::uint32_t transfer_check = 0x103f43b0;    // TRANSFER_MANAGER.CPP: (object, person, 0) -> byte
constexpr std::uint32_t team_fit_a = 0x103b05c0;        // tactics: (object, argument, person, 0) -> signed byte
constexpr std::uint32_t team_fit_b = 0x103af570;        // tactics: (object, argument, 0) -> record
constexpr std::uint32_t team_fit_c = 0x10111240;        // tactics: (record, byte, 0) -> value
constexpr std::uint32_t id_rating = 0x10187650;         // generated: (table cell, id) -> halfword
constexpr std::uint32_t big_query = 0x10216830;         // generated: (person, value, 0x800, 0, -1, 0, sign, n)
constexpr std::uint32_t conversion_high = 0x43300000;   // the high word of the compiler's int-to-double idiom

// The record offsets the routines read (inferred: the player record's position bytes, then ratings).
namespace pl = ::cm::aot::database::player;
constexpr std::uint32_t byte_1c = 0x1c, byte_25 = 0x25;

// The statistic numbers handed to 0x10303560 (meanings not established).
constexpr std::uint32_t mode_two = 2, mode_six = 6;

// One draw from the noise table: the 16-bit table entry at the pointer plus the seed word.
struct Draw {
    std::uint32_t entry, seed, sum;
};

// The noise table walk (the words r25-r30 hold in the original): the pointer cell, its bound cell, the
// table's address and the seed cell.
class NoiseTable {
public:
    NoiseTable(Cpu& c, std::uint32_t pointer_cell, std::uint32_t bound_cell, std::uint32_t table, std::uint32_t seed_cell)
        : c_(c), pointer_(pointer_cell), bound_(bound_cell), table_(table), seed_(seed_cell) {}
    // `lwz rX,0(r30); lwz r0,0(bound); addi r3,rX,2; cmplw; stw r3,0(r30); ble`, and past the bound (unsigned)
    // the pointer restarts at the table and the seed becomes the low half of the generator's draw. `back` is
    // the address after the original's `bl` to the generator; `pointer_register` is the register the
    // original loaded the old pointer into (r3 at every site but one).
    Draw next(std::uint32_t back, unsigned pointer_register = 3) {
        const auto old = c_.u32(pointer_);
        const auto bound = c_.u32(bound_);
        const auto position = old + 2;
        if (pointer_register != 3) c_.set(pointer_register, old);
        c_.set({{3, position}, {0, bound}});
        compare_logical(c_, position, bound);
        c_.put32(pointer_, position);
        if (position > bound) {
            c_.put32(pointer_, table_);
            c_.call(rng_core, back);
            const auto low = c_.r(3) & 0xffff;
            c_.set(0, low);
            c_.put32(seed_, low);
        }
        const auto entry = std::uint32_t(c_.u16(c_.u32(pointer_)));
        const auto seed = c_.u32(seed_);
        return {entry, seed, entry + seed};
    }

private:
    Cpu& c_;
    std::uint32_t pointer_, bound_, table_, seed_;
};

// A division of the (non-negative in practice) sum by a constant the way the compiler writes it: the high
// word of the product with the magic number (plus the dividend for some magics), an arithmetic shift (whose
// XER[CA] is observable), the sign bit added, then the remainder from the product with the divisor.
struct Division {
    std::uint32_t raw, sign, quotient, product, remainder;
};
Division quotient_of(Cpu& c, std::uint32_t n, std::uint32_t magic, unsigned shift, bool add_dividend = false) {
    const auto raw = mulhw(magic, n);
    auto high = raw;
    if (add_dividend) high += n;
    if (shift) high = shift_right_algebraic(c, high, shift);
    const auto sign = high >> 31;
    return {raw, sign, high + sign, 0, 0};
}
Division divide(Cpu& c, std::uint32_t n, std::uint32_t magic, unsigned shift, std::uint32_t divisor,
                bool add_dividend = false) {
    auto d = quotient_of(c, n, magic, shift, add_dividend);
    d.product = d.quotient * divisor;
    d.remainder = n - d.product;
    return d;
}
// The remainder by 4 as the compiler writes it for a signed value (rotate/mask idiom, no multiply).
std::uint32_t remainder_four(std::uint32_t n) {
    const auto sign = n >> 31;
    const auto low = std::rotl(((n << 30) & 0xc0000000u) - sign, 2);
    return low + sign;
}
// subfic rD,rA,imm: the immediate minus the register, XER[CA] set when no borrow.
std::uint32_t subtract_from(Cpu& c, std::uint32_t immediate, std::uint32_t value) {
    c.s.wr<1>(guest::reg::xer_ca, immediate >= value);
    return immediate - value;
}

// The compiler's int-to-double idiom: 0x43300000 and the flipped value stored at `at`, loaded as a double
// (the caller subtracts 2^52 + 2^31 to get the integer).
double stored_integer(Cpu& c, std::uint32_t at, std::uint32_t value) {
    const auto flipped = value ^ 0x80000000u;
    c.put32(at, conversion_high);
    c.put32(at + 4, flipped);
    return std::bit_cast<double>(std::uint64_t(conversion_high) << 32 | flipped);
}
// fctiwz f0,value; stfd f0,at(r1); the low word is what the original reads back.
std::uint32_t stored_truncation(Cpu& c, Fpu& fpu, std::uint32_t at, double value) {
    const auto bits = fpu.fctiwz(0, value);
    c.put64(at, bits);
    return std::uint32_t(bits);
}

// The person's player record is read afresh at each test, as the original does.
std::uint32_t record_of(Cpu& c, std::uint32_t person) { return c.u32(person + person::player); }

// The sum of ten signed ratings of the player record above 0x6e (0x100d73ac, 0x100d83c8): a record that is
// null leaves only r0 and cr0. Leaves the registers of the original's chain of loads.
bool rating_total_above_limit(Cpu& c, std::uint32_t person) {
    const auto record = record_of(c, person);
    c.set(0, record);
    if (compare_logical(c, record) == 0) return false;
    const auto signed_byte = [&](std::uint32_t offset) { return guest::extended(std::int8_t(c.u8(record + offset))); };
    const auto total = signed_byte(0x16) + signed_byte(0x14) + signed_byte(0x13) + signed_byte(0x11) +
                       signed_byte(0x10) + signed_byte(0x15) + signed_byte(0x19) + signed_byte(0x18) +
                       signed_byte(0x17) + signed_byte(0x12);
    c.set({{0, total}, {3, signed_byte(0x12)}, {4, signed_byte(0x17)}, {5, signed_byte(0x18)},
           {6, std::uint32_t(c.u8(record + 0x17))}, {7, record}, {8, record}});
    return compare(c, std::int32_t(total), 0x6e) > 0;
}

// The call to 0x10303560 with the registers the original sets: the object behind the cell, the player
// record of the person, the mode and the number, and the default (the pool's first single) in f1.
double call_stat(Cpu& c, std::uint32_t object_cell, std::uint32_t float_pool, std::uint32_t person,
                 std::uint32_t mode, std::uint32_t number, std::uint32_t back) {
    c.set({{3, c.u32(object_cell)}, {4, record_of(c, person)}, {5, mode}, {6, number}});
    c.set_f(1, double(c.f32(float_pool)));
    c.call_in_place(player_stat, back);
    return c.f(1);
}
// The call to 0x100d2330 (attribute `number` of the person and record): the C++ routine, with the link
// register of the original's `bl`. Answers r3.
std::uint32_t call_attribute(Cpu& c, std::uint32_t person, std::uint32_t number, std::uint32_t back) {
    const auto record = record_of(c, person);
    guest::enter(c, back, {person, record, number});
    database::attribute_by_number_31(c, person, record, std::int8_t(number));
    return c.r(3);
}

// The register picture after `byte (signed) > n % d + k` tests that load the byte into r3 and keep the
// remainder in r4 ("first layout": lwz r5,0(r30) ... mulhw r4,r4,r5; extsb r3,r0; ... subf r4,r0,r5; addi r0,r4,k).
void leave_first_layout(Cpu& c, std::uint32_t byte, const Draw& draw, const Division& d, std::uint32_t k) {
    c.set({{3, byte}, {4, d.remainder}, {5, draw.sum}, {6, draw.entry}, {0, d.remainder + k}});
}
// "Second layout": mulhw r3,r4,r5; extsb r0,r0; ...; subf r3,r3,r5; cmpw r0,r3 (r0 the byte plus `extra`,
// r3 the remainder or its negation, r4 the sign of the quotient).
void leave_second_layout(Cpu& c, std::uint32_t left, std::uint32_t right, const Draw& draw, const Division& d) {
    c.set({{0, left}, {3, right}, {4, d.sign}, {5, draw.sum}, {6, draw.entry}});
}

// The player record's bytes the position tests read (signed compares against 15 and unsigned against 20).
bool natural(Cpu& c, std::uint32_t record, std::uint32_t offset) { return c.u8(record + offset) == 20; }
bool short_of_fifteen(Cpu& c, std::uint32_t record, std::uint32_t offset) {
    return std::int32_t(guest::extended(std::int8_t(c.u8(record + offset)))) < 0xf;
}


// `signed byte of the record > n % d + k`, the test of most sites of the first layout (see leave_first_layout).
bool byte_above(Cpu& c, NoiseTable& noise, std::uint32_t person, std::uint32_t offset, std::uint32_t back,
                std::uint32_t magic, unsigned shift, std::uint32_t divisor, std::uint32_t k) {
    const auto draw = noise.next(back);
    const auto d = divide(c, draw.sum, magic, shift, divisor);
    const auto byte = guest::extended(std::int8_t(c.u8(record_of(c, person) + offset)));
    leave_first_layout(c, byte, draw, d, k);
    return compare(c, std::int32_t(byte), std::int32_t(d.remainder + k)) > 0;
}

// 0x100d71a0's frame: 0x1c0 bytes, r19-r31 saved at -0x44 (an 8-byte gap below f31).
constexpr guest::Saves frame_a{.first_gpr = 19, .gpr_offset = -0x44, .first_fpr = 31};
}


// 0x100d71a0: the trait mask of a person (a zero `enabled` or a null person answers 0).
std::uint32_t person_trait_mask(Cpu& c, std::uint32_t person, std::uint32_t enabled) {
    constexpr std::uint32_t entry = 0x100d71a0;
    c.set(0, c.lr());
    compare_logical(c, enabled);
    guest::Frame frame(c, entry, 0x1c0, frame_a);
    const auto stats_object = c.toc(-0x7b3c), floats = c.toc(-0x56bc), doubles = c.toc(-0x5614);
    guest::keep(c, {{22, floats}, {21, stats_object}, {26, c.toc(-0x7b8c)}, {25, c.toc(-0x7b88)}, {30, c.toc(-0x7b84)},
                    {20, c.toc(-0x7bac)}, {24, c.toc(-0x7ba8)}, {27, c.toc(-0x5618)}, {23, doubles},
                    {28, c.toc(-0x7b90)}, {31, person}});
    Kept<19> r19(c, c.toc(-0x7b40));
    Kept<29> mask(c, 0);
    const auto add = [&](std::uint32_t bits) { mask = std::uint32_t(mask) + bits; };
    if (enabled == 0 || compare_logical(c, person) == 0) {
        c.set(3, 0);
        return 0;
    }
    NoiseTable noise(c, c.toc(-0x7b84), c.toc(-0x7b88), c.toc(-0x7b8c), c.toc(-0x7b90));
    Fpu fpu(c);
    const auto stat = [&](std::uint32_t number, std::uint32_t back, std::uint32_t mode = mode_two) {
        return call_stat(c, stats_object, floats, person, mode, number, back);
    };
    const auto attribute = [&](std::uint32_t number, std::uint32_t back) {
        compare_logical(c, person);  // cmplwi r31,0 (the assert behind it cannot run)
        r19 = call_attribute(c, person, number, back);
    };
    const auto byte_above = [&](std::uint32_t offset, std::uint32_t back, std::uint32_t magic, unsigned shift,
                                std::uint32_t divisor, std::uint32_t k) {
        return ::cm::aot::database::byte_above(c, noise, person, offset, back, magic, shift, divisor, k);
    };
    // `signed byte of r19 > n % 5 + 15` (the sites that follow an attribute call).
    const auto attribute_above = [&](std::uint32_t back) {
        const auto draw = noise.next(back);
        const auto d = divide(c, draw.sum, 0x66666667, 1, 5);
        const auto byte = guest::extended(std::int8_t(std::uint32_t(r19)));
        c.set({{3, byte}, {4, d.remainder}, {5, draw.sum}, {0, d.remainder + 0xf}});
        return compare(c, std::int32_t(byte), std::int32_t(d.remainder + 0xf)) > 0;
    };
    // `signed byte > n % 50` (second layout); the blocks of 0x39 and 0x3e continue only when true.
    const auto byte_above_fifty = [&](std::uint32_t offset, std::uint32_t back) {
        const auto draw = noise.next(back);
        const auto d = divide(c, draw.sum, 0x51eb851f, 4, 50);
        const auto byte = guest::extended(std::int8_t(c.u8(record_of(c, person) + offset)));
        leave_second_layout(c, byte, d.remainder, draw, d);
        return compare(c, std::int32_t(byte), std::int32_t(d.remainder)) > 0;
    };

    // Natural positions: one bit each (the tests read the record afresh; the last two always run).
    {
        const auto record = record_of(c, person);
        if (natural(c, record, pl::goalkeeper)) add(0x1);
        if (natural(c, record, pl::defender) && natural(c, record, pl::central)) add(0x4);
        if (natural(c, record, pl::defender) && (natural(c, record, pl::right) || natural(c, record, pl::left))) add(0x2);
        if (natural(c, record, pl::defensive_midfielder) && short_of_fifteen(c, record, pl::left) &&
            short_of_fifteen(c, record, pl::right))
            add(0x8);
        if (natural(c, record, pl::midfielder) && short_of_fifteen(c, record, pl::left) &&
            short_of_fifteen(c, record, pl::right))
            add(0x10);
        if (natural(c, record, pl::attacking_midfielder)) add(0x20);
        const auto last = std::uint32_t(c.u8(record + pl::attacker));
        if (last == 20) add(0x40);
        c.set({{3, record}, {0, last}});
        compare(c, std::int32_t(last), 20);
    }

    // A nibble of a table byte (indexed by the person's id) above n % 3: ask the transfer manager.
    {
        const auto draw = noise.next(0x100d732c);
        const auto d = divide(c, draw.sum, 0x55555556, 0, 3);
        const auto table = c.u32(c.toc(-0x7b9c));
        const auto nibble = std::uint32_t(c.u8(table + c.u32(person) * 100 + 0xc)) >> 4;
        c.set({{0, nibble}, {3, d.remainder}, {4, table}, {5, d.raw}, {6, draw.sum}, {7, draw.entry}});
        if (compare(c, std::int32_t(nibble), std::int32_t(d.remainder)) > 0) {
            const auto manager = c.u32(r19);
            c.set(0, manager);
            if (compare_logical(c, manager) != 0) {
                c.set({{3, c.u32(r19)}, {4, person}, {5, 0}});
                c.call_in_place(transfer_check, 0x100d73a0);
                const auto answer = guest::extended(std::int8_t(c.r(3)));
                c.set(0, answer);
                if (compare(c, std::int32_t(answer)) == 0) add(0x1000000);
            }
        }
    }
    if (rating_total_above_limit(c, person)) add(0x80);
    if (byte_above(0x3c, 0x100d7480, 0x55555556, 0, 3, 0xf)) add(0x100);

    // Statistics 0x16, 0xd, 1 and 5: r19 collects two truncated values.
    r19 = 0;
    {
        const auto first = stat(0x16, 0x100d74ec);
        c.set_f(0, double(c.f32(floats + 4)));
        fpu.fcmp(0, first, c.f(0));
        if (first > c.f(0)) {
            const auto second = stat(0xd, 0x100d7510);
            const auto low = stored_truncation(c, fpu, frame.at(0x168), second);
            const auto byte = guest::extended(std::int8_t(low));
            c.set({{3, low}, {0, byte}});
            if (compare(c, std::int32_t(byte), 0xa) > 0) {
                const auto conversion = c.f64(doubles + 0x18);
                const auto loaded = stored_integer(c, frame.at(0x160), byte);
                c.set({{3, byte ^ 0x80000000u}, {0, conversion_high}});
                c.set_f(2, conversion);
                c.set_f(0, c.f64(doubles + 0xf0));
                const auto subtracted = fpu.fsub(loaded, conversion);
                c.set_f(1, subtracted);
                const auto shifted = fpu.fsub(subtracted, c.f(0));
                c.set_f(0, shifted);
                const auto result = stored_truncation(c, fpu, frame.at(0x158), shifted);
                c.set(0, result);
                r19 = std::uint32_t(r19) + result;
            }
            const auto divisor = stat(1, 0x100d7578);
            c.set_f(31, divisor);
            const auto dividend = stat(5, 0x100d7594);
            c.set_f(0, double(c.f32(floats + 8)));
            const auto product = fpu.fmuls(c.f(0), dividend);
            c.set_f(0, product);
            const auto quotient = fpu.fdivs(product, divisor);
            c.set_f(0, quotient);
            const auto result = stored_truncation(c, fpu, frame.at(0x150), quotient);
            c.set(0, result);
            r19 = std::uint32_t(r19) + result;
        }
    }

    // Field 0x37 plus r19 above n % 50.
    {
        const auto draw = noise.next(0x100d75d0);
        const auto d = divide(c, draw.sum, 0x51eb851f, 4, 50);
        const auto left = guest::extended(std::int8_t(c.u8(record_of(c, person) + 0x37))) + std::uint32_t(r19);
        leave_second_layout(c, left, d.remainder, draw, d);
        if (compare(c, std::int32_t(left), std::int32_t(d.remainder)) > 0) add(0x200);
    }
    // Statistic 0xe against a pool constant (and kept in f31).
    {
        const auto tall = stat(0xe, 0x100d763c);
        c.set_f(31, tall);
        c.set_f(0, c.f64(doubles + 0xf8));
        fpu.fcmp(0, tall, c.f(0));
        if (tall > c.f(0)) add(0x800000);
    }

    // Fields 0x39 and 0x3e above n % 50 each; then, when f31 is above its constant, the truncated f31 against
    // n % 10 + 5, or else the attribute 0x17 against n % 5 + 15 and statistics 0x16 and 7. Any false test
    // leaves the block.
    do {
        if (!byte_above_fifty(0x39, 0x100d7670)) break;
        if (!byte_above_fifty(0x3e, 0x100d76dc)) break;
        c.set_f(0, c.f64(doubles + 0x40));
        fpu.fcmp(0, c.f(31), c.f(0));
        if (c.f(31) > c.f(0)) {
            const auto draw = noise.next(0x100d7754);
            const auto low = stored_truncation(c, fpu, frame.at(0x150), c.f(31));
            const auto d = divide(c, draw.sum, 0x66666667, 2, 10);
            const auto byte = guest::extended(std::int8_t(low));
            c.set({{3, byte}, {4, d.remainder}, {5, draw.sum}, {0, d.remainder + 5}});
            if (compare(c, std::int32_t(byte), std::int32_t(d.remainder + 5)) > 0) {
                add(0x400);
                break;
            }
        }
        attribute(0x17, 0x100d7820);
        if (!attribute_above(0x100d7844)) break;
        const auto first = stat(0x16, 0x100d78a4);
        c.set_f(31, first);
        const auto second = stat(7, 0x100d78c0);
        c.set_f(0, c.f64(doubles + 0x100));
        const auto product = fpu.fmul(c.f(0), second);
        c.set_f(0, product);
        fpu.fcmp(0, product, c.f(31));
        if (product < c.f(31)) add(0x400);
    } while (false);

    if (byte_above(0x2e, 0x100d78f4, 0x66666667, 1, 5, 0xf)) add(0x800);
    if (byte_above(0x40, 0x100d7968, 0x66666667, 1, 5, 0xf)) add(0x1000);
    if (byte_above(0x44, 0x100d79dc, 0x66666667, 1, 5, 0xf)) add(0x2000);

    // Attributes 4, 0xd, 0x14, 0x15 and 1, each tested against n % 5 + 15.
    attribute(4, 0x100d7aa0);
    if (attribute_above(0x100d7ac4)) add(0x4000);
    attribute(0xd, 0x100d7b80);
    if (attribute_above(0x100d7ba4)) add(0x20000);
    attribute(0x14, 0x100d7c60);
    if (attribute_above(0x100d7c84)) add(0x100000);
    attribute(0x15, 0x100d7d40);
    if (attribute_above(0x100d7d64)) add(0x200000);
    attribute(1, 0x100d7e20);
    if (attribute_above(0x100d7e44)) add(0x400000);
    if (byte_above(0x21, 0x100d7eb0, 0x55555556, 0, 3, 0x10)) add(0x8000);

    // Attribute 0xb against n % 4 + 14, then statistic 0xf against a scaled n % 20.
    attribute(0xb, 0x100d7f74);
    {
        const auto draw = noise.next(0x100d7f98);
        const auto remainder = remainder_four(draw.sum);
        const auto byte = guest::extended(std::int8_t(std::uint32_t(r19)));
        c.set({{3, byte}, {4, remainder}, {0, remainder + 0xe}});
        if (compare(c, std::int32_t(byte), std::int32_t(remainder + 0xe)) > 0) {
            const auto answer = stat(0xf, 0x100d7fec);
            c.set_f(31, answer);
            const auto next = noise.next(0x100d8010);
            const auto d = divide(c, next.sum, 0x66666667, 3, 20);
            const auto loaded = stored_integer(c, frame.at(0x150), d.remainder);
            c.set({{5, next.entry}, {3, d.sign}, {0, d.remainder ^ 0x80000000u}, {4, next.sum}});
            const auto conversion = c.f64(doubles + 0x18), scale = c.f64(doubles + 0xb0), offset = c.f64(doubles + 0x108);
            c.set_f(2, conversion);
            c.set_f(0, scale);
            c.set_f(3, offset);
            const auto converted = fpu.fsubs(loaded, conversion);
            c.set_f(1, converted);
            const auto quotient = fpu.fdiv(converted, scale);
            const auto limit = fpu.fadd(offset, quotient);
            c.set_f(0, limit);
            fpu.fcmp(0, c.f(31), limit);
            if (c.f(31) > limit) add(0x10000);
        }
    }

    // Statistic 0x16 against a quotient of the record's halfword at +0xa; then (above it) statistics 1 and 2
    // scaled by n % 3 (mode 6).
    {
        const auto answer = stat(0x16, 0x100d8098);
        const auto value = guest::extended(std::int16_t(c.u16(record_of(c, person) + 0xa)));
        const auto d = quotient_of(c, value, 0x66666667, 2);
        const auto limit_word = subtract_from(c, 0x578, d.quotient);
        const auto flipped = limit_word ^ 0x80000000u;
        const auto loaded = stored_integer(c, frame.at(0x150), limit_word);
        const auto conversion = c.f64(doubles + 0x18);
        c.set({{4, 0x66666667}, {3, d.sign}, {0, flipped}});
        c.set_f(2, conversion);
        const auto limit = fpu.fsubs(loaded, conversion);
        c.set_f(0, limit);
        fpu.fcmp(0, answer, limit);
        if (answer > limit) {
            const auto draw = noise.next(0x100d8100);
            const auto n = divide(c, draw.sum, 0x55555556, 0, 3);
            c.set({{7, n.raw}, {8, draw.sum}, {0, n.product}});
            r19 = n.remainder;
            const auto first = stat(1, 0x100d814c, mode_six);
            const auto flipped_remainder = std::uint32_t(r19) ^ 0x80000000u;
            c.set({{3, flipped_remainder}, {0, conversion_high}});
            c.set_f(4, first);
            const auto loaded_remainder = stored_integer(c, frame.at(0x150), std::uint32_t(r19));
            c.set_f(2, conversion);
            c.set_f(0, c.f64(doubles + 0xb0));
            c.set_f(3, c.f64(doubles + 0x48));
            const auto remainder = fpu.fsubs(loaded_remainder, conversion);
            c.set_f(2, remainder);
            const auto divided = fpu.fdiv(remainder, c.f(0));
            const auto shifted = fpu.fadd(c.f(3), divided);
            const auto single = fpu.frsp(shifted);
            c.set_f(0, single);
            c.set_f(31, fpu.fmuls(first, single));
            const auto second = stat(2, 0x100d819c, mode_six);
            fpu.fcmp(0, second, c.f(31));
            if (second > c.f(31)) add(0x40000);
        }
    }

    // Statistics 0x15 below a constant, 0x10 above one, and 2 times a constant below 1.
    {
        const auto first = stat(0x15, 0x100d81c0);
        c.set_f(0, c.f64(doubles + 0xc0));
        fpu.fcmp(0, first, c.f(0));
        if (first < c.f(0)) {
            const auto second = stat(0x10, 0x100d81e4);
            c.set_f(0, c.f64(doubles + 0x110));
            fpu.fcmp(0, second, c.f(0));
            if (second > c.f(0)) {
                const auto one = stat(1, 0x100d8208);
                c.set_f(31, one);
                const auto two = stat(2, 0x100d8224);
                c.set_f(0, c.f64(doubles + 0xc0));
                const auto product = fpu.fmul(c.f(0), two);
                c.set_f(0, product);
                fpu.fcmp(0, product, c.f(31));
                if (product < c.f(31)) add(0x80000);
            }
        }
    }
    c.set(3, mask);
    return mask;
}

// 0x100d8260: the second trait mask. `argument` is handed to the tactics callees; zero (or a null person) answers 0.
std::uint32_t person_trait_mask_with_team_fit(Cpu& c, std::uint32_t person, std::uint32_t argument) {
    constexpr std::uint32_t entry = 0x100d8260;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x170, {.first_gpr = 15, .first_fpr = 30});
    const auto stats_object = c.toc(-0x7b3c), floats = c.toc(-0x56bc), doubles = c.toc(-0x5614),
               team_cell = c.toc(-0x7a6c), id_cell = c.toc(-0x7ba0);
    guest::keep(c, {{16, team_cell}, {17, id_cell}, {24, floats}, {20, stats_object}, {29, c.toc(-0x7b90)},
                    {27, c.toc(-0x7b8c)}, {26, c.toc(-0x7b88)}, {30, c.toc(-0x7b84)}, {18, c.toc(-0x7bac)},
                    {23, c.toc(-0x7ba8)}, {25, c.toc(-0x5618)}, {19, doubles}, {22, argument}, {31, person}});
    Kept<28> mask(c, 0);
    Kept<16> r16(c, team_cell);
    const auto add = [&](std::uint32_t bits) { mask = std::uint32_t(mask) + bits; };
    if (compare(c, std::int32_t(argument)) == 0 || compare_logical(c, person) == 0) {
        c.set(3, 0);
        return 0;
    }
    NoiseTable noise(c, c.toc(-0x7b84), c.toc(-0x7b88), c.toc(-0x7b8c), c.toc(-0x7b90));
    Fpu fpu(c);
    const auto stat = [&](std::uint32_t number, std::uint32_t back, std::uint32_t mode = mode_two) {
        return call_stat(c, stats_object, floats, person, mode, number, back);
    };
    const auto attribute = [&](std::uint32_t number, std::uint32_t back) {
        compare_logical(c, person);  // cmplwi r31,0 (the assert behind it cannot run)
        return call_attribute(c, person, number, back);
    };
    // A signed byte of the record below the remainder (negated for the divisor -50): second layout.
    const auto byte_below_remainder = [&](std::uint32_t offset, std::uint32_t back, std::uint32_t magic, unsigned shift,
                                          std::uint32_t divisor, bool negated = false, bool add_dividend = false) {
        const auto draw = noise.next(back);
        const auto d = divide(c, draw.sum, magic, shift, divisor, add_dividend);
        const auto byte = guest::extended(std::int8_t(c.u8(record_of(c, person) + offset)));
        const auto right = negated ? std::uint32_t(0) - d.remainder : d.remainder;
        leave_second_layout(c, byte, right, draw, d);
        return compare(c, std::int32_t(byte), std::int32_t(right)) < 0;
    };
    const auto below_minus_fifty = [&](std::uint32_t offset, std::uint32_t back) {
        return byte_below_remainder(offset, back, 0xae147ae1, 4, std::uint32_t(-50), true);
    };
    // The guard of three sites: the byte at 0xf (signed) below 20; leaves r3 the record and r0 the byte.
    const auto short_position = [&] {
        const auto record = record_of(c, person);
        const auto byte = guest::extended(std::int8_t(c.u8(record + pl::goalkeeper)));
        c.set({{3, record}, {0, byte}});
        return compare(c, std::int32_t(byte), 0x14) < 0;
    };
    const auto signed_record_byte = [&](std::uint32_t offset) { return guest::extended(std::int8_t(c.u8(record_of(c, person) + offset))); };

    // Natural positions.
    {
        const auto record = record_of(c, person);
        if (natural(c, record, pl::defender) && natural(c, record, pl::central)) add(0x20);
        if (natural(c, record, pl::defender) && (natural(c, record, pl::right) || natural(c, record, pl::left))) add(0x10);
        if (natural(c, record, pl::defensive_midfielder) && short_of_fifteen(c, record, pl::left) &&
            short_of_fifteen(c, record, pl::right))
            add(0x40);
        if (natural(c, record, pl::midfielder) && short_of_fifteen(c, record, pl::left) &&
            short_of_fifteen(c, record, pl::right))
            add(0x80);
        if (natural(c, record, pl::attacking_midfielder)) add(0x100);
        const auto last = std::uint32_t(c.u8(record + pl::attacker));
        if (last == 20) add(0x200);
        c.set({{3, record}, {0, last}});
        compare(c, std::int32_t(last), 20);
    }
    if (rating_total_above_limit(c, person)) add(0x800000);

    // The person's own byte at +0x69 below n % 4 + 5; then (a record byte above 15 or another below 6)
    // statistics 1, 0xa and 0xb: the product of the first against a multiply-add of the others.
    {
        const auto draw = noise.next(0x100d8494);
        const auto remainder = remainder_four(draw.sum);
        const auto byte = guest::extended(std::int8_t(c.u8(person + 0x69)));
        c.set({{3, byte}, {5, draw.entry}, {4, remainder}, {0, remainder + 5}});
        if (compare(c, std::int32_t(byte), std::int32_t(remainder + 5)) < 0) {
            const auto first = signed_record_byte(byte_1c);
            c.set({{3, record_of(c, person)}, {0, first}});
            bool stats = compare(c, std::int32_t(first), 0xf) > 0;
            if (!stats) {
                const auto second = signed_record_byte(byte_25);
                c.set({{3, record_of(c, person)}, {0, second}});
                stats = compare(c, std::int32_t(second), 6) < 0;
            }
            if (stats) {
                const auto one = stat(1, 0x100d8514);
                c.set_f(0, double(c.f32(floats + 0x10)));
                const auto scaled = fpu.fmuls(one, c.f(0));
                c.set_f(30, scaled);
                const auto ten = stat(0xa, 0x100d8534);
                c.set_f(31, ten);
                const auto eleven = stat(0xb, 0x100d8550);
                c.set_f(0, double(c.f32(floats + 0xc)));
                const auto sum = fpu.multiply_add(0x100d8554, c.f(0), eleven, c.f(31));
                c.set_f(0, sum);
                fpu.fcmp(0, sum, c.f(30));
                if (sum > c.f(30)) add(1);
            }
        }
    }

    // r21 starts at 4, +4 when the id's rating (0x10187650) exceeds n % 1000 + 500, +3 when the byte at +0x34
    // is below n % 12. It is 4, 7, 8 or 11, never zero: the original's zero path and its out-of-range path
    // (a quotient past +-0xffff) cannot run.
    const auto bump = [&](std::uint32_t by) { c.set(21, c.r(21) + by); };
    const auto id_rating_above = [&](std::uint32_t back_draw, std::uint32_t back_call, unsigned shift,
                                     std::uint32_t divisor, std::uint32_t offset) {
        const auto draw = noise.next(back_draw);
        const auto d = divide(c, draw.sum, 0x10624dd3, shift, divisor);
        c.set({{3, c.u32(id_cell)}, {4, c.u32(person)}, {5, d.sign}, {6, draw.sum}, {0, d.product}, {15, d.remainder}});
        c.call(id_rating, back_call);
        const auto rating = guest::extended(std::int16_t(c.r(3)));
        const auto limit = c.r(15) + offset;
        c.set({{0, limit}, {3, rating}});
        return compare(c, std::int32_t(rating), std::int32_t(limit)) > 0;
    };
    c.set(21, 4);
    if (id_rating_above(0x100d8588, 0x100d85cc, 6, 1000, 0x1f4)) bump(4);
    if (byte_below_remainder(0x34, 0x100d8600, 0x2aaaaaab, 1, 12)) bump(3);
    {
        compare(c, std::int32_t(c.r(21)));
        const auto draw = noise.next(0x100d8680);
        const auto divisor = std::int32_t(c.r(21));
        const auto remainder = std::uint32_t(std::int32_t(draw.sum) % divisor);
        compare(c, divisor);
        c.set({{3, draw.sum}, {0, remainder}, {15, remainder}});
    }

    // The byte at +0x3c below r15 + 8 and a short position (byte 0xf below 20): +2.
    {
        const auto byte = signed_record_byte(0x3c);
        const auto limit = c.r(15) + 8;
        c.set({{3, byte}, {0, limit}});
        if (compare(c, std::int32_t(byte), std::int32_t(limit)) < 0 && short_position()) add(2);
    }

    // The tactics callees: a team fit (0x103b05c0) answering a small signed byte starts a query chain whose
    // result, with a draw, goes to 0x10216830.
    {
        c.set({{3, c.u32(r16)}, {4, argument}, {5, person}, {6, 0}});
        c.call_in_place(team_fit_a, 0x100d8800);
        c.set(15, c.r(3));
        const auto fit = guest::extended(std::int8_t(c.r(15)));
        c.set(0, fit);
        bool inside = compare(c, std::int32_t(fit), -1) > 0;
        if (inside) {
            c.set(0, guest::extended(std::int8_t(c.r(15))));
            inside = compare(c, std::int32_t(fit), 0xb) < 0;
        }
        if (inside) {
            c.set({{3, c.u32(r16)}, {4, argument}, {5, 0}});
            c.call_in_place(team_fit_b, 0x100d882c);
            c.set({{4, c.r(15)}, {5, 0}});
            c.call_in_place(team_fit_c, 0x100d8838);
            c.set(15, c.r(3));
            const auto draw = noise.next(0x100d885c, 4);
            const auto d = divide(c, draw.sum, 0x66666667, 4, 40);
            c.set({{3, person}, {4, c.r(15)}, {5, 0x800}, {6, 0}, {7, 0xffffffff}, {8, 0}, {9, d.sign},
                   {10, draw.sum}, {0, d.product}, {15, d.remainder}});
            c.call(big_query, 0x100d88b0);
            const auto answer = c.r(3);
            const auto limit = c.r(15) + 0x96;
            c.set(0, limit);
            if (compare(c, std::int32_t(answer), std::int32_t(limit)) < 0) add(4);
        }
    }

    // Statistic 0x16 above a constant: r15 = 0x46 - the truncated statistic 0xd (when above 10), minus the
    // truncated statistic 5 scaled by statistic 1.
    c.set(15, 0);
    {
        const auto first = stat(0x16, 0x100d88dc);
        c.set_f(0, double(c.f32(floats + 4)));
        fpu.fcmp(0, first, c.f(0));
        if (first > c.f(0)) {
            const auto second = stat(0xd, 0x100d8900);
            const auto low = stored_truncation(c, fpu, frame.at(0x100), second);
            const auto byte = guest::extended(std::int8_t(low));
            c.set({{3, low}, {0, byte}});
            if (compare(c, std::int32_t(byte), 0xa) > 0) {
                const auto adjustment = subtract_from(c, 0x46, byte);
                c.set(0, adjustment);
                c.set(15, c.r(15) + adjustment);
            }
            const auto one = stat(1, 0x100d893c);
            c.set_f(31, one);
            const auto five = stat(5, 0x100d8958);
            c.set_f(0, double(c.f32(floats + 8)));
            const auto scaled = fpu.fmuls(c.f(0), five);
            c.set_f(0, scaled);
            const auto quotient = fpu.fdivs(scaled, c.f(31));
            c.set_f(0, quotient);
            const auto result = stored_truncation(c, fpu, frame.at(0x108), quotient);
            c.set(0, result);
            c.set(15, c.r(15) - result);
        }
    }

    // The byte at +0x37 below r15 - n % 50.
    {
        const auto draw = noise.next(0x100d8994);
        const auto d = divide(c, draw.sum, 0xae147ae1, 4, std::uint32_t(-50));
        const auto byte = signed_record_byte(0x37);
        const auto limit = c.r(15) - d.remainder;
        c.set({{3, byte}, {4, d.sign}, {5, draw.sum}, {6, draw.entry}, {0, limit}});
        if (compare(c, std::int32_t(byte), std::int32_t(limit)) < 0) add(0x400);
    }
    // Bytes 0x39 and 0x3e below -(n % 50) each; then statistic 0xe (above a constant) truncated against n % 10.
    if (below_minus_fifty(0x39, 0x100d8a0c) && below_minus_fifty(0x3e, 0x100d8a7c)) {
        const auto tall = stat(0xe, 0x100d8ae4);
        c.set_f(30, tall);
        c.set_f(0, c.f64(doubles + 0x40));
        fpu.fcmp(0, tall, c.f(0));
        if (tall > c.f(0)) {
            const auto draw = noise.next(0x100d8b14);
            const auto low = stored_truncation(c, fpu, frame.at(0x100), tall);
            const auto d = divide(c, draw.sum, 0x66666667, 2, 10);
            const auto byte = guest::extended(std::int8_t(low));
            c.set({{0, byte}, {3, d.remainder}, {4, d.sign}, {5, draw.sum}});
            if (compare(c, std::int32_t(byte), std::int32_t(d.remainder)) < 0) add(0x800);
        }
    }
    // The id rating again, against n % 500 + 750, when the cell is set.
    {
        const auto cell = c.u32(id_cell);
        c.set(0, cell);
        if (compare_logical(c, cell) != 0 && id_rating_above(0x100d8b94, 0x100d8bd8, 5, 500, 0x2ee)) add(8);
    }
    if (byte_below_remainder(0x2e, 0x100d8c0c, 0x88888889, 3, 15, false, true)) add(0x1000);
    if (short_position() && byte_below_remainder(0x40, 0x100d8c94, 0x66666667, 2, 10)) add(0x2000);
    if (short_position() && byte_below_remainder(0x44, 0x100d8d18, 0x2aaaaaab, 1, 12)) add(0x4000);
    if (below_minus_fifty(0x1e, 0x100d8d88)) add(0x8000);
    if (short_position()) {
        // Two bytes added, above n % 20 + 25.
        const auto draw = noise.next(0x100d8e14);
        const auto d = divide(c, draw.sum, 0x66666667, 3, 20);
        const auto total = signed_record_byte(byte_1c) + signed_record_byte(byte_25);
        c.set({{3, total}, {4, d.remainder}, {5, d.raw}, {6, draw.sum}, {0, d.remainder + 0x19}});
        if (compare(c, std::int32_t(total), std::int32_t(d.remainder + 0x19)) > 0) add(0x10000);
    }
    {
        // The byte at +0x21 below 12 - n % 10.
        const auto draw = noise.next(0x100d8e98);
        const auto d = divide(c, draw.sum, 0x66666667, 2, 10);
        const auto byte = signed_record_byte(0x21);
        const auto limit = subtract_from(c, 0xc, d.remainder);
        c.set({{3, byte}, {4, d.sign}, {5, draw.sum}, {6, draw.entry}, {0, limit}});
        if (compare(c, std::int32_t(byte), std::int32_t(limit)) < 0) add(0x20000);
    }

    // With the byte at +0x11 (signed) below 15: attribute 0xb against n % 4 + 14, then statistic 0xf against a
    // constant and attribute 0xb scaled.
    {
        const auto record = record_of(c, person);
        const auto byte = guest::extended(std::int8_t(c.u8(record + pl::defender)));
        c.set({{3, record}, {0, byte}});
        if (compare(c, std::int32_t(byte), 0xf) < 0) {
            r16 = attribute(0xb, 0x100d8f70);
            const auto draw = noise.next(0x100d8f94);
            const auto remainder = remainder_four(draw.sum);
            const auto value = guest::extended(std::int8_t(std::uint32_t(r16)));
            c.set({{3, value}, {4, remainder}, {0, remainder + 0xe}});
            if (compare(c, std::int32_t(value), std::int32_t(remainder + 0xe)) > 0) {
                const auto answer = stat(0xf, 0x100d8fe8);
                c.set_f(30, answer);
                c.set_f(0, c.f64(doubles + 0x40));
                fpu.fcmp(0, answer, c.f(0));
                if (answer > c.f(0)) {
                    const auto result = guest::extended(std::int8_t(attribute(0xb, 0x100d9068)));
                    const auto loaded = stored_integer(c, frame.at(0x100), result);
                    c.set({{3, result ^ 0x80000000u}, {0, conversion_high}});
                    const auto conversion = c.f64(doubles + 0x18);
                    c.set_f(1, conversion);
                    c.set_f(2, c.f64(doubles + 0x118));
                    const auto difference = fpu.fsub(loaded, conversion);
                    c.set_f(0, difference);
                    const auto limit = fpu.fmul(c.f(2), difference);
                    c.set_f(0, limit);
                    fpu.fcmp(0, answer, limit);
                    if (answer < limit) add(0x40000);
                }
            }
        }
    }
    if (byte_above(c, noise, person, 0x28, 0x100d90bc, 0x66666667, 1, 5, 0xd)) add(0x80000);

    // Statistic 0x16 above a constant: attribute 0xc and a draw against statistic 0x10 and the mode-6
    // statistics 1 and 2.
    {
        const auto answer = stat(0x16, 0x100d9128);
        c.set_f(0, double(c.f32(floats + 4)));
        fpu.fcmp(0, answer, c.f(0));
        if (answer > c.f(0)) {
            r16 = attribute(0xc, 0x100d91a4);
            const auto draw = noise.next(0x100d91c8);
            const auto d = divide(c, draw.sum, 0x66666667, 3, 20);
            c.set({{7, d.sign}, {8, draw.sum}, {0, d.product}, {15, d.remainder}});
            const auto result = stat(0x10, 0x100d9218);
            const auto first_value = guest::extended(std::int8_t(std::uint32_t(r16)));
            const auto second_value = c.r(15) + 0x14;
            const auto first_loaded = stored_integer(c, frame.at(0x100), first_value);
            const auto second_loaded = stored_integer(c, frame.at(0x108), second_value);
            c.set({{4, conversion_high}, {3, second_value ^ 0x80000000u}, {0, conversion_high}});
            const auto conversion = c.f64(doubles + 0x18);
            c.set_f(2, conversion);
            c.set_f(3, double(c.f32(floats + 0x14)));
            const auto first_difference = fpu.fsubs(first_loaded, conversion);
            c.set_f(0, first_difference);
            c.set_f(2, c.f64(doubles + 0x18));
            const auto sum = fpu.fadds(result, first_difference);
            c.set_f(1, sum);
            const auto limit = fpu.fsubs(second_loaded, c.f(2));
            c.set_f(0, limit);
            const auto adjusted = fpu.fsubs(sum, c.f(3));
            c.set_f(1, adjusted);
            fpu.fcmp(0, adjusted, limit);
            if (adjusted < limit) {
                const auto one = stat(1, 0x100d9284, mode_six);
                c.set_f(0, c.f64(doubles + 0x68));
                const auto scaled = fpu.fmul(c.f(0), one);
                c.set_f(31, scaled);
                const auto two = stat(2, 0x100d92a4, mode_six);
                fpu.fcmp(0, two, scaled);
                if (two < scaled) add(0x100000);
            }
        }
    }
    {
        const auto first = stat(0x15, 0x100d92c8);
        c.set_f(0, c.f64(doubles + 0xc0));
        fpu.fcmp(0, first, c.f(0));
        if (first < c.f(0)) {
            const auto second = stat(0x10, 0x100d92ec);
            c.set_f(0, c.f64(doubles + 0x110));
            fpu.fcmp(0, second, c.f(0));
            if (second > c.f(0)) add(0x200000);
        }
    }

    // Attribute 0x14 (signed): below 7 adds the last bit outright; otherwise below the record's halfword at
    // +4 divided by 15.
    const auto first = guest::extended(std::int8_t(attribute(0x14, 0x100d936c)));
    c.set(0, first);
    if (compare(c, std::int32_t(first), 7) < 0) {
        add(0x400000);
    } else {
        const auto second = guest::extended(std::int8_t(attribute(0x14, 0x100d93e8)));
        const auto record = record_of(c, person);
        const auto value = guest::extended(std::int16_t(c.u16(record + 4)));
        const auto d = quotient_of(c, value, 0x88888889, 3, true);
        c.set({{4, record}, {5, 0x88888889}, {6, second}, {3, d.sign}, {0, d.quotient}});
        if (compare(c, std::int32_t(second), std::int32_t(d.quotient)) < 0) add(0x400000);
    }
    c.set(3, mask);
    return mask;
}
}
