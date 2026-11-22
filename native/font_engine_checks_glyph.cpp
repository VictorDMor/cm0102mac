// The font engine, block `glyph` (GLYPH.C, 0x1075b970..0x1075c97c): cases for the glyph record's six routines. Labels start with
// "glyph:" (CM_FONT_ENGINE_BLOCK=glyph).
//  - 0x1075b970 reads points from a stream built in the scratch area: every code (0-3) at the edges of each radix of the format
//    (0, 1, 0x897 | 0x898, 0x898+0x65 | +0x66, 0x313b | 0x313c, 0x393b | 0x393c, 0x3fff), extra bytes at their own edges, the
//    two-halfword form (value 0 with codes 0 and 1), seeded random points, a non-zero position.
//  - 0x1075c830, 0x1075bba0, 0x1075bc90 run on the snapshot's real memory object (the allocator is the real one): counts at the
//    edges, simple glyphs of 1-10 contours (the inline arrays and the allocated ones), composites of one to a few components with
//    every flag combination, 300 and 700 components (the buffer grows once and twice), the reallocation answered with 0 and the
//    assertions (negative glyph index, no buffer) answered. 0x1075c830's stack image is verified (what the printf readers read).
//  - 0x1075c0f0 and 0x1075c8c0 work on records the case builds in the scratch area over a memory object of its own (so the cached
//    block pointers can be set equal and unequal); the allocator's calls are answered: the new blocks come from scratch areas,
//    the releases do nothing.
#include "font_engine_checks.hpp"
#include <algorithm>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t read_point = 0x1075b970, new_empty = 0x1075bba0, new_glyph = 0x1075bc90, append_entry = 0x1075c0f0, allocate_points = 0x1075c830,
                        delete_glyph = 0x1075c8c0, reporter = 0x107755c0, release = 0x10767bb0, allocate = 0x107678a0, reallocate = 0x10767a00,
                        game_toc = 0x108bba90;
constexpr std::uint32_t stream_at = scratch_base + 0x20000, data_at = scratch_base + 0x20100, out_x = scratch_base + 0x28000, out_y = scratch_base + 0x28004;
// Records of the append and delete cases.
constexpr std::uint32_t slot_at = scratch_base + 0x30000, destination_at = scratch_base + 0x31000, source_at = scratch_base + 0x32000,
                        destination_block = scratch_base + 0x33000, source_block = scratch_base + 0x34000, buffer_a = scratch_base + 0x35000,
                        buffer_b = scratch_base + 0x36000, fake_mem = scratch_base + 0x37000, destination_contours = scratch_base + 0x38000,
                        source_contours = scratch_base + 0x38400, instruction_block = scratch_base + 0x39000, component_block = scratch_base + 0x39400;

