// CUP.CPP, block `rounds`: 0x1047f3a0, the fixtures of one tie. A tie waiting in state 1 is marked 2 (first leg done);
// otherwise the tie becomes state 3 and, when both sides are known, its fixture is built in the frame at +0x98 (the
// game's 0x50-byte match record: ids, clubs, the round's dates and flags), completed by the cup's virtual +0x84 and
// the stadium rules 0x104aff20, moved to the day after the game's current date when it is not later than it, handed to
// the fixture manager (0x104888f0) and announced (news item 0x14). A round of kind 2 gets its second leg built the
// same way, on the round's replay date with the sides swapped. A tie with an unknown side files 0x104860a0 instead.
//
// The BlockMoveData bridge (0x10bf7bbc) and GAME.CPP read bytes this frame leaves: every store of the original into
// its frame is repeated at the original offset, and the frame's byte at +0xe7 keeps the stale bits 0x2f it had on entry
// (the original reads and masks it, 0x1047f56c).
#include "cup_base_rounds_detail.hpp"

namespace cm::aot::cup_base::rounds {
namespace {
constexpr std::int32_t date_cell_slot = -0x7b80, fixtures_slot = -0x7a60, rules_slot = -0x742c, competitions_slot = -0x7b5c;
// "date a is later than date b" as 0x1047f850.. compute it: years as unsigned halfwords, equal years by the sign bit
// of ((x >> 1) - (x & a)), x = a ^ b over the sign-extended days.
bool later(std::uint32_t day_a, std::uint32_t year_a, std::uint32_t day_b, std::uint32_t year_b) {
    if (year_a != year_b) return year_a > year_b;
    const auto x = std::int32_t(day_a ^ day_b);
    return ((std::uint32_t(x >> 1) - std::uint32_t(x & std::int32_t(day_a))) >> 31) != 0;
}
}

std::uint32_t schedule_fixtures(Cpu& c, std::uint32_t cup, std::uint32_t round, std::uint32_t tie_arg, std::uint32_t first_leg) {
    const Frame frame(c, 0x1047f3a0, 0x1f0, {.first_gpr = 24});
    Original o(c);
    const auto at = [&](std::uint32_t offset) { return frame.at(offset); };
    const auto put8 = [&](std::uint32_t offset, std::uint32_t value) { c.put8(at(offset), value); };
    const auto put16 = [&](std::uint32_t offset, std::uint32_t value) { c.put16(at(offset), value); };
    const auto put32 = [&](std::uint32_t offset, std::uint32_t value) { c.put32(at(offset), value); };
    const auto rounds = [&] { return c.u32(cup + cup::rounds); };
    const auto matches = [&] { return c.u32(cup + cup::round_matches); };
    const auto entrants = [&] { return c.u32(cup + cup::entrants); };
    const auto fixture = at(0x98), item = at(0xe8);
    o.v[24] = cup; o.v[25] = round; o.v[26] = tie_arg;
    o.v[28] = c.toc(rules_slot); o.v[29] = c.toc(fixtures_slot); o.v[30] = c.toc(date_cell_slot); o.v[31] = c.toc(toc::news);
    o.v[3] = c.u32(cup + competition::record);
    o.v[27] = c.u32(o.v[3]);
    const auto done = [&](std::uint32_t result) { o.leave(); return result; };
    const auto round_half = [&] { return sext16(o.v[25]); };
    if (std::int32_t(sext16(o.v[26])) == -1) {
        // The first tie waiting: state 1 is marked 2, state 2 is scheduled.
        o.v[8] = 0;
        o.v[3] = round_half() * round_row::size;
        o.v[7] = round_half() << 2;
        o.v[3] += round_row::ties;
        for (;;) {
            o.v[4] = rounds();
            o.v[5] = sext16(o.v[8]);
            const auto count = sext16(c.u16(o.v[4] + o.v[3]));
            if (!(std::int32_t(o.v[5]) < std::int32_t(count))) break;
            o.v[5] = matches();
            o.v[4] = sext16(o.v[8]) * tie::size;
            o.v[5] = c.u32(o.v[5] + o.v[7]);
            const auto state = std::int8_t(c.u8(o.v[5] + o.v[4] + tie::state));
            if (state == 1) { o.v[26] = o.v[8]; o.v[6] = 1; break; }
            if (state == 2) { o.v[26] = o.v[8]; o.v[6] = 0; break; }
            ++o.v[8];
        }
    } else {
        o.v[6] = first_leg;
    }
    o.v[4] = sext16(o.v[26]);
    if (std::int32_t(o.v[4]) == -1) { o.v[3] = 0; return done(0); }
    if (std::int8_t(o.v[6]) != 0) {
        o.v[3] = matches();
        o.v[4] *= tie::size;
        o.v[6] = 2;
        o.v[5] = c.u32(o.v[3] + (round_half() << 2));
        o.v[3] = 1;
        c.put8(o.v[5] + o.v[4] + tie::state, 2);
        return done(1);
    }
    o.v[3] = matches();
    o.v[4] *= tie::size;
    o.v[6] = 3;
    o.v[5] = round_half() << 2;
    o.v[3] = c.u32(o.v[3] + o.v[5]);
    c.put8(o.v[3] + o.v[4] + tie::state, 3);
    o.v[3] = matches();
    o.v[3] = c.u32(o.v[3] + o.v[5]);
    if (std::int32_t(sext16(c.u16(o.v[3] + o.v[4]))) == -1 || std::int32_t(sext16(c.u16(o.v[3] + o.v[4] + tie::second))) == -1) {
        o.v[3] = cup; o.v[4] = o.v[25]; o.v[5] = o.v[26];
        o.publish();
        c.link(0x1047fce0);
        missing_side_news(c, cup, o.v[25], o.v[26]);
        o.take();
        o.v[3] = 1;
        return done(1);
    }
    // The fixture record in the frame.
    o.v[3] = c.u32(cup + competition::record);
    if (o.v[3] != 0) {
        o.v[0] = c.u32(o.v[3]);
        put32(0x98, o.v[0]);
        put32(0xac, o.v[3]);
    } else {
        o.v[3] = 0xffffffffu; o.v[0] = 0;
        put32(0x98, o.v[3]);
        put32(0xac, o.v[0]);
    }
    o.v[4] = sext16(c.u16(cup + competition::year));
    o.v[3] = round_half() * round_row::size;
    put16(0xc4, o.v[4]);
    o.v[4] = sext16(c.u16(cup + competition::code));
    put16(0xca, o.v[4]);
    o.v[4] = rounds();
    if (sext8(c.u8(o.v[4] + o.v[3] + round_row::leg_rule)) == 2) {
        o.v[3] = sext16(c.u16(o.v[4] + o.v[3] + round_row::label));
        put16(0xc8, o.v[3] + 3);
    } else {
        put16(0xc8, c.u16(o.v[4] + o.v[3] + round_row::label));
    }
    const auto row = round_half() * round_row::size;  // r11
    o.v[12] = rounds();
    o.v[8] = 0xffffffffu;
    o.v[4] = c.u8(at(0xe7));  // the stale byte the original masks
    o.v[6] = 0;
    o.v[5] = 1;
    o.v[3] = sext16(c.u16(o.v[12] + row + 0xa));
    o.v[10] = c.u8(o.v[12] + row + round_row::byte_18);
    o.v[9] = sext16(c.u16(o.v[12] + row + round_row::rule_flags));
    o.v[7] = sext16(c.u16(o.v[12] + row + round_row::flags));
    put16(0xcc, o.v[3]);
    o.v[4] &= ~0xc0u;
    put8(0xd2, o.v[10]);
    put16(0xce, o.v[9]);
    put16(0xd0, o.v[7]);
    for (const std::uint32_t offset : {0xdbu, 0xdcu, 0xe3u, 0xe4u, 0xddu, 0xdeu, 0xdfu, 0xe0u, 0xe1u, 0xe2u}) put8(offset, o.v[8]);
    put32(0xa0, o.v[8]);
    o.v[7] = c.u8(cup + competition::owner);
    put8(0xe7, o.v[4]);
    o.v[4] = c.u8(at(0xe7)) & ~0x10u;
    put8(0xda, o.v[7]);
    put8(0xd5, o.v[6]);
    put8(0xd6, o.v[6]);
    put8(0xd3, o.v[8]);
    put8(0xd8, o.v[5]);
    put8(0xd9, o.v[5]);
    put8(0xe7, o.v[4]);
    o.v[3] = sext8(c.u8(o.v[12] + row + round_row::leg_rule));
    if (std::int32_t(o.v[3]) < 2) {
        o.v[3] = sext16(c.u16(at(0xd0))) & 0x800;
        if (o.v[3] != 0) {
            o.v[3] = sext16(o.v[26]);
            o.v[4] = matches();
            o.v[3] *= tie::size;
            o.v[0] = round_half() << 2;
            o.v[4] = c.u32(o.v[4] + o.v[0]);
            o.v[5] = entrants();
            o.v[0] = o.v[3] + 2;
            o.v[3] = sext16(c.u16(o.v[4] + o.v[3]));
            o.v[0] = sext16(c.u16(o.v[4] + o.v[0]));
            o.v[4] = o.v[3] << 3;
            o.v[3] = o.v[0] << 3;
            o.v[4] += 4;
            o.v[0] = o.v[3] + 4;
            o.v[3] = sext8(c.u8(o.v[5] + o.v[4]));
            o.v[0] = sext8(c.u8(o.v[5] + o.v[0]));
            put16(0xd0, std::int32_t(o.v[3]) < std::int32_t(o.v[0]) ? 0x800 : 0x1000);
        }
    }
    // The cup's rule for the fixture (virtual +0x84), and the flag bit 0x20.
    o.v[3] = cup; o.v[4] = fixture;
    o.virt(0x84, 0x1047f698, {cup, fixture});
    const auto set_bit_20 = [&](bool on) { put8(0xe7, (c.u8(at(0xe7)) & ~0x20u) | (on ? 0x20u : 0)); };
    o.v[0] = round_half();
    o.v[4] = rounds();
    o.v[3] = o.v[0] * round_row::size;
    if (sext8(c.u8(o.v[4] + o.v[3] + round_row::leg_rule)) < 2 && sext8(c.u8(o.v[4] + o.v[3] + round_row::byte_22)) > 0) {
        set_bit_20(true); o.v[3] = 1;
    } else {
        set_bit_20(false); o.v[3] = 0;
    }
    o.v[3] = round_half(); o.v[0] = sext16(o.v[26]); o.v[4] = matches(); o.v[3] <<= 2; o.v[0] *= tie::size; o.v[5] = entrants();
    o.v[3] = c.u32(o.v[4] + o.v[3]);
    o.v[0] = sext16(c.u16(o.v[3] + o.v[0]));
    o.v[0] <<= 3;
    o.v[3] = c.u32(o.v[5] + o.v[0]);
    put32(0xb4, o.v[3]);
    if (o.v[3] != 0) { o.v[0] = c.u32(o.v[3]); put32(0xa4, o.v[0]); } else { o.v[0] = 0xffffffffu; put32(0xa4, o.v[0]); }
    o.v[0] = sext16(o.v[26]); o.v[4] = round_half(); o.v[3] = o.v[0] * tie::size; o.v[5] = matches(); o.v[0] = o.v[4] << 2;
    o.v[6] = entrants();
    o.v[4] = c.u32(o.v[5] + o.v[0]);
    o.v[0] = o.v[3] + 2;
    o.v[0] = sext16(c.u16(o.v[4] + o.v[0]));
    o.v[0] <<= 3;
    o.v[3] = c.u32(o.v[6] + o.v[0]);
    put32(0xb8, o.v[3]);
    if (o.v[3] != 0) { o.v[0] = c.u32(o.v[3]); put32(0xa8, o.v[0]); } else { o.v[0] = 0xffffffffu; put32(0xa8, o.v[0]); }
    o.v[6] = round_half(); o.v[7] = rounds(); o.v[5] = o.v[6] * round_row::size;
    o.v[8] = sext16(c.u16(cup + competition::year));
    o.v[0] = o.v[5] + round_row::year_offset;
    o.v[4] = c.u8(o.v[7] + o.v[0]);
    o.v[3] = o.v[5] + round_row::fixture_day;
    o.v[0] = o.v[5] + round_row::fixture_kind;
    o.v[4] = sext8(o.v[4]);
    o.v[4] = o.v[8] + o.v[4];
    put16(0xc0, o.v[4]);
    o.v[3] = sext16(c.u16(o.v[7] + o.v[3]));
    put16(0xc2, o.v[3]);
    o.v[0] = c.u8(o.v[7] + o.v[0]);
    put8(0xd7, o.v[0]);
    o.v[3] = sext16(c.u16(cup + competition::rounds));
    o.v[0] = o.v[3] - 1;
    if (std::int32_t(o.v[6]) < std::int32_t(o.v[0])) {
        o.v[0] = o.v[6] + 1;
        o.v[4] = o.v[0] * round_row::size;
        o.v[0] = o.v[4] + round_row::fixture_day;
        o.v[3] = sext16(c.u16(o.v[7] + o.v[0]));
        o.v[0] = o.v[4] + round_row::year_offset;
        put16(0xbe, o.v[3]);
        o.v[0] = sext8(c.u8(o.v[7] + o.v[0]));
        o.v[0] = o.v[8] + o.v[0];
        put16(0xbc, o.v[0]);
    } else {
        o.v[0] = 0;
        put16(0xbe, 0);
        put16(0xbc, 0);
    }
    o.v[0] = round_half(); o.v[7] = rounds(); o.v[4] = o.v[0] * round_row::size;
    o.v[3] = c.u32(o.v[28]); o.v[6] = c.u32(cup + competition::word_24); o.v[5] = fixture; o.v[0] = o.v[4] + round_row::rules_id;
    o.v[4] = c.u32(o.v[7] + o.v[0]);
    o.call(0x104aff20, 0x1047f82c, {o.v[3], o.v[4], o.v[5], o.v[6]});
    // The fixture's date: when it is not later than the game's current date (the cell at TOC -0x7b80) it moves to the
    // day after.
    const auto current_date = [&](std::uint32_t copy_at) {  // the cell's two words copied into the frame
        o.v[4] = c.u32(o.v[30]);
        o.v[6] = c.u32(o.v[30] + 4);
        put32(copy_at, o.v[4]);
        put32(copy_at + 4, o.v[6]);
    };
    const auto unless_later = [&](std::uint32_t date_at, std::uint32_t current_at, std::uint32_t result_at, std::uint32_t back_from, std::uint32_t back_add,
                                  std::uint32_t temp_at) {
        current_date(current_at);
        o.v[0] = c.u16(at(0xc2));
        o.v[5] = c.u16(at(0xc0));
        o.v[4] = sext16(o.v[0]);
        o.date(date::entry::from_day_and_year, back_from, {date_at, o.v[4], o.v[5]});
        const auto date_ptr = o.v[3];
        o.v[4] = c.u16(at(current_at + 2));
        o.v[0] = c.u16(date_ptr + 2);
        const auto cell_day = sext16(c.u16(at(current_at))), date_day = sext16(c.u16(date_ptr));
        if (o.v[4] == o.v[0]) o.v[4] = date_day;
        if (!later(date_day, c.u16(date_ptr + 2), cell_day, c.u16(at(current_at + 2)))) {
            o.v[4] = o.v[30]; o.v[3] = result_at; o.v[5] = 1;
            o.date(date::entry::add_days, back_add, {o.v[3], o.v[4], o.v[5]});
            o.v[3] = sext16(c.u16(result_at));
            o.v[0] = c.u16(result_at + 2);
            c.put16(temp_at, o.v[3]);
            o.v[4] = c.u8(result_at + 4);
            c.put16(temp_at + 2, o.v[0]);
            o.v[3] = sext16(c.u16(temp_at));
            o.v[0] = c.u16(temp_at + 2);
            c.put8(temp_at + 4, o.v[4]);
            put16(0xc2, o.v[3]);
            put16(0xc0, o.v[0]);
        }
    };
    unless_later(at(0x78), 0x40, at(0x70), 0x1047f850, 0x1047f8ac, at(0x90));
    o.call(0x104888f0, 0x1047f8e4, {c.u32(o.v[29]), fixture, 1});
    const auto manager = [&] { return c.u32(o.v[31]); };
    const auto enabled = [&](std::uint32_t club, std::uint32_t back) { return std::int8_t(o.call(item::enabled, back, {manager(), club})) != 0; };
    bool announce = enabled(c.u32(at(0xb4)), 0x1047f8f0) || enabled(c.u32(at(0xb8)), 0x1047f904);
    if (announce) {
        o.v[0] = round_half(); o.v[4] = rounds(); o.v[3] = o.v[0] * round_row::size; o.v[0] = o.v[3] + round_row::flags;
        announce = !(sext16(c.u16(o.v[4] + o.v[0])) & 0x100);
    }
    if (announce) {
        const auto field = [&](std::uint32_t n, std::uint32_t value, std::uint32_t back) { o.call(item::set_field, back, {item, n, value}); };
        o.call(item::start, 0x1047f938, {item, 0x14, 0});
        o.v[5] = c.u32(cup + competition::record); o.v[5] = c.u32(o.v[5]); field(0, o.v[5], 0x1047f94c);
        o.v[5] = sext16(c.u16(cup + competition::code)); field(1, o.v[5], 0x1047f95c);
        o.v[0] = round_half(); o.v[6] = rounds(); o.v[5] = o.v[0] * round_row::size; o.v[0] = o.v[5] + round_row::label;
        o.v[5] = sext16(c.u16(o.v[6] + o.v[0])); field(2, o.v[5], 0x1047f97c);
        o.v[0] = round_half(); o.v[6] = rounds(); o.v[5] = o.v[0] * round_row::size; o.v[0] = o.v[5] + round_row::leg_rule;
        o.v[0] = sext8(c.u8(o.v[6] + o.v[0]));
        o.v[5] = o.v[0] == 2 ? 1 : 0; field(5, o.v[5], 0x1047f9ac);
        o.v[5] = c.u32(at(0x9c)); field(6, o.v[5], 0x1047f9bc);
        o.v[5] = c.u16(at(0xc2)); field(7, o.v[5], 0x1047f9cc);
        o.v[5] = c.u16(at(0xc0)); field(8, o.v[5], 0x1047f9dc);
        if (enabled(c.u32(at(0xb4)), 0x1047f9e8)) {
            o.v[5] = c.u32(at(0xa4)); field(3, o.v[5], 0x1047fa00);
            o.v[5] = c.u32(at(0xa8)); field(4, o.v[5], 0x1047fa10);
        } else {
            o.v[5] = c.u32(at(0xa8)); field(3, o.v[5], 0x1047fa24);
            o.v[5] = c.u32(at(0xa4)); field(4, o.v[5], 0x1047fa34);
        }
        o.v[5] = c.u32(at(0xb4)); o.call(item::file_under_club, 0x1047fa44, {manager(), item, o.v[5]});
        o.v[5] = c.u32(at(0xb8)); o.call(item::file_under_club, 0x1047fa54, {manager(), item, o.v[5]});
    }
    // A competition that stopped existing during the news ends the routine.
    o.v[3] = c.toc(competitions_slot);
    o.v[3] = c.u32(o.v[3]);
    if (c.u32(o.v[3] + (o.v[27] << 2)) == 0) { o.v[3] = 1; return done(1); }
    o.v[0] = round_half(); o.v[4] = rounds(); o.v[3] = o.v[0] * round_row::size; o.v[0] = o.v[3] + round_row::leg_rule;
    if (sext8(c.u8(o.v[4] + o.v[0])) != 2) { o.v[3] = 1; return done(1); }
    // Rounds of kind 2: the second leg, with the clubs swapped, on the replay date.
    o.v[0] = o.v[3] + round_row::byte_22;
    set_bit_20(sext8(c.u8(o.v[4] + o.v[0])) > 0);
    o.v[3] = sext8(c.u8(o.v[4] + o.v[0])) > 0 ? 1 : 0;
    o.v[0] = round_half(); o.v[6] = rounds(); o.v[4] = o.v[0] * round_row::size; o.v[3] = at(0x88); o.v[0] = o.v[4] + round_row::label;
    o.v[5] = sext16(c.u16(o.v[6] + o.v[0])); o.v[0] = o.v[4] + round_row::year_offset; o.v[4] += round_row::fixture_day;
    o.v[5] += 4;
    put16(0xc8, o.v[5]);
    o.v[0] = c.u8(o.v[6] + o.v[0]);
    o.v[5] = sext16(c.u16(o.v[6] + o.v[4]));
    o.v[4] = sext16(c.u16(cup + competition::year));
    o.v[0] = sext8(o.v[0]);
    o.v[5] &= 0xffff;
    o.v[0] = o.v[4] + o.v[0];
    o.v[4] = sext16(o.v[5]);
    o.v[5] = o.v[0] & 0xffff;
    o.date(date::entry::from_day_and_year, 0x1047fb14, {at(0x88), o.v[4], o.v[5]});
    o.v[0] = round_half(); o.v[6] = rounds(); o.v[5] = o.v[0] * round_row::size; o.v[3] = at(0x60); o.v[4] = at(0x88);
    o.v[0] = o.v[5] + round_row::byte_24;
    o.v[5] = c.u8(o.v[6] + o.v[0]);
    o.date(date::entry::add_days, 0x1047fb34, {o.v[3], o.v[4], o.v[5]});
    o.v[0] = sext16(c.u16(at(0x60)));
    o.v[3] = at(0x58);
    o.v[4] = c.u16(at(0x62));
    put16(0x88, o.v[0]);
    o.v[0] = c.u8(at(0x64));
    put16(0x8a, o.v[4]);
    o.v[4] = sext16(c.u16(at(0x88)));
    put8(0x8c, o.v[0]);
    o.v[0] = c.u16(at(0x8a));
    put16(0xc2, o.v[4]);
    o.v[4] = sext16(c.u16(at(0x88)));
    put16(0xc0, o.v[0]);
    o.v[7] = c.u16(at(0x8a));
    o.v[6] = c.u8(at(0x8c));
    o.v[0] = c.u16(at(0xc2));
    put16(0x68, o.v[4]);
    o.v[5] = c.u16(at(0xc0));
    o.v[4] = sext16(o.v[0]);
    put16(0x6a, o.v[7]);
    put8(0x6c, o.v[6]);
    o.date(date::entry::from_day_and_year, 0x1047fb88, {o.v[3], o.v[4], o.v[5]});
    o.date(date::entry::weekday, 0x1047fb8c, {});
    o.v[0] = sext16(o.v[3]);
    put8(0xd7, (o.v[0] == 5 || o.v[0] == 6) ? 1 : 2);
    o.v[3] = c.u8(at(0xe7));
    o.v[6] = 0;
    o.v[3] &= ~0xc0u;
    o.v[0] = round_half();
    put8(0xe7, o.v[3]);
    o.v[3] = o.v[0] * round_row::size;
    o.v[8] = c.u32(at(0xb4));
    o.v[5] = fixture;
    o.v[7] = c.u32(at(0xb8));
    o.v[4] = c.u8(at(0xe7)) & ~0x10u;
    o.v[9] = c.u32(at(0xa4));
    o.v[0] = o.v[3] + round_row::rules_id;
    o.v[6] = c.u32(at(0xa8));
    put32(0xb4, o.v[7]);
    o.v[3] = c.u32(o.v[28]);
    put32(0xb8, o.v[8]);
    put32(0xa4, o.v[6]);
    put32(0xa8, o.v[9]);
    put8(0xe7, o.v[4]);
    o.v[4] = rounds();
    o.v[6] = c.u32(cup + competition::word_24);
    o.v[4] = c.u32(o.v[4] + o.v[0]);
    o.call(0x104aff20, 0x1047fc14, {o.v[3], o.v[4], o.v[5], o.v[6]});
    unless_later(at(0x50), 0x38, at(0x48), 0x1047fc38, 0x1047fc94, at(0x80));
    o.call(0x104888f0, 0x1047fccc, {c.u32(o.v[29]), fixture, 1});
    o.v[3] = 1;
    return done(1);
}
}
