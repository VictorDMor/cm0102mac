// MANAGER_MANAGER.CPP rematch block (manager_manager_rematch.hpp): the search for a club's next
// manager (0x101aaa90) and the approach of a person who may move (0x101ac280). They call each other.
//
// The 0x28-byte offer record of 0x101aaa90 lives in its frame (sp+0x40) and is copied whole into the
// object's list at the end: its fields are written where the original writes them (some are never
// written on a path and keep what the frame held), and the bytes between them are whatever the earlier
// callees left on the stack.
#include "manager_manager_assessment.hpp"
#include "manager_manager_daily.hpp"
#include "manager_manager_lists.hpp"
#include "manager_manager_rematch.hpp"
#include "manager_manager_rematch_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
using namespace rematch;
namespace {
constexpr std::int8_t i8(std::uint32_t v) { return std::int8_t(v); }
std::uint32_t person_at(const Cpu& c, std::int32_t index) {
    return c.u32(c.toc(cell::people)) + std::uint32_t(index) * person::size;
}
// A date's halfword sums: the original stores 16 bits.
std::uint16_t h(std::int32_t v) { return std::uint16_t(v); }

// ---- 0x101aaa90 ----
class Vacancy {
public:
    Vacancy(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t leaving, std::uint32_t frame)
        : c(c), draw(c), object(object), club(club), leaving(leaving), rec(frame + 0x40),
          pool_at(c.toc(cell::pool)), pool_str(c.toc(slots::pool)), today(c.toc(cell::today)) {}

    void run() {
        out(c, 0x10715880, 0x101aaae0, {rec + offer::date_a});
        out(c, 0x10715880, 0x101aaae8, {rec + offer::date_b});
        if (i8(out(c, 0x100c7d00, 0x101aaaf8, {club})) == 0 && c.u32(club + club::league) == 0 &&
            c.i16(club + club::reputation) < 0x1388)
            return;
        if (i8(out(c, 0x100c7d00, 0x101aab20, {club})) != 0) {
            if (i8(out(c, 0x100ce250, 0x101aab30, {club})) != 0 && c.i32(c.u32(club + club::region) + 0x6c) <= 0x3e8) return;
            if (i8(out(c, 0x100ce250, 0x101aab50, {club})) == 0 && c.i32(c.u32(club + club::region) + 0x6c) <= 0x64) return;
        }
        // The club id (offer+0x10), the chosen person (-1) first, the club's row reset after them, then the
        // id of the person who leaves (-1 without one).
        c.put32(rec + offer::club, c.u32(club));
        c.put32(rec + offer::chosen, 0xffffffff);
        reset_row();
        c.put32(rec + offer::leaving, leaving ? c.u32(leaving) : 0xffffffff);
        first_search();
        if (!second_pass()) return;  // the assertion of the backward walk
        second_search_if_due();
        notify_leaving();
        date_and_file();
        scout_peers();
    }

private:
    Cpu& c;
    Draw draw;
    std::uint32_t object, club, leaving, rec, pool_at, pool_str, today;

    std::uint32_t row() const { return c.u32(object) + c.u32(club) * row::size; }
    std::int32_t chosen() const { return c.i32(rec + offer::chosen); }
    void choose(std::int32_t index) { c.put32(rec + offer::chosen, std::uint32_t(index)); }
    std::uint32_t chosen_person() const { return person_at(c, chosen()); }
    void put_day(std::uint32_t at, std::int32_t v) { c.put16(rec + at, h(v)); }
    std::int32_t day(std::uint32_t at) const { return c.i16(rec + at); }
    std::uint32_t year(std::uint32_t at) const { return c.u16(rec + at + 2); }
    void put_year(std::uint32_t at, std::uint32_t v) { c.put16(rec + at + 2, v); }

