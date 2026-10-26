#include "shortlist_manager_squads_detail.hpp"

// 0x1035a9f0: the staff places (0x1035de78-0x1035e9d8). The club's staff
// not yet linked to it (its assistant +0x94, then +0x160[5], +0x190[3],
// +0x174[7]) are listed; each open staff place takes the first listed
// person of its job, who holds it when rated at its budget, else stays the
// choice (or, on a refresh, is dropped when he has no contract).
//
// This is the routine's last phase: what it leaves in r3-r9 is what the
// routine returns with, so the volatile registers are set as the original
// sets them.
namespace cm::aot::shortlist_manager::squads {
namespace {
// Lists the club's staff at `field` (`count` words) that 0x1036d240 does
// not link yet; r15 counts the list.
void list_staff(Assessment& a, std::uint32_t field, std::int32_t count, std::uint32_t back) {
    Cpu& c = a.c;
    const auto club = a.club();
    Kept<17> i{c, 0};
    keep(c, {{13, a.at(local::staff)}});
    Kept<14> offset{c, 0};
    for (; byte_of(i) < count; offset = offset + 4, i = i + 1) {
        const Kept<16> at{c, offset + field};
        const auto person = c.u32(club + at);
        c.set(5, person);
        if (!nonzero_logical(c, person)) continue;
        if (nonzero_logical(c, a.program.call(call::club_link_row, back, {a.manager, club, person, 1, 0}))) continue;
        const auto staff = c.u32(club + at);
        c.set(3, staff);
        c.put32(c.r(13) + std::uint32_t(byte_of(c.r(15))) * 4, staff);
        keep(c, {{15, c.r(15) + 1}});
    }
}

// The original's call sites for one staff job.
struct JobSite {
    std::int8_t job, other;  // the person's job byte that fits (other: -1 none)
    bool unsigned_job;       // jobs 9 and 10 compare the byte unsigned
    std::uint32_t human_back, rating_human_back, rating_back, contract_back, drop_back;
};
constexpr JobSite job_sites[] = {
    {6, 0xd, false, 0x1035e14c, 0x1035e174, 0x1035e19c, 0x1035e1c8, 0x1035e200},
    {7, 0xe, false, 0x1035e318, 0x1035e340, 0x1035e368, 0x1035e394, 0x1035e3cc},
    {8, 0xf, false, 0x1035e4e4, 0x1035e50c, 0x1035e534, 0x1035e560, 0x1035e598},
    {9, -1, true, 0x1035e6a0, 0x1035e6c8, 0x1035e6f0, 0x1035e71c, 0x1035e754},
    {10, -1, true, 0x1035e85c, 0x1035e884, 0x1035e8ac, 0x1035e8d8, 0x1035e910},
};

// The place at r18 (`place`, r8) takes listed person `i` (r16 = i * 4,
// r17 the list).
void fill(Assessment& a, const JobSite& site, std::uint32_t place, std::int32_t i) {
    Cpu& c = a.c;
    const auto club = a.club();
    const auto list = a.at(local::staff);
    const Kept<17> base{c, list};
    const Kept<16> at{c, std::uint32_t(i) * 4};
    const auto person = [&] { return c.u32(base + at); };
    const std::int32_t budget = std::int32_t(c.r(14));
    if (site.unsigned_job) c.set(9, person());
    else c.set(7, person());
    Row{c, place}.previous() = c.i32(person());
    const bool human = nonzero(c, std::int8_t(a.program.call(call::human_club, site.human_back, {club, 1, 1, 0, 0})));
    const auto staff = [&] { return c.u32(c.toc(toc::staff)); };
    const std::uint32_t kind = c.u8(a.slots() + c.r(18));
    const auto rated = human ? a.program.call(call::staff_rating, site.rating_human_back, {staff(), person(), club, a.human_manager(), kind})
                             : a.program.call(call::staff_rating, site.rating_back, {staff(), person(), club, a.club_manager(), kind});
    const Kept<21> rating{c, rated};
    const Row slot{c, a.slots() + c.r(18)};
    auto short_of_budget = [&] {
        // Held open: the shortfall, scaled by 2.5, is the priority.
        const std::uint32_t slots = a.slots();
        const double gap = a.integer(0x99c, std::int32_t(std::uint32_t(budget) - rating));
        c.set(5, 0xffffffff);
        slot.target() = -1;
        const std::int32_t priority = a.truncated(0x9a4, a.fpu.fmul(a.constant(0xe0), gap));
        c.set_f(1, a.constant(0xe0));  // the original loads the constant into f1 (lfd f1)
        slot.weight() = priority;
        leave(c, {{3, slots}, {4, std::uint32_t(priority)}});
    };
    if (compare(c, std::int32_t(std::uint32_t(rating)), budget) >= 0) {
        const auto id = c.u32(person());
        slot.target() = std::int32_t(id);
        slot.weight() = 0;
        leave(c, {{3, a.slots()}, {4, 0}, {5, a.slots()}, {6, id}});
    } else if (!nonzero(c, a.local8(local::refreshed))) {
        short_of_budget();
    } else if (nonzero_logical(c, a.program.call(call::contract_by_id, site.contract_back, {a.contracts(), c.u32(person()), c.u32(club)}))) {
        short_of_budget();
    } else {
        slot.previous() = -1;
        slot.target() = -1;
        c.set(6, 0xffffffff);
        a.program.call(call::drop_staff, site.drop_back, {staff(), person(), club});
    }
    // The person leaves the list: the last one takes his place.
    const std::int32_t count = byte_of(c.r(15));
    keep(c, {{15, c.r(15) - 1}});
    c.put32(base + at, c.u32(list + std::uint32_t(count) * 4 - 4));
    leave(c, {{3, list}, {4, std::uint32_t(count) * 4}});
}

// Looks through the list for the place's job (`place` in r8); r3 the
// index, r4 the cursor, r6 the byte offset, as the original leaves them.
void staff_place(Assessment& a, const JobSite& site, std::uint32_t place) {
    Cpu& c = a.c;
    const auto list = a.at(local::staff);
    if (site.unsigned_job) keep(c, {{17, list}});
    std::uint32_t i = 0;
    const std::int32_t count = byte_of(c.r(15));
    leave(c, {{6, 0}, {4, list}});
    for (;; ++i) {
        c.set(5, std::uint32_t(byte_of(i)));
        if (compare(c, byte_of(i), count) >= 0) break;
        const auto person = c.u32(list + i * 4);
        const std::int32_t job = site.unsigned_job ? std::int32_t(c.u8(person + 0x62)) : std::int32_t(c.i8(person + 0x62));
        c.set(5, std::uint32_t(job));
        if (compare(c, job, site.job) == 0 || (site.other >= 0 && compare(c, job, site.other) == 0)) {
            c.set(3, i);
            return fill(a, site, place, byte_of(i));
        }
        leave(c, {{6, (i + 1) * 4}, {4, list + (i + 1) * 4}, {3, i + 1}});
    }
}
}

void assign_staff(Assessment& a) {
    Cpu& c = a.c;
    const auto club = a.club();
    keep(c, {{15, 0}});
    const auto assistant = c.u32(club + 0x94);
    c.set(5, assistant);
    if (nonzero_logical(c, assistant) &&
        !nonzero_logical(c, a.program.call(call::club_link_row, 0x1035de9c, {a.manager, club, assistant, 1, 0}))) {
        const auto again = c.u32(club + 0x94);
        c.put32(a.at(local::staff), again);
        leave(c, {{3, a.at(local::staff)}, {4, again}});
        keep(c, {{15, 1}});
    }
    list_staff(a, 0x160, 5, 0x1035def0);
    list_staff(a, 0x190, 3, 0x1035df54);
    list_staff(a, 0x174, 7, 0x1035dfb8);
    // The staff budget: a fifth of the club's value, at most 5000.
    const std::int32_t value = a.local32(local::value);
    const std::int32_t fifth = value / 5;
    leave(c, {{3, std::uint32_t(multiply_high(0x66666667, value) >> 1) >> 31}, {4, std::uint32_t(multiply_high(0x66666667, value))}});
    const Kept<14> budget{c, std::uint32_t(fifth < 5000 ? fifth : 5000)};
    Kept<13> index{c, 0};
    Kept<18> offset{c, 0};
    for (; compare(c, byte_of(index), slot_count) < 0; offset = offset + Row::size, index = index + 1) {
        const auto place = a.slots() + offset;
        c.set(6, place);
        const Row slot{c, place};
        if (compare(c, c.u8(place), Row::player) == 0) continue;
        slot.flags() = slot.flags() & 0x7f;
        slot.budget() = 0;
        const std::int32_t priority = a.truncated(0x9a4, a.fpu.fmul(a.constant(0xe0), a.integer(0x99c, std::int32_t(std::uint32_t(budget)))));
        c.set_f(1, a.constant(8));  // the original loads the bias into f1 (lfd f1)
        Row{c, a.slots() + offset}.weight() = priority;
        leave(c, {{3, 0}, {5, std::uint32_t(priority)}, {4, place}});
        if (compare(c, Row{c, a.slots() + offset}.approach(), -1) != 0) continue;
        Row{c, a.slots() + offset}.level() = std::int32_t(std::uint32_t(budget));
        const auto at = a.slots() + offset;
        c.set(8, at);
        const std::int32_t kind = c.i8(at);
        const JobSite* site = nullptr;
        for (const auto& s : job_sites)
            if (s.job == kind) site = &s;
        if (site) staff_place(a, *site, at);
    }
}
}
