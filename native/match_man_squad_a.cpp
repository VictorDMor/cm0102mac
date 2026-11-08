// MATCH_MAN.CPP squad_a block (match_man_squad_a.hpp): 0x1020a290.
#include "match_man_squad_a.hpp"
#include "match_man_squad_a_detail.hpp"
#include "match_man_detail.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"

namespace cm::aot::match_man {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Fpu;
using guest::Frame;
using guest::Kept;
namespace side_at = squad_side;
namespace player_at = squad_player;
namespace data_at = squad_data;
namespace staff_at = squad_staff;
namespace call_to = squad_entries;

// The values as registers hold them after extsb, extsh.
constexpr std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }
constexpr std::int32_t s16(std::uint32_t value) { return std::int16_t(value); }
constexpr std::uint32_t reg(std::int32_t value) { return std::uint32_t(value); }
// mulhw: the high word of the signed product.
constexpr std::uint32_t mulhw(std::int32_t a, std::int32_t b) { return std::uint32_t((std::int64_t(a) * b) >> 32); }
// divw. A zero divisor leaves quotient 0 (the generated code proves 0x1020a778's quotient is
// only multiplied back by its divisor).
constexpr std::int32_t divw(std::int32_t a, std::int32_t b) { return b == 0 ? 0 : std::int32_t(std::int64_t(a) / b); }

// The two pairs of stack words the int-to-double idiom uses, and its exponent word.
constexpr std::uint32_t low_pair = 0x38, high_pair = 0x40;
constexpr std::uint32_t exponent_word = 0x43300000;

// Pool offsets (doubles at TOC -0x5278, by their use).
namespace pool_at {
constexpr std::uint32_t one = 0x0, power_exponent = 0x20, c28 = 0x28, c38 = 0x38, c88 = 0x88, conversion = 0x90,
                        ca0 = 0xa0, ca8 = 0xa8, cb0 = 0xb0, cb8 = 0xb8, cc0 = 0xc0, cc8 = 0xc8, cd0 = 0xd0, cd8 = 0xd8,
                        ce0 = 0xe0, ce8 = 0xe8, cf0 = 0xf0, cf8 = 0xf8, c100 = 0x100, c108 = 0x108, c110 = 0x110,
                        c118 = 0x118, c120 = 0x120, c128 = 0x128, c130 = 0x130;
}
// Offsets of the singles at TOC -0x527c.
namespace single_at {
constexpr std::uint32_t form_low = 0x0, form_high = 0x4, ability_scale = 0x8, ratio_fallback = 0xc, ratio_floor = 0x10,
                        form_fresh = 0x14;
}

class SquadA {
public:
    SquadA(Cpu& cpu, Frame& stack, std::uint32_t side)
        : c(cpu), fpu(cpu), frame(stack), m(side), f31(cpu), f28(cpu), scale(cpu), ratio(cpu),
          singles(c.toc(-0x527c)), consts(c.toc(-0x5278)), r21(cpu), r22(cpu), r23(cpu), r24(cpu), r25(cpu), r27(cpu, 0) {}

    void run();

private:
    Cpu& c;
    Fpu fpu;
    Frame& frame;
    const std::uint32_t m;
    // What the original holds in non-volatile registers across its calls. r21-r25 begin as TOC
    // pointers (the random cursor, its limit and table, the registry slot); the loops reuse r21
    // and r22; r23 and r25 end up holding values; r27 is the first loop's sum; f29 and f30 scale
    // the tests.
    KeptDouble<31> f31;
    KeptDouble<28> f28;
    KeptDouble<30> scale;
    KeptDouble<29> ratio;
    const std::uint32_t singles, consts;
    Kept<21> r21;
    Kept<22> r22;
    Kept<23> r23;
    Kept<24> r24;
    Kept<25> r25;
    Kept<27> r27;

    double pool(std::uint32_t at) const { return c.f64(consts + at); }
    double single(std::uint32_t at) const { return double(c.f32(singles + at)); }
    std::uint32_t field(std::uint32_t at) const { return c.u32(m + at); }
    std::uint32_t orders() const { return field(side_at::orders); }
    // The int-to-double idiom: both words reach the stack image, then fsub.
    double convert(std::uint32_t at, std::int32_t value) {
        c.put32(frame.at(at), exponent_word);
        c.put32(frame.at(at + 4), std::uint32_t(value) ^ 0x80000000u);
        return fpu.fsub(c.f64(frame.at(at)), pool(pool_at::conversion));
    }
    // rlwinm. with a mask, record form: cr0 compares the result with 0.
    bool test(std::uint32_t value, std::uint32_t mask) { return compare(c, std::int32_t(value & mask)) != 0; }
    std::uint32_t role(std::uint32_t player, std::uint32_t at) {
        c.set(3, player);
        c.call(call_to::role, at + 4);
        return c.r(3);
    }
    std::uint32_t role_more(std::uint32_t player, std::uint32_t at) {
        c.set(3, player);
        c.call(call_to::role_more, at + 4);
        return c.r(3);
    }
    // 0x100d1cb0: a person's attribute as a signed byte.
    std::int32_t attribute(std::uint32_t person, std::uint32_t index, std::uint32_t at) {
        c.set({{3, person}, {4, index}});
        c.call(call_to::attribute, at + 4);
        return s8(c.r(3));
    }
    std::int32_t person_attribute(std::uint32_t player, std::uint32_t index, std::uint32_t at) {
        return attribute(c.u32(player + player_at::person), index, at);
    }
    void mark(std::uint32_t bits, std::uint32_t at) {
        c.set({{3, orders()}, {4, bits}});
        c.call(call_to::orders_flags, at + 4);
    }
    void place(std::uint32_t player, std::uint32_t a, std::uint32_t b, std::uint32_t at) {
        c.set({{3, orders()}, {4, c.u8(player + player_at::slot)}, {5, a}, {6, b}});
        c.call(call_to::place, at + 4);
    }
    // 0x10113e30 with r5 = `first`, r6 = `second`.
    void apply(std::uint32_t player, std::uint32_t first, std::uint32_t second, std::uint32_t at) {
        c.set({{3, orders()}, {4, c.u8(player + player_at::slot)}, {5, first}, {6, second}});
        c.call(call_to::orders_apply, at + 4);
    }
    // 0x10113e10 returns 64 bits in r3:r4; the original tests the low word under `mask` (and a
    // zero high mask), leaving r3 = r4 = the masked word and r5 = 0.
    bool queried(std::uint32_t player, std::uint32_t mask, std::uint32_t at) {
        c.set({{3, orders()}, {4, c.u8(player + player_at::slot)}});
        c.call(call_to::orders_query, at + 4);
        const auto low = c.r(4) & mask;
        c.set({{3, low}, {4, low}, {5, 0}});
        return compare(c, std::int32_t(low)) != 0;
    }
    // After a query: the orders' word at +0x588 picks between two settings.
    void apply_by_query(std::uint32_t player, std::uint32_t mask, std::uint32_t flag, std::uint32_t when_flag,
                        std::uint32_t otherwise, std::uint32_t at_query, std::uint32_t at_flag, std::uint32_t at_no_flag,
                        std::uint32_t at_none) {
        if (queried(player, mask, at_query)) {
            if (test(c.u32(orders() + 0x588), flag)) apply(player, 0, when_flag, at_flag);
            else apply(player, 0, otherwise, at_no_flag);
        } else {
            apply(player, 0, otherwise, at_none);
        }
    }
    // conv(attribute) above the pool value times the scale ("ble skip" fails).
    bool strong(std::uint32_t player, std::uint32_t index, std::uint32_t pool_offset, std::uint32_t at) {
        const auto value = person_attribute(player, index, at);
        const double bar = fpu.fmul(pool(pool_offset), scale);
        const double seen = convert(low_pair, value);
        fpu.fcmp(0, seen, bar);
        return seen > bar;
    }
    // Bytes b14 and `first` of the data record: passes when b14 > first - 1 (>= when !strict),
    // then byte 0x19 must be 20. Leaves r3-r5 as the original.
    bool margin(std::uint32_t data, std::uint32_t first, bool strict) {
        c.set(5, data);
        const std::int32_t high = s8(c.u8(data + data_at::b14)), low = s8(c.u8(data + first));
        c.set({{4, reg(high)}, {3, reg(low)}});
        const auto order = compare(c, high, low - 1);
        if (strict ? order <= 0 : order < 0) return false;
        return compare(c, c.u8(data + data_at::b19), 0x14) == 0;
    }
    bool rule_tail(std::uint32_t player, std::uint32_t first, bool strict, std::uint32_t pool_offset, std::uint32_t at) {
        return margin(c.u32(player + player_at::data), first, strict) && strong(player, 7, pool_offset, at);
    }
    // The match's table row of the player's opponent key (0x10114b50's first argument).
    std::uint32_t row_of(std::uint32_t player, std::uint32_t match) {
        return match + reg(s8(c.u8(player + player_at::opponent_key)) * 0x18e0 + 0x7c6c);
    }
    // The ratings of both clubs against the opposing player's ability: passes when the own
    // club is not far above (the original's two-step test).
    bool near_ratings(std::uint32_t rival_player) {
        const auto rival = field(side_at::rival_club), own = field(side_at::club);
        const std::int32_t rival_rating = c.i16(rival + squad_club::rating), own_rating = c.i16(own + squad_club::rating);
        c.set(4, reg(own_rating));
        if (compare(c, own_rating, s16(reg(rival_rating + 0xfa))) >= 0) {
            const auto ability = c.i16(c.u32(rival_player + player_at::data) + data_at::ability_a);
            if (compare(c, s16(reg(ability + 0xfa)), own_rating) <= 0) return false;
        }
        return true;
    }
    // The judge's rating (0x100d2a80) against the opposing person's id remainder plus `base`:
    // passes when it is above (mulhw 0x66666667: the remainder is the id mod 5, an extsb of the sum).
    bool judge_above(std::uint32_t rival_player, std::int32_t base, std::uint32_t at) {
        c.set({{3, field(side_at::judge)}, {4, 0xb}});
        c.call(call_to::judge_attribute, at + 4);
        const std::int32_t judged = s8(c.r(3));
        const auto person = c.u32(rival_player + player_at::person);
        const std::int32_t id = c.i32(person);
        c.set({{6, reg(judged)}, {5, person}, {4, reg(id)}});
        return compare(c, judged, s8(reg(id % 5 + base))) > 0;
    }

