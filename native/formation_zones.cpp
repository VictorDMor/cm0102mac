#include "formation_zones.hpp"
#include <algorithm>
#include <array>

namespace cm::aot::formation {
using guest::Cpu;
using guest::Frame;
namespace {
// FORMATION.CPP's string pool, the shared message buffer and success flag (TOC slots).
constexpr std::int32_t pool_slot = -0x550c, message_slot = -0x7ba8, success_slot = -0x7bac;
guest::Assertion assertion(Cpu& c, std::uint32_t at, const Frame& frame, std::int32_t name, std::int32_t extension,
                           std::uint32_t line) {
    const auto pool = c.toc(pool_slot);
    return {at, pool + 0xe, pool + 0x1c, pool + 0x29, frame.at(name), frame.at(extension), line};
}
void report(Cpu& c, const Frame& frame, std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
    guest::report_assertion(c, assertion(c, at, frame, name, extension, line));
}
// The same, for the sites that clear the flag through r4 (`lwz r4,-0x7bac(r2)`).
void report_r4(Cpu& c, const Frame& frame, std::uint32_t at, std::int32_t name, std::int32_t extension,
               std::uint32_t line) {
    report(c, frame, at, name, extension, line);
    c.set(4, c.toc(success_slot));
}
bool valid(std::int8_t player) { return player >= 0 && player < players; }
// CTR and XER[CA] as the original's last bdnz / carrying shift leave them.
void set_ctr(Cpu& c, std::uint32_t value) { c.s.wr<4>(guest::reg::ctr, value); }
void set_carry(Cpu& c, bool carry) { c.s.wr<1>(guest::reg::xer_ca, carry ? 1 : 0); }
bool valid_ball(std::int8_t with_ball) { return with_ball >= 0 && with_ball <= 1; }
// A signed byte or halfword as the 32-bit register `extsb` / `extsh` / `lha` leave.
std::uint32_t wide(std::int32_t value) { return std::uint32_t(value); }
std::uint16_t row_of(std::uint32_t cell) { return std::uint16_t(cell >> 16); }
std::uint16_t column_of(std::uint32_t cell) { return std::uint16_t(cell); }
bool valid_cell(std::uint32_t cell) { return row_of(cell) < spot_rows && column_of(cell) < spot_columns; }
// The spot of a player for a ball state and cell, relative to the record.
std::uint32_t spot_offset(std::int8_t player, std::int8_t with_ball, std::uint32_t row, std::uint32_t column) {
    return field::spots + wide(player) * field::players_spots + wide(with_ball) * field::ball_spots + row * 16 +
           column * 4;
}
std::uint32_t position_at(std::uint32_t formation, std::int8_t player, bool with_ball = false) {
    return formation + (with_ball ? field::position_with_ball : field::position) + wide(player) * 2;
}
// The original's abs (0x10775550, a leaf without stack stores, run in line).
std::uint32_t absolute(std::uint32_t value) {
    const auto sign = std::uint32_t(std::int32_t(value) >> 31);
    return (value ^ sign) - sign;
}
// Swaps `size` bytes (0x10745e00), compares them (0x10778390, memcmp), clears
// them (0x10778300). The caller sets r6-r10 as the original has them first.
void swap_bytes(Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t size, std::uint32_t return_address) {
    c.set({{3, a}, {4, b}, {5, size}});
    c.call(0x10745e00, return_address);
}
std::int32_t compare_bytes(Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t size, std::uint32_t return_address) {
    c.set({{3, a}, {4, b}, {5, size}});
    c.call(0x10778390, return_address);
    return std::int32_t(c.r(3));
}
void clear_bytes(Cpu& c, std::uint32_t at, std::uint32_t size, std::uint32_t return_address) {
    c.set({{3, at}, {4, 0}, {5, size}});
    c.call(0x10778300, return_address);
}
// Marks `player` as the first holder of `position` and every other player
// holding it as not first (the original's loop after each position change).
// Returns the position of the last other player it compared (the loop's last
// load); the loop ends with cr0 equal (its counter at 11).
std::int16_t claim_position(Cpu& c, std::uint32_t formation, std::int8_t player, auto&& position) {
    c.put8(formation + field::position_set + wide(player), 1);
    std::int16_t last = 0;
    for (std::int8_t other = 0; other < players; ++other) {
        if (other == player) continue;
        last = c.i16(position_at(formation, other));
        if (last == position()) c.put8(formation + field::position_set + wide(other), 0);
    }
    c.compare(0, players, players);
    return last;
}
}

std::uint32_t set_position(Cpu& c, std::uint32_t formation, std::uint32_t player_register, std::uint32_t position_register,
                           std::int8_t with_ball) {
    Frame frame(c, 0x10111310, 0x90, {.first_gpr = 26});
    const auto player = std::int8_t(player_register);
    const auto position = std::int16_t(position_register);
    c.set({{26, c.toc(message_slot)}, {27, formation}, {28, position_register}, {29, player_register},
           {31, c.toc(pool_slot)}});
    if (player >= 0) c.set(30, wide(player));
    if (!valid(player)) {
        report_r4(c, frame, 0x1011135c, 0x50, 0x48, 0x118);
        return 0;
    }
    if (with_ball) {
        c.put16(position_at(formation, player, true), position_register);
    } else {
        c.set(31, wide(player) * 2);
        if (const auto old = c.i16(position_at(formation, player)); old != position) {
            c.put16(position_at(formation, player), position_register);
            c.set({{4, wide(old)}, {5, position_at(formation, player) - field::position}});
            c.link(0x101113dc);
            update_shape(c, formation);
            claim_position(c, formation, player, [&] { return position; });
            c.set({{6, wide(players - 1)}, {7, wide(players) * 2}, {8, wide(players)}});
        }
        c.put16(position_at(formation, player, true), position_register);
    }

    // Each cell's spot without the ball comes from the position nearer the
    // own goal (the lower line), the spot with the ball from the other one.
    const auto cell_at = frame.at(0x44), spot_at = frame.at(0x40);
    c.set(31, formation + wide(player) * 2);
    for (std::uint16_t row = 0; row < spot_rows; ++row) {
        for (std::uint16_t column = 0; column < spot_columns; ++column) {
            c.put16(cell_at, row);
            c.put16(cell_at + 2, column);
            const auto here = c.i16(position_at(formation, player));
            const auto ball = c.i16(position_at(formation, player, true));
            c.set({{28, column}, {29, row}});
            auto first = here, second = here;
            std::uint32_t first_return = 0x1011148c, second_return = 0x101114d0;
            if (here != ball) {
                c.set({{3, wide(here)}, {4, frame.at(0x3c)}, {5, frame.at(0x3a)}});
                c.link(0x101114f0);
                split_position(c, here, frame.at(0x3c), frame.at(0x3a));
                c.set({{3, wide(ball)}, {4, frame.at(0x38)}, {5, frame.at(0x3a)}});
                c.link(0x10111500);
                split_position(c, ball, frame.at(0x38), frame.at(0x3a));
                // The lines are read back even when splitting failed.
                if (c.i16(frame.at(0x38)) < c.i16(frame.at(0x3c))) {
                    first = ball, second = here;
                    first_return = 0x10111590, second_return = 0x101115d4;
                } else {
                    second = ball;
                    first_return = 0x10111524, second_return = 0x10111568;
                }
            }
            c.set({{3, wide(first)}, {4, 0}, {5, c.u32(cell_at)}, {6, spot_at}});
            c.link(first_return);
            default_spot(c, first, 0, c.u32(cell_at), spot_at);
            const auto zone = formation + spot_offset(player, 0, c.u16(cell_at), c.u16(cell_at + 2));
            c.put16(zone, c.u16(spot_at));
            c.put16(zone + 2, c.u16(spot_at + 2));
            c.set(26, zone - field::spots);
            c.set({{3, wide(second)}, {4, 1}, {5, c.u32(cell_at)}, {6, spot_at}});
            c.link(second_return);
            default_spot(c, second, 1, c.u32(cell_at), spot_at);
            c.put16(zone + field::ball_spots, c.u16(spot_at));
            c.put16(zone + field::ball_spots + 2, c.u16(spot_at + 2));
        }
    }
    c.put8(formation + field::saved, 0);
    c.compare(0, spot_rows, spot_rows);
    return 1;
}

std::uint32_t central_in_line(Cpu& c, std::uint32_t formation, std::int16_t position, std::uint32_t table) {
    Frame frame(c, 0x10111630, 0x50, {.first_gpr = 30});
    c.set({{30, formation}, {31, table}, {3, wide(position)}, {4, frame.at(0x3a)}, {5, frame.at(0x38)}});
    c.link(0x10111660);
    split_position(c, position, frame.at(0x3a), frame.at(0x38));
    const auto line = c.i16(frame.at(0x3a));  // read back even when splitting failed
    // Registers as the original's loops leave them (r4-r6, cr0).
    const auto leave = [&](std::uint32_t r4, std::uint32_t r5, std::uint32_t r6) { c.set({{4, r4}, {5, r5}, {6, r6}}); };
    for (std::int8_t player = 0; player < players; ++player) {
        const auto code = c.i16(position_at(formation, player));
        if (!(code & line) || !(code & 0x200)) continue;
        if (!table) {
            leave(wide(player) * 2, wide(code), wide(player));
            c.compare_logical(0, 0, 0);
            set_ctr(c, wide(players - player));
            return 1;
        }
        std::int8_t slot = 0;
        for (std::uint32_t entry = 0; entry < 20; ++entry) {
            const auto at = table + entry * 0x1c4;
            leave(entry * 0x1c4, entry / 5 * 4, at);
            set_ctr(c, 4 - entry / 5);
            slot = c.i8(at + 0x19);
            if (slot == player) {
                c.compare(0, player, slot);
                return 1;
            }
            c.compare(0, slot, -2);
            if (slot != -2) continue;
            slot = c.i8(at + 0x1a);
            c.compare(0, player, slot);
            if (slot == player) return 1;
        }
        leave(20 * 0x1c4, 16, table + 19 * 0x1c4);
        set_ctr(c, 0);
        return 0;
    }
    set_ctr(c, 0);
    leave(wide(players) * 2, wide(c.i16(position_at(formation, players - 1))), wide(players));
    c.compare(0, 0, 0);
    return 0;
}

std::uint32_t swap_players(Cpu& c, std::uint32_t formation, std::uint32_t first_register, std::uint32_t second_register) {
    Frame frame(c, 0x10111890, 0xc0, {.first_gpr = 19});
    const auto a = std::int8_t(first_register), b = std::int8_t(second_register);
    c.set({{23, formation}, {24, first_register}, {25, second_register}, {26, c.toc(pool_slot)},
           {27, c.toc(message_slot)}, {28, c.toc(success_slot)}});
    if (a >= 0) c.set(30, wide(a));
    if (!valid(a)) {
        report(c, frame, 0x101118e0, 0x68, 0x40, 0x1d8);
        return 0;
    }
    if (b >= 0) c.set(31, wide(b));
    if (!valid(b)) {
        report(c, frame, 0x1011194c, 0x48, 0x38, 0x1e0);
        return 0;
    }
    const auto a_position = position_at(formation, a), b_position = position_at(formation, b);
    const auto old_a = c.i16(a_position);
    c.set({{26, wide(a) * 2}, {27, b_position - field::position}, {28, wide(b) * 2}, {29, wide(old_a)}});
    c.set(4, a_position - field::position);
    if (old_a != c.i16(b_position)) {
        c.put16(a_position, c.u16(b_position));
        c.link(0x101119b8);
        update_shape(c, formation);
        const auto last = claim_position(c, formation, a, [&] { return c.i16(b_position); });
        c.set({{3, 0}, {4, b_position - formation}, {5, wide(last)}, {6, wide(a)}, {7, wide(players - 1)},
               {8, wide(players) * 2}, {9, wide(players)}});
    }
    if (c.i16(b_position) != old_a) {
        c.put16(b_position, std::uint16_t(old_a));
        c.set(3, formation);
        c.link(0x10111a30);
        update_shape(c, formation);
        claim_position(c, formation, b, [&] { return old_a; });
        c.set({{3, 0}, {4, wide(b)}, {5, wide(players - 1)}, {6, wide(players) * 2}, {7, wide(players)}});
    }
    swap_bytes(c, position_at(formation, a, true), position_at(formation, b, true), 2, 0x10111aa0);

    const auto a_spots = wide(a) * field::players_spots, b_spots = wide(b) * field::players_spots;
    c.set({{25, b_spots}, {27, a_spots}});
    for (std::uint32_t row = 0; row < spot_rows; ++row) {
        c.set({{20, row}, {29, row * 16}, {26, a_spots + row * 16}, {24, b_spots + row * 16}});
        for (std::uint32_t column = 0; column < spot_columns; ++column) {
            const auto a_spot = formation + a_spots + row * 16 + column * 4;
            const auto b_spot = formation + b_spots + row * 16 + column * 4;
            c.set({{19, column}, {28, column * 4}, {21, a_spot}, {22, b_spot}});
            swap_bytes(c, a_spot + field::spots, b_spot + field::spots, 4, 0x10111ae8);
            swap_bytes(c, a_spot + field::spots + field::ball_spots, b_spot + field::spots + field::ball_spots, 4,
                       0x10111af8);
        }
        c.set({{19, spot_columns}, {28, spot_columns * 4}});
    }
    c.set({{20, spot_rows}, {29, spot_rows * 16}});
    swap_bytes(c, formation + field::instructions + wide(a) * 8, formation + field::instructions + wide(b) * 8, 8,
               0x10111b40);
    swap_bytes(c, formation + field::role + wide(a), formation + field::role + wide(b), 1, 0x10111b58);
    swap_bytes(c, formation + field::player + wide(a) * 4, formation + field::player + wide(b) * 4, 4, 0x10111b78);
    c.put8(formation + field::saved, 0);
    return 1;
}

std::uint32_t order_players(Cpu& c, std::uint32_t formation, std::uint32_t order, std::int32_t size) {
    Frame frame(c, 0x10111ba0, 0xc0, {.first_gpr = 24});
    c.set({{24, order}, {28, c.toc(pool_slot)}, {29, c.toc(message_slot)}, {30, c.toc(success_slot)},
           {31, formation}});
    if (!order) {
        report(c, frame, 0x10111bdc, 0x74, 0x40, 0x21a);
        return 0;
    }
    if (size < players) {
        report(c, frame, 0x10111c3c, 0x54, 0x38, 0x222);
        return 0;
    }
    const auto taken = frame.at(0x48);
    clear_bytes(c, taken, players, 0x10111c90);
    c.set(28, taken);
    for (std::uint32_t slot = 0; slot < std::uint32_t(players); ++slot) {
        // Picks the untaken player first in team order (the first of equals);
        // when none is left the slot keeps what it held.
        std::uint32_t best = 1000;
        for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
            if (c.i8(taken + player)) continue;
            const auto position = c.i16(position_at(formation, std::int8_t(player)));
            c.set({{24, best}, {25, player}, {26, slot}, {27, player * 2}, {29, taken + player}, {30, order + slot},
                   {3, wide(position)}});
            c.link(0x10111cc4);
            const auto rank = std::uint32_t(position_order(c, position));
            c.set({{3, rank}, {4, wide(std::int16_t(rank))}});
            if (std::int16_t(rank) < std::int16_t(best)) {
                c.put8(order + slot, player);
                best = rank;
            }
        }
        c.put8(taken + wide(c.i8(order + slot)), 1);
    }
    c.compare(0, players, players);
    return 1;
}