    // The club's row: counters and ratings back to their start (0x101aab6c..0x101aadb8).
    void reset_row() {
        const auto at = [&](std::uint32_t offset) { return row() + offset; };
        for (const std::uint32_t offset : {6u, 8u, 0xau, 0xcu}) c.put16(at(offset), 5000);
        c.put8(at(0), 0);
        c.put16(at(0xe), 0x10);
        for (const std::uint32_t offset : {0x10u, 0x12u, 0x14u, 0x16u}) c.put16(at(offset), 0);
        const auto clear = [&](std::uint32_t offset, std::uint32_t mask) { c.put8(at(offset), c.u8(at(offset)) & ~mask); };
        clear(2, 0x80);
        clear(2, 0x40);
        clear(2, 0x20);
        clear(2, 0x10);
        clear(2, 0x08);
        c.put8(at(4), 0);
        clear(3, 0x78);
        clear(2, 0x04);
        clear(2, 0x02);
        clear(2, 0x01);
        c.put32(at(0x28), 0);
        c.put8(at(5), 0);
        clear(3, 0x80);
        clear(0x40, 0x1f);
    }

    // The three-way status test that lets a person be offered the post (0xb, 0xc, 0xd, 0xf).
    static bool moving_status(std::uint32_t status) {
        return std::int8_t(status) == 0xb || std::uint8_t(status - 0xc) <= 1 || std::int8_t(status) == 0xf;
    }

    // ---- the first walk of the people (0x101aadd0..0x101aafec): the best assessment ----
    void first_search() {
        std::int32_t best = -1000000;
        auto candidate = person_at(c, 0);
        std::uint32_t row_offset = 0;
        for (std::int32_t index = 0; index < people_count(c); ++index, candidate += person::size, row_offset += 100) {
            if (c.u32(candidate + person::staff) == 0) continue;
            if (!(index < people_count(c) - 0x10) && c.i8(candidate + person::flag) == 0) continue;
            if (leaving == candidate) continue;
            if (!(index < people_count(c) - 0x10)) {
                const auto flag = i8(out(c, 0x100c7d00, 0x101aae1c, {club}));
                if (flag != 0 && c.u32(candidate + person::home) != c.u32(club + club::region) &&
                    c.i16(c.u32(candidate + person::staff) + staff::c) < 0xfa0)
                    continue;
            }
            if (!(index < people_count(c) - 0x10) && i8(out(c, 0x100d3c40, 0x101aae5c, {club})) == 0) continue;
            if (((c.u8(c.u32(c.toc(cell::rows)) + row_offset + 0xc) >> 2) & 3) != 0) continue;
            if (out(c, 0x10083110, 0x101aae84, {c.u32(c.toc(cell::object)), candidate}) != 0) continue;
            guest::enter(c, 0x101aae98, {object, candidate});
            if (i8(list_contains(c, object, candidate)) != 0) continue;
            const auto region = c.u32(club + club::region), home = c.u32(candidate + person::home);
            if (region == 0 || home == 0) continue;
            if (region != home && c.i16(c.u32(candidate + person::staff) + staff::b) < 0xea6) continue;
            if (c.i8(region + 0x117) != c.i8(home + 0x117) &&
                (c.i16(c.u32(candidate + person::staff) + staff::c) < 0x109a || c.i16(club + club::reputation) < 0x128e))
                continue;
            c.link(0x101aaf18);
            if (!candidate_willing(c, c.f64(pool_at + pool::bar), candidate, club)) continue;
            if (c.u32(candidate + person::post_a) != 0 && c.u32(candidate + person::job_club) != 0 &&
                moving_status(c.u8(candidate + person::status))) {
                const auto deal = out(c, 0x10082f20, 0x101aaf6c, {c.u32(c.toc(cell::object)), candidate});
                if (deal != 0 && (c.i8(deal + 0x54) != 0 || c.i8(deal + 0x20) == 0)) continue;
            }
            if (i8(out(c, 0x100c7d00, 0x101aaf94, {club})) != 0 && c.u8(candidate + person::status) != 5) continue;
            guest::enter(c, 0x101aafc0, {object, candidate, club, 0, 5});
            const auto score = std::int32_t(assess(c, candidate, club, 0, 5));
            if (score > best) {
                c.put32(rec + offer::score, std::uint32_t(best));
                best = score;
                choose(index);
            }
        }
    }