    bool opening();
    void orders_from_ratings();
    void first_loop();
    void player(std::uint32_t p);
    void position_rules(std::uint32_t p);
    void tendencies(std::uint32_t p);
    void scenarios(std::uint32_t p);
    bool scenario_one(std::uint32_t p);
    bool scenario_two(std::uint32_t p);
    void scenario_three(std::uint32_t p);
    void form_rules(std::uint32_t p);
    void fatigue_rules(std::uint32_t p);
    void sum_rules(std::uint32_t p);
    void kind_rules(std::uint32_t p);
    void closing(std::uint32_t p);
};

// The routine. The stack frame, the saved registers and their restoring are the original's
// (Frame); each method below is a stretch of its listing.

// 0x1020a290..0x1020a4c8. Returns false where the original jumps to its epilogue.
bool SquadA::opening() {
    c.set(26, m);
    const auto club = field(side_at::club), rival = field(side_at::rival_club);
    const std::int32_t own_rating = c.i16(club + squad_club::rating), rival_rating = c.i16(rival + squad_club::rating);
    c.set({{4, exponent_word}, {5, reg(own_rating) ^ 0x80000000u}, {6, club}, {3, reg(rival_rating) ^ 0x80000000u}});
    const double own_scaled = fpu.fdiv(convert(high_pair, own_rating), pool(pool_at::ca0));
    const double rival_scaled = fpu.fdiv(convert(low_pair, rival_rating), pool(pool_at::ca0));
    const auto raw_kind = c.u8(m + side_at::kind);
    compare(c, s8(raw_kind));
    ratio = fpu.fadd(own_scaled, rival_scaled);
    c.set_f(1, own_scaled);  // f1 still holds the first quotient
    // The TOC words the original loads now: r21-r25 (the random seed word, the cursor's reset
    // value, its limit and the cursor word; a registry word), r28, r31 (the registry), and the
    // two pools.
    c.set(28, c.toc(-0x7b3c));
    c.set(29, singles);
    c.set(30, consts);
    c.set(31, c.toc(-0x7a6c));
    r21 = c.toc(-0x7b90);
    r22 = c.toc(-0x7b8c);
    r23 = c.toc(-0x7b88);
    r24 = c.toc(-0x7b84);
    r25 = c.toc(-0x7a54);
    r27 = 0;

    if (s8(raw_kind) == 0) {
        // A club person among the newest few of the table: the registry may end the setup.
        const auto person = c.u32(club + squad_club::person);
        c.set(4, person);
        if (compare_logical(c, person, 0) != 0) {
            const std::uint32_t id = c.u32(person), count = c.u32(c.toc(-0x7be4));
            c.set({{5, id}, {3, count}});
            const bool recent = compare(c, std::int32_t(id), std::int32_t(count - 0x10)) >= 0 &&
                                compare(c, std::int32_t(id), std::int32_t(count)) < 0 &&
                                compare(c, s8(c.u8(person + 0x6b))) != 0;
            if (compare(c, recent ? 1 : 0) != 0) {
                c.set(3, c.u32(r25));
                c.call(call_to::registry_check, 0x1020a390);
                if (compare(c, s8(c.r(3))) != 0) {
                    c.set({{5, c.u32(field(side_at::club) + squad_club::person)}, {4, 2}, {3, c.u32(r25)}});
                    c.call(call_to::registry_act, 0x1020a3ac);
                    if (compare(c, s8(c.r(3))) != 0) return false;
                }
            }
        }
    }

    c.set(4, raw_kind);
    const bool plain = compare(c, s8(raw_kind)) == 0;
    bool reaches_flags = !plain;
    if (plain) {
        const auto extras = field(side_at::extras);
        c.set(3, extras);
        const std::int32_t type = s8(c.u8(extras + 0x3a));
        reaches_flags = compare(c, type, 1) == 0 || compare(c, type, 9) == 0;
    }
    bool flag_orders = false;
    if (reaches_flags) {
        if (compare(c, s8(raw_kind)) != 0) {
            flag_orders = true;
        } else {
            // The match's luck: samples of the game's random table and the clubs' rating gap.
            const std::uint32_t seed_slot = r21, reset_value = r22, limit_slot = r23, cursor_slot = r24;
            const auto advance = [&](std::uint32_t at) {
                c.put32(cursor_slot, reset_value);
                c.call(call_to::random, at + 4);
                c.put32(seed_slot, c.r(3) & 0xffff);
            };
            std::uint32_t next = c.u32(cursor_slot) + 2;
            const std::uint32_t limit_a = c.u32(limit_slot);
            c.put32(cursor_slot, next);
            compare_logical(c, next, limit_a);
            if (next > limit_a) advance(0x1020a400);
            const std::uint32_t cursor = c.u32(cursor_slot), seed = c.u32(seed_slot);
            const std::uint32_t step = c.u16(cursor);
            next = cursor + 2;
            const std::uint32_t limit_b = c.u32(limit_slot);
            const std::int32_t sum = reg(step + seed);
            c.put32(cursor_slot, next);
            // mulhw by -0x10624dd3 and srawi 8 divide by -4000 (checked for all 32-bit values);
            // the original multiplies back and subtracts: the remainder is sum mod 4000.
            const std::uint32_t high = mulhw(std::int32_t(0xef9db22d), sum);
            compare_logical(c, next, limit_b);
            r23 = 0u - reg(sum % 4000);
            c.set({{7, cursor}, {6, step}, {5, reg(sum)}, {4, high}});
            if (next > limit_b) advance(0x1020a458);
            const std::uint32_t cursor_c = c.u32(cursor_slot);
            const std::int32_t sum_c = reg(c.u16(cursor_c) + c.u32(seed_slot));
            const std::int32_t rival_c = c.i16(field(side_at::rival_club) + squad_club::rating);
            const std::int32_t own_c = c.i16(field(side_at::club) + squad_club::rating);
            const std::int32_t gap = own_c - rival_c;
            c.set({{4, reg(sum_c)}, {6, reg(rival_c)}, {5, reg(gap)}});
            // mulhw by 0x10624dd3 and srawi 7: the division by 2000, so the remainder is sum mod 2000.
            flag_orders = compare(c, gap, std::int32_t(std::uint32_t(r23)) + sum_c % 2000) > 0;
        }
    }
    if (flag_orders) mark(0x20, 0x1020a4bc);
    const auto staff = field(side_at::staff);
    c.set(4, staff);
    return compare_logical(c, staff, 0) != 0;
}

