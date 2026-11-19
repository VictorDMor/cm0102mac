// CUP.CPP, block `rounds`: the news a round's draw files. 0x10481e90 walks FIX_MAN's fixtures of the round's date
// (an iterator in the frame at +0x48): each one that is this round's own fixture row (same date, year, day kind
// and stage) is dated to the weekday its row asks for and handed to the fixture manager and the stadium rules,
// with a news item 3; every fixture then files news item 0x14 under the clubs the news manager follows.
// 0x104860a0 files item 0x26 for a tie one of whose sides is still undecided (it names the clubs of the ties
// that feed it).
#include "cup_base_rounds_detail.hpp"

namespace cm::aot::cup_base::rounds {
namespace {
constexpr std::int32_t fixtures_slot = -0x7a60, rules_slot = -0x742c;
// Offsets of a fixture record (0x50 bytes; FIX_MAN's) that this routine reads.
namespace fixture {
inline constexpr std::uint32_t day_kind = 0x3f, stage = 0x42, year = 0x28, day = 0x2a;
}
}

void draw_news(Cpu& c, std::uint32_t cup, std::uint32_t date, std::uint32_t round) {
    const Frame frame(c, 0x10481e90, 0x2c0, {.first_gpr = 21});
    Original o(c);
    const auto iterator = frame.at(0x48), item = frame.at(0x1ac), copy = frame.at(0x78), when = frame.at(0x38), grant = frame.at(0xc8);
    o.v[22] = date; o.v[31] = round; o.v[24] = c.toc(rules_slot); o.v[30] = cup; o.v[25] = c.toc(fixtures_slot);
    o.v[26] = c.toc(toc::news); o.v[27] = c.toc(toc::competitions); o.v[28] = c.toc(toc::message); o.v[29] = c.toc(toc::pool);
    const auto rounds = [&] { return c.u32(cup + cup::rounds); };
    const auto field = [&](std::uint32_t target, std::uint32_t n, std::uint32_t value, std::uint32_t back) {
        o.call(item::set_field, back, {target, n, value});
    };
    const auto manager = [&] { return c.u32(o.v[26]); };
    const auto enabled = [&](std::uint32_t club, std::uint32_t back) { return std::int8_t(o.call(item::enabled, back, {manager(), club})) != 0; };
    const auto file = [&](std::uint32_t target, std::uint32_t club, std::uint32_t back) { o.call(item::file_under_club, back, {manager(), target, club}); };
    const auto leave = [&] { o.leave(); };
    const auto close = [&](std::uint32_t back) { o.call(0x10738c20, back, {iterator, 0xffffffffu}); };
    o.v[4] = 0x14;
    o.call(item::start, 0x10481ed4, {item, 0x14, 0});
    o.v[3] = round * round_row::size;
    o.v[5] = c.u32(cup + competition::record);
    o.v[4] = rounds();
    o.v[23] = c.u32(o.v[5]);
    if (!(sext16(c.u16(o.v[4] + o.v[3] + round_row::flags)) & 0x100)) return leave();
    if (std::int8_t(c.u8(o.v[4] + o.v[3] + 0x2e)) == -1) return leave();
    o.call(0x10738c00, 0x10481f18, {iterator});
    o.v[3] = round * round_row::size;
    o.v[7] = c.u32(cup + competition::record);
    o.v[5] = sext16(c.u16(o.v[22]));
    o.v[8] = round;
    o.v[4] = rounds();
    o.v[10] = iterator;
    o.v[6] = c.u8(o.v[4] + o.v[3] + 7);
    o.v[3] = c.u32(o.v[25]);
    o.v[5] &= 0xffff;
    o.v[4] = c.u16(o.v[22] + 2);
    o.v[7] = c.u32(o.v[7]);
    o.v[9] = c.u8(cup + 0x44);
    const auto gathered = o.call(0x1048a2a0, 0x10481f50, {o.v[3], o.v[4], o.v[5], o.v[6], o.v[7], o.v[8], o.v[9], o.v[10]});
    // 0x10481f68..0x10481fcc (the assertion of source line 0x1018, then the iterator's close) never runs: its `bge` follows the
    // record form `rlwinm.` of a zero-extended halfword, whose result cannot be negative, so it is always taken.
    if ((gathered & 0xffff) == 0) {
        close(0x10481f64);
        return leave();
    }
    o.v[5] = c.u32(cup + competition::record); o.v[5] = c.u32(o.v[5]);
    field(item, 0, o.v[5], 0x10481fe4);
    o.v[5] = sext16(c.u16(cup + competition::code));
    field(item, 1, o.v[5], 0x10481ff4);
    o.v[5] = round * round_row::size; o.v[6] = rounds();
    o.v[5] = sext16(c.u16(o.v[6] + o.v[5] + round_row::label));
    field(item, 2, o.v[5], 0x10482010);
    o.v[5] = round * round_row::size; o.v[6] = rounds();
    o.v[5] = sext8(c.u8(o.v[6] + o.v[5] + round_row::leg_rule)) == 2 ? 1 : 0;
    field(item, 5, o.v[5], 0x1048203c);
    c.put32(frame.at(0x54), c.u32(frame.at(0x4c)));
    o.v[28] = 0;
    o.v[22] = 0;
    std::uint32_t fixture_at = 0;
    for (;;) {
        fixture_at = o.call(0x10738e00, 0x10482618, {iterator});
        o.v[29] = fixture_at;
        if (o.v[29] == 0) {
            close(0x1048262c);
            return leave();
        }
        o.v[3] = c.u8(cup + 0x44);
        const auto stage = sext8(c.u8(o.v[29] + fixture::stage));
        o.v[5] = sext8(o.v[3]);
        if (o.v[5] != stage) continue;
        const auto news_only = [&] {
            // A fixture that is not one of the round's own rows: news item 0x14 straight from the record.
            const auto f = o.v[29];
            if (!enabled(c.u32(f + 0x1c), 0x1048254c) && !enabled(c.u32(f + 0x20), 0x10482560)) return;
            if (enabled(c.u32(f + 0x1c), 0x10482574)) {
                field(item, 3, c.u32(f + 0xc), 0x1048258c);
                field(item, 4, c.u32(f + 0x10), 0x1048259c);
            } else {
                field(item, 3, c.u32(f + 0x10), 0x104825b0);
                field(item, 4, c.u32(f + 0xc), 0x104825c0);
            }
            field(item, 6, c.u32(f + 4), 0x104825d0);
            field(item, 7, c.u16(f + fixture::day), 0x104825e0);
            field(item, 8, c.u16(f + fixture::year), 0x104825f0);
            file(item, c.u32(f + 0x1c), 0x10482600);
            file(item, c.u32(f + 0x20), 0x10482610);
        };
        if (std::int32_t(o.v[28]) >= 8) { news_only(); continue; }
        o.v[3] = round * round_row::size;
        o.v[4] = rounds();
        o.v[6] = o.v[3] + o.v[22];
        o.v[6] += 0x2e;
        o.v[6] = sext8(c.u8(o.v[4] + o.v[6]));
        if (std::int32_t(o.v[6]) == -1) { news_only(); continue; }
        o.v[6] = o.v[3] + 4;
        // The fixture copied into the frame (words, halfwords, bytes, as the original copies them).
        for (std::uint32_t at = 0; at < 0x24; at += 4) c.put32(copy + at, c.u32(o.v[29] + at));
        for (std::uint32_t at = 0x24; at < 0x3a; at += 2) c.put16(copy + at, c.u16(o.v[29] + at));
        for (std::uint32_t at = 0x3a; at < 0x4e; ++at) c.put8(copy + at, c.u8(o.v[29] + at));
        c.put16(copy + 0x4e, c.u16(o.v[29] + 0x4e));
        o.v[7] = c.u16(o.v[29] + 0x4e);
        auto own = false;
        o.v[7] = c.u16(o.v[29] + fixture::day);
        o.v[6] = sext16(c.u16(o.v[4] + o.v[6]));
        if (o.v[7] == o.v[6]) {
            o.v[6] = o.v[3] + 6;
            o.v[7] = std::uint32_t(c.i16(cup + competition::year));
            o.v[6] = c.u8(o.v[4] + o.v[6]);
            o.v[8] = c.u16(o.v[29] + fixture::year);
            o.v[6] = sext8(o.v[6]);
            o.v[6] = o.v[7] + o.v[6];
            if (o.v[8] == o.v[6]) {
                o.v[3] += 7;
                o.v[6] = c.u8(o.v[29] + fixture::day_kind);
                o.v[3] = sext8(c.u8(o.v[4] + o.v[3]));
                // The last compare of the original (0x10482228, the fixture's stage against the cup's owner byte) repeats
                // the one that let the fixture in at 0x10482064: both registers still hold those values, so it holds.
                own = o.v[6] == o.v[3];
            }
        }
        if (own) {
            o.v[3] = 8;
            o.v[21] = o.call(callee::allocate_object, 0x10482238, {8});
            if (o.v[21] != 0) {
                o.v[4] = sext16(c.u16(o.v[29] + fixture::day));
                o.v[5] = c.u16(o.v[29] + fixture::year);
                o.date(date::entry::from_day_and_year, 0x10482250, {o.v[3], o.v[4], o.v[5]});
            }
            o.v[29] = o.v[21];
            o.date(date::entry::set_default, 0x1048225c, {when});
            o.v[6] = rounds();
            o.v[3] = o.v[21];
            o.v[5] = when;
            o.v[4] = round * round_row::size + o.v[22];
            o.v[4] = sext8(c.u8(o.v[6] + o.v[4] + 0x2c));
            o.date(date::entry::nearest_weekday, 0x10482280, {o.v[3], o.v[4], o.v[5]});
            o.v[4] = rounds();
            o.v[3] = round * round_row::size + o.v[22];
            if (std::int8_t(c.u8(o.v[4] + o.v[3] + 0x2e)) != 3) {
                o.call(item::start, 0x104822b0, {grant, 3, 0});
                field(grant, 0, c.u32(copy + 0xc), 0x104822c0);
                field(grant, 1, c.u32(copy + 0x10), 0x104822d0);
                field(grant, 2, c.u32(copy + 0x00), 0x104822e0);
                field(grant, 3, sext16(c.u16(copy + 0x32)), 0x104822f0);
                field(grant, 4, sext16(c.u16(copy + 0x30)), 0x10482300);
                field(grant, 5, c.u16(copy + 0x2a), 0x10482310);
                field(grant, 6, c.u16(copy + 0x28), 0x10482320);
                field(grant, 7, c.u8(copy + 0x3f), 0x10482330);
                field(grant, 8, sext16(c.u16(frame.at(0x38))), 0x10482340);
                field(grant, 9, c.u16(frame.at(0x3a)), 0x10482350);
                o.v[5] = round * round_row::size; o.v[6] = rounds();
                o.v[5] = sext8(c.u8(o.v[6] + o.v[5] + o.v[22] + 0x2d));
                field(grant, 10, o.v[5], 0x10482374);
                o.v[5] = round * round_row::size; o.v[6] = rounds();
                o.v[5] = sext8(c.u8(o.v[6] + o.v[5] + o.v[22] + 0x2e));
                field(grant, 11, o.v[5], 0x10482398);
                file(grant, c.u32(copy + 0x1c), 0x104823a8);
                file(grant, c.u32(copy + 0x20), 0x104823b8);
            }
            o.call(0x1048a140, 0x104823c4, {c.u32(o.v[25]), copy});
            c.put16(copy + fixture::day, c.u16(when));
            c.put16(copy + fixture::year, c.u16(frame.at(0x3a)));
            o.v[3] = round * round_row::size + o.v[22];
            o.v[6] = o.v[3] + 0x2d;
            o.v[4] = o.v[3] + 0x2e;
            o.v[7] = rounds();
            o.v[3] = c.u32(o.v[24]);
            o.v[6] = c.u8(o.v[7] + o.v[6]);
            c.put8(copy + 0x3f, o.v[6]);
            o.v[4] = c.u8(o.v[7] + o.v[4]);
            c.put8(copy + 0x3b, o.v[4]);
            o.v[4] = c.u32(o.v[7] + round * round_row::size + o.v[22] + 0x28);
            o.v[6] = c.u32(cup + competition::word_24);
            o.call(0x104aff20, 0x10482410, {o.v[3], o.v[4], copy, o.v[6]});
            o.call(0x104888f0, 0x10482420, {c.u32(o.v[25]), copy, 1});
            o.call(callee::free_object, 0x10482428, {o.v[29]});
            o.v[3] = c.u32(o.v[27]);
            if (c.u32(o.v[3] + (o.v[23] << 2)) == 0) {
                close(0x10482448);
                return leave();
            }
            o.v[4] = rounds();
            o.v[5] = 0xffffffffu;
            o.v[3] = round * round_row::size + o.v[22];
            c.put8(o.v[4] + o.v[3] + 0x2e, 0xff);
        }
        // News item 0x14 from the frame's copy of the fixture.
        if (enabled(c.u32(copy + 0x1c), 0x10482470) || enabled(c.u32(copy + 0x20), 0x10482484)) {
            if (enabled(c.u32(copy + 0x1c), 0x10482498)) {
                field(item, 3, c.u32(copy + 0xc), 0x104824b0);
                field(item, 4, c.u32(copy + 0x10), 0x104824c0);
            } else {
                field(item, 3, c.u32(copy + 0x10), 0x104824d4);
                field(item, 4, c.u32(copy + 0xc), 0x104824e4);
            }
            field(item, 6, c.u32(copy + 4), 0x104824f4);
            field(item, 7, c.u16(copy + fixture::day), 0x10482504);
            field(item, 8, c.u16(copy + fixture::year), 0x10482514);
            file(item, c.u32(copy + 0x1c), 0x10482524);
            file(item, c.u32(copy + 0x20), 0x10482534);
        }
        ++o.v[28];
        o.v[22] += 8;
    }
}

void missing_side_news(Cpu& c, std::uint32_t cup, std::uint32_t round_arg, std::uint32_t tie_arg) {
    const Frame frame(c, 0x104860a0, 0x150, {.first_gpr = 29});
    Original o(c);
    const auto item = frame.at(0x50);
    // sp+0x38 the first side's club, +0x3c the second's, +0x40/+0x44 the clubs of the tie that feeds the first side,
    // +0x48/+0x4c of the one that feeds the second (each starts as a word of the static buffers at TOC -0x47c8 /
    // -0x47cc and is replaced by the club when the tie has one).
    const auto first_club = frame.at(0x38), second_club = frame.at(0x3c), feeder1_a = frame.at(0x40), feeder1_b = frame.at(0x44),
               feeder2_a = frame.at(0x48), feeder2_b = frame.at(0x4c);
    o.v[29] = cup; o.v[30] = round_arg; o.v[31] = c.toc(toc::news);
    const auto statics_a = c.toc(toc::buffer_47cc), statics_b = c.toc(toc::buffer_47c8);
    const auto rounds = [&] { return c.u32(cup + cup::rounds); };
    const auto entrants = [&] { return c.u32(cup + cup::entrants); };
    const auto matches = [&] { return c.u32(cup + cup::round_matches); };
    o.v[7] = statics_a;
    o.v[3] = sext16(round_arg) << 2;
    const auto tie_offset = sext16(tie_arg) * tie::size;
    o.v[4] = matches();
    o.v[6] = c.u32(o.v[7]);
    o.v[3] = c.u32(o.v[4] + o.v[3]);
    o.v[4] = statics_b;
    o.v[3] += tie_offset;
    o.v[5] = c.u32(o.v[7] + 4);
    const auto tie_ptr = o.v[3];
    o.v[9] = c.u32(o.v[4]);
    o.v[8] = c.u32(o.v[4] + 4);
    o.v[4] = c.u32(o.v[7] + 8);
    c.put32(first_club, o.v[9]);
    c.put32(second_club, o.v[8]);
    c.put32(feeder1_a, o.v[6]);
    c.put32(feeder1_b, o.v[5]);
    c.put32(feeder2_a, o.v[4]);
    c.put32(feeder2_b, c.u32(o.v[7] + 12));
    const auto first = std::int32_t(sext16(c.u16(tie_ptr + tie::first)));
    const auto second = std::int32_t(sext16(c.u16(tie_ptr + tie::second)));
    if (first != -1 && second != -1) return o.leave();
    o.v[4] = sext16(round_arg) * round_row::size;
    o.v[5] = rounds();
    if (std::int16_t(c.u16(o.v[5] + o.v[4] + round_row::ties)) < 2) return o.leave();
    // The club of a feeder tie's first / second side goes to `a` / `b` when it is not null; nothing is done (the whole
    // routine ends) when the tie, or either of its sides, is unknown.
    const auto feed = [&](std::uint32_t round_at, std::uint32_t match_at, std::uint32_t a, std::uint32_t b) -> bool {
        o.v[4] = sext16(c.u16(tie_ptr + round_at));
        if (std::int32_t(o.v[4]) == -1) return false;
        const auto match = sext16(c.u16(tie_ptr + match_at));
        o.v[5] = o.v[4] << 2;
        o.v[6] = matches();
        o.v[4] = match * tie::size;
        o.v[6] = c.u32(o.v[6] + o.v[5]);
        o.v[5] = sext16(c.u16(o.v[6] + o.v[4]));
        if (std::int32_t(o.v[5]) == -1) return false;
        if (std::int32_t(sext16(c.u16(o.v[6] + o.v[4] + tie::second))) == -1) return false;
        o.v[4] = entrants();
        const auto club = c.u32(o.v[4] + o.v[5] * 8);
        if (club != 0) c.put32(a, club);
        const auto match_again = sext16(c.u16(tie_ptr + match_at)), round_again = sext16(c.u16(tie_ptr + round_at));
        o.v[4] = match_again * tie::size;
        o.v[6] = matches();
        o.v[7] = entrants();
        o.v[5] = c.u32(o.v[6] + round_again * 4);
        const auto other = c.u32(o.v[7] + sext16(c.u16(o.v[5] + o.v[4] + tie::second)) * 8);
        if (other != 0) c.put32(b, other);
        return true;
    };
    if (first == -1) {
        if (!feed(tie::first_feeder_round, tie::first_feeder_match, feeder1_a, feeder1_b)) return o.leave();
    } else {
        o.v[4] = entrants();
        const auto club = c.u32(o.v[4] + std::uint32_t(first) * 8);
        if (club != 0) c.put32(first_club, club);
    }
    if (second == -1) {
        const auto round_value = sext16(c.u16(tie_ptr + tie::second_feeder_round));
        o.v[4] = round_value;
        if (std::int32_t(o.v[4]) == -1) return o.leave();
        const auto match = sext16(c.u16(tie_ptr + tie::second_feeder_match));
        o.v[5] = o.v[4] << 2;
        o.v[6] = matches();
        o.v[4] = match * tie::size;
        o.v[6] = c.u32(o.v[6] + o.v[5]);
        o.v[5] = sext16(c.u16(o.v[6] + o.v[4]));
        if (std::int32_t(o.v[5]) == -1) return o.leave();
        if (std::int32_t(sext16(c.u16(o.v[6] + o.v[4] + tie::second))) == -1) return o.leave();
        o.v[4] = entrants();
        const auto club = c.u32(o.v[4] + o.v[5] * 8);
        if (club != 0) c.put32(feeder2_a, club);
        const auto match_again = sext16(c.u16(tie_ptr + tie::second_feeder_match)), round_again = sext16(c.u16(tie_ptr + tie::second_feeder_round));
        o.v[3] = match_again * tie::size;
        o.v[5] = matches();
        o.v[4] = round_again * 4;
        o.v[6] = entrants();
        o.v[4] = c.u32(o.v[5] + o.v[4]);
        const auto other = c.u32(o.v[6] + sext16(c.u16(o.v[4] + o.v[3] + tie::second)) * 8);
        if (other != 0) c.put32(feeder2_b, other);
    } else {
        o.v[3] = entrants();
        const auto club = c.u32(o.v[3] + std::uint32_t(second) * 8);
        if (club != 0) c.put32(second_club, club);
    }
    // The item.
    o.call(item::start, 0x104862cc, {item, 0x26, 0});
    o.v[5] = c.u32(cup + competition::record); o.v[5] = c.u32(o.v[5]);
    o.call(item::set_field, 0x104862e0, {item, 0, o.v[5]});
    o.v[5] = sext16(c.u16(cup + competition::code));
    o.call(item::set_field, 0x104862f0, {item, 1, o.v[5]});
    o.v[6] = rounds();
    o.v[5] = sext16(round_arg) * round_row::size;
    o.v[5] = sext16(c.u16(o.v[6] + o.v[5] + round_row::label));
    o.call(item::set_field, 0x10486310, {item, 2, o.v[5]});
    if (c.u32(first_club) != 0) {
        o.call(item::set_field, 0x1048632c, {item, 3, c.u32(c.u32(first_club))});
        o.call(item::set_field, 0x1048633c, {item, 4, 0xffffffffu});
    } else {
        o.call(item::set_field, 0x10486354, {item, 3, c.u32(c.u32(feeder1_a))});
        o.call(item::set_field, 0x10486368, {item, 4, c.u32(c.u32(feeder1_b))});
    }
    if (c.u32(second_club) != 0) {
        o.call(item::set_field, 0x10486384, {item, 5, c.u32(c.u32(second_club))});
        o.call(item::set_field, 0x10486394, {item, 6, 0xffffffffu});
    } else {
        o.call(item::set_field, 0x104863ac, {item, 5, c.u32(c.u32(feeder2_a))});
        o.call(item::set_field, 0x104863c0, {item, 6, c.u32(c.u32(feeder2_b))});
    }
    const auto file_if = [&](std::uint32_t club, std::uint32_t back) {
        if (club != 0) o.call(item::file_under_club, back, {c.u32(o.v[31]), item, club});
    };
    if (c.u32(first_club) != 0) {
        o.v[5] = c.u32(first_club);
        file_if(o.v[5], 0x104863d8);
    } else {
        o.v[5] = c.u32(feeder1_a);
        file_if(o.v[5], 0x104863f4);
        o.v[5] = c.u32(feeder1_b);
        file_if(o.v[5], 0x1048640c);
    }
    if (c.u32(second_club) != 0) {
        o.v[5] = c.u32(second_club);
        file_if(o.v[5], 0x10486424);
    } else {
        o.v[5] = c.u32(feeder2_a);
        file_if(o.v[5], 0x10486440);
        o.v[5] = c.u32(feeder2_b);
        file_if(o.v[5], 0x10486458);
    }
    o.leave();
}
}
