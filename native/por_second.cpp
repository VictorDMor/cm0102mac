#include "por_second_adapter.hpp"
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

bool declining = std::getenv("CM_POR_SECOND_ORIGINAL") != nullptr;
constexpr auto collect_entry = 0x10578680u;
constexpr auto calendar_entry = 0x10577800u, configure_entry = 0x10578840u;
constexpr auto create_entry = 0x105773c0u, destroy_entry = 0x105776f0u, season_entry = 0x10578940u;
constexpr unsigned club_stride = 0x248, standing_size = 0x44;
void call_virtual(Cpu&, std::uint32_t resume);

void block(Cpu& c, std::uint32_t pc, unsigned instructions) {
    c.s.step(pc);
    c.s.charge(instructions - 1);
}

int record_value(Cpu& c, unsigned destination, std::int32_t value) {
    const auto upper = c.cr(0) & 0xf0;
    c.set(destination, std::uint32_t(value));
    const auto sign = compare(c, value);
    c.set_cr(0, std::uint8_t(upper | c.cr(0)));
    return sign;
}

struct Division {
    Cpu& c;
    std::uint32_t address;
    guest::Field<std::int16_t> club_count() const { return {c, address + 0x3e}; }
    guest::Field<std::uint32_t> standings() const { return {c, address + 0xb0}; }
};

void report_failure(Cpu& c, std::uint32_t pc, std::uint32_t pool, std::uint32_t buffer,
                    unsigned message, unsigned detail, unsigned line) {
    const auto frame = c.sp();
    block(c, pc, 6);
    c.set({{3, pool}, {6, frame + message}, {7, frame + detail}, {4, 0}, {5, 0}});
    c.call(0x10739890, pc + 24);
    block(c, pc + 24, 1);
    c.link(pc + 28);
    c.set(3, c.toc(-0x5b20));
    c.s.charge(2);
    block(c, pc + 28, 7);
    c.set({{5, c.r(3)}, {3, buffer}, {4, pool + 0xf},
           {6, frame + message}, {7, frame + detail}, {8, line}});
    c.call(0x1077b720, pc + 56);
    block(c, pc + 56, 4);
    c.set({{4, buffer}, {3, pool + 0x1c}, {5, 0}});
    c.call(0x10769b60, pc + 72);
}

bool first_division_reserve(Cpu& c, std::uint32_t club, std::uint32_t division_slot) {
    block(c, 0x10578770, 4);
    c.set({{3, club}, {4, c.sp() + 0x38}, {5, 1}});
    c.call(0x100ce320, 0x10578780);
    block(c, 0x10578780, 2);
    if (!compare_logical(c, c.r(3))) return false;
    block(c, 0x10578788, 3);
    if (record_value(c, 0, c.i8(c.sp() + 0x38))) return false;
    block(c, 0x10578794, 3);
    c.set(3, c.u32(c.r(3) + 0x30));
    if (!compare_logical(c, c.r(3))) return false;
    block(c, 0x105787a0, 4);
    c.set(3, c.u32(c.r(3)));
    c.set(0, c.u32(division_slot));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0;
}

