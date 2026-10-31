// MATCH_ENG.CPP, tactics block A: the challenge target of a player.
#include "match_eng_tactics_a.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace tactics_a;

namespace {
// Match fields (offsets from the match).
constexpr std::uint32_t ball_holder_at = 0xe000;    // the player holding the ball (word)
constexpr std::uint32_t second_holder_at = 0xdff8;  // word
constexpr std::uint32_t cells_at = 0xa08;           // 9 rows of 0x450 bytes, 12 columns of 0x5c each
constexpr std::uint32_t row_stride = 0x450, column_stride = 0x5c, side_stride = 0x2c;
constexpr std::uint32_t cell_counts = 0x58;         // per side, after the 2 x 11 player words

// Pool offsets (doubles at TOC -0x52c4, singles at TOC -0x52c8).
constexpr std::uint32_t near_limit = 0x98, far_limit = 0x30, signed_bias = 0x60, unsigned_bias = 0xb0;

// The original's multiply by 0x51eb851f: the high word, the shifted quotient and its sign bit.
struct Scaled { std::int32_t quotient; std::int32_t high; std::uint32_t sign; };
Scaled hundredths(std::int32_t value, unsigned shift) {
    const auto high = std::int32_t((std::int64_t(0x51eb851f) * value) >> 32);
    const auto shifted = high >> shift;
    const auto sign = std::uint32_t(shifted) >> 31;
    return {shifted + std::int32_t(sign), high, sign};
}
std::uint32_t halfword_of(std::int32_t value) { return u(std::int16_t(value)); }

// `cror eq,x,eq` after a floating compare: EQ also set when `bit` (LT 8, GT 4) is.
void equal_or(Cpu& c, std::uint8_t bit) {
    const auto field = c.cr(0);
    if (field & bit) c.set_cr(0, field | 2);
}
}

// 0x101d7d30
std::uint32_t side_flag_400(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101d7d30, 0x40, {});
    c.put32(frame.at(0x58), match);
    c.put8(frame.at(0x5f), side);
    const auto team = match + u(s8(side) * 0x18e0) + 0x7c6c;
    c.set(4, match);
    enter(c, 0x101d7d60, {team});
    const auto flags = tactical_flags(c, team);
    return result(c, compare(c, std::int32_t(flags & 0x400)) != 0);
}

