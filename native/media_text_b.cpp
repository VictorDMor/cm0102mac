// The board's and the fans' reaction to a staff member's behaviour, as press text. The record names
// the people involved (a manager, perhaps another staff member and a chairman), the club, and numbers
// that choose the wording; 0x102cb570 reads it field by field. A short story is one line; a long one
// has a second paragraph on the club's prospects.
#include "media_text_b.hpp"
#include "media_leaves.hpp"
#include "media_text_b_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::press;
using namespace detail::text;

bool Press::transfer(std::uint32_t back) const {
    enter(c, back, {owner, std::uint32_t(code)});
    is_transfer_story(c, code);
    return nonzero(c, std::int8_t(c.r(3)));
}
bool Press::ordinary(std::uint32_t back) const {
    enter(c, back, {owner, std::uint32_t(code)});
    is_ordinary_story(c, code);
    return nonzero(c, std::int8_t(c.r(3)));
}
bool Press::grade_band(std::uint32_t back) const {
    enter(c, back, {owner, std::uint32_t(code)});
    is_grade_band(c, code);
    return nonzero(c, std::int8_t(c.r(3)));
}

std::uint32_t Press::value(Arg arg) const {
    if (arg.v & detail::press::pool_base && !(arg.v & detail::press::slot_base)) return c.toc(slot::pool) + (arg.v - pool_base);
    if (!(arg.v & detail::press::slot_base)) return arg.v;
    switch (arg.v - slot_base) {
    case 1: return frame_at(0xc0);
    case 2: return frame_at(0x48);
    case 3: return frame_at(0x2a0);
    case 4: return frame_at(0x228);
    case 5: return frame_at(0x1b0);
    case 6: return frame_at(0x138);
    case 7: return club + 0x221;
    case 8: return signed_byte(c.u8(club + 0x23b));
    default: return std::uint32_t(count % 20) + 0x41;  // the percent a poll shows
    }
}

// One phrase formatted into the text buffer: the first six arguments travel in r5-r10, the rest in the
// frame's outgoing words.
void Press::say(const Line& line) const {
    std::array<std::uint32_t, 10> values{};
    for (unsigned i = 0; i < line.count; ++i) values[i] = value(line.args[i]);
    c.set(3, text);
    c.set(4, c.toc(slot::pool) + line.phrase);
    for (unsigned i = 0; i < line.count; ++i) {
        if (i < 6) c.set(5 + i, values[i]);
        else c.put32(c.sp() + 0x38 + (i - 6) * 4, values[i]);
    }
    c.call(formatter, line.back);
}

