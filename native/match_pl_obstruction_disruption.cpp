#include "match_pl_obstruction.hpp"

namespace cm::aot::match_pl {
using namespace guest;

bool PlayerObstruction::goal_line_center() {
    c.set(0, c.u8(actor.address + 0x105));
    if (compare(c, std::int32_t(c.r(0)), 4) != 0) return false;
    c.set(0, std::uint32_t(actor.row()));
    if (compare(c, std::int32_t(c.r(0))) == 0) return true;
    c.set(0, c.u8(actor.address + 0x106));
    return compare(c, std::int32_t(c.r(0)), 11) == 0;
}

void PlayerObstruction::add_actor_byte(unsigned field, std::uint32_t amount) {
    c.set(3, c.u8(actor.address + field));
    c.set(0, c.r(3) + amount);
    c.put8(actor.address + field, c.r(0));
}

void PlayerObstruction::add_condition(unsigned amount) {
    c.set(3, std::uint32_t(c.i16(actor.address + 0x36)));
    c.set(0, c.r(3) + amount);
    c.put16(actor.address + 0x36, c.r(0));
}

void PlayerObstruction::queue_disruption(std::uint32_t queued_actor, std::uint32_t refill_back, std::uint32_t queue_back) {
    c.set(4, actor.match());
    c.set(3, c.u32(c.r(4) + 0x3108));
    c.set(0, c.r(3) - 300);
    c.put32(c.r(4) + 0x3108, c.r(0));
    refill(refill_back);
    const auto word = draw_word();
    c.set(11, word);
    const auto high = std::int32_t((std::int64_t(std::int32_t(word)) * 0x55555556) >> 32);
    const auto remainder = word - std::uint32_t(high + (high < 0)) * 3;
    c.set(0, remainder);
    c.set({{10, queued_actor}, {3, actor.match()}, {8, 0}, {4, c.u8(actor.address + 0x105)},
           {5, c.u8(actor.address + 0x106)}, {7, actor.side()}, {9, 1}, {6, std::uint32_t(std::int8_t(remainder))}});
    c.call(0x101f1100, queue_back);
}

void PlayerObstruction::finish_light_disruption() {
    c.set({{3, actor.match()}, {4, actor.side()}});
    c.call(0x101f1e10, 0x10273d88);
    if (compare_logical(c, c.r(3), actor.address) == 0) announce_actor(0x1fe7, 0, 0x10273db8);
    else {
        bool line_stop = goal_line_center();
        if (line_stop) {
            c.set(0, std::uint32_t(std::int16_t(c.r(18))));
            line_stop = compare(c, std::int32_t(c.r(0)), 200) > 0;
        }
        if (line_stop) {
            announce_actor(0x1ffd, 0, 0x10273e14);
            queue_disruption(0, 0x10273e44, 0x10273e9c);
        } else {
            const auto detail = std::uint32_t(std::int8_t(c.r(26)));
            c.set(0, std::uint32_t(std::int8_t(c.r(20))));
            if (compare(c, std::int32_t(c.r(0))) != 0) announce_actor(0x21d8, detail, 0x10273ed0);
            else {
                c.set(0, detail);
                if (compare(c, std::int32_t(detail), 5) == 0) announce_actor(0x1f85, detail, 0x10273f08);
                else announce_actor(0x1f84, detail, 0x10273f34);
            }
            queue_disruption(actor.address, 0x10273f64, 0x10273fbc);
        }
    }
    add_actor_byte(0x12, 1);
    add_condition(20);
    refill(0x10273ff4);
    c.set(4, c.u32(c.r(29)));
    c.set(3, 1);
    c.set(0, c.u32(c.r(28)));
    c.set(5, c.u16(c.r(4)));
    c.set(4, c.u8(actor.address + 0x2c));
    const auto remainder = std::int32_t(c.r(5) + c.r(0)) % 2;
    c.set(5, std::uint32_t(remainder));
    c.set(0, c.r(4) + std::uint32_t(remainder + 1));
    c.put8(actor.address + 0x2c, c.r(0));
    c.put16(actor.address + 0x19c, c.r(23));
}

void PlayerObstruction::finish_strong_disruption() {
    add_actor_byte(0x12, 1);
    add_condition(30);
    add_actor_byte(0x2c, 1);
    if (goal_line_center()) announce_actor(0x1ffe, 0, 0x102740ac);
    else {
        const auto detail = std::uint32_t(std::int8_t(c.r(26)));
        c.set(0, std::uint32_t(std::int8_t(c.r(20))));
        if (compare(c, std::int32_t(c.r(0))) != 0) announce_actor(0x21d6, detail, 0x102740e0);
        else {
            c.set(0, detail);
            if (compare(c, std::int32_t(detail), 5) == 0) announce_actor(0x1f66, detail, 0x10274118);
            else announce_actor(0x1f65, detail, 0x10274144);
        }
    }
    c.set(3, actor.address);
    c.call(0x1022e3a0, 0x1027414c);
    c.put16(actor.address + 0x19c, c.r(23));
    c.set(3, 1);
}
}
