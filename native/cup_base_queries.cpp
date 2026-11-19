// CUP.CPP, block `queries`: the news a cup files for a result (0x1047b360), the places it records with the
// staff-records object (0x1047b5b0), the final places it hands to COMP_UTIL (0x1047b7d0, 0x1047bde0) and the tie
// of a fixture (0x1047ba50). The news items write news records and the places write club histories and the club
// table: the other modules' routines are called through the dispatcher, with the non-volatile registers the
// original holds set before each call (they can reach imports; the news link rows copy register save areas).
// Registers r4-r10 are left as the original leaves them on every path (the checker compares them).
#include "aot_runtime.hpp"
#include "cup_base_queries_detail.hpp"

namespace cm::aot::cup_base::queries {
namespace {
constexpr std::int32_t history_cell = -0x7b48,     // cell: the staff-records object (0x10383410 and friends)
    club_table_cell = -0x7984,                     // cell: a table of 8-byte rows by club id: a word, -1 when free (inferred)
    word_cell_691c = -0x691c, word_cell_7a70 = -0x7a70, word_cell_7a4c = -0x7a4c, uefa_cell = -0x7a48,
    host_cell = -0x7424, world_cell = -0x7b98, class_cell_6af4 = -0x6af4;
constexpr std::uint32_t club_nation = 0x2c, nation_object = 0x74;
std::uint32_t signed_divide(std::int32_t dividend, std::int32_t divisor) {
    return std::uint32_t(divide_signed<4>(std::uint32_t(dividend), std::uint32_t(divisor)));
}
}

std::uint32_t file_result_news(Cpu& c, std::uint32_t cup, std::uint32_t club, std::uint32_t kind, std::uint32_t argument_6,
                               std::uint32_t fixture, std::uint32_t argument_8) {
    const Frame frame(c, 0x1047b360, 0x140, {.first_gpr = 25});
    Held h(c);
    const auto manager = c.toc(toc::news), rules = c.toc(comp::toc::fixture_rules), competitions = c.toc(toc::competitions);
    const auto record = c.u32(cup + competition::record);
    // r25 and r26 the two pass-through arguments (r26 then the virtual's answer), r27 the cup, r28 the club, r29 the kind,
    // r30 the fixture, r31 the news manager's cell.
    h.v[25] = argument_6; h.v[26] = argument_8; h.v[27] = cup; h.v[28] = club; h.v[29] = kind; h.v[30] = fixture; h.v[31] = manager;
    const auto out = frame.at(0x39), field_byte = frame.at(0x38), note = frame.at(0x3c);
    c.put8(out, 0xff);
    c.set(9, rules);
    h.publish();
    if (std::int8_t(invoke(c, 0x10494d20, 0x1047b3a4, {c.u32(rules), record})) != 0) return 0;
    h.publish();
    const auto own = c.u32(c.u32(competitions) + c.u32(record) * 4);
    const auto answer = virtual_call(c, 0x4c, 0x1047b3f0, {own, club, kind, argument_6, out, fixture, argument_8});
    h.v[26] = answer;
    if (std::int32_t(answer) == -1) return 0;
    if (fixture == 0) return 0;
    const auto field = [&](std::uint32_t n, std::uint32_t value, std::uint32_t back) {
        h.publish();
        invoke(c, item::set_field, back, {note, n, value});
    };
    h.publish();
    invoke(c, item::start, 0x1047b428, {note, 0x10, 0});
    field(0, c.u32(club), 0x1047b438);
    field(1, c.u32(record), 0x1047b44c);
    field(2, sext8(kind), 0x1047b45c);
    h.publish();
    field(3, sext8(invoke(c, 0x10464900, 0x1047b468, {club, fixture})), 0x1047b478);
    field(4, answer, 0x1047b488);
    c.put8(field_byte, 5);
    h.publish();
    invoke(c, 0x10463be0, 0x1047b4a0, {fixture, note, field_byte});
    const auto round = c.i16(fixture + fixture::round);
    const auto rounds = std::int32_t(c.i16(cup + competition::rounds));
    std::uint32_t label = 0;
    if (round < rounds - 1) label = sext16(c.u16(c.u32(cup + cup::rounds) + std::uint32_t(round + 1) * cup::round_row + round::label));
    if (round < rounds - 1 && label != 0x8c) field(c.u8(field_byte), label, 0x1047b4dc);
    else field(c.u8(field_byte), 0xffffffffu, 0x1047b4f0);
    const auto out_byte = c.u8(out);
    const auto filed = [&](std::uint32_t entry, std::uint32_t back, std::uint32_t target) {
        h.publish();
        invoke(c, entry, back, {c.u32(manager), note, target});
    };
    if (std::int8_t(out_byte) != -1) {
        const auto nation = c.u32(club + club_nation);
        h.publish();
        invoke(c, item::file_with_competition, 0x1047b534,
               {c.u32(manager), note, club, record, nation, nation ? c.u32(nation + nation_object) : 0, out_byte});
        return 1;
    }
    const auto code = c.i16(fixture + fixture::code);
    if ((std::int8_t(kind) == 1 && code == 0x96) || (std::int8_t(kind) == 3 && code == 0x8c))
        filed(callee_news::file_under_record_cup, 0x1047b578, record);
    else
        filed(item::file_under_club, 0x1047b58c, club);
    return 1;
}

std::uint32_t record_place(Cpu& c, std::uint32_t cup, std::uint32_t club, std::uint32_t kind, std::uint32_t fixture,
                           std::uint32_t argument_9) {
    const Frame frame(c, 0x1047b5b0, 0x50, {.first_gpr = 29});
    Held h(c);
    const auto history = c.toc(history_cell), table_cell = c.toc(club_table_cell);
    h.v[29] = cup; h.v[30] = club; h.v[31] = table_cell;
    if (c.u8(fixture + fixture::byte_3a) == 1) return 0;
    if (c.u32(cup + competition::word_2c) != 0) return 0;
    if (std::int8_t(c.u8(cup + competition::owner)) != -1) return 0;
    const auto record = c.u32(cup + competition::record);
    c.set(6, sext8(kind));
    const auto call = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        h.publish();
        return invoke(c, entry, back, arguments);
    };
    switch (std::int8_t(kind)) {
    case 1: {
        call(0x10383410, 0x1047b61c, {c.u32(history), club, fixture, argument_9});
        // The club table's entry for the club (-1: free) is set to the word at TOC -0x7a4c, unless the record id differs from the one
        // at TOC -0x691c and the record's nation has no class (+0x74/+8) equal to the word at TOC -0x7a70, or the fixture rules
        // refuse, the club's entry is taken, or the nation descriptor 0x10192130 names does not name this record (the original's order).
        const auto id = c.u32(record);
        c.set({{4, record}, {5, id}});
        if (id != c.u32(c.toc(word_cell_691c))) {
            const auto holder = c.u32(record + 8);
            if (holder == 0) return 0;
            const auto object = c.u32(holder + 0x74);
            c.set(5, object);
            if (object == 0) return 0;
            const auto class_id = c.u32(object + 8);
            c.set(5, class_id);
            if (class_id != c.u32(c.toc(word_cell_7a70))) return 0;
            if (std::int8_t(call(0x10494d20, 0x1047b66c, {c.u32(c.toc(comp::toc::fixture_rules)), record})) != 0) return 0;
            if (c.u32(c.u32(table_cell) + c.u32(club) * 8) != 0xffffffffu) return 0;
            if (std::int8_t(call(0x10192130, 0x1047b694, {record})) == -1) return 0;
            const auto nation = std::int8_t(call(0x10192130, 0x1047b6a8, {record}));
            const auto descriptors = c.toc(comp::toc::descriptors);
            c.set(4, descriptors);
            if (c.u32(descriptors + std::uint32_t(std::int32_t(nation)) * comp::descriptor::size + 0x20) != record) return 0;
        }
        const auto value = c.u32(c.toc(word_cell_7a4c));
        c.set(4, value);
        c.put32(c.u32(table_cell) + c.u32(club) * 8, value);
        return 0;
    }
    case 2:
        call(0x10383b70, 0x1047b6fc, {c.u32(history), club, fixture, argument_9});
        return 0;
    case 3:
        call(0x10384020, 0x1047b718, {c.u32(history), club, fixture, argument_9});
        return 0;
    case 0: {
        const auto round = std::int32_t(c.i16(fixture + fixture::round)), rounds = std::int32_t(c.i16(cup + competition::rounds));
        c.set({{9, std::uint32_t(rounds)}, {10, std::uint32_t(round)}});
        const auto next = std::int32_t(std::int16_t(round + 1));
        if (next >= rounds) return 0;
        const auto table = c.u32(cup + cup::rounds);
        c.set({{5, std::uint32_t(next) * cup::round_row}, {6, table}});
        const auto label = sext16(c.u16(table + std::uint32_t(next) * cup::round_row + round::label));
        c.set(7, label);
        if (label == 0x8c) return 0;
        const auto quotient = signed_divide(round * 0x63, rounds - 1);
        c.set(9, quotient);
        call(0x10384500, 0x1047b778, {c.u32(history), club, record, sext16(c.u16(fixture + fixture::half_32)), label,
                                      sext8(quotient + 1)});
        return 0;
    }
    case -1: {
        const auto round = std::int32_t(c.i16(fixture + fixture::round)), rounds = std::int32_t(c.i16(cup + competition::rounds));
        const auto quotient = signed_divide(round * 0x63, rounds);
        c.set(9, quotient);
        call(0x103846e0, 0x1047b7b0, {c.u32(history), club, record, sext16(c.u16(fixture + fixture::half_32)),
                                      sext16(c.u16(fixture + fixture::code)), sext8(quotient + 1)});
        return 0;
    }
    default:
        return 0;
    }
}

