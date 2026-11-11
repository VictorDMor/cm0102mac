// MANAGER_MANAGER.CPP assessment block: 0x101c1e20, the verdict on a manager offered to a club
// (manager_manager_assess.hpp). Native-first: the values the listing keeps in registers are locals here; the
// guest ABI appears at the calls to code outside the block and at the record the caller passes by address.
#include "manager_manager_assess.hpp"
#include "manager_manager_assess_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
using namespace assess;
namespace {
namespace callee {
constexpr std::uint32_t employer_record = 0x10103510, date_score = 0x10718130, adjust_employer = 0x100f5770,
                        relation = 0x100f55b0, set_rating = 0x100f4f20;
}
namespace toc_cell { constexpr std::int32_t date = -0x7b80, tiers = -0x7984, employer = -0x7ab8, last_date = -0x7a9c; }
constexpr std::uint32_t row_size = 0x4c;
namespace row_at {  // the club's row in the list object's table
constexpr std::uint32_t spare = 5, rating = 6, amount = 0x28, high = 0x30, low = 0x38, since = 0x44;
}
namespace club_at {
constexpr std::uint32_t id = 0, tier = 0x2c, board = 0x3c, owner = 0x80, manager = 0x90, reputation = 0x1ec,
                        kind = 0x23d, standing = 0x240;
}
namespace person_at { constexpr std::uint32_t profile = 0x1c, date_a = 0x4c, date_b = 0x50; }  // profile: +0x2a int16
namespace owner_at { constexpr std::uint32_t profile = 0x1c, temper = 0x63; }                    // +0x2f, +0x3d: int8
constexpr std::uint32_t frame_size = 0x110, date_local = 0x38, tail_local = 0x3c;
// The verdict byte by what 0x101c3220 does with it: 1 nothing; 2 posts a record to the list and flags the row;
// 3 lowers the club's row rating by 1250, 4 by 500. What each means in the game is not established.
constexpr std::int8_t verdict_none = 1, verdict_post = 2, verdict_lower_more = 3, verdict_lower_less = 4;

class Verdict {
public:
    Verdict(Cpu& cpu, std::uint32_t list_object, std::uint32_t record_, std::uint32_t club_, std::uint32_t person_)
        : c(cpu), fpu(cpu), draws(cpu), list(list_object), record(record_), club(club_), person(person_),
          date(cpu.toc(toc_cell::date)), pool(cpu.toc(toc_slot::constants)) {}

    std::int8_t run(const guest::Frame& frame) {
        employer = employer_of();
        if (!employer) { set(verdict_lower_more); return result(); }
        // The row's last date is replaced by today's; the old one is scored (the original passes it to the
        // scorer as two words read back from its stack: the byte after the date's fifth is a leftover).
        const auto last = row() + row_at::since;
        c.put16(frame.at(date_local), c.u16(last));
        c.put16(frame.at(date_local + 2), c.u16(last + 2));
        c.put8(frame.at(tail_local), c.u8(last + 4));
        c.put16(last + 2, c.u16(date + 2));
        c.put16(last, c.u16(date));
        c.put8(last + 4, c.u8(date + 4));
        const auto since = score(c.u32(frame.at(date_local)), c.u32(frame.at(tail_local)), 0x101c2000);
        const auto away = score(c.u32(person + person_at::date_a), c.u32(person + person_at::date_b), 0x101c2018);
        adjust = 0xfa;
        if (away < 0x1e) adjust = -500;
        else if (away < 0xb4) adjust = -0xfa;
        if (since < 0xf) adjust += 0x2ee;
        else if (since < 0x1e) adjust += 500;
        else if (since < 0xb4) adjust -= 0xfa;
        if (const auto owner = c.u32(club + club_at::owner); owner) {
            if (const auto profile = c.u32(owner + owner_at::profile); profile) {
                const std::int8_t level = c.i8(profile + 0x3d);
                if (level == 1) adjust -= 500;
                else if (level < 6) adjust -= 0xfa;
                else if (level > 0xf) adjust += 0xfa;
            }
        }
        if (since > 0xb4 || (since > 0x5a && score(c.u32(person + person_at::date_a), c.u32(person + person_at::date_b), 0x101c20cc) < 0x5a)) {
            const auto last_date = c.toc(toc_cell::last_date);
            const bool recent = person_value() >= 0x1482 &&
                                score(c.u32(last_date), c.u32(last_date + 4), 0x101c20f8) >= 0xb4;
            set(recent ? verdict_lower_more : verdict_lower_less);
            return result();
        }
        switch (std::int8_t(c.u8(record + 8))) {
            case 1: kind_one(); break;
            case 2: kind_two(); break;
            case 3: kind_three(); break;
            default: kind_other(); break;
        }
        return result();
    }

private:
    Cpu& c;
    guest::Fpu fpu;
    Draws draws;
    std::uint32_t list, record, club, person, date, pool, employer = 0;
    std::int32_t adjust = 0;  // the original's r26: the club's mood about the manager, added to every cut

