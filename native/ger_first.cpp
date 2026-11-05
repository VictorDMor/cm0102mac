#include "ger_first_adapter.hpp"
#include "guest_values.hpp"
#include "guest_fpu.hpp"
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

bool declining = std::getenv("CM_GER_FIRST_ORIGINAL") != nullptr;
constexpr auto collect_entry = 0x10539590u;
constexpr auto create_entry = 0x10536d50u, destroy_entry = 0x10537080u;
constexpr auto calendar_entry = 0x10537120u, configure_entry = 0x105397b0u;
constexpr auto link_entry = 0x10539bb0u, promote_entry = 0x10539d20u;
constexpr auto fill_entry = 0x10539f30u, reserves_entry = 0x1053a900u;
constexpr auto split_entry = 0x1053a200u;
constexpr auto next_season_entry = 0x10539880u;
constexpr unsigned club_stride = 0x248, standing_size = 0x44, definition_size = 0x6c;
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
    guest::Field<std::int16_t> year() const { return {c, address + 0x40}; }
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
    c.set({{5, c.r(3)}, {3, buffer}, {4, pool + 0xe},
           {6, frame + message}, {7, frame + detail}, {8, line}});
    c.call(0x1077b720, pc + 56);
    block(c, pc + 56, 4);
    c.set({{4, buffer}, {3, pool + 0x1b}, {5, 0}});
    c.call(0x10769b60, pc + 72);
}

// The inaugural 2001 pass remembers four regional definitions before moving
// their clubs to the shared division. It excludes the final two youth blocks.
void reassign_2001_clubs(Cpu& c, std::uint32_t clubs_slot, std::uint32_t total_slot) {
    block(c, 0x105395ec, 3);
    auto club = c.u32(clubs_slot);
    std::uint32_t index = 0;
    c.set({{11, club}, {10, index}});
    for (;;) {
        block(c, 0x10539658, 6);
        c.set(12, c.u32(c.r(9)));
        c.set(0, c.u32(total_slot));
        c.set(12, c.r(12) << 1);
        c.set(0, c.r(0) - c.r(12));
        if (compare(c, std::int32_t(index), std::int32_t(c.r(0))) >= 0) break;
        block(c, 0x105395f8, 3);
        c.set(12, c.u32(club + 0x30));
        if (compare_logical(c, c.r(12))) {
            block(c, 0x10539604, 4);
            c.set(24, c.u32(c.r(12)));
            c.set(0, c.u32(c.r(3)));
            bool selected = compare(c, std::int32_t(c.r(24)), std::int32_t(c.r(0))) == 0;
            for (unsigned group = 0; !selected && group < 3; ++group) {
                block(c, 0x10539614 + group * 12, 3);
                c.set(0, c.u32(c.r(4 + group)));
                selected = compare(c, std::int32_t(c.r(24)), std::int32_t(c.r(0))) == 0;
            }
            if (selected) {
                block(c, 0x10539638, 6);
                c.put32(club + 0x38, c.r(12));
                c.set(0, c.u32(c.r(8)));
                c.set(12, c.u32(c.r(7)));
                c.set(0, c.r(0) * definition_size);
                c.set(0, c.r(12) + c.r(0));
                c.put32(club + 0x30, c.r(0));
            }
        }
        block(c, 0x10539650, 2);
        ++index;
        club += club_stride;
        c.set({{10, index}, {11, club}});
    }
}

void collect_clubs(Cpu& c) {
    GuestNativeScope scope(c.s, collect_entry);
    block(c, collect_entry, 23);
    const auto stack = c.sp(), frame = stack - 0x80;
    c.set(0, c.lr());
    guest::transfer_saved_registers<24>(c, stack);
    const guest::Kept<25> object(c, c.r(3));
    const guest::Kept<27> division_id(c, c.toc(-0x6738));
    c.set(9, c.toc(-0x7b04));
    guest::Kept<26> included(c, 0);
    const guest::Kept<28> total_slot(c, c.toc(-0x7b50));
    c.set(8, c.toc(-0x6750));
    c.set(7, c.toc(-0x7bbc));
    c.set(6, c.toc(-0x674c));
    c.set(5, c.toc(-0x6748));
    c.set(4, c.toc(-0x6744));
    guest::Kept<29> clubs_slot(c, c.toc(-0x7be8));
    c.put32(stack + 8, c.r(0));
    c.set(0, 18);
    const guest::Kept<30> buffer(c, c.toc(-0x7ba8));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    const guest::Kept<31> pool(c, c.toc(-0x44f8));
    const Division league{c, object};
    league.club_count() = 18;
    c.set(0, std::uint32_t(std::int16_t(league.year())));
    c.set(3, c.toc(-0x6740));
    if (!compare(c, std::int32_t(c.r(0)), 2001)) reassign_2001_clubs(c, clubs_slot, total_slot);
    block(c, 0x10539670, 3);
    c.set(0, std::uint32_t(std::int16_t(league.club_count())));
    c.set(3, c.r(0) * standing_size);
    c.call(0x10775130, 0x1053967c);
    block(c, 0x1053967c, 3);
    const auto allocated = c.r(3);
    const auto present = compare_logical(c, allocated) != 0;
    league.standings() = allocated;
    if (!present) {
        report_failure(c, 0x10539688, pool, buffer, 0x40, 0x38, 0x26e);
        block(c, 0x105396d0, 5);
        c.set(4, c.toc(-0x7bac));
        c.set({{0, 0}, {3, 0}});
        c.put8(c.r(4), 0);
    } else {
        block(c, 0x105396e4, 3);
        guest::Kept<29> club(c, c.u32(clubs_slot));
        guest::Kept<24> index(c, 0);
        for (;;) {
            block(c, 0x10539744, 3);
            c.set(0, c.u32(total_slot));
            if (compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(c.r(0))) >= 0) break;
            block(c, 0x105396f4, 3);
            c.set(3, c.u32(club + 0x30));
            if (compare_logical(c, c.r(3))) {
                block(c, 0x10539700, 4);
                c.set(3, c.u32(c.r(3)));
                c.set(0, c.u32(division_id));
                if (!compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0)))) {
                    block(c, 0x10539710, 7);
                    c.set({{4, included}, {3, object}, {5, club}, {6, 0}, {7, 0}});
                    included = included + 1;
                    c.call(0x104a60e0, 0x1053972c);
                    block(c, 0x1053972c, 4);
                    c.set(0, std::uint32_t(std::int16_t(league.club_count())));
                    c.set(3, std::uint32_t(std::int8_t(std::uint32_t(included))));
                    if (!compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0)))) break;
                }
            }
            block(c, 0x1053973c, 2);
            index = index + 1;
            club = club + club_stride;
        }
        block(c, 0x10539750, 4);
        c.set(0, std::uint32_t(std::int16_t(league.club_count())));
        c.set(3, std::uint32_t(std::int8_t(std::uint32_t(included))));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0)))) {
            block(c, 0x10539760, 2);
            c.set(3, 0);
        } else {
            block(c, 0x10539768, 4);
            c.set({{4, 0x30000}, {3, object}});
            c.set(4, 200000);
            c.call(0x1049f540, 0x10539778);
            block(c, 0x10539778, 8);
            c.put32(object + 0x28, c.r(3));
            c.set({{0, 1}, {4, 0x4c0000}, {3, object}});
            c.put8(object + 0xe1, 1);
            c.set({{4, 5000000}, {5, 0}});
            c.call(0x1049f6d0, 0x10539798);
            block(c, 0x10539798, 1);
            c.set(3, 1);
        }
    }
    block(c, 0x1053979c, 5);
    c.set(0, c.u32(frame + 0x88));
    c.set(1, stack);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<24, true>(c, stack);
    c.finish();
}

struct CalendarRound {
    std::uint32_t call_site;
    unsigned day, month, following_year, weekday, duration, parts;
};
constexpr auto calendar_rounds = std::to_array<CalendarRound>({
    {0x10537200, 29, 6, 0, 5, 1, 3},
    {0x1053728c, 5, 7, 0, 5, 1, 3},
    {0x10537318, 12, 7, 0, 5, 1, 3},
    {0x105373a4, 19, 7, 0, 5, 1, 3},
    {0x10537430, 6, 8, 0, 2, 2, 1},
    {0x1053747c, 9, 8, 0, 5, 1, 3},
    {0x10537508, 16, 8, 0, 5, 1, 3},
    {0x10537594, 23, 8, 0, 5, 1, 3},
    {0x10537620, 30, 8, 0, 5, 1, 3},
    {0x105376ac, 14, 9, 0, 5, 1, 3},
    {0x10537738, 21, 9, 0, 5, 1, 3},
    {0x105377c4, 28, 9, 0, 5, 1, 3},
    {0x10537850, 4, 10, 0, 5, 1, 3},
    {0x105378dc, 11, 10, 0, 5, 1, 3},
    {0x10537968, 18, 10, 0, 5, 1, 3},
    {0x105379f4, 25, 10, 0, 5, 1, 3},
    {0x10537a80, 2, 11, 0, 5, 1, 3},
    {0x10537b0c, 9, 11, 0, 5, 1, 3},
    {0x10537b98, 13, 11, 0, 2, 2, 1},
    {0x10537be4, 16, 11, 0, 5, 1, 3},
    {0x10537c70, 27, 0, 1, 5, 1, 3},
    {0x10537cfc, 3, 1, 1, 5, 1, 3},
    {0x10537d88, 10, 1, 1, 5, 1, 3},
    {0x10537e14, 17, 1, 1, 5, 1, 3},
    {0x10537ea0, 24, 1, 1, 5, 1, 3},
    {0x10537f2c, 3, 2, 1, 5, 1, 3},
    {0x10537fb8, 10, 2, 1, 5, 1, 3},
    {0x10538044, 17, 2, 1, 5, 1, 3},
    {0x105380d0, 31, 2, 1, 5, 1, 3},
    {0x1053815c, 7, 3, 1, 5, 1, 3},
    {0x105381e8, 14, 3, 1, 5, 1, 3},
    {0x10538274, 21, 3, 1, 5, 1, 3},
    {0x10538300, 28, 3, 1, 5, 1, 1},
    {0x1053834c, 5, 4, 1, 5, 1, 1},
    {0x10538398, 12, 7, 0, 5, 1, 3},
    {0x10538424, 19, 7, 0, 5, 1, 3},
    {0x105384b0, 6, 8, 0, 2, 2, 1},
    {0x105384fc, 9, 8, 0, 5, 1, 3},
    {0x10538588, 16, 8, 0, 5, 1, 3},
    {0x10538614, 23, 8, 0, 5, 1, 3},
    {0x105386a0, 30, 8, 0, 5, 1, 3},
    {0x1053872c, 14, 9, 0, 5, 1, 3},
    {0x105387b8, 21, 9, 0, 5, 1, 3},
    {0x10538844, 28, 9, 0, 5, 1, 3},
    {0x105388d0, 4, 10, 0, 5, 1, 3},
    {0x1053895c, 11, 10, 0, 5, 1, 3},
    {0x105389e8, 18, 10, 0, 5, 1, 3},
    {0x10538a74, 25, 10, 0, 5, 1, 3},
    {0x10538b00, 2, 11, 0, 5, 1, 3},
    {0x10538b8c, 9, 11, 0, 5, 1, 3},
    {0x10538c18, 13, 11, 0, 2, 2, 1},
    {0x10538c64, 16, 11, 0, 5, 1, 3},
    {0x10538cf0, 27, 0, 1, 5, 1, 3},
    {0x10538d7c, 3, 1, 1, 5, 1, 3},
    {0x10538e08, 10, 1, 1, 5, 1, 3},
    {0x10538e94, 17, 1, 1, 5, 1, 3},
    {0x10538f20, 24, 1, 1, 5, 1, 3},
    {0x10538fac, 3, 2, 1, 5, 1, 3},
    {0x10539038, 10, 2, 1, 5, 1, 3},
    {0x105390c4, 17, 2, 1, 5, 1, 3},
    {0x10539150, 31, 2, 1, 5, 1, 3},
    {0x105391dc, 7, 3, 1, 5, 1, 3},
    {0x10539268, 14, 3, 1, 5, 1, 3},
    {0x105392f4, 21, 3, 1, 5, 1, 3},
    {0x10539380, 28, 3, 1, 5, 1, 3},
    {0x1053940c, 5, 4, 1, 5, 1, 3},
    {0x10539498, 12, 4, 1, 5, 1, 1},
    {0x105394e4, 19, 4, 1, 5, 1, 1},
});

