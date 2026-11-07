// DATABASE.CPP's generation block, part 2: adding a person or a club to a three-slot list
// (database_generation.hpp). The six routines share one algorithm over two lists: nothing
// happens when the id is already in the first (the "kept" list); an id found in the second
// (the "opposite" list) is removed from it and nothing is added; otherwise it takes the first
// empty slot of the kept list; and with all three slots taken, a non-zero flag replaces one
// slot, picked by the pool's sample (generation::pool_choice) over a cursor that wraps with a
// random draw. The routines differ in the record that holds the lists, the registers the
// compiler used, and the null checks.
#include "database_generation.hpp"
#include "database_generation_detail.hpp"
#include <array>

namespace cm::aot::database {
namespace {
using namespace generation;

struct Lists {
    std::uint32_t kept, opposite;  // offsets of the three slots, from the record that holds them
};
constexpr Lists club_favoured{0x5c, 0x68}, club_disliked{0x68, 0x5c};
constexpr Lists staff_favoured{0x1c, 0x28}, staff_disliked{0x28, 0x1c};
constexpr Lists clubs_favoured{0x4, 0x10}, clubs_disliked{0x10, 0x4};

std::uint32_t slot_at(std::uint32_t list, unsigned n) { return list + n * slot_stride; }

// 0x100d95e0 and 0x100d97a0: a club's lists, a person's id. r28 the club, r29 the id, r30 the
// salt's address, r31 the cursor's, r6 the end's, r7 the start (kept in registers, not reloaded).
void club_list(Cpu& c, std::uint32_t entry, const Lists& lists, std::uint32_t club, std::uint32_t id,
               std::uint32_t flag) {
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 28});
    const auto p = load_pool(c);
    c.set(7, p.start);
    c.set(30, p.salt_at);
    c.set(29, id);
    c.set(6, p.end_at);
    compare(c, std::int32_t(club));
    c.set(28, club);
    c.set(31, p.cursor_at);
    if (club == 0) return;
    if (compare_logical(c, id) == 0) return;
    compare_logical(c, id);
    c.set(3, 0);
    bool held = false;
    if (id != 0) {
        compare_logical(c, club);
        if (club != 0)
            for (unsigned n = 0; n < slot_count && !held; ++n) {
                c.set(0, c.u32(club + slot_at(lists.kept, n)));
                held = compare_logical(c, c.r(0), id) == 0;
            }
    }
    if (held) c.set(3, 1);
    if (compare(c, std::int32_t(c.r(3))) != 0) return;
    for (unsigned n = 0; n < slot_count; ++n) {
        c.set(0, c.u32(club + slot_at(lists.opposite, n)));
        if (compare_logical(c, c.r(0), id) != 0) continue;
        c.set(0, 0);
        c.put32(club + slot_at(lists.opposite, n), 0);
        return;
    }
    for (unsigned n = 0; n < slot_count; ++n) {
        c.set(0, c.u32(club + slot_at(lists.kept, n)));
        if (compare_logical(c, c.r(0), 0) != 0) continue;
        c.put32(club + slot_at(lists.kept, n), id);
        return;
    }
    c.set(0, extsb(flag));
    if (compare(c, std::int32_t(c.r(0))) == 0) return;
    advance_pool(c, {3, 0}, p.cursor_at, p.end_at, p.salt_at, p.start, entry + 0x130);
    const auto choice = pool_choice(c, p.cursor_at, p.salt_at);
    if (choice >= 0) c.put32(club + slot_at(lists.kept, unsigned(choice)), id);
}

// The membership test of the preference-record routines: the owner's preferences record is
// reloaded for every slot, as the original does, into `r`.
bool owner_has(Cpu& c, unsigned r, std::uint32_t owner, std::uint32_t list, std::uint32_t id) {
    for (unsigned n = 0; n < slot_count; ++n) {
        const auto preferences = load32(c, r, owner + person_preferences);
        c.set(r, c.u32(preferences + slot_at(list, n)));
        if (compare_logical(c, c.r(r), id) == 0) return true;
    }
    return false;
}

// What the owner-record routines do after the null checks, with `r` the scratch register
// (r4 for 0x100d9960 and 0x100d9b80, r3 for 0x100da110 and 0x100da320) and `flag_register`
// the register `extsb.` loads. The first membership test (the "held" test) is made by the caller.
void owner_update(Cpu& c, unsigned r, unsigned flag_register, const Lists& lists,
                  std::uint32_t owner, std::uint32_t id, std::uint32_t flag, const Pool& p,
                  std::uint32_t back) {
    for (unsigned n = 0; n < slot_count; ++n) {
        const auto preferences = load32(c, r, owner + person_preferences);
        c.set(r, c.u32(preferences + slot_at(lists.opposite, n)));
        if (compare_logical(c, c.r(r), id) != 0) continue;
        c.set(3, c.u32(owner + person_preferences));
        c.set(0, 0);
        c.put32(c.r(3) + slot_at(lists.opposite, n), 0);
        return;
    }
    for (unsigned n = 0; n < slot_count; ++n) {
        const auto preferences = load32(c, r, owner + person_preferences);
        c.set(r, c.u32(preferences + slot_at(lists.kept, n)));
        if (compare_logical(c, c.r(r), 0) != 0) continue;
        c.set(3, c.u32(owner + person_preferences));
        c.put32(c.r(3) + slot_at(lists.kept, n), id);
        return;
    }
    c.set(flag_register, extsb(flag));
    if (compare(c, std::int32_t(c.r(flag_register))) == 0) return;
    advance_pool(c, {4, 3}, p.cursor_at, p.end_at, p.salt_at, c.r(0), back);
    const auto choice = pool_choice(c, p.cursor_at, p.salt_at);
    if (choice < 0) return;
    c.set(3, c.u32(owner + person_preferences));
    c.put32(c.r(3) + slot_at(lists.kept, unsigned(choice)), id);
}

