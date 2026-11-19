// LEAGUE.CPP, block `rollover`: the daily update (league_base_rollover.hpp). The frame's temporaries hold copies
// of the league's dates that readers elsewhere see (tools/stack_contract.py: GAME.CPP 0x1011d1d4 and the
// BlockMoveData bridge read what 0x104a1540 leaves), so every store the original makes into its frame is made at its
// offset; registers that survive to a call, and r4/r5 (the equality flags and date words the original leaves in
// them, read by generated callers after the return), are set where the original defines them.
#include "league_base_detail.hpp"
#include "league_base_rollover.hpp"

namespace cm::aot::league_base {
namespace {
namespace cell {
inline constexpr std::int32_t
    today = -0x7b80,            // the game's current date (a 5-byte date: year halfword, day halfword, byte)
    club_moves = -0x7a98,       // cell: the object 0x104acee0 acts on (inferred)
    news_manager = -0x7afc,     // cell: the object 0x10011750 acts on (inferred)
    season_stats = -0x7ad4,     // cell: the object 0x1000cfc0 acts on (inferred)
    descriptors = -0x7b44,      // comp_detail.hpp's toc::descriptors
    cup_a = -0x7508, cup_b = -0x7a14;  // cells holding the statistics kinds 0x1a70/0x1b0c compare (inferred)
}
namespace field {
inline constexpr std::uint32_t
    last_date = 0xd4,           // the date the league was last updated (word pair), written at the end
    start_date = 0xcc,          // date_cc: a date the league runs its season setup on (inferred)
    finished_flag = 0xe0,       // byte: the league's end has been processed
    prize_done = 0xe6,          // byte (inferred)
    notified = 0x49,            // byte: the stages' end has been announced
    flags = 0x4c,               // byte: bit 1 asks for the season-start news
    stopped = 0xb4,
    round_matches = 0xac,       // word cleared before the end-of-season virtual
    played = 0x14;              // a row's signed byte: matches played
}
// The original's compare of a league's date with another: the word pair at `field` is copied into the frame at
// `copy` and the halfwords compared (the year halfword at +0 signed, the day halfword at +2 unsigned). r4 holds the
// equality flag and r5 the second word, as the original leaves them.
bool same_day(Cpu& c, const Frame& frame, std::uint32_t league, std::uint32_t field_offset, std::int32_t copy, std::uint32_t other) {
    const auto first = c.u32(league + field_offset), second = c.u32(league + field_offset + 4);
    c.set(4, 0);
    c.put32(frame.at(copy), first);
    c.set(5, second);
    c.put32(frame.at(copy + 4), second);
    const bool equal = c.u16(frame.at(copy + 2)) == c.u16(other + 2) && c.i16(frame.at(copy)) == c.i16(other);
    if (equal) c.set(4, 1);
    return equal;
}
// The tail's compares of a shifted date with the temporary copy of the schedule's: the day halfword, then the year.
void compare_shifted(Cpu& c, const Frame& frame, std::int32_t copy, std::int32_t shifted, bool& equal) {
    c.set(4, 0);
    equal = c.u16(frame.at(copy + 2)) == c.u16(frame.at(shifted + 2)) && c.i16(frame.at(copy)) == c.i16(frame.at(shifted));
    if (equal) c.set(4, 1);
}
}

std::uint32_t daily_update(Cpu& c, std::uint32_t league, std::uint32_t date, std::uint32_t active) {
    const Frame frame(c, 0x104a1540, 0x100, {.first_gpr = 22});
    const auto news_manager = c.toc(cell::news_manager);  // r27
    const auto today = c.toc(cell::today);                // r28
    const auto club_moves = c.toc(cell::club_moves);      // r29
    c.set({{24, league}, {25, date}, {26, active}, {27, news_manager}, {28, today}, {29, club_moves},
           {30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    c.put16(frame.at(0xa8), c.u16(date));
    c.put16(frame.at(0xaa), c.u16(date + 2));
    c.put8(frame.at(0xac), c.u8(date + 4));
    if (sext8(c.u8(league + field::stopped)) != 0 || sext8(c.u8(league + competition::byte_45)) == 0) return 0;
    const bool is_active = std::int8_t(active) != 0;
    const bool own = std::int8_t(c.u8(league + competition::owner)) == -1;
    if (c.i16(league + league::half_a4) == -1) {
        const std::int32_t clubs = c.i16(league + competition::club_count);
        c.set(4, std::uint32_t(clubs));
        if (clubs > 0)
            c.put16(league + league::half_a4, std::uint32_t(clubs - 1) * std::uint32_t(c.i16(league + competition::rounds)));
    }
    // The first day of the competition's season: the draw's news, once, on the day after the descriptor's day.
    if (const auto rows = c.u32(league + league::standings);
        rows && std::int8_t(c.u8(rows + field::played)) == 0 && own && is_active) {
        if (!same_day(c, frame, league, field::last_date, 0x68, date) && c.u32(c.u32(league + competition::record) + 8) != 0) {
            const auto code = std::int8_t(invoke(c, 0x10192130, 0x104a165c, {c.u32(league + competition::record)}));
            auto day = (std::uint32_t(c.u16(c.toc(cell::descriptors) + std::uint32_t(std::int32_t(code) * 0x4c) + 0x16)) + 1) & 0xffff;
            if (day > 0x16c) day = 0;
            if (std::int32_t(day) == c.i16(today) && c.i16(league + competition::year) == std::int32_t(c.u16(today + 2))) {
                c.set({{3, league}, {4, 0}});
                c.link(0x104a16b0);
                announce_pairings(c, league, 0);
            }
        }
    }
    if (own) {
        c.set({{22, 0}, {23, 0}});
        std::uint32_t offset = 0;
        for (std::int32_t index = 0; index <= c.i32(league + competition::last_stage); ++index, offset += 4) {
            const auto stage = c.u32(c.u32(league + competition::stages) + offset);
            c.set({{4, date}, {5, active}});
            virtual_call(c, 0x1c, 0x104a16e8, {stage, date, active});
            c.set({{23, offset + 4}, {22, std::uint32_t(index) + 1}});
        }
    }
    if (same_day(c, frame, league, field::start_date, 0x60, today) && std::int8_t(c.u8(league + field::finished_flag)) == 0) {
        c.put32(league + field::round_matches, 0);
        virtual_call(c, 0x9c, 0x104a1764, {league});
    }
    if (own && c.u32(league + league::standings) != 0) {
        if (!same_day(c, frame, league, field::last_date, 0x58, date)) {
            std::uint32_t all_done = 1;
            c.set(28, all_done);
            virtual_call(c, 0x18, 0x104a17e0, {league, 0, 0});
            if (c.u8(league + field::finished_flag) == 1) {
                c.set({{22, 0}, {23, 0}});
                std::uint32_t stage_offset = 0;
                for (std::int32_t index = 0; index <= c.i32(league + competition::last_stage); ++index, stage_offset += 4) {
                    const auto stage = c.u32(c.u32(league + competition::stages) + stage_offset);
                    if (std::int8_t(virtual_call(c, 0x18, 0x104a1818, {stage, 0, 1})) == 0) {
                        all_done = 0;
                        c.set(28, all_done);
                        break;
                    }
                    c.set({{23, stage_offset + 4}, {22, std::uint32_t(index) + 1}});
                }
                const auto last_stage = c.i32(league + competition::last_stage);
                if (all_done && last_stage < c.i32(league + competition::word_2c) - 1) {
                    c.set(4, std::uint32_t(last_stage));
                    virtual_call(c, 0x30, 0x104a186c, {league});
                } else if (all_done && std::int8_t(c.u8(league + field::notified)) == 0) {
                    invoke(c, 0x1000cfc0, 0x104a1898, {c.u32(c.toc(cell::season_stats)), league});
                    if (std::int8_t(invoke(c, 0x104646d0, 0x104a18a4, {c.u32(c.u32(league + competition::record))})) != 0)
                        invoke(c, 0x10011750, 0x104a18bc, {c.u32(news_manager), league, 1});
                    if (sext8(c.u8(league + field::prize_done)) != 0)
                        report(c, frame, {0x104a18dc, 0xb0, 0xa0, 0x74c});
                    virtual_call(c, 0x38, 0x104a192c, {league});
                    if (c.u32(league + competition::stats)) invoke(c, 0x10456290, 0x104a1940, {c.u32(league + competition::stats)});
                    c.put8(league + field::notified, 1);
                }
            }
            if (std::int8_t(invoke(c, 0x104646d0, 0x104a1954, {c.u32(c.u32(league + competition::record))})) != 0)
                invoke(c, 0x10011750, 0x104a196c, {c.u32(news_manager), league, 0});
            if (c.u32(league + competition::stats)) invoke(c, 0x104573c0, 0x104a197c, {c.u32(league + competition::stats)});
        }
    }
    if (c.u32(league + league::standings) != 0 && is_active && std::int8_t(c.u8(league + field::finished_flag)) == 0) {
        invoke(c, 0x10717a10, 0x104a19ac, {frame.at(0x88), frame.at(0xa8), 4});
        const std::uint32_t year = c.u16(frame.at(0x88)), day = c.u16(frame.at(0x8a)), extra = c.u8(frame.at(0x8c));
        c.put16(frame.at(0x98), year);
        c.put16(frame.at(0x9a), day);
        c.put8(frame.at(0x9c), extra);
        c.set({{5, day}, {6, sext16(year)}});
        invoke(c, 0x104a28d0, 0x104a19d0, {league, frame.at(0x98)});
    }
    if (is_active && (c.u8(league + field::flags) & 2) != 0 && own) {
        if (!same_day(c, frame, league, field::last_date, 0x50, date) && std::int8_t(c.u8(league + field::finished_flag)) == 0 &&
            std::int8_t(c.u8(c.u32(league + league::standings) + field::played)) == 0) {
            const auto schedule = c.u32(league + league::schedule);
            invoke(c, 0x107156b0, 0x104a1a70, {frame.at(0x90), sext16(c.u16(schedule)),
                                              (std::uint32_t(c.u16(schedule + 2)) + sext16(c.u16(league + competition::year))) & 0xffff});
            const auto record = c.u32(league + competition::record);
            c.set(5, record);
            const auto nation = record ? c.u32(record + 8) : 0;
            // Whether the competition's record names the statistics kind held in the TOC cell `slot`.
            const auto names_kind = [&](std::int32_t slot) {
                if (!record) return false;
                c.set(4, nation);
                if (!nation) return false;
                c.set(4, c.u32(nation + 0x68));
                return c.u32(nation + 0x68) == c.u32(c.toc(slot));
            };
            const auto copy = [&](std::int32_t at) {
                c.put32(frame.at(at), c.u32(frame.at(0x90)));
                c.put32(frame.at(at + 4), c.u32(frame.at(0x94)));
            };
            bool equal = false;
            if (names_kind(cell::cup_a)) {
                copy(0x48);
                invoke(c, 0x10717890, 0x104a1abc, {frame.at(0x80), frame.at(0xa8), 5});
                compare_shifted(c, frame, 0x48, 0x80, equal);
                if (equal) invoke(c, 0x104acee0, 0x104a1b00, {c.u32(club_moves), league, 0, 0});
            } else if (names_kind(cell::cup_b)) {
                copy(0x40);
                invoke(c, 0x10717890, 0x104a1b4c, {frame.at(0x78), frame.at(0xa8), 0x14});
                compare_shifted(c, frame, 0x40, 0x78, equal);
                if (equal) invoke(c, 0x104acee0, 0x104a1b90, {c.u32(club_moves), league, 0, 0});
            } else {
                copy(0x38);
                invoke(c, 0x10717890, 0x104a1bb4, {frame.at(0x70), frame.at(0xa8), 0xe});
                compare_shifted(c, frame, 0x38, 0x70, equal);
                if (equal) invoke(c, 0x104acee0, 0x104a1bf8, {c.u32(club_moves), league, 0, 0});
            }
        }
    }
    if (is_active) {
        c.put16(league + 0xd6, c.u16(date + 2));
        c.put16(league + 0xd4, c.u16(date));
        c.put8(league + 0xd8, c.u8(date + 4));
    }
    return 1;
}
}
