// HUMAN_MANAGER.CPP: a human's record and its fields, and whether a human runs a club.
#include "human_manager_records.hpp"
#include "database_records.hpp"
#include "human_manager_detail.hpp"

namespace cm::aot::human_manager {
std::uint32_t current_tactic_slot(Cpu& c, std::uint32_t humans, std::uint32_t staff) {
    const Opening frame(c, 0x101466c0);
    const auto human = record_or_report(c, frame, humans, staff, {0x1014671c, 0x12d, 0x1014679c, 0x139});
    return human ? current_tactic(c, *human) : 0;
}
std::uint32_t current_tactic_field(Cpu& c, const FieldRoutine& routine, std::uint32_t humans, std::uint32_t staff) {
    const Opening frame(c, routine.entry);
    const auto human = record_or_report(c, frame, humans, staff, routine.checks);
    return human ? current_tactic(c, *human) + routine.offset : 0;
}
std::uint32_t record_field(Cpu& c, const FieldRoutine& routine, std::uint32_t humans, std::uint32_t staff) {
    const Opening frame(c, routine.entry);
    const auto human = record_or_report(c, frame, humans, staff, routine.checks);
    return human ? *human + routine.offset : 0;
}

std::int32_t short_setting(Cpu& c, std::uint32_t humans, std::uint32_t staff) {
    const Opening frame(c, 0x10147bc0);
    const auto human = record_or_report(c, frame, humans, staff, {0x10147c1c, 0x39b, 0x10147c9c, 0x3a7});
    return human ? c.i16(*human + record::short_setting) : 0;
}
void set_short_setting(Cpu& c, std::uint32_t humans, std::uint16_t value, std::uint32_t staff) {
    const Opening frame(c, 0x10147d20);
    if (const auto human = record_or_report(c, frame, humans, staff, {0x10147d7c, 0x3c1, 0x10147df8, 0x3cd}))
        c.put16(*human + record::short_setting, value);
}
std::uint8_t away(Cpu& c, std::uint32_t humans, std::uint32_t staff) {
    const Opening frame(c, 0x10147e70);
    const auto human = record_or_report(c, frame, humans, staff, {0x10147ecc, 0x3e7, 0x10147f4c, 0x3f3});
    return human ? c.u8(*human + record::away) : 0;
}
std::uint8_t holiday(Cpu& c, std::uint32_t humans, std::uint32_t staff) {
    const Opening frame(c, 0x10149170);
    const auto human = record_or_report(c, frame, humans, staff, {0x101491cc, 0x601, 0x1014924c, 0x60d});
    return human ? c.u8(*human + record::holiday) : 0;
}
std::int8_t runs_club(Cpu& c, std::uint32_t humans, std::uint32_t staff) {
    const Opening frame(c, 0x101481d0);
    const auto human = record_or_report(c, frame, humans, staff, {0x1014822c, 0x448, 0x101482ac, 0x454});
    return human && c.i8(*human + record::runs_club) != 0 ? 1 : 0;
}
// A national side: the option, except that a human whose own club's competition has a rating below
// 400 (staff job +0x2c, competition +0x6c) never runs it, and says so in `refused` (when given).
std::int8_t runs_nation(Cpu& c, std::uint32_t humans, std::uint32_t staff, std::uint32_t refused) {
    const Opening frame(c, 0x101484b0, Opening::Order::message_first);
    if (refused) c.put8(refused, 0);
    const auto index = index_or_report(c, frame, humans, staff, {0x1014851c, 0x49a, 0x1014859c, 0x4a6});
    if (!index) return 0;
    if (const auto job = c.u32(staff + staff::job); job && c.i32(c.u32(job + 0x2c) + 0x6c) < 0x190) {
        if (refused) c.put8(refused, 1);
        return 0;
    }
    return c.i8(record_at(c, humans, *index) + record::runs_nation) != 0 ? 1 : 0;
}
std::uint32_t setting(Cpu& c, std::uint32_t humans, std::uint32_t staff) {
    const Opening frame(c, 0x10148bf0);
    const auto human = record_or_report(c, frame, humans, staff, {0x10148c4c, 0x565, 0x10148ccc, 0x571});
    return human ? c.u32(*human + record::setting) : 0;
}
std::uint32_t set_setting(Cpu& c, std::uint32_t humans, std::uint32_t value, std::uint32_t staff) {
    const Opening frame(c, 0x10148d50);
    const auto human = record_or_report(c, frame, humans, staff, {0x10148dac, 0x58c, 0x10148e2c, 0x598});
    if (!human) return 0;
    c.put32(*human + record::setting, value);
    return 1;
}
std::uint8_t flag(Cpu& c, std::uint32_t humans, std::int8_t which, std::uint32_t staff) {
    const Opening frame(c, 0x10149df0);
    const auto human = record_or_report(c, frame, humans, staff, {0x10149e4c, 0x777, 0x10149ecc, 0x783});
    return human ? c.u8(*human + std::uint32_t(std::int32_t(which)) + record::flags) : 0;
}

std::uint32_t folder(Cpu& c, std::uint32_t humans, std::uint32_t staff, std::int8_t kind) {
    const Frame frame(c, 0x10147150, 0xa0, {.first_gpr = 28});
    const auto buffer = c.toc(toc::folder_buffer), pool = c.toc(toc::pool);
    c.set({{28, buffer}, {29, c.toc(toc::success)}, {30, c.toc(toc::message)}, {31, pool}});
    if (!index_or_report(c, frame, humans, staff, {0x101471b4, 0x249, 0x10147234, 0x255})) return 0;
    // The original's compare ladder: 2 search; 1 tactics; 3 training; anything else print.
    const auto [name, back] = kind == 2 ? std::pair{text::search, 0x101472d4u}
                              : kind == 1 ? std::pair{text::tactics, 0x101472acu}
                              : kind == 3 ? std::pair{text::training, 0x101472c0u}
                                          : std::pair{text::print, 0x101472e8u};
    // Through the dispatcher: a rare path (file dialogs), and the checker answers it there.
    invoke(c, 0x10746300, back, {pool + name, buffer});
    return buffer;
}

std::uint32_t runs(Cpu& c, std::uint32_t club, std::uint32_t ignore_absence, std::uint32_t assume_for_club,
                   std::uint32_t manager, std::uint32_t assume) {
    // Every callee saves some of r24-r31, and code that runs later reads those frames as stale
    // stack (the stack audit lists readers of this routine's and its callees' saves): they hold
    // the prologue's values throughout, as in the original.
    const Frame frame(c, 0x1014c2e0, 0x80, {.first_gpr = 24});
    const auto humans_cell = c.toc(toc::humans);
    c.set({{24, c.toc(toc::message)}, {25, c.toc(toc::pool)}, {26, club}, {27, ignore_absence}, {28, assume_for_club},
           {29, manager}, {30, assume}, {31, humans_cell}});
    if (!club) {
        report(c, frame, {0x1014c328, 0x40, 0x38, 0xaf0});
        return 0;
    }
    if (!manager) c.set(29, manager = c.u32(club + 0x90));
    const auto own = c.u32(club + 0x90);
    const auto first_human = std::int32_t(c.u32(c.toc(toc::staff_count)) - std::uint32_t(human_slots));
    // The club's own manager, a human: unless he is away, his option settles it.
    bool asked = own && manager == own && c.i32(manager + staff::id) >= first_human;
    if (asked && std::int8_t(ignore_absence) == 0) {
        c.link(0x1014c3bc);
        asked = std::int8_t(holiday(c, c.u32(humans_cell), manager)) == 0;
        if (asked) {
            c.link(0x1014c3d0);
            asked = std::int8_t(away(c, c.u32(humans_cell), manager)) == 0;
        }
    }
    if (asked) {
        c.link(0x1014c3e0);
        database::range_position_doubled(c, club);
        const bool national = std::int8_t(c.r(3)) != 0;
        if (std::int8_t(assume) != 0 || std::int8_t(assume_for_club) != 0) return 1;
        c.link(national ? 0x1014c408 : 0x1014c434);
        if ((national ? runs_nation(c, c.u32(humans_cell), manager, 0) : runs_club(c, c.u32(humans_cell), manager)) != 0)
            return 1;
    }
    c.link(0x1014c44c);
    database::range_position(c, club);
    if (std::int8_t(c.r(3)) == 0) return 0;
    c.link(0x1014c45c);
    database::club_at_doubled_bias(c, c.u32(club + 0x2c));
    const auto team = c.r(3);
    c.link(0x1014c470);
    return runs(c, team, ignore_absence, 0, manager, assume);
}

std::uint32_t current_human_runs(Cpu& c, std::uint32_t club, std::uint32_t ignore_absence, std::uint32_t assume) {
    // The same register discipline as 0x1014c2e0 (r27 holds the message buffer, then the human).
    const Frame frame(c, 0x1014c490, 0x80, {.first_gpr = 26});
    const auto humans_cell = c.toc(toc::humans);
    c.set({{26, humans_cell}, {27, c.toc(toc::message)}, {28, c.toc(toc::pool)}, {29, club}, {30, ignore_absence},
           {31, assume}});
    if (!club) {
        report(c, frame, {0x1014c4d4, 0x40, 0x38, 0xb22});
        return 0;
    }
    const auto record = c.toc(toc::game);
    const auto current = std::uint32_t(std::int32_t(c.i16(record + game::current)));
    const auto own = c.u32(club + 0x90);
    const auto human = c.u32(record + current * game::human_size + game::human_staff);
    c.set(27, human);
    bool asked = own && human == own;
    if (asked && std::int8_t(ignore_absence) == 0) {
        c.link(0x1014c554);
        asked = std::int8_t(holiday(c, c.u32(humans_cell), human)) == 0;
        if (asked) {
            c.link(0x1014c568);
            asked = std::int8_t(away(c, c.u32(humans_cell), human)) == 0;
        }
    }
    if (asked) {
        if (std::int8_t(assume) != 0) return 1;
        c.link(0x1014c580);
        database::range_position_doubled(c, club);
        if (std::int8_t(c.r(3)) != 0) return 1;
        c.link(0x1014c594);
        if (runs_club(c, c.u32(humans_cell), human) != 0) return 1;
    }
    c.link(0x1014c5ac);
    database::range_position(c, club);
    if (std::int8_t(c.r(3)) == 0) return 0;
    c.link(0x1014c5bc);
    database::club_at_doubled_bias(c, c.u32(club + 0x2c));
    const auto team = c.r(3);
    c.link(0x1014c5c8);
    return current_human_runs(c, team, ignore_absence, assume);
}
}
