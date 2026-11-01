// Cases for the shots block: the flag halfword each position of a side's table holds (what picks the
// weights and cases), the attribute bytes of the players' records, and the fixture's ratings and
// competition (the post-loop scaling). A live match reaches only the nibble routine.
#include "guest_abi.hpp"
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("shots") {
    const auto match = scenario.match;
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, std::string label) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label)});
    };
    const auto add_answered = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, std::string label,
                                  std::vector<std::pair<std::uint32_t, std::uint32_t>> answers) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label), .answers = std::move(answers)});
    };
    const auto toc = [&](std::int32_t offset) { return scenario.r32(scenario.s.r<4>(cm::aot::guest::reg::gpr(2)) + std::uint32_t(offset)); };
    // The per-position flag table of a side (0x7934 + side*4), one halfword per position.
    const auto flag_writes = [&](unsigned side, std::uint32_t flags) {
        std::vector<Write> writes;
        const auto table = scenario.r32(match + 0x7934 + side * 4);
        for (std::uint32_t position = 0; position <= 10; ++position) writes.push_back({2, table + position * 2, flags});
        return writes;
    };
    // The attribute bytes of every player's record, one value for all and a halfword stat.
    constexpr std::uint32_t byte_offsets[] = {0x1b, 0x1e, 0x21, 0x26, 0x2b, 0x2e, 0x2f, 0x31, 0x32, 0x33, 0x35, 0x36, 0x37, 0x39, 0x3c, 0x3e, 0x3f, 0x40, 0x43, 0x44};
    const auto attribute_writes = [&](unsigned side, std::uint32_t value, std::uint32_t stat) {
        std::vector<Write> writes;
        for (unsigned index = 0; index < 20; ++index) {
            const auto record = scenario.r32(scenario.player(side, index) + 0x70);
            if (!record) continue;
            for (const auto offset : byte_offsets) writes.push_back({1, record + offset, value});
            writes.push_back({2, record + 4, stat});
        }
        return writes;
    };
    const auto join = [](std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; };
    const std::uint32_t flag_values[] = {0, 1, 2, 4, 8, 0x10, 0x20, 0x40, 0x41, 0x42, 0x44, 0x48, 0x50, 0x60, 0x80, 0x100, 0x7e, 0x3e, 0x1e, 0x0e, 0x06,
                                         0x48 | 0x80, 0x48 | 0x800, 0x50 | 0x80, 0x50 | 0x800, 0x40 | 0x100, 0x40 | 0x200, 0x40 | 0x400, 0x8000};

    // The choosers over every flag pattern and both sides.
    for (const unsigned side : {0u, 1u}) {
        for (const auto flags : flag_values) {
            const auto flagged = flag_writes(side, flags);
            for (const std::uint32_t reduced : {0u, 1u}) add(0x101e1640, {{3, match}, {4, side}, {5, reduced}}, flagged, "weighted");
            for (const std::uint32_t mode : {0u, 2u}) add(0x101e1a50, {{3, match}, {4, side}, {5, mode}}, flagged, "by mode");
            for (const std::uint32_t mode : {0u, 1u}) add(0x101e1bd0, {{3, match}, {4, side}, {5, mode}}, flagged, "ranked");
            add(0x101e2100, {{3, match}, {4, side}}, flagged, "simple");
            add(0x101e22e0, {{3, match}, {4, side}}, flagged, "traits");
        }
        // Attribute patterns: single bytes around 5, 6, 10, 15 and sums around 25, 35, 50, 65, 75.
        for (const std::uint32_t value : {0u, 2u, 5u, 6u, 7u, 9u, 10u, 14u, 15u, 16u, 17u, 20u, 0x80u, 0xffu}) {
            for (const std::uint32_t stat : {0x10u, 0x60u, 0x100u}) {
                for (const std::uint32_t flags : {0u, 0x40u, 0x42u, 0x50u | 0x80u}) {
                    const auto memory = join(flag_writes(side, flags), attribute_writes(side, value, stat));
                    add(0x101e1640, {{3, match}, {4, side}, {5, 0}}, memory, "weighted attributes");
                    add(0x101e1640, {{3, match}, {4, side}, {5, 1}}, memory, "weighted attributes");
                    add(0x101e1bd0, {{3, match}, {4, side}, {5, 0}}, memory, "ranked attributes");
                    add(0x101e1bd0, {{3, match}, {4, side}, {5, 5}}, memory, "ranked attributes");
                    add(0x101e22e0, {{3, match}, {4, side}}, memory, "traits attributes");
                }
            }
        }
        // The tendencies over every flag pattern.
        for (const auto flags : flag_values) add(0x101df110, {{3, match}, {4, side}}, flag_writes(side, flags), "tendencies flags");
        for (const std::uint32_t value : {0u, 2u, 5u, 6u, 7u, 9u, 10u, 14u, 15u, 16u, 17u, 20u, 0x80u}) {
            for (const std::uint32_t stat : {0x10u, 0x60u, 0x100u}) {
                for (const std::uint32_t flags : {0u, 0x40u, 0x42u, 0x4u, 0x8u, 0x20u}) {
                    add(0x101df110, {{3, match}, {4, side}}, join(flag_writes(side, flags), attribute_writes(side, value, stat)), "tendencies attributes");
                }
            }
        }
    }

    // The fixture's competition and the teams' ratings (the post-loop scaling).
    const auto fixture = scenario.r32(match + 0x3118);
    if (fixture) {
        const auto team_one = scenario.r32(fixture + 0x1c), team_two = scenario.r32(fixture + 0x20);
        const auto reference = scenario.r32(fixture + 0x14);
        const std::int32_t pool_ids[] = {-0x7af4, -0x7a1c, -0x7510, -0x7514, -0x7504, -0x7a10, -0x7a28, -0x750c, -0x79fc, -0x7a00};
        const auto competition_record = team_one ? scenario.r32(team_one + 0x2c) : 0;
        for (const unsigned side : {0u, 1u}) {
            if (competition_record) {
                for (const auto at : pool_ids) {
                    const auto id = scenario.r32(toc(at));
                    for (const bool handle_empty : {false, true}) {
                        std::vector<Write> memory = {{4, competition_record + 0x68, id}};
                        if (reference) memory.push_back({4, reference + 8, competition_record});
                        if (handle_empty) memory.push_back({4, toc(-0x7b54), 0});
                        add(0x101df110, {{3, match}, {4, side}}, memory, "competition");
                    }
                }
                add(0x101df110, {{3, match}, {4, side}}, {{4, competition_record + 0x68, 12345}}, "other competition");
                // With the fixture's reference pointing at the team's competition, the gate lets the scaling through.
                if (reference) {
                    add(0x101df110, {{3, match}, {4, side}}, {{4, competition_record + 0x68, 12345}, {4, reference + 8, competition_record}}, "other competition, gated");
                    for (const std::uint32_t verdict : {0u, 1u})
                        add_answered(0x101df110, {{3, match}, {4, side}}, {{4, reference + 8, competition_record}}, "reference verdict", {{0x10494d20, verdict}});
                }
            }
            for (const std::int32_t gap : {1500, 1000, 800, 300, 0, -100, -300, -800, -2000}) {
                for (const std::int32_t base : {1000, 3000}) {
                    for (const std::uint32_t flag_a : {0u, 1u}) {
                        for (const std::uint32_t flag_b : {0u, 1u}) {
                            std::vector<Write> memory = {{1, match + 0xe03d, flag_a}, {1, match + 0xe073, flag_b}};
                            if (team_one) memory.push_back({2, team_one + 0x1ec, std::uint32_t(base + gap) & 0xffff});
                            if (team_two) memory.push_back({2, team_two + 0x1ec, std::uint32_t(base) & 0xffff});
                            add(0x101df110, {{3, match}, {4, side}}, memory, "ratings");
                        }
                    }
                }
            }
            // Missing records end the scaling early.
            add(0x101df110, {{3, match}, {4, side}}, {{4, match + 0x3118, 0}}, "no fixture");
            if (team_one) add(0x101df110, {{3, match}, {4, side}}, {{4, fixture + 0x1c, 0}}, "no first team");
            if (team_two) add(0x101df110, {{3, match}, {4, side}}, {{4, fixture + 0x20, 0}}, "no second team");
            if (reference) add(0x101df110, {{3, match}, {4, side}}, {{4, fixture + 0x14, 0}}, "no reference");
            if (reference) add(0x101df110, {{3, match}, {4, side}}, {{4, reference + 8, 0}}, "reference differs");
            if (competition_record) add(0x101df110, {{3, match}, {4, side}}, {{4, team_one + 0x2c, 0}}, "no competition");
        }
        // A player without a person record: the assert macro.
        add(0x101df110, {{3, match}, {4, 0}}, {{4, scenario.player(0, 3) + 0x6c, 0}}, "assertion");
    }

    // The role nibbles: every match mode and side, over real players.
    for (const unsigned mode : {0u, 1u, 2u, 7u}) {
        for (const unsigned side : {0u, 1u}) {
            for (const unsigned index : {0u, 4u, 9u}) {
                const auto player = scenario.player(side, index);
                add(0x101e25a0, {{3, match}, {4, player}, {5, match + 0x20}, {6, match + 0x24}}, {{1, match + 0x7924, mode}}, "nibbles");
                add(0x101e25a0, {{3, match}, {4, player}, {5, match + 0x20}, {6, match + 0x24}}, {{1, match + 0x7924, mode}, {1, player + 0x27, 1 - side}}, "nibbles other side");
            }
        }
    }
    // A side byte of 2 (the second-side branch tests the byte): the "players" it reads are out of play.
    {
        std::vector<Write> out;
        for (unsigned index = 0; index < 20; ++index) out.push_back({1, scenario.player(2, index) + 0x19, 0xff});
        add(0x101df110, {{3, match}, {4, 2}}, out, "tendencies other side");
    }
}
