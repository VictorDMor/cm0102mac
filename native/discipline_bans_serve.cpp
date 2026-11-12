// DISCIPLINE.CPP bans block (discipline_bans.hpp), native-first: serving bans (their dates, days and
// matches running out) and clearing the ones that have.
#include "discipline_bans.hpp"
#include "discipline_bans_detail.hpp"
#include "discipline_adapter.hpp"

#include <array>

namespace cm::aot::discipline {
using guest::Cpu;
using guest::Frame;
using namespace manager_arrays;

namespace {
// A date as the game keeps it in a word: ordered by the low halfword first, then by the high one (signed).
struct Date {
    std::int16_t high;
    std::uint16_t low;
};
Date date_of(std::uint32_t word) { return {std::int16_t(word >> 16), std::uint16_t(word)}; }
// `today` is before `end`. The original computes the high half's test as the sign of
// (x >> 1) - (x & end_high) with x = end_high ^ today_high, which is end_high > today_high for
// every pair of halfwords (checked over all 2^32 pairs).
bool before(Date today, Date end) {
    if (today.low != end.low) return today.low < end.low;
    return end.high > today.high;
}

// Sends the ban to the history file (the table block's 0x100e7160) and clears it.
void retire(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t entry, std::uint32_t record,
            std::uint32_t history_back, std::uint32_t clear_back) {
    c.set({{3, manager}, {4, person}, {5, record}});
    discipline::call(c, callee::record_history, history_back);
    c.link(clear_back);
    clear_ban(c, manager, person, c.i16(entry + body_entry_index));
}
}

void serve_ban(Cpu& c, std::uint32_t manager, std::uint32_t person, std::int8_t body, std::int8_t count_served) {
    // Its dead stack is compared (a later BlockMoveData copies it): the frame and the date copies
    // below are the original's.
    Frame frame(c, 0x100e0250, 0x70, {.first_gpr = 26});
    // r26-r31 hold what the original keeps in them, from where it sets each: the generated callees
    // (0x105bc2b0, 0x10717890, the history file, 0x105bc5c0 under clear_ban) save them in their frames,
    // and a ruling body later copies a ban struct it never fully wrote from that stack into a ban record
    // (bytes +0xd..+0xf; the BlockMoveData reader 0x10bf7bbc that tools/stack_contract.py lists for
    // 0x100e0250). Found by the live check on season-day. All callers pass r6 with `li`.
    const auto today_address = c.toc(toc_slot::today);
    c.set({{26, today_address}, {27, manager}, {28, person}, {29, std::uint32_t(std::int32_t(count_served))}});
    c.set({{3, ruling_body(c, manager, body)}, {4, person}, {5, 0}});
    c.call(callee::find_entry, 0x100e0288);
    const auto entry = c.r(3);
    c.set(31, entry);
    if (!entry || c.i16(entry + body_entry_index) == -1) return;
    const auto record = ban_record(c, manager, c.i16(entry + body_entry_index));
    c.set(30, record);
    const auto today_word = c.u32(today_address), today_second = c.u32(today_address + 4);
    const auto today = date_of(today_word);
    if (c.i8(record + ban::matches) == -1) {
        // A days ban: it ends `days` after its start date.
        c.put32(frame.at(0x38), today_word);
        c.put32(frame.at(0x3c), today_second);
        const auto end_at = frame.at(0x48);
        c.set({{3, end_at}, {4, record + ban::date_high}, {5, std::uint32_t(std::int32_t(c.i16(record + ban::days)))}});
        c.call(callee::add_days, 0x100e0384);
        if (before(today, {c.i16(end_at), c.u16(end_at + 2)})) return;
        retire(c, manager, person, entry, record, 0x100e03e0, 0x100e03f0);
    } else {
        // A matches ban: its date is compared with today directly; a counted call serves one more match.
        c.put32(frame.at(0x40), today_word);
        c.put32(frame.at(0x44), today_second);
        if (before(today, {c.i16(record + ban::date_high), c.u16(record + ban::date_low)}) || count_served == 0) return;
        c.put8(record + ban::served, c.u8(record + ban::served) + 1u);
        if (c.i8(record + ban::matches) != c.i8(record + ban::served)) return;
        retire(c, manager, person, entry, record, 0x100e0350, 0x100e0360);
    }
}

void reduce_ban(Cpu& c, std::uint32_t manager, std::uint32_t person, std::int8_t body, std::int8_t counted,
                std::int8_t matches_default, std::int16_t days, std::int8_t matches, std::int16_t days_selector) {
    Frame frame(c, 0x100e0410, 0x60, {.first_gpr = 23});
    // r23-r31 hold what the original keeps in them: its arguments r7, r9, r10, r6, r8 (as the caller left
    // the registers) in r23, r24, r25, r29, r30, the body slot offset in r26, the manager and person in
    // r27 and r28, and from the first call on the entry found in r31. The generated callees (0x105bc2b0,
    // 0x105bd4f0, clear_ban's) save them in their frames, and a ruling body later copies a ban struct it
    // never fully wrote from that stack into a ban record (the BlockMoveData reader 0x10bf7bbc that
    // tools/stack_contract.py lists for 0x100e0410).
    const auto arguments = std::array{c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)};
    const auto set_registers = [&] {
        c.set({{23, arguments[1]}, {24, arguments[3]}, {25, arguments[4]}, {26, std::uint32_t(std::int32_t(body)) << 2},
               {27, manager}, {28, person}, {29, arguments[0]}, {30, arguments[2]}});
    };
    set_registers();
    c.set({{3, ruling_body(c, manager, body)}, {4, person}, {5, 0}});
    c.call(callee::find_entry, 0x100e0454);
    const auto entry = c.r(3);
    c.set(31, entry);
    if (!entry || c.i16(entry + body_entry_index) == -1) return;
    c.set({{3, ruling_body(c, manager, body)}, {4, person}});
    c.call(callee::touch_entry, 0x100e0478);
    const auto index = c.i16(entry + body_entry_index);
    const auto record = ban_record(c, manager, index);
    // The clear calls the original makes without setting r5 pass the index loaded above.
    const auto clear = [&](std::int16_t at, std::uint32_t back) {
        set_registers();
        c.link(back);
        clear_ban(c, manager, person, at);
    };
    const auto days_at = record + ban::days;
    if (c.i8(record + ban::matches) == -1) {
        // A days ban.
        if (days_selector != -1) {
            c.put16(days_at, std::uint32_t(c.i16(days_at) - days));
            if (c.i16(days_at) > 0) return;
            c.put16(days_at, 0);
            clear(c.i16(entry + body_entry_index), 0x100e055c);
        } else if (counted == 0) {
            clear(index, 0x100e05a0);
        } else {
            c.put16(days_at, std::uint32_t(c.i16(days_at) - days));
            if (c.i16(days_at) > 0) return;
            clear(c.i16(entry + body_entry_index), 0x100e0590);
        }
    } else if (matches == -1) {
        // A matches ban, default decrement (when counted): it ends when the matches left fall to the served count.
        if (counted == 0) return clear(index, 0x100e051c);
        c.put8(record + ban::matches, c.u8(record + ban::matches) - std::uint32_t(std::int32_t(matches_default)));
        if (c.i8(record + ban::matches) > c.i8(record + ban::served)) return;
        clear(c.i16(entry + body_entry_index), 0x100e050c);
    } else {
        c.put8(record + ban::matches, c.u8(record + ban::matches) - std::uint32_t(std::int32_t(matches)));
        if (c.i8(record + ban::matches) >= 0) return;
        c.put8(record + ban::matches, 0);
        clear(c.i16(entry + body_entry_index), 0x100e04d0);
    }
}

