// MATCH_ENG.CPP, contact: the queue of contacts the engine collects during a step (events at +0xb4, twelve
// bytes each, counted at +0x1bc), and what each one causes.
#include "match_eng_contact.hpp"
#include "match_eng_shots.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace shots_detail;

namespace {
constexpr std::uint32_t flags_of = 0x10289560;
}

// 0x101f75a0: first call of a period: the queue is handed over (0x102876c0) and the phase byte +0x792a becomes 1 once
// events were collected. Later: each queued event is judged by the discipline object and may add messages, mark the
// player and change play; then the sides whose flag byte (+0x1e4) is set are brought up to date and the phase becomes 2.
void process_contact_events(Cpu& c, std::uint32_t match, std::uint32_t queue) {
    Frame frame(c, 0x101f75a0, 0x70, {.first_gpr = 25});
    Kept<31> match_kept(c, match);
    c.put32(frame.caller() + 0x1c, queue);
    Kept<29> text_object(c, c.toc(-0x7a6c));
    if (compare(c, s8(c.u8(match + 0x792a)), 0) == 0) {
        c.set({{3, c.u32(frame.caller() + 0x1c)}, {4, 0}});
        c.call(0x102876c0, 0x101f75d8);
        if (compare(c, s8(c.u8(match + 0x1bc)), 0) != 0) c.put8(match + 0x792a, 1);
        return;
    }
    // bl 0x101d6710 (the register r8 is cleared too) and 0x101d5810: a message, with the match's word +0x79f4 in r3.
    const auto message = [&](std::uint32_t id, std::uint32_t first, std::uint32_t second, std::uint32_t back) {
        c.set({{4, id}, {5, match}, {6, first}, {7, second}, {8, 0}});
        call(c, 0x101d6710, back);
    };
    const auto short_message = [&](std::uint32_t id, std::uint32_t first, std::uint32_t second, std::uint32_t back) {
        c.set({{4, id}, {5, match}, {6, first}, {7, second}});
        call(c, 0x101d5810, back);
    };
    const auto word = [&] { c.set(3, match); word_79f4(c, match); };
    for (Kept<30> n(c, 0); (c.set(3, u(s8(n))), compare(c, s8(n), s8(c.u8(match + 0x1bc))) < 0); n = n + 1) {
        const auto event = match + u(s8(n) * 12) + 0xb4;
        const auto player = c.u32(event);
        const auto side_byte = [&] { return c.u8(player + 0x27); };
        c.set(3, player);
        c.call(flags_of, 0x101f7610);
        keep(c, {{27, extended(std::int16_t(c.r(3)))}});
        c.put8(frame.at(0x40), 1);
        keep(c, {{28, UINT32_MAX}});
        c.put32(frame.at(0x38), 0);
        enter(c, 0x101f767c, {match, player, c.u32(event + 4), frame.at(0x40), c.u8(event + 0xa), c.u8(event + 8), 0, frame.at(0x44)});
        discipline_1e00(c, match, player, c.u32(event + 4), frame.at(0x40), c.u8(event + 0xa), c.u8(event + 8), 0, frame.at(0x44));
        const auto outcome = s8(c.u8(frame.at(0x40)));
        if (outcome == 3 || outcome == 4) {
            keep(c, {{28, outcome == 3 ? 0x1ff7u : 0x1ff8u}});
            // The side's flag byte records whether the contact came with flags 1.
            c.set(4, u(std::int16_t(c.r(27)) == 1));
            c.set(3, u(s8(side_byte())));
            c.put8(match + 0x1e4 + c.r(3), c.r(4));
        } else if (outcome == 2) {
            keep(c, {{28, 0x1ff9}});
        } else {
            const auto roll = std::uint32_t(draw(c, 7, 0x101f7740)) + 2;
            c.set(4, roll);
            if (compare(c, s8(c.u8(event + 8)), std::int32_t(roll)) > 0) {
                c.set(3, player);
                if (compare(c, s8(position(c, player)), 0) >= 0) keep(c, {{28, 0x210a}});
            }
        }
        if (compare(c, std::int16_t(c.r(28)), -1) > 0) {
            keep(c, {{25, draw(c, 0x14, 0x101f7798)}});
            c.set(3, u(s8(c.u8(event + 8))));
            if (compare(c, std::int32_t(draw(c, c.r(3), 0x101f77b4)), std::int32_t(c.r(25))) > 0) {
                word();
                message(0x20f5, 0, u(s8(side_byte()) == 0), 0x101f77fc);
            }
            word();
            message(0x2109, c.u8(player + 0x2f), side_byte(), 0x101f783c);
            word();
            message(c.r(28), c.u8(player + 0x2f), side_byte(), 0x101f787c);
        }
        c.set({{3, match}, {4, c.u8(event + 9)}});
        call(c, 0x101ee530, 0x101f7894);
        const bool rejected = compare(c, s8(c.r(3)), 0) != 0 || compare(c, std::int16_t(c.r(28)), -1) <= 0;
        if (!rejected) {
            if (compare(c, std::int32_t(draw(c, 9, 0x101f78b4)), 0) == 0) {
                word();
                short_message(0x20df, side_byte(), 0, 0x101f78e8);
                if (compare(c, std::int32_t(draw(c, 3, 0x101f78f0)), 0) == 0) {
                    word();
                    short_message(0x20e0, side_byte(), 0, 0x101f7924);
                }
                if (outcome == 6 || outcome == 3 || outcome == 4 || outcome == 5) {
                    c.set(3, player);
                    c.set(4, 1);
                    c.put8(player + 0x16, 1);
                    if (compare(c, std::int32_t(draw(c, 7, 0x101f7984)), 0) == 0) {
                        word();
                        short_message(0x20d9, side_byte(), 0, 0x101f79b8);
                    }
                }
            }
            c.set({{3, match}, {4, player}});
            c.set_f(1, c.f32(frame.at(0x44)));
            call(c, 0x101d6780, 0x101f79d4);
            if (outcome == 4 || outcome == 3) {
                c.set({{3, player}, {4, 1}});
                call(c, 0x102884d0, 0x101f7a0c);
                c.set({{3, match}, {4, player}, {5, 0}});
                call(c, 0x101f1530, 0x101f7a28);
            } else {
                c.set({{3, player}, {4, 0}});
                call(c, 0x102884d0, 0x101f7a44);
            }
        } else if (outcome == 4 || outcome == 3) {
            c.set({{3, match}, {4, player}, {5, 0}});
            call(c, 0x101f1530, 0x101f7a84);
        }
    }
    c.put8(match + 0x1bc, 0);
    for (Kept<30> side(c, 0); compare(c, s8(side), 2) < 0; side = side + 1) {
        c.set(3, u(s8(side)));
        const auto stats = match + u(s8(side) * 0xd8) + side_stats_at;
        if (compare(c, s8(c.u8(match + 0x1e4 + u(s8(side)))), 0) != 0) {
            c.set({{3, match}, {4, side}});
            call(c, 0x101da5a0, 0x101f7ad0);
            if (compare(c, s8(c.r(3)), 0) == 0) {
                const auto stamp_clock = [&] { c.put16(match + 0x7944 + u(s8(side)) * 2, u(c.i16(match + 0x7948))); };
                c.set(3, stats);
                if (compare(c, s8(position(c, stats)), 0) != 0) {
                    c.set({{3, stats}, {4, 1}});
                    call(c, 0x1020ef70, 0x101f7b14);
                    stamp_clock();
                } else {
                    c.set(3, stats);
                    call(c, 0x102104d0, 0x101f7b40);
                    c.set({{3, match}, {4, side}});
                    call(c, 0x101d3bf0, 0x101f7b4c);
                    keep(c, {{25, c.r(3)}});
                    c.set(3, stats);
                    call(c, 0x10210430, 0x101f7b64);
                    keep(c, {{26, c.r(3)}});
                    c.set({{3, match}, {4, side}});
                    call(c, 0x101d3c50, 0x101f7b74);
                    c.set({{4, c.r(3)}, {3, c.u32(text_object)}, {5, c.r(26)}, {6, c.r(25)}});
                    c.call(0x103b0bf0, 0x101f7b88);
                    c.set({{3, match}, {4, side}});
                    call(c, 0x101f1de0, 0x101f7b94);
                    c.set({{3, match}, {4, side}});
                    call(c, 0x101f1e10, 0x101f7ba0);
                    stamp_clock();
                }
                c.set({{3, match}, {4, side}});
                call(c, 0x101d6a90, 0x101f7bc0);
                c.set(3, match);
                call(c, 0x101d6f90, 0x101f7bc8);
            }
        }
        c.set(4, 0);
        c.set(3, u(s8(side)));
        c.put8(match + 0x1e4 + u(s8(side)), 0);
    }
    c.put8(match + 0x792a, 2);
}
}
