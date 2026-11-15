// HUMAN_MANAGER.CPP: the sorted list of 0x20-byte entries each human's record holds (keys, texts).
#include "human_manager_lists.hpp"
#include "human_manager_detail.hpp"
#include "qsort4x.hpp"

namespace cm::aot::human_manager {
namespace {
constexpr std::uint32_t entry_size = 0x20, text_limit = 0x1a;
constexpr std::uint32_t bsearch = 0x10775600, allocate = 0x10775130, reallocate = 0x107751b0, fatal_exit = 0x1076f7d0,
                        text_length = 0x1077dbd0, copy_text = 0x1077dbf0;
constexpr std::int32_t comparator_slot = -0x73f8;  // the cell holding the comparator's vector

// The staff's slot in the table before the range check (the original keeps it in a register that a
// call's callee may save): id less the staff count, plus 16.
// The search the lists share: key in a stack buffer, then the library's bsearch over the record's
// entries. `back` is the address after the original's `bl`; the result is the entry or null.
std::uint32_t search(Cpu& c, const Frame& frame, std::int32_t buffer, std::uint32_t key, std::uint32_t record,
                     std::uint32_t entries, std::uint32_t vector, std::uint32_t back) {
    c.put32(frame.at(buffer), c.u32(key));
    c.put8(frame.at(buffer + 4), 0);
    return invoke(c, bsearch, back, {frame.at(buffer), entries, c.u32(record + record::entry_count), entry_size, vector});
}
// Closes the gap of the found entry: every later entry moves down (0x1e bytes each: the last two
// bytes of an entry are not copied), the last entry gets key -1 and a zero byte at +4, and the
// count drops by one.
void remove_found(Cpu& c, std::uint32_t record, std::uint32_t found) {
    auto index = (found - c.u32(record + record::entries)) >> 5;
    auto to = (found - c.u32(record + record::entries)) & ~0x1fu;
    while (std::int32_t(index) < std::int32_t(c.u32(record + record::entry_count) - 1)) {
        const auto base = c.u32(record + record::entries);
        const auto from = base + ((index + 1) << 5), dst = base + to;
        for (std::uint32_t offset = 0; offset < 0x1c; offset += 4) c.put32(dst + offset, c.u32(from + offset));
        c.put16(dst + 0x1c, std::uint16_t(c.u16(from + 0x1c)));
        to += entry_size;
        ++index;
    }
    const auto count = c.u32(record + record::entry_count);
    c.put8(c.u32(record + record::entries) + (count << 5) - 0x1c, 0);
    c.put32(c.u32(record + record::entries) + (c.u32(record + record::entry_count) << 5) - 0x20, 0xffffffffu);
    c.put32(record + record::entry_count, c.u32(record + record::entry_count) - 1);
}
// The sort of the record's entries by key (0x10768300 through its C++ owner).
void sort_entries(Cpu& c, std::uint32_t record, std::uint32_t vector, std::uint32_t back) {
    const auto rows = c.u32(record + record::entries), count = c.u32(record + record::entry_count);
    c.set({{3, rows}, {4, count}, {5, entry_size}, {6, vector}});  // the sort reads its arguments from r3-r6
    c.link(back);
    qsort4x::sort(c, rows, count, entry_size, vector);
}
}

void remove_key_everywhere(Cpu& c, std::uint32_t humans, std::uint32_t key) {
    const Frame frame(c, 0x1014b690, 0x70, {.first_gpr = 26});
    const auto vector = c.toc(comparator_slot);
    c.set({{26, humans}, {27, key}, {31, vector}});
    if (!key) return;
    c.set({{28, 0}, {30, 0}});
    // The counter is a byte (extsb) compared signed with the table's count.
    while (std::int8_t(c.r(28)) < c.i32(humans + table::count)) {
        c.set(29, c.u32(humans + table::records) + c.r(30));
        if (const auto record = c.r(29)) {
            const auto entries = c.u32(record + record::entries);
            if (!entries) return;  // 0x1014b6d4: leaves from inside the loop, nothing is sorted
            if (const auto found = search(c, frame, 0x38, key, record, entries, vector, 0x1014b6fc))
                remove_found(c, record, found);
        }
        c.set(30, c.r(30) + record::size);
        c.set(28, c.r(28) + 1);
    }
    // r29 is whatever record the loop saw last; when the loop never ran it is the caller's value
    // (0x1014b7dc reads it unset: this is the entry register).
    sort_entries(c, c.r(29), vector, 0x1014b7f4);
}

void add_entry(Cpu& c, std::uint32_t humans, std::uint32_t key, std::uint32_t text, std::uint32_t staff) {
    const Frame frame(c, 0x1014b810, 0x120, {.first_gpr = 25});
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message);
    c.set({{26, key}, {29, c.toc(toc::success)}, {28, humans}, {30, message}, {27, text}, {31, pool}});
    if (!key) return;
    if (!staff) {
        staff = staff_or_current(c, staff);
        if (!staff) {
            report(c, frame, {0x1014b874, 0xe0, 0x58, 0x999});
            return;
        }
    }
    const auto raw = raw_index(c, staff);
    c.set(25, raw);
    if (!inside(c, humans, raw)) {
        report(c, frame, {0x1014b8f0, 0xc0, 0x50, 0x9a5});
        return;
    }
    if (text && invoke(c, text_length, 0x1014b940, {text}) > text_limit) {
        report(c, frame, {0x1014b95c, 0xa0, 0x48, 0x9ad});
        return;
    }
    const auto record = record_at(c, humans, raw);
    c.set(28, record);
    if (!c.u32(record + record::entries)) {
        c.put32(record + record::entries, invoke(c, allocate, 0x1014b9c8, {0x140}));
        if (!c.u32(record + record::entries)) {
            report(c, frame, {0x1014b9f0, 0x80, 0x40, 0x9ba});
            invoke(c, fatal_exit, 0x1014ba34, {0xffffffffu});
        }
        c.put32(record + record::entry_capacity, 10);
    }
    if (c.u32(record + record::entry_capacity) == c.u32(record + record::entry_count)) {
        const auto capacity = c.u32(record + record::entry_capacity);
        c.put32(record + record::entries,
                invoke(c, reallocate, 0x1014ba64, {c.u32(record + record::entries), (capacity + 10) << 5}));
        if (!c.u32(record + record::entries)) {
            report(c, frame, {0x1014ba8c, 0x60, 0x38, 0x9c6});
            invoke(c, fatal_exit, 0x1014bad0, {0xffffffffu});
        }
        c.put32(record + record::entry_capacity, c.u32(record + record::entry_capacity) + 10);
    }
    const auto slot = c.u32(record + record::entry_count) << 5;
    c.put32(c.u32(record + record::entries) + slot, c.u32(key));
    const auto at = c.u32(record + record::entry_count) << 5;
    invoke(c, copy_text, 0x1014bb14, {c.u32(record + record::entries) + at + 4, text});
    c.put32(record + record::entry_count, c.u32(record + record::entry_count) + 1);
    sort_entries(c, record, c.toc(comparator_slot), 0x1014bb38);
}

