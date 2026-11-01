// Cases for the team_a block (the receiver search 0x101e8950, the ball-cell search 0x101eaf60, pow
// 0x101eaf10 and the random mark 0x101ec7b0) from a real match state: every kind code, with and
// without a reference player, both sides, and the assertion and null-candidate paths.
#include "match_eng_checks.hpp"
#include <bit>
#include "guest_abi.hpp"
#include "match_eng.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("team_a") {
    const auto match = scenario.match;
    const auto sp = std::uint32_t(scenario.s.r<4>(4));
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory,
                         std::string label, std::optional<double> f1 = {}) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = f1, .label = std::move(label)});
    };
    // pow and the random mark.
    for (const double base : {2.0, 0.5, 0.0, -1.0, 3.0, 1e10}) add(0x101eaf10, {}, {}, "pow", base);
    // pow also reads f2: the exponent goes in through the register file (the case list sets only f1 and
    // the integer registers, so f2 is written as two words of the FPR slot, high word first or last).
    {
        constexpr unsigned f2_slot = 0x4020 / 4;
        auto& state = scenario.s;
        const auto old = state.r<8>(0x4020);
        state.wr<8>(0x4020, std::bit_cast<std::uint64_t>(1.0));
        const bool high_first = state.r<4>(0x4020) == 0x3ff00000;
        state.wr<8>(0x4020, old);
        for (const double base : {2.0, 0.5, 0.0, -1.0, 3.0, 1e10, -8.0, 7.25})
            for (const double exponent : {2.0, 0.5, 3.0, -1.0, 0.0, 1.5, 1e-3, -0.5, 10.0}) {
                const auto bits = std::bit_cast<std::uint64_t>(exponent);
                const auto high = std::uint32_t(bits >> 32), low = std::uint32_t(bits);
                add(0x101eaf10, {{f2_slot, high_first ? high : low}, {f2_slot + 1, high_first ? low : high}}, {},
                    "pow, f2 " + std::to_string(exponent), base);
            }
    }
    add(0x101ec7b0, {{3, match}}, {}, "random mark");

    const std::uint32_t kinds[] = {0x01, 0x0b, 0x0c, 0x0f, 0x11, 0x12, 0x15, 0x1e, 0x1f, 0x34, 0x36, 0x37, 0x3b, 0x3c, 0x3d, 0x5a, 0x6c, 0x6d, 0x2e, 0xff};
    // 0x101e8950: the pass receiver. Arguments: match, row, column, five out pointers; the kind is
    // the byte at sp+0x3b.
    for (const std::uint32_t kind : kinds) {
        for (const auto [row, column] : {std::pair{4u, 5u}, {0u, 0u}, {8u, 11u}, {2u, 3u}, {6u, 9u}}) {
            for (const std::uint32_t last : {0u, 1u}) {
                add(0x101e8950, {{3, match}, {4, row}, {5, column}, {6, sp + 0x90}, {7, sp + 0x91}, {8, sp + 0x92}, {9, sp + 0x93}, {10, sp + 0x94}},
                    {{1, sp + 0x3b, kind}, {1, match + 0x7924, last}, {4, sp + 0x94, 0}, {4, match + 0xe000, scenario.player(last, 2)}}, "pass receiver kind " + std::to_string(kind) + " cell " + std::to_string(row) + "," + std::to_string(column) + " side " + std::to_string(last));
            }
        }
    }
    // 0x101e8950, the inputs the cases above leave alone: the kinds 6, 7, 8, the shot state byte (0x791f), the
    // pending player's trait words, the side's tactical word, its competition and its cell.
    const auto r2 = std::uint32_t(scenario.s.r<4>(8));
    const auto toc_word = [&](std::int32_t offset) { return scenario.r32(scenario.r32(r2 + std::uint32_t(offset))); };
    const auto trait_address = [&](std::uint32_t player) {
        const auto side = std::int32_t(std::int8_t(scenario.s.memory.read<1>(player + 0x27)));
        const auto index = std::int32_t(std::int8_t(scenario.s.memory.read<1>(player + 0x19)));
        return match + 0x7c6c + std::uint32_t(side * 0x18e0 + index * 8) + 0x1888;
    };
    const auto pass_case = [&](std::uint32_t kind, unsigned row, unsigned column, unsigned last, unsigned pending_index,
                               std::vector<Write> extra, const std::string& label) {
        std::vector<Write> memory = {{1, sp + 0x3b, kind}, {1, match + 0x7924, last}, {4, sp + 0x94, 0},
                                     {4, match + 0xe000, scenario.player(last, pending_index)}};
        memory.insert(memory.end(), extra.begin(), extra.end());
        add(0x101e8950, {{3, match}, {4, row}, {5, column}, {6, sp + 0x90}, {7, sp + 0x91}, {8, sp + 0x92}, {9, sp + 0x93}, {10, sp + 0x94}},
            std::move(memory), label);
    };
    for (const std::uint32_t kind : {6u, 7u, 8u, 0x3du})
        for (const auto [row, column] : {std::pair{4u, 5u}, {0u, 0u}, {8u, 11u}, {2u, 3u}, {6u, 9u}})
            for (const unsigned last : {0u, 1u}) pass_case(kind, row, column, last, 2, {}, "pass kind " + std::to_string(kind));
    // The shot state: -1 (kind -1 only reads it then), 3 and above, 4, and the values between.
    for (const std::uint32_t kind : {0xffu, 0x1eu, 0x37u, 0x34u})
        for (const std::uint32_t state : {0u, 1u, 2u, 3u, 4u, 5u, 0x7fu})
            for (const auto [row, column] : {std::pair{4u, 5u}, {0u, 0u}, {8u, 11u}, {2u, 3u}, {6u, 9u}})
                for (const unsigned last : {0u, 1u})
                    pass_case(kind, row, column, last, 2, {{1, match + 0x791f, state}}, "pass kind " + std::to_string(kind) + " state " + std::to_string(state));
    // The pending player's trait bits (low word 1, 0x10, 8, 4, 0x40, 0x100, 0x80; high word 1 and 4), the first
    // with each of the side's tactical flags (8, 4, 2).
    for (const unsigned last : {0u, 1u}) {
        const auto pending = scenario.player(last, 2);
        for (const auto [high, low] : {std::pair{0u, 1u}, {0u, 0x10u}, {0u, 8u}, {0u, 4u}, {0u, 0x40u}, {0u, 0x100u}, {0u, 0x80u},
                                       {1u, 0u}, {4u, 0u}, {5u, 0x1ffu}, {0u, 0u}})
            for (const std::uint32_t flags : (low == 1 ? std::vector<std::uint32_t>{0, 2, 4, 8, 0xe} : std::vector<std::uint32_t>{0}))
                for (const std::uint32_t kind : {0x01u, 0x1eu, 0x36u, 0x6cu, 0x11u, 0x34u})
                    for (const auto [row, column] : {std::pair{4u, 5u}, {2u, 3u}})
                        pass_case(kind, row, column, last, 2,
                                  {{4, trait_address(pending), high}, {4, trait_address(pending) + 4, low}, {4, match + 0x7c6c + last * 0x18e0 + 0x588, flags}},
                                  "pass traits " + std::to_string(high) + "/" + std::to_string(low) + " flags " + std::to_string(flags));
    }
    // The nine competitions that scale the noise (f29), and a team without a competition.
    for (const unsigned last : {0u, 1u}) {
        const auto pending = scenario.player(last, 2);
        const auto competition = scenario.r32(scenario.r32(pending + 0x6c) + 0xc);
        for (const std::int32_t slot : {-0x7a10, -0x7a1c, -0x757c, -0x7578, -0x7574, -0x7504, -0x7514, -0x750c, -0x7af4})
            for (const std::uint32_t kind : {0x01u, 0x0bu})
                pass_case(kind, 4, 5, last, 2, {{4, competition + 0x68, toc_word(slot)}}, "pass competition " + std::to_string(slot));
        pass_case(0x01, 4, 5, last, 2, {{4, scenario.r32(pending + 0x6c) + 0xc, 0}}, "pass without competition");
    }
    // 0x10237420: a pending player on the far end line, rows 3..5, makes the search end at once.
    for (const unsigned last : {0u, 1u})
        for (unsigned index = 0; index < 20; ++index) {
            const auto pending = scenario.player(last, index);
            for (const std::uint32_t kind : {0x01u, 0x0bu})
                for (const unsigned row : {4u, 3u, 5u, 6u})
                    pass_case(kind, 4, 5, last, index, {{1, pending + 0x105, row}, {1, pending + 0x106, last == 0 ? 0xbu : 0u}},
                              "pass pending at the end line, player " + std::to_string(index));
        }

    // 0x101eaf60: the ball-cell search. Arguments: match, reference, side, kind, two words, score
    // and kind out pointers; the flag pointer is the word at sp+0x38 and a byte at sp+0x3f.
    const auto cell = [&](unsigned row, unsigned column) { return match + 0xa08 + row * 0x450 + column * 0x5c; };
    for (const std::uint32_t kind : kinds) {
        for (const std::uint32_t reference : {0u, scenario.player(0, 4), scenario.player(1, 7)}) {
            for (const std::uint32_t side : {0u, 1u}) {
                for (const std::uint32_t flag : {0u, 1u}) {
                    for (const std::uint32_t state : {0xffu, 0u, 2u, 4u}) {
                        add(0x101eaf60, {{3, match}, {4, reference}, {5, side}, {6, kind}, {7, 0}, {8, 0}, {9, sp + 0x90}, {10, sp + 0x94}},
                            {{4, sp + 0x38, sp + 0x98}, {1, sp + 0x3f, flag}, {1, match + 0x791f, state}, {1, sp + 0x98, 0},
                             {2, sp + 0x90, 0}},
                            "ball cell kind " + std::to_string(kind) + " ref " + std::to_string(reference) + " side " + std::to_string(side) + " flag " + std::to_string(flag) + " state " + std::to_string(state));
                    }
                }
            }
        }
    }
    // The 0x15 kind keeps result 0x25 when its roll draws 0: the draw after k others is forced to 0 by zeroing
    // the pool word the cursor reads then and the salt it is added to (the draw is (word + salt) mod bound).
    {
        const auto salt = scenario.r32(r2 - 0x7b90);
        const auto cursor = scenario.r32(scenario.r32(r2 - 0x7b84));
        for (const std::uint32_t reference : {0u, scenario.player(0, 4), scenario.player(1, 7)})
            for (const std::uint32_t side : {0u, 1u})
                for (unsigned k = 0; k <= 48; ++k) {
                    std::vector<Write> memory = {{4, sp + 0x38, sp + 0x98}, {1, sp + 0x3f, 0}, {1, match + 0x791f, 0}, {1, sp + 0x98, 0},
                                                 {2, sp + 0x90, 0}, {4, salt, 0}};
                    if (k == 0) for (unsigned word = 1; word <= 64; ++word) memory.push_back({2, cursor + 2 * word, 0});
                    else memory.push_back({2, cursor + 2 * k, 0});
                    add(0x101eaf60, {{3, match}, {4, reference}, {5, side}, {6, 0x15}, {7, 0}, {8, 0}, {9, sp + 0x90}, {10, sp + 0x94}}, std::move(memory),
                        "ball cell kind 0x15, draw " + std::to_string(k) + " zero, ref " + std::to_string(reference) + " side " + std::to_string(side));
                }
    }
    // The positions' value table (the halfword per position that 0x10289560 reads) set so the candidates
    // all read 1 (goalkeepers' value), or alternately 1 and 0; and the pool made to draw 0 throughout.
    {
        const auto table_writes = [&](int mode) {
            std::vector<Write> writes;
            for (unsigned side = 0; side < 2; ++side) {
                const auto table = scenario.r32(match + 0x7934 + side * 4);
                for (unsigned k = 0; k < 11; ++k) if (mode) writes.push_back({2, table + 2 * k, mode == 1 ? 1u : (k + side) % 2});
            }
            return writes;
        };
        const auto salt = scenario.r32(r2 - 0x7b90);
        const auto cursor = scenario.r32(scenario.r32(r2 - 0x7b84));
        const std::uint32_t cell_kinds[] = {0x2e, 0x25, 0x15, 0x38, 0x67, 0x0f, 0x10, 0x3b, 0x6d, 0x6c, 0x12, 0x11, 0x0c, 0x0d, 0x0e};
        for (const int mode : {0, 1, 2})
            for (const bool zero_pool : {false, true})
                for (const std::uint32_t kind : cell_kinds)
                    for (const std::uint32_t reference : {0u, scenario.player(0, 4), scenario.player(1, 7)})
                        for (const std::uint32_t side : {0u, 1u})
                            for (const std::uint32_t flag : {0u, 1u})
                                for (const std::uint32_t state : {0u, 4u}) {
                                    auto memory = table_writes(mode);
                                    memory.insert(memory.end(), {{4, sp + 0x38, sp + 0x98}, {1, sp + 0x3f, flag}, {1, match + 0x791f, state},
                                                                 {1, sp + 0x98, 0}, {2, sp + 0x90, 0}});
                                    if (zero_pool) {
                                        memory.push_back({4, salt, 0});
                                        for (unsigned word = 1; word <= 64; ++word) memory.push_back({2, cursor + 2 * word, 0});
                                    }
                                    add(0x101eaf60, {{3, match}, {4, reference}, {5, side}, {6, kind}, {7, 0}, {8, 0}, {9, sp + 0x90}, {10, sp + 0x94}},
                                        std::move(memory),
                                        "ball cell kind " + std::to_string(kind) + " table " + std::to_string(mode) + (zero_pool ? ", zero pool" : "") +
                                            " ref " + std::to_string(reference) + " side " + std::to_string(side) + " flag " + std::to_string(flag) +
                                            " state " + std::to_string(state));
                                }
        for (const int mode : {1, 2})
            for (const std::uint32_t kind : {0x01u, 0x0bu, 0x1eu, 0x1fu, 0x36u, 0x37u, 0x34u, 0x3cu, 6u, 7u, 8u, 0xffu, 0x3du})
                for (const auto [row, column] : {std::pair{4u, 5u}, {2u, 3u}, {8u, 11u}})
                    for (const unsigned last : {0u, 1u})
                        for (const std::uint32_t state : {0u, 4u}) {
                            auto memory = table_writes(mode);
                            memory.push_back({1, match + 0x791f, state});
                            pass_case(kind, row, column, last, 2, std::move(memory), "pass table " + std::to_string(mode) + " kind " + std::to_string(kind));
                        }
    }
    // Candidates of the second club (0x101e909c...): 0x103b1370 maps the side's fixture team to club records by
    // mode (the table slot +0x58 + 4 * mode of the team's row, -1 for none; most teams have no second club). The
    // pass search takes a candidate whose club is the mode-1 one down a branch of its own, so the second slot is
    // pointed at the candidates' own club (the first slot stays empty here).
    for (const unsigned last : {0u, 1u}) {
        std::uint32_t row_address = 0, second_slot = 0, second_club = 0;
        {
            auto& state = scenario.s;
            const auto saved = state.registers;
            const auto saved_pc = state.pc;
            const auto saved_budget = state.budget;
            state.budget = UINT64_MAX;
            cm::aot::guest::Cpu c(state);
            const auto flag = cm::aot::match_eng::fixture_flag_40(c, match, last);
            const auto team = cm::aot::match_eng::fixture_team(c, match, last);
            const auto list = scenario.r32(scenario.r32(r2 - 0x7a6c));
            c.set(1, (c.sp() - 0x1000) & ~0xfu);
            c.set({{3, list}, {4, team}, {5, flag}});
            c.set_lr(cm::aot::return_sentinel);
            state.pc = 0x103b1960;
            cm::aot::run_until(state, cm::aot::return_sentinel);
            const auto index = std::int32_t(c.r(3));
            state.registers = saved;
            state.pc = saved_pc;
            state.budget = saved_budget;
            if (index != -1 && team) {
                row_address = scenario.r32(list + 0x5c4) + std::uint32_t(index) * 0x94;
                // The candidates' own club, as an index of the club array.
                second_club = scenario.r32(scenario.player(last, 0) + 0x6c);
                second_slot = (second_club - scenario.r32(scenario.r32(r2 - 0x7be0))) / 0x70;
            }
        }
        if (!second_club) continue;
        for (const std::uint32_t kind : {0x01u, 0x1eu, 0x36u, 0x34u})
            pass_case(kind, 4, 5, last, 2, {{4, row_address + 0x5c, second_slot}}, "pass, candidates of the second club");
    }
    // Pending-player and candidate fields behind the rarer draws: the pending player's adjustment byte (+0x109) with the
    // other side's tactical bit 0x400 (a roll of 50), and candidates on the end rows or columns with no adjustment, with
    // the pool drawing 0 throughout (the tail's chance rolls and the 100-roll).
    for (const unsigned last : {0u, 1u})
        for (const std::uint32_t adjustment : {1u, 5u, 0u})
            for (const std::uint32_t kind : {0x01u, 0x0bu, 0x36u})
                for (const std::uint32_t flags : {0x400u, 0u}) {
                    const auto pending = scenario.player(last, 2);
                    pass_case(kind, 4, 5, last, 2, {{1, pending + 0x109, adjustment}, {4, match + 0x7c6c + (1 - last) * 0x18e0 + 0x588, flags}},
                              "pass adjustment " + std::to_string(adjustment));
                }
    {
        const auto salt = scenario.r32(r2 - 0x7b90);
        const auto cursor = scenario.r32(scenario.r32(r2 - 0x7b84));
        for (const std::uint32_t row : {0u, 8u})
            for (const std::uint32_t reference : {scenario.player(0, 4), scenario.player(1, 7)})
                for (const std::uint32_t kind : {0x12u, 0x11u, 0x6cu, 0x0cu, 0x6du, 0x2eu, 0x3bu})
                    for (const std::uint32_t side : {0u, 1u})
                        for (const std::uint32_t flag : {0u, 1u}) {
                            std::vector<Write> memory = {{4, sp + 0x38, sp + 0x98}, {1, sp + 0x3f, flag}, {1, match + 0x791f, 0}, {1, sp + 0x98, 0},
                                                         {2, sp + 0x90, 0}, {4, salt, 0}};
                            for (unsigned word = 1; word <= 64; ++word) memory.push_back({2, cursor + 2 * word, 0});
                            for (unsigned team = 0; team < 2; ++team)
                                for (unsigned index = 0; index < 20; ++index) {
                                    const auto player = scenario.player(team, index);
                                    memory.push_back({1, player + 0x105, row});
                                    memory.push_back({1, player + 0x109, 0});
                                }
                            add(0x101eaf60, {{3, match}, {4, reference}, {5, side}, {6, kind}, {7, 0}, {8, 0}, {9, sp + 0x90}, {10, sp + 0x94}},
                                std::move(memory), "ball cell, zero pool, kind " + std::to_string(kind) + " rows " + std::to_string(row));
                        }
    }
    // The tail's chance block (a reference, no special case, nobody ahead of it, a candidate on an end row): every
    // player as the reference, at either end column, with the pool drawing 0 throughout and the candidates on row 0.
    {
        const auto salt = scenario.r32(r2 - 0x7b90);
        const auto cursor = scenario.r32(scenario.r32(r2 - 0x7b84));
        for (unsigned reference_side = 0; reference_side < 2; ++reference_side)
            for (unsigned reference_index = 0; reference_index < 20; ++reference_index)
                for (const std::uint32_t column : {0u, 11u})
                    for (const std::uint32_t kind : {0x12u, 0x6cu})
                        for (const std::uint32_t side : {0u, 1u}) {
                            std::vector<Write> memory = {{4, sp + 0x38, sp + 0x98}, {1, sp + 0x3f, 0}, {1, match + 0x791f, 0}, {1, sp + 0x98, 0},
                                                         {2, sp + 0x90, 0}, {4, salt, 0}};
                            for (unsigned word = 1; word <= 64; ++word) memory.push_back({2, cursor + 2 * word, 0});
                            for (unsigned team = 0; team < 2; ++team)
                                for (unsigned index = 0; index < 20; ++index) {
                                    memory.push_back({1, scenario.player(team, index) + 0x105, 0});
                                    memory.push_back({1, scenario.player(team, index) + 0x109, 0});
                                }
                            const auto reference = scenario.player(reference_side, reference_index);
                            memory.push_back({1, reference + 0x105, 4});
                            memory.push_back({1, reference + 0x106, column});
                            // An opponent counted in the reference's cell: 0x101ee770 answers 0 at once.
                            memory.push_back({1, match + 0xa60 + 4 * 0x450 + column * 0x5c + (reference_side == 0 ? 1 : 0), 1});
                            add(0x101eaf60, {{3, match}, {4, reference}, {5, side}, {6, kind}, {7, 0}, {8, 0}, {9, sp + 0x90}, {10, sp + 0x94}},
                                std::move(memory), "ball cell, sweep, kind " + std::to_string(kind));
                        }
    }
    // The null candidate: a cell whose list says there is a player but holds a null pointer (the
    // assertion, then the call to exit); and the pending-restart players.
    for (const std::uint32_t side : {0u, 1u}) {
        const auto searched = side == 0 ? 1u : 0u;  // the side searched is the other one
        add(0x101eaf60, {{3, match}, {4, 0}, {5, side}, {6, 0x2e}, {7, 0}, {8, 0}, {9, sp + 0x90}, {10, sp + 0x94}},
            {{4, sp + 0x38, sp + 0x98}, {1, sp + 0x3f, 0}, {1, match + 0x791f, 0}, {1, match + 0x791d, 4}, {1, match + 0x791e, 5},
             {1, cell(4, 5) + 0x58 + searched, 1}, {4, cell(4, 5) + searched * 0x2c, 0}},
            "null candidate");
    }
    add(0x101eaf60, {{3, match}, {4, scenario.player(0, 3)}, {5, 0}, {6, 0x2e}, {7, 0}, {8, 0}, {9, sp + 0x90}, {10, sp + 0x94}},
        {{4, sp + 0x38, sp + 0x98}, {1, sp + 0x3f, 0}, {1, match + 0x791f, 0xff}, {4, match + 0xe01c, scenario.player(0, 3)}},
        "shot state -1, reference is the taker");
    add(0x101eaf60, {{3, match}, {4, scenario.player(1, 3)}, {5, 0}, {6, 0x2e}, {7, 0}, {8, 0}, {9, sp + 0x90}, {10, sp + 0x94}},
        {{4, sp + 0x38, sp + 0x98}, {1, sp + 0x3f, 0}, {1, match + 0x791f, 0xff}, {4, match + 0xe020, scenario.player(1, 3)}},
        "shot state -1, second taker");
}
