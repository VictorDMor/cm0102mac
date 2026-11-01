// MATCH_ENG.CPP, contact: the step's bookkeeping over the match's list of pending events (0x101f84e0): each is
// kept or dropped, and for each side the players' appearances are weighed into a rating and reported.
#include "match_eng_contact.hpp"
#include "match_eng_shots.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace shots_detail;
using contact_detail::divide_by_11;

namespace {
constexpr std::uint32_t list_next = 0x10738e00, list_append = 0x10738d10, is_national_team = 0x100c7d00;
}

// 0x101f84e0: pending events of kind 0x1e (a team change), 0x82 (done) and the others are decided by chance and by the
// teams' standing; then each side's players are rated for the step and the result reported.
void update_contacts(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101f84e0, 0xe0, {.first_gpr = 13, .first_fpr = 30});
    Kept<30> match_kept(c, match);
    keep(c, {{22, c.toc(-0x7be0)}, {31, c.toc(-0x6428)}, {26, c.toc(-0x7be4)}, {21, c.toc(-0x7be8)}, {27, c.toc(-0x7b7c)},
             {28, c.toc(-0x7b54)}, {23, c.toc(-0x52c4)}});
    const auto table = c.r(23), sink = c.r(31), people = c.r(22), clubs = c.r(21);
    Fpu fp(c);
    const auto fixture_now = [&] { return c.u32(match + fixture_at); };
    const auto magic = [&] { const double value = c.f64(table + 0x60); c.set_f(1, value); return value; };
    // bl 0x101d3c50 after the call site set r3 and r4.
    const auto team = [&](std::uint32_t side, std::uint32_t back) {
        enter(c, back, {match, side});
        return fixture_team(c, match, side);
    };
    // bl 0x101f9440-style appends of an event record of 0x1c bytes to the sink.
    const auto append = [&](std::uint32_t item, std::uint32_t back) {
        c.set({{3, c.u32(sink)}, {4, item}, {5, 0x1c}});
        c.call(list_append, back);
    };
    const auto release = [&](std::uint32_t item) {
        c.put8(item + 0xe, 0);
        c.put32(item + 0x10, UINT32_MAX);
    };
    const auto flag_at = frame.at(0x3a);

    if (c.u32(match + 0x3114) != 0 && c.u32(sink) != 0 && fixture_now() != 0 && s8(c.u8(fixture_now() + 0x40)) != 0) {
        c.set(3, c.u32(match + 0x3114));
        reset_list_cursor(c, c.r(3));
        for (Kept<24> n(c, 0);; n = n + 1) {
            c.set(3, c.u32(match + 0x3114));
            halfword_0(c, c.r(3));
            if (!(s8(n) < std::int16_t(c.r(3)))) break;
            c.set(3, c.u32(match + 0x3114));
            call(c, list_next, 0x101f8568);
            Kept<25> item(c, c.r(3));
            if (item == 0) break;
            const std::uint32_t at = item;
            const auto kind = c.i16(at + 0x14);
            c.set(3, extended(kind));
            if (kind == 0x82) {
                append(at, 0x101f8590);
                continue;
            }
            if (kind == 0x1e) {
                keep(c, {{13, 0}});
                c.put8(flag_at, 0);
                // The pending event's side leads in goals, or at equal goals the other team's rating times a
                // factor (0.7 for a first-side event, 0.6 for a second-side event) exceeds this team's.
                const auto rated = [&](std::uint32_t other_side, std::uint32_t first_back, std::uint32_t second_back,
                                       std::uint32_t slot_a, std::uint32_t slot_b, std::uint32_t factor) {
                    const auto other_team = team(other_side, first_back);
                    const double magic_value = magic();
                    c.set_f(30, from_int(c, fp, frame.at(slot_a), u(c.i16(other_team + 0x1ec)), magic_value));
                    enter(c, second_back, {match, c.u8(at + 0xd)});
                    const auto this_team = fixture_team(c, match, c.u8(at + 0xd));
                    magic();
                    const double rating = from_int(c, fp, frame.at(slot_b), u(c.i16(this_team + 0x1ec)), magic_value);
                    c.set_f(1, rating);
                    fp.fcmp(0, fp.fmul(c.f64(table + factor), rating), c.f(30));
                    if (c.cr(0) & 4) c.put8(flag_at, 1);
                };
                if (s8(c.u8(at + 0xd)) != 0) {
                    keep(c, {{13, fixture_now()}});
                    const auto ahead = s8(c.u8(fixture_now() + 0x43)), behind = s8(c.u8(fixture_now() + 0x44));
                    c.set(3, u(ahead));
                    c.set(4, fixture_now());
                    if (ahead > behind) c.put8(flag_at, 1);
                    else if (behind == ahead) {
                        keep(c, {{13, u(s8(c.u8(at + 0xd)))}});
                        rated(u(s8(c.r(13)) == 0), 0x101f8620, 0x101f864c, 0x70, 0x68, 0x300);
                    }
                } else {
                    const auto first = s8(c.u8(fixture_now() + 0x44)), second = s8(c.u8(fixture_now() + 0x43));
                    c.set(4, u(first));
                    if (first > second) c.put8(flag_at, 1);
                    else {
                        keep(c, {{13, fixture_now()}});
                        c.set(6, fixture_now());
                        if (first == second) {
                            c.set(5, c.u8(at + 0xd));
                            rated(u(s8(c.r(5)) == 0), 0x101f8828, 0x101f8854, 0x68, 0x70, 0x308);
                        }
                    }
                }
                c.set(3, c.u8(flag_at));
                if (s8(c.u8(flag_at)) == 0) continue;
                if (s8(c.u8(at + 0xd)) != 0) {
                    // The side's record two steps down the second team's competition.
                    c.set(4, c.u32(fixture_now() + 0x20));
                    if (c.u32(c.r(4) + 0x94) == 0) continue;
                    keep(c, {{13, fixture_now()}});
                    if (c.u32(c.u32(c.u32(fixture_now() + 0x20) + 0x94) + 0x1c) == 0) continue;
                    c.set(4, c.u32(fixture_now() + 0x20));
                    keep(c, {{17, c.u32(c.r(4) + 0x94)}});
                    const std::uint32_t record = c.r(17);
                    keep(c, {{13, draw(c, 20, 0x101f86d4)}});
                    c.set(3, c.u32(record + 0x1c));
                    c.set(4, c.u8(c.r(3) + 0x37));
                    c.call(0x100d2f50, 0x101f86e8);
                    const auto reach = std::int32_t(draw(c, u(s8(c.r(3))), 0x101f86f4));
                    bool decided = false;
                    if (compare(c, reach, std::int32_t(c.r(13))) > 0) {
                        c.set(4, c.u32(record + 0x1c));
                        keep(c, {{13, c.u8(c.r(4) + 0x34)}});
                        if (compare(c, s8(c.r(13)), 5) > 0) {
                            c.put8(at + 0xe, 1);
                            c.put32(at + 0x10, c.u32(record));
                            decided = true;
                        }
                    }
                    if (!decided) {
                        const auto roll = draw(c, 1000, 0x101f8730);
                        c.set(4, roll);
                        c.set(5, c.u32(clubs));
                        keep(c, {{13, u(c.i16(c.r(5) + c.u32(at + 4) * 0x248 + 0x1ec)) + roll}});
                        const auto second_roll = draw(c, 1000, 0x101f8754);
                        c.set(5, c.r(13) - second_roll);
                        keep(c, {{13, c.u32(people)}});
                        c.set(4, c.u32(c.r(13) + c.u32(at) * 0x70 + 0x14));
                        if (!(compare(c, c.i16(c.r(4) + 0xa), std::int32_t(c.r(5))) > 0)) {
                            c.set(5, c.u32(people));
                            c.set(4, c.u32(c.r(5) + c.u32(at) * 0x70 + 0x14));
                            if (compare(c, c.i16(c.r(4) + 0xa), 0x1b58) <= 0) continue;
                        }
                        c.put8(at + 0xe, 0);
                        c.set(4, UINT32_MAX);
                        c.put32(at + 0x10, c.r(4));
                    }
                    append(at, 0x101f87bc);
                } else {
                    if (c.u32(c.u32(fixture_now() + 0x1c) + 0x94) == 0) continue;
                    if (c.u32(c.u32(c.u32(fixture_now() + 0x1c) + 0x94) + 0x1c) == 0) continue;
                    keep(c, {{16, c.u32(c.u32(fixture_now() + 0x1c) + 0x94)}});
                    const std::uint32_t record = c.r(16);
                    c.set(3, c.u32(record + 0x1c));
                    c.set(4, c.u8(c.r(3) + 0x37));
                    c.call(0x100d2f50, 0x101f88e4);
                    c.set(3, u(s8(c.r(3))));
                    const auto reach = std::int32_t(draw(c, c.r(3), 0x101f88ec));
                    if (compare(c, reach, 5) > 0 && compare(c, s8(c.u8(c.u32(record + 0x1c) + 0x34)), 5) > 0) {
                        c.put8(at + 0xe, 1);
                        c.put32(at + 0x10, c.u32(record));
                    } else {
                        c.set(5, c.u32(people));
                        const auto standing = std::int32_t(c.i16(c.u32(c.r(5) + c.u32(at) * 0x70 + 0x14) + 0xa));
                        c.set(5, c.u32(clubs));
                        c.set(4, c.u32(at + 4) * 0x248);
                        if (compare(c, standing, std::int32_t(c.i16(c.r(5) + c.r(4) + 0x1ec))) > 0 &&
                            compare(c, std::int32_t(draw(c, 4, 0x101f895c)), 0) == 0) {
                            release(at);
                        }
                    }
                    append(at, 0x101f8984);
                }
                continue;
            }
            // Any other kind: the event's record names the player it concerns.
            c.put32(frame.at(0x40), c.u32(people) + c.u32(at) * 0x70);
            c.set({{3, match}, {4, c.u32(frame.at(0x40))}, {5, 0}});
            call(c, 0x101da760, 0x101f89ac);
            c.put8(frame.at(0x39), c.r(3));
            if (s8(c.u8(frame.at(0x39))) != -1) c.put8(frame.at(0x38), 0);
            else {
                c.set({{3, match}, {4, c.u32(frame.at(0x40))}, {5, 1}});
                call(c, 0x101da760, 0x101f89dc);
                c.put8(frame.at(0x39), c.r(3));
                if (s8(c.u8(frame.at(0x39))) == -1) continue;
                c.put8(frame.at(0x38), 1);
            }
            c.set({{3, match}, {4, c.u8(frame.at(0x38))}, {5, c.u8(frame.at(0x39))}});
            keep(c, {{18, player_at(c, match, c.r(4), c.r(5))}});
            if (c.r(18) == 0) continue;
            const std::uint32_t player = c.r(18);
            c.set(4, extended(c.i16(at + 0x14)));
            if (c.i16(at + 0x14) == 6) {
                if (s8(c.u8(player + 0x24)) != 2 && s8(c.u8(player + 0x23)) == 0) continue;
                release(at);
                append(at, 0x101f8a60);
                continue;
            }
            if (s8(c.u8(player + 0x1b)) >= 7) continue;
            c.set(3, u(s8(c.u8(player + 0x17))));
            if (compare(c, std::int32_t(draw(c, c.r(3), 0x101f8a80)), 0) != 0) continue;
            c.put8(at + 0xe, 0);
            keep(c, {{13, UINT32_MAX}});
            c.put32(at + 0x10, c.r(13));
            append(at, 0x101f8aa8);
        }
    }

    // Each side's players are rated for the step.
    for (Kept<24> side(c, 0); compare(c, s8(side), 2) < 0; side = side + 1) {
        c.put32(frame.at(0x44), 0);
        c.put32(frame.at(0x48), 0);
        c.put8(frame.at(0x3b), 0);
        const auto other_index = [&] { return u(s8(s8(side) == 0)); };
        // bl 0x101d3c50 for the other side. Some of the original's sequences leave r5 (the count of leading
        // zeros) and r13 (the side as a signed byte) behind.
        const auto other_team = [&](std::uint32_t back, bool leading_zeros = false, bool side_in_r13 = false) {
            if (side_in_r13) c.set(13, u(s8(side)));
            if (leading_zeros) c.set(5, std::uint32_t(std::countl_zero(std::uint32_t(s8(side)))));
            return team(other_index(), back);
        };
        const auto own_team = [&](std::uint32_t back) { return team(side, back); };
        while (compare(c, s8(c.u8(frame.at(0x3b))), 0x14) < 0) {
            c.set(3, match);
            players(c, match);
            c.set(4, u(s8(side) * 0x2350));
            const std::int32_t index = s8(c.u8(frame.at(0x3b)));
            keep(c, {{29, c.r(3) + c.r(4) + u(index * 0x1c4)}});
            const std::uint32_t player = c.r(29);
            const auto record = [&] { return c.u32(player + 0x6c); };
            const auto rec_byte = [&](std::uint32_t at) { return s8(c.u8(record() + at)); };
            // An event record of 0x1c bytes at sp+0x4c, appended to the sink.
            const auto raise = [&](std::uint32_t kind, std::uint32_t flags, std::uint32_t back_team, std::uint32_t back) {
                c.put8(frame.at(0x5a), 0);
                c.put32(frame.at(0x5c), UINT32_MAX);
                c.put32(frame.at(0x50), c.u32(own_team(back_team)));
                c.put8(frame.at(0x58), 0);
                c.put32(frame.at(0x4c), c.u32(record()));
                c.put8(frame.at(0x59), c.r(24));
                c.put16(frame.at(0x60), kind);
                c.put32(frame.at(0x54), flags);
                c.set({{3, c.u32(sink)}, {4, frame.at(0x4c)}, {5, 0x1c}});
                c.call(list_append, back);
            };
            // The first event (kind 10): a promising player of a team that is not his own competition's.
            const auto first_event = [&] {
                if (c.u32(frame.at(0x3c)) == 0) return;
                if (own_team(0x101f8c50) == 0) return;
                if (c.u32(own_team(0x101f8c64) + 0x90) == record()) return;
                if (c.u32(fixture_now() + 0x14) == 0) return;
                c.set({{3, c.u32(c.r(28))}, {4, c.u32(fixture_now() + 0x14)}});
                c.call(0x10494d20, 0x101f8c94);
                if (s8(c.r(3)) != 0) return;
                if (s8(c.u8(player + 0x20)) == -1) return;
                if (s8(c.u8(player + 0x22)) != -1) return;
                c.set(3, c.u32(frame.at(0x3c)));
                byte_44(c, c.r(3));
                if (s8(c.r(3)) <= 0) return;
                if (c.i16(c.u32(player + 0x70) + 0xa) <= 0x109a) return;
                c.set(3, match);
                if (fixture(c, match) == 0) return;
                c.set(3, match);
                if (s8(c.u8(fixture(c, match) + 0x40)) == 0) return;
                keep(c, {{13, draw(c, 100, 0x101f8d14)}});
                c.set(3, player);
                fitness_1a8(c, player);
                c.put64(frame.at(0x68), fp.fctiwz(0, c.f(1)));
                if (compare(c, std::int32_t(draw(c, c.u32(frame.at(0x6c)), 0x101f8d30)), std::int32_t(c.r(13))) <= 0) return;
                keep(c, {{13, draw(c, 10, 0x101f8d40)}});
                if (compare(c, std::int32_t(draw(c, u(rec_byte(0x69)), 0x101f8d54)), std::int32_t(c.r(13))) >= 0) return;
                const auto rise = draw(c, 0x5dc, 0x101f8d64);
                c.set(4, rise);
                c.set_f(30, from_int_single(c, fp, frame.at(0x68), rise + 0x1d4c, magic()));
                c.set(3, player);
                fitness(c, player);
                fp.fcmp(0, c.f(1), c.f(30));
                if (!(c.cr(0) & 4)) return;
                if (rec_byte(0x67) >= 6) return;
                if (rec_byte(0x63) <= 0xa) return;
                keep(c, {{13, draw(c, u(rec_byte(0x63)), 0x101f8dd0)}});
                if (compare(c, std::int32_t(draw(c, u(rec_byte(0x67)), 0x101f8de4)), std::int32_t(c.r(13))) >= 0) return;
                raise(0xa, 0x4000000, 0x101f8e0c, 0x101f8e50);
            };
            // The player's report and the events the clubs' standing can raise; any failed test ends it.
            const auto report = [&] {
                if (record() == 0 || c.u32(player + 0x70) == 0) return;
                keep(c, {{14, record()}});
                if (c.r(14) != 0) {
                    keep(c, {{13, record()}});
                    if (!(compare(c, std::int32_t(c.u32(c.r(13))), std::int32_t(c.u32(c.r(26)))) < 0)) return;
                }
                c.set(3, match);
                keep(c, {{15, fixture(c, match)}});
                c.set(3, match);
                keep(c, {{13, other_team(0x101f8b84)}});
                c.set(5, own_team(0x101f8b94));
                c.set({{3, player}, {4, record()}, {6, c.r(13)}, {7, c.r(15)}});
                c.set_f(1, from_int_single(c, fp, frame.at(0x68), u(s8(c.u8(player + 0x1b))), magic()));
                c.set({{9, c.u8(match)}, {10, 1}});
                call(c, 0x101f9470, 0x101f8bd8);
                if (c.u32(sink) == 0) return;
                c.set(3, own_team(0x101f8bf0));
                c.call(is_national_team, 0x101f8bf4);
                if (s8(c.r(3)) != 0) return;
                c.set(3, match);
                if (s8(c.u8(fixture(c, match) + 0x40)) == 0) return;
                c.set(5, own_team(0x101f8c24));
                c.set({{3, c.u32(c.r(27))}, {4, record()}});
                c.call(0x10082f90, 0x101f8c34);
                c.put32(frame.at(0x3c), c.r(3));
                first_event();
                // The morale of the player: the clubs of the same town, rivals and relations move it by tens.
                keep(c, {{20, u(-std::int32_t(c.i16(player + 0x32)))}});
                const auto adjust = [&](std::int32_t by) { keep(c, {{20, c.r(20) + u(by)}}); };
                keep(c, {{13, other_team(0x101f8e70)}});
                if (c.u32(own_team(0x101f8e80) + 0x74) == c.r(13)) adjust(10);
                else {
                    keep(c, {{13, other_team(0x101f8ea4)}});
                    const auto own = own_team(0x101f8eb4);
                    c.set(4, own);
                    if (c.u32(own + 0x78) == c.r(13)) adjust(10);
                    else {
                        keep(c, {{13, other_team(0x101f8edc)}});
                        if (c.u32(own_team(0x101f8eec) + 0x7c) == c.r(13)) adjust(10);
                    }
                }
                keep(c, {{13, other_team(0x101f8f14, true)}});
                c.set(3, player);
                word_6c(c, player);
                c.set(4, c.r(13));
                c.call(0x100d94d0, 0x101f8f28);
                if (s8(c.r(3)) != 0) adjust(10);
                else {
                    keep(c, {{13, other_team(0x101f8f54, true, true)}});
                    c.set(3, player);
                    word_6c(c, player);
                    c.set(4, c.r(13));
                    c.call(0x100d9480, 0x101f8f68);
                    if (s8(c.r(3)) != 0) adjust(-10);
                }
                c.set(4, other_team(0x101f8f90, true));
                c.set(3, record());
                c.call(0x100da050, 0x101f8f9c);
                if (s8(c.r(3)) != 0) adjust(10);
                else {
                    c.set(4, other_team(0x101f8fc8));
                    c.set(3, record());
                    c.call(0x100da0b0, 0x101f8fd4);
                    if (s8(c.r(3)) != 0) adjust(-0x14);
                }
                c.set(4, c.u8(record() + 0x67));
                if (rec_byte(0x67) >= 6) return;
                if (rec_byte(0x69) >= 0xa) return;
                keep(c, {{13, draw(c, c.r(20) + 0x32, 0x101f9014)}});
                c.set(6, u(rec_byte(0x69)));
                c.set(5, u(rec_byte(0x66)));
                c.set(4, record());
                const auto weight = std::int32_t(rec_byte(0x67) * 100);
                const auto bound = u(rec_byte(0x69) * (rec_byte(0x66) * weight) + 0x7d0);
                if (compare(c, std::int32_t(draw(c, bound, 0x101f904c)), std::int32_t(c.r(13))) >= 0) return;
                raise(0xb, 0, 0x101f9074, 0x101f90b4);
            };
            bool counted = true;
            if (index > 10) {
                keep(c, {{15, c.u8(player + 0x1f)}});
                counted = s8(c.r(15)) != -1;
            }
            if (counted) {
                report();
                // The player's share of the step: 90 for the first eleven of a match with the flag, else the minutes
                // he is down to play (his entry and exit minutes, or the clock's).
                std::int32_t share;
                const auto in_first_eleven = s8(c.u8(frame.at(0x3b))) < 0xb;
                // The clock as a minute: halfword, mulhw by 0x2e8ba2e9, shifted.
                const auto clock_minute = [&](bool as_halfword_r4) {
                    c.set(3, match);
                    match_clock(c, match);
                    const auto clock = std::int32_t(std::int16_t(c.r(3)));
                    if (as_halfword_r4) c.set(4, u(clock));
                    const auto shifted = std::int32_t(std::int64_t(0x2e8ba2e9) * clock >> 32) >> 1;
                    c.set(3, std::uint32_t(shifted) >> 31);
                    return shifted + std::int32_t(std::uint32_t(shifted) >> 31);
                };
                if (s8(c.u8(match)) != 0 && in_first_eleven) share = 0x5a;
                else if (in_first_eleven) {
                    c.set(4, c.u8(player + 0x21));
                    c.set(3, s8(c.r(4)));
                    if (s8(c.r(4)) > -1) { c.set(3, c.u8(player + 0x21)); share = s8(c.r(3)); }
                    else {
                        c.set(3, u(s8(c.u8(player + 0x20))));
                        if (s8(c.r(3)) > -1) share = s8(c.u8(player + 0x20));
                        else {
                            const auto minute = clock_minute(true);
                            c.set(5, u(minute));
                            share = s8(c.r(5));
                        }
                    }
                } else {
                    c.set(3, c.u8(player + 0x1f));
                    if (s8(c.r(3)) == -1) share = 0;
                    else {
                        c.set(3, c.u8(player + 0x21));
                        c.set(5, u(s8(c.r(3))));
                        if (s8(c.r(5)) > -1) {
                            c.set(4, c.u8(player + 0x21));
                            share = s8(c.u8(player + 0x21) - c.u8(player + 0x1f));
                        } else {
                            c.set(3, u(s8(c.u8(player + 0x20))));
                            if (s8(c.r(3)) > -1) {
                                c.set(3, c.u8(player + 0x20) - c.u8(player + 0x1f));
                                share = s8(c.r(3));
                            } else {
                                const auto minute = clock_minute(false);
                                c.set(3, u(minute) - c.u8(player + 0x1f));
                                share = s8(c.r(3));
                            }
                        }
                    }
                }
                keep(c, {{19, u(share)}});
                if (share != 0) {
                    c.put32(frame.at(0x44), c.u32(frame.at(0x44)) + u(share));
                    c.set(4, c.u32(frame.at(0x48)));
                    c.set(3, u(s8(c.u8(player + 0x1b))));
                    c.put32(frame.at(0x48), c.r(4) + u(share * s8(c.u8(player + 0x1b))));
                }
            }
            c.set(3, c.u8(frame.at(0x3b)) + 1u);
            c.put8(frame.at(0x3b), c.r(3));
        }
        if (c.u32(frame.at(0x44)) != 0) {
            c.set(13, c.u32(own_team(0x101f924c) + 0x3c));
            c.set(3, match);
            const auto earlier = c.u32(fixture(c, match) + 0x18) == c.r(13);
            const double rated = c.f64(table + 0x168);
            const auto ratio = [&] {
                magic();
                const double total = from_int(c, fp, frame.at(0x68), c.u32(frame.at(0x48)), c.f64(table + 0x60));
                magic();
                const double weight = from_int(c, fp, frame.at(0x70), c.u32(frame.at(0x44)), c.f64(table + 0x60));
                return fp.fdiv(total, weight);
            };
            if (earlier) {
                const double quotient = ratio();
                c.set_f(1, quotient);
                c.set_f(31, fp.fsub(quotient, c.f64(table + 0x1c0)));
            } else {
                c.set_f(31, fp.fadd(rated, ratio()));
            }
        }
        c.set(3, match);
        keep(c, {{15, fixture(c, match)}});
        c.set(3, match);
        keep(c, {{14, other_team(0x101f9324)}});
        keep(c, {{13, own_team(0x101f9334)}});
        c.set({{3, match}, {4, side}});
        side_stats(c, match, side);
        c.set({{4, c.r(13)}, {5, c.r(14)}, {6, c.r(15)}});
        c.set_f(1, fp.frsp(c.f(31)));
        c.set({{8, c.u8(match)}, {9, 1}});
        call(c, 0x101fb580, 0x101f9360);
        c.set(3, own_team(0x101f936c));
        c.call(is_national_team, 0x101f9378);
        keep(c, {{15, c.r(3)}});
        if (s8(c.r(15)) == 0) {
            c.set({{3, match}, {4, side}});
            c.set_f(1, fp.frsp(c.f(31)));
            c.set({{6, c.u8(match)}, {7, 1}});
            call(c, 0x101fc670, 0x101f93a0);
        }
    }
}
}
