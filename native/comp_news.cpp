// COMP.CPP, block `news`: the three news items a competition files. Each builds one item in a frame buffer (the
// news manager's 0x102bdf90 starts it, 0x102cb4b0 sets field n) and files it (0x102cc140 under a club, 0x102cc280
// under a competition). The manager and the item routines are other modules' (NEWS.CPP): called through the
// dispatcher. Their calls can reach imports, so the non-volatile registers the original holds are set first.
#include "comp_detail.hpp"
#include "comp_news.hpp"

namespace cm::aot::comp {
namespace {
namespace news_item {
inline constexpr std::uint32_t start = 0x102bdf90, set_field = 0x102cb4b0, file_under_club = 0x102cc140,
                               file_under_competition = 0x102cc280, enabled = 0x102cdbf0;
}
// The competition's record id: **(competition + 4).
std::uint32_t record_id(Cpu& c, std::uint32_t competition) { return c.u32(c.u32(competition + competition::record) + record::id); }
void set_field(Cpu& c, std::uint32_t item, std::uint32_t field, std::uint32_t value, std::uint32_t back) {
    invoke(c, news_item::set_field, back, {item, field, value});
}
}

void news_prize_money(Cpu& c, std::uint32_t competition, std::uint32_t club, std::uint32_t amount_high, std::uint32_t amount_low,
                      std::uint32_t place, std::uint32_t round, std::uint32_t other, std::uint32_t flag) {
    const Frame frame(c, 0x10452010, 0x150, {.first_gpr = 23});
    const auto news = c.toc(toc::news);
    // r23 competition, r24 low, r25 high, r26 place, r27 round, r28 flag, r29 club, r30 other, r31 the news cell.
    c.set({{23, competition}, {24, amount_low}, {25, amount_high}, {26, place}, {27, round}, {28, flag}, {29, club},
           {30, other}, {31, news}});
    if (std::int8_t(invoke(c, news_item::enabled, 0x1045204c, {c.u32(news)})) == 0) return;
    // A signed 64-bit compare of the amount with 50 times the club's halfword (the xoris/subfc/subfe chain).
    const std::int64_t amount = std::int64_t((std::uint64_t(amount_high) << 32) | amount_low);
    const std::int64_t threshold = std::int32_t(std::uint32_t(std::int32_t(c.i16(club + 0x1ec)) * 50));
    if (amount < threshold) return;
    const auto item = frame.at(0x38);
    invoke(c, news_item::start, 0x1045208c, {item, 0x1a, 0});
    set_field(c, item, 0, record_id(c, competition), 0x104520a0);
    set_field(c, item, 1, amount_low, 0x104520b0);
    set_field(c, item, 2, c.u32(club), 0x104520c0);
    set_field(c, item, 3, std::uint32_t(std::int32_t(std::int8_t(place))), 0x104520d0);
    set_field(c, item, 4, std::uint32_t(std::int32_t(std::int16_t(round))), 0x104520e0);
    set_field(c, item, 7, std::uint32_t(std::int32_t(std::int8_t(flag))), 0x104520f0);
    if (other == 0) {
        set_field(c, item, 5, 0xffffffffu, 0x10452250);
        set_field(c, item, 6, 0, 0x10452260);
    } else if (club == c.u32(other + 0x1c)) {
        // The club is the other competition's first side: its second side's flags decide field 6.
        set_field(c, item, 5, c.u32(c.u32(other + 0x20)), 0x10452118);
        if (c.i8(other + 0x3d) != 0) set_field(c, item, 6, 1, 0x10452134);
        else if (c.i8(other + 0x4b) > c.i8(other + 0x4c)) set_field(c, item, 6, 1, 0x10452160);
        else if (c.i8(other + 0x43) > c.i8(other + 0x44)) set_field(c, item, 6, 1, 0x1045218c);
        else set_field(c, item, 6, 0, 0x104521a0);
    } else {
        // r3 still holds the word at other+0x1c from the compare: the field 5 is the word it points at.
        set_field(c, item, 5, c.u32(c.u32(other + 0x1c)), 0x104521b4);
        if (c.i8(other + 0x3e) != 0) set_field(c, item, 6, 1, 0x104521d0);
        else if (c.i8(other + 0x4b) < c.i8(other + 0x4c)) set_field(c, item, 6, 1, 0x104521fc);
        else if (c.i8(other + 0x43) < c.i8(other + 0x44)) set_field(c, item, 6, 1, 0x10452228);
        else set_field(c, item, 6, 0, 0x1045223c);
    }
    invoke(c, news_item::file_under_club, 0x10452270, {c.u32(news), item, club});
}

void news_competition_year(Cpu& c, std::uint32_t competition, std::uint32_t club, std::uint32_t value) {
    const Frame frame(c, 0x104527c0, 0x130, {.first_gpr = 28});
    const auto news = c.toc(toc::news);
    // r4 and r5 still hold the arguments at the first call, which sets r3 alone: a direct C++ caller has not
    // put them in the registers, and the news manager reads what is there.
    c.set({{4, club}, {5, value}, {28, competition}, {29, club}, {30, value}, {31, news}});
    if (std::int8_t(invoke(c, news_item::enabled, 0x104527f4, {c.u32(news)})) == 0) return;
    const auto item = frame.at(0x38);
    invoke(c, news_item::start, 0x1045280c, {item, 0x1f, 0});
    set_field(c, item, 0, c.u32(club), 0x1045281c);
    set_field(c, item, 1, record_id(c, competition), 0x10452830);
    set_field(c, item, 2, value, 0x10452840);
    set_field(c, item, 3, std::uint32_t(std::int32_t(c.i16(competition + competition::year))), 0x10452850);
    invoke(c, news_item::file_under_club, 0x10452860, {c.u32(news), item, club});
}

void news_movement(Cpu& c, std::uint32_t competition, std::uint32_t third, std::uint32_t first, std::uint32_t second,
                   std::uint32_t kind) {
    const Frame frame(c, 0x10452a60, 0x170, {.first_gpr = 24});
    const auto news = c.toc(toc::news), message = c.toc(toc::message), pool = c.toc(toc::pool);
    // r24 competition, r25 third, r26 first, r27 second, r28 kind, r29 the news cell, r30 the message buffer, r31 the pool.
    c.set({{24, competition}, {25, third}, {26, first}, {27, second}, {28, kind}, {29, news}, {30, message}, {31, pool}});
    if (first == 0) {
        report(c, frame, {0x10452aa8, 0x40, 0x38, 0x895});
        return;
    }
    const auto item = frame.at(0x60);
    invoke(c, news_item::start, 0x10452afc, {item, 0x23, 0});
    set_field(c, item, 0, record_id(c, competition), 0x10452b10);
    set_field(c, item, 1, c.u32(first), 0x10452b20);
    set_field(c, item, 2, second ? c.u32(second) : 0xffffffffu, 0x10452b40);
    set_field(c, item, 3, third ? c.u32(third) : 0xffffffffu, 0x10452b60);
    set_field(c, item, 4, std::uint32_t(std::int32_t(std::int8_t(kind))), 0x10452b70);
    invoke(c, news_item::file_under_competition, 0x10452b80, {c.u32(news), item, c.u32(competition + competition::record)});
    if (third) invoke(c, news_item::file_under_competition, 0x10452b98, {c.u32(news), item, third});
}
}
