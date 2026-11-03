#include "match_pl_goal_line.hpp"

namespace cm::aot::match_pl {
using namespace guest;

GoalLineAction::GoalLineAction(Cpu& cpu, bool opposite_goal)
    : c(cpu), opposite(opposite_goal),
      frame(c, resume(0x1026a1b0, 0x1026d660), 0xd0, {.first_gpr = first_register(), .gpr_offset = opposite ? -0x48 : -0x4c, .first_fpr = 31}),
      fp(c), actor{c, c.r(3)} {
    c.set(0, c.lr());
    c.set_f(31, c.f(1));
    c.set({{31, actor.address}, {25, c.toc(-0x51e8)}, {28, 0}, {26, 0}, {27, c.toc(-0x7b90)},
           {first_register(), c.toc(-0x7ba8)}, {begin_register(), c.toc(-0x7b8c)},
           {end_register(), c.toc(-0x7b88)}, {29, c.toc(-0x7b84)},
           {strings_register(), c.toc(-0x51e4)}, {floats_register(), c.toc(-0x51e0)},
           {opposite ? 23u : 22u, 0}});
    if (!opposite) c.set(24, 0);
    c.set_f(0, c.f64(c.r(25) + 0x88));
    fp.fcmp(0, c.f(0), c.f(31));
    if (c.cr(0) & 2) c.set_f(31, c.f32(c.r(floats_register()) + 0x34));
}

void GoalLineAction::replace_seed(std::uint32_t back) {
    c.put32(c.r(29), c.r(begin_register()));
    c.call(0x1077b770, back);
    c.set(0, c.r(3) & 0xffff);
    c.put32(c.r(27), c.r(0));
}

std::int32_t GoalLineAction::divide_pool(std::uint32_t word, unsigned shift) {
    const auto high = std::int32_t((std::int64_t(std::int32_t(word)) * 0x10624dd3) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << shift) - 1)));
    return high >> shift;
}

void GoalLineAction::initial_draws() {
    c.set(4, actor.match());
    c.set(3, c.u32(c.r(29)));
    c.set(0, c.u32(c.r(end_register())));
    c.set(3, c.r(3) + 2);
    c.set(clock_register(), c.u8(c.r(4) + 0x7920));
    const bool first_wrap = compare_logical(c, c.r(3), c.r(0)) > 0;
    c.put32(c.r(29), c.r(3));
    if (first_wrap) replace_seed(resume(0x1026a230, 0x1026d6e4));
    c.set(5, c.u32(c.r(29)));
    c.set(4, 0x10620000);
    c.set(3, c.u32(c.r(29)));
    c.set(4, 0x10624dd3);
    c.set(6, c.u16(c.r(5)));
    c.set(5, c.u32(c.r(27)));
    c.set(3, c.r(3) + 2);
    c.set(0, c.u32(c.r(end_register())));
    c.set(5, c.r(6) + c.r(5));
    c.put32(c.r(29), c.r(3));
    const bool second_wrap = compare_logical(c, c.r(3), c.r(0)) > 0;
    c.set(4, std::uint32_t((std::int64_t(std::int32_t(c.r(5))) * 0x10624dd3) >> 32));
    const auto quotient = divide_pool(c.r(5), 6);
    c.set(3, unsigned(quotient < 0));
    c.set(0, std::uint32_t(quotient + (quotient < 0)) * 1000);
    c.set(30, c.r(5) - c.r(0));
    if (second_wrap) replace_seed(resume(0x1026a288, 0x1026d73c));
}

void GoalLineAction::threshold_and_keeper() {
    c.set(5, c.u32(c.r(29)));
    const auto seed = c.u32(c.r(27));
    const auto word = c.u16(c.r(5));
    c.set(5, std::uint32_t(std::int8_t(c.r(clock_register()))) + 1);
    const auto sum = word + seed;
    c.put32(frame.at(0x78), 0x43300000);
    c.set_f(1, c.f64(c.r(25) + 0x80));
    if (opposite) {
        c.set(4, std::int8_t(actor.side()) == 0);
        c.set(3, actor.match());
    } else {
        c.set(3, actor.match());
        c.set(4, actor.side());
    }
    const auto quotient = divide_pool(sum, 7);
    const auto scaled_quotient = std::uint32_t(quotient + (quotient < 0)) * 2000;
    c.set(6, opposite ? scaled_quotient : unsigned(quotient < 0));
    c.set(7, opposite ? unsigned(quotient < 0) : sum);
    const auto sample = sum - scaled_quotient;
    c.set(0, ((c.r(30) - sample) * c.r(5)) ^ 0x80000000);
    c.put32(frame.at(0x7c), c.r(0));
    c.set_f(0, c.f64(frame.at(0x78)));
    c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
    c.set_f(0, fp.fdivs(c.f(0), c.f(31)));
    c.put64(frame.at(0x70), fp.fctiwz(0, c.f(0)));
    c.set(clock_register(), c.u32(frame.at(0x74)));
    c.call(0x101f1e10, resume(0x1026a318, 0x1026d7bc));
    c.set(30, c.r(3));
}

