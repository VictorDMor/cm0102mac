#include "shortlist_manager_detail.hpp"

// The staff-club links, transitions: removing a pair from both indexes and
// changing the club's link rows that name the staff.
namespace cm::aot::shortlist_manager {
using namespace detail;
using namespace links;
namespace {
// The link rows' modes (transition_staff_links' fourth argument).
namespace change {
constexpr std::int32_t release = 1, assign_long = 2, expire = 3, drop = 4, assign_short = 5;
}
// Days added to or subtracted from the game date for a row's date.
constexpr std::uint32_t long_term = 120, short_term = 14;

// Removes the pair found in one index by shifting its successors down one
// place, clearing the last pair and counting one less; false (nothing
// changed) when the pair is not there. `base` and `count` are the offsets of
// the index's pointer and used count in the store.
bool remove_pair(Cpu& c, Program& program, const Manager& store, std::uint32_t key, std::uint32_t base,
                 std::uint32_t count, std::uint32_t order, std::uint32_t back) {
    const auto found = program.bsearch(key, c.u32(store.address + base), c.u32(store.address + count), LinkPair::size, order, back);
    if (!nonzero_logical(c, found)) return false;
    const std::uint32_t offset = found - c.u32(store.address + base);
    std::uint32_t index = offset >> 3, at = offset & ~7u;
    while (compare(c, std::int32_t(index), std::int32_t(c.u32(store.address + count)) - 1) < 0) {
        const std::uint32_t pairs = c.u32(store.address + base);
        const std::uint32_t from = pairs + ((index + 1) << 3), to = pairs + at;
        ++index;
        at += 8;
        c.put32(to, c.u32(from));
        c.put32(to + 4, c.u32(from + 4));
    }
    const std::uint32_t used = c.u32(store.address + count);
    c.put32(c.u32(store.address + base) + (used << 3) - 4, 0);
    c.put32(c.u32(store.address + base) + (c.u32(store.address + count) << 3) - 8, 0);
    c.put32(store.address + count, c.u32(store.address + count) - 1);
    return true;
}
}

std::uint32_t transition_staff_links(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t club,
                                     std::uint32_t mode_register, std::uint32_t filter_register) {
    Frame frame(c, 0x10365590, 0xa0, {.first_gpr = 21});
    const Manager store{c, links};
    const Field<std::int32_t> staff_id{c, staff};
    const auto clubs = c.toc(slot::clubs), today = c.toc(slot::today);
    keep(c, {{29, today}, {26, clubs}, {21, links}, {22, staff}, {23, club}, {24, mode_register}, {25, filter_register}});
    Program program(c);
    const auto list = frame.at(0x60);  // a temporary list (16 bytes)
    program.call(0x10738c00, 0x103655c4, {list});
    Kept<28> missing(c, 0);  // set when an index lacked the pair
    const std::int32_t mode = byte_of(mode_register);
    const auto finish = [&](std::uint32_t back, std::uint32_t value) {
        program.call(0x10738c20, back, {list, std::uint32_t(-1)});
        return result(c, value);
    };
    const bool at_stored_club = !nonzero_logical(c, club) || compare_logical(c, club, c.u32(staff + 0x24)) == 0;
    if (at_stored_club && compare(c, mode, change::drop) == 0) {
        // Dropping the staff's stored club (or all clubs): every other link goes through the release mode.
        enter(c, 0x10365604, {links, staff, list, 0, 3, 0});
        collect_staff_links(c, links, staff, list, 0, 3, 0);
        c.put32(frame.at(0x6c), c.u32(frame.at(0x64)));
        for (;;) {
            const auto link = program.call(0x10738e00, 0x1036563c, {list});
            if (!nonzero_logical(c, link)) break;
            const std::uint32_t other = c.u32(clubs) + c.u32(link + 4) * 0x248;
            enter(c, 0x10365634, {links, staff, other, 1, 2});
            transition_staff_links(c, links, staff, other, 1, 2);
        }
    }
    if (!nonzero_logical(c, club)) return finish(0x10365658, 1);
    if (compare(c, mode, change::assign_short) == 0 ||
        (compare(c, mode, change::assign_short) != 0 && compare(c, mode, change::drop) != 0 &&
         compare_logical(c, club, c.u32(staff + 0x24)) != 0)) {
        // Another club than the staff's own: take the pair out of both indexes.
        if (!nonzero_logical(c, store.links_by_staff())) return finish(0x10365698, 0);
        if (!nonzero_logical(c, store.links_by_club())) return finish(0x103656b8, 0);
        const auto key = frame.at(0x58);
        c.put32(key, staff_id);
        if (nonzero_logical(c, club)) c.put32(key + 4, Field<std::uint32_t>{c, club});
        else c.put32(key + 4, std::uint32_t(-1));
        if (!remove_pair(c, program, store, key, 0, 0x10, c.toc(slot::compare_10366410_tv), 0x103656fc)) missing = 1;
        if (!remove_pair(c, program, store, key, 4, 0x14, c.toc(slot::compare_103663b0_tv), 0x103657ac)) missing = 1;
        // The staff's status record counts one link less (the upper nibble of the byte at +12), unless the club is of the first kind.
        const auto first_kind = program.call(0x1014c2e0, 0x1036585c, {club, 0, 1, 0, 0});
        if (!nonzero(c, byte_of(first_kind)) && !nonzero(c, byte_of(missing))) {
            const std::uint32_t table = c.u32(c.toc(link_slot::staff_status));
            const std::uint32_t offset = std::uint32_t(staff_id) * 100 + 0xc;
            const std::uint32_t before = c.u8(table + offset);
            const std::uint32_t counted = (before >> 4) & 0xf;
            c.put8(table + offset, (before & ~0xf0u) | (((counted - 1) << 4) & 0xf0));
        }
    }
    // The club's link rows that name the staff.
    const std::uint32_t entries = store.clubs();
    const std::uint32_t entry = entries + std::uint32_t(Field<std::int32_t>{c, club}) * ClubEntry::size;
    [[maybe_unused]] Kept<27> held_entry(c, entry);
    c.set(6, entries);
    const auto contract = program.call(0x10082f90, 0x103658b8, {c.u32(c.toc(slot::contracts)), staff, club});
    Kept<30> held_contract(c, contract);
    Kept<31> filter(c, extended(std::int8_t(filter_register)));
    Kept<26> index(c, 0);
    missing = 0;  // r28 now holds the byte offset of the row being visited
    const auto rows = [&] { return ClubEntry{c, entry}.rows().get(); };
    const std::int32_t filter_value = std::int32_t(std::uint32_t(filter));
    const auto date_into = [&](std::uint32_t row_offset, std::uint32_t local) {
        // The callee's result (a 5-byte date at `local`) goes into the row's date.
        const Row target{c, rows() + row_offset};
        target.date().year() = c.u16(local + 2);
        target.date().day() = std::int16_t(c.u16(local));
        target.date().derived() = c.u8(local + 4);
    };
    for (; compare(c, byte_of(index), 40) < 0; index = index + 1, missing = std::uint32_t(missing) + Row::size) {
        const std::uint32_t offset = missing;
        // Filter 1 skips the rows of type 11, filter 0 accepts only those.
        if (compare(c, filter_value, 1) == 0) {
            if (compare(c, byte_of(c.u8(rows() + offset)), 11) == 0) continue;
        }
        if (!nonzero(c, byte_of(filter_register))) {
            if (compare(c, byte_of(c.u8(rows() + offset)), 11) != 0) continue;
        }
        c.set(3, rows() + offset);
        c.set(4, staff_id);
        const Row row{c, rows() + offset};
        if (compare(c, staff_id, row.target()) != 0) {
            // Mode 4 also matches the intervening reference.
            if (compare(c, mode, change::drop) != 0) continue;
            if (compare(c, staff_id, row.previous()) != 0) continue;
        }
        const std::uint32_t m1 = mode;
        switch (m1) {
        case change::release: {
            c.set(5, std::uint32_t(-1));
            row.target() = -1;
            c.set(6, offset + 0x14);
            c.set(3, rows());
            Row{c, rows() + offset}.approach() = -1;
            c.set(3, rows());
            c.set(4, staff_id);
            if (compare(c, staff_id, Row{c, rows() + offset}.previous()) == 0)
                Row{c, rows() + offset}.previous() = -1;
            c.set(7, rows());
            c.set(4, offset + 0x28);
            c.set(5, 0);
            c.set(3, links);
            Row{c, rows() + offset}.flags() = Row{c, rows() + offset}.flags() & 0x7f;
            c.set(5, club);
            c.set(6, staff);
            c.set(4, rows() + offset);
            program.call(0x1035e9f0, 0x103659c4, {links, rows() + offset, club, staff});
            break;
        }
        case change::assign_long:
        case change::assign_short: {
            const bool longer = m1 == change::assign_long;
            const std::uint32_t local = frame.at(longer ? 0x50 : 0x48);
            program.call(0x10717890, longer ? 0x103659d8 : 0x10365a7c,
                         {local, today, longer ? long_term : short_term});
            date_into(offset, local);
            const Row target{c, rows() + offset};
            target.previous() = std::int32_t(staff_id);
            target.approach() = -1;
            target.weight() = 0;
            target.budget() = 0;
            if (nonzero_logical(c, held_contract))
                c.put8(held_contract + 0x60, (c.u8(held_contract + 0x60) & 0xf0) | ((c.u8(rows() + offset + 0x28) >> 3) & 0xf));
            ClubEntry{c, entry}.changes() = 100;
            leave(c, {{3, nonzero_logical(c, held_contract) ? std::uint32_t(byte_of((c.u8(rows() + offset + 0x28) >> 3) & 0xf)) : rows()},
                      {4, rows()}, {5, 0}, {6, rows()}, {7, std::uint32_t(-1)}, {8, rows()}, {9, std::uint32_t(staff_id)}});
            break;
        }
        case change::expire: {
            c.set(7, std::uint32_t(-1));
            row.target() = -1;
            const std::uint32_t local = frame.at(0x40);
            c.set(6, rows());
            Row{c, rows() + offset}.approach() = -1;
            program.call(0x10717b20, 0x10365b34, {local, today, 120});
            c.set(3, rows());
            c.set(5, offset + 0x14);
            date_into(offset, local);
            c.set(3, rows());
            c.set(4, staff_id);
            if (compare(c, staff_id, Row{c, rows() + offset}.previous()) == 0)
                Row{c, rows() + offset}.previous() = -1;
            break;
        }
        case change::drop: {
            if (compare(c, staff_id, row.previous()) == 0) row.previous() = -1;
            c.set(3, rows());
            c.set(5, offset + 0x10);
            c.set(4, staff_id);
            if (compare(c, staff_id, Row{c, rows() + offset}.target()) != 0) break;
            c.set(7, std::uint32_t(-1));
            Row{c, rows() + offset}.target() = -1;
            const std::uint32_t local = frame.at(0x38);
            c.set(6, rows());
            Row{c, rows() + offset}.approach() = -1;
            program.call(0x10717b20, 0x10365bcc, {local, today, 120});
            c.set(3, rows());
            c.set(4, rows() + offset + 0x1c);
            date_into(offset, local);
            break;
        }
        default: break;
        }
    }
    return finish(0x10365c10, 1);
}
}