    // The second half of the choice: the club's own slots, else the walk backward through the people
    // from the end. false when the walk came full circle (the original reports and returns).
    bool second_pass() {
        if (chosen() != -1) return true;
        bool first = true;
        if (const auto slot = c.u32(club + club::first_slot); slot != 0) {
            choose(c.i32(slot));
            return true;
        }
        if (const auto slot = c.u32(club + club::second_slot); slot != 0) {
            choose(c.i32(slot));
            return true;
        }
        std::int32_t walk = 0, start = 0;
        while (chosen() == -1) {
            if (first) {
                first = false;
                walk = people_count(c) - 0x11;
                choose(walk);
                start = walk;
            } else {
                --walk;
                if (walk < 0) walk = people_count(c) - 0x10;
                if (walk == start) {
                    const auto format = i8(out(c, 0x100ce250, 0x101ab070, {club})) != 0 ? 0x13363u : 0x133cbu;
                    out(c, 0x1077b720, format == 0x13363u ? 0x101ab090 : 0x101ab0ac,
                        {c.toc(cell::report), pool_str + format, club + club::name});
                    return false;
                }
                choose(walk);
            }
            const auto rows = c.u32(c.toc(cell::rows));
            if (((c.u8(rows + std::uint32_t(chosen()) * 100 + 0xc) >> 2) & 3) != 0) {
                choose(-1);
                continue;
            }
            if (chosen() >= people_count(c) - 0x10) {
                choose(-1);
                continue;
            }
            auto candidate = chosen_person();
            if (c.u32(candidate + person::staff) == 0) {
                choose(-1);
                continue;
            }
            const auto status = std::int32_t(c.i8(candidate + person::status));
            if (!(status == 0 || status == 0xb || status == 0xc || status == 5 || status == 8 || status == 0xf ||
                  status == 7 || status == 6)) {
                choose(-1);
                continue;
            }
            if (out(c, 0x10083110, 0x101ab170, {c.u32(c.toc(cell::object)), candidate}) != 0) {
                choose(-1);
                continue;
            }
            guest::enter(c, 0x101ab19c, {object, chosen_person()});
            if (i8(list_contains(c, object, chosen_person())) != 0) {
                choose(-1);
                continue;
            }
            candidate = chosen_person();
            if (c.u32(candidate + person::side_club) != 0 && c.u8(candidate + person::side_status) == 5) {
                choose(-1);
                continue;
            }
            // 0x101ab1e4..0x101ab280: a club of the AI (no flag) whose person's own club has a league entry
            // (or a reputation over 0x1964) keeps a person of status 5 out.
            if (!(i8(out(c, 0x100c7d00, 0x101ab1ec, {club})) != 0)) {
                candidate = chosen_person();
                if (const auto owner = c.u32(candidate + person::job_club); owner != 0) {
                    const auto region = c.u32(owner + club::region);
                    bool listed = false;
                    if (region != 0 && c.i8(region + 0x120) != 0) {
                        if (const auto rival = c.u32(owner + 0x30); rival != 0)
                            listed = c.u32(c.u32(c.toc(cell::leagues)) + c.u32(rival) * 4) != 0;
                    }
                    const bool by_reputation = !listed && c.i16(owner + club::reputation) <= 0x1964;
                    if (!by_reputation && c.u8(candidate + person::status) == 5) {
                        choose(-1);
                        continue;
                    }
                }
            }
            c.put32(rec + offer::score, 100);
        }
        return true;
    }

