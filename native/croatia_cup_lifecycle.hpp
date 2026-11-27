#pragma once
#include "comp_detail.hpp"

namespace cm::aot::croatia_cup {
// Keep the descriptor ABI (including its TOC and stack save) when calling a
// known native implementation. An object supplying another override retains it.
template<class Body>
std::uint32_t cup_method(guest::Cpu& c, std::uint32_t cup, std::uint32_t slot, std::uint32_t expected,
                         std::uint32_t back, Body body) {
    const auto descriptor = c.u32(c.u32(cup) + slot);
    const auto entry = c.u32(descriptor);
    if (entry != expected) return comp::virtual_call(c, slot, back, {cup});
    c.set({{3, cup}, {12, descriptor}, {0, entry}});
    c.link(back);
    c.put32(c.sp() + 0x14, c.r(2));
    c.s.wr<4>(guest::reg::ctr, entry);
    c.set(2, c.u32(descriptor + 4));
    body();
    c.set(2, c.u32(c.sp() + 0x14));
    return c.r(3);
}

struct Profile;
const Profile& season_profile(guest::Cpu&, std::uint32_t cup);
std::uint32_t configure(guest::Cpu&, std::uint32_t cup, const Profile&);
std::uint32_t construct(guest::Cpu&, std::uint32_t cup, std::uint32_t year, std::uint32_t definition);
std::uint32_t destroy(guest::Cpu&, std::uint32_t cup, std::uint32_t deleting);
void next_season(guest::Cpu&, std::uint32_t cup);
}
