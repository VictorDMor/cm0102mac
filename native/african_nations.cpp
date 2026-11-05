#include "african_nations_adapter.hpp"
#include "guest_values.hpp"
#include "guest_saved_registers.hpp"
#include "routine_check.hpp"
#include "import_descriptor_adapter.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
using guest::compare;
using guest::compare_logical;
bool declining = std::getenv("CM_AFRICAN_NATIONS_ORIGINAL") != nullptr;
constexpr auto placement_entry = 0x104f7000u;
constexpr auto final_placement_entry = 0x104f6d50u;
constexpr auto notification_entry = 0x104f5da0u;
constexpr auto event_text_entry = 0x104f5910u;
constexpr auto destroy_entry = 0x104f4ee0u;
constexpr auto child_update_entry = 0x104f5d60u;
constexpr auto calendar_entry = 0x104f4ff0u;
constexpr auto configure_entry = 0x104f5c80u;
constexpr auto create_entry = 0x104f4c80u;
constexpr auto next_season_entry = 0x104f5f40u;
constexpr auto group_entry = 0x104f6140u;
constexpr auto knockout_entry = 0x104f63a0u, advance_entry = 0x104f6080u;
constexpr auto draw_entry = 0x104f6610u;
constexpr auto daily_entry = 0x104f69d0u;
void call_virtual(Cpu&, std::uint32_t resume);
void call_owned(Cpu&, std::uint32_t entry, std::uint32_t resume);

void block(Cpu& c, std::uint32_t pc, unsigned instructions) {
    c.s.step(pc);
    c.s.charge(instructions - 1);
}

std::int32_t record_compare(Cpu& c, std::int32_t value) {
    // Record-form operations update the four CR bits individually in the
    // reference, preserving the upper, unused bits of its byte container.
    const auto upper = c.cr(0) & 0xf0;
    const auto order = compare(c, value);
    c.set_cr(0, upper | c.cr(0));
    return order;
}

struct MatchSlot {
    unsigned weekday, occurrence, flags;
};

// These are the tournament's ordered fixture policies. Call-site identity and
// argument marshalling stay at the ABI boundary around the shared builders.
constexpr std::array<std::array<MatchSlot, 6>, 4> group_slots{{
    {{{5, 1, 4}, {6, 1, 0x40}, {2, 1, 4}, {2, 2, 0x40}, {6, 1, 4}, {6, 1, 0x40}}},
    {{{0, 1, 4}, {0, 2, 0x40}, {3, 1, 4}, {3, 2, 0x40}, {0, 1, 4}, {0, 1, 0x40}}},
    {{{6, 1, 0x80}, {6, 2, 0x80}, {2, 2, 0x80}, {3, 2, 0x80}, {0, 2, 0x80}, {0, 2, 0x40}}},
    {{{0, 2, 0x80}, {1, 2, 0x80}, {4, 1, 0x80}, {4, 2, 0x80}, {1, 1, 0x40}, {1, 1, 0x80}}}
}};

void set_match_slot(Cpu& c, std::uint32_t pc, std::uint32_t builder,
                    std::uint32_t rows, unsigned round, unsigned slot, MatchSlot policy) {
    block(c, pc, 8);
    c.set({{3, rows}, {4, round}, {5, slot}, {6, policy.weekday},
           {7, policy.occurrence}, {8, 3}, {9, policy.flags}});
    c.call(builder, pc + 32);
}

struct KnockoutRound {
    unsigned priority, index, format, entrants, winners, extra_slots;
    void store(Cpu& c, std::uint32_t address) const {
        c.put16(address + 8, priority);
        c.put16(address + 0xa, index);
        c.put16(address + 0xc, 0);
        c.put16(address + 0xe, format);
        c.put8(address + 0x18, 10);
        c.put16(address + 0x1a, entrants);
        c.put16(address + 0x1c, winners);
        c.put16(address + 0x1e, extra_slots);
        c.put16(address + 0x10, 0);
        c.put16(address + 0x20, 0);
        c.put8(address + 0x22, 0);
        c.put8(address + 0x23, 1);
        c.put8(address + 0x24, 0);
        c.put32(address + 0x6c, 0);
        c.put32(address + 0x70, 0);
        c.put32(address + 0x74, 0);
    }
};

void report_failure(Cpu& c, std::uint32_t pc, std::uint32_t file, std::uint32_t message,
                    unsigned name, unsigned extension, unsigned line) {
    block(c, pc, 6);
    c.set({{3, file}, {6, c.sp() + name}, {7, c.sp() + extension}, {4, 0}, {5, 0}});
    c.call(0x10739890, pc + 24);
    block(c, pc + 24, 1);
    // The reference inlines this two-instruction version getter at its call
    // site; another dispatcher boundary would change a finite-budget prefix.
    c.link(pc + 28);
    c.set(3, c.toc(-0x5b20));
    c.s.charge(2);
    block(c, pc + 28, 7);
    c.set({{5, c.r(3)}, {3, message}, {4, file + 0x14},
           {6, c.sp() + name}, {7, c.sp() + extension}, {8, line}});
    c.call(0x1077b720, pc + 56);
    block(c, pc + 56, 4);
    c.set({{4, message}, {3, file + 0x21}, {5, 0}});
    c.call(0x10769b60, pc + 72);
}

void calendar_allocation_failure(Cpu& c, std::uint32_t pc, unsigned name, unsigned extension, unsigned line) {
    report_failure(c, pc, c.r(28), c.r(27), name, extension, line);
    block(c, pc + 72, 4);
    c.set({{0, 0}, {3, 0}});
    c.put8(c.r(31), 0);
}

void create_competition(Cpu& c) {
    GuestNativeScope scope(c.s, create_entry);
    block(c, create_entry, 12);
    const auto stack = c.sp(), frame = stack - 0xa0;
    const auto object_slot = frame + 0xb8;
    c.set(0, c.lr());
    guest::transfer_saved_registers<27>(c, stack);
    const guest::Kept<29> success(c, c.toc(-0x7bac));
    guest::Kept<27> retained(c, c.r(4));
    const guest::Kept<30> message(c, c.toc(-0x7ba8));
    const guest::Kept<28> definition(c, c.r(5));
    const guest::Kept<31> file(c, c.toc(-0x45ac));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack); c.set(1, frame); scope.frame(frame);
    c.put32(object_slot, c.r(3));
    // This constructor homes its object above the frame, then reloads it
    // between record writes and calls. Keep those observable stack reads.
    const auto object = [&](unsigned argument) {
        const auto address = c.u32(object_slot);
        c.set(argument, address);
        return address;
    };
    object(3);
    c.call(0x1049e4f0, 0x104f4cb0);
    block(c, 0x104f4cb0, 23);
    c.set(6, c.toc(-0x6078));
    c.set(5, 4);
    auto current = object(3);
    c.set({{4, UINT32_MAX}, {0, 0}});
    c.put32(current, c.r(6));
    current = object(3); c.put16(current + 0x40, retained);
    current = object(3); c.put32(current + 4, definition);
    current = object(3); c.put8(current + 0x42, c.r(5));
    current = object(3); c.put32(current + 0x1c, c.r(4));
    current = object(3); c.put32(current + 0x20, c.r(4));
    current = object(3); c.put8(current + 0x4a, c.r(0));
    current = object(4);
    c.set(3, std::uint32_t(c.i16(current + 0x40)));
    c.set(0, c.r(3) + 1);
    c.put16(current + 0x40, c.r(0));
    for (;;) {
        block(c, 0x104f4d18, 8);
        current = object(5);
        const auto year = std::int32_t(c.i16(current + 0x40));
        const auto offset = year - 1996;
        // Signed remainder replaces the rotate/xor sequence; retain only
        // its final register and condition effects at the ABI boundary.
        c.set(3, std::uint32_t(offset) >> 31);
        c.set(0, std::uint32_t(offset % 2));
        if (!record_compare(c, std::int32_t(c.r(0)))) break;
        block(c, 0x104f4d0c, 3);
        c.set(3, std::uint32_t(c.i16(c.r(5) + 0x40)));
        c.set(0, c.r(3) + 1);
        c.put16(c.r(5) + 0x40, c.r(0));
    }
    block(c, 0x104f4d38, 7);
    c.set({{0, 15}, {4, 1}});
    c.put8(c.r(5) + 0x4b, c.r(0));
    current = object(3);
    c.set(12, c.u32(current));
    c.set(12, c.u32(c.r(12) + 0x20));
    call_virtual(c, 0x104f4d54);
    block(c, 0x104f4d54, 3);
    c.set(2, c.u32(frame + 0x14));
    if (compare(c, std::int32_t(c.r(3)))) {
        block(c, 0x104f4d60, 2);
        object(3);
    } else {
        block(c, 0x104f4d68, 18);
        current = object(3);
        c.set({{0, 40}, {5, 0}, {4, UINT32_MAX}});
        c.put16(current + 0xdc, c.r(0));
        c.set(0, 4);
        current = object(3); c.put8(current + 0x45, c.r(5));
        current = object(3); c.put8(current + 0x44, c.r(4));
        current = object(3); c.put32(current + 0x30, c.r(4));
        current = object(3); c.put32(current + 0x2c, c.r(0));
        current = object(3); c.set(0, c.u32(current + 0x2c));
        if (compare(c, std::int32_t(c.r(0))) > 0) {
            block(c, 0x104f4db0, 2);
            c.set(3, c.r(0) * 4);
            c.call(0x10775130, 0x104f4db8);
            block(c, 0x104f4db8, 3);
            current = object(4);
            c.put32(current + 0xc, c.r(3));
        } else {
            block(c, 0x104f4dc4, 1);
            c.put32(c.r(3) + 0xc, c.r(5));
        }
        block(c, 0x104f4dc8, 4);
        current = object(3);
        c.set(12, c.u32(current));
        c.set(12, c.u32(c.r(12) + 0x94));
        call_virtual(c, 0x104f4dd8);
        block(c, 0x104f4dd8, 3);
        c.set(2, c.u32(frame + 0x14));
        if (!compare(c, std::int32_t(c.r(3)))) {
            report_failure(c, 0x104f4de4, file, message, 0x68, 0x40, 0x58);
            block(c, 0x104f4e2c, 4);
            c.set(0, 0); c.put8(success, 0);
            object(3);
        } else {
            block(c, 0x104f4e3c, 2);
            c.set(3, 0x634);
            c.call(0x10773520, 0x104f4e44);
            block(c, 0x104f4e44, 2);
            retained = c.r(3);
            if (record_compare(c, std::int32_t(std::uint32_t(retained)))) {
                block(c, 0x104f4e4c, 2);
                object(4);
                c.call(0x104533a0, 0x104f4e54);
            }
            block(c, 0x104f4e54, 6);
            current = object(3); c.put32(current + 8, retained);
            current = object(3); c.set(0, c.u32(current + 8));
            if (!compare_logical(c, c.r(0))) {
                report_failure(c, 0x104f4e6c, file, message, 0x48, 0x38, 0x5f);
                block(c, 0x104f4eb4, 3);
                c.set(0, 0); c.put8(success, 0);
                object(3);
            }
        }
    }
    block(c, 0x104f4ec0, 5);
    c.set(0, c.u32(frame + 0xa8));
    c.set(1, stack); c.set_lr(c.r(0));
    guest::transfer_saved_registers<27, true>(c, stack);
    c.finish();
}

