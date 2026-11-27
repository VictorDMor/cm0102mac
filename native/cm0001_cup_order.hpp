#pragma once
#include <algorithm>
#include <cstdint>
#include <span>

namespace cm::aot::cm0001_cups {
struct Candidate {
    std::uint32_t club;
    std::int16_t rank;
};

// Windows 3.89 CRT quicksort (0x8cc933), in descending key order. The
// ordering of equal keys can decide qualification and cup entry rounds.
template<class T, class Key>
void sort_descending(std::span<T> clubs, Key key) {
    if (clubs.size() <= 8) {
        for (auto end = clubs.size(); end > 1; --end) {
            unsigned weakest = 0;
            for (unsigned i = 1; i < end; ++i)
                if (key(clubs[i]) < key(clubs[weakest])) weakest = i;
            std::swap(clubs[weakest], clubs[end - 1]);
        }
        return;
    }
    std::swap(clubs.front(), clubs[clubs.size() / 2]);
    const auto pivot = key(clubs.front());
    unsigned left = 0, right = clubs.size();
    for (;;) {
        do { ++left; } while (left < clubs.size() && key(clubs[left]) >= pivot);
        do { --right; } while (right > 0 && key(clubs[right]) <= pivot);
        if (right < left) break;
        std::swap(clubs[left], clubs[right]);
    }
    std::swap(clubs.front(), clubs[right]);
    sort_descending(clubs.first(right), key);
    sort_descending(clubs.subspan(left), key);
}

inline void sort_by_rank(std::span<Candidate> clubs) {
    sort_descending(clubs, [](const Candidate& club) { return club.rank; });
}

// COMP_UTIL in 3.89 sorts all candidates by reputation, then makes one
// pair of draws per club in the first tenth, swapping within that tenth.
template<class Draw>
bool choose_front(std::span<Candidate> clubs, unsigned places, Draw draw) {
    if (clubs.size() < places) return false;
    if (places == 0) return true;
    sort_by_rank(clubs);
    const auto window = static_cast<unsigned>(clubs.size() / 10);
    for (unsigned i = 0; i < window; ++i) {
        const auto first = draw(window);
        const auto second = draw(window);
        std::swap(clubs[first], clubs[second]);
    }
    return true;
}
}
