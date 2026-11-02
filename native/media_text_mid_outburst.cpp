// A manager's outburst about a referee, a rival or the FA, as a press story: a log record with the
// manager (a staff member, by three name parts), his club, a second staff member and a variant
// 0-5 becomes the text, in a full form and a short one.
#include "media_text_mid.hpp"
#include "media_text_mid_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::text;

// Story text of record `record` for the manager it names; `full` picks the long text.
void manager_outburst_text(Cpu& c, std::uint32_t record, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x106a1860, 0x1cd0, {.first_gpr = 14});
    keep(c, {{16, record}, {15, c.toc(staff_slot)}, {28, c.toc(slot::pool)}, {14, full}, {29, output}});
    RecordReader read{.c = c, .record = record, .back = 0x106a1890};
    Kept<30> club_id(c, read(0));
    Kept<27> manager(c, read(1));
    Kept<26> manager_first(c, read(2));
    Kept<25> manager_middle(c, read(3));
    Kept<24> manager_last(c, read(4));
    Kept<23> variant(c, read(5));
    Kept<22> other(c, read(6));
    Kept<21> other_first(c, read(7));
    Kept<20> other_middle(c, read(8));
    Kept<19> other_last(c, read(9));
    Kept<18> amount_flag(c, read(0xb));
    Kept<17> number(c, read(0xa));
    Kept<16> player(c, read(0xc));
    Kept<31> fine(c, read(0xd));
    const auto club = club_at(c, club_id);
    club_id = club.at;
    const auto player_name = frame.at(0x250);
    if (compare(c, std::int32_t(std::uint32_t(player)), -1) != 0)
        Program{c}.call(entry::player_name, 0x106a19d0, {table(c, -0x7b98) + std::uint32_t(player) * person_size, player_name, 0x78, 9});
    else c.put8(player_name, 0);
    const Output out{c, output};
    const auto manager_name = frame.at(0x5d4), manager_short = frame.at(0x444), other_name = frame.at(0x764), text = frame.at(0x8f4);
    Program program{c};
    const auto set_text = [&](std::uint32_t phrase, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        format(c, text, phrase, back, arguments);
    };
    if (compare(c, signed_byte(full)) != 0) {
        program.call(entry::person_name, 0x106a1a04, {manager_first, manager_middle, manager_last, manager_name, 0x190, 1, manager});
        program.call(entry::person_name, 0x106a1a24, {manager_first, manager_middle, manager_last, manager_short, 0x190, 5, manager});
        program.call(entry::person_name, 0x106a1a44, {other_first, other_middle, other_last, other_name, 0x190, 1, other});
        switch (variant_of(c, std::int32_t(std::uint32_t(variant)))) {
        case 0:
            set_text(0x3db15, 0x106a1aa8, {club.long_marker(), club.long_marker(), club.long_name(), manager_name, other_name, manager_short});
            break;
        case 1:
            if (compare(c, std::int32_t(std::uint32_t(number)), -1) != 0)
                { StackWords{c} << number << club.long_name() << manager_name << other_name << number << club.short_name() << other_name << number; format(c, text, 0x3dc34, 0x106a1b20, {club.long_marker(), club.long_marker(), 0x64, number, club.short_marker(), club.short_marker()}); }
            else
                { StackWords{c} << manager_name << other_name << amount_flag << club.short_name() << other_name << amount_flag; format(c, text, 0x3ddcf, 0x106a1b84, {club.long_marker(), club.long_marker(), 0x64, club.short_marker(), club.short_marker(), club.long_name()}); }
            break;
        case 2:
            { StackWords{c} << manager_short << club.short_name(); format(c, text, 0x3df59, 0x106a1bd0, {club.long_marker(), club.long_marker(), club.short_marker(), club.short_marker(), club.long_name(), manager_name}); }
            break;
        case 3:
            set_text(0x3e0ef, 0x106a1c00, {club.long_marker(), club.long_marker(), club.long_name(), manager_name, manager_short});
            break;
        case 4:
            set_text(0x3e252, 0x106a1c30, {club.long_marker(), club.long_marker(), club.long_name(), manager_name, manager_short});
            break;
        case 5: {
            // A fine of 25,000 shown as an amount.
            const auto amount = frame.at(0x58);
            c.put32(amount, 0x61a8);
            program.call(entry::to_amount, 0x106a1c48, {amount, 6});
            program.call(entry::format_amount, 0x106a1c60, {frame.at(0x5c), 0x1f4, c.r(3), c.r(4), 6});
            { StackWords{c} << manager_name << manager_short << fine; format(c, text, 0x3e3d1, 0x106a1ca4, {club.long_marker(), club.long_marker(), 0x64, fine, club.long_name(), frame.at(0x5c)}); }
            break;
        }
        default: break;
        }
        out.set(text, 0x106a1cb0);
        out.link(club.short_name(), link_kind::club, club.at, 0x106a1cc4);
        out.link(club.long_name(), link_kind::club, club.at, 0x106a1cd8);
        out.staff_link(staff_at(c, other), other_first, other_middle, other_last, other_name, 0x106a1cf8, 0x106a1d1c);
        out.staff_link(staff_at(c, manager), manager_first, manager_middle, manager_last, manager_name, 0x106a1d3c, 0x106a1d60);
        return;
    }
    program.call(entry::person_name, 0x106a1d84, {manager_first, manager_middle, manager_last, manager_name, 0x190, 5, manager});
    program.call(entry::person_name, 0x106a1da4, {other_first, other_middle, other_last, other_name, 0x190, 5, other});
    switch (variant_of(c, std::int32_t(std::uint32_t(variant)))) {
    case 0: set_text(0x3e54d, 0x106a1df0, {manager_name, other_name}); break;
    case 1: set_text(0x3e599, 0x106a1e08, {manager_name}); break;
    case 2: set_text(0x3e5cf, 0x106a1e20, {manager_name}); break;
    case 3: set_text(0x3e617, 0x106a1e38, {manager_name}); break;
    case 4: set_text(0x3e655, 0x106a1e50, {manager_name}); break;
    case 5: set_text(0x3e69f, 0x106a1e80, {club.short_marker(), club.short_marker(), 0x64, club.short_name(), manager_name}); break;
    default: break;
    }
    out.set(text, 0x106a1e8c);
    out.staff_link(staff_at(c, manager), manager_first, manager_middle, manager_last, manager_name, 0x106a1eac, 0x106a1ed0);
    out.link(club.short_name(), link_kind::club, club.at, 0x106a1ee4);
}
}
