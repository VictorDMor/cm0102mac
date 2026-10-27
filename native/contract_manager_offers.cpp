#include "contract_manager_detail.hpp"
#include <utility>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
void copy_terms_prefix(Cpu& c, Program& program, std::uint32_t destination,
                       std::uint32_t source, std::uint32_t back) {
    for (const auto offset : {0u, 4u, 8u, 12u}) {
        const auto value = c.u32(source + offset); c.set(0, value); c.put32(destination + offset, value);
    }
    const auto first = c.u32(source + 0x10), second = c.u32(source + 0x14);
    c.set(6, first); c.set(0, second); c.put32(destination + 0x10, first); c.put32(destination + 0x14, second);
    for (const auto offset : {0x18u, 0x1au}) {
        const auto value = c.u16(source + offset); c.set(0, value); c.put16(destination + offset, value);
    }
    program.call(0x10771110, back, {destination + 0x1c, source + 0x1c, 5});
}
void copy_terms_tail(Cpu& c, std::uint32_t destination, std::uint32_t source, unsigned residue) {
    for (const auto offset : {0x28u, 0x30u}) {
        const auto year = c.u16(source + offset + 2); c.set(residue, year); c.put16(destination + offset + 2, year);
        const auto day = c.i16(source + offset); c.set(residue, extended(day)); c.put16(destination + offset, extended(day));
        const auto flag = c.u8(source + offset + 4); c.set(residue, flag); c.put8(destination + offset + 4, flag);
    }
    for (const auto [offset, width] : {std::pair{0x38u, 1u}, {0x3cu, 4u}, {0x40u, 1u}, {0x42u, 2u},
             {0x44u, 1u}, {0x45u, 1u}, {0x46u, 1u}, {0x48u, 4u}, {0x4cu, 1u}, {0x50u, 4u},
             {0x54u, 1u}, {0x58u, 4u}, {0x5cu, 1u}, {0x60u, 4u}}) {
        const auto value = width == 4 ? c.u32(source + offset) : width == 2 ? extended(c.i16(source + offset)) : c.u8(source + offset);
        c.set(residue, value);
        if (width == 4) c.put32(destination + offset, value);
        else if (width == 2) c.put16(destination + offset, value);
        else c.put8(destination + offset, value);
    }
}
std::int32_t packed_role(Cpu& c, std::uint32_t source) {
    const auto flags = c.u8(source + 0x38);
    const auto role = std::int32_t(std::int8_t(flags)) >> 2;
    c.set(0, std::uint32_t(role)); c.s.wr<1>(guest::reg::xer_ca, role < 0 && (flags & 2));
    return role;
}
}

