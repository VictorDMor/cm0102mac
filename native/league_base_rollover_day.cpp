// LEAGUE.CPP, block `rollover`: the league's day. The prize money, the question whether the league is finished,
// the news of a draw (league_base_rollover.hpp), and the daily update (league_base_rollover_update.cpp's sibling).
// Registers that survive to a call (non-volatile) or to a return (r4-r10, read by generated callers) are set where
// the original defines them: callees' prologues save them into the stack image and the call trace compares them.
#include "league_base_detail.hpp"
#include "league_base_rollover.hpp"

namespace cm::aot::league_base {
namespace {
namespace cell {
inline constexpr std::int32_t
    prizes = -0x7ab8,           // cell: the object 0x10103510 looks a club's prize account up in (inferred)
    news_manager = -0x7bf0,     // cell: the news manager (comp_detail.hpp's toc::news)
    draw_lookup = -0x7a60,      // cell: the object 0x1048b4e0 searches draws in (inferred)
    season_news = -0x7b48;      // cell: the object 0x10382d70 files the season's news in (inferred)
}
namespace field {
inline constexpr std::uint32_t
    prize_pool = 0x28,          // word: the money shared out by place (0x1049f320; inferred)
    kind_e1 = 0xe1,             // byte: 2 none, 3 asks the competition, else the shares are paid here (inferred)
    standings_played = 0x14,    // a row's signed byte: matches played (inferred)
    rounds_played = 0xa6,       // halfword (inferred)
    finished_flag = 0xe0,       // byte: set once the league's end has been processed (inferred)
    prize_done = 0xe6,          // byte: the prizes have been handed out (inferred)
    stopped = 0xb4;             // byte: nonzero for a league that has stopped (inferred)
}
}

void prize_money(Cpu& c, std::uint32_t league) {
    const Frame frame(c, 0x1049f320, 0x80, {.first_gpr = 25});
    c.set({{31, c.toc(cell::prizes)}, {30, league}, {28, c.toc(toc::message)}, {29, c.toc(toc::pool)}});
    const auto kind = sext8(c.u8(league + field::kind_e1));
    if (kind == 2) return;
    if (kind == 3) {
        const auto tables = c.u32(c.toc(toc::competitions));
        if (!tables) {
            report(c, frame, {0x1049f37c, 0x40, 0x38, 0x1df});
            return;
        }
        const auto object = c.u32(tables + (c.u32(c.u32(league + competition::record)) << 2));
        virtual_call(c, 0x74, 0x1049f3e0, {object, league, tables});
        return;
    }
    // The shares: the pool divided by 1 + 2 + .. + n (halfword sums), club k from the top gets (n - k) shares.
    const std::int32_t clubs = c.i16(league + competition::club_count);
    std::int32_t sum = 0;
    if (clubs > 0) sum = clubs * (clubs + 1) / 2;
    const std::int32_t divisor = std::int16_t(sum);
    const auto pool = c.i32(league + field::prize_pool);
    c.set(27, 0);
    // divw by zero stops the original (a league without clubs and a kind other than 2 and 3 reaches it): the same stop.
    if (divisor == 0) throw std::runtime_error("Signed division by zero in translated code");
    const std::uint32_t share = std::uint32_t(std::int64_t(pool) / divisor);
    c.set({{26, share}, {28, 0}});
    std::uint32_t offset = 0;
    for (std::uint32_t index = 0; std::int16_t(index) < c.i16(league + competition::club_count); ++index, offset += 0x44) {
        const auto counter = sext16(index);
        const auto amount = share * (std::uint32_t(c.i16(league + competition::club_count)) - counter);
        c.set({{29, counter}, {25, amount}});
        const auto club = c.u32(c.u32(league + league::standings) + offset);
        const auto account = invoke(c, 0x10103510, 0x1049f4d4, {c.u32(c.toc(cell::prizes)), club});
        invoke(c, 0x100f71f0, 0x1049f4dc, {account, amount});
        if (std::int32_t(amount) > 1000)
            invoke(c, 0x10452010, 0x1049f510,
                   {league, club, std::uint32_t(std::int32_t(amount) >> 31), amount, sext8(counter + 1), 0, 0, 0xfffffffeu});
        c.set({{28, offset + 0x44}, {27, index + 1}});
    }
}

std::uint32_t finished(Cpu& c, std::uint32_t league, std::uint32_t mode, std::uint32_t finish) {
    const Frame frame(c, 0x104a8f40, 0x50, {.first_gpr = 28});
    c.set({{29, league}, {30, mode}, {31, finish}});
    if (c.u32(league + league::standings) == 0) return 0;
    if (sext8(c.u8(league + field::stopped)) != 0 || sext8(c.u8(league + competition::byte_45)) == 0) return 0;
    if (std::int8_t(c.u8(league + competition::owner)) != -1 && std::int8_t(c.u8(league + field::finished_flag)) != 0) return 1;
    if (std::int8_t(c.u8(league + field::finished_flag)) == 0) {
        const std::int32_t clubs = c.i16(league + competition::club_count);
        if (c.i16(league + league::half_a4) == -1 && clubs > 0)
            c.put16(league + league::half_a4, std::uint32_t(clubs - 1) * std::uint32_t(c.i16(league + competition::rounds)));
        std::uint32_t offset = 0;
        // The loop's registers stay behind for the calls that follow: r4 the rows, r5 the offset, r6 the count.
        c.set({{6, 0}, {5, 0}});
        for (std::int32_t index = 0; index < c.i16(league + competition::club_count); ++index, offset += 0x44) {
            c.set(4, c.u32(league + league::standings));
            if (std::int8_t(c.u8(c.u32(league + league::standings) + offset + field::standings_played)) < c.i16(league + league::half_a4))
                return 0;
            c.set({{5, offset + 0x44}, {6, std::uint32_t(index) + 1}});
        }
        const auto national = invoke(c, 0x104646d0, 0x104a9048, {c.u32(c.u32(league + competition::record))});
        if (std::int8_t(national) != 0 && c.i16(league + field::rounds_played) > 8 && c.u8(league + field::prize_done) == 0)
            invoke(c, 0x10382d70, 0x104a9078, {c.u32(c.toc(cell::season_news)), league});
        if (std::int8_t(c.u8(league + field::prize_done)) == 0) {
            c.set(3, league);
            c.link(0x104a908c);
            prize_money(c, league);
        }
    }
    if (std::int32_t(mode) == 1 && std::int8_t(c.u8(league + competition::owner)) == -1) {
        std::uint32_t offset = 0;
        std::int32_t index = 0;
        c.set({{28, 0}, {30, 0}});
        for (; index <= c.i32(league + competition::last_stage); ++index, offset += 4) {
            c.set({{28, std::uint32_t(index)}, {30, offset}});
            const auto stage = c.u32(c.u32(league + competition::stages) + offset);
            if (std::int8_t(virtual_call(c, 0x18, 0x104a90cc, {stage, 0, 1})) == 0) return 0;
            c.set({{30, offset + 4}, {28, std::uint32_t(index) + 1}});
        }
        const auto last = c.i32(league + competition::last_stage);
        c.set(4, std::uint32_t(last));
        if (last < c.i32(league + competition::word_2c) - 1) return 0;
    }
    if (std::int8_t(c.u8(league + field::finished_flag)) == 0) {
        if (std::int8_t(finish) != 0) virtual_call(c, 0x9c, 0x104a9130, {league});
        c.put8(league + field::finished_flag, 1);
    }
    return 1;
}

void announce_pairings(Cpu& c, std::uint32_t league, std::uint32_t seen) {
    const Frame frame(c, 0x104a0ab0, 0x1b0, {.first_gpr = 24});
    const auto manager_cell = c.toc(cell::news_manager);  // r30
    const auto lookup_cell = c.toc(cell::draw_lookup);    // r31
    // Frame locals: the two lists (0x90 the default `seen`, 0x80 the draw's), the news item, the dates.
    const auto own_list = frame.at(0x90), draw_list = frame.at(0x80), item = frame.at(0xa0);
    const auto date = frame.at(0x78), shifted = frame.at(0x70);
    c.set({{26, league}, {27, seen}, {30, manager_cell}, {31, lookup_cell}});
    invoke(c, 0x10738c00, 0x104a0ad8, {own_list});
    if (std::int8_t(c.u8(league + competition::owner)) == -1 || seen == 0) {
        seen = own_list;
        c.set(27, seen);
    }
    const auto national = invoke(c, 0x104646d0, 0x104a0b00, {c.u32(c.u32(league + competition::record))});
    if (std::int8_t(national) != 0 && std::int8_t(c.u8(league + competition::owner)) == -1) {
        const auto schedule = c.u32(league + league::schedule);
        invoke(c, 0x107156b0, 0x104a0b38,
               {date, sext16(c.u16(schedule)), (std::uint32_t(c.u16(schedule + 2)) + sext16(c.u16(league + competition::year))) & 0xffffu});
        invoke(c, 0x10717b20, 0x104a0b48, {shifted, date, 5});
        const auto day_byte = c.u8(frame.at(0x74));
        const auto day_year = c.u16(frame.at(0x72));
        const auto day_month = c.u16(frame.at(0x70));
        c.put16(frame.at(0x7a), day_year);
        c.put16(frame.at(0x78), day_month);
        c.put8(frame.at(0x7c), day_byte);
        c.set({{28, 0}, {29, 0}});
        std::uint32_t offset = 0;
        for (std::uint32_t index = 0; std::int16_t(index) < c.i16(league + competition::club_count); ++index, offset += 0x44) {
            const auto row_club = [&] { return c.u32(c.u32(league + league::standings) + offset); };
            const auto involved = invoke(c, 0x102cdbf0, 0x104a0b7c, {c.u32(manager_cell), row_club()});
            if (std::int8_t(involved) != 0) {
                c.put32(seen + 0xc, c.u32(seen + 4));
                c.set(25, 0);
                bool found = false;
                for (;;) {
                    const auto element = invoke(c, 0x10738e00, 0x104a0bbc, {seen});
                    if (!element) break;
                    c.set(4, c.u32(element));
                    if (c.u32(element) == c.u32(row_club())) { found = true; c.set(25, 1); break; }
                }
                if (!found) {
                    c.put16(frame.at(0x6c), c.u16(frame.at(0x78)));
                    c.put16(frame.at(0x6a), c.u16(frame.at(0x7a)));
                    c.put8(frame.at(0x68), 0);
                    invoke(c, 0x10738c00, 0x104a0bec, {draw_list});
                    const auto id = c.u32(row_club());
                    c.put32(frame.at(0x38), id);
                    for (const auto at : {0x3c, 0x40, 0x44, 0x48}) c.put32(frame.at(at), 0xffffffffu);
                    c.put32(frame.at(0x4c), 3);
                    c.put32(frame.at(0x50), draw_list);
                    c.put32(frame.at(0x54), 0);
                    c.put32(frame.at(0x58), c.u8(league + competition::owner));
                    c.put32(frame.at(0x5c), 0xffffffffu);
                    c.put32(frame.at(0x60), 1);
                    const auto drawn = invoke(c, 0x1048b4e0, 0x104a0c64,
                                              {c.u32(lookup_cell), frame.at(0x6a), frame.at(0x6c), frame.at(0x68), 1,
                                               c.u32(c.u32(league + competition::record)), 0xffffffffu, 0xffffffffu});
                    if ((drawn & 0xffff) == 3) {
                        std::uint32_t slot = 3;
                        c.set(24, slot);
                        invoke(c, 0x102bdf90, 0x104a0c84, {item, 0xd, 0});
                        invoke(c, 0x102cb4b0, 0x104a0c98, {item, 0, c.u32(c.u32(league + competition::record))});
                        invoke(c, 0x102cb4b0, 0x104a0cb0, {item, 1, c.u32(row_club())});
                        invoke(c, 0x102cb4b0, 0x104a0cc0, {item, 2, sext16(c.u16(league + competition::year))});
                        c.put32(frame.at(0x8c), c.u32(frame.at(0x84)));
                        // Each drawn match: its two halfwords, then the opponent's id and whether the club is the home side.
                        for (;;) {
                            const auto match = invoke(c, 0x10738e00, 0x104a0d6c, {draw_list});
                            c.set(25, match);
                            if (!match) break;
                            c.set(24, slot + 1);
                            invoke(c, 0x102cb4b0, 0x104a0ce4, {item, slot++, c.u16(match + 0x2a)});
                            c.set(24, slot + 1);
                            invoke(c, 0x102cb4b0, 0x104a0cf8, {item, slot++, c.u16(match + 0x28)});
                            const auto home = c.u32(match + 0x1c);
                            if (row_club() == home) {
                                const auto other = c.u32(c.u32(match + 0x20));
                                c.set(24, slot + 1);
                                invoke(c, 0x102cb4b0, 0x104a0d24, {item, slot++, other});
                                c.set(24, slot + 1);
                                invoke(c, 0x102cb4b0, 0x104a0d38, {item, slot++, 1});
                            } else {
                                c.set(24, slot + 1);
                                invoke(c, 0x102cb4b0, 0x104a0d50, {item, slot++, c.u32(home)});
                                c.set(24, slot + 1);
                                invoke(c, 0x102cb4b0, 0x104a0d64, {item, slot++, 0});
                            }
                        }
                        invoke(c, 0x102cc140, 0x104a0d88, {c.u32(manager_cell), item, row_club()});
                        invoke(c, 0x10738d10, 0x104a0d9c, {seen, row_club(), 4});
                    }
                    invoke(c, 0x10738c20, 0x104a0da8, {draw_list, 0xffffffffu});
                }
            }
            c.set({{29, offset + 0x44}, {28, index + 1}});
        }
        if (std::int8_t(c.u8(league + competition::owner)) == -1) {
            c.set({{24, 0}, {25, 0}});
            std::uint32_t stage_offset = 0;
            for (std::int32_t stage_index = 0; std::int8_t(stage_index) <= c.i32(league + competition::last_stage);
                 ++stage_index, stage_offset += 4) {
                const auto stage = c.u32(c.u32(league + competition::stages) + stage_offset);
                if (c.u8(stage + competition::kind) == league::kind) {
                    // Through the dispatcher, not a direct call: 0x104a0ab0 is a draft, so production runs the original here.
                    invoke(c, 0x104a0ab0, 0x104a0dfc, {stage, seen});
                }
                c.set({{25, stage_offset + 4}, {24, std::uint32_t(stage_index) + 1}});
            }
        }
    }
    invoke(c, 0x10738c20, 0x104a0e20, {own_list, 0xffffffffu});
}
}
