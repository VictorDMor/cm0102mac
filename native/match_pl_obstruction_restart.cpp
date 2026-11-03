#include "match_pl_obstruction.hpp"

namespace cm::aot::match_pl {
using namespace guest;

// True continues to the restart-specific effects. False goes directly to the final contest.
bool PlayerObstruction::set_up_contest() {
    refill(0x10273028);
    const auto advantage = fixed_remainder(draw_word(), 150, 0x1b4e81b5, 4);
    c.set(0, advantage + 200);
    c.set(3, std::uint32_t(std::int16_t(c.r(21))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0) c.set(22, 1);
    c.set({{3, actor.match()}, {4, actor.address}, {5, c.r(23)}, {6, frame.at(0x41)}, {7, frame.at(0x40)}});
    c.call(0x101f50c0, 0x1027308c);
    c.put8(frame.at(0x42), c.r(3));
    c.set(0, std::uint32_t(c.i8(frame.at(0x42))));
    if (compare(c, std::int32_t(c.r(0))) != 0) {
        refill(0x102730bc);
        const auto chance = fixed_remainder(draw_word(), 5, 0x66666667, 1);
        c.set(0, chance);
        if (compare(c, std::int32_t(chance)) == 0) {
            c.set(0, std::uint32_t(std::int8_t(c.r(22))));
            if (compare(c, std::int32_t(c.r(0))) == 0) {
                c.set(0, c.u8(frame.at(0x41)));
                c.set({{4, c.r(23)}, {5, actor.address}, {7, c.r(22)}});
                c.put32(frame.at(0x38), c.r(0));
                c.set(6, 0);
                c.set(0, c.u8(frame.at(0x40)));
                c.put32(frame.at(0x3c), c.r(0));
                c.set({{3, actor.match()}, {8, c.u8(frame.at(0x42))},
                       {9, c.u8(actor.address + 0x105)}, {10, c.u8(actor.address + 0x106)}});
                c.call(0x101d68f0, 0x10273134);
                return false;
            }
        }
    }
    c.set(0, std::uint32_t(c.i8(frame.at(0x42))));
    if (compare(c, std::int32_t(c.r(0))) == 0) return false;
    c.set({{3, actor.match()}, {4, actor.address}, {6, c.u8(frame.at(0x41))}, {8, c.r(22)},
           {7, c.u8(frame.at(0x40))}, {5, frame.at(0x42)}, {9, frame.at(0x44)}});
    c.call(0x101f5180, 0x10273164);
    announce_actor(0x1fe8, 0, 0x1027318c);
    choose_restart_message();
    publish_restart();
    return true;
}

std::uint32_t PlayerObstruction::fixed_remainder(std::uint32_t word, unsigned bound, std::int32_t magic, unsigned shift) {
    auto high = std::uint32_t(std::uint64_t(std::int64_t(std::int32_t(word)) * magic) >> 32);
    if (magic < 0) high += word;
    if (shift) c.s.wr<1>(reg::xer_ca, std::int32_t(high) < 0 && (high & ((1u << shift) - 1)));
    const auto quotient = std::int32_t(high) >> shift;
    return word - std::uint32_t(quotient + (quotient < 0)) * bound;
}

void PlayerObstruction::announce_actor(std::uint32_t message, std::uint32_t detail, std::uint32_t back) {
    c.set(3, actor.match());
    c.set(4, message);
    c.set(7, actor.side());
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{9, UINT32_MAX}, {6, c.u8(actor.address + 0x2f)}, {10, detail}, {5, actor.match()}});
    c.call(0x10201180, back);
}

