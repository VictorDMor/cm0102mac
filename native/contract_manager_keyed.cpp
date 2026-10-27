#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;

std::uint32_t find_keyed_record(Cpu& c, std::uint32_t manager_address, std::uint32_t subject_address,
                                std::uint32_t target_address, std::uint32_t club_address) {
    Frame frame(c, 0x1007bf20, 0x60, {.first_gpr = 28});
    Kept<31> club(c, club_address);
    Kept<30> target(c, target_address);
    Kept<29> subject(c, subject_address);
    Kept<28> manager(c, manager_address);
    Program program(c);
    program.call(0x10715880, 0x1007bf54, {frame.at(0x44)});
    const KeyedRecords rows{c, manager};
    const auto data = rows.data().get();
    c.set(4, data);
    if (!nonzero_logical(c, data)) return result(c, 0);
    const auto subject_id = c.u32(subject);
    c.set(0, subject_id);
    c.set(3, frame.at(0x38));
    const auto comparator = c.toc(-0x5720);
    c.set(7, comparator);
    c.set(6, KeyedRecords::size);
    c.put32(frame.at(0x38), subject_id);
    const auto club_id = c.u32(club);
    c.set(0, club_id);
    c.put32(frame.at(0x3c), club_id);
    const auto target_id = c.u32(target);
    c.set(0, target_id);
    c.put32(frame.at(0x40), target_id);
    return program.call(0x10775600, 0x1007bf94,
                        {frame.at(0x38), data, std::uint32_t(rows.count().get()), KeyedRecords::size, comparator});
}

void add_keyed_record(Cpu& c, std::uint32_t manager_address, std::uint32_t subject_address,
                       std::uint32_t target_address, std::uint32_t club_address, std::uint32_t kind_value) {
    Frame frame(c, 0x1007bb70, 0xb0, {.first_gpr = 23});
    Kept<28> success_flag(c, c.toc(-0x7bac));
    Kept<23> manager(c, manager_address);
    Kept<29> error_buffer(c, c.toc(-0x7ba8));
    Kept<24> subject(c, subject_address);
    Kept<30> today(c, c.toc(-0x7b80));
    Kept<25> target(c, target_address);
    Kept<31> pool(c, c.toc(slot::pool));
    Kept<26> club(c, club_address);
    Kept<27> kind(c, kind_value);
    Program program(c);
    if (nonzero_logical(c, program.call(0x1007bf20, 0x1007bba8))) return;
    const KeyedRecords rows{c, manager};

    auto allocation_error = [&](std::uint32_t first_call, unsigned name, unsigned extension,
                                unsigned line, std::uint32_t exit_back) {
        program.call(0x10739890, first_call + 4, {pool + 0x37, 0, 0, frame.at(name), frame.at(extension)});
        const auto version = program.call(0x10768810, first_call + 8);
        program.call(0x1077b720, first_call + 0x24,
                     {error_buffer, pool + 0x23b, version, frame.at(name), frame.at(extension), line});
        program.call(0x10769b60, first_call + 0x34, {pool + 0x248, error_buffer, 0});
        c.set(0, 0);
        c.put8(success_flag, 0);
        program.call(0x1076f7d0, exit_back, {0xffffffff});
    };
    const auto old_data = rows.data().get();
    c.set(0, old_data);
    if (!nonzero_logical(c, old_data)) {
        rows.data() = program.call(0x10775130, 0x1007bbc4, {20 * KeyedRecords::size});
        const auto allocated = rows.data().get();
        c.set(0, allocated);
        if (!nonzero_logical(c, allocated)) allocation_error(0x1007bbe8, 0x68, 0x40, 0x5a6, 0x1007bc2c);
        c.set(0, 20);
        rows.capacity() = 20;
    }
    const auto capacity = rows.capacity().get(), count = rows.count().get();
    c.set(3, std::uint32_t(capacity));
    c.set(0, std::uint32_t(count));
    if (compare(c, capacity, count) == 0) {
        const auto enlarged = std::uint32_t(capacity) + 20;
        c.set(0, enlarged);
        const auto data = rows.data().get();
        rows.data() = program.call(0x107751b0, 0x1007bc54, {data, enlarged * KeyedRecords::size});
        const auto allocated = rows.data().get();
        c.set(0, allocated);
        if (!nonzero_logical(c, allocated)) allocation_error(0x1007bc78, 0x48, 0x38, 0x5b1, 0x1007bcbc);
        const auto current_capacity = rows.capacity().get();
        c.set(3, std::uint32_t(current_capacity));
        c.set(0, std::uint32_t(current_capacity) + 20);
        rows.capacity() = std::int32_t(std::uint32_t(current_capacity) + 20);
    }
    const auto subject_id = c.u32(subject);
    const auto comparator = c.toc(-0x5720);
    c.set(5, KeyedRecords::size);
    c.set(6, comparator);
    c.set(4, subject_id);
    c.put32(rows.append_address(), subject_id);
    const auto club_id = c.u32(club);
    c.set(4, club_id);
    c.put32(rows.append_address() + 4, club_id);
    const auto date = rows.append_address() + 12;
    c.set(3, date);
    const auto year = c.u16(today + 2);
    c.set(0, year);
    c.put16(date + 2, year);
    const auto day = c.i16(today);
    c.set(0, extended(day));
    c.put16(date, extended(day));
    const auto flag = c.u8(today + 4);
    c.set(0, flag);
    c.put8(date + 4, flag);
    const auto kind_row = rows.append_address();
    c.set(3, kind_row);
    c.put8(kind_row + 20, kind);
    const auto target_row = rows.append_address();
    c.set(3, target_row);
    const auto target_id = c.u32(target);
    c.set(4, target_id);
    c.put32(target_row + 8, target_id);
    const auto before_increment = rows.count().get();
    c.set(3, std::uint32_t(before_increment));
    c.set(0, std::uint32_t(before_increment) + 1);
    rows.count() = std::int32_t(std::uint32_t(before_increment) + 1);
    program.call(0x10768300, 0x1007bd6c,
                 {rows.data().get(), std::uint32_t(rows.count().get()), KeyedRecords::size, comparator});
}