std::uint32_t displaced(Cpu& c, std::uint32_t formation, std::int8_t player, std::uint32_t out) {
    Frame frame(c, 0x10111d30, 0x70, {.first_gpr = 30});
    c.set({{30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!valid(player)) {
        report_r4(c, frame, 0x10111d74, 0x40, 0x38, 0x274);
        return 0;
    }
    c.put8(out, 0);
    const auto set = formation + field::position_set;
    // Registers as the original's loop leaves them: r6 the other's position,
    // r7 the player, r8/r9 the loop's offset and index.
    std::int16_t theirs = 0, mine = 0;
    for (std::int8_t other = 0; other < players; ++other) {
        if (other == player) {
            c.compare(0, other, player);
            continue;
        }
        mine = c.i16(position_at(formation, player));
        theirs = c.i16(position_at(formation, other));
        c.compare(0, theirs, mine);
        if (theirs != mine) continue;
        const auto first = c.i8(set + wide(other)), own = c.i8(set + wide(player));
        c.set({{4, c.u8(set + wide(other))}, {6, wide(theirs)}, {7, wide(player)}, {8, wide(other) * 2},
               {9, wide(other)}});
        c.compare(0, first, own);
        set_ctr(c, wide(players - other));
        if (first < own) {
            c.put8(out, 1);
            return 1;
        }
        return 0;
    }
    c.set({{4, position_at(formation, player) - formation}, {6, wide(theirs)}, {7, wide(player)},
           {8, wide(players) * 2}, {9, wide(players)}});
    set_ctr(c, 0);
    return 1;
}

std::uint32_t move_spot(Cpu& c, std::uint32_t formation, std::uint32_t player_register, std::int8_t with_ball,
                        std::uint32_t cell, std::uint32_t spot) {
    Frame frame(c, 0x10111e60, 0x170, {.first_gpr = 13});
    // The cell and spot arguments live in the caller's frame; the search
    // below reuses those slots for its own cell and default spot.
    const auto cell_at = frame.caller() + 0x24, spot_at = frame.caller() + 0x28;
    c.put32(cell_at, cell);
    c.put32(spot_at, spot);
    const auto player = std::int8_t(player_register);
    const auto codes = c.toc(-0x7474);  // every position code, ending with -1
    c.set({{13, player_register}, {14, codes}, {15, c.toc(success_slot)}, {16, c.toc(pool_slot)},
           {17, c.toc(message_slot)}, {31, formation}});
    if (player >= 0) c.set(24, wide(player));
    if (!valid(player)) {
        report(c, frame, 0x10111eb8, 0xcc, 0x5c, 0x2ae);
        return 0;
    }
    if (!valid_ball(with_ball)) {
        report(c, frame, 0x10111f24, 0xac, 0x54, 0x2b6);
        return 0;
    }
    if (!valid_cell(cell)) {
        report(c, frame, 0x10111f94, 0x8c, 0x4c, 0x2bf);
        return 0;
    }
    const auto x = std::uint16_t(spot >> 16), y = std::uint16_t(spot);
    if (x > spot_x_max || y > spot_y_max) {
        report(c, frame, 0x10112004, 0x6c, 0x44, 0x2c8);
        return 0;
    }
    const auto own = formation + spot_offset(player, with_ball, row_of(cell), column_of(cell));
    c.put16(own, x);
    c.put16(own + 2, y);

    // For each ball state, the codes whose default spots lie nearest (in
    // zone-grid steps: the original's y * 12 / 36 and x * 9 / 27, which are
    // y / 3 and x / 3 for unsigned spots) to the player's spots in all twelve
    // cells; a code of the second line (0x2) counts double. Ties are kept in order.
    const auto counts = frame.at(0x38);  // int8[2]
    const auto best = frame.at(0x64);    // int32[2]
    const auto lists = frame.at(0xec);   // int16[2][11]; past 11 ties a list runs on up the frame
    const auto spots = formation + wide(player) * field::players_spots;
    // The multiply-high constants of its divisions by 36 and 27, kept in r26 / r28.
    constexpr std::uint32_t divide36 = 0x38e38e39, divide27 = 0x4bda12f7;
    std::uint32_t sum = 0, zone = c.r(19), distance = c.r(27), next = codes, index = 0;
    c.set({{20, spots}, {25, 0x7fffffff}, {26, divide36}, {28, divide27}, {7, wide(player) * field::players_spots},
           {8, x}});
    for (std::uint32_t ball = 0; ball < 2; ++ball) {
        c.put32(best + ball * 4, 0x7fffffff);
        for (next = codes, index = 0; c.i16(next) != -1; next += 2, index += 2) {
            const auto code = c.i16(next);
            sum = 0;
            for (std::uint32_t row = 0; row < spot_rows; ++row) {
                for (std::uint32_t column = 0; column < spot_columns; ++column) {
                    c.put16(cell_at, row);
                    c.put16(cell_at + 2, column);
                    c.set({{15, ball}, {16, column}, {17, row}, {18, sum}, {19, zone}, {21, ball * 0x16},
                           {22, ball * field::ball_spots}, {23, index}, {27, distance}, {29, next},
                           {30, best + ball * 4}, {3, wide(code)}, {4, ball}, {5, c.u32(cell_at)}, {6, spot_at}});
                    c.link(0x101120e4);
                    default_spot(c, code, ball, c.u32(cell_at), spot_at);
                    zone = spots + ball * field::ball_spots + c.u16(cell_at) * 16u + c.u16(cell_at + 2) * 4u;
                    distance = absolute(std::uint32_t(c.u16(zone + field::spots + 2) / 3) - c.u16(spot_at + 2) / 3);
                    sum += distance + absolute(std::uint32_t(c.u16(zone + field::spots) / 3) - c.u16(spot_at) / 3);
                }
            }
            if (code & 0x2) sum *= 2;
            const auto current = c.i32(best + ball * 4);
            if (std::int32_t(sum) > current) continue;
            if (std::int32_t(sum) < current) c.put8(counts + ball, 0);
            const auto count = c.u8(counts + ball);
            c.put32(best + ball * 4, sum);
            c.put8(counts + ball, count + 1);
            c.put16(lists + ball * 0x16 + wide(std::int8_t(count)) * 2, std::uint16_t(code));
            c.set(7, lists);
        }
    }
    c.put8(formation + field::saved, 0);
    // Registers as the search leaves them, for the calls below.
    c.set({{15, wide(player) * 2}, {16, 4}, {17, 3}, {18, sum}, {19, zone}, {21, 0x2c}, {22, 0x60},
           {23, index}, {27, distance}, {29, next}, {30, best + 8}});

    // The ball state with the smaller best sum (the first on a tie), unless
    // the player's position is in a list (the last list holding it wins).
    const auto best0 = c.u32(best), difference = best0 ^ c.u32(best + 4);
    std::uint32_t chosen = (std::uint32_t(std::int32_t(difference) >> 1) - (difference & best0)) >> 31, choice = 0;
    const auto own_position = position_at(formation, player);
    std::uint32_t scanned = 0, last_load = 0;  // the second list's scan, as r12 / r5 leave it
    for (std::int8_t ball = 0; ball < 2; ++ball) {
        std::int8_t i = 0;
        for (; i < c.i8(counts + wide(ball)); ++i)
            if (c.i16(own_position) == c.i16(lists + wide(ball) * 0x16 + wide(i) * 2)) {
                chosen = wide(ball), choice = wide(i);
                break;
            }
        scanned = wide(i);
        last_load = i < c.i8(counts + wide(ball)) ? wide(c.i16(own_position)) : wide(i);
    }
    c.set({{3, counts}, {4, 1}, {5, last_load}, {6, lists}, {7, own_position - formation}, {8, scanned * 2},
           {9, 0x2c}, {10, choice}, {11, 2}, {12, scanned}});
    c.compare(0, 2, 2);
    const auto picked = wide(std::int8_t(chosen)) * 0x16 + wide(std::int8_t(choice)) * 2;
    const auto old_position = c.i16(own_position);
    if (old_position != c.i16(lists + picked)) {
        c.put16(own_position, c.u16(lists + picked));
        c.set({{14, picked}, {16, chosen}, {17, lists}, {3, formation}, {4, wide(old_position)},
               {5, own_position - field::position}});
        c.link(0x10112308);
        update_shape(c, formation);
        const auto last = claim_position(c, formation, player, [&] { return c.i16(lists + picked); });
        c.set({{3, 0}, {4, wide(last)}, {5, wide(player)}, {6, wide(players - 1)}, {7, wide(players) * 2},
               {8, wide(players)}});
    } else {
        c.set({{4, wide(old_position)}, {5, own_position - field::position}});
        c.compare(0, 0, 0);
    }

    // With the ball: the new position if the other list holds it, else the
    // first code of the other list on the same side, else the code between
    // its two codes, else its first code.
    const auto other = 1 - wide(std::int8_t(chosen));
    const auto list = lists + other * 0x16;
    const auto count = c.i8(counts + other);
    const auto with_ball_at = position_at(formation, player, true);
    for (std::int8_t i = 0; i < count; ++i) {
        const auto here = c.i16(own_position);
        c.set({{5, wide(here)}, {6, own_position - formation}, {7, wide(i) * 2}, {8, list}, {9, wide(i)}});
        if (here == c.i16(list + wide(i) * 2)) {
            c.put16(with_ball_at, c.u16(list + wide(i) * 2));
            c.set(4, own_position - field::position);
            c.compare(0, 0, 0);
            return 1;
        }
    }
    c.set({{3, counts}, {4, wide(count)}, {5, c.u8(counts + other)}, {6, own_position - formation},
           {7, wide(count) * 2}, {8, list}, {9, wide(count)}});
    c.set({{13, own_position - formation}, {14, other * 0x16}, {16, wide(count)}, {18, other}, {20, 0}, {17, 0}});
    for (std::int8_t i = 0; i < count; ++i) {
        c.set({{17, wide(i) * 2}, {19, list}, {20, wide(i)}});
        const auto code = c.i16(list + wide(i) * 2);
        c.set({{3, wide(code)}, {4, frame.at(0x40)}, {5, frame.at(0x3e)}});
        c.link(0x10112408);
        split_position(c, code, frame.at(0x40), frame.at(0x3e));
        const auto here = c.i16(own_position);
        c.set({{3, wide(here)}, {4, frame.at(0x40)}, {5, frame.at(0x3c)}});
        c.link(0x10112418);
        split_position(c, here, frame.at(0x40), frame.at(0x3c));
        if (c.i16(frame.at(0x3e)) == c.i16(frame.at(0x3c))) {
            c.put16(with_ball_at, c.u16(list + wide(i) * 2));
            c.set(4, own_position - field::position);
            c.compare(0, 0, 0);
            return 1;
        }
        c.set({{17, wide(i + 1) * 2}, {20, wide(i + 1)}});
    }
    if (count == 2) {
        // Both codes come from the code list and have a line bit, so the
        // caller's r0 / r5 (passed as the original holds them) never count.
        const auto a = wide(c.i16(list)), b = wide(c.i16(list + 2));
        c.set({{0, other * 0x16}, {3, a}, {4, b}});
        c.link(0x1011247c);
        const auto between = between_positions(c, a, b, other * 0x16, c.r(5));
        c.put16(with_ball_at, std::uint16_t(between));
        c.set(4, own_position - field::position);
        return 1;
    }
    c.put16(with_ball_at, c.u16(list));
    c.set({{4, own_position - field::position}, {5, lists}});
    c.compare(0, count, 2);
    return 1;
}

std::uint32_t spot(Cpu& c, std::uint32_t formation, std::int8_t player, std::int8_t with_ball, std::uint32_t cell,
                   std::uint32_t out) {
    Frame frame(c, 0x101124c0, 0xc0, {.first_gpr = 29});
    c.put32(frame.caller() + 0x24, cell);
    c.set({{29, c.toc(success_slot)}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!valid(player)) {
        report(c, frame, 0x10112514, 0x90, 0x48, 0x34f);
        return 0;
    }
    if (!valid_ball(with_ball)) {
        report(c, frame, 0x10112580, 0x70, 0x40, 0x357);
        return 0;
    }
    if (!valid_cell(cell)) {
        report(c, frame, 0x101125ec, 0x50, 0x38, 0x360);
        return 0;
    }
    const auto at = formation + spot_offset(player, with_ball, row_of(cell), column_of(cell));
    c.put16(out, c.u16(at));
    c.put16(out + 2, c.u16(at + 2));
    const auto player_spots = wide(player) * field::players_spots;
    c.set({{4, at}, {5, player_spots + wide(with_ball) * field::ball_spots}, {6, player_spots}, {7, out},
           {8, column_of(cell)}});
    c.compare_logical(0, column_of(cell), spot_columns - 1);
    return 1;
}

namespace {
// The shared checks of the routines over every player of a cell: ball state,
// cell, then the array. Each entry: the first call's address, the name and
// extension buffers, the line.
struct Check { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
bool checked(Cpu& c, const Frame& frame, bool ok, const Check& check) {
    if (!ok) report(c, frame, check.at, check.name, check.extension, check.line);
    return ok;
}
}

std::uint32_t move_spots(Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t cell,
                         std::uint32_t spots) {
    Frame frame(c, 0x10112690, 0xd0, {.first_gpr = 27});
    const auto cell_at = frame.caller() + 0x20;
    c.put32(cell_at, cell);
    c.set({{27, formation}, {28, with_ball}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)},
           {31, c.toc(pool_slot)}});
    if (!checked(c, frame, valid_ball(std::int8_t(with_ball)), {0x101126e0, 0x90, 0x48, 0x37c}) ||
        !checked(c, frame, valid_cell(c.u32(cell_at)), {0x10112750, 0x70, 0x40, 0x385}) ||
        !checked(c, frame, spots != 0, {0x101127b0, 0x50, 0x38, 0x38d}))
        return 0;
    for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
        const auto cell_now = c.u32(cell_at), spot_now = c.u32(spots + player * 4);
        c.set({{29, player}, {31, spots + player * 4}, {3, formation}, {4, player}, {5, with_ball}, {6, cell_now},
               {7, spot_now}});
        c.link(0x10112818);
        move_spot(c, formation, player, std::int8_t(with_ball), cell_now, spot_now);
    }
    c.put8(formation + field::saved, 0);
    c.compare(0, players, players);
    return 1;
}

std::uint32_t spots(Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t cell, std::uint32_t out) {
    Frame frame(c, 0x10112850, 0xd0, {.first_gpr = 26});
    const auto cell_at = frame.caller() + 0x20;
    c.put32(cell_at, cell);
    c.set({{26, formation}, {27, with_ball}, {28, out}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)},
           {31, c.toc(pool_slot)}});
    if (!checked(c, frame, valid_ball(std::int8_t(with_ball)), {0x101128a4, 0x90, 0x48, 0x3b0}) ||
        !checked(c, frame, valid_cell(c.u32(cell_at)), {0x10112914, 0x70, 0x40, 0x3b9}) ||
        !checked(c, frame, out != 0, {0x10112974, 0x50, 0x38, 0x3c1}))
        return 0;
    for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
        c.set({{29, player * 4}, {30, player}});
        c.link(0x101129d8);
        spot(c, formation, std::int8_t(player), std::int8_t(with_ball), c.u32(cell_at), out + player * 4);
    }
    c.compare(0, players, players);
    return 1;
}

