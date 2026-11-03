#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
// The shot owns two caller-provided outputs. Keep their write order: either
// may alias the actor, the RNG state, or the other output.
class ShotSelection {
    Cpu& c;
    Frame frame;
    Fpu fp;
    Player actor;
    const std::uint32_t action, event, doubles, floats, distances;
    const std::uint32_t seed, begin, end, cursor;
    Kept<17> keeper;
    Kept<29> score;
    std::uint32_t at(unsigned offset) const { return frame.at(offset); }
    void advance(std::uint32_t back);
    void measure_geometry();
    std::uint32_t special_shot();
    bool accepts_goal(std::uint32_t upper_back, std::uint32_t lower_back);
    bool exceptional_candidate();
    bool select_action();
    void draw_attribute(unsigned offset, std::uint32_t back);
    void draw_biased_attribute(unsigned offset, int multiplier, std::uint32_t back);
    template<bool Ground> void draw_placement();
    void score_candidates(unsigned mode);
    struct FitnessBound { std::int32_t value; unsigned sign; };
    FitnessBound fitness_bound(unsigned slot, unsigned first, unsigned second);
    void draw_fitness(bool exceptional);
    void exceptional_rating();
    void ordinary_rating();
    void diagnose_selection();
    void sample_context_bound(std::uint32_t refill_back, std::uint32_t recursive_back);
    void low_context_attributes(bool fourth_mode);
    void context_rating(bool action_five);
    void lateral_penalty();
    void sample_lateral_penalty();
    void mark_long_shot();
    void match_bias();
    void paired_match_bias(unsigned kind);
    void keeper_adjustment();
    void distance_penalty();
    void long_distance_penalty();
    std::uint32_t write_event();
    static std::int32_t high(std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    }
    static std::int32_t remainder(std::uint32_t word, std::int32_t bound) {
        // divide_signed widens before division. In particular, INT32_MIN / -1
        // follows the reference's wrapped quotient without host signed UB.
        const auto quotient = std::uint32_t(divide_signed<4>(word, std::uint32_t(bound)));
        return std::int32_t(word - quotient * std::uint32_t(bound));
    }
    std::int32_t shifted(std::int32_t value, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, value < 0 && (std::uint32_t(value) & ((1u << bits) - 1)));
        return value >> bits;
    }
public:
    ShotSelection(Cpu& cpu, std::uint32_t player, std::uint32_t action_output, std::uint32_t event_output)
        : c(cpu), frame(c, 0x10252080, 0x130, {.first_gpr = 13, .gpr_offset = -0x6c, .first_fpr = 29}),
          fp(c), actor{c, player}, action(action_output), event(event_output),
          doubles(c.toc(-0x51e8)), floats(c.toc(-0x51e0)), distances(c.toc(-0x6424)),
          seed(c.toc(-0x7b90)), begin(c.toc(-0x7b8c)), end(c.toc(-0x7b88)), cursor(c.toc(-0x7b84)),
          keeper(c, c.r(17)), score(c, c.r(29)) {
        keep(c, {{31, player}, {13, c.toc(-0x7ba8)}, {24, action}, {18, distances}, {23, event},
                 {27, doubles}, {28, seed}, {26, begin}, {25, end}, {30, cursor},
                 {14, c.toc(-0x51e4)}, {21, floats}});
    }
    std::uint32_t run();
};

void ShotSelection::advance(std::uint32_t back) {
    const auto next = c.u32(cursor) + 2;
    c.set(3, next);
    const auto order = compare_logical(c, next, c.u32(end));
    c.put32(cursor, next);
    if (order > 0) {
        c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff);
    }
}

void ShotSelection::measure_geometry() {
    const auto side = actor.side();
    c.set(3, actor.match()); c.set(4, std::int8_t(side) == 0);
    c.call(0x101f1e10, 0x102520e8);
    const auto keeper_row = c.i8(c.r(3) + 0x106);
    keeper = c.r(3); c.set(4, c.u8(actor.address + 0x10e));
    keep(c, {{15, 0}, {16, 0}});
    c.set(3, std::uint32_t(2 * int(std::int8_t(c.r(4)))));
    const auto on_far_line = compare(c, keeper_row, 11) == 0;
    c.s.wr<1>(reg::xer_ca, c.r(3) <= std::uint32_t(-10));
    score = extended(std::int8_t(-10 - c.r(3)));
    if (!on_far_line && compare(c, c.i8(std::uint32_t(keeper) + 0x106)) != 0) {
        enter(c, 0x1025212c, {keeper}); current_role(c, keeper);
        if (compare(c, std::int16_t(c.r(3)), 1) == 0) {
            c.set(3, std::uint32_t(2 * int(c.i8(actor.address + 0x10e))));
            score = extended(std::int8_t(c.r(3) + 10));
        }
    }
    const auto keeper_column = c.i8(std::uint32_t(keeper) + 0x105);
    const auto column = actor.column();
    Kept<19> axis(c, c.u8(std::uint32_t(keeper) + 0x106));
    c.set(3, std::uint32_t(int(column) - keeper_column)); c.call(0x10775540, 0x10252168);
    const auto row = actor.row(); c.set(4, extended(std::int8_t(std::uint32_t(axis))));
    axis = extended(std::int8_t(c.r(3)));
    c.set(3, std::uint32_t(int(row) - std::int32_t(c.r(4)))); c.call(0x10775540, 0x10252180);
    c.set(3, std::uint32_t(int(std::int8_t(c.r(3))) * 36));
    c.set_f(0, c.f32(floats + 0x34));
    c.set_f(1, c.f32(distances + c.r(3) + std::uint32_t(int(std::int8_t(std::uint32_t(axis))) * 4)));
    fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 8) {
        enter(c, 0x102521ac, {keeper}); current_role(c, keeper);
        if (compare(c, std::int16_t(c.r(3)), 1) == 0)
            score = std::uint32_t(score) - std::uint32_t(c.i8(actor.address + 0x10e));
    }
    const bool upper = compare(c, std::int8_t(actor.side())) == 0;
    c.set(3, std::uint32_t(int(actor.column()) - 4));
    c.call(0x10775540, upper ? 0x102521e0 : 0x10252220);
    const auto goal_row = actor.row(); axis = extended(std::int8_t(c.r(3)));
    c.set(3, std::uint32_t(int(goal_row) - (upper ? 11 : 0)));
    c.call(0x10775540, upper ? 0x102521f4 : 0x10252234);
    c.set(3, std::uint32_t(int(std::int8_t(c.r(3))) * 36));
    c.set_f(0, c.f32(distances + c.r(3) + std::uint32_t(int(std::int8_t(std::uint32_t(axis))) * 4)));
}

std::uint32_t ShotSelection::special_shot() {
    c.put16(event, 0x1f7f);
    Kept<13> skill(c, extended(c.i16(actor.address + 0x184)));
    Kept<14> draw(c, c.r(14));
    const auto bound = 5 * int(std::int16_t(std::uint32_t(skill)));
    if (compare(c, bound) == 0) draw = 0;
    else {
        advance(0x102522a0); c.set(3, 65535);
        bool narrow = compare(c, bound, 65535) <= 0;
        if (narrow) { c.set(3, std::uint32_t(-65535)); narrow = compare(c, bound, -65535) >= 0; }
        if (narrow) {
            c.set(4, c.u32(cursor)); c.set(3, extended(std::int16_t(std::uint32_t(skill))));
            c.set(5, c.u32(seed)); c.set(6, c.u16(c.r(4)));
            c.set(4, std::uint32_t(bound)); c.set(5, c.r(6) + c.r(5));
            c.set(3, std::uint32_t(divide_signed<4>(c.r(5), c.r(4))) * c.r(4));
            draw = c.r(5) - c.r(3);
            if (compare(c, bound) <= 0) draw = 0u - std::uint32_t(draw);
        } else {
            c.set(3, std::uint32_t(bound)); c.put32(at(0xb0), 0x43300000);
            c.set(7, c.u32(cursor)); c.set(6, 0x80010000); c.set(4, 0x43300000);
            c.set_f(2, c.f64(doubles + 0x80)); c.set(8, c.u16(c.r(7)));
            c.put32(at(0xb4), std::uint32_t(bound) ^ 0x80000000);
            c.set(5, 0x10000); c.set(7, c.u32(seed)); c.set_f(1, c.f64(at(0xb0)));
            c.set(3, 0x80010000); c.set_f(0, c.f64(doubles)); c.set(7, c.r(8) + c.r(7));
            c.set_f(2, fp.fsub(c.f(1), c.f(2))); c.set(6, 0x80008001); c.put32(at(0xa8), c.r(4));
            c.set(4, std::uint32_t(high(c.r(7), std::int32_t(c.r(6))))); c.set(5, 65535);
            c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, fp.fdiv(c.f(2), c.f(0))); c.set(3, 0x80008001);
            const auto partial = shifted(std::int32_t(c.r(4) + c.r(7)), 15);
            c.set(6, partial < 0); c.set(4, std::uint32_t(partial + int(c.r(6))) * c.r(5));
            draw = c.r(7) - c.r(4); c.set(4, std::uint32_t(draw) ^ 0x80000000); c.put32(at(0xac), c.r(4));
            const auto numerator = std::uint32_t(50 * int(std::int16_t(std::uint32_t(skill))));
            c.set_f(0, c.f64(at(0xa8))); c.set(3, std::uint32_t(high(numerator, std::int32_t(c.r(3)))));
            c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
            const auto quotient = shifted(std::int32_t(c.r(3) + numerator), 15);
            c.set(3, quotient < 0); const auto converted = fp.fctiwz(0, c.f(0));
            c.set(3, std::uint32_t(quotient) + c.r(3)); c.put64(at(0xa0), converted); draw = c.u32(at(0xa4));
            enter(c, 0x10252394); sample_range(c, c.r(3));
            const auto correction = c.r(3) + 5;
            c.set(4, 0x66666667); c.set(3, std::uint32_t(high(correction, std::int32_t(c.r(4)))));
            c.set(4, std::uint32_t(shifted(std::int32_t(c.r(3)), 2))); c.set(3, std::uint32_t(bound));
            draw = std::uint32_t(draw) + c.r(4) + (std::int32_t(c.r(4)) < 0);
            if (compare(c, std::int32_t(std::uint32_t(draw)), bound - 1) > 0) draw = std::uint32_t(bound - 1);
        }
    }
    return result(c, extended(std::int16_t(std::uint32_t(draw) + 150)));
}

