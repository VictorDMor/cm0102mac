#include "bra_nat_second_adapter.hpp"
#include "guest_values.hpp"
#include "guest_saved_registers.hpp"
#include "import_descriptor_adapter.hpp"
#include "routine_check.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
using guest::compare;
using guest::compare_logical;

bool declining = std::getenv("CM_BRA_NAT_SECOND_ORIGINAL") != nullptr;
const bool original_rules = std::getenv("CM_BRA_NAT_SECOND_RULES_ORIGINAL") != nullptr;
constexpr auto construct_entry = 0x105177b0u, destroy_entry = 0x10517af0u;
constexpr auto advance_groups_entry = 0x10519010u, create_group_entry = 0x105190d0u;
constexpr auto record_result_entry = 0x105192c0u, announce_result_entry = 0x10519390u;
constexpr auto next_season_entry = 0x10518d70u;
constexpr auto collect_entry = 0x10518b50u;
constexpr auto configure_entry = 0x10518ca0u;
constexpr auto calendar_entry = 0x10517c00u;
constexpr auto standing_size = 0x44u, club_size = 0x248u;
constexpr auto update_group_entry = 0x10518b10u;
constexpr auto fixture_times_entry = 0x10519540u;
constexpr auto base_result_entry = 0x105195d0u;
constexpr auto fixture_record_lookup = 0x10459b50u;
constexpr auto base_result_for_club = 0x104aa430u;
constexpr auto base_table_finalizer = 0x104a1350u, base_result_history = 0x104a47f0u;
constexpr auto call_descriptor = 0x10781320u;

struct Division {
    Cpu& cpu;
    std::uint32_t address;
    guest::Field<std::int16_t> club_count() const { return {cpu, address + 0x3e}; }
    guest::Field<std::uint32_t> standings() const { return {cpu, address + 0xb0}; }
    std::int32_t year() const { return cpu.i16(address + 0x40); }
    std::uint32_t fixtures() const { return cpu.u32(address + 8); }
    guest::Field<std::uint32_t> group_storage() const { return {cpu, address + 0xc}; }
    std::uint32_t groups() const { return group_storage(); }
    guest::Field<std::int32_t> group_capacity() const { return {cpu, address + 0x2c}; }
    bool table_only() const { return !original_rules && group_capacity() == 0; }
    std::int32_t last_group() const { return cpu.i32(address + 0x30); }
    guest::Field<std::uint32_t> calendar_rows() const { return {cpu, address + 0xbc}; }
};

struct FixtureTimes {
    Cpu& cpu;
    std::uint32_t address;
    void store(std::uint32_t weekday, std::uint32_t first, std::uint32_t second) const {
        cpu.put8(address + 0x76, weekday);
        cpu.put8(address + 0x77, first);
        cpu.put8(address + 0x78, second);
    }
};

void block(Cpu& cpu, std::uint32_t pc, unsigned instructions) {
    cpu.s.step(pc);
    cpu.s.charge(instructions - 1);
}

int record_order(Cpu& cpu, std::int32_t value) {
    const auto preserved = cpu.cr(0) & 0xf0;
    const auto order = compare(cpu, value);
    cpu.set_cr(0, preserved | cpu.cr(0));
    return order;
}

void assertion(Cpu& cpu, std::uint32_t pc, std::uint32_t source, std::uint32_t message,
               unsigned filename, unsigned extension, unsigned line) {
    const auto frame = cpu.sp();
    block(cpu, pc, 6);
    cpu.set({{3, source}, {4, 0}, {5, 0}, {6, frame + filename}, {7, frame + extension}});
    cpu.call(0x10739890, pc + 24);
    block(cpu, pc + 24, 1);
    cpu.link(pc + 28);
    cpu.set(3, cpu.toc(-0x5b20));
    cpu.s.charge(2);
    block(cpu, pc + 28, 7);
    cpu.set({{5, cpu.r(3)}, {3, message}, {4, source + 0x13},
             {6, frame + filename}, {7, frame + extension}, {8, line}});
    cpu.call(0x1077b720, pc + 56);
    block(cpu, pc + 56, 4);
    cpu.set({{4, message}, {3, source + 0x20}, {5, 0}});
    cpu.call(0x10769b60, pc + 72);
}

template <unsigned First>
void return_multiple(Cpu& cpu, std::uint32_t pc, unsigned frame_size) {
    block(cpu, pc, 5);
    const auto stack = cpu.sp() + frame_size;
    cpu.set(0, cpu.u32(stack + 8));
    cpu.set(1, stack);
    cpu.set_lr(cpu.r(0));
    guest::transfer_saved_registers<First, true>(cpu, stack);
    cpu.finish();
}

void collect_clubs(Cpu& cpu) {
    GuestNativeScope scope(cpu.s, collect_entry);
    block(cpu, collect_entry, 15);
    const auto stack = cpu.sp(), frame = stack - 0x80;
    const auto object = cpu.r(3);
    cpu.set(0, cpu.lr());
    guest::transfer_saved_registers<26>(cpu, stack);
    cpu.set(30, object);
    const auto total = cpu.toc(-0x7b50);
    cpu.set(26, total);
    const auto second_division = cpu.toc(-0x6b7c);
    cpu.set(27, second_division);
    unsigned entered = 0;
    cpu.set(31, entered);
    const auto message = cpu.toc(-0x7ba8);
    cpu.set(28, message);
    const auto source = cpu.toc(-0x4550);
    cpu.set(29, source);
    cpu.put32(stack + 8, cpu.r(0));
    cpu.set(0, 28);
    cpu.put32(frame, stack);
    cpu.set(1, frame);
    scope.frame(frame);
    const Division league{cpu, object};
    league.club_count() = 28;
    const auto count = std::int32_t(league.club_count());
    cpu.set({{0, std::uint32_t(count)}, {3, std::uint32_t(count) * standing_size}});
    cpu.call(0x10775130, 0x10518b8c);
    block(cpu, 0x10518b8c, 3);
    const auto allocated = cpu.r(3);
    const bool present = compare_logical(cpu, allocated) != 0;
    league.standings() = allocated;
    if (!present) {
        assertion(cpu, 0x10518b98, source, message, 0x40, 0x38, 0x1aa);
        block(cpu, 0x10518be0, 5);
        const auto success = cpu.toc(-0x7bac);
        cpu.set({{4, success}, {0, 0}, {3, 0}});
        cpu.put8(success, 0);
    } else {
        block(cpu, 0x10518bf4, 4);
        const auto clubs = cpu.toc(-0x7be8);
        cpu.set(3, clubs);
        unsigned index = 0;
        cpu.set(28, index);
        auto club = cpu.u32(clubs);
        cpu.set(29, club);
        for (;;) {
            block(cpu, 0x10518c54, 4);
            const auto total_clubs = cpu.i32(total);
            cpu.set({{0, std::uint32_t(total_clubs)}, {3, std::uint32_t(std::int16_t(index))}});
            if (compare(cpu, std::int16_t(index), total_clubs) >= 0) break;
            block(cpu, 0x10518c04, 3);
            const auto division = cpu.u32(club + 0x30);
            cpu.set(3, division);
            if (compare_logical(cpu, division) != 0) {
                block(cpu, 0x10518c10, 4);
                const auto division_id = cpu.i32(division);
                cpu.set(3, std::uint32_t(division_id));
                const auto wanted = cpu.i32(second_division);
                cpu.set(0, std::uint32_t(wanted));
                if (compare(cpu, division_id, wanted) == 0) {
                    block(cpu, 0x10518c20, 7);
                    cpu.set({{3, object}, {4, entered}, {5, club}, {6, 0}, {7, 0}});
                    cpu.set(31, ++entered);
                    cpu.call(0x104a60e0, 0x10518c3c);
                    block(cpu, 0x10518c3c, 4);
                    const auto required = std::int32_t(league.club_count());
                    cpu.set({{0, std::uint32_t(required)}, {3, std::uint32_t(std::int8_t(entered))}});
                    if (compare(cpu, std::int8_t(entered), required) == 0) break;
                }
            }
            block(cpu, 0x10518c4c, 2);
            cpu.set(28, ++index);
            club += club_size;
            cpu.set(29, club);
        }
        block(cpu, 0x10518c64, 4);
        const auto required = std::int32_t(league.club_count());
        cpu.set({{0, std::uint32_t(required)}, {3, std::uint32_t(std::int8_t(entered))}});
        const bool complete = compare(cpu, std::int8_t(entered), required) == 0;
        block(cpu, complete ? 0x10518c7c : 0x10518c74, complete ? 1 : 2);
        cpu.set(3, complete);
    }
    return_multiple<26>(cpu, 0x10518c80, 0x80);
}