void build_calendar(Cpu& c) {
    GuestNativeScope scope(c.s, calendar_entry);
    block(c, calendar_entry, 4);
    const auto stack = c.sp(), frame = stack - 0xb0;
    c.set(0, c.lr());
    guest::transfer_saved_registers<26>(c, stack);
    c.set(31, c.toc(-0x7bac));
    c.set(29, c.r(3));
    // The generated reference returns to its dispatcher at this page seam.
    // Preserve its partial prologue when the allowance ends after four ops.
    block(c, 0x104f5000, 7);
    c.set(27, c.toc(-0x7ba8));
    c.set(28, c.toc(-0x45ac));
    c.put32(stack + 8, c.r(0));
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(c.r(4)))));
    const auto stage_order = compare(c, std::int32_t(c.r(0)), 3);
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    if (stage_order < 0) {
        block(c, 0x104f501c, 2);
        if (compare_logical(c, c.r(7))) {
            block(c, 0x104f5024, 2);
            c.set(0, 1);
            c.put8(c.r(7), 1);
        }
        block(c, 0x104f502c, 7);
        c.set(0, 3);
        const guest::Kept<30> stage(c, std::uint32_t(std::int32_t(std::int8_t(c.r(4)))));
        c.put16(c.r(5), c.r(0));
        c.set(0, stage + 1001);
        c.set(3, 0xe4);
        c.put16(c.r(6), c.r(0));
        c.call(0x10775130, 0x104f5048);
        block(c, 0x104f5048, 2);
        const guest::Kept<26> rows(c, c.r(3));
        if (!record_compare(c, std::int32_t(std::uint32_t(rows)))) {
            calendar_allocation_failure(c, 0x104f5050, 0x70, 0x48, 0x9e);
        } else {
            constexpr std::array starts{0x104f50a8u, 0x104f50f0u, 0x104f513cu};
            constexpr std::array finishes{0x104f50d0u, 0x104f511cu, 0x104f5168u};
            for (unsigned round = 0; round < 3; ++round) {
                const auto pc = starts[round];
                block(c, pc, round ? 11 : 10);
                c.set(0, 1);
                if (round) c.set(3, rows);
                c.put32(frame + 0x38, 1);
                c.set({{4, round}, {5, 8 + round * 4}, {6, 1}});
                if (!round) c.set(7, 0);
                c.set(10, std::uint32_t(c.i16(c.r(29) + 0x40)));
                if (round) c.set(7, 0);
                c.set({{8, 6 - round * 3}, {9, 1}});
                c.call(0x104a9810, finishes[round]);
                block(c, finishes[round], 8);
                c.set({{3, rows}, {4, round}, {5, 2}, {6, UINT32_MAX}, {7, UINT32_MAX}, {8, UINT32_MAX}, {9, 0}});
                c.call(0x104a9890, finishes[round] + 32);
            }
            block(c, 0x104f5188, 2);
            std::uint32_t policy_pc = 0x104f5338;
            int policy = 2;
            const auto order = compare(c, std::int32_t(std::uint32_t(stage)), 1);
            if (order) {
                block(c, 0x104f5190, 1);
                if (order > 0) {
                    block(c, 0x104f51a4, 2);
                    if (compare(c, std::int32_t(std::uint32_t(stage)), 3) < 0) {
                        block(c, 0x104f51ac, 1);
                        policy = 3; policy_pc = 0x104f53fc;
                    } else policy = -1;
                } else {
                    block(c, 0x104f5194, 2);
                    const auto previous = compare(c, std::int32_t(std::uint32_t(stage)), -1);
                    if (!previous) { policy = 0; policy_pc = 0x104f51b0; }
                    else {
                        block(c, 0x104f519c, 1);
                        if (previous > 0) { policy = 1; policy_pc = 0x104f5274; }
                        else { block(c, 0x104f51a0, 1); policy = -1; }
                    }
                }
            }
            if (policy >= 0) {
                for (unsigned i = 0; i < 6; ++i)
                    set_match_slot(c, policy_pc + i * 32, 0x104a9890, rows, i / 2, i % 2, group_slots[policy][i]);
                if (policy != 3) block(c, policy_pc + 192, 1);
            }
            block(c, 0x104f54bc, 2);
            c.set(3, rows);
        }
    } else {
        block(c, 0x104f54c4, 1);
        if (stage_order) { block(c, 0x104f58f0, 1); c.set(3, 0); }
        else {
            block(c, 0x104f54c8, 2);
            if (compare_logical(c, c.r(7))) {
                block(c, 0x104f54d0, 2);
                c.set(0, 0); c.put8(c.r(7), 0);
            }
            block(c, 0x104f54d8, 6);
            c.set({{3, 4}, {0, 0}});
            c.put16(c.r(5), 4);
            c.set(3, 0x1e0);
            c.put16(c.r(6), 0);
            c.call(0x10775130, 0x104f54f0);
            block(c, 0x104f54f0, 2);
            const guest::Kept<30> rows(c, c.r(3));
            if (!record_compare(c, std::int32_t(std::uint32_t(rows)))) {
                calendar_allocation_failure(c, 0x104f54f8, 0x50, 0x40, 0xe0);
            } else {
                block(c, 0x104f5550, 7);
                c.set(9, std::uint32_t(c.i16(c.r(29) + 0x40)));
                c.set({{4, 0}, {5, 18}, {6, 1}, {7, 0}, {8, 2}});
                c.call(0x104854f0, 0x104f556c);
                const auto closing_date = [&](std::uint32_t pc, unsigned round, unsigned day, unsigned weekday, unsigned weeks, unsigned flags) {
                    block(c, pc, 11);
                    c.set(0, flags); c.set(3, rows); c.put32(frame + 0x38, flags);
                    c.set({{4, round}, {5, day}, {6, 1}});
                    c.set(10, std::uint32_t(c.i16(c.r(29) + 0x40)));
                    c.set({{7, 0}, {8, weekday}, {9, weeks}});
                    c.call(0x10485570, pc + 44);
                };
                closing_date(0x104f556c, 0, 21, 5, 1, 1);
                constexpr std::array quarter_slots{MatchSlot{4,2,0x80}, MatchSlot{5,1,4}, MatchSlot{5,2,0x40}, MatchSlot{6,1,0x40}};
                for (unsigned i = 0; i < 4; ++i) set_match_slot(c, 0x104f5598 + i * 32, 0x10485600, rows, 0, i, quarter_slots[i]);
                block(c, 0x104f5618, 8);
                c.set({{3, rows}, {4, 0}, {5, 4}, {6, UINT32_MAX}, {7, UINT32_MAX}, {8, UINT32_MAX}, {9, 0}});
                c.call(0x10485600, 0x104f5638);
                block(c, 0x104f5638, 31);
                KnockoutRound{120,0,0x303,8,4,8}.store(c, rows);
                c.set({{0, 1}, {12, 0}, {11, 10}, {10, 8}, {3, rows}, {4, 1}, {5, 23}, {6, 1}, {7, 0}, {8, 0}});
                c.set(9, std::uint32_t(c.i16(c.r(29) + 0x40)));
                c.call(0x104854f0, 0x104f56b4);
                closing_date(0x104f56b4, 1, 25, 2, 1, 1);
                set_match_slot(c, 0x104f56e0, 0x10485600, rows, 1, 0, {2,1,4});
                set_match_slot(c, 0x104f5700, 0x10485600, rows, 1, 1, {2,2,0x80});
                block(c, 0x104f5720, 8);
                c.set({{3, rows}, {4, 1}, {5, 2}, {6, UINT32_MAX}, {7, UINT32_MAX}, {8, UINT32_MAX}, {9, 0}});
                c.call(0x10485600, 0x104f5740);
                block(c, 0x104f5740, 31);
                KnockoutRound{130,1,0x303,4,2,0}.store(c, rows + 0x78);
                c.set({{0, 2}, {12, 1}, {11, 0}, {10, 10}, {3, rows}, {4, 2}, {5, 26}, {6, 1}, {7, 0}, {8, 3}});
                c.set(9, std::uint32_t(c.i16(c.r(29) + 0x40)));
                c.call(0x104854f0, 0x104f57bc);
                closing_date(0x104f57bc, 2, 28, 5, 1, 4);
                block(c, 0x104f57e8, 30);
                KnockoutRound{150,2,0x203,2,1,0}.store(c, rows + 0xf0);
                c.set({{0, 1}, {11, 2}, {10, 0}, {3, rows}, {4, 3}, {5, 26}, {6, 1}, {7, 0}, {8, 3}});
                c.set(9, std::uint32_t(c.i16(c.r(29) + 0x40)));
                c.call(0x104854f0, 0x104f5860);
                closing_date(0x104f5860, 3, 27, 4, 2, 0x40);
                block(c, 0x104f588c, 25);
                KnockoutRound{140,3,0x203,2,1,0}.store(c, rows + 0x168);
                c.set({{3, rows}, {0, 1}, {7, 0}, {6, 0x203}, {5, 10}, {4, 2}});
            }
        }
    }
    block(c, 0x104f58f4, 5);
    c.set(0, c.u32(frame + 0xb8));
    c.set(1, stack); c.set_lr(c.r(0));
    guest::transfer_saved_registers<26, true>(c, stack);
    c.finish();
}

struct TournamentSettings {
    Cpu& c;
    std::uint32_t address;
    guest::Field<std::int16_t> year() const { return {c, address + 0x40}; }
    void initialize_rules() const {
        c.put16(address + 0x3c, 1);
        constexpr std::array<std::pair<unsigned, unsigned>, 13> fields{{
            {0xc4,3}, {0xc5,1}, {0xc6,10}, {0x43,2}, {0xc7,1}, {0xc8,2}, {0xc9,3},
            {0xc0,2}, {0xc1,0}, {0xc2,0}, {0xc3,0}, {0x46,7}, {0x47,3}
        }};
        for (const auto [offset, value] : fields) c.put8(address + offset, value);
    }
};

