#pragma once
#include "award_manager.hpp"
#include "guest_values.hpp"

namespace cm::aot::award_manager::detail {
using namespace guest;

constexpr unsigned award_groups = 29;
constexpr std::uint32_t no_id = 0xffffffff;
struct Manager {
    Cpu& cpu;
    std::uint32_t address;
    std::uint32_t groups() const { return cpu.u32(address + 8); }
    std::uint32_t group(unsigned index) const { return cpu.u32(groups() + index * 4); }
};

inline std::uint32_t call(Cpu& c, std::uint32_t target, std::uint32_t back,
                          std::initializer_list<std::uint32_t> arguments = {}) {
    enter(c, back, arguments);
    c.call(target, back);
    return c.r(3);
}

inline void virtual_call(Cpu& c, std::uint32_t object, unsigned slot, std::uint32_t back) {
    c.set(12, c.u32(c.u32(object) + slot));
    c.call(0x10781320, back);
    c.set(2, c.u32(c.sp() + 0x14));
}

inline void assertion(Cpu& c, const Frame& frame, std::uint32_t at,
                      unsigned name, unsigned extension, unsigned line) {
    const auto source = c.toc(-0x4a84);
    report_assertion(c, {at, source + 0x6a, source + 0x7c, source + 0x89,
                        frame.at(name), frame.at(extension), line});
}

// Parent checks and --only need the same dispatcher boundary as a generated caller.
bool dispatch_owned(std::uint32_t entry);
}
