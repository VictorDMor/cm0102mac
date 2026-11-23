// graph_carbon.cpp, block `text` (the .fnt loader): 0x1076c0b0 reads one bitmap font file into the font table.
//
// A .fnt file (little-endian words): the glyph height, then for each character 0x20..0xff four words (width, bytes a row, the space before and
// after) and, when the bytes a row are not 0, height x bytes-a-row bytes of 4-bit coverage nibbles (two pixels a byte, high nibble first). The
// loader reads straight into the font's record (0x1404 bytes: the height, then 0x14 bytes a character from 0 with the file's four words at +4..+0x10
// and the pixel pointer at +0x14: character 0x20's record starts 0x284 into the font), swaps each word in place and allocates the pixels. Fonts 6
// and 7 then replace some glyphs with copies of others (the pairs of the table at TOC -0x3be0, ended by -1): the destination's pixels are freed, the
// four words copied and the source's pixels duplicated. Any failure raises the assertion of its source line, frees the glyph pixels read so far
// (not for the first read) and closes the file; the routine returns 0, else 1.
//
// STACK CONTRACT: a leaver (stmw r24): the frame is the original's (0x3f0), the alias table is copied to the frame's 0x1a0.. as the original does, and
// every call is made at its own stack pointer with r24-r31 as the original holds them (the leaver check compares them for the nested leavers). The
// registers r3-r10 are as the original leaves them: the swap sequences and the alias loop set them, and the callee's leftovers flow through (`Scratch`).
#include "graph_carbon_text_detail.hpp"
#include <bit>