void configure_competition(Cpu& c) {
    GuestNativeScope scope(c.s, configure_entry);
    block(c, configure_entry, 33);
    const auto stack = c.sp(), frame = stack - 0x50;
    c.set(0, c.lr());
    c.put32(stack - 4, c.r(31));
    c.set({{11,1}, {10,3}});
    c.put32(stack + 8, c.r(0));
    const guest::Kept<31> competition(c, c.r(3));
    c.set({{9,10}, {8,2}});
    c.put32(frame, stack); c.set(1, frame); scope.frame(frame);
    c.set({{6,0}, {0,7}, {4,5}, {5,9}, {7,5}});
    const TournamentSettings settings{c, competition};
    settings.initialize_rules();
    c.set(3, frame + 0x38);
    c.set(6, std::uint32_t(std::int32_t(settings.year())));
    c.set(0, c.r(6) - 1);
    c.set(6, std::uint16_t(c.r(0)));
    c.call(0x107156f0, 0x104f5d04);
    block(c, 0x104f5d04, 12);
    c.set(4, std::uint32_t(c.i16(frame + 0x38)));
    c.set(0, UINT32_MAX);
    c.set(3, competition);
    c.set(5, competition + 0xa6);
    c.put16(competition + 0xb6, c.r(4));
    c.set({{6, competition + 0x3a}, {4, UINT32_MAX}, {7, 0}});
    c.put16(competition + 0xb8, c.r(0));
    c.set(12, c.u32(competition));
    c.set(12, c.u32(c.r(12) + 0x44));
    call_virtual(c, 0x104f5d34);
    block(c, 0x104f5d34, 8);
    c.set(2, c.u32(frame + 0x14));
    c.put32(competition + 0xbc, c.r(3));
    c.set(3, 1);
    c.set(0, c.u32(frame + 0x58));
    c.set(1, stack); c.set_lr(c.r(0));
    c.set(31, c.u32(stack - 4));
    c.finish();
}

void start_next_season(Cpu& c) {
    GuestNativeScope scope(c.s, next_season_entry);
    block(c, next_season_entry, 11);
    const auto stack = c.sp(), frame = stack - 0x80;
    c.set(0, c.lr());
    guest::transfer_saved_registers<27>(c, stack);
    const guest::Kept<30> message(c, c.toc(-0x7ba8));
    const guest::Kept<27> competition(c, c.r(3));
    const guest::Kept<31> file(c, c.toc(-0x45ac));
    c.set(4, 0); c.put32(stack + 8, c.r(0)); c.set(0, 0);
    c.put32(frame, stack); c.set(1, frame); scope.frame(frame);
    c.put8(competition + 0x49, 0);
    c.call(0x104a6660, 0x104f5f6c);
    block(c, 0x104f5f6c, 3);
    c.set(3, c.u32(competition + 0xbc));
    if (compare_logical(c, c.r(3))) {
        block(c, 0x104f5f78, 1);
        c.call(0x10775170, 0x104f5f7c);
        block(c, 0x104f5f7c, 2);
        c.set(0, 0); c.put32(competition + 0xbc, 0);
    }
    block(c, 0x104f5f84, 3);
    c.set(3, c.u32(competition + 8));
    if (compare_logical(c, c.r(3))) {
        block(c, 0x104f5f90, 2);
        c.set(4, 1);
        c.call(0x104564a0, 0x104f5f98);
    }
    block(c, 0x104f5f98, 3);
    guest::Kept<28> index(c, 0);
    guest::Kept<29> offset(c, 0);
    for (;;) {
        block(c, 0x104f5fd0, 3);
        c.set(0, c.u32(competition + 0x30));
        if (compare(c, std::int32_t(index), std::int32_t(c.r(0))) > 0) break;
        block(c, 0x104f5fa4, 4);
        c.set(3, c.u32(competition + 0xc));
        c.set(3, c.u32(c.r(3) + offset));
        if (compare_logical(c, c.r(3))) {
            block(c, 0x104f5fb4, 4);
            c.set(12, c.u32(c.r(3))); c.set(4, 1);
            c.set(12, c.u32(c.r(12) + 8));
            call_virtual(c, 0x104f5fc4);
            block(c, 0x104f5fc4, 1);
            c.set(2, c.u32(frame + 0x14));
        }
        block(c, 0x104f5fc8, 2);
        offset = offset + 4; index = index + 1;
    }
    block(c, 0x104f5fdc, 9);
    c.set({{3, UINT32_MAX}, {0, 0}});
    c.put32(competition + 0x30, c.r(3));
    c.set(3, competition);
    c.set(4, std::uint32_t(c.i16(competition + 0x40)));
    c.set(4, c.r(4) + 2);
    c.put16(competition + 0x40, c.r(4));
    c.put8(competition + 0x45, c.r(0));
    c.set(12, c.u32(competition));
    block(c, 0x104f6000, 2);
    c.set(12, c.u32(c.r(12) + 0x94));
    call_virtual(c, 0x104f6008);
    block(c, 0x104f6008, 3);
    c.set(2, c.u32(frame + 0x14));
    if (!compare(c, std::int32_t(c.r(3)))) {
        report_failure(c, 0x104f6014, file, message, 0x40, 0x38, 0x2a8);
        block(c, 0x104f605c, 3);
        c.set(3, c.toc(-0x7bac)); c.set(0, 0); c.put8(c.r(3), 0);
    }
    block(c, 0x104f6068, 5);
    c.set(0, c.u32(frame + 0x88));
    c.set(1, stack); c.set_lr(c.r(0));
    guest::transfer_saved_registers<27, true>(c, stack);
    c.finish();
}

void create_group(Cpu& c) {
    GuestNativeScope scope(c.s, group_entry);
    block(c, group_entry, 11);
    const auto stack = c.sp(), frame = stack - 0x100;
    c.set(0, c.lr());
    guest::transfer_saved_registers<21>(c, stack);
    const guest::Kept<25> competition(c, c.r(3));
    const guest::Kept<29> success(c, c.toc(-0x7bac));
    const guest::Kept<30> message(c, c.toc(-0x7ba8));
    const guest::Kept<26> group(c, c.r(4));
    const guest::Kept<31> file(c, c.toc(-0x45ac));
    c.set(3, 16); c.put32(stack + 8, c.r(0));
    c.put32(frame, stack); c.set(1, frame); scope.frame(frame);
    c.call(0x10775130, 0x104f616c);
    block(c, 0x104f616c, 2);
    const guest::Kept<28> entrants(c, c.r(3));
    if (!record_compare(c, std::int32_t(std::uint32_t(entrants)))) {
        report_failure(c, 0x104f6174, file, message, 0xac, 0x84, 0x2dc);
        block(c, 0x104f61bc, 4);
        c.set({{0, 0}, {3, 0}}); c.put8(success, 0);
    } else {
        // Reload the entrant table between rows: its pointer is live to
        // memory observers, even though the selected offsets are invariant.
        block(c, 0x104f61cc, 29);
        c.set(4, group + 1); c.set(5, c.u32(competition + 0xec));
        c.set(0, c.r(4) * 16); c.set(3, competition);
        c.set(0, c.u32(c.r(5) + c.r(0)));
        c.set(7, c.r(4) * 4); c.set(6, c.r(7) + 1);
        c.set(4, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(group)))));
        c.put32(entrants, c.r(0));
        c.set(5, c.r(7) + 2); c.set(0, c.r(7) + 3);
        c.set(6, c.r(6) * 4); c.set(7, c.u32(competition + 0xec));
        c.set(8, c.r(5) * 4); c.set(0, c.r(0) * 4);
        c.set(5, frame + 0x7a); c.set(9, c.u32(c.r(7) + c.r(6)));
        c.set({{6, frame + 0x78}, {7, 0}}); c.put32(entrants + 4, c.r(9));
        c.set(9, c.u32(competition + 0xec)); c.set(8, c.u32(c.r(9) + c.r(8)));
        c.put32(entrants + 8, c.r(8));
        c.set(8, c.u32(competition + 0xec)); c.set(0, c.u32(c.r(8) + c.r(0)));
        c.put32(entrants + 12, c.r(0));
        c.set(12, c.u32(competition)); c.set(12, c.u32(c.r(12) + 0x44));
        call_virtual(c, 0x104f6240);
        block(c, 0x104f6240, 5);
        c.set(2, c.u32(frame + 0x14)); c.set(0, c.r(3)); c.set(3, 0xec);
        const guest::Kept<27> calendar(c, c.r(0));
        c.call(0x10773520, 0x104f6254);
        block(c, 0x104f6254, 2);
        const guest::Kept<24> stage(c, c.r(3));
        guest::Kept<21> stage_offset(c, c.r(21));
        if (record_compare(c, std::int32_t(std::uint32_t(stage)))) {
            block(c, 0x104f625c, 37);
            c.set(0, c.u8(competition + 0xc4));
            c.set(22, competition + 0xc7); c.set(23, competition + 0xc0);
            c.set(12, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(group)))));
            c.put32(frame + 0x38, c.r(0));
            c.set({{10, 4}, {11, 0}, {8, UINT32_MAX}});
            c.set(5, c.u8(competition + 0xc5)); c.set(0, 2);
            c.set({{4, competition}, {6, entrants}}); c.put32(frame + 0x3c, c.r(5));
            c.set({{9, calendar}, {5, 4}, {7, 1}});
            stage_offset = c.u8(competition + 0xc6); c.put32(frame + 0x40, stage_offset);
            c.put32(frame + 0x44, c.r(22)); c.put32(frame + 0x48, c.r(23));
            c.set(23, std::uint32_t(c.i16(competition + 0x40))); c.put32(frame + 0x4c, c.r(23));
            c.put32(frame + 0x50, c.r(12));
            c.set(12, std::uint32_t(c.i16(frame + 0x78))); c.put32(frame + 0x54, c.r(12));
            c.set(12, c.u8(competition + 0x4b)); c.put32(frame + 0x58, c.r(12));
            c.put32(frame + 0x5c, c.r(10)); c.put32(frame + 0x60, c.r(11));
            c.set(10, std::uint32_t(c.i16(competition + 0xdc))); c.put32(frame + 0x64, c.r(10));
            c.put32(frame + 0x68, c.r(8)); c.put32(frame + 0x6c, c.r(11)); c.put32(frame + 0x70, c.r(0));
            c.set(8, c.u32(competition + 4)); c.set(10, std::uint32_t(c.i16(frame + 0x7a)));
            c.call(0x1056dc60, 0x104f62f0);
        }
        block(c, 0x104f62f0, 5);
        c.set(4, c.u32(competition + 0xc)); stage_offset = group * 4;
        c.set(3, entrants); c.put32(c.r(4) + stage_offset, stage);
        c.call(0x10775170, 0x104f6304);
        block(c, 0x104f6304, 2); c.set(3, calendar);
        c.call(0x10775170, 0x104f630c);
        block(c, 0x104f630c, 4);
        c.set(3, c.u32(competition + 0xc)); c.set(0, c.u32(c.r(3) + stage_offset));
        if (!compare_logical(c, c.r(0))) {
            report_failure(c, 0x104f631c, file, message, 0x8c, 0x7c, 0x2f2);
            block(c, 0x104f6364, 4);
            c.set({{0, 0}, {3, 0}}); c.put8(success, 0);
        } else {
            block(c, 0x104f6374, 4);
            c.put32(competition + 0x30, group);
            c.set(3, c.u32(competition + 0xc)); c.set(3, c.u32(c.r(3) + stage_offset));
            c.call(0x104a0e40, 0x104f6384);
            block(c, 0x104f6384, 1); c.set(3, 1);
        }
    }
    block(c, 0x104f6388, 5);
    c.set(0, c.u32(frame + 0x108)); c.set(1, stack); c.set_lr(c.r(0));
    guest::transfer_saved_registers<21, true>(c, stack); c.finish();
}