// 0x1020a4cc..0x1020a648: orders flags from the manager's bytes, the club's rating and notices.
void SquadA::orders_from_ratings() {
    const auto staff = field(side_at::staff);
    const auto b33 = c.u8(staff + staff_at::b33);
    c.set(4, b33);
    const double conv = convert(low_pair, s8(c.u8(staff + staff_at::b2e)));
    const std::int32_t trait = s8(b33);
    compare(c, trait, 8);
    const double shifted = fpu.fsub(conv, pool(pool_at::c88));
    const double divided = fpu.fdiv(shifted, pool(pool_at::ca8));
    ratio = fpu.fmul(ratio, fpu.fsub(pool(pool_at::one), divided));
    if (trait <= 8) {
        mark(2, 0x1020a524);
    } else if (compare(c, trait, 0xe) <= 0) {
        mark(1, 0x1020a53c);
    } else if (compare(c, trait, 0x12) <= 0) {
        mark(4, 0x1020a554);
    } else if (compare(c, trait, 0x14) <= 0) {
        mark(8, 0x1020a56c);
    }
    const std::int32_t cap = s8(c.u8(field(side_at::staff) + staff_at::b3f));
    if (compare(c, cap, 0xf) >= 0) mark(0x100, 0x1020a58c);
    else if (compare(c, cap, 5) <= 0) mark(0x80, 0x1020a5a4);

    const auto club = field(side_at::club);
    const auto staff_c = field(side_at::staff);
    const std::int32_t own_rating = c.i16(club + squad_club::rating);
    c.set({{4, exponent_word}, {5, staff_c}});
    const double rating_value = convert(high_pair, own_rating);
    const double bar = fpu.multiply_add(0x1020a5f0, pool(pool_at::cb8), rating_value, pool(pool_at::cb0));
    const double seen = convert(low_pair, s8(c.u8(staff_c + staff_at::b3c)));
    fpu.fcmp(0, seen, bar);
    if (seen > bar) {
        const auto flags = c.u32(orders() + 0x1880);
        if (!test(flags, 8)) mark(0x400, 0x1020a614);
    }
    const auto notices = c.u32(c.toc(-0x7a6c));
    const auto raw_kind = c.u8(m + side_at::kind);
    c.set({{3, notices}, {5, 0}, {4, field(side_at::club)}, {6, 0}, {7, raw_kind}});
    c.call(call_to::notice, 0x1020a630);
    c.set({{3, c.u32(c.toc(-0x7a6c))}, {5, 1}, {4, field(side_at::club)}, {6, 0}, {7, c.u8(m + side_at::kind)}});
    c.call(call_to::notice, 0x1020a648);
}

// 0x1020a648..0x1020a714: the first loop sums, for each player on the pitch in some roles with
// a high enough attribute, a power of the attribute above 14 plus the shortfall of a byte below 8.
void SquadA::first_loop() {
    const std::int32_t side_index = s8(c.u8(m + side_at::own_side));
    r21 = 0;
    std::uint32_t p = c.u32(m + side_at::match) + reg(side_index * int(player_at::table_stride)) + player_at::first;
    r22 = p;
    do {
        p = r22;
        if (compare(c, s8(c.u8(p + player_at::slot))) >= 0 && test(role(p, 0x1020a678), 0x40) &&
            test(role(p, 0x1020a688), 0x200)) {
            const std::int32_t ability = s8(c.u8(c.u32(p + player_at::data) + data_at::b2e));
            if (compare(c, ability, 0xe) > 0) {
                const double base = convert(low_pair, ability - 0xe);
                c.set_f(1, base);
                c.set_f(2, pool(pool_at::power_exponent));
                c.call(call_to::power, 0x1020a6d0);
                // The import glue switches r2 to the library's table; `lwz r2,0x14(r1)` brings it back.
                c.set(2, c.u32(frame.at(0x14)));
                const auto low_word = truncated_to_stack(c, fpu, frame.at(high_pair), c.f(1));
                const auto raw = c.u8(c.u32(p + player_at::data) + data_at::b40);
                c.set(4, raw);
                const bool short_of = compare(c, s8(raw), 8) < 0;
                r27 = r27 + low_word;
                if (short_of) r27 = r27 + reg(s8(reg(8 - std::int32_t(raw))));
            }
        }
        r21 = r21 + 1;
        r22 = r22 + player_at::size;
    } while (compare(c, std::int32_t(std::uint32_t(r21)), 0x14) < 0);
}

