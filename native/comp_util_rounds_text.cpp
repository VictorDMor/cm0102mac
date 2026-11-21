// COMP_UTIL.CPP, block `round_text`: the sentences built around a round's name (comp_util_rounds.cpp has the
// name itself, 0x10469bb0, and the class of its gender, 0x1046f1f0). A competition record (the database's 0x6c-byte
// row) has a long name (+0x1a) and a short one (+0x4d), each with a gender byte (+0x6a, +0x67); a fixture row has
// two round codes, halfwords at +0x30 and +0x32 (0 for none). The texts put the names together in the language's
// own order and pass each one's gender twice to the localized format beside it (the format picks a word form
// from it). Every routine keeps the original's frame and the registers at each call (the formatter and the news
// item's appenders save and copy the caller's non-volatile registers), and writes its locals into its own frame
// as the original does (two routines read locals no path of theirs has written).
#include "comp_util_detail.hpp"

namespace cm::aot::comp_util {
namespace {
constexpr std::uint32_t format_entry = 0x10727890, copy_entry = 0x1077dbf0, language_entry = 0x10727880, contraction_entry = 0x10728ac0,
                        begin_item_entry = 0x1074b830, append_entry = 0x1074bab0;
constexpr std::int32_t pool_slot = -0x47e4, assert_buffer_slot = -0x7ba8, success_flag_slot = -0x7bac, text_buffer_slot = -0x7bb8,
                       clubs_slot = -0x7be8;
constexpr std::uint32_t empty_text = 0x5380;  // the pool's empty string
// A club row's fields the news lines read: the name's gender byte and the name (comp_util_detail.hpp `club`).
constexpr std::uint32_t club_row_size = 0x248;

// A localized format: the register arguments set, then the call (stack arguments are stored by the caller).
void localized_format(Cpu& c, std::uint32_t back, std::initializer_list<std::pair<unsigned, std::uint32_t>> arguments) {
    c.set(arguments);
    c.call(format_entry, back);
}
std::uint32_t signed_byte(std::uint32_t value) { return sext8(std::uint8_t(value)); }
std::uint32_t signed_half(std::uint32_t value) { return sext16(std::uint16_t(value)); }
// A round code without its last digit below 1000 (the compiler's inline copy of 0x104648c0: the value minus its
// signed remainder by ten), as the news lines hand it on.
std::uint32_t without_last_digit(std::uint32_t code) {
    const auto value = std::int32_t(signed_half(code));
    return value < 1000 ? std::uint32_t(value - value % 10) : std::uint32_t(value);
}
// The bytes of a text through the language-8 table (an upper-case or accent map, TOC -0x618c): byte 0xff stays,
// the others become table[byte]; the index is a signed byte (the buffers are 100 bytes). Leaves r3..r6 as the loop.
void translate_text(Cpu& c, std::uint32_t text, std::uint32_t table) {
    std::uint32_t counter = 0, index = 0, byte = 0;
    c.set({{5, text}, {6, 0}});
    for (;;) {
        index = signed_byte(counter);
        byte = c.u8(text + index);
        if (signed_byte(byte) == 0) break;
        c.put8(text + index, signed_byte(byte) == 0xffffffffu ? 0xffu : c.u8(table + byte));
        ++counter;
    }
    c.set({{3, index}, {4, byte}, {5, text}, {6, counter}});
}
}

std::uint32_t round_pair_text(Cpu& c, std::uint32_t first_code, std::uint32_t second_code, std::uint32_t buffer, std::uint32_t size, std::uint32_t kind) {
    Frame frame(c, 0x10476f40, 0x140, {.first_gpr = 26});
    const auto pool = c.toc(pool_slot);
    c.set({{26, first_code}, {27, second_code}, {28, buffer}, {29, kind}, {30, pool}, {31, c.toc(assert_buffer_slot)}});
    if (buffer == 0 || std::int32_t(size) < 200) {
        guest::report_assertion(c, {0x10476f88, pool, pool + 0xe, pool + 0x1b, frame.at(0x40), frame.at(0x38), 0x26a0});
        c.set({{4, c.toc(success_flag_slot)}, {3, 0}});
        return 0;
    }
    c.put8(buffer, 0);
    // A round's name text with `kind` into `to` (room bytes), asked for the code's halfword.
    const auto name_of = [&](std::uint32_t to, std::uint32_t room, std::uint32_t code, std::uint32_t back) {
        c.set({{3, to}, {4, room}, {5, signed_half(code)}, {6, kind}});
        c.link(back);
        round_text(c, to, room, signed_half(code), kind);
    };
    const auto gender_of = [&](std::uint32_t code, std::uint32_t back) {
        c.set({{3, signed_half(code)}, {4, kind}});
        c.link(back);
        return std::uint32_t(round_gender(c, signed_half(code), kind));
    };
    // One code (a whole register compared with 0, not its halfword): that code's name, straight into the buffer.
    if (first_code == 0 || second_code == 0) {
        std::uint32_t answer = 1;
        if (first_code != 0) name_of(buffer, size, first_code, 0x10477078);
        else if (second_code != 0) name_of(buffer, size, second_code, 0x1047709c);
        else answer = 0;
        c.set(3, answer);
        return answer;
    }
    const auto first_text = frame.at(0xc4), second_text = frame.at(0x60);
    name_of(first_text, 100, first_code, 0x10476ffc);
    c.set(31, gender_of(first_code, 0x10477008));
    name_of(second_text, 100, second_code, 0x10477020);
    const auto first_gender = signed_byte(c.r(31)), second_gender = signed_byte(gender_of(second_code, 0x1047702c));
    localized_format(c, 0x10477054, {{3, buffer}, {4, pool + 0x8bf2}, {5, first_gender}, {6, first_gender}, {7, second_gender},
                                     {8, second_gender}, {9, first_text}, {10, second_text}});
    c.set(3, 1);
    return 1;
}

std::uint32_t fixture_name_text(Cpu& c, std::uint32_t fixture, std::uint32_t kind, std::uint32_t buffer, std::uint32_t size) {
    Frame frame(c, 0x104768a0, 0xa0, {.first_gpr = 29});
    const auto pool = c.toc(pool_slot);
    // r9 the fixture, r7 and r8 the buffer and size (the original's moves before the tests), r0 the kind.
    c.set({{9, fixture}, {31, pool}, {7, buffer}, {30, c.toc(assert_buffer_slot)}, {8, size}, {29, c.toc(success_flag_slot)}});
    if (fixture == 0 || buffer == 0 || std::int32_t(size) < 0x67) {
        guest::report_assertion(c, {0x104768fc, pool, pool + 0xe, pool + 0x1b, frame.at(0x68), frame.at(0x40), 0x2587});
        c.set(3, 0);
        return 0;
    }
    const auto record = c.u32(fixture + 0x14);
    if (record == 0) {
        guest::report_assertion(c, {0x10476960, pool, pool + 0xe, pool + 0x1b, frame.at(0x48), frame.at(0x38), 0x258f});
        c.set(3, 0);
        return 0;
    }
    const auto code_32 = signed_half(c.u16(fixture + 0x32)), code_30 = signed_half(c.u16(fixture + 0x30));
    c.set({{3, record}, {4, code_32}, {5, code_30}, {6, kind}});
    c.link(0x104769b4);
    return competition_name_text(c, record, code_32, code_30, kind, buffer, size);
}

std::uint32_t competition_name_text(Cpu& c, std::uint32_t record, std::uint32_t code_32, std::uint32_t code_30, std::uint32_t kind_register, std::uint32_t buffer, std::uint32_t size) {
    Frame frame(c, 0x104769d0, 0x1d0, {.first_gpr = 22});
    const auto pool = c.toc(pool_slot);
    const auto name = frame.at(0x138), text_32 = frame.at(0xd4), text_30 = frame.at(0x70);
    const auto table = c.toc(-0x618c), id_cell = c.toc(-0x6750), records = c.toc(-0x7bbc);
    // r22 the assertion buffer, r23 the record, r24 and r25 the codes, r26 and r27 the codes' genders (+0x30's and
    // +0x32's), r28 the name's gender, r29 the table, r30 the pool, r31 the output; r7 and r9 the cells read below.
    c.set({{31, buffer}, {29, table}, {9, id_cell}, {23, record}, {7, records}, {24, code_32}, {22, c.toc(assert_buffer_slot)}, {25, code_30},
           {30, pool}, {28, 0}, {27, 0}, {26, 0}});
    if (buffer == 0 || std::int32_t(size) < 0x67) {
        guest::report_assertion(c, {0x10476a30, pool, pool + 0xe, pool + 0x1b, frame.at(0x50), frame.at(0x48), 0x25af});
        c.set({{4, c.toc(success_flag_slot)}, {3, 0}});
        return 0;
    }
    // Two special competitions (the id the cell at TOC -0x6750 holds) answer to another record when the +0x32 code
    // is their marker: the record of the index the word at TOC -0x6748 (0x453) or -0x674c (0x456) points to.
    for (const auto& [marker, index_slot] : {std::pair<std::int32_t, std::int32_t>{0x453, -0x6748}, {0x456, -0x674c}}) {
        if (record != 0 && c.u32(record) == c.u32(id_cell) && std::int32_t(signed_half(code_32)) == marker) {
            const auto index_cell = c.toc(index_slot);
            code_32 = 0;
            record = c.u32(records) + c.u32(index_cell) * comp::record::size;
            c.set({{3, index_cell}, {24, 0}, {4, c.u32(records)}, {23, record}});
        }
    }
    // The kind picks the name (kind 1 the long one, 2 and 3 the short one) and which kind of round texts to ask for.
    const auto kind = std::int32_t(signed_byte(kind_register));
    const bool texts = kind == 1 || kind == 2 || kind == 3;
    if (texts) {
        const auto field = kind == 1 ? comp::record::name : comp::record::short_name;
        const auto gender_field = kind == 1 ? comp::record::name_gender : comp::record::short_name_gender;
        if (record != 0) {
            c.set({{3, name}, {4, record + field}});
            c.call(copy_entry, kind == 1 ? 0x10476b10 : kind == 2 ? 0x10476b88 : 0x10476c00);
            c.set(28, c.u8(record + gender_field));
        } else {
            c.put8(name, 0);
        }
        // Each round text and its gender; the bl sites differ per kind.
        const std::uint32_t round_kind = kind == 3 ? 1 : 0;
        const std::uint32_t sites[3][4] = {{0x10476b34, 0x10476b40, 0x10476b58, 0x10476b64},
                                           {0x10476bac, 0x10476bb8, 0x10476bd0, 0x10476bdc},
                                           {0x10476c24, 0x10476c30, 0x10476c48, 0x10476c54}};
        const auto& at = sites[kind - 1];
        c.set({{3, text_32}, {4, 100}, {5, code_32}, {6, round_kind}});
        c.link(at[0]);
        round_text(c, text_32, 100, code_32, round_kind);
        c.set({{3, code_32}, {4, round_kind}});
        c.link(at[1]);
        c.set(27, std::uint32_t(round_gender(c, code_32, round_kind)));
        c.set({{3, text_30}, {4, 100}, {5, code_30}, {6, round_kind}});
        c.link(at[2]);
        round_text(c, text_30, 100, code_30, round_kind);
        c.set({{3, code_30}, {4, round_kind}});
        c.link(at[3]);
        c.set(26, std::uint32_t(round_gender(c, code_30, round_kind)));
    }
    const bool has_32 = signed_half(code_32) != 0, has_30 = signed_half(code_30) != 0;
    // Language 8 (the lookup's current language) rewrites the name through 0x10728ac0 and the round texts through the table.
    if (record != 0) {
        c.call(language_entry, 0x10476c64);
        if (signed_byte(c.r(3)) == 8 && (has_32 || has_30)) {
            c.set({{3, name}, {4, pool + empty_text}, {5, pool + 0x8b42}, {6, 100}, {7, 100}});
            c.call(contraction_entry, 0x10476c9c);
            c.set({{3, name}, {4, c.r(3)}});
            c.call(copy_entry, 0x10476cac);
            if (has_32) translate_text(c, text_32, table);
            if (has_30) translate_text(c, text_30, table);
        }
    }
    const auto gender = [&](unsigned r) { return signed_byte(c.r(r)); };
    if (record != 0 && has_32 && has_30) {
        c.put32(frame.at(0x38), name);
        c.put32(frame.at(0x3c), text_32);
        c.put32(frame.at(0x40), text_30);
        const auto g = gender(28), g32 = gender(27), g30 = gender(26);
        localized_format(c, 0x10476d94, {{9, g30}, {5, g}, {7, g32}, {3, buffer}, {6, g}, {8, g32}, {10, g30}, {4, pool + 0x8b44}});
    } else if (record != 0 && has_32) {
        const auto g = gender(28), g32 = gender(27);
        localized_format(c, 0x10476dd0, {{7, g32}, {5, g}, {3, buffer}, {6, g}, {8, g32}, {9, name}, {10, text_32}, {4, pool + 0x8b7e}});
    } else if (record != 0 && has_30) {
        const auto g = gender(28), g30 = gender(26);
        localized_format(c, 0x10476e0c, {{7, g30}, {5, g}, {3, buffer}, {6, g}, {8, g30}, {9, name}, {10, text_30}, {4, pool + 0x8ba7}});
    } else if (record != 0) {
        c.set({{3, buffer}, {4, name}});
        c.call(copy_entry, 0x10476e24);
    } else if (has_32 && has_30) {
        const auto g32 = gender(27), g30 = gender(26);
        localized_format(c, 0x10476e64, {{7, g30}, {5, g32}, {3, buffer}, {6, g32}, {8, g30}, {9, text_32}, {10, text_30}, {4, pool + 0x8bd0}});
    } else if (has_32) {
        c.set({{3, buffer}, {4, text_32}});
        c.call(copy_entry, 0x10476e84);
    } else if (has_30) {
        c.set({{3, buffer}, {4, text_30}});
        c.call(copy_entry, 0x10476ea4);
    }
    c.set(3, 1);
    return 1;
}

void competition_round_news(Cpu& c, std::uint32_t club, std::uint32_t fixture, std::uint32_t kind_register, std::uint32_t item) {
    Frame frame(c, 0x10469710, 0x250, {.first_gpr = 25});
    const auto pool = c.toc(pool_slot);
    const auto text_buffer = c.toc(text_buffer_slot), clubs_cell = c.toc(clubs_slot);
    c.set({{26, fixture}, {29, text_buffer}, {30, clubs_cell}, {25, club}, {31, pool}, {27, kind_register}, {28, item}});
    const auto text = frame.at(0x38);
    // The competition record and the two round codes of the fixture; the +0x30 one is rounded down to ten.
    const auto record = c.u32(fixture + 0x14);
    const auto code_32 = signed_half(c.u16(fixture + 0x32)), code_30 = signed_half(without_last_digit(c.u16(fixture + 0x30)));
    c.set({{3, record}, {4, code_32}, {5, code_30}, {6, 2}, {7, text}, {8, 500}});
    c.link(0x1046978c);
    competition_name_text(c, record, code_32, code_30, 2, text, 500);
    c.set({{3, record}, {4, code_32}, {5, code_30}, {6, 2}});
    c.link(0x104697d4);
    const auto gender = signed_byte(competition_gender(c, record, code_32, code_30, 2));
    // Kinds 2, 4 and -1 take the first format, the others the second; each names the club (with its name's gender)
    // beside the competition's text.
    const auto kind = std::int32_t(signed_byte(kind_register));
    const bool first_format = kind == 2 || kind == 4 || kind == -1;
    const auto offset = club * club_row_size;
    const auto row = c.u32(clubs_cell) + offset;
    const auto club_gender = signed_byte(c.u8(row + comp::club::name_gender));
    localized_format(c, first_format ? 0x10469828 : 0x10469864,
                     {{3, text_buffer}, {7, gender}, {8, gender}, {5, club_gender}, {6, club_gender}, {9, row + comp::club::name}, {10, text},
                      {4, pool + (first_format ? 0x50d8u : 0x5138u)}});
    c.set({{3, item}, {4, text_buffer}});
    c.call(begin_item_entry, 0x10469870);
    c.set({{3, item}, {4, row + comp::club::name}, {5, 0x7d5}, {6, row}});
    c.call(append_entry, 0x1046988c);
    c.set({{3, item}, {4, text}, {5, 0x7d0}, {6, record}});
    c.call(append_entry, 0x104698a0);
}

void fixture_round_news(Cpu& c, std::uint32_t club, std::uint32_t code_register, std::uint32_t record, std::uint32_t kind_register, std::uint32_t with_round_register, std::uint32_t item) {
    Frame frame(c, 0x104698c0, 0x130, {.first_gpr = 25});
    const auto pool = c.toc(pool_slot);
    const auto text_buffer = c.toc(text_buffer_slot), clubs_cell = c.toc(clubs_slot);
    c.set({{27, text_buffer}, {25, club}, {28, clubs_cell}, {26, code_register}, {29, pool}, {30, record}, {31, item}});
    const auto kind = std::int32_t(signed_byte(kind_register));
    // Kinds 6 and 7 (one family) and -2 (the other) name the club and the competition's long name (with a round's
    // text) or short name (without one); any other kind builds nothing.
    if (kind != 6 && kind != 7 && kind != -2) return;
    const bool pair = kind != -2, with_round = signed_byte(with_round_register) != 0;
    // The addresses after each call, in the order of the original's code for this family and variant.
    struct Sites { std::uint32_t round_text, gender, format, begin, club_name, name, round; };
    constexpr Sites sites[2][2] = {{{0, 0, 0x10469b60, 0x10469b6c, 0x10469b84, 0x10469b98, 0},
                                    {0x10469a70, 0x10469a7c, 0x10469ad0, 0x10469adc, 0x10469af4, 0x10469b08, 0x10469b20}},
                                   {{0, 0, 0x10469a10, 0x10469a1c, 0x10469a34, 0x10469a48, 0},
                                    {0x1046991c, 0x10469928, 0x1046997c, 0x10469988, 0x104699a0, 0x104699b4, 0x104699d0}}};
    const auto& at = sites[pair][with_round];
    const std::uint32_t name_field = with_round ? comp::record::name : comp::record::short_name;
    const auto offset = club * club_row_size;
    const auto text = frame.at(pair ? 0xac : 0x48);
    std::uint32_t gender = 0;
    if (with_round) {
        c.set({{3, text}, {4, 100}, {5, code_register}, {6, 0}});
        c.link(at.round_text);
        round_text(c, text, 100, code_register, 0);
        c.set({{3, code_register}, {4, 0}});
        c.link(at.gender);
        gender = signed_byte(std::uint32_t(round_gender(c, code_register, 0)));
    }
    c.set(25, offset);
    const auto row = c.u32(c.toc(clubs_slot)) + offset;
    const auto club_gender = signed_byte(c.u8(row + comp::club::name_gender));
    if (with_round) {
        const auto record_gender = signed_byte(c.u8(record + comp::record::name_gender));
        c.put32(frame.at(0x38), row + comp::club::name);
        c.put32(frame.at(0x3c), text);
        c.put32(frame.at(0x40), record + name_field);
        localized_format(c, at.format, {{3, text_buffer}, {5, club_gender}, {6, club_gender}, {7, gender}, {8, gender}, {9, record_gender}, {10, record_gender},
                                        {4, pool + (pair ? 0x518eu : 0x527fu)}});
    } else {
        const auto record_gender = signed_byte(c.u8(record + comp::record::short_name_gender));
        localized_format(c, at.format, {{3, text_buffer}, {5, club_gender}, {6, club_gender}, {7, record_gender}, {8, record_gender},
                                        {9, row + comp::club::name}, {10, record + name_field}, {4, pool + (pair ? 0x5221u : 0x531au)}});
    }
    c.set({{3, item}, {4, text_buffer}});
    c.call(begin_item_entry, at.begin);
    c.set({{3, item}, {4, row + comp::club::name}, {5, 0x7d5}, {6, row}});
    c.call(append_entry, at.club_name);
    c.set({{3, item}, {4, record + name_field}, {5, 0x7d0}, {6, record}});
    c.call(append_entry, at.name);
    if (with_round) {
        // The round's text tagged with a word of the code's bits (rotated into place) and, for kinds 6 and 7, the
        // competition's id.
        const auto rotl = [](std::uint32_t v, unsigned n) { return (v << n) | (v >> (32 - n)); };
        const auto tag = rotl(code_register, 11) & 0x003ff800u;
        const auto id = c.u32(record);
        const auto word = pair ? (tag | (rotl(id, 22) & 0xffc00000u)) : (tag | 0xffc00000u);
        c.set({{3, item}, {4, text}, {5, 0x7e5}, {6, word}});
        c.call(append_entry, at.round);
    }
}
}