void collect_clubs(Cpu& c) {
    GuestNativeScope scope(c.s, collect_entry);
    block(c, collect_entry, 16);
    const auto stack = c.sp(), frame = stack - 0x90;
    c.set(0, c.lr());
    guest::transfer_saved_registers<24>(c, stack);
    const guest::Kept<24> object(c, c.r(3));
    const guest::Kept<29> total(c, c.toc(-0x7b50));
    const guest::Kept<30> first_division(c, c.toc(-0x67f0));
    guest::Kept<25> included(c, 0);
    const guest::Kept<31> second_division(c, c.toc(-0x67f4));
    c.set(27, c.toc(-0x7ba8));
    c.set(26, c.toc(-0x44a0));
    c.put32(stack + 8, c.r(0));
    c.set(0, 18);
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    const Division league{c, object};
    league.club_count() = 18;
    const auto count = std::int32_t(league.club_count());
    c.set({{0, std::uint32_t(count)}, {3, std::uint32_t(count) * standing_size}});
    c.call(0x10775130, 0x105786c0);
    block(c, 0x105786c0, 3);
    const auto allocated = c.r(3);
    const auto present = compare_logical(c, allocated) != 0;
    league.standings() = allocated;
    if (!present) {
        report_failure(c, 0x105786cc, c.r(26), c.r(27), 0x44, 0x3c, 0x16c);
        block(c, 0x10578714, 5);
        c.set(4, c.toc(-0x7bac));
        c.set({{0, 0}, {3, 0}});
        c.put8(c.r(4), 0);
    } else {
        block(c, 0x10578728, 5);
        c.set(3, c.toc(-0x7be8));
        guest::Kept<27> index(c, 0);
        guest::Kept<28> record_offset(c, 0);
        guest::Kept<26> club(c, c.u32(c.r(3)));
        for (;;) {
            block(c, 0x105787e0, 4);
            c.set(0, c.u32(total));
            const auto signed_index = std::int16_t(std::uint32_t(index));
            c.set(3, std::uint32_t(signed_index));
            if (compare(c, signed_index, std::int32_t(c.r(0))) >= 0) break;
            block(c, 0x1057873c, 3);
            c.set(3, c.u32(club + 0x30));
            if (compare_logical(c, c.r(3))) {
                block(c, 0x10578748, 4);
                c.set(3, c.u32(c.r(3)));
                c.set(0, c.u32(second_division));
                if (!compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0)))) {
                    block(c, 0x10578758, 6);
                    c.set({{3, object}, {4, included}, {5, club}, {6, 0}, {7, 0}});
                    c.call(0x104a60e0, 0x10578770);
                    if (first_division_reserve(c, club, first_division)) {
                        block(c, 0x105787b0, 4);
                        c.set(3, league.standings());
                        c.set({{0, record_offset + 0x40}, {4, std::uint32_t(-4)}});
                        c.put8(c.r(3) + c.r(0), c.r(4));
                    }
                    block(c, 0x105787c0, 6);
                    included = included + 1;
                    c.set(0, std::uint32_t(std::int16_t(league.club_count())));
                    c.set(3, std::uint32_t(std::int8_t(std::uint32_t(included))));
                    record_offset = record_offset + standing_size;
                    if (!compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0)))) break;
                }
            }
            block(c, 0x105787d8, 2);
            index = index + 1;
            club = club + club_stride;
        }
        block(c, 0x105787f0, 4);
        c.set(0, std::uint32_t(std::int16_t(league.club_count())));
        c.set(3, std::uint32_t(std::int8_t(std::uint32_t(included))));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0)))) {
            block(c, 0x10578800, 2);
            c.set(3, 0);
        } else {
            block(c, 0x10578808, 4);
            c.set({{4, 0xc350}, {3, object}});
            c.call(0x1049f540, 0x10578818);
            block(c, 0x10578818, 4);
            c.put32(object + 0x28, c.r(3));
            c.set({{0, 1}, {3, 1}});
            c.put8(object + 0xe1, 1);
        }
    }
    block(c, 0x10578828, 5);
    c.set(0, c.u32(frame + 0x98));
    c.set(1, stack);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<24, true>(c, stack);
    c.finish();
}