std::uint32_t move_all_spots(Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t spots) {
    Frame frame(c, 0x10112a00, 0xb0, {.first_gpr = 24});
    c.set({{24, formation}, {25, with_ball}, {26, spots}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)},
           {31, c.toc(pool_slot)}});
    if (!checked(c, frame, valid_ball(std::int8_t(with_ball)), {0x10112a50, 0x6c, 0x44, 0x3e2}) ||
        !checked(c, frame, spots != 0, {0x10112ab0, 0x4c, 0x3c, 0x3ea}))
        return 0;
    const auto cell_at = frame.at(0x38);
    for (std::uint32_t row = 0; row < spot_rows; ++row)
        for (std::uint32_t column = 0; column < spot_columns; ++column)
            for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
                c.put16(cell_at, row);
                c.put16(cell_at + 2, column);
                const auto cell = c.u32(cell_at), spot = c.u32(spots + (row * 44 + column * 11 + player) * 4);
                c.set({{27, player}, {28, column}, {29, column * 11}, {30, row * 44}, {31, row}, {3, formation},
                       {4, player}, {5, with_ball}, {6, cell}, {7, spot}});
                c.link(0x10112b3c);
                move_spot(c, formation, player, std::int8_t(with_ball), cell, spot);
            }
    c.put8(formation + field::saved, 0);
    c.compare(0, spot_rows, spot_rows);
    return 1;
}

