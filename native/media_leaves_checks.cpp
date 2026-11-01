// The leaves over scratch rows and every path of the
// predicates; the lists' real rows come through the mutated calls.
#include "media_checks.hpp"

using namespace cm::aot::media_checks;

MEDIA_CASES("leaves") {
    constexpr std::uint32_t person_at = 0x40, rows_at = 0x100;
    const auto base = scratch_base;
    const auto person = base + person_at, list = base, table = scenario.toc(-0x4218);
    const std::vector<Write> rows{{4, list, base + rows_at}, {4, list + 8, 3}, {4, person, 7}, {4, person + 0x24, 1},
        {4, base + rows_at + 0x18 * 0 + 8, 5}, {1, base + rows_at + 0x18 * 0 + 0x10, 2}, {2, base + rows_at + 0x18 * 0 + 0x12, 9},
        {4, base + rows_at + 0x18 * 1 + 8, 7}, {1, base + rows_at + 0x18 * 1 + 0x10, 1}, {2, base + rows_at + 0x18 * 1 + 0x12, 9},
        {4, base + rows_at + 0x18 * 2 + 8, 7}, {1, base + rows_at + 0x18 * 2 + 0x10, 2},
        {2, base + rows_at + 0x18 * 2 + 0x12, std::uint32_t(std::int8_t(scenario.s.memory.read<1>(table)))}};
    auto with = [&](std::vector<Write> more) { auto all = rows; all.insert(all.end(), more.begin(), more.end()); return all; };
    const std::vector<std::pair<std::uint32_t, std::vector<Write>>> variants{
        {7, rows}, {5, with({{4, person, 5}})}, {99, with({{4, person, 99}})},
        {0, with({{4, list + 8, 0}})}, {1, with({{4, person + 0x24, 0}})}, {2, with({{2, base + rows_at + 0x18 * 2 + 0x12, 0x7fff}})}};
    for (const auto& [mark, memory] : variants)
        for (const auto entry : {0x10658370u, 0x106583d0u, 0x10658480u})
            cases.push_back({.entry = entry, .registers = {{3, list}, {4, person}, {5, 0x51515151}, {6, 0x52525252}, {7, 0x53535353}, {8, 0x54545454}, {9, 0x55555555}}, .memory = memory, .f1 = {}, .label = "list " + std::to_string(mark)});
    cases.push_back({.entry = 0x106583d0, .registers = {{3, list}, {4, 0}}, .memory = rows, .f1 = {}, .label = "null person"});
    for (const std::uint32_t grade : {0u, 59u, 60u, 68u, 69u, 89u, 90u, 99u, 100u, 0xffffffffu, 0x80000000u, 0x7fffffffu})
        cases.push_back({.entry = 0x106727c0, .registers = {{3, 1}, {4, grade}}, .memory = {}, .f1 = {}, .label = {}});
    for (std::uint32_t story = 0; story < 100; ++story) for (const auto entry : {0x1069ec90u, 0x1069ecf0u})
        cases.push_back({.entry = entry, .registers = {{3, 1}, {4, story}}, .memory = {}, .f1 = {}, .label = {}});
    for (const auto entry : {0x1069ec90u, 0x1069ecf0u})
        cases.push_back({.entry = entry, .registers = {{4, 0xffffffffu}}, .memory = {}, .f1 = {}, .label = {}});
    cases.push_back({.entry = 0x1067d030, .registers = {{3, 0x12345}}, .memory = {}, .f1 = {}, .label = {}});
    const auto left = base + 0x400, right = base + 0x420;
    const auto pair = [&](std::uint32_t a0, std::uint32_t a4, std::uint32_t b0, std::uint32_t b4) {
        return std::vector<Write>{{4, left, a0}, {4, left + 4, a4}, {4, right, b0}, {4, right + 4, b4}};
    };
    for (const auto& memory : {pair(1, 2, 1, 2), pair(1, 2, 3, 2), pair(3, 2, 1, 2), pair(1, 0xffffffffu, 1, 2), pair(1, 2, 1, 0xffffffffu), pair(1, 2, 1, 9), pair(1, 9, 1, 2), pair(0x80000000u, 1, 1, 1)}) {
        cases.push_back({.entry = 0x1069e890, .registers = {{3, left}, {4, right}}, .memory = memory, .f1 = {}, .label = {}});
    }
    cases.push_back({.entry = 0x1069e890, .registers = {{3, 0}, {4, right}}, .memory = {}, .f1 = {}, .label = {}});
    cases.push_back({.entry = 0x1069e890, .registers = {{3, left}, {4, 0}}, .memory = {}, .f1 = {}, .label = {}});
    const auto row = [&](std::uint32_t a8, std::uint32_t ac, std::uint32_t a14, std::uint32_t a12, std::uint32_t b8, std::uint32_t bc, std::uint32_t b14, std::uint32_t b12) {
        return std::vector<Write>{{4, left + 8, a8}, {4, left + 0xc, ac}, {4, left + 0x14, a14}, {2, left + 0x12, a12},
                                  {4, right + 8, b8}, {4, right + 0xc, bc}, {4, right + 0x14, b14}, {2, right + 0x12, b12}};
    };
    for (const auto& memory : {row(1,1,1,1,1,1,1,1), row(2,1,1,1,1,1,1,1), row(1,1,1,1,2,1,1,1), row(1,2,1,1,1,1,1,1), row(1,1,1,1,1,2,1,1),
                               row(1,1,2,1,1,1,1,1), row(1,1,1,1,1,1,2,1), row(1,1,1,2,1,1,1,1), row(1,1,1,1,1,1,1,2), row(1,1,1,0xffff,1,1,1,1)})
        cases.push_back({.entry = 0x1069e900, .registers = {{3, left}, {4, right}}, .memory = memory, .f1 = {}, .label = {}});
    cases.push_back({.entry = 0x1069e900, .registers = {{3, 0}, {4, right}}, .memory = {}, .f1 = {}, .label = {}});
    cases.push_back({.entry = 0x1069e900, .registers = {{3, left}, {4, 0}}, .memory = {}, .f1 = {}, .label = {}});
}
