#include "african_nations_adapter.hpp"
#include "guest_values.hpp"
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
