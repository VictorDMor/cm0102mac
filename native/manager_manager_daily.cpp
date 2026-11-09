// MANAGER_MANAGER.CPP daily update block (manager_manager_daily.hpp): 0x10196390, the module's
// day, with its four helpers 0x10197650, 0x101976b0, 0x10197960 and 0x101b5090.
//
// Every routine keeps the original's stack image: the prologue stores, the date copies and the
// event record the day builds in its frame, and the registers the original holds at each call (the
// callee's prologue saves r13-r31) and leaves at its end (r3-r10, cr0). The day reads the word at
// sp+0xb4 of its own frame, whose last three bytes it never writes: they are what an earlier
// callee of its caller left there, so the read goes to guest memory every time.
#include "manager_manager_daily.hpp"
#include "guest_values.hpp"
#include "manager_manager_detail.hpp"
#include "manager_manager_daily_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
using guest::Frame;
using guest::Kept;
using guest::compare;
using guest::compare_logical;
using guest::nonzero;
using guest::nonzero_logical;
using namespace daily_detail;
namespace {
namespace callee {
constexpr std::uint32_t date_copy = 0x10717b20, month_of = 0x10718360, club_flag = 0x100c7d00, monthly = 0x101b7830,
    monthly_end = 0x101acf00, weekly = 0x101a3350, daily_review = 0x101aa510, list_next = 0x10738e00,
    list_remove = 0x10738e60, days_between = 0x10718130, make_date = 0x107156f0, rematch = 0x101aaa90,
    refuse = 0x101b7bc0, staff_gate = 0x10083110, club_gate = 0x100c96e0, board_gate = 0x103f5e50,
    club_test = 0x100ce250, bad_run = 0x100d3c40, retire_old = 0x101b5b20, post_notice = 0x10084a80,
    date_default = 0x10715880, event_post = 0x103f2d50, club_state = 0x100c9070, notice = 0x10078e70,
    employer_record = 0x10103510, level_of = 0x100f55b0, link_a = 0x104ade40, link_b = 0x104ae570,
    link_c = 0x104ad840, human_a = 0x1030f950, human_b = 0x103d2860, late_offer = 0x101b1720,
    settle = 0x1019dd00, finish = 0x101c0fc0, deal_of = 0x10082f20, side_news = 0x10308350, club_news = 0x10365590,
    folder = 0x103afb90, message_new = 0x102bdf90, message_set = 0x102cb4b0, message_pair = 0x102cb630,
    message_send = 0x102cc8e0, message_post = 0x102cc400;
}
namespace flag {
// The static bytes the day keeps beside the table of contents (r2 - 0x3940 and up): whether it
// has run, whether the next month check is the first, and the last month's number.
constexpr std::int32_t ran = -0x3940, first = -0x393f, month = -0x393e;
}

std::int32_t sbyte(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
std::uint32_t word(std::int32_t value) { return std::uint32_t(value); }
// The first human manager's id: the people count less the human slots, as `subi` computes it (wrapping).
std::int32_t floor_of(std::uint32_t count) { return std::int32_t(count - person::human_slots); }
std::uint32_t post(std::int32_t base, std::uint32_t index, std::uint32_t size) { return word(base) + index * size; }

// The registers one site of the original's date compare uses: the two years, and (equal years)
// the first date's day and the halved xor of the two days.
struct DateRegs {
    unsigned a_year, b_year, a_day, shift;
};
// The original's date compare, inlined at five sites: cmplw of the years, then (equal years) the
// halfword-signed trick for the days; the result is a byte tested with extsb. Answers whether the
// date at `a` is after the date at `b`; cr0 and the registers are left as the original leaves them.
bool date_after(Cpu& c, std::uint32_t a, std::uint32_t b, DateRegs regs) {
    const auto year_a = c.u16(a + date::year), year_b = c.u16(b + date::year);
    c.set(regs.b_year, year_b);
    c.set(regs.a_year, year_a);
    compare_logical(c, year_a, year_b);
    std::int32_t result;
    if (year_a > year_b) {
        result = 1;
    } else if (year_a < year_b) {
        result = 0;
    } else {
        const auto day_a = c.i16(a + date::day), day_b = c.i16(b + date::day);
        c.set(regs.a_day, word(day_a));
        c.set(regs.shift, word((day_a ^ day_b) >> 1));
        result = day_a > day_b ? 1 : 0;
    }
    compare(c, result);  // extsb. r0,r0
    return result != 0;
}
constexpr DateRegs humans_regs{6, 5, 6, 5}, due_regs{5, 4, 4, 3}, reminder_regs{4, 3, 4, 3}, message_regs{3, 5, 4, 3};
// A date copied halfword, halfword, byte.
void copy_date(Cpu& c, std::uint32_t to, std::uint32_t from) {
    c.put16(to + date::year, c.u16(from + date::year));
    c.put16(to + date::day, c.u16(from + date::day));
    c.put8(to + date::extra, c.u8(from + date::extra));
}
// The flag routine on a club (cr0 left by its extsb.).
bool flagged(Cpu& c, std::uint32_t club, std::uint32_t back) {
    c.set(3, club);
    c.call(callee::club_flag, back);
    return nonzero(c, sbyte(c.r(3)));
}
// Whether the first club-job status leaves the post in place (kept), and the new status:
// 12 becomes 5, 13 becomes 6 and 15 becomes 8 (all kept); every other status becomes 11 and the
// link is cleared. The compares are the original's chain of cmpwi.
struct Retired {
    std::int32_t status;
    bool keep;
};
Retired retired(Cpu& c, std::int32_t value) {
    compare(c, value, 0xe);
    if (value == 0xe) return {0xb, false};
    if (value > 0xe) {
        compare(c, value, 0x10);
        return value >= 0x10 ? Retired{0xb, false} : Retired{8, true};
    }
    compare(c, value, 0xc);
    if (value == 0xc) return {5, true};
    return value > 0xc ? Retired{6, true} : Retired{0xb, false};
}
// Clears every entry of `count` words at club + `base` that holds `person`.
void clear_entries(Cpu& c, std::uint32_t club, std::uint32_t base, unsigned count, std::uint32_t person) {
    for (unsigned n = 0; n < count; ++n)
        if (c.u32(club + base + 4 * n) == person) c.put32(club + base + 4 * n, 0);
}

// The day. The original keeps in registers: the object r15, the date r25, the tables' cells (clubs
// r26, people count r27, people r28), the success flag and message r29/r30, the pool r31 and the
// games' cells r13, r14, r20-r24; r16 and r17 are first the club count cell and the walk's club,
// then the offer's club and person; r18 the walk's index, then the person's previous employer;
// r19 the offer.
class Daily {
public:
    Daily(Cpu& c, Frame& frame, std::uint32_t object)
        : c(c), frame(frame), object(object), r16(c, c.toc(toc::club_count)), r17(c, c.r(17)), r18(c, c.r(18)),
          r19(c, c.r(19)), date(c.toc(toc::date)), clubs_cell(c.toc(toc::clubs)), count_cell(c.toc(toc::people_count)),
          people_cell(c.toc(toc::people)), pool(c.toc(toc::pool)) {
        guest::keep(c, {{20, c.toc(-0x7a7c)}, {21, c.toc(-0x7b1c)}, {14, c.toc(-0x7ab8)}, {22, c.toc(-0x7a98)},
                        {13, c.toc(-0x7a14)}, {23, c.toc(toc::news)}, {24, c.toc(toc::deals)}, {25, date},
                        {26, clubs_cell}, {27, count_cell}, {28, people_cell}, {29, c.toc(toc::success)},
                        {30, c.toc(toc::message)}, {31, pool}, {15, object}});
    }

    void run() {
        month_check();
        human_flags();
        weekly();
        for (;;) {
            c.set(3, c.u32(object + object::jobs));
            c.call(callee::list_next, 0x10197624);
            r19 = c.r(3);
            if (!nonzero(c, std::int32_t(c.r(3)))) break;  // or. r19,r3,r3
            offer();
        }
        c.set(3, object);
        c.call(callee::finish, 0x10197634);
        lmw_leaves(c, frame.caller());
    }

private:
    std::uint32_t at_r2(std::int32_t offset) const { return c.r(2) + word(offset); }
    std::uint32_t at(std::uint32_t offset) const { return frame.at(std::int32_t(offset)); }
    std::uint32_t rows() const { return c.u32(object + object::rows); }
    std::uint32_t people() const { return c.u32(people_cell); }
    // The human managers start `human_slots` before the people table's end.
    // `subi` wraps: computed on 32-bit words.
    std::int32_t humans_from() const { return floor_of(c.u32(count_cell)); }

    // ---- The month check (0x101963e0..0x101964c8). ----

    void month_check() {
        const auto ran = sbyte(c.u8(at_r2(flag::ran)));
        compare(c, ran);  // extsb. r0,r0
        if (ran == 0) {
            c.put8(at_r2(flag::first), 1);
            c.put8(at_r2(flag::ran), 1);
        }
        c.set({{3, at(0x98)}, {4, date}, {5, 0xb4}});
        c.call(callee::date_copy, 0x10196408);
        // The date, copied to the slot the human managers' loop reads as a word pair (the second
        // word's last three bytes are not written here).
        const auto first = sbyte(c.u8(at_r2(flag::first)));
        const auto day = c.u16(at(0x98)), year = c.u16(at(0x9a));
        c.set(4, word(std::int16_t(day)));  // lha r4,0x98(r1)
        c.set(3, year);
        compare(c, first);  // extsb. r0,r0
        const auto extra = c.u8(at(0x9c));
        c.put16(at(0xb0), day);
        c.put16(at(0xb2), year);
        c.put8(at(0xb4), extra);
        if (first != 0) {
            c.set(3, date);
            c.call(callee::month_of, 0x10196434);
            c.put8(at_r2(flag::month), c.r(3));
            c.put8(at_r2(flag::first), 0);
            return;
        }
        c.set(3, date);
        c.call(callee::month_of, 0x1019644c);
        const auto kept = sbyte(c.u8(at_r2(flag::month))), now = sbyte(c.r(3));
        c.set(3, word(now));
        compare(c, kept, now);
        if (kept == now) return;
        c.set(3, date);
        c.call(callee::month_of, 0x10196468);
        c.put8(at_r2(flag::month), c.r(3));
        r18 = 0;
        r17 = c.u32(clubs_cell);
        for (;;) {
            const auto count = c.i32(c.toc(toc::club_count));
            compare(c, std::int32_t(std::uint32_t(r18)), count);
            if (!(std::int32_t(std::uint32_t(r18)) < count)) break;
            bool work = !flagged(c, r17, 0x10196480);
            if (!work) {
                const auto region = c.u32(std::uint32_t(r17) + club::region);
                c.set(3, region);
                if (nonzero_logical(c, region)) work = nonzero_logical(c, c.u32(region + region::monthly));
            }
            if (work) {
                c.set({{3, object}, {4, r17}});
                c.call(callee::monthly, 0x101964ac);
            }
            r18 = std::uint32_t(r18) + 1;
            r17 = std::uint32_t(r17) + club::size;
        }
        c.set(3, object);
        c.call(callee::monthly_end, 0x101964c8);
    }

    // The last 16 people are human managers: one with a post and a flag byte whose date has passed
    // loses its club's row flag 0x80. The loop reads the date as the word pair at sp+0xb0/0xb4.
    void human_flags() {
        const auto first_offset = word(humans_from()) * person::size;
        auto who = people() + first_offset;
        c.set(3, first_offset);
        c.set(4, who);
        for (unsigned n = 0; n < person::human_slots; ++n, who += person::size, c.set(4, who)) {  // addi r4,r4,0x70
            const auto posted = c.u32(who + person::club_post);
            c.set(3, posted);
            if (!nonzero_logical(c, posted)) continue;
            if (!nonzero(c, sbyte(c.u8(who + person::human)))) continue;
            const auto first = c.u32(at(0xb0)), second = c.u32(at(0xb4));
            c.set(5, first);
            c.put32(at(0x50), first);
            c.put32(at(0x54), second);
            if (!date_after(c, at(0x50), who + person::club_since, humans_regs)) continue;
            const auto id = c.u32(posted + club::id);
            c.set(6, 0);
            c.set(5, rows());
            c.set(3, id * row::size + row::flags);
            const auto flags = rows() + id * row::size + row::flags;
            c.put8(flags, c.u8(flags) & ~row::flag_open);
        }
    }

    // Every seventh day: 0x101a3350; then the daily review, and the offers' cursor back to the head.
    void weekly() {
        const auto day = c.i16(date + date::day);
        c.set(4, word(day));
        const auto rest = day % 7;
        compare(c, rest);
        if (rest == 0) {
            c.set(3, object);
            c.call(callee::weekly, 0x101965b4);
        }
        c.set(3, object);
        c.call(callee::daily_review, 0x101965bc);
        const auto jobs = c.u32(object + object::jobs);
        c.put32(jobs + list::cursor, c.u32(jobs + list::head));
    }

    // ---- One offer of the list (0x101965cc..0x1019761c). ----

    void offer() {
        const auto job = std::uint32_t(r19);
        const auto high = c.u32(date), low = c.u32(date + 4);
        c.set(3, high);
        c.put32(at(0x48), high);
        c.put32(at(0x38), high);
        c.put32(at(0x3c), low);
        c.put32(at(0x4c), low);
        if (date_after(c, at(0x48), job + job::decide, due_regs)) first_date(job);
        else second_date(job);
    }

    // Past its second date: asked once, and only when the club has no manager.
    void second_date(std::uint32_t job) {
        const auto active = sbyte(c.u8(job + job::active));
        compare(c, active);
        if (active == 0) return;
        c.set(3, c.u32(at(0x38)));
        c.put32(at(0x40), c.u32(at(0x38)));
        c.put32(at(0x44), c.u32(at(0x3c)));
        if (!date_after(c, at(0x40), job + job::reminder, reminder_regs)) return;
        if (nonzero(c, sbyte(c.u8(job + job::asked)))) return;
        const auto candidate = c.i32(job + job::candidate);
        compare(c, candidate, -1);
        if (candidate == -1) return;
        c.put8(job + job::asked, 1);
        const auto club_now = post(c.i32(clubs_cell), c.u32(job + job::club), club::size);
        c.set(3, c.u32(clubs_cell));
        c.set(5, club_now);
        if (nonzero_logical(c, c.u32(club_now + club::manager))) return;
        c.set({{3, object}, {4, post(c.i32(people_cell), word(candidate), person::size)}, {5, club_now}});
        c.call(callee::late_offer, 0x1019761c);
    }

    // The offer's club-row stamp: a placeholder date built on the frame and copied to the row.
    // The registers it leaves are the original's, which differ at three sites (`Shape`).
    enum class Shape { job_rows, wide, plain };
    void stamp(std::uint32_t slot, std::uint32_t back, Shape shape = Shape::job_rows) {
        c.set({{3, at(slot)}, {4, 0x17}, {5, 7}, {6, 0x7b3}, {7, 0xffffffffu}});
        c.call(callee::make_date, back);
        const auto base = rows();
        const auto where = base + c.u32(std::uint32_t(r19) + job::club) * row::size + row::stamp;
        switch (shape) {
        case Shape::job_rows: c.set({{4, r19}, {5, base}, {3, where}}); break;
        case Shape::wide: c.set({{3, object}, {7, base}, {4, r16}, {5, r17}, {6, where}}); break;
        case Shape::plain: c.set({{4, base}, {3, where}}); break;
        }
        c.put16(where + date::year, c.u16(at(slot + 2)));
        c.put16(where + date::day, c.u16(at(slot)));
        c.put8(where + date::extra, c.u8(at(slot + 4)));
    }
    // The offer leaves the list.
    void drop(std::uint32_t back) {
        c.set({{3, c.u32(object + object::jobs)}, {4, r19}});
        c.call(callee::list_remove, back);
    }
    void rematch(std::uint32_t club_arg, std::uint32_t person_arg, std::uint32_t back) {
        c.set({{3, object}, {4, club_arg}, {5, person_arg}});
        c.call(callee::rematch, back);
    }
    // The helpers, called directly (the original's `bl`).
    void end_post_call(std::uint32_t back) {
        guest::enter(c, back, {object, r17, r18});
        end_post(c, object, r17, r18);
    }
    void clear_post_call(std::uint32_t back) {
        guest::enter(c, back, {object, r17, r18});
        clear_post(c, object, r17, r18);
    }
    void announce_call(std::uint32_t employer, std::uint32_t kind, std::uint32_t back) {
        guest::enter(c, back, {object, r17, employer, r16, kind});
        announce(c, object, r17, employer, r16, kind);
    }
    void notice_call(std::uint32_t back) {
        c.set({{3, c.u32(c.r(24))}, {4, r17}});
        c.call(callee::post_notice, back);
    }
    bool has_staff() { return nonzero_logical(c, c.u32(std::uint32_t(r17) + person::staff)); }
    bool has_career() { return nonzero_logical(c, c.u32(std::uint32_t(r17) + person::career)); }

    // Past its first date: the person and the club of the offer.
    void first_date(std::uint32_t job) {
        const auto who = c.u32(job + job::person), club_index = c.u32(job + job::club);
        const auto table = c.u32(people_cell), clubs = c.u32(clubs_cell);
        const auto count = c.i32(count_cell);
        r17 = table + who * person::size;
        const auto floor = floor_of(word(count));
        r16 = clubs + club_index * club::size;
        const auto id = c.i32(r17);
        c.set({{4, club_index * club::size}, {6, table}, {3, word(id)}, {5, clubs}});
        compare(c, id, floor);  // cmpw r3,r0
        if (id >= floor) manager_offer();
        else ordinary_offer();
    }

    // A human manager's offer.
    void manager_offer() {
        if (!nonzero(c, sbyte(c.u8(std::uint32_t(r17) + person::human)))) {
            stamp(0x90, 0x10196688);
            drop(0x101966c0);
            rematch(r16, r17, 0x101966d0);
            return;
        }
        const auto posted = c.u32(std::uint32_t(r17) + person::club_post);
        if (nonzero_logical(c, posted)) {
            c.set({{3, date}, {4, c.u32(std::uint32_t(r17) + person::club_since)},
                   {5, c.u32(std::uint32_t(r17) + person::club_since + 4)}});
            c.call(callee::days_between, 0x101966f0);
            const auto days = std::int32_t(c.r(3));
            compare(c, days, 0x5a);
            if (days < 0x5a) {
                stamp(0x88, 0x10196710, Shape::wide);
                rematch(r16, r17, 0x1019674c);
                drop(0x10196758);
                return;
            }
        }
        c.set({{3, object}, {4, r16}, {5, r17}});
        c.call(callee::refuse, 0x1019676c);
        drop(0x10196778);
        rematch(r16, r17, 0x10196788);
    }

    // An ordinary person's offer.
    void ordinary_offer() {
        if (!has_staff()) {
            stamp(0x80, 0x101967b0);
            drop(0x101967e8);
            rematch(r16, r17, 0x101967f8);
            return;
        }
        c.set({{3, c.u32(c.r(24))}, {4, r17}});
        c.call(callee::staff_gate, 0x10196808);
        if (nonzero_logical(c, c.r(3))) {
            stamp(0x78, 0x10196828);
            drop(0x10196860);
            rematch(r16, 0, 0x10196870);
            return;
        }
        const auto table = c.u32(c.toc(-0x7b9c));
        const auto key = c.u32(std::uint32_t(r17) + person::id) * 0x64;
        c.set({{4, table}, {3, key}});
        const auto bits = (c.u8(table + key + 0xc) >> 2) & 3;
        compare(c, std::int32_t(bits));  // rlwinm.
        if (bits != 0 && (!has_staff() || !has_career())) {
            stamp(0x70, 0x101968c4);
            drop(0x101968fc);
            rematch(r16, 0, 0x1019690c);
            return;
        }
        c.set(3, r16);
        c.call(callee::club_gate, 0x10196918);
        if (nonzero_logical(c, c.r(3))) {
            stamp(0x68, 0x10196938);
            drop(0x10196970);
            return;
        }
        c.set({{3, c.u32(c.r(23))}, {4, r17}, {5, 0}});
        c.call(callee::board_gate, 0x10196984);
        if (nonzero(c, sbyte(c.r(3)))) {
            stamp(0x60, 0x101969a4);
            drop(0x101969dc);
            return;
        }
        choose_employer();
        retire_employer();
        stamp(0x58, 0x10196b24, Shape::plain);
        if (!has_staff()) {
            // The original's assertion (line 0x5e2); it carries on.
            guest::report_assertion(c, {.at = 0x10196b70, .file = pool + 0x37, .format = pool + 0x4b,
                                        .title = pool + 0x58, .name = at(0xd8), .extension = at(0xa8), .line = 0x5e2});
        }
        if (!flagged(c, r16, 0x10196bb4)) club_side();
        else flagged_side();
        appoint();
    }

    // The person's employer for the move (r18), by the club's flag.
    std::uint32_t person_now() const { return post(c.i32(people_cell), c.u32(std::uint32_t(r19) + job::person), person::size); }
    // `lwz r0,8(r19); lwz r4,0(r28); mulli r3,r0,0x70`: the offset and the table, left in r3/r4.
    std::uint32_t person_offset() {
        const auto offset = c.u32(std::uint32_t(r19) + job::person) * person::size;
        c.set({{3, offset}, {4, c.u32(people_cell)}});
        return offset;
    }
    void choose_employer() {
        if (flagged(c, r16, 0x101969e8)) {
            const auto who = person_now();
            c.set(3, who);
            const auto side = c.u32(who + person::side_post);
            c.set(4, side);
            if (nonzero_logical(c, side)) {
                const auto status = std::int32_t(c.u8(who + person::side_status));
                compare(c, status, 0xb);
                if (status != 0xb) {
                    r18 = side;
                    return;
                }
            }
        }
        if (!flagged(c, r16, 0x10196a28)) {
            const auto posted = c.u32(c.u32(people_cell) + person_offset() + person::club_post);
            if (nonzero_logical(c, posted)) {
                r18 = posted;
                return;
            }
        }
        const auto count = c.u32(count_cell);
        const std::int32_t id = c.i32(r17), floor = floor_of(word(count));
        c.set({{3, count}, {4, word(id)}});
        compare(c, id, floor);
        if (id < floor && flagged(c, r16, 0x10196a70)) {
            c.set(3, r16);
            c.call(callee::club_test, 0x10196a80);
            if (!nonzero(c, sbyte(c.r(3)))) {
                const auto reputation = c.i16(std::uint32_t(r16) + club::reputation);
                compare(c, reputation, 0x1770);
                if (reputation > 0x1770) {
                    const auto posted = c.u32(c.u32(people_cell) + person_offset() + person::club_post);
                    if (nonzero_logical(c, posted)) {
                        r18 = posted;
                        return;
                    }
                }
            }
        }
        r18 = 0;
    }

    // The previous employer, when this person is his manager and he is in a bad run, is let go.
    void retire_employer() {
        if (!nonzero_logical(c, r18)) return;
        const auto manager = c.u32(std::uint32_t(r18) + club::manager);
        compare_logical(c, manager, r17);
        if (manager != std::uint32_t(r17)) return;
        c.set(3, r18);
        c.call(callee::bad_run, 0x10196ad8);
        if (!nonzero(c, sbyte(c.r(3)))) return;
        c.set({{3, object}, {4, r18}, {5, 1}});
        c.call(callee::retire_old, 0x10196af0);
        rematch(r18, person_now(), 0x10196b0c);
    }

    // The switch value of a status byte: v - 5 (unsigned; the default is above 10).
    std::uint32_t case_of(std::int32_t status) {
        c.set(3, word(status));
        const auto index = word(status - 5);
        compare_logical(c, index, 10);
        return index;
    }
    void set_status(std::uint32_t offset, std::uint32_t value) { c.put8(std::uint32_t(r17) + offset, value); }
    // The club is an ordinary club (flag clear).
    void club_side() {
        if (nonzero_logical(c, r18)) {
            const auto posted = c.u32(std::uint32_t(r17) + person::club_post);
            compare_logical(c, r18, posted);
            if (std::uint32_t(r18) == posted) {
                club_post_cases();
                notice_call(0x10196dd4);
                return;
            }
        }
        const auto id = c.i32(r17), floor = humans_from();
        c.set({{3, c.u32(count_cell)}, {4, word(id)}});  // 0x10196dd8..0x10196ddc
        compare(c, id, floor);
        if (id < floor && nonzero_logical(c, r18)) {
            const auto posted = c.u32(std::uint32_t(r17) + person::side_post);
            compare_logical(c, r18, posted);
            if (std::uint32_t(r18) == posted) {
                side_post_cases();
                notice_call(0x10196fb0);
                return;
            }
        }
        announce_call(0, 2, 0x10196fcc);
    }

    // The person's club post, by its status (0x10196bd0..0x10196dc8).
    void club_post_cases() {
        const auto status = sbyte(c.u8(std::uint32_t(r17) + person::club_status));
        const auto index = case_of(status);
        if (index > 10) return club_post_default();
        switch (index) {
        case 10:
            if (has_career()) end_post_call(0x10196c14);
            if (has_staff()) clear_post_call(0x10196c30);
            announce_call(r18, 2, 0x10196c48);
            break;
        case 6:
            if (has_career()) end_post_call(0x10196c68);
            announce_call(r18, 1, 0x10196c80);
            break;
        case 7:
            if (has_career()) end_post_call(0x10196ca0);
            if (has_staff()) clear_post_call(0x10196cbc);
            announce_call(r18, 3, 0x10196cd4);
            break;
        case 1:
        case 3:
            if (has_staff()) clear_post_call(0x10196cf4);
            announce_call(r18, 2, 0x10196d0c);
            break;
        case 0:
            if (has_staff()) clear_post_call(0x10196d2c);
            announce_call(r18, 3, 0x10196d44);
            break;
        default: club_post_default(); break;
        }
    }
    void club_post_default() {
        if (has_staff()) clear_post_call(0x10196d64);
        else set_status(person::club_status, 0xb);
        if (has_career()) end_post_call(0x10196d8c);
        else set_status(person::club_status, has_staff() ? 8 : 0);
        announce_call(r18, 2, 0x10196dc8);
    }

    // The person's side post, by its status (0x10196e00..0x10196fa4). The default writes the
    // club status byte (+0x62) in this variant too.
    void side_post_cases() {
        const auto status = sbyte(c.u8(std::uint32_t(r17) + person::side_status));
        const auto index = case_of(status);
        if (index > 10) return side_post_default();
        switch (index) {
        case 10:
            if (has_staff()) clear_post_call(0x10196e44);
            announce_call(r18, 2, 0x10196e5c);
            break;
        case 6: announce_call(r18, 1, 0x10196e78); break;
        case 7:
            if (has_staff()) clear_post_call(0x10196e98);
            announce_call(r18, 3, 0x10196eb0);
            break;
        case 1:
        case 3:
            if (has_staff()) clear_post_call(0x10196ed0);
            announce_call(r18, 2, 0x10196ee8);
            break;
        case 0:
            if (has_staff()) clear_post_call(0x10196f08);
            announce_call(r18, 3, 0x10196f20);
            break;
        default: side_post_default(); break;
        }
    }
    void side_post_default() {
        if (has_staff()) clear_post_call(0x10196f40);
        else set_status(person::club_status, 0xb);
        if (has_career()) end_post_call(0x10196f68);
        else set_status(person::club_status, has_staff() ? 8 : 0);
        announce_call(r18, 2, 0x10196fa4);
    }

    // The club is flagged (0x10196fd0..0x10197274).
    void flagged_side() {
        if (nonzero_logical(c, r18)) {
            const auto side = c.u32(std::uint32_t(r17) + person::side_post);
            compare_logical(c, r18, side);
            bool mine = std::uint32_t(r18) == side;
            if (!mine) {
                const auto posted = c.u32(std::uint32_t(r17) + person::club_post);
                compare_logical(c, r18, posted);
                mine = std::uint32_t(r18) == posted;
            }
            if (mine) {
                flagged_cases();
                if (!flagged(c, r18, 0x10197244)) notice_call(0x10197258);
                return;
            }
        }
        announce_call(0, 2, 0x10197274);
    }
    void flagged_cases() {
        const auto status = sbyte(c.u8(std::uint32_t(r17) + person::side_status));
        const auto index = case_of(status);
        if (index > 10) return flagged_default();
        switch (index) {
        case 10:
            if (has_career()) {
                end_post_call(0x10197034);
            } else {
                // Line 0x6bb: a moved person with no career record.
                guest::report_assertion(c, {.at = 0x1019704c, .file = pool + 0x37, .format = pool + 0x4b,
                                            .title = pool + 0x58, .name = at(0xb8), .extension = at(0xa0),
                                            .line = 0x6bb});
            }
            if (has_staff()) clear_post_call(0x101970a4);
            announce_call(r18, 2, 0x101970bc);
            break;
        case 6:
            if (has_career()) end_post_call(0x101970dc);
            announce_call(r18, 1, 0x101970f4);
            break;
        case 7:
            if (has_career()) end_post_call(0x10197114);
            if (has_staff()) clear_post_call(0x10197130);
            announce_call(r18, 2, 0x10197148);
            break;
        case 1:
        case 3:
            if (has_staff()) clear_post_call(0x10197168);
            announce_call(r18, 2, 0x10197180);
            break;
        case 0:
            if (has_staff()) clear_post_call(0x101971a0);
            announce_call(r18, 3, 0x101971b8);
            break;
        default: flagged_default(); break;
        }
    }
    void flagged_default() {
        if (has_staff()) clear_post_call(0x101971d8);
        else set_status(person::side_status, 0xb);
        if (has_career()) end_post_call(0x10197200);
        else set_status(person::club_status, has_staff() ? 8 : 0);
        announce_call(r18, 2, 0x1019723c);
    }

    // The move itself (0x10197274..0x1019761c): the event is posted, the person takes the post,
    // the club's manager is set, and the offer leaves the list.
    void appoint() {
        const auto person_at = std::uint32_t(r17), club_at = std::uint32_t(r16);
        if (nonzero_logical(c, c.u32(person_at + person::career)) && nonzero_logical(c, r18) &&
            !flagged(c, r18, 0x10197290) && nonzero_logical(c, r16) && !flagged(c, r16, 0x101972a8)) post_event();
        const auto flagged_club = flagged(c, r16, 0x10197360);
        copy_date(c, person_at + (flagged_club ? person::side_since : person::club_since), date);
        c.put32(club_at + club::manager, person_at);
        if (!flagged(c, r16, 0x101973a8)) {
            c.put32(person_at + person::club_post, club_at);
            if (!try_hire()) c.put8(person_at + person::club_status, status::signed_up);
            hired_level();
        } else {
            c.put32(person_at + person::side_post, club_at);
            const auto id = c.u32(club_at + club::id), base = rows();
            c.set({{5, 0}, {3, id * row::size + row::level}});
            c.set(4, base);
            c.put8(base + id * row::size + row::level, 0);
            c.put8(person_at + person::side_status, 5);
        }
        const auto id = c.i32(person_at + person::id);
        const auto floor = humans_from();
        c.set({{3, c.u32(count_cell)}, {4, word(id)}});
        compare(c, id, floor);
        if (id >= floor) {
            c.set({{3, c.u32(c.r(21))}, {4, person_at}});
            c.call(callee::human_a, 0x10197530);
            c.set({{3, c.u32(c.r(20))}, {4, person_at}, {5, r18}, {6, club_at}});
            c.call(callee::human_b, 0x10197544);
        }
        drop(0x10197550);
        c.set({{3, object}, {4, club_at}});
        c.call(callee::settle, 0x1019755c);
    }

    // The event record: the club, the employer, the person and his three places, the date.
    void post_event() {
        const auto person_at = std::uint32_t(r17);
        c.set(3, at(0x118));
        c.call(callee::date_default, 0x101972b8);
        c.put32(at(0x108), c.u32(r16));
        c.put16(at(0x11a), c.u16(date + date::year));
        c.put16(at(0x118), c.u16(date + date::day));
        c.put8(at(0x11c), c.u8(date + date::extra));
        c.put32(at(0x10c), c.u32(r18));
        c.put32(at(0xf8), c.u32(person_at));
        const auto place = [&](std::uint32_t slot, std::uint32_t field) {
            const auto record = c.u32(person_at + field);
            c.put32(at(slot), nonzero_logical(c, record) ? c.u32(record + 0x20) : 0xffffffffu);
        };
        place(0xfc, person::place_a);
        place(0x100, person::place_b);
        place(0x104, person::place_c);
        c.put8(at(0x114), 5);
        c.put32(at(0x110), 0);
        c.set({{3, c.u32(c.r(23))}, {4, at(0xf8)}, {5, 5}});
        c.call(callee::event_post, 0x10197358);
    }

    // Whether the move gets the hired status (12) rather than 5: the person has a career record
    // whose figure plus 0x2ee beats the club's reputation, the club's state is under 0x2d, its
    // region differs from the games' own, and the club takes him as an applicant (0x10197650).
    bool try_hire() {
        const auto person_at = std::uint32_t(r17), club_at = std::uint32_t(r16);
        const auto career = c.u32(person_at + person::career);
        if (!nonzero_logical(c, career)) return false;
        const std::int32_t figure = c.i16(career + 0xa) + 0x2ee, reputation = c.i16(club_at + club::reputation);
        compare(c, figure, reputation);
        if (figure <= reputation) return false;
        c.set({{3, club_at}, {4, 0}, {5, 0}});
        c.call(callee::club_state, 0x101973e4);
        const auto state = sbyte(c.r(3));
        compare(c, state, 0x2d);
        if (state >= 0x2d) return false;
        const auto region_at = c.u32(club_at + club::region);
        if (!nonzero_logical(c, region_at)) return false;
        const auto scope = c.i32(region_at + region::scope), own = c.i32(c.r(13));
        compare(c, scope, own);
        if (scope == own) return false;
        guest::enter(c, 0x1019741c, {object, person_at, club_at});
        add_applicant(c, object, person_at, club_at);
        if (!nonzero(c, sbyte(c.r(3)))) return false;
        c.put8(person_at + person::club_status, status::hired);
        if (!flagged(c, club_at, 0x10197434)) {
            c.set({{3, c.u32(c.r(24))}, {4, person_at}});
            c.call(callee::post_notice, 0x10197448);
            c.set({{3, c.u32(c.r(24))}, {4, person_at}, {5, c.u32(person_at + person::extra)}});
            c.call(callee::notice, 0x10197458);
            c.set({{3, c.u32(c.r(22))}, {4, person_at}, {5, club_at}, {6, 0}});
            c.call(callee::link_a, 0x1019746c);
            c.set({{3, c.u32(c.r(22))}, {4, club_at}, {5, 0}});
            c.call(callee::link_b, 0x1019747c);
            if (nonzero(c, sbyte(c.r(3)))) {
                c.set({{3, c.u32(c.r(22))}, {4, club_at}, {5, 0}});
                c.call(callee::link_c, 0x10197494);
            }
        }
        return true;
    }
    // The club's employer record: its level byte goes to the club's row (0 when there is none).
    void hired_level() {
        const auto club_at = std::uint32_t(r16);
        c.set({{3, c.u32(c.r(14))}, {4, club_at}});
        c.call(callee::employer_record, 0x101974ac);
        const auto record = c.r(3);
        const auto level = [&](std::uint32_t value, bool from_record) {
            const auto id = c.u32(club_at + club::id), base = rows();
            if (from_record) c.set({{5, base}, {4, id * row::size}});
            else c.set({{5, 0}, {4, base}, {3, id * row::size}});
            c.put8(base + id * row::size + row::level, value);
        };
        if (nonzero_logical(c, record)) {
            c.set(3, record);
            c.call(callee::level_of, 0x101974b8);
            level(c.r(3), true);
        } else {
            level(0, false);
        }
    }

    Cpu& c;
    Frame& frame;
    std::uint32_t object;
    Kept<16> r16;
    Kept<17> r17;
    Kept<18> r18;
    Kept<19> r19;
    std::uint32_t date, clubs_cell, count_cell, people_cell, pool;
};
}  // namespace

void daily(Cpu& c, std::uint32_t object) {
    Frame frame(c, 0x10196390, 0x170, {.first_gpr = 13});
    Daily(c, frame, object).run();
}

// 0x10197650. No frame, no calls: the first free of the club's 50 applicant slots.
std::uint32_t add_applicant(Cpu& c, std::uint32_t, std::uint32_t person, std::uint32_t club) {
    std::int32_t index = 0;
    while (index < std::int32_t(club::applicant_count) && c.u32(club + club::applicants + 4 * word(index)) != 0) ++index;
    compare(c, index, 0x32);
    c.set(6, word(index));
    if (index >= 0x32) return guest::result(c, 0);
    const auto slot = club + 4 * word(index);
    c.set(5, slot);
    c.put32(slot + club::applicants, person);
    return guest::result(c, 1);
}

// 0x101976b0. Frame 0x50 (r28-r31 saved, LR at +8). First the person's club post, then his side
// post; each moves its status on, and the club's applicant slot for him is cleared.
void end_post(Cpu& c, std::uint32_t, std::uint32_t person_at, std::uint32_t club_at) {
    Frame frame(c, 0x101976b0, 0x50, {.first_gpr = 28});
    Kept<31> r31(c, c.toc(-0x7a6c));
    Kept<30> r30(c, c.r(30));
    Kept<29> r29(c, club_at);
    Kept<28> r28(c, person_at);
    const auto clear_applicant = [&] {
        for (unsigned n = 0; n < club::applicant_count; ++n) {
            if (c.u32(club_at + club::applicants + 4 * n) == person_at) {
                c.put32(club_at + club::applicants + 4 * n, 0);
                break;
            }
        }
    };
    const auto folders = [&](std::uint32_t first_back, std::uint32_t second_back) {
        c.set({{3, c.u32(r31)}, {4, club_at}, {5, person_at}, {6, 0}, {7, 1}});
        c.call(callee::folder, first_back);
        c.set({{3, c.u32(r31)}, {4, club_at}, {5, person_at}, {6, 1}, {7, 1}});
        c.call(callee::folder, second_back);
    };
    const auto posted = c.u32(person_at + person::club_post);
    compare_logical(c, posted, club_at);
    if (posted == club_at) {
        const auto next = retired(c, sbyte(c.u8(person_at + person::club_status)));
        r30 = next.keep ? 1 : 0;
        c.put8(person_at + person::club_status, word(next.status));
        c.set({{3, c.u32(c.toc(-0x7b3c))}, {4, person_at}, {5, 0xffffffffu}, {6, 0}, {7, 0}, {8, 0xffffffffu}, {9, 0}});
        c.call(callee::side_news, 0x10197774);
        c.set({{3, c.u32(c.u32(c.toc(toc::news)) + 0x988)}, {4, person_at}, {5, club_at}, {6, 4}, {7, 2}});
        c.call(callee::club_news, 0x10197794);
        folders(0x101977ac, 0x101977c4);
        clear_applicant();
        compare(c, sbyte(r30));
        if (sbyte(r30) == 0) c.put32(person_at + person::club_post, 0);
    }
    const auto side = c.u32(person_at + person::side_post);
    compare_logical(c, side, club_at);
    if (side == club_at) {
        const auto next = retired(c, sbyte(c.u8(person_at + person::side_status)));
        r30 = next.keep ? 1 : 0;
        c.put8(person_at + person::side_status, word(next.status));
        folders(0x101978ac, 0x101978c4);
        clear_applicant();
        compare(c, sbyte(r30));
        if (sbyte(r30) == 0) c.put32(person_at + person::side_post, 0);
    }
    c.set({{3, c.u32(c.toc(toc::deals))}, {4, person_at}});
    c.call(callee::deal_of, 0x1019792c);
    const auto deal = c.r(3);
    if (nonzero_logical(c, deal)) c.put8(deal + 0x5c, 4);
}

// 0x10197960. Frame 0x50 (r28-r31 saved). The club's slots that hold the person are cleared by his
// status; the status tables are 16 wide (a byte above 15, or negative, takes the default). In the
// club-post branch r31 says whether the link stays (the cases set it; the default leaves the
// caller's r31), in the side-post branch r30 does (0 until a case sets it).
void clear_post(Cpu& c, std::uint32_t, std::uint32_t person_at, std::uint32_t club_at) {
    Frame frame(c, 0x10197960, 0x50, {.first_gpr = 28});
    Kept<28> r28(c, person_at);
    Kept<29> r29(c, club_at);
    Kept<30> r30(c, 0);
    Kept<31> r31(c, c.r(31));
    c.set({{3, c.u32(c.toc(toc::deals))}, {4, person_at}});
    c.call(callee::post_notice, 0x10197994);
    const auto zero = [&](std::uint32_t field) { c.put32(club_at + field, 0); };
    const auto zero_if_mine = [&](std::uint32_t field) {
        if (c.u32(club_at + field) == person_at) c.put32(club_at + field, 0);
    };
    const auto posted = c.u32(person_at + person::club_post);
    compare_logical(c, posted, club_at);
    if (posted == club_at) {
        const auto value = sbyte(c.u8(person_at + person::club_status));
        compare_logical(c, word(value), 0xf);
        if (word(value) <= 0xf) {
            switch (value) {
            case 1: zero(club::post_a); r31 = 0; break;
            case 2: clear_entries(c, club_at, club::slots_a, 3, person_at); r31 = 0; break;
            case 5: zero_if_mine(club::manager); r31 = 0; break;
            case 6: zero(club::post_b); r31 = 0; break;
            case 8: clear_entries(c, club_at, club::slots_b, 5, person_at); r31 = 0; break;
            case 9: clear_entries(c, club_at, club::slots_c, 7, person_at); r31 = 0; break;
            case 10: clear_entries(c, club_at, club::slots_d, 3, person_at); r31 = 0; break;
            case 12:
                c.put8(person_at + person::club_status, status::free);
                zero_if_mine(club::manager);
                r31 = 1;
                break;
            case 13:
                c.put8(person_at + person::club_status, status::free);
                r31 = 1;
                zero(club::post_b);
                break;
            case 15:
                c.put8(person_at + person::club_status, status::free);
                clear_entries(c, club_at, club::slots_b, 5, person_at);
                r31 = 1;
                break;
            default: break;  // 0, 3, 4, 7, 11, 14: r31 stays as the caller had it
            }
        }
        compare(c, sbyte(r31));
        if (sbyte(r31) == 0) c.put32(person_at + person::club_post, 0);
    } else {
        const auto side = c.u32(person_at + person::side_post);
        compare_logical(c, side, club_at);
        if (side == club_at) {
            const auto value = sbyte(c.u8(person_at + person::side_status));
            compare_logical(c, word(value), 0xf);
            if (word(value) <= 0xf) {
                switch (value) {
                case 1: zero(club::post_a); break;
                case 2: clear_entries(c, club_at, club::slots_a, 3, person_at); break;
                case 5: zero(club::manager); break;  // unconditionally, unlike the club-post branch
                case 6: zero(club::post_b); break;
                case 8: clear_entries(c, club_at, club::slots_b, 5, person_at); break;
                case 9: clear_entries(c, club_at, club::slots_c, 7, person_at); break;
                case 10: clear_entries(c, club_at, club::slots_d, 3, person_at); break;
                case 12:
                    zero(club::manager);
                    c.put8(person_at + person::side_status, status::free);
                    r30 = 1;
                    break;
                case 13:
                    c.put8(person_at + person::side_status, status::free);
                    r30 = 1;
                    zero(club::post_b);
                    break;
                case 15:
                    c.put8(person_at + person::side_status, status::free);
                    clear_entries(c, club_at, club::slots_b, 5, person_at);
                    r30 = 1;
                    break;
                default: break;
                }
            }
            compare(c, sbyte(r30));
            if (sbyte(r30) == 0) c.put32(person_at + person::side_post, 0);
        }
    }
    c.set({{3, c.u32(c.u32(c.toc(toc::news)) + 0x988)}, {4, person_at}, {5, club_at}, {6, 4}, {7, 2}});
    c.call(callee::club_news, 0x10197d94);
    const auto count = c.i32(c.toc(toc::people_count));
    c.set(3, word(count));
    const auto id = c.i32(person_at + person::id);
    c.set(4, word(id));
    const auto floor = floor_of(word(count));
    compare(c, id, floor);
    if (id < floor) return;
    c.set({{3, c.u32(c.toc(-0x7b1c))}, {4, person_at}});
    c.call(callee::human_a, 0x10197dbc);
    c.set({{3, c.u32(c.toc(-0x7a7c))}, {4, person_at}, {5, club_at}, {6, 0}});
    c.call(callee::human_b, 0x10197dd4);
}

// 0x101b5090. Frame 0x140 (r27-r31 saved). The news of the move: gated on the date and the club,
// built in the message record at sp+0x44 field by field, and sent by the person's kind.
void announce(Cpu& c, std::uint32_t, std::uint32_t person_at, std::uint32_t employer, std::uint32_t club_at,
              std::uint32_t kind) {
    compare(c, std::int32_t(person_at));  // or. r27,r4,r4
    Frame frame(c, 0x101b5090, 0x140, {.first_gpr = 27});
    Kept<27> r27(c, person_at);
    Kept<31> r31(c, c.toc(toc::events));
    const auto today = c.toc(toc::date);
    c.set(8, today);
    Kept<28> r28(c, employer);
    Kept<29> r29(c, club_at);
    Kept<30> r30(c, kind);
    const auto body = [&] {
        if (person_at == 0) return;
        if (!nonzero_logical(c, club_at)) return;
        // The message's date gate: the global date at TOC -0x7a9c copied to the frame; nothing is
        // sent when it is after today.
        const auto cell = c.toc(-0x7a9c);
        c.set(4, cell);
        const auto today_year = c.u16(today + date::year);
        c.set(5, today_year);
        const auto first = c.u32(cell), second = c.u32(cell + 4);
        c.put32(frame.at(0x3c), first);
        c.put32(frame.at(0x40), second);
        if (date_after(c, frame.at(0x3c), today, message_regs)) return;
        compare(c, sbyte(kind), 5);
        if (sbyte(kind) == 5) return;
        // Whether the club's region and league mark it as one that is reported.
        bool reported = false;
        const auto region_at = c.u32(club_at + club::region);
        if (nonzero_logical(c, region_at)) {
            const auto open = sbyte(c.u8(region_at + region::open_flag));
            if (nonzero(c, open)) {
                const auto league = c.u32(club_at + club::league);
                if (nonzero_logical(c, league)) {
                    const auto entry = c.u32(c.u32(c.toc(toc::leagues)) + (c.u32(league) << 2));
                    reported = nonzero_logical(c, entry);
                }
            }
        }
        if (!reported) {
            if (!flagged(c, club_at, 0x101b5178)) return;
            const auto region_now = c.u32(club_at + club::region);
            c.set(3, region_now);
            const auto open = sbyte(c.u8(region_now + region::open_flag));
            if (!nonzero(c, open)) return;
        }
        const auto message = frame.at(0x44);
        c.set({{3, message}, {4, 0x1787}, {5, 0}});
        c.call(callee::message_new, 0x101b51a4);
        const auto field = [&](std::uint32_t index, std::uint32_t value, std::uint32_t back) {
            c.set({{3, message}, {4, index}, {5, value}});
            c.call(callee::message_set, back);
        };
        const auto place_id = [&](std::uint32_t field_offset) { return c.u32(c.u32(person_at + field_offset) + 0x20); };
        field(0, c.u32(person_at), 0x101b51b4);
        field(1, place_id(person::place_a), 0x101b51c8);
        field(2, place_id(person::place_b), 0x101b51dc);
        field(3, place_id(person::place_c), 0x101b51f0);
        if (nonzero_logical(c, employer)) field(4, c.u32(employer), 0x101b5208);
        else field(4, 0xffffffffu, 0x101b521c);
        field(5, c.u32(club_at), 0x101b522c);
        field(6, word(sbyte(kind)), 0x101b523c);
        if (!nonzero_logical(c, employer)) {
            field(7, 0xffffffffu, 0x101b5294);
        } else if (flagged(c, employer, 0x101b524c)) {
            field(7, word(sbyte(c.u8(person_at + person::side_status))), 0x101b5268);
        } else {
            field(7, word(sbyte(c.u8(person_at + person::club_status))), 0x101b5280);
        }
        const auto id = c.i32(person_at + person::id);
        const auto count = c.u32(c.toc(toc::people_count));
        const auto floor = floor_of(count);
        c.set({{3, count}, {4, word(id)}});  // lwz r3 / lwz r4 before the compare (0x101b5294..0x101b52a4)
        compare(c, id, floor);
        if (id >= floor) {
            // A human manager's: the two places go through 0x102cb630 with a byte on the frame.
            c.put8(frame.at(0x38), 8);
            c.set({{3, message}, {4, 8}, {5, c.u32(person_at + person::place_a)}, {6, frame.at(0x38)}});
            c.call(callee::message_pair, 0x101b52c8);
            field(7, word(sbyte(c.u8(frame.at(0x38)))), 0x101b52dc);
            c.set({{3, message}, {4, c.u8(frame.at(0x38))}, {5, c.u32(person_at + person::place_b)},
                   {6, frame.at(0x38)}});
            c.call(callee::message_pair, 0x101b52f0);
            c.set({{3, c.u32(r31)}, {4, message}});
            c.call(callee::message_send, 0x101b52fc);
            return;
        }
        c.set(3, club_at);
        c.call(callee::bad_run, 0x101b5308);
        if (!nonzero(c, sbyte(c.r(3)))) return;
        bool send = flagged(c, club_at, 0x101b5318);
        if (!send) {
            const auto reputation = c.i16(club_at + club::reputation);
            compare(c, reputation, 0x1c52);
            send = reputation > 0x1c52;
        }
        if (send) {
            c.set({{3, c.u32(r31)}, {4, message}});
            c.call(callee::message_send, 0x101b5338);
            return;
        }
        const auto region_last = c.u32(club_at + club::region);
        c.set(5, region_last);
        if (!nonzero_logical(c, region_last)) return;
        c.set({{3, c.u32(r31)}, {4, message}, {5, region_last}});
        c.call(callee::message_post, 0x101b5354);
    };
    body();
    lmw_leaves(c, frame.caller());
}
}  // namespace cm::aot::manager_manager
