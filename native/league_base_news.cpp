// LEAGUE.CPP, block `news`: the league's prize money and the news items it files. The items write news records
// (NEWS.CPP's manager) and the prizes write club cash through 0x10103510/0x100f5730: they are called through the
// dispatcher; the non-volatile registers the original holds are set before each call.
#include "date.hpp"
#include "league_base_news_detail.hpp"

namespace cm::aot::league_base {
using namespace news;

std::uint32_t prize_total(Cpu& c, std::uint32_t league, std::uint32_t amount) {
    const Frame frame(c, 0x1049f540, 0x80, {.first_gpr = 27});
    const auto clubs = std::int32_t(c.i16(league + competition::club_count));
    if (clubs == 0) {
        // r27 the league, r28 the message buffer, r29 the pool, r30 the amount, r31 the running sum (0).
        c.set({{27, league}, {28, c.toc(toc::message)}, {29, c.toc(toc::pool)}, {30, amount}, {31, 0}});
        report(c, frame, {0x1049f584, 0x40, 0x38, 0x214});
    }
    // The original unrolls the sum by eight; the terms are (place + 1) * amount either way.
    std::uint32_t total = 0;
    for (std::int32_t place = 0; place < clubs; ++place) total += std::uint32_t(place + 1) * amount;
    return total;
}

void file_prize_news(Cpu& c, std::uint32_t league, std::uint32_t value, std::uint32_t only) {
    const Frame frame(c, 0x1049f6d0, 0x50, {.first_gpr = 26});
    const auto cash = c.toc(toc_slot::club_cash);
    if (c.u32(cash) == 0) return;
    Held h(c);
    h.v[26] = league; h.v[27] = value; h.v[28] = only; h.v[31] = cash;
    h.v[29] = 0; h.v[30] = 0;
    for (; std::int16_t(h.v[29]) < c.i16(league + competition::club_count); ++h.v[29], h.v[30] += league::standing_row) {
        const auto club = c.u32(c.u32(league + league::standings) + h.v[30]);
        if (only != 0 && c.u32(club + 0x34) != only) continue;
        h.publish();
        const auto cash_row = invoke(c, 0x10103510, 0x1049f734, {c.u32(cash), club});
        invoke(c, 0x100f5730, 0x1049f73c, {cash_row, value});
        invoke(c, 0x104527c0, 0x1049f750, {league, c.u32(c.u32(league + league::standings) + h.v[30]), value});
    }
}

void file_first_round_news(Cpu& c, std::uint32_t league) {
    const Frame frame(c, 0x104a0e40, 0x80, {.first_gpr = 23});
    Held h(c);
    const auto manager = c.toc(toc::news);
    const auto fresh = frame.at(0x50), best = frame.at(0x48), later = frame.at(0x40);
    auto& item = h.v[28];
    h.v[25] = league; item = 0; h.v[31] = manager;
    const auto clubs = [&] { return std::int32_t(c.i16(league + competition::club_count)); };
    const auto club_at = [&](std::uint32_t offset) { return c.u32(c.u32(league + league::standings) + offset); };
    const auto field = [&](std::uint32_t n, std::uint32_t value, std::uint32_t back) {
        h.publish();
        invoke(c, item::set_field, back, {item, n, value});
    };
    if (clubs() > 8) return;
    h.v[27] = 0; h.v[29] = 0;
    for (; std::int8_t(h.v[27]) < clubs(); ++h.v[27], h.v[29] += league::standing_row) {
        h.publish();
        if (std::int8_t(invoke(c, item::enabled, 0x104a0e84, {c.u32(manager), club_at(h.v[29])})) == 0) continue;
        if (item == 0) {
            h.publish();
            item = invoke(c, comp::callee::allocate_object, 0x104a0e9c, {0xe4});
            if (item) {
                h.publish();
                invoke(c, item::start, 0x104a0eb0, {item, 0x15, 0});
            }
            // The first round's date, then the earliest of its weekday slots moved to the nearest such weekday.
            const auto rounds = c.u32(league + league::schedule);
            h.publish();
            c.link(0x104a0ed0);
            date::from_day_and_year(c, fresh, sext16(c.u16(rounds)),
                                    std::uint16_t(c.u16(rounds + Round::year) + std::uint32_t(c.i16(league + competition::year))));
            h.publish();
            date::set_default(c, best);
            h.publish();
            date::set_default(c, later);
            c.put16(best, c.u16(fresh));
            c.put16(best + 2, c.u16(fresh + 2));
            c.put8(best + 4, c.u8(fresh + 4));
            h.v[26] = 0; h.v[30] = 0;
            while (std::int8_t(h.v[26]) < 8) {
                const auto row = c.u32(league + league::schedule) + h.v[30];
                if (std::int8_t(c.u8(row + 0xe)) == -1) break;
                h.publish();
                c.link(0x104a0f34);
                date::nearest_weekday(c, fresh, sext8(c.u8(row + 0xc)), later);
                const auto best_year = c.u16(best + 2), later_year = c.u16(later + 2);
                bool earlier;
                if (best_year > later_year) earlier = true;
                else if (best_year < later_year) earlier = false;
                else {
                    // The days compared by the original's xor/srawi/and/subf trick (the sign bit of the result).
                    const auto a = std::int32_t(c.i16(best)), b = std::int32_t(c.i16(later));
                    const auto x = a ^ b;
                    earlier = ((std::uint32_t(x >> 1) - std::uint32_t(x & a)) >> 31) != 0;
                }
                if (earlier) {
                    c.put16(best + 2, later_year);
                    c.put16(best, c.u16(later));
                    c.put8(best + 4, c.u8(later + 4));
                }
                h.v[30] += 8;
                ++h.v[26];
            }
            field(0, c.u32(c.u32(league + competition::record)), 0x104a0fcc);
            field(1, sext16(c.u16(league + competition::code)), 0x104a0fdc);
            field(3, sext16(c.u16(best)), 0x104a0fec);
            field(4, c.u16(best + 2), 0x104a0ffc);
            field(5, std::uint32_t(clubs() - 1), 0x104a1010);
        }
        field(2, c.u32(club_at(h.v[29])), 0x104a1028);
        // The other clubs, from field 6.
        h.v[26] = sext8(h.v[27]); h.v[24] = 0; h.v[23] = 6; h.v[30] = 0;
        while (std::int8_t(h.v[24]) < clubs()) {
            if (std::int8_t(h.v[24]) != std::int32_t(h.v[26])) {
                const auto n = h.v[23]++;
                field(n, c.u32(club_at(h.v[30])), 0x104a1064);
            }
            h.v[30] += league::standing_row;
            ++h.v[24];
        }
        h.publish();
        invoke(c, item::file_under_club, 0x104a1090, {c.u32(manager), item, club_at(h.v[29])});
    }
    if (item) {
        h.publish();
        invoke(c, comp::callee::free_object, 0x104a10bc, {item});
    }
}

std::uint32_t file_result_news(Cpu& c, std::uint32_t league, std::uint32_t club, std::uint32_t kind, std::uint32_t stage) {
    const Frame frame(c, 0x104a44f0, 0x150, {.first_gpr = 23});
    Held h(c);
    const auto records = c.toc(toc::records), manager = c.toc(toc::news), competitions = c.toc(toc::competitions);
    const auto record = [&] { return c.u32(league + competition::record); };
    h.v[24] = league; h.v[25] = club; h.v[26] = kind; h.v[27] = stage;
    h.v[29] = records; h.v[30] = manager; h.v[31] = competitions;
    h.v[28] = sext16(c.u16(league + competition::code));
    const auto out = frame.at(0x38), item = frame.at(0x3c);
    c.put8(out, 0xff);
    h.publish();
    if (std::int8_t(invoke(c, 0x10494d20, 0x104a4538, {c.u32(c.toc(toc_slot::fixture_rules)), record()})) != 0) return 0;
    const auto table = c.u32(competitions);
    const auto own = c.u32(table + c.u32(record()) * 4);
    c.set(10, table);
    h.publish();
    const auto answer = virtual_call(c, 0x4c, 0x104a4580, {own, club, kind, stage, out, 0, 0});
    h.v[23] = answer;
    if (std::int32_t(answer) == -1) return 0;
    if (std::int8_t(c.u8(league + competition::owner)) == -1 && std::int8_t(stage) != -1)
        h.v[28] = sext16(c.u16(c.u32(c.u32(league + competition::stages) + sext8(stage) * 4) + competition::code));
    const auto field = [&](std::uint32_t n, std::uint32_t value, std::uint32_t back) {
        h.publish();
        invoke(c, news::item::set_field, back, {item, n, value});
    };
    h.publish();
    invoke(c, news::item::start, 0x104a45d4, {item, 0xc, 0});
    field(0, c.u32(club), 0x104a45e4);
    field(1, c.u32(record()), 0x104a45f8);
    field(2, sext8(stage), 0x104a4608);
    field(3, sext8(kind), 0x104a4618);
    field(4, sext16(h.v[28]), 0x104a4628);
    field(5, answer, 0x104a4638);
    const auto filed = [&](std::uint32_t entry, std::uint32_t target, std::uint32_t back) {
        h.publish();
        invoke(c, entry, back, {c.u32(manager), item, target});
    };
    // The record of the competition `own`'s class names (+0x1c / +0x20: a second competition's record id, or -1).
    const auto related = [&](std::uint32_t slot, std::uint32_t back) {
        const auto id = c.u32(c.u32(c.u32(competitions) + c.u32(record()) * 4) + slot);
        if (id != 0xffffffffu) filed(news::item::file_under_record, c.u32(records) + id * 0x6c, back);
    };
    const auto out_byte = c.u8(out);
    c.set(9, out_byte);  // `lbz r9,0x38(r1)` at 0x104a4638 stays in r9 on every path that files by a hook that leaves it
    if (std::int8_t(out_byte) != -1) {
        const auto holder = c.u32(club + 0x2c);
        h.publish();
        invoke(c, news::item::file_with_competition, 0x104a4674,
               {c.u32(manager), item, club, record(), holder, holder ? c.u32(holder + 0x74) : 0, out_byte});
        return 1;
    }
    switch (std::int8_t(kind)) {
    case 5:
        if (c.u32(league + competition::parent) == 0xffffffffu)
            filed(news::item::file_under_nation, c.u32(record() + 8), 0x104a46a4);
        else filed(news::item::file_under_record, record(), 0x104a46b8);
        break;
    case 0:
        filed(news::item::file_under_record, record(), 0x104a46d4);
        related(0x1c, 0x104a470c);
        break;
    case 7: case 6:
        filed(news::item::file_under_club, club, 0x104a4730);
        break;
    case 1:
        filed(news::item::file_under_record, record(), 0x104a474c);
        break;
    case 2:
        filed(news::item::file_under_record, record(), 0x104a4768);
        break;
    case 3:
        filed(news::item::file_under_record, record(), 0x104a4784);
        related(0x20, 0x104a47bc);
        break;
    case -2:
        filed(news::item::file_under_club, club, 0x104a47d8);
        break;
    default: break;
    }
    return 1;
}

std::uint32_t record_place(Cpu& c, std::uint32_t league, std::uint32_t club, std::uint32_t kind) {
    const Frame frame(c, 0x104a47f0, 0x40, {});
    const auto cell = c.toc(toc_slot::history);
    c.set({{6, sext8(kind)}, {7, cell}, {8, league}});
    const auto record = c.u32(league + competition::record);
    switch (std::int8_t(kind)) {
    case 5: invoke(c, 0x10381b70, 0x104a481c, {c.u32(cell), club, record}); break;
    case 0: invoke(c, 0x103822e0, 0x104a4838, {c.u32(cell), club, record, 100}); break;
    case 3: invoke(c, 0x10382e50, 0x104a4850, {c.u32(cell), club, record}); break;
    default: break;
    }
    return 0;
}
}
