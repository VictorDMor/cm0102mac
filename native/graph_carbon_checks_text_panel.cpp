// graph_carbon.cpp, block `text` (the message panel): cases for 0x1076e5f0. Its callees are answered (the event pump 0x1076fe80, the club window's
// redraw 0x10718f60, the shaded frame 0x1076d6e0, the text box 0x1076e160, the bar 0x1076a580, the flush 0x107692f0: the leavers among them are
// recorded at their answers), so each case compares what the panel does itself: the arguments of every call (the call trace), the frame, the
// registers it leaves and the stack bytes it writes. Labels start with "panel:".
#include "graph_carbon_checks.hpp"
#include "graph_carbon_text_detail.hpp"
#include <string>

namespace cm::aot::graph_carbon_checks {
namespace {
using namespace cm::aot::graph_carbon;
constexpr std::uint32_t panel_entry = 0x1076e5f0, text_at = scratch_base + 0x20000;
constexpr std::uint32_t minus_one = 0xffffffffu;
std::vector<std::pair<unsigned, std::uint32_t>> registers(std::initializer_list<std::pair<unsigned, std::uint32_t>> used) {
    std::vector<std::pair<unsigned, std::uint32_t>> all;
    for (unsigned n = 3; n <= 10; ++n) all.emplace_back(n, 0x7a000000u + n * 0x01010101u);
    for (const auto& [n, value] : used) all[n - 3].second = value;
    return all;
}
}

GRAPH_CARBON_CASES("text") {
    const auto answered = std::vector<std::pair<std::uint32_t, std::uint32_t>>{{0x1076fe80, 0x11}, {0x10718f60, 0x22}, {0x1076d6e0, 0x33}, {0x1076e160, 0x44}, {0x1076a580, 0x55}, {0x107692f0, 0x66}};
    std::vector<Write> memory;
    for (unsigned n = 0; n < 12; ++n) memory.push_back(w8(text_at + n, 'A' + n));
    memory.push_back(w8(text_at + 12, 0));
    // Rare paths first: no text (the assertion), the graphics not initialised.
    cases.push_back({.entry = panel_entry, .registers = registers({{3, 0}, {4, 1}, {5, 2}}), .answers = answered, .label = "panel: no text (assertion)"});
    {
        Case item{.entry = panel_entry, .registers = registers({{3, text_at}, {4, 1}, {5, 2}}), .memory = memory, .answers = answered, .label = "panel: not initialised"};
        item.memory.push_back(w8(0x108bba90 - 0x3550, 1));
        cases.push_back(item);
    }
    for (const std::uint32_t text : {text_at, 0u})
        for (const std::uint32_t step : {0u, 1u, 2u, 5u, 100u, minus_one, 0x80000000u, 0x7fffffffu})
            for (const std::uint32_t total : {0u, minus_one, 1u, 2u, 3u, 100u, 0x7fffffffu, 0x80000000u}) {
                if (text == 0 && (step > 1 || total > 1)) continue;
                cases.push_back({.entry = panel_entry, .registers = registers({{3, text}, {4, step}, {5, total}}), .memory = memory, .answers = answered,
                                 .label = "panel: text " + hex(text) + " step " + hex(step) + " total " + hex(total)});
            }
}
}
