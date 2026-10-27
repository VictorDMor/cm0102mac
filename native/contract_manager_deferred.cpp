#include "contract_manager_detail.hpp"
#include <utility>

namespace cm::aot::contract_manager {
using namespace detail;

std::uint32_t collect_club_pending(Cpu& c, std::uint32_t manager_address, std::uint32_t club_address,
                                    std::uint32_t output_address) {
    Frame frame(c, 0x1007bfc0, 0x90, {.first_gpr = 21});
    Kept<21> object(c, manager_address); Kept<28> message(c, c.toc(-0x79e0));
    Kept<29> title(c, c.toc(-0x79dc)); Kept<22> club(c, club_address);
    Kept<30> error_buffer(c, c.toc(-0x7ba8)); Kept<23> output(c, output_address);
    Kept<31> pool(c, c.toc(slot::pool)); Kept<27> count(c, 0);
    Kept<26> capacity(c, 0); Kept<24> buffer(c, 0); Kept<25> record(c, c.r(25));
    Program program(c);
    const auto list = c.u32(object + 0x34); c.set(3, list);
    if (nonzero_logical(c, list)) {
        const auto first = c.u32(list + 4); c.set(0, first); c.put32(list + 12, first);
        for (;;) {
            record = program.call(0x10738e00, 0x1007c110, {c.u32(object + 0x34)});
            if (!nonzero(c, std::int32_t(std::uint32_t(record)))) break;
            const auto recorded_club = c.i32(record + 4), wanted = c.i32(club);
            c.set(3, std::uint32_t(recorded_club)); c.set(0, std::uint32_t(wanted));
            if (compare(c, recorded_club, wanted) != 0) continue;
            // Packed role is a signed six-bit value (the high six bits).
            const auto packed = c.u8(record + 0x38);
            const auto role = std::int32_t(std::int8_t(packed)) >> 2;
            c.set(0, std::uint32_t(role)); c.s.wr<1>(guest::reg::xer_ca, role < 0 && (packed & 2));
            if (compare(c, role, 5) == 0 || compare(c, role, 6) == 0) continue;
            const auto signed_capacity = std::int8_t(std::uint32_t(capacity));
            const auto signed_count = std::int8_t(std::uint32_t(count));
            c.set(3, std::uint32_t(std::int32_t(signed_capacity))); c.set(0, std::uint32_t(std::int32_t(signed_count)));
            if (compare(c, signed_capacity, signed_count) == 0) {
                capacity = std::uint32_t(capacity) + 20;
                const auto enlarged = std::int8_t(std::uint32_t(capacity)); c.set(0, std::uint32_t(std::int32_t(enlarged)));
                buffer = program.call(0x107751b0, 0x1007c064,
                                      {buffer, std::uint32_t(std::int32_t(enlarged)) * 136});
                // The original checks the list, not the reallocated buffer.
                const auto current_list = c.u32(object + 0x34); c.set(0, current_list);
                if (!nonzero_logical(c, current_list)) {
                    program.call(0x10739890, 0x1007c08c, {pool + 0x37, 0, 0, frame.at(0x40), frame.at(0x38)});
                    const auto version = program.call(0x10768810, 0x1007c090);
                    program.call(0x1077b720, 0x1007c0b0,
                                 {error_buffer, pool + 0x4c, version, frame.at(0x40), frame.at(0x38), 0x641, pool + 0x24e});
                    program.call(0x10727890, 0x1007c0bc, {title, pool + 0xb8});
                    program.call(0x10727890, 0x1007c0cc, {message, pool + 0xd0, error_buffer});
                    program.call(0x10769b60, 0x1007c0dc, {title, message, 0});
                    c.set(0, 0); c.put32(output, 0); return result(c, 0);
                }
            }
            const auto offset = std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(count)))) * 136;
            c.set(0, offset); count = std::uint32_t(count) + 1;
            program.call(0x10739ef0, 0x1007c108, {buffer + offset, record, 136});
        }
    }
    c.put32(output, buffer); return result(c, count);
}

std::uint32_t copy_contract(Cpu& c, std::uint32_t destination_address, std::uint32_t source_address) {
    Frame frame(c, 0x10082c50, 0x40, {.first_gpr = 30});
    Kept<31> source(c, source_address); Kept<30> destination(c, destination_address);
    Program program(c);
    for (const auto offset : {0u, 4u, 8u, 12u}) {
        const auto value = c.u32(source + offset); c.set(0, value); c.put32(destination + offset, value);
    }
    const auto first = c.u32(source + 0x10), second = c.u32(source + 0x14);
    c.set(6, first); c.set(0, second); c.put32(destination + 0x10, first); c.put32(destination + 0x14, second);
    for (const auto offset : {0x18u, 0x1au}) {
        const auto value = c.u16(source + offset); c.set(0, value); c.put16(destination + offset, value);
    }
    program.call(0x10771110, 0x10082cbc, {destination + 0x1c, source + 0x1c, 5});
    const auto initial = c.u32(source + 0x24); c.set(0, initial); c.set(3, destination); c.put32(destination + 0x24, initial);
    for (const auto offset : {0x28u, 0x30u}) {
        const auto year = c.u16(source + offset + 2); c.set(0, year); c.put16(destination + offset + 2, year);
        const auto day = c.i16(source + offset); c.set(0, extended(day)); c.put16(destination + offset, extended(day));
        const auto leap = c.u8(source + offset + 4); c.set(0, leap); c.put8(destination + offset + 4, leap);
    }
    for (const auto [offset, width] : {std::pair{0x38u, 1u}, {0x3cu, 4u}, {0x40u, 1u}, {0x42u, 2u},
             {0x44u, 1u}, {0x45u, 1u}, {0x46u, 1u}, {0x48u, 4u}, {0x4cu, 1u}, {0x50u, 4u},
             {0x54u, 1u}, {0x58u, 4u}, {0x5cu, 1u}, {0x60u, 4u}}) {
        const auto value = width == 4 ? c.u32(source + offset) : width == 2 ? extended(c.i16(source + offset)) : c.u8(source + offset);
        c.set(0, value);
        if (width == 4) c.put32(destination + offset, value);
        else if (width == 2) c.put16(destination + offset, value);
        else c.put8(destination + offset, value);
    }
    return destination;
}