struct RoundDate { unsigned day, month; bool midweek; };
constexpr std::array<RoundDate, 27> regular_rounds{{
    {5,7,false}, {8,7,true}, {12,7,false}, {15,7,true}, {19,7,false}, {26,7,false}, {29,7,true},
    {2,8,false}, {5,8,true}, {9,8,false}, {16,8,false}, {19,8,true}, {23,8,false}, {30,8,false},
    {3,9,true}, {7,9,false}, {10,9,true}, {14,9,false}, {21,9,false}, {28,9,false},
    {4,10,false}, {7,10,true}, {11,10,false}, {14,10,true}, {18,10,false}, {25,10,false}, {2,11,false},
}};

void regular_calendar(Cpu& cpu, Division league, std::uint32_t rows) {
    auto pc = 0x10517cb4u;
    for (unsigned round = 0; round < regular_rounds.size(); ++round) {
        const auto date = regular_rounds[round];
        const auto instructions = round == 0 ? 10u : 11u;
        block(cpu, pc, instructions);
        cpu.set(0, 0);
        if (round) cpu.set(3, rows);
        // The date builder reads this ninth argument in its caller's frame.
        cpu.put32(cpu.sp() + 0x38, 0);
        cpu.set({{4, round}, {5, date.day}, {6, date.month}});
        if (!round) cpu.set(7, 0);
        cpu.set(10, std::uint32_t(league.year()));
        cpu.set({{7, 0}, {8, date.midweek ? 2u : 6u}, {9, date.midweek ? 2u : 1u}});
        pc += instructions * 4;
        cpu.call(0x104a9810, pc);
        const unsigned extra = !date.midweek && round + 1 < regular_rounds.size() ? 2 : 0;
        for (unsigned slot = 0; slot <= extra; ++slot) {
            // One call setup straddles generated pages. The original can
            // stop at the second page with just the first four arguments set.
            const bool page_split = (pc & 0xfff) == 0xff0;
            block(cpu, pc, page_split ? 4 : 8);
            const bool scheduled = slot < extra;
            cpu.set({{3, rows}, {4, round}, {5, slot}, {6, scheduled ? 5u : UINT32_MAX}});
            if (page_split) block(cpu, pc + 16, 4);
            cpu.set({{7, scheduled ? 1u : UINT32_MAX}, {8, scheduled ? 1u : UINT32_MAX}, {9, 0}});
            pc += 32;
            cpu.call(0x104a9890, pc);
        }
    }
    block(cpu, pc, 2);
    cpu.set(3, rows);
}

void playoff_date(Cpu& cpu, Division league, std::uint32_t rows, std::uint32_t pc,
                  unsigned round, unsigned day) {
    block(cpu, pc, 11);
    cpu.set({{0, 0}, {3, rows}});
    cpu.put32(cpu.sp() + 0x38, 0);
    cpu.set({{4, round}, {5, day}, {6, 11}});
    cpu.set(10, std::uint32_t(league.year()));
    cpu.set({{7, 0}, {8, 2}, {9, 2}});
    cpu.call(0x10485570, pc + 44);
}

void playoff_calendar(Cpu& cpu, Division league, std::uint32_t rows) {
    block(cpu, 0x1051898c, 7);
    cpu.set(9, std::uint32_t(league.year()));
    cpu.set({{4, 0}, {5, 3}, {6, 11}, {7, 0}, {8, 0}});
    cpu.call(0x104854f0, 0x105189a8);
    playoff_date(cpu, league, rows, 0x105189a8, 0, 5);

    block(cpu, 0x105189d4, 33);
    // The playoff builder consumes the first round before the second round
    // receives its own rule fields. Keep the original store order.
    cpu.set({{0, 0x82}, {12, 0}});
    cpu.put16(rows + 8, 0x82);
    cpu.set({{3, 8}, {6, 0x200}, {11, 5}});
    cpu.put16(rows + 0xa, 0);
    cpu.set({{10, 4}, {9, 2}, {0, 1}});
    cpu.put16(rows + 0xc, 8);
    cpu.set({{3, rows}, {4, 1}, {5, 6}});
    cpu.put16(rows + 0xe, 0x200);
    cpu.set({{6, 11}, {7, 0}, {8, 3}});
    cpu.put8(rows + 0x18, 5);
    cpu.put16(rows + 0x1a, 4);
    cpu.put16(rows + 0x1c, 2);
    cpu.put16(rows + 0x1e, 4);
    cpu.put16(rows + 0x10, 1);
    cpu.put16(rows + 0x12, 0);
    cpu.put16(rows + 0x20, 0);
    cpu.put8(rows + 0x22, 0);
    cpu.put8(rows + 0x23, 2);
    cpu.put8(rows + 0x24, 4);
    cpu.put32(rows + 0x6c, 0);
    cpu.put32(rows + 0x70, 0);
    cpu.put32(rows + 0x74, 0);
    cpu.set(9, std::uint32_t(league.year()));
    cpu.call(0x104854f0, 0x10518a58);
    playoff_date(cpu, league, rows, 0x10518a58, 1, 12);

    block(cpu, 0x10518a84, 29);
    constexpr auto round_size = 0x78u;
    const auto final = rows + round_size;
    cpu.set({{0, 0x96}, {3, 0x80000}});
    cpu.put16(final + 8, 0x96);
    cpu.set({{9, 1}, {0, 8}, {8, 0x200}});
    cpu.put16(final + 0xa, 1);
    cpu.set({{7, 5}, {6, 2}, {5, 0}});
    cpu.put16(final + 0xc, 8);
    constexpr auto prize = 500000u;
    cpu.set({{4, 4}, {0, prize}, {3, rows}});
    cpu.put16(final + 0xe, 0x200);
    cpu.put8(final + 0x18, 5);
    cpu.put16(final + 0x1a, 2);
    cpu.put16(final + 0x1c, 1);
    cpu.put16(final + 0x1e, 0);
    cpu.put16(final + 0x10, 1);
    cpu.put16(final + 0x12, 0);
    cpu.put16(final + 0x20, 0);
    cpu.put8(final + 0x22, 0);
    cpu.put8(final + 0x23, 2);
    cpu.put8(final + 0x24, 4);
    cpu.put32(final + 0x6c, 0);
    cpu.put32(final + 0x70, prize);
    cpu.put32(final + 0x74, 0);
}

