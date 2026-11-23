// graph_carbon.cpp, block `remap` (0x10769460, the screen fade): cases. The routine captures a rectangle of the snapshot's real surface
// (0x1076b020, the real allocator), builds the 64K-entry remap table, and per frame writes the surface and calls the Toolbox (GetPort,
// GetWindowPort, SetPort, SetRect, GetPortBitMapForCopyBits, CopyBits, QDFlushPortBuffer): a snapshot has no host behind them, so each is
// scripted by name on both sides (Case::imports; the checker logs every import call's r3-r10, link register and r13-r31 into memory the
// comparison sees), and the glue's TOC save slot in the frame (sp - 0x90 + 0x14) is poked, which the original reloads r2 from.
// Real arguments: the fades the game starts (0x10131b3c.. in the intro/transition screens, 0x107192d4): the whole 800x600 surface and the
// steps 0x46, -0x32 and 0x14. The rest vary the rectangle (every order of the corners, one pixel, partly or wholly outside the surface),
// the step count (0, +-1.., 70, -50, INT_MIN), the surface's pixels (poked: all 65,536 values of the table, a uniform area, two values
// alternating, a gradient) and the failures: calloc, the capture, and the n-th allocation of a table entry. Labels start with "remap:".
#include "graph_carbon_checks.hpp"
#include "graph_carbon_detail.hpp"
#include <array>
#include <climits>
#include <cstdlib>
#include <random>

