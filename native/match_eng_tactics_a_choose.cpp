// MATCH_ENG.CPP, tactics block A: the destination cell of a player.
#include "match_eng_tactics_a.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace tactics_a;

namespace {
constexpr std::uint32_t cells_at = 0xa08;     // 9 rows of 0x450 bytes, 12 columns of 0x5c each
constexpr std::uint32_t row_stride = 0x450, column_stride = 0x5c, side_stride = 0x2c;
constexpr std::uint32_t cell_counts = 0x58;   // per side, after the 2 x 11 player words

// Pool offsets (doubles at TOC -0x52c4).
constexpr std::uint32_t signed_bias = 0x60;

// Bits of the caller's mode.
constexpr std::int32_t mode_first = 1, mode_open = 2, mode_support = 4, mode_cross = 8;
}

// 0x101d8260
std::uint32_t choose_cell(Cpu& c, std::uint32_t match, std::uint32_t player, std::uint32_t row_out,
                          std::uint32_t column_out, std::uint32_t mode_argument) {
    Frame frame(c, 0x101d8260, 0xc0, {.first_gpr = 15, .first_fpr = 28});
    c.put32(frame.at(0xe0), row_out);
    c.put32(frame.at(0xe4), column_out);
    const auto floats = c.toc(-0x52c8), doubles = c.toc(-0x52c4);
    keep(c, {{23, match}, {29, player}, {24, mode_argument}, {30, floats}, {31, doubles}});
    Fpu fp(c);
    const auto mode = [&] { return s8(c.r(24)); };
    const auto best_column_at = frame.at(0x38);
    Kept<22> pressure(c, u(-10));
    Kept<16> best_row(c, u(-1));
    c.put8(best_column_at, 0xff);
    KeptFloat<30> base(c, c.f32(floats + 0xc));
    KeptFloat<29> best_score(c, c.f32(floats + 0x30));
    KeptFloat<31> cost(c);
    KeptFloat<28> crowd(c);
    auto scratch = held<15>(c);
    auto forward = held<17>(c);
    auto index = held<18>(c);
    auto cell = held<21>(c);
    auto near = held<25>(c);
    auto scan_column = held<26>(c);
    auto scan_row = held<28>(c);

    const auto random = [&](std::uint32_t limit, std::uint32_t back) {
        return draw(c, limit, back);
    };
    // `lfd f1,k60; xoris; stw; stw; lfd f0; fsubs f0,f0,f1`: a signed integer as a single; the
    // result also lands in f1 where the original writes it there.
    const auto single = [&](std::uint32_t at, std::int32_t value, bool into_f1) {
        c.set_f(1, c.f64(doubles + signed_bias));
        const auto converted = fp.fsubs(integer_double(c, at, value), c.f(1));
        if (into_f1) c.set_f(1, converted);
        return converted;
    };
    // The same through `fsub` (a double result).
    const auto exact = [&](std::uint32_t at, std::int32_t value) {
        c.set_f(1, c.f64(doubles + signed_bias));
        return fp.fsub(integer_double(c, at, value), c.f(1));
    };
    const auto cost_minus = [&](double term) { cost = fp.fsubs(cost, term); };
    const auto cost_plus_pool = [&](std::uint32_t offset) { cost = fp.frsp(fp.fadd(cost, c.f64(doubles + offset))); };

    Kept<27> player_row(c, u(s8(row(c, player))));
    Kept<20> player_column(c, u(s8(column(c, player))));
    const auto r = [&] { return s8(scan_row); };
    const auto col = [&] { return s8(scan_column); };
    const auto row_p = [&] { return s8(player_row); };
    const auto col_p = [&] { return s8(player_column); };
    const auto side_of_player = [&] { return s8(c.u8(player + 0x27)); };

    if (compare(c, mode() & mode_open) == 0) {
        const auto trait = s8(trait_10a(c, player));
        c.set(4, u(trait));
        if (compare(c, trait) == 0) {
            if (compare(c, s8(secondary_rating(c, player))) == 0) return result(c, 0);
            scratch = random(6, 0x101d830c);
            const auto second = random(6, 0x101d8318);
            base = single(frame.at(0x4c), std::int32_t(std::uint32_t(scratch) - second), false);
        }
    }
    c.set(3, player);
    call(c, 0x10283690, 0x101d8340);
    Kept<19> opposite(c, s8(c.r(3)) == 0);

    // The cost of standing in the cell (scan_row, scan_column): one candidate.
    const auto evaluate = [&] {
        cell = match + u(r() * std::int32_t(row_stride)) + u(col() * std::int32_t(column_stride)) + cells_at;
        cost = base;
        near = 0;
        if (compare(c, mode() & mode_open) != 0) {
            // Moving into a cell the other side already holds weighs less when the player is no good at it.
            near = c.u8(cell + u(s8(opposite)) + cell_counts);
            if (compare(c, s8(near)) != 0) {
                scratch = random(3, 0x101d85b4);
                c.set(3, u(s8(c.u8(c.u32(player + 0x70) + 0x26))));
                const auto draw = random(c.r(3), 0x101d85c8);
                if (compare(c, std::int32_t(draw), std::int32_t(std::uint32_t(scratch))) > 0) {
                    const auto reduced = s8(near) - 2;
                    c.put32(frame.at(0x3c), compare(c, reduced) < 0 ? 0 : u(reduced));
                    near = u(s8(c.u32(frame.at(0x3c))));
                }
            }
            const auto presence = single(frame.at(0x4c), s8(near), true);
            cost_minus(fp.frsp(fp.fsub(presence, c.f64(doubles + 0x90))));
            const auto kind = s8(c.u8(player + 0x2d));
            const auto ordering = compare(c, kind, 1);
            if (ordering == 0 || (ordering < 0 && compare(c, kind, 0) >= 0)) {
                // Kinds 0 and 1 charge the player's pace for cells ahead of him (the side mirrors it).
                const bool high = (compare(c, side_of_player()) == 0) == (kind == 0);
                bool charged;
                if (high) {
                    charged = compare(c, r(), row_p()) > 0;
                    if (!charged && compare(c, r(), row_p()) >= 0) charged = compare(c, r(), 4) > 0;
                } else {
                    charged = compare(c, r(), row_p()) < 0;
                    if (!charged && compare(c, r(), row_p()) <= 0) charged = compare(c, r(), 4) < 0;
                }
                if (charged) {
                    const auto pace = single(frame.at(0x4c), s8(c.u8(player + 0x2e)), true);
                    cost_minus(fp.frsp(fp.fmul(pace, c.f64(doubles + 0xc0))));
                }
            }
        } else {
            // The other side's players in the cell who are on the pitch and not fresh.
            index = 0;
            for (;; index = index + 1) {
                c.set(4, u(s8(index)));
                const auto count = s8(c.u8(cell + u(s8(opposite)) + cell_counts));
                if (compare(c, s8(index), count) >= 0) break;
                c.put32(frame.at(0x40), c.u32(cell + u(s8(opposite) * std::int32_t(side_stride)) + (u(s8(index)) << 2)));
                if (compare(c, s8(position(c, c.u32(frame.at(0x40))))) >= 0 &&
                    compare(c, c.i8(c.u32(frame.at(0x40)) + 0x2c), 2) < 0)
                    near = near + 1;
            }
        }
        if (compare(c, s8(near)) == 0 || compare(c, mode() & mode_open) != 0) {
            c.set({{3, match}, {4, opposite}, {5, scan_row}, {6, scan_column}, {7, 1}});
            call(c, 0x101e3f10, 0x101d88a4);
            crowd = c.f(1);
            cost = fp.fadds(cost, fp.fmuls(crowd, crowd));
        } else {
            c.set_f(1, c.f64(doubles + signed_bias));
            const auto first = single(frame.at(0x4c), s8(near), false);
            const auto second = single(frame.at(0x44), s8(near), false);
            cost_minus(fp.frsp(fp.fmul(c.f64(doubles + 0x98), fp.fmuls(first, second))));
        }

        // Cells behind the player's column.
        bool behind = false;
        if (compare(c, side_of_player()) == 0) behind = compare(c, col(), s8(column(c, player))) < 0;
        if (!behind && compare(c, side_of_player(), 1) == 0) behind = compare(c, col(), s8(column(c, player))) > 0;
        if (behind) {
            if (compare(c, mode() & mode_cross) != 0) {
                forward = 0;
            } else {
                forward = 1;
                cost = fp.frsp(fp.fsub(cost, c.f64(doubles + 0xc8)));
            }
        } else {
            forward = 0;
            cost_plus_pool(compare(c, col(), col_p()) != 0 ? 0xd0 : 0x90);
        }
        if (compare(c, s8(pressure)) > 0) cost_minus(single(frame.at(0x44), s8(pressure) * 4, false));
        if (compare(c, s8(forward)) == 0) {
            if (compare(c, mode() & mode_open) == 0 || compare(c, mode() & mode_first) != 0) {
                if (compare(c, col(), 3) < 0 || compare(c, col(), 8) > 0) {
                    if (compare(c, r(), 6) < 0 && compare(c, r(), 2) > 0) cost_plus_pool(0xd8);
                }
            }
            // A cell level with the middle rows (or between the player and them) gets the pool bonus.
            const auto player_row_now = [&] { return s8(row(c, player)); };
            bool central = compare(c, r(), 4) == 0;
            if (!central) {
                if (compare(c, r(), 2) > 0 && compare(c, player_row_now(), 4) < 0 && compare(c, player_row_now(), r()) >= 0)
                    central = true;
                if (!central && compare(c, r(), 6) < 0 && compare(c, player_row_now(), 4) > 0 &&
                    compare(c, player_row_now(), r()) <= 0)
                    central = true;
            }
            if (central) cost_plus_pool(0x40);
            if (compare(c, mode() & mode_open) == 0) {
                // Moving back past the player's row costs three per row.
                bool charged = false;
                if (compare(c, row_p(), 4) > 0 && compare(c, row_p() - 4, r() - 4) < 0) {
                    cost_minus(single(frame.at(0x44), (row_p() - 4) * 3, false));
                    charged = true;
                }
                if (!charged && compare(c, row_p(), 4) < 0 && compare(c, row_p() - 4, r() - 4) > 0)
                    cost_minus(single(frame.at(0x44), (4 - row_p()) * 3, false));
            } else {
                c.set({{3, match}, {4, player_row}});
                call(c, 0x101ee9c0, 0x101d8b9c);
                if (compare(c, s8(c.r(3))) != 0) {
                    c.set({{3, match}, {4, scan_row}});
                    call(c, 0x101ee9c0, 0x101d8bb4);
                    if (compare(c, s8(c.r(3))) != 0 && compare(c, mode() & mode_first) == 0) {
                        c.set_f(1, c.f32(player + 0xe4));
                        cost = fp.fadds(cost, fp.frsp(fp.fmul(c.f(1), c.f64(doubles + 0xe0))));
                    }
                }
                bool charged = false;
                if (compare(c, row_p(), 4) >= 0 && compare(c, row_p() - 4, r() - 4) < 0) {
                    scratch = u(row_p());
                    cost_minus(fp.frsp(fp.fmul(c.f64(doubles + 0xe8), exact(frame.at(0x44), row_p() - 4))));
                    charged = true;
                }
                if (!charged && compare(c, row_p(), 4) <= 0 && compare(c, row_p() - 4, r() - 4) > 0)
                    cost_minus(fp.frsp(fp.fmul(c.f64(doubles + 0xe8), exact(frame.at(0x44), 4 - row_p()))));
                // The cell the caller already holds in its output bytes gets a bonus.
                const auto held_row = s8(c.u8(c.u32(frame.at(0xe0))));
                scratch = u(r());
                if (compare(c, held_row, r()) == 0) {
                    c.set(4, c.u32(frame.at(0xe4)));
                    scratch = u(s8(c.u8(c.u32(frame.at(0xe4)))));
                    if (compare(c, std::int32_t(std::uint32_t(scratch)), col()) == 0)
                        cost = fp.fadds(cost, c.f32(floats + 0x34));
                }
            }
        }
        fp.fcmp(0, cost, best_score);
        equal_or(c, 4);
        if (c.cr(0) & 2) {
            best_score = cost;
            best_row = scan_row;
            c.put8(best_column_at, scan_column);
        }
    };

    scan_column = u(s8(player_column - 1u));
    for (;; scan_column = scan_column + 1) {
        c.set({{4, u(s8(scan_column))}, {3, player_column + 1u}});
        if (compare(c, col(), s8(player_column + 1u)) > 0) break;
        if (compare(c, col(), 0) < 0) continue;
        if (compare(c, col(), 0xc) >= 0) continue;
        if (compare(c, mode() & mode_support) != 0) {
            // The crowding of the column (0x101e4ea0), softened by a draw for a poor judge.
            if (compare(c, s8(trait_10a(c, player)), 2) < 0) {
                scratch = random(3, 0x101d83a0);
                c.set(3, player);
                call(c, 0x10283690, 0x101d83ac);
                c.set({{3, match}, {4, c.r(3)}, {5, scan_column}});
                call(c, 0x101e4ea0, 0x101d83bc);
                scratch = c.r(3) + scratch;
                const auto jitter = random(3, 0x101d83c8);
                pressure = u(s8(std::uint32_t(scratch) - jitter));
            } else {
                c.set(3, player);
                call(c, 0x10283690, 0x101d83dc);
                c.set({{3, match}, {4, c.r(3)}, {5, scan_column}});
                call(c, 0x101e4ea0, 0x101d83ec);
                pressure = c.r(3);
            }
            const auto adjustment = s8(adjustment_byte(c, player));
            c.set(4, u(adjustment));
            if (compare(c, adjustment) != 0) {
                c.set(3, player);
                call(c, 0x10283690, 0x101d840c);
                const auto other = std::uint32_t(s8(c.r(3)) == 0);
                c.set(4, other);
                enter(c, 0x101d8424, {match, other});
                if (compare(c, s8(side_flag_400(c, match, other))) != 0) pressure = pressure + 1;
            }
            if (compare(c, s8(pressure), 2) > 0) continue;
        }
        scan_row = u(s8(player_row - 1u));
        for (;; scan_row = scan_row + 1) {
            if (compare(c, r(), s8(player_row + 1u)) > 0) break;
            if (compare(c, r(), 0) < 0) continue;
            if (compare(c, r(), 9) >= 0) continue;
            if (compare(c, row_p(), r()) == 0 && compare(c, col_p(), col()) == 0 && compare(c, mode() & mode_open) != 0)
                continue;
            if (compare(c, mode() & mode_open) != 0) {
                c.set({{3, player}, {4, scan_row}, {5, scan_column}});
                call(c, 0x10245050, 0x101d84b4);
                if (compare(c, s8(c.r(3))) == 0) continue;
            }
            // A tired or restricted player cannot take a cell the ball side is already contesting.
            c.set({{3, match}, {4, player}});
            call(c, 0x101e3d50, 0x101d84cc);
            if (compare(c, std::int16_t(c.r(3)) & 0x30) != 0) {
                c.set({{3, match}, {4, player}});
                call(c, 0x101e3d50, 0x101d84e8);
                const auto flags = std::int16_t(c.r(3)) & 0x880;
                compare(c, flags);
                if (compare(c, flags) != 0) {
                    c.set({{3, match}, {4, scan_row}});
                    call(c, 0x101ee9c0, 0x101d8504);
                    if (compare(c, s8(c.r(3))) == 0) {
                        c.set({{3, match}, {4, player_row}});
                        call(c, 0x101ee9c0, 0x101d851c);
                        if (compare(c, s8(c.r(3))) != 0) {
                            c.set(3, player);
                            call(c, 0x10283690, 0x101d8530);
                            c.set({{3, match}, {4, c.r(3)}, {5, player_row}, {6, player_column}});
                            call(c, 0x101eea30, 0x101d8544);
                            if (compare(c, s8(c.r(3))) == 0 && compare(c, mode() & mode_first) == 0) continue;
                        }
                    }
                }
            }
            evaluate();
        }
    }
    // The answer goes out through the two pointers the caller gave.
    c.set(4, c.u32(frame.at(0xe4)));
    c.put8(c.u32(frame.at(0xe4)), c.u8(best_column_at));
    c.put8(c.u32(frame.at(0xe0)), best_row);
    return result(c, compare(c, s8(best_row), -1) > 0);
}
}
