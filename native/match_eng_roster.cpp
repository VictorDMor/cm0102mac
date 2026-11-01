// MATCH_ENG.CPP, roster: players entering and leaving cells, the goalkeepers and tie totals.
#include "match_eng_roster.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
constexpr std::uint32_t player_side = 0x27, player_slot = 0x28, appearances = 0x23, minutes = 0x21;
constexpr std::uint32_t cells_at = 0xa08, cell_row_size = 0x450, cell_size = 0x5c, cell_list_size = 0x2c, cell_count_at = 0x58;
constexpr std::uint32_t goalkeeper_first = 0xe01c, goalkeeper_second = 0xe020;
constexpr std::uint32_t substitution_clock = 0x7944;
constexpr std::uint32_t role_bits_routine = 0x10289560;  // another module: the player's role bit set (a halfword)

std::uint32_t wrap(std::int32_t value) { return std::uint32_t(value); }

// The role bits of a player, asked by the `bl` at `at`.
std::int32_t role_bits(Cpu& c, std::uint32_t player, std::uint32_t at) {
    c.set(3, player);
    c.call(role_bits_routine, at + 4);
    return std::int16_t(c.r(3));
}
}  // namespace

// ---------------------------------------------------------------------------
// 0x101f1530
void player_arrives(Cpu& c, std::uint32_t match, std::uint32_t player, std::uint32_t carried) {
    Frame frame(c, 0x101f1530, 0x60, {.first_gpr = 23});
    keep(c, {{30, match}, {31, player}});
    c.put8(frame.at(0x83), carried);  // the caller's parameter slot
    Kept<29> keeper(c, 0);
    if (cmp_eq(c, s8(carried))) {
        c.put8(player + appearances, c.u8(player + appearances) + 1u);
        // clock / 11 (mulhw by 0x2e8ba2e9), kept as a byte.
        const auto clock = std::int64_t(c.i16(match + clock_at));
        auto quotient = std::int32_t((std::int64_t(0x2e8ba2e9) * clock) >> 32) >> 1;
        quotient += quotient < 0;
        c.put8(player + minutes, wrap(quotient));
        // The minutes cannot pass the end of the period the clock has reached.
        const auto cap = [&]<unsigned R>(std::int32_t limit) {
            const auto current = s8(c.u8(player + minutes));
            keep(c, {{R, wrap(cmp_lt(c, current, limit) ? current : limit)}});
            c.put8(player + minutes, c.r(R));
        };
        const auto period_end = std::int32_t(std::int16_t(minute_clock(c, match)));
        if (cmp_eq(c, period_end, 0x483)) cap.operator()<26>(0x69);
        else if (cmp_ge(c, period_end, 0x483)) { if (cmp_eq(c, period_end, 0x528)) cap.operator()<25>(0x78); }
        else if (cmp_eq(c, period_end, 0x3de)) cap.operator()<27>(0x5a);
        else if (cmp_lt(c, period_end, 0x3de) && cmp_eq(c, period_end, 0x1ef)) cap.operator()<28>(0x2d);
        enter(c, 0x101f1688, {match, player, wrap(-3)});
        leave_cell(c, match, player, wrap(-3));
    }
    if (cmp_eq(c, role_bits(c, player, 0x101f168c), 1)) keeper = 1;
    leave(c, {{3, player}, {4, wrap(-3)}});
    set_position(c, player, wrap(-3));
    if (cmp_eq(c, s8(c.u8(match)))) {
        c.set({{3, match}, {4, c.u8(player + player_side)}});
        call(c, 0x101da5a0, 0x101f16c8);
        if (cmp_ne(c, s8(c.r(3)))) return;
    }
    if (cmp_ne(c, s8(c.u8(match + 0x1bc)))) return;
    const auto side = s8(c.u8(player + player_side));
    const auto stats = match + wrap(side * std::int32_t(side_stats_size)) + side_stats_at;
    const auto record_clock = [&] {
        const auto clock = c.i16(match + clock_at);
        c.set({{3, wrap(side * 2)}, {4, extended(clock)}});
        c.put16(match + substitution_clock + wrap(side * 2), wrap(clock));
    };
    const auto stats_position = position(c, stats);
    if (cmp_ne(c, s8(stats_position)) && cmp_ne(c, s8(keeper))) {
        c.set({{3, stats}, {4, 1}});
        c.call(0x1020ef70, 0x101f1730);
        if (cmp_eq(c, s8(c.u8(match)))) record_clock();
    } else if (cmp_eq(c, s8(c.u8(match)))) {
        c.set(3, stats);
        c.call(0x102104d0, 0x101f1784);
        c.set({{3, stats}, {4, 0}});
        c.call(0x1020ef70, 0x101f17a0);
        c.set({{3, match}, {4, c.u8(player + player_side)}});
        call(c, 0x101d3b60, 0x101f17ac);
        if (cmpl_eq(c, player, c.r(3))) {
            c.set({{3, match}, {4, c.u8(player + player_side)}});
            call(c, 0x101d3bf0, 0x101f17c0);
            keep(c, {{23, c.r(3)}});
            c.set(3, stats);
            c.call(0x10210430, 0x101f17dc);
            keep(c, {{24, c.r(3)}});
            c.set({{3, match}, {4, c.u8(player + player_side)}});
            call(c, 0x101d3c50, 0x101f17ec);
            c.set({{4, c.r(3)}, {3, c.u32(c.toc(-0x7a6c))}, {5, c.r(24)}, {6, c.r(23)}});
            c.call(0x103b0bf0, 0x101f1804);
        }
        leave(c, {{3, match}, {4, c.u8(player + player_side)}});
        clear_goalkeeper(c, match, c.r(4));
        enter(c, 0x101f181c, {match, c.u8(player + player_side)});
        side_goalkeeper(c, match, c.r(4));
        record_clock();
    }
    c.set({{3, match}, {4, c.u8(player + player_side)}});
    call(c, 0x101d6a90, 0x101f1840);
    c.set(3, match);
    call(c, 0x101d6f90, 0x101f1848);
}