// 0x1020a73c..0x1020ac10: the manager's trait sets the scale f30; unless the orders are locked,
// the first of six role rules that holds places the player.
void SquadA::position_rules(std::uint32_t p) {
    const auto staff = field(side_at::staff);
    c.set({{3, staff}, {4, 0xb}});
    c.call(call_to::staff_attribute, 0x1020a748);
    const std::int32_t trait = s8(c.r(3));
    const auto data = c.u32(p + player_at::data);
    const std::int32_t half = (trait + (trait < 0 ? 1 : 0)) >> 1;
    const std::int32_t divisor = 13 - half;
    const std::int32_t sum = reg(c.u32(field(side_at::staff) + staff_at::id) + c.u32(data));
    const std::int32_t quotient = divw(sum, divisor);
    const std::int32_t remainder = sum - quotient * divisor;  // wraps, as mullw and subf do
    c.set({{4, reg(sum)}, {5, reg(divisor)}});
    const double stretch = fpu.fmul(pool(pool_at::c38), ratio);
    const double fraction = fpu.fdiv(convert(low_pair, remainder), pool(pool_at::cc0));
    const double blend = fpu.fadd(pool(pool_at::one), fraction);
    scale = fpu.fmul(stretch, blend);
    fpu.fcmp(0, scale, pool(pool_at::cc8));
    if (scale < pool(pool_at::cc8)) scale = pool(pool_at::cc8);

    const auto flags = c.u32(orders() + 0x1880);
    if (test(flags, 0x400000) || test(flags, 0x800000) || test(flags, 0x1000000)) return;
    // Chain of six rules (0x10289560 flags, 0x102895e0 more flags): the first that holds places the
    // player and ends the chain.
    if (test(role(p, 0x1020a7e8), 0x10) && test(role(p, 0x1020a7f8), 0x200) &&
        rule_tail(p, data_at::b13, true, pool_at::cd0, 0x1020a838)) {
        place(p, 0x220, 0, 0x1020a87c);
        place(p, 0x220, 1, 0x1020a890);
        return;
    }
    if (test(role(p, 0x1020a89c), 0x10) && !test(role_more(p, 0x1020a8ac), 0x20) && test(role(p, 0x1020a8bc), 0x100) &&
        rule_tail(p, data_at::b13, false, pool_at::cd0, 0x1020a8fc)) {
        place(p, 0x220, 1, 0x1020a940);
        return;
    }
    if (test(role(p, 0x1020a94c), 0x10) && !test(role_more(p, 0x1020a95c), 0x20) && test(role(p, 0x1020a96c), 0x400) &&
        rule_tail(p, data_at::b13, false, pool_at::cd0, 0x1020a9ac)) {
        place(p, 0x220, 1, 0x1020a9f0);
        return;
    }
    if (test(role(p, 0x1020a9fc), 0x40) && test(role(p, 0x1020aa0c), 0x200) &&
        rule_tail(p, data_at::b15, false, pool_at::cd8, 0x1020aa4c)) {
        place(p, 0x220, 0, 0x1020aa90);
        place(p, 0x240, 1, 0x1020aaa4);
        return;
    }
    if (test(role(p, 0x1020aab0), 0x40) && test(role(p, 0x1020aac0), 0x100) &&
        rule_tail(p, data_at::b15, false, pool_at::cd8, 0x1020ab00)) {
        place(p, 0x120, 0, 0x1020ab44);
        place(p, 0x140, 1, 0x1020ab58);
        return;
    }
    if (test(role(p, 0x1020ab64), 0x40) && test(role(p, 0x1020ab74), 0x400) &&
        rule_tail(p, data_at::b15, false, pool_at::cd8, 0x1020abb4)) {
        place(p, 0x420, 0, 0x1020abf8);
        place(p, 0x440, 1, 0x1020ac0c);
    }
}

// 0x1020ac10..0x1020aec0: attackers' and others' tendencies set through 0x10113e30.
void SquadA::tendencies(std::uint32_t p) {
    const auto person = [&] { return c.u32(p + player_at::person); };
    bool strong_runner = test(role(p, 0x1020ac14), 0x2) || test(role(p, 0x1020ac24), 0x20) || test(role(p, 0x1020ac34), 0x40);
    bool handled = false;
    if (strong_runner) {
        // The player's form (a float at +0x50) must be above the first single, and its data bytes.
        const float form = c.f32(p + player_at::form);
        fpu.fcmp(0, double(form), single(single_at::form_low));
        if (double(form) > single(single_at::form_low) && compare(c, s8(c.u8(c.u32(p + player_at::data) + data_at::b3c)), 0xc) > 0 &&
            strong(p, 7, pool_at::cd8, 0x1020ac6c) &&
            compare(c, s8(c.u8(c.u32(p + player_at::data) + data_at::b1a)), 0xf) > 0) {
            const std::int32_t speed = person_attribute(p, 0x14, 0x1020acbc);
            const auto staff = field(side_at::staff);
            const std::int32_t lead = s8(c.u8(staff + staff_at::b35));
            c.set({{5, reg(speed)}, {4, staff}});
            const double bar = fpu.fmul(pool(pool_at::ce0), scale);
            const double seen = convert(low_pair, (lead - 10) * 3 + speed);
            fpu.fcmp(0, seen, bar);
            if (seen > bar) {
                handled = true;
                apply(p, 0, 0x200000, 0x1020ad1c);
                const float fresh = c.f32(p + player_at::form);
                fpu.fcmp(0, double(fresh), single(single_at::form_high));
                const auto data = c.u32(p + player_at::data);
                bool call_it = double(fresh) < single(single_at::form_high);
                if (!call_it) call_it = compare(c, s8(c.u8(data + data_at::b3c)), 0x12) < 0;
                if (!call_it) call_it = compare(c, s8(c.u8(data + data_at::b44)), 0x10) > 0;
                if (call_it) apply(p, 0, 0x4000, 0x1020ad64);
            }
        }
        (void)person;
    }
    if (!handled) {
        const std::int32_t pace = person_attribute(p, 7, 0x1020ad74);
        r23 = reg(pace);
        const std::int32_t drive = person_attribute(p, 0xd, 0x1020ad84);
        r25 = reg(drive);
        const std::int32_t speed = person_attribute(p, 0x14, 0x1020ad94);
        const auto staff = field(side_at::staff);
        const auto data = c.u32(p + player_at::data);
        const std::int32_t lead = (s8(c.u8(staff + staff_at::b35)) - 10) * 3;
        const std::int32_t trained = (s8(c.u8(data + data_at::b1a)) - 10) * 3;
        const std::int32_t first = pace + speed;
        c.set({{4, reg(lead)}, {5, reg(first)}});
        const double bar = fpu.fmul(pool(pool_at::ce0), scale);
        const double seen = convert(low_pair, drive + (first + lead + trained));
        fpu.fcmp(0, seen, bar);
        if (seen < bar) apply(p, 0, 0x100000, 0x1020ae14);
    }

    if (test(role(p, 0x1020ae1c), 0x20) && !test(role(p, 0x1020ae2c), 0x80) && !test(role(p, 0x1020ae3c), 0x800))
        apply_by_query(p, 1, 8, 2, 4, 0x1020ae50, 0x1020ae90, 0x1020aea4, 0x1020aebc);
}