bool ShotSelection::accepts_goal(std::uint32_t upper_back, std::uint32_t lower_back) {
    const bool upper = compare(c, std::int8_t(actor.side())) == 0;
    bool at_goal = compare(c, upper ? int(c.u8(actor.address + 0x106)) : int(actor.row()), upper ? 11 : 0) == 0;
    if (at_goal) at_goal = compare(c, actor.column(), 3) >= 0;
    if (at_goal) at_goal = compare(c, actor.column(), 5) <= 0;
    if (at_goal) c.set(3, 1);
    else {
        c.set_f(1, c.f32(floats)); c.set(3, actor.address); c.set_f(2, c.f32(floats));
        c.set(4, 4); c.set(5, upper ? 11 : 0); c.set(6, 0); c.set(9, 0);
        enter(c, upper ? upper_back : lower_back);
        accepts_target(c, actor.address, 4, upper ? 11 : 0, 0, 0, c.f(1), c.f(2));
    }
    return compare(c, std::int8_t(c.r(3))) != 0;
}

bool ShotSelection::exceptional_candidate() {
    if (!accepts_goal(0x102524a8, 0x10252500)) return false;
    c.set(3, actor.match());
    if (compare(c, c.i8(c.r(3) + 0x791f), 3) >= 0) return false;
    const auto next = c.u32(cursor) + 2, limit = c.u32(end);
    c.set(3, next); Kept<19> axis(c, c.u8(actor.address + 0x10e));
    const auto order = compare_logical(c, next, limit); c.put32(cursor, next);
    if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, 0x10252540); c.put32(seed, c.r(3) & 0xffff); }
    c.set(5, c.u32(cursor)); c.set(3, 0x55555556); c.set(4, c.u32(seed)); c.set(5, c.u16(c.r(5)));
    c.set(5, c.r(5) + c.r(4)); c.set(4, std::uint32_t(high(c.r(5), std::int32_t(c.r(3)))));
    c.set(3, std::uint32_t(std::int32_t(c.r(5)) % 3));
    bool eligible = compare(c, std::int8_t(std::uint32_t(axis)), std::int32_t(c.r(3))) > 0;
    if (!eligible) {
        if (compare(c, std::int8_t(actor.side())) == 0) {
            c.set(3, extended(actor.row()));
            eligible = compare(c, std::int32_t(c.r(3)), c.i8(std::uint32_t(keeper) + 0x106)) > 0;
        }
        if (!eligible) {
            if (compare(c, actor.side(), 1) != 0) return false;
            c.set(3, extended(actor.row()));
            if (compare(c, std::int32_t(c.r(3)), c.i8(std::uint32_t(keeper) + 0x106)) >= 0) return false;
        }
    }
    bool forced = false;
    if (compare(c, c.i8(std::uint32_t(keeper) + 0x2c), 1) > 0) {
        const bool upper = compare(c, std::int8_t(actor.side()) == 0, 1) == 0;
        const auto column = actor.column(); c.set(3, 0);
        bool in_area = compare(c, column, 2) >= 0;
        if (in_area) in_area = compare(c, actor.column(), 6) <= 0;
        if (in_area) in_area = compare(c, actor.row(), upper ? 10 : 0) >= 0;
        if (in_area) in_area = compare(c, actor.row(), upper ? 11 : 1) <= 0;
        if (in_area) c.set(3, 1);
        forced = compare(c, std::int8_t(c.r(3))) != 0;
    }
    if (!forced) {
        enter(c, 0x1025269c, {keeper}); current_role(c, keeper);
        forced = compare(c, std::int16_t(c.r(3)), 1) != 0;
    }
    if (!forced) {
        const bool upper = compare(c, c.u8(std::uint32_t(keeper) + 0x27), 1) == 0;
        c.set(3, std::uint32_t(int(c.i8(std::uint32_t(keeper) + 0x105)) - 4));
        c.call(0x10775540, upper ? 0x102526c4 : 0x10252704);
        const auto row = c.i8(std::uint32_t(keeper) + 0x106); axis = extended(std::int8_t(c.r(3)));
        c.set(3, std::uint32_t(int(row) - (upper ? 11 : 0))); c.call(0x10775540, upper ? 0x102526d8 : 0x10252718);
        c.set(3, std::uint32_t(int(std::int8_t(c.r(3))) * 36));
        c.set_f(1, c.f32(distances + c.r(3) + std::uint32_t(int(std::int8_t(std::uint32_t(axis))) * 4)));
        c.set_f(0, c.f64(doubles + 0xc8)); fp.fcmp(0, c.f(1), c.f(0));
        c.set_cr(0, (c.cr(0) & ~2u) | ((c.cr(0) & 6) ? 2 : 0));
        if (c.cr(0) & 2) {
            const auto column = actor.column(); const auto keeper_column = c.i8(std::uint32_t(keeper) + 0x105);
            axis = c.u8(actor.address + 0x106);
            c.set(3, std::uint32_t(int(keeper_column) - column)); c.call(0x10775540, 0x1025275c);
            const auto keeper_row = c.i8(std::uint32_t(keeper) + 0x106);
            c.set(4, extended(std::int8_t(std::uint32_t(axis)))); axis = extended(std::int8_t(c.r(3)));
            c.set(3, std::uint32_t(int(keeper_row) - std::int32_t(c.r(4)))); c.call(0x10775540, 0x10252774);
            c.set(3, std::uint32_t(int(std::int8_t(c.r(3))) * 36)); c.set_f(0, c.f64(doubles + 0x10));
            c.set_f(1, c.f32(distances + c.r(3) + std::uint32_t(int(std::int8_t(std::uint32_t(axis))) * 4)));
            fp.fcmp(0, c.f(1), c.f(0)); c.set_cr(0, (c.cr(0) & ~2u) | ((c.cr(0) & 6) ? 2 : 0));
            forced = (c.cr(0) & 2) != 0;
        }
    }
    if (!forced) {
        axis = extended(c.i16(actor.address + 0x2a)); Kept<18> draw(c, c.r(18));
        const auto bound = std::int32_t(std::uint32_t(axis));
        if (compare(c, bound) == 0) draw = 0;
        else {
            advance(0x102527d0);
            c.set(3, 65535); compare(c, bound, 65535);
            c.set(3, std::uint32_t(-65535)); compare(c, bound, -65535);
            // The source's two wide arms cannot accept a signed halfword.
            c.set(3, c.u32(cursor)); compare(c, bound); const auto bias = c.u32(seed);
            c.set(3, std::uint32_t(c.u16(c.r(3))) + bias);
            const auto sampled = remainder(c.r(3), bound);
            draw = std::uint32_t(bound > 0 ? sampled : -sampled);
        }
        advance(0x1025292c);
        c.set(4, c.u32(cursor)); c.set(3, 0x10624dd3); const auto bias = c.u32(seed);
        c.set(4, std::uint32_t(c.u16(c.r(4))) + bias);
        const auto quotient = shifted(high(c.r(4), std::int32_t(c.r(3))), 7); c.set(3, quotient < 0);
        if (compare(c, std::int32_t(std::uint32_t(draw)), std::int32_t(c.r(4)) % 2000) <= 0) return false;
        c.set_f(0, c.f64(doubles + 0x90)); fp.fcmp(0, c.f(29), c.f(0));
        if (!(c.cr(0) & 8)) return false;
    }
    c.put8(action, 58); return true;
}

bool ShotSelection::select_action() {
    if (compare(c, c.u8(action), 53) == 0) return true;
    if (compare(c, c.i16(actor.address + 0x19c), 59) == 0) { c.put8(action, 57); return true; }
    if (exceptional_candidate()) return true;
    if (compare(c, c.i8(action), -1) > 0) return true;
    c.set(3, actor.match());
    if (compare(c, c.i8(c.r(3) + 0x791f)) == 0) {
        if (!accepts_goal(0x10252a04, 0x10252a5c)) return false;
        score_candidates(0); return true;
    }
    c.set(3, actor.match());
    if (compare(c, c.u8(c.r(3) + 0x791f), 1) == 0) {
        if (!accepts_goal(0x10253498, 0x102534f0)) return false;
        score_candidates(1); return true;
    }
    c.set(3, actor.match());
    bool moving = compare(c, c.u8(c.r(3) + 0x791f), 2) == 0;
    if (!moving) { c.set(3, actor.match()); moving = compare(c, c.u8(c.r(3) + 0x791f), 3) == 0; }
    if (moving) {
        if (!accepts_goal(0x10253d84, 0x10253ddc)) {
            c.set(3, actor.match());
            if (compare(c, c.u8(c.r(3) + 0x791f), 3) != 0) return false;
            c.put8(action, 5); return true;
        }
        score_candidates(2); return true;
    }
    c.set(3, actor.match());
    if (compare(c, c.u8(c.r(3) + 0x791f), 4) == 0) {
        c.put8(action, accepts_goal(0x10254234, 0x1025428c) ? 3 : 5); return true;
    }
    c.set(3, actor.match()); compare(c, c.u8(c.r(3) + 0x791f), 5); return false;
}

void ShotSelection::draw_attribute(unsigned offset, std::uint32_t back) {
    Kept<18> skill(c, extended(c.i16(actor.address + offset)));
    Kept<19> draw(c, c.r(19));
    const auto bound = std::int32_t(std::uint32_t(skill));
    if (compare(c, bound) == 0) { draw = 0; return; }
    advance(back);
    // A sign-extended halfword is always in the original narrow interval.
    c.set(3, 65535); compare(c, bound, 65535);
    c.set(3, std::uint32_t(-65535)); compare(c, bound, -65535);
    c.set(5, c.u32(cursor)); c.set(3, std::uint32_t(bound)); c.set(4, c.u32(seed)); compare(c, bound);
    const auto word = c.u16(c.r(5)); c.set(4, word + c.r(4));
    const auto sampled = remainder(c.r(4), bound);
    draw = std::uint32_t(bound > 0 ? sampled : -sampled);
}