std::uint32_t process_deferred(Cpu& c, std::uint32_t manager_address, std::uint32_t person_id) {
    Frame frame(c, 0x10082d80, 0x50, {.first_gpr = 27});
    Kept<27> object(c, manager_address); Kept<31> transfers(c, c.toc(-0x7b40));
    Kept<28> wanted(c, person_id); Kept<29> count(c, 0); Kept<30> record(c, c.r(30));
    Program program(c);
    const auto list = c.u32(object + 0x38); c.set(3, list);
    if (nonzero_logical(c, list)) {
        const auto first = c.u32(list + 4); c.set(0, first); c.put32(list + 12, first);
        for (;;) {
            record = program.call(0x10738e00, 0x10082e74, {c.u32(object + 0x38)});
            if (!nonzero(c, std::int32_t(std::uint32_t(record)))) break;
            const auto id = c.i32(record); c.set(0, std::uint32_t(id));
            if (compare(c, id, std::int32_t(std::uint32_t(wanted))) != 0) continue;
            const auto origin = c.i32(record + 0x70); c.set(4, std::uint32_t(origin));
            if (compare(c, origin, -1) != 0) {
                const auto node = program.call(0x103eb440, 0x10082dd8, {c.u32(transfers), std::uint32_t(origin)});
                if (nonzero_logical(c, node)) {
                    const auto offer = c.u32(node); c.set(4, offer);
                    if (nonzero_logical(c, offer)) {
                        const auto current = c.i32(offer + 0x2c), expected = c.i32(record + 0x74);
                        c.set(3, std::uint32_t(current)); c.set(0, std::uint32_t(expected));
                        if (compare(c, current, expected) == 0) {
                            const auto status = c.u8(offer + 0x34); c.set(0, status);
                            if (compare(c, status, 15) == 0) { c.set(0, 8); c.put8(offer + 0x34, 8); }
                            program.call(0x1007f200, 0x10082e34, {object, record, c.u32(record + 0x70),
                                         c.u32(record + 0x74), c.u8(record + 0x78), 1, 1, 0});
                        }
                    }
                }
            } else {
                program.call(0x1007f200, 0x10082e5c, {object, record, 0xffffffff, 0xffffffff,
                             c.u8(record + 0x78), 1, 1, 0});
            }
            program.call(0x10738e60, 0x10082e68, {c.u32(object + 0x38), record});
            count = std::uint32_t(count) + 1;
        }
    }
    return result(c, count);
}

void remove_deferred(Cpu& c, std::uint32_t manager_address, std::uint32_t person_id) {
    Frame frame(c, 0x10082ea0, 0x40, {.first_gpr = 30});
    Kept<31> wanted(c, person_id); Kept<30> object(c, manager_address); Program program(c);
    const auto list = c.u32(object + 0x38); c.set(3, list);
    if (!nonzero_logical(c, list)) return;
    const auto first = c.u32(list + 4); c.set(0, first); c.put32(list + 12, first);
    for (;;) {
        const auto record = program.call(0x10738e00, 0x10082ef4, {c.u32(object + 0x38)});
        c.set(4, record);
        if (!nonzero(c, std::int32_t(record))) return;
        const auto id = c.i32(record); c.set(0, std::uint32_t(id));
        if (compare(c, id, std::int32_t(std::uint32_t(wanted))) == 0)
            program.call(0x10738e60, 0x10082eec, {c.u32(object + 0x38), record});
    }
}

void remove_primary(Cpu& c, std::uint32_t manager_address, std::uint32_t person) {
    Frame frame(c, 0x10084a80, 0x40, {}); Program program(c);
    const auto holder = c.toc(slot::contract_index); c.set(8, holder);
    const auto person_id = c.u32(person);
    const auto indices = c.u32(holder), offset = person_id * Contract::size;
    c.set(5, indices); c.set(0, offset);
    const auto index = c.i32(indices + offset); c.set(0, std::uint32_t(index));
    if (compare(c, index, -1) == 0) return;
    const auto record_offset = std::uint32_t(index) * Contract::size;
    const auto records = Manager{c, manager_address}.records().get();
    c.set(9, record_offset); c.set(10, records);
    const auto clubs_holder = c.toc(-0x7be8); c.set(7, clubs_holder); c.set(6, 0xffffffff);
    const auto league_holder = c.toc(-0x7ab8); c.set(3, league_holder);
    c.set(5, records + record_offset);
    const auto club_id = c.u32(records + record_offset + 4); c.set(0, club_id);
    const auto clubs = c.u32(clubs_holder); c.set(7, clubs);
    const auto club_offset = club_id * 0x248; c.set(5, club_offset);
    c.put32(records + record_offset, 0xffffffff);
    const auto current_offset = c.u32(person) * Contract::size; c.set(0, current_offset);
    c.set(4, clubs + club_offset);
    const auto current_indices = c.u32(holder); c.set(5, current_indices);
    c.put32(current_indices + current_offset, 0xffffffff);
    const auto league = program.call(0x10103510, 0x10084aec, {c.u32(league_holder)});
    if (nonzero_logical(c, league)) program.call(0x100f4f80, 0x10084af8);
}
}
