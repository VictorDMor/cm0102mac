// DATABASE.CPP's lookups block, part 2: the club link rows, the club nation rules
// and the sort comparators (database_lookups.hpp).
#include "database_lookups.hpp"
#include "database_lookups_detail.hpp"

namespace cm::aot::database {
namespace {
using namespace lookups;
using guest::compare;
using guest::compare_logical;
using guest::result;

}

// 0x100ddf30: sets both links' index and key (+0x3c/+0x4c, +0x50/+0x60) of a club's
// row to -1. Nothing happens for a null club or an index not below the row count.
void clear_club_links(Cpu& c, std::uint32_t club) {
    compare_logical(c, club);
    const auto rows = slot(c, 8, link_rows::rows_slot);
    if (club == 0) return;
    compare_logical(c, club);
    const auto count_at = slot(c, 4, link_rows::count_slot);
    const auto index = load32(c, 5, club + club_record::id);
    const auto count = load32(c, 0, count_at);
    if (compare(c, std::int32_t(index), std::int32_t(count)) >= 0) return;
    c.set(9, 0);
    while (compare(c, std::int32_t(c.r(9)), 2) < 0) {
        const auto link = c.r(9) * link_rows::link_stride;
        const auto first = load32(c, 4, club + club_record::id);
        c.set(0, link);
        const auto base = load32(c, 5, rows);
        c.set({{7, 0xffffffff}, {6, 0xffffffff}});
        c.set(4, first * link_rows::row_size);
        c.set(4, base + c.r(4));
        c.set(4, c.r(4) + link);
        c.put32(c.r(4) + link_rows::first_index, 0xffffffff);
        c.set(0, link);
        c.set(9, c.r(9) + 1);
        const auto second = load32(c, 4, club + club_record::id);
        const auto base_again = load32(c, 5, rows);
        c.set(4, second * link_rows::row_size);
        c.set(4, base_again + c.r(4));
        c.set(4, c.r(4) + link);
        c.put32(c.r(4) + link_rows::first_key, 0xffffffff);
    }
}

// 0x100ddfc0: records `other`'s first word and `key` as the club's first free link
// (the first when its index is -1, else the second, replaced whatever it held).
void add_club_link(Cpu& c, std::uint32_t club, std::uint32_t other, std::uint32_t key) {
    compare_logical(c, club);
    const auto rows = slot(c, 6, link_rows::rows_slot);
    const auto count_at = slot(c, 8, link_rows::count_slot);
    if (club == 0) return;
    compare_logical(c, club);
    auto index = load32(c, 7, club + club_record::id);
    auto count = load32(c, 0, count_at);
    if (compare(c, std::int32_t(index), std::int32_t(count)) >= 0) return;
    if (compare_logical(c, other) == 0) return;
    compare_logical(c, other);
    index = load32(c, 7, other + club_record::id);
    count = load32(c, 0, count_at);
    if (compare(c, std::int32_t(index), std::int32_t(count)) >= 0) return;
    auto first = load32(c, 0, club + club_record::id);
    auto base = load32(c, 7, rows);
    c.set(0, first * link_rows::row_size);
    c.set(7, base + c.r(0));
    const auto held = load32(c, 0, c.r(7) + link_rows::first_index);
    const bool free_first = compare(c, std::int32_t(held), -1) == 0;
    first = load32(c, 0, club + club_record::id);
    base = load32(c, 7, rows);
    c.set(0, first * link_rows::row_size);
    const auto other_id = load32(c, 8, other + club_record::id);
    c.set(4, base + c.r(0));
    c.put32(c.r(4) + (free_first ? link_rows::first_index : link_rows::second_index), other_id);
    first = load32(c, 0, club + club_record::id);
    const auto base_again = load32(c, 3, rows);
    c.set(0, first * link_rows::row_size);
    c.set(3, base_again + c.r(0));
    c.put32(c.r(3) + (free_first ? link_rows::first_key : link_rows::second_key), key);
}

// 0x100de090: clears the link whose key is `key` (the first, else the second).
void remove_club_link(Cpu& c, std::uint32_t club, std::uint32_t key) {
    compare_logical(c, club);
    const auto rows = slot(c, 5, link_rows::rows_slot);
    if (club == 0) return;
    compare_logical(c, club);
    const auto count_at = slot(c, 6, link_rows::count_slot);
    auto index = load32(c, 7, club + club_record::id);
    auto count = load32(c, 0, count_at);
    if (compare(c, std::int32_t(index), std::int32_t(count)) >= 0) return;
    index = load32(c, 0, club + club_record::id);
    auto base = load32(c, 6, rows);
    c.set(0, index * link_rows::row_size);
    c.set(6, base + c.r(0));
    auto held = load32(c, 0, c.r(6) + link_rows::first_key);
    bool second = false;
    if (compare(c, std::int32_t(key), std::int32_t(held)) != 0) {
        second = true;
        index = load32(c, 0, club + club_record::id);
        base = load32(c, 6, rows);
        c.set(0, index * link_rows::row_size);
        c.set(6, base + c.r(0));
        held = load32(c, 0, c.r(6) + link_rows::second_key);
        if (compare(c, std::int32_t(key), std::int32_t(held)) != 0) return;
    }
    index = load32(c, 0, club + club_record::id);
    c.set(7, 0xffffffff);
    base = load32(c, 6, rows);
    c.set(4, 0xffffffff);
    c.set(0, index * link_rows::row_size);
    c.set(6, base + c.r(0));
    c.put32(c.r(6) + (second ? link_rows::second_index : link_rows::first_index), 0xffffffff);
    index = load32(c, 0, club + club_record::id);
    const auto last_base = load32(c, 3, rows);
    c.set(0, index * link_rows::row_size);
    c.set(3, last_base + c.r(0));
    c.put32(c.r(3) + (second ? link_rows::second_key : link_rows::first_key), 0xffffffff);
}

// 0x100de160: the 0x70-byte record a club's link with `key` points at, or 0. The
// second link's test reads the first link's index (+0x3c), not its own: kept.
std::uint32_t find_club_link_record(Cpu& c, std::uint32_t club, std::uint32_t key) {
    compare_logical(c, club);
    const auto rows = slot(c, 6, link_rows::rows_slot);
    const auto records = slot(c, 5, link_rows::records_slot);
    c.set(0, 0);
    if (club == 0) return result(c, 0);
    compare_logical(c, club);
    const auto count_at = slot(c, 7, link_rows::count_slot);
    auto index = load32(c, 8, club + club_record::id);
    const auto count = load32(c, 7, count_at);
    if (compare(c, std::int32_t(index), std::int32_t(count)) >= 0) return result(c, 0);
    // rows[club.first_word] + offset: the original reloads both words each time.
    const auto row = [&](unsigned index_register, unsigned base_register, std::uint32_t offset) {
        const auto first = load32(c, index_register, club + club_record::id);
        const auto base = load32(c, base_register, rows);
        c.set(index_register, first * link_rows::row_size);
        c.set(index_register, base + c.r(index_register));
        return load32(c, index_register, c.r(index_register) + offset);
    };
    const auto link_record = [&](std::uint32_t index_offset) {
        const auto first = load32(c, 0, club + club_record::id);
        const auto base = load32(c, 3, rows);
        c.set(0, first * link_rows::row_size);
        const auto table = load32(c, 4, records);
        c.set(3, base + c.r(0));
        const auto record_index = load32(c, 0, c.r(3) + index_offset);
        c.set(0, record_index * link_rows::record_size);
        c.set(0, table + c.r(0));
    };
    if (compare(c, std::int32_t(key), std::int32_t(row(7, 8, link_rows::first_key))) == 0 &&
        compare(c, std::int32_t(row(7, 8, link_rows::first_index)), -1) != 0) {
        link_record(link_rows::first_index);
        return result(c, c.r(0));
    }
    if (compare(c, std::int32_t(key), std::int32_t(row(7, 8, link_rows::second_key))) == 0) {
        // Quirk: the index tested is the first link's, the one used the second's.
        if (compare(c, std::int32_t(row(4, 7, link_rows::first_index)), -1) != 0) {
            link_record(link_rows::second_index);
            return result(c, c.r(0));
        }
    }
    return result(c, c.r(0));
}
}