void ShotSelection::draw_biased_attribute(unsigned offset, int multiplier, std::uint32_t back) {
    Kept<18> skill(c, extended(c.i16(actor.address + offset)));
    Kept<19> draw(c, c.r(19));
    const auto bias = int(std::int8_t(std::uint32_t(score))) * multiplier;
    const auto bound = std::int32_t(std::uint32_t(skill)) + bias;
    c.set(3, std::uint32_t(bias));
    if (compare(c, bound) == 0) { draw = 0; return; }
    advance(back);
    // multipliers 20 and 22 give [-35584,35561] at most. The original
    // wide arms are unreachable even for out-of-range game attributes.
    c.set(3, 65535); c.set(4, std::uint32_t(bias)); compare(c, bound, 65535);
    c.set(3, std::uint32_t(-65535)); c.set(4, std::uint32_t(bias)); compare(c, bound, -65535);
    c.set(3, c.u32(cursor)); c.set(5, std::uint32_t(bias)); c.set(4, std::uint32_t(skill));
    c.set(7, c.u16(c.r(3))); c.set(3, extended(std::int8_t(std::uint32_t(score)))); c.set(6, c.u32(seed));
    c.set(6, c.r(7) + c.r(6)); c.set(5, std::uint32_t(bound));
    c.set(4, std::uint32_t(divide_signed<4>(c.r(6), std::uint32_t(bound))) * c.r(5));
    c.set(3, std::uint32_t(bias)); draw = c.r(6) - c.r(4);
    if (compare(c, bound) <= 0) draw = 0u - std::uint32_t(draw);
}

template<bool Ground> void ShotSelection::draw_placement() {
    const auto next = c.u32(cursor) + 2, limit = c.u32(end); c.set(3, next);
    Kept<Ground ? 29 : 18> skill(c, extended(c.i16(actor.address + 0x158)));
    const auto ordering = compare_logical(c, next, limit); c.put32(cursor, next);
    if (ordering > 0) {
        c.put32(cursor, begin); c.call(0x1077b770, Ground ? 0x10252fec : 0x10253720); c.put32(seed, c.r(3) & 0xffff);
    }
    c.set(5, c.u32(cursor)); c.set(4, 0x1b4e81b5); c.set(3, c.u32(cursor));
    c.set(6, c.u16(c.r(5))); c.set(5, c.u32(seed)); c.set(3, c.r(3) + 2);
    const auto second_limit = c.u32(end); c.set(5, c.r(6) + c.r(5)); c.put32(cursor, c.r(3));
    c.set(4, std::uint32_t(high(c.r(5), std::int32_t(c.r(4)))));
    const auto second_order = compare_logical(c, c.r(3), second_limit);
    const auto partial = shifted(std::int32_t(c.r(4)), 4); c.set(3, partial < 0);
    Kept<Ground ? 20 : 19> variation(c, std::uint32_t(std::int32_t(c.r(5)) % 150));
    if (second_order > 0) {
        c.put32(cursor, begin); c.call(0x1077b770, Ground ? 0x10253044 : 0x10253778); c.put32(seed, c.r(3) & 0xffff);
    }
    c.set(5, c.u32(cursor)); c.set(3, 0x10624dd3); c.set(4, c.u32(seed)); c.set(5, c.u16(c.r(5)));
    c.set(5, c.r(5) + c.r(4)); c.set(3, std::uint32_t(high(c.r(5), std::int32_t(c.r(3)))));
    const auto signed_quotient = shifted(std::int32_t(c.r(3)), 5); c.set(4, signed_quotient < 0);
    c.set(3, std::uint32_t(signed_quotient + int(c.r(4))) * 500);
    Kept<Ground ? 19 : 20> penalty(c, c.r(5) - c.r(3));
    const auto bound = int(std::int16_t(std::uint32_t(skill))) + std::int32_t(std::uint32_t(variation)) - std::int32_t(std::uint32_t(penalty));
    Kept<Ground ? 18 : 29> draw(c, c.r(Ground ? 18 : 29));
    if (compare(c, bound) == 0) draw = 0;
    else {
        advance(Ground ? 0x102530b4 : 0x102537e8);
        // Signed remainders are within +/-149 and +/-499. Combined with
        // the saved halfword this bound remains in [-33416,33415].
        c.set(3, 65535); compare(c, bound, 65535);
        c.set(3, std::uint32_t(-65535)); compare(c, bound, -65535);
        c.set(4, c.u32(cursor)); c.set(3, std::uint32_t(int(std::int16_t(std::uint32_t(skill))) + std::int32_t(std::uint32_t(variation))));
        c.set(5, c.u16(c.r(4))); c.set(4, c.u32(seed)); c.set(3, std::uint32_t(bound)); compare(c, bound);
        c.set(4, c.r(5) + c.r(4)); const auto sampled = remainder(c.r(4), bound);
        draw = std::uint32_t(bound > 0 ? sampled : -sampled);
    }
    const auto candidate = std::int16_t(std::uint32_t(draw)); c.set(3, extended(std::int16_t(c.r(22))));
    if (compare(c, candidate, std::int32_t(c.r(3))) > 0) {
        c.set(3, 29); c.set(22, extended(candidate)); c.put8(action, c.r(3));
    }
}

void ShotSelection::score_candidates(unsigned mode) {
    if (mode == 0) {
        draw_attribute(0x14a, 0x10252a98);
        c.set(3, 22); const auto bias = int(std::int8_t(std::uint32_t(score))) * 20;
        c.put8(action, c.r(3)); c.set(3, std::uint32_t(bias)); c.set(22, extended(std::int16_t(c.r(19))));
        draw_biased_attribute(0x152, 20, 0x10252c3c);
        const auto candidate = std::int16_t(c.r(19)); c.set(3, extended(std::int16_t(c.r(22))));
        if (compare(c, candidate, std::int32_t(c.r(3))) > 0) {
            c.set(3, 26); c.set(22, extended(candidate)); c.put8(action, c.r(3));
        }
        c.set_f(0, c.f64(doubles + 0x90)); fp.fcmp(0, c.f(29), c.f(0));
        c.set_cr(0, (c.cr(0) & ~2u) | ((c.cr(0) & 6) ? 2 : 0));
        if (c.cr(0) & 2) {
            draw_attribute(0x156, 0x10252e54);
            const auto candidate = std::int16_t(c.r(19)); c.set(3, extended(std::int16_t(c.r(22))));
            if (compare(c, candidate, std::int32_t(c.r(3))) > 0) {
                c.set(3, 28); c.set(22, extended(candidate)); c.put8(action, c.r(3));
            }
        }
        draw_placement<true>(); draw_attribute(0x14c, 0x1025329c);
    } else {
        draw_biased_attribute(0x154, 22, mode == 1 ? 0x10253538 : 0x10253e24);
        c.set(22, extended(std::int16_t(c.r(19)))); c.put8(action, 27);
        if (mode == 1) {
            draw_placement<false>(); draw_attribute(0x14a, 0x102539d0);
            const auto candidate = std::int16_t(c.r(19)); c.set(3, extended(std::int16_t(c.r(22))));
            if (compare(c, candidate, std::int32_t(c.r(3))) > 0) {
                c.set(3, 22); c.set(22, extended(candidate)); c.put8(action, c.r(3));
            }
            draw_attribute(0x150, 0x10253b78);
        } else draw_attribute(0x14e, 0x1025401c);
    }
    const auto candidate = std::int16_t(c.r(19)); c.set(3, extended(std::int16_t(c.r(22))));
    if (compare(c, candidate, std::int32_t(c.r(3))) > 0) c.put8(action, mode == 0 ? 23 : mode == 1 ? 25 : 24);
}

ShotSelection::FitnessBound ShotSelection::fitness_bound(unsigned slot, unsigned first, unsigned second) {
    const auto a = fp.fctiwz(1, c.f(first)), b = fp.fctiwz(0, c.f(second));
    c.put64(at(slot), a); c.put64(at(slot + 8), b);
    const auto left = std::uint32_t(10000 - std::int16_t(c.u32(at(slot + 4))));
    const auto right = std::uint32_t(10000 - std::int16_t(c.u32(at(slot + 12))));
    const auto product = left * right;
    const auto quotient = shifted(high(product, 0x14f8b589), 12);
    return {quotient + int(quotient < 0), unsigned(quotient < 0)};
}

void ShotSelection::draw_fitness(bool exceptional) {
    const unsigned first = exceptional ? 31 : 30, second = exceptional ? 30 : 31;
    const unsigned slot = exceptional ? 0xa0 : 0x60;
    c.set_f(first, c.f32(actor.address + 0x50)); c.set(4, 0x14f90000);
    c.set_f(second, c.f32(actor.address + 0x50)); c.set(4, 0x14f8b589);
    Kept<13> draw(c, c.r(13));
    const auto initial = fitness_bound(slot, first, second); c.set(3, initial.sign);
    if (compare(c, initial.value) == 0) { draw = 0; return; }
    advance(exceptional ? 0x10254450 : 0x10254d48);
    c.set(5, 0x14f8b589); c.set(3, 65535);
    auto bound = fitness_bound(slot, first, second); c.set(4, bound.sign); compare(c, bound.value, 65535);
    c.set(5, 0x14f8b589); c.set(3, std::uint32_t(-65535));
    bound = fitness_bound(slot, first, second); c.set(4, bound.sign); compare(c, bound.value, -65535);
    // Each input narrows to int16. Even an arbitrary signed32 product /50000
    // cannot leave [-42949,42949], so both expanded wide arms are dead.
    const auto first_bits = fp.fctiwz(0, c.f(first)); c.set(3, c.u32(cursor));
    const auto second_bits = fp.fctiwz(2, c.f(second)); c.set(7, 0x14f90000);
    const auto repeated_first = fp.fctiwz(1, c.f(first)); c.set(5, 0x14f90000);
    c.put64(at(0x60), first_bits); const auto repeated_second = fp.fctiwz(0, c.f(second));
    c.set(8, c.u16(c.r(3))); c.set(7, 0x14f8b589); c.put64(at(0x68), second_bits); c.set(5, 0x14f8b589);
    c.set(3, extended(std::int16_t(c.u32(at(0x64))))); const auto other = std::int16_t(c.u32(at(0x6c)));
    c.set(4, c.u32(seed)); c.set(3, 10000u - c.r(3)); const auto right = std::uint32_t(10000 - int(other));
    c.put64(at(0x70), repeated_first); c.set(6, c.r(3) * right); const auto word = c.r(8) + c.r(4);
    c.put64(at(0x78), repeated_second);
    c.set(4, extended(std::int16_t(c.u32(at(0x74))))); c.set(3, extended(std::int16_t(c.u32(at(0x7c)))));
    c.set(6, std::uint32_t(shifted(high(c.r(6), std::int32_t(c.r(7))), 12)));
    c.set(7, std::int32_t(c.r(6)) < 0); c.set(4, 10000u - c.r(4));
    c.set(7, c.r(6) + c.r(7)); c.s.wr<1>(reg::xer_ca, c.r(3) <= 10000); c.set(3, 10000u - c.r(3));
    c.set(6, divide_signed<4>(word, c.r(7)));
    c.set(3, c.r(4) * c.r(3)); c.set(3, std::uint32_t(high(c.r(3), std::int32_t(c.r(5)))));
    c.set(5, c.r(6) * c.r(7)); c.set(3, std::uint32_t(shifted(std::int32_t(c.r(3)), 12)));
    c.set(4, std::int32_t(c.r(3)) < 0); draw = word - c.r(5);
    if (compare(c, std::int32_t(c.r(3) + c.r(4))) <= 0) draw = 0u - std::uint32_t(draw);
}

