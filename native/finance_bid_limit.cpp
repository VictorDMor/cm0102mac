// FINANCE.CPP, block bid: 0x100f5770, the most a club can bid.
//
// The funds in the request (a signed 64-bit word) less what the club already owes: wages of its staff,
// the open deals on the books, a plan cost by the manager's rating, the reputation and the standing of
// its grounds, with different scales by the league the club plays in. The result is a bid ceiling,
// cut by the kind of competition, by the other side's answer, and floored at 0. The original keeps the
// running amount in a register pair (r28:r27); here it is `bid`. Names are inferred from the listing
// and the callers.
//
// The frame words the original builds for each int-to-double and each fctiwz (0x43300000 and the biased
// integer, the stored result) are written at the original's offsets: later readers read them as stale stack.
#include "finance_bid.hpp"
#include "finance_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>
#include <initializer_list>

namespace cm::aot::finance {
using guest::Fpu;
using guest::Frame;

namespace {
namespace callee {
constexpr std::uint32_t person_counts = 0x100d3040,   // (person, 0): a byte, non-zero for a person whose pay counts
    human_managed = 0x1014c2e0,                      // (club, a, b, 0, 0): whether a human manages the club (club_control.hpp)
    pending_deals = 0x103f45a0,                       // (owner, club, out list): count of 0x1c-byte deals
    contract_of = 0x10082f90,                         // (table, person, club): a contract (word +0xc: wage) or 0
    post_of = 0x10082f20,                             // (table, person): a post (word +4: the club) or 0
    staff_list = 0x1007bfc0,                          // (table, club, out list): count of 0x88-byte entries
    free_block = 0x10775170,                          // (block)
    to_integer = 0x10773de0,                          // f1 -> r3:r4 (64 bits)
    to_double = 0x10773c80,                           // r3:r4 (signed 64 bits) -> f1
    to_double_unsigned = 0x10773d30,                  // r3:r4 (unsigned 64 bits) -> f1
    agent_of = 0x106fdb00,                            // (object, person): a record or 0
    answer_of = 0x106fe280;                           // (record, other): a byte
}
namespace slot {  // table-of-contents words (each holds the address of a cell)
constexpr std::int32_t clubs = -0x7be8, people_count = -0x7be4, people = -0x7be0, deal_table = -0x7b7c,
                       post_cell = -0x7b78, deal_owner = -0x7b40, leagues = -0x7b5c, own_kind = -0x7af4,
                       gate_table = -0x79f0, negotiating = -0x797c, negotiation = -0x7978;
}
namespace club_field {
constexpr std::uint32_t income_scale = 0x08, bonus = 0x14, competition = 0x2c, league = 0x30, ground = 0x3c,
                        manager = 0x80, agent = 0x90, staff_slots = 0x98, reputation = 0x1ec, ground_kind = 0x23d,
                        board_flag = 0x23e, standing = 0x240, ground_flag = 0x241;
}
namespace pool {  // offsets in the double pool
constexpr std::uint32_t hundred = 0x00, upper = 0x18, mark = 0x38, magic = 0x50, scale = 0x60, half = 0x78,
                        whole = 0xe0;
}

constexpr std::int64_t wide(std::int32_t v) { return v; }
constexpr std::int64_t zero_extended(std::uint32_t v) { return v; }
constexpr std::uint32_t high(std::int64_t v) { return std::uint32_t(std::uint64_t(v) >> 32); }
constexpr std::uint32_t low(std::int64_t v) { return std::uint32_t(v); }
constexpr std::int64_t join(std::uint32_t hi, std::uint32_t lo) { return std::int64_t((std::uint64_t(hi) << 32) | lo); }
constexpr std::int64_t plus(std::int64_t a, std::int64_t b) { return std::int64_t(std::uint64_t(a) + std::uint64_t(b)); }
constexpr std::int64_t minus(std::int64_t a, std::int64_t b) { return std::int64_t(std::uint64_t(a) - std::uint64_t(b)); }
constexpr std::int8_t signed_byte(std::uint32_t v) { return std::int8_t(v); }
constexpr std::uint32_t word(std::int32_t v) { return std::uint32_t(v); }
constexpr std::int32_t product(std::int32_t a, std::int32_t b) { return std::int32_t(std::uint32_t(a) * std::uint32_t(b)); }

class BidLimit {
public:
    BidLimit(Cpu& c, Frame& frame, std::uint32_t request, std::uint32_t other, std::uint32_t capped)
        : c(c), fpu(c), frame(frame), request(request), other(other), capped(capped), doubles(c.toc(toc::doubles)) {}

