#pragma once
// MANAGER_MANAGER.CPP rematch block: the pieces its checker cases share (manager_manager_checks_rematch*.cpp).
// A case writes the values the rules read into the real records of the career (undone afterwards) or into
// scratch records the real ones point to; the routines outside the block either run for real or are answered.
#include "manager_manager_checks.hpp"
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace cm::aot::manager_manager_checks::rematch {
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
// Scratch records (scratch_base + ...): two region records, an owner club and two rival records, two
// post records, a deal.
namespace scratch {
constexpr std::uint32_t region_a = scratch_base + 0x1000, region_b = scratch_base + 0x1400, owner = scratch_base + 0x2000,
                        owner_region = scratch_base + 0x2800, rival_a = scratch_base + 0x3000, rival_b = scratch_base + 0x3100,
                        post_a = scratch_base + 0x4000, post_b = scratch_base + 0x4100, deal = scratch_base + 0x5000,
                        league_cell = scratch_base + 0x6000, spare = scratch_base + 0x7000;
}
struct Put {
    Case& item;
    void operator()(unsigned size, std::uint32_t at, std::uint32_t value) const { item.memory.push_back({size, at, value}); }
};
// A choice among the values at and around a cut.
inline std::int16_t around(Rng& r, std::int32_t cut) {
    return std::int16_t(cut + r.pick<std::int32_t>({-1, 0, 0, 1, -300, 300, 0x4000}));
}
// People whose staff record, club and names are real (indices into the person table).
inline std::vector<unsigned> staff_people(const Scenario& sc, std::size_t want) {
    std::vector<unsigned> found;
    for (unsigned n = 0; n < sc.people() && found.size() < want; ++n) {
        const auto p = sc.person(n);
        if (sc.r32(p + 0x1c) && sc.r32(p + 0x24) && sc.r32(p + 0x28) && sc.r32(p + 0x2c) && sc.r32(p + 0x30)) found.push_back(n);
    }
    return found;
}
}