// 0x1020aec0..0x1020b614: the player's duel with an opposing player, in three scenarios.
void SquadA::scenarios(std::uint32_t p) {
    if (!test(role(p, 0x1020aec4), 0x8) && !test(role(p, 0x1020aed4), 0x10)) return;
    if (test(role(p, 0x1020aee4), 0x80) || test(role(p, 0x1020aef4), 0x800)) return;
    const std::int32_t seen_value = person_attribute(p, 7, 0x1020af08);
    const double bar = fpu.fmul(pool(pool_at::cd0), scale);
    const double seen = convert(low_pair, seen_value);
    fpu.fcmp(0, seen, bar);
    if (!(seen < bar)) return;
    apply_by_query(p, 1, 8, 2, 4, 0x1020af44, 0x1020af84, 0x1020af98, 0x1020afb0);

    // The opposing player, from the other side's table (0 when 0x101fd100 finds none).
    c.set({{3, field(side_at::match)}, {4, c.u8(p + player_at::opponent_key)}, {5, c.u8(p + player_at::slot)}});
    c.call(call_to::opponent_index, 0x1020afc4);
    const std::int32_t index = s8(c.r(3));
    r23 = 0;
    compare(c, index, -1);
    if (index != -1) {
        const std::uint32_t other_side = s8(c.u8(p + player_at::slot)) == 0 ? 1 : 0;  // cntlzw, shift 5
        const auto match = field(side_at::match);
        c.set(4, match);
        r23 = match + (other_side * player_at::table_stride + reg(index * int(player_at::size)) + player_at::first);
    }
    if (scenario_one(p)) return;
    if (scenario_two(p)) return;
    scenario_three(p);
}

// 0x1020b000..0x1020b200. True when it reached 0x1020b614 itself; false: try the next scenario.
bool SquadA::scenario_one(std::uint32_t p) {
    const std::uint32_t other = r23;
    if (compare_logical(c, other, 0) == 0) return false;
    if (compare_logical(c, c.u32(other + player_at::person), 0) == 0) return false;
    if (!test(role(other, 0x1020b014), 0x20) && !test(role_more(other, 0x1020b024), 0x20)) return false;
    if (!test(role(other, 0x1020b034), 0x200) && !test(role(other, 0x1020b044), 0x100) &&
        !test(role(other, 0x1020b054), 0x400))
        return false;
    if (!near_ratings(other)) return false;
    if (compare(c, person_attribute(p, 0x1c, 0x1020b0a0), 0xc) <= 0) return false;
    if (compare(c, person_attribute(p, 0x11, 0x1020b0b8), 0xa) <= 0) return false;
    if (compare(c, person_attribute(p, 0x1f, 0x1020b0d0), 0xa) <= 0) return false;
    if (compare(c, c.u8(c.u32(p + player_at::data) + data_at::b12), 0x14) != 0) return false;
    if (!test(role(p, 0x1020b0f4), 0x10)) return false;
    if (!test(role(p, 0x1020b104), 0x200) && !test(role(p, 0x1020b114), 0x100) && !test(role(p, 0x1020b124), 0x400))
        return false;
    if (!judge_above(other, 10, 0x1020b138)) return false;
    const auto club_word = c.u32(c.u32(c.u32(m + side_at::match) + 0x3118) + 0x1c);
    if (compare_logical(c, club_word, field(side_at::club)) != 0) {
        place(p, 0x208, 1, 0x1020b1d0);
    } else if (compare(c, person_attribute(p, 0x1a, 0x1020b198), 0xf) > 0) {
        place(p, 0x208, 1, 0x1020b1b8);
    }
    const auto match = field(side_at::match);
    c.set(4, match);
    const auto row = row_of(p, match);
    c.set(3, row);
    if (compare(c, std::int32_t(row)) == 0) return true;
    c.set({{3, row}, {4, c.u8(p + player_at::slot)}, {5, c.u32(other + player_at::person)}});
    c.call(call_to::mark_pair, 0x1020b1fc);
    return true;
}

// 0x1020b200..0x1020b3a4.
bool SquadA::scenario_two(std::uint32_t p) {
    const std::uint32_t other = r23;
    if (compare_logical(c, other, 0) == 0) return false;
    if (compare_logical(c, c.u32(other + player_at::person), 0) == 0) return false;
    if (!test(role(p, 0x1020b218), 0x8)) return false;
    if (!test(role(p, 0x1020b228), 0x200) && !test(role(p, 0x1020b238), 0x100) && !test(role(p, 0x1020b248), 0x400))
        return false;
    if (!test(role(other, 0x1020b258), 0x20) && !test(role_more(other, 0x1020b268), 0x20)) return false;
    if (!test(role(other, 0x1020b278), 0x200)) return false;
    if (!near_ratings(other)) return false;
    const auto match = field(side_at::match);
    c.set(4, match);
    if (compare(c, std::int32_t(row_of(p, match))) == 0) return false;
    if (compare(c, person_attribute(p, 0x1f, 0x1020b2e0), 0xa) <= 0) return false;
    if (compare(c, person_attribute(p, 0x1c, 0x1020b2f8), 0xf) <= 0) return false;
    if (compare(c, person_attribute(p, 0x13, 0x1020b310), 0xa) <= 0) return false;
    if (compare(c, person_attribute(p, 0x1a, 0x1020b328), 0xa) <= 0) return false;
    if (!judge_above(other, 7, 0x1020b340)) return false;
    const auto match_again = field(side_at::match);
    const auto row = row_of(p, match_again);
    c.set({{6, match_again}, {4, c.u8(p + player_at::slot)}, {3, row}});
    c.call(call_to::mark_pair, 0x1020b3a0);
    return true;
}

// 0x1020b3a4..0x1020b614.
void SquadA::scenario_three(std::uint32_t p) {
    const std::uint32_t other = r23;
    if (compare_logical(c, other, 0) == 0) return;
    if (compare_logical(c, c.u32(other + player_at::person), 0) == 0) return;
    if (!test(role(other, 0x1020b3bc), 0x10) && !test(role_more(other, 0x1020b3cc), 0x10)) return;
    if (!test(role(other, 0x1020b3dc), 0x200) && !test(role(other, 0x1020b3ec), 0x100) &&
        !test(role(other, 0x1020b3fc), 0x400))
        return;
    if (!near_ratings(other)) return;
    const auto match = field(side_at::match);
    c.set(4, match);
    if (compare(c, std::int32_t(row_of(p, match))) == 0) return;
    if (compare(c, person_attribute(p, 0x1f, 0x1020b464), 0xa) <= 0) return;
    if (compare(c, person_attribute(p, 0x1c, 0x1020b47c), 0xc) <= 0) return;
    if (compare(c, person_attribute(p, 0x13, 0x1020b494), 0xa) <= 0) return;
    if (compare(c, person_attribute(p, 0x1a, 0x1020b4ac), 0xa) <= 0) return;
    if (!judge_above(other, 8, 0x1020b4c4)) return;
    // Three ratings of the opposing player (0x10303560, rating kinds 1, 2, 5; r5 = 6).
    const auto table = c.u32(c.toc(-0x7b3c));
    const auto rate = [&](std::uint32_t kind, std::uint32_t at) {
        c.set({{3, table}, {5, 6}, {4, c.u32(other + player_at::data)}, {6, kind}});
        c.set_f(1, single(single_at::ability_scale));
        c.call(call_to::rating_of, at + 4);
    };
    rate(1, 0x1020b518);
    f28 = c.f(1);
    rate(2, 0x1020b534);
    f31 = c.f(1);
    rate(5, 0x1020b550);
    double first = single(single_at::ratio_fallback), second = first;
    fpu.fcmp(0, f28, single(single_at::ratio_floor));
    if (double(f28) > single(single_at::ratio_floor)) {
        first = fpu.fdivs(f31, f28);
        second = fpu.fdivs(c.f(1), f28);
    }
    fpu.fcmp(0, first, pool(pool_at::ce8));
    bool go = first > pool(pool_at::ce8);
    if (!go) {
        fpu.fcmp(0, second, pool(pool_at::cf0));
        go = second > pool(pool_at::cf0);
    }
    if (!go) return;
    if (test(role(other, 0x1020b58c), 0x20) || test(role_more(other, 0x1020b59c), 0x20)) {
        const auto data = c.u32(p + player_at::data);
        const bool keeper = compare(c, c.u8(data + data_at::b12), 0x14) == 0 ||
                            compare(c, c.u8(c.u32(p + player_at::data) + data_at::b13), 0x14) == 0;
        if (keeper && compare(c, person_attribute(p, 0x1a, 0x1020b5cc), 0xe) > 0) place(p, 0x208, 1, 0x1020b5ec);
    }
    const auto match_last = field(side_at::match);
    const auto row = row_of(p, match_last);
    c.set({{6, match_last}, {4, c.u8(p + player_at::slot)}, {5, c.u32(other + player_at::person)}, {3, row}});
    c.call(call_to::mark_pair, 0x1020b614);
}

