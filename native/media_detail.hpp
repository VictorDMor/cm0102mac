#pragma once
// What the media.cpp blocks share: the world's slots, the random stream the routines
// draw from in line, calls out of the module, the text formatter and the assert macro.
#include "guest_values.hpp"
#include "media.hpp"
#include <initializer_list>

namespace cm::aot::media::detail {
using namespace guest;
constexpr std::uint32_t club_size = 0x248, competition_size = 0x6c, person_size = 0x128;
constexpr std::uint32_t refill = 0x1077b770, formatter = 0x10727890;
namespace slot {
constexpr int pool = -0x41f8, floats = -0x41fc, doubles = -0x4200,
    flag = -0x7bac, message = -0x7ba8, text = -0x7bb8, competitions = -0x7bbc, clubs = -0x7be8, persons = -0x7b98,
    rng_seed = -0x7b90, rng_start = -0x7b8c, rng_end = -0x7b88, rng_cursor = -0x7b84;
}
// Strings at the source pool, after the data file's name.
namespace pool_text {
constexpr unsigned file_name = 0xa, format = 0x14, title = 0x21;
}
// The shared random stream: a cursor walking a table of 16-bit samples, plus
// a seed the refill replaces when the cursor passes the table's end.
struct Stream {
    Cpu& c;
    std::uint32_t cursor, end, start, seed;
    static Stream world(Cpu& c) {
        return {c, c.toc(slot::rng_cursor), c.toc(slot::rng_end), c.toc(slot::rng_start), c.toc(slot::rng_seed)};
    }
    void advance(std::uint32_t back) const {
        const auto next = c.u32(cursor) + 2;
        c.set(3, next);
        c.put32(cursor, next);
        if (compare_logical(c, next, c.u32(end)) > 0) {
            c.put32(cursor, start);
            c.call(refill, back);
            c.put32(seed, c.r(3) & 0xffff);
        }
    }
    // A step the compiler scheduled differently: the scratch registers it left in r3-r10 stand while the end is compared
    // and the refill is called (also when the table does not wrap), and r3 is not set to the new cursor.
    void advance(std::uint32_t back, std::initializer_list<LeftValue> leftovers) const {
        const auto next = c.u32(cursor) + 2;
        c.put32(cursor, next);
        const bool wraps = compare_logical(c, next, c.u32(end)) > 0;
        leave(c, leftovers);
        if (wraps) {
            c.put32(cursor, start);
            c.call(refill, back);
            c.put32(seed, c.r(3) & 0xffff);
        }
    }
    std::uint32_t sample() const { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); }
};
struct Program {
    Cpu& c;
    // Sets r3 on from the arguments, as far as they go.
    std::uint32_t call(std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> args = {}) const {
        unsigned n = 3;
        for (const auto value : args) c.set(n++, value);
        c.call(target, back);
        return c.r(3);
    }
    // `bl` into another routine of this module (the C++ when it runs here, else the generated body), arguments as in call.
    std::uint32_t module_call(std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> args = {}) const {
        unsigned n = 3;
        for (const auto value : args) c.set(n++, value);
        media::call(c, target, back);
        return c.r(3);
    }
    std::uint32_t virtual_call(std::uint32_t object, unsigned offset, std::uint32_t back) const {
        c.set(12, c.u32(c.u32(object) + offset));
        c.call(0x10781320, back);
        c.set(2, c.u32(c.sp() + 0x14));
        return c.r(3);
    }
};
// `bl 0x10727890` (varargs: it spills r3-r10): the first six arguments go to r5-r10, the rest to
// the frame's outgoing words at sp+0x38... Registers past the arguments keep what they hold, so set
// the leftovers the original has in them before calling.
inline void format(Cpu& c, std::uint32_t destination, unsigned phrase, std::uint32_t back,
                   std::initializer_list<std::uint32_t> arguments = {}) {
    c.set(3, destination);
    c.set(4, c.toc(slot::pool) + phrase);
    unsigned index = 0;
    for (const auto argument : arguments) {
        if (index < 6) c.set(5 + index, argument);
        else c.put32(c.sp() + 0x38 + (index - 6) * 4, argument);
        ++index;
    }
    c.call(formatter, back);
}
// The assert macro at `at` (the first instruction of the expansion).
inline void assertion(Cpu& c, const Frame& f, std::uint32_t at, unsigned name, unsigned extension, unsigned line) {
    const auto pool = c.toc(slot::pool);
    report_assertion(c, {at, pool + pool_text::file_name, pool + pool_text::format, pool + pool_text::title, f.at(name), f.at(extension), line});
}
}
