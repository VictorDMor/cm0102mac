#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;

std::uint32_t type_allows_unattached(Cpu& c, std::uint32_t person) {
    const auto raw_kind = c.u8(person + 0x62);
    const auto kind = std::int8_t(raw_kind);
    c.set(5, raw_kind);
    bool check_attachment = compare(c, kind, 1) == 0;
    if (!check_attachment) {
        const auto relative_kind = std::uint8_t(raw_kind - 2);
        c.set(0, relative_kind);
        check_attachment = compare_logical(c, relative_kind, 1) <= 0;
        if (!check_attachment) check_attachment = compare(c, kind, 4) == 0;
    }
    if (check_attachment) {
        const auto attachment = c.u32(person + 0x24);
        c.set(0, attachment);
        if (nonzero_logical(c, attachment)) return result(c, 0);
    }
    return result(c, 1);
}

std::uint32_t type_is_nonplaying(Cpu& c, std::uint32_t raw_kind) {
    const auto kind = std::int8_t(raw_kind);
    c.set(0, std::uint32_t(std::int32_t(kind)));
    if (compare(c, kind, 15) == 0) return result(c, 1);
    const auto relative_kind = std::uint8_t(raw_kind - 6);
    c.set(0, relative_kind);
    if (compare_logical(c, relative_kind, 7) <= 0) return result(c, 1);
    c.set(0, std::uint32_t(std::int32_t(kind)));
    return result(c, !nonzero(c, kind));
}

std::uint32_t type_is_employed(Cpu& c, std::uint32_t raw_kind) {
    const auto kind = std::int8_t(raw_kind);
    c.set(0, std::uint32_t(std::int32_t(kind)));
    if (compare(c, kind, 11) == 0) return result(c, 0);
    return result(c, nonzero(c, kind));
}

std::uint32_t person_status(Cpu& c, std::uint32_t person) {
    const auto attributes = c.u32(person + 0x14);
    return result(c, c.u8(attributes + 0x45));
}

std::uint32_t copy_three_words(Cpu& c, std::uint32_t destination, std::uint32_t source) {
    // Ordered word copies deliberately preserve overlapping-source behaviour.
    for (unsigned offset = 0; offset < 12; offset += 4) {
        const auto word = c.u32(source + offset);
        c.set(0, word);
        c.put32(destination + offset, word);
    }
    return destination;
}

std::uint32_t copy_date_through(Cpu& c, std::uint32_t output, std::uint32_t destination, std::uint32_t source) {
    const auto year = c.u16(source + 2);
    c.set(0, year);
    c.put16(destination + 2, year);
    const auto day = c.i16(source);
    c.set(0, extended(day));
    c.put16(destination, extended(day));
    const auto flag = c.u8(source + 4);
    c.set(0, flag);
    c.put8(destination + 4, flag);
    const auto copied_day = c.i16(destination);
    c.set(0, extended(copied_day));
    c.put16(output, extended(copied_day));
    const auto copied_year = c.u16(destination + 2);
    c.set(0, copied_year);
    c.put16(output + 2, copied_year);
    const auto copied_flag = c.u8(destination + 4);
    c.set(0, copied_flag);
    c.put8(output + 4, copied_flag);
    return output;
}

std::uint32_t compare_record_keys(Cpu& c, std::uint32_t first, std::uint32_t second) {
    if (!nonzero_logical(c, first)) return result(c, 0xffffffff);
    if (!nonzero_logical(c, second)) return result(c, 1);
    for (const auto offset : {0u, 4u}) {
        const auto left = c.i32(first + offset), right = c.i32(second + offset);
        c.set(5, std::uint32_t(left));
        c.set(0, std::uint32_t(right));
        if (compare(c, left, right) != 0) return result(c, std::uint32_t(left) - std::uint32_t(right));
    }
    const auto right = c.u32(second + 8), left = c.u32(first + 8);
    c.set(4, right);
    c.set(0, left);
    return result(c, left - right);
}

std::uint32_t contract_month(Cpu& c, std::uint32_t person) {
    Frame frame(c, 0x10089e40, 0x50, {.first_gpr = 31});
    Kept<31> saved_person(c, person);
    Program program(c);
    program.call(0x10715880, 0x10089e5c, {frame.at(0x40)});
    program.call(0x101929b0, 0x10089e6c, {saved_person, 0, frame.at(0x40)});
    program.call(0x10717890, 0x10089e7c, {frame.at(0x38), frame.at(0x40), 10});
    return program.call(0x10718360, 0x10089e84, {frame.at(0x38)});
}

void remove_secondary(Cpu& c, std::uint32_t manager_address, std::uint32_t person) {
    const auto offset = c.u32(person) * Contract::size;
    const auto holder = c.toc(slot::contract_index);
    c.set(0, offset);
    c.set(6, holder);
    const auto row = c.u32(holder) + offset;
    c.set(5, row);
    const auto index = c.i32(row + 4);
    c.set(0, std::uint32_t(index));
    if (compare(c, index, -1) == 0) return;
    const auto contract_offset = std::uint32_t(index) * Contract::size;
    const auto records = Manager{c, manager_address}.records().get();
    c.set(0, contract_offset);
    c.set(3, records);
    c.set(5, 0xffffffff);
    c.put32(records + contract_offset, 0xffffffff);
    // The write above may alias a person or index pointer; read both again.
    const auto current_offset = c.u32(person) * Contract::size;
    const auto current_indices = c.u32(holder);
    c.set(0, current_offset);
    c.set(3, current_indices + current_offset);
    c.put32(current_indices + current_offset + 4, 0xffffffff);
}
}