std::uint32_t all_spots(Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t out) {
    Frame frame(c, 0x10112b90, 0xb0, {.first_gpr = 24});
    c.set({{24, formation}, {25, with_ball}, {26, out}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)},
           {31, c.toc(pool_slot)}});
    if (!checked(c, frame, valid_ball(std::int8_t(with_ball)), {0x10112be0, 0x6c, 0x44, 0x418}) ||
        !checked(c, frame, out != 0, {0x10112c40, 0x4c, 0x3c, 0x420}))
        return 0;
    const auto cell_at = frame.at(0x38);
    for (std::uint32_t row = 0; row < spot_rows; ++row)
        for (std::uint32_t column = 0; column < spot_columns; ++column)
            for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
                c.put16(cell_at, row);
                c.put16(cell_at + 2, column);
                c.set({{27, player}, {28, column}, {29, column * 11}, {30, row * 44}, {31, row}});
                c.link(0x10112ccc);
                spot(c, formation, std::int8_t(player), std::int8_t(with_ball), c.u32(cell_at),
                     out + (row * 44 + column * 11 + player) * 4);
            }
    c.compare(0, spot_rows, spot_rows);
    return 1;
}

std::int32_t player_at_spot(Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t cell,
                            std::uint32_t spot_value) {
    Frame frame(c, 0x10112d20, 0xe0, {.first_gpr = 25});
    const auto cell_at = frame.caller() + 0x20;
    c.put32(cell_at, cell);
    c.put32(frame.caller() + 0x24, spot_value);
    const auto x = std::uint16_t(spot_value >> 16), y = std::uint16_t(spot_value);
    c.set({{25, formation}, {26, with_ball}, {27, x}, {28, y}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)},
           {31, c.toc(pool_slot)}});
    if (!checked(c, frame, valid_ball(std::int8_t(with_ball)), {0x10112d7c, 0x94, 0x4c, 0x44c}) ||
        !checked(c, frame, valid_cell(cell), {0x10112dec, 0x74, 0x44, 0x455}) ||
        !checked(c, frame, x <= spot_x_max && y <= spot_y_max, {0x10112e54, 0x54, 0x3c, 0x45e}))
        return 0;
    const auto found = frame.at(0x38);
    for (std::int8_t player = 0; player < players; ++player) {
        c.set(29, wide(player));
        c.link(0x10112eb8);
        spot(c, formation, player, std::int8_t(with_ball), c.u32(cell_at), found);
        if (c.u16(found) == x && c.u16(found + 2) == y) {
            c.compare_logical(0, y, y);
            return player;
        }
    }
    c.compare(0, players, players);
    return -1;
}