void calendar(Cpu& cpu) {
    GuestNativeScope scope(cpu.s, calendar_entry);
    block(cpu, calendar_entry, 11);
    const auto stack = cpu.sp(), frame = stack - 0xb0;
    const Division league{cpu, cpu.r(3)};
    const auto phase = std::int8_t(cpu.r(4));
    const auto count = cpu.r(5), start = cpu.r(6), regular = cpu.r(7);
    cpu.set(0, cpu.lr());
    guest::transfer_saved_registers<27>(cpu, stack);
    const auto success = cpu.toc(-0x7bac);
    cpu.set({{31, success}, {29, league.address}});
    const auto message = cpu.toc(-0x7ba8);
    cpu.set(27, message);
    const auto source = cpu.toc(-0x4550);
    cpu.set(28, source);
    cpu.put32(stack + 8, cpu.r(0));
    cpu.set(0, std::uint32_t(phase));
    const bool is_regular = compare(cpu, phase, -1) == 0;
    cpu.put32(frame, stack);
    cpu.set(1, frame);
    scope.frame(frame);
    if (!is_regular) {
        block(cpu, 0x105188fc, 2);
        if (record_order(cpu, phase) != 0 || league.table_only()) {
            block(cpu, 0x10518af8, 1);
            cpu.set(3, 0);
            return_multiple<27>(cpu, 0x10518afc, 0xb0);
            return;
        }
    }

    const auto gate = is_regular ? 0x10517c2cu : 0x10518904u;
    block(cpu, gate, 2);
    if (compare_logical(cpu, regular) != 0) {
        block(cpu, gate + 8, 2);
        cpu.set(0, is_regular);
        cpu.put8(regular, is_regular);
    }
    block(cpu, gate + 16, 6);
    const auto rounds = is_regular ? 27u : 2u;
    const auto bytes = is_regular ? 0x804u : 0xf0u;
    cpu.set({{3, rounds}, {0, 0}});
    cpu.put16(count, rounds);
    cpu.set(3, bytes);
    cpu.put16(start, 0);
    const auto resume = gate + 40;
    cpu.call(0x10775130, resume);
    block(cpu, resume, 2);
    const auto rows = cpu.r(3);
    cpu.set(30, rows);
    if (record_order(cpu, std::int32_t(rows)) == 0) {
        assertion(cpu, resume + 8, source, message, is_regular ? 0x70 : 0x50,
                  is_regular ? 0x48 : 0x40, is_regular ? 0xb0 : 0x159);
        block(cpu, resume + 80, 4);
        cpu.set({{0, 0}, {3, 0}});
        cpu.put8(success, 0);
    } else if (is_regular) {
        regular_calendar(cpu, league, rows);
    } else {
        playoff_calendar(cpu, league, rows);
    }
    return_multiple<27>(cpu, 0x10518afc, 0xb0);
}

void call_owned(Cpu& cpu, std::uint32_t target, std::uint32_t resume, void (*owner)(Cpu&)) {
    if (routine_checker || dispatch_probe || cpu.s.scalar_capture) {
        cpu.call(target, resume);
        return;
    }
    const auto stack = cpu.sp();
    cpu.link(resume);
    cpu.s.pc = target;
    owner(cpu);
    if (cpu.s.pc != resume || cpu.sp() != stack)
        throw std::logic_error("Brazilian Second Division owner returned outside its call boundary");
}

void call_virtual_owner(Cpu& cpu, std::uint32_t resume, std::uint32_t owned_entry, void (*owner)(Cpu&)) {
    if (routine_checker || cpu.s.import_descriptor_observer || cpu.s.scalar_capture || dispatch_probe) {
        cpu.call(call_descriptor, resume);
        return;
    }
    const auto stack = cpu.sp();
    cpu.link(resume);
    cpu.s.pc = call_descriptor;
    if (!try_modern_import_descriptor(cpu.s, call_descriptor) && !dispatch_page(cpu.s))
        throw std::logic_error("Missing Brazilian Second Division descriptor entry");
    if (cpu.s.pc == owned_entry) owner(cpu);
    else run_until(cpu.s, resume);
    if (cpu.s.pc != resume || cpu.sp() != stack)
        throw std::logic_error("Brazilian Second Division callback returned outside its call boundary");
}

void select_virtual(Cpu& cpu, std::uint32_t object, unsigned offset) {
    cpu.set(12, cpu.u32(object));
    cpu.set(12, cpu.u32(cpu.r(12) + offset));
}

void configure(Cpu& cpu) {
    GuestNativeScope scope(cpu.s, configure_entry);
    block(cpu, configure_entry, 42);
    const auto stack = cpu.sp(), frame = stack - 0x50;
    const auto address = cpu.r(3);
    cpu.set(0, cpu.lr());
    cpu.put32(stack - 4, cpu.r(31));
    cpu.set({{31, 3}, {12, 2}});
    cpu.put32(stack - 8, cpu.r(30));
    cpu.set({{30, 1}, {11, 0}, {10, 4}});
    cpu.put32(stack - 12, cpu.r(29));
    cpu.set(29, address);
    const auto promotion = cpu.toc(-0x6b88);
    cpu.set({{9, promotion}, {5, address + 0xa6}});
    cpu.put32(stack + 8, cpu.r(0));
    cpu.set(0, 7);
    const auto relegation = cpu.toc(-0x6b48);
    cpu.set({{8, relegation}, {6, address + 0x3a}});
    cpu.put32(frame, stack);
    cpu.set(1, frame);
    scope.frame(frame);
    cpu.set({{4, UINT32_MAX}, {7, 0}});
    cpu.put16(address + 0x3c, 1);
    constexpr std::array<unsigned, 6> placement{3, 1, 2, 3, 1, 0};
    for (unsigned n = 0; n < placement.size(); ++n) {
        cpu.put8(address + 0xc4 + n, placement[n]);
        if (n == 2) cpu.put8(address + 0x42, 1);
    }
    constexpr std::array<unsigned, 4> stage_settings{4, 0, 0, 4};
    for (unsigned n = 0; n < stage_settings.size(); ++n)
        cpu.put8(address + 0xc0 + n, stage_settings[n]);
    cpu.set(9, cpu.u32(promotion));
    cpu.put32(address + 0x1c, cpu.r(9));
    cpu.set(8, cpu.u32(relegation));
    cpu.put32(address + 0x20, cpu.r(8));
    // The base classification uses bit 0 to declare the table leader champion.
    cpu.put16(address + 0xdc, Division{cpu, address}.table_only() ? 3 : 2);
    cpu.put8(address + 0x4c, 2);
    cpu.put8(address + 0x46, 7);
    cpu.put8(address + 0x47, 3);
    cpu.set(12, cpu.u32(address));
    cpu.set(12, cpu.u32(cpu.r(12) + 0x44));

    constexpr auto resume = 0x10518d48u;
    call_virtual_owner(cpu, resume, calendar_entry, calendar);
    block(cpu, resume, 10);
    cpu.set(2, cpu.u32(frame + 0x14));
    cpu.put32(address + 0xbc, cpu.r(3));
    cpu.set(3, 1);
    cpu.set(0, cpu.u32(frame + 0x58));
    cpu.set(1, stack);
    cpu.set_lr(cpu.r(0));
    for (unsigned saved = 0; saved < 3; ++saved)
        cpu.set(31 - saved, cpu.u32(stack - (saved + 1) * 4));
    cpu.finish();
}