void configure_round_part(Cpu& c, const CalendarRound& round, unsigned index, unsigned part) {
    const auto start = round.call_site + 4 + part * 32;
    // Two part configurations cross original generated-page boundaries.
    const auto first_count = start == 0x10537ffc ? 1u : start == 0x10538ff0 ? 4u : 8u;
    block(c, start, first_count);
    c.set(3, c.r(28));
    if (first_count == 1) block(c, 0x10538000, 7);
    const bool split = part + 1 < round.parts;
    c.set({{4, index}, {5, part}, {6, split ? 6u : UINT32_MAX}});
    if (first_count == 4) block(c, 0x10539000, 4);
    c.set({{7, split ? 1u : UINT32_MAX}, {8, split ? 1u : UINT32_MAX}, {9, 0}});
    c.call(0x104a9890, start + 32);
}

void make_calendar(Cpu& c) {
    GuestNativeScope scope(c.s, calendar_entry);
    block(c, calendar_entry, 11);
    const auto stack = c.sp(), frame = stack - 0xb0;
    c.set(0, c.lr());
    guest::transfer_saved_registers<27>(c, stack);
    const guest::Kept<29> failure_flag(c, c.toc(-0x7bac));
    const guest::Kept<27> object(c, c.r(3));
    const guest::Kept<30> buffer(c, c.toc(-0x7ba8));
    const guest::Kept<31> pool(c, c.toc(-0x44f8));
    c.put32(stack + 8, c.r(0));
    c.set(0, std::uint32_t(std::int8_t(c.r(4))));
    const auto supported = compare(c, std::int32_t(c.r(0)), -1) == 0;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    if (!supported) {
        block(c, 0x10539570, 1);
        c.set(3, 0);
    } else {
        block(c, 0x1053714c, 2);
        if (compare_logical(c, c.r(7))) {
            block(c, 0x10537154, 2);
            c.set(0, 1);
            c.put8(c.r(7), 1);
        }
        block(c, 0x1053715c, 6);
        c.set({{3, 34}, {0, 0}});
        c.put16(c.r(5), 34);
        c.set(3, 0xa18);
        c.put16(c.r(6), 0);
        c.call(0x10775130, 0x10537174);
        block(c, 0x10537174, 2);
        if (!record_value(c, 28, std::int32_t(c.r(3)))) {
            report_failure(c, 0x1053717c, pool, buffer, 0x70, 0x48, 0xa1);
            block(c, 0x105371c4, 4);
            c.set({{0, 0}, {3, 0}});
            c.put8(failure_flag, 0);
        } else {
            block(c, 0x105371d4, 3);
            c.set(10, std::uint32_t(c.i16(object + 0x40)));
            const bool inaugural = compare(c, std::int32_t(c.r(10)), 2001) == 0;
            const auto first = inaugural ? 0u : 34u;
            for (unsigned index = 0; index < 34; ++index) {
                const auto& round = calendar_rounds[first + index];
                block(c, round.call_site - (index == 0 ? 32 : 40), index == 0 ? 9 : 11);
                c.set(0, 0);
                if (index == 0) c.set(4, 0);
                else c.set(3, c.r(28));
                c.put32(frame + 0x38, 0);
                c.set({{4, index}, {5, round.day}, {6, round.month}});
                if (index != 0) c.set(10, std::uint32_t(c.i16(object + 0x40)));
                c.set({{7, round.following_year}, {8, round.weekday}, {9, round.duration}});
                c.call(0x104a9810, round.call_site + 4);
                for (unsigned part = 0; part < round.parts; ++part) configure_round_part(c, round, index, part);
            }
            if (inaugural) {
                block(c, 0x10538370, 2);
                c.set(0, 34);
                block(c, 0x1053950c, 3);
            } else {
                block(c, 0x10539508, 4);
                c.set(0, 34);
            }
            // The original immediately sign-extends and compares literal 34.
            // No memory/call/observer boundary can reach its diagnostic arm.
            compare(c, std::int32_t(c.r(0)), 34);
            block(c, 0x10539568, 2);
            c.set(3, c.r(28));
        }
    }
    block(c, 0x10539574, 5);
    c.set(0, c.u32(frame + 0xb8));
    c.set(1, stack);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<27, true>(c, stack);
    c.finish();
}

void configure_division(Cpu& c) {
    GuestNativeScope scope(c.s, configure_entry);
    block(c, configure_entry, 40);
    const auto stack = c.sp(), frame = stack - 0x40;
    c.set(0, c.lr());
    c.put32(stack - 4, c.r(31));
    c.set({{31, 2}, {12, 3}});
    c.put32(stack - 8, c.r(30));
    const guest::Kept<30> object(c, c.r(3));
    c.set({{11, 1}, {10, 0}});
    c.put32(stack + 8, c.r(0));
    c.set({{9, UINT32_MAX}, {8, c.toc(-0x673c)}, {0, 6}});
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set({{5, object + 0xa6}, {6, object + 0x3a}, {4, UINT32_MAX}});
    c.put16(object + 0x3c, 2);
    c.set(7, 0);
    c.put8(object + 0xc4, 3);
    c.put8(object + 0xc5, 1);
    c.put8(object + 0xc6, 2);
    c.put16(object + 0xa8, 0);
    c.put16(object + 0xde, 0);
    for (const auto& [field, value] : std::array<std::pair<unsigned, unsigned>, 8>{{
            {0x42, 1}, {0xc7, 1}, {0xc8, 2}, {0xc9, 3}, {0xc0, 0},
            {0xc1, 0}, {0xc2, 0}, {0xc3, 3}}})
        c.put8(object + field, value);
    c.put32(object + 0x1c, UINT32_MAX);
    c.set(8, c.u32(c.r(8)));
    c.put32(object + 0x20, c.r(8));
    c.put8(object + 0x46, 6);
    c.put8(object + 0x47, 3);
    c.put8(object + 0x4c, 2);
    c.set(12, c.u32(object));
    c.set(12, c.u32(c.r(12) + 0x44));
    call_virtual(c, 0x10539850);
    block(c, 0x10539850, 9);
    c.set(2, c.u32(frame + 0x14));
    c.put32(object + 0xbc, c.r(3));
    c.set(3, 1);
    c.set(0, c.u32(frame + 0x48));
    c.set(1, stack);
    c.set_lr(c.r(0));
    c.set(31, c.u32(stack - 4));
    c.set(30, c.u32(stack - 8));
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
    const guest::Kept<31> pool(c, c.toc(-0x44f8));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    // The original keeps this argument in the caller's parameter area. Reload
    // it after callbacks, which can observe or replace that slot.
    c.put32(frame + 0x138, c.r(3));
    const auto object = [&] { return c.u32(frame + 0x138); };
    const auto finish = [&] {
        block(c, 0x10537064, 5);
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
    c.call(0x1049e4f0, 0x10536d80);
    block(c, 0x10536d80, 15);
    c.set({{5, c.toc(-0x6000)}, {0, 0xd}, {3, object()}, {4, 1}});
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
    call_virtual(c, 0x10536dbc);
    block(c, 0x10536dbc, 3);
    c.set(2, c.u32(frame + 0x14));
    if (compare(c, std::int32_t(c.r(3)))) {
        block(c, 0x10536dc8, 2);
        c.set(3, object());
        finish();
        return;
    }
    block(c, 0x10536dd0, 12);
    c.set({{3, object()}, {4, UINT32_MAX}, {0, 0}});
    c.put8(c.r(3) + 0x44, c.r(4));
    c.set(3, object());
    c.put32(c.r(3) + 0x30, c.r(4));
    c.set(3, object());
    c.put32(c.r(3) + 0x2c, c.r(0));
    c.set(4, object());
    c.set(3, c.u32(c.r(4) + 0x2c));
    if (compare(c, std::int32_t(c.r(3))) > 0) {
        block(c, 0x10536e00, 2);
        c.set(3, c.r(3) * 4);
        c.call(0x10775130, 0x10536e08);
        block(c, 0x10536e08, 3);
        c.set(4, object());
        c.put32(c.r(4) + 0xc, c.r(3));
    } else {
        block(c, 0x10536e14, 1);
        c.put32(c.r(4) + 0xc, c.r(0));
    }
    block(c, 0x10536e18, 4);
    c.set(3, object());
    c.set(12, c.u32(c.r(3)));
    c.set(12, c.u32(c.r(12) + 0x94));
    call_virtual(c, 0x10536e28);
    block(c, 0x10536e28, 3);
    c.set(2, c.u32(frame + 0x14));
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x10536e34, 0xe0, 0x58, 0x47); return; }
    block(c, 0x10536e8c, 2);
    c.set(3, object());
    collect_direct(c, 0x10536e94);
    block(c, 0x10536e94, 2);
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x10536e9c, 0xc0, 0x50, 0x4e); return; }
    block(c, 0x10536ef4, 2);
    c.set(3, object());
    c.call(0x104a00a0, 0x10536efc);
    block(c, 0x10536efc, 2);
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x10536f04, 0xa0, 0x48, 0x55); return; }
    block(c, 0x10536f5c, 3);
    c.set({{3, object()}, {4, 0}});
    c.call(0x1049eb30, 0x10536f68);
    block(c, 0x10536f68, 2);
    if (!compare(c, std::int32_t(c.r(3)))) { fail(0x10536f70, 0x80, 0x40, 0x5e); return; }
    block(c, 0x10536fc8, 2);
    c.set(3, 0x634);
    c.call(0x10773520, 0x10536fd0);
    block(c, 0x10536fd0, 2);
    if (record_value(c, 27, std::int32_t(c.r(3)))) {
        block(c, 0x10536fd8, 2);
        c.set(4, object());
        c.call(0x104533a0, 0x10536fe0);
    }
    block(c, 0x10536fe0, 6);
    const guest::Kept<27> stage(c, c.r(27));
    c.set(3, object());
    c.put32(c.r(3) + 8, stage);
    c.set(3, object());
    c.set(0, c.u32(c.r(3) + 8));
    if (!compare_logical(c, c.r(0))) { fail(0x10536ff8, 0x60, 0x38, 0x65); return; }
    block(c, 0x10537050, 3);
    c.set(12, c.u32(c.r(3)));
    c.set(12, c.u32(c.r(12) + 0x64));
    call_virtual(c, 0x1053705c);
    block(c, 0x1053705c, 2);
    c.set(2, c.u32(frame + 0x14));
    c.set(3, object());
    finish();
}