std::uint32_t swap_spots(Cpu& c, std::uint32_t formation, std::uint32_t first, std::uint32_t second,
                         std::uint32_t with_ball, std::uint32_t cell) {
    Frame frame(c, 0x10112f00, 0x100, {.first_gpr = 25});
    const auto cell_at = frame.caller() + 0x28;
    c.put32(cell_at, cell);
    c.set({{25, formation}, {26, first}, {27, second}, {28, with_ball}, {29, c.toc(message_slot)},
           {30, c.toc(pool_slot)}, {31, c.toc(success_slot)}});
    if (!checked(c, frame, valid(std::int8_t(first)), {0x10112f58, 0xc0, 0x58, 0x484}) ||
        !checked(c, frame, valid(std::int8_t(second)), {0x10112fc4, 0xa0, 0x50, 0x48c}) ||
        !checked(c, frame, valid_ball(std::int8_t(with_ball)), {0x10113030, 0x80, 0x48, 0x494}) ||
        !checked(c, frame, valid_cell(cell), {0x101130a0, 0x60, 0x40, 0x49d}))
        return 0;
    const auto first_spot = frame.at(0x3c), second_spot = frame.at(0x38);
    c.link(0x101130f4);
    spot(c, formation, std::int8_t(first), std::int8_t(with_ball), c.u32(cell_at), first_spot);
    c.link(0x1011310c);
    spot(c, formation, std::int8_t(second), std::int8_t(with_ball), c.u32(cell_at), second_spot);
    const auto move = [&](std::uint32_t player, std::uint32_t spot_value, std::uint32_t return_address) {
        const auto cell_now = c.u32(cell_at);
        c.set({{3, formation}, {4, player}, {5, with_ball}, {6, cell_now}, {7, spot_value}});
        c.link(return_address);
        move_spot(c, formation, player, std::int8_t(with_ball), cell_now, spot_value);
    };
    move(first, c.u32(second_spot), 0x10113124);
    move(second, c.u32(first_spot), 0x1011313c);
    c.put8(formation + field::saved, 0);
    return 1;
}

