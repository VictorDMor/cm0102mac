// DATABASE.CPP's search block, part 3: the club strength level (database_search.hpp).
#include "database_search.hpp"
#include "database_search_detail.hpp"
#include "guest_values.hpp"

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
std::uint32_t half(Cpu& c, std::uint32_t address) { return std::uint32_t(std::int32_t(c.i16(address))); }
std::uint32_t signed_byte(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int8_t(v))); }
}

// 0x100ddb70: a level 0..6 for club `a` (3 for no club), from the ratings of its best
// players (the sorted squad persons below the id bound, summed over at most 16 and
// never the last) or, with no such player, from the club's own rating; then lowered or
// raised by the rating gap to the other club `b` and by two flags of the clubs' entries
// in the table at TOC -0x7984 (byte +4 of an 8-byte entry indexed by the club's id).
std::int32_t club_level(Cpu& c, std::uint32_t a, std::uint32_t b) {
    guest::Frame frame(c, 0x100ddb70, 0x120, {.first_gpr = 26});
    const auto table_word = c.toc(-0x7984), bound_word = c.toc(-0x7be4);
    guest::keep(c, {{31, table_word}, {26, a}, {27, b}, {30, 0}, {29, 0}, {28, 0}});
    c.set(5, bound_word);
    const bool no_club = compare(c, std::int32_t(a)) == 0;
    if (no_club) {
        c.set(3, 3);
        return 3;
    }
    const auto list = frame.at(0x38);
    std::uint32_t listed = 0;
    c.set(6, 0);
    for (std::uint32_t i = 0;; ++i) {
        if (compare(c, std::int32_t(i), 0x32) >= 0) break;
        const auto person = c.u32(a + i * 4 + 0x98);
        c.set(4, person);
        c.set(6, i);
        if (compare_logical(c, person) == 0) continue;
        const auto linked = c.u32(person + 0x14);
        c.set(0, linked);
        if (compare_logical(c, linked) == 0) continue;
        compare_logical(c, person);  // the null test of the person again: never null here
        const auto id = c.u32(person);
        c.set(3, id);
        const auto bound = c.u32(bound_word);
        c.set(0, bound);
        if (compare(c, std::int32_t(id), std::int32_t(bound)) >= 0) continue;
        c.set(3, list);
        c.set(0, listed * 4);
        c.put32(list + listed * 4, person);
        c.set(30, ++listed);
    }
    c.set(6, c.toc(-0x56c0));
    c.set(4, listed);
    c.set(3, list);
    c.set(5, 4);
    c.call(search::qsort, 0x100ddc18);
    std::uint32_t used = 0, sum = 0;
    c.set(6, 0);
    for (std::uint32_t i = 0;; ++i) {
        if (compare(c, std::int32_t(i), 0x10) >= 0) break;
        const auto last = listed - 1;
        c.set(0, last);
        if (compare(c, std::int32_t(i), std::int32_t(last)) >= 0) break;
        c.set(0, i * 4);
        c.set(3, list);
        const auto person = c.u32(list + i * 4);
        c.set(3, person);
        c.set(28, ++used);
        c.set(6, i + 1);
        const auto player = c.u32(person + 0x14);
        c.set(3, player);
        const auto rating = half(c, player + 4);
        c.set(0, rating);
        sum += rating;
        c.set(29, sum);
    }
    // The level the persons or the club's own rating give.
    std::uint32_t level;
    const auto tier = [&](std::uint32_t value, std::initializer_list<std::pair<std::int32_t, std::uint32_t>> limits,
                          std::uint32_t floor_level, bool reload) {
        for (const auto& [limit, result] : limits) {
            if (reload) {
                c.set(0, half(c, a + 0x1ec));
                value = c.r(0);
            }
            if (compare(c, std::int32_t(value), limit) > 0) {
                c.set(3, result);
                return result;
            }
        }
        c.set(3, floor_level);
        return floor_level;
    };
    if (compare(c, std::int32_t(used)) == 0) {
        level = tier(0, {{0x1c52, 6}, {0x1676, 5}, {0x128e, 4}, {0xcb2, 3}, {0x8ca, 2}}, 1, true);
    } else {
        const auto average = std::uint32_t(std::int32_t(sum) / std::int32_t(used));
        c.set(0, average);
        level = tier(average, {{0x9b, 6}, {0x87, 5}, {0x6e, 4}, {0x55, 3}, {0x3c, 2}, {0x28, 1}}, 0, false);
    }
    // The flag byte of a club's entry in the table: sign-extended into r0, cr0 from it (extsb.).
    const auto entry_flag = [&](std::uint32_t club) {
        const auto id = c.u32(club);
        c.set(0, id << 3);
        const auto entry = c.u32(table_word) + (id << 3);
        c.set(4, entry);
        const auto flag = signed_byte(c.u8(entry + 4));
        c.set(0, flag);
        return compare(c, std::int32_t(flag));
    };
    const auto answer = [&](std::uint32_t value) {
        c.set(3, value);
        return std::int32_t(value);
    };
    // The result is clamped to 0..6 (extsb. / cmpwi 6).
    const auto clamp = [&] {
        const auto value = signed_byte(c.r(3));
        c.set(0, value);
        if (compare(c, std::int32_t(value)) < 0) return answer(0);
        c.set(0, signed_byte(c.r(3)));
        if (compare(c, std::int32_t(signed_byte(c.r(3))), 6) > 0) return answer(6);
        return std::int32_t(c.r(3));
    };
    if (compare_logical(c, b) == 0) return clamp();
    const auto rating_b = half(c, b + 0x1ec);
    c.set(0, rating_b);
    if (compare(c, std::int32_t(rating_b), 0x1c52) > 0) {
        const auto rating_a = half(c, a + 0x1ec);
        c.set(0, rating_a);
        if (compare(c, std::int32_t(rating_a), 0x109a) < 0) {
            const auto current = signed_byte(level);
            c.set(0, current);
            if (compare(c, std::int32_t(current), 2) > 0) {
                c.set(0, level - 2);
                level = signed_byte(level - 2);
                c.set(3, level);
            }
        }
        const auto rating_a2 = half(c, a + 0x1ec);
        c.set(0, rating_a2);
        if (compare(c, std::int32_t(rating_a2), 0x1c52) < 0) {
            const auto current = signed_byte(level);
            c.set(0, current);
            if (compare(c, std::int32_t(current)) > 0) {
                level = level - 1;
                c.set(3, level);
            }
        }
    } else {
        const auto rating_b2 = half(c, b + 0x1ec);
        c.set(0, rating_b2);
        if (compare(c, std::int32_t(rating_b2), 0x109a) < 0) {
            const auto current = signed_byte(level);
            c.set(0, current);
            if (compare(c, std::int32_t(current), 5) < 0) {
                level = level + 1;
                c.set(3, level);
            }
        }
    }
    const auto team_b = c.u32(b + 0x30);
    c.set(4, team_b);
    const auto team_a = c.u32(a + 0x30);
    c.set(0, team_a);
    if (compare_logical(c, team_b, team_a) != 0) return clamp();
    const auto rating_a3 = half(c, a + 0x1ec);
    c.set(4, rating_a3);
    const auto rating_b3 = half(c, b + 0x1ec);
    c.set(0, rating_b3);
    c.set(0, rating_b3 - rating_a3);
    const auto gap = std::uint32_t(std::int32_t(std::int16_t(rating_b3 - rating_a3)));
    c.set(5, gap);
    bool second_chance = true;
    if (compare(c, std::int32_t(gap), 0x4e2) > 0) {
        const auto current = signed_byte(level);
        c.set(0, current);
        if (compare(c, std::int32_t(current)) > 0 && entry_flag(b) >= 0 && entry_flag(a) <= 0) {
            level = level - 1;
            c.set(3, level);
            second_chance = false;
        }
    }
    if (second_chance) {
        c.set(0, gap);
        if (compare(c, std::int32_t(gap), 0x4e2) < 0) {
            const auto current = signed_byte(level);
            c.set(0, current);
            if (compare(c, std::int32_t(current), 5) < 0 && entry_flag(a) >= 0 && entry_flag(b) <= 0) {
                level = level + 1;
                c.set(3, level);
            }
        }
    }
    return clamp();
}
}
