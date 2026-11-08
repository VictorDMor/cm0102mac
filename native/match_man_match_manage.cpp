// MATCH_MAN.CPP match block (match_man_match.hpp): 0x1020ef70, the automatic substitutions.
#include "match_man_adapter.hpp"
#include "match_man_match.hpp"
#include "match_man_match_detail.hpp"
#include <algorithm>

namespace cm::aot::match_man {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Fpu;
using guest::Frame;
using guest::Kept;
using guest::nonzero;
using guest::nonzero_logical;
constexpr std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }

// Stack: the players considered (pointers) at 0x238, their persons at 0x1e8, the chosen
// persons 0x198, players coming on 0x148 and going off 0xf8 (pointers), categories 0xd0 and
// roles 0xa8 (halfwords), positions 0x80 and forms 0x94 (bytes), ratings 0x288 (8 bytes each).
namespace local {
constexpr std::uint32_t considered = 0x238, persons = 0x1e8, chosen = 0x198, coming = 0x148, going = 0xf8,
                        categories = 0xd0, roles = 0xa8, positions = 0x80, forms = 0x94, ratings = 0x288;
}
}

// 0x1020ef70. Frame 0x390 saving r13-r31. r15 the side, r16 the flag (then a count), r25 the
// seed word, r26 the cursor's restart, r27 the limit word's address, r28 the cursor, r29 the
// pool of doubles, r30 the strings, r13 the message buffer.
void manage_substitutions(Cpu& c, std::uint32_t side, std::uint32_t flag) {
    constexpr std::uint32_t entry = 0x1020ef70;
    using namespace lineup_entries;
    c.set(0, c.lr());
    Frame frame(c, entry, 0x390, {.first_gpr = 13});
    Fpu fpu(c);
    const RandomStream stream{c.toc(-0x7b84), c.toc(-0x7b90), c.toc(-0x7b88), c.toc(-0x7b8c)};
    Kept<15> r15(c, side);
    Kept<25> r25(c, stream.seed_slot);
    Kept<26> r26(c, stream.restart);
    Kept<16> r16(c, flag);
    Kept<27> r27(c, stream.limit_address);
    Kept<18> r18(c, 0);
    Kept<28> r28(c, stream.cursor_slot);
    Kept<17> r17(c, 0);
    Kept<29> r29(c, c.toc(-0x5278));
    Kept<13> r13(c, c.toc(-0x7ba8));
    Kept<30> r30(c, c.toc(-0x5274));
    Kept<14> r14(c, c.r(14));
    Kept<19> r19(c, c.u8(side + lineup_side::substitutes));
    Kept<20> r20(c, c.r(20));
    Kept<21> r21(c, c.r(21));
    Kept<22> r22(c, c.r(22));
    Kept<23> r23(c, c.r(23));
    Kept<24> r24(c, c.r(24));
    Kept<31> r31(c, c.r(31));
    const auto at = [&](std::uint32_t offset) { return frame.at(std::int32_t(offset)); };
    const auto pool_address = c.toc(-0x5278);
    const auto match = [&] { return c.u32(side + lineup_side::match); };
    const auto club = [&] { return c.u32(side + lineup_side::club); };
    const auto tactics = [&] { return c.u32(side + lineup_side::tactics); };

    c.set(3, r19);
    if (!nonzero(c, s8(r19))) return;  // nothing left to change (epilogue only)

    // The players considered: on the pitch, or sent off without the flag.
    for (unsigned word = 0; word < 20; ++word) c.put32(at(local::considered) + word * 4, 0);
    std::uint32_t considered = 0;
    auto player = c.u32(side + lineup_side::players);
    for (unsigned n = 0; n < 20; ++n, player += match_player::size) {
        const auto position = s8(c.u8(player + lineup_player::position));
        if (position < 0 && !(position == -2 && s8(c.u8(player + lineup_player::off_flag)) == 0)) continue;
        c.put32(at(local::considered) + std::uint32_t(s8(considered)) * 4, player);
        ++considered;
    }
    r14 = considered;
    // The clearing loop leaves r7-r10 (the sort reads them).
    c.set({{7, 0}, {8, 0x50}, {9, at(local::considered) + 0x3c}, {10, 20}});
    c.set({{3, at(local::considered)}, {4, std::uint32_t(s8(considered))}, {5, 4}});
    // Sorted by the comparators 0x1020ea00 or 0x1020ee00 (through the original's TOC words).
    c.set(6, c.toc(s8(considered) < 11 ? -0x63fc : -0x6400));
    c.call(0x10768300, s8(considered) < 11 ? 0x1020f084 : 0x1020f098);
    if (compare(c, s8(r14), s8(r19)) < 0) r19 = r14;

    // Categories, roles, positions, ratings and forms of those that can change.
    r20 = 0;
    r14 = std::uint32_t(s8(r19));
    r24 = 0;
    r31 = at(local::considered);
    r23 = 0;
    for (; compare(c, s8(r20), s8(r14)) < 0; r20 = r20 + 1, r31 = r31 + 4, r24 = r24 + 2, r23 = r23 + 8) {
        const auto record = c.u32(r31);
        c.set(3, record);
        if (s8(c.u8(record + lineup_player::position)) == -2) {
            c.call(benched_category, 0x1020f0e0);
            c.put16(at(local::categories) + r24, c.r(3));
            c.put16(at(local::roles) + r24, 0);
        } else {
            c.call(player_category, 0x1020f0fc);
            c.put16(at(local::categories) + r24, c.r(3));
            c.set(3, c.u32(r31));
            c.call(player_role, 0x1020f10c);
            c.put16(at(local::roles) + r24, c.r(3));
        }
        if (!nonzero(c, s8(r20)) && nonzero(c, s8(r16))) {
            // The first slot of a flagged call takes position 0.
            c.put8(at(local::positions) + std::uint32_t(s8(r20)), 0);
        } else {
            const auto who = c.u32(r31);
            const auto position = c.u8(who + lineup_player::position);
            compare(c, s8(position), -2);
            c.put8(at(local::positions) + std::uint32_t(s8(r20)),
                   s8(position) == -2 ? c.u8(who + lineup_player::role) : position);
        }
        r21 = std::uint32_t(s8(r20));
        r22 = std::uint32_t(s8(c.u8(at(local::positions) + r21)));
        c.set({{3, tactics()}, {4, r22}});
        c.call(person_rating, 0x1020f188);
        c.put32(at(local::ratings) + r23 + 4, c.r(4));
        c.put32(at(local::ratings) + r23, c.r(3));
        c.set({{4, r22}, {3, tactics()}});
        c.call(person_form, 0x1020f1a4);
        c.put8(at(local::forms) + r21, c.r(3));
    }

    // The bench: players still at position -1, within the bench size.
    r16 = r19;
    auto bench = c.u32(side + lineup_side::players) + 0x136c;
    for (unsigned n = 11;; ++n, bench += match_player::size) {
        const auto size = s8((c.u8(c.u32(side + lineup_side::extras) + lineup_side::extras_bench) >> 4) & 0xf);
        if (!(s8(n) < size + 11)) break;
        if (nonzero_logical(c, bench) && s8(c.u8(bench + lineup_player::position)) == -1) {
            c.put32(at(local::considered) + std::uint32_t(s8(r16)) * 4, bench);
            r16 = r16 + 1;
        }
    }
    // Their persons.
    c.set(3, 0);
    if (compare(c, s8(r16)) > 0)
        for (std::int32_t i = 0; i < s8(r16); ++i)
            c.put32(at(local::persons) + std::uint32_t(i) * 4, c.u32(c.u32(at(local::considered) + std::uint32_t(i) * 4) + match_player::person));
    // Past 8 persons the original copies them 8 at a time (0x1020f25c); the last unrolled block
    // leaves r8-r11 holding records 6, 5, 4 and 3 of its eight (these reach the later calls).
    if (s8(r16) > 8) {
        const std::int32_t limit = s8(std::uint32_t(s8(r16) - 8));
        std::int32_t base = 0;
        while (s8(std::uint32_t(base + 8)) < limit) base += 8;
        const auto record = [&](std::int32_t i) { return c.u32(at(local::considered) + std::uint32_t(base + i) * 4); };
        if (0 < limit) c.set({{8, record(6)}, {9, record(5)}, {10, record(4)}, {11, record(3)}});
    }

    // The side's flags for the assignment.
    c.set(3, club());
    c.call(competition_check, 0x1020f32c);
    compare(c, s8(c.r(3)));
    std::uint32_t flags = s8(c.r(3)) == 0 ? 0x8a0cb : 0x208a09d;
    r20 = flags;
    const auto extras = [&] { return c.u32(side + lineup_side::extras); };
    if (nonzero(c, c.i8(side + lineup_side::kind))) {
        r20 = ((flags | 0x20000) | 0x100) ^ 0x80000;
    } else {
        flags = r20;
        bool done = false;
        const auto mode = s8(c.u8(extras() + lineup_side::extras_mode));
        if (mode == 1 || mode == 9) {
            advance_stream(c, stream, 0x1020f3a0);
            const auto x1 = stream.draw(c);
            const auto old_cursor = c.u32(stream.cursor_slot);
            c.set(7, old_cursor);
            advance_stream(c, stream, 0x1020f3f8);
            const std::int32_t noise = -(std::int32_t(x1) % 4000);  // signed, as the mulhw division
            r14 = std::uint32_t(noise);  // neg r14: kept across the calls below
            const auto x2 = stream.draw(c);
            const auto rating = std::int32_t(c.i16(club() + 0x1ec) - c.i16(c.u32(side + lineup_side::opponent) + 0x1ec));
            if (compare(c, rating, noise + std::int32_t(x2) % 2000) > 0) { r20 = std::uint32_t(r20) | 0x100; done = true; }
        }
        if (!done) {
            advance_stream(c, stream, 0x1020f478);
            const auto x = stream.draw(c);
            const auto rating = std::int32_t(c.i16(club() + 0x1ec) - c.i16(c.u32(side + lineup_side::opponent) + 0x1ec));
            c.set(3, club());
            if (compare(c, rating, std::int32_t(x) % 3000 + 3000) > 0) {
                r20 = std::uint32_t(r20) | 0x100;
                c.call(competition_check, 0x1020f4d8);
                if (!nonzero(c, s8(c.r(3)))) r20 = std::uint32_t(r20) | 0x10000;
            } else {
                bool toggle = false;
                bool second = false;
                if (c.u8(side + lineup_side::mode) != 3) {
                    toggle = true;
                } else {
                    const auto word = c.i32(extras());
                    if (compare(c, word, -1) > 0) {
                        c.set({{5, c.toc(-0x7b5c)}, {4, c.r(3)}});
                        c.set(3, c.u32(c.u32(c.toc(-0x7b5c)) + std::uint32_t(word) * 4));
                        c.call(0x10452ed0, 0x1020f51c);
                        if (nonzero(c, s8(c.r(3)))) second = true; else toggle = true;
                    } else second = true;
                }
                if (toggle) {
                    r20 = (std::uint32_t(r20) | 0x100) ^ 0x80000;
                } else if (second) {
                    c.set(3, club());
                    c.call(competition_check, 0x1020f538);
                    if (!nonzero(c, s8(c.r(3)))) r20 = std::uint32_t(r20) | 0x10000;
                }
            }
        }
    }
    if (nonzero(c, c.i8(side + lineup_side::manager_flag))) r20 = std::uint32_t(r20) | 0x800000;
    c.set({{3, club()}, {4, 1}, {5, 1}, {6, 0}, {7, 0}});
    c.call(squad_check, 0x1020f56c);
    if (nonzero(c, s8(c.r(3))) && !nonzero(c, c.i8(side + lineup_side::kind)) &&
        nonzero_logical(c, c.u32(club() + 0x90))) {
        c.set(4, c.u32(club() + 0x90));  // the manager tested stays in r4 for the call
        c.set(3, c.u32(c.toc(-0x7a54)));
        c.call(registry_check, 0x1020f59c);
        if (nonzero(c, s8(c.r(3)))) {
            c.set({{5, c.u32(club() + 0x90)}, {4, 3}, {3, c.u32(c.toc(-0x7a54))}});
            c.call(registry_member, 0x1020f5bc);
            if (nonzero(c, s8(c.r(3)))) r20 = std::uint32_t(r20) | 0x400000;
        }
    }
    c.put32(at(0x38), side);
    c.put32(at(0x3c), c.u32(side + lineup_side::manager));
    c.put32(at(0x40), 0);
    c.put32(at(0x44), at(local::ratings));
    c.put32(at(0x48), at(local::forms));
    c.put32(at(0x4c), c.u32(tactics() + 0x588));
    c.put32(at(0x50), at(local::roles));
    c.set({{3, at(local::persons)}, {4, r16}, {5, at(local::categories)}, {6, r19}, {7, 0}, {8, at(local::chosen)},
           {9, r20}, {10, club()}});
    call(c, assign_positions, 0x1020f624);

    // Compare the chosen players with the ones on the pitch.
    r14 = std::uint32_t(s8(r16));
    r22 = at(local::persons);
    r16 = 0;
    const auto cap_minutes = [&](std::uint32_t record, std::uint32_t field) {
        const auto m = match();
        c.put8(record + field, std::uint32_t(c.i16(m + lineup_match::minutes) / 11));  // mulhw 0x2e8ba2e9: /11
        std::int32_t limit = 0;
        switch (c.i16(match() + lineup_match::competition)) {
            case 0x1ef: limit = 0x2d; break;
            case 0x3de: limit = 0x5a; break;
            case 0x483: limit = 0x69; break;
            case 0x528: limit = 0x78; break;
            default: return;
        }
        const auto value = s8(c.u8(record + field));
        c.put8(record + field, std::uint32_t(value >= limit ? limit : value));
    };
    for (; compare(c, s8(r16), s8(r14)) < 0; r22 = r22 + 4, r16 = r16 + 1) {
        c.set({{4, c.u32(r22)}, {3, side}});
        call(c, find_player, 0x1020f640);
        r20 = c.r(3);
        if (!nonzero_logical(c, r20)) {
            const auto strings = c.toc(-0x5274);
            guest::report_assertion(c, {.at = 0x1020f65c, .file = strings, .format = strings + 0xe, .title = strings + 0x1b,
                                        .name = at(0x60), .extension = at(0x58), .line = 0xda3});
            continue;
        }
        r21 = 0xffffffff;
        std::uint32_t j = 0;
        const auto limit = s8(r19);
        c.set({{4, std::uint32_t(limit)}, {3, at(local::chosen)}, {7, 0}, {6, 0}});
        for (; compare(c, s8(j), limit) < 0 && s8(r21) == -1; ++j) {
            const auto entry_value = c.u32(at(local::chosen) + j * 4);
            c.set({{5, entry_value}, {3, at(local::chosen) + j * 4}, {6, j * 4}, {7, j}});
            if (entry_value == c.u32(r20 + match_player::person)) r21 = j;
            c.set({{3, at(local::chosen) + j * 4 + 4}, {6, j * 4 + 4}, {7, j + 1}});
        }
        if (compare(c, s8(r21), -1) > 0) {
            if (s8(c.u8(r20 + lineup_player::position)) == -1) {
                c.put8(side + lineup_side::substitutes, c.u8(side + lineup_side::substitutes) - 1);
                c.set({{3, match()}, {4, r20}});
                c.call(0x101f1880, 0x1020f728);
                cap_minutes(r20, lineup_player::came_on);
                c.put32(at(local::coming) + std::uint32_t(s8(r18)) * 4, r20);
                r18 = r18 + 1;
            }
            const auto position = c.u8(r20 + lineup_player::position);
            if (s8(position) >= 0 || s8(position) == -1)
                c.put8(r20 + lineup_player::position, c.u8(at(local::positions) + std::uint32_t(s8(r21))));
        } else if (s8(c.u8(r20 + lineup_player::position)) != -1) {
            c.set({{3, match()}, {4, r20}, {5, 0xffffffff}});
            c.call(0x101f1950, 0x1020f860);
            c.put8(r20 + lineup_player::position, 0xfc);
            c.put8(r20 + lineup_player::off_flag, 1);
            cap_minutes(r20, lineup_player::went_off);
            c.put32(at(local::going) + std::uint32_t(s8(r17)) * 4, r20);
            r17 = r17 + 1;
        }
    }
    c.set({{3, match()}, {4, c.u8(side + lineup_side::index)}});
    c.call(0x101f1de0, 0x1020f97c);
    c.set({{3, match()}, {4, c.u8(side + lineup_side::index)}});
    c.call(0x101f1e10, 0x1020f988);

    const auto announce = [&](std::uint32_t code, std::uint32_t back) {
        const auto m = match();
        c.set({{5, m}, {4, code}, {7, std::uint32_t(s8(c.u8(side + lineup_side::index)))}, {6, 0xffffffff},
               {8, 0xffffffff}, {9, 0xffffffff}, {10, 0}});
        c.set(3, c.u32(m + lineup_match::events));
        c.call(events, back);
    };
    compare(c, s8(r18), 2);
    if (s8(r18) > 2) announce(0x1f59, 0x1020f9bc);
    else if (s8(r18) == 2) announce(0x1f5c, 0x1020f9ec);
    else {
        if (!nonzero(c, s8(r18))) return;
        announce(0x1f5a, 0x1020fa20);
    }

    // Each substitution: the injuries or dismissals the match rolls for the two players.
    r13 = 0;
    r16 = std::uint32_t(s8(r18));
    r18 = 0;
    r14 = at(local::going);
    const auto event_for = [&](std::uint32_t who, std::uint32_t back) {
        const auto m = match();
        const std::uint32_t flag_byte = c.u8(who + lineup_player::gone);
        c.set({{5, m}, {4, 0x211d}, {6, 0xffffffff}, {8, 0xffffffff}, {9, 0xffffffff}, {10, 0}});
        c.set(3, c.u32(m + lineup_match::events));
        c.set(7, std::uint32_t(std::int32_t(s8(flag_byte) == 0 ? 1 : 0)));
        c.call(events, back);
    };
    const auto event_second = [&](std::uint32_t who, std::uint32_t back) {
        const auto m = match();
        c.set({{5, m}, {4, 0x211e}, {6, 0xffffffff}, {8, 0xffffffff}, {9, 0xffffffff}, {10, 0}});
        c.set(3, c.u32(m + lineup_match::events));
        c.set(7, c.u8(who + lineup_player::gone));
        c.call(events, back);
    };
    // Whether the draw `n` (a spread) beats a roll over 30 (the ff3c / 0x1021013c tail).
    const auto tail = [&](std::uint32_t reseed_back) {
        advance_stream(c, stream, reseed_back);
        const auto x = stream.draw(c);
        return std::int32_t(x) % 30;
    };
    for (; compare(c, s8(r13), s8(r16)) < 0; r18 = r18 + 4, r14 = r14 + 4, r13 = r13 + 1) {
        const auto events_on = [&] { return nonzero(c, c.i8(match() + lineup_match::events_flag)); };
        if (events_on()) {
            r19 = c.u32(at(local::going) + r18);
            r17 = std::uint32_t(c.i16(r19 + lineup_player::form));
            if (!nonzero(c, std::int32_t(std::uint32_t(r17)))) r20 = 0;
            else {
                advance_stream(c, stream, 0x1020fa8c);
                random_spread(c, fpu, stream, std::int32_t(std::uint32_t(r17)), pool_address,
                              {at(0x338), at(0x330), at(0x328), 0x1020fb54, false, 20});
            }
            if (compare(c, std::int32_t(std::uint32_t(r20)), -10) < 0) { event_for(r19, 0x1020fbf0); goto after_pairs; }
        }
        if (events_on()) {
            r19 = c.u32(at(local::coming) + r18);
            r17 = std::uint32_t(c.i16(r19 + lineup_player::form));
            if (!nonzero(c, std::int32_t(std::uint32_t(r17)))) r20 = 0;
            else {
                advance_stream(c, stream, 0x1020fc40);
                random_spread(c, fpu, stream, std::int32_t(std::uint32_t(r17)), pool_address,
                              {at(0x328), at(0x330), at(0x338), 0x1020fd08, false, 20});
            }
            if (compare(c, std::int32_t(std::uint32_t(r20)), -10) < 0) event_for(r19, 0x1020fda4);
        }
    after_pairs:
        if (events_on()) {
            r19 = c.u32(at(local::going) + r18);
            const auto fit = c.i16(r19 + lineup_player::morale), fitness = c.i16(r19 + lineup_player::fitness);
            r17 = std::uint32_t(fit + (fitness / 30 - 0xe9));  // mulhw 0x88888889: /30
            if (!nonzero(c, std::int32_t(std::uint32_t(r17)))) r20 = 0;
            else {
                advance_stream(c, stream, 0x1020fe14);
                random_spread(c, fpu, stream, std::int32_t(std::uint32_t(r17)), pool_address,
                              {at(0x328), at(0x330), at(0x338), 0x1020fedc, false, 20});
            }
            const auto rest = tail(0x1020ff5c);
            if (compare(c, std::int32_t(std::uint32_t(r20)), rest) > 0) {
                event_second(r19, 0x1020ffc4);
                goto pair_done;
            }
        }
        if (events_on()) {
            r19 = c.u32(at(local::coming) + r18);
            r17 = std::uint32_t(c.i16(r19 + lineup_player::morale));
            if (!nonzero(c, std::int32_t(std::uint32_t(r17)))) r20 = 0;
            else {
                advance_stream(c, stream, 0x10210014);
                random_spread(c, fpu, stream, std::int32_t(std::uint32_t(r17)), pool_address,
                              {at(0x328), at(0x330), at(0x338), 0x102100dc, false, 20});
            }
            const auto rest = tail(0x1021015c);
            if (compare(c, std::int32_t(std::uint32_t(r20)), rest) > 0) event_second(r19, 0x102101c4);
        }
    pair_done:
        // The substitution itself is announced: who went off and who came on.
        const auto off = c.u32(r14);
        const bool known = s8(c.u8(off + lineup_player::substituted)) > -1;
        r22 = c.u32(at(local::coming) + r18);
        r19 = c.u8(off + lineup_player::temper);
        r20 = c.u8(r22 + lineup_player::temper);
        r21 = c.u32(match() + lineup_match::events);
        c.set(3, off);
        c.call(0x10283690, known ? 0x102101f4 : 0x10210244);
        r17 = c.r(3);
        c.set(3, r22);
        c.call(0x10283690, known ? 0x10210200 : 0x10210250);
        c.set({{5, match()}, {7, c.r(3)}, {3, r21}, {6, r20}, {8, r19}, {9, r17}, {4, known ? 0x20d7u : 0x1f5bu}, {10, 0}});
        c.call(events, known ? 0x10210224 : 0x10210274);
    }
    c.set(3, side);
    call(c, 0x10210430, 0x10210294);
    c.set({{5, c.r(3)}, {4, club()}, {3, c.u32(c.toc(-0x7a6c))}, {6, c.u8(side + lineup_side::kind)}});
    c.call(0x103b0bf0, 0x102102ac);
    c.set(3, side);
    call(c, lineup_entries::apply_assignment, 0x102102b4);
}
}
