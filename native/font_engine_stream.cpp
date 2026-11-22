// The font engine, block `stream` (T2KSTRM, 0x10762f40..0x107630e0): the input stream the TrueType tables are read through. A
// stream is {+0 base, +4 position, +8 length}; a read first checks that the new position stays within the length (a failed check
// reports the assertion through the game's reporter, which returns) and advances the position before it reads the bytes (so a
// read that faults leaves the position moved). The multi-byte values are big-endian, assembled byte by byte.
#include "font_engine_detail.hpp"

namespace cm::aot::font_engine {
namespace {
// `bl 0x107755c0` with the text, the unit's file name and the line, at the original's frame (so the reporter's own frame sits
// where the original's does) and with the registers the callee's prologue saves as the original holds them.
void report_overrun(Cpu& c, std::uint32_t entry, std::uint32_t frame_size, unsigned first_gpr, std::uint32_t line, std::uint32_t back,
                    std::initializer_list<std::pair<unsigned, std::uint32_t>> held) {
    const auto message = c.toc(toc::t2kstrm_pool);
    guest::Frame frame(c, entry, frame_size, {.first_gpr = first_gpr});
    c.set(held);
    c.set(3, message);
    c.set(4, message + t2kstrm_name_offset);
    c.set(5, line);
    c.call(assertion_report, back);
}
}

std::uint32_t read_unsigned_32(Cpu& c, std::uint32_t stream) {
    const auto position = c.u32(stream + stream::position), length = c.u32(stream + stream::length);
    const auto at = c.u32(stream + stream::base) + position;
    if (position + 4 > length) report_overrun(c, 0x10762f40, 0x50, 29, 0x53, 0x10762f88, {{29, stream}, {30, position}, {31, at}});
    c.put32(stream + stream::position, position + 4);
    const auto b0 = c.u8(at), b1 = c.u8(at + 1), b2 = c.u8(at + 2), b3 = c.u8(at + 3);
    // r4 and r5 hold the third and the first byte (the original reloads them as it assembles the word).
    c.set(4, b2);
    c.set(5, b0);
    return std::uint32_t(b0) << 24 | std::uint32_t(b1) << 16 | std::uint32_t(b2) << 8 | b3;
}

std::int32_t read_signed_16(Cpu& c, std::uint32_t stream) {
    const auto position = c.u32(stream + stream::position), length = c.u32(stream + stream::length);
    const auto at = c.u32(stream + stream::base) + position;
    // r4 holds the new position until the assertion's arguments replace it.
    c.set(4, position + 2);
    if (position + 2 > length) report_overrun(c, 0x10762fd0, 0x50, 29, 0x7c, 0x10763018, {{29, stream}, {30, position}, {31, at}});
    c.put32(stream + stream::position, position + 2);
    const auto b0 = c.u8(at), b1 = c.u8(at + 1);
    return std::int32_t(std::int16_t(std::uint16_t(b0 << 8 | b1)));
}

void read_bytes(Cpu& c, std::uint32_t stream, std::uint32_t destination, std::int32_t count) {
    if (count <= 0) return;
    // The held registers at the calls: r28 the stream, r29 the count, r30 the position, r31 the pool's first string.
    const auto message = c.toc(toc::t2kstrm_pool);
    guest::Frame frame(c, 0x10763050, 0x50, {.first_gpr = 28});
    const auto position = c.u32(stream + stream::position);
    c.set({{28, stream}, {29, std::uint32_t(count)}, {30, position}, {31, message}});
    c.set(3, destination);
    c.set(4, c.u32(stream + stream::base) + position);
    c.set(5, std::uint32_t(count));
    c.call(byte_move, 0x10763090);
    const auto end = position + std::uint32_t(count);
    if (end > c.u32(stream + stream::length)) {
        c.set(3, message);
        c.set(4, message + t2kstrm_name_offset);
        c.set(5, 0xb7);
        c.set({{30, end}});
        c.call(assertion_report, 0x107630b4);
    }
    c.put32(stream + stream::position, end);
    c.set(30, end);
}
}
