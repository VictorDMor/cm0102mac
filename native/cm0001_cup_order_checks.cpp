// Synthetic club IDs and reputation scores. The expected orders and draw counts
// were captured from CM 3.89's 0x4a6bc0 using the same sample stream below.
#include "cm0001_cup_order.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using cm::aot::cm0001_cups::Candidate;
using cm::aot::cm0001_cups::choose_front;

void check(unsigned count, unsigned places, auto reputation, unsigned window, unsigned expected_draws,
           std::span<const unsigned> prefix) {
    std::vector<Candidate> clubs;
    for (unsigned i = 0; i < count; ++i) clubs.push_back({i, std::int16_t(reputation(i))});
    const auto label = std::to_string(count) + " candidates, " + std::to_string(places) + " places: ";
    unsigned draws = 0;
    const auto draw = [&](unsigned bound) {
        if (bound == 0 || bound != window) throw std::runtime_error(label + "incorrect draw bound");
        const auto sample = 19 + 37 * draws++;
        return sample % bound;
    };
    if (choose_front(clubs, places, draw) != (places <= count))
        throw std::runtime_error(label + "incorrect success result");
    if (draws != expected_draws) throw std::runtime_error(label + "incorrect number of RNG draws");
    for (unsigned i = 0; i < prefix.size(); ++i) {
        if (clubs.at(i).club != prefix[i] || clubs[i].rank != reputation(prefix[i]))
            throw std::runtime_error(label + "wrong club or reputation at position " + std::to_string(i));
    }
}
}

int main() {
    try {
        const auto tied = [](unsigned) { return 0; };
        const auto mixed = [](unsigned i) { return int((i * 17 + 3) % 7) - 3; };
        // The Windows sort changes its treatment of equal keys at nine elements.
        check(8, 5, tied, 0, 0, std::array{1u, 2u, 3u, 4u, 5u, 6u, 7u, 0u});
        check(9, 5, tied, 0, 0, std::array{4u, 1u, 2u, 3u, 0u, 5u, 6u, 7u, 8u});
        // With exactly 18 candidates, the one possible swap still consumes two samples.
        check(18, 18, mixed, 1, 2, std::array{15u, 1u, 8u, 17u, 10u, 3u, 12u, 5u, 7u,
                                               0u, 14u, 2u, 9u, 16u, 11u, 4u, 6u, 13u});
        // Croatia's opening pool has 52 candidates for five league places or 18 cup places.
        constexpr std::array first18{1u, 8u, 29u, 22u, 50u, 43u, 15u, 36u, 45u,
                                     38u, 3u, 31u, 17u, 10u, 24u, 5u, 12u, 26u};
        check(52, 5, mixed, 5, 10, first18);
        check(52, 18, mixed, 5, 10, first18);
        // A window larger than the number of places can introduce a club from below the cutoff.
        check(190, 18, [](unsigned i) { return int(i) - 190; }, 19, 38,
              std::array{188u, 186u, 171u, 184u, 187u, 182u, 185u, 180u, 183u,
                         178u, 181u, 176u, 179u, 174u, 177u, 172u, 175u, 189u});
        constexpr std::array unchanged{0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u,
                                       12u, 13u, 14u, 15u, 16u};
        check(17, 18, mixed, 0, 0, unchanged);
        check(17, 0, mixed, 0, 0, unchanged);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
