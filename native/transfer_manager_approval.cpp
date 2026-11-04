#include "transfer_manager_approval.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;

constexpr auto status_table = 0x9a0, approval_array = 0xa6c, approval_count = 0xa70,
               approval_capacity = 0xa74, transfer_pairs = 0xa78;
constexpr auto comparator_slot = -0x4ba8, entry_size = 16, person_size = 0x70;
constexpr std::uint8_t approved = 0x20;
constexpr std::uint32_t removed = 0xffffffff;

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

void sort_approvals(Cpu& c, std::uint32_t manager, std::uint32_t back) {
    call(c, 0x10768300, back, {c.u32(manager + approval_array), c.u32(manager + approval_count),
                               entry_size, c.toc(comparator_slot)});
}

std::uint32_t next_approval(Cpu& c, std::uint32_t manager) {
    const auto offset = c.u32(manager + approval_count) * entry_size;
    const auto row = c.u32(manager + approval_array) + offset;
    c.set({{0, offset}, {3, row}});
    return row;
}
}

void append_regional_approval(Cpu& c, std::uint32_t manager, std::uint32_t person,
                              std::uint32_t nation, std::uint32_t first_year, std::uint32_t sort) {
    Frame frame(c, 0x103ec940, 0x90, {.first_gpr = 23});
    const Kept<23> body(c, c.toc(-0x79e0));
    const Kept<24> title(c, c.toc(-0x79dc));
    const Kept<25> message(c, c.toc(-0x7ba8));
    const Kept<26> pool(c, c.toc(-0x4b9c));
    const Kept<27> transfers(c, manager);
    const Kept<28> applicant(c, person);
    const Kept<29> target_nation(c, nation);
    const Kept<30> start_year(c, first_year);
    const Kept<31> sort_now(c, sort);
    const auto count = c.u32(transfers + approval_count), capacity = c.u32(transfers + approval_capacity);
    c.set({{0, count}, {3, capacity}});
    if (compare(c, count, capacity) == 0) {
        const auto new_capacity = capacity + 30;
        c.put32(transfers + approval_capacity, new_capacity);
        const auto array = c.u32(transfers + approval_array);
        c.set(3, array);
        const bool allocated = nonzero_logical(c, array);
        const auto new_size = c.u32(transfers + approval_capacity) * entry_size;
        c.set(0, new_capacity);
        const auto storage = allocated ? call(c, 0x107751b0, 0x103ec9a4, {array, new_size})
                                       : call(c, 0x10775130, 0x103ec9b8, {new_size});
        c.put32(transfers + approval_array, storage);
        c.set(0, c.u32(transfers + approval_array));
        if (!nonzero_logical(c, c.r(0))) {
            const auto name = frame.at(0x40), extension = frame.at(0x38);
            call(c, 0x10739890, 0x103ec9e0, {pool + 0x37, 0, 0, name, extension});
            const auto version = call(c, 0x10768810, 0x103ec9e4);
            call(c, 0x1077b720, 0x103eca04,
                 {message, pool + 0x4c, version, name, extension, 0x1b1a, pool + 0x19e6});
            call(c, 0x10727890, 0x103eca10, {title, pool + 0xa6});
            call(c, 0x10727890, 0x103eca20, {body, pool + 0xbe, message});
            call(c, 0x10769b60, 0x103eca30, {title, body, 0});
            return;
        }
    }

    const auto id = c.u32(applicant);
    const bool should_sort = nonzero(c, std::int8_t(sort_now));
    const auto array = c.u32(transfers + approval_array);
    c.set({{4, 0}, {6, id}});
    c.put32(array + c.u32(transfers + approval_count) * entry_size, id);
    const auto row = next_approval(c, transfers);
    const auto target = c.u32(target_nation + 0x68);
    c.set(5, target);
    c.put32(row + 0xc, target);
    c.put16(next_approval(c, transfers) + 8, start_year);
    c.put16(next_approval(c, transfers) + 6, 0);
    c.put16(next_approval(c, transfers) + 4, 0);
    c.put32(transfers + approval_count, c.u32(transfers + approval_count) + 1);
    const auto offset = c.u32(applicant) * 10, flags = c.u32(transfers + status_table);
    c.set({{3, offset}, {4, flags}});
    const auto updated = c.u8(flags + offset) | approved;
    c.set(0, updated);
    c.put8(flags + offset, updated);
    if (should_sort) sort_approvals(c, transfers, 0x103ecae0);
}