std::uint32_t mirror_spots(Cpu& c, std::uint32_t formation, std::uint32_t with_ball, std::uint32_t cell) {
    Frame frame(c, 0x10113160, 0xf0, {.first_gpr = 24});
    const auto cell_at = frame.caller() + 0x20;
    c.put32(cell_at, cell);
    // The line bits, -1 ended, copied to the stack before the checks.
    const auto lines = frame.at(0x48), table = c.toc(-0x5510);
    for (std::uint32_t i = 0; i < 16; i += 4) c.put32(lines + i, c.u32(table + i));
    c.set({{24, c.toc(pool_slot)}, {25, c.toc(message_slot)}, {27, formation}, {28, with_ball},
           {31, c.toc(success_slot)}});
    if (!checked(c, frame, valid_ball(std::int8_t(with_ball)), {0x101131d4, 0x84, 0x40, 0x4c2}) ||
        !checked(c, frame, valid_cell(cell), {0x10113244, 0x64, 0x38, 0x4cb}))
        return 0;

    // Every player's spot, turned end to end (x -> 26 - x).
    const auto spots_at = frame.at(0xa4);  // uint16[11][2]
    for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
        const auto at = spots_at + player * 4;
        c.set({{24, player * 4}, {25, at}, {26, player}});
        c.link(0x101132b0);
        spot(c, formation, std::int8_t(player), std::int8_t(with_ball), c.u32(cell_at), at);
        c.put16(at, spot_x_max - c.u16(at));
    }
    c.set({{24, std::uint32_t(players) * 4}, {25, spots_at + (std::uint32_t(players) - 1) * 4},
           {26, std::uint32_t(players)}});
    const auto taken = frame.at(0x58);
    clear_bytes(c, taken, players, 0x101132dc);

    // In each line, the players nearest and farthest (first of equals) swap
    // spots, then the next pair inward, so the line keeps its left to right order.
    std::uint32_t nearest = c.r(30), farthest = c.r(29);
    auto line = lines;
    std::int16_t mask = 0;
    c.set({{24, spots_at}, {25, taken}});
    while ((mask = c.i16(line)) != -1) {
        std::int16_t low = 0x1b, high = -1;
        for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
            if (c.i8(taken + player) || !(c.i16(position_at(formation, std::int8_t(player))) & mask)) continue;
            const auto x = std::int32_t(c.u16(spots_at + player * 4));
            c.set(6, wide(x));
            if (x < low) low = std::int16_t(x), nearest = player;
            if (x > high) high = std::int16_t(x), farthest = player;
        }
        // The scan's registers: r7 its counter, r8 / r9 the extremes.
        c.set({{7, wide(players)}, {8, wide(low)}, {9, wide(high)}});
        if (low < 0x1b && high > -1 && nearest != farthest) {
            c.set({{26, line}, {29, farthest}, {30, nearest}, {31, wide(mask)}});
            swap_bytes(c, spots_at + nearest * 4, spots_at + farthest * 4, 4, 0x101133a8);
            c.put8(taken + nearest, 1);
            c.put8(taken + farthest, 1);
            continue;
        }
        line += 2;
    }

    // Into the cell of the opposite row (row -> 2 - row).
    c.put16(cell_at, 2 - c.u16(cell_at));
    c.set({{26, line}, {30, nearest}, {31, wide(mask)}});
    for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
        const auto cell_now = c.u32(cell_at), spot_now = c.u32(spots_at + player * 4);
        c.set({{24, player}, {29, spots_at + player * 4}, {3, formation}, {4, player}, {5, with_ball}, {6, cell_now},
               {7, spot_now}});
        c.link(0x101133f8);
        move_spot(c, formation, player, std::int8_t(with_ball), cell_now, spot_now);
    }
    c.put8(formation + field::saved, 0);
    c.compare(0, players, players);
    return 1;
}