namespace cm::aot::database {
namespace {
using namespace lookups;
using guest::compare;
using guest::compare_logical;
using guest::result;

// The "assert" of a null club at the top of the two nation rules: 0x107755c0 gets the
// module's file pool at +0x2118 and the line. It returns, and the rule goes on
// to read the null club's fields.
void assert_club(Cpu& c, std::uint32_t entry, std::uint32_t pool, std::uint32_t line) {
    c.set(3, pool + 0x2118);
    c.set(5, line);
    c.call(0x107755c0, entry + 0x28);
}
}

// 0x100d33d0: a frame of 0x40 saving r31; r31 the club. True when the flag byte at
// TOC -0x7a68 is 1, or the club's nation is the word at -0x7b68; false without a
// nation record (+0x74) or for the four nations at -0x75a8, -0x75b0, -0x7590, -0x758c;
// otherwise whether the nation record's word (+8) is the one at -0x7a70.
bool club_nation_rule_a(Cpu& c, std::uint32_t club) {
    constexpr std::uint32_t entry = 0x100d33d0;
    constexpr std::array<std::int32_t, 4> excluded{-0x75a8, -0x75b0, -0x7590, -0x758c};
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x40, {.first_gpr = 31});
    guest::Kept<31> held(c, club);
    compare(c, std::int32_t(club));  // or. r31,r3,r3
    const auto pool = slot(c, 4, -0x5618);
    if (club == 0) assert_club(c, entry, pool, 0x4ad4);
    const auto flag_at = slot(c, 3, -0x7a68);
    const auto flag = load8(c, 0, flag_at);
    if (compare(c, std::int32_t(flag), 1) == 0) return result(c, true);
    {
        const auto at = slot(c, 3, -0x7b68);
        const auto nation = load32(c, 4, held + club_record::nation);
        const auto word = load32(c, 0, at);
        if (compare(c, std::int32_t(nation), std::int32_t(word)) == 0) return result(c, true);
    }
    const auto nation_record = load32(c, 0, held + club_record::nation_record);
    if (compare_logical(c, nation_record) == 0) return result(c, false);
    for (const auto offset : excluded) {
        const auto at = slot(c, 3, offset);
        const auto nation = load32(c, 4, held + club_record::nation);
        const auto word = load32(c, 0, at);
        if (compare(c, std::int32_t(nation), std::int32_t(word)) == 0) return result(c, false);
    }
    const auto record = load32(c, 4, held + club_record::nation_record);
    const auto at = slot(c, 3, -0x7a70);
    const auto record_word = load32(c, 4, record + nation_word);
    const auto word = load32(c, 0, at);
    return result(c, compare(c, std::int32_t(record_word), std::int32_t(word)) == 0);
}

