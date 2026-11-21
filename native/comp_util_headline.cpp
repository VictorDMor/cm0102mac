// COMP_UTIL.CPP, block `result_headline`: the news item about a club's result in a fixture (0x10464d00).
//
// The routine builds the headline ("X are through to the Quarter-finals after beating Y over 3 matches", "X have lost
// in the Cup to Y on penalties after a 2-2 draw", ...) of a fixture for one of its two clubs, and appends it to a news
// item with the clubs, the score text and the competition. The text is one of 173 localized sentences chosen by a
// decision tree whose leaves are all of seven shapes, so the tree is data: comp_util_headline_tables.inc is drafted
// from the original's code by tools/comp_util_headline.py (it walks every path, names each condition as a fact and
// each argument as an operand, and checks that exactly one sentence matches any state). This file computes the facts
// and the operands from the fixture, then makes the original's calls in the original's order with the registers and
// the stack words it leaves for them: the localized format (0x10727890) is a varargs routine that takes the sentence's
// arguments from r5..r10 and the stack words from sp+0x38, and every callee saves the caller's non-volatile registers.
//
// What the parameters mean (inferred from the callers and the sentences): `club` is the club the news is about, an
// index into the club table; `grade` the result's significance class (0x10464900 answers 1..7: 2 excellent, 3 fine,
// 5 disappointing, 6 a shock, 7 very disappointing); `side` says whether the club goes through (any value but 2, 4 and
// -1) or is out; `round_code` is the round the tie belongs to (a halfword: above 0 the round's name is asked for,
// -2 is a special value of the aggregate sentences); `item` the news item.
#include "comp_util_detail.hpp"
#include <stdexcept>