void destroy_groups(Cpu& cpu, Division league, std::uint32_t pc,
                    unsigned counter_register, unsigned offset_register) {
    block(cpu, pc, 3);
    unsigned index = 0, offset = 0;
    cpu.set({{counter_register, index}, {offset_register, offset}});
    for (;;) {
        block(cpu, pc + 60, 3);
        const auto final_group = league.last_group();
        cpu.set(0, std::uint32_t(final_group));
        if (compare(cpu, std::int32_t(index), final_group) > 0) break;
        block(cpu, pc + 16, 4);
        cpu.set(3, league.groups());
        const auto group = cpu.u32(cpu.r(3) + offset);
        cpu.set(3, group);
        if (compare_logical(cpu, group) != 0) {
            block(cpu, pc + 32, 4);
            cpu.set(12, cpu.u32(group));
            cpu.set(4, 1);
            cpu.set(12, cpu.u32(cpu.r(12) + 8));
            cpu.call(call_descriptor, pc + 48);
            block(cpu, pc + 48, 1);
            cpu.set(2, cpu.u32(cpu.sp() + 0x14));
        }
        block(cpu, pc + 52, 2);
        offset += 4;
        cpu.set({{offset_register, offset}, {counter_register, ++index}});
    }
}

void destroy_competition(Cpu& cpu) {
    GuestNativeScope scope(cpu.s, destroy_entry);
    block(cpu, destroy_entry, 10);
    const auto stack = cpu.sp(), frame = stack - 0x50;
    const auto address = cpu.r(3);
    const auto delete_flag = cpu.r(4);
    const Division league{cpu, address};
    cpu.set(0, cpu.lr());
    cpu.put32(stack - 4, cpu.r(31));
    cpu.put32(stack - 8, cpu.r(30));
    cpu.put32(stack - 12, cpu.r(29));
    cpu.set(29, delete_flag);
    cpu.put32(stack - 16, cpu.r(28));
    cpu.set(28, address);
    const bool present = record_order(cpu, std::int32_t(address)) != 0;
    cpu.put32(stack + 8, cpu.r(0));
    cpu.put32(frame, stack);
    cpu.set(1, frame);
    scope.frame(frame);

    if (present) {
        block(cpu, 0x10517b18, 4);
        cpu.set(0, cpu.toc(-0x6044));
        cpu.set(4, 0);
        cpu.put32(address, cpu.r(0));
        cpu.call(0x104a6660, 0x10517b28);
        block(cpu, 0x10517b28, 3);
        const std::uint32_t rows = league.calendar_rows();
        cpu.set(3, rows);
        if (compare_logical(cpu, rows) != 0) {
            block(cpu, 0x10517b34, 1);
            cpu.call(0x10775170, 0x10517b38);
            block(cpu, 0x10517b38, 2);
            cpu.set(0, 0);
            league.calendar_rows() = 0;
        }

        destroy_groups(cpu, league, 0x10517b40, 30, 31);

        block(cpu, 0x10517b88, 3);
        const auto groups = league.groups();
        cpu.set(3, groups);
        if (compare_logical(cpu, groups) != 0) {
            block(cpu, 0x10517b94, 1);
            cpu.call(0x10775170, 0x10517b98);
            block(cpu, 0x10517b98, 2);
            cpu.set(0, 0);
            league.group_storage() = 0;
        }
        block(cpu, 0x10517ba0, 3);
        const auto fixtures = league.fixtures();
        cpu.set(3, fixtures);
        if (compare_logical(cpu, fixtures) != 0) {
            block(cpu, 0x10517bac, 2);
            cpu.set(4, 1);
            cpu.call(0x104536c0, 0x10517bb4);
        }
        block(cpu, 0x10517bb4, 3);
        cpu.set({{3, address}, {4, 0}});
        cpu.call(0x1049e600, 0x10517bc0);
        block(cpu, 0x10517bc0, 2);
        const auto flag = std::int32_t(std::int16_t(delete_flag));
        cpu.set(0, std::uint32_t(flag));
        if (record_order(cpu, flag) > 0) {
            block(cpu, 0x10517bc8, 2);
            cpu.set(3, address);
            cpu.call(0x107735a0, 0x10517bd0);
        }
    }
    // The return-value move shares the epilogue's accounting block.
    block(cpu, 0x10517bd0, 9);
    cpu.set(3, address);
    cpu.set(0, cpu.u32(stack + 8));
    cpu.set(1, stack);
    cpu.set_lr(cpu.r(0));
    for (unsigned saved = 0; saved < 4; ++saved)
        cpu.set(31 - saved, cpu.u32(stack - (saved + 1) * 4));
    cpu.finish();
}

