#include "match_pl_ball_action_detail.hpp"

namespace cm::aot::match_pl::detail {
using namespace guest;

void BallAction::foul() {
    compare(c, byte(0x40)); c.put32(at(0x38), c.u8(at(0x41)));
    leave(c, {{3, match()}, {4, opponent()}, {5, player()}, {6, at(0x40)}, {7, c.u8(at(0x43))},
              {8, c.u8(at(0x42))}, {9, c.u8(at(0x44))}, {10, at(0x4c)}}); c.call(0x101f5110, 0x10248b30);
    style_message(false); c.set(16, UINT32_MAX);
    const auto severity = byte(0x40);
    if (compare(c, severity, 4) == 0) c.set(16, 0x1ff8);
    else if (severity < 4 && compare(c, severity, 2) == 0) c.set(16, 0x1ff9);
    else if (severity < 4 && severity > 2) c.set(16, 0x1ff7);
    else if (severity > 4 && compare(c, severity, 6) < 0) c.set(16, 0x1ff6);
    else {
        advance(0x10248dd0); const auto value = modulo(7);
        if (compare(c, byte(0x42), value + 2) > 0) c.set(16, 0x210a);
    }
    disciplinary_queue();
    if (compare(c, c.u8(at(0x40)), 7) == 0) { c.set(22, 0x1f79); c.put8(match() + 0x7928, 7); }
    else if (opposite_area(player())) {
        if (compare(c, std::int16_t(c.r(16)), -1) == 0) {
            advance(0x10249284); const auto choice = binary(4);
            if (compare(c, choice, 1) == 0) side_message(0x20ec, actor.side(), 0x1024931c);
            else if (choice < 1 && compare(c, choice) >= 0) side_message(0x20e9, actor.side(), 0x102492f0);
        }
        c.set(22, 0x1f7a); c.put8(match() + 0x7928, 5); c.put32(match() + 0x3110, player()); condition_delta(opponent(), -500);
        c.set(15, std::uint32_t(std::int32_t(c.i8(c.u32(opponent() + 0x6c) + 0x66))));
        range(std::int32_t(c.r(15)), 14, 0x10249378, 0x10249450, true);
        if (compare(c, std::int32_t(c.r(14))) == 0) {
            advance(0x102494dc); c.set(5, sample()); const auto change = std::int8_t(-modulo(10));
            enter(c, 0x10249524, {opponent(), std::uint32_t(std::int32_t(change))}); adjust_morale(c, opponent(), std::uint32_t(std::int32_t(change)));
            if (compare(c, std::int8_t(c.r(3))) != 0) { leave(c, {{3, opponent()}, {4, 0}}); c.call(0x1022ba10, 0x10249538); }
        }
    } else { c.set(22, 0x1f79); c.put8(match() + 0x7928, 6); }
    const bool penalty = compare(c, std::int16_t(c.r(22)), 0x1f7a) == 0;
    c.set(4, c.u8(player() + 0x105)); c.set(3, match()); c.put8(c.r(3) + 0x791d, c.r(4));
    c.set(4, c.u8(player() + 0x106)); c.set(3, match()); c.put8(c.r(3) + 0x791e, c.r(4));
    c.set(4, actor.side()); c.set(3, match()); c.put8(c.r(3) + 0x7924, c.r(4)); c.set(3, match() + 0x10000); c.put32(c.r(3) - 0x2000, 0);
    if (penalty) side_message(c.r(22), std::uint32_t(std::int32_t(std::int8_t(actor.side()))), 0x102495b8);
    if (compare(c, std::int16_t(c.r(16)), -1) > 0) {
        bool specific = compare(c, c.u8(at(0x45)), 44) == 0;
        if (!specific) specific = compare(c, c.u8(at(0x45)), 47) == 0;
        if (specific && compare(c, byte(0x41)) != 0) {
            const bool last = compare(c, c.u8(at(0x45)), 47) == 0;
            paired_message(last ? 0x21da : 0x21d9, last ? 0x1024961c : 0x10249648, true);
            if (compare_logical(c, std::uint8_t(c.u8(at(0x40)) - 3), 3) <= 0 || compare(c, std::int16_t(c.r(22)), 0x1f7a) == 0) c.put8(opponent() + 0x16, 1);
        }
        advance(0x10249690);
        if (compare(c, binary(4)) == 0) {
            c.set(15, std::uint32_t(std::int32_t(byte(0x42)))); range(std::int32_t(c.r(15)), 14, 0x102496f4, 0x102497cc, true);
            advance(0x10249850); const auto choice = modulo(20);
            if (compare(c, std::int32_t(c.r(14)), choice) > 0) {
                const auto controller = [&] {
                    const bool opposite = compare(c, int(std::int8_t(c.u8(opponent() + 0x27)) == 0)) != 0;
                    c.set(3, c.u32(c.u32(match() + 0x3118) + (opposite ? 0x20 : 0x1c))); return c.u32(c.r(3) + 0x2c);
                };
                if (compare_logical(c, controller()) != 0) {
                    c.set(3, controller());
                    if (compare(c, c.u8(c.r(3) + 0x119), 8) == 0) {
                        leave(c, {{3, c.u32(match() + 0x79f4)}, {4, 0x20f5}, {5, match()}, {6, 0}, {7, 0}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
                        c.call(0x10201180, 0x10249934);
                    }
                }
            }
            target_message(0x2109, opponent(), 0x1024995c);
        }
        target_message(c.r(16), opponent(), 0x10249984);
        bool discipline = compare(c, c.u8(at(0x40)), 6) == 0;
        if (!discipline) discipline = compare_logical(c, std::uint8_t(c.u8(at(0x40)) - 3), 1) <= 0;
        if (!discipline) discipline = compare(c, c.u8(at(0x40)), 5) == 0;
        if (discipline) { leave(c, {{3, match()}, {4, opponent()}, {5, 0}}); c.call(0x101f1530, 0x102499c0); }
    }
    if (compare(c, std::int16_t(c.r(22)), 0x1f7a) != 0) {
        leave(c, {{3, match()}, {4, actor.side()}, {5, c.u8(match() + 0x7928)}, {6, 0}}); c.call(0x101f3a30, 0x102499e4); c.set(14, c.r(3));
        if (compare(c, std::int32_t(c.r(14))) == 0) side_message(c.r(22), std::uint32_t(std::int32_t(std::int8_t(actor.side()))), 0x10249a18);
        else {
            const auto taker_message = [&](unsigned id, std::uint32_t back) {
                leave(c, {{3, c.u32(match() + 0x79f4)}, {4, id}, {5, match()}, {6, c.u8(c.r(14) + 0x2f)},
                          {7, std::uint32_t(std::int32_t(std::int8_t(actor.side())))}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}}); c.call(0x10201180, back);
            };
            if (compare(c, c.u8(at(0x40)), 7) == 0) taker_message(0x21e2, 0x10249a54);
            else side_message(c.r(22), std::uint32_t(std::int32_t(std::int8_t(actor.side()))), 0x10249a84);
            if (compare(c, std::int16_t(c.r(22)), 0x1f79) == 0 && compare(c, byte(0x41)) == 0) {
                if (compare(c, c.u8(at(0x40)), 7) == 0) {
                    enter(c, 0x10249ab0, {player()}); in_attacking_band(c, player());
                    if (compare(c, std::int8_t(c.r(3))) != 0 && !opposite_area(player())) taker_message(0x21e1, 0x10249ba0);
                } else {
                    enter(c, 0x10249bb0, {player(), c.u8(player() + 0x105)}); wide_column(c, actor.column()); bool wide = compare(c, std::int8_t(c.r(3))) != 0;
                    if (wide) { enter(c, 0x10249bc0, {player()}); in_attacking_band(c, player()); wide = compare(c, std::int8_t(c.r(3))) != 0; }
                    if (wide) taker_message(0x21df, 0x10249bf4);
                    else {
                        enter(c, 0x10249c00, {player()}); in_attacking_band(c, player()); if (compare(c, std::int8_t(c.r(3))) != 0) taker_message(0x21e0, 0x10249c34);
                    }
                }
            }
        }
    }
    if (!protected_action(0x10249c40)) {
        const auto card_or_penalty = [&] {
            return compare_logical(c, std::uint8_t(c.u8(at(0x40)) - 3), 3) <= 0 || compare(c, std::int16_t(c.r(22)), 0x1f7a) == 0;
        };
        if (compare(c, byte(0x41)) != 0) {
            player_message(0x20dd, 0x10249c7c);
            bool caution = compare(c, c.u8(at(0x40)), 4) == 0; if (!caution) caution = compare(c, c.u8(at(0x40)), 5) == 0;
            if (caution) c.put8(opponent() + 0x16, 1);
            if (card_or_penalty()) { advance(0x10249cdc); if (compare(c, binary()) == 0) side_message(0x20d9, unsigned(std::int8_t(actor.side()) == 0), 0x10249d40); }
        } else {
            advance(0x10249d64);
            if (compare(c, modulo(9)) == 0) {
                side_message(0x20df, unsigned(std::int8_t(actor.side()) == 0), 0x10249dd8);
                advance(0x10249df8); if (compare(c, modulo(3)) == 0) side_message(0x20e0, unsigned(std::int8_t(actor.side()) == 0), 0x10249e68);
                if (card_or_penalty()) { advance(0x10249ea8); if (compare(c, binary()) == 0) side_message(0x20d9, unsigned(std::int8_t(actor.side()) == 0), 0x10249f0c); }
            }
        }
        leave(c, {{3, match()}, {4, opponent()}}); c.set_f(1, c.f32(at(0x4c))); c.call(0x101d6780, 0x10249f1c);
        enter(c, 0x10249f28, {opponent(), 0}); resolve_incident_response(c, opponent(), 0);
    }
    c.set(4, std::uint32_t(std::int32_t(c.i16(opponent() + 0x36)))); c.put16(opponent() + 0x36, c.r(4) - 25); c.set(3, 0);
}
}
