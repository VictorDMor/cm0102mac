// COMP.CPP, block `news`: a club's fate in a league (0x10450030), a stage's position (0x10450580) and a club's
// place (0x10451ac0). Each formats a message into the shared text buffer (0x10727890) and hands it to the news
// text object, with a link row per name (0x1074bab0). 0x1074bab0 copies the register save area of the previous
// callee into its row, so the non-volatile registers the original holds are set at every call (the Cpu's registers
// stand for them: a store here mirrors the original's assignment at the same point).
#include "comp_news.hpp"
#include "croatia_first.hpp"
#include "comp_texts_detail.hpp"

namespace cm::aot::comp {
using namespace texts;

namespace {
// One wording of a club's fate: the call sites and the pool offset of its format.
struct Wording {
    std::uint32_t format_at, set_at, club_link_at, competition_link_at, offset;
    std::uint32_t gender, name;  // the record's gender byte and name offsets (0 when the wording has no record name)
};
// Kind 0..3 (and 5), then the form: with the competition's name, or without.
constexpr Wording plain_kinds[4] = {{0x10450278, 0x10450284, 0x1045029c, 0, 0x134, 0, 0}, {0x10450358, 0x10450364, 0x1045037c, 0, 0x1de, 0, 0},
                                    {0x10450438, 0x10450444, 0x1045045c, 0, 0x28a, 0, 0}, {0x10450518, 0x10450524, 0x1045053c, 0, 0x331, 0, 0}};
constexpr Wording named_kinds[4] = {{0x10450210, 0x1045021c, 0x10450234, 0x10450248, 0xc5, 0x6a, 0x1a},
                                    {0x104502f0, 0x104502fc, 0x10450314, 0x10450328, 0x166, 0x6a, 0x1a},
                                    {0x104503d0, 0x104503dc, 0x104503f4, 0x10450408, 0x211, 0x6a, 0x1a},
                                    {0x104504b0, 0x104504bc, 0x104504d4, 0x104504e8, 0x2c1, 0x6a, 0x1a}};
}

void league_fate_text(Cpu& c, std::uint32_t competition, std::uint32_t club, std::uint32_t kind, std::uint32_t mode, std::uint32_t pass,
                      std::uint32_t stack_byte, std::uint32_t form, std::uint32_t output) {
    if (croatia_first::format_group_news(c, competition, kind)) return;
    const Frame frame(c, 0x10450030, 0x50, {.first_gpr = 27});
    const auto pool = c.toc(toc::pool), clubs_cell = c.toc(toc::clubs), buffer = c.toc(toc::text);
    // The prologue: r10 the pool, r29 the text buffer, r30 the competition, r28 the clubs cell, r6 a copy of the kind,
    // r7 the form byte, r11 the stack byte, r31 the output. r27 keeps the caller's value until a branch uses it.
    c.set({{10, pool}, {29, buffer}, {30, competition}, {28, clubs_cell}, {6, kind}, {7, form}, {11, stack_byte}, {31, output}});
    if (mode != 0) {
        if (sext8(form) == 0) {
            c.set({{3, club}, {4, mode}, {6, output}});
            c.call(fate_by_flag, 0x1045009c);
        } else {
            c.set({{3, club}, {4, mode}, {5, stack_byte}, {7, pass}, {8, output}});
            c.call(fate_by_stage, 0x10450088);
        }
        return;
    }
    const auto row_offset = club * 0x248;
    const auto kind_value = std::int32_t(sext8(kind));
    const auto record = [&] { return c.u32(competition + competition::record); };
    const auto clubs = [&] { return c.u32(clubs_cell); };
    const auto club_gender = [&] { return sext8(c.u8(clubs() + row_offset + club::name_gender)); };
    const auto club_name = [&] { return clubs() + row_offset + club::name; };
    // The wordings that give the club's gender twice and the club's name.
    const auto short_form = [&](const Wording& w) {
        c.set(27, row_offset);
        const auto gender = club_gender();
        c.set({{3, buffer}, {4, pool + w.offset}, {5, gender}, {6, gender}, {7, club_name()}});
        c.call(callee::format, w.format_at + 4);
        set_text(c, w.set_at, output, buffer);
        link(c, w.club_link_at, output, club_name(), club_tag, clubs() + row_offset);
    };
    // The same with the competition's gender and name besides.
    const auto long_form = [&](const Wording& w) {
        c.set(27, row_offset);
        const auto rec = record();
        const auto record_gender = sext8(c.u8(rec + w.gender));
        const auto gender = club_gender();
        format(c, w.format_at, {buffer, pool + w.offset, gender, gender, record_gender, record_gender, club_name(), rec + w.name});
        set_text(c, w.set_at, output, buffer);
        link(c, w.club_link_at, output, club_name(), club_tag, clubs() + row_offset);
    };
    if (kind_value == 5) {
        if (sext8(form) == 0) {
            long_form({0x10450184, 0x10450190, 0x104501a8, 0, 0x6d, 0x67, 0x4d});
            link(c, 0x104501bc, output, record() + 0x4d, competition_tag, record());
        } else {
            long_form({0x104500f0, 0x104500fc, 0x10450114, 0, 0, 0x6a, 0x1a});
            c.set({{3, c.u32(c.toc(toc::fixture_rules))}, {4, record()}});
            c.call(fixture_rules_query, 0x10450128);
            if (sext8(c.r(3)) == 0) link(c, 0x10450140, output, record() + 0x1a, competition_tag, record());
        }
    } else if (kind_value >= 0 && kind_value <= 3) {
        if (sext8(form) == 0) {
            short_form(plain_kinds[kind_value]);
        } else {
            const auto& w = named_kinds[kind_value];
            long_form(w);
            link(c, w.competition_link_at, output, record() + w.name, competition_tag, record());
        }
    } else {
        c.set({{3, buffer}, {4, pool + 0x35f}, {5, record() + 0x4d}});
        c.call(callee::sprintf, 0x10450558);
        set_text(c, 0x10450560, output, buffer);
    }
}

void position_text(Cpu& c, std::uint32_t text, std::uint32_t position, std::uint32_t code_word, std::uint32_t second_word,
                   std::uint32_t kind, std::uint32_t club_record) {
    const Frame frame(c, 0x10450580, 0x80, {.first_gpr = 24});
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message);
    // r24 the text, r25 the position, r26 the code word, r27 the second word, r28 the kind, r29 the club record,
    // r30 the message buffer (until the code is extended), r31 the pool.
    c.set({{24, text}, {25, position}, {26, code_word}, {27, second_word}, {28, kind}, {29, club_record}, {30, message}, {31, pool}});
    if (std::int32_t(position) < 0x1e) {
        report(c, frame, {0x104505cc, 0x40, 0x38, 0x1d5});
        return;
    }
    const auto code = sext16(code_word);
    c.set(30, code);
    const bool no_group = std::int32_t(invoke(c, stage_group, 0x1045061c, {code})) == -1;
    if (!no_group) {
        // kind 0, 6, 7 (the low byte of kind-6 at most 1) or 8: the club's byte +0x242 in the format; else the other text.
        if (sext8(kind) == 0 || std::uint8_t(kind - 6) <= 1 || sext8(kind) == 8) {
            c.set({{3, text}, {4, pool + 0x39c}, {5, sext8(c.u8(club_record + 0x242))}, {6, pool + 0x3c1}});
            c.call(callee::format, 0x10450660);
        } else {
            c.set({{3, text}, {4, pool + 0x3c2}});
            c.call(callee::format, 0x10450670);
        }
        return;
    }
    const auto kind_value = sext8(kind);
    c.set(3, kind_value);
    if (kind_value == 1) {
        c.set({{3, text}, {4, pool + 0x3ce}});
        c.call(callee::format, 0x1045068c);
        return;
    }
    if (sext16(code_word) == 0 && sext16(second_word) == 0) {
        if (kind_value == 2) {
            c.set({{3, text}, {4, pool + 0x3d6}});
            c.call(callee::format, 0x104506b4);
            return;
        }
        if (kind_value == 3) {
            c.set({{3, text}, {4, pool + 0x3e1}});
            c.call(callee::format, 0x104506cc);
            return;
        }
    }
    invoke(c, stage_text, 0x104506e8, {code, sext16(second_word), text, position, 0});
}