void GoalLineAction::missing_keeper() {
    c.set({{3, c.r(strings_register()) + 0x8f}, {6, frame.at(0x48)}, {7, frame.at(0x40)}, {4, 0}, {5, 0}});
    c.call(0x10739890, resume(0x1026a338, 0x1026d7dc));
    c.call(0x10768810, resume(0x1026a33c, 0x1026d7e0));
    c.set({{5, c.r(3)}, {3, c.r(first_register())}, {4, c.r(strings_register()) + 0x9c},
           {6, frame.at(0x48)}, {7, frame.at(0x40)}, {8, resume(0x3ee5, 0x421f)}});
    c.call(0x1077b720, resume(0x1026a358, 0x1026d7fc));
    c.set({{4, c.r(first_register())}, {3, c.r(strings_register()) + 0xa9}, {5, 0}});
    c.call(0x10769b60, resume(0x1026a368, 0x1026d80c));
    c.set({{4, c.toc(-0x7bac)}, {0, 0}, {3, UINT32_MAX}});
    c.put8(c.r(4), 0);
    c.call(0x1076f7d0, resume(0x1026a37c, 0x1026d820));
}

void GoalLineAction::sample_keeper() {
    c.set(3, actor.match());
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x791f)));
    if (compare(c, std::int32_t(c.r(0)), 3) > 0) c.set(28, 1);
    const unsigned rating_register = opposite ? 20 : 26;
    if (opposite || compare_logical(c, c.r(30), actor.address) != 0) {
        c.set({{4, c.u8(actor.address + 0x105)}, {3, c.r(30)}, {5, c.u8(actor.address + 0x106)},
               {6, c.r(28)}, {8, frame.at(0x39)}, {9, frame.at(0x38)}, {7, UINT32_MAX}});
        c.call(0x10251270, resume(0x1026a3b4, 0x1026d860));
        c.set(rating_register, std::uint32_t(std::int16_t(c.r(3))));
    }
    // The lower-line self-keeper branch reads the original incoming stack byte.
    c.set(0, std::uint32_t(c.i8(frame.at(0x38))));
    if (compare(c, std::int32_t(c.r(0))) != 0) return;
    const auto rating = std::int32_t(std::int16_t(c.r(rating_register)));
    c.set(0, std::uint32_t(rating));
    if (compare(c, rating, 375) > 0) c.set(opposite ? 23 : 22, 1);
    else if (compare(c, rating, 260) > 0) c.set(opposite ? 26 : 24, 1);
}

