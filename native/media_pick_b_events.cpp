// 0x10698f50, the press's pass over its event rows (media.dat's first table, 28 bytes each: who,
// at which club, the day, a mark, a weight). A row is dropped once it is 31 days old, or when its
// person has no club manager or contract. A fresh row (mark 0) has a 1 in 25 chance of starting a
// story (item 0xfbc) about its person that also takes up the person's other fresh rows (five clubs
// at most); a marked row (mark 1) may close its person's chapter (mark 2).
#include "media_pick_b.hpp"
#include "media_pick_a_detail.hpp"
#include "media_pick_b_events_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::store;
using namespace detail::pick_b;
using namespace detail::pick;

namespace {
constexpr std::uint32_t any = 0xffffffff;
// The routine's frame (offsets from sp).
namespace local {
constexpr std::uint32_t today = 0x40, expiry = 0x48, stored = 0x38, clubs = 0x50, audience = 0x64, item = 0xdc, bytes = 0x1c0;
}
namespace callee {
constexpr std::uint32_t standing_of = 0x10082f20, record_of = 0x10082f90, add_days = 0x10717890, cached = 0x103f83c0,
    club_flag = 0x1014c2e0, person_flag = 0x100d3150, fame = 0x100d71a0, grade = 0x10117490, listed_clubs = 0x103fc710,
    rival_report = 0x100e9d10;
// This module's.
constexpr std::uint32_t verdict = 0x106584d0;
}
constexpr int persons_slot = -0x7be0, person_total_slot = -0x7be4, standings_slot = -0x7b7c, found_slot = -0x7b78,
    today_slot = -0x7b80, cache_slot = -0x7b40;
constexpr std::uint32_t person_size_here = 0x70;

struct EventPass {
    Cpu& c;
    const Frame& frame;
    const Press press;
    const Program program;
    Fpu fpu;
    const Stream rng;
    const Item item;
    std::uint32_t self;
    // Quirk: manager (r19) is only set when the person has an own club, so a later row without one still
    // sees the previous row's manager.
    // r14: the person's standing record; r16: how many places the story goes to; r17: the row's club;
    // r18: the person's own club; r19: that club's manager; r20: the person; r21: the verdict on the
    // club; r22: how many clubs the person's fresh rows name; r23: the story's field counter (and a
    // scan index between); r24: a scan index; r25: the row.
    Kept<14> standing;
    Kept<16> audience_size;
    Kept<17> row_club;
    Kept<18> own_club;
    Kept<19> manager;
    Kept<20> person;
    Kept<21> verdict;
    Kept<22> involved;
    Kept<23> field;
    Kept<24> scan;
    Kept<25> index;

    EventPass(Cpu& cpu, const Frame& f, std::uint32_t press_at, const Stream& stream)
        : c(cpu), frame(f), press{cpu, press_at}, program{cpu}, fpu(cpu), rng(stream), item{cpu, f.at(local::item)}, self(press_at),
          standing(cpu, cpu.r(14)), audience_size(cpu, cpu.r(16)), row_club(cpu, cpu.r(17)), own_club(cpu, cpu.r(18)),
          manager(cpu, 0), person(cpu, cpu.r(20)), verdict(cpu, cpu.r(21)), involved(cpu, cpu.r(22)),
          field(cpu, cpu.r(23)), scan(cpu, cpu.r(24)), index(cpu, 0) {}

    EventRow row(std::uint32_t n) const { return press.event_row(n); }
    std::uint32_t clubs_base() const { return c.u32(c.toc(slot::clubs)); }
    Field<std::uint32_t> club_list(std::uint32_t n) const { return {c, frame.at(std::int32_t(local::clubs + 4 * n))}; }
    Field<std::uint32_t> audience(std::uint32_t n) const { return {c, frame.at(std::int32_t(local::audience + 4 * n))}; }
    std::int32_t involved_count() const { return byte_word(involved); }
    std::int32_t audience_count() const { return byte_word(audience_size); }
    std::uint32_t club_at(std::uint32_t id) const { return clubs_base() + id * club_size; }

