#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;

std::uint32_t reserve_contract(Cpu& c, std::uint32_t manager_address, std::uint32_t append_only) {
    Frame frame(c, 0x10078f00, 0x80, {.first_gpr = 27});
    Kept<27> message(c, c.toc(-0x79e0));
    Kept<31> manager_pointer(c, manager_address);
    Kept<28> title(c, c.toc(-0x79dc));
    Kept<29> error_buffer(c, c.toc(-0x7ba8));
    Kept<30> pool(c, c.toc(slot::pool));
    Program program(c);
    const Manager manager{c, manager_pointer};
    const auto mode = std::int8_t(append_only);
    c.set(0, std::uint32_t(std::int32_t(mode)));
    if (!nonzero(c, mode)) {
        auto record = manager.records().get();
        c.set(4, record);
        std::uint32_t index = 0;
        for (;;) {
            c.set(3, index);
            const auto count = manager.count().get();
            c.set(0, std::uint32_t(count));
            if (compare(c, std::int32_t(index), count) >= 0) break;
            const auto person_id = Contract{c, record}.person_id().get();
            c.set(0, std::uint32_t(person_id));
            if (compare(c, person_id, -1) == 0) return result(c, index);
            ++index;
            record += Contract::size;
            c.set(4, record);
        }
    }
    const auto count = manager.count().get(), capacity = manager.capacity().get();
    c.set(0, std::uint32_t(count));
    c.set(3, std::uint32_t(capacity));
    if (compare(c, count, capacity) == 0) {
        const auto enlarged = std::uint32_t(capacity) + 200;
        c.set(0, enlarged);
        manager.capacity() = std::int32_t(enlarged);
        const auto old_records = manager.records().get();
        c.set(3, old_records);
        if (nonzero_logical(c, old_records)) {
            const auto new_capacity = std::uint32_t(manager.capacity().get());
            const auto bytes = new_capacity * Contract::size;
            c.set(0, new_capacity);
            manager.records() = program.call(0x107751b0, 0x10078f8c, {old_records, bytes});
        } else {
            const auto new_capacity = std::uint32_t(manager.capacity().get());
            c.set(0, new_capacity);
            manager.records() = program.call(0x10775130, 0x10078fa0, {new_capacity * Contract::size});
        }
        const auto records = manager.records().get();
        c.set(0, records);
        if (!nonzero_logical(c, records)) {
            // Preserve the original allocation-failure presentation and its
            // return of -1, including the varargs stack/register inputs.
            program.call(0x10739890, 0x10078fc8, {pool + 0x37, 0, 0, frame.at(0x40), frame.at(0x38)});
            const auto version = program.call(0x10768810, 0x10078fcc);
            program.call(0x1077b720, 0x10078fec,
                         {error_buffer, pool + 0x4c, version, frame.at(0x40), frame.at(0x38), 0x172, pool + 0x1e0});
            program.call(0x10727890, 0x10078ff8, {title, pool + 0xb8});
            program.call(0x10727890, 0x10079008, {message, pool + 0xd0, error_buffer});
            program.call(0x10769b60, 0x10079018, {title, message, 0});
            return result(c, 0xffffffff);
        }
    }
    const auto next = std::uint32_t(manager.count().get()) + 1;
    c.set(0, next);
    manager.count() = std::int32_t(next);
    // A fresh read also preserves unusual aliasing with the count field.
    return result(c, std::uint32_t(manager.count().get()) - 1);
}

std::uint32_t bind_primary(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address, std::uint32_t initial_value) {
    Frame frame(c, 0x10078e70, 0x50, {.first_gpr = 29});
    Kept<31> value(c, initial_value);
    Kept<30> person(c, person_address);
    Kept<29> manager(c, manager_address);
    Program program(c);
    const auto index = program.call(0x10078f00, 0x10078e9c, {manager, 1});
    if (compare(c, std::int32_t(index)) >= 0) {
        const auto person_id = c.u32(person);
        const auto contract_offset = index * Contract::size;
        c.set(0, person_id * Contract::size);
        c.set(7, contract_offset);
        const auto index_holder = c.toc(slot::contract_index);
        c.set(6, index_holder);
        leave(c, {{4, person}, {5, value}});
        const auto indices = c.u32(index_holder);
        c.set(6, indices);
        c.put32(indices + person_id * Contract::size, index);
        const auto current_person_id = c.u32(person);
        const auto records = Manager{c, manager}.records().get();
        c.set(0, current_person_id);
        c.set(3, records);
        c.put32(records + contract_offset, current_person_id);
        const auto current_records = Manager{c, manager}.records().get();
        c.set(0, current_records);
        return program.call(0x103727c0, 0x10078edc, {current_records + contract_offset});
    }
    return c.r(3);
}
}
