// TRANSFER_MANAGER.CPP, block "text_f": the text of the transfer news message 0x1040c580: a person with a date, two
// clubs, two money amounts (fields 6 and 8, read as floats) and up to three more people the text names.
#include "guest_fpu.hpp"
#include "transfer_manager_native_text_f.hpp"
#include "transfer_manager_native_text_a_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_a;

namespace {
// Frame slots of the original.
constexpr std::uint32_t person_slot = 0x848, day_slot = 0x844, month_slot = 0x840, date_month_slot = 0x83e, date_year_slot = 0x83c;
constexpr std::uint32_t low_magic = 0x838, high_magic = 0x834, second_low_magic = 0x830, second_high_magic = 0x82c;
constexpr std::uint32_t years = 0x58, months = 0x64, days = 0x70, records = 0x7c;  // three words each
constexpr std::uint32_t name_text = 0x1ec, description = 0x37c, extra_descriptions = 0x4a8, extra_step = 0x12c;
constexpr std::uint32_t word_text = 0xf0, date_object = 0x50, date_text = 0x124;
constexpr std::uint32_t first_slot = 0x4c, first_text = 0xbc, second_slot = 0x48, second_text = 0x88;
constexpr std::uint32_t pool_cell = -0x4ba0, threshold_cell = -0x4ba4, template_cell = -0x4be0;
}