namespace cm::aot::comp_util {
namespace {
constexpr std::uint32_t format_entry = 0x10727890, copy_entry = 0x1077dbf0, sprintf_entry = 0x1077b720, language_entry = 0x10727880,
                        length_entry = 0x1077dbd0, begin_item_entry = 0x1074b830, append_entry = 0x1074bab0, stats_entry = 0x1029eb20,
                        date_entry = 0x107156b0, friendly_entry = 0x1048fa50;
// TOC slots.
constexpr std::int32_t pool_slot = -0x47e4, empty_buffer_slot = -0x47e8, clubs_slot = -0x7be8, records_slot = -0x7bbc,
                       text_buffer_slot = -0x7bb8, special_id_slot = -0x7bc0, cup_id_slot = -0x7ad8, other_cup_id_slot = -0x7ae0,
                       cup_division_slot = -0x7970, other_cup_division_slot = -0x6b94, stats_slot = -0x7b4c, friendly_slot = -0x7b54;
// Offsets in the pool: the score text's format and the first leg's two (the languages 0 and 1, then the others).
constexpr std::uint32_t score_format = 0x21, first_leg_formats[2] = {0x49, 0x50};
// The frame (0x450 bytes): the stack words of the formatter's varargs from 0x38, the date at 0x58, the score text at
// 0x60, the round's text at 0x70 (100 bytes, first filled from the table at TOC -0x47e8), the first leg's name at
// 0xd4 and the competition's name at 0x200 (500 bytes).
constexpr std::uint32_t frame_size = 0x450, argument_words = 0x38, date_slot = 0x58, score_slot = 0x60, round_slot = 0x70, first_leg_slot = 0xd4,
                        competition_slot = 0x200, item_slot = 0x2c /* in the caller's frame: the original saves r8 there */;
constexpr std::uint32_t round_text_size = 100, competition_text_size = 500;
// What a news item's appenders are given as the kind of each row (the club, the score, the competition).
constexpr std::uint32_t kind_club = 0x7d5, kind_score = 0x7d3, kind_competition = 0x7d0;
// A club's league and division records (the fields the two special competitions look at, inferred) and the id.
constexpr std::uint32_t club_league = 0x2c, league_division = 0x74, division_id = 0x08;
constexpr std::uint32_t first_leg_record_gender = 0x4b, first_leg_record_name = 0x18, first_leg_record_club = 0x10, club_name_in_record = 0x1c;

std::uint32_t signed_byte(std::uint32_t value) { return sext8(std::uint8_t(value)); }
std::uint32_t signed_half(std::uint32_t value) { return sext16(std::uint16_t(value)); }

// ---- the data (comp_util_headline_tables.inc) -----------------------------------------------------------------
// The facts a sentence depends on, one bit each.
namespace Fact {
enum : std::uint32_t {
    result_positive = 1u << 0, result_minus_two = 1u << 1,
    // the tie's decider (the first of the fixture's two tie-break bytes that is nonzero)
    decider_aggregate = 1u << 2, decider_penalties = 1u << 3, decider_shootout = 1u << 4, decider_away_goals = 1u << 5,
    decider_golden_goal = 1u << 6, decider_seeding = 1u << 7,
    grade_excellent = 1u << 8, grade_fine = 1u << 9, grade_disappointing = 1u << 10, grade_shock = 1u << 11,
    grade_very_disappointing = 1u << 12,
    club_through = 1u << 13, has_first_leg = 1u << 14, no_deciding_score = 1u << 15,
    final_round = 1u << 16,          // the first round code (rounded down to ten) is 150 and there is no second
    either_below_zero = 1u << 17,    // one of the two tie-break bytes is negative
    mine_less = 1u << 18, mine_greater = 1u << 19,  // the club's tie-break byte against the other's (signed)
};
}
enum class Operand : std::uint8_t {
    my_gender, other_gender, first_leg_gender, my_name, other_name, my_score, other_score, first_leg_text,
    subject_gender, subject_text,
};
// A shape's arguments: r5..r10, then `stack_count` words from sp+0x38.
struct ShapeOperands { Operand registers[6]; Operand stack[8]; unsigned stack_count; };
enum class Shape : std::uint8_t {
    score_and_text, swapped_score_and_text, text_only, first_leg_score, first_leg_swapped, first_leg_only, name_and_score,
};
// What a sentence is about: the competition's name (what 0x104769d0 built into sp+0x200), the round's text (sp+0x70)
// or the competition record's own long name.
enum class Source : std::uint8_t { none, competition, round, record };
// A second call after the sentence: the score text, or a copy of the record's name over the competition's text.
enum class Trailer : std::uint8_t { none, score_text, competition_name };
struct Leaf {
    std::uint32_t when, unless;         // facts that must hold, facts that must not
    std::uint16_t key;                  // the sentence's offset in the pool
    std::uint32_t site;                 // the address after the sentence's `bl`
    Shape shape;
    Source gender, text;                // where the subject's gender and text come from
    Trailer trailer;
    std::uint32_t trailer_site;
};
#include "comp_util_headline_tables.inc"

const Leaf& sentence(std::uint32_t facts) {
    for (const auto& leaf : leaves)
        if ((facts & leaf.when) == leaf.when && (facts & leaf.unless) == 0) return leaf;
    throw std::logic_error("no sentence matches the result headline's facts");
}

// The non-volatile registers the original keeps its locals in; every call sets them as they are at that point.
struct Locals {
    std::uint32_t found = 0;          // r18: a special competition's name was built
    std::uint32_t theirs = 0, mine = 0;  // r19, r20: the tie-break bytes (raw), the club's and the other's
    std::uint32_t gender = 0;         // r21: the competition's gender (raw, as 0x10476ec0 answered)
    std::uint32_t decider = 0;        // r22: the first of the two bytes that is nonzero
    std::uint32_t side = 0;           // r23: the caller's side value, then 1 for through and 0 for out
    std::uint32_t other = 0, me = 0;  // r24, r25: the other club's row and this club's
    std::uint32_t score_mine = 0, score_other = 0;  // r26, r27: the scores, as signed bytes
    std::uint32_t code_30 = 0;        // r28: the first round code rounded down to ten
};
}

void result_headline(Cpu& c, std::uint32_t club, std::uint32_t fixture, std::uint32_t grade_register, std::uint32_t side_register,
                     std::uint32_t round_register, std::uint32_t item) {
    Frame frame(c, 0x10464d00, frame_size, {.first_gpr = 13});
    const auto pool = c.toc(pool_slot), buffer = c.toc(text_buffer_slot), records_cell = c.toc(records_slot),
               special_id_cell = c.toc(special_id_slot), other_division_cell = c.toc(other_cup_division_slot);
    Locals held{.gender = c.r(21), .side = side_register, .code_30 = c.r(28)};
    // The registers at a call: r13..r31 as the original holds them there.
    const auto sync = [&] {
        c.set({{13, other_division_cell}, {14, records_cell}, {15, fixture}, {16, grade_register}, {17, round_register}, {18, held.found},
               {19, held.theirs}, {20, held.mine}, {21, held.gender}, {22, held.decider}, {23, held.side}, {24, held.other},
               {25, held.me}, {26, held.score_mine}, {27, held.score_other}, {28, held.code_30}, {29, buffer},
               {30, special_id_cell}, {31, pool}});
    };
    const auto call = [&](std::uint32_t target, std::uint32_t back) { sync(); c.call(target, back); };
    const auto text = [&](std::uint32_t slot) { return frame.at(std::int32_t(slot)); };

    c.put32(frame.caller() + item_slot, item);
    // The round's text starts as the 100 bytes of a table of the game's (the empty text, whatever else it holds).
    const auto template_text = c.toc(empty_buffer_slot);
    for (std::uint32_t at = 0; at < 96; at += 4) c.put32(text(round_slot) + at, c.u32(template_text + at));
    c.put16(text(round_slot) + 96, c.u16(template_text + 96));
    c.put16(text(round_slot) + 98, c.u16(template_text + 98));

    // The two clubs and their scores. The club is the fixture's first (its word +0xc) or its second; the scores are the
    // last of three pairs that is set (-1 for none): +0x4b/+0x4c, then +0x47/+0x48, then +0x43/+0x44.
    held.me = c.u32(c.toc(clubs_slot)) + club * comp::club::size;
    const bool first = club == c.u32(fixture + fixture::first_club_id);
    const auto tiebreak_first = c.u8(fixture + fixture::tiebreak_first), tiebreak_second = c.u8(fixture + fixture::tiebreak_second);
    held.decider = signed_byte(tiebreak_first) != 0 ? tiebreak_first : tiebreak_second;
    held.mine = first ? tiebreak_first : tiebreak_second;
    held.theirs = first ? tiebreak_second : tiebreak_first;
    held.other = c.u32(fixture + (first ? fixture::second_club : fixture::first_club));
    {
        std::uint32_t a = c.u8(fixture + fixture::goals_4b), b = c.u8(fixture + fixture::goals_4c);
        if (signed_byte(a) == 0xffffffffu) {
            a = c.u8(fixture + fixture::goals_47);
            b = c.u8(fixture + fixture::goals_48);
            if (signed_byte(a) == 0xffffffffu) {
                a = c.u8(fixture + fixture::goals_43);
                b = c.u8(fixture + fixture::goals_44);
            }
        }
        held.score_mine = signed_byte(first ? a : b);
        held.score_other = signed_byte(first ? b : a);
    }
    // The score text ("2-1") into the local the news item's score row reads. r7..r10 are what the original has there:
    // the caller's r7 and r8 and the two values it loaded for the copy and the club offset.
    c.set({{3, text(score_slot)}, {4, pool + score_format}, {5, held.score_mine}, {6, held.score_other},
           {9, c.toc(empty_buffer_slot)}, {10, club * comp::club::size}});
    call(format_entry, 0x10464e64);

    // The club goes through for any side but 2, 4 and -1.
    const auto side = std::int32_t(signed_byte(side_register));
    held.side = side == 2 || side == 4 || side == -1 ? 0 : 1;
    // The first round code (a halfword) rounded down to ten when it is below 1000, as the compiler inlined 0x104648c0.
    {
        const auto code = std::int32_t(signed_half(c.u16(fixture + fixture::round_code_30)));
        held.code_30 = signed_half(std::uint32_t(code < 1000 ? code - code % 10 : code));
    }

    // The competition's name text and gender: of the competition record the fixture names, unless the fixture is of
    // the two special competitions and one of its clubs is outside the division their cells name, when the special
    // competition's own record gives them.
    const auto record = c.u32(fixture + fixture::record);
    const auto fixture_id = c.u32(fixture + fixture::competition_id);
    // The division cell is read only once a club has a league and a division, as the original does.
    const auto outside = [&](std::uint32_t club_row, std::uint32_t division_cell) {
        const auto league = c.u32(club_row + club_league);
        if (league == 0) return false;
        const auto division = c.u32(league + league_division);
        return division != 0 && c.u32(division + division_id) != c.u32(division_cell);
    };
    const auto either_outside = [&](std::uint32_t division_cell) {
        return outside(c.u32(fixture + fixture::first_club), division_cell) || outside(c.u32(fixture + fixture::second_club), division_cell);
    };
    bool special = false, other_cup = false;
    if (fixture_id == c.u32(c.toc(cup_id_slot))) special = either_outside(c.toc(cup_division_slot));
    else if (fixture_id == c.u32(c.toc(other_cup_id_slot))) special = other_cup = either_outside(other_division_cell);
    if (special) {
        const auto special_record = c.u32(records_cell) + c.u32(special_id_cell) * comp::record::size;
        c.set({{3, special_record}, {4, 0}, {5, 0xa0}, {6, 1}, {7, text(competition_slot)}, {8, competition_text_size}});
        sync();
        c.link(other_cup ? 0x1046501c : 0x10464f60);  // each cup has its own pair of calls
        competition_name_text(c, special_record, 0, 0xa0, 1, text(competition_slot), competition_text_size);
        c.set({{3, special_record}, {4, 0}, {5, 0xa0}, {6, 1}});
        sync();
        c.link(other_cup ? 0x1046503c : 0x10464f80);
        competition_gender(c, special_record, 0, 0xa0, 1);
        held.found = 1;
        held.gender = c.r(3);
    }
    if (!special) {
        const auto code_32 = signed_half(c.u16(fixture + fixture::round_code_32));
        c.set({{3, record}, {4, code_32}, {5, held.code_30}, {6, 1}, {7, text(competition_slot)}, {8, competition_text_size}});
        sync();
        c.link(0x10465068);
        competition_name_text(c, record, code_32, held.code_30, 1, text(competition_slot), competition_text_size);
        c.set({{3, record}, {4, code_32}, {5, signed_half(c.u16(fixture + fixture::round_code_30))}, {6, 1}});
        sync();
        c.link(0x1046507c);
        competition_gender(c, record, code_32, signed_half(c.u16(fixture + fixture::round_code_30)), 1);
        held.gender = c.r(3);
    }

    // The first leg's name: its record's name, with the club's (the record's word +0x10 names one) when there is one,
    // in the format of the language's group (languages 0 and 1, then the others).
    const auto first_leg = c.u32(fixture + fixture::first_leg);
    if (first_leg != 0) {
        call(language_entry, 0x10465090);
        bool group_one = signed_byte(c.r(3)) == 0;
        if (!group_one) {
            call(language_entry, 0x1046509c);
            group_one = signed_byte(c.r(3)) == 1;
        }
        const auto club_record = c.u32(first_leg + first_leg_record_club);
        using Backs = std::pair<std::uint32_t, std::uint32_t>;  // after the sprintf, after the plain copy
        const auto sprintf_back = group_one ? Backs{0x104650cc, 0x104650dc} : Backs{0x10465104, 0x10465114};
        c.set({{5, first_leg}, {6, club_record}});
        if (club_record != 0) {
            c.set({{3, text(first_leg_slot)}, {4, pool + first_leg_formats[group_one ? 0 : 1]}, {5, first_leg + first_leg_record_name},
                   {6, club_record + club_name_in_record}});
            call(sprintf_entry, sprintf_back.first);
        } else {
            c.set({{3, text(first_leg_slot)}, {4, first_leg + first_leg_record_name}});
            call(copy_entry, sprintf_back.second);
        }
    }

    // A round above 0 is named (the text, and its gender for the sentence).
    const bool round = std::int16_t(round_register) > 0;
    std::uint32_t round_gender_value = 0;
    if (round) {
        c.set({{3, text(round_slot)}, {4, round_text_size}, {5, round_register}, {6, 0}});
        sync();
        c.link(0x10465130);
        round_text(c, text(round_slot), round_text_size, round_register, 0);
        c.set({{3, round_register}, {4, 0}});
        sync();
        c.link(0x1046513c);
        round_gender(c, round_register, 0);
        round_gender_value = c.r(3);
    }

    // The facts, and the sentence they pick.
    const auto mine = std::int32_t(signed_byte(held.mine)), theirs = std::int32_t(signed_byte(held.theirs));
    const auto decider = std::int32_t(signed_byte(held.decider)), grade = std::int32_t(signed_byte(grade_register));
    std::uint32_t facts = 0;
    const auto when = [&](bool condition, std::uint32_t fact) { if (condition) facts |= fact; };
    when(std::int16_t(round_register) > 0, Fact::result_positive);
    when(std::int16_t(round_register) == -2, Fact::result_minus_two);
    when(decider == 3, Fact::decider_aggregate);
    when(decider == 4, Fact::decider_penalties);
    when(decider == 5, Fact::decider_shootout);
    when(decider == 6, Fact::decider_away_goals);
    when(decider == 7, Fact::decider_golden_goal);
    when(decider == 8, Fact::decider_seeding);
    when(grade == 2, Fact::grade_excellent);
    when(grade == 3, Fact::grade_fine);
    when(grade == 5, Fact::grade_disappointing);
    when(grade == 6, Fact::grade_shock);
    when(grade == 7, Fact::grade_very_disappointing);
    when(held.side != 0, Fact::club_through);
    when(first_leg != 0, Fact::has_first_leg);
    when(signed_byte(c.u8(fixture + fixture::goals_4b)) == 0xffffffffu, Fact::no_deciding_score);
    when(std::int32_t(held.code_30) == 150 && signed_half(c.u16(fixture + fixture::round_code_32)) == 0, Fact::final_round);
    when(mine < 0 || theirs < 0, Fact::either_below_zero);
    when(mine < theirs, Fact::mine_less);
    when(mine > theirs, Fact::mine_greater);
    const auto& leaf = sentence(facts);

    // The sentence: its operands in r5..r10 and on the stack, from the shape.
    const auto value = [&](Operand operand) -> std::uint32_t {
        switch (operand) {
        case Operand::my_gender: return signed_byte(c.u8(held.me + comp::club::name_gender));
        case Operand::other_gender: return signed_byte(c.u8(held.other + comp::club::name_gender));
        case Operand::first_leg_gender: return signed_byte(c.u8(first_leg + first_leg_record_gender));
        case Operand::my_name: return held.me + comp::club::name;
        case Operand::other_name: return held.other + comp::club::name;
        case Operand::my_score: return held.score_mine;
        case Operand::other_score: return held.score_other;
        case Operand::first_leg_text: return text(first_leg_slot);
        case Operand::subject_gender:
            return leaf.gender == Source::competition ? signed_byte(held.gender)
                 : leaf.gender == Source::round ? signed_byte(round_gender_value)
                 : signed_byte(c.u8(record + comp::record::name_gender));
        case Operand::subject_text:
            return leaf.text == Source::competition ? text(competition_slot) : leaf.text == Source::round ? text(round_slot)
                 : record + comp::record::name;
        }
        return 0;
    };
    const auto& shape = shapes[std::size_t(leaf.shape)];
    for (unsigned n = 0; n < shape.stack_count; ++n) c.put32(frame.at(std::int32_t(argument_words + 4 * n)), value(shape.stack[n]));
    c.set({{3, buffer}, {4, pool + leaf.key}});
    for (unsigned n = 0; n < 6; ++n) c.set(5 + n, value(shape.registers[n]));
    call(format_entry, leaf.site);
    if (leaf.trailer == Trailer::score_text) {
        c.set({{3, text(score_slot)}, {4, pool + score_format}, {5, held.score_other}, {6, held.score_mine}});
        call(format_entry, leaf.trailer_site);
    } else if (leaf.trailer == Trailer::competition_name) {
        c.set({{3, text(competition_slot)}, {4, record + comp::record::name}});
        call(copy_entry, leaf.trailer_site);
    }

    // The news item: the headline, the two clubs and, when the statistics know the fixture, its score; then the
    // competition's name and the round's text for a friendly.
    const auto append = [&](std::uint32_t name, std::uint32_t kind, std::uint32_t reference, std::uint32_t back) {
        c.set({{3, item}, {4, name}, {5, kind}, {6, reference}});
        call(append_entry, back);
    };
    c.set({{3, item}, {4, buffer}});
    call(begin_item_entry, 0x10469624);
    append(held.me + comp::club::name, kind_club, held.me, 0x10469638);
    append(held.other + comp::club::name, kind_club, held.other, 0x1046964c);
    const auto stats_object = c.u32(c.toc(stats_slot));
    c.set({{3, stats_object}, {4, c.u32(fixture + fixture::stats_leg)}, {5, c.u32(fixture + fixture::first_club_id)},
           {6, c.u32(fixture + fixture::second_club_id)}, {7, c.u16(fixture + fixture::date_b)}, {8, c.u16(fixture + fixture::date_a)}});
    call(stats_entry, 0x1046966c);
    if (signed_byte(c.r(3)) != 0) append(text(score_slot), kind_score, c.u32(fixture + fixture::stats_leg), 0x10469688);
    c.set({{3, text(date_slot)}, {4, signed_half(c.u16(fixture + fixture::date_b))}, {5, c.u16(fixture + fixture::date_a)}});
    call(date_entry, 0x1046969c);
    c.set({{5, c.r(3)}, {4, record}, {3, c.u32(c.toc(friendly_slot))}, {6, c.toc(friendly_slot)}});
    call(friendly_entry, 0x104696b0);
    if (signed_byte(c.r(3)) != 0) {
        append(text(competition_slot), kind_competition, record, 0x104696cc);
        c.set(3, text(round_slot));
        call(length_entry, 0x104696d4);
        if (c.r(3) != 0) append(text(round_slot), kind_competition, record, 0x104696f0);
    }
}
}