namespace cm::aot::graph_carbon_checks {
namespace {
constexpr std::uint32_t game_toc = 0x108bba90, fade_entry = 0x10769460;
constexpr std::int32_t surface_base_slot = -0x3564, row_stride_slot = -0x3568;
constexpr std::uint32_t frame_size = 0x90;
constexpr std::uint32_t saved_port = 0x38;
constexpr std::uint32_t calloc_entry = 0x10775200, capture_entry = 0x1076b020, allocate_entry = 0x10775130;

std::vector<std::pair<unsigned, std::uint32_t>> arguments(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::int32_t steps) {
    return {{3, std::uint32_t(x1)}, {4, std::uint32_t(y1)}, {5, std::uint32_t(x2)}, {6, std::uint32_t(y2)}, {7, std::uint32_t(steps)},
            {8, 0x7a000008u}, {9, 0x7a000009u}, {10, 0x7a00000au}};
}
}

GRAPH_CARBON_CASES("remap") {
    const auto frame = scenario.sp() - frame_size;
    const auto surface = scenario.r32(game_toc + std::uint32_t(surface_base_slot));
    const auto stride = scenario.u16(game_toc + std::uint32_t(row_stride_slot));
    const auto pixel_at = [&](std::uint32_t x, std::uint32_t y) { return surface + (y * stride + x) * 2; };
    // The Toolbox's answers, the glue's TOC slot, and what GetPort writes through its pointer (the port saved in the frame).
    const auto toolbox = [&] {
        return std::vector<Case::Import>{
            {"CarbonLib::GetPort", 0, 0, {w32(frame + saved_port, 0x6a000100)}},
            {"CarbonLib::GetWindowPort", 0, 0x6b000200},
            {"CarbonLib::SetPort", 0, 0},
            {"CarbonLib::SetRect", 0, 0},
            {"CarbonLib::GetPortBitMapForCopyBits", 0, 0x6c000300},
            {"CarbonLib::CopyBits", 0, 0},
            {"CarbonLib::QDFlushPortBuffer", 0, 0}};
    };
    const auto fade = [&](const std::string& what, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::int32_t steps,
                          std::vector<Write> memory = {}) {
        memory.push_back(w32(frame + 0x14, game_toc));
        Case item{.entry = fade_entry, .registers = arguments(x1, y1, x2, y2, steps), .memory = std::move(memory), .label = "remap: " + what};
        item.imports = toolbox();
        return item;
    };
    if (!surface || !stride) return;
    if (std::getenv("CM_GRAPH_CARBON_SHOW_STATE"))
        std::fprintf(stderr, "remap: surface %x stride %u (pixels), window slot %x, off-screen port %x\n", surface, stride,
                     scenario.toc(-0x5af8), scenario.r32(game_toc - 0x355c));

    // The pixels a case pokes into the surface: a row-major list from (x, y) over `width` columns.
    const auto paint = [&](std::uint32_t x, std::uint32_t y, std::uint32_t width, const std::vector<std::uint32_t>& pixels) {
        std::vector<Write> writes;
        for (std::size_t n = 0; n < pixels.size(); ++n) writes.push_back(w16(pixel_at(x + std::uint32_t(n % width), y + std::uint32_t(n / width)), pixels[n]));
        return writes;
    };
    // ---- Rectangles of the real surface with the real step counts and with small ones; each corner order, in a small rectangle (the capture
    // is of the rectangle the corners span). The first cases carry the call trace (the first 32 of an entry), so one per call site first.
    cases.push_back(fade("small up", 20, 10, 27, 15, 2));
    cases.push_back(fade("small down", 20, 10, 27, 15, -2));
    cases.push_back(fade("steps one", 30, 30, 33, 32, 1));
    cases.push_back(fade("steps minus one", 30, 30, 33, 32, -1));
    cases.push_back(fade("steps zero", 20, 10, 27, 15, 0));
    cases.push_back(fade("steps INT_MIN", 20, 10, 27, 15, INT_MIN));
    for (const std::int32_t steps : {3, -3, 5, 7, -9, 20, 70, -50}) cases.push_back(fade("small steps " + std::to_string(steps), 100, 60, 111, 69, steps));
    // The corners in every order and degenerate (one pixel, one row, one column).
    for (const auto& corners : std::vector<std::array<std::int32_t, 4>>{{40, 20, 49, 25}, {49, 25, 40, 20}, {49, 20, 40, 25}, {40, 25, 49, 20},
                                                                         {40, 20, 40, 20}, {40, 20, 55, 20}, {40, 20, 40, 30}, {0, 0, 0, 0}, {799, 599, 799, 599}})
        for (const std::int32_t steps : {4, -4})
            cases.push_back(fade("corners " + std::to_string(corners[0]) + "," + std::to_string(corners[1]) + " " + std::to_string(corners[2]) + "," + std::to_string(corners[3]) +
                                     " steps " + std::to_string(steps), corners[0], corners[1], corners[2], corners[3], steps));
    // Partly or wholly outside the surface (the capture clips as the original does; what it reads or writes outside is game memory).
    for (const auto& corners : std::vector<std::array<std::int32_t, 4>>{{-5, -5, 5, 5}, {790, 590, 809, 609}, {-40, 100, -20, 120}, {900, 700, 920, 720}, {0, 580, 30, 640}})
        cases.push_back(fade("outside " + std::to_string(corners[0]) + "," + std::to_string(corners[1]) + " " + std::to_string(corners[2]) + "," + std::to_string(corners[3]),
                             corners[0], corners[1], corners[2], corners[3], 3));

    // ---- The pixels of the table: every 16-bit value once (a 256 x 256 rectangle), a uniform area, two values alternating, and a gradient.
    std::vector<std::uint32_t> every_value(65536);
    for (std::uint32_t v = 0; v < 65536; ++v) every_value[v] = (v * 0x9e37u + 0x1234u) & 0xffff;  // a permutation of 0..65535 (0x9e37 is odd)
    cases.push_back(fade("every pixel value, steps 2", 0, 0, 255, 255, 2, paint(0, 0, 256, every_value)));
    cases.push_back(fade("every pixel value, steps -1", 0, 0, 255, 255, -1, paint(0, 0, 256, every_value)));
    cases.push_back(fade("uniform area", 60, 40, 99, 59, 6, paint(60, 40, 40, std::vector<std::uint32_t>(40 * 20, 0x4a52))));
    {
        std::vector<std::uint32_t> pattern(16 * 8);
        for (std::size_t n = 0; n < pattern.size(); ++n) pattern[n] = (n & 1) ? 0x7fff : 0x0421;
        cases.push_back(fade("two values", 60, 40, 75, 47, 5, paint(60, 40, 16, pattern)));
        cases.push_back(fade("two values down", 60, 40, 75, 47, -5, paint(60, 40, 16, pattern)));
    }
    {
        std::vector<std::uint32_t> gradient(32 * 4);
        for (std::size_t n = 0; n < gradient.size(); ++n) gradient[n] = ((n & 31) << 10) | (((n * 7) & 31) << 5) | ((n >> 2) & 31) | ((n & 64) ? 0x8000 : 0);
        cases.push_back(fade("gradient", 200, 100, 231, 103, 9, paint(200, 100, 32, gradient)));
        cases.push_back(fade("gradient down, wide steps", 200, 100, 231, 103, -70, paint(200, 100, 32, gradient)));
    }
    // The extreme pixel values: black, white, one component saturated (the leaf's clamps), bit 15 set.
    {
        const std::vector<std::uint32_t> extremes{0, 0x7fff, 0x7c00, 0x03e0, 0x001f, 0x8000, 0xffff, 0x8421, 0x7bde, 0x0001, 0x4000, 0x0400, 0x0020};
        cases.push_back(fade("extreme values", 10, 200, 22, 200, 4, paint(10, 200, 13, extremes)));
        cases.push_back(fade("extreme values down", 10, 200, 22, 200, -4, paint(10, 200, 13, extremes)));
    }

    // ---- Failures. calloc: the table is never made (nothing to free, no capture). The capture: the table is freed (65,536 empty entries scanned).
    // The n-th allocation of a table entry (counting the capture's own, if it goes through the dispatcher): the entries made so far are freed.
    {
        auto item = fade("calloc fails", 20, 10, 27, 15, 3);
        item.answers = {{calloc_entry, 0}};
        item.original_owners = {calloc_entry};
        cases.push_back(item);
        item = fade("capture fails", 20, 10, 27, 15, 3);
        item.answers = {{capture_entry, 0}};
        cases.push_back(item);
        for (unsigned nth = 1; nth <= 6; ++nth) {
            item = fade("allocation " + std::to_string(nth) + " fails", 100, 60, 111, 69, nth % 2 ? 3 : -3);
            item.nth_answers = {{allocate_entry, nth, 0}};
            item.original_owners = {allocate_entry};
            cases.push_back(item);
        }
        item = fade("allocation 4 fails, many values", 0, 0, 255, 255, 1, paint(0, 0, 256, every_value));
        item.nth_answers = {{allocate_entry, 40000, 0}};
        item.original_owners = {allocate_entry};
        cases.push_back(item);
    }

    // ---- The real fades: the whole surface with the three step counts the game uses (heavy: 480,000 pixels a frame).
    cases.push_back(fade("real: screen, steps 20", 0, 0, 799, 599, 20));
    cases.push_back(fade("real: screen, steps -50", 0, 0, 799, 599, -50));
    cases.push_back(fade("real: screen, steps 70", 0, 0, 799, 599, 70));
}
}
