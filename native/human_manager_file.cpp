// HUMAN_MANAGER.CPP: the humans' records in a saved game's file (0x1014a300 reads, 0x1014b0a0 writes).
//
// The file holds, in this order: the table's count; for each human the first two tactic slots, the
// fields below, the list, the count of sorted entries and the entries themselves; the table's third
// and fourth words; then for each human the last three tactic slots. ZIPDIR.CPP's file layer does the
// transfers (0x107475b0 opens, 0x10747a80 reads, 0x10747cc0 writes, 0x10747900 closes). The reader's
// old layouts (saved games below a version) read 0x1e5c-byte images and rebuild the slots from them.
#include "human_manager_file.hpp"
#include "human_manager_detail.hpp"

namespace cm::aot::human_manager {
namespace {
namespace callee {
inline constexpr std::uint32_t open = 0x107475b0, read = 0x10747a80, write = 0x10747cc0, close = 0x10747900,
                               set_option = 0x10113b60,
                               allocate_block = 0x10775130,
                               gauge = 0x10775550;  // the absolute value of a word (labs)
}
namespace file_toc {
inline constexpr std::int32_t scratch = -0x73f4,        // the buffer the older layouts are read into (0x1e5c * 3)
                              gauge = -0x7b74,          // the saved game's version (the word in this cell)
                              club_cells = -0x7be8, competition_cells = -0x7bbc, rating_cells = -0x7548;
}
// Versions: a saved game below each has the older layout.
inline constexpr std::uint32_t old_tactics_below = 0x3e021, old_list_below = 0x3e005, old_staff_below = 0x3e001;
inline constexpr std::uint32_t file_name = 0x8a;  // "human_manager.dat" in the pool
inline constexpr std::uint32_t handle_slot = 0x38;  // the frame word holding the file's stream

// The fields between the first two tactic slots and the list: offset, item size, item count.
struct Field { std::uint32_t offset, size, count; };
constexpr Field fields[] = {
    {0x9984, 1, 1},    {0x9985, 1, 1},    {0x998c, 0x64, 1}, {0x99f0, 4, 1},    {0x99f4, 4, 1},
    {0x99f8, 1, 1},    {0x99fc, 4, 1},    {0x9a00, 1, 1},    {0x9a01, 1, 0x32}, {0x9a33, 1, 0x32},
    {0x9a65, 1, 1},    {0x9a66, 2, 1},    {0x9a68, 0x50, 1}, {0x9b40, 1, 0x100}, {0x9c40, 1, 0x400},
    {0xa040, 1, 0x100}, {0xa140, 1, 0x400}, {0xa540, 1, 0x100}, {0xa640, 0x10, 1}, {0x9ab8, 1, 5},
};
constexpr std::uint32_t list_offset = 0x9ac0, list_item = 8, list_count = 0x10, list_words_offset = 0x40;
constexpr std::uint32_t entries_offset = 0xa650, entries_copy = 0xa654, entries_count = 0xa658, entry_size = 0x20;
constexpr std::uint32_t last_slots_at = 2 * record::tactic_size;  // 0x3d68
constexpr std::uint32_t reader_field_back = 0x1014a6e4, writer_field_back = 0x1014b25c, step = 0x20;

std::uint32_t sext8(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
}

// The older layout of a tactic slot: a 0x1e5c-byte image read into the scratch buffer, its first 0x1888
// bytes copied over the slot, its eleven instructions rebuilt (each pair gets the image's word at +0x58c +
// slot * 4 and a zero, then two calls of 0x10113e30), then its last 0x62c bytes copied to +0x18e0.
// `registers` names the original's registers for the slot index, the image offset, the image address,
// the slot's offset in the table, the slot index * 4 and the instruction index.
struct OldSlotRegisters { unsigned slot, image_offset, image, slot_offset, slot_times_4, instruction; };
template <class Call>
void read_old_slot(Cpu& c, CallRegisters& regs, const OldSlotRegisters& r, Call&& call, std::uint32_t humans,
                   std::uint32_t handle, std::uint32_t scratch, std::uint32_t slot, std::uint32_t slot_offset,
                   std::uint32_t copy_offset, const std::uint32_t (&backs)[5],
                   std::initializer_list<std::pair<unsigned, std::uint32_t>> at_start,
                   std::initializer_list<std::pair<unsigned, std::uint32_t>> after_read) {
    const auto records = [&] { return c.u32(humans + table::records); };
    const auto image_offset = slot * 0x1e5c, image = scratch + image_offset;
    regs.hold(at_start);
    regs.hold({{r.image_offset, image_offset}, {r.slot, slot}, {r.image, image}});
    call(0x10747a80, backs[0], {image, 0x1e5c, 1, handle});
    regs.hold(after_read);
    call(0x10739ef0, backs[1], {records() + copy_offset, image, 0x1888});
    regs.hold({{r.slot_offset, slot_offset}, {r.slot_times_4, sext8(slot) << 2}});
    for (std::uint32_t instruction = 0; instruction < 11; ++instruction) {
        regs.hold({{r.instruction, instruction}});
        const auto base = records();
        const auto pair = slot_offset + base + (sext8(instruction) << 3) + 0x1888;
        c.put32(pair + 4, c.u32(slot_offset + (sext8(slot) << 2) + base + 0x58c));
        c.put32(pair, 0);
        call(0x10113e30, backs[2], {records() + slot_offset, sext8(instruction), 8, 0});
        call(0x10113e30, backs[3], {records() + copy_offset, sext8(instruction), 2, 0});
    }
    regs.hold({{r.instruction, 11}});
    call(0x10739ef0, backs[4], {records() + copy_offset + 0x18e0, image + 0x1888, 0x62c});
}

std::uint32_t read_humans(Cpu& c, std::uint32_t humans, std::uint32_t file) {
    CallRegisters regs(c);
    const Frame frame(c, 0x1014a300, 0x240, {.first_gpr = 13});
    const auto pool = c.toc(toc::pool), scratch = c.toc(file_toc::scratch), version_cell = c.toc(file_toc::gauge),
               staff_count_cell = c.toc(toc::staff_count), club_cells = c.toc(file_toc::club_cells);
    regs.hold({{13, staff_count_cell}, {14, club_cells}, {15, humans}, {24, scratch}, {25, version_cell},
               {26, c.toc(toc::message)}, {27, pool}});
    const auto call = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        regs.apply();
        return invoke(c, entry, back, arguments);
    };
    const auto fail = [&](const Site& site) { regs.apply(); report(c, frame, site); };
    const auto handle = [&] { return c.u32(frame.at(handle_slot)); };
    const auto close = [&](std::uint32_t back) { call(callee::close, back, {frame.at(handle_slot)}); };
    const auto read = [&](std::uint32_t back, std::uint32_t buffer, std::uint32_t size, std::uint32_t count) {
        return std::int32_t(call(callee::read, back, {buffer, size, count, handle()}));
    };
    const auto records = [&] { return c.u32(humans + table::records); };
    const auto human_count = [&] { return c.i32(humans + table::count); };
    // The saved game's version, through 0x10775550 (its absolute value) each time it is asked.
    const auto version = [&](std::uint32_t back) { return std::int32_t(call(callee::gauge, back, {c.u32(version_cell)})); };
    if (!file) { fail({0x1014a348, 0x184, 0x7c, 0x80e}); return 0; }
    c.put32(frame.at(handle_slot), call(callee::open, 0x1014a3a8, {file, pool + file_name, 0, 0x16, 4}));
    if (!handle()) { fail({0x1014a3c8, 0x164, 0x74, 0x816}); return 0; }
    if (read(0x1014a424, humans + table::count, 4, 1) != 1) {
        fail({0x1014a440, 0x144, 0x6c, 0x81d});
        close(0x1014a488);
        return 0;
    }
    const auto count = c.u32(humans + table::count);
    regs.hold({{16, count}, {18, 0}});
    auto array = call(runtime::allocate, 0x1014a4ac, {count * record::size + 0x10});
    regs.hold({{17, array}});
    if (array) {
        regs.hold({{18, 1}});
        array = call(runtime::construct_array, 0x1014a4d0,
                     {array, c.toc(toc::record_constructor), c.toc(toc::record_destructor), record::size, count});
        regs.hold({{17, array}});
    }
    c.put32(humans + table::records, array);
    if (!c.u32(humans + table::records)) {
        fail({0x1014a4f8, 0x124, 0x64, 0x826});
        close(0x1014a540);
        return 0;
    }
    // Each human: the first two tactic slots, the fields, the list and the sorted entries.
    regs.hold({{28, old_tactics_below}, {30, 0x8000}, {31, old_list_below}});
    std::uint32_t human = 0, offset = 0;
    for (; regs.hold({{18, human}, {21, offset}}), std::int32_t(human) < human_count(); ++human, offset += record::size) {
        if (version(0x1014a570) < std::int32_t(old_tactics_below)) {
            for (std::uint32_t slot = 0; slot < 2; ++slot)
                read_old_slot(c, regs, {17, 23, 20, 19, 29, 16}, call, humans, handle(), scratch, slot,
                              offset + slot * record::tactic_size, offset + slot * record::tactic_size,
                              {0x1014a59c, 0x1014a5b4, 0x1014a60c, 0x1014a628, 0x1014a650},
                              {{22, slot * record::tactic_size}}, {});
            regs.hold({{17, 2}, {22, 2 * record::tactic_size}, {23, 2 * 0x1e5c}});
        } else {
            call(callee::read, 0x1014a680, {records() + offset, record::tactic_size, 2, handle()});
        }
        // A slot whose option word (+0x588) reads below 0x8000 gets the team option 0x20000 set.
        if (const auto slot = records() + offset; c.i32(slot + 0x588) < 0x8000)
            call(callee::set_option, 0x1014a6a0, {slot, 0x20000});
        if (const auto slot = records() + offset + record::tactic_size; c.i32(slot + 0x588) < 0x8000)
            call(callee::set_option, 0x1014a6c4, {slot, 0x20000});
        for (unsigned k = 0; k < std::size(fields); ++k)
            call(callee::read, reader_field_back + step * k, {records() + offset + fields[k].offset, fields[k].size, fields[k].count, handle()});
        const auto record_now = [&] { return records() + offset; };
        if (version(0x1014a94c) < std::int32_t(old_list_below)) {
            // Sixteen words, each stored in the first half of an 8-byte pair with -1 in the second.
            const auto image = frame.at(0x1a4);
            call(callee::read, 0x1014a968, {image, 4, list_count, handle()});
            for (unsigned half = 0; half < 2; ++half)
                for (unsigned word = 0; word < 8; ++word) {
                    const auto at = record_now() + list_offset + half * list_words_offset + word * 8;
                    c.put32(at, c.u32(image + half * 0x20 + word * 4));
                    c.put32(at + 4, 0xffffffffu);
                }
            c.set(7, c.u32(image + 0x20 + 7 * 4));  // the unrolled copy's last load (0x1014aac8) leaves r7
        } else {
            call(callee::read, 0x1014ab1c, {record_now() + list_offset, list_item, list_count, handle()});
        }
        if (read(0x1014ab3c, record_now() + entries_count, 4, 1) != 1) { fail({0x1014ab58, 0x104, 0x5c, 0x878}); return 0; }
        c.put32(record_now() + entries_copy, c.u32(record_now() + entries_count));
        if (const auto entries = c.u32(record_now() + entries_copy); entries == 0) {
            c.put32(record_now() + entries_offset, 0);
        } else {
            c.put32(record_now() + entries_offset, call(callee::allocate_block, 0x1014abd4, {entries << 5}));
            if (!c.u32(record_now() + entries_offset)) { fail({0x1014ac10, 0xe4, 0x54, 0x888}); return 0; }
            const auto got = call(callee::read, 0x1014ac68, {c.u32(record_now() + entries_offset), entry_size,
                                                             c.u32(record_now() + entries_count), handle()});
            if (c.i32(record_now() + entries_count) != std::int32_t(got)) { fail({0x1014ac94, 0xc4, 0x4c, 0x890}); return 0; }
        }
    }
    // The links: each record's competition, clubs and rating row, from the indices the file holds.
    {
        const auto competitions = c.u32(c.toc(file_toc::competition_cells)), ratings = c.u32(c.toc(file_toc::rating_cells));
        std::uint32_t at = 0;
        for (std::int32_t i = 0; i < human_count(); ++i, at += record::size) {
            const auto row = records() + at;
            c.put32(row + 0x99f0, 0);
            c.put32(row + 0x99f4, 0);
            if (c.u32(row + 0x9a7c) == 0) continue;
            c.put32(row + 0x9a7c, competitions + c.u32(row + 0x9a68) * 0x6c);
            c.put32(row + 0x9a84, c.u32(club_cells) + c.u32(row + 0x9a74) * 0x248);
            c.put32(row + 0x9a88, c.u32(club_cells) + c.u32(row + 0x9a78) * 0x248);
            c.put32(row + 0x9a80, ratings + c.u32(row + 0x9a6c) * 0x50);
        }
    }
    if (version(0x1014ade0) < std::int32_t(old_staff_below)) {
        // The oldest saves: the humans' sixteen staff rows get byte +0x65 = 0x14.
        std::uint32_t row = c.u32(staff_count_cell) - 0x10, at = row * 0x70;
        for (; std::int32_t(row) < c.i32(staff_count_cell); ++row, at += 0x70)
            c.put8(c.u32(c.toc(toc::people)) + at + 0x65, 0x14);
    }
    if (read(0x1014ae40, humans + 8, 4, 1) != 1) {
        fail({0x1014ae5c, 0xa4, 0x44, 0x8b7});
        close(0x1014aea4);
        return 0;
    }
    if (read(0x1014aec0, humans + 0xc, 4, 1) != 1) {
        fail({0x1014aedc, 0x84, 0x3c, 0x8bf});
        close(0x1014af24);
        return 0;
    }
    // The last three slots of every record.
    regs.hold({{14, old_tactics_below}});
    human = 0;
    offset = 0;
    for (; regs.hold({{21, human}, {17, offset}}), std::int32_t(human) < human_count(); ++human, offset += record::size) {
        if (version(0x1014af48) < std::int32_t(old_tactics_below)) {
            for (std::uint32_t slot = 0; slot < 3; ++slot) {
                const auto slot_offset = (slot + 2) * record::tactic_size;  // r19, set after the image's read
                read_old_slot(c, regs, {22, 16, 18, 20, 13, 23}, call, humans, handle(), scratch, slot,
                              offset + slot_offset, offset + slot_offset,
                              {0x1014af70, 0x1014af90, 0x1014afe8, 0x1014b004, 0x1014b02c}, {}, {{19, slot_offset}});
            }
            regs.hold({{22, 3}, {16, 3 * 0x1e5c}});
        } else {
            call(callee::read, 0x1014b05c, {records() + offset + last_slots_at, record::tactic_size, 3, handle()});
        }
    }
    close(0x1014b07c);
    return 1;
}

std::uint32_t write_humans(Cpu& c, std::uint32_t humans, std::uint32_t file) {
    CallRegisters regs(c);
    const Frame frame(c, 0x1014b0a0, 0x120, {.first_gpr = 26});
    const auto pool = c.toc(toc::pool);
    regs.hold({{29, c.toc(toc::success)}, {30, c.toc(toc::message)}, {26, humans}, {31, pool}});
    const auto call = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        regs.apply();
        return invoke(c, entry, back, arguments);
    };
    const auto fail = [&](const Site& site) { regs.apply(); report(c, frame, site); };
    const auto handle = [&] { return c.u32(frame.at(handle_slot)); };
    const auto close = [&](std::uint32_t back) { call(callee::close, back, {frame.at(handle_slot)}); };
    const auto write = [&](std::uint32_t back, std::uint32_t buffer, std::uint32_t size, std::uint32_t count) {
        return std::int32_t(call(callee::write, back, {buffer, size, count, handle()}));
    };
    const auto records = [&] { return c.u32(humans + table::records); };
    const auto human_count = [&] { return c.i32(humans + table::count); };