void PlayerObstruction::announce_opposition(std::uint32_t message, std::uint32_t back) {
    c.set(0, actor.side());
    c.set({{4, message}, {3, actor.match()}, {6, UINT32_MAX}});
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(5, actor.match());
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(0, c.r(0) == 0);
    c.set({{7, c.r(0)}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

void PlayerObstruction::choose_restart_message() {
    const auto outcome = std::int32_t(c.i8(frame.at(0x42)));
    c.set(0, std::uint32_t(outcome));
    c.set(26, UINT32_MAX);
    if (compare(c, outcome, 4) == 0) c.set(26, 0x1ff8);
    else if (outcome > 4) {
        if (compare(c, outcome, 6) == 0) c.set(26, 0x1ff5);
    } else if (compare(c, outcome, 2) == 0) c.set(26, 0x1ff9);
    else if (outcome > 2) c.set(26, 0x1ff7);
    if (c.r(26) != UINT32_MAX) return;

    refill(0x10273200);
    c.set(3, c.u32(c.r(29)));
    c.set(4, 0x92490000);
    c.set(5, c.u32(c.r(28)));
    c.set(4, 0x92492493);
    c.set(6, c.u16(c.r(3)));
    c.set(0, c.u8(frame.at(0x40)));
    c.set(5, c.r(6) + c.r(5));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(4, fixed_remainder(c.r(5), 7, std::int32_t(0x92492493u), 2));
    c.set(0, c.r(4) + 2);
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0) c.set(26, 0x210a);
}

void PlayerObstruction::publish_restart() {
    c.set(0, c.u8(frame.at(0x42)));
    if (compare(c, std::int32_t(c.r(0)), 7) == 0) {
        c.set({{3, actor.match()}, {0, 7}, {30, 0x1f79}});
        c.put8(c.r(3) + 0x7928, c.r(0));
    } else {
        c.set(0, actor.side());
        const bool away = compare(c, std::int32_t(c.r(0)), 1) == 0;
        c.set(3, 0);
        const bool inside = in_goal_region(away, 0, actor);
        c.set({{3, unsigned(inside)}, {0, unsigned(inside)}});
        if (compare(c, std::int32_t(c.r(0))) == 0) {
            c.set({{3, actor.match()}, {0, 6}, {30, 0x1f79}});
            c.put8(c.r(3) + 0x7928, c.r(0));
        } else {
            c.set(0, std::uint32_t(std::int16_t(c.r(26))));
            if (compare(c, std::int32_t(c.r(0)), -1) == 0) {
                refill(0x1027334c);
                const auto word = draw_word();
                const auto choice = std::int32_t(word) % 4;
                c.set({{3, word >> 31}, {0, std::uint32_t(choice)}});
                if (compare(c, choice, 1) == 0) announce_opposition(0x20ec, 0x10273404);
                else if (choice < 1 && compare(c, choice) >= 0) announce_opposition(0x20e9, 0x102733c8);
            }
            c.set({{4, actor.match()}, {3, 5}, {0, 0}, {30, 0x1f7a}});
            c.put8(c.r(4) + 0x7928, c.r(3));
            c.set(3, actor.match());
            c.put32(c.r(3) + 0x3110, c.r(0));
            c.set(3, std::uint32_t(c.i16(actor.address + 0x36)));
            c.set(0, c.r(3) - 500);
            c.put16(actor.address + 0x36, c.r(0));
        }
    }
    c.set({{4, c.u8(actor.address + 0x105)}, {0, std::uint32_t(std::int16_t(c.r(30)))}, {3, actor.match()}});
    const bool penalty = compare(c, std::int32_t(c.r(0)), 0x1f7a) == 0;
    c.set(0, 0);
    c.put8(c.r(3) + 0x791d, c.r(4));
    c.set({{4, c.u8(actor.address + 0x106)}, {3, actor.match()}});
    c.put8(c.r(3) + 0x791e, c.r(4));
    c.set({{3, actor.side()}, {4, actor.match()}});
    c.set(3, std::int8_t(c.r(3)) == 0);
    c.put8(c.r(4) + 0x7924, c.r(3));
    c.set(3, actor.match() + 0x10000);
    c.put32(c.r(3) - 0x2000, c.r(0));
    if (penalty) announce_opposition(c.r(30), 0x102734c4);
}

void PlayerObstruction::finish_restart() {
    c.set(0, std::uint32_t(std::int16_t(c.r(26))));
    if (compare(c, std::int32_t(c.r(0)), -1) > 0) announce_restart_reason();

    c.set(0, std::uint32_t(std::int16_t(c.r(30))));
    if (compare(c, std::int32_t(c.r(0)), 0x1f7a) != 0) announce_restart_player();
    c.set({{3, actor.match()}, {4, std::uint32_t(std::int8_t(c.r(23)))}});
    c.call(0x101ee530, 0x10273bac);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) == 0) {
        announce_possible_review();
        c.set({{3, actor.match()}, {4, actor.address}});
        c.set_f(1, c.f32(frame.at(0x44)));
        c.call(0x101d6780, 0x10273d5c);
        c.set({{3, actor.address}, {4, 0}});
        c.call(0x102884d0, 0x10273d68);
    }
    c.set(3, 1);
}

