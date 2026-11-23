// The assertion helpers: the path splitter 0x10739890 and the C assertion reporter 0x107755c0 (tools/assertion_audit.py).
//
// 0x10739890 is CodeWarrior's file-name splitter for Mac paths. Its frame (0x460 bytes) holds four 256-byte buffers that start as copies
// of four static arrays (TOC -0x3e78, -0x3e7c, -0x3e80, -0x3e84): the volume at sp + 0x338, the directory at sp + 0x238, the name at
// sp + 0x138 and the extension at sp + 0x38. The path is parsed back to front, all through those buffers in guest memory (the strings are
// strcpy'd, so a name longer than a buffer runs into the next one and a path past 32767 bytes wraps the halfword indices, as the original
// does):
//   - r31 is the index of the last ':' of the path, scanning from the terminator down, as a halfword (-1 without one);
//   - the name buffer takes the text after it and its last '.' splits off the extension (copied to the extension buffer, the name
//     cut there);
//   - unless the colon is the first character, the volume buffer takes the text before the first ':' (the buffer is not touched when there
//     is nothing to copy: its static copy stays), the directory buffer the text between that colon and the last one, and a trailing ':'
//     of the directory is cut;
//   - each non-null output pointer gets its buffer by strcpy.
// The terminators of the directory buffer come from its static copy (the loop that fills it never writes one).
//
// STACK AND REGISTERS. The frame is built by guest::Frame (r25-r31, the link register, the back chain) and every string call is made with
// `c.call` at the original's return address and with the registers the original holds there: r3 and r4 the arguments; r5-r10 what the
// previous callee left, except where the original's own loops write them (r4-r7 after the parsing loops, r8 the first static array's
// address and r4/r5 after the static copies); r25 the pointer the original computes, r26 the path (0 once it is spent), r27-r30 the outputs,
// r31 the colon index. r3-r10 on return are those the last callee leaves (or, for a null path, the static copies').
#include "assertion.hpp"