struct CalendarRound {
    std::uint32_t call_site;
    unsigned day, month, following_year, weekday;
    bool two_parts;
};
constexpr auto rounds = std::to_array<CalendarRound>({
    {0x105778e4, 22, 7, 0, 6, true},
    {0x10577950, 29, 7, 0, 6, true},
    {0x105779bc, 12, 8, 0, 6, true},
    {0x10577a28, 19, 8, 0, 6, true},
    {0x10577a94, 26, 8, 0, 6, true},
    {0x10577b00, 2, 9, 0, 5, false},
    {0x10577b4c, 17, 9, 0, 6, true},
    {0x10577bb8, 24, 9, 0, 6, true},
    {0x10577c24, 31, 9, 0, 6, true},
    {0x10577c90, 7, 10, 0, 6, true},
    {0x10577cfc, 21, 10, 0, 6, true},
    {0x10577d68, 28, 10, 0, 6, true},
    {0x10577dd4, 5, 11, 0, 6, true},
    {0x10577e40, 12, 11, 0, 6, true},
    {0x10577eac, 19, 11, 0, 6, true},
    {0x10577f18, 2, 0, 1, 6, true},
    {0x10577f84, 16, 0, 1, 6, true},
    {0x10577ff0, 23, 0, 1, 6, true},
    {0x1057805c, 30, 0, 1, 6, true},
    {0x105780c8, 6, 1, 1, 6, true},
    {0x10578134, 13, 1, 1, 6, true},
    {0x105781a0, 20, 1, 1, 6, true},
    {0x1057820c, 27, 1, 1, 6, true},
    {0x10578278, 4, 2, 1, 5, false},
    {0x105782c4, 12, 2, 1, 6, true},
    {0x10578330, 19, 2, 1, 6, true},
    {0x1057839c, 26, 2, 1, 6, true},
    {0x10578408, 2, 3, 1, 6, true},
    {0x10578474, 9, 3, 1, 6, true},
    {0x105784e0, 16, 3, 1, 6, true},
    {0x1057854c, 22, 3, 1, 5, false},
    {0x10578598, 30, 3, 1, 6, false},
    {0x105785e4, 7, 4, 1, 6, false},
    {0x10578630, 14, 4, 1, 6, false},
});

void configure_round_part(Cpu& c, const CalendarRound& round, unsigned index, unsigned part) {
    const auto start = round.call_site + 4 + part * 32;
    // Round 17's first configuration crosses an original generated-page edge.
    const bool page_edge = start == 0x10577ff4;
    block(c, start, page_edge ? 3 : 8);
    c.set({{3, c.r(31)}, {4, index}, {5, part}});
    if (page_edge) block(c, 0x10578000, 5);
    const bool split = round.two_parts && part == 0;
    c.set({{6, split ? 6u : UINT32_MAX}, {7, split ? 2u : UINT32_MAX},
           {8, split ? 1u : UINT32_MAX}, {9, 0}});
    c.call(0x104a9890, start + 32);
}

void make_calendar(Cpu& c) {
    GuestNativeScope scope(c.s, calendar_entry);
    block(c, calendar_entry, 13);
    const auto stack = c.sp(), frame = stack - 0x80;
    c.set(0, c.lr());
    c.put32(stack - 4, c.r(31));
    c.put32(stack - 8, c.r(30));
    const guest::Kept<30> object(c, c.r(3));
    c.put32(stack - 12, c.r(29));
    const guest::Kept<29> pool(c, c.toc(-0x44a0));
    c.put32(stack - 16, c.r(28));
    const guest::Kept<28> buffer(c, c.toc(-0x7ba8));
    c.put32(stack + 8, c.r(0));
    c.set(0, std::uint32_t(std::int8_t(c.r(4))));
    const auto supported = compare(c, std::int32_t(c.r(0)), -1) == 0;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    if (!supported) {
        block(c, 0x1057865c, 1);
        c.set(3, 0);
    } else {
        block(c, 0x10577834, 2);
        if (compare_logical(c, c.r(7))) {
            block(c, 0x1057783c, 2);
            c.set(0, 1);
            c.put8(c.r(7), 1);
        }
        block(c, 0x10577844, 6);
        c.set({{3, 34}, {0, 0}});
        c.put16(c.r(5), 34);
        c.set(3, 0xa18);
        c.put16(c.r(6), 0);
        c.call(0x10775130, 0x1057785c);
        block(c, 0x1057785c, 2);
        if (!record_value(c, 31, std::int32_t(c.r(3)))) {
            report_failure(c, 0x10577864, pool, buffer, 0x48, 0x40, 0xa8);
            block(c, 0x105778ac, 5);
            c.set(4, c.toc(-0x7bac));
            c.set({{0, 0}, {3, 0}});
            c.put8(c.r(4), 0);
        } else {
            for (unsigned index = 0; index < rounds.size(); ++index) {
                const auto& round = rounds[index];
                block(c, round.call_site - (index == 0 ? 36 : 40), index == 0 ? 10 : 11);
                c.set(0, 0);
                if (index == 0) c.set(4, 0);
                else c.set(3, c.r(31));
                c.put32(frame + 0x38, 0);
                c.set({{4, index}, {5, round.day}, {6, round.month}});
                if (index == 0) c.set(7, round.following_year);
                c.set(10, std::uint32_t(c.i16(object + 0x40)));
                c.set({{7, round.following_year}, {8, round.weekday}, {9, 1}});
                c.call(0x104a9810, round.call_site + 4);
                configure_round_part(c, round, index, 0);
                if (round.two_parts) configure_round_part(c, round, index, 1);
            }
            block(c, 0x10578654, 2);
            c.set(3, c.r(31));
        }
    }
    block(c, 0x10578660, 8);
    c.set(0, c.u32(frame + 0x88));
    c.set(1, stack);
    c.set_lr(c.r(0));
    for (unsigned reg = 31; reg >= 28; --reg) c.set(reg, c.u32(stack - (32 - reg) * 4));
    c.finish();
}