std::uint32_t mem_object(const Scenario& scenario) {
    static std::uint32_t found = 0;
    if (found) return found;
    for (const auto& [low, size] : scenario.s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x5f8 <= std::uint64_t(low) + size; at += 4)
            if (scenario.r32(std::uint32_t(at)) == 0x5a1234a5 && scenario.r32(std::uint32_t(at) + 0x5f4) == 0xa5fedc5a) return found = scenario.r32(std::uint32_t(at) + 4);
    }
    return found;
}
void put_bytes(std::vector<Write>& memory, std::uint32_t at, const std::vector<std::uint8_t>& bytes) {
    for (std::size_t n = 0; n < bytes.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), bytes[n]));
}
void stream_over(std::vector<Write>& memory, const std::vector<std::uint8_t>& bytes, std::uint32_t position = 0) {
    memory.push_back(w32(stream_at, data_at));
    memory.push_back(w32(stream_at + 4, position));
    memory.push_back(w32(stream_at + 8, std::uint32_t(bytes.size())));
    put_bytes(memory, data_at, bytes);
}
// One compact point: the code in the top bits, the value, and the extra bytes the radix needs.
std::vector<std::uint8_t> compact_point(unsigned code, unsigned value, std::mt19937& random, int extra = -1) {
    std::vector<std::uint8_t> bytes{std::uint8_t(code << 6 | value >> 8), std::uint8_t(value)};
    const unsigned more = value == 0 && code <= 1 ? 4 : value < 0x313c ? 0 : value < 0x393c ? 1 : 2;
    for (unsigned n = 0; n < more; ++n) bytes.push_back(extra >= 0 ? std::uint8_t(extra) : std::uint8_t(random()));
    return bytes;
}
constexpr std::uint32_t radix_edges[] = {0, 1, 2, 0x7f, 0x80, 0x897, 0x898, 0x899, 0x898 + 0x65, 0x898 + 0x66, 0x898 + 0x67, 0x313b, 0x313c, 0x313d, 0x393b, 0x393c, 0x393d, 0x3fff};
void varint(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    do {
        std::uint8_t byte = value & 0x7f;
        value >>= 7;
        if (value) byte |= 0x80;
        bytes.push_back(byte);
    } while (value);
}
void halfword(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(std::uint8_t(value >> 8));
    bytes.push_back(std::uint8_t(value));
}
// A glyph record and its arrays in the scratch area (a memory object at `mem`): `contours` contours over `points` points.
struct Shape {
    std::uint32_t at, block, contour_arrays;
    int contours, points;
    bool consistent = true;  // the last end point is points - 1
    bool inline_arrays = true;
    std::uint32_t cached = 0;  // the block is the memory object's cached one (the original's x block)
};
void build_glyph(std::vector<Write>& memory, const Shape& shape, std::uint32_t mem, std::mt19937& random) {
    const std::uint32_t slots = std::uint32_t(shape.points) + 4;
    const auto g = shape.at;
    memory.push_back(w32(g, mem));
    memory.push_back(w16(g + 4, std::uint32_t(shape.contours)));
    memory.push_back(w32(g + 8, std::uint32_t(shape.points)));
    memory.push_back(w16(g + 0x34, 2));
    memory.push_back(w16(g + 0x36, std::uint32_t(shape.contours)));
    memory.push_back(w16(g + 0x38, std::uint32_t(shape.points)));
    const bool inline_arrays = shape.inline_arrays && shape.contours <= 8;
    const auto starts = inline_arrays ? g + 0x14 : shape.contour_arrays, ends = inline_arrays ? g + 0x24 : shape.contour_arrays + 0x100;
    memory.push_back(w32(g + 0x3c, starts));
    memory.push_back(w32(g + 0x40, ends));
    const auto oox = shape.block, ooy = oox + slots * 4, xs = ooy + slots * 4, ys = xs + slots * 2, on_curve = ys + slots * 2;
    memory.push_back(w32(g + 0x44, xs));
    memory.push_back(w32(g + 0x48, ys));
    memory.push_back(w32(g + 0x4c, on_curve));
    memory.push_back(w32(g + 0x50, oox));
    memory.push_back(w32(g + 0x54, ooy));
    for (std::uint32_t n = 0; n < slots; ++n) {
        memory.push_back(w32(oox + n * 4, std::uint32_t(std::int32_t(random() % 0x4000) - 0x2000)));
        memory.push_back(w32(ooy + n * 4, std::uint32_t(std::int32_t(random() % 0x4000) - 0x2000)));
        memory.push_back(w16(xs + n * 2, random()));
        memory.push_back(w16(ys + n * 2, random()));
        memory.push_back(w8(on_curve + n, random() % 2));
    }
    // The contours split the points: each ends where the next starts less one; the last ends at points - 1.
    std::uint32_t start = 0;
    for (int n = 0; n < shape.contours; ++n) {
        const std::uint32_t remaining = std::uint32_t(shape.points) - start, left = std::uint32_t(shape.contours - n);
        std::uint32_t end = n + 1 == shape.contours ? std::uint32_t(shape.points) - 1 : start + (remaining > left ? (random() % (remaining - left + 1)) : 0);
        if (n + 1 == shape.contours && !shape.consistent) end += 2;
        memory.push_back(w16(starts + std::uint32_t(n) * 2, start));
        memory.push_back(w16(ends + std::uint32_t(n) * 2, end));
        start = end + 1;
    }
}
}

