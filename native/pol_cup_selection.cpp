#include "pol_cup_detail.hpp"

namespace cm::aot::pol_cup {
using namespace detail;
std::uint32_t select_clubs(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x106ce180, 0x1b0, {.first_gpr = 21});
    Kept<22> object(c, address);
    Kept<24> selected(c, 0); Kept<23> candidates(c, 0);
    keep(c, {{25, c.toc(slot::first_division)}, {26, c.toc(slot::second_division)},
             {27, c.toc(slot::excluded_count)}, {28, c.toc(slot::club_count)},
             {29, c.toc(slot::lower_division)}, {30, c.toc(slot::clubs)},
             {21, c.toc(-0x7ba8)}, {31, c.toc(slot::pool)}});
    Cup cup{c, address}; Program p{c};
    cup.club_count() = 52;
    const auto entrants = p.call(0x10775130, 0x106ce1d0, {52 * 8});
    compare_logical(c, entrants); cup.entrants() = entrants;
    if (entrants == 0) {
        assertion(c, f, 0x106ce1f0, 0x40, 0x38, 0x142);
        c.set(4, c.toc(-0x7bac)); return 0xffffffff;
    }
    const auto club_base = c.u32(c.toc(slot::clubs));
    const auto club_limit = c.u32(c.toc(slot::club_count)) - c.u32(c.toc(slot::excluded_count)) * 2;
    const auto lower_id = c.u32(c.toc(slot::lower_division));
    const auto candidate_list = f.at(0x60);
    leave(c, {{3, lower_id}, {4, candidate_list}, {8, club_limit}, {9, club_base}, {10, club_base}});
    compare(c, club_limit);
    // Non-reserve clubs precede reserve clubs in the candidate pool, each in
    // database order, with one shared 70-club limit across both passes.
    for (unsigned reserve = 0; reserve < 2; ++reserve) {
        for (std::uint32_t index = 0; std::int32_t(index) < std::int32_t(club_limit); ++index) {
            const Club club{c, club_base + index * Club::size};
            const auto division = club.division(); c.set(6, division);
            if (compare_logical(c, division) != 0) {
                const auto parent = club.parent_division();
                if (reserve == 0) c.set(7, parent);
                if (reserve != 0 || compare_logical(c, parent) != 0) {
                    const auto count = std::int16_t(std::uint32_t(candidates)); c.set(5, extended(count));
                    if (compare(c, count, 70) < 0) {
                        const auto id = c.u32(division);
                        if (compare(c, id, lower_id) == 0) {
                            if (reserve != 0) c.set(0, parent);
                            if ((compare_logical(c, parent, division) == 0) == (reserve != 0)) {
                                c.put32(candidate_list + std::uint32_t(count) * 4, club.address);
                                candidates = std::uint32_t(candidates) + 1;
                            }
                        }
                    }
                }
            }
            c.set(reserve ? 9 : 10, club.address + Club::size);
        }
        if (reserve == 0) {
            leave(c, {{7, extended(std::int16_t(std::uint32_t(candidates)))},
                      {3, candidate_list}, {4, lower_id}});
            compare(c, club_limit);
        }
    }
    const auto nonreserves = c.r(7);
    const auto total_candidates = extended(std::int16_t(std::uint32_t(candidates)));
    keep(c, {{29, total_candidates}});
    p.call(0x104635b0, 0x106ce334,
           {candidate_list + nonreserves * 4, extended(std::int16_t(total_candidates - nonreserves)),
            extended(std::int16_t(36 - nonreserves)), nonreserves * 4});
    const auto available_clubs = [&] {
        return c.u32(c.toc(slot::club_count)) - c.u32(c.toc(slot::excluded_count)) * 2;
    };
    // Priority in the second division: places 13..20, then all places <=12.
    for (unsigned group = 0; group < 2; ++group) {
        std::uint32_t index = 0, offset = std::uint32_t(selected) * 8;
        auto club_address = c.u32(c.toc(slot::clubs));
        leave(c, {{8, 0}, {9, club_address}, {7, offset}});
        while (true) {
            c.set(3, c.u32(c.toc(slot::excluded_count)) * 2);
            if (compare(c, index, available_clubs()) >= 0) break;
            const Club club{c, club_address};
            const auto division = club.division(); c.set(3, division);
            if (compare_logical(c, division) != 0) {
                const auto id = c.u32(division); c.set(3, id);
                if (compare(c, id, c.u32(c.toc(slot::second_division))) == 0) {
                    const auto position = club.position();
                    const bool eligible = group == 0 ? compare(c, position, 13) >= 0 && compare(c, position, 21) < 0
                                                     : compare(c, position, 12) <= 0;
                    if (eligible) {
                        c.put32(std::uint32_t(cup.entrants()) + offset, club_address);
                        c.put8(std::uint32_t(cup.entrants()) + offset + 5, 0);
                        c.put8(std::uint32_t(cup.entrants()) + offset + 4, 2);
                        leave(c, {{3, cup.entrants()}, {4, 2}, {5, cup.entrants()}, {6, 0}});
                        selected = std::uint32_t(selected) + 1; offset += 8; c.set(7, offset);
                    }
                }
            }
            ++index; club_address += Club::size; leave(c, {{8, index}, {9, club_address}});
        }
    }
    std::uint32_t candidate_index = 0, output_offset = std::uint32_t(selected) * 8;
    auto source = candidate_list;
    leave(c, {{10, 0}, {9, output_offset}, {7, 0}, {3, source}, {5, 3}});
    while (compare(c, selected, 36) < 0 && compare(c, candidate_index, total_candidates) < 0) {
        const auto club = c.u32(source);
        c.put32(std::uint32_t(cup.entrants()) + output_offset, club);
        c.put8(std::uint32_t(cup.entrants()) + output_offset + 5, 0);
        c.put8(std::uint32_t(cup.entrants()) + output_offset + 4, 3);
        source += 4; selected = std::uint32_t(selected) + 1;
        leave(c, {{3, source}, {4, cup.entrants()}, {6, cup.entrants()}, {8, club}});
        output_offset += 8; ++candidate_index; leave(c, {{9, output_offset}, {10, candidate_index}});
    }
    std::uint32_t index = 0, offset = std::uint32_t(selected) * 8;
    auto club_address = c.u32(c.toc(slot::clubs));
    leave(c, {{9, club_address}, {7, offset}, {8, 0}});
    while (true) {
        c.set(3, c.u32(c.toc(slot::excluded_count)) * 2);
        if (compare(c, index, available_clubs()) >= 0) break;
        const auto division = Club{c, club_address}.division(); c.set(3, division);
        if (compare_logical(c, division) != 0) {
            const auto id = c.u32(division); c.set(3, id);
            if (compare(c, id, c.u32(c.toc(slot::first_division))) == 0) {
                c.put32(std::uint32_t(cup.entrants()) + offset, club_address);
                c.put8(std::uint32_t(cup.entrants()) + offset + 5, 0);
                c.put8(std::uint32_t(cup.entrants()) + offset + 4, 1);
                leave(c, {{3, cup.entrants()}, {4, 1}, {5, cup.entrants()}, {6, 0}});
                selected = std::uint32_t(selected) + 1; offset += 8; c.set(7, offset);
            }
        }
        ++index; club_address += Club::size; leave(c, {{8, index}, {9, club_address}});
    }
    if (compare(c, selected, cup.club_count()) != 0)
        p.call(0x107755c0, 0x106ce550, {c.toc(slot::pool) + 0x1f, c.toc(slot::pool), 0x1a1});
    return 1;
}
}