void ShotSelection::exceptional_rating() {
    draw_fitness(true);
    c.set(3, 0x66660000); const auto bias = c.i16(actor.address + 0x1a0); c.set(3, 0x66666667);
    Kept<18> condition(c, extended(std::int16_t(c.r(13))));
    const auto bias_partial = shifted(high(std::uint32_t(bias), std::int32_t(c.r(3))), 1);
    c.set(3, bias_partial < 0); Kept<13> penalty(c, std::uint32_t(bias_partial) + c.r(3));
    Kept<19> bonus(c, c.r(19));
    const auto bonus_bound = std::int32_t(std::uint32_t(penalty));
    if (compare(c, bonus_bound) == 0) bonus = 0;
    else {
        advance(0x102547f4); c.set(3, 65535); compare(c, bonus_bound, 65535);
        c.set(3, std::uint32_t(-65535)); compare(c, bonus_bound, -65535);
        c.set(3, c.u32(cursor)); compare(c, bonus_bound); const auto bias = c.u32(seed);
        c.set(3, std::uint32_t(c.u16(c.r(3))) + bias);
        const auto sampled = remainder(c.r(3), bonus_bound); bonus = std::uint32_t(bonus_bound > 0 ? sampled : -sampled);
    }
    c.set(3, 0x66666667);
    const auto fitness = std::int16_t(std::uint32_t(condition));
    auto partial = shifted(high(std::uint32_t(fitness), std::int32_t(c.r(3))), 1);
    c.set(3, partial < 0); const auto penalty_bound = partial + int(c.r(3));
    if (compare(c, penalty_bound) == 0) penalty = 0;
    else {
        advance(0x10254978);
        for (const auto limit : {65535, -65535}) {
            c.set(4, 0x66666667); c.set(3, std::uint32_t(limit));
            partial = shifted(high(std::uint32_t(fitness), std::int32_t(c.r(4))), 1);
            c.set(4, partial < 0); compare(c, partial + int(c.r(4)), limit);
        }
        c.set(4, 0x66660000); c.set(5, c.u32(cursor)); c.set(3, 0x66660000); c.set(4, 0x66666667);
        c.set(6, c.u16(c.r(5))); c.set(4, std::uint32_t(high(std::uint32_t(fitness), std::int32_t(c.r(4)))));
        c.set(5, c.u32(seed)); c.set(3, 0x66666667); c.set(6, c.r(6) + c.r(5));
        c.set(4, std::uint32_t(shifted(std::int32_t(c.r(4)), 1))); c.set(5, std::int32_t(c.r(4)) < 0); c.set(5, c.r(4) + c.r(5));
        c.set(4, divide_signed<4>(c.r(6), c.r(5)));
        const auto sign_partial = shifted(high(std::uint32_t(fitness), std::int32_t(c.r(3))), 1);
        c.set(4, c.r(4) * c.r(5)); c.set(3, sign_partial < 0);
        compare(c, sign_partial + int(c.r(3))); penalty = c.r(6) - c.r(4);
        if (!(c.cr(0) & 4)) penalty = 0u - std::uint32_t(penalty);
    }
    c.set(6, extended(c.i16(actor.address + 0x2a))); c.set(3, 0x66666667); c.set(4, 0x66666667);
    const auto halved = shifted(std::int32_t(c.r(6) + (std::int32_t(c.r(6)) < 0)), 1);
    c.set(5, std::uint32_t(halved) + std::uint32_t(bonus) - std::uint32_t(penalty) + 210); c.put16(actor.address + 0x3a, c.r(5));
    c.set(5, std::uint32_t(high(std::uint32_t(fitness), std::int32_t(c.r(3)))));
    const auto live_bias = c.i16(actor.address + 0x1a0); c.set(6, c.u8(action)); c.set(3, c.u32(cursor));
    c.set(4, std::uint32_t(high(std::uint32_t(live_bias), std::int32_t(c.r(4))))); c.set(7, extended(std::int8_t(c.r(6))));
    c.set(6, extended(c.i16(actor.address + 0x2a))); c.set(3, c.r(3) + 2); c.set(8, c.r(7) * 2);
    const auto limit = c.u32(end); c.set(7, std::uint32_t(shifted(std::int32_t(c.r(5)), 1)));
    c.set(5, actor.address + c.r(8)); c.set(9, extended(c.i16(c.r(5) + 0x11e)));
    c.set(4, std::uint32_t(shifted(std::int32_t(c.r(4)), 1))); c.set(5, std::int32_t(c.r(4)) < 0);
    const auto order = compare_logical(c, c.r(3), limit); c.set(8, std::int32_t(c.r(7)) < 0);
    c.set(4, c.r(4) + c.r(5)); c.put32(cursor, c.r(3)); c.set(5, c.r(7) + c.r(8));
    score = extended(std::int16_t(c.r(9) + c.r(6) + c.r(4) - c.r(5)));
    if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, 0x10254c30); c.put32(seed, c.r(3) & 0xffff); }
    c.set(5, c.u32(cursor)); c.set(4, 0x66666667); c.set(3, c.u32(cursor)); c.set(6, c.u16(c.r(5)));
    c.set(5, c.u32(seed)); c.set(3, c.r(3) + 2); const auto second_limit = c.u32(end);
    c.set(5, c.r(6) + c.r(5)); c.put32(cursor, c.r(3)); c.set(4, std::uint32_t(high(c.r(5), std::int32_t(c.r(4)))));
    const auto second_order = compare_logical(c, c.r(3), second_limit); partial = shifted(std::int32_t(c.r(4)), 2); c.set(3, partial < 0);
    penalty = std::uint32_t(std::int32_t(c.r(5)) % 10);
    if (second_order > 0) { c.put32(cursor, begin); c.call(0x1077b770, 0x10254c88); c.put32(seed, c.r(3) & 0xffff); }
    c.set(5, c.u32(cursor)); c.set(3, 0x66666667); c.set(4, c.u32(seed)); c.set(5, c.u16(c.r(5)));
    const auto total = std::uint32_t(score) + std::uint32_t(penalty); c.set(5, c.r(5) + c.r(4));
    partial = shifted(high(c.r(5), std::int32_t(c.r(3))), 2); c.set(4, partial < 0);
    c.set(3, std::uint32_t(std::int32_t(c.r(5)) % 10)); score = extended(std::int16_t(total - c.r(3)));
}

void ShotSelection::ordinary_rating() {
    draw_fitness(false);
    const auto kind = c.u8(action); Kept<18> condition(c, extended(std::int16_t(c.r(13))));
    if (compare(c, kind, 53) == 0) c.put16(actor.address + 0x3a, 0);
    else {
        c.set(4, extended(c.i16(actor.address + 0x2a))); const auto bias = c.i16(actor.address + 0x1a0);
        c.set(3, std::uint32_t(shifted(std::int32_t(c.r(4) + (std::int32_t(c.r(4)) < 0)), 1)));
        c.put16(actor.address + 0x3a, std::uint32_t(bias) - std::uint32_t(condition) + c.r(3));
    }
    const auto live_kind = c.i8(action); Kept<20> penalty(c, extended(c.i16(actor.address + 0x1a0)));
    const auto bonus_bound = std::int32_t(std::uint32_t(penalty)); const auto zero = compare(c, bonus_bound) == 0;
    c.set(3, actor.address + std::uint32_t(int(live_kind) * 2)); Kept<22> base(c, extended(c.i16(c.r(3) + 0x11e)));
    Kept<19> bonus(c, c.r(19));
    if (zero) bonus = 0;
    else {
        advance(0x10255124); c.set(3, 65535); compare(c, bonus_bound, 65535);
        c.set(3, std::uint32_t(-65535)); compare(c, bonus_bound, -65535);
        c.set(3, c.u32(cursor)); compare(c, bonus_bound); const auto bias = c.u32(seed);
        c.set(3, std::uint32_t(c.u16(c.r(3))) + bias);
        const auto sampled = remainder(c.r(3), bonus_bound); bonus = std::uint32_t(bonus_bound > 0 ? sampled : -sampled);
    }
    const auto penalty_bound = std::int16_t(std::uint32_t(condition));
    if (compare(c, penalty_bound) == 0) penalty = 0;
    else {
        advance(0x10255290); c.set(3, 65535); compare(c, penalty_bound, 65535);
        c.set(3, std::uint32_t(-65535)); compare(c, penalty_bound, -65535);
        c.set(5, c.u32(cursor)); c.set(3, extended(penalty_bound)); c.set(4, c.u32(seed)); compare(c, penalty_bound);
        c.set(4, std::uint32_t(c.u16(c.r(5))) + c.r(4));
        const auto sampled = remainder(c.r(4), penalty_bound); penalty = std::uint32_t(penalty_bound > 0 ? sampled : -sampled);
    }
}