std::uint32_t place_text(Cpu& c, std::uint32_t competition, std::uint32_t text, std::uint32_t club, std::uint32_t stage_index) {
    const Frame frame(c, 0x10451ac0, 0x80, {.first_gpr = 25});
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message);
    // r26 the message buffer, r27 the pool, r28 the competition, r29 the text, r30 the club, r31 the stage index; r25
    // keeps the caller's value until the stage object is chosen.
    c.set({{26, message}, {27, pool}, {28, competition}, {29, text}, {30, club}, {31, stage_index}});
    c.set({{3, text}, {4, pool + 0x3c1}});
    c.call(callee::sprintf, 0x10451af4);  // the empty text first
    const auto rules = c.u32(c.toc(toc::fixture_rules));
    std::uint32_t object;
    if (rules != 0 && c.u32(competition + competition::record) == c.u32(rules + competition::record)) {
        object = competition;
        c.set(25, object);
    } else {
        object = c.u32(c.u32(c.toc(toc::competitions)) + c.u32(c.u32(competition + competition::record)) * 4);
        c.set(25, object);
        if (std::int32_t(stage_index) < -1 || std::int32_t(stage_index) > c.i32(object + competition::last_stage)) {
            report(c, frame, {0x10451b5c, 0x40, 0x38, 0x605});
            c.set(3, text);
            return text;
        }
        if (std::int32_t(stage_index) > -1) {
            object = c.u32(c.u32(object + competition::stages) + stage_index * 4);
            c.set(25, object);
        }
    }
    if (c.u8(object + competition::kind) == competition::knockout) {
        // The club's row number among the entries: the loop leaves r26 one past the match (or at the count).
        std::uint32_t index = 0, counter = 0;
        c.set({{26, 0}, {27, 0}});
        while (std::int32_t(counter) < std::int32_t(c.i16(object + competition::club_count))) {
            const auto previous = index;
            index = previous + 1;
            c.set(26, index);
            const auto row = invoke(c, round_row, 0x10451be4, {object, previous});
            if (c.u32(row) == club) break;
            counter = counter + 1;
            c.set(27, counter);
        }
        const auto ordinal_text = invoke(c, ordinal, 0x10451c0c, {index, 0xffffffffu});
        invoke(c, callee::strcpy, 0x10451c18, {text, ordinal_text});
    } else if (sext8(c.u8(competition + competition::type)) == 1) {
        if (c.u32(club + 0x30) != 0) invoke(c, callee::strcpy, 0x10451c48, {text, c.u32(club + 0x30) + 0x14});
    } else if (sext8(c.u8(competition + competition::type)) == 2) {
        if (c.u32(club + 0x2c) != 0) invoke(c, callee::strcpy, 0x10451c70, {text, c.u32(club + 0x2c) + 0xa4});
    }
    c.set(3, text);
    return text;
}
}