    // The person and clubs a row speaks of.
    void look_up(const EventRow& r) {
        const auto persons = c.u32(c.toc(persons_slot));
        person = persons + r.person().get() * person_size_here;
        row_club = club_at(r.club().get());
        const auto found = program.call(callee::standing_of, 0x10698fe0,
                                        {c.u32(c.toc(standings_slot)), person, clubs_base(), r.person().get(), persons});
        const auto cached_at = c.toc(found_slot);
        c.put32(cached_at, found);
        compare_logical(c, found);
        // r3 and r4 as the lookup of the own club leaves them.
        if (found) leave(c, {{3, found}, {4, clubs_base()}});
        else leave(c, {{4, cached_at}});
        own_club = found ? club_at(c.u32(c.u32(cached_at) + 4)) : 0;
        if (compare(c, std::int32_t(std::uint32_t(own_club))) != 0) {
            standing = program.call(callee::record_of, 0x1069902c, {c.u32(c.toc(standings_slot)), person, own_club});
            manager = Club{c, own_club}.manager();
        } else {
            standing = 0;
        }
    }

    // Older than 31 days: today against the row's day plus 31, by the original's date compare.
    bool expired(const EventRow& r) {
        const auto today = c.toc(today_slot);
        c.put32(frame.at(local::today), c.u32(today));
        c.put32(frame.at(local::today + 4), c.u32(today + 4));
        program.call(callee::add_days, 0x10699074, {frame.at(local::expiry), r.at + 8, 0x1f, c.u32(today + 4), c.u32(today)});
        return compare(c, byte_word(std::uint32_t(date_later(c, frame.at(local::today), frame.at(local::expiry), frame.at(local::expiry))))) != 0;
    }

    // Drops the row by moving the last one over it.
    void drop(std::uint32_t at) {
        const auto count = std::uint32_t(press.event_count().get());
        const auto rows = press.events().get();
        program.call(library::move, 0x10699110, {rows + at * event_size, rows + (count - 1) * event_size, event_size, at * event_size, rows});
        c.set(3, count);
        index = std::uint32_t(index) - 1;
        press.event_count() = std::int32_t(count - 1);
    }

    // Puts the next field of the story.
    void put(std::uint32_t value, std::uint32_t back) {
        const auto at = std::uint32_t(field);
        field = at + 1;
        item.put(at, value, back);
    }

    // The one-in-25 draw.
    bool story_drawn() {
        rng.advance(0x10699180);
        const auto d = reduced_by(rng.sample(), divisors::by_25);
        leave(c, {{3, d.sign}, {4, rng.sample()}});
        return compare(c, d.rest) == 0;
    }

    // Takes the person's fresh rows (this one first) into the story, five clubs at most. True when one
    // of them is already taken: the story is not told.
    bool gather(const EventRow& r) {
        bool taken = false;
        const auto rows = press.events().get();
        involved = 0;
        for (scan = 0; compare(c, std::int32_t(std::uint32_t(scan)), 5) < 0; scan = std::uint32_t(scan) + 1) {
            club_list(scan) = any;
            leave(c, {{3, frame.at(local::clubs)}, {4, 4 * std::uint32_t(scan)}, {5, any}});
        }
        club_list(byte_word(involved)) = r.club().get();
        scan = 0;
        involved = std::uint32_t(involved) + 1;
        r.mark() = 1;
        leave(c, {{3, r.at}, {4, rows}, {5, 1}, {6, 0}, {7, r.club().get()}});
        for (;; scan = std::uint32_t(scan) + 1) {
            leave(c, {{3, std::uint32_t(press.event_count().get())}});
            if (compare(c, std::int32_t(std::uint32_t(scan)), press.event_count()) >= 0) break;
            leave(c, {{3, std::uint32_t(involved_count())}});
            if (compare(c, involved_count(), 5) >= 0) break;
            const auto other = row(scan);
            if (compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(std::uint32_t(scan))) == 0) continue;
            leave(c, {{3, r.person().get()}, {4, rows}, {5, other.person().get()}, {6, rows}});
            if (compare(c, std::int32_t(other.person().get()), std::int32_t(r.person().get())) != 0) continue;
            leave(c, {{3, std::uint32_t(byte_word(other.mark().get()))}, {4, rows}});
            if (compare(c, byte_word(other.mark().get())) != 0) {
                taken = true;
                break;
            }
            const auto before = std::uint32_t(involved_count());
            club_list(byte_word(involved)) = other.club().get();
            involved = std::uint32_t(involved) + 1;
            other.mark() = 1;
            leave(c, {{3, other.at}, {4, rows}, {5, 1}, {6, 4 * before}, {7, other.club().get()}});
        }
        return compare(c, taken) != 0;
    }