// 0x1020b614..0x1020b970: the player's finishing attribute against the scale; then, for some roles,
// a second attribute against the manager's cap.
void SquadA::form_rules(std::uint32_t p) {
    const std::int32_t finishing = person_attribute(p, 0x12, 0x1020b61c);
    f31 = fpu.fmul(pool(pool_at::c88), scale);
    const double seen = convert(low_pair, finishing);
    fpu.fcmp(0, seen, f31);
    if (seen < double(f31)) {
        apply(p, 0, 0x10000000, 0x1020b664);
    } else {
        const std::int32_t again = person_attribute(p, 0x12, 0x1020b674);
        const double bar = fpu.fmul(pool(pool_at::cf8), scale);
        const double seen_again = convert(low_pair, again);
        fpu.fcmp(0, seen_again, bar);
        if (seen_again > bar) apply(p, 0, 0x20000000, 0x1020b6bc);
    }

    // The manager's byte at +0x3f over 5 (mulhw 0x66666667, srawi 1: a division), as a double.
    const auto cap_fifth = [&](std::int32_t attribute_value, double& seen_value) {
        const auto staff = field(side_at::staff);
        const std::int32_t cap = s8(c.u8(staff + staff_at::b3f));
        c.set({{5, reg(attribute_value) ^ 0x80000000u}, {6, exponent_word}, {4, 0x66660000}});
        seen_value = convert(low_pair, attribute_value);
        return convert(high_pair, cap / 5);
    };
    const bool eligible = (test(role(p, 0x1020b6c4), 0x8) || test(role(p, 0x1020b6d4), 0x10) ||
                           test(role(p, 0x1020b6e4), 0x20)) &&
                          !test(role(p, 0x1020b6f4), 0x80) && !test(role(p, 0x1020b704), 0x800);
    if (!eligible) return;
    // Setting 0x400 or 0x1000 after a query of bit 0x200, as 0x1020b7f4 and 0x1020b8f8 both do.
    double seen_value = 0;
    const std::int32_t first = person_attribute(p, 2, 0x1020b718);
    const double cap_value = cap_fifth(first, seen_value);
    const double limit = fpu.fadd(f31, cap_value);
    fpu.fcmp(0, seen_value, limit);
    if (!(seen_value < limit)) return;
    const std::int32_t second = person_attribute(p, 0x1f, 0x1020b788);
    const double cap_second = cap_fifth(second, seen_value);
    const double bar = fpu.multiply_add(0x1020b7e8, pool(pool_at::c100), scale, cap_second);
    fpu.fcmp(0, seen_value, bar);
    if (seen_value < bar) {
        apply_by_query(p, 0x200, 0x20, 0x400, 0x1000, 0x1020b7fc, 0x1020b83c, 0x1020b850, 0x1020b868);
        apply(p, 0, 0x8000, 0x1020b880);
        return;
    }
    const std::int32_t third = person_attribute(p, 0x1f, 0x1020b890);
    const double cap_third = cap_fifth(third, seen_value);
    const double limit_third = fpu.fadd(f31, cap_third);
    fpu.fcmp(0, seen_value, limit_third);
    if (seen_value < limit_third)
        apply_by_query(p, 0x200, 0x20, 0x400, 0x1000, 0x1020b900, 0x1020b940, 0x1020b954, 0x1020b96c);
}

// 0x1020b970..0x1020bdac: fatigue-like settings from the manager's bytes, then the stamina rules.
void SquadA::fatigue_rules(std::uint32_t p) {
    if (compare(c, s8(c.u8(p + player_at::flag24))) != 0) {
        const std::int32_t trait = person_attribute(p, 2, 0x1020b984);
        const auto staff = field(side_at::staff);
        c.set({{4, staff}, {5, reg(trait)}});
        const std::int32_t cap = s8(c.u8(staff + staff_at::b3f));
        if (compare(c, trait, cap / 4 + 8) > 0) apply(p, 0, 0x800, 0x1020b9bc);
    }
    // The manager's byte at +0x2e over 4 (srawi 2, addze), as a double; leaves r4, r5 as the original.
    const auto quarter = [&] {
        const auto staff = field(side_at::staff);
        c.set({{4, staff}, {5, exponent_word}});
        return convert(high_pair, s8(c.u8(staff + staff_at::b2e)) / 4);
    };
    const auto form = [&] { return double(c.f32(p + player_at::form)); };
    bool broad = test(role(p, 0x1020b9c4), 0x8) || test(role(p, 0x1020b9d4), 0x10);
    if (!broad) {
        const std::int32_t reach = s8(c.u8(field(side_at::staff) + staff_at::b2e));
        broad = compare(c, reach, 0x10) > 0 && test(role(p, 0x1020b9f8), 0x40);
    }
    if (broad) {
        const std::int32_t run = person_attribute(p, 0x14, 0x1020ba0c);
        f28 = fpu.fmul(pool(pool_at::c108), scale);
        double seen = convert(low_pair, run);
        double bar = fpu.fsub(f28, quarter());
        fpu.fcmp(0, seen, bar);
        bool applied = false;
        if (seen > bar) {
            const std::int32_t climb = person_attribute(p, 0xc, 0x1020ba74);
            seen = convert(low_pair, climb);
            bar = fpu.fsub(f28, quarter());
            fpu.fcmp(0, seen, bar);
            if (seen > bar) {
                fpu.fcmp(0, form(), single(single_at::form_fresh));
                if (form() > single(single_at::form_fresh)) {
                    apply(p, 0, 0x800000, 0x1020baf0);
                    applied = true;
                }
            }
        }
        if (!applied) {
            const std::int32_t again = person_attribute(p, 0x14, 0x1020bb00);
            seen = convert(low_pair, again);
            bar = fpu.fsub(f31, quarter());
            fpu.fcmp(0, seen, bar);
            bool tired = false;
            if (seen < bar) {
                const std::int32_t climb = person_attribute(p, 0xc, 0x1020bb60);
                seen = convert(low_pair, climb);
                bar = fpu.fsub(f31, quarter());
                fpu.fcmp(0, seen, bar);
                tired = seen > bar;
            }
            if (!tired) {
                fpu.fcmp(0, form(), single(single_at::form_fresh));
                tired = form() < single(single_at::form_fresh);
            }
            if (tired) apply(p, 0, 0x400000, 0x1020bbdc);
        }
    }

    // 0x1020bbe0: the sprint rules.
    const bool by_role = test(role(p, 0x1020bbe4), 0x10) || test(role(p, 0x1020bbf4), 0x20) || test(role(p, 0x1020bc04), 0x40);
    bool middle = true;
    if (!by_role) {
        const std::int32_t value = person_attribute(p, 0xb, 0x1020bc18);
        const double limit = fpu.fmul(pool(pool_at::cd0), scale);
        const double seen = convert(low_pair, value);
        fpu.fcmp(0, seen, limit);
        middle = seen > limit;
    }
    bool first_rule = false;
    if (middle) {
        const std::int32_t value = person_attribute(p, 0xb, 0x1020bc54);
        const double limit = fpu.fmul(pool(pool_at::c108), scale);
        const double seen = convert(low_pair, value);
        fpu.fcmp(0, seen, limit);
        first_rule = seen > limit;
        if (!first_rule) {
            r23 = reg(person_attribute(p, 0x15, 0x1020bc90));
            const std::int32_t pace = person_attribute(p, 0xb, 0x1020bca0);
            double total = 0;
            const double quarter_value = [&] {
                const auto staff = field(side_at::staff);
                c.set({{4, staff}, {5, exponent_word}});
                total = convert(low_pair, pace + s8(r23));
                return convert(high_pair, s8(c.u8(staff + staff_at::b2e)) / 4);
            }();
            const double bar = fpu.multiply_add(0x1020bcf4, pool(pool_at::c110), scale, quarter_value);
            fpu.fcmp(0, total, bar);
            first_rule = total > bar;
        }
    }
    if (first_rule) {
        apply(p, 0, 0x2000000, 0x1020bd14);
        return;
    }
    r23 = reg(person_attribute(p, 0x15, 0x1020bd24));
    const std::int32_t pace = person_attribute(p, 0xb, 0x1020bd34);
    const auto staff = field(side_at::staff);
    c.set({{4, staff}, {5, exponent_word}});
    const double total = convert(low_pair, pace + s8(r23));
    const double quarter_value = convert(high_pair, s8(c.u8(staff + staff_at::b2e)) / 4);
    const double bar = fpu.multiply_add(0x1020bd88, pool(pool_at::c28), scale, quarter_value);
    fpu.fcmp(0, total, bar);
    if (total < bar) apply(p, 0, 0x1000000, 0x1020bda8);
}

