#include "formation.hpp"
#include <array>
#include <bit>
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
}

std::int32_t position(Cpu& c, std::uint32_t formation, std::int8_t player, bool with_ball) {
    Frame frame(c, 0x10111240, 0x70, {.first_gpr = 30});
    if (!valid(player)) {
        c.set({{31, c.toc(pool_slot)}, {30, c.toc(message_slot)}});
        guest::report_assertion(c, assertion(c, 0x10111284, frame, 0x40, 0x38, 0xfb));
        return 1;
    }
    return Record(c, formation).position(player, with_ball);
}

std::int32_t player_at(Cpu& c, std::uint32_t formation, std::int16_t position) {
    const Record record(c, formation);
    for (std::int8_t player = 0; player < players; ++player)
        if (record.position(player) == position) return player;
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
    if (index <= -1) return 0;
    return c.u32(c.toc(staff_slot)) + std::uint32_t(index) * staff_size;
}

std::uint32_t set_staff(Cpu& c, std::uint32_t formation, std::int8_t slot, std::uint32_t staff) {
    c.put32(formation + field::player + std::uint32_t(std::int32_t(slot)) * 4, staff ? c.u32(staff) : 0xffffffffu);
    c.put8(formation + field::saved, 0);
    return 1;
}

std::uint32_t clear_staff(Cpu& c, std::uint32_t formation) {
    for (std::int8_t slot = 0; slot < players; ++slot) {
        const auto role = formation + field::role + std::uint32_t(slot);
        if ((c.u8(role) & 0x4) && (c.u8(role) & 0x8)) c.put8(role, 2);
        c.put32(formation + field::player + std::uint32_t(slot) * 4, 0xffffffffu);
    }
    return 1;
}

void update_shape(Cpu& c, std::uint32_t formation) {
    Frame frame(c, 0x10114c00, 0x50, {.first_gpr = 29});
    const Record record(c, formation);
    const auto shape = formation + field::shape;
    c.put32(shape, 0);
    for (std::int8_t player = 0; player < players; ++player)
        c.put32(shape, c.u32(shape) | shape_bit(record.position(player)));
    const auto code = c.u32(shape);
    for (const auto& [alias, canonical] : shape_aliases)
        if (code == alias) { c.put32(shape, canonical); break; }
}

std::uint32_t shape_bit(std::int16_t position) {
    // One bit per (line, side) pair. 0x802 shares 0x804's bit in the
    // original; nothing gets 0x20.
    switch (position) {
    case 0x001: return 0x1;
    case 0x082: return 0x2;       case 0x102: return 0x4;        case 0x202: return 0x8;
    case 0x402: return 0x10;      case 0x802: return 0x400;
    case 0x084: return 0x40;      case 0x104: return 0x80;       case 0x204: return 0x100;
    case 0x404: return 0x200;     case 0x804: return 0x400;
    case 0x088: return 0x800;     case 0x108: return 0x1000;     case 0x208: return 0x2000;
    case 0x408: return 0x4000;    case 0x808: return 0x8000;
    case 0x090: return 0x10000;   case 0x110: return 0x20000;    case 0x210: return 0x40000;
    case 0x410: return 0x80000;   case 0x810: return 0x100000;
    case 0x0a0: return 0x200000;  case 0x120: return 0x400000;   case 0x220: return 0x800000;
    case 0x420: return 0x1000000; case 0x820: return 0x2000000;
    case 0x0c0: return 0x4000000; case 0x140: return 0x8000000;  case 0x240: return 0x10000000;
    case 0x440: return 0x20000000; case 0x840: return 0x40000000;
    default: return 0;
    }
}

namespace {
// The line bit of a position (lowest of 0x1..0x40), and the side bit, a
// goalkeeper counting as central (0x200); 0 when missing.
std::uint16_t line_of(std::int16_t position) {
    for (std::uint16_t bit = 0x1; bit <= 0x40; bit <<= 1)
        if (std::uint16_t(position) & bit) return bit;
    return 0;
}
std::uint16_t side_of(std::int16_t position) {
    const auto code = std::uint16_t(position);
    if (code & 0x80) return 0x80;
    if (code & 0x100) return 0x100;
    if ((code & 0x200) || (code & 0x1)) return 0x200;
    if (code & 0x400) return 0x400;
    if (code & 0x800) return 0x800;
    return 0;
}
}

std::uint32_t split_position(Cpu& c, std::int16_t position, std::uint32_t line, std::uint32_t side) {
    Frame frame(c, 0x10116780, 0xa0, {.first_gpr = 29});
    const auto fail = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t number) {
        c.set({{31, c.toc(pool_slot)}, {30, c.toc(message_slot)}, {29, c.toc(-0x7bac)}});
        guest::report_assertion(c, assertion(c, at, frame, name, extension, number));
        return 0u;
    };
    const auto line_bit = line_of(position);
    if (!line_bit) return fail(0x10116848, 0x68, 0x40, 0xc13);
    c.put16(line, line_bit);
    const auto side_bit = side_of(position);
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
    const auto line_bit = line_of(position);
    if (!line_bit) return fail(0x1011725c, 0x68, 0x40, 0xe15);
    const auto order = std::int32_t(std::countr_zero(line_bit)) * 10;
    switch (side_of(position)) {
    case 0x80: return order;
    case 0x100: return order + 2;
    case 0x200: return order + 3;
    case 0x400: return order + 4;
    case 0x800: return order + 1;
    default: return fail(0x10117304, 0x48, 0x38, 0xe3e);
    }
}
}