void configure_division(Cpu& c) {
    GuestNativeScope scope(c.s, configure_entry);
    block(c, configure_entry, 33);
    const auto stack = c.sp(), frame = stack - 0x40;
    c.set(0, c.lr());
    c.put32(stack - 4, c.r(31));
    c.set(7, 2);
    const guest::Kept<31> object(c, c.r(3));
    c.put32(stack + 8, c.r(0));
    c.set({{0, 34}, {6, 3}, {5, 1}});
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(4, 4);
    c.put16(object + 0x3c, 2);
    c.put16(object + 0xa6, 34);
    c.set(0, 0);
    c.set(3, c.toc(-0x67f0));
    for (const auto& [field, value] : std::array<std::pair<unsigned, unsigned>, 12>{{
            {0xc4, 3}, {0xc5, 1}, {0xc6, 2}, {0x42, 1}, {0xc7, 4}, {0xc8, 1},
            {0xc9, 2}, {0xca, 3}, {0xc0, 3}, {0xc1, 0}, {0xc2, 0}, {0xc3, 3}}})
        c.put8(object + field, value);
    c.set(0, c.u32(c.r(3)));
    c.put32(object + 0x1c, c.r(0));
    c.set(3, c.u32(object + 4));
    c.set(3, c.u32(c.r(3) + 8));
    c.set(0, c.u8(c.r(3) + 0x120) & 4);
    if (record_value(c, 0, std::int32_t(c.r(0)))) {
        block(c, 0x105788c4, 4);
        c.set(3, c.toc(-0x6964));
        c.set(0, c.u32(c.r(3)));
        c.put32(object + 0x20, c.r(0));
    } else {
        block(c, 0x105788d4, 2);
        c.set(0, UINT32_MAX);
        c.put32(object + 0x20, c.r(0));
    }
    block(c, 0x105788dc, 14);
    c.set({{0, 2}, {4, 7}});
    c.put8(object + 0x4c, 2);
    c.set({{0, 3}, {3, object}, {5, object + 0xa6}});
    c.put8(object + 0x46, 7);
    c.set({{6, object + 0x3a}, {4, UINT32_MAX}, {7, 0}});
    c.put8(object + 0x47, 3);
    c.set(12, c.u32(object));
    c.set(12, c.u32(c.r(12) + 0x44));
    call_virtual(c, 0x10578914);
    block(c, 0x10578914, 8);
    c.set(2, c.u32(frame + 0x14));
    c.put32(object + 0xbc, c.r(3));
    c.set(3, 1);
    c.set(0, c.u32(frame + 0x48));
    c.set(1, stack);
    c.set_lr(c.r(0));
    c.set(31, c.u32(stack - 4));
    c.finish();
}

