#include "national_teams_adapter.hpp"
#include "guest_abi.hpp"
#include "guest_saved_registers.hpp"
#include "routine_check.hpp"

#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
bool declining = std::getenv("CM_NATIONAL_TEAMS_ORIGINAL") != nullptr;
constexpr auto selection_context_entry = 0x102ae9c0u;
constexpr auto selection_flag_entry = 0x102aea20u, clear_selection_flag_entry = 0x102aea50u;
constexpr auto selection_value_entry = 0x102af620u, selection_slot_entry = 0x102af650u;
constexpr auto assigned_staff_entry = 0x102b7450u;
constexpr unsigned nation_record_size = 0x74, selection_slot_size = 0x14, staff_record_size = 0x6c;
constexpr unsigned nation_index_offset = 0x68, selection_slots_offset = 0x40;
constexpr unsigned selection_value_offset = 0xa, selection_flag_offset = 0xb;
constexpr int unassigned = -1;

void block(Cpu& c, std::uint32_t pc, unsigned count) {
    c.s.step(pc);
    c.s.charge(count - 1);
}

bool record_nonzero_byte(Cpu& c, std::uint32_t raw) {
    const auto value = std::int8_t(raw);
    c.set(0, std::uint32_t(value));
    const auto high = c.cr(0) & 0xf0;
    c.compare(0, value, 0);
    c.set_cr(0, high | c.cr(0));
    return value != 0;
}

