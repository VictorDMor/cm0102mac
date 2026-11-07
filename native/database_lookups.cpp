// DATABASE.CPP's lookups block, part 1: value tests, nation-set tests, staff and
// club lists (database_lookups.hpp). Every routine leaves the registers and cr0 of
// the original, in the order of its loads and compares.
#include "database_lookups.hpp"
#include "database_lookups_detail.hpp"
#include "database_detail.hpp"
#include "database.hpp"

namespace cm::aot::database {
namespace {
using namespace lookups;
using guest::compare;
using guest::compare_logical;
using guest::result;

std::uint32_t answer(Cpu& c, bool value) { return result(c, value); }

// Nation sets: TOC slots whose words are compared with a nation word. The
// original loads each link as slot, value, word, then compares; the last link
// of a chain reuses the register that held the record.
constexpr std::array<std::int32_t, 6> set_6{-0x7a10, -0x7a00, -0x7af4, -0x7a14, -0x7a18, -0x7a28};
constexpr std::array<std::int32_t, 5> set_a{-0x7628, -0x762c, -0x7630, -0x7634, -0x7638};
constexpr std::array<std::int32_t, 5> set_b{-0x763c, -0x7640, -0x7644, -0x7648, -0x764c};
constexpr std::int32_t id_list_first = -0x75cc;  // 23 ids at consecutive slots downward
constexpr unsigned id_list_size = 23;
}

// 0x100c7a00: li r0,11; extsb r4,r3; subf r3,r4,r0; subf r0,r0,r4; or; rlwinm: (value != 11).
bool value_is_not_11(Cpu& c, std::uint32_t value) {
    const auto byte = sext8(value);
    c.set(4, byte);
    const auto below = 11 - byte, above = byte - 11;
    c.set(0, below | above);
    return answer(c, ((below | above) >> 31) != 0);
}

// 0x100c7a20: the byte minus 11 is 0 to 2 (unsigned byte), or the signed byte is 15.
bool value_is_11_to_13_or_15(Cpu& c, std::uint32_t value) {
    const auto distance = (value - 11) & 0xff;
    c.set(0, distance);
    if (compare_logical(c, distance, 2) <= 0) return answer(c, true);
    const auto byte = sext8(value);
    c.set(0, byte);
    return answer(c, compare(c, std::int32_t(byte), 15) == 0);
}

std::uint32_t read_flag_7ab0(Cpu& c) {
    const auto at = slot(c, 3, -0x7ab0);
    return load8(c, 3, at);
}

void set_pair_field(Cpu& c, std::uint32_t record, std::uint32_t value) {
    c.set(7, 3);
    c.set(5, sext8(value));
    c.set(0, 3);
    c.set(6, 0xc);
    const auto low = sext8(value) & 3;
    c.set(0, low);
    compare(c, std::int32_t(low));
    // Low bits in the value: keep bits 2-3 (0xc); otherwise keep bits 0-1 (0x3).
    // Bits 4-7 of the old byte are cleared either way.
    const std::uint32_t keep = low != 0 ? 0xc : 0x3;
    const auto old = load8(c, 5, record + holder::pair_field);
    c.set(0, keep);
    c.set(0, old & keep);
    c.put8(record + holder::pair_field, old & keep);
    const auto kept = load8(c, 0, record + holder::pair_field);
    c.set(0, kept | value);
    c.put8(record + holder::pair_field, kept | value);
}

bool linked_status_is_2(Cpu& c, std::uint32_t person) {
    const auto linked = load32(c, 0, person + holder::linked);
    if (compare_logical(c, linked) == 0) return answer(c, false);
    c.set(3, linked);
    const auto status = load8(c, 0, linked + club_record::status);
    return answer(c, compare(c, std::int32_t(status), 2) == 0);
}

namespace {
// 0x100dbb20 and 0x100dbbe0: the linked record's nation word differs from the
// word at TOC -0x7508 (the original's subf/or/shift sequence).
bool linked_nation_differs(Cpu& c, std::uint32_t person) {
    if (compare_logical(c, person) == 0) return answer(c, false);
    const auto linked = load32(c, 0, person + holder::linked);
    if (compare_logical(c, linked) == 0) return answer(c, false);
    c.set(4, linked);
    const auto word_address = slot(c, 3, -0x7508);
    const auto nation = load32(c, 4, linked + club_record::nation);
    const auto word = load32(c, 0, word_address);
    const auto word_minus_nation = word - nation, nation_minus_word = nation - word;
    c.set(3, word_minus_nation);
    c.set(0, nation_minus_word);
    c.set(0, word_minus_nation | nation_minus_word);
    return answer(c, ((word_minus_nation | nation_minus_word) >> 31) != 0);
}
}
bool linked_nation_differs_7508(Cpu& c, std::uint32_t p) { return linked_nation_differs(c, p); }
bool linked_nation_differs_7508_again(Cpu& c, std::uint32_t p) { return linked_nation_differs(c, p); }

namespace {
enum class Probe { absent, hit, miss };
// The linked record's nation against TOC words; the original reloads the linked
// pointer into r5 for each link (r4 for the last, whose slot pointer goes to r3).
template <std::size_t N>
Probe linked_nation_probe(Cpu& c, std::uint32_t person, const std::array<std::int32_t, N>& words) {
    if (compare_logical(c, person) == 0) return Probe::absent;
    const auto linked = load32(c, 0, person + holder::linked);
    if (compare_logical(c, linked) == 0) return Probe::absent;
    for (std::size_t i = 0; i < N; ++i) {
        const bool last = i + 1 == N;
        const auto record = load32(c, last ? 4 : 5, person + holder::linked);
        const auto at = slot(c, last ? 3 : 4, words[i]);
        const auto nation = load32(c, last ? 4 : 5, record + club_record::nation);
        const auto word = load32(c, 0, at);
        if (compare(c, std::int32_t(nation), std::int32_t(word)) == 0) return Probe::hit;
    }
    return Probe::miss;
}
constexpr std::array<std::int32_t, 3> set_3{-0x7508, -0x7a00, -0x7590};
}

// 0x100de260: the linked record's nation is in set_6.
bool linked_nation_in_set_6(Cpu& c, std::uint32_t person) {
    return answer(c, linked_nation_probe(c, person, set_6) == Probe::hit);
}
// 0x100dbb60: the linked record's nation is in none of -0x7508, -0x7a00, -0x7590
// (false without a linked record).
bool linked_nation_outside_set_3(Cpu& c, std::uint32_t person) {
    return answer(c, linked_nation_probe(c, person, set_3) == Probe::miss);
}

bool club_nation_in_set_6(Cpu& c, std::uint32_t club) {
    if (compare_logical(c, club) == 0) return answer(c, false);
    for (unsigned i = 0; i < set_6.size(); ++i) {
        const bool last = i + 1 == set_6.size();
        const auto at = slot(c, 4, set_6[i]);
        const auto nation = load32(c, last ? 3 : 5, club + club_record::nation);
        const auto word = load32(c, 0, at);
        if (compare(c, std::int32_t(nation), std::int32_t(word)) == 0) return answer(c, true);
    }
    return answer(c, false);
}

// 0x100ca800: the linked record's nation is the word at TOC -0x7a18, and the
// holder's first word equals one of 23 ids; r5 carries the first word, r3 the last.
bool listed_id_with_linked_nation_7a18(Cpu& c, std::uint32_t person) {
    if (compare_logical(c, person) == 0) return answer(c, false);
    const auto linked = load32(c, 0, person + holder::linked);
    if (compare_logical(c, linked) == 0) return answer(c, false);
    load32(c, 5, person + holder::linked);
    const auto at = slot(c, 4, -0x7a18);
    const auto nation = load32(c, 5, linked + club_record::nation);
    const auto word = load32(c, 0, at);
    if (compare(c, std::int32_t(nation), std::int32_t(word)) != 0) return answer(c, false);
    for (unsigned i = 0; i < id_list_size; ++i) {
        const bool last = i + 1 == id_list_size;
        const auto list_at = slot(c, 4, id_list_first - 4 * std::int32_t(i));
        const auto id = load32(c, last ? 3 : 5, person + holder::id);
        const auto listed = load32(c, 0, list_at);
        if (compare(c, std::int32_t(id), std::int32_t(listed)) == 0) return answer(c, true);
    }
    return answer(c, false);
}

namespace {
// 0x100caa10, 0x100caa90: the club's nation word against five TOC words.
template <std::size_t N>
bool club_nation_in(Cpu& c, std::uint32_t club, const std::array<std::int32_t, N>& words) {
    for (std::size_t i = 0; i < N; ++i) {
        const bool last = i + 1 == N;
        const auto at = slot(c, 4, words[i]);
        const auto nation = load32(c, last ? 3 : 5, club + club_record::nation);
        const auto word = load32(c, 0, at);
        if (compare(c, std::int32_t(nation), std::int32_t(word)) == 0) return answer(c, true);
    }
    return answer(c, false);
}
}
bool club_nation_in_set_a(Cpu& c, std::uint32_t club) { return club_nation_in(c, club, set_a); }
bool club_nation_in_set_b(Cpu& c, std::uint32_t club) { return club_nation_in(c, club, set_b); }

// 0x100d9440: a frame of 0x40 with LR; with a record, 0x101929b0(record, 0, 0).
std::uint32_t forward_if_set(Cpu& c, std::uint32_t record) {
    constexpr std::uint32_t entry = 0x100d9440;
    c.set(0, c.lr());
    compare_logical(c, record);
    guest::Frame frame(c, entry, 0x40, {});
    if (record == 0) return answer(c, false);
    c.set({{4, 0}, {5, 0}});
    c.call_in_place(0x101929b0, entry + 0x20);
    return c.r(3);
}

namespace {
// 0x100d9db0: three ids at +0x5c; the first equal to `id` is zeroed.
void remove_listed(Cpu& c, std::uint32_t record, std::uint32_t id, std::uint32_t first) {
    if (compare_logical(c, record) == 0) return;
    if (compare_logical(c, id) == 0) return;
    for (std::uint32_t i = 0; i < 3; ++i) {
        const auto listed = load32(c, 0, record + first + 4 * i);
        if (compare_logical(c, listed, id) == 0) {
            c.set(0, 0);
            c.put32(record + first + 4 * i, 0);
            return;
        }
    }
}
// 0x100d9e10, 0x100d9e90, 0x100da530: the same on a person's preferences (+0x18),
// reloaded into r5 before each slot; a hit zeroes the slot through r3 = preferences.
void remove_preferred(Cpu& c, std::uint32_t person, std::uint32_t id, std::uint32_t first) {
    if (compare_logical(c, person) == 0) return;
    const auto preferences = load32(c, 0, person + holder::preferences);
    if (compare_logical(c, preferences) == 0) return;
    if (compare_logical(c, id) == 0) return;
    for (std::uint32_t i = 0; i < 3; ++i) {
        const auto at = load32(c, 5, person + holder::preferences);
        const auto listed = load32(c, 0, at + first + 4 * i);
        if (compare_logical(c, listed, id) == 0) {
            c.set(3, load32(c, 3, person + holder::preferences));
            c.set(0, 0);
            c.put32(c.r(3) + first + 4 * i, 0);
            return;
        }
    }
}
// 0x100d9f10, 0x100d9fb0: how many of the three listed records have +0x24 equal to
// `key`; r6 counts, r5 (r3 for the last) walks the records.
std::uint32_t count_keyed(Cpu& c, std::uint32_t person, std::uint32_t key, std::uint32_t first) {
    std::uint32_t count = 0;
    compare_logical(c, person);
    c.set(6, 0);
    const auto done = [&] { return result(c, count); };
    if (person == 0) return done();
    const auto preferences = load32(c, 0, person + holder::preferences);
    if (compare_logical(c, preferences) == 0) return done();
    if (compare_logical(c, key) == 0) return done();
    for (std::uint32_t i = 0; i < 3; ++i) {
        const bool last = i == 2;
        const auto at = load32(c, 5, person + holder::preferences);
        const auto listed = load32(c, 0, at + first + 4 * i);
        if (compare_logical(c, listed) == 0) {
            if (last) return done();
            continue;
        }
        const auto prefs = load32(c, last ? 3 : 5, person + holder::preferences);
        const auto record = load32(c, last ? 3 : 5, prefs + first + 4 * i);
        const auto value = load32(c, 0, record + preferences_record::staff_key);
        if (compare_logical(c, value, key) == 0) c.set(6, ++count);
    }
    return done();
}
// 0x100dafa0, 0x100db060: the non-null ids of three slots appended to `out`, the
// count byte at `counter` first cleared and then incremented before each store.
void collect(Cpu& c, std::uint32_t person, std::uint32_t out, std::uint32_t counter, std::uint32_t first) {
    c.set(0, 0);
    compare_logical(c, person);
    c.put8(counter, 0);
    if (person == 0) return;
    const auto preferences = load32(c, 0, person + holder::preferences);
    if (compare_logical(c, preferences) == 0) return;
    if (compare_logical(c, out) == 0) return;
    for (std::uint32_t i = 0; i < 3; ++i) {
        const bool last = i == 2;
        const auto at = load32(c, 6, person + holder::preferences);
        const auto listed = load32(c, 0, at + first + 4 * i);
        if (compare_logical(c, listed) == 0) {
            if (last) return;
            continue;
        }
        // The last slot walks r3 instead of r6/r7/r8: r3 the preferences, then count + 1.
        const auto prefs = load32(c, last ? 3 : 6, person + holder::preferences);
        const auto count = load8(c, last ? 6 : 7, counter);
        const auto id = load32(c, last ? 7 : 8, prefs + first + 4 * i);
        c.set(last ? 3 : 6, count + 1);
        const auto offset = sext8(count) << 2;
        c.set(0, offset);
        c.put8(counter, count + 1);
        c.put32(out + offset, id);
    }
}
}

void club_remove_favoured_staff(Cpu& c, std::uint32_t club, std::uint32_t id) {
    remove_listed(c, club, id, database::club::favoured_staff);
}
void person_remove_favourite_staff(Cpu& c, std::uint32_t p, std::uint32_t id) {
    remove_preferred(c, p, id, preferences_record::favourite_staff);
}
void person_remove_disliked_staff(Cpu& c, std::uint32_t p, std::uint32_t id) {
    remove_preferred(c, p, id, preferences_record::disliked_staff);
}
void person_remove_favourite_club(Cpu& c, std::uint32_t p, std::uint32_t id) {
    remove_preferred(c, p, id, preferences_record::favourite_clubs);
}
std::uint32_t count_disliked_staff_keyed(Cpu& c, std::uint32_t p, std::uint32_t key) {
    return count_keyed(c, p, key, preferences_record::disliked_staff);
}
std::uint32_t count_favourite_staff_keyed(Cpu& c, std::uint32_t p, std::uint32_t key) {
    return count_keyed(c, p, key, preferences_record::favourite_staff);
}
void collect_favourite_staff(Cpu& c, std::uint32_t p, std::uint32_t out, std::uint32_t n) {
    collect(c, p, out, n, preferences_record::favourite_staff);
}
void collect_disliked_staff(Cpu& c, std::uint32_t p, std::uint32_t out, std::uint32_t n) {
    collect(c, p, out, n, preferences_record::disliked_staff);
}

// 0x100db120: a frame of 0x50 saving r28-r31. Counts the non-null holders among the
// 50 pointers at +0x98 that have a player record and whose position mask is `mask`.
std::uint32_t count_with_position_mask(Cpu& c, std::uint32_t squad, std::uint32_t mask) {
    constexpr std::uint32_t entry = 0x100db120;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 28});
    guest::Kept<31> count(c, 0);
    guest::Kept<30> index(c, 0);
    guest::Kept<29> wanted(c, mask);
    guest::Kept<28> holders(c, squad);
    for (; compare(c, std::int32_t(index), 0x32) < 0; index = index + 1) {
        c.set(0, index << 2);
        c.set(3, holders + (index << 2));
        const auto member = load32(c, 3, c.r(3) + holder::squad);
        if (compare_logical(c, member) == 0) continue;
        const auto player = load32(c, 0, member + database::person::player);
        if (compare_logical(c, player) == 0) continue;
        guest::enter(c, entry + 0x54, {member});
        const auto found = position_mask(c, member);
        if (compare(c, std::int32_t(wanted), std::int32_t(found)) == 0) count = count + 1;
    }
    return result(c, count);
}
}
