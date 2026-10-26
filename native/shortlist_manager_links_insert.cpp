#include "shortlist_manager_detail.hpp"

// The staff-club links, insertion: adding a pair to both indexes (allocating
// and growing them) and resetting the rows of a staff's links.
namespace cm::aot::shortlist_manager {
using namespace detail;
using namespace links;
namespace {
// One of the four allocation failures: the assertion report (source lines
// 0x157f, 0x1588, 0x15a0, 0x15a9) and the exit request. The report's callee
// registers: the success flag, message buffer and pool in r29-r31.
void allocation_failed(Cpu& c, Program& program, std::uint32_t at, std::uint32_t name, std::uint32_t extension,
                       std::uint32_t line, std::uint32_t back) {
    report(c, c.r(31), at, name, extension, line, false);
    program.call(0x1076f7d0, back, {std::uint32_t(-1)});
}
}

void add_staff_club_link(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t club) {
    Frame frame(c, 0x103632f0, 0x100, {.first_gpr = 25});
    const Manager store{c, links};
    keep(c, {{25, links}, {26, staff}, {27, club}, {28, c.toc(slot::compare_10366410_tv)}, {29, c.toc(slot::success)},
             {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    Program program(c);
    const std::uint32_t by_staff_order = c.r(28);
    // Each index is allocated for 100 pairs the first time.
    if (!nonzero_logical(c, store.links_by_staff())) {
        store.links_by_staff() = program.malloc(800, 0x10363330);
        if (!nonzero_logical(c, store.links_by_staff()))
            allocation_failed(c, program, 0x10363354, frame.at(0xc0), frame.at(0x58), 0x157f, 0x10363398);
        store.capacity_by_staff() = 100;
    }
    if (!nonzero_logical(c, store.links_by_club())) {
        store.links_by_club() = program.malloc(800, 0x103633b4);
        if (!nonzero_logical(c, store.links_by_club()))
            allocation_failed(c, program, 0x103633d8, frame.at(0xa0), frame.at(0x50), 0x1588, 0x1036341c);
        store.capacity_by_club() = 100;
    }
    // Whether the pair (staff id, club id) is in the staff-sorted index already.
    bool present = false;
    if (nonzero_logical(c, store.links_by_staff())) {
        const auto key = frame.at(0x38);
        c.put32(key, Field<std::uint32_t>{c, staff});
        if (nonzero_logical(c, club)) c.put32(key + 4, Field<std::uint32_t>{c, club});
        else c.put32(key + 4, std::uint32_t(-1));
        const auto found = program.bsearch(key, store.links_by_staff(), store.count_by_staff(), LinkPair::size,
                                           by_staff_order, 0x10363474);
        present = nonzero_logical(c, found);
    }
    if (nonzero(c, present ? 1 : 0)) return;
    // Grow an index by 100 pairs when it is full.
    if (compare(c, store.capacity_by_staff(), store.count_by_staff()) == 0) {
        store.links_by_staff() = program.realloc(store.links_by_staff(), std::uint32_t(store.capacity_by_staff() + 100) << 3, 0x103634b0);
        if (!nonzero_logical(c, store.links_by_staff()))
            allocation_failed(c, program, 0x103634d4, frame.at(0x80), frame.at(0x48), 0x15a0, 0x10363518);
        store.capacity_by_staff() = store.capacity_by_staff() + 100;
    }
    if (compare(c, store.capacity_by_club(), store.count_by_club()) == 0) {
        store.links_by_club() = program.realloc(store.links_by_club(), std::uint32_t(store.capacity_by_club() + 100) << 3, 0x10363544);
        if (!nonzero_logical(c, store.links_by_club()))
            allocation_failed(c, program, 0x10363568, frame.at(0x60), frame.at(0x40), 0x15a9, 0x103635ac);
        store.capacity_by_club() = store.capacity_by_club() + 100;
    }
    // Append the pair to both indexes and sort each by its comparator.
    // (Quirk: the club is dereferenced even when it is null.)
    const std::int32_t staff_id = Field<std::int32_t>{c, staff};
    const auto append = [&](std::uint32_t pairs, std::int32_t count) {
        const LinkPair at{c, pairs + (std::uint32_t(count) << 3)};
        at.staff() = staff_id;
        at.club() = std::int32_t(Field<std::int32_t>{c, club});
    };
    append(store.links_by_staff(), store.count_by_staff());
    store.count_by_staff() = store.count_by_staff() + 1;
    program.qsort(store.links_by_staff(), store.count_by_staff(), LinkPair::size, by_staff_order, 0x10363604);
    append(store.links_by_club(), store.count_by_club());
    store.count_by_club() = store.count_by_club() + 1;
    program.qsort(store.links_by_club(), store.count_by_club(), LinkPair::size, c.toc(slot::compare_103663b0_tv), 0x10363650);
    // One more link counted in the staff's status record (upper nibble of the byte at +12), unless the club is of the first kind.
    const auto first_kind = program.call(0x1014c2e0, 0x10363668, {club, 0, 1, 0, 0});
    if (nonzero(c, byte_of(first_kind))) return;
    const std::uint32_t table = c.u32(c.toc(link_slot::staff_status));
    const std::uint32_t offset = std::uint32_t(staff_id) * 100 + 0xc;
    const std::uint32_t before = c.u8(table + offset);
    const std::uint32_t counted = (before >> 4) & 0xf;
    const std::uint32_t after = (before & ~0xf0u) | (((counted + 1) << 4) & 0xf0);
    c.put8(table + offset, after);
    leave(c, {{3, counted}, {4, after}, {5, table}, {6, offset}});
}

void reset_linked_club_rows(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t clear_references) {
    Frame frame(c, 0x10363b80, 0x50, {.first_gpr = 27});
    keep(c, {{28, links}, {29, staff}, {30, clear_references}});
    Program program(c);
    const Field<std::int32_t> staff_id{c, staff};
    // A temporary list of the staff's links (16 bytes: count at +0, head at +4, cursor at +0xc).
    Kept<27> list(c, program.operator_new(16, 0x10363ba4));
    if (nonzero(c, std::int32_t(std::uint32_t(list)))) program.call(0x10738c00, 0x10363bb0, {list});
    enter_direct(c, 0x10363bcc, {links, staff, list, 1, 3, 0});
    collect_staff_links(c, links, staff, list, 1, 3, 0);
    const auto count = [&] { return std::int32_t(Field<std::int16_t>{c, list}.get()); };
    if (compare(c, count()) != 0) {
        c.put32(list + 0xc, c.u32(list + 4));
        Kept<31> n(c, 0);
        while (compare(c, std::int32_t(std::uint32_t(n)), count()) < 0) {
            const auto link = program.call(0x10738e00, 0x10363bf0, {list});
            if (nonzero_logical(c, link)) {
                // The link's club entry is marked changed (100), and its rows
                // that name the staff lose a nonnegative +0x18 reference.
                const std::uint32_t entry = Manager{c, links}.clubs() + c.u32(link + 4) * ClubEntry::size;
                c.set(3, c.u32(link + 4) * ClubEntry::size);
                c.set(4, Manager{c, links}.clubs());
                c.set(7, entry);
                ClubEntry{c, entry}.changes() = 100;
                if (nonzero(c, byte_of(clear_references))) {
                    std::uint32_t offset = 0;
                    c.set(3, std::uint32_t(-1));
                    c.set(5, 0);
                    c.set(8, 0);
                    for (std::uint32_t row_index = 0; row_index < Row::per_club; ++row_index) {
                        const Row row{c, ClubEntry{c, entry}.rows() + offset};
                        c.set(6, row.address);
                        c.set(4, staff_id);
                        if (row.target() == staff_id && row.approach() > -1) row.approach() = std::uint32_t(-1);
                        offset += Row::size;
                        c.set(5, offset);
                        c.set(8, row_index + 1);
                    }
                    compare(c, 40, 40);
                }
            }
            n = n + 1;
        }
    }
    program.call(0x10738c20, 0x10363c84, {list, 1});
}
}