void remove_entry(Cpu& c, std::uint32_t humans, std::uint32_t key, std::uint32_t staff) {
    const Frame frame(c, 0x1014bb50, 0xc0, {.first_gpr = 28});
    const auto vector = c.toc(comparator_slot);
    c.set({{30, vector}, {31, c.toc(toc::success)}, {28, c.toc(toc::message)}, {29, c.toc(toc::pool)}});
    if (!key) return;
    if (!staff) {
        staff = staff_or_current(c, staff);
        if (!staff) {
            report(c, frame, {0x1014bbbc, 0x68, 0x40, 0x9e8});
            return;
        }
    }
    const auto raw = raw_index(c, staff);
    if (!inside(c, humans, raw)) {
        report(c, frame, {0x1014bc38, 0x48, 0x38, 0x9f4});
        return;
    }
    const auto record = record_at(c, humans, raw);
    c.set(28, record);
    const auto entries = c.u32(record + record::entries);
    if (!entries) return;
    const auto found = search(c, frame, 0x88, key, record, entries, vector, 0x1014bcc4);
    if (!found) return;
    remove_found(c, record, found);
    sort_entries(c, record, vector, 0x1014bd9c);
}

std::uint32_t entry_text(Cpu& c, std::uint32_t humans, std::uint32_t key, std::uint32_t staff) {
    const Frame frame(c, 0x1014bdc0, 0xc0, {.first_gpr = 29});
    c.set({{30, c.toc(toc::message)}, {29, c.toc(toc::success)}, {31, c.toc(toc::pool)}});
    if (!key) return 0;
    if (!staff) {
        staff = staff_or_current(c, staff);
        if (!staff) {
            report(c, frame, {0x1014be2c, 0x68, 0x40, 0xa31});
            return 0;
        }
    }
    const auto raw = raw_index(c, staff);
    if (!inside(c, humans, raw)) {
        report(c, frame, {0x1014beac, 0x48, 0x38, 0xa3d});
        return 0;
    }
    const auto record = record_at(c, humans, raw);
    const auto entries = c.u32(record + record::entries);
    if (!entries) return 0;
    const auto found = search(c, frame, 0x88, key, record, entries, c.toc(comparator_slot), 0x1014bf44);
    return found ? found + 4 : 0;
}

