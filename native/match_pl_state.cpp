#include "match_pl_detail.hpp"
#include <array>

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t initialize(Cpu& c, std::uint32_t player) {
    // Frameless stmw: the caller can subsequently read this red-zone image.
    for (unsigned reg = 18; reg < 32; ++reg)
        c.put32(c.sp() - 0x38 + (reg - 18) * 4, c.r(reg));
    const auto constants = c.u32(c.r(2) - 0x51e0);
    const auto zero = c.u32(constants);
    c.put32(player + 0x1a4, 0);
    c.put8(player + 0x19, 0xfb);
    for (const auto offset : {0x27u, 0x105u, 0x106u}) c.put8(player + offset, 0);
    for (const auto offset : {0x60u, 0x64u, 0x68u}) c.put32(player + offset, UINT32_MAX);
    c.put32(player + 0x6c, 0); c.put32(player + 0x70, 0);
    c.put8(player, 0xff);
    for (const auto offset : {1u, 0x24u, 0x23u, 0x25u, 0x26u, 0x1cu}) c.put8(player + offset, 0);
    c.put32(player + 0x58, 0); c.put16(player + 0x38, 0);
    c.put8(player + 0x102, 0); c.put32(player + 0x1ac, 0);
    c.put32(player + 0x1a8, zero); c.put8(player + 0x1a, 0); c.put32(player + 0x1b0, 0);
    for (unsigned index = 0; index < 3; ++index) c.put32(player + 0x1b4 + index * 4, 0);
    // Keep the second load, even if a supplied record aliases the constants.
    const auto row_zero = c.u32(constants);
    c.put8(player + 0x104, 0); c.put8(player + 0x101, 0);
    c.put32(player + 0x50, row_zero);
    c.put16(player + 0x2a, 0); c.put16(player + 0x1a0, 0);
    c.put32(player + 0xfc, 0); c.put16(player + 0x1c0, 0);
    c.put8(player + 0x2c, 0xff); c.put8(player + 0x28, 0xff);
    for (unsigned offset = 1; offset <= 0x16; ++offset) c.put8(player + offset, 0);
    c.put8(player + 0x1b, 6); c.put16(player + 0x36, 6100); c.put8(player + 0x100, 0);
    for (unsigned reg = 18; reg < 32; ++reg)
        c.set(reg, c.u32(c.sp() - 0x38 + (reg - 18) * 4));
    compare(c, 3, 3);
    leave(c, {{4, 6100}, {5, 6}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0}});
    return result(c, player);
}

std::uint32_t destroy(Cpu& c, std::uint32_t player, std::int16_t release) {
    Frame frame(c, 0x102270f0, 0x40, {.first_gpr = 31});
    Kept<31> retained(c, player);
    if (compare(c, std::int32_t(player)) != 0) {
        c.put32(player + 0x1a4, 0);
        c.set(3, 0);
        if (compare(c, release) > 0) {
            c.set(3, retained);
            c.call(0x107735a0, 0x10227120);
        }
    }
    return result(c, retained);
}

void clear_match_target(Cpu& c, std::uint32_t player) {
    const Player actor{c, player};
    c.put32(actor.match() + 0x310c, 0);
    const auto match = actor.match();
    c.put8(match + 0x7924, 0xff);
    c.put32(actor.match() + 0xe000, 0);
    c.put8(actor.match() + 0x1c0, 0);
    c.put16(player + 0x2a, 0);
    c.put16(player + 0x1a0, 0);
    leave(c, {{4, 0}, {5, actor.match()}, {6, 0}, {7, 0}, {8, match}});
}

