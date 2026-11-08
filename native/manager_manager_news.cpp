// MANAGER_MANAGER.CPP manager news text block (manager_manager_news.hpp): 0x101b9b50, the news item of a
// club's manager after a match, and 0x101bfd10, the comment it can end with.
//
// Both keep the original's stack image: the prologue stores, the text buffers the formatter fills, the
// formatter's extra argument words from 0x38 and the registers the original holds at each call (a callee's
// prologue saves r13-r31) and leaves at its end (r3-r10, cr0). The formatter is varargs: it spills r3-r10 into
// the frame, so every register the original leaves in r5-r10 at a call is set the same way here.
#include "manager_manager_news.hpp"
#include "guest_values.hpp"
#include "manager_manager_detail.hpp"
#include "manager_manager_news_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
using guest::Frame;
using guest::Kept;
using guest::compare;
using guest::compare_logical;
using namespace news_detail;
namespace {
std::int32_t sbyte(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
std::uint32_t word(std::int32_t value) { return std::uint32_t(value); }

// The first formatting of an event: how the news text's arguments are laid out, by the event's rule.
// Every layout passes the club's text buffer (0xa4) and two names; they differ in what comes first.
enum class Layout {
    opponent_first,     // r5/r6 the opponent's class, r7/r8 the competition's, stack: opponent name, competition name
    competition_first,  // the same with the competition first
    short_form,         // the competition name on the stack, the opponent's name in r10
};
// An event code (or a pair of them, the base and its variant, differing by 0x100 or 0x200) and its text.
// `tested`: the competition is first asked whether it is a cup (0x10494d20), and a cup takes the shorter
// form with `cup_format`; otherwise the layout alone prints `format`. Formats are offsets in the unit's string
// pool (TOC -0x5368); `*_back` are the addresses after the original's `bl`s.
struct Rule {
    std::uint32_t first, second;
    bool tested;
    Layout layout;
    std::uint32_t cup_format, format;
    std::uint32_t test_back, cup_back, back;
    bool matches(std::uint32_t code) const { return code == first || (second != 0 && code == second); }
};
constexpr Rule rules[] = {
    {0x401, 0x501, true, Layout::short_form, 0x1bab8, 0x1c019, 0x101bab50, 0x101bab80, 0x101babbc},
    {0x402, 0x502, true, Layout::short_form, 0x1bc08, 0x1bcb1, 0x101ba9dc, 0x101baa0c, 0x101baa48},
    {0x408, 0x608, true, Layout::short_form, 0x1b126, 0x1b1c4, 0x101ba568, 0x101ba598, 0x101ba5d4},
    {0x420, 0x620, true, Layout::short_form, 0x1b3bc, 0x1b457, 0x101ba6dc, 0x101ba70c, 0x101ba748},
    {0x441, 0x541, true, Layout::short_form, 0x1bd63, 0x1be09, 0x101baa58, 0x101baa88, 0x101baac4},
    {0x442, 0x542, true, Layout::opponent_first, 0x1b93e, 0x1b9df, 0x101ba8dc, 0x101ba90c, 0x101ba950},
    {0x444, 0x544, true, Layout::opponent_first, 0x1b64c, 0x1b6f4, 0x101ba7d4, 0x101ba804, 0x101ba848},
    {0x448, 0x648, true, Layout::short_form, 0x1aea7, 0x1af44, 0x101ba470, 0x101ba4a0, 0x101ba4dc},
    {0x450, 0x650, true, Layout::short_form, 0x1b4f5, 0x1b599, 0x101ba758, 0x101ba788, 0x101ba7c4},
    {0x460, 0x660, true, Layout::short_form, 0x1b265, 0x1b30c, 0x101ba5e4, 0x101ba614, 0x101ba650},
    {0x481, 0x581, true, Layout::short_form, 0x1bec4, 0x1bf6d, 0x101baad4, 0x101bab04, 0x101bab40},
    {0x482, 0x582, true, Layout::short_form, 0x1bab8, 0x1bb58, 0x101ba960, 0x101ba990, 0x101ba9cc},
    {0x484, 0x584, true, Layout::opponent_first, 0x1b7c8, 0x1b86f, 0x101ba858, 0x101ba888, 0x101ba8cc},
    {0x488, 0x688, true, Layout::short_form, 0x1aff1, 0x1b086, 0x101ba4ec, 0x101ba51c, 0x101ba558},
    {0x4a0, 0x6a0, true, Layout::short_form, 0x1b126, 0x1b1c4, 0x101ba660, 0x101ba690, 0x101ba6cc},
    {0x808, 0xa08, false, Layout::competition_first, 0x0, 0x1a782, 0x0, 0x0, 0x101ba1c8},
    {0x810, 0xa10, false, Layout::competition_first, 0x0, 0x1ae00, 0x0, 0x0, 0x101ba460},
    {0x820, 0xa20, false, Layout::competition_first, 0x0, 0x1aadb, 0x0, 0x0, 0x101ba314},
    {0x848, 0xa48, true, Layout::opponent_first, 0x1a2fe, 0x1a39c, 0x101ba004, 0x101ba034, 0x101ba078},
    {0x850, 0xa50, true, Layout::opponent_first, 0x1ab8f, 0x1ac14, 0x101ba324, 0x101ba354, 0x101ba398},
    {0x860, 0xa60, true, Layout::opponent_first, 0x1a843, 0x1a8d4, 0x101ba1d8, 0x101ba208, 0x101ba24c},
    {0x888, 0xa88, true, Layout::opponent_first, 0x1a549, 0x1a5e1, 0x101ba0cc, 0x101ba0fc, 0x101ba140},
    {0x890, 0xa90, true, Layout::opponent_first, 0x1acc9, 0x1ad51, 0x101ba3a8, 0x101ba3d8, 0x101ba41c},
    {0x8a0, 0xaa0, true, Layout::opponent_first, 0x1a995, 0x1aa20, 0x101ba25c, 0x101ba28c, 0x101ba2d0},
    {0x948, 0x0, false, Layout::opponent_first, 0x0, 0x1a46a, 0x0, 0x0, 0x101ba0bc},
    {0x988, 0x0, false, Layout::opponent_first, 0x0, 0x1a6a9, 0x0, 0x0, 0x101ba184},
    {0x1001, 0x1101, true, Layout::opponent_first, 0x1d75e, 0x1d7fd, 0x101bb3a8, 0x101bb3d8, 0x101bb41c},
    {0x1002, 0x1102, false, Layout::competition_first, 0x0, 0x1d3a5, 0x0, 0x0, 0x101bb290},
    {0x1008, 0x0, true, Layout::opponent_first, 0x1c548, 0x1c5fa, 0x101bad4c, 0x101bad7c, 0x101badc0},
    {0x1020, 0x1120, true, Layout::opponent_first, 0x1cbad, 0x1cc57, 0x101baf8c, 0x101bafbc, 0x101bb000},
    {0x1041, 0x1141, true, Layout::opponent_first, 0x1d468, 0x1d51e, 0x101bb2a0, 0x101bb2d0, 0x101bb314},
    {0x1042, 0x1142, true, Layout::opponent_first, 0x1d08f, 0x1d152, 0x101bb154, 0x101bb184, 0x101bb1c8},
    {0x1044, 0x1144, true, Layout::opponent_first, 0x1cdbc, 0x1ce5d, 0x101bb04c, 0x101bb07c, 0x101bb0c0},
    {0x1048, 0x1148, true, Layout::opponent_first, 0x1c0c2, 0x1c15f, 0x101babcc, 0x101babfc, 0x101bac40},
    {0x1060, 0x1160, true, Layout::opponent_first, 0x1c763, 0x1c815, 0x101bae0c, 0x101bae3c, 0x101bae80},
    {0x1081, 0x1181, true, Layout::opponent_first, 0x1d5ed, 0x1d69b, 0x101bb324, 0x101bb354, 0x101bb398},
    {0x1082, 0x1182, true, Layout::opponent_first, 0x1d221, 0x1d2de, 0x101bb1d8, 0x101bb208, 0x101bb24c},
    {0x1084, 0x1184, true, Layout::opponent_first, 0x1cf27, 0x1cfcd, 0x101bb0d0, 0x101bb100, 0x101bb144},
    {0x1088, 0x1188, true, Layout::opponent_first, 0x1c309, 0x1c3ad, 0x101bac8c, 0x101bacbc, 0x101bad00},
    {0x10a0, 0x11a0, true, Layout::opponent_first, 0x1c98f, 0x1ca2e, 0x101baecc, 0x101baefc, 0x101baf40},
    {0x1208, 0x0, false, Layout::short_form, 0x0, 0x1c6c3, 0x0, 0x0, 0x101badfc},
    {0x1220, 0x0, false, Layout::short_form, 0x0, 0x1cd1e, 0x0, 0x0, 0x101bb03c},
    {0x1248, 0x0, false, Layout::short_form, 0x0, 0x1c236, 0x0, 0x0, 0x101bac7c},
    {0x1260, 0x0, false, Layout::short_form, 0x0, 0x1c8e7, 0x0, 0x0, 0x101baebc},
    {0x1288, 0x0, false, Layout::short_form, 0x0, 0x1c482, 0x0, 0x0, 0x101bad3c},
    {0x12a0, 0x0, false, Layout::short_form, 0x0, 0x1cafe, 0x0, 0x0, 0x101baf7c},
};
// The default: no listed event code. The code's own bits pick the text (0x800, 0x400 or neither).
struct Fallback {
    std::uint32_t format, back;
};
constexpr Fallback fallbacks[] = {{0x1d8b8, 0x101bb468}, {0x1d97b, 0x101bb4b4}, {0x1da3c, 0x101bb4f8}};

// The board's row flags that pick the comment when the lookup finds none, in the order the original tests them,
// and the text of none of them.
struct RowFlag {
    std::uint32_t mask, format, back;
};
constexpr RowFlag row_flags[] = {{0x1, 0x1def4, 0x101bb7a8}, {0x2, 0x1df57, 0x101bb7c4}, {0x4, 0x1dfb0, 0x101bb7e0},
                                 {0x8, 0x1e006, 0x101bb7fc}, {0x40, 0x1e05e, 0x101bb818}, {0x20, 0x1e0bb, 0x101bb834}};
constexpr RowFlag no_row_flag = {0, 0x1e102, 0x101bb848};

class Compose {
public:
    Compose(Cpu& c, Frame& frame, std::uint32_t object, std::uint32_t club, std::uint32_t opponent,
            std::uint32_t competition, std::uint32_t code, std::uint32_t phrase)
        : c(c), frame(frame), object(object), club(club), opponent(opponent), competition(competition), code(code),
          phrase(phrase), pool(c.toc(toc::pool)), news_line(c.toc(toc::news_line)),
          leagues(c.toc(toc::leagues)), r16(c, c.toc(toc::cups)),
          r19(c, c.u8(frame.caller() + argument::limit)) {
        const auto caller = frame.caller();
        people = c.u32(caller + argument::people);
        mode = c.u32(caller + argument::mode);
        kind = c.u8(caller + argument::kind);
        expected = c.u8(caller + argument::expected);
        actual = c.u8(caller + argument::actual);
        commented = c.u8(caller + argument::commented);
        guest::keep(c, {{15, leagues}, {17, mode}, {18, people}, {20, commented}, {21, news_line}, {22, actual},
                        {23, expected}, {24, kind}, {25, object}, {26, club}, {27, pool}, {28, phrase},
                        {29, code}, {30, opponent}, {31, competition}});
    }

    std::uint32_t run() {
        compare(c, std::int32_t(competition));  // or. r31,r6,r6
        if (competition == 0) {
            // Only the opponent: its text goes straight to the news line, and nothing is posted.
            const auto name_class = opponent_class();
            c.set({{5, name_class}, {6, name_class}, {7, frame.at(slot::club_text)}, {8, opponent + club::name}});
            format(0x101bb524, news_line, 0x1db03);
            return guest::result(c, 0);
        }
        club_text();
        const Rule* rule = nullptr;
        for (const auto& candidate : rules)
            if (candidate.matches(code)) { rule = &candidate; break; }
        if (!rule) return fallback();
        if (rule->tested) {
            c.set({{3, c.u32(r16)}, {4, competition}});
            c.call(callee::cup_flag, rule->test_back);
            const auto cup = sbyte(c.r(3));
            compare(c, cup);
            if (cup != 0) {
                const auto name_class = opponent_class();
                c.set({{5, name_class}, {6, name_class}, {7, frame.at(slot::club_text)}, {8, phrase},
                       {9, opponent + club::name}});
                format(rule->cup_back, frame.at(slot::news_text), rule->cup_format);
                return finish();
            }
        } else {
            // The dispatch's last compare is the equal one against the code itself (checked for every
            // value up to 0x1400 and beyond).
            compare(c, std::int32_t(code), std::int32_t(code));
        }
        layout(rule->layout, frame.at(slot::news_text), rule->format, rule->back);
        return finish();
    }

private:
    std::uint32_t opponent_class() { return word(sbyte(c.u8(opponent + club::name_class))); }
    std::uint32_t competition_class() { return word(sbyte(c.u8(competition + competition::name_class))); }

    // `sprintf(destination, pool + offset, r5...)` with the registers and stack words set by the caller.
    void format(std::uint32_t back, std::uint32_t destination, std::uint32_t offset) {
        c.set({{3, destination}, {4, pool + offset}});
        c.call(callee::formatter, back);
    }
    void format_comment(std::uint32_t offset, std::uint32_t back) { format(back, frame.at(slot::comment), offset); }

    // 0x101b9ba8: the club's own text into the club buffer: a club the flag test (0x100c7d00) singles out
    // gives its nation's text (0x10168a00), any other its name.
    void club_text() {
        c.set(3, club);
        c.call(callee::club_flag, 0x101b9bb0);
        const auto flagged = sbyte(c.r(3));
        compare(c, flagged);
        if (flagged != 0) {
            c.set({{3, c.u32(club + club::nation)}, {4, frame.at(slot::nation)}, {5, 0x64}, {6, 9}});
            c.call(callee::nation_name, 0x101b9bcc);
            c.set({{5, 0x64}, {6, 0x64}, {7, frame.at(slot::nation)}});
            format(0x101b9be8, frame.at(slot::club_text), 0x1a28c);
        } else {
            const auto name_class = word(sbyte(c.u8(club + club::name_class)));
            c.set({{5, name_class}, {6, name_class}, {7, club + club::name}});
            format(0x101b9c0c, frame.at(slot::club_text), 0x1a2c6);
        }
    }

    // The text of an event whose rule prints it by layout (the cup form aside).
    void layout(Layout form, std::uint32_t destination, std::uint32_t offset, std::uint32_t back) {
        const auto opponent_name = opponent + club::name, competition_name = competition + competition::name;
        const auto opponent_text = opponent_class(), competition_text = competition_class();
        const auto buffer = frame.at(slot::club_text);
        if (form == Layout::opponent_first) {
            c.put32(frame.at(slot::arguments), opponent_name);
            c.put32(frame.at(slot::arguments + 4), competition_name);
            c.set({{5, opponent_text}, {6, opponent_text}, {7, competition_text}, {8, competition_text}, {9, buffer},
                   {10, phrase}});
        } else if (form == Layout::competition_first) {
            c.put32(frame.at(slot::arguments), competition_name);
            c.put32(frame.at(slot::arguments + 4), opponent_name);
            c.set({{5, competition_text}, {6, competition_text}, {7, opponent_text}, {8, opponent_text}, {9, buffer},
                   {10, phrase}});
        } else {  // short_form
            c.put32(frame.at(slot::arguments), competition_name);
            c.set({{5, opponent_text}, {6, opponent_text}, {7, competition_text}, {8, competition_text}, {9, buffer},
                   {10, opponent_name}});
        }
        format(back, destination, offset);
    }

    // 0x101bb420: an event code without a rule. The text goes to the news line itself, the item is not
    // posted, and the answer is 0.
    std::uint32_t fallback() {
        // rlwinm. r0,r29,0,n,n: the code's bit 0x800, then 0x400 (the last choice has no test)
        const auto& choice = compare(c, std::int32_t(code & 0x800)) != 0   ? fallbacks[0]
                             : compare(c, std::int32_t(code & 0x400)) != 0 ? fallbacks[1]
                                                                           : fallbacks[2];
        layout(Layout::opponent_first, news_line, choice.format, choice.back);
        return guest::result(c, 0);
    }

    // 0x101bb52c on: the second paragraph (a comment on the result against the board's expectation), the
    // comment of the named people (0x101bfd10) when it is empty and the code asks for one, and the post.
    std::uint32_t finish() {
        write_comment();
        const auto text = frame.at(slot::comment);
        const bool empty = compare(c, sbyte(c.u8(text))) == 0;
        if (empty && compare(c, std::int32_t(code & 0x1800)) != 0) {
            c.set({{3, object}, {4, people}, {7, mode}, {5, (code >> 11) & 1}, {6, text}});
            c.link(0x101bb8a4);
            comment(c, people, (code >> 11) & 1, text, mode);
        }
        const bool present = compare(c, sbyte(c.u8(text))) != 0;
        c.set({{3, news_line}, {4, pool + (present ? 0xffb9u : 0xffc0u)}, {5, frame.at(slot::news_text)}});
        if (present) c.set(6, text);
        c.call(callee::post, present ? 0x101bb8c8 : 0x101bb8e0);
        return guest::result(c, 1);
    }

    void clear_comment() { c.put8(frame.at(slot::comment), 0); }

    // The comment on how the result compares with what was expected (the bytes `expected`, `actual`, `limit`),
    // by the kind: 2 is a result at or below the expectation, -2 one beyond it (in the club's league, asking the
    // league's method), anything else no comment.
    void write_comment() {
        if (compare(c, sbyte(commented)) == 0) return clear_comment();  // extsb. r0,r20
        compare(c, sbyte(kind), 2);
        if (sbyte(kind) == 2) {
            const auto base = sbyte(expected), score = sbyte(actual);
            compare(c, base, score);
            if (base >= score) {
                // score <= base, so the original's `score > base + 4` (0x101bb558) never holds, and neither
                // does `score > base` after `score <= base - 4` (0x101bb594, the chain at 0x101bb598).
                compare(c, score, base - 4);
                if (score > base - 4) return format_comment(0x1dbc9, 0x101bb58c);
                return clear_comment();
            }
        }
        compare(c, sbyte(kind), -2);
        if (sbyte(kind) != -2) return clear_comment();
        const auto base = sbyte(expected);
        if (compare(c, base, sbyte(r19)) < 0) return clear_comment();
        r16 = word(sbyte(actual));
        const auto score = sbyte(r16);
        compare(c, score, base + 4);
        if (score > base + 4) return format_comment(0x1de0d, 0x101bb6b0);
        compare(c, score, base - 4);
        if (score < base - 4 && league_comment(score)) return;
        compare(c, score, base + 2);
        if (score <= base + 2) return clear_comment();
        const auto flags = guest::extended(c.i16(row_address() + row::flags));
        for (const auto& flag : row_flags)
            if (compare(c, std::int32_t(flags & flag.mask)) != 0) return format_comment(flag.format, flag.back);
        format_comment(no_row_flag.format, no_row_flag.back);
    }

    std::uint32_t row_address() { return c.u32(object + object::table) + c.u32(club + club::id) * row::size; }
    // The record of the club's league in the leagues table (0 when the club has none).
    std::uint32_t league_record() {
        const auto link = c.u32(club + club::league);
        c.set(3, link);
        if (compare_logical(c, link) == 0) return 0;
        const auto id = c.u32(link + league::id);
        const auto table = c.u32(leagues);
        c.set(3, table);
        const auto record = c.u32(table + (id << 2));
        c.set(3, record);
        return compare_logical(c, record) == 0 ? 0 : record;
    }
    // 0x101bb6c0..0x101bb76c: the club's league has a margin of its own; when the row asks (the flag) and the
    // league's method agrees, the result must also be beyond the league's level less the margin. Returns
    // whether a comment was written (otherwise the original continues at 0x101bb770).
    bool league_comment(std::int32_t score) {
        const auto record = league_record();
        if (record == 0) return false;
        c.set(5, guest::extended(c.i16(record + league::level)));
        const auto row = row_address();
        c.set(4, row);
        r19 = word(sbyte(c.r(5)));
        const auto flags = guest::extended(c.i16(row + row::flags));
        if (compare(c, std::int32_t(flags & league::flag_asks)) != 0) {
            c.set({{12, c.u32(c.u32(record) + league::ask)}, {3, record}});
            c.call(callee::virtual_glue, 0x101bb714);
            c.set(2, c.u32(frame.at(slot::toc_save)));
            const auto agrees = sbyte(c.r(3));
            compare(c, agrees);
            if (agrees != 0) {
                const auto link = c.u32(club + club::league);
                const auto table = c.u32(leagues);
                const auto again = c.u32(table + (c.u32(link + league::id) << 2));
                c.set({{3, again}, {4, table}});
                const auto margin = sbyte(c.u8(again + league::margin));
                const auto limit = std::int32_t(r19) - margin;
                compare(c, score, limit);
                if (score > limit) {
                    format_comment(0x1de60, 0x101bb758);
                    return true;
                }
            }
        }
        format_comment(0x1dea3, 0x101bb76c);
        return true;
    }

    Cpu& c;
    Frame& frame;
    std::uint32_t object, club, opponent, competition, code, phrase;
    std::uint32_t pool, news_line, leagues;
    Kept<16> r16;  // the cups list holder, then the score of the comment
    Kept<19> r19;  // the limit byte, then the league's level
    std::uint32_t people = 0, mode = 0, kind = 0, expected = 0, actual = 0, commented = 0;
};

// 0x101bfd10: of the record's three person ids the first present (the ids are -1 when empty) becomes the
// person whose name is formatted. The original keeps the people it finds in r3/r30/r29 (named) or
// r28/r27/r26 and calls the name routine (0x100caeb0) with r3 or r28 alone, so a person found in a later slot
// (or none in the first) leaves r3 (the caller's, the module object) or r28 (the caller's) as the argument.
class Comment {
public:
    Comment(Cpu& c, std::uint32_t people, std::uint32_t named, std::uint32_t text,
            std::uint32_t mode, Frame& frame)
        : c(c), frame(frame), people(people), named(sbyte(named) != 0), text(text), mode(word(std::int32_t(mode))),
          second(c, 0), r26(c, c.r(26)), r27(c, c.r(27)), r28(c, c.r(28)), r29(c, c.r(29)), r30(c, c.r(30)) {
        c.set(8, c.toc(toc::people));
        guest::keep(c, {{31, c.toc(toc::pool)}, {23, text}, {24, mode}});
    }

    // The original switches on r0 (found, 0 or 1: `li r0,0` at entry, `li r0,1` where a person is found) and on
    // r25 (`second`, 0 or 1 likewise); its cases for 2 and 3 (0x101bfe2c..0x101bfec0, 0x101bff68..0x101bffc0,
    // which format three names) and the branches to them are dead.
    void run() {
        if (compare_logical(c, people) == 0) return;  // cmplwi r4,0
        const bool found = select();
        compare(c, found ? 1 : 0);  // the original's switch on r0: the last compare is against 0
        if (!found) c.put8(text, 0);
        else first_form();
        compare(c, std::int32_t(second), 0);
        if (std::uint32_t(second) == 0) c.put8(text, 0);  // flag unset: also empties what the first form wrote
        else second_form();
    }

private:
    std::uint32_t pool() { return c.r(31); }
    std::int32_t mode_value() { return std::int32_t(mode); }

    // The person at address `base + id * 0x70` lands in the register of its slot. Each slot's id is compared
    // with -1; the leftovers in r3-r5 are the original's. Answers whether the first form applies (r0 is set
    // with the flag only; without it r25 is).
    bool select() {
        c.set(5, word(sbyte(c.r(5))));  // extsb. r5,r5
        compare(c, sbyte(c.r(5)));
        const auto table = c.r(8);
        auto id = c.u32(people);
        c.set(5, id);
        compare(c, std::int32_t(id), -1);
        if (std::int32_t(id) != -1) {
            const auto offset = id * 0x70;
            if (named) {
                c.set({{3, offset}, {4, c.u32(table)}});
                c.set(3, c.r(4) + c.r(3));
            } else {
                c.set(4, offset);
                c.set(5, c.u32(table));
                second = 1;
                r28 = c.r(5) + c.r(4);
            }
            return named;
        }
        id = c.u32(people + 4);
        c.set(5, id);
        compare(c, std::int32_t(id), -1);
        if (std::int32_t(id) != -1) {
            c.set(4, id * 0x70);
            c.set(5, c.u32(table));
            if (!named) second = 1;
            if (named) r30 = c.r(5) + c.r(4);
            else r27 = c.r(5) + c.r(4);
            return named;
        }
        id = c.u32(people + 8);
        c.set(4, id);
        compare(c, std::int32_t(id), -1);
        if (std::int32_t(id) == -1) return false;
        c.set(4, id * 0x70);
        c.set(5, c.u32(table));
        if (!named) second = 1;
        if (named) r29 = c.r(5) + c.r(4);
        else r26 = c.r(5) + c.r(4);
        return named;
    }

    // The name goes to a buffer, then the text by the mode: 0, 1 or any other.
    void first_form() {
        c.set({{4, frame.at(comment_slot::name)}, {5, 0xc8}, {6, 1}});
        c.call(callee::person_name, 0x101bfed4);
        compare(c, mode_value(), 1);
        std::uint32_t format, back;
        if (mode_value() == 1) { format = 0x20893; back = 0x101bff1c; }
        else if (mode_value() > 1) { format = 0x208de; back = 0x101bff34; }
        else {
            compare(c, mode_value(), 0);
            if (mode_value() == 0) { format = 0x2083f; back = 0x101bff04; }
            else { format = 0x208de; back = 0x101bff34; }
        }
        c.set({{3, text}, {4, pool() + format}, {5, frame.at(comment_slot::name)}});
        c.call(callee::formatter, back);
    }
    void second_form() {
        c.set({{3, c.r(28)}, {4, frame.at(comment_slot::name)}, {5, 0xc8}, {6, 1}});
        c.call(callee::person_name, 0x101c001c);
        compare(c, mode_value(), 2);
        std::uint32_t format, back;
        if (mode_value() == 2) { format = 0x20b2b; back = 0x101c008c; }
        else if (mode_value() > 2) { format = 0x20b90; back = 0x101c00a8; }
        else {
            compare(c, mode_value(), 0);
            if (mode_value() == 0) { format = 0x20a3e; back = 0x101c0054; }
            else if (mode_value() > 0) { format = 0x20ab9; back = 0x101c0070; }
            else { format = 0x20b90; back = 0x101c00a8; }
        }
        c.set({{3, text}, {4, pool() + format}, {5, 0x64}, {6, frame.at(comment_slot::name)}});
        c.call(callee::formatter, back);
    }

    Cpu& c;
    Frame& frame;
    std::uint32_t people;
    bool named;
    std::uint32_t text, mode;
    Kept<25> second;  // 1 once a person is found without the flag
    Kept<26> r26;
    Kept<27> r27;
    Kept<28> r28;
    Kept<29> r29;
    Kept<30> r30;
};
}  // namespace

// 0x101b9b50. Frame 0xd70 (r15-r31 saved with stmw, LR at +8).
std::uint32_t compose(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t opponent, std::uint32_t competition,
                      std::uint32_t code, std::uint32_t phrase) {
    c.set(0, c.lr());
    Frame frame(c, 0x101b9b50, 0xd70, {.first_gpr = 15});
    const auto answer = Compose(c, frame, object, club, opponent, competition, code, phrase).run();
    lmw_leaves(c, frame.caller());
    return answer;
}

// 0x101bfd10. Frame 0x2c0 (r23-r31 saved with stmw, LR at +8).
void comment(Cpu& c, std::uint32_t people, std::uint32_t named, std::uint32_t text, std::uint32_t mode) {
    c.set(0, c.lr());
    Frame frame(c, 0x101bfd10, 0x2c0, {.first_gpr = 23});
    Comment(c, people, named, text, mode, frame).run();
    lmw_leaves(c, frame.caller());
}
}  // namespace cm::aot::manager_manager