    std::int8_t result() { return std::int8_t(c.u8(record + 9)); }
    void set(std::int8_t verdict) { c.put8(record + 9, std::uint32_t(verdict)); }
    std::uint32_t employer_of() {
        c.set({{3, c.u32(c.toc(toc_cell::employer))}, {4, club}});
        c.call(callee::employer_record, 0x101c1f98);
        return c.r(3);
    }
    std::uint32_t row() const { return c.u32(list) + c.u32(club + club_at::id) * row_size; }
    std::int32_t score(std::uint32_t first, std::uint32_t second, std::uint32_t back) {
        c.set({{3, date}, {4, first}, {5, second}});
        c.call(callee::date_score, back);
        return std::int32_t(c.r(3));
    }
    std::int32_t person_value() const { return c.i16(c.u32(person + person_at::profile) + 0x2a); }
    std::int32_t manager_value() const { return c.i16(c.u32(c.u32(club + club_at::manager) + 0x1c) + 0x2a); }
    std::int32_t reputation() const { return c.i16(club + club_at::reputation); }
    std::int8_t tier_class() const { return c.i8(c.u32(club + club_at::tier) + 0x11f); }
    // The club's byte at +4 of its entry (8 bytes each) in the table of the TOC cell -0x7984.
    std::int8_t standing() const { return c.i8(c.u32(c.toc(toc_cell::tiers)) + c.u32(club + club_at::id) * 8 + 4); }
    std::int32_t rating() const { return c.i16(row() + row_at::rating); }
    std::int32_t budget() const { return c.i32(c.u32(club + club_at::board) + 4); }
    // pool[scale] * x, truncated by fctiwz: the conversion of an integer through the magic double.
    std::int32_t scaled(std::uint32_t scale, std::int32_t x) {
        const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u);
        auto f = fpu.fsub(std::bit_cast<double>(bits), c.f64(pool + pool_magic));
        f = fpu.fmul(c.f64(pool + scale), f);
        return std::int32_t(std::uint32_t(fpu.fctiwz(0, f)));
    }
    // fcmpo then ble: the branch is taken when "greater" is clear (an unordered compare takes it).
    bool greater(double a, double b) {
        fpu.fcmp(0, a, b);
        return a > b;
    }

    // ---- kind 1 (0x101c2140) ----
    void kind_one() {
        const std::int32_t value = person_value();
        if (value < 0x109a) return set(verdict_post);
        const std::int32_t rated = rating();
        if (rated < 0x109a && value < reputation() + adjust + 0x2ee) return set(verdict_post);
        const std::int32_t rep = reputation();
        const std::int8_t tier = tier_class();
        std::int32_t limit;
        if (tier > 0x12) limit = rep * 7 + 0x1388;
        else if (tier > 0xf) limit = rep * 5 + 0x9c4;
        else if (tier > 0xb) limit = rep * 3 + 0x2710;
        else if (tier > 8) limit = rep * 2 + 0x9c4;
        else limit = rep * 2 + 0x4e2;
        if (c.u8(club + club_at::kind) == 3) limit = scaled(pool_scale_c, limit);
        if (budget() < limit && reputation() > 0x1676 && standing() >= 0 && rated > 0x1c52) return set(verdict_none);
        if (standing() > 2 || (value > 0x1482 && value > reputation() + adjust + 0x2ee)) {
            const auto drawn = draws.modulo(0x1d4c, 0x101c22ec);
            set(rating() > half(std::uint32_t(drawn) + 0x9c4u) ? verdict_lower_more : verdict_post);
            return;
        }
        set(verdict_post);
    }

