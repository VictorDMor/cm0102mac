// graph_carbon.cpp, block `colours` (0x1076b830, 0x1076b6e0, 0x1076b7e0, 0x1076b020, 0x1076b140, 0x1076b240, 0x1076b2f0, 0x1076b450;
// 688 instructions): the colour variables and the bitmaps.
//
// A bitmap is a 0x30-byte record (width, height, byte size, the pixels' address, then 0x20 bytes of metadata) with its 15-bit pixels in a
// block of its own (graph_carbon_colours_detail.hpp, `bitmap`). 0x1076b6e0 clones a record (or hands back the one it is given when that
// one has the same width and height), 0x1076b7e0 frees one, 0x1076b020 copies a rectangle of the surface into one and 0x1076b140 copies
// one back onto the surface. A `.rgm` region file is a record followed by its pixels: 0x1076b240 writes one, 0x1076b2f0 loads one (the
// cached `.rgm` next to a bitmap file, or else the bitmap file itself, which it converts and caches), 0x1076b450 reads a bitmap file
// (width, height and size little-endian, a format word, 0x20 bytes of metadata, the pixels; a format word of 0x7e0 means 5-6-5, which is
// converted to the surface's 5-5-5, and any other format has its halfwords byte-swapped).
//
// Every routine builds the original's frame and calls the rest of the program (the allocator, the file layer, the byte move) through the
// dispatcher at the original's return addresses (bl + 4) with the registers the original holds there: r3-r10 are compared in full at every
// return, so the values the original sets and the callees pass through are kept (r0, r11 and r12 are diagnostics). The routines of this
// block call each other directly. The clone copies the 0x10-0x2f bytes of the record it is given; 0x1076b020 builds that record on its own
// frame and fills only width, height and size, so the other bytes are what the frame's earlier owners left there (stack contract: the
// brief's 8,973 stale reads at 0x1076b760): nothing here stores to them.
//
// The two native drawing observation points inside this block (0x1076b138, the end of 0x1076b020, and 0x1076b234, the end of 0x1076b140)
// are passed once after the epilogue, as the generated page does.
#include "graph_carbon_colours_detail.hpp"
#include "native_drawing.hpp"

