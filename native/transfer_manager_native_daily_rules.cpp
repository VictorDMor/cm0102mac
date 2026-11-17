#include "transfer_manager_native_daily.hpp"
#include "transfer_manager_native_detail.hpp"
#include <array>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
using daily::Lookup;
constexpr std::uint32_t call_vector = 0x10781320, news_check = 0x102cdbf0, ready_offer = 0x103ec440, blocked_by = 0x103fb390,
                        forward_offer = 0x1040ce40, offer_to_recipient = 0x103e44b0, offer_home = 0x103e4130,
                        set_stage = 0x103eb4a0, decide_offer = 0x103e8280, nation_rules = 0x100c76c0, rules_of = 0x103f8380,
                        nation_round = 0x103e9ce0, today_of = 0x10715880, month_of = 0x10718360, previous_date = 0x10717a10,
                        next_date = 0x10717b20, make_date = 0x107156f0;
constexpr std::uint32_t stage_at = 0x34, kind_at = 0x38;
constexpr std::uint32_t stage_decided = 0xe, stage_closed = 0x13;

std::uint32_t found_club(const Lookup& look, std::uint32_t found) { return look.club_of_found(found); }
}

void process_nation_offers(Cpu& c, std::uint32_t manager, std::uint32_t nation) {
    Frame frame(c, 0x103e9ce0, 0x70, {.first_gpr = 19});
    Lookup look{c};
    const auto date_cell = c.toc(-0x7b80), news_cell = c.toc(toc::news);
    keep(c, 30, manager);
    keep(c, 31, nation);
    keep(c, 23, date_cell);
    keep(c, 24, news_cell);
    keep(c, 25, look.people);
    keep(c, 26, look.contracts);
    keep(c, 27, look.found_cell);
    keep(c, 28, look.clubs);
    const auto nation_object = invoke(c, nation_rules, 0x103e9d18, {nation});
    const auto rules = invoke(c, rules_of, 0x103e9d24, {manager, nation_object});
    keep(c, 21, 0);
    keep(c, 19, rules);
    keep(c, 22, 0);
    // The nation's rule object's method +0x18: whether it keeps the offer for itself.
    const auto rule_keeps = [&](std::uint32_t offer, std::uint32_t club, std::uint32_t back) {
        c.set({{3, rules}, {4, offer}, {5, club}, {6, date_cell}, {7, 0}});
        c.set(12, c.u32(c.u32(rules) + 0x18));
        c.call(call_vector, back);
        c.set(2, c.u32(frame.at(0x14)));
        return std::int8_t(c.r(3)) != 0;
    };
    const auto news_says = [&](std::uint32_t club, std::uint32_t back) {
        return std::int8_t(invoke(c, news_check, back, {c.u32(news_cell), club})) != 0;
    };
    // Opens the offer: stage 1 and its phase 1, the news about its club, and the hand-over to the recipients or the
    // club's forward; then, unless the club is blocked, stage 3.
    const auto open_offer = [&](std::uint32_t offer, std::uint32_t slot, bool flagged, bool second_pass) {
        c.put8(offer + stage_at, 1);
        c.put8(offer + kind_at, (c.u8(offer + kind_at) & 0xf) | 0x10);
        invoke(c, ready_offer, second_pass ? 0x103ea0f8 : 0x103e9e34, {manager, offer});
        bool forward = news_says(look.offer_club_of(offer), second_pass ? 0x103ea110 : 0x103e9e4c);
        if (!forward) {
            if (flagged) keep(c, 29, 0);
            auto found = look.find(look.person_of(offer), second_pass ? 0x103ea130 : 0x103e9e70);
            if (flagged) keep(c, 29, look.usable(found));
            const auto first = flagged ? (look.usable(found) ? found_club(look, found) : 0) : found_club(look, found);
            if (first != 0) {
                if (flagged) keep(c, 29, 0);
                found = look.find(look.person_of(offer), second_pass ? 0x103ea174 : 0x103e9ee0);
                if (flagged) keep(c, 29, look.usable(found));
                const auto second = flagged ? (look.usable(found) ? found_club(look, found) : 0) : found_club(look, found);
                forward = news_says(second, second_pass ? 0x103ea1a0 : 0x103e9f34);
            }
        }
        if (forward) {
            invoke_native(c, forward_offer, second_pass ? 0x103ea1b8 : 0x103e9f4c, {manager, offer, nation});
        } else {
            invoke_native(c, second_pass ? offer_home : offer_to_recipient, second_pass ? 0x103ea1d4 : 0x103e9f68,
                          {manager, slot, 0, 0});
        }
        if (std::int8_t(invoke(c, daily::club_flag, second_pass ? 0x103ea1f8 : 0x103e9f8c,
                               {look.offer_club_of(offer), 0, 1, 0, 0})) == 0) {
            c.put8(offer + stage_at, 3);
            invoke(c, set_stage, second_pass ? 0x103ea21c : 0x103e9fb0, {manager, offer, 1, 0});
        }
    };
    for (std::uint32_t index = 0; std::int32_t(index) < c.i32(manager + 4); ++index) {
        keep(c, 21, index);
        keep(c, 22, index * 4);
        const auto slot = c.u32(manager) + index * 4;
        const auto offer = c.u32(slot);
        keep(c, 20, offer);
        if (offer == 0) continue;
        if (c.u8(offer + stage_at) == stage_decided) {
            invoke_native(c, decide_offer, 0x103e9d64, {manager, offer, 1});
            continue;
        }
        bool second_look = true;
        if (c.u32(look.offer_club_of(offer) + 0x2c) == nation &&
            std::int8_t(invoke(c, blocked_by, 0x103e9d90, {manager, offer})) == 0 &&
            c.u32(look.person_of(offer) + 0x14) != 0 && (std::int32_t(c.i8(offer + kind_at)) >> 4) == 0 &&
            c.u8(offer + stage_at) != stage_closed) {
            second_look = rules != 0 && rule_keeps(offer, look.offer_club_of(offer), 0x103e9e08);
            if (!second_look) {
                open_offer(offer, slot, true, false);
                continue;
            }
        }
        if (second_look) {
            if (c.u32(offer + 4) == 0) continue;
            if (look.club_of_found(look.find(look.person_of(offer), 0x103e9fd8)) == 0) continue;
            const auto club = look.club_of_found(look.find(look.person_of(offer), 0x103ea01c));
            // Reads through club 0 when the second lookup finds nothing, as the original does.
            if (c.u32(club + 0x2c) != nation) continue;
            if (std::int8_t(invoke(c, blocked_by, 0x103ea058, {manager, offer})) != 0) continue;
            if (c.u8(offer + stage_at) == stage_closed) continue;
            if (rules != 0 && rule_keeps(offer, look.club_of_found(look.find(look.person_of(offer), 0x103ea08c)), 0x103ea0cc)) continue;
            open_offer(offer, slot, false, true);
        }
    }
}