void acquire_ball(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x1022e3a0, 0x40, {.first_gpr = 31});
    Kept<31> retained(c, player);
    const Player actor{c, player};
    const auto previous = c.u32(actor.match() + 0xe000);
    c.set(3, previous);
    if (compare(c, std::int32_t(previous)) != 0) {
        enter(c, 0x1022e3cc, {previous}); clear_match_target(c, previous);
    }
    c.put8(actor.match() + 0x1bd, 0);
    c.put8(actor.match() + 0x1c0, 0);
    c.put8(actor.match() + 0x1e6, 0);
    c.put32(actor.match() + 0x310c, 0);
    leave(c, {{4, 0}, {5, 0}});
    if (compare_logical(c, c.u32(actor.match() + 0xdff4)) != 0) {
        enter(c, 0x1022e420, {c.u32(actor.match() + 0xdff4)}); side(c, c.r(3));
        c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
        if (compare(c, std::int8_t(actor.side()), std::int32_t(c.r(3))) == 0) {
            const auto receiver = c.u32(actor.match() + 0xdff4);
            c.put8(receiver + 2, c.u8(receiver + 2) + 1);
            leave(c, {{3, actor.match()}, {4, retained}, {5, receiver}});
            c.call(0x101e3d50, 0x1022e458);
            unsigned reward = 120;
            if (compare(c, std::int16_t(c.r(3)), 1) == 0) reward = 108;
            else {
                enter(c, 0x1022e488, {retained});
                in_own_half(c, retained);
                if (compare(c, std::int8_t(c.r(3))) != 0) reward = 96;
                else {
                    enter(c, 0x1022e4b4, {retained});
                    in_attacking_third(c, retained);
                    if (compare(c, std::int8_t(c.r(3))) != 0) reward = 150;
                }
            }
            const auto target = c.u32(actor.match() + 0xdff4);
            c.put16(target + 0x36, std::uint32_t(c.i16(target + 0x36)) + reward);
            const auto match_at_reward = actor.match();
            c.put32(match_at_reward + 0x3108, c.u32(match_at_reward + 0x3108) + 240);
            const auto receiver_at_reward = c.u32(actor.match() + 0xdff4);
            const auto match_at_publication = actor.match();
            c.put32(match_at_publication + 0xe024, receiver_at_reward);
            c.put32(match_at_publication + 0xe028, retained);
        }
    }
    c.put32(actor.match() + 0xdff4, 0);
    c.put32(actor.match() + 0xdff8, 0);
    c.put32(actor.match() + 0x3104, 0);
    leave(c, {{3, actor.match()}, {4, retained}, {5, 0}, {6, 0}});
    c.call(0x101e3d50, 0x1022e558);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) {
        enter(c, 0x1022e570, {retained, actor.side()});
        in_goal_area(c, retained, std::int8_t(actor.side()));
        if (compare(c, std::int8_t(c.r(3))) != 0 &&
            compare(c, c.i8(actor.match() + std::uint32_t(std::int32_t(std::int8_t(actor.side()))) + 0x1be)) == 0) {
            c.put16(retained + 0x19e, actor.side() ? 180 : 0);
            c.put8(actor.match() + 0x791f, 0xff);
        }
    }
    enter(c, 0x1022e5c8, {retained}); in_own_half(c, retained);
    if (compare(c, std::int8_t(c.r(3))) != 0) c.put8(actor.match() + 0x7927, 0);

    const auto signed_side = std::uint32_t(std::int32_t(std::int8_t(actor.side())));
    c.put8(actor.match() + signed_side + 0x1be, 1);
    const auto opposite = std::uint32_t(actor.side() == 0);
    c.put8(actor.match() + opposite + 0x1be, 0);
    c.put8(actor.match() + 0x791d, c.u8(retained + 0x105));
    c.put8(actor.match() + 0x791e, c.u8(retained + 0x106));
    c.put8(actor.match() + 0x7924, actor.side());
    c.put32(actor.match() + 0xe000, retained);
    c.put16(actor.match() + 0x7950, std::uint32_t(c.i16(retained + 0x19e)));
    c.put8(actor.match() + 0x7920, 0);
    c.set(4, c.u8(actor.match() + 0x7921));
    if (compare(c, std::int8_t(actor.side()), std::int8_t(c.r(4))) != 0)
        c.put32(actor.match() + 0x3108, 0);
    const auto final_side = actor.side();
    const auto final_match = actor.match();
    c.put8(final_match + 0x7921, final_side);
    leave(c, {{5, final_match}, {6, final_side}});
    if (compare(c, std::int8_t(final_side), -1) != 0) {
        const auto own = final_match + std::uint32_t(std::int32_t(std::int8_t(final_side)));
        c.set(4, own);
        c.put8(own + 0x77bc, c.u8(own + 0x77bc) + 1);
        const auto other = final_match + std::uint32_t(final_side == 0);
        if (compare(c, c.i8(other + 0x77bc)) > 0) {
            c.set(4, other);
            c.put8(other + 0x77bc, c.u8(other + 0x77bc) - 10);
        }
    }
    c.put16(retained + 0x2a, 0); c.put16(retained + 0x1a0, 0);
    c.set(3, 0);
}

void advance_row(Cpu& c, std::uint32_t player, std::int8_t immediate) {
    Frame frame(c, 0x10239e50, 0x40, {});
    const Player actor{c, player};
    auto row = std::uint32_t(c.u8(player + 0x106));
    c.set(5, row);
    const bool reversed = compare(c, actor.side(), 1) == 0;
    if (reversed) {
        if (compare(c, std::int8_t(row), 11) < 0) row = std::uint32_t(std::int32_t(std::int8_t(row + 1)));
    } else if (compare(c, std::int8_t(row)) > 0) --row;
    c.set(5, row);
    const bool direct = compare(c, immediate) != 0;
    c.set(4, c.u8(player + 0x105));
    if (direct) {
        const auto destination = c.u8(actor.match() + (reversed ? 0x7926 : 0x7925));
        c.set(5, std::uint32_t(std::int32_t(std::int8_t(destination + (reversed ? 1u : UINT32_MAX)))));
        enter(c, reversed ? 0x10239ea0 : 0x10239ee8, {player, c.r(4), c.r(5)});
        set_coordinates(c, player, c.r(4), c.r(5));
    } else {
        leave(c, {{6, UINT32_MAX}, {7, 0}, {8, 0}});
        c.call(0x1022e7e0, reversed ? 0x10239eb8 : 0x10239f00);
    }
}
}
