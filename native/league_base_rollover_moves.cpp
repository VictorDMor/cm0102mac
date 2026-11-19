// LEAGUE.CPP, block `rollover`: moving clubs between divisions (league_base_rollover.hpp). The original keeps
// its loop state in non-volatile registers that the callees' prologues save into the stack image, so where a
// register survives to a call (or to the routine's return, for r4-r10) it is set where the original defines it.
#include "league_base_detail.hpp"
#include "league_base_rollover.hpp"

namespace cm::aot::league_base {
namespace {
// TOC cells of this block (comp_detail.hpp's toc:: names the shared ones).
namespace cell {
inline constexpr std::int32_t
    club_moves = -0x7a98,      // cell: the object 0x104ae570 asks whether a club's move is a promotion (inferred)
    database = -0x7b7c,        // cell: the database object 0x1007fd00/0x1007fe90/0x1007ffb0 act on (inferred)
    transfers = -0x79f0,       // cell: the transfer-news object 0x101aaa90/0x101aa680 act on (inferred)
    prizes = -0x7ab8;          // cell: the object 0x10103510 looks a club's prize account up in (inferred)
}
// The club row's fields this block uses.
namespace club_field {
inline constexpr std::uint32_t division = 0x30,       // the competition record the club plays in
                               previous = 0x34,       // the division before the last move
                               transfer_state = 0x90,
                               nation_flag = 0x3c,    // word: nonzero for a club with a nation row (inferred)
                               status_byte = 0x23d;   // 3 becomes 2 when the club moves into a real competition
}
// The league's rows (Standing, +0x44 bytes) and fields this block uses.
namespace row {
inline constexpr std::uint32_t outcome = 0x40;        // signed byte: the club's result in the season (-1 undecided)
inline constexpr std::int32_t undecided = -1, relegated = 0, promoted = 3, relegated_auto = 5;
}
}

void move_club(Cpu& c, std::uint32_t club, std::uint32_t division, std::uint32_t flag) {
    const Frame frame(c, 0x1049f780, 0x50, {.first_gpr = 26});
    const auto moves = c.toc(cell::club_moves);     // r27
    const auto tables = c.toc(toc::competitions);  // r28: the address of the competition table's pointer
    c.set({{26, flag}, {27, moves}, {28, tables}, {30, club}, {31, division}});
    const auto first = invoke(c, 0x104ae570, 0x1049f7b0, {c.u32(moves), club, 0});
    const auto previous = c.u32(club + club_field::division);
    c.put32(club + club_field::previous, previous);
    c.put32(club + club_field::division, division);
    c.set({{4, previous}, {29, 0}});
    std::uint32_t gained = 0;
    if (sext8(first) == 0) {
        const auto second = invoke(c, 0x104ae570, 0x1049f7d8, {c.u32(moves), club, 0});
        if (sext8(second) != 0) { gained = 1; c.set(29, 1); }
    }
    invoke(c, 0x1007ffb0, 0x1049f7f8, {c.u32(c.toc(cell::database)), club, sext8(gained)});
    if (sext8(flag) != 0 && c.u32(club + club_field::transfer_state) == 0)
        invoke(c, 0x101aaa90, 0x1049f820, {c.u32(c.toc(cell::transfers)), club, 0});
    if (division && c.u32(tables)) {
        const auto id = c.u32(division);
        const auto object = c.u32(c.u32(tables) + (id << 2));
        c.set({{29, id}, {27, object}});
        if (object && c.u32(club + club_field::nation_flag) && c.u8(object + competition::kind) == league::kind) {
            const auto first_half = sext16(c.u16(object + league::half_e2));
            c.set(26, first_half);
            if (std::int32_t(first_half) != -1) {
                const auto prize = invoke(c, 0x10103510, 0x1049f87c, {c.u32(c.toc(cell::prizes)), club});
                invoke(c, 0x100f7cf0, 0x1049f890, {prize, sext16(c.u16(object + league::half_e4)), first_half, id, 0});
            }
        }
    }
    if (division && c.u32(tables) && c.u32(c.u32(tables) + (c.u32(division) << 2)) &&
        c.u8(club + club_field::status_byte) == 3)
        c.put8(club + club_field::status_byte, 2);
}

void set_division(Cpu& c, std::uint32_t club, std::uint32_t division) {
    const Frame frame(c, 0x1049f8e0, 0x40, {.first_gpr = 30});
    c.set({{30, club}, {31, division}});
    c.put32(club + club_field::previous, c.u32(club + club_field::division));
    c.put32(club + club_field::division, division);
    invoke(c, 0x1007fe90, 0x1049f918, {c.u32(c.toc(cell::database)), club, c.u32(club + club_field::previous)});
    const auto tables = c.u32(c.toc(toc::competitions));
    if (!tables || !division || !c.u32(tables + (c.u32(division) << 2))) {
        if (sext8(invoke(c, 0x1014c2e0, 0x1049f95c, {club, 1, 1, 0, 0})) != 0)
            invoke(c, 0x101aa680, 0x1049f980, {c.u32(c.toc(cell::transfers)), club, 1, 7, 0});
    }
}

void chain_next(Cpu& c, std::uint32_t league, std::uint32_t argument) {
    const Frame frame(c, 0x104a9160, 0x50, {.first_gpr = 27});
    const auto tables = c.toc(toc::competitions);  // r31
    c.set({{27, league}, {28, argument}, {31, tables}});
    if (c.i32(league + competition::parent) >= 0) return;
    std::uint32_t previous = league;  // r30
    c.set(30, previous);
    virtual_call(c, 0xac, 0x104a9198, {league});
    for (auto next = c.i32(league + competition::next); next >= 0;) {
        const auto object = c.u32(c.u32(tables) + (std::uint32_t(next) << 2));  // r29
        c.set(29, object);
        virtual_call(c, 0xac, 0x104a91c0, {object});
        c.set({{3, league}, {4, previous}, {5, object}, {6, 1}, {7, argument}, {8, 0xffffffffu}, {9, 0xffffffffu}});
        c.link(0x104a91e4);
        exchange_clubs(c, league, previous, object, 1, argument, 0xffffffffu, 0xffffffffu);
        next = c.i32(object + competition::next);
        previous = object;
        c.set(30, previous);
    }
}

void exchange_clubs(Cpu& c, std::uint32_t league, std::uint32_t from, std::uint32_t to, std::uint32_t announce,
                    std::uint32_t pairwise, std::uint32_t expect_relegated, std::uint32_t expect_promoted) {
    const Frame frame(c, 0x104a9210, 0xe0, {.first_gpr = 13});
    const auto news = c.toc(cell::transfers);       // r24
    const auto database = c.toc(cell::database);    // r25
    const auto tables = c.toc(toc::competitions);   // r26
    c.set({{24, news}, {29, league}, {25, database}, {30, from}, {26, tables}, {31, to}, {13, c.toc(toc::success)},
           {20, pairwise}, {27, c.toc(toc::message)}, {14, expect_relegated}, {28, c.toc(toc::pool)},
           {15, expect_promoted}, {17, 0}, {16, 0}});
    std::uint32_t relegated = 0, promoted = 0;  // r16, r17
    const auto rows = [&](std::uint32_t division) { return c.u32(division + league::standings); };
    const auto count = [&](std::uint32_t division) { return std::int32_t(c.i16(division + competition::club_count)); };
    // A club's new division as the original inlines it (the body of 0x1049f8e0): the previous division kept, the
    // database told, and the news when the division's competition does not exist. `moved` and `division` live in
    // the registers given.
    struct Back { std::uint32_t database, check, news; };
    const auto settle = [&](std::uint32_t moved, std::uint32_t division, unsigned moved_register, unsigned division_register,
                            const Back& back) {
        c.set({{moved_register, moved}, {division_register, division}});
        c.put32(moved + club_field::previous, c.u32(moved + club_field::division));
        c.put32(moved + club_field::division, division);
        invoke(c, 0x1007fe90, back.database, {c.u32(database), moved, c.u32(moved + club_field::previous)});
        const auto table = c.u32(tables);
        if (!table || !division || !c.u32(table + (c.u32(division) << 2))) {
            if (sext8(invoke(c, 0x1014c2e0, back.check, {moved, 1, 1, 0, 0})) != 0)
                invoke(c, 0x101aa680, back.news, {c.u32(news), moved, 1, 7, 0});
        }
    };
    // The clubs of `to` go down into `from`'s division: the call of 0x1049f780 the original makes in place.
    const auto demote = [&](std::uint32_t moved, std::uint32_t back) {
        c.set({{3, league}, {4, moved}, {5, c.u32(from + competition::record)}, {6, 0}});
        c.link(back);
        move_club(c, moved, c.u32(from + competition::record), 0);
    };
    if (sext8(announce) != 0) {
        c.set({{19, 0}, {18, 0}});
        for (std::int32_t index = 0, offset = 0; index < count(to); ++index, offset += 0x44) {
            const auto row = rows(to) + std::uint32_t(offset);
            const auto outcome = c.u8(row + row::outcome);
            c.set({{4, row}, {3, outcome}});
            if (outcome != row::relegated_auto && sext8(outcome) != row::relegated)
                invoke(c, 0x1007fd00, 0x104a929c, {c.u32(database), c.u32(row), c.u32(to + competition::record)});
            c.set({{18, std::uint32_t(offset) + 0x44}, {19, std::uint32_t(index) + 1}});
        }
    }
    if (sext8(pairwise) != 0) {
        // Each promoted club of `from` is paired with the first club of `to` that goes down.
        c.set({{18, 0}, {19, 0}});
        for (std::int32_t index = 0, offset = 0; index < count(from); ++index, offset += 0x44) {
            const auto from_row = rows(from) + std::uint32_t(offset);
            c.set(21, from_row);
            if (c.u8(from_row + row::outcome) == row::promoted) {
                c.set({{5, 0}, {3, 0}});
                for (std::int32_t inner = 0, inner_offset = 0; inner < count(to); ++inner, inner_offset += 0x44) {
                    const auto to_rows = rows(to);
                    const auto outcome = c.u8(to_rows + std::uint32_t(inner_offset) + row::outcome);
                    c.set({{7, to_rows}, {4, outcome}});
                    if (sext8(outcome) != row::relegated && sext8(outcome) != row::relegated_auto) {
                        c.set({{3, std::uint32_t(inner_offset) + 0x44}, {5, std::uint32_t(inner) + 1}});
                        continue;
                    }
                    const auto to_row = to_rows + std::uint32_t(inner) * 0x44;
                    c.set(20, to_row);
                    demote(c.u32(to_row), 0x104a9320);
                    settle(c.u32(from_row), c.u32(to + competition::record), 23, 22, {0x104a9344, 0x104a9384, 0x104a93a4});
                    ++promoted;
                    c.set(17, promoted);
                    c.put8(to_row + row::outcome, std::uint32_t(row::undecided));
                    ++relegated;
                    c.set(16, relegated);
                    c.put8(from_row + row::outcome, std::uint32_t(row::undecided));
                    break;
                }
            }
            c.set({{19, std::uint32_t(offset) + 0x44}, {18, std::uint32_t(index) + 1}});
        }
    } else {
        c.set({{22, 0}, {21, 0}});
        for (std::int32_t index = 0, offset = 0; index < count(from); ++index, offset += 0x44) {
            const auto row = rows(from) + std::uint32_t(offset);
            c.set({{20, row}});
            if (c.u8(row + row::outcome) == row::promoted) {
                settle(c.u32(row), c.u32(to + competition::record), 18, 19, {0x104a9430, 0x104a9470, 0x104a9490});
                ++promoted;
                c.set(17, promoted);
                c.put8(row + row::outcome, std::uint32_t(row::undecided));
            }
            c.set({{21, std::uint32_t(offset) + 0x44}, {22, std::uint32_t(index) + 1}});
        }
        c.set({{18, 0}, {19, 0xffffffffu}, {20, 0}});
        for (std::int32_t index = 0, offset = 0; index < count(to); ++index, offset += 0x44) {
            const auto row = rows(to) + std::uint32_t(offset);
            const auto outcome = c.u8(row + row::outcome);
            c.set({{21, row}, {3, outcome}});
            if (sext8(outcome) == row::relegated || outcome == row::relegated_auto) {
                demote(c.u32(row), 0x104a94f8);
                c.put8(row + row::outcome, std::uint32_t(row::undecided));
                ++relegated;
                c.set(16, relegated);
            }
            c.set({{20, std::uint32_t(offset) + 0x44}, {18, std::uint32_t(index) + 1}});
        }
    }
    if (std::int8_t(expect_relegated) != -1 && std::int8_t(relegated) != std::int8_t(expect_relegated))
        report(c, frame, {0x104a9540, 0x68, 0x40, 0x1586});
    if (std::int8_t(expect_promoted) != -1 && std::int8_t(promoted) != std::int8_t(expect_promoted))
        report(c, frame, {0x104a95a8, 0x48, 0x38, 0x158c});
}
}