void add_pending(Cpu& c, std::uint32_t manager_address, std::uint32_t contract_address, std::uint32_t origin_value,
                   std::uint32_t offer_value, std::uint32_t kind_value, std::uint32_t notify_value,
                   std::uint32_t force_transfer_value) {
    Frame frame(c, 0x1007f200, 0x100, {.first_gpr = 21});
    Kept<23> origin(c, origin_value); Kept<30> today(c, c.toc(-0x7b80));
    Kept<22> source(c, contract_address); Kept<28> transfers(c, c.toc(-0x7b40));
    Kept<29> indices(c, c.toc(slot::contract_index)); Kept<21> object(c, manager_address);
    Kept<24> offer(c, offer_value); Kept<25> kind(c, kind_value);
    Kept<26> notify(c, notify_value); Kept<27> force_transfer(c, force_transfer_value);
    Kept<31> existing(c, c.r(31)); Program program(c);
    program.call(0x10717890, 0x1007f248, {frame.at(0x38), today, 14});
    program.call(0x103721f0, 0x1007f250, {frame.at(0x40)});
    program.call(0x10715880, 0x1007f258, {frame.at(0xa8)});
    const auto person_id = c.u32(source); c.set(4, person_id);
    const auto index_base = c.u32(indices); c.set(3, index_base);
    const auto person_offset = person_id * Contract::size; c.set(0, person_offset); c.set(3, index_base + person_offset);
    const auto pending_count = c.u8(index_base + person_offset + 8); c.set(0, pending_count);
    bool updating = false;
    if (nonzero_logical(c, pending_count)) {
        existing = program.call(0x1007f720, 0x1007f280, {object, person_id, c.u32(source + 4)});
        updating = nonzero(c, std::int32_t(std::uint32_t(existing)));
    }
    if (updating) {
        copy_terms_prefix(c, program, existing, source, 0x1007f2d8);
        const auto value = c.u32(source + 0x24); c.set(3, value);
        c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(force_transfer)))));
        const bool transfer = nonzero(c, std::int8_t(std::uint32_t(force_transfer)));
        c.put32(existing + 0x24, value);
        copy_terms_tail(c, existing, source, 0);
        c.set(0, transfer ? 2 : 0); c.put8(existing + 0x64, transfer ? 2 : 0);
        const auto year = c.u16(today + 2); c.set(3, year); c.set(0, 0xffffffff); c.put16(existing + 0x6a, year);
        const auto day = c.i16(today); c.set(3, extended(day)); c.put16(existing + 0x68, extended(day));
        const auto leap = c.u8(today + 4); c.set(3, leap); c.put8(existing + 0x6c, leap);
        c.put32(existing + 0x70, origin); c.put32(existing + 0x74, offer); c.put8(existing + 0x78, kind);
        c.put32(frame.at(0xbc), 0xffffffff);
        const bool acknowledgement = compare(c, packed_role(c, source), 6) != 0;
        c.set(0, acknowledgement); c.put8(existing + 0x79, acknowledgement);
    } else {
        const auto temporary = frame.at(0x40);
        copy_terms_prefix(c, program, temporary, source, 0x1007f448);
        const auto value = c.u32(source + 0x24); c.set(4, value); c.set(6, 0);
        const auto leap = c.u8(today + 4); c.set(3, leap); c.set(0, 0xffffffff); c.put32(temporary + 0x24, value);
        const auto year = c.u16(today + 2); c.set(5, year);
        // The original snapshots today's fields before the remaining copies.
        const auto first_year = c.u16(source + 0x2a); c.set(7, first_year);
        const auto day = c.i16(today); c.set(4, extended(day));
        c.put16(temporary + 0x2a, first_year);
        const auto first_day = c.i16(source + 0x28); c.set(7, extended(first_day)); c.put16(temporary + 0x28, extended(first_day));
        const auto first_leap = c.u8(source + 0x2c); c.set(7, first_leap); c.put8(temporary + 0x2c, first_leap);
        const auto second_year = c.u16(source + 0x32); c.set(7, second_year); c.put16(temporary + 0x32, second_year);
        const auto second_day = c.i16(source + 0x30); c.set(7, extended(second_day)); c.put16(temporary + 0x30, extended(second_day));
        const auto second_leap = c.u8(source + 0x34); c.set(7, second_leap); c.put8(temporary + 0x34, second_leap);
        for (const auto [offset, width] : {std::pair{0x38u, 1u}, {0x3cu, 4u}, {0x40u, 1u}, {0x42u, 2u},
                 {0x44u, 1u}, {0x45u, 1u}, {0x46u, 1u}, {0x48u, 4u}, {0x4cu, 1u}, {0x50u, 4u},
                 {0x54u, 1u}, {0x58u, 4u}, {0x5cu, 1u}, {0x60u, 4u}}) {
            const auto field = width == 4 ? c.u32(source + offset) : width == 2 ? extended(c.i16(source + offset)) : c.u8(source + offset);
            c.set(7, field);
            if (width == 4) c.put32(temporary + offset, field);
            else if (width == 2) c.put16(temporary + offset, field);
            else c.put8(temporary + offset, field);
        }
        c.put8(temporary + 0x64, 0); c.put16(temporary + 0x6a, year);
        c.put16(temporary + 0x68, extended(day)); c.put8(temporary + 0x6c, leap);
        c.put32(temporary + 0x70, origin); c.put32(temporary + 0x74, offer); c.put8(temporary + 0x78, kind);
        c.put32(temporary + 0x7c, 0xffffffff);
        const bool acknowledgement = compare(c, packed_role(c, source), 6) != 0;
        if (acknowledgement) c.set(0, 1);
        c.put8(temporary + 0x79, acknowledgement);
        const auto list = c.u32(object + 0x34); c.set(0, list);
        if (!nonzero_logical(c, list)) {
            offer = program.call(0x10773520, 0x1007f560, {16});
            if (nonzero(c, std::int32_t(std::uint32_t(offer)))) program.call(0x10738c00, 0x1007f56c);
            c.put32(object + 0x34, offer);
        }
        program.call(0x10738d10, 0x1007f580, {c.u32(object + 0x34), temporary, 136});
        const auto offset = c.u32(source) * Contract::size; c.set(0, offset);
        const auto base = c.u32(indices); c.set(3, base); c.set(4, base + offset);
        const auto count = c.u8(base + offset + 8); c.set(3, count); c.set(0, std::uint32_t(count) + 1);
        c.put8(base + offset + 8, std::uint32_t(count) + 1);
    }
    if (compare(c, std::int32_t(std::uint32_t(origin)), -1) != 0) {
        const auto node = program.call(0x103eb440, 0x1007f5b0, {c.u32(transfers), origin});
        const auto transfer = c.u32(node); c.set(3, transfer);
        const auto id = c.i32(source), transfer_person = c.i32(transfer + 12);
        c.set(5, std::uint32_t(id)); c.set(0, std::uint32_t(transfer_person));
        if (compare(c, transfer_person, id) == 0)
            program.call(0x103e6e30, 0x1007f5d0, {c.u32(transfers), origin});
        else program.call(0x103e6f70, 0x1007f5e0, {c.u32(transfers), origin});
    }
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(notify)))));
    if (nonzero(c, std::int8_t(std::uint32_t(notify))))
        program.call(0x10080120, 0x1007f5f8, {object, c.u32(source), origin});
}