bool PlayerObstruction::draw_zero(std::uint32_t refill_back, unsigned bound, std::int32_t magic, unsigned shift) {
    refill(refill_back);
    const auto word = draw_word();
    if (bound == 4 || bound == 2) {
        c.set(0, std::uint32_t(std::int32_t(word) % std::int32_t(bound)));
        c.set(3, word >> 31);
    } else c.set(0, fixed_remainder(word, bound, magic, shift));
    return compare(c, std::int32_t(c.r(0))) == 0;
}

std::uint32_t PlayerObstruction::other_club() {
    c.set(0, actor.side());
    c.set(3, actor.match());
    c.set(0, std::int8_t(c.r(0)) == 0);
    const bool away = compare(c, std::int32_t(c.r(0))) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118));
    c.set(3, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    return c.r(3);
}

void PlayerObstruction::announce_restart_reason() {
    if (draw_zero(0x102734f0, 4, 0x40000000, 0)) {
        const auto bound = std::int32_t(c.i8(frame.at(0x40)));
        c.set(20, std::uint32_t(bound));
        if (compare(c, bound) == 0) c.set(19, 0);
        else {
            refill(0x10273554);
            // The wait is an extended byte. Preserve the final comparison and narrow draw.
            c.set(3, std::uint32_t(-65535));
            compare(c, bound, -65535);
            c.set(3, c.u32(c.r(29)));
            compare(c, bound);
            c.set(0, c.u32(c.r(28)));
            c.set(3, c.u16(c.r(3)) + c.r(0));
            const auto remainder = narrow_remainder(c.r(3), bound);
            c.set(19, remainder);
            c.set(0, bound > 0 ? remainder : 0u - remainder);
            c.set(19, c.r(0));
        }
        refill(0x102736b0);
        c.set(0, fixed_remainder(draw_word(), 20, 0x66666667, 3));
        if (compare(c, std::int32_t(c.r(19)), std::int32_t(c.r(0))) > 0) {
            c.set(0, c.u32(other_club() + 0x2c));
            if (compare_logical(c, c.r(0)) != 0) {
                c.set(3, c.u32(other_club() + 0x2c));
                c.set(0, c.u8(c.r(3) + 0x119));
                if (compare(c, std::int32_t(c.r(0)), 8) == 0) announce_neutral(0x20f5, 0, 0, 0x10273794);
            }
        }
        announce_actor(0x2109, 0, 0x102737bc);
    }
    announce_actor(c.r(26), 0, 0x102737e4);
    c.set(0, c.u8(frame.at(0x42)));
    bool apply = compare(c, std::int32_t(c.r(0)), 6) == 0;
    if (!apply) {
        c.set(3, c.u8(frame.at(0x42)));
        c.set(0, std::uint8_t(c.r(3) - 3));
        apply = compare_logical(c, c.r(0), 1) <= 0;
    }
    if (!apply) {
        c.set(0, c.u8(frame.at(0x42)));
        apply = compare(c, std::int32_t(c.r(0)), 5) == 0;
    }
    if (apply) {
        c.set({{3, actor.match()}, {4, actor.address}, {5, 0}});
        c.call(0x101f1530, 0x10273820);
    }
}