void construct_competition(Cpu& cpu) {
    GuestNativeScope scope(cpu.s, construct_entry);
    block(cpu, construct_entry, 12);
    const auto stack = cpu.sp(), frame = stack - 0x120, object_slot = frame + 0x138;
    const auto year = cpu.r(4), definition = cpu.r(5);
    cpu.set(0, cpu.lr());
    guest::transfer_saved_registers<27>(cpu, stack);
    const auto success = cpu.toc(-0x7bac);
    cpu.set({{29, success}, {27, year}});
    const auto message = cpu.toc(-0x7ba8);
    cpu.set({{30, message}, {28, definition}});
    const auto source = cpu.toc(-0x4550);
    cpu.set(31, source);
    cpu.put32(stack + 8, cpu.r(0));
    cpu.put32(frame, stack);
    cpu.set(1, frame);
    scope.frame(frame);
    cpu.put32(object_slot, cpu.r(3));

    // The original homes the object above the frame and reloads it around
    // calls and between writes. Other fields may alias that home slot.
    const auto object = [&](unsigned argument = 3) {
        const auto address = cpu.u32(object_slot);
        cpu.set(argument, address);
        return address;
    };
    const auto fail = [&](std::uint32_t pc, unsigned filename, unsigned extension, unsigned line) {
        assertion(cpu, pc, source, message, filename, extension, line);
        block(cpu, pc + 72, 4);
        cpu.set(0, 0);
        cpu.put8(success, 0);
        object();
    };
    const auto initialize = [&] {
        object();
        cpu.call(0x1049e4f0, 0x105177e0);
        block(cpu, 0x105177e0, 15);
        const auto table = cpu.toc(-0x6044);
        cpu.set({{5, table}, {0, 6}});
        const auto address = object();
        cpu.set(4, 1);
        cpu.put32(address, table);
        cpu.put16(object() + 0x40, year);
        cpu.put32(object() + 4, definition);
        cpu.put8(object() + 0x4a, 6);
        select_virtual(cpu, object(), 0x20);
        cpu.call(call_descriptor, 0x1051781c);
        block(cpu, 0x1051781c, 3);
        cpu.set(2, cpu.u32(frame + 0x14));
        if (compare(cpu, std::int32_t(cpu.r(3))) != 0) {
            block(cpu, 0x10517828, 2);
            object();
            return;
        }

        block(cpu, 0x10517830, 15);
        const auto current = object();
        const auto capacity = original_rules ? 1u : 0u;
        cpu.set({{5, 10}, {4, UINT32_MAX}, {0, capacity}});
        cpu.put8(current + 0x4b, 10);
        cpu.put8(object() + 0x44, 0xff);
        cpu.put32(object() + 0x30, UINT32_MAX);
        cpu.put32(object() + 0x2c, capacity);
        const auto group_capacity = cpu.i32(object() + 0x2c);
        cpu.set(0, std::uint32_t(group_capacity));
        if (compare(cpu, group_capacity) > 0) {
            block(cpu, 0x1051786c, 2);
            cpu.set(3, std::uint32_t(group_capacity) * 4);
            cpu.call(0x10775130, 0x10517874);
            block(cpu, 0x10517874, 3);
            cpu.put32(object(4) + 0xc, cpu.r(3));
        } else {
            block(cpu, 0x10517880, 2);
            cpu.set(0, 0);
            cpu.put32(cpu.r(3) + 0xc, 0);
        }
        block(cpu, 0x10517888, 4);
        select_virtual(cpu, object(), 0x94);
        call_virtual_owner(cpu, 0x10517898, configure_entry, configure);
        block(cpu, 0x10517898, 3);
        cpu.set(2, cpu.u32(frame + 0x14));
        if (compare(cpu, std::int32_t(cpu.r(3))) == 0) return fail(0x105178a4, 0xe0, 0x58, 0x50);

        block(cpu, 0x105178fc, 2);
        object();
        call_owned(cpu, collect_entry, 0x10517904, collect_clubs);
        block(cpu, 0x10517904, 2);
        if (compare(cpu, std::int32_t(cpu.r(3))) == 0) return fail(0x1051790c, 0xc0, 0x50, 0x57);

        block(cpu, 0x10517964, 2);
        object();
        cpu.call(0x104a00a0, 0x1051796c);
        block(cpu, 0x1051796c, 2);
        if (compare(cpu, std::int32_t(cpu.r(3))) == 0) return fail(0x10517974, 0xa0, 0x48, 0x5e);

        block(cpu, 0x105179cc, 3);
        object();
        cpu.set(4, 0);
        cpu.call(0x1049eb30, 0x105179d8);
        block(cpu, 0x105179d8, 2);
        if (compare(cpu, std::int32_t(cpu.r(3))) == 0) return fail(0x105179e0, 0x80, 0x40, 0x67);

        block(cpu, 0x10517a38, 2);
        cpu.set(3, 0x634);
        cpu.call(0x10773520, 0x10517a40);
        block(cpu, 0x10517a40, 2);
        const auto fixtures = cpu.r(3);
        cpu.set(27, fixtures);
        if (record_order(cpu, std::int32_t(fixtures)) != 0) {
            block(cpu, 0x10517a48, 2);
            object(4);
            cpu.call(0x104533a0, 0x10517a50);
        }
        block(cpu, 0x10517a50, 6);
        cpu.put32(object() + 8, fixtures);
        const auto stored = cpu.u32(object() + 8);
        cpu.set(0, stored);
        if (compare_logical(cpu, stored) == 0) return fail(0x10517a68, 0x60, 0x38, 0x6e);

        block(cpu, 0x10517ac0, 3);
        select_virtual(cpu, cpu.r(3), 0x64);
        cpu.call(call_descriptor, 0x10517acc);
        block(cpu, 0x10517acc, 2);
        cpu.set(2, cpu.u32(frame + 0x14));
        object();
    };
    initialize();
    return_multiple<27>(cpu, 0x10517ad4, 0x120);
}

void next_season(Cpu& cpu) {
    GuestNativeScope scope(cpu.s, next_season_entry);
    block(cpu, next_season_entry, 12);
    const auto stack = cpu.sp(), frame = stack - 0xf0;
    const auto address = cpu.r(3);
    const Division league{cpu, address};
    cpu.set(0, cpu.lr());
    guest::transfer_saved_registers<26>(cpu, stack);
    const auto success = cpu.toc(-0x7bac);
    cpu.set({{31, success}, {26, address}});
    const auto message = cpu.toc(-0x7ba8);
    cpu.set({{29, message}, {4, 0}});
    const auto source = cpu.toc(-0x4550);
    cpu.set(30, source);
    cpu.put32(stack + 8, cpu.r(0));
    cpu.set(0, 0);
    cpu.put32(frame, stack);
    cpu.set(1, frame);
    scope.frame(frame);
    cpu.put8(address + 0x49, 0);
    cpu.call(0x104a6660, 0x10518da0);

    block(cpu, 0x10518da0, 3);
    const std::uint32_t rows = league.calendar_rows();
    cpu.set(3, rows);
    if (compare_logical(cpu, rows) != 0) {
        block(cpu, 0x10518dac, 1);
        cpu.call(0x10775170, 0x10518db0);
        block(cpu, 0x10518db0, 2);
        cpu.set(0, 0);
        league.calendar_rows() = 0;
    }
    block(cpu, 0x10518db8, 3);
    const auto fixtures = league.fixtures();
    cpu.set(3, fixtures);
    if (compare_logical(cpu, fixtures) != 0) {
        block(cpu, 0x10518dc4, 2);
        cpu.set(4, 1);
        cpu.call(0x104564a0, 0x10518dcc);
    }
    destroy_groups(cpu, league, 0x10518dcc, 27, 28);

    if (!original_rules) {
        // The original saves own this array until the last legacy group
        // has been destroyed. The next season needs no stage storage.
        const auto groups = league.groups();
        if (groups) {
            cpu.set(3, groups);
            cpu.call(0x10775170, 0x10518e14);
            league.group_storage() = 0;
        }
        league.group_capacity() = 0;
    }

    const auto fail = [&](std::uint32_t pc, unsigned filename, unsigned extension, unsigned line) {
        assertion(cpu, pc, source, message, filename, extension, line);
        block(cpu, pc + 72, 3);
        cpu.set(0, 0);
        cpu.put8(success, 0);
    };
    const auto build_new_season = [&] {
        block(cpu, 0x10518e14, 9);
        cpu.set({{0, UINT32_MAX}, {3, address}});
        cpu.put32(address + 0x30, UINT32_MAX);
        const auto year = std::uint32_t(league.year());
        cpu.set({{4, year}, {0, year + 1}});
        cpu.put16(address + 0x40, year + 1);
        select_virtual(cpu, address, 0x94);
        call_virtual_owner(cpu, 0x10518e38, configure_entry, configure);
        block(cpu, 0x10518e38, 3);
        cpu.set(2, cpu.u32(frame + 0x14));
        if (compare(cpu, std::int32_t(cpu.r(3))) == 0) return fail(0x10518e44, 0xb8, 0x50, 0x20c);

        block(cpu, 0x10518e98, 2);
        cpu.set(3, address);
        call_owned(cpu, collect_entry, 0x10518ea0, collect_clubs);
        block(cpu, 0x10518ea0, 2);
        if (compare(cpu, std::int32_t(cpu.r(3))) == 0) return fail(0x10518ea8, 0x98, 0x48, 0x213);

        block(cpu, 0x10518efc, 2);
        cpu.set(3, address);
        cpu.call(0x104a00a0, 0x10518f04);
        block(cpu, 0x10518f04, 2);
        if (compare(cpu, std::int32_t(cpu.r(3))) == 0) return fail(0x10518f0c, 0x78, 0x40, 0x219);

        block(cpu, 0x10518f60, 3);
        cpu.set({{3, address}, {4, 0}});
        cpu.call(0x1049eb30, 0x10518f6c);
        block(cpu, 0x10518f6c, 2);
        if (compare(cpu, std::int32_t(cpu.r(3))) == 0) return fail(0x10518f74, 0x58, 0x38, 0x220);

        block(cpu, 0x10518fc8, 4);
        cpu.set(3, address);
        select_virtual(cpu, address, 0x64);
        cpu.call(call_descriptor, 0x10518fd8);
        block(cpu, 0x10518fd8, 3);
        cpu.set(2, cpu.u32(frame + 0x14));
        cpu.set(3, address);
        cpu.call(0x104aa3a0, 0x10518fe4);
        block(cpu, 0x10518fe4, 4);
        const auto manager = cpu.toc(-0x7428);
        cpu.set(3, manager);
        cpu.set(4, cpu.u32(address + 4));
        cpu.set(3, cpu.u32(manager));
        cpu.call(0x102d46c0, 0x10518ff4);
    };
    build_new_season();

    // The multiword restore starts the next original page.
    block(cpu, 0x10518ff4, 3);
    cpu.set(0, cpu.u32(stack + 8));
    cpu.set(1, stack);
    cpu.set_lr(cpu.r(0));
    block(cpu, 0x10519000, 2);
    guest::transfer_saved_registers<26, true>(cpu, stack);
    cpu.finish();
}

