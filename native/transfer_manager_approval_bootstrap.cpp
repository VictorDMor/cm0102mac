#include "transfer_manager_approval_bootstrap.hpp"
#include "transfer_manager_adapter.hpp"
#include "transfer_manager_approval.hpp"
#include "transfer_manager_permissions.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr std::uint32_t default_date = 0x10715880, shift_date = 0x10717a10,
    career_table = 0x10305c60, release_history = 0x10775170, sort_rows = 0x10768300,
    regional_check = 0x103ecef0, append_approval = 0x103ec940, staff_size = 0x70, club_size = 0x248;

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

bool needs_approval(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club) {
    constexpr auto back = 0x103ec778;
    c.set({{3, manager}, {4, person}, {5, club}, {6, 0}});
    if (transfer_manager_direct_calls_enabled()) {
        c.link(back);
        c.s.pc = regional_check;
        requires_regional_transfer_approval(c, manager, person, club, 0);
        c.finish();
    } else {
        c.call(regional_check, back);
    }
    const auto approval = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(approval));
    return nonzero(c, approval);
}

void publish_approval(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t nation,
                      std::uint32_t year) {
    constexpr auto back = 0x103ec8fc;
    c.set({{3, manager}, {4, person}, {5, nation}, {6, year}, {7, 1}, {8, 0}});
    if (transfer_manager_direct_calls_enabled()) {
        c.link(back);
        c.s.pc = append_approval;
        append_regional_approval(c, manager, person, nation, year, 0);
        c.finish();
    } else {
        c.call(append_approval, back);
    }
}

void find_residency_start(Cpu& c, Kept<22>& year, std::uint32_t person, std::uint32_t history,
                          std::uint32_t slots, std::uint32_t clubs, std::uint32_t club_count) {
    constexpr std::uint32_t group_size = 0x120, slots_per_group = 6;
    auto remaining = (slots + slots_per_group - 1) / slots_per_group;
    std::uint32_t offset = 0;
    c.set({{0, slots_per_group}, {4, remaining}, {5, slots}, {6, clubs}, {7, club_count}, {8, offset}});
    c.s.wr<4>(reg::ctr, remaining);
    if (compare(c, slots) <= 0) return;
    do {
        const auto group = history + offset;
        const auto club_id = c.i32(group + 4);
        c.set({{9, group}, {0, std::uint32_t(club_id)}});
        if (compare(c, club_id) >= 0 && compare(c, club_id, club_count) < 0) {
            const auto club_offset = std::uint32_t(club_id) * club_size;
            const auto current_nation = c.u32(c.u32(person + 0x24) + 0x2c);
            const auto previous_nation = c.u32(clubs + club_offset + 0x2c);
            c.set({{5, club_offset}, {0, current_nation}, {4, previous_nation}});
            if (compare_logical(c, previous_nation, current_nation) != 0) return;
            const auto candidate_year = c.u16(group);
            const auto previous_year = std::uint16_t(std::uint32_t(year));
            c.set({{4, candidate_year}, {0, previous_year}});
            if (compare_logical(c, candidate_year, previous_year) < 0) year = candidate_year;
        }
        offset += group_size;
        c.set(8, offset);
        c.s.wr<4>(reg::ctr, --remaining);
    } while (remaining);
}

void copy_joined_date(Cpu& c, std::uint32_t out, std::uint32_t staff) {
    c.put16(out + 2, c.u16(staff + 0x4e));
    c.put16(out, c.u16(staff + 0x4c));
    const auto leap = c.u8(staff + 0x50);
    c.put8(out + 4, leap);
    c.set(0, leap);
}

bool precedes_start(Cpu& c, std::uint32_t joined, std::uint32_t start, std::uint32_t saved) {
    const auto first = c.u32(start), second = c.u32(start + 4);
    c.put32(saved, first);
    const auto joined_year = c.u16(joined + 2), start_year = c.u16(saved + 2);
    c.put32(saved + 4, second);
    c.set({{0, second}, {3, joined_year}, {4, start_year}});
    const auto order = compare_logical(c, start_year, joined_year);
    bool before = order > 0;
    if (order == 0) {
        // The signed day comparison leaves its half-word calculation and
        // shift carry visible to the next guest call.
        const auto start_day = c.i16(saved), joined_day = c.i16(joined);
        const auto difference = std::int32_t(start_day ^ joined_day);
        const auto half = difference >> 1;
        const auto bits = std::uint32_t(difference);
        c.s.wr<1>(reg::xer_ca, (bits >> 31) & bits);
        c.set({{3, std::uint32_t(half)}, {4, std::uint32_t(start_day)}});
        before = half - (difference & start_day) < 0;
    }
    c.set(0, before);
    return nonzero(c, before);
}
}