void collect_direct(Cpu& c, std::uint32_t resume) {
    if (routine_checker) c.call(collect_entry, resume);
    else {
        c.link(resume);
        c.s.pc = collect_entry;
        collect_clubs(c);
    }
}

void create_division(Cpu& c) {
    GuestNativeScope scope(c.s, create_entry);
    block(c, create_entry, 12);
    const auto stack = c.sp(), frame = stack - 0x120;
    c.set(0, c.lr());
    guest::transfer_saved_registers<27>(c, stack);
    const guest::Kept<29> failure_flag(c, c.toc(-0x7bac));
    const guest::Kept<27> initial_year(c, c.r(4));
    const guest::Kept<30> buffer(c, c.toc(-0x7ba8));
    const guest::Kept<28> definition(c, c.r(5));
    const guest::Kept<31> pool(c, c.toc(-0x44a0));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    // The original keeps this argument in the caller's parameter area. Reload
    // it after callbacks, which can observe or replace that slot.
    c.put32(frame + 0x138, c.r(3));
    const auto object = [&] { return c.u32(frame + 0x138); };
    const auto finish = [&] {
        block(c, 0x105776d4, 5);
        c.set(0, c.u32(frame + 0x128));
        c.set(1, stack);
        c.set_lr(c.r(0));
        guest::transfer_saved_registers<27, true>(c, stack);
        c.finish();
    };
    const auto fail = [&](std::uint32_t pc, unsigned message, unsigned detail, unsigned line) {
        report_failure(c, pc, pool, buffer, message, detail, line);
        block(c, pc + 72, 4);
        c.set(0, 0);
        c.put8(failure_flag, 0);
        c.set(3, object());
        finish();
    };
    c.set(3, object());
    c.call(0x1049e4f0, 0x105773f0);
    block(c, 0x105773f0, 15);
    c.set({{5, c.toc(-0x5fb8)}, {0, 0x17}, {3, object()}, {4, 1}});
    c.put32(c.r(3), c.r(5));
    c.set(3, object());
    c.put16(c.r(3) + 0x40, initial_year);
    c.set(3, object());
    c.put32(c.r(3) + 4, definition);
    c.set(3, object());
    c.put8(c.r(3) + 0x4a, c.r(0));
    c.set(3, object());
    c.set(12, c.u32(c.r(3)));
    c.set(12, c.u32(c.r(12) + 0x20));
    call_virtual(c, 0x1057742c);
    block(c, 0x1057742c, 3);
    c.set(2, c.u32(frame + 0x14));
    if (compare(c, std::int32_t(c.r(3)))) {
        block(c, 0x10577438, 2);
        c.set(3, object());
        finish();
        return;
    }
    block(c, 0x10577440, 12);
    c.set({{3, object()}, {4, UINT32_MAX}, {0, 0}});
    c.put8(c.r(3) + 0x44, c.r(4));
    c.set(3, object());
    c.put32(c.r(3) + 0x30, c.r(4));
    c.set(3, object());
    c.put32(c.r(3) + 0x2c, c.r(0));
    c.set(4, object());
    c.set(3, c.u32(c.r(4) + 0x2c));
    if (compare(c, std::int32_t(c.r(3))) > 0) {
        block(c, 0x10577470, 2);
        c.set(3, c.r(3) * 4);
        c.call(0x10775130, 0x10577478);
        block(c, 0x10577478, 3);
        c.set(4, object());
        c.put32(c.r(4) + 0xc, c.r(3));
    } else {
        block(c, 0x10577484, 1);
        c.put32(c.r(4) + 0xc, c.r(0));
    }
    block(c, 0x10577488, 4);
    c.set(3, object());
    c.set(12, c.u32(c.r(3)));
    c.set(12, c.u32(c.r(12) + 0x94));
    call_virtual(c, 0x10577498);
    block(c, 0x10577498, 3);
    c.set(2, c.u32(frame + 0x14));
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x105774a4, 0xe0, 0x58, 0x43); return; }
    block(c, 0x105774fc, 2);
    c.set(3, object());
    collect_direct(c, 0x10577504);
    block(c, 0x10577504, 2);
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x1057750c, 0xc0, 0x50, 0x4a); return; }
    block(c, 0x10577564, 2);
    c.set(3, object());
    c.call(0x104a00a0, 0x1057756c);
    block(c, 0x1057756c, 2);
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x10577574, 0xa0, 0x48, 0x51); return; }
    block(c, 0x105775cc, 3);
    c.set({{3, object()}, {4, 0}});
    c.call(0x1049eb30, 0x105775d8);
    block(c, 0x105775d8, 2);
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x105775e0, 0x80, 0x40, 0x5a); return; }
    block(c, 0x10577638, 2);
    c.set(3, 0x634);
    c.call(0x10773520, 0x10577640);
    block(c, 0x10577640, 2);
    if (record_value(c, 27, std::int32_t(c.r(3)))) {
        block(c, 0x10577648, 2);
        c.set(4, object());
        c.call(0x104533a0, 0x10577650);
    }
    block(c, 0x10577650, 6);
    const guest::Kept<27> stage(c, c.r(27));
    c.set(3, object());
    c.put32(c.r(3) + 8, stage);
    c.set(3, object());
    c.set(0, c.u32(c.r(3) + 8));
    if (!compare_logical(c, c.r(0))) { fail(0x10577668, 0x60, 0x38, 0x61); return; }
    block(c, 0x105776c0, 3);
    c.set(12, c.u32(c.r(3)));
    c.set(12, c.u32(c.r(12) + 0x64));
    call_virtual(c, 0x105776cc);
    block(c, 0x105776cc, 2);
    c.set(2, c.u32(frame + 0x14));
    c.set(3, object());
    finish();
}

