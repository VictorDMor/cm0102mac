// DATABASE.CPP's search block, part 5: 0x100c8930, the removal that mirrors add_to_record
// (0x100c82a0): it takes an id record out of the slot its kind names in a record
// (database_search.hpp).
#include "database_search.hpp"
#include "database_records.hpp"
#include "database_records_detail.hpp"
#include "guest_values.hpp"

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
using namespace records;

// The kinds (as add_to_record fills them, id_slot).
constexpr std::int32_t no_kind = 0, word_80_kind = 1, list_84_first = 2, list_84_last = 4, word_90_kind = 5,
                       word_94_first = 6, word_94_last = 7, list_160_kind = 8, list_174_kind = 9, list_190_kind = 10,
                       list_98_kind = 11, word_90_list_98_kind = 12, word_94_list_98_kind = 13, list_160_list_98_kind = 15;

// Looks for `person` in the `count` slots at `offset` of `club`, as the original's counting loop does: r3 the
// index, r0 the slot read, r4 the slot's address, cr0 the last compare (logical, then the index's).
// Returns the index when found, else `count`; afterwards compares the index with the count again, as the
// original does at the loop's exit.
std::uint32_t find_slot(Cpu& c, std::uint32_t club, std::uint32_t offset, std::int32_t count, std::uint32_t person) {
    std::uint32_t index = 0;
    c.set(3, 0);
    while (compare(c, std::int32_t(index), count) < 0) {
        c.set(0, index << 2);
        c.set(4, club + (index << 2));
        const auto slot = c.u32(club + (index << 2) + offset);
        c.set(0, slot);
        if (compare_logical(c, slot, person) == 0) break;
        c.set(3, ++index);
    }
    compare(c, std::int32_t(index), count);  // `cmpwi r3,count; blt found`
    return index;
}
}

