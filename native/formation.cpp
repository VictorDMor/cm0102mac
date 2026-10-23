#include "formation.hpp"
#include <array>
#include <utility>

namespace cm::aot::formation {
using guest::Cpu;
using guest::Frame;
namespace {
// FORMATION.CPP's string pool (a TOC slot) and the strings its asserts use.
constexpr std::int32_t pool_slot = -0x550c, message_slot = -0x7ba8;
constexpr std::int32_t staff_slot = -0x7be0;  // -> staff records
guest::Assertion assertion(Cpu& c, std::uint32_t at, const Frame& frame, std::int32_t name, std::int32_t extension,
                           std::uint32_t line) {
    const auto pool = c.toc(pool_slot);
    return {at, pool + 0xe, pool + 0x1c, pool + 0x29, frame.at(name), frame.at(extension), line};
}
bool valid(std::int8_t player) { return player >= 0 && player < players; }

// Shape codes the original treats as the same shape.
constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 11> shape_aliases{{
    {0x101176c1, 0x101f06c1}, {0x101b26c1, 0x101f06c1},
    {0x38115381, 0x381b0381}, {0x3a242381, 0x381b0381},
    {0x280e07c1, 0x280e8b81}, {0x091a07c1, 0x280e8b81}, {0x2884ab81, 0x280e8b81},
    {0x280aab81, 0x280e8b81}, {0x288c07c1, 0x280e8b81}, {0x281507c1, 0x280e8b81},
    {0x288a8b81, 0x281b2381}}};

// A record-form test (`rlwinm.`, `extsb.`): the value compared with 0 into cr0.
bool test(Cpu& c, std::uint32_t value) {
    c.compare(0, std::int32_t(value), 0);
    return value != 0;
}
}

std::int32_t position(Cpu& c, std::uint32_t formation, std::int8_t player, std::int8_t with_ball) {
    Frame frame(c, 0x10111240, 0x70, {.first_gpr = 30});
    test(c, std::uint32_t(std::int32_t(player)));
    if (player >= 0) c.compare(0, player, 10);
    if (!valid(player)) {
        c.set({{31, c.toc(pool_slot)}, {30, c.toc(message_slot)}});
        guest::report_assertion(c, assertion(c, 0x10111284, frame, 0x40, 0x38, 0xfb));
        c.set(4, c.toc(-0x7bac));
        return 1;
    }
    c.set(4, std::uint32_t(std::int32_t(player)));
    test(c, std::uint32_t(std::int32_t(with_ball)));
    return Record(c, formation).position(player, with_ball != 0);
}

std::int32_t player_at(Cpu& c, std::uint32_t formation, std::int16_t position) {
    const Record record(c, formation);
    c.set(4, std::uint32_t(std::int32_t(position)));
    std::int8_t player = 0;
    for (; player < players; ++player) {
        // r5 and r6 count halfwords and players as the loop goes.
        c.set({{5, std::uint32_t(player) * 2}, {6, std::uint32_t(player)}});
        c.compare(0, record.position(player), position);
        if (record.position(player) == position) return player;
    }
    c.set({{5, 22}, {6, 11}});
    return -1;
}

std::uint64_t instructions(Cpu& c, std::uint32_t formation, std::int8_t player) {
    return c.u64(formation + field::instructions + std::uint32_t(player) * 8);
}

std::uint8_t role(Cpu& c, std::uint32_t formation, std::int8_t slot) {
    return c.u8(formation + field::role + std::uint32_t(std::int32_t(slot)));
}

std::uint32_t staff_record(Cpu& c, std::uint32_t formation, std::int8_t slot) {
    const auto index = Record(c, formation).staff(slot);
    c.compare(0, index, -1);
    if (index <= -1) return 0;
    return c.u32(c.toc(staff_slot)) + std::uint32_t(index) * staff_size;
}

std::uint32_t set_staff(Cpu& c, std::uint32_t formation, std::int8_t slot, std::uint32_t staff) {
    c.compare_logical(0, staff, 0);
    const auto value = staff ? c.u32(staff) : 0xffffffffu;
    const auto entry = formation + std::uint32_t(std::int32_t(slot)) * 4;
    c.put32(entry + field::player, value);
    c.put8(formation + field::saved, 0);
    c.set({{4, entry}, {5, value}});
    return 1;
}

std::uint32_t clear_staff(Cpu& c, std::uint32_t formation) {
    std::uint32_t last = 0;
    for (std::int8_t slot = 0; slot < players; ++slot) {
        const auto role = formation + field::role + std::uint32_t(slot);
        last = std::uint32_t(std::int32_t(c.i8(role)));
        if (test(c, last & 0x4) && test(c, last & 0x8)) c.put8(role, 2);
        c.put32(formation + field::player + std::uint32_t(slot) * 4, 0xffffffffu);
    }
    // The loop's registers at its end, and its last compare (11 with 11).
    c.set({{4, 0xffffffffu}, {5, 2}, {6, last}, {7, 4 * players}, {8, field::role + players - 1}, {9, players}});
    c.compare(0, players, players);
    return 1;
}

void update_shape(Cpu& c, std::uint32_t formation) {
    Frame frame(c, 0x10114c00, 0x50, {.first_gpr = 29});
    const Record record(c, formation);
    const auto shape = formation + field::shape;
    c.put32(shape, 0);
    for (std::int8_t player = 0; player < players; ++player)
        c.put32(shape, c.u32(shape) | shape_bit(c, record.position(player)));
    c.set(4, std::uint32_t(std::int32_t(record.position(players - 1))));
    // Each alias is tested as `subis r0,code,high; cmplwi r0,low`.
    const auto code = c.u32(shape);
    for (const auto& [alias, canonical] : shape_aliases) {
        c.compare_logical(0, code - (alias & 0xffff0000u), alias & 0xffffu);
        if (code == alias) { c.put32(shape, canonical); break; }
    }
}

std::uint32_t shape_bit(Cpu& c, std::int16_t position) {
    // One bit per (line, side) pair, found by the original's binary search
    // (whose last compare stays in cr0). 0x802 shares 0x804's bit in the
    // original; nothing gets 0x20.
    const std::int32_t p = position;
    const auto at = [&](std::int32_t value) { c.compare(0, p, value); return p < value ? -1 : p > value ? 1 : 0; };
    const auto leaf = [&](std::int32_t value, std::uint32_t bit) { return at(value) == 0 ? bit : 0u; };
    if (const auto r = at(0x208); r == 0) return 0x2000;
    else if (r < 0) {
        if (const auto s = at(0x102); s == 0) return 0x4;
        else if (s < 0) {
            if (const auto u = at(0x88); u == 0) return 0x800;
            else if (u < 0) {
                if (const auto v = at(0x82); v == 0) return 0x2;
                else if (v < 0) return leaf(0x1, 0x1);
                return leaf(0x84, 0x40);
            }
            if (const auto v = at(0xa0); v == 0) return 0x200000;
            else if (v < 0) return leaf(0x90, 0x10000);
            return leaf(0xc0, 0x4000000);
        }
        if (const auto u = at(0x120); u == 0) return 0x400000;
        else if (u < 0) {
            if (const auto v = at(0x108); v == 0) return 0x1000;
            else if (v < 0) return leaf(0x104, 0x80);
            return leaf(0x110, 0x20000);
        }
        if (const auto v = at(0x202); v == 0) return 0x8;
        else if (v < 0) return leaf(0x140, 0x8000000);
        return leaf(0x204, 0x100);
    }
    if (const auto s = at(0x420); s == 0) return 0x1000000;
    else if (s < 0) {
        if (const auto u = at(0x402); u == 0) return 0x10;
        else if (u < 0) {
            if (const auto v = at(0x220); v == 0) return 0x800000;
            else if (v < 0) return leaf(0x210, 0x40000);
            return leaf(0x240, 0x10000000);
        }
        if (const auto v = at(0x408); v == 0) return 0x4000;
        else if (v < 0) return leaf(0x404, 0x200);
        return leaf(0x410, 0x80000);
    }
    if (const auto u = at(0x808); u == 0) return 0x8000;
    else if (u < 0) {
        if (const auto v = at(0x802); v == 0) return 0x400;
        else if (v < 0) return leaf(0x440, 0x20000000);
        return leaf(0x804, 0x400);
    }
    if (const auto v = at(0x820); v == 0) return 0x2000000;
    else if (v < 0) return leaf(0x810, 0x100000);
    return leaf(0x840, 0x40000000);
}

std::uint32_t split_position(Cpu& c, std::int16_t position, std::uint32_t line, std::uint32_t side) {
    Frame frame(c, 0x10116780, 0xa0, {.first_gpr = 29});
    const auto fail = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t number) {
        c.set({{31, c.toc(pool_slot)}, {30, c.toc(message_slot)}, {29, c.toc(-0x7bac)}});
        guest::report_assertion(c, assertion(c, at, frame, name, extension, number));
        return 0u;
    };
    const auto code = std::uint32_t(std::int32_t(position));
    c.set(6, code & 1);
    // Tested in order, each with a record form into cr0: the line bits,
    // then the sides, a goalkeeper (bit 0x1, in r6) counting as central.
    std::uint16_t line_bit = 0;
    for (std::uint16_t bit = 0x1; bit <= 0x40 && !line_bit; bit <<= 1)
        if (test(c, code & bit)) line_bit = bit;
    if (!line_bit) return fail(0x10116848, 0x68, 0x40, 0xc13);
    c.put16(line, line_bit);
    std::uint16_t side_bit = 0;
    if (test(c, code & 0x80)) side_bit = 0x80;
    else if (test(c, code & 0x100)) side_bit = 0x100;
    else if (test(c, code & 0x200) || test(c, code & 1)) side_bit = 0x200;
    else if (test(c, code & 0x400)) side_bit = 0x400;
    else if (test(c, code & 0x800)) side_bit = 0x800;
    if (!side_bit) return fail(0x1011690c, 0x48, 0x38, 0xc3c);
    c.put16(side, side_bit);
    return 1;
}

std::int32_t position_order(Cpu& c, std::int16_t position) {
    Frame frame(c, 0x101171b0, 0xa0, {.first_gpr = 29});
    const auto fail = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t number) {
        c.set({{31, c.toc(-0x7bac)}, {30, c.toc(pool_slot)}, {29, c.toc(message_slot)}});
        guest::report_assertion(c, assertion(c, at, frame, name, extension, number));
        return 0;
    };
    const auto code = std::uint32_t(std::int32_t(position));
    c.set({{4, code}, {5, code & 1}});
    std::int32_t order = -1;
    for (std::int32_t line = 0; line < 7 && order < 0; ++line)
        if (test(c, code & (1u << line))) order = line * 10;
    if (order < 0) return fail(0x1011725c, 0x68, 0x40, 0xe15);
    if (test(c, code & 0x80)) return order;
    if (test(c, code & 0x100)) return order + 2;
    if (test(c, code & 0x200) || test(c, code & 1)) return order + 3;
    if (test(c, code & 0x400)) return order + 4;
    if (test(c, code & 0x800)) return order + 1;
    return fail(0x10117304, 0x48, 0x38, 0xe3e);
}
}