void ShotSelection::diagnose_selection() {
    c.set(5, extended(c.i8(action))); c.set(4, c.r(14) + 0xe6); c.set(3, c.toc(-0x7bb8)); c.call(0x1077b720, 0x10254310);
    c.set(3, c.r(14) + 0x8f); c.set(6, at(0x40)); c.set(7, at(0x38)); c.set(4, 0); c.set(5, 0); c.call(0x10739890, 0x10254328);
    c.call(0x10768810, 0x1025432c); c.set(5, c.r(3)); c.set(3, c.r(13)); c.set(4, c.r(14) + 0x9c);
    c.set(6, at(0x40)); c.set(7, at(0x38)); c.set(8, 0x2c38); c.call(0x1077b720, 0x10254348);
    c.set(4, c.r(13)); c.set(3, c.r(14) + 0xa9); c.set(5, 0); c.call(0x10769b60, 0x10254358);
    c.set(3, c.u32(cursor)); c.set(5, 0); const auto limit = c.u32(end); c.set(3, c.r(3) + 2); c.set(4, c.toc(-0x7bac));
    const auto order = compare_logical(c, c.r(3), limit); c.put32(cursor, c.r(3)); c.put8(c.r(4), c.r(5));
    if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, 0x10254384); c.put32(seed, c.r(3) & 0xffff); }
    c.set(5, c.u32(cursor)); c.set(3, 0x51eb851f); c.set(4, c.u32(seed)); c.set(5, c.u16(c.r(5)));
    c.set(4, c.r(5) + c.r(4)); c.put16(actor.address + 0x3a, 0);
    const auto partial = shifted(high(c.r(4), std::int32_t(c.r(3))), 4); c.set(3, partial < 0);
    score = extended(std::int16_t(std::int32_t(c.r(4)) % 50));
}

void ShotSelection::sample_context_bound(std::uint32_t refill_back, std::uint32_t recursive_back) {
    Kept<14> bound(c, c.r(14)); Kept<13> draw(c, c.r(13));
    const auto signed_bound = std::int32_t(std::uint32_t(bound));
    if (compare(c, signed_bound) == 0) { draw = 0; return; }
    advance(refill_back); c.set(3, 65535);
    bool narrow = compare(c, signed_bound, 65535) <= 0;
    if (narrow) { c.set(3, std::uint32_t(-65535)); narrow = compare(c, signed_bound, -65535) >= 0; }
    if (narrow) {
        c.set(3, c.u32(cursor)); compare(c, signed_bound); const auto bias = c.u32(seed);
        c.set(3, std::uint32_t(c.u16(c.r(3))) + bias);
        const auto sampled = remainder(c.r(3), signed_bound);
        draw = signed_bound > 0 ? std::uint32_t(sampled) : 0u - std::uint32_t(sampled); return;
    }
    c.set(3, std::uint32_t(bound) ^ 0x80000000); c.put32(at(0x64), c.r(3)); c.set(6, 0x80010000);
    c.set(7, c.u32(cursor)); c.set(4, 0x43300000); c.put32(at(0x60), 0x43300000); c.set(5, 0x10000);
    c.set_f(2, c.f64(doubles + 0x80)); c.set(3, 0x80010000); c.set_f(1, c.f64(at(0x60)));
    c.set(6, 0x80008001); c.set_f(0, c.f64(doubles)); c.set(5, 65535); c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set(8, c.u16(c.r(7))); c.set(7, c.u32(seed)); const auto numerator = std::uint32_t(bound) * 10;
    c.set(3, 0x80008001); c.put32(at(0x68), c.r(4)); c.set_f(2, fp.fdiv(c.f(1), c.f(0)));
    c.set(7, c.r(8) + c.r(7)); c.set(4, std::uint32_t(high(c.r(7), std::int32_t(c.r(6)))));
    c.set_f(1, c.f64(doubles + 0x80));
    auto partial = shifted(std::int32_t(c.r(4) + c.r(7)), 15); c.set(4, std::uint32_t(partial)); c.set(6, partial < 0);
    c.set(4, (c.r(4) + c.r(6)) * c.r(5)); draw = c.r(7) - c.r(4);
    c.set(4, std::uint32_t(draw) ^ 0x80000000); c.put32(at(0x6c), c.r(4));
    c.set(3, std::uint32_t(high(numerator, std::int32_t(c.r(3))))); c.set_f(0, c.f64(at(0x68)));
    c.set_f(0, fp.fsub(c.f(0), c.f(1))); partial = shifted(std::int32_t(c.r(3) + numerator), 15);
    c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.set(3, partial < 0); c.set(3, std::uint32_t(partial) + c.r(3));
    c.put64(at(0x70), fp.fctiwz(0, c.f(0))); draw = c.u32(at(0x74));
    enter(c, recursive_back); sample_range(c, c.r(3));
    c.set(4, 0x66666667); const auto correction = c.r(3) + 5;
    c.set(3, std::uint32_t(shifted(high(correction, std::int32_t(c.r(4))), 2))); c.set(4, std::int32_t(c.r(3)) < 0);
    c.set(3, c.r(3) + c.r(4)); draw = std::uint32_t(draw) + c.r(3);
    if (compare(c, std::int32_t(std::uint32_t(draw)), std::int32_t(std::uint32_t(bound) - 1)) > 0) draw = std::uint32_t(bound) - 1;
}

void ShotSelection::low_context_attributes(bool fourth_mode) {
    c.set(3, extended(c.i16(actor.address + 0x3a))); c.set_f(0, c.f32(floats + 0x94));
    c.put16(actor.address + 0x3a, c.r(3) - (fourth_mode ? 300 : 250)); c.set_f(1, c.f32(actor.address + 0x7c));
    const std::array<unsigned, 3> attributes{0x7c, 0x84, 0xe8};
    const std::array<std::uint32_t, 3> fourth_sites{0x10255438, 0x102555d8, 0x10255778};
    const std::array<std::uint32_t, 3> third_sites{0x1025595c, 0x10255afc, 0x10255c9c};
    for (unsigned index = 0; index < attributes.size(); ++index) {
        if (index) { c.set_f(1, c.f32(actor.address + attributes[index])); c.set_f(0, c.f32(floats + 0x94)); }
        fp.fcmp(0, c.f(1), c.f(0));
        if (!(c.cr(0) & 8)) continue;
        const auto site = fourth_mode ? fourth_sites[index] : third_sites[index];
        c.set_f(2, c.f32(floats + 0x104)); c.set_f(1, c.f32(actor.address + attributes[index])); c.set_f(0, c.f32(floats + 0xd0));
        c.set_f(0, fp.multiply_add(site, c.f(2), c.f(1), c.f(0))); c.put64(at(0x60), fp.fctiwz(0, c.f(0)));
        c.set(14, c.u32(at(0x64))); sample_context_bound(site + 0x40, site + 0x118);
        score = std::uint32_t(score) + extended(std::int16_t(c.r(13)));
    }
    const auto scaled_trait = std::uint32_t(int(c.i8(actor.address + 0x10c)) * (fourth_mode ? 30 : 20));
    const auto base = fourth_mode ? 400u : 300u; c.s.wr<1>(reg::xer_ca, scaled_trait <= base);
    score = extended(std::int16_t(std::uint32_t(score) - (base - scaled_trait)));
}

void ShotSelection::context_rating(bool action_five) {
    bool fourth_mode = action_five;
    if (fourth_mode) { c.set(3, actor.match()); fourth_mode = compare(c, c.u8(c.r(3) + 0x791f), 4) == 0; }
    if (fourth_mode) low_context_attributes(true);
    else {
        bool third_mode = compare(c, c.u8(action), 5) == 0;
        if (third_mode) { c.set(3, actor.match()); third_mode = compare(c, c.u8(c.r(3) + 0x791f), 3) == 0; }
        if (third_mode) low_context_attributes(false);
        else if (compare(c, c.u8(action), 57) == 0) {
            c.set(3, actor.match()); c.put32(at(0x60), 0x43300000); const auto mode = c.i8(c.r(3) + 0x791f);
            c.set_f(3, c.f64(doubles + 0x228)); c.set_f(2, c.f32(actor.address + 0x7c));
            c.s.wr<1>(reg::xer_ca, std::uint32_t(mode) <= 3); const auto factor = std::uint32_t(3 - int(mode));
            c.set_f(0, c.f64(doubles + 0x38)); c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x64), factor ^ 0x80000000);
            c.set_f(2, fp.multiply_add(0x10255e78, c.f(3), c.f(2), c.f(0))); c.set_f(0, c.f64(at(0x60)));
            c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
            c.put64(at(0x68), fp.fctiwz(0, c.f(0))); c.set(14, c.u32(at(0x6c)));
            sample_context_bound(0x10255ec4, 0x10255f9c);
            const auto live = c.i16(actor.address + 0x3a); c.set(3, extended(std::int16_t(c.r(13))));
            score = std::uint32_t(score) - c.r(3); c.put16(actor.address + 0x3a, std::uint32_t(live) - c.r(3));
        } else {
            c.set(3, actor.match());
            if (compare(c, c.u8(c.r(3) + 0x791f), 3) == 0) {
                advance(0x10256048); c.set(5, c.u32(cursor)); c.set(3, 0x51eb851f); c.set(4, c.u32(seed)); c.set(5, c.u16(c.r(5)));
                const auto live = c.i16(actor.address + 0x3a); c.set(5, c.r(5) + c.r(4));
                const auto partial = shifted(high(c.r(5), std::int32_t(c.r(3))), 7); c.set(4, partial < 0);
                c.set(3, extended(std::int16_t(std::int32_t(c.r(5)) % 400)));
                c.put16(actor.address + 0x3a, std::uint32_t(live) - c.r(3)); score = std::uint32_t(score) - c.r(3);
            }
        }
    }
    const auto score_bound = std::int16_t(std::uint32_t(score)); Kept<18> sampled_score(c, c.r(18));
    if (compare(c, score_bound) == 0) sampled_score = 0;
    else {
        advance(0x102560c4); c.set(3, 65535); compare(c, score_bound, 65535);
        c.set(3, std::uint32_t(-65535)); compare(c, score_bound, -65535);
        c.set(5, c.u32(cursor)); c.set(3, extended(score_bound)); c.set(4, c.u32(seed)); compare(c, score_bound);
        c.set(4, std::uint32_t(c.u16(c.r(5))) + c.r(4)); const auto sampled = remainder(c.r(4), score_bound);
        sampled_score = std::uint32_t(score_bound > 0 ? sampled : -sampled);
    }
    Kept<13> placement(c, extended(c.i16(actor.address + 0x2a))); Kept<14> sampled_placement(c, c.r(14));
    const auto placement_bound = std::int32_t(std::uint32_t(placement));
    if (compare(c, placement_bound) == 0) sampled_placement = 0;
    else {
        advance(0x10256250); c.set(3, 65535); compare(c, placement_bound, 65535);
        c.set(3, std::uint32_t(-65535)); compare(c, placement_bound, -65535);
        c.set(3, c.u32(cursor)); compare(c, placement_bound); const auto bias = c.u32(seed);
        c.set(3, std::uint32_t(c.u16(c.r(3))) + bias); const auto sampled = remainder(c.r(3), placement_bound);
        sampled_placement = std::uint32_t(placement_bound > 0 ? sampled : -sampled);
    }
    score = extended(std::int16_t(std::uint32_t(sampled_score) + std::uint32_t(sampled_placement)));
}