namespace cm::aot::graph_carbon {
namespace {
// Routines of the rest of the program that this block calls (the dispatcher runs the generated body or its C++).
namespace callee {
inline constexpr std::uint32_t allocate = 0x10775130,   // new[]: a block of r3 bytes, or 0
    release = 0x10775170,                               // delete[]
    move_bytes = 0x10739ef0,                            // r3 = destination, r4 = source, r5 = count (a halfword here)
    open_file = 0x10739de0,                             // r3 = path, r4 = mode text: the stream or 0
    read_file = 0x10775fe0,                             // fread(buffer, size, count, stream)
    write_file = 0x10776360,                            // fwrite(buffer, size, count, stream)
    close_file = 0x107766f0,                            // fclose(stream)
    region_path = 0x10746470,                           // r3 = a bitmap file's path, r4 = buffer, r5 = extension: the path of its cache
    format_text = 0x1077b720,                           // sprintf(buffer, format, ...)
    block_move = 0x1077fc28,                            // the Toolbox's BlockMoveData(source, destination, count)
    swap_halfwords = 0x1074d5a0;                        // r3 = halfwords, r4 = count: each halfword byte-swapped in place
}
// The pool's texts (offsets from the pool the TOC slot -0x3bd0 holds): "graph_carbon.cpp" at 0, the assertion's format and title, "wb",
// ".rgm", "rb" and "converted region: %s".
namespace pool_text {
inline constexpr std::uint32_t assertion_format = 0x11, assertion_title = 0x1e, mode_write = 0x75, region_extension = 0x78, mode_read = 0x7d,
                               converted = 0x80;
}
// The slots the routines load: the shared message buffer and success flag of the assertion reports, the buffer of the conversion
// message, and a four-byte scratch variable (BlockMoveData's destination for the format word).
namespace slot {
inline constexpr std::int32_t message_buffer = -0x7ba8, success_flag = -0x7bac, conversion_message = -0x7bb8, format_scratch = -0x5afc;
}
// The format word of a bitmap file: the metadata's bytes 0x14-0x17, a little-endian 0x7e0 for 5-6-5.
inline constexpr std::uint32_t format_offset = 0x14, format_565 = 0x7e0;
// The source line of the assertion in the blit (0x1076b140) and the address of its first call.
inline constexpr std::uint32_t blit_assertion_line = 0x713, blit_assertion_at = 0x1076b1e4;

// `bl target` into a routine of this block that is called directly: the arguments and the link register as the original's call sets them;
// the checker's call trace is told (the original's generated call is a dispatch it records).
void direct_call(Cpu& c, std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
    enter(c, back, arguments);
    if (graph_carbon_direct_call_probe) graph_carbon_direct_call_probe(c.s, target);
}
// `lwz r2,0x14(r1)` after a glue call: the TOC the glue saved.
void reload_toc(Cpu& c) { c.set(2, c.u32(c.sp() + 0x14)); }
constexpr std::uint32_t extend16(std::uint32_t value) { return extended(std::int16_t(value)); }
constexpr std::uint32_t byte_swapped(std::uint32_t value) {
    return (value << 24) | ((value & 0xff00) << 8) | ((value >> 8) & 0xff00) | (value >> 24);
}

// The colour variables 0x1076b830 sets, each a 15-bit pixel at the address its TOC slot holds.
struct Colour { std::int32_t slot; std::uint8_t red, green, blue; };
constexpr Colour colours[] = {
    {-0x7bd4, 0x00, 0x00, 0x00}, {-0x7a8c, 0xff, 0xff, 0xff}, {-0x7bc8, 0xff, 0xff, 0xff}, {-0x7a74, 0xff, 0x00, 0x00},
    {-0x7a80, 0x80, 0x00, 0x00}, {-0x7bf4, 0xff, 0x80, 0x00}, {-0x7a84, 0x80, 0x40, 0x00}, {-0x7bd0, 0xff, 0xff, 0x00},
    {-0x5b00, 0xff, 0xff, 0x80}, {-0x7bcc, 0x00, 0x80, 0x00}, {-0x7a78, 0x00, 0x40, 0x00}, {-0x7a90, 0x00, 0xff, 0xff},
    {-0x7bd8, 0x80, 0xff, 0xff}, {-0x7b14, 0x00, 0x00, 0xff}, {-0x7a94, 0x00, 0x80, 0xff}, {-0x7bdc, 0x00, 0x00, 0x80},
    {-0x6224, 0x00, 0x00, 0x60}, {-0x5b04, 0x80, 0x00, 0x80}, {-0x7bec, 0x40, 0x00, 0x40}, {-0x5b08, 0x80, 0x40, 0x40},
    {-0x7bc4, 0x80, 0x80, 0x80}, {-0x5b0c, 0xe0, 0xe0, 0xe0}, {-0x7b10, 0xc0, 0xc0, 0xc0}, {-0x7a88, 0x40, 0x40, 0x40},
    {-0x5b10, 0x00, 0x80, 0x80},
};
// The surface's row stride in bytes... as 0x1076b020 reads it: the signed halfword of row bytes halved towards zero, as a halfword.
constexpr std::uint32_t half_of(std::uint32_t row_bytes) {
    const auto rounded = std::int32_t(row_bytes) + std::int32_t((std::uint32_t(row_bytes) >> 31) & 1);
    return extend16(std::uint32_t(rounded >> 1));
}
}

void colour_variables(Cpu& c) {
    Frame frame(c, 0x1076b830, 0x40, {});
    if (compare(c, std::int8_t(c.u8(small(c, surface_data::setting_up)))) != 0) return;
    // Each pixel is packed by 0x1076bb20 (a leaf that writes r3 only and leaves r4-r6 as they were set) and stored where its slot points.
    // What the original ends with is the last colour's: r3 its pixel, r4 its variable's address, r5 its blue, r6 0 and r7 the address of
    // the variable before.
    std::uint32_t previous = 0, pixel = 0, variable = 0;
    for (const auto& colour : colours) {
        previous = variable;
        variable = c.toc(colour.slot);
        pixel = pack_pixel(colour.red, colour.green, colour.blue);
        c.put16(variable, pixel);
    }
    c.set({{3, pixel}, {4, variable}, {5, colours[std::size(colours) - 1].blue}, {6, 0}, {7, previous}});
}

std::uint32_t clone_bitmap(Cpu& c, std::uint32_t source, std::uint32_t reuse) {
    Frame frame(c, 0x1076b6e0, 0x40, {.first_gpr = 30});
    nonzero_logical(c, reuse);
    c.set(30, source);
    if (reuse != 0) {
        // A record of the same width and height is handed back as it is.
        const auto width = c.u32(reuse);
        c.set(3, width);
        compare(c, std::int32_t(width), std::int32_t(c.u32(source)));
        if (width == c.u32(source)) {
            const auto height = c.u32(reuse + 4);
            c.set(3, height);
            compare(c, std::int32_t(height), std::int32_t(c.u32(source + 4)));
            if (height == c.u32(source + 4)) return result(c, reuse);
        }
    }
    c.set(3, c.u32(source + bitmap::bytes));
    c.call(callee::allocate, 0x1076b730);
    const auto pixels = c.r(3);
    c.set(31, pixels);
    if (!nonzero(c, std::int32_t(pixels))) return result(c, 0);
    c.set(3, bitmap::size);
    c.call(callee::allocate, 0x1076b748);
    const auto record = c.r(3);
    if (!nonzero_logical(c, record)) {
        c.set(3, pixels);
        c.call(callee::release, 0x1076b758);
        return result(c, 0);
    }
    // The record word by word as the original loads and stores it (the metadata from 0x10 is whatever the source holds), then the new
    // pixel block's address over the copied one. The copy of the last pair leaves its first word in r4.
    for (std::uint32_t at = 0; at < bitmap::metadata; at += 4) c.put32(record + at, c.u32(source + at));
    std::uint32_t first = 0;
    for (std::uint32_t at = bitmap::metadata; at < bitmap::size; at += 8) {
        first = c.u32(source + at);
        const auto second = c.u32(source + at + 4);
        c.put32(record + at, first);
        c.put32(record + at + 4, second);
    }
    c.set(4, first);
    c.put32(record + bitmap::pixels, pixels);
    return result(c, record);
}

void free_bitmap(Cpu& c, std::uint32_t pointer) {
    Frame frame(c, 0x1076b7e0, 0x40, {.first_gpr = 31});
    c.set(31, pointer);
    if (!nonzero(c, std::int32_t(pointer))) return;
    const auto pixels = c.u32(pointer + bitmap::pixels);
    c.set(3, pixels);
    if (nonzero_logical(c, pixels)) c.call(callee::release, 0x1076b808);
    c.set(3, pointer);
    c.call(callee::release, 0x1076b810);
}

std::uint32_t capture_rectangle(Cpu& c, std::uint32_t x1, std::uint32_t y1, std::uint32_t x2, std::uint32_t y2, std::uint32_t reuse) {
    {
        Frame frame(c, 0x1076b020, 0x90, {.first_gpr = 25});
        // The corners: the lesser coordinates (r8 and r0, then r31 and r30 as halfwords) and the greater ones (r5 and r6).
        std::uint32_t lesser_x = x2, lesser_y = y2, greater_x = x2, greater_y = y2;
        if (compare(c, std::int32_t(x1), std::int32_t(x2)) < 0) lesser_x = x1;
        if (compare(c, std::int32_t(y1), std::int32_t(y2)) < 0) lesser_y = y1;
        const auto left = extend16(lesser_x), top = extend16(lesser_y);
        if (compare(c, std::int32_t(x1), std::int32_t(x2)) > 0) greater_x = x1;
        if (compare(c, std::int32_t(y1), std::int32_t(y2)) > 0) greater_y = y1;
        const auto width_word = extend16(greater_x) + 1 - left, height_word = extend16(greater_y) + 1 - top;
        const auto width = extend16(width_word), height = extend16(height_word);
        // The clone's source is a record on this frame of which only width, height and size are written (twice the pixel count).
        const auto source = frame.at(0x38);
        c.put32(source, width);
        c.put32(source + 4, height);
        c.put32(source + 8, width * height * 2);
        keep(c, {{25, width}, {26, height}, {30, top}, {31, left}});
        c.set({{5, width_word}, {6, greater_y}, {8, lesser_x}});
        direct_call(c, 0x1076b6e0, 0x1076b0b0, {source, reuse});
        const auto bitmap_record = clone_bitmap(c, source, reuse);
        keep(c, {{29, bitmap_record}});
        if (nonzero(c, std::int32_t(bitmap_record))) {
            // The rows are copied from the surface into the bitmap, a row at a time (a halfword's worth of bytes: the tests of the row
            // count and of the byte count are on halfwords).
            const auto stride_pixels = half_of(extend16(c.u16(small(c, surface_data::row_bytes))));
            const auto surface = c.u32(small(c, surface_data::base));
            const auto bitmap_row = width << 1, surface_row = stride_pixels << 1, count = extend16(bitmap_row);
            std::uint32_t target = c.u32(bitmap_record + bitmap::pixels), at = surface + (left << 1) + ((top * stride_pixels) << 1), rows = height;
            c.set({{4, surface}, {5, stride_pixels}});
            while (compare(c, std::int16_t(rows), 0) != 0) {
                rows -= 1;
                keep(c, {{25, rows}, {26, at}, {27, target}, {28, bitmap_row}, {29, bitmap_record}, {30, surface_row}, {31, count}});
                c.set({{3, target}, {4, at}, {5, count}});
                c.call(callee::move_bytes, 0x1076b110);
                at += surface_row;
                target += bitmap_row;
            }
        }
        c.set(3, bitmap_record);
    }
    native_drawing_boundary(c.s, 0x1076b138);
    return c.r(3);
}

void blit_bitmap(Cpu& c, std::uint32_t x, std::uint32_t y, std::uint32_t bitmap_record) {
    {
        Frame frame(c, 0x1076b140, 0x80, {.first_gpr = 26});
        const auto pool = c.toc(toc::pool), buffer = c.toc(slot::message_buffer);
        c.set({{31, buffer}, {30, pool}});
        if (nonzero_logical(c, bitmap_record)) {
            // The bitmap's rows are copied onto the surface at (x, y).
            const auto stride_pixels = extended(std::int16_t(c.u16(small(c, surface_data::stride_pixels))));
            const auto rows_down = extend16(y) * stride_pixels;
            const auto width = c.u32(bitmap_record + bitmap::width), height = c.u32(bitmap_record + bitmap::height);
            const auto width_halfword = extend16(width), surface = c.u32(small(c, surface_data::base));
            const auto bitmap_row = width_halfword << 1, surface_row = stride_pixels << 1;
            std::uint32_t source = c.u32(bitmap_record + bitmap::pixels), at = surface + (extend16(x) << 1) + (rows_down << 1), rows = extend16(height);
            const auto count = extend16(bitmap_row);
            c.set({{3, extend16(x) << 1}, {4, surface}, {6, height}, {7, width_halfword}, {8, stride_pixels}});
            while (compare(c, std::int16_t(rows), 0) != 0) {
                rows -= 1;
                keep(c, {{26, rows}, {27, source}, {28, at}, {29, count}, {30, bitmap_row}, {31, surface_row}});
                c.set({{3, at}, {4, source}, {5, count}});
                c.call(callee::move_bytes, 0x1076b1b8);
                source += bitmap_row;
                at += surface_row;
            }
        } else {
            // No bitmap: the assertion report (0x10739890 splits the file name into the two buffers on this frame, 0x10768810 gives the
            // version, 0x1077b720 formats the message, 0x10769b60 shows it), then the shared success flag cleared.
            report_assertion(c, {blit_assertion_at, pool, pool + pool_text::assertion_format, pool + pool_text::assertion_title, frame.at(0x40),
                                 frame.at(0x38), blit_assertion_line});
            c.set(3, c.toc(slot::success_flag));
        }
    }
    native_drawing_boundary(c.s, 0x1076b234);
}

void save_region(Cpu& c, std::uint32_t bitmap_record, std::uint32_t path) {
    Frame frame(c, 0x1076b240, 0x40, {.first_gpr = 30});
    c.set(30, bitmap_record);
    if (compare(c, std::int8_t(c.u8(small(c, surface_data::setting_up)))) != 0) return;
    if (!nonzero_logical(c, bitmap_record) || !nonzero_logical(c, path)) return;
    // The record (0x30 bytes, as it stands in memory: big-endian words) and then its pixels.
    const auto pool = c.toc(toc::pool);
    c.set({{5, pool}, {3, path}, {4, pool + pool_text::mode_write}});
    c.call(callee::open_file, 0x1076b284);
    const auto stream = c.r(3);
    c.set(31, stream);
    if (!nonzero(c, std::int32_t(stream))) return;
    c.set({{3, bitmap_record}, {6, stream}, {4, bitmap::size}, {5, 1}});
    c.call(callee::write_file, 0x1076b2a0);
    if (compare_logical(c, c.r(3), 1) != 0) {
        c.set(3, stream);
        c.call(callee::close_file, 0x1076b2b0);
        return;
    }
    c.set({{3, c.u32(bitmap_record + bitmap::pixels)}, {6, stream}, {4, c.u32(bitmap_record + bitmap::bytes)}, {5, 1}});
    c.call(callee::write_file, 0x1076b2c8);
    c.set(3, stream);
    c.call(callee::close_file, 0x1076b2d0);
}

std::uint32_t read_bitmap(Cpu& c, std::uint32_t file, std::uint32_t reuse) {
    Frame frame(c, 0x1076b450, 0x80, {.first_gpr = 28});
    const auto scratch = c.toc(slot::format_scratch);
    keep(c, {{31, file}, {30, scratch}, {28, reuse}, {29, reuse}});
    const auto record = frame.at(0x38);
    std::uint32_t bitmap_record = reuse;
    // After a failed read, a failed clone or a failed pixel read: the stream closed, the clone's two blocks given back.
    const auto fail = [&] {
        if (nonzero_logical(c, file)) {
            c.set(3, file);
            c.call(callee::close_file, 0x1076b688);
        }
        if (nonzero_logical(c, bitmap_record) && compare_logical(c, bitmap_record, reuse) != 0) {
            const auto pixels = c.u32(bitmap_record + bitmap::pixels);
            c.set(3, pixels);
            if (nonzero_logical(c, pixels)) c.call(callee::release, 0x1076b6a8);
            c.set(3, bitmap_record);
            c.call(callee::release, 0x1076b6b0);
        }
        return result(c, 0);
    };
    // The header: width, height, size, the format word, then 0x20 bytes of metadata. Each read must return one item.
    struct Part { std::uint32_t offset, size, back; };
    constexpr Part parts[] = {{0, 4, 0x1076b490}, {4, 4, 0x1076b4ac}, {8, 4, 0x1076b4c8}, {0xc, 4, 0x1076b4e4}, {0x10, 0x20, 0x1076b500}};
    for (const auto& part : parts) {
        c.set({{3, record + part.offset}, {6, file}, {4, part.size}, {5, 1}});
        c.call(callee::read_file, part.back);
        if (compare_logical(c, c.r(3), 1) != 0) return fail();
    }
    // The three words are little-endian in the file; the record keeps them as the guest does.
    const auto raw_width = c.u32(record), raw_height = c.u32(record + 4), raw_size = c.u32(record + 8);
    c.set({{7, raw_width}, {6, record}, {8, raw_height}, {5, record + 4}, {9, raw_size}});
    c.put32(record, byte_swapped(raw_width));
    c.put32(record + 4, byte_swapped(raw_height));
    c.put32(record + 8, byte_swapped(raw_size));
    direct_call(c, 0x1076b6e0, 0x1076b538, {record, reuse});
    bitmap_record = clone_bitmap(c, record, reuse);
    c.set(29, bitmap_record);
    if (!nonzero_logical(c, bitmap_record)) return fail();
    c.set({{3, c.u32(bitmap_record + bitmap::pixels)}, {6, file}, {4, c.u32(bitmap_record + bitmap::bytes)}, {5, 1}});
    c.call(callee::read_file, 0x1076b554);
    if (compare_logical(c, c.r(3), 1) != 0) return fail();
    // The format word: four bytes of the metadata copied through BlockMoveData into the scratch variable.
    c.set({{4, scratch}, {3, record + 0x10 + format_offset}, {5, 4}});
    c.call(callee::block_move, 0x1076b56c);
    reload_toc(c);
    const auto format = c.u32(scratch);
    c.set(3, format);
    const auto pixels = c.u32(bitmap_record + bitmap::pixels);
    if (compare_logical(c, byte_swapped(format), format_565) == 0) {
        // 5-6-5 to 5-5-5 over width x height halfwords: the file's byte order swapped, red and blue kept, green's top five bits.
        const auto count = c.u32(bitmap_record + bitmap::height) * c.u32(bitmap_record + bitmap::width);
        c.set(7, pixels);
        if (compare(c, std::int32_t(count)) != 0) {
            std::uint32_t r4 = 0, r5 = 0, r6 = 0, at = pixels;
            for (std::uint32_t n = 0; n < count; ++n, at += 2) {
                const auto half = c.u16(at);
                r6 = ((half << 8) | (half >> 8)) & 0xffff;
                r5 = (r6 & 0xf800) >> 8;
                r4 = r6 & 0x1f;
                c.put16(at, ((r6 >> 1) & 0x3e0) | r4 | ((r5 << 7) & 0x7c00));
            }
            c.set({{4, r4}, {5, r5}, {6, r6}, {7, at}});
        }
    } else {
        c.set({{3, pixels}, {4, c.u32(bitmap_record + bitmap::height) * c.u32(bitmap_record + bitmap::width)}});
        c.call(callee::swap_halfwords, 0x1076b668);
    }
    c.set(3, file);
    c.call(callee::close_file, 0x1076b670);
    return result(c, bitmap_record);
}

std::uint32_t load_region(Cpu& c, std::uint32_t path, std::uint32_t reuse) {
    Frame frame(c, 0x1076b2f0, 0x180, {.first_gpr = 26});
    const auto pool = c.toc(toc::pool);
    keep(c, {{27, reuse}, {31, pool}, {26, path}, {30, 0}, {29, reuse}});
    if (compare(c, std::int8_t(c.u8(small(c, surface_data::setting_up)))) != 0) return result(c, 0);
    const auto cache_path = frame.at(0x68), record = frame.at(0x38);
    std::uint32_t stream = 0, bitmap_record = reuse;
    bool from_file = false;
    // The failure tail: the stream closed, a clone of this call given back.
    const auto fail = [&] {
        if (nonzero_logical(c, stream)) {
            c.set(3, stream);
            c.call(callee::close_file, 0x1076b420);
        }
        if (compare_logical(c, bitmap_record, reuse) != 0) {
            c.set(3, c.u32(bitmap_record + bitmap::pixels));
            c.call(callee::release, 0x1076b430);
            c.set(3, bitmap_record);
            c.call(callee::release, 0x1076b438);
        }
        return result(c, 0);
    };
    if (!nonzero_logical(c, path)) return fail();
    // The cache's path (the file's with the extension replaced by ".rgm"), opened for reading: the region file itself. Without it the
    // bitmap file is opened, read, converted and cached.
    c.set({{3, path}, {4, cache_path}, {5, pool + pool_text::region_extension}});
    c.call(callee::region_path, 0x1076b33c);
    c.set({{3, cache_path}, {4, pool + pool_text::mode_read}});
    c.call(callee::open_file, 0x1076b348);
    stream = c.r(3);
    c.set(30, stream);
    if (nonzero(c, std::int32_t(stream))) {
        c.set(28, 0);
    } else {
        c.set({{3, path}, {4, pool + pool_text::mode_read}, {28, 1}});
        c.call(callee::open_file, 0x1076b368);
        stream = c.r(3);
        c.set(30, stream);
        if (!nonzero(c, std::int32_t(stream))) return fail();
        from_file = true;
    }
    compare(c, std::int8_t(from_file));
    if (from_file) {
        direct_call(c, 0x1076b450, 0x1076b384, {stream, reuse});
        bitmap_record = read_bitmap(c, stream, reuse);
        c.set(29, bitmap_record);
    } else {
        c.set({{6, stream}, {3, record}, {4, bitmap::size}, {5, 1}});
        c.call(callee::read_file, 0x1076b3a0);
        if (compare_logical(c, c.r(3), 1) != 0) return fail();
        direct_call(c, 0x1076b6e0, 0x1076b3b4, {record, reuse});
        bitmap_record = clone_bitmap(c, record, reuse);
        c.set(29, bitmap_record);
        c.set(3, c.u32(bitmap_record + bitmap::pixels));
        c.set({{4, c.u32(bitmap_record + bitmap::bytes)}, {6, stream}, {5, 1}});
        c.call(callee::read_file, 0x1076b3cc);
        if (compare_logical(c, c.r(3), 1) != 0) return fail();
    }
    c.set(3, stream);
    c.call(callee::close_file, 0x1076b3dc);
    compare(c, std::int8_t(from_file));
    if (from_file && nonzero_logical(c, bitmap_record)) {
        // A converted bitmap file: a note in the message buffer and the region file written beside it.
        c.set({{3, c.toc(slot::conversion_message)}, {5, path}, {4, pool + pool_text::converted}});
        c.call(callee::format_text, 0x1076b3fc);
        direct_call(c, 0x1076b240, 0x1076b408, {bitmap_record, cache_path});
        save_region(c, bitmap_record, cache_path);
    }
    return result(c, bitmap_record);
}
}