void destroy_division(Cpu& c) {
    GuestNativeScope scope(c.s, destroy_entry);
    block(c, destroy_entry, 8);
    const auto stack = c.sp(), frame = stack - 0x40;
    c.set(0, c.lr());
    c.put32(stack - 4, c.r(31));
    const guest::Kept<31> delete_object(c, c.r(4));
    c.put32(stack - 8, c.r(30));
    const auto present = record_value(c, 30, std::int32_t(c.r(3)));
    const guest::Kept<30> object(c, c.r(30));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    if (present) {
        block(c, 0x105370a0, 4);
        c.set({{0, c.toc(-0x6000)}, {4, 0}});
        c.put32(object, c.r(0));
        c.call(0x104a6660, 0x105370b0);
        block(c, 0x105370b0, 3);
        c.set(3, c.u32(object + 0xbc));
        if (compare_logical(c, c.r(3))) {
            block(c, 0x105370bc, 1);
            c.call(0x10775170, 0x105370c0);
            block(c, 0x105370c0, 2);
            c.set(0, 0);
            c.put32(object + 0xbc, 0);
        }
        block(c, 0x105370c8, 3);
        c.set(3, c.u32(object + 8));
        if (compare_logical(c, c.r(3))) {
            block(c, 0x105370d4, 2);
            c.set(4, 1);
            c.call(0x104536c0, 0x105370dc);
        }
        block(c, 0x105370dc, 3);
        c.set({{3, object}, {4, 0}});
        c.call(0x1049e600, 0x105370e8);
        block(c, 0x105370e8, 2);
        if (record_value(c, 0, std::int16_t(std::uint32_t(delete_object))) > 0) {
            block(c, 0x105370f0, 2);
            c.set(3, object);
            c.call(0x107735a0, 0x105370f8);
        }
    }
    block(c, 0x105370f8, 7);
    c.set(3, object);
    c.set(0, c.u32(frame + 0x48));
    c.set(1, stack);
    c.set_lr(c.r(0));
    c.set(31, c.u32(stack - 4));
    c.set(30, c.u32(stack - 8));
    c.finish();
}

void link_divisions(Cpu& c) {
    GuestNativeScope scope(c.s, link_entry);
    block(c, link_entry, 16);
    const auto stack = c.sp(), frame = stack - 0x50;
    c.set(0, c.lr());
    guest::transfer_saved_registers<27>(c, stack);
    const guest::Kept<27> first_id(c, c.toc(-0x6738));
    const guest::Kept<28> object(c, c.r(3));
    const guest::Kept<29> divisions(c, c.toc(-0x7b5c));
    const guest::Kept<30> second_id(c, c.toc(-0x673c));
    const guest::Kept<31> regional_id(c, c.toc(-0x6750));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(0, c.u32(first_id));
    c.set(3, c.u32(divisions));
    c.set(0, c.r(0) * 4);
    c.set(3, c.u32(c.r(3) + c.r(0)));
    c.set(12, c.u32(c.r(3)));
    c.set(12, c.u32(c.r(12) + 0xac));
    call_virtual(c, 0x10539bf0);
    block(c, 0x10539bf0, 8);
    c.set(2, c.u32(frame + 0x14));
    c.set(0, c.u32(second_id));
    c.set(3, c.u32(divisions));
    c.set(0, c.r(0) * 4);
    c.set(3, c.u32(c.r(3) + c.r(0)));
    c.set(12, c.u32(c.r(3)));
    c.set(12, c.u32(c.r(12) + 0xac));
    call_virtual(c, 0x10539c10);
    block(c, 0x10539c10, 14);
    c.set(2, c.u32(frame + 0x14));
    c.set(4, c.u32(first_id));
    c.set(3, object);
    c.set(0, c.u32(second_id));
    c.set(6, 1);
    c.set(5, c.u32(divisions));
    c.set({{4, c.r(4) * 4}, {0, c.r(0) * 4}, {7, 1}});
    c.set(4, c.u32(c.r(5) + c.r(4)));
    c.set(8, UINT32_MAX);
    c.set(5, c.u32(c.r(5) + c.r(0)));
    c.set(9, UINT32_MAX);
    c.call(0x104a9210, 0x10539c48);
    block(c, 0x10539c48, 6);
    c.set(0, c.u32(regional_id));
    c.set(3, c.u32(divisions));
    c.set(0, c.r(0) * 4);
    c.set(3, c.u32(c.r(3) + c.r(0)));
    if (compare_logical(c, c.r(3))) {
        block(c, 0x10539c60, 3);
        c.set(12, c.u32(c.r(3)));
        c.set(12, c.u32(c.r(12) + 0xac));
        call_virtual(c, 0x10539c6c);
        block(c, 0x10539c6c, 10);
        c.set(2, c.u32(frame + 0x14));
        c.set(0, c.u32(regional_id));
        c.set(3, c.u32(divisions));
        c.set(0, c.r(0) * 4);
        c.set(3, c.u32(c.r(3) + c.r(0)));
        c.set(3, c.u32(c.r(3) + 0xc));
        c.set(3, c.u32(c.r(3)));
        c.set(12, c.u32(c.r(3)));
        c.set(12, c.u32(c.r(12) + 0xac));
        call_virtual(c, 0x10539c94);
        block(c, 0x10539c94, 14);
        c.set(2, c.u32(frame + 0x14));
        c.set(4, c.u32(second_id));
        c.set(3, object);
        c.set(0, c.u32(regional_id));
        c.set(6, 1);
        c.set(5, c.u32(divisions));
        c.set({{4, c.r(4) * 4}, {0, c.r(0) * 4}, {7, 1}});
        c.set(4, c.u32(c.r(5) + c.r(4)));
        c.set(8, UINT32_MAX);
        c.set(5, c.u32(c.r(5) + c.r(0)));
        c.set(9, UINT32_MAX);
        c.call(0x104a9210, 0x10539ccc);
        block(c, 0x10539ccc, 15);
        c.set(0, c.u32(regional_id));
        c.set(3, object);
        c.set(10, c.u32(divisions));
        c.set(6, 1);
        c.set(0, c.r(0) * 4);
        c.set(5, c.u32(second_id));
        c.set(4, c.u32(c.r(10) + c.r(0)));
        c.set(7, 1);
        c.set(0, c.r(5) * 4);
        c.set(8, UINT32_MAX);
        c.set(5, c.u32(c.r(4) + 0xc));
        c.set(9, UINT32_MAX);
        c.set(4, c.u32(c.r(10) + c.r(0)));
        c.set(5, c.u32(c.r(5)));
        c.call(0x104a9210, 0x10539d08);
    }
    block(c, 0x10539d08, 5);
    c.set(0, c.u32(frame + 0x58));
    c.set(1, stack);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<27, true>(c, stack);
    c.finish();
}

void load_standing(Cpu& c, std::uint32_t resume) {
    // The generated caller inlines the four-instruction external getter.
    // Preserve its read-before-charge boundary even when a budget expires.
    c.link(resume);
    c.set(0, c.r(4) * standing_size);
    c.set(3, c.u32(c.r(3) + 0xb0));
    c.set(3, c.r(3) + c.r(0));
    c.s.charge(4);
}

