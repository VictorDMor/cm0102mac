#include "shortlist_manager_detail.hpp"

namespace cm::aot::shortlist_manager {
using namespace detail;
namespace {
std::int32_t word(Cpu& c, std::uint32_t address) { return Field<std::int32_t>{c, address}; }
std::int32_t difference(std::int32_t a, std::int32_t b) { return std::int32_t(std::uint32_t(a) - std::uint32_t(b)); }

// The pair comparators: `major` and `minor` are the offsets (0 or 4) of the
// word compared first and of the one compared when those are equal.
std::uint32_t compare_pairs(Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t major, std::uint32_t minor) {
    if (!nonzero_logical(c, a)) return std::uint32_t(-1);
    if (!nonzero_logical(c, b)) return 1;
    const std::int32_t left = word(c, a + major), right = word(c, b + major);
    c.set(5, std::uint32_t(left));
    if (compare(c, left, right) != 0) return std::uint32_t(difference(left, right));
    const std::int32_t left_minor = word(c, a + minor);
    if (compare(c, left_minor, -1) == 0) return 0;
    const std::int32_t right_minor = word(c, b + minor);
    if (compare(c, right_minor, -1) == 0) return 0;
    return std::uint32_t(difference(left_minor, right_minor));
}
}

std::uint32_t compare_by_value_then_key(Cpu& c, std::uint32_t a, std::uint32_t b) {
    const std::int32_t right = word(c, b + 8);
    c.set(5, std::uint32_t(right));
    const std::int32_t by_value = difference(word(c, a + 8), right);
    if (nonzero(c, by_value)) return std::uint32_t(by_value);
    const std::int32_t right_key = word(c, b + 4);
    c.set(4, std::uint32_t(right_key));
    return std::uint32_t(difference(word(c, a + 4), right_key));
}

std::uint32_t compare_linked_first(Cpu& c, std::uint32_t a, std::uint32_t b) {
    if (compare(c, word(c, a + 0x10), -1) > 0) return 1;
    if (compare(c, word(c, b + 0x10), -1) > 0) return std::uint32_t(-1);
    if (compare(c, word(c, a + 0x18), -1) > 0) return 1;
    if (compare(c, word(c, b + 0x18), -1) > 0) return std::uint32_t(-1);
    const std::int32_t left = word(c, a + 0xc);
    c.set(5, std::uint32_t(left));
    // Descending; quirk: equal values compare as 1, not 0.
    const std::int32_t descending = difference(word(c, b + 0xc), left);
    return nonzero(c, descending) ? std::uint32_t(descending) : 1;
}

std::uint32_t compare_by_rank(Cpu& c, std::uint32_t a, std::uint32_t b) {
    const std::int32_t right = Field<std::int8_t>{c, b + 0x24};
    c.set(4, std::uint32_t(right));
    // Quirk: equal ranks compare as 1, not 0.
    const std::int32_t by_rank = difference(Field<std::int8_t>{c, a + 0x24}, right);
    return nonzero(c, by_rank) ? std::uint32_t(by_rank) : 1;
}

std::uint32_t compare_pairs_by_second(Cpu& c, std::uint32_t a, std::uint32_t b) { return compare_pairs(c, a, b, 4, 0); }
std::uint32_t compare_pairs_by_first(Cpu& c, std::uint32_t a, std::uint32_t b) { return compare_pairs(c, a, b, 0, 4); }
}
