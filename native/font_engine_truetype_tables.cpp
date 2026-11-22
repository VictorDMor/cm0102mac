// The font engine, block `truetype_tables` (TRUETYPE.C, 0x10764610..0x10765810): the readers of the sfnt's tables over a stream made at
// the table's offset: cmap (with the choice of the encoding the game's text uses and the glyphs of the digits), kern, hhea/vhea, head,
// maxp, loca, a glyph outline from glyf, the lookup of a table in the directory and the stream over a table.
//
// Every value is read big-endian through the stream readers (font_engine_stream.cpp, called directly); a record is a block of the
// memory object (the allocator is still generated: called through the dispatcher at the original's return addresses). A routine builds its
// frame at the original's size so what it calls runs at the original's stack pointer.
#include "font_engine_detail.hpp"

namespace cm::aot::font_engine {
namespace {
using guest::Frame;
constexpr std::uint32_t leaver_glyph_arrays = 0x1075c830;

constexpr std::int32_t shift_right(std::int32_t value, std::uint32_t amount) {
    amount &= 63;
    return amount >= 32 ? (value < 0 ? -1 : 0) : value >> amount;
}
std::uint32_t big_endian_16(const Cpu& c, std::uint32_t at) { return std::uint32_t(c.u8(at)) << 8 | c.u8(at + 1); }
}

std::uint32_t find_table_entry(Cpu& c, std::uint32_t font, std::uint32_t tag_word) {
    for (std::int32_t n = 0;; ++n) {
        const auto directory = c.u32(font + sfnt::directory);
        if (!(n < c.i16(directory + table_directory::count))) return 0;
        const auto entry = c.u32(c.u32(directory + table_directory::entries) + 4 * std::uint32_t(n));
        if (c.u32(entry + table_entry::tag) == tag_word) return entry;
    }
}

std::uint32_t new_table_stream(Cpu& c, std::uint32_t font, std::uint32_t tag_word) {
    Frame frame(c, 0x10765780, 0x40, {});
    c.set(9, font);  // the original keeps the sfnt in r9, which its callers' later calls see
    // The directory's lookup is inlined here (the entry is found by the same loop).
    const auto entry = find_table_entry(c, font, tag_word);
    if (entry == 0) return 0;
    return new_substream(c, c.u32(font + sfnt::mem), c.u32(font + sfnt::stream), c.u32(entry + table_entry::offset), c.u32(entry + table_entry::length), 0, 0);
}

std::uint32_t new_head(Cpu& c, std::uint32_t mem, std::uint32_t input) {
    Frame frame(c, 0x10764d10, 0x50, {.first_gpr = 29});
    const auto table = dispatched::allocate(c, mem, head::size, 0x10764d38);
    c.put32(table + head::mem, mem);
    for (const auto at : {head::version, head::font_revision, head::checksum_adjustment, head::magic}) c.put32(table + at, read_unsigned_32(c, input));
    c.put16(table + head::flags, std::uint32_t(read_signed_16(c, input)));
    c.put16(table + head::units_per_em, std::uint32_t(read_signed_16(c, input)));
    for (const auto at : {head::created_high, head::created_low, head::modified_high, head::modified_low}) c.put32(table + at, read_unsigned_32(c, input));
    for (std::uint32_t at = head::x_min; at <= head::glyph_data_format; at += 2) c.put16(table + at, std::uint32_t(read_signed_16(c, input)));
    return table;
}

std::uint32_t new_hhea(Cpu& c, std::uint32_t mem, std::uint32_t input) {
    Frame frame(c, 0x10764bf0, 0x50, {.first_gpr = 29});
    const auto table = dispatched::allocate(c, mem, hhea::size, 0x10764c18);
    c.put32(table + hhea::mem, mem);
    c.put32(table + hhea::version, read_unsigned_32(c, input));
    for (std::uint32_t at = hhea::ascender; at <= hhea::number_of_metrics; at += 2) c.put16(table + at, std::uint32_t(read_signed_16(c, input)));
    return table;
}

std::uint32_t new_maxp(Cpu& c, std::uint32_t mem, std::uint32_t input) {
    Frame frame(c, 0x10764e50, 0x50, {.first_gpr = 29});
    const auto table = dispatched::allocate(c, mem, maxp::size, 0x10764e78);
    c.put32(table + maxp::mem, mem);
    c.put32(table + maxp::version, read_unsigned_32(c, input));
    for (std::uint32_t at = maxp::num_glyphs; at <= maxp::max_component_depth; at += 2) c.put16(table + at, std::uint32_t(read_signed_16(c, input)));
    return table;
}

std::uint32_t new_loca(Cpu& c, std::uint32_t mem, std::uint32_t input, std::uint32_t format, std::uint32_t length) {
    Frame frame(c, 0x10764f60, 0x50, {.first_gpr = 26});
    const auto pool = c.toc(toc::truetype_pool);
    const auto table = dispatched::allocate(c, mem, loca::size, 0x10764f8c);
    const auto short_format = std::int32_t(std::int16_t(format));
    const auto count = shift_right(wrap(length), std::uint32_t(short_format + 1));
    c.put32(table + loca::mem, mem);
    c.put32(table + loca::count, std::uint32_t(count));
    c.put16(table + loca::format, format);
    c.put32(table + loca::offsets, dispatched::allocate(c, mem, std::uint32_t(count) << 2, 0x10764fb4));
    if (short_format == 1) {
        for (std::int32_t n = 0; n < count; ++n) c.put32(c.u32(table + loca::offsets) + 4 * std::uint32_t(n), read_unsigned_32(c, input));
    } else if (short_format == 0) {
        for (std::int32_t n = 0; n < count; ++n) c.put32(c.u32(table + loca::offsets) + 4 * std::uint32_t(n), rlwinm(std::uint32_t(read_signed_16(c, input)), 1, 15, 30));
    } else {
        dispatched::assertion(c, pool, 0x58, 0x16, 0x51a, 0x10765044);
    }
    return table;
}

std::uint32_t new_kern_subtable(Cpu& c, std::uint32_t mem, std::uint32_t apple, std::uint32_t input) {
    Frame frame(c, 0x107649a0, 0x50, {.first_gpr = 27});
    const auto table = dispatched::allocate(c, mem, kern_subtable::size, 0x107649c4);
    c.put32(table + kern_subtable::mem, mem);
    if (apple != 0) {
        c.put32(table + kern_subtable::length, read_unsigned_32(c, input));
        c.put16(table + kern_subtable::coverage, std::uint32_t(read_signed_16(c, input)));
        read_signed_16(c, input);
        c.put16(table + kern_subtable::format, c.u16(table + kern_subtable::coverage) & 0xff);
    } else {
        c.put16(table + kern_subtable::format, std::uint32_t(read_signed_16(c, input)));
        c.put32(table + kern_subtable::length, std::uint32_t(read_signed_16(c, input)) & 0xffff);
        c.put16(table + kern_subtable::coverage, std::uint32_t(read_signed_16(c, input)));
    }
    c.put32(table + kern_subtable::pairs, 0);
    if (c.u16(table + kern_subtable::format) != 0 || wrap(c.u32(table + kern_subtable::length)) <= 0) return table;
    const auto pairs = dispatched::allocate(c, mem, kern_pairs::size, 0x10764a58);
    c.put32(pairs + kern_pairs::mem, mem);
    c.put16(pairs + kern_pairs::count, std::uint32_t(read_signed_16(c, input)));
    c.put16(pairs + kern_pairs::search_range, std::uint32_t(read_signed_16(c, input)));
    c.put16(pairs + kern_pairs::entry_selector, std::uint32_t(read_signed_16(c, input)));
    c.put16(pairs + kern_pairs::range_shift, std::uint32_t(read_signed_16(c, input)));
    c.put32(pairs + kern_pairs::entries, dispatched::allocate(c, mem, std::uint32_t(c.u16(pairs + kern_pairs::count)) << 3, 0x10764aa0));
    for (std::uint32_t n = 0; n < c.u16(pairs + kern_pairs::count); ++n) {
        const auto entry = c.u32(pairs + kern_pairs::entries) + n * kern_pairs::entry_size;
        c.put32(entry, read_unsigned_32(c, input));
        c.put16(c.u32(pairs + kern_pairs::entries) + n * kern_pairs::entry_size + 4, std::uint32_t(read_signed_16(c, input)));
    }
    c.put32(table + kern_subtable::pairs, pairs);
    return table;
}

std::uint32_t new_kern(Cpu& c, std::uint32_t mem, std::uint32_t input) {
    Frame frame(c, 0x10764b10, 0x50, {.first_gpr = 26});
    std::uint32_t apple = 0;
    const auto table = dispatched::allocate(c, mem, kern::size, 0x10764b34);
    c.put32(table + kern::mem, mem);
    c.put16(table + kern::version, std::uint32_t(read_signed_16(c, input)));
    c.put32(table + kern::table_count, std::uint32_t(std::int32_t(read_signed_16(c, input))));
    if (c.u16(table + kern::version) == 1 && c.i32(table + kern::table_count) == 0) {
        c.put32(table + kern::table_count, read_unsigned_32(c, input));
        apple = 1;
    }
    c.put32(table + kern::tables, dispatched::allocate(c, mem, c.u32(table + kern::table_count) << 2, 0x10764b94));
    for (std::int32_t n = 0; n < c.i32(table + kern::table_count); ++n)
        c.put32(c.u32(table + kern::tables) + 4 * std::uint32_t(n), new_kern_subtable(c, mem, apple, input));
    return table;
}

std::uint32_t new_cmap(Cpu& c, std::uint32_t mem, std::uint32_t platform, std::uint32_t encoding, std::uint32_t input) {
    Frame frame(c, 0x10764610, 0x60, {.first_gpr = 24});
    // (The loop over the ten digits asserts that its counter stays below ten at line 0x115: it cannot fail, the counter starts at 0 and the
    // loop ends with the digit '9'.)
    const auto table = dispatched::allocate(c, mem, cmap::size, 0x1076463c);
    c.put32(table + cmap::mem, mem);
    c.put16(table + cmap::version, std::uint32_t(read_signed_16(c, input)));
    c.put16(table + cmap::encoding_count, std::uint32_t(read_signed_16(c, input)));
    c.put32(table + cmap::encodings, dispatched::allocate(c, mem, std::uint32_t(c.i16(table + cmap::encoding_count)) << 2, 0x10764670));
    for (std::int32_t n = 0; n < c.i16(table + cmap::encoding_count); ++n) {
        const auto record = dispatched::allocate(c, mem, cmap_encoding::size, 0x1076468c);
        c.put32(c.u32(table + cmap::encodings) + 4 * std::uint32_t(n), record);
        c.put16(record + cmap_encoding::platform, std::uint32_t(read_signed_16(c, input)));
        c.put16(record + cmap_encoding::encoding, std::uint32_t(read_signed_16(c, input)));
        c.put32(record + cmap_encoding::offset, read_unsigned_32(c, input) & 0xffff);
    }
    stream_rewind(c, input);
    c.put32(table + cmap::data_length, std::uint32_t(stream_remaining(c, input)));
    c.put32(table + cmap::data, dispatched::allocate(c, mem, c.u32(table + cmap::data_length), 0x10764708));
    read_bytes(c, input, c.u32(table + cmap::data), std::int32_t(c.u32(table + cmap::data_length)));
    c.put16(table + cmap::chosen, 0);
    const auto format_of = [&](std::uint32_t index) {
        return big_endian_16(c, c.u32(table + cmap::data) + c.u32(c.u32(c.u32(table + cmap::encodings) + 4 * index) + cmap_encoding::offset));
    };
    // The record the text uses: the wanted platform and encoding with format 0, 4 or 6 first, then by format 0, 6, 4 for any.
    const std::uint32_t wanted_platform = platform & 0xffff, wanted_encoding = encoding & 0xffff;
    bool chosen_pass = false;
    for (std::uint32_t pass = 0; pass <= 3;) {
        bool found = false;
        for (std::int32_t n = 0; n < c.i16(table + cmap::encoding_count); ++n) {
            const auto record = c.u32(c.u32(table + cmap::encodings) + 4 * std::uint32_t(n));
            const auto format = format_of(std::uint32_t(n));
            bool match = false;
            if (pass == 0) {
                if (format == 0 || format == 4 || format == 6) {
                    const bool platform_equal = c.u16(record + cmap_encoding::platform) == wanted_platform, encoding_equal = c.u16(record + cmap_encoding::encoding) == wanted_encoding;
                    match = platform_equal ? (encoding_equal || wanted_encoding == 0xffff) : (encoding_equal && wanted_platform == 0xffff);
                }
            } else {
                match = pass == 1 ? format == 0 : pass == 2 ? format == 6 : format == 4;
            }
            if (match) {
                c.put16(table + cmap::chosen, std::uint32_t(n));
                found = true;
                break;
            }
        }
        if (found) chosen_pass = true;
        pass = found ? 1000 : pass + 1;
    }
    // r9 counted the passes: 4 when none matched, 0x3e8 after a match (the digits' format 6 lookups then leave their own).
    c.set(9, chosen_pass ? 0x3e8 : 4);
    c.put16(table + cmap::format, format_of(std::uint32_t(c.i16(table + cmap::chosen))));
    if (c.u16(table + cmap::format) != 0 && c.u16(table + cmap::format) != 6 && c.u16(table + cmap::format) != 4) dispatched::raise_error(c, mem, 0x2725, 0x107648b4);
    // The glyphs of '0'..'9'.
    for (std::uint32_t digit = 0; digit < 10; ++digit) {
        const std::uint32_t code = 0x30 + digit;
        std::uint32_t glyph = 0;
        if (c.u16(table + cmap::format) == 0) {
            if (code < 0x100) glyph = c.u8(c.u32(table + cmap::data) + c.u32(c.u32(c.u32(table + cmap::encodings) + 4 * std::uint32_t(c.i16(table + cmap::chosen))) + cmap_encoding::offset) + code + 6);
        } else if (c.u16(table + cmap::format) == 6) {
            glyph = cmap6_lookup(c, table, code);
        } else if (c.u16(table + cmap::format) == 4) {
            glyph = cmap4_lookup(c, table, code);
        }
        c.put16(table + cmap::digit_glyphs + 2 * digit, glyph);
        if (c.u16(table + cmap::digit_glyphs + 2 * digit) == 0) c.put16(table + cmap::digit_glyphs + 2 * digit, 0xffff);
    }
    return table;
}

std::uint32_t new_glyph(Cpu& c, std::uint32_t mem, std::uint32_t input, std::uint32_t keep_instructions, std::uint32_t left_bearing, std::uint32_t advance, std::uint32_t top,
                        std::uint32_t bottom) {
    Frame frame(c, 0x10765060, 0x70, {.first_gpr = 18});
    const auto pool = c.toc(toc::truetype_pool);
    // The values the stack leaver 0x1075c830 saves where the game's printf reads them: r29-r31 are the last three arguments.
    c.set({{29, advance}, {30, top}, {31, bottom}});
    // r9 (the last argument on entry) is the original's flag counter in a simple glyph, then the high byte of the last word delta read.
    const auto glyph = dispatched::scratch(c, mem, glyph_record::size, 0, 0x1076509c);
    c.put32(glyph + glyph_record::mem, mem);
    c.put32(glyph + glyph_record::contour_ends, 0);
    c.put32(glyph + glyph_record::contour_starts, 0);
    c.put32(glyph + glyph_record::components, 0);
    c.put32(glyph + glyph_record::original_ys, 0);
    c.put32(glyph + glyph_record::original_xs, 0);
    const auto contours = std::int32_t(read_signed_16(c, input));
    c.put16(glyph + glyph_record::contours, std::uint32_t(contours));
    c.put16(glyph + glyph_record::x_min, std::uint32_t(read_signed_16(c, input)));
    c.put16(glyph + glyph_record::y_min, std::uint32_t(read_signed_16(c, input)));
    c.put16(glyph + glyph_record::x_max, std::uint32_t(read_signed_16(c, input)));
    c.put16(glyph + glyph_record::y_max, std::uint32_t(read_signed_16(c, input)));
    c.put32(glyph + glyph_record::component_words, 0);
    c.put32(glyph + glyph_record::instruction_length, 0);
    c.put32(glyph + glyph_record::instructions, 0);
    c.put32(glyph + glyph_record::point_count, 0);
    c.put32(glyph + 0xc, 0);
    c.put32(glyph + 0x10, 0);
    c.put16(glyph + glyph_record::shape, 2);
    c.put16(glyph + glyph_record::contour_count, 0);
    std::uint32_t points = 0;
    std::uint32_t xs = 0, ys = 0;
    // The instructions that follow the outline: kept (read into a block of the scratch slot 3) or skipped.
    const auto instructions = [&](std::uint32_t back_allocate, bool have) {
        c.put32(glyph + glyph_record::instruction_length, 0);
        if (!have) return;
        c.put32(glyph + glyph_record::instruction_length, std::uint32_t(std::int32_t(read_signed_16(c, input))));
        if (std::int8_t(keep_instructions) != 0) {
            if (c.i32(glyph + glyph_record::instruction_length) <= 0) return;
            c.put32(glyph + glyph_record::instructions, dispatched::scratch(c, c.u32(glyph + glyph_record::mem), c.u32(glyph + glyph_record::instruction_length), 3, back_allocate));
            read_bytes(c, input, c.u32(glyph + glyph_record::instructions), c.i32(glyph + glyph_record::instruction_length));
        } else {
            c.put32(input + stream::position, c.u32(input + stream::position) + c.u32(glyph + glyph_record::instruction_length));
            c.put32(glyph + glyph_record::instruction_length, 0);
        }
    };
    if (contours == 0) {
        dispatched::assertion(c, pool, 0x5e, 0x16, 0x5bf, 0x10765140);
        c.put16(glyph + glyph_record::outline_points, 0);
        return glyph;
    }
    if (contours < 0) {
        // A composite: its component words (flags, glyph index, arguments, a transform) up to the last component, then its instructions.
        c.put32(glyph + glyph_record::component_capacity, 0x400);
        std::uint32_t words = 0, offset = 0;
        auto block = dispatched::allocate(c, c.u32(glyph + glyph_record::mem), c.u32(glyph + glyph_record::component_capacity) * 2, 0x10765168);
        bool have_instructions;
        std::int32_t flags;
        do {
            const auto capacity = std::int32_t(c.u32(glyph + glyph_record::component_capacity));
            if (!(wrap(words) < capacity - 10)) {
                const auto sign = std::uint32_t(capacity) >> 31;
                const auto grown = std::uint32_t(capacity) + std::uint32_t(shift_right(wrap(sign + std::uint32_t(capacity)), 1));
                c.put32(glyph + glyph_record::component_capacity, grown);
                block = dispatched::reallocate(c, c.u32(glyph + glyph_record::mem), block, c.u32(glyph + glyph_record::component_capacity) * 2, 0x107651a8);
                if (block == 0) dispatched::assertion(c, pool, 0x70, 0x16, 0x5ce, 0x107651c0);
            }
            flags = std::int32_t(read_signed_16(c, input));
            c.put16(block + offset, std::uint32_t(flags));
            offset += 2;
            have_instructions = (std::uint32_t(flags) >> 8 & 1) != 0;
            c.put16(block + offset, std::uint32_t(read_signed_16(c, input)));
            offset += 2;
            const auto more = [&](unsigned count) {
                for (unsigned k = 0; k < count; ++k) {
                    c.put16(block + offset, std::uint32_t(read_signed_16(c, input)));
                    offset += 2;
                }
            };
            if (flags & 1) { more(2); words += 4; } else { more(1); words += 3; }
            if (flags & 8) { more(1); words += 1; }
            else if (flags & 0x40) { more(2); words += 2; }
            else if (flags & 0x80) { more(4); words += 4; }
        } while (flags & 0x20);
        c.put32(glyph + glyph_record::instruction_length, 0);
        instructions(0x10765310, have_instructions);
        c.set({{3, glyph}, {4, 0}});
        c.call(leaver_glyph_arrays, 0x1076534c);
        xs = c.u32(glyph + glyph_record::xs);
        ys = c.u32(glyph + glyph_record::ys);
        c.put32(glyph + glyph_record::components, block);
        c.put32(glyph + glyph_record::component_words, words);
        points = 0;
    } else {
        // A simple glyph: the contour end points, the instructions, the flags (with repeats), the x then the y deltas.
        std::uint32_t next_point = 0;
        if (contours <= 8) {
            c.put32(glyph + glyph_record::contour_starts, glyph + glyph_record::inline_ends);
            c.put32(glyph + glyph_record::contour_ends, c.u32(glyph + glyph_record::contour_starts) + 0x10);
        } else {
            c.put32(glyph + glyph_record::contour_starts, dispatched::allocate(c, c.u32(glyph + glyph_record::mem), std::uint32_t(contours) << 2, 0x10765394));
            c.put32(glyph + glyph_record::contour_ends, c.u32(glyph + glyph_record::contour_starts) + (std::uint32_t(contours) << 1));
        }
        for (std::int32_t n = 0; n < contours; ++n) {
            c.put16(c.u32(glyph + glyph_record::contour_starts) + 2 * std::uint32_t(n), next_point);
            const auto end = std::uint32_t(read_signed_16(c, input));
            next_point = end + 1;
            c.put16(c.u32(glyph + glyph_record::contour_ends) + 2 * std::uint32_t(n), end);
        }
        points = std::uint32_t(std::int32_t(std::int16_t(next_point)));
        if (wrap(points) <= 0) dispatched::assertion(c, pool, 0x86, 0x16, 0x60d, 0x107653f8);
        instructions(0x10765428, true);
        c.set({{3, glyph}, {4, points}});
        c.call(leaver_glyph_arrays, 0x10765464);
        const auto flags = c.u32(glyph + glyph_record::on_curve);
        xs = c.u32(glyph + glyph_record::xs);
        ys = c.u32(glyph + glyph_record::ys);
        c.put16(glyph + glyph_record::contour_count, std::uint32_t(contours));
        c.put32(glyph + glyph_record::point_count, points);
        const auto next_byte = [&] {
            const auto position = c.u32(input + stream::position);
            c.put32(input + stream::position, position + 1);
            return c.u8(c.u32(input + stream::base) + position);
        };
        // The flags: a flag with bit 3 is followed by a repeat count.
        std::int32_t filled = 0;
        auto to = flags;
        std::uint32_t r9 = 0;
        while (filled < wrap(points)) {
            const auto flag = next_byte();
            ++filled;
            r9 = std::uint32_t(filled);
            c.put8(to++, flag);
            if (flag & 8) {
                auto repeat = std::int32_t(next_byte());
                auto repeated = flags + std::uint32_t(filled);
                while (repeat > 0 && filled < wrap(points)) {
                    c.put8(repeated, flag);
                    ++filled;
                    r9 = std::uint32_t(filled);
                    ++to;
                    ++repeated;
                    --repeat;
                }
                // A repeat count that is exhausted leaves the loop at once (the original tests the count first).
            }
        }
        // x: bit 1 a byte (bit 4 its sign: set adds), else bit 4 set repeats the previous, else a halfword delta.
        std::int32_t value = 0;
        for (std::int32_t n = 0; n < wrap(points); ++n) {
            const auto flag = c.u8(flags + std::uint32_t(n));
            if (flag & 2) {
                const auto byte = std::int32_t(next_byte());
                value = std::int32_t(std::int16_t(std::uint32_t(flag & 0x10 ? value + byte : value - byte)));
            } else if (!(flag & 0x10)) {
                const auto high = next_byte();
                r9 = high;  // `lbzx r9,r9,r8`: the high byte replaces the base
                const auto delta = std::int16_t(std::uint32_t(high) << 8 | next_byte());
                value = std::int32_t(std::int16_t(std::uint32_t(value + delta)));
            }
            c.put16(xs + 2 * std::uint32_t(n), std::uint32_t(value));
        }
        // y: bit 2 a byte (bit 5 its sign), bit 5 set otherwise repeats; the flag keeps only its on-curve bit.
        value = 0;
        for (std::int32_t n = 0; n < wrap(points); ++n) {
            const auto flag = c.u8(flags + std::uint32_t(n));
            c.put8(flags + std::uint32_t(n), flag & 1);
            if (flag & 4) {
                const auto byte = std::int32_t(next_byte());
                value = std::int32_t(std::int16_t(std::uint32_t(flag & 0x20 ? value + byte : value - byte)));
            } else if (!(flag & 0x20)) {
                const auto high = next_byte();
                r9 = high;  // `lbzx r9,r9,r8`: the high byte replaces the base
                const auto delta = std::int16_t(std::uint32_t(high) << 8 | next_byte());
                value = std::int32_t(std::int16_t(std::uint32_t(value + delta)));
            }
            c.put16(ys + 2 * std::uint32_t(n), std::uint32_t(value));
        }
        c.set(9, r9);
        if (flags == 0) dispatched::assertion(c, pool, 0x95, 0x16, 0x64d, 0x10765698);
    }
    // The four phantom points after the outline: the left side bearing, the advance, the top and the bottom.
    const auto n2 = 2 * points;
    c.put16(ys + n2, 0);
    c.put16(xs + n2, std::uint32_t(c.i16(glyph + glyph_record::x_min)) - left_bearing);
    c.put16(ys + n2 + 2, 0);
    c.put16(xs + n2 + 2, advance + std::uint32_t(c.i16(xs + n2)));
    c.put16(ys + n2 + 4, std::uint32_t(c.i16(glyph + glyph_record::y_max)) + top);
    const auto middle = (c.i16(xs + n2) + c.i16(xs + n2 + 2)) >> 1;
    c.put16(xs + n2 + 4, std::uint32_t(middle));
    c.put16(ys + n2 + 6, std::uint32_t(c.i16(ys + n2 + 4)) - bottom);
    c.put16(xs + n2 + 6, std::uint32_t(middle));
    c.put16(glyph + glyph_record::outline_points, points);
    return glyph;
}

}