void remove_all_pending(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address) {
    Frame frame(c, 0x1007fa30, 0x50, {.first_gpr = 29});
    Kept<31> holder(c, c.toc(slot::contract_index)); Kept<30> person(c, person_address);
    Kept<29> object(c, manager_address); Program program(c);
    const auto list = c.u32(object + 0x34); c.set(0, list);
    if (!nonzero_logical(c, list)) return;
    const auto offset = c.u32(person) * Contract::size; c.set(0, offset);
    const auto indices = c.u32(holder); c.set(3, indices + offset);
    const auto count = c.u8(indices + offset + 8); c.set(0, count);
    if (!nonzero_logical(c, count)) return;
    program.call(0x103ea270, 0x1007fa88, {c.u32(c.toc(-0x7b40))});
    const auto current_list = c.u32(object + 0x34); c.set(3, current_list);
    const auto first = c.u32(current_list + 4); c.set(0, first); c.put32(current_list + 12, first);
    for (;;) {
        const auto record = program.call(0x10738e00, 0x1007faf4, {c.u32(object + 0x34)});
        c.set(4, record);
        if (!nonzero(c, std::int32_t(record))) return;
        const auto id = c.i32(record), wanted = c.i32(person); c.set(3, std::uint32_t(id)); c.set(0, std::uint32_t(wanted));
        if (compare(c, id, wanted) != 0) continue;
        program.call(0x10738e60, 0x1007fab4, {c.u32(object + 0x34), record});
        const auto person_offset = c.u32(person) * Contract::size; c.set(3, person_offset);
        const auto base = c.u32(holder); c.set(5, base); c.set(4, person_offset + 8);
        const auto pending = c.u8(base + person_offset + 8); c.set(3, pending); c.set(0, std::uint32_t(pending) - 1);
        c.put8(base + person_offset + 8, std::uint32_t(pending) - 1);
        const auto repeated = c.u32(person) * Contract::size; c.set(3, repeated);
        const auto current_base = c.u32(holder); c.set(4, current_base); c.set(0, repeated + 8);
        const auto left = c.u8(current_base + repeated + 8); c.set(0, left);
        if (!nonzero_logical(c, left)) return;
    }
}

std::uint32_t acknowledge_pending(Cpu& c, std::uint32_t manager_address, std::uint32_t offer_address) {
    Frame frame(c, 0x1007f610, 0x40, {.first_gpr = 30});
    Kept<31> offer(c, offer_address); Kept<30> object(c, manager_address); Program program(c);
    const auto list = c.u32(object + 0x34); c.set(3, list);
    const auto first = c.u32(list + 4); c.set(0, first); c.put32(list + 12, first);
    for (;;) {
        const auto record = program.call(0x10738e00, 0x1007f694, {c.u32(object + 0x34)});
        if (!nonzero_logical(c, record)) break;
        const auto id = c.i32(record), expected_id = c.i32(offer + 12);
        c.set(4, std::uint32_t(id)); c.set(0, std::uint32_t(expected_id));
        if (compare(c, id, expected_id) != 0) continue;
        const auto club = c.i32(record + 4), expected_club = c.i32(offer + 8);
        c.set(4, std::uint32_t(club)); c.set(0, std::uint32_t(expected_club));
        if (compare(c, club, expected_club) != 0) continue;
        const auto acknowledged = c.i8(record + 0x79); c.set(0, std::uint32_t(std::int32_t(acknowledged)));
        if (nonzero(c, acknowledged)) continue;
        c.set(0, 1); c.put8(record + 0x79, 1);
        program.call(0x10080120, 0x1007f684, {object, c.u32(offer + 12), c.u32(offer + 0x28)});
        break;
    }
    const auto id = c.u32(offer + 12), club_id = c.u32(offer + 8);
    c.set(6, 0x10624dd3);
    const auto person_offset = id * Staff::size, clubs = c.u32(c.toc(-0x7be8));
    const auto staff = c.u32(c.toc(-0x7be0)); c.set(5, staff);
    const auto person = staff + person_offset;
    const auto attributes = c.u32(person + 0x14); c.set(4, attributes);
    const auto ability = c.i16(attributes + 10); c.set(4, extended(ability));
    const auto club_offset = club_id * 0x248; c.set(0, club_offset); c.set(3, clubs + club_offset);
    const auto high = std::int32_t((std::int64_t(0x10624dd3) * ability) >> 32);
    const auto shifted = high >> 5;
    const auto quotient = std::uint32_t(shifted) + (std::uint32_t(shifted) >> 31);
    c.set(4, quotient);
    const auto reputation = c.i16(clubs + club_offset + 0x1ec);
    const auto bits = quotient ^ extended(reputation);
    c.set(3, std::uint32_t(std::int32_t(bits) >> 1)); c.set(0, (std::uint32_t(std::int32_t(bits) >> 1) - (bits & quotient)) >> 31);
    c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1));
    return result(c, std::int32_t(quotient) > reputation);
}
}
