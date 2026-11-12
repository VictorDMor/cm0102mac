// DISCIPLINE.CPP lifecycle block (discipline_lifecycle.hpp): the manager's file, discipline.dat, read from
// and written to the career file through the file layer (0x107475b0 and the routines beside it), native-first.
#include "discipline_lifecycle.hpp"
#include "discipline_lifecycle_detail.hpp"

namespace cm::aot::discipline {
using guest::Cpu;
using guest::Frame;
namespace callee = lifecycle_callee;
namespace field = lifecycle_field;

namespace {
constexpr std::uint32_t name_offset = 0x72, handle_offset = 0x3c, count_offset = 0x38;
constexpr std::int32_t person_row_size = 0x2c;
constexpr std::uint32_t history_record_size = 0x18, index_entry_size = 8, pending_item_size = 0x10;

// The open file: the original keeps its handle in the frame (0x3c), passes its address to the close and to the
// bodies, and reads it again for every call.
struct Stream {
    Cpu& c;
    const Frame& frame;
    std::uint32_t handle_at() const { return frame.at(handle_offset); }
    void open(std::uint32_t file, bool for_writing, std::uint32_t back) {
        c.set({{3, file}, {4, c.toc(lifecycle_toc::pool) + name_offset}, {5, for_writing}, {6, 0x16}, {7, 4}});
        c.call(callee::file_open, back);
        c.put32(handle_at(), c.r(3));
    }
    // The count of items the file layer moved (signed).
    std::int32_t transfer(std::uint32_t entry, std::uint32_t at, std::uint32_t size, std::uint32_t count, std::uint32_t back) {
        c.set({{3, at}, {4, size}, {5, count}, {6, c.u32(handle_at())}});
        c.call(entry, back);
        return std::int32_t(c.r(3));
    }
    std::int32_t read(std::uint32_t at, std::uint32_t size, std::uint32_t count, std::uint32_t back) {
        return transfer(callee::file_read, at, size, count, back);
    }
    std::int32_t write(std::uint32_t at, std::uint32_t size, std::uint32_t count, std::uint32_t back) {
        return transfer(callee::file_write, at, size, count, back);
    }
    void close(std::uint32_t back) {
        c.set(3, handle_at());
        c.call(callee::file_close, back);
    }
};
// The halfword at `at` as the 32-bit value `lha` leaves.
std::uint32_t halfword(const Cpu& c, std::uint32_t at) { return std::uint32_t(std::int32_t(c.i16(at))); }

constexpr CheckSite load_no_file{0x100de7a0, 0x210, 0x98, 0xd9}, load_no_open{0x100de81c, 0x1f0, 0x90, 0xe1},
    load_no_table{0x100de890, 0x1d0, 0x88, 0xe9}, load_table{0x100de90c, 0x1b0, 0x80, 0xf1},
    load_count{0x100de994, 0x190, 0x78, 0xfc}, load_no_records{0x100dea20, 0x170, 0x70, 0x109},
    load_records{0x100dea9c, 0x150, 0x68, 0x111}, load_index_count{0x100deb18, 0x130, 0x60, 0x11a},
    load_no_index{0x100deba4, 0x110, 0x58, 0x127}, load_index{0x100dec20, 0xf0, 0x50, 0x12f},
    load_pending_count{0x100dec9c, 0xd0, 0x48, 0x138}, load_item{0x100ded14, 0xb0, 0x40, 0x142};

constexpr CheckSite save_no_file{0x100dede0, 0x188, 0x80, 0x162}, save_no_open{0x100dee58, 0x168, 0x78, 0x16a},
    save_table{0x100deec8, 0x148, 0x70, 0x171}, save_count{0x100def78, 0x128, 0x68, 0x184},
    save_records{0x100deffc, 0x108, 0x60, 0x18e}, save_index_count{0x100df074, 0xe8, 0x58, 0x197},
    save_index{0x100df0f8, 0xc8, 0x50, 0x1a1}, save_pending_count{0x100df178, 0xa8, 0x48, 0x1ab},
    save_item{0x100df1f0, 0x88, 0x40, 0x1b6};
}

bool load_manager(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x100de760, 0x250, {.first_gpr = 27});
    // r27-r31 hold what the original keeps in them: the file layer's generated routines save the caller's
    // non-volatile registers and reach the file imports with them (the strict replay of same-save-continue
    // compares them there), and their frames are stack a later BlockMoveData may copy.
    c.set({{27, c.toc(lifecycle_toc::people_count)}, {28, manager}, {29, c.toc(-0x7bac)}, {30, c.toc(lifecycle_toc::pool)},
           {31, c.toc(-0x7ba8)}});
    Stream stream{c, frame};
    if (file == 0) {
        report_check(c, frame, load_no_file);
        return false;
    }
    stream.open(file, false, 0x100de7fc);
    if (c.u32(stream.handle_at()) == 0) {
        report_check(c, frame, load_no_open);
        return false;
    }
    // Reports the failure, closes the file and gives the load's answer.
    const auto fail = [&](const CheckSite& site, std::uint32_t closing_call) {
        report_check(c, frame, site);
        stream.close(closing_call);
        return false;
    };
    const auto people = c.toc(lifecycle_toc::people_count);  // the address of the people count
    c.set(3, c.u32(people) * std::uint32_t(person_row_size));
    c.call(callee::allocate, 0x100de86c);
    c.put32(manager + manager::people, c.r(3));
    if (c.u32(manager + manager::people) == 0) return fail(load_no_table, 0x100de8d4);
    const auto rows = stream.read(c.u32(manager + manager::people), person_row_size, c.u32(people), 0x100de8ec);
    if (c.i32(people) != rows) return fail(load_table, 0x100de950);
    c.link(0x100de964);
    create_bodies(c, manager, stream.handle_at());
    if (stream.read(manager + field::record_count, 2, 1, 0x100de978) != 1) return fail(load_count, 0x100de9d8);
    if (c.i16(manager + field::record_count) != 0) {
        c.put16(manager + field::record_capacity, c.u16(manager + field::record_count));
        c.set(3, halfword(c, manager + field::record_capacity) * history_record_size);
        c.call(callee::allocate, 0x100de9fc);
        c.put32(manager + field::records, c.r(3));
        if (c.u32(manager + field::records) == 0) return fail(load_no_records, 0x100dea64);
        const auto read = stream.read(c.u32(manager + field::records), history_record_size,
                                      halfword(c, manager + field::record_count), 0x100dea7c);
        if (c.i16(manager + field::record_count) != read) return fail(load_records, 0x100deae0);
    }
    if (stream.read(manager + field::index_count, 2, 1, 0x100deafc) != 1) return fail(load_index_count, 0x100deb5c);
    if (c.i16(manager + field::index_count) != 0) {
        c.put16(manager + field::index_capacity, c.u16(manager + field::index_count));
        c.set(3, halfword(c, manager + field::index_capacity) << 3);
        c.call(callee::allocate, 0x100deb80);
        c.put32(manager + field::index, c.r(3));
        if (c.u32(manager + field::index) == 0) return fail(load_no_index, 0x100debe8);
        const auto read = stream.read(c.u32(manager + field::index), index_entry_size,
                                      halfword(c, manager + field::index_count), 0x100dec00);
        if (c.i16(manager + field::index_count) != read) return fail(load_index, 0x100dec64);
    }
    // The pending list: a count, then that many items; a short read reports and carries on.
    if (stream.read(frame.at(count_offset), 2, 1, 0x100dec80) != 1) {
        report_check(c, frame, load_pending_count);
        c.put16(frame.at(count_offset), 0);
    }
    std::int16_t item = 0;
    for (; item < c.i16(frame.at(count_offset)); ++item) {
        c.set(27, std::uint32_t(std::int32_t(item)));  // the original's item counter
        if (stream.read(frame.at(0xa0), pending_item_size, 1, 0x100decf8) == 1) {
            c.set({{3, manager + manager::pending}, {4, frame.at(0xa0)}, {5, pending_item_size}});
            c.call(callee::list_append, 0x100ded64);
        } else {
            report_check(c, frame, load_item);
        }
    }
    c.set(27, std::uint32_t(std::int32_t(item)));
    stream.close(0x100ded80);
    return true;
}