void ShotSelection::lateral_penalty() {
    bool central = compare(c, actor.column(), 3) >= 0;
    if (central) central = compare(c, actor.column(), 5) <= 0;
    if (central) return;
    auto side = actor.side(); c.set_f(3, c.f32(floats));
    const bool upper = compare(c, std::int8_t(side)) == 0;
    bool calculate = upper;
    if (!upper) { side = actor.side(); calculate = compare(c, side, 1) == 0; }
    if (calculate) {
        c.set(5, c.u8(actor.address + 0x105)); c.set(4, 0x43300000); c.set(3, c.u8(actor.address + 0x106));
        c.set(5, extended(std::int8_t(c.r(5)))); c.put32(at(0x60), c.r(4));
        c.s.wr<1>(reg::xer_ca, c.r(5) <= 4); c.set(4, 4u - c.r(5)); c.set(3, extended(std::int8_t(c.r(3))));
        c.set(4, c.r(4) ^ 0x80000000); c.set_f(2, c.f64(doubles + 0x80)); c.put32(at(0x64), c.r(4));
        if (upper) { c.s.wr<1>(reg::xer_ca, c.r(3) <= 12); c.set(3, 12u - c.r(3)); }
        else c.set(3, c.r(3) + 1);
        c.set(3, c.r(3) ^ 0x80000000); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, c.f64(at(0x60)));
        c.put32(at(0x6c), c.r(3)); c.set_f(0, fp.fsub(c.f(0), c.f(2))); c.put32(at(0x68), 0x43300000);
        c.set_f(2, std::abs(c.f(0))); c.set_f(0, c.f64(at(0x68))); c.set_f(1, fp.fsubs(c.f(0), c.f(1)));
        c.set_f(0, fp.frsp(c.f(2))); c.set_f(3, fp.fdivs(c.f(1), c.f(0)));
    }
    c.set_f(3, fp.fmuls(c.f(3), c.f(3))); c.set_f(0, c.f64(doubles + 0x20)); fp.fcmp(0, c.f(3), c.f(0));
    if (c.cr(0) & 8) c.set(15, 1);
    c.set_f(1, c.f32(floats + 0x10c)); c.set_f(0, c.f32(floats + 0x108)); c.set_f(2, c.f32(floats + 0xb4));
    c.set_f(0, fp.multiply_add(0x102564ac, c.f(1), c.f(3), c.f(0))); fp.fcmp(0, c.f(2), c.f(0));
    if (c.cr(0) & 4) c.set_f(0, c.f32(floats + 0xb4));
    else {
        c.set_f(1, c.f32(floats + 0x10c)); c.set_f(0, c.f32(floats + 0x108));
        c.set_f(0, fp.multiply_add(0x102564c8, c.f(1), c.f(3), c.f(0)));
    }
    const auto converted = fp.fctiwz(0, c.f(0)); const auto kind = c.u8(action);
    const bool exceptional = compare(c, kind, 58) == 0; c.put64(at(0x60), converted); Kept<13> base(c, c.u32(at(0x64)));
    if (exceptional) {
        c.set(3, 0x55555556); c.set(3, std::uint32_t(high(extended(std::int16_t(std::uint32_t(base))), std::int32_t(c.r(3)))));
        base = extended(std::int16_t(c.r(3) + (std::int32_t(c.r(3)) < 0)));
    }
    c.set_f(1, c.f64(doubles + 0x10)); c.set_f(0, c.f32(actor.address + 0xe8)); fp.fcmp(0, c.f(1), c.f(0));
    c.set_f(30, c.cr(0) & 4 ? c.f64(doubles + 0x10) : c.f32(actor.address + 0xe8));
    c.set(3, extended(std::int16_t(std::uint32_t(base))) ^ 0x80000000); c.put32(at(0x68), 0x43300000);
    c.set_f(2, c.f64(doubles + 0x80)); c.put32(at(0x6c), c.r(3)); c.set_f(3, c.f64(doubles + 0x18));
    c.set_f(1, c.f64(at(0x68))); c.set_f(0, c.f64(doubles + 0x88)); c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(1, fp.fmul(c.f(3), c.f(1))); c.set_f(1, fp.fdiv(c.f(1), c.f(30))); fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 2) c.set(14, 0);
    else sample_lateral_penalty();
    c.set(4, extended(std::int16_t(std::uint32_t(base)))); c.set(5, extended(std::int16_t(c.r(14)))); c.set(3, std::int32_t(c.r(4)) < 0);
    const auto previous = c.i16(actor.address + 0x3a); c.set(3, c.r(3) + c.r(4)); score = std::uint32_t(score) - c.r(5);
    c.set(3, extended(std::int16_t(shifted(std::int32_t(c.r(3)), 1)))); c.put16(actor.address + 0x3a, std::uint32_t(previous) - c.r(3));
}

void ShotSelection::sample_lateral_penalty() {
    Kept<13> base(c, c.r(13)); Kept<14> draw(c, c.r(14));
    advance(0x1025657c);
    const auto check_bound = [&](std::int32_t limit) {
        c.set(3, extended(std::int16_t(std::uint32_t(base)))); c.set(4, c.r(3) ^ 0x80000000); c.put32(at(0x60), 0x43300000);
        c.set(3, std::uint32_t(limit)); c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x64), c.r(4));
        c.set_f(2, c.f64(doubles + 0x18)); c.set_f(0, c.f64(at(0x60))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
        c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.set_f(0, fp.fdiv(c.f(0), c.f(30)));
        c.put64(at(0x68), fp.fctiwz(0, c.f(0))); return compare(c, c.i32(at(0x6c)), limit);
    };
    bool narrow = check_bound(65535) <= 0;
    if (narrow) narrow = check_bound(-65535) >= 0;
    if (narrow) {
        c.set(4, 0x43300000); c.set(3, extended(std::int16_t(std::uint32_t(base))) ^ 0x80000000);
        c.put32(at(0x64), c.r(3)); c.set(3, extended(std::int16_t(std::uint32_t(base))) ^ 0x80000000);
        c.set(5, c.u32(cursor)); c.put32(at(0x60), c.r(4)); c.set_f(1, c.f64(doubles + 0x80));
        c.set_f(0, c.f64(at(0x60))); c.set_f(2, c.f64(doubles + 0x18)); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
        c.put32(at(0x74), c.r(3)); c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x70), 0x43300000);
        c.set_f(3, fp.fmul(c.f(2), c.f(0))); c.set_f(2, c.f64(doubles + 0x18)); c.set_f(0, c.f64(at(0x70)));
        c.set(3, c.u16(c.r(5))); c.set_f(0, fp.fsub(c.f(0), c.f(1))); const auto bias = c.u32(seed);
        c.set_f(1, fp.fdiv(c.f(3), c.f(30))); c.set(4, c.r(3) + bias); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
        const auto denominator = fp.fctiwz(1, c.f(1)); c.set_f(0, fp.fdiv(c.f(0), c.f(30))); c.put64(at(0x68), denominator);
        c.set(3, c.u32(at(0x6c)));
        // The floating guard precedes integer conversion. The original
        // lowers a zero denominator here to a dead zero quotient.
        const auto quotient = c.r(3) ? std::uint32_t(divide_signed<4>(c.r(4), c.r(3))) : 0;
        draw = c.r(4) - quotient * c.r(3);
        const auto sign_bits = fp.fctiwz(0, c.f(0)); c.set(3, 0u - std::uint32_t(draw)); c.put64(at(0x78), sign_bits);
        if (compare(c, c.i32(at(0x7c))) > 0) c.set(3, draw);
        draw = c.r(3); return;
    }
    c.set(4, 0x43300000); c.set(3, extended(std::int16_t(std::uint32_t(base))) ^ 0x80000000); c.put32(at(0x60), c.r(4));
    c.set(5, 0x43300000); c.set(6, 0x43300000); c.put32(at(0x64), c.r(3)); c.set_f(1, c.f64(doubles + 0x80));
    c.set(3, extended(std::int16_t(std::uint32_t(base))) ^ 0x80000000); c.set_f(0, c.f64(at(0x60))); c.put32(at(0x88), 0x43300000);
    c.set(8, 0x80010000); c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(2, c.f64(doubles + 0x18));
    c.set(7, 0x10000); c.set(4, 0x80010000); c.put32(at(0x8c), c.r(3)); c.set(8, 0x80008001);
    c.set_f(3, fp.fmul(c.f(2), c.f(0))); c.set(9, c.u32(cursor)); c.set_f(1, c.f64(doubles + 0x80));
    c.set(7, 65535); c.set_f(0, c.f64(at(0x88))); c.set(4, 0x80008001); c.set_f(5, fp.fdiv(c.f(3), c.f(30)));
    c.set_f(2, c.f64(doubles + 0x18)); c.set(3, c.u16(c.r(9))); const auto bias = c.u32(seed); c.put32(at(0x78), c.r(5)); c.set(5, c.r(3) + bias);
    c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set(3, std::uint32_t(high(c.r(5), std::int32_t(c.r(8)))));
    const auto first_bits = fp.fctiwz(5, c.f(5)); c.put32(at(0x70), c.r(6)); c.set_f(4, c.f64(doubles + 0x80));
    c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.put64(at(0x68), first_bits); const auto first_bound = c.u32(at(0x6c));
    c.set(3, c.r(3) + c.r(5)); c.set_f(0, fp.fdiv(c.f(0), c.f(30))); c.set_f(3, c.f64(doubles));
    c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x74), first_bound ^ 0x80000000);
    const auto partial = shifted(std::int32_t(c.r(3)), 15); c.set_f(2, c.f64(at(0x70))); c.set(3, partial < 0);
    const auto second_bits = fp.fctiwz(0, c.f(0)); c.set_f(2, fp.fsub(c.f(2), c.f(4)));
    const auto product = (std::uint32_t(partial) + c.r(3)) * c.r(7); c.put64(at(0x90), second_bits); c.set_f(2, fp.fdiv(c.f(2), c.f(3)));
    draw = c.r(5) - product; const auto numerator = c.u32(at(0x94)) * 10; c.set(3, std::uint32_t(draw) ^ 0x80000000); c.put32(at(0x7c), c.r(3));
    c.set_f(0, c.f64(at(0x78))); c.set(3, std::uint32_t(high(numerator, std::int32_t(c.r(4)))));
    c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    const auto quotient = shifted(std::int32_t(c.r(3) + numerator), 15); c.set(3, std::uint32_t(quotient) + (quotient < 0));
    c.put64(at(0x80), fp.fctiwz(0, c.f(0))); draw = c.u32(at(0x84)); enter(c, 0x10256734); sample_range(c, c.r(3));
    c.set(4, extended(std::int16_t(std::uint32_t(base))) ^ 0x80000000); c.put32(at(0x98), 0x43300000); c.set_f(1, c.f64(doubles + 0x80));
    const auto correction = c.r(3) + 5; c.set(3, correction); c.put32(at(0x9c), c.r(4)); c.set(4, 0x66666667);
    c.set_f(2, c.f64(doubles + 0x18)); c.set_f(0, c.f64(at(0x98))); const auto correction_high = high(correction, std::int32_t(c.r(4)));
    c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    const auto correction_partial = shifted(correction_high, 2); c.set(3, correction_partial < 0);
    draw = std::uint32_t(draw) + std::uint32_t(correction_partial) + c.r(3); c.set_f(0, fp.fdiv(c.f(0), c.f(30)));
    c.put64(at(0xa0), fp.fctiwz(0, c.f(0))); c.set(3, c.u32(at(0xa4)));
    if (compare(c, std::int32_t(std::uint32_t(draw)), std::int32_t(c.r(3) - 1)) > 0) {
        c.set(3, extended(std::int16_t(std::uint32_t(base))) ^ 0x80000000); c.put32(at(0xa8), 0x43300000);
        c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0xac), c.r(3)); c.set_f(2, c.f64(doubles + 0x18));
        c.set_f(0, c.f64(at(0xa8))); c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
        c.set_f(0, fp.fdiv(c.f(0), c.f(30))); c.put64(at(0xb0), fp.fctiwz(0, c.f(0))); c.set(3, c.u32(at(0xb4))); draw = c.r(3) - 1;
    }
}

