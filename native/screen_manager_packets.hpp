#pragma once
#include "screen_manager_detail.hpp"
#include <bit>

namespace cm::aot::screen_manager::detail {
// SCRMAN packets use little-endian integers inside big-endian guest memory.
// A failed bounds assertion skips only that field; later fields still run.
struct Packet {
    Cpu& c;
    const Frame& frame;
    std::uint32_t data, position, capacity;
    void integer(unsigned width, std::uint32_t value, std::uint32_t diagnostic,
                 unsigned name, unsigned extension, unsigned line) const {
        const auto offset = c.u32(position);
        c.set(width == 2 ? 5 : 4, offset);
        c.set(3, offset + width);
        if (compare_logical(c, c.r(3), c.u32(capacity)) > 0) {
            assertion(c, frame, diagnostic, name, extension, line, 0);
            return;
        }
        c.set(3, c.u32(data));
        if (width == 1) c.put8(c.r(3) + offset, value);
        else if (width == 2) c.put16(c.r(3) + offset, std::byteswap(std::uint16_t(value)));
        else c.put32(c.r(3) + offset, std::byteswap(value));
        c.set(3, c.u32(position));
        c.put32(position, c.r(3) + width);
    }
    void string(std::uint32_t source, std::uint32_t size, std::uint32_t back) const {
        Program{c}.call(0x10739ef0, back, {c.u32(data) + c.u32(position), source, size});
        c.put32(position, c.u32(position) + size);
    }
};
inline Packet packet(Cpu& c, const Frame& f, std::uint32_t manager) {
    return {c, f, manager + 0x2c2c, manager + 0x2c30, manager + 0x2c34};
}
struct SendSite {
    std::uint32_t local, forwarded, remote, diagnostic;
    unsigned name, extension, line;
};
inline void send_current(Cpu& c, const Frame& frame, std::uint32_t address, SendSite site) {
    Manager manager{c, address};
    c.set(6, extended(c.i16(manager.group())));
    Program program{c};
    if (compare(c, std::int16_t(c.r(6))) == 0) {
        program.call(0x1071de80, site.local + 4, {address + 0x2c68, c.u32(address + 0x2c64), 0, address + 0x2c2c});
        return;
    }
    if (site.forwarded) {
        c.set(3, c.u32(manager.group() + 0x24) + 0x130000);
        if (byte_nonzero(c, c.u8(c.r(3) - 0x56c2))) {
            program.call(0x1071de80, site.forwarded + 4,
                         {address + 0x12d6b0, c.u32(address + 0x2c64), 0, address + 0x2c2c});
            return;
        }
    }
    program.call(0x1073a4c0, site.remote + 4,
                 {c.u32(address + 0x2c64), c.u32(address + 0x2c2c), c.u32(address + 0x2c30)});
    if (!byte_nonzero(c, c.r(3))) assertion(c, frame, site.diagnostic, site.name, site.extension, site.line);
}
}

namespace cm::aot::screen_manager {
enum class Property { Word, Rectangle, Id, Short, Pointer, Byte, SignedShort, Text, Color, RealBits };
std::uint32_t send_property(Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, Property);
std::uint32_t send_rectangle(Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t send_command(Cpu&, std::uint32_t, unsigned);
std::uint32_t send_short_command(Cpu&, std::uint32_t, std::uint32_t, bool);
}
