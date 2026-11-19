// CUP.CPP, block `rounds`: 0x1047e980, the draw of a round. A round is drawn once (its first tie's state byte is 0).
// When a club of a tie is missing the round's dates move a day on (and every later round is kept no earlier than
// that); otherwise the clubs are ordered by the round's rules (the `draws` block's routines), the sides of the
// ties are put right (0x10481ad0, 0x10485d70), every tie becomes state 1 and, for a round with two or more
// ties that is not flagged otherwise, news item 0x27 is filed under each club.
#include "cup_base_rounds_detail.hpp"

namespace cm::aot::cup_base::rounds {
namespace {
constexpr std::int32_t fixture_rules_slot = -0x7b54, opponents_slot = -0x7a64;
// The original's "date a is later than date b" test (0x1047eb2c..): years compare as unsigned halfwords; equal
// years by the sign bit of ((x >> 1) - (x & a)) with x = a ^ b over the sign-extended days. `r4` is where the
// original leaves its day register (the equal-years case loads it).
bool later(std::uint32_t day_a, std::uint32_t year_a, std::uint32_t day_b, std::uint32_t year_b, std::uint32_t& r4) {
    if (year_a > year_b) return true;
    if (year_a < year_b) return false;
    r4 = day_a;
    const auto x = std::int32_t(day_a ^ day_b);
    return ((std::uint32_t(x >> 1) - std::uint32_t(x & std::int32_t(day_a))) >> 31) != 0;
}
}

// The block both 0x1047e980 and 0x1047ef50 inline when a club of a tie is missing: the round's draw date moves a day on
// (the date copied through the frame at +0x58, its successor at +0x40) and every later round whose draw date is
// earlier takes it. `counter` is the original's register of the loop over the later rounds (r28 / r29); the three
// `back` addresses are the returns of its DATE calls.
void move_round_dates(Cpu& c, Original& o, const Frame& frame, std::uint32_t cup, std::uint32_t round, unsigned counter,
                      const std::array<std::uint32_t, 3>& back) {
    const auto rounds = [&] { return c.u32(cup + cup::rounds); };
    const auto year = [&] { return c.i16(cup + competition::year); };
    const auto draw_date = [&](std::uint32_t row) {  // day (sign-extended) and year of a round's draw date
        return std::pair{std::uint32_t(c.i16(rounds() + row)), std::uint32_t(std::int32_t(year()) + std::int8_t(c.u8(rounds() + row + 2))) & 0xffff};
    };
    auto& counter_value = o.v[counter];
    // A club is missing: this round's draw date moves a day on and so does every later date that is earlier.
    o.v[4] = round * round_row::size;
    o.v[6] = rounds();
    o.v[5] = std::uint32_t(year());
    o.v[3] = frame.at(0x58);
    const auto [day, y] = draw_date(o.v[4]);
    o.v[4] = day;
    o.v[5] = y;
    o.date(date::entry::from_day_and_year, back[0], {frame.at(0x58), o.v[4], o.v[5]});
    o.date(date::entry::add_days, back[1], {frame.at(0x40), frame.at(0x58), 1});
    o.v[5] = c.u8(frame.at(0x44));
    o.v[3] = c.u16(frame.at(0x42));
    o.v[4] = round * round_row::size;
    o.v[6] = sext16(c.u16(frame.at(0x40)));
    counter_value = sext16(round + 1);
    c.put16(frame.at(0x5a), o.v[3]);
    o.v[3] = o.v[4] + 2;
    c.put16(frame.at(0x58), o.v[6]);
    o.v[7] = c.u16(frame.at(0x5a));
    c.put8(frame.at(0x5c), o.v[5]);
    o.v[8] = sext16(c.u16(frame.at(0x58)));
    o.v[6] = c.u8(frame.at(0x5c));
    o.v[5] = rounds();
    c.put16(o.v[5] + o.v[4], c.u16(frame.at(0x58)));
    o.v[5] = std::uint32_t(year());
    const auto new_year = c.u16(frame.at(0x5a));
    o.v[4] = rounds();
    c.put8(o.v[4] + o.v[3], new_year - o.v[5]);
    for (; std::int32_t(sext16(counter_value)) < std::int32_t(c.i16(cup + competition::rounds)); ++counter_value) {
        const auto row = sext16(counter_value) * round_row::size;
        o.v[6] = rounds();
        o.v[5] = std::uint32_t(year());
        const auto [d, yy] = draw_date(row);
        o.v[4] = d;
        o.v[5] = yy;
        o.v[3] = frame.at(0x50);
        o.date(date::entry::from_day_and_year, back[2], {frame.at(0x50), o.v[4], o.v[5]});
        c.put32(frame.at(0x38), c.u32(frame.at(0x58)));
        o.v[4] = c.u32(frame.at(0x5c));
        c.put32(frame.at(0x3c), o.v[4]);
        o.v[3] = c.u16(frame.at(0x3a));
        if (later(sext16(c.u16(frame.at(0x38))), o.v[3], sext16(c.u16(frame.at(0x50))), c.u16(frame.at(0x52)), o.v[4])) {
            o.v[5] = sext16(c.u16(frame.at(0x58)));
            o.v[3] = sext16(counter_value) * round_row::size;
            o.v[4] = rounds();
            c.put16(o.v[4] + o.v[3], o.v[5]);
            o.v[5] = std::uint32_t(year());
            o.v[4] = c.u16(frame.at(0x5a));
            o.v[3] = rounds();
            o.v[4] = o.v[4] - o.v[5];
            c.put8(o.v[3] + sext16(counter_value) * round_row::size + 2, o.v[4]);
        }
    }
}

std::uint32_t make_round_draw(Cpu& c, std::uint32_t cup, std::uint32_t round) {
    const Frame frame(c, 0x1047e980, 0x160, {.first_gpr = 26});
    Original o(c);
    o.v[29] = c.toc(opponents_slot); o.v[26] = cup; o.v[30] = c.toc(toc::news); o.v[27] = round;
    o.v[31] = c.toc(fixture_rules_slot);
    const auto done = [&] { o.leave(); return std::uint32_t(1); };
    const auto rounds = [&] { return c.u32(cup + cup::rounds); };
    if (std::int8_t(c.u8(cup + 0x45)) == 0) return done();
    o.v[3] = c.u32(cup + cup::round_matches);
    o.v[7] = round << 2;
    o.v[3] = c.u32(o.v[3] + o.v[7]);
    if (std::int8_t(c.u8(o.v[3] + tie::state)) != 0) return done();
    o.v[5] = round * round_row::size;
    o.v[28] = 0;
    o.v[3] = 0;
    o.v[4] = o.v[5] + round_row::ties;
    for (;; o.v[28]++, o.v[3] += tie::size) {
        o.v[6] = rounds();
        const auto count = sext16(c.u16(o.v[6] + o.v[4]));
        if (!(std::int32_t(o.v[28]) < std::int32_t(count))) break;
        bool missing = false;
        o.v[6] = c.u32(cup + cup::round_matches);
        o.v[6] = c.u32(o.v[6] + o.v[7]);
        const auto first = sext16(c.u16(o.v[6] + o.v[3]));
        if (std::int32_t(first) != -1) {
            o.v[6] = c.u32(cup + cup::entrants);
            missing = c.u32(o.v[6] + first * 8) == 0;
        }
        if (!missing) {
            o.v[6] = c.u32(cup + cup::round_matches);
            o.v[6] = c.u32(o.v[6] + o.v[7]);
            const auto second = sext16(c.u16(o.v[6] + o.v[3] + tie::second));
            if (std::int32_t(second) == -1) continue;
            o.v[6] = c.u32(cup + cup::entrants);
            if (c.u32(o.v[6] + second * 8) != 0) continue;
        }
        move_round_dates(c, o, frame, cup, round, 28, {0x1047ea70, 0x1047ea80, 0x1047eb14});
        return done();
    }
    // Every tie has its clubs: order them by the round's rules.
    const auto flags = [&] { return sext16(c.u16(o.v[6] + o.v[5] + round_row::flags)); };
    o.v[3] = flags();
    const auto round_half = sext16(round);
    if (o.v[3] & 0x400) {
        o.call(0x1047fd00, 0x1047ebf0, {cup, round_half});
    } else {
        const auto rule = sext16(c.u16(o.v[6] + o.v[5] + round_row::rule_flags));
        if (rule & 4) {
            o.call(0x10480620, 0x1047ec10, {cup, round_half});
        } else if (!(o.v[3] & 0x200)) {
            o.call(0x104810b0, 0x1047ec28, {cup, round_half});
        }
    }
    o.v[3] = cup; o.v[4] = round_half; o.v[5] = 0xffffffffu;
    o.publish();
    c.link(0x1047ec38);
    order_tie_sides(c, cup, round_half, 0xffffffffu);
    o.take();
    o.v[3] = round * round_row::size;
    o.v[4] = rounds();
    if (std::int8_t(c.u8(o.v[4] + o.v[3] + round_row::leg_rule)) == 2) {
        o.v[3] = cup; o.v[4] = round_half;
        o.publish();
        c.link(0x1047ec60);
        separate_same_nation_ties(c, cup, round_half);
        o.take();
    }
    o.v[5] = round * round_row::size;
    o.v[7] = round << 2;
    o.v[28] = 0;
    o.v[3] = 0;
    o.v[4] = o.v[5] + round_row::ties;
    o.v[8] = 1;
    std::int32_t ties;
    for (;; ++o.v[28], o.v[3] += tie::size) {
        o.v[6] = rounds();
        ties = std::int16_t(c.u16(o.v[6] + o.v[4]));
        if (!(std::int32_t(o.v[28]) < ties)) break;
        o.v[6] = c.u32(cup + cup::round_matches);
        o.v[6] = c.u32(o.v[6] + o.v[7]);
        c.put8(o.v[6] + o.v[3] + tie::state, o.v[8]);
    }
    if (ties < 2) return done();
    if (c.u16(o.v[6] + o.v[5] + round_row::flags) & 0x200) return done();
    // A news hook or the fixture rules can stop the news of a round.
    o.v[3] = c.u32(c.toc(opponents_slot));
    if (o.v[3] != 0) {
        o.v[4] = c.u32(cup + competition::record);
        if (std::int8_t(o.call(0x102aed80, 0x1047ece4, {o.v[3], o.v[4]})) != 0) return done();
    }
    o.v[3] = c.u32(c.toc(fixture_rules_slot));
    if (o.v[3] != 0) {
        o.v[4] = c.u32(cup + competition::record);
        if (std::int8_t(o.call(0x10494d20, 0x1047ed08, {o.v[3], o.v[4]})) != 0) return done();
    }
    o.v[3] = round * round_row::size;
    o.v[4] = rounds();
    if (sext16(c.u16(o.v[4] + o.v[3] + round_row::label)) != 0x8c) {
        o.v[3] = c.u8(cup + cup::byte_a9);
        c.put8(cup + cup::byte_a9, o.v[3] + 1);
    }
    // The halfword +0x1e (entrants) of every round up to and including this one, summed (the loop below walks that many
    // entrant rows).
    o.v[3] = round + 1;
    o.v[4] = 0;
    o.v[28] = 0;
    if (std::int32_t(round) >= 0) {
        const auto count = round + 1;
        std::uint32_t sum = 0;
        for (std::uint32_t k = 0; k < count; ++k) {
            o.v[6] = rounds();
            sum += sext16(c.u16(o.v[6] + k * round_row::size + round_row::entrants));
        }
        o.v[28] = sum;
        o.v[4] = count * round_row::size;
        o.v[5] = round * round_row::size + round_row::entrants;
    }
    // News item 0x27 for the round's clubs.
    const auto item = frame.at(0x60);
    const auto field = [&](std::uint32_t n, std::uint32_t back) { return o.call(item::set_field, back, {item, n, o.v[5]}); };
    o.call(item::start, 0x1047ee00, {item, 0x27, 0});
    o.v[5] = c.u32(cup + competition::record); o.v[5] = c.u32(o.v[5]); field(0, 0x1047ee14);
    o.v[5] = sext16(c.u16(cup + competition::code)); field(1, 0x1047ee24);
    o.v[5] = round * round_row::size; o.v[6] = rounds();
    o.v[5] = sext16(c.u16(o.v[6] + o.v[5] + round_row::label)); field(2, 0x1047ee40);
    o.v[5] = sext8(c.u8(cup + competition::owner)); field(4, 0x1047ee54);
    o.v[5] = round; field(5, 0x1047ee64);
    o.v[5] = 0; field(6, 0x1047ee74);
    o.v[3] = sext8(c.u8(cup + cup::current_round)) + 1;
    if (std::int32_t(o.v[3]) < std::int32_t(sext8(c.u8(cup + cup::byte_a9)))) { o.v[5] = 1; field(7, 0x1047eea0); }
    else { o.v[5] = 0; field(7, 0x1047eeb4); }
    o.v[29] = sext16(o.v[28]); o.v[28] = 0; o.v[27] = 0;
    for (; std::int32_t(o.v[28]) < std::int32_t(o.v[29]); ++o.v[28], o.v[27] += 8) {
        o.v[3] = c.u32(cup + cup::entrants);
        if (std::int8_t(c.u8(o.v[3] + o.v[27] + 5)) != 0) continue;
        o.v[4] = c.u32(o.v[3] + o.v[27]);
        if (o.v[4] == 0) continue;
        o.v[3] = c.u32(o.v[30]);
        if (std::int8_t(o.call(item::enabled, 0x1047eeec, {o.v[3], o.v[4]})) == 0) continue;
        o.v[5] = c.u32(cup + cup::entrants);
        o.v[5] = c.u32(o.v[5] + o.v[27]);
        o.v[5] = c.u32(o.v[5]);
        o.call(item::set_field, 0x1047ef0c, {item, 3, o.v[5]});
        o.v[5] = c.u32(cup + cup::entrants);
        o.v[5] = c.u32(o.v[5] + o.v[27]);
        o.call(item::file_under_club, 0x1047ef20, {c.u32(o.v[30]), item, o.v[5]});
    }
    return done();
}
}