    // ---- the second walk (0x101ab294..0x101ab57c): a free agent by chance ----
    void second_search_if_due() {
        c.put8(rec + offer::flag2, 0);
        c.put8(rec + offer::flag, 0);
        c.put32(rec + offer::second, 0xffffffff);
        bool due = true;
        if (i8(out(c, 0x100c7d00, 0x101ab2b0, {club})) == 0) {
            if (c.i16(club + club::reputation) <= 0x1194) {
                due = false;
            } else {
                const auto x = draw.next(0x101ab2e4);
                due = c.i16(club + club::reputation) > remainder(x, 2000) + 0x1194;
            }
        }
        if (!due) return;
        std::int32_t best = -1000000;
        std::uint32_t row_offset = 0;
        for (std::int32_t index = 0; index < people_count(c); ++index, row_offset += 100) {
            const auto n = people_count(c);
            // 0x101ab340 tests the count for 0 (the original then skips the draw), but the count in r20 is the
            // one the loop test read at 0x101ab570 (cmpw r18,r20; blt: signed, index >= 0) with no call between,
            // so it is at least 1 here: that branch (0x101ab348) cannot run.
            if (n == 0) throw std::logic_error("rematch: a zero count inside the second walk (0x101ab340)");
            const std::int32_t pick = draw.below(n, 0x101ab370, 0x101ab438);
            const auto candidate = person_at(c, pick);
            if (c.u32(candidate + person::staff) == 0) continue;
            if (leaving == candidate) continue;
            if (((c.u8(c.u32(c.toc(cell::rows)) + row_offset + 0xc) >> 2) & 3) != 0) continue;
            c.link(0x101ab4e0);
            if (!candidate_willing(c, c.f64(pool_at + pool::bar_random), candidate, club)) continue;
            if (!(c.i32(candidate) < people_count(c) - 0x10) && c.i8(candidate + person::flag) == 0) continue;
            const auto days = std::int32_t(out(c, 0x10718130, 0x101ab518,
                                                {today, c.u32(candidate + person::since_a), c.u32(candidate + person::since_b)}));
            if (days < 0x96) continue;
            guest::enter(c, 0x101ab52c, {object, candidate});
            if (i8(list_contains(c, object, candidate)) != 0) continue;
            guest::enter(c, 0x101ab54c, {object, candidate, club, 0, 5});
            const auto score = std::int32_t(assess(c, candidate, club, 0, 5));
            if (score > best) {
                best = score;
                c.put8(rec + offer::flag, 1);
                c.put32(rec + offer::second, c.u32(candidate));
            }
        }
    }

    // The leaving manager of a club of the last 16 learns it (0x100da320).
    void notify_leaving() {
        const auto limit = people_count(c) - 0x10;
        if (c.i32(chosen_person()) >= limit && leaving != 0 && c.i32(leaving) < limit)
            out(c, 0x100da320, 0x101ab5c0, {leaving, club, 1});
    }