    std::int64_t run(std::uint32_t squad, std::uint32_t checked, std::uint32_t deals);

private:
    Cpu& c;
    Fpu fpu;
    Frame& frame;
    std::uint32_t request, other, capped, doubles;
    std::uint32_t club = 0, league_entry = 0, competition = 0;
    std::int64_t bid = 0;
    std::uint32_t wages = 0, others = 0;  // the original's r29 and r30: what the contracts and the other deals cost
    std::int32_t level = 0;               // the plan level (r17), a signed byte
    std::int32_t reputation = 0;          // r16, once read
    std::int64_t extra = 0;               // r19:r20, the amount past the allowance

    double pool_double(std::uint32_t at) const { return c.f64(doubles + at); }
    double magic() const { return pool_double(pool::magic); }
    // The int-to-double the original builds in the frame: 0x43300000 and the biased word, at `at`.
    double stored(std::int32_t at, std::int32_t value) {
        c.put32(frame.at(at), 0x43300000);
        c.put32(frame.at(at + 4), word(value) ^ 0x80000000u);
        return c.f64(frame.at(at));
    }
    // fctiwz stored with stfd at `at`, the low word read back with lwz.
    std::int32_t truncated(std::int32_t at, double value) {
        c.put64(frame.at(at), fpu.fctiwz(0, value));
        return std::int32_t(c.u32(frame.at(at + 4)));
    }
    // pool[factor] * (value - m), the value stored as an int-to-double at `at`.
    double scaled(std::uint32_t factor, std::int32_t value, std::int32_t at) {
        const double source = stored(at, value);
        const double converted = fpu.fsub(source, magic());
        return fpu.fmul(pool_double(factor), converted);
    }
    void keep_bid() { c.set({{27, low(bid)}, {28, high(bid)}}); }
    bool in_kinds(std::initializer_list<std::int32_t> slots) const {
        const auto kind = c.u32(competition + 0x68);
        for (const auto at : slots)
            if (kind == c.u32(c.toc(at))) return true;
        return false;
    }
    std::uint32_t own_club() const { return club_record(c, c.u32(request + 8)); }
    std::int64_t funds() const { return std::int64_t(c.u64(request)); }

    std::int64_t to_wide(double value, std::uint32_t back) {
        c.set_f(1, value);
        c.call(callee::to_integer, back);
        return join(c.r(3), c.r(4));
    }
    double from_wide(std::int64_t value, std::uint32_t entry, std::uint32_t back) {
        c.set({{3, high(value)}, {4, low(value)}});
        c.call(entry, back);
        return c.f(1);
    }