// 0x100d34c0: true when the club's nation is one of the words at -0x7a10, -0x7a1c,
// -0x7574, -0x757c, -0x7578.
bool club_nation_rule_b(Cpu& c, std::uint32_t club) {
    constexpr std::uint32_t entry = 0x100d34c0;
    constexpr std::array<std::int32_t, 5> nations{-0x7a10, -0x7a1c, -0x7574, -0x757c, -0x7578};
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x40, {.first_gpr = 31});
    guest::Kept<31> held(c, club);
    compare(c, std::int32_t(club));  // or. r31,r3,r3
    const auto pool = slot(c, 4, -0x5618);
    if (club == 0) assert_club(c, entry, pool, 0x4af2);
    for (const auto offset : nations) {
        const auto at = slot(c, 3, offset);
        const auto nation = load32(c, 4, held + club_record::nation);
        const auto word = load32(c, 0, at);
        if (compare(c, std::int32_t(nation), std::int32_t(word)) == 0) return result(c, true);
    }
    return result(c, false);
}

// 0x100d3040: a frame of 0x50 saving r28-r31 (r28 the person, r29 the club, r30 the
// "last primary contract" word, r31 the contract manager's word). With no club, the
// club of the person's primary contract (published in the shared word, as
// contract_flag_09 does); the person's contract with that club must have bit 0 of
// its flags; then true when the club query 0x1014c2e0(club, 1, 1, 0, 0) answers
// nonzero (as a signed byte) or the person's byte +0x62 is not 0xf, else false.
bool person_club_status_rule(Cpu& c, std::uint32_t person, std::uint32_t the_club) {
    constexpr std::uint32_t entry = 0x100d3040;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 28});
    guest::Kept<31> manager(c, c.toc(manager_slot));
    guest::Kept<30> last_primary(c, c.toc(last_primary_slot));
    guest::Kept<29> club(c, the_club);
    guest::Kept<28> holder_address(c, person);
    compare(c, std::int32_t(person));
    if (person == 0) return result(c, false);
    compare_logical(c, club);
    if (club == 0) {
        c.set(3, c.u32(manager));
        c.set(4, holder_address);
        c.call_in_place(0x10082f20, entry + 0x4c);
        const auto primary = c.r(3);
        compare_logical(c, primary);
        c.put32(last_primary, primary);
        std::uint32_t club_address = 0;
        if (primary != 0) {
            const auto stored = load32(c, 3, last_primary);
            const auto table = slot(c, 4, club_table::base_slot);
            const auto index = load32(c, 0, stored + database::contract::club_index);
            const auto rows = load32(c, 3, table);
            club_address = rows + index * club_table::stride;
        }
        c.set(0, club_address);
        club = club_address;
        if (compare(c, std::int32_t(club_address)) == 0) return result(c, false);
    }
    c.set(3, c.u32(manager));
    c.set(4, holder_address);
    c.set(5, club);
    c.call_in_place(0x10082f90, entry + 0x98);
    const auto contract = c.r(3);
    if (compare_logical(c, contract) == 0) return result(c, false);
    const auto flags = load8(c, 0, contract + database::contract::flags);
    c.set(0, flags & 1);
    if (compare(c, std::int32_t(flags & 1)) == 0) return result(c, false);
    c.set({{3, club}, {4, 1}, {5, 1}, {6, 0}, {7, 0}});
    c.call_in_place(0x1014c2e0, entry + 0xc4);
    const auto answer = sext8(c.r(3));
    c.set(0, answer);
    if (compare(c, std::int32_t(answer)) != 0) return result(c, true);
    const auto role = load8(c, 0, holder_address + holder::role);
    if (compare(c, std::int32_t(role), 0xf) != 0) return result(c, true);
    return result(c, false);
}