    void begin_story() {
        item.begin(0xfbc, 0x106992e8);
        field = 0;
        put_person(c, *this, person, {0x10699300, 0x10699318, 0x10699330, 0x10699348});
        if (compare_logical(c, own_club) != 0) put(Club{c, own_club}.id(), 0x10699364);
        else put(any, 0x1069937c);
    }

    // A youngster may earn a story of his own (field 0x24, standing 1); otherwise the field is a draw of 3.
    void reaction_field() {
        if (compare(c, Person{c, person}.sportsmanship().get(), 0xf) < 0 && compare(c, Person{c, person}.loyalty().get(), 0xf) < 0) {
            rng.advance(0x106993bc);
            const auto d = reduced_by(rng.sample(), event_divisors::by_12500);
            leave(c, {{3, std::uint32_t(d.rest)}, {4, d.sign}, {5, rng.sample()}, {6, c.u16(c.u32(rng.cursor))}});
            if (compare(c, Person{c, person}.player_record().world_reputation(), d.rest) > 0) {
                const auto row_at = row(index).at;
                leave(c, {{3, row_at}, {4, 0x43300000}, {5, std::uint32_t(index) * event_size}, {6, press.events().get()}});
                const auto pool = c.toc(slot::floats);
                const auto bias = c.f64(pool + 8);
                const auto skill = fpu.fsub(stacked_integer(c, frame.at(std::int32_t(local::bytes)), signed_word(Person{c, person}.loyalty().get())), bias);
                const auto threshold = fpu.multiply_add(0x10699458, c.f64(pool + 0x118), skill, c.f64(pool + 0xb8));
                const auto weight = fpu.fsub(stacked_integer(c, frame.at(std::int32_t(local::bytes + 8)), signed_word(c.u32(row_at + 0x18))), bias);
                c.set_f(1, weight);
                fpu.fcmp(0, weight, threshold);
                if (weight > threshold) {
                    put(0x24, 0x1069947c);
                    standing = 1;
                    return;
                }
            }
        }
        rng.advance(0x106994a4);
        const auto d = reduced_by(rng.sample(), divisors::by_3);
        leave(c, {{6, rng.sample()}});
        put(std::uint32_t(d.rest), 0x106994e8);
        standing = 0;
    }

    // The club's verdict (when it is not barred) and the rest of the story's fields.
    void finish_story(const EventRow& r) {
        put(r.story().get(), 0x1069950c);
        bool judged = false;
        if (compare_logical(c, own_club) != 0) {
            const auto barred = program.call(callee::club_flag, 0x1069952c, {own_club, 0, 1, 0, 0});
            if (compare(c, byte_word(barred)) == 0) {
                verdict = program.module_call(callee::verdict, 0x10699548, {self, Club{c, own_club}.manager(), person, 0x1f});
                put(verdict, 0x10699560);
                judged = true;
            }
        }
        if (!judged) {
            verdict = 0;
            put(verdict, 0x1069957c);
        }
        put(program.call(callee::fame, 0x10699588, {person, own_club}), 0x1069959c);
        put(extended(std::int16_t(program.call(callee::grade, 0x106995a4, {Person{c, person}.player()}))), 0x106995b8);
        if (compare_logical(c, manager) != 0) put_person(c, *this, manager, {0x106995d4, 0x106995ec, 0x10699604, 0x1069961c});
        else put_no_person(*this, {0x10699634, 0x10699648, 0x1069965c, 0x10699670});
        const auto flagged = program.call(callee::person_flag, 0x1069967c, {person, 0});
        if (compare(c, byte_word(flagged)) != 0) put(1, 0x10699698);
        else put(0, 0x106996b0);
        put(involved, 0x106996c4);
    }

