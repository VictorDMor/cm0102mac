#pragma once
#include <algorithm>
#include <cstdint>
#include <span>

namespace cm::aot::cm0001_cups {
struct Candidate {
    std::uint32_t club;
    std::int16_t rank;
};

// Reproduce the Windows 3.89 CRT quicksort (0x8cc933). The ordering of
// equal ranks can decide FA Trophy qualification and Conference Cup entry rounds.
inline void sort_by_rank(std::span<Candidate> clubs) {
    if (clubs.size() <= 8) {
        for (auto end = clubs.size(); end > 1; --end) {
            unsigned weakest = 0;
            for (unsigned i = 1; i < end; ++i)
                if (clubs[i].rank < clubs[weakest].rank) weakest = i;
            std::swap(clubs[weakest], clubs[end - 1]);
        }
        return;
    }
    std::swap(clubs.front(), clubs[clubs.size() / 2]);
    const auto pivot = clubs.front().rank;
    unsigned left = 0, right = clubs.size();
    for (;;) {
        do { ++left; } while (left < clubs.size() && clubs[left].rank >= pivot);
        do { --right; } while (right > 0 && clubs[right].rank <= pivot);
        if (right < left) break;
        std::swap(clubs[left], clubs[right]);
    }
    std::swap(clubs.front(), clubs[right]);
    sort_by_rank(clubs.first(right));
    sort_by_rank(clubs.subspan(left));
}
}
