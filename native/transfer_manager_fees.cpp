#include "transfer_manager_fees.hpp"
#include "guest_values.hpp"
#include <array>

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;

constexpr std::uint32_t record_size = 16, staff_status = 0x9a0, club_size = 0x248;
constexpr auto pool_slot = -0x4b9c, message_slot = -0x7ba8, title_slot = -0x79dc, body_slot = -0x79e0;
constexpr std::uint32_t sort_rows = 0x10768300, search_rows = 0x10775600, primary_lookup = 0x10082f20;
struct FeeTable {
    std::uint32_t offset, status_bit, key_offset, byte_offset;
    std::uint32_t append_entry, lookup_entry, allocation_line, allocation_label;
    std::int32_t comparator_slot;
};
constexpr std::array fee_tables{
    FeeTable{0x9b4, 8, 0xc, 0, 0x103f1e60, 0x103fb400, 0x2552, 0x1a3c, -0x4bb0},
    FeeTable{0x9c0, 2, 0, 8, 0x103f1fb0, 0x103fb510, 0x2580, 0x1a7f, -0x4bb4},
    FeeTable{0x9cc, 4, 0, 8, 0x103f2100, 0x103fb5a0, 0x25ae, 0x1ac9, -0x4bb4},
};

struct FeeRegistry {
    Cpu& c;
    std::uint32_t manager;
    const FeeTable& table;
    Field<std::uint32_t> rows() const { return {c, manager + table.offset}; }
    Field<std::uint32_t> capacity() const { return {c, manager + table.offset + 4}; }
    Field<std::uint32_t> count() const { return {c, manager + table.offset + 8}; }
};

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

void copy_fee(Cpu& c, const FeeTable& table, std::uint32_t destination, std::uint32_t source) {
    for (std::uint32_t offset = 0; offset < record_size; offset += 4) {
        if (offset == table.byte_offset) {
            const auto value = c.u8(source + offset);
            c.set(0, value);
            c.put8(destination + offset, value);
        } else {
            const auto value = c.u32(source + offset);
            c.set(0, value);
            c.put32(destination + offset, value);
        }
    }
}

std::uint32_t search(Cpu& c, const FeeRegistry& registry, std::uint32_t key, std::uint32_t back) {
    return call(c, search_rows, back, {key, registry.rows(), registry.count(), record_size,
                                      c.toc(registry.table.comparator_slot)});
}

std::uint32_t lookup_employer(Cpu& c, std::uint32_t person, std::uint32_t ledger,
                              std::uint32_t cache, std::uint32_t clubs, std::uint32_t back) {
    const auto contract = call(c, primary_lookup, back, {c.u32(ledger), person});
    const bool found = nonzero_logical(c, contract);
    c.put32(cache, contract);
    if (!found) return 0;
    const auto offset = c.u32(contract + 4) * club_size;
    const auto base = c.u32(clubs);
    c.set({{0, offset}, {3, base}});
    return base + offset;
}

void forget_fee_flag(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t bit) {
    const auto offset = c.u32(person) * 10;
    const auto flags = c.u32(manager + staff_status);
    c.set({{4, offset}, {5, flags}});
    c.put8(flags + offset, c.u8(flags + offset) - bit);
}
}

void append_transfer_fee(Cpu& c, FeeKind kind, std::uint32_t manager, std::uint32_t record) {
    const auto& table = fee_tables[unsigned(kind)];
    const auto entry = table.append_entry;
    Frame frame(c, entry, 0x80, {.first_gpr = 26});
    const Kept<26> body(c, c.toc(body_slot));
    const Kept<27> title(c, c.toc(title_slot));
    const Kept<28> message(c, c.toc(message_slot));
    const Kept<29> pool(c, c.toc(pool_slot));
    const Kept<30> transfers(c, manager);
    const Kept<31> fee(c, record);
    const FeeRegistry registry{c, manager, table};
    const auto count = registry.count().get(), capacity = registry.capacity().get();
    c.set({{0, count}, {3, capacity}});
    if (compare(c, count, capacity) == 0) {
        registry.capacity() = capacity + 50;
        const auto old = registry.rows().get();
        c.set(3, old);
        const bool has_array = nonzero_logical(c, old);
        const auto next_capacity = registry.capacity().get();
        const auto size = next_capacity * record_size;
        c.set(0, next_capacity);
        const auto array = has_array ? call(c, 0x107751b0, entry + 0x6c, {old, size})
                                     : call(c, 0x10775130, entry + 0x58, {size});
        registry.rows() = array;
        c.set(0, registry.rows());
        if (!nonzero_logical(c, c.r(0))) {
            const auto name = frame.at(0x40), extension = frame.at(0x38);
            call(c, 0x10739890, entry + 0x94, {pool + 0x37, 0, 0, name, extension});
            const auto version = call(c, 0x10768810, entry + 0x98);
            call(c, 0x1077b720, entry + 0xb8,
                 {message, pool + 0x4c, version, name, extension, table.allocation_line, pool + table.allocation_label});
            call(c, 0x10727890, entry + 0xc4, {title, pool + 0xa6});
            call(c, 0x10727890, entry + 0xd4, {body, pool + 0xbe, message});
            call(c, 0x10769b60, entry + 0xe4, {title, body, 0});
            return;
        }
    }
    const auto length = registry.count().get(), rows = registry.rows().get();
    registry.count() = length + 1;
    const auto comparator = c.toc(table.comparator_slot);
    copy_fee(c, table, rows + length * record_size, fee);
    call(c, sort_rows, entry + 0x134, {registry.rows(), registry.count(), record_size, comparator});
}

