// The rb_*.cpp rulebooks (ruling_bodies.hpp). Each routine keeps the original's frame: the routines it
// calls leave their own frames below it, and the date routines' leftovers have known readers.
#include "ruling_bodies.hpp"
#include "date.hpp"

namespace cm::aot::ruling_bodies {
using guest::Cpu;
using guest::Frame;

namespace {
namespace toc {
constexpr std::int32_t message = -0x7ba8, success = -0x7bac, nations = -0x7b98, bans = -0x7aa4, today = -0x7b80,
                       persons = -0x7be0;
}
// Routines outside the family, called through the dispatcher: the first three are still generated;
// the two of DISCIPLINE.CPP are C++ and stay behind the dispatcher so that the checks can script them.
namespace callee {
constexpr std::uint32_t nation_text = 0x10168a00, base_construct = 0x105bb3e0, booking_record = 0x105bc2b0,
                        register_ban = 0x100dfb30, post_ban_news = 0x100e0700;
}
constexpr std::uint32_t nation_size = 0x128, person_size = 0x70;

// The cards of one player in one match, as the match hands them over.
struct MatchCards {
    Cpu& c;
    std::uint32_t at;
    std::uint32_t person() const { return c.u32(at); }
    unsigned yellows() const { return c.u8(at + 0x2c); }
    unsigned reds() const { return c.u8(at + 0x2d); }
    unsigned flag_48() const { return c.u8(at + 0x48); }
};
}

std::uint32_t nation_text(Cpu& c, const NationText& unit, std::uint32_t buffer, std::uint32_t width,
                          std::uint32_t option) {
    Frame frame(c, unit.entry, 0x70, {.first_gpr = 30});
    const auto pool = c.toc(unit.file);
    c.set({{31, pool}, {30, c.toc(toc::message)}});
    if (std::int32_t(width) < 20) {
        guest::report_assertion(c, {.at = unit.entry + 0x38, .file = pool, .format = pool + unit.format,
                                    .title = pool + unit.title, .name = c.sp() + 0x40, .extension = c.sp() + 0x38,
                                    .line = unit.line});
        return c.toc(toc::success);
    }
    const auto nation = c.u32(c.toc(toc::nations)) + c.u32(c.toc(unit.nation)) * nation_size;
    c.set({{3, nation}, {4, buffer}, {5, std::uint32_t(std::int16_t(width))}, {6, option}});
    c.call(callee::nation_text, unit.entry + 0x9c);
    return c.r(3);
}

std::uint32_t construct(Cpu& c, const Rulebook& unit, std::uint32_t self, std::uint32_t owner) {
    Frame frame(c, unit.entry, 0x40, {.first_gpr = 31});
    c.set({{31, self}, {3, self}, {4, owner}});
    c.call(callee::base_construct, unit.entry + 0x18);
    c.put32(self, c.toc(unit.table));
    c.put8(self + 4, 0);
    c.put8(self + 5, unit.body);
    c.put8(self + 6, unit.byte_6);
    c.put32(self + 0x18, c.u32(c.toc(unit.nation)));
    c.put32(self + 0x1c, c.u32(c.toc(unit.group)));
    c.put8(self + 0x20, unit.body);
    c.put8(self + 0x21, 0xff);
    c.put8(self + 0x22, 1);
    c.put8(self + 0x23, 0);
    return self;
}

bool ireland::next_card_bans(Cpu& c, std::uint32_t tally, std::uint32_t matches, std::uint32_t dismissal) {
    if (matches != 0) c.put8(matches, 1);
    if (dismissal != 0) c.put8(dismissal, 0xff);
    const unsigned cards = c.u8(tally + 4);
    return cards >= 4 && cards % 3 == 1;
}

bool northern_ireland::next_card_bans(Cpu& c, std::uint32_t booking, std::uint32_t matches, std::uint32_t dismissal) {
    if (dismissal != 0) c.put8(dismissal, 0xff);
    const unsigned points = c.u16(booking + 8);
    struct Level { unsigned points, matches; };
    for (const auto level : {Level{20, 1}, Level{32, 2}, Level{42, 4}, Level{51, 6}}) {
        if (points < level.points - 4 || points >= level.points) continue;
        if (matches != 0) c.put8(matches, level.matches);
        return true;
    }
    return false;
}

void northern_ireland::clear_points(Cpu& c, std::uint32_t self) {
    auto record = c.u32(self + 0xc);
    for (std::int32_t n = 0; n < c.i32(self + 0x14); ++n, record += 0x10) c.put16(record + 8, 0);
}

void northern_ireland::report_cards(Cpu& c, std::uint32_t self, std::uint32_t match, std::uint32_t fixture) {
    Frame frame(c, 0x106abfe0, 0x80, {.first_gpr = 24});
    const MatchCards cards{c, match};
    const auto bans = c.toc(toc::bans), today = c.toc(toc::today);
    std::uint32_t matches = 0, reason = UINT32_MAX, threshold = 0;  // threshold is set wherever matches is
    c.set({{25, match}, {26, fixture}, {28, reason}, {29, matches}, {30, bans}, {31, today}});
    if (cards.yellows() == 0 && cards.reds() == 0) return;

    c.set({{3, self}, {4, cards.person()}, {5, 1}});
    c.call(callee::booking_record, 0x106ac02c);
    const auto points_at = c.r(3) + 8;
    const std::uint32_t before = c.u16(points_at);
    if (cards.reds() == 1) {
        if (cards.yellows() == 1) c.put16(points_at, before + 12);
        matches = 1;
        reason = 2;
        threshold = 1;
    }
    // The counter is compared as a signed byte against the unsigned count, re-read every turn.
    for (std::uint32_t n = 0; std::int32_t(std::int8_t(n)) < std::int32_t(cards.yellows()); ++n)
        c.put16(points_at, c.u16(points_at) + 4u);
    const unsigned points = c.u16(points_at);
    const auto passed = [&](unsigned level) { return points >= level && std::int16_t(before) < int(level); };
    if (passed(51)) {
        c.put16(points_at, 0);
        reason = 0, threshold = 51, matches += 6;
    } else if (passed(42)) {
        reason = 0, threshold = 42, matches += 4;
    } else if (passed(32)) {
        reason = 0, threshold = 32, matches += 2;
    } else if (passed(20)) {
        reason = 0, threshold = 20, matches += 1;
    }
    if (std::int8_t(matches) == 0) return;

    // The ban record, 0x18 bytes at +0x40 of the frame; the date routines get +0x38 as scratch.
    const auto ban = c.sp() + 0x40, start = ban + 4, next = c.sp() + 0x38;
    const date::Date from(c, today), starts(c, start), moved(c, next);
    const auto start_on = [&](const date::Date& day) {
        starts.set_year(day.year());
        starts.set_day(std::uint16_t(day.day()));
        starts.set_leap(day.leap());
    };
    c.set({{24, start}, {27, threshold}, {28, reason}, {29, matches}});
    c.link(0x106ac128);
    date::set_default(c, start);
    c.put8(ban, matches);
    c.put16(ban + 2, 0xffff);
    c.put8(ban + 1, 0);
    start_on(from);
    for (;;) {
        c.link(0x106ac188);
        if (date::weekday(c, start) == 0) break;
        c.link(0x106ac168);
        date::add_days(c, next, start, 1);
        start_on(moved);
    }
    // The record is never initialised as a whole. Of the flags byte at +0x14, bit 6 is cleared and bit 5
    // set from the match's byte; its other six bits, and the bytes +9..+0xb, +0x11 and +0x15..+0x17, are
    // whatever an earlier call left at these stack addresses.
    const auto flags = c.u8(ban + 0x14) & ~0x40u;
    c.put8(ban + 0x10, reason);
    const auto manager = c.u32(bans);
    c.put8(ban + 0x14, flags);
    c.put16(ban + 0x12, threshold);
    c.put8(ban + 0xc, rulebook.body);
    c.put8(ban + 0xd, 0xff);
    c.put8(ban + 0xe, 0xff);
    c.put8(ban + 0xf, 0xff);
    c.put8(ban + 0x14, (flags & ~0x20u) | (cards.flag_48() == 0 ? 0x20u : 0));
    c.set({{3, manager}, {4, cards.person()}, {5, ban}});
    c.call(callee::register_ban, 0x106ac1e4);
    c.set({{3, c.u32(bans)}, {4, c.u32(c.toc(toc::persons)) + cards.person() * person_size}, {5, ban}, {6, fixture}});
    c.call(callee::post_ban_news, 0x106ac208);
}
}