void staff_reaction_text(Cpu& c, std::uint32_t owner, std::uint32_t record, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1066c670, 0x3a0, {.first_gpr = 13});
    Program program{c};
    // r13 holds the staff table's slot, r30 the text buffer, r31 the pool; r15 and r16 the arguments.
    keep(c, {{13, c.toc(staff_slot)}, {15, owner}, {16, output}, {25, record}, {28, 0}, {29, 0}, {30, c.toc(slot::text)}, {31, c.toc(slot::pool)}});
    const auto spare = c.r(6);  // r6 still holds the output object at the first read
    c.put8(frame.at(0x3c3), full);
    // The six name buffers start as copies of the pool's blank templates (0x78 bytes each).
    constexpr std::int32_t templates[6]{-0x421c, -0x4220, -0x4224, -0x4228, -0x422c, -0x4230};
    constexpr std::uint32_t buffers[6]{0x2a0, 0x228, 0x1b0, 0x138, 0xc0, 0x48};
    for (unsigned i = 0; i < 6; ++i)
        for (unsigned word = 0; word < 0x78; word += 4) c.put32(frame.at(buffers[i] + word), c.u32(c.toc(templates[i]) + word));
    c.set(17, 0);
    c.set(24, 0);
    // What the loops leave in the argument registers for the first call.
    c.set(5, frame.at(0xbc));
    c.set(6, spare);
    c.set(7, c.toc(templates[0]));
    Press p(c, frame, owner, output);
    RecordReader read{.c = c, .record = record, .cursor = 17};
    const auto set_club_registers = [&](std::uint32_t id) {
        c.set(5, c.toc(-0x7be8));
        c.set(6, id * club_size);
    };
    const auto kept = [&](unsigned reg, std::uint32_t value) { c.set(reg, value); return value; };

    p.about_board = read.counted(0x1066c7ac) == 1;
    c.put8(frame.at(0x324), std::uint32_t(p.about_board));
    p.code = std::int32_t(kept(23, read.counted(0x1066c7d4)));
    p.tone = std::int32_t(kept(22, read.counted(0x1066c7e8)));
    p.seed = std::int32_t(kept(21, read.counted(0x1066c7fc)));
    {
        const auto id = read.counted(0x1066c810);
        set_club_registers(id);
        p.club = kept(20, table(c, -0x7be8) + id * club_size);
    }
    p.kind = std::int32_t(kept(19, read.counted(0x1066c830)));
    {
        const auto id = read.counted(0x1066c844);
        c.set(5, id * staff_size);
        p.manager = kept(27, table(c, staff_slot) + id * staff_size);
    }
    const auto manager_name = frame.at(0x330), manager_middle = frame.at(0x32c), manager_last = frame.at(0x328);
    c.put32(manager_name, read.counted(0x1066c860));
    c.put32(manager_middle, read.counted(0x1066c874));
    c.put32(manager_last, read.counted(0x1066c888));
    // The staff member the story is about, when the record names one (-1: none): peeked at first,
    // then read for real along with the three parts of his name; one more field is dropped.
    const auto subject_first = frame.at(0x348), subject_middle = frame.at(0x344), subject_last = frame.at(0x340);
    if (compare(c, std::int32_t(read.counted(0x1066c89c, false)), -1) > 0) {
        const auto id = read.counted(0x1066c8b4);
        c.set(5, id * staff_size);
        p.subject = kept(29, table(c, staff_slot) + id * staff_size);
        c.put32(subject_first, read.counted(0x1066c8d0));
        c.put32(subject_middle, read.counted(0x1066c8e4));
        c.put32(subject_last, read.counted(0x1066c8f8));
        read.counted(0x1066c90c);
    } else c.set(17, c.r(17) + 1);
    // Then the chairman, the same way.
    const auto chairman_first = frame.at(0x33c), chairman_middle = frame.at(0x338), chairman_last = frame.at(0x334);
    if (compare(c, std::int32_t(read.counted(0x1066c920, false)), -1) > 0) {
        const auto id = read.counted(0x1066c938);
        c.set(5, id * staff_size);
        p.chairman = kept(28, table(c, staff_slot) + id * staff_size);
        c.put32(chairman_first, read.counted(0x1066c954));
        c.put32(chairman_middle, read.counted(0x1066c968));
        c.put32(chairman_last, read.counted(0x1066c97c));
    } else c.set(17, c.r(17) + 1);
    p.count = std::int32_t(kept(18, read.counted(0x1066c998)));
    // A flag word that is present only when the field before it is.
    if (compare(c, std::int32_t(read.counted(0x1066c9ac))) != 0) p.flags = std::int32_t(kept(24, read.counted(0x1066c9c4)));
    c.put32(frame.at(0x320), read.counted(0x1066c9d8));
    c.put32(frame.at(0x31c), read.counted(0x1066c9ec));
    c.put32(frame.at(0x318), read.counted(0x1066ca00));
    p.extra_a = std::int32_t(kept(14, read.counted(0x1066ca14)));
    p.extra_b = std::int32_t(kept(13, read.counted(0x1066ca28)));
    read.counted(0x1066ca3c);
    p.last = std::int32_t(kept(17, read.counted(0x1066ca48, false)));
    compare_logical(c, p.subject, 0);
    // The names the stories print: each person in full (kind 1) and by surname (kind 5).
    const auto name = [&](std::uint32_t member, std::uint32_t first, std::uint32_t middle, std::uint32_t last, std::uint32_t buffer,
                          std::uint32_t kind, std::uint32_t back) {
        program.call(entry::person_name, back, {c.u32(first), c.u32(middle), c.u32(last), frame.at(buffer), 0x78, kind, c.u32(member)});
    };
    if (p.subject != 0) {
        name(p.subject, subject_first, subject_middle, subject_last, 0xc0, 1, 0x1066ca74);
        name(p.subject, subject_first, subject_middle, subject_last, 0x48, 5, 0x1066ca94);
    }
    compare_logical(c, p.chairman, 0);
    if (p.chairman != 0) {
        name(p.chairman, chairman_first, chairman_middle, chairman_last, 0x2a0, 1, 0x1066cabc);
        name(p.chairman, chairman_first, chairman_middle, chairman_last, 0x228, 5, 0x1066cadc);
    }
    name(p.manager, manager_name, manager_middle, manager_last, 0x1b0, 1, 0x1066cafc);
    name(p.manager, manager_name, manager_middle, manager_last, 0x138, 5, 0x1066cb1c);

    // Whether the manager is the user's own (the user's record is at the selected slot).
    compare_logical(c, p.chairman, 0);
    const auto base = c.toc(-0x7bb0);
    const auto user_row = base + std::uint32_t(std::int32_t(c.i16(base + 0x2c58))) * 0x2c0;
    c.set(4, user_row);
    p.own_club = p.manager == c.u32(user_row + 4);
    kept(26, std::uint32_t(p.own_club));
    // The chairman takes part when his club's rating shape (a byte, mod 20) is at least the story's number mod 5.
    std::uint32_t involved = 0;
    c.set(3, 0);
    if (p.chairman != 0) {
        const auto club_of_chairman = c.u32(p.chairman + 0x1c);
        compare_logical(c, club_of_chairman, 0);
        if (club_of_chairman != 0) {
            const auto rating = signed_byte(c.u8(club_of_chairman + 0x36));
            const auto rating_rest = std::int32_t(rating) % 20, count_rest = p.count % 5;
            c.set(4, std::uint32_t(p.count / 5) * 5);
            c.set(5, std::uint32_t(rating_rest));
            c.set(6, std::uint32_t(std::int32_t(rating) / 20));
            if (compare(c, rating_rest, count_rest) >= 0) involved = 1;
        }
    }
    c.set(3, involved);
    p.chair = std::int8_t(involved);
    kept(25, std::uint32_t(p.chair));

    // The short story (one line) or the long one (a line and a second paragraph).
    const bool is_full = compare(c, std::int8_t(c.u8(frame.at(0x3c3)))) != 0;
    if (is_full) {
        if (compare(c, p.about_board) == 0) p.long_fans();
        else p.long_board();
        p.paragraph();
    } else {
        if (compare(c, p.about_board) == 0) p.short_fans();
        else p.short_board();
        p.out.set(p.text, 0x1066d9c4);
    }
    // Both ways end with the links: the club (long and short name), then each person named in the text.
    p.out.link(p.club + 0x1ee, link_kind::club, p.club, 0x106726bc);
    p.out.link(p.club + 0x221, link_kind::club, p.club, 0x106726d0);
    if (compare_logical(c, p.subject) != 0 && p.out.named(p.subject, c.u32(subject_first), c.u32(subject_middle), c.u32(subject_last), 0x106726f0)) {
        p.out.link(frame.at(0xc0), link_kind::staff, p.subject, 0x1067270c);
        p.out.link(frame.at(0x48), link_kind::staff, p.subject, 0x10672720);
    }
    if (compare_logical(c, p.chairman) != 0 &&
        p.out.named(p.chairman, c.u32(chairman_first), c.u32(chairman_middle), c.u32(chairman_last), 0x10672740))
        p.out.link(frame.at(0x2a0), link_kind::staff, p.chairman, 0x1067275c);
    if (compare_logical(c, p.manager) != 0 && p.out.named(p.manager, c.u32(manager_name), c.u32(manager_middle), c.u32(manager_last), 0x1067277c)) {
        p.out.link(frame.at(0x1b0), link_kind::staff, p.manager, 0x10672798);
        p.out.link(frame.at(0x138), link_kind::staff, p.manager, 0x106727ac);
    }
}
}
