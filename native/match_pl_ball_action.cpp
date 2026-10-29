#include "match_pl_ball_action_detail.hpp"

namespace cm::aot::match_pl::detail {
using namespace guest;

BallAction::BallAction(Cpu& cpu, std::uint32_t player_address, std::uint32_t source_column,
    std::uint32_t source_row, std::uint32_t action, std::uint32_t control,
    std::uint32_t recipient, std::uint32_t target_column, std::uint32_t target_row)
    : c(cpu), frame(c, 0x10247030, 0x150, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 30}),
      fp(c), actor{c, player_address}, doubles(c.toc(-0x51e8)), seed(c.toc(-0x7b90)),
      begin(c.toc(-0x7b8c)), end(c.toc(-0x7b88)), cursor(c.toc(-0x7b84)), singles(c.toc(-0x51e0)) {
    keep(c, {{23, player_address}, {24, action}, {25, recipient}, {26, target_column},
             {17, doubles}, {18, seed}, {19, begin}, {20, end}, {21, cursor}, {22, singles}});
    c.put16(at(0x4a), 0);
    c.set_f(31, c.f32(player() + 0x50)); c.set_f(30, c.f32(player() + 0x50));
    // These are byte-sized caller home slots. Their adjacent bytes are live.
    c.set(4, source_column); c.set(5, source_row); c.set(7, control); c.set(10, target_row);
}

std::uint32_t BallAction::run() {
    initial_condition_draw(); initial_distance_draw(); calculate_quality(); query_opponent();
    if (contest()) return guest::result(c, 0);
    delivery(); accuracy(); exertion(); contextual_commentary(); direction_flags(); action_commentary();
    flight_phase(); return publish();
}

