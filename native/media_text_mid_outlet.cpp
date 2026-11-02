// Stories a press outlet runs about a club's player or staff: the record names the staff,
// the club, the outlet and what was said; the text depends on the story kind and on the replies the user gave.
#include "media_text_mid.hpp"
#include "media_text_mid_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::text;

namespace {
constexpr std::uint32_t amount_text = 0x100d6300, joined_phrase = 0xc024, single_phrase = 0x3f438, sprintf_entry = 0x1077b720;
// The player of a club (the record its +0x3c points to, read where a story uses it): the name at +0x18 of
// his record, the marker byte at +0x4b.
struct Player {
    Cpu& c;
    std::uint32_t club;
    std::uint32_t at() const { return c.u32(club + 0x3c); }
    std::uint32_t name() const { return at() + 0x18; }
    std::uint32_t marker() const { return signed_byte(c.u8(at() + 0x4b)); }
};
bool is_users_manager_staff(Cpu& c, std::uint32_t member) {
    const auto base = c.toc(user_slot);
    const auto row = base + std::uint32_t(std::int32_t(c.i16(base + 0x2c58))) * 0x2c0;
    return compare(c, c.u32(row + 4) == member ? 1 : 0) != 0;
}
}

// Report in a press outlet on a club's story (kind 0 a player's remarks, 1 thanks, 2 a dismissal), with the user's reply.
void outlet_report_text(Cpu& c, std::uint32_t record, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x106a44d0, 0x4600, {.first_gpr = 13});
    constexpr std::uint32_t backs_array[]{0x106a450c, 0x106a4520, 0x106a4534, 0x106a4548, 0x106a455c, 0x106a4570, 0x106a4584, 0x106a4598,
                                          0x106a45ac, 0x106a45c0, 0x106a45d4, 0x106a45e8, 0x106a45fc, 0x106a4610, 0x106a4620, 0x106a4634,
                                          0x106a4648, 0x106a465c, 0x106a4670, 0x106a4684, 0x106a4698, 0x106a46a8};
    keep(c, {{27, record}, {29, c.toc(user_slot)}, {30, c.toc(slot::clubs)}, {15, output}, {31, c.toc(slot::pool)}});
    c.put8(frame.at(0x4623), full);
    RecordReader read{.c = c, .record = record, .cursor = 14, .backs = backs_array};
    Kept<26> manager(c, read.next());
    Kept<25> m_first(c, read.next());
    Kept<24> m_middle(c, read.next());
    Kept<23> m_last(c, read.next());
    Kept<22> other(c, read.next());
    const auto o_first = frame.at(0x45ac), o_middle = frame.at(0x45a8), o_last = frame.at(0x45a4);
    c.put32(o_first, read.next());
    c.put32(o_middle, read.next());
    c.put32(o_last, read.next());
    Kept<21> club_id(c, read.next());
    const auto flags = frame.at(0x45a0), region = frame.at(0x459c), amount_id = frame.at(0x4598), age = frame.at(0x4594), undecided = frame.at(0x4590);
    c.put32(flags, read.next());
    Kept<20> detail(c, read.next());
    Kept<19> outlet_kind(c, read.next());
    c.put32(region, read.next());
    read.next();  // a field the original reads and drops
    c.put32(amount_id, read.next());
    Kept<18> story(c, read.next());
    c.put32(age, read.next());
    Kept<13> mood(c, read.next());
    Kept<17> reply(c, read.next());
    Kept<16> attitude(c, read.next());
    c.put32(undecided, read.next());
    Kept<14> choice(c, read.next(false));
    const Club club{c, table(c, -0x7be8) + std::uint32_t(club_id) * club_size};
    Kept<27> club_held(c, club.at);
    Program program{c};
    const auto manager_name = frame.at(0x968), manager_short = frame.at(0x7d8), other_name = frame.at(0x648), amount = frame.at(0x260),
               outlet = frame.at(0x454), marker = frame.at(0x68), story_text = frame.at(0x1e80), reply_text = frame.at(0xaf8), text = frame.at(0x3208);
    program.call(entry::person_name, 0x106a46dc, {m_first, m_middle, m_last, manager_name, 0x190, 1, manager});
    program.call(entry::person_name, 0x106a46fc, {m_first, m_middle, m_last, manager_short, 0x190, 5, manager});
    program.call(entry::person_name, 0x106a471c, {c.u32(o_first), c.u32(o_middle), c.u32(o_last), other_name, 0x190, 5, other});
    program.call(amount_text, 0x106a4734, {extended(std::int16_t(c.u32(amount_id))), amount, 0xc8, 1});
    Kept<28> amount_flag(c, signed_byte(c.r(3)));
    describe_outlet(c, {0x106a4834, outlet, frame.at(0x6c), marker}, outlet_kind, club_id, c.u32(region));
    const Output out{c, output};
    if (compare(c, signed_byte(c.u8(frame.at(0x4623)))) == 0) {
        program.call(entry::person_name, 0x106a51c4, {m_first, m_middle, m_last, manager_name, 0x190, 5, manager});
        const int which = story_of(c, std::int32_t(std::uint32_t(story)));
        if (which == 0) {
            if (zero_one_or_other(c, std::int32_t(std::uint32_t(detail))) == 0) format(c, text, 0x40316, 0x106a5214, {manager_name});
            else format(c, text, 0x40355, 0x106a522c, {manager_name});
        } else if (which == 1) format(c, text, 0x4038c, 0x106a5244, {manager_name});
        else if (which == 2) format(c, text, 0x403c4, 0x106a525c, {manager_name});
        out.set(text, 0x106a5268);
        out.staff_link(staff_at(c, manager), m_first, m_middle, m_last, manager_name, 0x106a528c, 0x106a52b4);
        return;
    }
    const Player player{c, club.at};
    const auto spoken = signed_byte(c.u8(marker));
    const auto count = signed_byte(amount_flag);
    const int which = story_of(c, std::int32_t(std::uint32_t(story)));
    if (which == 0) {
        if (compare(c, std::int32_t(c.u32(flags) & 1)) != 0) {
            StackWords{c} << club.short_name();
            format(c, story_text, 0x3f4a9, 0x106a4b64, {spoken, spoken, club.short_marker(), club.short_marker(), outlet, manager_name});
        } else if (compare(c, std::int32_t(c.u32(flags) & 4)) != 0) {
            StackWords{c} << club.short_name() << manager_name << player.name() << outlet;
            format(c, story_text, 0x3f5b5, 0x106a4be0, {club.short_marker(), club.short_marker(), player.marker(), player.marker(), spoken, spoken});
        } else {
            const auto detail_kind = zero_one_or_other(c, std::int32_t(std::uint32_t(detail)));
            if (detail_kind == 0) {
                StackWords{c} << spoken << manager_name << player.name() << club.short_name() << outlet << other_name;
                format(c, story_text, 0x3f70b, 0x106a4c7c, {0x64, player.marker(), player.marker(), club.short_marker(), club.short_marker(), spoken});
            } else if (detail_kind == 1) {
                StackWords{c} << player.marker() << player.marker() << outlet << club.short_name() << amount << manager_name << player.name() << other_name;
                format(c, story_text, 0x3f8bb, 0x106a4d0c, {spoken, spoken, club.short_marker(), club.short_marker(), count, count});
            } else {
                // The original works the markers out in r13 on the way and leaves the club's in it.
                mood = club.short_marker();
                StackWords{c} << player.marker() << player.marker() << club.short_marker() << club.short_marker() << outlet << club.short_name() << amount
                              << manager_name << player.name() << club.short_name() << other_name << manager_name;
                format(c, story_text, 0x3fa7e, 0x106a4dc4, {spoken, spoken, club.short_marker(), club.short_marker(), count, count});
            }
        }
        if (compare(c, std::int32_t(std::uint32_t(reply))) == 0) {
            program.call(sprintf_entry, 0x106a4f74, {text, c.toc(slot::pool) + single_phrase, story_text});
        } else {
            const bool users = is_users_manager_staff(c, staff_at(c, manager));
            const auto answer = reply_of(c, std::int32_t(std::uint32_t(reply)));
            const bool chosen = compare(c, std::int32_t(std::uint32_t(choice))) != 0;
            if (users) {
                if (answer == 1) format(c, reply_text, 0x3fc95, 0x106a4e4c, {0x64, manager_name});
                else if (answer == 2) {
                    if (chosen) format(c, reply_text, 0x3fcf5, 0x106a4e6c, {manager_name});
                    else format(c, reply_text, 0x3fd4e, 0x106a4e84, {manager_name});
                } else if (answer == 3) format(c, reply_text, 0x3f15b, 0x106a4e30, {});
            } else {
                if (answer == 1) format(c, reply_text, 0x3fdd9, 0x106a4ef4, {0x64, 0x64, other_name, manager_name});
                else if (answer == 2) {
                    if (chosen) format(c, reply_text, 0x3fe5e, 0x106a4f20, {0x64, 0x64, other_name, manager_name});
                    else format(c, reply_text, 0x3feda, 0x106a4f44, {0x64, 0x64, other_name, manager_name});
                } else if (answer == 3) format(c, reply_text, 0x3f259, 0x106a4ed0, {club.long_marker(), club.long_marker(), club.short_name(), other_name});
            }
            program.call(sprintf_entry, 0x106a4f5c, {text, c.toc(slot::pool) + joined_phrase, story_text, reply_text});
        }
    } else if (which == 1) {
        const auto thanks = [&](std::uint32_t back) {
            format(c, text, 0x3ff88, back, {club.long_marker(), club.long_marker(), manager_name, club.long_name()});
        };
        // (A thanked player whose attitude is 1 or 2 gets this text first, and the mood's text replaces it.)
        if (compare(c, std::int32_t(std::uint32_t(attitude)), 1) == 0 || compare(c, std::int32_t(std::uint32_t(attitude)), 2) == 0) thanks(0x106a4fb0);
        if (compare(c, std::int32_t(std::uint32_t(mood)), 0x15) < 0) format(c, text, 0x4002d, 0x106a4fcc, {manager_name});
        else if (compare(c, std::int32_t(std::uint32_t(mood)), 0x22) > 0) format(c, text, 0x400ba, 0x106a4fec, {manager_name});
        else thanks(0x106a5018);
    } else if (which == 2) {
        const auto dismissed = [&](std::uint32_t phrase, std::uint32_t back) {
            format(c, text, phrase, back, {club.long_marker(), club.long_marker(), manager_name, club.long_name()});
        };
        if (compare(c, std::int32_t(c.u32(undecided))) != 0 &&
            (compare_logical(c, std::uint32_t(attitude) - 1, 1) <= 0 || compare(c, std::int32_t(std::uint32_t(attitude)), 3) == 0))
            dismissed(0x40135, 0x106a5064);
        else if (compare(c, std::int32_t(c.u32(age)), 0x19) > 0) format(c, text, 0x401e7, 0x106a5088, {manager_name});
        else dismissed(0x40259, 0x106a50b4);
    }
    out.set(text, 0x106a50c0);
    out.link(club.short_name(), link_kind::club, club.at, 0x106a50d4);
    out.link(club.long_name(), link_kind::club, club.at, 0x106a50e8);
    const auto member = staff_at(c, manager);
    if (out.named(member, m_first, m_middle, m_last, 0x106a510c)) {
        out.link(manager_name, link_kind::staff, member, 0x106a5134);
        out.link(manager_short, link_kind::staff, member, 0x106a5154);
    }
    out.staff_link(staff_at(c, other), c.u32(o_first), c.u32(o_middle), c.u32(o_last), other_name, 0x106a5178, 0x106a51a0);
}
}
