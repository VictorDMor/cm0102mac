#pragma once
#include "nation_awards.hpp"
#include "guest_values.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::nation_awards::detail {
using namespace guest;
constexpr std::uint32_t missing = 0xffffffff;
struct Person {
    Cpu& c;
    std::uint32_t address;
    static constexpr unsigned size = 0x70;
    std::uint32_t id() const { return c.u32(address); }
    std::uint32_t club() const { return c.u32(address + 0x24); }
    std::uint32_t name(unsigned part) const { return c.u32(address + 0x28 + part * 4); }
};
struct Award {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t ratings() const { return c.u32(address + 0x24); }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target, std::uint32_t back,
                       std::initializer_list<std::uint32_t> arguments = {}) const {
        unsigned r = 3;
        for (auto value : arguments) c.set(r++, value);
        c.call(target, back);
        return c.r(3);
    }
    void virtual_call(std::uint32_t object, unsigned method, std::uint32_t back) const {
        c.set(12, c.u32(c.u32(object) + method));
        c.call(0x10781320, back);
        c.set(2, c.u32(c.sp() + 0x14));
    }
};
inline std::uint32_t indirect(Cpu& c, int slot) { return c.u32(c.toc(slot)); }
inline void assertion(Cpu& c, const Frame& frame, std::uint32_t at,
                      unsigned name, unsigned extension, unsigned line, bool reload_toc = false) {
    const auto pool = c.toc(-0x4a18);
    report_assertion(c, {at, pool, pool + 0x12, pool + 0x1f,
                        frame.at(name), frame.at(extension), line},
        [&](std::uint32_t target,std::uint32_t back) {
            // The global/recent selectors reload TOC pointers between these
            // calls; those extra loads change their original return sites.
            if(reload_toc && target==0x1077b720)back+=4;
            if(reload_toc && target==0x10769b60)back+=8;
            c.call(target,back);
        });
}
inline void store_winner(Cpu& c, std::uint32_t record, std::uint32_t first,
                         std::uint32_t second, std::uint32_t third,
                         std::uint32_t person, std::uint32_t club) {
    c.put32(record, first); c.put32(record + 4, second); c.put32(record + 8, third);
    c.put32(record + 20, club); c.put32(record + 12, person);
}
}