void create_playoff_group(Cpu& cpu) {
    GuestNativeScope scope(cpu.s, create_group_entry);
    block(cpu, create_group_entry, 10);
    const auto stack = cpu.sp(), frame = stack - 0xd0;
    const Division league{cpu, cpu.r(3)};
    cpu.set(0, cpu.lr());
    guest::transfer_saved_registers<25>(cpu, stack);
    cpu.set(27, league.address);
    const auto success = cpu.toc(-0x7bac);
    cpu.set(29, success);
    const auto message = cpu.toc(-0x7ba8);
    cpu.set({{30, message}, {3, 16}});
    const auto source = cpu.toc(-0x4550);
    cpu.set(31, source);
    cpu.put32(stack + 8, cpu.r(0));
    cpu.put32(frame, stack);
    cpu.set(1, frame);
    scope.frame(frame);
    cpu.call(0x10775130, 0x105190f8);

    block(cpu, 0x105190f8, 2);
    const auto seeds = cpu.r(3);
    cpu.set(28, seeds);
    if (record_order(cpu, std::int32_t(seeds)) == 0) {
        assertion(cpu, 0x10519100, source, message, 0x8c, 0x64, 0x25f);
        block(cpu, 0x10519148, 3);
        cpu.set(0, 0);
        cpu.put8(success, 0);
        return_multiple<25>(cpu, 0x105192a4, 0xd0);
        return;
    }

    block(cpu, 0x10519154, 20);
    cpu.set(4, league.standings());
    cpu.set({{3, league.address}, {5, frame + 0x5a}, {6, frame + 0x58}});
    cpu.set(0, cpu.u32(cpu.r(4)));
    cpu.set({{4, 0}, {7, 0}});
    cpu.put32(seeds, cpu.r(0));
    constexpr std::array<unsigned, 3> seed_order{3, 1, 2};
    for (unsigned n = 0; n < seed_order.size(); ++n) {
        cpu.set(8, league.standings());
        cpu.set(0, cpu.u32(cpu.r(8) + seed_order[n] * standing_size));
        cpu.put32(seeds + (n + 1) * 4, cpu.r(0));
    }
    select_virtual(cpu, league.address, 0x44);
    call_virtual_owner(cpu, 0x105191a4, calendar_entry, calendar);
    block(cpu, 0x105191a4, 5);
    cpu.set(2, cpu.u32(frame + 0x14));
    const auto rows = cpu.r(3);
    cpu.set({{0, rows}, {3, 0xb4}, {26, rows}});
    cpu.call(0x10773520, 0x105191b8);

    block(cpu, 0x105191b8, 2);
    const auto group = cpu.r(3);
    cpu.set(25, group);
    if (record_order(cpu, std::int32_t(group)) != 0) {
        block(cpu, 0x105191c0, 20);
        cpu.set({{8, 0}, {5, 1}});
        cpu.put32(frame + 0x38, 0);
        cpu.set({{0, 20}, {4, league.address}, {6, seeds}});
        cpu.put32(frame + 0x3c, 1);
        cpu.set({{9, rows}, {5, 4}});
        const auto first_round = std::uint32_t(cpu.i16(frame + 0x58));
        cpu.set(7, first_round);
        cpu.put32(frame + 0x40, first_round);
        cpu.put32(frame + 0x44, 20);
        for (unsigned offset = 0x48; offset <= 0x54; offset += 4)
            cpu.put32(frame + offset, 0);
        cpu.set(7, std::uint32_t(cpu.i16(frame + 0x5a)));
        cpu.set(8, cpu.u32(league.address + 4));
        cpu.set(10, std::uint32_t(league.year()));
        cpu.call(0x104bc3c0, 0x10519210);
    }

    block(cpu, 0x10519210, 4);
    const auto groups = league.groups();
    cpu.set({{4, groups}, {3, rows}});
    cpu.put32(groups, group);
    cpu.call(0x10775170, 0x10519220);
    block(cpu, 0x10519220, 4);
    cpu.set(3, league.groups());
    const auto stored_group = cpu.u32(cpu.r(3));
    cpu.set(3, stored_group);
    if (compare_logical(cpu, stored_group) == 0) {
        assertion(cpu, 0x10519230, source, message, 0x6c, 0x5c, 0x276);
        block(cpu, 0x10519278, 4);
        cpu.set({{0, 0}, {3, seeds}});
        cpu.put8(success, 0);
        cpu.call(0x10775170, 0x10519288);
        block(cpu, 0x10519288, 1);
    } else {
        block(cpu, 0x1051928c, 2);
        cpu.set(4, 0);
        cpu.call(0x1047ef50, 0x10519294);
        block(cpu, 0x10519294, 2);
        cpu.set(3, seeds);
        cpu.call(0x10775170, 0x1051929c);
        block(cpu, 0x1051929c, 2);
        cpu.set(0, 0);
        cpu.put32(league.address + 0x30, 0);
    }
    return_multiple<25>(cpu, 0x105192a4, 0xd0);
}