// The sort comparators: qsort4x passes pointers to the array elements.
namespace {
// The tail 0x100cd330, 0x100cd390 share: a is less than b -> 1, otherwise
// a - b's subfc/subfe sign sequence (CA from the unsigned compare of the extended
// halves).
std::int32_t compare_halves(Cpu& c, std::uint32_t first, std::uint32_t second) {
    const auto a = sext16(first), b = sext16(second);
    c.set(3, a);
    c.set(0, b);
    if (compare(c, std::int32_t(a), std::int32_t(b)) < 0) return std::int32_t(result(c, 1));
    c.set(3, a);
    c.set(4, b);
    const auto difference = b - a;
    const auto carry = (std::uint64_t(b) + std::uint64_t(~a) + 1) >> 32;
    c.set(0, difference);
    const auto sign_a = a >> 31, sign_b = b >> 31;
    c.set(3, sign_a);
    c.set(0, sign_b);
    const auto total = std::uint64_t(~sign_b) + std::uint64_t(sign_a) + carry;
    c.s.wr<1>(guest::reg::xer_ca, std::uint8_t(total >> 32));
    return std::int32_t(result(c, std::uint32_t(total)));
}
}

std::int32_t compare_club_pointers_by_rating(Cpu& c, std::uint32_t a, std::uint32_t b) {
    const auto first = load32(c, 3, a);
    c.set(5, 0);
    const auto second = load32(c, 4, b);
    std::uint32_t first_rating = 0, second_rating = 0;
    compare_logical(c, first);
    if (first != 0) first_rating = std::uint32_t(load16s(c, 5, first + club_record::rating));
    compare_logical(c, second);
    c.set(6, 0);
    if (second != 0) second_rating = std::uint32_t(load16s(c, 6, second + club_record::rating));
    return compare_halves(c, first_rating, second_rating);
}

std::int32_t compare_club_indexes_by_rating(Cpu& c, std::uint32_t a, std::uint32_t b) {
    const auto first = load32(c, 0, a);
    c.set(6, 0);
    const auto table_at = slot(c, 5, club_table::base_slot);
    compare(c, std::int32_t(first), -1);
    const auto second = load32(c, 4, b);
    std::uint32_t first_rating = 0, second_rating = 0;
    if (std::int32_t(first) > -1) {
        c.set(0, first * club_table::stride);
        const auto rows = load32(c, 3, table_at);
        c.set(3, rows + c.r(0));
        first_rating = std::uint32_t(load16s(c, 6, c.r(3) + club_record::rating));
    }
    compare(c, std::int32_t(second), -1);
    c.set(7, 0);
    if (std::int32_t(second) > -1) {
        c.set(0, second * club_table::stride);
        const auto rows = load32(c, 3, table_at);
        c.set(3, rows + c.r(0));
        second_rating = std::uint32_t(load16s(c, 7, c.r(3) + club_record::rating));
    }
    return compare_halves(c, first_rating, second_rating);
}

