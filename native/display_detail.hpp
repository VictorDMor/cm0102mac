#pragma once
// DISPLAY.CPP: the layouts, globals and helpers more than one of the unit's files uses.
#include "guest_abi.hpp"
#include "display_adapter.hpp"
#include "adaptive_gui.hpp"
#include <bit>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <utility>

namespace cm::aot::display {
using guest::Cpu;
using guest::Frame;

// The main object (TOC game, the cell's address itself): one 0x2c0-byte row per human, whose word at
// +0x24 is the display table in use for that human, and the two display tables it owns.
namespace game {
inline constexpr std::uint32_t human_size = 0x2c0, current_table = 0x24,  // per human row
                               current_human = 0x2c58,                    // signed halfword
                               network_flag = 0x2c64,                     // word passed to the interpreter
                               first_table = 0x2c68, second_table = 0x12d6b0;
}
// A display table (0x130000 + its negative offsets: the generated code addresses most fields as
// `addis rX,table,0x13` then a negative displacement). Names marked (inferred) come from use.
namespace table {
inline constexpr std::uint32_t high = 0x130000;              // base of the negative displacements
inline constexpr std::uint32_t areas = 0x4, area_size = 0xbf0, area_capacity = 250;            // AREA.CPP objects
inline constexpr std::uint32_t guios = 0xba864, guio_size = 0x17c, guio_capacity = 1200;       // GUIO.CPP objects
inline constexpr std::uint32_t area_count = high - 0x625c,   // signed halfword
                               guio_count = high - 0x625a,   // signed halfword
                               announced_area_count = high - 0x56c4,  // signed halfword: the area count a screen's header announces (command 8 compares it with area_count)
                               announced_guio_count = high - 0x56c6,  // signed halfword: the GUI object count the header announces (compared with guio_count)
                               mode = high - 0x56bb,         // byte: handed to the control setter 0x10726ae0 (set by command 0)
                               screen_set = high - 0x56bc,   // byte: nonzero makes commands 1-3 do nothing
                               unsent = high - 0x56bd,       // byte: set when the screen changes, cleared after a successful send
                               reply_wanted = high - 0x56f8, // byte: nonzero makes commands 1-3 answer through 0x10723850
                               string_list = high - 0x56d4,  // word: the table's list of strings {array, count, capacity}, freed by 0x10718ba0, commands 23-25
                               width_flag = high - 0x56ff,   // byte (command 0)
                               height_flag = high - 0x56fe,  // halfword (command 0)
                               guio_limit = high - 0x5702,   // signed halfword: count of index_list
                               index_list = high - 0x6064;   // halfwords, indexed by guio_limit
}

// Addresses and extensions the unit's files all use, as the generated code computes them (wrapping 32-bit).
// The area / GUI object record `index` of a display table (`mulli`, then the table's offset).
constexpr std::uint32_t area_at(std::uint32_t base, std::int32_t index) {
    return base + std::uint32_t(index) * table::area_size;
}
constexpr std::uint32_t guio_at(std::uint32_t base, std::int32_t index) {
    return base + std::uint32_t(index) * table::guio_size + table::guios;
}
// A field of the table: `addis rX,table,0x13` and a negative displacement.
constexpr std::uint32_t field(std::uint32_t base, std::int32_t displacement) {
    return base + table::high + std::uint32_t(displacement);
}
// A halfword / byte as the original holds it in a register (lha, extsh, extsb).
constexpr std::uint32_t sign16(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }
constexpr std::uint32_t sign8(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int8_t(v))); }
constexpr std::int32_t half(std::uint32_t v) { return std::int16_t(v); }  // extsh, signed

// The screen's last column and row: 799 and 599 in the original (`li rX,0x31f` / `0x257` and the
// compares and `subfic`s against them); the adaptive interface (adaptive_gui.hpp, "New Screens")
// replaces them with the window's when the active screen opted in. In 800x600 they are the original's.
inline std::int32_t screen_right(const Cpu& c) {
    const auto canvas = gui_canvas(c.s);
    return canvas.original() ? 0x31f : canvas.width - 1;
}
inline std::int32_t screen_bottom(const Cpu& c) {
    const auto canvas = gui_canvas(c.s);
    return canvas.original() ? 0x257 : canvas.height - 1;
}

// Words of the table of contents (Cpu::toc).
namespace toc {
inline constexpr std::int32_t pool = -0x3f8c,       // the unit's strings
                              game = -0x7bb0,       // the main object
                              message = -0x7ba8,    // the shared message buffer
                              success = -0x7bac;    // the shared success flag
}
// Offsets in the unit's string pool.
namespace text {
inline constexpr std::uint32_t source_file = 0x00,   // "DISPLAY.CPP"
                               report_format = 0x0c, // "v%s %s.%s %d"
                               error = 0x19,         // "Error"
                               network_header = 0x39; // "network.h": the file name some assertions report
}

// The assertion report of a failed check: `at` is the address of the report's first call (`bl
// 0x10739890`), `name` and `extension` the frame offsets of its two buffers, `line` the source line,
// `file` the pool offset of the file name the site reports (DISPLAY.CPP or network.h).
struct Site { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; std::uint32_t file = text::source_file; };
inline void report(Cpu& c, const Frame& frame, const Site& site) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {site.at, pool + site.file, pool + text::report_format, pool + text::error,
                                frame.at(site.name), frame.at(site.extension), site.line});
}