void advance_groups(Cpu& cpu) {
    GuestNativeScope scope(cpu.s, advance_groups_entry);
    block(cpu, advance_groups_entry, 12);
    const auto stack = cpu.sp(), frame = stack - 0x70;
    const Division league{cpu, cpu.r(3)};
    cpu.set(0, cpu.lr());
    cpu.put32(stack - 4, cpu.r(31));
    const auto source = cpu.toc(-0x4550);
    cpu.set(31, source);
    cpu.put32(stack - 8, cpu.r(30));
    const auto message = cpu.toc(-0x7ba8);
    cpu.set(30, message);
    cpu.put32(stack + 8, cpu.r(0));
    cpu.put32(frame, stack);
    cpu.set(1, frame);
    scope.frame(frame);
    const auto max_group = cpu.u32(league.address + 0x2c) - 1;
    cpu.set(4, max_group + 1);
    const auto previous = league.last_group();
    cpu.set({{5, std::uint32_t(previous)}, {0, max_group}});
    if (compare(cpu, previous, std::int32_t(max_group)) >= 0) {
        assertion(cpu, 0x10519040, source, message, 0x40, 0x38, 0x23d);
        block(cpu, 0x10519088, 4);
        const auto success = cpu.toc(-0x7bac);
        cpu.set({{3, success}, {0, 0}});
        cpu.put8(success, 0);
    } else {
        block(cpu, 0x10519098, 5);
        cpu.set(0, std::uint32_t(previous) + 1);
        cpu.put32(league.address + 0x30, cpu.r(0));
        const auto group = league.last_group();
        cpu.set(0, std::uint32_t(group));
        if (compare(cpu, group) == 0) {
            block(cpu, 0x105190b0, 1);
            call_owned(cpu, create_group_entry, 0x105190b4, create_playoff_group);
        } else {
            block(cpu, 0x105190ac, 1);
        }
    }
    block(cpu, 0x105190b4, 6);
    cpu.set(0, cpu.u32(stack + 8));
    cpu.set(1, stack);
    cpu.set_lr(cpu.r(0));
    cpu.set(31, cpu.u32(stack - 4));
    cpu.set(30, cpu.u32(stack - 8));
    cpu.finish();
}

void record_result(Cpu& cpu) {
    if (Division{cpu, cpu.r(3)}.table_only()) {
        // Use the league history handler, including the table champion event.
        // Forward without an additional guest frame or change to call arguments.
        cpu.s.pc = base_result_history;
        run_until(cpu.s, cpu.lr());
        return;
    }
    GuestNativeScope scope(cpu.s, record_result_entry);
    block(cpu, record_result_entry, 13);
    const auto stack = cpu.sp(), frame = stack - 0x50;
    const auto division = cpu.r(3), club = cpu.r(4);
    const auto outcome = std::int32_t(std::int8_t(cpu.r(5)));
    const auto phase = std::int32_t(std::int8_t(cpu.r(6)));
    cpu.set(0, cpu.lr());
    cpu.put32(stack - 4, cpu.r(31));
    cpu.put32(stack - 8, cpu.r(30));
    const auto history = cpu.toc(-0x7b48);
    cpu.set(30, history);
    cpu.put32(stack - 12, cpu.r(29));
    cpu.set(29, club);
    cpu.put32(stack - 16, cpu.r(28));
    cpu.set(28, division);
    cpu.put32(stack + 8, cpu.r(0));
    cpu.set(0, std::uint32_t(phase));
    const bool regular = compare(cpu, phase, -1) == 0;
    cpu.put32(frame, stack);
    cpu.set(1, frame);
    scope.frame(frame);

    const auto report_regular_result = [&] {
        block(cpu, 0x105192f4, 2);
        cpu.set(0, std::uint32_t(outcome));
        if (record_order(cpu, outcome) == 0) {
            block(cpu, 0x105192fc, 4);
            cpu.set(3, cpu.u32(history));
            cpu.set(6, 100);
            cpu.set(5, cpu.u32(division + 4));
            cpu.call(0x103822e0, 0x1051930c);
            block(cpu, 0x1051930c, 1);
            return false;
        }
        block(cpu, 0x10519310, 3);
        if (compare(cpu, outcome, 3) != 0) return false;
        block(cpu, 0x1051931c, 3);
        cpu.set(3, cpu.u32(history));
        cpu.set(5, cpu.u32(division + 4));
        cpu.call(0x10382e50, 0x10519328);
        block(cpu, 0x10519328, 2);
        cpu.set(3, 0);
        return true;
    };
    const auto report_playoff_result = [&] {
        block(cpu, 0x10519330, 2);
        if (record_order(cpu, phase) != 0) return;
        block(cpu, 0x10519338, 3);
        cpu.set(0, std::uint32_t(outcome));
        if (compare(cpu, outcome, 1) != 0) return;
        block(cpu, 0x10519344, 2);
        cpu.set(5, 0);
        cpu.call(0x104aa560, 0x1051934c);
        block(cpu, 0x1051934c, 5);
        const auto standing = cpu.r(3);
        cpu.set(31, standing);
        cpu.set(3, cpu.u32(history));
        cpu.set(5, cpu.u32(division + 4));
        cpu.set(4, club);
        cpu.call(0x10381b70, 0x10519360);
        block(cpu, 0x10519360, 2);
        cpu.set(0, 5);
        cpu.put8(standing + 0x40, 5);
    };
    bool returned = false;
    if (regular) returned = report_regular_result();
    else report_playoff_result();
    if (!returned) {
        block(cpu, 0x10519368, 1);
        cpu.set(3, 0);
    }
    block(cpu, 0x1051936c, 8);
    cpu.set(0, cpu.u32(stack + 8));
    cpu.set(1, stack);
    cpu.set_lr(cpu.r(0));
    for (unsigned saved = 0; saved < 4; ++saved)
        cpu.set(31 - saved, cpu.u32(stack - (saved + 1) * 4));
    cpu.finish();
}

