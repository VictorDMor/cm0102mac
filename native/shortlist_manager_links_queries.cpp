#include "shortlist_manager_detail.hpp"

// The staff-club links, lookups: the pair indexes' membership test and
// collections, and the link rows' queries and status codes.
namespace cm::aot::shortlist_manager {
using namespace detail;
using namespace links;

std::uint32_t contains_staff_club_link(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t club) {
    Frame frame(c, 0x10362d70, 0x40, {});
    const Manager store{c, links};
    c.set(6, links);
    if (!nonzero_logical(c, store.links_by_staff())) return result(c, 0);
    // The key (staff id, club id): a missing club is -1, which the comparator
    // takes as matching any club.
    const auto key = frame.at(0x38);
    c.put32(key, Field<std::uint32_t>{c, staff});
    if (nonzero_logical(c, club)) c.put32(key + 4, Field<std::uint32_t>{c, club});
    else c.put32(key + 4, std::uint32_t(-1));
    Program program(c);
    const auto found = program.bsearch(key, store.links_by_staff(), store.count_by_staff(), LinkPair::size,
                                       c.toc(slot::compare_10366410_tv), 0x10362dd0);
    return result(c, nonzero_logical(c, found) ? 1 : 0);
}

void collect_club_links(Cpu& c, std::uint32_t links, std::uint32_t club, std::uint32_t out) {
    Frame frame(c, 0x10362e00, 0x50, {.first_gpr = 28});
    const Manager store{c, links};
    const Field<std::int32_t> club_id{c, club};
    keep(c, {{28, club}, {29, out}, {30, links}});
    // The key (any staff, this club) for the club-sorted index.
    const auto key = frame.at(0x38);
    c.put32(key + 4, club_id);
    c.put32(key, std::uint32_t(-1));
    Program program(c);
    const auto found = program.bsearch(key, store.links_by_club(), store.count_by_club(), LinkPair::size,
                                       c.toc(slot::compare_103663b0_tv), 0x10362e50);
    Kept<31> cursor(c, found);
    if (!nonzero(c, std::int32_t(found))) return;
    const std::uint32_t first = store.links_by_club(), count = std::uint32_t(store.count_by_club());
    c.set(3, count);
    c.set(4, first);
    Kept<30> last(c, first + ((count - 1) << 3));
    // Back to the first pair of the club.
    while (compare_logical(c, cursor, first) > 0) {
        const auto before = std::int32_t(c.u32(cursor - 4));
        c.set(3, std::uint32_t(before));
        cursor = cursor - 8;
        if (compare(c, before, club_id) != 0) {
            cursor = cursor + 8;
            break;
        }
    }
    // Then every pair of the club.
    for (;;) {
        const auto owner = std::int32_t(c.u32(cursor + 4));
        c.set(3, std::uint32_t(owner));
        if (compare(c, owner, club_id) != 0) break;
        program.call(0x10738d10, 0x10362ebc, {out, cursor, LinkPair::size});
        if (compare_logical(c, cursor, last) == 0) break;
        cursor = cursor + 8;
    }
}

void collect_staff_links(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t out, std::uint32_t force,
                         std::uint32_t mode, std::uint32_t skip_cached) {
    Frame frame(c, 0x10362ef0, 0x80, {.first_gpr = 20});
    const Manager store{c, links};
    const Field<std::int32_t> staff_id{c, staff};
    const auto service = c.toc(link_slot::staff_service);
    const auto person_count = c.toc(slot::person_count), persons = c.toc(slot::persons);
    keep(c, {{21, links}, {22, staff}, {23, out}, {24, force}, {25, mode}, {26, skip_cached}, {29, service},
             {30, person_count}, {31, persons}});
    Program program(c);
    if (nonzero(c, std::int32_t(out))) program.call(0x10738ca0, 0x10362f30, {out});
    Kept<20> scan_mode(c, extended(std::int8_t(mode)));
    Kept<28> index(c, 0);
    Kept<27> person(c, c.r(27));  // the entry value, until the first record
    // The last 16 person records (newest first), unless mode 3.
    while (compare(c, half_of(index), 16) < 0 && compare(c, std::int32_t(scan_mode), 3) != 0) {
        const auto count = std::int32_t(c.u32(person_count));
        const auto table = c.u32(persons);
        c.set(3, std::uint32_t(count - half_of(index)));
        c.set(4, table);
        person = table + std::uint32_t(count - half_of(index) - 1) * 0x70;
        const std::uint32_t record = person;
        if (nonzero(c, byte_of(c.u8(record + 0x6b)))) {
            const auto club = c.u32(record + 0x24);
            c.set(3, club);
            if (nonzero_logical(c, club)) {
                c.set(4, 0);
                c.set(5, 1);
                const auto reserved = program.call(0x1014c490, 0x10362f7c, {club, 0, 1});
                if (!nonzero(c, byte_of(reserved)) || compare(c, byte_of(mode), 2) != 0) {
                    const auto accepted = program.call(0x1030fbf0, 0x10362fa0, {c.u32(service), staff, record});
                    if (nonzero(c, byte_of(accepted))) {
                        // Appends the pair (staff id, the record's club id).
                        const auto pair = frame.at(0x38);
                        c.put32(pair, staff_id);
                        const auto owner = c.u32(record + 0x24);
                        c.set(6, owner);
                        c.put32(pair + 4, c.u32(owner));
                        program.call(0x10738d10, 0x10362fcc, {out, pair, LinkPair::size});
                    }
                }
            }
        }
        index = index + 1;
    }
    if (nonzero(c, byte_of(skip_cached))) return;

    // The cached links: the staff's pairs of the staff-sorted index whose club
    // entry's best row beats the staff's value.
    c.put32(frame.at(0x44), std::uint32_t(-1));
    const auto key = frame.at(0x40);
    c.put32(key, staff_id);
    const auto found = program.bsearch(key, store.links_by_staff(), store.count_by_staff(), LinkPair::size,
                                       c.toc(slot::compare_10366410_tv), 0x10363014);
    Kept<29> entry(c, found);
    if (!nonzero(c, std::int32_t(found))) return;
    const std::uint32_t first = store.links_by_staff(), count = std::uint32_t(store.count_by_staff());
    c.set(3, count);
    c.set(4, first);
    Kept<30> last(c, first + ((count - 1) << 3));
    // Back to the staff's first pair.
    while (compare_logical(c, entry, first) > 0) {
        entry = entry - 8;
        const auto before = std::int32_t(c.u32(entry));
        c.set(3, std::uint32_t(before));
        if (compare(c, before, staff_id) == 0) continue;
        entry = entry + 8;
        break;
    }
    for (;;) {
        const auto owner = std::int32_t(c.u32(entry));
        c.set(7, staff_id);
        if (compare(c, owner, staff_id) != 0) return;
        const auto club = std::int32_t(c.u32(entry + 4));  // r9
        const std::uint32_t club_entry = store.clubs() + std::uint32_t(club) * ClubEntry::size;  // r8
        c.set(9, std::uint32_t(club));
        c.set(8, club_entry);
        // The club's row held by this staff (the row's value), else the first row's.
        std::uint32_t rows = 0, row_index = 0, offset = 0;
        std::int32_t value = 0;
        c.set(5, 0);
        c.set(6, 0);
        bool matched = false;
        for (unsigned n = 0; n < Row::per_club; ++n) {
            rows = ClubEntry{c, club_entry}.rows();
            if (std::int32_t(c.u32(rows + offset + 0x10)) == staff_id) {
                value = std::int32_t(c.u32(rows + row_index * Row::size + 8));
                matched = true;
                break;
            }
            offset += Row::size;
            ++row_index;
        }
        if (!matched) value = std::int32_t(c.u32(rows + 8));
        c.set(4, rows);
        c.set(5, row_index);
        c.set(6, offset);
        c.set(3, std::uint32_t(value));
        bool append = false, skip = false;
        if (!nonzero(c, byte_of(force))) {
            const std::uint32_t changed = ClubEntry{c, club_entry}.changes();
            if (compare_logical(c, changed, 100) >= 0) skip = true;
        }
        if (!skip) {
            if (compare(c, value, std::int32_t(c.u32(staff + 8))) > 0) {
                const auto stored_club = c.u32(staff + 0x24);
                c.set(3, stored_club);
                if (!nonzero_logical(c, stored_club)) append = true;
                else {
                    const auto stored_id = std::int32_t(c.u32(stored_club));
                    append = compare(c, stored_id, club) != 0;
                }
            }
        }
        if (append) program.call(0x10738d10, 0x10363104, {out, entry, LinkPair::size});
        if (compare_logical(c, entry, last) == 0) return;
        entry = entry + 8;
    }
}

std::uint32_t find_link_row(Cpu& c, std::uint32_t links, std::uint32_t club, std::uint32_t staff, std::int8_t ordinary,
                            std::int8_t either) {
    const std::uint32_t entries = Manager{c, links}.clubs();
    const std::uint32_t rows = ClubEntry{c, entries + std::uint32_t(Field<std::int32_t>{c, club}) * ClubEntry::size}.rows();
    const std::int32_t staff_id = Field<std::int32_t>{c, staff};
    std::uint32_t offset = 0, row = rows;
    for (unsigned index = 0; index < Row::per_club; ++index, offset += Row::size) {
        row = rows + offset;
        const Row candidate{c, row};
        if (staff_id != candidate.target() && (either == 0 || staff_id != candidate.previous())) continue;
        // A match counts when its kind (type 11 or not) is the one asked for.
        const std::uint32_t type = candidate.kind();
        if ((type != 11) != (ordinary != 0)) continue;
        compare(c, std::int32_t(type), 11);
        leave(c, {{4, entries}, {8, offset}, {9, row}, {10, std::uint32_t(staff_id)}});
        return result(c, rows + index * Row::size);
    }
    compare(c, 40, 40);
    leave(c, {{4, entries}, {8, offset}, {9, row}, {10, std::uint32_t(staff_id)}});
    return result(c, 0);
}
}