std::uint32_t has_custom_spot(Cpu& c, std::uint32_t formation, std::uint32_t player) {
    Frame frame(c, 0x10113430, 0x80, {.first_gpr = 27});
    c.set({{27, formation}, {28, player}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!valid(std::int8_t(player))) {
        report_r4(c, frame, 0x10113478, 0x44, 0x3c, 0x531);
        return 0;
    }
    const auto cell_at = frame.at(0x38);
    for (std::uint32_t ball = 0; ball < 2; ++ball)
        for (std::uint32_t row = 0; row < spot_rows; ++row)
            for (std::uint32_t column = 0; column < spot_columns; ++column) {
                c.put16(cell_at, row);
                c.put16(cell_at + 2, column);
                c.set({{29, column}, {30, ball}, {31, row}, {3, formation}, {4, player}, {5, ball},
                       {6, c.u32(cell_at)}});
                c.link(0x101134f4);
                const auto custom = std::int8_t(custom_spot(c, formation, std::int8_t(player), ball, c.u32(cell_at)));
                c.compare(0, custom, 0);
                if (custom) return 1;
            }
    c.compare(0, 2, 2);
    return 0;
}

std::uint32_t custom_spot(Cpu& c, std::uint32_t formation, std::int8_t player, std::uint32_t with_ball,
                          std::uint32_t cell) {
    Frame frame(c, 0x10113550, 0xe0, {.first_gpr = 25});
    const auto cell_at = frame.caller() + 0x24;
    c.put32(cell_at, cell);
    c.set({{25, c.toc(success_slot)}, {26, c.toc(message_slot)}, {27, formation}, {28, with_ball},
           {29, c.toc(pool_slot)}});
    if (player >= 0) c.set(31, wide(player));
    if (!checked(c, frame, valid(player), {0x101135a0, 0x9c, 0x54, 0x560}) ||
        !checked(c, frame, valid_ball(std::int8_t(with_ball)), {0x1011360c, 0x7c, 0x4c, 0x568}))
        return 0;
    // Quirk: the original accepts zone-grid cells (rows to 8, columns to 11)
    // though spots exist for 3 by 4 cells.
    if (row_of(cell) <= 8) c.set(30, column_of(cell));
    if (!checked(c, frame, row_of(cell) <= 8 && column_of(cell) <= 11, {0x1011367c, 0x5c, 0x44, 0x571}))
        return 0;
    c.set(29, position_at(formation, player) - field::position);

    // As set_position derives them: the spot without the ball from the lower
    // line's position, the spot with the ball from the other.
    const auto here = c.i16(position_at(formation, player)), ball = c.i16(position_at(formation, player, true));
    auto first = here, second = here;
    std::array<std::uint32_t, 4> returns{0x101136e8, 0x1011371c, 0x10113740, 0x10113778};
    if (here != ball) {
        c.set({{3, wide(here)}, {4, frame.at(0x3c)}, {5, frame.at(0x3a)}});
        c.link(0x10113794);
        split_position(c, here, frame.at(0x3c), frame.at(0x3a));
        c.set({{3, wide(ball)}, {4, frame.at(0x38)}, {5, frame.at(0x3a)}});
        c.link(0x101137a4);
        split_position(c, ball, frame.at(0x38), frame.at(0x3a));
        if (c.i16(frame.at(0x38)) < c.i16(frame.at(0x3c))) {
            first = ball, second = here;
            returns = {0x1011387c, 0x101138b0, 0x101138d4, 0x1011390c};
        } else {
            second = ball;
            returns = {0x101137c8, 0x101137fc, 0x10113820, 0x10113858};
        }
    }
    const auto spot_at = frame.at(0x40);
    // cr0 as the original's last test leaves it: the comparison's result
    // against 0, or the ball state against 1 when it skips the second.
    const auto differs = [&](std::int16_t code, std::uint32_t state, std::uint32_t spot_return,
                             std::uint32_t compare_return) {
        c.set({{3, wide(code)}, {4, state}, {5, c.u32(cell_at)}, {6, spot_at}});
        c.link(spot_return);
        default_spot(c, code, state, c.u32(cell_at), spot_at);
        if (std::int8_t(with_ball) != std::int8_t(state)) {
            if (state == 1) c.compare(0, std::int8_t(with_ball), 1);
            return false;
        }
        const auto own = formation + field::spots + wide(player) * field::players_spots + state * field::ball_spots +
                         c.u16(cell_at) * 16u + column_of(cell) * 4u;
        const auto result = compare_bytes(c, own, spot_at, 4, compare_return);
        c.compare(0, result, 0);
        return result != 0;
    };
    if (differs(first, 0, returns[0], returns[1]) || differs(second, 1, returns[2], returns[3])) return 1;
    return 0;
}

std::uint32_t shared_spot(Cpu& c, std::uint32_t formation, std::int8_t player, std::uint32_t with_ball,
                          std::uint32_t cell, std::uint32_t later) {
    Frame frame(c, 0x10113940, 0xe0, {.first_gpr = 23});
    c.put32(frame.caller() + 0x24, cell);
    const auto row = row_of(cell), column = column_of(cell);
    c.set({{23, c.toc(message_slot)}, {24, c.toc(pool_slot)}, {25, formation}, {26, with_ball}, {27, later},
           {28, row}, {29, column}, {30, c.toc(success_slot)}});
    if (player >= 0) c.set(31, wide(player));
    // Quirk: zone-grid cells (rows to 8, columns to 11) pass, as in custom_spot.
    if (!checked(c, frame, valid(player), {0x1011399c, 0x90, 0x48, 0x5c7}) ||
        !checked(c, frame, valid_ball(std::int8_t(with_ball)), {0x10113a08, 0x70, 0x40, 0x5cf}) ||
        !checked(c, frame, row <= 8 && column <= 11, {0x10113a70, 0x50, 0x38, 0x5d8}))
        return 0;
    c.put8(later, 0);
    const auto offset = [&](std::int8_t slot) { return spot_offset(slot, std::int8_t(with_ball), row, column); };
    for (std::int8_t other = 0; other < players; ++other) {
        if (other == player) continue;
        c.set({{23, wide(other) * field::players_spots}, {30, wide(other)}, {6, (std::uint32_t(row) << 4) & 0xffff0},
               {7, (std::uint32_t(column) << 2) & 0x3fffc}});
        if (compare_bytes(c, formation + offset(other), formation + offset(player), 4, 0x10113b0c) == 0) {
            c.compare(0, other, player);
            if (other < player) return 0;
            c.put8(later, 1);
            return 1;
        }
    }
    c.compare(0, players, players);
    return 1;
}

namespace {
// Writes bytes to guest memory, a word at a time where it can.
template <std::size_t N> void put_bytes(Cpu& c, std::uint32_t at, const std::array<std::uint8_t, N>& bytes) {
    std::size_t i = 0;
    for (; i + 4 <= N; i += 4)
        c.put32(at + std::uint32_t(i), std::uint32_t(bytes[i]) << 24 | std::uint32_t(bytes[i + 1]) << 16 |
                                           std::uint32_t(bytes[i + 2]) << 8 | bytes[i + 3]);
    for (; i < N; ++i) c.put8(at + std::uint32_t(i), bytes[i]);
}
}

std::uint32_t update_zone_grid(Cpu& c, std::uint32_t formation) {
    // Frameless: stmw r13 and the two loop counters below the stack pointer
    // (the counters at their final values).
    const auto sp = c.sp();
    for (unsigned n = 13; n < 32; ++n) c.put32(sp - 0x4c + (n - 13) * 4, c.r(n));
    c.put32(sp - 0x54, std::uint32_t(players) * field::players_spots);
    c.put32(sp - 0x58, std::uint32_t(players));
    constexpr std::uint32_t grid_rows = 9, grid_columns = 12, grid_size = grid_rows * grid_columns;
    for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
        for (std::uint32_t ball = 0; ball < 2; ++ball) {
            // Each cell covers 3 by 3 grid bytes; the mirror holds (8 - x, 11 - y)
            // at the opposite place. Spots are unsigned, so x * 9 / 27 is x / 3.
            std::array<std::uint8_t, grid_size> grid{}, mirrored{};
            const auto spots = formation + field::spots + player * field::players_spots + ball * field::ball_spots;
            for (std::uint32_t row = 0; row < spot_rows; ++row)
                for (std::uint32_t column = 0; column < spot_columns; ++column) {
                    const auto x = std::uint32_t(c.u16(spots + row * 16 + column * 4) / 3);
                    const auto y = std::uint32_t(c.u16(spots + row * 16 + column * 4 + 2) / 3);
                    const auto zone = std::uint8_t((y & 0xf) << 4 | (x & 0xf));
                    const auto mirror = std::uint8_t(((11 - y) & 0xf) << 4 | ((8 - x) & 0xf));
                    for (std::uint32_t r = row * 3; r < row * 3 + 3; ++r)
                        for (std::uint32_t k = column * 3; k < column * 3 + 3; ++k) {
                            grid[r * grid_columns + k] = zone;
                            mirrored[(grid_rows - 1 - r) * grid_columns + (grid_columns - 1 - k)] = mirror;
                        }
                }
            const auto offset = player * 0xd8 + ball * grid_size;
            put_bytes(c, formation + field::zone_grid + offset, grid);
            put_bytes(c, formation + field::mirrored_grid + offset, mirrored);
        }
    }
    // Registers as the original's loops leave them: the last cell's offsets
    // and counters, the divisions' constants, cr0 from the player count test.
    c.set({{0, 0x38e38e39}, {4, std::uint32_t(players) * field::players_spots}, {5, 0x410}, {6, 0x3f0},
           {7, 0x4bda12f7}, {8, 6}, {9, 0x6c}, {10, 0x30}, {11, 0x10}, {12, 0x1b}});
    c.compare(0, players, players);
    set_ctr(c, 0);
    set_carry(c, false);
    return 1;
}

std::int16_t between_positions(Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t a_line, std::uint32_t b_line) {
    // Frameless: stmw r27, then the line bits (-1 ended) and the side bits
    // copied below the stack pointer.
    const auto sp = c.sp();
    for (unsigned n = 27; n < 32; ++n) c.put32(sp - 0x14 + (n - 27) * 4, c.r(n));
    const auto lines = sp - 0x2c, sides = sp - 0x38;
    const auto line_table = c.toc(-0x5520), side_table = c.toc(-0x5524);
    for (std::uint32_t i = 0; i < 16; i += 4) c.put32(lines + i, c.u32(line_table + i));
    for (std::uint32_t i = 0; i < 12; i += 4) c.put32(sides + i, c.u32(side_table + i));
    // The index of each code's (last) bit in each table; a code without one
    // keeps the default: the caller's r0 / r5 for lines, the code for sides.
    const auto code_a = std::int16_t(a), code_b = std::int16_t(b);
    auto a_side = a, b_side = b;
    for (std::uint32_t i = 0; c.i16(lines + i * 2) != -1; ++i) {
        if (code_a & c.i16(lines + i * 2)) a_line = i;
        if (code_b & c.i16(lines + i * 2)) b_line = i;
    }
    for (std::uint32_t i = 0; c.i16(sides + i * 2) != -1; ++i) {
        if (code_a & c.i16(sides + i * 2)) a_side = i;
        if (code_b & c.i16(sides + i * 2)) b_side = i;
    }
    const auto line_sum = std::int32_t(std::int8_t(a_line)) + std::int8_t(b_line);
    const auto side_sum = std::int32_t(std::int8_t(a_side)) + std::int8_t(b_side);
    const auto line = line_sum / 2, side = side_sum / 2;
    // Registers as the original leaves them; cr0 from the side loop's end.
    c.set({{4, lines}, {5, wide(line)}, {6, wide(line_sum)}, {7, sides}, {8, wide(code_b & 0x800)},
           {9, wide(code_b)}, {10, wide(code_a)}, {11, 0xffffffff}, {12, 5}});
    c.compare(0, -1, -1);
    const auto result = wide(c.i16(lines + wide(line) * 2)) | wide(c.i16(sides + wide(side) * 2));
    c.set(0, result);
    const auto halved = side_sum + (side_sum < 0);  // srawi's carry: a negative odd value
    set_carry(c, halved < 0 && (halved & 1));
    return std::int16_t(result);
}