// 0x101d70d0
std::uint32_t choose_challenge_target(Cpu& c, std::uint32_t match, std::uint32_t player) {
    Frame frame(c, 0x101d70d0, 0xd0, {.first_gpr = 15, .first_fpr = 30});
    const auto floats = c.toc(-0x52c8), doubles = c.toc(-0x52c4);
    keep(c, {{27, match}, {26, player}, {31, floats}, {30, doubles}});
    Fpu fp(c);
    // Stack locals: the window of cells (column and row of the player's neighbourhood),
    // the loop index in a cell, and the best score so far.
    const auto column_base_at = frame.at(0x39), row_base_at = frame.at(0x3a), index_at = frame.at(0x38),
               best_score_at = frame.at(0x3c);
    c.put16(best_score_at, 0);
    Kept<16> best(c, 0);

    // The registers the original keeps across its calls.
    auto scratch = held<15>(c);      // r15: temporary
    auto opposing_team = held<17>(c);
    auto own_team = held<18>(c);
    auto cell = held<19>(c);
    auto closeness = held<20>(c);    // how far apart the two stand, from 0x101e4020
    auto relation = held<21>(c);     // team relation value (halfword)
    auto marking = held<22>(c);      // the candidate's marker count
    auto opposing = held<23>(c);     // the opposing side
    auto scan_column = held<24>(c);
    auto scan_row = held<25>(c);
    auto score = held<28>(c);
    auto candidate = held<29>(c);
    KeptFloat<30> distance(c);       // 0x101ef8d0 between the two
    KeptFloat<31> goal_distance(c);

    const auto holder_of_ball = [&] { return c.u32(match + ball_holder_at); };
    const auto random = [&](std::uint32_t limit, std::uint32_t back) {
        c.set(3, limit);
        call(c, 0x101d3c90, back);
        return c.r(3);
    };
    // A distance: 0x101ef8d0(match, row, column, row base, column base) or its goal form.
    const auto measure = [&](std::uint32_t row, std::uint32_t column, std::uint32_t a, std::uint32_t b,
                             std::uint32_t back) {
        c.set({{3, match}, {4, row}, {5, column}, {6, a}, {7, b}});
        call(c, 0x101ef8d0, back);
        return c.f(1);
    };
    const auto closeness_of = [&](std::uint32_t row, std::uint32_t column, std::uint32_t back) {
        c.set({{3, match}, {4, opposing}, {5, row}, {6, column}, {7, 1}});
        call(c, 0x101e4020, back);
        closeness = c.r(3);
    };

    // The best candidate so far (`wide`: the cell scan also leaves r15 loaded).
    const auto record = [&](bool wide) {
        if (compare_logical(c, best) != 0) {
            const auto previous = c.i16(best_score_at);
            if (wide) scratch = u(previous);
            if (compare(c, std::int16_t(score), previous) <= 0) return;
        }
        best = candidate;
        c.put16(best_score_at, score);
    };
    // score += (int) (single[0x24] - single[0x28] * goal distance), through the stack slot.
    const auto goal_term = [&](std::uint32_t site, std::uint32_t slot) {
        c.set_f(1, c.f32(floats + 0x28));
        const auto term = fp.multiply_add(site, c.f(1), goal_distance, c.f32(floats + 0x24));
        c.put64(slot, fp.fctiwz(0, term));
        score = score + c.u32(slot + 4);
    };
    // Two draws of 0..29 that the fitness (single at `offset`) of candidate and player both
    // exceed give the goal term.
    const auto fitness_roll = [&](std::uint32_t slot, std::uint32_t offset, std::uint32_t first_back,
                                  std::uint32_t second_back, std::uint32_t term_site) {
        for (unsigned which = 0; which < 2; ++which) {
            const auto draw = random(0x1e, which ? second_back : first_back);
            c.set_f(1, c.f64(doubles + signed_bias));
            c.set_f(1, fp.fsubs(integer_double(c, slot, std::int32_t(draw)), c.f(1)));
            fp.fcmp(0, c.f32((which ? player : candidate) + offset), c.f(1));
            if (!(c.cr(0) & 4)) return;
        }
        goal_term(term_site, slot);
    };
    // Candidates standing on the wrong half of the pitch for a player with that trait are left out.
    const auto wrong_half = [&](bool at_carrier) {
        if (compare(c, s8(trait_10d(c, player))) == 0) return false;
        c.set(3, candidate);
        call(c, 0x102721a0, at_carrier ? 0x101d73c8 : 0x101d7814);
        if (compare(c, s8(c.r(3))) == 0) return false;
        if (at_carrier) c.set(4, opposing);
        if (compare(c, s8(opposing)) == 0) {
            if (at_carrier) c.set(3, c.u8(column_base_at));
            if (compare(c, s8(c.u8(column_base_at)), 6) >= 0) return true;
        }
        return compare(c, s8(opposing), 1) == 0 && compare(c, s8(c.u8(column_base_at)), 6) < 0;
    };
    const auto random_base = [&](std::uint32_t back) {
        score = halfword_of(std::int32_t(random(u((21 - s8(trait_10d(c, player))) * 5), back)));
    };
    // The score term for marks on the candidate.
    const auto marking_penalty = [&] { score = score - halfword_of(s8(marking) * 100); };
    // `score += extsh(r21 / 100 - 50)` after loading the relation value (a halfword of the candidate's
    // record: same team 8, same club 0xa, otherwise 0xc).
    const auto relation_term = [&](bool wide) {
        const auto player_team = c.u32(c.u32(player + 0x6c) + 0xc);
        const auto candidate_record = c.u32(candidate + 0x6c);
        const auto candidate_team = c.u32(candidate_record + 0xc);
        if (wide) { c.set(4, candidate_record); scratch = candidate_team; }
        const auto shared = [&](std::uint32_t at) { return u(c.i16(c.u32(candidate + 0x70) + at)); };
        if (compare_logical(c, player_team, candidate_team) == 0) {
            if (wide) c.set(4, c.u32(candidate + 0x70));
            relation = shared(8);
        } else if (compare_logical(c, own_team) != 0 && compare_logical(c, opposing_team) != 0 &&
                   compare_logical(c, c.u32(own_team + 0x2c), c.u32(opposing_team + 0x2c)) == 0) {
            if (wide) scratch = c.u32(candidate + 0x70);
            relation = shared(0xa);
        } else {
            relation = shared(0xc);
        }
        const std::int32_t value = std::int16_t(std::uint32_t(relation));
        const auto scaled = hundredths(value, 5);
        if (wide) { c.set({{5, u(value)}, {4, u(scaled.high)}}); scratch = scaled.sign; }
        score = score + halfword_of(scaled.quotient - 0x32);
    };
    // Terms from the goal distance and the distance between the two, shared by both scans.
    const auto distance_terms = [&](std::uint32_t goal_site, std::uint32_t goal_slot, std::uint32_t distance_slot) {
        const auto squared = fp.multiply_add(goal_site, goal_distance, goal_distance, c.f32(floats + 0x20));
        c.put64(goal_slot, fp.fctiwz(0, squared));
        score = score + c.u32(goal_slot + 4);
        c.set_f(1, c.f32(floats + 0x2c));
        const auto spread = fp.fmuls(c.f(1), fp.fmuls(distance, distance));
        c.put64(distance_slot, fp.fctiwz(0, spread));
        score = score - c.u32(distance_slot + 4);
    };

    // --- Preliminary answers ---
    c.set(4, position(c, player));
    c.set(3, match + u(s8(c.u8(player + 0x27)) * 0x18e0) + 0x7c6c);
    call(c, 0x101149f0, 0x101d7128);
    if (compare(c, s8(c.r(3)) & 1) != 0) return result(c, 0);

    // A player already marking someone live is his own answer.
    c.set(3, player);
    call(c, 0x10285ef0, 0x101d7148);
    candidate = c.r(3);
    if (compare_logical(c, candidate) != 0 && compare(c, s8(position(c, candidate))) >= 0)
        return result(c, candidate);

    // r23: the opposing side; r18 and r17: the player's team and the opposing one.
    opposing = s8(c.u8(player + 0x27)) == 0;
    const auto own_second = std::uint32_t(s8(opposing) == 0);
    c.set(4, own_second);
    enter(c, 0x101d719c, {match, own_second});
    own_team = fixture_team(c, match, own_second);
    enter(c, 0x101d71ac, {match, opposing});
    opposing_team = fixture_team(c, match, opposing);

    c.set({{3, match}, {4, player}, {5, row_base_at}, {6, column_base_at}, {7, 1}});
    call(c, 0x101e28b0, 0x101d71c8);
    // The window of cells: columns base-2 .. base+1 (the first only stands for the ball
    // carrier), rows base-1 .. base+1.
    scan_column = u(s8(c.u8(column_base_at) - 2u));

    enum class Next { row_done, skip, answer };
    const auto at_carrier = [&]() -> Next {
        if (compare(c, s8(scan_row), s8(c.u8(row_base_at)) - 1) != 0) return Next::row_done;
        c.set(4, holder_of_ball());
        if (compare_logical(c, holder_of_ball()) == 0) return Next::row_done;
        // Only a carrier within reach.
        scratch = column(c, player);
        c.set({{3, holder_of_ball()}, {4, row(c, player)}, {5, scratch}});
        call(c, 0x1022e760, 0x101d7280);
        fp.fcmp(0, c.f(1), c.f64(doubles + near_limit));
        equal_or(c, 8);
        if (!(c.cr(0) & 2)) return Next::row_done;

        scratch = column(c, holder_of_ball());
        const auto carrier_row = row(c, holder_of_ball());
        distance = measure(carrier_row, scratch, c.u8(row_base_at), c.u8(column_base_at), 0x101d72c4);
        scratch = column(c, holder_of_ball());
        closeness_of(row(c, holder_of_ball()), scratch, 0x101d72fc);
        const bool left_side = compare(c, s8(c.u8(player + 0x27))) == 0;
        scratch = column(c, holder_of_ball());
        goal_distance = measure(row(c, holder_of_ball()), scratch, 4, left_side ? 0 : 0xb,
                                left_side ? 0x101d7344 : 0x101d7380);
        goal_distance = fp.fadds(goal_distance, c.f32(floats + 8));

        candidate = holder_of_ball();
        marking = marking_count(c, candidate);
        c.set(4, s8(marking));
        if (compare(c, s8(marking), 3) == 0) return Next::skip;
        if (wrong_half(true)) return Next::skip;

        random_base(0x101d7424);
        fp.fcmp(0, goal_distance, c.f64(doubles + far_limit));
        if (c.cr(0) & 8) {
            if (compare(c, s8(trait_10c(c, candidate))) != 0) goal_term(0x101d7450, frame.at(0x68));
        }
        fitness_roll(frame.at(0x68), 0xb4, 0x101d746c, 0x101d749c, 0x101d74cc);
        marking_penalty();
        c.set({{3, match}, {4, candidate}});
        call(c, 0x101ee770, 0x101d74fc);
        if (compare(c, s8(c.r(3))) != 0) {
            if (compare_logical(c, candidate, marking_target(c, player)) == 0) return Next::answer;
            score = score + 0xaf;
        } else if (compare_logical(c, candidate, marking_target(c, player)) == 0) {
            if (compare(c, std::int32_t(random(u(s8(marking) + 1), 0x101d7544))) == 0) return Next::answer;
        }
        distance_terms(0x101d7558, frame.at(0x68), frame.at(0x60));
        const std::uint32_t k = s8(closeness);
        score = score + halfword_of(std::int32_t(200 - k * k * k));
        score = score + halfword_of(hundredths(c.i16(candidate + 0x36), 6).quotient);
        score = score + halfword_of(c.u8(candidate + 0xa) * 10);
        score = score + halfword_of(c.u8(candidate + 0xb) * 10);
        c.set_f(1, c.f64(doubles + unsigned_bias));
        const auto youth = fp.fmul(c.f64(doubles + 0xa0),
                                   fp.fsub(unsigned_double(c, frame.at(0x58), c.u8(candidate + 0x10)), c.f(1)));
        c.put64(frame.at(0x50), fp.fctiwz(0, youth));
        score = score + c.u32(frame.at(0x54));
        relation_term(false);
        record(false);
        return Next::row_done;
    };

    // One player in the cell being scanned; true when he is the answer outright.
    const auto in_cell = [&]() -> bool {
        const auto j = s8(c.u8(index_at));
        candidate = c.u32(cell + u(s8(opposing) * std::int32_t(side_stride)) + (u(j) << 2));
        if (compare_logical(c, candidate, holder_of_ball()) == 0) return false;
        marking = marking_count(c, candidate);
        if (compare(c, s8(marking), 3) == 0) return false;
        if (wrong_half(false)) return false;
        if (compare_logical(c, candidate, holder_of_ball()) != 0 &&
            compare_logical(c, candidate, c.u32(match + second_holder_at)) != 0) {
            const auto side = c.u8(player + 0x27);
            enter(c, 0x101d7884, {match, side});
            const bool flagged = compare(c, s8(side_flag_400(c, match, side))) != 0;
            const bool strict = flagged && compare(c, s8(trait_10d(c, player))) != 0;
            c.set(3, candidate);
            call(c, 0x10285750, strict ? 0x101d78ac : 0x101d78c4);
            const auto outcome = compare(c, s8(c.r(3)));
            if (strict ? outcome >= 0 : outcome > 0) return false;
        }
        random_base(0x101d78e8);
        fitness_roll(frame.at(0x50), 0xb0, 0x101d78f4, 0x101d7924, 0x101d7954);
        marking_penalty();
        c.set({{3, match}, {4, candidate}});
        call(c, 0x101ee770, 0x101d7984);
        if (compare(c, s8(c.r(3))) != 0) {
            if (compare_logical(c, candidate, marking_target(c, player)) == 0) return true;
            score = score + 0xaf;
        } else if (compare_logical(c, candidate, marking_target(c, player)) == 0) {
            if (compare(c, std::int32_t(random(u(s8(marking) + 1), 0x101d79cc))) == 0) {
                c.set(3, candidate);
                call(c, 0x102775c0, 0x101d79dc);
                if (compare(c, s8(c.r(3))) != 0) return true;
                if (compare_logical(c, candidate, c.u32(match + second_holder_at)) == 0) return true;
                score = score + 0x14;
            }
        }
        distance_terms(0x101d7a08, frame.at(0x50), frame.at(0x58));
        const std::uint32_t k = s8(closeness);
        score = score + halfword_of(std::int32_t(100 - k * k));
        c.set_f(1, c.f64(doubles + signed_bias));
        c.set_f(1, fp.fsub(integer_double(c, frame.at(0x60), c.i16(candidate + 0x36)), c.f(1)));
        const auto age = fp.multiply_add(0x101d7a78, c.f64(doubles + 0xa8), c.f(1), c.f64(doubles + 0x18));
        c.put64(frame.at(0x68), fp.fctiwz(0, age));
        score = score + c.u32(frame.at(0x6c));
        c.set(4, c.u8(candidate + 0xa) * 10);
        score = score + halfword_of(c.u8(candidate + 0xa) * 10);
        c.set(3, halfword_of(c.u8(candidate + 0xb) * 10));
        score = score + c.r(3);
        c.set_f(1, c.f64(doubles + unsigned_bias));
        const auto youth = fp.fmul(c.f64(doubles + 0xa0),
                                   fp.fsub(unsigned_double(c, frame.at(0x48), c.u8(candidate + 0x10)), c.f(1)));
        c.put64(frame.at(0x40), fp.fctiwz(0, youth));
        c.set(3, c.u32(frame.at(0x44)));
        score = score + c.r(3);
        // The candidate's state flags (0x101e3d50 and 0x101e3de0) adjust the score by what he is doing.
        const auto state_flags = [&](std::uint32_t entry, std::uint32_t back) {
            c.set({{3, match}, {4, candidate}});
            call(c, entry, back);
            return std::int32_t(std::int16_t(c.r(3)));
        };
        if (compare(c, state_flags(0x101e3d50, 0x101d7aec) & 0x20) != 0 &&
            compare(c, state_flags(0x101e3d50, 0x101d7b08) & 0x800) == 0 &&
            compare(c, state_flags(0x101e3d50, 0x101d7b24) & 0x80) == 0) {
            score = score + halfword_of(s8(c.u8(c.u32(candidate + 0x70) + 0x1a)) * 10);
        } else {
            bool tired = compare(c, state_flags(0x101e3d50, 0x101d7b5c), 0x140) == 0 ||
                         compare(c, state_flags(0x101e3d50, 0x101d7b74), 0x240) == 0 ||
                         compare(c, state_flags(0x101e3d50, 0x101d7b8c), 0x440) == 0;
            if (!tired) {
                if (compare(c, state_flags(0x101e3de0, 0x101d7ba8), 0x140) == 0 ||
                    compare(c, state_flags(0x101e3de0, 0x101d7bc0), 0x240) == 0 ||
                    compare(c, state_flags(0x101e3de0, 0x101d7bd8), 0x440) == 0)
                    score = score + 0x7d;
                else
                    tired = true;
            }
            if (tired) {
                c.set(3, candidate);
                call(c, 0x102864f0, 0x101d7bf4);
                if (compare(c, s8(c.r(3))) != 0) score = score + 0x28;
            }
        }
        relation_term(true);
        record(true);
        return false;
    };

    const auto column_in_window = [&] {
        c.set({{4, scan_column}, {3, c.u8(column_base_at)}});
        return compare(c, s8(scan_column), s8(c.u8(column_base_at) + 1u)) <= 0;
    };
    const auto row_in_window = [&] {
        c.set({{4, scan_row}, {3, u(s8(c.u8(row_base_at) + 1u))}});
        return compare(c, s8(scan_row), s8(c.u8(row_base_at) + 1u)) <= 0;
    };
    for (; column_in_window(); scan_column = scan_column + 1) {
        if (compare(c, s8(scan_column), s8(c.u8(column_base_at)) - 2) != 0) {
            if (compare(c, s8(scan_column), 0) < 0) continue;
            if (compare(c, s8(scan_column), 0xb) > 0) continue;
        }
        scan_row = u(s8(c.u8(row_base_at) - 1u));
        for (; row_in_window(); scan_row = scan_row + 1) {
            if (compare(c, s8(scan_column), s8(c.u8(column_base_at)) - 2) == 0) {
                const auto next = at_carrier();
                if (next == Next::answer) return result(c, candidate);
                if (next == Next::row_done) scan_row = u(s8(c.u8(row_base_at) + 1u));
                continue;
            }
            if (compare(c, s8(scan_row), 0) < 0) continue;
            if (compare(c, s8(scan_row), 8) > 0) continue;
            cell = match + u(s8(scan_row) * std::int32_t(row_stride)) + u(s8(scan_column) * std::int32_t(column_stride)) + cells_at;
            if (compare(c, s8(c.u8(cell + u(s8(opposing)) + cell_counts))) == 0) continue;
            distance = measure(scan_row, scan_column, c.u8(row_base_at), c.u8(column_base_at), 0x101d7734);
            closeness_of(scan_row, scan_column, 0x101d7750);
            const bool left_side = compare(c, s8(c.u8(player + 0x27))) == 0;
            goal_distance = measure(scan_row, scan_column, 4, left_side ? 0 : 0xb, left_side ? 0x101d777c : 0x101d779c);
            goal_distance = fp.fadds(goal_distance, c.f32(floats + 8));
            c.put8(index_at, 0);
            while (true) {
                scratch = u(s8(c.u8(index_at)));
                c.set(4, u(s8(opposing)) + cell_counts);
                const auto count = s8(c.u8(cell + u(s8(opposing)) + cell_counts));
                c.set(3, u(count));
                if (compare(c, std::int32_t(std::uint32_t(scratch)), count) >= 0) break;
                if (in_cell()) return result(c, candidate);
                const auto index = c.u8(index_at);
                c.set({{3, index}, {4, index + 1u}});
                c.put8(index_at, index + 1u);
            }
        }
    }
    return result(c, best);
}
}