    // ---- the two dates of the offer and its filing (0x101ab5c0..0x101abcb0) ----
    void date_and_file() {
        if (event_after_today(c) && i8(out(c, 0x100ce250, 0x101ab638, {club})) == 0) later_dates();
        else earlier_dates();
        const auto saved_cursor = c.u32(c.u32(object + 8) + list::cursor);
        out(c, 0x10738d10, 0x101abcc4, {c.u32(object + 8), rec, offer::size});
        c.put32(c.u32(object + 8) + list::cursor, saved_cursor);
    }
    // The date (day, year) at `at` moved on by a draw's remainder; an overflow of the day past 0x16c rolls the year.
    void later_dates() {
        const auto other = c.toc(cell::event_date);
        auto x = draw.next(0x101ab66c);
        put_year(offer::date_b, c.u16(other + 2));
        put_day(offer::date_b, c.i16(other) + remainder(x, 7) + 1);
        if (day(offer::date_b) > 0x16c) {
            x = draw.next(0x101ab6e4);
            put_year(offer::date_b, year(offer::date_b) + 1);
            put_day(offer::date_b, remainder(x, 5));
        }
        x = draw.next(0x101ab754);
        put_year(offer::date_a, year(offer::date_b));
        put_day(offer::date_a, day(offer::date_b) + remainder(x, 7) + 1);
        x = draw.next(0x101ab7bc);
        if (remainder(x, 200) == 0) {
            x = draw.next(0x101ab814);
            put_day(offer::date_a, day(offer::date_a) + std::int16_t(remainder(x, 20)));
        }
        if (day(offer::date_a) > 0x16c) {
            x = draw.next(0x101ab888);
            put_year(offer::date_a, year(offer::date_a) + 1);
            put_day(offer::date_a, remainder(x, 5) + 4);
        }
    }
    void earlier_dates() {
        auto x = draw.next(0x101ab900);
        put_year(offer::date_b, c.u16(today + 2));
        put_day(offer::date_b, c.i16(today) + remainder(x, 3) + 1);
        if (day(offer::date_b) > 0x16c) {
            x = draw.next(0x101ab970);
            put_year(offer::date_b, year(offer::date_b) + 1);
            put_day(offer::date_b, remainder(x, 5));
        }
        put_year(offer::date_a, year(offer::date_b));
        const auto candidate = chosen_person();
        const auto owner = c.u32(candidate + person::job_club);
        if (owner != 0 && i8(out(c, 0x1014c2e0, 0x101ab9f0, {owner, 0, 1, 0, 0})) != 0 &&
            c.u8(chosen_person() + person::status) != 5) {
            c.link(0x101aba20);
            post_approach_news(c, chosen_person(), club);
            x = draw.next(0x101aba40);
            put_day(offer::date_a, day(offer::date_b) + remainder(x, 5) + 10);
        }
        // Not an else: the original falls through, and the next draw overwrites the day.
        if (leaving != 0 && c.i32(leaving) >= people_count(c) - 0x10) {
            x = draw.next(0x101abac4);
            put_day(offer::date_a, day(offer::date_b) + remainder(x, 5) + 7);
        } else {
            x = draw.next(0x101abb30);
            put_day(offer::date_a, day(offer::date_b) + remainder(x, 5) + 4);
        }
        x = draw.next(0x101abb98);
        if (remainder(x, 500) == 0) {
            x = draw.next(0x101abbf0);
            put_day(offer::date_a, day(offer::date_a) + std::int16_t(remainder(x, 20)));
        }
        if (day(offer::date_a) > 0x16c) {
            x = draw.next(0x101abc64);
            put_year(offer::date_a, year(offer::date_a) + 1);
            put_day(offer::date_a, remainder(x, 5) + 4);
        }
    }

    // ---- the last 16 people may be approached for this club (0x101abcd0..0x101abd88) ----
    void scout_peers() {
        if (out(c, 0x100c96e0, 0x101abcd4, {club}) != 0) return;
        if (i8(out(c, 0x100d3c40, 0x101abce4, {club})) == 0) return;
        if (i8(out(c, 0x100c7d00, 0x101abcf4, {club})) != 0) return;
        const auto dispatcher = c.u32(c.toc(cell::dispatcher));
        for (std::int32_t index = people_count(c) - 0x10; index < people_count(c); ++index) {
            const auto peer = person_at(c, index);
            if (c.i8(peer + person::flag) == 0) continue;
            if (i8(out(c, 0x10147e70, 0x101abd2c, {dispatcher, peer})) == 0) continue;
            if (i8(out(c, 0x10149df0, 0x101abd44, {dispatcher, 1, peer})) == 0) continue;
            if (const auto owner = c.u32(peer + person::job_club); owner != 0 && !(c.i16(owner + club::reputation) < c.i16(club + club::reputation)))
                continue;
            c.link(0x101abd78);
            approach_candidate(c, object, club, peer);
        }
    }
};
}

void plan_vacancy(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t leaving) {
    guest::Frame frame(c, 0x101aaa90, 0xd0, {.first_gpr = 13});
    // The registers of the prologue (see manager_manager_rematch.cpp: they reach the padding of the record).
    guest::keep(c, {{13, c.toc(cell::dispatcher)}, {14, c.toc(-0x7b88)}, {15, object}, {16, club}, {17, leaving},
                    {24, c.toc(cell::rows)}, {25, c.toc(cell::object)}, {26, c.toc(-0x7b90)}, {27, c.toc(-0x7b8c)},
                    {28, c.toc(-0x7b84)}, {29, c.toc(cell::pool)}, {30, c.toc(cell::people_count)},
                    {31, c.toc(cell::people)}});
    Vacancy(c, object, club, leaving, frame.base()).run();
}

