// MATCH_ENG.CPP, match advancement: the pending restart and the messages about it.
#include "match_eng_advance.hpp"
#include <bit>

namespace cm::aot::match_eng {
using namespace guest;

namespace {
// The pending restart's fields (offsets from the match).
constexpr std::uint32_t restart_flag = 0x30dc;      // the kind of stoppage (0: none)
constexpr std::uint32_t restart_byte_f = 0x30df;
constexpr std::uint32_t restart_byte_0 = 0x30e0;
constexpr std::uint32_t restart_kind = 0x30e2;      // int16, the switch of the message
constexpr std::uint32_t restart_player = 0x30e4;
constexpr std::uint32_t restart_other = 0x30e8;
constexpr std::uint32_t restart_mode = 0x30ec;
constexpr std::uint32_t restart_scratch = 0x3110;    // a word set when the kick lands in an end area

// The player's side (0 or 1) as the original flips it: `cntlzw` of the side byte, shifted.
std::uint32_t other_side(std::uint32_t side_byte) { return std::uint32_t(s8(side_byte) == 0); }
}  // namespace

std::uint32_t side_flag_1000(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101d5760, 0x40, {});
    c.put32(frame.at(0x58), match);
    c.put8(frame.at(0x5f), side);
    const auto record = c.u32(frame.at(0x58));
    const auto team = record + u(s8(c.u8(frame.at(0x5f))) * team_tactics_size) + team_tactics;
    c.set({{4, record}, {3, team}});
    const auto flags = tactical_flags(c, team);
    return result(c, compare(c, std::int32_t(flags & 0x1000)) != 0);
}

void post_event(Cpu& c, std::uint32_t target, std::uint32_t id, std::uint32_t source, std::uint32_t side, std::uint32_t last) {
    Frame frame(c, 0x101d5810, 0x40, {});
    c.put32(frame.at(0x58), target);
    c.put16(frame.at(0x5e), id);
    c.put32(frame.at(0x60), source);
    c.put8(frame.at(0x67), side);
    c.put32(frame.at(0x68), last);
    c.set({{3, c.u32(frame.at(0x58))}, {4, u(c.i16(frame.at(0x5e)))}, {5, c.u32(frame.at(0x60))}, {6, UINT32_MAX},
           {7, c.u8(frame.at(0x67))}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, c.u32(frame.at(0x68))}});
    c.call(0x10201180, 0x101d5854);
}

void post_player_event(Cpu& c, std::uint32_t target, std::uint32_t id, std::uint32_t source, std::uint32_t first,
                       std::uint32_t second, std::uint32_t last) {
    Frame frame(c, 0x101d6710, 0x40, {});
    c.put32(frame.at(0x58), target);
    c.put16(frame.at(0x5e), id);
    c.put32(frame.at(0x60), source);
    c.put8(frame.at(0x67), first);
    c.put8(frame.at(0x6b), second);
    c.put32(frame.at(0x6c), last);
    c.set({{3, c.u32(frame.at(0x58))}, {4, u(c.i16(frame.at(0x5e)))}, {5, c.u32(frame.at(0x60))}, {6, c.u8(frame.at(0x67))},
           {7, c.u8(frame.at(0x6b))}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, c.u32(frame.at(0x6c))}});
    c.call(0x10201180, 0x101d6758);
}

