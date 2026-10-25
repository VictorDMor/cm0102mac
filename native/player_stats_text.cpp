#include "player_stats_detail.hpp"

namespace cm::aot::player_stats {
using namespace detail;
namespace {
// localize into `buffer`; texts with a "<%s - COMMENT ...>" part get the
// empty string as their argument (r5), the others leave r5 as it is.
void localized(Cpu& c, std::uint32_t buffer, std::uint32_t pool, std::uint32_t text, bool commented,
               std::uint32_t back) {
    if (commented) Program{c}.call(routine::localize, back, {buffer, pool + text, pool + text::empty});
    else Program{c}.localize(buffer, pool + text, back);
}
}

std::uint32_t competition_name(Cpu& c, std::int8_t type, std::uint32_t buffer, std::int32_t size) {
    Frame frame(c, 0x10302cb0, 0xa0, {.first_gpr = 29});
    const auto pool = c.toc(slot::pool);
    keep(c, {{29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {31, pool}});
    if (!nonzero_logical(c, buffer) || compare(c, size, minimum_text_buffer) < 0) {
        report(c, pool, 0x10302cf8, frame.at(0x68), frame.at(0x40), 0x3ce, false);
        return 0;
    }
    c.put8(buffer, 0);
    const auto name = [&](std::uint32_t text, bool commented, std::uint32_t back) {
        localized(c, buffer, pool, text, commented, back);
        return 1u;
    };
    // The original's switch compares as a binary search (4, then 6 or 2,
    // then 1); each compare reaches cr0.
    const std::int32_t value = type;
    if (const auto against_4 = compare(c, value, 4); against_4 == 0) return name(text::continental, true, 0x10302dcc);
    else if (against_4 > 0) {
        const auto against_6 = compare(c, value, 6);
        if (against_6 == 0) return name(text::senior_club, false, 0x10302df8);
        if (against_6 < 0) return name(text::international, true, 0x10302de4);
    } else {
        const auto against_2 = compare(c, value, 2);
        if (against_2 == 0) return name(text::league, false, 0x10302da0);
        if (against_2 > 0) return name(text::cup, false, 0x10302db4);
        if (compare(c, value, 1) >= 0) return name(text::non_competitive, true, 0x10302d8c);
    }
    report(c, pool, 0x10302e14, frame.at(0x48), frame.at(0x38), 0x404, false);
    return 0;
}

void recent_ratings_text(Cpu& c, std::uint32_t stats, std::uint32_t player, std::uint32_t buffer, std::int32_t size) {
    Frame frame(c, 0x10302e70, 0xe0, {.first_gpr = 25});
    Program program{c};
    const auto pool = c.toc(slot::pool);
    const bool has_player = nonzero(c, std::int32_t(player));  // or. r26,r4,r4
    // r28 holds the success flag's address, later the player's entry.
    Kept<28> held{c, c.toc(slot::success)};
    keep(c, {{25, stats}, {26, player}, {27, std::uint32_t(size)}, {29, c.toc(slot::message)}, {30, buffer}, {31, pool}});
    c.put8(buffer, 0);
    // Both reports fall through: the original goes on (reading player id 0
    // at address 0 when `player` is null).
    if (!has_player) report(c, pool, 0x10302ebc, frame.at(0x68), frame.at(0x40), 0x41d, false);
    if (!nonzero_logical(c, buffer) || compare(c, size, minimum_text_buffer) < 0)
        report(c, pool, 0x10302f1c, frame.at(0x48), frame.at(0x38), 0x424, false);
    const std::int32_t id = Field<std::int32_t>{c, player};
    c.set(4, std::uint32_t(id));
    if (compare(c, id, Field<std::int32_t>{c, c.toc(slot::stats_players)}) >= 0) {
        program.sprintf(0x10302f78, {buffer, pool + text::dash});
        return;
    }
    const PlayerEntry entry{c, Stats{c, stats}.players() + std::uint32_t(id) * PlayerEntry::size};
    held = entry.address;
    // The five ratings, oldest first: high and low nibbles of bytes 0-1,
    // then the high nibble of byte 2; each nonzero one appends "%d-".
    const auto part = frame.at(0x88);
    for (std::uint32_t n = 0; n < 5; ++n) {
        const std::uint8_t byte = entry.ratings(n / 2);
        const std::uint32_t rating = n % 2 ? byte & 0xf : byte >> 4;
        c.set(5, rating);
        if (!nonzero(c, std::int32_t(rating))) continue;
        program.sprintf(0x10302fa0 + n * 0x24, {part, pool + text::number_dash, rating});
        program.strcat(buffer, part, 0x10302fac + n * 0x24);
    }
    // Drop the last "-"; "-" alone when nothing was written. (A text of
    // one character, impossible here, also becomes "-".)
    if (nonzero(c, c.i8(buffer)) && compare_logical(c, program.strlen(buffer, 0x10303050), 2) >= 0) {
        keep(c, {{31, 0}});  // li r31,0: the original's zero for the store
        const auto length = program.strlen(buffer, 0x10303064);
        c.set(3, buffer + length);
        c.put8(buffer + length - 1, 0);
        return;
    }
    program.strcpy(buffer, pool + text::dash, 0x1030307c);
}

std::uint32_t column_title(Cpu& c, std::int8_t column, std::uint32_t buffer, std::int32_t size) {
    Frame frame(c, 0x103032e0, 0xa0, {.first_gpr = 29});
    const auto pool = c.toc(slot::pool);
    keep(c, {{29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {31, pool}});
    if (!nonzero_logical(c, buffer) || compare(c, size, minimum_text_buffer) < 0) {
        report(c, pool, 0x10303328, frame.at(0x68), frame.at(0x40), 0x5bc, false);
        return 0;
    }
    c.put8(buffer, 0);
    // The original's jump table covers columns 0-17; 0 and the rest assert.
    struct Title {
        std::uint32_t text;
        bool commented;
    };
    constexpr Title titles[18] = {{0, false},
                                  {text::apps_short, false},
                                  {text::goals_short, false},
                                  {text::conceded_short, true},
                                  {text::penalties_short, false},
                                  {text::assists_short, false},
                                  {text::team_goals_short, false},
                                  {text::team_conceded_short, false},
                                  {text::won_short, true},
                                  {text::lost_short, false},
                                  {text::yellow_short, false},
                                  {text::red_short, false},
                                  {text::man_of_match_short, false},
                                  {text::pass_short, false},
                                  {text::tackles_short, false},
                                  {text::dribbles_short, false},
                                  {text::shots_short, false},
                                  {text::rating_short, false}};
    // The return address of each case's call.
    constexpr std::uint32_t backs[18] = {0,          0x103033a0, 0x103033b4, 0x103033cc, 0x103033e0, 0x103033f4,
                                         0x10303408, 0x1030341c, 0x10303434, 0x10303448, 0x1030345c, 0x10303470,
                                         0x10303484, 0x103034ac, 0x103034c0, 0x103034d4, 0x103034e8, 0x10303498};
    const auto index = std::uint32_t(std::int32_t(column));
    if (compare_logical(c, index, 17) <= 0 && index != 0) {
        localized(c, buffer, pool, titles[index].text, titles[index].commented, backs[index]);
        return 1;
    }
    report(c, pool, 0x10303504, frame.at(0x48), frame.at(0x38), 0x63f, false);
    return 0;
}

std::uint32_t player_value_text(Cpu& c, std::uint32_t stats, std::uint32_t player, std::uint32_t type,
                                std::uint32_t column, std::uint32_t buffer, std::int32_t size, std::uint32_t detail,
                                std::uint32_t per_appearance, std::uint32_t zeros) {
    Frame frame(c, 0x10303ff0, 0xc0, {.first_gpr = 20});
    Program program{c};
    const bool has_buffer = nonzero(c, std::int32_t(buffer));  // or. r23,r7,r7
    const auto pool = c.toc(slot::pool);
    keep(c, {{20, player}, {21, type}, {22, column}, {23, buffer}, {24, std::uint32_t(size)}, {25, detail},
             {26, per_appearance}, {27, zeros}, {28, c.toc(slot::success)}, {29, c.toc(slot::message)}, {30, pool},
             {31, stats}});
    if (!has_buffer || compare(c, size, minimum_text_buffer) < 0) {
        report(c, pool, 0x10304050, frame.at(0x68), frame.at(0x40), 0x7c5, false);
        return 0;
    }
    constexpr std::int32_t average_rating = 17;  // shows "----" when empty
    if (compare(c, std::int8_t(column), average_rating) == 0) program.strcpy(buffer, pool + text::no_value, 0x103040b0);
    else program.strcpy(buffer, pool + text::dash, 0x103040c0);
    if (!nonzero_logical(c, player)) {
        report(c, pool, 0x103040dc, frame.at(0x48), frame.at(0x38), 0x7d7, false);
        return 0;
    }
    const std::int32_t id = Field<std::int32_t>{c, player};
    c.set(4, std::uint32_t(id));
    // Not a player: nothing to show, but no failure either.
    if (compare(c, id, Field<std::int32_t>{c, c.toc(slot::stats_players)}) >= 0) return 1;
    enter(c, 0x1030414c, {stats, player, type});
    const auto record = season_record(c, stats, player, std::int8_t(type));
    c.set(4, record);
    if (!nonzero(c, std::int32_t(record))) return 0;
    enter(c, 0x10304174, {stats, record, column, buffer, std::uint32_t(size), detail, per_appearance, zeros});
    return season_value_text(c, record, std::int8_t(column), buffer, size, detail, per_appearance, zeros);
}
}
