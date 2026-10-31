// MATCH_ENG.CPP, tactics block A: the pitch cells around a player.
#include "match_eng_tactics_a.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace tactics_a;

namespace {
// Match fields (offsets from the match).
constexpr std::uint32_t ball_holder_at = 0xe000;    // word
constexpr std::uint32_t second_holder_at = 0xdff8;  // word
constexpr std::uint32_t play_state_at = 0x7928;
constexpr std::uint32_t cells_at = 0xa08;           // 9 rows of 0x450 bytes, 12 columns of 0x5c each
constexpr std::uint32_t row_stride = 0x450, column_stride = 0x5c, side_stride = 0x2c;
constexpr std::uint32_t cell_counts = 0x58;         // per side, after the 2 x 11 player words

// Pool offsets (doubles at TOC -0x52c4, singles at TOC -0x52c8).
constexpr std::uint32_t signed_bias = 0x60;

// `x / 20` as the original's mulhw sequence computes it (toward zero).
std::int32_t twentieth(std::int32_t value) {
    const auto high = std::int32_t((std::int64_t(0x66666667) * value) >> 32);
    const auto shifted = high >> 3;
    return shifted + std::int32_t(std::uint32_t(shifted) >> 31);
}

std::uint32_t halfword_sum(std::int32_t value) { return u(std::int16_t(value)); }
}

// 0x101d7dc0
std::uint32_t assess_cell(Cpu& c, std::uint32_t match, std::uint32_t player, std::uint32_t score_out,
                          std::uint32_t copy_to) {
    Frame frame(c, 0x101d7dc0, 0x90, {.first_gpr = 17});
    c.put32(frame.at(0xa8), match);
    const auto doubles = c.toc(-0x52c4);
    keep(c, {{28, player}, {25, score_out}, {18, copy_to}, {31, doubles}});
    Fpu fp(c);
    Kept<27> count(c, 0);
    Kept<20> best_value(c, u(-0x3e8));
    Kept<19> second_value(c, u(-0x3e8));
    Kept<26> best(c, 0);
    Kept<24> second(c, 0);
    c.put16(score_out, 0);
    auto scratch = held<17>(c);
    auto value = held<29>(c);

    scratch = u(s8(column(c, player)) * std::int32_t(column_stride));
    const auto row_part = u(s8(row(c, player)) * std::int32_t(row_stride));
    Kept<23> cell(c, c.u32(frame.at(0xa8)) + (row_part + scratch + cells_at));
    c.set(3, player);
    call(c, 0x10283690, 0x101d7e38);
    // The other side's players in the cell.
    Kept<21> opposing(c, compare(c, s8(c.r(3))) == 0);
    Kept<22> slot(c, 0);
    auto candidate = held<30>(c);

    // ((trait sum) - pressure / 20) as the integer converted below.
    const auto adjusted = [&](std::int32_t trait_sum) {
        const auto adjustment = std::int16_t(pressure_adjustment(c, player));
        return trait_sum - twentieth(adjustment);
    };

    while (compare(c, std::int32_t(std::uint32_t(slot)), c.i8(cell + cell_counts + u(s8(opposing)))) < 0) {
        candidate = c.u32(cell + u(s8(opposing) * std::int32_t(side_stride)) + (std::uint32_t(slot) << 2));
        if (compare(c, s8(position(c, candidate))) >= 0) {
            value = 0;
            c.set({{3, player}, {4, candidate}});
            call(c, 0x10283540, 0x101d7e90);
            if (compare(c, s8(c.r(3))) != 0) {
                count = count + 2;
                c.set(3, candidate);
                call(c, 0x102373c0, 0x101d7ea8);
                scratch = u(s8(c.r(3)));
                scratch = u(s8(trait_10d(c, candidate)) + s8(scratch));
                scratch = scratch - u(s8(trait_10a(c, player)));
                const auto net = adjusted(std::int32_t(scratch));
                c.set_f(1, c.f64(doubles + signed_bias));
                c.set_f(1, fp.fsub(integer_double(c, frame.at(0x48), net), c.f(1)));
                const auto weight = fp.fmul(c.f64(doubles + 0xb8), c.f(1));
                c.put64(frame.at(0x40), fp.fctiwz(0, weight));
                value = c.u32(frame.at(0x44));
                c.put16(score_out, std::uint32_t(c.i16(score_out)) + value);
                value = u(std::int16_t(value << 1));
            } else if (compare(c, c.i8(candidate + 0x2c)) >= 0) {
                c.set(3, candidate);
                call(c, 0x10289560, 0x101d7f54);
                if (compare(c, std::int16_t(c.r(3)), 1) != 0 &&
                    (compare(c, s8(trait_10d(c, candidate))) != 0 ||
                     compare(c, std::int16_t(action_code(c, candidate)) & 0x100) != 0)) {
                    count = count + 1;
                    scratch = u(s8(trait_10d(c, candidate)));
                    scratch = scratch - u(s8(trait_10a(c, player)));
                    const auto net = adjusted(std::int32_t(scratch));
                    c.set_f(1, c.f64(doubles + signed_bias));
                    c.set_f(1, fp.fsub(integer_double(c, frame.at(0x40), net), c.f(1)));
                    const auto weight = fp.fmul(c.f64(doubles + 0xb8), c.f(1));
                    c.set_f(1, c.f64(doubles + signed_bias));
                    const auto divisor =
                        fp.fsub(integer_double(c, frame.at(0x48), s8(c.u8(candidate + 0x2c)) + 1), c.f(1));
                    c.put64(frame.at(0x38), fp.fctiwz(0, fp.fdiv(weight, divisor)));
                    value = c.u32(frame.at(0x3c));
                    c.put16(score_out, std::uint32_t(c.i16(score_out)) + value);
                }
            }
            // The two best, kept for the caller.
            if (compare_logical(c, copy_to) != 0 && compare(c, std::int16_t(value), 5) > 0) {
                if (compare(c, std::int16_t(value), std::int16_t(best_value)) > 0) {
                    if (compare_logical(c, best) != 0) { second = best; second_value = best_value; }
                    best = candidate;
                    best_value = value;
                } else if (compare(c, std::int16_t(value), std::int16_t(second_value)) > 0) {
                    second = candidate;
                    second_value = value;
                }
            }
        }
        slot = slot + 1;
    }
    if (compare_logical(c, copy_to) != 0) {
        if (compare_logical(c, best) != 0) {
            c.set({{3, copy_to}, {4, best}, {5, 0x1c4}});
            call(c, 0x10738d10, 0x101d80d0);
        }
        if (compare_logical(c, second) != 0) {
            c.set({{3, copy_to}, {4, second}, {5, 0x1c4}});
            call(c, 0x10738d10, 0x101d80e8);
        }
    }
    return result(c, count);
}

