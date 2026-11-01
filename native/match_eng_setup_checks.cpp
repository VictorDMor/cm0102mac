// Direct calls of the setup block from a real match: argument variants, the
// early returns, the statistics fixture (id -2), a club that cannot field eleven
// players (the assertion report) and the draws with unusual limits.
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("setup") {
    const auto match = scenario.match;
    const auto fixture = scenario.r32(match + 0x3118);
    const auto home = scenario.r32(fixture + 0x1c), away = scenario.r32(fixture + 0x20);
    const auto target = scenario.r32(match + 0x79f4);
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers,
                         std::vector<Write> memory, std::string label) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {},
                         .label = std::move(label)});
    };
    const auto setup = [&](std::uint32_t inactive, std::uint32_t message, std::uint32_t silent, std::vector<Write> memory,
                           std::string label) {
        add(0x101d1760, {{3, match}, {4, fixture}, {5, inactive}, {6, message}, {7, silent}}, std::move(memory), std::move(label));
    };
    for (const std::uint32_t inactive : {0u, 1u}) for (const std::uint32_t silent : {0u, 1u}) {
        setup(inactive, 0, silent, {}, "new message object");
        if (target) setup(inactive, target, silent, {}, "given message object");
    }
    setup(0, 0, 0, {{1, match + 0x7930, 1}}, "played elsewhere");
    setup(0, 0, 0, {{4, fixture, 0xfffffffe}}, "statistics fixture");
    setup(0, 0, 0, {{4, fixture, 0xffffffff}}, "friendly id");
    setup(0, 0, 0, {{4, fixture, 7}}, "other id");
    for (const std::uint32_t options : {0x00u, 0xffu, 0x9au, 0xa9u}) setup(0, 0, 0, {{1, fixture + 0x4e, options}}, "option nibbles");
    for (const std::uint32_t mode : {0x00u, 0x04u, 0x08u, 0x0cu}) setup(0, 0, 0, {{1, fixture + 0x4f, mode}}, "mode bits");
    setup(0, 0, 0, {{1, fixture + 0x40, 0}}, "no first flag");
    setup(0, 0, 0, {{4, fixture + 0x14, 0}}, "no competition record");
    // Clubs that cannot field eleven eligible players (every squad entry cleared).
    for (const auto club : {home, away}) {
        std::vector<Write> memory;
        for (std::uint32_t at = 0; at < 0x40; at += 4) memory.push_back({4, scenario.r32(club + 0x90) + at, 0});
        memory.push_back({4, club + 0x90, 0});
        setup(0, 0, 0, memory, "club without players");
        setup(0, 0, 1, memory, "club without players, silent");
    }
    // The computer can't pick eleven.
    cases.push_back({.entry = 0x101d1760, .registers = {{3, match}, {4, fixture}, {5, 0}, {6, 0}, {7, 0}}, .memory = {}, .f1 = {},
                     .label = "ten picked", .answers = {{0x102065f0, 10}}});
    cases.push_back({.entry = 0x101d1760, .registers = {{3, match}, {4, fixture}, {5, 0}, {6, 0}, {7, 1}}, .memory = {}, .f1 = {},
                     .label = "ten picked, silent", .answers = {{0x102065f0, 10}}});
    // The draws: zero, small, large and negative limits.
    for (const std::uint32_t limit : {0u, 1u, 2u, 7u, 100u, 0xffffu, 0x10000u, 0x12345u, 1000000u, 0xffffffffu, 0xffff0001u,
                                      0xfffefffeu, 0x80000000u, 0x7fffffffu})
        add(0x101d3c90, {{3, limit}}, {}, "draw");
    for (const std::uint32_t side : {0u, 1u, 2u, 0xffu}) add(0x101d3b60, {{3, match}, {4, side}}, {}, "side player");
    // The destructor needs a match it may take apart: null, and the flag for freeing.
    add(0x101d3e50, {{3, 0}, {4, 1}}, {}, "null match");
    add(0x101d3e50, {{3, match}, {4, 0}}, {}, "destroy");
    add(0x101d3e50, {{3, match}, {4, 1}}, {}, "destroy and free");
    add(0x101d3e50, {{3, match}, {4, 0}}, {{1, match + 0x7913, 0}}, "no owned target");
    add(0x101d16a0, {{3, match}}, {}, "construct");
}