    void staff_and_deals(std::uint32_t squad, std::uint32_t deals);
    void plan_level();
    void plan_cost();
    void ground_costs();
    void league_and_standing();
    bool allowance();
    void bonuses();
    void ladder();
    void from_answer();
    // The adjustment of a bid above `threshold` through the original's multiply-add.
    void levy(std::int64_t threshold, std::uint32_t factor, std::uint32_t constant, std::uint32_t site,
              std::uint32_t back_first, std::uint32_t back_second) {
        if (bid <= threshold) return;
        const double over = from_wide(minus(bid, threshold), callee::to_double, back_first);
        const double result = fpu.multiply_add(site, pool_double(factor), over, pool_double(constant));
        bid = to_wide(result, back_second);
        keep_bid();
    }
};

std::int64_t BidLimit::run(std::uint32_t squad, std::uint32_t checked, std::uint32_t deals) {
    c.put32(frame.at(0x3c), 0);
    c.put32(frame.at(0x38), 0);
    c.put8(frame.at(0xeb), checked);
    club = own_club();
    bid = funds();
    c.set({{13, deals}, {14, c.toc(slot::own_kind)}, {17, 0}, {18, c.toc(slot::post_cell)}, {19, c.toc(slot::people)},
           {20, c.toc(slot::deal_table)}, {21, c.toc(slot::clubs)}, {22, doubles}, {23, c.toc(slot::people_count)},
           {24, request}, {25, other}, {26, capped}, {29, 0}, {30, 0}, {31, club}});
    keep_bid();

    staff_and_deals(squad, deals);
    // A club with no league, or in a league the table has no entry for, keeps the sum so far.
    league_entry = c.u32(club + club_field::league);
    c.set(13, league_entry);
    if (!league_entry) return bid;
    if (c.u32(c.u32(c.toc(slot::leagues)) + c.u32(league_entry) * 4) == 0) return bid;
    plan_level();
    plan_cost();
    ground_costs();
    league_and_standing();
    if (allowance()) bonuses();
    ladder();
    from_answer();
    return bid;
}

// Phases 1-3: the staff on the books (50 slots), the open deals and the club's staff list.
void BidLimit::staff_and_deals(std::uint32_t squad, std::uint32_t deals) {
    const auto people_count_cell = c.toc(slot::people_count);
    if (signed_byte(squad) != 0) {
        for (std::int32_t i = 0; std::int8_t(i) < 50; ++i) {
            const auto person = c.u32(club + club_field::staff_slots + 4 * word(i));
            c.set({{15, person}, {16, word(i)}, {17, 4 * word(i)}});
            if (person == 0 || !(c.i32(person) < c.i32(people_count_cell))) continue;
            if (signed_byte(invoke(c, callee::person_counts, 0x100f5820, {person, 0})) == 0) continue;
            bid = plus(bid, wide(c.i32(person + 8)));
            keep_bid();
        }
        c.set({{16, 50}, {17, 200}});
    }
    if (capped) c.put8(capped, 0);
    if (signed_byte(deals) == 0) return;
    if (signed_byte(invoke(c, callee::human_managed, 0x100f5884, {club, 1, 1, 0, 0})) == 0) return;

    const auto count = std::int8_t(invoke(c, callee::pending_deals, 0x100f58a0,
                                          {c.u32(c.toc(slot::deal_owner)), club, frame.at(0x3c)}));
    c.set({{13, 0}, {15, word(count)}, {16, 0}});
    std::int32_t index = 0;
    for (; std::int8_t(index) < count; ++index) {
        const auto list = c.u32(frame.at(0x3c));
        const auto entry = list + 28u * word(index);
        c.set({{13, word(index)}, {16, 28u * word(index)}});
        if (c.i8(entry + 0x10) == 0) {
            if (other == 0 || c.u32(entry + 8) != c.u32(other)) {
                others += c.u32(entry + 0x14);
                c.set(30, others);
            }
            continue;
        }
        const auto kind = c.i8(entry + 0x19);
        if (kind != 3 && kind != 4 && kind != 6 && kind != 5) continue;
        // An earlier deal with the same person has been counted already.
        std::int32_t earlier = 0;
        bool found = false;
        for (; earlier < std::int8_t(index) && !found; ++earlier)
            if (c.u32(entry + 8) == c.u32(list + 28u * word(earlier) + 8)) found = true;
        if (!found) {
            others -= c.u32(entry + 0x14);
            c.set(30, others);
        }
        const auto person = c.u32(c.toc(slot::people)) + c.u32(list + 28u * word(earlier) + 8) * 0x70;
        const auto contract = invoke(c, callee::contract_of, 0x100f5974, {c.u32(c.toc(slot::deal_table)), person, own_club()});
        if (contract != 0) {
            wages -= c.u32(contract + 0xc);
            c.set(29, wages);
        }
    }
    c.set({{13, word(index)}, {16, 28u * word(index)}});
    if (const auto list = c.u32(frame.at(0x3c)); list != 0) invoke(c, callee::free_block, 0x100f59cc, {list});

    const auto staff_count = std::int8_t(invoke(c, callee::staff_list, 0x100f59dc,
                                                {c.u32(c.toc(slot::deal_table)), club, frame.at(0x38)}));
    c.set({{23, word(staff_count)}, {16, 0}, {17, 0}});
    std::int32_t at = 0;
    for (; std::int8_t(at) < staff_count; ++at) {
        const auto entry = c.u32(frame.at(0x38)) + 0x88u * word(at);
        c.set({{15, entry}, {16, word(at)}, {17, 0x88u * word(at)}});
        if (c.u8(entry + 0x64) == 4) continue;
        const auto person = c.u32(c.toc(slot::people)) + c.u32(entry) * 0x70;
        const auto post = invoke(c, callee::post_of, 0x100f5a1c, {c.u32(c.toc(slot::deal_table)), person});
        c.put32(c.toc(slot::post_cell), post);
        const auto post_club = post ? club_record(c, c.u32(post + 4)) : 0u;
        const auto mine = own_club();
        if (post_club == mine) {
            const auto person_again = c.u32(c.toc(slot::people)) + c.u32(entry) * 0x70;
            const auto contract = invoke(c, callee::contract_of, 0x100f5a70, {c.u32(c.toc(slot::deal_table)), person_again, mine});
            if (contract != 0) {
                wages -= c.u32(contract + 0xc);
                c.set(29, wages);
            }
        }
        wages += c.u32(entry + 0xc);
        others += c.u32(entry + 8);
        c.set({{29, wages}, {30, others}});
    }
    c.set({{16, word(at)}, {17, 0x88u * word(at)}});
    if (const auto list = c.u32(frame.at(0x38)); list != 0) invoke(c, callee::free_block, 0x100f5ab4, {list});
}

// Phase 4 head: the plan level by the manager's rating, and the first cost.
void BidLimit::plan_level() {
    const auto manager = c.u32(club + club_field::manager);
    const auto profile = manager ? c.u32(manager + 0x1c) : 0u;
    if (manager && profile) {
        level = signed_byte(c.u8(profile + 0x2f) + 6u);
        const auto temper = c.u8(manager + 0x63);
        if (std::int8_t(temper) > 0xf) level = signed_byte(word(level) - (std::uint32_t(temper) - 0xfu));
    } else {
        level = 0x10;
    }
    competition = c.u32(club + club_field::competition);
    if (competition && c.u32(competition + 0x68) == c.u32(c.toc(slot::own_kind))) level = signed_byte(word(level) + 5u);
    c.set({{15, competition}, {17, word(level)}});
}

void BidLimit::plan_cost() {
    const double a = fpu.fsub(stored(0x48, c.i32(request + 0x1c)), magic());
    const double ratio = fpu.fdiv(a, pool_double(pool::hundred));
    const double b = fpu.fsub(stored(0x50, std::int32_t(c.u32(request + 0xc) + wages)), magic());
    const double l = fpu.fsub(stored(0x58, level), magic());
    const double difference = fpu.fsub(b, ratio);
    const auto taken = to_wide(fpu.fmul(l, difference), 0x100f5bbc);
    const auto remaining = minus(bid, taken);
    c.set({{18, low(remaining)}, {16, high(remaining)}});

    // The cost of the club's ground, by its reputation (a plain product without one).
    const auto ground = c.u32(club + club_field::ground);
    const auto rep = std::int32_t(c.i16(club + club_field::reputation));
    const auto ground_cost = [&](std::uint32_t factor, std::uint32_t back) {
        const auto base = product(std::int32_t(c.u32(ground + 4)), level);
        bid = minus(remaining, to_wide(scaled(factor, base, 0x48), back));
    };
    if (ground == 0) {
        bid = minus(remaining, zero_extended(word(level) * 0x9c4u));
    } else if (c.i8(club + club_field::board_flag) == 0) {
        bid = minus(remaining, zero_extended(c.u32(ground + 4) * word(level) * 2u));
    } else if (rep > 0x1676) {
        ground_cost(0xa0, 0x100f5c1c);
    } else if (rep > 0x109a) {
        ground_cost(0x1b0, 0x100f5c6c);
    } else if (rep > 0x4e2 && c.u8(club + club_field::ground_kind) == 1) {
        ground_cost(0xb8, 0x100f5cc8);
    } else {
        bid = minus(remaining, zero_extended(c.u32(ground + 4) * word(level)));
    }
    keep_bid();
}

// The standing of the club's facilities, in cost per level.
void BidLimit::ground_costs() {
    const std::int32_t standing = c.i8(club + club_field::standing);
    std::int32_t per_level;
    if (standing < 6) per_level = standing * 200;
    else if (standing < 0x10) per_level = (standing - 4) * 500 + 1000;
    else per_level = (standing - 0xf) * 2000 + 6000;
    const auto cost = truncated(0x50, scaled(pool::scale, product(level, per_level), 0x48));
    bid = minus(bid, zero_extended(word(cost)));
    keep_bid();
}

void BidLimit::league_and_standing() {
    reputation = c.i16(club + club_field::reputation);
    c.set(16, word(reputation));
    const auto base = minus(bid, zero_extended(word(product(reputation, level))));
    c.set({{18, high(base)}, {19, low(base)}});
    const auto income = std::int32_t(c.u32(club + club_field::income_scale));
    const auto reputation_income = product(reputation, income);
    double l_scaled, income_scaled, factor;
    if (reputation < 4000) {
        l_scaled = fpu.fsub(stored(0x48, level), magic());
        income_scaled = fpu.fsub(stored(0x50, reputation_income), magic());
        factor = pool_double(0x1b8);
    } else {
        l_scaled = fpu.fsub(stored(0x58, level), magic());
        income_scaled = fpu.fsub(stored(0x40, reputation_income), magic());
        factor = pool_double(0x180);
    }
    const double inner = fpu.fmul(factor, income_scaled);
    const auto added = to_wide(fpu.fmul(l_scaled, inner), reputation < 4000 ? 0x100f5ecc : 0x100f5f20);
    bid = plus(base, added);
    keep_bid();

    // The league's power adds a cost by the kind of competition.
    // league_entry (r13) is never 0 here: 0x100f5ab8 returned for that (the null side of this test is dead).
    if (league_entry != 0 && competition != 0) {
        const std::int32_t power = c.i16(league_entry + 0x18);
        const auto square = product(power, power);
        std::int32_t rate = 750;
        if (in_kinds({-0x7a10, -0x7a14, -0x7a18})) rate = 5000;
        else if (in_kinds({slot::own_kind, -0x7a00})) rate = 2500;
        bid = plus(bid, wide(product(square, rate)));
        keep_bid();
    }

    // The reputation squared, scaled by the level.
    const auto squared = product(reputation, reputation);
    double reputation_scaled, level_scaled;
    if (c.i8(club + club_field::board_flag) != 0) {
        const double inner_squared = fpu.fmul(pool_double(0x180), fpu.fsub(stored(0x48, squared), magic()));
        level_scaled = fpu.fsub(stored(0x40, level), magic());
        reputation_scaled = inner_squared;
    } else {
        const double inner_squared = fpu.fmul(pool_double(0x1b8), fpu.fsub(stored(0x58, squared), magic()));
        level_scaled = fpu.fsub(stored(0x50, level), magic());
        reputation_scaled = inner_squared;
    }
    const auto squared_cost = to_wide(fpu.fmul(level_scaled, reputation_scaled),
                                      c.i8(club + club_field::board_flag) != 0 ? 0x100f604c : 0x100f60a0);
    bid = plus(bid, squared_cost);
    keep_bid();

    // A club with a booked bonus slot and a settled ground pays by reputation, scaled by the competition.
    if (std::int32_t(c.u32(club + club_field::bonus)) != -1 && c.u8(club + club_field::ground_kind) == 1) {
        std::int32_t amount = product(reputation, 300);
        if (competition == 0) {
            amount = 0;
        } else if (in_kinds({-0x7a10, slot::own_kind, -0x7a14, -0x7a00, -0x7a1c, -0x7a18})) {
            // unscaled
        } else if (in_kinds({-0x7a28, -0x7514, -0x7510, -0x79fc, -0x75b0})) {
            amount = truncated(0x48, scaled(pool::half, amount, 0x40));
        } else if (in_kinds({-0x750c, -0x7504})) {
            amount = truncated(0x48, scaled(0x90, amount, 0x40));
        } else {
            amount = truncated(0x48, scaled(pool::mark, amount, 0x40));
        }
        bid = plus(bid, wide(amount));
        keep_bid();
    }
}

// The part of the debt above the allowance comes off the bid (only the low word counts: quirk kept).
// Returns whether the bid is still positive, with the value block run.
bool BidLimit::allowance() {
    const auto difference = minus(std::int64_t(c.u64(request + 0x110)), std::int64_t(c.u64(request + 0x118)));
    const std::int32_t positive = difference > 0 ? std::int32_t(low(difference)) : 0;
    const auto beyond = minus(wide(positive), 8'000'000);
    const std::int32_t excess = beyond >= 0 ? std::int32_t(low(beyond)) : 0;
    const std::int32_t other_deals = std::int32_t(others) > 0 ? std::int32_t(others) : 0;
    extra = plus(wide(excess), wide(other_deals));
    bid = minus(bid, zero_extended(low(extra)));
    c.set({{17, high(bid)}, {18, low(bid)}, {19, high(extra)}, {20, low(extra)}});
    keep_bid();
    if (bid <= 0) return false;

    // A bid beyond a share of the funds is cut to that share (f31 holds it, as the original keeps it).
    const double whole = fpu.fmul(pool_double(pool::whole), from_wide(funds(), callee::to_double, 0x100f6338));
    c.set_f(31, whole);
    const double now = from_wide(bid, callee::to_double, 0x100f634c);
    fpu.fcmp(0, now, whole);
    if (now < whole) {
        const double cap = pool_double(0x1c0);
        fpu.fcmp(0, whole, cap);
        if (!(whole < cap)) c.set_f(31, cap);
        bid = to_wide(c.f(31), 0x100f6370);
        keep_bid();
        return false;
    }
    return true;
}

// The value block: a bid worth pulling back by the club's value, in a band by the competition.
void BidLimit::bonuses() {
    const auto squared = product(reputation, reputation);
    std::int32_t value = truncated(0x48, fpu.fmul(pool_double(pool::mark), fpu.fsub(stored(0x40, squared), magic())));
    const auto manager = c.u32(club + club_field::agent);
    if (manager != 0 && reputation != 0) {
        if (const auto profile = c.u32(manager + 0x1c); profile != 0) {
            const std::int32_t quality = c.i16(profile + 0x2a);
            const double q = fpu.fsubs(stored(0x40, quality), magic());
            const double r = fpu.fsubs(stored(0x48, reputation), magic());
            double ratio = fpu.fdivs(q, r);
            fpu.fcmp(0, ratio, pool_double(pool::upper));
            if (ratio < pool_double(pool::upper)) {
                const double mark = pool_double(pool::mark);
                fpu.fcmp(0, ratio, mark);
                if (ratio < mark) ratio = double(c.f32(c.toc(toc::floats) + 0xc));
                const double v = fpu.fsubs(stored(0x40, value), magic());
                const double w = fpu.fmuls(v, ratio);
                value = truncated(0x48, fpu.fmuls(ratio, w));
            }
        }
    }

    bool capped_kind = false;
    if (c.i8(club + club_field::ground_flag) != 0) capped_kind = true;
    else if (competition == 0 ||
             in_kinds({-0x7514, -0x7504, -0x7a20, -0x79fc, -0x750c, -0x7504, -0x7594, -0x7a1c, -0x75b0, slot::own_kind}))
        capped_kind = true;

    // The value cannot be more than the funds less a margin; in a competition of the club's own kind the margin is
    // smaller, and a capped kind also keeps the value inside what is left of a limit.
    if (competition == 0 || c.u32(competition + 0x68) == c.u32(c.toc(slot::own_kind))) {
        const auto lower = minus(funds(), 15'000'000);
        if (wide(value) < lower) value = std::int32_t(low(lower));
        // Always true here (0x100f6540..0x100f6558 enters this side only for a missing competition, set to 1
        // at 0x100f653c, or the club's own kind, which the list ending at 0x100f6538 holds; the flag also
        // comes from 0x100f6494): the original's beq at 0x100f659c is never taken.
        if (capped_kind) {
            const auto left = minus(35'000'000, extra);
            if (left < wide(value)) value = std::int32_t(low(left));
        }
    } else {
        const auto lower = minus(funds(), 10'000'000);
        if (wide(value) < lower) value = std::int32_t(low(lower));
        if (capped_kind) {
            const auto left = minus(40'000'000, extra);
            if (left < wide(value)) value = std::int32_t(low(left));
        }
    }
    if (value < 50000) value = 50000;
    c.set(17, word(value));
    if (wide(value) < bid) {
        const double over = from_wide(minus(bid, wide(value)), callee::to_double, 0x100f669c);
        const double base = fpu.fsub(stored(0x40, value), magic());
        const double result = fpu.multiply_add(0x100f66bc, pool_double(pool::whole), over, base);
        bid = to_wide(result, 0x100f66c4);
        keep_bid();
    }
}

// The table of gate sums, the reputation floor and the caps by reputation; then the bid's cut by competition.
void BidLimit::ladder() {
    if (const auto table = c.u32(c.toc(slot::gate_table)); table != 0) {
        // club == 0 can only be a table base plus an index wrapping to 0: 0x100f5ab4 (lwz 0x30(r31)) faulted first
        // on both sides (case "club record at address 0").
        const std::int32_t sum = club != 0 ? c.i32(c.u32(table) + c.u32(club) * 0x4c + 0x28) : 0;
        bid = plus(bid, wide(sum));
        keep_bid();
    }
    // league_entry: as above (r13 is held, 0x100f5ab8).
    if (reputation > 0x1388 && league_entry != 0) {
        const std::int32_t power = c.i16(league_entry + 0x18);
        const double r = fpu.fsub(stored(0x48, reputation), magic());
        double w = fpu.multiply_add(0x100f675c, pool_double(0x1c8), r, pool_double(0x1d0));
        const double p = fpu.fsub(stored(0x40, power), magic());
        w = fpu.fmul(p, w);
        w = fpu.fdiv(w, pool_double(0x1d8));
        const auto target = to_wide(w, 0x100f6774);
        if (bid < target) {
            bid = target;
            if (target < 50000) bid = 50000;
        }
        keep_bid();
    }
    if (reputation < 0xdac) {
        std::int32_t cap = product(reputation, 0xfa);
        if (c.u8(club + club_field::ground_kind) == 1) cap = product(reputation, 0x1f4);
        if (wide(cap) < bid) bid = wide(cap);
        keep_bid();
    }
    if (bid < 0) bid = 0;
    keep_bid();

    if (competition == 0) return;
    if (in_kinds({-0x7514, -0x7504, -0x750c, -0x757c, -0x7a20, -0x7594, -0x7574, -0x7578, -0x7508})) {
        bid = bid / 2;  // the compiler's signed halving, rounding toward zero
        keep_bid();
        levy(2'500'000, pool::whole, 0x1c0, 0x100f693c, 0x100f6934, 0x100f6944);
    } else if (in_kinds({-0x7510, -0x79fc})) {
        levy(15'000'000, pool::scale, 0x1e0, 0x100f69b8, 0x100f69b0, 0x100f69c0);
    } else if (in_kinds({-0x7b68})) {
        levy(2'500'000, pool::scale, 0x1c0, 0x100f6a24, 0x100f6a1c, 0x100f6a2c);
    } else if (in_kinds({slot::own_kind, -0x7a00, -0x7a1c, -0x7a28})) {
        levy(17'500'000, pool::scale, 0x1e8, 0x100f6abc, 0x100f6ab4, 0x100f6ac4);
    }
}

// The other side's answer (if a negotiation is running) can cut the bid again.
void BidLimit::from_answer() {
    if (other == 0) return;
    if (signed_byte(c.u8(frame.at(0xeb))) == 0) return;
    if (c.i8(c.toc(slot::negotiating)) == 0) return;
    if (c.u32(c.toc(slot::negotiation)) == 0) return;
    if (signed_byte(invoke(c, callee::human_managed, 0x100f6b18, {club, 0, 1, 0, 0})) == 0) return;
    const auto record = invoke(c, callee::agent_of, 0x100f6b30, {c.u32(c.toc(slot::negotiation)), c.u32(club + club_field::agent)});
    if (record == 0) return;
    const auto answer = std::int8_t(invoke(c, callee::answer_of, 0x100f6b40, {record, other}));

    const auto square_over = [&](std::uint32_t divisor) {
        const std::int32_t rep = c.i16(club + club_field::reputation);
        const double a = fpu.fsub(stored(0x40, rep), magic());
        const double b = fpu.fsub(stored(0x48, rep), magic());
        return truncated(0x50, fpu.fdiv(fpu.fmul(a, b), pool_double(divisor)));
    };
    const auto small_league = [&] {
        const auto record_of = c.u32(club + club_field::competition);
        return record_of != 0 && c.i8(record_of + 0x11f) < 9 && c.i16(club + club_field::reputation) < 0x109a;
    };
    std::int32_t amount;
    if (answer == 2) amount = square_over(0x1f0);
    else if (answer == 3) amount = std::int32_t(low(bid));
    else if (answer == 1) amount = small_league() ? square_over(0xf8) : square_over(0x1f8);
    else amount = small_league() ? square_over(0x1c8) : square_over(0xf8);

    // The agent's rating against the club's reputation, clamped, scales the amount.
    if (const auto agent = c.u32(club + club_field::agent); agent != 0) {
        if (const auto profile = c.u32(agent + 0x1c); profile != 0) {
            if (const std::int32_t rep = c.i16(club + club_field::reputation); rep != 0) {
                const std::int32_t quality = c.i16(profile + 0x2a);
                const double q = fpu.fsub(stored(0x40, quality), magic());
                const double r = fpu.fsub(stored(0x48, rep), magic());
                double ratio = fpu.fdiv(q, r);
                const double top = pool_double(pool::upper);
                fpu.fcmp(0, ratio, top);
                if (ratio > top) {
                    ratio = top;
                } else {
                    const double bottom = pool_double(pool::whole);
                    fpu.fcmp(0, ratio, bottom);
                    if (ratio < bottom) ratio = bottom;
                }
                const double v = fpu.fsub(stored(0x40, amount), magic());
                amount = truncated(0x48, fpu.fmul(ratio, v));
            }
        }
    }
    if (c.u32(club + club_field::league) != 0) {
        const auto record_of = c.u32(club + club_field::competition);
        const std::int32_t level_byte = c.i8(record_of + 0x11f);
        const double v = fpu.fsub(stored(0x40, level_byte), magic());
        double ratio = fpu.fmul(pool_double(0x200), v);
        const double bottom = pool_double(pool::scale);
        fpu.fcmp(0, ratio, bottom);
        if (ratio < bottom) {
            ratio = bottom;
        } else {
            const double top = pool_double(pool::upper);
            fpu.fcmp(0, ratio, top);
            if (ratio > top) ratio = top;
        }
        const double w = fpu.fsub(stored(0x40, amount), magic());
        amount = truncated(0x48, fpu.fmul(ratio, w));
    }
    // A club in the red (funds below zero) is offered a share by the other side's bonus slot.
    if (std::int32_t(c.u32(request)) < 0) {
        if (const std::int32_t rep = c.i16(club + club_field::reputation); rep != 0) {
            if (const auto slot_record = c.u32(other + 0x14); slot_record != 0) {
                const std::int32_t quality = c.i16(slot_record + 0xa);
                const double q = fpu.fsub(stored(0x40, quality), magic());
                const double r = fpu.fsub(stored(0x48, rep), magic());
                double ratio = fpu.fdiv(q, r);
                const double bottom = pool_double(0x138);
                fpu.fcmp(0, ratio, bottom);
                if (ratio < bottom) {
                    ratio = bottom;
                } else {
                    const double top = pool_double(0x208);
                    fpu.fcmp(0, ratio, top);
                    if (ratio > top) ratio = top;
                }
                amount = truncated(0x40, fpu.fmul(pool_double(pool::half), ratio));
            } else {
                const double v = fpu.fsub(stored(0x40, amount), magic());
                amount = truncated(0x48, fpu.fmul(pool_double(pool::half), v));
            }
        }
    }
    if (answer == 3 || !(wide(amount) < bid)) return;
    c.set(13, word(amount));
    const double over = from_wide(minus(bid, wide(amount)), callee::to_double_unsigned, 0x100f6f84);
    const auto raise = truncated(0x40, fpu.fmul(pool_double(0x210), over));
    bid = wide(std::int32_t(word(amount) + word(raise)));
    if (bid > 125000) {
        const auto ceiling = truncated(0x48, scaled(0x218, amount, 0x40));
        if (wide(ceiling) < bid) bid = wide(ceiling);
    }
    if (capped) c.put8(capped, 1);
}
}

std::int64_t bid_limit(Cpu& c, std::uint32_t request, std::uint32_t other, std::uint32_t squad,
                       std::uint32_t capped, std::uint32_t checked, std::uint32_t deals) {
    Frame frame(c, 0x100f5770, 0xc0, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 31});
    BidLimit limit(c, frame, request, other, capped);
    return limit.run(squad, checked, deals);
}
}
