// CUP.CPP, block `rounds`: 0x1047ef50, the step to a cup's next round. It draws the round when that has not been done
// (0x1047e980), moves the round's dates when a club of a tie is missing, schedules the fixtures of the current
// round's ties (0x1047f3a0), files the draw's news (0x10481e90, with an 8-byte date of the round's fixtures) and
// moves the current round on. 79 callers outside the unit read r4-r10 after it.
#include "cup_base_rounds_detail.hpp"

namespace cm::aot::cup_base::rounds {

std::uint32_t advance_round(Cpu& c, std::uint32_t cup, std::uint32_t round) {
    const Frame frame(c, 0x1047ef50, 0xb0, {.first_gpr = 24});
    Original o(c);
    o.v[30] = c.toc(toc::competitions); o.v[26] = cup; o.v[25] = c.toc(toc::message); o.v[31] = round;
    o.v[24] = c.toc(toc::pool);
    o.v[5] = c.u32(cup + competition::record);
    o.v[27] = c.u32(o.v[5]);
    const auto done = [&] { o.leave(); return std::uint32_t(1); };
    const auto rounds = [&] { return c.u32(cup + cup::rounds); };
    if (std::int8_t(c.u8(cup + 0x45)) == 0) return done();
    o.v[5] = sext16(c.u16(cup + competition::rounds));
    if (std::int32_t(round) > std::int32_t(o.v[5] - 1) || std::int32_t(round) < 0) {
        // The round is outside the cup: the assertion (line 0xa64), result -1.
        o.publish();
        report(c, frame, {0x1047efbc, 0x68, 0x60, 0xa64});
        c.set(4, c.toc(toc::success));
        return 0xffffffffu;
    }
    o.v[5] = c.u32(cup + cup::round_matches);
    o.v[5] = c.u32(o.v[5] + (round << 2));
    if (std::int8_t(c.u8(o.v[5] + tie::state)) == 0) {
        o.v[3] = cup; o.v[4] = round;
        o.publish();
        c.link(0x1047f020);
        make_round_draw(c, cup, round);
        o.take();
    }
    o.v[28] = sext8(c.u8(cup + cup::current_round));
    o.v[29] = 0;
    o.v[3] = 0;
    o.v[4] = o.v[28] * round_row::size;
    o.v[6] = o.v[28] << 2;
    o.v[4] += round_row::ties;
    for (;; ++o.v[29], o.v[3] += tie::size) {
        o.v[5] = rounds();
        const auto count = sext16(c.u16(o.v[5] + o.v[4]));
        if (!(std::int32_t(o.v[29]) < std::int32_t(count))) break;
        bool missing = false;
        o.v[5] = c.u32(cup + cup::round_matches);
        o.v[5] = c.u32(o.v[5] + o.v[6]);
        const auto first = sext16(c.u16(o.v[5] + o.v[3]));
        if (std::int32_t(first) != -1) {
            o.v[5] = c.u32(cup + cup::entrants);
            missing = c.u32(o.v[5] + first * 8) == 0;
        }
        if (!missing) {
            o.v[5] = c.u32(cup + cup::round_matches);
            o.v[5] = c.u32(o.v[5] + o.v[6]);
            const auto second = sext16(c.u16(o.v[5] + o.v[3] + tie::second));
            if (std::int32_t(second) == -1) continue;
            o.v[5] = c.u32(cup + cup::entrants);
            if (c.u32(o.v[5] + second * 8) != 0) continue;
        }
        // A club is missing: the requested round's draw date moves a day on, as in 0x1047e980.
        move_round_dates(c, o, frame, cup, round, 29, {0x1047f0c0, 0x1047f0d0, 0x1047f164});
        return done();
    }
    // The current round's ties: a tie in state 1 is scheduled as a first leg, one in state 2 as a second; the news
    // below needs the cup still to be in the competitions table.
    o.v[3] = o.v[28] * round_row::size;
    o.v[31] = o.v[28] << 2;
    o.v[29] = 0;
    o.v[25] = 0;
    o.v[24] = o.v[3] + round_row::ties;
    for (;; ++o.v[29], o.v[25] += tie::size) {
        o.v[3] = rounds();
        const auto count = sext16(c.u16(o.v[3] + o.v[24]));
        if (!(std::int32_t(o.v[29]) < std::int32_t(count))) break;
        const auto state = [&] {
            o.v[3] = c.u32(cup + cup::round_matches);
            o.v[3] = c.u32(o.v[3] + o.v[31]);
            return std::int8_t(c.u8(o.v[3] + o.v[25] + tie::state));
        };
        const auto schedule = [&](std::uint32_t first_leg, std::uint32_t back) {
            o.v[3] = cup; o.v[4] = o.v[28]; o.v[5] = sext16(o.v[29]); o.v[6] = first_leg;
            o.publish();
            c.link(back);
            schedule_fixtures(c, cup, o.v[28], o.v[5], first_leg);
            o.take();
        };
        if (state() == 1) schedule(1, 0x1047f26c);
        if (state() == 2) {
            schedule(0, 0x1047f29c);
            o.v[3] = c.u32(o.v[30]);
            if (c.u32(o.v[3] + (o.v[27] << 2)) == 0) return done();
        }
    }
    o.v[3] = 8;
    o.v[24] = o.call(callee::allocate_object, 0x1047f2dc, {8});
    if (o.v[24] != 0) {
        o.v[4] = o.v[28] * round_row::size;
        o.v[6] = rounds();
        o.v[5] = std::uint32_t(c.i16(cup + competition::year));
        const auto year = (o.v[5] + sext8(c.u8(o.v[6] + o.v[4] + 6))) & 0xffff;
        o.v[4] += 4;
        o.v[4] = sext16(c.u16(o.v[6] + o.v[4]));
        o.v[5] = year;
        o.date(date::entry::from_day_and_year, 0x1047f310, {o.v[24], o.v[4], o.v[5]});
    }
    o.v[3] = cup; o.v[4] = o.v[24]; o.v[5] = o.v[28];
    o.publish();
    c.link(0x1047f320);
    draw_news(c, cup, o.v[24], o.v[28]);
    o.take();
    o.call(callee::free_object, 0x1047f328, {o.v[24]});
    o.v[3] = c.u32(o.v[30]);
    if (c.u32(o.v[3] + (o.v[27] << 2)) == 0) return done();
    o.v[3] = o.v[28] * round_row::size;
    o.v[4] = rounds();
    if (sext16(c.u16(o.v[4] + o.v[3] + round_row::label)) != 0x8c) {
        o.v[3] = c.u8(cup + cup::current_round);
        c.put8(cup + cup::current_round, o.v[3] + 1);
    }
    o.v[4] = sext8(c.u8(cup + cup::byte_a9));
    o.v[3] = c.u8(cup + cup::current_round);
    if (std::int32_t(o.v[4]) < std::int32_t(sext8(o.v[3]))) c.put8(cup + cup::byte_a9, o.v[3]);
    return done();
}
}
