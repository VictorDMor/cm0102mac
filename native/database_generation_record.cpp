// DATABASE.CPP's generation block, part 7: 0x100c53e0, a record of a 0x2c-byte table chosen at random
// when no record is marked (database_generation.hpp).
#include "database_generation.hpp"
#include "database_generation_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::database {
using namespace generation;
namespace {
constexpr std::uint32_t entry = 0x100c53e0;
constexpr std::int32_t seen_flag = -0x39e9, cached_slot = -0x39e8;  // bytes/words at r2 + offset

// The index for a count beyond +-65535: the pool sample (plus salt, reduced modulo 65535 by the compiler's
// multiply-high division) as a fraction of the count in floating point, plus a bias from 0x100a0d80 divided
// by 10, clamped to count - 1. The result is left in r29, the registers as the compiler left them.
void wide_index(Cpu& c, std::uint32_t constants, std::uint32_t count) {
    const auto next = wide_scale(c, constants, count, 30, 29, 29);
    c.set(3, next.shifted + next.sign);
    c.link(entry + 0x1a0);
    range_sample(c, std::int32_t(c.r(3)));
    c.set(4, 0x66660000);
    c.set(3, c.r(3) + 5);
    c.set(4, c.r(4) + 0x6667);
    c.set(0, count - 1);
    c.set(3, std::uint32_t((std::int64_t(std::int32_t(c.r(4))) * std::int32_t(c.r(3))) >> 32));
    c.set(3, srawi(c, c.r(3), 2));
    c.set(4, c.r(3) >> 31);
    c.set(3, c.r(3) + c.r(4));
    c.set(29, c.r(29) + c.r(3));
    if (compare(c, std::int32_t(c.r(29)), std::int32_t(c.r(0))) > 0) c.set(29, count - 1);
}
}

// 0x100c53e0. A frame of 0x70 saving r27-r31 and LR. The first call looks for the first record whose first
// byte is 0 and keeps its index in a static word; later calls return that record. With none, the index is
// drawn from the sample pool (the pool cursor wraps with a random draw as in the list routines): for a
// count within +-65535 it is |(sample + salt) % count| and otherwise the floating-point scaling of the
// sample (0x100a0d80 adds a bias), clamped to count - 1.
std::uint32_t pick_unmarked_record(Cpu& c) {
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x70, {.first_gpr = 27});
    const auto count_at = c.toc(-0x7554);
    c.set(6, count_at);
    const auto records_at = c.toc(-0x7534);
    c.set(28, records_at);
    const auto salt_at = c.toc(-0x7b90);
    c.set(29, salt_at);
    const auto start = c.toc(-0x7b8c);
    c.set(5, start);
    const auto end_at = c.toc(-0x7b88);
    c.set(4, end_at);
    const auto cursor_at = c.toc(-0x7b84);
    c.set(30, cursor_at);
    const auto constants = c.toc(-0x5614);
    c.set(31, constants);
    const auto flag_at = c.r(2) + std::uint32_t(seen_flag), cached_at = c.r(2) + std::uint32_t(cached_slot);
    c.set(0, extsb(c.u8(flag_at)));
    if (compare(c, std::int32_t(c.r(0))) == 0) {
        c.set(3, 0xffffffff);
        c.set(0, 1);
        c.put32(cached_at, 0xffffffff);
        c.put8(flag_at, 1);
    }
    const auto record_of = [&](std::uint32_t index) {
        c.set(0, index * name_entry::stride);
        load32(c, 3, records_at);
        c.set(3, c.r(3) + c.r(0));
        return c.r(3);
    };
    load32(c, 0, cached_at);
    if (compare(c, std::int32_t(c.r(0)), -1) != 0) {
        load32(c, 0, cached_at);
        load32(c, 3, records_at);
        c.set(0, c.r(0) * name_entry::stride);
        c.set(3, c.r(3) + c.r(0));
        return c.r(3);
    }
    c.set(7, 0);
    for (;;) {
        load32(c, 0, count_at);
        if (compare(c, std::int32_t(c.r(7)), std::int32_t(c.r(0))) >= 0) break;
        c.set(0, c.r(7) * name_entry::stride);
        load32(c, 3, records_at);
        c.set(0, extsb(c.u8(c.r(3) + c.r(0))));
        if (compare(c, std::int32_t(c.r(0))) == 0) {
            c.put32(cached_at, c.r(7));
            break;
        }
        c.set(7, c.r(7) + 1);
    }
    load32(c, 0, cached_at);
    if (compare(c, std::int32_t(c.r(0)), -1) != 0) {
        load32(c, 0, cached_at);
        load32(c, 3, records_at);
        c.set(0, c.r(0) * name_entry::stride);
        c.set(3, c.r(3) + c.r(0));
        return c.r(3);
    }
    const auto count = load32(c, 27, count_at);
    if (compare(c, std::int32_t(count), 0) == 0) {
        c.set(29, 0);
        return record_of(0);
    }
    advance_pool(c, {3, 0}, cursor_at, end_at, salt_at, start, entry + 0xc8);
    c.set(3, 0xffff);
    const bool wide = compare(c, std::int32_t(count), 0xffff) > 0;
    if (!wide) {
        c.set(3, 0xffff0001);
        if (compare(c, std::int32_t(count), std::int32_t(0xffff0001)) >= 0) {
            // |(sample + salt) % count| from the pool's current sample.
            load32(c, 3, cursor_at);
            compare(c, std::int32_t(count), 0);
            load32(c, 0, salt_at);
            c.set(3, c.u16(c.r(3)));
            c.set(3, c.r(3) + c.r(0));
            const auto dividend = std::int32_t(c.r(3));
            c.set(0, std::uint32_t(divide_signed<4>(std::uint32_t(dividend), count)));  // as the reference: INT32_MIN / -1 is not UB
            c.set(0, c.r(0) * count);
            c.set(29, c.r(3) - c.r(0));
            c.set(0, 0u - c.r(29));
            if (std::int32_t(count) > 0) c.set(0, c.r(29));
            c.set(29, c.r(0));
            return record_of(c.r(29));
        }
    }
    wide_index(c, constants, count);
    return record_of(c.r(29));
}
}