// ---------------------------------------------------------------------------
// 0x101f1880
void put_off_pitch(Cpu& c, std::uint32_t match, std::uint32_t player) {
    Frame frame(c, 0x101f1880, 0x40, {.first_gpr = 30});
    c.put32(frame.at(0x58), match);
    keep(c, {{31, player}});
    const auto cell = c.u32(frame.at(0x58)) + 0x2eb0;  // row 8, column 6
    keep(c, {{30, cell}});
    leave(c, {{3, player}, {4, 8}});
    set_row(c, player, 8);
    leave(c, {{3, player}, {4, 6}});
    set_column(c, player, 6);
    const auto side = s8(c.u8(player + player_side));
    const auto lists = wrap(side * std::int32_t(cell_list_size));
    const auto count_at = cell + wrap(side + std::int32_t(cell_count_at));
    c.put32(cell + lists + wrap(s8(c.u8(count_at))) * 4, player);
    c.put8(player + player_slot, c.u8(count_at));
    const auto count = c.u8(count_at);
    c.put8(count_at, count + 1u);
    leave(c, {{3, count}, {4, wrap(side + std::int32_t(cell_count_at))}});
}

// ---------------------------------------------------------------------------
// 0x101f1950
void leave_cell(Cpu& c, std::uint32_t match, std::uint32_t player, std::uint32_t mode) {
    Frame frame(c, 0x101f1950, 0xa0, {.first_gpr = 18});
    keep(c, {{28, match}, {31, player}, {19, mode}, {23, c.toc(-0x52cc)}, {27, 0}, {21, wrap(-10000)}});
    const auto column_offset = wrap(s8(column(c, player)) * std::int32_t(cell_size));
    keep(c, {{18, column_offset}});
    const auto row_offset = wrap(s8(row(c, player)) * std::int32_t(cell_row_size));
    const auto cell = match + (row_offset + column_offset) + cells_at;
    keep(c, {{30, cell}});
    const auto old_position = position(c, player);
    keep(c, {{20, old_position}});
    if (cmp_lt(c, s8(old_position))) return;

    // Out of the cell's list: the last entry takes his slot.
    const auto side = s8(c.u8(player + player_side));
    const auto lists = wrap(side * std::int32_t(cell_list_size));
    const auto count_at = cell + wrap(side + std::int32_t(cell_count_at));
    const auto last = c.u32(cell + lists + wrap(s8(c.u8(count_at)) - 1) * 4);
    const auto slot = c.u8(player + player_slot);
    c.put32(cell + lists + wrap(s8(slot)) * 4, last);
    c.put8(count_at, c.u8(count_at) - 1u);
    const auto moved = c.u32(cell + lists + wrap(s8(c.u8(player + player_slot))) * 4);
    c.set(4, wrap(side + std::int32_t(cell_count_at)));
    if (cmpl_ne(c, moved)) {
        c.put8(moved + player_slot, slot);
        c.set(4, lists);
    }

    // He no longer marks, nor is he marked.
    auto target = marking_target(c, player);
    if (cmpl_ne(c, target)) {
        marking_target(c, player);
        c.set(4, player);
        c.call(0x10283540, 0x101f1a94);
        if (cmp_ne(c, s8(c.r(3)))) {
            marking_target(c, player);
            c.set(4, player);
            c.call(0x10283590, 0x101f1ab0);
        }
        leave(c, {{3, player}, {4, 0}});
        set_marking_target(c, player, 0);
    }
    if (cmp_ne(c, s8(marking_count(c, player)))) {
        keep(c, {{25, 0}});
        for (;;) {
            const auto count = s8(marking_count(c, player));
            if (!cmp_lt(c, s8(c.r(25)), count)) break;
            leave(c, {{3, player}, {4, c.r(25)}});
            const auto marked = marking_member(c, player, c.r(25));
            if (cmpl_eq(c, marking_target(c, marked), player)) {
                const auto member = marking_member(c, player, c.r(25));
                leave(c, {{3, member}, {4, 0}});
                set_marking_target(c, member, 0);
            }
            keep(c, {{25, c.r(25) + 1}});
        }
        c.set(3, player);
        clear_marking_list(c, player);
    }

    if (cmp_eq(c, s8(mode), -1)) return;
    if (!cmp_eq(c, s8(mode), -3)) {
        c.set(3, match + 0x10000);
        if (cmpl_ne(c, player, c.u32(match + goalkeeper_second)) && cmpl_ne(c, player, c.u32(match + goalkeeper_first))) return;
    }
    if (cmp_ne(c, role_bits(c, player, 0x101f1b64) & 0x40)) return;
    if (cmp_ne(c, role_bits(c, player, 0x101f1b7c) & 0x20)) return;
    c.set({{3, match}, {4, c.u8(player + player_side)}});
    call(c, 0x101da5a0, 0x101f1b9c);
    if (cmp_ne(c, s8(c.r(3)))) return;

    // The best other player of the side inherits the position. The first candidate
    // skips the role filters (they only apply once one is held).
    keep(c, {{22, c.u8(player + player_side)}, {24, 0}});
    for (; cmp_lt(c, s8(c.r(24)), 0x14); keep(c, {{24, c.r(24) + 1}})) {
        const auto candidate = match + wrap(s8(c.r(22)) * std::int32_t(side_size)) + wrap(s8(c.r(24)) * std::int32_t(player_size)) + players_at;
        keep(c, {{29, candidate}});
        if (cmp_lt(c, s8(position(c, candidate)))) continue;
        if (cmpl_eq(c, player, candidate)) continue;
        if (cmpl_ne(c, c.r(27))) {
            if (cmp_ne(c, role_bits(c, candidate, 0x101f1bfc) & 1)) continue;
            if (cmp_ne(c, role_bits(c, candidate, 0x101f1c14) & 4)) continue;
            if (cmp_ne(c, role_bits(c, candidate, 0x101f1c2c) & 2)) continue;
        }
        std::int32_t score;
        if (cmp_ne(c, role_bits(c, candidate, 0x101f1c44) & 0x40)) score = 70000;
        else if (cmp_ne(c, role_bits(c, candidate, 0x101f1c68) & 0x20)) score = 50000;
        else if (cmp_ne(c, role_bits(c, candidate, 0x101f1c8c) & 0x10)) score = 15000;
        else score = -5000;
        keep(c, {{26, wrap(score)}});
        if (cmp_ne(c, role_bits(c, candidate, 0x101f1cb0) & 0x80) || cmp_ne(c, role_bits(c, candidate, 0x101f1cc8) & 0x800)) {
            score += 5000;
            keep(c, {{26, wrap(score)}});
        }
        if (cmp_gt(c, score, std::int32_t(c.r(21))) || cmpl_eq(c, c.r(27))) keep(c, {{21, c.r(26)}, {27, candidate}});
    }
    if (cmpl_eq(c, c.r(27))) {
        // Nobody to take the place: the game's assertion, then an exit(-1).
        const auto messages = c.r(23);
        report_assertion(c, {.at = 0x101f1d24, .file = messages + 0x45, .format = messages + 0x53, .title = messages + 0x60,
                             .name = frame.at(0x3d), .extension = frame.at(0x38), .line = 0x3abd});
        c.set(3, wrap(-1));
        c.call(0x1076f7d0, 0x101f1d6c);
    }
    leave(c, {{3, c.r(27)}, {4, c.r(20)}});
    set_position(c, c.r(27), c.r(20));
    if (cmp_eq(c, role_bits(c, player, 0x101f1d7c), 1)) {
        c.set(3, match + 0x10000);
        c.put32(match + (cmp_eq(c, s8(c.r(22))) ? goalkeeper_first : goalkeeper_second), c.r(27));
    }
}