    // The clubs by rating, best first, then into the story.
    void rank_clubs() {
        const auto rating = [&](std::uint32_t n) { return std::int32_t(Club{c, club_at(club_list(n))}.reputation().get()); };
        for (scan = 0;; scan = std::uint32_t(scan) + 1) {
            leave(c, {{3, std::uint32_t(involved_count())}});
            if (compare(c, std::int32_t(std::uint32_t(scan)), involved_count() - 1) >= 0) break;
            std::uint32_t other = std::uint32_t(scan) + 1;
            for (c.set(3, other); compare(c, std::int32_t(other), involved_count()) < 0; ++other, c.set(3, other)) {
                const auto rating_other = rating(other);
                leave(c, {{4, std::uint32_t(rating_other)}, {5, std::uint32_t(rating_other)}, {6, clubs_base()}});
                if (compare(c, rating_other, rating(scan)) > 0) {
                    const auto kept = club_list(other).get(), moved = club_list(scan).get();
                    club_list(other) = moved;
                    club_list(scan) = kept;
                    leave(c, {{4, frame.at(local::clubs)}, {5, frame.at(local::clubs)}, {6, 4 * other}, {7, moved}, {8, kept}});
                }
            }
        }
        for (scan = 0; compare(c, std::int32_t(std::uint32_t(scan)), involved_count()) < 0; scan = std::uint32_t(scan) + 1)
            put(club_list(scan), 0x10699790);
    }