    if (!file) { fail({0x1014b0dc, 0xe4, 0x5c, 0x8fc}); return 0; }
    c.put32(frame.at(handle_slot), call(callee::open, 0x1014b138, {file, pool + file_name, 1, 0x16, 4}));
    if (!handle()) { fail({0x1014b158, 0xc4, 0x54, 0x904}); return 0; }
    if (write(0x1014b1b0, humans + table::count, 4, 1) != 1) {
        fail({0x1014b1cc, 0xa4, 0x4c, 0x90b});
        close(0x1014b210);
        return 0;
    }
    std::uint32_t human = 0, offset = 0;
    for (; regs.hold({{27, human}, {28, offset}}), std::int32_t(human) < human_count(); ++human, offset += record::size) {
        write(0x1014b23c, records() + offset, record::tactic_size, 2);
        for (unsigned k = 0; k < std::size(fields); ++k)
            write(writer_field_back + step * k, records() + offset + fields[k].offset, fields[k].size, fields[k].count);
        write(0x1014b4dc, records() + offset + list_offset, list_item, list_count);
        write(0x1014b4fc, records() + offset + entries_count, 4, 1);
        const auto row = records() + offset;
        write(0x1014b51c, c.u32(row + entries_offset), entry_size, c.u32(row + entries_count));
    }
    if (write(0x1014b548, humans + 8, 4, 1) != 1) {
        fail({0x1014b564, 0x84, 0x44, 0x933});
        close(0x1014b5a8);
        return 0;
    }
    if (write(0x1014b5c4, humans + 0xc, 4, 1) != 1) {
        fail({0x1014b5e0, 0x64, 0x3c, 0x93b});
        close(0x1014b624);
        return 0;
    }
    // The last three slots; the original counts in r28 and steps the offset in r27 this time.
    human = 0;
    offset = 0;
    for (; regs.hold({{28, human}, {27, offset}}), std::int32_t(human) < human_count(); ++human, offset += record::size)
        write(0x1014b654, records() + offset + last_slots_at, record::tactic_size, 3);
    close(0x1014b674);
    return 1;
}
}