void destroy_division(Cpu& c) {
    GuestNativeScope scope(c.s, destroy_entry);
    block(c, destroy_entry, 10);
    const auto stack = c.sp(), frame = stack - 0x50;
    c.set(0, c.lr());
    for (unsigned reg = 31; reg >= 29; --reg) c.put32(stack - (32 - reg) * 4, c.r(reg));
    const guest::Kept<29> delete_object(c, c.r(4));
    c.put32(stack - 16, c.r(28));
    const auto present = record_value(c, 28, std::int32_t(c.r(3)));
    const guest::Kept<28> object(c, c.r(28));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    if (present) {
        block(c, 0x10577718, 4);
        c.set({{0, c.toc(-0x5fb8)}, {4, 0}});
        c.put32(object, c.r(0));
        c.call(0x104a6660, 0x10577728);
        block(c, 0x10577728, 3);
        c.set(3, c.u32(object + 0xbc));
        if (compare_logical(c, c.r(3))) {
            block(c, 0x10577734, 1);
            c.call(0x10775170, 0x10577738);
            block(c, 0x10577738, 2);
            c.set(0, 0);
            c.put32(object + 0xbc, 0);
        }
        block(c, 0x10577740, 3);
        guest::Kept<30> index(c, 0);
        guest::Kept<31> offset(c, 0);
        for (;;) {
            block(c, 0x1057777c, 3);
            c.set(0, c.u32(object + 0x30));
            if (compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(c.r(0))) > 0) break;
            block(c, 0x10577750, 4);
            c.set(3, c.u32(object + 0xc));
            c.set(3, c.u32(c.r(3) + offset));
            if (compare_logical(c, c.r(3))) {
                block(c, 0x10577760, 4);
                c.set(12, c.u32(c.r(3)));
                c.set(4, 1);
                c.set(12, c.u32(c.r(12) + 8));
                call_virtual(c, 0x10577770);
                block(c, 0x10577770, 1);
                c.set(2, c.u32(frame + 0x14));
            }
            block(c, 0x10577774, 2);
            offset = offset + 4;
            index = index + 1;
        }
        block(c, 0x10577788, 3);
        c.set(3, c.u32(object + 0xc));
        if (compare_logical(c, c.r(3))) {
            block(c, 0x10577794, 1);
            c.call(0x10775170, 0x10577798);
            block(c, 0x10577798, 2);
            c.set(0, 0);
            c.put32(object + 0xc, 0);
        }
        block(c, 0x105777a0, 3);
        c.set(3, c.u32(object + 8));
        if (compare_logical(c, c.r(3))) {
            block(c, 0x105777ac, 2);
            c.set(4, 1);
            c.call(0x104536c0, 0x105777b4);
        }
        block(c, 0x105777b4, 3);
        c.set({{3, object}, {4, 0}});
        c.call(0x1049e600, 0x105777c0);
        block(c, 0x105777c0, 2);
        if (record_value(c, 0, std::int16_t(std::uint32_t(delete_object))) > 0) {
            block(c, 0x105777c8, 2);
            c.set(3, object);
            c.call(0x107735a0, 0x105777d0);
        }
    }
    block(c, 0x105777d0, 9);
    c.set(3, object);
    c.set(0, c.u32(frame + 0x58));
    c.set(1, stack);
    c.set_lr(c.r(0));
    for (unsigned reg = 31; reg >= 28; --reg) c.set(reg, c.u32(stack - (32 - reg) * 4));
    c.finish();
}