// 0x1020bdac..0x1020c218: bonuses and penalties from the data bytes, then the first loop's sum.
void SquadA::sum_rules(std::uint32_t p) {
    const auto data_of = [&] {
        const auto data = c.u32(p + player_at::data);
        c.set(4, data);
        return data;
    };
    if (test(role(p, 0x1020bdb0), 0x10)) {
        const auto data = data_of();
        if (compare(c, s8(c.u8(data + data_at::b1a)), 0xc) > 0 &&
            compare(c, s8(c.u8(data + data_at::b14)), s8(c.u8(data + data_at::b13))) >= 0) {
            bool placed = false;
            if (test(role(p, 0x1020bdec), 0x80)) {
                const auto again = data_of();
                if (compare(c, s8(c.u8(again + data_at::b19)), s8(c.u8(again + data_at::b18))) > 0) {
                    place(p, 0x120, 1, 0x1020be24);
                    placed = true;
                }
            }
            if (!placed && test(role(p, 0x1020be30), 0x800)) {
                const auto again = data_of();
                if (compare(c, s8(c.u8(again + data_at::b19)), s8(c.u8(again + data_at::b17))) > 0)
                    place(p, 0x420, 1, 0x1020be68);
            }
        }
    }

    // 0x1020be6c: attributes 0x1b, 0x15 and 0x14 balance (r23 keeps the running difference).
    bool applied = false;
    if (test(role(p, 0x1020be70), 0x40) && !test(role(p, 0x1020be80), 0x80) && !test(role(p, 0x1020be90), 0x800)) {
        r23 = reg(person_attribute(p, 0x1b, 0x1020bea4));
        const std::int32_t b = person_attribute(p, 0x15, 0x1020beb4);
        r23 = std::uint32_t(r23) - reg(b);
        const std::int32_t d = person_attribute(p, 0x14, 0x1020bec8);
        if (compare(c, std::int32_t(std::uint32_t(r23)) - d) > 0) {
            apply(p, 0, 0x8000000, 0x1020beec);
            applied = true;
        }
    }
    if (!applied) {
        r23 = reg(person_attribute(p, 0x1b, 0x1020befc));
        const std::int32_t b = person_attribute(p, 0x15, 0x1020bf0c);
        r23 = std::uint32_t(r23) - reg(b);
        const std::int32_t d = person_attribute(p, 0x14, 0x1020bf20);
        if (compare(c, std::int32_t(std::uint32_t(r23)) - d, -5) < 0) apply(p, 0, 0x4000000, 0x1020bf48);
    }

    // 0x1020bf4c: attribute 8 against the first loop's sum (the low byte of r27).
    const auto sum_value = [&] {
        c.set(4, exponent_word);
        return convert(high_pair, s8(r27));
    };
    // fmsub: pool * scale - sum, then the attribute's compare: true when it is above.
    const auto above_fused = [&](std::uint32_t site, std::uint32_t pool_offset, std::uint32_t at) {
        const std::int32_t value = person_attribute(p, 8, at);
        const double sum = sum_value();
        const double bar = fpu.multiply_add(site, pool(pool_offset), scale, sum);
        const double seen = convert(low_pair, value);
        fpu.fcmp(0, seen, bar);
        return std::pair{seen, bar};
    };
    const auto below_f31 = [&](std::uint32_t at) {
        const std::int32_t value = person_attribute(p, 8, at);
        const double sum = sum_value();
        const double bar = fpu.fsub(f31, sum);
        const double seen = convert(low_pair, value);
        fpu.fcmp(0, seen, bar);
        return std::pair{seen, bar};
    };
    if (test(role(p, 0x1020bf50), 0x80) || test(role(p, 0x1020bf60), 0x800)) {
        if (test(role(p, 0x1020bf70), 0x8) || test(role(p, 0x1020bf80), 0x10) || test(role(p, 0x1020bf90), 0x20)) {
            const auto [seen, bar] = below_f31(0x1020bfa4);
            if (seen > bar) {
                apply(p, 4, 0, 0x1020bffc);
            } else {
                const auto [seen2, bar2] = above_fused(0x1020c048, pool_at::c100, 0x1020c00c);
                if (seen2 < bar2) apply(p, 8, 0, 0x1020c068);
            }
        } else {
            const auto [seen, bar] = above_fused(0x1020c0b4, pool_at::cf8, 0x1020c078);
            if (seen > bar) {
                apply(p, 4, 0, 0x1020c0d4);
            } else {
                const auto [seen2, bar2] = above_fused(0x1020c120, pool_at::c118, 0x1020c0e4);
                if (seen2 < bar2) apply(p, 8, 0, 0x1020c140);
            }
        }
    } else {
        const auto [seen, bar] = above_fused(0x1020c18c, pool_at::cd0, 0x1020c150);
        if (seen > bar) {
            apply(p, 4, 0, 0x1020c1ac);
        } else {
            const auto [seen2, bar2] = below_f31(0x1020c1bc);
            if (seen2 < bar2) apply(p, 8, 0, 0x1020c214);
        }
    }
}