    // Whom the story goes to: the places 0x103fc710 lists for the person, the clubs of the rows, and
    // the clubs of the sixteen newest people who belong to them or are far below them.
    void collect_audience() {
        const auto cache = c.u32(c.toc(cache_slot));
        c.set(3, c.toc(cache_slot));
        if (compare_logical(c, cache) != 0) {
            audience_size = program.call(callee::listed_clubs, 0x106997cc, {cache, person, 0xda, frame.at(local::audience), 0x1e});
        } else {
            c.set(3, 0);
            audience_size = 0;
        }
        // Adds the club of a row unless the list has it (the list stops growing at 30).
        const auto add_from_rows = [&](std::uint32_t place) {
            bool found = false;
            field = 0;
            for (;; field = std::uint32_t(field) + 1) {
                leave(c, {{4, std::uint32_t(byte_word(field))}, {3, std::uint32_t(audience_count())}});
                if (compare(c, byte_word(field), audience_count()) >= 0) break;
                leave(c, {{3, std::uint32_t(found)}});
                if (compare(c, found) != 0) break;
                leave(c, {{3, place}, {4, clubs_base()}, {5, audience(byte_word(field))}, {6, 4 * std::uint32_t(byte_word(field))}});
                if (compare_logical(c, audience(byte_word(field)), place) == 0) found = true;
            }
            if (compare(c, found) != 0) return;
            audience(audience_count()) = place;
            audience_size = std::uint32_t(audience_size) + 1;
            leave(c, {{3, frame.at(local::audience)}, {4, place}, {5, clubs_base()}});
        };
        for (scan = 0; compare(c, std::int32_t(std::uint32_t(scan)), involved_count()) < 0 && compare(c, audience_count(), 0x1e) < 0;
             scan = std::uint32_t(scan) + 1)
            add_from_rows(club_at(club_list(scan)));
        auto candidate = c.u32(c.toc(persons_slot)) + (c.u32(c.toc(person_total_slot)) - 1) * person_size_here;
        leave(c, {{3, c.u32(c.toc(persons_slot))}, {4, candidate}});
        for (scan = 0; compare(c, std::int32_t(std::uint32_t(scan)), 0x10) < 0;
             scan = std::uint32_t(scan) + 1, candidate -= person_size_here, leave(c, {{4, candidate}})) {
            if (compare(c, byte_word(c.u8(candidate + 0x6b))) == 0) continue;
            if (compare_logical(c, Person{c, candidate}.club()) == 0) continue;
            for (field = 0;; field = std::uint32_t(field) + 1) {
                leave(c, {{3, std::uint32_t(byte_word(field))}});
                if (compare(c, byte_word(field), involved_count()) >= 0 || compare(c, audience_count(), 0x1e) >= 0) break;
                const auto index_here = club_list(byte_word(field)).get();
                const auto club = club_at(index_here);
                const auto home = Person{c, candidate}.club().get();
                const auto offset = index_here * club_size;
                // Same +0x30 record, or the same +0x2c record and 3000 points of rating above the person's club.
                const Club here{c, club}, there{c, home};
                leave(c, {{3, here.division()}, {5, offset}, {6, clubs_base()}});
                bool wanted = compare_logical(c, here.division(), there.division()) == 0;
                if (!wanted) {
                    leave(c, {{3, here.nation()}, {5, offset}, {6, clubs_base()}});
                    if (compare_logical(c, here.nation(), there.nation()) != 0) continue;
                    const auto margin = std::int32_t(here.reputation().get()) - 0xbb8;
                    leave(c, {{3, std::uint32_t(margin)}, {5, offset}, {6, clubs_base()}});
                    if (compare(c, margin, std::int32_t(there.reputation().get())) <= 0) continue;
                }
                bool found = false;
                for (std::uint32_t n = 0;; ++n) {
                    leave(c, {{3, n}, {5, std::uint32_t(audience_count())}});
                    if (compare(c, std::int32_t(n), audience_count()) >= 0) break;
                    leave(c, {{5, std::uint32_t(found)}});
                    if (compare(c, found) != 0) break;
                    leave(c, {{6, audience(n)}, {5, home}});
                    if (compare_logical(c, audience(n), home) == 0) found = true;
                }
                if (compare(c, found) != 0) continue;
                audience(audience_count()) = home;
                audience_size = std::uint32_t(audience_size) + 1;
                leave(c, {{3, frame.at(local::audience)}, {5, home}});
            }
        }
    }

    void post_story() {
        for (field = 0;; field = std::uint32_t(field) + 1) {
            leave(c, {{3, std::uint32_t(byte_word(field))}});
            if (compare(c, byte_word(field), audience_count()) >= 0) break;
            item.post(audience(byte_word(field)), 0x10699a20);
        }
        if (compare(c, byte_word(standing)) != 0) {
            enter(c, 0x10699a58, {self, 0x1f, person, own_club, frame.at(local::clubs), involved});
            weigh_story_targets(c, 0x1f, person, own_club, frame.at(local::clubs), involved);
        }
        if (compare_logical(c, own_club) == 0) return;
        const auto barred = program.call(callee::club_flag, 0x10699a78, {own_club, 0, 1, 0, 0});
        if (compare(c, byte_word(barred)) != 0) return;
        if (compare(c, std::int32_t(std::uint32_t(verdict))) == 0) return;
        if (compare_logical(c, row_club) != 0) {
            enter(c, 0x10699aa8, {self, person, row_club, own_club, std::uint32_t(byte_word(verdict))});
            post_squad_story(c, person, row_club);
        }
        program.module_call(0x10677d60, 0x10699ac0, {self, person, own_club, 0x1f, std::uint32_t(byte_word(verdict))});
    }