void next_season(Cpu& c) {
    GuestNativeScope scope(c.s, season_entry);
    block(c, season_entry, 12);
    const auto stack = c.sp(), frame = stack - 0xf0;
    c.set(0, c.lr());
    guest::transfer_saved_registers<26>(c, stack);
    const guest::Kept<31> failure_flag(c, c.toc(-0x7bac));
    const guest::Kept<26> object(c, c.r(3));
    const guest::Kept<29> buffer(c, c.toc(-0x7ba8));
    c.set(4, 0);
    const guest::Kept<30> pool(c, c.toc(-0x44a0));
    c.put32(stack + 8, c.r(0));
    c.set(0, 0);
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    const auto finish = [&] {
        block(c, 0x10578bc4, 5);
        c.set(0, c.u32(frame + 0xf8));
        c.set(1, stack);
        c.set_lr(c.r(0));
        guest::transfer_saved_registers<26, true>(c, stack);
        c.finish();
    };
    const auto fail = [&](std::uint32_t pc, unsigned message, unsigned detail, unsigned line) {
        report_failure(c, pc, pool, buffer, message, detail, line);
        block(c, pc + 72, 3);
        c.set(0, 0);
        c.put8(failure_flag, 0);
        finish();
    };
    c.put8(object + 0x49, 0);
    c.call(0x104a6660, 0x10578970);
    block(c, 0x10578970, 3);
    c.set(3, c.u32(object + 0xbc));
    if (compare_logical(c, c.r(3))) {
        block(c, 0x1057897c, 1);
        c.call(0x10775170, 0x10578980);
        block(c, 0x10578980, 2);
        c.set(0, 0);
        c.put32(object + 0xbc, 0);
    }
    block(c, 0x10578988, 3);
    c.set(3, c.u32(object + 8));
    if (compare_logical(c, c.r(3))) {
        block(c, 0x10578994, 2);
        c.set(4, 1);
        c.call(0x104564a0, 0x1057899c);
    }
    block(c, 0x1057899c, 3);
    guest::Kept<27> index(c, 0);
    guest::Kept<28> offset(c, 0);
    for (;;) {
        block(c, 0x105789d8, 3);
        c.set(0, c.u32(object + 0x30));
        if (compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(c.r(0))) > 0) break;
        block(c, 0x105789ac, 4);
        c.set(3, c.u32(object + 0xc));
        c.set(3, c.u32(c.r(3) + offset));
        if (compare_logical(c, c.r(3))) {
            block(c, 0x105789bc, 4);
            c.set(12, c.u32(c.r(3)));
            c.set(4, 1);
            c.set(12, c.u32(c.r(12) + 8));
            call_virtual(c, 0x105789cc);
            block(c, 0x105789cc, 1);
            c.set(2, c.u32(frame + 0x14));
        }
        block(c, 0x105789d0, 2);
        offset = offset + 4;
        index = index + 1;
    }
    block(c, 0x105789e4, 9);
    c.set({{0, UINT32_MAX}, {3, object}});
    c.put32(object + 0x30, c.r(0));
    c.set(4, std::uint32_t(c.i16(object + 0x40)));
    c.set(0, c.r(4) + 1);
    c.put16(object + 0x40, c.r(0));
    c.set(12, c.u32(object));
    c.set(12, c.u32(c.r(12) + 0x94));
    call_virtual(c, 0x10578a08);
    block(c, 0x10578a08, 3);
    c.set(2, c.u32(frame + 0x14));
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x10578a14, 0xb8, 0x50, 0x1e4); return; }
    block(c, 0x10578a68, 2);
    c.set(3, object);
    collect_direct(c, 0x10578a70);
    block(c, 0x10578a70, 2);
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x10578a78, 0x98, 0x48, 0x1eb); return; }
    block(c, 0x10578acc, 2);
    c.set(3, object);
    c.call(0x104a00a0, 0x10578ad4);
    block(c, 0x10578ad4, 2);
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x10578adc, 0x78, 0x40, 0x1f1); return; }
    block(c, 0x10578b30, 3);
    c.set({{3, object}, {4, 0}});
    c.call(0x1049eb30, 0x10578b3c);
    block(c, 0x10578b3c, 2);
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x10578b44, 0x58, 0x38, 0x1f8); return; }
    block(c, 0x10578b98, 4);
    c.set(3, object);
    c.set(12, c.u32(object));
    c.set(12, c.u32(c.r(12) + 0x64));
    call_virtual(c, 0x10578ba8);
    block(c, 0x10578ba8, 3);
    c.set(2, c.u32(frame + 0x14));
    c.set(3, object);
    c.call(0x104aa3a0, 0x10578bb4);
    block(c, 0x10578bb4, 4);
    c.set(3, c.toc(-0x7428));
    c.set(4, c.u32(object + 4));
    c.set(3, c.u32(c.r(3)));
    c.call(0x102d46c0, 0x10578bc4);
    finish();
}