namespace cm::aot::assertion {
namespace {
using guest::Frame;
namespace entry {
inline constexpr std::uint32_t split_file_name = 0x10739890, report_failed_assertion = 0x107755c0;
}
namespace callee {
inline constexpr std::uint32_t length = 0x1077dbd0, copy = 0x1077dbf0, print_to = 0x1077b610, abort_game = 0x10773fb0;
}
namespace back {  // the address after each `bl` of the splitter
inline constexpr std::uint32_t path_length = 0x10739964, name_copy = 0x107399a0, name_length = 0x107399a8, extension_copy = 0x107399f8,
    directory_length = 0x10739ab4, directory_length_again = 0x10739ad4, volume_out = 0x10739aec, directory_out = 0x10739b00,
    name_out = 0x10739b14, extension_out = 0x10739b28;
}
namespace toc {
inline constexpr std::int32_t arrays[4] = {-0x3e78, -0x3e7c, -0x3e80, -0x3e84},  // the statics the four buffers start as copies of
    streams = -0x5a68,                                                            // the C library's FILE array (the error stream is the third)
    report_format = -0x3b6c;                                                      // "Assertion (%s) failed in \"%s\" on line %d\n"
}
// The frame's four buffers, from the stack pointer, and what each static array is.
inline constexpr std::int32_t frame_size = 0x460, volume_buffer = 0x338, directory_buffer = 0x238, name_buffer = 0x138, extension_buffer = 0x38;
inline constexpr std::uint32_t array_size = 0x100, error_stream = 0xa8;
inline constexpr std::uint8_t colon = ':', dot = '.';

// A halfword register's value as the instruction sees it (`extsh`).
inline std::int32_t halfword(std::uint32_t value) { return std::int16_t(value); }
inline std::uint32_t word(std::int32_t value) { return std::uint32_t(value); }

std::uint32_t length_of(Cpu& c, std::uint32_t text, std::uint32_t return_address) {
    c.set(3, text);
    c.call(callee::length, return_address);
    return c.r(3);
}
void copy_string(Cpu& c, std::uint32_t to, std::uint32_t from, std::uint32_t return_address) {
    c.set({{3, to}, {4, from}});
    c.call(callee::copy, return_address);
}
}

void split_file_name(Cpu& c, std::uint32_t path, std::uint32_t volume, std::uint32_t directory, std::uint32_t name, std::uint32_t extension) {
    Frame frame(c, entry::split_file_name, frame_size, {.first_gpr = 25});
    c.set({{26, path}, {27, volume}, {28, directory}, {29, name}, {30, extension}});
    const auto volume_at = frame.at(volume_buffer), directory_at = frame.at(directory_buffer), name_at = frame.at(name_buffer),
               extension_at = frame.at(extension_buffer);

    // The four buffers start as copies of the four static arrays, two words at a time. r3 ends holding the second to last word of the last
    // array, r4 its last word's address and r5 the buffer's last word's address minus 0x100 + 0x34 (the loops' own registers); r8 holds
    // the first array's address throughout.
    const std::uint32_t buffers[4] = {volume_at, directory_at, name_at, extension_at};
    c.set(8, c.toc(toc::arrays[0]));
    std::uint32_t first_word = 0, source = 0;
    for (unsigned n = 0; n < 4; ++n) {
        source = c.toc(toc::arrays[n]);
        for (std::uint32_t at = 0; at < array_size; at += 8) {
            first_word = c.u32(source + at);
            const auto second_word = c.u32(source + at + 4);
            c.put32(buffers[n] + at, first_word);
            c.put32(buffers[n] + at + 4, second_word);
        }
    }
    c.set({{3, first_word}, {4, source + array_size - 4}, {5, extension_at + array_size - 4}});
    c.s.wr<4>(guest::reg::ctr, 0);
    if (path == 0) return;

    // The last ':' of the path (halfword index, -1 without one): the scan starts at the terminator.
    const auto path_size = length_of(c, path, back::path_length);
    std::int32_t colon_at = halfword(path_size);
    while (halfword(word(colon_at)) >= 0) {
        if (c.u8(path + word(halfword(word(colon_at)))) == colon) break;
        --colon_at;
    }
    c.set(31, word(colon_at));
    // The name buffer takes what follows it.
    copy_string(c, name_at, path + word(halfword(word(colon_at))) + 1, back::name_copy);

    // The extension is the last '.' of the name (the scan starts at the terminator too); it is cut off the name.
    std::int32_t dot_at = halfword(length_of(c, name_at, back::name_length));
    c.set({{4, word(dot_at)}, {3, name_at}});
    while (halfword(word(dot_at)) >= 0) {
        if (c.u8(name_at + word(halfword(word(dot_at)))) == dot) break;
        --dot_at;
    }
    c.set(4, word(dot_at));
    const auto dot_address = name_at + word(halfword(word(dot_at)));
    c.set(25, dot_address);
    if (c.u8(dot_address) == dot) {
        copy_string(c, extension_at, dot_address, back::extension_copy);
        c.put8(dot_address, 0);
    }

    // The volume: the text before the first ':' (the loop stops at the last one too), unless the last ':' is the first character or
    // the path starts with one. r7 counts what it copies.
    std::int32_t index = 0;
    c.set(7, 0);
    const bool has_volume = halfword(word(colon_at)) != 0 && c.u8(path) != colon;
    if (has_volume) {
        c.set({{5, volume_at}, {3, word(halfword(word(colon_at)))}});
        for (;;) {
            const auto at = halfword(word(index));
            c.set(4, word(at));
            if (at >= halfword(word(colon_at))) break;
            const auto byte = c.u8(path + word(at));
            c.set(6, byte);
            if (byte == colon) break;
            c.put8(volume_at + word(at), byte);
            c.set(7, word(++index));
        }
        c.set({{3, volume_at}, {4, 0}});
        c.put8(volume_at + word(halfword(word(index))), 0);
    }
    // The first ':' is skipped.
    if (c.u8(path + word(halfword(word(index)))) == colon) {
        index = halfword(word(index + 1));
        c.set(7, word(index));
    }

    // The directory: what lies between that ':' and the last one.
    c.set({{4, directory_at}, {6, 0}});
    {
        const auto limit = halfword(word(colon_at));
        std::int32_t count = 0;
        while (halfword(word(index)) < limit) {
            c.set(5, word(halfword(word(index))));
            const auto byte = c.u8(path + word(halfword(word(index))));
            c.set(5, byte);
            ++index;
            c.set(7, word(index));
            c.put8(directory_at + word(halfword(word(count))), byte);
            ++count;
            c.set(6, word(count));
        }
        c.set(3, word(halfword(word(index))));
    }
    // Its trailing ':' is cut (the second length is the same: the original asks again after clearing r26).
    const auto directory_size = length_of(c, directory_at, back::directory_length);
    const auto tail = frame.at(directory_buffer - 1);
    c.set(25, tail);
    if (c.u8(tail + directory_size) == colon) {
        c.set(26, 0);
        const auto again = length_of(c, directory_at, back::directory_length_again);
        c.put8(tail + again, 0);
    }

    if (volume) copy_string(c, volume, volume_at, back::volume_out);
    if (directory) copy_string(c, directory, directory_at, back::directory_out);
    if (name) copy_string(c, name, name_at, back::name_out);
    if (extension) copy_string(c, extension, extension_at, back::extension_out);
}

void report_failed_assertion(Cpu& c, std::uint32_t condition, std::uint32_t file, std::uint32_t line) {
    Frame frame(c, entry::report_failed_assertion, 0x40, {});
    const auto streams = c.toc(toc::streams);
    c.set({{6, file}, {7, line}, {8, streams}, {5, condition}, {4, c.toc(toc::report_format)}, {3, streams + error_stream}});
    c.call(callee::print_to, 0x107755e8);
    c.call(callee::abort_game, 0x107755ec);
}
}