    // ---- kind 2 (0x101c2360) ----
    void kind_two() {
        const std::int32_t value = person_value();
        const std::int32_t floor = 0x109a - adjust;
        if (value < floor) return set(verdict_post);
        const std::int32_t rated = rating();
        if (rated < floor && value < reputation() + 0x2ee - adjust) return set(verdict_post);
        const std::int8_t tier = tier_class();
        std::int32_t limit;  // a halfword (the scale below keeps a word, read back as a halfword)
        if (tier > 0xe) limit = reputation() > 0x186a ? 0x14 : reputation() > 0x109a ? 0xf : reputation() > 0xcb2 ? 0xa : 7;
        else if (tier > 8) limit = reputation() > 0x186a ? 0xf : reputation() > 0x109a ? 0xa : 4;
        else limit = reputation() > 0x186a ? 0xa : reputation() > 0x109a ? 7 : 2;
        if (c.u8(club + club_at::kind) == 3) limit = scaled(pool_scale_c, std::int32_t(half(std::uint32_t(limit))));
        if (c.i8(club + club_at::standing) < std::int32_t(half(std::uint32_t(limit))) && reputation() > 0x1676 &&
            standing() >= 0 && rated > 0x1c52 - adjust)
            return set(verdict_none);
        if (standing() > 2 || (value > 0x1482 - adjust && value > reputation() - 0x2ee - adjust)) {
            const auto drawn = draws.modulo(0x1482, 0x101c2554);
            set(rating() > half(std::uint32_t(drawn) + 0x9c4u) ? verdict_lower_more : verdict_post);
            return;
        }
        set(verdict_post);
    }

    // ---- kind 3 (0x101c25c8): a request for money ----
    void kind_three() {
        c.set({{3, employer}, {4, 0}, {5, 0}, {6, 0}, {7, 1}, {8, 0}});
        c.call(callee::adjust_employer, 0x101c25e4);
        const std::uint32_t extra = c.r(4);  // the second word the call answers
        const std::int32_t rep = reputation();
        const auto at = row();
        const double earnings = c.f64(at + row_at::low);
        const std::int32_t whole = std::int32_t(std::uint32_t(fpu.fctiwz(0, earnings)));
        const std::int32_t held = std::int32_t(std::uint32_t(whole) + extra);
        std::int32_t base;
        if (rep > 0x1d4c) base = (rep - 0x1d4c) * 0x1388 + 12500000;
        else if (rep > 0x1482) base = (rep - 0x1482) * 0x3e8 + 3000000;
        else base = (rep - 0x1388) * 100 + 50000;
        const std::int8_t tier = standing();
        const std::int32_t want = scaled(tier > 4 ? 0x28 : tier < 0 ? pool_scale_a : pool_scale_b, base);
        const std::int32_t value = person_value();
        const std::int32_t floor = 0x109a - adjust;
        if (value < floor) return set(verdict_post);
        if (std::int32_t(c.i16(at + row_at::rating)) < floor && value < rep + 0x2ee - adjust) return set(verdict_post);
        if (held > want) {
            if (!greater(earnings, c.f64(at + row_at::high))) return set(verdict_post);
            if (tier > 2) return set(verdict_lower_more);
            const auto manager = manager_value();
            set(manager > 0x1676 && manager > rep - 0x2ee - adjust ? verdict_lower_more : verdict_post);
            return;
        }
        if (c.u32(at + row_at::amount) != 0) {
            set(verdict_lower_more);
        } else if (greater(earnings, c.f64(at + row_at::high))) {
            if (tier > 2) {
                set(verdict_lower_more);
            } else {
                const auto manager = manager_value();
                if (manager > 0x1676 && manager > rep - 0x2ee - adjust) set(verdict_lower_more);
            }
        }
        // The owner's chance: a draw below the byte at +0x63 of his record (none when it is 0) must come up.
        if (const auto owner = c.u32(club + club_at::owner); owner) {
            const std::int8_t temper = c.i8(owner + owner_at::temper);
            const std::int32_t chance = temper == 0 ? 0 : draws.below(temper, 0x101c285c);
            if (chance == 0) return set(verdict_lower_more);
        }
        if (c.u32(club + club_at::owner) == 0 && draws.modulo(5, 0x101c29c4) == 0) return set(verdict_lower_more);
        set(verdict_none);
        // The amount asked: by the club's reputation.
        const std::int32_t rep2 = reputation();
        std::int32_t asked;
        if (rep2 > 0x1d4c) asked = (rep2 - 0x1d4c) * 0x1388 + 5000000;
        else if (rep2 > 0x109a) asked = (rep2 - 0x109a) * 0x3e8 + 2000000;
        else asked = rep2 * 100 + 50000;
        // The employer's 64-bit counter (words +0 high, +4 low) against it, signed (0x101c2a70..0x101c2a7c).
        const std::int64_t counter = std::int64_t(std::uint64_t(c.i32(employer)) << 32 | c.u32(employer + 4));
        if (counter < std::int64_t(asked)) {
            const auto owner = c.u32(club + club_at::owner);
            if (!owner || !c.u32(owner + owner_at::profile)) {
                if (draws.modulo(5, 0x101c2c2c) != 0) return set(verdict_lower_more);
            } else {
                const std::int8_t trait = c.i8(c.u32(owner + owner_at::profile) + 0x2f);
                const std::int32_t chance = trait == 0 ? 0 : draws.below(trait, 0x101c2ad0);
                if (chance > 1) return set(verdict_lower_more);
            }
        }
        c.put32(row() + row_at::amount, std::uint32_t(asked));
    }