void record_final_places(Cpu& c, std::uint32_t cup) {
    const Frame frame(c, 0x1047b7d0, 0x60, {.first_gpr = 24});
    Held h(c);
    const auto world = c.toc(world_cell), competitions_cell = c.toc(toc::competitions);
    // r24 the cup, r25 the host nation's club (0), r26 the third, r27 the second, r28 the winner, r29 the entrant index, r30 the
    // world table's cell, r31 the competitions' cell.
    auto& host = h.v[25]; auto& third = h.v[26]; auto& second = h.v[27]; auto& winner = h.v[28]; auto& index = h.v[29];
    h.v[24] = cup; host = 0; third = 0; second = 0; winner = 0; index = 0; h.v[30] = world; h.v[31] = competitions_cell;
    const auto entrants = [&] { return std::int32_t(c.i16(cup + competition::club_count)); };
    while (std::int16_t(index) < entrants()) {
        const auto row = c.u32(cup + cup::entrants) + sext16(index) * cup::entrant_row;
        switch (std::int8_t(c.u8(row + 5))) {
        case 1: winner = c.u32(row); break;
        case 2: second = c.u32(row); break;
        case 3: third = c.u32(row); break;
        default: break;
        }
        if (winner != 0 && second != 0 && third != 0) break;
        ++index;
    }
    const auto record = c.u32(cup + competition::record);
    if (record != 0 && c.u32(record) == c.u32(c.toc(class_cell_6af4))) {
        // The hosts: the first entrant COMP_STATS says has state 3 in the competition the record names.
        index = 0;
        while (std::int16_t(index) < entrants()) {
            const auto own = c.u32(c.u32(competitions_cell) + c.u32(c.u32(cup + competition::record)) * 4);
            const auto club = c.u32(c.u32(cup + cup::entrants) + sext16(index) * cup::entrant_row);
            c.set(5, own);
            h.publish();
            const auto stats_row = invoke(c, 0x10459b50, 0x1047b8c8, {c.u32(own + competition::stats), club});
            if (c.u8(stats_row + 0x76) == 3) {
                third = c.u32(c.u32(cup + cup::entrants) + sext16(index) * cup::entrant_row);
                break;
            }
            ++index;
        }
    }
    const auto record_now = [&] { return c.u32(cup + competition::record); };
    // The cup's own competition object (the one the record's id names) says whether the host nation applies (virtual +0x90).
    c.set(4, c.u32(competitions_cell));
    h.publish();
    const auto own = c.u32(c.u32(competitions_cell) + c.u32(record_now()) * 4);
    if (std::int8_t(virtual_call(c, 0x90, 0x1047b920, {own})) != 0) {
        h.publish();
        const auto status = std::int8_t(invoke(c, 0x1049d1c0, 0x1047b954,
            {c.u32(c.toc(host_cell)), c.u32(record_now()), c.u16(cup + competition::year), frame.at(0x38), frame.at(0x3c), 0}));
        const auto first_row = c.u32(frame.at(0x38)), second_row = c.u32(frame.at(0x3c));
        const auto base = c.u32(world);
        if (status == 2) {
            // The one of the two nation rows (0x128 bytes) with fewer of the halfword +0xac (signed), the first on a tie.
            const auto a = sext16(c.u16(base + first_row * 0x128 + 0xac)), b = sext16(c.u16(base + second_row * 0x128 + 0xac));
            c.set({{4, a}, {5, first_row * 0x128}, {6, base}});
            h.publish();
            host = invoke(c, 0x100c76c0, std::int32_t(a) < std::int32_t(b) ? 0x1047b9a4 : 0x1047b994,
                          {base + (std::int32_t(a) < std::int32_t(b) ? second_row : first_row) * 0x128});
        } else if (status != 0) {
            h.publish();
            host = invoke(c, 0x100c76c0, 0x1047b9c8, {base + first_row * 0x128});
        } else {
            host = 0;
        }
    }
    h.publish();
    invoke(c, 0x10463990, 0x1047b9ec, {cup, winner, second, third, host});
    h.publish();
    if (std::int8_t(invoke(c, 0x10494d20, 0x1047b9fc, {c.u32(c.toc(comp::toc::fixture_rules)), record_now()})) != 0) return;
    h.publish();
    const auto nation = std::int8_t(invoke(c, 0x10192130, 0x1047ba0c, {record_now()}));
    const auto descriptors = c.toc(comp::toc::descriptors);
    c.set(4, descriptors);
    if (c.u32(descriptors + std::uint32_t(std::int32_t(nation)) * comp::descriptor::size + 0x20) != record_now()) return;
    h.publish();
    c.set(3, cup);
    c.link(0x1047ba34);
    file_uefa_seeding(c, cup);
}