void promote_four(Cpu& c) {
    GuestNativeScope scope(c.s, promote_entry);
    block(c, promote_entry, 20);
    const auto stack = c.sp(), frame = stack - 0x90;
    c.set(0, c.lr());
    guest::transfer_saved_registers<22>(c, stack);
    const guest::Kept<28> second_id(c, c.toc(-0x673c));
    const guest::Kept<22> object(c, c.r(3));
    c.set(4, c.toc(-0x7b5c));
    guest::Kept<25> selected(c, 0);
    const guest::Kept<29> total(c, c.toc(-0x7b50));
    const guest::Kept<30> regional_id(c, c.toc(-0x6750));
    const guest::Kept<31> definitions(c, c.toc(-0x7bbc));
    c.set(26, c.toc(-0x7ba8));
    c.set(24, c.toc(-0x44f8));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(3, c.u32(second_id));
    c.set(5, c.u32(c.r(4)));
    c.set(4, c.r(3) * 4);
    c.set(0, c.u32(total));
    const guest::Kept<23> second(c, c.u32(c.r(5) + c.r(4)));
    c.set(3, c.r(0) * 4);
    c.call(0x10775130, 0x10539d70);
    block(c, 0x10539d70, 2);
    if (!record_value(c, 27, std::int32_t(c.r(3)))) {
        report_failure(c, 0x10539d78, c.r(24), c.r(26), 0x44, 0x3c, 0x354);
        block(c, 0x10539dc0, 4);
        c.set(3, c.toc(-0x7bac));
        c.set(0, 0);
        c.put8(c.r(3), 0);
    } else {
        const guest::Kept<27> candidates(c, c.r(27));
        block(c, 0x10539dd0, 4);
        c.set(3, c.toc(-0x7be8));
        guest::Kept<24> index(c, 0);
        guest::Kept<26> club(c, c.u32(c.r(3)));
        for (;;) {
            block(c, 0x10539e48, 3);
            c.set(0, c.u32(total));
            if (compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(c.r(0))) >= 0) break;
            block(c, 0x10539de4, 3);
            c.set(3, c.u32(club + 0x30));
            bool eligible = compare_logical(c, c.r(3)) != 0;
            if (eligible) {
                block(c, 0x10539df0, 3);
                c.set(0, c.u32(club + 0x2c));
                eligible = compare_logical(c, c.r(0)) != 0;
                if (eligible) {
                    block(c, 0x10539dfc, 4);
                    c.set(3, c.u32(c.r(3)));
                    c.set(0, c.u32(regional_id));
                    eligible = compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0;
                }
            }
            if (eligible) {
                block(c, 0x10539e0c, 4);
                c.set({{3, club}, {4, frame + 0x38}, {5, 1}});
                c.call(0x100ce320, 0x10539e1c);
                block(c, 0x10539e1c, 2);
                if (compare_logical(c, c.r(3))) {
                    block(c, 0x10539e24, 3);
                    c.set(0, c.u8(frame + 0x38));
                    eligible = record_value(c, 0, std::int8_t(c.r(0))) != 0;
                }
                if (eligible) {
                    block(c, 0x10539e30, 4);
                    c.set(0, std::uint32_t(std::int16_t(std::uint32_t(selected))));
                    selected = selected + 1;
                    c.set(0, c.r(0) * 4);
                    c.put32(candidates + c.r(0), club);
                }
            }
            block(c, 0x10539e40, 2);
            index = index + 1;
            club = club + club_stride;
        }
        block(c, 0x10539e54, 2);
        if (!record_value(c, 0, std::int16_t(std::uint32_t(selected)))) {
            block(c, 0x10539e5c, 2);
            c.set(3, candidates);
            c.call(0x10775170, 0x10539e64);
            block(c, 0x10539e64, 1);
        } else {
            block(c, 0x10539e68, 4);
            c.set({{3, candidates}, {4, selected}, {5, 4}});
            c.call(0x104635b0, 0x10539e78);
            block(c, 0x10539e78, 2);
            selected = 0;
            guest::Kept<24> cursor(c, candidates);
            // The original always consumes four slots after a nonempty scan.
            do {
                block(c, 0x10539e80, 8);
                c.set(0, c.u32(second_id));
                c.set(3, object);
                c.set(5, c.u32(definitions));
                c.set(6, 1);
                c.set(0, c.r(0) * definition_size);
                c.set(4, c.u32(cursor));
                c.set(5, c.r(5) + c.r(0));
                c.call(0x1049f780, 0x10539ea0);
                block(c, 0x10539ea0, 4);
                selected = selected + 1;
                cursor = cursor + 4;
            } while (compare(c, std::int32_t(std::uint32_t(selected)), 4) < 0);
            block(c, 0x10539eb0, 2);
            index = 0;
            for (;;) {
                block(c, 0x10539f08, 3);
                c.set(0, std::uint32_t(c.i16(second + 0x3e)));
                if (compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(c.r(0))) >= 0) break;
                block(c, 0x10539ebc, 3);
                c.set({{3, second}, {4, index}});
                load_standing(c, 0x10539ec8);
                block(c, 0x10539ec8, 3);
                c.set(0, c.u8(c.r(3) + 0x40));
                if (!compare(c, std::int32_t(c.r(0)), 3)) {
                    block(c, 0x10539ed4, 3);
                    c.set({{3, second}, {4, index}});
                    load_standing(c, 0x10539ee0);
                    block(c, 0x10539ee0, 9);
                    c.set(0, c.u32(regional_id));
                    c.set(4, c.r(3));
                    c.set(5, c.u32(definitions));
                    c.set(3, object);
                    c.set(0, c.r(0) * definition_size);
                    c.set(4, c.u32(c.r(4)));
                    c.set(6, 1);
                    c.set(5, c.r(5) + c.r(0));
                    c.call(0x1049f8e0, 0x10539f04);
                }
                block(c, 0x10539f04, 1);
                index = index + 1;
            }
            block(c, 0x10539f14, 2);
            c.set(3, candidates);
            c.call(0x10775170, 0x10539f1c);
        }
    }
    block(c, 0x10539f1c, 5);
    c.set(0, c.u32(frame + 0x98));
    c.set(1, stack);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<22, true>(c, stack);
    c.finish();
}

void fill_regional_slots(Cpu& c) {
    GuestNativeScope scope(c.s, fill_entry);
    block(c, fill_entry, 20);
    const auto stack = c.sp(), frame = stack - 0x3d0;
    c.set(0, c.lr());
    guest::transfer_saved_registers<17>(c, stack);
    c.set(4, c.toc(-0x7be8));
    const guest::Kept<31> object(c, c.r(3));
    const guest::Kept<22> missing_id(c, c.toc(-0x7a0c));
    guest::Kept<20> vacancies(c, 0);
    const guest::Kept<23> country_id(c, c.toc(-0x7af4));
    guest::Kept<19> selected(c, 0);
    const guest::Kept<24> divisions(c, c.toc(-0x7b5c));
    guest::Kept<21> index(c, 0);
    c.set(25, c.toc(-0x7b04));
    c.set(30, c.toc(-0x7b50));
    const guest::Kept<26> regional_id(c, c.toc(-0x6750));
    const guest::Kept<27> definitions(c, c.toc(-0x7bbc));
    const guest::Kept<28> buffer(c, c.toc(-0x7ba8));
    const guest::Kept<29> pool(c, c.toc(-0x44f8));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    guest::Kept<18> club(c, c.u32(c.r(4)));
    bool first_scan = true;
    for (;;) {
        // The reference joins the loop increment and next bound check in one
        // budget block; the initial cross-page branch enters its suffix.
        if (first_scan) block(c, 0x1053a02c, 6);
        first_scan = false;
        c.set(3, c.u32(c.r(25)));
        c.set(0, c.u32(c.r(30)));
        c.set(3, c.r(3) << 1);
        c.set(0, c.r(0) - c.r(3));
        if (compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(c.r(0))) >= 0) break;
        block(c, 0x10539f80, 3);
        c.set(3, c.u32(club + 0x2c));
        bool eligible = compare_logical(c, c.r(3)) != 0;
        if (eligible) {
            block(c, 0x10539f8c, 4);
            c.set(3, c.u32(c.r(3) + 0x68));
            c.set(0, c.u32(country_id));
            eligible = compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0;
        }
        if (eligible) {
            block(c, 0x10539f9c, 3);
            c.set(3, c.u32(club + 0x30));
            if (compare_logical(c, c.r(3))) {
                block(c, 0x10539fa8, 4);
                c.set(3, c.u32(c.r(3)));
                c.set(0, c.u32(missing_id));
                eligible = compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0;
            }
        }
        if (eligible) {
            block(c, 0x10539fb8, 4);
            c.set({{3, club}, {4, frame + 0x38}, {5, 1}});
            c.call(0x100ce320, 0x10539fc8);
            block(c, 0x10539fc8, 2);
            if (compare_logical(c, c.r(3))) {
                block(c, 0x10539fd0, 3);
                c.set(0, c.u8(frame + 0x38));
                if (!record_value(c, 0, std::int8_t(c.r(0)))) {
                    block(c, 0x10539fdc, 3);
                    c.set(3, c.u32(c.r(3) + 0x30));
                    eligible = compare_logical(c, c.r(3)) != 0;
                    if (eligible) {
                        block(c, 0x10539fe8, 4);
                        c.set(3, c.u32(c.r(3)));
                        c.set(0, c.u32(regional_id));
                        eligible = compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) != 0;
                        if (eligible) {
                            block(c, 0x10539ff8, 2);
                            c.set(0, c.u32(missing_id));
                            eligible = compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) != 0;
                            block(c, 0x1053a000, 1);
                        }
                    }
                }
            }
        }
        if (eligible) {
            block(c, 0x1053a004, 8);
            c.set(4, std::uint32_t(std::int16_t(std::uint32_t(selected))));
            selected = selected + 1;
            c.set(0, std::uint32_t(std::int16_t(std::uint32_t(selected))));
            c.set(3, frame + 0x64);
            c.set(4, c.r(4) * 4);
            const auto full = compare(c, std::int32_t(c.r(0)), 200) == 0;
            c.put32(c.r(3) + c.r(4), club);
            if (full) break;
        }
        block(c, 0x1053a024, 8);
        index = index + 1;
        club = club + club_stride;
    }
    block(c, 0x1053a044, 3);
    guest::Kept<17> group(c, UINT32_MAX);
    guest::Kept<25> child_offset(c, std::uint32_t(-4));
    for (;;) {
        block(c, 0x1053a0fc, 3);
        c.set(0, std::uint32_t(std::int8_t(std::uint32_t(group))));
        if (compare(c, std::int32_t(c.r(0)), 1) >= 0) break;
        block(c, 0x1053a050, 3);
        c.set(0, std::uint32_t(std::int8_t(std::uint32_t(group))));
        if (!compare(c, std::int32_t(c.r(0)), -1)) {
            block(c, 0x1053a05c, 5);
            c.set(0, c.u32(regional_id));
            c.set(3, c.u32(divisions));
            c.set(0, c.r(0) * 4);
            c.set(18, c.u32(c.r(3) + c.r(0)));
        } else {
            block(c, 0x1053a070, 6);
            c.set(0, c.u32(regional_id));
            c.set(3, c.u32(divisions));
            c.set(0, c.r(0) * 4);
            c.set(3, c.u32(c.r(3) + c.r(0)));
            c.set(3, c.u32(c.r(3) + 0xc));
            c.set(18, c.u32(c.r(3) + child_offset));
        }
        const guest::Kept<18> competition(c, c.r(18));
        block(c, 0x1053a088, 4);
        index = 0;
        guest::Kept<23> row_offset(c, index);
        const guest::Kept<30> zero(c, index);
        for (;;) {
            block(c, 0x1053a0e8, 3);
            c.set(0, std::uint32_t(c.i16(competition + 0x3e)));
            if (compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(c.r(0))) >= 0) break;
            block(c, 0x1053a09c, 5);
            c.set(0, c.u32(competition + 0xb0));
            c.set(4, c.r(0) + row_offset);
            c.set(0, c.u8(c.r(4) + 0x40));
            if (!compare(c, std::int32_t(c.r(0)), 3)) {
                block(c, 0x1053a0b0, 8);
                c.set(0, c.u32(missing_id));
                c.set(3, object);
                c.set(5, c.u32(definitions));
                c.set(6, 1);
                c.set(0, c.r(0) * definition_size);
                c.set(4, c.u32(c.r(4)));
                c.set(5, c.r(5) + c.r(0));
                c.call(0x1049f8e0, 0x1053a0d0);
                block(c, 0x1053a0d0, 4);
                c.set(3, c.u32(competition + 0xb0));
                vacancies = vacancies + 1;
                c.set(3, c.u32(c.r(3) + row_offset));
                c.put32(c.r(3) + 0x38, zero);
            }
            block(c, 0x1053a0e0, 2);
            row_offset = row_offset + standing_size;
            index = index + 1;
        }
        block(c, 0x1053a0f4, 2);
        child_offset = child_offset + 4;
        group = group + 1;
    }
    block(c, 0x1053a108, 7);
    c.set(0, c.u32(regional_id));
    c.set(3, c.u32(divisions));
    c.set(0, c.r(0) * 4);
    c.set(3, c.u32(c.r(3) + c.r(0)));
    c.set(0, std::uint32_t(c.i16(c.r(3) + 0x3e)));
    if (!compare(c, std::int32_t(c.r(0)), 19)) {
        block(c, 0x1053a124, 1);
        vacancies = vacancies - 1;
    }
    block(c, 0x1053a128, 4);
    c.set(3, std::uint32_t(std::int16_t(std::uint32_t(selected))));
    c.set(0, std::uint32_t(std::int16_t(std::uint32_t(vacancies))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) < 0) {
        report_failure(c, 0x1053a138, pool, buffer, 0x44, 0x3c, 0x3d8);
        block(c, 0x1053a180, 4);
        c.set(3, c.toc(-0x7bac));
        c.set(0, 0);
        c.put8(c.r(3), 0);
    } else {
        block(c, 0x1053a190, 4);
        c.set({{4, selected}, {5, vacancies}, {3, frame + 0x64}});
        c.call(0x104635b0, 0x1053a1a0);
        block(c, 0x1053a1a0, 4);
        const guest::Kept<18> count(c, std::uint32_t(std::int16_t(std::uint32_t(selected))));
        guest::Kept<17> cursor(c, frame + 0x64);
        selected = 0;
        for (;;) {
            block(c, 0x1053a1dc, 2);
            if (compare(c, std::int32_t(std::uint32_t(selected)), std::int32_t(std::uint32_t(count))) >= 0) break;
            block(c, 0x1053a1e4, 2);
            if (!record_value(c, 0, std::int16_t(std::uint32_t(vacancies)))) break;
            block(c, 0x1053a1b0, 8);
            c.set(0, c.u32(regional_id));
            c.set(3, object);
            c.set(5, c.u32(definitions));
            c.set(6, 1);
            c.set(0, c.r(0) * definition_size);
            c.set(4, c.u32(cursor));
            c.set(5, c.r(5) + c.r(0));
            c.call(0x1049f780, 0x1053a1d0);
            block(c, 0x1053a1d0, 3);
            vacancies = vacancies - 1;
            cursor = cursor + 4;
            selected = selected + 1;
        }
    }
    block(c, 0x1053a1ec, 5);
    c.set(0, c.u32(frame + 0x3d8));
    c.set(1, stack);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<17, true>(c, stack);
    c.finish();
}