struct Routine { std::uint32_t entry; void (*run)(Cpu&); };
constexpr auto routines = std::to_array<Routine>({
#define POR_SECOND_ROUTINE(entry, function) Routine{entry, function},
#include "por_second_routines.inc"
#undef POR_SECOND_ROUTINE
});

const Routine& find_routine(std::uint32_t entry) {
    const auto found = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    if (found == routines.end() || found->entry != entry)
        throw std::logic_error("Unknown POR_SECOND.CPP entry " + hex_address(entry));
    return *found;
}

void call_virtual(Cpu& c, std::uint32_t resume) {
    if (routine_checker) {
        c.call(0x10781320, resume);
        return;
    }
    c.link(resume);
    c.s.pc = 0x10781320;
    // The shared CFM owner preserves descriptor aliases, TOC, CTR, observers
    // and partial-budget behavior before selecting the actual virtual target.
    if (try_modern_import_descriptor(c.s, 0x10781320)) {
        const auto target = std::ranges::lower_bound(routines, c.s.pc, {}, &Routine::entry);
        if (target != routines.end() && target->entry == c.s.pc) target->run(c);
    }
    if (c.s.pc != resume) run_until(c.s, resume);
}
}

void por_second_run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, declining); }
void por_second_run_rewrite(State& s, std::uint32_t entry) { Cpu c(s); find_routine(entry).run(c); }

bool try_modern_por_second(State& s, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(s, entry, por_second_run_original, por_second_run_rewrite, RoutineChecker::result_r3);
    por_second_run_rewrite(s, entry);
    return true;
}
}