// 0x101d8120
std::uint32_t marker_available(Cpu& c, std::uint32_t match, std::uint32_t player, std::uint32_t row_argument,
                               std::uint32_t column_argument) {
    Frame frame(c, 0x101d8120, 0x60, {.first_gpr = 23});
    keep(c, {{30, match}, {31, player}});
    c.put8(frame.at(0x83), row_argument);
    c.put8(frame.at(0x87), column_argument);
    Kept<29> available(c, 0);
    Kept<28> flagged(c, 0);      // holds a marker trait
    Kept<27> not_second(c, 0);
    Kept<26> not_holder(c, 0);
    Kept<25> youngster(c, 0);
    Kept<24> elsewhere(c, 0);
    auto differs = held<23>(c);
    if (compare_logical(c, player) != 0) {
        differs = 1;
        if (compare(c, c.i8(frame.at(0x83)), s8(row(c, player))) == 0 &&
            compare(c, c.i8(frame.at(0x87)), s8(column(c, player))) == 0)
            differs = 0;
        if (compare(c, std::int32_t(std::uint32_t(differs))) != 0) elsewhere = 1;
    }
    if (compare(c, std::int32_t(std::uint32_t(elsewhere))) != 0 && compare(c, c.i8(player + 0x2c), 2) < 0) youngster = 1;
    if (compare(c, std::int32_t(std::uint32_t(youngster))) != 0 &&
        compare_logical(c, player, c.u32(match + ball_holder_at)) != 0)
        not_holder = 1;
    if (compare(c, std::int32_t(std::uint32_t(not_holder))) != 0 &&
        compare_logical(c, player, c.u32(match + second_holder_at)) != 0)
        not_second = 1;
    if (compare(c, std::int32_t(std::uint32_t(not_second))) != 0) {
        c.set(3, player);
        call(c, 0x102373c0, 0x101d8210);
        if (compare(c, s8(c.r(3))) != 0) flagged = 1;
    }
    if (compare(c, std::int32_t(std::uint32_t(flagged))) != 0 && compare(c, c.i8(match + play_state_at)) == 0)
        available = 1;
    return result(c, u(s8(available)));
}