void standing_row(Cpu& c, std::uint32_t resume) {
    // The reference inlines this four-instruction row lookup. Keep its live
    // table read and ABI effects without introducing a dispatcher boundary.
    c.link(resume); c.set(0, c.r(4) * 0x44);
    c.set(3, c.u32(c.r(3) + 0xb0)); c.set(3, c.r(3) + c.r(0)); c.s.charge(4);
}

void create_knockout(Cpu& c) {
    GuestNativeScope scope(c.s, knockout_entry);
    block(c, knockout_entry, 10);
    const auto stack = c.sp(), frame = stack - 0xd0;
    c.set(0, c.lr()); guest::transfer_saved_registers<25>(c, stack);
    const guest::Kept<27> competition(c, c.r(3));
    const guest::Kept<29> success(c, c.toc(-0x7bac));
    const guest::Kept<30> message(c, c.toc(-0x7ba8));
    c.set(3, 32); const guest::Kept<31> file(c, c.toc(-0x45ac));
    c.put32(stack + 8, c.r(0)); c.put32(frame, stack); c.set(1, frame); scope.frame(frame);
    c.call(0x10775130, 0x104f63c8);
    block(c, 0x104f63c8, 2);
    const guest::Kept<28> entrants(c, c.r(3));
    if (!record_compare(c, std::int32_t(std::uint32_t(entrants)))) {
        report_failure(c, 0x104f63d0, file, message, 0x8c, 0x64, 0x311);
        block(c, 0x104f6418, 3); c.set(0, 0); c.put8(success, 0);
    } else {
        block(c, 0x104f6424, 3); c.set({{3, competition}, {4, 0}}); standing_row(c, 0x104f6430);
        // Preserve the source's qualification order, including rereading the
        // live stage array around each shared standings lookup.
        constexpr std::array<unsigned, 6> next_places{1, 0, 1, 1, 0, 0};
        constexpr std::array<unsigned, 6> stages{0, 1, 2, 1, 2, 0};
        for (unsigned i = 0; i < stages.size(); ++i) {
            const auto pc = 0x104f6430 + i * 24;
            block(c, pc, 6); c.set(0, c.u32(c.r(3))); c.set(4, next_places[i]);
            c.put32(entrants + i * 4, c.r(0));
            c.set(3, c.u32(competition + 0xc)); c.set(3, c.u32(c.r(3) + stages[i] * 4));
            standing_row(c, pc + 24);
        }
        block(c, 0x104f64c0, 5); c.set(0, c.u32(c.r(3)));
        c.set({{3, competition}, {4, 1}}); c.put32(entrants + 24, c.r(0)); standing_row(c, 0x104f64d4);
        block(c, 0x104f64d4, 10); c.set(0, c.u32(c.r(3)));
        c.set({{3, competition}, {5, frame + 0x5a}, {6, frame + 0x58}});
        c.put32(entrants + 28, c.r(0)); c.set({{4, 3}, {7, 0}});
        c.set(12, c.u32(competition)); c.set(12, c.u32(c.r(12) + 0x44));
        call_virtual(c, 0x104f64fc);
        block(c, 0x104f64fc, 5); c.set(2, c.u32(frame + 0x14)); c.set(0, c.r(3)); c.set(3, 0xb4);
        const guest::Kept<26> calendar(c, c.r(0)); c.call(0x10773520, 0x104f6510);
        block(c, 0x104f6510, 2); const guest::Kept<25> stage(c, c.r(3));
        if (record_compare(c, std::int32_t(std::uint32_t(stage)))) {
            block(c, 0x104f6518, 21); c.set({{0, 3}, {5, 4}}); c.put32(frame + 0x38, c.r(0));
            c.set({{7, 20}, {0, 0}, {4, competition}}); c.put32(frame + 0x3c, c.r(5));
            c.set({{6, entrants}, {9, calendar}, {5, 8}}); c.set(8, std::uint32_t(c.i16(frame + 0x58)));
            c.put32(frame + 0x40, c.r(8)); c.put32(frame + 0x44, c.r(7));
            for (unsigned off = 0x48; off <= 0x54; off += 4) c.put32(frame + off, c.r(0));
            c.set(7, std::uint32_t(c.i16(frame + 0x5a))); c.set(8, c.u32(competition + 4));
            c.set(10, std::uint32_t(c.i16(competition + 0x40))); c.call(0x104bc3c0, 0x104f656c);
        }
        block(c, 0x104f656c, 4); c.set(4, c.u32(competition + 0xc)); c.set(3, entrants);
        c.put32(c.r(4) + 12, stage); c.call(0x10775170, 0x104f657c);
        block(c, 0x104f657c, 2); c.set(3, calendar); c.call(0x10775170, 0x104f6584);
        block(c, 0x104f6584, 4); c.set(3, c.u32(competition + 0xc)); c.set(3, c.u32(c.r(3) + 12));
        if (!compare_logical(c, c.r(3))) {
            report_failure(c, 0x104f6594, file, message, 0x6c, 0x5c, 0x32c);
            block(c, 0x104f65dc, 3); c.set(0, 0); c.put8(success, 0);
        } else {
            block(c, 0x104f65e8, 2); c.set(4, 0); c.call(0x1047ef50, 0x104f65f0);
            block(c, 0x104f65f0, 2); c.set(0, 3); c.put32(competition + 0x30, c.r(0));
        }
    }
    block(c, 0x104f65f8, 5); c.set(0, c.u32(frame + 0xd8)); c.set(1, stack);
    c.set_lr(c.r(0)); guest::transfer_saved_registers<25, true>(c, stack); c.finish();
}

void advance_stage(Cpu& c) {
    GuestNativeScope scope(c.s, advance_entry);
    block(c, advance_entry, 12);
    const auto stack = c.sp(), frame = stack - 0x70;
    c.set(0, c.lr()); c.put32(stack - 4, c.r(31));
    const guest::Kept<31> file(c, c.toc(-0x45ac)); c.put32(stack - 8, c.r(30));
    const guest::Kept<30> message(c, c.toc(-0x7ba8)); c.put32(stack + 8, c.r(0));
    c.put32(frame, stack); c.set(1, frame); scope.frame(frame);
    c.set(4, c.u32(c.r(3) + 0x2c)); c.set(5, c.u32(c.r(3) + 0x30)); c.set(0, c.r(4) - 1);
    if (compare(c, std::int32_t(c.r(5)), std::int32_t(c.r(0))) < 0) {
        block(c, 0x104f6108, 5); c.set(0, c.r(5) + 1); c.put32(c.r(3) + 0x30, c.r(0));
        c.set(0, c.u32(c.r(3) + 0x30));
        if (!compare(c, std::int32_t(c.r(0)), 3)) {
            block(c, 0x104f6120, 1); call_owned(c, knockout_entry, 0x104f6124);
        } else block(c, 0x104f611c, 1);
    } else {
        report_failure(c, 0x104f60b0, file, message, 0x40, 0x38, 0x2ba);
        block(c, 0x104f60f8, 4); c.set(3, c.toc(-0x7bac)); c.set(0, 0); c.put8(c.r(3), 0);
    }
    block(c, 0x104f6124, 6); c.set(0, c.u32(frame + 0x78)); c.set(1, stack); c.set_lr(c.r(0));
    c.set(31, c.u32(stack - 4)); c.set(30, c.u32(stack - 8)); c.finish();
}