void selection_context(Cpu& c) {
    GuestNativeScope scope(c.s, selection_context_entry);
    block(c, selection_context_entry, 7);
    const auto caller = c.sp(), frame = caller - 0x40, mode = c.r(5);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set({{31, mode}, {3, c.r(4)}});
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    c.call(0x100ce250, 0x102ae9dc);

    block(c, 0x102ae9dc, 2);
    const bool national = record_nonzero_byte(c, c.r(3));
    bool alternate = true;
    if (!national) {
        block(c, 0x102ae9e4, 2);
        alternate = record_nonzero_byte(c, mode);
    }
    if (national || alternate) {
        block(c, 0x102ae9f4, 2);
        if (record_nonzero_byte(c, mode)) {
            block(c, 0x102ae9fc, 2);
            c.set(3, 1);
        } else {
            block(c, 0x102aea04, 1);
            c.set(3, 2);
        }
    } else {
        block(c, 0x102ae9ec, 2);
        c.set(3, 0);
    }

    block(c, 0x102aea08, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.finish();
}

enum class SelectionAccess { flag, clear_flag, value, address };

template<SelectionAccess Access>
void selection_slot(Cpu& c) {
    constexpr auto entry = Access == SelectionAccess::flag ? selection_flag_entry :
        Access == SelectionAccess::clear_flag ? clear_selection_flag_entry :
        Access == SelectionAccess::value ? selection_value_entry : selection_slot_entry;
    block(c, entry, Access == SelectionAccess::clear_flag ? 10 : 9);
    const auto nation = c.u32(c.r(4) + nation_index_offset);
    c.set(4, nation);
    const auto displacement = std::uint32_t(std::int8_t(c.r(5))) * selection_slot_size;
    c.set(0, displacement);
    const auto records = c.u32(c.r(3) + 4);
    c.set(5, records);
    const auto nation_offset = nation * nation_record_size;
    if constexpr (Access == SelectionAccess::address) {
        c.set(3, records + nation_offset + displacement + selection_slots_offset);
    } else {
        const auto slot = records + nation_offset + displacement;
        c.set(3, slot);
        if constexpr (Access == SelectionAccess::clear_flag) {
            c.set(6, 0);
            c.put8(slot + selection_slots_offset + selection_flag_offset, 0);
        } else {
            constexpr auto field = Access == SelectionAccess::flag ? selection_flag_offset : selection_value_offset;
            c.set(3, c.u8(slot + selection_slots_offset + field));
        }
    }
    c.finish();
}

void assigned_staff(Cpu& c) {
    block(c, assigned_staff_entry, 2);
    const auto nation = c.r(4);
    c.compare_logical(0, nation, 0);
    if (nation) {
        block(c, 0x102b7458, 7);
        const auto nation_id = c.u32(nation + nation_index_offset);
        c.set(0, nation_id);
        const auto records = c.u32(c.r(3) + 4);
        c.set(3, records);
        const auto nation_offset = nation_id * nation_record_size;
        c.set(0, nation_offset);
        c.set(3, records + nation_offset);
        const auto staff = c.u32(records + nation_offset + 4);
        c.set(0, staff);
        c.compare(0, std::int32_t(staff), unassigned);
        if (std::int32_t(staff) > unassigned) {
            block(c, 0x102b7474, 5);
            const auto slot = c.toc(-0x7bbc);
            c.set(3, slot);
            c.set(0, staff * staff_record_size);
            c.set(3, c.u32(slot) + c.r(0));
            c.finish();
            return;
        }
    }
    block(c, 0x102b7488, 2);
    c.set(3, 0);
    c.finish();
}
constexpr auto international_club_entry = 0x102b7c00u, duty_conflict_entry = 0x102b7c70u;
constexpr auto classify_club = 0x100c7d00u;
constexpr int first_all_star_id = -0x7208, second_all_star_id = -0x7204;
constexpr unsigned club_size = 0x248, club_roster = 0x98, player_national_team = 0x20;

void international_club(Cpu& c) {
    GuestNativeScope scope(c.s, international_club_entry);
    block(c, international_club_entry, 7);
    const auto caller = c.sp(), club = c.r(4), frame = caller - 0x40;
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set(31, club);
    c.put32(caller + 8, c.r(0));
    c.set(3, club);
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    c.call(classify_club, 0x102b7c1c);
    block(c, 0x102b7c1c, 2);
    if (record_nonzero_byte(c, c.r(3))) {
        block(c, 0x102b7c24, 2);
        c.set(3, 1);
    } else {
        block(c, 0x102b7c2c, 5);
        c.set(3, c.toc(second_all_star_id));
        const auto id = c.u32(club);
        c.set(4, id);
        auto reserved_id = c.u32(c.r(3));
        c.set(0, reserved_id);
        c.compare(0, id, reserved_id);
        if (id != reserved_id) {
            block(c, 0x102b7c40, 4);
            c.set(3, c.toc(first_all_star_id));
            reserved_id = c.u32(c.r(3));
            c.set(0, reserved_id);
            c.compare(0, id, reserved_id);
        }
        const bool reserved = id == reserved_id;
        block(c, reserved ? 0x102b7c50 : 0x102b7c58, reserved ? 2 : 1);
        c.set(3, reserved);
    }
    block(c, 0x102b7c5c, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.finish();
}

struct DutyQuery {
    Cpu& c;
    std::uint32_t frame, manager, player, date, requested_club, label, competition;
    std::uint32_t first_star, second_star, shared_label, clubs, source;
    std::uint32_t list() const { return frame + 0x80; }
};

enum class DutyLabel : unsigned { fixture = 0x7c62, roster = 0x7c8d, all_star = 0x7cb7 };

void label_duty(const DutyQuery& q, std::uint32_t pc, DutyLabel label) {
    auto& c = q.c;
    block(c, pc, 4);
    const auto output = label == DutyLabel::all_star ? q.shared_label : q.label;
    c.set({{3, output}, {4, q.source + unsigned(label)}, {5, q.source + 0x7c8c}});
    c.call(0x10727890, pc + 16);
}

std::uint32_t end_duty(const DutyQuery& q, std::uint32_t pc, bool team) {
    auto& c = q.c;
    block(c, pc, team ? 4 : 3);
    if (team) c.set(21, c.u32(q.player + player_national_team));
    c.set({{3, q.list()}, {4, UINT32_MAX}});
    const auto resume = pc + (team ? 16 : 12);
    c.call(0x10738c20, resume);
    block(c, resume, resume == 0x102b8160 ? 1 : 2);
    return team ? c.r(21) : 0;
}

enum class RequestDisposition { eligible, national, all_star };

RequestDisposition ignored_request(const DutyQuery& q) {
    auto& c = q.c;
    block(c, 0x102b7cb4, 2);
    c.compare_logical(0, q.requested_club, 0);
    if (q.requested_club) {
        block(c, 0x102b7cbc, 2);
        c.set(3, q.requested_club);
        c.call(classify_club, 0x102b7cc4);
        block(c, 0x102b7cc4, 2);
        if (record_nonzero_byte(c, c.r(3))) return RequestDisposition::national;
    }
    block(c, 0x102b7ce0, 2);
    c.compare_logical(0, q.requested_club, 0);
    if (!q.requested_club) return RequestDisposition::eligible;
    block(c, 0x102b7ce8, 4);
    const auto id = c.u32(q.requested_club);
    c.set(3, id);
    auto reserved = c.u32(q.second_star);
    c.set(0, reserved);
    c.compare(0, id, reserved);
    if (id != reserved) {
        block(c, 0x102b7cf8, 3);
        reserved = c.u32(q.first_star);
        c.set(0, reserved);
        c.compare(0, id, reserved);
    }
    return id == reserved ? RequestDisposition::all_star : RequestDisposition::eligible;
}

std::uint32_t build_duty_window(const DutyQuery& q) {
    auto& c = q.c;
    const auto frame = q.frame;
    block(c, 0x102b7d18, 4);
    c.set({{4, q.date}, {3, frame + 0x60}, {5, 1}});
    c.call(0x10717b20, 0x102b7d28);
    block(c, 0x102b7d28, 10);
    c.set(5, std::uint32_t(c.i16(frame + 0x60)));
    c.set(4, q.date);
    c.set(6, c.u16(frame + 0x62));
    c.set(3, frame + 0x58);
    c.set(0, c.u8(frame + 0x64));
    c.put16(frame + 0x78, c.r(5));
    c.set(5, 1);
    c.put16(frame + 0x7a, c.r(6));
    c.put8(frame + 0x7c, c.r(0));
    c.call(0x10717890, 0x102b7d50);

    block(c, 0x102b7d50, 9);
    const auto national_team = c.u32(q.player + player_national_team);
    c.set(8, national_team);
    c.set(4, std::uint32_t(c.i16(frame + 0x58)));
    c.set(3, c.u16(frame + 0x5a));
    c.compare_logical(0, national_team, 0);
    c.set(0, c.u8(frame + 0x5c));
    c.put16(frame + 0x70, c.r(4));
    c.put16(frame + 0x72, c.r(3));
    c.put8(frame + 0x74, c.r(0));
    return national_team;
}

bool list_next(const DutyQuery& q, std::uint32_t pc, std::uint32_t list) {
    auto& c = q.c;
    block(c, pc, 2);
    c.set(3, list);
    c.call(0x10738e00, pc + 8);
    block(c, pc + 8, 2);
    c.compare_logical(0, c.r(3), 0);
    return c.r(3) != 0;
}

bool fixture_conflict(const DutyQuery& q, std::uint32_t national_team) {
    auto& c = q.c;
    const auto frame = q.frame;
    block(c, 0x102b7d74, 23);
    const auto flags = c.u8(q.player + 0x6a);
    c.set({{0, flags}, {4, std::uint32_t(-2)}});
    const auto lower_day = std::uint32_t(c.i16(frame + 0x78));
    c.set({{3, lower_day}, {9, q.list()}, {5, std::uint32_t(std::int8_t(flags))}});
    c.put32(frame + 0x38, c.r(4));
    const bool reserve_team = (flags & 4) != 0;
    c.set(5, reserve_team);
    const auto fixture_slot = c.toc(-0x7a60);
    const unsigned check_roster = !reserve_team;
    c.set({{11, fixture_slot}, {26, check_roster}});
    const auto upper_day = std::uint32_t(c.i16(frame + 0x70));
    c.set({{0, upper_day}, {4, check_roster}});
    c.set(5, c.u16(frame + 0x7a));
    c.set(4, check_roster ? 31 : 32);
    c.set(7, c.u16(frame + 0x72));
    c.set({{4, reserve_team}, {10, UINT32_MAX}, {6, reserve_team}});
    c.put32(frame + 0x3c, c.r(6));
    c.set({{4, std::uint16_t(lower_day)}, {6, std::uint16_t(upper_day)}});
    c.set(3, c.u32(fixture_slot));
    c.set(8, c.u32(national_team));
    c.call(0x1048b180, 0x102b7dd0);

    block(c, 0x102b7dd0, 2);
    const auto count = std::uint16_t(c.r(3));
    c.set(0, count);
    const auto high = c.cr(0) & 0xf0;
    c.compare(0, count, 0);
    c.set_cr(0, high | c.cr(0));
    if (!count) return false;
    block(c, 0x102b7dd8, 2);
    c.compare_logical(0, q.competition, 0);
    if (!q.competition) {
        block(c, 0x102b7e40, 2);
        c.compare_logical(0, q.label, 0);
        if (q.label) label_duty(q, 0x102b7e48, DutyLabel::fixture);
        c.set(3, end_duty(q, 0x102b7e58, true));
        return true;
    }
    block(c, 0x102b7de0, 3);
    c.set(0, c.u32(q.list() + 4));
    c.put32(q.list() + 12, c.r(0));
    block(c, 0x102b7e28, 1);
    while (list_next(q, 0x102b7e2c, q.list())) {
        block(c, 0x102b7dec, 3);
        const auto competition = c.u32(c.r(3) + 0x14);
        c.set(0, competition);
        c.compare_logical(0, competition, q.competition);
        if (competition != q.competition) continue;
        block(c, 0x102b7df8, 2);
        c.compare_logical(0, q.label, 0);
        if (q.label) label_duty(q, 0x102b7e00, DutyLabel::fixture);
        c.set(3, end_duty(q, 0x102b7e10, true));
        return true;
    }
    block(c, 0x102b7e3c, 1);
    return false;
}

void publish_day_order(Cpu& c, std::int16_t earlier, std::int16_t later) {
    // The original's signed-day comparison leaves the xor half and carry
    // live, even though the actual decision is an ordinary signed comparison.
    const auto bits = std::int32_t(earlier) ^ std::int32_t(later);
    c.set(3, std::uint32_t(bits >> 1));
    c.s.wr<1>(guest::reg::xer_ca, bits < 0 && (bits & 1));
    c.set(0, earlier < later);
}

bool roster_date_precedes_query(const DutyQuery& q, std::uint32_t roster) {
    auto& c = q.c;
    const auto frame = q.frame;
    block(c, 0x102b7ebc, 8);
    c.set(4, c.u32(roster + 8));
    c.set(0, c.u32(roster + 12));
    c.put32(frame + 0x50, c.r(4));
    const auto date_year = c.u16(q.date + 2);
    c.set(3, date_year);
    const auto roster_year = c.u16(frame + 0x52);
    c.set(4, roster_year);
    c.put32(frame + 0x54, c.r(0));
    c.compare_logical(0, roster_year, date_year);
    if (roster_year < date_year) {
        block(c, 0x102b7edc, 2);
        c.set(0, 1);
    } else {
        block(c, 0x102b7ee4, 1);
        if (roster_year > date_year) {
            block(c, 0x102b7ee8, 2);
            c.set(0, 0);
        } else {
            block(c, 0x102b7ef0, 8);
            const auto roster_day = c.i16(frame + 0x50);
            c.set(0, std::uint32_t(roster_day));
            const auto date_day = c.i16(q.date);
            c.set(4, std::uint32_t(date_day));
            publish_day_order(c, roster_day, date_day);
        }
    }
    block(c, 0x102b7f10, 2);
    return record_nonzero_byte(c, c.r(0));
}

bool roster_conflict(const DutyQuery& q) {
    auto& c = q.c;
    block(c, 0x102b7e70, 3);
    const auto roster_list = c.u32(q.manager);
    c.set(3, roster_list);
    c.compare_logical(0, roster_list, 0);
    if (!roster_list) return false;
    block(c, 0x102b7e7c, 2);
    if (record_nonzero_byte(c, c.r(26))) return false;
    block(c, 0x102b7e84, 3);
    c.set(0, c.u32(roster_list + 4));
    c.put32(roster_list + 12, c.r(0));
    while (true) {
        block(c, 0x102b7f48, 2);
        c.set(3, c.u32(q.manager));
        c.call(0x10738e00, 0x102b7f50);
        block(c, 0x102b7f50, 2);
        const auto roster = c.r(3);
        c.compare_logical(0, roster, 0);
        if (!roster) return false;

        block(c, 0x102b7e90, 5);
        const auto team = c.u32(q.player + player_national_team);
        c.set(4, team);
        const auto roster_id = c.u32(roster);
        c.set(5, roster_id);
        const auto team_id = c.u32(team);
        c.set(0, team_id);
        c.compare(0, roster_id, team_id);
        if (roster_id != team_id) continue;
        block(c, 0x102b7ea4, 2);
        c.compare_logical(0, q.competition, 0);
        if (q.competition) {
            block(c, 0x102b7eac, 4);
            const auto roster_competition = c.u32(roster + 4);
            c.set(4, roster_competition);
            const auto requested = c.u32(q.competition);
            c.set(0, requested);
            c.compare(0, roster_competition, requested);
            if (roster_competition != requested) continue;
        }
        if (!roster_date_precedes_query(q, roster)) continue;
        block(c, 0x102b7f18, 2);
        c.compare_logical(0, q.label, 0);
        if (q.label) label_duty(q, 0x102b7f20, DutyLabel::roster);
        c.set(3, end_duty(q, 0x102b7f30, true));
        return true;
    }
}

bool all_star_window(const DutyQuery& q) {
    auto& c = q.c;
    const auto frame = q.frame;
    block(c, 0x102b7f58, 8);
    const auto calendar_index = c.toc(-0x6868);
    c.set(3, calendar_index);
    const auto calendars = c.toc(-0x7b5c);
    c.set(4, calendars);
    c.set(0, c.u32(calendar_index));
    c.set(3, c.u32(calendars));
    c.set(0, c.r(0) * 4);
    const auto calendar = c.u32(c.r(3) + c.r(0));
    c.set(4, calendar);
    c.compare_logical(0, calendar, 0);
    if (!calendar) return false;
    block(c, 0x102b7f78, 9);
    const auto calendar_date = c.u32(calendar + 0xa0);
    c.set({{6, calendar_date}, {3, frame + 0x68}});
    const auto year = c.i16(calendar + 0x40);
    c.set(0, std::uint32_t(year));
    const auto year_delta = c.i8(calendar_date + 6);
    c.set(5, std::uint8_t(year_delta));
    c.set(4, std::uint32_t(c.i16(calendar_date + 4)));
    c.set({{5, std::uint32_t(year_delta)}, {0, std::uint32_t(year + year_delta)}, {5, std::uint16_t(year + year_delta)}});
    c.call(0x107156b0, 0x102b7f9c);

    block(c, 0x102b7f9c, 3);
    const auto club = c.u32(q.player + 0x24);
    c.set(3, club);
    c.compare_logical(0, club, 0);
    if (!club) return false;
    block(c, 0x102b7fa8, 3);
    const auto nation = c.u32(club + 0x30);
    c.set(4, nation);
    c.compare_logical(0, nation, 0);
    if (!nation) return false;
    block(c, 0x102b7fb4, 5);
    c.set(3, c.toc(-0x7b70));
    const auto id = c.u32(nation);
    c.set(4, id);
    const auto selected_nation = c.u32(c.r(3));
    c.set(0, selected_nation);
    c.compare(0, id, selected_nation);
    if (id != selected_nation) return false;

    block(c, 0x102b7fc8, 8);
    c.set(3, c.u32(frame + 0x78));
    c.set(0, c.u32(frame + 0x7c));
    c.put32(frame + 0x48, c.r(3));
    const auto deadline_year = c.u16(frame + 0x6a);
    c.set(5, deadline_year);
    const auto lower_year = c.u16(frame + 0x4a);
    c.set(3, lower_year);
    c.put32(frame + 0x4c, c.r(0));
    c.compare_logical(0, lower_year, deadline_year);
    if (lower_year > deadline_year) {
        block(c, 0x102b7fe8, 2);
        c.set(0, 1);
    } else {
        block(c, 0x102b7ff0, 1);
        if (lower_year < deadline_year) {
            block(c, 0x102b7ff4, 2);
            c.set(0, 0);
        } else {
            block(c, 0x102b7ffc, 1);
            const auto lower_day = c.i16(frame + 0x48);
            c.set(4, std::uint32_t(lower_day));
            block(c, 0x102b8000, 9);
            const auto deadline_day = c.i16(frame + 0x68);
            c.set(0, std::uint32_t(deadline_day));
            publish_day_order(c, deadline_day, lower_day);
        }
    }
    if (lower_year != deadline_year) block(c, 0x102b801c, 2);
    if (record_nonzero_byte(c, c.r(0))) return false;

    block(c, 0x102b8024, 7);
    c.set(3, c.u32(frame + 0x70));
    c.set(0, c.u32(frame + 0x74));
    c.put32(frame + 0x40, c.r(3));
    const auto upper_year = c.u16(frame + 0x42);
    c.set(3, upper_year);
    c.put32(frame + 0x44, c.r(0));
    c.compare_logical(0, upper_year, deadline_year);
    if (upper_year < deadline_year) {
        block(c, 0x102b8040, 2);
        c.set(0, 1);
    } else {
        block(c, 0x102b8048, 1);
        if (upper_year > deadline_year) {
            block(c, 0x102b804c, 2);
            c.set(0, 0);
        } else {
            block(c, 0x102b8054, 8);
            const auto upper_day = c.i16(frame + 0x40);
            c.set(0, std::uint32_t(upper_day));
            const auto deadline_day = c.i16(frame + 0x68);
            c.set(4, std::uint32_t(deadline_day));
            publish_day_order(c, upper_day, deadline_day);
        }
    }
    block(c, 0x102b8074, 2);
    return !record_nonzero_byte(c, c.r(0));
}

std::uint32_t all_star_club(const DutyQuery& q) {
    auto& c = q.c;
    block(c, 0x102b807c, 10);
    const auto first = c.u32(q.first_star);
    c.set({{3, first}, {7, 0}});
    const auto second = c.u32(q.second_star);
    c.set({{0, second}, {4, 0}, {3, first * club_size}});
    const auto clubs = c.u32(q.clubs);
    c.set({{5, clubs}, {0, second * club_size}, {6, clubs + first * club_size}, {5, clubs + second * club_size}});
    for (unsigned row = 0; ; ++row) {
        block(c, 0x102b8148, 3);
        c.set(0, row);
        c.compare(0, row, 50);
        if (row == 50) return end_duty(q, 0x102b8154, false);
        block(c, 0x102b80a8, 4);
        const auto offset = club_roster + row * 4;
        c.set(3, offset);
        const auto first_player = c.u32(clubs + first * club_size + offset);
        c.set(0, first_player);
        c.compare_logical(0, first_player, q.player);
        const bool first_match = first_player == q.player;
        bool match = first_match;
        if (!match) {
            block(c, 0x102b80f8, 3);
            const auto second_player = c.u32(clubs + second * club_size + offset);
            c.set(0, second_player);
            c.compare_logical(0, second_player, q.player);
            match = second_player == q.player;
        }
        if (match) {
            const auto branch = first_match ? 0x102b80b8u : 0x102b8104u;
            block(c, branch, 2);
            c.compare_logical(0, q.label, 0);
            if (q.label) label_duty(q, branch + 8, DutyLabel::all_star);
            const auto dispose = branch + 24;
            block(c, dispose, 7);
            c.set(0, c.u32(first_match ? q.first_star : q.second_star));
            c.set(3, q.list());
            c.set(5, c.u32(q.clubs));
            c.set({{4, UINT32_MAX}, {0, c.r(0) * club_size}});
            c.set(21, c.r(5) + c.r(0));
            c.call(0x10738c20, dispose + 28);
            block(c, dispose + 28, 2);
            return c.r(21);
        }
        block(c, 0x102b8140, 2);
        c.set({{4, (row + 1) * 4}, {7, row + 1}});
    }
}

std::uint32_t resolve_duty(const DutyQuery& q) {
    auto& c = q.c;
    switch (ignored_request(q)) {
    case RequestDisposition::national: return end_duty(q, 0x102b7ccc, false);
    case RequestDisposition::all_star: return end_duty(q, 0x102b7d04, false);
    case RequestDisposition::eligible: break;
    }
    if (const auto national_team = build_duty_window(q)) {
        if (fixture_conflict(q, national_team) || roster_conflict(q)) return c.r(3);
    }
    if (all_star_window(q)) return all_star_club(q);
    return end_duty(q, 0x102b8154, false);
}

void duty_conflict(Cpu& c) {
    GuestNativeScope scope(c.s, duty_conflict_entry);
    block(c, duty_conflict_entry, 17);
    const auto caller = c.sp(), frame = caller - 0xc0;
    const auto manager = c.r(3), player = c.r(4), date = c.r(5), club = c.r(6), label = c.r(7), competition = c.r(8);
    c.set(0, c.lr());
    guest::transfer_saved_registers<21>(c, caller);
    const auto first_star = c.toc(first_all_star_id);
    c.set({{27, first_star}, {21, manager}});
    const auto second_star = c.toc(second_all_star_id);
    c.set({{28, second_star}, {22, player}});
    const auto shared_label = c.toc(-0x7bb8);
    c.set({{29, shared_label}, {23, date}});
    const auto clubs = c.toc(-0x7be8);
    c.set({{30, clubs}, {26, club}});
    const auto source = c.toc(-0x513c);
    c.set({{31, source}, {24, label}, {25, competition}});
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    const DutyQuery query{c, frame, manager, player, date, club, label, competition,
                         first_star, second_star, shared_label, clubs, source};
    c.set(3, query.list());
    c.link(0x102b7cb4);
    c.set(0, 0);
    for (unsigned offset : {4u, 8u, 12u}) c.put32(query.list() + offset, 0);
    c.put16(query.list(), 0);
    c.s.charge(6); // The original inlines its temporary list constructor.

    c.set(3, resolve_duty(query));
    block(c, 0x102b8164, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    guest::transfer_saved_registers<21, true>(c, caller);
    c.finish();
}

}

void national_teams_run_original(State& state, std::uint32_t entry) {
    run_original_body(state, entry, declining);
}

void national_teams_run_rewrite(State& state, std::uint32_t entry) {
    Cpu c(state);
    switch (entry) {
    case selection_context_entry: selection_context(c); break;
    case selection_flag_entry: selection_slot<SelectionAccess::flag>(c); break;
    case clear_selection_flag_entry: selection_slot<SelectionAccess::clear_flag>(c); break;
    case selection_value_entry: selection_slot<SelectionAccess::value>(c); break;
    case selection_slot_entry: selection_slot<SelectionAccess::address>(c); break;
    case international_club_entry: international_club(c); break;
    case duty_conflict_entry: duty_conflict(c); break;
    case assigned_staff_entry: assigned_staff(c); break;
    default: throw std::logic_error("Unknown NATIONAL_TEAMS entry " + hex_address(entry));
    }
}

bool try_modern_national_teams(State& state, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(state, entry, national_teams_run_original, national_teams_run_rewrite,
                               RoutineChecker::result_r3);
    national_teams_run_rewrite(state, entry);
    return true;
}
}