bool GoalLineAction::phase_flag() {
    c.set(0, std::uint32_t(std::int8_t(c.r(28))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

bool GoalLineAction::phase_random(std::uint32_t back) {
    c.set(3, c.u32(c.r(29)));
    c.set(0, c.u32(c.r(end_register())));
    c.set(3, c.r(3) + 2);
    const bool wrap = compare_logical(c, c.r(3), c.r(0)) > 0;
    c.put32(c.r(29), c.r(3));
    if (wrap) replace_seed(back);
    c.set(3, c.u32(c.r(29)));
    c.set(0, c.u32(c.r(27)));
    const auto word = c.u16(c.r(3)) + c.r(0);
    c.set(3, word >> 31);
    c.set(0, std::uint32_t(std::int32_t(word) % 4));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

void GoalLineAction::choose_event() {
    const auto threshold = std::int32_t(std::int16_t(c.r(clock_register())));
    c.set(0, std::uint32_t(threshold));
    if (compare(c, threshold, 240) > 0) {
        c.set(28, 1);
        return;
    }
    if (compare(c, threshold, 10) < 0) {
        c.set(28, !phase_flag() ? 2 : phase_random(resume(0x1026a430, 0x1026d8dc)) ? 3 : 4);
        return;
    }
    const auto remaining = 240 - threshold;
    c.s.wr<1>(reg::xer_ca, std::uint32_t(threshold) <= 240);
    c.set(0, std::uint32_t(remaining));
    if (compare(c, remaining, 50) > 0)
        c.set(28, !phase_flag() ? 5 : phase_random(resume(0x1026a4b0, 0x1026d95c)) ? 6 : 7);
    else if (compare(c, remaining, 10) > 0) c.set(28, phase_flag() ? 9 : 8);
    else if (compare(c, remaining, 6) > 0) c.set(28, phase_flag() ? 11 : 10);
    else c.set(28, phase_flag() ? 13 : 12);
}

void GoalLineAction::tally_event() {
    if (opposite) {
        c.set(3, actor.match());
        c.call(0x101d6980, 0x1026a564);
        c.set(0, std::uint32_t(std::int8_t(c.r(3))));
        if (compare(c, std::int32_t(c.r(0)), -1) == 0) {
            c.set(3, c.u8(actor.address + 0xa));
            c.set(0, std::uint32_t(std::int8_t(c.r(28))));
            const bool goal = compare(c, std::int32_t(c.r(0)), 1) == 0;
            c.set(0, c.r(3) + 1);
            c.put8(actor.address + 0xa, c.r(0));
            if (goal) {
                c.set(3, c.u8(actor.address + 0xb));
                c.set(0, c.r(3) + 1);
                c.put8(actor.address + 0xb, c.r(0));
            }
        }
        c.set(3, actor.match());
        c.set(5, frame.at(0x3e));
        c.set(4, actor.side());
    } else {
        c.set(0, actor.side());
        c.set(5, frame.at(0x3e));
        c.set(3, actor.match());
        c.set(0, std::int8_t(c.r(0)) == 0);
        c.set(4, c.r(0));
    }
    c.set({{6, frame.at(0x3d)}, {7, frame.at(0x3c)}, {8, frame.at(0x3b)}, {9, frame.at(0x3a)}, {10, 1}});
    c.call(0x101f21d0, resume(0x1026a5b8, 0x1026da3c));
}

void GoalLineAction::announce(std::uint32_t message, std::uint32_t narrator, bool flip_side, std::uint32_t back, bool identify, std::uint32_t detail) {
    if (flip_side) c.set(0, c.u8(narrator + 0x27));
    c.set(4, message);
    c.set(3, actor.match());
    if (!flip_side) c.set(7, c.u8(narrator + 0x27));
    c.set(8, UINT32_MAX);
    if (flip_side) {
        c.set(0, std::uint32_t(std::int8_t(c.r(0))));
        c.set(6, identify ? c.u8(narrator + 0x2f) : UINT32_MAX);
    }
    c.set(3, c.u32(c.r(3) + 0x79f4));
    if (flip_side) c.set(0, c.r(0) == 0);
    c.set(9, UINT32_MAX);
    if (!flip_side) c.set(6, identify ? c.u8(narrator + 0x2f) : UINT32_MAX);
    c.set(10, detail);
    c.set(5, actor.match());
    if (flip_side) c.set(7, c.r(0));
    c.call(0x10201180, back);
}

bool GoalLineAction::keeper_role(std::uint32_t back) {
    c.set(3, c.r(30));
    c.call(0x10289560, back);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0)), 1) == 0;
}

bool GoalLineAction::stack_byte(unsigned offset) {
    c.set(0, std::uint32_t(c.i8(frame.at(offset))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

void GoalLineAction::refill_pool(std::uint32_t back) {
    c.set(3, c.u32(c.r(29)));
    c.set(0, c.u32(c.r(end_register())));
    c.set(3, c.r(3) + 2);
    const bool wrap = compare_logical(c, c.r(3), c.r(0)) > 0;
    c.put32(c.r(29), c.r(3));
    if (wrap) replace_seed(back);
}

void GoalLineAction::modify_halfword(std::uint32_t address, int amount, unsigned input_register) {
    c.set(input_register, std::uint32_t(c.i16(address)));
    c.set(0, c.r(input_register) + std::uint32_t(amount));
    c.put16(address, c.r(0));
}

std::int32_t GoalLineAction::draw_constant(std::uint32_t back, unsigned bound, std::int32_t magic, unsigned shift) {
    refill_pool(back);
    c.set(4, c.u32(c.r(29)));
    c.set(0, c.u32(c.r(27)));
    c.set(4, c.u16(c.r(4)) + c.r(0));
    auto high = std::uint32_t((std::int64_t(std::int32_t(c.r(4))) * magic) >> 32);
    if (magic < 0) high += c.r(4);
    if (shift) c.s.wr<1>(reg::xer_ca, std::int32_t(high) < 0 && (high & ((1u << shift) - 1)));
    const auto quotient = std::int32_t(high) >> shift;
    c.set(3, shift ? unsigned(quotient < 0) : std::uint32_t(high));
    c.set(0, c.r(4) - bound * std::uint32_t(quotient + (quotient < 0)));
    return std::int32_t(c.r(0));
}

bool GoalLineAction::draw_constant_zero(std::uint32_t back, unsigned bound, std::int32_t magic, unsigned shift) {
    return compare(c, draw_constant(back, bound, magic, shift)) == 0;
}

bool GoalLineAction::score_matches(int lead) {
    c.set(3, std::uint32_t(c.i8(frame.at(0x3e)) - lead));
    c.set(0, std::uint32_t(c.i8(frame.at(0x3d))));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0;
}

void GoalLineAction::through_tie_totals() {
    initial_draws();
    threshold_and_keeper();
    if (compare(c, std::int32_t(c.r(30))) == 0) missing_keeper();
    sample_keeper();
    choose_event();
    tally_event();
}

void GoalLineAction::run() {
    through_tie_totals();
    run_after_tie_totals();
}

void GoalLineAction::run_after_tie_totals() {
    if (finish_non_goal()) return;
    prepare_goal_effects();
    if (finish_saved_goal()) return;
    prepare_awarded_goal();
    if (review_initial_goal() || review_later_goal()) return;
    if (!confirm_goal()) apply_confirmed_goal_to_team();
    finish_confirmed_goal();
}
}
