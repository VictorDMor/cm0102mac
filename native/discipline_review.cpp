// DISCIPLINE.CPP review block (discipline_review.hpp), native-first: the ruling of one player after a match.
#include "discipline_review.hpp"
#include "discipline_adapter.hpp"
#include "discipline_bans.hpp"
#include "discipline_review_detail.hpp"

namespace cm::aot::discipline {
using guest::Cpu;
using guest::Frame;
using namespace review;

namespace {
// Offsets of the ban the ruling builds in its frame (the layout of a ban record, discipline_bans_detail.hpp),
// and of the two date buffers 0x101929b0 fills.
constexpr std::int32_t ban_at = 0x48, today_gap_at = 0x38, key_date_at = 0x40, ban_date_at = 0x4c;
constexpr std::int16_t default_days = 0x5a, short_days = 0xb4;
constexpr std::int32_t short_gap = 0x50;  // 0x10718130's result below this gives the longer ban
constexpr std::uint8_t flag_counted = 0x40, flag_fresh = 0x20;  // bits of the ban's flag byte this ruling sets

// The news write-up (0x106a15f0) the original asks for at each of its four sites.
void news_ruling(Cpu& c, std::uint32_t human_manager, std::uint32_t record, std::uint32_t kind, std::uint32_t days,
                 std::uint32_t back) {
    c.set({{3, c.u32(c.toc(globals::news))}, {4, human_manager}, {5, record}, {6, kind}, {7, 0xffffffffu}, {8, days},
           {9, 0xffffffffu}});
    c.call(outside::news_ruling, back);
}

// One draw taken as the original's three inline ones are: true when the entry plus the base is a multiple of 20.
// `from` is the cursor the original advances (see step()), `back` the address after its call of the generator.
bool one_in_twenty(Cpu& c, const DrawTable& table, std::uint32_t from, std::uint32_t back) {
    const auto position = step(c, table, from, back);
    // mulhw 0x66666667, srawi 3 and the sign fix-up are the compiler's signed division by 20.
    return std::int32_t(draw_sum(c, table, position)) % 20 == 0;
}

// Clears every ban the person holds in the first four slots of its row in the manager's table.
// `set_slot(slot)` sets the registers the original has at the calls of that iteration.
void clear_held_bans(Cpu& c, std::uint32_t manager, std::uint32_t entry, std::uint32_t row, const auto& set_slot) {
    for (std::int8_t slot = 0; slot < 4; ++slot) {
        const auto body = c.i8(row + std::uint32_t(slot) * 8);
        if (body == -1) continue;
        set_slot(slot);
        c.set({{3, ruling_body(c, manager, body)}, {4, c.u32(entry + entry_field::person)}, {5, 0}});
        c.call(callee::find_entry, 0x100e8840);
        const auto held = c.r(3);
        if (held == 0 || c.i16(held + body_entry_index) <= -1) continue;
        set_slot(slot);
        c.link(0x100e8860);
        clear_ban(c, manager, c.u32(entry + entry_field::person), c.i16(held + body_entry_index));
    }
}
}

std::uint32_t review_player(Cpu& c, std::uint32_t manager, std::uint32_t entry, std::uint32_t argument) {
    Frame frame(c, 0x100e8780, 0xc0, {.first_gpr = 17});
    const auto people = [&] { return c.u32(c.toc(toc_slot::people)); };
    const auto today = c.toc(toc_slot::today);
    // r17-r31 hold what the original keeps in them, set where it sets them: the generated callees (0x105bc2b0,
    // the dates', the generator 0x1077b770, the news write-up) and this module's own routines save them in
    // their frames, and a ruling body later copies a ban struct it never fully wrote from that stack into a ban
    // record (bytes +0xd..+0xf, part of +0x14; the BlockMoveData reader 0x10bf7bbc that tools/stack_contract.py
    // lists for 0x100e8780). r17 is the pointer into the person's table row (then the draw's remainder), r21
    // its slot counter (4 after the loop), r22 the person record, r29 the manager cell, then the person's
    // human manager, r31 the people table cell, then the conduct bound. Until r17, r21 and r22 are set they
    // are the caller's.
    const auto table_cells = draw_table(c);
    const auto set_fixed = [&] {
        c.set({{18, manager}, {19, entry}, {20, argument}, {23, c.toc(globals::news)}, {24, table_cells.base_cell},
               {25, table_cells.first}, {26, table_cells.last_cell}, {27, table_cells.cursor_cell},
               {28, c.toc(globals::standing)}, {30, today}});
    };
    std::uint32_t manager_cell = c.toc(globals::manager), people_cell = c.toc(toc_slot::people);
    const auto set_registers = [&] { set_fixed(); c.set({{29, manager_cell}, {31, people_cell}}); };
    set_registers();
    // The two date buffers: 0x10715880 constructs each.
    c.set(3, frame.at(key_date_at));
    c.call(outside::date_construct, 0x100e87c8);
    c.set(3, frame.at(today_gap_at));
    c.call(outside::date_construct, 0x100e87d0);

    const auto record = people() + c.u32(entry + entry_field::person) * person_record::size;
    c.set(22, record);
    if (c.u8(entry + entry_field::first_flag) != entry_field::ruling_case &&
        c.u8(entry + entry_field::second_flag) != entry_field::ruling_case)
        return 0;
    const auto held_row = c.u32(manager + manager::people) + c.u32(entry + entry_field::person) * row::size;
    clear_held_bans(c, manager, entry, held_row, [&](std::int8_t slot) {
        set_registers();
        c.set({{17, held_row + std::uint32_t(slot) * 8}, {21, std::uint32_t(std::int32_t(slot))}});
    });
    // The loop is over: r17 points past the row's four slots and r21 is 4.
    std::uint32_t r17 = held_row + 32;
    const auto set_loop_end = [&](std::uint32_t pointer_or_drawn) {
        set_registers();
        c.set({{17, pointer_or_drawn}, {21, 4}});
    };
    set_loop_end(r17);

    // The ban: 90 days, or 180 when the gap from the person's key dates to today is short. The words the
    // original never writes (+3 is part of the days halfword; +9..+0xb, +0x11 and +0x15..+0x17 of the flag word) stay
    // as the stack had them, and register_ban copies the flag word whole.
    c.set(3, frame.at(ban_date_at));
    c.call(outside::date_construct, 0x100e887c);
    c.put8(frame.at(ban_at + ban::matches), 0xff);
    c.put16(frame.at(ban_at + ban::days), std::uint16_t(default_days));
    c.set({{3, c.u32(record + person_manager)}, {4, frame.at(key_date_at)}, {5, frame.at(today_gap_at)}});
    c.call(outside::key_dates, 0x100e889c);
    c.set({{3, frame.at(today_gap_at)}, {4, c.u32(today)}, {5, c.u32(today + 4)}});
    c.call(outside::date_gap, 0x100e88ac);
    if (std::int32_t(c.r(3)) < short_gap) c.put16(frame.at(ban_at + ban::days), std::uint16_t(short_days));
    c.put8(frame.at(ban_at + ban::served), 0);
    c.put16(frame.at(ban_at + ban::date_high), c.u16(today));
    c.put16(frame.at(ban_at + ban::date_low), c.u16(today + 2));
    c.put8(frame.at(ban_at + 8), c.u8(today + 4));
    c.put8(frame.at(ban_at + 0x10), 3);
    c.put16(frame.at(ban_at + 0x12), 0);
    for (unsigned slot = 0; slot < 3; ++slot) c.put8(frame.at(ban_at + ban::bodies) + slot, 0xff);
    c.put8(frame.at(ban_at + ban::bodies + 3), 0);
    const auto flags_at = frame.at(ban_at + ban::flags);
    const auto flags = c.u8(flags_at) | flag_counted;
    c.put8(flags_at, (flags & ~flag_fresh) | (c.u8(entry + entry_field::cautioned) == 0 ? flag_fresh : 0));

    c.set({{3, ruling_body(c, manager, 0)}, {4, c.u32(entry + entry_field::person)}, {5, 1}});
    c.call(callee::find_entry, 0x100e8934);
    c.link(0x100e8944);
    register_ban(c, c.u32(c.toc(globals::manager)), c.u32(entry + entry_field::person), frame.at(ban_at));
    c.set({{3, c.u32(c.toc(globals::manager))},
           {4, people() + c.u32(entry + entry_field::person) * person_record::size},
           {5, frame.at(ban_at)},
           {6, argument}});
    discipline::call(c, outside::tell_body, 0x100e8964);

    // The person's human manager: a news write-up needs one of enough reputation, with a conduct
    // record, passing both of the game's own tests and a standing below -3 in the table.
    const auto human_manager = c.u32(record + person_manager);
    manager_cell = human_manager;  // `lwz r29,0x24(r22)`
    set_registers();
    if (human_manager == 0 || c.i16(human_manager + human::reputation) <= human::reputation_floor) return 1;
    const auto conduct = [&] { return c.u32(human_manager + human::conduct); };
    if (conduct() == 0) return 1;
    c.set({{3, human_manager}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
    c.call(outside::human_flag, 0x100e89a0);
    if (std::int8_t(c.r(3)) != 0) return 1;
    c.set(3, human_manager);
    c.call(outside::person_standing, 0x100e89b0);
    if (std::int8_t(c.r(3)) == 0) return 1;
    const auto standing = [&] {
        return c.i8(c.u32(c.toc(globals::standing)) + c.u32(human_manager + human::id) * 8 + 4);
    };
    if (standing() >= -3) return 1;
    const auto club_table = c.u32(c.u32(c.toc(globals::club_table)));
    if (c.i16(club_table + c.u32(human_manager + human::id) * 0x4c + 6) >= 0x109a) return 1;

    const auto table = draw_table(c);
    // The first draw: a multiple of 20 writes the person up for the ban itself.
    const auto position = step(c, table, c.u32(table.cursor_cell), 0x100e8a3c);
    if (std::int32_t(draw_sum(c, table, position)) % 20 == 0) {
        news_ruling(c, human_manager, record, 1, std::uint32_t(std::int32_t(c.i16(frame.at(ban_at + ban::days)))), 0x100e8a98);
        return 1;
    }
    const auto first = std::int8_t(c.i8(conduct() + human::conduct_first));
    bool bounded = false;
    std::int32_t drawn = 0;
    if (first + standing() < 10) {
        bounded = true;
        // A draw bounded by the two conduct bytes added (a signed byte, so |bound| < 65536 and the sampler's
        // small arm always applies: its wide arm at 0x100e8b28-0x100e8bfc, with the call of 0x100e8d10, is dead).
        const auto bound = std::int8_t(first + c.u8(conduct() + human::conduct_second));
        people_cell = std::uint32_t(std::int32_t(bound));  // `extsb. r31,r0`
        set_registers();
        if (bound != 0) {
            const auto second = step(c, table, position, 0x100e8b00);
            drawn = oriented_remainder(draw_sum(c, table, second), bound);
        }
        if (drawn == 0) {
            set_loop_end(0);
            news_ruling(c, human_manager, record, 3, 0xffffffffu, 0x100e8c54);
            return 1;
        }
    }
    set_loop_end(bounded ? std::uint32_t(drawn) : r17);
    if (c.i8(conduct() + human::conduct_first) > 10 && c.i8(conduct() + human::conduct_limit_byte) > 10 &&
        one_in_twenty(c, table, c.u32(table.cursor_cell), 0x100e8c9c))
        news_ruling(c, human_manager, record, 2, 0xffffffffu, 0x100e8cf8);
    return 1;
}
}