std::uint32_t restart_still_valid(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101d5880, 0x40, {.first_gpr = 31});
    Kept<31> r_match(c, match);
    c.set(3, match);
    if (compare(c, s8(pending_restart_side(c, match)), -1) == 0) return result(c, 0);
    c.set(3, match);
    if (compare(c, s8(pending_restart_side(c, match)), -1) <= 0) return result(c, 1);
    c.set(3, match + 0x10000);
    if (compare_logical(c, c.u32(match + pending_player)) == 0) return result(c, 1);
    const auto holder = c.u32(match + pending_player);
    c.set(3, holder);
    const auto holder_side = s8(c.u8(holder + 0x27));
    c.set(4, u(holder_side));
    const auto player = c.u32(match + restart_player);
    c.set(3, player);
    if (compare(c, holder_side, s8(other_side(c.u8(player + 0x27)))) != 0) return result(c, 1);
    const auto column = [&] { const auto value = s8(c.u8(match + ball_column_at)); c.set(3, u(value)); return value; };
    const auto wanted = [&] { return s8(c.u8(match + restart_column)); };
    const auto side_of_player = [&] { c.set(3, c.u32(match + restart_player)); return s8(c.u8(c.r(3) + 0x27)); };
    bool clear = false;
    if (compare(c, column(), wanted()) > 0) clear = compare(c, side_of_player(), 1) == 0;
    if (!clear) {
        if (compare(c, column(), wanted()) >= 0) return result(c, 1);
        if (compare(c, side_of_player(), 0) != 0) return result(c, 1);
    }
    c.set(3, match);
    clear_pending_restart(c, match);
    return result(c, 0);
}

std::uint32_t restart_owner(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101d69c0, 0x40, {.first_gpr = 31});
    Kept<31> r_match(c, match);
    c.set(3, match);
    if (compare(c, s8(pending_restart_side(c, match)), -1) <= 0) return result(c, 1);
    const auto player_side = s8(c.u8(c.u32(match + restart_player) + 0x27));
    c.set(3, u(player_side));
    if (compare(c, player_side, s8(c.u8(match + possession_side))) == 0) return result(c, 2);
    // A restart of kind 8 skips the end-area test.
    if (compare(c, s8(c.u8(match + restart_flag)), 8) != 0) {
        const auto side = u(s8(other_side(c.u8(match + possession_side))));
        c.set({{3, match}, {4, side}, {5, c.u8(match + restart_row)}, {6, c.u8(match + restart_column)}});
        if (compare(c, s8(inside_end_area(c, side, c.r(5), c.r(6)))) != 0) return result(c, 2);
    }
    if (compare(c, draw(c, 0xc8, 0x101d6a4c)) == 0) return result(c, 2);
    c.set(3, match);
    clear_pending_restart(c, match);
    return result(c, 1);
}

namespace {
// The message of a second player's restart, by kind: its id and where its code starts in the original
// (the call to the target getter sits at start+4, the message itself at start+0x34).
struct KindMessage { std::int32_t kind; std::uint32_t id, start; };
constexpr KindMessage kind_messages[] = {
    {45, 0x20fd, 0x5fa8}, {42, 0x20fe, 0x5fe4}, {43, 0x20ff, 0x6020}, {44, 0x2100, 0x605c}, {19, 0x2101, 0x6098},
    {33, 0x2101, 0x6098}, {34, 0x2105, 0x60d4}, {41, 0x2102, 0x6110}, {1, 0x2104, 0x614c},  {21, 0x2103, 0x6188},
    {46, 0x2106, 0x61c4}, {12, 0x2107, 0x6200},
};
}  // namespace

