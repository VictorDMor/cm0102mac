// graph_carbon.cpp, block `colours` (0x1076b830, 0x1076b6e0, 0x1076b7e0, 0x1076b020, 0x1076b140, 0x1076b240, 0x1076b2f0, 0x1076b450): cases.
//
// A screen-side block: what the checker compares is the surface and bitmap bytes, the heap (the allocator runs for real on both sides),
// the registers r3-r10 in full, the dead stack (graph_carbon_stack_exact.inc lists all eight entries), the call trace (callee, return
// address, held registers, arguments) and the logs of the Toolbox imports and of the native drawing observation. The surface of a case is a
// scratch area the case points the small data at (base, both strides, the display flag): the pixel at (x, y) holds 0x4000 + y * 0x100 + x,
// so a copy from or to a wrong address, row or width shows in the bytes. Files are not opened: the file layer is answered per call (fopen
// and fclose through the dispatcher, fread and fwrite as original owners answered by index), the data of a read is poked into the buffers the
// routine reads (its stack record, or the pixel block a reused record gives), and the bytes of a write are captured from the buffers it hands
// fwrite. Real heap blocks for the cases that free or clone are built in `prepare` by running the original allocator on each side.
// Labels start with "colours:".
#include "graph_carbon_checks.hpp"
#include "graph_carbon_adapter.hpp"
#include "graph_carbon_colours_detail.hpp"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>
#include <tuple>

namespace cm::aot::graph_carbon_checks {
namespace {
using namespace cm::aot::graph_carbon;
constexpr std::uint32_t game_toc = 0x108bba90;
constexpr std::uint32_t colours_entry = 0x1076b830, clone_entry = 0x1076b6e0, free_entry = 0x1076b7e0, capture_entry = 0x1076b020,
                        blit_entry = 0x1076b140, save_entry = 0x1076b240, load_entry = 0x1076b2f0, read_entry = 0x1076b450;
// The rest of the program the cases script.
constexpr std::uint32_t allocate = 0x10775130, release = 0x10775170, open_file = 0x10739de0, read_file = 0x10775fe0, write_file = 0x10776360,
                        close_file = 0x107766f0;
constexpr std::uint32_t surface_at = scratch_base + 0x20000, source_at = scratch_base + 0x30000, reuse_at = scratch_base + 0x30100,
                        text_at = scratch_base + 0x31000, stream_at = scratch_base + 0x38000, reuse_pixels = scratch_base + 0x40000,
                        capture_a = scratch_base + 0x70000, capture_b = scratch_base + 0x71000;
constexpr std::uint32_t small(std::int32_t offset) { return game_toc + std::uint32_t(offset); }
constexpr std::uint32_t swap32(std::uint32_t v) { return (v << 24) | ((v & 0xff00) << 8) | ((v >> 8) & 0xff00) | (v >> 24); }
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
// r3-r10 distinct wherever a routine does not use them, so that the full comparison sees a register that should have been kept.
Registers others(std::initializer_list<std::pair<unsigned, std::uint32_t>> used) {
    Registers registers;
    for (unsigned n = 3; n <= 10; ++n) registers.emplace_back(n, 0x7a000000u + n * 0x01010101u);
    for (const auto& [n, value] : used) registers[n - 3].second = value;
    return registers;
}
void add(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }
// The surface of a case: `stride` pixels a row (rows 0..39), `row_bytes` the variable 0x1076b020 reads (the stride's double unless a case
// says otherwise), `base` where the first pixel is.
std::vector<Write> surface(std::int32_t stride = 50, std::int32_t row_bytes = 100, std::uint32_t base = surface_at) {
    std::vector<Write> memory{w32(small(surface_data::base), base), w16(small(surface_data::stride_pixels), std::uint32_t(stride)),
                              w16(small(surface_data::row_bytes), std::uint32_t(row_bytes)), w8(small(surface_data::setting_up), 0)};
    // The pattern is laid out for a stride of 50 pixels from `surface_at`, whatever the variables say (a case with another stride reads it
    // with that stride).
    for (std::uint32_t y = 0; y < 40; ++y)
        for (std::uint32_t x = 0; x < 50; x += 2)
            memory.push_back(w32(surface_at + (y * 50 + x) * 2, ((0x4000 + y * 0x100 + x) << 16) | (0x4000 + y * 0x100 + x + 1)));
    return memory;
}
// A bitmap record in scratch: width, height, byte size, the pixels' address, then 0x20 bytes of metadata that name their place.
std::vector<Write> record(std::uint32_t at, std::uint32_t width, std::uint32_t height, std::uint32_t bytes, std::uint32_t pixels, std::uint32_t seed = 0x1000) {
    std::vector<Write> memory{w32(at, width), w32(at + 4, height), w32(at + 8, bytes), w32(at + 12, pixels)};
    for (std::uint32_t n = 0; n < 8; ++n) memory.push_back(w32(at + 0x10 + 4 * n, seed * (n + 1) + 0x0f0f0f0f * n));
    return memory;
}
// A bitmap's pixels: halfword n holds 0x2000 + n.
std::vector<Write> pixels_of(std::uint32_t at, std::uint32_t count) {
    std::vector<Write> memory;
    for (std::uint32_t n = 0; n < count; n += 2) memory.push_back(w32(at + 2 * n, ((0x2000 + n) << 16) | (0x2000 + n + 1)));
    return memory;
}
std::vector<Write> text(std::uint32_t at, const std::string& value) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n <= value.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), n < value.size() ? std::uint8_t(value[n]) : 0));
    return memory;
}
// Real heap blocks for the checked call's argument: the original allocator run from the run's own state (the same sequence on both sides),
// the pixel block holding the pattern 0x3000 + n and the record's metadata distinct.
struct Heap {
    std::uint32_t width, height, bytes;
    bool with_pixels = true;
};
std::uint32_t heap_bitmap(State& s, const Heap& heap) {
    guest::Cpu c(s);
    const auto call = [&](std::uint32_t size) { c.set(3, size); graph_carbon_run_original(s, 0x10775130); return c.r(3); };
    const auto pixels = heap.with_pixels ? call(heap.bytes) : 0u;
    const auto at = call(bitmap::size);
    c.put32(at, heap.width);
    c.put32(at + 4, heap.height);
    c.put32(at + 8, heap.bytes);
    c.put32(at + 12, pixels);
    for (std::uint32_t n = 0; n < 8; ++n) c.put32(at + 0x10 + 4 * n, 0xabc00000u + 0x111 * n);
    for (std::uint32_t n = 0; heap.with_pixels && n + 1 < heap.bytes; n += 2) c.put16(pixels + n, 0x3000 + n / 2);
    return at;
}
// A case on a heap bitmap: `prepare` allocates it and sets the argument registers (the others as `after` gives them).
Case heap_case(std::uint32_t entry, std::string label, const Heap& heap, std::vector<std::pair<unsigned, std::uint32_t>> after, std::vector<Write> memory = {},
               bool record_first = true) {
    Case item{.entry = entry, .registers = others({}), .memory = std::move(memory), .label = std::move(label)};
    item.prepare = [heap, after, record_first](State& s) {
        const auto at = heap_bitmap(s, heap);
        guest::Cpu c(s);
        for (unsigned n = 3; n <= 10; ++n) c.set(n, 0x7a000000u + n * 0x01010101u);
        c.set(record_first ? 3 : 5, at);
        for (const auto& [n, value] : after) c.set(n, value);
    };
    item.original_owners = {allocate, release};
    return item;
}
}

