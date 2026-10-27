#include "contract_manager_detail.hpp"
#include <array>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
struct ReadProblem {
    std::uint32_t start;
    unsigned first_buffer, second_buffer, detail, line;
    bool allocation = false;
};

// These lookups are inlined by the original loader. Keep the values left
// in its scratch registers as well as the validation and index read order.
std::uint32_t loaded_contract(Cpu& c, std::uint32_t manager, std::uint32_t indices,
                              std::uint32_t person, std::int32_t limit, bool secondary,
                              unsigned output = 6) {
    if (!nonzero_logical(c, person)) { c.set(output, 0); return 0; }
    const auto checked = c.i32(person); c.set(6, std::uint32_t(checked));
    if (compare(c, checked, limit) >= 0) { c.set(output, 0); return 0; }
    const auto person_offset = c.u32(person) * 100; c.set(6, person_offset);
    const auto table = c.u32(indices); c.set(7, table);
    const auto index_address = person_offset + (secondary ? 4u : 0u); c.set(6, index_address);
    const auto index = c.i32(table + index_address); c.set(6, std::uint32_t(index));
    if (compare(c, index) < 0) { c.set(output, 0); return 0; }
    const auto offset = std::uint32_t(index) * 100; c.set(6, offset);
    const auto rows = c.u32(manager); c.set(7, rows);
    const auto found = rows + offset; c.set(output, found); return found;
}

// Two inlined variants use r0 for the index and leave different operands
// behind. Their algorithms still operate on named, extracted values.
std::uint32_t loaded_secondary_tail(Cpu& c, std::uint32_t manager, std::uint32_t indices,
                                    std::uint32_t person, std::int32_t limit, bool destination) {
    const unsigned output = destination ? 5 : 0;
    if (!nonzero_logical(c, person)) { c.set(output, 0); return 0; }
    const auto checked = c.i32(person); c.set(6, std::uint32_t(checked));
    if (compare(c, checked, limit) >= 0) { c.set(output, 0); return 0; }
    const auto id = c.u32(person); c.set(0, id);
    const auto table = c.u32(indices); c.set(destination ? 6 : 7, table);
    const auto offset = id * 100; c.set(destination ? 5 : 6, offset); c.set(0, offset + 4);
    const auto index = c.i32(table + offset + 4); c.set(0, std::uint32_t(index));
    if (compare(c, index) < 0) { c.set(output, 0); return 0; }
    const auto row_offset = std::uint32_t(index) * 100; c.set(0, row_offset);
    const auto rows = c.u32(manager); c.set(destination ? 5 : 6, rows);
    const auto found = rows + row_offset; c.set(output, found); return found;
}

void copy_loaded_date(Cpu& c, std::uint32_t destination, std::uint32_t source) {
    const auto year = c.u16(source + 2); c.set(0, year); c.put16(destination + 2, year);
    const auto day = c.i16(source); c.set(0, extended(day)); c.put16(destination, extended(day));
    const auto leap = c.u8(source + 4); c.set(0, leap); c.put8(destination + 4, leap);
}

void convert_legacy_contract(Cpu& c, std::uint32_t manager, std::uint32_t source, std::uint32_t offset) {
    const auto destination = [&] { return c.u32(manager) + offset; };
    for (unsigned at = 0; at <= 0x18; at += 4) c.put32(destination() + at, c.u32(source + at));
    for (unsigned at = 0x1c; at <= 0x20; ++at) c.put8(destination() + at, c.u8(source + at));
    c.put32(destination() + 0x24, c.u32(source + 0x24));
    for (unsigned at : {0x28u, 0x30u}) {
        const auto target = destination() + at;
        c.put16(target + 2, c.u16(source + at + 2));
        c.put16(target, extended(c.i16(source + at)));
        c.put8(target + 4, c.u8(source + at + 4));
    }
    for (unsigned mask : {0xfcu, 2u, 1u}) {
        const auto target = destination() + 0x38;
        c.put8(target, (c.u8(target) & ~mask) | (c.u8(source + 0x38) & mask));
    }
    // The last signed shift used to extract bit zero clears carry.
    c.s.wr<1>(guest::reg::xer_ca, 0);
    c.put32(destination() + 0x3c, c.u32(source + 0x3c));
    const auto appearance = c.u8(source + 0x40); c.put8(destination() + 0x40, appearance);
    const auto bonus = c.i16(source + 0x42); c.put16(destination() + 0x42, extended(bonus));
    const auto interest = c.u8(source + 0x44); c.put8(destination() + 0x44, interest);
    const auto played = c.u8(source + 0x45); c.put8(destination() + 0x45, played);
    const auto omitted = c.u8(source + 0x46); c.put8(destination() + 0x46, omitted);
    c.put32(destination() + 0x48, 0);
    const auto flag = c.u8(source + 0x47); c.put8(destination() + 0x4c, flag);
    c.put32(destination() + 0x50, 0); c.put8(destination() + 0x54, c.u8(source + 0x4c));
    c.put32(destination() + 0x58, c.u32(source + 0x50));
    c.put8(destination() + 0x5c, c.u8(source + 0x54));
    for (unsigned mask : {0xf0u, 0x0fu}) {
        const auto target = destination() + 0x60;
        c.put8(target, (c.u8(target) & ~mask) | (c.u8(source + 0x58) & mask));
    }
    // The free() below saves these nonvolatile operands in its own frame.
    keep(c, {{14, flag}, {15, c.u32(manager)}, {16, omitted}, {17, played}, {18, interest},
             {19, extended(bonus)}, {20, appearance}, {21, 0}, {22, offset + 0x1d}});
}
}

