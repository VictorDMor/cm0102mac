// DISCIPLINE.CPP bans block (discipline_bans.hpp), native-first: registering a ban and clearing one.
#include "discipline_bans.hpp"
#include "discipline_bans_detail.hpp"

namespace cm::aot::discipline {
using guest::Cpu;
using guest::Frame;
using namespace manager_arrays;

namespace {
constexpr std::int32_t array_step = 100, array_limit = 32000, index_step = 20;
// Frame offsets of the assertion buffers (file name, extension) of each failure site.
struct Site { std::uint32_t at, name, extension, line; };
constexpr Site ban_array_full{0x100dfdc4, 0x90, 0x48, 0x400}, ban_array_failed{0x100dfe54, 0x70, 0x40, 0x40f},
    index_failed{0x100dffe0, 0x50, 0x38, 0x43d}, entry_missing{0x100e019c, 0x48, 0x38, 0x47c};

void report(Cpu& c, const Frame& frame, const Site& site) {
    const auto pool = c.toc(toc_slot::strings);
    guest::report_assertion(c, {site.at, pool + 0x50, pool + 0x5f, pool + 0x6c, frame.at(std::int32_t(site.name)),
                                frame.at(std::int32_t(site.extension)), site.line});
}

// r24-r31 of register_ban where the original has them (r27 the ban index, -1 until found; r28 the slot
// counter, 4 once the slot loops are done): the generated callees save them in their frames, and a ruling
// body later copies a ban struct it never fully wrote from that stack into a ban record (bytes +0xd..+0xf,
// part of +0x14; the BlockMoveData reader 0x10bf7bbc that tools/stack_contract.py lists for 0x100dfb30).
void registers(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t ban_at, std::uint32_t index,
               std::uint32_t counter) {
    c.set({{24, manager}, {25, person}, {26, ban_at}, {27, index}, {28, counter},
           {29, c.toc(toc_slot::assert_flag)}, {30, c.toc(toc_slot::message_buffer)}, {31, c.toc(toc_slot::strings)}});
}

// The ban index handed to each body of the ban: RULING_BODY.CPP files the person under it.
void announce(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t ban_at, std::uint32_t index) {
    for (std::int16_t slot = 0; slot < ban::body_slots; ++slot) {
        const auto body = c.i8(ban_at + ban::bodies + std::uint32_t(slot));
        if (body == -1) continue;
        registers(c, manager, person, ban_at, index, std::uint32_t(slot));
        c.set({{3, ruling_body(c, manager, body)}, {4, person}, {5, index}});
        c.call(callee::add_entry, 0x100dff74);
    }
}

// Enters the person in the sorted index (grown first when full), then sorts it again.
void add_to_index(Cpu& c, const Frame& frame, std::uint32_t manager, std::uint32_t person, std::uint32_t ban_at,
                  std::uint32_t ban_index) {
    const auto set_registers = [&] { registers(c, manager, person, ban_at, ban_index, 4); };
    if (c.i16(manager + sorted_capacity) == c.i16(manager + sorted_count)) {
        c.put16(manager + sorted_capacity, std::uint32_t(c.i16(manager + sorted_capacity) + index_step));
        c.set({{3, c.u32(manager + sorted)},
               {4, (std::uint32_t(std::int32_t(c.i16(manager + sorted_capacity))) << 3) & ~7u}});
        set_registers();
        c.call(callee::reallocate, 0x100dffbc);
        c.put32(manager + sorted, c.r(3));
        if (!c.u32(manager + sorted)) return set_registers(), report(c, frame, index_failed);
    }
    const auto entry_at = [&] {
        return c.u32(manager + sorted) + ((std::uint32_t(std::int32_t(c.i16(manager + sorted_count))) << 3) & ~7u);
    };
    c.put32(entry_at(), person);
    const auto people = c.u32(c.toc(toc_slot::people));
    const auto key_record = c.u32(people + person * person_record::size + person_record::key_source);
    c.put32(entry_at() + 4, c.u32(key_record + person_record::key));
    c.put16(manager + sorted_count, c.i16(manager + sorted_count) + 1u);
    c.set({{3, c.u32(manager + sorted)}, {4, std::uint32_t(std::int32_t(c.i16(manager + sorted_count)))},
           {5, sorted_entry_size}, {6, c.toc(toc_slot::comparator)}});
    set_registers();
    c.call(callee::qsort, 0x100e007c);
}
}

void register_ban(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t ban_at) {
    Frame frame(c, 0x100dfb30, 0xd0, {.first_gpr = 24});
    const auto not_found = std::uint32_t(-1);
    // A body that already holds a ban of the person: the new ban is added to it and the call ends.
    for (std::int16_t slot = 0; slot < ban::body_slots; ++slot) {
        const auto body = c.i8(ban_at + ban::bodies + std::uint32_t(slot));
        if (body == -1) continue;
        c.set({{3, ruling_body(c, manager, body)}, {4, person}, {5, 0}});
        registers(c, manager, person, ban_at, not_found, std::uint32_t(slot));
        c.call(callee::find_entry, 0x100dfb98);
        const auto found = c.r(3);
        const auto held = found ? c.i16(found + body_entry_index) : std::int16_t(-1);
        if (found && held > -1) {
            const auto record = ban_record(c, manager, held);
            // Both bans count matches: the matches add; else the days of the new ban are added.
            if (c.i8(record + ban::matches) != -1 && c.i8(ban_at + ban::matches) != -1)
                c.put8(record + ban::matches, c.u8(record + ban::matches) + c.u8(ban_at + ban::matches));
            else
                c.put16(record + ban::days, std::uint32_t(c.i16(record + ban::days) + c.i16(ban_at + ban::days)));
            // The flag is set on the caller's ban first and copied to the record from it.
            c.put8(ban_at + ban::flags, c.u8(ban_at + ban::flags) | ban::merged);
            c.put8(record + ban::flags, c.u8(record + ban::flags) | ban::merged);
            return;
        }
        c.put8(ban_at + ban::flags, c.u8(ban_at + ban::flags) & ~ban::merged);
    }

    std::int32_t index = -1;
    const auto records = c.u32(manager + manager_arrays::bans);
    for (std::int32_t at = 0; std::int16_t(at) < c.i16(manager + ban_count); ++at) {
        const auto record = records + std::uint32_t(at) * ban::size;
        if (c.i8(record + ban::matches) == -1 && c.i16(record + ban::days) == -1) {
            copy_ban(c, ban_at, record);
            index = at;
            break;
        }
    }
    // Both slot loops are over: r28 is 4 from here on, r27 the index (-1 when none was found).
    const auto set_registers = [&] { registers(c, manager, person, ban_at, std::uint32_t(index), 4); };
    if (std::int16_t(index) == -1) {
        const auto count = c.i16(manager + ban_count), capacity = c.i16(manager + ban_capacity);
        if (count < capacity) {
            const auto base = c.u32(manager + manager_arrays::bans);
            c.put16(manager + ban_count, std::uint32_t(count + 1));
            copy_ban(c, ban_at, base + std::uint32_t(std::int32_t(count)) * ban::size);
            index = std::int16_t(c.i16(manager + ban_count) - 1);
        } else {
            if (capacity >= array_limit) return set_registers(), report(c, frame, ban_array_full);
            c.put16(manager + ban_capacity, std::uint32_t(capacity + array_step));
            const auto bytes = std::uint32_t(std::int32_t(c.i16(manager + ban_capacity))) * ban::size;
            if (c.u32(manager + manager_arrays::bans) == 0) {
                c.set(3, bytes);
                set_registers();
                c.call(callee::allocate, 0x100dfe1c);
            } else {
                c.set({{3, c.u32(manager + manager_arrays::bans)}, {4, bytes}});
                set_registers();
                c.call(callee::reallocate, 0x100dfe30);
            }
            // A failed reallocation leaves the null pointer in the manager.
            c.put32(manager + manager_arrays::bans, c.r(3));
            const auto base = c.u32(manager + manager_arrays::bans);
            if (!base) return set_registers(), report(c, frame, ban_array_failed);
            index = c.i16(manager + ban_count);
            c.put16(manager + ban_count, std::uint32_t(index + 1));
            copy_ban(c, ban_at, base + std::uint32_t(index) * ban::size);
            // The new tail of the array is marked free.
            auto free_at = c.u32(manager + manager_arrays::bans) + std::uint32_t(std::int32_t(c.i16(manager + ban_count))) * ban::size;
            for (std::int32_t at = c.i16(manager + ban_count); std::int16_t(at) < c.i16(manager + ban_capacity); ++at) {
                c.put8(free_at, 0xff);
                c.put16(free_at + 2, 0xffff);
                free_at += ban::size;
            }
        }
    }

    announce(c, manager, person, ban_at, std::uint32_t(index));
    if (c.i8(ban_at + ban::bodies + 3) != -1) add_to_index(c, frame, manager, person, ban_at, std::uint32_t(index));
}

void clear_ban(Cpu& c, std::uint32_t manager, std::uint32_t person, std::int16_t at) {
    Frame frame(c, 0x100e0090, 0x90, {.first_gpr = 22});
    const auto record = ban_record(c, manager, at);
    const auto key_at = frame.at(0x40);
    // r23-r31 hold what the original keeps in them (r22 too once the fourth body's search starts): the
    // generated callees save them in their frames, and a ruling body copies a ban struct it never fully
    // wrote from that stack into a ban record (the BlockMoveData reader 0x10bf7bbc that
    // tools/stack_contract.py lists for 0x100e0090).
    const auto set_registers = [&](std::int8_t slot) {
        c.set({{23, manager}, {24, person}, {25, std::uint32_t(std::int32_t(slot))}, {26, record},
               {27, c.toc(toc_slot::comparator)}, {28, c.toc(toc_slot::people)}, {29, c.toc(toc_slot::assert_flag)},
               {30, c.toc(toc_slot::message_buffer)}, {31, c.toc(toc_slot::strings)}});
    };
    for (std::int8_t slot = 0; slot < ban::body_slots; ++slot) {
        const auto body = c.i8(record + ban::bodies + std::uint32_t(std::int32_t(slot)));
        if (body == -1) continue;
        c.set({{3, ruling_body(c, manager, body)}, {4, person}});
        set_registers(slot);
        c.call(callee::drop_entry, 0x100e0104);
        if (slot != ban::body_slots - 1) continue;
        // The fourth body is the one the sorted index follows: its entry is {person, key}, found by
        // the key of the person's first record, else of the second.
        const auto search = [&] {
            c.set({{3, key_at}, {4, c.u32(manager + manager_arrays::sorted)},
                   {5, std::uint32_t(std::int32_t(c.i16(manager + sorted_count)))}, {6, sorted_entry_size},
                   {7, c.toc(toc_slot::comparator)}});
        };
        const auto row = [&] { return c.u32(c.toc(toc_slot::people)) + person * person_record::size; };
        c.set(22, person * person_record::size);
        c.put32(key_at, person);
        c.put32(key_at + 4, c.u32(c.u32(row() + person_record::key_source) + person_record::key));
        search();
        set_registers(slot);
        c.call(callee::bsearch, 0x100e0144);
        if (!c.r(3)) {
            if (const auto second = c.u32(row() + person_record::second_key_source)) {
                c.put32(key_at + 4, c.u32(second + person_record::key));
                search();
                set_registers(slot);
                c.call(callee::bsearch, 0x100e0180);
            }
            if (!c.r(3)) {
                set_registers(slot);
                report(c, frame, entry_missing);
                continue;
            }
        }
        // The last entry takes the found one's place and the index is sorted again.
        const auto found = c.r(3);
        c.put16(manager + sorted_count, c.i16(manager + sorted_count) - 1u);
        const auto last = c.u32(manager + manager_arrays::sorted) +
                          ((std::uint32_t(std::int32_t(c.i16(manager + sorted_count))) << 3) & ~7u);
        c.put32(found, c.u32(last));
        c.put32(found + 4, c.u32(last + 4));
        c.set({{3, c.u32(manager + manager_arrays::sorted)}, {4, std::uint32_t(std::int32_t(c.i16(manager + sorted_count)))},
               {5, sorted_entry_size}, {6, c.toc(toc_slot::comparator)}});
        set_registers(slot);
        c.call(callee::qsort, 0x100e021c);
    }
    c.put8(record + ban::matches, 0xff);
    c.put16(record + ban::days, 0xffff);
}
}