// 0x100cd740: null elements sort by presence, then the difference of the players'
// 16-bit values at +0xa (b - a).
std::int32_t compare_holders_by_player_rating(Cpu& c, std::uint32_t a, std::uint32_t b) {
    const auto first = load32(c, 3, a);
    c.set(5, 0);
    const auto second = load32(c, 4, b);
    c.set(6, 0);
    compare_logical(c, first);
    if (first == 0) {
        if (compare_logical(c, second) == 0) return std::int32_t(result(c, 0));
    }
    if (compare_logical(c, first) == 0) return std::int32_t(result(c, 1));
    if (compare_logical(c, second) == 0) return std::int32_t(result(c, 0xffffffff));
    std::uint32_t first_value = 0, second_value = 0;
    const auto first_player = load32(c, 0, first + database::person::player);
    if (compare_logical(c, first_player) != 0) {
        c.set(3, load32(c, 3, first + database::person::player));
        first_value = std::uint32_t(load16s(c, 5, c.r(3) + 0xa));
    }
    const auto second_player = load32(c, 0, second + database::person::player);
    if (compare_logical(c, second_player) != 0) {
        c.set(3, load32(c, 3, second + database::person::player));
        second_value = std::uint32_t(load16s(c, 6, c.r(3) + 0xa));
    }
    c.set(3, sext16(first_value));
    c.set(0, sext16(second_value));
    return std::int32_t(result(c, sext16(second_value) - sext16(first_value)));
}
}

namespace cm::aot::database {
using namespace lookups;
using guest::compare;
using guest::compare_logical;
using guest::result;

// 0x100cde70: pointers to records whose +4 is a pointer to a record with a leading
// word. A null +4 sorts first (1 when only b's is set, 0 when neither), then the
// difference of the leading words, then the 16-bit values at +0x18 (b - a) unless one
// is -1, where the leading words' difference stays.
std::int32_t compare_by_key_then_value(Cpu& c, std::uint32_t a, std::uint32_t b) {
    const auto first_key = load32(c, 0, a + 4);
    if (compare_logical(c, first_key) == 0) {
        const auto second_key = load32(c, 3, b + 4);
        const auto negated = 0 - second_key;
        c.set(0, negated);
        c.set(0, negated | second_key);
        return std::int32_t(result(c, (negated | second_key) >> 31));
    }
    const auto second_key = load32(c, 0, b + 4);
    if (compare_logical(c, second_key) == 0) return std::int32_t(result(c, 0xffffffff));
    const auto second = load32(c, 6, b + 4);
    const auto first = load32(c, 5, a + 4);
    const auto second_word = load32(c, 6, second);
    const auto first_word = load32(c, 0, first);
    const auto difference = first_word - second_word;
    c.set(5, difference);
    if (compare(c, std::int32_t(difference)) != 0) return std::int32_t(result(c, difference));
    const auto first_value = load16s(c, 0, a + 0x18);
    if (compare(c, first_value, -1) == 0) return std::int32_t(result(c, difference));
    const auto second_value = load16s(c, 0, b + 0x18);
    if (compare(c, second_value, -1) == 0) return std::int32_t(result(c, difference));
    const auto first_again = load16s(c, 3, a + 0x18);
    const auto second_again = load16s(c, 0, b + 0x18);
    return std::int32_t(result(c, std::uint32_t(second_again) - std::uint32_t(first_again)));
}

// 0x100ddea0: elements without a player record (+0x14) sort first (1 for a, -1 for
// b); then the players' 16-bit values at +4 (b - a), or at +0xa when those are equal.
std::int32_t compare_by_player_values(Cpu& c, std::uint32_t a, std::uint32_t b) {
    const auto first = load32(c, 5, a);
    const auto second = load32(c, 6, b);
    if (compare_logical(c, first) == 0) return std::int32_t(result(c, 1));
    const auto first_player = load32(c, 0, first + database::person::player);
    if (compare_logical(c, first_player) == 0) return std::int32_t(result(c, 1));
    if (compare_logical(c, second) == 0) return std::int32_t(result(c, 0xffffffff));
    const auto second_player = load32(c, 0, second + database::person::player);
    if (compare_logical(c, second_player) == 0) return std::int32_t(result(c, 0xffffffff));
    const auto player_b = load32(c, 4, second + database::person::player);
    const auto player_a = load32(c, 3, first + database::person::player);
    const auto value_b = load16s(c, 4, player_b + 4);
    const auto value_a = load16s(c, 0, player_a + 4);
    c.set(3, std::uint32_t(value_b));
    if (compare(c, value_b, value_a) == 0) {
        const auto first_again = load32(c, 4, first + database::person::player);
        const auto second_again = load32(c, 3, second + database::person::player);
        const auto low_a = load16s(c, 4, first_again + 0xa);
        const auto low_b = load16s(c, 0, second_again + 0xa);
        return std::int32_t(result(c, std::uint32_t(low_b) - std::uint32_t(low_a)));
    }
    const auto first_again = load32(c, 4, first + database::person::player);
    const auto second_again = load32(c, 3, second + database::person::player);
    const auto high_a = load16s(c, 4, first_again + 4);
    const auto high_b = load16s(c, 0, second_again + 4);
    return std::int32_t(result(c, std::uint32_t(high_b) - std::uint32_t(high_a)));
}
}