void coordinate_reserves(Cpu& c) {
    GuestNativeScope scope(c.s, reserves_entry);
    block(c, reserves_entry, 25);
    const auto stack = c.sp(), frame = stack - 0xa0;
    c.set(0, c.lr());
    guest::transfer_saved_registers<19>(c, stack);
    const guest::Kept<26> regional_id(c, c.toc(-0x6750));
    guest::Kept<24> index(c, 0);
    c.set(3, c.toc(-0x7b5c));
    guest::Kept<25> offset(c, 0);
    const guest::Kept<27> south_id(c, c.toc(-0x674c));
    const guest::Kept<28> north_id(c, c.toc(-0x6748));
    const guest::Kept<29> failure_flag(c, c.toc(-0x7bac));
    const guest::Kept<30> buffer(c, c.toc(-0x7ba8));
    const guest::Kept<31> pool(c, c.toc(-0x44f8));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(0, c.u32(regional_id));
    c.set(4, c.u32(c.r(3)));
    c.set(0, c.r(0) * 4);
    c.set(3, c.toc(-0x673c));
    c.set(5, c.u32(c.r(4) + c.r(0)));
    c.set(0, c.u32(c.r(3)));
    c.set(3, c.u32(c.r(5) + 0xc));
    const guest::Kept<22> regional(c, c.r(5));
    c.set(0, c.r(0) * 4);
    const guest::Kept<23> second(c, c.u32(c.r(4) + c.r(0)));
    const guest::Kept<21> child(c, c.u32(c.r(3)));
    // An unrecognized previous division leaves the incoming/prior row in r19.
    // Preserve that original residue rather than inventing a null default.
    guest::Kept<19> destination(c, c.r(19));
    for (;;) {
        block(c, 0x1053aaa4, 4);
        c.set(0, std::uint32_t(c.i16(second + 0x3e)));
        c.set(3, std::uint32_t(std::int16_t(std::uint32_t(index))));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) >= 0) break;
        block(c, 0x1053a968, 5);
        c.set(3, c.u32(second + 0xb0));
        c.set({{4, frame + 0x38}, {5, 1}});
        c.set(3, c.u32(c.r(3) + offset));
        c.call(0x100ce320, 0x1053a97c);
        block(c, 0x1053a97c, 2);
        const auto found = record_value(c, 20, std::int32_t(c.r(3)));
        const guest::Kept<20> partner(c, c.r(20));
        bool eligible = found != 0;
        if (eligible) {
            block(c, 0x1053a984, 3);
            c.set(0, c.u8(frame + 0x38));
            eligible = record_value(c, 0, std::int8_t(c.r(0))) != 0;
        }
        if (eligible) {
            block(c, 0x1053a990, 3);
            c.set(3, c.u32(partner + 0x30));
            eligible = compare_logical(c, c.r(3)) != 0;
        }
        if (eligible) {
            block(c, 0x1053a99c, 4);
            c.set(3, c.u32(c.r(3)));
            c.set(0, c.u32(regional_id));
            eligible = compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0;
        }
        if (eligible) {
            block(c, 0x1053a9ac, 5);
            c.set(3, c.u32(partner + 0x38));
            c.set(0, c.u32(north_id));
            c.set(3, c.u32(c.r(3)));
            if (!compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0)))) {
                block(c, 0x1053a9c0, 4);
                c.set({{3, regional}, {4, partner}, {5, 0}});
                c.call(0x104aa560, 0x1053a9d0);
                block(c, 0x1053a9d0, 2);
                destination = c.r(3);
            } else {
                block(c, 0x1053a9d8, 3);
                c.set(0, c.u32(south_id));
                if (!compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0)))) {
                    block(c, 0x1053a9e4, 4);
                    c.set({{3, child}, {4, partner}, {5, 0}});
                    c.call(0x104aa560, 0x1053a9f4);
                    block(c, 0x1053a9f4, 1);
                    destination = c.r(3);
                }
            }
            block(c, 0x1053a9f8, 2);
            if (!compare_logical(c, destination)) {
                report_failure(c, 0x1053aa00, pool, buffer, 0x44, 0x3c, 0x4d2);
                block(c, 0x1053aa48, 3);
                c.set(0, 0);
                c.put8(failure_flag, 0);
            } else {
                block(c, 0x1053aa54, 6);
                c.set(3, c.u32(second + 0xb0));
                c.set(0, offset + 0x40);
                c.set(0, std::uint32_t(c.i8(c.r(3) + c.r(0))));
                if (!compare(c, std::int32_t(c.r(0)), 3)) {
                    block(c, 0x1053aa6c, 3);
                    c.set(0, c.u8(destination + 0x40));
                    if (compare(c, std::int32_t(c.r(0)), 3)) {
                        block(c, 0x1053aa78, 9);
                        c.set({{0, 3}, {3, regional}});
                        c.put8(destination + 0x40, 3);
                        c.set({{5, partner}, {7, 1}});
                        c.set(6, c.u32(second + 0xb0));
                        c.set(4, c.u32(second + 4));
                        c.set(6, c.u32(c.r(6) + offset));
                        c.call(0x10452a60, 0x1053aa9c);
                    }
                }
            }
        }
        block(c, 0x1053aa9c, 2);
        offset = offset + standing_size;
        index = index + 1;
    }
    block(c, 0x1053aab4, 5);
    c.set(0, c.u32(frame + 0xa8));
    c.set(1, stack);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<19, true>(c, stack);
    c.finish();
}