void remove_keyed_records(Cpu& c, std::uint32_t manager_address, std::uint32_t subject_address,
                           std::uint32_t target_address, std::uint32_t club_address) {
    Frame frame(c, 0x1007bd80, 0x50, {.first_gpr = 27});
    Kept<27> manager(c, manager_address);
    Kept<28> subject(c, subject_address);
    Kept<29> target(c, target_address);
    Kept<30> club(c, club_address);
    Kept<31> index(c, 0);
    const KeyedRecords rows{c, manager};
    Program program(c);
    std::int32_t final_count = 0;
    for (;;) {
        const auto count = rows.count().get();
        final_count = count;
        c.set(4, std::uint32_t(count));
        if (compare(c, std::int32_t(std::uint32_t(index)), count) >= 0) break;
        const auto data = rows.data().get();
        const auto row = data + std::uint32_t(index) * KeyedRecords::size;
        c.set(0, data);
        c.set(3, row);
        const auto subject_id = c.i32(subject), row_subject = c.i32(row);
        c.set(6, std::uint32_t(subject_id));
        c.set(5, std::uint32_t(row_subject));
        if (compare(c, subject_id, row_subject) == 0) {
            auto remove = [&](std::uint32_t back) {
                const auto last = data + (std::uint32_t(count) - 1) * KeyedRecords::size;
                program.call(0x10739ef0, back, {row, last, KeyedRecords::size});
                const auto current_count = rows.count().get();
                c.set(3, std::uint32_t(current_count));
                index = std::uint32_t(index) - 1;
                const auto remaining = std::uint32_t(current_count) - 1;
                c.set(0, remaining);
                rows.count() = std::int32_t(remaining);
            };
            auto matches = [&](std::uint32_t object, unsigned offset) {
                const auto id = c.i32(object), recorded = c.i32(row + offset);
                c.set(6, std::uint32_t(id));
                c.set(5, std::uint32_t(recorded));
                return compare(c, id, recorded) == 0;
            };
            bool removed = false;
            if (!nonzero_logical(c, target) && !nonzero_logical(c, club)) {
                remove(0x1007bde8); removed = true;
            }
            if (!removed && !nonzero_logical(c, target) && nonzero_logical(c, club) && matches(club, 4)) {
                remove(0x1007be30); removed = true;
            }
            if (!removed && !nonzero_logical(c, club) && nonzero_logical(c, target) && matches(target, 8)) {
                remove(0x1007be78); removed = true;
            }
            if (!removed && nonzero_logical(c, club) && matches(club, 4) &&
                nonzero_logical(c, target) && matches(target, 8)) remove(0x1007bed0);
        }
        index = std::uint32_t(index) + 1;
    }
    if (nonzero(c, final_count))
        program.call(0x10768300, 0x1007bf08,
                     {rows.data().get(), std::uint32_t(final_count), KeyedRecords::size, c.toc(-0x5720)});
}
}