void PlayerObstruction::announce_neutral(std::uint32_t message, std::uint32_t side, std::uint32_t index, std::uint32_t back) {
    c.set({{3, actor.match()}, {4, message}, {5, actor.match()}, {6, index}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{7, side}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

void PlayerObstruction::announce_receiver(std::uint32_t message, std::uint32_t back) {
    c.set(0, actor.side());
    c.set({{4, message}, {3, actor.match()}, {8, UINT32_MAX}});
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(6, c.u8(c.r(26) + 0x2f));
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(0, c.r(0) == 0);
    c.set({{5, actor.match()}, {7, c.r(0)}, {9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

bool PlayerObstruction::receiver_beyond_midfield(const Player& receiver) {
    c.set(0, receiver.side());
    if (compare(c, std::int32_t(c.r(0)), 1) == 0) {
        c.set(0, std::uint32_t(receiver.row()));
        if (compare(c, std::int32_t(c.r(0)), 4) < 0) return true;
    }
    c.set(0, std::uint32_t(std::int8_t(receiver.side())));
    if (compare(c, std::int32_t(c.r(0))) != 0) return false;
    c.set(0, std::uint32_t(receiver.row()));
    return compare(c, std::int32_t(c.r(0)), 7) > 0;
}

void PlayerObstruction::announce_restart_player() {
    c.set(0, actor.side());
    c.set(6, 0);
    c.set(3, actor.match());
    c.set(5, c.u8(c.r(3) + 0x7928));
    c.set(0, std::int8_t(c.r(0)) == 0);
    c.set(3, actor.match());
    c.set(4, c.r(0));
    c.call(0x101f3a30, 0x10273854);
    c.set(26, c.r(3));
    if (compare(c, std::int32_t(c.r(26))) == 0) {
        announce_opposition(c.r(30), 0x10273894);
        return;
    }
    c.set(0, c.u8(frame.at(0x42)));
    if (compare(c, std::int32_t(c.r(0)), 7) == 0) announce_receiver(0x21e2, 0x102738dc);
    else announce_opposition(c.r(30), 0x10273918);
    c.set(0, std::uint32_t(std::int16_t(c.r(30))));
    if (compare(c, std::int32_t(c.r(0)), 0x1f79) != 0) return;
    const Player receiver{c, c.r(26)};
    c.set(0, c.u8(frame.at(0x42)));
    if (compare(c, std::int32_t(c.r(0)), 7) == 0) {
        const bool beyond = receiver_beyond_midfield(receiver);
        c.set(0, unsigned(beyond));
        if (compare(c, std::int32_t(c.r(0))) == 0) return;
        c.set(0, actor.side());
        const bool away = compare(c, std::int32_t(c.r(0)), 1) == 0;
        c.set(3, 0);
        const bool inside = in_goal_region(away, 0, receiver);
        c.set({{3, unsigned(inside)}, {0, unsigned(inside)}});
        if (compare(c, std::int32_t(c.r(0))) == 0) announce_receiver(0x21e1, 0x10273a60);
        return;
    }
    c.set(3, c.u8(actor.address + 0x105));
    const auto column = std::int32_t(std::int8_t(c.r(3)));
    c.set(0, std::uint32_t(column));
    const bool outside = compare(c, column, 2) < 0 || compare(c, column, 6) > 0;
    c.set(0, unsigned(outside));
    if (compare(c, std::int32_t(c.r(0))) != 0) {
        const bool beyond = receiver_beyond_midfield(receiver);
        c.set(0, unsigned(beyond));
        if (compare(c, std::int32_t(c.r(0))) != 0) {
            announce_receiver(0x21df, 0x10273b18);
            return;
        }
    }
    const bool beyond = receiver_beyond_midfield(receiver);
    c.set(0, unsigned(beyond));
    if (compare(c, std::int32_t(c.r(0))) != 0) announce_receiver(0x21e0, 0x10273ba0);
}

void PlayerObstruction::announce_possible_review() {
    if (!draw_zero(0x10273bd4, 9, 0x38e38e39, 1)) return;
    announce_neutral(0x20df, actor.side(), UINT32_MAX, 0x10273c38);
    if (draw_zero(0x10273c58, 3, 0x55555556, 0)) announce_neutral(0x20e0, actor.side(), UINT32_MAX, 0x10273cb8);
    c.set(3, c.u8(frame.at(0x42)));
    c.set(0, std::uint8_t(c.r(3) - 3));
    bool applicable = compare_logical(c, c.r(0), 3) <= 0;
    if (!applicable) {
        c.set(0, std::uint32_t(std::int16_t(c.r(30))));
        applicable = compare(c, std::int32_t(c.r(0)), 0x1f7a) == 0;
    }
    if (applicable && draw_zero(0x10273cf8, 2, 0x40000000, 0))
        announce_neutral(0x20d9, actor.side(), UINT32_MAX, 0x10273d4c);
}
}
