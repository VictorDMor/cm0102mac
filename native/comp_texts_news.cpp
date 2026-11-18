// COMP.CPP, block `news`: the renderings of the news items filed by comp_news.cpp (kinds 0x1a, 0x1f and 0x23 of
// the news screens): each reads the item's fields (0x102cb570), formats a message (0x10727890) and gives it to the
// text object with a link row per name. Stack arguments of the formatter beyond r10 sit at 0x38(sp) onwards of this
// routine's frame, where the original writes them. The Cpu's registers stand for the original's non-volatile
// registers (see comp_texts.cpp).
#include "comp_news.hpp"
#include "comp_texts_detail.hpp"

namespace cm::aot::comp {
using namespace texts;

namespace {
// The amount's text: the item's amount word is stored at `slot` and shown by CASH (0x10009f20, 0x1000b410) into `buffer`.
void money_text(Cpu& c, std::uint32_t slot, std::uint32_t amount, std::uint32_t buffer, std::uint32_t at_a, std::uint32_t at_b) {
    c.put32(slot, amount);
    invoke(c, money_a, at_a + 4, {slot, 6});
    const auto high = c.r(3), low = c.r(4);
    invoke(c, money_b, at_b + 4, {buffer, 0x1e, high, low, 6});
}
}

void prize_news_text(Cpu& c, std::uint32_t competition, std::uint32_t item, std::uint32_t form, std::uint32_t output) {
    const Frame frame(c, 0x10452290, 0x1d0, {.first_gpr = 20});
    const auto pool = c.toc(toc::pool), buffer = c.toc(toc::text), clubs_cell = c.toc(toc::clubs);
    // r20 the item, r27 the pool, r28 the text buffer, r29 the clubs cell, r30 the competition, r31 the output.
    c.set({{20, item}, {27, pool}, {28, buffer}, {29, clubs_cell}, {30, competition}, {31, output}});
    const auto clubs = [&] { return c.u32(clubs_cell); };
    const auto record = [&] { return c.u32(competition + competition::record); };
    const auto money = frame.at(0x50);
    const auto link_club = [&](std::uint32_t at, std::uint32_t offset) {
        link(c, at, output, clubs() + offset + club::name, club_tag, clubs() + offset);
    };
    const auto link_competition = [&](std::uint32_t at) {
        link(c, at, output, record() + record::name, competition_tag, record());
    };
    if (sext8(form) == 0) {
        c.set(25, field(c, 0x10452718, item, 1));
        const auto amount = c.r(25);
        c.set(25, field(c, 0x10452728, item, 2));
        money_text(c, frame.at(0x48), amount, money, 0x1045273c, 0x10452754);
        c.set(23, c.r(25) * 0x248);
        const auto offset = c.r(23);
        const auto gender = sext8(c.u8(clubs() + offset + club::name_gender));
        format(c, 0x10452784, {buffer, pool + 0x848, gender, gender, clubs() + offset + club::name, money});
        set_text(c, 0x10452790, output, buffer);
        link_club(0x104527a8, offset);
        return;
    }
    c.set(24, field(c, 0x104522c8, item, 1));
    c.set(25, field(c, 0x104522d8, item, 2));
    c.set(23, sext8(field(c, 0x104522e8, item, 3)));
    c.set(22, sext16(field(c, 0x104522f8, item, 4)));
    c.set(26, field(c, 0x10452308, item, 5));
    c.set(21, sext8(field(c, 0x10452318, item, 6)));
    const auto seventh = field(c, 0x10452328, item, 7);
    const auto amount = c.r(24);
    c.set(24, sext8(seventh));
    money_text(c, frame.at(0x4c), amount, money, 0x1045233c, 0x10452354);
    const std::int32_t place = std::int32_t(c.r(23));
    const std::uint32_t stage = c.r(22);
    if (place > 0) {
        // The club's place (the ordinal text) in the competition.
        c.set(25, c.r(25) * 0x248);
        c.set(26, clubs());
        c.set(22, record());
        const auto offset = c.r(25), base = c.r(26), rec = c.r(22);
        c.set(24, sext8(c.u8(rec + record::name_gender)));
        c.set(23, sext8(c.u8(base + offset + club::name_gender)));
        const auto record_gender = c.r(24), gender = c.r(23);
        const auto ordinal_text = invoke(c, ordinal, 0x1045238c, {std::uint32_t(place), 0xffffffffu, base + offset});
        c.put32(frame.at(0x38), ordinal_text);
        c.put32(frame.at(0x3c), rec + record::name);
        format(c, 0x104523bc, {buffer, pool + 0x40a, gender, gender, record_gender, record_gender, base + offset + club::name, money});
        set_text(c, 0x104523c8, output, buffer);
        link_club(0x104523e0, offset);
        link_competition(0x104523f4);
        return;
    }
    if (std::int32_t(stage) != 0) {
        // The competition's stage the club reached, named by the record (0x104769d0 into the frame's buffer).
        const auto text = frame.at(0x70);
        invoke(c, stage_name, 0x10452420, {record(), 0, stage, 1, text, 0x12c});
        const auto stage_gender_value = sext8(invoke(c, stage_gender, 0x10452434, {record(), 0, stage, 1}));
        c.set(23, c.r(25) * 0x248);
        const auto offset = c.r(23);
        const auto gender = sext8(c.u8(clubs() + offset + club::name_gender));
        c.put32(frame.at(0x38), text);
        format(c, 0x10452470, {buffer, pool + 0x4c4, gender, gender, stage_gender_value, stage_gender_value,
                                       clubs() + offset + club::name, money});
        set_text(c, 0x1045247c, output, buffer);
        link_club(0x10452494, offset);
        link(c, 0x104524a8, output, text, competition_tag, record());
        return;
    }
    const std::int32_t kind = std::int32_t(c.r(24));
    if (kind == 1 || kind == 2) {
        c.set(23, c.r(25) * 0x248);
        const auto offset = c.r(23);
        const auto rec = record();
        const auto record_gender = sext8(c.u8(rec + record::name_gender));
        const auto gender = sext8(c.u8(clubs() + offset + club::name_gender));
        c.put32(frame.at(0x38), rec + record::name);
        if (kind == 1) {
            format(c, 0x104524fc, {buffer, pool + 0x563, gender, gender, record_gender, record_gender, clubs() + offset + club::name, money});
            set_text(c, 0x10452508, output, buffer);
            link_club(0x10452520, offset);
            link_competition(0x10452534);
        } else {
            format(c, 0x10452588, {buffer, pool + 0x601, gender, gender, record_gender, record_gender, clubs() + offset + club::name, money});
            set_text(c, 0x10452594, output, buffer);
            link_club(0x104525ac, offset);
            link_competition(0x104525c0);
        }
        return;
    }
    // Two clubs: field 2's and field 5's.
    const auto other = c.r(26);
    if (std::int32_t(other) <= -1) return;
    const auto first_offset = c.r(25) * 0x248, second_offset = other * 0x248;
    const auto rec = record();
    const auto first_gender = sext8(c.u8(clubs() + first_offset + club::name_gender));
    const auto second_gender = sext8(c.u8(clubs() + second_offset + club::name_gender));
    const auto record_gender = sext8(c.u8(rec + record::name_gender));
    c.put32(frame.at(0x38), clubs() + first_offset + club::name);
    c.put32(frame.at(0x3c), money);
    c.put32(frame.at(0x40), clubs() + second_offset + club::name);
    c.put32(frame.at(0x44), rec + record::name);
    const bool wins = sext8(c.r(21)) != 0;
    format(c, wins ? 0x10452640 : 0x104526b0,
           {buffer, pool + (wins ? 0x6abu : 0x779u), first_gender, first_gender, second_gender, second_gender, record_gender, record_gender});
    set_text(c, 0x104526bc, output, buffer);
    link_club(0x104526d8, first_offset);
    link_club(0x104526f4, second_offset);
    link_competition(0x10452708);
}

void money_news_text(Cpu& c, std::uint32_t competition, std::uint32_t item, std::uint32_t form, std::uint32_t output) {
    const Frame frame(c, 0x10452880, 0xa0, {.first_gpr = 24});
    const auto pool = c.toc(toc::pool), buffer = c.toc(toc::text), clubs_cell = c.toc(toc::clubs);
    // r24 the item, r27 the competition, r28 the output, r29 the pool, r30 the text buffer, r31 the clubs cell.
    c.set({{24, item}, {27, competition}, {28, output}, {29, pool}, {30, buffer}, {31, clubs_cell}});
    const auto clubs = [&] { return c.u32(clubs_cell); };
    if (sext8(form) == 0) {
        c.set(27, field(c, 0x104529b4, item, 0));
        const auto second = field(c, 0x104529c4, item, 2);
        money_text(c, frame.at(0x40), second, frame.at(0x5c), 0x104529d4, 0x104529ec);
        c.set(25, c.r(27) * 0x248);
        const auto offset = c.r(25);
        const auto gender = sext8(c.u8(clubs() + offset + club::name_gender));
        format(c, 0x10452a1c, {buffer, pool + 0x94c, gender, gender, clubs() + offset + club::name, frame.at(0x5c)});
        set_text(c, 0x10452a28, output, buffer);
        link(c, 0x10452a40, output, clubs() + offset + club::name, club_tag, clubs() + offset);
        return;
    }
    c.set(25, field(c, 0x104528b8, item, 0));
    c.set(26, field(c, 0x104528c8, item, 2));
    const auto third = field(c, 0x104528d8, item, 3);
    const auto amount = c.r(26);
    c.set(26, third & 0xffff);
    money_text(c, frame.at(0x44), amount, frame.at(0x5c), 0x104528ec, 0x10452904);
    c.set(25, c.r(25) * 0x248);
    const auto offset = c.r(25);
    invoke(c, club_text, 0x10452924, {c.r(26), clubs() + offset, frame.at(0x48), 0x14});
    const auto rec = c.u32(competition + competition::record);
    const auto gender = sext8(c.u8(clubs() + offset + club::name_gender));
    const auto record_gender = sext8(c.u8(rec + record::short_name_gender));
    c.put32(frame.at(0x38), rec + record::short_name);
    c.put32(frame.at(0x3c), frame.at(0x48));
    format(c, 0x1045296c, {buffer, pool + 0x88f, gender, gender, record_gender, record_gender, clubs() + offset + club::name, frame.at(0x5c)});
    set_text(c, 0x10452978, output, buffer);
    link(c, 0x10452990, output, clubs() + offset + club::name, club_tag, clubs() + offset);
    link(c, 0x104529a4, output, c.u32(competition + competition::record) + record::short_name, competition_tag, c.u32(competition + competition::record));
}

void movement_news_text(Cpu& c, std::uint32_t competition, std::uint32_t item, std::uint32_t form, std::uint32_t output) {
    const Frame frame(c, 0x10452bb0, 0x80, {.first_gpr = 21});
    const auto pool = c.toc(toc::pool), buffer = c.toc(toc::text), clubs_cell = c.toc(toc::clubs), records_cell = c.toc(toc::records);
    // r21 the form, r23 the item, r25 the records cell, r26 the pool, r27 the text buffer, r28 the clubs cell,
    // r30 the competition, r31 the output.
    c.set({{23, item}, {30, competition}, {25, records_cell}, {21, form}, {26, pool}, {31, output}, {27, buffer}, {28, clubs_cell}});
    const auto clubs = [&] { return c.u32(clubs_cell); };
    const auto record = [&] { return c.u32(competition + competition::record); };
    const auto kind = std::int32_t(sext8(field(c, 0x10452be8, item, 4)));
    const auto link_club = [&](std::uint32_t at, std::uint32_t offset) {
        link(c, at, output, clubs() + offset + club::name, club_tag, clubs() + offset);
    };
    if (sext8(form) == 0) {
        if (kind == 1) {
            c.set(21, field(c, 0x10452dcc, item, 1) * 0x248);
            const auto offset = c.r(21), rec = record();
            const auto record_gender = sext8(c.u8(rec + record::short_name_gender));
            const auto gender = sext8(c.u8(clubs() + offset + club::name_gender));
            format(c, 0x10452e0c, {buffer, pool + 0xb2a, gender, gender, record_gender, record_gender, clubs() + offset + club::name,
                                   rec + record::short_name});
            set_text(c, 0x10452e18, output, buffer);
            link_club(0x10452e30, offset);
            link(c, 0x10452e44, output, record() + record::short_name, competition_tag, record());
        } else if (kind == 2) {
            c.set(21, field(c, 0x10452e5c, item, 1) * 0x248);
            const auto offset = c.r(21);
            const auto gender = sext8(c.u8(clubs() + offset + club::name_gender));
            format(c, 0x10452e88, {buffer, pool + 0xb8e, gender, gender, clubs() + offset + club::name});
            set_text(c, 0x10452e94, output, buffer);
            link_club(0x10452eac, offset);
        }
        return;
    }
    if (kind == 1) {
        c.set(22, field(c, 0x10452c08, item, 1));
        c.set(29, field(c, 0x10452c18, item, 2));
        const auto third = field(c, 0x10452c28, item, 3);
        c.set(24, third * 0x6c);
        const auto rec = record(), records = c.u32(records_cell);
        const auto third_row = records + c.r(24);
        c.set(23, c.r(29) * 0x248);
        c.set(21, sext8(c.u8(third_row + record::short_name_gender)));
        c.set(22, c.r(22) * 0x248);
        const auto first_offset = c.r(22), second_offset = c.r(23);
        const auto record_gender = sext8(c.u8(rec + record::short_name_gender));
        const auto first_gender = sext8(c.u8(clubs() + first_offset + club::name_gender));
        const auto second_gender = sext8(c.u8(clubs() + second_offset + club::name_gender));
        c.put32(frame.at(0x38), c.r(21));
        c.put32(frame.at(0x3c), c.r(21));
        c.put32(frame.at(0x40), clubs() + first_offset + club::name);
        c.put32(frame.at(0x44), rec + record::name);
        c.put32(frame.at(0x48), clubs() + second_offset + club::name);
        c.put32(frame.at(0x4c), third_row + record::short_name);
        format(c, 0x10452cb4, {buffer, pool + 0x991, first_gender, first_gender, record_gender, record_gender, second_gender, second_gender});
        set_text(c, 0x10452cc0, output, buffer);
        link_club(0x10452cd8, first_offset);
        link_club(0x10452cf0, second_offset);
        link(c, 0x10452d04, output, record() + record::name, competition_tag, record());
        const auto base = c.u32(records_cell);
        link(c, 0x10452d24, output, base + c.r(24) + record::short_name, competition_tag, base + c.r(29) * 0x6c);
    } else if (kind == 2) {
        c.set(21, field(c, 0x10452d3c, item, 1) * 0x248);
        const auto offset = c.r(21), rec = record();
        const auto record_gender = sext8(c.u8(rec + record::name_gender));
        const auto gender = sext8(c.u8(clubs() + offset + club::name_gender));
        format(c, 0x10452d7c, {buffer, pool + 0xa76, gender, gender, record_gender, record_gender, clubs() + offset + club::name,
                               rec + record::name});
        set_text(c, 0x10452d88, output, buffer);
        link_club(0x10452da0, offset);
        link(c, 0x10452db4, output, record() + record::name, competition_tag, record());
    }
}
}