void select_and_draw_entrants(Cpu& c) {
    GuestNativeScope scope(c.s, draw_entry);
    block(c, draw_entry, 21);
    const auto stack = c.sp(), frame = stack - 0xe0;
    c.set(0, c.lr());
    guest::transfer_saved_registers<15>(c, stack);
    const guest::Kept<20> random_seed(c, c.toc(-0x7b90));
    const guest::Kept<31> competition(c, c.r(3));
    const guest::Kept<21> random_start(c, c.toc(-0x7b8c));
    guest::Kept<17> entrant_count(c, 0);
    const guest::Kept<22> random_end(c, c.toc(-0x7b88));
    guest::Kept<15> entrant_offset(c, 0);
    const guest::Kept<23> random_cursor(c, c.toc(-0x7b84));
    const guest::Kept<24> nation_count(c, c.toc(-0x7b04));
    const guest::Kept<25> continent(c, c.toc(-0x7974));
    const guest::Kept<26> nations(c, c.toc(-0x7b98));
    const guest::Kept<27> success(c, c.toc(-0x7bac));
    const guest::Kept<28> message(c, c.toc(-0x7ba8));
    const guest::Kept<29> file(c, c.toc(-0x45ac));
    c.put32(stack + 8, c.r(0)); c.set(0, 16);
    c.put32(frame, stack); c.set(1, frame); scope.frame(frame);
    c.put16(competition + 0xf0, 16);
    c.set(3, 200); c.call(0x10775130, 0x104f6664);
    block(c, 0x104f6664, 4);
    c.put32(competition + 0xec, c.r(3)); c.set(0, c.u32(competition + 0xec));
    const auto fail = [&](unsigned pc, unsigned name, unsigned extension, unsigned line) {
        report_failure(c, pc, file, message, name, extension, line);
        block(c, pc + 72, 4);
        c.set({{0, 0}, {3, 0}}); c.put8(success, 0);
    };
    if (!compare_logical(c, c.r(0))) {
        fail(0x104f6674, 0x70, 0x48, 0x349);
    } else {
        block(c, 0x104f66cc, 10);
        c.set(3, c.toc(-0x7424)); c.set(6, frame + 0x40);
        c.set(4, c.toc(-0x6950)); c.set(7, frame + 0x44);
        c.set(0, guest::extended(c.i16(competition + 0x40))); c.set(8, 1);
        c.set(3, c.u32(c.r(3))); c.set(4, c.u32(c.r(4))); c.set(5, std::uint16_t(c.r(0)));
        c.call(0x1049d1c0, 0x104f66f4);
        block(c, 0x104f66f4, 5);
        const guest::Kept<30> qualifier_result(c, c.r(3));
        guest::Kept<16> qualifier_cursor(c, frame + 0x40);
        const guest::Kept<19> qualifier_count(c, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
        guest::Kept<18> qualifier_index(c, 0);
        for (;;) {
            block(c, 0x104f6734, 3);
            c.set(0, guest::extended(std::int16_t(std::uint32_t(qualifier_index))));
            if (compare(c, std::int32_t(c.r(0)), std::int32_t(std::uint32_t(qualifier_count))) >= 0) break;
            block(c, 0x104f6708, 5);
            c.set(0, c.u32(qualifier_cursor)); c.set(3, c.u32(nations));
            c.set(0, c.r(0) * 0x128); c.set(3, c.r(3) + c.r(0));
            c.call(0x100c76c0, 0x104f671c);
            block(c, 0x104f671c, 6);
            c.set(4, c.u32(competition + 0xec));
            qualifier_cursor = qualifier_cursor + 4; qualifier_index = qualifier_index + 1;
            entrant_count = entrant_count + 1;
            c.put32(c.r(4) + entrant_offset, c.r(3)); entrant_offset = entrant_offset + 4;
        }
        block(c, 0x104f6740, 4);
        guest::Kept<18> nation(c, c.u32(nations));
        guest::Kept<16> append_offset(c, entrant_count * 4);
        guest::Kept<26> nation_index(c, 0);
        for (;;) {
            block(c, 0x104f67e0, 4);
            c.set(0, c.u32(nation_count));
            c.set(3, guest::extended(std::int16_t(std::uint32_t(nation_index))));
            if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) >= 0) break;
            block(c, 0x104f6750, 3);
            c.set(3, c.u32(nation + 0x74));
            if (compare_logical(c, c.r(3))) {
                block(c, 0x104f675c, 4);
                c.set(3, c.u32(c.r(3) + 8)); c.set(0, c.u32(continent));
                if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0) {
                    block(c, 0x104f676c, 2);
                    if (compare(c, std::int32_t(std::uint32_t(entrant_count)), 50) < 0) {
                        block(c, 0x104f6774, 5);
                        c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(qualifier_result)))));
                        c.set({{5, 0}, {6, 0}, {4, 0}});
                        // Check every seeded qualifier even after a match.
                        for (;;) {
                            block(c, 0x104f67ac, 3);
                            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(6)))));
                            if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) >= 0) break;
                            block(c, 0x104f678c, 5);
                            c.set(3, c.u32(competition + 0xec)); c.set(3, c.u32(c.r(3) + c.r(4)));
                            c.set(3, c.u32(c.r(3) + 0x2c));
                            if (compare_logical(c, c.r(3), nation) == 0) {
                                block(c, 0x104f67a0, 1); c.set(5, 1);
                            }
                            block(c, 0x104f67a4, 2);
                            c.set(4, c.r(4) + 4); c.set(6, c.r(6) + 1);
                        }
                        block(c, 0x104f67b8, 2);
                        c.set(0, std::uint32_t(std::int32_t(std::int8_t(c.r(5)))));
                        if (!record_compare(c, std::int32_t(c.r(0)))) {
                            block(c, 0x104f67c0, 2);
                            c.set(3, nation); c.call(0x100c76c0, 0x104f67c8);
                            block(c, 0x104f67c8, 4);
                            c.set(4, c.u32(competition + 0xec)); entrant_count = entrant_count + 1;
                            c.put32(c.r(4) + append_offset, c.r(3)); append_offset = append_offset + 4;
                        }
                    }
                }
            }
            block(c, 0x104f67d8, 2); nation_index = nation_index + 1; nation = nation + 0x128;
        }
        block(c, 0x104f67f0, 2);
        if (compare(c, std::int32_t(std::uint32_t(entrant_count)), 16) < 0) {
            block(c, 0x104f67f8, 2); c.set(3, 0);
        } else {
            block(c, 0x104f6800, 8);
            c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(qualifier_result)))));
            c.set(3, c.u32(competition + 0xec)); c.set(0, c.r(0) * 4);
            c.set(6, c.toc(-0x6188)); c.set(3, c.r(3) + c.r(0));
            c.set({{4, entrant_count - qualifier_count}, {5, 4}});
            c.call(0x10768300, 0x104f6820);
            block(c, 0x104f6820, 2); c.set(3, 64); c.call(0x10775130, 0x104f6828);
            block(c, 0x104f6828, 2);
            const guest::Kept<30> draw_buffer(c, c.r(3));
            if (!record_compare(c, std::int32_t(std::uint32_t(draw_buffer)))) {
                fail(0x104f6830, 0x50, 0x38, 0x380);
            } else {
                const auto draw = [&](unsigned pc, bool retry) {
                    block(c, pc, 6);
                    c.set(3, c.u32(random_cursor)); c.set(0, c.u32(random_end));
                    c.set(3, c.r(3) + 2);
                    const auto beyond_pool = compare_logical(c, c.r(3), c.r(0)) > 0;
                    c.put32(random_cursor, c.r(3));
                    if (beyond_pool) {
                        block(c, pc + 24, 2); c.put32(random_cursor, random_start);
                        c.call(0x1077b770, pc + 32);
                        block(c, pc + 32, 2);
                        c.set(0, std::uint16_t(c.r(3))); c.put32(random_seed, c.r(0));
                    }
                    block(c, pc + 40, retry ? 10 : 11);
                    c.set(3, c.u32(random_cursor)); c.set(0, c.u32(random_seed));
                    c.set(3, c.u16(c.r(3)));
                    const auto sample = std::int32_t(c.r(3) + c.r(0));
                    c.set(3, std::uint32_t(sample) >> 31);
                    c.set(0, std::uint32_t(sample % 4));
                };
                block(c, 0x104f6888, 3);
                guest::Kept<17> group(c, 0);
                guest::Kept<25> group_offset(c, 0);
                for (;;) {
                    block(c, 0x104f6998, 3);
                    c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(group)))));
                    if (compare(c, std::int32_t(c.r(0)), 4) >= 0) break;
                    block(c, 0x104f6894, 3);
                    guest::Kept<18> pot(c, 0);
                    guest::Kept<24> pot_base(c, 0);
                    for (;;) {
                        block(c, 0x104f6984, 3);
                        c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(pot)))));
                        if (compare(c, std::int32_t(c.r(0)), 4) >= 0) break;
                        draw(0x104f68a0, false);
                        for (;;) {
                            block(c, 0x104f6944, 7);
                            c.set(0, guest::extended(std::int16_t(c.r(0)))); c.set(3, c.u32(competition + 0xec));
                            c.set(0, c.r(0) + pot_base); c.set(5, c.r(0) * 4);
                            c.set(4, c.u32(c.r(3) + c.r(5)));
                            if (compare_logical(c, c.r(4))) break;
                            draw(0x104f68f4, true);
                        }
                        block(c, 0x104f6960, 9);
                        c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(pot)))));
                        c.set(0, 0); c.set(3, c.r(3) + group_offset); pot_base = pot_base + 4;
                        c.set(3, c.r(3) * 4); pot = pot + 1;
                        c.put32(draw_buffer + c.r(3), c.r(4));
                        c.set(3, c.u32(competition + 0xec)); c.put32(c.r(3) + c.r(5), 0);
                    }
                    block(c, 0x104f6990, 2); group_offset = group_offset + 4; group = group + 1;
                }
                block(c, 0x104f69a4, 2);
                c.set(3, c.u32(competition + 0xec)); c.call(0x10775170, 0x104f69ac);
                block(c, 0x104f69ac, 2); c.put32(competition + 0xec, draw_buffer); c.set(3, 1);
            }
        }
    }
    block(c, 0x104f69b4, 5);
    c.set(0, c.u32(frame + 0xe8)); c.set(1, stack); c.set_lr(c.r(0));
    guest::transfer_saved_registers<15, true>(c, stack); c.finish();
}