    // A row that is not (or no longer) a fresh one: a marked row may close its person's chapter.
    void marked_row(const EventRow& r) {
        c.set(3, r.at);
        if (compare(c, std::int32_t(r.mark().get()), 1) != 0) return;
        if (compare_logical(c, own_club) == 0) return;
        const auto barred = program.call(callee::club_flag, 0x10699afc, {own_club, 1, 1, 0, 0});
        if (compare(c, byte_word(barred)) != 0) return;
        c.put32(frame.at(local::stored), c.u32(r.at + 8));
        c.put32(frame.at(local::stored + 4), c.u32(r.at + 0xc));
        if (compare(c, byte_word(std::uint32_t(date_later(c, frame.at(local::stored), person + 0x4c, c.u32(r.at + 0xc))))) == 0) return;
        if (compare_logical(c, standing) == 0) return;
        if (compare(c, Contract{c, standing}.listed().get()) != 0) return;
        const auto flagged = program.call(callee::person_flag, 0x10699b98, {person, 0});
        if (compare(c, byte_word(flagged)) != 0) return;
        standing = std::uint8_t(Contract{c, standing}.happiness().get());
        rng.advance(0x10699bc4);
        const auto chance = reduced_by(rng.sample(), event_divisors::by_50);
        leave(c, {{3, std::uint32_t(byte_word(standing))}, {4, std::uint32_t(chance.rest)}, {5, rng.sample()}});
        if (compare(c, byte_word(standing), chance.rest + 0x28) <= 0) return;
        rng.advance(0x10699c2c);
        const auto thirtieth = reduced_adding_sample(rng.sample(), event_divisors::by_30);
        leave(c, {{3, thirtieth.sign}, {4, rng.sample()}});
        if (compare(c, thirtieth.rest) != 0) return;
        program.call(callee::rival_report, 0x10699c80, {person, own_club, c.u32(own_club + 0x80), 0});
        const auto rows = press.events().get();
        for (scan = 0;; scan = std::uint32_t(scan) + 1) {
            if (compare(c, std::int32_t(std::uint32_t(scan)), press.event_count()) >= 0) break;
            const auto theirs = row(scan).person().get(), mine = row(index).person().get();
            leave(c, {{4, theirs}, {5, rows}, {3, rows}});
            if (compare(c, std::int32_t(theirs), std::int32_t(mine)) == 0) {
                row(scan).mark() = 2;
                leave(c, {{3, row(scan).at}, {4, 2}});
            }
        }
    }

    void visit() {
        const auto r = row(index);
        look_up(r);
        if (expired(r) || compare_logical(c, manager) == 0 || compare_logical(c, Person{c, person}.player()) == 0) {
            drop(index);
            return;
        }
        const auto cached_here = program.call(callee::cached, 0x10699140, {c.u32(c.toc(cache_slot)), person, row_club, 0, 0});
        if (compare(c, byte_word(cached_here)) == 0) return;
        c.set(3, r.at);
        if (compare(c, byte_word(r.mark().get())) != 0 || !story_drawn()) {
            marked_row(r);
            return;
        }
        if (gather(r)) return;
        begin_story();
        reaction_field();
        finish_story(r);
        rank_clubs();
        collect_audience();
        post_story();
    }
};
}

// The press's pass over its event rows.
void process_press_events(Cpu& c, std::uint32_t self) {
    Frame frame(c, 0x10698f50, 0x220, {.first_gpr = 13});
    const auto stream = Stream::world(c);
    keep(c, {{13, c.toc(slot::floats)}, {15, self}, {26, c.toc(-0x7bf0)}, {27, c.toc(slot::clubs)},
             {28, stream.seed}, {29, stream.start}, {30, stream.end}, {31, stream.cursor}});
    EventPass pass(c, frame, self, stream);
    if (compare(c, pass.press.event_count()) == 0) return;
    for (pass.index = 0; compare(c, std::int32_t(std::uint32_t(pass.index)), pass.press.event_count()) < 0;
         pass.index = std::uint32_t(pass.index) + 1)
        pass.visit();
    if (compare(c, pass.press.event_count()) != 0)
        pass.program.call(library::sort, 0x10699cfc, {pass.press.events(), std::uint32_t(pass.press.event_count().get()), event_size, c.toc(slots::event_order)});
}
}
