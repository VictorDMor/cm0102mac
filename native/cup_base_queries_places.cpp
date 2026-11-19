// CUP.CPP, block `queries`: the small queries on a cup's rounds and match rows. The entrants of a tie's sides
// (0x10484b90 and its recursion 0x10484e10), the place bytes of an entrant (0x10484fd0, 0x10486480), a match's flags
// (0x10485340), the round difference (0x104858b0) and the comparison of two fixtures' records (0x10485ab0). The
// original's volatile registers are what callers read after some of them (COMP_SCREENS after 0x10484b90, r5-r10;
// process_fixture after 0x10484fd0, r9), and the virtual +0x70 the places go to reads r3-r10: each path sets the
// registers the original sets.
#include "cup_base_queries_detail.hpp"

namespace cm::aot::cup_base::queries {
namespace {
constexpr std::int32_t counter_slot = -0x3764;   // the word 0x10484b90's walk counts in (a TOC word itself)
std::uint32_t half_word(Cpu& c, std::uint32_t at) { return sext16(c.u16(at)); }
}

std::uint32_t side_entrants(Cpu& c, std::uint32_t cup, std::int32_t round, std::int32_t tie, std::uint32_t first_side,
                            std::uint32_t out) {
    const Frame frame(c, 0x10484b90, 0xf0, {.first_gpr = 29});
    const auto flag = c.toc(toc::success), message = c.toc(toc::message), pool = c.toc(toc::pool);
    c.set({{29, flag}, {30, message}, {31, pool}});
    const auto counter = c.r(2) + std::uint32_t(counter_slot);
    if (round < 0 || std::int32_t(c.i16(cup + competition::rounds)) < round) {
        report(c, frame, {0x10484bdc, 0xb8, 0x50, 0x153c});
        return 0;
    }
    if (c.u32(cup + cup::entrants) == 0) {
        report(c, frame, {0x10484c40, 0x98, 0x48, 0x1541});
        return 0;
    }
    if (tie == -1) {
        report(c, frame, {0x10484ca0, 0x78, 0x40, 0x1546});
        return 0;
    }
    c.put32(counter, 0);
    const bool first = std::int8_t(first_side) != 0;
    const auto offset = std::uint32_t(tie) * cup::match_row;
    const auto table = c.u32(cup + cup::round_matches);
    const auto row = c.u32(table + std::uint32_t(round) * 4);
    c.set({{5, offset}, {6, table}, {8, row}});
    // The side's own entrant when decided, else the walk through the feeder tie (round and tie from the match row).
    const auto side = half_word(c, row + offset + (first ? tie::side_0 : tie::side_1));
    if (side != 0xffffffffu) {
        c.put16(out, side);
        c.put32(counter, 1);
    } else {
        const auto feeder_round = half_word(c, row + offset + (first ? tie::feeder_round_0 : tie::feeder_round_1));
        const auto feeder_tie = half_word(c, row + offset + (first ? tie::feeder_tie_0 : tie::feeder_tie_1));
        c.set({{4, feeder_round}, {5, feeder_tie}, {6, out}, {7, counter}});
        c.link(first ? 0x10484d3c : 0x10484d8c);
        append_entrants(c, cup, std::int32_t(feeder_round), std::int32_t(feeder_tie), out, counter);
    }
    if (c.u32(counter) == 0) report(c, frame, {0x10484dac, 0x58, 0x38, 0x1571});
    return c.u32(counter);
}

std::uint32_t append_entrants(Cpu& c, std::uint32_t cup, std::int32_t round, std::int32_t tie, std::uint32_t out,
                              std::uint32_t counter) {
    const Frame frame(c, 0x10484e10, 0xb0, {.first_gpr = 24});
    // r24 the success flag, r25 the message buffer, r26 the pool, r27 the cup, r28 the round, r29 the tie, r30 the output,
    // r31 the counter.
    c.set({{24, c.toc(toc::success)}, {25, c.toc(toc::message)}, {26, c.toc(toc::pool)}, {27, cup}, {28, std::uint32_t(round)},
           {29, std::uint32_t(tie)}, {30, out}, {31, counter}});
    if (out == 0 || counter == 0) {
        report(c, frame, {0x10484e60, 0x68, 0x40, 0x157e});
        return c.u32(counter);
    }
    if (round < 0 || tie == -1) {
        report(c, frame, {0x10484ec8, 0x48, 0x38, 0x1584});
        return c.u32(counter);
    }
    const auto offset = std::uint32_t(tie) * cup::match_row;
    const auto matches = c.u32(cup + cup::round_matches);
    const auto row = c.u32(matches + std::uint32_t(round) * 4);
    const auto append = [&](std::uint32_t side) {
        c.put16(out + c.u32(counter) * 2, side);
        c.put32(counter, c.u32(counter) + 1);
    };
    // Side 0, then side 1; an undecided side expands through its feeder tie. The first recursion is made with r3 as it
    // came in and r6/r7 untouched; the second sets r3-r7 again.
    c.set({{4, matches}, {5, offset}, {8, row}});
    const auto first = half_word(c, row + offset + tie::side_0);
    if (first == 0xffffffffu) {
        const auto feeder_round = half_word(c, row + offset + tie::feeder_round_0);
        const auto feeder_tie = half_word(c, row + offset + tie::feeder_tie_0);
        c.set({{4, feeder_round}, {5, feeder_tie}});
        c.link(0x10484f3c);
        append_entrants(c, cup, std::int32_t(feeder_round), std::int32_t(feeder_tie), out, counter);
    } else {
        c.set(3, c.u32(counter));
        append(first);
    }
    const auto next_matches = c.u32(cup + cup::round_matches);
    const auto next_row = c.u32(next_matches + std::uint32_t(round) * 4);
    const auto second = half_word(c, next_row + offset + tie::side_1);
    c.set({{4, offset}, {5, next_row}, {3, second}});
    if (second == 0xffffffffu) {
        const auto feeder_round = half_word(c, next_row + offset + tie::feeder_round_1);
        const auto feeder_tie = half_word(c, next_row + offset + tie::feeder_tie_1);
        c.set({{4, feeder_round}, {3, cup}, {5, feeder_tie}, {6, out}, {7, counter}});
        c.link(0x10484f98);
        append_entrants(c, cup, std::int32_t(feeder_round), std::int32_t(feeder_tie), out, counter);
    } else {
        append(second);
    }
    return c.u32(counter);
}

void record_entrant_places(Cpu& c, std::uint32_t cup, std::uint32_t entrant, std::uint32_t round) {
    const Frame frame(c, 0x10484fd0, 0x40, {});
    const auto count = half_word(c, cup + competition::rounds);
    // The virtual +0x70 (the competition's own, 0x10451f80 in the base class) with the club, the cup's owner byte and three
    // place bytes; it reads r3-r10, of which r9 and r10 are set by the paths below.
    const auto tell = [&](std::uint32_t club, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t back) {
        virtual_call(c, 0x70, back, {cup, club, std::uint32_t(c.u8(cup + competition::owner)), a, b, d});
    };
    const auto state_at = [&](std::uint32_t row) { return std::int8_t(c.u8(row + 5)); };
    const auto place_byte = [&](std::uint32_t rounds, std::uint32_t index) {
        return std::uint32_t(c.u8(rounds + index * cup::round_row + round::first_place));
    };
    const auto ties_plus_one = [&](std::uint32_t rounds, std::uint32_t index) {
        return (half_word(c, rounds + index * cup::round_row + round::ties) + 1) & 0xff;
    };
    c.set(10, count);
    const auto index = sext16(entrant);
    if (std::int32_t(count) > 0) {
        const auto last = count - 1;
        const auto rounds = c.u32(cup + cup::rounds);
        c.set({{8, last}, {9, rounds}, {6, last * cup::round_row}});
        if (std::int32_t(half_word(c, rounds + last * cup::round_row + round::label)) == 0x8c) {
            // The last round is the 0x8c one: the ties before it pay by the entrant's states 1-4.
            const auto at = sext16(round), before_last = count - 2;
            c.set({{7, at}, {6, before_last}});
            const auto entrants = c.u32(cup + cup::entrants);
            c.set({{5, entrants}, {4, index << 3}});
            const auto row = entrants + (index << 3);
            if (std::int32_t(at) == std::int32_t(before_last)) {
                if (state_at(row) == 1) tell(c.u32(row), 1, 1, 1, 0x10485054);
                else if (state_at(row) == 2) tell(c.u32(row), 2, 2, 2, 0x10485084);
                return;
            }
            if (std::int32_t(at) == std::int32_t(last)) {
                if (state_at(row) == 3) tell(c.u32(row), 3, 3, 3, 0x104850d4);
                else if (state_at(row) == 4) tell(c.u32(row), 4, 4, 4, 0x10485104);
                return;
            }
            if (state_at(row) != 0) {
                const auto place = place_byte(rounds, at);
                tell(c.u32(row), place, place, place, 0x104851b4);
            } else if (std::int32_t(at + 1) == std::int32_t(before_last)) {
                tell(c.u32(row), 4, 1, 4, 0x10485154);
            } else {
                const auto place = place_byte(rounds, at + 1);
                tell(c.u32(row), place, 1, place, 0x10485184);
            }
            return;
        }
    }
    // The ordinary cup: the last round pays by state 1 or 2, every other by the round's own place byte.
    const auto at = sext16(round), last = half_word(c, cup + competition::rounds) - 1;
    c.set({{6, last + 1}, {10, at}, {5, last}});
    const auto entrants = c.u32(cup + cup::entrants);
    if (std::int32_t(at) == std::int32_t(last)) {
        c.set({{7, entrants}, {6, index << 3}});
        const auto row = entrants + (index << 3);
        if (state_at(row) == 1) {
            tell(c.u32(row), 1, 1, 1, 0x10485210);
        } else if (state_at(row) == 2) {
            const auto x = ties_plus_one(c.u32(cup + cup::rounds), at);
            tell(c.u32(row), x, x, x, 0x10485254);
        }
        return;
    }
    c.set({{9, entrants}, {8, index << 3}});
    const auto row = entrants + (index << 3);
    const auto rounds = c.u32(cup + cup::rounds);
    if (state_at(row) != 0) {
        const auto place = place_byte(rounds, at);
        tell(c.u32(row), place, place, place, 0x10485320);
    } else if (std::int32_t(at + 1) == std::int32_t(last)) {
        const auto x = ties_plus_one(rounds, at + 1);
        tell(c.u32(row), x, 1, x, 0x104852b8);
    } else {
        const auto place = place_byte(rounds, at + 1);
        tell(c.u32(row), place, 1, place, 0x104852ec);
    }
}

void settle_match_flags(Cpu& c, std::uint32_t cup, std::uint32_t match, std::uint32_t tie_index) {
    const auto round_offset = std::uint32_t(c.i16(match + fixture::round)) * cup::round_row;
    const auto rounds = c.u32(cup + cup::rounds);
    c.set({{6, round_offset}, {7, rounds}, {8, sext16(c.u16(match + fixture::flags))}});
    c.put16(match + fixture::flags, c.u16(match + fixture::flags) | c.u16(rounds + round_offset + round::flags));
    const auto flags = sext16(c.u16(match + fixture::flags));
    c.set(8, flags);
    if ((flags & 0x800) == 0) return;
    const auto matches = c.u32(cup + cup::round_matches);
    const auto row = c.u32(matches + std::uint32_t(c.i16(match + fixture::round)) * 4);
    const auto offset = tie_index * cup::match_row;
    const auto entrants = c.u32(cup + cup::entrants);
    const auto side_0 = half_word(c, row + offset + tie::side_0), side_1 = half_word(c, row + offset + tie::side_1);
    const auto state_0 = sext8(c.u8(entrants + (side_0 << 3) + 4)), state_1 = sext8(c.u8(entrants + (side_1 << 3) + 4));
    c.set({{3, state_0}, {5, (side_0 << 3) + 4}, {6, row}, {7, entrants}});
    // Bit 0x800 is cleared, then the smaller state of side 0 sets 0x1000, the larger 0x800, equal states 1.
    c.put16(match + fixture::flags, flags ^ 0x800);
    const auto cleared = sext16(c.u16(match + fixture::flags));
    const auto set = std::int32_t(state_0) < std::int32_t(state_1) ? 0x1000u : std::int32_t(state_0) > std::int32_t(state_1) ? 0x800u : 1u;
    c.put16(match + fixture::flags, cleared | set);
}

std::int32_t round_difference(Cpu& c, std::uint32_t cup, std::uint32_t match, std::uint32_t reverse, std::uint32_t out) {
    const auto round = half_word(c, match + fixture::round), rounds = half_word(c, cup + competition::rounds);
    const auto left = sext8(rounds - round);
    auto value = std::int32_t(std::int8_t(c.u8(cup + competition::byte_4b) - left));
    if (value < 1) value = 1;
    const auto byte = std::uint32_t(c.u8(match + fixture::byte_4b));
    c.set({{7, byte}, {8, round}});
    if (std::int8_t(byte) == -1) return value;
    const auto difference = std::int8_t(reverse) == 0 ? std::uint32_t(c.u8(match + fixture::byte_4c)) - byte
                                                      : byte - std::uint32_t(c.u8(match + fixture::byte_4c));
    c.set(4, difference);
    c.put8(out, difference + 1);
    if (c.i8(out) < 1) c.put8(out, 0xff);
    return value;
}

std::uint32_t lower_record(Cpu& c, std::uint32_t, std::uint32_t first, std::uint32_t second) {
    const auto record_of = [&](std::uint32_t at) {
        const auto record = c.u32(at + 0x30);
        return record ? half_word(c, record + 0x18) : 0u;
    };
    const auto a = std::int32_t(std::int16_t(record_of(first)));
    const auto b = record_of(second);
    c.set(6, b);
    if (a < std::int32_t(std::int16_t(b))) return first;
    if (std::int32_t(std::int16_t(b)) < a) return second;
    return 0;
}

std::uint32_t record_group_places(Cpu& c, std::uint32_t cup, std::uint32_t club) {
    const Frame frame(c, 0x10486480, 0x40, {});
    virtual_call(c, 0x70, 0x104864ac, {cup, club, std::uint32_t(c.u8(cup + competition::owner)), 1, 1,
                                       c.u16(cup + competition::club_count) & 0xffu});
    return 1;
}

}