namespace cm::aot::graph_carbon {
namespace {
guest::Assertion assertion_at(const Cpu& c, std::uint32_t at, std::uint32_t name, std::uint32_t extension, std::uint32_t line) {
    const auto pool = c.toc(toc::pool);
    return {at, pool, pool + text_pool::format, pool + text_pool::title, name, extension, line};
}
constexpr std::uint32_t fopen_entry = 0x10739de0, memset_entry = 0x10778300, fread_entry = 0x10775fe0, fclose_entry = 0x107766f0, sprintf_entry = 0x1077b720,
                        allocate_entry = 0x10775130, free_entry = 0x10775170, copy_entry = 0x10739ef0, free_pixels_entry = 0x1076c980;
// The word reads of a glyph and what each failure of the file does: the `bl` return addresses of the read, of the assertion's first call, of the free of
// the pixels read so far and of the close, with the source line and the assertion's two stack buffers (offsets from the frame's sp).
struct ReadSite { std::uint32_t read_back, assertion_at, line, name, extension, free_back, close_back; };
constexpr ReadSite word_reads[4] = {{0x1076c294, 0x1076c2b0, 0x9e1, 0x120, 0x60, 0x1076c2f4, 0x1076c2fc}, {0x1076c36c, 0x1076c388, 0x9ec, 0x100, 0x58, 0x1076c3cc, 0x1076c3d4},
                                    {0x1076c444, 0x1076c460, 0x9f7, 0xe0, 0x50, 0x1076c4a4, 0x1076c4ac}, {0x1076c51c, 0x1076c538, 0xa02, 0xc0, 0x48, 0x1076c57c, 0x1076c584}};
}

// 0x1076c0b0: loads bitmap font number `font` (1-7) from the file `path` into its table; 1 when loaded, else 0 (see above).
std::uint32_t load_bitmap_font(Cpu& c, std::uint32_t font, std::uint32_t path) {
    Frame frame(c, 0x1076c0b0, 0x3f0, {.first_gpr = 24});
    const auto pool = c.toc(toc::pool), flag = c.toc(text_toc::message_flag), buffer = c.toc(text_toc::message_buffer);
    std::uint32_t file = c.r(29), glyph = c.r(30), font_base = c.r(31);
    c.set({{24, path}, {25, flag}, {26, buffer}, {27, font}, {28, pool}});
    const auto hold = [&] { c.set({{24, path}, {25, flag}, {26, buffer}, {27, font}, {28, pool}, {29, file}, {30, glyph}, {31, font_base}}); };
    Scratch g = Scratch::take(c);
    const auto call = [&](std::uint32_t entry, std::uint32_t back) { hold(); g.put(c); c.call(entry, back); g = Scratch::take(c); };
    const auto report = [&](std::uint32_t at, std::uint32_t name, std::uint32_t extension, std::uint32_t line) {
        hold(); g.put(c);
        guest::report_assertion(c, assertion_at(c, at, frame.at(name), frame.at(extension), line), [&](std::uint32_t entry, std::uint32_t back) { c.call(entry, back); });
        g = Scratch::take(c);
    };
    // The exit of a failed read: the assertion, the glyph pixels freed (`free_back` 0: not), the file closed, 0 returned.
    const auto fail = [&](const ReadSite& site) {
        report(site.assertion_at, site.name, site.extension, site.line);
        g.r3 = font; call(free_pixels_entry, site.free_back);
        g.r3 = file; call(fclose_entry, site.close_back);
        g.r3 = 0; g.put(c);
        return 0u;
    };
    if (path == 0) {
        report(0x1076c0ec, 0x180, 0x78, 0x9bf);
        g.r3 = 0; g.put(c);
        return 0;
    }
    g.r3 = path; g.r4 = pool + 0x7d;
    call(fopen_entry, 0x1076c13c);
    file = g.r3;
    if (file == 0) {
        g.r5 = path; g.r3 = frame.at(0x1dc); g.r4 = pool + 0x11a;
        call(sprintf_entry, 0x1076c154);
        report(0x1076c168, 0x160, 0x70, 0x9c9);
        g.r3 = 0; g.put(c);
        return 0;
    }
    font_base = c.toc(text_toc::font_table) + (extsh(font) - 1) * bitmap_font::size;
    g.r4 = 0; g.r5 = bitmap_font::size; g.r3 = font_base;
    call(memset_entry, 0x1076c1d0);
    g.r3 = font_base; g.r6 = file; g.r4 = 4; g.r5 = 1;
    call(fread_entry, 0x1076c1e4);
    if (g.r3 != 1) {
        report(0x1076c200, 0x140, 0x68, 0x9d4);
        g.r3 = file; call(fclose_entry, 0x1076c244);
        g.r3 = 0; g.put(c);
        return 0;
    }
    const auto height_word = c.u32(font_base);
    c.put32(font_base, std::byteswap(height_word));
    g.r3 = g.r4 = g.r5 = height_word;  // the three loads of the word the swap is built from (the loop sets r3-r6 before its first call)
    for (glyph = 0x20; std::int32_t(glyph) <= 0xff; ++glyph) {
        const auto record = font_base + glyph * bitmap_font::glyph_size;
        for (std::uint32_t word = 0; word < 4; ++word) {
            const auto offset = 4 + 4 * word;
            g.r3 = record + offset; g.r4 = 4; g.r5 = 1; g.r6 = file;
            call(fread_entry, word_reads[word].read_back);
            if (g.r3 != 1) return fail(word_reads[word]);
            // The word is swapped in place. The first three swaps go on to the next word's read (r7 the record, r8 the swapped word, r9 and r10
            // the raw one); the last one has its own register use and goes on to the pixels.
            const auto raw = c.u32(record + offset);
            c.put32(record + offset, std::byteswap(raw));
            if (word < 3) {
                g.r3 = record + offset + 4; g.r4 = 4; g.r5 = 1; g.r6 = file; g.r7 = record; g.r8 = std::byteswap(raw); g.r9 = raw; g.r10 = raw;
            } else {
                g.r3 = record; g.r4 = std::byteswap(raw); g.r5 = raw; g.r6 = raw; g.r7 = raw;
            }
        }
        const auto row_bytes = c.u32(record + 8);
        if (row_bytes == 0) continue;
        const auto pixels = c.u32(font_base) * row_bytes;
        g.r4 = c.u32(font_base); g.r3 = pixels;
        call(allocate_entry, 0x1076c604);
        c.put32(record + 0x14, g.r3);
        g.r4 = record; g.r3 = record;
        if (c.u32(record + 0x14) == 0) {
            report(0x1076c638, 0xa0, 0x40, 0xa11);
            g.r3 = font; call(free_pixels_entry, 0x1076c67c);
            g.r3 = file; call(fclose_entry, 0x1076c684);
            g.r3 = 0; g.put(c);
            return 0;
        }
        g.r4 = c.u32(font_base) * row_bytes; g.r5 = 1; g.r6 = file; g.r3 = c.u32(record + 0x14);
        call(fread_entry, 0x1076c6b8);
        if (g.r3 != 1) {
            report(0x1076c6d4, 0x80, 0x38, 0xa1b);
            g.r3 = font; call(free_pixels_entry, 0x1076c718);
            g.r3 = file; call(fclose_entry, 0x1076c720);
            g.r3 = 0; g.put(c);
            return 0;
        }
    }
    g.r3 = file;
    call(fclose_entry, 0x1076c73c);
    if (extsh(font) == 6 || extsh(font) == 7) {
        // The pairs (destination, source) of glyphs that fonts 6 and 7 copy from others: the byte table (14 words and two halfwords, ended by -1) is
        // copied into the frame at 0x1a0 and read from there. r24-r31 hold the table, the position, the pair and the table's first words.
        const auto pairs = c.toc(-0x3be0);
        std::uint32_t words[14];
        for (unsigned n = 0; n < 14; ++n) words[n] = c.u32(pairs + 4 * n);
        const auto tail_a = std::uint32_t(c.u16(pairs + 0x38)), tail_b = std::uint32_t(c.u16(pairs + 0x3a));
        g.r10 = words[7]; g.r9 = words[8]; g.r8 = words[9]; g.r7 = words[10]; g.r6 = words[11]; g.r5 = words[12]; g.r4 = words[13]; g.r3 = tail_a;
        for (unsigned n = 0; n < 14; ++n) c.put32(frame.at(0x1a0 + 4 * n), words[n]);
        c.put16(frame.at(0x1d8), tail_a);
        c.put16(frame.at(0x1da), tail_b);
        std::uint32_t position = 0, destination = words[4], source = words[3];
        const auto hold_pairs = [&] {
            c.set({{24, pairs}, {25, position}, {26, destination}, {27, source}, {28, words[2]}, {29, words[1]}, {30, words[0]}, {31, font_base}});
        };
        const auto call_pairs = [&](std::uint32_t entry, std::uint32_t back) { hold_pairs(); g.put(c); c.call(entry, back); g = Scratch::take(c); };
        for (;;) {
            g.r3 = frame.at(0x1a0);
            const auto first = std::uint32_t(c.i16(frame.at(0x1a0) + (extsh(position) << 1)));
            if (first == 0xffffffffu) break;
            destination = std::uint32_t(c.i16(frame.at(0x1a0) + (extsh(position) << 1)));
            source = std::uint32_t(c.i16(frame.at(0x1a2) + (extsh(position) << 1)));
            const auto to = font_base + destination * bitmap_font::glyph_size, from = font_base + source * bitmap_font::glyph_size;
            g.r4 = extsh(position) << 1;
            g.r3 = to;
            if (c.u32(to + 0x14) != 0) {
                g.r3 = c.u32(to + 0x14);
                call_pairs(free_entry, 0x1076c82c);
                g.r4 = 0;
                g.r3 = to;
                c.put32(to + 0x14, 0);
            }
            // The four words of the source's record.
            const auto w1 = c.u32(from + 4), w2 = c.u32(from + 8), w3 = c.u32(from + 0xc), w4 = c.u32(from + 0x10);
            c.put32(to + 4, w1);
            c.put32(to + 8, w2);
            c.put32(to + 0xc, w3);
            c.put32(to + 0x10, w4);
            g.r3 = g.r5 = g.r7 = to;
            g.r4 = w4; g.r6 = w3; g.r8 = w2; g.r9 = w1;
            if (c.u32(to + 8) != 0) {
                const auto height = c.u32(font_base);
                g.r4 = height;
                g.r3 = height * c.u32(to + 8);
                call_pairs(allocate_entry, 0x1076c8f4);
                c.put32(to + 0x14, g.r3);
                g.r3 = c.u32(to + 0x14);
                g.r4 = c.u32(from + 0x14);
                g.r5 = c.u32(font_base) * c.u32(to + 8);
                g.r6 = from;
                call_pairs(copy_entry, 0x1076c940);
            }
            position += 2;
        }
    }
    g.r3 = 1; g.put(c);
    return 1;
}
}