void announce_result(Cpu& cpu) {
    GuestNativeScope scope(cpu.s, announce_result_entry);
    block(cpu, announce_result_entry, 14);
    const auto stack = cpu.sp(), frame = stack - 0x60;
    const auto division = cpu.r(3), club_index = cpu.r(4);
    const auto outcome = std::int32_t(std::int8_t(cpu.r(5)));
    const auto phase = std::int32_t(std::int8_t(cpu.r(6)));
    cpu.set(0, cpu.lr());
    guest::transfer_saved_registers<27>(cpu, stack);
    const auto text = cpu.toc(-0x7bb8);
    cpu.set({{28, text}, {30, division}});
    const auto clubs = cpu.toc(-0x7be8);
    cpu.set(29, clubs);
    const auto source = cpu.toc(-0x4550);
    cpu.set(10, source);
    cpu.put32(stack + 8, cpu.r(0));
    cpu.set(0, std::uint32_t(phase));
    const bool regular = compare(cpu, phase, -1) == 0;
    cpu.put32(frame, stack);
    cpu.set(1, frame);
    scope.frame(frame);
    const auto importance = cpu.u8(frame + 0x9b);
    cpu.set(12, importance);
    const auto personal = cpu.u8(frame + 0x9f);
    cpu.set(11, personal);
    const auto news = cpu.u32(frame + 0xa0);
    cpu.set(31, news);

    const auto forward = [&](std::uint32_t pc, unsigned priority) {
        block(cpu, pc, 5);
        cpu.put32(frame + 0x38, priority);
        cpu.set(10, 0);
        cpu.put32(frame + 0x3c, personal);
        cpu.put32(frame + 0x40, news);
        cpu.call(0x10450030, pc + 20);
        block(cpu, pc + 20, 1);
    };
    const auto format_news = [&](std::uint32_t pc, unsigned format) {
        block(cpu, pc, 16);
        const auto offset = club_index * club_size;
        cpu.set(27, offset);
        const auto club_base = cpu.u32(clubs);
        cpu.set(0, club_base);
        const auto definition = cpu.u32(division + 4);
        cpu.set({{6, definition}, {4, source + format}, {3, text}, {5, club_base + offset}});
        const auto competition_form = cpu.u8(definition + 0x67);
        cpu.set({{7, competition_form}, {9, offset + 0x221}});
        const auto club_form = cpu.u8(club_base + offset + 0x23b);
        cpu.set({{5, club_form}, {9, club_base + offset + 0x221}});
        const auto competition_word = std::uint32_t(std::int32_t(std::int8_t(competition_form)));
        const auto club_word = std::uint32_t(std::int32_t(std::int8_t(club_form)));
        cpu.set({{7, competition_word}, {10, definition + 0x4d}, {5, club_word}, {8, competition_word}, {6, club_word}});
        cpu.call(0x10727890, pc + 64);
        block(cpu, pc + 64, 3);
        cpu.set({{3, news}, {4, text}});
        cpu.call(0x1074b830, pc + 76);

        block(cpu, pc + 76, 6);
        const auto current_clubs = cpu.u32(clubs);
        const auto current_club = current_clubs + offset;
        cpu.set({{0, current_clubs}, {3, news}, {5, 0x7d5}, {6, current_club}, {4, current_club + 0x221}});
        cpu.call(0x1074bab0, pc + 100);
        block(cpu, pc + 100, 5);
        const auto current_definition = cpu.u32(division + 4);
        cpu.set({{6, current_definition}, {3, news}, {5, 0x7d0}, {4, current_definition + 0x4d}});
        cpu.call(0x1074bab0, pc + 120);
        block(cpu, pc + 120, 1);
    };
    const auto playoff_news = [&] {
        block(cpu, 0x105193e0, 2);
        if (record_order(cpu, phase) != 0) return;
        block(cpu, 0x105193e8, 2);
        const auto personal_flag = std::int32_t(std::int8_t(personal));
        cpu.set(0, std::uint32_t(personal_flag));
        if (record_order(cpu, personal_flag) != 0) return forward(0x105193f0, importance);
        block(cpu, 0x10519408, 3);
        cpu.set(0, std::uint32_t(outcome));
        if (compare(cpu, outcome, 1) == 0) return format_news(0x10519414, 0x26);
        block(cpu, 0x10519490, 2);
        if (compare(cpu, outcome, 2) == 0) return format_news(0x10519498, 0x54);

        block(cpu, 0x10519514, 6);
        cpu.set({{0, 0}, {10, 0}});
        cpu.put32(frame + 0x38, 0);
        cpu.put32(frame + 0x3c, personal);
        cpu.put32(frame + 0x40, news);
        cpu.call(0x10450030, 0x1051952c);
    };
    if (regular) forward(0x105193c8, importance);
    else playoff_news();
    return_multiple<27>(cpu, 0x1051952c, 0x60);
}

void update_first_group(Cpu& cpu) {
    if (Division{cpu, cpu.r(3)}.table_only()) {
        // The base finalizer writes first, second and third from the sorted
        // standings to the existing competition history record.
        cpu.s.pc = base_table_finalizer;
        run_until(cpu.s, cpu.lr());
        return;
    }
    block(cpu, update_group_entry, 8);
    cpu.set(0, cpu.lr());
    {
        guest::Frame frame(cpu, update_group_entry, 0x40, {});
        const Division division{cpu, cpu.r(3)};
        cpu.set(3, division.groups());
        const auto first_group = cpu.u32(cpu.r(3));
        cpu.set(3, first_group);
        const auto vtable = cpu.u32(first_group);
        cpu.set(12, vtable);
        cpu.set(12, cpu.u32(vtable + 0x38));
        cpu.call(call_descriptor, 0x10518b30);
        block(cpu, 0x10518b30, 5);
        cpu.set(2, cpu.u32(frame.at(0x14)));
    }
    cpu.finish();
}

void fixture_times(Cpu& cpu) {
    GuestNativeScope scope(cpu.s, fixture_times_entry);
    block(cpu, fixture_times_entry, 13);
    const auto stack = cpu.sp(), frame = stack - 0x50;
    const Division division{cpu, cpu.r(3)};
    const auto stage = std::int8_t(cpu.r(5));
    const auto weekday = cpu.r(6);
    auto first = cpu.r(7), second = cpu.r(8);

    // The descending saves and their reload order are visible to observers.
    cpu.set(0, cpu.lr());
    cpu.put32(stack - 4, cpu.r(31));
    cpu.set(31, second);
    cpu.put32(stack - 8, cpu.r(30));
    cpu.set(30, first);
    cpu.put32(stack - 12, cpu.r(29));
    cpu.set(29, weekday);
    cpu.put32(stack - 16, cpu.r(28));
    cpu.set(28, cpu.r(5));
    cpu.put32(stack + 8, cpu.r(0));
    cpu.put32(frame, stack);
    cpu.set(1, frame);
    scope.frame(frame);

    cpu.set(3, division.fixtures());
    cpu.call(fixture_record_lookup, 0x10519574);
    block(cpu, 0x10519574, 3);
    cpu.set(0, std::uint32_t(stage));
    if (compare(cpu, stage, -1) == 0 && !division.table_only()) {
        block(cpu, 0x10519580, 3);
        cpu.set(0, first & 0xff);
        if (compare_logical(cpu, first & 0xff, 5) < 0) {
            block(cpu, 0x1051958c, 1);
            first = 1;
            cpu.set(30, first);
        }
        block(cpu, 0x10519590, 3);
        cpu.set(0, second & 0xff);
        if (compare_logical(cpu, second & 0xff, 5) < 0) {
            block(cpu, 0x1051959c, 1);
            second = 4;
            cpu.set(31, second);
        }
    }
    block(cpu, 0x105195a0, 11);
    FixtureTimes{cpu, cpu.r(3)}.store(weekday, first, second);
    cpu.set(0, cpu.u32(frame + 0x58));
    cpu.set(1, stack);
    cpu.set_lr(cpu.r(0));
    for (unsigned saved = 0; saved < 4; ++saved)
        cpu.set(31 - saved, cpu.u32(stack - (saved + 1) * 4));
    cpu.finish();
}

void forward_base_result(Cpu& cpu) {
    block(cpu, base_result_entry, 4);
    cpu.set(0, cpu.lr());
    {
        guest::Frame frame(cpu, base_result_entry, 0x40, {});
        cpu.call(base_result_for_club, 0x105195e0);
        block(cpu, 0x105195e0, 4);
    }
    cpu.finish();
}

struct Routine { std::uint32_t entry; void (*run)(Cpu&); };
constexpr auto routines = std::to_array<Routine>({
#define BRA_NAT_SECOND_ROUTINE(entry, function) Routine{entry, function},
#include "bra_nat_second_routines.inc"
#undef BRA_NAT_SECOND_ROUTINE
});
}

void bra_nat_second_run_original(State& state, std::uint32_t entry) {
    run_original_body(state, entry, declining);
}
void bra_nat_second_run_rewrite(State& state, std::uint32_t entry) {
    const auto routine = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    if (routine == routines.end() || routine->entry != entry)
        throw std::logic_error("Unknown BRA_NAT_SECOND.CPP entry " + hex_address(entry));
    Cpu cpu(state);
    routine->run(cpu);
}
bool try_modern_bra_nat_second(State& state, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(state, entry, bra_nat_second_run_original,
                               bra_nat_second_run_rewrite, RoutineChecker::result_r3);
    bra_nat_second_run_rewrite(state, entry);
    return true;
}
}