GRAPH_CARBON_CASES("colours") {
    std::mt19937 random(0x6c42);
    const auto sp = scenario.sp();
    // ---- 0x1076b830: the 25 colour variables, 25 adjacent halfwords (0x10bf7200..0x10bf7230 in the game). Poisoned first, so a missing or
    // misplaced store shows, with a guard halfword on each side; the display flag clear (all set, from an independent formula) and set (nothing
    // stored: the variables keep the poison).
    struct Expected { int slot; std::uint8_t r, g, b; };
    static const Expected expected[] = {
        {-0x7bd4, 0, 0, 0}, {-0x7a8c, 255, 255, 255}, {-0x7bc8, 255, 255, 255}, {-0x7a74, 255, 0, 0}, {-0x7a80, 128, 0, 0}, {-0x7bf4, 255, 128, 0},
        {-0x7a84, 128, 64, 0}, {-0x7bd0, 255, 255, 0}, {-0x5b00, 255, 255, 128}, {-0x7bcc, 0, 128, 0}, {-0x7a78, 0, 64, 0}, {-0x7a90, 0, 255, 255},
        {-0x7bd8, 128, 255, 255}, {-0x7b14, 0, 0, 255}, {-0x7a94, 0, 128, 255}, {-0x7bdc, 0, 0, 128}, {-0x6224, 0, 0, 96}, {-0x5b04, 128, 0, 128},
        {-0x7bec, 64, 0, 64}, {-0x5b08, 128, 64, 64}, {-0x7bc4, 128, 128, 128}, {-0x5b0c, 224, 224, 224}, {-0x7b10, 192, 192, 192},
        {-0x7a88, 64, 64, 64}, {-0x5b10, 0, 128, 128}};
    {
        std::vector<Write> poison;
        const auto addresses = std::make_shared<std::vector<std::uint32_t>>();
        std::uint32_t lowest = ~0u, highest = 0;
        for (const auto& e : expected) {
            addresses->push_back(scenario.toc(e.slot));
            poison.push_back(w16(addresses->back(), 0xa5a5));
            lowest = std::min(lowest, addresses->back());
            highest = std::max(highest, addresses->back());
        }
        poison.push_back(w16(lowest - 2, 0x5a5a));
        poison.push_back(w16(highest + 2, 0x5a5a));
        for (const std::uint32_t flag : {0u, 1u, 2u, 0x80u, 0xffu, 0x40u}) {
            auto memory = poison;
            memory.push_back(w8(small(surface_data::setting_up), flag));
            Case item{.entry = colours_entry, .registers = others({}), .memory = memory, .label = "colours: variables, display flag " + hex(flag)};
            item.verify = [flag, addresses, lowest, highest](State& s) {
                guest::Cpu c(s);
                for (std::size_t n = 0; n < std::size(expected); ++n) {
                    const auto& e = expected[n];
                    const auto want = flag == 0 ? std::uint32_t(((e.r >> 3) << 10) | ((e.g >> 3) << 5) | (e.b >> 3)) : 0xa5a5u;
                    if (c.u16((*addresses)[n]) != want) return false;
                }
                return c.u16(lowest - 2) == 0x5a5a && c.u16(highest + 2) == 0x5a5a;
            };
            cases.push_back(std::move(item));
        }
        cases.push_back({.entry = colours_entry, .registers = others({}), .label = "colours: variables, the snapshot's state"});
    }

    // ---- 0x1076b6e0: the clone. Reuse 0 / the same size / a different width / a different height, both allocations, each failing (the
    // first: no pixel block; the second: the record, which gives the pixel block back). The source's metadata is copied whatever it holds.
    const auto clone_case = [&](const std::string& label, std::uint32_t width, std::uint32_t height, std::uint32_t bytes, std::uint32_t reuse,
                                std::uint32_t reuse_width, std::uint32_t reuse_height) {
        auto memory = record(source_at, width, height, bytes, 0x5eed0000, 0x1111);
        if (reuse) add(memory, record(reuse, reuse_width, reuse_height, 0x20, 0x600d0000, 0x77));
        return Case{.entry = clone_entry, .registers = others({{3, source_at}, {4, reuse}}), .memory = memory, .label = "colours: clone " + label,
                    .original_owners = {allocate, release}};
    };
    {
        auto first_fails = clone_case("first allocation fails", 10, 5, 100, 0, 0, 0);
        first_fails.nth_answers = {{allocate, 1, 0}};
        cases.push_back(first_fails);
        auto second_fails = clone_case("second allocation fails", 10, 5, 100, 0, 0, 0);
        second_fails.nth_answers = {{allocate, 2, 0}};
        cases.push_back(second_fails);
        auto reuse_misses = clone_case("other size, second allocation fails", 10, 5, 100, reuse_at, 9, 9);
        reuse_misses.nth_answers = {{allocate, 2, 0}};
        cases.push_back(reuse_misses);
    }
    cases.push_back(clone_case("no reuse 10x5", 10, 5, 100, 0, 0, 0));
    cases.push_back(clone_case("no reuse 1x1", 1, 1, 2, 0, 0, 0));
    cases.push_back(clone_case("no reuse empty", 0, 0, 0, 0, 0, 0));
    cases.push_back(clone_case("no reuse size 1", 3, 3, 1, 0, 0, 0));
    cases.push_back(clone_case("no reuse large", 640, 480, 640 * 480 * 2, 0, 0, 0));
    cases.push_back(clone_case("same size", 10, 5, 100, reuse_at, 10, 5));
    cases.push_back(clone_case("other width", 10, 5, 100, reuse_at, 11, 5));
    cases.push_back(clone_case("other height", 10, 5, 100, reuse_at, 10, 6));
    cases.push_back(clone_case("other both", 10, 5, 100, reuse_at, 4, 4));
    cases.push_back(clone_case("same width 0, same height", 0, 5, 100, reuse_at, 0, 5));
    cases.push_back(clone_case("width differs in the sign bit", 0x80000000u, 5, 100, reuse_at, 0, 5));
    cases.push_back(clone_case("height differs in the sign bit", 7, 0x80000005u, 100, reuse_at, 7, 5));
    for (unsigned n = 0; n < 40; ++n) {
        const auto w = std::uint32_t(random() % 40), h = std::uint32_t(random() % 40);
        cases.push_back(clone_case("random " + std::to_string(n), w, h, w * h * 2, n % 3 ? reuse_at : 0u, n % 2 ? w : std::uint32_t(random() % 40),
                                   n % 5 ? h : std::uint32_t(random() % 40)));
    }

    // ---- 0x1076b7e0: free. A real record with a pixel block, one without (nothing to free first), none at all (nothing happens). The leaver
    // check compares the frame's entry, and the dead stack is compared byte for byte.
    cases.push_back({.entry = free_entry, .registers = others({{3, 0}}), .label = "colours: free null"});
    cases.push_back({.entry = free_entry, .registers = others({{3, 0}, {4, 0x1234}}), .label = "colours: free null, r4 kept"});
    cases.push_back(heap_case(free_entry, "colours: free without pixels", {10, 5, 100, false}, {}));
    for (const std::uint32_t bytes : {2u, 100u, 5000u, 0u}) cases.push_back(heap_case(free_entry, "colours: free with pixels " + std::to_string(bytes), {10, 5, bytes}, {}));

    // ---- 0x1076b020: capture a rectangle of the surface. The corners in each order, the stride and the row-bytes variable apart, a reused
    // record of the same size or another, the clone's stale metadata (words the frame holds that the clone copies), allocation failures and
    // the zero-height wrap.
    const auto frame_record = sp - 0x90 + 0x38;
    const auto capture_case = [&](const std::string& label, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t reuse,
                                  std::vector<Write> memory, bool stale) {
        add(memory, surface());
        if (stale)
            for (std::uint32_t n = 0; n < 9; ++n) memory.push_back(w32(frame_record + 0xc + 4 * n, 0xc0de0000u + 0x10101 * n));
        return Case{.entry = capture_entry,
                    .registers = others({{3, std::uint32_t(x1)}, {4, std::uint32_t(y1)}, {5, std::uint32_t(x2)}, {6, std::uint32_t(y2)}, {7, reuse}}),
                    .memory = std::move(memory), .label = "colours: capture " + label, .original_owners = {allocate, release}};
    };
    // What a capture in the surface must have produced (an independent computation): the new record's size, its pixels, and the frame's words.
    const auto capture_result = [](std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, bool stale) {
        return [=](State& s) {
            guest::Cpu c(s);
            const auto at = c.r(3);
            if (!at) return false;
            const auto left = std::min(x1, x2), top = std::min(y1, y2), right = std::max(x1, x2), bottom = std::max(y1, y2);
            const auto width = std::uint32_t(right - left + 1), height = std::uint32_t(bottom - top + 1);
            if (c.u32(at) != width || c.u32(at + 4) != height || c.u32(at + 8) != width * height * 2) return false;
            const auto pixels = c.u32(at + 12);
            for (std::uint32_t y = 0; y < height; ++y)
                for (std::uint32_t x = 0; x < width; ++x)
                    if (c.u16(pixels + (y * width + x) * 2) != 0x4000 + std::uint32_t(top + std::int32_t(y)) * 0x100 + std::uint32_t(left + std::int32_t(x))) return false;
            if (stale)
                for (std::uint32_t n = 0; n < 7; ++n)
                    if (c.u32(at + 0x10 + 4 * n) != 0xc0de0000u + 0x10101 * (n + 1)) return false;
            return true;
        };
    };
    {
        auto first = capture_case("first allocation fails", 3, 2, 10, 7, 0, {}, true);
        first.nth_answers = {{allocate, 1, 0}};
        cases.push_back(first);
        auto second = capture_case("second allocation fails", 3, 2, 10, 7, 0, {}, true);
        second.nth_answers = {{allocate, 2, 0}};
        cases.push_back(second);
    }
    struct Rect { std::int32_t x1, y1, x2, y2; };
    for (const Rect r : {Rect{0, 0, 0, 0}, {3, 2, 10, 7}, {10, 7, 3, 2}, {3, 7, 10, 2}, {10, 2, 3, 7}, {0, 0, 49, 39}, {49, 39, 49, 39}, {5, 5, 5, 9},
                         {5, 5, 9, 5}, {1, 1, 2, 2}, {20, 10, 33, 25}}) {
        auto item = capture_case("(" + std::to_string(r.x1) + "," + std::to_string(r.y1) + ")-(" + std::to_string(r.x2) + "," + std::to_string(r.y2) + ")", r.x1, r.y1, r.x2, r.y2, 0, {}, true);
        item.verify = capture_result(r.x1, r.y1, r.x2, r.y2, true);
        cases.push_back(std::move(item));
    }
    cases.push_back(capture_case("with the snapshot's stack bytes", 4, 4, 9, 9, 0, {}, false));
    // The row-bytes variable the routine reads (halved towards zero) is not the stride the blit reads.
    for (const std::int32_t row_bytes : {101, 99, 1, 2, 3, -100, -101, 0x7fff, -0x8000}) {
        auto item = capture_case("row bytes " + std::to_string(row_bytes), 2, 3, 6, 9, 0, {}, true);
        item.memory.push_back(w16(small(surface_data::row_bytes), std::uint32_t(row_bytes)));
        // The rows of the two extreme lengths lie far apart: the base is put where both directions stay inside the scratch area.
        if (row_bytes == -0x8000) item.memory.push_back(w32(small(surface_data::base), surface_at + 0x70000));
        cases.push_back(std::move(item));
    }
    {
        auto item = capture_case("surface base in the middle, negative row length", 2, 3, 6, 9, 0, {}, true);
        item.memory.push_back(w32(small(surface_data::base), surface_at + 0x1000));
        item.memory.push_back(w16(small(surface_data::row_bytes), 0xff9c));
        cases.push_back(std::move(item));
    }
    // A reused record: the same size (handed back, its pixels rewritten in place), another width, another height.
    for (const Rect r : {Rect{3, 2, 10, 7}, Rect{3, 2, 10, 8}, Rect{3, 2, 9, 7}, Rect{3, 2, 11, 7}}) {
        auto memory = record(reuse_at, 8, 6, 96, reuse_pixels, 0x31);
        for (std::uint32_t n = 0; n < 48; n += 2) memory.push_back(w32(reuse_pixels + 2 * n, 0xdead0000u + n));
        cases.push_back(capture_case("reuse 8x6 for " + std::to_string(r.x2 - r.x1 + 1) + "x" + std::to_string(r.y2 - r.y1 + 1), r.x1, r.y1, r.x2, r.y2, reuse_at, memory, true));
    }
    // Halfword wraps: a span of 0x10000 is a zero height or width (no row copied, or rows of no bytes: r4 and r5 stay as the routine set them), a
    // larger one a negative size (the clone's allocation fails), coordinates above 0x7fff come back sign-extended.
    cases.push_back(capture_case("zero height by wrap", 2, 0, 6, 0xffff, 0, {}, true));
    cases.push_back(capture_case("zero width by wrap", 0, 3, 0xffff, 8, 0, {}, true));
    cases.push_back(capture_case("zero height by wrap, reuse", 2, 0, 6, 0xffff, reuse_at, record(reuse_at, 5, 0, 0, reuse_pixels, 0x31), true));
    cases.push_back(capture_case("negative height by wrap (the clone's size fails)", 2, 3, 6, 0xffff, 0, {}, true));
    cases.push_back(capture_case("negative width by wrap (the clone's size fails)", 2, 3, 0xffff, 8, 0, {}, true));
    cases.push_back(capture_case("negative coordinates", -2, -1, 3, 2, 0, {}, true));
    cases.push_back(capture_case("corner 0x10003", 0x10003, 2, 0x10005, 4, 0, {}, true));
    for (unsigned n = 0; n < 60; ++n) {
        const auto x1 = std::int32_t(random() % 60) - 5, y1 = std::int32_t(random() % 44) - 2, x2 = std::int32_t(random() % 60) - 5, y2 = std::int32_t(random() % 44) - 2;
        const auto w = std::uint32_t(random() % 12) + 1, h = std::uint32_t(random() % 12) + 1;
        cases.push_back(capture_case("random " + std::to_string(n), x1, y1, x2, y2, n % 4 == 0 ? reuse_at : 0u, n % 4 == 0 ? record(reuse_at, w, h, 0x40, reuse_pixels, 5) : std::vector<Write>{}, n % 2 == 0));
    }

    // ---- 0x1076b140: blit a bitmap onto the surface. Positions in and out of the surface (halfwords: 0xffff is -1), sizes (width 0, height 0, one
    // pixel, the whole surface), another stride than the capture's variable, and no bitmap (the assertion report).
    const auto blit_case = [&](const std::string& label, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t bitmap_at) {
        auto memory = surface();
        add(memory, record(source_at, width, height, width * height * 2, reuse_pixels, 0x2222));
        add(memory, pixels_of(reuse_pixels, width * height));
        return Case{.entry = blit_entry, .registers = others({{3, x}, {4, y}, {5, bitmap_at}}), .memory = memory, .label = "colours: blit " + label};
    };
    cases.push_back(blit_case("no bitmap", 3, 4, 6, 5, 0));
    for (const auto& [x, y, w, h] : {std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t>{0, 0, 1, 1}, {3, 2, 8, 6}, {40, 30, 10, 10}, {0, 0, 50, 40},
                                     {5, 5, 0, 4}, {5, 5, 4, 0}, {49, 39, 1, 1}, {7, 1, 3, 3}, {10, 10, 7, 5}}) {
        auto item = blit_case("(" + std::to_string(x) + "," + std::to_string(y) + ") " + std::to_string(w) + "x" + std::to_string(h), x, y, w, h, source_at);
        item.verify = [=, x = x, y = y, w = w, h = h](State& s) {
            guest::Cpu c(s);
            for (std::uint32_t row = 0; row < h; ++row)
                for (std::uint32_t col = 0; col < w; ++col)
                    if (c.u16(surface_at + ((y + row) * 50 + x + col) * 2) != 0x2000 + row * w + col) return false;
            return true;
        };
        cases.push_back(std::move(item));
    }
    cases.push_back(blit_case("negative position", 0xffff, 0xfffe, 6, 4, source_at));
    cases.push_back(blit_case("position above a halfword", 0x10005, 0x20003, 6, 4, source_at));
    cases.push_back(blit_case("width above a halfword", 2, 2, 0x10003, 1, source_at));
    {
        auto item = blit_case("other stride", 4, 4, 6, 5, source_at);
        item.memory.push_back(w16(small(surface_data::stride_pixels), 64));
        item.memory.push_back(w16(small(surface_data::row_bytes), 20));
        cases.push_back(std::move(item));
        auto negative = blit_case("negative stride", 4, 4, 6, 5, source_at);
        negative.memory.push_back(w16(small(surface_data::stride_pixels), 0xffce));
        negative.memory.push_back(w32(small(surface_data::base), surface_at + 0x1800));
        cases.push_back(std::move(negative));
    }
    for (unsigned n = 0; n < 50; ++n) {
        const auto x = std::uint32_t(random() % 56) - 3, y = std::uint32_t(random() % 44) - 2, w = std::uint32_t(random() % 9), h = std::uint32_t(random() % 7);
        cases.push_back(blit_case("random " + std::to_string(n), x, y, w, h, source_at));
    }
    // A heap record (as a capture returns one) put back.
    cases.push_back(heap_case(blit_entry, "colours: blit a heap record", {6, 4, 48}, {{3, 7}, {4, 5}}, surface(), false));

    // ---- 0x1076b240: save a region file. The display flag, no bitmap, no path, fopen failing, the first write failing, both writing, and the
    // answers of the writes. The bytes handed to fwrite are captured: at the second write the pixels.
    const auto save_case = [&](const std::string& label, std::uint32_t bitmap_at, std::uint32_t path, std::uint32_t flag, std::uint32_t opened, std::uint32_t first_write,
                               std::uint32_t second_write) {
        auto memory = record(source_at, 12, 5, 0x78, source_at + 0x100, 0x5555);
        add(memory, pixels_of(source_at + 0x100, 0x3c));
        add(memory, text(text_at, "regions/test.rgm"));
        memory.push_back(w8(small(surface_data::setting_up), flag));
        Case item{.entry = save_entry, .registers = others({{3, bitmap_at}, {4, path}}), .memory = memory, .label = "colours: save " + label};
        item.answers = {{open_file, opened}, {close_file, 0}};
        item.nth_answers = {{write_file, 1, first_write}, {write_file, 2, second_write}};
        item.original_owners = {write_file};
        item.captures = {{write_file, 3, 0x30, capture_a}, {write_file, 3, 0x78, capture_b}};
        return item;
    };
    cases.push_back(save_case("open fails", source_at, text_at, 0, 0, 1, 1));
    cases.push_back(save_case("first write fails", source_at, text_at, 0, stream_at, 0, 1));
    cases.push_back(save_case("first write returns 2", source_at, text_at, 0, stream_at, 2, 1));
    cases.push_back(save_case("second write fails", source_at, text_at, 0, stream_at, 1, 0));
    cases.push_back(save_case("both writes", source_at, text_at, 0, stream_at, 1, 1));
    cases.back().verify = [](State& s) {
        guest::Cpu c(s);
        for (std::uint32_t n = 0; n < 0x78; n += 2)
            if (c.u16(capture_b + n) != 0x2000 + n / 2) return false;
        return true;
    };
    cases.push_back(save_case("no bitmap", 0, text_at, 0, stream_at, 1, 1));
    cases.push_back(save_case("no path", source_at, 0, 0, stream_at, 1, 1));
    for (const std::uint32_t flag : {1u, 2u, 0x80u, 0xffu}) cases.push_back(save_case("display flag " + hex(flag), source_at, text_at, flag, stream_at, 1, 1));
    cases.push_back(save_case("stream pointer 0x80000000", source_at, text_at, 0, 0x80000000u, 1, 1));

    // ---- 0x1076b450 (and 0x1076b2f0): a bitmap file. The header (width, height, size, a format word, 0x20 bytes of metadata: little-endian in the
    // file) is poked into the record on the routine's frame at each read, the pixels into the pixel block of a reused record (so the conversion's
    // input is known) or left to what a fresh block holds (the allocator's bytes, the same on both sides). The reads are answered by index.
    const auto file_image = [&](std::uint32_t frame_record, std::uint32_t width, std::uint32_t height, std::uint32_t bytes, std::uint32_t format) {
        std::vector<Write> memory{w32(frame_record, swap32(width)), w32(frame_record + 4, swap32(height)), w32(frame_record + 8, swap32(bytes)),
                                  w32(frame_record + 0xc, 0x12345678u)};
        for (std::uint32_t n = 0; n < 8; ++n) memory.push_back(w32(frame_record + 0x10 + 4 * n, 0x01020304u * (n + 1)));
        memory.push_back(w32(frame_record + 0x10 + 0x14, swap32(format)));
        return memory;
    };
    const auto read_frame = sp - 0x80 + 0x38;
    // The pixels of a reused record, in the file's byte order: halfword n is `n * 7 + 3` (or n itself over the whole range).
    const auto file_pixels = [&](std::uint32_t count, bool whole_range) {
        std::vector<Write> memory;
        for (std::uint32_t n = 0; n < count; n += 2) {
            const auto first = whole_range ? n : (n * 7 + 3) & 0xffff, second = whole_range ? n + 1 : ((n + 1) * 7 + 3) & 0xffff;
            memory.push_back(w32(reuse_pixels + 2 * n, (first << 16) | second));
        }
        return memory;
    };
    const auto read_case = [&](const std::string& label, std::uint32_t width, std::uint32_t height, std::uint32_t format, std::uint32_t reuse, std::uint32_t file,
                               std::vector<Write> pixels, int fail_read, std::vector<Case::NthAnswer> more = {}) {
        auto memory = file_image(read_frame, width, height, width * height * 2, format);
        if (reuse) add(memory, record(reuse, width, height, width * height * 2, reuse_pixels, 0x41));
        Case item{.entry = read_entry, .registers = others({{3, file}, {4, reuse}}), .memory = memory, .label = "colours: read bitmap " + label};
        // Pokes happen at each read; they hold the header and (for a reused record) the pixels the last read leaves.
        item.pokes = {{read_file, [&] { auto all = file_image(read_frame, width, height, width * height * 2, format); add(all, pixels); return all; }()}};
        item.answers = {{read_file, 1}, {close_file, 0}};
        item.nth_answers = std::move(more);
        if (fail_read > 0) item.nth_answers.push_back({read_file, unsigned(fail_read), 0});
        item.original_owners = {read_file, allocate, release};
        item.imports = {{"CarbonLib::Logged", 0, 0}};
        return item;
    };
    // Failures first (the call trace pairs only the first cases of an entry): each read, both allocations, a file of 0.
    for (int k = 1; k <= 6; ++k) cases.push_back(read_case("read " + std::to_string(k) + " fails, fresh", 6, 4, 0x7e0, 0, stream_at, {}, k));
    for (int k = 5; k <= 6; ++k) cases.push_back(read_case("read " + std::to_string(k) + " fails, reuse", 6, 4, 0x7e0, reuse_at, stream_at, file_pixels(24, false), k));
    cases.push_back(read_case("read 1 fails, file 0", 6, 4, 0x7e0, 0, 0, {}, 1));
    cases.push_back(read_case("read 6 fails, file 0", 6, 4, 0x7e0, 0, 0, {}, 6));
    {
        auto item = read_case("first allocation fails", 6, 4, 0x7e0, 0, stream_at, {}, 0);
        item.nth_answers.push_back({allocate, 1, 0});
        cases.push_back(item);
        auto second = read_case("second allocation fails", 6, 4, 0x7e0, 0, stream_at, {}, 0);
        second.nth_answers.push_back({allocate, 2, 0});
        cases.push_back(second);
        auto file_zero = read_case("allocation fails, file 0", 6, 4, 0x7e0, 0, 0, {}, 0);
        file_zero.nth_answers.push_back({allocate, 2, 0});
        cases.push_back(file_zero);
    }
    // The conversion (5-6-5 to 5-5-5): over a reused record (known input), each count class (0, 1, 2, 3, even, odd, large), and over the whole
    // 16-bit range (256 x 256), then the other formats (a swap of the halfwords).
    for (const auto& [w, h] : {std::pair<std::uint32_t, std::uint32_t>{0, 5}, {5, 0}, {1, 1}, {2, 1}, {3, 1}, {1, 3}, {4, 4}, {5, 3}, {7, 7}, {16, 16}, {17, 9}}) {
        auto item = read_case("565 " + std::to_string(w) + "x" + std::to_string(h) + ", reuse", w, h, 0x7e0, reuse_at, stream_at, file_pixels(w * h, false), 0);
        cases.push_back(std::move(item));
    }
    cases.push_back(read_case("565 4x2, fresh block", 4, 2, 0x7e0, 0, stream_at, {}, 0));
    cases.push_back(read_case("565 256x256 whole range", 256, 256, 0x7e0, reuse_at, stream_at, file_pixels(65536, true), 0));
    for (const std::uint32_t format : {0u, 1u, 0x7e1u, 0x17e0u, 0x7e00u, 0x7e00000u, 0xe0070000u, 0x7ffu, 0x3e0u, 0xffffffffu}) {
        cases.push_back(read_case("format " + hex(format) + " 5x3, reuse", 5, 3, format, reuse_at, stream_at, file_pixels(15, false), 0));
    }
    cases.push_back(read_case("format 0 4x4, fresh block", 4, 4, 0, 0, stream_at, {}, 0));
    cases.push_back(read_case("format 0 0x0", 0, 0, 0, reuse_at, stream_at, {}, 0));
    cases.push_back(read_case("reuse other size", 6, 4, 0x7e0, reuse_at, stream_at, {}, 0, {}));
    cases.back().memory.push_back(w32(reuse_at, 5));
    cases.push_back(read_case("sizes 1x1, fresh", 1, 1, 0x7e0, 0, stream_at, {}, 0));
    cases.push_back(read_case("sizes 100x50, fresh", 100, 50, 0x7e0, 0, stream_at, {}, 0));
    for (unsigned n = 0; n < 20; ++n) {
        const auto w = std::uint32_t(random() % 13), h = std::uint32_t(random() % 13), format = n % 3 ? 0x7e0u : std::uint32_t(random());
        cases.push_back(read_case("random " + std::to_string(n), w, h, format, n % 2 ? reuse_at : 0u, n % 5 ? stream_at : 0u, n % 2 ? file_pixels(w * h, false) : std::vector<Write>{}, 0));
    }

    // ---- 0x1076b2f0: a region. The path (a text in scratch) names a bitmap file; its cache is the `.rgm` beside it (0x10746470 builds the name). The
    // first fopen is the cache's: answered with a stream, the file is read as a region (a 0x30 record and the pixels); answered with 0, the second
    // fopen opens the bitmap file itself (read, converted, cached by 0x1076b240 with a third fopen).
    const auto region_case = [&](const std::string& label, std::uint32_t path, std::uint32_t flag, std::uint32_t reuse, std::vector<Case::NthAnswer> opens,
                                 std::vector<Case::NthAnswer> reads, std::uint32_t width, std::uint32_t height, std::uint32_t format) {
        auto memory = text(text_at, "regions/test.bmp");
        memory.push_back(w8(small(surface_data::setting_up), flag));
        if (reuse) add(memory, record(reuse, width, height, width * height * 2, reuse_pixels, 0x61));
        Case item{.entry = load_entry, .registers = others({{3, path}, {4, reuse}}), .memory = memory, .label = "colours: region " + label};
        const auto region_record = sp - 0x180 + 0x38, bitmap_record = sp - 0x180 - 0x80 + 0x38;
        // The cache's record is read with big-endian words (as the game keeps them), the bitmap file's little-endian.
        std::vector<Write> pokes = file_image(bitmap_record, width, height, width * height * 2, format);
        pokes.insert(pokes.end(), {w32(region_record, width), w32(region_record + 4, height), w32(region_record + 8, width * height * 2), w32(region_record + 12, 0x5eed0000)});
        for (std::uint32_t n = 0; n < 8; ++n) pokes.push_back(w32(region_record + 0x10 + 4 * n, 0x0badf00du + n));
        if (reuse) add(pokes, file_pixels(width * height, false));
        item.pokes = {{read_file, pokes}};
        item.answers = {{open_file, stream_at}, {read_file, 1}, {close_file, 0}, {write_file, 1}};
        item.nth_answers = std::move(opens);
        for (const auto& read : reads) item.nth_answers.push_back(read);
        item.original_owners = {read_file, write_file, allocate, release};
        item.captures = {{write_file, 3, 0x30, capture_a}};
        item.imports = {{"CarbonLib::Logged", 0, 0}};
        return item;
    };
    cases.push_back(region_case("display flag set", text_at, 1, 0, {}, {}, 6, 4, 0x7e0));
    cases.push_back(region_case("no path", 0, 0, 0, {}, {}, 6, 4, 0x7e0));
    cases.push_back(region_case("no path, reuse", 0, 0, reuse_at, {}, {}, 6, 4, 0x7e0));
    cases.push_back(region_case("cache opens, header read fails", text_at, 0, 0, {}, {{read_file, 1, 0}}, 6, 4, 0x7e0));
    cases.push_back(region_case("cache opens, pixel read fails, fresh", text_at, 0, 0, {}, {{read_file, 2, 0}}, 6, 4, 0x7e0));
    cases.push_back(region_case("cache opens, pixel read fails, reuse", text_at, 0, reuse_at, {}, {{read_file, 2, 0}}, 6, 4, 0x7e0));
    cases.push_back(region_case("cache opens, fresh", text_at, 0, 0, {}, {}, 6, 4, 0x7e0));
    cases.push_back(region_case("cache opens, reuse of the same size", text_at, 0, reuse_at, {}, {}, 6, 4, 0x7e0));
    cases.push_back(region_case("cache opens, allocation fails", text_at, 0, 0, {}, {{allocate, 2, 0}}, 6, 4, 0x7e0));
    cases.push_back(region_case("neither opens", text_at, 0, 0, {{open_file, 1, 0}, {open_file, 2, 0}}, {}, 6, 4, 0x7e0));
    cases.push_back(region_case("neither opens, reuse", text_at, 0, reuse_at, {{open_file, 1, 0}, {open_file, 2, 0}}, {}, 6, 4, 0x7e0));
    cases.push_back(region_case("bitmap file, read fails", text_at, 0, 0, {{open_file, 1, 0}}, {{read_file, 3, 0}}, 6, 4, 0x7e0));
    cases.push_back(region_case("bitmap file, converted and cached", text_at, 0, 0, {{open_file, 1, 0}}, {}, 6, 4, 0x7e0));
    cases.push_back(region_case("bitmap file, converted and cached, reuse", text_at, 0, reuse_at, {{open_file, 1, 0}}, {}, 6, 4, 0x7e0));
    cases.push_back(region_case("bitmap file, other format", text_at, 0, 0, {{open_file, 1, 0}}, {}, 5, 3, 0x1234));
    cases.push_back(region_case("bitmap file, cache not written (open fails)", text_at, 0, 0, {{open_file, 1, 0}, {open_file, 3, 0}}, {}, 6, 4, 0x7e0));
    cases.push_back(region_case("bitmap file, cache write fails", text_at, 0, 0, {{open_file, 1, 0}, {write_file, 1, 0}}, {}, 6, 4, 0x7e0));
    cases.push_back(region_case("bitmap file, allocation fails", text_at, 0, 0, {{open_file, 1, 0}}, {{allocate, 1, 0}}, 6, 4, 0x7e0));
    cases.push_back(region_case("bitmap file, empty", text_at, 0, 0, {{open_file, 1, 0}}, {}, 0, 0, 0x7e0));
    (void)load_entry;
}
}