void initialize_staff_approvals(Cpu& c, std::uint32_t manager) {
    Frame frame(c, 0x103ec6d0, 0x90, {.first_gpr = 21});
    c.set(0, c.lr());
    const Kept<21> transfers(c, manager);
    const Kept<26> start(c, c.toc(-0x7a9c));
    const Kept<27> statistics(c, c.toc(-0x7b3c));
    const Kept<28> today(c, c.toc(-0x7b80));
    const Kept<29> clubs(c, c.toc(-0x7be8));
    const Kept<30> club_count(c, c.toc(-0x7b50));
    const Kept<31> staff_count(c, c.toc(-0x7be4));
    const auto joined = frame.at(0x4c);
    call(c, default_date, 0x103ec704, {joined});
    const auto people = c.toc(-0x7be0);
    c.set(3, people);
    Kept<24> index(c, 0);
    Kept<25> status_offset(c, 0);
    Kept<23> person(c, c.u32(people));
    for (;; index = index + 1, status_offset = status_offset + 10, person = person + staff_size) {
        const auto count = c.u32(staff_count);
        c.set(0, count);
        if (compare(c, index, count) >= 0) break;
        const auto roles = c.u8(person + 0x5e);
        c.set(0, roles);
        const auto high_band = compare_logical(c, roles, 13) > 0;
        if (high_band || compare_logical(c, roles, 5) > 0) {
            const auto mask = high_band ? 0x3fffu : 0x1fffu;
            const auto status = c.u32(transfers + 0x9a0);
            const auto field = status_offset + 4;
            c.set({{0, field}, {3, status}, {4, mask}});
            c.put16(status + field, mask);
        }
        const auto club = c.u32(person + 0x24);
        c.set(5, club);
        if (!nonzero_logical(c, club)) continue;
        const auto role = c.u8(person + 0x62);
        c.set(0, role);
        if (compare(c, role, 11) != 0 || !needs_approval(c, transfers, person, club)) continue;
        Kept<22> year(c, c.u16(person + 0x4e));
        const auto slot_count = frame.at(0x38);
        const auto history = call(c, career_table, 0x103ec794, {c.u32(statistics), person, slot_count});
        find_residency_start(c, year, person, history, c.u32(slot_count), c.u32(clubs), c.u32(club_count));
        if (nonzero_logical(c, history)) call(c, release_history, 0x103ec818);
        const auto earliest = std::uint16_t(std::uint32_t(year));
        const auto elapsed = std::int32_t(c.u16(today + 2)) - earliest;
        c.set({{0, std::uint32_t(elapsed)}, {3, earliest}});
        if (compare(c, elapsed, 5) >= 0) {
            const auto current_club = c.u32(person + 0x24);
            const auto nation = c.u32(current_club + 0x2c);
            c.set({{0, nation}, {3, current_club}});
            c.put32(person + 0x10, nation);
            continue;
        }
        copy_joined_date(c, joined, person);
        const auto shifted = frame.at(0x44);
        while (precedes_start(c, joined, start, frame.at(0x3c))) {
            call(c, shift_date, 0x103ec868, {shifted, joined, 52});
            const auto leap = c.u8(shifted + 4);
            const auto next_year = c.u16(shifted + 2);
            const auto day = c.i16(shifted);
            c.set({{0, leap}, {3, std::uint32_t(day)}, {4, next_year}});
            c.put16(joined + 2, next_year);
            c.put16(joined, std::uint32_t(day));
            c.put8(joined + 4, leap);
        }
        publish_approval(c, transfers, person, c.u32(c.u32(person + 0x24) + 0x2c), year);
    }
    call(c, sort_rows, 0x103ec928, {c.u32(transfers + 0xa6c), c.u32(transfers + 0xa70), 16, c.toc(-0x4ba8)});
}
}