void ShotSelection::mark_long_shot() {
    c.set_f(0, c.f64(doubles + 0x340)); fp.fcmp(0, c.f(29), c.f(0));
    if (!(c.cr(0) & 4)) return;
    const bool upper = compare(c, std::int8_t(actor.side()) == 0, 1) == 0;
    const auto column = actor.column(); c.set(3, 0);
    bool area = compare(c, column, 2) >= 0;
    if (area) area = compare(c, actor.column(), 6) <= 0;
    if (area) area = compare(c, actor.row(), upper ? 10 : 0) >= 0;
    if (area) area = compare(c, actor.row(), upper ? 11 : 1) <= 0;
    if (area) c.set(3, 1);
    if (compare(c, std::int8_t(c.r(3))) == 0) c.set(16, 1);
}

void ShotSelection::paired_match_bias(unsigned kind) {
    const auto first_back = kind == 2 ? 0x10256a04u : kind == 3 ? 0x10256ab4u : kind == 4 ? 0x10256b74u : 0x10256cc0u;
    const auto second_back = kind == 2 ? 0x10256a4cu : kind == 3 ? 0x10256b0cu : kind == 4 ? 0x10256bccu : 0x10256d18u;
    advance(first_back); Kept<13> first(c, c.r(13));
    int order;
    if (kind == 2) {
        c.set(4, c.u32(cursor)); c.set(3, c.u32(cursor)); c.set(5, c.u16(c.r(4))); c.set(4, c.u32(seed));
        c.set(3, c.r(3) + 2); const auto limit = c.u32(end); c.set(5, c.r(5) + c.r(4)); c.put32(cursor, c.r(3));
        order = compare_logical(c, c.r(3), limit); c.set(4, std::int32_t(c.r(5)) < 0); first = std::uint32_t(std::int32_t(c.r(5)) % 2);
    } else {
        c.set(5, c.u32(cursor)); c.set(4, 0x66666667); c.set(3, c.u32(cursor)); c.set(6, c.u16(c.r(5))); c.set(5, c.u32(seed));
        c.set(3, c.r(3) + 2); const auto limit = c.u32(end); c.set(5, c.r(6) + c.r(5)); c.put32(cursor, c.r(3));
        c.set(4, std::uint32_t(high(c.r(5), std::int32_t(c.r(4))))); order = compare_logical(c, c.r(3), limit);
        const auto partial = shifted(std::int32_t(c.r(4)), kind == 4 ? 2 : 1); c.set(3, partial < 0);
        first = std::uint32_t(std::int32_t(c.r(5)) % (kind == 4 ? 10 : 5));
    }
    if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, second_back); c.put32(seed, c.r(3) & 0xffff); }
    c.set(4, c.u32(cursor)); c.set(3, kind == 4 ? 0x88888889 : 0x66666667); const auto bias = c.u32(seed);
    c.set(4, std::uint32_t(c.u16(c.r(4))) + bias);
    auto product = std::uint32_t(high(c.r(4), std::int32_t(c.r(3)))); if (kind == 4) product += c.r(4);
    const auto partial = shifted(std::int32_t(product), kind == 4 ? 3 : kind == 3 ? 2 : 1); c.set(3, partial < 0);
    const auto second = std::int32_t(c.r(4)) % (kind == 4 ? 15 : kind == 3 ? 10 : 5);
    score = std::uint32_t(score) + extended(std::int16_t(std::uint32_t(first) - std::uint32_t(second)));
}

void ShotSelection::match_bias() {
    if (compare(c, std::int8_t(c.r(16))) == 0) return;
    if (compare_logical(c, actor.match()) == 0) return;
    bool family = compare(c, c.u8(action), 23) == 0;
    if (!family) family = compare(c, c.u8(action), 26) == 0;
    if (!family) family = compare(c, c.u8(action), 27) == 0;
    if (!family) family = compare(c, c.u8(action), 29) == 0;
    if (family) {
        c.set(3, actor.match()); const auto mode = c.i8(c.r(3) + 0x30d8);
        const auto order = compare(c, mode, 3);
        if (order == 0) paired_match_bias(3);
        else if (order < 0) { if (compare(c, mode, 1) > 0) paired_match_bias(2); }
        else if (compare(c, mode, 5) < 0) paired_match_bias(4);
        return;
    }
    if (compare(c, c.u8(action), 22) != 0) return;
    c.set(3, actor.match()); const auto mode = c.i8(c.r(3) + 1); const auto order = compare(c, mode, 32);
    if (order == 0) { paired_match_bias(5); return; }
    if (order > 0 || compare(c, mode, 2) != 0) return;
    advance(0x10256c68); c.set(3, c.u32(cursor)); const auto bias = c.u32(seed);
    const auto word = std::uint32_t(c.u16(c.r(3))) + bias; c.set(3, std::int32_t(word) < 0);
    score = std::uint32_t(score) + extended(std::int16_t(std::int32_t(word) % 4));
}

void ShotSelection::keeper_adjustment() {
    enter(c, 0x10256d64, {keeper}); current_role(c, keeper);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) return;
    advance(0x10256d90); c.set(5, c.u32(cursor)); c.set(4, 0x10624dd3); c.set(3, extended(c.i16(actor.address + 0x3a)));
    c.set(6, c.u16(c.r(5))); c.set(5, c.u32(seed)); const auto previous = c.r(3);
    c.set(5, c.r(6) + c.r(5)); c.put16(actor.address + 0x3a, previous + 250);
    const auto partial = shifted(high(c.r(5), std::int32_t(c.r(4))), 5); c.set(3, partial < 0);
    score = std::uint32_t(score) + extended(std::int16_t(std::int32_t(c.r(5)) % 500));
}

void ShotSelection::distance_penalty() {
    bool special = compare(c, c.u8(action), 3) == 0;
    if (!special) special = compare(c, c.u8(action), 57) == 0;
    if (special) {
        c.set_f(0, c.f64(doubles + 0x10)); fp.fcmp(0, c.f(29), c.f(0));
        c.set_cr(0, (c.cr(0) & ~2u) | ((c.cr(0) & 6) ? 2 : 0)); special = (c.cr(0) & 2) != 0;
        if (special) {
            bool goal_row = compare(c, actor.row()) == 0;
            if (!goal_row) goal_row = compare(c, c.u8(actor.address + 0x106), 11) == 0;
            if (goal_row) {
                c.set(3, std::uint32_t(int(actor.column()) - 4)); c.call(0x10775540, 0x10256e2c);
                special = compare(c, std::int32_t(c.r(3)), 1) != 0;
            }
        }
    }
    if (special) {
        c.set_f(0, c.f64(doubles + 0xc8)); c.set_f(2, c.f64(doubles + 0x348)); c.set_f(1, fp.fadd(c.f(0), c.f(29)));
        c.call(0x10780fc0, 0x10256e44); c.set(2, c.u32(at(0x14))); c.set_f(0, c.f64(doubles + 8));
        c.set_f(0, fp.fdiv(c.f(1), c.f(0))); c.put64(at(0x60), fp.fctiwz(0, c.f(0))); Kept<14> bound(c, c.u32(at(0x64)));
        if (compare(c, std::int32_t(std::uint32_t(bound)), 15000) > 0) bound = 15000;
        bound = extended(std::int16_t(std::uint32_t(bound)));
        if (compare(c, std::int32_t(std::uint32_t(bound))) < 0) bound = 15000;
        if (compare(c, c.u8(action), 58) == 0) {
            const auto value = std::int16_t(std::uint32_t(bound)); shifted(value, 3); bound = extended(std::int16_t(value / 8));
        }
        Kept<13> draw(c, c.r(13)); const auto signed_bound = std::int16_t(std::uint32_t(bound));
        if (compare(c, signed_bound) == 0) draw = 0;
        else {
            advance(0x10256ec0); c.set(3, 65535); compare(c, signed_bound, 65535);
            c.set(3, std::uint32_t(-65535)); compare(c, signed_bound, -65535);
            c.set(5, c.u32(cursor)); c.set(3, extended(signed_bound)); c.set(4, c.u32(seed)); compare(c, signed_bound);
            c.set(4, std::uint32_t(c.u16(c.r(5))) + c.r(4)); const auto sampled = remainder(c.r(4), signed_bound);
            // The preceding clamp makes this nonzero halfword positive.
            draw = std::uint32_t(sampled);
        }
        c.set(4, extended(std::int16_t(std::uint32_t(bound)))); c.set(5, extended(std::int16_t(std::uint32_t(draw)))); c.set(3, std::int32_t(c.r(4)) < 0);
        const auto previous = c.i16(actor.address + 0x3a); c.set(3, c.r(3) + c.r(4)); score = std::uint32_t(score) - c.r(5);
        c.set(3, extended(std::int16_t(shifted(std::int32_t(c.r(3)), 1)))); c.put16(actor.address + 0x3a, std::uint32_t(previous) - c.r(3)); return;
    }
    c.set_f(0, c.f32(floats)); fp.fcmp(0, c.f(0), c.f(29));
    if (c.cr(0) & 2) {
        c.set(3, extended(c.i16(actor.address + 0x3a))); score = extended(std::int16_t(std::uint32_t(score) + 200));
        c.put16(actor.address + 0x3a, c.r(3) + 200); return;
    }
    c.set_f(0, c.f64(doubles + 0x10)); fp.fcmp(0, c.f(0), c.f(29));
    if (c.cr(0) & 2) {
        c.set(3, extended(c.i16(actor.address + 0x3a))); score = extended(std::int16_t(std::uint32_t(score) + 100));
        c.put16(actor.address + 0x3a, c.r(3) + 100); return;
    }
    c.set_f(0, c.f64(doubles + 0x90)); fp.fcmp(0, c.f(29), c.f(0));
    if (c.cr(0) & 4) long_distance_penalty();
}