namespace {
// ---- 0x101ac280 ----
class Approach {
public:
    Approach(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t person, std::uint32_t frame)
        : c(c), draw(c), fpu(c), object(object), club(club), person(person), frame(frame),
          pool_at(c.toc(cell::pool)), pool_str(c.toc(slots::pool)), today(c.toc(cell::today)), record(frame + 0xb4) {}

    void run() {
        out(c, 0x10715880, 0x101ac2d0, {record + offer::date_a});
        out(c, 0x10715880, 0x101ac2d8, {record + offer::date_b});
        // (The original also reads and writes bytes of its still unwritten record in the block below, before
        // the copy that overwrites all 0x28 of them: none of it reaches memory.)
        if (c.u32(person + person::job_club) != 0) {
            const auto x = draw.next(0x101ac308);
            // x % 4 != 0 for every x: the low two bits.
            if ((x & 3) != 0 && i8(out(c, 0x100c7d00, 0x101ac340, {club})) == 0) shake_owner();
        }
        if (c.i32(person) >= people_count(c) - 0x10) raise_minimums();
        std::int32_t score = -250000;
        if (c.u32(person + person::staff) != 0) {
            guest::enter(c, 0x101acb3c, {object, person, club, 0, 5});
            score = std::int32_t(assess(c, person, club, 0, 5));
        }
        switch (locate(0x101acb64, 0x101acb80, 0x101acbe4, 0x94, 0x4c, 0x306d)) {
            case Found: break;
            case Ended: return;
            case Missing:
                c.link(0x101acc1c);
                plan_vacancy(c, object, club, 0);
                switch (locate(0x101acc34, 0x101acc50, 0x101accb4, 0x74, 0x44, 0x3088)) {
                    case Found: break;
                    case Ended: return;
                    case Missing:
                        assertion(0x101accf0, 0x54, 0x3c, 0x3097);
                        return;
                }
        }
        if (c.u32(node + 0xc) == c.u32(person)) {
            if (hit_in_deals()) {
                const auto buffer = frame + 0x2a4;
                start_message(c, buffer, 0x1773, 0x101acddc);
                set_field(c, buffer, 0, c.u32(club), 0x101acdec);
                set_field(c, buffer, 1, 1, 0x101acdfc);
                out(c, 0x102cbc10, 0x101ace10, {news_desk(), buffer, person, 0});
                return;
            }
            const auto buffer = frame + 0x1c0;
            start_message(c, buffer, 0x1773, 0x101ace24);
            set_field(c, buffer, 0, c.u32(club), 0x101ace34);
            set_field(c, buffer, 1, 0, 0x101ace44);
            out(c, 0x102cbc10, 0x101ace58, {news_desk(), buffer, person, 0});
        } else {
            const auto buffer = frame + 0xdc;
            start_message(c, buffer, 0x1773, 0x101ace6c);
            set_field(c, buffer, 0, c.u32(club), 0x101ace7c);
            set_field(c, buffer, 1, 0, 0x101ace8c);
            out(c, 0x102cbc10, 0x101acea0, {news_desk(), buffer, person, 0});
        }
        // A better score than the queued offer's replaces it (unless it is the same side of the person's id).
        if (score > c.i32(record + offer::score)) {
            const auto id = c.u32(person);
            if ((id == 0 ? 1u : 0u) != c.u32(node + 8)) {
                c.put32(record + offer::chosen, id);
                c.put32(record + offer::score, std::uint32_t(score));
                out(c, 0x10738e60, 0x101aced8, {list(), node});
                out(c, 0x10738d10, 0x101acee8, {list(), record, offer::size});
            }
        }
    }

private:
    enum Search { Found, Missing, Ended };
    Cpu& c;
    Draw draw;
    guest::Fpu fpu;
    std::uint32_t object, club, person, frame, pool_at, pool_str, today, record, node = 0;