std::uint32_t natural_positions(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10117490, 0x70, {.first_gpr = 30});
    c.set({{30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!player) {
        report_r4(c, frame, 0x101174d0, 0x40, 0x38, 0xe79);
        return 0;
    }
    const auto rating = [&](std::uint32_t offset) { return c.i8(player + offset); };
    // The lines rated at least 15, else 14, ... down to 10, the first level
    // with any; of lines 0x8, 0x20 and 0x10 only the first in that order.
    std::uint32_t bits = 0;
    for (std::int8_t level = 15; level >= 10 && !bits; --level) {
        if (rating(0xf) >= level) bits |= 0x1;
        if (rating(0x11) >= level) bits |= 0x4;
        if (rating(0x10) >= level) bits |= 0x2;
        if (rating(0x12) >= level) bits |= 0x8;
        else if (rating(0x14) >= level) bits |= 0x20;
        else if (rating(0x13) >= level) bits |= 0x10;
        if (rating(0x15) >= level) bits |= 0x40;
    }
    // Quirk: the sides use 15 at every level of their loop, so only sides
    // rated 15 or more count.
    const bool sides = rating(0x17) >= 15 || rating(0x18) >= 15 || rating(0x19) >= 15;
    if (rating(0x17) >= 15) bits |= 0x800;
    if (rating(0x18) >= 15) bits |= 0x80;
    if (rating(0x19) >= 15) bits |= 0x200;
    // Registers as the side loop leaves them: it stops after one round when a
    // side counted (r4 its flag, cr0 greater), else after six (r4 = 9, less).
    c.set({{4, sides ? 1u : 9u}, {5, sides ? 1u : 0u}, {6, sides ? 14u : 9u}, {7, wide(rating(0x15))}});
    c.compare(0, sides ? 1 : 9, sides ? 0 : 10);
    return bits;
}

std::uint32_t default_spot(Cpu& c, std::int16_t position, std::uint32_t with_ball, std::uint32_t cell,
                           std::uint32_t out) {
    Frame frame(c, 0x10117690, 0x120, {.first_gpr = 27});
    c.put32(frame.caller() + 0x20, cell);  // the cell argument, spilled into the caller's frame
    const auto row = row_of(cell), column = column_of(cell);
    const auto ball = std::int8_t(with_ball);
    c.set({{27, c.toc(message_slot)}, {28, c.toc(pool_slot)}, {29, with_ball}, {30, out}, {31, column}});
    if (!valid_ball(ball) || !valid_cell(cell)) {
        report_r4(c, frame, 0x101176f4, 0xe4, 0xdc, 0xede);
        return 0;
    }
    c.set({{4, frame.at(0x3a)}, {5, frame.at(0x38)}});
    c.link(0x10117748);
    split_position(c, position, frame.at(0x3a), frame.at(0x38));
    // The parts are read back even when splitting failed.
    const auto line = std::uint16_t(c.u16(frame.at(0x3a))), side = std::uint16_t(c.u16(frame.at(0x38)));

    // y: by line and column; tables of 4 signed bytes the original copies to
    // its stack (TOC -0x3990 + 8 * line, without then with the ball). With the
    // ball a wide player (side 0x80 or 0x800) outside goal moves one further,
    // a defender in the last column four.
    unsigned line_index = 6;
    for (unsigned i = 0; i < 6; ++i)
        if (line & (1u << i)) { line_index = i; break; }
    const auto depth_slot = std::int32_t(0x70 - 8 * line_index - (ball ? 4 : 0));
    const auto depths = c.toc(-0x3990 + std::int32_t(8 * line_index) + (ball ? 4 : 0));
    c.put32(frame.at(depth_slot), depths);
    std::uint16_t y = std::uint16_t(std::int8_t(depths >> (24 - 8 * column)));
    if (ball && line_index > 0 && (side & 0x880)) y += (line_index <= 2 && column == 3) ? 4 : 1;

    // x: by side, from tables of 5 signed bytes (right .. left) chosen by
    // line, ball and column, also copied to the stack.
    const auto side_index = side & 0x80 ? 0u : side & 0x100 ? 1u : side & 0x200 ? 2u : side & 0x400 ? 3u : 4u;
    std::int32_t widths_slot = 0, widths_toc = 0;
    if (line & 0x1)
        widths_slot = ball ? 0xcc : 0xd4, widths_toc = ball ? -0x552c : -0x5528;
    else if (line & 0x6)
        widths_slot = 0xc4 - (column ? 8 : 0) - (ball ? 0x10 : 0), widths_toc = -0x5530 - (column ? 4 : 0) - (ball ? 8 : 0);
    else if (line & 0x38)
        widths_slot = ball ? 0x94 : column ? 0x9c : 0xa4, widths_toc = ball ? -0x5548 : column ? -0x5544 : -0x5540;
    else if (!ball)
        widths_slot = 0x8c, widths_toc = -0x554c;
    else
        widths_slot = column <= 1 ? 0x84 : column == 2 ? 0x7c : 0x74,
        widths_toc = column <= 1 ? -0x5550 : column == 2 ? -0x5554 : -0x5558;
    const auto widths = c.toc(widths_toc);
    c.put32(frame.at(widths_slot), c.u32(widths));
    c.put8(frame.at(widths_slot + 4), c.u8(widths + 4));
    std::uint16_t x = std::uint16_t(c.i8(widths + side_index));

    // Towards the cell's row: one step (a goalkeeper away from the first
    // column, a defender in it, a forward off the wing in the last column
    // with the ball) or five (everyone else but goalkeepers), clamped to the
    // row's band.
    const bool one_step = ((line & 0x1) && column != 0) || ((line & 0x6) && column == 0) ||
                          ((line & 0x40) && (side & 0x700) && column == 3 && ball == 1);
    if (one_step || !(line & 0x1)) {
        const std::int32_t step = one_step ? 1 : 5;
        if (row == 0) x = std::uint16_t(std::clamp(std::int32_t(x) - step, 2, 17));
        else if (row == 2) x = std::uint16_t(std::clamp(std::int32_t(x) + step, 9, 24));
    }
    c.put16(out, std::min<std::uint16_t>(x, spot_x_max));
    c.put16(out + 2, std::min<std::uint16_t>(y, spot_y_max));
    // Registers as the original's clamps leave them.
    c.set({{0, std::min<std::uint16_t>(y, spot_y_max)}, {4, y}, {5, c.u32(widths)}});
    c.compare(0, y, spot_y_max);
    set_carry(c, y != 0);  // srawi of (-y | y)
    return 1;
}
}