FONT_ENGINE_CASES("glyph") {
    std::mt19937 random(0x61795);
    const auto first_case = cases.size();
    const auto mem = mem_object(scenario);
    const auto toc = std::pair<unsigned, std::uint32_t>{2, game_toc};
    // ---- 0x1075b970: one compact point --------------------------------------------------------------------------------------
    {
        const auto add = [&](const std::vector<std::uint8_t>& bytes, std::uint32_t position, const std::string& what) {
            // The point sits `position` bytes into the data (a different start for the stream's position).
            std::vector<std::uint8_t> data(position, 0x5a);
            data.insert(data.end(), bytes.begin(), bytes.end());
            data.resize(std::max<std::size_t>(data.size(), 8), 0x33);
            Case item{.entry = read_point, .registers = {{3, stream_at}, {4, out_x}, {5, out_y}, toc}, .label = "glyph: point " + what};
            stream_over(item.memory, data, position);
            item.memory.push_back(w16(out_x, 0xaaaa));
            item.memory.push_back(w16(out_y, 0xbbbb));
            cases.push_back(item);
        };
        for (unsigned code = 0; code < 4; ++code)
            for (const auto value : radix_edges)
                for (const int extra : {0, 0xff, 0x80, 0x7f, 0x01, -1, -1})
                    add(compact_point(code, value, random, extra), 0, "code " + std::to_string(code) + " value " + hex(value) + " extra " + std::to_string(extra));
        for (unsigned n = 0; n < 1500; ++n) {
            const auto value = random() % 3 ? radix_edges[random() % std::size(radix_edges)] + random() % 3 : random() % 0x4000;
            add(compact_point(random() % 4, std::min<unsigned>(value, 0x3fff), random), random() % 3, "random " + std::to_string(n));
        }
        for (unsigned n = 0; n < 40; ++n) {
            std::vector<std::uint8_t> raw(8);
            for (auto& byte : raw) byte = std::uint8_t(random());
            add(raw, 0, "raw bytes " + std::to_string(n));
        }
    }
    // ---- 0x1075c830: the point arrays (the stack image is verified) --------------------------------------------------------
    for (const std::int32_t count : {-4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 100, 255, 256, 1000, 2000}) {
        const auto g = scratch_base + 0x40000;
        Case item{.entry = allocate_points, .registers = {{3, g}, {4, std::uint32_t(count)}, toc}, .memory = {w32(g, mem)}, .label = "glyph: points " + std::to_string(count)};
        item.verify = [](State& s) {
            // The frame the printf readers find: the caller's r29-r31 as saved by the prologue (the epilogue restored the registers from
            // these very words), the link register and the back chain.
            const auto sp = std::uint32_t(s.r<4>(guest::reg::gpr(1)));
            const auto word = [&](std::uint32_t at) { return std::uint32_t(s.memory.read<4>(at)); };
            const auto reg = [&](unsigned n) { return std::uint32_t(s.r<4>(guest::reg::gpr(n))); };
            return word(sp - 12) == reg(29) && word(sp - 8) == reg(30) && word(sp - 4) == reg(31) && word(sp + 8) == s.pc && word(sp - 0x50) == sp;
        };
        cases.push_back(item);
    }
    // ---- 0x1075bba0: a glyph without points ----------------------------------------------------------------------------------
    for (const std::uint32_t a : {0u, 1u, 0x7fffu, 0x8000u, 0xffffu, 0xffff8000u, 0x12345u})
        for (const std::uint32_t b : {0u, 1u, 0x7fffu, 0xffffffffu, 0x8000u, 0x10000u})
            for (const std::uint32_t d : {0u, 5u, 0xffffu, 0x80000000u})
                cases.push_back({.entry = new_empty, .registers = {{3, mem}, {5, a}, {6, b}, {7, d}, toc},
                                 .label = "glyph: empty " + hex(a) + " " + hex(b) + " " + hex(d)});
    // ---- 0x1075bc90: a glyph read from a stream -------------------------------------------------------------------------------
    {
        const auto add = [&](const std::vector<std::uint8_t>& bytes, const std::string& what, std::uint32_t lsb, std::uint32_t advance, std::uint32_t top, std::uint32_t vertical,
                             std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            Case item{.entry = new_glyph, .registers = {{3, mem}, {4, stream_at}, {6, lsb}, {7, advance}, {8, top}, {9, vertical}, toc}, .label = "glyph: new " + what};
            stream_over(item.memory, bytes);
            item.answers = std::move(answers);
            cases.push_back(item);
        };
        const auto simple = [&](unsigned contours, unsigned max_delta, bool edge_points) {
            std::vector<std::uint8_t> bytes;
            halfword(bytes, contours);
            unsigned points = 0;
            for (unsigned n = 0; n < contours; ++n) {
                const auto delta = random() % (max_delta + 1);
                varint(bytes, delta);
                points += delta + 1;
            }
            for (unsigned n = 0; n < points; ++n) {
                const auto value = edge_points ? radix_edges[random() % std::size(radix_edges)] : random() % 0x4000;
                const auto point = compact_point(random() % 4, value, random);
                bytes.insert(bytes.end(), point.begin(), point.end());
            }
            return bytes;
        };
        for (const unsigned contours : {1u, 2u, 3u, 7u, 8u, 9u, 10u, 12u})
            for (unsigned variant = 0; variant < 8; ++variant)
                add(simple(contours, variant % 4 == 0 ? 0 : 4, variant % 2), "simple " + std::to_string(contours) + " contours v" + std::to_string(variant),
                    random() % 100, random() % 1000, random() % 200, random() % 1500);
        for (const std::uint32_t lsb : {0u, 0xffffu, 0x8000u, 0xffff8000u})
            for (const std::uint32_t advance : {0u, 0x7fffu, 0xffffffffu})
                add(simple(2, 3, true), "simple metrics " + hex(lsb) + " " + hex(advance), lsb, advance, advance ^ 0x55, lsb ^ 0xaa);
        {   // Zero contours, and a negative-count composite with no components beyond the first.
            std::vector<std::uint8_t> zero;
            halfword(zero, 0);
            add(zero, "no contours", 3, 4, 5, 6);
        }
        // Composites.
        const auto composite = [&](unsigned components, bool negative_index, std::uint32_t force_flags, bool force) {
            std::vector<std::uint8_t> bytes;
            halfword(bytes, 0xffff);
            for (unsigned n = 0; n < components; ++n) {
                std::uint32_t flags = force_flags;
                if (!force) {
                    const std::uint32_t scales[] = {0, 8, 0x40, 0x80};
                    flags = (random() % 3 ? 1u : 0u) | scales[random() % 4] | (random() % 2 ? 2u : 0u) | (random() % 3 == 0 ? 4u : 0u) | (random() % 4 == 0 ? 0x200u : 0u);
                }
                if (n + 1 < components) flags |= 0x20; else flags &= ~0x20u;
                halfword(bytes, flags);
                halfword(bytes, negative_index && n == components - 1 ? 0x8000 : random() % 200);
                halfword(bytes, random());
                if (flags & 1) halfword(bytes, random());
                if (flags & 8) halfword(bytes, random());
                else if (flags & 0x40) { halfword(bytes, random()); halfword(bytes, random()); }
                else if (flags & 0x80) { for (int k = 0; k < 4; ++k) halfword(bytes, random()); }
            }
            return bytes;
        };
        for (const std::uint32_t flags : {0u, 1u, 8u, 0x40u, 0x80u, 1u | 8u, 1u | 0x40u, 1u | 0x80u, 8u | 0x40u, 0x40u | 0x80u, 0x202u, 0x2u | 0x4u | 0x1u})
            add(composite(1, false, flags, true), "composite one component, flags " + hex(flags), 1, 2, 3, 4);
        for (unsigned n = 0; n < 40; ++n)
            add(composite(1 + random() % 6, false, 0, false), "composite random " + std::to_string(n), random() % 50, random() % 900, random() % 30, random() % 800);
        add(composite(3, true, 0, false), "composite negative glyph index (assertion answered)", 1, 2, 3, 4, {{reporter, 0}});
        add(composite(2, true, 0, false), "composite negative glyph index (real reporter)", 1, 2, 3, 4);
        // The buffer: 0x400 words, grown by half when a component starts within ten words of the end.
        add(composite(200, false, 1, true), "composite 200 components (no growth yet)", 1, 2, 3, 4);
        add(composite(300, false, 1, true), "composite 300 components (one growth)", 1, 2, 3, 4);
        add(composite(300, false, 0, false), "composite 300 random components", 1, 2, 3, 4);
        add(composite(700, false, 1, true), "composite 700 components (two growths)", 1, 2, 3, 4);
        add(composite(300, false, 1, true), "composite 300 components, reallocation answered 0 (assertion answered)", 1, 2, 3, 4, {{reallocate, 0}, {reporter, 0}});
    }
    // ---- 0x1075c8c0: a glyph's release over a memory object the case builds -------------------------------------------------------
    {
        // The fake memory object: its cached blocks at +0x128.. (slots 0-5) and the free flags at +0x158..: the original's reads and writes.
        for (unsigned variant = 0; variant < 64; ++variant) {
            const bool x_cached = variant & 1, inline_arrays = variant & 2, instructions_cached = variant & 4, record_cached = variant & 8,
                       has_components = variant & 16, has_instructions = variant & 32;
            std::vector<Write> memory;
            Shape shape{destination_at, destination_block, destination_contours, 2, 3, true, inline_arrays};
            build_glyph(memory, shape, fake_mem, random);
            memory.push_back(w32(fake_mem + 0x128, record_cached ? destination_at : 0x70000008));
            memory.push_back(w32(fake_mem + 0x12c, x_cached ? destination_block : 0x70000010));
            memory.push_back(w32(fake_mem + 0x134, instructions_cached ? instruction_block : 0x70000018));
            memory.push_back(w32(destination_at + 0x64, has_instructions ? instruction_block : 0));
            memory.push_back(w32(destination_at + 0x58, has_components ? component_block : 0));
            Case item{.entry = delete_glyph, .registers = {{3, destination_at}, {4, 0x1111}, {5, 0x2222}, {6, 0x3333}, {7, 0x4444}, {9, 0x5555}, {10, 0x6666}, toc},
                      .memory = memory, .label = "glyph: delete variant " + std::to_string(variant)};
            item.answers = {{release, 0x7777}};
            cases.push_back(item);
        }
        cases.push_back({.entry = delete_glyph, .registers = {{3, 0}, {4, 0x1111}, {9, 0x5555}, {10, 0x6666}, toc}, .label = "glyph: delete null"});
    }
    // ---- 0x1075c0f0: a component added to a glyph --------------------------------------------------------------------------------
    {
        const auto base = [&](std::vector<Write>& memory, int destination_points, int destination_contours_count, int source_points, int source_contours_count, bool consistent,
                              bool destination_cached, bool inline_arrays, bool with_destination) {
            if (with_destination) {
                Shape destination{destination_at, destination_block, destination_contours, destination_contours_count, destination_points, consistent, inline_arrays};
                build_glyph(memory, destination, fake_mem, random);
                memory.push_back(w32(slot_at, destination_at));
            }
            Shape source{source_at, source_block, source_contours, source_contours_count, source_points};
            build_glyph(memory, source, fake_mem, random);
            memory.push_back(w32(fake_mem + 0x12c, destination_cached ? destination_block : 0x70000010));
            memory.push_back(w32(fake_mem + 0x15c, 0));
        };
        const auto make = [&](std::uint32_t flags, std::uint32_t dx, std::uint32_t dy, std::vector<Write> memory, const std::string& label, bool null_source = false) {
            Case item{.entry = append_entry, .registers = {{3, slot_at}, {4, null_source ? 0u : source_at}, {5, flags}, {6, dx}, {7, dy}, {9, 0x5555}, {10, 0x6666}, toc},
                      .memory = std::move(memory), .label = "glyph: append " + label};
            item.answers = {{release, 0x7777}, {reporter, 0}};
            item.nth_answers = {{allocate, 1, buffer_a}, {allocate, 2, buffer_b}};
            cases.push_back(item);
        };
        const std::uint32_t offsets[] = {0, 5, 0x1f, 0x20, 0x21, 0x3f, 0x40, 0xffffffe0u, 0xffffffdfu, 0x12345, 0x7fffffffu, 0xffffffffu, 0x80000000u};
        // The first component: nothing to merge into.
        for (const std::uint32_t flags : {0u, 2u, 4u, 6u, 0x200u, 0x202u, 0x206u, 0x1fffeu})
            for (const auto dx : offsets)
                for (const auto dy : {0u, 0x21u, 0xffffffe0u, 0x7fffffffu}) {
                    if (flags == 0x1fffeu && dx != 0x21) continue;
                    std::vector<Write> memory;
                    base(memory, 0, 0, 1 + int(random() % 9), 1, true, false, true, false);
                    memory.push_back(w32(slot_at, 0));
                    make(flags, dx, dy, memory, "first, flags " + hex(flags) + " offset " + hex(dx) + " " + hex(dy));
                }
        {   // A first component without points (the count is not positive), and the null source (assertion answered).
            std::vector<Write> memory;
            base(memory, 0, 0, 0, 0, true, false, true, false);
            memory.push_back(w32(slot_at, 0));
            make(2, 40, 41, memory, "first, no points");
            std::vector<Write> null_source;
            null_source.push_back(w32(slot_at, 0));
            make(2, 40, 41, null_source, "first, null source (assertion)", true);
        }
        // Merging: counts of points and contours of both, every flag combination.
        for (const std::uint32_t flags : {2u, 6u, 0x202u, 0x206u, 0u, 0x200u, 0x4u, 0x20u})
            for (const int destination_points : {1, 2, 3, 8, 9, 17})
                for (const int source_points : {0, 1, 4, 8, 9, 17}) {
                    const int destination_contours_count = 1 + int(random() % std::min(3, destination_points));
                    const int source_contours_count = source_points == 0 ? 0 : 1 + int(random() % std::min(3, source_points));
                    for (unsigned variant = 0; variant < 3; ++variant) {
                        std::vector<Write> memory;
                        base(memory, destination_points, destination_contours_count, source_points, source_contours_count, true, variant & 1, variant != 1, true);
                        const bool matching = !(flags & 2);
                        const std::uint32_t dx = matching ? random() % std::uint32_t(destination_points + 4) : offsets[random() % std::size(offsets)];
                        const std::uint32_t dy = matching ? random() % std::uint32_t(source_points + 4) : offsets[random() % std::size(offsets)];
                        make(flags, dx, dy, memory, "merge " + std::to_string(destination_points) + "+" + std::to_string(source_points) + " flags " + hex(flags) +
                                                      " v" + std::to_string(variant));
                    }
                }
        {   // The destination whose last end point is not its last point (the assertion), the allocated contour arrays.
            for (const bool answered : {true, false}) {
                std::vector<Write> memory;
                base(memory, 5, 2, 3, 1, false, true, true, true);
                Case item{.entry = append_entry, .registers = {{3, slot_at}, {4, source_at}, {5, 2}, {6, 7}, {7, 9}, toc}, .memory = memory,
                          .label = std::string("glyph: append inconsistent destination ") + (answered ? "answered" : "real reporter")};
                item.nth_answers = {{allocate, 1, buffer_a}, {allocate, 2, buffer_b}};
                item.answers = {{release, 0x7777}};
                if (answered) item.answers.push_back({reporter, 0});
                cases.push_back(item);
            }
            std::vector<Write> memory;
            base(memory, 12, 10, 12, 9, true, false, false, true);
            make(2, 3, 4, memory, "merge with 10 + 9 contours (allocated arrays)");
        }
    }
    // The call trace pairs the calls of an entry's first 32 cases (then every 64th): the cases that reach the rarer call sites (the
    // assertions, the reallocation, the allocated contour arrays, the release of the old blocks) go first.
    const auto rare = [](const Case& item) {
        for (const char* word : {"assertion", "reallocation", "300 components", "allocated", "inconsistent", "negative", "variant 1", "variant 2", "reporter", "10 contours", "merge 9+9"})
            if (item.label.find(word) != std::string::npos) return true;
        return false;
    };
    std::stable_partition(cases.begin() + std::ptrdiff_t(first_case), cases.end(), rare);
}
}