    std::uint32_t list() const { return c.u32(object + 8); }
    std::uint32_t news_desk() const { return c.u32(c.toc(cell::news_desk)); }
    void assertion(std::uint32_t at, std::uint32_t name, std::uint32_t extension, std::uint32_t line) {
        guest::report_assertion(c, {.at = at, .file = pool_str + 0x37, .format = pool_str + 0x4b,
                                    .title = pool_str + 0x58, .name = frame + name, .extension = frame + extension,
                                    .line = line});
    }
    // The offer list walked for the club's entry (word +0x10 is the club's id); it is copied into the frame
    // record. A list shorter than its count ends the routine (the original's assertion).
    Search locate(std::uint32_t back_node, std::uint32_t assertion_at, std::uint32_t back_copy, std::uint32_t name,
                  std::uint32_t extension, std::uint32_t line) {
        c.put32(list() + list::cursor, c.u32(list() + list::head));
        for (std::int32_t n = 0; n < c.i16(list() + list::count); ++n) {
            node = out(c, 0x10738e00, back_node, {list()});
            if (!node) {
                assertion(assertion_at, name, extension, line);
                return Ended;
            }
            if (c.u32(node + 0x10) == c.u32(club)) {
                out(c, 0x10739ef0, back_copy, {record, node, offer::size});
                return Found;
            }
        }
        return Missing;
    }
    // Whether the person's side has a pending entry in the deals' list for this club (a 0x96-kind entry less
    // than 0xb4 days old for the person).
    bool hit_in_deals() {
        bool hit = false;
        const auto deals = out(c, 0x10386be0, 0x101acd58, {c.u32(c.toc(cell::deals)), club, 0});
        if (deals != 0) {
            c.put32(deals + list::cursor, c.u32(deals + 8));
            for (;;) {
                const auto entry = out(c, 0x10738e30, 0x101acdb0, {deals});
                if (!entry) break;
                const auto days = std::int32_t(out(c, 0x10718130, 0x101acd7c, {today, c.u32(entry), c.u32(entry + 4)}));
                if (days >= 0xb4 || c.u8(entry + 8) != 0x96 || c.u32(entry + 0xc) != c.u32(person)) continue;
                hit = true;
                break;
            }
            out(c, 0x10738c20, 0x101acdc4, {deals, 1});
        }
        return hit;
    }

