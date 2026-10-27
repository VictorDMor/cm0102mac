#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
std::uint32_t pending_list(Cpu& c, std::uint32_t manager) { return c.u32(manager + 0x34); }

void reset_cursor(Cpu& c, std::uint32_t list) {
    const auto first_node = c.u32(list + 4);
    c.set(0, first_node);
    c.put32(list + 12, first_node);
}

// Both removal entries perform the same news/transfer cleanup before
// unlinking the record and decrementing the person's pending-count byte.
void unlink_pending(Cpu& c, Program& program, std::uint32_t manager, std::uint32_t person,
                     std::uint32_t record, std::uint32_t news_back, std::uint32_t transfer_back,
                     std::uint32_t unlink_back) {
    const PendingRecord row{c, record};
    const auto news = row.news_id().get();
    c.set(4, std::uint32_t(news));
    if (compare(c, news, -1) != 0) {
        const auto news_holder = c.toc(-0x7b28);
        const auto first = c.u32(record + 0x80), second = c.u32(record + 0x84);
        program.call(0x102dea50, news_back, {c.u32(news_holder), std::uint32_t(news), first, second});
    }
    const auto kind = row.kind().get();
    c.set(0, kind);
    if (compare(c, kind, 2) == 0)
        program.call(0x103ea270, transfer_back, {c.u32(c.toc(-0x7b40)), person});
    program.call(0x10738e60, unlink_back, {pending_list(c, manager), record});
    const auto person_offset = c.u32(person) * Contract::size;
    c.set(0, person_offset);
    const auto holder = c.toc(slot::contract_index);
    c.set(3, holder);
    const auto indices = c.u32(holder);
    const auto person_row = indices + person_offset;
    c.set(3, indices);
    c.set(4, person_row);
    const auto count = c.u8(person_row + 8);
    c.set(3, count);
    c.set(0, std::uint32_t(count) - 1);
    c.put8(person_row + 8, std::uint32_t(count) - 1);
}
}

std::uint32_t find_pending(Cpu& c, std::uint32_t manager_address, std::uint32_t person_id,
                           std::uint32_t club_id) {
    Frame frame(c, 0x1007f720, 0x50, {.first_gpr = 29});
    Kept<31> wanted_club(c, club_id);
    Kept<30> wanted_person(c, person_id);
    Kept<29> manager(c, manager_address);
    Program program(c);
    const auto list = pending_list(c, manager);
    c.set(3, list);
    if (!nonzero_logical(c, list)) return result(c, 0);
    reset_cursor(c, list);
    for (;;) {
        const auto record = program.call(0x10738e00, 0x1007f788, {pending_list(c, manager)});
        if (!nonzero_logical(c, record)) return result(c, 0);
        const auto id = PendingRecord{c, record}.person_id().get();
        c.set(0, std::uint32_t(id));
        if (compare(c, id, std::int32_t(std::uint32_t(wanted_person))) != 0) continue;
        const auto club = PendingRecord{c, record}.club_id().get();
        c.set(0, std::uint32_t(club));
        if (compare(c, club, std::int32_t(std::uint32_t(wanted_club))) == 0) return result(c, record);
    }
}

std::uint32_t find_pending_transfer(Cpu& c, std::uint32_t manager_address, std::uint32_t person_id) {
    Frame frame(c, 0x1007f7b0, 0x40, {.first_gpr = 30});
    Kept<31> wanted_person(c, person_id);
    Kept<30> manager(c, manager_address);
    Program program(c);
    const auto list = pending_list(c, manager);
    c.set(3, list);
    if (!nonzero_logical(c, list)) return result(c, 0);
    reset_cursor(c, list);
    for (;;) {
        const auto record = program.call(0x10738e00, 0x1007f810, {pending_list(c, manager)});
        if (!nonzero_logical(c, record)) return result(c, 0);
        const PendingRecord row{c, record};
        const auto id = row.person_id().get();
        c.set(0, std::uint32_t(id));
        if (compare(c, id, std::int32_t(std::uint32_t(wanted_person))) != 0) continue;
        const auto kind = row.kind().get();
        c.set(0, kind);
        if (compare(c, kind, 2) == 0) return result(c, record);
    }
}

void remove_pending(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address, std::uint32_t club_address) {
    Frame frame(c, 0x1007f840, 0x50, {.first_gpr = 27});
    Kept<27> manager(c, manager_address);
    Kept<28> person(c, person_address);
    Program program(c);
    const auto list = pending_list(c, manager);
    c.set(3, list);
    Kept<29> club_id(c, c.u32(club_address));
    const bool has_list = nonzero_logical(c, list);
    Kept<30> person_id(c, c.u32(person));
    Kept<31> selected(c, c.r(31));
    if (has_list) {
        reset_cursor(c, list);
        for (;;) {
            selected = program.call(0x10738e00, 0x1007f8a0, {pending_list(c, manager)});
            if (!nonzero(c, std::int32_t(std::uint32_t(selected)))) break;
            const PendingRecord row{c, selected};
            const auto id = row.person_id().get();
            c.set(0, std::uint32_t(id));
            if (compare(c, id, std::int32_t(std::uint32_t(person_id))) != 0) continue;
            const auto recorded_club = row.club_id().get();
            c.set(0, std::uint32_t(recorded_club));
            if (compare(c, recorded_club, std::int32_t(std::uint32_t(club_id))) == 0) break;
        }
    } else selected = 0;
    if (nonzero_logical(c, selected))
        unlink_pending(c, program, manager, person, selected, 0x1007f8d4, 0x1007f8f0, 0x1007f8fc);
}

void remove_pending_origin(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address,
                            std::uint32_t wanted_origin) {
    Frame frame(c, 0x1007f930, 0x50, {.first_gpr = 28});
    Kept<30> origin(c, wanted_origin);
    Kept<29> person(c, person_address);
    Kept<28> manager(c, manager_address);
    Kept<31> selected(c, c.r(31));
    Program program(c);
    const auto list = pending_list(c, manager);
    c.set(3, list);
    if (!nonzero_logical(c, list)) return;
    reset_cursor(c, list);
    for (;;) {
        selected = program.call(0x10738e00, 0x1007f994, {pending_list(c, manager)});
        if (!nonzero(c, std::int32_t(std::uint32_t(selected)))) break;
        const PendingRecord row{c, selected};
        const auto id = row.person_id().get(), current_person_id = c.i32(person);
        c.set(3, std::uint32_t(id));
        c.set(0, std::uint32_t(current_person_id));
        if (compare(c, id, current_person_id) != 0) continue;
        const auto recorded_origin = row.origin().get();
        c.set(0, recorded_origin);
        if (compare(c, std::int32_t(recorded_origin), std::int32_t(std::uint32_t(origin))) == 0) break;
    }
    if (nonzero_logical(c, selected))
        unlink_pending(c, program, manager, person, selected, 0x1007f9c4, 0x1007f9e0, 0x1007f9ec);
}
}