// 0x1040c580: field 0 a person with a date (fields 1-3), fields 4 and 5 the clubs (5 may be -1), fields 6 and 8 two
// amounts, 10 and 11 a month and a year, 12 the number of extra people (their records start at field 13, four fields each).
void news_text_1040c580(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1040c580, 0x8b0, {.first_gpr = 13, .first_fpr = 30});
    guest::Fpu fpu(c);
    const auto buffer = c.toc(buffer_cell), people_cell = c.toc(toc::people), text = c.toc(text_cell);
    const Clubs clubs{c, c.toc(toc::clubs)};
    keep(c, 15, item);
    keep(c, 26, buffer);
    keep(c, 16, output);
    keep(c, 27, people_cell);
    keep(c, 20, 0xd);
    keep(c, 28, clubs.cell);
    keep(c, 29, text);
    if (std::int8_t(full) != 0) {
        const auto desc = frame.at(description);
        // The text object's initial bytes come from the template the table of contents points at.
        const auto source = c.toc(template_cell);
        for (std::uint32_t at = 0; at < 400; at += 4) c.put32(frame.at(name_text + at), c.u32(source + at));
        const auto person = field(c, item, 0, 0x1040c5ec);
        c.put32(frame.at(person_slot), person);
        const auto day = field(c, item, 1, 0x1040c600);
        c.put32(frame.at(day_slot), day);
        const auto month = field(c, item, 2, 0x1040c614);
        c.put32(frame.at(month_slot), month);
        const auto year = field(c, item, 3, 0x1040c628);
        keep(c, 13, year);
        const auto first = field(c, item, 4, 0x1040c63c);
        keep(c, 30, first);
        const auto second = field(c, item, 5, 0x1040c64c);
        keep(c, 31, second);
        // An amount: the word's sign-flipped value in the low half of a double with the magic exponent, less the pool's magic double.
        const auto amount = [&](std::uint32_t high, std::uint32_t low, std::uint32_t value) {
            c.put32(frame.at(low), value ^ 0x80000000u);
            c.put32(frame.at(high), 0x43300000);
            return fpu.fsubs(c.f64(frame.at(high)), c.f64(c.toc(pool_cell) + 8));
        };
        const auto first_amount = amount(high_magic, low_magic, field(c, item, 6, 0x1040c65c));
        c.set_f(31, first_amount);
        const auto second_amount = amount(second_high_magic, second_low_magic, field(c, item, 8, 0x1040c688));
        c.set_f(30, second_amount);
        c.put16(frame.at(date_month_slot), field(c, item, 10, 0x1040c6b4) & 0xffff);
        c.put16(frame.at(date_year_slot), field(c, item, 11, 0x1040c6c8) & 0xffff);
        const auto count = std::int8_t(field(c, item, 12, 0x1040c6dc));
        keep_signed(c, 18, count);
        if (count != 0) {
            std::uint32_t index = 0xd;
            keep(c, 25, 0);
            keep(c, 14, frame.at(records));
            keep(c, 24, 0);
            keep(c, 21, frame.at(days));
            keep(c, 22, frame.at(months));
            keep(c, 23, frame.at(years));
            keep(c, 19, 0);
            const std::int32_t limit = count >= 3 ? 3 : count;
            for (std::int32_t i = 0; i < limit; ++i) {
                keep(c, 20, index + 1);
                const auto who = field(c, item, index++, 0x1040c718);
                keep(c, 17, who);
                c.put32(frame.at(records + 4 * i), c.u32(people_cell) + who * person_size);
                keep(c, 20, index + 1);
                c.put32(frame.at(days + 4 * i), field(c, item, index++, 0x1040c73c));
                keep(c, 20, index + 1);
                c.put32(frame.at(months + 4 * i), field(c, item, index++, 0x1040c750));
                keep(c, 20, index + 1);
                c.put32(frame.at(years + 4 * i), field(c, item, index++, 0x1040c764));
                describe(c, 0x1040c78c, c.u32(frame.at(days + 4 * i)), c.u32(frame.at(months + 4 * i)), c.u32(frame.at(years + 4 * i)),
                         frame.at(extra_descriptions + extra_step * i), 300, 1, who);
                keep(c, 25, 4 * (i + 1));
                keep(c, 24, extra_step * (i + 1));
                keep(c, 19, i + 1);
            }
            const auto names = frame.at(name_text);
            if (count == 1) {
                invoke(c, 0x1077dbf0, after(0x1040c7cc), {names, frame.at(extra_descriptions)});
            } else if (count == 2) {
                say(c, frame, 0x1040c7ec, names, text + 0x243a, {frame.at(extra_descriptions), frame.at(extra_descriptions + extra_step)});
            } else if (count == 3) {
                say(c, frame, 0x1040c810, names, text + 0x248f,
                    {frame.at(extra_descriptions), frame.at(extra_descriptions + extra_step), frame.at(extra_descriptions + 2 * extra_step)});
            } else {
                invoke(c, 0x10746a70, after(0x1040c828), {std::uint32_t(std::int32_t(count)), frame.at(word_text), 0x32, 1});
                say(c, frame, 0x1040c838, names, text + 0x250e, {frame.at(word_text)});
            }
        }
        describe(c, 0x1040c858, day, month, year, desc, 300, 1, person);
        const auto date_month = c.u16(frame.at(date_month_slot)), date_year = c.u16(frame.at(date_year_slot));
        const auto date = invoke(c, 0x107156b0, after(0x1040c86c), {frame.at(date_object), std::uint32_t(std::int32_t(std::int16_t(date_month))), date_year});
        invoke(c, 0x10716eb0, after(0x1040c87c), {date, frame.at(date_text), 200, 0xff});
        const auto first_len = clubs.long_len(first), first_name = clubs.long_name(first);
        // The second club may be -1 (no club): the texts that do not name it never read its record.
        const auto second_len = [&] { return clubs.long_len(second); };
        const auto second_name = [&] { return clubs.long_name(second); };
        const auto names = frame.at(name_text);
        const auto threshold = double(c.f32(c.toc(threshold_cell) + 0x30));
        fpu.fcmp(0, second_amount, first_amount);
        if (second_amount > first_amount) {
            c.put64(frame.at(second_high_magic), fpu.fctiwz(0, second_amount));
            c.put32(frame.at(first_slot), c.u32(frame.at(second_low_magic)));
            const auto money = invoke(c, 0x10009f20, after(0x1040c8a0), {frame.at(first_slot), 6});
            invoke(c, 0x1000b410, after(0x1040c8b8), {frame.at(first_text), 0x32, money, c.r(4), 6});
            if (count == 0) {
                say(c, frame, 0x1040c920, buffer, text + text_page - 0x1693,
                    {100, first_len, first_len, second_len(), second_len(), desc, first_name, second_name(), frame.at(first_text)});
            } else {
                say(c, frame, 0x1040c98c, buffer, text + text_page - 0x15d5,
                    {100, first_len, first_len, second_len(), second_len(), desc, first_name, second_name(), frame.at(first_text), names});
            }
        } else {
            c.put64(frame.at(second_high_magic), fpu.fctiwz(0, first_amount));
            c.put32(frame.at(second_slot), c.u32(frame.at(second_low_magic)));
            const auto money = invoke(c, 0x10009f20, after(0x1040c9ac), {frame.at(second_slot), 6});
            invoke(c, 0x1000b410, after(0x1040c9c4), {frame.at(second_text), 0x32, money, c.r(4), 6});
            fpu.fcmp(0, first_amount, threshold);
            const bool high = first_amount > threshold;
            if (count == 0) {
                if (high) {
                    say(c, frame, 0x1040ca3c, buffer, text + text_page - 0x14dc,
                        {100, first_len, first_len, second_len(), second_len(), desc, first_name, second_name(), frame.at(second_text)});
                } else if (second == 0xffffffffu) {
                    say(c, frame, 0x1040cadc, buffer, text + text_page - 0x1398, {100, first_len, first_len, desc, first_name});
                } else {
                    say(c, frame, 0x1040caa0, buffer, text + text_page - 0x142a,
                        {100, first_len, first_len, second_len(), second_len(), desc, first_name, second_name()});
                }
            } else if (high) {
                say(c, frame, 0x1040cb58, buffer, text + text_page - 0x132d,
                    {100, first_len, first_len, second_len(), second_len(), desc, first_name, second_name(), frame.at(second_text), names});
            } else {
                say(c, frame, 0x1040cbbc, buffer, text + text_page - 0x1240,
                    {100, first_len, first_len, second_len(), second_len(), desc, first_name, second_name(), names});
            }
        }
        begin_text(c, 0x1040cbc8, output, buffer);
        say(c, frame, 0x1040cbe0, buffer, text + text_page - 0x115e, {100, frame.at(date_text)});
        extend_text(c, 0x1040cbec, output, buffer);
        link(c, 0x1040cc08, output, first_name, link_club, clubs.at(first));
        if (second != 0xffffffffu) link(c, 0x1040cc2c, output, second_name(), link_club, clubs.at(second));
        const auto person_offset = c.u32(frame.at(person_slot)) * person_size;
        keep(c, 14, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040cc50, record, c.u32(frame.at(day_slot)), c.u32(frame.at(month_slot)), year)) link(c, 0x1040cc70, output, desc, link_person, record);
        if (count != 0 && count <= 3) {
            keep(c, 21, 0);
            keep(c, 17, frame.at(records));
            keep(c, 19, 0);
            keep(c, 15, frame.at(days));
            keep(c, 14, frame.at(months));
            keep(c, 13, frame.at(years));
            for (std::int32_t i = 0; i < count; ++i) {
                const auto who = c.u32(frame.at(records + 4 * i));
                keep(c, 20, who);
                if (known(c, 0x1040ccb8, who, c.u32(frame.at(days + 4 * i)), c.u32(frame.at(months + 4 * i)), c.u32(frame.at(years + 4 * i))))
                    link(c, 0x1040ccd8, output, frame.at(extra_descriptions + extra_step * i), link_person, who);
                keep(c, 13, frame.at(years + 4 * (i + 1)));
                keep(c, 14, frame.at(months + 4 * (i + 1)));
                keep(c, 15, frame.at(days + 4 * (i + 1)));
                keep(c, 17, frame.at(records + 4 * (i + 1)));
                keep(c, 19, extra_step * (i + 1));
                keep(c, 21, i + 1);
            }
        }
    } else {
        const auto person = field(c, item, 0, 0x1040cd0c);
        keep(c, 14, person);
        const auto day = field(c, item, 1, 0x1040cd20);
        keep(c, 17, day);
        const auto month = field(c, item, 2, 0x1040cd34);
        keep(c, 18, month);
        const auto year = field(c, item, 3, 0x1040cd48);
        keep(c, 15, year);
        const auto first = field(c, item, 4, 0x1040cd5c);
        keep(c, 13, first);
        describe(c, 0x1040cd80, day, month, year, frame.at(description), 300, 5, person);
        const auto first_offset = first * club_size;
        keep(c, 13, first_offset);
        say(c, frame, 0x1040cdb4, buffer, text + text_page - 0x110e,
            {clubs.long_len(first), clubs.long_len(first), clubs.long_name(first), frame.at(description)});
        begin_text(c, 0x1040cdc0, output, buffer);
        link(c, 0x1040cdd8, output, clubs.long_name(first), link_club, clubs.at(first));
        const auto person_offset = person * person_size;
        keep(c, 13, person_offset);
        const auto record = c.u32(people_cell) + person_offset;
        if (known(c, 0x1040cdf8, record, day, month, year)) link(c, 0x1040ce18, output, frame.at(description), link_person, record);
    }
}
}