// 0x100d9960 (and 0x100d9b80 with `self_check`): a person's own preferences list gets another
// person. r28 the owner, r29 the id, r30/r31 the salt's and cursor's addresses, r3 the end's
// address, r0 the start, r6 the "already held" flag, r4 the scratch.
void staff_list(Cpu& c, std::uint32_t entry, const Lists& lists, std::uint32_t owner, std::uint32_t id,
                std::uint32_t flag, bool self_check) {
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 28});
    const auto p = load_pool(c);
    c.set(31, p.cursor_at);
    c.set(30, p.salt_at);
    c.set(29, id);
    c.set(28, owner);
    if (self_check) compare_logical(c, owner, id);
    else compare(c, std::int32_t(owner));
    c.set(3, p.end_at);
    c.set(0, p.start);
    if (self_check ? owner == id : owner == 0) return;
    if (self_check) {
        if (compare_logical(c, owner) == 0) return;
    }
    const auto preferences = load32(c, 4, owner + person_preferences);
    if (compare_logical(c, preferences) == 0) return;
    if (compare_logical(c, id) == 0) return;
    compare_logical(c, id);
    c.set(6, 0);
    compare_logical(c, owner);
    if (owner != 0 && preferences != 0) {
        const auto again = load32(c, 4, owner + person_preferences);
        compare_logical(c, again);
        if (again != 0 && owner_has(c, 4, owner, lists.kept, id)) c.set(6, 1);
    }
    if (compare(c, std::int32_t(c.r(6))) != 0) return;
    owner_update(c, 4, 4, lists, owner, id, flag, p, entry + (self_check ? 0x190 : 0x184));
}

// 0x100da110 and 0x100da320: a person's preferences list of clubs gets a club, unless that
// club's list of people (+0x5c or +0x68) already holds the person. r28 the person, r29 the club,
// r6 the end's address, r4 the "held" flag, r3 the scratch, r0 the start.
void club_list_of_person(Cpu& c, std::uint32_t entry, const Lists& lists, std::uint32_t club_staff,
                         std::uint32_t person, std::uint32_t club, std::uint32_t flag) {
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 28});
    const auto p = load_pool(c);
    c.set(6, p.end_at);
    c.set(30, p.salt_at);
    c.set(29, club);
    c.set(31, p.cursor_at);
    compare(c, std::int32_t(person));
    c.set(28, person);
    c.set(0, p.start);
    if (person == 0) return;
    const auto preferences = load32(c, 3, person + person_preferences);
    if (compare_logical(c, preferences) == 0) return;
    if (compare_logical(c, club) == 0) return;
    compare_logical(c, person);
    c.set(4, 0);
    if (person != 0) {
        compare_logical(c, club);
        bool held = false;
        for (unsigned n = 0; club != 0 && n < slot_count && !held; ++n) {
            c.set(3, c.u32(club + slot_at(club_staff, n)));
            held = compare_logical(c, c.r(3), person) == 0;
        }
        if (held) c.set(4, 1);
    }
    if (compare(c, std::int32_t(c.r(4))) != 0) return;
    owner_update(c, 3, 3, lists, person, club, flag, p, entry + 0x16c);
}
}

void club_favour_person(Cpu& c, std::uint32_t club, std::uint32_t person, std::int8_t replace) {
    club_list(c, 0x100d95e0, club_favoured, club, person, std::uint8_t(replace));
}
void club_dislike_person(Cpu& c, std::uint32_t club, std::uint32_t person, std::int8_t replace) {
    club_list(c, 0x100d97a0, club_disliked, club, person, std::uint8_t(replace));
}

void person_favour_person(Cpu& c, std::uint32_t owner, std::uint32_t other, std::int8_t replace) {
    staff_list(c, 0x100d9960, staff_favoured, owner, other, std::uint8_t(replace), false);
}
void person_dislike_person(Cpu& c, std::uint32_t owner, std::uint32_t other, std::int8_t replace) {
    staff_list(c, 0x100d9b80, staff_disliked, owner, other, std::uint8_t(replace), true);
}
void person_favour_club(Cpu& c, std::uint32_t person, std::uint32_t club, std::int8_t replace) {
    club_list_of_person(c, 0x100da110, clubs_favoured, 0x5c, person, club, std::uint8_t(replace));
}
void person_dislike_club(Cpu& c, std::uint32_t person, std::uint32_t club, std::int8_t replace) {
    club_list_of_person(c, 0x100da320, clubs_disliked, 0x68, person, club, std::uint8_t(replace));
}
}