    // ---- the other kinds (0x101c2c8c) ----
    void kind_other() {
        std::int8_t mood = 0;
        if (const auto owner = c.u32(club + club_at::owner); owner)
            if (const auto profile = c.u32(owner + owner_at::profile); profile)
                mood = std::int8_t(10 - c.u8(profile + 0x2f));
        c.set(3, employer);
        c.call(callee::relation, 0x101c2cc0);
        const std::int8_t relation = std::int8_t(c.r(3));
        switch (relation) {
            case 0: mood = std::int8_t(mood + 5); break;
            case -1: mood = std::int8_t(mood - 5); break;
            case 1: mood = std::int8_t(mood + 10); break;
            case 2: mood = std::int8_t(mood + 15); break;
            default: mood = std::int8_t(mood - 15); break;  // -2 and everything beyond the first four
        }
        const std::int8_t tier = standing();
        const std::int8_t bound = std::int8_t(scaled(tier > 4 ? 0x28 : tier > 0 ? pool_scale_b : pool_scale_a, mood));
        const std::int32_t value = manager_value();
        const std::int32_t rep = reputation();
        if (value < 0x1388 || value < rep - 0x5dc - adjust) return set(verdict_post);
        const auto at = row();
        const bool rival = c.i8(at + row_at::spare) != 0;
        if (rival) {
            if (!greater(c.f64(at + row_at::low), c.f64(at + row_at::high))) return set(verdict_post);
            set(tier > 2 || (value > 0x1676 && value > rep - 0x4e2 - adjust) ? verdict_lower_more : verdict_post);
            return;
        }
        if (c.i32(employer + 0x14) > 0x15f90) {
            if (!greater(c.f64(at + row_at::low), c.f64(at + row_at::high))) return set(verdict_post);
            set(tier > 2 || (value > 0x1676 - adjust && value > rep - 0x4e2 - adjust) ? verdict_lower_more : verdict_post);
            return;
        }
        const std::int32_t chance = bound == 0 ? 0 : draws.below(bound, 0x101c2f08);
        if (chance > draws.modulo(0x32, 0x101c3050)) {
            const auto owner = c.u32(club + club_at::owner);
            if (!owner || !c.u32(owner + owner_at::profile)) {
                const auto spare = draws.modulo(10, 0x101c3110);
                c.put8(row() + row_at::spare, std::uint32_t(spare + 5));
            } else {
                const std::int8_t temper = c.i8(owner + owner_at::temper);
                c.put8(row() + row_at::spare, temper > 5 ? c.u8(owner + owner_at::temper) - 5u : 5u);
            }
            c.set({{3, employer}, {4, c.u8(row() + row_at::spare)}});
            c.call(callee::set_rating, 0x101c317c);
            return set(verdict_none);
        }
        const auto later = row();
        if (!greater(c.f64(later + row_at::low), c.f64(later + row_at::high))) return set(verdict_post);
        if (standing() > 2) return set(verdict_lower_more);
        const auto manager = manager_value();
        set(manager > 0x1676 - adjust && manager > reputation() - 0x4e2 - adjust ? verdict_lower_more : verdict_post);
    }
};
}

std::int8_t assess_verdict(Cpu& c, std::uint32_t list, std::uint32_t record, std::uint32_t club, std::uint32_t person) {
    guest::Frame frame(c, 0x101c1e20, frame_size, {.first_gpr = 18});
    if (!record) {
        report_assertion(c, frame, 0x98, 0x50, 0x5a0b, {0x101c1e84, 0x101c1e88, 0x101c1ea4, 0x101c1eb4});
        return 0;
    }
    if (!club) {
        report_assertion(c, frame, 0x78, 0x48, 0x5a11, {0x101c1ee0, 0x101c1ee4, 0x101c1f00, 0x101c1f10});
        c.put8(record + 9, 3);
        return 3;
    }
    if (!person) {
        report_assertion(c, frame, 0x58, 0x40, 0x5a18, {0x101c1f44, 0x101c1f48, 0x101c1f64, 0x101c1f74});
        c.put8(record + 9, 3);
        return 3;
    }
    return Verdict(c, list, record, club, person).run(frame);
}
}