bool remove_from_record(Cpu& c, std::uint32_t club, std::uint32_t person, std::uint32_t date, std::uint32_t flag_a,
                        std::uint32_t flag_b) {
    c.set(0, c.lr());
    guest::Frame frame(c, 0x100c8930, 0x130, {.first_gpr = 22});
    const auto low_slot = c.toc(slot::range_low), success = c.toc(slot::success), message = c.toc(slot::message),
               pool = c.toc(slot::text_pool), high_slot = c.toc(slot::range_high);
    guest::keep(c, {{22, club}, {27, low_slot}, {28, success}, {23, person}, {29, message}, {24, date}, {30, pool},
                    {25, flag_a}, {31, high_slot}, {26, flag_b}});
    const auto answer = [&](std::uint32_t value) {
        c.set(3, value);
        return value != 0;
    };
    if (compare(c, std::int32_t(club)) == 0) return answer(0);
    if (compare_logical(c, person) == 0) {
        fail_assertion(c, frame, 0x100c8990, 0x363a, 0xe0, 0x58, 0, false);
        return false;
    }
    if (compare_logical(c, date) != 0) {
        // (The null-club assert on this path is dead code: the club was just found non-null.)
        compare_logical(c, club);
        const bool below = below_range_limit(c, club);
        const auto fields = below ? 0x4cu : 0x3cu;
        c.set(0, c.u16(date + 2));
        c.put16(person + fields + 2, c.r(0));
        c.set(0, guest::extended(c.i16(date)));
        c.put16(person + fields, c.r(0));
        c.set(0, c.u8(date + 4));
        c.put8(person + fields + 4, c.r(0));
    }
    compare_logical(c, club);
    const bool below = below_range_limit(c, club);
    // The person's kind: the byte at +0x62 below the range limit, else +0x60.
    c.set(3, c.u8(person + (below ? 0x62 : 0x60)));
    auto kind = c.r(3);
    if (compare(c, guest::extended(std::int8_t(flag_b))) != 0) {
        const auto reduced = (kind - 0xb) & 0xff;
        c.set(0, reduced);
        bool wanted;
        if (compare_logical(c, reduced, 2) <= 0) {
            wanted = true;
        } else {
            const auto number = guest::extended(std::int8_t(kind));
            c.set(0, number);
            wanted = compare(c, std::int32_t(number), 0xf) == 0;
        }
        c.set(0, wanted);
        compare(c, wanted);
        kind = wanted ? 0xb : 0;
        c.set(3, kind);
    }
    const auto number = guest::extended(std::int8_t(kind));
    c.set(0, number);
    if (compare_logical(c, number, 0xf) > 0) return answer(0);
    // The jump table at TOC -0x5670: the handler per kind; r3 holds it and CTR too.
    constexpr std::uint32_t handlers[16] = {0x100c8fa4, 0x100c8ba0, 0x100c8bc0, 0x100c8bc0, 0x100c8bc0, 0x100c8c0c,
                                            0x100c8ce8, 0x100c8ce8, 0x100c8d08, 0x100c8d54, 0x100c8da0, 0x100c8dec,
                                            0x100c8e38, 0x100c8ea0, 0x100c8f9c, 0x100c8f08};
    c.set(0, number << 2);
    c.set(3, handlers[number]);
    c.s.wr<4>(guest::reg::ctr, handlers[number]);
    const auto no = [&] { return answer(0); };
    // A single id word that must hold the person, then is cleared.
    const auto clear_word = [&](std::uint32_t offset) {
        const auto word = c.u32(club + offset);
        c.set(0, word);
        if (compare_logical(c, word, person) != 0) return false;
        c.set(0, 0);
        c.put32(club + offset, 0);
        return true;
    };
    // Removes the person from the list at `offset` (clears its slot).
    const auto clear_in_list = [&](std::uint32_t offset, std::int32_t count) {
        const auto index = find_slot(c, club, offset, count, person);
        if (std::int32_t(index) >= count) return false;
        c.set(0, index << 2);
        c.set(4, 0);
        c.set(3, club + (index << 2));
        c.put32(club + (index << 2) + offset, 0);
        return true;
    };
    switch (number) {
    case no_kind: break;
    case word_80_kind:
        if (!clear_word(id_slot::word_80)) return no();
        break;
    case list_84_first: case 3: case list_84_last:
        if (!clear_in_list(id_slot::list_84, id_slot::list_84_count)) return no();
        break;
    case word_90_kind: {
        if (!clear_word(id_slot::word_90)) return no();
        compare_logical(c, club);  // never null here
        const bool below_limit = below_range_limit(c, club);
        if (!below_limit) {
            // Not below the limit: the club's range position is asked for; when it is 0 the person
            // is also taken out of the record of the owner's club at +0x90.
            guest::enter(c, 0x100c8cc0, {club});
            const auto position = range_position(c, club);
            const auto flag = guest::extended(std::int8_t(position));
            c.set(0, flag);
            if (compare(c, std::int32_t(flag)) == 0) {
                c.set(3, c.u32(club + 0x2c));
                guest::enter(c, 0x100c8cd0, {c.r(3)});
                const auto owner = club_at_bias(c, c.r(3));
                const auto word = c.u32(owner + id_slot::word_90);
                c.set(0, word);
                if (compare_logical(c, word, person) == 0) {
                    c.set(0, 0);
                    c.put32(owner + id_slot::word_90, 0);
                }
            }
        }
        break;
    }
    case word_94_first: case word_94_last:
        if (!clear_word(id_slot::word_94)) return no();
        break;
    case list_160_kind:
        if (!clear_in_list(id_slot::list_160, id_slot::list_160_count)) return no();
        break;
    case list_174_kind:
        if (!clear_in_list(id_slot::list_174, id_slot::list_174_count)) return no();
        break;
    case list_190_kind:
        if (!clear_in_list(id_slot::list_190, id_slot::list_190_count)) return no();
        break;
    case list_98_kind:
        if (!clear_in_list(id_slot::list_98, id_slot::list_98_count)) return no();
        break;
    case word_90_list_98_kind:
        if (!clear_word(id_slot::word_90)) return no();
        c.set(3, 0);
        if (!clear_in_list(id_slot::list_98, id_slot::list_98_count)) return no();
        break;
    case word_94_list_98_kind:
        if (!clear_word(id_slot::word_94)) return no();
        c.set(3, 0);
        if (!clear_in_list(id_slot::list_98, id_slot::list_98_count)) return no();
        break;
    case list_160_list_98_kind: {
        const auto index = find_slot(c, club, id_slot::list_160, id_slot::list_160_count, person);
        if (std::int32_t(index) >= id_slot::list_160_count) return no();
        c.set(0, index << 2);
        c.set(5, 0);
        c.set(4, club + (index << 2));
        c.set(3, 0);
        c.put32(club + (index << 2) + id_slot::list_160, 0);
        if (!clear_in_list(id_slot::list_98, id_slot::list_98_count)) return no();
        break;
    }
    default: return no();  // kind 14 and the rest
    }
    // The tail: with the second flag set, the person's club field is cleared (+0x20 not below the limit,
    // else +0x24).
    if (compare(c, guest::extended(std::int8_t(flag_a))) != 0) {
        compare_logical(c, club);
        const bool below_now = below_range_limit(c, club);
        c.set(0, 0);
        c.put32(person + (below_now ? 0x24 : 0x20), 0);
    }
    return answer(1);
}
}
