// The font engine, block `t2kextra` (T2KEXTRA.C, 0x10761070..0x10761270): the colour table rendering the character renderer
// (0x10760e30) runs for command bit 0x80, which the game never sets (it asks for cmd = 2): the 8-bit coverage bitmap of a rendered glyph
// becomes a wider bitmap with a five-tap filter spread over each pixel's neighbours (the usual sub-pixel colour fringe filter), the
// record's left and one more field moved by 0x80, its width and row stride grown by four. Dormant for the game: cases only.
//
// The old bitmap is given back: if it is the memory object's scratch buffer of slot 5 (pointer at +0x13c) that slot is marked free
// (flag +0x16c = 1), otherwise tsi_DeAllocMem releases it (the stack-contract leaver, at this routine's own frame's sp with r24-r31 as
// the original holds them). tsi_AllocMem never returns null (it raises), so the assertion after it is not reachable in the original
// either; it is written as the original's.
#include "font_engine_detail.hpp"
#include <array>

namespace cm::aot::font_engine {
void render_color_table(Cpu& c, std::uint32_t t2k) {
    using namespace extra;
    guest::Frame frame(c, 0x10761070, 0x60, {.first_gpr = 24});
    // r3-r10 as the original leaves them (the routine returns to generated code, 0x10760e30 and FONT.CPP, with r4-r10 unwritten):
    // v[n] shadows rn and is stored into the registers at the calls and the return.
    std::array<std::uint32_t, 11> v{};
    const auto store = [&] { for (unsigned n = 3; n <= 10; ++n) c.set(n, v[n]); };
    const auto load = [&] { for (unsigned n = 3; n <= 10; ++n) v[n] = c.r(n); };
    load();
    const auto pool = c.toc(toc::t2kextra_pool);
    c.set({{25, t2k}, {24, pool}});
    const auto left = c.u32(t2k + left_at);
    const auto width = c.u32(t2k + width_at), height = c.u32(t2k + height_at), old_stride = c.u32(t2k + stride_at);
    const auto stride = width + 4;
    const auto size = stride * height;
    c.put32(t2k + left_at, left - 0x80);
    c.put32(t2k + margin_at, c.u32(t2k + margin_at) - 0x80);
    c.put32(t2k + stride_at, stride);
    c.put32(t2k + width_at, stride);
    const auto mem = c.u32(t2k + mem_at);
    // The held registers at the calls: r27 the old row stride, r28 the height, r29 the new stride, r30 the old width, r31 the new size.
    c.set({{27, old_stride}, {28, height}, {29, stride}, {30, width}, {31, size}});
    v[3] = mem; v[4] = size;
    store();
    c.link(0x107610c8);
    const auto bitmap = allocate_block(c, mem, size);
    c.set(26, bitmap);
    load();
    if (bitmap == 0) {
        c.set({{3, pool + 0x80}, {4, pool + 8}, {5, 0xd2}});
        c.call(assertion_report, 0x107610e0);
        load();
    }
    v[6] = bitmap; v[5] = 0;
    if (std::int32_t(size) > 0) {
        for (std::uint32_t n = 0; n < size; ++n) c.put8(bitmap + n, 0);
        // The zero fill is unrolled by eight: its last pass leaves r5 = 8 * passes, the byte loop after it r3 = the end, r4 = 0.
        v[5] = size > 8 ? 8 * ((size - 1) >> 3) : 0;
        v[4] = 0;
        v[3] = bitmap + size;
    }
    // Each nonzero source pixel v adds its filter taps to the five bytes from its own position: (v>>3)-(v>>6), then
    // (v+1)/2 - (v+4)/8 - (v+15)/32 - v/64 less the first tap, the same without it, the second again less the first, and the first.
    // r24 and r31 are the scratch registers of that code: they keep the last nonzero pixel's v/64 and the last pixel read.
    auto source = c.u32(t2k + bitmap_at);
    auto destination = bitmap;
    std::uint32_t last_pixel = size, last_sixty_fourth = pool;
    v[5] = 0;  // li r5,0
    for (std::int32_t row = 0; row < std::int32_t(height); ++row) {
        auto pixel = source;
        v[8] = source;
        v[4] = 0;
        if (std::int32_t(width) > 0)
            for (std::uint32_t x = 0; x < width; ++x, ++pixel) {
                const std::uint32_t value = c.u8(pixel);
                last_pixel = value;
                if (value != 0) {
                    const std::uint32_t sixty_fourth = value >> 6, first = (value >> 3) - sixty_fourth;
                    const std::uint32_t second = ((value + 1) >> 1) - ((value + 4) >> 3) - ((value + 0xf) >> 5) - sixty_fourth;
                    last_sixty_fourth = sixty_fourth;
                    const auto at = destination + x;
                    c.put8(at, first + c.u8(at));
                    c.put8(at + 1, second + c.u8(at + 1) - first);
                    c.put8(at + 2, second + c.u8(at + 2));
                    c.put8(at + 3, second + c.u8(at + 3) - first);
                    const auto fifth = c.u8(at + 4);
                    c.put8(at + 4, first + fifth);
                    v[3] = at; v[7] = first + fifth; v[9] = fifth; v[10] = second;
                }
                v[4] = x + 1;
                v[8] = pixel + 1;
            }
        source += old_stride;
        destination += stride;
        ++v[5];
        v[6] = destination;
    }
    const auto owner = c.u32(t2k + mem_at), old_bitmap = c.u32(t2k + bitmap_at);
    v[3] = owner; v[4] = old_bitmap;
    if (old_bitmap == c.u32(owner + tsimem::scratch_pointer + 4 * 5)) {
        c.put32(owner + tsimem::scratch_free + 4 * 5, 1);
        store();
    } else {
        c.set({{24, last_sixty_fourth}, {31, last_pixel}});
        store();
        c.link(0x10761250);
        release_block(c, owner, old_bitmap);
    }
    c.put32(t2k + bitmap_at, bitmap);
}
}