std::int8_t has_entry(Cpu& c, std::uint32_t humans, std::uint32_t key, std::uint32_t staff) {
    // Callers read r5-r10 after this routine (tools/register_contract.py --after 0x1014bf80: 0x1038c518 and
    // 0x10399a80 read r5-r8), and the routine's leftovers have known stale-stack readers: it leaves r4-r8 as
    // the original does on every path (the same-save-continue capture diverged without it).
    const Frame frame(c, 0x1014bf80, 0x90, {.first_gpr = 30});
    const auto game = c.toc(toc::game);
    c.set({{30, c.toc(toc::message)}, {31, c.toc(toc::pool)}, {6, game}});
    if (!key) return 0;
    if (!staff) {
        staff = staff_or_current(c, staff);
        c.set(5, staff);
        if (!staff) return 0;  // 0x1014bfd0: no assertion here
    }
    const auto raw = raw_index(c, staff);
    c.set({{5, raw - std::uint32_t(human_slots)}, {6, raw}});
    if (std::int32_t(raw) >= 0) c.set(5, c.u32(humans + table::count));
    if (!inside(c, humans, raw)) {
        report(c, frame, {0x1014c014, 0x40, 0x38, 0xa7c});
        c.set(4, c.toc(toc::success));
        return 0;
    }
    const auto record = record_at(c, humans, raw);
    const auto entries = c.u32(record + record::entries);
    c.set({{5, entries}, {8, record + 0x10000}});
    if (!entries) return 0;
    return search(c, frame, 0x60, key, record, entries, c.toc(comparator_slot), 0x1014c0b0) != 0 ? 1 : 0;
}

void set_entry_text(Cpu& c, std::uint32_t humans, std::uint32_t key, std::uint32_t text, std::uint32_t staff) {
    const Frame frame(c, 0x1014c0e0, 0xf0, {.first_gpr = 25});
    c.set({{26, humans}, {27, key}, {29, c.toc(toc::success)}, {28, text}, {30, c.toc(toc::message)},
           {31, c.toc(toc::pool)}});
    if (!key) return;
    if (!staff) {
        staff = staff_or_current(c, staff);
        if (!staff) {
            report(c, frame, {0x1014c144, 0x90, 0x48, 0xab0});
            return;
        }
    }
    const auto raw = raw_index(c, staff);
    c.set(25, raw);
    if (!inside(c, humans, raw)) {
        report(c, frame, {0x1014c1c0, 0x70, 0x40, 0xabc});
        return;
    }
    if (text && invoke(c, text_length, 0x1014c210, {text}) > text_limit) {
        report(c, frame, {0x1014c22c, 0x50, 0x38, 0xac4});
        return;
    }
    const auto record = record_at(c, humans, raw);
    const auto entries = c.u32(record + record::entries);
    if (!entries) return;
    const auto found = search(c, frame, 0xb0, key, record, entries, c.toc(comparator_slot), 0x1014c2b4);
    if (found) invoke(c, copy_text, 0x1014c2c8, {found + 4, text});
}

// 0x1014d520: a frameless leaf. A null row sorts first (-1), a null second row last (1); otherwise
// the difference of the keys as the original computes it (wrapping, and zero when equal), which is
// not an ordering of the keys' signed or unsigned values when they are far apart.
std::uint32_t compare_entries(Cpu& c, std::uint32_t first, std::uint32_t second) {
    if (!first) return 0xffffffffu;
    if (!second) return 1;
    return c.u32(first) - c.u32(second);
}
}