    // The club of the person is told the person's moods change: his level drops when the move is recent, a draw
    // decides a message about it, and the ratings of the club's row are scaled.
    void shake_owner() {
        bool recent = false;
        if (c.u32(person + person::job_club) != 0) {
            const auto days = std::int32_t(out(c, 0x10718130, 0x101ac364, {today, c.u32(person + person::since_a), c.u32(person + person::since_b)}));
            recent = days < 0xb4;
        }
        if (recent) {
            const auto level = c.i8(person + person::level);
            if (level > 1) c.put8(person + person::level, std::uint32_t(level) - 1);
            const auto n = divide(c.i8(person + person::level), 3);
            const auto pick = n == 0 ? 0 : draw.small(n, 0x101ac3cc);
            if (pick == 0) hire_message(0x46c, 0x39, 0);
        } else {
            const std::int32_t n = c.i8(person + person::level) + 5;
            const auto pick = n == 0 ? 0 : draw.small(n, 0x101ac624);
            if (pick == 0) hire_message(0x388, 0x38, 0x258);
        }
        scale_rows();
    }
    // The message of kind 0x1390: the person, his club, `club`, his three names, and, for a person of the last
    // 16, the pair of names through 0x102cb630. `delta` moves the original's call sites (the same code twice).
    void hire_message(std::uint32_t buffer_at, std::uint32_t pick_at, std::uint32_t delta) {
        const auto buffer = frame + buffer_at;
        const auto owner = c.u32(person + person::job_club);
        start_message(c, buffer, 0x1390, 0x101ac510 + delta);
        set_field(c, buffer, 0, c.u32(person), 0x101ac520 + delta);
        set_field(c, buffer, 1, c.u32(owner), 0x101ac530 + delta);
        set_field(c, buffer, 2, c.u32(club), 0x101ac540 + delta);
        set_field(c, buffer, 3, c.u32(c.u32(person + person::name_a) + 0x20), 0x101ac554 + delta);
        set_field(c, buffer, 4, c.u32(c.u32(person + person::name_b) + 0x20), 0x101ac568 + delta);
        set_field(c, buffer, 5, c.u32(c.u32(person + person::name_c) + 0x20), 0x101ac57c + delta);
        if (c.i32(person) >= people_count(c) - 0x10) {
            const auto pick = frame + pick_at;
            c.put8(pick, 7);
            out(c, 0x102cb630, 0x101ac5ac + delta, {buffer, 7, c.u32(person + person::name_a), pick});
            set_field(c, buffer, 6, sext8(c.u8(pick)), 0x101ac5c0 + delta);
            out(c, 0x102cb630, 0x101ac5d4 + delta, {buffer, c.u8(pick), c.u32(person + person::name_b), pick});
        }
        out(c, 0x102cbc10, 0x101ac5e8 + delta, {news_desk(), buffer, person, 0});
    }
    // The person's club's row: ratings at +6, +8, +0xa, +0xc scaled (one pair of factors when that club's reputation is
    // above this club's, another otherwise), and the row's date at +0x20 set to today.
    void scale_rows() {
        const auto owner = c.u32(person + person::job_club);
        const bool higher = c.i16(owner + club::reputation) > c.i16(club + club::reputation);
        const auto factor_a = higher ? pool::scale_d : pool::scale_e, factor_b = higher ? pool::scale_a : pool::scale_f;
        const auto row_now = [&] { return c.u32(object) + c.u32(c.u32(person + person::job_club)) * row::size; };
        const std::pair<std::uint32_t, std::uint32_t> fields[] = {{6, factor_a}, {8, factor_b}, {0xa, factor_a}, {0xc, factor_a}};
        for (const auto& [offset, factor] : fields) {
            const auto at = row_now() + offset;
            c.put16(at, std::uint32_t(scaled(fpu, c, pool_at, factor, c.i16(at))));
        }
        const auto date_at = row_now() + 0x20;
        c.put16(date_at + 2, c.u16(today + 2));
        c.put16(date_at, c.u16(today));
        c.put8(date_at + 4, c.u8(today + 4));
    }
    // A person of the last 16 keeps the least of his staff record's three ratings.
    void raise_minimums() {
        const std::pair<std::uint32_t, std::int32_t> floors[] = {{staff::a, 0x5dc}, {staff::b, 0x4e2}, {staff::c, 0xfa}};
        for (const auto& [offset, floor] : floors) {
            const auto record_at = c.u32(person + person::staff);
            if (c.i16(record_at + offset) < floor) c.put16(record_at + offset, std::uint32_t(floor));
        }
    }
};
}

void approach_candidate(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t person) {
    guest::Frame frame(c, 0x101ac280, 0x5e0, {.first_gpr = 15});
    guest::keep(c, {{16, c.toc(-0x7b90)}, {17, c.toc(-0x7b8c)}, {18, c.toc(-0x7b88)}, {19, c.toc(-0x7b84)},
                    {20, c.toc(cell::pool)}, {21, c.toc(cell::people_count)}, {22, c.toc(-0x7bac)},
                    {25, c.toc(cell::news_desk)}, {26, c.toc(cell::today)}, {27, c.toc(-0x7ba8)},
                    {28, c.toc(slots::pool)}, {29, object}, {30, club}, {31, person}});
    Approach(c, object, club, person, frame.base()).run();
}
}