void update_daily(Cpu& c) {
    GuestNativeScope scope(c.s, daily_entry);
    block(c, daily_entry, 12);
    const auto stack = c.sp(), frame = stack - 0x120;
    c.set(0, c.lr());
    guest::transfer_saved_registers<24>(c, stack);
    const guest::Kept<28> mode(c, c.r(5));
    const guest::Kept<29> success(c, c.toc(-0x7bac));
    const guest::Kept<30> message(c, c.toc(-0x7ba8));
    const guest::Kept<26> competition(c, c.r(3));
    const guest::Kept<31> file(c, c.toc(-0x45ac));
    const guest::Kept<27> date(c, c.r(4));
    c.put32(stack + 8, c.r(0));
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(mode)))));
    const auto needs_calendar = record_compare(c, std::int32_t(c.r(0))) != 0;
    c.put32(frame, stack); c.set(1, frame); scope.frame(frame);

    const auto fail = [&](unsigned pc, unsigned name, unsigned extension, unsigned line) {
        report_failure(c, pc, file, message, name, extension, line);
        block(c, pc + 72, 4); c.set({{0, 0}, {3, 0}}); c.put8(success, 0);
        return false;
    };
    const auto start_tournament = [&] {
        block(c, 0x104f6ad8, 2); c.set(3, competition);
        call_owned(c, draw_entry, 0x104f6ae0);
        block(c, 0x104f6ae0, 4); c.set(3, competition);
        c.set(12, c.u32(competition)); c.set(12, c.u32(c.r(12) + 0x64));
        call_virtual(c, 0x104f6af0);
        block(c, 0x104f6af0, 6);
        c.set(2, c.u32(frame + 0x14)); c.set(0, 4); c.put16(competition + 0x3e, 4);
        c.set(0, guest::extended(c.i16(competition + 0x3e)));
        c.set(3, c.r(0) * 0x44); c.call(0x10775130, 0x104f6b08);
        block(c, 0x104f6b08, 3);
        const auto allocated = compare_logical(c, c.r(3)) != 0;
        c.put32(competition + 0xb0, c.r(3));
        if (!allocated) {
            report_failure(c, 0x104f6b14, file, message, 0x78, 0x40, 0x1f1);
            block(c, 0x104f6b5c, 3); c.set(0, 0); c.put8(success, 0);
        } else {
            block(c, 0x104f6b68, 3);
            guest::Kept<25> index(c, 0);
            guest::Kept<24> offset(c, 0);
            for (;;) {
                block(c, 0x104f6b98, 4);
                c.set(0, guest::extended(c.i16(competition + 0x3e)));
                c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(index)))));
                if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) >= 0) break;
                block(c, 0x104f6b74, 7);
                c.set(5, c.u32(competition + 0xec)); c.set({{3, competition}, {4, index}, {6, 0}});
                c.set(5, c.u32(c.r(5) + offset)); c.set(7, 0);
                c.call(0x104a60e0, 0x104f6b90);
                block(c, 0x104f6b90, 2); offset = offset + 4; index = index + 1;
            }
            block(c, 0x104f6ba8, 2); c.set(3, competition); c.call(0x104a0e40, 0x104f6bb0);
            block(c, 0x104f6bb0, 1); c.set(0, 1);
        }
        block(c, 0x104f6bb4, 2);
        if (compare(c, std::int32_t(c.r(0))) == 0) return fail(0x104f6bbc, 0xd8, 0x68, 0x3c2);

        block(c, 0x104f6c14, 2); c.set(3, competition); c.call(0x104a00a0, 0x104f6c1c);
        block(c, 0x104f6c1c, 2);
        if (compare(c, std::int32_t(c.r(3))) == 0) return fail(0x104f6c24, 0xb8, 0x60, 0x3c8);
        block(c, 0x104f6c7c, 3); c.set({{3, competition}, {4, 0}});
        c.call(0x1049eb30, 0x104f6c88);
        block(c, 0x104f6c88, 2);
        if (compare(c, std::int32_t(c.r(3))) == 0) return fail(0x104f6c90, 0x98, 0x58, 0x3cf);

        block(c, 0x104f6ce8, 1);
        guest::Kept<24> group(c, 0);
        do {
            block(c, 0x104f6cec, 3); c.set({{3, competition}, {4, group}});
            call_owned(c, group_entry, 0x104f6cf8);
            block(c, 0x104f6cf8, 3); group = group + 1;
        } while (compare(c, std::int32_t(std::uint32_t(group)), 3) < 0);
        block(c, 0x104f6d04, 2); c.set(3, c.u32(competition + 0xec));
        c.call(0x10775170, 0x104f6d0c);
        block(c, 0x104f6d0c, 5); c.set({{3, 0}, {0, 1}});
        c.put32(competition + 0xec, 0); c.put16(competition + 0xf0, 0);
        c.put8(competition + 0x45, 1);
        return true;
    };

    if (!needs_calendar) {
        block(c, 0x104f6a00, 1); c.call(0x104a1540, 0x104f6a04);
        block(c, 0x104f6a04, 1);
    } else {
        block(c, 0x104f6a08, 7);
        c.set(5, guest::extended(c.i16(competition + 0xb8))); c.set(3, frame + 0x70);
        c.set(0, guest::extended(c.i16(competition + 0x40)));
        c.set(4, guest::extended(c.i16(competition + 0xb6)));
        c.set(0, c.r(5) + c.r(0)); c.set(5, std::uint16_t(c.r(0)));
        c.call(0x107156b0, 0x104f6a24);
        block(c, 0x104f6a24, 10);
        c.set(3, c.u32(date)); c.set(0, c.u32(date + 4)); c.put32(frame + 0x50, c.r(3));
        c.set(5, c.u16(frame + 0x72)); c.set(4, c.u16(frame + 0x52)); c.put32(frame + 0x38, c.r(3));
        const auto year_order = compare_logical(c, c.r(4), c.r(5));
        c.put32(frame + 0x3c, c.r(0)); c.put32(frame + 0x54, c.r(0));
        if (year_order < 0) {
            block(c, 0x104f6a4c, 2); c.set(0, 1);
        } else {
            block(c, 0x104f6a54, 1);
            if (year_order > 0) {
                block(c, 0x104f6a58, 2); c.set(0, 0);
            } else {
                block(c, 0x104f6a60, 8);
                const auto today = c.i16(frame + 0x50);
                c.set(0, guest::extended(today));
                const auto deadline = c.i16(frame + 0x70);
                c.set(4, guest::extended(deadline));
                const auto day_bits = std::int32_t(c.r(0) ^ c.r(4));
                // Keep the shift's carry and scratch result at the ABI boundary.
                c.s.wr<1>(guest::reg::xer_ca, day_bits < 0 && (day_bits & 1));
                c.set(3, std::uint32_t(day_bits >> 1));
                c.set(0, today < deadline);
            }
        }
        block(c, 0x104f6a80, 2);
        if (record_compare(c, std::int32_t(c.r(0))) != 0) {
            block(c, 0x104f6a88, 2); c.set(3, 0);
        } else {
            block(c, 0x104f6a90, 8);
            c.set(0, c.u32(frame + 0x38)); c.set(4, 0); c.set(3, c.u32(frame + 0x3c));
            c.put32(frame + 0x48, c.r(0)); c.set(0, c.u16(frame + 0x4a)); c.put32(frame + 0x4c, c.r(3));
            if (compare_logical(c, c.r(0), c.r(5)) == 0) {
                block(c, 0x104f6ab0, 4);
                c.set(3, guest::extended(c.i16(frame + 0x48)));
                c.set(0, guest::extended(c.i16(frame + 0x70)));
                if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0) {
                    block(c, 0x104f6ac0, 1); c.set(4, 1);
                }
            }
            block(c, 0x104f6ac4, 2);
            bool proceed = true;
            if (compare(c, std::int32_t(c.r(4))) != 0) {
                block(c, 0x104f6acc, 3);
                c.set(0, guest::extended(c.i8(competition + 0x45)));
                if (record_compare(c, std::int32_t(c.r(0))) == 0) proceed = start_tournament();
            }
            if (proceed) {
                block(c, 0x104f6d20, 4); c.set({{3, competition}, {4, date}, {5, mode}});
                c.call(0x104a1540, 0x104f6d30);
            }
        }
    }
    block(c, 0x104f6d30, 5);
    c.set(0, c.u32(frame + 0x128)); c.set(1, stack); c.set_lr(c.r(0));
    guest::transfer_saved_registers<24, true>(c, stack); c.finish();
}

void destroy_competition(Cpu& c) {
    GuestNativeScope scope(c.s, destroy_entry);
    block(c, destroy_entry, 10);
    const auto stack = c.sp(), frame = stack - 0x50;
    c.set(0, c.lr());
    c.put32(stack - 4, c.r(31));
    c.put32(stack - 8, c.r(30));
    c.put32(stack - 12, c.r(29));
    const guest::Kept<29> delete_mode(c, c.r(4));
    c.put32(stack - 16, c.r(28));
    const guest::Kept<28> competition(c, c.r(3));
    const auto present = record_compare(c, std::int32_t(std::uint32_t(competition)));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    if (present) {
        block(c, 0x104f4f08, 4);
        c.set(0, c.toc(-0x6078));
        c.set(4, 0);
        c.put32(competition, c.r(0));
        c.call(0x104a6660, 0x104f4f18);
        block(c, 0x104f4f18, 3);
        c.set(3, c.u32(competition + 0xbc));
        if (compare_logical(c, c.r(3))) {
            block(c, 0x104f4f24, 1);
            c.call(0x10775170, 0x104f4f28);
            block(c, 0x104f4f28, 2);
            c.set(0, 0);
            c.put32(competition + 0xbc, 0);
        }
        block(c, 0x104f4f30, 3);
        guest::Kept<30> index(c, 0);
        guest::Kept<31> offset(c, 0);
        for (;;) {
            block(c, 0x104f4f6c, 3);
            c.set(0, c.u32(competition + 0x30));
            if (compare(c, std::int32_t(index), std::int32_t(c.r(0))) > 0) break;
            block(c, 0x104f4f40, 4);
            c.set(3, c.u32(competition + 0xc));
            c.set(3, c.u32(c.r(3) + offset));
            if (compare_logical(c, c.r(3))) {
                block(c, 0x104f4f50, 4);
                c.set(12, c.u32(c.r(3)));
                c.set(4, 1);
                c.set(12, c.u32(c.r(12) + 8));
                call_virtual(c, 0x104f4f60);
                block(c, 0x104f4f60, 1);
                c.set(2, c.u32(frame + 0x14));
            }
            block(c, 0x104f4f64, 2);
            offset = offset + 4;
            index = index + 1;
        }
        block(c, 0x104f4f78, 3);
        c.set(3, c.u32(competition + 0xc));
        if (compare_logical(c, c.r(3))) {
            block(c, 0x104f4f84, 1);
            c.call(0x10775170, 0x104f4f88);
            block(c, 0x104f4f88, 2);
            c.set(0, 0);
            c.put32(competition + 0xc, 0);
        }
        block(c, 0x104f4f90, 3);
        c.set(3, c.u32(competition + 8));
        if (compare_logical(c, c.r(3))) {
            block(c, 0x104f4f9c, 2);
            c.set(4, 1);
            c.call(0x104536c0, 0x104f4fa4);
        }
        block(c, 0x104f4fa4, 3);
        c.set(3, competition);
        c.set(4, 0);
        c.call(0x1049e600, 0x104f4fb0);
        block(c, 0x104f4fb0, 2);
        c.set(0, std::uint32_t(std::int32_t(std::int16_t(std::uint32_t(delete_mode)))));
        if (record_compare(c, std::int32_t(c.r(0))) > 0) {
            block(c, 0x104f4fb8, 2);
            c.set(3, competition);
            c.call(0x107735a0, 0x104f4fc0);
        }
    }
    block(c, 0x104f4fc0, 9);
    c.set(3, competition);
    c.set(0, c.u32(frame + 0x58));
    c.set(1, stack);
    c.set_lr(c.r(0));
    for (unsigned n = 0; n < 4; ++n) c.set(31 - n, c.u32(stack - 4 * (n + 1)));
    c.finish();
}

void update_child(Cpu& c) {
    GuestNativeScope scope(c.s, child_update_entry);
    block(c, child_update_entry, 8);
    const auto stack = c.sp(), frame = stack - 0x40;
    c.set(0, c.lr());
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(3, c.u32(c.r(3) + 0xc));
    c.set(3, c.u32(c.r(3) + 0xc));
    c.set(12, c.u32(c.r(3)));
    c.set(12, c.u32(c.r(12) + 0x38));
    call_virtual(c, 0x104f5d80);
    block(c, 0x104f5d80, 5);
    c.set(2, c.u32(frame + 0x14));
    c.set(0, c.u32(frame + 0x48));
    c.set(1, stack);
    c.set_lr(c.r(0));
    c.finish();
}