void ShotSelection::long_distance_penalty() {
    c.set_f(1, c.f64(doubles + 0x150)); c.set_f(0, c.f64(doubles + 0x90)); c.set_f(2, c.f64(doubles + 0x70));
    c.set_f(1, fp.multiply_add(0x102570a4, c.f(1), c.f(29), c.f(0))); c.call(0x10780fc0, 0x102570ac); c.set(2, c.u32(at(0x14)));
    c.put64(at(0x60), fp.fctiwz(0, c.f(1))); Kept<14> bound(c, c.u32(at(0x64)));
    if (compare(c, std::int32_t(std::uint32_t(bound)), 30000) > 0) bound = 30000;
    if (compare(c, c.u8(action), 58) == 0) {
        c.set(4, c.u8(actor.address + 0x10c)); c.put32(at(0x60), 0x43300000); c.set(3, 0x66660000);
        c.set_f(2, c.f64(doubles + 0x80)); c.set_f(3, c.f32(actor.address + 0xb4)); c.put32(at(0x64), extended(std::int8_t(c.r(4))) ^ 0x80000000);
        c.set(3, 0x66666667); const auto product = high(bound, std::int32_t(c.r(3))); c.set_f(1, c.f64(doubles + 0x2a0));
        c.set_f(0, c.f64(at(0x60))); c.set_f(4, c.f64(doubles + 0x10)); c.set_f(2, fp.fsubs(c.f(0), c.f(2)));
        c.set_f(0, c.f64(doubles + 0x2c0)); c.set_f(2, fp.fadds(c.f(3), c.f(2)));
        const auto partial = shifted(product, 3); c.set(3, partial < 0); bound = std::uint32_t(partial) + c.r(3);
        c.set_f(1, fp.fdiv(c.f(2), c.f(1))); c.set_f(2, fp.fsub(c.f(4), c.f(1))); fp.fcmp(0, c.f(2), c.f(0));
        if (c.cr(0) & 8) c.set_f(2, c.f64(doubles + 0x2c0));
    } else if (compare(c, c.u8(action), 53) == 0) {
        c.set_f(1, c.f32(actor.address + 0xa4)); c.set(3, 0x88890000); c.set_f(0, c.f64(doubles + 0x40)); c.set(3, 0x88888889);
        const auto product = std::uint32_t(high(bound, std::int32_t(c.r(3)))); c.set_f(2, c.f64(doubles + 0x10));
        c.set_f(1, fp.fdiv(c.f(1), c.f(0))); c.set_f(0, c.f64(doubles + 0x2c0));
        const auto partial = shifted(std::int32_t(product + std::uint32_t(bound)), 3); c.set(3, partial < 0); bound = std::uint32_t(partial) + c.r(3);
        c.set_f(2, fp.fsub(c.f(2), c.f(1))); fp.fcmp(0, c.f(2), c.f(0));
        if (c.cr(0) & 8) c.set_f(2, c.f64(doubles + 0x2c0));
    } else {
        c.set(4, c.u8(actor.address + 0x10c)); c.set(3, std::int32_t(std::uint32_t(bound)) < 0); c.put32(at(0x60), 0x43300000);
        c.set_f(2, c.f64(doubles + 0x80)); c.set(3, c.r(3) + std::uint32_t(bound)); c.put32(at(0x64), extended(std::int8_t(c.r(4))) ^ 0x80000000);
        bound = std::uint32_t(shifted(std::int32_t(c.r(3)), 1)); c.set_f(3, c.f32(actor.address + 0xb4)); c.set_f(0, c.f64(at(0x60)));
        c.set_f(1, c.f64(doubles + 0x2a0)); c.set_f(2, fp.fsubs(c.f(0), c.f(2))); c.set_f(4, c.f64(doubles + 0x10));
        c.set_f(0, c.f64(doubles + 0x2c0)); c.set_f(2, fp.fadds(c.f(3), c.f(2))); c.set_f(1, fp.fdiv(c.f(2), c.f(1)));
        c.set_f(2, fp.fsub(c.f(4), c.f(1))); fp.fcmp(0, c.f(2), c.f(0));
        if (c.cr(0) & 8) c.set_f(2, c.f64(doubles + 0x2c0));
    }
    c.set(3, std::uint32_t(bound) ^ 0x80000000); c.put32(at(0x64), c.r(3));
    c.set(3, std::uint32_t(bound) + (std::int32_t(std::uint32_t(bound)) < 0)); c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x60), 0x43300000);
    c.set(3, std::uint32_t(shifted(std::int32_t(c.r(3)), 1))); const auto previous = c.i16(actor.address + 0x3a);
    c.set(3, extended(std::int16_t(c.r(3)))); c.set_f(0, c.f64(at(0x60))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.put16(actor.address + 0x3a, std::uint32_t(previous) - c.r(3)); c.set_f(0, fp.fmul(c.f(0), c.f(2)));
    c.put64(at(0x68), fp.fctiwz(0, c.f(0))); bound = c.u32(at(0x6c));
    if (compare(c, std::int32_t(std::uint32_t(bound)), 30000) > 0) bound = 30000;
    sample_context_bound(0x10257268, 0x10257340); score = std::uint32_t(score) - extended(std::int16_t(c.r(13)));
}

std::uint32_t ShotSelection::write_event() {
    c.set(3, extended(c.i8(action))); const auto index = c.r(3) - 3;
    unsigned code = 0x1f81;
    if (compare_logical(c, index, 55) <= 0) {
        c.set(3, c.toc(-0x5228)); c.set(3, c.u32(c.r(3) + index * 4)); c.s.wr<4>(reg::ctr, c.r(3));
        const auto flagged = [&](unsigned lateral, unsigned distant, unsigned plain) {
            if (compare(c, std::int8_t(c.r(15))) != 0) return lateral;
            if (compare(c, std::int8_t(c.r(16))) != 0) return distant;
            return plain;
        };
        switch (c.r(3)) {
        case 0x102573d4: code = 0x1f9d; break;
        case 0x102573e0: code = flagged(0x1fb0, 0x1fa2, 0x1f86); break;
        case 0x10257414: code = flagged(0x1fb1, 0x1fa3, 0x1f89); break;
        case 0x10257448: code = flagged(0x1fb3, 0x1fa5, 0x1f8b); break;
        case 0x1025747c: code = flagged(0x1fb5, 0x1fa7, 0x1f8d); break;
        case 0x102574b0: code = flagged(0x1fb6, 0x1fa8, 0x1f8e); break;
        case 0x102574e4: code = flagged(0x1fb7, 0x1fa9, 0x1f91); break;
        case 0x10257518: code = flagged(0x1fb8, 0x1faa, 0x1f92); break;
        case 0x1025754c: code = flagged(0x1fb9, 0x1fab, 0x1f93); break;
        case 0x10257580: code = flagged(0x1fba, 0x1fac, 0x1f96); break;
        case 0x102575b4: code = flagged(0x1fbb, 0x1fad, 0x1f99); break;
        case 0x102575e8: code = flagged(0x1fbc, 0x1fae, 0x1f9c); break;
        case 0x1025761c:
            code = flagged(0x1fbd, 0x1faf, 0);
            if (!code) {
                advance(0x10257664); c.set(4, c.u32(cursor)); c.set(3, 0x55555556); const auto bias = c.u32(seed);
                c.set(4, std::uint32_t(c.u16(c.r(4))) + bias); c.set(3, std::uint32_t(high(c.r(4), std::int32_t(c.r(3)))));
                code = compare(c, std::int32_t(c.r(4)) % 3) == 0 ? 0x21bd : 0x1fbe;
            }
            break;
        case 0x102576b4: break;
        default: throw std::runtime_error("Unknown MATCH_PL shot event table target " + hex_address(c.r(3)));
        }
    }
    c.put16(event, code); return result(c, extended(std::int16_t(std::uint32_t(score))));
}

std::uint32_t ShotSelection::run() {
    measure_geometry();
    const auto kind = c.u8(action); c.set_f(29, c.f(0));
    if (compare(c, kind, 51) == 0) return special_shot();
    if (!select_action()) {
        c.set(3, 22); c.put8(action, c.r(3)); c.set(3, std::uint32_t(-1000)); c.put16(event, 0x1f81);
        return c.r(3);
    }
    c.s.pc = 0x102542ec;
    const auto selected = c.i8(action);
    if (compare(c, selected, -1) == 0) diagnose_selection();
    else if (compare(c, c.u8(action), 58) == 0) exceptional_rating();
    else {
        ordinary_rating();
        c.s.pc = 0x102553e8;
        const auto live_kind = c.u8(action); c.set(3, c.r(22) + c.r(19) - c.r(20));
        const auto action_five = compare(c, live_kind, 5) == 0; score = extended(std::int16_t(c.r(3)));
        context_rating(action_five);
    }
    c.s.pc = 0x10256394;
    lateral_penalty(); mark_long_shot(); match_bias(); keeper_adjustment(); distance_penalty();
    return write_event();
}
}

std::uint32_t choose_shot(Cpu& c, std::uint32_t player, std::uint32_t action, std::uint32_t event) {
    return ShotSelection(c, player, action, event).run();
}
}
