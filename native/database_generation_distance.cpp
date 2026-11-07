// DATABASE.CPP's generation block, part 4: the distance between two people's attribute slots
// (database_generation.hpp). Both routines copy each person's attributes into a static pair of
// slot arrays (the generation_attributes copies), start from a scaled difference of one record
// field, then add, for each slot until the terminator -0x7f, a penalty by how far apart the
// two attributes are. Their registers and stack words are the compiler's; all are reproduced.
#include "database_generation.hpp"
#include "database_generation_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::database {
using namespace generation;
namespace {
using guest::Fpu;
constexpr std::uint32_t abs_entry = 0x10775540;  // the C library's abs: r3 = |r3|, r4 the sign mask
constexpr std::uint32_t slot_size = 4, slot_total = 63;
constexpr std::uint32_t terminator = 0xff81;  // -0x7f as a 16-bit value, and the byte 0x81 in the flag byte
std::uint32_t extsh(std::uint16_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }

// The two static slot arrays' addresses and the two flag bytes at TOC offsets (set the first time
// through, cleared after the arrays are filled).
struct Statics {
    std::int32_t first_slot, second_slot;     // TOC words: the arrays
    std::int32_t flag_a, flag_b;              // bytes at r2 + offset: "seen" (-0x39ed / -0x39eb) is flag_a
};

// The `lbz/extsb.` pair, then the first-time setup: both flag bytes are set to 1 and the arrays
// are filled with the terminator (value, and the byte at +2).
void initialise_arrays(Cpu& c, const Statics& s, std::uint32_t first, std::uint32_t second) {
    c.set(0, extsb(c.u8(c.r(2) + std::uint32_t(s.flag_a))));
    if (compare(c, std::int32_t(c.r(0))) == 0) {
        c.set(3, 1);
        c.set(0, 1);
        c.put8(c.r(2) + std::uint32_t(s.flag_b), 1);
        c.put8(c.r(2) + std::uint32_t(s.flag_a), 1);
    }
    c.set(0, extsb(c.u8(c.r(2) + std::uint32_t(s.flag_b))));
    if (compare(c, std::int32_t(c.r(0))) == 0) return;
    for (unsigned i = 0; i < slot_total; ++i) {
        c.put16(second + i * slot_size, terminator);
        c.put16(first + i * slot_size, terminator);
        c.put8(second + i * slot_size + 2, 0x81);
        c.put8(first + i * slot_size + 2, 0x81);
    }
    c.set(9, slot_total);
    c.set(0, 0);
    c.put8(c.r(2) + std::uint32_t(s.flag_b), 0);
}

// The floating-point step both routines run after the second copy: the scaled |difference| of two
// 16-bit record fields becomes the starting total in r29. `shift` is 0 (frame words at +0x38/+0x40/+0x44) or
// 4 (+0x3c/+0x44/+0x48), and the second routine runs `between` between the fsub and the fmul.
template <class Between>
void scaled_start(Cpu& c, std::uint32_t frame, std::uint32_t shift, Between&& between) {
    c.set(0, 0x43300000);
    c.set(3, c.r(3) ^ 0x80000000u);
    const auto pool = c.toc(-0x5614);
    c.set(4, pool);
    c.put32(frame + 0x44 + shift, c.r(3));
    const auto constant = c.f64(pool + 0x18);
    c.set_f(2, constant);
    c.put32(frame + 0x40 + shift, c.r(0));
    c.set(3, c.toc(-0x5614));
    const auto converted = std::bit_cast<double>(std::uint64_t(c.u32(frame + 0x40 + shift)) << 32 | c.u32(frame + 0x44 + shift));
    c.set_f(1, converted);
    c.set_f(0, c.f64(c.r(3) + 0xa8));
    Fpu fpu(c);
    c.set_f(1, fpu.fsub(c.f(1), c.f(2)));
    between();
    c.set_f(0, fpu.fmul(c.f(0), c.f(1)));
    c.put64(frame + 0x38 + shift, fpu.fctiwz(0, c.f(0)));
    c.set(29, c.u32(frame + 0x3c + shift));
}

bool in_rating_range(Cpu& c, std::uint32_t array, std::uint32_t index_register) {
    c.set(0, extsb(c.r(index_register)));
    c.set(3, 0);
    c.set(0, c.r(0) << 2);
    c.set(0, extsh(c.u16(array + c.r(0))));
    if (compare(c, std::int32_t(c.r(0)), 1) >= 0) {
        c.set(0, extsb(c.r(index_register)));
        c.set(0, c.r(0) << 2);
        c.set(0, extsh(c.u16(array + c.r(0))));
        if (compare(c, std::int32_t(c.r(0)), 0x14) <= 0) c.set(3, 1);
    }
    return compare(c, std::int32_t(c.r(3))) != 0;
}

// |second[i] - first[i]| of the slot r28 names: the registers r3 and r0 as the original loads them,
// then the call of abs; `back` is the return address.
void slot_distance(Cpu& c, std::uint32_t first, std::uint32_t second, std::uint32_t back) {
    c.set(3, extsb(c.r(28)));
    c.set(0, extsb(c.r(28)));
    c.set(3, c.r(3) << 2);
    c.set(0, c.r(0) << 2);
    c.set(3, extsh(c.u16(first + c.r(3))));
    c.set(0, extsh(c.u16(second + c.r(0))));
    c.set(3, c.r(0) - c.r(3));
    c.call(abs_entry, back);
}
}