std::uint32_t find_regional_approval(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    Frame frame(c, 0x103ecb00, 0x50, {});
    c.set(8, manager);
    if (nonzero_logical(c, person)) {
        const auto count = c.toc(-0x7be4), id = c.u32(person);
        c.set({{3, count}, {5, id}});
        if (compare(c, id, c.i32(count)) >= 0) return result(c, 0);
    }
    const auto id = c.u32(person);
    const auto status = c.u32(manager + status_table);
    c.set({{3, status}, {4, id}});
    const auto flag = c.u8(status + id * 10) & approved;
    c.set(0, flag);
    if (!nonzero(c, flag)) return result(c, 0);
    const auto key = frame.at(0x38);
    c.put32(key, id);
    return call(c, 0x10775600, 0x103ecb70, {key, c.u32(manager + approval_array),
                                           c.u32(manager + approval_count), entry_size, c.toc(comparator_slot)});
}

std::uint32_t find_regional_transfer_pair(Cpu& c, std::uint32_t manager, std::uint32_t person,
                                          std::uint32_t destination) {
    Frame frame(c, 0x103ecb80, 0x50, {.first_gpr = 29});
    const Kept<29> transfers(c, manager);
    const Kept<30> applicant(c, person);
    const Kept<31> target(c, destination);
    const auto list = c.u32(transfers + transfer_pairs);
    c.set(3, list);
    if (!nonzero_logical(c, list)) return result(c, 0);
    const auto head = c.u32(list + 4);
    c.set(0, head);
    c.put32(list + 0xc, head);
    while (true) {
        const auto row = call(c, 0x10738e00, 0x103ecbf0, {c.u32(transfers + transfer_pairs)});
        if (!nonzero_logical(c, row)) return result(c, 0);
        const auto id = c.u32(applicant);
        c.set({{4, id}, {0, c.u32(row)}});
        if (compare(c, id, c.r(0)) != 0) continue;
        const auto target_id = c.u32(target);
        c.set({{4, target_id}, {0, c.u32(row + 4)}});
        if (compare(c, target_id, c.r(0)) == 0) return row;
    }
}

void prune_regional_approvals(Cpu& c, std::uint32_t manager) {
    Frame frame(c, 0x103ecc20, 0x40, {.first_gpr = 31});
    const Kept<31> transfers(c, manager);
    const auto nation_table = c.toc(-0x7b98), date = c.toc(-0x7b80), staff_table = c.toc(-0x7be0);
    c.set({{6, nation_table}, {5, date}, {3, staff_table}});
    bool changed = false;
    std::int32_t index = 0;
    std::uint32_t offset = 0;
    for (;; ++index, offset += entry_size) {
        const auto count = c.i32(transfers + approval_count);
        c.set({{4, std::uint32_t(count)}, {7, offset}, {8, std::uint32_t(index)}});
        if (compare(c, index, count) >= 0) break;
        const auto array = c.u32(transfers + approval_array);
        const auto people = c.u32(staff_table);
        const auto person = people + c.u32(array + offset) * person_size;
        const auto status = c.u32(transfers + status_table);
        const auto flag_offset = c.u32(person) * 10;
        const auto flags = status + flag_offset;
        const auto status_byte = c.u8(flags);
        const auto flag = status_byte & approved;
        c.set({{11, status}, {12, person}, {10, flag_offset}, {9, status_byte}});
        if (!nonzero(c, flag)) c.put8(flags, flag);
        const auto row = c.u32(transfers + approval_array) + offset;
        const auto start_year = c.u16(row + 8);
        const auto years_elapsed = std::int32_t(c.u16(date + 2)) - start_year;
        c.set({{10, row}, {9, start_year}});
        if (compare(c, years_elapsed, 4) > 0) {
            const auto nation_index = c.u32(row + 0xc);
            c.put32(person + 0x10, c.u32(nation_table) + nation_index * 0x128);
        } else if (nonzero_logical(c, c.u32(person + 0x14))) {
            continue;
        }
        const auto current_status = c.u32(transfers + status_table), current_offset = c.u32(person) * 10;
        c.set({{9, removed}, {11, current_status}, {10, current_offset}});
        const auto current_flags = current_status + current_offset;
        c.put8(current_flags, c.u8(current_flags) & ~approved);
        c.put32(c.u32(transfers + approval_array) + offset, removed);
        changed = true;
    }
    c.set(0, changed);
    if (!nonzero(c, changed)) return;
    sort_approvals(c, transfers, 0x103ecd40);

    auto remaining = c.u32(transfers + approval_count);
    const auto last = remaining - 1;
    c.s.wr<1>(reg::xer_ca, remaining != 0);
    compare(c, last);
    auto tail = last * entry_size;
    c.set({{3, last}, {4, tail}});
    c.s.wr<4>(reg::ctr, remaining);
    if (std::int32_t(last) < 0) return;
    do {
        const auto array = c.u32(transfers + approval_array);
        const auto id = c.u32(array + tail);
        c.set(3, array);
        if (compare(c, id, -1) != 0) return;
        const auto count = c.u32(transfers + approval_count);
        tail -= entry_size;
        c.set({{3, count}, {4, tail}});
        c.put32(transfers + approval_count, count - 1);
        c.s.wr<4>(reg::ctr, --remaining);
    } while (remaining);
}
}
