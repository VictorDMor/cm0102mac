#include "ger_first_adapter.hpp"
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

bool declining = std::getenv("CM_GER_FIRST_ORIGINAL") != nullptr;
constexpr auto collect_entry = 0x10539590u;
constexpr auto calendar_entry = 0x10537120u, configure_entry = 0x105397b0u;
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