void split_region_assignments(Cpu& c, std::uint32_t frame, unsigned count,
                              unsigned north_count, unsigned south_count,
                              std::uint32_t north_definition, std::uint32_t south_definition) {
    const auto combined = frame + 0xf0, north = frame + 0x138, previous = frame + 0x60;
    const auto set_ctr = [&](unsigned value) { c.s.wr<4>(guest::reg::ctr, value); };
    block(c, 0x1053a4e4, 3);
    const auto nonempty = compare(c, std::int32_t(count)) > 0;
    unsigned copied = 0;
    c.set(4, copied);
    if (nonempty) {
        block(c, 0x1053a4f0, 3);
        const auto wide = compare(c, std::int32_t(count), 8) > 0;
        c.set(12, count - 8);
        if (wide) {
            block(c, 0x1053a4fc, 6);
            c.set(6, (count - 1) / 8);
            c.set(27, 0);
            const auto batches = c.r(6);
            set_ctr(batches);
            if (compare(c, std::int32_t(count - 8)) > 0) {
                for (unsigned batch = 0; batch < batches; ++batch) {
                    block(c, 0x1053a514, 31);
                    const auto from = combined + copied * 4, to = previous + copied * 4;
                    c.set(26, from);
                    c.set(28, previous);
                    c.set(4, copied += 8);
                    const auto club0 = c.u32(from); c.set(12, club0);
                    c.set(28, to);
                    const auto club4 = c.u32(from + 16); c.set(31, club4);
                    c.set(27, copied * 4);
                    const auto old0 = c.u32(club0 + 0x38); c.set(29, old0);
                    const auto club1 = c.u32(from + 4); c.set(6, club1);
                    const auto club2 = c.u32(from + 8); c.set(12, club2);
                    c.put32(to, old0);
                    const auto old1 = c.u32(club1 + 0x38); c.set(29, old1);
                    const auto club3 = c.u32(from + 12); c.set(6, club3);
                    c.put32(to + 4, old1);
                    const auto old2 = c.u32(club2 + 0x38); c.set(12, old2);
                    const auto old3 = c.u32(club3 + 0x38); c.set(29, old3);
                    c.put32(to + 8, old2);
                    const auto club5 = c.u32(from + 20); c.set(30, club5);
                    const auto club6 = c.u32(from + 24); c.set(12, club6);
                    const auto club7 = c.u32(from + 28); c.set(6, club7);
                    c.put32(to + 12, old3);
                    const auto old4 = c.u32(club4 + 0x38); c.set(31, old4);
                    const auto old5 = c.u32(club5 + 0x38); c.set(30, old5);
                    c.put32(to + 16, old4);
                    const auto old6 = c.u32(club6 + 0x38); c.set(12, old6);
                    c.put32(to + 20, old5);
                    const auto old7 = c.u32(club7 + 0x38); c.set(6, old7);
                    c.put32(to + 24, old6);
                    c.put32(to + 28, old7);
                    set_ctr(batches - batch - 1);
                }
            }
        }
        block(c, 0x1053a590, 9);
        c.set(27, copied * 4);
        auto source = combined + copied * 4, destination = previous + copied * 4;
        c.set(30, source);
        c.set(12, destination);
        c.set(6, count - copied);
        set_ctr(count - copied);
        if (compare(c, std::int32_t(copied), std::int32_t(count)) < 0) {
            for (unsigned i = copied; i < count; ++i) {
                block(c, 0x1053a5b4, 6);
                const auto club = c.u32(source); c.set(4, club);
                c.set(30, source += 4);
                const auto old = c.u32(club + 0x38); c.set(4, old);
                c.put32(destination, old);
                c.set(12, destination += 4);
                set_ctr(count - i - 1);
            }
        }
    }
    block(c, 0x1053a5cc, 3);
    const auto have_north = record_value(c, 12, std::int16_t(north_count)) > 0;
    unsigned assigned = 0;
    c.set(27, assigned);
    if (have_north) {
        block(c, 0x1053a5d8, 3);
        const auto wide = compare(c, std::int16_t(north_count), 8) > 0;
        c.set(6, north_count - 8);
        if (wide) {
            block(c, 0x1053a5e4, 6);
            const auto batches = (north_count - 1) / 8;
            c.set(4, batches);
            c.set(11, 0);
            set_ctr(batches);
            if (compare(c, std::int32_t(north_count - 8)) > 0) {
                for (unsigned batch = 0; batch < batches; ++batch) {
                    block(c, 0x1053a5fc, 20);
                    const auto source = north + assigned * 4;
                    c.set(26, source);
                    c.set(11, (batch + 1) * 32);
                    auto left = c.u32(source); c.set(6, left);
                    c.set(27, assigned += 8);
                    auto right = c.u32(source + 4); c.set(4, right);
                    c.put32(left + 0x38, north_definition);
                    for (unsigned lane = 2; lane < 8; lane += 2) {
                        left = c.u32(source + lane * 4); c.set(6, left);
                        c.put32(right + 0x38, north_definition);
                        right = c.u32(source + (lane + 1) * 4); c.set(4, right);
                        c.put32(left + 0x38, north_definition);
                    }
                    c.put32(right + 0x38, north_definition);
                    set_ctr(batches - batch - 1);
                }
            }
        }
        block(c, 0x1053a64c, 6);
        auto cursor = north + assigned * 4;
        c.set(6, cursor);
        c.set(4, north_count - assigned);
        set_ctr(north_count - assigned);
        if (compare(c, std::int32_t(assigned), std::int16_t(north_count)) < 0) {
            for (unsigned i = assigned; i < north_count; ++i) {
                block(c, 0x1053a664, 4);
                const auto club = c.u32(cursor); c.set(4, club);
                c.set(6, cursor += 4);
                c.put32(club + 0x38, north_definition);
                set_ctr(north_count - i - 1);
            }
        }
    }
    block(c, 0x1053a674, 3);
    const auto have_south = record_value(c, 10, std::int16_t(south_count)) > 0;
    assigned = 0;
    c.set(12, assigned);
    if (have_south) {
        block(c, 0x1053a680, 3);
        const auto wide = compare(c, std::int16_t(south_count), 8) > 0;
        c.set(4, south_count - 8);
        if (wide) {
            block(c, 0x1053a68c, 6);
            const auto batches = (south_count - 1) / 8;
            c.set(0, batches);
            c.set(9, 0);
            set_ctr(batches);
            if (compare(c, std::int32_t(south_count - 8)) > 0) {
                for (unsigned batch = 0; batch < batches; ++batch) {
                    block(c, 0x1053a6a4, 20);
                    const auto source = combined + assigned * 4;
                    c.set(11, source);
                    c.set(9, (batch + 1) * 32);
                    auto left = c.u32(source); c.set(6, left);
                    c.set(12, assigned += 8);
                    auto right = c.u32(source + 4); c.set(4, right);
                    c.put32(left + 0x38, south_definition);
                    for (unsigned lane = 2; lane < 8; lane += 2) {
                        left = c.u32(source + lane * 4); c.set(6, left);
                        c.put32(right + 0x38, south_definition);
                        right = c.u32(source + (lane + 1) * 4); c.set(4, right);
                        c.put32(left + 0x38, south_definition);
                    }
                    c.put32(right + 0x38, south_definition);
                    set_ctr(batches - batch - 1);
                }
            }
        }
        block(c, 0x1053a6f4, 6);
        auto cursor = combined + assigned * 4;
        c.set(4, cursor);
        c.set(0, south_count - assigned);
        set_ctr(south_count - assigned);
        if (compare(c, std::int32_t(assigned), std::int16_t(south_count)) < 0) {
            for (unsigned i = assigned; i < south_count; ++i) {
                block(c, 0x1053a70c, 4);
                const auto club = c.u32(cursor); c.set(6, club);
                c.set(4, cursor += 4);
                c.put32(club + 0x38, south_definition);
                set_ctr(south_count - i - 1);
            }
        }
    }
    // Preserve previous memberships whenever a pair can exchange its proposed
    // regions. Later rows observe every restoration made by an earlier pair.
    block(c, 0x1053a71c, 5);
    c.set(9, count - 1);
    auto club_cursor = combined, old_cursor = previous;
    c.set({{6, club_cursor}, {4, old_cursor}, {29, 0}});
    for (unsigned i = 0;;) {
        block(c, 0x1053a7d0, 2);
        if (compare(c, std::int32_t(i), std::int32_t(count - 1)) >= 0) break;
        block(c, 0x1053a730, 5);
        const auto first = c.u32(club_cursor); c.set(30, first);
        const auto first_old = c.u32(old_cursor); c.set(28, first_old);
        const auto first_new = c.u32(first + 0x38); c.set(26, first_new);
        if (compare_logical(c, first_new, first_old)) {
            block(c, 0x1053a744, 10);
            unsigned j = i + 1;
            c.set(25, j);
            c.set(12, combined);
            c.set(27, j * 4);
            c.set(10, previous);
            c.set(0, count - j);
            auto partner_cursor = combined + j * 4, partner_old = previous + j * 4;
            c.set(8, partner_cursor);
            c.set(7, partner_old);
            set_ctr(count - j);
            if (compare(c, std::int32_t(j), std::int32_t(count)) < 0) {
                block(c, 0x1053a76c, 1);
                for (; j < count;) {
                    block(c, 0x1053a770, 5);
                    const auto second = c.u32(partner_cursor); c.set(11, second);
                    const auto second_old = c.u32(partner_old); c.set(0, second_old);
                    const auto second_new = c.u32(second + 0x38); c.set(11, second_new);
                    bool exchange = compare_logical(c, second_new, second_old) != 0;
                    if (exchange) {
                        block(c, 0x1053a784, 2);
                        exchange = compare_logical(c, first_new, second_old) == 0;
                    }
                    if (exchange) {
                        block(c, 0x1053a78c, 2);
                        exchange = compare_logical(c, second_new, first_old) == 0;
                    }
                    if (exchange) {
                        block(c, 0x1053a794, 6);
                        c.set(7, j * 4);
                        c.put32(first + 0x38, first_old);
                        const auto old = c.u32(previous + j * 4); c.set(0, old);
                        const auto club = c.u32(combined + j * 4); c.set(7, club);
                        c.put32(club + 0x38, old);
                        break;
                    }
                    block(c, 0x1053a7b0, 5);
                    c.set(27, (j + 1) * 4);
                    c.set(7, partner_old += 4);
                    c.set(8, partner_cursor += 4);
                    c.set(25, ++j);
                    set_ctr(count - j);
                }
            }
        }
        block(c, 0x1053a7c4, 3);
        c.set(4, old_cursor += 4);
        c.set(6, club_cursor += 4);
        c.set(29, ++i);
    }
    // The original also looks for three-way cycles. Keep its ordered fresh
    // reads and partial writes, including observable mutation/alias cases.
    block(c, 0x1053a7d8, 4);
    club_cursor = combined; old_cursor = previous;
    c.set({{6, club_cursor}, {4, old_cursor}, {31, 0}});
    for (unsigned i = 0;;) {
        block(c, 0x1053a8e4, 2);
        if (compare(c, std::int32_t(i), std::int32_t(count)) >= 0) break;
        // The mutation observer watches this read exactly; generation isolates
        // it from the following run so a finite budget exposes the same prefix.
        block(c, 0x1053a7e8, 1);
        const auto first = c.u32(club_cursor); c.set(7, first);
        block(c, 0x1053a7ec, 4);
        const auto first_old = c.u32(old_cursor); c.set(0, first_old);
        const auto first_new = c.u32(first + 0x38); c.set(7, first_new);
        if (compare_logical(c, first_new, first_old)) {
            block(c, 0x1053a7fc, 2);
            unsigned j = 0;
            c.set(25, j);
            for (;;) {
                block(c, 0x1053a8d0, 2);
                if (compare(c, std::int32_t(j), std::int32_t(count)) >= 0) break;
                block(c, 0x1053a804, 2);
                if (compare(c, std::int32_t(i), std::int32_t(j))) {
                    block(c, 0x1053a80c, 8);
                    c.set(11, j * 4);
                    c.set(30, combined);
                    const auto second = c.u32(combined + j * 4); c.set(7, second);
                    c.set(29, previous);
                    const auto second_old = c.u32(previous + j * 4); c.set(12, second_old);
                    const auto second_new = c.u32(second + 0x38); c.set(27, second_new);
                    if (compare_logical(c, second_new, second_old)) {
                        block(c, 0x1053a82c, 7);
                        auto third_cursor = combined, third_old_cursor = previous;
                        c.set({{8, third_cursor}, {7, third_old_cursor}, {24, 0}, {10, 0}});
                        set_ctr(count);
                        if (compare(c, std::int32_t(count)) > 0) {
                            for (unsigned k = 0; k < count;) {
                                block(c, 0x1053a848, 2);
                                bool cycle = compare(c, std::int32_t(k), std::int32_t(i)) != 0;
                                if (cycle) {
                                    block(c, 0x1053a850, 2);
                                    cycle = compare(c, std::int32_t(k), std::int32_t(j)) != 0;
                                }
                                if (cycle) {
                                    block(c, 0x1053a858, 5);
                                    const auto third = c.u32(third_cursor); c.set(9, third);
                                    const auto third_old = c.u32(third_old_cursor); c.set(26, third_old);
                                    const auto third_new = c.u32(third + 0x38); c.set(28, third_new);
                                    cycle = compare_logical(c, third_new, third_old) != 0;
                                    std::uint32_t restore_first = 0, restore_first_old = 0;
                                    if (cycle) {
                                        block(c, 0x1053a86c, 4);
                                        restore_first = c.u32(club_cursor); c.set(9, restore_first);
                                        const auto current_first = c.u32(restore_first + 0x38); c.set(0, current_first);
                                        cycle = compare_logical(c, current_first, second_old) == 0;
                                    }
                                    if (cycle) {
                                        block(c, 0x1053a87c, 2);
                                        cycle = compare_logical(c, second_new, third_old) == 0;
                                    }
                                    if (cycle) {
                                        block(c, 0x1053a884, 3);
                                        restore_first_old = c.u32(old_cursor); c.set(0, restore_first_old);
                                        cycle = compare_logical(c, third_new, restore_first_old) == 0;
                                    }
                                    if (cycle) {
                                        block(c, 0x1053a890, 10);
                                        c.put32(restore_first + 0x38, restore_first_old);
                                        c.set(9, k * 4);
                                        const auto restore_second = c.u32(previous + j * 4); c.set(8, restore_second);
                                        const auto second_index = j;
                                        j = count;
                                        c.set(25, j);
                                        const auto fresh_second = c.u32(combined + second_index * 4); c.set(7, fresh_second);
                                        const auto restore_third = c.u32(previous + k * 4); c.set(0, restore_third);
                                        c.put32(fresh_second + 0x38, restore_second);
                                        const auto fresh_third = c.u32(combined + k * 4); c.set(7, fresh_third);
                                        c.put32(fresh_third + 0x38, restore_third);
                                        break;
                                    }
                                }
                                block(c, 0x1053a8b8, 5);
                                c.set(10, (k + 1) * 4);
                                c.set(7, third_old_cursor += 4);
                                c.set(8, third_cursor += 4);
                                c.set(24, ++k);
                                set_ctr(count - k);
                            }
                        }
                    }
                }
                block(c, 0x1053a8cc, 1);
                c.set(25, ++j);
            }
        }
        block(c, 0x1053a8d8, 3);
        c.set(4, old_cursor += 4);
        c.set(6, club_cursor += 4);
        c.set(31, ++i);
    }
}