void serve_key_holders(Cpu& c, std::uint32_t manager, std::uint32_t ruling_body_at) {
    Frame frame(c, 0x100e05c0, 0x60, {.first_gpr = 30});
    const auto list = frame.at(0x40), key = frame.at(0x38);
    // r30 is the manager throughout and r31 the argument, then the walk's entry pointer from the search on:
    // the generated callees (the list object's, bsearch 0x10775600) save them in their frames, and a ruling
    // body later copies a ban struct it never fully wrote from that stack into a ban record (the
    // BlockMoveData reader 0x10bf7bbc that tools/stack_contract.py lists for 0x100e05c0).
    c.set({{30, manager}, {31, ruling_body_at}});
    c.set(3, list);
    c.call(callee::list_construct, 0x100e05e4);
    c.put32(key, 0xffffffff);
    c.put32(key + 4, c.u32(c.u32(ruling_body_at + ruling_body_key_source) + person_record::key));
    c.set({{3, key}, {4, c.u32(manager + sorted)}, {5, std::uint32_t(std::int32_t(c.i16(manager + sorted_count)))},
           {6, sorted_entry_size}, {7, c.toc(toc_slot::comparator)}});
    c.call(callee::bsearch, 0x100e0610);
    auto entry = c.r(3);
    c.set(31, entry);
    if (!entry) {
        c.set({{3, list}, {4, 0xffffffff}});
        c.call(callee::list_release, 0x100e0624);
        return;
    }
    const auto same_key = [&](std::uint32_t at, std::uint32_t back) {
        c.set({{3, key}, {4, at}, {31, at}});
        discipline::call(c, callee::compare_entries, back);
        return c.r(3) == 0;
    };
    // The entries with this key are adjacent: walk back to the first, then forward over all of them.
    while (entry > c.u32(manager + sorted)) {
        entry -= sorted_entry_size;
        if (!same_key(entry, 0x100e063c)) { entry += sorted_entry_size; break; }
    }
    while (entry <= c.u32(manager + sorted) + (std::uint32_t(std::int32_t(c.i16(manager + sorted_count)) - 1) << 3) &&
           same_key(entry, 0x100e0668)) {
        c.set({{3, list}, {4, entry}, {5, 4}, {31, entry}});
        c.call(callee::list_append, 0x100e0680);
        entry += sorted_entry_size;
    }
    c.put32(frame.at(0x4c), c.u32(frame.at(0x44)));  // the list's cursor starts at its head
    c.set(31, entry);
    while (true) {
        c.set(3, list);
        c.call(callee::list_next, 0x100e06cc);
        const auto item = c.r(3);
        if (!item) break;
        c.link(0x100e06c4);
        serve_ban(c, manager, c.u32(item), 0, 1);
    }
    c.set({{3, list}, {4, 0xffffffff}});
    c.call(callee::list_release, 0x100e06e0);
}
}