// 0x101d8d70
std::uint32_t lane_clear(Cpu& c, std::uint32_t match, std::uint32_t player, std::uint32_t flag) {
    Frame frame(c, 0x101d8d70, 0x60, {.first_gpr = 22});
    keep(c, {{24, match}, {30, player}});
    c.put8(frame.at(0x83), flag);
    c.set(3, player);
    call(c, 0x10283690, 0x101d8d94);
    Kept<25> side(c, c.r(3));
    c.set(3, player);
    call(c, 0x10283690, 0x101d8da0);
    Kept<26> opposing(c, s8(c.r(3)) == 0);
    auto cell_column = held<22>(c);
    if (compare(c, c.i8(frame.at(0x83))) == 0) {
        // The cell itself must hold none of the other side.
        cell_column = u(s8(column(c, player)) * std::int32_t(column_stride));
        const auto row_part = u(s8(row(c, player)) * std::int32_t(row_stride));
        if (compare(c, c.i8(match + (row_part + cell_column + s8(opposing)) + cells_at + cell_counts)) != 0)
            return result(c, 0);
    }
    Kept<28> player_row(c, row(c, player));
    Kept<27> player_column(c, column(c, player));
    Kept<31> opponent(c, match + u(s8(opposing) * 0x2350) + players_at);
    Kept<29> index(c, 0);
    auto previous = held<23>(c);
    for (; compare(c, std::int32_t(std::uint32_t(index)), 0x14) < 0; index = index + 1, previous = opponent, opponent = opponent + player_size) {
        if (compare(c, s8(position(c, opponent))) < 0) continue;
        c.set(3, opponent);
        call(c, 0x10289560, 0x101d8e54);
        if (compare(c, std::int16_t(c.r(3)), 1) == 0) continue;
        // On the far side of the player along the lane (columns), for either side of the pitch.
        bool beyond = false;
        if (compare(c, s8(side), 1) == 0) {
            const auto other = s8(column(c, opponent));
            beyond = compare(c, s8(player_column), other) > 0;
        }
        if (!beyond) {
            if (compare(c, s8(side)) != 0) continue;
            const auto other = s8(column(c, opponent));
            if (compare(c, s8(player_column), other) >= 0) continue;
        }
        const auto opponent_row = s8(row(c, opponent));
        c.set(4, u(opponent_row));
        if (compare(c, s8(player_row) + 1, opponent_row) <= 0) continue;
        const auto again = s8(row(c, opponent));
        c.set(4, u(again));
        if (compare(c, s8(player_row) - 1, again) >= 0) continue;
        return result(c, 0);
    }
    return result(c, 1);
}

// 0x101d8f20
void rate_goal_pressure(Cpu& c, std::uint32_t match, std::uint32_t side_argument) {
    Frame frame(c, 0x101d8f20, 0x60, {.first_gpr = 25});
    keep(c, {{31, match}, {25, side_argument}});
    Kept<30> weighted(c, 0);
    const auto flag_at = match + u(s8(side_argument)) + 6;
    c.set(4, 0);
    c.put8(flag_at, 0);
    const auto count_at = [&](std::int32_t row_index, std::int32_t column_index) {
        return match + u(row_index * 0x450) + u(column_index * 0x5c) + (s8(side_argument) == 0) + 0xa60;
    };
    if (compare(c, s8(side_argument)) == 0) {
        // Opponents in the six columns by our goal, the nearer the heavier.
        Kept<29> column_index(c, 5);
        for (; compare(c, s8(column_index), 0) >= 0; column_index = column_index - 1) {
            Kept<27> row_index(c, 0);
            for (; compare(c, s8(row_index), 9) < 0; row_index = row_index + 1) {
                const auto count = s8(c.u8(count_at(s8(row_index), s8(column_index))));
                weighted = weighted + halfword_sum(count * (6 - s8(column_index)));
            }
        }
    } else {
        Kept<28> column_index(c, 6);
        for (; compare(c, s8(column_index), 12) < 0; column_index = column_index + 1) {
            Kept<26> row_index(c, 0);
            for (; compare(c, s8(row_index), 9) < 0; row_index = row_index + 1) {
                const auto count = s8(c.u8(count_at(s8(row_index), s8(column_index))));
                c.set(4, u(count));
                weighted = weighted + halfword_sum(count * (s8(column_index) - 5));
            }
        }
    }
    c.set(3, 0xf);
    call(c, 0x101d3c90, 0x101d904c);
    c.set(3, c.r(3) + 0x1e);
    if (compare(c, std::int16_t(weighted), std::int32_t(c.r(3))) > 0) {
        c.set({{4, 1}, {3, u(s8(side_argument))}});
        c.put8(flag_at, 1);
    }
}
}