// Two contiguous 18-club lists are also the original's combined 36-club
// scratch array. Keep the ordered guest accesses: observers and aliases see
// the compiler's pipelined copies, even though iteration lives in C++ locals.
void split_regions(Cpu& c) {
    GuestNativeScope scope(c.s, split_entry);
    block(c, split_entry, 34);
    const auto stack = c.sp(), frame = stack - 0x230;
    c.set(0, c.lr());
    guest::transfer_saved_registers<24>(c, stack);
    c.set(4, c.toc(-0x6748));
    unsigned selected_count = 0, located_count = 0, north_count = 0, south_count = 0;
    c.set(5, selected_count);
    c.set(3, c.toc(-0x674c));
    c.set(6, located_count);
    c.set(7, c.toc(-0x7bbc));
    c.set(9, south_count);
    c.set(27, c.toc(-0x7b04));
    c.set(11, north_count);
    c.set(12, c.toc(-0x7b50));
    c.set(28, c.toc(-0x7be8));
    c.put32(stack + 8, c.r(0));
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(0, c.u32(c.r(4)));
    const auto south_list = frame + 0xf0, north_list = frame + 0x138;
    const auto candidates = frame + 0x180;
    c.set(8, south_list);
    c.set(4, c.u32(c.r(3)));
    c.set(10, north_list);
    c.set(30, c.r(0) * definition_size);
    c.set(31, c.u32(c.r(7)));
    c.set(0, c.u32(c.r(27)));
    c.set(3, 0);
    c.set(12, c.u32(c.r(12)));
    c.set(7, c.r(4) * definition_size);
    c.set(29, c.r(0) * 2);
    const auto constants = c.toc(-0x44fc);
    c.set(4, constants);
    const auto north_definition = c.r(31) + c.r(30);
    c.set(0, north_definition);
    const auto regional_id = c.toc(-0x6750);
    c.set(27, regional_id);
    const auto south_definition = c.r(31) + c.r(7);
    c.set(7, south_definition);
    const auto buffer = c.toc(-0x7ba8);
    c.set(30, buffer);
    const auto total_clubs = c.r(12) - c.r(29);
    c.set(29, total_clubs);
    const auto pool = c.toc(-0x44f8);
    c.set(31, pool);
    auto club = c.u32(c.r(28));
    c.set(24, club);
    for (unsigned examined = 0;;) {
        block(c, 0x1053a2dc, 2);
        if (compare(c, std::int32_t(examined), std::int32_t(total_clubs)) >= 0) break;
        block(c, 0x1053a2e4, 3);
        c.set(12, std::uint32_t(std::int8_t(selected_count)));
        if (compare(c, std::int32_t(c.r(12)), 36) >= 0) break;
        block(c, 0x1053a288, 3);
        const auto definition = c.u32(club + 0x30);
        c.set(12, definition);
        bool eligible = compare_logical(c, definition) != 0;
        if (eligible) {
            block(c, 0x1053a294, 4);
            const auto id = c.u32(definition);
            c.set(28, id);
            c.set(12, c.u32(regional_id));
            eligible = compare(c, std::int32_t(id), std::int32_t(c.r(12))) == 0;
        }
        if (eligible) {
            block(c, 0x1053a2a4, 8);
            const auto location = c.u32(club + 0x3c);
            c.set(26, location);
            c.set(12, std::uint32_t(std::int8_t(selected_count)));
            c.set(28, c.r(12) * 4);
            c.set(12, candidates);
            const auto has_location = compare_logical(c, location) != 0;
            c.put32(candidates + c.r(28), club);
            c.set(5, ++selected_count);
            if (has_location) {
                block(c, 0x1053a2c4, 3);
                c.set(12, c.u32(location + 0x10));
                if (compare_logical(c, c.r(12))) {
                    block(c, 0x1053a2d0, 1);
                    c.set(6, ++located_count);
                }
            }
        }
        block(c, 0x1053a2d4, 2);
        c.set(3, ++examined);
        club += club_stride;
        c.set(24, club);
    }
    block(c, 0x1053a2f0, 3);
    c.set(3, std::uint32_t(std::int8_t(selected_count)));
    if (compare(c, std::int32_t(c.r(3)), 36)) {
        report_failure(c, 0x1053a2fc, pool, buffer, 0x40, 0x38, 0x419);
        block(c, 0x1053a344, 4);
        c.set(3, c.toc(-0x7bac));
        c.set(0, 0);
        c.put8(c.r(3), 0);
    } else {
        guest::Fpu fp(c);
        const auto set_ctr = [&](unsigned count) { c.s.wr<4>(guest::reg::ctr, count); };
        block(c, 0x1053a354, 3);
        auto north_threshold = c.f64(constants), south_threshold = 0.0;
        c.set_f(2, north_threshold);
        c.set(29, std::uint32_t(std::int8_t(located_count)));
        south_threshold = c.f64(constants + 8);
        c.set_f(3, south_threshold);
        for (;;) {
            block(c, 0x1053a360, 5);
            auto cursor = candidates;
            c.set(6, cursor);
            c.set(27, 0);
            set_ctr(selected_count);
            const auto scan = compare(c, std::int32_t(selected_count)) > 0;
            if (scan) {
                block(c, 0x1053a374, 1);
                for (unsigned i = 0; i < selected_count; ++i) {
                    block(c, 0x1053a378, 3);
                    const auto candidate = c.u32(cursor);
                    c.set(26, candidate);
                    bool located = compare_logical(c, candidate) != 0;
                    auto coordinates = 0u;
                    if (located) {
                        block(c, 0x1053a384, 3);
                        const auto location = c.u32(candidate + 0x3c);
                        c.set(12, location);
                        located = compare_logical(c, location) != 0;
                        if (located) {
                            block(c, 0x1053a390, 3);
                            coordinates = c.u32(location + 0x10);
                            c.set(12, coordinates);
                            located = compare_logical(c, coordinates) != 0;
                        }
                    }
                    if (located) {
                        block(c, 0x1053a39c, 3);
                        const auto latitude = c.f64(coordinates);
                        c.set_f(0, latitude);
                        fp.fcmp(0, latitude, north_threshold);
                        bool northern = (c.cr(0) & 4) != 0;
                        if (northern) {
                            block(c, 0x1053a3a8, 3);
                            c.set(12, std::uint32_t(std::int16_t(north_count)));
                            northern = compare(c, std::int32_t(c.r(12)), 18) < 0;
                        }
                        if (northern) {
                            block(c, 0x1053a3b4, 6);
                            c.set(30, north_count * 4);
                            c.set(12, 0);
                            c.put32(north_list + north_count * 4, candidate);
                            c.set(11, ++north_count);
                            c.put32(cursor, 0);
                        } else {
                            block(c, 0x1053a3cc, 2);
                            fp.fcmp(0, latitude, south_threshold);
                            if (c.cr(0) & 8) {
                                block(c, 0x1053a3d4, 3);
                                c.set(12, std::uint32_t(std::int16_t(south_count)));
                                if (compare(c, std::int32_t(c.r(12)), 18) < 0) {
                                    block(c, 0x1053a3e0, 5);
                                    c.set(30, south_count * 4);
                                    c.set(12, 0);
                                    c.put32(south_list + south_count * 4, candidate);
                                    c.set(9, ++south_count);
                                    c.put32(cursor, 0);
                                }
                            }
                        }
                    }
                    block(c, 0x1053a3f4, 3);
                    c.set(27, (i + 1) * 4);
                    c.set(6, cursor += 4);
                    set_ctr(selected_count - i - 1);
                }
            }
            block(c, 0x1053a400, 5);
            c.set(12, std::uint32_t(std::int16_t(north_count)));
            c.set(6, std::uint32_t(std::int16_t(south_count)));
            c.set(6, c.r(12) + c.r(6));
            if (!compare(c, std::int8_t(located_count), std::int32_t(c.r(6)))) break;
            block(c, 0x1053a414, 5);
            const auto north_step = c.f64(constants + 0x10);
            c.set_f(1, north_step);
            const auto south_step = c.f64(constants + 0x18);
            c.set_f(0, south_step);
            north_threshold = fp.fsub(north_threshold, north_step);
            c.set_f(2, north_threshold);
            south_threshold = fp.fadd(south_threshold, south_step);
            c.set_f(3, south_threshold);
        }
        block(c, 0x1053a428, 2);
        unsigned pass = 0;
        c.set(24, pass);
        for (;;) {
            block(c, 0x1053a4d8, 3);
            c.set(4, std::uint32_t(std::int8_t(pass)));
            if (compare(c, std::int32_t(c.r(4)), 2) >= 0) break;
            block(c, 0x1053a430, 5);
            auto cursor = candidates;
            c.set(4, cursor);
            c.set(27, 0);
            set_ctr(selected_count);
            if (compare(c, std::int32_t(selected_count)) > 0) {
                block(c, 0x1053a444, 1);
                for (unsigned i = 0; i < selected_count; ++i) {
                    block(c, 0x1053a448, 3);
                    const auto candidate = c.u32(cursor);
                    c.set(26, candidate);
                    if (compare_logical(c, candidate)) {
                        block(c, 0x1053a454, 3);
                        c.set(6, std::uint32_t(std::int8_t(pass)));
                        bool northern = compare(c, std::int32_t(c.r(6)), 1) == 0;
                        if (!northern) {
                            block(c, 0x1053a460, 3);
                            c.set(6, c.u32(candidate + 0x38));
                            northern = compare_logical(c, c.r(6), north_definition) == 0;
                        }
                        if (northern) {
                            block(c, 0x1053a46c, 3);
                            c.set(6, std::uint32_t(std::int16_t(north_count)));
                            northern = compare(c, std::int32_t(c.r(6)), 18) < 0;
                        }
                        if (northern) {
                            block(c, 0x1053a478, 6);
                            c.set(12, north_count * 4);
                            c.set(6, 0);
                            c.put32(north_list + north_count * 4, candidate);
                            c.set(11, ++north_count);
                            c.put32(cursor, 0);
                        } else {
                            block(c, 0x1053a490, 3);
                            c.set(6, std::uint32_t(std::int8_t(pass)));
                            bool southern = compare(c, std::int32_t(c.r(6)), 1) == 0;
                            if (!southern) {
                                block(c, 0x1053a49c, 3);
                                c.set(6, c.u32(candidate + 0x38));
                                southern = compare_logical(c, c.r(6), south_definition) == 0;
                            }
                            if (southern) {
                                block(c, 0x1053a4a8, 3);
                                c.set(6, std::uint32_t(std::int16_t(south_count)));
                                southern = compare(c, std::int32_t(c.r(6)), 18) < 0;
                            }
                            if (southern) {
                                block(c, 0x1053a4b4, 5);
                                c.set(12, south_count * 4);
                                c.set(6, 0);
                                c.put32(south_list + south_count * 4, candidate);
                                c.set(9, ++south_count);
                                c.put32(cursor, 0);
                            }
                        }
                    }
                    block(c, 0x1053a4c8, 3);
                    c.set(27, (i + 1) * 4);
                    c.set(4, cursor += 4);
                    set_ctr(selected_count - i - 1);
                }
            }
            block(c, 0x1053a4d4, 1);
            c.set(24, ++pass);
        }
        // Continued below: snapshot, assign and preserve previous pair/triple
        // cycles. Keeping this phase separate makes scratch ownership clear.
        split_region_assignments(c, frame, selected_count, north_count, south_count,
                                 north_definition, south_definition);
    }
    block(c, 0x1053a8ec, 5);
    c.set(0, c.u32(frame + 0x238));
    c.set(1, stack);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<24, true>(c, stack);
    c.finish();
}