void save_manager(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x100deda0, 0x1c0, {.first_gpr = 26});
    // r26-r31 hold what the original keeps in them: the file layer's generated routines save the caller's
    // non-volatile registers and reach the file imports with them (the strict replay of same-save-continue
    // compares them there), and their frames are stack a later BlockMoveData may copy.
    c.set({{27, c.toc(lifecycle_toc::people_count)}, {28, manager}, {29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)},
           {31, c.toc(lifecycle_toc::pool)}});
    Stream stream{c, frame};
    if (file == 0) return report_check(c, frame, save_no_file);
    stream.open(file, true, 0x100dee38);
    if (c.u32(stream.handle_at()) == 0) return report_check(c, frame, save_no_open);
    // Reports the failure and closes the file.
    const auto fail = [&](const CheckSite& site, std::uint32_t closing_call) {
        report_check(c, frame, site);
        stream.close(closing_call);
    };
    const auto people = c.toc(lifecycle_toc::people_count);
    const auto rows = stream.write(c.u32(manager + manager::people), person_row_size, c.u32(people), 0x100deea8);
    if (c.i32(people) != rows) return fail(save_table, 0x100def0c);
    for (std::int8_t index = 0; index < manager::body_count; ++index) {
        c.set({{26, std::uint32_t(index)}, {27, std::uint32_t(index) * 4}});  // the original's loop counters
        const auto body = body_object(c, manager, std::uint32_t(index));
        if (body == 0) continue;
        c.set({{3, body}, {4, stream.handle_at()}});
        c.call(callee::body_save, 0x100def34);
    }
    c.set({{26, std::uint32_t(manager::body_count)}, {27, std::uint32_t(manager::body_count) * 4}});
    if (stream.write(manager + field::record_count, 2, 1, 0x100def5c) != 1) return fail(save_count, 0x100defbc);
    if (c.i16(manager + field::record_count) != 0) {
        const auto written = stream.write(c.u32(manager + field::records), history_record_size,
                                          halfword(c, manager + field::record_count), 0x100defdc);
        if (c.i16(manager + field::record_count) != written) return fail(save_records, 0x100df040);
    }
    if (stream.write(manager + field::index_count, 2, 1, 0x100df058) != 1) return fail(save_index_count, 0x100df0b8);
    if (c.i16(manager + field::index_count) != 0) {
        const auto written = stream.write(c.u32(manager + field::index), index_entry_size,
                                          halfword(c, manager + field::index_count), 0x100df0d8);
        if (c.i16(manager + field::index_count) != written) return fail(save_index, 0x100df13c);
    }
    // The pending list: its count, then every item. A failed count write is reported and the items still go.
    c.put16(frame.at(count_offset), c.u16(manager + field::pending_count));
    if (stream.write(frame.at(count_offset), 2, 1, 0x100df15c) != 1) {
        report_check(c, frame, save_pending_count);
        c.put16(frame.at(count_offset), 0);
    }
    c.put32(manager + manager::pending_cursor, c.u32(manager + manager::pending_head));
    for (;;) {
        c.set(3, manager + manager::pending);
        c.call(callee::list_next, 0x100df238);
        const auto item = c.r(3);
        if (item == 0) break;
        if (stream.write(item, pending_item_size, 1, 0x100df1d4) != 1) {
            report_check(c, frame, save_item);
            break;
        }
    }
    stream.close(0x100df248);
}
}
