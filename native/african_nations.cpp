#include "african_nations_adapter.hpp"
#include "guest_values.hpp"
#include "guest_saved_registers.hpp"
#include "routine_check.hpp"

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
constexpr auto notification_entry = 0x104f5da0u;
constexpr auto destroy_entry = 0x104f4ee0u;
constexpr auto child_update_entry = 0x104f5d60u;
constexpr auto calendar_entry = 0x104f4ff0u;

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

void calendar_allocation_failure(Cpu& c, std::uint32_t pc, unsigned name, unsigned extension, unsigned line) {
    block(c, pc, 6);
    c.set({{3, c.r(28)}, {6, c.sp() + name}, {7, c.sp() + extension}, {4, 0}, {5, 0}});
    c.call(0x10739890, pc + 24);
    block(c, pc + 24, 1);
    c.call(0x10768810, pc + 28);
    block(c, pc + 28, 7);
    c.set({{5, c.r(3)}, {3, c.r(27)}, {4, c.r(28) + 0x14},
           {6, c.sp() + name}, {7, c.sp() + extension}, {8, line}});
    c.call(0x1077b720, pc + 56);
    block(c, pc + 56, 4);
    c.set({{4, c.r(27)}, {3, c.r(28) + 0x21}, {5, 0}});
    c.call(0x10769b60, pc + 72);
    block(c, pc + 72, 4);
    c.set({{0, 0}, {3, 0}});
    c.put8(c.r(31), 0);
}

void build_calendar(Cpu& c) {
    GuestNativeScope scope(c.s, calendar_entry);
    block(c, calendar_entry, 11);
    const auto stack = c.sp(), frame = stack - 0xb0;
    c.set(0, c.lr());
    guest::transfer_saved_registers<26>(c, stack);
    c.set(31, c.toc(-0x7bac));
    c.set(29, c.r(3));
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
                c.call(0x10781320, 0x104f4f60);
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
    c.call(0x10781320, 0x104f5d80);
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