void dispatch_rule_dates(Cpu& c, std::uint32_t manager, std::uint32_t date) {
    constexpr std::uint32_t rules_at = 0x9d8, rules_count = 0x9dc, nation_objects = 0x9e0;
    constexpr std::uint32_t method_dated = 0x34;
    Frame frame(c, 0x103ec0f0, 0xb0, {.first_gpr = 25});
    keep(c, 25, manager);
    keep(c, 26, date);
    // The three months around the date's, with the year each falls in.
    const auto year_cell = c.toc(-0x4bac);
    std::array<std::uint16_t, 3> years{c.u16(year_cell), c.u16(year_cell + 2), c.u16(year_cell + 4)};
    // The original keeps these in frame slots, and later routines read leftovers there: the slots are written as it does.
    c.put32(frame.at(0x84), c.u32(year_cell));
    c.put16(frame.at(0x88), c.u16(year_cell + 4));
    invoke(c, today_of, 0x103ec124, {frame.at(0x7c)});
    std::array<std::int8_t, 3> months{};
    months[0] = std::int8_t(invoke(c, month_of, 0x103ec12c, {date}) - 1);
    c.put8(frame.at(0x38), std::uint8_t(months[0]));
    if (months[0] < 0) {
        months[0] = 11;
        c.put8(frame.at(0x38), 11);
        years[0] = std::uint16_t(years[0] - 1);
        c.put16(frame.at(0x84), years[0]);
    }
    months[1] = std::int8_t(invoke(c, month_of, 0x103ec15c, {date}));
    c.put8(frame.at(0x39), std::uint8_t(months[1]));
    months[2] = std::int8_t(invoke(c, month_of, 0x103ec168, {date}) + 1);
    c.put8(frame.at(0x3a), std::uint8_t(months[2]));
    if (months[2] > 11) {
        months[2] = 0;
        c.put8(frame.at(0x3a), 0);
        years[2] = std::uint16_t(years[2] + 1);
        c.put16(frame.at(0x88), years[2]);
    }
    struct Moment { std::int16_t hi; std::uint16_t lo; };
    invoke(c, previous_date, 0x103ec1a4, {frame.at(0x64), date, 1});
    const Moment a_moment{c.i16(frame.at(0x64)), c.u16(frame.at(0x66))};
    c.put16(frame.at(0x74), std::uint16_t(a_moment.hi));
    c.put16(frame.at(0x76), a_moment.lo);
    c.put8(frame.at(0x78), c.u8(frame.at(0x68)));
    invoke(c, next_date, 0x103ec1cc, {frame.at(0x5c), date, 1});
    const Moment b_moment{c.i16(frame.at(0x5c)), c.u16(frame.at(0x5e))};
    c.put16(frame.at(0x6c), std::uint16_t(b_moment.hi));
    c.put16(frame.at(0x6e), b_moment.lo);
    c.put8(frame.at(0x70), c.u8(frame.at(0x60)));
    keep(c, 31, frame.at(0x38));
    keep(c, 29, 0);
    auto cursor = c.u32(manager + rules_at);
    keep(c, 28, cursor);
    // The rule record's nation object gets the call; `mode` is what the record asks for.
    std::uint32_t made_lo = 0;
    const auto call_rule = [&](std::uint32_t record, std::uint32_t mode, std::uint32_t back) {
        const auto object = c.u32(manager + std::uint32_t(std::int32_t(c.i8(record))) * 4 + nation_objects);
        // The method reads r6 too: the original holds the low halfword of the date it made there.
        c.set({{3, object}, {4, record}, {5, mode}, {6, made_lo}});
        c.set(12, c.u32(c.u32(object) + method_dated));
        c.call(call_vector, back);
        c.set(2, c.u32(frame.at(0x14)));
    };
    for (std::uint32_t index = 0; std::int32_t(index) < c.i32(manager + rules_count); ++index, cursor += 4) {
        keep(c, 29, index);
        keep(c, 28, cursor);
        for (std::int32_t k = 0; k < 3; ++k) {
            keep(c, 27, std::uint32_t(k));
            keep(c, 30, std::uint32_t(k) * 2);
            auto record = c.u32(cursor);
            if (std::int32_t(c.i8(record + 4)) != std::int32_t(months[std::size_t(k)])) continue;
            invoke(c, make_date, 0x103ec24c,
                   {frame.at(0x54), std::uint32_t(std::int32_t(c.i8(record + 3))), std::uint32_t(std::int32_t(c.i8(record + 4))),
                    std::uint32_t(std::uint16_t(years[std::size_t(k)] + c.u16(date + 2))),
                    std::uint32_t(std::int32_t(c.i8(record + 2)))});
            const Moment made{c.i16(frame.at(0x54)), c.u16(frame.at(0x56))};
            made_lo = made.lo;
            c.put16(frame.at(0x7e), made.lo);
            c.put16(frame.at(0x7c), std::uint16_t(made.hi));
            c.put8(frame.at(0x80), c.u8(frame.at(0x58)));
            c.put32(frame.at(0x4c), c.u32(date));
            c.put32(frame.at(0x50), c.u32(date + 4));
            const auto same_as_made = [&](std::uint16_t lo, std::int16_t hi) { return lo == made.lo && hi == made.hi; };
            if (same_as_made(c.u16(date + 2), c.i16(date))) {
                record = c.u32(cursor);
                call_rule(record, c.i8(record + 5) != 0 ? 3 : 1, 0x103ec2e0);
                break;
            }
            // The second and third tests copy a word of the frame that the original wrote only in part (0x103ec2e8
            // reads sp+112, 0x103ec38c sp+120): the bytes past the written ones are stale, and the copy goes on to the slots.
            c.put32(frame.at(0x44), c.u32(frame.at(0x6c)));
            c.put32(frame.at(0x48), c.u32(frame.at(0x70)));
            record = c.u32(cursor);
            if (same_as_made(b_moment.lo, b_moment.hi) && c.i8(record + 5) == 0) {
                call_rule(record, 0, 0x103ec358);
                const auto object = c.u32(manager + std::uint32_t(std::int32_t(c.i8(c.u32(cursor)))) * 4 + nation_objects);
                if (const auto nation = c.u32(object + 0xc)) invoke_native(c, nation_round, 0x103ec388, {manager, nation});
                break;
            }
            c.put32(frame.at(0x3c), c.u32(frame.at(0x74)));
            c.put32(frame.at(0x40), c.u32(frame.at(0x78)));
            record = c.u32(cursor);
            if (same_as_made(a_moment.lo, a_moment.hi) && c.i8(record + 5) == 0) {
                call_rule(record, 2, 0x103ec3fc);
                break;
            }
        }
    }
}
}