std::uint32_t load_contracts(Cpu& c, std::uint32_t manager_address, std::uint32_t file_address) {
    Frame frame(c, 0x10091bf0, 0x490, {.first_gpr = 13});
    Kept<23> object(c, manager_address); Kept<31> pool(c, c.toc(slot::pool));
    Kept<24> special_count(c, c.toc(-0x7b04)); Kept<25> club_count(c, c.toc(-0x7b50));
    Kept<13> work(c, c.toc(-0x7b74)); Kept<26> clubs(c, c.toc(-0x7be8));
    Kept<27> indices(c, c.toc(slot::contract_index)); Kept<28> message(c, c.toc(-0x79e0));
    Kept<29> title(c, c.toc(-0x79dc)); Kept<30> error_buffer(c, c.toc(-0x7ba8));
    Program program(c);
    const auto report = [&](ReadProblem problem) {
        program.call(0x10739890, problem.start + 0x18,
                     {pool + 0x37, 0, 0, frame.at(problem.first_buffer), frame.at(problem.second_buffer)});
        const auto version = program.call(0x10768810, problem.start + 0x1c);
        program.call(0x1077b720, problem.start + 0x3c,
                     {error_buffer, pool + 0x4c, version, frame.at(problem.first_buffer), frame.at(problem.second_buffer), problem.line, pool + problem.detail});
        program.call(0x10727890, problem.start + 0x48, {title, pool + (problem.allocation ? 0xb8 : 0x7232)});
        program.call(0x10727890, problem.start + 0x58, {message, pool + (problem.allocation ? 0xd0 : 0x7242), error_buffer});
        program.call(0x10769b60, problem.start + 0x68, {title, message, 0});
        return result(c, 0);
    };
    const auto stream = program.call(0x107475b0, 0x10091c40, {file_address, pool + 0x71e7, 0, 0x16, 4});
    const bool opened = nonzero_logical(c, stream); c.put32(frame.at(0x3c), stream);
    if (!opened) return report({0x10091c4c, 0x308, 0xe0, 0x71f4, 0x3071});
    const auto read = [&](std::uint32_t data, unsigned width, std::uint32_t count, std::uint32_t back) {
        return program.call(0x10747a80, back, {data, width, count, c.u32(frame.at(0x3c))});
    };
    auto version = program.call(0x10775550, 0x10091cc4, {c.u32(work)});
    c.set(4, 0x40000); c.set(0, 0x3e000);
    if (compare(c, std::int32_t(version), 0x3e000) > 0) {
        if (compare(c, std::int32_t(read(object + 0x30, 4, 1, 0x10091ce8)), 1) != 0)
            return report({0x10091cf0, 0x2e8, 0xd8, 0x72ed, 0x307b});
        const auto capacity = c.u32(object + 0x30); c.set(0, capacity); c.put32(object + 0x2c, capacity);
        const auto count = c.i32(object + 0x30); c.set(0, std::uint32_t(count));
        if (nonzero(c, count)) {
            c.put32(object + 0x28, program.call(0x10775130, 0x10091d7c, {std::uint32_t(count) * 24}));
            const auto rows = c.u32(object + 0x28); c.set(3, rows);
            if (!nonzero_logical(c, rows)) return report({0x10091d8c, 0x2c8, 0xd0, 0x7338, 0x3086, true});
            const auto loaded = read(rows, 24, c.u32(object + 0x30), 0x10091e0c);
            const auto current_count = c.i32(object + 0x30); c.set(0, std::uint32_t(current_count));
            if (compare(c, current_count, std::int32_t(loaded)) != 0) return report({0x10091e18, 0x2a8, 0xc8, 0x7382, 0x308e});
        }
    }
    if (compare(c, std::int32_t(read(object + 4, 4, 1, 0x10091e9c)), 1) != 0)
        return report({0x10091ea4, 0x288, 0xc0, 0x73ca, 0x3097});
    const auto capacity = c.u32(object + 4); c.set(0, capacity); c.put32(object + 8, capacity);
    const auto contract_count = c.i32(object + 4); c.set(0, std::uint32_t(contract_count));
    if (nonzero(c, contract_count)) {
        c.put32(object, program.call(0x10775130, 0x10091f30, {std::uint32_t(contract_count) * 100}));
        const auto allocated = c.u32(object); c.set(0, allocated);
        if (!nonzero_logical(c, allocated)) return report({0x10091f40, 0x268, 0xb8, 0x7411, 0x30a4, true});
        version = program.call(0x10775550, 0x10091fb8, {c.u32(work)});
        c.set(4, 0x40000); c.set(0, 0x3e032);
        if (compare(c, std::int32_t(version), 0x3e032) > 0) {
            const auto loaded = read(c.u32(object), 100, c.u32(object + 4), 0x10091fdc);
            const auto count = c.i32(object + 4); c.set(0, std::uint32_t(count));
            if (compare(c, count, std::int32_t(loaded)) != 0) return report({0x10091fe8, 0x248, 0xb0, 0x7457, 0x30ae});
        } else {
            work = program.call(0x10775130, 0x10092064, {c.u32(object + 4) * 92});
            if (!nonzero(c, std::int32_t(std::uint32_t(work)))) return report({0x1009206c, 0x228, 0xa8, 0x749b, 0x30ba, true});
            // Preserve the reference's destination: the 92-byte read goes
            // into the main array, but conversion reads the temporary one.
            const auto loaded = read(c.u32(object), 92, c.u32(object + 4), 0x100920f0);
            const auto count = c.i32(object + 4); c.set(0, std::uint32_t(count));
            if (compare(c, count, std::int32_t(loaded)) != 0) return report({0x100920fc, 0x208, 0xa0, 0x7457, 0x30c1});
            std::uint32_t row = 0, source_offset = 0, destination_offset = 0;
            c.set({{5, 0}, {8, 0}, {6, 0}});
            for (;;) {
                const auto current_count = c.i32(object + 4); c.set(0, std::uint32_t(current_count));
                if (compare(c, std::int32_t(row), current_count) >= 0) break;
                convert_legacy_contract(c, object, work + source_offset, destination_offset);
                source_offset += 92; destination_offset += 100; ++row;
                c.set({{5, source_offset}, {6, destination_offset}, {8, row}});
            }
            program.call(0x10775170, 0x100923fc, {work});
        }
        // Normalize stored person ids through the loaded primary index.
        std::uint32_t id = 0, offset = 0; c.set(5, 0); c.set(3, 0);
        for (;;) {
            const auto count_holder = c.toc(slot::person_count); c.set(4, count_holder);
            const auto count = c.i32(count_holder); c.set(0, std::uint32_t(count));
            if (compare(c, std::int32_t(id), count) >= 0) break;
            const auto person = c.u32(c.toc(-0x7be0)) + offset; c.set(4, person);
            const auto primary = loaded_contract(c, object, indices, person, count, false);
            if (nonzero_logical(c, primary)) {
                const auto again = loaded_contract(c, object, indices, person, count, false);
                const auto stored = c.i32(again); c.set(6, std::uint32_t(stored));
                if (compare(c, std::int32_t(id), stored) != 0) {
                    const auto current = loaded_contract(c, object, indices, person, count, false, 4);
                    c.put32(current, id);
                }
            }
            offset += 112; ++id; c.set(3, offset); c.set(5, id);
        }
    }
    if (compare(c, std::int32_t(read(object + 0x10, 4, 1, 0x10092550)), 1) != 0)
        return report({0x10092558, 0x1e8, 0x98, 0x74e5, 0x30fd});
    const auto scheduled_capacity = c.u32(object + 0x10); c.set(0, scheduled_capacity); c.put32(object + 0x14, scheduled_capacity);
    const auto scheduled_count = c.i32(object + 0x10); c.set(0, std::uint32_t(scheduled_count));
    if (nonzero(c, scheduled_count)) {
        c.put32(object + 0x0c, program.call(0x10775130, 0x100925e4, {std::uint32_t(scheduled_count) * 16}));
        const auto rows = c.u32(object + 0x0c); c.set(3, rows);
        if (!nonzero_logical(c, rows)) return report({0x100925f4, 0x1c8, 0x90, 0x752f, 0x310a, true});
        const auto loaded = read(rows, 16, c.u32(object + 0x10), 0x10092674);
        const auto count = c.i32(object + 0x10); c.set(0, std::uint32_t(count));
        if (compare(c, count, std::int32_t(loaded)) != 0) return report({0x10092680, 0x1a8, 0x88, 0x7571, 0x3112});
    }
    if (compare(c, std::int32_t(read(object + 0x18, 4, 1, 0x10092704)), 1) != 0)
        return report({0x1009270c, 0x188, 0x80, 0x75b1, 0x311a});
    struct ListRead {
        unsigned offset, buffer, date;
        std::uint32_t count_back, construct_back, date_back, allocation_back, init_back, row_back, append_back;
        ReadProblem count_problem, row_problem;
    };
    constexpr std::array lists{
        ListRead{0x34, 0x3b0, 0x418, 0x10092790, 0x1009281c, 0x10092824, 0x1009282c, 0x10092838, 0x10092858, 0x100928e0,
            {0x10092798, 0x168, 0x78, 0x75f9, 0x3121}, {0x10092860, 0x148, 0x70, 0x7637, 0x3133}},
        ListRead{0x38, 0x328, 0x390, 0x10092908, 0x10092994, 0x1009299c, 0x100929a4, 0x100929b0, 0x10092a4c, 0x10092ad4,
            {0x10092910, 0x128, 0x68, 0x75f9, 0x313f}, {0x10092a54, 0xe8, 0x58, 0x7637, 0x3156}}
    };
    for (const auto& list : lists) {
        if (compare(c, std::int32_t(read(frame.at(0x3a), 2, 1, list.count_back)), 1) != 0) return report(list.count_problem);
        const auto count = c.i16(frame.at(0x3a)); c.set(0, extended(count));
        if (!nonzero(c, count)) continue;
        program.call(0x103721f0, list.construct_back, {frame.at(list.buffer)});
        program.call(0x10715880, list.date_back, {frame.at(list.date)});
        work = program.call(0x10773520, list.allocation_back, {16});
        if (nonzero(c, std::int32_t(std::uint32_t(work)))) program.call(0x10738c00, list.init_back);
        c.put32(object + list.offset, work);
        if (list.offset == 0x38) {
            const auto present = c.u32(object + list.offset); c.set(0, present);
            if (!nonzero_logical(c, present)) return report({0x100929c0, 0x108, 0x60, 0x766e, 0x314c, true});
        }
        work = 0;
        for (;;) {
            const auto current_count = c.i16(frame.at(0x3a)); c.set(0, extended(current_count));
            const auto index = std::int16_t(std::uint32_t(work)); c.set(3, extended(index));
            if (compare(c, index, current_count) >= 0) break;
            if (compare(c, std::int32_t(read(frame.at(list.buffer), 136, 1, list.row_back)), 1) != 0) return report(list.row_problem);
            program.call(0x10738d10, list.append_back, {c.u32(object + list.offset), frame.at(list.buffer), 136});
            work = std::uint32_t(work) + 1;
        }
    }
    const auto has_flag = read(frame.at(0x38), 1, 1, 0x10092afc);
    if (!nonzero(c, std::int32_t(has_flag))) {
        const auto pending = c.u32(object + 0x34); c.set(3, pending);
        if (nonzero_logical(c, pending)) {
            const auto first = c.u32(pending + 4); c.set(0, first); c.put32(pending + 12, first);
            for (;;) {
                work = program.call(0x10738e00, 0x10092bbc, {c.u32(object + 0x34)});
                if (!nonzero(c, std::int32_t(std::uint32_t(work)))) break;
                const auto person_id = c.u32(work); c.set(0, person_id);
                const auto staff = c.u32(c.toc(-0x7be0)); c.set(5, staff); c.set(0, person_id * 112 + 0x2c);
                const auto matching = program.call(0x1077dc90, 0x10092b3c, {c.u32(staff + person_id * 112 + 0x2c), pool + 0x76b9});
                if (!nonzero(c, std::int32_t(matching))) {
                    program.call(0x107156f0, 0x10092b5c, {frame.at(0x50), 1, 5, 2004, 0xffffffff});
                    copy_loaded_date(c, work + 0x30, frame.at(0x50));
                } else {
                    const auto year = c.u16(work + 0x32); c.set(0, year);
                    if (compare_logical(c, year, 2000) < 0) {
                        program.call(0x107156f0, 0x10092b9c, {frame.at(0x48), 1, 5, 2002, 0xffffffff});
                        copy_loaded_date(c, work + 0x30, frame.at(0x48));
                    }
                }
            }
        }
    }
    program.call(0x10747900, 0x10092bcc, {frame.at(0x3c)});
    // A secondary contract cannot outlive its primary. With no primary,
    // remove roster references and invalidate its record and index.
    std::uint32_t id = 0, person_offset = 0; c.set(4, 0); c.set(3, 0);
    for (;;) {
        const auto count_holder = c.toc(slot::person_count); c.set(5, count_holder);
        const auto count = c.i32(count_holder); c.set(0, std::uint32_t(count));
        if (compare(c, std::int32_t(id), count) >= 0) break;
        const auto person = c.u32(c.toc(-0x7be0)) + person_offset; c.set(5, person);
        const auto second = loaded_contract(c, object, indices, person, count, true);
        if (nonzero_logical(c, second) && nonzero_logical(c, loaded_contract(c, object, indices, person, count, false))) {
            const auto secondary = loaded_contract(c, object, indices, person, count, true, 8);
            const auto primary = loaded_contract(c, object, indices, person, count, false);
            const auto date = c.u32(primary + 0x30), tail = c.u32(primary + 0x34);
            c.set(7, date); c.set(6, tail); c.put32(frame.at(0x40), date);
            const auto second_year = c.u16(secondary + 0x32), first_year = c.u16(frame.at(0x42));
            c.set(7, second_year); c.set(9, first_year); c.put32(frame.at(0x44), tail);
            const auto years = compare_logical(c, first_year, second_year);
            bool earlier = years < 0;
            if (!years) {
                const auto first_day = c.i16(frame.at(0x40)), second_day = c.i16(secondary + 0x30);
                const auto bits = extended(second_day) ^ extended(first_day);
                c.set(8, extended(second_day)); c.set(7, std::uint32_t(std::int32_t(bits) >> 1));
                c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1));
                earlier = first_day < second_day;
            }
            c.set(6, earlier);
            if (nonzero(c, earlier)) {
                const auto source = loaded_contract(c, object, indices, person, count, false, 7);
                const auto destination = loaded_secondary_tail(c, object, indices, person, count, true);
                copy_loaded_date(c, destination + 0x30, source + 0x30);
            }
        } else {
            const auto secondary = loaded_secondary_tail(c, object, indices, person, count, false);
            if (nonzero_logical(c, secondary)) {
                std::uint32_t club_id = 0, club_offset = 0; c.set({{9, 0}, {10, 0}, {7, 0}});
                for (;;) {
                    const auto omitted = c.u32(special_count) * 2, total = c.u32(club_count);
                    const auto limit = std::int32_t(total - omitted); c.set(6, omitted); c.set(0, std::uint32_t(limit));
                    if (compare(c, std::int32_t(club_id), limit) >= 0) break;
                    for (unsigned slot = 0; slot < 50; ++slot) {
                        const auto rows = c.u32(clubs), roster = rows + slot * 4 + 0x98;
                        const auto occupant = c.u32(club_offset + roster); c.set(6, roster); c.set(0, occupant);
                        if (compare_logical(c, occupant, person) == 0) c.put32(club_offset + roster, 0);
                    }
                    c.set(8, 200); c.set(11, 40); c.s.wr<4>(guest::reg::ctr, 0);
                    club_offset += 0x248; ++club_id; c.set(9, club_offset); c.set(10, club_id);
                }
                const auto current_id = c.u32(person); c.set(0, current_id);
                const auto table = c.u32(indices); c.set(7, table); c.set(6, current_id * 100); c.set(0, current_id * 100 + 4);
                const auto index = c.i32(table + current_id * 100 + 4); c.set(0, std::uint32_t(index));
                if (compare(c, index, -1) != 0) {
                    const auto rows = c.u32(object); c.set(0, std::uint32_t(index) * 100); c.set(6, rows); c.set(7, 0xffffffff);
                    c.put32(rows + std::uint32_t(index) * 100, 0xffffffff);
                    const auto repeated_id = c.u32(person), repeated_table = c.u32(indices);
                    c.set(6, repeated_table); c.set(5, repeated_id * 100); c.set(0, repeated_id * 100 + 4);
                    c.put32(repeated_table + repeated_id * 100 + 4, 0xffffffff);
                }
            }
        }
        person_offset += 112; ++id; c.set(3, person_offset); c.set(4, id);
    }
    return result(c, 1);
}
}