std::int32_t BallAction::high(std::uint32_t value, std::int32_t multiplier) {
    return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
}
std::uint32_t BallAction::shifted_sign(std::int32_t value, unsigned bits) {
    c.s.wr<1>(reg::xer_ca, value < 0 && (std::uint32_t(value) & ((1u << bits) - 1)));
    return unsigned(value < 0);
}
void BallAction::advance(std::uint32_t back) {
    const auto next = c.u32(cursor) + 2; c.set(3, next);
    const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
    if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
}
std::int32_t BallAction::modulo(unsigned divisor) {
    const auto word = sample(); c.set(4, word);
    std::int32_t product = 0; unsigned shift = 0; bool add_word = false;
    switch (divisor) {
    case 3: product = high(word, 0x55555556); break;
    case 5: product = high(word, 0x66666667); shift = 1; break;
    case 6: product = high(word, 0x2aaaaaab); break;
    case 7: product = high(word, std::int32_t(0x92492493)); shift = 2; add_word = true; break;
    case 9: product = high(word, 0x38e38e39); shift = 1; break;
    case 10: product = high(word, 0x66666667); shift = 2; break;
    case 20: product = high(word, 0x66666667); shift = 3; break;
    case 35: product = high(word, std::int32_t(0xea0ea0eb)); shift = 5; add_word = true; break;
    case 40: product = high(word, 0x66666667); shift = 4; break;
    case 50: product = high(word, 0x51eb851f); shift = 4; break;
    default: throw std::logic_error("Unsupported ball-action remainder");
    }
    if (add_word) product = std::int32_t(std::uint32_t(product) + word);
    c.set(3, shift ? shifted_sign(product, shift) : unsigned(product < 0));
    return std::int32_t(word) % int(divisor);
}
std::int32_t BallAction::binary(unsigned divisor) {
    c.set(3, c.u32(cursor)); const auto value = sample();
    c.set(3, unsigned(std::int32_t(value) < 0)); return std::int32_t(value) % int(divisor);
}
void BallAction::range(std::int32_t bound, unsigned destination, std::uint32_t back,
    std::uint32_t correction, bool narrow_only, bool encoded_bound) {
    if (compare(c, bound) == 0) { c.set(destination, 0); return; }
    advance(back); c.set(3, 65535); bool narrow = compare(c, bound, 65535) <= 0;
    if (narrow) { c.set(3, std::uint32_t(-65535)); narrow = compare(c, bound, -65535) >= 0; }
    if (narrow) {
        std::uint32_t word;
        if (encoded_bound) {
            c.set(5, c.u32(cursor)); c.set(3, std::uint32_t(bound)); c.set(4, c.u32(seed)); compare(c, bound);
            word = std::uint32_t(c.u16(c.r(5))) + c.r(4); c.set(4, word);
        } else {
            c.set(3, c.u32(cursor)); compare(c, bound); const auto seed_word = c.u32(seed);
            word = std::uint32_t(c.u16(c.r(3))) + seed_word; c.set(3, word);
        }
        const auto residual = word - std::uint32_t(divide_signed<4>(word, std::uint32_t(bound))) * std::uint32_t(bound);
        c.set(destination, bound > 0 ? residual : 0u - residual); return;
    }
    if (narrow_only) throw std::logic_error("Ball-action narrow-range invariant");
    c.put32(at(0x98), std::uint32_t(bound) ^ 0x80000000); c.put32(at(0x94), 0x43300000);
    c.set(7, c.u32(cursor)); c.set(6, 0x80008001); c.set(5, 65535); c.set(4, 0x43300000); c.set(3, 0x80008001);
    c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, c.f64(at(0x94))); c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set(8, c.u16(c.r(7))); c.set_f(0, c.f64(doubles)); c.set(7, c.u32(seed)); c.put32(at(0x9c), c.r(4));
    c.set_f(2, fp.fdiv(c.f(1), c.f(0))); c.set(7, c.r(8) + c.r(7)); c.set_f(1, c.f64(doubles + 0x80));
    const auto word = c.r(7);
    const auto product = std::int32_t(std::uint32_t(high(word, std::int32_t(0x80008001))) + word);
    c.set(6, shifted_sign(product, 15));
    const auto reduced = word - std::uint32_t(std::int32_t(word) / 65535) * 65535u; c.set(destination, reduced);
    c.set(4, reduced ^ 0x80000000); c.put32(at(0xa0), c.r(4)); c.set_f(0, fp.fsub(c.f64(at(0x9c)), c.f(1)));
    c.set_f(0, fp.fmul(c.f(2), c.f(0))); const auto numerator = std::uint32_t(bound) * 10;
    shifted_sign(std::int32_t(std::uint32_t(high(numerator, std::int32_t(0x80008001))) + numerator), 15);
    c.set(3, std::uint32_t(std::int32_t(numerator) / 65535)); c.put64(at(0xa4), fp.fctiwz(0, c.f(0)));
    c.set(destination, c.u32(at(0xa8)));
    // The only word-sized wide caller is the keeper's displacement guard.
    // Its correction call retains the first reduction's quotient/product.
    const auto word_quotient = std::uint32_t(std::int32_t(word) / 65535);
    c.set(4, 0x80008001); c.set(5, word_quotient * 65535u); c.set(6, word_quotient); c.set(7, unsigned(product < 0));
    enter(c, correction, {c.r(3)}); sample_range(c, c.r(3));
    const auto adjustment = c.r(3) + 5; c.set(4, shifted_sign(high(adjustment, 0x66666667), 2));
    c.set(3, std::uint32_t(std::int32_t(adjustment) / 10)); const auto value = c.r(destination) + c.r(3);
    c.set(destination, compare(c, std::int32_t(value), bound - 1) > 0 ? std::uint32_t(bound - 1) : value);
}
void BallAction::condition_delta(std::uint32_t address, int amount) {
    c.set(3, std::uint32_t(std::int32_t(c.i16(address + 0x36)))); c.put16(address + 0x36, c.r(3) + std::uint32_t(amount));
}
void BallAction::player_message(unsigned id, std::uint32_t back, bool duration) {
    leave(c, {{3, c.u32(match() + 0x79f4)}, {4, id}, {5, match()}, {6, c.u8(player() + 0x2f)},
              {7, actor.side()}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, duration ? std::uint32_t(std::int32_t(std::int8_t(c.r(27)))) : 0}});
    c.call(0x10201180, back);
}
void BallAction::receiver_message(unsigned id, std::uint32_t back, bool duration) {
    leave(c, {{3, c.u32(match() + 0x79f4)}, {4, id}, {5, match()}, {6, c.u8(player() + 0x2f)},
              {7, actor.side()}, {8, c.u8(receiver() + 0x2f)}, {9, c.u8(receiver() + 0x27)},
              {10, duration ? std::uint32_t(std::int32_t(std::int8_t(c.r(27)))) : 0}}); c.call(0x10201180, back);
}
void BallAction::target_message(unsigned id, std::uint32_t address, std::uint32_t back) {
    leave(c, {{3, c.u32(match() + 0x79f4)}, {4, id}, {5, match()}, {6, c.u8(address + 0x2f)},
              {7, c.u8(address + 0x27)}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}}); c.call(0x10201180, back);
}
void BallAction::paired_message(unsigned id, std::uint32_t back, bool reverse) {
    const auto first = reverse ? player() : opponent(), second = reverse ? opponent() : player();
    leave(c, {{3, c.u32(match() + 0x79f4)}, {4, id}, {5, match()}, {6, c.u8(first + 0x2f)},
              {7, c.u8(first + 0x27)}, {8, c.u8(second + 0x2f)}, {9, c.u8(second + 0x27)}, {10, 0}});
    c.call(0x10201180, back);
}
void BallAction::side_message(unsigned id, std::uint32_t side, std::uint32_t back) {
    leave(c, {{3, c.u32(match() + 0x79f4)}, {4, id}, {5, match()}, {6, UINT32_MAX},
              {7, side}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}}); c.call(0x10201180, back);
}
bool BallAction::opposite_area(std::uint32_t address) {
    const Player p{c, address}; const bool upper = compare(c, int(std::int8_t(actor.side())) == 0, 1) == 0;
    c.set(3, 0); bool inside = compare(c, p.column(), 2) >= 0;
    if (inside) inside = compare(c, p.column(), 6) <= 0;
    if (inside) inside = upper ? compare(c, p.row(), 10) >= 0 : compare(c, p.row()) >= 0;
    if (inside) inside = compare(c, p.row(), upper ? 11 : 1) <= 0;
    if (inside) c.set(3, 1); return compare(c, std::int8_t(c.r(3))) != 0;
}
bool BallAction::defensive_area(std::uint32_t address) {
    const Player p{c, address}; const bool upper = compare(c, p.side(), 1) == 0;
    c.set(3, 0); bool inside = compare(c, p.column(), 2) >= 0;
    if (inside) inside = compare(c, p.column(), 6) <= 0;
    if (inside) inside = upper ? compare(c, p.row(), 10) >= 0 : compare(c, p.row()) >= 0;
    if (inside) inside = compare(c, p.row(), upper ? 11 : 1) <= 0;
    if (inside) c.set(3, 1); return compare(c, std::int8_t(c.r(3))) != 0;
}
bool BallAction::protected_action(std::uint32_t back) {
    leave(c, {{3, match()}, {4, c.u8(at(0x45))}}); c.call(0x101ee530, back); return compare(c, std::int8_t(c.r(3))) != 0;
}
}

namespace cm::aot::match_pl {
std::uint32_t resolve_ball_action(Cpu& c, std::uint32_t player, std::uint32_t from_column,
    std::uint32_t from_row, std::uint32_t kind, std::uint32_t control,
    std::uint32_t receiver, std::uint32_t to_column, std::uint32_t to_row) {
    detail::BallAction action(c, player, from_column, from_row, kind, control, receiver, to_column, to_row);
    return action.run();
}
}