// 0x1020c218..0x1020c468: by the player's role word (the original's compare tree), the sum of
// attributes 7 and 0x16 against a pool value.
void SquadA::kind_rules(std::uint32_t p) {
    const std::int32_t kind = s16(role(p, 0x1020c21c));
    // 0 default, 1 and 2 the two groups; the compares go in the original's order (cr0).
    enum Group { other, first_group, second_group };
    const auto classify = [&](std::int32_t k) -> Group {
        const auto is = [&](std::int32_t value) { return compare(c, k, value) == 0; };
        std::int32_t order = compare(c, k, 0x210);
        if (order == 0) return second_group;
        if (order < 0) {
            order = compare(c, k, 0x110);
            if (order == 0) return second_group;
            if (order < 0) {
                order = compare(c, k, 0xa0);
                if (order == 0) return second_group;
                if (order < 0) return is(0x90) ? first_group : other;
                return is(0x108) ? first_group : other;
            }
            order = compare(c, k, 0x202);
            if (order == 0) return first_group;
            if (order < 0) return is(0x120) ? second_group : other;
            return is(0x208) ? first_group : other;
        }
        order = compare(c, k, 0x420);
        if (order == 0) return second_group;
        if (order < 0) {
            order = compare(c, k, 0x408);
            if (order == 0) return first_group;
            if (order < 0) return is(0x220) ? second_group : other;
            return is(0x410) ? second_group : other;
        }
        order = compare(c, k, 0x820);
        if (order == 0) return second_group;
        if (order > 0) return other;
        return is(0x810) ? first_group : other;
    };
    // Attributes 7 and 0x16: true when conv(a16 + a7) is above pool * scale; the first
    // attribute's value stays in r23.
    const auto sum_above = [&](std::uint32_t pool_offset, std::uint32_t at_first, std::uint32_t at_second) {
        r23 = reg(person_attribute(p, 7, at_first));
        const std::int32_t second = person_attribute(p, 0x16, at_second);
        const double bar = fpu.fmul(pool(pool_offset), scale);
        const double seen = convert(low_pair, second + s8(r23));
        fpu.fcmp(0, seen, bar);
        // What the original leaves in r3 (the converted sum's low word) and f1 (the sum as a double).
        c.set(3, reg(second + s8(r23)) ^ 0x80000000u);
        c.set_f(1, seen);
        return std::pair{seen, bar};
    };
    switch (classify(kind)) {
    case first_group: {
        const auto [seen, bar] = sum_above(pool_at::c120, 0x1020c2d4, 0x1020c2e4);
        if (seen > bar) apply(p, 1, 0, 0x1020c32c);
        break;
    }
    case second_group: {
        const auto [seen, bar] = sum_above(pool_at::c128, 0x1020c33c, 0x1020c34c);
        if (seen > bar) apply(p, 1, 0, 0x1020c394);
        break;
    }
    case other: {
        const auto [seen, bar] = sum_above(pool_at::c130, 0x1020c3a4, 0x1020c3b4);
        if (seen > bar) {
            apply(p, 1, 0, 0x1020c3fc);
        } else {
            const auto [seen2, bar2] = sum_above(pool_at::cc0, 0x1020c40c, 0x1020c41c);
            if (seen2 < bar2) apply(p, 2, 0, 0x1020c464);
        }
        break;
    }
    }
}

// 0x1020c468..0x1020c598: a notice to the registry when a trio of attributes outweighs the club's
// rating over 500 and the player's person is one of the side's three.
void SquadA::closing(std::uint32_t p) {
    const bool candidate = (test(role(p, 0x1020c46c), 0x10) || test(role(p, 0x1020c47c), 0x8) ||
                            test(role(p, 0x1020c48c), 0x20)) &&
                           !test(role(p, 0x1020c49c), 0x80) && !test(role(p, 0x1020c4ac), 0x800) &&
                           !test(role_more(p, 0x1020c4bc), 0x40);
    if (!candidate) return;
    r24 = reg(person_attribute(p, 9, 0x1020c4d0));
    r23 = reg(person_attribute(p, 0x16, 0x1020c4e0));
    const std::int32_t seven = person_attribute(p, 7, 0x1020c4f0);
    const auto club = field(side_at::club);
    const std::int32_t rating = c.i16(club + squad_club::rating);
    const std::int32_t pair = s8(r24) + seven;
    c.set({{4, club}, {6, exponent_word}, {7, reg(seven)}, {5, reg(pair)}});
    // mulhw by 0x10624dd3 and srawi 5 divide the rating by 500.
    const double seen = convert(low_pair, s8(r23) + pair);
    const double quotient = convert(high_pair, rating / 500);
    const double bar = fpu.multiply_add(0x1020c554, pool(pool_at::ca8), scale, quotient);
    fpu.fcmp(0, seen, bar);
    // r3 ends as the sign bit of the shifted high word (rlwinm r3,r0,1,31,31); f1 holds the pool value.
    c.set(3, (std::int32_t(mulhw(0x10624dd3, rating)) >> 5) < 0 ? 1 : 0);
    c.set_f(1, pool(pool_at::ca8));
    if (!(seen > bar)) return;
    const auto person = c.u32(p + player_at::person);
    c.set(6, person);
    bool listed = false;
    for (const auto offset : side_at::others) {
        if (compare_logical(c, field(offset), person) == 0) {
            listed = true;
            break;
        }
    }
    if (!listed) return;
    const auto notices = c.u32(c.toc(-0x7a6c));
    c.set({{3, notices}, {5, 0}, {7, c.u8(m + side_at::kind)}});
    c.call(call_to::notice, 0x1020c598);
}

void SquadA::player(std::uint32_t p) {
    position_rules(p);
    tendencies(p);
    scenarios(p);
    form_rules(p);
    fatigue_rules(p);
    sum_rules(p);
    kind_rules(p);
    closing(p);
}

void SquadA::run() {
    if (!opening()) return;
    orders_from_ratings();
    first_loop();
    const std::int32_t side_index = s8(c.u8(m + side_at::own_side));
    r22 = 0;
    r21 = c.u32(m + side_at::match) + reg(side_index * int(player_at::table_stride)) + player_at::first;
    do {
        const std::uint32_t p = r21;
        if (compare(c, s8(c.u8(p + player_at::slot))) >= 0) player(p);
        r22 = r22 + 1;
        r21 = r21 + player_at::size;
    } while (compare(c, std::int32_t(std::uint32_t(r22)), 0x14) < 0);
}
}

void squad_a(Cpu& c, std::uint32_t side) {
    constexpr std::uint32_t entry = 0x1020a290;
    c.set(0, c.lr());
    // r21-r31 at -0x4c, f28-f31 at -0x20 (the stmw and savefpr_28 of the prologue).
    Frame frame(c, entry, 0xa0, {.first_gpr = 21, .first_fpr = 28});
    SquadA routine(c, frame, side);
    routine.run();
}
}