void next_season(Cpu& c) {
    GuestNativeScope scope(c.s, next_season_entry);
    block(c, next_season_entry, 19);
    const auto stack = c.sp(), frame = stack - 0x100;
    c.set(0, c.lr());
    guest::transfer_saved_registers<25>(c, stack);
    const guest::Kept<28> regional_id(c, c.toc(-0x6750));
    const guest::Kept<25> object(c, c.r(3));
    const guest::Kept<26> divisions(c, c.toc(-0x7b5c));
    const guest::Kept<27> second_id(c, c.toc(-0x673c));
    const guest::Kept<29> failure_flag(c, c.toc(-0x7bac));
    const guest::Kept<30> buffer(c, c.toc(-0x7ba8));
    const guest::Kept<31> pool(c, c.toc(-0x44f8));
    c.put32(stack + 8, c.r(0));
    c.set(0, 0);
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.put8(object + 0x49, 0);
    c.set(0, c.u32(regional_id));
    c.set(4, c.u32(divisions));
    c.set(0, c.r(0) * 4);
    c.set(0, c.u32(c.r(4) + c.r(0)));
    const auto has_regional = compare_logical(c, c.r(0)) != 0;
    const auto owned = [&](std::uint32_t entry, std::uint32_t resume, auto implementation) {
        if (routine_checker) c.call(entry, resume);
        else {
            c.link(resume);
            c.s.pc = entry;
            implementation(c);
        }
    };
    const auto finish = [&] {
        block(c, 0x10539b98, 5);
        c.set(0, c.u32(frame + 0x108));
        c.set(1, stack);
        c.set_lr(c.r(0));
        guest::transfer_saved_registers<25, true>(c, stack);
        c.finish();
    };
    const auto fail = [&](std::uint32_t pc, unsigned message, unsigned detail, unsigned line) {
        report_failure(c, pc, pool, buffer, message, detail, line);
        block(c, pc + 72, 3);
        c.set(0, 0);
        c.put8(failure_flag, 0);
        finish();
    };
    if (has_regional) {
        block(c, 0x105398cc, 1);
        owned(reserves_entry, 0x105398d0, coordinate_reserves);
    }
    block(c, 0x105398d0, 5);
    c.set({{3, object}, {4, 1}});
    c.set(12, c.u32(object));
    c.set(12, c.u32(c.r(12) + 0xb8));
    call_virtual(c, 0x105398e4);
    block(c, 0x105398e4, 7);
    c.set(2, c.u32(frame + 0x14));
    c.set(0, c.u32(regional_id));
    c.set(3, c.u32(divisions));
    c.set(0, c.r(0) * 4);
    c.set(0, c.u32(c.r(3) + c.r(0)));
    if (compare_logical(c, c.r(0))) {
        block(c, 0x10539900, 2);
        c.set(3, object);
        owned(fill_entry, 0x10539908, fill_regional_slots);
        block(c, 0x10539908, 1);
    } else {
        block(c, 0x1053990c, 2);
        c.set(3, object);
        owned(promote_entry, 0x10539914, promote_four);
    }
    block(c, 0x10539914, 2);
    c.set(3, object);
    owned(split_entry, 0x1053991c, split_regions);
    block(c, 0x1053991c, 3);
    c.set({{3, object}, {4, 0}});
    c.call(0x104a6660, 0x10539928);
    block(c, 0x10539928, 3);
    c.set(3, c.u32(object + 0xbc));
    if (compare_logical(c, c.r(3))) {
        block(c, 0x10539934, 1);
        c.call(0x10775170, 0x10539938);
        block(c, 0x10539938, 2);
        c.set(0, 0);
        c.put32(object + 0xbc, 0);
    }
    block(c, 0x10539940, 3);
    c.set(3, c.u32(object + 8));
    if (compare_logical(c, c.r(3))) {
        block(c, 0x1053994c, 2);
        c.set(4, 1);
        c.call(0x104564a0, 0x10539954);
    }
    block(c, 0x10539954, 7);
    c.set(4, std::uint32_t(c.i16(object + 0x40)));
    c.set(3, object);
    c.set(0, c.r(4) + 1);
    c.put16(object + 0x40, c.r(0));
    c.set(12, c.u32(object));
    c.set(12, c.u32(c.r(12) + 0x94));
    call_virtual(c, 0x10539970);
    block(c, 0x10539970, 3);
    c.set(2, c.u32(frame + 0x14));
    if (!compare(c, std::int32_t(c.r(3)))) {
        fail(0x1053997c, 0xb8, 0x50, 0x2ec);
        return;
    }
    block(c, 0x105399d0, 2);
    c.set(3, object);
    owned(collect_entry, 0x105399d8, collect_clubs);
    block(c, 0x105399d8, 2);
    if (!compare(c, std::int32_t(c.r(3)))) {
        fail(0x105399e0, 0x98, 0x48, 0x2f3);
        return;
    }
    block(c, 0x10539a34, 2);
    c.set(3, object);
    c.call(0x104a00a0, 0x10539a3c);
    block(c, 0x10539a3c, 2);
    if (!compare(c, std::int32_t(c.r(3)))) {
        fail(0x10539a44, 0x78, 0x40, 0x2f9);
        return;
    }
    block(c, 0x10539a98, 3);
    c.set({{3, object}, {4, 0}});
    c.call(0x1049eb30, 0x10539aa4);
    block(c, 0x10539aa4, 2);
    if (!compare(c, std::int32_t(c.r(3)))) {
        fail(0x10539aac, 0x58, 0x38, 0x300);
        return;
    }
    block(c, 0x10539b00, 9);
    c.set(0, c.u32(second_id));
    c.set(4, 1);
    c.set(3, c.u32(divisions));
    c.set(5, 3);
    c.set(0, c.r(0) * 4);
    c.set(6, std::uint32_t(-3));
    c.set(3, c.u32(c.r(3) + c.r(0)));
    c.set(7, 1);
    c.call(0x104aa240, 0x10539b24);
    block(c, 0x10539b24, 7);
    c.set(0, c.u32(second_id));
    c.set(3, c.u32(divisions));
    c.set(0, c.r(0) * 4);
    c.set(3, c.u32(c.r(3) + c.r(0)));
    c.set(12, c.u32(c.r(3)));
    c.set(12, c.u32(c.r(12) + 0x10));
    call_virtual(c, 0x10539b40);
    block(c, 0x10539b40, 7);
    c.set(2, c.u32(frame + 0x14));
    c.set(0, c.u32(regional_id));
    c.set(3, c.u32(divisions));
    c.set(0, c.r(0) * 4);
    c.set(3, c.u32(c.r(3) + c.r(0)));
    if (compare_logical(c, c.r(3))) {
        block(c, 0x10539b5c, 3);
        c.set(12, c.u32(c.r(3)));
        c.set(12, c.u32(c.r(12) + 0x10));
        call_virtual(c, 0x10539b68);
        block(c, 0x10539b68, 1);
        c.set(2, c.u32(frame + 0x14));
    }
    block(c, 0x10539b6c, 4);
    c.set(3, object);
    c.set(12, c.u32(object));
    c.set(12, c.u32(c.r(12) + 0x64));
    call_virtual(c, 0x10539b7c);
    block(c, 0x10539b7c, 3);
    c.set(2, c.u32(frame + 0x14));
    c.set(3, object);
    c.call(0x104aa3a0, 0x10539b88);
    block(c, 0x10539b88, 4);
    c.set(3, c.toc(-0x7428));
    c.set(4, c.u32(object + 4));
    c.set(3, c.u32(c.r(3)));
    c.call(0x102d46c0, 0x10539b98);
    finish();
}

struct Routine { std::uint32_t entry; void (*run)(Cpu&); };
constexpr auto routines = std::to_array<Routine>({
#define GER_FIRST_ROUTINE(entry, function) Routine{entry, function},
#include "ger_first_routines.inc"
#undef GER_FIRST_ROUTINE
});

const Routine& find_routine(std::uint32_t entry) {
    const auto found = std::ranges::lower_bound(routines, entry, {}, &Routine::entry);
    if (found == routines.end() || found->entry != entry)
        throw std::logic_error("Unknown GER_FIRST.CPP entry " + hex_address(entry));
    return *found;
}

void call_virtual(Cpu& c, std::uint32_t resume) {
    if (routine_checker) { c.call(0x10781320, resume); return; }
    c.link(resume);
    c.s.pc = 0x10781320;
    if (try_modern_import_descriptor(c.s, 0x10781320)) {
        const auto target = std::ranges::lower_bound(routines, c.s.pc, {}, &Routine::entry);
        if (target != routines.end() && target->entry == c.s.pc) target->run(c);
    }
    if (c.s.pc != resume) run_until(c.s, resume);
}
}

void ger_first_run_original(State& s, std::uint32_t entry) { run_original_body(s, entry, declining); }
void ger_first_run_rewrite(State& s, std::uint32_t entry) { Cpu c(s); find_routine(entry).run(c); }

bool try_modern_ger_first(State& s, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(s, entry, ger_first_run_original, ger_first_run_rewrite, RoutineChecker::result_r3);
    ger_first_run_rewrite(s, entry);
    return true;
}
}