// ---------------------------------------------------------------------------
// 0x101f1e10
std::uint32_t side_goalkeeper(Cpu& c, std::uint32_t match, std::uint32_t second_side) {
    Frame frame(c, 0x101f1e10, 0x60, {.first_gpr = 23});
    keep(c, {{31, match}});
    c.put8(frame.at(0x7f), second_side);  // the caller's parameter slot
    // One side: forget a goalkeeper who is gone, else take the first player who counts as
    // one; a replacement is pulled from the squad (the first player on the pitch) and
    // inherits the position of the one he replaces. `Left` and `Right` are the registers
    // the original leaves its previous player in during the first and the second search.
    const auto settle = [&]<unsigned First, unsigned Second>(std::uint32_t slot, std::uint32_t players, std::uint32_t shift) {
        if (cmpl_ne(c, c.u32(slot))) {
            if (cmp_lt(c, s8(position(c, c.u32(slot))))) c.put32(slot, 0);
        }
        if (cmpl_ne(c, c.u32(slot))) return;
        Kept<30> current(c, players);
        keep(c, {{28, 0}, {29, 0}});
        for (; cmp_lt(c, s8(c.r(29)), 0x14); keep(c, {{29, c.r(29) + 1}, {First, c.r(30)}}), current = current + player_size) {
            const auto player = std::uint32_t(current);
            if (cmpl_eq(c, c.u32(slot)) && cmp_ge(c, s8(position(c, player)))) {
                c.set({{3, match}, {4, player}});
                call(c, 0x101e3d50, 0x101f1eac + shift);
                if (cmp_eq(c, std::int16_t(c.r(3)), 1)) c.put32(slot, player);
            }
            c.set({{3, match}, {4, player}});
            call(c, 0x101e3e70, 0x101f1ecc + shift);
            if (cmp_eq(c, std::int16_t(c.r(3)), 1) && cmp_eq(c, s8(position(c, player)), -2) && cmp_eq(c, s8(c.u8(player + 0x20)), -1))
                keep(c, {{28, player}});
        }
        if (cmpl_ne(c, c.u32(slot))) return;
        current = players;
        keep(c, {{27, 0}, {29, 0}});
        for (; cmp_lt(c, s8(c.r(29)), 0x14); keep(c, {{29, c.r(29) + 1}, {Second, c.r(30)}}), current = current + player_size) {
            const auto player = std::uint32_t(current);
            if (cmp_lt(c, s8(position(c, player)))) continue;
            if (cmpl_ne(c, c.r(28))) {
                keep(c, {{27, previous_position(c, c.r(28))}});
                const auto position_now = position(c, player);
                leave(c, {{4, position_now}, {3, c.r(28)}});
                set_previous_position(c, c.r(28), position_now);
            }
            c.put32(slot, player);
            const auto table = c.u32(match + 0x7934 + wrap(s8(c.u8(c.u32(slot) + player_side))) * 4);
            if (cmp_eq(c, c.i16(table + wrap(s8(c.r(27))) * 2), 1)) {
                leave(c, {{3, c.u32(slot)}, {4, c.r(27)}});
                set_position(c, c.u32(slot), c.r(27));
            }
            return;
        }
    };
    settle.operator()<26, 25>(match + goalkeeper_first, match + players_at, 0);
    settle.operator()<24, 23>(match + goalkeeper_second, match + players_at + side_size, 0x1b0);
    const bool second = cmp_eq(c, s8(c.u8(frame.at(0x7f))), 1);
    return result(c, c.u32(match + (second ? goalkeeper_second : goalkeeper_first)));
}

