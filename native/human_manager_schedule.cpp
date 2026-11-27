// HUMAN_MANAGER.CPP block "schedule": see human_manager_schedule.hpp.
#include "human_manager_schedule.hpp"
#include "database_records.hpp"
#include "date.hpp"
#include "human_manager_detail.hpp"
#include "human_manager_records.hpp"
#include "news.hpp"
#include "holiday_digest.hpp"

namespace cm::aot::human_manager {
namespace {
// TOC words (the cell at each slot holds an address) this block alone reads.
namespace slot {
inline constexpr std::int32_t notify = -0x7420,           // word: non-zero while the match day wants to hear of changes
                              news = -0x7bf0,             // the news queue's cell
                              fixtures = -0x7a60,         // the fixture lists (FIX_MAN.CPP)
                              fixture_lists = -0x73f0,    // byte: how many lists there are
                              bookings_skipped = -0x73ec, // byte, read by 0x10149790
                              season_byte = -0x79ec,      // byte, compared with a row's +0x3f
                              today = -0x7b80;            // halfwords: day, year (compared with a row's +0x2a / +0x28)
}
namespace routine {
inline constexpr std::uint32_t match_day = 0x101cc810, fill = 0x10778300, block_move = 0x1077fc28, fixture_list = 0x1048d420,
                               list_next = 0x10738e00;
}
// The table's counters of humans away / on holiday (words after `count`).
inline constexpr std::uint32_t table_holidays = 0x08, table_away = 0x0c;
// The booking row (0x50 bytes) and its fields; the human's own copy is at record::booking.
namespace row {
inline constexpr std::uint32_t size = 0x50, first = 0x00, fourth = 0x0c, fifth = 0x10, first_club = 0x1c, second_club = 0x20,
                               year = 0x28, day = 0x2a, halfword = 0x34, season = 0x3f, first_flag = 0x40, second_flag = 0x41,
                               kind = 0x43;
}
inline constexpr std::uint32_t booking = 0x9a68;  // in a human's record

// `lwz r3,-0x7420(r2); lwz r3,0(r3); cmplwi; bl 0x101cc810`
void tell_match_day(Cpu& c, std::uint32_t back) {
    if (const auto flag = c.u32(c.toc(slot::notify))) invoke(c, routine::match_day, back, {flag});
}

// 0x10148340 and 0x10148660: a byte of the record, then the match day.
struct OptionByte { std::uint32_t entry, offset, back; Checks checks; };
void store_option(Cpu& c, const OptionByte& option, std::uint32_t humans, std::uint32_t value, std::uint32_t staff) {
    const Opening frame(c, option.entry);
    const auto index = index_or_report(c, frame, humans, staff, option.checks);
    if (!index) return;
    c.put8(record_at(c, humans, *index) + option.offset, value);
    tell_match_day(c, option.back);
}
// The signed byte 0 is the only value a flag byte compares equal to zero (extsb).
bool nonzero(std::uint32_t byte) { return std::int8_t(byte) != 0; }
}

void choose_tactic_for_club(Cpu& c, std::uint32_t humans, std::uint32_t flag, std::uint32_t club, std::uint32_t staff) {
    const Frame frame(c, 0x10146830, 0x80, {.first_gpr = 26});
    c.set({{26, c.toc(toc::message)}, {27, c.toc(toc::pool)}, {28, humans}, {29, flag}, {30, club}});
    staff = staff_or_current(c, staff);
    if (!staff) return;
    if (!club) c.set(30, club = c.u32(staff + 0x24));
    c.set(31, c.u32(staff + staff::id) - c.u32(c.toc(toc::staff_count)) + std::uint32_t(human_slots));
    const auto index = index_of(c, humans, staff);
    if (!index) {
        report(c, frame, {0x101468c0, 0x40, 0x38, 0x167});
        return;
    }
    const auto store = [&](std::uint32_t slot) { c.put8(record_at(c, humans, *index) + record::current_tactic, slot); };
    if (club) {
        c.link(0x10146914);
        database::range_position(c, club);
        if (nonzero(c.r(3))) return store(4);
    }
    if (nonzero(flag) && club) {
        c.link(0x10146958);
        database::range_position_doubled(c, club);
        if (nonzero(c.r(3))) return store(3);
    }
    // Asked a second time when the first answer was no (the original does).
    if (club) {
        c.link(0x10146994);
        database::range_position_doubled(c, club);
        if (nonzero(c.r(3))) return store(2);
    }
    store(nonzero(flag) ? 1 : 0);
}

void set_away(Cpu& c, std::uint32_t humans, std::uint32_t flag, std::uint32_t staff) {
    const Frame frame(c, 0x10147fd0, 0xa0, {.first_gpr = 27});
    const auto news_queue = c.toc(slot::news);
    c.set({{27, c.toc(toc::message)}, {28, c.toc(toc::pool)}, {29, staff}, {30, news_queue}, {31, c.toc(toc::success)}});
    c.set(29, staff = staff_or_current(c, staff));
    if (!staff) {
        report(c, frame, {0x10148028, 0x68, 0x40, 0x40d});
        return;
    }
    const auto index = index_of(c, humans, staff);
    if (!index) {
        report(c, frame, {0x101480a4, 0x48, 0x38, 0x419});
        return;
    }
    const auto away = record_at(c, humans, *index) + record::away;
    const bool was_away = nonzero(c.u8(away));
    if (nonzero(flag) && !was_away) c.put32(humans + table_away, c.u32(humans + table_away) + 1);
    if (!nonzero(flag) && was_away) c.put32(humans + table_away, c.u32(humans + table_away) - 1);
    c.put8(away, flag);
    if (nonzero(flag)) {
        // Both of the staff's club words get the news (+0x24 first, then +0x20).
        if (const auto club = c.u32(staff + 0x24)) {
            c.link(0x10148190);
            news::perform_matching_actions(c, c.u32(news_queue), staff, club);
        }
        if (const auto club = c.u32(staff + 0x20)) {
            c.link(0x101481a8);
            news::perform_matching_actions(c, c.u32(news_queue), staff, club);
        }
    }
    tell_match_day(c, 0x101481bc);
    if (was_away != nonzero(flag)) holiday_digest::transition(c, staff, nonzero(flag));
}

void set_runs_club(Cpu& c, std::uint32_t humans, std::uint32_t value, std::uint32_t staff) {
    store_option(c, {0x10148340, record::runs_club, 0x10148488, {0x1014839c, 0x46e, 0x10148418, 0x47a}}, humans, value, staff);
}
void set_runs_nation(Cpu& c, std::uint32_t humans, std::uint32_t value, std::uint32_t staff) {
    store_option(c, {0x10148660, record::runs_nation, 0x101487a8, {0x101486bc, 0x4ca, 0x10148738, 0x4d6}}, humans, value, staff);
}

void set_holiday(Cpu& c, std::uint32_t humans, std::uint32_t flag, std::uint32_t staff) {
    const Opening frame(c, 0x101492d0, Opening::Order::message_first);
    const auto index = index_or_report(c, frame, humans, staff, {0x1014932c, 0x628, 0x101493a8, 0x634});
    if (!index) return;
    const auto holiday = record_at(c, humans, *index) + record::holiday;
    if (nonzero(flag) && !nonzero(c.u8(holiday))) c.put32(humans + table_holidays, c.u32(humans + table_holidays) + 1);
    if (!nonzero(flag) && nonzero(c.u8(holiday))) c.put32(humans + table_holidays, c.u32(humans + table_holidays) - 1);
    c.put8(holiday, flag);
    tell_match_day(c, 0x10149488);
}

std::uint32_t booking_matches(Cpu& c, std::uint32_t humans, std::uint32_t row, std::uint32_t staff) {
    const Opening frame(c, 0x101494b0);
    if (!row) {
        report(c, frame, {0x101494f4, 0x68, 0x40, 0x654});
        return 0;
    }
    staff = staff_or_current(c, staff);
    if (!staff) return 0;  // silent, as in the original
    const auto index = index_of(c, humans, staff);
    if (!index) {
        report(c, frame, {0x1014959c, 0x48, 0x38, 0x66a});
        return 0;
    }
    const auto own = record_at(c, humans, *index) + booking;
    return c.u8(row + row::season) == c.u8(own + row::season) && c.u16(row + row::day) == c.u16(own + row::day) &&
           c.u16(row + row::year) == c.u16(own + row::year) && c.u32(row + row::first) == c.u32(own + row::first) &&
           c.u16(row + row::halfword) == c.u16(own + row::halfword) && c.u32(row + row::fourth) == c.u32(own + row::fourth) &&
           c.u32(row + row::fifth) == c.u32(own + row::fifth);
}

std::uint32_t any_booking_matches(Cpu& c, std::uint32_t humans, std::uint32_t row) {
    const Frame frame(c, 0x10149690, 0x80, {.first_gpr = 26});
    const auto count_cell = c.toc(toc::staff_count), people_cell = c.toc(toc::people);
    c.set({{26, count_cell}, {27, people_cell}, {28, c.toc(toc::message)}, {29, c.toc(toc::pool)}, {30, humans}, {31, row}});
    if (!row) {
        report(c, frame, {0x101496d0, 0x40, 0x38, 0x68b});
        return 0;
    }
    // The 16 humans' person records: the last 16 of the staff.
    for (std::uint32_t human = 0; std::int8_t(human) < human_slots; ++human) {
        c.set(28, human);
        const auto person = c.u32(people_cell) + (c.u32(count_cell) + human - std::uint32_t(human_slots)) * 0x70;
        if (!nonzero(c.u8(person + 0x6b))) continue;
        c.link(0x10149754);
        if (nonzero(booking_matches(c, humans, row, person))) return 1;
    }
    return 0;
}

std::uint32_t set_booking(Cpu& c, std::uint32_t humans, std::uint32_t row, std::uint32_t staff) {
    const Frame frame(c, 0x10149790, 0x90, {.first_gpr = 23});
    const auto skipped = c.toc(slot::bookings_skipped), season = c.toc(slot::season_byte), today = c.toc(slot::today),
               notify = c.toc(slot::notify);
    c.set({{23, humans}, {24, row}, {26, skipped}, {27, season}, {28, today}, {29, notify}, {30, c.toc(toc::message)},
           {31, c.toc(toc::pool)}});
    staff = staff_or_current(c, staff);
    if (!staff) return 0;
    c.set(25, c.u32(staff + staff::id) - c.u32(c.toc(toc::staff_count)) + std::uint32_t(human_slots));
    const auto index = index_of(c, humans, staff);
    if (!index) {
        report(c, frame, {0x10149828, 0x40, 0x38, 0x6bf});
        return 0;
    }
    if (row) {
        c.link(0x10149884);
        if (!nonzero(booking_is_free(c, humans, row, staff))) return 0;
    }
    // Both ways: nothing to change when the day, year and season already are today's and the game
    // listens (a signed halfword against an unsigned one: -1 never equals).
    const std::uint32_t today_day = std::uint32_t(std::int32_t(c.i16(today))), today_year = c.u16(today + 2);
    const auto unchanged = [&](std::uint32_t at) {
        return c.u16(at + row::day) == today_day && c.u16(at + row::year) == today_year &&
               c.u8(at + row::season) == c.u8(season) && c.u32(notify) != 0 && !nonzero(c.u8(skipped));
    };
    if (row && unchanged(row)) return 0;
    if (!row && unchanged(record_at(c, humans, *index) + booking)) return 0;
    const auto own = record_at(c, humans, *index) + booking;
    if (!row) {
        invoke(c, routine::fill, 0x101499bc, {own, 0, row::size});
    } else {
        invoke(c, routine::block_move, 0x1014998c, {row, own, row::size});
        c.set(2, c.u32(frame.at(0x14)));  // the import glue saves r2 at sp+0x14 and the original reloads it
    }
    tell_match_day(c, 0x101499cc);
    return 1;
}

std::uint32_t booking_is_free(Cpu& c, std::uint32_t /*humans*/, std::uint32_t row, std::uint32_t staff) {
    const Frame frame(c, 0x101499f0, 0x90, {.first_gpr = 25});
    const auto lists_cell = c.toc(slot::fixtures), count_cell = c.toc(slot::fixture_lists);
    c.set({{26, c.toc(toc::message)}, {27, c.toc(toc::pool)}, {28, row}, {29, staff}, {30, count_cell}, {31, lists_cell}});
    if (!row) {
        report(c, frame, {0x10149a34, 0x48, 0x40, 0x6ed});
        return 0;
    }
    c.set(29, staff = staff_or_current(c, staff));
    if (!staff) return 0;
    if (std::int8_t(c.u8(row + row::kind)) > -1) return 0;
    // Whether the human runs either club of a row (the clubs' own flag bytes at +0x40 / +0x41).
    const auto runs_a_club = [&](std::uint32_t at, std::uint32_t first_back, std::uint32_t second_back) {
        c.link(first_back);
        if (nonzero(runs(c, c.u32(at + row::first_club), 0, c.u8(at + row::first_flag), staff, 0))) return true;
        c.link(second_back);
        return nonzero(runs(c, c.u32(at + row::second_club), 0, c.u8(at + row::second_flag), staff, 0));
    };
    if (runs_a_club(row, 0x10149ad4, 0x10149af4)) return 0;
    for (std::uint32_t list = 0; std::int8_t(list) < c.i8(count_cell); ++list) {
        c.set(26, list);
        c.link(0x10149b20);
        date::from_day_and_year(c, frame.at(0x38), std::uint32_t(std::int32_t(c.i16(row + row::day))), c.u16(row + row::year));
        const auto fixtures = invoke(c, routine::fixture_list, 0x10149b34,
                                     {c.u32(lists_cell), c.r(3), c.u8(row + row::season), list});
        c.set(25, fixtures);
        if (!fixtures) continue;
        c.put32(fixtures + 0xc, c.u32(fixtures + 4));
        for (;;) {
            const auto node = invoke(c, routine::list_next, 0x10149b98, {fixtures});
            c.set(27, node);
            if (!node) break;
            if (runs_a_club(node, 0x10149b60, 0x10149b80)) return 0;
        }
    }
    return 1;
}

std::uint32_t clear_taken_bookings(Cpu& c, std::uint32_t humans) {
    const Frame frame(c, 0x10149bd0, 0x50, {.first_gpr = 27});
    const auto count_cell = c.toc(toc::staff_count), people_cell = c.toc(toc::people);
    c.set({{27, humans}, {30, count_cell}, {31, people_cell}});
    std::uint32_t offset = 0;
    for (std::uint32_t human = 0; std::int8_t(human) < human_slots; ++human, offset += record::size) {
        c.set({{28, human}, {29, offset}});
        const auto person = c.u32(people_cell) + (c.u32(count_cell) + human - std::uint32_t(human_slots)) * 0x70;
        if (!nonzero(c.u8(person + 0x6b))) continue;
        c.link(0x10149c38);
        if (nonzero(booking_is_free(c, humans, c.u32(humans) + offset + booking, person))) continue;
        invoke(c, routine::fill, 0x10149c5c, {c.u32(humans) + offset + booking, 0, row::size});
    }
    return 1;
}

// A frameless leaf: the club's own staff, else the candidate with the highest +0x2a of its +0x1c record.
std::uint32_t club_head_staff(Cpu& c, std::uint32_t club) {
    std::uint32_t best = 0;
    for (std::int32_t slot = -1; slot < 5; ++slot) {
        if (slot == -1) {
            if ((best = c.u32(club + 0x94))) return best;
            continue;
        }
        const auto candidate = c.u32(club + 0x160 + std::uint32_t(slot) * 4);
        if (!candidate) continue;
        const auto record = c.u32(candidate + 0x1c);
        if (!record) continue;
        if (best && c.i16(record + 0x2a) <= c.i16(c.u32(best + 0x1c) + 0x2a)) continue;
        best = candidate;
    }
    return best;
}

void drop_unfit_entries(Cpu& c, std::uint32_t humans) {
    const Frame frame(c, 0x1014c680, 0x80, {.first_gpr = 27});
    const auto people_cell = c.toc(toc::people);
    c.set({{27, humans}, {28, c.toc(toc::pool)}, {30, c.toc(toc::message)}, {31, people_cell}});
    const auto staff = staff_or_current(c, 0);
    c.set(29, staff);
    if (!staff) {
        report(c, frame, {0x1014c6d0, 0x40, 0x38, 0xb73});
        return;
    }
    // No range check here (unlike the other routines): the index is used as it comes.
    const auto offset = (c.u32(staff + staff::id) - c.u32(c.toc(toc::staff_count)) + std::uint32_t(human_slots)) * record::size;
    c.set(30, offset);
    auto slot = c.u8(c.u32(humans) + offset + record::current_tactic);
    if (slot == 1) slot = 0;  // cmpwi on the unsigned byte; the slot itself is signed below
    const auto list = c.u32(humans) + offset + std::uint32_t(std::int32_t(std::int8_t(slot))) * record::tactic_size + 0x1dec;
    c.set(28, list);
    // A national side's entries are compared with the staff's club's nation (0x100c7860), else with the table.
    std::uint32_t other = humans;
    if (const auto job = c.u32(staff + 0x20)) {
        c.link(0x1014c778);
        database::club_at_bias(c, c.u32(job + 0x2c));
        other = c.r(3);
    }
    for (std::uint32_t at = 0; std::int8_t(at / 4) < 50; at += 4) {
        const auto entry = c.u32(list + at);
        if (entry == 0xffffffffu) continue;
        const auto person = c.u32(people_cell) + entry * 0x70;
        const auto kind = std::int8_t(c.u8(c.u32(humans) + record::current_tactic + offset));
        const auto drop = [&] { c.put32(list + at, 0xffffffffu); };
        if (kind == 0 || kind == 1) {
            if (c.u32(person + 0x24) != c.u32(staff + 0x24)) drop();
        } else if (kind == 2 || kind == 3) {
            if (c.u32(person + 0x20) != c.u32(staff + 0x20) || (c.u8(person + 0x6a) & (kind == 2 ? 4 : 8)) == 0) drop();
        } else if (kind == 4) {
            if (c.u32(person + 0x20) != other || (c.u8(person + 0x6a) & 4) == 0) drop();
        }
    }
}
}
