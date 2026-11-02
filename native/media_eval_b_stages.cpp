// 0x10662d30..0x10664778 of the press's reaction (0x106620b0): once a person has passed the gate, the flag
// bits of the story nudge his morale (one stage, the first bit set, in the order 0x1, 0x2, 0x4, 0x8, 0x10,
// 0x20, 0x200), then every stage falls into the same tail: with flag 0x40 a lucky person's tally byte grows.
#include "media_eval_b_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;

namespace eval_b {
namespace {
// The visited person's traits, signed bytes: ambition (+0x63) and sportsmanship (+0x67).
std::int32_t ambition(Reaction& r) { return r.who(r.person).ambition().get(); }
std::int32_t sportsmanship(Reaction& r) { return r.who(r.person).sportsmanship().get(); }

// random(n) against the next sample reduced by a constant: `cmpw r15,r0`, as -1, 0 or 1.
std::int32_t draw_versus(Reaction& r, std::int32_t n, std::uint32_t draw_back, const Reciprocal& by,
                         std::uint32_t reduce_back) {
    const auto rolled = r.draw(n, draw_back);
    const auto bound = r.beaten_by(by, reduce_back);
    return compare(r.c, rolled, bound);
}

// The last nudge of the third, fourth and fifth stages: the next sample's remainder by 5, as a byte (the sample
// stays in r6).
void nudge_by_five(Reaction& r, std::uint32_t advance_back, std::uint32_t morale_back) {
    const auto rest = r.beaten_by(divisors::by_5, advance_back);
    r.c.set(6, r.rng.sample());
    r.morale(r.person, byte_word(std::uint32_t(rest)), morale_back);
}

// random(4) - random(4) as a byte: the compiler overlapped the pair (the first remainder waits in r15 and the second
// refill sees the first sample's scratch in r3-r5), as eight_apart in the sister routine.
std::int32_t four_apart(Reaction& r, std::uint32_t first_back, std::uint32_t second_back) {
    r.rng.advance(first_back);
    const auto x = r.rng.sample(), sign = x >> 31;
    const auto first = power_remainder(x, 2, sign);
    r.drawn = std::uint32_t(first);
    r.rng.advance(second_back, {{3, sign}, {4, (x & 3) << 30}, {5, x}});
    const auto y = r.rng.sample();
    const auto second = power_remainder(y, 2, y >> 31);
    return byte_word(std::uint32_t(first - second));
}

// ---- the draws whose range is a float expression
// 0x10663878: random(n) with n = the person's ambition plus the factor, truncated (kept in r16, the result in r15).
// Beyond 16 bits it takes the float form of the original, in the schedule of this site.
std::int32_t ranged_draw(Reaction& r, std::int32_t n) {
    Cpu& c = r.c;
    if (!nonzero(c, n)) {
        r.drawn = 0;
        return 0;
    }
    r.rng.advance(0x10663898);
    if (!beyond_table(n)) return narrow_site(c, r.rng, n, 15);
    const Floats pool(c, r.pool);
    const auto d = wide_draw(c, r.fpu, r.rng, pool, n, r.frame.at(0x58), r.frame.at(0x60), r.frame.at(0x68));
    r.drawn = std::uint32_t(d.part);
    leave(c, {{4, 0x80008001u}, {5, d.multiple}, {6, std::uint32_t(d.whole.value)}, {7, d.whole.sign}, {8, d.entry}});
    compare(c, n, n > 0xffff ? 0xffff : -0xffff);
    enter(c, 0x10663970, {std::uint32_t(d.coarse.value)});
    const auto tenth = rounded_tenth(random_below(c, d.coarse.value));
    const auto limit = std::int32_t(std::uint32_t(n) - 1);
    const auto total = std::int32_t(std::uint32_t(d.part) + std::uint32_t(tenth.value));
    leave(c, {{3, std::uint32_t(tenth.value)}, {4, tenth.sign}});
    // part + a tenth of the finer draw, at most n - 1 (quirk: no lower bound).
    const auto value = compare(c, total, limit) > 0 ? limit : total;
    r.drawn = std::uint32_t(value);
    return value;
}

// The stack slots (frame offsets) a bounded draw uses: the original recomputes the range, the factor's float
// expression, in a fresh stack pair at every use.
struct BoundSlots {
    std::uint32_t check;   // the range tested against +-0xffff (the narrow form divides by the pair at 0x38)
    std::uint32_t pair;    // the range as a double, for the float form's share
    std::uint32_t rest;    // the sample's remainder as a double
    std::uint32_t second;  // the range again, times 10
    std::uint32_t part;    // the float form's part
    std::uint32_t third;   // the range, after the finer draw: the limit n - 1
    std::uint32_t fourth;  // the range, when the limit is reached
};
constexpr BoundSlots first_bound{0x58, 0x60, 0x68, 0x48, 0x50, 0x40, 0x38};
constexpr BoundSlots second_bound{0x38, 0x40, 0x48, 0x58, 0x50, 0x60, 0x68};

// r16 = random(n) with n = trunc(single(0x50) + factor): nothing is drawn when the sum equals the pool's single at
// +8. The range is recomputed (fadds, fctiwz into a stack pair) at each of its uses, as the original does. Within
// 16 bits it is the sample's remainder by the range (the sign test is on a second evaluation); beyond, the float
// share of the range plus a tenth of a finer draw from 0x10647c00, at most n - 1.
std::int32_t bound_draw(Reaction& r, const BoundSlots& s, std::uint32_t refill_back, std::uint32_t call_back) {
    Cpu& c = r.c;
    Fpu& fpu = r.fpu;
    const auto widened = [&] { return fpu.fadds(r.single(0x50), r.factor); };
    const auto stored = [&](std::uint32_t at) {
        c.put64(r.frame.at(at), fpu.fctiwz(0, widened()));
        return std::int32_t(c.u32(r.frame.at(at + 4)));
    };
    const auto finish = [&](std::int32_t value) {
        r.range = std::uint32_t(value);
        return value;
    };
    const double sum = widened();
    c.set_f(1, sum);
    fpu.fcmp(0, sum, r.single(8));
    if (sum == r.single(8)) return finish(0);
    r.rng.advance(refill_back);
    c.set(3, 0xffff);
    bool narrow = false;
    if (compare(c, stored(s.check), 0xffff) <= 0) {
        c.set(3, std::uint32_t(-0xffff));
        narrow = compare(c, stored(s.check), -0xffff) >= 0;
    }
    if (narrow) {
        // The remainder of the sample by the range (the first evaluation, at 0x38), negated unless the second (0x40) is
        // positive.
        const double first_sum = widened();
        const auto entry = c.u16(c.u32(r.rng.cursor));
        const auto x = entry + c.u32(r.rng.seed);
        const double second_sum = widened();
        const auto first_bits = fpu.fctiwz(1, first_sum);
        const auto second_bits = fpu.fctiwz(0, second_sum);
        c.put64(r.frame.at(0x38), first_bits);
        const auto range = std::int32_t(c.u32(r.frame.at(0x3c)));
        c.put64(r.frame.at(0x40), second_bits);
        const auto sign_of = std::int32_t(c.u32(r.frame.at(0x44)));
        // divw by zero leaves 0 (the range can be 0 here: the sum is not the pool's single).
        const auto quotient = range ? divide(std::int32_t(x), range) : 0;
        compare(c, sign_of, 0);
        const auto rest = std::int32_t(x - std::uint32_t(quotient) * std::uint32_t(range));
        leave(c, {{3, std::uint32_t(quotient)}, {4, std::uint32_t(range)}, {5, x}});
        return finish(sign_of > 0 ? rest : std::int32_t(0u - std::uint32_t(rest)));
    }
    // Beyond 16 bits.
    const double first_sum = widened();
    const double second_sum = widened();
    const auto first_bits = fpu.fctiwz(1, first_sum);
    const auto second_bits = fpu.fctiwz(0, second_sum);
    c.put64(r.frame.at(s.check), first_bits);
    c.put64(r.frame.at(s.second), second_bits);
    const auto range = std::int32_t(c.u32(r.frame.at(s.check + 4)));
    const double ranged = fpu.fsub(stacked_integer(c, r.frame.at(s.pair), signed_word(range)), r.bias());
    const double share = fpu.fdiv(ranged, r.dbl(0));
    const auto entry = c.u16(c.u32(r.rng.cursor));
    const auto x = entry + c.u32(r.rng.seed);
    const auto whole = by_65535(x);
    const auto multiple = std::uint32_t(whole.value) * 0xffff, rest = x - multiple;
    const auto tenfold = c.u32(r.frame.at(s.second + 4)) * 10;
    const auto coarse = by_65535(tenfold);
    const double stacked_rest = stacked_integer(c, r.frame.at(s.rest), signed_word(std::int32_t(rest)));
    const double rest_float = fpu.fsub(stacked_rest, r.bias());
    c.put64(r.frame.at(s.part), fpu.fctiwz(0, fpu.fmul(share, rest_float)));
    const auto part = c.u32(r.frame.at(s.part + 4));
    r.range = part;
    c.set_f(1, r.bias());
    leave(c, {{4, 0x80008001u}, {5, multiple}, {6, 0xffff}, {7, whole.sign}, {8, entry}});
    enter(c, call_back, {std::uint32_t(coarse.value)});
    const auto tenth = rounded_tenth(random_below(c, coarse.value));
    const auto third = stored(s.third), limit = std::int32_t(std::uint32_t(third) - 1);
    leave(c, {{3, std::uint32_t(third)}, {4, tenth.sign}});
    const auto total = std::int32_t(part + std::uint32_t(tenth.value));
    if (compare(c, total, limit) <= 0) return finish(total);
    // At most n - 1 (quirk: no lower bound).
    const auto cap = stored(s.fourth);
    c.set(3, std::uint32_t(cap));
    return finish(std::int32_t(std::uint32_t(cap) - 1));
}

// ---- the stages
// Flag 0x4: random(4) - random(4) of morale.
void flag_4(Reaction& r) {
    const auto delta = four_apart(r, 0x10662d90, 0x10662ddc);
    r.morale(r.person, delta, 0x10662e1c);
}

// Flag 0x8: a person whose ambition draw beats the sample mod 20 and whose sportsmanship draw then falls short
// of one is lowered by 5; otherwise a sportsmanship draw above one raises him by the sample mod 5.
void flag_8(Reaction& r) {
    if (draw_versus(r, ambition(r), 0x10662e5c, divisors::by_20, 0x10662fb8) > 0) {
        if (draw_versus(r, sportsmanship(r), 0x1066302c, divisors::by_20, 0x10663188) < 0) {
            r.morale(r.person, -5, 0x106631d8);
            return;
        }
    }
    if (draw_versus(r, sportsmanship(r), 0x10663210, divisors::by_20, 0x1066336c) <= 0) return;
    nudge_by_five(r, 0x106633cc, 0x10663414);
}

// Flag 0x10: sportsmanship, then ambition, each beating a sample mod 20, raise him by the sample mod 5.
void flag_10(Reaction& r) {
    if (draw_versus(r, sportsmanship(r), 0x10663454, divisors::by_20, 0x106635b0) <= 0) return;
    if (draw_versus(r, ambition(r), 0x10663624, divisors::by_20, 0x10663780) <= 0) return;
    nudge_by_five(r, 0x106637e0, 0x10663828);
}

// Flag 0x20: a first draw over a range of ambition plus the factor against the sample mod 10; a person who
// passes draws sportsmanship against a range of the factor's float and is lowered by 8 when he comes out below
// it; then (also for the one who failed the first) sportsmanship against that range again, above it raises him.
void flag_20(Reaction& r) {
    Cpu& c = r.c;
    c.set_f(1, r.bias());
    const double base = r.fpu.fsubs(stacked_integer(c, r.frame.at(0x58), signed_word(ambition(r))), r.bias());
    c.put64(r.frame.at(0x60), r.fpu.fctiwz(0, r.fpu.fadds(base, r.factor)));
    r.range = c.u32(r.frame.at(0x64));
    const auto first = ranged_draw(r, std::int32_t(std::uint32_t(r.range)));
    const auto bound = r.beaten_by(divisors::by_10, 0x106639f4);
    if (compare(c, first, bound) > 0) {
        const auto roll = r.draw(sportsmanship(r), 0x10663a68);
        const auto limit = bound_draw(r, first_bound, 0x10663be0, 0x10663d08);
        if (compare(c, roll, limit) < 0) {
            r.morale(r.person, -8, 0x10663dd4);
            return;
        }
    }
    const auto roll = r.draw(sportsmanship(r), 0x10663e0c);
    const auto limit = bound_draw(r, second_bound, 0x10663f84, 0x106640ac);
    if (compare(c, roll, limit) <= 0) return;
    nudge_by_five(r, 0x10664188, 0x106641d0);
}

// Flag 0x200: another person than the player is lowered by 8 morale, and the visited person's contract morale
// moves by the player's temperament (kept within -100..100); then two sportsmanship/temperament draws of the player
// that both come out 0 make the visited person fall out with the manager and clear the contract's bit 8.
void flag_200(Reaction& r) {
    Cpu& c = r.c;
    if (!r.has(0x200)) return;
    if (compare_logical(c, r.player, r.person) == 0) return;
    r.morale(r.player, -8, 0x106641f4);
    const auto me = r.who(r.player);
    r.shift_contract_morale(r.person_contract_record(), std::uint32_t(me.temperament().get() & 0xff) * 3 - 0x64);
    if (compare(c, r.draw(me.sportsmanship().get(), 0x10664288), 0) != 0) return;
    if (compare(c, r.draw(me.temperament().get(), 0x10664400), 0) != 0) return;
    r.fall_out_with_manager(r.person, 0, 0x10664554);
    r.program.call(clear_flag, 0x10664564, {r.person_contract, 0x8, 0});
}

// 0x10664564: with flag 0x40 a person whose tally is under 50 and whose sportsmanship draw is 0 adds the sample mod 20.
void tail(Reaction& r) {
    Cpu& c = r.c;
    if (!r.has(0x40)) return;
    const auto id = c.u32(r.person);
    if (compare_logical(c, r.tally(r.person).get(), 0x32) >= 0) return;
    if (compare(c, r.draw(sportsmanship(r), 0x106645bc), 0) != 0) return;
    const auto rest = r.beaten_by(divisors::by_20, 0x10664720);
    const auto sign = c.r(3), sample = c.r(4);
    const auto count = r.tally(r.person);
    const auto before = count.get();
    count = std::uint8_t(before + (std::uint32_t(rest) & 0xff));
    leave(c, {{3, before}, {4, records::tally_row_of(c, id).at}, {5, sign}, {6, sample}});
}
}  // namespace

void Reaction::stages() {
    if (has(0x1)) {
        morale(person, 5, 0x10662d48);
    } else if (has(0x2)) {
        morale(person, -5, 0x10662d64);
    } else if (has(0x4)) {
        flag_4(*this);
    } else if (has(0x8)) {
        flag_8(*this);
    } else if (has(0x10)) {
        flag_10(*this);
    } else if (has(0x20)) {
        flag_20(*this);
    } else {
        flag_200(*this);
    }
    tail(*this);
}
}
}
