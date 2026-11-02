// Press reaction to a staff signing: fans or players celebrate (or a spokesman does), in a full
// form with the club and the outlet, or a short one.
#include "media_text_mid.hpp"
#include "media_text_mid_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::text;

namespace {
constexpr std::uint32_t amount_text = 0x100d6300, sprintf_entry = 0x1077b720, unknown_spokesman_phrase = 0x40404;
}

void signing_reaction_text(Cpu& c, std::uint32_t record, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x106a6230, 0x20e0, {.first_gpr = 13});
    keep(c, {{15, record}, {13, c.toc(user_slot)}, {17, c.toc(slot::clubs)}, {19, output}, {14, c.toc(staff_slot)}, {18, c.toc(slot::pool)}});
    RecordReader read{.c = c, .record = record, .cursor = 16};
    c.put8(frame.at(0x2103), full);
    Kept<16> cursor(c, 0);
    Kept<31> staff_id(c, read.at(0x106a6270));
    Kept<30> first(c, read.at(0x106a6284));
    Kept<29> middle(c, read.at(0x106a6298));
    Kept<28> last(c, read.at(0x106a62ac));
    Kept<27> club_id(c, read.at(0x106a62c0));
    Kept<26> detail(c, read.at(0x106a62d4));
    Kept<25> outlet_kind(c, read.at(0x106a62e8));
    const auto region = frame.at(0x2080), amount_id = frame.at(0x207c);
    c.put32(region, read.at(0x106a62fc));
    c.put32(amount_id, read.at(0x106a6310));
    Kept<24> variant(c, read.at(0x106a6324));
    read.at(0x106a6338);  // a field the original reads and drops
    Kept<23> spokesman(c, c.r(23));  // (set below, on one side only)
    Kept<22> s_first(c, c.r(22));  // (set below, on one side only)
    Kept<21> s_middle(c, c.r(21));  // (set below, on one side only)
    Kept<20> s_last(c, c.r(20));  // (set below, on one side only)
    const auto spokesman_name = frame.at(0x64);
    Program program{c};
    // Variant 2 has a spokesman: his id and three name parts follow in the record.
    if (compare(c, std::int32_t(std::uint32_t(variant)), 2) == 0) {
        spokesman = read.at(0x106a6350);
        s_first = read.at(0x106a6364);
        s_middle = read.at(0x106a6378);
        s_last = read.at(0x106a638c);
        program.call(entry::person_name, 0x106a63b4, {s_first, s_middle, s_last, spokesman_name, 0x190, 1, spokesman});
    } else {
        spokesman = 0;
        s_first = 0;
        s_middle = 0;
        s_last = 0;
        program.call(sprintf_entry, 0x106a63d8, {spokesman_name, c.toc(slot::pool) + unknown_spokesman_phrase});
    }
    const auto fan_flag = frame.at(0x2078), o_first = frame.at(0x2070), o_middle = frame.at(0x206c), o_id = frame.at(0x2074);
    c.put32(fan_flag, read.at(0x106a63e8));
    c.put32(o_id, read.at(0x106a63fc));
    c.put32(o_first, read.at(0x106a6410));
    c.put32(o_middle, read.at(0x106a6424));
    cursor = read.at(0x106a6434, false);
    Kept<15> club_held(c, table(c, -0x7be8) + std::uint32_t(club_id) * club_size);
    const Club club{c, club_held};
    const auto name = frame.at(0xb54), short_name = frame.at(0x9c4), other_name = frame.at(0x834), amount = frame.at(0x44c),
               outlet = frame.at(0x640), marker = frame.at(0x60), text = frame.at(0xce4);
    program.call(entry::person_name, 0x106a6468, {first, middle, last, name, 0x190, 1, staff_id});
    program.call(entry::person_name, 0x106a6488, {first, middle, last, short_name, 0x190, 5, staff_id});
    program.call(entry::person_name, 0x106a64a8, {c.u32(o_first), c.u32(o_middle), cursor, other_name, 0x190, 5, c.u32(o_id)});
    cursor = program.call(amount_text, 0x106a64c0, {extended(std::int16_t(c.u32(amount_id))), amount, 0xc8, 1});
    describe_outlet(c, {0x106a65c0, outlet, frame.at(0x258), marker}, outlet_kind, club_id, c.u32(region));
    const Output out{c, output};
    const auto count = signed_byte(cursor);
    if (compare(c, signed_byte(c.u8(frame.at(0x2103)))) != 0) {
        const auto spoken = signed_byte(c.u8(marker));
        const Club& one = club;
        const auto manager_zero = [&] { return compare(c, std::int32_t(std::uint32_t(detail))) == 0; };
        const std::uint32_t player_record = c.u32(club.at + 0x3c);
        const auto player_marker = [&] { return signed_byte(c.u8(player_record + 0x4b)); };
        const auto player_name = player_record + 0x18;
        const auto against_two = compare(c, std::int32_t(std::uint32_t(variant)), 2);
        if (against_two == 0) {
            if (compare(c, std::int32_t(c.u32(fan_flag))) != 0) {
                if (manager_zero()) {
                    { StackWords{c} << one.short_marker() << one.short_marker() << one.long_name() << spokesman_name << player_name << name << one.short_name(); format(c, text, 0x4084d, 0x106a6a9c, {one.long_marker(), one.long_marker(), 0x64, player_marker(), player_marker(), 0x64}); }
                } else {
                    { StackWords{c} << one.long_name() << spokesman_name << player_name << name; format(c, text, 0x409aa, 0x106a6b04, {one.long_marker(), one.long_marker(), 0x64, player_marker(), player_marker(), 0x64}); }
                }
            } else if (manager_zero()) {
                { StackWords{c} << spokesman_name; format(c, text, 0x40a90, 0x106a6b4c, {one.long_marker(), one.long_marker(), 0x64, 0x64, one.long_name(), name}); }
            } else {
                { StackWords{c} << one.short_marker() << one.short_marker() << one.long_name() << name << player_name << spokesman_name << one.short_name(); format(c, text, 0x40b72, 0x106a6bd4, {one.long_marker(), one.long_marker(), 0x64, player_marker(), player_marker(), 0x64}); }
            }
        } else if (against_two < 0 && compare(c, std::int32_t(std::uint32_t(variant)), 1) >= 0) {
            if (compare(c, std::int32_t(c.u32(fan_flag))) != 0) {
                { StackWords{c} << name; format(c, text, 0x40411, 0x106a68dc, {one.long_marker(), one.long_marker(), 0x64, 0x64, one.long_name(), other_name}); }
            } else {
                const auto detail_kind = zero_one_or_other(c, std::int32_t(std::uint32_t(detail)));
                if (detail_kind == 0) {
                    // The original works the club's marker out in r13 on the way and leaves it there.
                    Kept<13> scratch(c, one.short_marker());
                    { StackWords{c} << spoken << 0x64 << one.short_marker() << one.short_marker() << one.long_name() << amount << name << outlet << other_name << one.short_name(); format(c, text, 0x4057e, 0x106a6988, {one.long_marker(), one.long_marker(), count, count, 0x64, spoken}); }
                } else {
                    { StackWords{c} << one.short_marker() << one.short_marker() << one.long_name() << other_name << amount << name << one.short_name(); format(c, text, 0x40729, 0x106a69fc, {one.long_marker(), one.long_marker(), 0x64, count, count, 0x64}); }
                }
            }
        } else {
            program.call(sprintf_entry, 0x106a6bec, {text, c.toc(slot::pool) + 0x40d63, name});
        }
        out.set(text, 0x106a6bf8);
        out.link(club.short_name(), link_kind::club, club.at, 0x106a6c0c);
        out.link(club.long_name(), link_kind::club, club.at, 0x106a6c20);
        if (compare(c, std::int32_t(std::uint32_t(variant)), 2) == 0)
            out.staff_link(staff_at(c, spokesman), s_first, s_middle, s_last, spokesman_name, 0x106a6c48, 0x106a6c6c);
        const auto member = staff_at(c, staff_id);
        if (out.named(member, first, middle, last, 0x106a6c8c)) {
            out.link(name, link_kind::staff, member, 0x106a6cb0);
            out.link(short_name, link_kind::staff, member, 0x106a6ccc);
        }
        return;
    }
    program.call(entry::person_name, 0x106a6cf0, {first, middle, last, name, 0x190, 5, staff_id});
    if (compare(c, std::int32_t(std::uint32_t(variant)), 2) == 0 && compare(c, std::int32_t(std::uint32_t(spokesman)), -1) != 0)
        program.call(entry::person_name, 0x106a6d20, {s_first, s_middle, s_last, spokesman_name, 0x190, 5, spokesman});
    const auto against_two = compare(c, std::int32_t(std::uint32_t(variant)), 2);
    if (against_two == 0) {
        if (compare(c, std::int32_t(std::uint32_t(spokesman)), -1) == 0) format(c, text, 0x40e3f, 0x106a6d74, {0x64, name});
        else format(c, text, 0x40e83, 0x106a6d94, {0x64, spokesman_name, name});
    } else if (against_two < 0 && compare(c, std::int32_t(std::uint32_t(variant)), 1) >= 0) {
        format(c, text, 0x40dfe, 0x106a6d50, {0x64, name});
    } else {
        program.call(sprintf_entry, 0x106a6dac, {text, c.toc(slot::pool) + 0x40eea, name});
    }
    out.set(text, 0x106a6db8);
    if (compare(c, std::int32_t(std::uint32_t(variant)), 2) == 0)
        out.staff_link(staff_at(c, spokesman), s_first, s_middle, s_last, spokesman_name, 0x106a6de0, 0x106a6e04);
    out.staff_link(staff_at(c, staff_id), first, middle, last, name, 0x106a6e24, 0x106a6e48);
}
}