void remove_transfer_fee(Cpu& c, FeeKind kind, std::uint32_t manager, std::uint32_t person) {
    const auto& table = fee_tables[unsigned(kind)];
    const FeeRegistry registry{c, manager, table};
    const auto status_offset = c.u32(person) * 10;
    const auto status = c.u32(manager + staff_status);
    const auto flags = c.u8(status + status_offset);
    c.set(5, flags);
    if (nonzero(c, flags & table.status_bit)) c.put8(status + status_offset, flags - table.status_bit);
    auto row = registry.rows().get();
    for (std::int32_t index = 0; ; ++index, row += record_size) {
        const auto count = registry.count().get();
        c.set({{0, count}, {6, std::uint32_t(index)}, {7, row}});
        if (compare(c, index, count) >= 0) return;
        const auto key = c.u32(row + table.key_offset), id = c.u32(person);
        c.set({{0, id}, {5, key}});
        if (compare(c, key, id) != 0) continue;
        auto offset = std::uint32_t(++index) * record_size;
        for (;; ++index, offset += record_size, row += record_size) {
            const auto remaining = registry.count().get();
            c.set({{4, remaining}, {5, offset}, {6, std::uint32_t(index)}, {7, row}});
            if (compare(c, index, remaining) >= 0) {
                c.set(0, remaining - 1);
                registry.count() = remaining - 1;
                return;
            }
            copy_fee(c, table, row, registry.rows().get() + offset);
        }
    }
}

std::uint32_t find_sell_on_fee(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    constexpr auto& table = fee_tables[0];
    Frame frame(c, table.lookup_entry, 0x60, {.first_gpr = 27});
    const Kept<27> ledger(c, c.toc(-0x7b7c));
    const Kept<28> cache(c, c.toc(-0x7b78));
    const Kept<29> clubs(c, c.toc(-0x7be8));
    const Kept<30> transfers(c, manager);
    const Kept<31> staff(c, person);
    const auto flags = c.u32(transfers + staff_status);
    c.set(3, flags);
    const auto bit = c.u8(flags + c.u32(staff) * 10) & table.status_bit;
    c.set(0, bit);
    if (!nonzero(c, bit)) return result(c, 0);
    const auto employer = lookup_employer(c, staff, ledger, cache, clubs, 0x103fb444);
    c.set(0, employer);
    if (!nonzero_logical(c, employer)) return result(c, 0);
    const auto key = frame.at(0x38);
    const auto id = c.u32(staff);
    c.set(0, id);
    c.put32(key + 0xc, id);
    const auto club = lookup_employer(c, staff, ledger, cache, clubs, 0x103fb484);
    c.set(3, club);
    const auto club_id = c.u32(club);
    c.set(0, club_id);
    c.put32(key + 4, club_id);
    const auto row = search(c, FeeRegistry{c, manager, table}, key, 0x103fb4c8);
    if (nonzero_logical(c, row)) return row;
    forget_fee_flag(c, transfers, staff, table.status_bit);
    return result(c, 0);
}

std::uint32_t find_appearance_fee(Cpu& c, FeeKind kind, std::uint32_t manager, std::uint32_t person) {
    const auto& table = fee_tables[unsigned(kind)];
    Frame frame(c, table.lookup_entry, 0x50, {.first_gpr = 30});
    const Kept<30> transfers(c, manager);
    const Kept<31> staff(c, person);
    const auto id = c.u32(staff), flags = c.u32(transfers + staff_status);
    c.set({{3, flags}, {4, id}});
    const auto bit = c.u8(flags + id * 10) & table.status_bit;
    c.set(0, bit);
    if (!nonzero(c, bit)) return result(c, 0);
    const auto key = frame.at(0x38);
    c.put32(key, id);
    const auto row = search(c, FeeRegistry{c, manager, table}, key, table.lookup_entry + 0x50);
    if (nonzero_logical(c, row)) return row;
    forget_fee_flag(c, transfers, staff, table.status_bit);
    return result(c, 0);
}
}
