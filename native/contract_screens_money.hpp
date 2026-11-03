#pragma once

#include "screen_manager_detail.hpp"

namespace cm::aot::contract_screens::detail {
using namespace guest;
using screen_manager::detail::Program;

struct MoneySites {
    std::uint32_t parse, render, length = 0, short_parse = 0, short_render = 0;
};

inline void money_text(Cpu& c, const Program& program, std::uint32_t text, std::uint32_t money, MoneySites sites, unsigned precision = 10) {
    const auto render = [&](unsigned precision, std::uint32_t parse_back, std::uint32_t render_back) {
        const auto number = program.call(0x10009f20, parse_back, {money, precision});
        program.call(0x1000b410, render_back, {text, 2000, number, c.r(4), precision});
    };
    render(precision, sites.parse, sites.render);
    if (sites.length && compare_logical(c, program.call(0x1077dbd0, sites.length, {text}), 8) > 0)
        render(precision - 1, sites.short_parse, sites.short_render);
}

}