// 0x100c58c0. A frame of 0x60 saving r26-r31. r26 and r27 hold the two people, r28 the slot,
// r29 the total, r30 and r31 the two arrays. The first copy's r4 is the caller's r4.
std::int32_t attribute_distance(Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t start) {
    constexpr std::uint32_t entry = 0x100c58c0;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x60, {.first_gpr = 26});
    const auto first = c.toc(-0x5654), second = c.toc(-0x5650);
    c.set({{30, first}, {26, a}, {31, second}, {27, b}, {29, start}, {28, 0}});
    initialise_arrays(c, {-0x5654, -0x5650, -0x39ed, -0x39ec}, first, second);
    guest::enter(c, entry + 0xb4, {a, c.r(4), second, 0});
    copy_attributes_to_slots(c, a, second, 0);
    guest::enter(c, entry + 0xc8, {b, start, first, 0});
    copy_attributes_to_slots(c, b, first, 0);
    c.set(3, extsh(c.u16(b + 0x24)));
    c.set(0, extsh(c.u16(a + 0x24)));
    c.set(3, c.r(0) - c.r(3));
    c.call(abs_entry, entry + 0xd8);
    scaled_start(c, frame.base(), 0, [] {});
    for (;;) {
        c.set(0, extsb(c.r(28)));
        c.set(0, c.r(0) << 2);
        c.set(0, extsh(c.u16(second + c.r(0))));
        if (compare(c, std::int32_t(c.r(0)), -0x7f) == 0) break;
        if (!in_rating_range(c, first, 28)) {
            c.set(29, c.r(29) + 0x64);
        } else if (in_rating_range(c, second, 28)) {
            slot_distance(c, first, second, entry + 0x1ac);
            c.set(0, c.r(3) << 4);
            c.set(29, c.r(29) + c.r(0));
        }
        c.set(28, c.r(28) + 1);
    }
    c.set(3, c.r(29));
    return std::int32_t(c.r(3));
}

// 0x100c5630: the same over the larger attribute slots (copy_all_attributes_to_slots, whose first count
// output is a byte in this frame at +0x38), with a total that starts at 0 for two people whose first slots both
// hold 20, and otherwise weighs the first slot by 1000, the next ones below the count by 25 and the rest by 16.
// `a_other` and `b_other` are the second records the copies read (+0x64): the caller's r4 and r6.
std::int32_t attribute_distance_all(Cpu& c, std::uint32_t a, std::uint32_t a_other, std::uint32_t b,
                                    std::uint32_t b_other) {
    constexpr std::uint32_t entry = 0x100c5630;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x70, {.first_gpr = 26});
    const auto first = c.toc(-0x564c), second = c.toc(-0x5648);
    c.set({{30, first}, {26, a}, {31, second}, {27, b}, {29, b_other}, {28, 0}});
    initialise_arrays(c, {-0x564c, -0x5648, -0x39eb, -0x39ea}, first, second);
    c.set({{3, a}, {5, second}, {6, frame.at(0x38)}, {7, 0}, {8, 0}});
    guest::enter(c, entry + 0xbc, {a, a_other, second, frame.at(0x38), 0, 0});
    copy_all_attributes_to_slots(c, a, a_other, second, frame.at(0x38), 0, 0);
    guest::enter(c, entry + 0xd8, {b, b_other, first, 0, 0, 0});
    copy_all_attributes_to_slots(c, b, b_other, first, 0, 0, 0);
    c.set(3, extsh(c.u16(b + 4)));
    c.set(0, extsh(c.u16(a + 4)));
    c.set(3, c.r(0) - c.r(3));
    c.call(abs_entry, entry + 0xe8);
    bool both_twenty = false;
    bool second_twenty = false;
    scaled_start(c, frame.base(), 4, [&] {
        c.set(0, extsh(c.u16(second)));
        second_twenty = compare(c, std::int32_t(c.r(0)), 0x14) == 0;
    });
    if (second_twenty) {  // the first array's first slot decides too
        c.set(0, extsh(c.u16(first)));
        both_twenty = compare(c, std::int32_t(c.r(0)), 0x14) == 0;
    }
    if (both_twenty) {
        c.set(3, 0);
        return 0;
    }
    for (;;) {
        c.set(0, extsb(c.r(28)));
        c.set(0, c.r(0) << 2);
        c.set(0, extsh(c.u16(second + c.r(0))));
        if (compare(c, std::int32_t(c.r(0)), -0x7f) == 0) break;
        if (!in_rating_range(c, first, 28)) {
            c.set(29, c.r(29) + 0x64);
        } else if (in_rating_range(c, second, 28)) {
            c.set(0, extsb(c.r(28)));
            const bool at_zero = compare(c, std::int32_t(c.r(0))) == 0;
            if (at_zero) {
                slot_distance(c, first, second, entry + 0x1e0);
                c.set(0, c.r(3) * 0x3e8);
            } else {
                c.set(0, c.u8(frame.at(0x38)));
                c.set(3, extsb(c.r(28)));
                c.set(0, extsb(c.r(0)));
                if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) < 0) {
                    slot_distance(c, first, second, entry + 0x220);
                    c.set(0, c.r(3) * 0x19);
                } else {
                    slot_distance(c, first, second, entry + 0x24c);
                    c.set(0, c.r(3) << 4);
                }
            }
            c.set(29, c.r(29) + c.r(0));
        }
        c.set(28, c.r(28) + 1);
    }
    c.set(3, c.r(29));
    return std::int32_t(c.r(3));
}
}
