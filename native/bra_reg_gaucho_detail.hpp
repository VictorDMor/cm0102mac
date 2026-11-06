#pragma once
#include "bra_reg_gaucho.hpp"
#include "guest_values.hpp"

namespace cm::aot::bra_reg_gaucho::detail {
using namespace guest;
#if defined(CM_BRA_REG_GAUCHO_CHECKS)
void observe_entry(Cpu&, std::uint32_t);
#else
inline void observe_entry(Cpu&, std::uint32_t) {}
#endif
constexpr std::uint32_t missing = 0xffffffff, standing_size = 0x44, club_size = 0x248;
namespace slot {
constexpr int pool = -0x407c, vtable = -0x5ca8, regional = -0x6b64,
    flag = -0x7bac, message = -0x7ba8, text = -0x7bb8, history = -0x7b48,
    club_total = -0x7b50, clubs = -0x7be8, schedule_events = -0x7428;
}
// Strings at the source pool, after the file name, version format and title.
namespace pool_text {
constexpr unsigned format = 0x13, title = 0x20, qualified = 0x26, qualified_short = 0x9c, champions = 0xd6;
}
struct Competition {
    Cpu& c;
    std::uint32_t address;
    Field<std::uint32_t> definition() const { return {c, address + 4}; }
    Field<std::uint32_t> fixtures() const { return {c, address + 8}; }
    Field<std::uint32_t> stages() const { return {c, address + 0xc}; }
    Field<std::int32_t> stage_count() const { return {c, address + 0x2c}; }
    Field<std::int32_t> last_stage() const { return {c, address + 0x30}; }
    Field<std::int16_t> club_count() const { return {c, address + 0x3e}; }
    Field<std::int16_t> year() const { return {c, address + 0x40}; }
    Field<std::uint32_t> standings() const { return {c, address + 0xb0}; }
    Field<std::uint32_t> schedule() const { return {c, address + 0xbc}; }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target, std::uint32_t back,
                       std::initializer_list<std::uint32_t> args = {}) const {
        unsigned n = 3;
        for (const auto value : args) c.set(n++, value);
        c.call(target, back);
        return c.r(3);
    }
    template<class Function> void owned(std::uint32_t target, std::uint32_t back, Function body) const {
        if (defer_owned(c, target, back)) return;
        c.link(back); body();
    }
    std::uint32_t virtual_call(std::uint32_t object, unsigned offset, std::uint32_t back) const {
        c.set(12, c.u32(c.u32(object) + offset));
        c.call(0x10781320, back);
        c.set(2, c.u32(c.sp() + 0x14));
        return c.r(3);
    }
};
inline void assertion(Cpu& c, const Frame& f, std::uint32_t at, unsigned name,
                      unsigned extension, unsigned line) {
    const auto pool = c.toc(slot::pool);
    report_assertion(c, {at, pool, pool + pool_text::format, pool + pool_text::title,
                         f.at(name), f.at(extension), line});
}
}
