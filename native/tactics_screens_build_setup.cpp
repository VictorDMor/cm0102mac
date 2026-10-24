// The tactics screen's state object: its constructor (0x103c4b10), which
// copies the formation, squad roles and line-up the screen edits from the
// tactics records (or a preset, or a match in progress), and two helpers it
// calls (0x103cb0b0, 0x103c7690).
#include "tactics_screens_build.hpp"
#include "tactics_leaves.hpp"

namespace cm::aot::tactics_screens::build {
using guest::Cpu;
using guest::Frame;
using detail::fail;
namespace {
// TOC slots the constructor and its helpers read.
constexpr std::int32_t date_slot = -0x7b80;        // -> today's date (int16 year?, uint16 day)
constexpr std::int32_t humans_slot = -0x7a54;      // -> the human manager table
constexpr std::int32_t tactics_slot = -0x7a6c;     // -> CTACTICS (tactics.hpp)
constexpr std::int32_t success_slot = -0x7bac;     // -> the shared success flag
constexpr std::int32_t message_slot = -0x7ba8;     // -> the assert message buffer
constexpr std::int32_t presets_slot = -0x7b4c;     // -> the object 0x1029ec40 / 0x1029f600 read presets from
constexpr std::int32_t clubs_slot = -0x7be8;       // -> club records (0x248 bytes)
constexpr std::int32_t fixtures_slot = -0x7a60;    // -> the object 0x1048b180 lists a club's fixtures from
constexpr std::uint32_t club_size = 0x248, team_block_size = 0x664;

std::uint32_t sext8(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
std::uint32_t indexed(std::uint32_t value, std::uint32_t size) { return sext8(value) * size; }

// Fields of the state object only the constructor and its helpers use.
namespace at {
constexpr std::uint32_t fixture_clubs = 0x14;   // int32[2]: the preset's club ids; team blocks at +0x120
constexpr std::uint32_t team_blocks = 0x134;    // two 0x664-byte team blocks (+0x34 kind, +0x35 formation, +0x74 picks)
constexpr std::uint32_t fixture = 0xe10;        // the fixture shown (0x103c1c00 copies it)
constexpr std::uint32_t list_control = 0xe24;
constexpr std::uint32_t fixture_home = 0xe2c, fixture_away = 0xe30;
constexpr std::uint32_t list_origin = 0xe38;    // int16[2]
constexpr std::uint32_t list_style = 0xe5e;     // uint8: high nibble
constexpr std::uint32_t opponent = 0xe64;       // -> the other club's record, 0 none
constexpr std::uint32_t home_side = 0xe69;      // int8
constexpr std::uint32_t team_formation = 0xe6c; // -> the formation record to start from, 0 none
constexpr std::uint32_t opponents = 0xe70;      // the opponent's listed players (0x50 bytes)
constexpr std::uint32_t editable = 0xec0;       // int8
constexpr std::uint32_t shown_cell = 0x27a6, shown_cell_column = 0x27a8;
constexpr std::uint32_t ball_cell = 0x27aa, ball_cell_column = 0x27ac;
constexpr std::uint32_t free_kick_side = 0x27ae;
constexpr std::uint32_t substitutes = 0x4b12;   // int8
constexpr std::uint32_t pick = 0x4b14;          // the squad's pick (a staff id)
constexpr std::uint32_t captains = 0x4b18;      // -> captain, vice-captain staff records
constexpr std::uint32_t takers = 0x4b20;        // [5][2] set-piece takers
constexpr std::uint32_t lineup = 0x4b48;        // int8[11]
constexpr std::uint32_t changed = 0x4b53;       // int8
constexpr std::uint32_t edited = 0x4b54;        // the second formation record
constexpr std::uint32_t saved = 0x87e4;         // the third: the formation as it was on opening
constexpr std::uint32_t flags = 0xc6b0;         // int8[2]
}
}

std::uint32_t construct(Cpu& c, std::uint32_t screen, std::uint32_t club, std::uint32_t match, std::uint32_t opened_from,
                        std::uint32_t in_match, std::uint32_t preset, std::uint32_t national) {
    Frame frame(c, 0x103c4b10, 0x1a10, {.first_gpr = 13});
    // The arguments the original keeps in its caller's frame.
    const auto screen_slot = frame.at(0x1a28), managed_slot = frame.at(0x1a37);
    c.put32(screen_slot, screen);
    c.put32(frame.at(0x1a30), match);
    c.put8(managed_slot, opened_from);
    c.put8(frame.at(0x1a3b), in_match);
    c.put32(frame.at(0x1a3c), preset);
    c.put8(frame.at(0x1a43), national);
    const auto date = c.toc(date_slot), humans = c.toc(humans_slot), tactics = c.toc(tactics_slot);
    c.set({{19, club}, {27, date}, {26, humans}, {31, tactics}, {22, c.toc(success_slot)},
           {25, c.toc(message_slot)}, {28, c.toc(detail::pool_slot)}});

    // The members: two team blocks and three formation records.
    c.set(4, screen);
    c.link(0x103c4b60);
    construct_teams(c, screen + at::fixture_clubs);
    c.set(4, screen);
    c.link(0x103c4b6c);
    construct_formation(c, screen + field::formation);
    c.link(0x103c4b78);
    construct_formation(c, screen + at::edited);
    c.set(4, screen);
    c.link(0x103c4b88);
    const auto saved = construct_formation(c, screen + at::saved);
    // A local list of fixtures (constructed from the last record's address).
    const auto fixtures = frame.at(0x70);
    c.set({{3, fixtures}, {4, saved}});
    c.call(0x10738c00, 0x103c4b94);
    const auto leave = [&](unsigned holder, std::uint32_t back) {
        c.set({{holder, screen}, {3, fixtures}, {4, 0xffffffffu}});
        c.call(0x10738c20, back);
        return screen;
    };
    if (!club) {
        fail(c, frame, 0x103c4bb0, 0xbe, 0x6a, 0x1289);
        return leave(18, 0x103c4bfc);
    }
    c.set({{3, c.u32(humans)}, {4, c.u8(managed_slot)}, {5, club}, {6, 0}});
    c.call(0x10146830, 0x103c4c18);
    c.put32(screen + field::club, club);
    c.put32(screen + field::match, c.u32(frame.at(0x1a30)));
    c.put8(screen + field::opened_from, c.u8(managed_slot));
    c.put8(screen + field::in_match, c.u8(frame.at(0x1a3b)));
    c.put32(screen + field::preset, c.u32(frame.at(0x1a3c)));
    c.put8(screen + field::national, c.u8(frame.at(0x1a43)));
    const auto club_id = [&] { return c.u32(c.u32(screen + field::club)); };
    const auto side = [&] { return sext8(c.u8(screen + field::side)); };

    if (c.i32(screen + field::preset) > -1) {
        // A preset: its fixture's clubs and teams.
        c.set({{3, c.u32(c.toc(presets_slot))}, {4, c.u32(screen + field::preset)}, {5, screen + at::fixture_clubs}});
        c.call(0x1029ec40, 0x103c4c88);
        c.set(24, c.r(3));
        if (std::int8_t(c.r(3)) == 0) {
            fail(c, frame, 0x103c4cac, 0x9f, 0x65, 0x12a9);
            return leave(17, 0x103c4cf8);
        }
        c.set({{3, c.u32(c.toc(presets_slot))}, {4, c.u32(screen + field::preset)}, {5, screen + at::fixture},
               {6, screen}});
        c.call(0x1029f600, 0x103c4d1c);
        c.set(24, c.r(3));
        if (std::int8_t(c.r(3)) == 0) {
            fail(c, frame, 0x103c4d40, 0x80, 0x60, 0x12b1);
            return leave(16, 0x103c4d8c);
        }
        c.put8(screen + field::limit_squad, 1);
        const auto home = c.i32(screen + at::fixture_clubs) == std::int32_t(club_id());
        const auto other = c.u32(screen + at::fixture_clubs + (home ? 4 : 0));
        c.put32(screen + at::opponent, c.u32(c.toc(clubs_slot)) + other * club_size);
        c.put8(screen + field::side, club_id() == c.u32(screen + at::fixture_clubs) ? 0 : 1);
        c.put8(screen + at::home_side, club_id() == c.u32(screen + at::fixture_clubs) ? 1 : 0);
        c.put8(screen + field::team_kind, c.u8(screen + at::team_blocks + side() * team_block_size + 0x34));
        c.put32(screen + at::team_formation, 0);
    } else if (const auto playing = c.u32(screen + field::match)) {
        // A match in progress: its fixture, the opponent and both teams' players.
        c.set(3, playing);
        c.call(0x101f9410, 0x103c4ec4);
        c.set({{4, c.r(3)}, {3, screen + at::fixture}});
        c.call(0x103c1c00, 0x103c4ed4);
        c.put8(screen + field::limit_squad, 1);
        const auto home_club = [&](std::uint32_t back) {
            c.link(back);
            return match_home_club(c, c.u32(screen + field::match));
        };
        if (const auto home = home_club(0x103c4eec); c.u32(screen + field::club) == home) {
            c.link(0x103c4f08);
            c.put32(screen + at::opponent, match_away_club(c, c.u32(screen + field::match)));
        } else {
            c.put32(screen + at::opponent, home_club(0x103c4f20));
        }
        const auto home = home_club(0x103c4f34);
        c.put8(screen + field::side, c.u32(screen + field::club) == home ? 0 : 1);
        const auto home_again = home_club(0x103c4f6c);
        c.put8(screen + at::home_side, c.u32(screen + field::club) == home_again ? 1 : 0);
        c.link(0x103c4fac);
        c.put8(screen + field::team_kind, match_side_byte(c, c.u32(screen + field::match), c.u8(screen + field::side)));
        c.set({{3, c.u32(screen + field::match)}, {4, c.u8(screen + field::side)}});
        tactics_leaf_103c5940(c.s);
        c.put32(screen + at::team_formation, c.r(3));
        for (std::uint32_t entry = 0; std::int8_t(entry) < 20; ++entry) {
            c.set({{30, entry}, {3, c.u32(screen + field::match)}, {4, c.u8(screen + field::side)}, {5, entry}});
            c.call(0x101f9440, 0x103c4ff4);
            c.set({{4, c.r(3)}, {3, screen + indexed(entry, field::player_entry_size) + field::players}});
            c.call(0x103c14a0, 0x103c5010);
        }
        c.set(30, 20);
    } else {
        // The club's next fixture within 24 days (8 for a national team?), from today.
        c.put8(screen + field::limit_squad, 0);
        c.set(3, c.u32(screen + field::club));
        c.call(0x100c7d00, 0x103c503c);
        const bool first = std::int8_t(c.r(3)) != 0;
        const std::uint32_t days = first ? 0x18 : 8;
        const std::uint32_t future = c.i8(screen + field::opened_from) != 0 ? 0 : 1;
        c.set(first ? 21 : 20, future);
        const auto shifted = [&](std::int32_t offset, std::uint32_t back) {
            c.set({{3, frame.at(offset)}, {4, date}, {5, days}});
            c.call(0x10717a10, back);
            c.set(3, frame.at(offset));
        };
        const auto day = [&](std::uint32_t at) { c.set(3, at); tactics_leaf_103be9a0(c.s); return c.r(3); };
        const auto year = [&](std::uint32_t at) { c.set(3, at); tactics_leaf_103be9b0(c.s); return c.r(3); };
        std::uint32_t from_day, from_year, to_day;
        if (first) {
            shifted(0x50, 0x103c507c);
            to_day = day(frame.at(0x50));
            c.set(13, to_day);
            shifted(0x58, 0x103c509c);
            const auto to_year = year(frame.at(0x58)) & 0xffff;
            c.set(14, to_year);
            from_day = day(date);
            c.set(15, from_day);
            from_year = year(date);
            c.put32(frame.at(0x38), 0xfffffffeu);
            c.put32(frame.at(0x3c), future);
            c.set({{3, c.u32(c.toc(fixtures_slot))}, {4, from_year & 0xffff}, {5, from_day}, {6, to_year}, {7, to_day},
                   {8, club_id()}, {9, fixtures}, {10, 0xffffffffu}});
            c.call(0x1048b180, 0x103c5100);
        } else {
            shifted(0x40, 0x103c5138);
            to_day = day(frame.at(0x40));
            c.set(15, to_day);
            shifted(0x48, 0x103c5158);
            const auto to_year = year(frame.at(0x48)) & 0xffff;
            c.set(14, to_year);
            from_day = day(date);
            c.set(13, from_day);
            from_year = year(date);
            c.put32(frame.at(0x38), 0xfffffffeu);
            c.put32(frame.at(0x3c), future);
            c.set({{3, c.u32(c.toc(fixtures_slot))}, {4, from_year & 0xffff}, {5, from_day}, {6, to_year}, {7, to_day},
                   {8, club_id()}, {9, fixtures}, {10, 0xffffffffu}});
            c.call(0x1048b180, 0x103c51bc);
            c.set(4, c.r(3));
        }
        c.set(3, fixtures);
        c.call(0x101f9430, 0x103c51c8);
        if (std::int16_t(c.r(3)) != 0) {
            c.set(3, fixtures);
            c.call(0x101f9460, 0x103c51e0);
            // The first listed fixture whose byte +0x43 is -1.
            for (;;) {
                c.set(3, fixtures);
                c.call(0x10738e00, 0x103c521c);
                const auto fixture = c.r(3);
                c.set(23, fixture);
                if (!fixture) break;
                if (c.i8(fixture + 0x43) != -1) continue;
                c.set({{3, screen + at::fixture}, {4, fixture}});
                c.call(0x103c1c00, 0x103c5204);
                c.put8(screen + field::limit_squad, 1);
                break;
            }
        }
        if (c.i8(screen + field::limit_squad) != 0) {
            const auto home = c.u32(screen + at::fixture_home);
            c.put32(screen + at::opponent, c.u32(screen + field::club) == home ? c.u32(screen + at::fixture_away) : home);
        } else {
            c.put32(screen + at::opponent, 0);
        }
        c.put8(screen + field::team_kind, 1);
        c.set({{3, c.u32(screen + field::club)}, {4, 0}, {5, 1}, {6, 0}, {7, 1}});
        c.call(0x1014c2e0, 0x103c52b0);
        if (std::int8_t(c.r(3)) != 0) {
            c.set(3, c.u32(screen + field::club));
            c.call(0x100c96e0, 0x103c52c8);
            c.set({{4, c.r(3)}, {3, c.u32(humans)}});
            c.call(0x101466c0, 0x103c52d4);
            c.put32(screen + at::team_formation, c.r(3));
        } else {
            c.set({{3, c.u32(tactics)}, {4, c.u32(screen + field::club)}, {5, 0}});
            c.call(0x103af570, 0x103c52f4);  // tactics::team_formation
            c.put32(screen + at::team_formation, c.r(3));
        }
        c.link(0x103c5304);
        list_opponents(c, screen);
    }

    // What the screen edits: the formation, the cells shown, the pick,
    // set-piece takers, line-up and captains.
    const auto reset_view = [&] {
        c.put8(screen + field::view, 1);
        c.put8(screen + field::list_view, 1);
        c.put16(screen + at::shown_cell_column, 0);
        c.put16(screen + at::shown_cell, 0);
        c.put16(screen + at::ball_cell_column, 0);
        c.put16(screen + at::ball_cell, 0);
        c.put8(screen + at::free_kick_side, 0);
        c.put8(screen + at::substitutes, 0);
    };
    const auto close = [&](std::uint32_t editable, std::uint32_t back) {
        c.put8(screen + at::changed, 0);
        c.put8(screen + at::flags, 0);
        c.put8(screen + at::flags + 1, 0);
        c.put8(screen + at::editable, editable);
        c.set({{3, screen + at::saved}, {4, screen + field::formation}});
        c.call(0x103c1340, back);
    };
    const auto managed = [&] { return c.u8(screen + field::opened_from); };
    if (c.i32(screen + field::preset) > -1) {
        // The preset's formation for this side; no roles.
        const auto copy = frame.at(0xe0);
        c.set({{3, copy}, {4, screen + at::fixture_clubs}, {5, c.u8(screen + field::side)}});
        c.call(0x10110f40, 0x103c532c);
        c.set({{3, screen + field::formation}, {4, copy}});
        c.call(0x103c1980, 0x103c533c);
        c.set({{3, copy}, {4, 0xffffffffu}});
        c.call(0x10002070, 0x103c5348);
        reset_view();
        const auto block = screen + at::team_blocks + side() * team_block_size;
        const auto chosen = sext8(c.u8(screen + at::team_blocks + side() * team_block_size + 0x35));
        c.put32(screen + at::pick, c.u32(block + chosen * 0x4c + 0x74));
        for (std::uint32_t kind = 0; kind < 5; ++kind)
            for (std::uint32_t order = 0; order < 2; ++order) c.put32(screen + at::takers + kind * 8 + order * 4, 0);
        for (std::uint32_t place = 0; place < 11; ++place) c.put8(screen + at::lineup + place, 0xff);
        c.set({{29, 2}, {30, 11}});
        c.put32(screen + at::captains + 4, 0);
        c.put32(screen + at::captains, 0);
        close(1, 0x103c54d0);
    } else {
        c.set({{3, c.u32(screen + field::club)}, {4, 0}, {5, 1}});
        c.call(0x1014c490, 0x103c54e8);
        const bool human = std::int8_t(c.r(3)) != 0;
        if (const auto start = c.u32(screen + at::team_formation)) {
            c.set({{3, screen + field::formation}, {4, start}});
            c.call(0x103c1980, human ? 0x103c5518 : 0x103c5748);
        }
        // A human manager's screen comes back as he left it.
        const auto ask = [&](std::uint32_t target, std::uint32_t back) {
            c.set({{3, c.u32(humans)}, {4, 0}});
            c.call(target, back);
            return c.r(3);
        };
        if (human) {
            c.put8(screen + field::view, c.u8(ask(0x10146d00, 0x103c5524)));
            c.put8(screen + field::list_view, c.u8(ask(0x10146e70, 0x103c553c)));
            c.set({{4, ask(0x10146a20, 0x103c5554)}, {5, screen}, {3, screen + at::shown_cell}});
            tactics_leaf_103baae0(c.s);
            c.set({{4, ask(0x10146b90, 0x103c5570)}, {5, screen}, {3, screen + at::ball_cell}});
            tactics_leaf_103baae0(c.s);
            c.put8(screen + at::free_kick_side, c.u8(ask(0x10146fe0, 0x103c558c)));
            c.put8(screen + at::substitutes, 0);
        } else {
            reset_view();
            c.set(13, screen);
        }
        // The roles from the tactics records (TACTICS.CPP, through the dispatcher).
        const auto ask_tactics = [&](std::uint32_t target, std::uint32_t back,
                                     std::initializer_list<std::pair<unsigned, std::uint32_t>> arguments) {
            c.set({{3, c.u32(tactics)}, {4, c.u32(screen + field::club)}});
            c.set(arguments);
            c.call(target, back);
            return c.r(3);
        };
        c.put32(screen + at::pick, ask_tactics(0x103b0b20, human ? 0x103c55bc : 0x103c57b8, {{5, managed()}}));
        if (!human) c.set(14, screen);
        for (std::uint32_t kind = 0; std::int8_t(kind) < 5; ++kind) {
            c.set(30, kind);
            for (std::uint32_t order = 0; std::int8_t(order) < 2; ++order) {
                c.set(29, order);
                if (!human) c.set(15, screen);
                const auto taker = ask_tactics(0x103b0e70, human ? 0x103c55f4 : 0x103c57f0,
                                               {{5, kind}, {6, order}, {7, managed()}});
                c.put32(screen + at::takers + indexed(kind, 8) + indexed(order, 4), taker);
                // The AI path's store leaves its operands for the next call.
                if (!human)
                    c.set({{0, sext8(kind)}, {3, indexed(kind, 8) + indexed(order, 4) + at::takers}, {4, taker},
                           {5, screen}, {8, indexed(order, 4)}});
            }
            c.set(29, 2);
        }
        c.set(30, 5);
        if (human) {
            c.link(0x103c5640);
            pick_lineup(c, screen);
        }
        for (std::uint32_t place = 0; std::int8_t(place) < 11; ++place) {
            c.set(30, place);
            if (!human) c.set(15, screen);
            const auto slot = ask_tactics(0x103b1670, human ? 0x103c5664 : 0x103c5858, {{5, place}, {6, managed()}});
            if (!human) c.set({{14, slot}, {13, screen}});
            c.put8(screen + at::lineup + indexed(place, 1), slot);
            if (!human) c.set({{3, sext8(place)}, {4, sext8(place) + at::lineup}});
        }
        c.set(30, 11);
        for (std::uint32_t which = 0; which < 2; ++which) {
            const auto back = human ? (which ? 0x103c56c8u : 0x103c56a4u) : (which ? 0x103c58c0u : 0x103c5898u);
            const auto captain = ask_tactics(0x103b1370, back, {{5, which}, {6, managed()}});
            c.put32(screen + at::captains + which * 4, captain);
            if (human) continue;
            if (which) c.set(4, screen);
            else c.set({{8, screen}, {9, captain}});
        }
        if (!human) c.set({{0, 1}, {5, screen}});
        close(human ? 0 : 1, human ? 0x103c5720 : 0x103c5918);
    }
    c.set({{3, fixtures}, {4, 0xffffffffu}});
    c.call(0x10738c20, 0x103c5924);
    return screen;
}

std::uint32_t list_opponents(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103cb0b0, 0x190, {.first_gpr = 23});
    const auto rules = c.toc(-0x7a64), players_db = c.toc(-0x7aa4), staff_db = c.toc(-0x7ba0);
    c.set({{31, screen}, {27, rules}, {28, players_db}, {29, staff_db}});
    // Default positions of the 11 shown players (0x38 bytes copied from a table).
    const auto positions = frame.at(0x60), table = c.toc(-0x4ce4);
    for (std::uint32_t offset = 0; offset < 0x38; offset += 4) c.put32(positions + offset, c.u32(table + offset));
    c.set(23, positions);
    c.set({{3, screen + at::opponents}, {4, 0}, {5, 0x50}});
    c.call(0x10778300, 0x103cb158);
    c.compare_logical(0, c.u32(screen + at::opponent), 0);
    if (!c.u32(screen + at::opponent)) return 0;
    c.compare(0, c.i8(screen + field::limit_squad), 0);
    if (c.i8(screen + field::limit_squad) == 0) return 0;
    // The opponent's players (50 squad entries at +0x98) who are fit to be shown.
    const auto players = frame.at(0x98), date = frame.at(0x58);
    std::uint32_t count = 0;
    c.set(25, count);
    for (std::uint32_t entry = 0; std::int8_t(entry) < 50; ++entry) {
        c.set(30, entry);
        c.set(4, c.u32(screen + at::opponent));  // left in r4 when the entry is skipped
        const auto player = c.u32(c.u32(screen + at::opponent) + indexed(entry, 4) + 0x98);
        c.set(26, player);
        if (!player || !c.u32(player + 0x14)) continue;
        c.set({{3, c.u32(staff_db)}, {4, player}});
        c.call(0x10187680, 0x103cb1c4);
        if (std::int8_t(c.r(3)) == 2) continue;
        c.set({{3, c.u32(players_db)}, {4, player}, {5, c.u32(screen + at::list_control)}, {6, 1}, {7, 0}});
        c.call(0x100e7920, 0x103cb1e8);
        if (std::int8_t(c.r(3)) != 0) continue;
        c.set({{3, date}, {4, std::uint32_t(std::int32_t(c.i16(screen + at::list_origin + 2)))},
               {5, c.u16(screen + at::list_origin)}});
        c.call(0x107156b0, 0x103cb208);
        c.set({{3, c.u32(rules)}, {4, player}, {5, date}, {6, c.u32(screen + at::opponent)}, {7, 0}, {8, 0}});
        c.call(0x102b7c70, 0x103cb224);
        if (c.r(3)) continue;
        c.put32(players + indexed(count, 4), player);
        c.set(25, ++count);
    }
    c.set(30, 50);
    c.compare(0, std::int8_t(count), 0);
    if (std::int8_t(count) == 0) return 0;
    c.set({{3, c.u32(c.toc(tactics_slot))}, {4, c.u32(screen + at::opponent)}, {5, 0}});
    c.call(0x103af570, 0x103cb27c);  // tactics::team_formation
    const auto formation = c.r(3);
    c.set(24, formation);
    if (formation) {
        for (std::uint32_t player = 0; std::int8_t(player) < 11; ++player) {
            c.set({{30, player}, {3, formation}, {4, player}, {5, 0}});
            c.call(0x10111240, 0x103cb2a0);  // formation::position
            c.put16(positions + indexed(player, 2), c.r(3));
            c.set(4, positions);
        }
        c.set(30, 11);
    }
    c.set(3, c.u32(screen + at::opponent));
    c.call(0x100c7d00, 0x103cb2c8);
    const bool club = std::int8_t(c.r(3)) != 0;
    for (std::int32_t offset = 0x38; offset <= 0x50; offset += 4) c.put32(frame.at(offset), 0);
    c.set({{3, players}, {4, count}, {5, positions}, {6, 11}, {7, (c.u8(screen + at::list_style) >> 4) & 0xf},
           {8, screen + at::opponents}, {9, club ? 0x2014u : 0x2042u}, {10, c.u32(screen + at::opponent)}});
    c.call(0x10210c50, club ? 0x103cb338 : 0x103cb3a0);
    return 1;
}

std::uint32_t pick_lineup(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c7690, 0x90, {.first_gpr = 19});
    const auto humans = c.toc(humans_slot), tactics = c.toc(tactics_slot);
    c.set({{25, screen}, {31, humans}, {28, tactics}});
    // How many of the 20 squad entries can play (counted, not used).
    std::uint32_t available = 0;
    c.set(20, available);
    for (std::uint32_t entry = 0; std::int8_t(entry) < 20; ++entry) {
        c.set({{24, entry}, {3, screen}, {4, entry}, {5, 0}, {6, 0}});
        c.call(0x103c9810, 0x103c76cc);
        if (std::int8_t(c.r(3)) != 0) c.set(20, ++available);
    }
    c.set(24, 20);
    const auto used = frame.at(0x38);
    c.set({{3, used}, {4, 0}, {5, 0x14}});
    c.call(0x10778300, 0x103c76fc);
    // For each place, the unused available entry with the lowest rank from
    // 0x1014d300 (-1 counting as 0x33), ties going to the higher byte +0x38.
    const auto rank = [&](std::uint32_t player, std::uint32_t back) {
        c.set({{3, c.u32(humans)}, {4, player}, {5, 0}});
        c.call(0x1014d300, back);
        return sext8(c.r(3));
    };
    const auto tiebreak = [&](std::uint32_t player) { return c.i8(c.u32(player + 0x14) + 0x38); };
    for (std::uint32_t place = 0; std::int8_t(place) < 11; ++place) {
        c.set(23, place);
        std::uint32_t best = 0xffffffffu, best_player = 0;
        c.set({{27, best}, {26, best_player}});
        for (std::uint32_t entry = 0; std::int8_t(entry) < 20; ++entry) {
            c.set({{30, entry}, {3, screen}, {4, entry}, {5, 0}});
            c.call(0x103ca4e0, 0x103c7724);
            const auto player = c.r(3);
            c.set(29, player);
            if (!player || !c.u32(player + 0x14)) continue;
            c.set({{3, screen}, {4, entry}, {5, 0}, {6, 0}});
            c.call(0x103c9810, 0x103c7750);
            if (std::int8_t(c.r(3)) == 0) continue;
            if (c.i8(used + sext8(entry)) != 0) continue;
            auto mine = rank(player, 0x103c7784);
            if (mine == 0xffffffffu) mine = 0x33;
            c.set(22, mine);
            const auto theirs = best_player ? rank(best_player, 0x103c77ac) : 0xffffffffu;
            c.set(19, theirs);
            auto other = sext8(theirs);
            if (other == 0xffffffffu) other = 0x33;
            c.set(21, other);
            const bool better = std::int8_t(best) == -1 || std::int8_t(mine) < std::int8_t(other) ||
                                (std::int8_t(mine) == std::int8_t(other) && tiebreak(player) > tiebreak(best_player));
            if (!better) continue;
            best = entry;
            best_player = player;
            c.set({{27, best}, {26, best_player}});
        }
        c.set({{30, 20}, {3, c.u32(tactics)}, {4, c.u32(screen + field::club)}, {5, place}, {6, best},
               {7, c.u8(screen + field::opened_from)}});
        c.call(0x103b17b0, 0x103c7850);  // tactics::set_lineup_slot
        c.compare(0, std::int8_t(best), -1);
        if (std::int8_t(best) > -1) {
            c.put8(used + sext8(best), 1);
            c.set({{4, 1}, {3, used}});
        }
    }
    c.compare(0, 11, 11);  // the loop's last test
    return 1;
}
}