std::uint32_t resolve_restart(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101d5980, 0x60, {.first_gpr = 28});
    Kept<31> r_match(c, match);
    Kept<30> follow_up(c, c.r(30));  // the message after the kick (-1: none)
    Kept<29> outcome(c, c.r(29));    // 0x1f79: the kick is taken, 0x1f7a: it lands in the end area
    Kept<28> r_scratch(c, c.r(28));
    const auto player = [&] { return c.u32(match + restart_player); };
    const auto other = [&] { return c.u32(match + restart_other); };
    const auto side_byte = [&](std::uint32_t at) { return c.u8(at + 0x27); };
    const auto kind = [&] { return s8(c.u8(match + restart_flag)); };
    // `bl 0x101d5870`: the message target, in r3.
    const auto target = [&] { c.set(3, match); return word_79f4(c, match); };
    const auto finish = [&] {
        c.set(3, match);
        clear_pending_restart(c, match);
        return result(c, 1);
    };
    // The side the restart player does not belong to, as the original computes it into r4/r6.
    const auto opposite = [&](std::uint32_t at) { return u(s8(other_side(side_byte(at)))); };

    if (compare(c, kind(), 0) == 0) return result(c, 0);
    if (compare(c, s8(c.u8(match + play_state)), 0) != 0) {
        // A restart for the side that just had the ball, behind the line it was awarded on, is dropped.
        const auto cancelled = [&] {
            const auto last = s8(c.u8(match + possession_side));
            const auto holder_side = s8(side_byte(player()));
            c.set({{4, u(last)}, {6, std::uint32_t(std::countl_zero(u(holder_side)))}, {3, u(holder_side == 0)}});
            if (compare(c, last, holder_side == 0) != 0) return false;
            const auto column = [&] { const auto value = s8(c.u8(match + ball_column_at)); c.set(6, u(value)); return value; };
            const auto wanted = [&] { return s8(c.u8(match + restart_column)); };
            if (compare(c, column(), wanted()) > 0) {
                c.set(3, side_byte(player()));
                c.set(6, u(s8(c.r(3))));
                if (compare(c, s8(c.r(6)), 1) == 0) return true;
            }
            if (compare(c, column(), wanted()) >= 0) return false;
            c.set(3, player());
            return compare(c, s8(side_byte(c.r(3))), 0) == 0;
        };
        if (cancelled()) {
            c.set(3, match);
            clear_pending_restart(c, match);
            return result(c, 0);
        }
        const auto to = target();
        enter(c, 0x101d5a74, {to, 0x20f0, match, side_byte(player()), 0});
        post_event(c, to, 0x20f0, match, side_byte(player()), 0);
    }

    if (compare(c, kind(), 8) == 0) {
        // A kick-off: counted for the side, the ball and the turn are set.
        c.set(3, player());
        r_scratch = byte_2f(c, player());
        const auto to = target();
        enter(c, 0x101d5ab4, {to, 0x20ee, match, r_scratch, side_byte(player()), 0});
        post_player_event(c, to, 0x20ee, match, r_scratch, side_byte(player()), 0);
        c.set({{3, match}, {4, side_byte(player())}});
        count_side_event(c, match, c.r(4), 0x1fb6);
        c.set(3, player());
        increment_byte(c, player() + 0x13);
        c.set({{3, match}, {4, c.u8(match + restart_row)}});
        set_ball_row(c, match, c.r(4));
        c.set({{3, match}, {4, c.u8(match + restart_column)}});
        set_ball_column(c, match, c.r(4));
        c.set({{3, match}, {4, 7}});
        set_byte_7928(c, match, 7);
        c.set(6, u(s8(side_byte(player()))));
        c.set({{3, match}, {4, opposite(player())}});
        set_possession_side(c, match, c.r(4));
        c.set({{3, match}, {4, 0}});
        set_word_e000(c, match, 0);
        return finish();
    }
    if (compare_logical(c, player()) == 0) return finish();

    // From here the restart has a player and the code runs twice over: with one player
    // (the first branch of the original) or with a second player involved.
    const bool two = compare_logical(c, other()) != 0;
    const auto pick = [&](std::uint32_t one, std::uint32_t both) { return two ? both : one; };
    // A message through post_event about `side`.
    const auto event = [&](std::uint32_t id, std::uint32_t back, std::uint32_t side) {
        const auto to = target();
        enter(c, back, {to, id, match, side, 0});
        post_event(c, to, id, match, side, 0);
    };
    const auto player_event = [&](std::uint32_t id, std::uint32_t back, std::uint32_t first, std::uint32_t second) {
        const auto to = target();
        enter(c, back, {to, id, match, first, second, 0});
        post_player_event(c, to, id, match, first, second, 0);
    };

    if (!two) {
        c.set({{3, match}, {4, player()}, {5, match + restart_flag}, {6, c.u8(match + restart_byte_f)},
               {7, c.u8(match + restart_byte_0)}, {8, u(s8(c.u8(match + restart_mode)))}, {9, frame.at(0x40)}});
        call(c, 0x101f5180, 0x101d5b5c);
        player_event(0x20fb, 0x101d5b84, c.u8(player() + 0x2f), side_byte(player()));
    } else {
        c.put32(frame.at(0x38), 0);
        c.set({{3, match}, {4, player()}, {5, other()}, {6, match + restart_flag}, {7, c.u8(match + restart_byte_f)},
               {8, c.u8(match + restart_byte_0)}, {9, u(s8(c.u8(match + restart_mode)))}, {10, frame.at(0x40)}});
        call(c, 0x101f5110, 0x101d5f88);
        // The announcement depends on the kind of restart (a jump table in the original).
        const auto word = c.i16(match + restart_kind);
        std::uint32_t id = 0x20fc, start = 0x623c;
        if (compare_logical(c, u(word), 0x2e) <= 0)
            for (const auto& entry : kind_messages)
                if (entry.kind == word) { id = entry.id; start = entry.start; }
        const auto to = target();
        c.set({{3, to}, {4, id}, {5, match}, {6, c.u8(player() + 0x2f)}, {7, side_byte(player())},
               {8, c.u8(other() + 0x2f)}, {9, side_byte(other())}, {10, 0}});
        c.call(0x10201180, 0x101d0000 + start + 0x38);
    }

    // The consequence of the kind: a message id for the first branch's table, which differs in 5 and 6.
    follow_up = UINT32_MAX;
    const auto restart_kind_now = kind();
    bool by_chance = false;
    if (const auto order = compare(c, restart_kind_now, 4); order == 0) {
        follow_up = 0x1ff8;
    } else if (order > 0) {
        if (!two) {
            if (compare(c, restart_kind_now, 6) == 0) follow_up = 0x1ff5; else by_chance = true;
        } else {
            if (compare(c, restart_kind_now, 6) >= 0) by_chance = true; else follow_up = 0x1ff6;
        }
    } else if (const auto lower = compare(c, restart_kind_now, 2); lower == 0) {
        follow_up = 0x1ff9;
    } else if (lower > 0) {
        follow_up = 0x1ff7;
    } else {
        by_chance = true;
    }
    if (by_chance) {
        const auto roll = std::int32_t(draw(c, 7, pick(0x101d5be0, 0x101d62d0))) + 2;
        c.set(3, u(roll));
        if (compare(c, s8(c.u8(match + restart_byte_0)), roll) > 0) follow_up = 0x210a;
    }

    // Where the kick ends up: a free kick (7) is simply taken, otherwise the end area matters.
    if (compare(c, kind(), 7) == 0) {
        outcome = 0x1f79;
        c.set({{3, match}, {4, 7}});
        set_byte_7928(c, match, 7);
    } else {
        c.set({{3, match}, {4, side_byte(player())}, {5, c.u8(match + restart_row)}, {6, c.u8(match + restart_column)}});
        if (compare(c, s8(inside_end_area(c, c.r(4), c.r(5), c.r(6)))) != 0) {
            outcome = 0x1f7a;
            c.set({{3, match}, {4, 5}});
            set_byte_7928(c, match, 5);
            const auto hurt = pick(player(), other());
            c.put32(match + restart_scratch, two ? other() : 0);
            const auto before = c.i16(hurt + 0x36);
            c.set({{4, hurt}, {3, u(before)}});
            c.put16(hurt + 0x36, u(before) - 0x1f4);
        } else {
            outcome = 0x1f79;
            c.set({{3, match}, {4, 6}});
            set_byte_7928(c, match, 6);
        }
    }
    c.set({{3, match}, {4, c.u8(match + restart_row)}});
    set_ball_row(c, match, c.r(4));
    c.set({{3, match}, {4, c.u8(match + restart_column)}});
    set_ball_column(c, match, c.r(4));
    c.set({{3, match}, {4, opposite(player())}});
    set_possession_side(c, match, c.r(4));
    c.set({{3, match}, {4, 0}});
    set_word_e000(c, match, 0);
    if (compare(c, std::int16_t(outcome), 0x1f7a) == 0) {
        c.set(4, outcome);
        event(0x1f7a, pick(0x101d5cfc, 0x101d63dc), two ? side_byte(other()) : opposite(player()));
    }

    if (compare(c, std::int16_t(follow_up), -1) > 0) {
        if (compare(c, draw(c, 4, pick(0x101d5d10, 0x101d63f0)), 0) == 0) {
            r_scratch = u(draw(c, 0x14, pick(0x101d5d20, 0x101d6400)));
            const auto roll = std::int32_t(draw(c, u(s8(c.u8(match + restart_byte_0))), pick(0x101d5d30, 0x101d6410)));
            if (compare(c, roll, std::int32_t(std::uint32_t(r_scratch))) > 0) player_event(0x20f5, pick(0x101d5d58, 0x101d6438), 0, 0);
            player_event(0x2109, pick(0x101d5d80, 0x101d6460), c.u8(player() + 0x2f), side_byte(player()));
        }
        player_event(follow_up, pick(0x101d5da8, 0x101d6488), c.u8(player() + 0x2f), side_byte(player()));
        if (compare(c, kind(), 6) == 0 || compare(c, kind(), 3) == 0 || compare(c, kind(), 4) == 0 || compare(c, kind(), 5) == 0) {
            c.set({{3, match}, {4, player()}, {5, 0}});
            call(c, 0x101f1530, pick(0x101d5df8, 0x101d64d8));
        }
    }
    if (compare(c, std::int16_t(outcome), 0x1f7a) != 0) {
        c.set(4, outcome);
        event(outcome, pick(0x101d5e34, 0x101d6504), two ? side_byte(other()) : opposite(player()));
    }

    c.set({{3, match}, {4, u(s8(c.i16(match + restart_kind)))}});
    call(c, 0x101ee530, pick(0x101d5e44, 0x101d6514));
    if (compare(c, s8(c.r(3)), 0) != 0) return finish();
    if (compare(c, draw(c, 9, pick(0x101d5e58, 0x101d6528)), 0) == 0) {
        event(0x20df, pick(0x101d5e80, 0x101d6560), two ? opposite(other()) : side_byte(player()));
        if (compare(c, draw(c, 3, pick(0x101d5e88, 0x101d6568)), 0) == 0)
            event(0x20e0, pick(0x101d5eb0, 0x101d65a0), two ? opposite(player()) : side_byte(player()));
        bool hit;
        if (!two) {
            hit = compare(c, kind(), 6) == 0 || compare(c, kind(), 3) == 0 || compare(c, kind(), 4) == 0 ||
                  compare(c, kind(), 5) == 0 || compare(c, std::int16_t(outcome), 0x1f7a) == 0;
        } else {
            // the original leaves the compared bytes in r3, r6 and r28 here
            const auto probe = [&](unsigned reg, std::int32_t value) {
                const auto byte = c.u8(match + restart_flag);
                c.set(reg, reg == 6 ? byte : u(s8(byte)));
                return compare(c, s8(byte), value) == 0;
            };
            hit = probe(3, 6) || probe(3, 3) || probe(6, 4) || probe(28, 5);
            if (!hit) { c.set(3, u(std::int16_t(outcome))); hit = compare(c, std::int16_t(outcome), 0x1f7a) == 0; }
        }
        if (hit) {
            if (two) c.set({{7, 1}, {4, player()}});
            else c.set(3, player());
            c.put8(player() + 0x16, 1);
            if (compare(c, draw(c, 7, pick(0x101d5f10, 0x101d6600)), 0) == 0)
                event(0x20d9, pick(0x101d5f38, 0x101d6644), two ? opposite(player()) : side_byte(player()));
        }
    }
    c.set({{3, match}, {4, player()}});
    c.set_f(1, c.f32(frame.at(0x40)));
    enter(c, pick(0x101d5f48, 0x101d6654), {match, player()});
    adjust_side_around(c, match, player(), c.f(1));
    c.set({{3, player()}, {4, 0}});
    call(c, 0x102884d0, pick(0x101d5f54, 0x101d6660));
    return finish();
}
}  // namespace cm::aot::match_eng