std::uint32_t normalize_middle_placing(Cpu& c, std::uint32_t placing) {
    block(c, 0x104f7040, 3);
    const auto value = std::uint8_t(placing);
    c.set(0, value);
    const auto order = compare(c, value, 3);
    if (!order) {
        block(c, 0x104f7070, 2);
        return 9;
    }
    block(c, 0x104f704c, 1);
    if (order > 0) {
        block(c, 0x104f705c, 2);
        if (compare(c, value, 5) >= 0) return placing;
        block(c, 0x104f7064, 1);
        block(c, 0x104f7078, 1);
        return 13;
    }
    block(c, 0x104f7050, 2);
    if (compare(c, value, 1) >= 0) {
        block(c, 0x104f7068, 2);
        return 1;
    }
    block(c, 0x104f7058, 1);
    return placing;
}

std::uint32_t normalize_outer_placing(Cpu& c, std::uint32_t placing, std::uint32_t start) {
    block(c, start, 3);
    const auto value = std::uint8_t(placing);
    c.set(0, value);
    const auto order = compare(c, value, 4);
    if (!order) {
        block(c, start + 0x30, 1);
        return 13;
    }
    block(c, start + 0xc, 1);
    if (order > 0) return placing;
    block(c, start + 0x10, 2);
    const auto first = compare(c, value, 1);
    if (!first) {
        block(c, start + 0x20, 2);
        return 5;
    }
    block(c, start + 0x18, 1);
    if (first > 0) {
        block(c, start + 0x28, 2);
        return 9;
    }
    block(c, start + 0x1c, 1);
    return placing;
}

void write_event_text(Cpu& c) {
    GuestNativeScope scope(c.s, event_text_entry);
    block(c, event_text_entry, 16);
    const auto stack = c.sp(), frame = stack - 0x50;
    c.set(0, c.lr());
    guest::transfer_saved_registers<27>(c, stack);
    const guest::Kept<31> club_index(c, c.r(4));
    const guest::Kept<28> message(c, c.toc(-0x7bb8));
    const guest::Kept<27> clubs(c, c.toc(-0x7be8));
    guest::Kept<30> competition(c, c.r(3));
    c.set(4, c.toc(-0x45ac));
    c.put32(stack + 8, c.r(0));
    const auto phase = std::int8_t(c.r(6));
    c.set(0, std::uint32_t(std::int32_t(phase)));
    const auto stage_order = compare(c, phase, 3);
    c.set(6, c.r(5));
    c.put32(frame, stack); c.set(1, frame); scope.frame(frame);
    c.set(11, c.u8(stack + 0x3b)); c.set(10, c.u8(stack + 0x3f));
    const guest::Kept<29> output(c, c.u32(stack + 0x40));

    const auto read_kind = [&] {
        const auto kind = std::int8_t(c.r(6));
        c.set(0, std::uint32_t(std::int32_t(kind)));
        return kind;
    };
    const auto has_detail = [&] {
        c.set(0, std::uint32_t(std::int32_t(std::int8_t(c.r(10)))));
        return record_compare(c, std::int32_t(c.r(0))) != 0;
    };
    const auto forward_group = [&](std::uint32_t pc) {
        block(c, pc, 6);
        c.set(5, c.u32(competition + 4));
        c.set({{4, c.r(7)}, {3, club_index}, {7, c.r(10)}, {8, output}});
        c.call(0x104698c0, pc + 24);
        block(c, pc + 24, 1);
    };
    const auto copy_message = [&](std::uint32_t pc) {
        block(c, pc, 3); c.set({{3, output}, {4, message}});
        c.call(0x1074b830, pc + 12);
    };
    if (stage_order < 0) {
        block(c, 0x104f5950, 3);
        const auto kind = read_kind();
        if (compare(c, kind, 6) == 0) {
            block(c, 0x104f595c, 2);
            if (has_detail()) {
                forward_group(0x104f5964);
            } else {
                block(c, 0x104f5980, 11);
                competition = club_index * 0x248;
                c.set(0, c.u32(clubs)); c.set(3, message); c.set(4, c.r(4) + 0x27);
                c.set(5, c.r(0) + competition); c.set(5, c.u8(c.r(5) + 0x23b));
                c.set(7, c.r(0) + competition + 0x221);
                c.set(5, std::uint32_t(std::int32_t(std::int8_t(c.r(5))))); c.set(6, c.r(5));
                c.call(0x10727890, 0x104f59ac);
                copy_message(0x104f59ac);
                block(c, 0x104f59b8, 6);
                c.set(0, c.u32(clubs)); c.set({{3, output}, {5, 2005}});
                c.set(6, c.r(0) + competition); c.set(4, c.r(6) + 0x221);
                c.call(0x1074bab0, 0x104f59d0);
                block(c, 0x104f59d0, 1);
            }
        } else {
            block(c, 0x104f59d4, 2);
            if (compare(c, kind, -2) == 0) forward_group(0x104f59dc);
        }
    } else {
        block(c, 0x104f59f8, 1);
        if (stage_order == 0) {
            block(c, 0x104f59fc, 2);
            if (has_detail()) {
                block(c, 0x104f5a04, 6);
                c.set({{4, c.r(8)}, {3, club_index}, {5, c.r(11)}, {7, c.r(9)}, {8, output}});
                c.call(0x10464d00, 0x104f5a1c);
                block(c, 0x104f5a1c, 1);
            } else {
                struct TextChoice { unsigned test, body, offset; };
                constexpr std::array<TextChoice, 4> choices{{
                    {0x104f5a20, 0x104f5a2c, 0x69}, {0x104f5a70, 0x104f5a78, 0xb9},
                    {0x104f5abc, 0x104f5ac4, 0x11a}, {0x104f5b08, 0x104f5b10, 0x17c},
                }};
                const auto format_final = [&](unsigned pc, unsigned offset) {
                    block(c, pc, 16);
                    c.set(8, club_index * 0x248); c.set(6, c.u32(competition + 4));
                    c.set(0, c.u32(clubs)); c.set(3, message); c.set(7, c.u8(c.r(6) + 0x67));
                    c.set(4, c.r(4) + offset); c.set(5, c.r(0) + c.r(8)); c.set(9, c.r(8) + 0x221);
                    c.set(5, c.u8(c.r(5) + 0x23b));
                    c.set(7, std::uint32_t(std::int32_t(std::int8_t(c.r(7))))); c.set(8, c.r(7));
                    c.set(9, c.r(0) + c.r(9));
                    c.set(5, std::uint32_t(std::int32_t(std::int8_t(c.r(5)))));
                    c.set(10, c.r(6) + 0x4d); c.set(6, c.r(5)); c.call(0x10727890, pc + 64);
                };
                bool selected = false;
                std::int8_t kind = 0;
                for (unsigned i = 0; i < choices.size(); ++i) {
                    const auto [test, body, offset] = choices[i];
                    block(c, test, i == 0 ? 3 : 2);
                    if (i == 0) kind = read_kind();
                    if (compare(c, kind, i + 1) == 0) {
                        format_final(body, offset); block(c, body + 64, 1);
                        selected = true;
                        break;
                    }
                }
                if (!selected) {
                    block(c, 0x104f5b54, 2);
                    if (compare(c, kind, -1) == 0) {
                        // Both following branches test the same unchanged CR.
                        block(c, 0x104f5ba0, 1); block(c, 0x104f5be8, 1);
                        format_final(0x104f5bec, 0x244);
                    } else {
                        format_final(0x104f5b5c, 0x1df); block(c, 0x104f5b9c, 1);
                    }
                }
                copy_message(0x104f5c2c);
                block(c, 0x104f5c38, 7);
                c.set(0, club_index * 0x248); c.set(4, c.u32(clubs));
                c.set({{3, output}, {5, 2005}}); c.set(6, c.r(4) + c.r(0)); c.set(4, c.r(6) + 0x221);
                c.call(0x1074bab0, 0x104f5c54);
                block(c, 0x104f5c54, 5);
                c.set(6, c.u32(competition + 4)); c.set({{3, output}, {5, 2000}}); c.set(4, c.r(6) + 0x4d);
                c.call(0x1074bab0, 0x104f5c68);
            }
        }
    }
    block(c, 0x104f5c68, 5);
    c.set(0, c.u32(frame + 0x58)); c.set(1, stack); c.set_lr(c.r(0));
    guest::transfer_saved_registers<27, true>(c, stack); c.finish();
}

void publish_final_placements(Cpu& c) {
    GuestNativeScope scope(c.s, final_placement_entry);
    block(c, final_placement_entry, 11);
    const auto stack = c.sp(), frame = stack - 0xa0;
    c.set(0, c.lr());
    guest::transfer_saved_registers<27>(c, stack);
    const guest::Kept<27> success(c, c.toc(-0x7bac));
    const guest::Kept<30> competition(c, c.r(3));
    const guest::Kept<28> message(c, c.toc(-0x7ba8));
    const guest::Kept<29> file(c, c.toc(-0x45ac));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack); c.set(1, frame); scope.frame(frame);
    c.set(0, c.u32(competition + 8));
    if (!compare_logical(c, c.r(0))) {
        report_failure(c, 0x104f6d7c, file, message, 0x68, 0x40, 0x3f4);
        block(c, 0x104f6dc4, 3); c.set(0, 0); c.put8(success, 0);
    } else {
        const guest::Field<std::int16_t> entrant_count{c, competition + 0xf0};
        block(c, 0x104f6dd0, 3);
        c.set(0, std::uint32_t(std::int32_t(entrant_count)));
        c.set(3, c.r(0) * 4);
        c.call(0x10775130, 0x104f6ddc);
        block(c, 0x104f6ddc, 2);
        const guest::Kept<31> sorted(c, c.r(3));
        if (!record_compare(c, std::int32_t(std::uint32_t(sorted)))) {
            report_failure(c, 0x104f6de4, file, message, 0x48, 0x38, 0x3fc);
            block(c, 0x104f6e2c, 3); c.set(0, 0); c.put8(success, 0);
        } else {
            block(c, 0x104f6e38, 4);
            c.set(0, std::uint32_t(std::int32_t(entrant_count)));
            c.set(4, c.u32(competition + 0xec));
            c.set(5, c.r(0) * 4);
            c.call(0x10739ef0, 0x104f6e48);
            block(c, 0x104f6e48, 5);
            c.set(4, std::uint32_t(std::int32_t(entrant_count)));
            c.set(3, sorted); c.set(6, c.toc(-0x6188)); c.set(5, 4);
            c.call(0x10768300, 0x104f6e5c);
            constexpr std::array ranks{1u, 2u, 3u, 4u, 5u, 5u, 5u, 5u,
                                       9u, 9u, 9u, 9u, 13u, 13u, 13u, 13u};
            // The original always publishes sixteen ranks after sorting,
            // reloading the live standings table before each publication.
            for (unsigned index = 0; index < ranks.size(); ++index) {
                const auto pc = 0x104f6e5c + 24 * index;
                block(c, pc, 6);
                c.set(0, index);
                c.set(3, c.u32(competition + 8));
                c.set(0, index * 4); c.set(5, ranks[index]);
                c.set(4, c.u32(sorted + c.r(0)));
                c.call(0x10457490, pc + 24);
            }
            block(c, 0x104f6fdc, 2); c.set(3, sorted);
            c.call(0x10775170, 0x104f6fe4);
        }
    }
    block(c, 0x104f6fe4, 5);
    c.set(0, c.u32(frame + 0xa8)); c.set(1, stack); c.set_lr(c.r(0));
    guest::transfer_saved_registers<27, true>(c, stack); c.finish();
}

