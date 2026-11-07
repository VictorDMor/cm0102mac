// DATABASE.CPP's preference-list queries (database.hpp).
#include "database.hpp"
#include "database_detail.hpp"
#include "guest_values.hpp"

namespace cm::aot::database {
namespace {
using guest::compare_logical;
using guest::nonzero_logical;

// The three ids from `slots` on, compared with `id` in order; each compare
// is the original's cmplw slot,id into cr0, and the original loads each
// slot into r0. `before(i)` runs before slot i is read, for the registers
// the original reloads the record pointer into.
template <class Before>
bool three_slots(Cpu& c, std::uint32_t slots, std::uint32_t id, Before before) {
    for (std::uint32_t i = 0; i < 3; ++i) {
        before(i);
        const auto slot = c.u32(slots + 4 * i);
        c.set(0, slot);
        if (compare_logical(c, slot, id) == 0) return true;
    }
    return false;
}

// The preferences pointer, loaded into r0 and tested as the original does.
bool loaded_nonzero(Cpu& c, std::uint32_t value) {
    c.set(0, value);
    return nonzero_logical(c, value);
}

// 0x100d9480, 0x100d94d0: r3 person, r4 club; the answer goes through r5.
bool club_list(Cpu& c, std::uint32_t someone, std::uint32_t the_club, std::uint32_t list) {
    const bool found = nonzero_logical(c, someone) && nonzero_logical(c, the_club) &&
                       three_slots(c, the_club + list, someone, [](std::uint32_t) {});
    c.set(5, found);
    c.set(3, found);
    return found;
}

// 0x100d9520, 0x100d9580: r3 the other person, r4 the person whose
// preferences are read; the answer goes through r6. The original reloads
// the preferences pointer before each slot (into r5, r5, then r4); reading
// it once leaves the same values, and the first read is where a bad pointer
// faults in both.
bool person_list(Cpu& c, std::uint32_t other, std::uint32_t someone, std::uint32_t list) {
    bool found = false;
    if (nonzero_logical(c, other) && nonzero_logical(c, someone)) {
        const auto kept = c.u32(someone + person::preferences);
        found = loaded_nonzero(c, kept) &&
                three_slots(c, kept + list, other, [&](std::uint32_t i) { c.set(i < 2 ? 5 : 4, kept); });
    }
    c.set(6, found);
    c.set(3, found);
    return found;
}

// 0x100da050, 0x100da0b0: r3 the person, r4 the club. The preferences
// pointer is read and tested before the club is; it is reloaded into r5 for
// the first two slots and r3 for the last.
bool person_club_list(Cpu& c, std::uint32_t someone, std::uint32_t the_club, std::uint32_t list) {
    bool found = false;
    if (nonzero_logical(c, someone)) {
        const auto kept = c.u32(someone + person::preferences);
        found = loaded_nonzero(c, kept) && nonzero_logical(c, the_club) &&
                three_slots(c, kept + list, the_club, [&](std::uint32_t i) { c.set(i < 2 ? 5 : 3, kept); });
    }
    c.set(3, found);
    return found;
}
}

bool club_favours(Cpu& c, std::uint32_t p, std::uint32_t k) { return club_list(c, p, k, club::favoured_staff); }
bool club_dislikes(Cpu& c, std::uint32_t p, std::uint32_t k) { return club_list(c, p, k, club::disliked_staff); }
bool favours_person(Cpu& c, std::uint32_t o, std::uint32_t p) { return person_list(c, o, p, preferences::favourite_staff); }
bool dislikes_person(Cpu& c, std::uint32_t o, std::uint32_t p) { return person_list(c, o, p, preferences::disliked_staff); }
bool dislikes_club(Cpu& c, std::uint32_t p, std::uint32_t k) { return person_club_list(c, p, k, preferences::disliked_clubs); }
bool favours_club(Cpu& c, std::uint32_t p, std::uint32_t k) { return person_club_list(c, p, k, preferences::favourite_clubs); }
}