std::int32_t tie_index(Cpu& c, std::uint32_t cup, std::uint32_t fixture) {
    const Frame frame(c, 0x1047ba50, 0x70, {.first_gpr = 30});
    if (fixture == 0) {
        // r30 the message buffer, r31 the pool at the report's calls; the original clears the flag through its address in r4.
        c.set({{30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
        report(c, frame, {0x1047ba88, 0x40, 0x38, 0x387});
        c.set(4, c.toc(toc::success));
        return -1;
    }
    const auto round = std::int32_t(c.i16(fixture + fixture::round));
    const auto wanted = [&] { return c.u32(fixture + fixture::club_id); };
    // r7 and r8 at each return are what the original's last pass loaded: the club row and the fixture's club id on a find,
    // the round's row offset and the round table when the passes run out.
    const auto leave = [&](std::int32_t index, std::uint32_t offset, std::uint32_t r7, std::uint32_t r8) {
        c.set({{4, fixture}, {5, std::uint32_t(index)}, {6, offset}, {7, r7}, {8, r8}});
    };
    std::int32_t index = 0;
    std::uint32_t offset = 0;
    for (;; ++index, offset += cup::match_row) {
        const auto rounds = c.u32(cup + cup::rounds), row_offset = std::uint32_t(round) * cup::round_row;
        if (index >= std::int32_t(sext16(c.u16(rounds + row_offset + round::ties)))) {
            leave(index, offset, row_offset, rounds);
            return -1;
        }
        for (const std::uint32_t half : {0u, 2u}) {
            const auto matches = c.u32(c.u32(cup + cup::round_matches) + std::uint32_t(round) * 4);
            const auto side = std::int32_t(sext16(c.u16(matches + offset + half)));
            if (side < 0) continue;
            const auto club = c.u32(c.u32(cup + cup::entrants) + std::uint32_t(side) * cup::entrant_row);
            if (wanted() == c.u32(club)) {
                leave(index, offset, club, wanted());
                return index;
            }
        }
    }
}

void file_uefa_seeding(Cpu& c, std::uint32_t cup) {
    const Frame frame(c, 0x1047bde0, 0x40, {});
    // The original's volatile registers: r4 the counter (a word, read as its signed halfword), r5 and r6 the two clubs
    // (-1: not found), r7/r8 the first and second side of the final's tie, r9/r10 the semi-final round and the final.
    std::uint32_t r4 = 0, r5 = 0xffffffffu, r6 = 0xffffffffu, r7 = c.r(7), r8 = c.r(8), r9 = 0xffffffffu, r10 = 0xffffffffu;
    const auto leave = [&] { c.set({{4, r4}, {5, r5}, {6, r6}, {7, r7}, {8, r8}, {9, r9}, {10, r10}}); };
    const auto rounds = [&] { return std::int32_t(c.i16(cup + competition::rounds)); };
    const auto half_at = [&](std::uint32_t at) { return sext16(c.u16(at)); };
    for (;; ++r4) {
        r7 = sext16(r4);
        if (std::int32_t(r7) >= rounds()) break;
        r8 = c.u32(cup + cup::rounds);
        r7 = sext16(r4) * cup::round_row;
        const auto label = std::int32_t(half_at(r8 + r7 + round::label));
        if (label == 0x82) r9 = r4;
        else if (label == 0x96) r10 = r4;
        if (std::int32_t(sext16(r9)) != -1 && std::int32_t(sext16(r10)) != -1) break;
    }
    r9 = sext16(r9);
    if (std::int32_t(r9) == -1) return leave();
    const auto final_round = sext16(r10);
    if (std::int32_t(final_round) == -1) return leave();
    const auto semis = r9;                   // the round labelled 0x82 (r9, then re-used); r0 = its index * 4
    r8 = semis * cup::round_row;
    r9 = c.u32(c.u32(cup + cup::round_matches) + final_round * 4);
    r4 = 0;
    r7 = half_at(r9 + tie::side_0);
    r10 = r8 + round::ties;
    r8 = half_at(r9 + tie::side_1);
    // One side of a tie of the semi-final round: a club other than the final's two sides, as the first or the second.
    enum class Outcome { next, leave, call };
    const auto half = [&](std::uint32_t extra) {
        r9 = sext16(r4) * cup::match_row + extra;
        const auto side = half_at(c.u32(c.u32(cup + cup::round_matches) + semis * 4) + r9);
        if (r7 == side || r8 == side) return Outcome::next;
        if (std::int32_t(side) == -1) return Outcome::leave;
        const auto club_row = c.u32(c.u32(cup + cup::entrants) + side * cup::entrant_row);
        if (r5 == 0xffffffffu) {
            r9 = c.u32(cup + cup::entrants);
            r5 = c.u32(club_row);
            return Outcome::next;
        }
        r4 = club_row;
        r6 = c.u32(club_row);
        return Outcome::call;
    };
    bool call = false;
    for (;; ++r4) {
        r9 = c.u32(cup + cup::rounds);
        r9 = half_at(r9 + r10);
        if (!(std::int32_t(sext16(r4)) < std::int32_t(r9))) break;
        auto outcome = half(0);
        if (outcome == Outcome::next) outcome = half(2);
        if (outcome == Outcome::leave) return leave();
        if (outcome == Outcome::call) { call = true; break; }
    }
    (void)call;
    r4 = c.u32(cup + competition::record);
    r7 = c.toc(uefa_cell);
    r4 = c.u32(r4);
    c.set({{5, r5}, {6, r6}, {7, r7}, {8, r8}, {9, r9}, {10, r10}});
    invoke(c, 0x104b7860, 0x1047bf94, {c.u32(r7), r4, r5, r6});
}
}
