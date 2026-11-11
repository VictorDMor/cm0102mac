#pragma once
// Helpers the tail block's case files share: a seeded generator and the module's draw table written for a case.
#include "manager_manager_checks.hpp"
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace cm::aot::manager_manager_checks::tail {
struct Rng {
    std::uint64_t state;
    std::uint32_t next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return std::uint32_t(state >> 33);
    }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    bool chance(unsigned percent) { return below(100) < percent; }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + below(unsigned(values.size()))); }
};

// Writes the draw table so that the case's draws come out as `targets` (each a halfword plus the table's base): the
// cursor sits at the first entry (or at the last, to make the first draw wrap and call the redraw, answered with
// `redraw`), the base is 0 (or the redraw's low halfword after a wrap).
inline void put_draws(const Scenario& sc, Case& item, bool wrap, std::uint32_t redraw,
                      const std::vector<std::uint32_t>& targets) {
    const auto first = sc.toc(-0x7b8c), last = sc.r32(sc.toc(-0x7b88));
    const std::uint32_t base = wrap ? redraw & 0xffff : 0;
    item.memory.push_back({4, sc.toc(-0x7b84), wrap ? last : first});
    item.memory.push_back({4, sc.toc(-0x7b90), 0});
    for (std::size_t n = 0; n < targets.size(); ++n)
        item.memory.push_back({2, first + 2 * std::uint32_t(n + (wrap ? 0 : 1)), (targets[n] - base) & 0xffff});
    item.answers.push_back({0x1077b770, redraw});
}
}