void set_placements(Cpu& c) {
    GuestNativeScope scope(c.s, placement_entry);
    block(c, placement_entry, 13);
    const auto stack = c.sp(), frame = stack - 0x50;
    c.set(0, c.lr());
    // The individual saves and argument retention are interleaved in the
    // original. Keep their order for aliases and memory observers.
    c.put32(stack - 4, c.r(31));
    guest::Kept<31> third(c, c.r(8));
    c.put32(stack - 8, c.r(30));
    guest::Kept<30> second(c, c.r(7));
    c.put32(stack - 12, c.r(29));
    guest::Kept<29> first(c, c.r(6));
    c.put32(stack - 16, c.r(28));
    const guest::Kept<28> phase(c, c.r(5));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(3, c.u32(c.r(3) + 8));
    c.call(0x10459b50, 0x104f7034);
    block(c, 0x104f7034, 3);
    const auto stage = std::int8_t(std::uint32_t(phase));
    c.set(0, std::uint32_t(std::int32_t(stage)));
    if (compare(c, stage, 3) < 0) {
        second = normalize_middle_placing(c, second);
        third = normalize_outer_placing(c, third, 0x104f707c);
        first = normalize_outer_placing(c, first, 0x104f70b0);
    }
    block(c, 0x104f70e4, 11);
    const auto record = c.r(3);
    c.put8(record + 0x76, first);
    c.put8(record + 0x77, second);
    c.put8(record + 0x78, third);
    c.set(0, c.u32(frame + 0x58));
    c.set(1, stack);
    c.set_lr(c.r(0));
    for (unsigned n = 0; n < 4; ++n) c.set(31 - n, c.u32(stack - 4 * (n + 1)));
    c.finish();
}

void notify_result(Cpu& c) {
    GuestNativeScope scope(c.s, notification_entry);
    block(c, notification_entry, 14);
    const auto stack = c.sp(), frame = stack - 0x50;
    c.set(0, c.lr());
    c.put32(stack - 4, c.r(31));
    const guest::Kept<31> manager_slot(c, c.toc(-0x7b48));
    c.put32(stack - 8, c.r(30));
    const guest::Kept<30> event(c, c.r(8));
    c.put32(stack - 12, c.r(29));
    const guest::Kept<29> team(c, c.r(4));
    c.put32(stack - 16, c.r(28));
    const guest::Kept<28> competition(c, c.r(3));
    c.put32(stack + 8, c.r(0));
    const auto phase = std::int8_t(c.r(6));
    c.set(0, std::uint32_t(std::int32_t(phase)));
    const auto stage_order = compare(c, phase, 3);
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    if (stage_order < 0) {
        block(c, 0x104f5dd8, 3);
        const auto kind = std::int8_t(c.r(5));
        c.set(0, std::uint32_t(std::int32_t(kind)));
        if (!compare(c, kind, 6)) {
            block(c, 0x104f5de4, 6);
            c.set(3, c.u32(manager_slot));
            c.set(6, 0);
            c.set(5, c.u32(competition + 4));
            c.set({{7, 120}, {8, 25}});
            c.call(0x10384500, 0x104f5dfc);
            block(c, 0x104f5dfc, 1);
        } else {
            block(c, 0x104f5e00, 2);
            if (!compare(c, kind, -2)) {
                block(c, 0x104f5e08, 5);
                c.set(3, c.u32(manager_slot));
                c.set(6, 0x41c);
                c.set(5, c.u32(competition + 4));
                c.set(7, 25);
                c.call(0x10384820, 0x104f5e1c);
                block(c, 0x104f5e1c, 1);
            }
        }
    } else {
        block(c, 0x104f5e20, 1);
        if (!stage_order) {
            block(c, 0x104f5e24, 2);
            const auto kind = std::int8_t(c.r(5));
            const auto upper = c.cr(0) & 0xf0;
            c.set(0, std::uint32_t(std::int32_t(kind)));
            compare(c, kind);
            c.set_cr(0, std::uint8_t(upper | c.cr(0)));
            if (!kind) {
                block(c, 0x104f5e2c, 5);
                c.set(5, std::uint32_t(c.i16(event + 0x34)));
                c.set({{3, 40}, {4, 100}, {6, 3}});
                c.call(0x10476780, 0x104f5e40);
                block(c, 0x104f5e40, 13);
                c.set(5, c.u32(competition + 0xc));
                c.set(8, c.r(3));
                c.set(0, std::uint32_t(c.i16(event + 0x34)));
                c.set(4, team);
                c.set(5, c.u32(c.r(5) + 0xc));
                c.set(0, c.r(0) * 0x78);
                c.set(3, c.u32(manager_slot));
                c.set(6, c.u32(c.r(5) + 0xa0));
                c.set(5, c.u32(competition + 4));
                c.set(7, c.r(6) + c.r(0));
                c.set(6, std::uint32_t(c.i16(event + 0x32)));
                c.set(7, std::uint32_t(c.i16(c.r(7) + 0x80)));
                c.call(0x10384500, 0x104f5e74);
                block(c, 0x104f5e74, 1);
            } else {
                block(c, 0x104f5e78, 3);
                c.set(0, std::uint32_t(std::int32_t(kind)));
                if (!compare(c, kind, -1)) {
                    block(c, 0x104f5e84, 5);
                    c.set(5, std::uint32_t(c.i16(event + 0x34)));
                    c.set({{3, 40}, {4, 100}, {6, 3}});
                    c.call(0x10476780, 0x104f5e98);
                    block(c, 0x104f5e98, 7);
                    c.set(8, c.r(3));
                    c.set(3, c.u32(manager_slot));
                    c.set(5, c.u32(competition + 4));
                    c.set(4, team);
                    c.set(6, std::uint32_t(c.i16(event + 0x32)));
                    c.set(7, std::uint32_t(c.i16(event + 0x30)));
                    c.call(0x103846e0, 0x104f5eb4);
                    block(c, 0x104f5eb4, 1);
                } else {
                    block(c, 0x104f5eb8, 2);
                    if (compare(c, kind, 4)) {
                        block(c, 0x104f5ec0, 2);
                        if (!compare(c, kind, 1)) {
                            block(c, 0x104f5ec8, 4);
                            c.set(3, c.u32(manager_slot));
                            c.set({{5, event}, {6, c.r(9)}});
                            c.call(0x10383410, 0x104f5ed8);
                            block(c, 0x104f5ed8, 1);
                        } else {
                            block(c, 0x104f5edc, 2);
                            if (!compare(c, kind, 2)) {
                                block(c, 0x104f5ee4, 4);
                                c.set(3, c.u32(manager_slot));
                                c.set({{5, event}, {6, c.r(9)}});
                                c.call(0x10383b70, 0x104f5ef4);
                                block(c, 0x104f5ef4, 1);
                            } else {
                                block(c, 0x104f5ef8, 2);
                                if (!compare(c, kind, 3)) {
                                    block(c, 0x104f5f00, 4);
                                    c.set(3, c.u32(manager_slot));
                                    c.set({{5, event}, {6, c.r(9)}});
                                    c.call(0x10384020, 0x104f5f10);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    block(c, 0x104f5f10, 9);
    c.set(3, 0);
    c.set(0, c.u32(frame + 0x58));
    c.set(1, stack);
    c.set_lr(c.r(0));
    for (unsigned n = 0; n < 4; ++n) c.set(31 - n, c.u32(stack - 4 * (n + 1)));
    c.finish();
}

struct Routine { std::uint32_t entry; void (*run)(Cpu&); };
constexpr auto routines = std::to_array<Routine>({
#define AFRICAN_NATIONS_ROUTINE(entry, function) Routine{entry, function},
#include "african_nations_routines.inc"
#undef AFRICAN_NATIONS_ROUTINE
});
const Routine& find_routine(std::uint32_t entry) {
    const auto found = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    if (found == routines.end() || found->entry != entry)
        throw std::logic_error("Unknown AFRICAN_NATIONS.CPP entry " + hex_address(entry));
    return *found;
}

void call_owned(Cpu& c, std::uint32_t entry, std::uint32_t resume) {
    if (routine_checker || c.s.import_descriptor_observer || c.s.scalar_capture || dispatch_probe) { c.call(entry, resume); return; }
    const auto stack = c.sp(); c.link(resume); c.s.pc = entry;
    find_routine(entry).run(c);
    if (c.s.pc != resume || c.sp() != stack)
        throw std::logic_error("African Nations owned call violated its return boundary");
}

void call_virtual(Cpu& c, std::uint32_t resume) {
    if (routine_checker || c.s.import_descriptor_observer || c.s.scalar_capture || dispatch_probe) {
        c.call(0x10781320, resume); return;
    }
    const auto stack = c.sp();
    c.link(resume);
    c.s.pc = 0x10781320;
    // Diagnostics use the ordinary dispatcher above. The normal route calls
    // known owners directly, retaining original descriptor-body controls.
    if (!try_modern_import_descriptor(c.s, 0x10781320) && !dispatch_page(c.s))
        throw std::logic_error("Missing African Nations virtual descriptor body");
    const auto target = std::ranges::lower_bound(routines, c.s.pc, {}, &Routine::entry);
    if (target != routines.end() && target->entry == c.s.pc) {
        target->run(c);
        if (c.s.pc != resume) throw std::logic_error("African Nations virtual owner did not return to its caller");
    } else run_until(c.s, resume);
    if (c.sp() != stack) throw std::logic_error("African Nations virtual call returned on a different stack");
}
}

void african_nations_run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, declining); }
void african_nations_run_rewrite(State& s, std::uint32_t entry) { Cpu c(s); find_routine(entry).run(c); }
bool try_modern_african_nations(State& s, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(s, entry, african_nations_run_original, african_nations_run_rewrite, RoutineChecker::result_r3);
    african_nations_run_rewrite(s, entry);
    return true;
}
}