// The non-volatile registers r13-r31 at a routine's dispatched calls, for routines that track many of
// them: callees save them into frames that later code may read as stale stack, so each call sees the
// values the original holds there. The algorithm keeps its own named locals; `hold` records what the
// original's register holds from that point on, `apply` writes them before a call.
class CallRegisters {
public:
    explicit CallRegisters(Cpu& cpu) : c(cpu) { for (unsigned n = 13; n < 32; ++n) value[n] = c.r(n); }
    void hold(std::initializer_list<std::pair<unsigned, std::uint32_t>> held) { for (const auto& [n, v] : held) value[n] = v; }
    std::uint32_t operator[](unsigned n) const { return value[n]; }
    void apply() const { for (unsigned n = 13; n < 32; ++n) c.set(n, value[n]); }
private:
    Cpu& c;
    std::uint32_t value[32]{};
};

// network.h's checked stream the unit reads and writes ({bytes, position, length} words; the values
// inside are little-endian in big-endian memory). Every access compares `position + size` with `length`
// (unsigned, wrapping); a failed check runs `before_report` (the register image the original has there),
// reports the assertion with file "network.h" and leaves the destination and the position unchanged.
// A halfword read is zero-extended by read16 and sign-extended by read16_signed: the call picks the one
// its listing has. The reads and writes keep the original's order: position, length, bytes, the access,
// then (writes) the position again before it moves.
class CheckedStream {
public:
    using Hook = std::function<void()>;
    CheckedStream(Cpu& cpu, const Frame& f, std::uint32_t address, const CallRegisters& regs)
        : c(cpu), frame(f), self(address), before([&regs] { regs.apply(); }) {}
    CheckedStream(Cpu& cpu, const Frame& f, std::uint32_t address, Hook before_report, Hook after_report = {})
        : c(cpu), frame(f), self(address), before(std::move(before_report)), after(std::move(after_report)) {}

    std::uint32_t bytes() const { return c.u32(self); }
    std::uint32_t position() const { return c.u32(self + 4); }
    std::uint32_t at_position() const { return bytes() + position(); }  // the next byte's address
    void advance(std::uint32_t count) { c.put32(self + 4, position() + count); }  // also past an unchecked copy

    // The check alone: the position when `size` more bytes fit, else nullopt after the report.
    std::optional<std::uint32_t> reserve(std::uint32_t size, const Site& site) {
        const auto start = position();
        if (start + size > c.u32(self + 8)) {  // cmplw
            if (before) before();
            auto at_site = site;
            at_site.file = text::network_header;
            report(c, frame, at_site);
            if (after) after();
            return std::nullopt;
        }
        return start;
    }
    std::optional<std::uint32_t> read8(const Site& site) { return take(1, site); }
    std::optional<std::uint32_t> read16(const Site& site) {
        const auto start = reserve(2, site);
        if (!start) return std::nullopt;
        const std::uint32_t value = std::byteswap(c.u16(bytes() + *start));
        c.put32(self + 4, *start + 2);
        return value;
    }
    std::optional<std::uint32_t> read16_signed(const Site& site) {
        const auto value = read16(site);
        return value ? std::optional<std::uint32_t>(sign16(*value)) : std::nullopt;
    }
    std::optional<std::uint32_t> read32(const Site& site) {
        const auto start = reserve(4, site);
        if (!start) return std::nullopt;
        const auto value = std::byteswap(c.u32(bytes() + *start));
        c.put32(self + 4, *start + 4);
        return value;
    }
    // The same read of 1, 2 or 4 bytes loaded one byte at a time (the listings that assemble the value),
    // zero-extended.
    std::optional<std::uint32_t> read_le_bytes(unsigned size, const Site& site) {
        const auto start = reserve(size, site);
        if (!start) return std::nullopt;
        const auto at = bytes() + *start;
        std::uint32_t value = 0;
        for (unsigned n = 0; n < size; ++n) value |= std::uint32_t(c.u8(at + n)) << (8 * n);
        c.put32(self + 4, *start + size);
        return value;
    }
    // A checked store of `size` (1, 2 or 4) bytes, little-endian. False when the check failed.
    bool write(const Site& site, unsigned size, std::uint32_t value) {
        const auto start = reserve(size, site);
        if (!start) return false;
        const auto to = bytes() + *start;
        if (size == 1) c.put8(to, value);
        else if (size == 2) c.put16(to, std::byteswap(std::uint16_t(value)));
        else c.put32(to, std::byteswap(value));
        advance(size);
        return true;
    }
    bool write8(const Site& site, std::uint32_t value) { return write(site, 1, value); }
    bool write16(const Site& site, std::uint32_t value) { return write(site, 2, value); }
    bool write32(const Site& site, std::uint32_t value) { return write(site, 4, value); }

private:
    std::optional<std::uint32_t> take(std::uint32_t size, const Site& site) {
        const auto start = reserve(size, site);
        if (!start) return std::nullopt;
        const std::uint32_t value = c.u8(bytes() + *start);
        c.put32(self + 4, *start + size);
        return value;
    }
    Cpu& c;
    const Frame& frame;
    std::uint32_t self;
    Hook before, after;
};

// `bl entry` with the arguments in r3 onwards; the callee's r3. For routines outside the unit.
inline std::uint32_t invoke(Cpu& c, std::uint32_t entry, std::uint32_t back,
                            std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    c.call(entry, back);
    return c.r(3);
}
// The same for a routine of this unit that lives in another block's file: runs its C++ in place.
inline std::uint32_t invoke_unit(Cpu& c, std::uint32_t entry, std::uint32_t back,
                                 std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    call(c, entry, back);
    return c.r(3);
}
}