// ---------------------------------------------------------------------------
// 0x101f21d0
std::uint32_t tie_totals(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t home_at, std::uint32_t away_at,
                         std::uint32_t decider_at, std::uint32_t two_legs_at, std::uint32_t knockout_at,
                         std::uint32_t leg_flag) {
    Frame frame(c, 0x101f21d0, 0x70, {.first_gpr = 20});
    keep(c, {{31, match}, {20, side}, {26, home_at}, {27, away_at}, {28, decider_at}, {22, two_legs_at}, {23, knockout_at}, {21, leg_flag}});
    for (const auto at : {home_at, away_at, decider_at, two_legs_at, knockout_at}) c.put8(at, 0);
    keep(c, {{25, 0}, {24, 0}});
    std::uint32_t own, other;
    if (cmp_eq(c, s8(side))) {
        own = home_at; other = away_at;
        keep(c, {{30, own}, {29, other}});
        if (cmp_ne(c, s8(leg_flag))) keep(c, {{25, 1}});
    } else {
        own = away_at; other = home_at;
        keep(c, {{30, own}, {29, other}});
        if (cmp_ne(c, s8(leg_flag))) keep(c, {{24, 1}});
    }
    c.put8(own, c.u8(match + 0xe03d));
    c.put8(other, c.u8(match + 0xe073));
    auto fixture = c.u32(match + fixture_at);
    c.set(3, extended(c.i16(fixture + 0x30)));
    c.call(0x10464880, 0x101f229c);
    if (cmp_eq(c, s8(c.r(3)), 3)) c.put8(knockout_at, 1);
    fixture = c.u32(match + fixture_at);
    c.set(3, fixture);
    if (cmp_ne(c, s8(c.u8(fixture + 0x4b)), -1)) {
        c.set({{4, c.u8(own)}, {3, fixture}});
        c.put8(own, c.u8(own) + c.u8(fixture + 0x4b));
        c.set({{4, c.u8(other)}, {3, fixture}});
        c.put8(other, c.u8(other) + c.u8(fixture + 0x4c));
        c.put8(two_legs_at, 1);
        const auto own_total = s8(c.u8(own)) + s8(c.r(25));
        const auto other_total = s8(c.u8(other)) + s8(c.r(24));
        c.set({{3, wrap(s8(c.u8(other)))}, {4, wrap(own_total)}});
        if (cmp_eq(c, own_total, other_total)) {
            c.set(3, fixture);
            const auto flags = c.i16(fixture + 0x38);
            if (cmp_eq(c, flags & 4)) {
                c.set(3, fixture);
                bool skip = false;
                if (cmp_ne(c, c.i16(fixture + 0x38) & 0x4000)) skip = cmp_le(c, c.i16(match + period_end), 0x3de);
                if (!skip) {
                    if (cmp_eq(c, s8(side))) {
                        const auto total = std::int32_t(c.u8(match + 0xe073)) + s8(c.r(25));
                        c.set({{3, c.u8(match + 0xe073)}, {4, wrap(total)}});
                        c.set(3, c.u32(match + fixture_at));
                        if (cmp_lt(c, total, s8(c.u8(fixture + 0x4b)))) c.put8(decider_at, 1);
                    } else {
                        const auto total = std::int32_t(c.u8(match + 0xe073)) + s8(c.r(24));
                        c.set({{3, c.u8(match + 0xe073)}, {4, wrap(total)}});
                        c.set(3, c.u32(match + fixture_at));
                        if (cmp_gt(c, total, s8(c.u8(fixture + 0x4b)))) c.put8(decider_at, 1);
                    }
                }
            }
        }
    }
    if (cmp_ne(c, s8(c.u8(decider_at)))) return result(c, 1);
    return result(c, wrap(s8(c.u8(home_at) - c.u8(away_at))));
}
}
